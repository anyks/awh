/**
 * @file: crypto.hpp
 * @date: 2026-01-20
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

#ifndef __AWH_CRYPTO__
#define __AWH_CRYPTO__

/**
 * Стандартные модули
 */
#include <array>
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
	 * @brief Класс криптографии
	 *
	 */
	typedef class AWH_SHARED_EXPORT Crypto {
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
			 * @brief Типы ключей RSA
			 *
			 */
			enum class key_type_t : uint8_t {
				NONE    = 0x00, // Тип ключа не установлен
				PUBLIC  = 0x01, // Публичный ключ
				PRIVATE = 0x02  // Приватный ключ
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
		public:
			/**
			 * @brief Тип 128-битного хэша
			 *
			 */
			using uint128_t = std::array <uint8_t, 16>;
		public:
			/**
			 * @brief Стейт шифрования
			 *
			 */
			typedef struct State {
				// Тип хэш-суммы
				hash_t hash;
				// Тип шифрования
				cipher_t cipher;
				// Контекст выбранного шифра
				const void * evp;
				// Контекст шифрования
				const void * ctx;
				// Ключ шифрования
				vector <uint8_t> key;
				// Вектор инициализации
				vector <uint8_t> ivec;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit State() noexcept :
				 hash(hash_t::NONE),
				 cipher(cipher_t::NONE),
				 evp(nullptr), ctx(nullptr) {}
			} state_t;
		private:
			/**
			 * @brief Структура ключа RSA
			 *
			 */
			typedef struct Key {
				// Контекст RSA ключа
				void * ctx;
				// Тип RSA ключа
				key_type_t type;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Key() noexcept :
				 ctx(nullptr), type(key_type_t::NONE) {}
			} key_rsa_t;
		private:
			// Стейт AES шифрования
			state_t _state;
			// Объект RSA ключа
			key_rsa_t _key;
		private:
			// Количество раундов шифрования
			int32_t _rounds;
		private:
			// Соль шифрования
			string _salt;
			// Пароль шифрования
			string _password;
		private:
			// Локер для потокобезопасной работы
			mutable lock_state_t <std::mutex> _mtx;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		public:
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
			 * @tparam A тип возвращаемого результата
			 * @tparam B тип буфера данных
			 */
			template <typename A, typename B>
			/**
			 * @brief Метод хэширования текста
			 *
			 * @param text текст для хэширования
			 * @return     результат хэширования
			 */
			auto hashing(const B & text) const noexcept -> A;
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
			 * @tparam A тип возвращаемого результата
			 * @tparam B тип буфера данных
			 */
			template <typename A, typename B>
			/**
			 * @brief Метод хэширования текста c ключом
			 *
			 * @param text текст для хэширования
			 * @param seed ключ для хэширования
			 * @return     результат хэширования
			 */
			auto hashingWithSeed(const B & text, const A seed) const noexcept -> A;
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
			auto hashingWithSeed(const void * buffer, const size_t size, const T seed) const noexcept -> T;
		public:
			/**
			 * @brief Шаблон метода хэширования текста c несколькими ключами
			 *
			 * @tparam A тип возвращаемого результата
			 * @tparam B тип буфера данных
			 */
			template <typename A, typename B>
			/**
			 * @brief Метод хэширования текста c несколькими ключами
			 *
			 * @param text  текст для хэширования
			 * @param seed1 первый ключ для хэширования
			 * @param seed2 второй ключ для хэширования
			 * @return      результат хэширования
			 */
			auto hashingWithSeeds(const B & text, const A seed1, const A seed2) const noexcept -> A;
			/**
			 * @brief Шаблон метода хэширования текста c несколькими ключами
			 *
			 * @tparam T тип возвращаемого результата
			 */
			template <typename T>
			/**
			 * @brief Метод хэширования текста c несколькими ключами
			 *
			 * @param buffer буфер данных для хэширования
			 * @param size   размер данных для хэширования
			 * @param seed1  первый ключ для хэширования
			 * @param seed2  второй ключ для хэширования
			 * @return       результат хэширования
			 */
			auto hashingWithSeeds(const void * buffer, const size_t size, const T seed1, const T seed2) const noexcept -> T;
		public:
			/**
			 * @brief Шаблон метода хэширования текста
			 *
			 * @tparam A тип возвращаемого результата
			 * @tparam B тип буфера данных
			 */
			template <typename A, typename B>
			/**
			 * @brief Метод хэширования текста
			 *
			 * @param buffer буфер данных для хэширования
			 * @param hash   тип хэш-суммы
			 * @return       результат хэширования
			 */
			auto hashing(const B & buffer, const hash_t hash) const noexcept -> A;
		public:
			/**
			 * @brief Шаблон метода хэширования текста с ключом
			 *
			 * @tparam A тип возвращаемого результата
			 * @tparam B тип буфера данных
			 */
			template <typename A, typename B>
			/**
			 * @brief Метод хэширования текста с ключом
			 *
			 * @param key    ключ для подписи
			 * @param buffer буфер данных для хэширования
			 * @param hash   тип хэш-суммы
			 * @return       результат хэширования
			 */
			auto hmac(const string & key, const B & buffer, const hash_t hash) const noexcept -> A;
		public:
			/**
			 * @brief Шаблон метода кодирования
			 *
			 * @tparam T тип возвращаемого результата
			 */
			template <typename T>
			/**
			 * @brief Метод финализации контекста шифрования
			 *
			 * @param buffer буфер куда следует положить результат
			 * @return       результат финализации
			 */
			bool finalize(T & buffer) noexcept;
			/**
			 * @brief Метод инициализации контекста шифрования
			 *
			 * @param event  событие шифрования (ENCODE, DECODE)
			 * @param hash   тип хэш-суммы
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @return       результат инициализации
			 */
			bool initialize(const event_t event, const hash_t hash, const cipher_t cipher) noexcept;
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
			 * @param hash   тип хэш-суммы
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @return       результат кодирования
			 */
			auto encode(const B & buffer, const hash_t hash = hash_t::NONE, const cipher_t cipher = cipher_t::NONE) const noexcept -> A;
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
			 * @param hash   тип хэш-суммы
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @return       результат кодирования
			 */
			auto encode(const void * buffer, const size_t size, const hash_t hash = hash_t::NONE, const cipher_t cipher = cipher_t::NONE) const noexcept -> T;
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
			 * @param hash   тип хэш-суммы
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @return       результат кодирования
			 */
			auto decode(const B & buffer, const hash_t hash = hash_t::NONE, const cipher_t cipher = cipher_t::NONE) const noexcept -> A;
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
			 * @param hash   тип хэш-суммы
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @return       результат кодирования
			 */
			auto decode(const void * buffer, const size_t size, const hash_t hash = hash_t::NONE, const cipher_t cipher = cipher_t::NONE) const noexcept -> T;
		public:
			/**
			 * @brief Метод генерации приватного ключа RSA
			 *
			 * @param size размер ключа в битах (2048, 3072, 4096)
			 * @return     результат генерации ключа
			 */
			bool generatePrivateKeyRSA(const size_t size = 0) noexcept;
		public:
			/**
			 * @brief Метод получения публичного ключа RSA
			 *
			 * @return публичный ключ RSA
			 */
			string getPublicKeyRSA() const noexcept;
			/**
			 * @brief Метод установки публичного ключа RSA
			 *
			 * @param key публичный ключ RSA
			 * @return    результат установки ключа
			 */
			bool setPublicKeyRSA(const string & key) noexcept;
		public:
			/**
			 * @brief Метод установки приватного ключа RSA
			 *
			 * @param key приватный ключ RSA
			 * @return    результат установки ключа
			 */
			bool setPrivateKeyRSA(const string & key) noexcept;
			/**
			 * @brief Метод получения приватного ключа RSA
			 *
			 * @param cipher тип шифрования приватного ключа
			 * @return       приватный ключ RSA
			 */
			string getPrivateKeyRSA(const cipher_t cipher = cipher_t::NONE) const noexcept;
		public:
			/**
			 * @brief Метод загрузки публичного ключа RSA из файла
			 *
			 * @param path путь к файлу с публичным ключом
			 * @return     результат загрузки ключа
			 */
			bool loadPublicKeyRSA(const string & path) noexcept;
			/**
			 * @brief Метод загрузки приватного ключа RSA из файла
			 *
			 * @param path путь к файлу с приватным ключом
			 * @return     результат загрузки ключа
			 */
			bool loadPrivateKeyRSA(const string & path) noexcept;
		public:
			/**
			 * @brief Метод сохранения публичного ключа RSA в файл
			 *
			 * @param path путь к файлу для сохранения публичного ключа
			 * @return     результат сохранения ключа
			 */
			bool savePublicKeyRSA(const string & path) const noexcept;
			/**
			 * @brief Метод сохранения приватного ключа RSA в файл
			 *
			 * @param path   путь к файлу для сохранения приватного ключа
			 * @param cipher тип шифрования приватного ключа
			 * @return       результат сохранения ключа
			 */
			bool savePrivateKeyRSA(const string & path, const cipher_t cipher = cipher_t::NONE) const noexcept;
		public:
			/**
			 * @brief Метод шифрования данных публичным ключом RSA
			 *
			 * @param buffer буфер данных для шифрования
			 * @param result буфер куда следует положить результат
			 */
			void encryptWithPublicKey(const vector <uint8_t> & buffer, vector <uint8_t> & result) const noexcept;
			/**
			 * @brief Метод шифрования данных публичным ключом RSA
			 *
			 * @param buffer буфер данных для шифрования
			 * @param size   размер данных для шифрования
			 * @param result буфер куда следует положить результат
			 */
			void encryptWithPublicKey(const uint8_t * buffer, const size_t size, vector <uint8_t> & result) const noexcept;
		public:
			/**
			 * @brief Метод дешифрования данных приватным ключом RSA
			 *
			 * @param buffer буфер данных для дешифрования
			 * @param result буфер куда следует положить результат
			 */
			void decryptWithPrivateKey(const vector <uint8_t> & buffer, vector <uint8_t> & result) const noexcept;
			/**
			 * @brief Метод дешифрования данных приватным ключом RSA
			 *
			 * @param buffer буфер данных для дешифрования
			 * @param size   размер данных для дешифрования
			 * @param result буфер куда следует положить результат
			 */
			void decryptWithPrivateKey(const uint8_t * buffer, const size_t size, vector <uint8_t> & result) const noexcept;
		public:
			/**
			 * @brief Метод подписания данных приватным ключом RSA
			 *
			 * @param buffer буфер данных для подписи
			 * @param hash   тип хэш-суммы
			 * @param result буфер куда следует положить результат
			 */
			void signWithPrivateKey(const vector <uint8_t> & buffer, const hash_t hash, vector <uint8_t> & result) const noexcept;
			/**
			 * @brief Метод подписания данных приватным ключом RSA
			 *
			 * @param buffer буфер данных для подписи
			 * @param size   размер данных для подписи
			 * @param hash   тип хэш-суммы
			 * @param result буфер куда следует положить результат
			 */
			void signWithPrivateKey(const uint8_t * buffer, const size_t size, const hash_t hash, vector <uint8_t> & result) const noexcept;
		public:
			/**
			 * @brief Метод верификации данных публичным ключом RSA
			 *
			 * @param buffer    буфер данных для верификации
			 * @param signature буфер с подписью данных
			 * @param hash      тип хэш-суммы
			 * @return          результат верификации
			 */
			bool verifyWithPublicKey(const vector <uint8_t> & buffer, const vector <uint8_t> & signature, const hash_t hash) const noexcept;
			/**
			 * @brief Метод верификации данных публичным ключом RSA
			 *
			 * @param buffer    буфер данных для верификации
			 * @param size      размер данных для верификации
			 * @param signature буфер с подписью данных
			 * @param hash      тип хэш-суммы
			 * @return          результат верификации
			 */
			bool verifyWithPublicKey(const uint8_t * buffer, const size_t size, const vector <uint8_t> & signature, const hash_t hash) const noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			explicit Crypto(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Crypto() noexcept;
	} crypto_t;
};

#endif // __AWH_CRYPTO__
