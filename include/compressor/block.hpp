/**
 * @file compressor.hpp
 * @date 2026-01-21
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
 * @brief Заголовочный файл модуля блочной (one-shot) компрессии — публичный API класса compressor::Block,
 *        выполняющего сжатие и распаковку данных целиком за один вызов методами GZip, Zlib, Deflate, Brotli, LZ4,
 *        Zstd, LZma, BZip2, Lzip и другими поддерживаемыми алгоритмами
 *
 * \~english
 * @brief Header file of the block (one-shot) compression module — the public API of the compressor::Block class
 *        performing compression and decompression of data in whole within a single call by the GZip, Zlib, Deflate,
 *        Brotli, LZ4, Zstd, LZma, BZip2, Lzip and other supported algorithms
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_COMPRESSOR_BLOCK__
#define __AWH_COMPRESSOR_BLOCK__

/**
 * Стандартные заголовочные файлы
 */
#include <atomic>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "types.hpp"
#include "stream.hpp"
#include "../sys/log.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён компрессора
	 *
	 * \~english
	 * @brief Compressor namespace
	 *
	 * \~
	 */
	namespace compressor {
		/**
		 * \~russian
		 * @brief Предварительное объявление непрозрачного контекста потока Deflate
		 *
		 * @details Полное определение скрыто в модуле реализации,
		 *          что позволяет не подключать заголовочные файлы стороннего компрессора (zlib) в публичный интерфейс.
		 *
		 * \~english
		 * @brief Forward declaration of the opaque Deflate stream context
		 *
		 * @details The full definition is hidden in the implementation module,
		 *          which makes it possible not to include the header files of the third-party compressor (zlib) in the public interface.
		 *
		 * \~
		 */
		struct deflate_stream_t;
		/**
		 * \~russian
		 * @brief Класс блочной (one-shot) компрессии/декомпрессии данных
		 *
		 * @details Намеренные решения:
		 *
		 *          1. Форматы блочного и потокового режимов совпадают не для всех движков.
		 *             Для LZ4 и Lizard блочный режим выдаёт «сырой» блок (LZ4_compress_fast /
		 *             Lizard_compress), а потоковый — кадр (LZ4F_* / LizardF_*), так как «сырой»
		 *             блок по устройству формата не самоописателен и не может быть разобран по
		 *             частям. Поэтому данные, сжатые Block, разбираются только Block, а данные,
		 *             сжатые Stream — только Stream. Для остальных движков (GZip, Zlib, Deflate,
		 *             Brotli, Zstandard, LZMA, BZip2) форматы обоих режимов совместимы.
		 *             У Deflate совместимость с оговоркой: блочное сообщение завершается
		 *             Z_SYNC_FLUSH и конца потока не несёт, поэтому потоковая сессия его
		 *             разбирает, но done() у неё остаётся ложным — движку конца потока
		 *             никто не показывал. Данные при этом целы, а сессия жива.
		 *
		 *          2. Пустой вход (nullptr либо нулевой размер) не является ошибкой — результатом
		 *             будет пустой буфер. Признаком ошибки служит пустой результат при непустом входе.
		 *
		 *          3. Размер скользящего окна принимается в промежутке 9…15, а не 8…15:
		 *             окно в восемь разрядов библиотека сжатия не заводит ни «сырым» потоком,
		 *             ни потоком с заголовком gzip, а у формата zlib заводит сжатие, но окно
		 *             молча поднимает до девяти — и разбор тем же значением поток уже не берёт.
		 *             Узел, требующий ровно восьми разрядов (RFC 7692 такое допускает),
		 *             обслужен быть не может: слою рукопожатия следует предлагать девять и выше.
		 *
		 *          4. Настройки движков семейства Zlib раздельны: у GZip, Zlib и Deflate по
		 *             своему размеру скользящего окна (wbitsGZip, wbitsZlib, wbitsDeflate), а
		 *             переиспользование контекста между сообщениями (takeoverDeflate) заведено
		 *             для одного лишь Deflate. Окно Deflate согласуется с узлом отдельно
		 *             (RFC 7692), и общее поле означало бы, что согласование с чужим узлом молча
		 *             меняет сжатие, к WebSocket не относящееся.
		 *
		 *          5. Единой политики обращения с хвостом за концом кадра нет: движок либо
		 *             отбрасывает посторонние октеты, либо считает их порчей и отвергает кадр.
		 *             Работа получает длину буфера от вызывающей стороны и сама границы кадра
		 *             не ищет, поэтому подавать следует ровно кадр. Гарантируется одно: работа
		 *             завершается, а выданные данные не искажены.
		 *
		 *          6. Метод DEFLATE завершает сообщение Z_SYNC_FLUSH, а не Z_FINISH, и потому
		 *             оставляет в конце результата четыре октета 00 00 FF FF. Так требует RFC 7692
		 *             (permessage-deflate): контекст переиспользуется между сообщениями, и Z_FINISH
		 *             закрыл бы поток. Снятие хвоста — дело вызывающей стороны, знающей, отдаёт ли
		 *             она сообщение в кадр WebSocket или хранит его целиком.
		 *
		 *          Полный перечень намеренных решений, реестр отклонённых находок и список
		 *          открытых вопросов — в src/compressor/README.md. Разбор модуля следует
		 *          начинать с него: там записано то, что уже вносилось в отчёты и проверялось.
		 *
		 * \~english
		 * @brief Data block (one-shot) compression/decompression class
		 *
		 * @details Deliberate decisions:
		 *
		 *          1. The formats of the block and streaming modes do not coincide for every engine.
		 *             For LZ4 and Lizard the block mode produces a "raw" block (LZ4_compress_fast /
		 *             Lizard_compress), whereas the streaming one produces a frame (LZ4F_* / LizardF_*),
		 *             since a "raw" block is by the design of the format not self-describing and cannot
		 *             be parsed piecewise. Therefore data compressed by Block is parsed only by Block, and
		 *             data compressed by Stream — only by Stream. For the remaining engines (GZip, Zlib,
		 *             Deflate, Brotli, Zstandard, LZMA, BZip2) the formats of both modes are compatible.
		 *             For Deflate the compatibility comes with a caveat: a block message is terminated with
		 *             Z_SYNC_FLUSH and carries no end of stream, therefore a streaming session parses it,
		 *             but its done() stays false — nobody showed the engine the end of the stream. The data
		 *             is intact meanwhile, and the session is alive.
		 *
		 *          2. Empty input (nullptr or zero size) is not an error — the result will be an empty
		 *             buffer. The sign of an error is an empty result on non-empty input.
		 *
		 *          3. The sliding window size is accepted within the 9…15 range rather than 8…15: a window
		 *             of eight bits is not set up by the compression library either for a "raw" stream or
		 *             for a stream with a gzip header, while for the zlib format it does set up compression
		 *             but silently raises the window to nine — and parsing with that same value no longer
		 *             accepts the stream. A peer demanding exactly eight bits (RFC 7692 permits such) cannot
		 *             be served: the handshake layer should offer nine and above.
		 *
		 *          4. The settings of the Zlib family engines are separate: GZip, Zlib and Deflate each have
		 *             their own sliding window size (wbitsGZip, wbitsZlib, wbitsDeflate), while context reuse
		 *             between messages (takeoverDeflate) is provided for Deflate alone. The Deflate window is
		 *             negotiated with the peer separately (RFC 7692), and a shared field would mean that
		 *             negotiation with a foreign peer silently changes compression unrelated to WebSocket.
		 *
		 *          5. There is no single policy for handling the tail beyond the end of the frame: an engine
		 *             either discards the extraneous octets or treats them as corruption and rejects the frame.
		 *             The routine receives the buffer length from the calling side and does not look for frame
		 *             boundaries itself, therefore exactly one frame should be fed. One thing is guaranteed:
		 *             the routine terminates, and the data produced is not distorted.
		 *
		 *          6. The DEFLATE method terminates a message with Z_SYNC_FLUSH rather than Z_FINISH, and
		 *             therefore leaves four octets 00 00 FF FF at the end of the result. RFC 7692 requires
		 *             exactly that (permessage-deflate): the context is reused between messages, and Z_FINISH
		 *             would close the stream. Stripping the tail is the business of the calling side, which
		 *             knows whether it hands the message into a WebSocket frame or stores it whole.
		 *
		 *          The full list of deliberate decisions, the registry of rejected findings and the list of
		 *          open questions are in src/compressor/README.md. Examination of the module should begin
		 *          with it: what has already been put into reports and checked is recorded there.
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Block {
			private:
				/**
				 * \~russian
				 * @brief Структура буфера контекстов Deflate
				 *
				 * @note Владеющие указатели на непрозрачный контекст (управление временем жизни в конструкторе/деструкторе Block)
				 *
				 * \~english
				 * @brief Structure of the Deflate contexts buffer
				 *
				 * @note Owning pointers to the opaque context (lifetime management in the constructor/destructor of Block)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ BufferDeflate {
					// Поток Deflate для компрессии
					deflate_stream_t * compress;
					// Поток Deflate для декомпрессии
					deflate_stream_t * decompress;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit BufferDeflate() noexcept;
				} buffer_deflate_t;
				/**
				 * \~russian
				 * @brief Структура переиспользования контекста компрессии/декомпрессии
				 *
				 * \~english
				 * @brief Structure of compression/decompression context reuse
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Takeover {
					// Флаг переиспользования контекста компрессии
					atomic_bool compress;
					// Флаг переиспользования контекста декомпрессии
					atomic_bool decompress;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Takeover() noexcept;
				} takeover_t;
				/**
				 * \~russian
				 * @brief Структура параметров движка со скользящим окном
				 *
				 * @details Заведена для GZip и Zlib: у обоих из настроек только окно,
				 *          и контекст между сообщениями они не переиспользуют
				 *
				 * \~english
				 * @brief Structure of the parameters of an engine with a sliding window
				 *
				 * @details Provided for GZip and Zlib: both have only the window among their settings,
				 *          and they do not reuse the context between messages
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Window {
					// Размер скользящего окна (атомарный для потокобезопасного доступа)
					atomic_int16_t wbits;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Window() noexcept;
				} window_t;
				/**
				 * \~russian
				 * @brief Структура параметров движка Deflate
				 *
				 * @details Переиспользование контекста между сообщениями (RFC 7692) заведено
				 *          для одного лишь Deflate, поэтому контексты и флаги живут здесь,
				 *          а не в общей структуре семейства
				 *
				 * \~english
				 * @brief Structure of the Deflate engine parameters
				 *
				 * @details Context reuse between messages (RFC 7692) is provided for Deflate alone,
				 *          therefore the contexts and flags live here rather than in the common
				 *          structure of the family
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Deflate {
					// Флаги переиспользования контекста компрессии/декомпрессии
					takeover_t takeover;
					// Буфер переиспользуемых контекстов
					buffer_deflate_t buffer;
					// Размер скользящего окна (атомарный для потокобезопасного доступа)
					atomic_int16_t wbits;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Deflate() noexcept;
				} deflate_t;
			private:
				/**
				 * \~russian
				 * @brief Уровни компрессии (атомарные для потокобезопасного доступа)
				 *
				 * @details Индексы соответствуют движкам компрессии:
				 *          [0] LZ4 (уровень LZ4F, знаковый), [1] GZip/Zlib/Deflate (0 - 9),
				 *          [2] Zstandard (1 - ZSTD_maxCLevel()), [3] Lizard (10 - 49),
				 *          [4] Density (DENSITY_ALGORITHM), [5] Brotli (0 - 11),
				 *          [6] LZMA (пресет 0 - 9), [7] BZip2 (размер блока 1 - 9)
				 *
				 *          Уровень LZ4 в обоих режимах трактуется так, как его трактует
				 *          кадровый формат: от LZ4HC_CLEVEL_MIN включается режим высокой
				 *          степени сжатия, отрицательное значение задаёт ускорение быстрого
				 *          режима. Приняв в блочном режиме ускорение, а в кадровом уровень,
				 *          одно значение переворачивало бы направление уровня в одном из них
				 *
				 * \~english
				 * @brief Compression levels (atomic for thread-safe access)
				 *
				 * @details The indices correspond to the compression engines:
				 *          [0] LZ4 (LZ4F level, signed), [1] GZip/Zlib/Deflate (0 - 9),
				 *          [2] Zstandard (1 - ZSTD_maxCLevel()), [3] Lizard (10 - 49),
				 *          [4] Density (DENSITY_ALGORITHM), [5] Brotli (0 - 11),
				 *          [6] LZMA (preset 0 - 9), [7] BZip2 (block size 1 - 9)
				 *
				 *          The LZ4 level in both modes is interpreted the way the frame format interprets
				 *          it: from LZ4HC_CLEVEL_MIN the high compression ratio mode is engaged, a negative
				 *          value sets the acceleration of the fast mode. Had the block mode taken an
				 *          acceleration and the frame mode a level, one and the same value would flip the
				 *          direction of the level in one of them
				 *
				 * \~
				 */
				atomic_int32_t _level[8];
			private:
				// Параметры движка GZip
				mutable window_t _gzip;
				// Параметры движка Zlib
				mutable window_t _zlib;
				// Параметры движка Deflate
				mutable deflate_t _deflate;
			private:
				// Объект работы с логами
				const log_t * _log;
			public:
				/**
				 * \~russian
				 * @brief Метод установки уровня компрессии
				 *
				 * @param level уровень компрессии
				 *
				 * \~english
				 * @brief Compression level setting method
				 *
				 * @param level compression level
				 *
				 * \~
				 */
				void level(const level_t level) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки размера скользящего окна GZip
				 *
				 * @details Окно каждого движка семейства задаётся своим методом: делить его
				 *          на всех нельзя, потому что размер окна Deflate согласуется с узлом
				 *          отдельно (RFC 7692), а окно GZip и Zlib к этому согласованию отношения не имеет
				 *
				 * @param wbits размер скользящего окна
				 *
				 * \~english
				 * @brief GZip sliding window size setting method
				 *
				 * @details The window of each engine of the family is set by its own method: it cannot be
				 *          shared among them all, because the Deflate window size is negotiated with the peer
				 *          separately (RFC 7692), whereas the GZip and Zlib window has nothing to do with that negotiation
				 *
				 * @param wbits sliding window size
				 *
				 * \~
				 */
				void wbitsGZip(const int16_t wbits) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки размера скользящего окна Zlib
				 *
				 * @param wbits размер скользящего окна
				 *
				 * \~english
				 * @brief Zlib sliding window size setting method
				 *
				 * @param wbits sliding window size
				 *
				 * \~
				 */
				void wbitsZlib(const int16_t wbits) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки размера скользящего окна Deflate
				 *
				 * @details Смена размера окна пересобирает переиспользуемые контексты обеими
				 *          половинами разом: у живого контекста размер окна сменить нельзя,
				 *          и отказ пересборки гасит переиспользование, а не оставляет его наполовину
				 *
				 * @param wbits размер скользящего окна
				 * @return      результат установки размера
				 *
				 * \~english
				 * @brief Deflate sliding window size setting method
				 *
				 * @details Changing the window size rebuilds the reused contexts of both halves at once:
				 *          the window size of a live context cannot be changed, and a failure of the rebuild
				 *          extinguishes the reuse rather than leaving it half-done
				 *
				 * @param wbits sliding window size
				 * @return      result of setting the size
				 *
				 * \~
				 */
				bool wbitsDeflate(const int16_t wbits) noexcept;
				/**
				 * \~russian
				 * @brief Метод включения/отключения переиспользования контекста Deflate
				 *
				 * @details Переиспользование контекста между сообщениями (RFC 7692) заведено
				 *          для одного лишь Deflate: у прочих движков семейства сообщение
				 *          самостоятельно, и держать контекст между вызовами им незачем.
				 *          Направления взводятся раздельно — RFC согласует их порознь
				 *
				 * @param event событие выполнения операции
				 * @param flag  флаг переиспользования контекста компрессии/декомпрессии
				 * @return      результат установки флага
				 *
				 * \~english
				 * @brief Method enabling/disabling the reuse of the Deflate context
				 *
				 * @details Context reuse between messages (RFC 7692) is provided for Deflate alone: for the
				 *          other engines of the family a message stands on its own, and there is no point in
				 *          holding a context between calls for them. The directions are engaged separately —
				 *          the RFC negotiates them apart
				 *
				 * @param event operation execution event
				 * @param flag  compression/decompression context reuse flag
				 * @return      result of setting the flag
				 *
				 * \~
				 */
				bool takeoverDeflate(const event_t event, const bool flag) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки поддержки потокового режима методом компрессии
				 *
				 * @param method метод компрессии
				 * @return       результат проверки
				 *
				 * \~english
				 * @brief Method checking the support of the streaming mode by a compression method
				 *
				 * @param method compression method
				 * @return       check result
				 *
				 * \~
				 */
				static bool streamable(const method_t method) noexcept;
				/**
				 * \~russian
				 * @brief Метод создания потоковой сессии
				 *
				 * @details Создаёт объект потоковой (streaming) компрессии/декомпрессии,
				 *          инициализированный текущей конфигурацией. Для методов, не
				 *          поддерживающих потоковый режим, возвращается невалидный поток.
				 *
				 * @param method метод компрессии
				 * @param event  направление операции
				 * @return       объект потоковой сессии
				 *
				 * \~english
				 * @brief Streaming session creation method
				 *
				 * @details Creates an object of streaming compression/decompression initialized with the
				 *          current configuration. For methods not supporting the streaming mode an invalid
				 *          stream is returned.
				 *
				 * @param method compression method
				 * @param event  operation direction
				 * @return       streaming session object
				 *
				 * \~
				 */
				stream_t stream(const method_t method, const event_t event) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Шаблон метода компрессии данных
				 *
				 * @tparam T тип возвращаемого результата
				 *
				 * \~english
				 * @brief Template of the data compression method
				 *
				 * @tparam T returned result type
				 *
				 * \~
				 */
				template <typename T>
				/**
				 * \~russian
				 * @brief Метод компрессии данных
				 *
				 * @param buffer буфер данных для компрессии
				 * @param method метод компрессии
				 * @return       результат компрессии
				 *
				 * \~english
				 * @brief Data compression method
				 *
				 * @param buffer data buffer to compress
				 * @param method compression method
				 * @return       compression result
				 *
				 * \~
				 */
				auto compress(string_view buffer, const method_t method) const noexcept -> T;
				/**
				 * \~russian
				 * @brief Шаблон метода компрессии данных
				 *
				 * @tparam A тип возвращаемого результата
				 * @tparam B тип буфера данных
				 *
				 * \~english
				 * @brief Template of the data compression method
				 *
				 * @tparam A returned result type
				 * @tparam B data buffer type
				 *
				 * \~
				 */
				template <typename A, typename B>
				/**
				 * \~russian
				 * @brief Метод компрессии данных
				 *
				 * @param buffer буфер данных для компрессии
				 * @param method метод компрессии
				 * @return       результат компрессии
				 *
				 * \~english
				 * @brief Data compression method
				 *
				 * @param buffer data buffer to compress
				 * @param method compression method
				 * @return       compression result
				 *
				 * \~
				 */
				auto compress(const B & buffer, const method_t method) const noexcept -> A;
				/**
				 * \~russian
				 * @brief Шаблон метода компрессии данных
				 *
				 * @tparam T тип возвращаемого результата
				 *
				 * \~english
				 * @brief Template of the data compression method
				 *
				 * @tparam T returned result type
				 *
				 * \~
				 */
				template <typename T>
				/**
				 * \~russian
				 * @brief Метод компрессии данных
				 *
				 * @param buffer буфер данных для компрессии
				 * @param size   размер данных для компрессии
				 * @param method метод компрессии
				 * @return       результат компрессии
				 *
				 * \~english
				 * @brief Data compression method
				 *
				 * @param buffer data buffer to compress
				 * @param size   size of the data to compress
				 * @param method compression method
				 * @return       compression result
				 *
				 * \~
				 */
				auto compress(const void * buffer, const size_t size, const method_t method) const noexcept -> T;
			public:
				/**
				 * \~russian
				 * @brief Шаблон метода компрессии данных
				 *
				 * @tparam T тип возвращаемого результата
				 *
				 * \~english
				 * @brief Template of the data compression method
				 *
				 * @tparam T returned result type
				 *
				 * \~
				 */
				template <typename T>
				/**
				 * \~russian
				 * @brief Метод компрессии данных
				 *
				 * @param buffer буфер данных для компрессии
				 * @param size   размер данных для компрессии
				 * @param method метод компрессии
				 * @param result контейнер куда следует положить результат
				 *
				 * \~english
				 * @brief Data compression method
				 *
				 * @param buffer data buffer to compress
				 * @param size   size of the data to compress
				 * @param method compression method
				 * @param result container the result should be placed into
				 *
				 * \~
				 */
				void compress(const void * buffer, const size_t size, const method_t method, T & result) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Шаблон метода декомпрессии данных
				 *
				 * @tparam T тип возвращаемого результата
				 *
				 * \~english
				 * @brief Template of the data decompression method
				 *
				 * @tparam T returned result type
				 *
				 * \~
				 */
				template <typename T>
				/**
				 * \~russian
				 * @brief Метод декомпрессии данных
				 *
				 * @param buffer буфер данных для декомпрессии
				 * @param method метод компрессии
				 * @return       результат декомпрессии
				 *
				 * \~english
				 * @brief Data decompression method
				 *
				 * @param buffer data buffer to decompress
				 * @param method compression method
				 * @return       decompression result
				 *
				 * \~
				 */
				auto decompress(string_view buffer, const method_t method) const noexcept -> T;
				/**
				 * \~russian
				 * @brief Шаблон метода декомпрессии данных
				 *
				 * @tparam A тип возвращаемого результата
				 * @tparam B тип буфера данных
				 *
				 * \~english
				 * @brief Template of the data decompression method
				 *
				 * @tparam A returned result type
				 * @tparam B data buffer type
				 *
				 * \~
				 */
				template <typename A, typename B>
				/**
				 * \~russian
				 * @brief Метод декомпрессии данных
				 *
				 * @param buffer буфер данных для декомпрессии
				 * @param method метод компрессии
				 * @return       результат декомпрессии
				 *
				 * \~english
				 * @brief Data decompression method
				 *
				 * @param buffer data buffer to decompress
				 * @param method compression method
				 * @return       decompression result
				 *
				 * \~
				 */
				auto decompress(const B & buffer, const method_t method) const noexcept -> A;
				/**
				 * \~russian
				 * @brief Шаблон метода декомпрессии данных
				 *
				 * @tparam T тип возвращаемого результата
				 *
				 * \~english
				 * @brief Template of the data decompression method
				 *
				 * @tparam T returned result type
				 *
				 * \~
				 */
				template <typename T>
				/**
				 * \~russian
				 * @brief Метод декомпрессии данных
				 *
				 * @param buffer буфер данных для декомпрессии
				 * @param size   размер данных для декомпрессии
				 * @param method метод компрессии
				 * @return       результат декомпрессии
				 *
				 * \~english
				 * @brief Data decompression method
				 *
				 * @param buffer data buffer to decompress
				 * @param size   size of the data to decompress
				 * @param method compression method
				 * @return       decompression result
				 *
				 * \~
				 */
				auto decompress(const void * buffer, const size_t size, const method_t method) const noexcept -> T;
			public:
				/**
				 * \~russian
				 * @brief Шаблон метода декомпрессии данных
				 *
				 * @tparam T тип возвращаемого результата
				 *
				 * \~english
				 * @brief Template of the data decompression method
				 *
				 * @tparam T returned result type
				 *
				 * \~
				 */
				template <typename T>
				/**
				 * \~russian
				 * @brief Метод декомпрессии данных
				 *
				 * @param buffer буфер данных для декомпрессии
				 * @param size   размер данных для декомпрессии
				 * @param method метод компрессии
				 * @param result контейнер куда следует положить результат
				 *
				 * \~english
				 * @brief Data decompression method
				 *
				 * @param buffer data buffer to decompress
				 * @param size   size of the data to decompress
				 * @param method compression method
				 * @param result container the result should be placed into
				 *
				 * \~
				 */
				void decompress(const void * buffer, const size_t size, const method_t method, T & result) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param log объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * @param log object for working with logs
				 *
				 * \~
				 */
				explicit Block(const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				~Block() noexcept;
		} block_t;
	};
};

#endif // __AWH_COMPRESSOR_BLOCK__
