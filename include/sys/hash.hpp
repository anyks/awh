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
 * @copyright: Copyright © 2025
 */

#ifndef __AWH_HASH__
#define __AWH_HASH__

/**
 * Стандартные модули
 */
#include <ctime>
#include <string>
#include <vector>
#include <cstdio>
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
 * Подключаем Zstandard
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
#include <openssl/sha.h>
#include <openssl/aes.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

/**
 * Наши модули
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>

// Параметры Zlib
#define MOD_GZIP_ZLIB_CFACTOR 9
#define MOD_GZIP_ZLIB_BSIZE 8096

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * Hash Класс хеширования данных
	 */
	typedef class AWHSHARED_EXPORT Hash {
		public:
			/**
			 * State Стрейт шифрования
			 */
			typedef struct State {
				// Количество обработанных байт
				int32_t num;
				// Ключ шифрования
				AES_KEY key;
				// Буфер данных для шифрования
				u_char ivec[AES_BLOCK_SIZE];
				/**
				 * State Конструктор
				 */
				State() noexcept : num(0), ivec{0} {}
			} state_t;
		public:
			/**
			 * События выполнения операции
			 */
			enum class event_t : uint8_t {
				NONE   = 0x00, // Событие не установленно
				ENCODE = 0x01, // Кодирование данных
				DECODE = 0x02  // Декодирование данных
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
			/**
			 * Набор размеров шифрования
			 */
			enum class cipher_t : uint16_t {
				NONE   = 0,   // Размер шифрования не установлен
				BASE64 = 64,  // Шиффрование в BASE64
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
				ZSTD    = 0x03, // Метод сжатия Zstandard
				GZIP    = 0x04, // Метод сжатия GZip
				BZIP2   = 0x05, // Метод сжания BZip2
				BROTLI  = 0x06, // Метод сжатия Brotli
				DEFLATE = 0x07  // Метод сжатия Deflate
			};
			/**
			 * type_t Тип хэш-суммы
			 */
			enum class type_t : uint8_t {
				NONE   = 0x00, // Не установлено
				MD5    = 0x01, // Хэш MD5
				SHA1   = 0x02, // Хэш SHA1
				SHA224 = 0x03, // Хэш SHA224
				SHA256 = 0x04, // Хэш SHA256
				SHA384 = 0x05, // Хэш SHA384
				SHA512 = 0x06  // Хэш SHA512
			};
		private:
			// Стейт шифрования
			state_t _state;
		private:
			// Размер скользящего окна
			int16_t _wbit;
			// Устанавливаем количество раундов
			int32_t _rounds;
		private:
			// Уровни компрессии
			uint32_t _level[3];
		private:
			// Соль и пароль для шифрования
			string _salt, _password;
		private:
			// Флаг переиспользования контекста компрессии
			bool _takeOverCompress;
			// Флаг переиспользования контекста декомпрессии
			bool _takeOverDecompress;
		private:
			// Хвостовой буфер для удаления из финального сообщения
			const char _btype[4];
		private:
			// Создаем поток ZLib для декомпрессии
			mutable z_stream _zinf;
			// Создаем поток ZLib для компрессии
			mutable z_stream _zdef;
		private:
			// Объект работы с логами
			const log_t * _log;
		private:
			/**
			 * cipher Метод инициализации AES шифрования
			 * @param cipher тип шифрования (AES128, AES192, AES256)
			 * @return       результат инициализации
			 */
			bool cipher(const cipher_t cipher) noexcept;
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
		public:
			/**
			 * Шаблон метода хэширования текста
			 * @tparam T тип возвращаемого результата
			 */
			template <typename T>
			/**
			 * hashing Метод хэширования текста
			 * @param text текст для хэширования
			 * @param type тип хэш-суммы
			 * @return     результат хэширования
			 */
			auto hashing(const string & text, const type_t type) const noexcept -> T {
				// Результат работы функции
				T result;
				// Если текст передан
				if(!text.empty())
					// Выполняем хэширование
					this->hashing(text, type, result);
				// Выводим результат
				return result;
			}
		public:
			/**
			 * hashing Метод хэширования текста
			 * @param text   текст для хэширования
			 * @param type   тип хэш-суммы
			 * @param result строка куда следует положить результат
			 */
			void hashing(const string & text, const type_t type, string & result) const noexcept;
			/**
			 * hash Метод хэширования текста
			 * @param text   текст для хэширования
			 * @param type   тип хэш-суммы
			 * @param result буфер куда следует положить результат
			 */
			void hashing(const string & text, const type_t type, vector <char> & result) const noexcept;
		public:
			/**
			 * Шаблон метода хэширования текста с ключом
			 * @tparam T тип возвращаемого результата
			 */
			template <typename T>
			/**
			 * hmac Метод хэширования текста с ключом
			 * @param key  ключ для подписи
			 * @param text текст для хэширования
			 * @param type тип хэш-суммы
			 * @return     результат хэширования
			 */
			auto hmac(const string & key, const string & text, const type_t type) const noexcept -> T {
				// Результат работы функции
				T result;
				// Если текст передан
				if(!text.empty())
					// Выполняем хэширование
					this->hmac(key, text, type, result);
				// Выводим результат
				return result;
			}
		public:
			/**
			 * hmac Метод хэширования текста с ключом
			 * @param key    ключ для подписи
			 * @param text   текст для хэширования
			 * @param type   тип хэш-суммы
			 * @param result строка куда следует положить результат
			 */
			void hmac(const string & key, const string & text, const type_t type, string & result) const noexcept;
			/**
			 * hmac Метод хэширования текста с ключом
			 * @param key    ключ для подписи
			 * @param text   текст для хэширования
			 * @param type   тип хэш-суммы
			 * @param result буфер куда следует положить результат
			 */
			void hmac(const string & key, const string & text, const type_t type, vector <char> & result) const noexcept;
		public:
			/**
			 * Шаблон метода кодирования
			 * @tparam T тип возвращаемого результата
			 */
			template <typename T>
			/**
			 * encode Метод кодирования
			 * @param buffer буфер данных для шифрования
			 * @param size   размер данных для шифрования
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @return       результат кодирования
			 */
			auto encode(const char * buffer, const size_t size, const cipher_t cipher) const noexcept -> T {
				// Результат работы функции
				T result;
				// Если буфер данных передан
				if((buffer != nullptr) && (size > 0))
					// Выполняем кодирование
					this->encode(buffer, size, cipher, result);
				// Выводим результат
				return result;
			}
			/**
			 * Шаблон метода декодирования
			 * @tparam T тип возвращаемого результата
			 */
			template <typename T>
			/**
			 * decode Метод декодирования
			 * @param buffer буфер данных для шифрования
			 * @param size   размер данных для шифрования
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @return       результат кодирования
			 */
			auto decode(const char * buffer, const size_t size, const cipher_t cipher) const noexcept -> T {
				// Результат работы функции
				T result;
				// Если буфер данных передан
				if((buffer != nullptr) && (size > 0))
					// Выполняем декодирование
					this->decode(buffer, size, cipher, result);
				// Выводим результат
				return result;
			}
		public:
			/**
			 * encode Метод кодирования
			 * @param buffer буфер данных для шифрования
			 * @param size   размер данных для шифрования
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @param result строка куда следует положить результат
			 */
			void encode(const char * buffer, const size_t size, const cipher_t cipher, string & result) const noexcept;
			/**
			 * encode Метод кодирования
			 * @param buffer буфер данных для шифрования
			 * @param size   размер данных для шифрования
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @param result буфер куда следует положить результат
			 */
			void encode(const char * buffer, const size_t size, const cipher_t cipher, vector <char> & result) const noexcept;
		public:
			/**
			 * decode Метод декодирования
			 * @param buffer буфер данных для шифрования
			 * @param size   размер данных для шифрования
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @param result строка куда следует положить результат
			 */
			void decode(const char * buffer, const size_t size, const cipher_t cipher, string & result) const noexcept;
			/**
			 * decode Метод декодирования
			 * @param buffer буфер данных для шифрования
			 * @param size   размер данных для шифрования
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @param result буфер куда следует положить результат
			 */
			void decode(const char * buffer, const size_t size, const cipher_t cipher, vector <char> & result) const noexcept;
		public:
			/**
			 * Шаблон метода декодирования
			 * @tparam T тип возвращаемого результата
			 */
			template <typename T>
			/**
			 * compress Метод компрессии данных
			 * @param buffer буфер данных для компрессии
			 * @param size   размер данных для компрессии
			 * @param method метод компрессии
			 * @return       результат компрессии
			 */
			auto compress(const char * buffer, const size_t size, const method_t method) const noexcept -> T {
				// Результат работы функции
				T result;
				// Если буфер данных передан
				if((buffer != nullptr) && (size > 0))
					// Выполняем кодирование
					this->compress(buffer, size, method, result);
				// Выводим результат
				return result;
			}
			/**
			 * Шаблон метода декодирования
			 * @tparam T тип возвращаемого результата
			 */
			template <typename T>
			/**
			 * decompress Метод декомпрессии данных
			 * @param buffer буфер данных для декомпрессии
			 * @param size   размер данных для декомпрессии
			 * @param method метод компрессии
			 * @return       результат декомпрессии
			 */
			auto decompress(const char * buffer, const size_t size, const method_t method) const noexcept -> T {
				// Результат работы функции
				T result;
				// Если буфер данных передан
				if((buffer != nullptr) && (size > 0))
					// Выполняем декомпрессию
					this->decompress(buffer, size, method, result);
				// Выводим результат
				return result;
			}
		public:
			/**
			 * compress Метод компрессии данных
			 * @param buffer буфер данных для компрессии
			 * @param size   размер данных для компрессии
			 * @param method метод компрессии
			 * @param result строка куда следует положить результат
			 */
			void compress(const char * buffer, const size_t size, const method_t method, string & result) const noexcept;
			/**
			 * compress Метод компрессии данных
			 * @param buffer буфер данных для компрессии
			 * @param size   размер данных для компрессии
			 * @param method метод компрессии
			 * @param result буфер куда следует положить результат
			 */
			void compress(const char * buffer, const size_t size, const method_t method, vector <char> & result) const noexcept;
		public:
			/**
			 * decompress Метод декомпрессии данных
			 * @param buffer буфер данных для декомпрессии
			 * @param size   размер данных для декомпрессии
			 * @param method метод компрессии
			 * @param result строка куда следует положить результат
			 */
			void decompress(const char * buffer, const size_t size, const method_t method, string & result) const noexcept;
			/**
			 * decompress Метод декомпрессии данных
			 * @param buffer буфер данных для декомпрессии
			 * @param size   размер данных для декомпрессии
			 * @param method метод компрессии
			 * @param result буфер куда следует положить результат
			 */
			void decompress(const char * buffer, const size_t size, const method_t method, vector <char> & result) const noexcept;
		public:
			/**
			 * wbit Метод установки размера скользящего окна
			 * @param wbit размер скользящего окна
			 */
			void wbit(const int16_t wbit) noexcept;
			/**
			 * round Метод установки количества раундов шифрования
			 * @param round количество раундов шифрования
			 */
			void round(const int32_t round) noexcept;
			/**
			 * level Метод установки уровня компрессии
			 * @param level уровень компрессии
			 */
			void level(const level_t level) noexcept;
		public:
			/**
			 * salt Метод установки соли шифрования
			 * @param salt соль для шифрования
			 */
			void salt(const string & salt) noexcept;
			/**
			 * password Метод установки пароля шифрования
			 * @param password пароль шифрования
			 */
			void password(const string & password) noexcept;
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
			 _wbit(MAX_WBITS), _rounds(5),
			 _level{1, Z_DEFAULT_COMPRESSION, ZSTD_CLEVEL_DEFAULT},
			 _salt{""}, _password{""}, _takeOverCompress(false), _takeOverDecompress(false),
			 _btype{0x00, 0x00, 0xFF, 0xFF}, _zinf({0}), _zdef({0}), _log(log) {}
			/**
			 * ~Hash Деструктор
			 */
			~Hash() noexcept;
	} hash_t;
};

#endif // __AWH_HASH__
