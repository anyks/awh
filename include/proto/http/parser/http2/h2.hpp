/**
 * @file h2.hpp
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
 * @brief Заголовочный файл констант протокола HTTP/2 (RFC 9113) — перечисления типов фреймов, флагов,
 *        параметров SETTINGS, состояний потоков и кодов ошибок, общие для слоя фреймов, HPACK-кодека и парсера сессии
 *
 * \~english
 * @brief Header file of the constants of the HTTP/2 protocol (RFC 9113) — the enumerations of the types of the frames, of the flags,
 *        of the parameters of SETTINGS, of the states of the streams and of the error codes, common for the layer of the frames, the HPACK codec and the parser of the session
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_HTTP_PARSER_HTTP2_H2__
#define __AWH_HTTP_PARSER_HTTP2_H2__

/**
 * Стандартные заголовочные файлы
 */
#include <cstddef>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочный файл проекта
 */
#include "../../../../sys/global.hpp"

/**
 * Снимаем на время объявлений макросы, чьи имена заняты
 * членами перечислений ниже (возвращает их pop.hpp в конце файла)
 */
#include "../../../../sys/push.hpp"

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
		 * @brief Пространство имён внутренних слоёв протокола HTTP/2 (RFC 9113) и HPACK (RFC 7541)
		 *
		 * @details Содержит базовые типы и константы протокола, общие для framing-слоя (frame.hpp),
		 *          HPACK-кодека (hpack.hpp) и парсера сессии (http.hpp). Логики здесь нет —
		 *          только перечисления, константы протокола и POD-структуры.
		 *
		 * \~english
		 * @brief Namespace of the internal layers of the HTTP/2 protocol (RFC 9113) and of HPACK (RFC 7541)
		 * @details Contains the base types and the constants of the protocol, common for the framing layer (frame.hpp),
		 *          the HPACK codec (hpack.hpp) and the parser of the session (http.hpp). There is no logic here —
		 *          only the enumerations, the constants of the protocol and the POD structures.
		 *
		 * \~
		 */
		namespace h2 {
			/**
			 * \~russian
			 * @brief Пространство имён флагов фреймов (RFC 9113 §6)
			 *
			 * @details Один и тот же бит означает разное для разных типов фреймов,
			 *          поэтому это набор констант, а не enum class.
			 *
			 * \~english
			 * @brief Namespace of the flags of the frames (RFC 9113 §6)
			 * @details One and the same bit means different things for the different types of the frames,
			 *          therefore this is a collection of the constants rather than an enum class.
			 *
			 * \~
			 */
			namespace flag {
				/**
				 * \~russian
				 * @brief Флаги не установлены
				 *
				 * \~english
				 * @brief The flags are not set
				 *
				 * \~
				 */
				static constexpr uint8_t NONE = 0x00;
				/**
				 * \~russian
				 * @brief Подтверждение получения (SETTINGS, PING)
				 *
				 * \~english
				 * @brief Confirmation of the receipt (SETTINGS, PING)
				 *
				 * \~
				 */
				static constexpr uint8_t ACK = 0x01;
				/**
				 * \~russian
				 * @brief Наличие padding в нагрузке (DATA, HEADERS, PUSH_PROMISE)
				 *
				 * \~english
				 * @brief Presence of a padding in the payload (DATA, HEADERS, PUSH_PROMISE)
				 *
				 * \~
				 */
				static constexpr uint8_t PADDED = 0x08;
				/**
				 * \~russian
				 * @brief Наличие полей приоритета (HEADERS)
				 *
				 * \~english
				 * @brief Presence of the fields of the priority (HEADERS)
				 *
				 * \~
				 */
				static constexpr uint8_t PRIORITY = 0x20;
				/**
				 * \~russian
				 * @brief Завершение потока (DATA, HEADERS)
				 *
				 * \~english
				 * @brief Completion of a stream (DATA, HEADERS)
				 *
				 * \~
				 */
				static constexpr uint8_t END_STREAM = 0x01;
				/**
				 * \~russian
				 * @brief Завершение блока заголовков (HEADERS, PUSH_PROMISE, CONTINUATION)
				 *
				 * \~english
				 * @brief Completion of a block of the headers (HEADERS, PUSH_PROMISE, CONTINUATION)
				 *
				 * \~
				 */
				static constexpr uint8_t END_HEADERS = 0x04;
			};

			/**
			 * \~russian
			 * @brief Пространство имён констант протокола (RFC 9113)
			 *
			 * \~english
			 * @brief Namespace of the constants of the protocol (RFC 9113)
			 *
			 * \~
			 */
			namespace proto {
				/**
				 * \~russian
				 * @brief Размер заголовка любого фрейма в октетах (RFC 9113 §4.1)
				 *
				 * \~english
				 * @brief Size of the header of any frame in octets (RFC 9113 §4.1)
				 *
				 * \~
				 */
				static constexpr size_t FRAME_HEADER_SIZE = 9;
				/**
				 * \~russian
				 * @brief Флаг разрешения server push (SETTINGS_ENABLE_PUSH)
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 *
				 * \~english
				 * @brief Flag of the permission of a server push (SETTINGS_ENABLE_PUSH)
				 * @note The values by default are selected conservatively
				 *
				 * \~
				 */
				static constexpr uint32_t DEFAULT_ENABLE_PUSH = 1;
				/**
				 * \~russian
				 * @brief Максимальное число одновременных потоков в соединении
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 *
				 * \~english
				 * @brief Largest number of the simultaneous streams in a connection
				 * @note The values by default are selected conservatively
				 *
				 * \~
				 */
				static constexpr uint32_t MAX_COUNT_STREAMS = 128;
				/**
				 * \~russian
				 * @brief Максимальный размер списка заголовков
				 *
				 * @note 0 - без лимита в SETTINGS, действует maxHeadersTotal
				 *
				 * \~english
				 * @brief Largest size of the list of the headers
				 * @note 0 - without a limit in SETTINGS, maxHeadersTotal is in force
				 *
				 * \~
				 */
				static constexpr uint64_t MAX_HEADER_LIST_SIZE = 0;
				/**
				 * \~russian
				 * @brief Нижняя граница для SETTINGS_MAX_FRAME_SIZE
				 *
				 * \~english
				 * @brief Lower boundary for SETTINGS_MAX_FRAME_SIZE
				 *
				 * \~
				 */
				static constexpr uint32_t MIN_MAX_FRAME_SIZE = 16384;
				/**
				 * \~russian
				 * @brief Начальный размер окна управления потоком (RFC 9113 §6.9.2)
				 *
				 * \~english
				 * @brief Initial size of the window of the flow control (RFC 9113 §6.9.2)
				 *
				 * \~
				 */
				static constexpr int32_t DEFAULT_WINDOW_SIZE = 65535;
				/**
				 * \~russian
				 * @brief Максимально допустимое значение поля Length (24 бита)
				 *
				 * \~english
				 * @brief Largest admissible value of the field Length (24 bits)
				 *
				 * \~
				 */
				static constexpr uint32_t MAX_FRAME_LENGTH = 0xFFFFFF;
				/**
				 * \~russian
				 * @brief Максимальное значение окна (2^31 - 1)
				 *
				 * \~english
				 * @brief Largest value of the window (2^31 - 1)
				 *
				 * \~
				 */
				static constexpr int32_t MAX_WINDOW_SIZE = 0x7FFFFFFF;
				/**
				 * \~russian
				 * @brief Маска для извлечения 31-битного идентификатора потока (сброс reserved-бита)
				 *
				 * \~english
				 * @brief Mask for the extraction of the 31-bit identifier of a stream (the reset of the reserved bit)
				 *
				 * \~
				 */
				static constexpr uint32_t STREAM_ID_MASK = 0x7FFFFFFF;
				/**
				 * \~russian
				 * @brief Верхняя граница для SETTINGS_MAX_FRAME_SIZE
				 *
				 * \~english
				 * @brief Upper boundary for SETTINGS_MAX_FRAME_SIZE
				 *
				 * \~
				 */
				static constexpr uint32_t MAX_MAX_FRAME_SIZE = 16777215;
				/**
				 * \~russian
				 * @brief Значение SETTINGS_MAX_FRAME_SIZE по умолчанию (16 КиБ)
				 *
				 * \~english
				 * @brief Value of SETTINGS_MAX_FRAME_SIZE by default (16 KiB)
				 *
				 * \~
				 */
				static constexpr uint32_t DEFAULT_MAX_FRAME_SIZE = 16384;
				/**
				 * \~russian
				 * @brief Размер динамической таблицы HPACK по умолчанию (RFC 7541)
				 *
				 * \~english
				 * @brief Size of the dynamic table of HPACK by default (RFC 7541)
				 *
				 * \~
				 */
				static constexpr uint32_t DEFAULT_HEADER_TABLE_SIZE = 4096;
				/**
				 * \~russian
				 * @brief Идентификатор ALPN для HTTP/2 поверх TLS
				 *
				 * \~english
				 * @brief Identifier of ALPN for HTTP/2 over TLS
				 *
				 * \~
				 */
				static constexpr string_view ALPN = "h2";
				/**
				 * \~russian
				 * @brief Идентификатор ALPN для HTTP/2 поверх открытого TCP (h2c)
				 *
				 * \~english
				 * @brief Identifier of ALPN for HTTP/2 over an open TCP (h2c)
				 *
				 * \~
				 */
				static constexpr string_view ALPN_CLEARTEXT = "h2c";
				/**
				 * \~russian
				 * @brief Срочность потока по умолчанию (RFC 9218 §4.1)
				 *
				 * \~english
				 * @brief Urgency of a stream by default (RFC 9218 §4.1)
				 *
				 * \~
				 */
				static constexpr uint8_t DEFAULT_URGENCY = 3;
				/**
				 * \~russian
				 * @brief Наименее срочный уровень расширенного приоритета (RFC 9218 §4.1)
				 *
				 * \~english
				 * @brief Least urgent level of the extended priority (RFC 9218 §4.1)
				 *
				 * \~
				 */
				static constexpr uint8_t MAX_URGENCY = 7;
				/**
				 * \~russian
				 * @brief Идентификатор потока для предупреждающего GOAWAY плавного завершения (RFC 9113 §6.8)
				 *
				 * \~english
				 * @brief Identifier of a stream for the warning GOAWAY of a smooth completion (RFC 9113 §6.8)
				 *
				 * \~
				 */
				static constexpr uint32_t MAX_STREAM_ID = 0x7FFFFFFF;
				/**
				 * \~russian
				 * @brief Клиентский connection preface (24 октета), отправляется до первого SETTINGS
				 *
				 * \~english
				 * @brief Client connection preface (24 octets), is sent before the first SETTINGS
				 *
				 * \~
				 */
				static constexpr string_view PREFACE = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
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
				CLIENT = 0x00, // Нечётные stream id инициируем мы
				SERVER = 0x01  // Чётные stream id (push) инициируем мы
			};

			/**
			 * \~russian
			 * @brief Результат пошаговой обработки/разбора
			 *
			 * \~english
			 * @brief Result of a step-by-step processing/parsing
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
			 * @brief Состояния потока (RFC 9113 §5.1)
			 *
			 * \~english
			 * @brief States of a stream (RFC 9113 §5.1)
			 *
			 * \~
			 */
			enum class stream_state_t : uint8_t {
				IDLE               = 0x00, // Поток ещё не использован
				RESERVED_LOCAL     = 0x01, // Зарезервирован нами через PUSH_PROMISE
				RESERVED_REMOTE    = 0x02, // Зарезервирован пиром через PUSH_PROMISE
				OPEN               = 0x03, // Оба конца могут слать данные
				HALF_CLOSED_LOCAL  = 0x04, // Мы отправили END_STREAM
				HALF_CLOSED_REMOTE = 0x05, // Пир отправил END_STREAM
				CLOSED             = 0x06  // Поток завершён
			};

			/**
			 * \~russian
			 * @brief Тип фрейма (RFC 9113 §6) — значение поля Type (8 бит)
			 *
			 * \~english
			 * @brief Type of a frame (RFC 9113 §6) — the value of the field Type (8 bits)
			 *
			 * \~
			 */
			enum class frame_t : uint8_t {
				DATA          = 0x00, // Данные тела (RFC 9113 §6.1)
				HEADERS       = 0x01, // Блок заголовков, открывает поток (RFC 9113 §6.2)
				PRIORITY      = 0x02, // Приоритет (RFC 7540, deprecated) (RFC 9113 §6.3)
				RST_STREAM    = 0x03, // Аварийное закрытие потока (RFC 9113 §6.4)
				SETTINGS      = 0x04, // Параметры соединения (RFC 9113 §6.5)
				PUSH_PROMISE  = 0x05, // Server push - резервирование потока (RFC 9113 §6.6)
				PING          = 0x06, // Проверка живости / измерение RTT (RFC 9113 §6.7)
				GOAWAY        = 0x07, // Завершение соединения (RFC 9113 §6.8)
				WINDOW_UPDATE = 0x08, // Обновление окна flow control (RFC 9113 §6.9)
				CONTINUATION    = 0x09, // Продолжение блока заголовков (RFC 9113 §6.10)
				ALTSVC          = 0x0a, // Анонс альтернативного сервиса (RFC 7838 §4)
				ORIGIN          = 0x0c, // Набор origin, обслуживаемых соединением (RFC 8336 §2)
				PRIORITY_UPDATE = 0x10  // Обновление расширенного приоритета потока (RFC 9218 §7.1)
			};

			/**
			 * \~russian
			 * @brief Идентификаторы параметров SETTINGS (RFC 9113 §6.5.2)
			 *
			 * \~english
			 * @brief Identifiers of the parameters of SETTINGS (RFC 9113 §6.5.2)
			 *
			 * \~
			 */
			enum class setting_t : uint16_t {
				HEADER_TABLE_SIZE      = 0x01, // Размер динамической таблицы HPACK (по умолчанию 4096)
				ENABLE_PUSH            = 0x02, // Разрешён ли server push (0/1, по умолчанию 1)
				MAX_CONCURRENT_STREAMS = 0x03, // Лимит одновременных потоков (по умолчанию без лимита)
				INITIAL_WINDOW_SIZE    = 0x04, // Начальное окно потока (по умолчанию 65535)
				MAX_FRAME_SIZE         = 0x05, // Максимальный размер фрейма (по умолчанию 16384)
				MAX_HEADER_LIST_SIZE    = 0x06, // Лимит размера списка заголовков (по умолчанию без лимита)
				ENABLE_CONNECT_PROTOCOL = 0x08, // Разрешён ли расширенный CONNECT (0/1, по умолчанию 0) - RFC 8441 §3
				NO_RFC7540_PRIORITIES   = 0x09  // Отказ от приоритетов RFC 7540 (0/1, по умолчанию 0) - RFC 9218 §2.1
			};

			/**
			 * \~russian
			 * @brief Коды ошибок HTTP/2 (RFC 9113 §7) — используются в RST_STREAM и GOAWAY
			 *
			 * \~english
			 * @brief Error codes of HTTP/2 (RFC 9113 §7) — are used in RST_STREAM and GOAWAY
			 *
			 * \~
			 */
			enum class error_t : uint32_t {
				NO_ERROR            = 0x00, // Штатное завершение
				PROTOCOL_ERROR      = 0x01, // Нарушение протокола
				INTERNAL_ERROR      = 0x02, // Внутренняя ошибка реализации
				FLOW_CONTROL_ERROR  = 0x03, // Нарушение flow control
				SETTINGS_TIMEOUT    = 0x04, // Не получен ACK на SETTINGS
				STREAM_CLOSED       = 0x05, // Фрейм для закрытого потока
				FRAME_SIZE_ERROR    = 0x06, // Некорректный размер фрейма
				REFUSED_STREAM      = 0x07, // Поток отклонён до обработки
				CANCEL              = 0x08, // Поток больше не нужен
				COMPRESSION_ERROR   = 0x09, // Ошибка состояния HPACK
				CONNECT_ERROR       = 0x0A, // Ошибка соединения для метода CONNECT
				ENHANCE_YOUR_CALM   = 0x0B, // Обнаружено чрезмерное поведение (флуд)
				INADEQUATE_SECURITY = 0x0C, // Недостаточный уровень безопасности TLS
				HTTP_1_1_REQUIRED   = 0x0D  // Требуется откат на HTTP/1.1
			};

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
			 * @brief Функция получения человекочитаемого названия кода ошибки
			 *
			 * @param code код ошибки протокола
			 * @return     название кода ошибки
			 *
			 * \~english
			 * @brief Function of getting the human-readable name of an error code
			 * @param code error code of the protocol
			 * @return     name of the error code
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ string_view errorName(const error_t code) noexcept;
		}
	};
};

/**
 * Возвращаем макросы, снятые в начале файла
 */
#include "../../../../sys/pop.hpp"

#endif // __AWH_HTTP_PARSER_HTTP2_H2__
