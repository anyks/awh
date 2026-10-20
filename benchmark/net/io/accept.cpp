/**
 * @file accept.cpp
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
 * @brief Сценарий измерения установления соединений — полный цикл создания события,
 *        подключения, принятия его сервером и освобождения обеих сторон
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
 * @brief Внутренние параметры и сценарии бенчмарков установления соединений
 *
 */
namespace {
	/**
	 * @brief Количество циклов прогрева сценария
	 *
	 */
	static constexpr size_t WARMUP_ROUNDS = 200;
	/**
	 * @brief Порог скорости установления соединений в подключениях в секунду
	 *
	 * @details Порог откалиброван по отладочной сборке репозитория с двукратным
	 *          запасом: показатель ловит появление лишней работы на пути
	 *          создания и освобождения события подключения
	 *
	 */
	static constexpr double ACCEPT_THRESHOLD = 2500.0;
	/**
	 * @brief Порог количества выделений памяти на одно подключение
	 *
	 * @details Полный цикл короткоживущего соединения выполняет пятнадцать
	 *          выделений: состояние события, его подписки и записи учёта. Порог
	 *          задавался вплотную к измеренному значению в три десятка, но
	 *          работа по сокращению выделений опустила измеренное вдвое, и запас
	 *          у порога теперь двукратный. Снижать его до нового измеренного
	 *          значения стоит вместе со следующим замером, а не задним числом:
	 *          порог обязан подтверждаться прогоном, а не правкой отчёта
	 *
	 */
	static constexpr double ALLOCATIONS_THRESHOLD = 40.0;
	/**
	 * @brief Порог количества системных вызовов на одно подключение
	 *
	 * @details Порог задавался вплотную к измеренному значению, потому что счётчик
	 *          вызовов детерминирован: на той же нагрузке он совпадает от прогона
	 *          к прогону вплоть до единиц, и запас ему нужен только на разницу
	 *          между прогревом и замером. Измеренная тогда величина втрое
	 *          превышала необходимое: опрос конфигурации сети выполнялся на каждое
	 *          создание события. После его устранения измеренное составляет
	 *          пятнадцать вызовов - против пяти необходимых на создание сокета,
	 *          подключение, принятие и два закрытия, плюс один на установку
	 *          немедленного обрыва соединения. Порог оставлен прежним по той же
	 *          причине, что и порог выделений: понижать его следует замером, а не
	 *          правкой
	 *
	 */
	static constexpr double SYSCALLS_THRESHOLD = 34.0;
	/**
	 * @brief Шаг опроса событий сценария в миллисекундах
	 *
	 * @details Опрос с бесконечным ожиданием оставлял сценарий без выхода: любое
	 *          неполученное событие останавливало его насовсем и вместе с ним весь
	 *          набор бенчмарков. Конечный шаг превращает такую остановку в
	 *          диагностируемый исход
	 *
	 */
	static constexpr int32_t POLL_TIMEOUT = 250;
	/**
	 * @brief Предельный срок завершения одного круга подключения в секундах
	 *
	 * @details Круг на петлевом интерфейсе укладывается в доли миллисекунды, так
	 *          что срок задан с запасом в четыре порядка: он ловит не медленный
	 *          круг, а незавершающийся
	 *
	 */
	static constexpr double ROUND_DEADLINE = 5.0;

	/**
	 * @brief Структура состояния полного цикла подключения
	 *
	 */
	typedef struct State {
		// Признак подтверждения подключения клиента
		bool ready;
		// Причина прекращения прогона
		const char * failure;
		// Идентификатор текущего клиента
		awh::event::id_t client;
		// Идентификатор принятого подключения
		awh::event::id_t peer;
		// Количество успешных подключений клиента
		size_t connected;
		// Количество принятых сервером подключений
		size_t accepted;
		// Количество полностью освобождённых событий
		size_t closed;
		// Количество полных циклов прогрева
		size_t warmed;
		// Количество полных измеренных циклов
		size_t done;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit State() noexcept :
		 ready(false), failure(nullptr), client(0), peer(0), connected(0), accepted(0), closed(0), warmed(0), done(0) {}
	} state_t;

