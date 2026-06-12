/**
 * @file session.hpp
 * @brief Сессия HTTP/2 — конечный автомат соединения (RFC 9113 §5).
 *
 * Сессия владеет состоянием соединения: HPACK-кодером/декодером, картой потоков,
 * окнами flow control и согласованными SETTINGS. Модель работы повторяет проверенную
 * схему nghttp2, но реализована на C++17 (std-контейнеры, RAII, без C-инфраструктуры):
 *
 *   1. входящие байты скармливаются в feed();
 *   2. сессия разбирает фреймы и дёргает callbacks (function-pointer, zero-overhead);
 *   3. исходящие фреймы накапливаются во внутреннем буфере (pending()).
 *
 * ВАЖНО: это скелет (этапы 3–6 из README.md ещё не реализованы). Реализован разбор
 * preface и базовая диспетчеризация фреймов с обвязкой HPACK; полный конечный автомат
 * состояний потоков, flow control и защиты от DoS помечены TODO.
 */

#ifndef AWH_EXPERIENCE_H2_SESSION_HPP
#define AWH_EXPERIENCE_H2_SESSION_HPP

#include "h2.hpp"
#include "frame.hpp"
#include "hpack.hpp"

#include <string>
#include <cstdint>
#include <unordered_map>

namespace awh {
	namespace http2 {
		/**
		 * @brief Token-bucket для ограничения частоты событий (защита от flood).
		 *
		 * Повторяет модель nghttp2_ratelim: целочисленные «токены», пополнение rate
		 * токенов в секунду до предела burst, таймштамп в секундах. Время задаётся
		 * извне через Session::updateTime(); без обновления времени работает только
		 * стартовый запас burst (этого достаточно, чтобы погасить мгновенный всплеск).
		 */
		struct ratelim_t {
			uint64_t val    = 0; // текущее число токенов
			uint64_t burst  = 0; // максимум токенов
			uint64_t rate   = 0; // пополнение токенов в секунду
			uint64_t tstamp = 0; // последний момент обновления (секунды)

			void init(uint64_t b, uint64_t r) noexcept { val = burst = b; rate = r; tstamp = 0; }
			void update(uint64_t ts) noexcept {
				if(ts <= tstamp) return;
				const uint64_t d = ts - tstamp;
				tstamp = ts;
				const uint64_t gain = rate * d;
				val += gain;
				if(val > burst) val = burst;
			}
			/// Списать n токенов; false — токенов не хватает (превышение лимита).
			bool drain(uint64_t n) noexcept { if(val < n) return false; val -= n; return true; }
		};

		/**
		 * @brief Согласованные параметры SETTINGS (значения по умолчанию из RFC 9113).
		 */
		struct settings_t {
			uint32_t headerTableSize      = proto::DEFAULT_HEADER_TABLE_SIZE; // 4096
			uint32_t enablePush           = 1;
			uint32_t maxConcurrentStreams = 0xFFFFFFFF;                       // без лимита
			int32_t  initialWindowSize    = proto::DEFAULT_WINDOW_SIZE;       // 65535
			uint32_t maxFrameSize         = proto::DEFAULT_MAX_FRAME_SIZE;    // 16384
			uint32_t maxHeaderListSize    = 0;                               // 0 = без лимита
		};

		/**
		 * @brief Pull-провайдер тела (опционально, для больших тел без копии в sendBuffer).
		 *
		 * Заполняет buf (не более cap байт), выставляет *eof = true по достижении конца тела.
		 * Возвращает число записанных байт, либо -1 при ошибке (поток будет сброшен).
		 */
		using data_provider_t = int64_t (*)(void * user, uint32_t streamId, uint8_t * buf, size_t cap, bool * eof);

		/**
		 * @brief Состояние одного потока.
		 */
		struct stream_t {
			uint32_t       id            = 0;
			stream_state_t state         = stream_state_t::IDLE;
			int32_t        localWindow   = proto::DEFAULT_WINDOW_SIZE;  // сколько ещё можем принять
			int32_t        remoteWindow  = proto::DEFAULT_WINDOW_SIZE;  // сколько ещё можем отправить
			bool           headersDone   = false;                       // получен END_HEADERS

			// --- очередь отправки тела (flow control, этап 4) ---
			std::string     sendBuffer;                 // ещё не нарезанные в DATA байты (ограничен high-water)
			bool            endStreamPending = false;   // на последнем фрагменте выставить END_STREAM
			bool            endStreamSent    = false;    // END_STREAM уже отправлен
			bool            writableNotified = false;    // onStreamWritable уже вызван для текущего «провала»
			data_provider_t provider     = nullptr;     // pull-источник (если задан вместо submitData)
			void *          providerUser  = nullptr;
			bool            providerEof   = false;
		};

