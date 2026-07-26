/**
 * @file: libev.cpp
 * @date: 2026-07-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Эталонный стенд сравнения на libev — те же сценарии нагрузки, что и у бенчмарков
 *        сетевого движка AWH, выполненные средствами наблюдателей готовности библиотеки
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
	static void handshakeAccept(struct ev_loop *, ev_io * watcher, int32_t) noexcept {
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
	 * @brief Функция обратного вызова срабатывания таймера
	 *
	 * @param watcher наблюдатель таймера
	 *
	 */
	static void timerFired(struct ev_loop *, ev_timer * watcher, int32_t) noexcept {
		// Считаем сработавший таймер
		(*reinterpret_cast <size_t *> (watcher->data))++;
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
		// Список наблюдателей таймеров
		vector <ev_timer> timers(TIMER_COUNT);
		// Создаём цикл событий стенда
		struct ev_loop * loop = ::ev_loop_new(EVFLAG_AUTO);
		// Запоминаем момент начала замера
		const auto start = now();
		/**
		 * Выполняем постановку требуемого количества одноразовых таймеров
		 */
		for(size_t i = 0; i < TIMER_COUNT; i++){
			// Дедлайн срабатывания таймера с разбросом по диапазону
			const double deadline = (static_cast <double> (1 + (i % TIMER_SPREAD)) / 1000.0);
			// Инициализируем наблюдатель таймера
			ev_timer_init(&timers[i], &::timerFired, deadline, 0.0);
			// Устанавливаем счётчик сработавших таймеров наблюдателю
			timers[i].data = &fired;
			// Выполняем постановку таймера
			::ev_timer_start(loop, &timers[i]);
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
