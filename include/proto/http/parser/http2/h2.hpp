/**
 * @file: h2.hpp
 * @date: 2026-07-19
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

#ifndef __AWH_HTTP_PARSER_HTTP2_H2__
#define __AWH_HTTP_PARSER_HTTP2_H2__

/**
 * Стандартные заголовочные файлы
 */
#include <cstddef>
#include <cstdint>
#include <string_view>

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
	 * @brief Пространство имён HTTP-протокола
	 *
	 */
	namespace http {
		/**
		 * @brief Пространство имён внутренних слоёв протокола HTTP/2 (RFC 9113) и HPACK (RFC 7541)
		 *
		 * @details Содержит базовые типы и константы протокола, общие для framing-слоя (frame.hpp),
		 *          HPACK-кодека (hpack.hpp) и парсера сессии (http.hpp). Логики здесь нет —
		 *          только перечисления, константы протокола и POD-структуры.
		 */
		namespace h2 {
			/**
			 * @brief Пространство имён констант протокола (RFC 9113)
			 *
			 */
			namespace proto {
				/**
				 * @brief Клиентский connection preface (24 октета), отправляется до первого SETTINGS
				 *
				 */
				static constexpr string_view PREFACE = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
				/**
				 * @brief Идентификатор ALPN для HTTP/2 поверх TLS
				 *
				 */
				static constexpr string_view ALPN = "h2";
				/**
				 * @brief Идентификатор ALPN для HTTP/2 поверх открытого TCP (h2c)
				 *
				 */
				static constexpr string_view ALPN_CLEARTEXT = "h2c";
				/**
				 * @brief Размер заголовка любого фрейма в октетах (RFC 9113 §4.1)
				 *
				 */
				static constexpr size_t FRAME_HEADER_SIZE = 9;
				/**
				 * @brief Максимально допустимое значение поля Length (24 бита)
				 *
				 */
				static constexpr uint32_t MAX_FRAME_LENGTH = 0xFFFFFF;
				/**
				 * @brief Значение SETTINGS_MAX_FRAME_SIZE по умолчанию (16 КиБ)
				 *
				 */
				static constexpr uint32_t DEFAULT_MAX_FRAME_SIZE = 16384;
				/**
				 * @brief Нижняя граница для SETTINGS_MAX_FRAME_SIZE
				 *
				 */
				static constexpr uint32_t MIN_MAX_FRAME_SIZE = 16384;
				/**
				 * @brief Верхняя граница для SETTINGS_MAX_FRAME_SIZE
				 *
				 */
				static constexpr uint32_t MAX_MAX_FRAME_SIZE = 16777215;
				/**
				 * @brief Начальный размер окна управления потоком (RFC 9113 §6.9.2)
				 *
				 */
				static constexpr int32_t DEFAULT_WINDOW_SIZE = 65535;
				/**
				 * @brief Максимальное значение окна (2^31 - 1)
				 *
				 */
				static constexpr int32_t MAX_WINDOW_SIZE = 0x7FFFFFFF;
				/**
				 * @brief Размер динамической таблицы HPACK по умолчанию (RFC 7541)
				 *
				 */
				static constexpr uint32_t DEFAULT_HEADER_TABLE_SIZE = 4096;
				/**
				 * @brief Маска для извлечения 31-битного идентификатора потока (сброс reserved-бита)
				 *
				 */
				static constexpr uint32_t STREAM_ID_MASK = 0x7FFFFFFF;
			}

			/**
			 * @brief Тип фрейма (RFC 9113 §6) — значение поля Type (8 бит)
			 *
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
				CONTINUATION  = 0x09  // Продолжение блока заголовков (RFC 9113 §6.10)
			};

			/**
			 * @brief Пространство имён флагов фреймов (RFC 9113 §6)
			 *
			 * @details Один и тот же бит означает разное для разных типов фреймов,
			 *          поэтому это набор констант, а не enum class
			 */
			namespace flag {
				/**
				 * @brief Флаги не установлены
				 *
				 */
				static constexpr uint8_t NONE = 0x00;
				/**
				 * @brief Завершение потока (DATA, HEADERS)
				 *
				 */
				static constexpr uint8_t END_STREAM = 0x01;
				/**
				 * @brief Подтверждение получения (SETTINGS, PING)
				 *
				 */
				static constexpr uint8_t ACK = 0x01;
				/**
				 * @brief Завершение блока заголовков (HEADERS, PUSH_PROMISE, CONTINUATION)
				 *
				 */
				static constexpr uint8_t END_HEADERS = 0x04;
				/**
				 * @brief Наличие padding в нагрузке (DATA, HEADERS, PUSH_PROMISE)
				 *
				 */
				static constexpr uint8_t PADDED = 0x08;
				/**
				 * @brief Наличие полей приоритета (HEADERS)
				 *
				 */
				static constexpr uint8_t PRIORITY = 0x20;
			}

			/**
			 * @brief Идентификаторы параметров SETTINGS (RFC 9113 §6.5.2)
			 *
			 */
			enum class setting_t : uint16_t {
				HEADER_TABLE_SIZE      = 0x01, // Размер динамической таблицы HPACK (по умолчанию 4096)
				ENABLE_PUSH            = 0x02, // Разрешён ли server push (0/1, по умолчанию 1)
				MAX_CONCURRENT_STREAMS = 0x03, // Лимит одновременных потоков (по умолчанию без лимита)
				INITIAL_WINDOW_SIZE    = 0x04, // Начальное окно потока (по умолчанию 65535)
				MAX_FRAME_SIZE         = 0x05, // Максимальный размер фрейма (по умолчанию 16384)
				MAX_HEADER_LIST_SIZE   = 0x06  // Лимит размера списка заголовков (по умолчанию без лимита)
			};

			/**
			 * @brief Коды ошибок HTTP/2 (RFC 9113 §7) — используются в RST_STREAM и GOAWAY
			 *
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
			 * @brief Состояния потока (RFC 9113 §5.1)
			 *
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
			 * @brief Роль локального эндпоинта на соединении
			 *
			 */
			enum class endpoint_t : uint8_t {
				CLIENT = 0x00, // Нечётные stream id инициируем мы
				SERVER = 0x01  // Чётные stream id (push) инициируем мы
			};

			/**
			 * @brief Результат пошаговой обработки/разбора
			 *
			 */
			enum class status_t : uint8_t {
				OK         = 0x00, // Успешно, можно продолжать
				INCOMPLETE = 0x01, // Данных недостаточно - нужен ещё ввод
				ERROR      = 0x02  // Ошибка протокола - см. сопутствующий error_t
			};

			/**
			 * @brief Функция получения человекочитаемого названия типа фрейма
			 *
			 * @param type тип фрейма
			 * @return     название типа фрейма
			 */
			const char * frameName(const frame_t type) noexcept;

			/**
			 * @brief Функция получения человекочитаемого названия кода ошибки
			 *
			 * @param code код ошибки протокола
			 * @return     название кода ошибки
			 */
			const char * errorName(const error_t code) noexcept;
		}
	};
};

#endif // __AWH_HTTP_PARSER_HTTP2_H2__
