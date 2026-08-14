/**
 * @file http.hpp
 * @date 2026-07-19
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
 * \~russian
 * @brief Заголовочный файл парсера сессии HTTP/2 — класс Parser_HTTP2, управляющий состояниями потоков,
 *        окнами flow control, параметрами SETTINGS, приоритетами,
 *        частотными лимитами и лимитами на размер распакованных заголовков и тела
 *
 * \~english
 * @brief Header file of the parser of an HTTP/2 session — the class Parser_HTTP2 controlling the states of the streams,
 *        the windows of the flow control, the parameters of SETTINGS, the priorities,
 *        the frequency limits and the limits on the size of the unpacked headers and of the body
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_HTTP_PARSER_HTTP2__
#define __AWH_HTTP_PARSER_HTTP2__

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
#include "h2.hpp"
#include "frame.hpp"
#include "hpack.hpp"
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
		 * @brief Класс парсера протокола HTTP/2 (RFC 9113)
		 *
		 * @details В отличие от парсера HTTP/1.x работает на уровне СОЕДИНЕНИЯ, а не одного
		 *          сообщения: HTTP/2 мультиплексирует множество потоков (сообщений) в одном
		 *          соединении, поэтому все события сопровождаются идентификатором потока.
		 *          Парсер владеет состоянием соединения: HPACK-кодером/декодером, картой
		 *          потоков, окнами flow control и согласованными SETTINGS.
		 *
		 *          Протокол двунаправленный: разбор входящих фреймов порождает обязательные
		 *          исходящие (SETTINGS ACK, PING ACK, WINDOW_UPDATE, RST_STREAM, GOAWAY),
		 *          поэтому у парсера есть канал записи - функция обратного вызова записи
		 *          (write_callback_t) либо pull-модель pending()/consumePending().
		 *
		 *          Направление трафика задаёт роль эндпоинта:
		 *          - direct_t::REQUEST  - разбираем запросы клиента (мы - сервер);
		 *          - direct_t::RESPONSE - разбираем ответы сервера (мы - клиент).
		 *
		 *          Встроенные защиты от DoS: лимит частоты RST_STREAM (Rapid Reset,
		 *          CVE-2023-44487), лимит частоты управляющих фреймов (SETTINGS/PING/пустые
		 *          DATA flood), лимиты сборки блока заголовков (CONTINUATION flood, 2024),
		 *          лимит распакованного списка заголовков (decompression bomb) и лимит
		 *          размера тела потока.
		 *
		 * \~english
		 * @brief Class of the parser of the HTTP/2 protocol (RFC 9113)
		 * @details Unlike the parser of HTTP/1.x it works at the level of a CONNECTION rather than of a single
		 *          message: HTTP/2 multiplexes a multitude of the streams (the messages) in one
		 *          connection, therefore all the events are accompanied by an identifier of the stream.
		 *          The parser owns the state of the connection: the HPACK encoder/decoder, the map
		 *          of the streams, the windows of the flow control and the agreed SETTINGS.
		 *          The protocol is bidirectional: the parsing of the incoming frames generates the obligatory
		 *          outgoing ones (a SETTINGS ACK, a PING ACK, a WINDOW_UPDATE, a RST_STREAM, a GOAWAY),
		 *          therefore the parser has a channel of the writing - a callback function of the writing
		 *          (write_callback_t) or the pull model pending()/consumePending().
		 *          The direction of the traffic sets the role of the endpoint:
		 *          - direct_t::REQUEST  - we parse the requests of a client (we are a server);
		 *          - direct_t::RESPONSE - we parse the answers of a server (we are a client).
		 *          The built-in protections from a DoS: a limit of the frequency of the RST_STREAM (a Rapid Reset,
		 *          CVE-2023-44487), a limit of the frequency of the control frames (a SETTINGS/PING/empty
		 *          DATA flood), the limits of the assembly of a block of the headers (a CONTINUATION flood, 2024),
		 *          a limit of the unpacked list of the headers (a decompression bomb) and a limit
		 *          of the size of the body of a stream.
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Parser_HTTP2 : public parser_t {
			public:
				/**
				 * \~russian
				 * @brief Пополнение лимита частоты входящих RST_STREAM (токенов в секунду)
				 *
				 * @note Значения по умолчанию соответствуют nghttp2
				 *
				 * \~english
				 * @brief Replenishment of the limit of the frequency of the incoming RST_STREAM (tokens per second)
				 * @note The values by default correspond to nghttp2
				 *
				 * \~
				 */
				static constexpr uint64_t RST_LIMIT_RATE = (33);
				/**
				 * \~russian
				 * @brief Пополнение лимита частоты управляющих фреймов (токенов в секунду)
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
				 * @brief Стартовый запас лимита частоты входящих RST_STREAM (защита от Rapid Reset)
				 *
				 * @note Значения по умолчанию соответствуют nghttp2
				 *
				 * \~english
				 * @brief Starting reserve of the limit of the frequency of the incoming RST_STREAM (a protection from a Rapid Reset)
				 * @note The values by default correspond to nghttp2
				 *
				 * \~
				 */
				static constexpr uint64_t RST_LIMIT_BURST = (1000);
				/**
				 * \~russian
				 * @brief Стартовый запас лимита частоты управляющих фреймов (SETTINGS/PING/пустые DATA)
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 *
				 * \~english
				 * @brief Starting reserve of the limit of the frequency of the control frames (a SETTINGS/PING/empty DATA)
				 * @note The values by default are selected conservatively
				 *
				 * \~
				 */
				static constexpr uint64_t CTRL_LIMIT_BURST = (1000);
				/**
				 * \~russian
				 * @brief Пополнение лимита частоты кадров приоритета (токенов в секунду)
				 *
				 * @note Лимит отдельный от управляющих фреймов и заметно щедрее: клиент вправе
				 *       переставлять приоритеты на каждый загружаемый ресурс страницы
				 *
				 * \~english
				 * @brief Replenishment of the limit of the frequency of the frames of the priority (tokens per second)
				 * @note The limit is separate from the control frames and is noticeably more generous: a client is entitled
				 *       to rearrange the priorities per every loaded resource of a page
				 *
				 * \~
				 */
				static constexpr uint64_t PRIO_LIMIT_RATE = (500);
				/**
				 * \~russian
				 * @brief Стартовый запас лимита частоты кадров приоритета (PRIORITY/PRIORITY_UPDATE)
				 *
				 * @note Лимит отдельный от управляющих фреймов и заметно щедрее: клиент вправе
				 *       переставлять приоритеты на каждый загружаемый ресурс страницы
				 *
				 * \~english
				 * @brief Starting reserve of the limit of the frequency of the frames of the priority (PRIORITY/PRIORITY_UPDATE)
				 * @note The limit is separate from the control frames and is noticeably more generous: a client is entitled
				 *       to rearrange the priorities per every loaded resource of a page
				 *
				 * \~
				 */
				static constexpr uint64_t PRIO_LIMIT_BURST = (5000);
				/**
				 * \~russian
				 * @brief Порог сигнала о готовности потока принимать данные (low-water)
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 *
				 * \~english
				 * @brief Threshold of the signal about the readiness of a stream to accept the data (low-water)
				 * @note The values by default are selected conservatively
				 *
				 * \~
				 */
				static constexpr size_t SEND_LOW_WATER = (64 * 1024);
				/**
				 * \~russian
				 * @brief Ёмкость буфера отправки одного потока (high-water)
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 *
				 * \~english
				 * @brief Capacity of the buffer of the sending of a single stream (high-water)
				 * @note The values by default are selected conservatively
				 *
				 * \~
				 */
				static constexpr size_t SEND_HIGH_WATER = (256 * 1024);
				/**
				 * \~russian
				 * @brief Максимальное число фреймов в одном блоке заголовков
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 *
				 * \~english
				 * @brief Largest number of the frames in a single block of the headers
				 * @note The values by default are selected conservatively
				 *
				 * \~
				 */
				static constexpr uint32_t MAX_CONTINUATION_FRAMES = (64);
				/**
				 * \~russian
				 * @brief Порог выходного буфера соединения (backpressure от TCP-стадии)
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 *
				 * \~english
				 * @brief Threshold of the output buffer of the connection (a backpressure from the TCP stage)
				 * @note The values by default are selected conservatively
				 *
				 * \~
				 */
				static constexpr size_t OUTPUT_HIGH_WATER = (1024 * 1024);
				/**
				 * \~russian
				 * @brief Максимальный суммарный размер блока заголовков (HEADERS + все CONTINUATION)
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 *
				 * \~english
				 * @brief Largest total size of a block of the headers (a HEADERS + all the CONTINUATION)
				 * @note The values by default are selected conservatively
				 *
				 * \~
				 */
				static constexpr size_t MAX_HEADER_BLOCK_SIZE = (64 * 1024);
				/**
				 * \~russian
				 * @brief Число запоминаемых потоков, оборванных сбросом
				 *
				 * @details Кадры на оборванном потоке ещё летят к нам, пока пир не получил
				 *          наш RST_STREAM, и обязаны считаться потоковой ошибкой, а не
				 *          ошибкой соединения (RFC 9113 §5.1). Один идентификатор запомнить
				 *          мало: под нагрузкой сбросы идут подряд, и второй вытеснял бы
				 *          первый, пока кадры первого ещё в сети
				 *
				 * \~english
				 * @brief Number of the remembered streams broken by a reset
				 * @details The frames on a broken stream are still flying to us while the peer has not obtained
				 *          our RST_STREAM, and are obliged to be considered an error of a stream rather than
				 *          an error of the connection (RFC 9113 §5.1). To remember one identifier is
				 *          not enough: under a load the resets go in a row, and the second would evict
				 *          the first while the frames of the first are still in the network
				 *
				 * \~
				 */
				static constexpr size_t RESET_STREAMS_CACHE = (64);
				/**
				 * \~russian
				 * @brief Число запоминаемых приоритетов ещё не открытых потоков
				 *
				 * @details Кадр PRIORITY_UPDATE допустим для потока в состоянии idle
				 *          (RFC 9218 §7.1): сигнал приходит раньше HEADERS и обязан
				 *          примениться, когда поток откроется. Объектов потоков под
				 *          такие сигналы не создаём, а число самих сигналов ограничено:
				 *          иначе пир наполнял бы память приоритетами потоков,
				 *          которые открывать не собирается
				 *
				 * \~english
				 * @brief Number of the remembered priorities of the not yet opened streams
				 * @details A PRIORITY_UPDATE frame is admissible for a stream in the state idle
				 *          (RFC 9218 §7.1): the signal comes earlier than a HEADERS and is obliged
				 *          to be applied when the stream opens. We do not create the objects of the streams for
				 *          such signals, while the number of the signals themselves is limited:
				 *          otherwise a peer would fill the memory by the priorities of the streams
				 *          which it is not going to open
				 *
				 * \~
				 */
				static constexpr size_t PENDING_PRIORITIES_CACHE = (32);
			private:
				/**
				 * \~russian
				 * @brief Ёмкость набора отправляемых параметров SETTINGS
				 *
				 * @details Парсер анонсирует восемь параметров: размер таблицы HPACK,
				 *          разрешение push, начальное окно, размер кадра, лимит
				 *          одновременных потоков, лимит списка заголовков, разрешение
				 *          расширенного CONNECT и отказ от приоритетов RFC 7540.
				 *          Ёмкость взята с запасом: набор собирается инкрементом
				 *          счётчика без проверки границы на каждой записи, поэтому
				 *          девятый параметр, добавленный без правки этой константы,
				 *          вышел бы за массив молча
				 *
				 * \~english
				 * @brief Capacity of the collection of the sent parameters of SETTINGS
				 * @details The parser announces eight parameters: the size of the table of HPACK,
				 *          the permission of a push, the initial window, the size of a frame, the limit
				 *          of the simultaneous streams, the limit of the list of the headers, the permission
				 *          of an extended CONNECT and the refusal of the priorities of RFC 7540.
				 *          The capacity is taken with a reserve: the collection is assembled by an increment
				 *          of a counter without a check of the boundary at every record, therefore
				 *          a ninth parameter added without an editing of this constant
				 *          would go beyond the array silently
				 *
				 * \~
				 */
				static constexpr size_t SETTINGS_ENTRIES = (16);
			public:
				/**
				 * \~russian
				 * @brief Тип кода ошибки протокола HTTP/2 (RFC 9113 §7)
				 *
				 * \~english
				 * @brief Type of an error code of the HTTP/2 protocol (RFC 9113 §7)
				 *
				 * \~
				 */
				using error_t = h2::error_t;
			public:
				/**
				 * \~russian
				 * @brief Структура ограничений безопасности парсера HTTP/2
				 *
				 * @details Расширяет общее ядро лимитов базового парсера лимитами,
				 *          специфичными для HTTP/2. Общие лимиты применяются так:
				 *          - maxHeaderName/maxHeaderValue - длины декодированных заголовков;
				 *          - maxHeaderCount - число заголовков в одном блоке;
				 *          - maxHeadersTotal - суммарный размер распакованного списка (HPACK bomb);
				 *          - maxBodySize - суммарный размер тела одного потока
				 *
				 * \~english
				 * @brief Structure of the limitations of the safety of the parser of HTTP/2
				 * @details It extends the common core of the limits of the base parser by the limits
				 *          specific to HTTP/2. The common limits are applied thus:
				 *          - maxHeaderName/maxHeaderValue - the lengths of the decoded headers;
				 *          - maxHeaderCount - the number of the headers in a single block;
				 *          - maxHeadersTotal - the total size of the unpacked list (an HPACK bomb);
				 *          - maxBodySize - the total size of the body of a single stream
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Limits : parser_t::limits_t {
					// Пополнение лимита частоты входящих RST_STREAM (токенов в секунду)
					uint64_t rstLimitRate;
					// Стартовый запас лимита частоты входящих RST_STREAM
					uint64_t rstLimitBurst;
					// Пополнение лимита частоты управляющих фреймов (токенов в секунду)
					uint64_t ctrlLimitRate;
					// Стартовый запас лимита частоты управляющих фреймов
					uint64_t ctrlLimitBurst;
					// Пополнение лимита частоты кадров приоритета (токенов в секунду)
					uint64_t prioLimitRate;
					// Стартовый запас лимита частоты кадров приоритета
					uint64_t prioLimitBurst;
					// Максимальный суммарный размер блока заголовков (HEADERS + все CONTINUATION)
					size_t maxHeaderBlockSize;
					// Максимальное число фреймов в одном блоке заголовков
					uint32_t maxContinuationFrames;
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
				 * @brief Структура согласованных параметров SETTINGS (RFC 9113 §6.5.2)
				 *
				 * @details Значения по умолчанию из RFC 9113 (кроме maxConcurrentStreams -
				 *          безопасный дефолт вместо "без лимита").
				 *
				 * \~english
				 * @brief Structure of the agreed parameters of SETTINGS (RFC 9113 §6.5.2)
				 * @details The values by default are from RFC 9113 (except maxConcurrentStreams -
				 *          a safe default instead of «without a limit»).
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Settings {
					// Начальное окно потока
					int32_t windowSize;
					// Разрешён ли server push (0/1)
					uint32_t enablePush;
					// Максимальный размер фрейма
					uint32_t maxFrameSize;
					// Размер динамической таблицы HPACK
					uint32_t headerTableSize;
					// Лимит размера списка заголовков (0 - без лимита в SETTINGS, действует maxHeadersTotal)
					uint32_t maxHeaderListSize;
					// Лимит одновременных потоков
					uint32_t maxConcurrentStreams;
					// Разрешён ли расширенный CONNECT (0/1) - RFC 8441 §3
					uint32_t enableConnectProtocol;
					// Отказ от приоритетов RFC 7540 (0/1) - RFC 9218 §2.1
					uint32_t noRfc7540Priorities;
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
				 * @details Возврат false сбрасывает поток (RST_STREAM с кодом CANCEL).
				 *
				 * @param sid идентификатор потока
				 * @return    результат обработки (false - поток сбрасывается)
				 *
				 * \~english
				 * @brief Type of the callback function for the processing of the opening of a new stream
				 * @details A return of false resets the stream (a RST_STREAM with the code CANCEL).
				 * @param sid identifier of the stream
				 * @return    result of the processing (false - the stream is reset)
				 *
				 * \~
				 */
				using begin_callback_t = function <bool (const uint32_t)>;
				/**
				 * \~russian
				 * @brief Тип функции обратного вызова о готовности потока принимать данные тела
				 *
				 * @details Вызывается когда буфер отправки потока опустился ниже low-water
				 *          (после частичного приёма в sendData можно отправлять дальше).
				 *
				 * @param sid идентификатор потока
				 *
				 * \~english
				 * @brief Type of the callback function about the readiness of a stream to accept the data of the body
				 * @details It is called when the buffer of the sending of the stream has descended below the low-water
				 *          (after a partial acceptance in sendData it is possible to send onward).
				 * @param sid identifier of the stream
				 *
				 * \~
				 */
				using writable_callback_t = function <void (const uint32_t)>;
				/**
				 * \~russian
				 * @brief Тип функции обратного вызова записи исходящих байтов в сеть
				 *
				 * @details Если установлена - парсер сам отдаёт исходящие байты сетевому слою
				 *          сразу по мере формирования. Если не установлена - исходящие байты
				 *          накапливаются во внутреннем буфере (pull-модель: pending() +
				 *          consumePending()).
				 *
				 * @param buffer буфер исходящих данных
				 * @param size   размер исходящих данных
				 *
				 * \~english
				 * @brief Type of the callback function of the writing of the outgoing octets into the network
				 * @details If it is set - the parser itself issues the outgoing octets to the network layer
				 *          at once as they are formed. If it is not set - the outgoing octets
				 *          accumulate in an internal buffer (the pull model: pending() +
				 *          consumePending()).
				 * @param buffer buffer of the outgoing data
				 * @param size   size of the outgoing data
				 *
				 * \~
				 */
				using write_callback_t = function <void (const void *, const size_t)>;
				/**
				 * \~russian
				 * @brief Тип функции обратного вызова для обработки анонса server push (только клиент)
				 *
				 * @details Заголовки обещанного запроса придут через header_callback_t /
				 *          provider_callback_t с идентификатором обещанного потока.
				 *          Возврат false отклоняет push (RST_STREAM с кодом CANCEL).
				 *
				 * @param sid         идентификатор ассоциированного потока клиента
				 * @param promisedSid идентификатор обещанного потока
				 * @return            результат обработки (false - push отклоняется)
				 *
				 * \~english
				 * @brief Type of the callback function for the processing of an announcement of a server push (only a client)
				 * @details The headers of the promised request will come through header_callback_t /
				 *          provider_callback_t with the identifier of the promised stream.
				 *          A return of false rejects the push (a RST_STREAM with the code CANCEL).
				 * @param sid         identifier of the associated stream of the client
				 * @param promisedSid identifier of the promised stream
				 * @return            result of the processing (false - the push is rejected)
				 *
				 * \~
				 */
				using push_callback_t = function <bool (const uint32_t, const uint32_t)>;
				/**
				 * \~russian
				 * @brief Тип функции обратного вызова для обработки закрытия потока
				 *
				 * @param sid  идентификатор потока
				 * @param code код ошибки закрытия (NO_ERROR - штатное закрытие)
				 *
				 * \~english
				 * @brief Type of the callback function for the processing of the closing of a stream
				 * @param sid  identifier of the stream
				 * @param code error code of the closing (NO_ERROR - a regular closing)
				 *
				 * \~
				 */
				using close_callback_t = function <void (const uint32_t, const error_t)>;
				/**
				 * \~russian
				 * @brief Тип функции обратного вызова для обработки ошибки уровня соединения
				 *
				 * @details После этого события соединение необходимо закрыть
				 *          (GOAWAY уже поставлен в очередь отправки).
				 *
				 * @param code    код ошибки протокола
				 * @param message текстовое описание ошибки
				 *
				 * \~english
				 * @brief Type of the callback function for the processing of an error of the level of the connection
				 * @details After this event the connection is necessary to close
				 *          (a GOAWAY is already put into the queue of the sending).
				 * @param code    error code of the protocol
				 * @param message text description of the error
				 *
				 * \~
				 */
				using error_callback_t = function <void (const error_t, const string_view)>;
				/**
				 * \~russian
				 * @brief Тип функции обратного вызова для обработки фазы приёма сообщения потока
				 *
				 * @details Последовательность событий при приёме одного сообщения потока
				 *          (сигнатура и порядок универсальны с HTTP/1):
				 *          1. (BEGIN, NONE)    - получен первый блок заголовков потока
				 *          2. (END, HEADERS)   - блок заголовков доставлен (после провайдера)
				 *          3. (BEGIN, BODY)    - ожидается тело (END_STREAM не получен с заголовками)
				 *          4. (END, BODY)      - тело полностью принято (END_STREAM во фрейме DATA
				 *                                либо получен блок трейлеров)
				 *          5. (BEGIN, TRAILER) - получен блок трейлеров (только если пир их прислал)
				 *          6. (END, TRAILER)   - трейлеры доставлены (после провайдера с nullptr)
				 *          7. (END, NONE)      - сообщение потока полностью принято (END_STREAM применён)
				 *          Для обещанных запросов PUSH_PROMISE фазы не вызываются - фазы начнутся
				 *          с приходом ответа на обещанном потоке. Для информационных ответов
				 *          сервера (1xx) фазы также не вызываются: такой блок промежуточный и
				 *          доставляется только через header_callback_t / provider_callback_t,
				 *          а фазы начнутся с приходом финального блока заголовков.
				 *          Возврат false сбрасывает поток (RST_STREAM с кодом CANCEL).
				 *
				 * @param sid   идентификатор потока
				 * @param phase фаза приёма сообщения потока
				 * @param part  часть сообщения (заголовки, трейлеры, тело), NONE - сообщение целиком
				 * @return      результат обработки (false - поток сбрасывается)
				 *
				 * \~english
				 * @brief Type of the callback function for the processing of the phase of the acceptance of a message of a stream
				 * @details The sequence of the events at the acceptance of a single message of a stream
				 *          (the signature and the order are universal with HTTP/1):
				 *          1. (BEGIN, NONE)    - the first block of the headers of the stream is obtained
				 *          2. (END, HEADERS)   - the block of the headers is delivered (after the provider)
				 *          3. (BEGIN, BODY)    - a body is expected (an END_STREAM is not obtained with the headers)
				 *          4. (END, BODY)      - the body is fully accepted (an END_STREAM in a DATA frame
				 *                                or a block of the trailers is obtained)
				 *          5. (BEGIN, TRAILER) - a block of the trailers is obtained (only if the peer has sent them)
				 *          6. (END, TRAILER)   - the trailers are delivered (after the provider with a nullptr)
				 *          7. (END, NONE)      - the message of the stream is fully accepted (the END_STREAM is applied)
				 *          For the promised requests of a PUSH_PROMISE the phases are not called - the phases will begin
				 *          with the arrival of the answer on the promised stream. For the informational answers
				 *          of a server (1xx) the phases are likewise not called: such a block is an intermediate one and
				 *          is delivered only through header_callback_t / provider_callback_t,
				 *          while the phases will begin with the arrival of the final block of the headers.
				 *          A return of false resets the stream (a RST_STREAM with the code CANCEL).
				 * @param sid   identifier of the stream
				 * @param phase phase of the acceptance of the message of the stream
				 * @param part  part of the message (the headers, the trailers, the body), NONE - the message as a whole
				 * @return      result of the processing (false - the stream is reset)
				 *
				 * \~
				 */
				using phase_callback_t = function <bool (const uint32_t, const phase_t, const part_t)>;
				/**
				 * \~russian
				 * @brief Тип функции обратного вызова для обработки провайдера заголовков потока
				 *
				 * @details Вызывается по завершению блока заголовков (получен END_HEADERS).
				 *          Провайдер собран из псевдо-заголовков блока: для направления REQUEST
				 *          это request_t (:method/:path), для RESPONSE - response_t (:status).
				 *          Для трейлеров провайдер передаётся как nullptr.
				 *          Возврат false сбрасывает поток (RST_STREAM с кодом CANCEL).
				 *
				 * @param sid       идентификатор потока
				 * @param provider  провайдер заголовков потока (nullptr для трейлеров)
				 * @param endStream флаг завершения потока (тела не будет)
				 * @return          результат обработки (false - поток сбрасывается)
				 *
				 * \~english
				 * @brief Type of the callback function for the processing of the provider of the headers of a stream
				 * @details It is called at the completion of a block of the headers (an END_HEADERS is obtained).
				 *          The provider is assembled from the pseudo headers of the block: for the direction REQUEST
				 *          this is a request_t (:method/:path), for a RESPONSE - a response_t (:status).
				 *          For the trailers the provider is transmitted as a nullptr.
				 *          A return of false resets the stream (a RST_STREAM with the code CANCEL).
				 * @param sid       identifier of the stream
				 * @param provider  provider of the headers of the stream (a nullptr for the trailers)
				 * @param endStream flag of the completion of the stream (there will be no body)
				 * @return          result of the processing (false - the stream is reset)
				 *
				 * \~
				 */
				using provider_callback_t = function <bool (const uint32_t, const provider_t *, const bool)>;
				/**
				 * \~russian
				 * @brief Тип функции обратного вызова для обработки полученного GOAWAY
				 *
				 * @details Указатель debug действителен ТОЛЬКО на время вызова
				 *
				 * @param sid   наибольший идентификатор обработанного пиром потока
				 * @param code  код ошибки завершения соединения
				 * @param debug необязательные отладочные данные пира
				 *
				 * \~english
				 * @brief Type of the callback function for the processing of an obtained GOAWAY
				 * @details The pointer debug is valid ONLY for the time of the call
				 * @param sid   largest identifier of a stream processed by the peer
				 * @param code  error code of the completion of the connection
				 * @param debug optional debug data of the peer
				 *
				 * \~
				 */
				using goaway_callback_t = function <void (const uint32_t, const error_t, const string_view)>;
				/**
				 * \~russian
				 * @brief Тип функции обратного вызова для обработки полученного ALTSVC (RFC 7838 §4)
				 *
				 * @details Представления origin и value действительны ТОЛЬКО на время вызова.
				 *          Нулевой идентификатор потока означает анонс для соединения,
				 *          и тогда origin непустой; для потока origin приходит пустым,
				 *          а сам origin определяется этим потоком
				 *
				 * @param sid    идентификатор потока, либо 0 для соединения
				 * @param origin origin анонсируемого сервиса
				 * @param value  значение поля Alt-Svc (RFC 7838 §3)
				 *
				 * \~english
				 * @brief Type of the callback function for the processing of an obtained ALTSVC (RFC 7838 §4)
				 * @details The representations origin and value are valid ONLY for the time of the call.
				 *          A zero identifier of the stream means an announcement for the connection,
				 *          and then the origin is non-empty; for a stream the origin comes empty,
				 *          while the origin itself is determined by that stream
				 * @param sid    identifier of the stream, or 0 for the connection
				 * @param origin origin of the service being announced
				 * @param value  value of the field Alt-Svc (RFC 7838 §3)
				 *
				 * \~
				 */
				using altsvc_callback_t = function <void (const uint32_t, const string_view, const string_view)>;
				/**
				 * \~russian
				 * @brief Тип функции обратного вызова для обработки полученного ORIGIN (RFC 8336 §2)
				 *
				 * @details Вызывается по одному разу на каждый origin набора. Представление
				 *          действительно ТОЛЬКО на время вызова
				 *
				 * @param origin очередной origin, обслуживаемый соединением
				 *
				 * \~english
				 * @brief Type of the callback function for the processing of an obtained ORIGIN (RFC 8336 §2)
				 * @details It is called once for every origin of the collection. The representation
				 *          is valid ONLY for the time of the call
				 * @param origin next origin served by the connection
				 *
				 * \~
				 */
				using origin_callback_t = function <void (const string_view)>;
				/**
				 * \~russian
				 * @brief Тип функции обратного вызова для обработки фрагмента тела потока
				 *
				 * @details Указатель buffer действителен ТОЛЬКО на время вызова (zero-copy).
				 *          Возврат false сбрасывает поток (RST_STREAM с кодом CANCEL).
				 *
				 * @param sid       идентификатор потока
				 * @param buffer    буфер данных тела
				 * @param size      размер данных тела
				 * @param endStream флаг завершения потока
				 * @return          результат обработки (false - поток сбрасывается)
				 *
				 * \~english
				 * @brief Type of the callback function for the processing of a fragment of the body of a stream
				 * @details The pointer buffer is valid ONLY for the time of the call (zero-copy).
				 *          A return of false resets the stream (a RST_STREAM with the code CANCEL).
				 * @param sid       identifier of the stream
				 * @param buffer    buffer of the data of the body
				 * @param size      size of the data of the body
				 * @param endStream flag of the completion of the stream
				 * @return          result of the processing (false - the stream is reset)
				 *
				 * \~
				 */
				using data_callback_t = function <bool (const uint32_t, const void *, const size_t, const bool)>;
				/**
				 * \~russian
				 * @brief Тип pull-источника данных тела потока (для больших тел без лишней копии)
				 *
				 * @details Альтернатива sendData: парсер сам запрашивает у источника данные
				 *          ровно тогда, когда открыто окно и есть место в выходном буфере.
				 *          Источник заполняет буфер (не более cap байт), выставляет eof = true
				 *          по достижении конца тела и возвращает число записанных байт,
				 *          либо -1 при ошибке (поток будет сброшен).
				 *
				 * @param sid    идентификатор потока
				 * @param buffer буфер для заполнения
				 * @param cap    ёмкость буфера
				 * @param eof    флаг достижения конца тела
				 * @return       число записанных байт либо -1 при ошибке
				 *
				 * \~english
				 * @brief Type of the pull source of the data of the body of a stream (for the big bodies without a superfluous copy)
				 * @details An alternative to sendData: the parser itself requests the data from the source
				 *          exactly then when the window is open and there is a place in the output buffer.
				 *          The source fills the buffer (not more than cap octets), sets eof = true
				 *          at the reaching of the end of the body and returns the number of the written octets,
				 *          or -1 at an error (the stream will be reset).
				 * @param sid    identifier of the stream
				 * @param buffer buffer for the filling
				 * @param cap    capacity of the buffer
				 * @param eof    flag of the reaching of the end of the body
				 * @return       number of the written octets or -1 at an error
				 *
				 * \~
				 */
				using data_source_callback_t = function <int64_t (const uint32_t, uint8_t *, const size_t, bool &)>;
				/**
				 * \~russian
				 * @brief Тип функции обратного вызова для обработки заголовков или трейлеров потока
				 *
				 * @details Указатели name/value действительны ТОЛЬКО на время вызова.
				 *          Передаются все заголовки, включая псевдо-заголовки (:method/:path/
				 *          :status и др.); наиболее употребимые псевдо-заголовки дополнительно
				 *          собираются в провайдер заголовков потока.
				 *          Возврат false сбрасывает поток (RST_STREAM с кодом CANCEL).
				 *
				 * @param sid   идентификатор потока
				 * @param name  название заголовка
				 * @param value значение заголовка
				 * @param part  часть сообщения (HEADERS или TRAILER)
				 * @return      результат обработки (false - поток сбрасывается)
				 *
				 * \~english
				 * @brief Type of the callback function for the processing of the headers or of the trailers of a stream
				 * @details The pointers name/value are valid ONLY for the time of the call.
				 *          All the headers are transmitted, including the pseudo headers (:method/:path/
				 *          :status and others); the most usable pseudo headers are additionally
				 *          assembled into the provider of the headers of the stream.
				 *          A return of false resets the stream (a RST_STREAM with the code CANCEL).
				 * @param sid   identifier of the stream
				 * @param name  name of the header
				 * @param value value of the header
				 * @param part  part of the message (HEADERS or TRAILER)
				 * @return      result of the processing (false - the stream is reset)
				 *
				 * \~
				 */
				using header_callback_t = function <bool (const uint32_t, const string_view, const string_view, const part_t)>;
			private:
				/**
				 * \~russian
				 * @brief Класс token-bucket для ограничения частоты событий (защита от flood)
				 *
				 * @details Повторяет модель nghttp2_ratelim: целочисленные токены, пополнение
				 *          rate токенов в секунду до предела burst. Время задаётся извне через
				 *          updateTime(); без обновления времени работает только стартовый запас
				 *          burst (этого достаточно, чтобы погасить мгновенный всплеск).
				 *
				 * \~english
				 * @brief Class of a token-bucket for the limitation of the frequency of the events (a protection from a flood)
				 * @details It repeats the model nghttp2_ratelim: the integer tokens, a replenishment of
				 *          rate tokens per second up to the limit burst. The time is set from the outside through
				 *          updateTime(); without an updating of the time only the starting reserve
				 *          burst works (this suffices to quench an instantaneous burst).
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
						 * @return      результат списания (false - токенов не хватает, превышение лимита)
						 *
						 * \~english
						 * @brief Method of the writing off of the tokens
						 * @param value number of the tokens being written off
						 * @return      result of the writing off (false - the tokens do not suffice, an exceeding of the limit)
						 *
						 * \~
						 */
						bool drain(const uint64_t value) noexcept;
						/**
						 * \~russian
						 * @brief Метод пополнения токенов по текущему времени
						 *
						 * @param stamp текущее время (секунды)
						 *
						 * \~english
						 * @brief Method of the replenishment of the tokens by the current time
						 * @param stamp current time (seconds)
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
				 * @brief Класс состояния одного потока (RFC 9113 §5.1)
				 *
				 * \~english
				 * @brief Class of the state of a single stream (RFC 9113 §5.1)
				 *
				 * \~
				 */
				typedef class __AWH_SHARED_EXPORT__ Stream {
					public:
						// Идентификатор потока
						uint32_t id;
						// Достигнут конец тела pull-источника данных
						bool sourceEof;
						// Получен END_HEADERS (повторный HEADERS = трейлеры)
						bool headersDone;
						// END_STREAM уже отправлен
						bool endStreamSent;
						// На последнем фрагменте выставить END_STREAM
						bool endStreamPending;
						// Сигнал writable уже подан для текущего провала буфера
						bool writableNotified;
						/**
						 * \~russian
						 * Поток стоит в очереди готовых к отправке
						 *
						 * @details Признак хранится у потока, а не ищется в очереди:
						 *          иначе добавление стоило бы линейного поиска по ней
						 *
						 * \~english
						 * The stream stands in the queue of those ready for the sending
						 * @details The flag is stored at the stream rather than sought in the queue:
						 *          otherwise an addition would cost a linear search over it
						 *
						 * \~
						 */
						bool queued;
						// Суммарный размер принятого тела потока (лимит maxBodySize)
						uint64_t recvBody;
						// Объявленная заголовком content-length длина тела (-1 - не объявлена)
						int64_t contentLength;
						/**
						 * Принимаемое сообщение не может нести тело, даже если объявляет
						 * content-length: ответ на HEAD, а также ответы 204 и 304 (RFC 9110 §8.6)
						 */
						bool bodyless;
						/**
						 * \~russian
						 * Отправляемое сообщение не может нести тело: ответ на принятый
						 * запрос методом HEAD (RFC 9110 §9.3.2)
						 *
						 * @details Признак отдельный от bodyless: тело самого запроса HEAD
						 *          запрещено лишь через SHOULD NOT, поэтому принимать его
						 *          парсер обязан, а вот отдавать тело в ответ на него - нет
						 *
						 * \~english
						 * A message being sent cannot carry a body: an answer to an accepted
						 * request by the method HEAD (RFC 9110 §9.3.2)
						 * @details The flag is separate from bodyless: the body of a HEAD request itself
						 *          is prohibited only through a SHOULD NOT, therefore the parser is obliged
						 *          to accept it, but to issue a body in an answer to it - no
						 *
						 * \~
						 */
						bool bodylessSend;
						/**
						 * \~russian
						 * Принимаемое сообщение не может нести секцию трейлеров:
						 * ответы 204 и 304 (RFC 9110 §15.3.5, §15.4.5)
						 *
						 * @details Признак отдельный от bodyless: у ответа на HEAD
						 *          запрещено только содержимое, а трейлеры §9.3.2
						 *          не запрещает, и кадрирование их допускает
						 *
						 * \~english
						 * A message being accepted cannot carry a section of the trailers:
						 * the answers 204 and 304 (RFC 9110 §15.3.5, §15.4.5)
						 * @details The flag is separate from bodyless: at an answer to a HEAD
						 *          only the content is prohibited, while the trailers §9.3.2
						 *          does not prohibit, and the framing admits them
						 *
						 * \~
						 */
						bool trailerless;
						/**
						 * \~russian
						 * Отправляемое сообщение не может нести секцию трейлеров:
						 * ответы 204 и 304 (RFC 9110 §15.3.5, §15.4.5)
						 *
						 * @details Признак отдельный от bodylessSend: у ответа на HEAD
						 *          запрещено только содержимое, а трейлеры §9.3.2 не
						 *          запрещает вовсе, и кадрирование их допускает
						 *
						 * \~english
						 * A message being sent cannot carry a section of the trailers:
						 * the answers 204 and 304 (RFC 9110 §15.3.5, §15.4.5)
						 * @details The flag is separate from bodylessSend: at an answer to a HEAD
						 *          only the content is prohibited, while the trailers §9.3.2 does not
						 *          prohibit at all, and the framing admits them
						 *
						 * \~
						 */
						bool trailerlessSend;
						// Срочность потока (0 - наивысшая, 7 - наименьшая) - RFC 9218 §4.1
						uint8_t urgency;
						// Признак инкрементальной доставки потока - RFC 9218 §4.2
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
						/**
						 * \~russian
						 * Приоритет потока объявлен приложением и ещё не отправлен
						 *
						 * @details Сигнал приоритета для собственного потока передаётся
						 *          заголовком [priority] в секции заголовков (RFC 9218 §5):
						 *          серверу это единственный доступный способ - кадр
						 *          PRIORITY_UPDATE ему запрещён. Признак снимается отправкой
						 *          секции, потому что объявлять приоритет повторно в трейлерах
						 *          бессмысленно
						 *
						 * \~english
						 * The priority of the stream is announced by the application and is not yet sent
						 * @details The signal of the priority for one's own stream is transmitted
						 *          by the header [priority] in the section of the headers (RFC 9218 §5):
						 *          for a server this is the only accessible way - a
						 *          PRIORITY_UPDATE frame is prohibited to it. The flag is removed by the sending
						 *          of the section, because to announce the priority repeatedly in the trailers
						 *          is senseless
						 *
						 * \~
						 */
						bool announce;
						// Окно приёма потока (сколько ещё можем принять)
						int32_t localWindow;
						// Окно отправки потока (сколько ещё можем отправить)
						int32_t remoteWindow;
						// Префикс sendBuffer, уже отправленный (вместо erase(0,..))
						size_t sendOffset;
						// Ещё не нарезанные в DATA байты тела (ограничен high-water)
						string sendBuffer;
						// Блок заголовков потока уже отправлен нами
						bool headersSent;
						// На завершение тела отложена секция трейлеров
						bool trailersPending;
						/**
						 * Отложенная секция трейлеров: хранится полями, а не закодированным
						 * блоком. HPACK-блоки обязаны кодироваться в том же порядке, в каком
						 * уходят в сеть, поэтому отложить можно только сами заголовки
						 */
						vector <h2::hpack::field_t> trailers;
						// Состояние потока
						h2::stream_state_t state;
						// Pull-источник данных тела (если задан вместо sendData)
						data_source_callback_t source;
						// Провайдер заголовков потока (собирается из псевдо-заголовков)
						unique_ptr <http::provider_t> headers;
					public:
						/**
						 * \~russian
						 * @brief Метод получения логического объёма ещё не отправленных данных тела
						 *
						 * @return объём не отправленных данных (без учтённого префикса)
						 *
						 * \~english
						 * @brief Method of getting the logical volume of the not yet sent data of the body
						 * @return volume of the not sent data (without the accounted prefix)
						 *
						 * \~
						 */
						size_t pending() const noexcept;
						/**
						 * \~russian
						 * @brief Метод снятия учтённого префикса буфера отправки
						 *
						 * @details Очистка при полном расходе, иначе амортизированная компактификация
						 *
						 * \~english
						 * @brief Method of the removal of the accounted prefix of the buffer of the sending
						 * @details A clearing at a full expenditure, otherwise an amortized compaction
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
			private:
				/**
				 * \~russian
				 * @brief Структура rate-лимитов
				 *
				 * \~english
				 * @brief Structure of the rate limits
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Ratelims {
					// Текущее время в секундах (для rate-лимитов)
					uint64_t now;
					// Лимит частоты входящих RST_STREAM (против Rapid Reset)
					ratelim_t rst;
					// Лимит частоты управляющих фреймов (против flood SETTINGS/PING/пустых DATA)
					ratelim_t ctrl;
					// Лимит частоты кадров приоритета (против flood PRIORITY/PRIORITY_UPDATE)
					ratelim_t prio;
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
					explicit Ratelims() noexcept;
				} ratelims_t;
				/**
				 * \~russian
				 * @brief Структура окон flow control соединения
				 *
				 * \~english
				 * @brief Structure of the windows of the flow control of the connection
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Window {
					// Окно приёма соединения (текущее)
					int32_t local;
					// Окно отправки соединения
					int32_t remote;
					// Целевой размер окна приёма соединения
					int32_t localMax;
					// Анонсированное пиру начальное окно приёма потока (SETTINGS_INITIAL_WINDOW_SIZE)
					int32_t localInit;
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
					explicit Window() noexcept;
				} window_t;
				/**
				 * \~russian
				 * @brief Структура буферов соединения
				 *
				 * \~english
				 * @brief Structure of the buffers of the connection
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Buffer {
					/**
					 * \~russian
					 * Незавершённый фрейм, не уместившийся в поданную порцию
					 *
					 * @details Фрейм HTTP/2 разбирается только целиком, а границы порций
					 *          чтения из сокета с границами фреймов не совпадают. Здесь
					 *          копится ровно недостающая часть одного фрейма и ничего
					 *          сверх: целые фреймы разбираются прямо по буферу вызывающей
					 *          стороны и в этот буфер не попадают. Поэтому объём копирования
					 *          на порцию ограничен размером одного фрейма, а не её размером
					 *
					 * \~english
					 * An uncompleted frame which has not fitted into the supplied portion
					 * @details A frame of HTTP/2 is parsed only entirely, while the boundaries of the portions
					 *          of the reading from the socket do not coincide with the boundaries of the frames. Here
					 *          exactly the lacking part of a single frame accumulates and nothing
					 *          above: the whole frames are parsed right by the buffer of the calling
					 *          side and do not get into this buffer. Therefore the volume of the copying
					 *          per portion is limited by the size of a single frame rather than by its size
					 *
					 * \~
					 */
					string input;
					/**
					 * Байты, поданные реентрантным вызовом parse() из пользовательской
					 * функции: дописывать их сразу в буфер незавершённого фрейма нельзя -
					 * перевыделение памяти обесценит zero-copy указатели, уже отданные наружу
					 */
					string deferred;
					// Буфер исходящих байтов
					string output;
					// Префикс буфера исходящих байтов, уже отданный в сокет (вместо erase(0,..))
					size_t outputPos;
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
					explicit Buffer() noexcept;
				} buffer_t;
				/**
				 * \~russian
				 * @brief Структура собираемого блока заголовков (HEADERS + CONTINUATION)
				 *
				 * \~english
				 * @brief Structure of the block of the headers being assembled (a HEADERS + a CONTINUATION)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Header_Block {
					// Идентификатор потока собираемого блока заголовков (0 - сборка не идёт)
					uint32_t stream;
					// Число фреймов в текущем блоке заголовков
					uint32_t frames;
					// Идентификатор обещанного потока (!= 0: блок принадлежит PUSH_PROMISE)
					uint32_t promised;
					// Накопленный фрагмент блока заголовков (HEADERS + CONTINUATION)
					string buffer;
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
					explicit Header_Block() noexcept;
				} header_block_t;
				/**
				 * \~russian
				 * @brief Структура флагов состояния соединения
				 *
				 * \~english
				 * @brief Structure of the flags of the state of the connection
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Flags {
					// Защита от реентерабельного pump() (writable -> sendData -> pump)
					bool inPump;
					// Защита от реентерабельного parse() (callback -> parse)
					bool inParse;
					// Отправлен GOAWAY
					bool goawaySent;
					// Отправлен предупреждающий GOAWAY плавного завершения (RFC 9113 §6.8)
					bool goawayGraceful;
					// Поток отклонён (RST_STREAM), блок декодируем только для синхронизации HPACK
					bool hbcRefused;
					// Флаг END_STREAM собираемого блока заголовков
					bool hbcEndStream;
					// Получен ACK на наш SETTINGS
					bool settingsAcked;
					// Получен первый SETTINGS пира (connection preface, RFC 9113 §3.4)
					bool settingsReceived;
					// Параметр отказа от приоритетов RFC 7540 уже зафиксирован пиром (RFC 9218 §2.1)
					bool prioritiesLocked;
					// Получен GOAWAY
					bool goawayReceived;
					// Получен connection preface (для сервера; клиент отправляет его сам)
					bool prefaceReceived;
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
					explicit Flags() noexcept;
				} flags_t;
				/**
				 * \~russian
				 * @brief Структура записи о потоке, оборванном сбросом
				 *
				 * @details Различать, чей это был сброс, обязательно: после нашего RST_STREAM
				 *          кадры пира обязаны молча игнорироваться, а после его сброса -
				 *          отвергаться потоковой ошибкой STREAM_CLOSED (RFC 9113 §5.1)
				 *
				 * \~english
				 * @brief Structure of a record about a stream broken by a reset
				 * @details To distinguish whose reset it was is obligatory: after our RST_STREAM
				 *          the frames of the peer are obliged to be silently ignored, while after its reset -
				 *          to be rejected by the error of a stream STREAM_CLOSED (RFC 9113 §5.1)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Reset {
					// Идентификатор потока (0 - ячейка кольца пуста)
					uint32_t id;
					// Сброс отправлен нами (иначе - принят от пира)
					bool local;
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
					explicit Reset() noexcept : id(0), local(false) {}
				} reset_t;
				/**
				 * \~russian
				 * @brief Структура параметров передачи данных
				 *
				 * \~english
				 * @brief Structure of the parameters of the transmission of the data
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Transfer {
					// Наибольший принятый идентификатор потока (для GOAWAY)
					uint32_t lastStreamId;
					/**
					 * \~russian
					 * Кольцо потоков, оборванных сбросом
					 *
					 * @details Кадры на таком потоке ещё могут быть в полёте: их отправили
					 *          до того, как сброс дошёл до отправителя. Кольцо, а не один
					 *          идентификатор: сбросы идут подряд, и хранить надо все недавние
					 *
					 * \~english
					 * Ring of the streams broken by a reset
					 * @details The frames on such a stream may still be in a flight: they have been sent
					 *          before the reset has reached the sender. A ring rather than a single
					 *          identifier: the resets go in a row, and all the recent ones should be stored
					 *
					 * \~
					 */
					vector <reset_t> resetStreams;
					// Позиция записи в кольце оборванных сбросом потоков
					size_t resetCursor;
					/**
					 * \~russian
					 * Наибольший идентификатор, попадавший в кольцо оборванных сбросом потоков
					 *
					 * @details Служит отсечкой перебора: кольцо просматривается на каждом кадре
					 *          потока, которого нет в карте, а таким оказывается и всякий вновь
					 *          открываемый поток - то есть путь этот не редкий, как можно
					 *          подумать, а самый обычный. Идентификатор нового потока заведомо
					 *          старше всех попадавших в кольцо, и перебор шестидесяти четырёх
					 *          ячеек для него бессмыслен. Отсечка точна: значение только растёт,
					 *          поэтому идентификатор больше него в кольце отсутствует наверняка
					 *
					 * \~english
					 * Largest identifier which has got into the ring of the streams broken by a reset
					 * @details It serves as a cutoff of the enumeration: the ring is looked through at every frame
					 *          of a stream which is not in the map, and such turns out to be every newly
					 *          opened stream as well - that is this path is not a rare one, as one might
					 *          think, but the most ordinary one. The identifier of a new stream is knowingly
					 *          older than all those which have got into the ring, and an enumeration of the sixty-four
					 *          cells for it is senseless. The cutoff is exact: the value only grows,
					 *          therefore an identifier bigger than it is certainly absent from the ring
					 *
					 * \~
					 */
					uint32_t resetMaxId;
					// Наибольший наш идентификатор потока, по которому уже отправлен блок заголовков
					uint32_t localOpened;
					// Следующий инициируемый нами идентификатор потока
					uint32_t nextStreamId;
					// Число активных потоков, открытых пиром (лимит MAX_CONCURRENT_STREAMS)
					uint32_t peerStreamCount;
					// Число активных потоков, открытых нами (лимит MAX_CONCURRENT_STREAMS пира)
					uint32_t localStreamCount;
					// Число отправленных нами SETTINGS, подтверждения которых ещё не получены (RFC 9113 §6.5.3)
					uint32_t settingsAckPending;
					// Порог сигнала writable (low-water)
					size_t sendLowWater;
					// Ёмкость буфера отправки потока (high-water)
					size_t sendHighWater;
					// Порог выходного буфера соединения (backpressure от TCP-стадии)
					size_t outputHighWater;
					/**
					 * \~russian
					 * @brief Структура ячейки снимка планировщика отправки
					 *
					 * \~english
					 * @brief Structure of a cell of the snapshot of the scheduler of the sending
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Slot {
						// Срочность потока (RFC 9218 §4.1)
						uint8_t urgency;
						// Признак инкрементальной доставки потока
						bool incremental;
						// Идентификатор потока
						uint32_t id;
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
						explicit Slot() noexcept : urgency(0), incremental(false), id(0) {}
					} slot_t;
					/**
					 * \~russian
					 * Переиспользуемый снимок планировщика отправки (без аллокаций на вызов)
					 *
					 * @details Признаки приоритета снимаются вместе с идентификатором, а не
					 *          читаются из карты потоков в компараторе: иначе каждое сравнение
					 *          стоило бы двух хеш-поисков, и упорядочивание снимка обходилось
					 *          дороже самой отправки. Снимок берётся до любых выходов наружу,
					 *          поэтому снятые признаки заведомо те же, что в карте
					 *
					 * \~english
					 * Reused snapshot of the scheduler of the sending (without the allocations per call)
					 * @details The flags of the priority are taken together with the identifier rather than
					 *          read from the map of the streams in the comparator: otherwise every comparison
					 *          would cost two hash searches, and the ordering of the snapshot would come
					 *          dearer than the sending itself. The snapshot is taken before any exits outside,
					 *          therefore the taken flags are knowingly the same as those in the map
					 *
					 * \~
					 */
					vector <slot_t> pumpIds;
					/**
					 * \~russian
					 * Очередь потоков, которым есть что отправлять
					 *
					 * @details Снимок планировщика собирается из неё, а не обходом карты
					 *          потоков: тело отдают единицы из сотен открытых, и полный
					 *          обход стоил бы O(N) на каждый выпущенный DATA-фрейм.
					 *          Запись добавляется там, где поток обзаводится содержимым,
					 *          и снимается лениво - на ближайшей прокачке, когда окажется,
					 *          что отправлять уже нечего. Ложная запись стоит одной проверки,
					 *          поэтому снятие вправе запаздывать, а добавление - нет
					 *
					 * \~english
					 * Queue of the streams which have something to send
					 * @details The snapshot of the scheduler is assembled out of it rather than by a traversal of the map
					 *          of the streams: the body is issued by units out of the hundreds of the open ones, and a full
					 *          traversal would cost O(N) per every issued DATA frame.
					 *          A record is added there where a stream acquires a content,
					 *          and is removed lazily - at the nearest pumping, when it turns out
					 *          that there is nothing to send any more. A false record costs one check,
					 *          therefore the removal is entitled to be late, while the addition - not
					 *
					 * \~
					 */
					vector <uint32_t> readyIds;
					/**
					 * \~russian
					 * @brief Структура отложенного приоритета ещё не открытого потока
					 *
					 * \~english
					 * @brief Structure of a postponed priority of a not yet opened stream
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Pending {
						// Срочность потока (RFC 9218 §4.1)
						uint8_t urgency;
						// Признак инкрементальной доставки потока
						bool incremental;
						// Идентификатор приоритизируемого потока
						uint32_t id;
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
						explicit Pending() noexcept : urgency(0), incremental(false), id(0) {}
					} pending_t;
					/**
					 * \~russian
					 * Кольцо приоритетов, объявленных до открытия потока (RFC 9218 §7.1)
					 *
					 * @details Кадр PRIORITY_UPDATE вправе опередить HEADERS, и сигнал по
					 *          потоку в состоянии idle обязан примениться при его открытии.
					 *          Объект потока под сигнал не создаётся: до прихода HEADERS это
					 *          позволило бы пиру наполнить карту потоков даром. Кольцо
					 *          ограничивает память сверху, вытесняя самую старую запись
					 *
					 * \~english
					 * Ring of the priorities announced before the opening of a stream (RFC 9218 §7.1)
					 * @details A PRIORITY_UPDATE frame is entitled to outstrip a HEADERS, and a signal by
					 *          a stream in the state idle is obliged to be applied at its opening.
					 *          An object of a stream for a signal is not created: before the arrival of a HEADERS this
					 *          would allow a peer to fill the map of the streams for nothing. The ring
					 *          limits the memory from above, evicting the very oldest record
					 *
					 * \~
					 */
					vector <pending_t> pendingPriorities;
					/**
					 * Переиспользуемый снимок идентификаторов закрываемых потоков (обработка GOAWAY).
					 * Отдельный от pumpIds буфер обязателен: закрытие потока вызывает пользовательскую
					 * функцию обратного вызова, а та вправе отправить данные и реентрантно запустить
					 * pump() - тот перезаполнит свой снимок, и перебор по общему буферу разъедется
					 */
					vector <uint32_t> closeIds;
					// Карта активных потоков
					unordered_map <uint32_t, stream_t> streams;
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
					explicit Transfer() noexcept;
				} transfer_t;
			private:
				/**
				 * \~russian
				 * @brief Структура функций обратного вызова
				 *
				 * \~english
				 * @brief Structure of the callback functions
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Callbacks {
					/**
					 * \~russian
					 * @brief Функция обратного вызова для обработки фрагмента тела потока
					 *
					 * \~english
					 * @brief Callback function for the processing of a fragment of the body of a stream
					 *
					 * \~
					 */
					data_callback_t data;
					/**
					 * \~russian
					 * @brief Функция обратного вызова для обработки анонса server push
					 *
					 * \~english
					 * @brief Callback function for the processing of an announcement of a server push
					 *
					 * \~
					 */
					push_callback_t push;
					/**
					 * \~russian
					 * @brief Функция обратного вызова записи исходящих байтов в сеть
					 *
					 * \~english
					 * @brief Callback function of the writing of the outgoing octets into the network
					 *
					 * \~
					 */
					write_callback_t write;
					/**
					 * \~russian
					 * @brief Функция обратного вызова для обработки открытия нового потока
					 *
					 * \~english
					 * @brief Callback function for the processing of the opening of a new stream
					 *
					 * \~
					 */
					begin_callback_t begin;
					/**
					 * \~russian
					 * @brief Функция обратного вызова для обработки закрытия потока
					 *
					 * \~english
					 * @brief Callback function for the processing of the closing of a stream
					 *
					 * \~
					 */
					close_callback_t close;
					/**
					 * \~russian
					 * @brief Функция обратного вызова для обработки ошибки уровня соединения
					 *
					 * \~english
					 * @brief Callback function for the processing of an error of the level of the connection
					 *
					 * \~
					 */
					error_callback_t error;
					/**
					 * \~russian
					 * @brief Функция обратного вызова для обработки фазы приёма сообщения потока
					 *
					 * \~english
					 * @brief Callback function for the processing of the phase of the acceptance of a message of a stream
					 *
					 * \~
					 */
					phase_callback_t phase;
					/**
					 * \~russian
					 * @brief Функция обратного вызова для обработки заголовков или трейлеров потока
					 *
					 * \~english
					 * @brief Callback function for the processing of the headers or of the trailers of a stream
					 *
					 * \~
					 */
					header_callback_t header;
					/**
					 * \~russian
					 * @brief Функция обратного вызова для обработки полученного GOAWAY
					 *
					 * \~english
					 * @brief Callback function for the processing of an obtained GOAWAY
					 *
					 * \~
					 */
					goaway_callback_t goaway;
					/**
					 * \~russian
					 * @brief Функция обратного вызова для обработки полученного ALTSVC
					 *
					 * \~english
					 * @brief Callback function for the processing of an obtained ALTSVC
					 *
					 * \~
					 */
					altsvc_callback_t altsvc;
					/**
					 * \~russian
					 * @brief Функция обратного вызова для обработки полученного ORIGIN
					 *
					 * \~english
					 * @brief Callback function for the processing of an obtained ORIGIN
					 *
					 * \~
					 */
					origin_callback_t origin;
					/**
					 * \~russian
					 * @brief Функция обратного вызова о готовности потока принимать данные тела
					 *
					 * \~english
					 * @brief Callback function about the readiness of a stream to accept the data of the body
					 *
					 * \~
					 */
					writable_callback_t writable;
					/**
					 * \~russian
					 * @brief Функция обратного вызова для обработки применённого SETTINGS пира
					 *
					 * \~english
					 * @brief Callback function for the processing of an applied SETTINGS of the peer
					 *
					 * \~
					 */
					settings_callback_t settings;
					/**
					 * \~russian
					 * @brief Функция обратного вызова для обработки провайдера заголовков потока
					 *
					 * \~english
					 * @brief Callback function for the processing of the provider of the headers of a stream
					 *
					 * \~
					 */
					provider_callback_t provider;
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
				// Флаги состояния соединения
				flags_t _flags;
			private:
				/**
				 * Поколение состояния соединения: увеличивается каждым сбросом (reset/clear).
				 * Пользовательская функция обратного вызова вправе сбросить парсер прямо
				 * из обработчика - после этого все ссылки на разбираемые данные, снимки
				 * потоков и списки заголовков недействительны, и разбор обязан свернуться
				 */
				uint64_t _epoch;
			private:
				// Код ошибки уровня соединения
				error_t _error;
			private:
				// Буферы соединения
				buffer_t _buffer;
			private:
				// Окна flow control соединения
				window_t _window;
			private:
				// Протокол, с которым работает парсер
				proto_t _proto;
			private:
				// Настраиваемые лимиты безопасности
				limits_t _limits;
			private:
				// Наши параметры SETTINGS
				settings_t _local;
				// Параметры SETTINGS пира
				settings_t _remote;
			private:
				// Состояние собираемого блока заголовков (HEADERS + CONTINUATION)
				header_block_t _hbc;
			private:
				// Rate-лимиты соединения
				ratelims_t _ratelims;
			private:
				// Параметры передачи данных
				transfer_t _transfer;
			private:
				// Объект функций обратного вызова
				callbacks_t _callbacks;
			private:
				/**
				 * Переиспользуемый список декодированных заголовков блока: представления
				 * ссылаются в арену декодера и действительны до следующего декодирования
				 */
				vector <h2::hpack::field_view_t> _fields;
			private:
				// HPACK-энкодер (наша динамическая таблица)
				h2::hpack::encoder_t _encoder;
				// HPACK-декодер (динамическая таблица пира)
				h2::hpack::decoder_t _decoder;
			private:
				/**
				 * \~russian
				 * @brief Метод передачи исходящих байтов сетевому слою через функцию обратного вызова записи
				 *
				 * @details Если функция записи не установлена - байты остаются во внутреннем
				 *          буфере до выборки через pending()/consumePending().
				 *
				 * \~english
				 * @brief Method of the transmission of the outgoing octets to the network layer through the callback function of the writing
				 * @details If the function of the writing is not set - the octets remain in the internal
				 *          buffer until the selection through pending()/consumePending().
				 *
				 * \~
				 */
				void flush() noexcept;
				/**
				 * \~russian
				 * @brief Метод очистки буфера незавершённого фрейма
				 *
				 * \~english
				 * @brief Method of the clearing of the buffer of an uncompleted frame
				 *
				 * \~
				 */
				void clearInput() noexcept;
				/**
				 * \~russian
				 * @brief Метод получения объёма накопленного незавершённого фрейма
				 *
				 * @return объём накопленной части незавершённого фрейма
				 *
				 * \~english
				 * @brief Method of getting the volume of the accumulated uncompleted frame
				 * @return volume of the accumulated part of the uncompleted frame
				 *
				 * \~
				 */
				size_t inputPending() const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения логического объёма ещё не отправленных исходящих байтов
				 *
				 * @return объём не отправленных исходящих байтов
				 *
				 * \~english
				 * @brief Method of getting the logical volume of the not yet sent outgoing octets
				 * @return volume of the not sent outgoing octets
				 *
				 * \~
				 */
				size_t outputPending() const noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод разбора порции входящих байтов (preface + поток фреймов)
				 *
				 * @details Разбор идёт прямо по буферу вызывающей стороны: копии входа
				 *          у парсера нет. Разбирается столько целых фреймов, сколько
				 *          уложилось в порцию; неразобранный хвост остаётся вызывающей
				 *          стороне и подаётся снова со следующей порцией
				 *
				 * @param buffer буфер входящих байтов
				 * @param size   размер буфера входящих байтов
				 * @return       результат разбора (OK/ERROR)
				 *
				 * \~english
				 * @brief Method of the parsing of a portion of the incoming octets (a preface + a stream of the frames)
				 * @details The parsing goes right by the buffer of the calling side: the parser has no copy
				 *          of the input. As many whole frames are parsed as have
				 *          fitted into the portion; the unparsed tail remains to the calling
				 *          side and is supplied again with the next portion
				 * @param buffer buffer of the incoming octets
				 * @param size   size of the buffer of the incoming octets
				 * @return       result of the parsing (OK/ERROR)
				 *
				 * \~
				 */
				h2::status_t parseInput(const uint8_t * buffer, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки одного полностью собранного фрейма
				 *
				 * @param header  заголовок фрейма
				 * @param payload полезная нагрузка фрейма
				 * @return        результат обработки (OK/ERROR)
				 *
				 * \~english
				 * @brief Method of the processing of a single fully assembled frame
				 * @param header  header of the frame
				 * @param payload payload of the frame
				 * @return        result of the processing (OK/ERROR)
				 *
				 * \~
				 */
				h2::status_t parseFrame(const h2::frame::header_t & header, const uint8_t * payload) noexcept;
				/**
				 * \~russian
				 * @brief Метод декодирования накопленного блока заголовков и вызова функций обратного вызова
				 *
				 * @return результат обработки (OK/ERROR)
				 *
				 * \~english
				 * @brief Method of the decoding of the accumulated block of the headers and of the call of the callback functions
				 * @return result of the processing (OK/ERROR)
				 *
				 * \~
				 */
				h2::status_t deliverHeaders() noexcept;
				/**
				 * \~russian
				 * @brief Метод аварийного завершения соединения (ошибка, GOAWAY, запись в лог)
				 *
				 * @param code    код ошибки протокола
				 * @param message текстовое описание ошибки
				 * @return        статус ошибки (для проброса из обработчиков)
				 *
				 * \~english
				 * @brief Method of an emergency completion of the connection (an error, a GOAWAY, a record into the log)
				 * @param code    error code of the protocol
				 * @param message text description of the error
				 * @return        status of the error (for a throwing from the handlers)
				 *
				 * \~
				 */
				h2::status_t fail(const error_t code, const char * message) noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки корректности нового потока, открываемого пиром (чётность + монотонность id)
				 *
				 * @param id  идентификатор потока
				 * @param err код ошибки протокола
				 * @return    результат проверки (OK/ERROR)
				 *
				 * \~english
				 * @brief Method of checking the correctness of a new stream opened by the peer (the parity + the monotonicity of the id)
				 * @param id  identifier of the stream
				 * @param err error code of the protocol
				 * @return    result of the checking (OK/ERROR)
				 *
				 * \~
				 */
				h2::status_t validateNewStream(const uint32_t id, error_t & err) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки одного полного фрейма
				 *
				 * @param header  заголовок фрейма
				 * @param payload полезная нагрузка фрейма (ровно h.length байт)
				 * @return        результат обработки (OK/ERROR)
				 *
				 * \~english
				 * @brief Method of the processing of a single whole frame
				 * @param header  header of the frame
				 * @param payload payload of the frame (exactly h.length octets)
				 * @return        result of the processing (OK/ERROR)
				 *
				 * \~
				 */
				h2::status_t dispatch(const h2::frame::header_t & header, const uint8_t * payload) noexcept;
				/**
				 * \~russian
				 * @brief Метод доставки декодированного блока обещанного запроса (PUSH_PROMISE, сторона клиента)
				 *
				 * @param sid         идентификатор ассоциированного потока клиента
				 * @param promisedSid идентификатор обещанного потока
				 * @param fields      декодированные заголовки обещанного запроса
				 * @return            результат обработки (OK/ERROR)
				 *
				 * \~english
				 * @brief Method of the delivery of a decoded block of a promised request (a PUSH_PROMISE, the side of the client)
				 * @param sid         identifier of the associated stream of the client
				 * @param promisedSid identifier of the promised stream
				 * @param fields      decoded headers of the promised request
				 * @return            result of the processing (OK/ERROR)
				 *
				 * \~
				 */
				h2::status_t deliverPushPromise(const uint32_t sid, const uint32_t promisedSid, const vector <h2::hpack::field_view_t> & fields) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод получения существующего либо создания нового потока
				 *
				 * @param id идентификатор потока
				 * @return   объект потока
				 *
				 * \~english
				 * @brief Method of getting an existing or of creating a new stream
				 * @param id identifier of the stream
				 * @return   object of the stream
				 *
				 * \~
				 */
				stream_t & stream(const uint32_t id) noexcept;
				/**
				 * \~russian
				 * @brief Метод поиска потока без создания
				 *
				 * @param id идентификатор потока
				 * @return   объект потока либо nullptr
				 *
				 * \~english
				 * @brief Method of the search of a stream without a creation
				 * @param id identifier of the stream
				 * @return   object of the stream or nullptr
				 *
				 * \~
				 */
				stream_t * findStream(const uint32_t id) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод применения отправленного нами END_STREAM (переход состояния, возможно закрытие потока)
				 *
				 * @param stream объект потока (ссылка может стать недействительной после вызова)
				 *
				 * \~english
				 * @brief Method of the application of an END_STREAM sent by us (a transition of the state, possibly a closing of the stream)
				 * @param stream object of the stream (the reference may become invalid after the call)
				 *
				 * \~
				 */
				void applyLocalEndStream(stream_t & stream) noexcept;
				/**
				 * \~russian
				 * @brief Метод применения полученного END_STREAM (переход состояния, возможно закрытие потока)
				 *
				 * @param stream объект потока (ссылка может стать недействительной после вызова)
				 *
				 * \~english
				 * @brief Method of the application of an obtained END_STREAM (a transition of the state, possibly a closing of the stream)
				 * @param stream object of the stream (the reference may become invalid after the call)
				 *
				 * \~
				 */
				void applyRemoteEndStream(stream_t & stream) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод проверки того, что поток инициирован пиром (а не нами)
				 *
				 * @param id идентификатор потока
				 * @return   результат проверки
				 *
				 * \~english
				 * @brief Method of checking that a stream is initiated by the peer (and not by us)
				 * @param id identifier of the stream
				 * @return   result of the checking
				 *
				 * \~
				 */
				bool peerInitiated(const uint32_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки того, что поток ещё ни разу не использовался (состояние idle)
				 *
				 * @details Поток пира считается использованным, пока его идентификатор не превышает
				 *          наибольший принятый; наш собственный - если он уже выдан nextStreamId().
				 *          Для закрытого и удалённого из карты потока метод возвращает false:
				 *          запоздалые фреймы на нём - штатная гонка, а не ошибка соединения
				 *
				 * @param id идентификатор потока
				 * @return   результат проверки
				 *
				 * \~english
				 * @brief Method of checking that a stream has not been used yet even once (the state idle)
				 * @details A stream of the peer is considered used while its identifier does not exceed
				 *          the largest accepted one; our own one - if it is already issued by nextStreamId().
				 *          For a closed and removed from the map stream the method returns false:
				 *          the belated frames on it are a regular race rather than an error of the connection
				 * @param id identifier of the stream
				 * @return   result of the checking
				 *
				 * \~
				 */
				bool idleStream(const uint32_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод запоминания потока, оборванного сбросом
				 *
				 * @details Кадры на оборванном потоке ещё могут быть в полёте: пир отправил
				 *          их до того, как получил наш RST_STREAM. Помнить об этом надо
				 *          и для потоков пира, и для наших собственных: клиент так же
				 *          вправе получить запоздалый ответ на поток, который сам и сбросил
				 *
				 * @param id    идентификатор потока
				 * @param local сброс отправлен нами (иначе - принят от пира)
				 *
				 * \~english
				 * @brief Method of the remembering of a stream broken by a reset
				 * @details The frames on a broken stream may still be in a flight: the peer has sent
				 *          them before it has obtained our RST_STREAM. To remember about this is necessary
				 *          both for the streams of the peer and for our own ones: a client likewise
				 *          is entitled to obtain a belated answer on a stream which it has itself reset
				 * @param id    identifier of the stream
				 * @param local the reset is sent by us (otherwise - accepted from the peer)
				 *
				 * \~
				 */
				void markReset(const uint32_t id, const bool local) noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки того, что поток был недавно оборван сбросом
				 *
				 * @param id идентификатор потока
				 * @return   результат проверки
				 *
				 * \~english
				 * @brief Method of checking that a stream has been recently broken by a reset
				 * @param id identifier of the stream
				 * @return   result of the checking
				 *
				 * \~
				 */
				bool wasReset(const uint32_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки того, что поток был оборван нашим сбросом
				 *
				 * @details После нашего RST_STREAM кадры пира обязаны молча игнорироваться:
				 *          он отправил их до того, как получил сброс, и повторный кадр сброса
				 *          ему ни о чём не сообщит (RFC 9113 §5.1)
				 *
				 * @param id идентификатор потока
				 * @return   результат проверки
				 *
				 * \~english
				 * @brief Method of checking that a stream has been broken by our reset
				 * @details After our RST_STREAM the frames of the peer are obliged to be silently ignored:
				 *          it has sent them before it has obtained the reset, and a repeated frame of a reset
				 *          will report nothing to it (RFC 9113 §5.1)
				 * @param id identifier of the stream
				 * @return   result of the checking
				 *
				 * \~
				 */
				bool resetLocally(const uint32_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки RST_STREAM с учётом оборванного потока
				 *
				 * @details Единственная точка отправки сброса внутри модуля: учёт оборванного
				 *          потока и сам кадр обязаны идти вместе, иначе запоздалые кадры
				 *          на нём выглядели бы как кадры на никогда не открывавшемся потоке
				 *          и рвали бы соединение
				 *
				 * @param id   идентификатор потока
				 * @param code код ошибки, с которым обрывается поток
				 *
				 * \~english
				 * @brief Method of the sending of a RST_STREAM with the account of the broken stream
				 * @details The only point of the sending of a reset inside the module: the account of the broken
				 *          stream and the frame itself are obliged to go together, otherwise the belated frames
				 *          on it would look like the frames on a never opened stream
				 *          and would break the connection
				 * @param id   identifier of the stream
				 * @param code error code with which the stream is broken
				 *
				 * \~
				 */
				void rejectStream(const uint32_t id, const error_t code) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод удаления потока из карты с корректным учётом счётчика встречных потоков
				 *
				 * @param id идентификатор потока
				 *
				 * \~english
				 * @brief Method of the removal of a stream from the map with a correct account of the counter of the oncoming streams
				 * @param id identifier of the stream
				 *
				 * \~
				 */
				void eraseStream(const uint32_t id) noexcept;
				/**
				 * \~russian
				 * @brief Метод закрытия потока с вызовом функции обратного вызова закрытия
				 *
				 * @param id   идентификатор потока
				 * @param code код ошибки закрытия
				 *
				 * \~english
				 * @brief Method of the closing of a stream with the call of the callback function of the closing
				 * @param id   identifier of the stream
				 * @param code error code of the closing
				 *
				 * \~
				 */
				void closeStream(const uint32_t id, const error_t code) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод вызова функции обратного вызова обработки фазы приёма сообщения потока
				 *
				 * @details При возврате false пользовательской функцией (или исключении)
				 *          поток сбрасывается (RST_STREAM с кодом CANCEL) и метод возвращает false.
				 *
				 * @param id    идентификатор потока
				 * @param phase фаза приёма сообщения потока
				 * @param part  часть сообщения (заголовки, трейлеры, тело), NONE - сообщение целиком
				 * @return      результат обработки (false - поток сброшен)
				 *
				 * \~english
				 * @brief Method of the call of the callback function of the processing of the phase of the acceptance of a message of a stream
				 * @details At a return of false by the user function (or at an exception)
				 *          the stream is reset (a RST_STREAM with the code CANCEL) and the method returns false.
				 * @param id    identifier of the stream
				 * @param phase phase of the acceptance of the message of the stream
				 * @param part  part of the message (the headers, the trailers, the body), NONE - the message as a whole
				 * @return      result of the processing (false - the stream is reset)
				 *
				 * \~
				 */
				bool firePhase(const uint32_t id, const phase_t phase, const part_t part) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод вызова функции обратного вызова обработки применённого SETTINGS пира
				 *
				 * @details Все методы разбора объявлены noexcept, поэтому исключение из
				 *          пользовательской функции обязано быть перехвачено на месте вызова -
				 *          иначе оно завершает процесс, не доходя до обработчика в parse()
				 *
				 * \~english
				 * @brief Method of the call of the callback function of the processing of an applied SETTINGS of the peer
				 * @details All the methods of the parsing are declared noexcept, therefore an exception from
				 *          a user function is obliged to be intercepted at the place of the call -
				 *          otherwise it terminates the process without reaching the handler in parse()
				 *
				 * \~
				 */
				void fireSettings() noexcept;
				/**
				 * \~russian
				 * @brief Метод вызова функции обратного вызова о готовности потока принимать данные
				 *
				 * @param id идентификатор потока
				 *
				 * \~english
				 * @brief Method of the call of the callback function about the readiness of a stream to accept the data
				 * @param id identifier of the stream
				 *
				 * \~
				 */
				void fireWritable(const uint32_t id) noexcept;
				/**
				 * \~russian
				 * @brief Метод вызова функции обратного вызова обработки открытия нового потока
				 *
				 * @param id идентификатор потока
				 * @return   результат обработки (false - поток требуется сбросить)
				 *
				 * \~english
				 * @brief Method of the call of the callback function of the processing of the opening of a new stream
				 * @param id identifier of the stream
				 * @return   result of the processing (false - the stream is required to be reset)
				 *
				 * \~
				 */
				bool fireBegin(const uint32_t id) noexcept;
				/**
				 * \~russian
				 * @brief Метод вызова функции обратного вызова обработки анонса server push
				 *
				 * @param sid         идентификатор ассоциированного потока клиента
				 * @param promisedSid идентификатор обещанного потока
				 * @return            результат обработки (false - push требуется отклонить)
				 *
				 * \~english
				 * @brief Method of the call of the callback function of the processing of an announcement of a server push
				 * @param sid         identifier of the associated stream of the client
				 * @param promisedSid identifier of the promised stream
				 * @return            result of the processing (false - the push is required to be rejected)
				 *
				 * \~
				 */
				bool firePush(const uint32_t sid, const uint32_t promisedSid) noexcept;
				/**
				 * \~russian
				 * @brief Метод вызова функции обратного вызова обработки полученного GOAWAY
				 *
				 * @param sid   наибольший идентификатор обработанного пиром потока
				 * @param code  код ошибки завершения соединения
				 * @param debug отладочные данные пира
				 *
				 * \~english
				 * @brief Method of the call of the callback function of the processing of an obtained GOAWAY
				 * @param sid   largest identifier of a stream processed by the peer
				 * @param code  error code of the completion of the connection
				 * @param debug debug data of the peer
				 *
				 * \~
				 */
				void fireGoaway(const uint32_t sid, const error_t code, const string_view debug) noexcept;
				/**
				 * \~russian
				 * @brief Метод вызова функции обратного вызова обработки провайдера заголовков потока
				 *
				 * @param id        идентификатор потока
				 * @param provider  провайдер заголовков потока (nullptr для трейлеров)
				 * @param endStream флаг завершения потока
				 * @return          результат обработки (false - поток требуется сбросить)
				 *
				 * \~english
				 * @brief Method of the call of the callback function of the processing of the provider of the headers of a stream
				 * @param id        identifier of the stream
				 * @param provider  provider of the headers of the stream (a nullptr for the trailers)
				 * @param endStream flag of the completion of the stream
				 * @return          result of the processing (false - the stream is required to be reset)
				 *
				 * \~
				 */
				bool fireProvider(const uint32_t id, const provider_t * provider, const bool endStream) noexcept;
				/**
				 * \~russian
				 * @brief Метод вызова функции обратного вызова обработки фрагмента тела потока
				 *
				 * @param id        идентификатор потока
				 * @param buffer    буфер данных тела
				 * @param size      размер данных тела
				 * @param endStream флаг завершения потока
				 * @return          результат обработки (false - поток требуется сбросить)
				 *
				 * \~english
				 * @brief Method of the call of the callback function of the processing of a fragment of the body of a stream
				 * @param id        identifier of the stream
				 * @param buffer    buffer of the data of the body
				 * @param size      size of the data of the body
				 * @param endStream flag of the completion of the stream
				 * @return          result of the processing (false - the stream is required to be reset)
				 *
				 * \~
				 */
				bool fireData(const uint32_t id, const void * buffer, const size_t size, const bool endStream) noexcept;
				/**
				 * \~russian
				 * @brief Метод вызова функции обратного вызова обработки заголовка или трейлера потока
				 *
				 * @param id    идентификатор потока
				 * @param name  название заголовка
				 * @param value значение заголовка
				 * @param part  часть сообщения (HEADERS или TRAILER)
				 * @return      результат обработки (false - поток требуется сбросить)
				 *
				 * \~english
				 * @brief Method of the call of the callback function of the processing of a header or of a trailer of a stream
				 * @param id    identifier of the stream
				 * @param name  name of the header
				 * @param value value of the header
				 * @param part  part of the message (HEADERS or TRAILER)
				 * @return      result of the processing (false - the stream is required to be reset)
				 *
				 * \~
				 */
				bool fireHeader(const uint32_t id, const string_view name, const string_view value, const part_t part) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод постановки потока в очередь готовых к отправке
				 *
				 * @details Вызывается там, где поток обзаводится содержимым: буфером тела,
				 *          источником данных, отложенным завершением либо трейлерами.
				 *          Пропущенный вызов оставит содержимое неотправленным, поэтому
				 *          добавление обязано опережать прокачку
				 *
				 * @param stream объект потока
				 *
				 * \~english
				 * @brief Method of the putting of a stream into the queue of those ready for the sending
				 * @details It is called there where a stream acquires a content: a buffer of the body,
				 *          a source of the data, a postponed completion or the trailers.
				 *          A missed call will leave the content not sent, therefore
				 *          the addition is obliged to outstrip the pumping
				 * @param stream object of the stream
				 *
				 * \~
				 */
				void markReady(stream_t & stream) noexcept;
				/**
				 * \~russian
				 * @brief Метод прокачки отправки по всем потокам с учётом окон и порога выходного буфера
				 *
				 * @details Round-robin: за каждый проход отправляется не более одного DATA-фрейма
				 *          с потока, пока хоть один поток делает прогресс - исключает голодание
				 *          потоков (head-of-line blocking).
				 *
				 * \~english
				 * @brief Method of the pumping of the sending over all the streams with the account of the windows and of the threshold of the output buffer
				 * @details A round-robin: per every pass not more than one DATA frame is sent
				 *          from a stream, while at least one stream makes a progress - it excludes a starvation
				 *          of the streams (a head-of-line blocking).
				 *
				 * \~
				 */
				void pump() noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки не более одного DATA-фрейма потока
				 *
				 * @param stream объект потока (ссылка может стать недействительной после вызова)
				 * @return       признак прогресса отправки
				 *
				 * \~english
				 * @brief Method of the sending of not more than one DATA frame of a stream
				 * @param stream object of the stream (the reference may become invalid after the call)
				 * @return       flag of a progress of the sending
				 *
				 * \~
				 */
				bool pumpStream(stream_t & stream) noexcept;
				/**
				 * \~russian
				 * @brief Метод дозагрузки буфера отправки из pull-источника данных (если он задан)
				 *
				 * @param stream объект потока
				 *
				 * \~english
				 * @brief Method of the loading of the buffer of the sending from the pull source of the data (if it is given)
				 * @param stream object of the stream
				 *
				 * \~
				 */
				void refillFromSource(stream_t & stream) noexcept;
				/**
				 * \~russian
				 * @brief Метод сигнализации о готовности потока принимать данные (один раз на провал буфера)
				 *
				 * @param stream объект потока
				 *
				 * \~english
				 * @brief Method of the signalling about the readiness of a stream to accept the data (once per descent of the buffer)
				 * @param stream object of the stream
				 *
				 * \~
				 */
				void maybeNotifyWritable(stream_t & stream) noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки того, что все данные потока для отправки уже получены
				 *
				 * @param stream объект потока
				 * @return       результат проверки (нет источника данных или достигнут его eof)
				 *
				 * \~english
				 * @brief Method of checking that all the data of a stream for the sending is already obtained
				 * @param stream object of the stream
				 * @return       result of the checking (there is no source of the data or its eof is reached)
				 *
				 * \~
				 */
				bool sourceDone(const stream_t & stream) const noexcept;
				/**
				 * \~russian
				 * @brief Метод пополнения окна приёма (соединения/потока) с отправкой WINDOW_UPDATE при просадке
				 *
				 * @param stream   объект потока (nullptr - только окно соединения)
				 * @param consumed число принятых байт
				 *
				 * \~english
				 * @brief Method of the replenishment of the window of the acceptance (of the connection/of a stream) with the sending of a WINDOW_UPDATE at a descent
				 * @param stream   object of the stream (a nullptr - only the window of the connection)
				 * @param consumed number of the accepted octets
				 *
				 * \~
				 */
				void replenishReceiveWindow(stream_t * stream, const uint32_t consumed) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод проверки декодированных заголовков на лимиты безопасности
				 *
				 * @param fields декодированные заголовки блока
				 * @return       результат проверки (false - лимиты превышены)
				 *
				 * \~english
				 * @brief Method of checking the decoded headers for the limits of the safety
				 * @param fields decoded headers of the block
				 * @return       result of the checking (false - the limits are exceeded)
				 *
				 * \~
				 */
				bool checkHeaderLimits(const vector <h2::hpack::field_view_t> & fields) const noexcept;
				/**
				 * \~russian
				 * @brief Метод предупреждения о полностью снятом лимите списка заголовков
				 *
				 * @details Вызывается при изменении лимитов безопасности и параметров SETTINGS:
				 *          снятие обоих лимитов сразу оставляет арену декодера без границы
				 *
				 * \~english
				 * @brief Method of the warning about a fully removed limit of the list of the headers
				 * @details It is called at a change of the limits of the safety and of the parameters of SETTINGS:
				 *          a removal of both limits at once leaves the arena of the decoder without a boundary
				 *
				 * \~
				 */
				void checkHeaderListLimits() const noexcept;
				/**
				 * \~russian
				 * @brief Метод сверки отправляемого блока заголовков с лимитом пира
				 *
				 * @details Пир анонсирует SETTINGS_MAX_HEADER_LIST_SIZE как рекомендацию
				 *          (RFC 9113 §6.5.2) и вправе отвергнуть превышающий её блок.
				 *          Отправку не блокирует - только предупреждает в лог
				 *
				 * @param sid идентификатор потока
				 *
				 * \~english
				 * @brief Method of the comparison of a block of the headers being sent with the limit of the peer
				 * @details The peer announces a SETTINGS_MAX_HEADER_LIST_SIZE as a recommendation
				 *          (RFC 9113 §6.5.2) and is entitled to reject a block exceeding it.
				 *          It does not block the sending - it only warns into the log
				 * @param sid identifier of the stream
				 *
				 * \~
				 */
				void checkPeerHeaderList(const uint32_t sid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки соответствия принятого тела объявленному content-length
				 *
				 * @details RFC 9113 §8.1.1: расхождение суммы длин DATA с content-length делает
				 *          сообщение малформированным. При расхождении поток сбрасывается
				 *
				 * @param sid идентификатор потока
				 * @return    результат проверки (false - поток сброшен)
				 *
				 * \~english
				 * @brief Method of checking the correspondence of an accepted body to an announced content-length
				 * @details RFC 9113 §8.1.1: a divergence of the sum of the lengths of the DATA with the content-length makes
				 *          a message malformed. At a divergence the stream is reset
				 * @param sid identifier of the stream
				 * @return    result of the checking (false - the stream is reset)
				 *
				 * \~
				 */
				bool checkBodyLength(const uint32_t sid) noexcept;
				/**
				 * \~russian
				 * @brief Метод применения расширенного приоритета к потоку (RFC 9218 §4)
				 *
				 * @details Значение - структурированный словарь вида "u=2, i": ключ [u] задаёт
				 *          срочность 0..7 (по умолчанию 3), ключ [i] - инкрементальную доставку.
				 *          Неизвестные и некорректные ключи игнорируются (RFC 9218 §4.3)
				 *
				 * @param stream объект потока
				 * @param value  значение поля приоритета
				 *
				 * \~english
				 * @brief Method of the application of an extended priority to a stream (RFC 9218 §4)
				 * @details The value is a structured dictionary of the form "u=2, i": the key [u] sets
				 *          the urgency 0..7 (by default 3), the key [i] - the incremental delivery.
				 *          The unknown and the incorrect keys are ignored (RFC 9218 §4.3)
				 * @param stream object of the stream
				 * @param value  value of the field of the priority
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
				 * @param urgency     срочность потока (выходной параметр)
				 * @param incremental признак инкрементальной доставки (выходной параметр)
				 *
				 * \~english
				 * @brief Method of the parsing of the value of the field of an extended priority (RFC 9218 §4)
				 * @details A signal sets the priority entirely: a parameter absent in it
				 *          takes the value by default rather than preserving the previous one
				 * @param value       value of the field of the priority
				 * @param urgency     urgency of the stream (an output parameter)
				 * @param incremental flag of the incremental delivery (an output parameter)
				 *
				 * \~
				 */
				void parsePriority(const string_view value, uint8_t & urgency, bool & incremental) const noexcept;
				/**
				 * \~russian
				 * @brief Метод запоминания приоритета ещё не открытого потока (RFC 9218 §7.1)
				 *
				 * @param id    идентификатор приоритизируемого потока
				 * @param value значение поля приоритета
				 * @return      результат запоминания (false - исчерпан лимит одновременных потоков)
				 *
				 * \~english
				 * @brief Method of the remembering of the priority of a not yet opened stream (RFC 9218 §7.1)
				 * @param id    identifier of the stream being prioritized
				 * @param value value of the field of the priority
				 * @return      result of the remembering (false - the limit of the simultaneous streams is exhausted)
				 *
				 * \~
				 */
				bool deferPriority(const uint32_t id, const string_view value) noexcept;
				/**
				 * \~russian
				 * @brief Метод применения приоритета, отложенного до открытия потока
				 *
				 * @details Вместе с применённой записью снимаются записи потоков с меньшими
				 *          идентификаторами: идентификаторы пира строго возрастают
				 *          (RFC 9113 §5.1.1), и открыты такие потоки уже не будут
				 *
				 * @param stream объект открываемого потока
				 *
				 * \~english
				 * @brief Method of the application of a priority postponed to the opening of a stream
				 * @details Together with the applied record the records of the streams with the smaller
				 *          identifiers are removed: the identifiers of the peer strictly increase
				 *          (RFC 9113 §5.1.1), and such streams will no longer be opened
				 * @param stream object of the stream being opened
				 *
				 * \~
				 */
				void applyPendingPriority(stream_t & stream) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод отправки собранного HPACK-блока заголовков потока
				 *
				 * @details Общая часть перегрузок sendHeaders: нарезка блока на
				 *          HEADERS + CONTINUATION и переходы состояния потока.
				 *
				 * @param sid       идентификатор потока
				 * @param block     закодированный HPACK-блок заголовков
				 * @param endStream флаг завершения потока (тела не будет)
				 *
				 * \~english
				 * @brief Method of the sending of an assembled HPACK block of the headers of a stream
				 * @details The common part of the overloads of sendHeaders: the cutting of the block into
				 *          a HEADERS + a CONTINUATION and the transitions of the state of the stream.
				 * @param sid       identifier of the stream
				 * @param block     encoded HPACK block of the headers
				 * @param endStream flag of the completion of the stream (there will be no body)
				 *
				 * \~
				 */
				void commitHeaders(const uint32_t sid, const string & block, const bool endStream);
				/**
				 * \~russian
				 * @brief Метод откладывания секции трейлеров до конца отправки тела потока
				 *
				 * @details Трейлеры идут после тела, а тело может ждать открытия окна
				 *          управления потоком. Отправить их сразу означает выпустить
				 *          в сеть блок заголовков раньше данных, которые он завершает.
				 *          Откладываются именно поля: кодирование HPACK обязано
				 *          совпадать по порядку с отправкой
				 *
				 * @param sid       идентификатор потока
				 * @param fields    заголовки секции трейлеров
				 * @param endStream флаг завершения потока
				 * @return          результат откладывания (true - отправка отложена)
				 *
				 * \~english
				 * @brief Method of the postponement of a section of the trailers to the end of the sending of the body of a stream
				 * @details The trailers go after the body, while the body may wait for the opening of the window
				 *          of the flow control. To send them at once means to issue
				 *          into the network a block of the headers earlier than the data which it completes.
				 *          Exactly the fields are postponed: the encoding of HPACK is obliged
				 *          to coincide in the order with the sending
				 * @param sid       identifier of the stream
				 * @param fields    headers of the section of the trailers
				 * @param endStream flag of the completion of the stream
				 * @return          result of the postponement (true - the sending is postponed)
				 *
				 * \~
				 */
				bool deferTrailers(const uint32_t sid, const vector <h2::hpack::field_t> & fields, const bool endStream) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки отложенной секции трейлеров потока
				 *
				 * @param stream объект потока (ссылка может стать недействительной после вызова)
				 * @return       признак отправки секции трейлеров
				 *
				 * \~english
				 * @brief Method of the sending of a postponed section of the trailers of a stream
				 * @param stream object of the stream (the reference may become invalid after the call)
				 * @return       flag of the sending of the section of the trailers
				 *
				 * \~
				 */
				bool flushTrailers(stream_t & stream) noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки допустимости отправки блока заголовков в поток
				 *
				 * @details Отправка допустима в существующий поток (ответ/трейлеры) либо
				 *          в новый поток, идентификатор которого выделен нами через
				 *          nextStreamId() и ещё не использовался. Проверка выполняется
				 *          ДО кодирования блока: HPACK-кодирование меняет динамическую
				 *          таблицу, поэтому отброшенный после кодирования блок
				 *          рассинхронизировал бы декодер пира.
				 *
				 * @param sid идентификатор потока
				 * @return    результат проверки
				 *
				 * \~english
				 * @brief Method of checking the admissibility of the sending of a block of the headers into a stream
				 * @details The sending is admissible into an existing stream (an answer/the trailers) or
				 *          into a new stream the identifier of which is allotted by us through
				 *          nextStreamId() and has not been used yet. The checking is performed
				 *          BEFORE the encoding of the block: the HPACK encoding changes the dynamic
				 *          table, therefore a block discarded after the encoding
				 *          would desynchronize the decoder of the peer.
				 * @param sid identifier of the stream
				 * @return    result of the checking
				 *
				 * \~
				 */
				bool canSendHeaders(const uint32_t sid) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод построения провайдера заголовков потока из псевдо-заголовков
				 *
				 * @param fields  декодированные заголовки блока
				 * @param request собирается запрос клиента (true) или ответ сервера (false)
				 * @return        собранный провайдер заголовков
				 *
				 * \~english
				 * @brief Method of the building of the provider of the headers of a stream out of the pseudo headers
				 * @param fields  decoded headers of the block
				 * @param request a request of a client is assembled (true) or an answer of a server (false)
				 * @return        assembled provider of the headers
				 *
				 * \~
				 */
				unique_ptr <provider_t> buildProvider(const vector <h2::hpack::field_view_t> & fields, const bool request) const noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки того, что расширенный CONNECT разрешён нами
				 *
				 * @details Разрешение выдаётся параметром SETTINGS_ENABLE_CONNECT_PROTOCOL
				 *          либо подразумевается ролью узла: соединение, объявленное несущим
				 *          WebSocket, иначе отвергало бы единственный запрос, ради которого
				 *          заведено (RFC 8441 §3, §5)
				 *
				 * @return признак разрешения расширенного CONNECT
				 *
				 * \~english
				 * @brief Method of checking that an extended CONNECT is permitted by us
				 * @details The permission is issued by the parameter SETTINGS_ENABLE_CONNECT_PROTOCOL
				 *          or is implied by the role of the node: a connection announced as carrying
				 *          a WebSocket would otherwise reject the only request for the sake of which
				 *          it is started (RFC 8441 §3, §5)
				 * @return flag of the permission of an extended CONNECT
				 *
				 * \~
				 */
				bool connectProtocol() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод полной очистки всех данных парсера
				 *
				 * @details Помимо полного сброса состояния соединения возвращает лимиты
				 *          безопасности и параметры SETTINGS к значениям по умолчанию
				 *          и удаляет установленные функции обратного вызова.
				 *
				 * \~english
				 * @brief Method of a full clearing of all the data of the parser
				 * @details Besides a full reset of the state of the connection it returns the limits
				 *          of the safety and the parameters of SETTINGS to the values by default
				 *          and removes the set callback functions.
				 *
				 * \~
				 */
				void clear() noexcept override;
				/**
				 * \~russian
				 * @brief Метод полного сброса состояния соединения
				 *
				 * @details В отличие от HTTP/1.x, где reset() готовит парсер к следующему
				 *          сообщению, для HTTP/2 сбрасывается ВСЁ соединение: HPACK-таблицы,
				 *          карта потоков, окна, буферы (семантика нового соединения).
				 *          Лимиты безопасности, параметры SETTINGS и функции обратного
				 *          вызова сохраняются.
				 *
				 * \~english
				 * @brief Method of a full reset of the state of the connection
				 * @details Unlike HTTP/1.x where reset() prepares the parser for the next
				 *          message, for HTTP/2 the WHOLE connection is reset: the tables of HPACK,
				 *          the map of the streams, the windows, the buffers (the semantics of a new connection).
				 *          The limits of the safety, the parameters of SETTINGS and the callback
				 *          functions are preserved.
				 *
				 * \~
				 */
				void reset() noexcept override;
			public:
				/**
				 * \~russian
				 * @brief Метод клонирования объекта парсера
				 *
				 * @details Клон получает те же направление трафика, роль узла на соединении,
				 *          лимиты безопасности, параметры SETTINGS и функции обратного вызова,
				 *          но чистое состояние соединения ("фабрика с теми же настройками").
				 *
				 * @return копия объекта парсера
				 *
				 * \~english
				 * @brief Method of cloning the object of the parser
				 * @details The clone gets the same direction of the traffic, role of the node on the connection,
				 *          limits of the safety, parameters of SETTINGS and callback functions,
				 *          but a clean state of the connection («a factory with the same settings»).
				 * @return copy of the object of the parser
				 *
				 * \~
				 */
				unique_ptr <parser_t> clone() const noexcept override;
			public:
				/**
				 * \~russian
				 * @brief Метод уведомления парсера о завершении потока данных (закрытии соединения)
				 *
				 * @details Если соединение завершено корректно (обменялись GOAWAY, активных
				 *          потоков нет) - фиксируется статус COMPLETE. Если соединение закрыто
				 *          посреди активных потоков или незавершённого фрейма - фиксируется
				 *          ошибка PROTOCOL_ERROR (обрыв соединения).
				 *
				 * \~english
				 * @brief Method of notifying the parser about the completion of the stream of the data (the closing of the connection)
				 * @details If the connection is completed correctly (the GOAWAY are exchanged, there are no active
				 *          streams) - the status COMPLETE is fixed. If the connection is closed
				 *          in the middle of the active streams or of an uncompleted frame - the error
				 *          PROTOCOL_ERROR is fixed (a break of the connection).
				 *
				 * \~
				 */
				void eof() noexcept override;
				/**
				 * \~russian
				 * @brief Метод разбора данных
				 *
				 * @details Скармливает парсеру очередную порцию входящих байтов соединения.
				 *          Неполный хвост фрейма буферизуется внутри до следующего вызова,
				 *          поэтому метод всегда потребляет все переданные байты. По ходу
				 *          разбора вызываются функции обратного вызова, а обязательные
				 *          ответные фреймы уходят в канал записи.
				 *          Итоговый статус необходимо контролировать методом status():
				 *          - PARTIAL:  соединение живо, разбор продолжается;
				 *          - COMPLETE: соединение завершено (обменялись GOAWAY);
				 *          - ERROR:    ошибка уровня соединения - причина в методе error().
				 *
				 * @param buffer буфер данных для разбора
				 * @param size   размер данных для разбора
				 * @return       количество обработанных байт данных
				 *
				 * \~english
				 * @brief Method of parsing the data
				 * @details It feeds the parser the next portion of the incoming octets of the connection.
				 *          An incomplete tail of a frame is buffered inside until the next call,
				 *          therefore the method always consumes all the transmitted octets. In the course
				 *          of the parsing the callback functions are called, while the obligatory
				 *          reply frames go away into the channel of the writing.
				 *          The resulting status is necessary to control by the method status():
				 *          - PARTIAL:  the connection is alive, the parsing continues;
				 *          - COMPLETE: the connection is completed (the GOAWAY are exchanged);
				 *          - ERROR:    an error of the level of the connection - the reason is in the method error().
				 * @param buffer buffer of the data for the parsing
				 * @param size   size of the data for the parsing
				 * @return       number of the processed octets of the data
				 *
				 * \~
				 */
				size_t parse(const void * buffer, const size_t size) noexcept override;
			public:
				/**
				 * \~russian
				 * @brief Метод получения кода ошибки уровня соединения
				 *
				 * @return код ошибки протокола
				 *
				 * \~english
				 * @brief Method of getting the error code of the level of the connection
				 * @return error code of the protocol
				 *
				 * \~
				 */
				error_t error() const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения человекочитаемого названия текущей ошибки разбора
				 *
				 * @return название текущей ошибки разбора
				 *
				 * \~english
				 * @brief Method of getting the human-readable name of the current error of the parsing
				 * @return name of the current error of the parsing
				 *
				 * \~
				 */
				string_view errorName() const noexcept override;
				/**
				 * \~russian
				 * @brief Метод получения человекочитаемого названия кода ошибки
				 *
				 * @param error код ошибки протокола
				 * @return      название кода ошибки
				 *
				 * \~english
				 * @brief Method of getting the human-readable name of an error code
				 * @param error error code of the protocol
				 * @return      name of the error code
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
				 * @details Парсер говорит только на HTTP/2, поэтому допустимы лишь
				 *          значения этого семейства: HTTP2 - прямое соединение с узлом,
				 *          PROXY2 - работа промежуточным узлом, WEBSOCKET2 - соединение,
				 *          несущее WebSocket поверх расширенного CONNECT. Значение
				 *          любого другого семейства отвергается с записью в лог:
				 *          разбирать HTTP/1.x либо HTTP/3 этот парсер не умеет, и
				 *          молчаливое принятие такого указания создало бы у вызывающей
				 *          стороны ложное представление о происходящем.
				 *          По умолчанию установлен HTTP2.
				 *
				 * @note Режим промежуточного узла ужесточает приём: конечному получателю
				 *       достаточно минимальной проверки полей (RFC 9113 §8.2.1), а узлу,
				 *       передающему сообщение дальше в другой версии протокола, - нет.
				 *       Отвергается то, границу чего следующее звено определит иначе:
				 *       управляющие символы в значениях полей, пробелы и управляющие
				 *       символы в псевдо-заголовках, из которых собирается стартовая
				 *       строка, а также объявленная длина тела у ответов 1xx и 204
				 *       (RFC 9113 §10.3). Подробности - в README модуля
				 *
				 * @param proto протокол работы парсера
				 *
				 * \~english
				 * @brief Method of setting the protocol with which the parser works
				 * @details The parser speaks only HTTP/2, therefore only the values
				 *          of this family are admissible: HTTP2 - a direct connection with a node,
				 *          PROXY2 - a work as an intermediate node, WEBSOCKET2 - a connection
				 *          carrying a WebSocket over an extended CONNECT. A value
				 *          of any other family is rejected with a record into the log:
				 *          this parser is not able to parse HTTP/1.x or HTTP/3, and
				 *          a silent acceptance of such an indication would create at the calling
				 *          side a false notion of what is happening.
				 *          By default HTTP2 is set.
				 * @note The mode of an intermediate node toughens the acceptance: for a final receiver
				 *       a minimal check of the fields suffices (RFC 9113 §8.2.1), while for a node
				 *       passing a message onward in another version of the protocol - not.
				 *       That is rejected the boundary of which the next link will determine differently:
				 *       the control characters in the values of the fields, the spaces and the control
				 *       characters in the pseudo headers out of which the starting line
				 *       is assembled, and also an announced length of the body at the answers 1xx and 204
				 *       (RFC 9113 §10.3). The details are in the README of the module
				 * @param proto protocol of the work of the parser
				 *
				 * \~
				 */
				void proto(const proto_t proto) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения лимитов безопасности
				 *
				 * @return лимиты безопасности
				 *
				 * \~english
				 * @brief Method of getting the limits of the safety
				 * @return limits of the safety
				 *
				 * \~
				 */
				const limits_t & limits() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки лимитов безопасности
				 *
				 * @param limits лимиты безопасности
				 *
				 * \~english
				 * @brief Method of setting the limits of the safety
				 * @param limits limits of the safety
				 *
				 * \~
				 */
				void limits(const limits_t & limits) noexcept;
			public:
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
				 * @note Отправка выполняется методами sendPreface()/sendSettings()
				 *
				 * @param settings наши параметры SETTINGS
				 *
				 * \~english
				 * @brief Method of setting our parameters of SETTINGS
				 * @note The sending is performed by the methods sendPreface()/sendSettings()
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
			public:
				/**
				 * \~russian
				 * @brief Метод проверки того, что соединение помечено на завершение
				 *
				 * @return признак завершения (отправлен или получен GOAWAY)
				 *
				 * \~english
				 * @brief Method of checking that the connection is marked for a completion
				 * @return flag of the completion (a GOAWAY is sent or obtained)
				 *
				 * \~
				 */
				bool isClosed() const noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки того, что наш SETTINGS подтверждён пиром
				 *
				 * @details Позволяет внешнему таймеру отследить отсутствие ACK и завершить
				 *          соединение с кодом SETTINGS_TIMEOUT (RFC 9113 §6.5.3)
				 *
				 * @return признак получения ACK на наш SETTINGS
				 *
				 * \~english
				 * @brief Method of checking that our SETTINGS is confirmed by the peer
				 * @details It allows an external timer to track the absence of an ACK and to complete
				 *          the connection with the code SETTINGS_TIMEOUT (RFC 9113 §6.5.3)
				 * @return flag of the receipt of an ACK to our SETTINGS
				 *
				 * \~
				 */
				bool isSettingsAcked() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод отправки исходящего preface соединения
				 *
				 * @details Клиент отправляет magic-строку + свой SETTINGS, сервер - только SETTINGS.
				 *          Обязан быть первым исходящим сообщением соединения.
				 *
				 * \~english
				 * @brief Method of the sending of the outgoing preface of the connection
				 * @details A client sends the magic string + its SETTINGS, a server - only the SETTINGS.
				 *          It is obliged to be the first outgoing message of the connection.
				 *
				 * \~
				 */
				void sendPreface() noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки нашего SETTINGS-фрейма (текущие параметры)
				 *
				 * \~english
				 * @brief Method of the sending of our SETTINGS frame (the current parameters)
				 *
				 * \~
				 */
				void sendSettings() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод отправки RST_STREAM (аварийное закрытие потока)
				 *
				 * @note Поток удаляется из карты активных с вызовом функции обратного вызова
				 *       закрытия - так же, как при сбросе потока пиром
				 *
				 * @param sid  идентификатор потока
				 * @param code код ошибки, с которым сбрасывается поток
				 *
				 * \~english
				 * @brief Method of the sending of a RST_STREAM (an emergency closing of a stream)
				 * @note The stream is removed from the map of the active ones with the call of the callback function
				 *       of the closing - the same as at a reset of a stream by the peer
				 * @param sid  identifier of the stream
				 * @param code error code with which the stream is reset
				 *
				 * \~
				 */
				void sendRstStream(const uint32_t sid, const error_t code) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки GOAWAY (пометка соединения завершаемым)
				 *
				 * @param code  код ошибки завершения соединения
				 * @param debug необязательные отладочные данные
				 *
				 * \~english
				 * @brief Method of the sending of a GOAWAY (a marking of the connection as being completed)
				 * @param code  error code of the completion of the connection
				 * @param debug optional debug data
				 *
				 * \~
				 */
				void sendGoaway(const error_t code, string_view debug = {}) noexcept;
				/**
				 * \~russian
				 * @brief Метод начала плавного завершения соединения (RFC 9113 §6.8)
				 *
				 * @details Отправляет предупреждающий GOAWAY с максимальным идентификатором
				 *          потока: пир узнаёт о предстоящем закрытии, но уже начатые потоки
				 *          не отклоняются и новые формально ещё допустимы. Через интервал
				 *          порядка RTT (например, после ответа на PING) вызовите sendGoaway()
				 *          с фактическим кодом - он объявит реально обработанный поток
				 *          и запретит новые. Без второй фазы соединение не завершается.
				 *
				 * @param debug необязательные отладочные данные
				 *
				 * \~english
				 * @brief Method of the beginning of a smooth completion of the connection (RFC 9113 §6.8)
				 * @details It sends a warning GOAWAY with the largest identifier
				 *          of a stream: the peer learns about the forthcoming closing, but the already begun streams
				 *          are not rejected and the new ones are formally still admissible. After an interval
				 *          of the order of an RTT (for example, after an answer to a PING) call sendGoaway()
				 *          with the actual code - it will announce the really processed stream
				 *          and will prohibit the new ones. Without the second phase the connection is not completed.
				 * @param debug optional debug data
				 *
				 * \~
				 */
				void sendShutdown(string_view debug = {}) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки расширенного приоритета потока (RFC 9218 §7.1)
				 *
				 * @details Кадр PRIORITY_UPDATE перепланирует уже открытый поток. Отправляется
				 *          только если пир объявил отказ от приоритетов RFC 7540 либо явно
				 *          поддерживает расширенные приоритеты
				 *
				 * @param sid         идентификатор потока
				 * @param urgency     срочность потока (0 - наивысшая, 7 - наименьшая)
				 * @param incremental признак инкрементальной доставки
				 *
				 * \~english
				 * @brief Method of the sending of an extended priority of a stream (RFC 9218 §7.1)
				 * @details A PRIORITY_UPDATE frame replans an already opened stream. It is sent
				 *          only if the peer has announced a refusal of the priorities of RFC 7540 or explicitly
				 *          supports the extended priorities
				 * @param sid         identifier of the stream
				 * @param urgency     urgency of the stream (0 - the highest, 7 - the least)
				 * @param incremental flag of the incremental delivery
				 *
				 * \~
				 */
				void sendPriority(const uint32_t sid, const uint8_t urgency, const bool incremental) noexcept;
				/**
				 * \~russian
				 * @brief Метод объявления приоритета собственного потока заголовком (RFC 9218 §5)
				 *
				 * @details Приоритет применяется к планировщику отправки сразу, а в секцию
				 *          заголовков потока добавляется заголовок [priority] - при условии,
				 *          что приложение не задало его само. Это единственный способ
				 *          объявить приоритет для сервера: кадр PRIORITY_UPDATE ему запрещён
				 *
				 * @note Заголовок добавляется только перегрузкой sendHeaders с контейнером
				 *       headers_t. Перегрузка со списком полей отдаёт их как есть -
				 *       это её контракт
				 *
				 * @param sid         идентификатор потока
				 * @param urgency     срочность потока (0 - наивысшая, 7 - наименьшая)
				 * @param incremental признак инкрементальной доставки
				 *
				 * \~english
				 * @brief Method of the announcement of the priority of one's own stream by a header (RFC 9218 §5)
				 * @details The priority is applied to the scheduler of the sending at once, while into the section
				 *          of the headers of the stream the header [priority] is added - on the condition
				 *          that the application has not set it itself. This is the only way
				 *          to announce a priority for a server: a PRIORITY_UPDATE frame is prohibited to it
				 * @note The header is added only by the overload of sendHeaders with the container
				 *       headers_t. The overload with a list of the fields issues them as they are -
				 *       this is its contract
				 * @param sid         identifier of the stream
				 * @param urgency     urgency of the stream (0 - the highest, 7 - the least)
				 * @param incremental flag of the incremental delivery
				 *
				 * \~
				 */
				void priority(const uint32_t sid, const uint8_t urgency, const bool incremental) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки анонса альтернативного сервиса (RFC 7838 §4)
				 *
				 * @details Кадр отправляет только сервер. Нулевой идентификатор потока
				 *          означает анонс для соединения и требует непустого origin;
				 *          анонс для потока, наоборот, требует пустого - origin там
				 *          определяется самим потоком
				 *
				 * @param sid    идентификатор потока, либо 0 для соединения
				 * @param origin origin анонсируемого сервиса
				 * @param value  значение поля Alt-Svc (RFC 7838 §3)
				 *
				 * \~english
				 * @brief Method of the sending of an announcement of an alternative service (RFC 7838 §4)
				 * @details The frame is sent only by a server. A zero identifier of the stream
				 *          means an announcement for the connection and requires a non-empty origin;
				 *          an announcement for a stream, on the contrary, requires an empty one - the origin there
				 *          is determined by the stream itself
				 * @param sid    identifier of the stream, or 0 for the connection
				 * @param origin origin of the service being announced
				 * @param value  value of the field Alt-Svc (RFC 7838 §3)
				 *
				 * \~
				 */
				void sendAltSvc(const uint32_t sid, const string & origin, const string & value) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки набора origin, обслуживаемых соединением (RFC 8336 §2)
				 *
				 * @details Кадр отправляет только сервер и только для соединения целиком.
				 *          Пустой набор законен и означает, что соединение не обслуживает
				 *          ни одного origin сверх того, для которого установлено
				 *
				 * @param origins набор origin
				 *
				 * \~english
				 * @brief Method of the sending of the collection of the origins served by the connection (RFC 8336 §2)
				 * @details The frame is sent only by a server and only for the connection as a whole.
				 *          An empty collection is lawful and means that the connection serves
				 *          not a single origin above that for which it is established
				 * @param origins collection of the origins
				 *
				 * \~
				 */
				void sendOrigin(const vector <string> & origins) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки WINDOW_UPDATE
				 *
				 * @param sid       идентификатор потока (0 - окно всего соединения)
				 * @param increment инкремент окна flow control
				 *
				 * \~english
				 * @brief Method of the sending of a WINDOW_UPDATE
				 * @param sid       identifier of the stream (0 - the window of the whole connection)
				 * @param increment increment of the window of the flow control
				 *
				 * \~
				 */
				void sendWindowUpdate(const uint32_t sid, const uint32_t increment) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод передачи части тела потока для отправки (push-модель, bounded buffer)
				 *
				 * @details Копирует во внутренний буфер потока столько байт, сколько влезает до
				 *          high-water, и возвращает это число (0..size). Если вернулось меньше
				 *          size - буфер заполнен: приостановите выдачу и дождитесь функции
				 *          обратного вызова writable. Нарезку во фреймы, учёт окон и
				 *          автоматическую досылку по WINDOW_UPDATE парсер делает сам.
				 *
				 * @param sid       идентификатор потока
				 * @param buffer    буфер данных тела
				 * @param size      размер данных тела
				 * @param endStream флаг завершения потока
				 * @return          число принятых байт (0..size)
				 *
				 * \~english
				 * @brief Method of the transmission of a part of the body of a stream for the sending (the push model, a bounded buffer)
				 * @details It copies into the internal buffer of the stream as many octets as fit up to the
				 *          high-water, and returns this number (0..size). If less than
				 *          size has been returned - the buffer is filled: suspend the issue and wait for the callback
				 *          function writable. The cutting into the frames, the account of the windows and
				 *          the automatic further sending by a WINDOW_UPDATE the parser does itself.
				 * @param sid       identifier of the stream
				 * @param buffer    buffer of the data of the body
				 * @param size      size of the data of the body
				 * @param endStream flag of the completion of the stream
				 * @return          number of the accepted octets (0..size)
				 *
				 * \~
				 */
				size_t sendData(const uint32_t sid, const void * buffer, const size_t size, const bool endStream) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод анонса server push (только сервер)
				 *
				 * @details Отправляет PUSH_PROMISE на потоке клиента и резервирует чётный
				 *          push-поток. Дальше ответ отправляется обычным путём:
				 *          sendHeaders(promisedSid, ...) + sendData(promisedSid, ...).
				 *
				 * @param sid    идентификатор потока клиента, в ответ на который выполняется push
				 * @param fields заголовки обещанного запроса (псевдо-заголовки как у запроса клиента)
				 * @return       идентификатор зарезервированного push-потока либо 0, если push невозможен
				 *
				 * \~english
				 * @brief Method of the announcement of a server push (only a server)
				 * @details It sends a PUSH_PROMISE on the stream of the client and reserves an even
				 *          push stream. Further the answer is sent by the usual way:
				 *          sendHeaders(promisedSid, ...) + sendData(promisedSid, ...).
				 * @param sid    identifier of the stream of the client in an answer to which the push is performed
				 * @param fields headers of the promised request (the pseudo headers as at a request of a client)
				 * @return       identifier of the reserved push stream or 0, if the push is impossible
				 *
				 * \~
				 */
				uint32_t sendPushPromise(const uint32_t sid, const vector <h2::hpack::field_t> & fields) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод отправки блока заголовков (запрос/ответ/трейлеры) потока
				 *
				 * @details Если поток ещё не существует и мы инициатор - поток открывается.
				 *          При endStream поток сразу полузакрывается с нашей стороны (тела не будет).
				 *          Блок, превышающий SETTINGS_MAX_FRAME_SIZE пира, автоматически режется
				 *          на HEADERS + CONTINUATION (RFC 9113 §6.2/§6.10).
				 *
				 * @param sid       идентификатор потока
				 * @param fields    заголовки (псевдо-заголовки :method/:path/... должны идти первыми)
				 * @param endStream флаг завершения потока (тела не будет)
				 *
				 * \~english
				 * @brief Method of the sending of a block of the headers (a request/an answer/the trailers) of a stream
				 * @details If the stream does not exist yet and we are the initiator - the stream is opened.
				 *          At an endStream the stream is at once half-closed from our side (there will be no body).
				 *          A block exceeding the SETTINGS_MAX_FRAME_SIZE of the peer is automatically cut
				 *          into a HEADERS + a CONTINUATION (RFC 9113 §6.2/§6.10).
				 * @param sid       identifier of the stream
				 * @param fields    headers (the pseudo headers :method/:path/... are obliged to go first)
				 * @param endStream flag of the completion of the stream (there will be no body)
				 *
				 * \~
				 */
				void sendHeaders(const uint32_t sid, const vector <h2::hpack::field_t> & fields, const bool endStream) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки блока заголовков потока из контейнера заголовков (zero-copy)
				 *
				 * @details Заголовки кодируются в HPACK напрямую из контейнера, без промежуточных
				 *          копий. Псевдо-заголовки формируются автоматически из провайдера контейнера:
				 *          для запроса (request_t) - [:method]/[:scheme]/[:authority]/[:path]
				 *          (для метода CONNECT - только [:method]/[:authority], RFC 9113 §8.5),
				 *          для ответа (response_t) - [:status]. Заголовок Host конвертируется
				 *          в [:authority]. Если провайдер контейнера не установлен - блок кодируется
				 *          без псевдо-заголовков (трейлеры). Названия заголовков приводятся к нижнему
				 *          регистру (RFC 9113 §8.2.1), запрещённые в HTTP/2 connection-specific
				 *          заголовки (Connection/Keep-Alive/Proxy-Connection/Transfer-Encoding/Upgrade,
				 *          а также TE со значением кроме "trailers") пропускаются (RFC 9113 §8.2.2).
				 *
				 * @param sid       идентификатор потока
				 * @param headers   контейнер заголовков (провайдер контейнера задаёт псевдо-заголовки)
				 * @param endStream флаг завершения потока (тела не будет)
				 * @param scheme    схема запроса для псевдо-заголовка [:scheme] (для ответа не используется)
				 *
				 * \~english
				 * @brief Method of the sending of a block of the headers of a stream out of a container of the headers (zero-copy)
				 * @details The headers are encoded into HPACK directly from the container, without the intermediate
				 *          copies. The pseudo headers are formed automatically from the provider of the container:
				 *          for a request (request_t) - [:method]/[:scheme]/[:authority]/[:path]
				 *          (for the method CONNECT - only [:method]/[:authority], RFC 9113 §8.5),
				 *          for an answer (response_t) - [:status]. The header Host is converted
				 *          into [:authority]. If the provider of the container is not set - the block is encoded
				 *          without the pseudo headers (the trailers). The names of the headers are brought to the lower
				 *          case (RFC 9113 §8.2.1), the connection-specific headers prohibited in HTTP/2
				 *          (Connection/Keep-Alive/Proxy-Connection/Transfer-Encoding/Upgrade,
				 *          and also a TE with a value other than "trailers") are skipped (RFC 9113 §8.2.2).
				 * @param sid       identifier of the stream
				 * @param headers   container of the headers (the provider of the container sets the pseudo headers)
				 * @param endStream flag of the completion of the stream (there will be no body)
				 * @param scheme    scheme of the request for the pseudo header [:scheme] (for an answer it is not used)
				 *
				 * \~
				 */
				void sendHeaders(const uint32_t sid, const headers_t & headers, const bool endStream, string_view scheme = "https") noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод выделения идентификатора для нового инициируемого нами потока
				 *
				 * @details Клиент получает нечётные идентификаторы (1, 3, 5...), выделенный
				 *          идентификатор передаётся в sendHeaders() для открытия потока.
				 *
				 * @return идентификатор нового потока
				 *
				 * \~english
				 * @brief Method of the allotment of an identifier for a new stream initiated by us
				 * @details A client gets the odd identifiers (1, 3, 5...), the allotted
				 *          identifier is passed into sendHeaders() for the opening of the stream.
				 * @return identifier of the new stream
				 *
				 * \~
				 */
				uint32_t nextStreamId() noexcept;
				/**
				 * \~russian
				 * @brief Метод назначения pull-источника данных тела потока
				 *
				 * @details Источник пишет данные напрямую в буфер отправки потока и возвращает
				 *          число записанных байт (отрицательное значение сбрасывает поток).
				 *          Закрывать поток прямо из источника допустимо, но переданный ему
				 *          буфер после этого уничтожен - записывать в него уже нельзя.
				 *
				 * @param sid    идентификатор потока
				 * @param source pull-источник данных тела
				 *
				 * \~english
				 * @brief Method of the assignment of the pull source of the data of the body of a stream
				 * @details The source writes the data directly into the buffer of the sending of the stream and returns
				 *          the number of the written octets (a negative value resets the stream).
				 *          To close the stream right from the source is admissible, but the buffer passed to it
				 *          is destroyed after this - to write into it is no longer possible.
				 * @param sid    identifier of the stream
				 * @param source pull source of the data of the body
				 *
				 * \~
				 */
				void dataSource(const uint32_t sid, data_source_callback_t source) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод настройки порога выходного буфера соединения (backpressure от TCP-стадии)
				 *
				 * @param high порог выходного буфера соединения
				 *
				 * \~english
				 * @brief Method of the configuration of the threshold of the output buffer of the connection (a backpressure from the TCP stage)
				 * @param high threshold of the output buffer of the connection
				 *
				 * \~
				 */
				void outputHighWater(const size_t high) noexcept;
				/**
				 * \~russian
				 * @brief Метод настройки порогов буфера отправки потока
				 *
				 * @param high ёмкость буфера отправки потока (high-water)
				 * @param low  порог сигнала writable (low-water)
				 *
				 * \~english
				 * @brief Method of the configuration of the thresholds of the buffer of the sending of a stream
				 * @param high capacity of the buffer of the sending of the stream (high-water)
				 * @param low  threshold of the signal writable (low-water)
				 *
				 * \~
				 */
				void sendWaterMarks(const size_t high, const size_t low) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод сообщения текущего монотонного времени для пополнения rate-лимитов
				 *
				 * @details Вызывайте периодически (например, перед parse); необязательно -
				 *          без обновления времени работает только стартовый запас burst.
				 *
				 * @param seconds текущее монотонное время (секунды)
				 *
				 * \~english
				 * @brief Method of the reporting of the current monotonic time for the replenishment of the rate limits
				 * @details Call it periodically (for example, before parse); it is optional -
				 *          without an updating of the time only the starting reserve burst works.
				 * @param seconds current monotonic time (seconds)
				 *
				 * \~
				 */
				void updateTime(const uint64_t seconds) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод увеличения приёмного окна соединения
				 *
				 * @details По умолчанию окно соединения 65535 байт - узкое место при высокой
				 *          пропускной способности. Поднимает целевой размер окна приёма и сразу
				 *          отправляет WINDOW_UPDATE(0) на разницу. Только увеличение.
				 *
				 * @param size новый целевой размер окна приёма соединения
				 *
				 * \~english
				 * @brief Method of the increase of the receiving window of the connection
				 * @details By default the window of the connection is 65535 octets - a bottleneck at a high
				 *          throughput. It raises the target size of the window of the acceptance and at once
				 *          sends a WINDOW_UPDATE(0) for the difference. Only an increase.
				 * @param size new target size of the window of the acceptance of the connection
				 *
				 * \~
				 */
				void connectionReceiveWindow(const int32_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения ещё не отправленных исходящих байтов (pull-модель)
				 *
				 * @details View действителен до следующего вызова любого метода парсера.
				 *          После записи в сокет освободите отправленную часть методом
				 *          consumePending(). При установленной функции обратного вызова
				 *          записи буфер опустошается автоматически.
				 *
				 * @return ещё не отправленные исходящие байты (zero-copy view во внутренний буфер)
				 *
				 * \~english
				 * @brief Method of getting the not yet sent outgoing octets (the pull model)
				 * @details The view is valid until the next call of any method of the parser.
				 *          After the writing into the socket free the sent part by the method
				 *          consumePending(). At a set callback function
				 *          of the writing the buffer is emptied automatically.
				 * @return not yet sent outgoing octets (a zero-copy view into the internal buffer)
				 *
				 * \~
				 */
				string_view pending() const noexcept;
				/**
				 * \~russian
				 * @brief Метод освобождения отправленных байтов из исходящего буфера (амортизированно O(1))
				 *
				 * @param size число отправленных байт
				 *
				 * \~english
				 * @brief Method of the release of the sent octets from the outgoing buffer (amortized O(1))
				 * @param size number of the sent octets
				 *
				 * \~
				 */
				void consumePending(const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова для обработки анонса server push
				 *
				 * @param callback функция обратного вызова для обработки анонса server push
				 *
				 * \~english
				 * @brief Method of setting the callback function for the processing of an announcement of a server push
				 * @param callback callback function for the processing of an announcement of a server push
				 *
				 * \~
				 */
				void on(push_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова для обработки фрагмента тела потока
				 *
				 * @param callback функция обратного вызова для обработки фрагмента тела потока
				 *
				 * \~english
				 * @brief Method of setting the callback function for the processing of a fragment of the body of a stream
				 * @param callback callback function for the processing of a fragment of the body of a stream
				 *
				 * \~
				 */
				void on(data_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова для обработки закрытия потока
				 *
				 * @param callback функция обратного вызова для обработки закрытия потока
				 *
				 * \~english
				 * @brief Method of setting the callback function for the processing of the closing of a stream
				 * @param callback callback function for the processing of the closing of a stream
				 *
				 * \~
				 */
				void on(close_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова для обработки ошибки уровня соединения
				 *
				 * @param callback функция обратного вызова для обработки ошибки уровня соединения
				 *
				 * \~english
				 * @brief Method of setting the callback function for the processing of an error of the level of the connection
				 * @param callback callback function for the processing of an error of the level of the connection
				 *
				 * \~
				 */
				void on(error_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова записи исходящих байтов в сеть
				 *
				 * @param callback функция обратного вызова записи исходящих байтов в сеть
				 *
				 * \~english
				 * @brief Method of setting the callback function of the writing of the outgoing octets into the network
				 * @param callback callback function of the writing of the outgoing octets into the network
				 *
				 * \~
				 */
				void on(write_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова для обработки открытия нового потока
				 *
				 * @param callback функция обратного вызова для обработки открытия нового потока
				 *
				 * \~english
				 * @brief Method of setting the callback function for the processing of the opening of a new stream
				 * @param callback callback function for the processing of the opening of a new stream
				 *
				 * \~
				 */
				void on(begin_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова для обработки фазы приёма сообщения потока
				 *
				 * @param callback функция обратного вызова для обработки фазы приёма сообщения потока
				 *
				 * \~english
				 * @brief Method of setting the callback function for the processing of the phase of the acceptance of a message of a stream
				 * @param callback callback function for the processing of the phase of the acceptance of a message of a stream
				 *
				 * \~
				 */
				void on(phase_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова для обработки полученного GOAWAY
				 *
				 * @param callback функция обратного вызова для обработки полученного GOAWAY
				 *
				 * \~english
				 * @brief Method of setting the callback function for the processing of an obtained GOAWAY
				 * @param callback callback function for the processing of an obtained GOAWAY
				 *
				 * \~
				 */
				void on(goaway_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова для обработки полученного ALTSVC
				 *
				 * @param callback функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback function for the processing of an obtained ALTSVC
				 * @param callback callback function
				 *
				 * \~
				 */
				void on(altsvc_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова для обработки полученного ORIGIN
				 *
				 * @param callback функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback function for the processing of an obtained ORIGIN
				 * @param callback callback function
				 *
				 * \~
				 */
				void on(origin_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова для обработки заголовков или трейлеров потока
				 *
				 * @param callback функция обратного вызова для обработки заголовков или трейлеров потока
				 *
				 * \~english
				 * @brief Method of setting the callback function for the processing of the headers or of the trailers of a stream
				 * @param callback callback function for the processing of the headers or of the trailers of a stream
				 *
				 * \~
				 */
				void on(header_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова о готовности потока принимать данные тела
				 *
				 * @param callback функция обратного вызова о готовности потока принимать данные тела
				 *
				 * \~english
				 * @brief Method of setting the callback function about the readiness of a stream to accept the data of the body
				 * @param callback callback function about the readiness of a stream to accept the data of the body
				 *
				 * \~
				 */
				void on(writable_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова для обработки применённого SETTINGS пира
				 *
				 * @param callback функция обратного вызова для обработки применённого SETTINGS пира
				 *
				 * \~english
				 * @brief Method of setting the callback function for the processing of an applied SETTINGS of the peer
				 * @param callback callback function for the processing of an applied SETTINGS of the peer
				 *
				 * \~
				 */
				void on(settings_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова для обработки провайдера заголовков потока
				 *
				 * @param callback функция обратного вызова для обработки провайдера заголовков потока
				 *
				 * \~english
				 * @brief Method of setting the callback function for the processing of the provider of the headers of a stream
				 * @param callback callback function for the processing of the provider of the headers of a stream
				 *
				 * \~
				 */
				void on(provider_callback_t callback) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param direct направление трафика (REQUEST - мы сервер, RESPONSE - мы клиент)
				 * @param fmk    объект фреймворка
				 * @param log    объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param direct direction of the traffic (REQUEST - we are a server, RESPONSE - we are a client)
				 * @param fmk    object of the framework
				 * @param log    object for the work with the logs
				 *
				 * \~
				 */
				explicit Parser_HTTP2(const direct_t direct, const fmk_t * fmk, const log_t * log) noexcept;
			public:
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
				~Parser_HTTP2() noexcept override;
		} parser_http2_t;
	};
};

#endif // __AWH_HTTP_PARSER_HTTP2__
