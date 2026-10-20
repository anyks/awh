/**
 * @file stream.cpp
 * @date 2026-07-26
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * @brief Сценарий измерения пропускной способности потоковой передачи —
 *        передача крупного объёма данных блоками по одному подключению петлевого интерфейса
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <chrono>
#include <cstdio>
#include <vector>

/**
 * Подключаем заголовочный файл бенчмарков сетевого движка
 */
#include "io.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён бенчмарков сетевого движка
 */
using namespace awh::benchmark::io;

/**
 * @brief Внутренние параметры и сценарии бенчмарков потоковой передачи
 *
 */
namespace {
	/**
	 * @brief Порог пропускной способности потоковой передачи в мебибайтах в секунду
	 *
	 * @details Порог откалиброван по отладочной сборке репозитория с трёхкратным
	 *          запасом: показатель ловит появление лишнего копирования блока
	 *          на пути от очереди события до сокета
	 *
	 */
	static constexpr double STREAM_THRESHOLD = 3000.0;
	/**
	 * @brief Порог количества выделений памяти на один переданный блок
	 *
	 * @details Установившаяся передача выделений памяти не выполняет вовсе:
	 *          очередь отправки события выходит на рабочий объём за первые
	 *          блоки и дальше переиспользуется
	 *
	 */
	static constexpr double ALLOCATIONS_THRESHOLD = 1.0;
	/**
	 * @brief Порог количества системных вызовов на один блок
	 *
	 * @details Здесь движок работает так, как надо: на блок приходится одна
	 *          передача, один приём и малая доля возврата за готовностью, потому
	 *          что очередь отправки опустошается в пределах одного пробуждения
	 *          ядра. Порог стоит сторожем: его превышение означает, что возврат за
	 *          готовностью снова выполняется на каждый блок
	 *
	 */
	static constexpr double SYSCALLS_THRESHOLD = 2.5;

	/**
	 * @brief Предельное время подготовки или передачи в секундах
	 *
	 */
	static constexpr double STREAM_DEADLINE = 30.0;

	/**
	 * @brief Структура состояния полного потока
	 *
	 */
	typedef struct State {
		// Признак успешного подключения клиента
		bool ready;
		// Признак открытого окна передачи
		bool running;
		// Признак полного завершения передачи
		bool stop;
		// Признак завершающей очистки
		bool closing;
		// Причина отказа
		const char * failure;
		// Идентификатор принятой стороны
		awh::event::id_t peer;
		// Остаток единственного незавершённого блока
		size_t pending;
		// Объём поставленных в очередь блоков
		size_t queued;
		// Объём подтверждённой записи
		size_t written;
		// Объём принятых октетов
		size_t received;
		// Начало измерения передачи
		std::chrono::steady_clock::time_point start;
		// Конец измерения передачи
		std::chrono::steady_clock::time_point finish;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit State() noexcept :
		 ready(false), running(false), stop(false), closing(false), failure(nullptr), peer(0),
		 pending(0), queued(0), written(0), received(0), start{}, finish{} {}
	} state_t;

