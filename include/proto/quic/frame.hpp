/**
 * @file: frame.hpp
 * @date: 2026-07-21
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
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
#include "varint.hpp"
#include "../../sys/global.hpp"

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Пространство имён транспортного протокола QUIC
	 *
	 */
	namespace quic {
		/**
		 * @brief Пространство имён слоя фреймов QUIC (RFC 9000 §19): разбор и сборка фреймов
		 *
		 * @details Фреймы лежат в расшифрованной нагрузке пакета друг за другом без
		 *          разделителей. Разбор zero-copy: данные потоков отдаются как string_view
		 *          с указателем в буфер расшифрованной нагрузки. Каждая функция разбора
		 *          читает тип фрейма сама и возвращает количество потреблённых октетов.
		 *          Слой не хранит состояния соединения - это чистые функции над байтами.
		 */
		namespace frame {
			/**
			 * @brief Структура диапазона подтверждённых номеров пакетов [low, high]
			 *
			 */
			typedef struct __AWH_SHARED_EXPORT__ Range {
				// Наименьший номер пакета диапазона
				uint64_t low;
				// Наибольший номер пакета диапазона
				uint64_t high;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Range() noexcept;
			} range_t;
			/**
			 * @brief Структура фрейма ACK (RFC 9000 §19.3), диапазоны декодированы в абсолютные номера
			 *
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
				 * @brief Конструктор
				 *
				 */
				explicit Ack() noexcept;
			} ack_t;
			/**
			 * @brief Структура фрейма RESET_STREAM (RFC 9000 §19.4)
			 *
			 */
			typedef struct __AWH_SHARED_EXPORT__ Reset_Stream {
				// Идентификатор потока
				uint64_t streamId;
				// Код ошибки приложения
				uint64_t code;
				// Финальный размер потока в октетах
				uint64_t finalSize;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Reset_Stream() noexcept;
			} reset_stream_t;
			/**
			 * @brief Структура фрейма STOP_SENDING (RFC 9000 §19.5)
			 *
			 */
			typedef struct __AWH_SHARED_EXPORT__ Stop_Sending {
				// Идентификатор потока
				uint64_t streamId;
				// Код ошибки приложения
				uint64_t code;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Stop_Sending() noexcept;
			} stop_sending_t;
			/**
			 * @brief Структура фрейма CRYPTO (RFC 9000 §19.6)
			 *
			 */
			typedef struct __AWH_SHARED_EXPORT__ Crypto {
				// Смещение данных в потоке криптографического хендшейка
				uint64_t offset;
				// Данные криптографического хендшейка (zero-copy в буфер нагрузки)
				string_view data;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Crypto() noexcept;
			} crypto_t;
			/**
			 * @brief Структура фрейма STREAM (RFC 9000 §19.8), поля Offset/Length декодированы
			 *
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
				 * @brief Конструктор
				 *
				 */
				explicit Stream() noexcept;
			} stream_t;
			/**
			 * @brief Структура фрейма NEW_CONNECTION_ID (RFC 9000 §19.15)
			 *
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
				 * @brief Конструктор
				 *
				 */
				explicit New_Connection_Id() noexcept;
			} new_connection_id_t;
			/**
			 * @brief Структура фрейма CONNECTION_CLOSE (RFC 9000 §19.19)
			 *
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
				 * @brief Конструктор
				 *
				 */
				explicit Connection_Close() noexcept;
			} connection_close_t;

			/**
			 * @brief Пространство имён функций разбора фреймов QUIC (RFC 9000 §19)
			 *
			 * @details Каждая функция ожидает буфер, указывающий на октет типа фрейма,
			 *          проверяет соответствие типа и возвращает количество потреблённых
			 *          октетов через параметр consumed
			 */
			namespace parser {
				/**
				 * @brief Функция определения типа очередного фрейма в нагрузке
				 *
				 * @param data   буфер расшифрованной нагрузки (начало очередного фрейма)
				 * @param size   доступно байт
				 * @param output определённый тип фрейма (UNKNOWN - тип не распознан)
				 * @return       результат определения (true - в буфере было достаточно байт)
				 */
				__AWH_SHARED_EXPORT__ bool type(const uint8_t * data, const size_t size, frame_t & output) noexcept;
				/**
				 * @brief Функция разбора последовательности фреймов PADDING (RFC 9000 §19.1)
				 *
				 * @param data     буфер расшифрованной нагрузки (начало фрейма)
				 * @param size     доступно байт
				 * @param consumed количество потреблённых октетов (длина серии PADDING)
				 * @param error    код ошибки транспорта
				 * @return         результат разбора (OK/ERROR)
				 */
				__AWH_SHARED_EXPORT__ status_t padding(const uint8_t * data, const size_t size, size_t & consumed, error_t & error) noexcept;
				/**
				 * @brief Функция разбора фрейма PING (RFC 9000 §19.2)
				 *
				 * @param data     буфер расшифрованной нагрузки (начало фрейма)
				 * @param size     доступно байт
				 * @param consumed количество потреблённых октетов
				 * @param error    код ошибки транспорта
				 * @return         результат разбора (OK/ERROR)
				 */
				__AWH_SHARED_EXPORT__ status_t ping(const uint8_t * data, const size_t size, size_t & consumed, error_t & error) noexcept;
				/**
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
				 */
				__AWH_SHARED_EXPORT__ status_t ack(const uint8_t * data, const size_t size, ack_t & output, size_t & consumed, error_t & error) noexcept;
				/**
				 * @brief Функция разбора фрейма RESET_STREAM (RFC 9000 §19.4)
				 *
				 * @param data     буфер расшифрованной нагрузки (начало фрейма)
				 * @param size     доступно байт
				 * @param output   разобранный фрейм
				 * @param consumed количество потреблённых октетов
				 * @param error    код ошибки транспорта
				 * @return         результат разбора (OK/ERROR)
				 */
				__AWH_SHARED_EXPORT__ status_t resetStream(const uint8_t * data, const size_t size, reset_stream_t & output, size_t & consumed, error_t & error) noexcept;
				/**
				 * @brief Функция разбора фрейма STOP_SENDING (RFC 9000 §19.5)
				 *
				 * @param data     буфер расшифрованной нагрузки (начало фрейма)
				 * @param size     доступно байт
				 * @param output   разобранный фрейм
				 * @param consumed количество потреблённых октетов
				 * @param error    код ошибки транспорта
				 * @return         результат разбора (OK/ERROR)
				 */
				__AWH_SHARED_EXPORT__ status_t stopSending(const uint8_t * data, const size_t size, stop_sending_t & output, size_t & consumed, error_t & error) noexcept;
				/**
				 * @brief Функция разбора фрейма CRYPTO (RFC 9000 §19.6)
				 *
				 * @param data     буфер расшифрованной нагрузки (начало фрейма)
				 * @param size     доступно байт
				 * @param output   разобранный фрейм
				 * @param consumed количество потреблённых октетов
				 * @param error    код ошибки транспорта
				 * @return         результат разбора (OK/ERROR)
				 */
				__AWH_SHARED_EXPORT__ status_t crypto(const uint8_t * data, const size_t size, crypto_t & output, size_t & consumed, error_t & error) noexcept;
				/**
				 * @brief Функция разбора фрейма NEW_TOKEN (RFC 9000 §19.7)
				 *
				 * @param data     буфер расшифрованной нагрузки (начало фрейма)
				 * @param size     доступно байт
				 * @param token    токен для будущих соединений (zero-copy в буфер нагрузки)
				 * @param consumed количество потреблённых октетов
				 * @param error    код ошибки транспорта
				 * @return         результат разбора (OK/ERROR)
				 */
				__AWH_SHARED_EXPORT__ status_t newToken(const uint8_t * data, const size_t size, string_view & token, size_t & consumed, error_t & error) noexcept;
				/**
				 * @brief Функция разбора фрейма STREAM всех вариантов 0x08-0x0F (RFC 9000 §19.8)
				 *
				 * @param data     буфер расшифрованной нагрузки (начало фрейма)
				 * @param size     доступно байт
				 * @param output   разобранный фрейм
				 * @param consumed количество потреблённых октетов
				 * @param error    код ошибки транспорта
				 * @return         результат разбора (OK/ERROR)
				 */
				__AWH_SHARED_EXPORT__ status_t stream(const uint8_t * data, const size_t size, stream_t & output, size_t & consumed, error_t & error) noexcept;
				/**
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
				 */
				__AWH_SHARED_EXPORT__ status_t single(const uint8_t * data, const size_t size, const frame_t type, uint64_t & value, size_t & consumed, error_t & error) noexcept;
				/**
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
				 */
				__AWH_SHARED_EXPORT__ status_t pair(const uint8_t * data, const size_t size, const frame_t type, uint64_t & streamId, uint64_t & value, size_t & consumed, error_t & error) noexcept;
				/**
				 * @brief Функция разбора фрейма NEW_CONNECTION_ID (RFC 9000 §19.15)
				 *
				 * @param data     буфер расшифрованной нагрузки (начало фрейма)
				 * @param size     доступно байт
				 * @param output   разобранный фрейм
				 * @param consumed количество потреблённых октетов
				 * @param error    код ошибки транспорта
				 * @return         результат разбора (OK/ERROR)
				 */
				__AWH_SHARED_EXPORT__ status_t newConnectionId(const uint8_t * data, const size_t size, new_connection_id_t & output, size_t & consumed, error_t & error) noexcept;
				/**
				 * @brief Функция разбора фреймов PATH_CHALLENGE/PATH_RESPONSE (RFC 9000 §19.17/§19.18)
				 *
				 * @param data     буфер расшифрованной нагрузки (начало фрейма)
				 * @param size     доступно байт
				 * @param type     ожидаемый тип фрейма
				 * @param output   извлечённые данные проверки пути (8 октетов)
				 * @param consumed количество потреблённых октетов
				 * @param error    код ошибки транспорта
				 * @return         результат разбора (OK/ERROR)
				 */
				__AWH_SHARED_EXPORT__ status_t path(const uint8_t * data, const size_t size, const frame_t type, uint8_t output[proto::PATH_DATA_SIZE], size_t & consumed, error_t & error) noexcept;
				/**
				 * @brief Функция разбора фрейма CONNECTION_CLOSE обоих вариантов (RFC 9000 §19.19)
				 *
				 * @param data     буфер расшифрованной нагрузки (начало фрейма)
				 * @param size     доступно байт
				 * @param output   разобранный фрейм
				 * @param consumed количество потреблённых октетов
				 * @param error    код ошибки транспорта
				 * @return         результат разбора (OK/ERROR)
				 */
				__AWH_SHARED_EXPORT__ status_t connectionClose(const uint8_t * data, const size_t size, connection_close_t & output, size_t & consumed, error_t & error) noexcept;
				/**
				 * @brief Функция разбора фрейма HANDSHAKE_DONE (RFC 9000 §19.20)
				 *
				 * @param data     буфер расшифрованной нагрузки (начало фрейма)
				 * @param size     доступно байт
				 * @param consumed количество потреблённых октетов
				 * @param error    код ошибки транспорта
				 * @return         результат разбора (OK/ERROR)
				 */
				__AWH_SHARED_EXPORT__ status_t handshakeDone(const uint8_t * data, const size_t size, size_t & consumed, error_t & error) noexcept;
			};

			/**
			 * @brief Пространство имён функций сборки фреймов QUIC (RFC 9000 §19)
			 *
			 */
			namespace serialize {
				/**
				 * @brief Функция сборки последовательности фреймов PADDING (заполнение дописывается в output)
				 *
				 * @param output выходной буфер нагрузки пакета
				 * @param count  количество октетов заполнения
				 */
				__AWH_SHARED_EXPORT__ void padding(string & output, const size_t count) noexcept;
				/**
				 * @brief Функция сборки фрейма PING (фрейм дописывается в output)
				 *
				 * @param output выходной буфер нагрузки пакета
				 */
				__AWH_SHARED_EXPORT__ void ping(string & output) noexcept;
				/**
				 * @brief Функция сборки фрейма ACK/ACK_ECN (фрейм дописывается в output)
				 *
				 * @note Диапазоны должны идти в порядке убывания и не пересекаться
				 *
				 * @param output выходной буфер нагрузки пакета
				 * @param frame  фрейм подтверждения приёма пакетов
				 * @return       результат сборки (false - некорректные диапазоны)
				 */
				__AWH_SHARED_EXPORT__ bool ack(string & output, const ack_t & frame) noexcept;
				/**
				 * @brief Функция сборки фрейма RESET_STREAM (фрейм дописывается в output)
				 *
				 * @param output    выходной буфер нагрузки пакета
				 * @param streamId  идентификатор потока
				 * @param code      код ошибки приложения
				 * @param finalSize финальный размер потока в октетах
				 */
				__AWH_SHARED_EXPORT__ void resetStream(string & output, const uint64_t streamId, const uint64_t code, const uint64_t finalSize) noexcept;
				/**
				 * @brief Функция сборки фрейма STOP_SENDING (фрейм дописывается в output)
				 *
				 * @param output   выходной буфер нагрузки пакета
				 * @param streamId идентификатор потока
				 * @param code     код ошибки приложения
				 */
				__AWH_SHARED_EXPORT__ void stopSending(string & output, const uint64_t streamId, const uint64_t code) noexcept;
				/**
				 * @brief Функция сборки фрейма CRYPTO (фрейм дописывается в output)
				 *
				 * @param output выходной буфер нагрузки пакета
				 * @param offset смещение данных в потоке криптографического хендшейка
				 * @param data   данные криптографического хендшейка
				 */
				__AWH_SHARED_EXPORT__ void crypto(string & output, const uint64_t offset, string_view data) noexcept;
				/**
				 * @brief Функция сборки фрейма NEW_TOKEN (фрейм дописывается в output)
				 *
				 * @param output выходной буфер нагрузки пакета
				 * @param token  токен для будущих соединений клиента (не пустой)
				 * @return       результат сборки (false - пустой токен запрещён)
				 */
				__AWH_SHARED_EXPORT__ bool newToken(string & output, string_view token) noexcept;
				/**
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
				 */
				__AWH_SHARED_EXPORT__ void stream(string & output, const uint64_t streamId, const uint64_t offset, string_view data, const bool fin) noexcept;
				/**
				 * @brief Функция сборки фреймов с одним целочисленным полем (фрейм дописывается в output)
				 *
				 * @details Подходит для MAX_DATA, MAX_STREAMS, DATA_BLOCKED,
				 *          STREAMS_BLOCKED и RETIRE_CONNECTION_ID
				 *
				 * @param output выходной буфер нагрузки пакета
				 * @param type   тип фрейма
				 * @param value  значение целочисленного поля
				 */
				__AWH_SHARED_EXPORT__ void single(string & output, const frame_t type, const uint64_t value) noexcept;
				/**
				 * @brief Функция сборки фреймов с полями идентификатора потока и лимита (фрейм дописывается в output)
				 *
				 * @details Подходит для MAX_STREAM_DATA и STREAM_DATA_BLOCKED
				 *
				 * @param output   выходной буфер нагрузки пакета
				 * @param type     тип фрейма
				 * @param streamId идентификатор потока
				 * @param value    значение лимита
				 */
				__AWH_SHARED_EXPORT__ void pair(string & output, const frame_t type, const uint64_t streamId, const uint64_t value) noexcept;
				/**
				 * @brief Функция сборки фрейма NEW_CONNECTION_ID (фрейм дописывается в output)
				 *
				 * @param output выходной буфер нагрузки пакета
				 * @param frame  фрейм анонса нового идентификатора соединения
				 * @return       результат сборки (false - некорректная длина идентификатора)
				 */
				__AWH_SHARED_EXPORT__ bool newConnectionId(string & output, const new_connection_id_t & frame) noexcept;
				/**
				 * @brief Функция сборки фреймов PATH_CHALLENGE/PATH_RESPONSE (фрейм дописывается в output)
				 *
				 * @param output выходной буфер нагрузки пакета
				 * @param type   тип фрейма (PATH_CHALLENGE или PATH_RESPONSE)
				 * @param data   данные проверки пути (8 октетов)
				 */
				__AWH_SHARED_EXPORT__ void path(string & output, const frame_t type, const uint8_t data[proto::PATH_DATA_SIZE]) noexcept;
				/**
				 * @brief Функция сборки фрейма CONNECTION_CLOSE (фрейм дописывается в output)
				 *
				 * @param output    выходной буфер нагрузки пакета
				 * @param code      код ошибки (транспорта или приложения)
				 * @param frameType тип фрейма, вызвавшего ошибку (игнорируется для ошибки приложения)
				 * @param reason    человекочитаемая причина завершения
				 * @param app       флаг ошибки приложения (тип фрейма 0x1D)
				 */
				__AWH_SHARED_EXPORT__ void connectionClose(string & output, const uint64_t code, const uint64_t frameType, string_view reason, const bool app) noexcept;
				/**
				 * @brief Функция сборки фрейма HANDSHAKE_DONE (фрейм дописывается в output)
				 *
				 * @param output выходной буфер нагрузки пакета
				 */
				__AWH_SHARED_EXPORT__ void handshakeDone(string & output) noexcept;
			};
		};
	};
};

#endif // __AWH_PROTO_QUIC_FRAME__
