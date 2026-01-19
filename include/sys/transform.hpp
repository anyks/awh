/**
 * @file: transform.hpp
 * @date: 2026-01-18
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

#ifndef __AWH_TRANSFORM__
#define __AWH_TRANSFORM__

/**
 * Стандартные модули
 */
#include <any>
#include <array>
#include <atomic>
#include <string>
#include <vector>

/**
 * Наши модули
 */
#include "fmk.hpp"
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
	 * @brief Класс трансформации данных
	 *
	 */
	typedef class AWH_SHARED_EXPORT Transform {
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
			 * @brief Набор размеров шифрования
			 *
			 */
			enum class cipher_t : uint16_t {
				NONE   = 0,   // Размер шифрования не установлен
				BASE64 = 64,  // Шиффрование в BASE64
				AES128 = 128, // Размер шифрования 128 бит
				AES192 = 192, // Размер шифрования 192 бит
				AES256 = 256  // Размер шифрования 256 бит
			};
			/**
			 * @brief Тип хэш-суммы
			 *
			 */
			enum class hash_t : uint8_t {
				NONE   = 0x00, // Не установлено
				MD5    = 0x01, // Хэш MD5
				SHA1   = 0x02, // Хэш SHA1
				SHA224 = 0x03, // Хэш SHA224
				SHA256 = 0x04, // Хэш SHA256
				SHA384 = 0x05, // Хэш SHA384
				SHA512 = 0x06  // Хэш SHA512
			};
			/**
			 * @brief Типы компрессоров
			 *
			 */
			enum class compressor_t : uint8_t {
				NONE    = 0x00, // Метод сжатия не установлен
				LZ4     = 0x01, // Метод сжатия Lz4
				LZMA    = 0x02, // Метод сжатия LZma
				ZSTD    = 0x03, // Метод сжатия ZStd
				GZIP    = 0x04, // Метод сжатия GZip
				BZIP2   = 0x05, // Метод сжания BZip2
				BROTLI  = 0x06, // Метод сжатия Brotli
				LIZARD  = 0x07, // Метод сжатия Lizard
				SNAPPY  = 0x08, // Метод сжатия Snappy
				DEFLATE = 0x09, // Метод сжатия Deflate
				DENSITY = 0x0A  // Метод сжатия Density
			};
		public:
			/**
			 * @brief Тип 128-битного хэша
			 *
			 */
			using uint128_t = std::array <uint8_t, 16>;
		public:
			/**
			 * @brief Стрейт шифрования
			 *
			 */
			typedef struct CryptoState {
				// Количество обработанных байт
				int32_t num;
				// Ключ шифрования
				std::any key;
				// Буфер данных для шифрования
				uint8_t ivec[16];
				/**
				 * @brief Конструктор
				 *
				 */
				CryptoState() noexcept : num(0), ivec{0} {}
			} crypto_state_t;
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
				Takeover() noexcept :
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
				GZip() noexcept : wbits(0) {}
			} gzip_t;
			/**
			 * @brief Структура криптографии
			 *
			 */
			typedef struct Crypto {
				// Количество раундов шифрования
				int32_t rounds;
				// Стейт шифрования AES
				crypto_state_t aes;
				// Соль шифрования
				string salt;
				// Пароль шифрования
				string password;
				/**
				 * @brief Конструктор
				 *
				 */
				Crypto() noexcept :
				 rounds(5), salt{""}, password{""} {}
			} crypto_t;
		private:
			// Уровни компрессии
			uint32_t _level[5];
		private:
			// Структура GZip
			mutable gzip_t _gzip;
			// Структура криптографии
			mutable crypto_t _crypto;
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
		private:
			/**
			 * @brief Метод инициализации AES шифрования
			 *
			 * @param cipher тип шифрования (AES128, AES192, AES256)
			 * @return       результат инициализации
			 */
			bool cipher(const cipher_t cipher) noexcept;
			/**
			 * @brief Метод установки количества раундов шифрования
			 *
			 * @param round количество раундов шифрования
			 */
			void roundAES(const int32_t round) noexcept;
		public:
			/**
			 * @brief Метод установки соли шифрования
			 *
			 * @param salt соль для шифрования
			 */
			void salt(const string & salt) noexcept;
			/**
			 * @brief Метод установки пароля шифрования
			 *
			 * @param password пароль шифрования
			 */
			void password(const string & password) noexcept;
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
			 */
			void wbitsGZip(const int16_t wbits) noexcept;
			/**
			 * @brief Метод включения/отключения флага переиспользования контекста компрессии/декомпрессии
			 *
			 * @param event событие выполнения операции
			 * @param flag  флаг переиспользования контекста компрессии/декомпрессии
			 */
			void takeoverGZip(const event_t event, const bool flag) noexcept;
		public:
			/**
			 * @brief Метод преобразования 128-битного хэша в 64-битный
			 *
			 * @param hash 128-битный хэш
			 * @return     64-битный хэш
			 */
			uint64_t hash128to64(const uint128_t & hash) const noexcept;
		public:
			/**
			 * @brief Шаблон метода хэширования текста
			 *
			 * @tparam T тип возвращаемого результата
			 */
			template <typename T>
			/**
			 * @brief Метод хэширования текста
			 *
			 * @param text текст для хэширования
			 * @return     результат хэширования
			 */
			auto hashing(const string & text) const noexcept -> T;
			/**
			 * @brief Шаблон метода хэширования текста
			 *
			 * @tparam T тип возвращаемого результата
			 */
			template <typename T>
			/**
			 * @brief Метод хэширования текста
			 *
			 * @param buffer буфер данных для хэширования
			 * @return       результат хэширования
			 */
			auto hashing(const vector <char> & buffer) const noexcept -> T;
			/**
			 * @brief Шаблон метода хэширования текста
			 *
			 * @tparam T тип возвращаемого результата
			 */
			template <typename T>
			/**
			 * @brief Метод хэширования текста
			 *
			 * @param buffer буфер данных для хэширования
			 * @return       результат хэширования
			 */
			auto hashing(const vector <uint8_t> & buffer) const noexcept -> T;
			/**
			 * @brief Шаблон метода хэширования текста
			 *
			 * @tparam T тип возвращаемого результата
			 */
			template <typename T>
			/**
			 * @brief Метод хэширования текста
			 *
			 * @param buffer буфер данных для хэширования
			 * @param size   размер данных для хэширования
			 * @return       результат хэширования
			 */
			auto hashing(const void * buffer, const size_t size) const noexcept -> T;
		public:
			/**
			 * @brief Шаблон метода хэширования текста c ключом
			 *
			 * @tparam T тип возвращаемого результата
			 */
			template <typename T>
			/**
			 * @brief Метод хэширования текста c ключом
			 *
			 * @param text текст для хэширования
			 * @param seed ключ для хэширования
			 * @return     результат хэширования
			 */
			auto hashing(const string & text, const T seed) const noexcept -> T;
			/**
			 * @brief Шаблон метода хэширования текста c ключом
			 *
			 * @tparam T тип возвращаемого результата
			 */
			template <typename T>
			/**
			 * @brief Метод хэширования текста c ключом
			 *
			 * @param buffer буфер данных для хэширования
			 * @param seed   ключ для хэширования
			 * @return       результат хэширования
			 */
			auto hashing(const vector <char> & buffer, const T seed) const noexcept -> T;
			/**
			 * @brief Шаблон метода хэширования текста c ключом
			 *
			 * @tparam T тип возвращаемого результата
			 */
			template <typename T>
			/**
			 * @brief Метод хэширования текста c ключом
			 *
			 * @param buffer буфер данных для хэширования
			 * @param seed   ключ для хэширования
			 * @return       результат хэширования
			 */
			auto hashing(const vector <uint8_t> & buffer, const T seed) const noexcept -> T;
			/**
			 * @brief Шаблон метода хэширования текста c ключом
			 *
			 * @tparam T тип возвращаемого результата
			 */
			template <typename T>
			/**
			 * @brief Метод хэширования текста c ключом
			 *
			 * @param buffer буфер данных для хэширования
			 * @param size   размер данных для хэширования
			 * @param seed   ключ для хэширования
			 * @return       результат хэширования
			 */
			auto hashing(const void * buffer, const size_t size, const T seed) const noexcept -> T;
		public:
			/**
			 * @brief Шаблон метода хэширования текста c ключом
			 *
			 * @tparam T тип возвращаемого результата
			 */
			template <typename T>
			/**
			 * @brief Метод хэширования текста c ключом
			 *
			 * @param text  текст для хэширования
			 * @param seed1 первый ключ для хэширования
			 * @param seed2 второй ключ для хэширования
			 * @return      результат хэширования
			 */
			auto hashing(const string & text, const T seed1, const T seed2) const noexcept -> T;
			/**
			 * @brief Шаблон метода хэширования текста c ключом
			 *
			 * @tparam T тип возвращаемого результата
			 */
			template <typename T>
			/**
			 * @brief Метод хэширования текста c ключом
			 *
			 * @param buffer буфер данных для хэширования
			 * @param seed1  первый ключ для хэширования
			 * @param seed2  второй ключ для хэширования
			 * @return       результат хэширования
			 */
			auto hashing(const vector <char> & buffer, const T seed1, const T seed2) const noexcept -> T;
			/**
			 * @brief Шаблон метода хэширования текста c ключом
			 *
			 * @tparam T тип возвращаемого результата
			 */
			template <typename T>
			/**
			 * @brief Метод хэширования текста c ключом
			 *
			 * @param buffer буфер данных для хэширования
			 * @param seed1  первый ключ для хэширования
			 * @param seed2  второй ключ для хэширования
			 * @return       результат хэширования
			 */
			auto hashing(const vector <uint8_t> & buffer, const T seed1, const T seed2) const noexcept -> T;
			/**
			 * @brief Шаблон метода хэширования текста c ключом
			 *
			 * @tparam T тип возвращаемого результата
			 */
			template <typename T>
			/**
			 * @brief Метод хэширования текста c ключом
			 *
			 * @param buffer буфер данных для хэширования
			 * @param size   размер данных для хэширования
			 * @param seed1  первый ключ для хэширования
			 * @param seed2  второй ключ для хэширования
			 * @return       результат хэширования
			 */
			auto hashing(const void * buffer, const size_t size, const T seed1, const T seed2) const noexcept -> T;
		public:
			/**
			 * @brief Шаблон метода хэширования текста
			 *
			 * @tparam T тип возвращаемого результата
			 */
			template <typename T>
			/**
			 * @brief Метод хэширования текста
			 *
			 * @param text текст для хэширования
			 * @param hash тип хэш-суммы
			 * @return     результат хэширования
			 */
			auto hashing(const string & text, const hash_t hash) const noexcept -> T;
		public:
			/**
			 * @brief Метод хэширования текста
			 *
			 * @param text   текст для хэширования
			 * @param hash   тип хэш-суммы
			 * @param result строка куда следует положить результат
			 */
			void hashing(const string & text, const hash_t hash, string & result) const noexcept;
			/**
			 * @brief Метод хэширования текста
			 *
			 * @param text   текст для хэширования
			 * @param hash   тип хэш-суммы
			 * @param result буфер куда следует положить результат
			 */
			void hashing(const string & text, const hash_t hash, vector <char> & result) const noexcept;
			/**
			 * @brief Метод хэширования текста
			 *
			 * @param text   текст для хэширования
			 * @param hash   тип хэш-суммы
			 * @param result буфер куда следует положить результат
			 */
			void hashing(const string & text, const hash_t hash, vector <uint8_t> & result) const noexcept;
		public:
			/**
			 * @brief Шаблон метода хэширования текста с ключом
			 *
			 * @tparam T тип возвращаемого результата
			 */
			template <typename T>
			/**
			 * @brief Метод хэширования текста с ключом
			 *
			 * @param key  ключ для подписи
			 * @param text текст для хэширования
			 * @param hash тип хэш-суммы
			 * @return     результат хэширования
			 */
			auto hmac(const string & key, const string & text, const hash_t hash) const noexcept -> T;
		public:
			/**
			 * @brief Метод хэширования текста с ключом
			 *
			 * @param key    ключ для подписи
			 * @param text   текст для хэширования
			 * @param hash   тип хэш-суммы
			 * @param result строка куда следует положить результат
			 */
			void hmac(const string & key, const string & text, const hash_t hash, string & result) const noexcept;
			/**
			 * @brief Метод хэширования текста с ключом
			 *
			 * @param key    ключ для подписи
			 * @param text   текст для хэширования
			 * @param hash   тип хэш-суммы
			 * @param result буфер куда следует положить результат
			 */
			void hmac(const string & key, const string & text, const hash_t hash, vector <char> & result) const noexcept;
			/**
			 * @brief Метод хэширования текста с ключом
			 *
			 * @param key    ключ для подписи
			 * @param text   текст для хэширования
			 * @param hash   тип хэш-суммы
			 * @param result буфер куда следует положить результат
			 */
			void hmac(const string & key, const string & text, const hash_t hash, vector <uint8_t> & result) const noexcept;
		public:
			/**
			 * @brief Шаблон метода кодирования
			 *
			 * @tparam A тип возвращаемого результата
			 * @tparam B тип буфера данных
			 */
			template <typename A, typename B>
			/**
			 * @brief Метод кодирования
			 *
			 * @param buffer буфер данных для шифрования
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @return       результат кодирования
			 */
			auto encode(const B & buffer, const cipher_t cipher) const noexcept -> A;
			/**
			 * @brief Шаблон метода кодирования
			 *
			 * @tparam T тип возвращаемого результата
			 */
			template <typename T>
			/**
			 * @brief Метод кодирования
			 *
			 * @param buffer буфер данных для шифрования
			 * @param size   размер данных для шифрования
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @return       результат кодирования
			 */
			auto encode(const void * buffer, const size_t size, const cipher_t cipher) const noexcept -> T;
		public:
			/**
			 * @brief Метод кодирования
			 *
			 * @param buffer буфер данных для шифрования
			 * @param size   размер данных для шифрования
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @param result строка куда следует положить результат
			 */
			void encode(const void * buffer, const size_t size, const cipher_t cipher, string & result) const noexcept;
			/**
			 * @brief Метод кодирования
			 *
			 * @param buffer буфер данных для шифрования
			 * @param size   размер данных для шифрования
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @param result буфер куда следует положить результат
			 */
			void encode(const void * buffer, const size_t size, const cipher_t cipher, vector <char> & result) const noexcept;
			/**
			 * @brief Метод кодирования
			 *
			 * @param buffer буфер данных для шифрования
			 * @param size   размер данных для шифрования
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @param result буфер куда следует положить результат
			 */
			void encode(const void * buffer, const size_t size, const cipher_t cipher, vector <uint8_t> & result) const noexcept;
		public:
			/**
			 * @brief Шаблон метода декодирования
			 *
			 * @tparam A тип возвращаемого результата
			 * @tparam B тип буфера данных
			 */
			template <typename A, typename B>
			/**
			 * @brief Метод декодирования
			 *
			 * @param buffer буфер данных для шифрования
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @return       результат кодирования
			 */
			auto decode(const B & buffer, const cipher_t cipher) const noexcept -> A;
			/**
			 * @brief Шаблон метода декодирования
			 *
			 * @tparam T тип возвращаемого результата
			 */
			template <typename T>
			/**
			 * @brief Метод декодирования
			 *
			 * @param buffer буфер данных для шифрования
			 * @param size   размер данных для шифрования
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @return       результат кодирования
			 */
			auto decode(const void * buffer, const size_t size, const cipher_t cipher) const noexcept -> T;
		public:
			/**
			 * @brief Метод декодирования
			 *
			 * @param buffer буфер данных для шифрования
			 * @param size   размер данных для шифрования
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @param result строка куда следует положить результат
			 */
			void decode(const void * buffer, const size_t size, const cipher_t cipher, string & result) const noexcept;
			/**
			 * @brief Метод декодирования
			 *
			 * @param buffer буфер данных для шифрования
			 * @param size   размер данных для шифрования
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @param result буфер куда следует положить результат
			 */
			void decode(const void * buffer, const size_t size, const cipher_t cipher, vector <char> & result) const noexcept;
			/**
			 * @brief Метод декодирования
			 *
			 * @param buffer буфер данных для шифрования
			 * @param size   размер данных для шифрования
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @param result буфер куда следует положить результат
			 */
			void decode(const void * buffer, const size_t size, const cipher_t cipher, vector <uint8_t> & result) const noexcept;
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
			 * @param buffer     буфер данных для компрессии
			 * @param compressor метод компрессии
			 * @return           результат компрессии
			 */
			auto compress(const B & buffer, const compressor_t compressor) const noexcept -> A;
			/**
			 * @brief Шаблон метода компрессии данных
			 *
			 * @tparam T тип возвращаемого результата
			 */
			template <typename T>
			/**
			 * @brief Метод компрессии данных
			 *
			 * @param buffer     буфер данных для компрессии
			 * @param size       размер данных для компрессии
			 * @param compressor метод компрессии
			 * @return           результат компрессии
			 */
			auto compress(const void * buffer, const size_t size, const compressor_t compressor) const noexcept -> T;
		public:
			/**
			 * @brief Метод компрессии данных
			 *
			 * @param buffer     буфер данных для компрессии
			 * @param size       размер данных для компрессии
			 * @param compressor метод компрессии
			 * @param result     строка куда следует положить результат
			 */
			void compress(const void * buffer, const size_t size, const compressor_t compressor, string & result) const noexcept;
			/**
			 * @brief Метод компрессии данных
			 *
			 * @param buffer     буфер данных для компрессии
			 * @param size       размер данных для компрессии
			 * @param compressor метод компрессии
			 * @param result     буфер куда следует положить результат
			 */
			void compress(const void * buffer, const size_t size, const compressor_t compressor, vector <char> & result) const noexcept;
			/**
			 * @brief Метод компрессии данных
			 *
			 * @param buffer     буфер данных для компрессии
			 * @param size       размер данных для компрессии
			 * @param compressor метод компрессии
			 * @param result     буфер куда следует положить результат
			 */
			void compress(const void * buffer, const size_t size, const compressor_t compressor, vector <uint8_t> & result) const noexcept;
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
			 * @param buffer     буфер данных для декомпрессии
			 * @param compressor метод компрессии
			 * @return           результат декомпрессии
			 */
			auto decompress(const B & buffer, const compressor_t compressor) const noexcept -> A;
			/**
			 * @brief Шаблон метода декомпрессии данных
			 *
			 * @tparam T тип возвращаемого результата
			 */
			template <typename T>
			/**
			 * @brief Метод декомпрессии данных
			 *
			 * @param buffer     буфер данных для декомпрессии
			 * @param size       размер данных для декомпрессии
			 * @param compressor метод компрессии
			 * @return           результат декомпрессии
			 */
			auto decompress(const void * buffer, const size_t size, const compressor_t compressor) const noexcept -> T;
		public:
			/**
			 * @brief Метод декомпрессии данных
			 *
			 * @param buffer     буфер данных для декомпрессии
			 * @param size       размер данных для декомпрессии
			 * @param compressor метод компрессии
			 * @param result     строка куда следует положить результат
			 */
			void decompress(const void * buffer, const size_t size, const compressor_t compressor, string & result) const noexcept;
			/**
			 * @brief Метод декомпрессии данных
			 *
			 * @param buffer     буфер данных для декомпрессии
			 * @param size       размер данных для декомпрессии
			 * @param compressor метод компрессии
			 * @param result     буфер куда следует положить результат
			 */
			void decompress(const void * buffer, const size_t size, const compressor_t compressor, vector <char> & result) const noexcept;
			/**
			 * @brief Метод декомпрессии данных
			 *
			 * @param buffer     буфер данных для декомпрессии
			 * @param size       размер данных для декомпрессии
			 * @param compressor метод компрессии
			 * @param result     буфер куда следует положить результат
			 */
			void decompress(const void * buffer, const size_t size, const compressor_t compressor, vector <uint8_t> & result) const noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param log объект для работы с логами
			 */
			Transform(const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Transform() noexcept;
	} transform_t;
};

#endif // __AWH_TRANSFORM__
