/**
 * @file: hash.hpp
 * @date: 2021-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2021
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
#include <zlib.h>
#include <sys/stat.h>
#include <sys/types.h>

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
				AES128 = 128, // Размер шифрования 128 бит
				AES192 = 192, // Размер шифрования 192 бит
				AES256 = 256  // Размер шифрования 256 бит
			};
			/**
			 * Методы компрессии
			 */
			enum class method_t : uint8_t {
				NONE    = 0x00, // Метод сжатия не установлен
				GZIP    = 0x01, // Метод сжатия GZIP
				BROTLI  = 0x02, // Метод сжатия BROTLI
				DEFLATE = 0x03  // Метод сжатия DEFLATE
			};
		private:
			// Размер скользящего окна
			short _wbit;
			// Устанавливаем количество раундов
			int _rounds;
		public:
			// Уровень сжатия
			u_int levelGzip;
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
			static constexpr u_int CHUNK_BUFFER_SIZE = 0x4000;
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
			 * compressBrotli Метод компрессии данных в Brotli
			 * @param buffer буфер данных для компрессии
			 * @param size   размер данных для компрессии
			 * @return       результат компрессии
			 */
			vector <char> compressBrotli(const char * buffer, const size_t size) noexcept;
			/**
			 * decompressBrotli Метод декомпрессии данных в Brotli
			 * @param buffer буфер данных для декомпрессии
			 * @param size   размер данных для декомпрессии
			 * @return       результат декомпрессии
			 */
			vector <char> decompressBrotli(const char * buffer, const size_t size) noexcept;
		private:
			/**
			 * compressGzip Метод компрессии данных в GZIP
			 * @param buffer буфер данных для компрессии
			 * @param size   размер данных для компрессии
			 * @return       результат компрессии
			 */
			vector <char> compressGzip(const char * buffer, const size_t size) const noexcept;
			/**
			 * decompressGzip Метод декомпрессии данных в GZIP
			 * @param buffer буфер данных для декомпрессии
			 * @param size   размер данных для декомпрессии
			 * @return       результат декомпрессии
			 */
			vector <char> decompressGzip(const char * buffer, const size_t size) const noexcept;
		private:
			/**
			 * compressDeflate Метод компрессии данных в DEFLATE
			 * @param buffer буфер данных для компрессии
			 * @param size   размер данных для компрессии
			 * @return       результат компрессии
			 */
			vector <char> compressDeflate(const char * buffer, const size_t size) const noexcept;
			/**
			 * decompressDeflate Метод декомпрессии данных в DEFLATE
			 * @param buffer буфер данных для декомпрессии
			 * @param size   размер данных для декомпрессии
			 * @return       результат декомпрессии
			 */
			vector <char> decompressDeflate(const char * buffer, const size_t size) const noexcept;
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
			 _wbit(MAX_WBITS), _rounds(5), levelGzip(Z_DEFAULT_COMPRESSION),
			 _salt(""), _pass(""), _takeOverCompress(false), _takeOverDecompress(false),
			 _btype{0x00, 0x00, 0xFF, 0xFF}, _cipher(cipher_t::AES128), _key{}, _zinf{0}, _zdef{0}, _log(log) {}
			/**
			 * ~Hash Деструктор
			 */
			~Hash() noexcept;
	} hash_t;
};

#endif // __AWH_HASH__
