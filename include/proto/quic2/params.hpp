/**
 * @file: params.hpp
 * @date: 2026-07-21
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл транспортных параметров QUIC (RFC 9000 §18) —
 *        структура параметров соединения с значениями по умолчанию,
 *        предпочтительный адрес сервера и функции их разбора и сборки с контролем ролевых ограничений
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_PROTO_QUIC2_PARAMS__
#define __AWH_PROTO_QUIC2_PARAMS__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <cstdint>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "quic.hpp"
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
	namespace quic2 {
		/**
		 * @brief Пространство имён кодека параметров транспорта QUIC (RFC 9000 §18)
		 *
		 * @details Параметры транспорта передаются в TLS-расширении quic_transport_parameters
		 *          (RFC 9001 §8.2) как последовательность записей (идентификатор, длина, значение)
		 *          без общего заголовка. Неизвестные параметры игнорируются, дубликаты и
		 *          нарушение ролевых ограничений - TRANSPORT_PARAMETER_ERROR (RFC 9000 §7.4).
		 *          Слой не хранит состояния соединения - это чистые функции над байтами.
		 *
		 */
		namespace params {
			/**
			 * @brief Идентификаторы параметров транспорта (RFC 9000 §18.2)
			 *
			 */
			enum class id_t : uint64_t {
				ORIGINAL_DESTINATION_CONNECTION_ID  = 0x00, // Исходный DCID первого пакета Initial клиента (только сервер)
				MAX_IDLE_TIMEOUT                    = 0x01, // Таймаут простоя соединения в миллисекундах (0 - отключён)
				STATELESS_RESET_TOKEN               = 0x02, // Токен сброса без сохранения состояния (только сервер)
				MAX_UDP_PAYLOAD_SIZE                = 0x03, // Максимальный размер принимаемой UDP-нагрузки (по умолчанию 65527)
				INITIAL_MAX_DATA                    = 0x04, // Начальный лимит данных соединения
				INITIAL_MAX_STREAM_DATA_BIDI_LOCAL  = 0x05, // Начальный лимит данных локально инициируемых двунаправленных потоков
				INITIAL_MAX_STREAM_DATA_BIDI_REMOTE = 0x06, // Начальный лимит данных удалённо инициируемых двунаправленных потоков
				INITIAL_MAX_STREAM_DATA_UNI         = 0x07, // Начальный лимит данных однонаправленных потоков
				INITIAL_MAX_STREAMS_BIDI            = 0x08, // Начальный лимит числа двунаправленных потоков
				INITIAL_MAX_STREAMS_UNI             = 0x09, // Начальный лимит числа однонаправленных потоков
				ACK_DELAY_EXPONENT                  = 0x0A, // Показатель степени задержки подтверждения (по умолчанию 3, не более 20)
				MAX_ACK_DELAY                       = 0x0B, // Максимальная задержка подтверждения в миллисекундах (по умолчанию 25)
				DISABLE_ACTIVE_MIGRATION            = 0x0C, // Запрет активной миграции соединения (без значения)
				PREFERRED_ADDRESS                   = 0x0D, // Предпочтительный адрес сервера (только сервер)
				ACTIVE_CONNECTION_ID_LIMIT          = 0x0E, // Лимит активных идентификаторов соединения (по умолчанию 2, не менее 2)
				INITIAL_SOURCE_CONNECTION_ID        = 0x0F, // SCID первого пакета эндпоинта
				RETRY_SOURCE_CONNECTION_ID          = 0x10, // SCID пакета Retry (только сервер)
				MAX_DATAGRAM_FRAME_SIZE             = 0x20  // Предельный размер принимаемого фрейма DATAGRAM (RFC 9221 §3)
			};

			/**
			 * @brief Структура предпочтительного адреса сервера (RFC 9000 §18.2)
			 *
			 */
			typedef struct __AWH_SHARED_EXPORT__ Preferred_Address {
				// Порт IPv4-адреса сервера
				uint16_t ipv4Port;
				// Порт IPv6-адреса сервера
				uint16_t ipv6Port;
				// IPv4-адрес сервера в сетевом порядке байт (нулевой - не используется)
				uint8_t ipv4[4];
				// IPv6-адрес сервера в сетевом порядке байт (нулевой - не используется)
				uint8_t ipv6[16];
				// Идентификатор соединения для предпочтительного адреса (1-20 октетов)
				cid_t cid;
				// Токен сброса без сохранения состояния для идентификатора
				uint8_t resetToken[proto::RESET_TOKEN_SIZE];
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Preferred_Address() noexcept;
			} preferred_address_t;

			/**
			 * @brief Структура параметров транспорта QUIC (RFC 9000 §18.2)
			 *
			 * @details Скалярные поля инициализируются значениями по умолчанию из RFC 9000 §18.2.
			 *          Наличие параметров без значений по умолчанию отражают флаги has*
			 *
			 */
			typedef struct __AWH_SHARED_EXPORT__ Params {
				// Флаг наличия исходного DCID первого пакета Initial клиента
				bool hasOdcid;
				// Флаг наличия SCID первого пакета эндпоинта
				bool hasInitialScid;
				// Флаг наличия SCID пакета Retry
				bool hasRetryScid;
				// Флаг наличия токена сброса без сохранения состояния
				bool hasResetToken;
				// Флаг наличия предпочтительного адреса сервера
				bool hasPreferredAddress;
				// Флаг запрета активной миграции соединения
				bool disableActiveMigration;
				// Таймаут простоя соединения в миллисекундах (0 - отключён)
				uint64_t maxIdleTimeout;
				// Максимальный размер принимаемой UDP-нагрузки
				uint64_t maxUdpPayloadSize;
				// Начальный лимит данных соединения
				uint64_t initialMaxData;
				// Начальный лимит данных локально инициируемых двунаправленных потоков
				uint64_t initialMaxStreamDataBidiLocal;
				// Начальный лимит данных удалённо инициируемых двунаправленных потоков
				uint64_t initialMaxStreamDataBidiRemote;
				// Начальный лимит данных однонаправленных потоков
				uint64_t initialMaxStreamDataUni;
				// Начальный лимит числа двунаправленных потоков
				uint64_t initialMaxStreamsBidi;
				// Начальный лимит числа однонаправленных потоков
				uint64_t initialMaxStreamsUni;
				// Показатель степени задержки подтверждения
				uint64_t ackDelayExponent;
				// Максимальная задержка подтверждения в миллисекундах
				uint64_t maxAckDelay;
				// Лимит активных идентификаторов соединения
				uint64_t activeConnectionIdLimit;
				/**
				 * Предельный размер принимаемого фрейма DATAGRAM вместе с заголовком
				 * фрейма. Нулевое значение означает, что фреймы DATAGRAM эндпоинтом
				 * не принимаются, и отправлять их ему нельзя (RFC 9221 §3)
				 */
				uint64_t maxDatagramFrameSize;
				// Исходный DCID первого пакета Initial клиента (только сервер)
				cid_t odcid;
				// SCID первого пакета эндпоинта
				cid_t initialScid;
				// SCID пакета Retry (только сервер)
				cid_t retryScid;
				// Токен сброса без сохранения состояния (только сервер)
				uint8_t resetToken[proto::RESET_TOKEN_SIZE];
				// Предпочтительный адрес сервера (только сервер)
				preferred_address_t preferredAddress;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Params() noexcept;
			} params_t;

			/**
			 * @brief Пространство имён функций разбора параметров транспорта (RFC 9000 §18)
			 *
			 */
			namespace parser {
				/**
				 * @brief Функция разбора параметров транспорта из TLS-расширения
				 *
				 * @note Параметр sender задаёт роль эндпоинта, закодировавшего параметры:
				 *       параметры только для сервера в параметрах клиента - TRANSPORT_PARAMETER_ERROR
				 *
				 * @param data   буфер значения TLS-расширения quic_transport_parameters
				 * @param size   доступно байт
				 * @param sender роль эндпоинта, закодировавшего параметры
				 * @param output разобранные параметры транспорта
				 * @param error  код ошибки транспорта
				 * @return       результат разбора (OK/ERROR)
				 *
				 */
				__AWH_SHARED_EXPORT__ status_t decode(const uint8_t * data, const size_t size, const endpoint_t sender, params_t & output, error_t & error) noexcept;
			};

			/**
			 * @brief Пространство имён функций сборки параметров транспорта (RFC 9000 §18)
			 *
			 */
			namespace serialize {
				/**
				 * @brief Функция сборки параметров транспорта для TLS-расширения
				 *
				 * @note Скалярные параметры со значениями по умолчанию не кодируются.
				 *       Параметры только для сервера у клиента - ошибка сборки
				 *
				 * @param output выходной буфер значения TLS-расширения
				 * @param params параметры транспорта
				 * @param sender роль эндпоинта, кодирующего параметры
				 * @return       результат сборки (false - некорректные параметры)
				 *
				 */
				__AWH_SHARED_EXPORT__ bool encode(string & output, const params_t & params, const endpoint_t sender) noexcept;
				/**
				 * @brief Функция сборки контекста ранних данных из параметров транспорта (RFC 9001 §4.6.1)
				 *
				 * @note Кодируются только параметры, которые клиент запоминает вместе
				 *       с билетом возобновления и под лимиты которых отправляет ранние
				 *       данные. Полное представление параметров здесь неприменимо:
				 *       идентификаторы соединения и токен сброса уникальны для каждого
				 *       соединения, и совпадение контекста стало бы невозможным
				 *
				 * @param output выходной буфер контекста ранних данных
				 * @param params параметры транспорта
				 * @return       результат сборки (false - некорректные параметры)
				 *
				 */
				__AWH_SHARED_EXPORT__ bool early(string & output, const params_t & params) noexcept;
			};
		};
	};
};

#endif // __AWH_PROTO_QUIC2_PARAMS__
