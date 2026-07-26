/**
 * @file: types.hpp
 * @date: 2026-07-13
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
#ifndef __AWH_COMPRESSOR_TYPES__
#define __AWH_COMPRESSOR_TYPES__

/**
 * Стандартные заголовочные файлы
 */
#include <cstdint>

/**
 * Подключаем заголовочный файл проекта
 */
#include "../sys/global.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён компрессора
	 *
	 */
	namespace compressor {
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
		 * @brief Режим сброса данных в потоковом режиме
		 *
		 */
		enum class flush_t : uint8_t {
			NONE   = 0x00, // Копить данные ради степени сжатия
			SYNC   = 0x01, // Выдавить накопленное на границе (для отправки в сеть)
			FINISH = 0x02  // Финализировать поток
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
		/**
		 * @brief Параметры инициализации потоковой сессии
		 *
		 */
		typedef struct __AWH_SHARED_EXPORT__ Params {
			// Размер скользящего окна (для deflate/gzip/zlib, по умолчанию 15)
			int16_t wbits;
			// Уровень компрессии (интерпретация зависит от движка)
			int32_t level;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Params() noexcept;
		} params_t;
	};
};

#endif // __AWH_COMPRESSOR_TYPES__
