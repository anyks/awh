/**
 * @file libuv.cpp
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
 * @brief Эталонный стенд сравнения на libuv — те же сценарии нагрузки, что и у бенчмарков
 *        сетевого движка AWH, выполненные средствами интерфейса завершения операций библиотеки
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <memory>

/**
 * Подключаем заголовочные файлы библиотеки libuv
 */
#include <uv.h>

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён эталонных стендов
 */
using namespace rival;

/**
 * @brief Внутреннее окружение стенда
 *
 * @details Библиотека не отдаёт готовность наружу: она сама читает в
 *          предоставленный буфер и сама держит очередь отправки, поэтому
 *          сравнима с движком AWH напрямую, а libevent и libev в
 *          низкоуровневом режиме дают отсчёт «голой» доставки готовности.
 *          Сокеты создаёт сама библиотека, поэтому опции задаются её
 *          средствами - отключение алгоритма Нейгла вызовом `uv_tcp_nodelay`
 *
 */
namespace {
	// Полезная нагрузка одного обмена
	static uint8_t gPayload[ECHO_PAYLOAD] = {0};
	// Блок передаваемых данных сценария пропускной способности
	static vector <uint8_t> gChunk(STREAM_CHUNK, 0x5A);
	// Буфер приёма данных
	static vector <uint8_t> gBuffer(STREAM_CHUNK, 0);
	/**
	 * @brief Количество принятых подключений сценария наблюдения
	 *
	 * @note Счётчик вынесен в область видимости стенда, потому что дескриптор
	 *       слушающего сокета библиотеки несёт единственное поле пользовательских
	 *       данных, и оно занято под иные нужды в прочих сценариях
	 *
	 */
	static size_t gAccepted = 0;

	/**
	 * @brief Структура состояния одного подключения сценария обмена
	 *
	 */
	typedef struct Connection {
		// Дескриптор подключения библиотеки
		uv_tcp_t handle;
		// Запрос отправки данных
		uv_write_t request;
		// Запрос подключения к серверу
		uv_connect_t connection;
		// Количество принятых октетов текущего сообщения
		size_t received;
		// Состояние прогона сценария
		echo_t * state;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Connection() noexcept :
		 handle{}, request{}, connection{}, received(0), state(nullptr) {}
	} connection_t;

	/**
	 * @brief Структура состояния прогона сценария пропускной способности
	 *
	 */
	typedef struct Stream {
		// Дескриптор передатчика
		uv_tcp_t * sender;
		// Запрос отправки блока
		uv_write_t request;
		// Количество поставленных в очередь октетов
		size_t queued;
		// Количество принятых приёмником октетов
		size_t received;
		// Цикл событий стенда
		uv_loop_t * loop;
		// Момент начала замера
		std::chrono::steady_clock::time_point start;
		// Момент окончания замера
		std::chrono::steady_clock::time_point finish;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Stream() noexcept :
		 sender(nullptr), request{}, queued(0), received(0), loop(nullptr) {}
	} stream_t;

	/**
	 * @brief Структура состояния прогона сценария установления соединений
	 *
	 */
	typedef struct Handshake {
		// Цикл событий стенда
		uv_loop_t * loop;
		// Параметры подключения к слушающему сокету
		struct sockaddr_in address;
		// Дескриптор текущего подключения
		uv_tcp_t * handle;
		// Запрос текущего подключения
		uv_connect_t * request;
		// Флаг активности замера
		bool measuring;
		// Количество выполненных циклов прогрева
		size_t warmed;
		// Количество выполненных циклов замера
		size_t done;
		// Момент начала замера
		std::chrono::steady_clock::time_point start;
		// Момент окончания замера
		std::chrono::steady_clock::time_point finish;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Handshake() noexcept :
		 loop(nullptr), address{}, handle(nullptr), request(nullptr),
		 measuring(false), warmed(0), done(0) {}
	} handshake_t;

