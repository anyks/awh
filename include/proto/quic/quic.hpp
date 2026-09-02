/**
 * @file quic.hpp
 * @date 2026-07-21
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
 * @brief Заголовочный файл общих типов протокола QUIC (RFC 9000) — перечисления типов пакетов, уровней шифрования,
 *        типов фреймов, кодов ошибок и статусов, структура идентификатора соединения и константы протокола
 *
 * \~english
 * @brief Header file of the common types of the QUIC protocol (RFC 9000) — the enumerations of the types of the packets, of the encryption levels,
 *        of the types of the frames, of the error codes and of the statuses, the structure of a connection identifier and the constants of the protocol
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_PROTO_QUIC__
#define __AWH_PROTO_QUIC__

/**
 * Стандартные заголовочные файлы
 */
#include <cstddef>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочный файл проекта
 */
#include "../../net/net.hpp"
#include "../../sys/global.hpp"

/**
 * Подавляем системные макросы, занявшие имена членов перечислений ниже:
 * DELETE и ERROR у MS Windows, CS и PRIVATE у Sun Solaris, CS5 у termios.
 * Имена снимаются лишь на время объявлений - возврат в конце файла
 */
#include "../../sys/macro/suppress.hpp"

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
	 * @brief Пространство имён транспортного протокола QUIC (RFC 9000/9001)
	 *
	 * @details Содержит базовые типы и константы протокола, общие для слоя целых чисел
	 *          переменной длины (varint.hpp), слоя пакетов (packet.hpp) и слоя фреймов
	 *          (frame.hpp). Логики здесь нет — только перечисления, константы протокола
	 *          и объявления функций человекочитаемых названий.
	 *
	 * \~english
	 * @brief Namespace of the QUIC transport protocol (RFC 9000/9001)
	 * @details Contains the base types and the constants of the protocol, common to the layer of the variable-length
	 *          integers (varint.hpp), to the layer of the packets (packet.hpp) and to the layer of the frames
	 *          (frame.hpp). There is no logic here — only the enumerations, the constants of the protocol
	 *          and the declarations of the functions of the human-readable names.
	 *
	 * \~
	 */
	namespace quic {
		/**
		 * \~russian
		 * @brief Пространство имён констант протокола (RFC 9000)
		 *
		 * \~english
		 * @brief Namespace of the constants of the protocol (RFC 9000)
		 *
		 * \~
		 */
		namespace proto {
			/**
			 * \~russian
			 * @brief Номер версии QUIC v1 (RFC 9000)
			 *
			 * \~english
			 * @brief Version number of QUIC v1 (RFC 9000)
			 *
			 * \~
			 */
			static constexpr uint32_t VERSION_1 = 0x00000001;
			/**
			 * \~russian
			 * @brief Номер версии пакета Version Negotiation (RFC 9000 §17.2.1)
			 *
			 * \~english
			 * @brief Version number of a Version Negotiation packet (RFC 9000 §17.2.1)
			 *
			 * \~
			 */
			static constexpr uint32_t VERSION_NEGOTIATION = 0x00000000;
			/**
			 * \~russian
			 * @brief Максимальная длина идентификатора соединения в QUIC v1 (RFC 9000 §17.2)
			 *
			 * \~english
			 * @brief Maximum length of a connection identifier in QUIC v1 (RFC 9000 §17.2)
			 *
			 * \~
			 */
			static constexpr size_t MAX_CID_SIZE = 20;
			/**
			 * \~russian
			 * @brief Размер тега целостности пакета Retry (RFC 9001 §5.8)
			 *
			 * \~english
			 * @brief Size of the integrity tag of a Retry packet (RFC 9001 §5.8)
			 *
			 * \~
			 */
			static constexpr size_t RETRY_TAG_SIZE = 16;
			/**
			 * \~russian
			 * @brief Размер данных фреймов PATH_CHALLENGE/PATH_RESPONSE (RFC 9000 §19.17/§19.18)
			 *
			 * \~english
			 * @brief Size of the data of the PATH_CHALLENGE/PATH_RESPONSE frames (RFC 9000 §19.17/§19.18)
			 *
			 * \~
			 */
			static constexpr size_t PATH_DATA_SIZE = 8;
			/**
			 * \~russian
			 * @brief Размер токена сброса без сохранения состояния (RFC 9000 §10.3)
			 *
			 * \~english
			 * @brief Size of a stateless reset token (RFC 9000 §10.3)
			 *
			 * \~
			 */
			static constexpr size_t RESET_TOKEN_SIZE = 16;
			/**
			 * \~russian
			 * @brief Минимальный размер UDP-датаграммы с пакетом Initial клиента (RFC 9000 §14.1)
			 *
			 * \~english
			 * @brief Minimum size of a UDP datagram with an Initial packet of a client (RFC 9000 §14.1)
			 *
			 * \~
			 */
			static constexpr size_t MIN_INITIAL_SIZE = 1200;
			/**
			 * \~russian
			 * @brief Максимальный размер номера пакета в закодированном виде (RFC 9000 §17.1)
			 *
			 * \~english
			 * @brief Maximum size of a packet number in the encoded form (RFC 9000 §17.1)
			 *
			 * \~
			 */
			static constexpr size_t MAX_PKT_NUM_SIZE = 4;
			/**
			 * \~russian
			 * @brief Максимальное значение целого числа переменной длины (2^62 - 1) (RFC 9000 §16)
			 *
			 * \~english
			 * @brief Maximum value of a variable-length integer (2^62 - 1) (RFC 9000 §16)
			 *
			 * \~
			 */
			static constexpr uint64_t VARINT_MAX = 0x3FFFFFFFFFFFFFFF;
			/**
			 * \~russian
			 * @brief Максимальное значение номера пакета (2^62 - 1) (RFC 9000 §17.1)
			 *
			 * \~english
			 * @brief Maximum value of a packet number (2^62 - 1) (RFC 9000 §17.1)
			 *
			 * \~
			 */
			static constexpr uint64_t MAX_PKT_NUM = 0x3FFFFFFFFFFFFFFF;
		};

		/**
		 * \~russian
		 * @brief Роль локального эндпоинта на соединении
		 *
		 * \~english
		 * @brief Role of the local endpoint on a connection
		 *
		 * \~
		 */
		enum class endpoint_t : uint8_t {
			CLIENT = 0x00, // Инициатор соединения
			SERVER = 0x01  // Принимающая сторона
		};

		/**
		 * \~russian
		 * @brief Результат пошаговой обработки/разбора
		 *
		 * \~english
		 * @brief Result of the step-by-step processing/parsing
		 *
		 * \~
		 */
		enum class status_t : uint8_t {
			OK         = 0x00, // Успешно, можно продолжать
			INCOMPLETE = 0x01, // Данных недостаточно - нужен ещё ввод
			ERROR      = 0x02  // Ошибка протокола - см. сопутствующий error_t
		};

		/**
		 * \~russian
		 * @brief Тип пакета QUIC (RFC 9000 §17)
		 *
		 * \~english
		 * @brief Type of a QUIC packet (RFC 9000 §17)
		 *
		 * \~
		 */
		enum class packet_t : uint8_t {
			INITIAL             = 0x00, // Установка соединения, содержит начальный CRYPTO-хендшейк (RFC 9000 §17.2.2)
			ZERO_RTT            = 0x01, // Ранние данные клиента до завершения хендшейка (RFC 9000 §17.2.3)
			HANDSHAKE           = 0x02, // Продолжение криптографического хендшейка (RFC 9000 §17.2.4)
			RETRY               = 0x03, // Проверка адреса клиента сервером без сохранения состояния (RFC 9000 §17.2.5)
			VERSION_NEGOTIATION = 0x04, // Согласование версии протокола (RFC 9000 §17.2.1)
			ONE_RTT             = 0x05, // Пакет с коротким заголовком после завершения хендшейка (RFC 9000 §17.3)
			UNKNOWN             = 0xFF  // Неизвестный или нераспознанный тип пакета
		};

		/**
		 * \~russian
		 * @brief Уровень шифрования пакета (RFC 9001 §4)
		 *
		 * \~english
		 * @brief Encryption level of a packet (RFC 9001 §4)
		 *
		 * \~
		 */
		enum class level_t : uint8_t {
			INITIAL     = 0x00, // Ключи Initial, выводятся из Destination CID клиента
			EARLY_DATA  = 0x01, // Ключи 0-RTT ранних данных
			HANDSHAKE   = 0x02, // Ключи хендшейка
			APPLICATION = 0x03  // Ключи приложения (1-RTT)
		};

		/**
		 * \~russian
		 * @brief Тип фрейма QUIC (RFC 9000 §12.4) — значение целого числа переменной длины
		 *
		 * @details Фреймы STREAM занимают диапазон 0x08-0x0F: три младших бита типа
		 *          кодируют наличие полей Offset (0x04), Length (0x02) и флага FIN (0x01)
		 *
		 * \~english
		 * @brief Type of a QUIC frame (RFC 9000 §12.4) — a value of a variable-length integer
		 * @details The STREAM frames occupy the range 0x08-0x0F: the three low bits of the type
		 *          encode the presence of the Offset (0x04) and Length (0x02) fields and of the FIN flag (0x01)
		 *
		 * \~
		 */
		enum class frame_t : uint64_t {
			PADDING             = 0x00, // Заполнение без полезной нагрузки (RFC 9000 §19.1)
			PING                = 0x01, // Проверка живости пути (RFC 9000 §19.2)
			ACK                 = 0x02, // Подтверждение приёма пакетов (RFC 9000 §19.3)
			ACK_ECN             = 0x03, // Подтверждение приёма пакетов со счётчиками ECN (RFC 9000 §19.3)
			RESET_STREAM        = 0x04, // Аварийное завершение потока отправителем (RFC 9000 §19.4)
			STOP_SENDING        = 0x05, // Запрос прекращения передачи потока (RFC 9000 §19.5)
			CRYPTO              = 0x06, // Данные криптографического хендшейка (RFC 9000 §19.6)
			NEW_TOKEN           = 0x07, // Токен для будущих соединений клиента (RFC 9000 §19.7)
			STREAM              = 0x08, // Данные потока приложения, базовый тип диапазона 0x08-0x0F (RFC 9000 §19.8)
			MAX_DATA            = 0x10, // Обновление лимита данных соединения (RFC 9000 §19.9)
			MAX_STREAM_DATA     = 0x11, // Обновление лимита данных потока (RFC 9000 §19.10)
			MAX_STREAMS_BIDI    = 0x12, // Обновление лимита двунаправленных потоков (RFC 9000 §19.11)
			MAX_STREAMS_UNI     = 0x13, // Обновление лимита однонаправленных потоков (RFC 9000 §19.11)
			DATA_BLOCKED        = 0x14, // Блокировка лимитом данных соединения (RFC 9000 §19.12)
			STREAM_DATA_BLOCKED = 0x15, // Блокировка лимитом данных потока (RFC 9000 §19.13)
			STREAMS_BLOCKED_BIDI = 0x16, // Блокировка лимитом двунаправленных потоков (RFC 9000 §19.14)
			STREAMS_BLOCKED_UNI = 0x17, // Блокировка лимитом однонаправленных потоков (RFC 9000 §19.14)
			NEW_CONNECTION_ID   = 0x18, // Анонс нового идентификатора соединения (RFC 9000 §19.15)
			RETIRE_CONNECTION_ID = 0x19, // Вывод идентификатора соединения из обращения (RFC 9000 §19.16)
			PATH_CHALLENGE      = 0x1A, // Проверка достижимости пути (RFC 9000 §19.17)
			PATH_RESPONSE       = 0x1B, // Ответ на проверку достижимости пути (RFC 9000 §19.18)
			CONNECTION_CLOSE    = 0x1C, // Завершение соединения с ошибкой транспорта (RFC 9000 §19.19)
			CONNECTION_CLOSE_APP = 0x1D, // Завершение соединения с ошибкой приложения (RFC 9000 §19.19)
			HANDSHAKE_DONE      = 0x1E, // Подтверждение завершения хендшейка сервером (RFC 9000 §19.20)
			DATAGRAM            = 0x30, // Ненадёжно доставляемая датаграмма приложения, базовый тип диапазона 0x30-0x31 (RFC 9221 §4)
			UNKNOWN             = 0xFFFFFFFFFFFFFFFF // Неизвестный или нераспознанный тип фрейма
		};

		/**
		 * \~russian
		 * @brief Коды ошибок транспорта QUIC (RFC 9000 §20.1) — используются в CONNECTION_CLOSE
		 *
		 * \~english
		 * @brief Codes of the QUIC transport errors (RFC 9000 §20.1) — used in CONNECTION_CLOSE
		 *
		 * \~
		 */
		enum class error_t : uint64_t {
			NO_ERROR                  = 0x00, // Штатное завершение
			INTERNAL_ERROR            = 0x01, // Внутренняя ошибка реализации
			CONNECTION_REFUSED        = 0x02, // Сервер отказал в приёме соединения
			FLOW_CONTROL_ERROR        = 0x03, // Нарушение flow control
			STREAM_LIMIT_ERROR        = 0x04, // Превышен лимит числа потоков
			STREAM_STATE_ERROR        = 0x05, // Фрейм недопустим в текущем состоянии потока
			FINAL_SIZE_ERROR          = 0x06, // Нарушение финального размера потока
			FRAME_ENCODING_ERROR      = 0x07, // Некорректное кодирование фрейма
			TRANSPORT_PARAMETER_ERROR = 0x08, // Некорректные параметры транспорта
			CONNECTION_ID_LIMIT_ERROR = 0x09, // Превышен лимит идентификаторов соединения
			PROTOCOL_VIOLATION        = 0x0A, // Общее нарушение протокола
			INVALID_TOKEN             = 0x0B, // Некорректный токен в пакете Initial
			APPLICATION_ERROR         = 0x0C, // Ошибка приложения
			CRYPTO_BUFFER_EXCEEDED    = 0x0D, // Переполнен буфер CRYPTO-данных
			KEY_UPDATE_ERROR          = 0x0E, // Ошибка обновления ключей
			AEAD_LIMIT_REACHED        = 0x0F, // Достигнут лимит использования AEAD-ключей
			NO_VIABLE_PATH            = 0x10, // Нет пригодного сетевого пути
			VERSION_NEGOTIATION_ERROR = 0x11, // Согласование версии не дало общей версии (RFC 9368 §4)
			CRYPTO_ERROR              = 0x0100 // База диапазона ошибок TLS-хендшейка 0x0100-0x01FF (RFC 9001 §4.8)
		};

		/**
		 * \~russian
		 * @brief Структура идентификатора соединения (RFC 9000 §5.1)
		 *
		 * \~english
		 * @brief Structure of a connection identifier (RFC 9000 §5.1)
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Cid {
			// Длина идентификатора соединения в октетах
			size_t size;
			// Данные идентификатора соединения
			uint8_t data[proto::MAX_CID_SIZE];
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
			explicit Cid() noexcept;
		} cid_t;

		/**
		 * \~russian
		 * @brief Оператор сравнения идентификаторов соединения
		 *
		 * @param a первый идентификатор соединения
		 * @param b второй идентификатор соединения
		 * @return  результат сравнения
		 *
		 * \~english
		 * @brief Comparison operator of the connection identifiers
		 * @param a first connection identifier
		 * @param b second connection identifier
		 * @return  result of the comparison
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool operator == (const cid_t & a, const cid_t & b) noexcept;

		/**
		 * \~russian
		 * @brief Функция извлечения идентификатора соединения получателя из датаграммы (RFC 9000 §17.2)
		 *
		 * @note Идентификатор соединения намеренно оставлен открытым, чтобы датаграмму
		 *       можно было отнести к соединению не расшифровывая её. Пакеты с длинным
		 *       заголовком несут длину идентификатора явно, короткие - нет, поэтому
		 *       для них она берётся из локальной политики выдачи идентификаторов
		 *
		 * @param data   данные датаграммы
		 * @param size   размер датаграммы
		 * @param length длина идентификаторов, выдаваемых локальным эндпоинтом
		 * @param key    извлечённый идентификатор соединения получателя
		 * @return       результат извлечения (false - датаграмма не является пакетом QUIC)
		 *
		 * \~english
		 * @brief Function of extracting the connection identifier of the recipient from a datagram (RFC 9000 §17.2)
		 * @note The connection identifier is deliberately left open so that a datagram
		 *       can be assigned to a connection without decrypting it. The packets with a long
		 *       header carry the length of the identifier explicitly, the short ones do not, therefore
		 *       for them it is taken from the local policy of the issuance of the identifiers
		 * @param data   data of the datagram
		 * @param size   size of the datagram
		 * @param length length of the identifiers issued by the local endpoint
		 * @param key    extracted connection identifier of the recipient
		 * @return       result of the extraction (false — the datagram is not a QUIC packet)
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool route(const uint8_t * data, const size_t size, const uint8_t length, net::origin_key_t & key) noexcept;

		/**
		 * \~russian
		 * @brief Функция вывода токена сброса без сохранения состояния (RFC 9000 §10.3.2)
		 *
		 * @note Токен выводится из идентификатора соединения и общего ключа, поэтому
		 *       воспроизводится без хранения состояния - в том числе для соединения,
		 *       о котором локальный эндпоинт уже ничего не помнит
		 *
		 * @param key   общий ключ вывода токенов сброса
		 * @param cid   идентификатор соединения локального эндпоинта
		 * @param token выведенный токен сброса без сохранения состояния
		 * @return      результат вывода (false - пустой ключ либо ошибка кода аутентичности)
		 *
		 * \~english
		 * @brief Function of the derivation of a stateless reset token (RFC 9000 §10.3.2)
		 * @note The token is derived from the connection identifier and from the common key, therefore
		 *       it is reproduced without storing the state — including for a connection
		 *       about which the local endpoint no longer remembers anything
		 * @param key   common key of the derivation of the reset tokens
		 * @param cid   connection identifier of the local endpoint
		 * @param token derived stateless reset token
		 * @return      result of the derivation (false — an empty key or an error of the authentication code)
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool resetToken(string_view key, const cid_t & cid, uint8_t token[proto::RESET_TOKEN_SIZE]) noexcept;

		/**
		 * \~russian
		 * @brief Функция генерации общего ключа вывода токенов сброса (RFC 9000 §10.3.2)
		 *
		 * @note Ключ живёт столько же, сколько выданные на нём токены: чтобы сброс
		 *       работал и после перезапуска приложения, ключ следует сохранять
		 *       и восстанавливать, а не генерировать заново
		 *
		 * @param output сгенерированный общий ключ вывода токенов сброса
		 * @return       результат генерации (false - ошибка генератора случайных чисел)
		 *
		 * \~english
		 * @brief Function of the generation of the common key of the derivation of the reset tokens (RFC 9000 §10.3.2)
		 * @note The key lives as long as the tokens issued on it: so that the reset
		 *       works after a restart of the application as well, the key should be preserved
		 *       and restored rather than generated anew
		 * @param output generated common key of the derivation of the reset tokens
		 * @return       result of the generation (false — an error of the random number generator)
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool resetKey(string & output) noexcept;

		/**
		 * \~russian
		 * @brief Функция сборки пакета сброса без сохранения состояния (RFC 9000 §10.3)
		 *
		 * @note Пакет неотличим от пакета 1-RTT со случайным содержимым и завершается
		 *       токеном сброса. Размер выбирается меньше размера вызвавшей его датаграммы:
		 *       иначе два эндпоинта, утративших состояние, отвечали бы друг другу
		 *       сбросами неограниченно долго (RFC 9000 §10.3.3)
		 *
		 * @param output  выходной буфер пакета сброса
		 * @param key     общий ключ вывода токенов сброса
		 * @param cid     идентификатор соединения получателя из вызвавшей датаграммы
		 * @param trigger размер вызвавшей сброс датаграммы
		 * @return        результат сборки (false - датаграмма слишком мала либо ошибка вывода токена)
		 *
		 * \~english
		 * @brief Function of assembling a stateless reset packet (RFC 9000 §10.3)
		 * @note The packet is indistinguishable from a 1-RTT packet with a random content and ends
		 *       with the reset token. The size is chosen smaller than the size of the datagram that has caused it:
		 *       otherwise two endpoints that have lost their state would answer one another
		 *       with the resets indefinitely long (RFC 9000 §10.3.3)
		 * @param output  output buffer of the reset packet
		 * @param key     common key of the derivation of the reset tokens
		 * @param cid     connection identifier of the recipient from the datagram that has caused it
		 * @param trigger size of the datagram that has caused the reset
		 * @return        result of the assembly (false — the datagram is too small or an error of the derivation of the token)
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool reset(string & output, string_view key, const cid_t & cid, const size_t trigger) noexcept;

		/**
		 * \~russian
		 * @brief Функция получения человекочитаемого названия типа пакета
		 *
		 * @param type тип пакета
		 * @return     название типа пакета
		 *
		 * \~english
		 * @brief Function of getting the human-readable name of a type of a packet
		 * @param type type of the packet
		 * @return     name of the type of the packet
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ string_view packetName(const packet_t type) noexcept;

		/**
		 * \~russian
		 * @brief Функция получения человекочитаемого названия типа фрейма
		 *
		 * @param type тип фрейма
		 * @return     название типа фрейма
		 *
		 * \~english
		 * @brief Function of getting the human-readable name of a type of a frame
		 * @param type type of the frame
		 * @return     name of the type of the frame
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ string_view frameName(const frame_t type) noexcept;

		/**
		 * \~russian
		 * @brief Функция получения человекочитаемого названия кода ошибки транспорта
		 *
		 * @param code код ошибки транспорта
		 * @return     название кода ошибки
		 *
		 * \~english
		 * @brief Function of getting the human-readable name of a transport error code
		 * @param code transport error code
		 * @return     name of the error code
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ string_view errorName(const error_t code) noexcept;
	};
};

/**
 * Возвращаем системные макросы потребителю библиотеки:
 * имена, подавленные в начале файла, снова принадлежат ему
 */
#include "../../sys/macro/restore.hpp"

#endif // __AWH_PROTO_QUIC__
