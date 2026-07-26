/**
 * @file: h3.hpp
 * @date: 2026-07-27
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл констант протокола HTTP/3 (RFC 9114) — перечисления типов кадров, типов однонаправленных
 *        потоков, параметров SETTINGS, состояний потоков и кодов ошибок, общие для слоя кадров, QPACK-кодека и парсера сессии
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_HTTP_PARSER_HTTP3_H3__
#define __AWH_HTTP_PARSER_HTTP3_H3__

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
		 * @brief Пространство имён внутренних слоёв протокола HTTP/3 (RFC 9114) и QPACK (RFC 9204)
		 *
		 * @details Содержит базовые типы и константы протокола, общие для слоя кадров (frame.hpp),
		 *          QPACK-кодека (qpack.hpp) и парсера сессии (http.hpp). Логики здесь нет —
		 *          только перечисления, константы протокола и POD-структуры.
		 *
		 * @note Все идентификаторы протокола (типы кадров, типы потоков, параметры SETTINGS,
		 *       коды ошибок) закодированы как целые переменной длины QUIC (RFC 9000 §16),
		 *       поэтому их область значений - 62 бита, а не 8 или 16 бит, как в HTTP/2
		 *
		 */
		namespace h3 {
			/**
			 * @brief Пространство имён констант протокола (RFC 9114)
			 *
			 */
			namespace proto {
				/**
				 * @brief Идентификатор ALPN для HTTP/3
				 *
				 * @note Открытого варианта у HTTP/3 нет: протокол определён только поверх QUIC,
				 *       а QUIC обязателен к использованию с TLS 1.3 (RFC 9001)
				 *
				 */
				static constexpr string_view ALPN = "h3";
				/**
				 * @brief Максимальное значение целого переменной длины QUIC (2^62-1)
				 *
				 * @details Верхняя граница для любого идентификатора протокола: типа кадра,
				 *          типа потока, параметра SETTINGS, идентификатора потока и push
				 *
				 */
				static constexpr uint64_t MAX_VARINT = 0x3FFFFFFFFFFFFFFFULL;
				/**
				 * @brief Шаг последовательности зарезервированных идентификаторов (RFC 9114 §7.2.8)
				 *
				 * @details Типы кадров и параметры SETTINGS вида (0x1F * N + 0x21) зарезервированы
				 *          намеренно и обязаны игнорироваться: ими проверяется, что реализация
				 *          не считает набор идентификаторов закрытым
				 *
				 */
				static constexpr uint64_t GREASE_STEP = 0x1F;
				/**
				 * @brief Смещение последовательности зарезервированных идентификаторов (RFC 9114 §7.2.8)
				 *
				 */
				static constexpr uint64_t GREASE_BASE = 0x21;
				/**
				 * @brief Размер динамической таблицы QPACK по умолчанию (RFC 9204 §5)
				 *
				 * @note Ноль означает, что динамическая таблица не используется вовсе: это
				 *       значение по умолчанию самого протокола, а не выбор реализации
				 *
				 */
				static constexpr uint64_t DEFAULT_QPACK_TABLE_CAPACITY = 0;
				/**
				 * @brief Число потоков, которым разрешено ожидать пополнения таблицы QPACK (RFC 9204 §5)
				 *
				 */
				static constexpr uint64_t DEFAULT_QPACK_BLOCKED_STREAMS = 0;
				/**
				 * @brief Размер динамической таблицы QPACK, анонсируемый нами
				 *
				 * @note Совпадает с размером таблицы HPACK по умолчанию: объём тот же,
				 *       а сравнение сжатия с HTTP/2 при равных таблицах осмысленно
				 *
				 */
				static constexpr uint64_t QPACK_TABLE_CAPACITY = 4096;
				/**
				 * @brief Число потоков, которым мы разрешаем ожидать пополнения таблицы QPACK
				 *
				 * @note Значение по умолчанию подобрано консервативно: каждый заблокированный
				 *       поток удерживает разобранный, но не выданный наружу блок заголовков
				 *
				 */
				static constexpr uint64_t QPACK_BLOCKED_STREAMS = 16;
				/**
				 * @brief Максимальный размер секции полей
				 *
				 * @note 0 - без лимита в SETTINGS, действует maxHeadersTotal
				 *
				 */
				static constexpr uint64_t MAX_FIELD_SECTION_SIZE = 0;
				/**
				 * @brief Максимальное число одновременных запросов в соединении
				 *
				 * @details В HTTP/3 лимит одновременных потоков задаёт транспорт параметром
				 *          initial_max_streams_bidi, а не сам HTTP: у протокола нет параметра,
				 *          подобного SETTINGS_MAX_CONCURRENT_STREAMS. Значение служит границей
				 *          карты потоков парсера и подсказкой для настройки транспорта
				 *
				 */
				static constexpr uint64_t MAX_COUNT_STREAMS = 128;
				/**
				 * @brief Срочность потока по умолчанию (RFC 9218 §4.1)
				 *
				 */
				static constexpr uint8_t DEFAULT_URGENCY = 3;
				/**
				 * @brief Наименее срочный уровень расширенного приоритета (RFC 9218 §4.1)
				 *
				 */
				static constexpr uint8_t MAX_URGENCY = 7;
			};

			/**
			 * @brief Роль локального эндпоинта на соединении
			 *
			 */
			enum class endpoint_t : uint8_t {
				CLIENT = 0x00, // Двунаправленные потоки запросов инициируем мы
				SERVER = 0x01  // Двунаправленные потоки запросов инициирует пир
			};

			/**
			 * @brief Результат пошаговой обработки/разбора
			 *
			 */
			enum class status_t : uint8_t {
				OK         = 0x00, // Успешно, можно продолжать
				INCOMPLETE = 0x01, // Данных недостаточно - нужен ещё ввод
				BLOCKED    = 0x02, // Поток ждёт пополнения динамической таблицы QPACK (RFC 9204 §2.1.2)
				ERROR      = 0x03  // Ошибка протокола - см. сопутствующий error_t
			};

			/**
			 * @brief Состояние потока запроса
			 *
			 * @details В отличие от HTTP/2 состояние потока в HTTP/3 ведёт транспорт: направления
			 *          закрываются признаком FIN потока QUIC, а аварийное закрытие выполняется
			 *          кадрами RESET_STREAM и STOP_SENDING транспорта, а не кадром HTTP.
			 *          Резервирования потока под push здесь нет: PUSH_PROMISE лишь обещает
			 *          идентификатор, а сам push приходит отдельным однонаправленным потоком
			 *
			 */
			enum class stream_state_t : uint8_t {
				IDLE               = 0x00, // Поток ещё не использован
				OPEN               = 0x01, // Оба направления открыты
				HALF_CLOSED_LOCAL  = 0x02, // Мы закрыли своё направление признаком FIN
				HALF_CLOSED_REMOTE = 0x03, // Пир закрыл своё направление признаком FIN
				CLOSED             = 0x04  // Поток завершён в обоих направлениях
			};

			/**
			 * @brief Тип однонаправленного потока (RFC 9114 §6.2, RFC 9204 §4.2)
			 *
			 * @details Первым целым переменной длины в однонаправленном потоке идёт его тип.
			 *          Поток неизвестного типа не является ошибкой: его содержимое отбрасывается,
			 *          а отправителю посылается STOP_SENDING с кодом H3_STREAM_CREATION_ERROR
			 *
			 */
			enum class unistream_t : uint64_t {
				CONTROL       = 0x00, // Управляющий поток соединения (RFC 9114 §6.2.1)
				PUSH          = 0x01, // Поток server push (RFC 9114 §6.2.2)
				QPACK_ENCODER = 0x02, // Поток инструкций кодера QPACK (RFC 9204 §4.2.1)
				QPACK_DECODER = 0x03  // Поток инструкций декодера QPACK (RFC 9204 §4.2.2)
			};

			/**
			 * @brief Тип кадра (RFC 9114 §7.2, §11.2.1) — значение поля Type (целое переменной длины)
			 *
			 */
			enum class frame_t : uint64_t {
				DATA         = 0x00, // Данные тела (RFC 9114 §7.2.1)
				HEADERS      = 0x01, // Секция полей заголовков либо трейлеров (RFC 9114 §7.2.2)
				CANCEL_PUSH  = 0x03, // Отмена обещанного push (RFC 9114 §7.2.3)
				SETTINGS     = 0x04, // Параметры соединения (RFC 9114 §7.2.4)
				PUSH_PROMISE = 0x05, // Server push - обещание запроса (RFC 9114 §7.2.5)
				GOAWAY       = 0x07, // Завершение соединения (RFC 9114 §7.2.6)
				MAX_PUSH_ID  = 0x0D, // Верхняя граница идентификаторов push (RFC 9114 §7.2.7)
				/**
				 * Обновление расширенного приоритета потока запроса (RFC 9218 §7.2)
				 */
				PRIORITY_UPDATE_REQUEST = 0x0F0700,
				/**
				 * Обновление расширенного приоритета потока push (RFC 9218 §7.2)
				 */
				PRIORITY_UPDATE_PUSH = 0x0F0701
			};

			/**
			 * @brief Идентификаторы параметров SETTINGS (RFC 9114 §7.2.4.1, RFC 9204 §5)
			 *
			 */
			enum class setting_t : uint64_t {
				QPACK_MAX_TABLE_CAPACITY = 0x01, // Размер динамической таблицы QPACK (по умолчанию 0)
				MAX_FIELD_SECTION_SIZE   = 0x06, // Лимит размера секции полей (по умолчанию без лимита)
				QPACK_BLOCKED_STREAMS    = 0x07, // Число потоков, ожидающих пополнения таблицы (по умолчанию 0)
				ENABLE_CONNECT_PROTOCOL  = 0x08  // Разрешён ли расширенный CONNECT (0/1, по умолчанию 0) - RFC 9220 §3
			};

			/**
			 * @brief Коды ошибок HTTP/3 (RFC 9114 §8.1, RFC 9204 §6)
			 *
			 * @details Передаются в кадре CONNECTION_CLOSE транспорта для ошибок уровня соединения
			 *          и в кадрах RESET_STREAM и STOP_SENDING - для ошибок уровня потока.
			 *          Собственного кадра для сообщения об ошибке у HTTP/3 нет
			 *
			 */
			enum class error_t : uint64_t {
				H3_NO_ERROR                = 0x0100, // Штатное завершение
				H3_GENERAL_PROTOCOL_ERROR  = 0x0101, // Нарушение протокола без более точного кода
				H3_INTERNAL_ERROR          = 0x0102, // Внутренняя ошибка реализации
				H3_STREAM_CREATION_ERROR   = 0x0103, // Поток создан или использован недопустимым образом
				H3_CLOSED_CRITICAL_STREAM  = 0x0104, // Закрыт поток, обязанный жить всё соединение
				H3_FRAME_UNEXPECTED        = 0x0105, // Кадр недопустим в этом потоке либо в этот момент
				H3_FRAME_ERROR             = 0x0106, // Кадр нарушает требования к своей нагрузке
				H3_EXCESSIVE_LOAD          = 0x0107, // Обнаружено чрезмерное поведение (флуд)
				H3_ID_ERROR                = 0x0108, // Идентификатор вне допустимых границ
				H3_SETTINGS_ERROR          = 0x0109, // Недопустимое содержимое кадра SETTINGS
				H3_MISSING_SETTINGS        = 0x010A, // Управляющий поток начат не кадром SETTINGS
				H3_REQUEST_REJECTED        = 0x010B, // Запрос отклонён до обработки - можно повторить
				H3_REQUEST_CANCELLED       = 0x010C, // Запрос отменён либо его обработка прекращена
				H3_REQUEST_INCOMPLETE      = 0x010D, // Поток запроса закрыт до полной передачи сообщения
				H3_MESSAGE_ERROR           = 0x010E, // Сообщение нарушает семантику HTTP
				H3_CONNECT_ERROR           = 0x010F, // Соединение метода CONNECT оборвалось либо не установлено
				H3_VERSION_FALLBACK        = 0x0110, // Запрошенный ресурс доступен только по другой версии HTTP
				QPACK_DECOMPRESSION_FAILED = 0x0200, // Секция полей не разбирается - состояние QPACK разошлось
				QPACK_ENCODER_STREAM_ERROR = 0x0201, // Ошибка в потоке инструкций кодера QPACK
				QPACK_DECODER_STREAM_ERROR = 0x0202  // Ошибка в потоке инструкций декодера QPACK
			};

			/**
			 * @brief Функция проверки принадлежности идентификатора зарезервированной последовательности
			 *
			 * @details Идентификаторы вида (0x1F * N + 0x21) зарезервированы RFC 9114 §7.2.8 и обязаны
			 *          игнорироваться. Пир отправляет их намеренно, проверяя, что реализация не
			 *          обрывает соединение на незнакомом идентификаторе
			 *
			 * @param value проверяемый идентификатор типа кадра либо параметра SETTINGS
			 * @return      признак принадлежности зарезервированной последовательности
			 *
			 */
			__AWH_SHARED_EXPORT__ bool reserved(const uint64_t value) noexcept;

			/**
			 * @brief Функция проверки идентификатора на изъятый из употребления в HTTP/3
			 *
			 * @details Типы кадров 0x02, 0x06, 0x08 и 0x09 занимали в HTTP/2 кадры PRIORITY, PING,
			 *          WINDOW_UPDATE и CONTINUATION. В HTTP/3 они не переиспользуются, и их
			 *          получение обязано обрывать соединение с H3_FRAME_UNEXPECTED (RFC 9114 §11.2.1) —
			 *          иначе пир, ошибочно отправляющий кадры HTTP/2, остался бы незамеченным
			 *
			 * @param type проверяемый тип кадра
			 * @return     признак изъятого из употребления типа кадра
			 *
			 */
			__AWH_SHARED_EXPORT__ bool retired(const uint64_t type) noexcept;

			/**
			 * @brief Функция проверки параметра SETTINGS на изъятый из употребления в HTTP/3
			 *
			 * @details Параметры 0x02, 0x03, 0x04 и 0x05 занимали в HTTP/2 ENABLE_PUSH,
			 *          MAX_CONCURRENT_STREAMS, INITIAL_WINDOW_SIZE и MAX_FRAME_SIZE. Их получение
			 *          обязано обрывать соединение с H3_SETTINGS_ERROR (RFC 9114 §7.2.4.1)
			 *
			 * @param identifier проверяемый идентификатор параметра
			 * @return           признак изъятого из употребления параметра
			 *
			 */
			__AWH_SHARED_EXPORT__ bool retiredSetting(const uint64_t identifier) noexcept;

			/**
			 * @brief Функция получения человекочитаемого названия типа кадра
			 *
			 * @param type тип кадра
			 * @return     название типа кадра
			 *
			 */
			__AWH_SHARED_EXPORT__ string_view frameName(const frame_t type) noexcept;

			/**
			 * @brief Функция получения человекочитаемого названия типа однонаправленного потока
			 *
			 * @param type тип однонаправленного потока
			 * @return     название типа потока
			 *
			 */
			__AWH_SHARED_EXPORT__ string_view unistreamName(const unistream_t type) noexcept;

			/**
			 * @brief Функция получения человекочитаемого названия кода ошибки
			 *
			 * @param code код ошибки протокола
			 * @return     название кода ошибки
			 *
			 */
			__AWH_SHARED_EXPORT__ string_view errorName(const error_t code) noexcept;
		}
	};
};

#endif // __AWH_HTTP_PARSER_HTTP3_H3__