	/**
	 * @brief Функция прогона полных циклов установления соединений
	 *
	 * @details Следующий клиент создаётся после успешного подключения,
	 *          принятия сервером и фактического освобождения обеих сторон.
	 *          Отложенное удаление AWH завершается опросом вне callback.
	 *          Любой неполный прогон возвращает пустой результат
	 *
	 * @return итоги полного прогона либо пустой результат при отказе
	 *
	 */
	static outcome_t handshake() noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Состояние текущего цикла и общие счётчики
		state_t state;
		// Создаём движок событий
		awh::engine::io_t io;
		// Получаем свободный порт петлевого интерфейса
		const uint16_t number = port();
		// Создаём событие сервера
		const awh::event::id_t server = io.event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
		// Проверяем создание и настройку сервера
		if((server == 0) || !io.setSourcePort(server, number) || !io.initialize() || !io.setOptions(server, options()) || !io.setAddress(server, awh::event::address_t::IPV4, "127.0.0.1"))
			// Запоминаем отказ подготовки
			state.failure = "server-setup";
		// Подтверждаем принятие именно текущего цикла
		io.on(server, static_cast <awh::engine::callback::accept_t> ([&io, &state]([[maybe_unused]] const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
			// Не допускаем второй peer и приём вне активного цикла
			if((state.client == 0) || (state.peer != 0) || (state.failure != nullptr)){
				// Сохраняем первый отказ
				if(state.failure == nullptr)
					// Отмечаем нарушение последовательности
					state.failure = "unexpected-accept";
				// Освобождаем лишнее подключение
				io.destroy(cid);
				// Прекращаем обработку
				return;
			}
			// Сохраняем принятое событие до общего закрытия
			state.peer = cid;
			// Учитываем серверную сторону
			state.accepted++;
		}));
		// Запускаем слушающий сервер только после успешной подготовки
		if((state.failure == nullptr) && (!io.commit(server) || !io.listen(server, BACKLOG) || !io.launch(server)))
			// Запоминаем отказ запуска
			state.failure = "server-launch";
		// Момент начала измеряемой части
		auto start = std::chrono::steady_clock::time_point{};
		// Момент окончания измеряемой части
		auto finish = start;
		/**
		 * Выполняем полные циклы прогрева и измерения
		 */
		for(size_t round = 0; (round < (WARMUP_ROUNDS + ACCEPT_ROUNDS)) && (state.failure == nullptr); round++){
			// Открываем окно перед созданием первого измеряемого клиента
			if(round == WARMUP_ROUNDS){
				// Включаем учёт выделений
				awh::benchmark::counting(true);
				// Включаем учёт системных вызовов
				awh::benchmark::syscall::counting(true);
				// Запоминаем начало измерения
				start = std::chrono::steady_clock::now();
			}
			// Сбрасываем подтверждение предыдущего клиента
			state.ready = false;
			// Устанавливаем общий срок подключения и освобождения
			const auto tick = std::chrono::steady_clock::now();
			// Создаём очередного клиента
			state.client = io.event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем все настройки текущего клиента
			if((state.client == 0) || !io.setTargetPort(state.client, number) || !io.setOptions(state.client, options() | awh::event::options::HARD_CLOSE) || !io.setAddress(state.client, awh::event::address_t::IPV4, "0.0.0.0") || !io.setTarget(state.client, "127.0.0.1")){
				// Запоминаем отказ подготовки
				state.failure = "client-setup";
				// Прекращаем прогон
				break;
			}
			// Проверяем исход подключения и принадлежность текущему циклу
			io.on(state.client, static_cast <awh::engine::callback::connect_t> ([&state](const awh::event::id_t eid, const bool ok) noexcept -> void {
				// Не засчитываем отказ или повторный callback
				if(!ok || (eid != state.client) || state.ready){
					// Сохраняем первый отказ
					if(state.failure == nullptr)
						// Запоминаем отказ подключения
						state.failure = "connect-failed";
					// Прекращаем обработку
					return;
				}
				// Подтверждаем клиентскую сторону
				state.ready = true;
				// Учитываем успешное подключение
				state.connected++;
			}));
			// Проверяем запуск неблокирующего подключения
			if(!io.commit(state.client) || !io.connect(state.client) || !io.launch(state.client))
				// Запоминаем отказ запуска
				state.failure = "client-launch";
			/**
			 * Ожидаем подтверждений обеих сторон текущего цикла
			 */
			while((state.failure == nullptr) && (!state.ready || (state.peer == 0))){
				// Проверяем результат конечного опроса
				if(!io.poll(POLL_TIMEOUT))
					// Запоминаем отказ опроса
					state.failure = "poll-failed";
				// Проверяем общий срок текущего цикла
				if(std::chrono::duration <double> (std::chrono::steady_clock::now() - tick).count() >= ROUND_DEADLINE)
					// Запоминаем превышение срока
					state.failure = "round-deadline";
			}
			// Не закрываем ошибочный цикл как успешный
			if(state.failure != nullptr)
				// Прекращаем прогон
				break;
			// Запрашиваем освобождение клиента
			const bool clientClosed = io.destroy(state.client);
			// Запрашиваем освобождение принятой стороны независимо от первого исхода
			const bool peerClosed = io.destroy(state.peer);
			// Проверяем оба запроса освобождения
			if(!clientClosed || !peerClosed)
				// Запоминаем отказ закрытия
				state.failure = "destroy-failed";
			/**
			 * Дожидаемся удаления обоих узлов, не подменяя его пометкой DESTROYED
			 */
			while((state.failure == nullptr) && ((io.status(state.client) != awh::event::status_t::NONE) || (io.status(state.peer) != awh::event::status_t::NONE))){
				// Продвигаем отложенное освобождение без ожидания новых подключений
				if(!io.poll(0))
					// Запоминаем отказ опроса
					state.failure = "close-poll-failed";
				// Ограничиваем также стадию освобождения
				if(std::chrono::duration <double> (std::chrono::steady_clock::now() - tick).count() >= ROUND_DEADLINE)
					// Запоминаем превышение срока
					state.failure = "close-deadline";
			}
			// Не учитываем неполное освобождение
			if(state.failure != nullptr)
				// Прекращаем прогон
				break;
			// Учитываем освобождение обеих сторон
			state.closed += 2;
			// Снимаем идентификатор уничтоженного клиента
			state.client = 0;
			// Снимаем идентификатор уничтоженного peer
			state.peer = 0;
			// Учитываем полный цикл прогрева
			if(round < WARMUP_ROUNDS)
				// Увеличиваем счётчик прогрева
				state.warmed++;
			// Учитываем полный измеренный цикл
			else state.done++;
		}
		// Закрываем окно также при отказе
		finish = std::chrono::steady_clock::now();
		// Выключаем учёт выделений
		awh::benchmark::counting(false);
		// Выключаем учёт системных вызовов
		awh::benchmark::syscall::counting(false);
		// Проверяем полноту обеих сторон и всех освобождений
		if((state.failure == nullptr) && ((state.warmed != WARMUP_ROUNDS) || (state.done != ACCEPT_ROUNDS) || (state.connected != (WARMUP_ROUNDS + ACCEPT_ROUNDS)) || (state.accepted != state.connected) || (state.closed != (2 * state.connected)) || (finish <= start)))
			// Запоминаем неполный прогон
			state.failure = "incomplete";
		// Публикуем только полностью завершённую нагрузку
		if(state.failure == nullptr){
			// Устанавливаем точное количество измеренных циклов
			result.operations = state.done;
			// Устанавливаем длительность полного окна
			result.seconds = std::chrono::duration <double> (finish - start).count();
			// Снимаем показатели до завершающей очистки
			collect(result);
		}
		// Освобождаем также остатки ошибочного цикла
		if(!io.deinitialize()){
			// Отбрасываем результат при отказе очистки
			result = outcome_t{};
			// Запоминаем отказ очистки
			state.failure = "cleanup-failed";
		}
		// Сообщаем отказ вне измеряемого окна
		if(state.failure != nullptr)
			// Выводим причину и фактически выполненную работу
			::fprintf(stderr, "ACCEPT_BENCH_FAIL: %s warmed=%zu measured=%zu accepted=%zu closed=%zu\n", state.failure, state.warmed, state.done, state.accepted, state.closed);
		// Подтверждаем полноту успешно завершённого прогона
		else
			// Выводим контрольные счётчики обеих сторон
			::printf("ACCEPT_ACCOUNT warmed=%zu connected=%zu accepted=%zu closed=%zu measured=%zu\n", state.warmed, state.connected, state.accepted, state.closed, state.done);
		// Выводим итоги либо пустой результат при отказе
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона установления соединений
	 *
	 * @note Итоги снимаются с единственного прогона: повторный расходовал бы
	 *       динамические порты операционной системы, которых на второй прогон
	 *       подряд может не остаться
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & measured() noexcept {
		// Итоги прогона сценария установления соединений
		static const outcome_t result = ::handshake();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария скорости установления соединений
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t connections() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем итоги прогона сценария установления соединений
		const outcome_t & outcome = ::measured();
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Убеждаемся, что сценарий выполнил хоть одну операцию
		validate(result, outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария учёта выделений памяти на одно подключение
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t allocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем итоги прогона сценария установления соединений
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

	/**
	 * @brief Функция прогона сценария учёта системных вызовов на одно подключение
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
		// Получаем итоги прогона сценария установления соединений
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

	// Регистрируем сценарий скорости установления соединений
	static const bool gAccept = awh::benchmark::add(
		"net/io/accept/connections", "подключений/с", ACCEPT_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::connections
	);
	// Регистрируем сценарий учёта системных вызовов на одно подключение
	static const bool gSyscalls = awh::benchmark::add(
		"net/io/accept/syscalls-per-connection", "вызовов", SYSCALLS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::syscalls
	);
	// Регистрируем сценарий учёта выделений памяти на одно подключение
	static const bool gAllocations = awh::benchmark::add(
		"net/io/accept/allocations-per-connection", "выделений", ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::allocations
	);
};
