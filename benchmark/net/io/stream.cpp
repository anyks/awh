/**
 * @file: stream.cpp
 * @date: 2026-07-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Сценарий измерения пропускной способности потоковой передачи —
 *        передача крупного объёма данных блоками по одному подключению петлевого интерфейса
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <chrono>
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
	 * @brief Структура состояния прогона сценария
	 *
	 */
	typedef struct State {
		// Флаг остановки цикла событий
		bool stop;
		// Количество октетов текущего блока, ожидающих записи в сокет
		size_t pending;
		// Количество поставленных в очередь октетов
		size_t queued;
		// Количество принятых сервером октетов
		size_t received;
		// Момент начала замера
		std::chrono::steady_clock::time_point start;
		// Момент окончания замера
		std::chrono::steady_clock::time_point finish;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit State() noexcept : stop(false), pending(0), queued(0), received(0) {}
	} state_t;

	/**
	 * @brief Функция прогона сценария потоковой передачи
	 *
	 * @details Передача идёт блоками фиксированного размера, и в полёте
	 *          находится ровно один блок: следующий ставится в очередь после
	 *          того, как движок сообщил о записи предыдущего. Такое окно
	 *          воспроизводится всеми эталонными реализациями конкурентов -
	 *          иначе сравнивалась бы не пропускная способность, а глубина
	 *          буферизации, у каждой библиотеки своя
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t transfer() noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Состояние прогона сценария
		state_t state;
		// Блок передаваемых данных
		static vector <uint8_t> chunk(STREAM_CHUNK, 0x5A);
		// Создаём объект асинхронного движка ввода-вывода
		awh::engine::io_t io(framework(), logger());
		// Получаем свободный порт петлевого интерфейса
		const uint16_t number = port();
		// Добавляем новое событие сервера
		const awh::event::id_t server = io.event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
		// Устанавливаем порт события сервера
		io.setSourcePort(server, number);
		// Если инициализация движка не выполнена
		if(!io.initialize())
			// Выводим пустые итоги прогона
			return result;
		// Устанавливаем опции события сервера
		io.setOptions(server, options());
		// Устанавливаем адрес события сервера
		io.setAddress(server, awh::event::address_t::IPV4, "127.0.0.1");
		// Устанавливаем функцию обратного вызова на принятие входящего подключения
		io.on(server, static_cast <awh::engine::callback::accept_t> ([&io, &state](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
			// Устанавливаем опции принятого подключения
			io.setOptions(cid, options());
			/**
			 * Устанавливаем функцию обратного вызова на чтение из принятого подключения:
			 * приёмник только считает принятые октеты, разбор данных в сценарии
			 * пропускной способности измерял бы потребителя, а не движок
			 */
			io.on(cid, [&state](const awh::event::id_t eid, const uint8_t *, const size_t size) noexcept -> void {
				// Накапливаем количество принятых октетов
				state.received += size;
				// Если весь объём передачи принят
				if(state.received >= STREAM_VOLUME){
					// Запоминаем момент окончания замера
					state.finish = std::chrono::steady_clock::now();
					// Отключаем учёт выделений памяти
					awh::benchmark::counting(false);
					// Останавливаем цикл событий
					state.stop = true;
				}
			});
		}));
		// Выполняем фиксацию настроек события сервера
		io.commit(server);
		// Переводим событие сервера в режим прослушивания
		io.listen(server, BACKLOG);
		// Запускаем событие сервера
		io.launch(server);
		// Добавляем новое событие клиента
		const awh::event::id_t client = io.event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
		// Устанавливаем порт назначения события клиента
		io.setTargetPort(client, number);
		// Устанавливаем опции события клиента
		io.setOptions(client, options());
		// Устанавливаем адрес привязки события клиента
		io.setAddress(client, awh::event::address_t::IPV4, "0.0.0.0");
		// Устанавливаем адрес назначения события клиента
		io.setTarget(client, "127.0.0.1");
		// Устанавливаем функцию обратного вызова на подключение клиента
		io.on(client, static_cast <awh::engine::callback::connect_t> ([&io, &state](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Если подключение выполнено успешно
			if(ok){
				// Включаем учёт выделений памяти
				awh::benchmark::counting(true);
				// Запоминаем момент начала замера
				state.start = std::chrono::steady_clock::now();
				// Устанавливаем количество октетов блока, ожидающих записи
				state.pending = STREAM_CHUNK;
				// Считаем поставленный в очередь объём
				state.queued += STREAM_CHUNK;
				// Ставим в очередь первый блок передачи
				io.send(eid, chunk.data(), STREAM_CHUNK);
			}
		}));
		/**
		 * Устанавливаем функцию обратного вызова на запись в событие клиента:
		 * блок вправе уйти в сокет по частям, поэтому следующий ставится в
		 * очередь только после записи предыдущего целиком
		 */
		io.on(client, static_cast <awh::engine::callback::write_t> ([&io, &state](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Если записанный объём превышает остаток блока
			if(size >= state.pending)
				// Обнуляем остаток блока
				state.pending = 0;
			// Уменьшаем остаток блока на записанный объём
			else state.pending -= size;
			// Если блок записан не полностью
			if(state.pending > 0)
				// Ожидаем записи оставшейся части блока
				return;
			// Если весь объём передачи поставлен в очередь
			if(state.queued >= STREAM_VOLUME)
				// Завершаем передачу
				return;
			// Устанавливаем количество октетов следующего блока
			state.pending = STREAM_CHUNK;
			// Считаем поставленный в очередь объём
			state.queued += STREAM_CHUNK;
			// Ставим в очередь следующий блок передачи
			io.send(eid, chunk.data(), STREAM_CHUNK);
		}));
		// Выполняем фиксацию настроек события клиента
		io.commit(client);
		// Выполняем подключение клиента к серверу
		io.connect(client);
		// Запускаем событие клиента
		io.launch(client);
		/**
		 * Запускаем опрос событий до передачи всего объёма
		 */
		while(!state.stop && io.poll());
		// Устанавливаем количество выполненных операций
		result.operations = (state.received / STREAM_CHUNK);
		// Устанавливаем объём переданных данных
		result.bytes = state.received;
		// Устанавливаем затраченное время
		result.seconds = std::chrono::duration <double> (state.finish - state.start).count();
		// Получаем статистику выделений памяти
		awh::benchmark::allocations(result.allocations, result.allocated);
		// Получаем пиковый объём занятой процессом памяти
		result.footprint = footprint();
		// Выполняем деинициализацию движка
		io.deinitialize();
		// Выводим итоги прогона сценария
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
	// Регистрируем сценарий учёта выделений памяти на один блок
	static const bool gAllocations = awh::benchmark::add(
		"net/io/stream/allocations-per-chunk", "выделений", ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::allocations
	);
};
