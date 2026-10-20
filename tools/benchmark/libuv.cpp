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
 * @brief Функция сведения описателя библиотеки к описателю сокета стенда
 *
 * @details У наречия POSIX описатель библиотеки - это целое, и приведение
 *          тождественно. У MS Windows это описатель системы (`void *`), а сокет
 *          там - целое без знака указательной ширины; сведение идёт через
 *          целочисленный вид указательной ширины, иначе значение усекается молча
 *
 * @param fd описатель, отданный библиотекой
 * @return   описатель сокета в виде, принятом стендом
 *
 */
static inline rival::socket_t native(const uv_os_fd_t fd) noexcept {
	/**
	 * Если стенд собран под операционную систему MS Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Выводим описатель сокета, сведённый через целое указательной ширины
		return static_cast <rival::socket_t> (reinterpret_cast <intptr_t> (fd));
	/**
	 * Если стенд собран под все остальные операционные системы
	 */
	#else
		// Выводим описатель сокета как есть
		return static_cast <rival::socket_t> (fd);
	#endif
}
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
		// Цикл событий потока
		uv_loop_t * loop;
		// Передатчик потока
		uv_tcp_t * sender;
		// Принятый приёмник потока
		uv_tcp_t * peer;
		// Единственный запрос записи
		uv_write_t request;
		// Признак успешного подключения клиента
		bool ready;
		// Признак открытого окна передачи
		bool running;
		// Признак полного завершения передачи
		bool stop;
		// Признак завершающей очистки
		bool closing;
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
		 loop(nullptr), sender(nullptr), peer(nullptr), request{},
		 ready(false), running(false), stop(false), closing(false), failure(nullptr),
		 pending(0), queued(0), written(0), received(0), start{}, finish{} {}
	} stream_t;

	/**
	 * @brief Структура состояния прогона сценария установления соединений
	 *
	 */
	typedef struct Handshake {
		// Цикл событий сценария
		uv_loop_t * loop;
		// Адрес слушающего сокета
		struct sockaddr_in address;
		// Дескриптор текущего клиента
		uv_tcp_t * handle;
		// Дескриптор принятой стороны
		uv_tcp_t * peer;
		// Запрос текущего подключения
		uv_connect_t * request;
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
		 loop(nullptr), address{}, handle(nullptr), peer(nullptr), request(nullptr),
		 ready(false), failure(nullptr), connected(0), accepted(0), closed(0), warmed(0), done(0) {}
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
	 * @brief Состояние TCP-прогона с индивидуальным прогревом подключений
	 *
	 */
	struct Echo_State;
	/**
	 * @brief Структура участника TCP-обмена с собственными буферами отправки
	 *
	 * @note Один запрос ожидает один ответ. Пока завершение прежней отправки
	 *       ещё не доставлено, следующий фрагмент сохраняется отдельно;
	 *       память uv_write_t и переданные октеты живут до write-callback
	 *
	 */
	typedef struct Echo_Peer {
		// Дескриптор подключения
		uv_tcp_t handle;
		// Запрос подключения клиента
		uv_connect_t connection;
		// Запрос текущей отправки
		uv_write_t request;
		// Состояние всего прогона
		Echo_State * state;
		// Признак клиентской стороны
		bool client;
		// Признак незавершённой отправки
		bool writing;
		// Признак отправки запроса внутри окна замера
		bool measured;
		// Количество октетов незавершённого ответа
		size_t received;
		// Количество индивидуальных обменов прогрева
		size_t warmed;
		// Количество октетов, ожидающих завершения прежней отправки
		size_t queued;
		// Буфер приёма одного сообщения или его части
		char input[ECHO_PAYLOAD];
		// Буфер, используемый текущей отправкой
		char output[ECHO_PAYLOAD];
		// Буфер следующей отправки
		char pending[ECHO_PAYLOAD];
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Echo_Peer() noexcept :
		 handle{}, connection{}, request{}, state(nullptr), client(false), writing(false), measured(false), received(0), warmed(0), queued(0), input{}, output{}, pending{} {}
	} echo_peer_t;
	/**
	 * @brief Структура состояния TCP-прогона с точной границей измерения
	 *
	 */
	typedef struct Echo_State {
		// Цикл событий текущего прогона
		uv_loop_t loop;
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
		// Момент открытия измеряемого окна
		chrono::steady_clock::time_point start;
		// Единственная отметка конца измеряемого окна
		chrono::steady_clock::time_point finish;
		// Клиентские подключения, живущие до закрытия цикла
		vector <unique_ptr <echo_peer_t>> clients;
		// Серверные подключения, живущие до закрытия цикла
		vector <unique_ptr <echo_peer_t>> peers;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Echo_State() noexcept :
		 loop{}, stop(false), measuring(false), fleet(false), connections(0), ready(0), accepted(0), rounds(0), done(0), failure(nullptr) {}
	} echo_state_t;
	/**
	 * @brief Функция остановки недействительного TCP-прогона
	 *
	 * @param state  состояние прогона
	 * @param reason причина отказа
	 *
	 */
	static void echoFail(echo_state_t & state, const char * reason) noexcept {
		// Не изменяем уже закрытое окно при очистке
		if(state.stop)
			// Оставляем первый итог прогона
			return;
		// Сохраняем причину отказа
		state.failure = reason;
		// Запрещаем дальнейшие отправки и учёт
		state.stop = true;
		// Прерываем ожидание остальных участников
		::uv_stop(&state.loop);
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
	 * @brief Функция постановки стабильного буфера в очередь отправки
	 *
	 * @param peer состояние отправителя
	 * @param data передаваемые октеты
	 * @param size количество передаваемых октетов
	 *
	 */
	static void echoWrite(echo_peer_t & peer, const char * data, const size_t size) noexcept;
	/**
	 * @brief Функция завершения отправки и освобождения её буфера
	 *
	 * @param request завершённый запрос
	 * @param status  результат отправки
	 *
	 */
	static void echoWritten(uv_write_t * request, const int32_t status) noexcept {
		// Получаем состояние отправителя
		echo_peer_t * peer = reinterpret_cast <echo_peer_t *> (request->data);
		// Разрешаем повторное использование запроса и буфера
		peer->writing = false;
		// Не запускаем хвост очереди при очистке
		if(peer->state->stop)
			// Оставляем очередь общему завершению
			return;
		// Если система отказала в отправке
		if(status < 0){
			// Отклоняем неполный прогон
			::echoFail(* peer->state, "write-completion-failed");
			// Не отправляем оставшиеся октеты
			return;
		}
		// Если накоплены октеты следующей отправки
		if(peer->queued > 0){
			// Запоминаем размер очереди
			const size_t size = peer->queued;
			// Освобождаем место для новых фрагментов
			peer->queued = 0;
			// Передаём накопленные октеты в освободившийся запрос
			::echoWrite(* peer, peer->pending, size);
		}
	}
	/**
	 * @brief Функция постановки стабильного буфера в очередь отправки
	 *
	 * @param peer состояние отправителя
	 * @param data передаваемые октеты
	 * @param size количество передаваемых октетов
	 *
	 */
	static void echoWrite(echo_peer_t & peer, const char * data, const size_t size) noexcept {
		// Не добавляем отправки за границей прогона
		if(peer.state->stop)
			// Оставляем буферы общему завершению
			return;
		// Если предыдущий запрос ещё принадлежит libuv
		if(peer.writing){
			// Один запрос в полёте ограничивает очередь одним сообщением
			if(size > (ECHO_PAYLOAD - peer.queued)){
				// Отклоняем нарушение протокола вместо перезаписи памяти
				::echoFail(* peer.state, "write-queue-overflow");
				// Не копируем лишние октеты
				return;
			}
			// Сохраняем октеты отдельно от активного буфера
			::memcpy(peer.pending + peer.queued, data, size);
			// Увеличиваем размер отложенной отправки
			peer.queued += size;
			// Ожидаем callback прежнего запроса
			return;
		}
		// Проверяем границу стабильного буфера
		if(size > ECHO_PAYLOAD){
			// Отклоняем сообщение неизвестного размера
			::echoFail(* peer.state, "write-size-invalid");
			// Не передаём системе недействительный буфер
			return;
		}
		// Сохраняем данные до завершения асинхронной отправки
		::memcpy(peer.output, data, size);
		// Формируем описание стабильного буфера
		uv_buf_t buffer = ::uv_buf_init(peer.output, static_cast <uint32_t> (size));
		// Запрещаем повторное использование запроса до callback
		peer.writing = true;
		// Проверяем постановку отправки в очередь
		if(::uv_write(&peer.request, reinterpret_cast <uv_stream_t *> (&peer.handle), &buffer, 1, &::echoWritten) != 0){
			// Запрос не принят системой и не получит callback
			peer.writing = false;
			// Отклоняем неполный прогон
			::echoFail(* peer.state, "write-submit-failed");
		}
	}
	/**
	 * @brief Функция предоставления отдельного буфера приёма
	 *
	 * @param handle дескриптор подключения
	 * @param buffer выводимое описание буфера
	 *
	 */
	static void echoAllocate(uv_handle_t * handle, size_t, uv_buf_t * buffer) noexcept {
		// Получаем состояние читающего подключения
		echo_peer_t * peer = reinterpret_cast <echo_peer_t *> (handle->data);
		// Предоставляем буфер только этого подключения
		(*buffer) = ::uv_buf_init(peer->input, ECHO_PAYLOAD);
	}
	/**
	 * @brief Функция обработки принятых октетов TCP-обмена
	 *
	 * @param stream дескриптор подключения
	 * @param size   количество принятых октетов или ошибка
	 * @param buffer описание принятых октетов
	 *
	 */
	static void echoRead(uv_stream_t * stream, const ssize_t size, const uv_buf_t * buffer) noexcept {
		// Получаем состояние подключения
		echo_peer_t * peer = reinterpret_cast <echo_peer_t *> (stream->data);
		// Получаем состояние прогона
		echo_state_t & state = * peer->state;
		// Не учитываем хвост пачки после закрытия окна
		if(state.stop)
			// Оставляем подключение общему завершению
			return;
		// Если чтение завершилось отказом или разрывом
		if(size < 0){
			// Отклоняем потерю участника
			::echoFail(state, "read-failed");
			// Не используем недействительный размер
			return;
		}
		// Если очередное чтение пока не принесло октетов
		if(size == 0)
			// Продолжаем ожидание
			return;
		// Если октеты приняты сервером
		if(!peer->client){
			// Возвращаем фактические данные, сохраняя их до write-callback
			::echoWrite(* peer, buffer->base, static_cast <size_t> (size));
			// Сервер не учитывает клиентские обмены
			return;
		}
		// Накапливаем полный клиентский ответ
		peer->received += static_cast <size_t> (size);
		// Если ответ пока неполный
		if(peer->received < ECHO_PAYLOAD)
			// Ожидаем остальные октеты
			return;
		// Исключаем неожиданное объединение нескольких ответов
		if(peer->received != ECHO_PAYLOAD){
			// Отклоняем нарушение одного запроса в полёте
			::echoFail(state, "response-size-invalid");
			// Не выдаём лишние октеты за очередной обмен
			return;
		}
		// Освобождаем счётчик для следующего ответа
		peer->received = 0;
		// Учитываем ответ в соответствующей фазе
		const bool more = ::echoAccount(state, * peer);
		// Последний готовый участник открывает окно без остановки потока
		if(state.fleet && !state.measuring && (state.ready == state.connections)){
			// Повторно проверяем полный принятый состав
			if(!::echoBegin(state))
				// Отклоняем неполный прогрев
				::echoFail(state, "warmup-incomplete");
		}
		// Если следующий запрос разрешён
		if(more && !state.stop){
			// Запоминаем фазу именно нового запроса
			peer->measured = state.measuring;
			// Продолжаем обмен без повторного массового запуска
			::echoWrite(* peer, reinterpret_cast <const char *> (gPayload), ECHO_PAYLOAD);
		}
		// Если достигнут первый барьер или конец прогона
		if(state.stop || (!state.fleet && (state.ready == state.connections)))
			// Возвращаем управление подготовке следующей фазы или очистке
			::uv_stop(&state.loop);
	}
	/**
	 * @brief Функция запуска подключившегося клиента
	 *
	 * @param request запрос подключения
	 * @param status  результат подключения
	 *
	 */
	static void echoConnected(uv_connect_t * request, const int32_t status) noexcept {
		// Получаем состояние клиента
		echo_peer_t * peer = reinterpret_cast <echo_peer_t *> (request->data);
		// Не запускаем клиента после отказа другого участника
		if(peer->state->stop)
			// Оставляем подключение очистке
			return;
		// Проверяем результат подключения
		if(status < 0){
			// Отклоняем отсутствующего клиента
			::echoFail(* peer->state, "client-connect-failed");
			// Не запускаем чтение неподключённого дескриптора
			return;
		}
		// Проверяем подписку чтения подключённого клиента
		if(::uv_read_start(reinterpret_cast <uv_stream_t *> (&peer->handle), &::echoAllocate, &::echoRead) != 0){
			// Отклоняем клиента без чтения ответов
			::echoFail(* peer->state, "client-read-start-failed");
			// Не отправляем сообщение неподключённому клиенту
			return;
		}
		// Запускаем индивидуальный прогрев
		::echoWrite(* peer, reinterpret_cast <const char *> (gPayload), ECHO_PAYLOAD);
	}
	/**
	 * @brief Функция принятия участника TCP-прогона
	 *
	 * @param server слушающий дескриптор
	 * @param status результат ожидания входящего подключения
	 *
	 */
	static void echoAccepted(uv_stream_t * server, const int32_t status) noexcept {
		// Получаем состояние прогона
		echo_state_t & state = * reinterpret_cast <echo_state_t *> (server->data);
		// Не принимаем новых участников после остановки
		if(state.stop)
			// Оставляем слушатель очистке
			return;
		// Если входящее подключение недействительно или состав превышен
		if((status < 0) || (state.accepted >= state.connections)){
			// Не подменяем состав случайным подключением
			::echoFail(state, "accept-status-invalid");
			// Прекращаем приём
			return;
		}
		// Сохраняем состояние до закрытия всех дескрипторов
		state.peers.push_back(unique_ptr <echo_peer_t> (new echo_peer_t));
		// Получаем добавленного участника
		echo_peer_t & peer = * state.peers.back();
		// Устанавливаем общее состояние прогона
		peer.state = &state;
		// Инициализируем серверную сторону подключения
		if(::uv_tcp_init(&state.loop, &peer.handle) != 0){
			// Отклоняем отсутствие дескриптора
			::echoFail(state, "peer-init-failed");
			// Не используем неинициализированный дескриптор
			return;
		}
		// Связываем callback чтения с состоянием участника
		peer.handle.data = &peer;
		// Связываем callback отправки со стабильным запросом
		peer.request.data = &peer;
		// Проверяем приём, параметры сокета и запуск чтения
		if((::uv_accept(server, reinterpret_cast <uv_stream_t *> (&peer.handle)) != 0) || (::uv_tcp_nodelay(&peer.handle, 1) != 0) || (::uv_read_start(reinterpret_cast <uv_stream_t *> (&peer.handle), &::echoAllocate, &::echoRead) != 0)){
			// Отклоняем неподготовленного участника
			::echoFail(state, "peer-setup-failed");
			// Не учитываем неполное подключение
			return;
		}
		// Учитываем только полностью подготовленную серверную сторону
		state.accepted++;
	}
	/**
	 * @brief Функция ограничения подготовки и измерения общим сроком
	 *
	 * @param timer дескриптор предельного срока
	 *
	 */
	static void echoExpired(uv_timer_t * timer) noexcept {
		// Отклоняем незавершённый прогрев или обмен
		::echoFail(* reinterpret_cast <echo_state_t *> (timer->data), "deadline-expired");
	}
	/**
	 * @brief Функция закрытия дескриптора после остановки прогона
	 *
	 * @param handle закрываемый дескриптор
	 *
	 */
	static void echoClose(uv_handle_t * handle, void *) noexcept {
		// Не закрываем уже закрываемый дескриптор повторно
		if(!::uv_is_closing(handle))
			// Сохраняем память дескриптора до осушения цикла
			::uv_close(handle, nullptr);
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
		// Состояние текущего прогона и его дескрипторы
		echo_state_t state;
		// Проверяем параметры и создание цикла до использования дескрипторов
		if((connections == 0) || (rounds == 0) || (::uv_loop_init(&state.loop) != 0)){
			// Сообщаем об отказе до запуска нагрузки
			::fprintf(stderr, "ECHO_BENCH_FAIL: loop-or-parameters-invalid\n");
			// Возвращаем недействительный результат
			return result;
		}
		// Устанавливаем полный состав участников
		state.connections = connections;
		// Устанавливаем точное число измеряемых обменов
		state.rounds = rounds;
		// Резервируем состояния клиентов до начала нагрузки
		state.clients.reserve(connections);
		// Резервируем состояния сервера до начала нагрузки
		state.peers.reserve(connections);
		// Слушающий дескриптор
		uv_tcp_t server{};
		// Общий предельный срок подготовки и измерения
		uv_timer_t deadline{};
		// Адрес слушателя с автоматически выбранным портом
		struct sockaddr_in address{};
		// Размер адреса для получения выбранного порта
		int32_t length = sizeof(address);
		// Проверяем создание слушателя
		if(::uv_tcp_init(&state.loop, &server) != 0)
			// Не используем неинициализированный дескриптор
			::echoFail(state, "listener-init-failed");
		// Если дескриптор слушателя создан
		if(!state.stop){
			// Передаём состояние прогона обработчику приёма
			server.data = &state;
			// Проверяем привязку и запуск слушателя
			if((::uv_ip4_addr("127.0.0.1", 0, &address) != 0) || (::uv_tcp_bind(&server, reinterpret_cast <const struct sockaddr *> (&address), 0) != 0) || (::uv_tcp_getsockname(&server, reinterpret_cast <struct sockaddr *> (&address), &length) != 0) || (::uv_listen(reinterpret_cast <uv_stream_t *> (&server), BACKLOG, &::echoAccepted) != 0))
				// Не запускаем клиентов без готового сервера
				::echoFail(state, "listener-setup-failed");
		}
		// Если слушатель готов и можно ограничить время прогона
		if(!state.stop){
			// Проверяем создание таймера
			if(::uv_timer_init(&state.loop, &deadline) != 0)
				// Не оставляем прогон без ограничения
				::echoFail(state, "deadline-init-failed");
			else {
				// Передаём состояние обработчику предельного срока
				deadline.data = &state;
				// Ограничиваем подготовку и измерение одной минутой
				if(::uv_timer_start(&deadline, &::echoExpired, 60000, 0) != 0)
					// Отклоняем прогон без рабочего таймера
					::echoFail(state, "deadline-start-failed");
			}
		}
		/**
		 * Создаём клиентов, обслуживая приём между запросами подключения
		 */
		for(size_t i = 0; (i < connections) && !state.stop; i++){
			// Сохраняем состояние клиента до осушения цикла
			state.clients.push_back(unique_ptr <echo_peer_t> (new echo_peer_t));
			// Получаем добавленного клиента
			echo_peer_t & peer = * state.clients.back();
			// Подключаем общее состояние прогона
			peer.state = &state;
			// Отмечаем клиентскую сторону
			peer.client = true;
			// Проверяем создание клиентского дескриптора
			if(::uv_tcp_init(&state.loop, &peer.handle) != 0){
				// Отклоняем неполный состав
				::echoFail(state, "client-init-failed");
				// Не используем неинициализированный дескриптор
				break;
			}
			// Связываем чтение с состоянием клиента
			peer.handle.data = &peer;
			// Связываем завершение отправки со стабильным запросом
			peer.request.data = &peer;
			// Связываем завершение подключения с клиентом
			peer.connection.data = &peer;
			// Проверяем параметры и начало подключения
			if((::uv_tcp_nodelay(&peer.handle, 1) != 0) || (::uv_tcp_connect(&peer.connection, &peer.handle, reinterpret_cast <const struct sockaddr *> (&address), &::echoConnected) != 0))
				// Не подменяем отказавшего клиента меньшим составом
				::echoFail(state, "client-connect-submit-failed");
			// Если запрос подключения принят
			if(!state.stop)
				// Не оставляем весь всплеск ConnectEx без обработки AcceptEx
				::uv_run(&state.loop, UV_RUN_NOWAIT);
		}
		// Если полный первый барьер ещё не достигнут при создании клиентов
		if(!state.stop && (state.ready < connections))
			// Ждём индивидуального прогрева каждого клиента
			::uv_run(&state.loop, UV_RUN_DEFAULT);
		// Проверяем полный состав перед общим прогревом
		if(!state.stop && (state.ready == connections) && (state.accepted == connections)){
			// Переключаемся на непрерывный общий прогрев
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
			 * Запускаем общую нагрузку без открытия измеряемого окна
			 */
			for(const auto & peer : state.clients)
				// Поддерживаем один запрос в полёте у каждого клиента
				::echoWrite(* peer, reinterpret_cast <const char *> (gPayload), ECHO_PAYLOAD);
			// Если отправки общего прогрева приняты
			if(!state.stop)
				// Переходим от общего прогрева к замеру без паузы
				::uv_run(&state.loop, UV_RUN_DEFAULT);
		} else
			// Неполный состав не имеет измеряемого результата
			::echoFail(state, "warmup-incomplete");
		// Проверяем единственное успешное завершение на точной границе
		if((state.failure == nullptr) && (!state.stop || !state.measuring || (state.done != rounds)))
			// Не выдаём преждевременный выход за выполненную нагрузку
			state.failure = "exchange-incomplete";
		// Запрещаем обработчикам очистки продолжать обмен
		state.stop = true;
		// Закрываем слушатель, таймер и обе стороны всех подключений
		::uv_walk(&state.loop, &::echoClose, nullptr);
		/**
		 * Осушаем цикл, включая сброс uv_stop после отказа во время подготовки
		 */
		while(::uv_loop_alive(&state.loop))
			// Доставляем отменённые запросы и закрытия до разрушения их памяти
			::uv_run(&state.loop, UV_RUN_DEFAULT);
		// Проверяем полное освобождение цикла
		if(::uv_loop_close(&state.loop) != 0)
			// Не публикуем результат с незавершёнными ресурсами
			state.failure = "loop-close-failed";
		// Если все проверки пройдены
		if(state.failure == nullptr){
			// Устанавливаем точное число обменов
			result.operations = state.done;
			// Устанавливаем объём данных в обоих направлениях
			result.bytes = (state.done * ECHO_PAYLOAD * 2);
			// Используем только сохранённое измеряемое окно
			result.seconds = elapsed(state.start, state.finish);
		} else
			// Сообщаем причину отказа вне измеряемого окна
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
		uv_udp_t handle;
		// Состояние прогона сценария
		echo_t * state;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Datagram() noexcept : handle{}, state(nullptr) {}
	} datagram_t;

	// Признак того, что датаграммный прогон упёрся в предельный срок
	static bool gExpired = false;

	/**
	 * @brief Функция обратного вызова приёма датаграммы приёмником
	 *
	 * @details Приёмник обслуживает ВСЕХ отправителей одним дескриптором, поэтому
	 *          адрес отправителя приходит отдельным доводом и ответ уходит по нему же
	 *
	 * @note Отправка выполняется через `uv_udp_try_send` намеренно. Обычный
	 *       `uv_udp_send` требует ЗАПРОСА на каждую отправку, а у приёмника их
	 *       разом столько, сколько отправителей, и запрос пришлось бы либо
	 *       выделять на каждый ответ, либо переиспользовать занятый. Первое
	 *       внесло бы в замер выделения памяти, которых нет у прочих стендов,
	 *       второе - неопределённое поведение
	 *
	 * @param handle дескриптор приёмника датаграмм
	 * @param size   размер принятой датаграммы
	 * @param buffer буфер принятой датаграммы
	 * @param source адрес отправителя датаграммы
	 *
	 */
	static void receiverRead(uv_udp_t * handle, ssize_t size, const uv_buf_t * buffer, const struct sockaddr * source, uint32_t) noexcept {
		// Если датаграмма не принята
		if((size <= 0) || (source == nullptr))
			// Ожидаем следующего приёма
			return;
		// Формируем блок возвращаемых данных
		uv_buf_t response = ::uv_buf_init(buffer->base, static_cast <uint32_t> (size));
		// Возвращаем принятые октеты отправителю
		::uv_udp_try_send(handle, &response, 1, source);
	}
	/**
	 * @brief Функция обратного вызова приёма датаграммы отправителем
	 *
	 * @note Накопления принятых октетов здесь нет намеренно: датаграмма приходит
	 *       целиком, и частично принятого сообщения не бывает
	 *
	 * @param handle дескриптор отправителя датаграмм
	 * @param size   размер принятой датаграммы
	 *
	 */
	static void emitterRead(uv_udp_t * handle, ssize_t size, const uv_buf_t *, const struct sockaddr *, uint32_t) noexcept {
		// Если датаграмма не принята
		if(size <= 0)
			// Ожидаем следующего приёма
			return;
		// Получаем состояние отправителя
		datagram_t * datagram = reinterpret_cast <datagram_t *> (handle->data);
		// Если обмен следует продолжать
		if(datagram->state->account()){
			// Формируем блок отправляемых данных
			uv_buf_t payload = ::uv_buf_init(reinterpret_cast <char *> (gPayload), ECHO_PAYLOAD);
			// Отправляем следующую датаграмму
			::uv_udp_try_send(handle, &payload, 1, nullptr);
		// Останавливаем цикл событий
		} else ::uv_stop(handle->loop);
	}
	/**
	 * @brief Функция обратного вызова истечения предельного срока прогона
	 *
	 * @param timer наблюдатель предельного срока прогона
	 *
	 */
	static void expire(uv_timer_t * timer) noexcept {
		// Отмечаем прогон упёршимся в предельный срок
		gExpired = true;
		// Останавливаем цикл событий
		::uv_stop(timer->loop);
	}
	/**
	 * @brief Функция прогона сценария датаграммного обмена
	 *
	 * @details Постановка повторяет сценарий обмена короткими сообщениями во всём,
	 *          кроме переноса: та же нагрузка, то же количество обменов, тот же
	 *          учёт границ прогрева и замера. Только так разница между итогами
	 *          выражает цену переноса, а не разницу постановок
	 *
	 * @note Датаграммы здесь идут через СОБСТВЕННЫЙ дескриптор библиотеки
	 *       `uv_udp_t`, а не через сырой сокет, - ровно как потоковые сценарии
	 *       этого стенда идут через `uv_tcp_t`. Взять здесь сырой сокет значило бы
	 *       мерить у libuv не то, чем она пользуется на деле
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
		// Цикл событий стенда
		uv_loop_t loop{};
		// Выполняем инициализацию цикла событий стенда
		::uv_loop_init(&loop);
		// Параметры привязки приёмника датаграмм
		struct sockaddr_in address{};
		// Формируем адрес привязки приёмника датаграмм
		::uv_ip4_addr("127.0.0.1", 0, &address);
		// Дескриптор приёмника датаграмм
		uv_udp_t server{};
		// Выполняем инициализацию дескриптора приёмника датаграмм
		::uv_udp_init(&loop, &server);
		// Выполняем привязку приёмника датаграмм
		::uv_udp_bind(&server, reinterpret_cast <const struct sockaddr *> (&address), 0);
		// Требуемый объём приёмного буфера сокета
		int32_t capacity = DATAGRAM_BUFFER;
		// Расширяем приёмный буфер приёмника датаграмм
		::uv_recv_buffer_size(reinterpret_cast <uv_handle_t *> (&server), &capacity);
		// Размер структуры параметров сокета
		int32_t length = sizeof(address);
		// Извлекаем параметры привязки приёмника датаграмм
		::uv_udp_getsockname(&server, reinterpret_cast <struct sockaddr *> (&address), &length);
		// Запускаем приём датаграмм приёмником
		::uv_udp_recv_start(&server, &::allocate, &::receiverRead);
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
			// Устанавливаем состояние прогона сценария
			datagram->state = &state;
			// Выполняем инициализацию дескриптора отправителя
			::uv_udp_init(&loop, &datagram->handle);
			// Устанавливаем состояние отправителя дескриптору
			datagram->handle.data = datagram;
			// Требуемый объём приёмного буфера сокета
			int32_t size = DATAGRAM_BUFFER;
			// Расширяем приёмный буфер отправителя датаграмм
			::uv_recv_buffer_size(reinterpret_cast <uv_handle_t *> (&datagram->handle), &size);
			// Закрепляем за отправителем адрес приёмника датаграмм
			::uv_udp_connect(&datagram->handle, reinterpret_cast <const struct sockaddr *> (&address));
			// Запускаем приём датаграмм отправителем
			::uv_udp_recv_start(&datagram->handle, &::allocate, &::emitterRead);
			// Формируем блок отправляемых данных
			uv_buf_t payload = ::uv_buf_init(reinterpret_cast <char *> (gPayload), ECHO_PAYLOAD);
			// Отправляем первую датаграмму, начиная обмен
			::uv_udp_try_send(&datagram->handle, &payload, 1, nullptr);
		}
		// Наблюдатель предельного срока прогона
		uv_timer_t deadline{};
		// Выполняем инициализацию наблюдателя предельного срока прогона
		::uv_timer_init(&loop, &deadline);
		// Активируем наблюдатель предельного срока прогона
		::uv_timer_start(&deadline, &::expire, (static_cast <uint64_t> (DATAGRAM_DEADLINE) * 1000), 0);
		/**
		 * Запускаем цикл событий до выполнения требуемого количества обменов
		 */
		::uv_run(&loop, UV_RUN_DEFAULT);
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
		 * Выполняем остановку отправителей датаграмм
		 */
		for(auto & datagram : clients)
			// Останавливаем приём датаграмм отправителем
			::uv_udp_recv_stop(&datagram->handle);
		// Останавливаем приём датаграмм приёмником
		::uv_udp_recv_stop(&server);
		// Освобождаем цикл событий стенда
		::uv_loop_close(&loop);
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
		uv_os_fd_t fd{};
		// Если дескриптор сокета подключения получен
		if(::uv_fileno(reinterpret_cast <uv_handle_t *> (&connection->handle), &fd) == 0)
			// Включаем немедленный обрыв соединения при закрытии сокета
			hardClose(::native(fd));
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
	 * @brief Предельное время подготовки или передачи в секундах
	 *
	 */
	static constexpr double STREAM_DEADLINE = 30.0;
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
	 * @brief Функция подтверждения записи текущего блока
	 *
	 */
	static void streamWrite(uv_write_t * request, int32_t status) noexcept;
	/**
	 * @brief Функция постановки одного блока после записи предыдущего
	 *
	 * @param state состояние потока
	 *
	 */
	static void streamQueue(stream_t & state) noexcept {
		// Не подаём лишний блок или блок после отказа
		if((state.failure != nullptr) || (state.queued == STREAM_VOLUME))
			// Завершаем постановку
			return;
		// Устанавливаем размер единственного незавершённого блока
		state.pending = STREAM_CHUNK;
		// Учитываем поставленный блок до вызова библиотеки
		state.queued += STREAM_CHUNK;
		// Формируем описание неизменяемого блока
		uv_buf_t chunk = ::uv_buf_init(reinterpret_cast <char *> (gChunk.data()), STREAM_CHUNK);
		// Проверяем постановку запроса записи
		if(::uv_write(&state.request, reinterpret_cast <uv_stream_t *> (state.sender), &chunk, 1, &::streamWrite) != 0)
			// Не ждём callback для отвергнутого запроса
			state.failure = "write-start";
	}
	/**
	 * @brief Функция подтверждения записи текущего блока
	 *
	 * @param request завершённый запрос
	 * @param status  фактический исход записи
	 *
	 */
	static void streamWrite(uv_write_t * request, int32_t status) noexcept {
		// Получаем состояние потока
		stream_t & state = *reinterpret_cast <stream_t *> (request->data);
		// Допускаем ожидаемую отмену только при завершающей очистке
		if(state.closing || (state.failure != nullptr))
			// Не подаём новые данные
			return;
		// Не засчитываем ошибку записи как отправленный блок
		if((status != 0) || (request != &state.request) || (state.pending != STREAM_CHUNK)){
			// Сохраняем отказ подтверждения
			state.failure = "write-failed";
			// Завершаем обработку
			return;
		}
		// Учитываем полностью записанный блок
		state.written += state.pending;
		// Снимаем остаток завершённой записи
		state.pending = 0;
		// Подаём следующий блок только после текущего callback
		::streamQueue(state);
		// Проверяем завершение обеих сторон
		::streamComplete(state);
	}
	/**
	 * @brief Функция приёма потока с проверкой EOF и ошибок
	 *
	 * @param stream дескриптор приёмника
	 * @param size   исход чтения
	 *
	 */
	static void streamRead(uv_stream_t * stream, ssize_t size, const uv_buf_t *) noexcept {
		// Получаем состояние потока
		stream_t & state = *reinterpret_cast <stream_t *> (stream->data);
		// Нулевая готовность libuv не означает EOF
		if(state.closing || (state.failure != nullptr) || (size == 0))
			// Не меняем счётчики
			return;
		// Отбрасываем EOF, ошибку, преждевременные или избыточные данные
		if(!state.running || (size < 0) || (static_cast <size_t> (size) > (STREAM_VOLUME - state.received))){
			// Прерываем незавершённый поток
			state.failure = "read-size-or-eof";
			// Завершаем обработку
			return;
		}
		// Учитываем фактически принятый объём
		state.received += static_cast <size_t> (size);
		// Проверяем завершение обеих сторон
		::streamComplete(state);
	}
	/**
	 * @brief Функция подготовки принятой стороны потока
	 *
	 * @param server слушающий дескриптор
	 * @param status исход уведомления
	 *
	 */
	static void streamAccept(uv_stream_t * server, int32_t status) noexcept {
		// Получаем состояние потока
		stream_t & state = *reinterpret_cast <stream_t *> (server->data);
		// Отбрасываем ошибку и лишнее подключение
		if((status != 0) || (state.peer != nullptr) || (state.failure != nullptr)){
			// Сохраняем первый отказ
			if(state.failure == nullptr)
				// Не начинаем поток с неверным составом
				state.failure = "accept-status";
			// Завершаем обработку
			return;
		}
		// Выделяем дескриптор приёмника
		uv_tcp_t * peer = new (nothrow) uv_tcp_t;
		// Проверяем инициализацию перед обязательным закрытием
		if((peer == nullptr) || (::uv_tcp_init(state.loop, peer) != 0)){
			// Освобождаем неинициализированный объект
			delete peer;
			// Запоминаем отказ подготовки
			state.failure = "peer-init";
			// Завершаем обработку
			return;
		}
		// Сохраняем приёмник для завершающего закрытия
		state.peer = peer;
		// Связываем приёмник с состоянием
		peer->data = &state;
		// Проверяем приём и подписку на чтение до начала измерения
		if((::uv_accept(server, reinterpret_cast <uv_stream_t *> (peer)) != 0) || (::uv_tcp_nodelay(peer, 1) != 0) || (::uv_read_start(reinterpret_cast <uv_stream_t *> (peer), &::allocate, &::streamRead) != 0))
			// Прерываем неполную подготовку
			state.failure = "peer-registration";
	}
	/**
	 * @brief Функция проверки успешного подключения клиента
	 *
	 * @param request запрос подключения
	 * @param status  фактический исход подключения
	 *
	 */
	static void streamConnect(uv_connect_t * request, int32_t status) noexcept {
		// Получаем состояние потока
		stream_t & state = *reinterpret_cast <stream_t *> (request->data);
		// Не меняем первый отказ при отмене во время очистки
		if(state.closing || (state.failure != nullptr))
			// Завершаем обработку
			return;
		// Проверяем исход подключения
		if(status != 0){
			// Не оставляем отказавшее подключение в ожидании
			state.failure = "connect-failed";
			// Завершаем обработку
			return;
		}
		// Получаем системный дескриптор клиента
		uv_os_fd_t fd{};
		// Настройки немедленного закрытия
		struct linger value{};
		// Не оставляем порт в TIME_WAIT
		value.l_onoff = 1;
		// Проверяем настройку сокета
		if((::uv_fileno(reinterpret_cast <uv_handle_t *> (state.sender), &fd) != 0) || (::setsockopt(::native(fd), SOL_SOCKET, SO_LINGER, raw(&value), sizeof(value)) != 0)){
			// Запоминаем отказ настройки
			state.failure = "client-linger";
			// Завершаем обработку
			return;
		}
		// Подтверждаем клиентскую сторону без запуска передачи
		state.ready = true;
	}
	/**
	 * @brief Функция периодического возврата для проверки срока
	 *
	 */
	static void streamTick(uv_timer_t *) noexcept {}
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
		// Отдельный цикл событий
		uv_loop_t loop{};
		// Проверяем инициализацию цикла
		if(::uv_loop_init(&loop) != 0){
			// Сообщаем отказ без частичного результата
			::fprintf(stderr, "STREAM_BENCH_FAIL: loop-init\n");
			// Возвращаем пустой исход
			return result;
		}
		// Связываем состояние с циклом
		state.loop = &loop;
		// Создаём дескриптор сервера
		uv_tcp_t server{};
		// Создаём дескриптор клиента
		uv_tcp_t sender{};
		// Создаём периодический сторож
		uv_timer_t timer{};
		// Создаём запрос подключения
		uv_connect_t connection{};
		// Сохраняем успешность инициализации сервера для очистки
		const bool serverInitialized = (::uv_tcp_init(&loop, &server) == 0);
		// Сохраняем успешность инициализации клиента для очистки
		const bool senderInitialized = (::uv_tcp_init(&loop, &sender) == 0);
		// Сохраняем успешность инициализации таймера для очистки
		const bool timerInitialized = (::uv_timer_init(&loop, &timer) == 0);
		// Связываем сервер с состоянием
		server.data = &state;
		// Связываем клиента с состоянием
		sender.data = &state;
		// Сохраняем передатчик для очереди блоков
		state.sender = &sender;
		// Связываем запрос подключения с состоянием
		connection.data = &state;
		// Связываем запрос записи с состоянием
		state.request.data = &state;
		// Параметры loopback-подключения
		struct sockaddr_in address{};
		// Размер адреса для получения порта
		int32_t length = sizeof(address);
		// Проверяем все операции подготовки сервера, клиента и сторожа
		if(!serverInitialized || !senderInitialized || !timerInitialized || (::uv_ip4_addr("127.0.0.1", 0, &address) != 0) || (::uv_tcp_bind(&server, reinterpret_cast <struct sockaddr *> (&address), 0) != 0) || (::uv_tcp_getsockname(&server, reinterpret_cast <struct sockaddr *> (&address), &length) != 0) || (::uv_listen(reinterpret_cast <uv_stream_t *> (&server), BACKLOG, &::streamAccept) != 0) || (::uv_tcp_nodelay(&sender, 1) != 0) || (::uv_timer_start(&timer, &::streamTick, 250, 250) != 0))
			// Запоминаем отказ подготовки
			state.failure = "setup-failed";
		// Запускаем подключение только после успешной подготовки
		if((state.failure == nullptr) && (::uv_tcp_connect(&connection, &sender, reinterpret_cast <const struct sockaddr *> (&address), &::streamConnect) != 0))
			// Не ждём callback для отвергнутого запроса
			state.failure = "connect-start";
		// Запоминаем начало ожидания подготовки
		const auto prepared = now();
		/**
		 * Дожидаемся обеих готовых сторон до начала измерения
		 */
		while((state.failure == nullptr) && (!state.ready || (state.peer == nullptr))){
			// Проверяем продолжение цикла событий
			if(::uv_run(&loop, UV_RUN_ONCE) == 0)
				// Запоминаем неожиданный конец цикла
				state.failure = "setup-poll";
			// Ограничиваем время подготовки
			if(elapsed(prepared, now()) >= STREAM_DEADLINE)
				// Прерываем незавершённое подключение
				state.failure = "setup-deadline";
		}
		// Начинаем передачу после полной подготовки
		if(state.failure == nullptr){
			// Разрешаем обработку данных
			state.running = true;
			// Открываем окно перед первой записью
			state.start = now();
			// Подаём первый блок
			::streamQueue(state);
		}
		/**
		 * Ожидаем полной записи и приёма с общим ограничением времени
		 */
		while((state.failure == nullptr) && !state.stop){
			// Продвигаем передачу конечным опросом
			if(::uv_run(&loop, UV_RUN_ONCE) == 0)
				// Запоминаем неожиданный конец цикла
				state.failure = "transfer-poll";
			// Ограничиваем время полного потока
			if(elapsed(state.start, now()) >= STREAM_DEADLINE)
				// Прерываем зависший поток
				state.failure = "transfer-deadline";
		}
		// Разрешаем отменённые callbacks только на стадии очистки
		state.closing = true;
		// Закрываем принятый дескриптор с освобождением после callback
		if(state.peer != nullptr)
			// Не удаляем объект раньше фактического закрытия
			::uv_close(reinterpret_cast <uv_handle_t *> (state.peer), &::release);
		// Закрываем инициализированного клиента
		if(senderInitialized)
			// Отменяем также незавершённые запросы записи и подключения
			::uv_close(reinterpret_cast <uv_handle_t *> (&sender), nullptr);
		// Закрываем инициализированный сервер
		if(serverInitialized)
			// Отменяем приём новых соединений
			::uv_close(reinterpret_cast <uv_handle_t *> (&server), nullptr);
		// Закрываем инициализированный таймер
		if(timerInitialized)
			// Удаляем сторож ожидания
			::uv_close(reinterpret_cast <uv_handle_t *> (&timer), nullptr);
		// Дожидаемся отменённых запросов и всех закрытий до разрушения состояния
		::uv_run(&loop, UV_RUN_DEFAULT);
		// Проверяем освобождение самого цикла
		if((::uv_loop_close(&loop) != 0) && (state.failure == nullptr))
			// Не скрываем незавершённую очистку
			state.failure = "loop-close";
		// Проверяем полноту обеих сторон и корректное окно
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
	 * @brief Функция обработки неожиданного чтения принятого подключения
	 *
	 * @param stream дескриптор принятой стороны
	 * @param size   исход чтения
	 *
	 */
	static void handshakeRead(uv_stream_t * stream, ssize_t size, const uv_buf_t *) noexcept {
		// Получаем состояние сценария
		handshake_t & state = *reinterpret_cast <handshake_t *> (stream->data);
		// Нулевая готовность допустима, данные и EOF до закрытия не предусмотрены
		if((size != 0) && (state.failure == nullptr))
			// Отмечаем преждевременное событие принятой стороны
			state.failure = "unexpected-peer-read";
	}
	/**
	 * @brief Функция принятия серверной стороны текущего цикла
	 *
	 * @param server слушающий дескриптор
	 * @param status исход уведомления
	 *
	 */
	static void handshakeAccept(uv_stream_t * server, int32_t status) noexcept {
		// Получаем состояние сценария
		handshake_t & state = *reinterpret_cast <handshake_t *> (server->data);
		// Не допускаем ошибку, повторный приём и приём вне активного цикла
		if((status < 0) || (state.peer != nullptr) || (state.handle == nullptr) || (state.failure != nullptr)){
			// Сохраняем первый отказ
			if(state.failure == nullptr)
				// Отмечаем нарушение приёма
				state.failure = "accept-status";
			// Завершаем обработку
			return;
		}
		// Выделяем дескриптор принятого подключения
		uv_tcp_t * peer = new (nothrow) uv_tcp_t;
		// Проверяем создание и инициализацию дескриптора
		if((peer == nullptr) || (::uv_tcp_init(state.loop, peer) != 0)){
			// Освобождаем неинициализированный объект
			delete peer;
			// Запоминаем отказ подготовки
			state.failure = "peer-init";
			// Завершаем обработку
			return;
		}
		// Сохраняем инициализированный объект для обязательного закрытия
		state.peer = peer;
		// Связываем принятую сторону с состоянием сценария
		peer->data = &state;
		// Проверяем приём, настройку и реальную подписку на чтение
		if((::uv_accept(server, reinterpret_cast <uv_stream_t *> (peer)) != 0) || (::uv_tcp_nodelay(peer, 1) != 0) || (::uv_read_start(reinterpret_cast <uv_stream_t *> (peer), &::allocate, &::handshakeRead) != 0)){
			// Не засчитываем неподготовленную серверную сторону
			state.failure = "peer-registration";
			// Завершаем обработку
			return;
		}
		// Подтверждаем серверную сторону
		state.accepted++;
	}
	/**
	 * @brief Функция подтверждения фактического закрытия одной стороны
	 *
	 * @param handle закрытый дескриптор
	 *
	 */
	static void handshakeClosed(uv_handle_t * handle) noexcept {
		// Получаем состояние сценария
		handshake_t & state = *reinterpret_cast <handshake_t *> (handle->data);
		// Снимаем ссылку на закрытый клиент
		if(handle == reinterpret_cast <uv_handle_t *> (state.handle))
			// Подтверждаем закрытие клиентской стороны
			state.handle = nullptr;
		// Снимаем ссылку на закрытый peer
		else if(handle == reinterpret_cast <uv_handle_t *> (state.peer))
			// Подтверждаем закрытие серверной стороны
			state.peer = nullptr;
		// Не допускаем счёт чужого дескриптора
		else {
			// Сохраняем первый отказ
			if(state.failure == nullptr)
				// Запоминаем нарушение принадлежности
				state.failure = "unexpected-close";
			// Завершаем обработку неизвестного объекта
			return;
		}
		// Освобождаем объект только после close callback
		delete reinterpret_cast <uv_tcp_t *> (handle);
		// Учитываем реально завершённое закрытие
		state.closed++;
	}
	/**
	 * @brief Функция проверки исхода подключения клиента
	 *
	 * @param request запрос подключения
	 * @param status  фактический исход подключения
	 *
	 */
	static void handshakeConnect(uv_connect_t * request, int32_t status) noexcept {
		// Получаем состояние сценария
		handshake_t & state = *reinterpret_cast <handshake_t *> (request->data);
		// Проверяем успешность и принадлежность текущему циклу
		if((status != 0) || (request != state.request) || state.ready){
			// Сохраняем первый отказ, включая отмену при очистке
			if(state.failure == nullptr)
				// Не засчитываем отказ подключения
				state.failure = "connect-failed";
			// Завершаем обработку
			return;
		}
		// Получаем системный дескриптор клиента
		uv_os_fd_t fd{};
		// Параметры немедленного закрытия
		struct linger value{};
		// Не оставляем порт в TIME_WAIT
		value.l_onoff = 1;
		// Проверяем получение сокета и применение опции
		if((::uv_fileno(reinterpret_cast <uv_handle_t *> (state.handle), &fd) != 0) || (::setsockopt(::native(fd), SOL_SOCKET, SO_LINGER, raw(&value), sizeof(value)) != 0)){
			// Запоминаем отказ настройки закрытия
			state.failure = "client-linger";
			// Завершаем обработку
			return;
		}
		// Подтверждаем клиентскую сторону
		state.ready = true;
		// Учитываем успешное подключение отдельно от приёма
		state.connected++;
	}
	/**
	 * @brief Функция периодического возврата из ожидания для проверки срока
	 *
	 */
	static void handshakeTick(uv_timer_t *) noexcept {}
	/**
	 * @brief Функция запроса закрытия обеих сторон текущего цикла
	 *
	 * @param state состояние сценария
	 *
	 */
	static void handshakeClose(handshake_t & state) noexcept {
		/**
		 * Закрываем все созданные стороны, не освобождая объекты до callback
		 */
		for(uv_tcp_t * peer : {state.handle, state.peer}){
			// Пропускаем несозданные и уже закрываемые объекты
			if((peer == nullptr) || ::uv_is_closing(reinterpret_cast <uv_handle_t *> (peer)))
				// Продолжаем закрытие остальных сторон
				continue;
			// Передаём освобождение подтверждающему callback
			::uv_close(reinterpret_cast <uv_handle_t *> (peer), &::handshakeClosed);
		}
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
		// Состояние обеих сторон и счётчики
		handshake_t state;
		// Отдельный цикл событий сценария
		uv_loop_t loop{};
		// Проверяем создание цикла
		if(::uv_loop_init(&loop) != 0){
			// Сообщаем отказ подготовки
			::fprintf(stderr, "ACCEPT_BENCH_FAIL: loop-init\n");
			// Возвращаем пустой результат
			return result;
		}
		// Связываем состояние с циклом
		state.loop = &loop;
		// Слушающий дескриптор
		uv_tcp_t server{};
		// Общий сторож ожидания
		uv_timer_t timer{};
		// Проверяем и запоминаем инициализацию сервера для очистки
		const bool serverInitialized = (::uv_tcp_init(&loop, &server) == 0);
		// Проверяем и запоминаем инициализацию таймера для очистки
		const bool timerInitialized = (::uv_timer_init(&loop, &timer) == 0);
		// Связываем слушающий дескриптор с состоянием
		server.data = &state;
		// Размер адреса для получения назначенного порта
		int32_t length = sizeof(state.address);
		// Проверяем все операции подготовки сервера и сторожа
		if(!serverInitialized || !timerInitialized || (::uv_ip4_addr("127.0.0.1", 0, &state.address) != 0) || (::uv_tcp_bind(&server, reinterpret_cast <const struct sockaddr *> (&state.address), 0) != 0) || (::uv_tcp_getsockname(&server, reinterpret_cast <struct sockaddr *> (&state.address), &length) != 0) || (::uv_listen(reinterpret_cast <uv_stream_t *> (&server), BACKLOG, &::handshakeAccept) != 0) || (::uv_timer_start(&timer, &::handshakeTick, 250, 250) != 0))
			// Запоминаем отказ подготовки
			state.failure = "server-setup";
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
			// Устанавливаем общий срок подключения и закрытия
			const auto tick = now();
			// Выделяем очередной клиентский дескриптор
			uv_tcp_t * handle = new (nothrow) uv_tcp_t;
			// Проверяем создание и инициализацию клиента
			if((handle == nullptr) || (::uv_tcp_init(&loop, handle) != 0)){
				// Освобождаем неинициализированный объект
				delete handle;
				// Запоминаем отказ подготовки
				state.failure = "client-init";
				// Прекращаем прогон
				break;
			}
			// Сохраняем инициализированный объект для обязательного закрытия
			state.handle = handle;
			// Связываем клиента с состоянием сценария
			handle->data = &state;
			// Выделяем запрос текущего подключения
			state.request = new (nothrow) uv_connect_t;
			// Проверяем запрос и обязательные опции клиента
			if((state.request == nullptr) || (::uv_tcp_nodelay(handle, 1) != 0)){
				// Запоминаем отказ подготовки
				state.failure = "client-setup";
				// Прекращаем прогон
				break;
			}
			// Связываем запрос с состоянием
			state.request->data = &state;
			// Проверяем начало подключения
			if(::uv_tcp_connect(state.request, handle, reinterpret_cast <const struct sockaddr *> (&state.address), &::handshakeConnect) != 0)
				// Запоминаем немедленный отказ
				state.failure = "connect-start";
			/**
			 * Ожидаем подтверждения обеих сторон текущего цикла
			 */
			while((state.failure == nullptr) && (!state.ready || (state.accepted != (round + 1)))){
				// Проверяем наличие активного цикла событий
				if(::uv_run(&loop, UV_RUN_ONCE) == 0)
					// Запоминаем неожиданное завершение цикла
					state.failure = "poll-failed";
				// Ограничиваем длительность текущего цикла
				if(elapsed(tick, now()) >= 5.0)
					// Запоминаем превышение срока
					state.failure = "round-deadline";
			}
			// Не засчитываем ошибочный цикл
			if(state.failure != nullptr)
				// Прекращаем прогон
				break;
			// Запрашиваем закрытие обеих сторон одновременно
			::handshakeClose(state);
			/**
			 * Дожидаемся обоих close callback до освобождения запроса и следующего цикла
			 */
			while((state.failure == nullptr) && ((state.handle != nullptr) || (state.peer != nullptr))){
				// Продвигаем обработку отложенных закрытий
				::uv_run(&loop, UV_RUN_ONCE);
				// Ограничиваем также стадию освобождения
				if(elapsed(tick, now()) >= 5.0)
					// Запоминаем превышение срока
					state.failure = "close-deadline";
			}
			// Не учитываем неполное закрытие
			if(state.failure != nullptr)
				// Прекращаем прогон
				break;
			// Освобождаем завершённый запрос подключения
			delete state.request;
			// Снимаем указатель на освобождённый запрос
			state.request = nullptr;
			// Учитываем полный цикл прогрева
			if(round < ACCEPT_WARMUP)
				// Увеличиваем счётчик прогрева
				state.warmed++;
			// Учитываем полный измеренный цикл
			else state.done++;
		}
		// Закрываем окно после обоих закрытий последнего цикла
		const auto finish = now();
		// Закрываем остатки ошибочного цикла
		::handshakeClose(state);
		// Закрываем сервер только после успешной инициализации
		if(serverInitialized)
			// Отменяем приём новых подключений
			::uv_close(reinterpret_cast <uv_handle_t *> (&server), nullptr);
		// Закрываем сторож только после успешной инициализации
		if(timerInitialized)
			// Отменяем периодический возврат
			::uv_close(reinterpret_cast <uv_handle_t *> (&timer), nullptr);
		// Дожидаемся завершения отменённых запросов и всех close callback
		::uv_run(&loop, UV_RUN_DEFAULT);
		// Освобождаем оставшийся запрос после закрытия клиента
		delete state.request;
		// Снимаем указатель на освобождённый запрос
		state.request = nullptr;
		// Проверяем освобождение самого цикла событий
		if((::uv_loop_close(&loop) != 0) && (state.failure == nullptr))
			// Запоминаем незавершённую очистку
			state.failure = "loop-close";
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
		const outcome_t outcome = ::echoExchange(1, ECHO_SINGLE_ROUNDS);
		// Не публикуем неполный или недействительный прогон
		if((outcome.operations != ECHO_SINGLE_ROUNDS) || (outcome.seconds <= 0.0))
			// Сообщаем вызывающей обвязке об отказе
			return EXIT_FAILURE;
		// Выводим результат прогона сценария
		report("net/io/echo/single", "обменов/с", perSecond(outcome), outcome);
	}
	// Если сценарий обмена на множестве подключений выполняется
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
	// Если сценарий обмена среди наблюдаемых подключений выполняется
	if(selected("net/io/idle/exchanges", name)){
		// Выполняем прогон сценария обмена при множестве наблюдаемых подключений
		const outcome_t outcome = ::watched();
		// Выводим результат прогона сценария
		report("net/io/idle/exchanges", "обменов/с", perSecond(outcome), outcome);
	}
	// Если сценарий обмена множеством подключений выполняется
	if(selected("net/io/echo/multi", name)){
		// Выполняем прогон сценария обмена на множестве подключений
		const outcome_t outcome = ::echoExchange(ECHO_MULTI_CONNECTIONS, ECHO_MULTI_ROUNDS);
		// Не публикуем неполный или недействительный прогон
		if((outcome.operations != ECHO_MULTI_ROUNDS) || (outcome.seconds <= 0.0))
			// Сообщаем вызывающей обвязке об отказе
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
