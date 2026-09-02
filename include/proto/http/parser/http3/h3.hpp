/**
 * @file h3.hpp
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
 * @brief Заголовочный файл констант протокола HTTP/3 (RFC 9114) — перечисления типов кадров, типов однонаправленных
 *        потоков, параметров SETTINGS, состояний потоков и кодов ошибок, общие для слоя кадров, QPACK-кодека и парсера сессии
 *
 * \~english
 * @brief Header file of the constants of the HTTP/3 protocol (RFC 9114) — the enumerations of the types of the frames, of the types of the unidirectional
 *        streams, of the parameters of SETTINGS, of the states of the streams and of the error codes, common for the layer of the frames, the QPACK codec and the parser of the session
 *
 * \~
 *
 * @copyright Copyright © 2026
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
		 * \~english
		 * @brief Namespace of the internal layers of the HTTP/3 protocol (RFC 9114) and of QPACK (RFC 9204)
		 * @details Contains the base types and the constants of the protocol, common for the layer of the frames (frame.hpp),
		 *          the QPACK codec (qpack.hpp) and the parser of the session (http.hpp). There is no logic here —
		 *          only the enumerations, the constants of the protocol and the POD structures.
		 * @note All the identifiers of the protocol (the types of the frames, the types of the streams, the parameters of SETTINGS,
		 *       the error codes) are encoded as the integers of a variable length of QUIC (RFC 9000 §16),
		 *       therefore their range of the values is 62 bits rather than 8 or 16 bits as in HTTP/2
		 *
		 * \~
		 */
		namespace h3 {
			/**
			 * \~russian
			 * @brief Пространство имён констант протокола (RFC 9114)
			 *
			 * \~english
			 * @brief Namespace of the constants of the protocol (RFC 9114)
			 *
			 * \~
			 */
			namespace proto {
				/**
				 * \~russian
				 * @brief Идентификатор ALPN для HTTP/3
				 *
				 * @note Открытого варианта у HTTP/3 нет: протокол определён только поверх QUIC,
				 *       а QUIC обязателен к использованию с TLS 1.3 (RFC 9001)
				 *
				 * \~english
				 * @brief Identifier of ALPN for HTTP/3
				 * @note HTTP/3 has no open variety: the protocol is determined only over QUIC,
				 *       while QUIC is obligatory for the use with TLS 1.3 (RFC 9001)
				 *
				 * \~
				 */
				static constexpr string_view ALPN = "h3";
				/**
				 * \~russian
				 * @brief Максимальное значение целого переменной длины QUIC (2^62-1)
				 *
				 * @details Верхняя граница для любого идентификатора протокола: типа кадра,
				 *          типа потока, параметра SETTINGS, идентификатора потока и push
				 *
				 * \~english
				 * @brief Largest value of an integer of a variable length of QUIC (2^62-1)
				 * @details The upper boundary for any identifier of the protocol: of a type of a frame,
				 *          of a type of a stream, of a parameter of SETTINGS, of an identifier of a stream and of a push
				 *
				 * \~
				 */
				static constexpr uint64_t MAX_VARINT = 0x3FFFFFFFFFFFFFFFULL;
				/**
				 * \~russian
				 * @brief Шаг последовательности зарезервированных идентификаторов (RFC 9114 §7.2.8)
				 *
				 * @details Типы кадров и параметры SETTINGS вида (0x1F * N + 0x21) зарезервированы
				 *          намеренно и обязаны игнорироваться: ими проверяется, что реализация
				 *          не считает набор идентификаторов закрытым
				 *
				 * \~english
				 * @brief Step of the sequence of the reserved identifiers (RFC 9114 §7.2.8)
				 * @details The types of the frames and the parameters of SETTINGS of the form (0x1F * N + 0x21) are reserved
				 *          deliberately and are obliged to be ignored: by them it is checked that an implementation
				 *          does not consider the collection of the identifiers closed
				 *
				 * \~
				 */
				static constexpr uint64_t GREASE_STEP = 0x1F;
				/**
				 * \~russian
				 * @brief Смещение последовательности зарезервированных идентификаторов (RFC 9114 §7.2.8)
				 *
				 * \~english
				 * @brief Displacement of the sequence of the reserved identifiers (RFC 9114 §7.2.8)
				 *
				 * \~
				 */
				static constexpr uint64_t GREASE_BASE = 0x21;
				/**
				 * \~russian
				 * @brief Размер динамической таблицы QPACK по умолчанию (RFC 9204 §5)
				 *
				 * @note Ноль означает, что динамическая таблица не используется вовсе: это
				 *       значение по умолчанию самого протокола, а не выбор реализации
				 *
				 * \~english
				 * @brief Size of the dynamic table of QPACK by default (RFC 9204 §5)
				 * @note A zero means that the dynamic table is not used at all: this is
				 *       the value by default of the protocol itself rather than a choice of the implementation
				 *
				 * \~
				 */
				static constexpr uint64_t DEFAULT_QPACK_TABLE_CAPACITY = 0;
				/**
				 * \~russian
				 * @brief Число потоков, которым разрешено ожидать пополнения таблицы QPACK (RFC 9204 §5)
				 *
				 * \~english
				 * @brief Number of the streams which are allowed to wait for a replenishment of the table of QPACK (RFC 9204 §5)
				 *
				 * \~
				 */
				static constexpr uint64_t DEFAULT_QPACK_BLOCKED_STREAMS = 0;
				/**
				 * \~russian
				 * @brief Размер динамической таблицы QPACK, анонсируемый нами
				 *
				 * @note Совпадает с размером таблицы HPACK по умолчанию: объём тот же,
				 *       а сравнение сжатия с HTTP/2 при равных таблицах осмысленно
				 *
				 * \~english
				 * @brief Size of the dynamic table of QPACK announced by us
				 * @note It coincides with the size of the table of HPACK by default: the volume is the same,
				 *       and a comparison of the compression with HTTP/2 at equal tables is meaningful
				 *
				 * \~
				 */
				static constexpr uint64_t QPACK_TABLE_CAPACITY = 4096;
				/**
				 * \~russian
				 * @brief Число потоков, которым мы разрешаем ожидать пополнения таблицы QPACK
				 *
				 * @note Значение по умолчанию подобрано консервативно: каждый заблокированный
				 *       поток удерживает разобранный, но не выданный наружу блок заголовков
				 *
				 * \~english
				 * @brief Number of the streams which we allow to wait for a replenishment of the table of QPACK
				 * @note The value by default is selected conservatively: every blocked
				 *       stream holds a parsed but not issued outside block of the headers
				 *
				 * \~
				 */
				static constexpr uint64_t QPACK_BLOCKED_STREAMS = 16;
				/**
				 * \~russian
				 * @brief Максимальный размер секции полей
				 *
				 * @note 0 - без лимита в SETTINGS, действует maxHeadersTotal
				 *
				 * \~english
				 * @brief Largest size of a section of the fields
				 * @note 0 - without a limit in SETTINGS, maxHeadersTotal is in force
				 *
				 * \~
				 */
				static constexpr uint64_t MAX_FIELD_SECTION_SIZE = 0;
				/**
				 * \~russian
				 * @brief Максимальное число одновременных запросов в соединении
				 *
				 * @details В HTTP/3 лимит одновременных потоков задаёт транспорт параметром
				 *          initial_max_streams_bidi, а не сам HTTP: у протокола нет параметра,
				 *          подобного SETTINGS_MAX_CONCURRENT_STREAMS. Значение служит границей
				 *          карты потоков парсера и подсказкой для настройки транспорта
				 *
				 * \~english
				 * @brief Largest number of the simultaneous requests in a connection
				 * @details In HTTP/3 the limit of the simultaneous streams is set by the transport by the parameter
				 *          initial_max_streams_bidi rather than by HTTP itself: the protocol has no parameter
				 *          similar to SETTINGS_MAX_CONCURRENT_STREAMS. The value serves as the boundary
				 *          of the map of the streams of the parser and as a hint for the setting of the transport
				 *
				 * \~
				 */
				static constexpr uint64_t MAX_COUNT_STREAMS = 128;
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
				CLIENT = 0x00, // Двунаправленные потоки запросов инициируем мы
				SERVER = 0x01  // Двунаправленные потоки запросов инициирует пир
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
				BLOCKED    = 0x02, // Поток ждёт пополнения динамической таблицы QPACK (RFC 9204 §2.1.2)
				ERROR      = 0x03  // Ошибка протокола - см. сопутствующий error_t
			};

			/**
			 * \~russian
			 * @brief Состояние потока запроса
			 *
			 * @details В отличие от HTTP/2 состояние потока в HTTP/3 ведёт транспорт: направления
			 *          закрываются признаком FIN потока QUIC, а аварийное закрытие выполняется
			 *          кадрами RESET_STREAM и STOP_SENDING транспорта, а не кадром HTTP.
			 *          Резервирования потока под push здесь нет: PUSH_PROMISE лишь обещает
			 *          идентификатор, а сам push приходит отдельным однонаправленным потоком
			 *
			 * \~english
			 * @brief State of a stream of a request
			 * @details Unlike HTTP/2 the state of a stream in HTTP/3 is conducted by the transport: the directions
			 *          are closed by the flag FIN of a stream of QUIC, while an emergency closing is performed
			 *          by the frames RESET_STREAM and STOP_SENDING of the transport rather than by a frame of HTTP.
			 *          There is no reservation of a stream for a push here: PUSH_PROMISE only promises
			 *          an identifier, while the push itself comes as a separate unidirectional stream
			 *
			 * \~
			 */
			enum class stream_state_t : uint8_t {
				IDLE               = 0x00, // Поток ещё не использован
				OPEN               = 0x01, // Оба направления открыты
				HALF_CLOSED_LOCAL  = 0x02, // Мы закрыли своё направление признаком FIN
				HALF_CLOSED_REMOTE = 0x03, // Пир закрыл своё направление признаком FIN
				CLOSED             = 0x04  // Поток завершён в обоих направлениях
			};

			/**
			 * \~russian
			 * @brief Тип однонаправленного потока (RFC 9114 §6.2, RFC 9204 §4.2)
			 *
			 * @details Первым целым переменной длины в однонаправленном потоке идёт его тип.
			 *          Поток неизвестного типа не является ошибкой: его содержимое отбрасывается,
			 *          а отправителю посылается STOP_SENDING с кодом H3_STREAM_CREATION_ERROR
			 *
			 * \~english
			 * @brief Type of a unidirectional stream (RFC 9114 §6.2, RFC 9204 §4.2)
			 * @details The first integer of a variable length in a unidirectional stream is its type.
			 *          A stream of an unknown type is not an error: its content is discarded,
			 *          while STOP_SENDING with the code H3_STREAM_CREATION_ERROR is sent to the sender
			 *
			 * \~
			 */
			enum class unistream_t : uint64_t {
				CONTROL       = 0x00, // Управляющий поток соединения (RFC 9114 §6.2.1)
				PUSH          = 0x01, // Поток server push (RFC 9114 §6.2.2)
				QPACK_ENCODER = 0x02, // Поток инструкций кодера QPACK (RFC 9204 §4.2.1)
				QPACK_DECODER = 0x03  // Поток инструкций декодера QPACK (RFC 9204 §4.2.2)
			};

			/**
			 * \~russian
			 * @brief Тип кадра (RFC 9114 §7.2, §11.2.1) — значение поля Type (целое переменной длины)
			 *
			 * \~english
			 * @brief Type of a frame (RFC 9114 §7.2, §11.2.1) — the value of the field Type (an integer of a variable length)
			 *
			 * \~
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
			 * \~russian
			 * @brief Идентификаторы параметров SETTINGS (RFC 9114 §7.2.4.1, RFC 9204 §5)
			 *
			 * \~english
			 * @brief Identifiers of the parameters of SETTINGS (RFC 9114 §7.2.4.1, RFC 9204 §5)
			 *
			 * \~
			 */
			enum class setting_t : uint64_t {
				QPACK_MAX_TABLE_CAPACITY = 0x01, // Размер динамической таблицы QPACK (по умолчанию 0)
				MAX_FIELD_SECTION_SIZE   = 0x06, // Лимит размера секции полей (по умолчанию без лимита)
				QPACK_BLOCKED_STREAMS    = 0x07, // Число потоков, ожидающих пополнения таблицы (по умолчанию 0)
				ENABLE_CONNECT_PROTOCOL  = 0x08  // Разрешён ли расширенный CONNECT (0/1, по умолчанию 0) - RFC 9220 §3
			};

			/**
			 * \~russian
			 * @brief Коды ошибок HTTP/3 (RFC 9114 §8.1, RFC 9204 §6)
			 *
			 * @details Передаются в кадре CONNECTION_CLOSE транспорта для ошибок уровня соединения
			 *          и в кадрах RESET_STREAM и STOP_SENDING - для ошибок уровня потока.
			 *          Собственного кадра для сообщения об ошибке у HTTP/3 нет
			 *
			 * \~english
			 * @brief Error codes of HTTP/3 (RFC 9114 §8.1, RFC 9204 §6)
			 * @details They are transmitted in the frame CONNECTION_CLOSE of the transport for the errors of the level of the connection
			 *          and in the frames RESET_STREAM and STOP_SENDING - for the errors of the level of a stream.
			 *          HTTP/3 has no frame of its own for a report about an error
			 *
			 * \~
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
			 * \~russian
			 * @brief Функция проверки принадлежности идентификатора зарезервированной последовательности
			 *
			 * @details Идентификаторы вида (0x1F * N + 0x21) зарезервированы RFC 9114 §7.2.8 и обязаны
			 *          игнорироваться. Пир отправляет их намеренно, проверяя, что реализация не
			 *          обрывает соединение на незнакомом идентификаторе
			 *
			 * @param value проверяемый идентификатор типа кадра либо параметра SETTINGS
			 * @return      признак принадлежности зарезервированной последовательности
			 *
			 * \~english
			 * @brief Function of checking the belonging of an identifier to the reserved sequence
			 * @details The identifiers of the form (0x1F * N + 0x21) are reserved by RFC 9114 §7.2.8 and are obliged
			 *          to be ignored. A peer sends them deliberately, checking that an implementation does not
			 *          break the connection at an unfamiliar identifier
			 * @param value identifier of a type of a frame or of a parameter of SETTINGS being checked
			 * @return      flag of the belonging to the reserved sequence
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool reserved(const uint64_t value) noexcept;

			/**
			 * \~russian
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
			 * \~english
			 * @brief Function of checking an identifier for one withdrawn from the use in HTTP/3
			 * @details The types of the frames 0x02, 0x06, 0x08 and 0x09 were occupied in HTTP/2 by the frames PRIORITY, PING,
			 *          WINDOW_UPDATE and CONTINUATION. In HTTP/3 they are not reused, and their
			 *          receipt is obliged to break the connection with H3_FRAME_UNEXPECTED (RFC 9114 §11.2.1) —
			 *          otherwise a peer erroneously sending the frames of HTTP/2 would remain unnoticed
			 * @param type type of the frame being checked
			 * @return     flag of a type of a frame withdrawn from the use
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool retired(const uint64_t type) noexcept;

			/**
			 * \~russian
			 * @brief Функция проверки параметра SETTINGS на изъятый из употребления в HTTP/3
			 *
			 * @details Параметры 0x02, 0x03, 0x04 и 0x05 занимали в HTTP/2 ENABLE_PUSH,
			 *          MAX_CONCURRENT_STREAMS, INITIAL_WINDOW_SIZE и MAX_FRAME_SIZE. Их получение
			 *          обязано обрывать соединение с H3_SETTINGS_ERROR (RFC 9114 §7.2.4.1)
			 *
			 * @param identifier проверяемый идентификатор параметра
			 * @return           признак изъятого из употребления параметра
			 *
			 * \~english
			 * @brief Function of checking a parameter of SETTINGS for one withdrawn from the use in HTTP/3
			 * @details The parameters 0x02, 0x03, 0x04 and 0x05 were occupied in HTTP/2 by ENABLE_PUSH,
			 *          MAX_CONCURRENT_STREAMS, INITIAL_WINDOW_SIZE and MAX_FRAME_SIZE. Their receipt
			 *          is obliged to break the connection with H3_SETTINGS_ERROR (RFC 9114 §7.2.4.1)
			 * @param identifier identifier of the parameter being checked
			 * @return           flag of a parameter withdrawn from the use
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool retiredSetting(const uint64_t identifier) noexcept;

			/**
			 * \~russian
			 * @brief Функция получения человекочитаемого названия типа кадра
			 *
			 * @param type тип кадра
			 * @return     название типа кадра
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
			 * @brief Функция получения человекочитаемого названия типа однонаправленного потока
			 *
			 * @param type тип однонаправленного потока
			 * @return     название типа потока
			 *
			 * \~english
			 * @brief Function of getting the human-readable name of a type of a unidirectional stream
			 * @param type type of the unidirectional stream
			 * @return     name of the type of the stream
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ string_view unistreamName(const unistream_t type) noexcept;

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

#endif // __AWH_HTTP_PARSER_HTTP3_H3__
