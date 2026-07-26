/**
 * @file: echo.cpp
 * @date: 2026-07-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Сценарии измерения обмена короткими сообщениями — стоимость одного оборота цикла событий
 *        на одном подключении и на множестве одновременных подключений
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <chrono>
#include <memory>
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
 * @brief Внутренние параметры и сценарии бенчмарков обмена короткими сообщениями
 *
 */
namespace {
	/**
	 * @brief Порог скорости обмена на одном подключении в обменах в секунду
	 *
	 * @details Пороги пропускной способности зависят от машины и режима сборки,
	 *          поэтому откалиброваны по отладочной сборке репозитория с
	 *          двукратным запасом: они ловят регрессию цикла событий в разы, а
	 *          не колебания планировщика операционной системы. Показатели
	 *          сценариев обмена упираются в переключение контекста на петлевом
	 *          интерфейсе, поэтому между отладочной и оптимизированной сборкой
	 *          различаются мало
	 *
	 */
	static constexpr double SINGLE_THRESHOLD = 25000.0;
	/**
	 * @brief Порог скорости обмена на множестве подключений в обменах в секунду
	 *
	 */
	static constexpr double MULTI_THRESHOLD = 80000.0;
	/**
	 * @brief Порог количества выделений памяти на один обмен
	 *
	 * @details В отличие от пропускной способности показатель от машины и режима
	 *          сборки не зависит, поэтому порог задан вплотную к измеренному
	 *          значению: установившийся обмен выделений памяти не выполняет
	 *          вовсе, и любое ненулевое значение означает появление выделения
	 *          в горячем пути доставки готовности
	 *
	 */
	static constexpr double ALLOCATIONS_THRESHOLD = 0.5;
	/**
	 * @brief Порог количества системных вызовов на один обмен
	 *
	 * @details Обмен требует четырёх обращений к ядру - по приёму и передаче на
	 *          каждой стороне - плюс возврат за готовностью, итого шесть. Ровно
	 *          столько и измеряется: движок узнаёт об исчерпании буфера из объёма,
	 *          объявленного готовым событием, а не повторным приёмом, возвращающим
	 *          отказ. Прежде таких приёмов было по одному на сторону, и величина
	 *          составляла восемь.
	 *
	 *          Порог задан вплотную к измеренному: шесть вызовов есть необходимый
	 *          минимум для обмена, снизить его нечем, а появление седьмого означало
	 *          бы возврат лишнего обращения в горячий путь
	 *
	 */
	static constexpr double SYSCALLS_THRESHOLD = 6.5;

	/**
	 * @brief Структура состояния одного подключения сценария
	 *
	 * @note Состояние захватывается функцией обратного вызова конкретного
	 *       подключения по указателю, поэтому поиск состояния по идентификатору
	 *       события в горячем пути не выполняется: он измерялся бы вместе с
	 *       движком, а эталонные реализации конкурентов держат состояние
	 *       в поле наблюдателя и такого поиска не делают
	 *
	 */
	typedef struct Connection {
		// Идентификатор события подключения
		awh::event::id_t id;
		// Количество принятых октетов текущего сообщения
		size_t received;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Connection() noexcept : id(0), received(0) {}
	} connection_t;

	/**
	 * @brief Структура состояния прогона сценария
	 *
	 */
	typedef struct State {
		// Флаг остановки цикла событий
		bool stop;
		// Флаг активности замера
		bool measuring;
		// Количество выполненных обменов прогрева
		size_t warmed;
		// Количество выполненных обменов замера
		size_t done;
		// Требуемое количество обменов прогрева
		size_t warmup;
		// Требуемое количество обменов замера
		size_t rounds;
		// Момент начала замера
		std::chrono::steady_clock::time_point start;
		// Момент окончания замера
		std::chrono::steady_clock::time_point finish;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit State() noexcept :
		 stop(false), measuring(false), warmed(0), done(0), warmup(0), rounds(0) {}
	} state_t;

