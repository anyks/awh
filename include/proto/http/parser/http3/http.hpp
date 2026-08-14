/**
 * @file: http.hpp
 * @date: 2026-07-27
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл парсера сессии HTTP/3 (RFC 9114) — класс Parser_HTTP3, управляющий
 *        потоками запросов, однонаправленными потоками соединения, параметрами SETTINGS,
 *        состоянием кодека QPACK, приоритетами и лимитами безопасности
 *
 * \~english
 * @brief Header file of the parser of an HTTP/3 session (RFC 9114) — the class Parser_HTTP3 controlling
 *        the streams of the requests, the unidirectional streams of the connection, the parameters of SETTINGS,
 *        the state of the QPACK codec, the priorities and the limits of the safety
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_HTTP_PARSER_HTTP3__
#define __AWH_HTTP_PARSER_HTTP3__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <string_view>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "h3.hpp"
#include "frame.hpp"
#include "qpack.hpp"
#include "../parser.hpp"
#include "../../headers.hpp"
#include "../../provider.hpp"
#include "../../../../sys/global.hpp"

/**
 * \~russian
 * @brief основное пространство имён
 *
 *
 * \~english
 * @brief main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён HTTP-протокола
	 *
	 *
	 * \~english
	 * @brief HTTP protocol namespace
	 *
	 * \~
	 */
	namespace http {
		/**
		 * \~russian
		 * @brief Класс парсера протокола HTTP/3 (RFC 9114)
		 *
		 * @details Как и парсер HTTP/2, работает на уровне СОЕДИНЕНИЯ, а не одного сообщения,
		 *          поэтому все события сопровождаются идентификатором потока. На этом сходство
		 *          заканчивается: HTTP/3 переносит на транспорт почти всё, что HTTP/2 делал сам.
		 *
		 *          Чего у этого парсера нет по сравнению с HTTP/2 и почему:
		 *          - **управления потоком**: окна ведёт QUIC, кадра WINDOW_UPDATE в HTTP/3 нет;
		 *          - **кадра RST_STREAM**: поток обрывается кадрами RESET_STREAM и STOP_SENDING
		 *            транспорта, поэтому парсер просит об этом обвязку, а не пишет кадр сам;
		 *          - **кадра PING**: проверка живости - забота транспорта;
		 *          - **кадра CONTINUATION**: длина кадра не ограничена, дробить секцию полей
		 *            не нужно, и целого класса атак CONTINUATION flood здесь не существует;
		 *          - **лимита одновременных потоков в SETTINGS**: его задаёт транспортный
		 *            параметр initial_max_streams_bidi;
		 *          - **своего кадра сообщения об ошибке**: код ошибки уходит в CONNECTION_CLOSE.
		 *
		 *          Что появилось взамен:
		 *          - **однонаправленные потоки**: управляющий, два потока QPACK и потоки push.
		 *            Первым целым переменной длины в таком потоке идёт его тип. Управляющий поток
		 *            и потоки QPACK обязаны жить всё соединение: их закрытие - ошибка соединения;
		 *          - **разбор с адресацией по потокам**: вход - parse(sid, ...), а не parse(...),
		 *            потому что единого байтового потока у соединения больше нет;
		 *          - **заблокированные потоки**: секция полей и инструкции QPACK идут разными
		 *            потоками и обгоняют друг друга, поэтому секция, пришедшая раньше нужных
		 *            вставок, откладывается и разбирается заново после их прихода.
		 *
		 *          Направление трафика задаёт роль эндпоинта:
		 *          - direct_t::REQUEST  - разбираем запросы клиента (мы - сервер);
		 *          - direct_t::RESPONSE - разбираем ответы сервера (мы - клиент).
		 *
		 * @note Парсер не зависит от транспорта: он разбирает байты потоков и формирует байты
		 *       потоков. Всё, что связано с QUIC, TLS и таймерами, остаётся за обвязкой.
		 *       Операции, которые парсер выполнить не может - открыть однонаправленный поток,
		 *       оборвать поток, закрыть соединение, - запрашиваются функциями обратного вызова
		 *
		 * \~english
		 * @brief Class of the parser of the HTTP/3 protocol (RFC 9114)
		 * @details Like the parser of HTTP/2, it works at the level of a CONNECTION rather than of a single message,
		 *          therefore all the events are accompanied by an identifier of the stream. Here the similarity
		 *          ends: HTTP/3 carries over onto the transport almost everything that HTTP/2 did itself.
		 *          What this parser has not in comparison with HTTP/2 and why:
		 *          - **the flow control**: the windows are conducted by QUIC, there is no WINDOW_UPDATE frame in HTTP/3;
		 *          - **the RST_STREAM frame**: a stream is broken by the frames RESET_STREAM and STOP_SENDING
		 *            of the transport, therefore the parser asks the binding about this rather than writing the frame itself;
		 *          - **the PING frame**: the check of the liveness is a business of the transport;
		 *          - **the CONTINUATION frame**: the length of a frame is not limited, to split a section of the fields
		 *            is not needed, and a whole class of the attacks CONTINUATION flood does not exist here;
		 *          - **the limit of the simultaneous streams in SETTINGS**: it is set by the transport
		 *            parameter initial_max_streams_bidi;
		 *          - **its own frame of a report about an error**: the error code goes away into a CONNECTION_CLOSE.
		 *          What has appeared instead:
		 *          - **the unidirectional streams**: the control one, two streams of QPACK and the streams of a push.
		 *            The first integer of a variable length in such a stream is its type. The control stream
		 *            and the streams of QPACK are obliged to live the whole connection: their closing is an error of the connection;
		 *          - **the parsing with an addressing by the streams**: the input is parse(sid, ...) rather than parse(...),
		 *            because the connection no longer has a single octet stream;
		 *          - **the blocked streams**: a section of the fields and the instructions of QPACK go by the different
		 *            streams and outstrip each other, therefore a section which has come earlier than the needed
		 *            insertions is postponed and is parsed anew after their arrival.
		 *          The direction of the traffic sets the role of the endpoint:
		 *          - direct_t::REQUEST  - we parse the requests of a client (we are a server);
		 *          - direct_t::RESPONSE - we parse the answers of a server (we are a client).
		 * @note The parser does not depend on the transport: it parses the octets of the streams and forms the octets
		 *       of the streams. Everything connected with QUIC, TLS and the timers remains at the binding.
		 *       The operations which the parser cannot perform - to open a unidirectional stream,
		 *       to break a stream, to close the connection, - are requested by the callback functions
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Parser_HTTP3 : public parser_t {
			public:
				/**
				 * \~russian
				 * @brief Пополнение лимита частоты управляющих кадров (токенов в секунду)
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 *
				 * \~english
				 * @brief Replenishment of the limit of the frequency of the control frames (tokens per second)
				 * @note The values by default are selected conservatively
				 *
				 * \~
				 */
				static constexpr uint64_t CTRL_LIMIT_RATE = (100);
				/**
				 * \~russian
				 * @brief Стартовый запас лимита частоты управляющих кадров
				 *
				 * \~english
				 * @brief Starting reserve of the limit of the frequency of the control frames
				 *
				 * \~
				 */
				static constexpr uint64_t CTRL_LIMIT_BURST = (1000);
				/**
				 * \~russian
				 * @brief Пополнение лимита частоты кадров приоритета (токенов в секунду)
				 *
				 * @note Лимит отдельный от управляющих кадров и заметно щедрее: клиент вправе
				 *       переставлять приоритеты на каждый загружаемый ресурс страницы
				 *
				 * \~english
				 * @brief Replenishment of the limit of the frequency of the frames of the priority (tokens per second)
				 * @note The limit is separate from the control frames and is noticeably more generous: a client is entitled
				 *       to rearrange the priorities per every loaded resource of a page
				 *
				 * \~
				 */
				static constexpr uint64_t PRIORITY_LIMIT_RATE = (500);
				/**
				 * \~russian
				 * @brief Стартовый запас лимита частоты кадров приоритета
				 *
				 * \~english
				 * @brief Starting reserve of the limit of the frequency of the frames of the priority
				 *
				 * \~
				 */
				static constexpr uint64_t PRIORITY_LIMIT_BURST = (5000);
				/**
				 * \~russian
				 * @brief Число запоминаемых идентификаторов push
				 *
				 * @details Окно учёта отменённых и уже пришедших обещаний. Оно же задаёт
				 *          глубину, на которой ловится повторный поток одного обещания
				 *
				 * \~english
				 * @brief Number of the remembered identifiers of a push
				 * @details The window of the account of the cancelled and of the already arrived promises. It also sets
				 *          the depth at which a repeated stream of one promise is caught
				 *
				 * \~
				 */
				static constexpr size_t PUSH_HISTORY_CACHE = (64);
				/**
				 * \~russian
				 * @brief Число запоминаемых приоритетов ещё не открытых потоков
				 *
				 * @details Кадр PRIORITY_UPDATE допустим для потока, который ещё не открыт
				 *          (RFC 9218 §7.2): сигнал приходит раньше секции полей и обязан
				 *          примениться при открытии потока. Состояния потока под такой
				 *          сигнал не создаётся, а число самих сигналов ограничено: иначе
				 *          пир наполнял бы память приоритетами потоков, которые
				 *          открывать не собирается
				 *
				 * \~english
				 * @brief Number of the remembered priorities of the not yet opened streams
				 * @details A PRIORITY_UPDATE frame is admissible for a stream which is not yet opened
				 *          (RFC 9218 §7.2): the signal comes earlier than a section of the fields and is obliged
				 *          to be applied at the opening of the stream. A state of a stream for such a
				 *          signal is not created, while the number of the signals themselves is limited: otherwise
				 *          a peer would fill the memory by the priorities of the streams which
				 *          it is not going to open
				 *
				 * \~
				 */
				static constexpr size_t PENDING_PRIORITIES_CACHE = (32);
			public:
				/**
				 * \~russian
				 * @brief Порог сигнала готовности потока принимать данные (low-water)
				 *
				 * \~english
				 * @brief Threshold of the signal of the readiness of a stream to accept the data (low-water)
				 *
				 * \~
				 */
				static constexpr size_t SEND_LOW_WATER = (64 * 1024);
				/**
				 * \~russian
				 * @brief Ёмкость буфера отправки потока (high-water)
				 *
				 * \~english
				 * @brief Capacity of the buffer of the sending of a stream (high-water)
				 *
				 * \~
				 */
				static constexpr size_t SEND_HIGH_WATER = (256 * 1024);
				/**
				 * \~russian
				 * @brief Порог накопленных исходящих данных потока
				 *
				 * @details Действует только в pull-модели: пока обвязка не вычитала
				 *          накопленное методом consumePending(), новые кадры тела
				 *          в буфер не собираются. В push-модели буфера нет вовсе -
				 *          функция обратного вызова записи забирает байты сразу
				 *
				 * \~english
				 * @brief Threshold of the accumulated outgoing data of a stream
				 * @details It is in force only in the pull model: while the binding has not read out
				 *          the accumulated by the method consumePending(), the new frames of the body
				 *          are not assembled into the buffer. In the push model there is no buffer at all -
				 *          the callback function of the writing takes the octets at once
				 *
				 * \~
				 */
				static constexpr size_t OUTPUT_HIGH_WATER = (1024 * 1024);
				/**
				 * \~russian
				 * @brief Размер порции, запрашиваемой у источника данных тела
				 *
				 * @details Длина кадра в HTTP/3 протоколом не ограничена, поэтому
				 *          размер порции задаём сами: слишком мелкая множит заголовки
				 *          кадров, слишком крупная задерживает выдачу первых октетов
				 *
				 * \~english
				 * @brief Size of the portion requested from the source of the data of the body
				 * @details The length of a frame in HTTP/3 is not limited by the protocol, therefore
				 *          the size of a portion we set ourselves: too small a one multiplies the headers
				 *          of the frames, too large a one delays the issue of the first octets
				 *
				 * \~
				 */
				static constexpr size_t SOURCE_CHUNK = (16 * 1024);
			public:
				/**
				 * \~russian
				 * @brief Тип кода ошибки протокола
				 *
				 * \~english
				 * @brief Type of an error code of the protocol
				 *
				 * \~
				 */
				using error_t = h3::error_t;
			public:
				/**
				 * \~russian
				 * @brief Структура лимитов безопасности парсера
				 *
				 * \~english
				 * @brief Structure of the limits of the safety of the parser
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Limits : parser_t::limits_t {
					public:
						/**
						 * \~russian
						 * Максимальный размер сжатой секции полей
						 *
						 * @details Длина кадра в HTTP/3 не ограничена протоколом, поэтому без
						 *          этого лимита отправитель одним кадром HEADERS задавал бы
						 *          потребление памяти получателем
						 *
						 * \~english
						 * Largest size of a compressed section of the fields
						 * @details The length of a frame in HTTP/3 is not limited by the protocol, therefore without
						 *          this limit a sender would by a single HEADERS frame set
						 *          the consumption of the memory by the receiver
						 *
						 * \~
						 */
						uint64_t maxHeaderSection;
						/**
						 * \~russian
						 * Максимальный размер нагрузки управляющего кадра
						 *
						 * @details Нагрузка управляющих кадров накапливается в буфере целиком,
						 *          поэтому её размер обязан быть ограничен
						 *
						 * \~english
						 * Largest size of the payload of a control frame
						 * @details The payload of the control frames accumulates in a buffer entirely,
						 *          therefore its size is obliged to be limited
						 *
						 * \~
						 */
						uint64_t maxControlFrame;
						/**
						 * \~russian
						 * Максимальный размер неразобранного хвоста заблокированного потока
						 *
						 * @details Пока секция ждёт вставок QPACK, идущие следом кадры разобрать
						 *          нельзя и они копятся в буфере. Пир, не присылающий инструкций
						 *          кодера вовсе, иначе задавал бы потребление памяти получателем
						 *
						 * \~english
						 * Largest size of the unparsed tail of a blocked stream
						 * @details While a section waits for the insertions of QPACK, the frames going after it cannot
						 *          be parsed and they accumulate in a buffer. A peer not sending the instructions
						 *          of the encoder at all would otherwise set the consumption of the memory by the receiver
						 *
						 * \~
						 */
						uint64_t maxBlockedTail;
						// Максимальное количество одновременно живых потоков запросов
						size_t maxStreams;
					public:
						// Пополнение лимита частоты управляющих кадров (токенов в секунду)
						uint64_t ctrlLimitRate;
						// Стартовый запас лимита частоты управляющих кадров
						uint64_t ctrlLimitBurst;
						// Пополнение лимита частоты кадров приоритета (токенов в секунду)
						uint64_t prioLimitRate;
						// Стартовый запас лимита частоты кадров приоритета
						uint64_t prioLimitBurst;
					public:
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						explicit Limits() noexcept;
				} limits_t;
				/**
				 * \~russian
				 * @brief Структура параметров SETTINGS (RFC 9114 §7.2.4.1, RFC 9204 §5)
				 *
				 * \~english
				 * @brief Structure of the parameters of SETTINGS (RFC 9114 §7.2.4.1, RFC 9204 §5)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Settings {
					public:
						// Размер динамической таблицы QPACK
						uint64_t qpackMaxTableCapacity;
						// Число потоков, которым разрешено ожидать пополнения таблицы QPACK
						uint64_t qpackBlockedStreams;
						// Максимальный размер секции полей (0 - без лимита)
						uint64_t maxFieldSectionSize;
						// Разрешён ли расширенный CONNECT (RFC 9220 §3)
						bool enableConnectProtocol;
					public:
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						explicit Settings() noexcept;
				} settings_t;
				/**
				 * \~russian
				 * @brief Структура расширенного приоритета потока (RFC 9218 §4)
				 *
				 * \~english
				 * @brief Structure of the extended priority of a stream (RFC 9218 §4)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Priority {
					// Срочность потока (0 - наивысшая, 7 - наименьшая)
					uint8_t urgency;
					// Признак инкрементальной доставки потока
					bool incremental;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Priority() noexcept;
				} priority_t;
			public:
				/**
				 * \~russian
				 * @brief Тип функции обратного вызова для обработки применённого SETTINGS пира
				 *
				 * \~english
				 * @brief Type of the callback function for the processing of an applied SETTINGS of the peer
				 *
				 * \~
				 */
				using settings_callback_t = function <void (void)>;
				/**
				 * \~russian
				 * @brief Тип функции обратного вызова для обработки открытия нового потока
				 *
				 * @details Возврат false обрывает поток запросом RESET_STREAM с кодом
				 *          H3_REQUEST_REJECTED
				 *
				 * @param sid идентификатор потока
				 * @return    результат обработки (false - поток обрывается)
				 *
				 * \~english
				 * @brief Type of the callback function for the processing of the opening of a new stream
				 * @details A return of false breaks the stream by a request of a RESET_STREAM with the code
				 *          H3_REQUEST_REJECTED
				 * @param sid identifier of the stream
				 * @return    result of the processing (false - the stream is broken)
				 *
				 * \~
				 */
				using begin_callback_t = function <bool (const uint64_t)>;
				/**
				 * \~russian
				 * @brief Тип функции обратного вызова для обработки закрытия потока
				 *
				 * @param sid  идентификатор потока
				 * @param code код ошибки закрытия (H3_NO_ERROR - штатное закрытие)
				 *
				 * \~english
				 * @brief Type of the callback function for the processing of the closing of a stream
				 * @param sid  identifier of the stream
				 * @param code error code of the closing (H3_NO_ERROR - a regular closing)
				 *
				 * \~
				 */
				using close_callback_t = function <void (const uint64_t, const error_t)>;
				/**
				 * \~russian
				 * @brief Тип функции обратного вызова для обработки ошибки уровня соединения
				 *
				 * @details Своего кадра для сообщения об ошибке у HTTP/3 нет: код уходит
				 *          в кадре CONNECTION_CLOSE транспорта (RFC 9114 §8). Поэтому событие
				 *          одновременно и извещает об ошибке, и обязывает обвязку закрыть
				 *          соединение QUIC с этим кодом - разделять их было бы нечем
				 *
				 * @param code    код ошибки протокола
				 * @param message текстовое описание ошибки
				 *
				 * \~english
				 * @brief Type of the callback function for the processing of an error of the level of the connection
				 * @details HTTP/3 has no frame of its own for a report about an error: the code goes away
				 *          in a CONNECTION_CLOSE frame of the transport (RFC 9114 §8). Therefore the event
				 *          simultaneously both notifies about the error and obliges the binding to close
				 *          the QUIC connection with this code - there would be nothing to separate them by
				 * @param code    error code of the protocol
				 * @param message text description of the error
				 *
				 * \~
				 */
				using error_callback_t = function <void (const error_t, const string_view)>;
				/**
				 * \~russian
				 * @brief Тип функции обратного вызова для обработки полученного GOAWAY
				 *
				 * @details В HTTP/3 кадр GOAWAY несёт единственное число и не несёт кода ошибки:
				 *          от сервера это идентификатор потока запроса, от клиента - идентификатор
				 *          push. Причина завершения сообщается транспортом в CONNECTION_CLOSE
				 *
				 * @param id идентификатор потока запроса либо идентификатор push
				 *
				 * \~english
				 * @brief Type of the callback function for the processing of an obtained GOAWAY
				 * @details In HTTP/3 a GOAWAY frame carries a single number and does not carry an error code:
				 *          from a server this is an identifier of a stream of a request, from a client - an identifier
				 *          of a push. The reason of the completion is reported by the transport in a CONNECTION_CLOSE
				 * @param id identifier of a stream of a request or identifier of a push
				 *
				 * \~
				 */
				using goaway_callback_t = function <void (const uint64_t)>;
				/**
				 * \~russian
				 * @brief Тип функции обратного вызова для обработки анонса server push (только клиент)
				 *
				 * @details Поля обещанного запроса приходят через header_callback_t и
				 *          provider_callback_t с идентификатором ассоциированного потока.
				 *          Возврат false отклоняет push кадром CANCEL_PUSH
				 *
				 * @param sid    идентификатор ассоциированного потока запроса
				 * @param pushId идентификатор обещанного push
				 * @return       результат обработки (false - push отклоняется)
				 *
				 * \~english
				 * @brief Type of the callback function for the processing of an announcement of a server push (only a client)
				 * @details The fields of the promised request come through header_callback_t and
				 *          provider_callback_t with the identifier of the associated stream.
				 *          A return of false rejects the push by a CANCEL_PUSH frame
				 * @param sid    identifier of the associated stream of the request
				 * @param pushId identifier of the promised push
				 * @return       result of the processing (false - the push is rejected)
				 *
				 * \~
				 */
				using push_callback_t = function <bool (const uint64_t, const uint64_t)>;
				/**
				 * \~russian
				 * @brief Тип функции обратного вызова для обработки фазы приёма сообщения потока
				 *
				 * @details Последовательность событий при приёме одного сообщения потока
				 *          совпадает с HTTP/1 и HTTP/2:
				 *          1. (BEGIN, NONE)    - получена первая секция полей потока
				 *          2. (END, HEADERS)   - секция полей доставлена (после провайдера)
				 *          3. (BEGIN, BODY)    - ожидается тело (поток не завершён секцией полей)
				 *          4. (END, BODY)      - тело полностью принято
				 *          5. (BEGIN, TRAILER) - получена секция трейлеров
				 *          6. (END, TRAILER)   - трейлеры доставлены (после провайдера с nullptr)
				 *          7. (END, NONE)      - сообщение потока полностью принято
				 *          Для обещанных запросов PUSH_PROMISE фазы не вызываются, как и для
				 *          информационных ответов сервера (1xx): такая секция промежуточная.
				 *          Возврат false обрывает поток
				 *
				 * @param sid   идентификатор потока
				 * @param phase фаза приёма сообщения потока
				 * @param part  часть сообщения (заголовки, трейлеры, тело), NONE - сообщение целиком
				 * @return      результат обработки (false - поток обрывается)
				 *
				 * \~english
				 * @brief Type of the callback function for the processing of the phase of the acceptance of a message of a stream
				 * @details The sequence of the events at the acceptance of a single message of a stream
				 *          coincides with HTTP/1 and HTTP/2:
				 *          1. (BEGIN, NONE)    - the first section of the fields of the stream is obtained
				 *          2. (END, HEADERS)   - the section of the fields is delivered (after the provider)
				 *          3. (BEGIN, BODY)    - a body is expected (the stream is not completed by the section of the fields)
				 *          4. (END, BODY)      - the body is fully accepted
				 *          5. (BEGIN, TRAILER) - a section of the trailers is obtained
				 *          6. (END, TRAILER)   - the trailers are delivered (after the provider with a nullptr)
				 *          7. (END, NONE)      - the message of the stream is fully accepted
				 *          For the promised requests of a PUSH_PROMISE the phases are not called, as well as for
				 *          the informational answers of a server (1xx): such a section is an intermediate one.
				 *          A return of false breaks the stream
				 * @param sid   identifier of the stream
				 * @param phase phase of the acceptance of the message of the stream
				 * @param part  part of the message (the headers, the trailers, the body), NONE - the message as a whole
				 * @return      result of the processing (false - the stream is broken)
				 *
				 * \~
				 */
				using phase_callback_t = function <bool (const uint64_t, const phase_t, const part_t)>;
				/**
				 * \~russian
				 * @brief Тип функции обратного вызова для обработки провайдера полей потока
				 *
				 * @details Вызывается по завершению секции полей. Провайдер собран из
				 *          псевдо-заголовков секции: для направления REQUEST это request_t,
				 *          для RESPONSE - response_t. Для трейлеров провайдер передаётся
				 *          как nullptr. Возврат false обрывает поток
				 *
				 * @param sid       идентификатор потока
				 * @param provider  провайдер полей потока (nullptr для трейлеров)
				 * @param endStream признак завершения потока (тела не будет)
				 * @return          результат обработки (false - поток обрывается)
				 *
				 * \~english
				 * @brief Type of the callback function for the processing of the provider of the fields of a stream
				 * @details It is called at the completion of a section of the fields. The provider is assembled from
				 *          the pseudo headers of the section: for the direction REQUEST this is a request_t,
				 *          for a RESPONSE - a response_t. For the trailers the provider is transmitted
				 *          as a nullptr. A return of false breaks the stream
				 * @param sid       identifier of the stream
				 * @param provider  provider of the fields of the stream (a nullptr for the trailers)
				 * @param endStream flag of the completion of the stream (there will be no body)
				 * @return          result of the processing (false - the stream is broken)
				 *
				 * \~
				 */
				using provider_callback_t = function <bool (const uint64_t, const provider_t *, const bool)>;
				/**
				 * \~russian
				 * @brief Тип функции обратного вызова для обработки поля секции заголовков либо трейлеров
				 *
				 * @details Указатели name/value действительны ТОЛЬКО на время вызова.
				 *          Возврат false обрывает поток
				 *
				 * @param sid   идентификатор потока
				 * @param name  название поля
				 * @param value значение поля
				 * @param part  часть сообщения (HEADERS или TRAILER)
				 * @return      результат обработки (false - поток обрывается)
				 *
				 * \~english
				 * @brief Type of the callback function for the processing of a field of a section of the headers or of the trailers
				 * @details The pointers name/value are valid ONLY for the time of the call.
				 *          A return of false breaks the stream
				 * @param sid   identifier of the stream
				 * @param name  name of the field
				 * @param value value of the field
				 * @param part  part of the message (HEADERS or TRAILER)
				 * @return      result of the processing (false - the stream is broken)
				 *
				 * \~
				 */
				using header_callback_t = function <bool (const uint64_t, const string_view, const string_view, const part_t)>;
				/**
				 * \~russian
				 * @brief Тип функции обратного вызова для обработки фрагмента тела потока
				 *
				 * @details Указатель buffer действителен ТОЛЬКО на время вызова (zero-copy).
				 *          Возврат false обрывает поток
				 *
				 * @param sid       идентификатор потока
				 * @param buffer    буфер данных тела
				 * @param size      размер данных тела
				 * @param endStream признак завершения потока
				 * @return          результат обработки (false - поток обрывается)
				 *
				 * \~english
				 * @brief Type of the callback function for the processing of a fragment of the body of a stream
				 * @details The pointer buffer is valid ONLY for the time of the call (zero-copy).
				 *          A return of false breaks the stream
				 * @param sid       identifier of the stream
				 * @param buffer    buffer of the data of the body
				 * @param size      size of the data of the body
				 * @param endStream flag of the completion of the stream
				 * @return          result of the processing (false - the stream is broken)
				 *
				 * \~
				 */
				using data_callback_t = function <bool (const uint64_t, const void *, const size_t, const bool)>;
			public:
				/**
				 * \~russian
				 * @brief Тип функции обратного вызова открытия однонаправленного потока
				 *
				 * @details Идентификаторы потоков выдаёт транспорт, а не парсер, поэтому открыть
				 *          управляющий поток и два потока QPACK парсер может только через
				 *          обвязку. Возврат отрицательного значения означает, что транспорт
				 *          сейчас открыть поток не может: парсер повторит попытку позже
				 *
				 * @return идентификатор открытого потока либо отрицательное значение
				 *
				 * \~english
				 * @brief Type of the callback function of the opening of a unidirectional stream
				 * @details The identifiers of the streams are issued by the transport rather than by the parser, therefore to open
				 *          the control stream and the two streams of QPACK the parser can only through
				 *          the binding. A return of a negative value means that the transport
				 *          cannot open a stream now: the parser will repeat the attempt later
				 * @return identifier of the opened stream or a negative value
				 *
				 * \~
				 */
				using open_callback_t = function <int64_t (void)>;
				/**
				 * \~russian
				 * @brief Тип функции обратного вызова записи исходящих байтов потока
				 *
				 * @details Если установлена - парсер отдаёт исходящие байты транспорту сразу
				 *          по мере формирования. Если не установлена - байты накапливаются
				 *          в буферах потоков (pull-модель: outgoing() + pending() +
				 *          consumePending())
				 *
				 * @param sid    идентификатор потока
				 * @param buffer буфер исходящих данных
				 * @param size   размер исходящих данных
				 * @param fin    признак завершения потока в исходящем направлении
				 *
				 * \~english
				 * @brief Type of the callback function of the writing of the outgoing octets of a stream
				 * @details If it is set - the parser issues the outgoing octets to the transport at once
				 *          as they are formed. If it is not set - the octets accumulate
				 *          in the buffers of the streams (the pull model: outgoing() + pending() +
				 *          consumePending())
				 * @param sid    identifier of the stream
				 * @param buffer buffer of the outgoing data
				 * @param size   size of the outgoing data
				 * @param fin    flag of the completion of the stream in the outgoing direction
				 *
				 * \~
				 */
				using write_callback_t = function <void (const uint64_t, const void *, const size_t, const bool)>;
				/**
				 * \~russian
				 * @brief Тип функции обратного вызова обрыва потока
				 *
				 * @details Своего кадра для обрыва потока у HTTP/3 нет: это делают кадры
				 *          RESET_STREAM и STOP_SENDING транспорта (RFC 9114 §4.1)
				 *
				 * @param sid  идентификатор потока
				 * @param code код ошибки, с которым обрывается поток
				 * @param stop признак остановки приёма (STOP_SENDING) вместо обрыва отправки
				 *
				 * \~english
				 * @brief Type of the callback function of the breaking of a stream
				 * @details HTTP/3 has no frame of its own for the breaking of a stream: this is done by the frames
				 *          RESET_STREAM and STOP_SENDING of the transport (RFC 9114 §4.1)
				 * @param sid  identifier of the stream
				 * @param code error code with which the stream is broken
				 * @param stop flag of a stopping of the acceptance (STOP_SENDING) instead of a breaking of the sending
				 *
				 * \~
				 */
				using abort_callback_t = function <void (const uint64_t, const error_t, const bool)>;
				/**
				 * \~russian
				 * @brief Тип функции обратного вызова готовности потока принимать данные тела
				 *
				 * @details Подаётся один раз на каждое опустошение буфера отправки ниже
				 *          нижней водяной метки: приложение, получившее короткую запись
				 *          от sendData(), дожидается сигнала и досылает остаток.
				 *          Для pull-источника данных сигнал не подаётся - источник
				 *          опрашивается парсером сам
				 *
				 * @note Сигнатура шире, чем у одноимённой функции HTTP/2, по двум
				 *       причинам. Во-первых, набор функций обратного вызова HTTP/3 уже
				 *       содержит goaway_callback_t того же вида void (uint64_t), и
				 *       перегрузка on() их бы не различила. Во-вторых, свободное место
				 *       приложению всё равно нужно: короткая запись sendData() сообщает,
				 *       сколько байт принято, но не сколько примут в следующий раз
				 *
				 * @param sid  идентификатор потока
				 * @param room свободное место в буфере отправки потока
				 *
				 * \~english
				 * @brief Type of the callback function of the readiness of a stream to accept the data of the body
				 * @details It is supplied once per every emptying of the buffer of the sending below
				 *          the lower water mark: an application which has obtained a short writing
				 *          from sendData() waits for the signal and sends the remainder.
				 *          For a pull source of the data the signal is not supplied - the source
				 *          is polled by the parser itself
				 * @note The signature is wider than at the function of HTTP/2 of the same name, for two
				 *       reasons. Firstly, the collection of the callback functions of HTTP/3 already
				 *       contains a goaway_callback_t of the same kind void (uint64_t), and
				 *       the overload of on() would not distinguish them. Secondly, the free place
				 *       is needed by the application all the same: a short writing of sendData() reports
				 *       how many octets are accepted but not how many will be accepted the next time
				 * @param sid  identifier of the stream
				 * @param room free place in the buffer of the sending of the stream
				 *
				 * \~
				 */
				using writable_callback_t = function <void (const uint64_t, const size_t)>;
				/**
				 * \~russian
				 * @brief Тип функции pull-источника данных тела потока
				 *
				 * @details Парсер сам запрашивает у источника данные ровно тогда, когда
				 *          в буфере отправки потока есть место ниже верхней водяной метки.
				 *          Источник пишет данные напрямую в переданный буфер (не более cap
				 *          октетов), выставляет eof = true по достижении конца тела и
				 *          возвращает число записанных байт либо -1 при ошибке - тогда
				 *          поток обрывается. Приложение не держит копию всего тела
				 *
				 * @param sid    идентификатор потока
				 * @param buffer буфер для заполнения (хвост буфера отправки потока)
				 * @param cap    ёмкость буфера
				 * @param eof    флаг достижения конца тела
				 * @return       число записанных байт либо -1 при ошибке
				 *
				 * \~english
				 * @brief Type of the function of a pull source of the data of the body of a stream
				 * @details The parser itself requests the data from the source exactly then when
				 *          in the buffer of the sending of the stream there is a place below the upper water mark.
				 *          The source writes the data directly into the transmitted buffer (not more than cap
				 *          octets), sets eof = true at the reaching of the end of the body and
				 *          returns the number of the written octets or -1 at an error - then
				 *          the stream is broken. The application does not hold a copy of the whole body
				 * @param sid    identifier of the stream
				 * @param buffer buffer for the filling (the tail of the buffer of the sending of the stream)
				 * @param cap    capacity of the buffer
				 * @param eof    flag of the reaching of the end of the body
				 * @return       number of the written octets or -1 at an error
				 *
				 * \~
				 */
				using data_source_callback_t = function <int64_t (const uint64_t, uint8_t *, const size_t, bool &)>;
			private:
				/**
				 * \~russian
				 * @brief Класс token-bucket для ограничения частоты событий
				 *
				 * @details Целочисленные токены, пополнение rate токенов в секунду до предела
				 *          burst. Время задаётся извне через updateTime(); без обновления времени
				 *          работает только стартовый запас burst, чего достаточно, чтобы погасить
				 *          мгновенный всплеск
				 *
				 * \~english
				 * @brief Class of a token-bucket for the limitation of the frequency of the events
				 * @details The integer tokens, a replenishment of rate tokens per second up to the limit
				 *          burst. The time is set from the outside through updateTime(); without an updating of the time
				 *          only the starting reserve burst works, which suffices to quench
				 *          an instantaneous burst
				 *
				 * \~
				 */
				typedef class __AWH_SHARED_EXPORT__ Ratelim {
					public:
						// Пополнение токенов в секунду
						uint64_t rate;
						// Максимум токенов
						uint64_t burst;
						// Текущее число токенов
						uint64_t value;
						// Последний момент обновления (секунды)
						uint64_t stamp;
					public:
						/**
						 * \~russian
						 * @brief Метод списания токенов
						 *
						 * @param value число списываемых токенов
						 * @return      результат списания (false - токенов не хватает)
						 *
						 * \~english
						 * @brief Method of the writing off of the tokens
						 * @param value number of the tokens being written off
						 * @return      result of the writing off (false - the tokens do not suffice)
						 *
						 * \~
						 */
						bool drain(const uint64_t value) noexcept;
						/**
						 * \~russian
						 * @brief Метод обновления момента времени
						 *
						 * @param stamp текущий момент времени в секундах
						 *
						 * \~english
						 * @brief Method of the updating of the moment of the time
						 * @param stamp current moment of the time in seconds
						 *
						 * \~
						 */
						void update(const uint64_t stamp) noexcept;
						/**
						 * \~russian
						 * @brief Метод инициализации лимита
						 *
						 * @param burst стартовый запас токенов
						 * @param rate  пополнение токенов в секунду
						 *
						 * \~english
						 * @brief Method of the initialization of the limit
						 * @param burst starting reserve of the tokens
						 * @param rate  replenishment of the tokens per second
						 *
						 * \~
						 */
						void init(const uint64_t burst, const uint64_t rate) noexcept;
					public:
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						explicit Ratelim() noexcept;
				} ratelim_t;
				/**
				 * \~russian
				 * @brief Структура кольца идентификаторов push
				 *
				 * @details Учёт идентификаторов ведётся окном, а не полным множеством:
				 *          пир вправе слать отмены обещаний, потоки которых не откроет
				 *          никогда, и множество росло бы вместе с длительностью соединения.
				 *          Вытеснение предпочтительнее фатального предела: забытая отмена
				 *          означает лишь прочитанный поток push, который никому не нужен,
				 *          а исчерпанный предел означал бы разрыв штатного соединения
				 *
				 * \~english
				 * @brief Structure of the ring of the identifiers of a push
				 * @details The account of the identifiers is conducted by a window rather than by a full set:
				 *          a peer is entitled to send the cancellations of the promises the streams of which it will never
				 *          open, and a set would grow together with the duration of the connection.
				 *          An eviction is preferable to a fatal limit: a forgotten cancellation
				 *          means only a read stream of a push which nobody needs,
				 *          while an exhausted limit would mean a break of a regular connection
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Ring {
					public:
						// Ячейки кольца (UINT64_MAX - ячейка пуста)
						vector <uint64_t> items;
						// Позиция записи в кольце
						size_t cursor;
					public:
						/**
						 * \~russian
						 * @brief Метод проверки наличия идентификатора в кольце
						 *
						 * @param value искомый идентификатор
						 * @return      результат проверки
						 *
						 * \~english
						 * @brief Method of checking the presence of an identifier in the ring
						 * @param value sought identifier
						 * @return      result of the checking
						 *
						 * \~
						 */
						bool has(const uint64_t value) const noexcept;
						/**
						 * \~russian
						 * @brief Метод записи идентификатора в кольцо
						 *
						 * @param value записываемый идентификатор
						 *
						 * \~english
						 * @brief Method of the writing of an identifier into the ring
						 * @param value identifier being written
						 *
						 * \~
						 */
						void put(const uint64_t value) noexcept;
						/**
						 * \~russian
						 * @brief Метод удаления идентификатора из кольца
						 *
						 * @param value удаляемый идентификатор
						 *
						 * \~english
						 * @brief Method of the removal of an identifier from the ring
						 * @param value identifier being removed
						 *
						 * \~
						 */
						void drop(const uint64_t value) noexcept;
						/**
						 * \~russian
						 * @brief Метод очистки кольца
						 *
						 * \~english
						 * @brief Method of the clearing of the ring
						 *
						 * \~
						 */
						void clear() noexcept;
					public:
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						explicit Ring() noexcept;
				} ring_t;
				/**
				 * \~russian
				 * @brief Структура записи расширенного приоритета
				 *
				 * @details Одна и та же запись описывает и приоритет ещё не открытого
				 *          потока, и приоритет обещания push: сигнал в обоих случаях
				 *          один и тот же, различается только адресуемая сущность
				 *
				 * \~english
				 * @brief Structure of a record of an extended priority
				 * @details One and the same record describes both the priority of a not yet opened
				 *          stream and the priority of a promise of a push: the signal in both cases
				 *          is one and the same, only the addressed entity differs
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Signal {
					// Срочность (0 - наивысшая, 7 - наименьшая)
					uint8_t urgency;
					// Признак инкрементальной доставки
					bool incremental;
					// Идентификатор потока либо обещания push
					uint64_t id;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Signal() noexcept : urgency(0), incremental(false), id(0) {}
				} signal_t;
				/**
				 * \~russian
				 * @brief Структура записи об обещанном push
				 *
				 * @details Повтор идентификатора обещания сам по себе допустим: сервер вправе
				 *          пообещать один и тот же push на нескольких потоках запросов.
				 *          Недопустимо расхождение секций полей при таком повторе, и чтобы
				 *          его заметить, от секции хранится отпечаток - хранить сами поля
				 *          значило бы отдать серверу управление нашей памятью (RFC 9114 §7.2.5)
				 *
				 * \~english
				 * @brief Structure of a record about a promised push
				 * @details A repetition of an identifier of a promise by itself is admissible: a server is entitled
				 *          to promise one and the same push on several streams of the requests.
				 *          A divergence of the sections of the fields at such a repetition is inadmissible, and to notice
				 *          it, a fingerprint is stored from a section - to store the fields themselves
				 *          would mean to give the server the control of our memory (RFC 9114 §7.2.5)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Promise {
					// Идентификатор обещания (UINT64_MAX - ячейка пуста)
					uint64_t id;
					// Отпечаток секции полей обещания
					size_t digest;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Promise() noexcept : id(UINT64_MAX), digest(0) {}
				} promise_t;
				/**
				 * \~russian
				 * @brief Структура состояния разбора кадров одного потока
				 *
				 * @details Разбор кадра идёт в три состояния: накопление заголовка кадра,
				 *          накопление нагрузки управляющего кадра и потоковая выдача нагрузки
				 *          кадра DATA. Третье состояние существует именно потому, что длина
				 *          кадра DATA протоколом не ограничена
				 *
				 * \~english
				 * @brief Structure of the state of the parsing of the frames of a single stream
				 * @details The parsing of a frame goes in three states: the accumulation of the header of the frame,
				 *          the accumulation of the payload of a control frame and the streaming issue of the payload
				 *          of a DATA frame. The third state exists exactly because the length
				 *          of a DATA frame is not limited by the protocol
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Framing {
					public:
						// Признак того, что заголовок кадра разобран и идёт разбор нагрузки
						bool active;
						// Тип разбираемого кадра
						uint64_t type;
						// Длина нагрузки разбираемого кадра
						uint64_t length;
						// Остаток нагрузки разбираемого кадра
						uint64_t remain;
						// Буфер накопления заголовка кадра либо нагрузки управляющего кадра
						string buffer;
					public:
						/**
						 * \~russian
						 * @brief Метод сброса состояния разбора
						 *
						 * \~english
						 * @brief Method of the reset of the state of the parsing
						 *
						 * \~
						 */
						void clear() noexcept;
					public:
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						explicit Framing() noexcept;
				} framing_t;
				/**
				 * \~russian
				 * @brief Структура состояния потока запроса
				 *
				 * \~english
				 * @brief Structure of the state of a stream of a request
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Stream {
					public:
						// Состояние потока
						h3::stream_state_t state;
						// Состояние разбора кадров потока
						framing_t framing;
					public:
						/**
						 * \~russian
						 * Поколение состояния потока
						 *
						 * @details Отличает пересозданный поток от пережившего выход наружу.
						 *          Сверки одного лишь адреса мало: карта потоков узловая,
						 *          и удаление с последующей вставкой того же идентификатора
						 *          способны вернуть тот же адрес
						 *
						 * \~english
						 * Generation of the state of the stream
						 * @details It distinguishes a recreated stream from one which has survived an exit outside.
						 *          A comparison of the address alone is not enough: the map of the streams is a nodal one,
						 *          and a removal with a subsequent insertion of the same identifier
						 *          are capable of returning the same address
						 *
						 * \~
						 */
						uint64_t generation;
					public:
						// Признак получения финальной секции полей
						bool headers;
						// Признак получения секции трейлеров
						bool trailers;
						// Признак начала фазы приёма тела
						bool body;
						// Признак завершённости приёма сообщения
						bool completed;
						// Признак безтелесности принимаемого сообщения (ответ на HEAD, статусы 204 и 304)
						bool headless;
						/**
						 * \~russian
						 * Признак безтелесности отправляемого сообщения (ответ на принятый HEAD)
						 *
						 * @details Признак отдельный от headless: тело самого запроса HEAD
						 *          запрещено лишь через SHOULD NOT (RFC 9110 §9.3.2), поэтому
						 *          принимать его парсер обязан, а отдавать тело в ответ - нет
						 *
						 * \~english
						 * Flag of the bodylessness of a message being sent (an answer to an accepted HEAD)
						 * @details The flag is separate from headless: the body of a HEAD request itself
						 *          is prohibited only through a SHOULD NOT (RFC 9110 §9.3.2), therefore
						 *          the parser is obliged to accept it, but to issue a body in an answer - no
						 *
						 * \~
						 */
						bool headlessSend;
						/**
						 * \~russian
						 * Признак запрета секции трейлеров в принимаемом сообщении
						 * (ответы 204 и 304 - RFC 9110 §15.3.5, §15.4.5)
						 *
						 * @details Признак отдельный от headless: у ответа на HEAD
						 *          запрещено только содержимое, а трейлеры §9.3.2
						 *          не запрещает, и кадрирование их допускает
						 *
						 * \~english
						 * Flag of the prohibition of a section of the trailers in a message being accepted
						 * (the answers 204 and 304 - RFC 9110 §15.3.5, §15.4.5)
						 * @details The flag is separate from headless: at an answer to a HEAD
						 *          only the content is prohibited, while the trailers §9.3.2
						 *          does not prohibit, and the framing admits them
						 *
						 * \~
						 */
						bool trailerless;
						/**
						 * \~russian
						 * Признак запрета секции трейлеров в отправляемом сообщении
						 * (ответы 204 и 304 - RFC 9110 §15.3.5, §15.4.5)
						 *
						 * @details Признак отдельный от headlessSend: у ответа на HEAD
						 *          запрещено только содержимое, а трейлеры §9.3.2 не
						 *          запрещает вовсе, и кадрирование их допускает
						 *
						 * \~english
						 * Flag of the prohibition of a section of the trailers in a message being sent
						 * (the answers 204 and 304 - RFC 9110 §15.3.5, §15.4.5)
						 * @details The flag is separate from headlessSend: at an answer to a HEAD
						 *          only the content is prohibited, while the trailers §9.3.2 does not
						 *          prohibit at all, and the framing admits them
						 *
						 * \~
						 */
						bool trailerlessSend;
						/**
						 * \~russian
						 * Признак завершения потока в нашем направлении
						 *
						 * @details Поток удаляется только после завершения обоих направлений:
						 *          сервер отвечает на том же потоке, на котором принял запрос,
						 *          поэтому завершение приёма закрывать поток не вправе
						 *
						 * \~english
						 * Flag of the completion of the stream in our direction
						 * @details A stream is removed only after the completion of both the directions:
						 *          a server answers on the same stream on which it has accepted the request,
						 *          therefore the completion of the acceptance is not entitled to close the stream
						 *
						 * \~
						 */
						bool localFin;
					public:
						// Суммарный размер принятого тела потока
						uint64_t length;
						// Объявленное значение заголовка content-length (UINT64_MAX - не объявлено)
						uint64_t declared;
					public:
						// Срочность потока (RFC 9218 §4.1)
						uint8_t urgency;
						// Признак инкрементального потока (RFC 9218 §4.2)
						bool incremental;
						/**
						 * \~russian
						 * Приоритет потока задан кадром PRIORITY_UPDATE
						 *
						 * @details Кадр перекрывает любой другой сигнал приоритета
						 *          (RFC 9218 §7), поэтому заголовок [priority], пришедший
						 *          после него, приоритет уже не меняет
						 *
						 * \~english
						 * The priority of the stream is set by a PRIORITY_UPDATE frame
						 * @details The frame overrides any other signal of the priority
						 *          (RFC 9218 §7), therefore a header [priority] which has come
						 *          after it no longer changes the priority
						 *
						 * \~
						 */
						bool prioritized;
					public:
						/**
						 * \~russian
						 * Отложенная секция полей заблокированного потока
						 *
						 * @details Секция, потребовавшая ещё не пришедших вставок QPACK,
						 *          сохраняется целиком и разбирается заново после обработки
						 *          очередной порции инструкций потока кодера
						 *
						 * \~english
						 * Postponed section of the fields of a blocked stream
						 * @details A section which has required not yet arrived insertions of QPACK
						 *          is preserved entirely and is parsed anew after the processing
						 *          of the next portion of the instructions of the stream of the encoder
						 *
						 * \~
						 */
						string blocked;
						/**
						 * \~russian
						 * Неразобранный хвост потока, накопленный за время блокировки
						 *
						 * @details Кадры, пришедшие следом за заблокированной секцией, разобрать
						 *          нельзя: тело до неё недопустимо, а вторая секция затёрла бы
						 *          отложенную. Хвост накапливается целиком и разбирается заново
						 *          сразу после того, как отложенная секция разошлась по обработчикам
						 *
						 * \~english
						 * Unparsed tail of the stream accumulated during the time of the blocking
						 * @details The frames which have come after a blocked section cannot be parsed:
						 *          a body before it is inadmissible, while a second section would erase
						 *          the postponed one. The tail accumulates entirely and is parsed anew
						 *          right after the postponed section has gone away to the handlers
						 *
						 * \~
						 */
						string blockedTail;
						// Признак наличия отложенной секции
						bool blockedActive;
						// Тип кадра, которому принадлежит отложенная секция
						uint64_t blockedType;
						// Идентификатор push отложенного обещания
						uint64_t blockedPushId;
						// Признак завершения потока вместе с отложенной секцией
						bool blockedFin;
					public:
						/**
						 * \~russian
						 * Буфер отправки тела потока
						 *
						 * @details Накапливает тело, ещё не обёрнутое в кадры DATA. Его
						 *          ёмкость и есть верхняя водяная метка: заполненный буфер
						 *          превращает sendData() в короткую запись, а опустевший -
						 *          в сигнал готовности
						 *
						 * \~english
						 * Buffer of the sending of the body of the stream
						 * @details It accumulates the body not yet wrapped into the DATA frames. Its
						 *          capacity is exactly the upper water mark: a filled buffer
						 *          turns sendData() into a short writing, while an emptied one -
						 *          into a signal of the readiness
						 *
						 * \~
						 */
						string sendBuffer;
						/**
						 * \~russian
						 * Количество уже обёрнутых в кадры октетов начала буфера отправки
						 *
						 * @details Отправленное не вырезается сразу: вырезание сдвигает весь
						 *          остаток буфера и даёт квадратичную стоимость на длинном
						 *          теле. Освобождение выполняется уплотнением
						 *
						 * \~english
						 * Number of the already wrapped into the frames octets of the beginning of the buffer of the sending
						 * @details The sent is not cut out at once: a cutting out shifts the whole
						 *          remainder of the buffer and gives a quadratic cost on a long
						 *          body. The release is performed by a compaction
						 *
						 * \~
						 */
						size_t sendOffset;
						// Pull-источник данных тела потока (пусто - не задан)
						data_source_callback_t source;
						// Признак достижения конца тела источника
						bool sourceEof;
						// Признак отложенного завершения потока на последнем фрагменте тела
						bool endStreamPending;
						/**
						 * \~russian
						 * Признак поданного сигнала готовности для текущего заполнения буфера
						 *
						 * @details Взводится приёмом данных выше нижней метки, снимается подачей
						 *          сигнала: на одно заполнение приходится ровно один сигнал
						 *
						 * \~english
						 * Flag of a supplied signal of the readiness for the current filling of the buffer
						 * @details It is raised by an acceptance of the data above the lower mark, is removed by a supply
						 *          of the signal: per one filling exactly one signal falls
						 *
						 * \~
						 */
						bool writableNotified;
						/**
						 * \~russian
						 * Отложенная секция трейлеров потока
						 *
						 * @note Названа иначе, чем признак принятой секции трейлеров:
						 *       trailers относится к приёму, sendTrailers - к отправке
						 *
						 * \~english
						 * Postponed section of the trailers of the stream
						 * @note It is named differently from the flag of an accepted section of the trailers:
						 *       trailers relates to the acceptance, sendTrailers - to the sending
						 *
						 * \~
						 */
						vector <h3::qpack::field_t> sendTrailers;
						// Признак отложенной секции трейлеров
						bool trailersPending;
						/**
						 * \~russian
						 * Признак отправленной нами секции полей
						 *
						 * @note Отличается от headers: тот относится к принятой секции,
						 *       а этот - к отправленной. Следующая по потоку секция
						 *       после него - уже трейлеры
						 *
						 * \~english
						 * Flag of a section of the fields sent by us
						 * @note It differs from headers: that one relates to an accepted section,
						 *       while this one - to a sent one. The next section along the stream
						 *       after it is already the trailers
						 *
						 * \~
						 */
						bool headersSent;
					public:
						/**
						 * \~russian
						 * @brief Метод получения объёма ещё не обёрнутого в кадры тела
						 *
						 * @return объём данных буфера отправки
						 *
						 * \~english
						 * @brief Method of getting the volume of the body not yet wrapped into the frames
						 * @return volume of the data of the buffer of the sending
						 *
						 * \~
						 */
						size_t pending() const noexcept;
						/**
						 * \~russian
						 * @brief Метод амортизированного уплотнения буфера отправки
						 *
						 * \~english
						 * @brief Method of an amortized compaction of the buffer of the sending
						 *
						 * \~
						 */
						void compactSendBuffer() noexcept;
					public:
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						explicit Stream() noexcept;
				} stream_t;
				/**
				 * \~russian
				 * @brief Структура буфера исходящих данных потока
				 *
				 * @details Нужна только в pull-модели, когда функция обратного вызова записи
				 *          не установлена. Буфер заводится на любой поток, включая служебные
				 *          однонаправленные: инструкции QPACK и кадры управляющего потока
				 *          уходят тем же путём, что и данные потоков запросов
				 *
				 * \~english
				 * @brief Structure of the buffer of the outgoing data of a stream
				 * @details It is needed only in the pull model, when the callback function of the writing
				 *          is not set. The buffer is started for any stream, including the service
				 *          unidirectional ones: the instructions of QPACK and the frames of the control stream
				 *          go away by the same path as the data of the streams of the requests
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Outgoing {
					public:
						// Буфер накопленных исходящих данных
						string buffer;
						// Количество уже выданных наружу октетов буфера
						size_t consumed;
						// Признак завершения потока в исходящем направлении
						bool fin;
					public:
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						explicit Outgoing() noexcept;
				} outgoing_t;
				/**
				 * \~russian
				 * @brief Структура состояния однонаправленного потока
				 *
				 * \~english
				 * @brief Structure of the state of a unidirectional stream
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Unistream {
					public:
						// Признак прочитанного типа потока
						bool known;
						// Тип однонаправленного потока
						uint64_t type;
						// Идентификатор push для потока push
						uint64_t pushId;
						// Признак прочитанного идентификатора push
						bool identified;
						// Состояние разбора кадров потока
						framing_t framing;
						// Буфер накопления типа потока либо инструкций кодека
						string buffer;
					public:
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						explicit Unistream() noexcept;
				} unistream_t;
				/**
				 * \~russian
				 * @brief Структура набора функций обратного вызова парсера
				 *
				 * \~english
				 * @brief Structure of the collection of the callback functions of the parser
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Callbacks {
					public:
						// Функция обратного вызова открытия однонаправленного потока
						open_callback_t open;
						// Функция обратного вызова записи исходящих байтов потока
						write_callback_t write;
						// Функция обратного вызова обрыва потока
						abort_callback_t abort;
					public:
						// Функция обратного вызова обработки применённого SETTINGS пира
						settings_callback_t settings;
						// Функция обратного вызова обработки открытия нового потока
						begin_callback_t begin;
						// Функция обратного вызова обработки закрытия потока
						close_callback_t close;
						// Функция обратного вызова обработки ошибки уровня соединения
						error_callback_t error;
						// Функция обратного вызова обработки полученного GOAWAY
						goaway_callback_t goaway;
						// Функция обратного вызова обработки анонса server push
						push_callback_t push;
						// Функция обратного вызова обработки фазы приёма сообщения потока
						phase_callback_t phase;
						// Функция обратного вызова обработки провайдера полей потока
						provider_callback_t provider;
						// Функция обратного вызова обработки поля секции
						header_callback_t header;
						// Функция обратного вызова обработки фрагмента тела потока
						data_callback_t data;
						// Функция обратного вызова готовности потока принимать данные тела
						writable_callback_t writable;
					public:
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						explicit Callbacks() noexcept;
				} callbacks_t;
			private:
				// Роль локального эндпоинта на соединении
				h3::endpoint_t _endpoint;
			private:
				// Кодер полей QPACK
				h3::qpack::encoder_t _encoder;
				// Декодер полей QPACK
				h3::qpack::decoder_t _decoder;
			private:
				// Протокол, с которым работает парсер
				proto_t _proto;
			private:
				// Порог сигнала готовности потока принимать данные
				size_t _sendLowWater;
				// Ёмкость буфера отправки потока
				size_t _sendHighWater;
				// Порог накопленных исходящих данных потока (только pull-модель)
				size_t _outputHighWater;
			private:
				// Лимиты безопасности парсера
				limits_t _limits;
				// Наши параметры SETTINGS
				settings_t _settings;
				// Параметры SETTINGS, полученные от пира
				settings_t _remote;
			private:
				// Набор функций обратного вызова парсера
				callbacks_t _callbacks;
			private:
				// Карта потоков запросов
				unordered_map <uint64_t, stream_t> _streams;
				// Карта однонаправленных потоков
				unordered_map <uint64_t, unistream_t> _unistreams;
				// Карта буферов исходящих данных потоков (pull-модель)
				unordered_map <uint64_t, outgoing_t> _pending;
			private:
				// Идентификатор нашего управляющего потока (UINT64_MAX - не открыт)
				uint64_t _controlLocal;
				// Идентификатор управляющего потока пира (UINT64_MAX - не открыт)
				uint64_t _controlRemote;
				// Идентификатор нашего потока инструкций кодера QPACK
				uint64_t _encoderLocal;
				// Идентификатор нашего потока инструкций декодера QPACK
				uint64_t _decoderLocal;
				// Идентификатор потока инструкций кодера QPACK пира
				uint64_t _encoderRemote;
				// Идентификатор потока инструкций декодера QPACK пира
				uint64_t _decoderRemote;
			private:
				// Признак получения SETTINGS от пира
				bool _settingsReceived;
				// Признак отправки нашего SETTINGS
				bool _settingsSent;
				// Признак завершённости соединения
				bool _closed;
			private:
				// Наибольший идентификатор push, разрешённый пиром
				uint64_t _maxPushId;
				// Наибольший идентификатор push, разрешённый нами
				uint64_t _localMaxPushId;
				// Идентификатор следующего выдаваемого push
				uint64_t _nextPushId;
				/**
				 * \~russian
				 * Идентификаторы отменённых обещаний push, потоки которых ещё не пришли
				 *
				 * @details Отмена обгоняет поток: кадр CANCEL_PUSH идёт управляющим потоком,
				 *          а сам push - своим, и порядок между ними не задан. Запись снимается
				 *          приходом потока; отмена обещания, поток которого уже пришёл, эффекта
				 *          не имеет вовсе и не записывается (RFC 9114 §7.2.3)
				 *
				 * \~english
				 * Identifiers of the cancelled promises of a push the streams of which have not yet come
				 * @details A cancellation outstrips a stream: a CANCEL_PUSH frame goes by the control stream,
				 *          while the push itself - by its own, and the order between them is not set. A record is removed
				 *          by the arrival of the stream; a cancellation of a promise the stream of which has already come has no effect
				 *          at all and is not written down (RFC 9114 §7.2.3)
				 *
				 * \~
				 */
				ring_t _cancelledPush;
				/**
				 * \~russian
				 * Идентификаторы обещаний push, потоки которых уже приходили
				 *
				 * @details Идентификатор обещания используется ровно одним потоком: второй
				 *          поток с тем же идентификатором - ошибка соединения (RFC 9114 §4.6).
				 *          Учёт ведётся окном, поэтому повтор ловится в пределах кольца
				 *
				 * \~english
				 * Identifiers of the promises of a push the streams of which have already come
				 * @details An identifier of a promise is used by exactly one stream: a second
				 *          stream with the same identifier is an error of the connection (RFC 9114 §4.6).
				 *          The account is conducted by a window, therefore a repetition is caught within the limits of the ring
				 *
				 * \~
				 */
				ring_t _openedPush;
				/**
				 * \~russian
				 * Отпечатки секций полей уже полученных обещаний push
				 *
				 * @details Один push сервер вправе пообещать на нескольких потоках запросов,
				 *          но секции полей таких обещаний обязаны совпадать: расхождение -
				 *          ошибка соединения (RFC 9114 §7.2.5). Учёт ведётся окном, поэтому
				 *          расхождение ловится в пределах кольца
				 *
				 * \~english
				 * Fingerprints of the sections of the fields of the already obtained promises of a push
				 * @details One push a server is entitled to promise on several streams of the requests,
				 *          but the sections of the fields of such promises are obliged to coincide: a divergence is
				 *          an error of the connection (RFC 9114 §7.2.5). The account is conducted by a window, therefore
				 *          a divergence is caught within the limits of the ring
				 *
				 * \~
				 */
				vector <promise_t> _promisedPush;
				// Позиция записи в кольце обещаний push
				size_t _promisedCursor;
				/**
				 * \~russian
				 * Кольцо приоритетов, объявленных до открытия потока (RFC 9218 §7.2)
				 *
				 * @details Кадр PRIORITY_UPDATE вправе опередить секцию полей: клиент
				 *          отправляет его на управляющем потоке, а порядок между потоками
				 *          QUIC не гарантирует вовсе. Состояния потока под такой сигнал
				 *          не создаётся - до прихода секции это позволило бы пиру наполнить
				 *          карту потоков даром; запись ложится в кольцо, ёмкость которого
				 *          и ограничивает цену сигнала
				 *
				 * \~english
				 * Ring of the priorities announced before the opening of a stream (RFC 9218 §7.2)
				 * @details A PRIORITY_UPDATE frame is entitled to outstrip a section of the fields: a client
				 *          sends it on the control stream, while the order between the streams
				 *          QUIC does not guarantee at all. A state of a stream for such a signal
				 *          is not created - before the arrival of a section this would allow a peer to fill
				 *          the map of the streams for nothing; a record lies down into a ring the capacity of which
				 *          also limits the price of a signal
				 *
				 * \~
				 */
				vector <signal_t> _pendingPriorities;
				/**
				 * \~russian
				 * Кольцо приоритетов обещаний push (RFC 9218 §7.2)
				 *
				 * @details Приоритет адресуется идентификатором обещания, а не потока,
				 *          и состояния потока у него нет вовсе: поток push откроется
				 *          позже и может не открыться никогда. Учёт ведётся окном
				 *          по тем же соображениям, что и учёт самих обещаний
				 *
				 * \~english
				 * Ring of the priorities of the promises of a push (RFC 9218 §7.2)
				 * @details A priority is addressed by an identifier of a promise rather than of a stream,
				 *          and it has no state of a stream at all: a stream of a push will open
				 *          later and may never open. The account is conducted by a window
				 *          by the same considerations as the account of the promises themselves
				 *
				 * \~
				 */
				vector <signal_t> _pushPriorities;
			private:
				// Идентификатор потока, объявленный нами в GOAWAY
				uint64_t _goawayLocal;
				// Идентификатор потока, объявленный пиром в GOAWAY
				uint64_t _goawayRemote;
			private:
				// Лимит частоты управляющих кадров
				ratelim_t _ctrlLimit;
				// Лимит частоты кадров приоритета
				ratelim_t _priorityLimit;
			private:
				// Код последней ошибки протокола
				error_t _error;
			private:
				/**
				 * Поколение состояния соединения: увеличивается каждым сбросом (reset/clear).
				 * Пользовательская функция обратного вызова вправе сбросить парсер прямо
				 * из обработчика - после этого все ссылки на разбираемые данные, состояния
				 * потоков и списки полей недействительны, и разбор обязан свернуться
				 */
				uint64_t _epoch;
			private:
				// Декодированные поля текущей секции (ёмкость переиспользуется)
				vector <h3::qpack::field_view_t> _fields;
				// Буфер сборки исходящей секции полей (ёмкость переиспользуется)
				string _section;
				// Буфер сборки исходящего кадра (ёмкость переиспользуется)
				string _frame;
				/**
				 * \~russian
				 * Накопитель ёмкости под нагрузку принятого кадра
				 *
				 * @details Нагрузка, собранная по кускам, обрабатывается вне буфера
				 *          накопления: обработчик вправе реентрантно продолжить разбор
				 *          того же потока и переиспользовать буфер. Ёмкость при этом
				 *          теряться не должна, поэтому буфер не создаётся заново,
				 *          а изымается отсюда и возвращается после обработки
				 *
				 * \~english
				 * Accumulator of the capacity for the payload of an accepted frame
				 * @details A payload assembled by the pieces is processed outside the buffer
				 *          of the accumulation: a handler is entitled to continue reentrantly the parsing
				 *          of the same stream and to reuse the buffer. The capacity thereby
				 *          is not obliged to be lost, therefore the buffer is not created anew
				 *          but is taken away from here and is returned after the processing
				 *
				 * \~
				 */
				string _payload;
				/**
				 * Накопитель ёмкости для списков идентификаторов потоков. Сам список
				 * на время обхода изымается отсюда: вложенный обход из пользовательской
				 * функции собрал бы свой список поверх нашего
				 */
				vector <uint64_t> _outgoing;
				// Счётчик поколений состояний потоков (защита от пересоздания под тем же адресом)
				uint64_t _generation;
			private:
				/**
				 * \~russian
				 * @brief Метод фиксации ошибки уровня соединения
				 *
				 * @param code    код ошибки протокола
				 * @param message текстовое описание ошибки
				 * @return        результат обработки (всегда ERROR)
				 *
				 * \~english
				 * @brief Method of the fixation of an error of the level of the connection
				 * @param code    error code of the protocol
				 * @param message text description of the error
				 * @return        result of the processing (always ERROR)
				 *
				 * \~
				 */
				h3::status_t fail(const error_t code, const char * message) noexcept;
				/**
				 * \~russian
				 * @brief Метод записи исходящих байтов потока
				 *
				 * @param sid    идентификатор потока
				 * @param buffer буфер исходящих данных
				 * @param size   размер исходящих данных
				 * @param fin    признак завершения потока в исходящем направлении
				 *
				 * \~english
				 * @brief Method of the writing of the outgoing octets of a stream
				 * @param sid    identifier of the stream
				 * @param buffer buffer of the outgoing data
				 * @param size   size of the outgoing data
				 * @param fin    flag of the completion of the stream in the outgoing direction
				 *
				 * \~
				 */
				void emit(const uint64_t sid, const void * buffer, const size_t size, const bool fin) noexcept;
				/**
				 * \~russian
				 * @brief Метод выгрузки накопленных инструкций кодека QPACK
				 *
				 * @details Инструкции обоих потоков QPACK копятся внутри кодека и обязаны
				 *          уходить пиру вне зависимости от того, отправляем ли мы сейчас
				 *          секцию полей: без них таблицы разъедутся
				 *
				 * \~english
				 * @brief Method of the unloading of the accumulated instructions of the QPACK codec
				 * @details The instructions of both the streams of QPACK accumulate inside the codec and are obliged
				 *          to go away to the peer independently of whether we are sending now
				 *          a section of the fields: without them the tables would diverge
				 *
				 * \~
				 */
				void flushQpack() noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод открытия служебных однонаправленных потоков
				 *
				 * @details Открывает управляющий поток и два потока QPACK, если они ещё
				 *          не открыты. Транспорт вправе отказать: попытка повторится позже
				 *
				 * @return признак готовности служебных потоков
				 *
				 * \~english
				 * @brief Method of the opening of the service unidirectional streams
				 * @details It opens the control stream and the two streams of QPACK, if they are not yet
				 *          opened. The transport is entitled to refuse: the attempt will be repeated later
				 * @return flag of the readiness of the service streams
				 *
				 * \~
				 */
				bool prepare() noexcept;
				/**
				 * \~russian
				 * @brief Метод получения состояния потока запроса
				 *
				 * @param sid идентификатор потока
				 * @return    состояние потока запроса
				 *
				 * \~english
				 * @brief Method of getting the state of a stream of a request
				 * @param sid identifier of the stream
				 * @return    state of the stream of the request
				 *
				 * \~
				 */
				stream_t & stream(const uint64_t sid) noexcept;
				/**
				 * \~russian
				 * @brief Метод поиска состояния потока запроса
				 *
				 * @param sid идентификатор потока
				 * @return    состояние потока запроса либо nullptr
				 *
				 * \~english
				 * @brief Method of the search of the state of a stream of a request
				 * @param sid identifier of the stream
				 * @return    state of the stream of the request or nullptr
				 *
				 * \~
				 */
				stream_t * findStream(const uint64_t sid) noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки того, что поток пережил выход в пользовательскую функцию
				 *
				 * @param sid        идентификатор потока
				 * @param generation поколение состояния потока до выхода наружу
				 * @return           признак того, что поток жив и не пересоздан
				 *
				 * \~english
				 * @brief Method of checking that a stream has survived an exit into a user function
				 * @param sid        identifier of the stream
				 * @param generation generation of the state of the stream before the exit outside
				 * @return           flag of the stream being alive and not recreated
				 *
				 * \~
				 */
				bool aliveStream(const uint64_t sid, const uint64_t generation) noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки принадлежности потока инициатору
				 *
				 * @param sid идентификатор потока
				 * @return    признак того, что поток инициирован пиром
				 *
				 * \~english
				 * @brief Method of checking the belonging of a stream to the initiator
				 * @param sid identifier of the stream
				 * @return    flag of the stream being initiated by the peer
				 *
				 * \~
				 */
				bool peerInitiated(const uint64_t sid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки двунаправленности потока
				 *
				 * @param sid идентификатор потока
				 * @return    признак двунаправленного потока
				 *
				 * \~english
				 * @brief Method of checking the bidirectionality of a stream
				 * @param sid identifier of the stream
				 * @return    flag of a bidirectional stream
				 *
				 * \~
				 */
				static bool bidirectional(const uint64_t sid) noexcept;
				/**
				 * \~russian
				 * @brief Метод закрытия потока запроса
				 *
				 * @param sid  идентификатор потока
				 * @param code код ошибки закрытия
				 *
				 * \~english
				 * @brief Method of the closing of a stream of a request
				 * @param sid  identifier of the stream
				 * @param code error code of the closing
				 *
				 * \~
				 */
				void closeStream(const uint64_t sid, const error_t code) noexcept;
				/**
				 * \~russian
				 * @brief Метод закрытия потока по завершении обоих направлений
				 *
				 * @param sid идентификатор потока
				 *
				 * \~english
				 * @brief Method of the closing of a stream at the completion of both the directions
				 * @param sid identifier of the stream
				 *
				 * \~
				 */
				void maybeClose(const uint64_t sid) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод разбора байтов потока запроса
				 *
				 * @param sid    идентификатор потока
				 * @param data   входной буфер
				 * @param size   доступно байт
				 * @param fin    признак завершения потока пиром
				 * @return       результат разбора
				 *
				 * \~english
				 * @brief Method of the parsing of the octets of a stream of a request
				 * @param sid    identifier of the stream
				 * @param data   input buffer
				 * @param size   octets available
				 * @param fin    flag of the completion of the stream by the peer
				 * @return       result of the parsing
				 *
				 * \~
				 */
				h3::status_t parseRequest(const uint64_t sid, const uint8_t * data, const size_t size, const bool fin) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки принятого кадра потока сообщения
				 *
				 * @param sid     идентификатор потока
				 * @param type    тип кадра
				 * @param payload нагрузка кадра
				 * @param size    размер нагрузки кадра
				 * @param last    признак завершения потока вместе с этим кадром
				 * @return        результат обработки
				 *
				 * \~english
				 * @brief Method of the processing of an accepted frame of a stream of a message
				 * @param sid     identifier of the stream
				 * @param type    type of the frame
				 * @param payload payload of the frame
				 * @param size    size of the payload of the frame
				 * @param last    flag of the completion of the stream together with this frame
				 * @return        result of the processing
				 *
				 * \~
				 */
				h3::status_t dispatchMessage(const uint64_t sid, const uint64_t type, const uint8_t * payload, const size_t size, const bool last) noexcept;
				/**
				 * \~russian
				 * @brief Метод применения завершения потока пиром
				 *
				 * @details Завершает приём сообщения: сверяет объявленную длину тела
				 *          с принятой, выпускает завершающие фазы и закрывает поток
				 *
				 * @param sid идентификатор потока
				 * @return    результат применения
				 *
				 * \~english
				 * @brief Method of the application of a completion of a stream by the peer
				 * @details It completes the acceptance of the message: it compares the announced length of the body
				 *          with the accepted one, issues the concluding phases and closes the stream
				 * @param sid identifier of the stream
				 * @return    result of the application
				 *
				 * \~
				 */
				h3::status_t applyFin(const uint64_t sid) noexcept;
				/**
				 * \~russian
				 * @brief Метод разбора байтов однонаправленного потока
				 *
				 * @param sid  идентификатор потока
				 * @param data входной буфер
				 * @param size доступно байт
				 * @param fin  признак завершения потока пиром
				 * @return     результат разбора
				 *
				 * \~english
				 * @brief Method of the parsing of the octets of a unidirectional stream
				 * @param sid  identifier of the stream
				 * @param data input buffer
				 * @param size octets available
				 * @param fin  flag of the completion of the stream by the peer
				 * @return     result of the parsing
				 *
				 * \~
				 */
				h3::status_t parseUnistream(const uint64_t sid, const uint8_t * data, const size_t size, const bool fin) noexcept;
				/**
				 * \~russian
				 * @brief Метод разбора кадров управляющего потока
				 *
				 * @param sid    идентификатор потока
				 * @param stream состояние однонаправленного потока
				 * @param data   входной буфер
				 * @param size   доступно байт
				 * @return       результат разбора
				 *
				 * \~english
				 * @brief Method of the parsing of the frames of the control stream
				 * @param sid    identifier of the stream
				 * @param stream state of the unidirectional stream
				 * @param data   input buffer
				 * @param size   octets available
				 * @return       result of the parsing
				 *
				 * \~
				 */
				h3::status_t parseControl(const uint64_t sid, unistream_t & stream, const uint8_t * data, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки кадра управляющего потока
				 *
				 * @param type    тип кадра
				 * @param payload нагрузка кадра
				 * @param size    размер нагрузки кадра
				 * @return        результат обработки
				 *
				 * \~english
				 * @brief Method of the processing of a frame of the control stream
				 * @param type    type of the frame
				 * @param payload payload of the frame
				 * @param size    size of the payload of the frame
				 * @return        result of the processing
				 *
				 * \~
				 */
				h3::status_t dispatchControl(const uint64_t type, const uint8_t * payload, const size_t size) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод применения полученного набора параметров SETTINGS
				 *
				 * @param items набор параметров
				 * @return      результат применения
				 *
				 * \~english
				 * @brief Method of the application of an obtained collection of the parameters of SETTINGS
				 * @param items collection of the parameters
				 * @return      result of the application
				 *
				 * \~
				 */
				h3::status_t applySettings(const vector <h3::frame::setting_entry_t> & items) noexcept;
				/**
				 * \~russian
				 * @brief Метод повторного разбора заблокированных потоков
				 *
				 * @details Вызывается после обработки инструкций потока кодера QPACK: часть
				 *          отложенных секций могла стать разбираемой
				 *
				 * @return результат разбора
				 *
				 * \~english
				 * @brief Method of a repeated parsing of the blocked streams
				 * @details It is called after the processing of the instructions of the stream of the encoder of QPACK: a part
				 *          of the postponed sections could have become parsable
				 * @return result of the parsing
				 *
				 * \~
				 */
				h3::status_t retryBlocked() noexcept;
				/**
				 * \~russian
				 * @brief Метод накопления неразобранного хвоста заблокированного потока
				 *
				 * @details Кадры, идущие следом за секцией, ждущей вставок QPACK, разобрать
				 *          нельзя: тело до секции недопустимо, а вторая секция затёрла бы
				 *          отложенную. Хвост откладывается до разблокировки потока
				 *
				 * @param sid  идентификатор потока
				 * @param data неразобранный хвост
				 * @param size размер неразобранного хвоста
				 * @param fin  признак завершения потока пиром
				 * @return     результат накопления
				 *
				 * \~english
				 * @brief Method of the accumulation of the unparsed tail of a blocked stream
				 * @details The frames going after a section waiting for the insertions of QPACK cannot be
				 *          parsed: a body before the section is inadmissible, while a second section would erase
				 *          the postponed one. The tail is postponed until the unblocking of the stream
				 * @param sid  identifier of the stream
				 * @param data unparsed tail
				 * @param size size of the unparsed tail
				 * @param fin  flag of the completion of the stream by the peer
				 * @return     result of the accumulation
				 *
				 * \~
				 */
				h3::status_t deferTail(const uint64_t sid, const uint8_t * data, const size_t size, const bool fin) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки принятой секции полей
				 *
				 * @param sid     идентификатор потока
				 * @param section секция полей
				 * @param fin     признак завершения потока
				 * @return        результат обработки
				 *
				 * \~english
				 * @brief Method of the processing of an accepted section of the fields
				 * @param sid     identifier of the stream
				 * @param section section of the fields
				 * @param fin     flag of the completion of the stream
				 * @return        result of the processing
				 *
				 * \~
				 */
				h3::status_t commitSection(const uint64_t sid, string_view section, const bool fin) noexcept;
				/**
				 * \~russian
				 * @brief Метод доставки декодированной секции полей
				 *
				 * @param sid идентификатор потока
				 * @param fin признак завершения потока
				 * @return    результат доставки
				 *
				 * \~english
				 * @brief Method of the delivery of a decoded section of the fields
				 * @param sid identifier of the stream
				 * @param fin flag of the completion of the stream
				 * @return    result of the delivery
				 *
				 * \~
				 */
				h3::status_t deliverSection(const uint64_t sid, const bool fin) noexcept;
				/**
				 * \~russian
				 * @brief Метод доставки декодированной секции обещанного запроса
				 *
				 * @details Обещание push не продвигает состояние потока, на котором пришло:
				 *          оно лишь объявляет запрос, ответ на который придёт отдельным
				 *          однонаправленным потоком
				 *
				 * @param sid    идентификатор ассоциированного потока
				 * @param pushId идентификатор обещанного push
				 * @return       результат доставки
				 *
				 * \~english
				 * @brief Method of the delivery of a decoded section of a promised request
				 * @details A promise of a push does not advance the state of the stream on which it has come:
				 *          it only announces a request the answer to which will come by a separate
				 *          unidirectional stream
				 * @param sid    identifier of the associated stream
				 * @param pushId identifier of the promised push
				 * @return       result of the delivery
				 *
				 * \~
				 */
				/**
				 * \~russian
				 * @brief Метод сверки секции полей повторно обещанного push
				 *
				 * @details Один push сервер вправе пообещать на нескольких потоках запросов,
				 *          но секции полей таких обещаний обязаны совпадать: расхождение -
				 *          ошибка соединения (RFC 9114 §7.2.5). Сверяются отпечатки: хранить
				 *          сами поля значило бы отдать серверу управление нашей памятью
				 *
				 * @param pushId идентификатор обещанного push
				 * @return       результат сверки
				 *
				 * \~english
				 * @brief Method of the comparison of the section of the fields of a repeatedly promised push
				 * @details One push a server is entitled to promise on several streams of the requests,
				 *          but the sections of the fields of such promises are obliged to coincide: a divergence is
				 *          an error of the connection (RFC 9114 §7.2.5). The fingerprints are compared: to store
				 *          the fields themselves would mean to give the server the control of our memory
				 * @param pushId identifier of the promised push
				 * @return       result of the comparison
				 *
				 * \~
				 */
				h3::status_t checkPromise(const uint64_t pushId) noexcept;
				h3::status_t deliverPromise(const uint64_t sid, const uint64_t pushId) noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки семантики секции полей (RFC 9114 §4.1, §4.2)
				 *
				 * @details Секция обещания push проверяется как запрос при любом направлении
				 *          разбора и состояния потока не меняет: обещание приходит на чужой
				 *          поток, а собственного потока у него ещё нет (RFC 9114 §4.6)
				 *
				 * @param sid     идентификатор потока
				 * @param trailer признак секции трейлеров
				 * @param error   код ошибки протокола
				 * @param promise признак секции обещанного запроса
				 * @return        результат проверки
				 *
				 * \~english
				 * @brief Method of checking the semantics of a section of the fields (RFC 9114 §4.1, §4.2)
				 * @details A section of a promise of a push is checked as a request at any direction
				 *          of the parsing and does not change the state of the stream: a promise comes onto a foreign
				 *          stream, while it has no stream of its own yet (RFC 9114 §4.6)
				 * @param sid     identifier of the stream
				 * @param trailer flag of a section of the trailers
				 * @param error   error code of the protocol
				 * @param promise flag of a section of a promised request
				 * @return        result of the checking
				 *
				 * \~
				 */
				bool validateSection(const uint64_t sid, const bool trailer, error_t & error, const bool promise = false) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения лимита распакованной секции полей
				 *
				 * @details Лимит задаётся двумя источниками сразу - настроечным maxHeadersTotal
				 *          и объявленным пиру SETTINGS_MAX_FIELD_SECTION_SIZE. В обоих ноль
				 *          означает "без лимита", поэтому берётся не минимум, а строжайший
				 *          из заданных: иначе maxHeadersTotal == 0 обнулял бы и объявленный
				 *          нами лимит
				 *
				 * @return лимит распакованной секции полей (0 - без лимита)
				 *
				 * \~english
				 * @brief Method of getting the limit of an unpacked section of the fields
				 * @details The limit is set by two sources at once - the configurable maxHeadersTotal
				 *          and the SETTINGS_MAX_FIELD_SECTION_SIZE announced to the peer. In both a zero
				 *          means «without a limit», therefore not the minimum is taken but the strictest
				 *          of the given ones: otherwise maxHeadersTotal == 0 would zero out the limit announced
				 *          by us as well
				 * @return limit of an unpacked section of the fields (0 - without a limit)
				 *
				 * \~
				 */
				uint64_t sectionLimit() const noexcept;
				/**
				 * \~russian
				 * @brief Метод предупреждения о полностью снятом лимите секции полей
				 *
				 * @details Вызывается при изменении лимитов безопасности и параметров SETTINGS:
				 *          снятие обоих лимитов сразу оставляет арену декодера без границы
				 *
				 * \~english
				 * @brief Method of the warning about a fully removed limit of a section of the fields
				 * @details It is called at a change of the limits of the safety and of the parameters of SETTINGS:
				 *          a removal of both limits at once leaves the arena of the decoder without a boundary
				 *
				 * \~
				 */
				void checkFieldSectionLimits() const noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки того, что расширенный CONNECT разрешён нами
				 *
				 * @details Разрешение выдаётся параметром SETTINGS_ENABLE_CONNECT_PROTOCOL
				 *          либо подразумевается ролью узла: соединение, объявленное несущим
				 *          WebSocket, иначе отвергало бы единственный запрос, ради которого
				 *          заведено (RFC 9220 §3)
				 *
				 * @return признак разрешения расширенного CONNECT
				 *
				 * \~english
				 * @brief Method of checking that an extended CONNECT is permitted by us
				 * @details The permission is issued by the parameter SETTINGS_ENABLE_CONNECT_PROTOCOL
				 *          or is implied by the role of the node: a connection announced as carrying
				 *          a WebSocket would otherwise reject the only request for the sake of which
				 *          it is started (RFC 9220 §3)
				 * @return flag of the permission of an extended CONNECT
				 *
				 * \~
				 */
				bool connectProtocol() const noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод дозагрузки буфера отправки потока из pull-источника
				 *
				 * @details Источник опрашивается порциями, пока буфер не заполнен до
				 *          верхней водяной метки либо источник не сообщил конец тела.
				 *          Источник вправе закрыть поток прямо из своего вызова,
				 *          поэтому на время вызова он изымается из состояния потока
				 *
				 * @param sid    идентификатор потока
				 * @param stream объект потока (ссылка может стать недействительной)
				 *
				 * \~english
				 * @brief Method of the loading of the buffer of the sending of a stream from a pull source
				 * @details The source is polled by the portions until the buffer is filled up to
				 *          the upper water mark or the source has reported the end of the body.
				 *          The source is entitled to close the stream right from its own call,
				 *          therefore for the time of the call it is taken away from the state of the stream
				 * @param sid    identifier of the stream
				 * @param stream object of the stream (the reference may become invalid)
				 *
				 * \~
				 */
				void refillFromSource(const uint64_t sid, stream_t & stream) noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки того, что всё тело потока для отправки получено
				 *
				 * @param stream объект потока
				 * @return       результат проверки (источника нет либо достигнут его конец)
				 *
				 * \~english
				 * @brief Method of checking that the whole body of a stream for the sending is obtained
				 * @param stream object of the stream
				 * @return       result of the checking (there is no source or its end is reached)
				 *
				 * \~
				 */
				bool sourceDone(const stream_t & stream) const noexcept;
				/**
				 * \~russian
				 * @brief Метод выдачи накопленного тела потока кадрами DATA
				 *
				 * @details Вызывается всюду, где у потока появляется что отправить либо
				 *          освобождается место наружу: приёмом данных, назначением
				 *          источника и вычитыванием накопленного обвязкой
				 *
				 * @param sid идентификатор потока
				 *
				 * \~english
				 * @brief Method of the issue of the accumulated body of a stream by the DATA frames
				 * @details It is called everywhere where a stream acquires something to send or
				 *          a place outside is freed: by an acceptance of the data, by an assignment
				 *          of a source and by a reading out of the accumulated by the binding
				 * @param sid identifier of the stream
				 *
				 * \~
				 */
				void pumpStream(const uint64_t sid) noexcept;
				/**
				 * \~russian
				 * @brief Метод сигнализации о готовности потока принимать данные
				 *
				 * @param sid    идентификатор потока
				 * @param stream объект потока
				 *
				 * \~english
				 * @brief Method of the signalling about the readiness of a stream to accept the data
				 * @param sid    identifier of the stream
				 * @param stream object of the stream
				 *
				 * \~
				 */
				void maybeNotifyWritable(const uint64_t sid, stream_t & stream) noexcept;
				/**
				 * \~russian
				 * @brief Метод вызова функции обратного вызова готовности потока
				 *
				 * @param sid  идентификатор потока
				 * @param room свободное место в буфере отправки потока
				 *
				 * \~english
				 * @brief Method of the call of the callback function of the readiness of a stream
				 * @param sid  identifier of the stream
				 * @param room free place in the buffer of the sending of the stream
				 *
				 * \~
				 */
				void fireWritable(const uint64_t sid, const size_t room) noexcept;
				/**
				 * \~russian
				 * @brief Метод откладывания секции трейлеров до конца отправки тела
				 *
				 * @param sid       идентификатор потока
				 * @param fields    поля секции трейлеров
				 * @param endStream признак завершения потока
				 * @return          признак откладывания секции
				 *
				 * \~english
				 * @brief Method of the postponement of a section of the trailers to the end of the sending of the body
				 * @param sid       identifier of the stream
				 * @param fields    fields of the section of the trailers
				 * @param endStream flag of the completion of the stream
				 * @return          flag of the postponement of the section
				 *
				 * \~
				 */
				bool deferTrailers(const uint64_t sid, const vector <h3::qpack::field_t> & fields, const bool endStream) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки отложенной секции трейлеров потока
				 *
				 * @param sid идентификатор потока
				 *
				 * \~english
				 * @brief Method of the sending of a postponed section of the trailers of a stream
				 * @param sid identifier of the stream
				 *
				 * \~
				 */
				void flushTrailers(const uint64_t sid) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения объёма накопленных исходящих данных потока
				 *
				 * @param sid идентификатор потока
				 * @return    объём ещё не выданных наружу октетов
				 *
				 * \~english
				 * @brief Method of getting the volume of the accumulated outgoing data of a stream
				 * @param sid identifier of the stream
				 * @return    volume of the octets not yet issued outside
				 *
				 * \~
				 */
				size_t outputPending(const uint64_t sid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод сборки провайдера полей потока
				 *
				 * @param request признак сборки провайдера запроса клиента
				 * @return        собранный провайдер полей потока
				 *
				 * \~english
				 * @brief Method of the assembly of the provider of the fields of a stream
				 * @param request flag of the assembly of a provider of a request of a client
				 * @return        assembled provider of the fields of the stream
				 *
				 * \~
				 */
				unique_ptr <provider_t> buildProvider(const bool request) const noexcept;
				/**
				 * \~russian
				 * @brief Метод применения значения заголовка приоритета (RFC 9218 §5)
				 *
				 * @param stream состояние потока
				 * @param value  значение заголовка приоритета
				 *
				 * \~english
				 * @brief Method of the application of the value of a header of the priority (RFC 9218 §5)
				 * @param stream state of the stream
				 * @param value  value of the header of the priority
				 *
				 * \~
				 */
				void applyPriority(stream_t & stream, string_view value) noexcept;
				/**
				 * \~russian
				 * @brief Метод разбора значения поля расширенного приоритета (RFC 9218 §4)
				 *
				 * @details Сигнал задаёт приоритет целиком: параметр, в нём отсутствующий,
				 *          принимает значение по умолчанию, а не сохраняет прежнее
				 *
				 * @param value       значение поля приоритета
				 * @param urgency     срочность (выходной параметр)
				 * @param incremental признак инкрементальной доставки (выходной параметр)
				 *
				 * \~english
				 * @brief Method of the parsing of the value of the field of an extended priority (RFC 9218 §4)
				 * @details A signal sets the priority entirely: a parameter absent in it
				 *          takes the value by default rather than preserving the previous one
				 * @param value       value of the field of the priority
				 * @param urgency     urgency (an output parameter)
				 * @param incremental flag of the incremental delivery (an output parameter)
				 *
				 * \~
				 */
				void parsePriority(const string_view value, uint8_t & urgency, bool & incremental) const noexcept;
				/**
				 * \~russian
				 * @brief Метод запоминания приоритета ещё не открытого потока (RFC 9218 §7.2)
				 *
				 * @param sid   идентификатор приоритизируемого потока
				 * @param value значение поля приоритета
				 *
				 * \~english
				 * @brief Method of the remembering of the priority of a not yet opened stream (RFC 9218 §7.2)
				 * @param sid   identifier of the stream being prioritized
				 * @param value value of the field of the priority
				 *
				 * \~
				 */
				void deferPriority(const uint64_t sid, const string_view value) noexcept;
				/**
				 * \~russian
				 * @brief Метод применения приоритета, отложенного до открытия потока
				 *
				 * @details Вместе с применённой записью снимаются записи потоков с меньшими
				 *          идентификаторами: идентификаторы потоков запросов строго
				 *          возрастают, и открыты такие потоки уже не будут
				 *
				 * @param sid    идентификатор открываемого потока
				 * @param stream состояние открываемого потока
				 *
				 * \~english
				 * @brief Method of the application of a priority postponed to the opening of a stream
				 * @details Together with the applied record the records of the streams with the smaller
				 *          identifiers are removed: the identifiers of the streams of the requests strictly
				 *          increase, and such streams will no longer be opened
				 * @param sid    identifier of the stream being opened
				 * @param stream state of the stream being opened
				 *
				 * \~
				 */
				void applyPendingPriority(const uint64_t sid, stream_t & stream) noexcept;
				/**
				 * \~russian
				 * @brief Метод применения приоритета обещания push (RFC 9218 §7.2)
				 *
				 * @param pushId идентификатор обещания push
				 * @param value  значение поля приоритета
				 *
				 * \~english
				 * @brief Method of the application of the priority of a promise of a push (RFC 9218 §7.2)
				 * @param pushId identifier of the promise of the push
				 * @param value  value of the field of the priority
				 *
				 * \~
				 */
				void applyPushPriority(const uint64_t pushId, const string_view value) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод вызова функции обратного вызова фазы приёма сообщения потока
				 *
				 * @param sid   идентификатор потока
				 * @param phase фаза приёма сообщения потока
				 * @param part  часть сообщения
				 * @return      результат вызова (false - поток обрывается)
				 *
				 * \~english
				 * @brief Method of the call of the callback function of the phase of the acceptance of a message of a stream
				 * @param sid   identifier of the stream
				 * @param phase phase of the acceptance of the message of the stream
				 * @param part  part of the message
				 * @return      result of the call (false - the stream is broken)
				 *
				 * \~
				 */
				bool firePhase(const uint64_t sid, const phase_t phase, const part_t part) noexcept;
				/**
				 * \~russian
				 * @brief Метод вызова функции обратного вызова открытия нового потока
				 *
				 * @param sid идентификатор потока
				 * @return    результат вызова (false - поток обрывается)
				 *
				 * \~english
				 * @brief Method of the call of the callback function of the opening of a new stream
				 * @param sid identifier of the stream
				 * @return    result of the call (false - the stream is broken)
				 *
				 * \~
				 */
				bool fireBegin(const uint64_t sid) noexcept;
				/**
				 * \~russian
				 * @brief Метод вызова функции обратного вызова анонса server push
				 *
				 * @param sid    идентификатор ассоциированного потока
				 * @param pushId идентификатор обещанного push
				 * @return       результат вызова (false - push отклоняется)
				 *
				 * \~english
				 * @brief Method of the call of the callback function of an announcement of a server push
				 * @param sid    identifier of the associated stream
				 * @param pushId identifier of the promised push
				 * @return       result of the call (false - the push is rejected)
				 *
				 * \~
				 */
				bool firePush(const uint64_t sid, const uint64_t pushId) noexcept;
				/**
				 * \~russian
				 * @brief Метод вызова функции обратного вызова провайдера полей потока
				 *
				 * @param sid       идентификатор потока
				 * @param provider  провайдер полей потока
				 * @param endStream признак завершения потока
				 * @return          результат вызова (false - поток обрывается)
				 *
				 * \~english
				 * @brief Method of the call of the callback function of the provider of the fields of a stream
				 * @param sid       identifier of the stream
				 * @param provider  provider of the fields of the stream
				 * @param endStream flag of the completion of the stream
				 * @return          result of the call (false - the stream is broken)
				 *
				 * \~
				 */
				bool fireProvider(const uint64_t sid, const provider_t * provider, const bool endStream) noexcept;
				/**
				 * \~russian
				 * @brief Метод вызова функции обратного вызова поля секции
				 *
				 * @param sid   идентификатор потока
				 * @param name  название поля
				 * @param value значение поля
				 * @param part  часть сообщения
				 * @return      результат вызова (false - поток обрывается)
				 *
				 * \~english
				 * @brief Method of the call of the callback function of a field of a section
				 * @param sid   identifier of the stream
				 * @param name  name of the field
				 * @param value value of the field
				 * @param part  part of the message
				 * @return      result of the call (false - the stream is broken)
				 *
				 * \~
				 */
				bool fireHeader(const uint64_t sid, const string_view name, const string_view value, const part_t part) noexcept;
				/**
				 * \~russian
				 * @brief Метод вызова функции обратного вызова фрагмента тела потока
				 *
				 * @param sid       идентификатор потока
				 * @param buffer    буфер данных тела
				 * @param size      размер данных тела
				 * @param endStream признак завершения потока
				 * @return          результат вызова (false - поток обрывается)
				 *
				 * \~english
				 * @brief Method of the call of the callback function of a fragment of the body of a stream
				 * @param sid       identifier of the stream
				 * @param buffer    buffer of the data of the body
				 * @param size      size of the data of the body
				 * @param endStream flag of the completion of the stream
				 * @return          result of the call (false - the stream is broken)
				 *
				 * \~
				 */
				bool fireData(const uint64_t sid, const void * buffer, const size_t size, const bool endStream) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод очистки состояния парсера
				 *
				 * \~english
				 * @brief Method of the clearing of the state of the parser
				 *
				 * \~
				 */
				void clear() noexcept override;
				/**
				 * \~russian
				 * @brief Метод сброса состояния парсера
				 *
				 * \~english
				 * @brief Method of the reset of the state of the parser
				 *
				 * \~
				 */
				void reset() noexcept override;
				/**
				 * \~russian
				 * @brief Метод создания копии парсера
				 *
				 * @return копия парсера
				 *
				 * \~english
				 * @brief Method of the creation of a copy of the parser
				 * @return copy of the parser
				 *
				 * \~
				 */
				unique_ptr <parser_t> clone() const noexcept override;
				/**
				 * \~russian
				 * @brief Метод обработки завершения ввода
				 *
				 * \~english
				 * @brief Method of the processing of the completion of the input
				 *
				 * \~
				 */
				void eof() noexcept override;
				/**
				 * \~russian
				 * @brief Метод получения названия кода последней ошибки
				 *
				 * @return название кода последней ошибки
				 *
				 * \~english
				 * @brief Method of getting the name of the code of the last error
				 * @return name of the code of the last error
				 *
				 * \~
				 */
				string_view errorName() const noexcept override;
				/**
				 * \~russian
				 * @brief Метод разбора данных соединения
				 *
				 * @details Единого байтового потока у соединения HTTP/3 нет: данные всегда
				 *          принадлежат конкретному потоку QUIC. Наследуемая сигнатура
				 *          неприменима и намеренно завершается ошибкой, а не молчаливым
				 *          отказом: тихий отказ выглядел бы как исправная работа
				 *
				 * @param buffer буфер данных
				 * @param size   размер данных
				 * @return       количество разобранных байт (всегда 0)
				 *
				 * \~english
				 * @brief Method of the parsing of the data of the connection
				 * @details A connection of HTTP/3 has no single octet stream: the data always
				 *          belongs to a particular stream of QUIC. The inherited signature
				 *          is inapplicable and deliberately completes with an error rather than with a silent
				 *          refusal: a quiet refusal would look like a correct work
				 * @param buffer buffer of the data
				 * @param size   size of the data
				 * @return       number of the parsed octets (always 0)
				 *
				 * \~
				 */
				size_t parse(const void * buffer, const size_t size) noexcept override;
			public:
				/**
				 * \~russian
				 * @brief Метод разбора данных потока
				 *
				 * @param sid    идентификатор потока QUIC
				 * @param buffer буфер данных потока
				 * @param size   размер данных потока
				 * @param fin    признак завершения потока пиром
				 * @return       результат разбора (OK/ERROR)
				 *
				 * \~english
				 * @brief Method of the parsing of the data of a stream
				 * @param sid    identifier of the stream of QUIC
				 * @param buffer buffer of the data of the stream
				 * @param size   size of the data of the stream
				 * @param fin    flag of the completion of the stream by the peer
				 * @return       result of the parsing (OK/ERROR)
				 *
				 * \~
				 */
				h3::status_t parse(const uint64_t sid, const void * buffer, const size_t size, const bool fin) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки обрыва потока пиром
				 *
				 * @details Вызывается обвязкой при получении кадра RESET_STREAM либо
				 *          STOP_SENDING транспорта
				 *
				 * @param sid  идентификатор потока
				 * @param code код ошибки, с которым поток оборван
				 *
				 * \~english
				 * @brief Method of the processing of a breaking of a stream by the peer
				 * @details It is called by the binding at the receipt of a RESET_STREAM or a
				 *          STOP_SENDING frame of the transport
				 * @param sid  identifier of the stream
				 * @param code error code with which the stream is broken
				 *
				 * \~
				 */
				void aborted(const uint64_t sid, const uint64_t code) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения кода последней ошибки протокола
				 *
				 * @return код последней ошибки протокола
				 *
				 * \~english
				 * @brief Method of getting the code of the last error of the protocol
				 * @return code of the last error of the protocol
				 *
				 * \~
				 */
				error_t error() const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения названия кода ошибки протокола
				 *
				 * @param error код ошибки протокола
				 * @return      название кода ошибки протокола
				 *
				 * \~english
				 * @brief Method of getting the name of an error code of the protocol
				 * @param error error code of the protocol
				 * @return      name of the error code of the protocol
				 *
				 * \~
				 */
				static string_view errorName(const error_t error) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения протокола, с которым работает парсер
				 *
				 * @return протокол работы парсера
				 *
				 * \~english
				 * @brief Method of getting the protocol with which the parser works
				 * @return protocol of the work of the parser
				 *
				 * \~
				 */
				proto_t proto() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки протокола, с которым работает парсер
				 *
				 * @details Парсер говорит только на HTTP/3, поэтому допустимы лишь
				 *          значения этого семейства: HTTP3 - прямое соединение с узлом,
				 *          PROXY3 - работа промежуточным узлом, WEBSOCKET3 - соединение,
				 *          несущее WebSocket поверх расширенного CONNECT. Значение
				 *          любого другого семейства отвергается с записью в лог:
				 *          разбирать HTTP/1.x либо HTTP/2 этот парсер не умеет, и
				 *          молчаливое принятие такого указания создало бы у вызывающей
				 *          стороны ложное представление о происходящем.
				 *          По умолчанию установлен HTTP3.
				 *
				 * @note Режим промежуточного узла ужесточает приём: конечному получателю
				 *       достаточно минимальной проверки полей, а узлу, передающему
				 *       сообщение дальше в другой версии протокола, - нет. Отвергается
				 *       то, границу чего следующее звено определит иначе: управляющие
				 *       символы в значениях полей, пробелы и управляющие символы в
				 *       псевдо-заголовках, из которых собирается стартовая строка, а
				 *       также объявленная длина тела у ответов 1xx и 204
				 *       (RFC 9114 §10.3). Подробности - в README модуля
				 *
				 * @param proto протокол работы парсера
				 *
				 * \~english
				 * @brief Method of setting the protocol with which the parser works
				 * @details The parser speaks only HTTP/3, therefore only the values
				 *          of this family are admissible: HTTP3 - a direct connection with a node,
				 *          PROXY3 - a work as an intermediate node, WEBSOCKET3 - a connection
				 *          carrying a WebSocket over an extended CONNECT. A value
				 *          of any other family is rejected with a record into the log:
				 *          this parser is not able to parse HTTP/1.x or HTTP/2, and
				 *          a silent acceptance of such an indication would create at the calling
				 *          side a false notion of what is happening.
				 *          By default HTTP3 is set.
				 * @note The mode of an intermediate node toughens the acceptance: for a final receiver
				 *       a minimal check of the fields suffices, while for a node passing
				 *       a message onward in another version of the protocol - not. That is rejected
				 *       the boundary of which the next link will determine differently: the control
				 *       characters in the values of the fields, the spaces and the control characters in
				 *       the pseudo headers out of which the starting line is assembled, and
				 *       also an announced length of the body at the answers 1xx and 204
				 *       (RFC 9114 §10.3). The details are in the README of the module
				 * @param proto protocol of the work of the parser
				 *
				 * \~
				 */
				void proto(const proto_t proto) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения лимитов безопасности парсера
				 *
				 * @return лимиты безопасности парсера
				 *
				 * \~english
				 * @brief Method of getting the limits of the safety of the parser
				 * @return limits of the safety of the parser
				 *
				 * \~
				 */
				const limits_t & limits() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки лимитов безопасности парсера
				 *
				 * @param limits лимиты безопасности парсера
				 *
				 * \~english
				 * @brief Method of setting the limits of the safety of the parser
				 * @param limits limits of the safety of the parser
				 *
				 * \~
				 */
				void limits(const limits_t & limits) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения наших параметров SETTINGS
				 *
				 * @return наши параметры SETTINGS
				 *
				 * \~english
				 * @brief Method of getting our parameters of SETTINGS
				 * @return our parameters of SETTINGS
				 *
				 * \~
				 */
				const settings_t & settings() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки наших параметров SETTINGS
				 *
				 * @note Вызывается до отправки SETTINGS: после отправки параметры менять нельзя,
				 *       кадр SETTINGS в соединении единственный (RFC 9114 §7.2.4)
				 *
				 * @param settings наши параметры SETTINGS
				 *
				 * \~english
				 * @brief Method of setting our parameters of SETTINGS
				 * @note It is called before the sending of the SETTINGS: after the sending the parameters cannot be changed,
				 *       the SETTINGS frame in a connection is the only one (RFC 9114 §7.2.4)
				 * @param settings our parameters of SETTINGS
				 *
				 * \~
				 */
				void settings(const settings_t & settings) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения параметров SETTINGS пира
				 *
				 * @return параметры SETTINGS пира
				 *
				 * \~english
				 * @brief Method of getting the parameters of SETTINGS of the peer
				 * @return parameters of SETTINGS of the peer
				 *
				 * \~
				 */
				const settings_t & remoteSettings() const noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки получения SETTINGS от пира
				 *
				 * @return признак получения SETTINGS от пира
				 *
				 * \~english
				 * @brief Method of checking the receipt of a SETTINGS from the peer
				 * @return flag of the receipt of a SETTINGS from the peer
				 *
				 * \~
				 */
				bool isSettingsReceived() const noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки завершённости соединения
				 *
				 * @return признак завершённости соединения
				 *
				 * \~english
				 * @brief Method of checking the completeness of the connection
				 * @return flag of the completeness of the connection
				 *
				 * \~
				 */
				bool isClosed() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод отправки параметров соединения
				 *
				 * @details Открывает управляющий поток и два потока QPACK и записывает в
				 *          управляющий поток кадр SETTINGS. Вызывается один раз в начале
				 *          соединения; повторный вызов ничего не делает
				 *
				 * \~english
				 * @brief Method of the sending of the parameters of the connection
				 * @details It opens the control stream and the two streams of QPACK and writes into
				 *          the control stream a SETTINGS frame. It is called once at the beginning of the
				 *          connection; a repeated call does nothing
				 *
				 * \~
				 */
				void sendSettings() noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки секции полей потока
				 *
				 * @param sid       идентификатор потока
				 * @param fields    поля секции (псевдо-заголовки должны идти первыми)
				 * @param endStream признак завершения потока
				 *
				 * \~english
				 * @brief Method of the sending of a section of the fields of a stream
				 * @param sid       identifier of the stream
				 * @param fields    fields of the section (the pseudo headers are obliged to go first)
				 * @param endStream flag of the completion of the stream
				 *
				 * \~
				 */
				void sendHeaders(const uint64_t sid, const vector <h3::qpack::field_t> & fields, const bool endStream) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки секции полей потока из провайдера
				 *
				 * @param sid       идентификатор потока
				 * @param headers   набор заголовков сообщения
				 * @param endStream признак завершения потока
				 * @param scheme    схема запроса для псевдо-заголовка [:scheme]
				 *
				 * \~english
				 * @brief Method of the sending of a section of the fields of a stream out of a provider
				 * @param sid       identifier of the stream
				 * @param headers   collection of the headers of the message
				 * @param endStream flag of the completion of the stream
				 * @param scheme    scheme of the request for the pseudo header [:scheme]
				 *
				 * \~
				 */
				void sendHeaders(const uint64_t sid, const headers_t & headers, const bool endStream, string_view scheme = "https") noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки данных тела потока
				 *
				 * @param sid       идентификатор потока
				 * @param buffer    буфер данных тела
				 * @param size      размер данных тела
				 * @param endStream признак завершения потока
				 * @return          количество принятых к отправке байт
				 *
				 * \~english
				 * @brief Method of the sending of the data of the body of a stream
				 * @param sid       identifier of the stream
				 * @param buffer    buffer of the data of the body
				 * @param size      size of the data of the body
				 * @param endStream flag of the completion of the stream
				 * @return          number of the octets accepted for the sending
				 *
				 * \~
				 */
				size_t sendData(const uint64_t sid, const void * buffer, const size_t size, const bool endStream) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки анонса server push (только сервер)
				 *
				 * @param sid    идентификатор ассоциированного потока запроса
				 * @param fields поля обещанного запроса
				 * @return       идентификатор обещанного push либо UINT64_MAX при отказе
				 *
				 * \~english
				 * @brief Method of the sending of an announcement of a server push (only a server)
				 * @param sid    identifier of the associated stream of the request
				 * @param fields fields of the promised request
				 * @return       identifier of the promised push or UINT64_MAX at a refusal
				 *
				 * \~
				 */
				uint64_t sendPushPromise(const uint64_t sid, const vector <h3::qpack::field_t> & fields) noexcept;
				/**
				 * \~russian
				 * @brief Метод отмены обещанного push
				 *
				 * @param pushId идентификатор отменяемого push
				 *
				 * \~english
				 * @brief Method of the cancellation of a promised push
				 * @param pushId identifier of the push being cancelled
				 *
				 * \~
				 */
				void sendCancelPush(const uint64_t pushId) noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки отменённости обещанного push
				 *
				 * @details Серверу отвечает, отказался ли клиент от обещания кадром
				 *          CANCEL_PUSH: поток такого push открывать уже незачем.
				 *          Клиенту отвечает, отменено ли обещание им самим либо сервером
				 *
				 * @param pushId идентификатор обещанного push
				 * @return       признак отменённости обещания
				 *
				 * \~english
				 * @brief Method of checking the cancelledness of a promised push
				 * @details To a server it answers whether the client has refused the promise by a
				 *          CANCEL_PUSH frame: there is no longer any point in opening the stream of such a push.
				 *          To a client it answers whether the promise is cancelled by itself or by the server
				 * @param pushId identifier of the promised push
				 * @return       flag of the cancelledness of the promise
				 *
				 * \~
				 */
				bool pushCancelled(const uint64_t pushId) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения расширенного приоритета потока (RFC 9218 §4)
				 *
				 * @details Очерёдность отправки в HTTP/3 определяет транспорт, а не HTTP,
				 *          поэтому принятый приоритет модуль только разбирает и хранит -
				 *          распорядиться им может лишь обвязка. Для неизвестного потока
				 *          выводятся значения по умолчанию
				 *
				 * @param sid идентификатор потока
				 * @return    расширенный приоритет потока
				 *
				 * \~english
				 * @brief Method of getting the extended priority of a stream (RFC 9218 §4)
				 * @details The order of the sending in HTTP/3 is determined by the transport rather than by HTTP,
				 *          therefore an accepted priority the module only parses and stores -
				 *          to dispose of it only the binding can. For an unknown stream
				 *          the values by default are output
				 * @param sid identifier of the stream
				 * @return    extended priority of the stream
				 *
				 * \~
				 */
				priority_t priority(const uint64_t sid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения расширенного приоритета обещания push (RFC 9218 §4)
				 *
				 * @details Приоритет push адресуется идентификатором обещания, а не потока:
				 *          поток push откроется позже и может не открыться вовсе. Для
				 *          неизвестного обещания выводятся значения по умолчанию
				 *
				 * @param pushId идентификатор обещания push
				 * @return       расширенный приоритет обещания push
				 *
				 * \~english
				 * @brief Method of getting the extended priority of a promise of a push (RFC 9218 §4)
				 * @details The priority of a push is addressed by an identifier of a promise rather than of a stream:
				 *          a stream of a push will open later and may never open. For
				 *          an unknown promise the values by default are output
				 * @param pushId identifier of the promise of the push
				 * @return       extended priority of the promise of the push
				 *
				 * \~
				 */
				priority_t pushPriority(const uint64_t pushId) const noexcept;
				/**
				 * \~russian
				 * @brief Метод разрешения пиру выдавать push (только клиент)
				 *
				 * @param pushId наибольший разрешённый идентификатор push
				 *
				 * \~english
				 * @brief Method of the permission to the peer to issue a push (only a client)
				 * @param pushId largest permitted identifier of a push
				 *
				 * \~
				 */
				void sendMaxPushId(const uint64_t pushId) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки приоритета потока (RFC 9218 §7.2)
				 *
				 * @param sid         идентификатор потока
				 * @param urgency     срочность потока (0 - наивысшая, 7 - наименьшая)
				 * @param incremental признак инкрементального потока
				 *
				 * \~english
				 * @brief Method of the sending of the priority of a stream (RFC 9218 §7.2)
				 * @param sid         identifier of the stream
				 * @param urgency     urgency of the stream (0 - the highest, 7 - the least)
				 * @param incremental flag of an incremental stream
				 *
				 * \~
				 */
				void sendPriority(const uint64_t sid, const uint8_t urgency, const bool incremental) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки приоритета обещания push (RFC 9218 §7.2)
				 *
				 * @details Отдельный метод, а не признак у sendPriority(): кадры разных
				 *          типов адресуют разные пространства идентификаторов, и булев
				 *          признак на границе API читался бы на месте вызова хуже имени
				 *
				 * @param pushId      идентификатор обещания push
				 * @param urgency     срочность (0 - наивысшая, 7 - наименьшая)
				 * @param incremental признак инкрементальной доставки
				 *
				 * \~english
				 * @brief Method of the sending of the priority of a promise of a push (RFC 9218 §7.2)
				 * @details A separate method rather than a flag at sendPriority(): the frames of the different
				 *          types address the different spaces of the identifiers, and a boolean
				 *          flag at the boundary of the API would read at the place of the call worse than a name
				 * @param pushId      identifier of the promise of the push
				 * @param urgency     urgency (0 - the highest, 7 - the least)
				 * @param incremental flag of the incremental delivery
				 *
				 * \~
				 */
				void sendPushPriority(const uint64_t pushId, const uint8_t urgency, const bool incremental) noexcept;
				/**
				 * \~russian
				 * @brief Метод обрыва потока
				 *
				 * @param sid  идентификатор потока
				 * @param code код ошибки, с которым обрывается поток
				 *
				 * \~english
				 * @brief Method of the breaking of a stream
				 * @param sid  identifier of the stream
				 * @param code error code with which the stream is broken
				 *
				 * \~
				 */
				void sendReset(const uint64_t sid, const error_t code) noexcept;
				/**
				 * \~russian
				 * @brief Метод завершения соединения (RFC 9114 §5.2)
				 *
				 * @param id идентификатор потока запроса (от сервера) либо push (от клиента)
				 *
				 * \~english
				 * @brief Method of the completion of the connection (RFC 9114 §5.2)
				 * @param id identifier of a stream of a request (from a server) or of a push (from a client)
				 *
				 * \~
				 */
				void sendGoaway(const uint64_t id) noexcept;
				/**
				 * \~russian
				 * @brief Метод плавного завершения соединения
				 *
				 * @details Отправляет GOAWAY с предельным идентификатором: пир прекращает
				 *          открывать новые потоки, а уже открытые доживают штатно. Итоговый
				 *          GOAWAY с фактическим идентификатором отправляется позже
				 *
				 * \~english
				 * @brief Method of a smooth completion of the connection
				 * @details It sends a GOAWAY with the limiting identifier: the peer ceases
				 *          to open the new streams, while the already opened ones live out regularly. The resulting
				 *          GOAWAY with the actual identifier is sent later
				 *
				 * \~
				 */
				void sendShutdown() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод обновления момента времени для частотных лимитов
				 *
				 * @param seconds текущий момент времени в секундах
				 *
				 * \~english
				 * @brief Method of the updating of the moment of the time for the frequency limits
				 * @param seconds current moment of the time in seconds
				 *
				 * \~
				 */
				void updateTime(const uint64_t seconds) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения списка потоков с накопленными исходящими данными
				 *
				 * @details Нужен в pull-модели, когда функция обратного вызова записи
				 *          не установлена
				 *
				 * @param output список идентификаторов потоков
				 *
				 * \~english
				 * @brief Method of getting the list of the streams with the accumulated outgoing data
				 * @details It is needed in the pull model, when the callback function of the writing
				 *          is not set
				 * @param output list of the identifiers of the streams
				 *
				 * \~
				 */
				void outgoing(vector <uint64_t> & output) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения накопленных исходящих данных потока
				 *
				 * @param sid идентификатор потока
				 * @return    представление накопленных исходящих данных
				 *
				 * \~english
				 * @brief Method of getting the accumulated outgoing data of a stream
				 * @param sid identifier of the stream
				 * @return    representation of the accumulated outgoing data
				 *
				 * \~
				 */
				string_view pending(const uint64_t sid) noexcept;
				/**
				 * \~russian
				 * @brief Метод отметки исходящих данных потока как отправленных
				 *
				 * @param sid  идентификатор потока
				 * @param size количество отправленных октетов
				 *
				 * \~english
				 * @brief Method of marking the outgoing data of a stream as sent
				 * @param sid  identifier of the stream
				 * @param size number of the sent octets
				 *
				 * \~
				 */
				void consumePending(const uint64_t sid, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод назначения pull-источника данных тела потока
				 *
				 * @details Источник пишет данные напрямую в буфер отправки потока и
				 *          возвращает число записанных байт (отрицательное значение
				 *          обрывает поток). Закрывать поток прямо из источника допустимо,
				 *          но переданный ему буфер после этого уничтожен - записывать
				 *          в него уже нельзя. Пока источник назначен, сигнал готовности
				 *          потоку не подаётся: опрашивает источник сам парсер
				 *
				 * @param sid    идентификатор потока
				 * @param source pull-источник данных тела
				 *
				 * \~english
				 * @brief Method of the assignment of the pull source of the data of the body of a stream
				 * @details The source writes the data directly into the buffer of the sending of the stream and
				 *          returns the number of the written octets (a negative value
				 *          breaks the stream). To close the stream right from the source is admissible,
				 *          but the buffer transmitted to it is destroyed after this - to write
				 *          into it is no longer possible. While the source is assigned, the signal of the readiness
				 *          is not supplied to the stream: the source is polled by the parser itself
				 * @param sid    identifier of the stream
				 * @param source pull source of the data of the body
				 *
				 * \~
				 */
				void dataSource(const uint64_t sid, data_source_callback_t source) noexcept;
				/**
				 * \~russian
				 * @brief Метод возобновления выдачи тела потока
				 *
				 * @details Источник, вернувший ноль без признака конца тела, объявил себя
				 *          временно пустым, и парсер перестаёт его опрашивать. Заново он
				 *          опрашивается вычитыванием накопленного (consumePending) - а если
				 *          байты забирает функция обратного вызова записи, накапливать
				 *          нечего, и вычитывания не будет. Этот метод и есть недостающая
				 *          точка возобновления: приложение вызывает его, когда у источника
				 *          снова появились данные
				 *
				 * @param sid идентификатор потока
				 *
				 * \~english
				 * @brief Method of the resumption of the issue of the body of a stream
				 * @details A source which has returned a zero without a flag of the end of the body has declared itself
				 *          temporarily empty, and the parser ceases to poll it. Anew it
				 *          is polled by a reading out of the accumulated (consumePending) - and if
				 *          the octets are taken by the callback function of the writing, there is nothing to accumulate,
				 *          and there will be no reading out. This method is exactly the lacking
				 *          point of the resumption: the application calls it when the source
				 *          has data again
				 * @param sid identifier of the stream
				 *
				 * \~
				 */
				void resume(const uint64_t sid) noexcept;
				/**
				 * \~russian
				 * @brief Метод настройки порогов буфера отправки потока
				 *
				 * @param high ёмкость буфера отправки потока (high-water)
				 * @param low  порог сигнала готовности (low-water)
				 *
				 * \~english
				 * @brief Method of the configuration of the thresholds of the buffer of the sending of a stream
				 * @param high capacity of the buffer of the sending of the stream (high-water)
				 * @param low  threshold of the signal of the readiness (low-water)
				 *
				 * \~
				 */
				void sendWaterMarks(const size_t high, const size_t low) noexcept;
				/**
				 * \~russian
				 * @brief Метод настройки порога накопленных исходящих данных потока
				 *
				 * @details Действует только в pull-модели: в push-модели байты забирает
				 *          функция обратного вызова записи, и накапливать нечего
				 *
				 * @param high порог накопленных исходящих данных потока
				 *
				 * \~english
				 * @brief Method of the configuration of the threshold of the accumulated outgoing data of a stream
				 * @details It is in force only in the pull model: in the push model the octets are taken by
				 *          the callback function of the writing, and there is nothing to accumulate
				 * @param high threshold of the accumulated outgoing data of the stream
				 *
				 * \~
				 */
				void outputHighWater(const size_t high) noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки завершения потока в исходящем направлении
				 *
				 * @param sid идентификатор потока
				 * @return    признак того, что поток закрыт нами и данных больше не будет
				 *
				 * \~english
				 * @brief Method of checking the completion of a stream in the outgoing direction
				 * @param sid identifier of the stream
				 * @return    flag of the stream being closed by us and there being no more data
				 *
				 * \~
				 */
				bool finished(const uint64_t sid) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова открытия однонаправленного потока
				 *
				 * @param callback функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback function of the opening of a unidirectional stream
				 * @param callback callback function
				 *
				 * \~
				 */
				void on(open_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова записи исходящих байтов
				 *
				 * @param callback функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback function of the writing of the outgoing octets
				 * @param callback callback function
				 *
				 * \~
				 */
				void on(write_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова обрыва потока
				 *
				 * @param callback функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback function of the breaking of a stream
				 * @param callback callback function
				 *
				 * \~
				 */
				void on(abort_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова применённого SETTINGS пира
				 *
				 * @param callback функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback function of an applied SETTINGS of the peer
				 * @param callback callback function
				 *
				 * \~
				 */
				void on(settings_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова открытия нового потока
				 *
				 * @param callback функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback function of the opening of a new stream
				 * @param callback callback function
				 *
				 * \~
				 */
				void on(begin_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова закрытия потока
				 *
				 * @param callback функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback function of the closing of a stream
				 * @param callback callback function
				 *
				 * \~
				 */
				void on(close_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова ошибки уровня соединения
				 *
				 * @param callback функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback function of an error of the level of the connection
				 * @param callback callback function
				 *
				 * \~
				 */
				void on(error_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова полученного GOAWAY
				 *
				 * @param callback функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback function of an obtained GOAWAY
				 * @param callback callback function
				 *
				 * \~
				 */
				void on(goaway_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова анонса server push
				 *
				 * @param callback функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback function of an announcement of a server push
				 * @param callback callback function
				 *
				 * \~
				 */
				void on(push_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова фазы приёма сообщения потока
				 *
				 * @param callback функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback function of the phase of the acceptance of a message of a stream
				 * @param callback callback function
				 *
				 * \~
				 */
				void on(phase_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова провайдера полей потока
				 *
				 * @param callback функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback function of the provider of the fields of a stream
				 * @param callback callback function
				 *
				 * \~
				 */
				void on(provider_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова поля секции
				 *
				 * @param callback функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback function of a field of a section
				 * @param callback callback function
				 *
				 * \~
				 */
				void on(header_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова фрагмента тела потока
				 *
				 * @param callback функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback function of a fragment of the body of a stream
				 * @param callback callback function
				 *
				 * \~
				 */
				void on(data_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова готовности потока принимать данные
				 *
				 * @param callback функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback function of the readiness of a stream to accept the data
				 * @param callback callback function
				 *
				 * \~
				 */
				void on(writable_callback_t callback) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param direct направление разбора сообщений
				 * @param fmk    объект фреймворка
				 * @param log    объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param direct direction of the parsing of the messages
				 * @param fmk    object of the framework
				 * @param log    object for the work with the logs
				 *
				 * \~
				 */
				explicit Parser_HTTP3(const direct_t direct, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				~Parser_HTTP3() noexcept = default;
		} parser_http3_t;
	};
};

#endif // __AWH_HTTP_PARSER_HTTP3__
