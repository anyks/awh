/**
 * @file types.hpp
 * @date 2026-07-13
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
 * @brief Заголовочный файл общих типов подсистемы компрессии — перечисления событий, режимов сброса буфера,
 *        уровней сжатия и методов компрессии, а также структура параметров инициализации потоковой сессии,
 *        общая для блочного и потокового кодеков
 *
 * \~english
 * @brief Header file of the compression subsystem common types — enumerations of events, buffer flush modes,
 *        compression levels and compression methods, as well as the streaming session initialization
 *        parameters structure shared by the block and streaming codecs
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
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
#include <cstddef>

/**
 * Подключаем заголовочный файл проекта
 */
#include "../sys/global.hpp"

/**
 * \~russian
 * @brief Предел памяти, отводимой распаковке LZMA
 *
 * @details Общий для блочного и потокового режимов: один и тот же контейнер должен
 *          приниматься либо отвергаться обоими одинаково. Величина ограждает от
 *          подделанного заголовка со словарём непомерного размера — распаковка такого
 *          контейнера отводила бы память по указанию недоверенной стороны. Ста
 *          двадцати восьми мегабайт хватает с запасом: наибольший словарь, которым
 *          пользуются на деле, вчетверо меньше
 *
 * \~english
 * @brief Memory limit allotted to LZMA decompression
 *
 * @details Shared by the block and streaming modes: the very same container must be accepted
 *          or rejected by both alike. The value guards against a forged header carrying an
 *          outrageously large dictionary — decompressing such a container would allocate memory
 *          at the direction of an untrusted party. One hundred and twenty eight megabytes is
 *          ample: the largest dictionary used in practice is four times smaller
 *
 * \~
 */
#ifndef AWH_COMPRESSOR_LZMA_MEMLIMIT
	#define AWH_COMPRESSOR_LZMA_MEMLIMIT (128ull * 1024ull * 1024ull)
#endif

/**
 * \~russian
 * @brief Предел объёма, выдаваемого распаковкой за одну подачу
 *
 * @details Общий для блочного и потокового режимов. Ограждает от недоверенного входа,
 *          у которого крошечный кадр разворачивается в гигабайты: движок отводил бы
 *          память по указанию отправляющей стороны. Предел действует на одну подачу,
 *          а не на весь поток — иначе долгий обмен, ради которого потоковый режим и
 *          заведён, упирался бы в него на законных данных
 *
 * \~english
 * @brief Limit on the volume produced by decompression per single feed
 *
 * @details Shared by the block and streaming modes. Guards against untrusted input whose tiny
 *          frame expands into gigabytes: the engine would allocate memory at the direction of
 *          the sending party. The limit applies to a single feed rather than to the whole stream —
 *          otherwise the lengthy exchange the streaming mode exists for would hit it on legitimate data
 *
 * \~
 */
#ifndef AWH_COMPRESSOR_MAX_OUTPUT
	#define AWH_COMPRESSOR_MAX_OUTPUT (1ull << 30)
#endif

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
		 * @brief События выполнения операции
		 *
		 * \~english
		 * @brief Operation execution events
		 *
		 * \~
		 */
		enum class event_t : uint8_t {
			NONE   = 0x00, // Событие не установленно
			ENCODE = 0x01, // Кодирование данных
			DECODE = 0x02  // Декодирование данных
		};
		/**
		 * \~russian
		 * @brief Режим сброса данных в потоковом режиме
		 *
		 * \~english
		 * @brief Data flush mode in the streaming mode
		 *
		 * \~
		 */
		enum class flush_t : uint8_t {
			NONE   = 0x00, // Копить данные ради степени сжатия
			SYNC   = 0x01, // Выдавить накопленное на границе (для отправки в сеть)
			FINISH = 0x02  // Финализировать поток
		};
		/**
		 * \~russian
		 * @brief Уровень компрессии
		 *
		 * \~english
		 * @brief Compression level
		 *
		 * \~
		 */
		enum class level_t : uint8_t {
			NONE   = 0x00, // Уровень сжатия не установлен
			BEST   = 0x01, // Максимальный уровень компрессии
			SPEED  = 0x02, // Максимальная скорость компрессии
			NORMAL = 0x03  // Нормальный уровень компрессии
		};
		/**
		 * \~russian
		 * @brief Методы компрессоров
		 *
		 * \~english
		 * @brief Compressor methods
		 *
		 * \~
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
		 * \~russian
		 * @brief Функция проверки размера буфера данных на пригодность движку компрессии
		 *
		 * @details Часть движков принимает размер разрядностью 32 бита, и буфер больше их
		 *          предела был бы обработан частично, без всякого признака отказа. Проверка
		 *          общая для блочного и потокового режимов: договор у них один
		 *
		 * @param size   размер буфера данных
		 * @param method метод компрессии
		 * @return       результат проверки
		 *
		 * \~english
		 * @brief Function checking the data buffer size for suitability to the compression engine
		 *
		 * @details Some engines accept a size of 32-bit width, and a buffer larger than their limit
		 *          would be processed partially, without any sign of failure. The check is shared by
		 *          the block and streaming modes: their contract is one and the same
		 *
		 * @param size   data buffer size
		 * @param method compression method
		 * @return       check result
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool fits(const size_t size, const method_t method) noexcept;
		/**
		 * \~russian
		 * @brief Параметры инициализации потоковой сессии
		 *
		 * \~english
		 * @brief Streaming session initialization parameters
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Params {
			// Размер скользящего окна (для deflate/gzip/zlib, по умолчанию 15)
			int16_t wbits;
			/**
			 * \~russian
			 * @brief Уровень компрессии (интерпретация зависит от движка)
			 *
			 * @details Значение -1 означает «не задано»: движок подставит собственное умолчание.
			 *          Ноль значением по умолчанию быть не может — у Brotli это законное качество
			 *          компрессии, а у zlib — запрет сжатия вовсе
			 *
			 * \~english
			 * @brief Compression level (interpretation depends on the engine)
			 *
			 * @details The value -1 means "not set": the engine substitutes its own default. Zero cannot
			 *          serve as the default — for Brotli it is a legitimate compression quality, and for
			 *          zlib it forbids compression altogether
			 *
			 * \~
			 */
			int32_t level;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * \~
			 */
			explicit Params() noexcept;
		} params_t;
	};
};

#endif // __AWH_COMPRESSOR_TYPES__
