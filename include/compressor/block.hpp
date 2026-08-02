/**
 * @file: compressor.hpp
 * @date: 2026-01-21
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл модуля блочной (one-shot) компрессии — публичный API класса compressor::Block,
 *        выполняющего сжатие и распаковку данных целиком за один вызов методами GZip, Zlib, Deflate, Brotli, LZ4,
 *        Zstd, LZma, BZip2, Lzip и другими поддерживаемыми алгоритмами
 *
 * @copyright: Copyright © 2026
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
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Пространство имён компрессора
	 *
	 */
	namespace compressor {
		/**
		 * @brief Предварительное объявление непрозрачного контекста потока Deflate
		 *
		 * @details Полное определение скрыто в модуле реализации,
		 *          что позволяет не подключать заголовочные файлы стороннего компрессора (zlib) в публичный интерфейс.
		 *
		 */
		struct deflate_stream_t;
		/**
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
		 */
		typedef class __AWH_SHARED_EXPORT__ Block {
			private:
				/**
				 * @brief Структура буфера контекстов Deflate
				 *
				 * @note Владеющие указатели на непрозрачный контекст (управление временем жизни в конструкторе/деструкторе Block)
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ BufferDeflate {
					// Поток Deflate для компрессии
					deflate_stream_t * compress;
					// Поток Deflate для декомпрессии
					deflate_stream_t * decompress;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit BufferDeflate() noexcept;
				} buffer_deflate_t;
				/**
				 * @brief Структура переиспользования контекста компрессии/декомпрессии
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Takeover {
					// Флаг переиспользования контекста компрессии
					atomic_bool compress;
					// Флаг переиспользования контекста декомпрессии
					atomic_bool decompress;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Takeover() noexcept;
				} takeover_t;
				/**
				 * @brief Структура параметров движка со скользящим окном
				 *
				 * @details Заведена для GZip и Zlib: у обоих из настроек только окно,
				 *          и контекст между сообщениями они не переиспользуют
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Window {
					// Размер скользящего окна (атомарный для потокобезопасного доступа)
					atomic_int16_t wbits;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Window() noexcept;
				} window_t;
				/**
				 * @brief Структура параметров движка Deflate
				 *
				 * @details Переиспользование контекста между сообщениями (RFC 7692) заведено
				 *          для одного лишь Deflate, поэтому контексты и флаги живут здесь,
				 *          а не в общей структуре семейства
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Deflate {
					// Флаги переиспользования контекста компрессии/декомпрессии
					takeover_t takeover;
					// Буфер переиспользуемых контекстов
					buffer_deflate_t buffer;
					// Размер скользящего окна (атомарный для потокобезопасного доступа)
					atomic_int16_t wbits;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Deflate() noexcept;
				} deflate_t;
			private:
				/**
				 * Уровни компрессии (атомарные для потокобезопасного доступа)
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
				 * @brief Метод установки уровня компрессии
				 *
				 * @param level уровень компрессии
				 *
				 */
				void level(const level_t level) noexcept;
			public:
				/**
				 * @brief Метод установки размера скользящего окна GZip
				 *
				 * @details Окно каждого движка семейства задаётся своим методом: делить его
				 *          на всех нельзя, потому что размер окна Deflate согласуется с узлом
				 *          отдельно (RFC 7692), а окно GZip и Zlib к этому согласованию отношения не имеет
				 *
				 * @param wbits размер скользящего окна
				 *
				 */
				void wbitsGZip(const int16_t wbits) noexcept;
				/**
				 * @brief Метод установки размера скользящего окна Zlib
				 *
				 * @param wbits размер скользящего окна
				 *
				 */
				void wbitsZlib(const int16_t wbits) noexcept;
				/**
				 * @brief Метод установки размера скользящего окна Deflate
				 *
				 * @details Смена размера окна пересобирает переиспользуемые контексты обеими
				 *          половинами разом: у живого контекста размер окна сменить нельзя,
				 *          и отказ пересборки гасит переиспользование, а не оставляет его наполовину
				 *
				 * @param wbits размер скользящего окна
				 * @return      результат установки размера
				 *
				 */
				bool wbitsDeflate(const int16_t wbits) noexcept;
				/**
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
				 */
				bool takeoverDeflate(const event_t event, const bool flag) noexcept;
			public:
				/**
				 * @brief Метод проверки поддержки потокового режима методом компрессии
				 *
				 * @param method метод компрессии
				 * @return       результат проверки
				 *
				 */
				static bool streamable(const method_t method) noexcept;
				/**
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
				 */
				stream_t stream(const method_t method, const event_t event) const noexcept;
			public:
				/**
				 * @brief Шаблон метода компрессии данных
				 *
				 * @tparam T тип возвращаемого результата
				 *
				 */
				template <typename T>
				/**
				 * @brief Метод компрессии данных
				 *
				 * @param buffer буфер данных для компрессии
				 * @param method метод компрессии
				 * @return       результат компрессии
				 *
				 */
				auto compress(string_view buffer, const method_t method) const noexcept -> T;
				/**
				 * @brief Шаблон метода компрессии данных
				 *
				 * @tparam A тип возвращаемого результата
				 * @tparam B тип буфера данных
				 *
				 */
				template <typename A, typename B>
				/**
				 * @brief Метод компрессии данных
				 *
				 * @param buffer буфер данных для компрессии
				 * @param method метод компрессии
				 * @return       результат компрессии
				 *
				 */
				auto compress(const B & buffer, const method_t method) const noexcept -> A;
				/**
				 * @brief Шаблон метода компрессии данных
				 *
				 * @tparam T тип возвращаемого результата
				 *
				 */
				template <typename T>
				/**
				 * @brief Метод компрессии данных
				 *
				 * @param buffer буфер данных для компрессии
				 * @param size   размер данных для компрессии
				 * @param method метод компрессии
				 * @return       результат компрессии
				 *
				 */
				auto compress(const void * buffer, const size_t size, const method_t method) const noexcept -> T;
			public:
				/**
				 * @brief Шаблон метода компрессии данных
				 *
				 * @tparam T тип возвращаемого результата
				 *
				 */
				template <typename T>
				/**
				 * @brief Метод компрессии данных
				 *
				 * @param buffer буфер данных для компрессии
				 * @param size   размер данных для компрессии
				 * @param method метод компрессии
				 * @param result контейнер куда следует положить результат
				 *
				 */
				void compress(const void * buffer, const size_t size, const method_t method, T & result) const noexcept;
			public:
				/**
				 * @brief Шаблон метода декомпрессии данных
				 *
				 * @tparam T тип возвращаемого результата
				 *
				 */
				template <typename T>
				/**
				 * @brief Метод декомпрессии данных
				 *
				 * @param buffer буфер данных для декомпрессии
				 * @param method метод компрессии
				 * @return       результат декомпрессии
				 *
				 */
				auto decompress(string_view buffer, const method_t method) const noexcept -> T;
				/**
				 * @brief Шаблон метода декомпрессии данных
				 *
				 * @tparam A тип возвращаемого результата
				 * @tparam B тип буфера данных
				 *
				 */
				template <typename A, typename B>
				/**
				 * @brief Метод декомпрессии данных
				 *
				 * @param buffer буфер данных для декомпрессии
				 * @param method метод компрессии
				 * @return       результат декомпрессии
				 *
				 */
				auto decompress(const B & buffer, const method_t method) const noexcept -> A;
				/**
				 * @brief Шаблон метода декомпрессии данных
				 *
				 * @tparam T тип возвращаемого результата
				 *
				 */
				template <typename T>
				/**
				 * @brief Метод декомпрессии данных
				 *
				 * @param buffer буфер данных для декомпрессии
				 * @param size   размер данных для декомпрессии
				 * @param method метод компрессии
				 * @return       результат декомпрессии
				 *
				 */
				auto decompress(const void * buffer, const size_t size, const method_t method) const noexcept -> T;
			public:
				/**
				 * @brief Шаблон метода декомпрессии данных
				 *
				 * @tparam T тип возвращаемого результата
				 *
				 */
				template <typename T>
				/**
				 * @brief Метод декомпрессии данных
				 *
				 * @param buffer буфер данных для декомпрессии
				 * @param size   размер данных для декомпрессии
				 * @param method метод компрессии
				 * @param result контейнер куда следует положить результат
				 *
				 */
				void decompress(const void * buffer, const size_t size, const method_t method, T & result) const noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param log объект для работы с логами
				 *
				 */
				explicit Block(const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Block() noexcept;
		} block_t;
	};
};

#endif // __AWH_COMPRESSOR_BLOCK__
