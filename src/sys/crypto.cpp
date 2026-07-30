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
		// Ключ шифрования
		vector <uint8_t> key;
		// Вектор инициализации
		vector <uint8_t> ivec;
		// Контекст выбранного шифра
		const EVP_CIPHER * evp;
		// Контекст шифрования
		const EVP_CIPHER_CTX * ctx;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit state_t() noexcept :
		 hash(crypto_t::hash_t::NONE),
		 cipher(crypto_t::cipher_t::NONE),
		 evp(nullptr), ctx(nullptr) {}
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
						// Выполняем получение подписи
						::HMAC(::EVP_md5(), key.data(), key.size(), reinterpret_cast <const uint8_t *> (buffer.data()), buffer.size(), digest, nullptr);
						// Формируем данные MD5-хэша
						driver::hex(digest, 16, result);
					} break;
					// Если тип хэш-суммы указан как HMAC_SHA1
					case static_cast <uint8_t> (crypto_t::hash_t::SHA1): {
						// Выделяем память для буфера данных
						result.resize(40, 0);
						// Буфер для бинарного результата подписи
						uint8_t digest[EVP_MAX_MD_SIZE] = {0};
						// Выполняем получение подписи
						::HMAC(::EVP_sha1(), key.data(), key.size(), reinterpret_cast <const uint8_t *> (buffer.data()), buffer.size(), digest, nullptr);
						// Формируем данные SHA1-хэша
						driver::hex(digest, 20, result);
					} break;
					// Если тип хэш-суммы указан как HMAC_SHA224
					case static_cast <uint8_t> (crypto_t::hash_t::SHA224): {
						// Выделяем память для буфера данных
						result.resize(56, 0);
						// Буфер для бинарного результата подписи
						uint8_t digest[EVP_MAX_MD_SIZE] = {0};
						// Выполняем получение подписи
						::HMAC(::EVP_sha224(), key.data(), key.size(), reinterpret_cast <const uint8_t *> (buffer.data()), buffer.size(), digest, nullptr);
						// Формируем данные SHA224-хэша
						driver::hex(digest, 28, result);
					} break;
					// Если тип хэш-суммы указан как HMAC_SHA256
					case static_cast <uint8_t> (crypto_t::hash_t::SHA256): {
						// Выделяем память для буфера данных
						result.resize(64, 0);
						// Буфер для бинарного результата подписи
						uint8_t digest[EVP_MAX_MD_SIZE] = {0};
						// Выполняем получение подписи
						::HMAC(::EVP_sha256(), key.data(), key.size(), reinterpret_cast <const uint8_t *> (buffer.data()), buffer.size(), digest, nullptr);
						// Формируем данные SHA256-хэша
						driver::hex(digest, 32, result);
					} break;
					// Если тип хэш-суммы указан как HMAC_SHA384
					case static_cast <uint8_t> (crypto_t::hash_t::SHA384): {
						// Выделяем память для буфера данных
						result.resize(96, 0);
						// Буфер для бинарного результата подписи
						uint8_t digest[EVP_MAX_MD_SIZE] = {0};
						// Выполняем получение подписи
						::HMAC(::EVP_sha384(), key.data(), key.size(), reinterpret_cast <const uint8_t *> (buffer.data()), buffer.size(), digest, nullptr);
						// Формируем данные SHA384-хэша
						driver::hex(digest, 48, result);
					} break;
					// Если тип хэш-суммы указан как HMAC_SHA512
					case static_cast <uint8_t> (crypto_t::hash_t::SHA512): {
						// Выделяем память для буфера данных
						result.resize(128, 0);
						// Буфер для бинарного результата подписи
						uint8_t digest[EVP_MAX_MD_SIZE] = {0};
						// Выполняем получение подписи
						::HMAC(::EVP_sha512(), key.data(), key.size(), reinterpret_cast <const uint8_t *> (buffer.data()), buffer.size(), digest, nullptr);
						// Формируем данные SHA512-хэша
						driver::hex(digest, 64, result);
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
	 */
	static void hash(const char * buffer, const size_t size, const crypto_t::cipher_t cipher, const crypto_t::event_t event, state_t & state, T & result, const log_t * log) noexcept {
		// Если буфер данных передан
		if((buffer != nullptr) && (size > 0)){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Выполняем очистку блока с результатом
				result.clear();
				/**
				 * Определяем тип шифрования
				 */
				switch(static_cast <uint8_t> (cipher)){
					// Если производится работы с BASE64
					case static_cast <uint8_t> (crypto_t::cipher_t::BASE64): {
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
								if(length > 0)
									// Удаляем все лишние символы
									result.erase(result.begin() + length, result.end());
								// Если получение хэша не удалось
								else {
									// Выполняем сброс результата
									result.clear();
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										log->debug("Error during data decoding", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (cipher), static_cast <uint16_t> (event)), log_t::flag_t::WARNING);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										log->print("Error during data decoding", log_t::flag_t::WARNING);
									#endif
								}
							}
							/**
							 * Очищаем всю цепочку BIO одним вызовом от головы:
							 * после BIO_push(b64, bio) объект b64 владеет ссылкой на bio и освобождает его в своём деструкторе.
							 * Раздельное освобождение bio и b64 приводит к двойному освобождению bio.
							 */
							::BIO_free_all(b64);
						}
					} break;
					// Если производится работы с AES128
					case static_cast <uint8_t> (crypto_t::cipher_t::AES128):
					// Если производится работы с AES192
					case static_cast <uint8_t> (crypto_t::cipher_t::AES192):
					// Если производится работы с AES256
					case static_cast <uint8_t> (crypto_t::cipher_t::AES256): {
						// Если контекст шифрования не создан
						if(state.ctx == nullptr){
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
								// Выходим из функции
								return;
							}
							/**
							 * Определяем событие кодирование или декодирование
							 */
							switch(static_cast <uint8_t> (event)){
								// Если производится кодирование данных
								case static_cast <uint8_t> (crypto_t::event_t::ENCODE):
									// Выполняем инициализацию контекста шифрования
									if(::EVP_CipherInit_ex(ctx, state.evp, nullptr, state.key.data(), state.ivec.data(), AES_ENCRYPT) != 1){
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
										// Выходим из функции
										return;
									}
								break;
								// Если производится декодирование данных
								case static_cast <uint8_t> (crypto_t::event_t::DECODE):
									// Выполняем инициализацию контекста шифрования
									if(::EVP_CipherInit_ex(ctx, state.evp, nullptr, state.key.data(), state.ivec.data(), AES_DECRYPT) != 1){
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
										// Выходим из функции
										return;
									}
								break;
							}
							// Отключаем padding (обязательно для CFB)
							::EVP_CIPHER_CTX_set_padding(ctx, 0);
							// Очищаем выходной буфер
							result.clear();
							// Буфер для выходных данных (на 1 блок больше — стандартная практика)
							result.resize(AES_BLOCK_SIZE + size, 0);
							// Обрабатываем весь входной буфер за один вызов (рекомендуется)
							int32_t length = 0;
							// Выполняем обновление шифрования
							if(::EVP_CipherUpdate(ctx, reinterpret_cast <uint8_t *> (result.data()), &length, reinterpret_cast <const uint8_t *> (buffer), static_cast <int32_t> (size)) != 1){
								// Очищаем выходной буфер
								result.clear();
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
								// Выходим из функции
								return;
							}
							// Размер обработанных данных
							int32_t offset = 0;
							// Финализация (в CFB обычно ничего не добавляет, но нужна)
							if(::EVP_CipherFinal_ex(ctx, reinterpret_cast <uint8_t *> (result.data() + length), &offset) != 1){
								// Очищаем выходной буфер
								result.clear();
								// Освобождаем контекст шифрования
								::EVP_CIPHER_CTX_free(ctx);
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log->debug("Error cipher final", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (cipher), static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									log->print("Error cipher final", log_t::flag_t::CRITICAL);
								#endif
								// Выходим из функции
								return;
							}
							// Изменяем размер результата на фактический размер данных
							result.resize(static_cast <size_t> (length + offset));
							// Освобождаем контекст
							::EVP_CIPHER_CTX_free(ctx);
						// Если контекст шифрования создан
						} else {
							// Очищаем выходной буфер
							result.clear();
							// Буфер для выходных данных (на 1 блок больше — стандартная практика)
							result.resize(AES_BLOCK_SIZE + size, 0);
							// Обрабатываем весь входной буфер за один вызов (рекомендуется)
							int32_t length = 0;
							// Выполняем обновление шифрования
							if(::EVP_CipherUpdate(const_cast <EVP_CIPHER_CTX *> (state.ctx), reinterpret_cast <uint8_t *> (result.data()), &length, reinterpret_cast <const uint8_t *> (buffer), static_cast <int32_t> (size)) != 1){
								// Очищаем выходной буфер
								result.clear();
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
								// Выходим из функции
								return;
							}
							// Изменяем размер результата на фактический размер данных
							result.resize(static_cast <size_t> (length));
						}
					} break;
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
			}
		}
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
	static bool cipher(const crypto_t::cipher_t cipher, const crypto_t::hash_t hash, const string & pass, const string & salt, const uint32_t rounds, state_t & state, const log_t * log) noexcept {
		// Переменная результата
		bool result = false;
		// Если пароль для шифрования не пустой
		if(!pass.empty()){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Обнуляем массив ключа
				state.key.clear();
				// Обнуляем массив IVEC
				state.ivec.clear();
				// Устанавливаем размер массива IVEC
				state.ivec.resize(AES_BLOCK_SIZE, 0);
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
						evp = ::EVP_aes_128_cfb128();
					} break;
					// Устанавливаем шифрование в 192
					case static_cast <uint16_t> (crypto_t::cipher_t::AES192): {
						// Устанавливаем размер массива KEY
						state.key.resize(24, 0);
						// Устанавливаем функцию шифрования
						evp = ::EVP_aes_192_cfb128();
					} break;
					// Устанавливаем шифрование в 256
					case static_cast <uint16_t> (crypto_t::cipher_t::AES256): {
						// Устанавливаем размер массива KEY
						state.key.resize(32, 0);
						// Устанавливаем функцию шифрования
						evp = ::EVP_aes_256_cfb128();
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
				// Временный буфер
				uint8_t buffer[32 + AES_BLOCK_SIZE] = {0};
				// Генерация ключа и IV через PBKDF2
				result = (::PKCS5_PBKDF2_HMAC(
					pass.c_str(),
					static_cast <int32_t> (pass.length()),
					(salt.empty() ? nullptr : reinterpret_cast <const uint8_t *> (salt.c_str())),
					static_cast <int32_t> (salt.length()),
					static_cast <int32_t> (rounds), md, sizeof(buffer), buffer
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
					// Копируем данные ключа в наш стейт
					std::copy(buffer, buffer + state.key.size(), state.key.begin());
					// Копируем данные IVEC в наш стейт
					std::copy(buffer + state.key.size(), buffer + (state.key.size() + state.ivec.size()), state.ivec.begin());
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
 salt{""}, password{""},
 state(nullptr), key(nullptr) {}

/**
 * @brief Метод установки безопасности работы потоков
 *
 * @param mode флаг режима безопасности потоков
 *
 */
void awh::Crypto::threadSafety(const bool mode) noexcept {
	// Устанавливаем режим безопасности потоков
	this->_mtx.enabled = mode;
}
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
		// Выполняем блокировку потоков
		const locker_t <> lock(this->_mtx);
		// Устанавливаем количество раундов шифрования
		this->_params.rounds = round;
		// Сбрасываем стейт шифрования
		(* this->_params.state) = state_t();
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
		// Выполняем блокировку потоков
		const locker_t <> lock(this->_mtx);
		// Устанавливаем соль для шифрования
		this->_params.salt = salt;
		// Сбрасываем стейт шифрования
		(* this->_params.state) = state_t();
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
		// Выполняем блокировку потоков
		const locker_t <> lock(this->_mtx);
		// Устанавливаем пароль шифрования
		this->_params.password = password;
		// Сбрасываем стейт шифрования
		* this->_params.state = state_t();
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
			// Если результат не пустой
			if(!buffer.empty()){
				// Размер обработанных данных
				int32_t offset = 0;
				// Получаем текущий размер результата
				const size_t length = buffer.size();
				// Расширяем буфер под результат
				buffer.resize(length + AES_BLOCK_SIZE, 0);
				// Финализация (в CFB обычно ничего не добавляет, но нужна)
				if(!(result = (::EVP_CipherFinal_ex(const_cast <EVP_CIPHER_CTX *> (state.ctx), reinterpret_cast <uint8_t *> (buffer.data() + length), &offset) == 1))){
					// Очищаем выходной буфер
					buffer.clear();
					// Освобождаем контекст шифрования
					::EVP_CIPHER_CTX_free(const_cast <EVP_CIPHER_CTX *> (state.ctx));
					// Зануляем контекст шифрования (исключаем повторное освобождение)
					state.ctx = nullptr;
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Error cipher final", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Error cipher final", log_t::flag_t::CRITICAL);
					#endif
					// Выходим из метода
					return result;
				}
				// Изменяем размер результата на фактический размер данных
				buffer.resize(static_cast <size_t> (length + offset));
			}
			// Освобождаем контекст шифрования
			::EVP_CIPHER_CTX_free(const_cast <EVP_CIPHER_CTX *> (state.ctx));
			// Зануляем контекст шифрования
			state.ctx = nullptr;
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
			// Если контекст шифрования не создан
			if(state.ctx == nullptr){
				// Выполняем блокировку потоков
				const locker_t <> lock(this->_mtx);
				// Если инициализация ключей не выполнена
				if(!driver::cipher(cipher, hash, this->_params.password, this->_params.salt, this->_params.rounds, state, this->_log)){
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
					case static_cast <uint8_t> (event_t::ENCODE):
						// Выполняем инициализацию контекста шифрования
						if(!(result = (::EVP_CipherInit_ex(const_cast <EVP_CIPHER_CTX *> (state.ctx), state.evp, nullptr, state.key.data(), state.ivec.data(), AES_ENCRYPT) == 1))){
							// Освобождаем контекст шифрования
							::EVP_CIPHER_CTX_free(const_cast <EVP_CIPHER_CTX *> (state.ctx));
							// Зануляем контекст шифрования
							state.ctx = nullptr;
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
					break;
					// Если производится декодирование данных
					case static_cast <uint8_t> (event_t::DECODE):
						// Выполняем инициализацию контекста шифрования
						if(!(result = (::EVP_CipherInit_ex(const_cast <EVP_CIPHER_CTX *> (state.ctx), state.evp, nullptr, state.key.data(), state.ivec.data(), AES_DECRYPT) == 1))){
							// Освобождаем контекст шифрования
							::EVP_CIPHER_CTX_free(const_cast <EVP_CIPHER_CTX *> (state.ctx));
							// Зануляем контекст шифрования
							state.ctx = nullptr;
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
					break;
				}
				// Отключаем padding (обязательно для CFB)
				::EVP_CIPHER_CTX_set_padding(const_cast <EVP_CIPHER_CTX *> (state.ctx), 0);
			}
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
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Определяем тип шифрования
		 */
		switch(static_cast <uint8_t> (cipher)){
			// Если производится работы с BASE64
			case static_cast <uint8_t> (cipher_t::BASE64): {
				// Выполняем блокировку потоков
				const locker_t <> lock(this->_mtx);
				// Получаем состояние объекта
				const state_t & state = (* this->_params.state);
				// Выполняем кодирование строки BASE64
				driver::hash(reinterpret_cast <const char *> (buffer), size, cipher, event_t::ENCODE, const_cast <state_t &> (state), result, this->_log);
				// Если кодирование не вышло
				if(result.empty()){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Unable to encrypt \"%s\" string data into BASE64 format", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (hash), static_cast <uint16_t> (cipher)), log_t::flag_t::WARNING, string(reinterpret_cast <const char *> (buffer), size).c_str());
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
			case static_cast <uint8_t> (cipher_t::AES128):
			// Если производится работы с AES192
			case static_cast <uint8_t> (cipher_t::AES192):
			// Если производится работы с AES256
			case static_cast <uint8_t> (cipher_t::AES256): {
				// Если пароль установлен
				if(!this->_params.password.empty()){
					// Выполняем блокировку потоков на всю критическую секцию (исключаем гонку проверки и инициализации)
					const locker_t <> lock(this->_mtx);
					// Получаем состояние объекта
					const state_t & state = (* this->_params.state);
					// Если контекст шифрования не создан
					if(state.ctx == nullptr){
						// Проверяем текущее состояние
						if((state.hash != hash) || (state.cipher != cipher)){
							// Если инициализация ключей не выполнена
							if(!driver::cipher(cipher, hash, this->_params.password, this->_params.salt, this->_params.rounds, const_cast <state_t &> (state), this->_log)){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("Unable to initialize AES cipher for encoding data", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (hash), static_cast <uint16_t> (cipher)), log_t::flag_t::CRITICAL);
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
						driver::hash(reinterpret_cast <const char *> (buffer), size, cipher, event_t::ENCODE, const_cast <state_t &> (state), result, this->_log);
					// Если контекст шифрования уже создан
					} else
						// Выполняем шифрование данных
						driver::hash(reinterpret_cast <const char *> (buffer), size, state.cipher, event_t::ENCODE, const_cast <state_t &> (state), result, this->_log);
				}
				// Если кодирование не вышло
				if(result.empty()){
					// Не возвращаем открытый текст: при неудаче шифрования результат остаётся пустым
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Unable to encrypt data into AES", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (hash), static_cast <uint16_t> (cipher)), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Unable to encrypt data into AES", log_t::flag_t::WARNING);
					#endif
				}
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
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Определяем тип шифрования
		 */
		switch(static_cast <uint8_t> (cipher)){
			// Если производится работы с BASE64
			case static_cast <uint8_t> (cipher_t::BASE64): {
				// Выполняем блокировку потоков
				const locker_t <> lock(this->_mtx);
				// Получаем состояние объекта
				const state_t & state = (* this->_params.state);
				// Выполняем декодирование строки BASE64
				driver::hash(reinterpret_cast <const char *> (buffer), size, cipher, event_t::DECODE, const_cast <state_t &> (state), result, this->_log);
				// Если декодирование не вышло
				if(result.empty()){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Unable to extract data from BASE64 encoded \"%s\" hash", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (hash), static_cast <uint16_t> (cipher)), log_t::flag_t::WARNING, string(reinterpret_cast <const char *> (buffer), size).c_str());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Unable to extract data from BASE64 encoded \"%s\" hash", log_t::flag_t::WARNING, string(reinterpret_cast <const char *> (buffer), size).c_str());
					#endif
				}
			} break;
			// Если производится работы с AES128
			case static_cast <uint8_t> (cipher_t::AES128):
			// Если производится работы с AES192
			case static_cast <uint8_t> (cipher_t::AES192):
			// Если производится работы с AES256
			case static_cast <uint8_t> (cipher_t::AES256): {
				// Если пароль установлен
				if(!this->_params.password.empty()){
					// Выполняем блокировку потоков на всю критическую секцию (исключаем гонку проверки и инициализации)
					const locker_t <> lock(this->_mtx);
					// Получаем состояние объекта
					const state_t & state = (* this->_params.state);
					// Если контекст шифрования не создан
					if(state.ctx == nullptr){
						// Проверяем текущее состояние
						if((state.hash != hash) || (state.cipher != cipher)){
							// Если инициализация ключей не выполнена
							if(!driver::cipher(cipher, hash, this->_params.password, this->_params.salt, this->_params.rounds, const_cast <state_t &> (state), this->_log)){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("Unable to initialize AES cipher for encoding data", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (hash), static_cast <uint16_t> (cipher)), log_t::flag_t::CRITICAL);
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
						driver::hash(reinterpret_cast <const char *> (buffer), size, cipher, event_t::DECODE, const_cast <state_t &> (state), result, this->_log);
					// Если контекст шифрования уже создан
					} else
						// Выполняем дешифрование данных
						driver::hash(reinterpret_cast <const char *> (buffer), size, state.cipher, event_t::DECODE, const_cast <state_t &> (state), result, this->_log);
				}
				// Если дешифрование не вышло
				if(result.empty()){
					// Не возвращаем зашифрованные данные: при неудаче дешифрования результат остаётся пустым
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Unable to decrypt data from AES", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (hash), static_cast <uint16_t> (cipher)), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Unable to decrypt data from AES", log_t::flag_t::WARNING);
					#endif
				}
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
 * @param size размер ключа в битах (2048, 3072, 4096)
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
		// Выполняем генерацию ключа RSA
		EVP_PKEY * privKey = ::EVP_RSA_gen(size == 0 ? 2048 : size);
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
		// Выполняем блокировку потоков
		const locker_t <> lock(this->_mtx);
		// Получаем ссылку на объект ключа RSA
		key_rsa_t & key = (* this->_params.key);
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
		// Если путь к файлу передан правильно
		if(key.ctx != nullptr){
			// Выполняем блокировку потоков
			const locker_t <> lock(this->_mtx);
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
					// Выполняем блокировку потоков
					const locker_t <> lock(this->_mtx);
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
				EVP_PKEY * pkey = ::PEM_read_bio_PrivateKey(bio, nullptr, nullptr, this->_params.password.empty() ? nullptr : reinterpret_cast <void *> (this->_params.password.data()));
				// Освобождаем объект BIO
				::BIO_free(bio);
				// Если приватный ключ получен
				if(pkey != nullptr){
					// Выполняем блокировку потоков
					const locker_t <> lock(this->_mtx);
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
		// Если путь к файлу передан правильно
		if(key.ctx != nullptr){
			// Если ключ является приватным
			if(key.type == key_type_t::PRIVATE){
				// Выполняем блокировку потоков
				const locker_t <> lock(this->_mtx);
				// Создаём объект BIO для записи приватного ключа
				BIO * bio = ::BIO_new(::BIO_s_mem());
				// Если объект BIO создан успешно
				if(bio != nullptr){
					// Если пароль не установлен
					if(this->_params.password.empty()){
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
							// Если ничего не выбрано
							default:
								// Устанавливаем функцию шифрования
								evp = ::EVP_aes_256_cbc();
						}
						// Если файл не может быть записан
						if(::PEM_write_bio_PKCS8PrivateKey(bio, key.ctx, evp, this->_params.password.c_str(), static_cast <int32_t> (this->_params.password.size()), nullptr, nullptr) != 1){
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
			// Если путь к файлу не передан или публичный ключ не загружен
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
						// Выполняем блокировку потоков
						const locker_t <> lock(this->_mtx);
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
				FILE * file = ::fopen(path.data(), "rb");
				// Если файл открыт удачно
				if(file != nullptr){
					// Читаем публичный ключ из файла
					EVP_PKEY * pkey = ::PEM_read_PUBKEY(file, nullptr, nullptr, nullptr);
					// Закрываем файл
					::fclose(file);
					// Если публичный ключ получен
					if(pkey != nullptr){
						// Выполняем блокировку потоков
						const locker_t <> lock(this->_mtx);
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
					EVP_PKEY * pkey = ::PEM_read_PrivateKey(file, nullptr, nullptr, nullptr);
					// Закрываем файл
					::fclose(file);
					// Если приватный ключ получен
					if(pkey != nullptr){
						// Выполняем блокировку потоков
						const locker_t <> lock(this->_mtx);
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
				// Открываем файл с приватным ключом
				FILE * file = ::fopen(path.data(), "rb");
				// Если файл открыт удачно
				if(file != nullptr){
					// Читаем приватный ключ из файла
					EVP_PKEY * pkey = ::PEM_read_PrivateKey(file, nullptr, nullptr, this->_params.password.empty() ? nullptr : reinterpret_cast <void *> (this->_params.password.data()));
					// Закрываем файл
					::fclose(file);
					// Если приватный ключ получен
					if(pkey != nullptr){
						// Выполняем блокировку потоков
						const locker_t <> lock(this->_mtx);
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
					// Выполняем блокировку потоков
					const locker_t <> lock(this->_mtx);
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
				FILE * file = ::fopen(path.data(), "wb");
				// Если файл открыт удачно
				if(file != nullptr){
					// Выполняем блокировку потоков
					const locker_t <> lock(this->_mtx);
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
						// Выполняем блокировку потоков
						const locker_t <> lock(this->_mtx);
						// Если пароль не установлен
						if(this->_params.password.empty()){
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
								// Если ничего не выбрано
								default:
									// Устанавливаем функцию шифрования
									evp = ::EVP_aes_256_cbc();
							}
							// Если файл не может быть записан
							if(!(result = (::PEM_write_PKCS8PrivateKey(file, key.ctx, evp, this->_params.password.c_str(), static_cast <int32_t> (this->_params.password.size()), nullptr, nullptr) == 1))){
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
					// Сохраняем приватный ключ
					FILE * file = ::fopen(path.data(), "wb");
					// Если файл открыт удачно
					if(file != nullptr){
						// Выполняем блокировку потоков
						const locker_t <> lock(this->_mtx);
						// Если пароль не установлен
						if(this->_params.password.empty()){
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
								// Если ничего не выбрано
								default:
									// Устанавливаем функцию шифрования
									evp = ::EVP_aes_256_cbc();
							}
							// Если файл не может быть записан
							if(!(result = (::PEM_write_PKCS8PrivateKey(file, key.ctx, evp, this->_params.password.c_str(), static_cast <int32_t> (this->_params.password.size()), nullptr, nullptr) == 1))){
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
	 * Выполняем перехват ошибок
	 */
	try {
		// Если буфер данных и размер данных переданы правильно
		if((buffer != nullptr) && (size != 0)){
			// Получаем ссылку на объект ключа RSA
			const key_rsa_t & key = (* this->_params.key);
			// Если публичный ключ сгенерирован
			if(key.ctx != nullptr){
				// Выполняем блокировку потоков
				const locker_t <> lock(this->_mtx);
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
					// Выполняем блокировку потоков
					const locker_t <> lock(this->_mtx);
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
						// Выполняем блокировку потоков
						const locker_t <> lock(this->_mtx);
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
						if(::EVP_DigestSignInit(ctx, nullptr, md, nullptr, key.ctx) != 1){
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
					// Выполняем блокировку потоков
					const locker_t <> lock(this->_mtx);
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
					if((::EVP_DigestVerifyInit(ctx, nullptr, md, nullptr, key.ctx) == 1) &&
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
	// Деактивируем мьютекс на время инициализации
	this->_mtx.enabled = false;
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
	// Получаем ссылку на стейт AES-шифрования
	state_t & state = (* this->_params.state);
	// Если контекст потокового AES-шифрования не был финализирован
	if(state.ctx != nullptr){
		// Освобождаем контекст шифрования
		::EVP_CIPHER_CTX_free(const_cast <EVP_CIPHER_CTX *> (state.ctx));
		// Зануляем контекст шифрования
		state.ctx = nullptr;
	}
	// Освобождаем память контекста ключа RSA
	delete this->_params.key;
	// Освобождаем память контекста стейта AES-шифрования
	delete this->_params.state;
}
