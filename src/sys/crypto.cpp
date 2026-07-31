/**
 * @file: crypto.cpp
 * @date: 2026-01-20
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация модуля криптографии — симметричное шифрование блоками AES, хеширование (MD5, SHA, HMAC),
 *        кодирование Base64 и работа с ключами RSA поверх криптографической библиотеки
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Заголовочные файлы OpenSSL
 */
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

/**
 * Если размер имитовставки режима с проверкой подлинности не определён
 */
#ifndef AWH_CRYPTO_TAG_SIZE
	/**
	 * Устанавливаем размер имитовставки по умолчанию — полную разрядность счётчика Галуа
	 */
	#define AWH_CRYPTO_TAG_SIZE 16
#endif

/**
 * Если наименьшая разрядность ключа RSA не определена
 */
#ifndef AWH_CRYPTO_RSA_BITS
	/**
	 * Устанавливаем наименьшую разрядность ключа RSA — ключ короче стойкости не имеет
	 */
	#define AWH_CRYPTO_RSA_BITS 2048
#endif

/**
 * Системные заголовочные файлы
 */
#include <cstdio>
#include <cstring>
#include <csignal>
#include <sys/stat.h>
#include <sys/types.h>

/**
 * Подключаем заголовочный файл проекта
 */
#include <sys/crypto.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Пространство имён AWH
 *
 */
namespace awh {
	/**
	 * @brief Полное определение непрозрачного контекста ключа RSA
	 *
	 * @details Обёртка над контекстом OpenSSL, скрывающая прямую зависимость от OpenSSL в публичном заголовке
	 *
	 */
	struct key_rsa_t {
		// Контекст RSA ключа
		EVP_PKEY * ctx;
		// Тип RSA ключа
		crypto_t::key_type_t type;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit key_rsa_t() noexcept :
		 ctx(nullptr), type(crypto_t::key_type_t::NONE) {}
	};
	/**
	 * @brief Полное определение непрозрачного контекста стейта AES-шифрования
	 *
	 * @details Обёртка над контекстом OpenSSL, скрывающая прямую зависимость от OpenSSL в публичном заголовке
	 *
	 */
	struct state_t {
		// Тип хэш-суммы
		crypto_t::hash_t hash;
		// Тип шифрования
		crypto_t::cipher_t cipher;
		// Режим блочного шифрования
		crypto_t::mode_t mode;
		/**
		 * Направление работы потокового шифрования
		 *
		 * @details Направление задаётся при заведении контекста и им же
		 *          определяется: контекст, заведённый на шифрование, расшифровать
		 *          не может. Прежде оно нигде не хранилось, и вызов расшифровки
		 *          поверх контекста шифрования молча шифровал ещё раз
		 *
		 */
		crypto_t::event_t event;
		/**
		 * Признак ожидания вектора инициализации
		 *
		 * @details Вектор инициализации передаётся в начале шифротекста: при
		 *          шифровании он выписывается первой порцией выхода, при расшифровке
		 *          вычитывается из первой порции входа, и до этого контекст
		 *          расшифровки завести нечем
		 *
		 */
		bool pending;
		// Ключ шифрования
		vector <uint8_t> key;
		/**
		 * Удерживаемый хвост потока
		 *
		 * @details Служит двум надобностям по очереди: сперва накапливает вектор
		 *          инициализации, покуда он не придёт целиком, затем удерживает
		 *          последние октеты потока - имитовставка стоит в самом конце
		 *          шифротекста, а какая порция окажется последней, до завершения
		 *          работы неизвестно
		 *
		 */
		vector <uint8_t> tail;
		// Вектор инициализации
		vector <uint8_t> ivec;
		// Контекст выбранного шифра
		const EVP_CIPHER * evp;
		// Контекст шифрования
		const EVP_CIPHER_CTX * ctx;
		/**
		 * @brief Метод сброса стейта AES-шифрования
		 *
		 * @details Стейт сбрасывался присвоением заново созданного объекта, а
		 *          контекст шифрования принадлежит библиотеке криптографии и
		 *          присвоением не освобождается: смена пароля, соли либо числа
		 *          итераций после начала потокового шифрования теряла контекст
		 *          безвозвратно.
		 *
		 *          Ключ и вектор инициализации затираются, а не просто
		 *          освобождаются: выведенный из пароля ключ остаётся в памяти
		 *          после освобождения набора и достаётся тому, кому эта память
		 *          выдана следующей
		 *
		 */
		void reset() noexcept {
			// Если контекст шифрования заведён
			if(this->ctx != nullptr){
				// Освобождаем контекст шифрования
				::EVP_CIPHER_CTX_free(const_cast <EVP_CIPHER_CTX *> (this->ctx));
				// Зануляем контекст шифрования
				this->ctx = nullptr;
			}
			// Если ключ шифрования выведен
			if(!this->key.empty())
				// Выполняем затирание ключа шифрования
				::OPENSSL_cleanse(this->key.data(), this->key.size());
			// Если вектор инициализации выведен
			if(!this->ivec.empty())
				// Выполняем затирание вектора инициализации
				::OPENSSL_cleanse(this->ivec.data(), this->ivec.size());
			// Освобождаем ключ шифрования
			this->key.clear();
			// Если удерживаемый хвост потока не пуст
			if(!this->tail.empty())
				// Выполняем затирание удерживаемого хвоста потока
				::OPENSSL_cleanse(this->tail.data(), this->tail.size());
			// Освобождаем вектор инициализации
			this->ivec.clear();
			// Освобождаем удерживаемый хвост потока
			this->tail.clear();
			// Сбрасываем признак ожидания вектора инициализации
			this->pending = true;
			// Сбрасываем контекст выбранного шифра
			this->evp = nullptr;
			// Сбрасываем тип хэш-суммы
			this->hash = crypto_t::hash_t::NONE;
			// Сбрасываем тип шифрования
			this->cipher = crypto_t::cipher_t::NONE;
			// Сбрасываем режим блочного шифрования
			this->mode = crypto_t::mode_t::NONE;
			// Сбрасываем направление работы потокового шифрования
			this->event = crypto_t::event_t::NONE;
		}
		/**
		 * @brief Конструктор
		 *
		 */
		explicit state_t() noexcept :
		 hash(crypto_t::hash_t::NONE),
		 cipher(crypto_t::cipher_t::NONE),
		 mode(crypto_t::mode_t::NONE),
		 event(crypto_t::event_t::NONE), pending(true),
		 evp(nullptr), ctx(nullptr) {}
		/**
		 * @brief Деструктор
		 *
		 */
		~state_t() noexcept {
			// Выполняем сброс стейта AES-шифрования
			this->reset();
		}
	};
};

/**
 * @brief пространство имён драйвера
 *
 */
