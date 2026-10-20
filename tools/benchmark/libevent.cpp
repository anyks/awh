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
		socket_t fd;
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
		// Наблюдатель записи текущего блока
		struct event * writer;
		// Признак открытого окна передачи
		bool running;
		// Признак полного завершения передачи
		bool stop;
		// Причина отказа
		const char * failure;
		// Остаток единственного незавершённого блока
		size_t pending;
		// Объём поставленных в очередь блоков
		size_t queued;
		// Объём подтверждённой записи
		size_t written;
		// Объём принятых октетов
		size_t received;
		// Начало измерения передачи
		std::chrono::steady_clock::time_point start;
		// Конец измерения передачи
		std::chrono::steady_clock::time_point finish;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Stream() noexcept :
		 writer(nullptr),
		 running(false), stop(false), failure(nullptr),
		 pending(0), queued(0), written(0), received(0), start{}, finish{} {}
	} stream_t;

	/**
	 * @brief Структура состояния прогона сценария установления соединений
	 *
	 */
	typedef struct Handshake {
		// Цикл событий сценария
		struct event_base * base;
		// Адрес слушающего сокета
		struct sockaddr_in address;
		// Подписка подтверждения клиента
		struct event * watcher;
		// Подписка чтения принятой стороны
		struct event * reader;
		// Сокет текущего клиента
		socket_t fd;
		// Сокет принятой стороны
		socket_t peer;
		// Признак подтверждения подключения клиента
		bool ready;
		// Причина прекращения сценария
		const char * failure;
		// Количество успешных подключений клиента
		size_t connected;
		// Количество подготовленных серверных сторон
		size_t accepted;
		// Количество завершённых закрытий сторон
		size_t closed;
		// Количество полных прогревочных циклов
		size_t warmed;
		// Количество полных измеренных циклов
		size_t done;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Handshake() noexcept :
		 base(nullptr), address{}, watcher(nullptr), reader(nullptr), fd(INVALID), peer(INVALID),
		 ready(false), failure(nullptr), connected(0), accepted(0), closed(0), warmed(0), done(0) {}
	} handshake_t;

	/**
	 * @brief Функция обратного вызова готовности чтения принятого подключения
	 *
	 * @param fd дескриптор сокета подключения
	 *
	 */
	static void peerRead(evutil_socket_t fd, short, void *) noexcept {
		// Выполняем чтение принятых данных
		const ssize_t bytes = ::recv(fd, raw(gBuffer.data()), gBuffer.size(), 0);
		// Если данные приняты
		if(bytes > 0)
			// Возвращаем принятые данные отправителю
			::send(fd, raw(gBuffer.data()), static_cast <size_t> (bytes), 0);
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
		const ssize_t bytes = ::recv(fd, raw(gBuffer.data()), ECHO_PAYLOAD, 0);
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
			::send(fd, raw(gPayload), ECHO_PAYLOAD, 0);
		// Останавливаем цикл событий
		else ::event_base_loopbreak(connection->base);
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
			const socket_t peer = ::accept(fd, nullptr, nullptr);
			// Если ожидающих подключений не осталось
			if(broken(peer))
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
	 * @brief Состояние полного состава TCP-прогона
	 *
	 */
	struct Echo_State;
	/**
	 * @brief Состояние подключения с отдельной очередью отправки
	 *
	 */
	typedef struct Echo_Peer {
		// Дескриптор подключения
		socket_t fd;
		// Общее состояние прогона
		Echo_State * state;
		// Наблюдатель чтения
		struct event * reader;
		// Наблюдатель неполной отправки
		struct event * writer;
		// Наблюдатель завершения подключения
		struct event * connecting;
		// Признак клиентской стороны
		bool client;
		// Признак активного наблюдателя отправки
		bool writing;
		// Признак отправки запроса внутри измеряемого окна
		bool measured;
		// Число принятых октетов текущего ответа
		size_t received;
		// Число индивидуальных обменов прогрева
		size_t warmed;
		// Число неотправленных октетов
		size_t queued;
		// Смещение неотправленного хвоста
		size_t offset;
		// Буфер текущего чтения
		char input[ECHO_PAYLOAD];
		// Буфер, сохраняющий данные до полной отправки
		char output[ECHO_PAYLOAD];
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Echo_Peer() noexcept :
		 fd(INVALID), state(nullptr), reader(nullptr), writer(nullptr), connecting(nullptr), client(false), writing(false), measured(false), received(0), warmed(0), queued(0), offset(0), input{}, output{} {}
	} echo_peer_t;
	/**
	 * @brief Структура TCP-прогона с двумя фазами прогрева и точным окном
	 *
	 */
	typedef struct Echo_State {
		// Цикл событий прогона
		struct event_base * base;
		// Признак остановки прогона
		bool stop;
		// Признак открытого измеряемого окна
		bool measuring;
		// Признак общего прогрева после индивидуального барьера
		bool fleet;
		// Требуемое количество подключений
		size_t connections;
		// Количество участников, завершивших текущий прогрев
		size_t ready;
		// Количество принятых сервером подключений
		size_t accepted;
		// Требуемое количество измеряемых обменов
		size_t rounds;
		// Количество завершённых измеряемых обменов
		size_t done;
		// Причина отказа прогона
		const char * failure;
		// Момент открытия окна
		chrono::steady_clock::time_point start;
		// Единственная отметка конца окна
		chrono::steady_clock::time_point finish;
		// Состояния клиентских подключений
		vector <unique_ptr <echo_peer_t>> clients;
		// Состояния серверных подключений
		vector <unique_ptr <echo_peer_t>> peers;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Echo_State() noexcept :
		 base(nullptr), stop(false), measuring(false), fleet(false), connections(0), ready(0), accepted(0), rounds(0), done(0), failure(nullptr) {}
	} echo_state_t;
	/**
	 * @brief Функция остановки недействительного TCP-прогона
	 *
	 * @param state  состояние прогона
	 * @param reason причина отказа
	 *
	 */
	static void echoFail(echo_state_t & state, const char * reason) noexcept {
		// Не изменяем уже закрытое окно
		if(state.stop)
			// Оставляем первый итог прогона
			return;
		// Сохраняем причину отказа
		state.failure = reason;
		// Запрещаем дальнейшую работу обработчиков
		state.stop = true;
		// Если цикл создан
		if(state.base != nullptr)
			// Прерываем ожидание остальных участников
			::event_base_loopbreak(state.base);
	}
	/**
	 * @brief Функция открытия окна после прогрева полного состава
	 *
	 * @param state состояние прогона
	 * @return      признак открытия окна
	 *
	 */
	static bool echoBegin(echo_state_t & state) noexcept {
		// Не допускаем неполный состав, повторный запуск и нулевую нагрузку
		if(state.stop || state.measuring || !state.fleet || (state.connections == 0) || (state.rounds == 0) || (state.ready != state.connections) || (state.accepted != state.connections))
			// Сохраняем окно закрытым
			return false;
		// Открываем измеряемое окно
		state.measuring = true;
		// Запоминаем момент начала замера
		state.start = chrono::steady_clock::now();
		// Сообщаем об открытии окна
		return true;
	}
	/**
	 * @brief Функция учёта полного ответа конкретному клиенту
	 *
	 * @param state состояние прогона
	 * @param peer  состояние ответившего клиента
	 * @return      признак необходимости следующего запроса
	 *
	 */
	static bool echoAccount(echo_state_t & state, echo_peer_t & peer) noexcept {
		// Хвост текущей пачки не должен менять закрытое окно
		if(state.stop)
			// Прекращаем обмен
			return false;
		// Если измерение ещё не началось
		if(!state.measuring){
			// Готовый клиент не выполняет прогрев за остальных
			if(peer.warmed >= 8)
				// Сохраняем непрерывный поток только во второй фазе
				return state.fleet;
			// Учитываем ответ именно этого клиента
			peer.warmed++;
			// Если индивидуальный прогрев ещё не завершён
			if(peer.warmed < 8)
				// Продолжаем прогрев клиента
				return true;
			// Учитываем готового участника ровно один раз
			state.ready++;
			// Первый барьер останавливает клиента, второй сохраняет поток
			return state.fleet;
		}
		// Не учитываем ответ на запрос, отправленный до начала окна
		if(!peer.measured)
			// Продолжаем обмен уже измеряемым запросом
			return true;
		// Учитываем завершённый обмен замера
		state.done++;
		// Если целевое количество ещё не достигнуто
		if(state.done < state.rounds)
			// Продолжаем измерение
			return true;
		// Фиксируем конец окна единственный раз
		state.finish = chrono::steady_clock::now();
		// Запрещаем учёт и отправки в оставшихся callback
		state.stop = true;
		// Прекращаем обмен
		return false;
	}
	/**
	 * @brief Функция проверки временного отказа неблокирующего сокета
	 *
	 * @param error код отказа
	 * @return      признак необходимости повторить обращение
	 *
	 */
	static bool echoBlocked(const int32_t error) noexcept {
		/**
		 * Проверяем код соответствующего интерфейса сокетов
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Проверяем временную недоступность Winsock
			return (error == WSAEWOULDBLOCK);
		/**
		 * Для интерфейса сокетов POSIX
		 */
		#else
			// Проверяем оба обозначения временной недоступности POSIX
			return ((error == EAGAIN) || (error == EWOULDBLOCK));
		#endif
	}
	/**
	 * @brief Функция проверки прерывания обращения к сокету
	 *
	 * @param error код отказа
	 * @return      признак прерванного обращения
	 *
	 */
	static bool echoInterrupted(const int32_t error) noexcept {
		/**
		 * Проверяем код соответствующего интерфейса сокетов
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Проверяем прерывание Winsock
			return (error == WSAEINTR);
		/**
		 * Для интерфейса сокетов POSIX
		 */
		#else
			// Проверяем прерывание POSIX
			return (error == EINTR);
		#endif
	}
	/**
	 * @brief Функция отправки сохранённого хвоста
	 *
	 * @param peer состояние отправителя
	 *
	 */
	static void echoFlush(echo_peer_t & peer) noexcept {
		/**
		 * Отправляем доступную часть очереди до временного отказа
		 */
		while((peer.queued > 0) && !peer.state->stop){
			// Передаём системе только неотправленный хвост
			const ssize_t bytes = ::send(peer.fd, peer.output + peer.offset, static_cast <int32_t> (peer.queued), 0);
			// Если обращение отвергнуто
			if(bytes < 0){
				// Сохраняем код до следующего системного обращения
				const int32_t error = EVUTIL_SOCKET_ERROR();
				// Если обращение прервано
				if(::echoInterrupted(error))
					// Повторяем отправку без потери октетов
					continue;
				// Если необходимо дождаться готовности
				if(::echoBlocked(error)){
					// Если наблюдатель ещё не активен
					if(!peer.writing){
						// Проверяем регистрацию ожидания записи
						if(::event_add(peer.writer, nullptr) != 0){
							// Не оставляем хвост без источника пробуждения
							::echoFail(* peer.state, "write-watch-failed");
							// Прекращаем отправку
							return;
						}
						// Запоминаем активное ожидание записи
						peer.writing = true;
					}
					// Сохраняем очередь до следующей готовности
					return;
				}
				// Отклоняем постоянный отказ отправки
				::echoFail(* peer.state, "send-failed");
				// Не считаем потерянные октеты отправленными
				return;
			}
			// Если отправка не продвинулась или вернула неверный размер
			if((bytes == 0) || (static_cast <size_t> (bytes) > peer.queued)){
				// Отклоняем недействительный результат отправки
				::echoFail(* peer.state, "send-size-invalid");
				// Не оставляем бесконечный цикл без прогресса
				return;
			}
			// Сдвигаем начало сохранённого хвоста
			peer.offset += static_cast <size_t> (bytes);
			// Уменьшаем число оставшихся октетов
			peer.queued -= static_cast <size_t> (bytes);
		}
		// Если очередь полностью отправлена
		if(peer.queued == 0){
			// Освобождаем весь буфер следующему сообщению
			peer.offset = 0;
			// Если ожидание записи было заведено
			if(peer.writing){
				// Проверяем снятие ненужного наблюдателя
				if(::event_del(peer.writer) != 0)
					// Не допускаем холостое вращение наблюдателя
					::echoFail(* peer.state, "write-unwatch-failed");
				// Снимаем признак ожидания записи
				peer.writing = false;
			}
		}
	}
	/**
	 * @brief Функция сохранения данных до полной отправки
	 *
	 * @param peer состояние отправителя
	 * @param data передаваемые октеты
	 * @param size количество октетов
	 *
	 */
	static void echoWrite(echo_peer_t & peer, const char * data, const size_t size) noexcept {
		// Не запускаем обмен за границей прогона
		if(peer.state->stop)
			// Оставляем состояние общей очистке
			return;
		// Один запрос в полёте ограничивает очередь одним сообщением
		if(size > (ECHO_PAYLOAD - peer.queued)){
			// Отклоняем нарушение границ буфера
			::echoFail(* peer.state, "write-queue-overflow");
			// Не перезаписываем хвост предыдущей отправки
			return;
		}
		// Если хвост находится не в начале буфера
		if((peer.offset > 0) && (peer.queued > 0))
			// Освобождаем непрерывное место под новые фрагменты
			::memmove(peer.output, peer.output + peer.offset, peer.queued);
		// Возвращаем начало очереди к началу буфера
		peer.offset = 0;
		// Сохраняем данные отдельно от буфера чтения
		::memcpy(peer.output + peer.queued, data, size);
		// Учитываем добавленные октеты
		peer.queued += size;
		// Пробуем передать очередь без лишнего ожидания записи
		::echoFlush(peer);
	}
	/**
	 * @brief Функция обработки готовности неполной отправки
	 *
	 * @param arg состояние подключения
	 *
	 */
	static void echoWritable(evutil_socket_t, short, void * arg) noexcept {
		// Продолжаем отправку сохранённого хвоста
		::echoFlush(* reinterpret_cast <echo_peer_t *> (arg));
	}
	/**
	 * @brief Функция обработки чтения TCP-обмена
	 *
	 * @param fd  дескриптор подключения
	 * @param arg состояние подключения
	 *
	 */
	static void echoRead(evutil_socket_t fd, short, void * arg) noexcept {
		// Получаем состояние подключения
		echo_peer_t & peer = * reinterpret_cast <echo_peer_t *> (arg);
		// Получаем состояние всего прогона
		echo_state_t & state = * peer.state;
		// Не разбираем оставшиеся события закрытого окна
		if(state.stop)
			// Оставляем подключение очистке
			return;
		// Получаем текущую часть сообщения
		const ssize_t bytes = ::recv(fd, peer.input, ECHO_PAYLOAD, 0);
		// Если чтение отвергнуто
		if(bytes < 0){
			// Сохраняем код отказа до следующих вызовов
			const int32_t error = EVUTIL_SOCKET_ERROR();
			// Если отказ временный или обращение прервано
			if(::echoBlocked(error) || ::echoInterrupted(error))
				// Ожидаем следующего события чтения
				return;
			// Отклоняем постоянный отказ чтения
			::echoFail(state, "recv-failed");
			// Не учитываем неполное сообщение
			return;
		}
		// Если встречная сторона закрылась до завершения нагрузки
		if(bytes == 0){
			// Отклоняем незавершённый обмен
			::echoFail(state, "unexpected-eof");
			// Не ждём бесконечно закрытое подключение
			return;
		}
		// Если читается серверная сторона
		if(!peer.client){
			// Возвращаем именно принятые октеты
			::echoWrite(peer, peer.input, static_cast <size_t> (bytes));
			// Сервер не учитывает клиентские обмены
			return;
		}
		// Накапливаем полный ответ клиента
		peer.received += static_cast <size_t> (bytes);
		// Если ответ ещё не получен полностью
		if(peer.received < ECHO_PAYLOAD)
			// Ожидаем оставшиеся октеты
			return;
		// Если один запрос дал неверное количество октетов
		if(peer.received != ECHO_PAYLOAD){
			// Не выдаём лишние октеты за очередной обмен
			::echoFail(state, "response-size-invalid");
			// Прекращаем разбор недействительного сообщения
			return;
		}
		// Освобождаем счётчик для следующего ответа
		peer.received = 0;
		// Учитываем полный ответ в соответствующей фазе
		const bool more = ::echoAccount(state, peer);
		// Если весь состав завершил общий прогрев
		if(state.fleet && !state.measuring && (state.ready == state.connections)){
			// Проверяем состав и открываем окно без массового перезапуска
			if(!::echoBegin(state))
				// Не измеряем неполный состав
				::echoFail(state, "warmup-incomplete");
		}
		// Если следующий запрос разрешён
		if(more && !state.stop){
			// Помечаем фазу именно нового запроса
			peer.measured = state.measuring;
			// Продолжаем обмен одним запросом в полёте
			::echoWrite(peer, reinterpret_cast <const char *> (gPayload), ECHO_PAYLOAD);
		}
		// Если достигнут первый барьер или завершён прогон
		if(state.stop || (!state.fleet && (state.ready == state.connections)))
			// Возвращаем управление подготовке следующей фазы или очистке
			::event_base_loopbreak(state.base);
	}
	/**
	 * @brief Функция подготовки наблюдателей и сокета TCP-участника
	 *
	 * @param peer состояние подключения
	 * @return     признак успешной подготовки
	 *
	 */
	static bool echoSetup(echo_peer_t & peer) noexcept {
		// Устанавливаем общий для сетевых стендов набор опций
		::options(peer.fd);
		// Значение включённого режима TCP_NODELAY
		const int32_t enabled = 1;
		// Проверяем обязательные параметры неблокирующего обмена
		if((::evutil_make_socket_nonblocking(peer.fd) != 0) || (::setsockopt(peer.fd, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast <const char *> (&enabled), sizeof(enabled)) != 0))
			// Не запускаем участника с неподходящими параметрами
			return false;
		// Создаём постоянный наблюдатель чтения
		peer.reader = ::event_new(peer.state->base, peer.fd, EV_READ | EV_PERSIST, &::echoRead, &peer);
		// Создаём наблюдатель записи, активируемый только при временном отказе
		peer.writer = ::event_new(peer.state->base, peer.fd, EV_WRITE | EV_PERSIST, &::echoWritable, &peer);
		// Проверяем создание обоих наблюдателей
		return ((peer.reader != nullptr) && (peer.writer != nullptr));
	}
	/**
	 * @brief Функция завершения подключения клиента
	 *
	 * @param fd  дескриптор клиента
	 * @param arg состояние клиента
	 *
	 */
	static void echoConnected(evutil_socket_t fd, short, void * arg) noexcept {
		// Получаем состояние клиента
		echo_peer_t & peer = * reinterpret_cast <echo_peer_t *> (arg);
		// Не запускаем клиента после остановки прогона
		if(peer.state->stop)
			// Оставляем состояние очистке
			return;
		// Код результата неблокирующего подключения
		int32_t error = 0;
		// Размер результата проверки подключения
		socklen_t length = sizeof(error);
		// Проверяем действительное завершение подключения
		if((::getsockopt(fd, SOL_SOCKET, SO_ERROR, reinterpret_cast <char *> (&error), &length) != 0) || (error != 0)){
			// Не выдаём готовность записи за успешное подключение
			::echoFail(* peer.state, "connect-failed");
			// Не запускаем обмен неготового клиента
			return;
		}
		// Освобождаем одноразовый наблюдатель подключения
		::event_free(peer.connecting);
		// Исключаем повторное освобождение при очистке
		peer.connecting = nullptr;
		// Проверяем активацию чтения клиента
		if(::event_add(peer.reader, nullptr) != 0){
			// Не отправляем запрос без обработчика ответа
			::echoFail(* peer.state, "client-read-watch-failed");
			// Прекращаем подготовку клиента
			return;
		}
		// Запускаем индивидуальный прогрев клиента
		::echoWrite(peer, reinterpret_cast <const char *> (gPayload), ECHO_PAYLOAD);
	}
	/**
	 * @brief Функция приёма участников TCP-прогона
	 *
	 * @param fd  дескриптор слушателя
	 * @param arg состояние прогона
	 *
	 */
	static void echoAccepted(evutil_socket_t fd, short, void * arg) noexcept {
		// Получаем состояние прогона
		echo_state_t & state = * reinterpret_cast <echo_state_t *> (arg);
		/**
		 * Принимаем готовые подключения до исчерпания очереди
		 */
		while(!state.stop){
			// Получаем очередное серверное подключение
			const socket_t socket = ::accept(fd, nullptr, nullptr);
			// Если подключение не принято
			if(::broken(socket)){
				// Получаем код отказа до следующих вызовов
				const int32_t error = EVUTIL_SOCKET_ERROR();
				// Если обращение было прервано
				if(::echoInterrupted(error))
					// Повторяем приём
					continue;
				// Если очередь ещё не пуста, а отказ постоянный
				if(!::echoBlocked(error))
					// Отклоняем неполный состав
					::echoFail(state, "accept-failed");
				// Ожидаем следующей готовности слушателя либо очистки
				return;
			}
			// Если подключился участник вне ожидаемого состава
			if(state.accepted >= state.connections){
				// Закрываем лишний дескриптор
				::shut(socket);
				// Не подменяем состав прогона
				::echoFail(state, "accept-count-invalid");
				// Прекращаем приём
				return;
			}
			// Сохраняем состояние до общей очистки
			state.peers.push_back(unique_ptr <echo_peer_t> (new echo_peer_t));
			// Получаем добавленное серверное подключение
			echo_peer_t & peer = * state.peers.back();
			// Устанавливаем дескриптор серверной стороны
			peer.fd = socket;
			// Устанавливаем общее состояние прогона
			peer.state = &state;
			// Проверяем настройку сокета и начало чтения
			if(!::echoSetup(peer) || (::event_add(peer.reader, nullptr) != 0)){
				// Не учитываем неподготовленное подключение
				::echoFail(state, "peer-setup-failed");
				// Прекращаем приём
				return;
			}
			// Учитываем полностью готовую серверную сторону
			state.accepted++;
		}
	}
	/**
	 * @brief Функция ограничения подготовки и обмена общим сроком
	 *
	 * @param arg состояние прогона
	 *
	 */
	static void echoExpired(evutil_socket_t, short, void * arg) noexcept {
		// Отклоняем незавершённый прогрев или обмен
		::echoFail(* reinterpret_cast <echo_state_t *> (arg), "deadline-expired");
	}
	/**
	 * @brief Функция освобождения наблюдателей и сокета участника
	 *
	 * @param peer состояние подключения
	 *
	 */
	static void echoClose(echo_peer_t & peer) noexcept {
		// Если наблюдатель подключения создан
		if(peer.connecting != nullptr)
			// Снимаем ожидающее завершение подключения
			::event_free(peer.connecting);
		// Если наблюдатель чтения создан
		if(peer.reader != nullptr)
			// Снимаем наблюдатель до закрытия сокета
			::event_free(peer.reader);
		// Если наблюдатель отправки создан
		if(peer.writer != nullptr)
			// Снимаем оставшееся ожидание записи
			::event_free(peer.writer);
		// Если сокет создан
		if(!::broken(peer.fd))
			// Закрываем дескриптор после снятия всех наблюдателей
			::shut(peer.fd);
	}
	/**
	 * @brief Функция прогона TCP-обмена с тем же прогревом, что и у AWH
	 *
	 * @param connections количество одновременных подключений
	 * @param rounds      требуемое количество обменов замера
	 * @return            итоги прогона или нулевой результат при отказе
	 *
	 */
	static outcome_t echoExchange(const size_t connections, const size_t rounds) noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Состояние текущего прогона
		echo_state_t state;
		// Проверяем ненулевой состав и нагрузку
		if((connections == 0) || (rounds == 0)){
			// Сообщаем об отказе до создания дескрипторов
			::fprintf(stderr, "ECHO_BENCH_FAIL: parameters-invalid\n");
			// Возвращаем недействительный результат
			return result;
		}
		// Создаём цикл событий
		state.base = ::event_base_new();
		// Проверяем создание цикла
		if(state.base == nullptr){
			// Сообщаем об отказе до запуска нагрузки
			::fprintf(stderr, "ECHO_BENCH_FAIL: loop-create-failed\n");
			// Возвращаем недействительный результат
			return result;
		}
		// Указываем фактически выбранный механизм вне окна измерения
		::printf("ECHO_BACKEND: %s\n", ::event_base_get_method(state.base));
		// Устанавливаем полный состав участников
		state.connections = connections;
		// Устанавливаем точное число измеряемых обменов
		state.rounds = rounds;
		// Резервируем состояния клиентов
		state.clients.reserve(connections);
		// Резервируем состояния серверных подключений
		state.peers.reserve(connections);
		// Адрес слушателя с автоматически выбранным портом
		struct sockaddr_in address{};
		// Создаём слушающий дескриптор
		const socket_t server = ::listener(address);
		// Создаём наблюдатель приёма подключений
		struct event * listen = (::broken(server) ? nullptr : ::event_new(state.base, server, EV_READ | EV_PERSIST, &::echoAccepted, &state));
		// Создаём таймер общего предельного срока
		struct event * deadline = ::event_new(state.base, -1, 0, &::echoExpired, &state);
		// Ограничиваем подготовку и измерение одной минутой
		const struct timeval limit = {60, 0};
		// Проверяем готовность слушателя и таймера
		if((listen == nullptr) || (deadline == nullptr) || (::evutil_make_socket_nonblocking(server) != 0) || (::event_add(listen, nullptr) != 0) || (::event_add(deadline, &limit) != 0))
			// Не запускаем нагрузку без полного окружения
			::echoFail(state, "listener-or-deadline-failed");
		/**
		 * Создаём клиентов, обслуживая приём между подключениями
		 */
		for(size_t i = 0; (i < connections) && !state.stop; i++){
			// Сохраняем состояние клиента до общей очистки
			state.clients.push_back(unique_ptr <echo_peer_t> (new echo_peer_t));
			// Получаем добавленного клиента
			echo_peer_t & peer = * state.clients.back();
			// Подключаем общее состояние прогона
			peer.state = &state;
			// Отмечаем клиентскую сторону
			peer.client = true;
			// Создаём клиентский сокет
			peer.fd = ::socket(AF_INET, SOCK_STREAM, 0);
			// Проверяем создание сокета и наблюдателей
			if(::broken(peer.fd) || !::echoSetup(peer)){
				// Отклоняем неполный состав
				::echoFail(state, "client-setup-failed");
				// Не используем неготовое подключение
				break;
			}
			// Создаём одноразовый наблюдатель подключения
			peer.connecting = ::event_new(state.base, peer.fd, EV_WRITE, &::echoConnected, &peer);
			// Проверяем ожидание завершения подключения
			if((peer.connecting == nullptr) || (::event_add(peer.connecting, nullptr) != 0)){
				// Отклоняем отсутствие обработчика подключения
				::echoFail(state, "connect-watch-failed");
				// Не запускаем неподконтрольное подключение
				break;
			}
			// Запускаем неблокирующее подключение
			if(::connect(peer.fd, reinterpret_cast <const struct sockaddr *> (&address), sizeof(address)) != 0){
				// Сохраняем код результата подключения
				const int32_t error = EVUTIL_SOCKET_ERROR();
				/**
				 * Определяем допустимые коды незавершённого подключения
				 */
				#if defined(_WIN32) || defined(_WIN64)
					// Если подключение отклонено окончательно
					if((error != WSAEWOULDBLOCK) && (error != WSAEINPROGRESS))
						// Не ждём готовность после постоянного отказа
						::echoFail(state, "connect-submit-failed");
				/**
				 * Для интерфейса сокетов POSIX
				 */
				#else
					// Если подключение не находится в работе
					if((error != EINPROGRESS) && (error != EINTR))
						// Не подменяем постоянный отказ ожиданием
						::echoFail(state, "connect-submit-failed");
				#endif
			}
			// Если подключение начато
			if(!state.stop && (::event_base_loop(state.base, EVLOOP_NONBLOCK) < 0))
				// Отклоняем отказ цикла во время создания состава
				::echoFail(state, "prepare-loop-failed");
		}
		// Если индивидуальный барьер ещё не достигнут
		if(!state.stop && (state.ready < connections) && (::event_base_dispatch(state.base) < 0))
			// Отклоняем отказ ожидания прогрева
			::echoFail(state, "warmup-loop-failed");
		// Проверяем полный состав перед общим прогревом
		if(!state.stop && (state.ready == connections) && (state.accepted == connections)){
			// Переходим к непрерывному общему прогреву
			state.fleet = true;
			// Считаем готовность участников второй фазы отдельно
			state.ready = 0;
			/**
			 * Сбрасываем счётчики всех клиентов перед первой общей отправкой
			 */
			for(const auto & peer : state.clients)
				// Каждый клиент должен снова выполнить восемь обменов
				peer->warmed = 0;
			/**
			 * Запускаем нагрузку без открытия измеряемого окна
			 */
			for(const auto & peer : state.clients)
				// Поддерживаем один запрос в полёте у каждого клиента
				::echoWrite(* peer, reinterpret_cast <const char *> (gPayload), ECHO_PAYLOAD);
			// Если начальные запросы отправлены
			if(!state.stop && (::event_base_dispatch(state.base) < 0))
				// Отклоняем отказ измеряемого цикла
				::echoFail(state, "exchange-loop-failed");
		// Если полный состав не прогрет
		} else
			// Не выдаём неполную нагрузку за измерение
			::echoFail(state, "warmup-incomplete");
		// Проверяем точное завершение окна
		if((state.failure == nullptr) && (!state.stop || !state.measuring || (state.done != rounds)))
			// Не публикуем преждевременный выход
			state.failure = "exchange-incomplete";
		// Запрещаем продолжение обмена при очистке
		state.stop = true;
		// Если наблюдатель слушателя создан
		if(listen != nullptr)
			// Снимаем наблюдение до закрытия слушателя
			::event_free(listen);
		// Если таймер создан
		if(deadline != nullptr)
			// Снимаем предельный срок прогона
			::event_free(deadline);
		/**
		 * Освобождаем клиентские наблюдатели и сокеты
		 */
		for(const auto & peer : state.clients)
			// Закрываем клиента после прекращения callback
			::echoClose(* peer);
		/**
		 * Освобождаем серверные наблюдатели и сокеты
		 */
		for(const auto & peer : state.peers)
			// Закрываем серверную сторону подключения
			::echoClose(* peer);
		// Если слушающий сокет создан
		if(!::broken(server))
			// Освобождаем слушающий дескриптор
			::shut(server);
		// Освобождаем цикл после снятия всех наблюдателей
		::event_base_free(state.base);
		// Если все проверки пройдены
		if(state.failure == nullptr){
			// Устанавливаем точное число завершённых обменов
			result.operations = state.done;
			// Устанавливаем объём данных в обоих направлениях
			result.bytes = (state.done * ECHO_PAYLOAD * 2);
			// Используем только сохранённое измеряемое окно
			result.seconds = elapsed(state.start, state.finish);
		// Если прогон недействителен
		} else
			// Сообщаем причину отказа за пределами окна
			::fprintf(stderr, "ECHO_BENCH_FAIL: %s\n", state.failure);
		// Выводим итоги прогона
		return result;
	}
	/**
	 * @brief Структура состояния датаграммного сокета
	 *
	 */
	typedef struct Datagram {
		// Дескриптор датаграммного сокета
		socket_t fd;
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
			const ssize_t bytes = ::recvfrom(fd, raw(gBuffer.data()), ECHO_PAYLOAD, 0, reinterpret_cast <struct sockaddr *> (&source), &length);
			// Если ожидающих датаграмм не осталось
			if(bytes <= 0)
				// Завершаем приём датаграмм
				return;
			// Возвращаем принятые октеты отправителю
			::sendto(fd, raw(gBuffer.data()), static_cast <size_t> (bytes), 0, reinterpret_cast <const struct sockaddr *> (&source), length);
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
		const ssize_t bytes = ::recv(fd, raw(gBuffer.data()), ECHO_PAYLOAD, 0);
		// Если датаграмма не принята
		if(bytes <= 0)
			// Ожидаем следующей готовности сокета
			return;
		// Если обмен следует продолжать
		if(datagram->state->account())
			// Отправляем следующую датаграмму
			::send(fd, raw(gPayload), ECHO_PAYLOAD, 0);
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
		const socket_t server = receiver(address);
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
			::send(datagram->fd, raw(gPayload), ECHO_PAYLOAD, 0);
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
			shut(datagram->fd);
		}
		// Освобождаем наблюдатель предельного срока прогона
		::event_free(deadline);
		// Освобождаем наблюдатель готовности приёмника датаграмм
		::event_free(listen);
		// Выполняем закрытие приёмника датаграмм
		shut(server);
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
		const socket_t server = listener(address);
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
			::send(clients[i]->fd, raw(gPayload), ECHO_PAYLOAD, 0);
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
			shut(connection->fd);
		}
		/**
		 * Выполняем освобождение клиентских подключений
		 */
		for(auto & connection : clients){
			// Освобождаем наблюдатель готовности подключения
			::event_free(connection->watcher);
			// Выполняем закрытие сокета подключения
			shut(connection->fd);
		}
		// Освобождаем наблюдатель готовности слушающего сокета
		::event_free(listen);
		// Выполняем закрытие слушающего сокета
		shut(server);
		// Освобождаем цикл событий стенда
		::event_base_free(base);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Предельное время подготовки или передачи в секундах
	 *
	 */
	static constexpr double STREAM_DEADLINE = 30.0;
	/**
	 * @brief Функция проверки опций общего TCP-подключения
	 *
	 */
	static bool handshakeOptions(const socket_t fd, const bool hard) noexcept;
	/**
	 * @brief Функция подготовки принятой стороны общего TCP-подключения
	 *
	 */
	static void handshakeAccept(evutil_socket_t fd, short, void * arg) noexcept;
	/**
	 * @brief Функция проверки исхода общего TCP-подключения
	 *
	 */
	static void handshakeConnect(evutil_socket_t fd, short, void * arg) noexcept;
	/**
	 * @brief Функция периодического возврата из ожидания
	 *
	 */
	static void handshakeTick(evutil_socket_t, short, void *) noexcept;
	/**
	 * @brief Функция очистки общего TCP-подключения
	 *
	 */
	static bool handshakeClose(handshake_t & state) noexcept;
	/**
	 * @brief Функция завершения полного потока
	 *
	 * @param state состояние потока
	 *
	 */
	static void streamComplete(stream_t & state) noexcept {
		// Останавливаем часы после полной записи и полного приёма
		if(!state.stop && (state.failure == nullptr) && (state.written == STREAM_VOLUME) && (state.received == STREAM_VOLUME) && (state.pending == 0)){
			// Запоминаем конец передачи
			state.finish = now();
			// Подтверждаем завершение обеих сторон
			state.stop = true;
		}
	}
	/**
	 * @brief Функция чтения потока с проверкой EOF и ошибок
	 *
	 * @param fd  сокет приёмника
	 * @param arg состояние потока
	 *
	 */
	static void streamRead(evutil_socket_t fd, short, void * arg) noexcept {
		// Получаем состояние потока
		stream_t & state = *reinterpret_cast <stream_t *> (arg);
		// Не продолжаем обработку после отказа
		if(state.failure != nullptr)
			// Завершаем обработку
			return;
		// Читаем фактически доступные данные
		const ssize_t bytes = ::recv(fd, raw(gBuffer.data()), gBuffer.size(), 0);
		// Отличаем временную недоступность от отказа
		if(bytes < 0){
			// Получаем причину непосредственно после recv
			const int32_t code = EVUTIL_SOCKET_ERROR();
			// Сохраняем только настоящий отказ
			if(!::echoBlocked(code) && !::echoInterrupted(code))
				// Прерываем незавершённый поток
				state.failure = "read-failed";
			// Ожидаем новую готовность либо возвращаем отказ
			return;
		}
		// Не выдаём EOF или лишние октеты за успешную передачу
		if(!state.running || (bytes == 0) || (static_cast <size_t> (bytes) > (STREAM_VOLUME - state.received))){
			// Запоминаем недействительный приём
			state.failure = "read-size-or-eof";
			// Завершаем обработку
			return;
		}
		// Учитываем фактический объём
		state.received += static_cast <size_t> (bytes);
		// Проверяем завершение обеих сторон
		::streamComplete(state);
	}
	/**
	 * @brief Функция записи потока с сохранением хвоста текущего блока
	 *
	 * @param fd  сокет передатчика
	 * @param arg состояние потока
	 *
	 */
	static void streamWrite(evutil_socket_t fd, short, void * arg) noexcept {
		// Получаем состояние потока
		stream_t & state = *reinterpret_cast <stream_t *> (arg);
		// Не отправляем новый объём после отказа или завершения
		if((state.failure != nullptr) || state.stop)
			// Завершаем обработку
			return;
		// Отправляем только сохранённый хвост текущего блока
		const ssize_t bytes = ::send(fd, raw(gChunk.data() + (STREAM_CHUNK - state.pending)), state.pending, 0);
		// Отличаем временную недоступность от отказа
		if(bytes < 0){
			// Получаем причину непосредственно после send
			const int32_t code = EVUTIL_SOCKET_ERROR();
			// Не теряем хвост при EAGAIN или EINTR
			if(!::echoBlocked(code) && !::echoInterrupted(code))
				// Прерываем настоящий отказ записи
				state.failure = "write-failed";
			// Ожидаем новую готовность либо возвращаем отказ
			return;
		}
		// Проверяем фактическое продвижение записи
		if((bytes == 0) || (static_cast <size_t> (bytes) > state.pending)){
			// Не допускаем бесконечное ожидание без прогресса
			state.failure = "write-size";
			// Завершаем обработку
			return;
		}
		// Сохраняем остаток блока
		state.pending -= static_cast <size_t> (bytes);
		// Учитываем подтверждённую запись
		state.written += static_cast <size_t> (bytes);
		// Не подаём следующий блок до записи текущего
		if(state.pending != 0)
			// Ожидаем готовности оставшегося хвоста
			return;
		// Подаём следующий блок при неполном объёме очереди
		if(state.queued < STREAM_VOLUME){
			// Устанавливаем остаток нового блока
			state.pending = STREAM_CHUNK;
			// Учитываем новый блок
			state.queued += STREAM_CHUNK;
		// Снимаем ненужное ожидание после последнего блока
		} else if(::event_del(state.writer) != 0)
			// Не скрываем отказ снятия наблюдения
			state.failure = "write-unwatch";
		// Проверяем завершение обеих сторон
		::streamComplete(state);
	}
	/**
	 * @brief Функция полной передачи после подготовки обеих сторон
	 *
	 * @return итоги полного потока либо пустой результат
	 *
	 */
	static outcome_t transfer() noexcept {
		// Итоги потока
		outcome_t result;
		// Состояние передачи
		stream_t state;
		// Используем проверенную подготовку и очистку TCP-сокетов
		handshake_t connection;
		// Создаём базу событий
		connection.base = ::event_base_new();
		// Создаём слушающий сокет
		const socket_t server = ::socket(AF_INET, SOCK_STREAM, 0);
		// Выбираем IPv4 loopback
		connection.address.sin_family = AF_INET;
		// Предоставляем системе выбор порта
		connection.address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		// Размер адреса для получения порта
		socklen_t length = sizeof(connection.address);
		// Проверяем все операции подготовки сервера
		if((connection.base == nullptr) || broken(server) || !::handshakeOptions(server, false) || (::bind(server, reinterpret_cast <struct sockaddr *> (&connection.address), sizeof(connection.address)) != 0) || (::getsockname(server, reinterpret_cast <struct sockaddr *> (&connection.address), &length) != 0) || (::listen(server, BACKLOG) != 0))
			// Запоминаем отказ подготовки
			connection.failure = "server-setup";
		// Наблюдатель приёма подключения
		struct event * listener = nullptr;
		// Периодический возврат для проверки срока
		struct event * timer = nullptr;
		// Готовим обе стороны только после успешной подготовки сервера
		if(connection.failure == nullptr){
			// Подписываемся на приём подключения
			listener = ::event_new(connection.base, server, EV_READ | EV_PERSIST, &::handshakeAccept, &connection);
			// Создаём общий сторож ожидания
			timer = ::event_new(connection.base, INVALID, EV_PERSIST, &::handshakeTick, nullptr);
			// Ограничиваем шаг ожидания четвертью секунды
			const struct timeval step{0, 250000};
			// Проверяем регистрацию наблюдателей
			if((listener == nullptr) || (timer == nullptr) || (::event_add(listener, nullptr) != 0) || (::event_add(timer, &step) != 0))
				// Запоминаем отказ регистрации
				connection.failure = "setup-registration";
			// Создаём неблокирующий клиент
			connection.fd = ::socket(AF_INET, SOCK_STREAM, 0);
			// Проверяем опции клиента
			if(broken(connection.fd) || !::handshakeOptions(connection.fd, true))
				// Запоминаем отказ подготовки
				connection.failure = "client-setup";
		}
		// Запускаем подключение после полной подготовки
		if(connection.failure == nullptr){
			// Проверяем немедленный исход connect
			if(::connect(connection.fd, reinterpret_cast <const struct sockaddr *> (&connection.address), sizeof(connection.address)) != 0){
				// Получаем причину незавершённого подключения
				const int32_t code = EVUTIL_SOCKET_ERROR();
				/**
				 * Для Winsock допустим только неблокирующий исход
				 */
				#if defined(_WIN32) || defined(_WIN64)
					// Отбрасываем немедленный отказ
					if(code != WSAEWOULDBLOCK)
						// Запоминаем ошибку запуска
						connection.failure = "connect-start";
				/**
				 * Для интерфейса POSIX допустим незавершённый connect
				 */
				#else
					// Отбрасываем немедленный отказ
					if(code != EINPROGRESS)
						// Запоминаем ошибку запуска
						connection.failure = "connect-start";
				#endif
			}
			// Создаём подтверждение SO_ERROR
			connection.watcher = ::event_new(connection.base, connection.fd, EV_WRITE, &::handshakeConnect, &connection);
			// Проверяем регистрацию подтверждения
			if((connection.watcher == nullptr) || (::event_add(connection.watcher, nullptr) != 0))
				// Запоминаем отказ регистрации
				connection.failure = "connect-registration";
		}
		// Запоминаем начало ожидания подготовки
		const auto prepared = now();
		/**
		 * Дожидаемся успешного connect и подготовленного приёмника
		 */
		while((connection.failure == nullptr) && (!connection.ready || (connection.accepted != 1))){
			// Продвигаем подготовку конечным опросом
			if(::event_base_loop(connection.base, EVLOOP_ONCE) != 0)
				// Запоминаем отказ опроса
				connection.failure = "setup-poll";
			// Ограничиваем ожидание подключения
			if(elapsed(prepared, now()) >= STREAM_DEADLINE)
				// Прерываем незавершённую подготовку
				connection.failure = "setup-deadline";
		}
		// Переносим исход подготовки в состояние потока
		state.failure = connection.failure;
		// Снимаем временный наблюдатель connect
		if(connection.watcher != nullptr)
			// Освобождаем подписку до её замены
			::event_free(connection.watcher);
		// Снимаем указатель на старую подписку
		connection.watcher = nullptr;
		// Снимаем временный наблюдатель приёмника
		if(connection.reader != nullptr)
			// Освобождаем подписку до её замены
			::event_free(connection.reader);
		// Снимаем указатель на старую подписку
		connection.reader = nullptr;
		// Подготавливаем передачу вне окна измерения
		if(state.failure == nullptr){
			// Больше не принимаем дополнительные подключения
			if(::event_del(listener) != 0)
				// Запоминаем отказ снятия наблюдателя
				state.failure = "listen-unwatch";
			// Создаём подписку чтения потока
			connection.reader = ::event_new(connection.base, connection.peer, EV_READ | EV_PERSIST, &::streamRead, &state);
			// Создаём подписку записи потока
			connection.watcher = ::event_new(connection.base, connection.fd, EV_WRITE | EV_PERSIST, &::streamWrite, &state);
			// Сохраняем наблюдатель для снятия после последнего блока
			state.writer = connection.watcher;
			// Проверяем подготовку наблюдателей
			if((connection.reader == nullptr) || (state.writer == nullptr) || (::event_add(connection.reader, nullptr) != 0))
				// Запоминаем отказ регистрации
				state.failure = "stream-registration";
		}
		// Открываем окно перед постановкой первой записи
		if(state.failure == nullptr){
			// Разрешаем обработку потока
			state.running = true;
			// Запоминаем начало передачи
			state.start = now();
			// Устанавливаем первый блок
			state.pending = STREAM_CHUNK;
			// Учитываем первую постановку
			state.queued = STREAM_CHUNK;
			// Запускаем запись первого блока
			if(::event_add(state.writer, nullptr) != 0)
				// Запоминаем отказ запуска
				state.failure = "write-registration";
		}
		/**
		 * Ожидаем полный поток с ограничением общего времени
		 */
		while((state.failure == nullptr) && !state.stop){
			// Продвигаем обмен конечным опросом
			if(::event_base_loop(connection.base, EVLOOP_ONCE) != 0)
				// Запоминаем отказ опроса
				state.failure = "transfer-poll";
			// Ограничиваем время передачи
			if(elapsed(state.start, now()) >= STREAM_DEADLINE)
				// Прерываем зависший поток
				state.failure = "transfer-deadline";
		}
		// Закрываем обе стороны, включая ошибочную подготовку
		if(!::handshakeClose(connection) && (state.failure == nullptr))
			// Не скрываем отказ очистки
			state.failure = "cleanup-failed";
		// Освобождаем наблюдатель сервера
		if(listener != nullptr)
			// Удаляем подписку
			::event_free(listener);
		// Освобождаем сторож ожидания
		if(timer != nullptr)
			// Удаляем таймер
			::event_free(timer);
		// Проверяем закрытие сервера
		if(!broken(server) && (::evutil_closesocket(server) != 0) && (state.failure == nullptr))
			// Запоминаем отказ закрытия
			state.failure = "server-close";
		// Освобождаем базу событий
		if(connection.base != nullptr)
			// Удаляем базу
			::event_base_free(connection.base);
		// Проверяем полноту потока и положительное окно
		if((state.failure == nullptr) && (!state.stop || (state.queued != STREAM_VOLUME) || (state.written != STREAM_VOLUME) || (state.received != STREAM_VOLUME) || (state.pending != 0) || (state.finish <= state.start)))
			// Отбрасываем неполный результат
			state.failure = "incomplete";
		// Публикуем только полный поток
		if(state.failure == nullptr){
			// Устанавливаем точное число блоков
			result.operations = (STREAM_VOLUME / STREAM_CHUNK);
			// Устанавливаем точный объём
			result.bytes = STREAM_VOLUME;
			// Устанавливаем время передачи без очистки
			result.seconds = elapsed(state.start, state.finish);
			// Подтверждаем контрольные счётчики
			::printf("STREAM_ACCOUNT queued=%zu written=%zu received=%zu pending=%zu\n", state.queued, state.written, state.received, state.pending);
		// Отказываем без публикации частичной скорости
		} else
			// Сообщаем причину и фактический объём
			::fprintf(stderr, "STREAM_BENCH_FAIL: %s queued=%zu written=%zu received=%zu pending=%zu\n", state.failure, state.queued, state.written, state.received, state.pending);
		// Выводим только полный результат либо пустой исход
		return result;
	}
	/**
	 * @brief Функция проверки настроек сокета сценария подключений
	 *
	 * @param fd   сокет текущей стороны
	 * @param hard признак немедленного закрытия клиента
	 * @return     результат установки всех обязательных опций
	 *
	 */
	static bool handshakeOptions(const socket_t fd, const bool hard) noexcept {
		// Отключаем задержку коротких отправлений
		const int32_t enable = 1;
		// Проверяем неблокирующий режим и алгоритм Нейгла
		if((::evutil_make_socket_nonblocking(fd) != 0) || (::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, raw(&enable), sizeof(enable)) != 0))
			// Отказываем при неполной настройке
			return false;
		// Настраиваем обрыв только клиентской стороны
		if(hard){
			// Не оставляем динамический порт в TIME_WAIT
			struct linger value{};
			// Включаем немедленное закрытие
			value.l_onoff = 1;
			// Проверяем применение опции
			if(::setsockopt(fd, SOL_SOCKET, SO_LINGER, raw(&value), sizeof(value)) != 0)
				// Отказываем при неустановленной опции
				return false;
		}
		// Подтверждаем полную настройку
		return true;
	}
	/**
	 * @brief Функция обработки неожиданного чтения принятой стороны
	 *
	 * @param arg состояние сценария
	 *
	 */
	static void handshakeRead(evutil_socket_t, short, void * arg) noexcept {
		// Получаем состояние сценария
		handshake_t & state = *reinterpret_cast <handshake_t *> (arg);
		// До общего закрытия данные и EOF не предусмотрены нагрузкой
		if(state.failure == nullptr)
			// Отмечаем преждевременное событие
			state.failure = "unexpected-peer-read";
	}
	/**
	 * @brief Функция принятия серверной стороны текущего цикла
	 *
	 * @param fd  слушающий сокет
	 * @param arg состояние сценария
	 *
	 */
	static void handshakeAccept(evutil_socket_t fd, short, void * arg) noexcept {
		// Получаем состояние сценария
		handshake_t & state = *reinterpret_cast <handshake_t *> (arg);
		// Принимаем одно ожидающее подключение
		const socket_t peer = ::accept(fd, nullptr, nullptr);
		// Различаем временную недоступность и настоящий отказ
		if(broken(peer)){
			// Получаем причину неудачи немедленно
			const int32_t error = EVUTIL_SOCKET_ERROR();
			// Не подменяем ошибку ожиданием следующего события
			if(!::echoBlocked(error) && !::echoInterrupted(error) && (state.failure == nullptr))
				// Запоминаем отказ приёма
				state.failure = "accept-failed";
			// Завершаем текущий callback
			return;
		}
		// Не допускаем приём вне текущего единственного цикла
		if(!broken(state.peer) || broken(state.fd) || (state.failure != nullptr)){
			// Освобождаем неожиданное подключение
			::evutil_closesocket(peer);
			// Сохраняем первый отказ
			if(state.failure == nullptr)
				// Отмечаем нарушение последовательности
				state.failure = "unexpected-accept";
			// Завершаем обработку
			return;
		}
		// Сохраняем peer для обязательной очистки даже при ошибке настройки
		state.peer = peer;
		// Проверяем опции принятой стороны
		if(!::handshakeOptions(peer, false)){
			// Запоминаем отказ настройки
			state.failure = "peer-options";
			// Завершаем обработку
			return;
		}
		// Создаём реальную подписку принятого подключения на чтение
		state.reader = ::event_new(state.base, peer, EV_READ | EV_PERSIST, &::handshakeRead, &state);
		// Проверяем создание и регистрацию подписки
		if((state.reader == nullptr) || (::event_add(state.reader, nullptr) != 0)){
			// Не засчитываем неподготовленную серверную сторону
			state.failure = "peer-registration";
			// Завершаем обработку
			return;
		}
		// Подтверждаем готовность принятой стороны
		state.accepted++;
	}
	/**
	 * @brief Функция проверки исхода подключения клиента
	 *
	 * @param fd  сокет клиента
	 * @param arg состояние сценария
	 *
	 */
	static void handshakeConnect(evutil_socket_t fd, short, void * arg) noexcept {
		// Получаем состояние сценария
		handshake_t & state = *reinterpret_cast <handshake_t *> (arg);
		// Значение системной ошибки подключения
		int32_t code = 0;
		// Размер получаемого значения
		socklen_t length = sizeof(code);
		// Проверяем принадлежность, однократность и фактический исход подключения
		if((static_cast <socket_t> (fd) != state.fd) || state.ready || (::getsockopt(fd, SOL_SOCKET, SO_ERROR, raw(&code), &length) != 0) || (code != 0)){
			// Сохраняем первый отказ
			if(state.failure == nullptr)
				// Не засчитываем отказ как успешное подключение
				state.failure = "connect-failed";
			// Завершаем обработку
			return;
		}
		// Подтверждаем клиентскую сторону
		state.ready = true;
		// Учитываем успешный connect отдельно от accept
		state.connected++;
	}
	/**
	 * @brief Функция периодического возврата из ожидания для проверки срока
	 *
	 */
	static void handshakeTick(evutil_socket_t, short, void *) noexcept {}
	/**
	 * @brief Функция освобождения обеих сторон текущего цикла
	 *
	 * @param state состояние сценария
	 * @return      результат закрытия обоих сокетов
	 *
	 */
	static bool handshakeClose(handshake_t & state) noexcept {
		// Результат освобождения обеих сторон
		bool result = true;
		// Снимаем клиентский наблюдатель до закрытия сокета
		if(state.watcher != nullptr){
			// Освобождаем подписку клиента
			::event_free(state.watcher);
			// Снимаем указатель на освобождённую подписку
			state.watcher = nullptr;
		}
		// Снимаем серверный наблюдатель до закрытия сокета
		if(state.reader != nullptr){
			// Освобождаем подписку peer
			::event_free(state.reader);
			// Снимаем указатель на освобождённую подписку
			state.reader = nullptr;
		}
		/**
		 * Закрываем все созданные сокеты даже при отказе первого закрытия
		 */
		for(socket_t * fd : {&state.fd, &state.peer}){
			// Пропускаем несозданную сторону
			if(broken(*fd))
				// Продолжаем очистку
				continue;
			// Проверяем закрытие реального сокета
			if(::evutil_closesocket(*fd) != 0)
				// Сохраняем отказ освобождения
				result = false;
			// Учитываем подтверждённое закрытие
			else state.closed++;
			// Снимаем использованный дескриптор
			*fd = INVALID;
		}
		// Выводим результат очистки
		return result;
	}
	/**
	 * @brief Функция прогона полных циклов установления соединений
	 *
	 * @return итоги полного прогона либо пустой результат при отказе
	 *
	 */
	static outcome_t handshake() noexcept {
		// Итоги прогона сценария
		outcome_t result;
		// Состояние обеих сторон и общие счётчики
		handshake_t state;
		// Создаём цикл событий
		state.base = ::event_base_new();
		// Создаём слушающий сокет
		const socket_t server = ::socket(AF_INET, SOCK_STREAM, 0);
		// Устанавливаем семейство loopback-адреса
		state.address.sin_family = AF_INET;
		// Устанавливаем петлевой адрес и оставляем выбор порта системе
		state.address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		// Размер адреса для получения назначенного порта
		socklen_t length = sizeof(state.address);
		// Проверяем все операции подготовки слушающего сокета
		if((state.base == nullptr) || broken(server) || (::evutil_make_socket_nonblocking(server) != 0) || (::bind(server, reinterpret_cast <struct sockaddr *> (&state.address), sizeof(state.address)) != 0) || (::getsockname(server, reinterpret_cast <struct sockaddr *> (&state.address), &length) != 0) || (::listen(server, BACKLOG) != 0))
			// Запоминаем отказ подготовки
			state.failure = "server-setup";
		// Подписка слушающего сокета
		struct event * listener = nullptr;
		// Периодический возврат для проверки срока
		struct event * timer = nullptr;
		// Настраиваем наблюдатели только у подготовленного сервера
		if(state.failure == nullptr){
			// Создаём подписку приёма подключений
			listener = ::event_new(state.base, server, EV_READ | EV_PERSIST, &::handshakeAccept, &state);
			// Создаём общий сторож ожидания без переустановки на каждый цикл
			timer = ::event_new(state.base, INVALID, EV_PERSIST, &::handshakeTick, nullptr);
			// Ограничиваем шаг блокирующего ожидания четвертью секунды
			const struct timeval step{0, 250000};
			// Проверяем регистрацию обоих наблюдателей
			if((listener == nullptr) || (timer == nullptr) || (::event_add(listener, nullptr) != 0) || (::event_add(timer, &step) != 0))
				// Запоминаем отказ регистрации
				state.failure = "server-registration";
		}
		// Момент начала измеряемой части
		auto start = std::chrono::steady_clock::time_point{};
		/**
		 * Выполняем полные циклы прогрева и измерения
		 */
		for(size_t round = 0; (round < (ACCEPT_WARMUP + ACCEPT_ROUNDS)) && (state.failure == nullptr); round++){
			// Открываем окно перед созданием первого измеряемого клиента
			if(round == ACCEPT_WARMUP)
				// Запоминаем начало измерения
				start = now();
			// Сбрасываем подтверждение клиента
			state.ready = false;
			// Устанавливаем общий срок текущего цикла
			const auto tick = now();
			// Создаём очередной клиентский сокет
			state.fd = ::socket(AF_INET, SOCK_STREAM, 0);
			// Проверяем создание и обязательные опции клиента
			if(broken(state.fd) || !::handshakeOptions(state.fd, true)){
				// Запоминаем отказ подготовки
				state.failure = "client-setup";
				// Прекращаем прогон
				break;
			}
			// Начинаем подключение к подготовленному серверу
			const int32_t connected = ::connect(state.fd, reinterpret_cast <const struct sockaddr *> (&state.address), sizeof(state.address));
			// Проверяем немедленный исход подключения
			if(connected != 0){
				// Получаем причину незавершённого подключения
				const int32_t error = EVUTIL_SOCKET_ERROR();
				/**
				 * Проверяем допустимый неблокирующий исход соответствующей платформы
				 */
				#if defined(_WIN32) || defined(_WIN64)
					// Ожидаем только штатно начатое подключение Winsock
					if(error != WSAEWOULDBLOCK)
						// Запоминаем немедленный отказ
						state.failure = "connect-start";
				/**
				 * Для интерфейса сокетов POSIX
				 */
				#else
					// Ожидаем только штатно начатое подключение POSIX
					if(error != EINPROGRESS)
						// Запоминаем немедленный отказ
						state.failure = "connect-start";
				#endif
			}
			// Создаём одноразовое подтверждение исхода connect
			state.watcher = ::event_new(state.base, state.fd, EV_WRITE, &::handshakeConnect, &state);
			// Проверяем создание и регистрацию подтверждения
			if((state.watcher == nullptr) || (::event_add(state.watcher, nullptr) != 0))
				// Запоминаем отказ подписки
				state.failure = "client-registration";
			/**
			 * Ожидаем именно обе стороны текущего цикла
			 */
			while((state.failure == nullptr) && (!state.ready || (state.accepted != (round + 1)))){
				// Проверяем результат одного оборота событий
				if(::event_base_loop(state.base, EVLOOP_ONCE) != 0)
					// Запоминаем отказ цикла
					state.failure = "poll-failed";
				// Проверяем предельное время полного цикла
				if(elapsed(tick, now()) >= 5.0)
					// Запоминаем превышение срока
					state.failure = "round-deadline";
			}
			// Не засчитываем ошибочный цикл
			if(state.failure != nullptr)
				// Прекращаем прогон
				break;
			// Закрываем обе стороны до счёта и следующего подключения
			if(!::handshakeClose(state)){
				// Запоминаем отказ освобождения
				state.failure = "close-failed";
				// Прекращаем прогон
				break;
			}
			// Учитываем полный цикл прогрева
			if(round < ACCEPT_WARMUP)
				// Увеличиваем счётчик прогрева
				state.warmed++;
			// Учитываем полный измеренный цикл
			else state.done++;
		}
		// Закрываем окно после освобождения последней пары
		const auto finish = now();
		// Очищаем остатки ошибочного цикла
		if(!::handshakeClose(state) && (state.failure == nullptr))
			// Запоминаем отказ завершающей очистки
			state.failure = "cleanup-failed";
		// Освобождаем подписку слушающего сокета
		if(listener != nullptr)
			// Удаляем наблюдатель
			::event_free(listener);
		// Освобождаем сторож ожидания
		if(timer != nullptr)
			// Удаляем таймер
			::event_free(timer);
		// Проверяем закрытие слушающего сокета
		if(!broken(server) && (::evutil_closesocket(server) != 0) && (state.failure == nullptr))
			// Запоминаем отказ закрытия сервера
			state.failure = "server-close";
		// Освобождаем цикл событий
		if(state.base != nullptr)
			// Удаляем базу наблюдателей
			::event_base_free(state.base);
		// Проверяем точное количество полных циклов и обеих сторон
		if((state.failure == nullptr) && ((state.warmed != ACCEPT_WARMUP) || (state.done != ACCEPT_ROUNDS) || (state.connected != (ACCEPT_WARMUP + ACCEPT_ROUNDS)) || (state.accepted != state.connected) || (state.closed != (2 * state.connected)) || (finish <= start)))
			// Отбрасываем неполный прогон
			state.failure = "incomplete";
		// Публикуем только полный корректный результат
		if(state.failure == nullptr){
			// Устанавливаем число полных измеренных циклов
			result.operations = state.done;
			// Устанавливаем длительность полного окна
			result.seconds = elapsed(start, finish);
			// Подтверждаем полноту обеих сторон
			::printf("ACCEPT_ACCOUNT warmed=%zu connected=%zu accepted=%zu closed=%zu measured=%zu\n", state.warmed, state.connected, state.accepted, state.closed, state.done);
		// Сообщаем отказ без публикации положительного результата
		} else
			// Выводим причину и фактические счётчики
			::fprintf(stderr, "ACCEPT_BENCH_FAIL: %s warmed=%zu measured=%zu accepted=%zu closed=%zu\n", state.failure, state.warmed, state.done, state.accepted, state.closed);
		// Выводим итог полного прогона либо пустой результат
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
		return timeval{static_cast <decltype(timeval::tv_sec)> (milliseconds / 1000), static_cast <decltype(timeval::tv_usec)> ((milliseconds % 1000) * 1000)};
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
		/**
		 * Освобождаем цикл событий стенда
		 */
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
		/**
		 * Освобождаем цикл событий стенда
		 */
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
		/**
		 * Освобождаем цикл событий стенда
		 */
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
			deadline.tv_usec = static_cast <decltype(timeval::tv_usec)> ((1 + (i % TIMER_SPREAD)) * 1000);
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
		const outcome_t outcome = ::echoExchange(1, ECHO_SINGLE_ROUNDS);
		// Не публикуем неполный прогон как успешный замер
		if((outcome.operations != ECHO_SINGLE_ROUNDS) || (outcome.seconds <= 0))
			// Возвращаем отказ вызывающему измерителю
			return EXIT_FAILURE;
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
	// Если сценарий TCP-обмена множеством подключений выполняется
	if(selected("net/io/echo/multi", name)){
		// Выполняем прогон сценария обмена на множестве подключений
		const outcome_t outcome = ::echoExchange(ECHO_MULTI_CONNECTIONS, ECHO_MULTI_ROUNDS);
		// Не публикуем неполный прогон как успешный замер
		if((outcome.operations != ECHO_MULTI_ROUNDS) || (outcome.seconds <= 0))
			// Возвращаем отказ вызывающему измерителю
			return EXIT_FAILURE;
		// Выводим результат прогона сценария
		report("net/io/echo/multi", "обменов/с", perSecond(outcome), outcome);
	}
	// Если сценарий потоковой передачи выполняется
	if(selected("net/io/stream/throughput", name)){
		// Выполняем прогон сценария потоковой передачи
		const outcome_t outcome = ::transfer();
		// Не публикуем ошибочный или неполный поток
		if((outcome.operations != (STREAM_VOLUME / STREAM_CHUNK)) || (outcome.bytes != STREAM_VOLUME) || (outcome.seconds <= 0))
			// Возвращаем отказ вызывающему измерителю
			return EXIT_FAILURE;
		// Выводим результат прогона сценария
		report("net/io/stream/throughput", "МБ/с", megabytes(outcome), outcome);
	}
	// Если сценарий установления соединений выполняется
	if(selected("net/io/accept/connections", name)){
		// Выполняем прогон сценария установления соединений
		const outcome_t outcome = ::handshake();
		// Не публикуем неполный или ошибочный прогон
		if((outcome.operations != ACCEPT_ROUNDS) || (outcome.seconds <= 0))
			// Возвращаем отказ вызывающему измерителю
			return EXIT_FAILURE;
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