		/**
		 * @brief Набор callbacks. Любой указатель может быть nullptr.
		 *
		 * Указатели в data/name/value действительны ТОЛЬКО на время вызова (zero-copy).
		 */
		struct callbacks_t {
			/// Открыт новый поток (получен первый HEADERS).
			void (* onStreamBegin)(void * user, uint32_t streamId) = nullptr;
			/// Очередной декодированный заголовок потока.
			void (* onHeader)(void * user, uint32_t streamId, std::string_view name, std::string_view value) = nullptr;
			/// Блок заголовков потока завершён (получен END_HEADERS).
			void (* onHeadersComplete)(void * user, uint32_t streamId, bool endStream) = nullptr;
			/// Фрагмент тела (zero-copy во входной буфер).
			void (* onData)(void * user, uint32_t streamId, const uint8_t * data, size_t len, bool endStream) = nullptr;
			/// Поток закрыт (штатно или по RST_STREAM с кодом code).
			void (* onStreamClose)(void * user, uint32_t streamId, error_t code) = nullptr;
			/// (Клиент) получен PUSH_PROMISE: сервер обещает ответ в потоке promisedStreamId.
			/// Заголовки обещанного запроса придут через onHeader(promisedStreamId, …).
			/// Чтобы отказаться от push, вызовите submitRstStream(promisedStreamId, CANCEL).
			void (* onPushPromise)(void * user, uint32_t associatedStreamId, uint32_t promisedStreamId) = nullptr;
			/// Буфер отправки потока опустился ниже low-water — можно слать ещё (см. submitData).
			void (* onStreamWritable)(void * user, uint32_t streamId) = nullptr;
			/// Получен и применён SETTINGS пира.
			void (* onSettings)(void * user) = nullptr;
			/// Получен GOAWAY.
			void (* onGoaway)(void * user, uint32_t lastStreamId, error_t code) = nullptr;
			/// Ошибка уровня соединения (после этого соединение нужно закрыть).
			void (* onError)(void * user, error_t code, const char * message) = nullptr;
		};

		/**
		 * @brief Сессия HTTP/2.
		 */
		class Session {
			public:
				/**
				 * @brief Конструктор.
				 *
				 * @param endpoint  роль локального эндпоинта (клиент/сервер)
				 * @param callbacks набор обработчиков событий
				 * @param user      произвольный пользовательский контекст для callbacks
				 */
				Session(endpoint_t endpoint, const callbacks_t & callbacks, void * user) noexcept;

				/**
				 * @brief Подготовить исходящий preface: (клиент) magic + SETTINGS,
				 *        (сервер) только SETTINGS. Результат — в pending().
				 */
				void submitPreface() noexcept;

				/**
				 * @brief Поставить в очередь свой SETTINGS-фрейм.
				 */
				void submitSettings(const settings_t & settings) noexcept;

				/**
				 * @brief Поставить в очередь WINDOW_UPDATE.
				 */
				void submitWindowUpdate(uint32_t streamId, uint32_t increment) noexcept;

				/**
				 * @brief Поставить в очередь RST_STREAM.
				 */
				void submitRstStream(uint32_t streamId, error_t code) noexcept;

				/**
				 * @brief Поставить в очередь GOAWAY и пометить соединение завершаемым.
				 */
				void submitGoaway(error_t code, std::string_view debug = {}) noexcept;

				/**
				 * @brief Поставить в очередь блок заголовков (запрос/ответ) для потока.
				 *
				 * Если поток ещё не существует и это запрос клиента — поток открывается.
				 * При endStream поток сразу полузакрывается с нашей стороны (тело отсутствует).
				 * TODO(CONTINUATION): разбивать блок, превышающий MAX_FRAME_SIZE.
				 */
				void submitHeaders(uint32_t streamId, const std::vector <hpack::field_t> & fields, bool endStream) noexcept;

				/**
				 * @brief (Сервер) Анонсировать server push: отправить PUSH_PROMISE на потоке клиента.
				 *
				 * @param associatedStreamId поток клиента, в ответ на который выполняется push
				 *        (должен быть в состоянии open / half-closed(remote))
				 * @param request заголовки обещанного запроса (псевдо-заголовки :method/:scheme/
				 *        :path/:authority, как будто этот запрос прислал клиент)
				 * @return id зарезервированного (чётного) push-потока, либо 0, если push невозможен
				 *         (мы не сервер / клиент запретил push / некорректный associatedStreamId).
				 *
				 * Дальше сервер шлёт ответ обычным путём: submitHeaders(promisedId, responseHeaders)
				 * + submitData(promisedId, …). Поток автоматически переходит reserved(local) →
				 * half-closed(remote) при отправке заголовков ответа.
				 */
				uint32_t submitPushPromise(uint32_t associatedStreamId, const std::vector <hpack::field_t> & request) noexcept;

