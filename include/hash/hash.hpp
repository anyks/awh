/**
 * @file: hash.hpp
 * @date: 2023-02-11
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2023
 */

#ifndef __AWH_HASH__
#define __AWH_HASH__

/**
 * Стандартная библиотека
 */
#include <ctime>
#include <string>
#include <vector>
#include <csignal>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>

/**
 * Подключаем LZ4
*/
#include <lz4.h>
#include <lz4hc.h>

/**
 * Подключаем GZip
*/
#include <zlib.h>

/**
 * Подключаем ZStd
*/
#include <zstd.h>

/**
 * Подключаем BZip2
*/
#include <bzlib.h>

/**
 * Подключаем LZma
*/
#include <lzma.h>

/**
 * Подключаем Brotli
 */
#include <brotli/decode.h>
#include <brotli/encode.h>

/**
 * Подключаем OpenSSL
 */
#include <openssl/md5.h>
#include <openssl/aes.h>
#include <openssl/evp.h>

/**
 * Наши модули
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>

// Параметры Zlib
#define MOD_GZIP_ZLIB_CFACTOR 9
#define MOD_GZIP_ZLIB_BSIZE 8096

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Hash Класс хеширования данных
	 */
	typedef class Hash {
		private:
			/**
			 * События выполнения операции
			 */
			enum class event_t : uint8_t {
				NONE       = 0x00, // Событие не установленно
				COMPRESS   = 0x01, // Компрессия данных
				DECOMPRESS = 0x02  // Декомпрессия данных
			};
		private:
			/**
			 * State Стрейт шифрования
			 */
			mutable struct State {
				// Количество обработанных байт
				int num;
				// Буфер данных для шифрования
				u_char ivec[AES_BLOCK_SIZE];
				/**
				 * State Конструктор
				 */
				State() noexcept : num(0), ivec{0} {}
			} _state;
		public:
			/**
			 * Набор размеров шифрования
			 */
			enum class cipher_t : uint16_t {
				NONE   = 0,   // Размер шифрования не установлен
				AES128 = 128, // Размер шифрования 128 бит
				AES192 = 192, // Размер шифрования 192 бит
				AES256 = 256  // Размер шифрования 256 бит
			};
			/**
			 * Методы компрессии
			 */
			enum class method_t : uint8_t {
				NONE    = 0x00, // Метод сжатия не установлен
				LZ4     = 0x01, // Метод сжатия Lz4
				LZMA    = 0x02, // Метод сжатия LZma
				ZSTD    = 0x03, // Метод сжатия ZStd
				GZIP    = 0x04, // Метод сжатия GZip
				BZIP2   = 0x05, // Метод сжания BZip2
				BROTLI  = 0x06, // Метод сжатия Brotli
				DEFLATE = 0x07  // Метод сжатия Deflate
			};
			/**
			 * Уровень компрессии
			 */
			enum class level_t : uint8_t {
				NONE   = 0x00, // Уровень сжатия не установлен
				BEST   = 0x01, // Максимальный уровень компрессии
				SPEED  = 0x02, // Максимальная скорость компрессии
				NORMAL = 0x03  // Нормальный уровень компрессии
			};
		private:
			// Размер скользящего окна
			short _wbit;
			// Устанавливаем количество раундов
			int _rounds;
		private:
			// Уровни компрессии
			uint32_t _level[3];
		private:
			// Соль и пароль для шифрования
			string _salt, _pass;
		private:
			// Флаг переиспользования контекста компрессии
			bool _takeOverCompress;
			// Флаг переиспользования контекста декомпрессии
			bool _takeOverDecompress;
		private:
			// Хвостовой буфер для удаления из финального сообщения
			const char _btype[4];
		private:
			// Определяем размер шифрования по умолчанию
			cipher_t _cipher;
		private:
			// Ключ шифрования
			mutable AES_KEY _key;
		private:
			// Создаем поток ZLib для декомпрессии
			mutable z_stream _zinf;
			// Создаем поток ZLib для компрессии
			mutable z_stream _zdef;
		private:
			// Создаём объект работы с логами
			const log_t * _log;
		private:
			// Устанавливаем уровень сжатия
			static constexpr u_short DEFAULT_MEM_LEVEL = 4;
			// Размер буфера чанка в байтах
			static constexpr uint32_t CHUNK_BUFFER_SIZE = 0x4000;
		private:
			/**
			 * init Метод инициализации AES шифрования
			 * @return результат инициализации
			 */
			bool init() const;
		public:
			/**
			 * cipher Метод получения размера шифрования
			 * @return размер шифрования
			 */
			cipher_t cipher() const;
			/**
			 * cipher Метод установки размера шифрования
			 * @param cipher размер шифрования (128, 192, 256)
			 */
			void cipher(const cipher_t cipher) noexcept;
		public:
			/**
			 * rmTail Метод удаления хвостовых данных
			 * @param buffer буфер для удаления хвоста
			 */
			void rmTail(vector <char> & buffer) const noexcept;
			/**
			 * setTail Метод добавления хвостовых данных
			 * @param buffer буфер для добавления хвоста
			 */
			void setTail(vector <char> & buffer) const noexcept;
		private:
			/**
			 * lz4 Метод работы с компрессором Lz4
			 * @param buffer буфер данных для компрессии
			 * @param size   размер данных для компрессии
			 * @param event  событие выполнения операции
			 * @return       результат выполнения операции
			 */
			vector <char> lz4(const char * buffer, const size_t size, const event_t event) noexcept;
			/**
			 * lzma Метод работы с компрессором LZma
			 * @param buffer буфер данных для компрессии
			 * @param size   размер данных для компрессии
			 * @param event  событие выполнения операции
			 * @return       результат выполнения операции
			 */
			vector <char> lzma(const char * buffer, const size_t size, const event_t event) noexcept;
			/**
			 * zstd Метод работы с компрессором ZStd
			 * @param buffer буфер данных для компрессии
			 * @param size   размер данных для компрессии
			 * @param event  событие выполнения операции
			 * @return       результат выполнения операции
			 */
			vector <char> zstd(const char * buffer, const size_t size, const event_t event) noexcept;
			/**
			 * gzip Метод работы с компрессором GZip
			 * @param buffer буфер данных для компрессии
			 * @param size   размер данных для компрессии
			 * @param event  событие выполнения операции
			 * @return       результат выполнения операции
			 */
			vector <char> gzip(const char * buffer, const size_t size, const event_t event) noexcept;
			/**
			 * bzip2 Метод работы с компрессором BZip2
			 * @param buffer буфер данных для компрессии
			 * @param size   размер данных для компрессии
			 * @param event  событие выполнения операции
			 * @return       результат выполнения операции
			 */
			vector <char> bzip2(const char * buffer, const size_t size, const event_t event) noexcept;
			/**
			 * brotli Метод работы с компрессором Brotli
			 * @param buffer буфер данных для компрессии
			 * @param size   размер данных для компрессии
			 * @param event  событие выполнения операции
			 * @return       результат выполнения операции
			 */
			vector <char> brotli(const char * buffer, const size_t size, const event_t event) noexcept;
			/**
			 * deflate Метод работы с компрессором Deflate
			 * @param buffer буфер данных для компрессии
			 * @param size   размер данных для компрессии
			 * @param event  событие выполнения операции
			 * @return       результат выполнения операции
			 */
			vector <char> deflate(const char * buffer, const size_t size, const event_t event) noexcept;
		public:
			/**
			 * encrypt Метод шифрования текста
			 * @param buffer буфер данных для шифрования
			 * @param size   размер данных для шифрования
			 * @return       результат шифрования
			 */
			vector <char> encrypt(const char * buffer, const size_t size) const noexcept;
			/**
			 * decrypt Метод дешифрования текста
			 * @param buffer буфер данных для дешифрования
			 * @param size   размер данных для дешифрования
			 * @return       результат дешифрования
			 */
			vector <char> decrypt(const char * buffer, const size_t size) const noexcept;
		public:
			/**
			 * compress Метод компрессии данных
			 * @param buffer буфер данных для компрессии
			 * @param size   размер данных для компрессии
			 * @param method метод компрессии
			 * @return       результат компрессии
			 */
			vector <char> compress(const char * buffer, const size_t size, const method_t method) noexcept;
			/**
			 * decompress Метод декомпрессии данных
			 * @param buffer буфер данных для декомпрессии
			 * @param size   размер данных для декомпрессии
			 * @param method метод компрессии
			 * @return       результат декомпрессии
			 */
			vector <char> decompress(const char * buffer, const size_t size, const method_t method) noexcept;
		public:
			/**
			 * wbit Метод установки размера скользящего окна
			 * @param wbit размер скользящего окна
			 */
			void wbit(const short wbit) noexcept;
			/**
			 * round Метод установки количества раундов шифрования
			 * @param round количество раундов шифрования
			 */
			void round(const int round) noexcept;
			/**
			 * salt Метод установки соли шифрования
			 * @param salt соль для шифрования
			 */
			void salt(const string & salt) noexcept;
			/**
			 * pass Метод установки пароля шифрования
			 * @param pass пароль шифрования
			 */
			void pass(const string & pass) noexcept;
			/**
			 * level Метод установки уровня компрессии
			 * @param level уровень компрессии
			 */
			void level(const level_t level) noexcept;
		public:
			/**
			 * takeoverCompress Метод установки флага переиспользования контекста компрессии
			 * @param flag флаг переиспользования контекста компрессии
			 */
			void takeoverCompress(const bool flag) noexcept;
			/**
			 * takeoverDecompress Метод установки флага переиспользования контекста декомпрессии
			 * @param flag флаг переиспользования контекста декомпрессии
			 */
			void takeoverDecompress(const bool flag) noexcept;
		public:
			/**
			 * Hash Конструктор
			 * @param log объект для работы с логами
			 */
			Hash(const log_t * log) noexcept :
			 _wbit(MAX_WBITS), _rounds(5), _level{1, Z_DEFAULT_COMPRESSION, ZSTD_CLEVEL_DEFAULT},
			 _salt{""}, _pass{""}, _takeOverCompress(false), _takeOverDecompress(false),
			 _btype{0x00, 0x00, 0xFF, 0xFF}, _cipher(cipher_t::AES128), _zinf({0}), _zdef({0}), _log(log) {}
			/**
			 * ~Hash Деструктор
			 */
			~Hash() noexcept;
	} hash_t;
};

#endif // __AWH_HASH__
