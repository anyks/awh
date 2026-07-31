/**
 * @file: crypto.hpp
 * @date: 2026-01-20
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл модуля криптографии — класс Crypto, выполняющий симметричное шифрование блоками AES,
 *        хеширование (MD5, SHA, HMAC), кодирование Base64 и работу с ключами RSA поверх криптографической библиотеки
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CRYPTO__
#define __AWH_CRYPTO__

/**
 * Стандартные заголовочные файлы
 */
#include <array>
#include <string>
#include <vector>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "fmk.hpp"
#include "log.hpp"
#include "hash.hpp"

/**
 * Если количество итераций PBKDF2 для вывода ключа AES не определено
 */
#ifndef AWH_CRYPTO_AES_ROUNDS
	/**
	 * Устанавливаем количество итераций PBKDF2 по умолчанию для вывода ключа AES из пароля
	 */
	#define AWH_CRYPTO_AES_ROUNDS 0x186A0
#endif

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
	 * @brief Предварительное объявление непрозрачного контекста стейта AES-шифрования
	 *
	 * @details Полное определение скрыто в модуле реализации,
	 *          что позволяет не подключать заголовочные файлы стороннего криптопровайдера (OpenSSL) в публичный интерфейс.
	 *
	 */
	struct state_t;
	/**
	 * @brief Предварительное объявление непрозрачного контекста ключа RSA
	 *
	 * @details Полное определение скрыто в модуле реализации,
	 *          что позволяет не подключать заголовочные файлы стороннего криптопровайдера (OpenSSL) в публичный интерфейс.
	 *
	 */
	struct key_rsa_t;
	/**
	 * @brief Класс криптографии
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Crypto {
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
			 * @brief Режим блочного шифрования
			 *
			 * @details Режим задаёт, каким образом блочный шифр применяется к данным,
			 *          и от разрядности ключа не зависит: разрядность задаётся набором
			 *          размеров шифрования отдельно.
			 *
			 * @warning Режим гаммирования подделку не обнаруживает: изменение
			 *          шифротекста меняет открытый текст предсказуемым образом. Брать
			 *          его следует лишь тогда, когда подлинность данных обеспечена
			 *          иначе - подписью либо имитовставкой поверх шифротекста
			 *
			 */
			enum class mode_t : uint8_t {
				NONE = 0x00, // Режим шифрования не установлен
				CFB  = 0x01, // Гаммирование с обратной связью по шифротексту, без проверки подлинности
				GCM  = 0x02  // Счётчик Галуа с проверкой подлинности данных
			};
			/**
			 * @brief Схема дополнения подписи RSA
			 *
			 * @details Схема задаёт, каким образом хэш-сумма данных превращается в
			 *          подпись. Шифрования данных ключом RSA схема не касается - оно
			 *          выполняется дополнением OAEP всегда.
			 *
			 * @warning Схема PKCS#1 v1.5 стойкости доказанной не имеет и оставлена
			 *          ради согласия с работами, иной не принимающими. Для новых
			 *          работ следует брать вероятностную схему
			 *
			 */
			enum class padding_t : uint8_t {
				NONE  = 0x00, // Схема дополнения подписи не установлена
				PKCS1 = 0x01, // Дополнение PKCS#1 v1.5, ради согласия с прежними работами
				PSS   = 0x02  // Вероятностная схема подписи, стойкость доказана
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
		private:
			/**
			 * @brief Структура параметров RSA
			 *
			 * @details Хранит параметры шифрования и расшифровки данных с помощью RSA.
			 *
			 */
			typedef struct __AWH_SHARED_EXPORT__ Params_RSA {
				// Количество итераций PBKDF2 для вывода ключа AES из пароля (по умолчанию 100000)
				uint32_t rounds;
				// Режим блочного шифрования
				mode_t mode;
				// Схема дополнения подписи RSA
				padding_t padding;
				// Соль шифрования
				string salt;
				// Пароль шифрования
				string password;
				// Пароль защиты приватного ключа RSA
				string passwordRSA;
				// Стейт AES шифрования
				state_t * state;
				// Объект RSA ключа
				key_rsa_t * key;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Params_RSA() noexcept;
			} params_rsa_t;
		public:
			/**
			 * @brief Тип 128-битного хэша
			 *
			 * @details Представлен в виде массива из 16 байт.
			 *
			 */
			using uint128_t = array <uint8_t, 16>;
		private:
			// Стейт AES шифрования
			params_rsa_t _params;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * @brief Метод установки количества итераций PBKDF2 для вывода ключа AES
			 *
			 * @param round количество итераций PBKDF2
			 *
			 */
			void roundAES(const uint32_t round) noexcept;
		public:
			/**
			 * @brief Метод установки соли шифрования
			 *
			 * @param salt соль для шифрования
			 *
			 */
			void salt(string_view salt) noexcept;
			/**
			 * @brief Метод установки пароля шифрования
			 *
			 * @param password пароль шифрования
			 *
			 */
			void password(string_view password) noexcept;
			/**
			 * @brief Метод установки режима блочного шифрования
			 *
			 * @details Метод сбрасывает контекст потокового шифрования, поэтому
			 *          менять режим посреди потока нельзя.
			 *
			 *          По умолчанию заведён режим с проверкой подлинности: он
			 *          обнаруживает подделку шифротекста, тогда как гаммирование её
			 *          пропускает. Ценой служит длина - шифротекст растёт на вектор
			 *          инициализации и имитовставку
			 *
			 * @param mode режим блочного шифрования
			 *
			 */
			void mode(const mode_t mode) noexcept;
			/**
			 * @brief Метод извлечения режима блочного шифрования
			 *
			 * @return режим блочного шифрования
			 *
			 */
			mode_t mode() const noexcept;
			/**
			 * @brief Метод установки схемы дополнения подписи RSA
			 *
			 * @details По умолчанию заведена вероятностная схема: стойкость её
			 *          доказана, тогда как у схемы PKCS#1 v1.5 доказательства нет.
			 *          Схема PKCS#1 v1.5 оставлена ради согласия с работами, иной
			 *          не принимающими.
			 *
			 *          Шифрования данных ключом RSA схема не касается - оно
			 *          выполняется дополнением OAEP всегда
			 *
			 * @param padding схема дополнения подписи RSA
			 *
			 */
			void padding(const padding_t padding) noexcept;
			/**
			 * @brief Метод извлечения схемы дополнения подписи RSA
			 *
			 * @return схема дополнения подписи RSA
			 *
			 */
			padding_t padding() const noexcept;
			/**
			 * @brief Метод установки пароля защиты приватного ключа RSA
			 *
			 * @details Пароль этот защищает приватный ключ при выписывании его в
			 *          файл и вычитывании оттуда, и с паролем шифрования данных
			 *          общего не имеет: прежде поле было одно на оба назначения, и
			 *          ключ выписывался под тем же паролем, которым шифруются
			 *          данные - утрата одного означала утрату и другого.
			 *
			 *          Незаданный пароль означает, что приватный ключ выписывается
			 *          и вычитывается незащищённым
			 *
			 * @param password пароль защиты приватного ключа RSA
			 *
			 */
			void passwordRSA(string_view password) noexcept;
		public:
			/**
			 * @brief Метод преобразования 128-битного хэша в 64-битный
			 *
			 * @param hash 128-битный хэш
			 * @return     64-битный хэш
			 *
			 */
			uint64_t hash128to64(const uint128_t & hash) const noexcept;
		public:
			/**
			 * @brief Шаблон типа результата хэширования
			 *
			 * @tparam T тип результата хэширования
			 *
			 */
			template <typename T = uint64_t>
			/**
			 * @brief Метод хэширования текста
			 *
			 * @details Результат хэширования выводится во встроенном числовом типе,
			 *          в массиве байтов либо в длинном числе модуля BigNum любой
			 *          объявленной разрядности.
			 *
			 * @param text текст для хэширования
			 * @return     результат хэширования
			 *
			 */
			AWH_HASH_INLINE T hash(string_view text) const noexcept {
				// Выполняем хэширование текста
				return hashing::create <T> (text.data(), text.size());
			}
			/**
			 * @brief Шаблон типа результата хэширования и типа буфера данных
			 *
			 * @tparam A тип результата хэширования
			 * @tparam B тип буфера данных для хэширования
			 *
			 */
			template <typename A = uint64_t, typename B, typename = decltype(std::declval <const B &> ().data())>
			/**
			 * @brief Метод хэширования буфера данных
			 *
			 * @param text буфер данных для хэширования
			 * @return     результат хэширования
			 *
			 */
			AWH_HASH_INLINE A hash(const B & text) const noexcept {
				// Выполняем хэширование буфера данных
				return hashing::create <A> (text.data(), text.size() * sizeof(typename B::value_type));
			}
			/**
			 * @brief Шаблон типа результата хэширования
			 *
			 * @tparam T тип результата хэширования
			 *
			 */
			template <typename T = uint64_t>
			/**
			 * @brief Метод хэширования буфера данных
			 *
			 * @param buffer буфер данных для хэширования
			 * @param size   размер данных для хэширования
			 * @return       результат хэширования
			 *
			 */
			AWH_HASH_INLINE T hash(const void * buffer, const size_t size) const noexcept {
				// Выполняем хэширование буфера данных
				return hashing::create <T> (buffer, size);
			}
		public:
			/**
			 * @brief Шаблон типа результата хэширования
			 *
			 * @tparam T тип результата хэширования
			 *
			 */
			template <typename T = uint64_t>
			/**
			 * @brief Метод хэширования текста c ключом
			 *
			 * @param text текст для хэширования
			 * @param seed ключ для хэширования
			 * @return     результат хэширования
			 *
			 */
			AWH_HASH_INLINE T hashWithSeed(string_view text, const uint64_t seed) const noexcept {
				// Выполняем хэширование текста с ключом
				return hashing::create <T> (text.data(), text.size(), seed);
			}
			/**
			 * @brief Шаблон типа результата хэширования и типа буфера данных
			 *
			 * @tparam A тип результата хэширования
			 * @tparam B тип буфера данных для хэширования
			 *
			 */
			template <typename A = uint64_t, typename B, typename = decltype(std::declval <const B &> ().data())>
			/**
			 * @brief Метод хэширования буфера данных c ключом
			 *
			 * @param text буфер данных для хэширования
			 * @param seed ключ для хэширования
			 * @return     результат хэширования
			 *
			 */
			AWH_HASH_INLINE A hashWithSeed(const B & text, const uint64_t seed) const noexcept {
				// Выполняем хэширование буфера данных с ключом
				return hashing::create <A> (text.data(), text.size() * sizeof(typename B::value_type), seed);
			}
			/**
			 * @brief Шаблон типа результата хэширования
			 *
			 * @tparam T тип результата хэширования
			 *
			 */
			template <typename T = uint64_t>
			/**
			 * @brief Метод хэширования буфера данных c ключом
			 *
			 * @param buffer буфер данных для хэширования
			 * @param size   размер данных для хэширования
			 * @param seed   ключ для хэширования
			 * @return       результат хэширования
			 *
			 */
			AWH_HASH_INLINE T hashWithSeed(const void * buffer, const size_t size, const uint64_t seed) const noexcept {
				// Выполняем хэширование буфера данных с ключом
				return hashing::create <T> (buffer, size, seed);
			}
		public:
			/**
			 * @brief Шаблон типа результата хэширования
			 *
			 * @tparam T тип результата хэширования
			 *
			 */
			template <typename T = uint64_t>
			/**
			 * @brief Метод хэширования текста c несколькими ключами
			 *
			 * @param text  текст для хэширования
			 * @param seed1 первый ключ для хэширования
			 * @param seed2 второй ключ для хэширования
			 * @return      результат хэширования
			 *
			 */
			AWH_HASH_INLINE T hashWithSeeds(string_view text, const uint64_t seed1, const uint64_t seed2) const noexcept {
				// Выполняем хэширование текста с ключами
				return hashing::create <T> (text.data(), text.size(), hashing::merge(seed1, seed2));
			}
			/**
			 * @brief Шаблон типа результата хэширования и типа буфера данных
			 *
			 * @tparam A тип результата хэширования
			 * @tparam B тип буфера данных для хэширования
			 *
			 */
			template <typename A = uint64_t, typename B, typename = decltype(std::declval <const B &> ().data())>
			/**
			 * @brief Метод хэширования буфера данных c несколькими ключами
			 *
			 * @param text  буфер данных для хэширования
			 * @param seed1 первый ключ для хэширования
			 * @param seed2 второй ключ для хэширования
			 * @return      результат хэширования
			 *
			 */
			AWH_HASH_INLINE A hashWithSeeds(const B & text, const uint64_t seed1, const uint64_t seed2) const noexcept {
				// Выполняем хэширование буфера данных с ключами
				return hashing::create <A> (text.data(), text.size() * sizeof(typename B::value_type), hashing::merge(seed1, seed2));
			}
			/**
			 * @brief Шаблон типа результата хэширования
			 *
			 * @tparam T тип результата хэширования
			 *
			 */
			template <typename T = uint64_t>
			/**
			 * @brief Метод хэширования буфера данных c несколькими ключами
			 *
			 * @param buffer буфер данных для хэширования
			 * @param size   размер данных для хэширования
			 * @param seed1  первый ключ для хэширования
			 * @param seed2  второй ключ для хэширования
			 * @return       результат хэширования
			 *
			 */
			AWH_HASH_INLINE T hashWithSeeds(const void * buffer, const size_t size, const uint64_t seed1, const uint64_t seed2) const noexcept {
				// Выполняем хэширование буфера данных с ключами
				return hashing::create <T> (buffer, size, hashing::merge(seed1, seed2));
			}
		public:
			/**
			 * @brief Шаблон метода хэширования текста
			 *
			 * @tparam T тип возвращаемого результата
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод хэширования текста
			 *
			 * @param buffer буфер данных для хэширования
			 * @param hash   тип хэш-суммы
			 * @return       результат хэширования
			 *
			 */
			auto hash(string_view buffer, const hash_t hash) const noexcept -> T;
			/**
			 * @brief Шаблон метода хэширования текста
			 *
			 * @tparam A тип возвращаемого результата
			 * @tparam B тип буфера данных
			 *
			 */
			template <typename A, typename B>
			/**
			 * @brief Метод хэширования текста
			 *
			 * @param buffer буфер данных для хэширования
			 * @param hash   тип хэш-суммы
			 * @return       результат хэширования
			 *
			 */
			auto hash(const B & buffer, const hash_t hash) const noexcept -> A;
		public:
			/**
			 * @brief Шаблон метода хэширования текста с ключом
			 *
			 * @tparam T тип возвращаемого результата
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод хэширования текста с ключом
			 *
			 * @param key    ключ для подписи
			 * @param buffer буфер данных для хэширования
			 * @param hash   тип хэш-суммы
			 * @return       результат хэширования
			 *
			 */
			auto hmac(string_view key, string_view buffer, const hash_t hash) const noexcept -> T;
			/**
			 * @brief Шаблон метода хэширования текста с ключом
			 *
			 * @tparam T тип возвращаемого результата
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод хэширования текста с ключом
			 *
			 * @param key    ключ для подписи
			 * @param buffer буфер данных для хэширования
			 * @param hash   тип хэш-суммы
			 * @return       результат хэширования
			 *
			 */
			auto hmac(const string & key, string_view buffer, const hash_t hash) const noexcept -> T;
			/**
			 * @brief Шаблон метода хэширования текста с ключом
			 *
			 * @tparam A тип возвращаемого результата
			 * @tparam B тип буфера данных
			 *
			 */
			template <typename A, typename B>
			/**
			 * @brief Метод хэширования текста с ключом
			 *
			 * @param key    ключ для подписи
			 * @param buffer буфер данных для хэширования
			 * @param hash   тип хэш-суммы
			 * @return       результат хэширования
			 *
			 */
			auto hmac(string_view key, const B & buffer, const hash_t hash) const noexcept -> A;
			/**
			 * @brief Шаблон метода хэширования текста с ключом
			 *
			 * @tparam A тип возвращаемого результата
			 * @tparam B тип буфера данных
			 *
			 */
			template <typename A, typename B>
			/**
			 * @brief Метод хэширования текста с ключом
			 *
			 * @param key    ключ для подписи
			 * @param buffer буфер данных для хэширования
			 * @param hash   тип хэш-суммы
			 * @return       результат хэширования
			 *
			 */
			auto hmac(const string & key, const B & buffer, const hash_t hash) const noexcept -> A;
		public:
			/**
			 * @brief Шаблон метода кодирования
			 *
			 * @tparam T тип возвращаемого результата
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод финализации контекста шифрования
			 *
			 * @param buffer буфер куда следует положить результат
			 * @return       результат финализации
			 *
			 */
			bool finalize(T & buffer) noexcept;
			/**
			 * @brief Метод инициализации контекста шифрования
			 *
			 * @param event  событие шифрования (ENCODE, DECODE)
			 * @param hash   тип хэш-суммы
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @return       результат инициализации
			 *
			 */
			bool initialize(const event_t event, const hash_t hash, const cipher_t cipher) noexcept;
		public:
			/**
			 * @brief Шаблон метода кодирования
			 *
			 * @tparam T тип возвращаемого результата
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод кодирования
			 *
			 * @param buffer буфер данных для шифрования
			 * @param hash   тип хэш-суммы
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @return       результат кодирования
			 *
			 */
			auto encrypt(string_view buffer, const hash_t hash = hash_t::NONE, const cipher_t cipher = cipher_t::NONE) const noexcept -> T;
			/**
			 * @brief Шаблон метода кодирования
			 *
			 * @tparam A тип возвращаемого результата
			 * @tparam B тип буфера данных
			 *
			 */
			template <typename A, typename B>
			/**
			 * @brief Метод кодирования
			 *
			 * @param buffer буфер данных для шифрования
			 * @param hash   тип хэш-суммы
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @return       результат кодирования
			 *
			 */
			auto encrypt(const B & buffer, const hash_t hash = hash_t::NONE, const cipher_t cipher = cipher_t::NONE) const noexcept -> A;
			/**
			 * @brief Шаблон метода кодирования
			 *
			 * @tparam T тип возвращаемого результата
			 *
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
			 *
			 */
			auto encrypt(const void * buffer, const size_t size, const hash_t hash = hash_t::NONE, const cipher_t cipher = cipher_t::NONE) const noexcept -> T;
		public:
			/**
			 * @brief Шаблон метода кодирования с выводом признака работы
			 *
			 * @tparam T тип буфера результата
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод кодирования с выводом признака работы
			 *
			 * @details Признак работы выводится отдельно от буфера: пустой буфер
			 *          отказом не является - расшифровка сообщения, октетов не
			 *          имеющего, даёт пустой открытый текст, - и судить по нему об
			 *          удаче нельзя. Перегрузки, выводящие один лишь буфер,
			 *          оставлены для работ, которым различать это не нужно.
			 *
			 * @param buffer буфер данных для шифрования
			 * @param size   размер данных для шифрования
			 * @param result буфер, куда следует положить результат
			 * @param hash   тип хэш-суммы
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @return       признак успешно выполненной работы
			 *
			 */
			bool encrypt(const void * buffer, const size_t size, T & result, const hash_t hash = hash_t::NONE, const cipher_t cipher = cipher_t::NONE) const noexcept;
		public:
			/**
			 * @brief Шаблон метода декодирования
			 *
			 * @tparam T тип возвращаемого результата
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод декодирования
			 *
			 * @param buffer буфер данных для шифрования
			 * @param hash   тип хэш-суммы
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @return       результат кодирования
			 *
			 */
			auto decrypt(string_view buffer, const hash_t hash = hash_t::NONE, const cipher_t cipher = cipher_t::NONE) const noexcept -> T;
			/**
			 * @brief Шаблон метода декодирования
			 *
			 * @tparam A тип возвращаемого результата
			 * @tparam B тип буфера данных
			 *
			 */
			template <typename A, typename B>
			/**
			 * @brief Метод декодирования
			 *
			 * @param buffer буфер данных для шифрования
			 * @param hash   тип хэш-суммы
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @return       результат кодирования
			 *
			 */
			auto decrypt(const B & buffer, const hash_t hash = hash_t::NONE, const cipher_t cipher = cipher_t::NONE) const noexcept -> A;
			/**
			 * @brief Шаблон метода декодирования
			 *
			 * @tparam T тип возвращаемого результата
			 *
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
			 *
			 */
			auto decrypt(const void * buffer, const size_t size, const hash_t hash = hash_t::NONE, const cipher_t cipher = cipher_t::NONE) const noexcept -> T;
		public:
			/**
			 * @brief Шаблон метода декодирования с выводом признака работы
			 *
			 * @tparam T тип буфера результата
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод декодирования с выводом признака работы
			 *
			 * @details Признак работы выводится отдельно от буфера: пустой буфер
			 *          отказом не является - расшифровка сообщения, октетов не
			 *          имеющего, даёт пустой открытый текст, - и судить по нему об
			 *          удаче нельзя. Перегрузки, выводящие один лишь буфер,
			 *          оставлены для работ, которым различать это не нужно.
			 *
			 * @param buffer буфер данных для шифрования
			 * @param size   размер данных для шифрования
			 * @param result буфер, куда следует положить результат
			 * @param hash   тип хэш-суммы
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @return       признак успешно выполненной работы
			 *
			 */
			bool decrypt(const void * buffer, const size_t size, T & result, const hash_t hash = hash_t::NONE, const cipher_t cipher = cipher_t::NONE) const noexcept;
		public:
			/**
			 * @brief Метод генерации приватного ключа RSA
			 *
			 * @details Разрядность принимается любая от двух тысяч сорока восьми и
			 *          выше, а не только привычные 2048, 3072 и 4096: разрядность
			 *          есть свойство вызывающего, и перечнем её ограничивать не за
			 *          что. Меньшая разрядность отвергается - стойкости у неё нет.
			 *          Незаданная разрядность означает две тысячи сорок восемь.
			 *
			 * @param size размер ключа в битах, от 2048 и выше (0 — по умолчанию)
			 * @return     результат генерации ключа
			 *
			 */
			bool generatePrivateKeyRSA(const size_t size = 0) noexcept;
		public:
			/**
			 * @brief Метод получения публичного ключа RSA
			 *
			 * @return публичный ключ RSA
			 *
			 */
			string getPublicKeyRSA() const noexcept;
			/**
			 * @brief Метод установки публичного ключа RSA
			 *
			 * @param key публичный ключ RSA
			 * @return    результат установки ключа
			 *
			 */
			bool setPublicKeyRSA(string_view key) noexcept;
		public:
			/**
			 * @brief Метод установки приватного ключа RSA
			 *
			 * @param key приватный ключ RSA
			 * @return    результат установки ключа
			 *
			 */
			bool setPrivateKeyRSA(string_view key) noexcept;
			/**
			 * @brief Метод получения приватного ключа RSA
			 *
			 * @param cipher тип шифрования приватного ключа
			 * @return       приватный ключ RSA
			 *
			 */
			string getPrivateKeyRSA(const cipher_t cipher = cipher_t::NONE) const noexcept;
		public:
			/**
			 * @brief Метод загрузки публичного ключа RSA из файла
			 *
			 * @param path путь к файлу с публичным ключом
			 * @return     результат загрузки ключа
			 *
			 */
			bool loadPublicKeyRSA(string_view path) noexcept;
			/**
			 * @brief Метод загрузки приватного ключа RSA из файла
			 *
			 * @param path путь к файлу с приватным ключом
			 * @return     результат загрузки ключа
			 *
			 */
			bool loadPrivateKeyRSA(string_view path) noexcept;
		public:
			/**
			 * @brief Метод сохранения публичного ключа RSA в файл
			 *
			 * @param path путь к файлу для сохранения публичного ключа
			 * @return     результат сохранения ключа
			 *
			 */
			bool savePublicKeyRSA(string_view path) const noexcept;
			/**
			 * @brief Метод сохранения приватного ключа RSA в файл
			 *
			 * @param path   путь к файлу для сохранения приватного ключа
			 * @param cipher тип шифрования приватного ключа
			 * @return       результат сохранения ключа
			 *
			 */
			bool savePrivateKeyRSA(string_view path, const cipher_t cipher = cipher_t::NONE) const noexcept;
		public:
			/**
			 * @brief Метод шифрования данных публичным ключом RSA
			 *
			 * @param buffer буфер данных для шифрования
			 * @param result буфер куда следует положить результат
			 *
			 */
			void encryptWithPublicKey(const vector <uint8_t> & buffer, vector <uint8_t> & result) const noexcept;
			/**
			 * @brief Метод шифрования данных публичным ключом RSA
			 *
			 * @param buffer буфер данных для шифрования
			 * @param size   размер данных для шифрования
			 * @param result буфер куда следует положить результат
			 *
			 */
			void encryptWithPublicKey(const uint8_t * buffer, const size_t size, vector <uint8_t> & result) const noexcept;
		public:
			/**
			 * @brief Метод дешифрования данных приватным ключом RSA
			 *
			 * @param buffer буфер данных для дешифрования
			 * @param result буфер куда следует положить результат
			 *
			 */
			void decryptWithPrivateKey(const vector <uint8_t> & buffer, vector <uint8_t> & result) const noexcept;
			/**
			 * @brief Метод дешифрования данных приватным ключом RSA
			 *
			 * @param buffer буфер данных для дешифрования
			 * @param size   размер данных для дешифрования
			 * @param result буфер куда следует положить результат
			 *
			 */
			void decryptWithPrivateKey(const uint8_t * buffer, const size_t size, vector <uint8_t> & result) const noexcept;
		public:
			/**
			 * @brief Метод подписания данных приватным ключом RSA
			 *
			 * @param buffer буфер данных для подписи
			 * @param hash   тип хэш-суммы
			 * @param result буфер куда следует положить результат
			 *
			 */
			void signWithPrivateKey(const vector <uint8_t> & buffer, const hash_t hash, vector <uint8_t> & result) const noexcept;
			/**
			 * @brief Метод подписания данных приватным ключом RSA
			 *
			 * @param buffer буфер данных для подписи
			 * @param size   размер данных для подписи
			 * @param hash   тип хэш-суммы
			 * @param result буфер куда следует положить результат
			 *
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
			 *
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
			 *
			 */
			bool verifyWithPublicKey(const uint8_t * buffer, const size_t size, const vector <uint8_t> & signature, const hash_t hash) const noexcept;
		public:
			/**
			 * @brief Оператор копирования
			 *
			 * @details Копирование запрещено: объект владеет состоянием шифрования и
			 *          ключом RSA по указателям, и поверхностная копия освободила бы
			 *          контекст библиотеки криптографии дважды. Тайну копия к тому же
			 *          разносила бы вширь - пароли и выведенный ключ достались бы ей
			 *          заодно, а гасит их лишь тот, кто их завёл.
			 *
			 */
			Crypto & operator = (const Crypto &) = delete;
			/**
			 * @brief Оператор переноса
			 *
			 */
			Crypto & operator = (Crypto &&) = delete;
		public:
			/**
			 * @brief Конструктор копирования
			 *
			 */
			Crypto(const Crypto &) = delete;
			/**
			 * @brief Конструктор переноса
			 *
			 */
			Crypto(Crypto &&) = delete;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 *
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