				/**
				 * @brief Передать сессии часть тела для отправки (push-модель, bounded buffer).
				 *
				 * Копирует во внутренний буфер потока столько байт, сколько влезает до high-water,
				 * и возвращает это число (0..len). Если вернулось меньше len — буфер заполнен:
				 * приостановите выдачу и дождитесь callback onStreamWritable. Нарезку во фреймы,
				 * учёт окон и автоматическую дослыку по WINDOW_UPDATE сессия делает сама.
				 *
				 * @return число принятых байт (0..len)
				 */
				size_t submitData(uint32_t streamId, const void * data, size_t len, bool endStream) noexcept;

				/**
				 * @brief Назначить pull-провайдер тела (zero-extra-copy для больших тел/файлов).
				 *
				 * Альтернатива submitData: сессия сама запрашивает у провайдера данные ровно тогда,
				 * когда открыто окно и есть место в выходном буфере.
				 */
				void setDataProvider(uint32_t streamId, data_provider_t provider, void * user) noexcept;

				/**
				 * @brief Настроить пороги буфера отправки потока (high/low water).
				 */
				void setSendWaterMarks(size_t high, size_t low) noexcept { _streamSendHighWater = high; _streamSendLowWater = low; }

				/**
				 * @brief Настроить порог выходного буфера соединения (backpressure от TCP-стадии).
				 */
				void setOutputHighWater(size_t high) noexcept { _outputHighWater = high; }

				/**
				 * @brief Сообщить текущее монотонное время (в секундах) для пополнения rate-лимитов.
				 *        Вызывайте периодически (например, перед feed); необязательно.
				 */
				void updateTime(uint64_t seconds) noexcept { _now = seconds; }

				/**
				 * @brief Лимит частоты входящих RST_STREAM (защита от Rapid Reset, CVE-2023-44487).
				 */
				void setResetRateLimit(uint64_t burst, uint64_t rate) noexcept { _rstLimit.init(burst, rate); }

				/**
				 * @brief Лимит частоты «дешёвых» управляющих фреймов (SETTINGS/PING/пустые DATA).
				 */
				void setControlRateLimit(uint64_t burst, uint64_t rate) noexcept { _ctrlLimit.init(burst, rate); }

				/**
				 * @brief Лимиты сборки блока заголовков (защита от CONTINUATION flood, 2024).
				 *
				 * @param maxBytes  максимальный суммарный размер блока (HEADERS + все CONTINUATION)
				 * @param maxFrames максимальное число фреймов в одном блоке
				 */
				void setHeaderBlockLimits(size_t maxBytes, uint32_t maxFrames) noexcept { _maxHeaderBlockSize = maxBytes; _maxContinuationFrames = maxFrames; }

				/**
				 * @brief Скормить сессии очередную порцию входящих байтов.
				 *
				 * Необработанный хвост (неполный фрейм) буферизуется внутри до следующего
				 * вызова. По ходу разбора вызываются callbacks.
				 *
				 * @param data входной буфер (может быть nullptr при size == 0)
				 * @param size доступно байт
				 * @return OK / ERROR (на ошибке вызван onError и поставлен GOAWAY)
				 */
				status_t feed(const uint8_t * data, size_t size) noexcept;

				/**
				 * @brief Буфер исходящих байтов, накопленных методами submit/feed (для отправки в сокет).
				 *        После отправки очистите его методом consumePending().
				 */
				const std::string & pending() const noexcept { return _output; }

				/**
				 * @brief Удалить из исходящего буфера n отправленных байт.
				 */
				void consumePending(size_t n) noexcept;

