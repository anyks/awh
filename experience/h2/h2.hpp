/**
 * @file h2.hpp
 * @brief Базовые типы и константы протокола HTTP/2 (RFC 9113) и HPACK (RFC 7541).
 *
 * Здесь нет логики — только перечисления, константы протокола и POD-структуры,
 * общие для framing-слоя (frame.hpp), HPACK (hpack.hpp) и сессии (session.hpp).
 *
 * Дизайн в стиле experience/http: namespace + свободные функции + POD-структуры,
 * zero-copy, без C-инфраструктуры. nghttp2 используется как референс (см. README.md).
 */

#ifndef AWH_EXPERIENCE_H2_HPP
#define AWH_EXPERIENCE_H2_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace awh {
	namespace http2 {
		/**
		 * @brief Константы протокола (RFC 9113).
		 */
		namespace proto {
			/// Клиентский connection preface (24 октета), отправляется до первого SETTINGS.
			static constexpr std::string_view PREFACE = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
			/// Идентификатор ALPN для HTTP/2 поверх TLS.
			static constexpr std::string_view ALPN = "h2";
			/// Идентификатор ALPN для HTTP/2 поверх открытого TCP (h2c).
			static constexpr std::string_view ALPN_CLEARTEXT = "h2c";

			/// Размер заголовка любого фрейма в октетах (RFC 9113 §4.1).
			static constexpr size_t FRAME_HEADER_SIZE = 9;
			/// Максимально допустимое значение поля Length (24 бита).
			static constexpr uint32_t MAX_FRAME_LENGTH = 0xFFFFFF;
			/// Значение SETTINGS_MAX_FRAME_SIZE по умолчанию (16 КиБ).
			static constexpr uint32_t DEFAULT_MAX_FRAME_SIZE = 16384;
			/// Нижняя/верхняя граница для SETTINGS_MAX_FRAME_SIZE.
			static constexpr uint32_t MIN_MAX_FRAME_SIZE = 16384;
			static constexpr uint32_t MAX_MAX_FRAME_SIZE = 16777215;

			/// Начальный размер окна управления потоком (RFC 9113 §6.9.2).
			static constexpr int32_t DEFAULT_WINDOW_SIZE = 65535;
			/// Максимальное значение окна (2^31 - 1).
			static constexpr int32_t MAX_WINDOW_SIZE = 0x7FFFFFFF;

			/// Размер динамической таблицы HPACK по умолчанию (RFC 7541).
			static constexpr uint32_t DEFAULT_HEADER_TABLE_SIZE = 4096;

			/// Маска для извлечения 31-битного идентификатора потока (сброс reserved-бита).
			static constexpr uint32_t STREAM_ID_MASK = 0x7FFFFFFF;
		}

		/**
		 * @brief Тип фрейма (RFC 9113 §6). Значение поля Type (8 бит).
		 */
		enum class frame_t : uint8_t {
			DATA          = 0x00, // §6.1  — данные тела
			HEADERS       = 0x01, // §6.2  — блок заголовков (открывает поток)
			PRIORITY      = 0x02, // §6.3  — приоритет (RFC 7540, deprecated)
			RST_STREAM    = 0x03, // §6.4  — аварийное закрытие потока
			SETTINGS      = 0x04, // §6.5  — параметры соединения
			PUSH_PROMISE  = 0x05, // §6.6  — server push (резервирование потока)
			PING          = 0x06, // §6.7  — проверка живости / измерение RTT
			GOAWAY        = 0x07, // §6.8  — завершение соединения
			WINDOW_UPDATE = 0x08, // §6.9  — обновление окна flow control
			CONTINUATION  = 0x09  // §6.10 — продолжение блока заголовков
		};

		/**
		 * @brief Флаги фреймов (RFC 9113 §6). Семантика зависит от типа фрейма.
		 *
		 * Один и тот же бит означает разное для разных фреймов, поэтому это набор
		 * констант, а не enum class.
		 */
		namespace flag {
			static constexpr uint8_t NONE        = 0x00;
			static constexpr uint8_t END_STREAM  = 0x01; // DATA, HEADERS
			static constexpr uint8_t ACK         = 0x01; // SETTINGS, PING
			static constexpr uint8_t END_HEADERS = 0x04; // HEADERS, PUSH_PROMISE, CONTINUATION
			static constexpr uint8_t PADDED      = 0x08; // DATA, HEADERS, PUSH_PROMISE
			static constexpr uint8_t PRIORITY    = 0x20; // HEADERS
		}

		/**
		 * @brief Идентификаторы параметров SETTINGS (RFC 9113 §6.5.2).
		 */
		enum class setting_t : uint16_t {
			HEADER_TABLE_SIZE      = 0x01, // размер динамической таблицы HPACK (по умолч. 4096)
			ENABLE_PUSH            = 0x02, // разрешён ли server push (0/1, по умолч. 1)
			MAX_CONCURRENT_STREAMS = 0x03, // лимит одновременных потоков (по умолч. без лимита)
			INITIAL_WINDOW_SIZE    = 0x04, // начальное окно потока (по умолч. 65535)
			MAX_FRAME_SIZE         = 0x05, // максимальный размер фрейма (по умолч. 16384)
			MAX_HEADER_LIST_SIZE   = 0x06  // лимит размера списка заголовков (по умолч. без лимита)
		};

		/**
		 * @brief Коды ошибок HTTP/2 (RFC 9113 §7). Используются в RST_STREAM и GOAWAY.
		 */
		enum class error_t : uint32_t {
			NO_ERROR            = 0x00, // штатное завершение
			PROTOCOL_ERROR      = 0x01, // нарушение протокола
			INTERNAL_ERROR      = 0x02, // внутренняя ошибка реализации
			FLOW_CONTROL_ERROR  = 0x03, // нарушение flow control
			SETTINGS_TIMEOUT    = 0x04, // не получен ACK на SETTINGS
			STREAM_CLOSED       = 0x05, // фрейм для закрытого потока
			FRAME_SIZE_ERROR    = 0x06, // некорректный размер фрейма
			REFUSED_STREAM      = 0x07, // поток отклонён до обработки
			CANCEL              = 0x08, // поток больше не нужен
			COMPRESSION_ERROR   = 0x09, // ошибка состояния HPACK
			CONNECT_ERROR       = 0x0A, // ошибка соединения для метода CONNECT
			ENHANCE_YOUR_CALM   = 0x0B, // обнаружено чрезмерное поведение (флуд)
			INADEQUATE_SECURITY = 0x0C, // недостаточный уровень безопасности TLS
			HTTP_1_1_REQUIRED   = 0x0D  // требуется откат на HTTP/1.1
		};

		/**
		 * @brief Состояния потока (RFC 9113 §5.1).
		 */
		enum class stream_state_t : uint8_t {
			IDLE,               // поток ещё не использован
			RESERVED_LOCAL,     // зарезервирован нами через PUSH_PROMISE
			RESERVED_REMOTE,    // зарезервирован пиром через PUSH_PROMISE
			OPEN,               // оба конца могут слать данные
			HALF_CLOSED_LOCAL,  // мы отправили END_STREAM
			HALF_CLOSED_REMOTE, // пир отправил END_STREAM
			CLOSED              // поток завершён
		};

		/**
		 * @brief Роль локального эндпоинта на соединении.
		 */
		enum class endpoint_t : uint8_t {
			CLIENT, // нечётные stream id инициируем мы
			SERVER  // чётные stream id (push) инициируем мы
		};

		/**
		 * @brief Результат пошаговой обработки/разбора.
		 */
		enum class status_t : uint8_t {
			OK,         // успешно, можно продолжать
			INCOMPLETE, // данных недостаточно — нужен ещё ввод
			ERROR       // ошибка протокола — см. сопутствующий error_t
		};

		/**
		 * @brief Человекочитаемое имя типа фрейма.
		 */
		const char * frameName(frame_t type) noexcept;

		/**
		 * @brief Человекочитаемое имя кода ошибки.
		 */
		const char * errorName(error_t code) noexcept;
	}
}

#endif // AWH_EXPERIENCE_H2_HPP