	/**
	 * @brief Функция выделения буфера приёма данных
	 *
	 * @note Буфер общий на весь стенд: цикл событий однопоточный, и функция
	 *       обратного вызова чтения вызывается сразу за выделением
	 *
	 * @param buffer выводимый буфер приёма данных
	 *
	 */
	static void allocate(uv_handle_t *, size_t, uv_buf_t * buffer) noexcept {
		// Устанавливаем общий буфер приёма данных
		(*buffer) = ::uv_buf_init(reinterpret_cast <char *> (gBuffer.data()), static_cast <uint32_t> (gBuffer.size()));
	}
	/**
	 * @brief Функция обратного вызова освобождения дескриптора
	 *
	 * @param handle освобождаемый дескриптор
	 *
	 */
	static void release(uv_handle_t * handle) noexcept {
		// Выполняем освобождение дескриптора
		delete reinterpret_cast <uv_tcp_t *> (handle);
	}
	/**
	 * @brief Функция обратного вызова чтения принятого подключения
	 *
	 * @param stream дескриптор подключения
	 * @param size   размер принятых данных
	 * @param buffer буфер принятых данных
	 *
	 */
	static void peerRead(uv_stream_t * stream, ssize_t size, const uv_buf_t * buffer) noexcept {
		// Если данные не приняты
		if(size <= 0)
			// Ожидаем следующего чтения подключения
			return;
		// Получаем запрос отправки данных подключения
		uv_write_t * request = reinterpret_cast <uv_write_t *> (stream->data);
		// Формируем блок возвращаемых данных
		uv_buf_t response = ::uv_buf_init(buffer->base, static_cast <uint32_t> (size));
		// Возвращаем принятые данные отправителю
		::uv_write(request, stream, &response, 1, nullptr);
	}
	/**
	 * @brief Функция обратного вызова чтения клиентского подключения
	 *
	 * @param stream дескриптор подключения
	 * @param size   размер принятых данных
	 *
	 */
	static void clientRead(uv_stream_t * stream, ssize_t size, const uv_buf_t *) noexcept {
		// Если данные не приняты
		if(size <= 0)
			// Ожидаем следующего чтения подключения
			return;
		// Получаем состояние подключения
		connection_t * connection = reinterpret_cast <connection_t *> (stream->data);
		// Накапливаем количество принятых октетов текущего сообщения
		connection->received += static_cast <size_t> (size);
		// Если сообщение принято не полностью
		if(connection->received < ECHO_PAYLOAD)
			// Ожидаем оставшуюся часть сообщения
			return;
		// Учитываем принятое сообщение
		connection->received -= ECHO_PAYLOAD;
		// Если обмен следует продолжать
		if(connection->state->account()){
			// Формируем блок отправляемых данных
			uv_buf_t payload = ::uv_buf_init(reinterpret_cast <char *> (gPayload), ECHO_PAYLOAD);
			// Отправляем следующее сообщение обмена
			::uv_write(&connection->request, stream, &payload, 1, nullptr);
		// Останавливаем цикл событий
		} else ::uv_stop(stream->loop);
	}
	/**
	 * @brief Функция обратного вызова завершения клиентского подключения
	 *
	 * @param request запрос подключения к серверу
	 *
	 */
	static void clientConnect(uv_connect_t * request, int32_t) noexcept {
		// Получаем состояние подключения
		connection_t * connection = reinterpret_cast <connection_t *> (request->data);
		// Устанавливаем состояние подключения дескриптору
		connection->handle.data = connection;
		// Запускаем чтение данных подключения
		::uv_read_start(reinterpret_cast <uv_stream_t *> (&connection->handle), &::allocate, &::clientRead);
		// Формируем блок отправляемых данных
		uv_buf_t payload = ::uv_buf_init(reinterpret_cast <char *> (gPayload), ECHO_PAYLOAD);
		// Отправляем первое сообщение обмена
		::uv_write(&connection->request, reinterpret_cast <uv_stream_t *> (&connection->handle), &payload, 1, nullptr);
	}
	/**
	 * @brief Функция обратного вызова принятия входящего подключения сценария обмена
	 *
	 * @param server дескриптор слушающего сокета
	 *
	 */
	static void echoAccept(uv_stream_t * server, int32_t status) noexcept {
		// Если принятие подключения завершилось ошибкой
		if(status < 0)
			// Завершаем приём подключения
			return;
		// Создаём дескриптор принятого подключения
		uv_tcp_t * peer = new uv_tcp_t;
		// Выполняем инициализацию дескриптора принятого подключения
		::uv_tcp_init(server->loop, peer);
		// Если принятие подключения не выполнено
		if(::uv_accept(server, reinterpret_cast <uv_stream_t *> (peer)) != 0){
			// Выполняем закрытие дескриптора принятого подключения
			::uv_close(reinterpret_cast <uv_handle_t *> (peer), &::release);
			// Завершаем приём подключения
			return;
		}
		// Отключаем алгоритм Нейгла принятого подключения
		::uv_tcp_nodelay(peer, 1);
		// Создаём запрос отправки данных принятого подключения
		peer->data = new uv_write_t;
		// Запускаем чтение данных принятого подключения
		::uv_read_start(reinterpret_cast <uv_stream_t *> (peer), &::allocate, &::peerRead);
		// Считаем принятое подключение
		gAccepted++;
	}
	/**
	 * @brief Функция прогона сценария обмена короткими сообщениями
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
		echo_t state(connections, rounds);
		// Цикл событий стенда
		uv_loop_t loop{};
		// Выполняем инициализацию цикла событий стенда
		::uv_loop_init(&loop);
		// Параметры привязки слушающего сокета
		struct sockaddr_in address{};
		// Формируем адрес привязки слушающего сокета
		::uv_ip4_addr("127.0.0.1", 0, &address);
		// Дескриптор слушающего сокета
		uv_tcp_t server{};
		// Выполняем инициализацию дескриптора слушающего сокета
		::uv_tcp_init(&loop, &server);
		// Выполняем привязку слушающего сокета
		::uv_tcp_bind(&server, reinterpret_cast <const struct sockaddr *> (&address), 0);
		// Размер структуры параметров сокета
		int32_t length = sizeof(address);
		// Извлекаем параметры привязки слушающего сокета
		::uv_tcp_getsockname(&server, reinterpret_cast <struct sockaddr *> (&address), &length);
		// Переводим слушающий сокет в режим прослушивания
		::uv_listen(reinterpret_cast <uv_stream_t *> (&server), BACKLOG, &::echoAccept);
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
			// Устанавливаем состояние прогона сценария
			connection->state = &state;
			// Выполняем инициализацию дескриптора подключения
			::uv_tcp_init(&loop, &connection->handle);
			// Отключаем алгоритм Нейгла подключения
			::uv_tcp_nodelay(&connection->handle, 1);
			// Устанавливаем состояние подключения запросу подключения
			connection->connection.data = connection;
			// Выполняем подключение к слушающему сокету
			::uv_tcp_connect(&connection->connection, &connection->handle, reinterpret_cast <const struct sockaddr *> (&address), &::clientConnect);
		}
		/**
		 * Запускаем цикл событий до выполнения требуемого количества обменов
		 */
		::uv_run(&loop, UV_RUN_DEFAULT);
		// Устанавливаем количество выполненных операций
		result.operations = state.done;
		// Устанавливаем объём переданных данных с учётом обоих направлений обмена
		result.bytes = (state.done * ECHO_PAYLOAD * 2);
		// Устанавливаем затраченное время
		result.seconds = elapsed(state.start, state.finish);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция обратного вызова завершения подключения сценария наблюдения
	 *
	 * @details Отличается от обмена тем, что первое сообщение не отправляется:
	 *          обмен начинается лишь после того, как установлены все наблюдаемые
	 *          подключения. Иначе окно замера захватило бы часть установления, и
	 *          количество наблюдаемых во время замера не было бы постоянным
	 *
	 * @param request запрос подключения к серверу
	 *
	 */
	static void idleConnect(uv_connect_t * request, int32_t) noexcept {
		// Получаем состояние подключения
		connection_t * connection = reinterpret_cast <connection_t *> (request->data);
		// Устанавливаем состояние подключения дескриптору
		connection->handle.data = connection;
		// Дескриптор сокета подключения
		uv_os_fd_t fd = -1;
		// Если дескриптор сокета подключения получен
		if(::uv_fileno(reinterpret_cast <uv_handle_t *> (&connection->handle), &fd) == 0)
			// Включаем немедленный обрыв соединения при закрытии сокета
			hardClose(static_cast <int32_t> (fd));
		// Запускаем чтение данных подключения
		::uv_read_start(reinterpret_cast <uv_stream_t *> (&connection->handle), &::allocate, &::clientRead);
	}
	/**
	 * @brief Функция прогона сценария обмена при множестве наблюдаемых подключений
	 *
	 * @details Устанавливается заданное количество подключений, все они остаются
	 *          под наблюдением до конца прогона, но обмен идёт только на первых
	 *          сорока. Это единственный сценарий стенда, где готовых много меньше
	 *          наблюдаемых, - тот самый режим, ради которого создавались
	 *          современные механизмы опроса
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t watched() noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Состояние прогона сценария
		echo_t state(IDLE_ACTIVE, IDLE_ROUNDS);
		// Устанавливаем требуемое количество обменов прогрева
		state.warmup = IDLE_WARMUP;
		// Обнуляем количество принятых подключений
		gAccepted = 0;
		// Цикл событий стенда
		uv_loop_t loop{};
		// Выполняем инициализацию цикла событий стенда
		::uv_loop_init(&loop);
		// Параметры привязки слушающего сокета
		struct sockaddr_in address{};
		// Формируем адрес привязки слушающего сокета
		::uv_ip4_addr("127.0.0.1", 0, &address);
		// Дескриптор слушающего сокета
		uv_tcp_t server{};
		// Выполняем инициализацию дескриптора слушающего сокета
		::uv_tcp_init(&loop, &server);
		// Выполняем привязку слушающего сокета
		::uv_tcp_bind(&server, reinterpret_cast <const struct sockaddr *> (&address), 0);
		// Размер структуры параметров сокета
		int32_t length = sizeof(address);
		// Извлекаем параметры привязки слушающего сокета
		::uv_tcp_getsockname(&server, reinterpret_cast <struct sockaddr *> (&address), &length);
		// Переводим слушающий сокет в режим прослушивания
		::uv_listen(reinterpret_cast <uv_stream_t *> (&server), BACKLOG, &::echoAccept);
		// Список состояний клиентских подключений
		vector <unique_ptr <connection_t>> clients;
		// Резервируем память под состояния клиентских подключений
		clients.reserve(IDLE_WATCHED);
		/**
		 * Выполняем установление требуемого количества подключений порциями
		 */
		for(size_t i = 0; i < IDLE_WATCHED; i++){
			// Создаём состояние клиентского подключения
			clients.push_back(unique_ptr <connection_t> (new connection_t));
			// Получаем состояние созданного подключения
			connection_t * connection = clients.back().get();
			// Устанавливаем состояние прогона сценария
			connection->state = &state;
			// Выполняем инициализацию дескриптора подключения
			::uv_tcp_init(&loop, &connection->handle);
			// Отключаем алгоритм Нейгла подключения
			::uv_tcp_nodelay(&connection->handle, 1);
			// Устанавливаем состояние подключения запросу подключения
			connection->connection.data = connection;
			// Выполняем подключение к слушающему сокету
			::uv_tcp_connect(&connection->connection, &connection->handle, reinterpret_cast <const struct sockaddr *> (&address), &::idleConnect);
			// Если порция подключений подана не полностью, продолжаем её подачу
			if((((i + 1) % IDLE_BATCH) != 0) && ((i + 1) < IDLE_WATCHED))
				// Переходим к следующему подключению порции
				continue;
			// Запоминаем момент начала установления порции подключений
			const auto started = now();
			/**
			 * Дожидаемся принятия всех поданных подключений
			 */
			while(gAccepted <= i){
				// Прокручиваем цикл событий без ожидания
				::uv_run(&loop, UV_RUN_NOWAIT);
				// Если порция подключений не установилась в отведённый срок
				if(elapsed(started, now()) > IDLE_DEADLINE){
					// Сообщаем о неустановившейся порции: молча усечённый прогон читался бы как полный
					::fprintf(stderr, "idle: подключения не установились за %.0f с, подано %zu, принято %zu\n", IDLE_DEADLINE, (i + 1), gAccepted);
					// Выводим пустые итоги прогона
					return result;
				}
			}
		}
		/**
		 * Начинаем обмен на подключениях, которым он назначен
		 */
		for(size_t i = 0; i < IDLE_ACTIVE; i++){
			// Формируем блок отправляемых данных
			uv_buf_t payload = ::uv_buf_init(reinterpret_cast <char *> (gPayload), ECHO_PAYLOAD);
			// Отправляем первое сообщение обмена
			::uv_write(&clients[i]->request, reinterpret_cast <uv_stream_t *> (&clients[i]->handle), &payload, 1, nullptr);
		}
		/**
		 * Запускаем цикл событий до выполнения требуемого количества обменов
		 */
		::uv_run(&loop, UV_RUN_DEFAULT);
		// Устанавливаем количество выполненных операций
		result.operations = state.done;
		// Устанавливаем объём переданных данных с учётом обоих направлений обмена
		result.bytes = (state.done * ECHO_PAYLOAD * 2);
		// Устанавливаем затраченное время
		result.seconds = elapsed(state.start, state.finish);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция обратного вызова чтения приёмника потока
	 *
	 * @param stream дескриптор приёмника
	 * @param size   размер принятых данных
	 *
	 */
	static void streamRead(uv_stream_t * stream, ssize_t size, const uv_buf_t *) noexcept {
		// Если данные не приняты
		if(size <= 0)
			// Ожидаем следующего чтения приёмника
			return;
		// Получаем состояние прогона сценария
		stream_t * state = reinterpret_cast <stream_t *> (stream->data);
		// Накапливаем количество принятых октетов
		state->received += static_cast <size_t> (size);
		// Если весь объём передачи принят
		if(state->received >= STREAM_VOLUME){
			// Запоминаем момент окончания замера
			state->finish = now();
			// Останавливаем цикл событий
			::uv_stop(state->loop);
		}
	}
	/**
	 * @brief Функция обратного вызова записи блока передатчиком потока
	 *
	 * @param request запрос отправки блока
	 *
	 */
	static void streamWrite(uv_write_t * request, int32_t) noexcept {
		// Получаем состояние прогона сценария
		stream_t * state = reinterpret_cast <stream_t *> (request->data);
		// Если весь объём передачи поставлен в очередь
		if(state->queued >= STREAM_VOLUME)
			// Завершаем передачу
			return;
		// Считаем поставленный в очередь объём
		state->queued += STREAM_CHUNK;
		// Формируем блок передаваемых данных
		uv_buf_t chunk = ::uv_buf_init(reinterpret_cast <char *> (gChunk.data()), STREAM_CHUNK);
		// Ставим в очередь следующий блок передачи
		::uv_write(request, reinterpret_cast <uv_stream_t *> (state->sender), &chunk, 1, &::streamWrite);
	}
	/**
	 * @brief Функция обратного вызова принятия входящего подключения сценария потока
	 *
	 * @param server дескриптор слушающего сокета
	 *
	 */
	static void streamAccept(uv_stream_t * server, int32_t status) noexcept {
		// Если принятие подключения завершилось ошибкой
		if(status < 0)
			// Завершаем приём подключения
			return;
		// Создаём дескриптор принятого подключения
		uv_tcp_t * peer = new uv_tcp_t;
		// Выполняем инициализацию дескриптора принятого подключения
		::uv_tcp_init(server->loop, peer);
		// Если принятие подключения не выполнено
		if(::uv_accept(server, reinterpret_cast <uv_stream_t *> (peer)) != 0){
			// Выполняем закрытие дескриптора принятого подключения
			::uv_close(reinterpret_cast <uv_handle_t *> (peer), &::release);
			// Завершаем приём подключения
			return;
		}
		// Отключаем алгоритм Нейгла принятого подключения
		::uv_tcp_nodelay(peer, 1);
		// Устанавливаем состояние прогона сценария приёмнику
		peer->data = server->data;
		// Запускаем чтение данных приёмника
		::uv_read_start(reinterpret_cast <uv_stream_t *> (peer), &::allocate, &::streamRead);
	}
	/**
	 * @brief Функция прогона сценария потоковой передачи
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t transfer() noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Состояние прогона сценария
		stream_t state;
		// Цикл событий стенда
		uv_loop_t loop{};
		// Выполняем инициализацию цикла событий стенда
		::uv_loop_init(&loop);
		// Устанавливаем цикл событий стенда состоянию прогона
		state.loop = &loop;
		// Параметры привязки слушающего сокета
		struct sockaddr_in address{};
		// Формируем адрес привязки слушающего сокета
		::uv_ip4_addr("127.0.0.1", 0, &address);
		// Дескриптор слушающего сокета
		uv_tcp_t server{};
		// Выполняем инициализацию дескриптора слушающего сокета
		::uv_tcp_init(&loop, &server);
		// Устанавливаем состояние прогона сценария слушающему сокету
		server.data = &state;
		// Выполняем привязку слушающего сокета
		::uv_tcp_bind(&server, reinterpret_cast <const struct sockaddr *> (&address), 0);
		// Размер структуры параметров сокета
		int32_t length = sizeof(address);
		// Извлекаем параметры привязки слушающего сокета
		::uv_tcp_getsockname(&server, reinterpret_cast <struct sockaddr *> (&address), &length);
		// Переводим слушающий сокет в режим прослушивания
		::uv_listen(reinterpret_cast <uv_stream_t *> (&server), BACKLOG, &::streamAccept);
		// Дескриптор передатчика
		uv_tcp_t sender{};
		// Запрос подключения передатчика
		uv_connect_t connection{};
		// Выполняем инициализацию дескриптора передатчика
		::uv_tcp_init(&loop, &sender);
		// Отключаем алгоритм Нейгла передатчика
		::uv_tcp_nodelay(&sender, 1);
		// Устанавливаем дескриптор передатчика состоянию прогона
		state.sender = &sender;
		// Устанавливаем состояние прогона сценария запросу отправки
		state.request.data = &state;
		// Устанавливаем состояние прогона сценария запросу подключения
		connection.data = &state;
		/**
		 * @brief Функция обратного вызова завершения подключения передатчика
		 *
		 */
		auto connected = [](uv_connect_t * request, int32_t) noexcept -> void {
			// Получаем состояние прогона сценария
			stream_t * state = reinterpret_cast <stream_t *> (request->data);
			// Запоминаем момент начала замера
			state->start = now();
			// Считаем поставленный в очередь объём
			state->queued += STREAM_CHUNK;
			// Формируем блок передаваемых данных
			uv_buf_t chunk = ::uv_buf_init(reinterpret_cast <char *> (gChunk.data()), STREAM_CHUNK);
			// Ставим в очередь первый блок передачи
			::uv_write(&state->request, reinterpret_cast <uv_stream_t *> (state->sender), &chunk, 1, &::streamWrite);
		};
		// Выполняем подключение передатчика к слушающему сокету
		::uv_tcp_connect(&connection, &sender, reinterpret_cast <const struct sockaddr *> (&address), connected);
		/**
		 * Запускаем цикл событий до передачи всего объёма
		 */
		::uv_run(&loop, UV_RUN_DEFAULT);
		// Устанавливаем количество выполненных операций
		result.operations = (state.received / STREAM_CHUNK);
		// Устанавливаем объём переданных данных
		result.bytes = state.received;
		// Устанавливаем затраченное время
		result.seconds = elapsed(state.start, state.finish);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция обратного вызова принятия входящего подключения сценария соединений
	 *
	 * @param server дескриптор слушающего сокета
	 *
	 */
	static void handshakeAccept(uv_stream_t * server, int32_t status) noexcept {
		// Если принятие подключения завершилось ошибкой
		if(status < 0)
			// Завершаем приём подключения
			return;
		// Создаём дескриптор принятого подключения
		uv_tcp_t * peer = new uv_tcp_t;
		// Выполняем инициализацию дескриптора принятого подключения
		::uv_tcp_init(server->loop, peer);
		// Выполняем принятие входящего подключения
		::uv_accept(server, reinterpret_cast <uv_stream_t *> (peer));
		/**
		 * Подписываем принятое подключение на чтение
		 *
		 * @note Движок AWH отдаёт из приёма подключения готовый узел события,
		 *       подписанный на чтение, - настоящему серверу принятый сокет и нужен
		 *       именно таким. Прежде стенд принятое подключение сразу закрывал, и
		 *       подписка ему не стоила ничего
		 */
		::uv_read_start(reinterpret_cast <uv_stream_t *> (peer), &::allocate, &::peerRead);
		// Прекращаем чтение принятого подключения
		::uv_read_stop(reinterpret_cast <uv_stream_t *> (peer));
		// Выполняем закрытие принятого подключения
		::uv_close(reinterpret_cast <uv_handle_t *> (peer), &::release);
	}
	/**
	 * @brief Функция запуска очередного цикла подключения
	 *
	 * @param state состояние прогона сценария
	 *
	 */
	static void handshakeLaunch(handshake_t * state) noexcept;
	/**
	 * @brief Функция обратного вызова закрытия подключения
	 *
	 * @param handle закрытый дескриптор подключения
	 *
	 */
	static void handshakeClosed(uv_handle_t * handle) noexcept {
		// Получаем состояние прогона сценария
		handshake_t * state = reinterpret_cast <handshake_t *> (handle->data);
		// Выполняем освобождение дескриптора подключения
		delete reinterpret_cast <uv_tcp_t *> (handle);
		// Выполняем освобождение запроса подключения
		delete state->request;
		// Если замер ещё не начат
		if(!state->measuring){
			// Считаем выполненный цикл прогрева
			state->warmed++;
			// Если прогрев завершён
			if(state->warmed >= ACCEPT_WARMUP){
				// Включаем режим замера
				state->measuring = true;
				// Запоминаем момент начала замера
				state->start = now();
			}
		// Если замер выполняется
		} else {
			// Считаем выполненный цикл замера
			state->done++;
			// Если требуемое количество циклов выполнено
			if(state->done >= ACCEPT_ROUNDS){
				// Запоминаем момент окончания замера
				state->finish = now();
				// Останавливаем цикл событий
				::uv_stop(state->loop);
				// Завершаем прогон сценария
				return;
			}
		}
		// Запускаем очередной цикл подключения
		::handshakeLaunch(state);
	}
	/**
	 * @brief Функция обратного вызова завершения подключения
	 *
	 * @param request запрос подключения к серверу
	 *
	 */
	static void handshakeConnect(uv_connect_t * request, int32_t) noexcept {
		// Получаем состояние прогона сценария
		handshake_t * state = reinterpret_cast <handshake_t *> (request->data);
		// Устанавливаем состояние прогона сценария дескриптору подключения
		state->handle->data = state;
		// Дескриптор сокета подключения
		uv_os_fd_t fd = -1;
		/**
		 * Если дескриптор сокета подключения получен
		 *
		 * @note Библиотека создаёт сокет сама, и до завершения подключения
		 *       дескриптора у стенда нет. Опция задержки закрытия нужна лишь
		 *       к моменту закрытия, поэтому ставится здесь - и стенд перестаёт
		 *       расходовать динамические порты иначе, чем остальные три
		 */
		if(::uv_fileno(reinterpret_cast <uv_handle_t *> (state->handle), &fd) == 0)
			// Включаем немедленный обрыв соединения при закрытии сокета
			hardClose(static_cast <int32_t> (fd));
		// Выполняем закрытие текущего подключения
		::uv_close(reinterpret_cast <uv_handle_t *> (state->handle), &::handshakeClosed);
	}
	/**
	 * @brief Функция запуска очередного цикла подключения
	 *
	 * @param state состояние прогона сценария
	 *
	 */
	static void handshakeLaunch(handshake_t * state) noexcept {
		// Создаём дескриптор очередного подключения
		state->handle = new uv_tcp_t;
		// Создаём запрос очередного подключения
		state->request = new uv_connect_t;
		// Выполняем инициализацию дескриптора подключения
		::uv_tcp_init(state->loop, state->handle);
		// Отключаем алгоритм Нейгла подключения
		::uv_tcp_nodelay(state->handle, 1);
		// Устанавливаем состояние прогона сценария запросу подключения
		state->request->data = state;
		// Выполняем подключение к слушающему сокету
		::uv_tcp_connect(state->request, state->handle, reinterpret_cast <const struct sockaddr *> (&state->address), &::handshakeConnect);
	}
	/**
	 * @brief Функция прогона сценария установления соединений
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t handshake() noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Состояние прогона сценария
		handshake_t state;
		// Цикл событий стенда
		uv_loop_t loop{};
		// Выполняем инициализацию цикла событий стенда
		::uv_loop_init(&loop);
		// Устанавливаем цикл событий стенда состоянию прогона
		state.loop = &loop;
		// Формируем адрес привязки слушающего сокета
		::uv_ip4_addr("127.0.0.1", 0, &state.address);
		// Дескриптор слушающего сокета
		uv_tcp_t server{};
		// Выполняем инициализацию дескриптора слушающего сокета
		::uv_tcp_init(&loop, &server);
		// Выполняем привязку слушающего сокета
		::uv_tcp_bind(&server, reinterpret_cast <const struct sockaddr *> (&state.address), 0);
		// Размер структуры параметров сокета
		int32_t length = sizeof(state.address);
		// Извлекаем параметры привязки слушающего сокета
		::uv_tcp_getsockname(&server, reinterpret_cast <struct sockaddr *> (&state.address), &length);
		// Переводим слушающий сокет в режим прослушивания
		::uv_listen(reinterpret_cast <uv_stream_t *> (&server), BACKLOG, &::handshakeAccept);
		// Запускаем первый цикл подключения
		::handshakeLaunch(&state);
		/**
		 * Запускаем цикл событий до выполнения требуемого количества циклов
		 */
		::uv_run(&loop, UV_RUN_DEFAULT);
		// Устанавливаем количество выполненных операций
		result.operations = state.done;
		// Устанавливаем затраченное время
		result.seconds = elapsed(state.start, state.finish);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция обратного вызова дедлайна, который не должен сработать
	 *
	 */
	static void deadlineFired(uv_timer_t *) noexcept {}
	/**
	 * @brief Функция вычисления дедлайна таймера
	 *
	 * @param index порядковый номер таймера
	 * @return      дедлайн таймера в миллисекундах
	 *
	 */
	static uint64_t deadline(const size_t index) noexcept {
		// Выводим дедлайн таймера, отнесённый далеко в будущее
		return static_cast <uint64_t> (DEADLINE_OFFSET + (index % DEADLINE_SPREAD));
	}
	/**
	 * @brief Функция прогона сценария постановки таймеров с поиском по идентификатору
	 *
	 * @details Стенд поставлен в те же условия, что и движок AWH: описатель
	 *          наблюдателя берётся не напрямую, а разрешается поиском в реестре по
	 *          целочисленному идентификатору. Настоящему серверу описатель тоже
	 *          неоткуда взять иначе, поэтому стоимость поиска он платит в любом
	 *          случае - вопрос лишь в том, внутри библиотеки или снаружи
	 *
	 * @param count количество таймеров
	 * @return      итоги прогона сценария
	 *
	 */
	static outcome_t armingById(const size_t count) noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Список наблюдателей таймеров
		uv_loop_t loop;
		// Список наблюдателей таймеров
		vector <uv_timer_t> timers(count);
		// Инициализируем цикл событий стенда
		::uv_loop_init(&loop);
		// Реестр описателей наблюдателей по идентификатору
		registry_t <uv_timer_t *> registry;
		// Резервируем память под реестр описателей
		registry.reserve(count);
		/**
		 * Выполняем подготовку наблюдателей и наполнение реестра вне окна замера
		 */
		for(size_t i = 0; i < count; i++){
			// Инициализируем наблюдатель таймера
			::uv_timer_init(&loop, &timers[i]);
			// Записываем описатель наблюдателя в реестр
			registry.emplace(static_cast <uint32_t> (i + 1), &timers[i]);
		}
		// Время самого быстрого прохода замера
		double best = 0.0;
		/**
		 * Выполняем требуемое количество проходов замера
		 */
		for(size_t pass = 0; pass < DEADLINE_PASSES; pass++){
			// Снимаем таймеры, поставленные прошлым проходом, вне окна замера
			if(pass > 0){
				/**
				 * Выполняем снятие всех таймеров набора
				 */
				for(size_t k = 0; k < count; k++)
					// Снимаем таймер со структуры дедлайнов
					::uv_timer_stop(&timers[k]);
			}
			// Запоминаем момент начала замера
			const auto start = now();
			/**
			 * Выполняем постановку всех таймеров с разрешением описателя по идентификатору
			 */
			for(size_t k = 0; k < count; k++){
				// Разрешаем описатель наблюдателя по идентификатору
				auto i = registry.find(static_cast <uint32_t> (k + 1));
				// Если описатель наблюдателя не найден
				if(i == registry.end())
					// Переходим к следующему таймеру
					continue;
				// Ставим таймер в структуру дедлайнов
				::uv_timer_start(i->second, &::deadlineFired, ::deadline(i->first - 1), 0);
			}
			// Запоминаем момент окончания замера
			const auto finish = now();
			// Получаем время текущего прохода замера
			const double seconds = elapsed(start, finish);
			// Запоминаем время прохода, если он оказался самым быстрым
			if((best <= 0.0) || (seconds < best))
				// Запоминаем время самого быстрого прохода замера
				best = seconds;
		}
		// Устанавливаем количество выполненных операций
		result.operations = count;
		// Устанавливаем затраченное время
		result.seconds = best;
		// Освобождаем цикл событий стенда
		::uv_loop_close(&loop);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария отмены таймеров с поиском по идентификатору
	 *
	 * @param count количество таймеров
	 * @return      итоги прогона сценария
	 *
	 */
	static outcome_t cancellingById(const size_t count) noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Список наблюдателей таймеров
		uv_loop_t loop;
		// Список наблюдателей таймеров
		vector <uv_timer_t> timers(count);
		// Инициализируем цикл событий стенда
		::uv_loop_init(&loop);
		// Реестр описателей наблюдателей по идентификатору
		registry_t <uv_timer_t *> registry;
		// Резервируем память под реестр описателей
		registry.reserve(count);
		/**
		 * Выполняем подготовку, постановку и наполнение реестра вне окна замера
		 */
		for(size_t i = 0; i < count; i++){
			// Инициализируем наблюдатель таймера
			::uv_timer_init(&loop, &timers[i]);
			// Ставим таймер в структуру дедлайнов
			::uv_timer_start(&timers[i], &::deadlineFired, ::deadline(i), 0);
			// Записываем описатель наблюдателя в реестр
			registry.emplace(static_cast <uint32_t> (i + 1), &timers[i]);
		}
		// Время самого быстрого прохода замера
		double best = 0.0;
		/**
		 * Выполняем требуемое количество проходов замера
		 */
		for(size_t pass = 0; pass < DEADLINE_PASSES; pass++){
			// Возвращаем таймеры в структуру дедлайнов вне окна замера
			if(pass > 0){
				/**
				 * Выполняем постановку всех таймеров набора
				 */
				for(size_t k = 0; k < count; k++)
					// Ставим таймер в структуру дедлайнов
					::uv_timer_start(&timers[k], &::deadlineFired, ::deadline(k), 0);
			}
			// Запоминаем момент начала замера
			const auto start = now();
			/**
			 * Выполняем отмену всех таймеров с разрешением описателя по идентификатору
			 */
			for(size_t k = 0; k < count; k++){
				// Разрешаем описатель наблюдателя по идентификатору
				auto i = registry.find(static_cast <uint32_t> (k + 1));
				// Если описатель наблюдателя не найден
				if(i == registry.end())
					// Переходим к следующему таймеру
					continue;
				// Снимаем таймер со структуры дедлайнов
				::uv_timer_stop(i->second);
			}
			// Запоминаем момент окончания замера
			const auto finish = now();
			// Получаем время текущего прохода замера
			const double seconds = elapsed(start, finish);
			// Запоминаем время прохода, если он оказался самым быстрым
			if((best <= 0.0) || (seconds < best))
				// Запоминаем время самого быстрого прохода замера
				best = seconds;
		}
		// Устанавливаем количество выполненных операций
		result.operations = count;
		// Устанавливаем затраченное время
		result.seconds = best;
		// Освобождаем цикл событий стенда
		::uv_loop_close(&loop);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария перевзведения таймеров с поиском по идентификатору
	 *
	 * @param count количество таймеров
	 * @return      итоги прогона сценария
	 *
	 */
	static outcome_t rearmingById(const size_t count) noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Список наблюдателей таймеров
		uv_loop_t loop;
		// Список наблюдателей таймеров
		vector <uv_timer_t> timers(count);
		// Инициализируем цикл событий стенда
		::uv_loop_init(&loop);
		// Реестр описателей наблюдателей по идентификатору
		registry_t <uv_timer_t *> registry;
		// Резервируем память под реестр описателей
		registry.reserve(count);
		/**
		 * Выполняем подготовку, постановку и наполнение реестра вне окна замера
		 */
		for(size_t i = 0; i < count; i++){
			// Инициализируем наблюдатель таймера
			::uv_timer_init(&loop, &timers[i]);
			// Ставим таймер в структуру дедлайнов
			::uv_timer_start(&timers[i], &::deadlineFired, ::deadline(i), 0);
			// Записываем описатель наблюдателя в реестр
			registry.emplace(static_cast <uint32_t> (i + 1), &timers[i]);
		}
		// Время самого быстрого прохода замера
		double best = 0.0;
		/**
		 * Выполняем требуемое количество проходов замера
		 */
		for(size_t pass = 0; pass < DEADLINE_PASSES; pass++){
			// Запоминаем момент начала замера
			const auto start = now();
			/**
			 * Выполняем перевзведение всех таймеров с разрешением описателя по идентификатору
			 */
			for(size_t k = 0; k < count; k++){
				// Разрешаем описатель наблюдателя по идентификатору
				auto i = registry.find(static_cast <uint32_t> (k + 1));
				// Если описатель наблюдателя не найден
				if(i == registry.end())
					// Переходим к следующему таймеру
					continue;
				// Сдвигаем дедлайн таймера вперёд
				::uv_timer_start(i->second, &::deadlineFired, (::deadline(i->first - 1) + (static_cast <uint64_t> (pass + 1) * DEADLINE_SPREAD)), 0);
			}
			// Запоминаем момент окончания замера
			const auto finish = now();
			// Получаем время текущего прохода замера
			const double seconds = elapsed(start, finish);
			// Запоминаем время прохода, если он оказался самым быстрым
			if((best <= 0.0) || (seconds < best))
				// Запоминаем время самого быстрого прохода замера
				best = seconds;
		}
		// Устанавливаем количество выполненных операций
		result.operations = count;
		// Устанавливаем затраченное время
		result.seconds = best;
		// Освобождаем цикл событий стенда
		::uv_loop_close(&loop);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария постановки таймеров в структуру дедлайнов
	 *
	 * @param count количество таймеров
	 * @return      итоги прогона сценария
	 *
	 */
	static outcome_t arming(const size_t count) noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Объект цикла событий стенда
		uv_loop_t loop;
		// Список наблюдателей таймеров
		vector <uv_timer_t> timers(count);
		// Инициализируем цикл событий стенда
		::uv_loop_init(&loop);
		/**
		 * Выполняем подготовку наблюдателей вне окна замера
		 */
		for(size_t i = 0; i < count; i++)
			// Инициализируем наблюдатель таймера
			::uv_timer_init(&loop, &timers[i]);
		// Запоминаем момент начала замера
		const auto start = now();
		/**
		 * Выполняем постановку всех таймеров
		 */
		for(size_t i = 0; i < count; i++)
			// Ставим таймер в структуру дедлайнов
			::uv_timer_start(&timers[i], &::deadlineFired, ::deadline(i), 0);
		// Запоминаем момент окончания замера
		const auto finish = now();
		// Устанавливаем количество выполненных операций
		result.operations = count;
		// Устанавливаем затраченное время
		result.seconds = elapsed(start, finish);
		/**
		 * Снимаем наблюдатели таймеров
		 */
		for(size_t i = 0; i < count; i++)
			// Снимаем таймер со структуры дедлайнов
			::uv_timer_stop(&timers[i]);
		// Освобождаем цикл событий стенда
		::uv_loop_close(&loop);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария отмены таймеров в структуре дедлайнов
	 *
	 * @param count количество таймеров
	 * @return      итоги прогона сценария
	 *
	 */
	static outcome_t cancelling(const size_t count) noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Объект цикла событий стенда
		uv_loop_t loop;
		// Список наблюдателей таймеров
		vector <uv_timer_t> timers(count);
		// Инициализируем цикл событий стенда
		::uv_loop_init(&loop);
		/**
		 * Выполняем подготовку и постановку наблюдателей вне окна замера
		 */
		for(size_t i = 0; i < count; i++){
			// Инициализируем наблюдатель таймера
			::uv_timer_init(&loop, &timers[i]);
			// Ставим таймер в структуру дедлайнов
			::uv_timer_start(&timers[i], &::deadlineFired, ::deadline(i), 0);
		}
		// Запоминаем момент начала замера
		const auto start = now();
		/**
		 * Выполняем отмену всех таймеров
		 */
		for(size_t i = 0; i < count; i++)
			// Снимаем таймер со структуры дедлайнов
			::uv_timer_stop(&timers[i]);
		// Запоминаем момент окончания замера
		const auto finish = now();
		// Устанавливаем количество выполненных операций
		result.operations = count;
		// Устанавливаем затраченное время
		result.seconds = elapsed(start, finish);
		// Освобождаем цикл событий стенда
		::uv_loop_close(&loop);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария перевзведения таймеров в структуре дедлайнов
	 *
	 * @note Отдельной функции перевзведения библиотека не предоставляет: повторная
	 *       постановка уже поставленного таймера сама снимает прежнюю запись
	 *
	 * @param count количество таймеров
	 * @return      итоги прогона сценария
	 *
	 */
	static outcome_t rearming(const size_t count) noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Объект цикла событий стенда
		uv_loop_t loop;
		// Список наблюдателей таймеров
		vector <uv_timer_t> timers(count);
		// Инициализируем цикл событий стенда
		::uv_loop_init(&loop);
		/**
		 * Выполняем подготовку и постановку наблюдателей вне окна замера
		 */
		for(size_t i = 0; i < count; i++){
			// Инициализируем наблюдатель таймера
			::uv_timer_init(&loop, &timers[i]);
			// Ставим таймер в структуру дедлайнов
			::uv_timer_start(&timers[i], &::deadlineFired, ::deadline(i), 0);
		}
		// Запоминаем момент начала замера
		const auto start = now();
		/**
		 * Выполняем перевзведение всех таймеров
		 */
		for(size_t i = 0; i < count; i++)
			// Сдвигаем дедлайн таймера вперёд
			::uv_timer_start(&timers[i], &::deadlineFired, (::deadline(i) + DEADLINE_SPREAD), 0);
		// Запоминаем момент окончания замера
		const auto finish = now();
		// Устанавливаем количество выполненных операций
		result.operations = count;
		// Устанавливаем затраченное время
		result.seconds = elapsed(start, finish);
		/**
		 * Снимаем наблюдатели таймеров
		 */
		for(size_t i = 0; i < count; i++)
			// Снимаем таймер со структуры дедлайнов
			::uv_timer_stop(&timers[i]);
		// Освобождаем цикл событий стенда
		::uv_loop_close(&loop);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция обратного вызова срабатывания таймера
	 *
	 * @param timer сработавший таймер
	 *
	 */
	/**
	 * @brief Функция обратного вызова завершения закрытия таймера
	 *
	 * @param handle закрытый дескриптор таймера
	 *
	 */
	static void timerClosed(uv_handle_t * handle) noexcept {
		// Освобождаем дескриптор таймера
		delete reinterpret_cast <uv_timer_t *> (handle);
	}
	/**
	 * @brief Функция обратного вызова срабатывания таймера
	 *
	 * @param timer сработавший таймер
	 *
	 */
	static void timerFired(uv_timer_t * timer) noexcept {
		// Считаем сработавший таймер
		(*reinterpret_cast <size_t *> (timer->data))++;
		/**
		 * Закрываем и освобождаем таймер здесь же, внутри окна замера.
		 *
		 * Одноразовый таймер движка AWH - это узел события, который при срабатывании
		 * уничтожается, и стоимость уничтожения попадает в окно замера. Прежде
		 * дескрипторы libuv заготавливались вектором до начала окна и не
		 * освобождались вовсе, то есть соперник не платил ни за выделение, ни за
		 * освобождение. Освобождение у libuv отложенное: сперва требуется закрытие,
		 * и память возвращается в его функции завершения - но происходит это пока
		 * цикл работает, а значит внутри окна
		 */
		::uv_close(reinterpret_cast <uv_handle_t *> (timer), &::timerClosed);
	}
	/**
	 * @brief Функция прогона сценария обслуживания таймеров
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t schedule() noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Количество сработавших таймеров
		size_t fired = 0;
		// Цикл событий стенда
		uv_loop_t loop{};
		// Выполняем инициализацию цикла событий стенда
		::uv_loop_init(&loop);
		// Запоминаем момент начала замера
		const auto start = now();
		/**
		 * Выполняем постановку требуемого количества одноразовых таймеров.
		 *
		 * Дескриптор выделяется здесь же, внутри окна замера, а не заготавливается
		 * вектором до его начала, и освобождается по срабатыванию - тоже внутри окна.
		 * Одноразовый таймер движка AWH - это узел события, который движок заводит
		 * при постановке и уничтожает при срабатывании, и обе стоимости попадают в
		 * окно. Заготовка всех дескрипторов заранее снимала бы с соперника и
		 * выделение, и освобождение целиком
		 */
		for(size_t i = 0; i < TIMER_COUNT; i++){
			// Выделяем дескриптор таймера
			uv_timer_t * timer = new uv_timer_t;
			// Выполняем инициализацию таймера
			::uv_timer_init(&loop, timer);
			// Устанавливаем счётчик сработавших таймеров таймеру
			timer->data = &fired;
			// Выполняем постановку таймера с разбросом по диапазону
			::uv_timer_start(timer, &::timerFired, static_cast <uint64_t> (1 + (i % TIMER_SPREAD)), 0);
		}
		/**
		 * Запускаем цикл событий до срабатывания всех таймеров
		 */
		::uv_run(&loop, UV_RUN_DEFAULT);
		// Запоминаем момент окончания замера
		const auto finish = now();
		// Устанавливаем количество выполненных операций
		result.operations = fired;
		// Устанавливаем затраченное время
		result.seconds = elapsed(start, finish);
		// Выводим итоги прогона сценария
		return result;
	}
};