				/// Признак того, что соединение помечено на завершение (отправлен/получен GOAWAY).
				bool closed() const noexcept { return _goawaySent || _goawayReceived; }
			private:
				/// Обработать один полный фрейм (заголовок + payload длиной h.length).
				status_t dispatch(const frame::header_t & h, const uint8_t * payload) noexcept;
				/// Декодировать накопленный блок заголовков (_hbcBuffer) и вызвать callbacks.
				status_t deliverHeaders() noexcept;
				/// Доставить декодированный блок обещанного запроса (PUSH_PROMISE, сторона клиента).
				status_t deliverPushPromise(uint32_t associatedStreamId, uint32_t promisedStreamId, std::vector <hpack::field_t> & fields) noexcept;
				/// Завершить аварийно: вызвать onError, отправить GOAWAY.
				status_t fail(error_t code, const char * message) noexcept;
				/// Получить/создать поток.
				stream_t & stream(uint32_t id) noexcept;
				/// Найти поток или nullptr (без создания).
				stream_t * findStream(uint32_t id) noexcept;
				/// Проверить корректность нового потока, открываемого пиром (чётность + монотонность id).
				status_t validateNewStream(uint32_t id, error_t & err) noexcept;
				/// Применить полученный END_STREAM: перевести состояние и при закрытии вызвать onStreamClose.
				void applyRemoteEndStream(stream_t & s) noexcept;
				/// Применить отправленный нами END_STREAM: перевести состояние (возможно закрыть поток).
				void applyLocalEndStream(stream_t & s) noexcept;
				/// Закрыть поток и вызвать onStreamClose.
				void closeStream(uint32_t id, error_t code) noexcept;
				/// Прокачать отправку по всем потокам с учётом окон и порога выходного буфера.
				void pump() noexcept;
				/// Прокачать отправку одного потока.
				void pumpStream(stream_t & s) noexcept;
				/// Дозагрузить sendBuffer из pull-провайдера (если он задан).
				void refillFromProvider(stream_t & s) noexcept;
				/// При просадке буфера ниже low-water — вызвать onStreamWritable (один раз на «провал»).
				void maybeNotifyWritable(stream_t & s) noexcept;
				/// Все данные потока для отправки уже получены (нет провайдера или достигнут eof).
				bool providerDone(const stream_t & s) const noexcept;
				/// Пополнить окно приёма (соединения/потока) и отправить WINDOW_UPDATE при просадке.
				void replenishReceiveWindow(stream_t * s, uint32_t consumed) noexcept;
			private:
				endpoint_t  _endpoint;
				callbacks_t _cb;
				void *      _user = nullptr;

				settings_t _local;  // наши настройки
				settings_t _remote; // настройки пира

				hpack::Encoder _encoder;
				hpack::Decoder _decoder;

				std::unordered_map <uint32_t, stream_t> _streams;

				int32_t _localWindow  = proto::DEFAULT_WINDOW_SIZE; // окно соединения (приём)
				int32_t _remoteWindow = proto::DEFAULT_WINDOW_SIZE; // окно соединения (отправка)

				uint32_t _lastStreamId   = 0;     // наибольший принятый stream id (для GOAWAY)
				uint32_t _nextStreamId   = 1;     // следующий инициируемый нами stream id

				bool _prefaceReceived = false;    // (сервер) получен клиентский preface
				bool _settingsAcked   = false;    // получен ACK на наш SETTINGS
				bool _goawaySent      = false;
				bool _goawayReceived  = false;

				// Сборка блока заголовков из HEADERS + CONTINUATION.
				uint32_t    _hbcStream = 0;       // 0 = сборка не идёт
				bool        _hbcEndStream = false;
			std::string _hbcBuffer;           // накопленный фрагмент блока заголовков
			uint32_t    _hbcFrames = 0;       // число фреймов в текущем блоке (HEADERS + CONTINUATION)
			uint32_t    _hbcPromised = 0;     // != 0: собираемый блок принадлежит PUSH_PROMISE (id обещанного потока)

				std::string _input;               // буфер неразобранного хвоста
				std::string _output;              // буфер исходящих байтов

				// --- flow control / backpressure (этап 4) ---
				size_t _streamSendHighWater = 256 * 1024;  // ёмкость буфера отправки потока
				size_t _streamSendLowWater  =  64 * 1024;  // порог сигнала onStreamWritable
				size_t _outputHighWater     = 1024 * 1024; // порог выходного буфера (backpressure TCP)
				bool   _inPump = false;                    // защита от реентерабельного pump()

				// --- защиты от DoS (этап 5) ---
				uint64_t  _now = 0;                          // текущее время в секундах (для rate-лимитов)
				ratelim_t _rstLimit;                         // против Rapid Reset (RST_STREAM flood)
				ratelim_t _ctrlLimit;                        // против flood SETTINGS/PING/пустых DATA
				size_t    _maxHeaderBlockSize     = 64 * 1024; // лимит размера блока заголовков
				uint32_t  _maxContinuationFrames  = 64;        // лимит числа фреймов в блоке заголовков
		};
	}
}

#endif // AWH_EXPERIENCE_H2_SESSION_HPP