	/**
	 * @brief Функция учёта выполненного обмена
	 *
	 * @param state состояние прогона сценария
	 * @return      результат учёта (true - обмен следует продолжать)
	 *
	 */
	static bool account(state_t & state) noexcept {
		// Если замер ещё не начат
		if(!state.measuring){
			// Считаем выполненный обмен прогрева
			state.warmed++;
			// Если прогрев ещё не завершён
			if(state.warmed < state.warmup)
				// Продолжаем обмен
				return true;
			// Включаем режим замера
			state.measuring = true;
			// Включаем учёт выделений памяти
			awh::benchmark::counting(true);
			// Включаем учёт системных вызовов
			awh::benchmark::syscall::counting(true);
			// Запоминаем момент начала замера
			state.start = std::chrono::steady_clock::now();
			// Продолжаем обмен
			return true;
		}
		// Считаем выполненный обмен замера
		state.done++;
		// Если требуемое количество обменов ещё не выполнено
		if(state.done < state.rounds)
			// Продолжаем обмен
			return true;
		// Запоминаем момент окончания замера
		state.finish = std::chrono::steady_clock::now();
		// Закрываем окно замера: счётчики обязаны остановиться там же, где часы
		awh::benchmark::counting(false);
		awh::benchmark::syscall::counting(false);
		// Останавливаем цикл событий
		state.stop = true;
		// Прекращаем обмен
		return false;
	}
	/**
	 * @brief Функция прогона сценария обмена короткими сообщениями
	 *
	 * @details Обе стороны обмена обслуживаются одним циклом событий в одном
	 *          потоке: клиент отправляет сообщение, сервер возвращает его
	 *          обратно, и приём ответа клиентом считается одним обменом.
	 *          Каждое подключение держит ровно одно сообщение в полёте,
	 *          поэтому показатель отражает стоимость оборота цикла, а не
	 *          глубину конвейеризации
	 *
	 * @param connections количество одновременных подключений
	 * @param rounds      требуемое количество обменов замера
	 * @return            итоги прогона сценария
	 *
	 */
	static outcome_t exchange(const size_t connections, const size_t rounds) noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Состояние прогона сценария
		state_t state;
		// Устанавливаем требуемое количество обменов замера
		state.rounds = rounds;
		/**
		 * Устанавливаем требуемое количество обменов прогрева: буферы сокетов и
		 * накопители движка выходят на рабочий объём за первые обмены, и их
		 * стоимость к установившемуся режиму отношения не имеет
		 */
		state.warmup = (connections * 8);
		// Полезная нагрузка одного обмена
		static uint8_t payload[ECHO_PAYLOAD] = {0};
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
		// Список состояний принятых сервером подключений
		vector <unique_ptr <connection_t>> accepted;
		// Резервируем память под состояния принятых подключений
		accepted.reserve(connections);
		// Устанавливаем функцию обратного вызова на принятие входящего подключения
		io.on(server, static_cast <awh::engine::callback::accept_t> ([&io, &accepted](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
			// Устанавливаем опции принятого подключения
			io.setOptions(cid, options());
			// Создаём состояние принятого подключения
			accepted.push_back(unique_ptr <connection_t> (new connection_t));
			// Устанавливаем идентификатор события принятого подключения
			accepted.back()->id = cid;
			/**
			 * Устанавливаем функцию обратного вызова на чтение из принятого подключения:
			 * сервер возвращает принятые октеты как есть, разбор сообщения на его
			 * стороне не нужен - границы восстанавливает принимающий клиент
			 */
			io.on(cid, [&io](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Отправляем принятые данные обратно клиенту
				io.send(eid, data, size);
			});
		}));
		// Выполняем фиксацию настроек события сервера
		io.commit(server);
		// Переводим событие сервера в режим прослушивания
		io.listen(server, BACKLOG);
		// Запускаем событие сервера
		io.launch(server);
		// Список состояний клиентских подключений
		vector <unique_ptr <connection_t>> clients;
		// Резервируем память под состояния клиентских подключений
		clients.reserve(connections);
		/**
		 * Выполняем создание требуемого количества клиентских подключений
		 */
		for(size_t i = 0; i < connections; i++){
			// Создаём состояние клиентского подключения
			clients.push_back(unique_ptr <connection_t> (new connection_t));
			// Получаем состояние созданного подключения
			connection_t * connection = clients.back().get();
			// Добавляем новое событие клиента
			connection->id = io.event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Устанавливаем порт назначения события клиента
			io.setTargetPort(connection->id, number);
			// Устанавливаем опции события клиента
			io.setOptions(connection->id, options());
			// Устанавливаем адрес привязки события клиента
			io.setAddress(connection->id, awh::event::address_t::IPV4, "0.0.0.0");
			// Устанавливаем адрес назначения события клиента
			io.setTarget(connection->id, "127.0.0.1");
			// Устанавливаем функцию обратного вызова на подключение клиента
			io.on(connection->id, static_cast <awh::engine::callback::connect_t> ([&io](const awh::event::id_t eid, const bool ok) noexcept -> void {
				// Если подключение выполнено успешно
				if(ok)
					// Отправляем первое сообщение обмена
					io.send(eid, payload, ECHO_PAYLOAD);
			}));
			/**
			 * Устанавливаем функцию обратного вызова на чтение из события клиента:
			 * границы сообщения восстанавливаются накоплением принятых октетов,
			 * поскольку поток TCP вправе доставить ответ по частям
			 */
			io.on(connection->id, [&io, &state, connection](const awh::event::id_t eid, const uint8_t *, const size_t size) noexcept -> void {
				// Накапливаем количество принятых октетов текущего сообщения
				connection->received += size;
				// Если сообщение принято не полностью
				if(connection->received < ECHO_PAYLOAD)
					// Ожидаем оставшуюся часть сообщения
					return;
				// Учитываем принятое сообщение
				connection->received -= ECHO_PAYLOAD;
				// Если обмен следует продолжать
				if(::account(state))
					// Отправляем следующее сообщение обмена
					io.send(eid, payload, ECHO_PAYLOAD);
			});
			// Выполняем фиксацию настроек события клиента
			io.commit(connection->id);
			// Выполняем подключение клиента к серверу
			io.connect(connection->id);
			// Запускаем событие клиента
			io.launch(connection->id);
		}
		/**
		 * Запускаем опрос событий до выполнения требуемого количества обменов
		 */
		while(!state.stop && io.poll());
		// Устанавливаем количество выполненных операций
		result.operations = state.done;
		// Устанавливаем объём переданных данных с учётом обоих направлений обмена
		result.bytes = (state.done * ECHO_PAYLOAD * 2);
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
	 * @brief Функция получения итогов прогона обмена на одном подключении
	 *
	 * @note Итоги снимаются с единственного прогона: показатели скорости и
	 *       выделений памяти описывают один и тот же обмен, а повторный прогон
	 *       расходовал бы динамические порты операционной системы впустую
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & measured() noexcept {
		// Итоги прогона сценария обмена на одном подключении
		static const outcome_t result = ::exchange(1, ECHO_SINGLE_ROUNDS);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария обмена на одном подключении
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t single() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем итоги прогона сценария обмена
		const outcome_t & outcome = ::measured();
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария обмена на множестве подключений
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t multi() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон сценария обмена
		const outcome_t outcome = ::exchange(ECHO_MULTI_CONNECTIONS, ECHO_MULTI_ROUNDS);
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария учёта выделений памяти на один обмен
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t allocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем итоги прогона сценария обмена
		const outcome_t & outcome = ::measured();
		// Устанавливаем измеренное значение
		result.value = perOperation(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	// Регистрируем сценарий обмена на одном подключении
	static const bool gSingle = awh::benchmark::add(
		"net/io/echo/single", "обменов/с", SINGLE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::single
	);
	// Регистрируем сценарий обмена на множестве подключений
	static const bool gMulti = awh::benchmark::add(
		"net/io/echo/multi", "обменов/с", MULTI_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::multi
	);
	/**
	 * @brief Функция прогона сценария учёта системных вызовов на один обмен
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
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	// Регистрируем сценарий учёта выделений памяти на один обмен
	static const bool gAllocations = awh::benchmark::add(
		"net/io/echo/allocations-per-round", "выделений", ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::allocations
	);
	// Регистрируем сценарий учёта системных вызовов на один обмен
	static const bool gSyscalls = awh::benchmark::add(
		"net/io/echo/syscalls-per-round", "вызовов", SYSCALLS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::syscalls
	);
};