namespace driver {
	/**
	 * Подписываемся на пространства имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Шаблон функции затирания и очистки буфера результата
	 *
	 * @tparam T тип буфера результата
	 *
	 */
	template <typename T>
	/**
	 * @brief Функция затирания и очистки буфера результата
	 *
	 * @details Отказ работы буфер лишь очищал, тогда как очистка содержимого не
	 *          гасит: открытый текст, уже выписанный в буфер, оставался лежать в
	 *          памяти до тех пор, покуда её не выдадут кому-то ещё. Подделка
	 *          шифротекста тем самым выдавала открытый текст в кучу.
	 *
	 * @param result буфер результата, подлежащий затиранию
	 *
	 */
	static void wipe(T & result) noexcept {
		// Если буфер результата не пуст
		if(!result.empty())
			// Выполняем затирание буфера результата
			::OPENSSL_cleanse(result.data(), result.size());
		// Выполняем очистку буфера результата
		result.clear();
	}
	/**
	 * @brief Шаблон функции преобразования бинарного буфера в HEX-строку
	 *
	 * @tparam T тип результирующего буфера
	 *
	 */
	template <typename T>
	/**
	 * @brief Функция преобразования бинарного буфера в HEX-строку
	 *
	 * @param digest бинарный буфер для преобразования
	 * @param length размер бинарного буфера в байтах
	 * @param result результирующий буфер (должен быть выделен под length * 2 байт)
	 *
	 */
	static void hex(const uint8_t * digest, const size_t length, T & result) noexcept {
		// Таблица символов для формирования HEX-строки (кросс-платформенно, без sprintf/snprintf)
		static constexpr char alphabet[] = "0123456789abcdef";
		/**
		 * Выполняем перебор всех байт бинарного буфера
		 */
		for(size_t i = 0; i < length; i++){
			// Формируем старший полубайт
			result[i * 2] = alphabet[(digest[i] >> 4) & 0x0F];
			// Формируем младший полубайт
			result[(i * 2) + 1] = alphabet[digest[i] & 0x0F];
		}
	}
	/**
	 * @brief Шаблон функции хэширования текста
	 *
	 * @tparam A тип возвращаемого результата
	 * @tparam B тип буфера данных
	 *
	 */
	template <typename A, typename B>
	/**
	 * @brief Функция хэширования текста
	 *
	 * @param buffer буфер для хэширования
	 * @param hash   тип хэш-суммы
	 * @param result результат хэширования
	 * @param log    объект для работы с логами
	 *
	 */
	static void hash(const B & buffer, const crypto_t::hash_t hash, A & result, const log_t * log) noexcept {
		// Если буфер для хэширования передан
		if(!buffer.empty()){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Выполняем очистку блока с результатом
				result.clear();
				// Буфер промежуточных значений
				vector <uint8_t> digest;
				/**
				 * Определяем тип хэш-суммы
				 */
				switch(static_cast <uint8_t> (hash)){
					// Если тип хэш-суммы указан как MD5
					case static_cast <uint8_t> (crypto_t::hash_t::MD5): {
						// Создаем контекст
						::MD5_CTX ctx;
						// Выполняем инициализацию контекста
						::MD5_Init(&ctx);
						// Выделяем память для промежуточных значений
						digest.resize(16, 0);
						// Выделяем память для буфера данных
						result.resize(32, 0);
						// Выполняем расчет суммы
						::MD5_Update(&ctx, buffer.data(), buffer.size());
						// Копируем полученные данные
						::MD5_Final(digest.data(), &ctx);
						// Формируем данные MD5-хэша
						driver::hex(digest.data(), 16, result);
					} break;
					// Если тип хэш-суммы указан как SHA1
					case static_cast <uint8_t> (crypto_t::hash_t::SHA1): {
						// Создаем контекст
						::SHA_CTX ctx;
						// Выполняем инициализацию контекста
						::SHA1_Init(&ctx);
						// Выделяем память для промежуточных значений
						digest.resize(20, 0);
						// Выделяем память для буфера данных
						result.resize(40, 0);
						// Выполняем расчет суммы
						::SHA1_Update(&ctx, buffer.data(), buffer.size());
						// Копируем полученные данные
						::SHA1_Final(digest.data(), &ctx);
						// Формируем данные SHA1-хэша
						driver::hex(digest.data(), 20, result);
					} break;
					// Если тип хэш-суммы указан как SHA224
					case static_cast <uint8_t> (crypto_t::hash_t::SHA224): {
						// Создаем контекст
						::SHA256_CTX ctx;
						// Выполняем инициализацию контекста
						::SHA224_Init(&ctx);
						// Выделяем память для промежуточных значений
						digest.resize(28, 0);
						// Выделяем память для буфера данных
						result.resize(56, 0);
						// Выполняем расчет суммы
						::SHA224_Update(&ctx, buffer.data(), buffer.size());
						// Копируем полученные данные
						::SHA224_Final(digest.data(), &ctx);
						// Формируем данные SHA224-хэша
						driver::hex(digest.data(), 28, result);
					} break;
					// Если тип хэш-суммы указан как SHA256
					case static_cast <uint8_t> (crypto_t::hash_t::SHA256): {
						// Создаем контекст
						::SHA256_CTX ctx;
						// Выполняем инициализацию контекста
						::SHA256_Init(&ctx);
						// Выделяем память для промежуточных значений
						digest.resize(32, 0);
						// Выделяем память для буфера данных
						result.resize(64, 0);
						// Выполняем расчет суммы
						::SHA256_Update(&ctx, buffer.data(), buffer.size());
						// Копируем полученные данные
						::SHA256_Final(digest.data(), &ctx);
						// Формируем данные SHA256-хэша
						driver::hex(digest.data(), 32, result);
					} break;
					// Если тип хэш-суммы указан как SHA384
					case static_cast <uint8_t> (crypto_t::hash_t::SHA384): {
						// Создаем контекст
						::SHA512_CTX ctx;
						// Выполняем инициализацию контекста
						::SHA384_Init(&ctx);
						// Выделяем память для промежуточных значений
						digest.resize(48, 0);
						// Выделяем память для буфера данных
						result.resize(96, 0);
						// Выполняем расчет суммы
						::SHA384_Update(&ctx, buffer.data(), buffer.size());
						// Копируем полученные данные
						::SHA384_Final(digest.data(), &ctx);
						// Формируем данные SHA384-хэша
						driver::hex(digest.data(), 48, result);
					} break;
					// Если тип хэш-суммы указан как SHA512
					case static_cast <uint8_t> (crypto_t::hash_t::SHA512): {
						// Создаем контекст
						::SHA512_CTX ctx;
						// Выполняем инициализацию контекста
						::SHA512_Init(&ctx);
						// Выделяем память для промежуточных значений
						digest.resize(64, 0);
						// Выделяем память для буфера данных
						result.resize(128, 0);
						// Выполняем расчет суммы
						::SHA512_Update(&ctx, buffer.data(), buffer.size());
						// Копируем полученные данные
						::SHA512_Final(digest.data(), &ctx);
						// Формируем данные SHA512-хэша
						driver::hex(digest.data(), 64, result);
					} break;
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выполняем очистку блока с результатом
				result.clear();
				// Записываем ошибку в лог в лог
				log->print("%s", log_t::flag_t::CRITICAL, error.what());
			}
		}
	}
	/**
	 * @brief Шаблон функции хэширования текста с ключом
	 *
	 * @tparam A тип возвращаемого результата
	 * @tparam B тип буфера данных
	 *
	 */
	template <typename A, typename B, typename C>
	/**
	 * @brief Функция хэширования текста с ключом
	 *
	 * @param key    ключ для подписи
	 * @param buffer буфер для хэширования
	 * @param hash   тип хэш-суммы
	 * @param result результат хэширования
	 * @param log    объект для работы с логами
	 *
	 */
	static void hmac(const C & key, const B & buffer, const crypto_t::hash_t hash, A & result, const log_t * log) noexcept {
		// Если ключ и текст для хэширования переданы
		if(!key.empty() && !buffer.empty()){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Выполняем очистку блока с результатом
				result.clear();
				/**
				 * Определяем тип хэш-суммы
				 */
				switch(static_cast <uint8_t> (hash)){
					// Если тип хэш-суммы указан как HMAC_MD5
					case static_cast <uint8_t> (crypto_t::hash_t::MD5): {
						// Выделяем память для буфера данных
						result.resize(32, 0);
						// Буфер для бинарного результата подписи
						uint8_t digest[EVP_MAX_MD_SIZE] = {0};
						/**
						 * Отказ выработки подписи оставлял в буфере нули, и наружу уходила
						 * шестнадцатеричная запись нулей, от настоящей подписи неотличимая
						 */
						// Выполняем получение подписи
						if(::HMAC(::EVP_md5(), key.data(), key.size(), reinterpret_cast <const uint8_t *> (buffer.data()), buffer.size(), digest, nullptr) == nullptr)
							// Выполняем очистку блока с результатом
							result.clear();
						// Формируем данные MD5-хэша
						else driver::hex(digest, 16, result);
					} break;
					// Если тип хэш-суммы указан как HMAC_SHA1
					case static_cast <uint8_t> (crypto_t::hash_t::SHA1): {
						// Выделяем память для буфера данных
						result.resize(40, 0);
						// Буфер для бинарного результата подписи
						uint8_t digest[EVP_MAX_MD_SIZE] = {0};
						/**
						 * Отказ выработки подписи оставлял в буфере нули, и наружу уходила
						 * шестнадцатеричная запись нулей, от настоящей подписи неотличимая
						 */
						// Выполняем получение подписи
						if(::HMAC(::EVP_sha1(), key.data(), key.size(), reinterpret_cast <const uint8_t *> (buffer.data()), buffer.size(), digest, nullptr) == nullptr)
							// Выполняем очистку блока с результатом
							result.clear();
						// Формируем данные SHA1-хэша
						else driver::hex(digest, 20, result);
					} break;
					// Если тип хэш-суммы указан как HMAC_SHA224
					case static_cast <uint8_t> (crypto_t::hash_t::SHA224): {
						// Выделяем память для буфера данных
						result.resize(56, 0);
						// Буфер для бинарного результата подписи
						uint8_t digest[EVP_MAX_MD_SIZE] = {0};
						/**
						 * Отказ выработки подписи оставлял в буфере нули, и наружу уходила
						 * шестнадцатеричная запись нулей, от настоящей подписи неотличимая
						 */
						// Выполняем получение подписи
						if(::HMAC(::EVP_sha224(), key.data(), key.size(), reinterpret_cast <const uint8_t *> (buffer.data()), buffer.size(), digest, nullptr) == nullptr)
							// Выполняем очистку блока с результатом
							result.clear();
						// Формируем данные SHA224-хэша
						else driver::hex(digest, 28, result);
					} break;
					// Если тип хэш-суммы указан как HMAC_SHA256
					case static_cast <uint8_t> (crypto_t::hash_t::SHA256): {
						// Выделяем память для буфера данных
						result.resize(64, 0);
						// Буфер для бинарного результата подписи
						uint8_t digest[EVP_MAX_MD_SIZE] = {0};
						/**
						 * Отказ выработки подписи оставлял в буфере нули, и наружу уходила
						 * шестнадцатеричная запись нулей, от настоящей подписи неотличимая
						 */
						// Выполняем получение подписи
						if(::HMAC(::EVP_sha256(), key.data(), key.size(), reinterpret_cast <const uint8_t *> (buffer.data()), buffer.size(), digest, nullptr) == nullptr)
							// Выполняем очистку блока с результатом
							result.clear();
						// Формируем данные SHA256-хэша
						else driver::hex(digest, 32, result);
					} break;
					// Если тип хэш-суммы указан как HMAC_SHA384
					case static_cast <uint8_t> (crypto_t::hash_t::SHA384): {
						// Выделяем память для буфера данных
						result.resize(96, 0);
						// Буфер для бинарного результата подписи
						uint8_t digest[EVP_MAX_MD_SIZE] = {0};
						/**
						 * Отказ выработки подписи оставлял в буфере нули, и наружу уходила
						 * шестнадцатеричная запись нулей, от настоящей подписи неотличимая
						 */
						// Выполняем получение подписи
						if(::HMAC(::EVP_sha384(), key.data(), key.size(), reinterpret_cast <const uint8_t *> (buffer.data()), buffer.size(), digest, nullptr) == nullptr)
							// Выполняем очистку блока с результатом
							result.clear();
						// Формируем данные SHA384-хэша
						else driver::hex(digest, 48, result);
					} break;
					// Если тип хэш-суммы указан как HMAC_SHA512
					case static_cast <uint8_t> (crypto_t::hash_t::SHA512): {
						// Выделяем память для буфера данных
						result.resize(128, 0);
						// Буфер для бинарного результата подписи
						uint8_t digest[EVP_MAX_MD_SIZE] = {0};
						/**
						 * Отказ выработки подписи оставлял в буфере нули, и наружу уходила
						 * шестнадцатеричная запись нулей, от настоящей подписи неотличимая
						 */
						// Выполняем получение подписи
						if(::HMAC(::EVP_sha512(), key.data(), key.size(), reinterpret_cast <const uint8_t *> (buffer.data()), buffer.size(), digest, nullptr) == nullptr)
							// Выполняем очистку блока с результатом
							result.clear();
						// Формируем данные SHA512-хэша
						else driver::hex(digest, 64, result);
					} break;
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выполняем очистку блока с результатом
				result.clear();
				// Записываем ошибку в лог в лог
				log->print("%s", log_t::flag_t::CRITICAL, error.what());
			}
		}
	}
	/**
	 * @brief Шаблон функции хэширования данных
	 *
	 * @tparam T сигнатура функции
	 *
	 */
	template <typename T>
	/**
	 * @brief Функция хэширования данных
	 *
	 * @param buffer буфер данных для шифрования
	 * @param size   размер данных для шифрования
	 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
	 * @param state  объект стейта шифрования
	 * @param result строка куда следует положить результат
	 * @param log    объект для работы с логами
	 *
	 * @return       признак успешно выполненной работы
	 */
	static bool hash(const char * buffer, const size_t size, const crypto_t::cipher_t cipher, const crypto_t::event_t event, state_t & state, T & result, const log_t * log) noexcept {
		/**
		 * Предел разрядности довода библиотеки криптографии отвергается явно:
		 * приведение размера к знаковому 32-битному числу молча обрезало буфер,
		 * и работа выдавала шифротекст части поданных данных за шифротекст всех
		 */
		// Если размер данных превышает предел разрядности библиотеки криптографии
		if(size > static_cast <size_t> (INT32_MAX)){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log->debug("Buffer size exceeds the limit of the cryptography library", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (cipher), static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log->print("Buffer size exceeds the limit of the cryptography library", log_t::flag_t::CRITICAL);
			#endif
			// Выходим из функции с признаком отказа
			return false;
		}
		/**
		 * Пустое сообщение работой принимается: у него есть шифротекст - вектор
		 * инициализации и имитовставка, - и разовая работа обязана выдать его так же,
		 * как выдаёт потоковая
		 */
		// Если буфер данных передан
		if((buffer != nullptr) || (size == 0)){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Выполняем очистку блока с результатом
				result.clear();
				/**
				 * Определяем тип шифрования
				 */
				switch(static_cast <uint16_t> (cipher)){
					// Если производится работы с BASE64
					case static_cast <uint16_t> (crypto_t::cipher_t::BASE64): {
						/**
						 * Признак работы выводится и здесь: прежде ветвь BASE64 доходила
						 * до общего успешного выхода при любом исходе, и негодный BASE64
						 * был неотличим от разбора в пустоту
						 */
						// Признак успешно выполненной работы
						bool success = false;
						// Инициализируем объект BASE64
						BIO * b64 = ::BIO_new(::BIO_f_base64());
						// Если объект BASE64 инициализирован
						if(b64 != nullptr){
							// Инициализируем объект BIO
							BIO * bio = ::BIO_new(::BIO_s_mem());
							// Если объект BIO инициализирован
							if(bio != nullptr){
								// Размер обработанных данных
								ssize_t length = 0;
								// Устанавливаем флаги
								::BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
								// Записываем параметры
								::BIO_push(b64, bio);
								/**
								 * Определяем событие кодирование или декодирование
								 */
								switch(static_cast <uint8_t> (event)){
									// Если производится кодирование данных
									case static_cast <uint8_t> (crypto_t::event_t::ENCODE): {
										// Выполняем кодирование в BASE64
										length = ::BIO_write(b64, buffer, size);
										// Выполняем очистку объекта
										BIO_flush(b64);
										// Если запись выполнена
										if(length > 0){
											// Выделяем память под запрошенный результат
											result.resize((4 * ((length + 2) / 3)) + 1, 0);
											// Выполняем чтение полученного результата
											length = ::BIO_read(bio, result.data(), result.size());
										}
									} break;
									// Если производится декодирование данных
									case static_cast <uint8_t> (crypto_t::event_t::DECODE): {
										// Выполняем декодирование из BASE64
										length = ::BIO_write(bio, buffer, size);
										// Выполняем очистку объекта
										BIO_flush(bio);
										// Если запись выполнена
										if(length > 0){
											// Выделяем память под запрошенный результат
											result.resize((3 * length / 4) + 1, 0);
											// Выполняем чтение полученного результата
											length = ::BIO_read(b64, result.data(), result.size());
										}
									} break;
								}
								// Если получение хэша произведено успешно
								if(length > 0){
									// Удаляем все лишние символы
									result.erase(result.begin() + length, result.end());
									// Запоминаем признак успешно выполненной работы
									success = true;
								/**
								 * Сообщение, октетов не имеющее, кодируется в пустоту:
								 * работы здесь нет вовсе, и отказом это звать нельзя
								 */
								// Если сообщение октетов не имеет
								} else if(size == 0)
									// Запоминаем признак успешно выполненной работы
									success = true;
								// Если получение хэша не удалось
								// Выполняем сброс результата, отказ записывается в лог единожды ниже
								else result.clear();
							}
							/**
							 * Очищаем всю цепочку BIO одним вызовом от головы:
							 * после BIO_push(b64, bio) объект b64 владеет ссылкой на bio и освобождает его в своём деструкторе.
							 * Раздельное освобождение bio и b64 приводит к двойному освобождению bio.
							 */
							::BIO_free_all(b64);
						}
						// Если работа не выполнена
						if(!success){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log->debug("Error during BASE64 processing", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (cipher), static_cast <uint16_t> (event)), log_t::flag_t::WARNING);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								log->print("Error during BASE64 processing", log_t::flag_t::WARNING);
							#endif
							// Выходим из функции с признаком отказа
							return false;
						}
					} break;
					// Если производится работы с AES128
					case static_cast <uint16_t> (crypto_t::cipher_t::AES128):
					// Если производится работы с AES192
					case static_cast <uint16_t> (crypto_t::cipher_t::AES192):
					// Если производится работы с AES256
					case static_cast <uint16_t> (crypto_t::cipher_t::AES256): {
						/**
						 * Устройство шифротекста
						 *
						 * @details Вектор инициализации выписывается в начало шифротекста,
						 *          а в режиме с проверкой подлинности имитовставка - в его
						 *          конец. Прежде вектор выводился из пароля вместе с ключом,
						 *          и повторное шифрование теми же паролем и солью давало ту
						 *          же гамму: два сообщения, сложенные по модулю два, выдавали
						 *          друг друга без всякого ключа
						 */
						// Размер вектора инициализации шифра
						const size_t ivsize = state.ivec.size();
						// Размер имитовставки режима с проверкой подлинности
						const size_t tagsize = ((state.mode == crypto_t::mode_t::GCM) ? AWH_CRYPTO_TAG_SIZE : 0);
						// Если контекст шифрования не создан
						if(state.ctx == nullptr){
							/**
							 * Если выполняется расшифровка, а данных не хватает даже на
							 * вектор инициализации с имитовставкой
							 */
							if((event == crypto_t::event_t::DECODE) && (size < (ivsize + tagsize))){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log->debug("Ciphertext is too short to carry the initialization vector", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (cipher), static_cast <uint16_t> (event)), log_t::flag_t::WARNING);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									log->print("Ciphertext is too short to carry the initialization vector", log_t::flag_t::WARNING);
								#endif
								// Выходим из функции с признаком отказа
								return false;
							}
							// Получаем буфер обрабатываемых данных
							const uint8_t * data = reinterpret_cast <const uint8_t *> (buffer);
							/**
							 * Если выполняется шифрование данных
							 */
							if(event == crypto_t::event_t::ENCODE){
								// Выполняем формирование случайного вектора инициализации
								if(::RAND_bytes(state.ivec.data(), static_cast <int32_t> (ivsize)) != 1){
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										log->debug("Error during initialization vector generation", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (cipher), static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										log->print("Error during initialization vector generation", log_t::flag_t::CRITICAL);
									#endif
									// Выходим из функции с признаком отказа
									return false;
								}
							/**
							 * Если выполняется расшифровка данных
							 */
							} else ::memcpy(state.ivec.data(), data, ivsize);
							// Создаем контекст шифрования
							EVP_CIPHER_CTX * ctx = ::EVP_CIPHER_CTX_new();
							// Если контекст не создан
							if(ctx == nullptr){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log->debug("Error during AES context creation", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (cipher), static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									log->print("Error during AES context creation", log_t::flag_t::CRITICAL);
								#endif
								// Выходим из функции с признаком отказа
								return false;
							}
							// Признак работы шифрования
							const int32_t direction = ((event == crypto_t::event_t::ENCODE) ? AES_ENCRYPT : AES_DECRYPT);
							// Выполняем инициализацию контекста шифрования
							if(::EVP_CipherInit_ex(ctx, state.evp, nullptr, state.key.data(), state.ivec.data(), direction) != 1){
								// Освобождаем контекст шифрования
								::EVP_CIPHER_CTX_free(ctx);
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log->debug("Error during AES context initialization", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (cipher), static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									log->print("Error during AES context initialization", log_t::flag_t::CRITICAL);
								#endif
								// Выходим из функции с признаком отказа
								return false;
							}
							// Отключаем padding (обязательно для потоковых режимов)
							::EVP_CIPHER_CTX_set_padding(ctx, 0);
							// Начало обрабатываемых данных
							const uint8_t * source = ((event == crypto_t::event_t::ENCODE) ? data : (data + ivsize));
							// Размер обрабатываемых данных
							const size_t count = ((event == crypto_t::event_t::ENCODE) ? size : (size - ivsize - tagsize));
							// Очищаем выходной буфер
							result.clear();
							/**
							 * Шифрование выписывает вектор в начало результата, расшифровка
							 * его лишь читает, поэтому размеры результата у них разные
							 */
							// Устанавливаем размер выходного буфера
							result.resize((((event == crypto_t::event_t::ENCODE) ? (ivsize + tagsize) : 0) + count + AES_BLOCK_SIZE), 0);
							/**
							 * Если выполняется шифрование данных
							 */
							if(event == crypto_t::event_t::ENCODE)
								// Выписываем вектор инициализации в начало результата
								::memcpy(result.data(), state.ivec.data(), ivsize);
							// Смещение записи результата
							const size_t shift = ((event == crypto_t::event_t::ENCODE) ? ivsize : 0);
							// Размер обработанных данных
							int32_t length = 0;
							// Выполняем обновление шифрования
							if(::EVP_CipherUpdate(ctx, reinterpret_cast <uint8_t *> (result.data()) + shift, &length, source, static_cast <int32_t> (count)) != 1){
								// Затираем и очищаем выходной буфер
								wipe(result);
								// Освобождаем контекст шифрования
								::EVP_CIPHER_CTX_free(ctx);
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log->debug("Error cipher update", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (cipher), static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									log->print("Error cipher update", log_t::flag_t::CRITICAL);
								#endif
								// Выходим из функции с признаком отказа
								return false;
							}
							/**
							 * При расшифровке в режиме с проверкой подлинности имитовставка
							 * передаётся до завершения работы: именно ею завершение и
							 * проверяет подлинность данных
							 */
							if((event == crypto_t::event_t::DECODE) && (tagsize > 0)){
								// Выполняем установку имитовставки из хвоста шифротекста
								if(::EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, static_cast <int32_t> (tagsize), const_cast <uint8_t *> (data + size - tagsize)) != 1){
									// Затираем и очищаем выходной буфер
									wipe(result);
									// Освобождаем контекст шифрования
									::EVP_CIPHER_CTX_free(ctx);
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										log->debug("Error during authentication tag setup", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (cipher), static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										log->print("Error during authentication tag setup", log_t::flag_t::CRITICAL);
									#endif
									// Выходим из функции с признаком отказа
									return false;
								}
							}
							// Размер добавленных завершением данных
							int32_t offset = 0;
							// Выполняем завершение работы шифрования
							if(::EVP_CipherFinal_ex(ctx, reinterpret_cast <uint8_t *> (result.data()) + shift + length, &offset) != 1){
								// Затираем и очищаем выходной буфер
								wipe(result);
								// Освобождаем контекст шифрования
								::EVP_CIPHER_CTX_free(ctx);
								/**
								 * Отказ завершения при расшифровке в режиме с проверкой
								 * подлинности означает подделку шифротекста, а не сбой
								 */
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log->debug((((event == crypto_t::event_t::DECODE) && (tagsize > 0)) ? "Authentication of the ciphertext failed" : "Error cipher final"), __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (cipher), static_cast <uint16_t> (event)), log_t::flag_t::WARNING);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									log->print((((event == crypto_t::event_t::DECODE) && (tagsize > 0)) ? "Authentication of the ciphertext failed" : "Error cipher final"), log_t::flag_t::WARNING);
								#endif
								// Выходим из функции с признаком отказа
								return false;
							}
							/**
							 * При шифровании в режиме с проверкой подлинности имитовставка
							 * снимается после завершения работы и дописывается в конец
							 */
							if((event == crypto_t::event_t::ENCODE) && (tagsize > 0)){
								// Выполняем снятие имитовставки шифротекста
								if(::EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, static_cast <int32_t> (tagsize), reinterpret_cast <uint8_t *> (result.data()) + shift + length + offset) != 1){
									// Затираем и очищаем выходной буфер
									wipe(result);
									// Освобождаем контекст шифрования
									::EVP_CIPHER_CTX_free(ctx);
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										log->debug("Error during authentication tag retrieval", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (cipher), static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										log->print("Error during authentication tag retrieval", log_t::flag_t::CRITICAL);
									#endif
									// Выходим из функции с признаком отказа
									return false;
								}
							}
							// Изменяем размер результата на фактический размер данных
							result.resize(shift + static_cast <size_t> (length + offset) + ((event == crypto_t::event_t::ENCODE) ? tagsize : 0));
							// Освобождаем контекст
							::EVP_CIPHER_CTX_free(ctx);
						// Если контекст шифрования создан
						} else {
							// Очищаем выходной буфер
							result.clear();
							/**
							 * Направление работы сверяется с тем, на которое заведён
							 * контекст: контекст, заведённый на шифрование, расшифровать
							 * не может, а довод направления в потоковом режиме прежде не
							 * читался вовсе - расшифровка поверх контекста шифрования
							 * молча шифровала ещё раз, и работа получала мусор
							 */
							// Если направление работы не совпадает с направлением контекста
							if(state.event != event){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log->debug("Direction of the stream cipher does not match the one it was initialized with", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (cipher), static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									log->print("Direction of the stream cipher does not match the one it was initialized with", log_t::flag_t::CRITICAL);
								#endif
								// Выходим из функции с признаком отказа
								return false;
							}
								// Получаем буфер обрабатываемых данных
								const uint8_t * data = reinterpret_cast <const uint8_t *> (buffer);
								// Размер обрабатываемых данных
								size_t count = size;
								// Размер выписываемого в начало потока вектора инициализации
								size_t prefix = 0;
								/**
								 * Если выполняется шифрование данных, а вектор инициализации
								 * в поток ещё не выписан
								 */
								if((event == crypto_t::event_t::ENCODE) && state.pending)
									// Запоминаем размер выписываемого вектора инициализации
									prefix = ivsize;
								/**
								 * Если выполняется расшифровка данных, а вектор инициализации
								 * из потока ещё не вычитан
								 */
								else if((event == crypto_t::event_t::DECODE) && state.pending) {
									// Определяем количество недостающих октетов вектора инициализации
									const size_t need = (ivsize - state.tail.size());
									// Определяем количество вычитываемых из потока октетов
									const size_t take = ((count < need) ? count : need);
									// Выполняем накопление вектора инициализации
									state.tail.insert(state.tail.end(), data, data + take);
									// Выполняем смещение в буфере обрабатываемых данных
									data += take;
									// Уменьшаем размер обрабатываемых данных
									count -= take;
									// Если вектор инициализации пришёл ещё не целиком
									if(state.tail.size() < ivsize)
										// Выходим из функции, выхода на этой порции нет
										return true;
									// Копируем накопленный вектор инициализации в стейт
									::memcpy(state.ivec.data(), state.tail.data(), ivsize);
									// Освобождаем накопитель вектора инициализации
									state.tail.clear();
									// Выполняем инициализацию контекста расшифровки
									if(::EVP_CipherInit_ex(const_cast <EVP_CIPHER_CTX *> (state.ctx), state.evp, nullptr, state.key.data(), state.ivec.data(), AES_DECRYPT) != 1){
										/**
										 * Стейт сбрасывается: вектор инициализации из потока
										 * уже вычитан, и оставь мы поток в прежнем состоянии,
										 * следующая порция была бы прочитана как вектор ещё раз
										 */
										// Выполняем сброс стейта AES-шифрования
										state.reset();
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Записываем ошибку в лог
											log->debug("Error during AES context initialization", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (cipher), static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL);
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Записываем ошибку в лог
											log->print("Error during AES context initialization", log_t::flag_t::CRITICAL);
										#endif
										// Выходим из функции с признаком отказа
										return false;
									}
									// Отключаем padding (обязательно для потоковых режимов)
									::EVP_CIPHER_CTX_set_padding(const_cast <EVP_CIPHER_CTX *> (state.ctx), 0);
									// Снимаем признак ожидания вектора инициализации
									state.pending = false;
								}
								/**
								 * При расшифровке в режиме с проверкой подлинности последние
								 * октеты потока удерживаются: имитовставка стоит в самом конце
								 * шифротекста, а какая порция окажется последней, до завершения
								 * работы неизвестно
								 */
								if((event == crypto_t::event_t::DECODE) && (tagsize > 0)){
									// Выполняем накопление удерживаемого хвоста потока
									state.tail.insert(state.tail.end(), data, data + count);
									// Определяем количество октетов, которые можно обработать
									count = ((state.tail.size() > tagsize) ? (state.tail.size() - tagsize) : 0);
									// Устанавливаем начало обрабатываемых данных
									data = state.tail.data();
								}
								// Буфер для выходных данных (на 1 блок больше — стандартная практика)
								result.resize(prefix + count + AES_BLOCK_SIZE, 0);
								/**
								 * Если вектор инициализации выписывается в начало потока
								 */
								if(prefix > 0){
									// Выписываем вектор инициализации в начало результата
									::memcpy(result.data(), state.ivec.data(), ivsize);
									// Снимаем признак ожидания вектора инициализации
									state.pending = false;
								}
								// Обрабатываем весь входной буфер за один вызов (рекомендуется)
								int32_t length = 0;
								// Выполняем обновление шифрования
								if((count > 0) && (::EVP_CipherUpdate(const_cast <EVP_CIPHER_CTX *> (state.ctx), reinterpret_cast <uint8_t *> (result.data()) + prefix, &length, data, static_cast <int32_t> (count)) != 1)){
									// Затираем и очищаем выходной буфер
									wipe(result);
									/**
									 * Стейт сбрасывается: часть потока контекстом уже поглощена,
									 * и продолжать работу поверх него нельзя - поток разошёлся бы
									 * с шифротекстом безвозвратно
									 */
									// Выполняем сброс стейта AES-шифрования
									state.reset();
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										log->debug("Error cipher update", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (cipher), static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										log->print("Error cipher update", log_t::flag_t::CRITICAL);
									#endif
									// Выходим из функции с признаком отказа
									return false;
								}
								/**
								 * Обработанные октеты снимаются с удерживаемого хвоста, в нём
								 * остаются лишь те, что могут оказаться имитовставкой
								 */
								if((event == crypto_t::event_t::DECODE) && (tagsize > 0) && (count > 0))
									// Выполняем снятие обработанных октетов с удерживаемого хвоста
									state.tail.erase(state.tail.begin(), state.tail.begin() + count);
								// Изменяем размер результата на фактический размер данных
								result.resize(prefix + static_cast <size_t> (length));
						}
					} break;
					// Если тип шифрования не установлен либо разбору не знаком
					default: {
						/**
						 * Тип шифрования, разбору не знакомый, отвергается явно: прежде
						 * отбор шёл по младшему октету значения, а разрядность AES256 в него
						 * не умещается - его метка совпадала с меткой незаданного шифрования,
						 * и работа без шифрования уходила в ветвь AES
						 */
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							log->debug("Cipher type is not set", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (cipher), static_cast <uint16_t> (event)), log_t::flag_t::WARNING);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log->print("Cipher type is not set", log_t::flag_t::WARNING);
						#endif
						// Выходим из функции с признаком отказа
						return false;
					}
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выполняем очистку блока с результатом
				result.clear();
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (cipher), static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
				// Выходим из функции с признаком отказа
				return false;
			}
			// Выводим признак успешно выполненной работы
			return true;
		}
		// Выводим признак отказа
		return false;
	}
	/**
	 * @brief Функция установки схемы дополнения подписи RSA
	 *
	 * @details Схема применяется к ключам RSA и только к ним: у ключей иного
	 *          устройства дополнения нет вовсе, и установка его на них отменила
	 *          бы работу без всякого довода.
	 *
	 *          Длина соли вероятностной схемы берётся равной длине хэш-суммы -
	 *          так её выбирает и проверяющая сторона, разбирающая подпись без
	 *          уговора о длине (RFC 8017 9.1)
	 *
	 * @param pctx    контекст ключа подписи
	 * @param key     ключ подписи
	 * @param md      функция хэширования подписи
	 * @param padding схема дополнения подписи RSA
	 * @param log     объект для работы с логами
	 * @return        результат установки схемы дополнения
	 *
	 */
	static bool padding(EVP_PKEY_CTX * pctx, EVP_PKEY * key, const EVP_MD * md, const crypto_t::padding_t padding, const log_t * log) noexcept {
		// Если ключ подписи устройства RSA не имеет
		if(::EVP_PKEY_base_id(key) != EVP_PKEY_RSA)
			// Выводим успех: дополнения у такого ключа нет вовсе
			return true;
		/**
		 * Если схема дополнения подписи не задана
		 */
		if(padding == crypto_t::padding_t::NONE){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log->debug("Signature padding scheme is not set", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log->print("Signature padding scheme is not set", log_t::flag_t::CRITICAL);
			#endif
			// Выводим отказ установки схемы дополнения
			return false;
		}
		// Выполняем установку схемы дополнения подписи
		if(::EVP_PKEY_CTX_set_rsa_padding(pctx, ((padding == crypto_t::padding_t::PSS) ? RSA_PKCS1_PSS_PADDING : RSA_PKCS1_PADDING)) <= 0){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log->debug("Error during signature padding setup", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log->print("Error during signature padding setup", log_t::flag_t::CRITICAL);
			#endif
			// Выводим отказ установки схемы дополнения
			return false;
		}
		/**
		 * Если схема дополнения подписи вероятностная
		 */
		if(padding == crypto_t::padding_t::PSS){
			// Выполняем установку функции хэширования маски
			if(::EVP_PKEY_CTX_set_rsa_mgf1_md(pctx, md) <= 0)
				// Выводим отказ установки схемы дополнения
				return false;
			// Выполняем установку длины соли подписи
			if(::EVP_PKEY_CTX_set_rsa_pss_saltlen(pctx, RSA_PSS_SALTLEN_DIGEST) <= 0)
				// Выводим отказ установки схемы дополнения
				return false;
		}
		// Выводим успех установки схемы дополнения
		return true;
	}
	/**
	 * @brief Функция инициализации контекста шифрования
	 *
	 * @param cipher тип шифрования (AES128, AES192, AES256)
	 * @param hash   тип хэш-суммы
	 * @param pass   пароль для шифрования
	 * @param salt   соль для шифрования
	 * @param rounds количество итераций PBKDF2
	 * @param state  стейт шифрования AES
	 * @param log    объект для работы с логами
	 * @return       результат инициализации
	 *
	 */
	static bool cipher(const crypto_t::cipher_t cipher, const crypto_t::mode_t mode, const crypto_t::hash_t hash, const string & pass, const string & salt, const uint32_t rounds, state_t & state, const log_t * log) noexcept {
		// Переменная результата
		bool result = false;
		// Если пароль для шифрования не пустой
		if(!pass.empty()){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				/**
				 * Прежние ключ и вектор затираются перед выводом новых: освобождение
				 * набора содержимое не гасит, и прежний ключ оставался лежать в памяти
				 */
				// Если прежний ключ шифрования выведен
				if(!state.key.empty())
					// Выполняем затирание прежнего ключа шифрования
					::OPENSSL_cleanse(state.key.data(), state.key.size());
				// Если прежний вектор инициализации заведён
				if(!state.ivec.empty())
					// Выполняем затирание прежнего вектора инициализации
					::OPENSSL_cleanse(state.ivec.data(), state.ivec.size());
				// Обнуляем массив ключа
				state.key.clear();
				// Обнуляем массив IVEC
				state.ivec.clear();
				// Тип шифрования AES
				const EVP_CIPHER * evp = ::EVP_enc_null();
				/**
				 * Определяем длину шифрования
				 */
				switch(static_cast <uint16_t> (cipher)){
					// Устанавливаем шифрование в 128
					case static_cast <uint16_t> (crypto_t::cipher_t::AES128): {
						// Устанавливаем размер массива KEY
						state.key.resize(16, 0);
						// Устанавливаем функцию шифрования
						evp = ((mode == crypto_t::mode_t::GCM) ? ::EVP_aes_128_gcm() : ::EVP_aes_128_cfb128());
					} break;
					// Устанавливаем шифрование в 192
					case static_cast <uint16_t> (crypto_t::cipher_t::AES192): {
						// Устанавливаем размер массива KEY
						state.key.resize(24, 0);
						// Устанавливаем функцию шифрования
						evp = ((mode == crypto_t::mode_t::GCM) ? ::EVP_aes_192_gcm() : ::EVP_aes_192_cfb128());
					} break;
					// Устанавливаем шифрование в 256
					case static_cast <uint16_t> (crypto_t::cipher_t::AES256): {
						// Устанавливаем размер массива KEY
						state.key.resize(32, 0);
						// Устанавливаем функцию шифрования
						evp = ((mode == crypto_t::mode_t::GCM) ? ::EVP_aes_256_gcm() : ::EVP_aes_256_cfb128());
					} break;
					// Если ничего не выбрано
					default: {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							log->debug("Unsupported cipher type", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (cipher), static_cast <uint16_t> (hash), pass, salt, rounds), log_t::flag_t::CRITICAL);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log->print("Unsupported cipher type", log_t::flag_t::CRITICAL);
						#endif
						// Возвращаем результат работы функции
						return result;
					}
				}
				/**
				 * Режим блочного шифрования отвергается, если он не задан: прежде
				 * режим был один и в доводах не значился вовсе
				 */
				// Если режим блочного шифрования не задан
				if(mode == crypto_t::mode_t::NONE){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						log->debug("Block cipher mode is not set", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (cipher), static_cast <uint16_t> (hash), pass, salt, rounds), log_t::flag_t::CRITICAL);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						log->print("Block cipher mode is not set", log_t::flag_t::CRITICAL);
					#endif
					// Возвращаем результат работы функции
					return result;
				}
				/**
				 * Размер вектора инициализации берётся у самого шифра: у гаммирования
				 * он равен блоку, у счётчика Галуа - двенадцати октетам
				 */
				// Устанавливаем размер массива IVEC
				state.ivec.resize(static_cast <size_t> (::EVP_CIPHER_iv_length(evp)), 0);
				// Выбираем хэш-функцию
				const EVP_MD * md = nullptr;
				/**
				 * Определяем тип хэш-суммы
				 */
				switch(static_cast <uint8_t> (hash)){
					// Если тип хэш-суммы указан как MD5
					case static_cast <uint8_t> (crypto_t::hash_t::MD5):
						// Устанавливаем функцию хэширования
						md = ::EVP_md5();
					break;
					// Если тип хэш-суммы указан как SHA1
					case static_cast <uint8_t> (crypto_t::hash_t::SHA1):
						// Устанавливаем функцию хэширования
						md = ::EVP_sha1();
					break;
					// Если тип хэш-суммы указан как SHA224
					case static_cast <uint8_t> (crypto_t::hash_t::SHA224):
						// Устанавливаем функцию хэширования
						md = ::EVP_sha224();
					break;
					// Если тип хэш-суммы указан как SHA256
					case static_cast <uint8_t> (crypto_t::hash_t::SHA256):
						// Устанавливаем функцию хэширования
						md = ::EVP_sha256();
					break;
					// Если тип хэш-суммы указан как SHA384
					case static_cast <uint8_t> (crypto_t::hash_t::SHA384):
						// Устанавливаем функцию хэширования
						md = ::EVP_sha384();
					break;
					// Если тип хэш-суммы указан как SHA512
					case static_cast <uint8_t> (crypto_t::hash_t::SHA512):
						// Устанавливаем функцию хэширования
						md = ::EVP_sha512();
					break;
					// Если ничего не выбрано
					default: {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							log->debug("Unsupported hash type", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (cipher), static_cast <uint16_t> (hash), pass, salt, rounds), log_t::flag_t::CRITICAL);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log->print("Unsupported hash type", log_t::flag_t::CRITICAL);
						#endif
						// Возвращаем результат работы функции
						return result;
					}
				}
				/**
				 * Из пароля выводится один лишь ключ: вектор инициализации выводился
				 * оттуда же, отчего повторное шифрование теми же паролем и солью
				 * давало ту же гамму - два сообщения, сложенные по модулю два, выдавали
				 * друг друга без всякого ключа. Вектор теперь берётся случайным на
				 * каждое сообщение и выписывается в начало шифротекста
				 */
				/**
				 * Соль работой не требуется, но без неё вывод ключа теряет всякую
				 * стойкость к предвычисленным таблицам: один и тот же пароль даёт
				 * один и тот же ключ у всех, кто им пользуется
				 */
				// Если соль вывода ключа не установлена
				if(salt.empty()){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем предупреждение в лог
						log->debug("Key derivation without a salt is vulnerable to precomputed tables", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (cipher), static_cast <uint16_t> (hash), pass, salt, rounds), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем предупреждение в лог
						log->print("Key derivation without a salt is vulnerable to precomputed tables", log_t::flag_t::WARNING);
					#endif
				}
				// Генерация ключа шифрования через PBKDF2
				result = (::PKCS5_PBKDF2_HMAC(
					pass.c_str(),
					static_cast <int32_t> (pass.length()),
					(salt.empty() ? nullptr : reinterpret_cast <const uint8_t *> (salt.c_str())),
					static_cast <int32_t> (salt.length()),
					static_cast <int32_t> (rounds), md,
					static_cast <int32_t> (state.key.size()), state.key.data()
				) == 1);
				// Если инициализация не произошла
				if(!result){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						log->debug("Generate key and IV failed", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (cipher), static_cast <uint16_t> (hash), pass, salt, rounds), log_t::flag_t::CRITICAL);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						log->print("Generate key and IV failed", log_t::flag_t::CRITICAL);
					#endif
				// Если инициализация произошла успешно
				} else {
					// Устанавливаем контекст шифрования в стейт
					state.evp = evp;
					// Устанавливаем тип хэш-суммы в стейт
					state.hash = hash;
					// Устанавливаем тип шифрования в стейт
					state.cipher = cipher;
					// Устанавливаем режим блочного шифрования в стейт
					state.mode = mode;
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (cipher), static_cast <uint16_t> (hash), pass, salt, rounds), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			}
		}
		// Возвращаем результат работы функции
		return result;
	}
};

