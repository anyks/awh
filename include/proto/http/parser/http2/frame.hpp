/**
 * @file: frame.hpp
 * @date: 2026-07-19
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл слоя фреймов HTTP/2 (RFC 9113) — структуры полезной нагрузки DATA, HEADERS, PRIORITY,
 *        SETTINGS, GOAWAY и PUSH_PROMISE,
 *        а также чистые функции разбора и сборки фреймов без хранения состояния соединения
 *
 * \~english
 * @brief Header file of the layer of the frames of HTTP/2 (RFC 9113) — the structures of the payload of DATA, HEADERS, PRIORITY,
 *        SETTINGS, GOAWAY and PUSH_PROMISE,
 *        and also the pure functions of the parsing and of the assembly of the frames without a storing of the state of the connection
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_HTTP_PARSER_HTTP2_FRAME__
#define __AWH_HTTP_PARSER_HTTP2_FRAME__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "h2.hpp"
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
		 * @brief Пространство имён внутренних слоёв протокола HTTP/2
		 *
		 * \~english
		 * @brief Namespace of the internal layers of the HTTP/2 protocol
		 *
		 * \~
		 */
		namespace h2 {
			/**
			 * \~russian
			 * @brief Пространство имён framing-слоя HTTP/2 (RFC 9113 §4-6): разбор и сборка фреймов
			 *
			 * @details Разбор zero-copy: полезная нагрузка отдаётся как string_view с указателем
			 *          во входной буфер. Сборка дописывает байты в string (выходной буфер соединения).
			 *          Слой не хранит состояния соединения - это чистые функции над байтами. Логика
			 *          состояний потоков, flow control и HPACK живёт в http.hpp / hpack.hpp.
			 *
			 * \~english
			 * @brief Namespace of the framing layer of HTTP/2 (RFC 9113 §4-6): the parsing and the assembly of the frames
			 * @details The parsing is zero-copy: the payload is issued as a string_view with a pointer
			 *          into the input buffer. The assembly appends the octets into a string (the output buffer of the connection).
			 *          The layer stores no state of the connection - these are pure functions over the octets. The logic
			 *          of the states of the streams, of the flow control and of HPACK lives in http.hpp / hpack.hpp.
			 *
			 * \~
			 */
			namespace frame {
				/**
				 * \~russian
				 * @brief Структура параметра SETTINGS (идентификатор + значение)
				 *
				 * \~english
				 * @brief Structure of a parameter of SETTINGS (an identifier + a value)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Setting {
					// Идентификатор параметра
					setting_t id;
					// Значение параметра
					uint32_t value;
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
					explicit Setting() noexcept;
				} setting_entry_t;
				/**
				 * \~russian
				 * @brief Структура полезной нагрузки DATA (RFC 9113 §6.1), padding уже снят
				 *
				 * \~english
				 * @brief Structure of the payload of DATA (RFC 9113 §6.1), the padding is already removed
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Data {
					// Флаг завершения потока
					bool endStream;
					// Данные тела (zero-copy во входной буфер)
					string_view data;
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
					explicit Data() noexcept;
				} data_t;
				/**
				 * \~russian
				 * @brief Структура полезной нагрузки PRIORITY (RFC 9113 §6.3)
				 *
				 * \~english
				 * @brief Structure of the payload of PRIORITY (RFC 9113 §6.3)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Priority {
					// Флаг эксклюзивной зависимости потока
					bool exclusive;
					// Вес потока
					uint8_t weight;
					// Идентификатор потока, от которого зависит текущий
					uint32_t streamDep;
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
				/**
				 * \~russian
				 * @brief Структура полезной нагрузки GOAWAY (RFC 9113 §6.8)
				 *
				 * \~english
				 * @brief Structure of the payload of GOAWAY (RFC 9113 §6.8)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Goaway {
					// Код ошибки завершения соединения
					error_t code;
					// Наибольший идентификатор обработанного потока
					uint32_t lastStreamId;
					// Необязательные отладочные данные (zero-copy во входной буфер)
					string_view debugData;
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
					explicit Goaway() noexcept;
				} goaway_t;
				/**
				 * \~russian
				 * @brief Структура разобранного заголовка фрейма (RFC 9113 §4.1)
				 *
				 * \~english
				 * @brief Structure of a parsed header of a frame (RFC 9113 §4.1)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Header {
					// Тип фрейма
					frame_t type;
					// Флаги фрейма (семантика зависит от типа)
					uint8_t flags;
					// Длина полезной нагрузки (24 бита)
					uint32_t length;
					// Идентификатор потока (31 бит)
					uint32_t streamId;
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
					explicit Header() noexcept;
				} header_t;
				/**
				 * \~russian
				 * @brief Структура полезной нагрузки HEADERS (RFC 9113 §6.2), padding уже снят
				 *
				 * \~english
				 * @brief Structure of the payload of HEADERS (RFC 9113 §6.2), the padding is already removed
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Headers {
					// Флаг эксклюзивной зависимости потока
					bool exclusive;
					// Флаг завершения потока
					bool endStream;
					// Флаг завершения блока заголовков
					bool endHeaders;
					// Флаг наличия полей приоритета (RFC 7540, deprecated)
					bool hasPriority;
					// Вес потока (фактический вес = weight + 1)
					uint8_t weight;
					// Идентификатор потока, от которого зависит текущий
					uint32_t streamDep;
					// Фрагмент блока заголовков HPACK (zero-copy во входной буфер)
					string_view block;
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
					explicit Headers() noexcept;
				} headers_t;
				/**
				 * \~russian
				 * @brief Структура полезной нагрузки PUSH_PROMISE (RFC 9113 §6.6), padding уже снят
				 *
				 * \~english
				 * @brief Structure of the payload of PUSH_PROMISE (RFC 9113 §6.6), the padding is already removed
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Push_Promise {
					// Флаг завершения блока заголовков
					bool endHeaders;
					// Идентификатор обещанного потока
					uint32_t promisedStreamId;
					// Фрагмент блока заголовков HPACK (zero-copy во входной буфер)
					string_view block;
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
					explicit Push_Promise() noexcept;
				} push_promise_t;

				/**
				 * \~russian
				 * @brief Пространство имён функций разбора полезной нагрузки фреймов HTTP/2 (RFC 9113 §4-6)
				 *
				 * \~english
				 * @brief Namespace of the functions of the parsing of the payload of the frames of HTTP/2 (RFC 9113 §4-6)
				 *
				 * \~
				 */
				namespace parser {
					/**
					 * \~russian
					 * @brief Функция разбора 9-байтового заголовка фрейма
					 *
					 * @param data   входной буфер
					 * @param size   доступно байт
					 * @param output разобранный заголовок фрейма
					 * @return       результат разбора (true - в буфере было достаточно байт и заголовок разобран)
					 *
					 * \~english
					 * @brief Function of parsing the 9-octet header of a frame
					 * @param data   input buffer
					 * @param size   octets available
					 * @param output parsed header of the frame
					 * @return       result of the parsing (true - there were enough octets in the buffer and the header is parsed)
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ bool header(const uint8_t * data, const size_t size, header_t & output) noexcept;
					/**
					 * \~russian
					 * @brief Функция разбора полезной нагрузки DATA
					 *
					 * @note Параметр payload должен указывать на ровно h.length байт нагрузки
					 *
					 * @param header  заголовок фрейма
					 * @param payload полезная нагрузка фрейма
					 * @param output  разобранная полезная нагрузка
					 * @param error   код ошибки протокола (PROTOCOL_ERROR на некорректном padding)
					 * @return        результат разбора (OK/ERROR)
					 *
					 * \~english
					 * @brief Function of parsing the payload of DATA
					 * @note The parameter payload is obliged to point to exactly h.length octets of the payload
					 * @param header  header of the frame
					 * @param payload payload of the frame
					 * @param output  parsed payload
					 * @param error   error code of the protocol (PROTOCOL_ERROR at an incorrect padding)
					 * @return        result of the parsing (OK/ERROR)
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ status_t data(const header_t & header, const uint8_t * payload, data_t & output, error_t & error) noexcept;
					/**
					 * \~russian
					 * @brief Функция разбора полезной нагрузки PING (требует ровно 8 байт opaque-данных)
					 *
					 * @param header  заголовок фрейма
					 * @param payload полезная нагрузка фрейма
					 * @param opaque  извлечённые opaque-данные
					 * @param error   код ошибки протокола
					 * @return        результат разбора (OK/ERROR)
					 *
					 * \~english
					 * @brief Function of parsing the payload of PING (requires exactly 8 octets of the opaque data)
					 * @param header  header of the frame
					 * @param payload payload of the frame
					 * @param opaque  extracted opaque data
					 * @param error   error code of the protocol
					 * @return        result of the parsing (OK/ERROR)
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ status_t ping(const header_t & header, const uint8_t * payload, uint8_t opaque[8], error_t & error) noexcept;
					/**
					 * \~russian
					 * @brief Функция разбора полезной нагрузки RST_STREAM (требует ровно 4 байта)
					 *
					 * @param header  заголовок фрейма
					 * @param payload полезная нагрузка фрейма
					 * @param code    код ошибки, с которым сброшен поток
					 * @param error   код ошибки протокола
					 * @return        результат разбора (OK/ERROR)
					 *
					 * \~english
					 * @brief Function of parsing the payload of RST_STREAM (requires exactly 4 octets)
					 * @param header  header of the frame
					 * @param payload payload of the frame
					 * @param code    error code with which the stream is reset
					 * @param error   error code of the protocol
					 * @return        result of the parsing (OK/ERROR)
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ status_t rstStream(const header_t & header, const uint8_t * payload, error_t & code, error_t & error) noexcept;
					/**
					 * \~russian
					 * @brief Функция разбора полезной нагрузки GOAWAY (минимум 8 байт)
					 *
					 * @param header  заголовок фрейма
					 * @param payload полезная нагрузка фрейма
					 * @param output  разобранная полезная нагрузка
					 * @param error   код ошибки протокола
					 * @return        результат разбора (OK/ERROR)
					 *
					 * \~english
					 * @brief Function of parsing the payload of GOAWAY (a minimum of 8 octets)
					 * @param header  header of the frame
					 * @param payload payload of the frame
					 * @param output  parsed payload
					 * @param error   error code of the protocol
					 * @return        result of the parsing (OK/ERROR)
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ status_t goaway(const header_t & header, const uint8_t * payload, goaway_t & output, error_t & error) noexcept;
					/**
					 * \~russian
					 * @brief Функция разбора полезной нагрузки HEADERS (с учётом padding и приоритета)
					 *
					 * @param header  заголовок фрейма
					 * @param payload полезная нагрузка фрейма
					 * @param output  разобранная полезная нагрузка
					 * @param error   код ошибки протокола
					 * @return        результат разбора (OK/ERROR)
					 *
					 * \~english
					 * @brief Function of parsing the payload of HEADERS (with the account of the padding and of the priority)
					 * @param header  header of the frame
					 * @param payload payload of the frame
					 * @param output  parsed payload
					 * @param error   error code of the protocol
					 * @return        result of the parsing (OK/ERROR)
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ status_t headers(const header_t & header, const uint8_t * payload, headers_t & output, error_t & error) noexcept;
					/**
					 * \~russian
					 * @brief Функция разбора полезной нагрузки PRIORITY (требует ровно 5 байт)
					 *
					 * @param header  заголовок фрейма
					 * @param payload полезная нагрузка фрейма
					 * @param output  разобранная полезная нагрузка
					 * @param error   код ошибки протокола
					 * @return        результат разбора (OK/ERROR)
					 *
					 * \~english
					 * @brief Function of parsing the payload of PRIORITY (requires exactly 5 octets)
					 * @param header  header of the frame
					 * @param payload payload of the frame
					 * @param output  parsed payload
					 * @param error   error code of the protocol
					 * @return        result of the parsing (OK/ERROR)
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ status_t priority(const header_t & header, const uint8_t * payload, priority_t & output, error_t & error) noexcept;
					/**
					 * \~russian
					 * @brief Функция разбора полезной нагрузки WINDOW_UPDATE (требует ровно 4 байта)
					 *
					 * @note Нулевой инкремент - PROTOCOL_ERROR (на уровне потока - RST_STREAM)
					 *
					 * @param header    заголовок фрейма
					 * @param payload   полезная нагрузка фрейма
					 * @param increment извлечённый инкремент окна
					 * @param error     код ошибки протокола
					 * @return          результат разбора (OK/ERROR)
					 *
					 * \~english
					 * @brief Function of parsing the payload of WINDOW_UPDATE (requires exactly 4 octets)
					 * @note A zero increment is a PROTOCOL_ERROR (at the level of a stream - RST_STREAM)
					 * @param header    header of the frame
					 * @param payload   payload of the frame
					 * @param increment extracted increment of the window
					 * @param error     error code of the protocol
					 * @return          result of the parsing (OK/ERROR)
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ status_t windowUpdate(const header_t & header, const uint8_t * payload, uint32_t & increment, error_t & error) noexcept;
					/**
					 * \~russian
					 * @brief Функция разбора полезной нагрузки PUSH_PROMISE (с учётом padding)
					 *
					 * @param header  заголовок фрейма
					 * @param payload полезная нагрузка фрейма
					 * @param output  разобранная полезная нагрузка
					 * @param error   код ошибки протокола
					 * @return        результат разбора (OK/ERROR)
					 *
					 * \~english
					 * @brief Function of parsing the payload of PUSH_PROMISE (with the account of the padding)
					 * @param header  header of the frame
					 * @param payload payload of the frame
					 * @param output  parsed payload
					 * @param error   error code of the protocol
					 * @return        result of the parsing (OK/ERROR)
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ status_t pushPromise(const header_t & header, const uint8_t * payload, push_promise_t & output, error_t & error) noexcept;
					/**
					 * \~russian
					 * @brief Функция разбора полезной нагрузки SETTINGS (длина кратна 6)
					 *
					 * @note Для ACK-фрейма нагрузка должна быть пустой. Параметры дописываются в output
					 *
					 * @param header  заголовок фрейма
					 * @param payload полезная нагрузка фрейма
					 * @param output  список разобранных параметров
					 * @param error   код ошибки протокола
					 * @return        результат разбора (OK/ERROR)
					 *
					 * \~english
					 * @brief Function of parsing the payload of SETTINGS (the length is a multiple of 6)
					 * @note For an ACK frame the payload is obliged to be empty. The parameters are appended into output
					 * @param header  header of the frame
					 * @param payload payload of the frame
					 * @param output  list of the parsed parameters
					 * @param error   error code of the protocol
					 * @return        result of the parsing (OK/ERROR)
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ status_t settings(const header_t & header, const uint8_t * payload, vector <setting_entry_t> & output, error_t & error) noexcept;
					/**
					 * \~russian
					 * @brief Функция разбора полезной нагрузки CONTINUATION (фрагмент блока заголовков)
					 *
					 * @param header     заголовок фрейма
					 * @param payload    полезная нагрузка фрейма
					 * @param block      фрагмент блока заголовков (zero-copy во входной буфер)
					 * @param endHeaders флаг завершения блока заголовков
					 * @param err        код ошибки протокола
					 * @return           результат разбора (OK/ERROR)
					 *
					 * \~english
					 * @brief Function of parsing the payload of CONTINUATION (a fragment of a block of the headers)
					 * @param header     header of the frame
					 * @param payload    payload of the frame
					 * @param block      fragment of the block of the headers (zero-copy into the input buffer)
					 * @param endHeaders flag of the completion of the block of the headers
					 * @param err        error code of the protocol
					 * @return           result of the parsing (OK/ERROR)
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ status_t continuation(const header_t & header, const uint8_t * payload, string_view & block, bool & endHeaders, error_t & err) noexcept;
					/**
					 * \~russian
					 * @brief Функция разбора полезной нагрузки PRIORITY_UPDATE (RFC 9218 §7.1)
					 *
					 * @note Кадр относится к соединению (stream id == 0), а приоритизируемый
					 *       поток указан в первых 4 октетах нагрузки
					 *
					 * @param header   заголовок фрейма
					 * @param payload  полезная нагрузка фрейма
					 * @param streamId идентификатор приоритизируемого потока
					 * @param value    значение поля приоритета (zero-copy во входной буфер)
					 * @param error    код ошибки протокола
					 * @return         результат разбора (OK/ERROR)
					 *
					 * \~english
					 * @brief Function of parsing the payload of PRIORITY_UPDATE (RFC 9218 §7.1)
					 * @note The frame relates to the connection (stream id == 0), while the stream being prioritized
					 *       is indicated in the first 4 octets of the payload
					 * @param header   header of the frame
					 * @param payload  payload of the frame
					 * @param streamId identifier of the stream being prioritized
					 * @param value    value of the field of the priority (zero-copy into the input buffer)
					 * @param error    error code of the protocol
					 * @return         result of the parsing (OK/ERROR)
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ status_t priorityUpdate(const header_t & header, const uint8_t * payload, uint32_t & streamId, string_view & value, error_t & error) noexcept;
					/**
					 * \~russian
					 * @brief Функция разбора полезной нагрузки ALTSVC (RFC 7838 §4)
					 *
					 * @note Ошибки этого кадра не рвут ни поток, ни соединение: RFC предписывает
					 *       некорректный ALTSVC игнорировать. Отрицательный результат означает
					 *       именно «кадр подлежит игнорированию», а не ошибку протокола
					 *
					 * @param header  заголовок фрейма
					 * @param payload полезная нагрузка фрейма
					 * @param origin  origin анонсируемого сервиса (zero-copy во входной буфер)
					 * @param value   значение поля Alt-Svc (zero-copy во входной буфер)
					 * @return        признак пригодности кадра к обработке
					 *
					 * \~english
					 * @brief Function of parsing the payload of ALTSVC (RFC 7838 §4)
					 * @note The errors of this frame break neither the stream nor the connection: the RFC prescribes
					 *       to ignore an incorrect ALTSVC. A negative result means
					 *       exactly «the frame is subject to being ignored» rather than an error of the protocol
					 * @param header  header of the frame
					 * @param payload payload of the frame
					 * @param origin  origin of the service being announced (zero-copy into the input buffer)
					 * @param value   value of the field Alt-Svc (zero-copy into the input buffer)
					 * @return        flag of the suitability of the frame for a processing
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ bool altsvc(const header_t & header, const uint8_t * payload, string_view & origin, string_view & value) noexcept;
					/**
					 * \~russian
					 * @brief Функция разбора полезной нагрузки ORIGIN (RFC 8336 §2)
					 *
					 * @note Ошибки этого кадра тоже не рвут соединение: RFC предписывает
					 *       некорректный ORIGIN игнорировать целиком
					 *
					 * @param header  заголовок фрейма
					 * @param payload полезная нагрузка фрейма
					 * @param origins набор origin (zero-copy во входной буфер)
					 * @return        признак пригодности кадра к обработке
					 *
					 * \~english
					 * @brief Function of parsing the payload of ORIGIN (RFC 8336 §2)
					 * @note The errors of this frame likewise do not break the connection: the RFC prescribes
					 *       to ignore an incorrect ORIGIN entirely
					 * @param header  header of the frame
					 * @param payload payload of the frame
					 * @param origins collection of the origins (zero-copy into the input buffer)
					 * @return        flag of the suitability of the frame for a processing
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ bool origin(const header_t & header, const uint8_t * payload, vector <string_view> & origins) noexcept;
				};

				/**
				 * \~russian
				 * @brief Пространство имён функций сборки полезной нагрузки фреймов HTTP/2 (RFC 9113 §4-6)
				 *
				 * \~english
				 * @brief Namespace of the functions of the assembly of the payload of the frames of HTTP/2 (RFC 9113 §4-6)
				 *
				 * \~
				 */
				namespace serialize {
					/**
					 * \~russian
					 * @brief Функция сборки фрейма PING (заголовок + нагрузка дописываются в output)
					 *
					 * @param output выходной буфер соединения
					 * @param opaque произвольные opaque-данные (8 байт)
					 * @param ack    флаг подтверждения получения PING пира
					 *
					 * \~english
					 * @brief Function of assembling a PING frame (the header + the payload are appended into output)
					 * @param output output buffer of the connection
					 * @param opaque arbitrary opaque data (8 octets)
					 * @param ack    flag of the confirmation of the receipt of a PING of the peer
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ void ping(string & output, const uint8_t opaque[8], const bool ack) noexcept;
					/**
					 * \~russian
					 * @brief Функция сборки фрейма RST_STREAM (заголовок + нагрузка дописываются в output)
					 *
					 * @param output   выходной буфер соединения
					 * @param streamId идентификатор потока
					 * @param error    код ошибки, с которым сбрасывается поток
					 *
					 * \~english
					 * @brief Function of assembling a RST_STREAM frame (the header + the payload are appended into output)
					 * @param output   output buffer of the connection
					 * @param streamId identifier of the stream
					 * @param error    error code with which the stream is reset
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ void rstStream(string & output, const uint32_t streamId, const error_t error) noexcept;
					/**
					 * \~russian
					 * @brief Функция сборки фрейма WINDOW_UPDATE (заголовок + нагрузка дописываются в output)
					 *
					 * @param output    выходной буфер соединения
					 * @param streamId  идентификатор потока (0 - окно всего соединения)
					 * @param increment инкремент окна flow control
					 *
					 * \~english
					 * @brief Function of assembling a WINDOW_UPDATE frame (the header + the payload are appended into output)
					 * @param output    output buffer of the connection
					 * @param streamId  identifier of the stream (0 - the window of the whole connection)
					 * @param increment increment of the window of the flow control
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ void windowUpdate(string & output, const uint32_t streamId, const uint32_t increment) noexcept;
					/**
					 * \~russian
					 * @brief Функция сборки фрейма DATA (заголовок + нагрузка дописываются в output)
					 *
					 * @param output    выходной буфер соединения
					 * @param streamId  идентификатор потока
					 * @param data      данные тела
					 * @param endStream флаг завершения потока
					 *
					 * \~english
					 * @brief Function of assembling a DATA frame (the header + the payload are appended into output)
					 * @param output    output buffer of the connection
					 * @param streamId  identifier of the stream
					 * @param data      data of the body
					 * @param endStream flag of the completion of the stream
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ void data(string & output, const uint32_t streamId, string_view data, const bool endStream) noexcept;
					/**
					 * \~russian
					 * @brief Функция сборки фрейма SETTINGS (заголовок + нагрузка дописываются в output)
					 *
					 * @param output выходной буфер соединения
					 * @param items  список параметров (для ACK игнорируется)
					 * @param count  количество параметров
					 * @param ack    флаг подтверждения получения SETTINGS пира
					 *
					 * \~english
					 * @brief Function of assembling a SETTINGS frame (the header + the payload are appended into output)
					 * @param output output buffer of the connection
					 * @param items  list of the parameters (for an ACK it is ignored)
					 * @param count  number of the parameters
					 * @param ack    flag of the confirmation of the receipt of a SETTINGS of the peer
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ void settings(string & output, const setting_entry_t * items, const size_t count, const bool ack) noexcept;
					/**
					 * \~russian
					 * @brief Функция сборки фрейма GOAWAY (заголовок + нагрузка дописываются в output)
					 *
					 * @param output       выходной буфер соединения
					 * @param lastStreamId наибольший идентификатор обработанного потока
					 * @param error        код ошибки завершения соединения
					 * @param debugData    необязательные отладочные данные
					 *
					 * \~english
					 * @brief Function of assembling a GOAWAY frame (the header + the payload are appended into output)
					 * @param output       output buffer of the connection
					 * @param lastStreamId largest identifier of a processed stream
					 * @param error        error code of the completion of the connection
					 * @param debugData    optional debug data
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ void goaway(string & output, const uint32_t lastStreamId, const error_t error, string_view debugData) noexcept;
					/**
					 * \~russian
					 * @brief Функция сборки фрейма CONTINUATION (заголовок + нагрузка дописываются в output)
					 *
					 * @param output     выходной буфер соединения
					 * @param streamId   идентификатор потока
					 * @param block      фрагмент блока заголовков HPACK
					 * @param endHeaders флаг завершения блока заголовков
					 *
					 * \~english
					 * @brief Function of assembling a CONTINUATION frame (the header + the payload are appended into output)
					 * @param output     output buffer of the connection
					 * @param streamId   identifier of the stream
					 * @param block      fragment of the block of the headers of HPACK
					 * @param endHeaders flag of the completion of the block of the headers
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ void continuation(string & output, const uint32_t streamId, string_view block, const bool endHeaders) noexcept;
					/**
					 * \~russian
					 * @brief Функция сборки фрейма HEADERS (заголовок + нагрузка дописываются в output)
					 *
					 * @param output     выходной буфер соединения
					 * @param streamId   идентификатор потока
					 * @param block      фрагмент блока заголовков HPACK
					 * @param endStream  флаг завершения потока
					 * @param endHeaders флаг завершения блока заголовков
					 *
					 * \~english
					 * @brief Function of assembling a HEADERS frame (the header + the payload are appended into output)
					 * @param output     output buffer of the connection
					 * @param streamId   identifier of the stream
					 * @param block      fragment of the block of the headers of HPACK
					 * @param endStream  flag of the completion of the stream
					 * @param endHeaders flag of the completion of the block of the headers
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ void headers(string & output, const uint32_t streamId, string_view block, const bool endStream, const bool endHeaders) noexcept;
					/**
					 * \~russian
					 * @brief Функция сборки фрейма PRIORITY (заголовок + нагрузка дописываются в output)
					 *
					 * @param output    выходной буфер соединения
					 * @param streamId  идентификатор потока
					 * @param exclusive флаг эксклюзивной зависимости потока
					 * @param streamDep идентификатор потока, от которого зависит текущий
					 * @param weight    вес потока
					 *
					 * \~english
					 * @brief Function of assembling a PRIORITY frame (the header + the payload are appended into output)
					 * @param output    output buffer of the connection
					 * @param streamId  identifier of the stream
					 * @param exclusive flag of an exclusive dependence of the stream
					 * @param streamDep identifier of the stream on which the current one depends
					 * @param weight    weight of the stream
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ void priority(string & output, const uint32_t streamId, const bool exclusive, const uint32_t streamDep, const uint8_t weight) noexcept;
					/**
					 * \~russian
					 * @brief Функция сборки фрейма PRIORITY_UPDATE (RFC 9218 §7.1)
					 *
					 * @param output   выходной буфер соединения
					 * @param streamId идентификатор приоритизируемого потока
					 * @param value    значение поля приоритета (структурированный словарь, например "u=2, i")
					 *
					 * \~english
					 * @brief Function of assembling a PRIORITY_UPDATE frame (RFC 9218 §7.1)
					 * @param output   output buffer of the connection
					 * @param streamId identifier of the stream being prioritized
					 * @param value    value of the field of the priority (a structured dictionary, for example "u=2, i")
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ void priorityUpdate(string & output, const uint32_t streamId, string_view value) noexcept;
					/**
					 * \~russian
					 * @brief Функция сборки фрейма ALTSVC (RFC 7838 §4)
					 *
					 * @note Кадр для соединения (streamId == 0) обязан нести непустой origin,
					 *       кадр для потока - пустой: во втором случае origin берётся
					 *       из самого потока
					 *
					 * @param output   выходной буфер соединения
					 * @param streamId идентификатор потока, либо 0 для соединения
					 * @param origin   origin анонсируемого сервиса
					 * @param value    значение поля Alt-Svc (RFC 7838 §3)
					 *
					 * \~english
					 * @brief Function of assembling an ALTSVC frame (RFC 7838 §4)
					 * @note A frame for a connection (streamId == 0) is obliged to carry a non-empty origin,
					 *       a frame for a stream - an empty one: in the second case the origin is taken
					 *       from the stream itself
					 * @param output   output buffer of the connection
					 * @param streamId identifier of the stream, or 0 for the connection
					 * @param origin   origin of the service being announced
					 * @param value    value of the field Alt-Svc (RFC 7838 §3)
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ void altsvc(string & output, const uint32_t streamId, string_view origin, string_view value) noexcept;
					/**
					 * \~russian
					 * @brief Функция сборки фрейма ORIGIN (RFC 8336 §2)
					 *
					 * @note Кадр относится к соединению целиком и отправляется только
					 *       в потоке 0
					 *
					 * @param output  выходной буфер соединения
					 * @param origins набор origin, обслуживаемых соединением
					 *
					 * \~english
					 * @brief Function of assembling an ORIGIN frame (RFC 8336 §2)
					 * @note The frame relates to the connection as a whole and is sent only
					 *       in the stream 0
					 * @param output  output buffer of the connection
					 * @param origins collection of the origins served by the connection
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ void origin(string & output, const vector <string> & origins) noexcept;
					/**
					 * \~russian
					 * @brief Функция сборки HPACK-блока в HEADERS + CONTINUATION (RFC 9113 §6.2/§6.10)
					 *
					 * @note END_STREAM, если задан, ставится только на первый HEADERS -
					 *       даже если блок продолжается в CONTINUATION
					 *
					 * @param output          выходной буфер соединения
					 * @param streamId        идентификатор потока
					 * @param block           блок заголовков HPACK целиком
					 * @param endStream       флаг завершения потока
					 * @param maxFramePayload максимальный размер полезной нагрузки одного фрейма (SETTINGS_MAX_FRAME_SIZE пира)
					 *
					 * \~english
					 * @brief Function of assembling a block of HPACK into HEADERS + CONTINUATION (RFC 9113 §6.2/§6.10)
					 * @note END_STREAM, if it is given, is put only onto the first HEADERS -
					 *       even if the block continues in a CONTINUATION
					 * @param output          output buffer of the connection
					 * @param streamId        identifier of the stream
					 * @param block           block of the headers of HPACK entirely
					 * @param endStream       flag of the completion of the stream
					 * @param maxFramePayload largest size of the payload of a single frame (SETTINGS_MAX_FRAME_SIZE of the peer)
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ void headerBlock(string & output, const uint32_t streamId, string_view block, const bool endStream, const uint32_t maxFramePayload) noexcept;
					/**
					 * \~russian
					 * @brief Функция сборки фрейма PUSH_PROMISE (заголовок + нагрузка дописываются в output)
					 *
					 * @param output           выходной буфер соединения
					 * @param streamId         идентификатор ассоциированного потока клиента
					 * @param promisedStreamId идентификатор обещанного потока
					 * @param block            фрагмент блока заголовков HPACK
					 * @param endHeaders       флаг завершения блока заголовков
					 *
					 * \~english
					 * @brief Function of assembling a PUSH_PROMISE frame (the header + the payload are appended into output)
					 * @param output           output buffer of the connection
					 * @param streamId         identifier of the associated stream of the client
					 * @param promisedStreamId identifier of the promised stream
					 * @param block            fragment of the block of the headers of HPACK
					 * @param endHeaders       flag of the completion of the block of the headers
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ void pushPromise(string & output, const uint32_t streamId, const uint32_t promisedStreamId, string_view block, const bool endHeaders) noexcept;
					/**
					 * \~russian
					 * @brief Функция сборки HPACK-блока обещанного запроса в PUSH_PROMISE + CONTINUATION
					 *
					 * @note Первый фрейм резервирует 4 октета под Promised Stream ID;
					 *       остаток блока уходит в CONTINUATION при необходимости
					 *
					 * @param output           выходной буфер соединения
					 * @param streamId         идентификатор ассоциированного потока клиента
					 * @param promisedStreamId идентификатор обещанного потока
					 * @param block            блок заголовков HPACK целиком
					 * @param maxFramePayload  максимальный размер полезной нагрузки одного фрейма
					 *
					 * \~english
					 * @brief Function of assembling a block of HPACK of a promised request into PUSH_PROMISE + CONTINUATION
					 * @note The first frame reserves 4 octets for the Promised Stream ID;
					 *       the remainder of the block goes away into a CONTINUATION at the necessity
					 * @param output           output buffer of the connection
					 * @param streamId         identifier of the associated stream of the client
					 * @param promisedStreamId identifier of the promised stream
					 * @param block            block of the headers of HPACK entirely
					 * @param maxFramePayload  largest size of the payload of a single frame
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ void pushPromiseBlock(string & output, const uint32_t streamId, const uint32_t promisedStreamId, string_view block, const uint32_t maxFramePayload) noexcept;
				};
			}
		}
	};
};

#endif // __AWH_HTTP_PARSER_HTTP2_FRAME__
