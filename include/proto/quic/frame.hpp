/**
 * @file: frame.hpp
 * @date: 2026-07-21
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл слоя фреймов QUIC (RFC 9000 §19) — структуры фреймов ACK, STREAM, CRYPTO, RESET_STREAM,
 *        STOP_SENDING, NEW_CONNECTION_ID, CONNECTION_CLOSE и чистые функции их разбора и сборки над байтовым буфером
 *
 * \~english
 * @brief Header file of the QUIC frame layer (RFC 9000 §19) — the structures of the ACK, STREAM, CRYPTO, RESET_STREAM,
 *        STOP_SENDING, NEW_CONNECTION_ID, CONNECTION_CLOSE frames and the pure functions of their parsing and assembly over a byte buffer
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_PROTO_QUIC_FRAME__
#define __AWH_PROTO_QUIC_FRAME__

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
#include "quic.hpp"
#include "../../sys/global.hpp"

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
	 * @brief Пространство имён транспортного протокола QUIC
	 *
	 *
	 * \~english
	 * @brief QUIC transport protocol namespace
	 *
	 * \~
	 */
	namespace quic {
		/**
		 * \~russian
		 * @brief Пространство имён слоя фреймов QUIC (RFC 9000 §19): разбор и сборка фреймов
		 *
		 * @details Фреймы лежат в расшифрованной нагрузке пакета друг за другом без
		 *          разделителей. Разбор zero-copy: данные потоков отдаются как string_view
		 *          с указателем в буфер расшифрованной нагрузки. Каждая функция разбора
		 *          читает тип фрейма сама и возвращает количество потреблённых октетов.
		 *          Слой не хранит состояния соединения - это чистые функции над байтами.
		 *
		 * \~english
		 * @brief Namespace of the QUIC frame layer (RFC 9000 §19): the parsing and the assembly of the frames
		 * @details The frames lie in the decrypted payload of a packet one after another without
		 *          separators. The parsing is zero-copy: the data of the streams is given away as a string_view
		 *          with a pointer into the buffer of the decrypted payload. Every function of the parsing
		 *          reads the type of the frame itself and returns the number of the consumed octets.
		 *          The layer does not store the state of a connection — these are pure functions over the bytes.
		 *
		 * \~
		 */
		namespace frame {
			/**
			 * \~russian
			 * @brief Предельное количество диапазонов в принимаемом фрейме ACK (RFC 9000 §19.3)
			 *
			 * @details Протокол количество диапазонов не ограничивает - в пакет 1200 октетов
			 *          их помещается несколько сотен, а сопоставление каждого диапазона
			 *          с отправленными пакетами стоит процессорного времени. Превышение
			 *          лимита трактуется как FRAME_ENCODING_ERROR
			 *
			 * \~english
			 * @brief Limit of the number of the ranges in an accepted ACK frame (RFC 9000 §19.3)
			 * @details The protocol does not limit the number of the ranges — several hundred of them fit
			 *          into a packet of 1200 octets, while the matching of every range
			 *          against the sent packets costs processor time. An excess
			 *          of the limit is treated as FRAME_ENCODING_ERROR
			 *
			 * \~
			 */
			static constexpr size_t MAX_ACK_RANGES = 256;

			/**
			 * \~russian
			 * @brief Структура диапазона подтверждённых номеров пакетов [low, high]
			 *
			 * \~english
			 * @brief Structure of a range of the acknowledged packet numbers [low, high]
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Range {
				// Наименьший номер пакета диапазона
				uint64_t low;
				// Наибольший номер пакета диапазона
				uint64_t high;
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
				explicit Range() noexcept;
			} range_t;
			/**
			 * \~russian
			 * @brief Структура фрейма ACK (RFC 9000 §19.3), диапазоны декодированы в абсолютные номера
			 *
			 * \~english
			 * @brief Structure of the ACK frame (RFC 9000 §19.3), the ranges are decoded into the absolute numbers
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Ack {
				// Флаг наличия счётчиков ECN (тип фрейма 0x03)
				bool hasEcn;
				// Задержка подтверждения в единицах 2^ackDelayExponent микросекунд
				uint64_t delay;
				// Счётчик пакетов с маркировкой ECT(0)
				uint64_t ect0;
				// Счётчик пакетов с маркировкой ECT(1)
				uint64_t ect1;
				// Счётчик пакетов с маркировкой CE (перегрузка)
				uint64_t ce;
				// Диапазоны подтверждённых номеров пакетов в порядке убывания (первый содержит наибольший номер)
				vector <range_t> ranges;
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
				explicit Ack() noexcept;
			} ack_t;
			/**
			 * \~russian
			 * @brief Структура фрейма RESET_STREAM (RFC 9000 §19.4)
			 *
			 * \~english
			 * @brief Structure of the RESET_STREAM frame (RFC 9000 §19.4)
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Reset_Stream {
				// Идентификатор потока
				uint64_t streamId;
				// Код ошибки приложения
				uint64_t code;
				// Финальный размер потока в октетах
				uint64_t finalSize;
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
				explicit Reset_Stream() noexcept;
			} reset_stream_t;
			/**
			 * \~russian
			 * @brief Структура фрейма STOP_SENDING (RFC 9000 §19.5)
			 *
			 * \~english
			 * @brief Structure of the STOP_SENDING frame (RFC 9000 §19.5)
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Stop_Sending {
				// Идентификатор потока
				uint64_t streamId;
				// Код ошибки приложения
				uint64_t code;
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
				explicit Stop_Sending() noexcept;
			} stop_sending_t;
			/**
			 * \~russian
			 * @brief Структура фрейма CRYPTO (RFC 9000 §19.6)
			 *
			 * \~english
			 * @brief Structure of the CRYPTO frame (RFC 9000 §19.6)
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Crypto {
				// Смещение данных в потоке криптографического хендшейка
				uint64_t offset;
				// Данные криптографического хендшейка (zero-copy в буфер нагрузки)
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
				explicit Crypto() noexcept;
			} crypto_t;
			/**
			 * \~russian
			 * @brief Структура фрейма STREAM (RFC 9000 §19.8), поля Offset/Length декодированы
			 *
			 * \~english
			 * @brief Structure of the STREAM frame (RFC 9000 §19.8), the Offset/Length fields are decoded
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Stream {
				// Флаг завершения потока (FIN)
				bool fin;
				// Идентификатор потока
				uint64_t streamId;
				// Смещение данных в потоке
				uint64_t offset;
				// Данные потока приложения (zero-copy в буфер нагрузки)
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
				explicit Stream() noexcept;
			} stream_t;
			/**
			 * \~russian
			 * @brief Структура фрейма NEW_CONNECTION_ID (RFC 9000 §19.15)
			 *
			 * \~english
			 * @brief Structure of the NEW_CONNECTION_ID frame (RFC 9000 §19.15)
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ New_Connection_Id {
				// Порядковый номер идентификатора соединения
				uint64_t seq;
				// Порядковый номер, до которого идентификаторы выводятся из обращения
				uint64_t retirePriorTo;
				// Новый идентификатор соединения
				cid_t cid;
				// Токен сброса без сохранения состояния (RFC 9000 §10.3)
				uint8_t resetToken[proto::RESET_TOKEN_SIZE];
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
				explicit New_Connection_Id() noexcept;
			} new_connection_id_t;
			/**
			 * \~russian
			 * @brief Структура фрейма CONNECTION_CLOSE (RFC 9000 §19.19)
			 *
			 * \~english
			 * @brief Structure of the CONNECTION_CLOSE frame (RFC 9000 §19.19)
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Connection_Close {
				// Флаг ошибки приложения (тип фрейма 0x1D)
				bool app;
				// Код ошибки (транспорта или приложения)
				uint64_t code;
				// Тип фрейма, вызвавшего ошибку (только для ошибки транспорта)
				uint64_t frameType;
				// Человекочитаемая причина завершения (zero-copy в буфер нагрузки)
				string_view reason;
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
				explicit Connection_Close() noexcept;
			} connection_close_t;

			/**
			 * \~russian
			 * @brief Пространство имён функций разбора фреймов QUIC (RFC 9000 §19)
			 *
			 * @details Каждая функция ожидает буфер, указывающий на октет типа фрейма,
			 *          проверяет соответствие типа и возвращает количество потреблённых
			 *          октетов через параметр consumed
			 *
			 * \~english
			 * @brief Namespace of the functions of the parsing of the QUIC frames (RFC 9000 §19)
			 * @details Every function expects a buffer pointing at the octet of the type of the frame,
			 *          checks the correspondence of the type and returns the number of the consumed
			 *          octets through the consumed parameter
			 *
			 * \~
			 */
			namespace parser {
				/**
				 * \~russian
				 * @brief Функция определения типа очередного фрейма в нагрузке
				 *
				 * @param data   буфер расшифрованной нагрузки (начало очередного фрейма)
				 * @param size   доступно байт
				 * @param output определённый тип фрейма (UNKNOWN - тип не распознан)
				 * @return       результат определения (true - в буфере было достаточно байт)
				 *
				 * \~english
				 * @brief Function of determining the type of the next frame in the payload
				 * @param data   buffer of the decrypted payload (the beginning of the next frame)
				 * @param size   bytes available
				 * @param output determined type of the frame (UNKNOWN — the type has not been recognized)
				 * @return       result of the determination (true — there were enough bytes in the buffer)
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ bool type(const uint8_t * data, const size_t size, frame_t & output) noexcept;
				/**
				 * \~russian
				 * @brief Функция разбора последовательности фреймов PADDING (RFC 9000 §19.1)
				 *
				 * @param data     буфер расшифрованной нагрузки (начало фрейма)
				 * @param size     доступно байт
				 * @param consumed количество потреблённых октетов (длина серии PADDING)
				 * @param error    код ошибки транспорта
				 * @return         результат разбора (OK/ERROR)
				 *
				 * \~english
				 * @brief Function of parsing a sequence of the PADDING frames (RFC 9000 §19.1)
				 * @param data     buffer of the decrypted payload (the beginning of the frame)
				 * @param size     bytes available
				 * @param consumed number of the consumed octets (the length of the PADDING series)
				 * @param error    transport error code
				 * @return         result of the parsing (OK/ERROR)
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ status_t padding(const uint8_t * data, const size_t size, size_t & consumed, error_t & error) noexcept;
				/**
				 * \~russian
				 * @brief Функция разбора фрейма PING (RFC 9000 §19.2)
				 *
				 * @param data     буфер расшифрованной нагрузки (начало фрейма)
				 * @param size     доступно байт
				 * @param consumed количество потреблённых октетов
				 * @param error    код ошибки транспорта
				 * @return         результат разбора (OK/ERROR)
				 *
				 * \~english
				 * @brief Function of parsing the PING frame (RFC 9000 §19.2)
				 * @param data     buffer of the decrypted payload (the beginning of the frame)
				 * @param size     bytes available
				 * @param consumed number of the consumed octets
				 * @param error    transport error code
				 * @return         result of the parsing (OK/ERROR)
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ status_t ping(const uint8_t * data, const size_t size, size_t & consumed, error_t & error) noexcept;
				/**
				 * \~russian
				 * @brief Функция разбора фрейма ACK/ACK_ECN (RFC 9000 §19.3)
				 *
				 * @note Диапазоны декодируются в абсолютные номера пакетов с проверкой
				 *       монотонности; нарушение - FRAME_ENCODING_ERROR
				 *
				 * @param data     буфер расшифрованной нагрузки (начало фрейма)
				 * @param size     доступно байт
				 * @param output   разобранный фрейм
				 * @param consumed количество потреблённых октетов
				 * @param error    код ошибки транспорта
				 * @return         результат разбора (OK/ERROR)
				 *
				 * \~english
				 * @brief Function of parsing the ACK/ACK_ECN frame (RFC 9000 §19.3)
				 * @note The ranges are decoded into the absolute packet numbers with a check
				 *       of the monotonicity; a violation is FRAME_ENCODING_ERROR
				 * @param data     buffer of the decrypted payload (the beginning of the frame)
				 * @param size     bytes available
				 * @param output   parsed frame
				 * @param consumed number of the consumed octets
				 * @param error    transport error code
				 * @return         result of the parsing (OK/ERROR)
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ status_t ack(const uint8_t * data, const size_t size, ack_t & output, size_t & consumed, error_t & error) noexcept;
				/**
				 * \~russian
				 * @brief Функция разбора фрейма RESET_STREAM (RFC 9000 §19.4)
				 *
				 * @param data     буфер расшифрованной нагрузки (начало фрейма)
				 * @param size     доступно байт
				 * @param output   разобранный фрейм
				 * @param consumed количество потреблённых октетов
				 * @param error    код ошибки транспорта
				 * @return         результат разбора (OK/ERROR)
				 *
				 * \~english
				 * @brief Function of parsing the RESET_STREAM frame (RFC 9000 §19.4)
				 * @param data     buffer of the decrypted payload (the beginning of the frame)
				 * @param size     bytes available
				 * @param output   parsed frame
				 * @param consumed number of the consumed octets
				 * @param error    transport error code
				 * @return         result of the parsing (OK/ERROR)
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ status_t resetStream(const uint8_t * data, const size_t size, reset_stream_t & output, size_t & consumed, error_t & error) noexcept;
				/**
				 * \~russian
				 * @brief Функция разбора фрейма STOP_SENDING (RFC 9000 §19.5)
				 *
				 * @param data     буфер расшифрованной нагрузки (начало фрейма)
				 * @param size     доступно байт
				 * @param output   разобранный фрейм
				 * @param consumed количество потреблённых октетов
				 * @param error    код ошибки транспорта
				 * @return         результат разбора (OK/ERROR)
				 *
				 * \~english
				 * @brief Function of parsing the STOP_SENDING frame (RFC 9000 §19.5)
				 * @param data     buffer of the decrypted payload (the beginning of the frame)
				 * @param size     bytes available
				 * @param output   parsed frame
				 * @param consumed number of the consumed octets
				 * @param error    transport error code
				 * @return         result of the parsing (OK/ERROR)
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ status_t stopSending(const uint8_t * data, const size_t size, stop_sending_t & output, size_t & consumed, error_t & error) noexcept;
				/**
				 * \~russian
				 * @brief Функция разбора фрейма CRYPTO (RFC 9000 §19.6)
				 *
				 * @param data     буфер расшифрованной нагрузки (начало фрейма)
				 * @param size     доступно байт
				 * @param output   разобранный фрейм
				 * @param consumed количество потреблённых октетов
				 * @param error    код ошибки транспорта
				 * @return         результат разбора (OK/ERROR)
				 *
				 * \~english
				 * @brief Function of parsing the CRYPTO frame (RFC 9000 §19.6)
				 * @param data     buffer of the decrypted payload (the beginning of the frame)
				 * @param size     bytes available
				 * @param output   parsed frame
				 * @param consumed number of the consumed octets
				 * @param error    transport error code
				 * @return         result of the parsing (OK/ERROR)
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ status_t crypto(const uint8_t * data, const size_t size, crypto_t & output, size_t & consumed, error_t & error) noexcept;
				/**
				 * \~russian
				 * @brief Функция разбора фрейма NEW_TOKEN (RFC 9000 §19.7)
				 *
				 * @param data     буфер расшифрованной нагрузки (начало фрейма)
				 * @param size     доступно байт
				 * @param token    токен для будущих соединений (zero-copy в буфер нагрузки)
				 * @param consumed количество потреблённых октетов
				 * @param error    код ошибки транспорта
				 * @return         результат разбора (OK/ERROR)
				 *
				 * \~english
				 * @brief Function of parsing the NEW_TOKEN frame (RFC 9000 §19.7)
				 * @param data     buffer of the decrypted payload (the beginning of the frame)
				 * @param size     bytes available
				 * @param token    token for the future connections (zero-copy into the buffer of the payload)
				 * @param consumed number of the consumed octets
				 * @param error    transport error code
				 * @return         result of the parsing (OK/ERROR)
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ status_t newToken(const uint8_t * data, const size_t size, string_view & token, size_t & consumed, error_t & error) noexcept;
				/**
				 * \~russian
				 * @brief Функция разбора фрейма DATAGRAM обоих вариантов 0x30-0x31 (RFC 9221 §4)
				 *
				 * @note Младший бит типа кодирует наличие поля Length. Без него данные
				 *       занимают весь остаток пакета, поэтому такой фрейм бывает
				 *       в пакете только последним
				 *
				 * @param data     буфер расшифрованной нагрузки (начало фрейма)
				 * @param size     доступно байт
				 * @param output   данные датаграммы приложения (zero-copy в буфер нагрузки)
				 * @param consumed количество потреблённых октетов
				 * @param error    код ошибки транспорта
				 * @return         результат разбора (OK/ERROR)
				 *
				 * \~english
				 * @brief Function of parsing the DATAGRAM frame of both variants 0x30-0x31 (RFC 9221 §4)
				 * @note The low bit of the type encodes the presence of the Length field. Without it the data
				 *       occupies the whole remainder of the packet, therefore such a frame occurs
				 *       in a packet only as the last one
				 * @param data     buffer of the decrypted payload (the beginning of the frame)
				 * @param size     bytes available
				 * @param output   data of the application datagram (zero-copy into the buffer of the payload)
				 * @param consumed number of the consumed octets
				 * @param error    transport error code
				 * @return         result of the parsing (OK/ERROR)
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ status_t datagram(const uint8_t * data, const size_t size, string_view & output, size_t & consumed, error_t & error) noexcept;
				/**
				 * \~russian
				 * @brief Функция разбора фрейма STREAM всех вариантов 0x08-0x0F (RFC 9000 §19.8)
				 *
				 * @param data     буфер расшифрованной нагрузки (начало фрейма)
				 * @param size     доступно байт
				 * @param output   разобранный фрейм
				 * @param consumed количество потреблённых октетов
				 * @param error    код ошибки транспорта
				 * @return         результат разбора (OK/ERROR)
				 *
				 * \~english
				 * @brief Function of parsing the STREAM frame of all the variants 0x08-0x0F (RFC 9000 §19.8)
				 * @param data     buffer of the decrypted payload (the beginning of the frame)
				 * @param size     bytes available
				 * @param output   parsed frame
				 * @param consumed number of the consumed octets
				 * @param error    transport error code
				 * @return         result of the parsing (OK/ERROR)
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ status_t stream(const uint8_t * data, const size_t size, stream_t & output, size_t & consumed, error_t & error) noexcept;
				/**
				 * \~russian
				 * @brief Функция разбора фреймов с одним целочисленным полем
				 *
				 * @details Подходит для MAX_DATA, MAX_STREAM_DATA (второе поле), MAX_STREAMS,
				 *          DATA_BLOCKED, STREAMS_BLOCKED и RETIRE_CONNECTION_ID - используйте
				 *          специализированные функции ниже
				 *
				 * @param data     буфер расшифрованной нагрузки (начало фрейма)
				 * @param size     доступно байт
				 * @param type     ожидаемый тип фрейма
				 * @param value    прочитанное значение поля
				 * @param consumed количество потреблённых октетов
				 * @param error    код ошибки транспорта
				 * @return         результат разбора (OK/ERROR)
				 *
				 * \~english
				 * @brief Function of parsing the frames with a single integer field
				 * @details Suitable for MAX_DATA, MAX_STREAM_DATA (the second field), MAX_STREAMS,
				 *          DATA_BLOCKED, STREAMS_BLOCKED and RETIRE_CONNECTION_ID — use
				 *          the specialized functions below
				 * @param data     buffer of the decrypted payload (the beginning of the frame)
				 * @param size     bytes available
				 * @param type     expected type of the frame
				 * @param value    read value of the field
				 * @param consumed number of the consumed octets
				 * @param error    transport error code
				 * @return         result of the parsing (OK/ERROR)
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ status_t single(const uint8_t * data, const size_t size, const frame_t type, uint64_t & value, size_t & consumed, error_t & error) noexcept;
				/**
				 * \~russian
				 * @brief Функция разбора фреймов с полями идентификатора потока и лимита
				 *
				 * @details Подходит для MAX_STREAM_DATA и STREAM_DATA_BLOCKED
				 *
				 * @param data     буфер расшифрованной нагрузки (начало фрейма)
				 * @param size     доступно байт
				 * @param type     ожидаемый тип фрейма
				 * @param streamId прочитанный идентификатор потока
				 * @param value    прочитанное значение лимита
				 * @param consumed количество потреблённых октетов
				 * @param error    код ошибки транспорта
				 * @return         результат разбора (OK/ERROR)
				 *
				 * \~english
				 * @brief Function of parsing the frames with the fields of a stream identifier and of a limit
				 * @details Suitable for MAX_STREAM_DATA and STREAM_DATA_BLOCKED
				 * @param data     buffer of the decrypted payload (the beginning of the frame)
				 * @param size     bytes available
				 * @param type     expected type of the frame
				 * @param streamId read stream identifier
				 * @param value    read value of the limit
				 * @param consumed number of the consumed octets
				 * @param error    transport error code
				 * @return         result of the parsing (OK/ERROR)
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ status_t pair(const uint8_t * data, const size_t size, const frame_t type, uint64_t & streamId, uint64_t & value, size_t & consumed, error_t & error) noexcept;
				/**
				 * \~russian
				 * @brief Функция разбора фрейма NEW_CONNECTION_ID (RFC 9000 §19.15)
				 *
				 * @param data     буфер расшифрованной нагрузки (начало фрейма)
				 * @param size     доступно байт
				 * @param output   разобранный фрейм
				 * @param consumed количество потреблённых октетов
				 * @param error    код ошибки транспорта
				 * @return         результат разбора (OK/ERROR)
				 *
				 * \~english
				 * @brief Function of parsing the NEW_CONNECTION_ID frame (RFC 9000 §19.15)
				 * @param data     buffer of the decrypted payload (the beginning of the frame)
				 * @param size     bytes available
				 * @param output   parsed frame
				 * @param consumed number of the consumed octets
				 * @param error    transport error code
				 * @return         result of the parsing (OK/ERROR)
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ status_t newConnectionId(const uint8_t * data, const size_t size, new_connection_id_t & output, size_t & consumed, error_t & error) noexcept;
				/**
				 * \~russian
				 * @brief Функция разбора фреймов PATH_CHALLENGE/PATH_RESPONSE (RFC 9000 §19.17/§19.18)
				 *
				 * @param data     буфер расшифрованной нагрузки (начало фрейма)
				 * @param size     доступно байт
				 * @param type     ожидаемый тип фрейма
				 * @param output   извлечённые данные проверки пути (8 октетов)
				 * @param consumed количество потреблённых октетов
				 * @param error    код ошибки транспорта
				 * @return         результат разбора (OK/ERROR)
				 *
				 * \~english
				 * @brief Function of parsing the PATH_CHALLENGE/PATH_RESPONSE frames (RFC 9000 §19.17/§19.18)
				 * @param data     buffer of the decrypted payload (the beginning of the frame)
				 * @param size     bytes available
				 * @param type     expected type of the frame
				 * @param output   extracted data of the path validation (8 octets)
				 * @param consumed number of the consumed octets
				 * @param error    transport error code
				 * @return         result of the parsing (OK/ERROR)
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ status_t path(const uint8_t * data, const size_t size, const frame_t type, uint8_t output[proto::PATH_DATA_SIZE], size_t & consumed, error_t & error) noexcept;
				/**
				 * \~russian
				 * @brief Функция разбора фрейма CONNECTION_CLOSE обоих вариантов (RFC 9000 §19.19)
				 *
				 * @param data     буфер расшифрованной нагрузки (начало фрейма)
				 * @param size     доступно байт
				 * @param output   разобранный фрейм
				 * @param consumed количество потреблённых октетов
				 * @param error    код ошибки транспорта
				 * @return         результат разбора (OK/ERROR)
				 *
				 * \~english
				 * @brief Function of parsing the CONNECTION_CLOSE frame of both variants (RFC 9000 §19.19)
				 * @param data     buffer of the decrypted payload (the beginning of the frame)
				 * @param size     bytes available
				 * @param output   parsed frame
				 * @param consumed number of the consumed octets
				 * @param error    transport error code
				 * @return         result of the parsing (OK/ERROR)
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ status_t connectionClose(const uint8_t * data, const size_t size, connection_close_t & output, size_t & consumed, error_t & error) noexcept;
				/**
				 * \~russian
				 * @brief Функция разбора фрейма HANDSHAKE_DONE (RFC 9000 §19.20)
				 *
				 * @param data     буфер расшифрованной нагрузки (начало фрейма)
				 * @param size     доступно байт
				 * @param consumed количество потреблённых октетов
				 * @param error    код ошибки транспорта
				 * @return         результат разбора (OK/ERROR)
				 *
				 * \~english
				 * @brief Function of parsing the HANDSHAKE_DONE frame (RFC 9000 §19.20)
				 * @param data     buffer of the decrypted payload (the beginning of the frame)
				 * @param size     bytes available
				 * @param consumed number of the consumed octets
				 * @param error    transport error code
				 * @return         result of the parsing (OK/ERROR)
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ status_t handshakeDone(const uint8_t * data, const size_t size, size_t & consumed, error_t & error) noexcept;
			};

			/**
			 * \~russian
			 * @brief Пространство имён функций сборки фреймов QUIC (RFC 9000 §19)
			 *
			 * \~english
			 * @brief Namespace of the functions of the assembly of the QUIC frames (RFC 9000 §19)
			 *
			 * \~
			 */
			namespace serialize {
				/**
				 * \~russian
				 * @brief Функция сборки последовательности фреймов PADDING (заполнение дописывается в output)
				 *
				 * @param output выходной буфер нагрузки пакета
				 * @param count  количество октетов заполнения
				 *
				 * \~english
				 * @brief Function of assembling a sequence of the PADDING frames (the padding is appended into output)
				 * @param output output buffer of the payload of the packet
				 * @param count  number of the octets of the padding
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ void padding(string & output, const size_t count) noexcept;
				/**
				 * \~russian
				 * @brief Функция сборки фрейма PING (фрейм дописывается в output)
				 *
				 * @param output выходной буфер нагрузки пакета
				 *
				 * \~english
				 * @brief Function of assembling the PING frame (the frame is appended into output)
				 * @param output output buffer of the payload of the packet
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ void ping(string & output) noexcept;
				/**
				 * \~russian
				 * @brief Функция сборки фрейма ACK/ACK_ECN (фрейм дописывается в output)
				 *
				 * @note Диапазоны должны идти в порядке убывания и не пересекаться
				 *
				 * @param output выходной буфер нагрузки пакета
				 * @param frame  фрейм подтверждения приёма пакетов
				 * @return       результат сборки (false - некорректные диапазоны)
				 *
				 * \~english
				 * @brief Function of assembling the ACK/ACK_ECN frame (the frame is appended into output)
				 * @note The ranges must go in the descending order and must not intersect
				 * @param output output buffer of the payload of the packet
				 * @param frame  frame of the acknowledgement of the reception of the packets
				 * @return       result of the assembly (false — the ranges are incorrect)
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ bool ack(string & output, const ack_t & frame) noexcept;
				/**
				 * \~russian
				 * @brief Функция сборки фрейма RESET_STREAM (фрейм дописывается в output)
				 *
				 * @param output    выходной буфер нагрузки пакета
				 * @param streamId  идентификатор потока
				 * @param code      код ошибки приложения
				 * @param finalSize финальный размер потока в октетах
				 *
				 * \~english
				 * @brief Function of assembling the RESET_STREAM frame (the frame is appended into output)
				 * @param output    output buffer of the payload of the packet
				 * @param streamId  stream identifier
				 * @param code      error code of the application
				 * @param finalSize final size of the stream in octets
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ void resetStream(string & output, const uint64_t streamId, const uint64_t code, const uint64_t finalSize) noexcept;
				/**
				 * \~russian
				 * @brief Функция сборки фрейма STOP_SENDING (фрейм дописывается в output)
				 *
				 * @param output   выходной буфер нагрузки пакета
				 * @param streamId идентификатор потока
				 * @param code     код ошибки приложения
				 *
				 * \~english
				 * @brief Function of assembling the STOP_SENDING frame (the frame is appended into output)
				 * @param output   output buffer of the payload of the packet
				 * @param streamId stream identifier
				 * @param code     error code of the application
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ void stopSending(string & output, const uint64_t streamId, const uint64_t code) noexcept;
				/**
				 * \~russian
				 * @brief Функция сборки фрейма CRYPTO (фрейм дописывается в output)
				 *
				 * @param output выходной буфер нагрузки пакета
				 * @param offset смещение данных в потоке криптографического хендшейка
				 * @param data   данные криптографического хендшейка
				 *
				 * \~english
				 * @brief Function of assembling the CRYPTO frame (the frame is appended into output)
				 * @param output output buffer of the payload of the packet
				 * @param offset offset of the data in the stream of the cryptographic handshake
				 * @param data   data of the cryptographic handshake
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ void crypto(string & output, const uint64_t offset, string_view data) noexcept;
				/**
				 * \~russian
				 * @brief Функция сборки фрейма NEW_TOKEN (фрейм дописывается в output)
				 *
				 * @param output выходной буфер нагрузки пакета
				 * @param token  токен для будущих соединений клиента (не пустой)
				 * @return       результат сборки (false - пустой токен запрещён)
				 *
				 * \~english
				 * @brief Function of assembling the NEW_TOKEN frame (the frame is appended into output)
				 * @param output output buffer of the payload of the packet
				 * @param token  token for the future connections of the client (not empty)
				 * @return       result of the assembly (false — an empty token is prohibited)
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ bool newToken(string & output, string_view token) noexcept;
				/**
				 * \~russian
				 * @brief Функция сборки фрейма DATAGRAM с полем Length (фрейм дописывается в output)
				 *
				 * @note Собирается вариант 0x31 с явной длиной: без неё фрейм занимает
				 *       весь остаток пакета и коалесцирование с другими фреймами
				 *       становится невозможным (RFC 9221 §4)
				 *
				 * @param output выходной буфер нагрузки пакета
				 * @param data   данные датаграммы приложения
				 *
				 * \~english
				 * @brief Function of assembling the DATAGRAM frame with the Length field (the frame is appended into output)
				 * @note The variant 0x31 with an explicit length is assembled: without it the frame occupies
				 *       the whole remainder of the packet and the coalescing with the other frames
				 *       becomes impossible (RFC 9221 §4)
				 * @param output output buffer of the payload of the packet
				 * @param data   data of the application datagram
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ void datagram(string & output, string_view data) noexcept;
				/**
				 * \~russian
				 * @brief Функция сборки фрейма STREAM (фрейм дописывается в output)
				 *
				 * @note Поле Offset кодируется только при ненулевом смещении,
				 *       поле Length кодируется всегда (фрейм не обязан быть последним в пакете)
				 *
				 * @param output   выходной буфер нагрузки пакета
				 * @param streamId идентификатор потока
				 * @param offset   смещение данных в потоке
				 * @param data     данные потока приложения
				 * @param fin      флаг завершения потока (FIN)
				 *
				 * \~english
				 * @brief Function of assembling the STREAM frame (the frame is appended into output)
				 * @note The Offset field is encoded only at a non-zero offset,
				 *       the Length field is encoded always (the frame is not obliged to be the last one in the packet)
				 * @param output   output buffer of the payload of the packet
				 * @param streamId stream identifier
				 * @param offset   offset of the data in the stream
				 * @param data     data of the application stream
				 * @param fin      flag of the termination of the stream (FIN)
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ void stream(string & output, const uint64_t streamId, const uint64_t offset, string_view data, const bool fin) noexcept;
				/**
				 * \~russian
				 * @brief Функция сборки фреймов с одним целочисленным полем (фрейм дописывается в output)
				 *
				 * @details Подходит для MAX_DATA, MAX_STREAMS, DATA_BLOCKED,
				 *          STREAMS_BLOCKED и RETIRE_CONNECTION_ID
				 *
				 * @param output выходной буфер нагрузки пакета
				 * @param type   тип фрейма
				 * @param value  значение целочисленного поля
				 *
				 * \~english
				 * @brief Function of assembling the frames with a single integer field (the frame is appended into output)
				 * @details Suitable for MAX_DATA, MAX_STREAMS, DATA_BLOCKED,
				 *          STREAMS_BLOCKED and RETIRE_CONNECTION_ID
				 * @param output output buffer of the payload of the packet
				 * @param type   type of the frame
				 * @param value  value of the integer field
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ void single(string & output, const frame_t type, const uint64_t value) noexcept;
				/**
				 * \~russian
				 * @brief Функция сборки фреймов с полями идентификатора потока и лимита (фрейм дописывается в output)
				 *
				 * @details Подходит для MAX_STREAM_DATA и STREAM_DATA_BLOCKED
				 *
				 * @param output   выходной буфер нагрузки пакета
				 * @param type     тип фрейма
				 * @param streamId идентификатор потока
				 * @param value    значение лимита
				 *
				 * \~english
				 * @brief Function of assembling the frames with the fields of a stream identifier and of a limit (the frame is appended into output)
				 * @details Suitable for MAX_STREAM_DATA and STREAM_DATA_BLOCKED
				 * @param output   output buffer of the payload of the packet
				 * @param type     type of the frame
				 * @param streamId stream identifier
				 * @param value    value of the limit
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ void pair(string & output, const frame_t type, const uint64_t streamId, const uint64_t value) noexcept;
				/**
				 * \~russian
				 * @brief Функция сборки фрейма NEW_CONNECTION_ID (фрейм дописывается в output)
				 *
				 * @param output выходной буфер нагрузки пакета
				 * @param frame  фрейм анонса нового идентификатора соединения
				 * @return       результат сборки (false - некорректная длина идентификатора)
				 *
				 * \~english
				 * @brief Function of assembling the NEW_CONNECTION_ID frame (the frame is appended into output)
				 * @param output output buffer of the payload of the packet
				 * @param frame  frame of the announcement of a new connection identifier
				 * @return       result of the assembly (false — the length of the identifier is incorrect)
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ bool newConnectionId(string & output, const new_connection_id_t & frame) noexcept;
				/**
				 * \~russian
				 * @brief Функция сборки фреймов PATH_CHALLENGE/PATH_RESPONSE (фрейм дописывается в output)
				 *
				 * @param output выходной буфер нагрузки пакета
				 * @param type   тип фрейма (PATH_CHALLENGE или PATH_RESPONSE)
				 * @param data   данные проверки пути (8 октетов)
				 *
				 * \~english
				 * @brief Function of assembling the PATH_CHALLENGE/PATH_RESPONSE frames (the frame is appended into output)
				 * @param output output buffer of the payload of the packet
				 * @param type   type of the frame (PATH_CHALLENGE or PATH_RESPONSE)
				 * @param data   data of the path validation (8 octets)
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ void path(string & output, const frame_t type, const uint8_t data[proto::PATH_DATA_SIZE]) noexcept;
				/**
				 * \~russian
				 * @brief Функция сборки фрейма CONNECTION_CLOSE (фрейм дописывается в output)
				 *
				 * @param output    выходной буфер нагрузки пакета
				 * @param code      код ошибки (транспорта или приложения)
				 * @param frameType тип фрейма, вызвавшего ошибку (игнорируется для ошибки приложения)
				 * @param reason    человекочитаемая причина завершения
				 * @param app       флаг ошибки приложения (тип фрейма 0x1D)
				 *
				 * \~english
				 * @brief Function of assembling the CONNECTION_CLOSE frame (the frame is appended into output)
				 * @param output    output buffer of the payload of the packet
				 * @param code      error code (of the transport or of the application)
				 * @param frameType type of the frame that has caused the error (ignored for an error of the application)
				 * @param reason    human-readable reason of the termination
				 * @param app       flag of an error of the application (the frame type 0x1D)
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ void connectionClose(string & output, const uint64_t code, const uint64_t frameType, string_view reason, const bool app) noexcept;
				/**
				 * \~russian
				 * @brief Функция сборки фрейма HANDSHAKE_DONE (фрейм дописывается в output)
				 *
				 * @param output выходной буфер нагрузки пакета
				 *
				 * \~english
				 * @brief Function of assembling the HANDSHAKE_DONE frame (the frame is appended into output)
				 * @param output output buffer of the payload of the packet
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ void handshakeDone(string & output) noexcept;
			};
		};
	};
};

#endif // __AWH_PROTO_QUIC_FRAME__
