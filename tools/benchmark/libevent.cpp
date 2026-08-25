/**
 * @file libevent.cpp
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
 * @brief Эталонный стенд сравнения на libevent2 — те же сценарии нагрузки, что и у бенчмарков
 *        сетевого движка AWH, выполненные средствами низкоуровневого интерфейса библиотеки
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
 * Подключаем заголовочные файлы библиотеки libevent2
 */
#include <event2/event.h>

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
 * @details Библиотека используется на самом низком уровне - через наблюдатели
 *          готовности `event`, без надстройки `bufferevent`: она добавляет
 *          собственную буферизацию, которой нет у libev, и сравнение перестало
 *          бы быть сравнением циклов событий
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
		// Дескриптор сокета подключения
		int32_t fd;
		// Количество принятых октетов текущего сообщения
		size_t received;
		// Наблюдатель готовности подключения
		struct event * watcher;
		// Цикл событий стенда
		struct event_base * base;
		// Состояние прогона сценария
		echo_t * state;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Connection() noexcept :
		 fd(-1), received(0), watcher(nullptr), base(nullptr), state(nullptr) {}
	} connection_t;

	/**
	 * @brief Структура контекста слушающего сокета сценария обмена
	 *
	 */
	typedef struct Acceptor {
		// Цикл событий стенда
		struct event_base * base;
		// Список состояний принятых подключений
		vector <unique_ptr <connection_t>> * accepted;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Acceptor() noexcept : base(nullptr), accepted(nullptr) {}
	} acceptor_t;

	/**
	 * @brief Структура состояния прогона сценария пропускной способности
	 *
	 */
	typedef struct Stream {
		// Цикл событий стенда
		struct event_base * base;
		// Наблюдатель готовности передатчика
		struct event * writer;
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
		 base(nullptr), writer(nullptr), pending(0), queued(0), received(0) {}
	} stream_t;

	/**
	 * @brief Структура состояния прогона сценария установления соединений
	 *
	 */
	typedef struct Handshake {
		// Цикл событий стенда
		struct event_base * base;
		// Параметры подключения к слушающему сокету
		struct sockaddr_in address;
		// Наблюдатель завершения текущего подключения
		struct event * watcher;
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
		 base(nullptr), address{}, watcher(nullptr), fd(-1),
		 measuring(false), warmed(0), done(0) {}
	} handshake_t;

	/**
	 * @brief Функция обратного вызова готовности чтения принятого подключения
	 *
	 * @param fd дескриптор сокета подключения
	 *
	 */
	static void peerRead(evutil_socket_t fd, short, void *) noexcept {
		// Выполняем чтение принятых данных
		const ssize_t bytes = ::recv(fd, gBuffer.data(), gBuffer.size(), 0);
		// Если данные приняты
		if(bytes > 0)
			// Возвращаем принятые данные отправителю
			::send(fd, gBuffer.data(), static_cast <size_t> (bytes), 0);
	}
	/**
	 * @brief Функция обратного вызова готовности чтения клиентского подключения
	 *
	 * @param fd  дескриптор сокета подключения
	 * @param arg состояние подключения
	 *
	 */
	static void clientRead(evutil_socket_t fd, short, void * arg) noexcept {
		// Получаем состояние подключения
		connection_t * connection = reinterpret_cast <connection_t *> (arg);
		// Выполняем чтение принятых данных
		const ssize_t bytes = ::recv(fd, gBuffer.data(), ECHO_PAYLOAD, 0);
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
			::send(fd, gPayload, ECHO_PAYLOAD, 0);
		// Останавливаем цикл событий
		else ::event_base_loopbreak(connection->base);
	}
	/**
	 * @brief Функция обратного вызова завершения клиентского подключения
	 *
	 * @param fd  дескриптор сокета подключения
	 * @param arg состояние подключения
	 *
	 */
	static void clientConnect(evutil_socket_t fd, short, void * arg) noexcept {
		// Получаем состояние подключения
		connection_t * connection = reinterpret_cast <connection_t *> (arg);
		// Освобождаем наблюдатель завершения подключения
		::event_free(connection->watcher);
		// Создаём наблюдатель готовности чтения подключения
		connection->watcher = ::event_new(connection->base, fd, EV_READ | EV_PERSIST, &::clientRead, connection);
		// Активируем наблюдатель готовности чтения
		::event_add(connection->watcher, nullptr);
		// Отправляем первое сообщение обмена
		::send(fd, gPayload, ECHO_PAYLOAD, 0);
	}
	/**
	 * @brief Функция обратного вызова принятия входящего подключения сценария обмена
	 *
	 * @param fd  дескриптор слушающего сокета
	 * @param arg контекст слушающего сокета
	 *
	 */
	static void echoAccept(evutil_socket_t fd, short, void * arg) noexcept {
		// Получаем контекст слушающего сокета
		acceptor_t * context = reinterpret_cast <acceptor_t *> (arg);
		/**
		 * Выполняем приём всех ожидающих подключений
		 */
		while(true){
			// Выполняем приём входящего подключения
			const int32_t peer = ::accept(fd, nullptr, nullptr);
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
			// Устанавливаем цикл событий стенда
			connection->base = context->base;
			// Создаём наблюдатель готовности чтения принятого подключения
			connection->watcher = ::event_new(context->base, peer, EV_READ | EV_PERSIST, &::peerRead, connection);
			// Активируем наблюдатель готовности чтения
			::event_add(connection->watcher, nullptr);
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
		struct event_base * base = ::event_base_new();
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
		// Устанавливаем цикл событий стенда контексту слушающего сокета
		context.base = base;
		// Устанавливаем список состояний принятых подключений
		context.accepted = &accepted;
		// Создаём наблюдатель готовности слушающего сокета
		struct event * listen = ::event_new(base, server, EV_READ | EV_PERSIST, &::echoAccept, &context);
		// Активируем наблюдатель готовности слушающего сокета
		::event_add(listen, nullptr);
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
			// Устанавливаем цикл событий стенда
			connection->base = base;
			// Создаём наблюдатель завершения подключения
			connection->watcher = ::event_new(base, connection->fd, EV_WRITE, &::clientConnect, connection);
			// Активируем наблюдатель завершения подключения
			::event_add(connection->watcher, nullptr);
		}
		/**
		 * Запускаем цикл событий до выполнения требуемого количества обменов
		 */
		::event_base_dispatch(base);
		// Устанавливаем количество выполненных операций
		result.operations = state.done;
		// Устанавливаем объём переданных данных с учётом обоих направлений обмена
		result.bytes = (state.done * ECHO_PAYLOAD * 2);
		// Устанавливаем затраченное время
		result.seconds = elapsed(state.start, state.finish);
		/**
		 * Выполняем освобождение принятых подключений
		 */
		for(auto & connection : accepted){
			// Освобождаем наблюдатель готовности подключения
			::event_free(connection->watcher);
			// Выполняем закрытие сокета подключения
			::close(connection->fd);
		}
		/**
		 * Выполняем освобождение клиентских подключений
		 */
		for(auto & connection : clients){
			// Освобождаем наблюдатель готовности подключения
			::event_free(connection->watcher);
			// Выполняем закрытие сокета подключения
			::close(connection->fd);
		}
		// Освобождаем наблюдатель готовности слушающего сокета
		::event_free(listen);
		// Выполняем закрытие слушающего сокета
		::close(server);
		// Освобождаем цикл событий стенда
		::event_base_free(base);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Структура состояния датаграммного сокета
	 *
	 */
	typedef struct Datagram {
		// Дескриптор датаграммного сокета
		int32_t fd;
		// Наблюдатель готовности сокета
		struct event * watcher;
		// Цикл событий стенда
		struct event_base * base;
		// Состояние прогона сценария
		echo_t * state;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Datagram() noexcept :
		 fd(-1), watcher(nullptr), base(nullptr), state(nullptr) {}
	} datagram_t;

	// Признак того, что датаграммный прогон упёрся в предельный срок
	static bool gExpired = false;

	/**
	 * @brief Функция обратного вызова готовности чтения приёмника датаграмм
	 *
	 * @details Приёмник обслуживает ВСЕХ отправителей одним сокетом, поэтому
	 *          адрес отправителя извлекается у каждой датаграммы и ответ уходит
	 *          по нему же. Накопления принятых октетов здесь нет намеренно:
	 *          датаграмма приходит целиком, и частично принятого сообщения
	 *          не бывает
	 *
	 * @param fd дескриптор приёмника датаграмм
	 *
	 */
	static void receiverRead(evutil_socket_t fd, short, void *) noexcept {
		// Адрес отправителя датаграммы
		struct sockaddr_in source{};
		// Размер структуры адреса отправителя
		socklen_t length = sizeof(source);
		/**
		 * Выполняем приём всех ожидающих датаграмм
		 */
		while(true){
			// Восстанавливаем размер структуры адреса отправителя
			length = sizeof(source);
			// Выполняем приём датаграммы
			const ssize_t bytes = ::recvfrom(fd, gBuffer.data(), ECHO_PAYLOAD, 0, reinterpret_cast <struct sockaddr *> (&source), &length);
			// Если ожидающих датаграмм не осталось
			if(bytes <= 0)
				// Завершаем приём датаграмм
				return;
			// Возвращаем принятые октеты отправителю
			::sendto(fd, gBuffer.data(), static_cast <size_t> (bytes), 0, reinterpret_cast <const struct sockaddr *> (&source), length);
		}
	}
	/**
	 * @brief Функция обратного вызова готовности чтения отправителя датаграмм
	 *
	 * @param fd  дескриптор отправителя датаграмм
	 * @param arg состояние отправителя
	 *
	 */
	static void emitterRead(evutil_socket_t fd, short, void * arg) noexcept {
		// Получаем состояние отправителя
		datagram_t * datagram = reinterpret_cast <datagram_t *> (arg);
		// Выполняем приём датаграммы
		const ssize_t bytes = ::recv(fd, gBuffer.data(), ECHO_PAYLOAD, 0);
		// Если датаграмма не принята
		if(bytes <= 0)
			// Ожидаем следующей готовности сокета
			return;
		// Если обмен следует продолжать
		if(datagram->state->account())
			// Отправляем следующую датаграмму
			::send(fd, gPayload, ECHO_PAYLOAD, 0);
		// Останавливаем цикл событий
		else ::event_base_loopbreak(datagram->base);
	}
	/**
	 * @brief Функция обратного вызова истечения предельного срока прогона
	 *
	 * @param arg цикл событий стенда
	 *
	 */
	static void expire(evutil_socket_t, short, void * arg) noexcept {
		// Отмечаем прогон упёршимся в предельный срок
		gExpired = true;
		// Останавливаем цикл событий
		::event_base_loopbreak(reinterpret_cast <struct event_base *> (arg));
	}
	/**
	 * @brief Функция прогона сценария датаграммного обмена
	 *
	 * @details Постановка повторяет сценарий обмена короткими сообщениями во всём,
	 *          кроме переноса: та же нагрузка, то же количество обменов, тот же
	 *          учёт границ прогрева и замера. Только так разница между итогами
	 *          выражает цену переноса, а не разницу постановок
	 *
	 * @note Отправителей много, а приёмник ОДИН - у датаграмм принятого сокета
	 *       на каждого отправителя не возникает. Этим датаграммный сценарий на
	 *       множестве отличается от потокового, и разницу их итогов следует
	 *       читать с оглядкой на это, а не как отставание переноса
	 *
	 * @warning Предельный срок здесь не украшение. Переспроса у датаграмм нет:
	 *          единственная потерянная датаграмма оставляет отправителя ждать
	 *          ответа, которого не будет, и прогон встал бы навсегда
	 *
	 * @param senders количество одновременных отправителей
	 * @param rounds  требуемое количество обменов замера
	 * @return        итоги прогона сценария
	 *
	 */
	static outcome_t transmit(const size_t senders, const size_t rounds) noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Сбрасываем признак истечения предельного срока
		gExpired = false;
		// Состояние прогона сценария
		echo_t state(senders, rounds);
		// Создаём цикл событий стенда
		struct event_base * base = ::event_base_new();
		// Параметры привязки приёмника датаграмм
		struct sockaddr_in address{};
		// Создаём приёмник датаграмм петлевого интерфейса
		const int32_t server = receiver(address);
		// Создаём наблюдатель готовности приёмника датаграмм
		struct event * listen = ::event_new(base, server, EV_READ | EV_PERSIST, &::receiverRead, nullptr);
		// Активируем наблюдатель готовности приёмника датаграмм
		::event_add(listen, nullptr);
		// Список состояний отправителей
		vector <unique_ptr <datagram_t>> clients;
		// Резервируем память под состояния отправителей
		clients.reserve(senders);
		/**
		 * Выполняем создание требуемого количества отправителей
		 */
		for(size_t i = 0; i < senders; i++){
			// Создаём состояние отправителя
			clients.push_back(unique_ptr <datagram_t> (new datagram_t));
			// Получаем состояние созданного отправителя
			datagram_t * datagram = clients.back().get();
			// Выполняем создание отправителя датаграмм
			datagram->fd = emitter(address);
			// Устанавливаем состояние прогона сценария
			datagram->state = &state;
			// Устанавливаем цикл событий стенда
			datagram->base = base;
			// Создаём наблюдатель готовности чтения отправителя
			datagram->watcher = ::event_new(base, datagram->fd, EV_READ | EV_PERSIST, &::emitterRead, datagram);
			// Активируем наблюдатель готовности чтения отправителя
			::event_add(datagram->watcher, nullptr);
			// Отправляем первую датаграмму, начиная обмен
			::send(datagram->fd, gPayload, ECHO_PAYLOAD, 0);
		}
		// Создаём наблюдатель предельного срока прогона
		struct event * deadline = ::event_new(base, -1, 0, &::expire, base);
		// Срок ожидания завершения прогона
		struct timeval limit{};
		// Устанавливаем срок ожидания завершения прогона в секундах
		limit.tv_sec = DATAGRAM_DEADLINE;
		// Активируем наблюдатель предельного срока прогона
		::event_add(deadline, &limit);
		/**
		 * Запускаем цикл событий до выполнения требуемого количества обменов
		 */
		::event_base_dispatch(base);
		// Если прогон уложился в предельный срок
		if(!gExpired){
			// Устанавливаем количество выполненных операций
			result.operations = state.done;
			// Устанавливаем объём переданных данных с учётом обоих направлений обмена
			result.bytes = (state.done * ECHO_PAYLOAD * 2);
			// Устанавливаем затраченное время
			result.seconds = elapsed(state.start, state.finish);
		}
		/**
		 * Выполняем освобождение отправителей датаграмм
		 */
		for(auto & datagram : clients){
			// Освобождаем наблюдатель готовности отправителя
			::event_free(datagram->watcher);
			// Выполняем закрытие сокета отправителя
			::close(datagram->fd);
		}
		// Освобождаем наблюдатель предельного срока прогона
		::event_free(deadline);
		// Освобождаем наблюдатель готовности приёмника датаграмм
		::event_free(listen);
		// Выполняем закрытие приёмника датаграмм
		::close(server);
		// Освобождаем цикл событий стенда
		::event_base_free(base);
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
	 * @param fd  дескриптор сокета подключения
	 * @param arg состояние подключения
	 *
	 */
	static void idleConnect(evutil_socket_t fd, short, void * arg) noexcept {
		// Получаем состояние подключения
		connection_t * connection = reinterpret_cast <connection_t *> (arg);
		// Освобождаем наблюдатель завершения подключения
		::event_free(connection->watcher);
		// Создаём наблюдатель готовности чтения подключения
		connection->watcher = ::event_new(connection->base, fd, EV_READ | EV_PERSIST, &::clientRead, connection);
		// Активируем наблюдатель готовности чтения
		::event_add(connection->watcher, nullptr);
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
		struct event_base * base = ::event_base_new();
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
		// Устанавливаем цикл событий стенда контексту слушающего сокета
		context.base = base;
		// Устанавливаем список состояний принятых подключений
		context.accepted = &accepted;
		// Создаём наблюдатель готовности слушающего сокета
		struct event * listen = ::event_new(base, server, EV_READ | EV_PERSIST, &::echoAccept, &context);
		// Активируем наблюдатель готовности слушающего сокета
		::event_add(listen, nullptr);
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
			// Устанавливаем цикл событий стенда
			connection->base = base;
			// Создаём наблюдатель завершения подключения
			connection->watcher = ::event_new(base, connection->fd, EV_WRITE, &::idleConnect, connection);
			// Активируем наблюдатель завершения подключения
			::event_add(connection->watcher, nullptr);
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
				::event_base_loop(base, EVLOOP_NONBLOCK);
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
		::event_base_dispatch(base);
		// Устанавливаем количество выполненных операций
		result.operations = state.done;
		// Устанавливаем объём переданных данных с учётом обоих направлений обмена
		result.bytes = (state.done * ECHO_PAYLOAD * 2);
		// Устанавливаем затраченное время
		result.seconds = elapsed(state.start, state.finish);
		/**
		 * Выполняем освобождение принятых подключений
		 */
		for(auto & connection : accepted){
			// Освобождаем наблюдатель готовности подключения
			::event_free(connection->watcher);
			// Выполняем закрытие сокета подключения
			::close(connection->fd);
		}
		/**
		 * Выполняем освобождение клиентских подключений
		 */
		for(auto & connection : clients){
			// Освобождаем наблюдатель готовности подключения
			::event_free(connection->watcher);
			// Выполняем закрытие сокета подключения
			::close(connection->fd);
		}
		// Освобождаем наблюдатель готовности слушающего сокета
		::event_free(listen);
		// Выполняем закрытие слушающего сокета
		::close(server);
		// Освобождаем цикл событий стенда
		::event_base_free(base);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция обратного вызова готовности чтения приёмника потока
	 *
	 * @param fd  дескриптор сокета приёмника
	 * @param arg состояние прогона сценария
	 *
	 */
	static void streamRead(evutil_socket_t fd, short, void * arg) noexcept {
		// Получаем состояние прогона сценария
		stream_t * state = reinterpret_cast <stream_t *> (arg);
		// Выполняем чтение принятых данных
		const ssize_t bytes = ::recv(fd, gBuffer.data(), gBuffer.size(), 0);
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
			::event_base_loopbreak(state->base);
		}
	}
	/**
	 * @brief Функция обратного вызова готовности записи передатчика потока
	 *
	 * @param fd  дескриптор сокета передатчика
	 * @param arg состояние прогона сценария
	 *
	 */
	static void streamWrite(evutil_socket_t fd, short, void * arg) noexcept {
		// Получаем состояние прогона сценария
		stream_t * state = reinterpret_cast <stream_t *> (arg);
		// Выполняем запись остатка текущего блока
		const ssize_t bytes = ::send(fd, gChunk.data() + (STREAM_CHUNK - state->pending), state->pending, 0);
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
			::event_del(state->writer);
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
		state.base = ::event_base_new();
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
		// Создаём наблюдатель готовности чтения приёмника
		struct event * reader = ::event_new(state.base, peer, EV_READ | EV_PERSIST, &::streamRead, &state);
		// Активируем наблюдатель готовности чтения приёмника
		::event_add(reader, nullptr);
		// Создаём наблюдатель готовности записи передатчика
		state.writer = ::event_new(state.base, client, EV_WRITE | EV_PERSIST, &::streamWrite, &state);
		// Активируем наблюдатель готовности записи передатчика
		::event_add(state.writer, nullptr);
		// Устанавливаем количество октетов первого блока
		state.pending = STREAM_CHUNK;
		// Считаем поставленный в очередь объём
		state.queued = STREAM_CHUNK;
		// Запоминаем момент начала замера
		state.start = now();
		/**
		 * Запускаем цикл событий до передачи всего объёма
		 */
		::event_base_dispatch(state.base);
		// Устанавливаем количество выполненных операций
		result.operations = (state.received / STREAM_CHUNK);
		// Устанавливаем объём переданных данных
		result.bytes = state.received;
		// Устанавливаем затраченное время
		result.seconds = elapsed(state.start, state.finish);
		// Освобождаем наблюдатель готовности чтения приёмника
		::event_free(reader);
		// Освобождаем наблюдатель готовности записи передатчика
		::event_free(state.writer);
		// Выполняем закрытие сокета приёмника
		::close(peer);
		// Выполняем закрытие сокета передатчика
		::close(client);
		// Выполняем закрытие слушающего сокета
		::close(server);
		// Освобождаем цикл событий стенда
		::event_base_free(state.base);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция обратного вызова принятия входящего подключения сценария соединений
	 *
	 * @param fd дескриптор слушающего сокета
	 *
	 */
	static void handshakeAccept(evutil_socket_t fd, short, void * arg) noexcept {
		/**
		 * Выполняем приём всех ожидающих подключений
		 */
		while(true){
			// Выполняем приём входящего подключения
			const int32_t peer = ::accept(fd, nullptr, nullptr);
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
			struct event * peerWatcher = ::event_new(reinterpret_cast <struct event_base *> (arg), peer, EV_READ, &::handshakeAccept, arg);
			// Активируем наблюдатель готовности принятого подключения
			::event_add(peerWatcher, nullptr);
			// Освобождаем наблюдатель готовности принятого подключения
			::event_free(peerWatcher);
			// Выполняем закрытие принятого подключения
			::close(peer);
		}
	}
	/**
	 * @brief Функция обратного вызова завершения подключения
	 *
	 * @param arg состояние прогона сценария
	 *
	 */
	static void handshakeConnect(evutil_socket_t, short, void * arg) noexcept {
		// Получаем состояние прогона сценария
		handshake_t * state = reinterpret_cast <handshake_t *> (arg);
		// Освобождаем наблюдатель завершения текущего подключения
		::event_free(state->watcher);
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
				::event_base_loopbreak(state->base);
				// Завершаем прогон сценария
				return;
			}
		}
		// Выполняем подключение очередного цикла
		state->fd = connector(state->address);
		// Включаем немедленный обрыв соединения при закрытии сокета
		hardClose(state->fd);
		// Создаём наблюдатель завершения подключения
		state->watcher = ::event_new(state->base, state->fd, EV_WRITE, &::handshakeConnect, state);
		// Активируем наблюдатель завершения подключения
		::event_add(state->watcher, nullptr);
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
		// Создаём цикл событий стенда
		state.base = ::event_base_new();
		// Создаём слушающий сокет петлевого интерфейса
		const int32_t server = listener(state.address);
		// Создаём наблюдатель готовности слушающего сокета
		struct event * listen = ::event_new(state.base, server, EV_READ | EV_PERSIST, &::handshakeAccept, state.base);
		// Активируем наблюдатель готовности слушающего сокета
		::event_add(listen, nullptr);
		// Выполняем подключение первого цикла
		state.fd = connector(state.address);
		// Включаем немедленный обрыв соединения при закрытии сокета
		hardClose(state.fd);
		// Создаём наблюдатель завершения подключения
		state.watcher = ::event_new(state.base, state.fd, EV_WRITE, &::handshakeConnect, &state);
		// Активируем наблюдатель завершения подключения
		::event_add(state.watcher, nullptr);
		/**
		 * Запускаем цикл событий до выполнения требуемого количества циклов
		 */
		::event_base_dispatch(state.base);
		// Устанавливаем количество выполненных операций
		result.operations = state.done;
		// Устанавливаем затраченное время
		result.seconds = elapsed(state.start, state.finish);
		// Освобождаем наблюдатель готовности слушающего сокета
		::event_free(listen);
		// Выполняем закрытие слушающего сокета
		::close(server);
		// Освобождаем цикл событий стенда
		::event_base_free(state.base);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция обратного вызова дедлайна, который не должен сработать
	 *
	 */
	static void deadlineFired(evutil_socket_t, short, void *) noexcept {}
	/**
	 * @brief Функция вычисления дедлайна таймера
	 *
	 * @param index порядковый номер таймера
	 * @return      дедлайн таймера
	 *
	 */
	static struct timeval deadline(const size_t index, const uint32_t shift = 0) noexcept {
		// Дедлайн таймера в миллисекундах, отнесённый далеко в будущее
		const uint32_t milliseconds = (DEADLINE_OFFSET + static_cast <uint32_t> (index % DEADLINE_SPREAD) + shift);
		// Выводим дедлайн таймера
		return timeval{static_cast <time_t> (milliseconds / 1000), static_cast <suseconds_t> ((milliseconds % 1000) * 1000)};
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
		vector <struct event *> timers(count, nullptr);
		// Создаём цикл событий стенда
		struct event_base * base = ::event_base_new();
		// Реестр описателей наблюдателей по идентификатору
		registry_t <struct event *> registry;
		// Резервируем память под реестр описателей
		registry.reserve(count);
		/**
		 * Выполняем подготовку наблюдателей и наполнение реестра вне окна замера
		 */
		for(size_t i = 0; i < count; i++){
			// Инициализируем наблюдатель таймера
			timers[i] = ::evtimer_new(base, &::deadlineFired, nullptr);
			// Записываем описатель наблюдателя в реестр
			registry.emplace(static_cast <uint32_t> (i + 1), timers[i]);
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
					::evtimer_del(timers[k]);
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
				struct timeval tv = ::deadline(i->first - 1);
					::evtimer_add(i->second, &tv);
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
		for(size_t i = 0; i < count; i++)
			::event_free(timers[i]);
		::event_base_free(base);
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
		vector <struct event *> timers(count, nullptr);
		// Создаём цикл событий стенда
		struct event_base * base = ::event_base_new();
		// Реестр описателей наблюдателей по идентификатору
		registry_t <struct event *> registry;
		// Резервируем память под реестр описателей
		registry.reserve(count);
		/**
		 * Выполняем подготовку, постановку и наполнение реестра вне окна замера
		 */
		for(size_t i = 0; i < count; i++){
			// Инициализируем наблюдатель таймера
			timers[i] = ::evtimer_new(base, &::deadlineFired, nullptr);
			// Ставим таймер в структуру дедлайнов
			struct timeval tv = ::deadline(i);
			::evtimer_add(timers[i], &tv);
			// Записываем описатель наблюдателя в реестр
			registry.emplace(static_cast <uint32_t> (i + 1), timers[i]);
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
					// Ставим таймер в структуру дедлайнов
					struct timeval tv = ::deadline(k);
					::evtimer_add(timers[k], &tv);
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
				::evtimer_del(i->second);
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
		for(size_t i = 0; i < count; i++)
			::event_free(timers[i]);
		::event_base_free(base);
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
		vector <struct event *> timers(count, nullptr);
		// Создаём цикл событий стенда
		struct event_base * base = ::event_base_new();
		// Реестр описателей наблюдателей по идентификатору
		registry_t <struct event *> registry;
		// Резервируем память под реестр описателей
		registry.reserve(count);
		/**
		 * Выполняем подготовку, постановку и наполнение реестра вне окна замера
		 */
		for(size_t i = 0; i < count; i++){
			// Инициализируем наблюдатель таймера
			timers[i] = ::evtimer_new(base, &::deadlineFired, nullptr);
			// Ставим таймер в структуру дедлайнов
			struct timeval tv = ::deadline(i);
			::evtimer_add(timers[i], &tv);
			// Записываем описатель наблюдателя в реестр
			registry.emplace(static_cast <uint32_t> (i + 1), timers[i]);
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
				struct timeval tv = ::deadline(i->first - 1, static_cast <uint32_t> (pass + 1) * DEADLINE_SPREAD);
					::evtimer_add(i->second, &tv);
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
		for(size_t i = 0; i < count; i++)
			::event_free(timers[i]);
		::event_base_free(base);
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
		vector <struct event *> timers(count, nullptr);
		// Создаём цикл событий стенда
		struct event_base * base = ::event_base_new();
		/**
		 * Выполняем подготовку наблюдателей вне окна замера
		 */
		for(size_t i = 0; i < count; i++)
			// Создаём наблюдатель таймера
			timers[i] = ::evtimer_new(base, &::deadlineFired, nullptr);
		// Запоминаем момент начала замера
		const auto start = now();
		/**
		 * Выполняем постановку всех таймеров
		 */
		for(size_t i = 0; i < count; i++){
			// Получаем дедлайн таймера
			struct timeval tv = ::deadline(i);
			// Ставим таймер в структуру дедлайнов
			::evtimer_add(timers[i], &tv);
		}
		// Запоминаем момент окончания замера
		const auto finish = now();
		// Устанавливаем количество выполненных операций
		result.operations = count;
		// Устанавливаем затраченное время
		result.seconds = elapsed(start, finish);
		/**
		 * Освобождаем наблюдатели таймеров
		 */
		for(size_t i = 0; i < count; i++)
			// Освобождаем наблюдатель таймера
			::event_free(timers[i]);
		// Освобождаем цикл событий стенда
		::event_base_free(base);
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
		vector <struct event *> timers(count, nullptr);
		// Создаём цикл событий стенда
		struct event_base * base = ::event_base_new();
		/**
		 * Выполняем подготовку и постановку наблюдателей вне окна замера
		 */
		for(size_t i = 0; i < count; i++){
			// Создаём наблюдатель таймера
			timers[i] = ::evtimer_new(base, &::deadlineFired, nullptr);
			// Получаем дедлайн таймера
			struct timeval tv = ::deadline(i);
			// Ставим таймер в структуру дедлайнов
			::evtimer_add(timers[i], &tv);
		}
		// Запоминаем момент начала замера
		const auto start = now();
		/**
		 * Выполняем отмену всех таймеров
		 */
		for(size_t i = 0; i < count; i++)
			// Снимаем таймер со структуры дедлайнов
			::evtimer_del(timers[i]);
		// Запоминаем момент окончания замера
		const auto finish = now();
		// Устанавливаем количество выполненных операций
		result.operations = count;
		// Устанавливаем затраченное время
		result.seconds = elapsed(start, finish);
		/**
		 * Освобождаем наблюдатели таймеров
		 */
		for(size_t i = 0; i < count; i++)
			// Освобождаем наблюдатель таймера
			::event_free(timers[i]);
		// Освобождаем цикл событий стенда
		::event_base_free(base);
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
		// Список наблюдателей таймеров
		vector <struct event *> timers(count, nullptr);
		// Создаём цикл событий стенда
		struct event_base * base = ::event_base_new();
		/**
		 * Выполняем подготовку и постановку наблюдателей вне окна замера
		 */
		for(size_t i = 0; i < count; i++){
			// Создаём наблюдатель таймера
			timers[i] = ::evtimer_new(base, &::deadlineFired, nullptr);
			// Получаем дедлайн таймера
			struct timeval tv = ::deadline(i);
			// Ставим таймер в структуру дедлайнов
			::evtimer_add(timers[i], &tv);
		}
		// Запоминаем момент начала замера
		const auto start = now();
		/**
		 * Выполняем перевзведение всех таймеров
		 */
		for(size_t i = 0; i < count; i++){
			// Получаем сдвинутый вперёд дедлайн таймера
			struct timeval tv = ::deadline(i, DEADLINE_SPREAD);
			// Сдвигаем дедлайн таймера вперёд
			::evtimer_add(timers[i], &tv);
		}
		// Запоминаем момент окончания замера
		const auto finish = now();
		// Устанавливаем количество выполненных операций
		result.operations = count;
		// Устанавливаем затраченное время
		result.seconds = elapsed(start, finish);
		/**
		 * Освобождаем наблюдатели таймеров
		 */
		for(size_t i = 0; i < count; i++)
			// Освобождаем наблюдатель таймера
			::event_free(timers[i]);
		// Освобождаем цикл событий стенда
		::event_base_free(base);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Состояние одноразового таймера сценария обслуживания таймеров
	 *
	 * @details Событие libevent требуется освободить из его же функции обратного
	 *          вызова, а функция получает только один нетипизированный указатель.
	 *          Поэтому счётчик срабатываний и само событие передаются вместе
	 *
	 */
	typedef struct Timer_State {
		// Событие одноразового таймера
		struct event * event;
		// Счётчик сработавших таймеров
		size_t * fired;
	} timer_state_t;

	/**
	 * @brief Функция обратного вызова срабатывания таймера
	 *
	 * @param arg состояние сработавшего таймера
	 *
	 */
	static void timerFired(evutil_socket_t, short, void * arg) noexcept {
		// Получаем состояние сработавшего таймера
		timer_state_t * state = reinterpret_cast <timer_state_t *> (arg);
		// Считаем сработавший таймер
		(*state->fired)++;
		/**
		 * Освобождаем событие здесь же, внутри окна замера.
		 *
		 * Одноразовый таймер движка AWH - это узел события, который при срабатывании
		 * уничтожается, и стоимость уничтожения попадает в окно замера. Прежде
		 * `event_free` вызывался после закрытия окна, и соперник за освобождение не
		 * платил вовсе. Освобождать событие из его собственной функции обратного
		 * вызова libevent позволяет: событие одноразовое и к этому моменту уже снято
		 */
		::event_free(state->event);
		// Освобождаем состояние таймера
		delete state;
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
		struct event_base * base = ::event_base_new();
		// Запоминаем момент начала замера
		const auto start = now();
		/**
		 * Выполняем постановку требуемого количества одноразовых таймеров.
		 *
		 * Событие и его состояние выделяются внутри окна замера, а освобождаются в
		 * функции обратного вызова - тоже внутри окна. Прежде освобождение
		 * выполнялось после закрытия окна, и стоимость его в замер не попадала,
		 * тогда как у движка AWH одноразовый узел уничтожается при срабатывании и
		 * потому за освобождение внутри окна платит
		 */
		for(size_t i = 0; i < TIMER_COUNT; i++){
			// Дедлайн срабатывания таймера
			struct timeval deadline{};
			// Устанавливаем дедлайн таймера с разбросом по диапазону
			deadline.tv_usec = static_cast <suseconds_t> ((1 + (i % TIMER_SPREAD)) * 1000);
			// Выделяем состояние одноразового таймера
			timer_state_t * state = new timer_state_t;
			// Устанавливаем счётчик сработавших таймеров
			state->fired = &fired;
			// Создаём наблюдатель таймера
			state->event = ::evtimer_new(base, &::timerFired, state);
			// Выполняем постановку таймера
			::evtimer_add(state->event, &deadline);
		}
		/**
		 * Запускаем цикл событий до срабатывания всех таймеров
		 */
		::event_base_dispatch(base);
		// Запоминаем момент окончания замера
		const auto finish = now();
		// Устанавливаем количество выполненных операций
		result.operations = fired;
		// Устанавливаем затраченное время
		result.seconds = elapsed(start, finish);
		/**
		 * Освобождать наблюдатели здесь больше не требуется: каждый из них
		 * освобождается в своей функции обратного вызова, внутри окна замера
		 */
		// Освобождаем цикл событий стенда
		::event_base_free(base);
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
	::printf("СТЕНД libevent %s\n\nСЦЕНАРИЙ                               ИЗМЕРЕНО\n", ::event_get_version());
	// Если сценарий обмена на одном подключении выполняется
	if(selected("net/io/echo/single", name)){
		// Выполняем прогон сценария обмена на одном подключении
		const outcome_t outcome = ::exchange(1, ECHO_SINGLE_ROUNDS);
		// Выводим результат прогона сценария
		report("net/io/echo/single", "обменов/с", perSecond(outcome), outcome);
	}
	// Если сценарий датаграммного обмена одним отправителем выполняется
	if(selected("net/io/datagram/single", name)){
		// Выполняем прогон сценария датаграммного обмена одним отправителем
		const outcome_t outcome = ::transmit(1, ECHO_SINGLE_ROUNDS);
		// Выводим результат прогона сценария
		report("net/io/datagram/single", "обменов/с", perSecond(outcome), outcome);
	}
	// Если сценарий датаграммного обмена множеством отправителей выполняется
	if(selected("net/io/datagram/multi", name)){
		// Выполняем прогон сценария датаграммного обмена множеством отправителей
		const outcome_t outcome = ::transmit(ECHO_MULTI_CONNECTIONS, ECHO_MULTI_ROUNDS);
		// Выводим результат прогона сценария
		report("net/io/datagram/multi", "обменов/с", perSecond(outcome), outcome);
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
