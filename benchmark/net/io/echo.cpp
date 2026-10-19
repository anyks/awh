/**
 * @file echo.cpp
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
 * @brief Сценарии измерения обмена короткими сообщениями — стоимость одного оборота цикла событий
 *        на одном подключении и на множестве одновременных подключений
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <chrono>
#include <cstdio>
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
		// Количество завершённых обменов прогрева этого подключения
		size_t warmed;
		// Признак отправки текущего запроса после открытия окна
		bool measured;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Connection() noexcept : id(0), received(0), warmed(0), measured(false) {}
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
		// Признак общего прогрева после готовности всех подключений
		bool fleet;
		// Количество полностью прогретых подключений
		size_t ready;
		// Количество выполненных обменов замера
		size_t done;
		// Причина недействительности прогона
		const char * failure;
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
		 stop(false), measuring(false), fleet(false), ready(0), done(0), failure(nullptr), rounds(0) {}
	} state_t;

	/**
	 * @brief Функция остановки недействительного прогона
	 *
	 * @param state  состояние прогона
	 * @param reason причина отказа
	 *
	 */
	static void fail(state_t & state, const char * reason) noexcept {
		// Не изменяем уже закрытое окно при очистке событий
		if(state.stop)
			// Прекращаем повторную остановку
			return;
		// Сохраняем первую причину отказа
		state.failure = reason;
		// Останавливаем обмены и цикл событий
		state.stop = true;
		// Выключаем учёт выделений даже при отказе внутри окна
		awh::benchmark::counting(false);
		// Выключаем учёт системных вызовов на той же границе
		awh::benchmark::syscall::counting(false);
	}
	/**
	 * @brief Функция открытия окна после готовности всех подключений
	 *
	 * @param state       состояние прогона
	 * @param connections требуемое количество подключений
	 * @param accepted    количество принятых подключений
	 * @return            признак открытия окна измерения
	 *
	 */
	static bool begin(state_t & state, const size_t connections, const size_t accepted) noexcept {
		// Не допускаем неполный состав, повторный запуск и нулевую нагрузку
		if(state.stop || state.measuring || (connections == 0) || (state.rounds == 0) || (state.ready != connections) || (accepted != connections))
			// Сохраняем окно закрытым
			return false;
		// Отмечаем начало установившегося обмена
		state.measuring = true;
		// Включаем учёт выделений памяти
		awh::benchmark::counting(true);
		// Включаем учёт системных вызовов
		awh::benchmark::syscall::counting(true);
		// Запоминаем момент начала замера
		state.start = std::chrono::steady_clock::now();
		// Разрешаем запуск измеряемых обменов
		return true;
	}
	/**
	 * @brief Функция учёта выполненного обмена конкретного подключения
	 *
	 * @param state      состояние прогона сценария
	 * @param connection состояние ответившего подключения
	 * @return           результат учёта (true - обмен следует продолжать)
	 *
	 */
	static bool account(state_t & state, connection_t & connection) noexcept {
		// Хвост текущей пачки не должен менять уже закрытое окно
		if(state.stop)
			// Прекращаем отправку следующего сообщения
			return false;
		// Если замер ещё не начат
		if(!state.measuring){
			// Готовый клиент не подменяет прогрев остальных повторным учётом
			if(connection.warmed >= 8)
				// После первого барьера сохраняем общий поток без паузы
				return state.fleet;
			// Считаем полный ответ именно этого клиента
			connection.warmed++;
			// Если клиент ещё не прогрет
			if(connection.warmed < 8)
				// Продолжаем его обмены прогрева
				return true;
			// Учитываем клиента у барьера ровно один раз
			state.ready++;
			// Общий прогрев переходит в измерение без массового перезапуска
			return state.fleet;
		}
		// Последний ответ прогрева мог прийти уже после открытия общего окна
		if(!connection.measured)
			// Продолжаем обмен, не считая запрос, начатый до измерения
			return true;
		// Считаем выполненный обмен замера
		state.done++;
		// Если требуемое количество обменов ещё не выполнено
		if(state.done < state.rounds)
			// Продолжаем обмен
			return true;
		// Запоминаем момент окончания замера ровно один раз
		state.finish = std::chrono::steady_clock::now();
		// Закрываем учёт выделений на границе последнего обмена
		awh::benchmark::counting(false);
		// Закрываем учёт системных вызовов на той же границе
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
		// Полезная нагрузка одного обмена
		static uint8_t payload[ECHO_PAYLOAD] = {0};
		// Создаём объект асинхронного движка ввода-вывода
		awh::engine::io_t io;
		// Общий обработчик ошибок не допускает частично выполненную нагрузку
		const awh::engine::callback::error_t error = [&state]([[maybe_unused]] const awh::event::id_t eid, [[maybe_unused]] const awh::event::error_t code, [[maybe_unused]] const string & message) noexcept -> void {
			// Отмечаем отказ движка независимо от режима проверки порогов
			::fail(state, "engine-error");
		};
		// Общий обработчик разрыва останавливает незавершённые обмены
		const awh::engine::callback::event_t closed = [&state]([[maybe_unused]] const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			// Если связь закрыта до завершения всей нагрузки
			if((action == awh::event::action_t::CLOSE) || (action == awh::event::action_t::DISCONNECT))
				// Не выдаём оставшихся участников за полный состав
				::fail(state, "connection-closed");
		};
		// Получаем свободный порт петлевого интерфейса
		const uint16_t number = port();
		// Добавляем новое событие сервера
		const awh::event::id_t server = io.event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
		// Устанавливаем порт события сервера
		io.setSourcePort(server, number);
		// Добавляем общий предельный срок подготовки и измерения
		const awh::event::id_t deadline = io.event(awh::event::node_t::TIMEOUT, awh::event::family_t::TIMER);
		// Если инициализация движка не выполнена
		if(!io.initialize())
			// Выводим пустые итоги прогона
			return result;
		// Регистрируем отказ слушающего события
		io.on(server, error);
		// Устанавливаем опции события сервера
		io.setOptions(server, options());
		// Устанавливаем адрес события сервера
		io.setAddress(server, awh::event::address_t::IPV4, "127.0.0.1");
		// Список состояний принятых сервером подключений
		vector <unique_ptr <connection_t>> accepted;
		// Резервируем память под состояния принятых подключений
		accepted.reserve(connections);
		// Устанавливаем функцию обратного вызова на принятие входящего подключения
		io.on(server, static_cast <awh::engine::callback::accept_t> ([&io, &state, &accepted, &error, &closed]([[maybe_unused]] const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
			// Отслеживаем ошибки принятого подключения
			io.on(cid, error);
			// Отслеживаем преждевременное закрытие принятого подключения
			io.on(cid, closed);
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
			io.on(cid, [&io, &state](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Не отправляем хвост сообщений после закрытия окна
				if(!state.stop && (io.send(eid, data, size) != size))
					// Отмечаем неполную постановку ответа в очередь
					::fail(state, "echo-send-failed");
			});
		}));
		// Проверяем настройку и запуск слушающего события
		if(!io.commit(server) || !io.listen(server, BACKLOG) || !io.launch(server))
			// Не запускаем клиентов без рабочего сервера
			::fail(state, "listener-setup-failed");
		// Ограничиваем весь прогон одной минутой без часов в горячем пути
		io.setTimeout(deadline, awh::event::action_t::NONE, 60000);
		// Регистрируем завершение предельного срока
		io.on(deadline, [&state]([[maybe_unused]] const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			// Если предельный срок достигнут
			if(status == awh::event::status_t::SUCCESS)
				// Отклоняем неполный прогон вместо бесконечного ожидания
				::fail(state, "deadline-expired");
		});
		// Проверяем запуск общего предельного срока
		if(!io.commit(deadline) || !io.launch(deadline))
			// Не допускаем неограниченное ожидание при отказе таймера
			::fail(state, "deadline-setup-failed");
		// Список состояний клиентских подключений
		vector <unique_ptr <connection_t>> clients;
		// Резервируем память под состояния клиентских подключений
		clients.reserve(connections);
		/**
		 * Выполняем создание требуемого количества клиентских подключений
		 */
		for(size_t i = 0; (i < connections) && !state.stop; i++){
			// Создаём состояние клиентского подключения
			clients.push_back(unique_ptr <connection_t> (new connection_t));
			// Получаем состояние созданного подключения
			connection_t * connection = clients.back().get();
			// Добавляем новое событие клиента
			connection->id = io.event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Регистрируем отказ клиентского события
			io.on(connection->id, error);
			// Регистрируем преждевременное закрытие клиента
			io.on(connection->id, closed);
			// Устанавливаем порт назначения события клиента
			io.setTargetPort(connection->id, number);
			// Устанавливаем опции события клиента
			io.setOptions(connection->id, options());
			// Устанавливаем адрес привязки события клиента
			io.setAddress(connection->id, awh::event::address_t::IPV4, "0.0.0.0");
			// Устанавливаем адрес назначения события клиента
			io.setTarget(connection->id, "127.0.0.1");
			// Устанавливаем функцию обратного вызова на подключение клиента
			io.on(connection->id, static_cast <awh::engine::callback::connect_t> ([&io, &state](const awh::event::id_t eid, const bool ok) noexcept -> void {
				// Не продолжаем подготовку после первого отказа
				if(state.stop)
					// Оставляем очистку общему завершению
					return;
				// Проверяем подключение и первое сообщение прогрева
				if(!ok || (io.send(eid, payload, ECHO_PAYLOAD) != ECHO_PAYLOAD))
					// Неподключённый клиент не должен исчезнуть из нагрузки
					::fail(state, "client-connect-or-send-failed");
			}));
			/**
			 * Устанавливаем функцию обратного вызова на чтение из события клиента:
			 * границы сообщения восстанавливаются накоплением принятых октетов,
			 * поскольку поток TCP вправе доставить ответ по частям
			 */
			io.on(connection->id, [&io, &state, &accepted, connection, connections](const awh::event::id_t eid, const uint8_t *, const size_t size) noexcept -> void {
				// Хвост пачки после остановки не принадлежит измерению
				if(state.stop)
					// Не изменяем счётчики и не создаём новых отправок
					return;
				// Накапливаем количество принятых октетов текущего сообщения
				connection->received += size;
				// Если сообщение принято не полностью
				if(connection->received < ECHO_PAYLOAD)
					// Ожидаем оставшуюся часть сообщения
					return;
				// Учитываем принятое сообщение
				connection->received -= ECHO_PAYLOAD;
				// Учитываем завершённый запрос до выбора фазы следующей отправки
				const bool more = ::account(state, * connection);
				// Последний готовый участник открывает окно внутри непрерывного потока
				if(state.fleet && !state.measuring && (state.ready == connections)){
					// Повторно проверяем полный состав на самой границе измерения
					if(!::begin(state, connections, accepted.size()))
						// Неполный состав не должен попасть в результат
						::fail(state, "warmup-incomplete");
				}
				// Если следующий обмен разрешён и прогон не остановлен
				if(more && !state.stop){
					// Помечаем именно новый запрос, а не пришедший ответ прогрева
					connection->measured = state.measuring;
					// Проверяем полную постановку следующего сообщения
					if(io.send(eid, payload, ECHO_PAYLOAD) != ECHO_PAYLOAD)
						// Отклоняем потерянное продолжение обмена
						::fail(state, "client-send-failed");
				}
			});
			// Проверяем настройку, подключение и запуск клиента
			if(!io.commit(connection->id) || !io.connect(connection->id) || !io.launch(connection->id))
				// Не подменяем неполный состав меньшей нагрузкой
				::fail(state, "client-setup-failed");
		}
		/**
		 * Каждый клиент выполняет восемь обменов и ждёт у общего барьера
		 */
		while(!state.stop && (state.ready < connections)){
			// Проверяем успешность опроса, включая пробуждение предельным сроком
			if(!io.poll())
				// Отказ опроса не должен превращаться в частичный результат
				::fail(state, "warmup-poll-failed");
		}
		// Проверяем полный состав перед общим прогревом очередей отправки
		if(!state.stop && (connections > 0) && (rounds > 0) && (state.ready == connections) && (accepted.size() == connections)){
			// Сохраняем поток готовых клиентов до включения измерения
			state.fleet = true;
			// Отдельно считаем участников общего прогрева
			state.ready = 0;
			/**
			 * Сбрасываем индивидуальные счётчики до первой общей отправки
			 */
			for(const auto & connection : clients)
				// Каждый клиент должен снова завершить свои восемь обменов
				connection->warmed = 0;
			/**
			 * Прогреваем максимальное число одновременно незавершённых отправок
			 */
			for(const auto & connection : clients){
				// Не запускаем оставшихся участников после отказа
				if(state.stop)
					// Оставляем очистку общему завершению
					break;
				// Первый общий запуск ещё не принадлежит измерению
				if(io.send(connection->id, payload, ECHO_PAYLOAD) != ECHO_PAYLOAD)
					// Не открываем окно при неполной постановке прогрева
					::fail(state, "fleet-warmup-send-failed");
			}
			/**
			 * Общий прогрев продолжается измерением без остановки всех клиентов
			 */
			while(!state.stop){
				// Проверяем опрос обеих фаз до целевого обмена или первого отказа
				if(!io.poll())
					// Не выдаём неполный прогон за полный
					::fail(state, "fleet-poll-failed");
			}
		} else
			// Нулевой или неполный состав не имеет измеряемого результата
			::fail(state, "warmup-incomplete");
		// При отказе оставляем нулевые операции для обязательного validate
		if((state.failure == nullptr) && (state.done == rounds) && (rounds > 0)){
			// Устанавливаем точное количество выполненных обменов
			result.operations = state.done;
			// Устанавливаем объём данных в обоих направлениях
			result.bytes = (state.done * ECHO_PAYLOAD * 2);
			// Устанавливаем длительность единственного закрытого окна
			result.seconds = std::chrono::duration <double> (state.finish - state.start).count();
		}
		// Снимаем показатели до очистки и вывода диагностики
		collect(result);
		// Если нагрузка не была выполнена полностью
		if(state.failure != nullptr)
			// Сообщаем причину отказа вне измеряемого окна
			::fprintf(stderr, "ECHO_BENCH_FAIL: %s\n", state.failure);
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
	/**
	 * @note Имя сценария несёт приставку неспроста: `single` у части систем занято
	 *       СИСТЕМНЫМ ИМЕНЕМ ТИПА (`typedef float single` в `floatingpoint.h` у Solaris
	 *       и illumos), и взятие адреса такого имени разбирается как взятие адреса типа -
	 *       сборка отвечает «expected primary-expression before ')' token», указывая на
	 *       строку с закрывающей скобкой, а не на имя
	 *
	 */
	static awh::benchmark::result_t singleFlow() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем итоги прогона сценария обмена
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
	 * @brief Функция прогона сценария обмена на множестве подключений
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t multiFlow() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон сценария обмена
		const outcome_t outcome = ::exchange(ECHO_MULTI_CONNECTIONS, ECHO_MULTI_ROUNDS);
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
		// Убеждаемся, что сценарий выполнил хоть одну операцию
		validate(result, outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	// Регистрируем сценарий обмена на одном подключении
	static const bool gSingle = awh::benchmark::add(
		"net/io/echo/single", "обменов/с", SINGLE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::singleFlow
	);
	// Регистрируем сценарий обмена на множестве подключений
	static const bool gMulti = awh::benchmark::add(
		"net/io/echo/multi", "обменов/с", MULTI_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::multiFlow
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
		// Убеждаемся, что сценарий выполнил хоть одну операцию
		validate(result, outcome);
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
