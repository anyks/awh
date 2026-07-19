/**
 * @file: http.hpp
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
 * Подключаем наши заголовочные файлы
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
		 */
		typedef class __AWH_SHARED_EXPORT__ Parser_HTTP2 : public parser_t {
			public:
				/**
				 * @brief Максимальный суммарный размер блока заголовков (HEADERS + все CONTINUATION)
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 */
				static constexpr size_t MAX_HEADER_BLOCK_SIZE = (64 * 1024);
				/**
				 * @brief Максимальное число фреймов в одном блоке заголовков
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 */
				static constexpr uint32_t MAX_CONTINUATION_FRAMES = (64);
				/**
				 * @brief Стартовый запас лимита частоты входящих RST_STREAM (защита от Rapid Reset)
				 *
				 * @note Значения по умолчанию соответствуют nghttp2
				 */
				static constexpr uint64_t RST_LIMIT_BURST = (1000);
				/**
				 * @brief Пополнение лимита частоты входящих RST_STREAM (токенов в секунду)
				 *
				 * @note Значения по умолчанию соответствуют nghttp2
				 */
				static constexpr uint64_t RST_LIMIT_RATE = (33);
				/**
				 * @brief Стартовый запас лимита частоты управляющих фреймов (SETTINGS/PING/пустые DATA)
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 */
				static constexpr uint64_t CTRL_LIMIT_BURST = (1000);
				/**
				 * @brief Пополнение лимита частоты управляющих фреймов (токенов в секунду)
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 */
				static constexpr uint64_t CTRL_LIMIT_RATE = (100);
				/**
				 * @brief Ёмкость буфера отправки одного потока (high-water)
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 */
				static constexpr size_t SEND_HIGH_WATER = (256 * 1024);
				/**
				 * @brief Порог сигнала о готовности потока принимать данные (low-water)
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 */
				static constexpr size_t SEND_LOW_WATER = (64 * 1024);
				/**
				 * @brief Порог выходного буфера соединения (backpressure от TCP-стадии)
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 */
				static constexpr size_t OUTPUT_HIGH_WATER = (1024 * 1024);
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
				 */
				typedef struct Limits : parser_t::limits_t {
					// Максимальный суммарный размер блока заголовков (HEADERS + все CONTINUATION)
					size_t maxHeaderBlockSize;
					// Максимальное число фреймов в одном блоке заголовков
					uint32_t maxContinuationFrames;
					// Стартовый запас лимита частоты входящих RST_STREAM
					uint64_t rstLimitBurst;
					// Пополнение лимита частоты входящих RST_STREAM (токенов в секунду)
					uint64_t rstLimitRate;
					// Стартовый запас лимита частоты управляющих фреймов
					uint64_t ctrlLimitBurst;
					// Пополнение лимита частоты управляющих фреймов (токенов в секунду)
					uint64_t ctrlLimitRate;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Limits() noexcept :
					 parser_t::limits_t(),
					 maxHeaderBlockSize(MAX_HEADER_BLOCK_SIZE),
					 maxContinuationFrames(MAX_CONTINUATION_FRAMES),
					 rstLimitBurst(RST_LIMIT_BURST), rstLimitRate(RST_LIMIT_RATE),
					 ctrlLimitBurst(CTRL_LIMIT_BURST), ctrlLimitRate(CTRL_LIMIT_RATE) {}
				} limits_t;
				/**
				 * @brief Структура согласованных параметров SETTINGS (RFC 9113 §6.5.2)
				 *
				 * @details Значения по умолчанию из RFC 9113 (кроме maxConcurrentStreams -
				 *          безопасный дефолт вместо "без лимита")
				 */
				typedef struct Settings {
					// Размер динамической таблицы HPACK
					uint32_t headerTableSize;
					// Разрешён ли server push (0/1)
					uint32_t enablePush;
					// Лимит одновременных потоков
					uint32_t maxConcurrentStreams;
					// Начальное окно потока
					int32_t initialWindowSize;
					// Максимальный размер фрейма
					uint32_t maxFrameSize;
					// Лимит размера списка заголовков (0 - без лимита в SETTINGS, действует maxHeadersTotal)
					uint32_t maxHeaderListSize;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Settings() noexcept :
					 headerTableSize(h2::proto::DEFAULT_HEADER_TABLE_SIZE),
					 enablePush(1), maxConcurrentStreams(128),
					 initialWindowSize(h2::proto::DEFAULT_WINDOW_SIZE),
					 maxFrameSize(h2::proto::DEFAULT_MAX_FRAME_SIZE),
					 maxHeaderListSize(0) {}
				} settings_t;
			public:
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
				 */
				using write_callback_t = function <void (const void *, const size_t)>;
				/**
				 * @brief Тип функции обратного вызова для обработки открытия нового потока
				 *
				 * @details Возврат false сбрасывает поток (RST_STREAM с кодом CANCEL)
				 *
				 * @param sid идентификатор потока
				 * @return    результат обработки (false - поток сбрасывается)
				 */
				using begin_callback_t = function <bool (const uint32_t)>;
				/**
				 * @brief Тип функции обратного вызова для обработки заголовков или трейлеров потока
				 *
				 * @details Указатели name/value действительны ТОЛЬКО на время вызова.
				 *          Передаются все заголовки, включая псевдо-заголовки (:method/:path/
				 *          :status и др.); наиболее употребимые псевдо-заголовки дополнительно
				 *          собираются в провайдер заголовков потока.
				 *          Возврат false сбрасывает поток (RST_STREAM с кодом CANCEL)
				 *
				 * @param sid   идентификатор потока
				 * @param name  название заголовка
				 * @param value значение заголовка
				 * @param part  часть сообщения (HEADERS или TRAILER)
				 * @return      результат обработки (false - поток сбрасывается)
				 */
				using header_callback_t = function <bool (const uint32_t, const string_view, const string_view, const part_t)>;
				/**
				 * @brief Тип функции обратного вызова для обработки провайдера заголовков потока
				 *
				 * @details Вызывается по завершению блока заголовков (получен END_HEADERS).
				 *          Провайдер собран из псевдо-заголовков блока: для направления REQUEST
				 *          это request_t (:method/:path), для RESPONSE - response_t (:status).
				 *          Для трейлеров провайдер передаётся как nullptr.
				 *          Возврат false сбрасывает поток (RST_STREAM с кодом CANCEL)
				 *
				 * @param sid       идентификатор потока
				 * @param provider  провайдер заголовков потока (nullptr для трейлеров)
				 * @param endStream флаг завершения потока (тела не будет)
				 * @return          результат обработки (false - поток сбрасывается)
				 */
				using provider_callback_t = function <bool (const uint32_t, const provider_t *, const bool)>;
				/**
				 * @brief Тип функции обратного вызова для обработки фрагмента тела потока
				 *
				 * @details Указатель buffer действителен ТОЛЬКО на время вызова (zero-copy).
				 *          Возврат false сбрасывает поток (RST_STREAM с кодом CANCEL)
				 *
				 * @param sid       идентификатор потока
				 * @param buffer    буфер данных тела
				 * @param size      размер данных тела
				 * @param endStream флаг завершения потока
				 * @return          результат обработки (false - поток сбрасывается)
				 */
				using data_callback_t = function <bool (const uint32_t, const void *, const size_t, const bool)>;
				/**
				 * @brief Тип функции обратного вызова для обработки закрытия потока
				 *
				 * @param sid  идентификатор потока
				 * @param code код ошибки закрытия (NO_ERROR - штатное закрытие)
				 */
				using close_callback_t = function <void (const uint32_t, const error_t)>;
				/**
				 * @brief Тип функции обратного вызова для обработки анонса server push (только клиент)
				 *
				 * @details Заголовки обещанного запроса придут через header_callback_t /
				 *          provider_callback_t с идентификатором обещанного потока.
				 *          Возврат false отклоняет push (RST_STREAM с кодом CANCEL)
				 *
				 * @param sid         идентификатор ассоциированного потока клиента
				 * @param promisedSid идентификатор обещанного потока
				 * @return            результат обработки (false - push отклоняется)
				 */
				using push_callback_t = function <bool (const uint32_t, const uint32_t)>;
				/**
				 * @brief Тип функции обратного вызова о готовности потока принимать данные тела
				 *
				 * @details Вызывается когда буфер отправки потока опустился ниже low-water
				 *          (после частичного приёма в sendData можно отправлять дальше)
				 *
				 * @param sid идентификатор потока
				 */
				using writable_callback_t = function <void (const uint32_t)>;
				/**
				 * @brief Тип функции обратного вызова для обработки применённого SETTINGS пира
				 *
				 */
				using settings_callback_t = function <void (void)>;
				/**
				 * @brief Тип функции обратного вызова для обработки полученного GOAWAY
				 *
				 * @details Указатель debug действителен ТОЛЬКО на время вызова
				 *
				 * @param sid   наибольший идентификатор обработанного пиром потока
				 * @param code  код ошибки завершения соединения
				 * @param debug необязательные отладочные данные пира
				 */
				using goaway_callback_t = function <void (const uint32_t, const error_t, const string_view)>;
				/**
				 * @brief Тип функции обратного вызова для обработки ошибки уровня соединения
				 *
				 * @details После этого события соединение необходимо закрыть
				 *          (GOAWAY уже поставлен в очередь отправки)
				 *
				 * @param code    код ошибки протокола
				 * @param message текстовое описание ошибки
				 */
				using error_callback_t = function <void (const error_t, const string_view)>;
				/**
				 * @brief Тип pull-источника данных тела потока (для больших тел без лишней копии)
				 *
				 * @details Альтернатива sendData: парсер сам запрашивает у источника данные
				 *          ровно тогда, когда открыто окно и есть место в выходном буфере.
				 *          Источник заполняет буфер (не более cap байт), выставляет eof = true
				 *          по достижении конца тела и возвращает число записанных байт,
				 *          либо -1 при ошибке (поток будет сброшен)
				 *
				 * @param sid    идентификатор потока
				 * @param buffer буфер для заполнения
				 * @param cap    ёмкость буфера
				 * @param eof    флаг достижения конца тела
				 * @return       число записанных байт либо -1 при ошибке
				 */
				using data_source_callback_t = function <int64_t (const uint32_t, uint8_t *, const size_t, bool &)>;
			private:
				/**
				 * @brief Структура token-bucket для ограничения частоты событий (защита от flood)
				 *
				 * @details Повторяет модель nghttp2_ratelim: целочисленные токены, пополнение
				 *          rate токенов в секунду до предела burst. Время задаётся извне через
				 *          updateTime(); без обновления времени работает только стартовый запас
				 *          burst (этого достаточно, чтобы погасить мгновенный всплеск)
				 */
				typedef struct Ratelim {
					// Текущее число токенов
					uint64_t val;
					// Максимум токенов
					uint64_t burst;
					// Пополнение токенов в секунду
					uint64_t rate;
					// Последний момент обновления (секунды)
					uint64_t tstamp;
					/**
					 * @brief Метод инициализации лимита
					 *
					 * @param b стартовый запас токенов
					 * @param r пополнение токенов в секунду
					 */
					void init(const uint64_t b, const uint64_t r) noexcept;
					/**
					 * @brief Метод пополнения токенов по текущему времени
					 *
					 * @param ts текущее время (секунды)
					 */
					void update(const uint64_t ts) noexcept;
					/**
					 * @brief Метод списания токенов
					 *
					 * @param n число списываемых токенов
					 * @return  результат списания (false - токенов не хватает, превышение лимита)
					 */
					bool drain(const uint64_t n) noexcept;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Ratelim() noexcept :
					 val(0), burst(0), rate(0), tstamp(0) {}
				} ratelim_t;
				/**
				 * @brief Структура состояния одного потока (RFC 9113 §5.1)
				 *
				 */
				typedef struct Stream {
					// Идентификатор потока
					uint32_t id;
					// Состояние потока
					h2::stream_state_t state;
					// Окно приёма потока (сколько ещё можем принять)
					int32_t localWindow;
					// Окно отправки потока (сколько ещё можем отправить)
					int32_t remoteWindow;
					// Получен END_HEADERS (повторный HEADERS = трейлеры)
					bool headersDone;
					// Суммарный размер принятого тела потока (лимит maxBodySize)
					uint64_t recvBody;
					// Ещё не нарезанные в DATA байты тела (ограничен high-water)
					string sendBuffer;
					// Префикс sendBuffer, уже отправленный (вместо erase(0,..))
					size_t sendOffset;
					// На последнем фрагменте выставить END_STREAM
					bool endStreamPending;
					// END_STREAM уже отправлен
					bool endStreamSent;
					// Сигнал writable уже подан для текущего провала буфера
					bool writableNotified;
					// Достигнут конец тела pull-источника данных
					bool sourceEof;
					// Pull-источник данных тела (если задан вместо sendData)
					data_source_callback_t source;
					// Провайдер заголовков потока (собирается из псевдо-заголовков)
					unique_ptr <http::provider_t> headers;
					/**
					 * @brief Метод получения логического объёма ещё не отправленных данных тела
					 *
					 * @return объём не отправленных данных (без учтённого префикса)
					 */
					size_t pending() const noexcept;
					/**
					 * @brief Метод снятия учтённого префикса буфера отправки
					 *
					 * @details Очистка при полном расходе, иначе амортизированная компактификация
					 */
					void compactSendBuffer() noexcept;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Stream() noexcept :
					 id(0), state(h2::stream_state_t::IDLE),
					 localWindow(h2::proto::DEFAULT_WINDOW_SIZE),
					 remoteWindow(h2::proto::DEFAULT_WINDOW_SIZE),
					 headersDone(false), recvBody(0),
					 sendBuffer{""}, sendOffset(0),
					 endStreamPending(false), endStreamSent(false),
					 writableNotified(false), sourceEof(false),
					 source(nullptr), headers(nullptr) {}
				} stream_t;
				/**
				 * @brief Структура функций обратного вызова
				 *
				 */
				typedef struct Callbacks {
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
					 * @brief Функция обратного вызова для обработки заголовков или трейлеров потока
					 *
					 */
					header_callback_t header;
					/**
					 * @brief Функция обратного вызова для обработки провайдера заголовков потока
					 *
					 */
					provider_callback_t provider;
					/**
					 * @brief Функция обратного вызова для обработки фрагмента тела потока
					 *
					 */
					data_callback_t data;
					/**
					 * @brief Функция обратного вызова для обработки закрытия потока
					 *
					 */
					close_callback_t close;
					/**
					 * @brief Функция обратного вызова для обработки анонса server push
					 *
					 */
					push_callback_t push;
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
					 * @brief Функция обратного вызова для обработки полученного GOAWAY
					 *
					 */
					goaway_callback_t goaway;
					/**
					 * @brief Функция обратного вызова для обработки ошибки уровня соединения
					 *
					 */
					error_callback_t error;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Callbacks() noexcept :
					 write(nullptr), begin(nullptr), header(nullptr),
					 provider(nullptr), data(nullptr), close(nullptr),
					 push(nullptr), writable(nullptr), settings(nullptr),
					 goaway(nullptr), error(nullptr) {}
				} callbacks_t;
			private:
				// Код ошибки уровня соединения
				error_t _error;
			private:
				// Настраиваемые лимиты безопасности
				limits_t _limits;
			private:
				// Наши параметры SETTINGS
				settings_t _local;
				// Параметры SETTINGS пира
				settings_t _remote;
			private:
				// HPACK-кодер (наша динамическая таблица)
				h2::hpack::encoder_t _encoder;
				// HPACK-декодер (динамическая таблица пира)
				h2::hpack::decoder_t _decoder;
			private:
				// Карта активных потоков
				unordered_map <uint32_t, stream_t> _streams;
			private:
				// Окно приёма соединения (текущее)
				int32_t _localWindow;
				// Целевой размер окна приёма соединения
				int32_t _localWindowMax;
				// Окно отправки соединения
				int32_t _remoteWindow;
			private:
				// Переиспользуемый снимок идентификаторов потоков для pump() (без аллокаций на вызов)
				vector <uint32_t> _pumpIds;
			private:
				// Наибольший принятый идентификатор потока (для GOAWAY)
				uint32_t _lastStreamId;
				// Следующий инициируемый нами идентификатор потока
				uint32_t _nextStreamId;
			private:
				// Получен connection preface (для сервера; клиент отправляет его сам)
				bool _prefaceReceived;
				// Получен ACK на наш SETTINGS
				bool _settingsAcked;
				// Отправлен GOAWAY
				bool _goawaySent;
				// Получен GOAWAY
				bool _goawayReceived;
			private:
				// Идентификатор потока собираемого блока заголовков (0 - сборка не идёт)
				uint32_t _hbcStream;
				// Флаг END_STREAM собираемого блока заголовков
				bool _hbcEndStream;
				// Накопленный фрагмент блока заголовков (HEADERS + CONTINUATION)
				string _hbcBuffer;
				// Число фреймов в текущем блоке заголовков
				uint32_t _hbcFrames;
				// Идентификатор обещанного потока (!= 0: блок принадлежит PUSH_PROMISE)
				uint32_t _hbcPromised;
				// Поток отклонён (RST_STREAM), блок декодируем только для синхронизации HPACK
				bool _hbcRefused;
			private:
				// Число активных потоков, открытых пиром (лимит MAX_CONCURRENT_STREAMS)
				uint32_t _peerStreamCount;
			private:
				// Защита от реентерабельного parse() (callback -> parse)
				bool _inParse;
				// Защита от реентерабельного pump() (writable -> sendData -> pump)
				bool _inPump;
			private:
				// Буфер неразобранного хвоста входящих данных
				string _input;
				// Буфер исходящих байтов
				string _output;
				// Префикс буфера исходящих байтов, уже отданный в сокет (вместо erase(0,..))
				size_t _outputPos;
			private:
				// Ёмкость буфера отправки потока (high-water)
				size_t _sendHighWater;
				// Порог сигнала writable (low-water)
				size_t _sendLowWater;
				// Порог выходного буфера соединения (backpressure от TCP-стадии)
				size_t _outputHighWater;
			private:
				// Текущее время в секундах (для rate-лимитов)
				uint64_t _now;
				// Лимит частоты входящих RST_STREAM (против Rapid Reset)
				ratelim_t _rstLimit;
				// Лимит частоты управляющих фреймов (против flood SETTINGS/PING/пустых DATA)
				ratelim_t _ctrlLimit;
			private:
				// Объект функций обратного вызова
				callbacks_t _callbacks;
			private:
				/**
				 * @brief Метод получения логического объёма ещё не отправленных исходящих байтов
				 *
				 * @return объём не отправленных исходящих байтов
				 */
				size_t outputPending() const noexcept;
				/**
				 * @brief Метод передачи исходящих байтов сетевому слою через функцию обратного вызова записи
				 *
				 * @details Если функция записи не установлена - байты остаются во внутреннем
				 *          буфере до выборки через pending()/consumePending()
				 */
				void flush() noexcept;
			private:
				/**
				 * @brief Метод разбора накопленного входного буфера (preface + поток фреймов)
				 *
				 * @return результат разбора (OK/ERROR)
				 */
				h2::status_t parseInput() noexcept;
				/**
				 * @brief Метод обработки одного полного фрейма
				 *
				 * @param h       заголовок фрейма
				 * @param payload полезная нагрузка фрейма (ровно h.length байт)
				 * @return        результат обработки (OK/ERROR)
				 */
				h2::status_t dispatch(const h2::frame::header_t & h, const uint8_t * payload) noexcept;
				/**
				 * @brief Метод декодирования накопленного блока заголовков и вызова функций обратного вызова
				 *
				 * @return результат обработки (OK/ERROR)
				 */
				h2::status_t deliverHeaders() noexcept;
				/**
				 * @brief Метод доставки декодированного блока обещанного запроса (PUSH_PROMISE, сторона клиента)
				 *
				 * @param sid         идентификатор ассоциированного потока клиента
				 * @param promisedSid идентификатор обещанного потока
				 * @param fields      декодированные заголовки обещанного запроса
				 * @return            результат обработки (OK/ERROR)
				 */
				h2::status_t deliverPushPromise(const uint32_t sid, const uint32_t promisedSid, vector <h2::hpack::field_t> & fields) noexcept;
				/**
				 * @brief Метод аварийного завершения соединения (ошибка, GOAWAY, запись в лог)
				 *
				 * @param code    код ошибки протокола
				 * @param message текстовое описание ошибки
				 * @return        статус ошибки (для проброса из обработчиков)
				 */
				h2::status_t fail(const error_t code, const char * message) noexcept;
			private:
				/**
				 * @brief Метод получения существующего либо создания нового потока
				 *
				 * @param id идентификатор потока
				 * @return   объект потока
				 */
				stream_t & stream(const uint32_t id) noexcept;
				/**
				 * @brief Метод поиска потока без создания
				 *
				 * @param id идентификатор потока
				 * @return   объект потока либо nullptr
				 */
				stream_t * findStream(const uint32_t id) noexcept;
				/**
				 * @brief Метод проверки корректности нового потока, открываемого пиром (чётность + монотонность id)
				 *
				 * @param id  идентификатор потока
				 * @param err код ошибки протокола
				 * @return    результат проверки (OK/ERROR)
				 */
				h2::status_t validateNewStream(const uint32_t id, error_t & err) noexcept;
				/**
				 * @brief Метод применения полученного END_STREAM (переход состояния, возможно закрытие потока)
				 *
				 * @param s объект потока (ссылка может стать недействительной после вызова)
				 */
				void applyRemoteEndStream(stream_t & s) noexcept;
				/**
				 * @brief Метод применения отправленного нами END_STREAM (переход состояния, возможно закрытие потока)
				 *
				 * @param s объект потока (ссылка может стать недействительной после вызова)
				 */
				void applyLocalEndStream(stream_t & s) noexcept;
				/**
				 * @brief Метод закрытия потока с вызовом функции обратного вызова закрытия
				 *
				 * @param id   идентификатор потока
				 * @param code код ошибки закрытия
				 */
				void closeStream(const uint32_t id, const error_t code) noexcept;
				/**
				 * @brief Метод удаления потока из карты с корректным учётом счётчика встречных потоков
				 *
				 * @param id идентификатор потока
				 */
				void eraseStream(const uint32_t id) noexcept;
				/**
				 * @brief Метод проверки того, что поток инициирован пиром (а не нами)
				 *
				 * @param id идентификатор потока
				 * @return   результат проверки
				 */
				bool peerInitiated(const uint32_t id) const noexcept;
			private:
				/**
				 * @brief Метод прокачки отправки по всем потокам с учётом окон и порога выходного буфера
				 *
				 * @details Round-robin: за каждый проход отправляется не более одного DATA-фрейма
				 *          с потока, пока хоть один поток делает прогресс - исключает голодание
				 *          потоков (head-of-line blocking)
				 */
				void pump() noexcept;
				/**
				 * @brief Метод отправки не более одного DATA-фрейма потока
				 *
				 * @param s объект потока (ссылка может стать недействительной после вызова)
				 * @return  признак прогресса отправки
				 */
				bool pumpStream(stream_t & s) noexcept;
				/**
				 * @brief Метод дозагрузки буфера отправки из pull-источника данных (если он задан)
				 *
				 * @param s объект потока
				 */
				void refillFromSource(stream_t & s) noexcept;
				/**
				 * @brief Метод сигнализации о готовности потока принимать данные (один раз на провал буфера)
				 *
				 * @param s объект потока
				 */
				void maybeNotifyWritable(stream_t & s) noexcept;
				/**
				 * @brief Метод проверки того, что все данные потока для отправки уже получены
				 *
				 * @param s объект потока
				 * @return  результат проверки (нет источника данных или достигнут его eof)
				 */
				bool sourceDone(const stream_t & s) const noexcept;
				/**
				 * @brief Метод пополнения окна приёма (соединения/потока) с отправкой WINDOW_UPDATE при просадке
				 *
				 * @param s        объект потока (nullptr - только окно соединения)
				 * @param consumed число принятых байт
				 */
				void replenishReceiveWindow(stream_t * s, const uint32_t consumed) noexcept;
			private:
				/**
				 * @brief Метод построения провайдера заголовков потока из псевдо-заголовков
				 *
				 * @param fields  декодированные заголовки блока
				 * @param request собирается запрос клиента (true) или ответ сервера (false)
				 * @return        собранный провайдер заголовков
				 */
				unique_ptr <provider_t> buildProvider(const vector <h2::hpack::field_t> & fields, const bool request) const noexcept;
				/**
				 * @brief Метод отправки собранного HPACK-блока заголовков потока
				 *
				 * @details Общая часть перегрузок sendHeaders: нарезка блока на
				 *          HEADERS + CONTINUATION и переходы состояния потока
				 *
				 * @param sid       идентификатор потока
				 * @param block     закодированный HPACK-блок заголовков
				 * @param endStream флаг завершения потока (тела не будет)
				 */
				void commitHeaders(const uint32_t sid, const string & block, const bool endStream);
				/**
				 * @brief Метод проверки декодированных заголовков на лимиты безопасности
				 *
				 * @param fields декодированные заголовки блока
				 * @return       результат проверки (false - лимиты превышены)
				 */
				bool checkHeaderLimits(const vector <h2::hpack::field_t> & fields) const noexcept;
			public:
				/**
				 * @brief Метод полной очистки всех данных парсера
				 *
				 * @details Помимо полного сброса состояния соединения возвращает лимиты
				 *          безопасности и параметры SETTINGS к значениям по умолчанию
				 *          и удаляет установленные функции обратного вызова
				 */
				void clear() noexcept override;
				/**
				 * @brief Метод полного сброса состояния соединения
				 *
				 * @details В отличие от HTTP/1.x, где reset() готовит парсер к следующему
				 *          сообщению, для HTTP/2 сбрасывается ВСЁ соединение: HPACK-таблицы,
				 *          карта потоков, окна, буферы (семантика нового соединения).
				 *          Лимиты безопасности, параметры SETTINGS и функции обратного
				 *          вызова сохраняются
				 */
				void reset() noexcept override;
			public:
				/**
				 * @brief Метод клонирования объекта парсера
				 *
				 * @details Клон получает те же направление трафика, лимиты безопасности,
				 *          параметры SETTINGS и функции обратного вызова, но чистое
				 *          состояние соединения ("фабрика с теми же настройками")
				 *
				 * @return копия объекта парсера
				 */
				unique_ptr <parser_t> clone() const noexcept override;
			public:
				/**
				 * @brief Метод уведомления парсера о завершении потока данных (закрытии соединения)
				 *
				 * @details Если соединение завершено корректно (обменялись GOAWAY, активных
				 *          потоков нет) - фиксируется статус COMPLETE. Если соединение закрыто
				 *          посреди активных потоков или незавершённого фрейма - фиксируется
				 *          ошибка PROTOCOL_ERROR (обрыв соединения)
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
				 */
				size_t parse(const void * buffer, const size_t size) noexcept override;
			public:
				/**
				 * @brief Метод получения кода ошибки уровня соединения
				 *
				 * @return код ошибки протокола
				 */
				error_t error() const noexcept;
				/**
				 * @brief Метод получения человекочитаемого названия текущей ошибки разбора
				 *
				 * @return название текущей ошибки разбора
				 */
				string errorName() const noexcept override;
				/**
				 * @brief Метод получения человекочитаемого названия кода ошибки
				 *
				 * @param error код ошибки протокола
				 * @return      название кода ошибки
				 */
				static string errorName(const error_t error) noexcept;
			public:
				/**
				 * @brief Метод получения лимитов безопасности
				 *
				 * @return лимиты безопасности
				 */
				const limits_t & limits() const noexcept;
				/**
				 * @brief Метод установки лимитов безопасности
				 *
				 * @param limits лимиты безопасности
				 */
				void limits(const limits_t & limits) noexcept;
			public:
				/**
				 * @brief Метод получения наших параметров SETTINGS
				 *
				 * @return наши параметры SETTINGS
				 */
				const settings_t & settings() const noexcept;
				/**
				 * @brief Метод установки наших параметров SETTINGS
				 *
				 * @note Отправка выполняется методами sendPreface()/sendSettings()
				 *
				 * @param settings наши параметры SETTINGS
				 */
				void settings(const settings_t & settings) noexcept;
				/**
				 * @brief Метод получения параметров SETTINGS пира
				 *
				 * @return параметры SETTINGS пира
				 */
				const settings_t & remoteSettings() const noexcept;
			public:
				/**
				 * @brief Метод отправки исходящего preface соединения
				 *
				 * @details Клиент отправляет magic-строку + свой SETTINGS, сервер - только SETTINGS.
				 *          Обязан быть первым исходящим сообщением соединения
				 */
				void sendPreface() noexcept;
				/**
				 * @brief Метод отправки нашего SETTINGS-фрейма (текущие параметры)
				 *
				 */
				void sendSettings() noexcept;
				/**
				 * @brief Метод отправки блока заголовков (запрос/ответ/трейлеры) потока
				 *
				 * @details Если поток ещё не существует и мы инициатор - поток открывается.
				 *          При endStream поток сразу полузакрывается с нашей стороны (тела не будет).
				 *          Блок, превышающий SETTINGS_MAX_FRAME_SIZE пира, автоматически режется
				 *          на HEADERS + CONTINUATION (RFC 9113 §6.2/§6.10)
				 *
				 * @param sid       идентификатор потока
				 * @param fields    заголовки (псевдо-заголовки :method/:path/... должны идти первыми)
				 * @param endStream флаг завершения потока (тела не будет)
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
				 *          а также TE со значением кроме "trailers") пропускаются (RFC 9113 §8.2.2)
				 *
				 * @param sid       идентификатор потока
				 * @param headers   контейнер заголовков (провайдер контейнера задаёт псевдо-заголовки)
				 * @param endStream флаг завершения потока (тела не будет)
				 * @param scheme    схема запроса для псевдо-заголовка [:scheme] (для ответа не используется)
				 */
				void sendHeaders(const uint32_t sid, const headers_t & headers, const bool endStream, string_view scheme = "https") noexcept;
				/**
				 * @brief Метод передачи части тела потока для отправки (push-модель, bounded buffer)
				 *
				 * @details Копирует во внутренний буфер потока столько байт, сколько влезает до
				 *          high-water, и возвращает это число (0..size). Если вернулось меньше
				 *          size - буфер заполнен: приостановите выдачу и дождитесь функции
				 *          обратного вызова writable. Нарезку во фреймы, учёт окон и
				 *          автоматическую досылку по WINDOW_UPDATE парсер делает сам
				 *
				 * @param sid       идентификатор потока
				 * @param buffer    буфер данных тела
				 * @param size      размер данных тела
				 * @param endStream флаг завершения потока
				 * @return          число принятых байт (0..size)
				 */
				size_t sendData(const uint32_t sid, const void * buffer, const size_t size, const bool endStream) noexcept;
				/**
				 * @brief Метод анонса server push (только сервер)
				 *
				 * @details Отправляет PUSH_PROMISE на потоке клиента и резервирует чётный
				 *          push-поток. Дальше ответ отправляется обычным путём:
				 *          sendHeaders(promisedSid, ...) + sendData(promisedSid, ...)
				 *
				 * @param sid    идентификатор потока клиента, в ответ на который выполняется push
				 * @param fields заголовки обещанного запроса (псевдо-заголовки как у запроса клиента)
				 * @return       идентификатор зарезервированного push-потока либо 0, если push невозможен
				 */
				uint32_t sendPushPromise(const uint32_t sid, const vector <h2::hpack::field_t> & fields) noexcept;
				/**
				 * @brief Метод отправки RST_STREAM (аварийное закрытие потока)
				 *
				 * @param sid  идентификатор потока
				 * @param code код ошибки, с которым сбрасывается поток
				 */
				void sendRstStream(const uint32_t sid, const error_t code) noexcept;
				/**
				 * @brief Метод отправки GOAWAY (пометка соединения завершаемым)
				 *
				 * @param code  код ошибки завершения соединения
				 * @param debug необязательные отладочные данные
				 */
				void sendGoaway(const error_t code, string_view debug = {}) noexcept;
				/**
				 * @brief Метод отправки WINDOW_UPDATE
				 *
				 * @param sid       идентификатор потока (0 - окно всего соединения)
				 * @param increment инкремент окна flow control
				 */
				void sendWindowUpdate(const uint32_t sid, const uint32_t increment) noexcept;
			public:
				/**
				 * @brief Метод выделения идентификатора для нового инициируемого нами потока
				 *
				 * @details Клиент получает нечётные идентификаторы (1, 3, 5...), выделенный
				 *          идентификатор передаётся в sendHeaders() для открытия потока
				 *
				 * @return идентификатор нового потока
				 */
				uint32_t nextStreamId() noexcept;
				/**
				 * @brief Метод назначения pull-источника данных тела потока
				 *
				 * @param sid    идентификатор потока
				 * @param source pull-источник данных тела
				 */
				void dataSource(const uint32_t sid, data_source_callback_t source) noexcept;
			public:
				/**
				 * @brief Метод настройки порогов буфера отправки потока
				 *
				 * @param high ёмкость буфера отправки потока (high-water)
				 * @param low  порог сигнала writable (low-water)
				 */
				void sendWaterMarks(const size_t high, const size_t low) noexcept;
				/**
				 * @brief Метод настройки порога выходного буфера соединения (backpressure от TCP-стадии)
				 *
				 * @param high порог выходного буфера соединения
				 */
				void outputHighWater(const size_t high) noexcept;
				/**
				 * @brief Метод увеличения приёмного окна соединения
				 *
				 * @details По умолчанию окно соединения 65535 байт - узкое место при высокой
				 *          пропускной способности. Поднимает целевой размер окна приёма и сразу
				 *          отправляет WINDOW_UPDATE(0) на разницу. Только увеличение
				 *
				 * @param size новый целевой размер окна приёма соединения
				 */
				void connectionReceiveWindow(const int32_t size) noexcept;
				/**
				 * @brief Метод сообщения текущего монотонного времени для пополнения rate-лимитов
				 *
				 * @details Вызывайте периодически (например, перед parse); необязательно -
				 *          без обновления времени работает только стартовый запас burst
				 *
				 * @param seconds текущее монотонное время (секунды)
				 */
				void updateTime(const uint64_t seconds) noexcept;
			public:
				/**
				 * @brief Метод получения ещё не отправленных исходящих байтов (pull-модель)
				 *
				 * @details View действителен до следующего вызова любого метода парсера.
				 *          После записи в сокет освободите отправленную часть методом
				 *          consumePending(). При установленной функции обратного вызова
				 *          записи буфер опустошается автоматически
				 *
				 * @return ещё не отправленные исходящие байты (zero-copy view во внутренний буфер)
				 */
				string_view pending() const noexcept;
				/**
				 * @brief Метод освобождения отправленных байтов из исходящего буфера (амортизированно O(1))
				 *
				 * @param n число отправленных байт
				 */
				void consumePending(const size_t n) noexcept;
				/**
				 * @brief Метод проверки того, что соединение помечено на завершение
				 *
				 * @return признак завершения (отправлен или получен GOAWAY)
				 */
				bool isClosed() const noexcept;
			public:
				/**
				 * @brief Метод установки функции обратного вызова записи исходящих байтов в сеть
				 *
				 * @param callback функция обратного вызова записи исходящих байтов в сеть
				 */
				void on(write_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки открытия нового потока
				 *
				 * @param callback функция обратного вызова для обработки открытия нового потока
				 */
				void on(begin_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки заголовков или трейлеров потока
				 *
				 * @param callback функция обратного вызова для обработки заголовков или трейлеров потока
				 */
				void on(header_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки провайдера заголовков потока
				 *
				 * @param callback функция обратного вызова для обработки провайдера заголовков потока
				 */
				void on(provider_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки фрагмента тела потока
				 *
				 * @param callback функция обратного вызова для обработки фрагмента тела потока
				 */
				void on(data_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки закрытия потока
				 *
				 * @param callback функция обратного вызова для обработки закрытия потока
				 */
				void on(close_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки анонса server push
				 *
				 * @param callback функция обратного вызова для обработки анонса server push
				 */
				void on(push_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова о готовности потока принимать данные тела
				 *
				 * @param callback функция обратного вызова о готовности потока принимать данные тела
				 */
				void on(writable_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки применённого SETTINGS пира
				 *
				 * @param callback функция обратного вызова для обработки применённого SETTINGS пира
				 */
				void on(settings_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки полученного GOAWAY
				 *
				 * @param callback функция обратного вызова для обработки полученного GOAWAY
				 */
				void on(goaway_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки ошибки уровня соединения
				 *
				 * @param callback функция обратного вызова для обработки ошибки уровня соединения
				 */
				void on(error_callback_t callback) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param direct направление трафика (REQUEST - мы сервер, RESPONSE - мы клиент)
				 * @param fmk    объект фреймворка
				 * @param log    объект для работы с логами
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