/**
 * @brief Конструктор
 *
 */
awh::Crypto::Params_RSA::Params_RSA() noexcept :
 rounds(AWH_CRYPTO_AES_ROUNDS),
 mode(crypto_t::mode_t::GCM),
 padding(crypto_t::padding_t::PSS),
 salt{""}, password{""}, passwordRSA{""},
 state(nullptr), key(nullptr) {}

/**
 * @brief Метод установки количества итераций PBKDF2 для вывода ключа AES
 *
 * @param round количество итераций PBKDF2
 *
 */
void awh::Crypto::roundAES(const uint32_t round) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Нулевое количество итераций отвергается здесь, а не в глубине вывода
		 * ключа: оттуда пришёл бы отказ OpenSSL без указания на настоящую причину
		 */
		// Если количество итераций не задано
		if(round == 0){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Number of PBKDF2 iterations must be at least one", __PRETTY_FUNCTION__, make_tuple(round), log_t::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Number of PBKDF2 iterations must be at least one", log_t::flag_t::CRITICAL);
			#endif
			// Выходим из функции
			return;
		}
		// Устанавливаем количество раундов шифрования
		this->_params.rounds = round;
		// Сбрасываем стейт шифрования
		this->_params.state->reset();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (round)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки соли шифрования
 *
 * @param salt соль для шифрования
 *
 */
void awh::Crypto::salt(string_view salt) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Прежняя соль затирается наравне с паролями: строка стандартной библиотеки
		 * при записи поверх содержимое не гасит, и прежняя соль оставалась лежать
		 * в памяти до тех пор, покуда её не выдадут кому-то ещё
		 */
		// Если соль вывода ключа уже установлена
		if(!this->_params.salt.empty())
			// Выполняем затирание прежней соли вывода ключа
			::OPENSSL_cleanse(&this->_params.salt.front(), this->_params.salt.size());
		// Устанавливаем соль для шифрования
		this->_params.salt = salt;
		// Сбрасываем стейт шифрования
		this->_params.state->reset();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(salt), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки пароля шифрования
 *
 * @param password пароль шифрования
 *
 */
void awh::Crypto::password(string_view password) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Прежний пароль затирается: строка стандартной библиотеки при записи
		 * поверх содержимое не гасит, и прежний пароль оставался лежать в памяти
		 * до тех пор, покуда её не выдадут кому-то ещё
		 */
		// Если прежний пароль шифрования установлен
		if(!this->_params.password.empty())
			// Выполняем затирание прежнего пароля шифрования
			::OPENSSL_cleanse(&this->_params.password.front(), this->_params.password.size());
		// Устанавливаем пароль шифрования
		this->_params.password = password;
		// Сбрасываем стейт шифрования
		this->_params.state->reset();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(password), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки режима блочного шифрования
 *
 * @param mode режим блочного шифрования
 *
 */
void awh::Crypto::mode(const mode_t mode) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Устанавливаем режим блочного шифрования
		this->_params.mode = mode;
		// Сбрасываем стейт шифрования
		this->_params.state->reset();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод извлечения режима блочного шифрования
 *
 * @return режим блочного шифрования
 *
 */
