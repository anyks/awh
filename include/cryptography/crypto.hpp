/**
 * @file crypto.hpp
 * @date 2026-01-20
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
 * @brief Заголовочный файл модуля криптографии — класс Crypto, выполняющий симметричное шифрование блоками AES,
 *        хеширование (MD5, SHA, HMAC), кодирование Base64 и работу с ключами RSA поверх криптографической библиотеки
 *
 * \~english
 * @brief Header file of the cryptography module — the Crypto class performing symmetric encryption by AES blocks,
 *        hashing (MD5, SHA, HMAC), Base64 encoding and work with RSA keys on top of a cryptographic library
 *
 * \~
 *
 * @copyright Copyright © 2026
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
#include "../sys/fmk.hpp"
#include "../sys/log.hpp"
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
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Предварительное объявление непрозрачного контекста стейта AES-шифрования
	 *
	 * @details Полное определение скрыто в модуле реализации,
	 *          что позволяет не подключать заголовочные файлы стороннего криптопровайдера (OpenSSL) в публичный интерфейс.
	 *
	 * \~english
	 * @brief Forward declaration of the opaque context of the AES encryption state
	 *
	 * @details The full definition is hidden in the implementation module,
	 *          which makes it possible not to include the header files of the third-party cryptographic provider (OpenSSL) in the public interface.
	 *
	 * \~
	 */
	struct state_t;
	/**
	 * \~russian
	 * @brief Предварительное объявление непрозрачного контекста ключа RSA
	 *
	 * @details Полное определение скрыто в модуле реализации,
	 *          что позволяет не подключать заголовочные файлы стороннего криптопровайдера (OpenSSL) в публичный интерфейс.
	 *
	 * \~english
	 * @brief Forward declaration of the opaque context of the RSA key
	 *
	 * @details The full definition is hidden in the implementation module,
	 *          which makes it possible not to include the header files of the third-party cryptographic provider (OpenSSL) in the public interface.
	 *
	 * \~
	 */
	struct key_rsa_t;
	/**
	 * \~russian
	 * @brief Класс криптографии
	 *
	 * \~english
	 * @brief Cryptography class
	 *
	 * \~
	 */
	typedef class __AWH_SHARED_EXPORT__ Crypto {
		public:
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
			 * @brief Типы ключей RSA
			 *
			 * \~english
			 * @brief Types of the RSA keys
			 *
			 * \~
			 */
			enum class key_type_t : uint8_t {
				NONE    = 0x00, // Тип ключа не установлен
				PUBLIC  = 0x01, // Публичный ключ
				PRIVATE = 0x02  // Приватный ключ
			};
			/**
			 * \~russian
			 * @brief Набор размеров шифрования
			 *
			 * \~english
			 * @brief Set of the encryption sizes
			 *
			 * \~
			 */
			enum class cipher_t : uint16_t {
				NONE   = 0,   // Размер шифрования не установлен
				BASE64 = 64,  // Шиффрование в BASE64
				AES128 = 128, // Размер шифрования 128 бит
				AES192 = 192, // Размер шифрования 192 бит
				AES256 = 256  // Размер шифрования 256 бит
			};
			/**
			 * \~russian
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
			 * \~english
			 * @brief Mode of the block encryption
			 *
			 * @details The mode sets in what manner the block cipher is applied to the data,
			 *          and does not depend on the width of the key: the width is set by the set
			 *          of the encryption sizes separately.
			 *
			 * @warning The stream cipher mode does not detect a forgery: a change of
			 *          the ciphertext changes the plaintext in a predictable manner. It should be taken
			 *          only when the authenticity of the data is ensured
			 *          otherwise - by a signature or by a message authentication code on top of the ciphertext
			 *
			 * \~
			 */
			enum class mode_t : uint8_t {
				NONE = 0x00, // Режим шифрования не установлен
				CFB  = 0x01, // Гаммирование с обратной связью по шифротексту, без проверки подлинности
				GCM  = 0x02  // Счётчик Галуа с проверкой подлинности данных
			};
			/**
			 * \~russian
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
			 * \~english
			 * @brief Padding scheme of the RSA signature
			 *
			 * @details The scheme sets in what manner the hash sum of the data turns into
			 *          a signature. The scheme does not concern the encryption of data by an RSA key - that
			 *          is performed by the OAEP padding always.
			 *
			 * @warning The PKCS#1 v1.5 scheme has no proven strength and has been left
			 *          for the sake of agreement with the works accepting no other. For new
			 *          works the probabilistic scheme should be taken
			 *
			 * \~
			 */
			enum class padding_t : uint8_t {
				NONE  = 0x00, // Схема дополнения подписи не установлена
				PKCS1 = 0x01, // Дополнение PKCS#1 v1.5, ради согласия с прежними работами
				PSS   = 0x02  // Вероятностная схема подписи, стойкость доказана
			};
			/**
			 * \~russian
			 * @brief Тип хэш-суммы
			 *
			 * \~english
			 * @brief Type of the hash sum
			 *
			 * \~
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
			 * \~russian
			 * @brief Вид записи выработанной хэш-суммы
			 *
			 * @details Вид задаёт, в каком виде хэш-сумма и имитовставка выдаются
			 *          наружу. Шестнадцатеричная запись взята по умолчанию ради
			 *          согласия с работами, её ожидающими - проверка подлинности
			 *          Digest по RFC 7616 сличает именно её, - но подписи сообщений
			 *          по RFC 9421 кодируют BASE64 двоичный вид, и шестнадцатеричная
			 *          запись дала бы подпись, чужими работами не принимаемую
			 *
			 * \~english
			 * @brief Kind of the record of the produced hash sum
			 *
			 * @details The kind sets in what form the hash sum and the message authentication code are given
			 *          outward. The hexadecimal record has been taken by default for the sake
			 *          of agreement with the works expecting it - the Digest authenticity check
			 *          per RFC 7616 compares exactly it, - but the message signatures
			 *          per RFC 9421 encode the binary form in BASE64, and the hexadecimal
			 *          record would give a signature not accepted by foreign works
			 *
			 * \~
			 */
			enum class format_t : uint8_t {
				HEX = 0x01, // Шестнадцатеричная запись, вдвое длиннее двоичного вида
				RAW = 0x02  // Двоичный вид, как его выдаёт библиотека криптографии
			};
		private:
			/**
			 * \~russian
			 * @brief Структура параметров RSA
			 *
			 * @details Хранит параметры шифрования и расшифровки данных с помощью RSA.
			 *
			 * \~english
			 * @brief Structure of the RSA parameters
			 *
			 * @details Stores the parameters of the encryption and decryption of data by means of RSA.
			 *
			 * \~
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
				 * \~russian
				 * @brief Конструктор
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				explicit Params_RSA() noexcept;
			} params_rsa_t;
		public:
			/**
			 * \~russian
			 * @brief Тип 128-битного хэша
			 *
			 * @details Представлен в виде массива из 16 байт.
			 *
			 * \~english
			 * @brief Type of the 128-bit hash
			 *
			 * @details Represented as an array of 16 bytes.
			 *
			 * \~
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
		private:
			/**
			 * \~russian
			 * @brief Метод проверки готовности объекта к работе
			 *
			 * @details Стейт шифрования и ключевые данные отводятся в куче при сборке
			 *          объекта, а сборка отказа не выдаёт: конструктор объявлен не
			 *          бросающим исключений, и сбой отведения памяти оставляет объект
			 *          собранным наполовину. Обращение к незаведённым данным - поведение
			 *          неопределённое, и потому всякий открытый метод спрашивает
			 *          готовность прежде работы. Отказ записывается в лог: причина у него
			 *          одна - нехватка памяти при сборке, - и вызывающей стороне о ней
			 *          иначе не узнать
			 *
			 * @return результат проверки готовности
			 *
			 * \~english
			 * @brief Method checking the readiness of the object for work
			 *
			 * @details The encryption state and the key data are allotted in the heap during the assembly of
			 *          the object, and the assembly gives out no failure: the constructor is declared as not
			 *          throwing exceptions, and a failure of the memory allotment leaves the object
			 *          assembled by halves. Addressing data that has not been set up is undefined
			 *          behaviour, and therefore every public method asks for the
			 *          readiness before the work. The failure is recorded into the log: it has one
			 *          reason - a shortage of memory during the assembly, - and the calling side has
			 *          no other way to learn of it
			 *
			 * @return result of the readiness check
			 *
			 * \~
			 */
			bool ready() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки количества итераций PBKDF2 для вывода ключа AES
			 *
			 * @param round количество итераций PBKDF2
			 *
			 * \~english
			 * @brief Method setting the number of the PBKDF2 iterations for deriving the AES key
			 *
			 * @param round number of the PBKDF2 iterations
			 *
			 * \~
			 */
			void roundAES(const uint32_t round) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки соли шифрования
			 *
			 * @param salt соль для шифрования
			 *
			 * \~english
			 * @brief Method setting the encryption salt
			 *
			 * @param salt salt for the encryption
			 *
			 * \~
			 */
			void salt(string_view salt) noexcept;
			/**
			 * \~russian
			 * @brief Метод установки пароля шифрования
			 *
			 * @param password пароль шифрования
			 *
			 * \~english
			 * @brief Method setting the encryption password
			 *
			 * @param password encryption password
			 *
			 * \~
			 */
			void password(string_view password) noexcept;
			/**
			 * \~russian
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
			 * \~english
			 * @brief Method setting the mode of the block encryption
			 *
			 * @details The method resets the context of the streaming encryption, therefore
			 *          the mode must not be changed in the middle of a stream.
			 *
			 *          By default the mode with the authenticity check is provided: it
			 *          detects a forgery of the ciphertext, whereas the stream cipher lets it
			 *          through. The price is the length - the ciphertext grows by the initialization
			 *          vector and the message authentication code
			 *
			 * @param mode mode of the block encryption
			 *
			 * \~
			 */
			void mode(const mode_t mode) noexcept;
			/**
			 * \~russian
			 * @brief Метод извлечения режима блочного шифрования
			 *
			 * @return режим блочного шифрования
			 *
			 * \~english
			 * @brief Method extracting the mode of the block encryption
			 *
			 * @return mode of the block encryption
			 *
			 * \~
			 */
			mode_t mode() const noexcept;
			/**
			 * \~russian
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
			 * \~english
			 * @brief Method setting the padding scheme of the RSA signature
			 *
			 * @details By default the probabilistic scheme is provided: its strength has been
			 *          proven, whereas the PKCS#1 v1.5 scheme has no proof.
			 *          The PKCS#1 v1.5 scheme has been left for the sake of agreement with the works accepting
			 *          no other.
			 *
			 *          The scheme does not concern the encryption of data by an RSA key - that
			 *          is performed by the OAEP padding always
			 *
			 * @param padding padding scheme of the RSA signature
			 *
			 * \~
			 */
			void padding(const padding_t padding) noexcept;
			/**
			 * \~russian
			 * @brief Метод извлечения схемы дополнения подписи RSA
			 *
			 * @return схема дополнения подписи RSA
			 *
			 * \~english
			 * @brief Method extracting the padding scheme of the RSA signature
			 *
			 * @return padding scheme of the RSA signature
			 *
			 * \~
			 */
			padding_t padding() const noexcept;
			/**
			 * \~russian
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
			 * \~english
			 * @brief Method setting the password protecting the private RSA key
			 *
			 * @details This password protects the private key when writing it out into
			 *          a file and reading it out from there, and has nothing in common with the password of the data
			 *          encryption: formerly the field was one for both purposes, and
			 *          the key was written out under the same password by which the data
			 *          is encrypted - the loss of one meant the loss of the other as well.
			 *
			 *          An unset password means that the private key is written out
			 *          and read out unprotected
			 *
			 * @param password password protecting the private RSA key
			 *
			 * \~
			 */
			void passwordRSA(string_view password) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод преобразования 128-битного хэша в 64-битный
			 *
			 * @param hash 128-битный хэш
			 * @return     64-битный хэш
			 *
			 * \~english
			 * @brief Method converting a 128-bit hash into a 64-bit one
			 *
			 * @param hash 128-bit hash
			 * @return     64-bit hash
			 *
			 * \~
			 */
			uint64_t hash128to64(const uint128_t & hash) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Шаблон типа результата хэширования
			 *
			 * @tparam T тип результата хэширования
			 *
			 * \~english
			 * @brief Template of the type of the hashing result
			 *
			 * @tparam T type of the hashing result
			 *
			 * \~
			 */
			template <typename T = uint64_t>
			/**
			 * \~russian
			 * @brief Метод хэширования текста
			 *
			 * @details Результат хэширования выводится во встроенном числовом типе,
			 *          в массиве байтов либо в длинном числе модуля BigNum любой
			 *          объявленной разрядности.
			 *
			 * @param text текст для хэширования
			 * @return     результат хэширования
			 *
			 * \~english
			 * @brief Method of hashing a text
			 *
			 * @details The hashing result is output in a built-in numeric type,
			 *          in a byte array or in a long number of the BigNum module of any
			 *          declared width.
			 *
			 * @param text text for the hashing
			 * @return     hashing result
			 *
			 * \~
			 */
			AWH_HASH_INLINE T hash(string_view text) const noexcept {
				// Выполняем хэширование текста
				return hashing::create <T> (text.data(), text.size());
			}
			/**
			 * \~russian
			 * @brief Шаблон типа результата хэширования и типа буфера данных
			 *
			 * @tparam A тип результата хэширования
			 * @tparam B тип буфера данных для хэширования
			 *
			 * \~english
			 * @brief Template of the type of the hashing result and of the type of the data buffer
			 *
			 * @tparam A type of the hashing result
			 * @tparam B type of the data buffer for the hashing
			 *
			 * \~
			 */
			template <typename A = uint64_t, typename B, typename = decltype(std::declval <const B &> ().data())>
			/**
			 * \~russian
			 * @brief Метод хэширования буфера данных
			 *
			 * @param text буфер данных для хэширования
			 * @return     результат хэширования
			 *
			 * \~english
			 * @brief Method of hashing a data buffer
			 *
			 * @param text data buffer for the hashing
			 * @return     hashing result
			 *
			 * \~
			 */
			AWH_HASH_INLINE A hash(const B & text) const noexcept {
				// Выполняем хэширование буфера данных
				return hashing::create <A> (text.data(), text.size() * sizeof(typename B::value_type));
			}
			/**
			 * \~russian
			 * @brief Шаблон типа результата хэширования
			 *
			 * @tparam T тип результата хэширования
			 *
			 * \~english
			 * @brief Template of the type of the hashing result
			 *
			 * @tparam T type of the hashing result
			 *
			 * \~
			 */
			template <typename T = uint64_t>
			/**
			 * \~russian
			 * @brief Метод хэширования буфера данных
			 *
			 * @param buffer буфер данных для хэширования
			 * @param size   размер данных для хэширования
			 * @return       результат хэширования
			 *
			 * \~english
			 * @brief Method of hashing a data buffer
			 *
			 * @param buffer data buffer for the hashing
			 * @param size   size of the data for the hashing
			 * @return       hashing result
			 *
			 * \~
			 */
			AWH_HASH_INLINE T hash(const void * buffer, const size_t size) const noexcept {
				// Выполняем хэширование буфера данных
				return hashing::create <T> (buffer, size);
			}
		public:
			/**
			 * \~russian
			 * @brief Шаблон типа результата хэширования
			 *
			 * @tparam T тип результата хэширования
			 *
			 * \~english
			 * @brief Template of the type of the hashing result
			 *
			 * @tparam T type of the hashing result
			 *
			 * \~
			 */
			template <typename T = uint64_t>
			/**
			 * \~russian
			 * @brief Метод хэширования текста c ключом
			 *
			 * @param text текст для хэширования
			 * @param seed ключ для хэширования
			 * @return     результат хэширования
			 *
			 * \~english
			 * @brief Method of hashing a text with a key
			 *
			 * @param text text for the hashing
			 * @param seed key for the hashing
			 * @return     hashing result
			 *
			 * \~
			 */
			AWH_HASH_INLINE T hashWithSeed(string_view text, const uint64_t seed) const noexcept {
				// Выполняем хэширование текста с ключом
				return hashing::create <T> (text.data(), text.size(), seed);
			}
			/**
			 * \~russian
			 * @brief Шаблон типа результата хэширования и типа буфера данных
			 *
			 * @tparam A тип результата хэширования
			 * @tparam B тип буфера данных для хэширования
			 *
			 * \~english
			 * @brief Template of the type of the hashing result and of the type of the data buffer
			 *
			 * @tparam A type of the hashing result
			 * @tparam B type of the data buffer for the hashing
			 *
			 * \~
			 */
			template <typename A = uint64_t, typename B, typename = decltype(std::declval <const B &> ().data())>
			/**
			 * \~russian
			 * @brief Метод хэширования буфера данных c ключом
			 *
			 * @param text буфер данных для хэширования
			 * @param seed ключ для хэширования
			 * @return     результат хэширования
			 *
			 * \~english
			 * @brief Method of hashing a data buffer with a key
			 *
			 * @param text data buffer for the hashing
			 * @param seed key for the hashing
			 * @return     hashing result
			 *
			 * \~
			 */
			AWH_HASH_INLINE A hashWithSeed(const B & text, const uint64_t seed) const noexcept {
				// Выполняем хэширование буфера данных с ключом
				return hashing::create <A> (text.data(), text.size() * sizeof(typename B::value_type), seed);
			}
			/**
			 * \~russian
			 * @brief Шаблон типа результата хэширования
			 *
			 * @tparam T тип результата хэширования
			 *
			 * \~english
			 * @brief Template of the type of the hashing result
			 *
			 * @tparam T type of the hashing result
			 *
			 * \~
			 */
			template <typename T = uint64_t>
			/**
			 * \~russian
			 * @brief Метод хэширования буфера данных c ключом
			 *
			 * @param buffer буфер данных для хэширования
			 * @param size   размер данных для хэширования
			 * @param seed   ключ для хэширования
			 * @return       результат хэширования
			 *
			 * \~english
			 * @brief Method of hashing a data buffer with a key
			 *
			 * @param buffer data buffer for the hashing
			 * @param size   size of the data for the hashing
			 * @param seed   key for the hashing
			 * @return       hashing result
			 *
			 * \~
			 */
			AWH_HASH_INLINE T hashWithSeed(const void * buffer, const size_t size, const uint64_t seed) const noexcept {
				// Выполняем хэширование буфера данных с ключом
				return hashing::create <T> (buffer, size, seed);
			}
		public:
			/**
			 * \~russian
			 * @brief Шаблон типа результата хэширования
			 *
			 * @tparam T тип результата хэширования
			 *
			 * \~english
			 * @brief Template of the type of the hashing result
			 *
			 * @tparam T type of the hashing result
			 *
			 * \~
			 */
			template <typename T = uint64_t>
			/**
			 * \~russian
			 * @brief Метод хэширования текста c несколькими ключами
			 *
			 * @param text  текст для хэширования
			 * @param seed1 первый ключ для хэширования
			 * @param seed2 второй ключ для хэширования
			 * @return      результат хэширования
			 *
			 * \~english
			 * @brief Method of hashing a text with several keys
			 *
			 * @param text  text for the hashing
			 * @param seed1 first key for the hashing
			 * @param seed2 second key for the hashing
			 * @return      hashing result
			 *
			 * \~
			 */
			AWH_HASH_INLINE T hashWithSeeds(string_view text, const uint64_t seed1, const uint64_t seed2) const noexcept {
				// Выполняем хэширование текста с ключами
				return hashing::create <T> (text.data(), text.size(), hashing::merge(seed1, seed2));
			}
			/**
			 * \~russian
			 * @brief Шаблон типа результата хэширования и типа буфера данных
			 *
			 * @tparam A тип результата хэширования
			 * @tparam B тип буфера данных для хэширования
			 *
			 * \~english
			 * @brief Template of the type of the hashing result and of the type of the data buffer
			 *
			 * @tparam A type of the hashing result
			 * @tparam B type of the data buffer for the hashing
			 *
			 * \~
			 */
			template <typename A = uint64_t, typename B, typename = decltype(std::declval <const B &> ().data())>
			/**
			 * \~russian
			 * @brief Метод хэширования буфера данных c несколькими ключами
			 *
			 * @param text  буфер данных для хэширования
			 * @param seed1 первый ключ для хэширования
			 * @param seed2 второй ключ для хэширования
			 * @return      результат хэширования
			 *
			 * \~english
			 * @brief Method of hashing a data buffer with several keys
			 *
			 * @param text  data buffer for the hashing
			 * @param seed1 first key for the hashing
			 * @param seed2 second key for the hashing
			 * @return      hashing result
			 *
			 * \~
			 */
			AWH_HASH_INLINE A hashWithSeeds(const B & text, const uint64_t seed1, const uint64_t seed2) const noexcept {
				// Выполняем хэширование буфера данных с ключами
				return hashing::create <A> (text.data(), text.size() * sizeof(typename B::value_type), hashing::merge(seed1, seed2));
			}
			/**
			 * \~russian
			 * @brief Шаблон типа результата хэширования
			 *
			 * @tparam T тип результата хэширования
			 *
			 * \~english
			 * @brief Template of the type of the hashing result
			 *
			 * @tparam T type of the hashing result
			 *
			 * \~
			 */
			template <typename T = uint64_t>
			/**
			 * \~russian
			 * @brief Метод хэширования буфера данных c несколькими ключами
			 *
			 * @param buffer буфер данных для хэширования
			 * @param size   размер данных для хэширования
			 * @param seed1  первый ключ для хэширования
			 * @param seed2  второй ключ для хэширования
			 * @return       результат хэширования
			 *
			 * \~english
			 * @brief Method of hashing a data buffer with several keys
			 *
			 * @param buffer data buffer for the hashing
			 * @param size   size of the data for the hashing
			 * @param seed1  first key for the hashing
			 * @param seed2  second key for the hashing
			 * @return       hashing result
			 *
			 * \~
			 */
			AWH_HASH_INLINE T hashWithSeeds(const void * buffer, const size_t size, const uint64_t seed1, const uint64_t seed2) const noexcept {
				// Выполняем хэширование буфера данных с ключами
				return hashing::create <T> (buffer, size, hashing::merge(seed1, seed2));
			}
		public:
			/**
			 * \~russian
			 * @brief Шаблон метода хэширования текста
			 *
			 * @tparam T тип возвращаемого результата
			 *
			 * \~english
			 * @brief Template of the method of hashing a text
			 *
			 * @tparam T type of the returned result
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод хэширования текста
			 *
			 * @param buffer буфер данных для хэширования
			 * @param hash   тип хэш-суммы
			 * @param format вид записи выработанной хэш-суммы
			 * @return       результат хэширования
			 *
			 * \~english
			 * @brief Method of hashing a text
			 *
			 * @param buffer data buffer for the hashing
			 * @param hash   type of the hash sum
			 * @param format kind of the record of the produced hash sum
			 * @return       hashing result
			 *
			 * \~
			 */
			auto hash(string_view buffer, const hash_t hash, const format_t format = format_t::HEX) const noexcept -> T;
			/**
			 * \~russian
			 * @brief Шаблон метода хэширования текста
			 *
			 * @tparam A тип возвращаемого результата
			 * @tparam B тип буфера данных
			 *
			 * \~english
			 * @brief Template of the method of hashing a text
			 *
			 * @tparam A type of the returned result
			 * @tparam B type of the data buffer
			 *
			 * \~
			 */
			template <typename A, typename B>
			/**
			 * \~russian
			 * @brief Метод хэширования текста
			 *
			 * @param buffer буфер данных для хэширования
			 * @param hash   тип хэш-суммы
			 * @param format вид записи выработанной хэш-суммы
			 * @return       результат хэширования
			 *
			 * \~english
			 * @brief Method of hashing a text
			 *
			 * @param buffer data buffer for the hashing
			 * @param hash   type of the hash sum
			 * @param format kind of the record of the produced hash sum
			 * @return       hashing result
			 *
			 * \~
			 */
			auto hash(const B & buffer, const hash_t hash, const format_t format = format_t::HEX) const noexcept -> A;
		public:
			/**
			 * \~russian
			 * @brief Шаблон метода хэширования текста с ключом
			 *
			 * @tparam T тип возвращаемого результата
			 *
			 * \~english
			 * @brief Template of the method of hashing a text with a key
			 *
			 * @tparam T type of the returned result
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод хэширования текста с ключом
			 *
			 * @param key    ключ для подписи
			 * @param buffer буфер данных для хэширования
			 * @param hash   тип хэш-суммы
			 * @param format вид записи выработанной хэш-суммы
			 * @return       результат хэширования
			 *
			 * \~english
			 * @brief Method of hashing a text with a key
			 *
			 * @param key    key for the signature
			 * @param buffer data buffer for the hashing
			 * @param hash   type of the hash sum
			 * @param format kind of the record of the produced hash sum
			 * @return       hashing result
			 *
			 * \~
			 */
			auto hmac(string_view key, string_view buffer, const hash_t hash, const format_t format = format_t::HEX) const noexcept -> T;
			/**
			 * \~russian
			 * @brief Шаблон метода хэширования текста с ключом
			 *
			 * @tparam T тип возвращаемого результата
			 *
			 * \~english
			 * @brief Template of the method of hashing a text with a key
			 *
			 * @tparam T type of the returned result
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод хэширования текста с ключом
			 *
			 * @param key    ключ для подписи
			 * @param buffer буфер данных для хэширования
			 * @param hash   тип хэш-суммы
			 * @param format вид записи выработанной хэш-суммы
			 * @return       результат хэширования
			 *
			 * \~english
			 * @brief Method of hashing a text with a key
			 *
			 * @param key    key for the signature
			 * @param buffer data buffer for the hashing
			 * @param hash   type of the hash sum
			 * @param format kind of the record of the produced hash sum
			 * @return       hashing result
			 *
			 * \~
			 */
			auto hmac(const string & key, string_view buffer, const hash_t hash, const format_t format = format_t::HEX) const noexcept -> T;
			/**
			 * \~russian
			 * @brief Шаблон метода хэширования текста с ключом
			 *
			 * @tparam A тип возвращаемого результата
			 * @tparam B тип буфера данных
			 *
			 * \~english
			 * @brief Template of the method of hashing a text with a key
			 *
			 * @tparam A type of the returned result
			 * @tparam B type of the data buffer
			 *
			 * \~
			 */
			template <typename A, typename B>
			/**
			 * \~russian
			 * @brief Метод хэширования текста с ключом
			 *
			 * @param key    ключ для подписи
			 * @param buffer буфер данных для хэширования
			 * @param hash   тип хэш-суммы
			 * @param format вид записи выработанной хэш-суммы
			 * @return       результат хэширования
			 *
			 * \~english
			 * @brief Method of hashing a text with a key
			 *
			 * @param key    key for the signature
			 * @param buffer data buffer for the hashing
			 * @param hash   type of the hash sum
			 * @param format kind of the record of the produced hash sum
			 * @return       hashing result
			 *
			 * \~
			 */
			auto hmac(string_view key, const B & buffer, const hash_t hash, const format_t format = format_t::HEX) const noexcept -> A;
			/**
			 * \~russian
			 * @brief Шаблон метода хэширования текста с ключом
			 *
			 * @tparam A тип возвращаемого результата
			 * @tparam B тип буфера данных
			 *
			 * \~english
			 * @brief Template of the method of hashing a text with a key
			 *
			 * @tparam A type of the returned result
			 * @tparam B type of the data buffer
			 *
			 * \~
			 */
			template <typename A, typename B>
			/**
			 * \~russian
			 * @brief Метод хэширования текста с ключом
			 *
			 * @param key    ключ для подписи
			 * @param buffer буфер данных для хэширования
			 * @param hash   тип хэш-суммы
			 * @param format вид записи выработанной хэш-суммы
			 * @return       результат хэширования
			 *
			 * \~english
			 * @brief Method of hashing a text with a key
			 *
			 * @param key    key for the signature
			 * @param buffer data buffer for the hashing
			 * @param hash   type of the hash sum
			 * @param format kind of the record of the produced hash sum
			 * @return       hashing result
			 *
			 * \~
			 */
			auto hmac(const string & key, const B & buffer, const hash_t hash, const format_t format = format_t::HEX) const noexcept -> A;
		public:
			/**
			 * \~russian
			 * @brief Шаблон метода кодирования
			 *
			 * @tparam T тип возвращаемого результата
			 *
			 * \~english
			 * @brief Template of the encoding method
			 *
			 * @tparam T type of the returned result
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод финализации контекста шифрования
			 *
			 * @details Завершает работу потока, заведённого методом initialize, и
			 *          освобождает его контекст. Буфер результата метод <b>дополняет</b>, а
			 *          не перезаписывает - в отличие от самих encrypt и decrypt, которые
			 *          выдают одну лишь очередную порцию. Накопление потока целиком лежит
			 *          на вызывающей стороне.
			 *
			 *          В режиме с проверкой подлинности вызов обязателен: имитовставка
			 *          дописывается здесь при шифровании и сверяется здесь при расшифровке.
			 *          Поток, не завершённый вызовом, шифротекста не даёт вовсе, а при
			 *          расшифровке оставляет открытый текст непроверенным.
			 *
			 * @note Успех работы судится возвращаемым признаком, а не длиной результата:
			 *       пустая порция - законный успех, покуда набирается вектор инициализации
			 *       либо удерживается имитовставка
			 * @param buffer буфер куда следует положить результат
			 * @return       результат финализации
			 * @see initialize
			 *
			 * @code{.cpp}
			 * std::string encoded;
			 * crypto.initialize(ch::event_t::ENCODE, ch::hash_t::SHA256, ch::cipher_t::AES256);
			 * for(auto & chunk : chunks)
			 *     // Порция заменяет буфер, накапливает вызывающая сторона
			 *     encoded.append(crypto.encrypt <std::string> (chunk.data(), chunk.size()));
			 * // Завершение дописывает вектор дополнения и имитовставку
			 * crypto.finalize(encoded);
			 * @endcode
			 *
			 * \~english
			 * @brief Method finalizing the encryption context
			 *
			 * @details Terminates the work of the stream started by the initialize method, and
			 *          releases its context. The method <b>appends to</b> the result buffer rather
			 *          than overwrites it - unlike encrypt and decrypt themselves, which
			 *          give out only the next portion. The accumulation of the whole stream lies
			 *          upon the calling side.
			 *
			 *          In the mode with the authenticity check the call is mandatory: the message authentication code
			 *          is appended here during the encryption and is verified here during the decryption.
			 *          A stream not terminated by the call gives no ciphertext at all, and during
			 *          the decryption leaves the plaintext unverified.
			 *
			 * @note The success of the work is judged by the returned sign rather than by the length of the result:
			 *       an empty portion is a legitimate success as long as the initialization vector is being gathered
			 *       or the message authentication code is being held
			 *
			 * @param buffer buffer the result should be placed into
			 * @return       result of the finalization
			 *
			 * @see initialize
			 *
			 * @code{.cpp}
			 * std::string encoded;
			 * crypto.initialize(ch::event_t::ENCODE, ch::hash_t::SHA256, ch::cipher_t::AES256);
			 * for(auto & chunk : chunks)
			 *     // A portion replaces the buffer, the calling side accumulates
			 *     encoded.append(crypto.encrypt <std::string> (chunk.data(), chunk.size()));
			 * // The completion appends the vector of the padding and the authentication tag
			 * crypto.finalize(encoded);
			 * @endcode
			 *
			 */
			bool finalize(T & buffer) noexcept;
			/**
			 * \~russian
			 * @brief Метод инициализации контекста шифрования
			 *
			 * @details Заводит поток и задаёт его направление, тип шифрования и тип
			 *          хэш-суммы вывода ключа. Дальше данные подаются порциями через
			 *          encrypt либо decrypt, а поток закрывается вызовом finalize.
			 *
			 *          Направление задаётся здесь и им же определяется: контекст,
			 *          заведённый на шифрование, расшифровать не может. Доводы порции
			 *          заведённый поток не переопределяют - незаданный берётся у потока,
			 *          заданный с потоком сверяется, а расходящийся отвергается.
			 *
			 *          Ключ, выведенный теми же приметами, выводится не заново:
			 *          пересчитывается он лишь при смене пароля, соли, типа шифрования,
			 *          типа хэш-суммы, режима блочного шифрования либо числа итераций.
			 *
			 * @note Повторный вызов заведённый прежде поток сбрасывает, а не отвергает:
			 *       сменить направление либо шифр этим и следует
			 *
			 * @param event  событие шифрования (ENCODE, DECODE)
			 * @param hash   тип хэш-суммы
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @return       результат инициализации
			 *
			 * \~english
			 * @brief Method initializing the encryption context
			 *
			 * @details Starts a stream and sets its direction, the type of the encryption and the type
			 *          of the hash sum of the key derivation. Further the data is fed in portions through
			 *          encrypt or decrypt, and the stream is closed by a call of finalize.
			 *
			 *          The direction is set here and is determined by it: a context
			 *          started for encryption cannot decrypt. The arguments of a portion
			 *          do not redefine the started stream - an unset one is taken from the stream,
			 *          a set one is verified against the stream, and a diverging one is rejected.
			 *
			 *          A key derived with the same attributes is not derived anew:
			 *          it is recomputed only upon a change of the password, of the salt, of the type of the encryption,
			 *          of the type of the hash sum, of the mode of the block encryption or of the number of the iterations.
			 *
			 * @note A repeated call resets the previously started stream rather than rejects it:
			 *       that is exactly how the direction or the cipher should be changed
			 *
			 * @param event  encryption event (ENCODE, DECODE)
			 * @param hash   type of the hash sum
			 * @param cipher type of the encryption (BASE64, AES128, AES192, AES256)
			 * @return       result of the initialization
			 *
			 * \~
			 */
			bool initialize(const event_t event, const hash_t hash, const cipher_t cipher) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Шаблон метода кодирования
			 *
			 * @tparam T тип возвращаемого результата
			 *
			 * \~english
			 * @brief Template of the encoding method
			 *
			 * @tparam T type of the returned result
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод кодирования
			 *
			 * @param buffer буфер данных для шифрования
			 * @param hash   тип хэш-суммы
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @return       результат кодирования
			 *
			 * \~english
			 * @brief Encoding method
			 *
			 * @param buffer data buffer for the encryption
			 * @param hash   type of the hash sum
			 * @param cipher type of the encryption (BASE64, AES128, AES192, AES256)
			 * @return       result of the encoding
			 *
			 * \~
			 */
			auto encrypt(string_view buffer, const hash_t hash = hash_t::NONE, const cipher_t cipher = cipher_t::NONE) const noexcept -> T;
			/**
			 * \~russian
			 * @brief Шаблон метода кодирования
			 *
			 * @tparam A тип возвращаемого результата
			 * @tparam B тип буфера данных
			 *
			 * \~english
			 * @brief Template of the encoding method
			 *
			 * @tparam A type of the returned result
			 * @tparam B type of the data buffer
			 *
			 * \~
			 */
			template <typename A, typename B>
			/**
			 * \~russian
			 * @brief Метод кодирования
			 *
			 * @param buffer буфер данных для шифрования
			 * @param hash   тип хэш-суммы
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @return       результат кодирования
			 *
			 * \~english
			 * @brief Encoding method
			 *
			 * @param buffer data buffer for the encryption
			 * @param hash   type of the hash sum
			 * @param cipher type of the encryption (BASE64, AES128, AES192, AES256)
			 * @return       result of the encoding
			 *
			 * \~
			 */
			auto encrypt(const B & buffer, const hash_t hash = hash_t::NONE, const cipher_t cipher = cipher_t::NONE) const noexcept -> A;
			/**
			 * \~russian
			 * @brief Шаблон метода кодирования
			 *
			 * @tparam T тип возвращаемого результата
			 *
			 * \~english
			 * @brief Template of the encoding method
			 *
			 * @tparam T type of the returned result
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод кодирования
			 *
			 * @param buffer буфер данных для шифрования
			 * @param size   размер данных для шифрования
			 * @param hash   тип хэш-суммы
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @return       результат кодирования
			 *
			 * \~english
			 * @brief Encoding method
			 *
			 * @param buffer data buffer for the encryption
			 * @param size   size of the data for the encryption
			 * @param hash   type of the hash sum
			 * @param cipher type of the encryption (BASE64, AES128, AES192, AES256)
			 * @return       result of the encoding
			 *
			 * \~
			 */
			auto encrypt(const void * buffer, const size_t size, const hash_t hash = hash_t::NONE, const cipher_t cipher = cipher_t::NONE) const noexcept -> T;
		public:
			/**
			 * \~russian
			 * @brief Шаблон метода кодирования с выводом признака работы
			 *
			 * @tparam T тип буфера результата
			 *
			 * \~english
			 * @brief Template of the encoding method with the output of the sign of the work
			 *
			 * @tparam T type of the result buffer
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод кодирования с выводом признака работы
			 *
			 * @details Признак работы выводится отдельно от буфера: пустой буфер
			 *          отказом не является - расшифровка сообщения, октетов не
			 *          имеющего, даёт пустой открытый текст, - и судить по нему об
			 *          удаче нельзя. Перегрузки, выводящие один лишь буфер,
			 *          оставлены для работ, которым различать это не нужно.
			 *
			 * @param buffer буфер данных для шифрования
			 * @details Поверх заведённого потока метод выдаёт одну лишь очередную порцию
			 *          выхода и буфер результата <b>перезаписывает</b>: накопление потока
			 *          целиком лежит на вызывающей стороне, а завершает поток finalize,
			 *          который буфер уже дополняет.
			 *
			 *          Вне потока метод выполняет работу целиком за один вызов.
			 *
			 * @note Успех работы судится возвращаемым признаком, а не длиной результата:
			 *       пустая порция - законный успех, покуда набирается вектор инициализации
			 *       либо удерживается имитовставка. Перегрузки, выдающие один лишь буфер,
			 *       этого различить не дают, и в потоке следует пользоваться этой
			 *
			 * @param size   размер данных для шифрования
			 * @param result буфер, куда следует положить результат
			 * @param hash   тип хэш-суммы
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @return       признак успешно выполненной работы
			 *
			 * @see initialize, finalize
			 *
			 * \~english
			 * @brief Encoding method with the output of the sign of the work
			 *
			 * @details The sign of the work is output separately from the buffer: an empty buffer
			 *          is not a failure - the decryption of a message having no
			 *          octets gives an empty plaintext, - and the success cannot be judged
			 *          by it. The overloads outputting the buffer alone
			 *          have been left for the works that need not distinguish this.
			 *
			 * @param buffer data buffer for the encryption
			 * @details On top of a started stream the method gives out only the next portion
			 *          of the output and <b>overwrites</b> the result buffer: the accumulation of the whole
			 *          stream lies upon the calling side, and the stream is terminated by finalize,
			 *          which already appends to the buffer.
			 *
			 *          Outside a stream the method performs the whole work within a single call.
			 *
			 * @note The success of the work is judged by the returned sign rather than by the length of the result:
			 *       an empty portion is a legitimate success as long as the initialization vector is being gathered
			 *       or the message authentication code is being held. The overloads giving out the buffer alone
			 *       do not allow distinguishing this, and within a stream this one should be used
			 *
			 * @param size   size of the data for the encryption
			 * @param result buffer the result should be placed into
			 * @param hash   type of the hash sum
			 * @param cipher type of the encryption (BASE64, AES128, AES192, AES256)
			 * @return       sign of the successfully performed work
			 *
			 * @see initialize, finalize
			 *
			 * \~
			 */
			bool encrypt(const void * buffer, const size_t size, T & result, const hash_t hash = hash_t::NONE, const cipher_t cipher = cipher_t::NONE) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Шаблон метода декодирования
			 *
			 * @tparam T тип возвращаемого результата
			 *
			 * \~english
			 * @brief Template of the decoding method
			 *
			 * @tparam T type of the returned result
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод декодирования
			 *
			 * @param buffer буфер данных для шифрования
			 * @param hash   тип хэш-суммы
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @return       результат кодирования
			 *
			 * \~english
			 * @brief Decoding method
			 *
			 * @param buffer data buffer for the encryption
			 * @param hash   type of the hash sum
			 * @param cipher type of the encryption (BASE64, AES128, AES192, AES256)
			 * @return       result of the encoding
			 *
			 * \~
			 */
			auto decrypt(string_view buffer, const hash_t hash = hash_t::NONE, const cipher_t cipher = cipher_t::NONE) const noexcept -> T;
			/**
			 * \~russian
			 * @brief Шаблон метода декодирования
			 *
			 * @tparam A тип возвращаемого результата
			 * @tparam B тип буфера данных
			 *
			 * \~english
			 * @brief Template of the decoding method
			 *
			 * @tparam A type of the returned result
			 * @tparam B type of the data buffer
			 *
			 * \~
			 */
			template <typename A, typename B>
			/**
			 * \~russian
			 * @brief Метод декодирования
			 *
			 * @param buffer буфер данных для шифрования
			 * @param hash   тип хэш-суммы
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @return       результат кодирования
			 *
			 * \~english
			 * @brief Decoding method
			 *
			 * @param buffer data buffer for the encryption
			 * @param hash   type of the hash sum
			 * @param cipher type of the encryption (BASE64, AES128, AES192, AES256)
			 * @return       result of the encoding
			 *
			 * \~
			 */
			auto decrypt(const B & buffer, const hash_t hash = hash_t::NONE, const cipher_t cipher = cipher_t::NONE) const noexcept -> A;
			/**
			 * \~russian
			 * @brief Шаблон метода декодирования
			 *
			 * @tparam T тип возвращаемого результата
			 *
			 * \~english
			 * @brief Template of the decoding method
			 *
			 * @tparam T type of the returned result
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод декодирования
			 *
			 * @param buffer буфер данных для шифрования
			 * @param size   размер данных для шифрования
			 * @param hash   тип хэш-суммы
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @return       результат кодирования
			 *
			 * \~english
			 * @brief Decoding method
			 *
			 * @param buffer data buffer for the encryption
			 * @param size   size of the data for the encryption
			 * @param hash   type of the hash sum
			 * @param cipher type of the encryption (BASE64, AES128, AES192, AES256)
			 * @return       result of the encoding
			 *
			 * \~
			 */
			auto decrypt(const void * buffer, const size_t size, const hash_t hash = hash_t::NONE, const cipher_t cipher = cipher_t::NONE) const noexcept -> T;
		public:
			/**
			 * \~russian
			 * @brief Шаблон метода декодирования с выводом признака работы
			 *
			 * @tparam T тип буфера результата
			 *
			 * \~english
			 * @brief Template of the decoding method with the output of the sign of the work
			 *
			 * @tparam T type of the result buffer
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод декодирования с выводом признака работы
			 *
			 * @details Признак работы выводится отдельно от буфера: пустой буфер
			 *          отказом не является - расшифровка сообщения, октетов не
			 *          имеющего, даёт пустой открытый текст, - и судить по нему об
			 *          удаче нельзя. Перегрузки, выводящие один лишь буфер,
			 *          оставлены для работ, которым различать это не нужно.
			 *
			 * @param buffer буфер данных для шифрования
			 * @details Поверх заведённого потока метод выдаёт одну лишь очередную порцию
			 *          выхода и буфер результата <b>перезаписывает</b>: накопление потока
			 *          целиком лежит на вызывающей стороне, а завершает поток finalize,
			 *          который буфер уже дополняет.
			 *
			 *          Вне потока метод выполняет работу целиком за один вызов.
			 *
			 * @note Успех работы судится возвращаемым признаком, а не длиной результата:
			 *       пустая порция - законный успех, покуда набирается вектор инициализации
			 *       либо удерживается имитовставка. Перегрузки, выдающие один лишь буфер,
			 *       этого различить не дают, и в потоке следует пользоваться этой
			 *
			 * @param size   размер данных для шифрования
			 * @param result буфер, куда следует положить результат
			 * @param hash   тип хэш-суммы
			 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
			 * @return       признак успешно выполненной работы
			 *
			 * @see initialize, finalize
			 *
			 * \~english
			 * @brief Decoding method with the output of the sign of the work
			 *
			 * @details The sign of the work is output separately from the buffer: an empty buffer
			 *          is not a failure - the decryption of a message having no
			 *          octets gives an empty plaintext, - and the success cannot be judged
			 *          by it. The overloads outputting the buffer alone
			 *          have been left for the works that need not distinguish this.
			 *
			 * @param buffer data buffer for the encryption
			 * @details On top of a started stream the method gives out only the next portion
			 *          of the output and <b>overwrites</b> the result buffer: the accumulation of the whole
			 *          stream lies upon the calling side, and the stream is terminated by finalize,
			 *          which already appends to the buffer.
			 *
			 *          Outside a stream the method performs the whole work within a single call.
			 *
			 * @note The success of the work is judged by the returned sign rather than by the length of the result:
			 *       an empty portion is a legitimate success as long as the initialization vector is being gathered
			 *       or the message authentication code is being held. The overloads giving out the buffer alone
			 *       do not allow distinguishing this, and within a stream this one should be used
			 *
			 * @param size   size of the data for the encryption
			 * @param result buffer the result should be placed into
			 * @param hash   type of the hash sum
			 * @param cipher type of the encryption (BASE64, AES128, AES192, AES256)
			 * @return       sign of the successfully performed work
			 *
			 * @see initialize, finalize
			 *
			 * \~
			 */
			bool decrypt(const void * buffer, const size_t size, T & result, const hash_t hash = hash_t::NONE, const cipher_t cipher = cipher_t::NONE) const noexcept;
		public:
			/**
			 * \~russian
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
			 * \~english
			 * @brief Method generating a private RSA key
			 *
			 * @details The width is accepted as any from two thousand and forty eight and
			 *          above, and not only the customary 2048, 3072 and 4096: the width
			 *          is a property of the caller, and there is nothing to limit it by
			 *          a list. A lesser width is rejected - it has no strength.
			 *          An unset width means two thousand and forty eight.
			 *
			 * @param size size of the key in bits, from 2048 and above (0 — by default)
			 * @return     result of the key generation
			 *
			 * \~
			 */
			bool generatePrivateKeyRSA(const size_t size = 0) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения публичного ключа RSA
			 *
			 * @return публичный ключ RSA
			 *
			 * \~english
			 * @brief Method obtaining the public RSA key
			 *
			 * @return public RSA key
			 *
			 * \~
			 */
			string getPublicKeyRSA() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки публичного ключа RSA
			 *
			 * @param key публичный ключ RSA
			 * @return    результат установки ключа
			 *
			 * \~english
			 * @brief Method setting the public RSA key
			 *
			 * @param key public RSA key
			 * @return    result of setting the key
			 *
			 * \~
			 */
			bool setPublicKeyRSA(string_view key) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки приватного ключа RSA
			 *
			 * @param key приватный ключ RSA
			 * @return    результат установки ключа
			 *
			 * \~english
			 * @brief Method setting the private RSA key
			 *
			 * @param key private RSA key
			 * @return    result of setting the key
			 *
			 * \~
			 */
			bool setPrivateKeyRSA(string_view key) noexcept;
			/**
			 * \~russian
			 * @brief Метод получения приватного ключа RSA
			 *
			 * @param cipher тип шифрования приватного ключа
			 * @return       приватный ключ RSA
			 *
			 * \~english
			 * @brief Method obtaining the private RSA key
			 *
			 * @param cipher type of the encryption of the private key
			 * @return       private RSA key
			 *
			 * \~
			 */
			string getPrivateKeyRSA(const cipher_t cipher = cipher_t::NONE) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод загрузки публичного ключа RSA из файла
			 *
			 * @param path путь к файлу с публичным ключом
			 * @return     результат загрузки ключа
			 *
			 * \~english
			 * @brief Method loading the public RSA key from a file
			 *
			 * @param path path to the file with the public key
			 * @return     result of loading the key
			 *
			 * \~
			 */
			bool loadPublicKeyRSA(string_view path) noexcept;
			/**
			 * \~russian
			 * @brief Метод загрузки приватного ключа RSA из файла
			 *
			 * @param path путь к файлу с приватным ключом
			 * @return     результат загрузки ключа
			 *
			 * \~english
			 * @brief Method loading the private RSA key from a file
			 *
			 * @param path path to the file with the private key
			 * @return     result of loading the key
			 *
			 * \~
			 */
			bool loadPrivateKeyRSA(string_view path) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод сохранения публичного ключа RSA в файл
			 *
			 * @param path путь к файлу для сохранения публичного ключа
			 * @return     результат сохранения ключа
			 *
			 * \~english
			 * @brief Method saving the public RSA key into a file
			 *
			 * @param path path to the file for saving the public key
			 * @return     result of saving the key
			 *
			 * \~
			 */
			bool savePublicKeyRSA(string_view path) const noexcept;
			/**
			 * \~russian
			 * @brief Метод сохранения приватного ключа RSA в файл
			 *
			 * @param path   путь к файлу для сохранения приватного ключа
			 * @param cipher тип шифрования приватного ключа
			 * @return       результат сохранения ключа
			 *
			 * \~english
			 * @brief Method saving the private RSA key into a file
			 *
			 * @param path   path to the file for saving the private key
			 * @param cipher type of the encryption of the private key
			 * @return       result of saving the key
			 *
			 * \~
			 */
			bool savePrivateKeyRSA(string_view path, const cipher_t cipher = cipher_t::NONE) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод шифрования данных публичным ключом RSA
			 *
			 * @param buffer буфер данных для шифрования
			 * @param result буфер куда следует положить результат
			 * @return       признак успешно выполненной работы
			 *
			 * \~english
			 * @brief Method encrypting the data by the public RSA key
			 *
			 * @param buffer data buffer for the encryption
			 * @param result buffer the result should be placed into
			 * @return       sign of the successfully performed work
			 *
			 * \~
			 */
			bool encryptWithPublicKey(const vector <uint8_t> & buffer, vector <uint8_t> & result) const noexcept;
			/**
			 * \~russian
			 * @brief Метод шифрования данных публичным ключом RSA
			 *
			 * @param buffer буфер данных для шифрования
			 * @param size   размер данных для шифрования
			 * @param result буфер куда следует положить результат
			 * @return       признак успешно выполненной работы
			 *
			 * \~english
			 * @brief Method encrypting the data by the public RSA key
			 *
			 * @param buffer data buffer for the encryption
			 * @param size   size of the data for the encryption
			 * @param result buffer the result should be placed into
			 * @return       sign of the successfully performed work
			 *
			 * \~
			 */
			bool encryptWithPublicKey(const uint8_t * buffer, const size_t size, vector <uint8_t> & result) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод дешифрования данных приватным ключом RSA
			 *
			 * @param buffer буфер данных для дешифрования
			 * @param result буфер куда следует положить результат
			 * @return       признак успешно выполненной работы
			 *
			 * \~english
			 * @brief Method decrypting the data by the private RSA key
			 *
			 * @param buffer data buffer for the decryption
			 * @param result buffer the result should be placed into
			 * @return       sign of the successfully performed work
			 *
			 * \~
			 */
			bool decryptWithPrivateKey(const vector <uint8_t> & buffer, vector <uint8_t> & result) const noexcept;
			/**
			 * \~russian
			 * @brief Метод дешифрования данных приватным ключом RSA
			 *
			 * @param buffer буфер данных для дешифрования
			 * @param size   размер данных для дешифрования
			 * @param result буфер куда следует положить результат
			 * @return       признак успешно выполненной работы
			 *
			 * \~english
			 * @brief Method decrypting the data by the private RSA key
			 *
			 * @param buffer data buffer for the decryption
			 * @param size   size of the data for the decryption
			 * @param result buffer the result should be placed into
			 * @return       sign of the successfully performed work
			 *
			 * \~
			 */
			bool decryptWithPrivateKey(const uint8_t * buffer, const size_t size, vector <uint8_t> & result) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод подписания данных приватным ключом RSA
			 *
			 * @param buffer буфер данных для подписи
			 * @param hash   тип хэш-суммы
			 * @param result буфер куда следует положить результат
			 * @return       признак успешно выполненной работы
			 *
			 * \~english
			 * @brief Method signing the data by the private RSA key
			 *
			 * @param buffer data buffer for the signature
			 * @param hash   type of the hash sum
			 * @param result buffer the result should be placed into
			 * @return       sign of the successfully performed work
			 *
			 * \~
			 */
			bool signWithPrivateKey(const vector <uint8_t> & buffer, const hash_t hash, vector <uint8_t> & result) const noexcept;
			/**
			 * \~russian
			 * @brief Метод подписания данных приватным ключом RSA
			 *
			 * @param buffer буфер данных для подписи
			 * @param size   размер данных для подписи
			 * @param hash   тип хэш-суммы
			 * @param result буфер куда следует положить результат
			 * @return       признак успешно выполненной работы
			 *
			 * \~english
			 * @brief Method signing the data by the private RSA key
			 *
			 * @param buffer data buffer for the signature
			 * @param size   size of the data for the signature
			 * @param hash   type of the hash sum
			 * @param result buffer the result should be placed into
			 * @return       sign of the successfully performed work
			 *
			 * \~
			 */
			bool signWithPrivateKey(const uint8_t * buffer, const size_t size, const hash_t hash, vector <uint8_t> & result) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод верификации данных публичным ключом RSA
			 *
			 * @param buffer    буфер данных для верификации
			 * @param signature буфер с подписью данных
			 * @param hash      тип хэш-суммы
			 * @return          результат верификации
			 *
			 * \~english
			 * @brief Method verifying the data by the public RSA key
			 *
			 * @param buffer    data buffer for the verification
			 * @param signature buffer with the signature of the data
			 * @param hash      type of the hash sum
			 * @return          result of the verification
			 *
			 * \~
			 */
			bool verifyWithPublicKey(const vector <uint8_t> & buffer, const vector <uint8_t> & signature, const hash_t hash) const noexcept;
			/**
			 * \~russian
			 * @brief Метод верификации данных публичным ключом RSA
			 *
			 * @param buffer    буфер данных для верификации
			 * @param size      размер данных для верификации
			 * @param signature буфер с подписью данных
			 * @param hash      тип хэш-суммы
			 * @return          результат верификации
			 *
			 * \~english
			 * @brief Method verifying the data by the public RSA key
			 *
			 * @param buffer    data buffer for the verification
			 * @param size      size of the data for the verification
			 * @param signature buffer with the signature of the data
			 * @param hash      type of the hash sum
			 * @return          result of the verification
			 *
			 * \~
			 */
			bool verifyWithPublicKey(const uint8_t * buffer, const size_t size, const vector <uint8_t> & signature, const hash_t hash) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор копирования
			 *
			 * @details Копирование запрещено: объект владеет состоянием шифрования и
			 *          ключом RSA по указателям, и поверхностная копия освободила бы
			 *          контекст библиотеки криптографии дважды. Тайну копия к тому же
			 *          разносила бы вширь - пароли и выведенный ключ достались бы ей
			 *          заодно, а гасит их лишь тот, кто их завёл.
			 *
			 * \~english
			 * @brief Copy assignment operator
			 *
			 * @details Copying is forbidden: the object owns the encryption state and
			 *          the RSA key by pointers, and a shallow copy would release
			 *          the context of the cryptographic library twice. A copy would besides
			 *          spread the secret wider - the passwords and the derived key would go to it
			 *          as well, and only the one who started them extinguishes them.
			 *
			 * \~
			 */
			Crypto & operator = (const Crypto &) = delete;
			/**
			 * \~russian
			 * @brief Оператор переноса
			 *
			 * \~english
			 * @brief Move assignment operator
			 *
			 * \~
			 */
			Crypto & operator = (Crypto &&) = delete;
		public:
			/**
			 * \~russian
			 * @brief Конструктор копирования
			 *
			 * \~english
			 * @brief Copy constructor
			 *
			 * \~
			 */
			Crypto(const Crypto &) = delete;
			/**
			 * \~russian
			 * @brief Конструктор переноса
			 *
			 * \~english
			 * @brief Move constructor
			 *
			 * \~
			 */
			Crypto(Crypto &&) = delete;
		public:
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * @param fmk framework object
			 * @param log object for working with logs
			 *
			 * \~
			 */
			explicit Crypto(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * \~russian
			 * @brief Деструктор
			 *
			 * \~english
			 * @brief Destructor
			 *
			 * \~
			 */
			~Crypto() noexcept;
	} crypto_t;
};

#endif // __AWH_CRYPTO__
