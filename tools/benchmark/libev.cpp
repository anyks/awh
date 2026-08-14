/**
 * @file libev.cpp
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
 * @brief Эталонный стенд сравнения на libev — те же сценарии нагрузки, что и у бенчмарков
 *        сетевого движка AWH, выполненные средствами наблюдателей готовности библиотеки
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
 * Подключаем заголовочные файлы библиотеки libev
 */
#include <ev.h>

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
 * @details Библиотека предоставляет только наблюдатели готовности: буферизации
 *          и очереди отправки у неё нет вовсе, поэтому запись выполняется прямо
 *          в сокет по готовности. Память наблюдателей принадлежит приложению -
 *          в замер входит только их инициализация и постановка
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
		// Наблюдатель готовности подключения
		ev_io watcher;
		// Дескриптор сокета подключения
		int32_t fd;
		// Количество принятых октетов текущего сообщения
		size_t received;
		// Состояние прогона сценария
		echo_t * state;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Connection() noexcept : watcher{}, fd(-1), received(0), state(nullptr) {}
	} connection_t;

	/**
	 * @brief Структура контекста слушающего сокета сценария обмена
	 *
	 */
	typedef struct Acceptor {
		// Наблюдатель готовности слушающего сокета
		ev_io watcher;
		// Список состояний принятых подключений
		vector <unique_ptr <connection_t>> * accepted;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Acceptor() noexcept : watcher{}, accepted(nullptr) {}
	} acceptor_t;

	/**
	 * @brief Структура состояния прогона сценария пропускной способности
	 *
	 */
	typedef struct Stream {
		// Наблюдатель готовности приёмника
		ev_io reader;
		// Наблюдатель готовности передатчика
		ev_io writer;
		// Количество октетов текущего блока, ожидающих записи в сокет
		size_t pending;
		// Количество поставленных в очередь октетов
		size_t queued;
		// Количество принятых приёмником октетов
		size_t received;
		// Момент начала замера
		std::chrono::steady_clock::time_point start;
		// Момент окончания замера
		std::chrono::steady_clock::time_point finish;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Stream() noexcept :
		 reader{}, writer{}, pending(0), queued(0), received(0) {}
	} stream_t;

	/**
	 * @brief Структура состояния прогона сценария установления соединений
	 *
	 */
	typedef struct Handshake {
		// Наблюдатель завершения текущего подключения
		ev_io watcher;
		// Параметры подключения к слушающему сокету
		struct sockaddr_in address;
		// Дескриптор сокета текущего подключения
		int32_t fd;
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
		 watcher{}, address{}, fd(-1), measuring(false), warmed(0), done(0) {}
	} handshake_t;

	/**
	 * @brief Функция обратного вызова готовности чтения принятого подключения
	 *
	 * @param watcher наблюдатель готовности подключения
	 *
	 */
	static void peerRead(struct ev_loop *, ev_io * watcher, int32_t) noexcept {
		// Выполняем чтение принятых данных
		const ssize_t bytes = ::recv(watcher->fd, gBuffer.data(), gBuffer.size(), 0);
		// Если данные приняты
		if(bytes > 0)
			// Возвращаем принятые данные отправителю
			::send(watcher->fd, gBuffer.data(), static_cast <size_t> (bytes), 0);
	}
	/**
	 * @brief Функция обратного вызова готовности чтения клиентского подключения
	 *
	 * @param loop    цикл событий стенда
	 * @param watcher наблюдатель готовности подключения
	 *
	 */
	static void clientRead(struct ev_loop * loop, ev_io * watcher, int32_t) noexcept {
		// Получаем состояние подключения
		connection_t * connection = reinterpret_cast <connection_t *> (watcher->data);
		// Выполняем чтение принятых данных
		const ssize_t bytes = ::recv(watcher->fd, gBuffer.data(), ECHO_PAYLOAD, 0);
		// Если данные не приняты
		if(bytes <= 0)
			// Ожидаем следующей готовности подключения
			return;
		// Накапливаем количество принятых октетов текущего сообщения
		connection->received += static_cast <size_t> (bytes);
		// Если сообщение принято не полностью
		if(connection->received < ECHO_PAYLOAD)
			// Ожидаем оставшуюся часть сообщения
			return;
		// Учитываем принятое сообщение
		connection->received -= ECHO_PAYLOAD;
		// Если обмен следует продолжать
		if(connection->state->account())
			// Отправляем следующее сообщение обмена
			::send(watcher->fd, gPayload, ECHO_PAYLOAD, 0);
		// Останавливаем цикл событий
		else ::ev_break(loop, EVBREAK_ALL);
	}
	/**
	 * @brief Функция обратного вызова завершения клиентского подключения
	 *
	 * @param loop    цикл событий стенда
	 * @param watcher наблюдатель завершения подключения
	 *
	 */
	static void clientConnect(struct ev_loop * loop, ev_io * watcher, int32_t) noexcept {
		// Получаем состояние подключения
		connection_t * connection = reinterpret_cast <connection_t *> (watcher->data);
		// Прекращаем наблюдение за завершением подключения
		::ev_io_stop(loop, watcher);
		// Инициализируем наблюдатель готовности чтения подключения
		ev_io_init(watcher, &::clientRead, connection->fd, EV_READ);
		// Устанавливаем состояние подключения наблюдателю
		watcher->data = connection;
		// Активируем наблюдатель готовности чтения
		::ev_io_start(loop, watcher);
		// Отправляем первое сообщение обмена
		::send(connection->fd, gPayload, ECHO_PAYLOAD, 0);
	}
	/**
	 * @brief Функция обратного вызова принятия входящего подключения сценария обмена
	 *
	 * @param loop    цикл событий стенда
	 * @param watcher наблюдатель готовности слушающего сокета
	 *
	 */
	static void echoAccept(struct ev_loop * loop, ev_io * watcher, int32_t) noexcept {
		// Получаем контекст слушающего сокета
		acceptor_t * context = reinterpret_cast <acceptor_t *> (watcher->data);
		/**
		 * Выполняем приём всех ожидающих подключений
		 */
		while(true){
			// Выполняем приём входящего подключения
			const int32_t peer = ::accept(watcher->fd, nullptr, nullptr);
			// Если ожидающих подключений не осталось
			if(peer < 0)
				// Завершаем приём подключений
				return;
			// Выполняем настройку принятого сокета
			adjust(peer);
			// Создаём состояние принятого подключения
			context->accepted->push_back(unique_ptr <connection_t> (new connection_t));
			// Получаем состояние принятого подключения
			connection_t * connection = context->accepted->back().get();
			// Устанавливаем дескриптор сокета подключения
			connection->fd = peer;
			// Инициализируем наблюдатель готовности чтения принятого подключения
			ev_io_init(&connection->watcher, &::peerRead, peer, EV_READ);
			// Устанавливаем состояние подключения наблюдателю
			connection->watcher.data = connection;
			// Активируем наблюдатель готовности чтения
			::ev_io_start(loop, &connection->watcher);
		}
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
		// Создаём цикл событий стенда
		struct ev_loop * loop = ::ev_loop_new(EVFLAG_AUTO);
		// Параметры привязки слушающего сокета
		struct sockaddr_in address{};
		// Создаём слушающий сокет петлевого интерфейса
		const int32_t server = listener(address);
		// Список состояний принятых подключений
		vector <unique_ptr <connection_t>> accepted;
		// Резервируем память под состояния принятых подключений
		accepted.reserve(connections);
		// Список состояний клиентских подключений
		vector <unique_ptr <connection_t>> clients;
		// Резервируем память под состояния клиентских подключений
		clients.reserve(connections);
		// Контекст слушающего сокета
		acceptor_t context;
		// Устанавливаем список состояний принятых подключений
		context.accepted = &accepted;
		// Инициализируем наблюдатель готовности слушающего сокета
		ev_io_init(&context.watcher, &::echoAccept, server, EV_READ);
		// Устанавливаем контекст слушающего сокета наблюдателю
		context.watcher.data = &context;
		// Активируем наблюдатель готовности слушающего сокета
		::ev_io_start(loop, &context.watcher);
		/**
		 * Выполняем создание требуемого количества клиентских подключений
		 */
		for(size_t i = 0; i < connections; i++){
			// Создаём состояние клиентского подключения
			clients.push_back(unique_ptr <connection_t> (new connection_t));
			// Получаем состояние созданного подключения
			connection_t * connection = clients.back().get();
			// Выполняем подключение к слушающему сокету
			connection->fd = connector(address);
			// Устанавливаем состояние прогона сценария
			connection->state = &state;
			// Инициализируем наблюдатель завершения подключения
			ev_io_init(&connection->watcher, &::clientConnect, connection->fd, EV_WRITE);
			// Устанавливаем состояние подключения наблюдателю
			connection->watcher.data = connection;
			// Активируем наблюдатель завершения подключения
			::ev_io_start(loop, &connection->watcher);
		}
		/**
		 * Запускаем цикл событий до выполнения требуемого количества обменов
		 */
		::ev_run(loop, 0);
		// Устанавливаем количество выполненных операций
		result.operations = state.done;
		// Устанавливаем объём переданных данных с учётом обоих направлений обмена
		result.bytes = (state.done * ECHO_PAYLOAD * 2);
		// Устанавливаем затраченное время
		result.seconds = elapsed(state.start, state.finish);
		/**
		 * Выполняем освобождение принятых подключений
		 */
		for(auto & connection : accepted)
			// Выполняем закрытие сокета подключения
			::close(connection->fd);
		/**
		 * Выполняем освобождение клиентских подключений
		 */
		for(auto & connection : clients)
			// Выполняем закрытие сокета подключения
			::close(connection->fd);
		// Выполняем закрытие слушающего сокета
		::close(server);
		// Освобождаем цикл событий стенда
		::ev_loop_destroy(loop);
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
	 * @param loop    цикл событий стенда
	 * @param watcher наблюдатель завершения подключения
	 *
	 */
	static void idleConnect(struct ev_loop * loop, ev_io * watcher, int32_t) noexcept {
		// Получаем состояние подключения
		connection_t * connection = reinterpret_cast <connection_t *> (watcher->data);
		// Прекращаем наблюдение за завершением подключения
		::ev_io_stop(loop, watcher);
		// Инициализируем наблюдатель готовности чтения подключения
		ev_io_init(watcher, &::clientRead, connection->fd, EV_READ);
		// Устанавливаем состояние подключения наблюдателю
		watcher->data = connection;
		// Активируем наблюдатель готовности чтения
		::ev_io_start(loop, watcher);
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
		// Создаём цикл событий стенда
		struct ev_loop * loop = ::ev_loop_new(EVFLAG_AUTO);
		// Параметры привязки слушающего сокета
		struct sockaddr_in address{};
		// Создаём слушающий сокет петлевого интерфейса
		const int32_t server = listener(address);
		// Список состояний принятых подключений
		vector <unique_ptr <connection_t>> accepted;
		// Резервируем память под состояния принятых подключений
		accepted.reserve(IDLE_WATCHED);
		// Список состояний клиентских подключений
		vector <unique_ptr <connection_t>> clients;
		// Резервируем память под состояния клиентских подключений
		clients.reserve(IDLE_WATCHED);
		// Контекст слушающего сокета
		acceptor_t context;
		// Устанавливаем список состояний принятых подключений
		context.accepted = &accepted;
		// Инициализируем наблюдатель готовности слушающего сокета
		ev_io_init(&context.watcher, &::echoAccept, server, EV_READ);
		// Устанавливаем контекст слушающего сокета наблюдателю
		context.watcher.data = &context;
		// Активируем наблюдатель готовности слушающего сокета
		::ev_io_start(loop, &context.watcher);
		/**
		 * Выполняем установление требуемого количества подключений порциями
		 */
		for(size_t i = 0; i < IDLE_WATCHED; i++){
			// Создаём состояние клиентского подключения
			clients.push_back(unique_ptr <connection_t> (new connection_t));
			// Получаем состояние созданного подключения
			connection_t * connection = clients.back().get();
			// Выполняем подключение к слушающему сокету
			connection->fd = connector(address);
			// Включаем немедленный обрыв соединения при закрытии сокета
			hardClose(connection->fd);
			// Устанавливаем состояние прогона сценария
			connection->state = &state;
			// Инициализируем наблюдатель завершения подключения
			ev_io_init(&connection->watcher, &::idleConnect, connection->fd, EV_WRITE);
			// Устанавливаем состояние подключения наблюдателю
			connection->watcher.data = connection;
			// Активируем наблюдатель завершения подключения
			::ev_io_start(loop, &connection->watcher);
			// Если порция подключений подана не полностью, продолжаем её подачу
			if((((i + 1) % IDLE_BATCH) != 0) && ((i + 1) < IDLE_WATCHED))
				// Переходим к следующему подключению порции
				continue;
			// Запоминаем момент начала установления порции подключений
			const auto started = now();
			/**
			 * Дожидаемся принятия всех поданных подключений
			 */
			while(accepted.size() <= i){
				// Прокручиваем цикл событий без ожидания
				::ev_run(loop, EVRUN_NOWAIT);
				// Если порция подключений не установилась в отведённый срок
				if(elapsed(started, now()) > IDLE_DEADLINE){
					// Сообщаем о неустановившейся порции: молча усечённый прогон читался бы как полный
					::fprintf(stderr, "idle: подключения не установились за %.0f с, подано %zu, принято %zu\n", IDLE_DEADLINE, (i + 1), accepted.size());
					// Выводим пустые итоги прогона
					return result;
				}
			}
		}
		/**
		 * Начинаем обмен на подключениях, которым он назначен
		 */
		for(size_t i = 0; i < IDLE_ACTIVE; i++)
			// Отправляем первое сообщение обмена
			::send(clients[i]->fd, gPayload, ECHO_PAYLOAD, 0);
		/**
		 * Запускаем цикл событий до выполнения требуемого количества обменов
		 */
		::ev_run(loop, 0);
		// Устанавливаем количество выполненных операций
		result.operations = state.done;
		// Устанавливаем объём переданных данных с учётом обоих направлений обмена
		result.bytes = (state.done * ECHO_PAYLOAD * 2);
		// Устанавливаем затраченное время
		result.seconds = elapsed(state.start, state.finish);
		/**
		 * Выполняем освобождение принятых подключений
		 */
		for(auto & connection : accepted)
			// Выполняем закрытие сокета подключения
			::close(connection->fd);
		/**
		 * Выполняем освобождение клиентских подключений
		 */
		for(auto & connection : clients)
			// Выполняем закрытие сокета подключения
			::close(connection->fd);
		// Выполняем закрытие слушающего сокета
		::close(server);
		// Освобождаем цикл событий стенда
		::ev_loop_destroy(loop);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция обратного вызова готовности чтения приёмника потока
	 *
	 * @param loop    цикл событий стенда
	 * @param watcher наблюдатель готовности приёмника
	 *
	 */
	static void streamRead(struct ev_loop * loop, ev_io * watcher, int32_t) noexcept {
		// Получаем состояние прогона сценария
		stream_t * state = reinterpret_cast <stream_t *> (watcher->data);
		// Выполняем чтение принятых данных
		const ssize_t bytes = ::recv(watcher->fd, gBuffer.data(), gBuffer.size(), 0);
		// Если данные не приняты
		if(bytes <= 0)
			// Ожидаем следующей готовности приёмника
			return;
		// Накапливаем количество принятых октетов
		state->received += static_cast <size_t> (bytes);
		// Если весь объём передачи принят
		if(state->received >= STREAM_VOLUME){
			// Запоминаем момент окончания замера
			state->finish = now();
			// Останавливаем цикл событий
			::ev_break(loop, EVBREAK_ALL);
		}
	}
	/**
	 * @brief Функция обратного вызова готовности записи передатчика потока
	 *
	 * @param loop    цикл событий стенда
	 * @param watcher наблюдатель готовности передатчика
	 *
	 */
	static void streamWrite(struct ev_loop * loop, ev_io * watcher, int32_t) noexcept {
		// Получаем состояние прогона сценария
		stream_t * state = reinterpret_cast <stream_t *> (watcher->data);
		// Выполняем запись остатка текущего блока
		const ssize_t bytes = ::send(watcher->fd, gChunk.data() + (STREAM_CHUNK - state->pending), state->pending, 0);
		// Если запись не выполнена
		if(bytes <= 0)
			// Ожидаем следующей готовности передатчика
			return;
		// Уменьшаем остаток блока на записанный объём
		state->pending -= static_cast <size_t> (bytes);
		// Если блок записан не полностью
		if(state->pending > 0)
			// Ожидаем записи оставшейся части блока
			return;
		// Если весь объём передачи поставлен в очередь
		if(state->queued >= STREAM_VOLUME){
			// Прекращаем наблюдение за готовностью записи
			::ev_io_stop(loop, watcher);
			// Завершаем передачу
			return;
		}
		// Устанавливаем количество октетов следующего блока
		state->pending = STREAM_CHUNK;
		// Считаем поставленный в очередь объём
		state->queued += STREAM_CHUNK;
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
		// Создаём цикл событий стенда
		struct ev_loop * loop = ::ev_loop_new(EVFLAG_AUTO);
		// Параметры привязки слушающего сокета
		struct sockaddr_in address{};
		// Создаём слушающий сокет петлевого интерфейса
		const int32_t server = listener(address);
		// Выполняем подключение к слушающему сокету
		const int32_t client = connector(address);
		// Дескриптор принятого подключения
		int32_t peer = -1;
		/**
		 * Ожидаем принятия входящего подключения
		 */
		while(peer < 0)
			// Выполняем приём входящего подключения
			peer = ::accept(server, nullptr, nullptr);
		// Выполняем настройку принятого сокета
		adjust(peer);
		// Инициализируем наблюдатель готовности чтения приёмника
		ev_io_init(&state.reader, &::streamRead, peer, EV_READ);
		// Устанавливаем состояние прогона сценария наблюдателю
		state.reader.data = &state;
		// Активируем наблюдатель готовности чтения приёмника
		::ev_io_start(loop, &state.reader);
		// Инициализируем наблюдатель готовности записи передатчика
		ev_io_init(&state.writer, &::streamWrite, client, EV_WRITE);
		// Устанавливаем состояние прогона сценария наблюдателю
		state.writer.data = &state;
		// Активируем наблюдатель готовности записи передатчика
		::ev_io_start(loop, &state.writer);
		// Устанавливаем количество октетов первого блока
		state.pending = STREAM_CHUNK;
		// Считаем поставленный в очередь объём
		state.queued = STREAM_CHUNK;
		// Запоминаем момент начала замера
		state.start = now();
		/**
		 * Запускаем цикл событий до передачи всего объёма
		 */
		::ev_run(loop, 0);
		// Устанавливаем количество выполненных операций
		result.operations = (state.received / STREAM_CHUNK);
		// Устанавливаем объём переданных данных
		result.bytes = state.received;
		// Устанавливаем затраченное время
		result.seconds = elapsed(state.start, state.finish);
		// Выполняем закрытие сокета приёмника
		::close(peer);
		// Выполняем закрытие сокета передатчика
		::close(client);
		// Выполняем закрытие слушающего сокета
		::close(server);
		// Освобождаем цикл событий стенда
		::ev_loop_destroy(loop);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция обратного вызова принятия входящего подключения сценария соединений
	 *
	 * @param watcher наблюдатель готовности слушающего сокета
	 *
	 */
	static void handshakeAccept(struct ev_loop * loop, ev_io * watcher, int32_t) noexcept {
		/**
		 * Выполняем приём всех ожидающих подключений
		 */
		while(true){
			// Выполняем приём входящего подключения
			const int32_t peer = ::accept(watcher->fd, nullptr, nullptr);
			// Если ожидающих подключений не осталось
			if(peer < 0)
				// Завершаем приём подключений
				return;
			/**
			 * Заводим наблюдатель готовности принятого подключения
			 *
			 * @note Движок AWH отдаёт из приёма подключения готовый узел события,
			 *       подписанный на чтение, - настоящему серверу принятый сокет и
			 *       нужен именно таким. Прежде стенд принятое подключение сразу
			 *       закрывал, и подписка ему не стоила ничего
			 */
			ev_io peerWatcher;
			// Инициализируем наблюдатель готовности принятого подключения
			ev_io_init(&peerWatcher, &::handshakeAccept, peer, EV_READ);
			// Активируем наблюдатель готовности принятого подключения
			::ev_io_start(loop, &peerWatcher);
			// Прекращаем наблюдение за принятым подключением
			::ev_io_stop(loop, &peerWatcher);
			// Выполняем закрытие принятого подключения
			::close(peer);
		}
	}
	/**
	 * @brief Функция обратного вызова завершения подключения
	 *
	 * @param loop    цикл событий стенда
	 * @param watcher наблюдатель завершения подключения
	 *
	 */
	static void handshakeConnect(struct ev_loop * loop, ev_io * watcher, int32_t) noexcept {
		// Получаем состояние прогона сценария
		handshake_t * state = reinterpret_cast <handshake_t *> (watcher->data);
		// Прекращаем наблюдение за завершением подключения
		::ev_io_stop(loop, watcher);
		/**
		 * Проверяем исход подключения
		 *
		 * @note Неблокирующее подключение сообщает о своём исходе через параметр
		 *       сокета, а не готовностью записи: готовность наступает и при отказе.
		 *       Движок AWH этот параметр запрашивает, и без такой же проверки стенд
		 *       засчитывал бы за состоявшееся подключение любое пробуждение
		 */
		int32_t code = 0;
		// Размер получаемого значения
		socklen_t length = sizeof(code);
		// Выполняем получение исхода подключения
		::getsockopt(state->fd, SOL_SOCKET, SO_ERROR, &code, &length);
		// Выполняем закрытие сокета текущего подключения
		::close(state->fd);
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
				::ev_break(loop, EVBREAK_ALL);
				// Завершаем прогон сценария
				return;
			}
		}
		// Выполняем подключение очередного цикла
		state->fd = connector(state->address);
		// Включаем немедленный обрыв соединения при закрытии сокета
		hardClose(state->fd);
		// Инициализируем наблюдатель завершения подключения
		ev_io_init(watcher, &::handshakeConnect, state->fd, EV_WRITE);
		// Устанавливаем состояние прогона сценария наблюдателю
		watcher->data = state;
		// Активируем наблюдатель завершения подключения
		::ev_io_start(loop, watcher);
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
		// Наблюдатель готовности слушающего сокета
		ev_io listen{};
		// Создаём цикл событий стенда
		struct ev_loop * loop = ::ev_loop_new(EVFLAG_AUTO);
		// Создаём слушающий сокет петлевого интерфейса
		const int32_t server = listener(state.address);
		// Инициализируем наблюдатель готовности слушающего сокета
		ev_io_init(&listen, &::handshakeAccept, server, EV_READ);
		// Активируем наблюдатель готовности слушающего сокета
		::ev_io_start(loop, &listen);
		// Выполняем подключение первого цикла
		state.fd = connector(state.address);
		// Включаем немедленный обрыв соединения при закрытии сокета
		hardClose(state.fd);
		// Инициализируем наблюдатель завершения подключения
		ev_io_init(&state.watcher, &::handshakeConnect, state.fd, EV_WRITE);
		// Устанавливаем состояние прогона сценария наблюдателю
		state.watcher.data = &state;
		// Активируем наблюдатель завершения подключения
		::ev_io_start(loop, &state.watcher);
		/**
		 * Запускаем цикл событий до выполнения требуемого количества циклов
		 */
		::ev_run(loop, 0);
		// Устанавливаем количество выполненных операций
		result.operations = state.done;
		// Устанавливаем затраченное время
		result.seconds = elapsed(state.start, state.finish);
		// Выполняем закрытие слушающего сокета
		::close(server);
		// Освобождаем цикл событий стенда
		::ev_loop_destroy(loop);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция обратного вызова дедлайна, который не должен сработать
	 *
	 */
	static void deadlineFired(struct ev_loop *, ev_timer *, int32_t) noexcept {}
	/**
	 * @brief Функция вычисления дедлайна таймера
	 *
	 * @param index порядковый номер таймера
	 * @return      дедлайн таймера в секундах
	 *
	 */
	static double deadline(const size_t index) noexcept {
		// Выводим дедлайн таймера, отнесённый далеко в будущее
		return (static_cast <double> (DEADLINE_OFFSET + (index % DEADLINE_SPREAD)) / 1000.0);
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
		vector <ev_timer> timers(count);
		// Создаём цикл событий стенда
		struct ev_loop * loop = ::ev_loop_new(EVFLAG_AUTO);
		// Реестр описателей наблюдателей по идентификатору
		registry_t <ev_timer *> registry;
		// Резервируем память под реестр описателей
		registry.reserve(count);
		/**
		 * Выполняем подготовку наблюдателей и наполнение реестра вне окна замера
		 */
		for(size_t i = 0; i < count; i++){
			// Инициализируем наблюдатель таймера
			ev_timer_init(&timers[i], &::deadlineFired, ::deadline(i), 0.0);
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
					::ev_timer_stop(loop, &timers[k]);
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
				::ev_timer_start(loop, i->second);
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
		::ev_loop_destroy(loop);
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
		vector <ev_timer> timers(count);
		// Создаём цикл событий стенда
		struct ev_loop * loop = ::ev_loop_new(EVFLAG_AUTO);
		// Реестр описателей наблюдателей по идентификатору
		registry_t <ev_timer *> registry;
		// Резервируем память под реестр описателей
		registry.reserve(count);
		/**
		 * Выполняем подготовку, постановку и наполнение реестра вне окна замера
		 */
		for(size_t i = 0; i < count; i++){
			// Инициализируем наблюдатель таймера
			ev_timer_init(&timers[i], &::deadlineFired, ::deadline(i), 0.0);
			// Ставим таймер в структуру дедлайнов
			::ev_timer_start(loop, &timers[i]);
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
				for(size_t k = 0; k < count; k++){
					// Возвращаем таймеру заданный ему дедлайн
					ev_timer_init(&timers[k], &::deadlineFired, ::deadline(k), ::deadline(k));
					// Ставим таймер в структуру дедлайнов
					::ev_timer_start(loop, &timers[k]);
				}
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
				::ev_timer_stop(loop, i->second);
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
		::ev_loop_destroy(loop);
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
		vector <ev_timer> timers(count);
		// Создаём цикл событий стенда
		struct ev_loop * loop = ::ev_loop_new(EVFLAG_AUTO);
		// Реестр описателей наблюдателей по идентификатору
		registry_t <ev_timer *> registry;
		// Резервируем память под реестр описателей
		registry.reserve(count);
		/**
		 * Выполняем подготовку, постановку и наполнение реестра вне окна замера
		 */
		for(size_t i = 0; i < count; i++){
			// Инициализируем наблюдатель таймера
			ev_timer_init(&timers[i], &::deadlineFired, ::deadline(i), ::deadline(i));
			// Ставим таймер в структуру дедлайнов
			::ev_timer_start(loop, &timers[i]);
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
				/**
				 * Сдвигаем дедлайн таймера вперёд
				 *
				 * @note Величина повторения задаётся перед каждым перевзведением.
				 *       Без этого `ev_timer_again` возвращал бы таймер на прежнее
				 *       место в куче, тогда как остальные стенды отодвигают дедлайн
				 *       за весь остаток набора и оплачивают полное просеивание
				 */
				i->second->repeat = (::deadline(i->first - 1) + (static_cast <double> ((pass + 1) * DEADLINE_SPREAD) / 1000.0));
				// Перевзводим таймер средствами самой библиотеки
				::ev_timer_again(loop, i->second);
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
		::ev_loop_destroy(loop);
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
		// Список наблюдателей таймеров
		vector <ev_timer> timers(count);
		// Создаём цикл событий стенда
		struct ev_loop * loop = ::ev_loop_new(EVFLAG_AUTO);
		/**
		 * Выполняем подготовку наблюдателей вне окна замера
		 */
		for(size_t i = 0; i < count; i++)
			// Инициализируем наблюдатель таймера
			ev_timer_init(&timers[i], &::deadlineFired, ::deadline(i), 0.0);
		// Запоминаем момент начала замера
		const auto start = now();
		/**
		 * Выполняем постановку всех таймеров
		 */
		for(size_t i = 0; i < count; i++)
			// Ставим таймер в структуру дедлайнов
			::ev_timer_start(loop, &timers[i]);
		// Запоминаем момент окончания замера
		const auto finish = now();
		// Устанавливаем количество выполненных операций
		result.operations = count;
		// Устанавливаем затраченное время
		result.seconds = elapsed(start, finish);
		// Освобождаем цикл событий стенда
		::ev_loop_destroy(loop);
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
		// Список наблюдателей таймеров
		vector <ev_timer> timers(count);
		// Создаём цикл событий стенда
		struct ev_loop * loop = ::ev_loop_new(EVFLAG_AUTO);
		/**
		 * Выполняем подготовку и постановку наблюдателей вне окна замера
		 */
		for(size_t i = 0; i < count; i++){
			// Инициализируем наблюдатель таймера
			ev_timer_init(&timers[i], &::deadlineFired, ::deadline(i), 0.0);
			// Ставим таймер в структуру дедлайнов
			::ev_timer_start(loop, &timers[i]);
		}
		// Запоминаем момент начала замера
		const auto start = now();
		/**
		 * Выполняем отмену всех таймеров
		 */
		for(size_t i = 0; i < count; i++)
			// Снимаем таймер со структуры дедлайнов
			::ev_timer_stop(loop, &timers[i]);
		// Запоминаем момент окончания замера
		const auto finish = now();
		// Устанавливаем количество выполненных операций
		result.operations = count;
		// Устанавливаем затраченное время
		result.seconds = elapsed(start, finish);
		// Освобождаем цикл событий стенда
		::ev_loop_destroy(loop);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария перевзведения таймеров в структуре дедлайнов
	 *
	 * @note Библиотека предоставляет для этого отдельную функцию `ev_timer_again`,
	 *       и именно она здесь используется: подгонять стенд под чужой интерфейс
	 *       значило бы мерить не то, чем пользуются
	 *
	 * @param count количество таймеров
	 * @return      итоги прогона сценария
	 *
	 */
	static outcome_t rearming(const size_t count) noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Список наблюдателей таймеров
		vector <ev_timer> timers(count);
		// Создаём цикл событий стенда
		struct ev_loop * loop = ::ev_loop_new(EVFLAG_AUTO);
		/**
		 * Выполняем подготовку и постановку наблюдателей вне окна замера
		 */
		for(size_t i = 0; i < count; i++){
			// Инициализируем наблюдатель таймера с повторением для перевзведения
			ev_timer_init(&timers[i], &::deadlineFired, ::deadline(i), ::deadline(i));
			// Ставим таймер в структуру дедлайнов
			::ev_timer_start(loop, &timers[i]);
		}
		// Запоминаем момент начала замера
		const auto start = now();
		/**
		 * Выполняем перевзведение всех таймеров
		 */
		for(size_t i = 0; i < count; i++){
			// Задаём величину повторения, отодвигающую дедлайн за остаток набора
			timers[i].repeat = (::deadline(i) + (static_cast <double> (DEADLINE_SPREAD) / 1000.0));
			// Перевзводим таймер средствами самой библиотеки
			::ev_timer_again(loop, &timers[i]);
		}
		// Запоминаем момент окончания замера
		const auto finish = now();
		// Устанавливаем количество выполненных операций
		result.operations = count;
		// Устанавливаем затраченное время
		result.seconds = elapsed(start, finish);
		// Освобождаем цикл событий стенда
		::ev_loop_destroy(loop);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция обратного вызова срабатывания таймера
	 *
	 * @param watcher наблюдатель таймера
	 *
	 */
	static void timerFired(struct ev_loop * loop, ev_timer * watcher, int32_t) noexcept {
		// Считаем сработавший таймер
		(*reinterpret_cast <size_t *> (watcher->data))++;
		/**
		 * Снимаем наблюдатель и освобождаем его здесь же, внутри окна замера.
		 *
		 * Одноразовый таймер движка AWH - это узел события, который при срабатывании
		 * уничтожается, и стоимость уничтожения попадает в окно замера. Чтобы
		 * сравнение оставалось сравнением, освобождение должно попадать в окно и у
		 * соперника, а не выноситься за его пределы
		 */
		::ev_timer_stop(loop, watcher);
		// Освобождаем наблюдатель таймера
		delete watcher;
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
		// Создаём цикл событий стенда
		struct ev_loop * loop = ::ev_loop_new(EVFLAG_AUTO);
		// Запоминаем момент начала замера
		const auto start = now();
		/**
		 * Выполняем постановку требуемого количества одноразовых таймеров.
		 *
		 * Наблюдатель выделяется здесь же, внутри окна замера, а не заготавливается
		 * вектором до его начала. Одноразовый таймер движка AWH - это узел события,
		 * который движок заводит при постановке и уничтожает при срабатывании, и обе
		 * стоимости попадают в окно. Заготовка всех наблюдателей заранее сняла бы с
		 * соперника стоимость выделения целиком и превратила бы сравнение стоимости
		 * обслуживания таймера в сравнение двух разных вещей
		 */
		for(size_t i = 0; i < TIMER_COUNT; i++){
			// Дедлайн срабатывания таймера с разбросом по диапазону
			const double deadline = (static_cast <double> (1 + (i % TIMER_SPREAD)) / 1000.0);
			// Выделяем наблюдатель таймера
			ev_timer * timer = new ev_timer;
			// Инициализируем наблюдатель таймера
			ev_timer_init(timer, &::timerFired, deadline, 0.0);
			// Устанавливаем счётчик сработавших таймеров наблюдателю
			timer->data = &fired;
			// Выполняем постановку таймера
			::ev_timer_start(loop, timer);
		}
		/**
		 * Запускаем цикл событий до срабатывания всех таймеров
		 */
		::ev_run(loop, 0);
		// Запоминаем момент окончания замера
		const auto finish = now();
		// Устанавливаем количество выполненных операций
		result.operations = fired;
		// Устанавливаем затраченное время
		result.seconds = elapsed(start, finish);
		// Освобождаем цикл событий стенда
		::ev_loop_destroy(loop);
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
	::printf("СТЕНД libev %d.%d\n\nСЦЕНАРИЙ                               ИЗМЕРЕНО\n", ::ev_version_major(), ::ev_version_minor());
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