awh::Crypto::mode_t awh::Crypto::mode() const noexcept {
	// Выводим режим блочного шифрования
	return this->_params.mode;
}
/**
 * @brief Метод установки схемы дополнения подписи RSA
 *
 * @param padding схема дополнения подписи RSA
 *
 */
void awh::Crypto::padding(const padding_t padding) noexcept {
	// Устанавливаем схему дополнения подписи RSA
	this->_params.padding = padding;
}
/**
 * @brief Метод извлечения схемы дополнения подписи RSA
 *
 * @return схема дополнения подписи RSA
 *
 */
awh::Crypto::padding_t awh::Crypto::padding() const noexcept {
	// Выводим схему дополнения подписи RSA
	return this->_params.padding;
}
/**
 * @brief Метод установки пароля защиты приватного ключа RSA
 *
 * @param password пароль защиты приватного ключа RSA
 *
 */
void awh::Crypto::passwordRSA(string_view password) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Прежний пароль затирается: строка стандартной библиотеки при записи
		 * поверх содержимое не гасит, и прежний пароль оставался лежать в памяти
		 * до тех пор, покуда её не выдадут кому-то ещё
		 */
		// Если прежний пароль защиты приватного ключа установлен
		if(!this->_params.passwordRSA.empty())
			// Выполняем затирание прежнего пароля защиты приватного ключа
			::OPENSSL_cleanse(&this->_params.passwordRSA.front(), this->_params.passwordRSA.size());
		// Устанавливаем пароль защиты приватного ключа RSA
		this->_params.passwordRSA = password;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод преобразования 128-битного хэша в 64-битный
 *
 * @param hash 128-битный хэш
 * @return     64-битный хэш
 *
 */
uint64_t awh::Crypto::hash128to64(const uint128_t & hash) const noexcept {
	// Первая половина 128-битного хэша
	uint64_t value1 = 0;
	// Вторая половина 128-битного хэша
	uint64_t value2 = 0;
	// Копируем первую половину 128-битного хэша
	::memcpy(&value1, hash.data(), sizeof(value1));
	// Копируем вторую половину 128-битного хэша
	::memcpy(&value2, hash.data() + sizeof(value1), sizeof(value2));
	// Выводим свёртку половин 128-битного хэша
	return hashing::avalanche(hashing::mix(value1, value2));
}
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
auto awh::Crypto::hash(string_view buffer, const hash_t hash) const noexcept -> T {
	// Переменная результата
	T result;
	// Если текст передан
	if(!buffer.empty()){
		// Выполняем хэширование
		driver::hash(buffer, hash, result, this->_log);
		// Если хэширование не вышло
		if(result.empty()){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug(
					"Text hashing \"%s\" could not be performed",
					__PRETTY_FUNCTION__, make_tuple(
						string(reinterpret_cast <const char *> (buffer.data()), buffer.size()),
						static_cast <uint16_t> (hash)
					), log_t::flag_t::WARNING,
					string(reinterpret_cast <const char *> (buffer.data()), buffer.size()).c_str()
				);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print(
					"Text hashing \"%s\" could not be performed",
					log_t::flag_t::WARNING,
					string(reinterpret_cast <const char *> (buffer.data()), buffer.size()).c_str()
				);
			#endif
		}
	}
	// Возвращаем результат работы функции
	return result;
}
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в строку
 *
 */
