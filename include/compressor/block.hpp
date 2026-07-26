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
 * @copyright: Copyright © 2026
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
#include "../sys/locker.hpp"

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
		 * @brief Предварительное объявление непрозрачного контекста потока GZip
		 *
		 * @details Полное определение скрыто в модуле реализации,
		 *          что позволяет не подключать заголовочные файлы стороннего компрессора (zlib) в публичный интерфейс.
		 */
		struct gzip_stream_t;
		/**
		 * @brief Класс блочной (one-shot) компрессии/декомпрессии данных
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Block {
			private:
				/**
				 * @brief Структура буфера GZip
				 *
				 * @note Владеющие указатели на непрозрачный контекст (управление временем жизни в конструкторе/деструкторе Block)
				 */
				typedef struct __AWH_SHARED_EXPORT__ BufferGZip {
					// Поток GZip для компрессии
					gzip_stream_t * compress;
					// Поток GZip для декомпрессии
					gzip_stream_t * decompress;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit BufferGZip() noexcept;
				} buffer_gzip_t;
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
				 * @brief Структура Zlib
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Zlib {
					// Размер скользящего окна (атомарный для потокобезопасного доступа)
					atomic_int16_t wbits;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Zlib() noexcept;
				} zlib_t;
				/**
				 * @brief Структура GZip
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ GZip {
					// Флаги переиспользования контекста компрессии/декомпрессии
					takeover_t takeover;
					// Буфер GZip
					buffer_gzip_t buffer;
					// Размер скользящего окна (атомарный для потокобезопасного доступа)
					atomic_int16_t wbits;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit GZip() noexcept;
				} gzip_t;
			private:
				// Уровни компрессии (атомарные для потокобезопасного доступа)
				atomic_uint32_t _level[5];
			private:
				// Структура Zlib
				mutable zlib_t _zlib;
				// Структура GZip
				mutable gzip_t _gzip;
			private:
				// Локер для потокобезопасной работы
				mutable lock_state_t <std::mutex> _mtx;
			private:
				// Объект работы с логами
				const log_t * _log;
			public:
				/**
				 * @brief Метод установки уровня компрессии
				 *
				 * @param level уровень компрессии
				 */
				void level(const level_t level) noexcept;
			public:
				/**
				 * @brief Метод установки безопасности работы потоков
				 *
				 * @param mode флаг режима безопасности потоков
				 */
				void threadSafety(const bool mode) noexcept;
			public:
				/**
				 * @brief Метод установки размера скользящего окна Zlib
				 *
				 * @param wbits размер скользящего окна
				 */
				void wbitsZlib(const int16_t wbits) noexcept;
				/**
				 * @brief Метод установки размера скользящего окна GZip/Deflate
				 *
				 * @param wbits размер скользящего окна
				 * @return      результат установки размера
				 */
				bool wbitsGZip(const int16_t wbits) noexcept;
				/**
				 * @brief Метод включения/отключения флага переиспользования контекста компрессии/декомпрессии
				 *
				 * @param event событие выполнения операции
				 * @param flag  флаг переиспользования контекста компрессии/декомпрессии
				 * @return      результат установки флага
				 */
				bool takeoverGZip(const event_t event, const bool flag) noexcept;
			public:
				/**
				 * @brief Метод проверки поддержки потокового режима методом компрессии
				 *
				 * @param method метод компрессии
				 * @return       результат проверки
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
				 */
				stream_t stream(const method_t method, const event_t event) const noexcept;
			public:
				/**
				 * @brief Шаблон метода компрессии данных
				 *
				 * @tparam T тип возвращаемого результата
				 */
				template <typename T>
				/**
				 * @brief Метод компрессии данных
				 *
				 * @param buffer буфер данных для компрессии
				 * @param method метод компрессии
				 * @return       результат компрессии
				 */
				auto compress(string_view buffer, const method_t method) const noexcept -> T;
				/**
				 * @brief Шаблон метода компрессии данных
				 *
				 * @tparam A тип возвращаемого результата
				 * @tparam B тип буфера данных
				 */
				template <typename A, typename B>
				/**
				 * @brief Метод компрессии данных
				 *
				 * @param buffer буфер данных для компрессии
				 * @param method метод компрессии
				 * @return       результат компрессии
				 */
				auto compress(const B & buffer, const method_t method) const noexcept -> A;
				/**
				 * @brief Шаблон метода компрессии данных
				 *
				 * @tparam T тип возвращаемого результата
				 */
				template <typename T>
				/**
				 * @brief Метод компрессии данных
				 *
				 * @param buffer буфер данных для компрессии
				 * @param size   размер данных для компрессии
				 * @param method метод компрессии
				 * @return       результат компрессии
				 */
				auto compress(const void * buffer, const size_t size, const method_t method) const noexcept -> T;
			public:
				/**
				 * @brief Шаблон метода компрессии данных
				 *
				 * @tparam T тип возвращаемого результата
				 */
				template <typename T>
				/**
				 * @brief Метод компрессии данных
				 *
				 * @param buffer буфер данных для компрессии
				 * @param size   размер данных для компрессии
				 * @param method метод компрессии
				 * @param result контейнер куда следует положить результат
				 */
				void compress(const void * buffer, const size_t size, const method_t method, T & result) const noexcept;
			public:
				/**
				 * @brief Шаблон метода декомпрессии данных
				 *
				 * @tparam T тип возвращаемого результата
				 */
				template <typename T>
				/**
				 * @brief Метод декомпрессии данных
				 *
				 * @param buffer буфер данных для декомпрессии
				 * @param method метод компрессии
				 * @return       результат декомпрессии
				 */
				auto decompress(string_view buffer, const method_t method) const noexcept -> T;
				/**
				 * @brief Шаблон метода декомпрессии данных
				 *
				 * @tparam A тип возвращаемого результата
				 * @tparam B тип буфера данных
				 */
				template <typename A, typename B>
				/**
				 * @brief Метод декомпрессии данных
				 *
				 * @param buffer буфер данных для декомпрессии
				 * @param method метод компрессии
				 * @return       результат декомпрессии
				 */
				auto decompress(const B & buffer, const method_t method) const noexcept -> A;
				/**
				 * @brief Шаблон метода декомпрессии данных
				 *
				 * @tparam T тип возвращаемого результата
				 */
				template <typename T>
				/**
				 * @brief Метод декомпрессии данных
				 *
				 * @param buffer буфер данных для декомпрессии
				 * @param size   размер данных для декомпрессии
				 * @param method метод компрессии
				 * @return       результат декомпрессии
				 */
				auto decompress(const void * buffer, const size_t size, const method_t method) const noexcept -> T;
			public:
				/**
				 * @brief Шаблон метода декомпрессии данных
				 *
				 * @tparam T тип возвращаемого результата
				 */
				template <typename T>
				/**
				 * @brief Метод декомпрессии данных
				 *
				 * @param buffer буфер данных для декомпрессии
				 * @param size   размер данных для декомпрессии
				 * @param method метод компрессии
				 * @param result контейнер куда следует положить результат
				 */
				void decompress(const void * buffer, const size_t size, const method_t method, T & result) const noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param log объект для работы с логами
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
