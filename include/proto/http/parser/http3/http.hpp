/**
 * @file: http.hpp
 * @date: 2026-07-27
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл парсера сессии HTTP/3 (RFC 9114) — класс Parser_HTTP3, управляющий
 *        потоками запросов, однонаправленными потоками соединения, параметрами SETTINGS,
 *        состоянием кодека QPACK, приоритетами и лимитами безопасности
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_HTTP_PARSER_HTTP3__
#define __AWH_HTTP_PARSER_HTTP3__

/**
 * Стандартные заголовочные файлы
 */
#include <deque>
#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <string_view>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "h3.hpp"
#include "frame.hpp"
#include "qpack.hpp"
#include "../parser.hpp"
#include "../../headers.hpp"
#include "../../provider.hpp"
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
		 * @brief Класс парсера протокола HTTP/3 (RFC 9114)
		 *
		 * @details Как и парсер HTTP/2, работает на уровне СОЕДИНЕНИЯ, а не одного сообщения,
		 *          поэтому все события сопровождаются идентификатором потока. На этом сходство
		 *          заканчивается: HTTP/3 переносит на транспорт почти всё, что HTTP/2 делал сам.
		 *
		 *          Чего у этого парсера нет по сравнению с HTTP/2 и почему:
		 *          - **управления потоком**: окна ведёт QUIC, кадра WINDOW_UPDATE в HTTP/3 нет;
		 *          - **кадра RST_STREAM**: поток обрывается кадрами RESET_STREAM и STOP_SENDING
		 *            транспорта, поэтому парсер просит об этом обвязку, а не пишет кадр сам;
		 *          - **кадра PING**: проверка живости - забота транспорта;
		 *          - **кадра CONTINUATION**: длина кадра не ограничена, дробить секцию полей
		 *            не нужно, и целого класса атак CONTINUATION flood здесь не существует;
		 *          - **лимита одновременных потоков в SETTINGS**: его задаёт транспортный
		 *            параметр initial_max_streams_bidi;
		 *          - **своего кадра сообщения об ошибке**: код ошибки уходит в CONNECTION_CLOSE.
		 *
		 *          Что появилось взамен:
		 *          - **однонаправленные потоки**: управляющий, два потока QPACK и потоки push.
		 *            Первым целым переменной длины в таком потоке идёт его тип. Управляющий поток
		 *            и потоки QPACK обязаны жить всё соединение: их закрытие - ошибка соединения;
		 *          - **разбор с адресацией по потокам**: вход - parse(sid, ...), а не parse(...),
		 *            потому что единого байтового потока у соединения больше нет;
		 *          - **заблокированные потоки**: секция полей и инструкции QPACK идут разными
		 *            потоками и обгоняют друг друга, поэтому секция, пришедшая раньше нужных
		 *            вставок, откладывается и разбирается заново после их прихода.
		 *
		 *          Направление трафика задаёт роль эндпоинта:
		 *          - direct_t::REQUEST  - разбираем запросы клиента (мы - сервер);
		 *          - direct_t::RESPONSE - разбираем ответы сервера (мы - клиент).
		 *
		 * @note Парсер не зависит от транспорта: он разбирает байты потоков и формирует байты
		 *       потоков. Всё, что связано с QUIC, TLS и таймерами, остаётся за обвязкой.
		 *       Операции, которые парсер выполнить не может - открыть однонаправленный поток,
		 *       оборвать поток, закрыть соединение, - запрашиваются функциями обратного вызова
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Parser_HTTP3 : public parser_t {
			public:
				/**
				 * @brief Пополнение лимита частоты управляющих кадров (токенов в секунду)
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 *
				 */
				static constexpr uint64_t CTRL_LIMIT_RATE = (100);
				/**
				 * @brief Стартовый запас лимита частоты управляющих кадров
				 *
				 */
				static constexpr uint64_t CTRL_LIMIT_BURST = (1000);
				/**
				 * @brief Пополнение лимита частоты кадров приоритета (токенов в секунду)
				 *
				 * @note Лимит отдельный от управляющих кадров и заметно щедрее: клиент вправе
				 *       переставлять приоритеты на каждый загружаемый ресурс страницы
				 *
				 */
				static constexpr uint64_t PRIORITY_LIMIT_RATE = (500);
				/**
				 * @brief Стартовый запас лимита частоты кадров приоритета
				 *
				 */
				static constexpr uint64_t PRIORITY_LIMIT_BURST = (5000);
				/**
				 * @brief Число запоминаемых идентификаторов push
				 *
				 * @details Окно учёта отменённых и уже пришедших обещаний. Оно же задаёт
				 *          глубину, на которой ловится повторный поток одного обещания
				 *
				 */
				static constexpr size_t PUSH_HISTORY_CACHE = (64);
			public:
				/**
				 * @brief Тип кода ошибки протокола
				 *
				 */
				using error_t = h3::error_t;
			public:
				/**
				 * @brief Структура лимитов безопасности парсера
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Limits : parser_t::limits_t {
					public:
						/**
						 * Максимальный размер сжатой секции полей
						 *
						 * @details Длина кадра в HTTP/3 не ограничена протоколом, поэтому без
						 *          этого лимита отправитель одним кадром HEADERS задавал бы
						 *          потребление памяти получателем
						 *
						 */
						uint64_t maxHeaderSection;
						/**
						 * Максимальный размер нагрузки управляющего кадра
						 *
						 * @details Нагрузка управляющих кадров накапливается в буфере целиком,
						 *          поэтому её размер обязан быть ограничен
						 *
						 */
						uint64_t maxControlFrame;
						/**
						 * Максимальный размер неразобранного хвоста заблокированного потока
						 *
						 * @details Пока секция ждёт вставок QPACK, идущие следом кадры разобрать
						 *          нельзя и они копятся в буфере. Пир, не присылающий инструкций
						 *          кодера вовсе, иначе задавал бы потребление памяти получателем
						 *
						 */
						uint64_t maxBlockedTail;
						// Максимальное количество одновременно живых потоков запросов
						size_t maxStreams;
					public:
						// Пополнение лимита частоты управляющих кадров (токенов в секунду)
						uint64_t ctrlLimitRate;
						// Стартовый запас лимита частоты управляющих кадров
						uint64_t ctrlLimitBurst;
						// Пополнение лимита частоты кадров приоритета (токенов в секунду)
						uint64_t prioLimitRate;
						// Стартовый запас лимита частоты кадров приоритета
						uint64_t prioLimitBurst;
					public:
						/**
						 * @brief Конструктор
						 *
						 */
						explicit Limits() noexcept;
				} limits_t;
				/**
				 * @brief Структура параметров SETTINGS (RFC 9114 §7.2.4.1, RFC 9204 §5)
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Settings {
					public:
						// Размер динамической таблицы QPACK
						uint64_t qpackMaxTableCapacity;
						// Число потоков, которым разрешено ожидать пополнения таблицы QPACK
						uint64_t qpackBlockedStreams;
						// Максимальный размер секции полей (0 - без лимита)
						uint64_t maxFieldSectionSize;
						// Разрешён ли расширенный CONNECT (RFC 9220 §3)
						bool enableConnectProtocol;
					public:
						/**
						 * @brief Конструктор
						 *
						 */
						explicit Settings() noexcept;
				} settings_t;
			public:
				/**
				 * @brief Тип функции обратного вызова для обработки применённого SETTINGS пира
				 *
				 */
				using settings_callback_t = function <void (void)>;
				/**
				 * @brief Тип функции обратного вызова для обработки открытия нового потока
				 *
				 * @details Возврат false обрывает поток запросом RESET_STREAM с кодом
				 *          H3_REQUEST_REJECTED
				 *
				 * @param sid идентификатор потока
				 * @return    результат обработки (false - поток обрывается)
				 *
				 */
				using begin_callback_t = function <bool (const uint64_t)>;
				/**
				 * @brief Тип функции обратного вызова для обработки закрытия потока
				 *
				 * @param sid  идентификатор потока
				 * @param code код ошибки закрытия (H3_NO_ERROR - штатное закрытие)
				 *
				 */
				using close_callback_t = function <void (const uint64_t, const error_t)>;
				/**
				 * @brief Тип функции обратного вызова для обработки ошибки уровня соединения
				 *
				 * @details Своего кадра для сообщения об ошибке у HTTP/3 нет: код уходит
				 *          в кадре CONNECTION_CLOSE транспорта (RFC 9114 §8). Поэтому событие
				 *          одновременно и извещает об ошибке, и обязывает обвязку закрыть
				 *          соединение QUIC с этим кодом - разделять их было бы нечем
				 *
				 * @param code    код ошибки протокола
				 * @param message текстовое описание ошибки
				 *
				 */
				using error_callback_t = function <void (const error_t, const string_view)>;
				/**
				 * @brief Тип функции обратного вызова для обработки полученного GOAWAY
				 *
				 * @details В HTTP/3 кадр GOAWAY несёт единственное число и не несёт кода ошибки:
				 *          от сервера это идентификатор потока запроса, от клиента - идентификатор
				 *          push. Причина завершения сообщается транспортом в CONNECTION_CLOSE
				 *
				 * @param id идентификатор потока запроса либо идентификатор push
				 *
				 */
				using goaway_callback_t = function <void (const uint64_t)>;
				/**
				 * @brief Тип функции обратного вызова для обработки анонса server push (только клиент)
				 *
				 * @details Поля обещанного запроса приходят через header_callback_t и
				 *          provider_callback_t с идентификатором ассоциированного потока.
				 *          Возврат false отклоняет push кадром CANCEL_PUSH
				 *
				 * @param sid    идентификатор ассоциированного потока запроса
				 * @param pushId идентификатор обещанного push
				 * @return       результат обработки (false - push отклоняется)
				 *
				 */
				using push_callback_t = function <bool (const uint64_t, const uint64_t)>;
				/**
				 * @brief Тип функции обратного вызова для обработки фазы приёма сообщения потока
				 *
				 * @details Последовательность событий при приёме одного сообщения потока
				 *          совпадает с HTTP/1 и HTTP/2:
				 *          1. (BEGIN, NONE)    - получена первая секция полей потока
				 *          2. (END, HEADERS)   - секция полей доставлена (после провайдера)
				 *          3. (BEGIN, BODY)    - ожидается тело (поток не завершён секцией полей)
				 *          4. (END, BODY)      - тело полностью принято
				 *          5. (BEGIN, TRAILER) - получена секция трейлеров
				 *          6. (END, TRAILER)   - трейлеры доставлены (после провайдера с nullptr)
				 *          7. (END, NONE)      - сообщение потока полностью принято
				 *          Для обещанных запросов PUSH_PROMISE фазы не вызываются, как и для
				 *          информационных ответов сервера (1xx): такая секция промежуточная.
				 *          Возврат false обрывает поток
				 *
				 * @param sid   идентификатор потока
				 * @param phase фаза приёма сообщения потока
				 * @param part  часть сообщения (заголовки, трейлеры, тело), NONE - сообщение целиком
				 * @return      результат обработки (false - поток обрывается)
				 *
				 */
				using phase_callback_t = function <bool (const uint64_t, const phase_t, const part_t)>;
				/**
				 * @brief Тип функции обратного вызова для обработки провайдера полей потока
				 *
				 * @details Вызывается по завершению секции полей. Провайдер собран из
				 *          псевдо-заголовков секции: для направления REQUEST это request_t,
				 *          для RESPONSE - response_t. Для трейлеров провайдер передаётся
				 *          как nullptr. Возврат false обрывает поток
				 *
				 * @param sid       идентификатор потока
				 * @param provider  провайдер полей потока (nullptr для трейлеров)
				 * @param endStream признак завершения потока (тела не будет)
				 * @return          результат обработки (false - поток обрывается)
				 *
				 */
				using provider_callback_t = function <bool (const uint64_t, const provider_t *, const bool)>;
				/**
				 * @brief Тип функции обратного вызова для обработки поля секции заголовков либо трейлеров
				 *
				 * @details Указатели name/value действительны ТОЛЬКО на время вызова.
				 *          Возврат false обрывает поток
				 *
				 * @param sid   идентификатор потока
				 * @param name  название поля
				 * @param value значение поля
				 * @param part  часть сообщения (HEADERS или TRAILER)
				 * @return      результат обработки (false - поток обрывается)
				 *
				 */
				using header_callback_t = function <bool (const uint64_t, const string_view, const string_view, const part_t)>;
				/**
				 * @brief Тип функции обратного вызова для обработки фрагмента тела потока
				 *
				 * @details Указатель buffer действителен ТОЛЬКО на время вызова (zero-copy).
				 *          Возврат false обрывает поток
				 *
				 * @param sid       идентификатор потока
				 * @param buffer    буфер данных тела
				 * @param size      размер данных тела
				 * @param endStream признак завершения потока
				 * @return          результат обработки (false - поток обрывается)
				 *
				 */
				using data_callback_t = function <bool (const uint64_t, const void *, const size_t, const bool)>;
			public:
				/**
				 * @brief Тип функции обратного вызова открытия однонаправленного потока
				 *
				 * @details Идентификаторы потоков выдаёт транспорт, а не парсер, поэтому открыть
				 *          управляющий поток и два потока QPACK парсер может только через
				 *          обвязку. Возврат отрицательного значения означает, что транспорт
				 *          сейчас открыть поток не может: парсер повторит попытку позже
				 *
				 * @return идентификатор открытого потока либо отрицательное значение
				 *
				 */
				using open_callback_t = function <int64_t (void)>;
				/**
				 * @brief Тип функции обратного вызова записи исходящих байтов потока
				 *
				 * @details Если установлена - парсер отдаёт исходящие байты транспорту сразу
				 *          по мере формирования. Если не установлена - байты накапливаются
				 *          в буферах потоков (pull-модель: outgoing() + pending() +
				 *          consumePending())
				 *
				 * @param sid    идентификатор потока
				 * @param buffer буфер исходящих данных
				 * @param size   размер исходящих данных
				 * @param fin    признак завершения потока в исходящем направлении
				 *
				 */
				using write_callback_t = function <void (const uint64_t, const void *, const size_t, const bool)>;
				/**
				 * @brief Тип функции обратного вызова обрыва потока
				 *
				 * @details Своего кадра для обрыва потока у HTTP/3 нет: это делают кадры
				 *          RESET_STREAM и STOP_SENDING транспорта (RFC 9114 §4.1)
				 *
				 * @param sid  идентификатор потока
				 * @param code код ошибки, с которым обрывается поток
				 * @param stop признак остановки приёма (STOP_SENDING) вместо обрыва отправки
				 *
				 */
				using abort_callback_t = function <void (const uint64_t, const error_t, const bool)>;
			private:
				/**
				 * @brief Класс token-bucket для ограничения частоты событий
				 *
				 * @details Целочисленные токены, пополнение rate токенов в секунду до предела
				 *          burst. Время задаётся извне через updateTime(); без обновления времени
				 *          работает только стартовый запас burst, чего достаточно, чтобы погасить
				 *          мгновенный всплеск
				 *
				 */
				typedef class __AWH_SHARED_EXPORT__ Ratelim {
					public:
						// Пополнение токенов в секунду
						uint64_t rate;
						// Максимум токенов
						uint64_t burst;
						// Текущее число токенов
						uint64_t value;
						// Последний момент обновления (секунды)
						uint64_t stamp;
					public:
						/**
						 * @brief Метод списания токенов
						 *
						 * @param value число списываемых токенов
						 * @return      результат списания (false - токенов не хватает)
						 *
						 */
						bool drain(const uint64_t value) noexcept;
						/**
						 * @brief Метод обновления момента времени
						 *
						 * @param stamp текущий момент времени в секундах
						 *
						 */
						void update(const uint64_t stamp) noexcept;
						/**
						 * @brief Метод инициализации лимита
						 *
						 * @param burst стартовый запас токенов
						 * @param rate  пополнение токенов в секунду
						 *
						 */
						void init(const uint64_t burst, const uint64_t rate) noexcept;
					public:
						/**
						 * @brief Конструктор
						 *
						 */
						explicit Ratelim() noexcept;
				} ratelim_t;
				/**
				 * @brief Структура кольца идентификаторов push
				 *
				 * @details Учёт идентификаторов ведётся окном, а не полным множеством:
				 *          пир вправе слать отмены обещаний, потоки которых не откроет
				 *          никогда, и множество росло бы вместе с длительностью соединения.
				 *          Вытеснение предпочтительнее фатального предела: забытая отмена
				 *          означает лишь прочитанный поток push, который никому не нужен,
				 *          а исчерпанный предел означал бы разрыв штатного соединения
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Ring {
					public:
						// Ячейки кольца (UINT64_MAX - ячейка пуста)
						vector <uint64_t> items;
						// Позиция записи в кольце
						size_t cursor;
					public:
						/**
						 * @brief Метод проверки наличия идентификатора в кольце
						 *
						 * @param value искомый идентификатор
						 * @return      результат проверки
						 *
						 */
						bool has(const uint64_t value) const noexcept;
						/**
						 * @brief Метод записи идентификатора в кольцо
						 *
						 * @param value записываемый идентификатор
						 *
						 */
						void put(const uint64_t value) noexcept;
						/**
						 * @brief Метод удаления идентификатора из кольца
						 *
						 * @param value удаляемый идентификатор
						 *
						 */
						void drop(const uint64_t value) noexcept;
						/**
						 * @brief Метод очистки кольца
						 *
						 */
						void clear() noexcept;
					public:
						/**
						 * @brief Конструктор
						 *
						 */
						explicit Ring() noexcept;
				} ring_t;
				/**
				 * @brief Структура записи об обещанном push
				 *
				 * @details Повтор идентификатора обещания сам по себе допустим: сервер вправе
				 *          пообещать один и тот же push на нескольких потоках запросов.
				 *          Недопустимо расхождение секций полей при таком повторе, и чтобы
				 *          его заметить, от секции хранится отпечаток - хранить сами поля
				 *          значило бы отдать серверу управление нашей памятью (RFC 9114 §7.2.5)
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Promise {
					// Идентификатор обещания (UINT64_MAX - ячейка пуста)
					uint64_t id;
					// Отпечаток секции полей обещания
					size_t digest;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Promise() noexcept : id(UINT64_MAX), digest(0) {}
				} promise_t;
				/**
				 * @brief Структура состояния разбора кадров одного потока
				 *
				 * @details Разбор кадра идёт в три состояния: накопление заголовка кадра,
				 *          накопление нагрузки управляющего кадра и потоковая выдача нагрузки
				 *          кадра DATA. Третье состояние существует именно потому, что длина
				 *          кадра DATA протоколом не ограничена
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Framing {
					public:
						// Признак того, что заголовок кадра разобран и идёт разбор нагрузки
						bool active;
						// Тип разбираемого кадра
						uint64_t type;
						// Длина нагрузки разбираемого кадра
						uint64_t length;
						// Остаток нагрузки разбираемого кадра
						uint64_t remain;
						// Буфер накопления заголовка кадра либо нагрузки управляющего кадра
						string buffer;
					public:
						/**
						 * @brief Метод сброса состояния разбора
						 *
						 */
						void clear() noexcept;
					public:
						/**
						 * @brief Конструктор
						 *
						 */
						explicit Framing() noexcept;
				} framing_t;
				/**
				 * @brief Структура состояния потока запроса
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Stream {
					public:
						// Состояние потока
						h3::stream_state_t state;
						// Состояние разбора кадров потока
						framing_t framing;
					public:
						// Признак получения финальной секции полей
						bool headers;
						// Признак получения секции трейлеров
						bool trailers;
						// Признак начала фазы приёма тела
						bool body;
						// Признак завершённости приёма сообщения
						bool completed;
						// Признак безтелесного сообщения (ответ на HEAD, статусы 204 и 304)
						bool headless;
						// Признак туннеля метода CONNECT
						bool tunnel;
						/**
						 * Признак завершения потока в нашем направлении
						 *
						 * @details Поток удаляется только после завершения обоих направлений:
						 *          сервер отвечает на том же потоке, на котором принял запрос,
						 *          поэтому завершение приёма закрывать поток не вправе
						 *
						 */
						bool localFin;
					public:
						// Суммарный размер принятого тела потока
						uint64_t length;
						// Объявленное значение заголовка content-length (UINT64_MAX - не объявлено)
						uint64_t declared;
					public:
						// Срочность потока (RFC 9218 §4.1)
						uint8_t urgency;
						// Признак инкрементального потока (RFC 9218 §4.2)
						bool incremental;
					public:
						/**
						 * Отложенная секция полей заблокированного потока
						 *
						 * @details Секция, потребовавшая ещё не пришедших вставок QPACK,
						 *          сохраняется целиком и разбирается заново после обработки
						 *          очередной порции инструкций потока кодера
						 *
						 */
						string blocked;
						/**
						 * Неразобранный хвост потока, накопленный за время блокировки
						 *
						 * @details Кадры, пришедшие следом за заблокированной секцией, разобрать
						 *          нельзя: тело до неё недопустимо, а вторая секция затёрла бы
						 *          отложенную. Хвост накапливается целиком и разбирается заново
						 *          сразу после того, как отложенная секция разошлась по обработчикам
						 *
						 */
						string blockedTail;
						// Признак наличия отложенной секции
						bool blockedActive;
						// Тип кадра, которому принадлежит отложенная секция
						uint64_t blockedType;
						// Идентификатор push отложенного обещания
						uint64_t blockedPushId;
						// Признак завершения потока вместе с отложенной секцией
						bool blockedFin;
					public:
						/**
						 * @brief Конструктор
						 *
						 */
						explicit Stream() noexcept;
				} stream_t;
				/**
				 * @brief Структура буфера исходящих данных потока
				 *
				 * @details Нужна только в pull-модели, когда функция обратного вызова записи
				 *          не установлена. Буфер заводится на любой поток, включая служебные
				 *          однонаправленные: инструкции QPACK и кадры управляющего потока
				 *          уходят тем же путём, что и данные потоков запросов
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Outgoing {
					public:
						// Буфер накопленных исходящих данных
						string buffer;
						// Количество уже выданных наружу октетов буфера
						size_t consumed;
						// Признак завершения потока в исходящем направлении
						bool fin;
					public:
						/**
						 * @brief Конструктор
						 *
						 */
						explicit Outgoing() noexcept;
				} outgoing_t;
				/**
				 * @brief Структура состояния однонаправленного потока
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Unistream {
					public:
						// Признак прочитанного типа потока
						bool known;
						// Тип однонаправленного потока
						uint64_t type;
						// Идентификатор push для потока push
						uint64_t pushId;
						// Признак прочитанного идентификатора push
						bool identified;
						// Состояние разбора кадров потока
						framing_t framing;
						// Буфер накопления типа потока либо инструкций кодека
						string buffer;
					public:
						/**
						 * @brief Конструктор
						 *
						 */
						explicit Unistream() noexcept;
				} unistream_t;
				/**
				 * @brief Структура набора функций обратного вызова парсера
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Callbacks {
					public:
						// Функция обратного вызова открытия однонаправленного потока
						open_callback_t open;
						// Функция обратного вызова записи исходящих байтов потока
						write_callback_t write;
						// Функция обратного вызова обрыва потока
						abort_callback_t abort;
					public:
						// Функция обратного вызова обработки применённого SETTINGS пира
						settings_callback_t settings;
						// Функция обратного вызова обработки открытия нового потока
						begin_callback_t begin;
						// Функция обратного вызова обработки закрытия потока
						close_callback_t close;
						// Функция обратного вызова обработки ошибки уровня соединения
						error_callback_t error;
						// Функция обратного вызова обработки полученного GOAWAY
						goaway_callback_t goaway;
						// Функция обратного вызова обработки анонса server push
						push_callback_t push;
						// Функция обратного вызова обработки фазы приёма сообщения потока
						phase_callback_t phase;
						// Функция обратного вызова обработки провайдера полей потока
						provider_callback_t provider;
						// Функция обратного вызова обработки поля секции
						header_callback_t header;
						// Функция обратного вызова обработки фрагмента тела потока
						data_callback_t data;
					public:
						/**
						 * @brief Конструктор
						 *
						 */
						explicit Callbacks() noexcept;
				} callbacks_t;
			private:
				// Роль локального эндпоинта на соединении
				h3::endpoint_t _endpoint;
			private:
				// Кодер полей QPACK
				h3::qpack::encoder_t _encoder;
				// Декодер полей QPACK
				h3::qpack::decoder_t _decoder;
			private:
				// Лимиты безопасности парсера
				limits_t _limits;
				// Наши параметры SETTINGS
				settings_t _settings;
				// Параметры SETTINGS, полученные от пира
				settings_t _remote;
			private:
				// Набор функций обратного вызова парсера
				callbacks_t _callbacks;
			private:
				// Карта потоков запросов
				unordered_map <uint64_t, stream_t> _streams;
				// Карта однонаправленных потоков
				unordered_map <uint64_t, unistream_t> _unistreams;
				// Карта буферов исходящих данных потоков (pull-модель)
				unordered_map <uint64_t, outgoing_t> _pending;
			private:
				// Идентификатор нашего управляющего потока (UINT64_MAX - не открыт)
				uint64_t _controlLocal;
				// Идентификатор управляющего потока пира (UINT64_MAX - не открыт)
				uint64_t _controlRemote;
				// Идентификатор нашего потока инструкций кодера QPACK
				uint64_t _encoderLocal;
				// Идентификатор нашего потока инструкций декодера QPACK
				uint64_t _decoderLocal;
				// Идентификатор потока инструкций кодера QPACK пира
				uint64_t _encoderRemote;
				// Идентификатор потока инструкций декодера QPACK пира
				uint64_t _decoderRemote;
			private:
				// Признак получения SETTINGS от пира
				bool _settingsReceived;
				// Признак отправки нашего SETTINGS
				bool _settingsSent;
				// Признак завершённости соединения
				bool _closed;
			private:
				// Наибольший идентификатор push, разрешённый пиром
				uint64_t _maxPushId;
				// Наибольший идентификатор push, разрешённый нами
				uint64_t _localMaxPushId;
				// Идентификатор следующего выдаваемого push
				uint64_t _nextPushId;
				/**
				 * Идентификаторы отменённых обещаний push, потоки которых ещё не пришли
				 *
				 * @details Отмена обгоняет поток: кадр CANCEL_PUSH идёт управляющим потоком,
				 *          а сам push - своим, и порядок между ними не задан. Запись снимается
				 *          приходом потока; отмена обещания, поток которого уже пришёл, эффекта
				 *          не имеет вовсе и не записывается (RFC 9114 §7.2.3)
				 *
				 */
				ring_t _cancelledPush;
				/**
				 * Идентификаторы обещаний push, потоки которых уже приходили
				 *
				 * @details Идентификатор обещания используется ровно одним потоком: второй
				 *          поток с тем же идентификатором - ошибка соединения (RFC 9114 §4.6).
				 *          Учёт ведётся окном, поэтому повтор ловится в пределах кольца
				 *
				 */
				ring_t _openedPush;
				/**
				 * Отпечатки секций полей уже полученных обещаний push
				 *
				 * @details Один push сервер вправе пообещать на нескольких потоках запросов,
				 *          но секции полей таких обещаний обязаны совпадать: расхождение -
				 *          ошибка соединения (RFC 9114 §7.2.5). Учёт ведётся окном, поэтому
				 *          расхождение ловится в пределах кольца
				 *
				 */
				vector <promise_t> _promisedPush;
				// Позиция записи в кольце обещаний push
				size_t _promisedCursor;
			private:
				// Идентификатор потока, объявленный нами в GOAWAY
				uint64_t _goawayLocal;
				// Идентификатор потока, объявленный пиром в GOAWAY
				uint64_t _goawayRemote;
			private:
				// Лимит частоты управляющих кадров
				ratelim_t _ctrlLimit;
				// Лимит частоты кадров приоритета
				ratelim_t _priorityLimit;
			private:
				// Код последней ошибки протокола
				error_t _error;
			private:
				/**
				 * Поколение состояния соединения: увеличивается каждым сбросом (reset/clear).
				 * Пользовательская функция обратного вызова вправе сбросить парсер прямо
				 * из обработчика - после этого все ссылки на разбираемые данные, состояния
				 * потоков и списки полей недействительны, и разбор обязан свернуться
				 */
				uint64_t _epoch;
			private:
				// Декодированные поля текущей секции (ёмкость переиспользуется)
				vector <h3::qpack::field_view_t> _fields;
				// Буфер сборки исходящей секции полей (ёмкость переиспользуется)
				string _section;
				// Буфер сборки исходящего кадра (ёмкость переиспользуется)
				string _frame;
				// Список потоков с накопленными исходящими данными (ёмкость переиспользуется)
				vector <uint64_t> _outgoing;
			private:
				/**
				 * @brief Метод фиксации ошибки уровня соединения
				 *
				 * @param code    код ошибки протокола
				 * @param message текстовое описание ошибки
				 * @return        результат обработки (всегда ERROR)
				 *
				 */
				h3::status_t fail(const error_t code, const char * message) noexcept;
				/**
				 * @brief Метод записи исходящих байтов потока
				 *
				 * @param sid    идентификатор потока
				 * @param buffer буфер исходящих данных
				 * @param size   размер исходящих данных
				 * @param fin    признак завершения потока в исходящем направлении
				 *
				 */
				void emit(const uint64_t sid, const void * buffer, const size_t size, const bool fin) noexcept;
				/**
				 * @brief Метод выгрузки накопленных инструкций кодека QPACK
				 *
				 * @details Инструкции обоих потоков QPACK копятся внутри кодека и обязаны
				 *          уходить пиру вне зависимости от того, отправляем ли мы сейчас
				 *          секцию полей: без них таблицы разъедутся
				 *
				 */
				void flushQpack() noexcept;
			private:
				/**
				 * @brief Метод открытия служебных однонаправленных потоков
				 *
				 * @details Открывает управляющий поток и два потока QPACK, если они ещё
				 *          не открыты. Транспорт вправе отказать: попытка повторится позже
				 *
				 * @return признак готовности служебных потоков
				 *
				 */
				bool prepare() noexcept;
				/**
				 * @brief Метод получения состояния потока запроса
				 *
				 * @param sid идентификатор потока
				 * @return    состояние потока запроса
				 *
				 */
				stream_t & stream(const uint64_t sid) noexcept;
				/**
				 * @brief Метод поиска состояния потока запроса
				 *
				 * @param sid идентификатор потока
				 * @return    состояние потока запроса либо nullptr
				 *
				 */
				stream_t * findStream(const uint64_t sid) noexcept;
				/**
				 * @brief Метод проверки принадлежности потока инициатору
				 *
				 * @param sid идентификатор потока
				 * @return    признак того, что поток инициирован пиром
				 *
				 */
				bool peerInitiated(const uint64_t sid) const noexcept;
				/**
				 * @brief Метод проверки двунаправленности потока
				 *
				 * @param sid идентификатор потока
				 * @return    признак двунаправленного потока
				 *
				 */
				static bool bidirectional(const uint64_t sid) noexcept;
				/**
				 * @brief Метод закрытия потока запроса
				 *
				 * @param sid  идентификатор потока
				 * @param code код ошибки закрытия
				 *
				 */
				void closeStream(const uint64_t sid, const error_t code) noexcept;
				/**
				 * @brief Метод закрытия потока по завершении обоих направлений
				 *
				 * @param sid идентификатор потока
				 *
				 */
				void maybeClose(const uint64_t sid) noexcept;
			private:
				/**
				 * @brief Метод разбора байтов потока запроса
				 *
				 * @param sid    идентификатор потока
				 * @param data   входной буфер
				 * @param size   доступно байт
				 * @param fin    признак завершения потока пиром
				 * @return       результат разбора
				 *
				 */
				h3::status_t parseRequest(const uint64_t sid, const uint8_t * data, const size_t size, const bool fin) noexcept;
				/**
				 * @brief Метод обработки принятого кадра потока сообщения
				 *
				 * @param sid     идентификатор потока
				 * @param type    тип кадра
				 * @param payload нагрузка кадра
				 * @param size    размер нагрузки кадра
				 * @param last    признак завершения потока вместе с этим кадром
				 * @return        результат обработки
				 *
				 */
				h3::status_t dispatchMessage(const uint64_t sid, const uint64_t type, const uint8_t * payload, const size_t size, const bool last) noexcept;
				/**
				 * @brief Метод применения завершения потока пиром
				 *
				 * @details Завершает приём сообщения: сверяет объявленную длину тела
				 *          с принятой, выпускает завершающие фазы и закрывает поток
				 *
				 * @param sid идентификатор потока
				 * @return    результат применения
				 *
				 */
				h3::status_t applyFin(const uint64_t sid) noexcept;
				/**
				 * @brief Метод разбора байтов однонаправленного потока
				 *
				 * @param sid  идентификатор потока
				 * @param data входной буфер
				 * @param size доступно байт
				 * @param fin  признак завершения потока пиром
				 * @return     результат разбора
				 *
				 */
				h3::status_t parseUnistream(const uint64_t sid, const uint8_t * data, const size_t size, const bool fin) noexcept;
				/**
				 * @brief Метод разбора кадров управляющего потока
				 *
				 * @param sid    идентификатор потока
				 * @param stream состояние однонаправленного потока
				 * @param data   входной буфер
				 * @param size   доступно байт
				 * @return       результат разбора
				 *
				 */
				h3::status_t parseControl(const uint64_t sid, unistream_t & stream, const uint8_t * data, const size_t size) noexcept;
				/**
				 * @brief Метод обработки кадра управляющего потока
				 *
				 * @param type    тип кадра
				 * @param payload нагрузка кадра
				 * @param size    размер нагрузки кадра
				 * @return        результат обработки
				 *
				 */
				h3::status_t dispatchControl(const uint64_t type, const uint8_t * payload, const size_t size) noexcept;
			private:
				/**
				 * @brief Метод применения полученного набора параметров SETTINGS
				 *
				 * @param items набор параметров
				 * @return      результат применения
				 *
				 */
				h3::status_t applySettings(const vector <h3::frame::setting_entry_t> & items) noexcept;
				/**
				 * @brief Метод повторного разбора заблокированных потоков
				 *
				 * @details Вызывается после обработки инструкций потока кодера QPACK: часть
				 *          отложенных секций могла стать разбираемой
				 *
				 * @return результат разбора
				 *
				 */
				h3::status_t retryBlocked() noexcept;
				/**
				 * @brief Метод накопления неразобранного хвоста заблокированного потока
				 *
				 * @details Кадры, идущие следом за секцией, ждущей вставок QPACK, разобрать
				 *          нельзя: тело до секции недопустимо, а вторая секция затёрла бы
				 *          отложенную. Хвост откладывается до разблокировки потока
				 *
				 * @param sid  идентификатор потока
				 * @param data неразобранный хвост
				 * @param size размер неразобранного хвоста
				 * @param fin  признак завершения потока пиром
				 * @return     результат накопления
				 *
				 */
				h3::status_t deferTail(const uint64_t sid, const uint8_t * data, const size_t size, const bool fin) noexcept;
				/**
				 * @brief Метод обработки принятой секции полей
				 *
				 * @param sid     идентификатор потока
				 * @param section секция полей
				 * @param fin     признак завершения потока
				 * @return        результат обработки
				 *
				 */
				h3::status_t commitSection(const uint64_t sid, string_view section, const bool fin) noexcept;
				/**
				 * @brief Метод доставки декодированной секции полей
				 *
				 * @param sid идентификатор потока
				 * @param fin признак завершения потока
				 * @return    результат доставки
				 *
				 */
				h3::status_t deliverSection(const uint64_t sid, const bool fin) noexcept;
				/**
				 * @brief Метод доставки декодированной секции обещанного запроса
				 *
				 * @details Обещание push не продвигает состояние потока, на котором пришло:
				 *          оно лишь объявляет запрос, ответ на который придёт отдельным
				 *          однонаправленным потоком
				 *
				 * @param sid    идентификатор ассоциированного потока
				 * @param pushId идентификатор обещанного push
				 * @return       результат доставки
				 *
				 */
				/**
				 * @brief Метод сверки секции полей повторно обещанного push
				 *
				 * @details Один push сервер вправе пообещать на нескольких потоках запросов,
				 *          но секции полей таких обещаний обязаны совпадать: расхождение -
				 *          ошибка соединения (RFC 9114 §7.2.5). Сверяются отпечатки: хранить
				 *          сами поля значило бы отдать серверу управление нашей памятью
				 *
				 * @param pushId идентификатор обещанного push
				 * @return       результат сверки
				 *
				 */
				h3::status_t checkPromise(const uint64_t pushId) noexcept;
				h3::status_t deliverPromise(const uint64_t sid, const uint64_t pushId) noexcept;
				/**
				 * @brief Метод проверки семантики секции полей (RFC 9114 §4.1, §4.2)
				 *
				 * @param sid     идентификатор потока
				 * @param trailer признак секции трейлеров
				 * @param error   код ошибки протокола
				 * @return        результат проверки
				 *
				 */
				bool validateSection(const uint64_t sid, const bool trailer, error_t & error) noexcept;
				/**
				 * @brief Метод сборки провайдера полей потока
				 *
				 * @param request признак сборки провайдера запроса клиента
				 * @return        собранный провайдер полей потока
				 *
				 */
				unique_ptr <provider_t> buildProvider(const bool request) const noexcept;
				/**
				 * @brief Метод применения значения заголовка приоритета (RFC 9218 §5)
				 *
				 * @param stream состояние потока
				 * @param value  значение заголовка приоритета
				 *
				 */
				void applyPriority(stream_t & stream, string_view value) noexcept;
			private:
				/**
				 * @brief Метод вызова функции обратного вызова фазы приёма сообщения потока
				 *
				 * @param sid   идентификатор потока
				 * @param phase фаза приёма сообщения потока
				 * @param part  часть сообщения
				 * @return      результат вызова (false - поток обрывается)
				 *
				 */
				bool firePhase(const uint64_t sid, const phase_t phase, const part_t part) noexcept;
				/**
				 * @brief Метод вызова функции обратного вызова открытия нового потока
				 *
				 * @param sid идентификатор потока
				 * @return    результат вызова (false - поток обрывается)
				 *
				 */
				bool fireBegin(const uint64_t sid) noexcept;
				/**
				 * @brief Метод вызова функции обратного вызова анонса server push
				 *
				 * @param sid    идентификатор ассоциированного потока
				 * @param pushId идентификатор обещанного push
				 * @return       результат вызова (false - push отклоняется)
				 *
				 */
				bool firePush(const uint64_t sid, const uint64_t pushId) noexcept;
				/**
				 * @brief Метод вызова функции обратного вызова провайдера полей потока
				 *
				 * @param sid       идентификатор потока
				 * @param provider  провайдер полей потока
				 * @param endStream признак завершения потока
				 * @return          результат вызова (false - поток обрывается)
				 *
				 */
				bool fireProvider(const uint64_t sid, const provider_t * provider, const bool endStream) noexcept;
				/**
				 * @brief Метод вызова функции обратного вызова поля секции
				 *
				 * @param sid   идентификатор потока
				 * @param name  название поля
				 * @param value значение поля
				 * @param part  часть сообщения
				 * @return      результат вызова (false - поток обрывается)
				 *
				 */
				bool fireHeader(const uint64_t sid, const string_view name, const string_view value, const part_t part) noexcept;
				/**
				 * @brief Метод вызова функции обратного вызова фрагмента тела потока
				 *
				 * @param sid       идентификатор потока
				 * @param buffer    буфер данных тела
				 * @param size      размер данных тела
				 * @param endStream признак завершения потока
				 * @return          результат вызова (false - поток обрывается)
				 *
				 */
				bool fireData(const uint64_t sid, const void * buffer, const size_t size, const bool endStream) noexcept;
			public:
				/**
				 * @brief Метод очистки состояния парсера
				 *
				 */
				void clear() noexcept override;
				/**
				 * @brief Метод сброса состояния парсера
				 *
				 */
				void reset() noexcept override;
				/**
				 * @brief Метод создания копии парсера
				 *
				 * @return копия парсера
				 *
				 */
				unique_ptr <parser_t> clone() const noexcept override;
				/**
				 * @brief Метод обработки завершения ввода
				 *
				 */
				void eof() noexcept override;
				/**
				 * @brief Метод получения названия кода последней ошибки
				 *
				 * @return название кода последней ошибки
				 *
				 */
				string_view errorName() const noexcept override;
				/**
				 * @brief Метод разбора данных соединения
				 *
				 * @details Единого байтового потока у соединения HTTP/3 нет: данные всегда
				 *          принадлежат конкретному потоку QUIC. Наследуемая сигнатура
				 *          неприменима и намеренно завершается ошибкой, а не молчаливым
				 *          отказом: тихий отказ выглядел бы как исправная работа
				 *
				 * @param buffer буфер данных
				 * @param size   размер данных
				 * @return       количество разобранных байт (всегда 0)
				 *
				 */
				size_t parse(const void * buffer, const size_t size) noexcept override;
			public:
				/**
				 * @brief Метод разбора данных потока
				 *
				 * @param sid    идентификатор потока QUIC
				 * @param buffer буфер данных потока
				 * @param size   размер данных потока
				 * @param fin    признак завершения потока пиром
				 * @return       результат разбора (OK/ERROR)
				 *
				 */
				h3::status_t parse(const uint64_t sid, const void * buffer, const size_t size, const bool fin) noexcept;
				/**
				 * @brief Метод обработки обрыва потока пиром
				 *
				 * @details Вызывается обвязкой при получении кадра RESET_STREAM либо
				 *          STOP_SENDING транспорта
				 *
				 * @param sid  идентификатор потока
				 * @param code код ошибки, с которым поток оборван
				 *
				 */
				void aborted(const uint64_t sid, const uint64_t code) noexcept;
			public:
				/**
				 * @brief Метод получения кода последней ошибки протокола
				 *
				 * @return код последней ошибки протокола
				 *
				 */
				error_t error() const noexcept;
				/**
				 * @brief Метод получения названия кода ошибки протокола
				 *
				 * @param error код ошибки протокола
				 * @return      название кода ошибки протокола
				 *
				 */
				static string_view errorName(const error_t error) noexcept;
			public:
				/**
				 * @brief Метод получения лимитов безопасности парсера
				 *
				 * @return лимиты безопасности парсера
				 *
				 */
				const limits_t & limits() const noexcept;
				/**
				 * @brief Метод установки лимитов безопасности парсера
				 *
				 * @param limits лимиты безопасности парсера
				 *
				 */
				void limits(const limits_t & limits) noexcept;
				/**
				 * @brief Метод получения наших параметров SETTINGS
				 *
				 * @return наши параметры SETTINGS
				 *
				 */
				const settings_t & settings() const noexcept;
				/**
				 * @brief Метод установки наших параметров SETTINGS
				 *
				 * @note Вызывается до отправки SETTINGS: после отправки параметры менять нельзя,
				 *       кадр SETTINGS в соединении единственный (RFC 9114 §7.2.4)
				 *
				 * @param settings наши параметры SETTINGS
				 *
				 */
				void settings(const settings_t & settings) noexcept;
				/**
				 * @brief Метод получения параметров SETTINGS пира
				 *
				 * @return параметры SETTINGS пира
				 *
				 */
				const settings_t & remoteSettings() const noexcept;
				/**
				 * @brief Метод проверки получения SETTINGS от пира
				 *
				 * @return признак получения SETTINGS от пира
				 *
				 */
				bool isSettingsReceived() const noexcept;
				/**
				 * @brief Метод проверки завершённости соединения
				 *
				 * @return признак завершённости соединения
				 *
				 */
				bool isClosed() const noexcept;
			public:
				/**
				 * @brief Метод отправки параметров соединения
				 *
				 * @details Открывает управляющий поток и два потока QPACK и записывает в
				 *          управляющий поток кадр SETTINGS. Вызывается один раз в начале
				 *          соединения; повторный вызов ничего не делает
				 *
				 */
				void sendSettings() noexcept;
				/**
				 * @brief Метод отправки секции полей потока
				 *
				 * @param sid       идентификатор потока
				 * @param fields    поля секции (псевдо-заголовки должны идти первыми)
				 * @param endStream признак завершения потока
				 *
				 */
				void sendHeaders(const uint64_t sid, const vector <h3::qpack::field_t> & fields, const bool endStream) noexcept;
				/**
				 * @brief Метод отправки секции полей потока из провайдера
				 *
				 * @param sid       идентификатор потока
				 * @param headers   набор заголовков сообщения
				 * @param endStream признак завершения потока
				 * @param scheme    схема запроса для псевдо-заголовка [:scheme]
				 *
				 */
				void sendHeaders(const uint64_t sid, const headers_t & headers, const bool endStream, string_view scheme = "https") noexcept;
				/**
				 * @brief Метод отправки данных тела потока
				 *
				 * @param sid       идентификатор потока
				 * @param buffer    буфер данных тела
				 * @param size      размер данных тела
				 * @param endStream признак завершения потока
				 * @return          количество принятых к отправке байт
				 *
				 */
				size_t sendData(const uint64_t sid, const void * buffer, const size_t size, const bool endStream) noexcept;
				/**
				 * @brief Метод отправки анонса server push (только сервер)
				 *
				 * @param sid    идентификатор ассоциированного потока запроса
				 * @param fields поля обещанного запроса
				 * @return       идентификатор обещанного push либо UINT64_MAX при отказе
				 *
				 */
				uint64_t sendPushPromise(const uint64_t sid, const vector <h3::qpack::field_t> & fields) noexcept;
				/**
				 * @brief Метод отмены обещанного push
				 *
				 * @param pushId идентификатор отменяемого push
				 *
				 */
				void sendCancelPush(const uint64_t pushId) noexcept;
				/**
				 * @brief Метод проверки отменённости обещанного push
				 *
				 * @details Серверу отвечает, отказался ли клиент от обещания кадром
				 *          CANCEL_PUSH: поток такого push открывать уже незачем.
				 *          Клиенту отвечает, отменено ли обещание им самим либо сервером
				 *
				 * @param pushId идентификатор обещанного push
				 * @return       признак отменённости обещания
				 *
				 */
				bool pushCancelled(const uint64_t pushId) const noexcept;
				/**
				 * @brief Метод разрешения пиру выдавать push (только клиент)
				 *
				 * @param pushId наибольший разрешённый идентификатор push
				 *
				 */
				void sendMaxPushId(const uint64_t pushId) noexcept;
				/**
				 * @brief Метод отправки приоритета потока (RFC 9218 §7.2)
				 *
				 * @param sid         идентификатор потока
				 * @param urgency     срочность потока (0 - наивысшая, 7 - наименьшая)
				 * @param incremental признак инкрементального потока
				 *
				 */
				void sendPriority(const uint64_t sid, const uint8_t urgency, const bool incremental) noexcept;
				/**
				 * @brief Метод обрыва потока
				 *
				 * @param sid  идентификатор потока
				 * @param code код ошибки, с которым обрывается поток
				 *
				 */
				void sendReset(const uint64_t sid, const error_t code) noexcept;
				/**
				 * @brief Метод завершения соединения (RFC 9114 §5.2)
				 *
				 * @param id идентификатор потока запроса (от сервера) либо push (от клиента)
				 *
				 */
				void sendGoaway(const uint64_t id) noexcept;
				/**
				 * @brief Метод плавного завершения соединения
				 *
				 * @details Отправляет GOAWAY с предельным идентификатором: пир прекращает
				 *          открывать новые потоки, а уже открытые доживают штатно. Итоговый
				 *          GOAWAY с фактическим идентификатором отправляется позже
				 *
				 */
				void sendShutdown() noexcept;
			public:
				/**
				 * @brief Метод обновления момента времени для частотных лимитов
				 *
				 * @param seconds текущий момент времени в секундах
				 *
				 */
				void updateTime(const uint64_t seconds) noexcept;
			public:
				/**
				 * @brief Метод получения списка потоков с накопленными исходящими данными
				 *
				 * @details Нужен в pull-модели, когда функция обратного вызова записи
				 *          не установлена
				 *
				 * @param output список идентификаторов потоков
				 *
				 */
				void outgoing(vector <uint64_t> & output) noexcept;
				/**
				 * @brief Метод получения накопленных исходящих данных потока
				 *
				 * @param sid идентификатор потока
				 * @return    представление накопленных исходящих данных
				 *
				 */
				string_view pending(const uint64_t sid) noexcept;
				/**
				 * @brief Метод отметки исходящих данных потока как отправленных
				 *
				 * @param sid  идентификатор потока
				 * @param size количество отправленных октетов
				 *
				 */
				void consumePending(const uint64_t sid, const size_t size) noexcept;
				/**
				 * @brief Метод проверки завершения потока в исходящем направлении
				 *
				 * @param sid идентификатор потока
				 * @return    признак того, что поток закрыт нами и данных больше не будет
				 *
				 */
				bool finished(const uint64_t sid) noexcept;
			public:
				/**
				 * @brief Метод установки функции обратного вызова открытия однонаправленного потока
				 *
				 * @param callback функция обратного вызова
				 *
				 */
				void on(open_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова записи исходящих байтов
				 *
				 * @param callback функция обратного вызова
				 *
				 */
				void on(write_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова обрыва потока
				 *
				 * @param callback функция обратного вызова
				 *
				 */
				void on(abort_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова применённого SETTINGS пира
				 *
				 * @param callback функция обратного вызова
				 *
				 */
				void on(settings_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова открытия нового потока
				 *
				 * @param callback функция обратного вызова
				 *
				 */
				void on(begin_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова закрытия потока
				 *
				 * @param callback функция обратного вызова
				 *
				 */
				void on(close_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова ошибки уровня соединения
				 *
				 * @param callback функция обратного вызова
				 *
				 */
				void on(error_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова полученного GOAWAY
				 *
				 * @param callback функция обратного вызова
				 *
				 */
				void on(goaway_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова анонса server push
				 *
				 * @param callback функция обратного вызова
				 *
				 */
				void on(push_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова фазы приёма сообщения потока
				 *
				 * @param callback функция обратного вызова
				 *
				 */
				void on(phase_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова провайдера полей потока
				 *
				 * @param callback функция обратного вызова
				 *
				 */
				void on(provider_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова поля секции
				 *
				 * @param callback функция обратного вызова
				 *
				 */
				void on(header_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова фрагмента тела потока
				 *
				 * @param callback функция обратного вызова
				 *
				 */
				void on(data_callback_t callback) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param direct направление разбора сообщений
				 * @param fmk    объект фреймворка
				 * @param log    объект для работы с логами
				 *
				 */
				explicit Parser_HTTP3(const direct_t direct, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Parser_HTTP3() noexcept = default;
		} parser_http3_t;
	};
};

#endif // __AWH_HTTP_PARSER_HTTP3__