template string awh::Crypto::hash <string> (string_view, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в буфер
 *
 */
template vector <char> awh::Crypto::hash <vector <char>> (string_view, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Crypto::hash <vector <uint8_t>> (string_view, const hash_t) const noexcept;
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
auto awh::Crypto::hash(const B & buffer, const hash_t hash) const noexcept -> A {
	// Переменная результата
	A result;
	// Если текст передан
	if(!buffer.empty()){
		// Выполняем хэширование
		driver::hash(buffer, hash, result, this->_log);
		// Если хэширование не вышло
		if(result.empty()){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug(
					"Text hashing \"%s\" could not be performed",
					__PRETTY_FUNCTION__, make_tuple(
						string(reinterpret_cast <const char *> (buffer.data()), buffer.size()),
						static_cast <uint16_t> (hash)
					), log_t::flag_t::WARNING,
					string(reinterpret_cast <const char *> (buffer.data()), buffer.size()).c_str()
				);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print(
					"Text hashing \"%s\" could not be performed",
					log_t::flag_t::WARNING,
					string(reinterpret_cast <const char *> (buffer.data()), buffer.size()).c_str()
				);
			#endif
		}
	}
	// Возвращаем результат работы функции
	return result;
}
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в строку
 *
 */
template string awh::Crypto::hash <string> (const string &, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования буфера данных с выводом результата в строку
 *
 */
template string awh::Crypto::hash <string> (const vector <char> &, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования бинарного буфера данных с выводом результата в строку
 *
 */
template string awh::Crypto::hash <string> (const vector <uint8_t> &, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в буфер
 *
 */
template vector <char> awh::Crypto::hash <vector <char>> (const string &, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования буфера данных с выводом результата в буфер
 *
 */
template vector <char> awh::Crypto::hash <vector <char>> (const vector <char> &, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования бинарного буфера данных с выводом результата в буфер
 *
 */
template vector <char> awh::Crypto::hash <vector <char>> (const vector <uint8_t> &, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Crypto::hash <vector <uint8_t>> (const string &, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования буфера данных с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Crypto::hash <vector <uint8_t>> (const vector <char> &, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования бинарного буфера данных с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Crypto::hash <vector <uint8_t>> (const vector <uint8_t> &, const hash_t) const noexcept;
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
auto awh::Crypto::hmac(string_view key, string_view buffer, const hash_t hash) const noexcept -> T {
	// Переменная результата
	T result;
	// Если текст передан
	if(!buffer.empty()){
		// Выполняем хэширование
		driver::hmac(key, buffer, hash, result, this->_log);
		// Если хэширование не вышло
		if(result.empty()){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug(
					"Key \"%s\" and text \"%s\" hashing could not be performed",
					__PRETTY_FUNCTION__, make_tuple(
						key,
						string(reinterpret_cast <const char *> (buffer.data()), buffer.size()),
						static_cast <uint16_t> (hash)
					), log_t::flag_t::WARNING,
					string(key).c_str(),
					string(reinterpret_cast <const char *> (buffer.data()), buffer.size()).c_str()
				);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print(
					"Key \"%s\" and text \"%s\" hashing could not be performed",
					log_t::flag_t::WARNING,
					string(key).c_str(),
					string(reinterpret_cast <const char *> (buffer.data()), buffer.size()).c_str()
				);
			#endif
		}
	}
	// Возвращаем результат работы функции
	return result;
}
/**
 * @brief Явный специализированный шаблон метода хэширования текста с ключом и выводом результата в строку
 *
 */
template string awh::Crypto::hmac(string_view, string_view, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с ключом и выводом результата в буфер
 *
 */
template vector <char> awh::Crypto::hmac(string_view, string_view, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с ключом и выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Crypto::hmac(string_view, string_view, const hash_t) const noexcept;
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
auto awh::Crypto::hmac(const string & key, string_view buffer, const hash_t hash) const noexcept -> T {
	// Переменная результата
	T result;
	// Если текст передан
	if(!buffer.empty()){
		// Выполняем хэширование
		driver::hmac(key, buffer, hash, result, this->_log);
		// Если хэширование не вышло
		if(result.empty()){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug(
					"Key \"%s\" and text \"%s\" hashing could not be performed",
					__PRETTY_FUNCTION__, make_tuple(
						key,
						string(reinterpret_cast <const char *> (buffer.data()), buffer.size()),
						static_cast <uint16_t> (hash)
					), log_t::flag_t::WARNING,
					key.c_str(),
					string(reinterpret_cast <const char *> (buffer.data()), buffer.size()).c_str()
				);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print(
					"Key \"%s\" and text \"%s\" hashing could not be performed",
					log_t::flag_t::WARNING,
					key.c_str(),
					string(reinterpret_cast <const char *> (buffer.data()), buffer.size()).c_str()
				);
			#endif
		}
	}
	// Возвращаем результат работы функции
	return result;
}
/**
 * @brief Явный специализированный шаблон метода хэширования текста с ключом и выводом результата в строку
 *
 */
template string awh::Crypto::hmac(const string &, string_view, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с ключом и выводом результата в буфер
 *
 */
template vector <char> awh::Crypto::hmac(const string &, string_view, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с ключом и выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Crypto::hmac(const string &, string_view, const hash_t) const noexcept;
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
auto awh::Crypto::hmac(string_view key, const B & buffer, const hash_t hash) const noexcept -> A {
	// Переменная результата
	A result;
	// Если текст передан
	if(!buffer.empty()){
		// Выполняем хэширование
		driver::hmac(key, buffer, hash, result, this->_log);
		// Если хэширование не вышло
		if(result.empty()){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug(
					"Key \"%s\" and text \"%s\" hashing could not be performed",
					__PRETTY_FUNCTION__, make_tuple(
						key,
						string(reinterpret_cast <const char *> (buffer.data()), buffer.size()),
						static_cast <uint16_t> (hash)
					), log_t::flag_t::WARNING,
					string(key).c_str(),
					string(reinterpret_cast <const char *> (buffer.data()), buffer.size()).c_str()
				);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print(
					"Key \"%s\" and text \"%s\" hashing could not be performed",
					log_t::flag_t::WARNING,
					string(key).c_str(),
					string(reinterpret_cast <const char *> (buffer.data()), buffer.size()).c_str()
				);
			#endif
		}
	}
	// Возвращаем результат работы функции
	return result;
}
/**
 * @brief Явный специализированный шаблон метода хэширования текста с ключом и выводом результата в строку
 *
 */
template string awh::Crypto::hmac(string_view, const string &, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования буфера данных с ключом и выводом результата в строку
 *
 */
template string awh::Crypto::hmac(string_view, const vector <char> &, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования бинарного буфера данных с ключом и выводом результата в строку
 *
 */
template string awh::Crypto::hmac(string_view, const vector <uint8_t> &, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с ключом и выводом результата в буфер
 *
 */
template vector <char> awh::Crypto::hmac(string_view, const string &, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования буфера данных с ключом и выводом результата в буфер
 *
 */
template vector <char> awh::Crypto::hmac(string_view, const vector <char> &, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования бинарного буфера данных с ключом и выводом результата в буфер
 *
 */
template vector <char> awh::Crypto::hmac(string_view, const vector <uint8_t> &, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с ключом и выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Crypto::hmac(string_view, const string &, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования буфера данных с ключом и выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Crypto::hmac(string_view, const vector <char> &, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования бинарного буфера данных с ключом и выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Crypto::hmac(string_view, const vector <uint8_t> &, const hash_t) const noexcept;
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
auto awh::Crypto::hmac(const string & key, const B & buffer, const hash_t hash) const noexcept -> A {
	// Переменная результата
	A result;
	// Если текст передан
	if(!buffer.empty()){
		// Выполняем хэширование
		driver::hmac(key, buffer, hash, result, this->_log);
		// Если хэширование не вышло
		if(result.empty()){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug(
					"Key \"%s\" and text \"%s\" hashing could not be performed",
					__PRETTY_FUNCTION__, make_tuple(
						key,
						string(reinterpret_cast <const char *> (buffer.data()), buffer.size()),
						static_cast <uint16_t> (hash)
					), log_t::flag_t::WARNING,
					key.c_str(),
					string(reinterpret_cast <const char *> (buffer.data()), buffer.size()).c_str()
				);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print(
					"Key \"%s\" and text \"%s\" hashing could not be performed",
					log_t::flag_t::WARNING,
					key.c_str(),
					string(reinterpret_cast <const char *> (buffer.data()), buffer.size()).c_str()
				);
			#endif
		}
	}
	// Возвращаем результат работы функции
	return result;
}
/**
 * @brief Явный специализированный шаблон метода хэширования текста с ключом и выводом результата в строку
 *
 */
template string awh::Crypto::hmac(const string &, const string &, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования буфера данных с ключом и выводом результата в строку
 *
 */
template string awh::Crypto::hmac(const string &, const vector <char> &, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования бинарного буфера данных с ключом и выводом результата в строку
 *
 */
template string awh::Crypto::hmac(const string &, const vector <uint8_t> &, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с ключом и выводом результата в буфер
 *
 */
template vector <char> awh::Crypto::hmac(const string &, const string &, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования буфера данных с ключом и выводом результата в буфер
 *
 */
template vector <char> awh::Crypto::hmac(const string &, const vector <char> &, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования бинарного буфера данных с ключом и выводом результата в буфер
 *
 */
template vector <char> awh::Crypto::hmac(const string &, const vector <uint8_t> &, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с ключом и выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Crypto::hmac(const string &, const string &, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования буфера данных с ключом и выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Crypto::hmac(const string &, const vector <char> &, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования бинарного буфера данных с ключом и выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Crypto::hmac(const string &, const vector <uint8_t> &, const hash_t) const noexcept;
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
bool awh::Crypto::finalize(T & buffer) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Получаем состояние объекта
		state_t & state = (* this->_params.state);
		// Если контекст шифрования уже создан
		if((result = (state.ctx != nullptr))){
			// Размер имитовставки режима с проверкой подлинности
			const size_t tagsize = ((state.mode == mode_t::GCM) ? AWH_CRYPTO_TAG_SIZE : 0);
			/**
			 * Расшифровка, не получившая вектора инициализации целиком, отвергается
			 * здесь: контекст расшифровки заводится лишь по вычитывании вектора из
			 * начала потока, и завершение шло по контексту, ключом не наделённому
			 */
			// Если вектор инициализации из потока вычитан не целиком
			if((state.event == event_t::DECODE) && state.pending){
				// Затираем и очищаем выходной буфер
				driver::wipe(buffer);
				// Выполняем сброс стейта AES-шифрования
				state.reset();
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("Ciphertext does not carry the full initialization vector", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Ciphertext does not carry the full initialization vector", log_t::flag_t::WARNING);
				#endif
				// Выходим из метода
				return (result = false);
			}
			/**
			 * При расшифровке в режиме с проверкой подлинности имитовставка
			 * передаётся до завершения работы: именно ею завершение и проверяет
			 * подлинность данных. Лежит она в удерживаемом хвосте потока
			 */
			if((state.event == event_t::DECODE) && (tagsize > 0)){
				// Если удерживаемый хвост потока имитовставки не несёт
				if(state.tail.size() != tagsize){
					// Затираем и очищаем выходной буфер
					driver::wipe(buffer);
					// Выполняем сброс стейта AES-шифрования
					state.reset();
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Ciphertext does not carry the authentication tag", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Ciphertext does not carry the authentication tag", log_t::flag_t::WARNING);
					#endif
					// Выходим из метода
					return (result = false);
				}
				// Выполняем установку имитовставки из удерживаемого хвоста потока
				if(::EVP_CIPHER_CTX_ctrl(const_cast <EVP_CIPHER_CTX *> (state.ctx), EVP_CTRL_AEAD_SET_TAG, static_cast <int32_t> (tagsize), state.tail.data()) != 1){
					// Затираем и очищаем выходной буфер
					driver::wipe(buffer);
					// Выполняем сброс стейта AES-шифрования
					state.reset();
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Error during authentication tag setup", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Error during authentication tag setup", log_t::flag_t::CRITICAL);
					#endif
					// Выходим из метода
					return (result = false);
				}
			}
			/**
			 * Признак проверки подлинности снимается до сброса стейта: сброс
			 * обнуляет направление работы, и сообщение об отказе читало уже
			 * сброшенное значение - подделка объявлялась сбоем завершения
			 */
			// Признак того, что отказ завершения означает подделку шифротекста
			const bool tampered = ((state.event == event_t::DECODE) && (tagsize > 0));
			// Размер добавленных завершением данных
			int32_t offset = 0;
			/**
			 * Вектор инициализации выписывается здесь, если ни одной порции данных
			 * не подавалось: выписывается он первой же порцией выхода, и поток без
			 * единой порции оставался бы без вектора - расшифровать его было бы
			 * нечем, хотя имитовставка в него попадала
			 */
			// Если вектор инициализации в поток ещё не выписан
			if((state.event == event_t::ENCODE) && state.pending){
				// Выписываем вектор инициализации в начало результата
				buffer.insert(buffer.begin(), state.ivec.begin(), state.ivec.end());
				// Снимаем признак ожидания вектора инициализации
				state.pending = false;
			}
			// Получаем текущий размер результата
			const size_t length = buffer.size();
			// Расширяем буфер под результат
			buffer.resize(length + AES_BLOCK_SIZE + tagsize, 0);
			// Выполняем завершение работы шифрования
			if(!(result = (::EVP_CipherFinal_ex(const_cast <EVP_CIPHER_CTX *> (state.ctx), reinterpret_cast <uint8_t *> (buffer.data() + length), &offset) == 1))){
				// Затираем и очищаем выходной буфер
				driver::wipe(buffer);
				// Выполняем сброс стейта AES-шифрования
				state.reset();
				/**
				 * Отказ завершения при расшифровке в режиме с проверкой подлинности
				 * означает подделку шифротекста, а не сбой
				 */
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug((tampered ? "Authentication of the ciphertext failed" : "Error cipher final"), __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print((tampered ? "Authentication of the ciphertext failed" : "Error cipher final"), log_t::flag_t::WARNING);
				#endif
				// Выходим из метода
				return result;
			}
			/**
			 * При шифровании в режиме с проверкой подлинности имитовставка
			 * снимается после завершения работы и дописывается в конец потока
			 */
			if((state.event == event_t::ENCODE) && (tagsize > 0)){
				// Выполняем снятие имитовставки шифротекста
				if(!(result = (::EVP_CIPHER_CTX_ctrl(const_cast <EVP_CIPHER_CTX *> (state.ctx), EVP_CTRL_AEAD_GET_TAG, static_cast <int32_t> (tagsize), reinterpret_cast <uint8_t *> (buffer.data() + length + offset)) == 1))){
					// Затираем и очищаем выходной буфер
					driver::wipe(buffer);
					// Выполняем сброс стейта AES-шифрования
					state.reset();
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Error during authentication tag retrieval", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Error during authentication tag retrieval", log_t::flag_t::CRITICAL);
					#endif
					// Выходим из метода
					return result;
				}
				// Изменяем размер результата с учётом имитовставки
				buffer.resize(length + static_cast <size_t> (offset) + tagsize);
			// Изменяем размер результата на фактический размер данных
			} else buffer.resize(length + static_cast <size_t> (offset));
			// Выполняем сброс стейта AES-шифрования
			state.reset();
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат работы функции
	return result;
}
/**
 * @brief Явный специализированный шаблон метода финализации контекста шифрования
 *
 */
template bool awh::Crypto::finalize(string &) noexcept;
/**
 * @brief Явный специализированный шаблон метода финализации контекста шифрования
 *
 */
template bool awh::Crypto::finalize(vector <char> &) noexcept;
/**
 * @brief Явный специализированный шаблон метода финализации контекста шифрования
 *
 */
template bool awh::Crypto::finalize(vector <uint8_t> &) noexcept;
/**
 * @brief Метод инициализации контекста шифрования
 *
 * @param event  событие шифрования (ENCODE, DECODE)
 * @param hash   тип хэш-суммы
 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
 * @return       результат инициализации
 *
 */
bool awh::Crypto::initialize(const event_t event, const hash_t hash, const cipher_t cipher) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если пароль установлен
		if(!this->_params.password.empty()){
			// Получаем состояние объекта
			state_t & state = (* this->_params.state);
			{
				/**
				 * Заведённый прежде контекст шифрования сбрасывается, а не отменяет
				 * работу: повторная инициализация иначе всегда отвечала отказом, и
				 * сменить направление либо шифр после первого раза было нечем
				 */
				// Выполняем сброс стейта AES-шифрования
				state.reset();
				// Если инициализация ключей не выполнена
				if(!driver::cipher(cipher, this->_params.mode, hash, this->_params.password, this->_params.salt, this->_params.rounds, state, this->_log)){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Unable to initialize AES cipher for encoding data", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (hash), static_cast <uint16_t> (cipher)), log_t::flag_t::CRITICAL);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Unable to initialize AES cipher for encoding data", log_t::flag_t::CRITICAL);
					#endif
					// Выходим из метода
					return result;
				}
				// Создаем контекст шифрования
				state.ctx = ::EVP_CIPHER_CTX_new();
				// Если контекст не создан
				if(state.ctx == nullptr){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Error during AES context creation", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (cipher), static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Error during AES context creation", log_t::flag_t::CRITICAL);
					#endif
					// Выходим из метода
					return result;
				}
				/**
				 * Определяем событие кодирование или декодирование
				 */
				switch(static_cast <uint8_t> (event)){
					// Если производится кодирование данных
					case static_cast <uint8_t> (event_t::ENCODE): {
						/**
						 * Вектор инициализации берётся случайным на каждый поток и
						 * выписывается в его начало первой же порцией выхода
						 */
						// Выполняем формирование случайного вектора инициализации
						if(::RAND_bytes(state.ivec.data(), static_cast <int32_t> (state.ivec.size())) != 1){
							// Выполняем сброс стейта AES-шифрования
							state.reset();
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Error during initialization vector generation", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (cipher), static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Error during initialization vector generation", log_t::flag_t::CRITICAL);
							#endif
							// Выходим из метода
							return result;
						}
						// Выполняем инициализацию контекста шифрования
						if(!(result = (::EVP_CipherInit_ex(const_cast <EVP_CIPHER_CTX *> (state.ctx), state.evp, nullptr, state.key.data(), state.ivec.data(), AES_ENCRYPT) == 1))){
							// Выполняем сброс стейта AES-шифрования
							state.reset();
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Error during AES context initialization", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (cipher), static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Error during AES context initialization", log_t::flag_t::CRITICAL);
							#endif
							// Выходим из метода
							return result;
						}
						// Отключаем padding (обязательно для потоковых режимов)
						::EVP_CIPHER_CTX_set_padding(const_cast <EVP_CIPHER_CTX *> (state.ctx), 0);
					} break;
					// Если производится декодирование данных
					case static_cast <uint8_t> (event_t::DECODE):
						/**
						 * Контекст расшифровки здесь не заводится: вектор инициализации
						 * стоит в начале шифротекста и придёт первой же порцией входа,
						 * а до него завести контекст нечем
						 */
						// Помечаем успешную инициализацию потоковой расшифровки
						result = true;
					break;
					// Если направление работы не задано
					default: {
						/**
						 * Направление, разбору не знакомое, отменяет инициализацию, а
						 * контекст при этом освобождается: прежде он оставался заведённым
						 * и неинициализированным, и всякая следующая попытка видела его
						 * заведённым и отвечала отказом безвозвратно
						 */
						// Выполняем сброс стейта AES-шифрования
						state.reset();
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("Direction of the stream cipher is not set", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (cipher), static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("Direction of the stream cipher is not set", log_t::flag_t::CRITICAL);
						#endif
						// Выходим из метода
						return result;
					}
				}
				// Помечаем ожидание вектора инициализации в начале потока
				state.pending = true;
				::EVP_CIPHER_CTX_set_padding(const_cast <EVP_CIPHER_CTX *> (state.ctx), 0);
				// Запоминаем направление работы потокового шифрования
				state.event = event;
			}
		/**
		 * Если пароль шифрования не установлен
		 */
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Password of the stream cipher is not set", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (hash), static_cast <uint16_t> (cipher)), log_t::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Password of the stream cipher is not set", log_t::flag_t::CRITICAL);
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (hash), static_cast <uint16_t> (cipher)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат работы функции
	return result;
}
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
auto awh::Crypto::encrypt(string_view buffer, const hash_t hash, const cipher_t cipher) const noexcept -> T {
	// Выполняем кодирование
	return this->encrypt <T> (buffer.data(), buffer.size(), hash, cipher);
}
/**
 * @brief Явный специализированный шаблон метода кодирования данных из строки с выводом результата в строку
 *
 */
template string awh::Crypto::encrypt(string_view, const hash_t, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода кодирования данных из строки с выводом результата в буфер
 *
 */
template vector <char> awh::Crypto::encrypt(string_view, const hash_t, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода кодирования данных из строки с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Crypto::encrypt(string_view, const hash_t, const cipher_t) const noexcept;
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
auto awh::Crypto::encrypt(const B & buffer, const hash_t hash, const cipher_t cipher) const noexcept -> A {
	// Выполняем кодирование
	return this->encrypt <A> (buffer.data(), buffer.size(), hash, cipher);
}
/**
 * @brief Явный специализированный шаблон метода кодирования данных из строки с выводом результата в строку
 *
 */
template string awh::Crypto::encrypt(const string &, const hash_t, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода кодирования данных из буфера с выводом результата в строку
 *
 */
template string awh::Crypto::encrypt(const vector <char> &, const hash_t, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода кодирования данных из бинарного буфера с выводом результата в строку
 *
 */
template string awh::Crypto::encrypt(const vector <uint8_t> &, const hash_t, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода кодирования данных из строки с выводом результата в буфер
 *
 */
template vector <char> awh::Crypto::encrypt(const string &, const hash_t, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода кодирования данных из буфера с выводом результата в буфер
 *
 */
template vector <char> awh::Crypto::encrypt(const vector <char> &, const hash_t, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода кодирования данных из бинарного буфера с выводом результата в буфер
 *
 */
template vector <char> awh::Crypto::encrypt(const vector <uint8_t> &, const hash_t, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода кодирования данных из строки с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Crypto::encrypt(const string &, const hash_t, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода кодирования данных из буфера с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Crypto::encrypt(const vector <char> &, const hash_t, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода кодирования данных из бинарного буфера с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Crypto::encrypt(const vector <uint8_t> &, const hash_t, const cipher_t) const noexcept;
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
auto awh::Crypto::encrypt(const void * buffer, const size_t size, const hash_t hash, const cipher_t cipher) const noexcept -> T {
	// Переменная результата
	T result;
	/**
	 * Пустое сообщение работой принимается: в режиме с проверкой подлинности
	 * у него есть шифротекст - вектор инициализации и имитовставка, а потоковый
	 * путь пустое сообщение принимал и прежде. Отказ по одному лишь нулевому
	 * размеру расходил one-shot с потоком
	 */
	// Если буфер данных передан
	if((buffer != nullptr) || (size == 0)){
		/**
		 * Состояние берётся изменяемой ссылкой: постоянство работы относится к
		 * самому объекту, а состояние потока живёт за указателем и его постоянством
		 * не связано - приведение постоянства здесь лишь скрывало это правило
		 */
		// Получаем состояние объекта
		state_t & state = (* this->_params.state);
		/**
		 * Тип шифрования и тип хэш-суммы берутся у заведённого контекста, когда
		 * самим вызовом они не заданы: потоковый режим держался на том, что метка
		 * незаданного шифрования совпадала с меткой AES256 по младшему октету, и
		 * работа после инициализации попадала в ветвь AES по совпадению значений.
		 * Теперь она попадает туда по существу - потому что контекст заведён
		 */
		// Признак работы поверх заведённого контекста потокового шифрования
		const bool stream = ((cipher == cipher_t::NONE) && (state.ctx != nullptr));
		// Тип шифрования, которым выполняется работа
		const cipher_t actual = (stream ? state.cipher : cipher);
		// Тип хэш-суммы, которым выводится ключ шифрования
		const hash_t digest = (stream ? state.hash : hash);
		/**
		 * Определяем тип шифрования
		 */
		switch(static_cast <uint16_t> (actual)){
			// Если производится работы с BASE64
			case static_cast <uint16_t> (cipher_t::BASE64): {
				// Выполняем кодирование строки BASE64 и получаем признак успеха
				const bool success = driver::hash(reinterpret_cast <const char *> (buffer), size, actual, event_t::ENCODE, state, result, this->_log);
				// Если кодирование не вышло
				if(!success){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Unable to encrypt \"%s\" string data into BASE64 format", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (digest), static_cast <uint16_t> (actual)), log_t::flag_t::WARNING, string(reinterpret_cast <const char *> (buffer), size).c_str());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Unable to encrypt \"%s\" string data into BASE64 format", log_t::flag_t::WARNING, string(reinterpret_cast <const char *> (buffer), size).c_str());
					#endif
				}
			} break;
			// Если производится работы с AES128
			case static_cast <uint16_t> (cipher_t::AES128):
			// Если производится работы с AES192
			case static_cast <uint16_t> (cipher_t::AES192):
			// Если производится работы с AES256
			case static_cast <uint16_t> (cipher_t::AES256): {
				/**
				 * Отказ определяется признаком работы, а не пустотой выхода: в потоке
				 * порция без выхода законна - контекст накапливает вектор инициализации
				 * либо удерживает хвост, - и звать её отказом значило бы засорять лог
				 * ложными предупреждениями. Пустой выход законен и у разовой работы:
				 * расшифровка пустого сообщения даёт пустой открытый текст
				 */
				// Признак успешно выполненной работы
				bool success = false;
				// Если пароль установлен
				if(!this->_params.password.empty()){
					// Если контекст шифрования не создан
					if(state.ctx == nullptr){
						// Проверяем текущее состояние
						if((state.hash != digest) || (state.cipher != actual)){
							// Если инициализация ключей не выполнена
							if(!driver::cipher(actual, this->_params.mode, digest, this->_params.password, this->_params.salt, this->_params.rounds, state, this->_log)){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("Unable to initialize AES cipher for encoding data", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (digest), static_cast <uint16_t> (actual)), log_t::flag_t::CRITICAL);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("Unable to initialize AES cipher for encoding data", log_t::flag_t::CRITICAL);
								#endif
								// Выходим из метода
								return result;
							}
						}
						// Выполняем шифрование данных
						success = driver::hash(reinterpret_cast <const char *> (buffer), size, actual, event_t::ENCODE, state, result, this->_log);
					// Если контекст шифрования уже создан
					} else {
						/**
						 * Заведённый контекст потокового шифрования переопределить
						 * доводами вызова нельзя: разрядность и тип хэш-суммы задаются
						 * при инициализации потока и живут в самом контексте. Прежде
						 * доводы вызова здесь молча отбрасывались - работа думала, что
						 * шифрует одной разрядностью, а шифровала другой
						 */
						// Если доводы вызова расходятся с заведённым контекстом
						if(!stream && ((state.cipher != actual) || (state.hash != digest))){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Cipher of the call does not match the one the stream was initialized with", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (digest), static_cast <uint16_t> (actual)), log_t::flag_t::CRITICAL);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Cipher of the call does not match the one the stream was initialized with", log_t::flag_t::CRITICAL);
							#endif
							// Выходим из метода
							return result;
						}
						// Выполняем шифрование данных
						success = driver::hash(reinterpret_cast <const char *> (buffer), size, state.cipher, event_t::ENCODE, state, result, this->_log);
					}
				}
				// Если кодирование не вышло
				if(!success){
					// Не возвращаем открытый текст: при неудаче шифрования результат остаётся пустым
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Unable to encrypt data into AES", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (digest), static_cast <uint16_t> (actual)), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Unable to encrypt data into AES", log_t::flag_t::WARNING);
					#endif
				}
			} break;
			// Если тип шифрования не установлен либо разбору не знаком
			default: {
				/**
				 * Тип шифрования, разбору не знакомый, отвергается явно: прежде
				 * отбор шёл по младшему октету значения, а разрядность AES256 в него
				 * не умещается - его метка совпадала с меткой незаданного шифрования,
				 * и работа без шифрования уходила в ветвь AES
				 */
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("Unable to encrypt data, cipher type is not set", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (digest), static_cast <uint16_t> (actual)), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Unable to encrypt data, cipher type is not set", log_t::flag_t::WARNING);
				#endif
			} break;
		}
	}
	// Возвращаем результат работы функции
	return result;
}
/**
 * @brief Явный специализированный шаблон метода кодирования с выводом результата в строку
 *
 */
template string awh::Crypto::encrypt(const void *, const size_t, const hash_t, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода кодирования с выводом результата в буфер
 *
 */
template vector <char> awh::Crypto::encrypt(const void *, const size_t, const hash_t, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода кодирования с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Crypto::encrypt(const void *, const size_t, const hash_t, const cipher_t) const noexcept;
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
auto awh::Crypto::decrypt(string_view buffer, const hash_t hash, const cipher_t cipher) const noexcept -> T {
	// Выполняем декодирование
	return this->decrypt <T> (buffer.data(), buffer.size(), hash, cipher);
}
/**
 * @brief Явный специализированный шаблон метода декодирования данных из строки с выводом результата в строку
 *
 */
template string awh::Crypto::decrypt(string_view, const hash_t, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декодирования данных из строки с выводом результата в буфер
 *
 */
template vector <char> awh::Crypto::decrypt(string_view, const hash_t, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декодирования данных из строки с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Crypto::decrypt(string_view, const hash_t, const cipher_t) const noexcept;
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
auto awh::Crypto::decrypt(const B & buffer, const hash_t hash, const cipher_t cipher) const noexcept -> A {
	// Выполняем декодирование
	return this->decrypt <A> (buffer.data(), buffer.size(), hash, cipher);
}
/**
 * @brief Явный специализированный шаблон метода декодирования данных из строки с выводом результата в строку
 *
 */
template string awh::Crypto::decrypt(const string &, const hash_t, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декодирования данных из буфера с выводом результата в строку
 *
 */
template string awh::Crypto::decrypt(const vector <char> &, const hash_t, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декодирования данных из бинарного буфера с выводом результата в строку
 *
 */
template string awh::Crypto::decrypt(const vector <uint8_t> &, const hash_t, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декодирования данных из строки с выводом результата в буфер
 *
 */
template vector <char> awh::Crypto::decrypt(const string &, const hash_t, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декодирования данных из буфера с выводом результата в буфер
 *
 */
template vector <char> awh::Crypto::decrypt(const vector <char> &, const hash_t, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декодирования данных из бинарного буфера с выводом результата в буфер
 *
 */
template vector <char> awh::Crypto::decrypt(const vector <uint8_t> &, const hash_t, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декодирования данных из строки с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Crypto::decrypt(const string &, const hash_t, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декодирования данных из буфера с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Crypto::decrypt(const vector <char> &, const hash_t, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декодирования данных из бинарного буфера с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Crypto::decrypt(const vector <uint8_t> &, const hash_t, const cipher_t) const noexcept;
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
auto awh::Crypto::decrypt(const void * buffer, const size_t size, const hash_t hash, const cipher_t cipher) const noexcept -> T {
	// Переменная результата
	T result;
	/**
	 * Пустое сообщение работой принимается: в режиме с проверкой подлинности
	 * у него есть шифротекст - вектор инициализации и имитовставка, а потоковый
	 * путь пустое сообщение принимал и прежде. Отказ по одному лишь нулевому
	 * размеру расходил one-shot с потоком
	 */
	// Если буфер данных передан
	if((buffer != nullptr) || (size == 0)){
		/**
		 * Состояние берётся изменяемой ссылкой: постоянство работы относится к
		 * самому объекту, а состояние потока живёт за указателем и его постоянством
		 * не связано - приведение постоянства здесь лишь скрывало это правило
		 */
		// Получаем состояние объекта
		state_t & state = (* this->_params.state);
		/**
		 * Тип шифрования и тип хэш-суммы берутся у заведённого контекста, когда
		 * самим вызовом они не заданы: потоковый режим держался на том, что метка
		 * незаданного шифрования совпадала с меткой AES256 по младшему октету, и
		 * работа после инициализации попадала в ветвь AES по совпадению значений.
		 * Теперь она попадает туда по существу - потому что контекст заведён
		 */
		// Признак работы поверх заведённого контекста потокового шифрования
		const bool stream = ((cipher == cipher_t::NONE) && (state.ctx != nullptr));
		// Тип шифрования, которым выполняется работа
		const cipher_t actual = (stream ? state.cipher : cipher);
		// Тип хэш-суммы, которым выводится ключ шифрования
		const hash_t digest = (stream ? state.hash : hash);
		/**
		 * Определяем тип шифрования
		 */
		switch(static_cast <uint16_t> (actual)){
			// Если производится работы с BASE64
			case static_cast <uint16_t> (cipher_t::BASE64): {
				// Выполняем декодирование строки BASE64 и получаем признак успеха
				const bool success = driver::hash(reinterpret_cast <const char *> (buffer), size, actual, event_t::DECODE, state, result, this->_log);
				// Если декодирование не вышло
				if(!success){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Unable to extract data from BASE64 encoded \"%s\" digest", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (digest), static_cast <uint16_t> (actual)), log_t::flag_t::WARNING, string(reinterpret_cast <const char *> (buffer), size).c_str());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Unable to extract data from BASE64 encoded \"%s\" digest", log_t::flag_t::WARNING, string(reinterpret_cast <const char *> (buffer), size).c_str());
					#endif
				}
			} break;
			// Если производится работы с AES128
			case static_cast <uint16_t> (cipher_t::AES128):
			// Если производится работы с AES192
			case static_cast <uint16_t> (cipher_t::AES192):
			// Если производится работы с AES256
			case static_cast <uint16_t> (cipher_t::AES256): {
				/**
				 * Отказ определяется признаком работы, а не пустотой выхода: в потоке
				 * порция без выхода законна - контекст накапливает вектор инициализации
				 * либо удерживает хвост, - и звать её отказом значило бы засорять лог
				 * ложными предупреждениями. Пустой выход законен и у разовой работы:
				 * расшифровка пустого сообщения даёт пустой открытый текст
				 */
				// Признак успешно выполненной работы
				bool success = false;
				// Если пароль установлен
				if(!this->_params.password.empty()){
					// Если контекст шифрования не создан
					if(state.ctx == nullptr){
						// Проверяем текущее состояние
						if((state.hash != digest) || (state.cipher != actual)){
							// Если инициализация ключей не выполнена
							if(!driver::cipher(actual, this->_params.mode, digest, this->_params.password, this->_params.salt, this->_params.rounds, state, this->_log)){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("Unable to initialize AES cipher for encoding data", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (digest), static_cast <uint16_t> (actual)), log_t::flag_t::CRITICAL);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("Unable to initialize AES cipher for encoding data", log_t::flag_t::CRITICAL);
								#endif
								// Выходим из метода
								return result;
							}
						}
						// Выполняем дешифрование данных
						success = driver::hash(reinterpret_cast <const char *> (buffer), size, actual, event_t::DECODE, state, result, this->_log);
					// Если контекст шифрования уже создан
					} else {
						/**
						 * Заведённый контекст потокового шифрования переопределить
						 * доводами вызова нельзя: разрядность и тип хэш-суммы задаются
						 * при инициализации потока и живут в самом контексте. Прежде
						 * доводы вызова здесь молча отбрасывались - работа думала, что
						 * расшифровывает одной разрядностью, а расшифровывала другой
						 */
						// Если доводы вызова расходятся с заведённым контекстом
						if(!stream && ((state.cipher != actual) || (state.hash != digest))){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Cipher of the call does not match the one the stream was initialized with", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (digest), static_cast <uint16_t> (actual)), log_t::flag_t::CRITICAL);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Cipher of the call does not match the one the stream was initialized with", log_t::flag_t::CRITICAL);
							#endif
							// Выходим из метода
							return result;
						}
						// Выполняем дешифрование данных
						success = driver::hash(reinterpret_cast <const char *> (buffer), size, state.cipher, event_t::DECODE, state, result, this->_log);
					}
				}
				// Если дешифрование не вышло
				if(!success){
					// Не возвращаем зашифрованные данные: при неудаче дешифрования результат остаётся пустым
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Unable to decrypt data from AES", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (digest), static_cast <uint16_t> (actual)), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Unable to decrypt data from AES", log_t::flag_t::WARNING);
					#endif
				}
			} break;
			// Если тип шифрования не установлен либо разбору не знаком
			default: {
				/**
				 * Тип шифрования, разбору не знакомый, отвергается явно: прежде
				 * отбор шёл по младшему октету значения, а разрядность AES256 в него
				 * не умещается - его метка совпадала с меткой незаданного шифрования,
				 * и работа без шифрования уходила в ветвь AES
				 */
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("Unable to decrypt data, cipher type is not set", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (digest), static_cast <uint16_t> (actual)), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Unable to decrypt data, cipher type is not set", log_t::flag_t::WARNING);
				#endif
			} break;
		}
	}
	// Возвращаем результат работы функции
	return result;
}
/**
 * @brief Явный специализированный шаблон метода декодирования с выводом результата в строку
 *
 */
template string awh::Crypto::decrypt(const void *, const size_t, const hash_t, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декодирования с выводом результата в буфер
 *
 */
template vector <char> awh::Crypto::decrypt(const void *, const size_t, const hash_t, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декодирования с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Crypto::decrypt(const void *, const size_t, const hash_t, const cipher_t) const noexcept;
/**
 * @brief Метод генерации приватного ключа RSA
 *
 * @param size размер ключа в битах, от 2048 и выше (0 — по умолчанию)
 * @return     результат генерации ключа
 *
 */
bool awh::Crypto::generatePrivateKeyRSA(const size_t size) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Разрядность ключа отвергается здесь, а не в глубине выработки: ключ
		 * короче двух тысяч разрядов стойкости не имеет, а незаданная разрядность
		 * означает разрядность по умолчанию, а не отказ
		 */
		// Разрядность вырабатываемого ключа
		const size_t bits = ((size == 0) ? AWH_CRYPTO_RSA_BITS : size);
		// Если разрядность ключа стойкости не имеет
		if(bits < AWH_CRYPTO_RSA_BITS){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Private key size is too small to be secure", __PRETTY_FUNCTION__, make_tuple(size), log_t::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Private key size is too small to be secure", log_t::flag_t::CRITICAL);
			#endif
			// Выходим из метода
			return result;
		}
		// Выполняем генерацию ключа RSA
		EVP_PKEY * privKey = ::EVP_RSA_gen(bits);
		// Если ключ не получен
		if(privKey == nullptr){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Private key generation failed", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Private key generation failed", log_t::flag_t::CRITICAL);
			#endif
			// Возвращаем результат
			return result;
		}
		// Получаем ссылку на объект ключа RSA
		key_rsa_t & key = (* this->_params.key);
		/**
		 * Прежде заведённый ключ освобождается: запись нового поверх старого
		 * теряла его безвозвратно, тогда как установка ключа извне и загрузка
		 * его из файла освобождение выполняют
		 */
		// Если ключ RSA уже был заведён
		if(key.ctx != nullptr)
			// Освобождаем память выделенную под прежний ключ
			::EVP_PKEY_free(key.ctx);
		// Устанавливаем тип ключа
		key.type = key_type_t::PRIVATE;
		// Запоминаем приватный ключ
		key.ctx = privKey;
		// Формируем итоговый результат
		result = (key.ctx != nullptr);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод получения публичного ключа RSA
 *
 * @return публичный ключ RSA
 *
 */
string awh::Crypto::getPublicKeyRSA() const noexcept {
	// Переменная результата
	string result = "";
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Получаем ссылку на объект ключа RSA
		const key_rsa_t & key = (* this->_params.key);
		// Если ключ загружен
		if(key.ctx != nullptr){
			// Создаём объект BIO для записи публичного ключа
			BIO * bio = ::BIO_new(::BIO_s_mem());
			// Если объект BIO создан успешно
			if(bio != nullptr){
				// Если файл не может быть записан
				if(::PEM_write_bio_PUBKEY(bio, key.ctx) != 1){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Public key export failed", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Public key export failed", log_t::flag_t::CRITICAL);
					#endif
				}
				// Получаем указатель на данные из объекта BIO
				char * buffer = nullptr;
				// Получаем размер данных из объекта BIO
				const size_t size = static_cast <size_t> (::BIO_get_mem_data(bio, &buffer));
				// Если данные получены успешно
				if((buffer != nullptr) && (size > 0))
					// Записываем данные в результат
					result.assign(buffer, size);
				// Освобождаем объект BIO
				::BIO_free_all(bio);
			// Если объект BIO не создан
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("Public key BIO creation failed", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Public key BIO creation failed", log_t::flag_t::CRITICAL);
				#endif
			}
		// Если путь к файлу не передан или публичный ключ не загружен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Public key is not loaded", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Public key is not loaded", log_t::flag_t::WARNING);
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки публичного ключа RSA
 *
 * @param key публичный ключ RSA
 * @return    результат установки ключа
 *
 */
bool awh::Crypto::setPublicKeyRSA(string_view key) noexcept {
	// Переменная результата
	bool result = false;
	// Если публичный ключ передан
	if(!key.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем парсинг ключа
			BIO * bio = ::BIO_new_mem_buf(key.data(), static_cast <int32_t> (key.size()));
			// Если объект BIO создан успешно
			if(bio != nullptr){
				// Читаем публичный ключ из буфера
				EVP_PKEY * pkey = ::PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
				// Освобождаем объект BIO
				::BIO_free(bio);
				// Если публичный ключ получен
				if(pkey != nullptr){
					// Получаем ссылку на объект ключа RSA
					key_rsa_t & key = (* this->_params.key);
					// Если публичный ключ уже сгенерирован
					if(key.ctx != nullptr)
						// Освобождаем публичный ключ
						::EVP_PKEY_free(key.ctx);
					// Запоминаем публичный ключ
					key.ctx = pkey;
					// Устанавливаем тип ключа
					key.type = key_type_t::PUBLIC;
					// Формируем итоговый результат
					result = (key.ctx != nullptr);
				// Если публичный ключ не получен
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Public key import failed", __PRETTY_FUNCTION__, make_tuple(key), log_t::flag_t::CRITICAL);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Public key import failed", log_t::flag_t::CRITICAL);
					#endif
				}
			// Если объект BIO не создан
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("Public key BIO import failed", __PRETTY_FUNCTION__, make_tuple(key), log_t::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Public key BIO import failed", log_t::flag_t::CRITICAL);
				#endif
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(key), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки приватного ключа RSA
 *
 * @param key приватный ключ RSA
 * @return    результат установки ключа
 *
 */
bool awh::Crypto::setPrivateKeyRSA(string_view key) noexcept {
	// Переменная результата
	bool result = false;
	// Если приватный ключ передан
	if(!key.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем парсинг ключа
			BIO * bio = ::BIO_new_mem_buf(key.data(), static_cast <int32_t> (key.size()));
			// Если объект BIO создан успешно
			if(bio != nullptr){
				// Читаем приватный ключ из буфера
				EVP_PKEY * pkey = ::PEM_read_bio_PrivateKey(bio, nullptr, nullptr, this->_params.passwordRSA.empty() ? nullptr : reinterpret_cast <void *> (&this->_params.passwordRSA.front()));
				// Освобождаем объект BIO
				::BIO_free(bio);
				// Если приватный ключ получен
				if(pkey != nullptr){
					// Получаем ссылку на объект ключа RSA
					key_rsa_t & key = (* this->_params.key);
					// Если приватный ключ уже сгенерирован
					if(key.ctx != nullptr)
						// Освобождаем приватный ключ
						::EVP_PKEY_free(key.ctx);
					// Запоминаем приватный ключ
					key.ctx = pkey;
					// Устанавливаем тип ключа
					key.type = key_type_t::PRIVATE;
					// Формируем итоговый результат
					result = (key.ctx != nullptr);
				// Если приватный ключ не получен
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Private key import failed", __PRETTY_FUNCTION__, make_tuple(key), log_t::flag_t::CRITICAL);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Private key import failed", log_t::flag_t::CRITICAL);
					#endif
				}
			// Если объект BIO не создан
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("Private key BIO import failed", __PRETTY_FUNCTION__, make_tuple(key), log_t::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Private key BIO import failed", log_t::flag_t::CRITICAL);
				#endif
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(key), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод получения приватного ключа RSA
 *
 * @param cipher тип шифрования приватного ключа
 * @return       приватный ключ RSA
 *
 */
string awh::Crypto::getPrivateKeyRSA(const cipher_t cipher) const noexcept {
	// Переменная результата
	string result = "";
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Получаем ссылку на объект ключа RSA
		const key_rsa_t & key = (* this->_params.key);
		// Если ключ загружен
		if(key.ctx != nullptr){
			// Если ключ является приватным
			if(key.type == key_type_t::PRIVATE){
				// Создаём объект BIO для записи приватного ключа
				BIO * bio = ::BIO_new(::BIO_s_mem());
				// Если объект BIO создан успешно
				if(bio != nullptr){
					// Если пароль защиты приватного ключа не установлен
					if(this->_params.passwordRSA.empty()){
						// Если файл не может быть записан
						if(::PEM_write_bio_PrivateKey(bio, key.ctx, nullptr, nullptr, 0, nullptr, nullptr) != 1){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Private key export failed", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Private key export failed", log_t::flag_t::CRITICAL);
							#endif
						}
					// Если пароль установлен
					} else {
						// Тип шифрования AES
						const EVP_CIPHER * evp = ::EVP_enc_null();
						/**
						 * Определяем длину шифрования
						 */
						switch(static_cast <uint16_t> (cipher)){
							// Устанавливаем шифрование в 128
							case static_cast <uint16_t> (cipher_t::AES128):
								// Устанавливаем функцию шифрования
								evp = ::EVP_aes_128_cbc();
							break;
							// Устанавливаем шифрование в 192
							case static_cast <uint16_t> (cipher_t::AES192):
								// Устанавливаем функцию шифрования
								evp = ::EVP_aes_192_cbc();
							break;
							// Устанавливаем шифрование в 256
							case static_cast <uint16_t> (cipher_t::AES256):
							// Разрядность шифрования по умолчанию равна наибольшей
							case static_cast <uint16_t> (cipher_t::NONE):
								// Устанавливаем функцию шифрования
								evp = ::EVP_aes_256_cbc();
							break;
							/**
							 * Тип шифрования, защите ключа не подходящий, отвергается явно: прежде
							 * он молча подменялся наибольшей разрядностью, и работа думала, что ключ
							 * защищён тем шифром, который она назвала
							 */
							// Если тип шифрования разбору не знаком
							default:
								// Снимаем функцию шифрования
								evp = nullptr;
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("Cipher type is not suitable for private key protection", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (cipher)), log_t::flag_t::CRITICAL);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("Cipher type is not suitable for private key protection", log_t::flag_t::CRITICAL);
								#endif
						}
						// Если файл не может быть записан
						if((evp == nullptr) || (::PEM_write_bio_PKCS8PrivateKey(bio, key.ctx, evp, this->_params.passwordRSA.c_str(), static_cast <int32_t> (this->_params.passwordRSA.size()), nullptr, nullptr) != 1)){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Private key export failed", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Private key export failed", log_t::flag_t::CRITICAL);
							#endif
						}
					}
					// Получаем указатель на данные из объекта BIO
					char * buffer = nullptr;
					// Получаем размер данных из объекта BIO
					const size_t size = static_cast <size_t> (::BIO_get_mem_data(bio, &buffer));
					// Если данные получены успешно
					if((buffer != nullptr) && (size > 0))
						// Записываем данные в результат
						result.assign(buffer, size);
					// Освобождаем объект BIO
					::BIO_free_all(bio);
				// Если объект BIO не создан
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Private key BIO creation failed", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Private key BIO creation failed", log_t::flag_t::CRITICAL);
					#endif
				}
			// Если ключ не является приватным
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("Key cannot be export because it is not private", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Key cannot be export because it is not private", log_t::flag_t::CRITICAL);
				#endif
			}
		// Если ключ не загружен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Private key is not loaded", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Private key is not loaded", log_t::flag_t::WARNING);
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод загрузки публичного ключа RSA из файла
 *
 * @param path путь к файлу с публичным ключом
 * @return     результат загрузки ключа
 *
 */
bool awh::Crypto::loadPublicKeyRSA(string_view path) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если путь к файлу передан правильно
		if(!path.empty()){
			/**
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				// Открываем файл с публичным ключом
				FILE * file = ::_wfopen(this->_fmk->convert(path).c_str(), L"rb");
				// Если файл открыт удачно
				if(file != nullptr){
					// Читаем публичный ключ из файла
					EVP_PKEY * pkey = ::PEM_read_PUBKEY(file, nullptr, nullptr, nullptr);
					// Закрываем файл
					::fclose(file);
					// Если публичный ключ получен
					if(pkey != nullptr){
						// Получаем ссылку на объект ключа RSA
						key_rsa_t & key = (* this->_params.key);
						// Если публичный ключ уже сгенерирован
						if(key.ctx != nullptr)
							// Освобождаем публичный ключ
							::EVP_PKEY_free(key.ctx);
						// Запоминаем публичный ключ
						key.ctx = pkey;
						// Устанавливаем тип ключа
						key.type = key_type_t::PUBLIC;
						// Формируем итоговый результат
						result = (key.ctx != nullptr);
					// Если публичный ключ не получен
					} else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("Public key reading failed", __PRETTY_FUNCTION__, make_tuple(path), log_t::flag_t::CRITICAL);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("Public key reading failed", log_t::flag_t::CRITICAL);
						#endif
					}
				// Если файл не открыт
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Public key file opening failed", __PRETTY_FUNCTION__, make_tuple(path), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Public key file opening failed", log_t::flag_t::WARNING);
					#endif
				}
			/**
			 * Для других операционных систем
			 */
			#else
				// Открываем файл с публичным ключом
				/**
				 * Путь снимается в свою строку: обозрение строки завершающего нуля
				 * не обещает, и подача его указателя открывала бы не тот файл либо
				 * уводила чтение за границу обозреваемого
				 */
				// Имя файла, завершающим нулём оканчивающееся
				const string filename(path);
				FILE * file = ::fopen(filename.c_str(), "rb");
				// Если файл открыт удачно
				if(file != nullptr){
					// Читаем публичный ключ из файла
					EVP_PKEY * pkey = ::PEM_read_PUBKEY(file, nullptr, nullptr, nullptr);
					// Закрываем файл
					::fclose(file);
					// Если публичный ключ получен
					if(pkey != nullptr){
						// Получаем ссылку на объект ключа RSA
						key_rsa_t & key = (* this->_params.key);
						// Если публичный ключ уже сгенерирован
						if(key.ctx != nullptr)
							// Освобождаем публичный ключ
							::EVP_PKEY_free(key.ctx);
						// Запоминаем публичный ключ
						key.ctx = pkey;
						// Устанавливаем тип ключа
						key.type = key_type_t::PUBLIC;
						// Формируем итоговый результат
						result = (key.ctx != nullptr);
					// Если публичный ключ не получен
					} else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("Public key reading failed", __PRETTY_FUNCTION__, make_tuple(path), log_t::flag_t::CRITICAL);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("Public key reading failed", log_t::flag_t::CRITICAL);
						#endif
					}
				// Если файл не открыт
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Public key file opening failed", __PRETTY_FUNCTION__, make_tuple(path), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Public key file opening failed", log_t::flag_t::WARNING);
					#endif
				}
			#endif
		// Если путь к файлу не передан
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Invalid path for public key", __PRETTY_FUNCTION__, make_tuple(path), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Invalid path for public key", log_t::flag_t::WARNING);
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(path), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод загрузки приватного ключа RSA из файла
 *
 * @param path путь к файлу с приватным ключом
 * @return     результат загрузки ключа
 *
 */
bool awh::Crypto::loadPrivateKeyRSA(string_view path) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если путь к файлу передан правильно
		if(!path.empty()){
			/**
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				// Открываем файл с приватным ключом
				FILE * file = ::_wfopen(this->_fmk->convert(path).c_str(), L"rb");
				// Если файл открыт удачно
				if(file != nullptr){
					// Читаем приватный ключ из файла
					EVP_PKEY * pkey = ::PEM_read_PrivateKey(file, nullptr, nullptr, this->_params.passwordRSA.empty() ? nullptr : reinterpret_cast <void *> (&this->_params.passwordRSA.front()));
					// Закрываем файл
					::fclose(file);
					// Если приватный ключ получен
					if(pkey != nullptr){
						// Получаем ссылку на объект ключа RSA
						key_rsa_t & key = (* this->_params.key);
						// Если приватный ключ уже сгенерирован
						if(key.ctx != nullptr)
							// Освобождаем приватный ключ
							::EVP_PKEY_free(key.ctx);
						// Запоминаем приватный ключ
						key.ctx = pkey;
						// Устанавливаем тип ключа
						key.type = key_type_t::PRIVATE;
						// Формируем итоговый результат
						result = (key.ctx != nullptr);
					// Если публичный ключ не получен
					} else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("Private key reading failed", __PRETTY_FUNCTION__, make_tuple(path), log_t::flag_t::CRITICAL);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("Private key reading failed", log_t::flag_t::CRITICAL);
						#endif
					}
				// Если файл не открыт
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Private key file opening failed", __PRETTY_FUNCTION__, make_tuple(path), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Private key file opening failed", log_t::flag_t::WARNING);
					#endif
				}
			/**
			 * Для других операционных систем
			 */
			#else
				/**
				 * Путь снимается в свою строку: обозрение строки завершающего нуля
				 * не обещает, и подача его указателя открывала бы не тот файл либо
				 * уводила чтение за границу обозреваемого
				 */
				// Имя файла, завершающим нулём оканчивающееся
				const string filename(path);
				// Открываем файл с приватным ключом
				FILE * file = ::fopen(filename.c_str(), "rb");
				// Если файл открыт удачно
				if(file != nullptr){
					// Читаем приватный ключ из файла
					EVP_PKEY * pkey = ::PEM_read_PrivateKey(file, nullptr, nullptr, this->_params.passwordRSA.empty() ? nullptr : reinterpret_cast <void *> (&this->_params.passwordRSA.front()));
					// Закрываем файл
					::fclose(file);
					// Если приватный ключ получен
					if(pkey != nullptr){
						// Получаем ссылку на объект ключа RSA
						key_rsa_t & key = (* this->_params.key);
						// Если приватный ключ уже сгенерирован
						if(key.ctx != nullptr)
							// Освобождаем приватный ключ
							::EVP_PKEY_free(key.ctx);
						// Запоминаем приватный ключ
						key.ctx = pkey;
						// Устанавливаем тип ключа
						key.type = key_type_t::PRIVATE;
						// Формируем итоговый результат
						result = (key.ctx != nullptr);
					// Если публичный ключ не получен
					} else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("Private key reading failed", __PRETTY_FUNCTION__, make_tuple(path), log_t::flag_t::CRITICAL);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("Private key reading failed", log_t::flag_t::CRITICAL);
						#endif
					}
				// Если файл не открыт
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Private key file opening failed", __PRETTY_FUNCTION__, make_tuple(path), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Private key file opening failed", log_t::flag_t::WARNING);
					#endif
				}
			#endif
		// Если путь к файлу не передан
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Invalid path for private key", __PRETTY_FUNCTION__, make_tuple(path), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Invalid path for private key", log_t::flag_t::WARNING);
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(path), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод сохранения публичного ключа RSA в файл
 *
 * @param path путь к файлу для сохранения публичного ключа
 * @return     результат сохранения ключа
 *
 */
bool awh::Crypto::savePublicKeyRSA(string_view path) const noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Получаем ссылку на объект ключа RSA
		const key_rsa_t & key = (* this->_params.key);
		// Если путь к файлу передан правильно
		if(!path.empty() && (key.ctx != nullptr)){
			/**
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				// Сохраняем публичный ключ
				FILE * file = ::_wfopen(this->_fmk->convert(path).c_str(), L"wb");
				// Если файл открыт удачно
				if(file != nullptr){
					// Если файл не может быть записан
					if(!(result = (::PEM_write_PUBKEY(file, key.ctx) == 1))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("Public key saving failed", __PRETTY_FUNCTION__, make_tuple(path), log_t::flag_t::CRITICAL);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("Public key saving failed", log_t::flag_t::CRITICAL);
						#endif
					}
					// Закрываем файл
					::fclose(file);
				// Если файл не открыт
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Public key file opening failed", __PRETTY_FUNCTION__, make_tuple(path), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Public key file opening failed", log_t::flag_t::WARNING);
					#endif
				}
			/**
			 * Для других операционных систем
			 */
			#else
				// Сохраняем публичный ключ
				/**
				 * Путь снимается в свою строку: обозрение строки завершающего нуля
				 * не обещает, и подача его указателя открывала бы не тот файл либо
				 * уводила чтение за границу обозреваемого
				 */
				// Имя файла, завершающим нулём оканчивающееся
				const string filename(path);
				FILE * file = ::fopen(filename.c_str(), "wb");
				// Если файл открыт удачно
				if(file != nullptr){
					// Если файл не может быть записан
					if(!(result = (::PEM_write_PUBKEY(file, key.ctx) == 1))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("Public key saving failed", __PRETTY_FUNCTION__, make_tuple(path), log_t::flag_t::CRITICAL);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("Public key saving failed", log_t::flag_t::CRITICAL);
						#endif
					}
					// Закрываем файл
					::fclose(file);
				// Если файл не открыт
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Public key file opening failed", __PRETTY_FUNCTION__, make_tuple(path), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Public key file opening failed", log_t::flag_t::WARNING);
					#endif
				}
			#endif
		// Если путь к файлу не передан или публичный ключ не загружен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Invalid path or public key is not loaded", __PRETTY_FUNCTION__, make_tuple(path), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Invalid path or public key is not loaded", log_t::flag_t::WARNING);
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(path), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод сохранения приватного ключа RSA в файл
 *
 * @param path   путь к файлу для сохранения приватного ключа
 * @param cipher тип шифрования приватного ключа
 * @return       результат сохранения ключа
 *
 */
bool awh::Crypto::savePrivateKeyRSA(string_view path, const cipher_t cipher) const noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Получаем ссылку на объект ключа RSA
		const key_rsa_t & key = (* this->_params.key);
		// Если путь к файлу передан правильно
		if(!path.empty() && (key.ctx != nullptr)){
			// Если ключ является приватным
			if(key.type == key_type_t::PRIVATE){
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Сохраняем приватный ключ
					FILE * file = ::_wfopen(this->_fmk->convert(path).c_str(), L"wb");
					// Если файл открыт удачно
					if(file != nullptr){
						// Если пароль защиты приватного ключа не установлен
						if(this->_params.passwordRSA.empty()){
							// Если файл не может быть записан
							if(!(result = (::PEM_write_PrivateKey(file, key.ctx, nullptr, nullptr, 0, nullptr, nullptr) == 1))){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("Private key saving failed", __PRETTY_FUNCTION__, make_tuple(path), log_t::flag_t::CRITICAL);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("Private key saving failed", log_t::flag_t::CRITICAL);
								#endif
							}
						// Если пароль установлен
						} else {
							// Тип шифрования AES
							const EVP_CIPHER * evp = ::EVP_enc_null();
							/**
							 * Определяем длину шифрования
							 */
							switch(static_cast <uint16_t> (cipher)){
								// Устанавливаем шифрование в 128
								case static_cast <uint16_t> (cipher_t::AES128):
									// Устанавливаем функцию шифрования
									evp = ::EVP_aes_128_cbc();
								break;
								// Устанавливаем шифрование в 192
								case static_cast <uint16_t> (cipher_t::AES192):
									// Устанавливаем функцию шифрования
									evp = ::EVP_aes_192_cbc();
								break;
								// Устанавливаем шифрование в 256
								case static_cast <uint16_t> (cipher_t::AES256):
								// Разрядность шифрования по умолчанию равна наибольшей
								case static_cast <uint16_t> (cipher_t::NONE):
									// Устанавливаем функцию шифрования
									evp = ::EVP_aes_256_cbc();
								break;
								/**
								 * Тип шифрования, защите ключа не подходящий, отвергается явно: прежде
								 * он молча подменялся наибольшей разрядностью, и работа думала, что ключ
								 * защищён тем шифром, который она назвала
								 */
								// Если тип шифрования разбору не знаком
								default:
									// Снимаем функцию шифрования
									evp = nullptr;
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug("Cipher type is not suitable for private key protection", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (cipher)), log_t::flag_t::CRITICAL);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("Cipher type is not suitable for private key protection", log_t::flag_t::CRITICAL);
									#endif
							}
							// Если файл не может быть записан
							if((evp == nullptr) || !(result = (::PEM_write_PKCS8PrivateKey(file, key.ctx, evp, this->_params.passwordRSA.c_str(), static_cast <int32_t> (this->_params.passwordRSA.size()), nullptr, nullptr) == 1))){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("Private key saving failed", __PRETTY_FUNCTION__, make_tuple(path), log_t::flag_t::CRITICAL);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("Private key saving failed", log_t::flag_t::CRITICAL);
								#endif
							}
						}
						// Закрываем файл
						::fclose(file);
					// Если файл не открыт
					} else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("Private key file opening failed", __PRETTY_FUNCTION__, make_tuple(path), log_t::flag_t::WARNING);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("Private key file opening failed", log_t::flag_t::WARNING);
						#endif
					}
				/**
				 * Для других операционных систем
				 */
				#else
					/**
					 * Путь снимается в свою строку: обозрение строки завершающего нуля
					 * не обещает, и подача его указателя открывала бы не тот файл либо
					 * уводила чтение за границу обозреваемого
					 */
					// Имя файла, завершающим нулём оканчивающееся
					const string filename(path);
					// Сохраняем приватный ключ
					FILE * file = ::fopen(filename.c_str(), "wb");
					// Если файл открыт удачно
					if(file != nullptr){
						// Если пароль защиты приватного ключа не установлен
						if(this->_params.passwordRSA.empty()){
							// Если файл не может быть записан
							if(!(result = (::PEM_write_PrivateKey(file, key.ctx, nullptr, nullptr, 0, nullptr, nullptr) == 1))){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("Private key saving failed", __PRETTY_FUNCTION__, make_tuple(path), log_t::flag_t::CRITICAL);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("Private key saving failed", log_t::flag_t::CRITICAL);
								#endif
							}
						// Если пароль установлен
						} else {
							// Тип шифрования AES
							const EVP_CIPHER * evp = ::EVP_enc_null();
							/**
							 * Определяем длину шифрования
							 */
							switch(static_cast <uint16_t> (cipher)){
								// Устанавливаем шифрование в 128
								case static_cast <uint16_t> (cipher_t::AES128):
									// Устанавливаем функцию шифрования
									evp = ::EVP_aes_128_cbc();
								break;
								// Устанавливаем шифрование в 192
								case static_cast <uint16_t> (cipher_t::AES192):
									// Устанавливаем функцию шифрования
									evp = ::EVP_aes_192_cbc();
								break;
								// Устанавливаем шифрование в 256
								case static_cast <uint16_t> (cipher_t::AES256):
								// Разрядность шифрования по умолчанию равна наибольшей
								case static_cast <uint16_t> (cipher_t::NONE):
									// Устанавливаем функцию шифрования
									evp = ::EVP_aes_256_cbc();
								break;
								/**
								 * Тип шифрования, защите ключа не подходящий, отвергается явно: прежде
								 * он молча подменялся наибольшей разрядностью, и работа думала, что ключ
								 * защищён тем шифром, который она назвала
								 */
								// Если тип шифрования разбору не знаком
								default:
									// Снимаем функцию шифрования
									evp = nullptr;
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug("Cipher type is not suitable for private key protection", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (cipher)), log_t::flag_t::CRITICAL);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("Cipher type is not suitable for private key protection", log_t::flag_t::CRITICAL);
									#endif
							}
							// Если файл не может быть записан
							if((evp == nullptr) || !(result = (::PEM_write_PKCS8PrivateKey(file, key.ctx, evp, this->_params.passwordRSA.c_str(), static_cast <int32_t> (this->_params.passwordRSA.size()), nullptr, nullptr) == 1))){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("Private key saving failed", __PRETTY_FUNCTION__, make_tuple(path), log_t::flag_t::CRITICAL);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("Private key saving failed", log_t::flag_t::CRITICAL);
								#endif
							}
						}
						// Закрываем файл
						::fclose(file);
					// Если файл не открыт
					} else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("Private key file opening failed", __PRETTY_FUNCTION__, make_tuple(path), log_t::flag_t::WARNING);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("Private key file opening failed", log_t::flag_t::WARNING);
						#endif
					}
				#endif
			// Если ключ не является приватным
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("Key cannot be saved because it is not private", __PRETTY_FUNCTION__, make_tuple(path), log_t::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Key cannot be saved because it is not private", log_t::flag_t::CRITICAL);
				#endif
			}
		// Если путь к файлу не передан или приватный ключ не сгенерирован
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Invalid path or private key is not generated", __PRETTY_FUNCTION__, make_tuple(path), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Invalid path or private key is not generated", log_t::flag_t::WARNING);
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(path), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод шифрования данных публичным ключом RSA
 *
 * @param buffer буфер данных для шифрования
 * @param result буфер куда следует положить результат
 *
 */
void awh::Crypto::encryptWithPublicKey(const vector <uint8_t> & buffer, vector <uint8_t> & result) const noexcept {
	// Вызываем метод шифрования данных публичным ключом RSA
	this->encryptWithPublicKey(buffer.data(), buffer.size(), result);
}
/**
 * @brief Метод шифрования данных публичным ключом RSA
 *
 * @param buffer буфер данных для шифрования
 * @param size   размер данных для шифрования
 * @param result буфер куда следует положить результат
 *
 */
void awh::Crypto::encryptWithPublicKey(const uint8_t * buffer, const size_t size, vector <uint8_t> & result) const noexcept {
	/**
	 * Буфер результата очищается на входе: отказ, случившийся до отведения
	 * буфера, оставлял в нём итог прежней работы - подпись прежних данных
	 * выглядела бы подписью нынешних
	 */
	// Очищаем буфер результата
	result.clear();
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если буфер данных и размер данных переданы правильно
		if((buffer != nullptr) && (size != 0)){
			// Получаем ссылку на объект ключа RSA
			const key_rsa_t & key = (* this->_params.key);
			// Если публичный ключ сгенерирован
			if(key.ctx != nullptr){
				// Создаём контекст для шифрования
				EVP_PKEY_CTX * ctx = ::EVP_PKEY_CTX_new(key.ctx, nullptr);
				// Если контекст для подписи не создан
				if(ctx == nullptr){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Context allocation failed", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Context allocation failed", log_t::flag_t::CRITICAL);
					#endif
				// Если контекст для подписи создан
				} else {
					// Если контекст для шифрования не инициализирован
					if((::EVP_PKEY_encrypt_init(ctx) <= 0) || (::EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) || (::EVP_PKEY_CTX_set_rsa_oaep_md(ctx, ::EVP_sha256()) <= 0) || (::EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, ::EVP_sha256()) <= 0)){
						// Освобождаем контекст для шифрования
						::EVP_PKEY_CTX_free(ctx);
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("Encrypt init failed", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("Encrypt init failed", log_t::flag_t::CRITICAL);
						#endif
						// Выходим из метода
						return;
					}
					// Размер подписи в байтах
					size_t length = 0;
					// Получаем размер зашифрованных данных
					if(::EVP_PKEY_encrypt(ctx, nullptr, &length, buffer, size) <= 0){
						// Освобождаем контекст для шифрования
						::EVP_PKEY_CTX_free(ctx);
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("Get encrypted data size failed", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("Get encrypted data size failed", log_t::flag_t::CRITICAL);
						#endif
						// Выходим из метода
						return;
					}
					// Очищаем объект результата
					result.clear();
					// Выделяем память под подпись
					result.resize(length, 0);
					// Получаем зашифрованные данные
					if(::EVP_PKEY_encrypt(ctx, result.data(), &length, buffer, size) <= 0){
						/**
						 * Результат очищается: буфер под него уже отведён и заполнен нулями,
						 * и работа, судящая об удаче по его непустоте, приняла бы нули за
						 * готовый результат
						 */
						// Очищаем буфер результата
						result.clear();
						// Освобождаем контекст для шифрования
						::EVP_PKEY_CTX_free(ctx);
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("Encrypt data failed", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("Encrypt data failed", log_t::flag_t::CRITICAL);
						#endif
						// Выходим из метода
						return;
					}
					// Изменяем размер буфера под фактический размер зашифрованных данных
					result.resize(length);
					// Освобождаем контекст для шифрования
					::EVP_PKEY_CTX_free(ctx);
				}
			// Если публичный ключ не сгенерирован или не загружен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("Public or private key is not generated", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Public or private key is not generated", log_t::flag_t::WARNING);
				#endif
			}
		// Если буфер данных или размер данных переданы неправильно
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Invalid buffer or size for encryption", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Invalid buffer or size for encryption", log_t::flag_t::WARNING);
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод дешифрования данных приватным ключом RSA
 *
 * @param buffer буфер данных для дешифрования
 * @param result буфер куда следует положить результат
 *
 */
void awh::Crypto::decryptWithPrivateKey(const vector <uint8_t> & buffer, vector <uint8_t> & result) const noexcept {
	// Вызываем метод дешифрования данных приватным ключом RSA
	this->decryptWithPrivateKey(buffer.data(), buffer.size(), result);
}
/**
 * @brief Метод дешифрования данных приватным ключом RSA
 *
 * @param buffer буфер данных для дешифрования
 * @param size   размер данных для дешифрования
 * @param result буфер куда следует положить результат
 *
 */
void awh::Crypto::decryptWithPrivateKey(const uint8_t * buffer, const size_t size, vector <uint8_t> & result) const noexcept {
	/**
	 * Буфер результата очищается на входе: отказ, случившийся до отведения
	 * буфера, оставлял в нём итог прежней работы - подпись прежних данных
	 * выглядела бы подписью нынешних
	 */
	// Очищаем буфер результата
	result.clear();
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если буфер данных и размер данных переданы правильно
		if((buffer != nullptr) && (size != 0)){
			// Получаем ссылку на объект ключа RSA
			const key_rsa_t & key = (* this->_params.key);
			// Если приватный ключ сгенерирован
			if(key.ctx != nullptr){
				// Если ключ является приватным
				if(key.type == key_type_t::PRIVATE){
					// Создаём контекст для шифрования
					EVP_PKEY_CTX * ctx = ::EVP_PKEY_CTX_new(key.ctx, nullptr);
					// Если контекст для подписи не создан
					if(ctx == nullptr){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("Context allocation failed", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("Context allocation failed", log_t::flag_t::CRITICAL);
						#endif
					// Если контекст для подписи создан
					} else {
						// Если контекст для дешифрования не инициализирован
						if((::EVP_PKEY_decrypt_init(ctx) <= 0) || (::EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) || (::EVP_PKEY_CTX_set_rsa_oaep_md(ctx, ::EVP_sha256()) <= 0) || (::EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, ::EVP_sha256()) <= 0)){
							// Освобождаем контекст для дешифрования
							::EVP_PKEY_CTX_free(ctx);
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Decrypt init failed", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Decrypt init failed", log_t::flag_t::CRITICAL);
							#endif
							// Выходим из метода
							return;
						}
						// Размер подписи в байтах
						size_t length = 0;
						// Получаем размер дешифрованных данных
						if(::EVP_PKEY_decrypt(ctx, nullptr, &length, buffer, size) <= 0){
							// Освобождаем контекст для дешифрования
							::EVP_PKEY_CTX_free(ctx);
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Get decrypted data size failed", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Get decrypted data size failed", log_t::flag_t::CRITICAL);
							#endif
							// Выходим из метода
							return;
						}
						// Очищаем объект результата
						result.clear();
						// Выделяем память под подпись
						result.resize(length, 0);
						// Получаем дешифрованные данные
						if(::EVP_PKEY_decrypt(ctx, result.data(), &length, buffer, size) <= 0){
							/**
							 * Результат очищается: буфер под него уже отведён и заполнен нулями,
							 * и работа, судящая об удаче по его непустоте, приняла бы нули за
							 * готовый результат
							 */
							// Очищаем буфер результата
							result.clear();
							// Освобождаем контекст для дешифрования
							::EVP_PKEY_CTX_free(ctx);
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Decrypt data failed", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Decrypt data failed", log_t::flag_t::CRITICAL);
							#endif
							// Выходим из метода
							return;
						}
						// Изменяем размер буфера под фактический размер дешифрованных данных
						result.resize(length);
						// Освобождаем контекст для дешифрования
						::EVP_PKEY_CTX_free(ctx);
					}
				// Если ключ не является приватным
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Unable to decrypt because the key is not private", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Unable to decrypt because the key is not private", log_t::flag_t::CRITICAL);
					#endif
				}
			// Если приватный ключ не сгенерирован или не загружен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("Private key is not generated", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Private key is not generated", log_t::flag_t::WARNING);
				#endif
			}
		// Если буфер данных или размер данных переданы неправильно
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Invalid buffer or size for decryption", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Invalid buffer or size for decryption", log_t::flag_t::WARNING);
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод подписания данных приватным ключом RSA
 *
 * @param buffer буфер данных для подписи
 * @param hash   тип хэш-суммы
 * @param result буфер куда следует положить результат
 *
 */
void awh::Crypto::signWithPrivateKey(const vector <uint8_t> & buffer, const hash_t hash, vector <uint8_t> & result) const noexcept {
	// Вызываем метод подписания данных приватным ключом RSA
	this->signWithPrivateKey(buffer.data(), buffer.size(), hash, result);
}
/**
 * @brief Метод подписания данных приватным ключом RSA
 *
 * @param buffer буфер данных для подписи
 * @param size   размер данных для подписи
 * @param hash   тип хэш-суммы
 * @param result буфер куда следует положить результат
 *
 */
void awh::Crypto::signWithPrivateKey(const uint8_t * buffer, const size_t size, const hash_t hash, vector <uint8_t> & result) const noexcept {
	/**
	 * Буфер результата очищается на входе: отказ, случившийся до отведения
	 * буфера, оставлял в нём итог прежней работы - подпись прежних данных
	 * выглядела бы подписью нынешних
	 */
	// Очищаем буфер результата
	result.clear();
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если буфер данных и размер данных переданы правильно
		if((buffer != nullptr) && (size != 0)){
			// Получаем ссылку на объект ключа RSA
			const key_rsa_t & key = (* this->_params.key);
			// Если приватный ключ сгенерирован
			if(key.ctx != nullptr){
				// Если ключ является приватным
				if(key.type == key_type_t::PRIVATE){
					// Создаём контекст для подписи
					EVP_MD_CTX * ctx = ::EVP_MD_CTX_new();
					// Если контекст для подписи не создан
					if(ctx == nullptr){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("Context allocation failed", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (hash)), log_t::flag_t::CRITICAL);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("Context allocation failed", log_t::flag_t::CRITICAL);
						#endif
					// Если контекст для подписи создан
					} else {
						// Выбираем хэш-функцию
						const EVP_MD * md = nullptr;
						/**
						 * Определяем тип хэш-суммы
						 */
						switch(static_cast <uint8_t> (hash)){
							// Если тип хэш-суммы указан как MD5
							case static_cast <uint8_t> (hash_t::MD5):
								// Устанавливаем функцию хэширования
								md = ::EVP_md5();
							break;
							// Если тип хэш-суммы указан как SHA1
							case static_cast <uint8_t> (hash_t::SHA1):
								// Устанавливаем функцию хэширования
								md = ::EVP_sha1();
							break;
							// Если тип хэш-суммы указан как SHA224
							case static_cast <uint8_t> (hash_t::SHA224):
								// Устанавливаем функцию хэширования
								md = ::EVP_sha224();
							break;
							// Если тип хэш-суммы указан как SHA256
							case static_cast <uint8_t> (hash_t::SHA256):
								// Устанавливаем функцию хэширования
								md = ::EVP_sha256();
							break;
							// Если тип хэш-суммы указан как SHA384
							case static_cast <uint8_t> (hash_t::SHA384):
								// Устанавливаем функцию хэширования
								md = ::EVP_sha384();
							break;
							// Если тип хэш-суммы указан как SHA512
							case static_cast <uint8_t> (hash_t::SHA512):
								// Устанавливаем функцию хэширования
								md = ::EVP_sha512();
							break;
							// Если ничего не выбрано
							default: {
								// Освобождаем контекст для подписи
								::EVP_MD_CTX_free(ctx);
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("Unsupported hash type", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (hash)), log_t::flag_t::CRITICAL);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("Unsupported hash type", log_t::flag_t::CRITICAL);
								#endif
								// Выходим из метода
								return;
							}
						}
						// Инициализируем подпись данных
						// Контекст ключа подписи для установки схемы дополнения
						EVP_PKEY_CTX * pctx = nullptr;
						if((::EVP_DigestSignInit(ctx, &pctx, md, nullptr, key.ctx) != 1) ||
						   !driver::padding(pctx, key.ctx, md, this->_params.padding, this->_log)){
							// Освобождаем контекст для подписи
							::EVP_MD_CTX_free(ctx);
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Digest signature init failed", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (hash)), log_t::flag_t::CRITICAL);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Digest signature init failed", log_t::flag_t::CRITICAL);
							#endif
							// Выходим из метода
							return;
						}
						// Обновляем подпись данными из буфера
						if(::EVP_DigestSignUpdate(ctx, buffer, size) != 1){
							// Освобождаем контекст для подписи
							::EVP_MD_CTX_free(ctx);
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Digest signature update failed", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (hash)), log_t::flag_t::CRITICAL);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Digest signature update failed", log_t::flag_t::CRITICAL);
							#endif
							// Выходим из метода
							return;
						}
						// Размер подписи в байтах
						size_t length = 0;
						// Получаем размер подписи
						if(::EVP_DigestSignFinal(ctx, nullptr, &length) != 1){
							// Освобождаем контекст для подписи
							::EVP_MD_CTX_free(ctx);
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Digest signature final (get length) failed", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (hash)), log_t::flag_t::CRITICAL);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Digest signature final (get length) failed", log_t::flag_t::CRITICAL);
							#endif
							// Выходим из метода
							return;
						}
						// Очищаем объект результата
						result.clear();
						// Выделяем память под подпись
						result.resize(length, 0);
						// Получаем подпись данных
						if(::EVP_DigestSignFinal(ctx, result.data(), &length) != 1){
							/**
							 * Результат очищается: буфер под него уже отведён и заполнен нулями,
							 * и работа, судящая об удаче по его непустоте, приняла бы нули за
							 * готовый результат
							 */
							// Очищаем буфер результата
							result.clear();
							// Освобождаем контекст для подписи
							::EVP_MD_CTX_free(ctx);
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Digest signature final (get signature) failed", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (hash)), log_t::flag_t::CRITICAL);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Digest signature final (get signature) failed", log_t::flag_t::CRITICAL);
							#endif
							// Выходим из метода
							return;
						}
						// Изменяем размер буфера под фактический размер подписи
						result.resize(length);
						// Освобождаем контекст для подписи
						::EVP_MD_CTX_free(ctx);
					}
				// Если ключ не является приватным
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Unable to sign because the key is not private", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (hash)), log_t::flag_t::CRITICAL);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Unable to sign because the key is not private", log_t::flag_t::CRITICAL);
					#endif
				}
			// Если приватный ключ не сгенерирован или не загружен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("Private key is not generated", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (hash)), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Private key is not generated", log_t::flag_t::WARNING);
				#endif
			}
		// Если буфер данных или размер данных переданы неправильно
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Invalid buffer or size for signing", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (hash)), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Invalid buffer or size for signing", log_t::flag_t::WARNING);
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (hash)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод верификации данных публичным ключом RSA
 *
 * @param buffer    буфер данных для верификации
 * @param signature буфер с подписью данных
 * @param hash      тип хэш-суммы
 * @return          результат верификации
 *
 */
bool awh::Crypto::verifyWithPublicKey(const vector <uint8_t> & buffer, const vector <uint8_t> & signature, const hash_t hash) const noexcept {
	// Вызываем метод верификации данных публичным ключом RSA
	return this->verifyWithPublicKey(buffer.data(), buffer.size(), signature, hash);
}
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
bool awh::Crypto::verifyWithPublicKey(const uint8_t * buffer, const size_t size, const vector <uint8_t> & signature, const hash_t hash) const noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если буфер данных и размер данных переданы правильно
		if((buffer != nullptr) && (size != 0) && !signature.empty()){
			// Получаем ссылку на объект ключа RSA
			const key_rsa_t & key = (* this->_params.key);
			// Если публичный ключ сгенерирован
			if(key.ctx != nullptr){
				// Создаём контекст для верификации
				EVP_MD_CTX * ctx = ::EVP_MD_CTX_new();
				// Если контекст для подписи не создан
				if(ctx == nullptr){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Context allocation failed", __PRETTY_FUNCTION__, make_tuple(buffer, size, signature.size(), static_cast <uint16_t> (hash)), log_t::flag_t::CRITICAL);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Context allocation failed", log_t::flag_t::CRITICAL);
					#endif
				// Если контекст для подписи создан
				} else {
					// Выбираем хэш-функцию
					const EVP_MD * md = nullptr;
					/**
					 * Определяем тип хэш-суммы
					 */
					switch(static_cast <uint8_t> (hash)){
						// Если тип хэш-суммы указан как MD5
						case static_cast <uint8_t> (hash_t::MD5):
							// Устанавливаем функцию хэширования
							md = ::EVP_md5();
						break;
						// Если тип хэш-суммы указан как SHA1
						case static_cast <uint8_t> (hash_t::SHA1):
							// Устанавливаем функцию хэширования
							md = ::EVP_sha1();
						break;
						// Если тип хэш-суммы указан как SHA224
						case static_cast <uint8_t> (hash_t::SHA224):
							// Устанавливаем функцию хэширования
							md = ::EVP_sha224();
						break;
						// Если тип хэш-суммы указан как SHA256
						case static_cast <uint8_t> (hash_t::SHA256):
							// Устанавливаем функцию хэширования
							md = ::EVP_sha256();
						break;
						// Если тип хэш-суммы указан как SHA384
						case static_cast <uint8_t> (hash_t::SHA384):
							// Устанавливаем функцию хэширования
							md = ::EVP_sha384();
						break;
						// Если тип хэш-суммы указан как SHA512
						case static_cast <uint8_t> (hash_t::SHA512):
							// Устанавливаем функцию хэширования
							md = ::EVP_sha512();
						break;
						// Если ничего не выбрано
						default: {
							// Освобождаем контекст для верификации
							::EVP_MD_CTX_free(ctx);
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Unsupported hash type", __PRETTY_FUNCTION__, make_tuple(buffer, size, signature.size(), static_cast <uint16_t> (hash)), log_t::flag_t::CRITICAL);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Unsupported hash type", log_t::flag_t::CRITICAL);
							#endif
							// Выходим из метода
							return result;
						}
					}
					// Инициализируем верификацию данных
					// Контекст ключа подписи для установки схемы дополнения
					EVP_PKEY_CTX * pctx = nullptr;
					if((::EVP_DigestVerifyInit(ctx, &pctx, md, nullptr, key.ctx) == 1) &&
					   driver::padding(pctx, key.ctx, md, this->_params.padding, this->_log) &&
					   (::EVP_DigestVerifyUpdate(ctx, buffer, size) == 1))
						// Выполняем верификацию данных
						result = (::EVP_DigestVerifyFinal(ctx, signature.data(), signature.size()) == 1);
					// Освобождаем контекст для верификации
					::EVP_MD_CTX_free(ctx);
				}
			// Если публичный ключ не сгенерирован или не загружен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("Public key is not generated", __PRETTY_FUNCTION__, make_tuple(buffer, size, signature.size(), static_cast <uint16_t> (hash)), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Public key is not generated", log_t::flag_t::WARNING);
				#endif
			}
		// Если буфер данных или размер данных переданы неправильно
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Invalid buffer or size for verification signature", __PRETTY_FUNCTION__, make_tuple(buffer, size, signature.size(), static_cast <uint16_t> (hash)), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Invalid buffer or size for verification signature", log_t::flag_t::WARNING);
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size, signature.size(), static_cast <uint16_t> (hash)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::Crypto::Crypto(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {
	// Выделяем память под стейт шифрования
	this->_params.state = new state_t();
	// Выделяем память под ключевые данные
	this->_params.key = new key_rsa_t();
}
/**
 * @brief Деструктор
 *
 */
awh::Crypto::~Crypto() noexcept {
	// Получаем ссылку на объект ключа RSA
	key_rsa_t & key = (* this->_params.key);
	// Если ключ уже установлен
	if(key.ctx != nullptr)
		// Освобождаем память выделенную под ключ
		::EVP_PKEY_free(key.ctx);
	// Если пароль шифрования установлен
	if(!this->_params.password.empty())
		// Выполняем затирание пароля шифрования
		::OPENSSL_cleanse(&this->_params.password.front(), this->_params.password.size());
	// Если пароль защиты приватного ключа RSA установлен
	if(!this->_params.passwordRSA.empty())
		// Выполняем затирание пароля защиты приватного ключа RSA
		::OPENSSL_cleanse(&this->_params.passwordRSA.front(), this->_params.passwordRSA.size());
	/**
	 * Соль затирается наравне с паролями: сама по себе она тайной не является,
	 * но вместе с паролем она составляет всё, из чего выводится ключ
	 */
	// Если соль вывода ключа установлена
	if(!this->_params.salt.empty())
		// Выполняем затирание соли вывода ключа
		::OPENSSL_cleanse(&this->_params.salt.front(), this->_params.salt.size());
	// Освобождаем память контекста ключа RSA
	delete this->_params.key;
	/**
	 * Стейт AES-шифрования освобождает свой контекст и затирает ключ
	 * собственным деструктором, поэтому делать это здесь не требуется
	 */
	// Освобождаем память контекста стейта AES-шифрования
	delete this->_params.state;
}
