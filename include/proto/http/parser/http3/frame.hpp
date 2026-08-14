/**
 * @file frame.hpp
 * @date 2026-07-27
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
 * @brief Заголовочный файл слоя кадров HTTP/3 (RFC 9114 §7) — разбор и сборка кадров,
 *        закодированных целыми переменной длины QUIC, без состояния соединения
 *
 * \~english
 * @brief Header file of the layer of the frames of HTTP/3 (RFC 9114 §7) — the parsing and the assembly of the frames
 *        encoded by the integers of a variable length of QUIC, without the state of the connection
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_HTTP_PARSER_HTTP3_FRAME__
#define __AWH_HTTP_PARSER_HTTP3_FRAME__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <unordered_set>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "h3.hpp"
#include "../../../quic/varint.hpp"
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
		 * @brief Пространство имён внутренних слоёв протокола HTTP/3
		 *
		 * \~english
		 * @brief Namespace of the internal layers of the HTTP/3 protocol
		 *
		 * \~
		 */
		namespace h3 {
			/**
			 * \~russian
			 * @brief Пространство имён слоя кадров HTTP/3 (RFC 9114 §7): разбор и сборка кадров
			 *
			 * @details Разбор zero-copy: полезная нагрузка отдаётся как string_view с указателем
			 *          во входной буфер. Сборка дописывает байты в string (выходной буфер потока).
			 *          Слой не хранит состояния соединения - это чистые функции над байтами.
			 *
			 *          Устройство кадра предельно простое: тип и длина нагрузки, оба целыми
			 *          переменной длины QUIC (RFC 9000 §16). Ни флагов, ни идентификатора потока
			 *          в кадре нет: поток задаёт транспорт, а роль флагов END_STREAM и END_HEADERS
			 *          исполняют признак FIN потока QUIC и сама граница кадра.
			 *
			 * @note Верхней границы длины кадра протокол не задаёт - параметра, подобного
			 *       SETTINGS_MAX_FRAME_SIZE, в HTTP/3 нет. Поэтому нагрузка кадра DATA обязана
			 *       разбираться по частям по мере поступления, а не собираться в буфере целиком:
			 *       иначе отправитель одним кадром задавал бы потребление памяти получателем.
			 *       Функции этого слоя разбирают нагрузку целиком только для управляющих кадров,
			 *       длина которых ограничена лимитами парсера сессии
			 *
			 * \~english
			 * @brief Namespace of the layer of the frames of HTTP/3 (RFC 9114 §7): the parsing and the assembly of the frames
			 * @details The parsing is zero-copy: the payload is issued as a string_view with a pointer
			 *          into the input buffer. The assembly appends the octets into a string (the output buffer of the stream).
			 *          The layer stores no state of the connection - these are pure functions over the octets.
			 *          The arrangement of a frame is utterly simple: the type and the length of the payload, both as integers
			 *          of a variable length of QUIC (RFC 9000 §16). There are neither flags nor an identifier of the stream
			 *          in a frame: the stream is given by the transport, while the role of the flags END_STREAM and END_HEADERS
			 *          is performed by the flag FIN of a stream of QUIC and by the boundary of the frame itself.
			 * @note The protocol gives no upper boundary of the length of a frame - there is no parameter similar to
			 *       SETTINGS_MAX_FRAME_SIZE in HTTP/3. Therefore the payload of a DATA frame is obliged
			 *       to be parsed by parts as it arrives rather than to be collected in a buffer entirely:
			 *       otherwise a sender would by a single frame set the consumption of the memory by the receiver.
			 *       The functions of this layer parse the payload entirely only for the control frames
			 *       the length of which is limited by the limits of the parser of the session
			 *
			 * \~
			 */
			namespace frame {
				/**
				 * \~russian
				 * @brief Порог перехода на множество при поиске повторов SETTINGS
				 *
				 * @details Обычный кадр несёт единицы параметров, и перебор набора
				 *          дешевле хеш-множества вместе с его аллокацией. Кадр
				 *          предельного размера вмещает тысячи различных
				 *          идентификаторов, и перебор становится квадратичным
				 *
				 * \~english
				 * @brief Threshold of the transition to a set at the search of the repetitions of SETTINGS
				 * @details An ordinary frame carries units of the parameters, and an enumeration of the collection
				 *          is cheaper than a hash set together with its allocation. A frame
				 *          of the limiting size holds thousands of different
				 *          identifiers, and the enumeration becomes quadratic
				 *
				 * \~
				 */
				static constexpr size_t SETTINGS_LOOKUP_THRESHOLD = (16);
				/**
				 * \~russian
				 * @brief Структура параметра SETTINGS (идентификатор + значение)
				 *
				 * @note Оба поля - целые переменной длины, поэтому 64-битные: в HTTP/2
				 *       идентификатор занимал 16 бит, а значение 32
				 *
				 * \~english
				 * @brief Structure of a parameter of SETTINGS (an identifier + a value)
				 * @note Both fields are integers of a variable length, therefore they are 64-bit ones: in HTTP/2
				 *       an identifier occupied 16 bits, while a value 32
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Setting {
					// Идентификатор параметра
					uint64_t id;
					// Значение параметра
					uint64_t value;
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
				 * @brief Структура разобранного заголовка кадра (RFC 9114 §7.1)
				 *
				 * \~english
				 * @brief Structure of a parsed header of a frame (RFC 9114 §7.1)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Header {
					// Тип кадра
					uint64_t type;
					// Длина полезной нагрузки в октетах
					uint64_t length;
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
				 * @brief Структура полезной нагрузки PUSH_PROMISE (RFC 9114 §7.2.5)
				 *
				 * \~english
				 * @brief Structure of the payload of PUSH_PROMISE (RFC 9114 §7.2.5)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Push_Promise {
					// Идентификатор обещанного push
					uint64_t pushId;
					// Секция полей запроса, закодированная QPACK (zero-copy во входной буфер)
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
				 * @brief Структура полезной нагрузки PRIORITY_UPDATE (RFC 9218 §7.2)
				 *
				 * \~english
				 * @brief Structure of the payload of PRIORITY_UPDATE (RFC 9218 §7.2)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Priority_Update {
					// Признак того, что приоритет назначается потоку push, а не потоку запроса
					bool push;
					// Идентификатор потока запроса либо идентификатор push
					uint64_t id;
					// Значение поля приоритета в синтаксисе структурированных полей (zero-copy)
					string_view value;
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
					explicit Priority_Update() noexcept;
				} priority_update_t;

				/**
				 * \~russian
				 * @brief Пространство имён функций разбора кадров HTTP/3 (RFC 9114 §7)
				 *
				 * @details Функции разбора нагрузки принимают её целиком: длина управляющих
				 *          кадров ограничена лимитами парсера сессии, поэтому их накопление
				 *          в буфере безопасно. Нагрузка кадра DATA через этот слой не проходит -
				 *          она отдаётся потребителю по частям парсером сессии
				 *
				 * \~english
				 * @brief Namespace of the functions of the parsing of the frames of HTTP/3 (RFC 9114 §7)
				 * @details The functions of the parsing of a payload accept it entirely: the length of the control
				 *          frames is limited by the limits of the parser of the session, therefore their accumulation
				 *          in a buffer is safe. The payload of a DATA frame does not pass through this layer -
				 *          it is issued to the consumer by parts by the parser of the session
				 *
				 * \~
				 */
				namespace parser {
					/**
					 * \~russian
					 * @brief Функция разбора заголовка кадра
					 *
					 * @details Заголовок кадра занимает от двух до шестнадцати октетов: тип и длина
					 *          кодируются целыми переменной длины независимо друг от друга
					 *
					 * @param data   входной буфер
					 * @param size   доступно байт
					 * @param output разобранный заголовок кадра
					 * @return       количество прочитанных октетов либо 0, если данных недостаточно
					 *
					 * \~english
					 * @brief Function of parsing the header of a frame
					 * @details The header of a frame occupies from two to sixteen octets: the type and the length
					 *          are encoded by the integers of a variable length independently of each other
					 * @param data   input buffer
					 * @param size   octets available
					 * @param output parsed header of the frame
					 * @return       number of the read octets or 0, if the data is insufficient
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ size_t header(const uint8_t * data, const size_t size, header_t & output) noexcept;
					/**
					 * \~russian
					 * @brief Функция разбора нагрузки кадра из единственного целого переменной длины
					 *
					 * @details Общая форма нагрузки кадров CANCEL_PUSH, MAX_PUSH_ID и GOAWAY.
					 *          Лишние октеты после числа - ошибка H3_FRAME_ERROR (RFC 9114 §7.1)
					 *
					 * @param payload полезная нагрузка кадра
					 * @param size    размер полезной нагрузки
					 * @param value   разобранное число
					 * @param error   код ошибки протокола
					 * @return        результат разбора (OK/ERROR)
					 *
					 * \~english
					 * @brief Function of parsing the payload of a frame out of a single integer of a variable length
					 * @details The common form of the payload of the frames CANCEL_PUSH, MAX_PUSH_ID and GOAWAY.
					 *          Superfluous octets after the number are an error H3_FRAME_ERROR (RFC 9114 §7.1)
					 * @param payload payload of the frame
					 * @param size    size of the payload
					 * @param value   parsed number
					 * @param error   error code of the protocol
					 * @return        result of the parsing (OK/ERROR)
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ status_t identifier(const uint8_t * payload, const size_t size, uint64_t & value, error_t & error) noexcept;
					/**
					 * \~russian
					 * @brief Функция разбора нагрузки кадра SETTINGS (RFC 9114 §7.2.4)
					 *
					 * @details Повторно встреченный идентификатор - ошибка H3_SETTINGS_ERROR
					 *          (RFC 9114 §7.2.4.1), а обрыв нагрузки посреди пары - H3_FRAME_ERROR
					 *          (RFC 9114 §7.1): требования к нагрузке любого кадра и требования
					 *          именно к SETTINGS нарушаются по-разному и разводятся по кодам.
					 *          Зарезервированные идентификаторы отдаются наружу как есть: решение
					 *          об их пропуске принимает парсер сессии
					 *
					 * @param payload полезная нагрузка кадра
					 * @param size    размер полезной нагрузки
					 * @param output  разобранный набор параметров
					 * @param error   код ошибки протокола
					 * @return        результат разбора (OK/ERROR)
					 *
					 * \~english
					 * @brief Function of parsing the payload of a SETTINGS frame (RFC 9114 §7.2.4)
					 * @details A repeatedly met identifier is an error H3_SETTINGS_ERROR
					 *          (RFC 9114 §7.2.4.1), while a break of the payload in the middle of a pair is H3_FRAME_ERROR
					 *          (RFC 9114 §7.1): the requirements to the payload of any frame and the requirements
					 *          to SETTINGS in particular are violated differently and are separated by the codes.
					 *          The reserved identifiers are issued outside as they are: the decision
					 *          about their skipping is taken by the parser of the session
					 * @param payload payload of the frame
					 * @param size    size of the payload
					 * @param output  parsed collection of the parameters
					 * @param error   error code of the protocol
					 * @return        result of the parsing (OK/ERROR)
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ status_t settings(const uint8_t * payload, const size_t size, vector <setting_entry_t> & output, error_t & error) noexcept;
					/**
					 * \~russian
					 * @brief Функция разбора нагрузки кадра PUSH_PROMISE (RFC 9114 §7.2.5)
					 *
					 * @param payload полезная нагрузка кадра
					 * @param size    размер полезной нагрузки
					 * @param output  разобранная полезная нагрузка
					 * @param error   код ошибки протокола
					 * @return        результат разбора (OK/ERROR)
					 *
					 * \~english
					 * @brief Function of parsing the payload of a PUSH_PROMISE frame (RFC 9114 §7.2.5)
					 * @param payload payload of the frame
					 * @param size    size of the payload
					 * @param output  parsed payload
					 * @param error   error code of the protocol
					 * @return        result of the parsing (OK/ERROR)
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ status_t pushPromise(const uint8_t * payload, const size_t size, push_promise_t & output, error_t & error) noexcept;
					/**
					 * \~russian
					 * @brief Функция разбора нагрузки кадра PRIORITY_UPDATE (RFC 9218 §7.2)
					 *
					 * @param type    тип кадра, различающий поток запроса и поток push
					 * @param payload полезная нагрузка кадра
					 * @param size    размер полезной нагрузки
					 * @param output  разобранная полезная нагрузка
					 * @param error   код ошибки протокола
					 * @return        результат разбора (OK/ERROR)
					 *
					 * \~english
					 * @brief Function of parsing the payload of a PRIORITY_UPDATE frame (RFC 9218 §7.2)
					 * @param type    type of the frame distinguishing a stream of a request and a stream of a push
					 * @param payload payload of the frame
					 * @param size    size of the payload
					 * @param output  parsed payload
					 * @param error   error code of the protocol
					 * @return        result of the parsing (OK/ERROR)
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ status_t priorityUpdate(const uint64_t type, const uint8_t * payload, const size_t size, priority_update_t & output, error_t & error) noexcept;
				};

				/**
				 * \~russian
				 * @brief Пространство имён функций сборки кадров HTTP/3 (RFC 9114 §7)
				 *
				 * \~english
				 * @brief Namespace of the functions of the assembly of the frames of HTTP/3 (RFC 9114 §7)
				 *
				 * \~
				 */
				namespace serialize {
					/**
					 * \~russian
					 * @brief Функция записи заголовка кадра
					 *
					 * @param output выходной буфер
					 * @param type   тип кадра
					 * @param length длина полезной нагрузки
					 *
					 * \~english
					 * @brief Function of writing the header of a frame
					 * @param output output buffer
					 * @param type   type of the frame
					 * @param length length of the payload
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ void header(string & output, const uint64_t type, const uint64_t length) noexcept;
					/**
					 * \~russian
					 * @brief Функция записи типа однонаправленного потока (RFC 9114 §6.2)
					 *
					 * @details Тип потока отправляется единственным целым переменной длины
					 *          в самое начало потока, до любых кадров
					 *
					 * @param output выходной буфер
					 * @param type   тип однонаправленного потока
					 *
					 * \~english
					 * @brief Function of writing the type of a unidirectional stream (RFC 9114 §6.2)
					 * @details The type of the stream is sent as a single integer of a variable length
					 *          into the very beginning of the stream, before any frames
					 * @param output output buffer
					 * @param type   type of the unidirectional stream
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ void unistream(string & output, const uint64_t type) noexcept;
					/**
					 * \~russian
					 * @brief Функция записи кадра DATA (RFC 9114 §7.2.1)
					 *
					 * @param output выходной буфер
					 * @param data   данные тела
					 *
					 * \~english
					 * @brief Function of writing a DATA frame (RFC 9114 §7.2.1)
					 * @param output output buffer
					 * @param data   data of the body
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ void data(string & output, string_view data) noexcept;
					/**
					 * \~russian
					 * @brief Функция записи кадра HEADERS (RFC 9114 §7.2.2)
					 *
					 * @note Нарезки на несколько кадров, подобной CONTINUATION в HTTP/2, здесь нет:
					 *       длина кадра не ограничена, поэтому секция полей передаётся целиком
					 *
					 * @param output выходной буфер
					 * @param block  секция полей, закодированная QPACK
					 *
					 * \~english
					 * @brief Function of writing a HEADERS frame (RFC 9114 §7.2.2)
					 * @note There is no cutting into several frames similar to CONTINUATION in HTTP/2 here:
					 *       the length of a frame is not limited, therefore the section of the fields is transmitted entirely
					 * @param output output buffer
					 * @param block  section of the fields encoded by QPACK
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ void headers(string & output, string_view block) noexcept;
					/**
					 * \~russian
					 * @brief Функция записи кадра GOAWAY (RFC 9114 §7.2.6)
					 *
					 * @param output выходной буфер
					 * @param id     идентификатор потока запроса (от сервера) либо push (от клиента)
					 *
					 * \~english
					 * @brief Function of writing a GOAWAY frame (RFC 9114 §7.2.6)
					 * @param output output buffer
					 * @param id     identifier of a stream of a request (from the server) or of a push (from the client)
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ void goaway(string & output, const uint64_t id) noexcept;
					/**
					 * \~russian
					 * @brief Функция записи кадра CANCEL_PUSH (RFC 9114 §7.2.3)
					 *
					 * @param output выходной буфер
					 * @param pushId идентификатор отменяемого push
					 *
					 * \~english
					 * @brief Function of writing a CANCEL_PUSH frame (RFC 9114 §7.2.3)
					 * @param output output buffer
					 * @param pushId identifier of the push being cancelled
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ void cancelPush(string & output, const uint64_t pushId) noexcept;
					/**
					 * \~russian
					 * @brief Функция записи кадра MAX_PUSH_ID (RFC 9114 §7.2.7)
					 *
					 * @param output выходной буфер
					 * @param pushId наибольший допустимый идентификатор push
					 *
					 * \~english
					 * @brief Function of writing a MAX_PUSH_ID frame (RFC 9114 §7.2.7)
					 * @param output output buffer
					 * @param pushId largest admissible identifier of a push
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ void maxPushId(string & output, const uint64_t pushId) noexcept;
					/**
					 * \~russian
					 * @brief Функция записи кадра SETTINGS (RFC 9114 §7.2.4)
					 *
					 * @param output выходной буфер
					 * @param items  набор параметров
					 * @param count  количество параметров
					 *
					 * \~english
					 * @brief Function of writing a SETTINGS frame (RFC 9114 §7.2.4)
					 * @param output output buffer
					 * @param items  collection of the parameters
					 * @param count  number of the parameters
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ void settings(string & output, const setting_entry_t * items, const size_t count) noexcept;
					/**
					 * \~russian
					 * @brief Функция записи кадра PUSH_PROMISE (RFC 9114 §7.2.5)
					 *
					 * @param output выходной буфер
					 * @param pushId идентификатор обещанного push
					 * @param block  секция полей запроса, закодированная QPACK
					 *
					 * \~english
					 * @brief Function of writing a PUSH_PROMISE frame (RFC 9114 §7.2.5)
					 * @param output output buffer
					 * @param pushId identifier of the promised push
					 * @param block  section of the fields of the request encoded by QPACK
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ void pushPromise(string & output, const uint64_t pushId, string_view block) noexcept;
					/**
					 * \~russian
					 * @brief Функция записи кадра PRIORITY_UPDATE (RFC 9218 §7.2)
					 *
					 * @param output выходной буфер
					 * @param push   признак назначения приоритета потоку push, а не потоку запроса
					 * @param id     идентификатор потока запроса либо идентификатор push
					 * @param value  значение поля приоритета в синтаксисе структурированных полей
					 *
					 * \~english
					 * @brief Function of writing a PRIORITY_UPDATE frame (RFC 9218 §7.2)
					 * @param output output buffer
					 * @param push   flag of the assignment of the priority to a stream of a push rather than to a stream of a request
					 * @param id     identifier of the stream of the request or identifier of the push
					 * @param value  value of the field of the priority in the syntax of the structured fields
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ void priorityUpdate(string & output, const bool push, const uint64_t id, string_view value) noexcept;
					/**
					 * \~russian
					 * @brief Функция записи зарезервированного кадра (RFC 9114 §7.2.8)
					 *
					 * @details Кадр с типом вида (0x1F * N + 0x21) и произвольной нагрузкой обязан
					 *          игнорироваться получателем. Отправка такого кадра - единственный
					 *          способ убедиться, что пир не считает набор типов кадров закрытым
					 *
					 * @param output выходной буфер
					 * @param seed   порядковый номер N в последовательности зарезервированных типов
					 * @param data   произвольная нагрузка кадра
					 *
					 * \~english
					 * @brief Function of writing a reserved frame (RFC 9114 §7.2.8)
					 * @details A frame with a type of the form (0x1F * N + 0x21) and an arbitrary payload is obliged
					 *          to be ignored by the receiver. The sending of such a frame is the only
					 *          way to make sure that the peer does not consider the collection of the types of the frames closed
					 * @param output output buffer
					 * @param seed   ordinal number N in the sequence of the reserved types
					 * @param data   arbitrary payload of the frame
					 *
					 * \~
					 */
					__AWH_SHARED_EXPORT__ void reserved(string & output, const uint64_t seed, string_view data = {}) noexcept;
				};
			};
		}
	};
};

#endif // __AWH_HTTP_PARSER_HTTP3_FRAME__
