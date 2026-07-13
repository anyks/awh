/**
 * @file: stateless.hpp
 * @date: 2026-07-13
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

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_COMPRESSOR_STATELESS__
#define __AWH_COMPRESSOR_STATELESS__

/**
 * Стандартные заголовочные файлы
 */
#include <atomic>

/**
 * Подключаем заголовочные файлы проекта
 */
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
		 * @brief Класс компрессии/декомпрессии данных (No Context Takeover)
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Stateless {
			public:
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
					ZLIB    = 0x05, // Метод сжатия Zlib (RFC 1950)
					BZIP2   = 0x06, // Метод сжатия BZip2
					BROTLI  = 0x07, // Метод сжатия Brotli
					LIZARD  = 0x08, // Метод сжатия Lizard
					SNAPPY  = 0x09, // Метод сжатия Snappy
					DEFLATE = 0x0A, // Метод сжатия Deflate
					DENSITY = 0x0B, // Метод сжатия Density
				};
			private:
				// Размер скользящего окна (атомарный для потокобезопасного доступа)
				atomic_int16_t _wbits;
				// Уровни компрессии (атомарные для потокобезопасного доступа)
				atomic_uint32_t _level[5];
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
				 * @brief Метод установки размера скользящего окна
				 *
				 * @param wbits размер скользящего окна
				 */
				void wbitsGZip(const int16_t wbits) noexcept;
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
				explicit Stateless(const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Stateless() noexcept;
		} stateless_t;
	};
};

#endif // __AWH_COMPRESSOR_STATELESS__
