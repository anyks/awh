/**
 * @file: accept.cpp
 * @date: 2026-07-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Сценарий измерения установления соединений — полный цикл создания события,
 *        подключения, принятия его сервером и освобождения обеих сторон
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
	 * @details Полный цикл короткоживущего соединения выполняет три десятка
	 *          выделений: состояние события, его подписки и записи учёта.
	 *          Порог задан вплотную к измеренному значению - его рост означает
	 *          появление ещё одной структуры на подключение
	 *
	 */
	static constexpr double ALLOCATIONS_THRESHOLD = 40.0;
	/**
	 * @brief Порог количества системных вызовов на одно подключение
	 *
	 * @details Порог задан вплотную к измеренному значению, потому что счётчик
	 *          вызовов детерминирован: на той же нагрузке он совпадает от прогона
	 *          к прогону вплоть до единиц, и запас ему нужен только на разницу
	 *          между прогревом и замером. Измеренная величина втрое превышает
	 *          необходимое: полный цикл короткоживущего соединения требует одного
	 *          создания сокета, одного подключения, одного принятия и двух
	 *          закрытий, всё остальное - опрос конфигурации сети на каждое
	 *          создание события. Целевое значение после его устранения - не более
	 *          десяти вызовов на подключение
	 *
	 */
	static constexpr double SYSCALLS_THRESHOLD = 34.0;

	/**
	 * @brief Структура состояния прогона сценария
	 *
	 */
	typedef struct State {
		// Флаг остановки цикла событий
		bool stop;
		// Флаг активности замера
		bool measuring;
		// Флаг завершения текущего цикла подключения
		bool ready;
		// Количество выполненных циклов прогрева
		size_t warmed;
		// Количество выполненных циклов замера
		size_t done;
		// Идентификатор события текущего подключения
		awh::event::id_t client;
		// Список принятых сервером подключений, подлежащих освобождению
		vector <awh::event::id_t> accepted;
		// Момент начала замера
		std::chrono::steady_clock::time_point start;
		// Момент окончания замера
		std::chrono::steady_clock::time_point finish;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit State() noexcept :
		 stop(false), measuring(false), ready(false), warmed(0), done(0), client(0) {}
	} state_t;

	/**
	 * @brief Функция прогона сценария установления соединений
	 *
	 * @details Циклы выполняются последовательно: следующее подключение
	 *          создаётся только после освобождения предыдущего с обеих сторон.
	 *          Замер включает создание события, подключение, принятие его
	 *          сервером и освобождение обоих событий - то есть полную стоимость
	 *          обслуживания короткоживущего соединения. Освобождение выполняется
	 *          вне функций обратного вызова: движок вправе обращаться к
	 *          состоянию события после их возврата
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t handshake() noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Состояние прогона сценария
		state_t state;
		// Резервируем память под список принятых подключений
		state.accepted.reserve(16);
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
		io.on(server, static_cast <awh::engine::callback::accept_t> ([&state](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
			// Запоминаем принятое подключение для освобождения
			state.accepted.push_back(cid);
		}));
		// Выполняем фиксацию настроек события сервера
		io.commit(server);
		// Переводим событие сервера в режим прослушивания
		io.listen(server, BACKLOG);
		// Запускаем событие сервера
		io.launch(server);
		/**
		 * @brief Функция запуска очередного цикла подключения
		 *
		 */
		auto launch = [&io, &state, number]() noexcept -> void {
			// Добавляем новое событие клиента
			state.client = io.event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Устанавливаем порт назначения события клиента
			io.setTargetPort(state.client, number);
			// Устанавливаем опции события клиента
			io.setOptions(state.client, options());
			// Устанавливаем адрес привязки события клиента
			io.setAddress(state.client, awh::event::address_t::IPV4, "0.0.0.0");
			// Устанавливаем адрес назначения события клиента
			io.setTarget(state.client, "127.0.0.1");
			// Устанавливаем функцию обратного вызова на подключение клиента
			io.on(state.client, static_cast <awh::engine::callback::connect_t> ([&state](const awh::event::id_t eid, const bool ok) noexcept -> void {
				// Отмечаем завершение текущего цикла подключения
				state.ready = true;
			}));
			// Выполняем фиксацию настроек события клиента
			io.commit(state.client);
			// Выполняем подключение клиента к серверу
			io.connect(state.client);
			// Запускаем событие клиента
			io.launch(state.client);
		};
		// Запускаем первый цикл подключения
		launch();
		/**
		 * Выполняем опрос событий до выполнения требуемого количества циклов
		 */
		while(!state.stop){
			// Если опрос событий завершился ошибкой
			if(!io.poll())
				// Прекращаем прогон сценария
				break;
			// Если текущий цикл подключения ещё не завершён
			if(!state.ready)
				// Ожидаем завершения текущего цикла подключения
				continue;
			// Сбрасываем флаг завершения текущего цикла подключения
			state.ready = false;
			// Освобождаем событие клиента
			io.destroy(state.client);
			/**
			 * Освобождаем принятые сервером подключения
			 */
			for(auto & id : state.accepted)
				// Освобождаем принятое подключение
				io.destroy(id);
			// Очищаем список принятых подключений
			state.accepted.clear();
			// Если замер ещё не начат
			if(!state.measuring){
				// Считаем выполненный цикл прогрева
				state.warmed++;
				// Если прогрев завершён
				if(state.warmed >= WARMUP_ROUNDS){
					// Включаем режим замера
					state.measuring = true;
					// Включаем учёт выделений памяти
					awh::benchmark::counting(true);
					// Включаем учёт системных вызовов
					awh::benchmark::syscall::counting(true);
					// Запоминаем момент начала замера
					state.start = std::chrono::steady_clock::now();
				}
			// Если замер выполняется
			} else {
				// Считаем выполненный цикл замера
				state.done++;
				// Если требуемое количество циклов выполнено
				if(state.done >= ACCEPT_ROUNDS){
					// Запоминаем момент окончания замера
					state.finish = std::chrono::steady_clock::now();
					// Закрываем окно замера: счётчики обязаны остановиться там же, где часы
					awh::benchmark::counting(false);
					awh::benchmark::syscall::counting(false);
					// Останавливаем цикл событий
					state.stop = true;
					// Прекращаем прогон сценария
					break;
				}
			}
			// Запускаем очередной цикл подключения
			launch();
		}
		// Устанавливаем количество выполненных операций
		result.operations = state.done;
		// Устанавливаем затраченное время
		result.seconds = std::chrono::duration <double> (state.finish - state.start).count();
		// Снимаем показатели окружения по итогам замера
		collect(result);
		// Выполняем деинициализацию движка
		io.deinitialize();
		// Выводим итоги прогона сценария
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
