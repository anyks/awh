/**
 * @file: http.hpp
 * @date: 2026-07-19
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл парсера сессии HTTP/2 — класс Parser_HTTP2, управляющий состояниями потоков,
 *        окнами flow control, параметрами SETTINGS, приоритетами,
 *        частотными лимитами и лимитами на размер распакованных заголовков и тела
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_HTTP_PARSER_HTTP2__
#define __AWH_HTTP_PARSER_HTTP2__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <string_view>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "h2.hpp"
#include "frame.hpp"
#include "hpack.hpp"
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
		 * @brief Класс парсера протокола HTTP/2 (RFC 9113)
		 *
		 * @details В отличие от парсера HTTP/1.x работает на уровне СОЕДИНЕНИЯ, а не одного
		 *          сообщения: HTTP/2 мультиплексирует множество потоков (сообщений) в одном
		 *          соединении, поэтому все события сопровождаются идентификатором потока.
		 *          Парсер владеет состоянием соединения: HPACK-кодером/декодером, картой
		 *          потоков, окнами flow control и согласованными SETTINGS.
		 *
		 *          Протокол двунаправленный: разбор входящих фреймов порождает обязательные
		 *          исходящие (SETTINGS ACK, PING ACK, WINDOW_UPDATE, RST_STREAM, GOAWAY),
		 *          поэтому у парсера есть канал записи - функция обратного вызова записи
		 *          (write_callback_t) либо pull-модель pending()/consumePending().
		 *
		 *          Направление трафика задаёт роль эндпоинта:
		 *          - direct_t::REQUEST  - разбираем запросы клиента (мы - сервер);
		 *          - direct_t::RESPONSE - разбираем ответы сервера (мы - клиент).
		 *
		 *          Встроенные защиты от DoS: лимит частоты RST_STREAM (Rapid Reset,
		 *          CVE-2023-44487), лимит частоты управляющих фреймов (SETTINGS/PING/пустые
		 *          DATA flood), лимиты сборки блока заголовков (CONTINUATION flood, 2024),
		 *          лимит распакованного списка заголовков (decompression bomb) и лимит
		 *          размера тела потока.
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Parser_HTTP2 : public parser_t {
			public:
				/**
				 * @brief Пополнение лимита частоты входящих RST_STREAM (токенов в секунду)
				 *
				 * @note Значения по умолчанию соответствуют nghttp2
				 *
				 */
				static constexpr uint64_t RST_LIMIT_RATE = (33);
				/**
				 * @brief Пополнение лимита частоты управляющих фреймов (токенов в секунду)
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 *
				 */
				static constexpr uint64_t CTRL_LIMIT_RATE = (100);
				/**
				 * @brief Стартовый запас лимита частоты входящих RST_STREAM (защита от Rapid Reset)
				 *
				 * @note Значения по умолчанию соответствуют nghttp2
				 *
				 */
				static constexpr uint64_t RST_LIMIT_BURST = (1000);
				/**
				 * @brief Стартовый запас лимита частоты управляющих фреймов (SETTINGS/PING/пустые DATA)
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 *
				 */
				static constexpr uint64_t CTRL_LIMIT_BURST = (1000);
				/**
				 * @brief Пополнение лимита частоты кадров приоритета (токенов в секунду)
				 *
				 * @note Лимит отдельный от управляющих фреймов и заметно щедрее: клиент вправе
				 *       переставлять приоритеты на каждый загружаемый ресурс страницы
				 *
				 */
				static constexpr uint64_t PRIO_LIMIT_RATE = (500);
				/**
				 * @brief Стартовый запас лимита частоты кадров приоритета (PRIORITY/PRIORITY_UPDATE)
				 *
				 * @note Лимит отдельный от управляющих фреймов и заметно щедрее: клиент вправе
				 *       переставлять приоритеты на каждый загружаемый ресурс страницы
				 *
				 */
				static constexpr uint64_t PRIO_LIMIT_BURST = (5000);
				/**
				 * @brief Порог сигнала о готовности потока принимать данные (low-water)
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 *
				 */
				static constexpr size_t SEND_LOW_WATER = (64 * 1024);
				/**
				 * @brief Ёмкость буфера отправки одного потока (high-water)
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 *
				 */
				static constexpr size_t SEND_HIGH_WATER = (256 * 1024);
				/**
				 * @brief Максимальное число фреймов в одном блоке заголовков
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 *
				 */
				static constexpr uint32_t MAX_CONTINUATION_FRAMES = (64);
				/**
				 * @brief Порог выходного буфера соединения (backpressure от TCP-стадии)
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 *
				 */
				static constexpr size_t OUTPUT_HIGH_WATER = (1024 * 1024);
				/**
				 * @brief Максимальный суммарный размер блока заголовков (HEADERS + все CONTINUATION)
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 *
				 */
				static constexpr size_t MAX_HEADER_BLOCK_SIZE = (64 * 1024);
				/**
				 * @brief Число запоминаемых потоков, оборванных сбросом
				 *
				 * @details Кадры на оборванном потоке ещё летят к нам, пока пир не получил
				 *          наш RST_STREAM, и обязаны считаться потоковой ошибкой, а не
				 *          ошибкой соединения (RFC 9113 §5.1). Один идентификатор запомнить
				 *          мало: под нагрузкой сбросы идут подряд, и второй вытеснял бы
				 *          первый, пока кадры первого ещё в сети
				 *
				 */
				static constexpr size_t RESET_STREAMS_CACHE = (64);
				/**
				 * @brief Число запоминаемых приоритетов ещё не открытых потоков
				 *
				 * @details Кадр PRIORITY_UPDATE допустим для потока в состоянии idle
				 *          (RFC 9218 §7.1): сигнал приходит раньше HEADERS и обязан
				 *          примениться, когда поток откроется. Объектов потоков под
				 *          такие сигналы не создаём, а число самих сигналов ограничено:
				 *          иначе пир наполнял бы память приоритетами потоков,
				 *          которые открывать не собирается
				 *
				 */
				static constexpr size_t PENDING_PRIORITIES_CACHE = (32);
			private:
				/**
				 * @brief Ёмкость набора отправляемых параметров SETTINGS
				 *
				 * @details Парсер анонсирует восемь параметров: размер таблицы HPACK,
				 *          разрешение push, начальное окно, размер кадра, лимит
				 *          одновременных потоков, лимит списка заголовков, разрешение
				 *          расширенного CONNECT и отказ от приоритетов RFC 7540.
				 *          Ёмкость взята с запасом: набор собирается инкрементом
				 *          счётчика без проверки границы на каждой записи, поэтому
				 *          девятый параметр, добавленный без правки этой константы,
				 *          вышел бы за массив молча
				 *
				 */
				static constexpr size_t SETTINGS_ENTRIES = (16);
			public:
				/**
				 * @brief Тип кода ошибки протокола HTTP/2 (RFC 9113 §7)
				 *
				 */
				using error_t = h2::error_t;
			public:
				/**
				 * @brief Структура ограничений безопасности парсера HTTP/2
				 *
				 * @details Расширяет общее ядро лимитов базового парсера лимитами,
				 *          специфичными для HTTP/2. Общие лимиты применяются так:
				 *          - maxHeaderName/maxHeaderValue - длины декодированных заголовков;
				 *          - maxHeaderCount - число заголовков в одном блоке;
				 *          - maxHeadersTotal - суммарный размер распакованного списка (HPACK bomb);
				 *          - maxBodySize - суммарный размер тела одного потока
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Limits : parser_t::limits_t {
					// Пополнение лимита частоты входящих RST_STREAM (токенов в секунду)
					uint64_t rstLimitRate;
					// Стартовый запас лимита частоты входящих RST_STREAM
					uint64_t rstLimitBurst;
					// Пополнение лимита частоты управляющих фреймов (токенов в секунду)
					uint64_t ctrlLimitRate;
					// Стартовый запас лимита частоты управляющих фреймов
					uint64_t ctrlLimitBurst;
					// Пополнение лимита частоты кадров приоритета (токенов в секунду)
					uint64_t prioLimitRate;
					// Стартовый запас лимита частоты кадров приоритета
					uint64_t prioLimitBurst;
					// Максимальный суммарный размер блока заголовков (HEADERS + все CONTINUATION)
					size_t maxHeaderBlockSize;
					// Максимальное число фреймов в одном блоке заголовков
					uint32_t maxContinuationFrames;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Limits() noexcept;
				} limits_t;
				/**
				 * @brief Структура согласованных параметров SETTINGS (RFC 9113 §6.5.2)
				 *
				 * @details Значения по умолчанию из RFC 9113 (кроме maxConcurrentStreams -
				 *          безопасный дефолт вместо "без лимита").
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Settings {
					// Начальное окно потока
					int32_t windowSize;
					// Разрешён ли server push (0/1)
					uint32_t enablePush;
					// Максимальный размер фрейма
					uint32_t maxFrameSize;
					// Размер динамической таблицы HPACK
					uint32_t headerTableSize;
					// Лимит размера списка заголовков (0 - без лимита в SETTINGS, действует maxHeadersTotal)
					uint32_t maxHeaderListSize;
					// Лимит одновременных потоков
					uint32_t maxConcurrentStreams;
					// Разрешён ли расширенный CONNECT (0/1) - RFC 8441 §3
					uint32_t enableConnectProtocol;
					// Отказ от приоритетов RFC 7540 (0/1) - RFC 9218 §2.1
					uint32_t noRfc7540Priorities;
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
				 * @details Возврат false сбрасывает поток (RST_STREAM с кодом CANCEL).
				 *
				 * @param sid идентификатор потока
				 * @return    результат обработки (false - поток сбрасывается)
				 *
				 */
				using begin_callback_t = function <bool (const uint32_t)>;
				/**
				 * @brief Тип функции обратного вызова о готовности потока принимать данные тела
				 *
				 * @details Вызывается когда буфер отправки потока опустился ниже low-water
				 *          (после частичного приёма в sendData можно отправлять дальше).
				 *
				 * @param sid идентификатор потока
				 *
				 */
				using writable_callback_t = function <void (const uint32_t)>;
				/**
				 * @brief Тип функции обратного вызова записи исходящих байтов в сеть
				 *
				 * @details Если установлена - парсер сам отдаёт исходящие байты сетевому слою
				 *          сразу по мере формирования. Если не установлена - исходящие байты
				 *          накапливаются во внутреннем буфере (pull-модель: pending() +
				 *          consumePending()).
				 *
				 * @param buffer буфер исходящих данных
				 * @param size   размер исходящих данных
				 *
				 */
				using write_callback_t = function <void (const void *, const size_t)>;
				/**
				 * @brief Тип функции обратного вызова для обработки анонса server push (только клиент)
				 *
				 * @details Заголовки обещанного запроса придут через header_callback_t /
				 *          provider_callback_t с идентификатором обещанного потока.
				 *          Возврат false отклоняет push (RST_STREAM с кодом CANCEL).
				 *
				 * @param sid         идентификатор ассоциированного потока клиента
				 * @param promisedSid идентификатор обещанного потока
				 * @return            результат обработки (false - push отклоняется)
				 *
				 */
				using push_callback_t = function <bool (const uint32_t, const uint32_t)>;
				/**
				 * @brief Тип функции обратного вызова для обработки закрытия потока
				 *
				 * @param sid  идентификатор потока
				 * @param code код ошибки закрытия (NO_ERROR - штатное закрытие)
				 *
				 */
				using close_callback_t = function <void (const uint32_t, const error_t)>;
				/**
				 * @brief Тип функции обратного вызова для обработки ошибки уровня соединения
				 *
				 * @details После этого события соединение необходимо закрыть
				 *          (GOAWAY уже поставлен в очередь отправки).
				 *
				 * @param code    код ошибки протокола
				 * @param message текстовое описание ошибки
				 *
				 */
				using error_callback_t = function <void (const error_t, const string_view)>;
				/**
				 * @brief Тип функции обратного вызова для обработки фазы приёма сообщения потока
				 *
				 * @details Последовательность событий при приёме одного сообщения потока
				 *          (сигнатура и порядок универсальны с HTTP/1):
				 *          1. (BEGIN, NONE)    - получен первый блок заголовков потока
				 *          2. (END, HEADERS)   - блок заголовков доставлен (после провайдера)
				 *          3. (BEGIN, BODY)    - ожидается тело (END_STREAM не получен с заголовками)
				 *          4. (END, BODY)      - тело полностью принято (END_STREAM во фрейме DATA
				 *                                либо получен блок трейлеров)
				 *          5. (BEGIN, TRAILER) - получен блок трейлеров (только если пир их прислал)
				 *          6. (END, TRAILER)   - трейлеры доставлены (после провайдера с nullptr)
				 *          7. (END, NONE)      - сообщение потока полностью принято (END_STREAM применён)
				 *          Для обещанных запросов PUSH_PROMISE фазы не вызываются - фазы начнутся
				 *          с приходом ответа на обещанном потоке. Для информационных ответов
				 *          сервера (1xx) фазы также не вызываются: такой блок промежуточный и
				 *          доставляется только через header_callback_t / provider_callback_t,
				 *          а фазы начнутся с приходом финального блока заголовков.
				 *          Возврат false сбрасывает поток (RST_STREAM с кодом CANCEL).
				 *
				 * @param sid   идентификатор потока
				 * @param phase фаза приёма сообщения потока
				 * @param part  часть сообщения (заголовки, трейлеры, тело), NONE - сообщение целиком
				 * @return      результат обработки (false - поток сбрасывается)
				 *
				 */
				using phase_callback_t = function <bool (const uint32_t, const phase_t, const part_t)>;
				/**
				 * @brief Тип функции обратного вызова для обработки провайдера заголовков потока
				 *
				 * @details Вызывается по завершению блока заголовков (получен END_HEADERS).
				 *          Провайдер собран из псевдо-заголовков блока: для направления REQUEST
				 *          это request_t (:method/:path), для RESPONSE - response_t (:status).
				 *          Для трейлеров провайдер передаётся как nullptr.
				 *          Возврат false сбрасывает поток (RST_STREAM с кодом CANCEL).
				 *
				 * @param sid       идентификатор потока
				 * @param provider  провайдер заголовков потока (nullptr для трейлеров)
				 * @param endStream флаг завершения потока (тела не будет)
				 * @return          результат обработки (false - поток сбрасывается)
				 *
				 */
				using provider_callback_t = function <bool (const uint32_t, const provider_t *, const bool)>;
				/**
				 * @brief Тип функции обратного вызова для обработки полученного GOAWAY
				 *
				 * @details Указатель debug действителен ТОЛЬКО на время вызова
				 *
				 * @param sid   наибольший идентификатор обработанного пиром потока
				 * @param code  код ошибки завершения соединения
				 * @param debug необязательные отладочные данные пира
				 *
				 */
				using goaway_callback_t = function <void (const uint32_t, const error_t, const string_view)>;
				/**
				 * @brief Тип функции обратного вызова для обработки полученного ALTSVC (RFC 7838 §4)
				 *
				 * @details Представления origin и value действительны ТОЛЬКО на время вызова.
				 *          Нулевой идентификатор потока означает анонс для соединения,
				 *          и тогда origin непустой; для потока origin приходит пустым,
				 *          а сам origin определяется этим потоком
				 *
				 * @param sid    идентификатор потока, либо 0 для соединения
				 * @param origin origin анонсируемого сервиса
				 * @param value  значение поля Alt-Svc (RFC 7838 §3)
				 *
				 */
				using altsvc_callback_t = function <void (const uint32_t, const string_view, const string_view)>;
				/**
				 * @brief Тип функции обратного вызова для обработки полученного ORIGIN (RFC 8336 §2)
				 *
				 * @details Вызывается по одному разу на каждый origin набора. Представление
				 *          действительно ТОЛЬКО на время вызова
				 *
				 * @param origin очередной origin, обслуживаемый соединением
				 *
				 */
				using origin_callback_t = function <void (const string_view)>;
				/**
				 * @brief Тип функции обратного вызова для обработки фрагмента тела потока
				 *
				 * @details Указатель buffer действителен ТОЛЬКО на время вызова (zero-copy).
				 *          Возврат false сбрасывает поток (RST_STREAM с кодом CANCEL).
				 *
				 * @param sid       идентификатор потока
				 * @param buffer    буфер данных тела
				 * @param size      размер данных тела
				 * @param endStream флаг завершения потока
				 * @return          результат обработки (false - поток сбрасывается)
				 *
				 */
				using data_callback_t = function <bool (const uint32_t, const void *, const size_t, const bool)>;
				/**
				 * @brief Тип pull-источника данных тела потока (для больших тел без лишней копии)
				 *
				 * @details Альтернатива sendData: парсер сам запрашивает у источника данные
				 *          ровно тогда, когда открыто окно и есть место в выходном буфере.
				 *          Источник заполняет буфер (не более cap байт), выставляет eof = true
				 *          по достижении конца тела и возвращает число записанных байт,
				 *          либо -1 при ошибке (поток будет сброшен).
				 *
				 * @param sid    идентификатор потока
				 * @param buffer буфер для заполнения
				 * @param cap    ёмкость буфера
				 * @param eof    флаг достижения конца тела
				 * @return       число записанных байт либо -1 при ошибке
				 *
				 */
				using data_source_callback_t = function <int64_t (const uint32_t, uint8_t *, const size_t, bool &)>;
				/**
				 * @brief Тип функции обратного вызова для обработки заголовков или трейлеров потока
				 *
				 * @details Указатели name/value действительны ТОЛЬКО на время вызова.
				 *          Передаются все заголовки, включая псевдо-заголовки (:method/:path/
				 *          :status и др.); наиболее употребимые псевдо-заголовки дополнительно
				 *          собираются в провайдер заголовков потока.
				 *          Возврат false сбрасывает поток (RST_STREAM с кодом CANCEL).
				 *
				 * @param sid   идентификатор потока
				 * @param name  название заголовка
				 * @param value значение заголовка
				 * @param part  часть сообщения (HEADERS или TRAILER)
				 * @return      результат обработки (false - поток сбрасывается)
				 *
				 */
				using header_callback_t = function <bool (const uint32_t, const string_view, const string_view, const part_t)>;
			private:
				/**
				 * @brief Класс token-bucket для ограничения частоты событий (защита от flood)
				 *
				 * @details Повторяет модель nghttp2_ratelim: целочисленные токены, пополнение
				 *          rate токенов в секунду до предела burst. Время задаётся извне через
				 *          updateTime(); без обновления времени работает только стартовый запас
				 *          burst (этого достаточно, чтобы погасить мгновенный всплеск).
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
						 * @return      результат списания (false - токенов не хватает, превышение лимита)
						 *
						 */
						bool drain(const uint64_t value) noexcept;
						/**
						 * @brief Метод пополнения токенов по текущему времени
						 *
						 * @param stamp текущее время (секунды)
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
				 * @brief Класс состояния одного потока (RFC 9113 §5.1)
				 *
				 */
				typedef class __AWH_SHARED_EXPORT__ Stream {
					public:
						// Идентификатор потока
						uint32_t id;
						// Достигнут конец тела pull-источника данных
						bool sourceEof;
						// Получен END_HEADERS (повторный HEADERS = трейлеры)
						bool headersDone;
						// END_STREAM уже отправлен
						bool endStreamSent;
						// На последнем фрагменте выставить END_STREAM
						bool endStreamPending;
						// Сигнал writable уже подан для текущего провала буфера
						bool writableNotified;
						/**
						 * Поток стоит в очереди готовых к отправке
						 *
						 * @details Признак хранится у потока, а не ищется в очереди:
						 *          иначе добавление стоило бы линейного поиска по ней
						 *
						 */
						bool queued;
						// Суммарный размер принятого тела потока (лимит maxBodySize)
						uint64_t recvBody;
						// Объявленная заголовком content-length длина тела (-1 - не объявлена)
						int64_t contentLength;
						/**
						 * Принимаемое сообщение не может нести тело, даже если объявляет
						 * content-length: ответ на HEAD, а также ответы 204 и 304 (RFC 9110 §8.6)
						 */
						bool bodyless;
						/**
						 * Отправляемое сообщение не может нести тело: ответ на принятый
						 * запрос методом HEAD (RFC 9110 §9.3.2)
						 *
						 * @details Признак отдельный от bodyless: тело самого запроса HEAD
						 *          запрещено лишь через SHOULD NOT, поэтому принимать его
						 *          парсер обязан, а вот отдавать тело в ответ на него - нет
						 *
						 */
						bool bodylessSend;
						/**
						 * Принимаемое сообщение не может нести секцию трейлеров:
						 * ответы 204 и 304 (RFC 9110 §15.3.5, §15.4.5)
						 *
						 * @details Признак отдельный от bodyless: у ответа на HEAD
						 *          запрещено только содержимое, а трейлеры §9.3.2
						 *          не запрещает, и кадрирование их допускает
						 *
						 */
						bool trailerless;
						/**
						 * Отправляемое сообщение не может нести секцию трейлеров:
						 * ответы 204 и 304 (RFC 9110 §15.3.5, §15.4.5)
						 *
						 * @details Признак отдельный от bodylessSend: у ответа на HEAD
						 *          запрещено только содержимое, а трейлеры §9.3.2 не
						 *          запрещает вовсе, и кадрирование их допускает
						 *
						 */
						bool trailerlessSend;
						// Срочность потока (0 - наивысшая, 7 - наименьшая) - RFC 9218 §4.1
						uint8_t urgency;
						// Признак инкрементальной доставки потока - RFC 9218 §4.2
						bool incremental;
						/**
						 * Приоритет потока задан кадром PRIORITY_UPDATE
						 *
						 * @details Кадр перекрывает любой другой сигнал приоритета
						 *          (RFC 9218 §7), поэтому заголовок [priority], пришедший
						 *          после него, приоритет уже не меняет
						 *
						 */
						bool prioritized;
						/**
						 * Приоритет потока объявлен приложением и ещё не отправлен
						 *
						 * @details Сигнал приоритета для собственного потока передаётся
						 *          заголовком [priority] в секции заголовков (RFC 9218 §5):
						 *          серверу это единственный доступный способ - кадр
						 *          PRIORITY_UPDATE ему запрещён. Признак снимается отправкой
						 *          секции, потому что объявлять приоритет повторно в трейлерах
						 *          бессмысленно
						 *
						 */
						bool announce;
						// Окно приёма потока (сколько ещё можем принять)
						int32_t localWindow;
						// Окно отправки потока (сколько ещё можем отправить)
						int32_t remoteWindow;
						// Префикс sendBuffer, уже отправленный (вместо erase(0,..))
						size_t sendOffset;
						// Ещё не нарезанные в DATA байты тела (ограничен high-water)
						string sendBuffer;
						// Блок заголовков потока уже отправлен нами
						bool headersSent;
						// На завершение тела отложена секция трейлеров
						bool trailersPending;
						/**
						 * Отложенная секция трейлеров: хранится полями, а не закодированным
						 * блоком. HPACK-блоки обязаны кодироваться в том же порядке, в каком
						 * уходят в сеть, поэтому отложить можно только сами заголовки
						 */
						vector <h2::hpack::field_t> trailers;
						// Состояние потока
						h2::stream_state_t state;
						// Pull-источник данных тела (если задан вместо sendData)
						data_source_callback_t source;
						// Провайдер заголовков потока (собирается из псевдо-заголовков)
						unique_ptr <http::provider_t> headers;
					public:
						/**
						 * @brief Метод получения логического объёма ещё не отправленных данных тела
						 *
						 * @return объём не отправленных данных (без учтённого префикса)
						 *
						 */
						size_t pending() const noexcept;
						/**
						 * @brief Метод снятия учтённого префикса буфера отправки
						 *
						 * @details Очистка при полном расходе, иначе амортизированная компактификация
						 *
						 */
						void compactSendBuffer() noexcept;
					public:
						/**
						 * @brief Конструктор
						 *
						 */
						explicit Stream() noexcept;
				} stream_t;
			private:
				/**
				 * @brief Структура rate-лимитов
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Ratelims {
					// Текущее время в секундах (для rate-лимитов)
					uint64_t now;
					// Лимит частоты входящих RST_STREAM (против Rapid Reset)
					ratelim_t rst;
					// Лимит частоты управляющих фреймов (против flood SETTINGS/PING/пустых DATA)
					ratelim_t ctrl;
					// Лимит частоты кадров приоритета (против flood PRIORITY/PRIORITY_UPDATE)
					ratelim_t prio;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Ratelims() noexcept;
				} ratelims_t;
				/**
				 * @brief Структура окон flow control соединения
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Window {
					// Окно приёма соединения (текущее)
					int32_t local;
					// Окно отправки соединения
					int32_t remote;
					// Целевой размер окна приёма соединения
					int32_t localMax;
					// Анонсированное пиру начальное окно приёма потока (SETTINGS_INITIAL_WINDOW_SIZE)
					int32_t localInit;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Window() noexcept;
				} window_t;
				/**
				 * @brief Структура буферов соединения
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Buffer {
					/**
					 * Незавершённый фрейм, не уместившийся в поданную порцию
					 *
					 * @details Фрейм HTTP/2 разбирается только целиком, а границы порций
					 *          чтения из сокета с границами фреймов не совпадают. Здесь
					 *          копится ровно недостающая часть одного фрейма и ничего
					 *          сверх: целые фреймы разбираются прямо по буферу вызывающей
					 *          стороны и в этот буфер не попадают. Поэтому объём копирования
					 *          на порцию ограничен размером одного фрейма, а не её размером
					 *
					 */
					string input;
					/**
					 * Байты, поданные реентрантным вызовом parse() из пользовательской
					 * функции: дописывать их сразу в буфер незавершённого фрейма нельзя -
					 * перевыделение памяти обесценит zero-copy указатели, уже отданные наружу
					 */
					string deferred;
					// Буфер исходящих байтов
					string output;
					// Префикс буфера исходящих байтов, уже отданный в сокет (вместо erase(0,..))
					size_t outputPos;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Buffer() noexcept;
				} buffer_t;
				/**
				 * @brief Структура собираемого блока заголовков (HEADERS + CONTINUATION)
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Header_Block {
					// Идентификатор потока собираемого блока заголовков (0 - сборка не идёт)
					uint32_t stream;
					// Число фреймов в текущем блоке заголовков
					uint32_t frames;
					// Идентификатор обещанного потока (!= 0: блок принадлежит PUSH_PROMISE)
					uint32_t promised;
					// Накопленный фрагмент блока заголовков (HEADERS + CONTINUATION)
					string buffer;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Header_Block() noexcept;
				} header_block_t;
				/**
				 * @brief Структура флагов состояния соединения
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Flags {
					// Защита от реентерабельного pump() (writable -> sendData -> pump)
					bool inPump;
					// Защита от реентерабельного parse() (callback -> parse)
					bool inParse;
					// Отправлен GOAWAY
					bool goawaySent;
					// Отправлен предупреждающий GOAWAY плавного завершения (RFC 9113 §6.8)
					bool goawayGraceful;
					// Поток отклонён (RST_STREAM), блок декодируем только для синхронизации HPACK
					bool hbcRefused;
					// Флаг END_STREAM собираемого блока заголовков
					bool hbcEndStream;
					// Получен ACK на наш SETTINGS
					bool settingsAcked;
					// Получен первый SETTINGS пира (connection preface, RFC 9113 §3.4)
					bool settingsReceived;
					// Параметр отказа от приоритетов RFC 7540 уже зафиксирован пиром (RFC 9218 §2.1)
					bool prioritiesLocked;
					// Получен GOAWAY
					bool goawayReceived;
					// Получен connection preface (для сервера; клиент отправляет его сам)
					bool prefaceReceived;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Flags() noexcept;
				} flags_t;
				/**
				 * @brief Структура записи о потоке, оборванном сбросом
				 *
				 * @details Различать, чей это был сброс, обязательно: после нашего RST_STREAM
				 *          кадры пира обязаны молча игнорироваться, а после его сброса -
				 *          отвергаться потоковой ошибкой STREAM_CLOSED (RFC 9113 §5.1)
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Reset {
					// Идентификатор потока (0 - ячейка кольца пуста)
					uint32_t id;
					// Сброс отправлен нами (иначе - принят от пира)
					bool local;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Reset() noexcept : id(0), local(false) {}
				} reset_t;
				/**
				 * @brief Структура параметров передачи данных
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Transfer {
					// Наибольший принятый идентификатор потока (для GOAWAY)
					uint32_t lastStreamId;
					/**
					 * Кольцо потоков, оборванных сбросом
					 *
					 * @details Кадры на таком потоке ещё могут быть в полёте: их отправили
					 *          до того, как сброс дошёл до отправителя. Кольцо, а не один
					 *          идентификатор: сбросы идут подряд, и хранить надо все недавние
					 *
					 */
					vector <reset_t> resetStreams;
					// Позиция записи в кольце оборванных сбросом потоков
					size_t resetCursor;
					/**
					 * Наибольший идентификатор, попадавший в кольцо оборванных сбросом потоков
					 *
					 * @details Служит отсечкой перебора: кольцо просматривается на каждом кадре
					 *          потока, которого нет в карте, а таким оказывается и всякий вновь
					 *          открываемый поток - то есть путь этот не редкий, как можно
					 *          подумать, а самый обычный. Идентификатор нового потока заведомо
					 *          старше всех попадавших в кольцо, и перебор шестидесяти четырёх
					 *          ячеек для него бессмыслен. Отсечка точна: значение только растёт,
					 *          поэтому идентификатор больше него в кольце отсутствует наверняка
					 *
					 */
					uint32_t resetMaxId;
					// Наибольший наш идентификатор потока, по которому уже отправлен блок заголовков
					uint32_t localOpened;
					// Следующий инициируемый нами идентификатор потока
					uint32_t nextStreamId;
					// Число активных потоков, открытых пиром (лимит MAX_CONCURRENT_STREAMS)
					uint32_t peerStreamCount;
					// Число активных потоков, открытых нами (лимит MAX_CONCURRENT_STREAMS пира)
					uint32_t localStreamCount;
					// Число отправленных нами SETTINGS, подтверждения которых ещё не получены (RFC 9113 §6.5.3)
					uint32_t settingsAckPending;
					// Порог сигнала writable (low-water)
					size_t sendLowWater;
					// Ёмкость буфера отправки потока (high-water)
					size_t sendHighWater;
					// Порог выходного буфера соединения (backpressure от TCP-стадии)
					size_t outputHighWater;
					/**
					 * @brief Структура ячейки снимка планировщика отправки
					 *
					 */
					typedef struct __AWH_SHARED_EXPORT__ Slot {
						// Срочность потока (RFC 9218 §4.1)
						uint8_t urgency;
						// Признак инкрементальной доставки потока
						bool incremental;
						// Идентификатор потока
						uint32_t id;
						/**
						 * @brief Конструктор
						 *
						 */
						explicit Slot() noexcept : urgency(0), incremental(false), id(0) {}
					} slot_t;
					/**
					 * Переиспользуемый снимок планировщика отправки (без аллокаций на вызов)
					 *
					 * @details Признаки приоритета снимаются вместе с идентификатором, а не
					 *          читаются из карты потоков в компараторе: иначе каждое сравнение
					 *          стоило бы двух хеш-поисков, и упорядочивание снимка обходилось
					 *          дороже самой отправки. Снимок берётся до любых выходов наружу,
					 *          поэтому снятые признаки заведомо те же, что в карте
					 *
					 */
					vector <slot_t> pumpIds;
					/**
					 * Очередь потоков, которым есть что отправлять
					 *
					 * @details Снимок планировщика собирается из неё, а не обходом карты
					 *          потоков: тело отдают единицы из сотен открытых, и полный
					 *          обход стоил бы O(N) на каждый выпущенный DATA-фрейм.
					 *          Запись добавляется там, где поток обзаводится содержимым,
					 *          и снимается лениво - на ближайшей прокачке, когда окажется,
					 *          что отправлять уже нечего. Ложная запись стоит одной проверки,
					 *          поэтому снятие вправе запаздывать, а добавление - нет
					 *
					 */
					vector <uint32_t> readyIds;
					/**
					 * @brief Структура отложенного приоритета ещё не открытого потока
					 *
					 */
					typedef struct __AWH_SHARED_EXPORT__ Pending {
						// Срочность потока (RFC 9218 §4.1)
						uint8_t urgency;
						// Признак инкрементальной доставки потока
						bool incremental;
						// Идентификатор приоритизируемого потока
						uint32_t id;
						/**
						 * @brief Конструктор
						 *
						 */
						explicit Pending() noexcept : urgency(0), incremental(false), id(0) {}
					} pending_t;
					/**
					 * Кольцо приоритетов, объявленных до открытия потока (RFC 9218 §7.1)
					 *
					 * @details Кадр PRIORITY_UPDATE вправе опередить HEADERS, и сигнал по
					 *          потоку в состоянии idle обязан примениться при его открытии.
					 *          Объект потока под сигнал не создаётся: до прихода HEADERS это
					 *          позволило бы пиру наполнить карту потоков даром. Кольцо
					 *          ограничивает память сверху, вытесняя самую старую запись
					 *
					 */
					vector <pending_t> pendingPriorities;
					/**
					 * Переиспользуемый снимок идентификаторов закрываемых потоков (обработка GOAWAY).
					 * Отдельный от pumpIds буфер обязателен: закрытие потока вызывает пользовательскую
					 * функцию обратного вызова, а та вправе отправить данные и реентрантно запустить
					 * pump() - тот перезаполнит свой снимок, и перебор по общему буферу разъедется
					 */
					vector <uint32_t> closeIds;
					// Карта активных потоков
					unordered_map <uint32_t, stream_t> streams;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Transfer() noexcept;
				} transfer_t;
			private:
				/**
				 * @brief Структура функций обратного вызова
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Callbacks {
					/**
					 * @brief Функция обратного вызова для обработки фрагмента тела потока
					 *
					 */
					data_callback_t data;
					/**
					 * @brief Функция обратного вызова для обработки анонса server push
					 *
					 */
					push_callback_t push;
					/**
					 * @brief Функция обратного вызова записи исходящих байтов в сеть
					 *
					 */
					write_callback_t write;
					/**
					 * @brief Функция обратного вызова для обработки открытия нового потока
					 *
					 */
					begin_callback_t begin;
					/**
					 * @brief Функция обратного вызова для обработки закрытия потока
					 *
					 */
					close_callback_t close;
					/**
					 * @brief Функция обратного вызова для обработки ошибки уровня соединения
					 *
					 */
					error_callback_t error;
					/**
					 * @brief Функция обратного вызова для обработки фазы приёма сообщения потока
					 *
					 */
					phase_callback_t phase;
					/**
					 * @brief Функция обратного вызова для обработки заголовков или трейлеров потока
					 *
					 */
					header_callback_t header;
					/**
					 * @brief Функция обратного вызова для обработки полученного GOAWAY
					 *
					 */
					goaway_callback_t goaway;
					/**
					 * @brief Функция обратного вызова для обработки полученного ALTSVC
					 *
					 */
					altsvc_callback_t altsvc;
					/**
					 * @brief Функция обратного вызова для обработки полученного ORIGIN
					 *
					 */
					origin_callback_t origin;
					/**
					 * @brief Функция обратного вызова о готовности потока принимать данные тела
					 *
					 */
					writable_callback_t writable;
					/**
					 * @brief Функция обратного вызова для обработки применённого SETTINGS пира
					 *
					 */
					settings_callback_t settings;
					/**
					 * @brief Функция обратного вызова для обработки провайдера заголовков потока
					 *
					 */
					provider_callback_t provider;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Callbacks() noexcept;
				} callbacks_t;
			private:
				// Флаги состояния соединения
				flags_t _flags;
			private:
				/**
				 * Поколение состояния соединения: увеличивается каждым сбросом (reset/clear).
				 * Пользовательская функция обратного вызова вправе сбросить парсер прямо
				 * из обработчика - после этого все ссылки на разбираемые данные, снимки
				 * потоков и списки заголовков недействительны, и разбор обязан свернуться
				 */
				uint64_t _epoch;
			private:
				// Код ошибки уровня соединения
				error_t _error;
			private:
				// Буферы соединения
				buffer_t _buffer;
			private:
				// Окна flow control соединения
				window_t _window;
			private:
				// Протокол, с которым работает парсер
				proto_t _proto;
			private:
				// Настраиваемые лимиты безопасности
				limits_t _limits;
			private:
				// Наши параметры SETTINGS
				settings_t _local;
				// Параметры SETTINGS пира
				settings_t _remote;
			private:
				// Состояние собираемого блока заголовков (HEADERS + CONTINUATION)
				header_block_t _hbc;
			private:
				// Rate-лимиты соединения
				ratelims_t _ratelims;
			private:
				// Параметры передачи данных
				transfer_t _transfer;
			private:
				// Объект функций обратного вызова
				callbacks_t _callbacks;
			private:
				/**
				 * Переиспользуемый список декодированных заголовков блока: представления
				 * ссылаются в арену декодера и действительны до следующего декодирования
				 */
				vector <h2::hpack::field_view_t> _fields;
			private:
				// HPACK-энкодер (наша динамическая таблица)
				h2::hpack::encoder_t _encoder;
				// HPACK-декодер (динамическая таблица пира)
				h2::hpack::decoder_t _decoder;
			private:
				/**
				 * @brief Метод передачи исходящих байтов сетевому слою через функцию обратного вызова записи
				 *
				 * @details Если функция записи не установлена - байты остаются во внутреннем
				 *          буфере до выборки через pending()/consumePending().
				 *
				 */
				void flush() noexcept;
				/**
				 * @brief Метод очистки буфера незавершённого фрейма
				 *
				 */
				void clearInput() noexcept;
				/**
				 * @brief Метод получения объёма накопленного незавершённого фрейма
				 *
				 * @return объём накопленной части незавершённого фрейма
				 *
				 */
				size_t inputPending() const noexcept;
				/**
				 * @brief Метод получения логического объёма ещё не отправленных исходящих байтов
				 *
				 * @return объём не отправленных исходящих байтов
				 *
				 */
				size_t outputPending() const noexcept;
			private:
				/**
				 * @brief Метод разбора порции входящих байтов (preface + поток фреймов)
				 *
				 * @details Разбор идёт прямо по буферу вызывающей стороны: копии входа
				 *          у парсера нет. Разбирается столько целых фреймов, сколько
				 *          уложилось в порцию; неразобранный хвост остаётся вызывающей
				 *          стороне и подаётся снова со следующей порцией
				 *
				 * @param buffer буфер входящих байтов
				 * @param size   размер буфера входящих байтов
				 * @return       результат разбора (OK/ERROR)
				 *
				 */
				h2::status_t parseInput(const uint8_t * buffer, const size_t size) noexcept;
				/**
				 * @brief Метод обработки одного полностью собранного фрейма
				 *
				 * @param header  заголовок фрейма
				 * @param payload полезная нагрузка фрейма
				 * @return        результат обработки (OK/ERROR)
				 *
				 */
				h2::status_t parseFrame(const h2::frame::header_t & header, const uint8_t * payload) noexcept;
				/**
				 * @brief Метод декодирования накопленного блока заголовков и вызова функций обратного вызова
				 *
				 * @return результат обработки (OK/ERROR)
				 *
				 */
				h2::status_t deliverHeaders() noexcept;
				/**
				 * @brief Метод аварийного завершения соединения (ошибка, GOAWAY, запись в лог)
				 *
				 * @param code    код ошибки протокола
				 * @param message текстовое описание ошибки
				 * @return        статус ошибки (для проброса из обработчиков)
				 *
				 */
				h2::status_t fail(const error_t code, const char * message) noexcept;
				/**
				 * @brief Метод проверки корректности нового потока, открываемого пиром (чётность + монотонность id)
				 *
				 * @param id  идентификатор потока
				 * @param err код ошибки протокола
				 * @return    результат проверки (OK/ERROR)
				 *
				 */
				h2::status_t validateNewStream(const uint32_t id, error_t & err) noexcept;
				/**
				 * @brief Метод обработки одного полного фрейма
				 *
				 * @param header  заголовок фрейма
				 * @param payload полезная нагрузка фрейма (ровно h.length байт)
				 * @return        результат обработки (OK/ERROR)
				 *
				 */
				h2::status_t dispatch(const h2::frame::header_t & header, const uint8_t * payload) noexcept;
				/**
				 * @brief Метод доставки декодированного блока обещанного запроса (PUSH_PROMISE, сторона клиента)
				 *
				 * @param sid         идентификатор ассоциированного потока клиента
				 * @param promisedSid идентификатор обещанного потока
				 * @param fields      декодированные заголовки обещанного запроса
				 * @return            результат обработки (OK/ERROR)
				 *
				 */
				h2::status_t deliverPushPromise(const uint32_t sid, const uint32_t promisedSid, const vector <h2::hpack::field_view_t> & fields) noexcept;
			private:
				/**
				 * @brief Метод получения существующего либо создания нового потока
				 *
				 * @param id идентификатор потока
				 * @return   объект потока
				 *
				 */
				stream_t & stream(const uint32_t id) noexcept;
				/**
				 * @brief Метод поиска потока без создания
				 *
				 * @param id идентификатор потока
				 * @return   объект потока либо nullptr
				 *
				 */
				stream_t * findStream(const uint32_t id) noexcept;
			private:
				/**
				 * @brief Метод применения отправленного нами END_STREAM (переход состояния, возможно закрытие потока)
				 *
				 * @param stream объект потока (ссылка может стать недействительной после вызова)
				 *
				 */
				void applyLocalEndStream(stream_t & stream) noexcept;
				/**
				 * @brief Метод применения полученного END_STREAM (переход состояния, возможно закрытие потока)
				 *
				 * @param stream объект потока (ссылка может стать недействительной после вызова)
				 *
				 */
				void applyRemoteEndStream(stream_t & stream) noexcept;
			private:
				/**
				 * @brief Метод проверки того, что поток инициирован пиром (а не нами)
				 *
				 * @param id идентификатор потока
				 * @return   результат проверки
				 *
				 */
				bool peerInitiated(const uint32_t id) const noexcept;
				/**
				 * @brief Метод проверки того, что поток ещё ни разу не использовался (состояние idle)
				 *
				 * @details Поток пира считается использованным, пока его идентификатор не превышает
				 *          наибольший принятый; наш собственный - если он уже выдан nextStreamId().
				 *          Для закрытого и удалённого из карты потока метод возвращает false:
				 *          запоздалые фреймы на нём - штатная гонка, а не ошибка соединения
				 *
				 * @param id идентификатор потока
				 * @return   результат проверки
				 *
				 */
				bool idleStream(const uint32_t id) const noexcept;
				/**
				 * @brief Метод запоминания потока, оборванного сбросом
				 *
				 * @details Кадры на оборванном потоке ещё могут быть в полёте: пир отправил
				 *          их до того, как получил наш RST_STREAM. Помнить об этом надо
				 *          и для потоков пира, и для наших собственных: клиент так же
				 *          вправе получить запоздалый ответ на поток, который сам и сбросил
				 *
				 * @param id    идентификатор потока
				 * @param local сброс отправлен нами (иначе - принят от пира)
				 *
				 */
				void markReset(const uint32_t id, const bool local) noexcept;
				/**
				 * @brief Метод проверки того, что поток был недавно оборван сбросом
				 *
				 * @param id идентификатор потока
				 * @return   результат проверки
				 *
				 */
				bool wasReset(const uint32_t id) const noexcept;
				/**
				 * @brief Метод проверки того, что поток был оборван нашим сбросом
				 *
				 * @details После нашего RST_STREAM кадры пира обязаны молча игнорироваться:
				 *          он отправил их до того, как получил сброс, и повторный кадр сброса
				 *          ему ни о чём не сообщит (RFC 9113 §5.1)
				 *
				 * @param id идентификатор потока
				 * @return   результат проверки
				 *
				 */
				bool resetLocally(const uint32_t id) const noexcept;
				/**
				 * @brief Метод отправки RST_STREAM с учётом оборванного потока
				 *
				 * @details Единственная точка отправки сброса внутри модуля: учёт оборванного
				 *          потока и сам кадр обязаны идти вместе, иначе запоздалые кадры
				 *          на нём выглядели бы как кадры на никогда не открывавшемся потоке
				 *          и рвали бы соединение
				 *
				 * @param id   идентификатор потока
				 * @param code код ошибки, с которым обрывается поток
				 *
				 */
				void rejectStream(const uint32_t id, const error_t code) noexcept;
			private:
				/**
				 * @brief Метод удаления потока из карты с корректным учётом счётчика встречных потоков
				 *
				 * @param id идентификатор потока
				 *
				 */
				void eraseStream(const uint32_t id) noexcept;
				/**
				 * @brief Метод закрытия потока с вызовом функции обратного вызова закрытия
				 *
				 * @param id   идентификатор потока
				 * @param code код ошибки закрытия
				 *
				 */
				void closeStream(const uint32_t id, const error_t code) noexcept;
			private:
				/**
				 * @brief Метод вызова функции обратного вызова обработки фазы приёма сообщения потока
				 *
				 * @details При возврате false пользовательской функцией (или исключении)
				 *          поток сбрасывается (RST_STREAM с кодом CANCEL) и метод возвращает false.
				 *
				 * @param id    идентификатор потока
				 * @param phase фаза приёма сообщения потока
				 * @param part  часть сообщения (заголовки, трейлеры, тело), NONE - сообщение целиком
				 * @return      результат обработки (false - поток сброшен)
				 *
				 */
				bool firePhase(const uint32_t id, const phase_t phase, const part_t part) noexcept;
			private:
				/**
				 * @brief Метод вызова функции обратного вызова обработки применённого SETTINGS пира
				 *
				 * @details Все методы разбора объявлены noexcept, поэтому исключение из
				 *          пользовательской функции обязано быть перехвачено на месте вызова -
				 *          иначе оно завершает процесс, не доходя до обработчика в parse()
				 *
				 */
				void fireSettings() noexcept;
				/**
				 * @brief Метод вызова функции обратного вызова о готовности потока принимать данные
				 *
				 * @param id идентификатор потока
				 *
				 */
				void fireWritable(const uint32_t id) noexcept;
				/**
				 * @brief Метод вызова функции обратного вызова обработки открытия нового потока
				 *
				 * @param id идентификатор потока
				 * @return   результат обработки (false - поток требуется сбросить)
				 *
				 */
				bool fireBegin(const uint32_t id) noexcept;
				/**
				 * @brief Метод вызова функции обратного вызова обработки анонса server push
				 *
				 * @param sid         идентификатор ассоциированного потока клиента
				 * @param promisedSid идентификатор обещанного потока
				 * @return            результат обработки (false - push требуется отклонить)
				 *
				 */
				bool firePush(const uint32_t sid, const uint32_t promisedSid) noexcept;
				/**
				 * @brief Метод вызова функции обратного вызова обработки полученного GOAWAY
				 *
				 * @param sid   наибольший идентификатор обработанного пиром потока
				 * @param code  код ошибки завершения соединения
				 * @param debug отладочные данные пира
				 *
				 */
				void fireGoaway(const uint32_t sid, const error_t code, const string_view debug) noexcept;
				/**
				 * @brief Метод вызова функции обратного вызова обработки провайдера заголовков потока
				 *
				 * @param id        идентификатор потока
				 * @param provider  провайдер заголовков потока (nullptr для трейлеров)
				 * @param endStream флаг завершения потока
				 * @return          результат обработки (false - поток требуется сбросить)
				 *
				 */
				bool fireProvider(const uint32_t id, const provider_t * provider, const bool endStream) noexcept;
				/**
				 * @brief Метод вызова функции обратного вызова обработки фрагмента тела потока
				 *
				 * @param id        идентификатор потока
				 * @param buffer    буфер данных тела
				 * @param size      размер данных тела
				 * @param endStream флаг завершения потока
				 * @return          результат обработки (false - поток требуется сбросить)
				 *
				 */
				bool fireData(const uint32_t id, const void * buffer, const size_t size, const bool endStream) noexcept;
				/**
				 * @brief Метод вызова функции обратного вызова обработки заголовка или трейлера потока
				 *
				 * @param id    идентификатор потока
				 * @param name  название заголовка
				 * @param value значение заголовка
				 * @param part  часть сообщения (HEADERS или TRAILER)
				 * @return      результат обработки (false - поток требуется сбросить)
				 *
				 */
				bool fireHeader(const uint32_t id, const string_view name, const string_view value, const part_t part) noexcept;
			private:
				/**
				 * @brief Метод постановки потока в очередь готовых к отправке
				 *
				 * @details Вызывается там, где поток обзаводится содержимым: буфером тела,
				 *          источником данных, отложенным завершением либо трейлерами.
				 *          Пропущенный вызов оставит содержимое неотправленным, поэтому
				 *          добавление обязано опережать прокачку
				 *
				 * @param stream объект потока
				 *
				 */
				void markReady(stream_t & stream) noexcept;
				/**
				 * @brief Метод прокачки отправки по всем потокам с учётом окон и порога выходного буфера
				 *
				 * @details Round-robin: за каждый проход отправляется не более одного DATA-фрейма
				 *          с потока, пока хоть один поток делает прогресс - исключает голодание
				 *          потоков (head-of-line blocking).
				 *
				 */
				void pump() noexcept;
				/**
				 * @brief Метод отправки не более одного DATA-фрейма потока
				 *
				 * @param stream объект потока (ссылка может стать недействительной после вызова)
				 * @return       признак прогресса отправки
				 *
				 */
				bool pumpStream(stream_t & stream) noexcept;
				/**
				 * @brief Метод дозагрузки буфера отправки из pull-источника данных (если он задан)
				 *
				 * @param stream объект потока
				 *
				 */
				void refillFromSource(stream_t & stream) noexcept;
				/**
				 * @brief Метод сигнализации о готовности потока принимать данные (один раз на провал буфера)
				 *
				 * @param stream объект потока
				 *
				 */
				void maybeNotifyWritable(stream_t & stream) noexcept;
				/**
				 * @brief Метод проверки того, что все данные потока для отправки уже получены
				 *
				 * @param stream объект потока
				 * @return       результат проверки (нет источника данных или достигнут его eof)
				 *
				 */
				bool sourceDone(const stream_t & stream) const noexcept;
				/**
				 * @brief Метод пополнения окна приёма (соединения/потока) с отправкой WINDOW_UPDATE при просадке
				 *
				 * @param stream   объект потока (nullptr - только окно соединения)
				 * @param consumed число принятых байт
				 *
				 */
				void replenishReceiveWindow(stream_t * stream, const uint32_t consumed) noexcept;
			private:
				/**
				 * @brief Метод проверки декодированных заголовков на лимиты безопасности
				 *
				 * @param fields декодированные заголовки блока
				 * @return       результат проверки (false - лимиты превышены)
				 *
				 */
				bool checkHeaderLimits(const vector <h2::hpack::field_view_t> & fields) const noexcept;
				/**
				 * @brief Метод предупреждения о полностью снятом лимите списка заголовков
				 *
				 * @details Вызывается при изменении лимитов безопасности и параметров SETTINGS:
				 *          снятие обоих лимитов сразу оставляет арену декодера без границы
				 *
				 */
				void checkHeaderListLimits() const noexcept;
				/**
				 * @brief Метод сверки отправляемого блока заголовков с лимитом пира
				 *
				 * @details Пир анонсирует SETTINGS_MAX_HEADER_LIST_SIZE как рекомендацию
				 *          (RFC 9113 §6.5.2) и вправе отвергнуть превышающий её блок.
				 *          Отправку не блокирует - только предупреждает в лог
				 *
				 * @param sid идентификатор потока
				 *
				 */
				void checkPeerHeaderList(const uint32_t sid) const noexcept;
				/**
				 * @brief Метод проверки соответствия принятого тела объявленному content-length
				 *
				 * @details RFC 9113 §8.1.1: расхождение суммы длин DATA с content-length делает
				 *          сообщение малформированным. При расхождении поток сбрасывается
				 *
				 * @param sid идентификатор потока
				 * @return    результат проверки (false - поток сброшен)
				 *
				 */
				bool checkBodyLength(const uint32_t sid) noexcept;
				/**
				 * @brief Метод применения расширенного приоритета к потоку (RFC 9218 §4)
				 *
				 * @details Значение - структурированный словарь вида "u=2, i": ключ [u] задаёт
				 *          срочность 0..7 (по умолчанию 3), ключ [i] - инкрементальную доставку.
				 *          Неизвестные и некорректные ключи игнорируются (RFC 9218 §4.3)
				 *
				 * @param stream объект потока
				 * @param value  значение поля приоритета
				 *
				 */
				void applyPriority(stream_t & stream, string_view value) noexcept;
				/**
				 * @brief Метод разбора значения поля расширенного приоритета (RFC 9218 §4)
				 *
				 * @details Сигнал задаёт приоритет целиком: параметр, в нём отсутствующий,
				 *          принимает значение по умолчанию, а не сохраняет прежнее
				 *
				 * @param value       значение поля приоритета
				 * @param urgency     срочность потока (выходной параметр)
				 * @param incremental признак инкрементальной доставки (выходной параметр)
				 *
				 */
				void parsePriority(const string_view value, uint8_t & urgency, bool & incremental) const noexcept;
				/**
				 * @brief Метод запоминания приоритета ещё не открытого потока (RFC 9218 §7.1)
				 *
				 * @param id    идентификатор приоритизируемого потока
				 * @param value значение поля приоритета
				 * @return      результат запоминания (false - исчерпан лимит одновременных потоков)
				 *
				 */
				bool deferPriority(const uint32_t id, const string_view value) noexcept;
				/**
				 * @brief Метод применения приоритета, отложенного до открытия потока
				 *
				 * @details Вместе с применённой записью снимаются записи потоков с меньшими
				 *          идентификаторами: идентификаторы пира строго возрастают
				 *          (RFC 9113 §5.1.1), и открыты такие потоки уже не будут
				 *
				 * @param stream объект открываемого потока
				 *
				 */
				void applyPendingPriority(stream_t & stream) noexcept;
			private:
				/**
				 * @brief Метод отправки собранного HPACK-блока заголовков потока
				 *
				 * @details Общая часть перегрузок sendHeaders: нарезка блока на
				 *          HEADERS + CONTINUATION и переходы состояния потока.
				 *
				 * @param sid       идентификатор потока
				 * @param block     закодированный HPACK-блок заголовков
				 * @param endStream флаг завершения потока (тела не будет)
				 *
				 */
				void commitHeaders(const uint32_t sid, const string & block, const bool endStream);
				/**
				 * @brief Метод откладывания секции трейлеров до конца отправки тела потока
				 *
				 * @details Трейлеры идут после тела, а тело может ждать открытия окна
				 *          управления потоком. Отправить их сразу означает выпустить
				 *          в сеть блок заголовков раньше данных, которые он завершает.
				 *          Откладываются именно поля: кодирование HPACK обязано
				 *          совпадать по порядку с отправкой
				 *
				 * @param sid       идентификатор потока
				 * @param fields    заголовки секции трейлеров
				 * @param endStream флаг завершения потока
				 * @return          результат откладывания (true - отправка отложена)
				 *
				 */
				bool deferTrailers(const uint32_t sid, const vector <h2::hpack::field_t> & fields, const bool endStream) noexcept;
				/**
				 * @brief Метод отправки отложенной секции трейлеров потока
				 *
				 * @param stream объект потока (ссылка может стать недействительной после вызова)
				 * @return       признак отправки секции трейлеров
				 *
				 */
				bool flushTrailers(stream_t & stream) noexcept;
				/**
				 * @brief Метод проверки допустимости отправки блока заголовков в поток
				 *
				 * @details Отправка допустима в существующий поток (ответ/трейлеры) либо
				 *          в новый поток, идентификатор которого выделен нами через
				 *          nextStreamId() и ещё не использовался. Проверка выполняется
				 *          ДО кодирования блока: HPACK-кодирование меняет динамическую
				 *          таблицу, поэтому отброшенный после кодирования блок
				 *          рассинхронизировал бы декодер пира.
				 *
				 * @param sid идентификатор потока
				 * @return    результат проверки
				 *
				 */
				bool canSendHeaders(const uint32_t sid) noexcept;
			private:
				/**
				 * @brief Метод построения провайдера заголовков потока из псевдо-заголовков
				 *
				 * @param fields  декодированные заголовки блока
				 * @param request собирается запрос клиента (true) или ответ сервера (false)
				 * @return        собранный провайдер заголовков
				 *
				 */
				unique_ptr <provider_t> buildProvider(const vector <h2::hpack::field_view_t> & fields, const bool request) const noexcept;
				/**
				 * @brief Метод проверки того, что расширенный CONNECT разрешён нами
				 *
				 * @details Разрешение выдаётся параметром SETTINGS_ENABLE_CONNECT_PROTOCOL
				 *          либо подразумевается ролью узла: соединение, объявленное несущим
				 *          WebSocket, иначе отвергало бы единственный запрос, ради которого
				 *          заведено (RFC 8441 §3, §5)
				 *
				 * @return признак разрешения расширенного CONNECT
				 *
				 */
				bool connectProtocol() const noexcept;
			public:
				/**
				 * @brief Метод полной очистки всех данных парсера
				 *
				 * @details Помимо полного сброса состояния соединения возвращает лимиты
				 *          безопасности и параметры SETTINGS к значениям по умолчанию
				 *          и удаляет установленные функции обратного вызова.
				 *
				 */
				void clear() noexcept override;
				/**
				 * @brief Метод полного сброса состояния соединения
				 *
				 * @details В отличие от HTTP/1.x, где reset() готовит парсер к следующему
				 *          сообщению, для HTTP/2 сбрасывается ВСЁ соединение: HPACK-таблицы,
				 *          карта потоков, окна, буферы (семантика нового соединения).
				 *          Лимиты безопасности, параметры SETTINGS и функции обратного
				 *          вызова сохраняются.
				 *
				 */
				void reset() noexcept override;
			public:
				/**
				 * @brief Метод клонирования объекта парсера
				 *
				 * @details Клон получает те же направление трафика, роль узла на соединении,
				 *          лимиты безопасности, параметры SETTINGS и функции обратного вызова,
				 *          но чистое состояние соединения ("фабрика с теми же настройками").
				 *
				 * @return копия объекта парсера
				 *
				 */
				unique_ptr <parser_t> clone() const noexcept override;
			public:
				/**
				 * @brief Метод уведомления парсера о завершении потока данных (закрытии соединения)
				 *
				 * @details Если соединение завершено корректно (обменялись GOAWAY, активных
				 *          потоков нет) - фиксируется статус COMPLETE. Если соединение закрыто
				 *          посреди активных потоков или незавершённого фрейма - фиксируется
				 *          ошибка PROTOCOL_ERROR (обрыв соединения).
				 *
				 */
				void eof() noexcept override;
				/**
				 * @brief Метод разбора данных
				 *
				 * @details Скармливает парсеру очередную порцию входящих байтов соединения.
				 *          Неполный хвост фрейма буферизуется внутри до следующего вызова,
				 *          поэтому метод всегда потребляет все переданные байты. По ходу
				 *          разбора вызываются функции обратного вызова, а обязательные
				 *          ответные фреймы уходят в канал записи.
				 *          Итоговый статус необходимо контролировать методом status():
				 *          - PARTIAL:  соединение живо, разбор продолжается;
				 *          - COMPLETE: соединение завершено (обменялись GOAWAY);
				 *          - ERROR:    ошибка уровня соединения - причина в методе error().
				 *
				 * @param buffer буфер данных для разбора
				 * @param size   размер данных для разбора
				 * @return       количество обработанных байт данных
				 *
				 */
				size_t parse(const void * buffer, const size_t size) noexcept override;
			public:
				/**
				 * @brief Метод получения кода ошибки уровня соединения
				 *
				 * @return код ошибки протокола
				 *
				 */
				error_t error() const noexcept;
				/**
				 * @brief Метод получения человекочитаемого названия текущей ошибки разбора
				 *
				 * @return название текущей ошибки разбора
				 *
				 */
				string_view errorName() const noexcept override;
				/**
				 * @brief Метод получения человекочитаемого названия кода ошибки
				 *
				 * @param error код ошибки протокола
				 * @return      название кода ошибки
				 *
				 */
				static string_view errorName(const error_t error) noexcept;
			public:
				/**
				 * @brief Метод получения протокола, с которым работает парсер
				 *
				 * @return протокол работы парсера
				 *
				 */
				proto_t proto() const noexcept;
				/**
				 * @brief Метод установки протокола, с которым работает парсер
				 *
				 * @details Парсер говорит только на HTTP/2, поэтому допустимы лишь
				 *          значения этого семейства: HTTP2 - прямое соединение с узлом,
				 *          PROXY2 - работа промежуточным узлом, WEBSOCKET2 - соединение,
				 *          несущее WebSocket поверх расширенного CONNECT. Значение
				 *          любого другого семейства отвергается с записью в лог:
				 *          разбирать HTTP/1.x либо HTTP/3 этот парсер не умеет, и
				 *          молчаливое принятие такого указания создало бы у вызывающей
				 *          стороны ложное представление о происходящем.
				 *          По умолчанию установлен HTTP2.
				 *
				 * @note Режим промежуточного узла ужесточает приём: конечному получателю
				 *       достаточно минимальной проверки полей (RFC 9113 §8.2.1), а узлу,
				 *       передающему сообщение дальше в другой версии протокола, - нет.
				 *       Отвергается то, границу чего следующее звено определит иначе:
				 *       управляющие символы в значениях полей, пробелы и управляющие
				 *       символы в псевдо-заголовках, из которых собирается стартовая
				 *       строка, а также объявленная длина тела у ответов 1xx и 204
				 *       (RFC 9113 §10.3). Подробности - в README модуля
				 *
				 * @param proto протокол работы парсера
				 *
				 */
				void proto(const proto_t proto) noexcept;
			public:
				/**
				 * @brief Метод получения лимитов безопасности
				 *
				 * @return лимиты безопасности
				 *
				 */
				const limits_t & limits() const noexcept;
				/**
				 * @brief Метод установки лимитов безопасности
				 *
				 * @param limits лимиты безопасности
				 *
				 */
				void limits(const limits_t & limits) noexcept;
			public:
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
				 * @note Отправка выполняется методами sendPreface()/sendSettings()
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
			public:
				/**
				 * @brief Метод проверки того, что соединение помечено на завершение
				 *
				 * @return признак завершения (отправлен или получен GOAWAY)
				 *
				 */
				bool isClosed() const noexcept;
				/**
				 * @brief Метод проверки того, что наш SETTINGS подтверждён пиром
				 *
				 * @details Позволяет внешнему таймеру отследить отсутствие ACK и завершить
				 *          соединение с кодом SETTINGS_TIMEOUT (RFC 9113 §6.5.3)
				 *
				 * @return признак получения ACK на наш SETTINGS
				 *
				 */
				bool isSettingsAcked() const noexcept;
			public:
				/**
				 * @brief Метод отправки исходящего preface соединения
				 *
				 * @details Клиент отправляет magic-строку + свой SETTINGS, сервер - только SETTINGS.
				 *          Обязан быть первым исходящим сообщением соединения.
				 *
				 */
				void sendPreface() noexcept;
				/**
				 * @brief Метод отправки нашего SETTINGS-фрейма (текущие параметры)
				 *
				 */
				void sendSettings() noexcept;
			public:
				/**
				 * @brief Метод отправки RST_STREAM (аварийное закрытие потока)
				 *
				 * @note Поток удаляется из карты активных с вызовом функции обратного вызова
				 *       закрытия - так же, как при сбросе потока пиром
				 *
				 * @param sid  идентификатор потока
				 * @param code код ошибки, с которым сбрасывается поток
				 *
				 */
				void sendRstStream(const uint32_t sid, const error_t code) noexcept;
				/**
				 * @brief Метод отправки GOAWAY (пометка соединения завершаемым)
				 *
				 * @param code  код ошибки завершения соединения
				 * @param debug необязательные отладочные данные
				 *
				 */
				void sendGoaway(const error_t code, string_view debug = {}) noexcept;
				/**
				 * @brief Метод начала плавного завершения соединения (RFC 9113 §6.8)
				 *
				 * @details Отправляет предупреждающий GOAWAY с максимальным идентификатором
				 *          потока: пир узнаёт о предстоящем закрытии, но уже начатые потоки
				 *          не отклоняются и новые формально ещё допустимы. Через интервал
				 *          порядка RTT (например, после ответа на PING) вызовите sendGoaway()
				 *          с фактическим кодом - он объявит реально обработанный поток
				 *          и запретит новые. Без второй фазы соединение не завершается.
				 *
				 * @param debug необязательные отладочные данные
				 *
				 */
				void sendShutdown(string_view debug = {}) noexcept;
				/**
				 * @brief Метод отправки расширенного приоритета потока (RFC 9218 §7.1)
				 *
				 * @details Кадр PRIORITY_UPDATE перепланирует уже открытый поток. Отправляется
				 *          только если пир объявил отказ от приоритетов RFC 7540 либо явно
				 *          поддерживает расширенные приоритеты
				 *
				 * @param sid         идентификатор потока
				 * @param urgency     срочность потока (0 - наивысшая, 7 - наименьшая)
				 * @param incremental признак инкрементальной доставки
				 *
				 */
				void sendPriority(const uint32_t sid, const uint8_t urgency, const bool incremental) noexcept;
				/**
				 * @brief Метод объявления приоритета собственного потока заголовком (RFC 9218 §5)
				 *
				 * @details Приоритет применяется к планировщику отправки сразу, а в секцию
				 *          заголовков потока добавляется заголовок [priority] - при условии,
				 *          что приложение не задало его само. Это единственный способ
				 *          объявить приоритет для сервера: кадр PRIORITY_UPDATE ему запрещён
				 *
				 * @note Заголовок добавляется только перегрузкой sendHeaders с контейнером
				 *       headers_t. Перегрузка со списком полей отдаёт их как есть -
				 *       это её контракт
				 *
				 * @param sid         идентификатор потока
				 * @param urgency     срочность потока (0 - наивысшая, 7 - наименьшая)
				 * @param incremental признак инкрементальной доставки
				 *
				 */
				void priority(const uint32_t sid, const uint8_t urgency, const bool incremental) noexcept;
				/**
				 * @brief Метод отправки анонса альтернативного сервиса (RFC 7838 §4)
				 *
				 * @details Кадр отправляет только сервер. Нулевой идентификатор потока
				 *          означает анонс для соединения и требует непустого origin;
				 *          анонс для потока, наоборот, требует пустого - origin там
				 *          определяется самим потоком
				 *
				 * @param sid    идентификатор потока, либо 0 для соединения
				 * @param origin origin анонсируемого сервиса
				 * @param value  значение поля Alt-Svc (RFC 7838 §3)
				 *
				 */
				void sendAltSvc(const uint32_t sid, const string & origin, const string & value) noexcept;
				/**
				 * @brief Метод отправки набора origin, обслуживаемых соединением (RFC 8336 §2)
				 *
				 * @details Кадр отправляет только сервер и только для соединения целиком.
				 *          Пустой набор законен и означает, что соединение не обслуживает
				 *          ни одного origin сверх того, для которого установлено
				 *
				 * @param origins набор origin
				 *
				 */
				void sendOrigin(const vector <string> & origins) noexcept;
				/**
				 * @brief Метод отправки WINDOW_UPDATE
				 *
				 * @param sid       идентификатор потока (0 - окно всего соединения)
				 * @param increment инкремент окна flow control
				 *
				 */
				void sendWindowUpdate(const uint32_t sid, const uint32_t increment) noexcept;
			public:
				/**
				 * @brief Метод передачи части тела потока для отправки (push-модель, bounded buffer)
				 *
				 * @details Копирует во внутренний буфер потока столько байт, сколько влезает до
				 *          high-water, и возвращает это число (0..size). Если вернулось меньше
				 *          size - буфер заполнен: приостановите выдачу и дождитесь функции
				 *          обратного вызова writable. Нарезку во фреймы, учёт окон и
				 *          автоматическую досылку по WINDOW_UPDATE парсер делает сам.
				 *
				 * @param sid       идентификатор потока
				 * @param buffer    буфер данных тела
				 * @param size      размер данных тела
				 * @param endStream флаг завершения потока
				 * @return          число принятых байт (0..size)
				 *
				 */
				size_t sendData(const uint32_t sid, const void * buffer, const size_t size, const bool endStream) noexcept;
			public:
				/**
				 * @brief Метод анонса server push (только сервер)
				 *
				 * @details Отправляет PUSH_PROMISE на потоке клиента и резервирует чётный
				 *          push-поток. Дальше ответ отправляется обычным путём:
				 *          sendHeaders(promisedSid, ...) + sendData(promisedSid, ...).
				 *
				 * @param sid    идентификатор потока клиента, в ответ на который выполняется push
				 * @param fields заголовки обещанного запроса (псевдо-заголовки как у запроса клиента)
				 * @return       идентификатор зарезервированного push-потока либо 0, если push невозможен
				 *
				 */
				uint32_t sendPushPromise(const uint32_t sid, const vector <h2::hpack::field_t> & fields) noexcept;
			public:
				/**
				 * @brief Метод отправки блока заголовков (запрос/ответ/трейлеры) потока
				 *
				 * @details Если поток ещё не существует и мы инициатор - поток открывается.
				 *          При endStream поток сразу полузакрывается с нашей стороны (тела не будет).
				 *          Блок, превышающий SETTINGS_MAX_FRAME_SIZE пира, автоматически режется
				 *          на HEADERS + CONTINUATION (RFC 9113 §6.2/§6.10).
				 *
				 * @param sid       идентификатор потока
				 * @param fields    заголовки (псевдо-заголовки :method/:path/... должны идти первыми)
				 * @param endStream флаг завершения потока (тела не будет)
				 *
				 */
				void sendHeaders(const uint32_t sid, const vector <h2::hpack::field_t> & fields, const bool endStream) noexcept;
				/**
				 * @brief Метод отправки блока заголовков потока из контейнера заголовков (zero-copy)
				 *
				 * @details Заголовки кодируются в HPACK напрямую из контейнера, без промежуточных
				 *          копий. Псевдо-заголовки формируются автоматически из провайдера контейнера:
				 *          для запроса (request_t) - [:method]/[:scheme]/[:authority]/[:path]
				 *          (для метода CONNECT - только [:method]/[:authority], RFC 9113 §8.5),
				 *          для ответа (response_t) - [:status]. Заголовок Host конвертируется
				 *          в [:authority]. Если провайдер контейнера не установлен - блок кодируется
				 *          без псевдо-заголовков (трейлеры). Названия заголовков приводятся к нижнему
				 *          регистру (RFC 9113 §8.2.1), запрещённые в HTTP/2 connection-specific
				 *          заголовки (Connection/Keep-Alive/Proxy-Connection/Transfer-Encoding/Upgrade,
				 *          а также TE со значением кроме "trailers") пропускаются (RFC 9113 §8.2.2).
				 *
				 * @param sid       идентификатор потока
				 * @param headers   контейнер заголовков (провайдер контейнера задаёт псевдо-заголовки)
				 * @param endStream флаг завершения потока (тела не будет)
				 * @param scheme    схема запроса для псевдо-заголовка [:scheme] (для ответа не используется)
				 *
				 */
				void sendHeaders(const uint32_t sid, const headers_t & headers, const bool endStream, string_view scheme = "https") noexcept;
			public:
				/**
				 * @brief Метод выделения идентификатора для нового инициируемого нами потока
				 *
				 * @details Клиент получает нечётные идентификаторы (1, 3, 5...), выделенный
				 *          идентификатор передаётся в sendHeaders() для открытия потока.
				 *
				 * @return идентификатор нового потока
				 *
				 */
				uint32_t nextStreamId() noexcept;
				/**
				 * @brief Метод назначения pull-источника данных тела потока
				 *
				 * @details Источник пишет данные напрямую в буфер отправки потока и возвращает
				 *          число записанных байт (отрицательное значение сбрасывает поток).
				 *          Закрывать поток прямо из источника допустимо, но переданный ему
				 *          буфер после этого уничтожен - записывать в него уже нельзя.
				 *
				 * @param sid    идентификатор потока
				 * @param source pull-источник данных тела
				 *
				 */
				void dataSource(const uint32_t sid, data_source_callback_t source) noexcept;
			public:
				/**
				 * @brief Метод настройки порога выходного буфера соединения (backpressure от TCP-стадии)
				 *
				 * @param high порог выходного буфера соединения
				 *
				 */
				void outputHighWater(const size_t high) noexcept;
				/**
				 * @brief Метод настройки порогов буфера отправки потока
				 *
				 * @param high ёмкость буфера отправки потока (high-water)
				 * @param low  порог сигнала writable (low-water)
				 *
				 */
				void sendWaterMarks(const size_t high, const size_t low) noexcept;
			public:
				/**
				 * @brief Метод сообщения текущего монотонного времени для пополнения rate-лимитов
				 *
				 * @details Вызывайте периодически (например, перед parse); необязательно -
				 *          без обновления времени работает только стартовый запас burst.
				 *
				 * @param seconds текущее монотонное время (секунды)
				 *
				 */
				void updateTime(const uint64_t seconds) noexcept;
			public:
				/**
				 * @brief Метод увеличения приёмного окна соединения
				 *
				 * @details По умолчанию окно соединения 65535 байт - узкое место при высокой
				 *          пропускной способности. Поднимает целевой размер окна приёма и сразу
				 *          отправляет WINDOW_UPDATE(0) на разницу. Только увеличение.
				 *
				 * @param size новый целевой размер окна приёма соединения
				 *
				 */
				void connectionReceiveWindow(const int32_t size) noexcept;
			public:
				/**
				 * @brief Метод получения ещё не отправленных исходящих байтов (pull-модель)
				 *
				 * @details View действителен до следующего вызова любого метода парсера.
				 *          После записи в сокет освободите отправленную часть методом
				 *          consumePending(). При установленной функции обратного вызова
				 *          записи буфер опустошается автоматически.
				 *
				 * @return ещё не отправленные исходящие байты (zero-copy view во внутренний буфер)
				 *
				 */
				string_view pending() const noexcept;
				/**
				 * @brief Метод освобождения отправленных байтов из исходящего буфера (амортизированно O(1))
				 *
				 * @param size число отправленных байт
				 *
				 */
				void consumePending(const size_t size) noexcept;
			public:
				/**
				 * @brief Метод установки функции обратного вызова для обработки анонса server push
				 *
				 * @param callback функция обратного вызова для обработки анонса server push
				 *
				 */
				void on(push_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки фрагмента тела потока
				 *
				 * @param callback функция обратного вызова для обработки фрагмента тела потока
				 *
				 */
				void on(data_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки закрытия потока
				 *
				 * @param callback функция обратного вызова для обработки закрытия потока
				 *
				 */
				void on(close_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки ошибки уровня соединения
				 *
				 * @param callback функция обратного вызова для обработки ошибки уровня соединения
				 *
				 */
				void on(error_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова записи исходящих байтов в сеть
				 *
				 * @param callback функция обратного вызова записи исходящих байтов в сеть
				 *
				 */
				void on(write_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки открытия нового потока
				 *
				 * @param callback функция обратного вызова для обработки открытия нового потока
				 *
				 */
				void on(begin_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки фазы приёма сообщения потока
				 *
				 * @param callback функция обратного вызова для обработки фазы приёма сообщения потока
				 *
				 */
				void on(phase_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки полученного GOAWAY
				 *
				 * @param callback функция обратного вызова для обработки полученного GOAWAY
				 *
				 */
				void on(goaway_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки полученного ALTSVC
				 *
				 * @param callback функция обратного вызова
				 *
				 */
				void on(altsvc_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки полученного ORIGIN
				 *
				 * @param callback функция обратного вызова
				 *
				 */
				void on(origin_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки заголовков или трейлеров потока
				 *
				 * @param callback функция обратного вызова для обработки заголовков или трейлеров потока
				 *
				 */
				void on(header_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова о готовности потока принимать данные тела
				 *
				 * @param callback функция обратного вызова о готовности потока принимать данные тела
				 *
				 */
				void on(writable_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки применённого SETTINGS пира
				 *
				 * @param callback функция обратного вызова для обработки применённого SETTINGS пира
				 *
				 */
				void on(settings_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки провайдера заголовков потока
				 *
				 * @param callback функция обратного вызова для обработки провайдера заголовков потока
				 *
				 */
				void on(provider_callback_t callback) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param direct направление трафика (REQUEST - мы сервер, RESPONSE - мы клиент)
				 * @param fmk    объект фреймворка
				 * @param log    объект для работы с логами
				 *
				 */
				explicit Parser_HTTP2(const direct_t direct, const fmk_t * fmk, const log_t * log) noexcept;
			public:
				/**
				 * @brief Деструктор
				 *
				 */
				~Parser_HTTP2() noexcept override;
		} parser_http2_t;
	};
};

#endif // __AWH_HTTP_PARSER_HTTP2__
