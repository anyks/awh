/**
 * @file: libuv.cpp
 * @date: 2026-07-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Эталонный стенд сравнения на libuv — те же сценарии нагрузки, что и у бенчмарков
 *        сетевого движка AWH, выполненные средствами интерфейса завершения операций библиотеки
 *
 * @copyright: Copyright © 2026
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
	 * @brief Функция обратного вызова срабатывания таймера
	 *
	 * @param timer сработавший таймер
	 *
	 */
	static void timerFired(uv_timer_t * timer) noexcept {
		// Считаем сработавший таймер
		(*reinterpret_cast <size_t *> (timer->data))++;
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
		// Список таймеров стенда
		vector <uv_timer_t> timers(TIMER_COUNT);
		// Цикл событий стенда
		uv_loop_t loop{};
		// Выполняем инициализацию цикла событий стенда
		::uv_loop_init(&loop);
		// Запоминаем момент начала замера
		const auto start = now();
		/**
		 * Выполняем постановку требуемого количества одноразовых таймеров
		 */
		for(size_t i = 0; i < TIMER_COUNT; i++){
			// Выполняем инициализацию таймера
			::uv_timer_init(&loop, &timers[i]);
			// Устанавливаем счётчик сработавших таймеров таймеру
			timers[i].data = &fired;
			// Выполняем постановку таймера с разбросом по диапазону
			::uv_timer_start(&timers[i], &::timerFired, static_cast <uint64_t> (1 + (i % TIMER_SPREAD)), 0);
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
	// Выводим успешный код выхода
	return EXIT_SUCCESS;
}
