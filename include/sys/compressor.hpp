/**
 * @file: compressor.hpp
 * @date: 2026-01-21
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

#ifndef __AWH_COMPRESSOR__
#define __AWH_COMPRESSOR__

/**
 * Стандартные модули
 */
#include <any>
#include <atomic>
#include <string>
#include <vector>

/**
 * Наши модули
 */
#include "log.hpp"
#include "locker.hpp"

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
	 * @brief Класс компрессии/декомпрессии данных
	 *
	 */
	typedef class AWH_SHARED_EXPORT Compressor {
		public:
			/**
			 * @brief Режимы событий
			 *
			 */
			enum class mode_t : uint8_t {
				ENABLED  = 0x00, // Режим включён
				DISABLED = 0x01  // Режим отключён
			};
			/**
			 * @brief События выполнения операции
			 *
			 */
			enum class event_t : uint8_t {
				NONE   = 0x00, // Событие не установленно
				ENCODE = 0x01, // Кодирование данных
				DECODE = 0x02  // Декодирование данных
			};
			/**
			 * @brief Уровень компрессии
			 *
			 */
			enum class level_t : uint8_t {
				NONE   = 0x00, // Уровень сжатия не установлен
				BEST   = 0x01, // Максимальный уровень компрессии
				SPEED  = 0x02, // Максимальная скорость компрессии
				NORMAL = 0x03  // Нормальный уровень компрессии
			};
			/**
			 * @brief Методы компрессоров
			 *
			 */
			enum class method_t : uint8_t {
				NONE    = 0x00, // Метод сжатия не установлен
				LZ4     = 0x01, // Метод сжатия Lz4
				LZMA    = 0x02, // Метод сжатия LZma
				ZSTD    = 0x03, // Метод сжатия ZStd
				GZIP    = 0x04, // Метод сжатия GZip
				BZIP2   = 0x05, // Метод сжатия BZip2
				BROTLI  = 0x06, // Метод сжатия Brotli
				LIZARD  = 0x07, // Метод сжатия Lizard
				SNAPPY  = 0x08, // Метод сжатия Snappy
				DEFLATE = 0x09, // Метод сжатия Deflate
				DENSITY = 0x0A  // Метод сжатия Density
			};
		private:
			/**
			 * @brief Буфер GZip
			 *
			 */
			typedef struct BufferGZip {
				// Создаем поток GZip для компрессии
				std::any compress;
				// Создаем поток GZip для декомпрессии
				std::any decompress;
			} buffer_gzip_t;
			/**
			 * @brief Структура переиспользования контекста компрессии/декомпрессии
			 *
			 */
			typedef struct Takeover {
				// Флаг переиспользования контекста компрессии
				std::atomic_bool compress;
				// Флаг переиспользования контекста декомпрессии
				std::atomic_bool decompress;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Takeover() noexcept :
				 compress(false), decompress(false) {}
			} takeover_t;
			/**
			 * @brief Структура GZip
			 *
			 */
			typedef struct GZip {
				// Размер скользящего окна
				int16_t wbits;
				// Флаги переиспользования контекста компрессии/декомпрессии
				takeover_t takeover;
				// Буфер GZip
				buffer_gzip_t buffer;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit GZip() noexcept : wbits(0) {}
			} gzip_t;
		private:
			// Уровни компрессии
			uint32_t _level[5];
		private:
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
			 * @param mode режим безопасности потоков
			 */
			void threadSafety(const mode_t mode) noexcept;
		public:
			/**
			 * @brief Метод установки размера скользящего окна
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
			explicit Compressor(const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Compressor() noexcept;
	} compressor_t;
};

#endif // __AWH_COMPRESSOR__