/**
 * @brief Главная функция стенда
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из стенда
 *
 */
int32_t main(int32_t argc, char ** argv){
	// Получаем фильтр названий выполняемых сценариев
	const char * name = filter(argc, argv);
	// Выводим заголовок таблицы результатов
	::printf("СТЕНД libuv %s\n\nСЦЕНАРИЙ                               ИЗМЕРЕНО\n", ::uv_version_string());
	// Если сценарий обмена на одном подключении выполняется
	if(selected("net/io/echo/single", name)){
		// Выполняем прогон сценария обмена на одном подключении
		const outcome_t outcome = ::exchange(1, ECHO_SINGLE_ROUNDS);
		// Выводим результат прогона сценария
		report("net/io/echo/single", "обменов/с", perSecond(outcome), outcome);
	}
	// Если сценарий обмена на множестве подключений выполняется
	if(selected("net/io/idle/exchanges", name)){
		// Выполняем прогон сценария обмена при множестве наблюдаемых подключений
		const outcome_t outcome = ::watched();
		// Выводим результат прогона сценария
		report("net/io/idle/exchanges", "обменов/с", perSecond(outcome), outcome);
	}
	if(selected("net/io/echo/multi", name)){
		// Выполняем прогон сценария обмена на множестве подключений
		const outcome_t outcome = ::exchange(ECHO_MULTI_CONNECTIONS, ECHO_MULTI_ROUNDS);
		// Выводим результат прогона сценария
		report("net/io/echo/multi", "обменов/с", perSecond(outcome), outcome);
	}
	// Если сценарий потоковой передачи выполняется
	if(selected("net/io/stream/throughput", name)){
		// Выполняем прогон сценария потоковой передачи
		const outcome_t outcome = ::transfer();
		// Выводим результат прогона сценария
		report("net/io/stream/throughput", "МБ/с", megabytes(outcome), outcome);
	}
	// Если сценарий установления соединений выполняется
	if(selected("net/io/accept/connections", name)){
		// Выполняем прогон сценария установления соединений
		const outcome_t outcome = ::handshake();
		// Выводим результат прогона сценария
		report("net/io/accept/connections", "подключений/с", perSecond(outcome), outcome);
	}
	// Если сценарий обслуживания таймеров выполняется
	if(selected("net/io/timers", name)){
		// Выполняем прогон сценария обслуживания таймеров
		const outcome_t outcome = ::schedule();
		// Выводим результат прогона сценария
		report("net/io/timers", "таймеров/с", perSecond(outcome), outcome);
	}
	// Если сценарий постановки таймеров выполняется
	if(selected("net/io/deadlines/arm", name)){
		// Выполняем прогон сценария постановки таймеров
		const outcome_t outcome = ::arming(DEADLINE_COUNT);
		// Выводим результат прогона сценария
		report("net/io/deadlines/arm", "постановок/с", perSecond(outcome), outcome);
	}
	// Если сценарий отмены таймеров выполняется
	if(selected("net/io/deadlines/cancel", name)){
		// Выполняем прогон сценария отмены таймеров
		const outcome_t outcome = ::cancelling(DEADLINE_COUNT);
		// Выводим результат прогона сценария
		report("net/io/deadlines/cancel", "отмен/с", perSecond(outcome), outcome);
	}
	// Если сценарий перевзведения таймеров выполняется
	if(selected("net/io/deadlines/rearm", name)){
		// Выполняем прогон сценария перевзведения таймеров
		const outcome_t outcome = ::rearming(DEADLINE_COUNT);
		// Выводим результат прогона сценария
		report("net/io/deadlines/rearm", "перевзведений/с", perSecond(outcome), outcome);
	}
	// Если сценарий оценки сложности структуры дедлайнов выполняется
	if(selected("net/io/deadlines/scaling", name)){
		// Выполняем прогон на уменьшенном количестве таймеров
		const outcome_t small = ::arming(DEADLINE_SMALL_COUNT);
		// Выполняем прогон на полном количестве таймеров
		const outcome_t large = ::arming(DEADLINE_COUNT);
		// Вычисляем стоимость одной операции на уменьшенном прогоне
		const double base = ((small.seconds * 1e6) / static_cast <double> (small.operations));
		// Вычисляем стоимость одной операции на полном прогоне
		const double cost = ((large.seconds * 1e6) / static_cast <double> (large.operations));
		// Выводим отношение стоимостей одной операции
		::printf("%-38s %14.2f   (отношение, %.3f -> %.3f мкс)\n", "net/io/deadlines/scaling", (cost / base), base, cost);
	}
	// Если сценарий постановки таймеров с поиском по идентификатору выполняется
	if(selected("net/io/deadlines/arm-by-id", name)){
		// Выполняем прогон сценария постановки таймеров с поиском по идентификатору
		const outcome_t outcome = ::armingById(DEADLINE_COUNT);
		// Выводим результат прогона сценария
		report("net/io/deadlines/arm-by-id", "постановок/с", perSecond(outcome), outcome);
	}
	// Если сценарий отмены таймеров с поиском по идентификатору выполняется
	if(selected("net/io/deadlines/cancel-by-id", name)){
		// Выполняем прогон сценария отмены таймеров с поиском по идентификатору
		const outcome_t outcome = ::cancellingById(DEADLINE_COUNT);
		// Выводим результат прогона сценария
		report("net/io/deadlines/cancel-by-id", "отмен/с", perSecond(outcome), outcome);
	}
	// Если сценарий перевзведения таймеров с поиском по идентификатору выполняется
	if(selected("net/io/deadlines/rearm-by-id", name)){
		// Выполняем прогон сценария перевзведения таймеров с поиском по идентификатору
		const outcome_t outcome = ::rearmingById(DEADLINE_COUNT);
		// Выводим результат прогона сценария
		report("net/io/deadlines/rearm-by-id", "перевзведений/с", perSecond(outcome), outcome);
	}
	// Выводим успешный код выхода
	return EXIT_SUCCESS;
}