	/**
	 * @brief Функция завершения окна после подтверждения обеих сторон
	 *
	 * @param state состояние потока
	 *
	 */
	static void complete(state_t & state) noexcept {
		// Останавливаем часы только после полной записи и полного приёма
		if(!state.stop && (state.failure == nullptr) && (state.written == STREAM_VOLUME) && (state.received == STREAM_VOLUME) && (state.pending == 0)){
			// Запоминаем конец измерения
			state.finish = std::chrono::steady_clock::now();
			// Закрываем учёт в той же точке, что и часы
			awh::benchmark::counting(false);
			// Закрываем учёт системных вызовов
			awh::benchmark::syscall::counting(false);
			// Подтверждаем завершение обеих сторон
			state.stop = true;
		}
	}
	/**
	 * @brief Функция передачи полного объёма через подготовленное подключение
	 *
	 * @details Окно открывается после connect и accept, перед первой отправкой.
	 *          Следующий блок подаётся после подтверждения записи предыдущего.
	 *          Ошибка или неполный объём возвращают пустой результат
	 *
	 * @return итоги полной передачи либо пустой результат
	 *
	 */
	static outcome_t transfer() noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Состояние прогона сценария
		state_t state;
		// Блок передаваемых данных
		static vector <uint8_t> chunk(STREAM_CHUNK, 0x5A);
		// Создаём движок событий
		awh::engine::io_t io;
		// Получаем свободный порт
		const uint16_t number = port();
		// Ограничиваем общую подготовку подключения
		const auto prepared = std::chrono::steady_clock::now();
		// Не допускаем успешный результат при ошибке движка
		const awh::engine::callback::error_t error = [&state]([[maybe_unused]] const awh::event::id_t eid, [[maybe_unused]] const awh::event::error_t code, [[maybe_unused]] const string & message) noexcept -> void {
			// Сохраняем первый отказ до ожидаемой очистки
			if(!state.closing && !state.stop && (state.failure == nullptr))
				// Запоминаем отказ движка
				state.failure = "engine-error";
		};
		// Прерываем незавершённую передачу при разрыве связи
		const awh::engine::callback::event_t closed = [&state]([[maybe_unused]] const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			// Отличаем ожидаемую очистку от преждевременного закрытия
			if(!state.closing && !state.stop && (state.failure == nullptr) && ((action == awh::event::action_t::CLOSE) || (action == awh::event::action_t::DISCONNECT)))
				// Не выдаём неполный поток за успешный
				state.failure = "connection-closed";
		};
		// Создаём слушающее событие
		const awh::event::id_t server = io.event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
		// Проверяем подготовку сервера
		if((server == 0) || !io.setSourcePort(server, number) || !io.initialize() || !io.setOptions(server, options()) || !io.setAddress(server, awh::event::address_t::IPV4, "127.0.0.1"))
			// Запоминаем отказ подготовки
			state.failure = "server-setup";
		// Подписываем сервер на ошибки
		io.on(server, error);
		// Настраиваем принятую сторону до открытия измеряемого окна
		io.on(server, static_cast <awh::engine::callback::accept_t> ([&io, &state, &error, &closed]([[maybe_unused]] const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
			// Не допускаем второе подключение
			if(state.peer != 0){
				// Отмечаем нарушение сценария
				state.failure = "unexpected-accept";
				// Освобождаем лишнее событие
				io.destroy(cid);
				// Завершаем обработку
				return;
			}
			// Сохраняем принятую сторону для проверки готовности
			state.peer = cid;
			// Проверяем обязательные опции
			if(!io.setOptions(cid, options()))
				// Запоминаем отказ настройки
				state.failure = "peer-options";
			// Подписываем приёмник на ошибки
			io.on(cid, error);
			// Подписываем приёмник на разрыв связи
			io.on(cid, closed);
			// Считаем принятый объём без разбора полезной нагрузки
			io.on(cid, [&state]([[maybe_unused]] const awh::event::id_t eid, const uint8_t *, const size_t size) noexcept -> void {
				// Не продолжаем подсчёт после отказа или при очистке
				if(state.closing || (state.failure != nullptr))
					// Завершаем обработку
					return;
				// Не допускаем нулевой, преждевременный или избыточный приём
				if(!state.running || (size == 0) || (size > (STREAM_VOLUME - state.received))){
					// Не принимаем повреждённый счёт за полный поток
					state.failure = "read-size";
					// Завершаем обработку
					return;
				}
				// Учитываем только фактический приём
				state.received += size;
				// Проверяем завершение обеих сторон
				::complete(state);
			});
		}));
		// Проверяем запуск сервера
		if((state.failure == nullptr) && (!io.commit(server) || !io.listen(server, BACKLOG) || !io.launch(server)))
			// Запоминаем отказ запуска
			state.failure = "server-launch";
		// Создаём клиента
		const awh::event::id_t client = io.event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
		// Проверяем подготовку клиента
		if((state.failure == nullptr) && ((client == 0) || !io.setTargetPort(client, number) || !io.setOptions(client, options() | awh::event::options::HARD_CLOSE) || !io.setAddress(client, awh::event::address_t::IPV4, "0.0.0.0") || !io.setTarget(client, "127.0.0.1")))
			// Запоминаем отказ подготовки
			state.failure = "client-setup";
		// Подписываем клиента на ошибки
		io.on(client, error);
		// Подписываем клиента на разрыв связи
		io.on(client, closed);
		// Подтверждаем подключение без преждевременного запуска отправки
		io.on(client, static_cast <awh::engine::callback::connect_t> ([&state]([[maybe_unused]] const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Сохраняем отказ подключения
			if(!ok && (state.failure == nullptr))
				// Прерываем подготовку
				state.failure = "connect-failed";
			// Запоминаем клиентскую готовность
			state.ready = ok;
		}));
		// Учитываем каждое подтверждение записи, включая частичные записи блока
		io.on(client, static_cast <awh::engine::callback::write_t> ([&io, &state](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Не продолжаем отправку после отказа или при очистке
			if(state.closing || (state.failure != nullptr))
				// Завершаем обработку
				return;
			// Нулевая запись сообщает также о временном отсутствии готовности
			if(size == 0)
				// Сохраняем блок до следующего подтверждения или ошибки движка
				return;
			// Проверяем размер подтверждённой записи
			if(!state.running || (size > state.pending)){
				// Не скрываем избыточное подтверждение обнулением остатка
				state.failure = "write-size";
				// Завершаем обработку
				return;
			}
			// Уменьшаем остаток текущего блока
			state.pending -= size;
			// Учитываем фактически записанный объём
			state.written += size;
			// Дожидаемся всего текущего блока
			if(state.pending != 0)
				// Не подаём следующий блок раньше времени
				return;
			// Подаём следующий блок только при неполной очереди
			if(state.queued < STREAM_VOLUME){
				// Устанавливаем остаток до возможного синхронного callback
				state.pending = STREAM_CHUNK;
				// Учитываем новый блок до вызова движка
				state.queued += STREAM_CHUNK;
				// Проверяем принятие всего блока в очередь движка
				if(io.send(eid, chunk.data(), STREAM_CHUNK) != STREAM_CHUNK)
					// Не допускаем тихую потерю хвоста
					state.failure = "send-failed";
			}
			// Проверяем завершение обеих сторон
			::complete(state);
		}));
		// Проверяем запуск клиента
		if((state.failure == nullptr) && (!io.commit(client) || !io.connect(client) || !io.launch(client)))
			// Запоминаем отказ запуска
			state.failure = "client-launch";
		/**
		 * Дожидаемся готовности клиента и приёмника до начала измерения
		 */
		while((state.failure == nullptr) && (!state.ready || (state.peer == 0))){
			// Продвигаем подготовку с конечным ожиданием
			if(!io.poll(250))
				// Запоминаем отказ опроса
				state.failure = "setup-poll";
			// Ограничиваем подготовку подключения
			if(std::chrono::duration <double> (std::chrono::steady_clock::now() - prepared).count() >= STREAM_DEADLINE)
				// Прерываем незавершённую подготовку
				state.failure = "setup-deadline";
		}
		// Начинаем передачу только после полной подготовки
		if(state.failure == nullptr){
			// Разрешаем обработку данных
			state.running = true;
			// Включаем учёт выделений
			awh::benchmark::counting(true);
			// Включаем учёт системных вызовов
			awh::benchmark::syscall::counting(true);
			// Открываем окно перед первой отправкой
			state.start = std::chrono::steady_clock::now();
			// Устанавливаем остаток первого блока
			state.pending = STREAM_CHUNK;
			// Учитываем первый блок
			state.queued = STREAM_CHUNK;
			// Проверяем принятие первого блока
			if(io.send(client, chunk.data(), STREAM_CHUNK) != STREAM_CHUNK)
				// Прерываем неполную отправку
				state.failure = "send-failed";
		}
		/**
		 * Ожидаем весь поток с ограничением общего времени передачи
		 */
		while((state.failure == nullptr) && !state.stop){
			// Продвигаем обмен с конечным ожиданием
			if(!io.poll(250))
				// Прерываем отказавший опрос
				state.failure = "transfer-poll";
			// Ограничиваем передачу, включая зависший приёмник
			if(std::chrono::duration <double> (std::chrono::steady_clock::now() - state.start).count() >= STREAM_DEADLINE)
				// Прерываем незавершённый поток
				state.failure = "transfer-deadline";
		}
		// Закрываем учёт также при ошибке
		awh::benchmark::counting(false);
		// Закрываем учёт системных вызовов также при ошибке
		awh::benchmark::syscall::counting(false);
		// Проверяем полный объём и корректное окно
		if((state.failure == nullptr) && (!state.stop || (state.queued != STREAM_VOLUME) || (state.written != STREAM_VOLUME) || (state.received != STREAM_VOLUME) || (state.pending != 0) || (state.finish <= state.start)))
			// Отбрасываем неполный результат
			state.failure = "incomplete";
		// Снимаем показатели до очистки только у полного потока
		if(state.failure == nullptr){
			// Устанавливаем количество полных блоков
			result.operations = (STREAM_VOLUME / STREAM_CHUNK);
			// Устанавливаем точный объём
			result.bytes = STREAM_VOLUME;
			// Устанавливаем длительность передачи
			result.seconds = std::chrono::duration <double> (state.finish - state.start).count();
			// Снимаем показатели окружения
			collect(result);
		}
		// Отделяем ожидаемые закрытия от ошибок передачи
		state.closing = true;
		// Проверяем очистку всех событий, включая ошибочную подготовку
		if(!io.deinitialize()){
			// Не публикуем результат при отказе очистки
			result = outcome_t{};
			// Запоминаем отказ очистки
			state.failure = "cleanup-failed";
		}
		// Сообщаем первый отказ и фактические счётчики
		if(state.failure != nullptr)
			// Не выводим частичный объём как успешный поток
			::fprintf(stderr, "STREAM_BENCH_FAIL: %s queued=%zu written=%zu received=%zu pending=%zu\n", state.failure, state.queued, state.written, state.received, state.pending);
		// Подтверждаем полный объём вне окна замера
		else
			// Выводим контрольные счётчики
			::printf("STREAM_ACCOUNT queued=%zu written=%zu received=%zu pending=%zu\n", state.queued, state.written, state.received, state.pending);
		// Выводим только полный результат либо пустой исход
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона потоковой передачи
	 *
	 * @note Итоги снимаются с единственного прогона: показатели пропускной
	 *       способности и выделений памяти описывают одну и ту же передачу
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & measured() noexcept {
		// Итоги прогона сценария потоковой передачи
		static const outcome_t result = ::transfer();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария пропускной способности потоковой передачи
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t throughput() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем итоги прогона сценария потоковой передачи
		const outcome_t & outcome = ::measured();
		// Устанавливаем измеренное значение
		result.value = megabytes(outcome);
		// Убеждаемся, что сценарий выполнил хоть одну операцию
		validate(result, outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария учёта выделений памяти на один блок
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t allocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем итоги прогона сценария потоковой передачи
		const outcome_t & outcome = ::measured();
		// Устанавливаем измеренное значение
		result.value = perOperation(outcome);
		// Убеждаемся, что сценарий выполнил хоть одну операцию
		validate(result, outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	// Регистрируем сценарий пропускной способности потоковой передачи
	static const bool gThroughput = awh::benchmark::add(
		"net/io/stream/throughput", "МБ/с", STREAM_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::throughput
	);
	/**
	 * @brief Функция прогона сценария учёта системных вызовов на один блок
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t syscalls() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Если учёт системных вызовов недоступен
		if(!awh::benchmark::syscall::available()){
			// Отмечаем измерение как не выполнявшееся
			result.skipped = true;
			// Устанавливаем причину, по которой измерение не выполнялось
			result.reason = awh::benchmark::syscall::reason();
			// Выводим результат измерения
			return result;
		}
		// Получаем итоги прогона сценария
		const outcome_t & outcome = ::measured();
		// Устанавливаем измеренное значение
		result.value = perSyscall(outcome);
		// Убеждаемся, что сценарий выполнил хоть одну операцию
		validate(result, outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	// Регистрируем сценарий учёта выделений памяти на один блок
	static const bool gAllocations = awh::benchmark::add(
		"net/io/stream/allocations-per-chunk", "выделений", ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::allocations
	);
	// Регистрируем сценарий учёта системных вызовов на один блок
	static const bool gSyscalls = awh::benchmark::add(
		"net/io/stream/syscalls-per-chunk", "вызовов", SYSCALLS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::syscalls
	);
};
