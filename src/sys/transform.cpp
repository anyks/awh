/**
 * @file: transform.cpp
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
 * Подключаем Snappy
 */
#include <snappy.h>

/**
 * Подключаем Density
 */
#include <density_api.h>

/**
 * Подключаем Brotli
 */
#include <brotli/decode.h>
#include <brotli/encode.h>

/**
 * Подключаем Lizard
 */
#include "lizard_compress.h"
#include "lizard_decompress.h"

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
 * Подключаем CityHash
 */
#include <cityhash/city.h>

/**
 * Стандартные модули
 */
#include <cstdio>
#include <cstring>
#include <csignal>
#include <sys/stat.h>
#include <sys/types.h>

/**
 * Подключаем заголовочный файл
 */
#include <sys/transform.hpp>

/**
 * Параметры компрессора GZip
 */
#define MOD_GZIP_ZLIB_CFACTOR 9
#define MOD_GZIP_ZLIB_BSIZE 8096

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Если размер буфера чанка не определён
 */
#ifndef AWH_TRANSFORM_CHUNK_BUFFER_SIZE
	/**
	 * Устанавливаем размер буфера чанка для компрессии/декомпрессии
	 */
	#define AWH_TRANSFORM_CHUNK_BUFFER_SIZE 0x4000
#endif

/**
 * @brief пространство имён драйвера
 *
 */
namespace driver {
	/**
	 * @brief Шаблон функции хэширования текста
	 *
	 * @tparam T сигнатура функции
	 */
	template <typename T>
	/**
	 * @brief Функция хэширования текста
	 *
	 * @param text   текст для хэширования
	 * @param hash   тип хэш-суммы
	 * @param result результат хэширования
	 * @param log    объект для работы с логами
	 */
	static void hashing(const string & text, const awh::transform_t::hash_t hash, T & result, const awh::log_t * log) noexcept {
		// Если текст для хэширования передан
		if(!text.empty()){
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
					case static_cast <uint8_t> (awh::transform_t::hash_t::MD5): {
						// Создаем контекст
						::MD5_CTX ctx;
						// Выполняем инициализацию контекста
						::MD5_Init(&ctx);
						// Выделяем память для промежуточных значений
						digest.resize(16, 0);
						// Выделяем память для буфера данных
						result.resize(33, 0);
						// Выполняем расчет суммы
						::MD5_Update(&ctx, text.c_str(), text.length());
						// Копируем полученные данные
						::MD5_Final(digest.data(), &ctx);
						// Заполняем строку данными MD5
						for(uint8_t i = 0; i < 16; i++)
							// Формируем данные MD5-хэша
							::sprintf(reinterpret_cast <char *> (&result[i * 2]), "%02x", static_cast <uint32_t> (digest[i]));
						// Удаляем последний символ
						result.pop_back();
					} break;
					// Если тип хэш-суммы указан как SHA1
					case static_cast <uint8_t> (awh::transform_t::hash_t::SHA1): {
						// Создаем контекст
						::SHA_CTX ctx;
						// Выполняем инициализацию контекста
						::SHA1_Init(&ctx);
						// Выделяем память для промежуточных значений
						digest.resize(20, 0);
						// Выделяем память для буфера данных
						result.resize(41, 0);
						// Выполняем расчет суммы
						::SHA1_Update(&ctx, text.c_str(), text.length());
						// Копируем полученные данные
						::SHA1_Final(digest.data(), &ctx);
						// Заполняем строку данными SHA1
						for(uint8_t i = 0; i < 20; i++)
							// Формируем данные SHA1-хэша
							::sprintf(reinterpret_cast <char *> (&result[i * 2]), "%02x", static_cast <uint32_t> (digest[i]));
						// Удаляем последний символ
						result.pop_back();
					} break;
					// Если тип хэш-суммы указан как SHA224
					case static_cast <uint8_t> (awh::transform_t::hash_t::SHA224): {
						// Создаем контекст
						::SHA256_CTX ctx;
						// Выполняем инициализацию контекста
						::SHA224_Init(&ctx);
						// Выделяем память для промежуточных значений
						digest.resize(28, 0);
						// Выделяем память для буфера данных
						result.resize(57, 0);
						// Выполняем расчет суммы
						::SHA224_Update(&ctx, text.c_str(), text.length());
						// Копируем полученные данные
						::SHA224_Final(digest.data(), &ctx);
						// Заполняем строку данными SHA224
						for(uint8_t i = 0; i < 28; i++)
							// Формируем данные SHA224-хэша
							::sprintf(reinterpret_cast <char *> (&result[i * 2]), "%02x", static_cast <uint32_t> (digest[i]));
						// Удаляем последний символ
						result.pop_back();
					} break;
					// Если тип хэш-суммы указан как SHA256
					case static_cast <uint8_t> (awh::transform_t::hash_t::SHA256): {
						// Создаем контекст
						::SHA256_CTX ctx;
						// Выполняем инициализацию контекста
						::SHA256_Init(&ctx);
						// Выделяем память для промежуточных значений
						digest.resize(32, 0);
						// Выделяем память для буфера данных
						result.resize(65, 0);
						// Выполняем расчет суммы
						::SHA256_Update(&ctx, text.c_str(), text.length());
						// Копируем полученные данные
						::SHA256_Final(digest.data(), &ctx);
						// Заполняем строку данными SHA256
						for(uint8_t i = 0; i < 32; i++)
							// Формируем данные SHA256-хэша
							::sprintf(reinterpret_cast <char *> (&result[i * 2]), "%02x", static_cast <uint32_t> (digest[i]));
						// Удаляем последний символ
						result.pop_back();
					} break;
					// Если тип хэш-суммы указан как SHA384
					case static_cast <uint8_t> (awh::transform_t::hash_t::SHA384): {
						// Создаем контекст
						::SHA512_CTX ctx;
						// Выполняем инициализацию контекста
						::SHA384_Init(&ctx);
						// Выделяем память для промежуточных значений
						digest.resize(48, 0);
						// Выделяем память для буфера данных
						result.resize(97, 0);
						// Выполняем расчет суммы
						::SHA384_Update(&ctx, text.c_str(), text.length());
						// Копируем полученные данные
						::SHA384_Final(digest.data(), &ctx);
						// Заполняем строку данными SHA384
						for(uint8_t i = 0; i < 48; i++)
							// Формируем данные SHA384-хэша
							::sprintf(reinterpret_cast <char *> (&result[i * 2]), "%02x", static_cast <uint32_t> (digest[i]));
						// Удаляем последний символ
						result.pop_back();
					} break;
					// Если тип хэш-суммы указан как SHA512
					case static_cast <uint8_t> (awh::transform_t::hash_t::SHA512): {
						// Создаем контекст
						::SHA512_CTX ctx;
						// Выполняем инициализацию контекста
						::SHA512_Init(&ctx);
						// Выделяем память для промежуточных значений
						digest.resize(64, 0);
						// Выделяем память для буфера данных
						result.resize(129, 0);
						// Выполняем расчет суммы
						::SHA512_Update(&ctx, text.c_str(), text.length());
						// Копируем полученные данные
						::SHA512_Final(digest.data(), &ctx);
						// Заполняем строку данными SHA512
						for(uint8_t i = 0; i < 64; i++)
							// Формируем данные SHA512-хэша
							::sprintf(reinterpret_cast <char *> (&result[i * 2]), "%02x", static_cast <uint32_t> (digest[i]));
						// Удаляем последний символ
						result.pop_back();
					} break;
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выполняем очистку блока с результатом
				result.clear();
				// Выводим сообщение об ошибке в лог
				log->print("%s", awh::log_t::flag_t::CRITICAL, error.what());
			}
		}
	}
	/**
	 * @brief Шаблон функции хэширования текста с ключом
	 *
	 * @tparam T сигнатура функции
	 */
	template <typename T>
	/**
	 * @brief Функция хэширования текста с ключом
	 *
	 * @param key    ключ для подписи
	 * @param text   текст для хэширования
	 * @param hash   тип хэш-суммы
	 * @param result результат хэширования
	 * @param log    объект для работы с логами
	 */
	static void hmac(const string & key, const string & text, const awh::transform_t::hash_t hash, T & result, const awh::log_t * log) noexcept {
		// Если ключ и текст для хэширования переданы
		if(!key.empty() && !text.empty()){
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
					case static_cast <uint8_t> (awh::transform_t::hash_t::MD5): {
						// Выделяем память для буфера данных
						result.resize(33, 0);
						// Выполняем получение подписи
						const uint8_t * digest = ::HMAC(::EVP_md5(), key.data(), key.size(), reinterpret_cast <const uint8_t *> (text.data()), text.size(), nullptr, nullptr);
						// Заполняем строку данными MD5
						for(uint8_t i = 0; i < 16; i++)
							// Формируем данные MD5-хэша
							::sprintf(reinterpret_cast <char *> (&result[i * 2]), "%02x", static_cast <uint32_t> (digest[i]));
						// Удаляем последний символ
						result.pop_back();
					} break;
					// Если тип хэш-суммы указан как HMAC_SHA1
					case static_cast <uint8_t> (awh::transform_t::hash_t::SHA1): {
						// Выделяем память для буфера данных
						result.resize(41, 0);
						// Выполняем получение подписи
						const uint8_t * digest = ::HMAC(::EVP_sha1(), key.data(), key.size(), reinterpret_cast <const uint8_t *> (text.data()), text.size(), nullptr, nullptr);
						// Заполняем строку данными SHA1
						for(uint8_t i = 0; i < 20; i++)
							// Формируем данные SHA1-хэша
							::sprintf(reinterpret_cast <char *> (&result[i * 2]), "%02x", static_cast <uint32_t> (digest[i]));
						// Удаляем последний символ
						result.pop_back();
					} break;
					// Если тип хэш-суммы указан как HMAC_SHA224
					case static_cast <uint8_t> (awh::transform_t::hash_t::SHA224): {
						// Выделяем память для буфера данных
						result.resize(57, 0);
						// Выполняем получение подписи
						const uint8_t * digest = ::HMAC(::EVP_sha224(), key.data(), key.size(), reinterpret_cast <const uint8_t *> (text.data()), text.size(), nullptr, nullptr);
						// Заполняем строку данными SHA224
						for(uint8_t i = 0; i < 28; i++)
							// Формируем данные SHA224-хэша
							::sprintf(reinterpret_cast <char *> (&result[i * 2]), "%02x", static_cast <uint32_t> (digest[i]));
						// Удаляем последний символ
						result.pop_back();
					} break;
					// Если тип хэш-суммы указан как HMAC_SHA256
					case static_cast <uint8_t> (awh::transform_t::hash_t::SHA256): {
						// Выделяем память для буфера данных
						result.resize(65, 0);
						// Выполняем получение подписи
						const uint8_t * digest = ::HMAC(::EVP_sha256(), key.data(), key.size(), reinterpret_cast <const uint8_t *> (text.data()), text.size(), nullptr, nullptr);
						// Заполняем строку данными SHA256
						for(uint8_t i = 0; i < 32; i++)
							// Формируем данные SHA256-хэша
							::sprintf(reinterpret_cast <char *> (&result[i * 2]), "%02x", static_cast <uint32_t> (digest[i]));
						// Удаляем последний символ
						result.pop_back();
					} break;
					// Если тип хэш-суммы указан как HMAC_SHA384
					case static_cast <uint8_t> (awh::transform_t::hash_t::SHA384): {
						// Выделяем память для буфера данных
						result.resize(97, 0);
						// Выполняем получение подписи
						const uint8_t * digest = ::HMAC(::EVP_sha384(), key.data(), key.size(), reinterpret_cast <const uint8_t *> (text.data()), text.size(), nullptr, nullptr);
						// Заполняем строку данными SHA384
						for(uint8_t i = 0; i < 48; i++)
							// Формируем данные SHA384-хэша
							::sprintf(reinterpret_cast <char *> (&result[i * 2]), "%02x", static_cast <uint32_t> (digest[i]));
						// Удаляем последний символ
						result.pop_back();
					} break;
					// Если тип хэш-суммы указан как HMAC_SHA512
					case static_cast <uint8_t> (awh::transform_t::hash_t::SHA512): {
						// Выделяем память для буфера данных
						result.resize(129, 0);
						// Выполняем получение подписи
						const uint8_t * digest = ::HMAC(::EVP_sha512(), key.data(), key.size(), reinterpret_cast <const uint8_t *> (text.data()), text.size(), nullptr, nullptr);
						// Заполняем строку данными SHA512
						for(uint8_t i = 0; i < 64; i++)
							// Формируем данные SHA512-хэша
							::sprintf(reinterpret_cast <char *> (&result[i * 2]), "%02x", static_cast <uint32_t> (digest[i]));
						// Удаляем последний символ
						result.pop_back();
					} break;
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выполняем очистку блока с результатом
				result.clear();
				// Выводим сообщение об ошибке в лог
				log->print("%s", awh::log_t::flag_t::CRITICAL, error.what());
			}
		}
	}
	/**
	 * @brief Шаблон функции хэширования данных
	 *
	 * @tparam T сигнатура функции
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
	 */
	static void hashing(const char * buffer, const size_t size, const awh::transform_t::cipher_t cipher, const awh::transform_t::event_t event, awh::transform_t::crypto_state_t & state, T & result, const awh::log_t * log) noexcept {
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
					case static_cast <uint8_t> (awh::transform_t::cipher_t::BASE64): {
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
									case static_cast <uint8_t> (awh::transform_t::event_t::ENCODE): {
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
									case static_cast <uint8_t> (awh::transform_t::event_t::DECODE): {
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
									// Выводим сообщение об ошибке в лог
									log->print("Error during data decoding", awh::log_t::flag_t::WARNING);
								}
								// Очищаем всю выделенную память
								::BIO_free_all(bio);
							}
							// Очищаем объект BASE64
							::BIO_free(b64);
						}
					} break;
					// Если производится работы с AES128
					case static_cast <uint8_t> (awh::transform_t::cipher_t::AES128):
					// Если производится работы с AES192
					case static_cast <uint8_t> (awh::transform_t::cipher_t::AES192):
					// Если производится работы с AES256
					case static_cast <uint8_t> (awh::transform_t::cipher_t::AES256): {
						// Смещение в бинарном буфере
						size_t offset = 0;
						// Размер записываемых данных
						size_t length = 0;
						// Определяем размер данных для считывания
						size_t actual = size;
						// Выделяем память для буфера данных
						vector <uint8_t> output(AES_BLOCK_SIZE, 0);
						/**
						 * Выполняем шифровку всех данных
						 */
						do {
							// Максимальный размер считываемых данных
							length = (actual > AES_BLOCK_SIZE ? AES_BLOCK_SIZE : actual);
							/**
							 * Определяем событие кодирование или декодирование
							 */
							switch(static_cast <uint8_t> (event)){
								// Если производится кодирование данных
								case static_cast <uint8_t> (awh::transform_t::event_t::ENCODE):
									// Выполняем сжатие данных
									::AES_cfb128_encrypt(reinterpret_cast <const uint8_t *> (buffer) + offset, output.data(), length, &std::any_cast <AES_KEY &>(state.key), state.ivec, &state.num, AES_ENCRYPT);
								break;
								// Если производится декодирование данных
								case static_cast <uint8_t> (awh::transform_t::event_t::DECODE):
									// Выполняем сжатие данных
									::AES_cfb128_encrypt(reinterpret_cast <const uint8_t *> (buffer) + offset, output.data(), length, &std::any_cast <AES_KEY &>(state.key), state.ivec, &state.num, AES_DECRYPT);
								break;
							}
							// Увеличиваем смещение
							offset += length;
							// Вычитаем считанные данные
							actual -= length;
							// Выполняем добавление полученных данных
							result.insert(result.end(), output.data(), output.data() + length);
						/**
						 * Если данные ещё не зашифрованны
						 */
						} while(actual > 0);
					} break;
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выполняем очистку блока с результатом
				result.clear();
				// Выводим сообщение об ошибке в лог
				log->print("%s", awh::log_t::flag_t::CRITICAL, error.what());
			}
		}
	}
	/**
	 * @brief Шаблон функции работы с компрессором LZma
	 *
	 * @tparam T сигнатура функции
	 */
	template <typename T>
	/**
	 * @brief Функция работы с компрессором LZma
	 *
	 * @param buffer буфер данных для компрессии
	 * @param size   размер данных для компрессии
	 * @param event  событие выполнения операции
	 * @param result строка куда следует положить результат
	 * @param log    объект для работы с логами
	 */
	static void lzma(const char * buffer, const size_t size, const awh::transform_t::event_t event, T & result, const awh::log_t * log) noexcept {
		// Если буфер данных передан
		if((buffer != nullptr) && (size > 80)){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Выполняем очистку блока с результатом
				result.clear();
				/**
				 * Определяем событие выполнения операции
				 */
				switch(static_cast <uint8_t> (event)){
					// Если необходимо выполнить компрессию данных
					case static_cast <uint8_t> (awh::transform_t::event_t::ENCODE): {
						// Инициализируем опции компрессора LZma
						static const lzma_options_lzma options = {
							1u << 20u, nullptr, 0, LZMA_LC_DEFAULT, LZMA_LP_DEFAULT,
							LZMA_PB_DEFAULT, LZMA_MODE_FAST, 128, LZMA_MF_HC3, 4
						};
						// Инициализируем фильтры компрессора LZma
						static const lzma_filter filters[] = {
							{LZMA_FILTER_LZMA2, const_cast <lzma_options_lzma *> (&options)},
							{LZMA_VLI_UNKNOWN, nullptr}
						};
						// Актуальный размер сжатых данных
						size_t actual = 0;
						// Выделяем буфер памяти нужного нам размера
						result.resize(size, 0);
						// Выполняем компрессию буфера данных
						lzma_ret rv = ::lzma_stream_buffer_encode(const_cast <lzma_filter *> (filters), LZMA_CHECK_NONE, nullptr, reinterpret_cast <const uint8_t *> (buffer), size, reinterpret_cast <uint8_t *> (result.data()), &actual, size - 1);
						// Если мы получили ошибку
						if(rv != LZMA_OK){
							// Выполняем очистку результата
							result.clear();
							// Выводим сообщение об ошибке в лог
							log->print("LZMA: Error during data compression", awh::log_t::flag_t::WARNING);
							// Выходим из функции
							return;
						}
						// Корректируем размер результирующего буфера
						result.resize(actual);
					} break;
					// Если необходимо выполнить декомпрессию данных
					case static_cast <uint8_t> (awh::transform_t::event_t::DECODE): {
						// Указатель позиции в буфере для распаковки
						char * ptr = nullptr;
						// Индекс потока LZma компрессора
						lzma_index * index = nullptr;
						// Лимит доступной памяти
						uint64_t memlimit = 0x8000000;
						// Позиции в буферах и актуальный размер данных результата
						size_t inpos = 0, outpos = 0, actual = 0;
						// Смещаем указатель в буфере на подвал
						if((ptr = const_cast <char *> (buffer) + size - 12) < buffer)
							// Переходим к выводу ошибки
							goto Error;
						// Список флагов потока LZma
						lzma_stream_flags flags;
						// Пытаемся декодировать подвал архива
						if(::lzma_stream_footer_decode(&flags, reinterpret_cast <uint8_t *> (ptr)) != LZMA_OK)
							// Переходим к выводу ошибки
							goto Error;
						// Если буфер данных испорчен
						if((ptr -= flags.backward_size) < buffer)
							// Переходим к выводу ошибки
							goto Error;
						// Выполняем декодирование буфера LZma
						if(::lzma_index_buffer_decode(&index, &memlimit, nullptr, reinterpret_cast <uint8_t *> (ptr), &inpos, size - (ptr - buffer)) != LZMA_OK)
							// Переходим к выводу ошибки
							goto Error;
						// Сбрасываем иозицию во входящем буфере
						inpos = 0;
						// Сбрасываем лимит доступной памяти
						memlimit = 0x8000000;
						// Получаем размер результирующего буфера данных
						actual = ::lzma_index_uncompressed_size(index);
						// Выделяем буфер памяти нужного нам размера
						result.resize(actual, 0);
						// Выполняем декомпрессию буфера бинарных данных
						if(::lzma_stream_buffer_decode(&memlimit, 0, nullptr, reinterpret_cast <const uint8_t *> (buffer), &inpos, size, reinterpret_cast <uint8_t *> (result.data()), &outpos, actual) == LZMA_OK){
							// Выполняем закрытие индекса компрессора LZma
							::lzma_index_end(index, nullptr);
							// Выходим из функции
							return;
						}
						// Устанавливаем метку вывода ошибки
						Error:
						// Выполняем очистку результата
						result.clear();
						// Выводим сообщение об ошибке в лог
						log->print("LZMA: Error during data decompression", awh::log_t::flag_t::WARNING);
						// Выходим из функции
						return;
					}
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выполняем очистку блока с результатом
				result.clear();
				// Выводим сообщение об ошибке в лог
				log->print("LZMA: %s", awh::log_t::flag_t::CRITICAL, error.what());
			}
		}
	}
	/**
	 * @brief Шаблон функции работы с компрессором BZip2
	 *
	 * @tparam T сигнатура функции
	 */
	template <typename T>
	/**
	 * @brief Функция работы с компрессором BZip2
	 *
	 * @param buffer буфер данных для компрессии
	 * @param size   размер данных для компрессии
	 * @param event  событие выполнения операции
	 * @param result строка куда следует положить результат
	 * @param log    объект для работы с логами
	 */
	static void bzip2(const char * buffer, const size_t size, const awh::transform_t::event_t event, T & result, const awh::log_t * log) noexcept {
		// Если буфер данных передан
		if((buffer != nullptr) && (size > 0)){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Выполняем очистку блока с результатом
				result.clear();
				// Результат выполнения компрессии
				int32_t rv = BZ_OK;
				// Выполняем создание объекта потока
				bz_stream stream;
				// Выполняем зануление параметров потока
				stream.bzfree  = nullptr;
				stream.opaque  = nullptr;
				stream.bzalloc = nullptr;
				/**
				 * Определяем событие выполнения операции
				 */
				switch(static_cast <uint8_t> (event)){
					// Если необходимо выполнить компрессию данных
					case static_cast <uint8_t> (awh::transform_t::event_t::ENCODE): {
						// Выполняем инициализацию потока
						if(::BZ2_bzCompressInit(&stream, 5, 0, 0) != BZ_OK){
							// Выводим сообщение об ошибке в лог
							log->print("Bzip2: Error during data compression", awh::log_t::flag_t::WARNING);
							// Выходим из функции
							return;
						}
						// Выделяем память на результирующий буфер
						result.resize(size, 0);
						// Указываем размер входного буфера
						stream.avail_in = static_cast <uint32_t> (size);
						// Заполняем входные данные буфера
						stream.next_in = const_cast <char *> (buffer);
						// Устанавливаем буфер для получения результата
						stream.next_out = reinterpret_cast <char *> (result.data());
						// Устанавливаем максимальный размер буфера
						stream.avail_out = static_cast <uint32_t> (result.size());
						/**
						 * Выполняем компрессию буфера бинарных данных
						 */
						while((rv = ::BZ2_bzCompress(&stream, BZ_FINISH)) != BZ_STREAM_END){
							// Выполняем ещё одну попытку компрессии
							rv = ::BZ2_bzCompress(&stream, BZ_FINISH);
							// Если произошла ошибка компрессии
							if((rv != BZ_FINISH_OK) && (rv != BZ_STREAM_END)){
								// Выводим сообщение об ошибке в лог
								log->print("Bzip2: Error during data compression", awh::log_t::flag_t::WARNING);
								// Выходим из цикла
								break;
							}
						}
						// Если данные обработаны удачно
						if((rv == BZ_FINISH_OK) || (rv == BZ_STREAM_END))
							// Добавляем оставшиеся данные в список
							result.erase(result.begin() + (result.size() - stream.avail_out), result.end());
						// Если произошла ошибка компрессии
						else {
							// Выполняем очистку буфера данных
							result.clear();
							// Выводим сообщение об ошибке в лог
							log->print("Bzip2: Error during data compression", awh::log_t::flag_t::WARNING);
						}
						// Выполняем очистку объекта потока
						::BZ2_bzCompressEnd(&stream);
					} break;
					// Если необходимо выполнить декомпрессию данных
					case static_cast <uint8_t> (awh::transform_t::event_t::DECODE): {
						// Выполняем инициализацию потока
						if(::BZ2_bzDecompressInit(&stream, 0, 0) != BZ_OK){
							// Выводим сообщение об ошибке в лог
							log->print("Bzip2: Error during data decompression", awh::log_t::flag_t::WARNING);
							// Выходим из функции
							return;
						}
						// Заполняем входные данные буфера
						stream.next_in = const_cast <char *> (buffer);
						// Указываем размер входного буфера
						stream.avail_in = static_cast <uint32_t> (size);
						// Размер буфера извлечённых данных
						uint32_t actual = (static_cast <uint32_t> (size) * 2);
						// Выделяем память на результирующий буфер
						result.resize(actual, 0);
						/**
						 * Выполняем компрессию всех данных
						 */
						do {
							// Если место для извлечения данных закончилось
							if((actual - stream.total_out_lo32) == 0){
								// Увеличиваем буфер исходящих данных в два раза
								actual *= 2;
								// Выделяем пмять для буфера извлечения данных
								result.resize(actual, 0);
							}
							// Устанавливаем буфер для получения результата
							stream.next_out = reinterpret_cast <char *> (result.data() + stream.total_out_lo32);
							// Устанавливаем максимальный размер буфера
							stream.avail_out = (actual - stream.total_out_lo32);
							// Выполняем декомпрессию
							rv = ::BZ2_bzDecompress(&stream);
							// Если мы завершили сбор данных
							if((rv == BZ_STREAM_END) || (rv == BZ_FINISH_OK)){
								// Выводим сообщение об ошибке в лог
								log->print("Bzip2: Error during data decompression", awh::log_t::flag_t::WARNING);
								// Выходим из цикла
								break;
							}
						/**
						 * Если данные ещё не извлечены
						 */
						} while(rv == BZ_OK);
						// Если данные обработаны удачно
						if((rv == BZ_FINISH_OK) || (rv == BZ_STREAM_END))
							// Добавляем оставшиеся данные в список
							result.erase(result.begin() + (result.size() - stream.avail_out), result.end());
						// Если произошла ошибка декомпрессии
						else {
							// Выполняем очистку буфера данных
							result.clear();
							// Выводим сообщение об ошибке в лог
							log->print("Bzip2: Error during data decompression", awh::log_t::flag_t::WARNING);
						}
						// Выполняем очистку объекта потока
						::BZ2_bzDecompressEnd(&stream);
					} break;
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выполняем очистку блока с результатом
				result.clear();
				// Выводим сообщение об ошибке в лог
				log->print("Bzip2: %s", awh::log_t::flag_t::CRITICAL, error.what());
			}
		}
	}
	/**
	 * @brief Шаблон функции работы с компрессором Brotli
	 *
	 * @tparam T сигнатура функции
	 */
	template <typename T>
	/**
	 * @brief Функция работы с компрессором Brotli
	 *
	 * @param buffer буфер данных для компрессии
	 * @param size   размер данных для компрессии
	 * @param event  событие выполнения операции
	 * @param result строка куда следует положить результат
	 * @param log    объект для работы с логами
	 */
	static void brotli(const char * buffer, const size_t size, const awh::transform_t::event_t event, T & result, const awh::log_t * log) noexcept {
		// Если буфер данных передан
		if((buffer != nullptr) && (size > 0)){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Выполняем очистку блока с результатом
				result.clear();
				// Получаем размер бинарного буфера входящих данных
				size_t sizeInput = size;
				// Создаём временный буфер данных
				vector <uint8_t> data(AWH_TRANSFORM_CHUNK_BUFFER_SIZE, 0);
				// Получаем бинарный буфер входящих данных
				const uint8_t * nextInput = reinterpret_cast <const uint8_t *> (buffer);
				/**
				 * Определяем событие выполнения операции
				 */
				switch(static_cast <uint8_t> (event)){
					// Если необходимо выполнить компрессию данных
					case static_cast <uint8_t> (awh::transform_t::event_t::ENCODE): {
						// Инициализируем стейт энкодера Brotli
						BrotliEncoderState * encoder = ::BrotliEncoderCreateInstance(nullptr, nullptr, nullptr);
						/**
						 * Выполняем сжатие данных
						 */
						while(!::BrotliEncoderIsFinished(encoder)){
							// Получаем размер буфера закодированных бинарных данных
							size_t sizeOutput = data.size();
							// Получаем буфер закодированных бинарных данных
							uint8_t * nextOutput = data.data();
							// Если сжатие данных закончено, то завершаем работу
							if(!::BrotliEncoderCompressStream(encoder, BROTLI_OPERATION_FINISH, &sizeInput, &nextInput, &sizeOutput, &nextOutput, nullptr)){
								// Выводим сообщение об ошибке в лог
								log->print("Brotli: Error during data compression", awh::log_t::flag_t::WARNING);
								// Выходим из цикла
								break;
							}
							// Получаем размер полученных данных
							const size_t size = (data.size() - sizeOutput);
							// Если данные получены, формируем результирующий буфер
							if(size > 0){
								// Получаем буфер данных
								const char * buffer = reinterpret_cast <const char *> (data.data());
								// Формируем результирующий буфер бинарных данных
								result.insert(result.end(), buffer, buffer + size);
							}
						}
						// Освобождаем память энкодера
						::BrotliEncoderDestroyInstance(encoder);
					} break;
					// Если необходимо выполнить декомпрессию данных
					case static_cast <uint8_t> (awh::transform_t::event_t::DECODE): {
						// Полный размер обработанных данных
						size_t total = 0, size = 0;
						// Активируем работу декодера
						BrotliDecoderResult ret = BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT;
						// Инициализируем стейт декодера Brotli
						BrotliDecoderState * decoder = ::BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
						/**
						 * Если декодеру есть с чем работать
						 */
						while(ret == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT){
							// Получаем размер буфера декодированных бинарных данных
							size_t sizeOutput = data.size();
							// Получаем буфер декодированных бинарных данных
							char * nextOutput = reinterpret_cast <char *> (data.data());
							// Выполняем декодирование бинарных данных
							ret = ::BrotliDecoderDecompressStream(decoder, &sizeInput, &nextInput, &sizeOutput, reinterpret_cast <uint8_t **> (&nextOutput), &total);
							// Если декодирование данных не выполнено
							if(ret == BROTLI_DECODER_RESULT_ERROR){
								// Выводим сообщение об ошибке в лог
								log->print("Brotli: Error during data decompression", awh::log_t::flag_t::WARNING);
								// Выходим из цикла
								break;
							}
							// Получаем размер полученных данных
							size = (data.size() - sizeOutput);
							// Если данные получены, формируем результирующий буфер
							if(size > 0){
								// Получаем буфер данных
								const char * buffer = reinterpret_cast <const char *> (data.data());
								// Формируем результирующий буфер бинарных данных
								result.insert(result.end(), buffer, buffer + size);
							}
						}
						// Если декомпрессия данных выполнена не удачно
						if((ret != BROTLI_DECODER_RESULT_SUCCESS) && (ret != BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT)){
							// Выводим сообщение об ошибке в лог
							log->print("Brotli: Error during data decompression", awh::log_t::flag_t::WARNING);
							// Выполняем очистку результата
							result.clear();
						}
						// Освобождаем память декодера
						::BrotliDecoderDestroyInstance(decoder);
					} break;
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выполняем очистку блока с результатом
				result.clear();
				// Выводим сообщение об ошибке в лог
				log->print("Brotli: %s", awh::log_t::flag_t::CRITICAL, error.what());
			}
		}
	}
	/**
	 * @brief Шаблон функции работы с компрессором Snappy
	 *
	 * @tparam T сигнатура функции
	 */
	template <typename T>
	/**
	 * @brief Функция работы с компрессором Snappy
	 *
	 * @param buffer буфер данных для компрессии
	 * @param size   размер данных для компрессии
	 * @param event  событие выполнения операции
	 * @param result строка куда следует положить результат
	 * @param log    объект для работы с логами
	 */
	static void snappy(const char * buffer, const size_t size, const awh::transform_t::event_t event, T & result, const awh::log_t * log) noexcept {
		// Если буфер данных передан
		if((buffer != nullptr) && (size > 0)){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Выполняем очистку блока с результатом
				result.clear();
				// Временный промежуточный буфер данных
				string data = "";
				/**
				 * Определяем событие выполнения операции
				 */
				switch(static_cast <uint8_t> (event)){
					// Если необходимо выполнить компрессию данных
					case static_cast <uint8_t> (awh::transform_t::event_t::ENCODE):
						// Выполняем компрессию данных
						snappy::Compress(buffer, size, &data);
					break;
					// Если необходимо выполнить декомпрессию данных
					case static_cast <uint8_t> (awh::transform_t::event_t::DECODE):
						// Выполняем декомпрессию данных
						snappy::Uncompress(buffer, size, &data);
					break;
				}
				// Если результат получен
				if(!data.empty())
					// Формируем результат
					result.assign(data.begin(), data.end());
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выполняем очистку блока с результатом
				result.clear();
				// Выводим сообщение об ошибке в лог
				log->print("Snappy: %s", awh::log_t::flag_t::CRITICAL, error.what());
			}
		}
	}
	/**
	 * @brief Шаблон функции работы с компрессором Density
	 *
	 * @tparam T сигнатура функции
	 */
	template <typename T>
	/**
	 * @brief Функция работы с компрессором Density
	 *
	 * @param buffer буфер данных для компрессии
	 * @param size   размер данных для компрессии
	 * @param event  событие выполнения операции
	 * @param result строка куда следует положить результат
	 */
	static void density(const char * buffer, const size_t size, const awh::transform_t::event_t event, T & result, const awh::log_t * log) noexcept {
		// Если буфер данных передан
		if((buffer != nullptr) && (size > 0)){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Выполняем очистку блока с результатом
				result.clear();
				/**
				 * Определяем событие выполнения операции
				 */
				switch(static_cast <uint8_t> (event)){
					// Если необходимо выполнить компрессию данных
					case static_cast <uint8_t> (awh::transform_t::event_t::ENCODE): {
						// Выполняем получение размер результирующего буфера
						const uint_fast64_t actual = ::density_compress_safe_size(size);
						// Если размер выделен
						if(actual == 0){
							// Выводим сообщение об ошибке в лог
							log->print("Density: Error during data compression", awh::log_t::flag_t::WARNING);
							// Выходим из функции
							return;
						}
						// Выделяем буфер памяти нужного нам размера
						result.resize(actual, 0);
						// Выполняем компрессию буфера данных
						const auto & status = ::density_compress(reinterpret_cast <const uint8_t *> (buffer), size, reinterpret_cast <uint8_t *> (result.data()), actual, DENSITY_ALGORITHM_CHAMELEON);
						// Если мы получили ошибку
						if(!status.state){
							// Выполняем очистку блока с результатом
							result.clear();
							// Выводим сообщение об ошибке в лог
							log->print("Density: Error during data compression", awh::log_t::flag_t::WARNING);
							// Выходим из функции
							return;
						}
						// Корректируем размер результирующего буфера
						result.resize(status.bytesWritten);
					} break;
					// Если необходимо выполнить декомпрессию данных
					case static_cast <uint8_t> (awh::transform_t::event_t::DECODE): {
						// Выполняем получение размер результирующего буфера
						const uint_fast64_t actual = ::density_decompress_safe_size(size);
						// Если размер выделен
						if(actual == 0){
							// Выводим сообщение об ошибке в лог
							log->print("Density: Error during data decompression", awh::log_t::flag_t::WARNING);
							// Выходим из функции
							return;
						}
						// Выделяем буфер памяти нужного нам размера
						result.resize(actual, 0);
						// Выполняем компрессию буфера данных
						const auto & status = ::density_decompress(reinterpret_cast <const uint8_t *> (buffer), size, reinterpret_cast <uint8_t *> (result.data()), actual);
						// Если мы получили ошибку
						if(!status.state){
							// Выполняем очистку блока с результатом
							result.clear();
							// Выводим сообщение об ошибке в лог
							log->print("Density: Error during data decompression", awh::log_t::flag_t::WARNING);
							// Выходим из функции
							return;
						}
						// Корректируем размер результирующего буфера
						result.resize(status.bytesWritten);
					} break;
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выполняем очистку блока с результатом
				result.clear();
				// Выводим сообщение об ошибке в лог
				log->print("Density: %s", awh::log_t::flag_t::CRITICAL, error.what());
			}
		}
	}
	/**
	 * @brief Шаблон функции работы с компрессором Lizard
	 *
	 * @tparam T сигнатура функции
	 */
	template <typename T>
	/**
	 * @brief Функция работы с компрессором Lizard
	 *
	 * @param buffer буфер данных для компрессии
	 * @param size   размер данных для компрессии
	 * @param level  уровень компрессии
	 * @param event  событие выполнения операции
	 * @param result строка куда следует положить результат
	 */
	static void lizard(const char * buffer, const size_t size, const uint32_t level, const awh::transform_t::event_t event, T & result, const awh::log_t * log) noexcept {
		// Если буфер данных передан
		if((buffer != nullptr) && (size > 0)){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Выполняем очистку блока с результатом
				result.clear();
				/**
				 * Определяем событие выполнения операции
				 */
				switch(static_cast <uint8_t> (event)){
					// Если необходимо выполнить компрессию данных
					case static_cast <uint8_t> (awh::transform_t::event_t::ENCODE): {
						// Выполняем получение размер результирующего буфера
						int32_t actual = ::Lizard_compressBound(size);
						// Если размер выделен
						if(actual == 0){
							// Выводим сообщение об ошибке в лог
							log->print("Lizard: Error during data compression", awh::log_t::flag_t::WARNING);
							// Выходим из функции
							return;
						}
						// Выделяем буфер памяти нужного нам размера
						result.resize(actual, 0);
						// Выполняем компрессию буфера данных
						actual = ::Lizard_compress(buffer, reinterpret_cast <char *> (result.data()), size, actual, level);
						// Если мы получили ошибку
						if(actual <= 0){
							// Выполняем очистку блока с результатом
							result.clear();
							// Выводим сообщение об ошибке в лог
							log->print("Lizard: Error during data compression", awh::log_t::flag_t::WARNING);
							// Выходим из функции
							return;
						}
						// Корректируем размер результирующего буфера
						result.resize(actual);
					} break;
					// Если необходимо выполнить декомпрессию данных
					case static_cast <uint8_t> (awh::transform_t::event_t::DECODE): {
						// Множитель
						size_t factor = 2;
						/**
						 * Выполняем извлечение данных пока не извлечём
						 */
						for(;;){
							// Выделяем буфер памяти нужного нам размера
							result.resize(size * factor, 0);
							// Выполняем получение размер результирующего буфера
							int32_t actual = result.size();
							// Выполняем декомпрессию буфера бинарных данных
							actual = ::Lizard_decompress_safe(buffer, reinterpret_cast <char *> (result.data()), size, actual);
							// Если компрессия не выполнена из-за отсутствия памяти
							if(actual < 0)
								// Выполняем увеличение множителя
								factor++;
							// Если компрессия не выполнена
							else if(actual == 0) {
								// Выполняем очистку блока с результатом
								result.clear();
								// Выводим сообщение об ошибке в лог
								log->print("Lizard: Error during data decompression", awh::log_t::flag_t::WARNING);
								// Выходим из функции
								return;
							// Если данные извлечены удачно
							} else {
								// Корректируем размер результирующего буфера
								result.resize(actual);
								// Выходим из цикла
								break;
							}
						}
					} break;
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выполняем очистку блока с результатом
				result.clear();
				// Выводим сообщение об ошибке в лог
				log->print("Lizard: %s", awh::log_t::flag_t::CRITICAL, error.what());
			}
		}
	}
	/**
	 * @brief Шаблон функции работы с компрессором Lz4
	 *
	 * @tparam T сигнатура функции
	 */
	template <typename T>
	/**
	 * @brief Функция работы с компрессором Lz4
	 *
	 * @param buffer буфер данных
	 * @param size   размер данных
	 * @param level  уровень компрессии
	 * @param event  событие выполнения операции
	 * @param result строка куда следует положить результат
	 * @param log    объект для работы с логами
	 */
	static void lz4(const char * buffer, const size_t size, const uint32_t level, const awh::transform_t::event_t event, T & result, const awh::log_t * log) noexcept {
		// Если буфер данных передан
		if((buffer != nullptr) && (size > 0)){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Выполняем очистку блока с результатом
				result.clear();
				/**
				 * Определяем событие выполнения операции
				 */
				switch(static_cast <uint8_t> (event)){
					// Если необходимо выполнить компрессию данных
					case static_cast <uint8_t> (awh::transform_t::event_t::ENCODE): {
						// Выполняем получение размер результирующего буфера
						int32_t actual = ::LZ4_compressBound(size);
						// Если размер выделен
						if(actual <= 0){
							// Выполняем очистку результата
							result.clear();
							// Выводим сообщение об ошибке в лог
							log->print("LZ4: Error during data compression", awh::log_t::flag_t::WARNING);
							// Выходим из функции
							return;
						}
						// Выделяем буфер памяти нужного нам размера
						result.resize(actual, 0);
						// Выполняем компрессию буфера бинарных данных
						actual = ::LZ4_compress_fast(buffer, reinterpret_cast <char *> (result.data()), size, actual, level);
						// Если компрессия не выполнена
						if((actual <= 0) || (static_cast <uint32_t> (actual) > static_cast <uint32_t> (size + size / 10))){
							// Выполняем очистку результата
							result.clear();
							// Выводим сообщение об ошибке в лог
							log->print("LZ4: Error during data compression", awh::log_t::flag_t::WARNING);
							// Выходим из функции
							return;
						}
						// Корректируем размер результирующего буфера
						result.resize(actual);
					} break;
					// Если необходимо выполнить декомпрессию данных
					case static_cast <uint8_t> (awh::transform_t::event_t::DECODE): {
						// Множитель
						size_t factor = 2;
						/**
						 * Выполняем извлечение данных пока не извлечём
						 */
						for(;;){
							// Выделяем буфер памяти нужного нам размера
							result.resize(size * factor, 0);
							// Выполняем получение размер результирующего буфера
							int32_t actual = result.size();
							// Выполняем декомпрессию буфера бинарных данных
							actual = ::LZ4_decompress_safe(buffer, reinterpret_cast <char *> (result.data()), size, actual);
							// Если компрессия не выполнена из-за отсутствия памяти
							if(actual < 0)
								// Выполняем увеличение множителя
								factor++;
							// Если компрессия не выполнена
							else if(actual == 0){
								// Выполняем очистку результата
								result.clear();
								// Выводим сообщение об ошибке в лог
								log->print("LZ4: Error during data decompression", awh::log_t::flag_t::WARNING);
								// Выходим из функции
								return;
							// Если данные извлечены удачно
							} else {
								// Корректируем размер результирующего буфера
								result.resize(actual);
								// Выходим из цикла
								break;
							}
						}
					} break;
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выполняем очистку блока с результатом
				result.clear();
				// Выводим сообщение об ошибке в лог
				log->print("LZ4: %s", awh::log_t::flag_t::CRITICAL, error.what());
			}
		}
	}
	/**
	 * @brief Шаблон функции работы с компрессором Zstandard
	 *
	 * @tparam T сигнатура функции
	 */
	template <typename T>
	/**
	 * @brief Функция работы с компрессором Zstandard
	 *
	 * @param buffer буфер данных
	 * @param size   размер данных
	 * @param level  уровень компрессии
	 * @param event  событие выполнения операции
	 * @param result строка куда следует положить результат
	 * @param log    объект для работы с логами
	 */
	static void zstd(const char * buffer, const size_t size, const uint32_t level, const awh::transform_t::event_t event, T & result, const awh::log_t * log) noexcept {
		// Если буфер данных передан
		if((buffer != nullptr) && (size > 0)){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Выполняем очистку блока с результатом
				result.clear();
				/**
				 * Определяем событие выполнения операции
				 */
				switch(static_cast <uint8_t> (event)){
					// Если необходимо выполнить компрессию данных
					case static_cast <uint8_t> (awh::transform_t::event_t::ENCODE): {
						// Выполняем создание контекста потока
						ZSTD_CStream * ctx = ::ZSTD_createCStream();
						// Если контекст потока создан
						if(ctx == nullptr){
							// Выполняем очистку результата
							result.clear();
							// Выводим сообщение об ошибке в лог
							log->print("Zstandard: Error during data compression", awh::log_t::flag_t::WARNING);
							// Выходим из функции
							return;
						}
						// Выполняем инициализацию потока
						size_t status = ::ZSTD_initCStream(ctx, level);
						// Если мы получили ошибку инициализации
						if(::ZSTD_isError(status)){
							// Выполняем очистку результата
							result.clear();
							// Выполняем удаление потока
							::ZSTD_freeCStream(ctx);
							// Выводим сообщение об ошибке в лог
							log->print("Zstandard: %s", awh::log_t::flag_t::WARNING, ::ZSTD_getErrorName(status));
							// Выходим из функции
							return;
						}
						// Инициализируем переменные смещения в буфере и актуальный размер данных
						size_t offset = 0, actual = 0;
						// Получаем длину итогового буфера данных
						const size_t length = ::ZSTD_CStreamOutSize();
						// Выполняем инициализацию итогового буфера данных
						const auto data = std::make_unique <char []> (length);
						// Выполняем создание буфера исходящих данных
						ZSTD_outBuffer output = {data.get(), length, 0};
						/**
						 * Выполняем обработку всех входящих данных
						 */
						while(offset < size){
							// Определяем актуальный размер данных
							actual = (((size - offset) > static_cast <size_t> (::ZSTD_CStreamInSize())) ? static_cast <size_t> (::ZSTD_CStreamInSize()) : (size - offset));
							// Выполняем создание буфера данных для входящих сжатых данных
							ZSTD_inBuffer input = {buffer + offset, actual, 0};
							/**
							 * Выполняем обработку до тех пор пока все не обработаем
							 */
							while(input.pos < input.size){
								// Сбрасываем позицию буфера
								output.pos = 0;
								// Выполняем компрессию полученных данных
								status = ::ZSTD_compressStream(ctx, &output, &input);
								// Если мы получили ошибку инициализации
								if(::ZSTD_isError(status)){
									// Выполняем очистку результата
									result.clear();
									// Выполняем удаление потока
									::ZSTD_freeCStream(ctx);
									// Выводим сообщение об ошибке в лог
									log->print("Zstandard: %s", awh::log_t::flag_t::WARNING, ::ZSTD_getErrorName(status));
									// Выходим из функции
									return;
								}
								// Выполняем формирование полученных данных
								result.insert(result.end(), data.get(), data.get() + output.pos);
							}
							// Увеличиваем смещение в исходном буфере необработанных данных
							offset += actual;
						}
						// Сбрасываем позицию буфера
						output.pos = 0;
						// Завершаем поток
						status = ::ZSTD_endStream(ctx, &output);
						// Если мы получили ошибку инициализации
						if(::ZSTD_isError(status)){
							// Выполняем очистку результата
							result.clear();
							// Выполняем удаление потока
							::ZSTD_freeCStream(ctx);
							// Выводим сообщение об ошибке в лог
							log->print("Zstandard: %s", awh::log_t::flag_t::WARNING, ::ZSTD_getErrorName(status));
							// Выходим из функции
							return;
						}
						// Выполняем формирование полученных данных
						result.insert(result.end(), data.get(), data.get() + output.pos);
						// Выполняем удаление потока
						::ZSTD_freeCStream(ctx);
						/*
						// Выполняем получение размер результирующего буфера
						size_t actual = ::ZSTD_compressBound(size);
						// Если размер выделен
						if(actual == 0){
							// Выполняем очистку результата
							result.clear();
							// Выходим из функции
							return;
						}
						// Выделяем буфер памяти нужного нам размера
						result.resize(actual, 0);
						// Выполняем компрессию буфера данных
						actual = ::ZSTD_compress(result.data(), actual, buffer, size, level);
						// Если мы получили ошибку
						if(::ZSTD_isError(actual)){
							// Выполняем очистку результата
							result.clear();
							// Выводим сообщение об ошибке в лог
							log->print("Zstandard: %s", awh::log_t::flag_t::WARNING, ::ZSTD_getErrorName(actual));
							// Выходим из функции
							return;
						}
						// Корректируем размер результирующего буфера
						result.resize(actual);
						*/
					} break;
					// Если необходимо выполнить декомпрессию данных
					case static_cast <uint8_t> (awh::transform_t::event_t::DECODE): {
						// Выполняем создание контекста потока
						ZSTD_DStream * ctx = ::ZSTD_createDStream();
						// Если контекст потока создан
						if(ctx == nullptr){
							// Выполняем очистку результата
							result.clear();
							// Выводим сообщение об ошибке в лог
							log->print("Zstandard: Error during data decompression", awh::log_t::flag_t::WARNING);
							// Выходим из функции
							return;
						}
						// Выполняем инициализацию потока
						size_t status = ::ZSTD_initDStream(ctx);
						// Если мы получили ошибку инициализации
						if(::ZSTD_isError(status)){
							// Выполняем очистку результата
							result.clear();
							// Выполняем удаление потока
							::ZSTD_freeDStream(ctx);
							// Выводим сообщение об ошибке в лог
							log->print("Zstandard: %s", awh::log_t::flag_t::WARNING, ::ZSTD_getErrorName(status));
							// Выходим из функции
							return;
						}
						// Инициализируем переменные смещения в буфере и актуальный размер данных
						size_t offset = 0, actual = 0;
						// Получаем длину итогового буфера данных
						const size_t length = ::ZSTD_DStreamOutSize();
						// Выполняем инициализацию итогового буфера данных
						const auto data = std::make_unique <char []> (length);
						// Выполняем создание буфера исходящих данных
						ZSTD_outBuffer output = {data.get(), length, 0};
						/**
						 * Выполняем обработку всех входящих данных
						 */
						while(offset < size){
							// Определяем актуальный размер данных
							actual = (((size - offset) > static_cast <size_t> (::ZSTD_DStreamInSize())) ? static_cast <size_t> (::ZSTD_DStreamInSize()) : (size - offset));
							// Выполняем создание буфера данных для входящих сжатых данных
							ZSTD_inBuffer input = {buffer + offset, actual, 0};
							/**
							 * Выполняем обработку до тех пор пока все не обработаем
							 */
							while(input.pos < input.size){
								// Сбрасываем позицию буфера
								output.pos = 0;
								// Выполняем декомпрессию полученных данных
								status = ::ZSTD_decompressStream(ctx, &output, &input);
								// Если мы получили ошибку инициализации
								if(::ZSTD_isError(status)){
									// Выполняем очистку результата
									result.clear();
									// Выполняем удаление потока
									::ZSTD_freeDStream(ctx);
									// Выводим сообщение об ошибке в лог
									log->print("Zstandard: %s", awh::log_t::flag_t::WARNING, ::ZSTD_getErrorName(status));
									// Выходим из функции
									return;
								}
								// Выполняем формирование полученных данных
								result.insert(result.end(), data.get(), data.get() + output.pos);
							}
							// Увеличиваем смещение в исходном буфере необработанных данных
							offset += actual;
						}
						// Выполняем удаление потока
						::ZSTD_freeDStream(ctx);
						/*
						// Получаем размер будущего фрейма (определяем размер контента)
						size_t actual = ::ZSTD_getFrameContentSize(buffer, size);
						// Если размер контента не получен или неизвестен
						if((actual == 0) || (actual == ZSTD_CONTENTSIZE_UNKNOWN) || (actual == ZSTD_CONTENTSIZE_ERROR))
							// Выполняем перерасчёт итогового размера
							actual = (size * 5);
						// Выделяем буфер памяти нужного нам размера
						result.resize(actual, 0);
						// Выполняем декомпрессию буфера данных
						actual = ::ZSTD_decompress(result.data(), actual, buffer, size);
						// Если мы получили ошибку
						if(::ZSTD_isError(actual)){
							// Выполняем очистку результата
							result.clear();
							// Выводим сообщение об ошибке в лог
							log->print("Zstandard: %s", awh::log_t::flag_t::WARNING, ::ZSTD_getErrorName(actual));
							// Выходим из функции
							return;
						}
						// Корректируем размер результирующего буфера
						result.resize(actual);
						*/
					} break;
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выполняем очистку блока с результатом
				result.clear();
				// Выводим сообщение об ошибке в лог
				log->print("Zstandard: %s", awh::log_t::flag_t::CRITICAL, error.what());
			}
		}
	}
	/**
	 * @brief Шаблон функции работы с компрессором GZip
	 *
	 * @tparam T сигнатура функции
	 */
	template <typename T>
	/**
	 * @brief Функция работы с компрессором GZip
	 *
	 * @param buffer буфер данных
	 * @param size   размер данных
	 * @param level  уровень компрессии
	 * @param wbit   размер скользящего окна
	 * @param event  событие выполнения операции
	 * @param result строка куда следует положить результат
	 * @param log    объект для работы с логами
	 */
	static void gzip(const char * buffer, const size_t size, const uint32_t level, const int16_t wbit, const awh::transform_t::event_t event, T & result, const awh::log_t * log) noexcept {
		// Если буфер данных передан
		if((buffer != nullptr) && (size > 0)){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Выполняем очистку блока с результатом
				result.clear();
				// Создаем поток Zip
				z_stream zs{};
				// Инициализируем поля структуры
				zs.zalloc = Z_NULL;
				zs.zfree  = Z_NULL;
				zs.opaque = Z_NULL;
				// Вычисляем размер скользящего окна
				const int32_t windowBits = (wbit + 16);
				/**
				 * Определяем событие выполнения операции
				 */
				switch(static_cast <uint8_t> (event)){
					// Если необходимо выполнить компрессию данных
					case static_cast <uint8_t> (awh::transform_t::event_t::ENCODE): {
						// Если поток инициализировать не удалось, выходим
						if(::deflateInit2(&zs, static_cast <int32_t> (level), Z_DEFLATED, windowBits, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY) == Z_OK){
							// Указываем размер входного буфера
							zs.avail_in = static_cast <uInt> (size);
							// Заполняем входные данные буфера
							zs.next_in = reinterpret_cast <Bytef *> (const_cast <char *> (buffer));
							// Оценка максимального размера (включая заголовок и хвост)
							const size_t maxSize = ::deflateBound(&zs, static_cast <uLong> (size));
							// Выделяем память на результирующий буфер
							result.resize(maxSize);
							// Устанавливаем максимальный размер буфера
							zs.avail_out = static_cast <uInt> (maxSize);
							// Устанавливаем буфер для получения результата
							zs.next_out = reinterpret_cast <Bytef *> (result.data());
							// Выполняем сжатие данных
							const int32_t ret = ::deflate(&zs, Z_FINISH);
							// Завершаем сжатие
							::deflateEnd(&zs);
							// Если мы успешно завершили сжатие
							if(ret == Z_STREAM_END)
								// Корректируем размер результирующего буфера
								result.resize(zs.total_out);
							// Если произошла ошибка компрессии
							else {
								// Выполняем очистку буфера данных
								result.clear();
								// Выводим сообщение об ошибке в лог
								log->print("GZip: Error during data compression", awh::log_t::flag_t::WARNING);
							}
						// Выводим сообщение об ошибке в лог
						} else log->print("GZip: Error during data compression", awh::log_t::flag_t::WARNING);
					} break;
					// Если необходимо выполнить декомпрессию данных
					case static_cast <uint8_t> (awh::transform_t::event_t::DECODE): {
						// Если поток инициализировать не удалось, выходим
						if(::inflateInit2(&zs, windowBits) == Z_OK){
							// Устанавливаем размер входного буфера
							zs.avail_in = static_cast <uInt> (size);
							// Устанавливаем буфер входящих данных
							zs.next_in = reinterpret_cast <Bytef *> (const_cast <char *> (buffer));
							// Буфер для извлечённых данных
							vector <uint8_t> output(::max <size_t>(256, size * 2));
							// Результат проверки декомпрессии
							int32_t ret = Z_OK;
							// Переменная подсчёта сжатых данных
							size_t produced = 0;
							/**
							 * Выполняем декомпрессию всех данных
							 */
							do {
								// Устанавливаем буфер для получения результата
								zs.next_out = output.data();
								// Устанавливаем максимальный размер буфера
								zs.avail_out = static_cast <uInt> (output.size());
								// Выполняем декомпрессию данных
								ret = ::inflate(&zs, Z_NO_FLUSH);
								// Если произошла ошибка декомпрессии
								if((ret != Z_OK) && (ret != Z_STREAM_END)){
									// Выполняем очистку буфера данных
									result.clear();
									// Завершаем работу с потоком
									::inflateEnd(&zs);
									// Выводим сообщение об ошибке в лог
									log->print("GZip: Error during data decompression", awh::log_t::flag_t::WARNING);
									// Выходим из функции
									return;
								}
								// Вычисляем количество извлечённых данных
								produced = (output.size() - static_cast <size_t> (zs.avail_out));
								// Если данные извлечены, формируем результирующий буфер
								if(produced > 0)
									// Формируем результирующий буфер бинарных данных
									result.insert(result.end(), output.data(), output.data() + produced);
								// Если место для извлечения данных закончилось
								if(zs.avail_out == 0)
									// Увеличиваем буфер исходящих данных в два раза
									output.resize(output.size() * 2);
							/**
							 * Если данные ещё не извлечены
							 */
							} while(ret == Z_OK);
							// Завершаем работу с потоком
							::inflateEnd(&zs);
						// Выводим сообщение об ошибке в лог
						} else log->print("GZip: Error during data decompression", awh::log_t::flag_t::WARNING);
					} break;
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выполняем очистку блока с результатом
				result.clear();
				// Выводим сообщение об ошибке в лог
				log->print("GZip: %s", awh::log_t::flag_t::CRITICAL, error.what());
			}
		}
	}
	/**
	 * @brief Шаблон функции компрессии/декомпрессии данных в формате raw deflate
	 *
	 * @tparam T выходной контейнер (например, string или vector <char>)
	 */
	template <typename T>
	/**
	 * @brief Функция компрессии/декомпрессии данных в формате raw deflate
	 *
	 * Поддерживает два режима:
	 * - streaming == false: каждый вызов независим (контекст создаётся и уничтожается внутри).
	 * - streaming == true:  контекст (z_stream) переиспользуется между вызовами (должен быть проинициализирован извне).
	 *
	 * Для WebSocket:
	 * - streaming = true  → permessage-deflate с context takeover.
	 * - streaming = false → permessage-deflate без context takeover (каждое сообщение независимо).
	 *
	 * @param buffer    буфер входных данных
	 * @param size      размер входных данных
	 * @param level     уровень компрессии
	 * @param wbits     размер скользящего окна
	 * @param streaming флаг потокового режима (переиспользование контекста)
	 * @param stream    объект потока zlib
	 * @param event     событие выполнения операции
	 * @param result    выходной контейнер
	 * @param log       объект для работы с логами
	 */
	static void deflate(const char * buffer, const size_t size, const uint32_t level, const int16_t wbits, const bool streaming, z_stream & stream, const awh::transform_t::event_t event, T & result, const awh::log_t * log) noexcept {
		// Если буфер данных передан
		if((buffer != nullptr) && (size > 0)){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Выполняем очистку блока с результатом
				result.clear();
				// Локальный поток (если не streaming)
				z_stream local{};
				// Создаем поток Zip
				z_stream * zs = (streaming ? &stream : &local);
				// Создаём выходной буфер с запасом по памяти
				vector <Bytef> output(::max <size_t> (256, size * 2));
				// Результат проверки декомпрессии
				int32_t ret = Z_OK;
				/**
				 * Определяем событие выполнения операции
				 */
				switch(static_cast <uint8_t> (event)){
					// Если необходимо выполнить компрессию данных
					case static_cast <uint8_t> (awh::transform_t::event_t::ENCODE): {
						// Если не используется streaming
						if(!streaming){
							// Инициализируем локальный поток для компрессии
							local.zalloc = Z_NULL;
							local.zfree  = Z_NULL;
							local.opaque = Z_NULL;
							// Инициализируем поток для компрессии
							ret = ::deflateInit2(&local, static_cast <int32_t> (level), Z_DEFLATED, static_cast <int32_t> (-1 * wbits), MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
							// Если инициализация не удалась, выходим
							if(ret != Z_OK){
								// Выводим сообщение об ошибке в лог
								log->print("Deflate: Error during data compression", awh::log_t::flag_t::WARNING);
								// Выводим из функции
								return;
							}
						}
						// Устанавливаем размер входных данных
						zs->avail_in = static_cast <uInt> (size);
						// Устанавливаем буфер входящих данных
						zs->next_in = reinterpret_cast <Bytef *> (const_cast <char *> (buffer));
						// Переменная подсчёта сжатых данных
						size_t produced = 0;
						/**
						 * Сжатие основного тела (без flush)
						 */
						while(zs->avail_in > 0){
							// Устанавливаем выходной буфер данных
							zs->next_out = output.data();
							// Устанавливаем размер выходного буфера данных
							zs->avail_out = static_cast <uInt> (output.size());
							// Выполняем сжатие данных
							ret = ::deflate(zs, Z_NO_FLUSH);
							// Если возникает ошибка
							if(ret != Z_OK){
								// Если не используется streaming
								if(!streaming)
									// Завершаем работу локального потока
									::deflateEnd(&local);
								// Выполняем очистку блока с результатом
								result.clear();
								// Выводим сообщение об ошибке в лог
								log->print("Deflate: Error during data compression", awh::log_t::flag_t::WARNING);
								// Выводим из функции
								return;
							}
							// Вычисляем количество сжатых данных
							produced = (output.size() - static_cast <size_t> (zs->avail_out));
							// Если сжато хоть что-то
							if(produced > 0)
								// Добавляем сжатые данные в результат
								result.insert(result.end(), output.data(), output.data() + produced);
							// Если выходной буфер заполнен, увеличиваем его размер
							if(zs->avail_out == 0)
								// Увеличиваем размер выходного буфера
								output.resize(output.size() * 2);
						}
						/**
						 * Завершение сообщения: один Z_SYNC_FLUSH
						 */
						do {
							// Устанавливаем выходной буфер данных
							zs->next_out = output.data();
							// Устанавливаем размер выходного буфера данных
							zs->avail_out = static_cast <uInt> (output.size());
							// Выполняем сжатие данных
							ret = ::deflate(zs, Z_SYNC_FLUSH);
							// Если возникает ошибка
							if(ret != Z_OK){
								// Если не используется streaming
								if(!streaming)
									// Завершаем работу локального потока
									::deflateEnd(&local);
								// Выполняем очистку блока с результатом
								result.clear();
								// Выводим сообщение об ошибке в лог
								log->print("Deflate: Error during data compression", awh::log_t::flag_t::WARNING);
								// Выводим из функции
								return;
							}
							// Вычисляем количество сжатых данных
							produced = output.size() - static_cast <size_t> (zs->avail_out);
							// Если сжато хоть что-то
							if(produced > 0)
								// Добавляем сжатые данные в результат
								result.insert(result.end(), output.data(), output.data() + produced);
							// Если выходной буфер заполнен, увеличиваем его размер
							if(zs->avail_out == 0)
								// Увеличиваем размер выходного буфера
								output.resize(output.size() * 2);
						/**
						 * Пока выходной буфер полностью заполнен
						 */
						} while(zs->avail_out == 0);
						// Если не используется streaming
						if(!streaming)
							// Завершаем работу локального потока
							::deflateEnd(&local);
					} break;
					// Если необходимо выполнить декомпрессию данных
					case static_cast <uint8_t> (awh::transform_t::event_t::DECODE): {
						// Если не используется streaming
						if(!streaming){
							// Инициализируем локальный поток для компрессии
							local.zalloc = Z_NULL;
							local.zfree  = Z_NULL;
							local.opaque = Z_NULL;
							// Инициализируем поток для декомпрессии
							ret = ::inflateInit2(&local, static_cast <int32_t> (-1 * wbits));
							// Если инициализация не удалась, выходим
							if(ret != Z_OK){
								// Выводим сообщение об ошибке в лог
								log->print("Deflate: Error during data decompression", awh::log_t::flag_t::WARNING);
								// Выводим из функции
								return;
							}
						}
						// Устанавливаем размер входных данных
						zs->avail_in = static_cast <uInt> (size);
						// Устанавливаем буфер входящих данных
						zs->next_in = reinterpret_cast <Bytef *> (const_cast <char *> (buffer));
						// Переменная подсчёта декомпрессированных данных
						size_t produced = 0;
						/**
						 * Декомпрессия данных
						 */
						do {
							// Устанавливаем выходной буфер данных
							zs->next_out = output.data();
							// Устанавливаем размер выходного буфера данных
							zs->avail_out = static_cast <uInt> (output.size());
							// inflate игнорирует flush — всегда Z_NO_FLUSH
							ret = ::inflate(zs, Z_NO_FLUSH);
							// Если возникает ошибка
							if((ret != Z_OK) && (ret != Z_STREAM_END)){
								// Если не используется streaming
								if(!streaming)
									// Завершаем работу локального потока
									::inflateEnd(&local);
								// Выполняем очистку блока с результатом
								result.clear();
								// Выводим сообщение об ошибке в лог
								log->print("Deflate: Error during data decompression", awh::log_t::flag_t::WARNING);
								// Выводим из функции
								return;
							}
							// Вычисляем количество декомпрессированных данных
							produced = (output.size() - static_cast <size_t> (zs->avail_out));
							// Если декомпрессировано хоть что-то
							if(produced > 0)
								// Добавляем декомпрессированные данные в результат
								result.insert(result.end(), output.data(), output.data() + produced);
							// Если выходной буфер заполнен, увеличиваем его размер
							if(zs->avail_out == 0)
								// Увеличиваем размер выходного буфера
								output.resize(output.size() * 2);
						/**
						 * Пока нет конца потока и есть входные данные
						 */
						} while((ret == Z_OK) && (zs->avail_in > 0));
						// Если не используется streaming
						if(!streaming)
							// Завершаем работу локального потока
							::inflateEnd(&local);
					} break;
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выполняем очистку блока с результатом
				result.clear();
				// Выводим сообщение об ошибке в лог
				log->print("Deflate: %s", awh::log_t::flag_t::CRITICAL, error.what());
			}
		}
	}
};

/**
 * @brief Метод установки уровня компрессии
 *
 * @param level уровень компрессии
 */
void awh::Transform::level(const level_t level) noexcept {
	// Выполняем блокировку потоков
	const locker_t <> lock(this->_mtx);
	/**
	 * Определяем переданный уровень компрессии
	 */
	switch(static_cast <uint8_t> (level)){
		// Выполняем установку максимального уровня компрессии
		case static_cast <uint8_t> (level_t::BEST): {
			// Выполняем установку уровня максимальной компрессии Lz4
			this->_level[0] = 0;
			// Выполняем установку уровня компрессии GZip
			this->_level[1] = Z_BEST_COMPRESSION;
			// Выполняем установку уровня максимальной компрессии Zstandard
			this->_level[2] = 100;
			// Выполняем установку уровня компрессии Lizard
			this->_level[3] = LIZARD_MAX_CLEVEL;
		} break;
		// Выполняем установку уровень компрессии на максимальную производительность
		case static_cast <uint8_t> (level_t::SPEED): {
			// Выполняем установку уровня максимальной компрессии Lz4
			this->_level[0] = 3;
			// Выполняем установку уровня компрессии GZip
			this->_level[1] = Z_BEST_SPEED;
			// Выполняем установку уровня максимальной компрессии Zstandard
			this->_level[2] = ZSTD_CLEVEL_DEFAULT;
			// Выполняем установку уровня компрессии Lizard
			this->_level[3] = LIZARD_MIN_CLEVEL;
		} break;
		// Выполняем установку нормального уровня компрессии
		case static_cast <uint8_t> (level_t::NORMAL): {
			// Выполняем установку уровня максимальной компрессии Lz4
			this->_level[0] = 1;
			// Выполняем установку уровня компрессии GZip
			this->_level[1] = Z_DEFAULT_COMPRESSION;
			// Выполняем установку уровня максимальной компрессии Zstandard
			this->_level[2] = 22;
			// Выполняем установку уровня компрессии Lizard
			this->_level[3] = LIZARD_DEFAULT_CLEVEL;
		} break;
	}
}
/**
 * @brief Метод инициализации AES шифрования
 *
 * @param cipher тип шифрования (AES128, AES192, AES256)
 * @return       результат инициализации
 */
bool awh::Transform::cipher(const cipher_t cipher) noexcept {
	// Результат работы функции
	bool result = false;
	// Формируем массивы для шифрования
	vector <uint8_t> ivec, key;
	// Создаем тип шифрования
	const EVP_CIPHER * evp = ::EVP_enc_null();
	/**
	 * Определяем длину шифрования
	 */
	switch(static_cast <uint16_t> (cipher)){
		// Устанавливаем шифрование в 128
		case static_cast <uint16_t> (cipher_t::AES128): {
			// Устанавливаем размер массива IVEC
			ivec.resize(8, 0);
			// Устанавливаем размер массива KEY
			key.resize(16, 0);
			// Устанавливаем функцию шифрования
			evp = ::EVP_aes_128_ecb();
		} break;
		// Устанавливаем шифрование в 192
		case static_cast <uint16_t> (cipher_t::AES192): {
			// Устанавливаем размер массива IVEC
			ivec.resize(12, 0);
			// Устанавливаем размер массива KEY
			key.resize(24, 0);
			// Устанавливаем функцию шифрования
			evp = ::EVP_aes_192_ecb();
		} break;
		// Устанавливаем шифрование в 256
		case static_cast <uint16_t> (cipher_t::AES256): {
			// Устанавливаем размер массива IVEC
			ivec.resize(16, 0);
			// Устанавливаем размер массива KEY
			key.resize(32, 0);
			// Устанавливаем функцию шифрования
			evp = ::EVP_aes_256_ecb();
		} break;
		// Если ничего не выбрано, сбрасываем
		default: return false;
	}
	// Создаем контекст
	EVP_CIPHER_CTX * ctx = ::EVP_CIPHER_CTX_new();
	// Если контекст для шифрования удачно инициализирован
	if((result = (ctx != nullptr))){
		// Выполняем блокировку потоков
		const locker_t <> lock(this->_mtx);
		// Объявляем структуру ключа
		this->_crypto.aes.key = AES_KEY{};
		// Выполняем инициализацию ключа шифрования
		std::any_cast <AES_KEY &> (this->_crypto.aes.key) = {{0},0};
		// Привязываем контекст к типу шифрования
		::EVP_EncryptInit_ex(ctx, evp, nullptr, nullptr, nullptr);
		/*
		// Выделяем нужное количество памяти
		vector <uint8_t> iv(::EVP_CIPHER_CTX_iv_length(ctx), 0);
		vector <uint8_t> key(::EVP_CIPHER_CTX_key_length(ctx), 0);
		*/
		// Выполняем инициализацию ключа
		const int32_t ok = EVP_BytesToKey(
			evp, ::EVP_sha256(),
			(this->_crypto.salt.empty() ? nullptr : reinterpret_cast <uint8_t *> (const_cast <transform_t *> (this)->_crypto.salt.data())),
			reinterpret_cast <uint8_t *> (const_cast <transform_t *> (this)->_crypto.password.data()),
			this->_crypto.password.length(), this->_crypto.rounds, key.data(), ivec.data()
		);
		// Очищаем контекст
		::EVP_CIPHER_CTX_free(ctx);
		// Если инициализация не произошла
		if(!(result = (ok != 0)))
			// Выходим из функции
			return result;
		// Устанавливаем ключ шифрования
		if(!(result = (::AES_set_encrypt_key(key.data(), key.size() * 8, &std::any_cast <AES_KEY &> (this->_crypto.aes.key)) == 0)))
			// Выходим из функции
			return result;
		// Обнуляем номер блока шифрования
		this->_crypto.aes.num = 0;
		// Заполняем половину структуры нулями
		::memset(this->_crypto.aes.ivec, 0, 16);
		// Копируем данные шифрования
		::memcpy(this->_crypto.aes.ivec, ivec.data(), ivec.size());
		// Выполняем шифрование
		// ::AES_encrypt(this->_crypto.aes.ivec, this->_crypto.aes.count, &std::any_cast <AES_KEY &> (this->_crypto.aes.key));
	// Выводим сообщение об ошибке
	} else this->_log->print("%s", log_t::flag_t::CRITICAL, "Context for encryption/decryption could not be initialized");
	// Сообщаем что всё удачно
	return result;
}
/**
 * @brief Метод установки количества раундов шифрования
 *
 * @param round количество раундов шифрования
 */
void awh::Transform::roundAES(const int32_t round) noexcept {
	// Устанавливаем количество раундов шифрования
	this->_crypto.rounds = round;
}
/**
 * @brief Метод установки соли шифрования
 *
 * @param salt соль для шифрования
 */
void awh::Transform::salt(const string & salt) noexcept {
	// Устанавливаем соль шифрования
	this->_crypto.salt = salt;
}
/**
 * @brief Метод установки пароля шифрования
 *
 * @param password пароль шифрования
 */
void awh::Transform::password(const string & password) noexcept {
	// Устанавливаем пароль шифрования
	this->_crypto.password = password;
}
/**
 * @brief Метод установки безопасности работы потоков
 *
 * @param mode режим безопасности потоков
 */
void awh::Transform::threadSafety(const mode_t mode) noexcept {
	// Устанавливаем режим безопасности потоков
	this->_mtx.enabled = (mode == mode_t::ENABLED);
}
/**
 * @brief Метод установки размера скользящего окна
 *
 * @param wbits размер скользящего окна
 */
void awh::Transform::wbitsGZip(const int16_t wbits) noexcept {
	// Устанавливаем размер скользящего окна
	this->_gzip.wbits = wbits;
	// Выполняем пересборку контекстов LZ77 для компрессии
	this->takeoverGZip(event_t::ENCODE, this->_gzip.takeover.compress.load(std::memory_order_acquire));
	// Выполняем пересборку контекстов LZ77 для декомпрессии
	this->takeoverGZip(event_t::DECODE, this->_gzip.takeover.decompress.load(std::memory_order_acquire));
}
/**
 * @brief Метод включения/отключения флага переиспользования контекста компрессии/декомпрессии
 *
 * @param event событие выполнения операции
 * @param flag  флаг переиспользования контекста компрессии/декомпрессии
 */
void awh::Transform::takeoverGZip(const event_t event, const bool flag) noexcept {
	/**
	 * Определяем событие выполнения операции
	 */
	switch(static_cast <uint8_t> (event)){
		// Выполняем установку флага переиспользования контекста компрессии
		case static_cast <uint8_t> (event_t::ENCODE): {
			// Выполняем блокировку потоков
			const locker_t <> lock(this->_mtx);
			// Извлекаем буфер GZip
			z_stream & buffer = std::any_cast <z_stream &> (this->_gzip.buffer.compress);
			// Если уже выделена память для компрессора
			if(this->_gzip.takeover.compress.load(std::memory_order_acquire))
				// Очищаем выделенную память для компрессора
				::deflateEnd(&buffer);
			// Если флаг установлен
			if(flag){
				// Заполняем его нулями потока для компрессора
				::memset(&buffer, 0, sizeof(buffer));
				// Обнуляем структуру потока для компрессора
				buffer.zalloc = Z_NULL;
				buffer.zfree  = Z_NULL;
				buffer.opaque = Z_NULL;
				// Если поток инициализировать не удалось, выходим
				if(::deflateInit2(&buffer, this->_level[1], Z_DEFLATED, static_cast <int32_t> (-1 * this->_gzip.wbits), MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY) != Z_OK){
					// Выводим сообщение об ошибке
					this->_log->print("Deflate stream is not create", log_t::flag_t::CRITICAL);
					/**
					 * Для операционной системы не являющейся MS Windows
					 */
					#if !_WIN32 && !_WIN64
						// Выходим из приложения
						::raise(SIGINT);
						// Выходим из функции
						return;
					/**
					 * Для операционной системы MS Windows
					 */
					#else
						// Выходим из приложения
						::exit(EXIT_FAILURE);
					#endif
				}
			}
			// Устанавливаем переданный флаг
			this->_gzip.takeover.compress.store(flag, std::memory_order_release);
		} break;
		// Выполняем установку флага переиспользования контекста декомпрессии
		case static_cast <uint8_t> (event_t::DECODE): {
			// Выполняем блокировку потоков
			const locker_t <> lock(this->_mtx);
			// Извлекаем буфер GZip
			z_stream & buffer = std::any_cast <z_stream &> (this->_gzip.buffer.decompress);
			// Если уже выделена память для декомпрессора
			if(this->_gzip.takeover.decompress.load(std::memory_order_acquire))
				// Очищаем выделенную память для декомпрессора
				::inflateEnd(&buffer);
			// Если флаг установлен
			if(flag){
				// Заполняем его нулями потока для декомпрессора
				::memset(&buffer, 0, sizeof(buffer));
				// Обнуляем структуру потока для декомпрессора
				buffer.avail_in = 0;
				// Инициализируем остальные поля структуры потока для декомпрессора
				buffer.zalloc  = Z_NULL;
				buffer.zfree   = Z_NULL;
				buffer.opaque  = Z_NULL;
				buffer.next_in = Z_NULL;
				// Если поток инициализировать не удалось, выходим
				if(::inflateInit2(&buffer, static_cast <int32_t> (-1 * this->_gzip.wbits)) != Z_OK){
					// Выводим сообщение об ошибке
					this->_log->print("Inflate stream is not create", log_t::flag_t::CRITICAL);
					/**
					 * Для операционной системы не являющейся MS Windows
					 */
					#if !_WIN32 && !_WIN64
						// Выходим из приложения
						::raise(SIGINT);
						// Выходим из функции
						return;
					/**
					 * Для операционной системы MS Windows
					 */
					#else
						// Выходим из приложения
						::exit(EXIT_FAILURE);
					#endif
				}
			}
			// Устанавливаем переданный флаг
			this->_gzip.takeover.decompress.store(flag, std::memory_order_release);
		} break;
	}
}
/**
 * @brief Метод преобразования 128-битного хэша в 64-битный
 *
 * @param hash 128-битный хэш
 * @return     64-битный хэш
 */
uint64_t awh::Transform::hash128to64(const uint128_t & hash) const noexcept {
	// Буфер для хранения 128-битного хэша
	uint128 buffer;
	// Копируем первую часть результата в выходной буфер
	::memcpy(&buffer.first, &hash[0], 8);
	// Копируем вторую часть результата в выходной буфер
	::memcpy(&buffer.second, &hash[8], 8);
	// Возвращаем 64-битный хэш
	return ::Hash128to64(buffer);
}
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
auto awh::Transform::hashing(const string & text) const noexcept -> T {
	// Результат работы функции
	T result;
	/**
	 * Определяем размер типа возвращаемого результата
	 */
	switch(sizeof(T)){
		// Если необходимо вернуть 32 битный хэш
		case 4: {
			// Возвращаем 32 битный хэш
			const auto & hash = ::CityHash32(text.data(), text.size());
			// Копируем результат в выходной буфер
			::memcpy(&result, &hash, sizeof(hash));
		} break;
		// Если необходимо вернуть 64 битный хэш
		case 8: {
			// Возвращаем 64 битный хэш
			const auto & hash = ::CityHash64(text.data(), text.size());
			// Копируем результат в выходной буфер
			::memcpy(&result, &hash, sizeof(hash));
		} break;
		// Если необходимо вернуть 128 битный хэш
		case 16: {
			// Возвращаем 128 битный хэш
			const auto & hash = ::CityHash128(text.data(), text.size());
			// Копируем первую часть результата в выходной буфер
			::memcpy(reinterpret_cast <char *> (&result), &hash.first, sizeof(hash.first));
			// Копируем вторую часть результата в выходной буфер
			::memcpy(reinterpret_cast <char *> (&result) + 8, &hash.second, sizeof(hash.second));
		} break;
	}
	// Возвращаем результат по умолчанию
	return result;
}
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 32 бит
 *
 */
template uint32_t awh::Transform::hashing <uint32_t> (const string &) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 64 бит
 *
 */
template uint64_t awh::Transform::hashing <uint64_t> (const string &) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 128 бит
 *
 */
template awh::Transform::uint128_t awh::Transform::hashing <awh::Transform::uint128_t> (const string &) const noexcept;
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
auto awh::Transform::hashing(const vector <char> & buffer) const noexcept -> T {
	// Результат работы функции
	T result;
	/**
	 * Определяем размер типа возвращаемого результата
	 */
	switch(sizeof(T)){
		// Если необходимо вернуть 32 битный хэш
		case 4: {
			// Возвращаем 32 битный хэш
			const auto & hash = ::CityHash32(buffer.data(), buffer.size());
			// Копируем результат в выходной буфер
			::memcpy(&result, &hash, sizeof(hash));
		} break;
		// Если необходимо вернуть 64 битный хэш
		case 8: {
			// Возвращаем 64 битный хэш
			const auto & hash = ::CityHash64(buffer.data(), buffer.size());
			// Копируем результат в выходной буфер
			::memcpy(&result, &hash, sizeof(hash));
		} break;
		// Если необходимо вернуть 128 битный хэш
		case 16: {
			// Возвращаем 128 битный хэш
			const auto & hash = ::CityHash128(buffer.data(), buffer.size());
			// Копируем первую часть результата в выходной буфер
			::memcpy(reinterpret_cast <char *> (&result), &hash.first, sizeof(hash.first));
			// Копируем вторую часть результата в выходной буфер
			::memcpy(reinterpret_cast <char *> (&result) + 8, &hash.second, sizeof(hash.second));
		} break;
	}
	// Возвращаем результат по умолчанию
	return result;
}
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 32 бит
 *
 */
template uint32_t awh::Transform::hashing <uint32_t> (const vector <char> &) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 64 бит
 *
 */
template uint64_t awh::Transform::hashing <uint64_t> (const vector <char> &) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 128 бит
 *
 */
template uint128 awh::Transform::hashing <uint128> (const vector <char> &) const noexcept;
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
auto awh::Transform::hashing(const vector <uint8_t> & buffer) const noexcept -> T {
	// Результат работы функции
	T result;
	/**
	 * Определяем размер типа возвращаемого результата
	 */
	switch(sizeof(T)){
		// Если необходимо вернуть 32 битный хэш
		case 4: {
			// Возвращаем 32 битный хэш
			const auto & hash = ::CityHash32(reinterpret_cast <const char *> (buffer.data()), buffer.size());
			// Копируем результат в выходной буфер
			::memcpy(&result, &hash, sizeof(hash));
		} break;
		// Если необходимо вернуть 64 битный хэш
		case 8: {
			// Возвращаем 64 битный хэш
			const auto & hash = ::CityHash64(reinterpret_cast <const char *> (buffer.data()), buffer.size());
			// Копируем результат в выходной буфер
			::memcpy(&result, &hash, sizeof(hash));
		} break;
		// Если необходимо вернуть 128 битный хэш
		case 16: {
			// Возвращаем 128 битный хэш
			const auto & hash = ::CityHash128(reinterpret_cast <const char *> (buffer.data()), buffer.size());
			// Копируем первую часть результата в выходной буфер
			::memcpy(reinterpret_cast <uint8_t *> (&result), &hash.first, sizeof(hash.first));
			// Копируем вторую часть результата в выходной буфер
			::memcpy(reinterpret_cast <uint8_t *> (&result) + 8, &hash.second, sizeof(hash.second));
		} break;
	}
	// Возвращаем результат по умолчанию
	return result;
}
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 32 бит
 *
 */
template uint32_t awh::Transform::hashing <uint32_t> (const vector <uint8_t> &) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 64 бит
 *
 */
template uint64_t awh::Transform::hashing <uint64_t> (const vector <uint8_t> &) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 128 бит
 *
 */
template uint128 awh::Transform::hashing <uint128> (const vector <uint8_t> &) const noexcept;
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
auto awh::Transform::hashing(const void * buffer, const size_t size) const noexcept -> T {
	// Результат работы функции
	T result;
	/**
	 * Определяем размер типа возвращаемого результата
	 */
	switch(sizeof(T)){
		// Если необходимо вернуть 32 битный хэш
		case 4: {
			// Возвращаем 32 битный хэш
			const auto & hash = ::CityHash32(static_cast <const char *> (buffer), size);
			// Копируем результат в выходной буфер
			::memcpy(&result, &hash, sizeof(hash));
		} break;
		// Если необходимо вернуть 64 битный хэш
		case 8: {
			// Возвращаем 64 битный хэш
			const auto & hash = ::CityHash64(static_cast <const char *> (buffer), size);
			// Копируем результат в выходной буфер
			::memcpy(&result, &hash, sizeof(hash));
		} break;
		// Если необходимо вернуть 128 битный хэш
		case 16: {
			// Возвращаем 128 битный хэш
			const auto & hash = ::CityHash128(static_cast <const char *> (buffer), size);
			// Копируем первую часть результата в выходной буфер
			::memcpy(reinterpret_cast <char *> (&result), &hash.first, sizeof(hash.first));
			// Копируем вторую часть результата в выходной буфер
			::memcpy(reinterpret_cast <char *> (&result) + 8, &hash.second, sizeof(hash.second));
		} break;
	}
	// Возвращаем результат по умолчанию
	return result;
}
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 32 бит
 *
 */
template uint32_t awh::Transform::hashing <uint32_t> (const void *, const size_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 64 бит
 *
 */
template uint64_t awh::Transform::hashing <uint64_t> (const void *, const size_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 128 бит
 *
 */
template uint128 awh::Transform::hashing <uint128> (const void *, const size_t) const noexcept;
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
auto awh::Transform::hashing(const string & text, const T seed) const noexcept -> T {
	// Результат работы функции
	T result;
	/**
	 * Определяем размер типа возвращаемого результата
	 */
	switch(sizeof(T)){
		// Если необходимо вернуть 32 битный хэш
		case 4: {
			// Возвращаем 32 битный хэш
			const auto & hash = ::CityHash32(text.data(), text.size());
			// Копируем результат в выходной буфер
			::memcpy(&result, &hash, sizeof(hash));
		} break;
		// Если необходимо вернуть 64 битный хэш
		case 8: {
			// 64-битный ключевой буфер
			uint64_t key = 0;
			// Копируем первую часть результата в выходной буфер
			::memcpy(&key, &seed, 8);
			// Возвращаем 64 битный хэш
			const auto & hash = ::CityHash64WithSeed(text.data(), text.size(), key);
			// Копируем результат в выходной буфер
			::memcpy(&result, &hash, sizeof(hash));
		} break;
		// Если необходимо вернуть 128 битный хэш
		case 16: {
			// 128-битный ключевой буфер
			uint128 key;
			// Копируем первую часть результата в выходной буфер
			::memcpy(&key.first, reinterpret_cast <const char *> (&seed), 8);
			// Копируем вторую часть результата в выходной буфер
			::memcpy(&key.second, reinterpret_cast <const char *> (&seed) + 8, 8);
			// Возвращаем 128 битный хэш
			const auto & hash = ::CityHash128WithSeed(text.data(), text.size(), key);
			// Копируем первую часть результата в выходной буфер
			::memcpy(reinterpret_cast <char *> (&result), &hash.first, sizeof(hash.first));
			// Копируем вторую часть результата в выходной буфер
			::memcpy(reinterpret_cast <char *> (&result) + 8, &hash.second, sizeof(hash.second));
		} break;
	}
	// Возвращаем результат по умолчанию
	return result;
}
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 32 бит
 *
 */
template uint32_t awh::Transform::hashing <uint32_t> (const string &, const uint32_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 64 бит
 *
 */
template uint64_t awh::Transform::hashing <uint64_t> (const string &, const uint64_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 128 бит
 *
 */
template uint128 awh::Transform::hashing <uint128> (const string &, const uint128) const noexcept;
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
auto awh::Transform::hashing(const vector <char> & buffer, const T seed) const noexcept -> T {
	// Результат работы функции
	T result;
	/**
	 * Определяем размер типа возвращаемого результата
	 */
	switch(sizeof(T)){
		// Если необходимо вернуть 32 битный хэш
		case 4: {
			// Возвращаем 32 битный хэш
			const auto & hash = ::CityHash32(buffer.data(), buffer.size());
			// Копируем результат в выходной буфер
			::memcpy(&result, &hash, sizeof(hash));
		} break;
		// Если необходимо вернуть 64 битный хэш
		case 8: {
			// 64-битный ключевой буфер
			uint64_t key = 0;
			// Копируем первую часть результата в выходной буфер
			::memcpy(&key, &seed, 8);
			// Возвращаем 64 битный хэш
			const auto & hash = ::CityHash64WithSeed(buffer.data(), buffer.size(), key);
			// Копируем результат в выходной буфер
			::memcpy(&result, &hash, sizeof(hash));
		} break;
		// Если необходимо вернуть 128 битный хэш
		case 16: {
			// 128-битный ключевой буфер
			uint128 key;
			// Копируем первую часть результата в выходной буфер
			::memcpy(&key.first, reinterpret_cast <const char *> (&seed), 8);
			// Копируем вторую часть результата в выходной буфер
			::memcpy(&key.second, reinterpret_cast <const char *> (&seed) + 8, 8);
			// Возвращаем 128 битный хэш
			const auto & hash = ::CityHash128WithSeed(buffer.data(), buffer.size(), key);
			// Копируем первую часть результата в выходной буфер
			::memcpy(reinterpret_cast <char *> (&result), &hash.first, sizeof(hash.first));
			// Копируем вторую часть результата в выходной буфер
			::memcpy(reinterpret_cast <char *> (&result) + 8, &hash.second, sizeof(hash.second));
		} break;
	}
	// Возвращаем результат по умолчанию
	return result;
}
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 32 бит
 *
 */
template uint32_t awh::Transform::hashing <uint32_t> (const vector <char> &, const uint32_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 64 бит
 *
 */
template uint64_t awh::Transform::hashing <uint64_t> (const vector <char> &, const uint64_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 128 бит
 *
 */
template uint128 awh::Transform::hashing <uint128> (const vector <char> &, const uint128) const noexcept;
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
auto awh::Transform::hashing(const vector <uint8_t> & buffer, const T seed) const noexcept -> T {
	// Результат работы функции
	T result;
	/**
	 * Определяем размер типа возвращаемого результата
	 */
	switch(sizeof(T)){
		// Если необходимо вернуть 32 битный хэш
		case 4: {
			// Возвращаем 32 битный хэш
			const auto & hash = ::CityHash32(reinterpret_cast <const char *> (buffer.data()), buffer.size());
			// Копируем результат в выходной буфер
			::memcpy(&result, &hash, sizeof(hash));
		} break;
		// Если необходимо вернуть 64 битный хэш
		case 8: {
			// 64-битный ключевой буфер
			uint64_t key = 0;
			// Копируем первую часть результата в выходной буфер
			::memcpy(&key, &seed, 8);
			// Возвращаем 64 битный хэш
			const auto & hash = ::CityHash64WithSeed(reinterpret_cast <const char *> (buffer.data()), buffer.size(), key);
			// Копируем результат в выходной буфер
			::memcpy(&result, &hash, sizeof(hash));
		} break;
		// Если необходимо вернуть 128 битный хэш
		case 16: {
			// 128-битный ключевой буфер
			uint128 key;
			// Копируем первую часть результата в выходной буфер
			::memcpy(&key.first, reinterpret_cast <const char *> (&seed), 8);
			// Копируем вторую часть результата в выходной буфер
			::memcpy(&key.second, reinterpret_cast <const char *> (&seed) + 8, 8);
			// Возвращаем 128 битный хэш
			const auto & hash = ::CityHash128WithSeed(reinterpret_cast <const char *> (buffer.data()), buffer.size(), key);
			// Копируем первую часть результата в выходной буфер
			::memcpy(reinterpret_cast <uint8_t *> (&result), &hash.first, sizeof(hash.first));
			// Копируем вторую часть результата в выходной буфер
			::memcpy(reinterpret_cast <uint8_t *> (&result) + 8, &hash.second, sizeof(hash.second));
		} break;
	}
	// Возвращаем результат по умолчанию
	return result;
}
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 32 бит
 *
 */
template uint32_t awh::Transform::hashing <uint32_t> (const vector <uint8_t> &, const uint32_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 64 бит
 *
 */
template uint64_t awh::Transform::hashing <uint64_t> (const vector <uint8_t> &, const uint64_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 128 бит
 *
 */
template uint128 awh::Transform::hashing <uint128> (const vector <uint8_t> &, const uint128) const noexcept;
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
auto awh::Transform::hashing(const void * buffer, const size_t size, const T seed) const noexcept -> T {
	// Результат работы функции
	T result;
	/**
	 * Определяем размер типа возвращаемого результата
	 */
	switch(sizeof(T)){
		// Если необходимо вернуть 32 битный хэш
		case 4: {
			// Возвращаем 32 битный хэш
			const auto & hash = ::CityHash32(static_cast <const char *> (buffer), size);
			// Копируем результат в выходной буфер
			::memcpy(&result, &hash, sizeof(hash));
		} break;
		// Если необходимо вернуть 64 битный хэш
		case 8: {
			// 64-битный ключевой буфер
			uint64_t key = 0;
			// Копируем первую часть результата в выходной буфер
			::memcpy(&key, &seed, 8);
			// Возвращаем 64 битный хэш
			const auto & hash = ::CityHash64WithSeed(static_cast <const char *> (buffer), size, key);
			// Копируем результат в выходной буфер
			::memcpy(&result, &hash, sizeof(hash));
		} break;
		// Если необходимо вернуть 128 битный хэш
		case 16: {
			// 128-битный ключевой буфер
			uint128 key;
			// Копируем первую часть результата в выходной буфер
			::memcpy(&key.first, reinterpret_cast <const char *> (&seed), 8);
			// Копируем вторую часть результата в выходной буфер
			::memcpy(&key.second, reinterpret_cast <const char *> (&seed) + 8, 8);
			// Возвращаем 128 битный хэш
			const auto & hash = ::CityHash128WithSeed(static_cast <const char *> (buffer), size, key);
			// Копируем первую часть результата в выходной буфер
			::memcpy(reinterpret_cast <char *> (&result), &hash.first, sizeof(hash.first));
			// Копируем вторую часть результата в выходной буфер
			::memcpy(reinterpret_cast <char *> (&result) + 8, &hash.second, sizeof(hash.second));
		} break;
	}
	// Возвращаем результат по умолчанию
	return result;
}
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 32 бит
 *
 */
template uint32_t awh::Transform::hashing <uint32_t> (const void *, const size_t, const uint32_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 64 бит
 *
 */
template uint64_t awh::Transform::hashing <uint64_t> (const void *, const size_t, const uint64_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 128 бит
 *
 */
template uint128 awh::Transform::hashing <uint128> (const void *, const size_t, const uint128) const noexcept;
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
auto awh::Transform::hashing(const string & text, const T seed1, const T seed2) const noexcept -> T {
	// Результат работы функции
	T result;
	/**
	 * Определяем размер типа возвращаемого результата
	 */
	switch(sizeof(T)){
		// Если необходимо вернуть 32 битный хэш
		case 4: {
			// Возвращаем 32 битный хэш
			const auto & hash = ::CityHash32(text.data(), text.size());
			// Копируем результат в выходной буфер
			::memcpy(&result, &hash, sizeof(hash));
		} break;
		// Если необходимо вернуть 64 битный хэш
		case 8: {
			// 64-битный ключевой буфер
			uint64_t key1 = 0, key2 = 0;
			// Копируем первую часть результата в выходной буфер
			::memcpy(&key1, &seed1, 8);
			// Копируем вторую часть результата в выходной буфер
			::memcpy(&key2, &seed2, 8);
			// Возвращаем 64 битный хэш
			const auto & hash = ::CityHash64WithSeeds(text.data(), text.size(), key1, key2);
			// Копируем результат в выходной буфер
			::memcpy(&result, &hash, sizeof(hash));
		} break;
		// Если необходимо вернут1ь 128 битный хэш
		case 16: {
			// 128-битный ключевой буфер
			uint128 key;
			// Копируем первую часть результата в выходной буфер
			::memcpy(&key.first, reinterpret_cast <const char *> (&seed1), 8);
			// Копируем вторую часть результата в выходной буфер
			::memcpy(&key.second, reinterpret_cast <const char *> (&seed1) + 8, 8);
			// Возвращаем 128 битный хэш
			const auto & hash = ::CityHash128WithSeed(text.data(), text.size(), key);
			// Копируем первую часть результата в выходной буфер
			::memcpy(reinterpret_cast <char *> (&result), &hash.first, sizeof(hash.first));
			// Копируем вторую часть результата в выходной буфер
			::memcpy(reinterpret_cast <char *> (&result) + 8, &hash.second, sizeof(hash.second));
		} break;
	}
	// Возвращаем результат по умолчанию
	return result;
}
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 32 бит
 *
 */
template uint32_t awh::Transform::hashing <uint32_t> (const string &, const uint32_t, const uint32_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 64 бит
 *
 */
template uint64_t awh::Transform::hashing <uint64_t> (const string &, const uint64_t, const uint64_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 128 бит
 *
 */
template uint128 awh::Transform::hashing <uint128> (const string &, const uint128, const uint128) const noexcept;
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
auto awh::Transform::hashing(const vector <char> & buffer, const T seed1, const T seed2) const noexcept -> T {
	// Результат работы функции
	T result;
	/**
	 * Определяем размер типа возвращаемого результата
	 */
	switch(sizeof(T)){
		// Если необходимо вернуть 32 битный хэш
		case 4: {
			// Возвращаем 32 битный хэш
			const auto & hash = ::CityHash32(buffer.data(), buffer.size());
			// Копируем результат в выходной буфер
			::memcpy(&result, &hash, sizeof(hash));
		} break;
		// Если необходимо вернуть 64 битный хэш
		case 8: {
			// 64-битный ключевой буфер
			uint64_t key1 = 0, key2 = 0;
			// Копируем первую часть результата в выходной буфер
			::memcpy(&key1, &seed1, 8);
			// Копируем вторую часть результата в выходной буфер
			::memcpy(&key2, &seed2, 8);
			// Возвращаем 64 битный хэш
			const auto & hash = ::CityHash64WithSeeds(buffer.data(), buffer.size(), key1, key2);
			// Копируем результат в выходной буфер
			::memcpy(&result, &hash, sizeof(hash));
		} break;
		// Если необходимо вернут1ь 128 битный хэш
		case 16: {
			// 128-битный ключевой буфер
			uint128 key;
			// Копируем первую часть результата в выходной буфер
			::memcpy(&key.first, reinterpret_cast <const char *> (&seed1), 8);
			// Копируем вторую часть результата в выходной буфер
			::memcpy(&key.second, reinterpret_cast <const char *> (&seed1) + 8, 8);
			// Возвращаем 128 битный хэш
			const auto & hash = ::CityHash128WithSeed(buffer.data(), buffer.size(), key);
			// Копируем первую часть результата в выходной буфер
			::memcpy(reinterpret_cast <char *> (&result), &hash.first, sizeof(hash.first));
			// Копируем вторую часть результата в выходной буфер
			::memcpy(reinterpret_cast <char *> (&result) + 8, &hash.second, sizeof(hash.second));
		} break;
	}
	// Возвращаем результат по умолчанию
	return result;
}
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 32 бит
 *
 */
template uint32_t awh::Transform::hashing <uint32_t> (const vector <char> &, const uint32_t, const uint32_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 64 бит
 *
 */
template uint64_t awh::Transform::hashing <uint64_t> (const vector <char> &, const uint64_t, const uint64_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 128 бит
 *
 */
template uint128 awh::Transform::hashing <uint128> (const vector <char> &, const uint128, const uint128) const noexcept;
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
auto awh::Transform::hashing(const vector <uint8_t> & buffer, const T seed1, const T seed2) const noexcept -> T {
	// Результат работы функции
	T result;
	/**
	 * Определяем размер типа возвращаемого результата
	 */
	switch(sizeof(T)){
		// Если необходимо вернуть 32 битный хэш
		case 4: {
			// Возвращаем 32 битный хэш
			const auto & hash = ::CityHash32(reinterpret_cast <const char *> (buffer.data()), buffer.size());
			// Копируем результат в выходной буфер
			::memcpy(&result, &hash, sizeof(hash));
		} break;
		// Если необходимо вернуть 64 битный хэш
		case 8: {
			// 64-битный ключевой буфер
			uint64_t key1 = 0, key2 = 0;
			// Копируем первую часть результата в выходной буфер
			::memcpy(&key1, &seed1, 8);
			// Копируем вторую часть результата в выходной буфер
			::memcpy(&key2, &seed2, 8);
			// Возвращаем 64 битный хэш
			const auto & hash = ::CityHash64WithSeeds(reinterpret_cast <const char *> (buffer.data()), buffer.size(), key1, key2);
			// Копируем результат в выходной буфер
			::memcpy(&result, &hash, sizeof(hash));
		} break;
		// Если необходимо вернут1ь 128 битный хэш
		case 16: {
			// 128-битный ключевой буфер
			uint128 key;
			// Копируем первую часть результата в выходной буфер
			::memcpy(&key.first, reinterpret_cast <const char *> (&seed1), 8);
			// Копируем вторую часть результата в выходной буфер
			::memcpy(&key.second, reinterpret_cast <const char *> (&seed1) + 8, 8);
			// Возвращаем 128 битный хэш
			const auto & hash = ::CityHash128WithSeed(reinterpret_cast <const char *> (buffer.data()), buffer.size(), key);
			// Копируем первую часть результата в выходной буфер
			::memcpy(reinterpret_cast <uint8_t *> (&result), &hash.first, sizeof(hash.first));
			// Копируем вторую часть результата в выходной буфер
			::memcpy(reinterpret_cast <uint8_t *> (&result) + 8, &hash.second, sizeof(hash.second));
		} break;
	}
	// Возвращаем результат по умолчанию
	return result;
}
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 32 бит
 *
 */
template uint32_t awh::Transform::hashing <uint32_t> (const vector <uint8_t> &, const uint32_t, const uint32_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 64 бит
 *
 */
template uint64_t awh::Transform::hashing <uint64_t> (const vector <uint8_t> &, const uint64_t, const uint64_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 128 бит
 *
 */
template uint128 awh::Transform::hashing <uint128> (const vector <uint8_t> &, const uint128, const uint128) const noexcept;
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
auto awh::Transform::hashing(const void * buffer, const size_t size, const T seed1, const T seed2) const noexcept -> T {
	// Результат работы функции
	T result;
	/**
	 * Определяем размер типа возвращаемого результата
	 */
	switch(sizeof(T)){
		// Если необходимо вернуть 32 битный хэш
		case 4: {
			// Возвращаем 32 битный хэш
			const auto & hash = ::CityHash32(reinterpret_cast <const char *> (buffer), size);
			// Копируем результат в выходной буфер
			::memcpy(&result, &hash, sizeof(hash));
		} break;
		// Если необходимо вернуть 64 битный хэш
		case 8: {
			// 64-битный ключевой буфер
			uint64_t key1 = 0, key2 = 0;
			// Копируем первую часть результата в выходной буфер
			::memcpy(&key1, &seed1, 8);
			// Копируем вторую часть результата в выходной буфер
			::memcpy(&key2, &seed2, 8);
			// Возвращаем 64 битный хэш
			const auto & hash = ::CityHash64WithSeeds(reinterpret_cast <const char *> (buffer), size, key1, key2);
			// Копируем результат в выходной буфер
			::memcpy(&result, &hash, sizeof(hash));
		} break;
		// Если необходимо вернут1ь 128 битный хэш
		case 16: {
			// 128-битный ключевой буфер
			uint128 key;
			// Копируем первую часть результата в выходной буфер
			::memcpy(&key.first, reinterpret_cast <const char *> (&seed1), 8);
			// Копируем вторую часть результата в выходной буфер
			::memcpy(&key.second, reinterpret_cast <const char *> (&seed1) + 8, 8);
			// Возвращаем 128 битный хэш
			const auto & hash = ::CityHash128WithSeed(reinterpret_cast <const char *> (buffer), size, key);
			// Копируем первую часть результата в выходной буфер
			::memcpy(reinterpret_cast <char *> (&result), &hash.first, sizeof(hash.first));
			// Копируем вторую часть результата в выходной буфер
			::memcpy(reinterpret_cast <char *> (&result) + 8, &hash.second, sizeof(hash.second));
		} break;
	}
	// Возвращаем результат по умолчанию
	return result;
}
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 32 бит
 *
 */
template uint32_t awh::Transform::hashing <uint32_t> (const void *, const size_t, const uint32_t, const uint32_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 64 бит
 *
 */
template uint64_t awh::Transform::hashing <uint64_t> (const void *, const size_t, const uint64_t, const uint64_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в число 128 бит
 *
 */
template uint128 awh::Transform::hashing <uint128> (const void *, const size_t, const uint128, const uint128) const noexcept;
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
auto awh::Transform::hashing(const string & text, const hash_t hash) const noexcept -> T {
	// Результат работы функции
	T result;
	// Если текст передан
	if(!text.empty())
		// Выполняем хэширование
		this->hashing(text, hash, result);
	// Выводим результат
	return result;
}
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в строку
 *
 */
template string awh::Transform::hashing <string> (const string &, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в буфер
 *
 */
template vector <char> awh::Transform::hashing <vector <char>> (const string &, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Transform::hashing <vector <uint8_t>> (const string &, const hash_t) const noexcept;
/**
 * @brief Метод хэширования текста
 *
 * @param text   текст для хэширования
 * @param hash   тип хэш-суммы
 * @param result строка куда следует положить результат
 */
void awh::Transform::hashing(const string & text, const hash_t hash, string & result) const noexcept {
	// Если текст для хэширования передан
	if(!text.empty()){
		// Выполняем хэширование
		driver::hashing(text, hash, result, this->_log);
		// Если хэширование не вышло
		if(result.empty())
			// Выводим сообщение об ошибке
			this->_log->print("Text hashing \"%s\" could not be performed", log_t::flag_t::WARNING, text.c_str());
	}
}
/**
 * @brief Метод хэширования текста
 *
 * @param text   текст для хэширования
 * @param hash   тип хэш-суммы
 * @param result буфер куда следует положить результат
 */
void awh::Transform::hashing(const string & text, const hash_t hash, vector <char> & result) const noexcept {
	// Если текст для хэширования передан
	if(!text.empty()){
		// Выполняем хэширование
		driver::hashing(text, hash, result, this->_log);
		// Если хэширование не вышло
		if(result.empty())
			// Выводим сообщение об ошибке
			this->_log->print("Text hashing \"%s\" could not be performed", log_t::flag_t::WARNING, text.c_str());
	}
}
/**
 * @brief Метод хэширования текста
 *
 * @param text   текст для хэширования
 * @param hash   тип хэш-суммы
 * @param result буфер куда следует положить результат
 */
void awh::Transform::hashing(const string & text, const hash_t hash, vector <uint8_t> & result) const noexcept {
	// Если текст для хэширования передан
	if(!text.empty()){
		// Выполняем хэширование
		driver::hashing(text, hash, result, this->_log);
		// Если хэширование не вышло
		if(result.empty())
			// Выводим сообщение об ошибке
			this->_log->print("Text hashing \"%s\" could not be performed", log_t::flag_t::WARNING, text.c_str());
	}
}
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
auto awh::Transform::hmac(const string & key, const string & text, const hash_t hash) const noexcept -> T {
	// Результат работы функции
	T result;
	// Если текст передан
	if(!text.empty())
		// Выполняем хэширование
		this->hmac(key, text, hash, result);
	// Выводим результат
	return result;
}
/**
 * @brief Явный специализированный шаблон метода хэширования текста с ключом и выводом результата в строку
 *
 */
template string awh::Transform::hmac(const string &, const string &, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с ключом и выводом результата в буфер
 *
 */
template vector <char> awh::Transform::hmac(const string &, const string &, const hash_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода хэширования текста с ключом и выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Transform::hmac(const string &, const string &, const hash_t) const noexcept;
/**
 * @brief Метод хэширования текста с ключом
 *
 * @param key    ключ для подписи
 * @param text   текст для хэширования
 * @param hash   тип хэш-суммы
 * @param result строка куда следует положить результат
 */
void awh::Transform::hmac(const string & key, const string & text, const hash_t hash, string & result) const noexcept {
	// Если ключ и текст для хэширования переданы
	if(!key.empty() && !text.empty()){
		// Выполняем хэширование
		driver::hmac(key, text, hash, result, this->_log);
		// Если хэширование не вышло
		if(result.empty())
			// Выводим сообщение об ошибке
			this->_log->print("Key \"%s\" and text \"%s\" hashing  could not be performed", log_t::flag_t::WARNING, key.c_str(), text.c_str());
	}
}
/**
 * @brief Метод хэширования текста с ключом
 *
 * @param key    ключ для подписи
 * @param text   текст для хэширования
 * @param hash   тип хэш-суммы
 * @param result буфер куда следует положить результат
 */
void awh::Transform::hmac(const string & key, const string & text, const hash_t hash, vector <char> & result) const noexcept {
	// Если ключ и текст для хэширования переданы
	if(!key.empty() && !text.empty()){
		// Выполняем хэширование
		driver::hmac(key, text, hash, result, this->_log);
		// Если хэширование не вышло
		if(result.empty())
			// Выводим сообщение об ошибке
			this->_log->print("Key \"%s\" and text \"%s\" hashing  could not be performed", log_t::flag_t::WARNING, key.c_str(), text.c_str());
	}
}
/**
 * @brief Метод хэширования текста с ключом
 *
 * @param key    ключ для подписи
 * @param text   текст для хэширования
 * @param hash   тип хэш-суммы
 * @param result буфер куда следует положить результат
 */
void awh::Transform::hmac(const string & key, const string & text, const hash_t hash, vector <uint8_t> & result) const noexcept {
	// Если ключ и текст для хэширования переданы
	if(!key.empty() && !text.empty()){
		// Выполняем хэширование
		driver::hmac(key, text, hash, result, this->_log);
		// Если хэширование не вышло
		if(result.empty())
			// Выводим сообщение об ошибке
			this->_log->print("Key \"%s\" and text \"%s\" hashing  could not be performed", log_t::flag_t::WARNING, key.c_str(), text.c_str());
	}
}
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
auto awh::Transform::encode(const B & buffer, const cipher_t cipher) const noexcept -> A {
	// Результат работы функции
	A result;
	// Если буфер данных передан
	if(!buffer.empty())
		// Выполняем кодирование
		this->encode(buffer.data(), buffer.size(), cipher, result);
	// Выводим результат
	return result;
}
/**
 * @brief Явный специализированный шаблон метода кодирования данных из строки с выводом результата в строку
 *
 */
template string awh::Transform::encode(const string &, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода кодирования данных из буфера с выводом результата в строку
 *
 */
template string awh::Transform::encode(const vector <char> &, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода кодирования данных из бинарного буфера с выводом результата в строку
 *
 */
template string awh::Transform::encode(const vector <uint8_t> &, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода кодирования данных из строки с выводом результата в буфер
 *
 */
template vector <char> awh::Transform::encode(const string &, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода кодирования данных из буфера с выводом результата в буфер
 *
 */
template vector <char> awh::Transform::encode(const vector <char> &, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода кодирования данных из бинарного буфера с выводом результата в буфер
 *
 */
template vector <char> awh::Transform::encode(const vector <uint8_t> &, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода кодирования данных из строки с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Transform::encode(const string &, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода кодирования данных из буфера с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Transform::encode(const vector <char> &, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода кодирования данных из бинарного буфера с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Transform::encode(const vector <uint8_t> &, const cipher_t) const noexcept;
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
auto awh::Transform::encode(const void * buffer, const size_t size, const cipher_t cipher) const noexcept -> T {
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
 * @brief Явный специализированный шаблон метода кодирования с выводом результата в строку
 *
 */
template string awh::Transform::encode(const void *, const size_t, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода кодирования с выводом результата в буфер
 *
 */
template vector <char> awh::Transform::encode(const void *, const size_t, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода кодирования с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Transform::encode(const void *, const size_t, const cipher_t) const noexcept;
/**
 * @brief Метод кодирования
 *
 * @param buffer буфер данных для шифрования
 * @param size   размер данных для шифрования
 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
 * @param result строка куда следует положить результат
 */
void awh::Transform::encode(const void * buffer, const size_t size, const cipher_t cipher, string & result) const noexcept {
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
				// Выполняем кодирование строки BASE64
				driver::hashing(reinterpret_cast <const char *> (buffer), size, cipher, event_t::ENCODE, const_cast <crypto_state_t &> (this->_crypto.aes), result, this->_log);
				// Если кодирование не вышло
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Unable to encode \"%s\" string data into BASE64 format", log_t::flag_t::WARNING, string(reinterpret_cast <const char *> (buffer), size).c_str());
			} break;
			// Если производится работы с AES128
			case static_cast <uint8_t> (cipher_t::AES128):
			// Если производится работы с AES192
			case static_cast <uint8_t> (cipher_t::AES192):
			// Если производится работы с AES256
			case static_cast <uint8_t> (cipher_t::AES256): {
				// Если пароль установлен
				if(!this->_crypto.password.empty()){
					// Выполняем инициализацию AES
					if(const_cast <transform_t *> (this)->cipher(cipher)){
						// Выполняем блокировку потоков
						const locker_t <> lock(this->_mtx);
						// Выполняем шифрование данных
						driver::hashing(reinterpret_cast <const char *> (buffer), size, cipher, event_t::ENCODE, const_cast <crypto_state_t &> (this->_crypto.aes), result, this->_log);
					}
				}
				// Если кодирование не вышло
				if(result.empty()){
					// Выводим сообщение об ошибке
					this->_log->print("Unable to encode data into AES", log_t::flag_t::WARNING);
					// Выводим тот же самый буфер как он был передан
					result.assign(reinterpret_cast <const char *> (buffer), reinterpret_cast <const char *> (buffer) + size);
				}
			} break;
		}
	}
}
/**
 * @brief Метод кодирования
 *
 * @param buffer буфер данных для шифрования
 * @param size   размер данных для шифрования
 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
 * @param result буфер куда следует положить результат
 */
void awh::Transform::encode(const void * buffer, const size_t size, const cipher_t cipher, vector <char> & result) const noexcept {
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
				// Выполняем кодирование строки BASE64
				driver::hashing(reinterpret_cast <const char *> (buffer), size, cipher, event_t::ENCODE, const_cast <crypto_state_t &> (this->_crypto.aes), result, this->_log);
				// Если кодирование не вышло
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Unable to encode \"%s\" string data into BASE64 format", log_t::flag_t::WARNING, string(reinterpret_cast <const char *> (buffer), size).c_str());
			} break;
			// Если производится работы с AES128
			case static_cast <uint8_t> (cipher_t::AES128):
			// Если производится работы с AES192
			case static_cast <uint8_t> (cipher_t::AES192):
			// Если производится работы с AES256
			case static_cast <uint8_t> (cipher_t::AES256): {
				// Если пароль установлен
				if(!this->_crypto.password.empty()){
					// Выполняем инициализацию AES
					if(const_cast <transform_t *> (this)->cipher(cipher)){
						// Выполняем блокировку потоков
						const locker_t <> lock(this->_mtx);
						// Выполняем шифрование данных
						driver::hashing(reinterpret_cast <const char *> (buffer), size, cipher, event_t::ENCODE, const_cast <crypto_state_t &> (this->_crypto.aes), result, this->_log);
					}
				}
				// Если кодирование не вышло
				if(result.empty()){
					// Выводим сообщение об ошибке
					this->_log->print("Unable to encode data into AES", log_t::flag_t::WARNING);
					// Выводим тот же самый буфер как он был передан
					result.assign(reinterpret_cast <const uint8_t *> (buffer), reinterpret_cast <const uint8_t *> (buffer) + size);
				}
			} break;
		}
	}
}
/**
 * @brief Метод кодирования
 *
 * @param buffer буфер данных для шифрования
 * @param size   размер данных для шифрования
 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
 * @param result буфер куда следует положить результат
 */
void awh::Transform::encode(const void * buffer, const size_t size, const cipher_t cipher, vector <uint8_t> & result) const noexcept {
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
				// Выполняем кодирование строки BASE64
				driver::hashing(reinterpret_cast <const char *> (buffer), size, cipher, event_t::ENCODE, const_cast <crypto_state_t &> (this->_crypto.aes), result, this->_log);
				// Если кодирование не вышло
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Unable to encode \"%s\" string data into BASE64 format", log_t::flag_t::WARNING, string(reinterpret_cast <const char *> (buffer), size).c_str());
			} break;
			// Если производится работы с AES128
			case static_cast <uint8_t> (cipher_t::AES128):
			// Если производится работы с AES192
			case static_cast <uint8_t> (cipher_t::AES192):
			// Если производится работы с AES256
			case static_cast <uint8_t> (cipher_t::AES256): {
				// Если пароль установлен
				if(!this->_crypto.password.empty()){
					// Выполняем инициализацию AES
					if(const_cast <transform_t *> (this)->cipher(cipher)){
						// Выполняем блокировку потоков
						const locker_t <> lock(this->_mtx);
						// Выполняем шифрование данных
						driver::hashing(reinterpret_cast <const char *> (buffer), size, cipher, event_t::ENCODE, const_cast <crypto_state_t &> (this->_crypto.aes), result, this->_log);
					}
				}
				// Если кодирование не вышло
				if(result.empty()){
					// Выводим сообщение об ошибке
					this->_log->print("Unable to encode data into AES", log_t::flag_t::WARNING);
					// Выводим тот же самый буфер как он был передан
					result.assign(reinterpret_cast <const uint8_t *> (buffer), reinterpret_cast <const uint8_t *> (buffer) + size);
				}
			} break;
		}
	}
}
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
auto awh::Transform::decode(const B & buffer, const cipher_t cipher) const noexcept -> A {
	// Результат работы функции
	A result;
	// Если буфер данных передан
	if(!buffer.empty())
		// Выполняем декодирование
		this->decode(buffer.data(), buffer.size(), cipher, result);
	// Выводим результат
	return result;
}
/**
 * @brief Явный специализированный шаблон метода декодирования данных из строки с выводом результата в строку
 *
 */
template string awh::Transform::decode(const string &, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декодирования данных из буфера с выводом результата в строку
 *
 */
template string awh::Transform::decode(const vector <char> &, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декодирования данных из бинарного буфера с выводом результата в строку
 *
 */
template string awh::Transform::decode(const vector <uint8_t> &, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декодирования данных из строки с выводом результата в буфер
 *
 */
template vector <char> awh::Transform::decode(const string &, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декодирования данных из буфера с выводом результата в буфер
 *
 */
template vector <char> awh::Transform::decode(const vector <char> &, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декодирования данных из бинарного буфера с выводом результата в буфер
 *
 */
template vector <char> awh::Transform::decode(const vector <uint8_t> &, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декодирования данных из строки с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Transform::decode(const string &, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декодирования данных из буфера с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Transform::decode(const vector <char> &, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декодирования данных из бинарного буфера с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Transform::decode(const vector <uint8_t> &, const cipher_t) const noexcept;
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
auto awh::Transform::decode(const void * buffer, const size_t size, const cipher_t cipher) const noexcept -> T {
	// Результат работы функции
	T result;
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0))
		// Выполняем декодирование
		this->decode(buffer, size, cipher, result);
	// Выводим результат
	return result;
}
/**
 * @brief Явный специализированный шаблон метода декодирования с выводом результата в строку
 *
 */
template string awh::Transform::decode(const void *, const size_t, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декодирования с выводом результата в буфер
 *
 */
template vector <char> awh::Transform::decode(const void *, const size_t, const cipher_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декодирования с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Transform::decode(const void *, const size_t, const cipher_t) const noexcept;
/**
 * @brief Метод декодирования
 *
 * @param buffer буфер данных для шифрования
 * @param size   размер данных для шифрования
 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
 * @param result строка куда следует положить результат
 */
void awh::Transform::decode(const void * buffer, const size_t size, const cipher_t cipher, string & result) const noexcept {
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
				// Выполняем декодирование строки BASE64
				driver::hashing(reinterpret_cast <const char *> (buffer), size, cipher, event_t::DECODE, const_cast <crypto_state_t &> (this->_crypto.aes), result, this->_log);
				// Если декодирование не вышло
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Unable to extract data from BASE64 encoded \"%s\" hash", log_t::flag_t::WARNING, string(reinterpret_cast <const char *> (buffer), size).c_str());
			} break;
			// Если производится работы с AES128
			case static_cast <uint8_t> (cipher_t::AES128):
			// Если производится работы с AES192
			case static_cast <uint8_t> (cipher_t::AES192):
			// Если производится работы с AES256
			case static_cast <uint8_t> (cipher_t::AES256): {
				// Если пароль установлен
				if(!this->_crypto.password.empty()){
					// Выполняем инициализацию AES
					if(const_cast <transform_t *> (this)->cipher(cipher)){
						// Выполняем блокировку потоков
						const locker_t <> lock(this->_mtx);
						// Выполняем шифрование данных
						driver::hashing(reinterpret_cast <const char *> (buffer), size, cipher, event_t::DECODE, const_cast <crypto_state_t &> (this->_crypto.aes), result, this->_log);
					}
				}
				// Если кодирование не вышло
				if(result.empty()){
					// Выводим сообщение об ошибке
					this->_log->print("Unable to decode data from AES", log_t::flag_t::WARNING);
					// Выводим тот же самый буфер как он был передан
					result.assign(reinterpret_cast <const char *> (buffer), reinterpret_cast <const char *> (buffer) + size);
				}
			} break;
		}
	}
}
/**
 * @brief Метод декодирования
 *
 * @param buffer буфер данных для шифрования
 * @param size   размер данных для шифрования
 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
 * @param result буфер куда следует положить результат
 */
void awh::Transform::decode(const void * buffer, const size_t size, const cipher_t cipher, vector <char> & result) const noexcept {
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
				// Выполняем декодирование строки BASE64
				driver::hashing(reinterpret_cast <const char *> (buffer), size, cipher, event_t::DECODE, const_cast <crypto_state_t &> (this->_crypto.aes), result, this->_log);
				// Если декодирование не вышло
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Unable to extract data from BASE64 encoded \"%s\" hash", log_t::flag_t::WARNING, string(reinterpret_cast <const char *> (buffer), size).c_str());
			} break;
			// Если производится работы с AES128
			case static_cast <uint8_t> (cipher_t::AES128):
			// Если производится работы с AES192
			case static_cast <uint8_t> (cipher_t::AES192):
			// Если производится работы с AES256
			case static_cast <uint8_t> (cipher_t::AES256): {
				// Если пароль установлен
				if(!this->_crypto.password.empty()){
					// Выполняем инициализацию AES
					if(const_cast <transform_t *> (this)->cipher(cipher)){
						// Выполняем блокировку потоков
						const locker_t <> lock(this->_mtx);
						// Выполняем шифрование данных
						driver::hashing(reinterpret_cast <const char *> (buffer), size, cipher, event_t::DECODE, const_cast <crypto_state_t &> (this->_crypto.aes), result, this->_log);
					}
				}
				// Если кодирование не вышло
				if(result.empty()){
					// Выводим сообщение об ошибке
					this->_log->print("Unable to decode data from AES", log_t::flag_t::WARNING);
					// Выводим тот же самый буфер как он был передан
					result.assign(reinterpret_cast <const char *> (buffer), reinterpret_cast <const char *> (buffer) + size);
				}
			} break;
		}
	}
}
/**
 * @brief Метод декодирования
 *
 * @param buffer буфер данных для шифрования
 * @param size   размер данных для шифрования
 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
 * @param result буфер куда следует положить результат
 */
void awh::Transform::decode(const void * buffer, const size_t size, const cipher_t cipher, vector <uint8_t> & result) const noexcept {
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
				// Выполняем декодирование строки BASE64
				driver::hashing(reinterpret_cast <const char *> (buffer), size, cipher, event_t::DECODE, const_cast <crypto_state_t &> (this->_crypto.aes), result, this->_log);
				// Если декодирование не вышло
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Unable to extract data from BASE64 encoded \"%s\" hash", log_t::flag_t::WARNING, string(reinterpret_cast <const char *> (buffer), size).c_str());
			} break;
			// Если производится работы с AES128
			case static_cast <uint8_t> (cipher_t::AES128):
			// Если производится работы с AES192
			case static_cast <uint8_t> (cipher_t::AES192):
			// Если производится работы с AES256
			case static_cast <uint8_t> (cipher_t::AES256): {
				// Если пароль установлен
				if(!this->_crypto.password.empty()){
					// Выполняем инициализацию AES
					if(const_cast <transform_t *> (this)->cipher(cipher)){
						// Выполняем блокировку потоков
						const locker_t <> lock(this->_mtx);
						// Выполняем шифрование данных
						driver::hashing(reinterpret_cast <const char *> (buffer), size, cipher, event_t::DECODE, const_cast <crypto_state_t &> (this->_crypto.aes), result, this->_log);
					}
				}
				// Если кодирование не вышло
				if(result.empty()){
					// Выводим сообщение об ошибке
					this->_log->print("Unable to decode data from AES", log_t::flag_t::WARNING);
					// Выводим тот же самый буфер как он был передан
					result.assign(reinterpret_cast <const char *> (buffer), reinterpret_cast <const char *> (buffer) + size);
				}
			} break;
		}
	}
}
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
auto awh::Transform::compress(const B & buffer, const compressor_t compressor) const noexcept -> A {
	// Результат работы функции
	A result;
	// Если буфер данных передан
	if(!buffer.empty())
		// Выполняем кодирование
		this->compress(buffer.data(), buffer.size(), compressor, result);
	// Выводим результат
	return result;
}
/**
 * @brief Явный специализированный шаблон метода компрессии данных из строки с выводом результата в строку
 *
 */
template string awh::Transform::compress(const string &, const compressor_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода компрессии данных из буфера с выводом результата в строку
 *
 */
template string awh::Transform::compress(const vector <char> &, const compressor_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода компрессии данных из бинарного буфера с выводом результата в строку
 *
 */
template string awh::Transform::compress(const vector <uint8_t> &, const compressor_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода компрессии данных из строки с выводом результата в буфер
 *
 */
template vector <char> awh::Transform::compress(const string &, const compressor_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода компрессии данных из буфера с выводом результата в буфер
 *
 */
template vector <char> awh::Transform::compress(const vector <char> &, const compressor_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода компрессии данных из бинарного буфера с выводом результата в буфер
 *
 */
template vector <char> awh::Transform::compress(const vector <uint8_t> &, const compressor_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода компрессии данных из строки с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Transform::compress(const string &, const compressor_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода компрессии данных из буфера с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Transform::compress(const vector <char> &, const compressor_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода компрессии данных из бинарного буфера с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Transform::compress(const vector <uint8_t> &, const compressor_t) const noexcept;
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
auto awh::Transform::compress(const void * buffer, const size_t size, const compressor_t compressor) const noexcept -> T {
	// Результат работы функции
	T result;
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0))
		// Выполняем кодирование
		this->compress(buffer, size, compressor, result);
	// Выводим результат
	return result;
}
/**
 * @brief Явный специализированный шаблон метода компрессии данных с выводом результата в строку
 *
 */
template string awh::Transform::compress(const void *, const size_t, const compressor_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода компрессии данных с выводом результата в буфер
 *
 */
template vector <char> awh::Transform::compress(const void *, const size_t, const compressor_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода компрессии данных с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Transform::compress(const void *, const size_t, const compressor_t) const noexcept;
/**
 * @brief Метод компрессии данных
 *
 * @param buffer     буфер данных для компрессии
 * @param size       размер данных для компрессии
 * @param compressor метод компрессии
 * @param result     строка куда следует положить результат
 */
void awh::Transform::compress(const void * buffer, const size_t size, const compressor_t compressor, string & result) const noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Определяем метод компрессии данных
		 */
		switch(static_cast <uint8_t> (compressor)){
			// Если метод компрессии установлен Lz4
			case static_cast <uint8_t> (compressor_t::LZ4): {
				// Выполняем компрессию данных методом Lz4
				driver::lz4(reinterpret_cast <const char *> (buffer), size, this->_level[0], event_t::ENCODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("LZ4: %s", log_t::flag_t::WARNING, "Compress failed");
			} break;
			// Если метод компрессии установлен LZMA
			case static_cast <uint8_t> (compressor_t::LZMA): {
				// Выполняем компрессию данных методом LZMA
				driver::lzma(reinterpret_cast <const char *> (buffer), size, event_t::ENCODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("LZMA: %s", log_t::flag_t::WARNING, "Compress failed");
			} break;
			// Если метод компрессии установлен Zstandard
			case static_cast <uint8_t> (compressor_t::ZSTD): {
				// Выполняем компрессию данных методом Zstandard
				driver::zstd(reinterpret_cast <const char *> (buffer), size, this->_level[2], event_t::ENCODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Zstandard: %s", log_t::flag_t::WARNING, "Compress failed");
			} break;
			// Если метод компрессии установлен GZip
			case static_cast <uint8_t> (compressor_t::GZIP): {
				// Выполняем компрессию данных методом GZip
				driver::gzip(reinterpret_cast <const char *> (buffer), size, this->_level[1], this->_gzip.wbits, event_t::ENCODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("GZip: %s", log_t::flag_t::WARNING, "Compress failed");
			} break;
			// Если метод компрессии установлен Bzip2
			case static_cast <uint8_t> (compressor_t::BZIP2): {
				// Выполняем компрессию данных методом Bzip2
				driver::bzip2(reinterpret_cast <const char *> (buffer), size, event_t::ENCODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Bzip2: %s", log_t::flag_t::WARNING, "Compress failed");
			} break;
			// Если метод компрессии установлен Lizard
			case static_cast <uint8_t> (compressor_t::LIZARD): {
				// Выполняем компрессию данных методом Lizard
				driver::lizard(reinterpret_cast <const char *> (buffer), size, this->_level[3], event_t::ENCODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Lizard: %s", log_t::flag_t::WARNING, "Compress failed");
			} break;
			// Если метод компрессии установлен Snappy
			case static_cast <uint8_t> (compressor_t::SNAPPY): {
				// Выполняем компрессию данных методом Snappy
				driver::snappy(reinterpret_cast <const char *> (buffer), size, event_t::ENCODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Snappy: %s", log_t::flag_t::WARNING, "Compress failed");
			} break;
			// Если метод компрессии установлен Density
			case static_cast <uint8_t> (compressor_t::DENSITY): {
				// Выполняем компрессию данных методом Density
				driver::density(reinterpret_cast <const char *> (buffer), size, event_t::ENCODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Density: %s", log_t::flag_t::WARNING, "Compress failed");
			} break;
			// Если метод компрессии установлен Brotli
			case static_cast <uint8_t> (compressor_t::BROTLI): {
				// Выполняем компрессию данных методом Brotli
				driver::brotli(reinterpret_cast <const char *> (buffer), size, event_t::ENCODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Brotli: %s", log_t::flag_t::WARNING, "Compress failed");
			} break;
			// Если метод компрессии установлен Deflate
			case static_cast <uint8_t> (compressor_t::DEFLATE): {
				// Выполняем блокировку потоков
				const locker_t <> lock(this->_mtx);
				// Выполняем компрессию данных методом Deflate
				driver::deflate(reinterpret_cast <const char *> (buffer), size, this->_level[1], this->_gzip.wbits, this->_gzip.takeover.compress.load(std::memory_order_acquire), std::any_cast <z_stream &> (this->_gzip.buffer.compress), event_t::ENCODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Deflate: %s", log_t::flag_t::WARNING, "Compress failed");
			} break;
			// Если метод компрессии не установлен
			case static_cast <uint8_t> (compressor_t::NONE):
				// Выводим переданный буфер данных
				result.assign(reinterpret_cast <const char *> (buffer), reinterpret_cast <const char *> (buffer) + size);
			break;
		}
	}
}
/**
 * @brief Метод компрессии данных
 *
 * @param buffer     буфер данных для компрессии
 * @param size       размер данных для компрессии
 * @param compressor метод компрессии
 * @param result     буфер куда следует положить результат
 */
void awh::Transform::compress(const void * buffer, const size_t size, const compressor_t compressor, vector <char> & result) const noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Определяем метод компрессии данных
		 */
		switch(static_cast <uint8_t> (compressor)){
			// Если метод компрессии установлен Z4
			case static_cast <uint8_t> (compressor_t::LZ4): {
				// Выполняем компрессию данных методом LZ4
				driver::lz4(reinterpret_cast <const char *> (buffer), size, this->_level[0], event_t::ENCODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("LZ4: %s", log_t::flag_t::WARNING, "Compress failed");
			} break;
			// Если метод компрессии установлен LZMA
			case static_cast <uint8_t> (compressor_t::LZMA): {
				// Выполняем компрессию данных методом LZMA
				driver::lzma(reinterpret_cast <const char *> (buffer), size, event_t::ENCODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("LZMA: %s", log_t::flag_t::WARNING, "Compress failed");
			} break;
			// Если метод компрессии установлен Zstandard
			case static_cast <uint8_t> (compressor_t::ZSTD): {
				// Выполняем компрессию данных методом Zstandard
				driver::zstd(reinterpret_cast <const char *> (buffer), size, this->_level[2], event_t::ENCODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Zstandard: %s", log_t::flag_t::WARNING, "Compress failed");
			} break;
			// Если метод компрессии установлен GZip
			case static_cast <uint8_t> (compressor_t::GZIP): {
				// Выполняем компрессию данных методом GZip
				driver::gzip(reinterpret_cast <const char *> (buffer), size, this->_level[1], this->_gzip.wbits, event_t::ENCODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("GZip: %s", log_t::flag_t::WARNING, "Compress failed");
			} break;
			// Если метод компрессии установлен Bzip2
			case static_cast <uint8_t> (compressor_t::BZIP2): {
				// Выполняем компрессию данных методом Bzip2
				driver::bzip2(reinterpret_cast <const char *> (buffer), size, event_t::ENCODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Bzip2: %s", log_t::flag_t::WARNING, "Compress failed");
			} break;
			// Если метод компрессии установлен Lizard
			case static_cast <uint8_t> (compressor_t::LIZARD): {
				// Выполняем компрессию данных методом Lizard
				driver::lizard(reinterpret_cast <const char *> (buffer), size, this->_level[3], event_t::ENCODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Lizard: %s", log_t::flag_t::WARNING, "Compress failed");
			} break;
			// Если метод компрессии установлен Snappy
			case static_cast <uint8_t> (compressor_t::SNAPPY): {
				// Выполняем компрессию данных методом Snappy
				driver::snappy(reinterpret_cast <const char *> (buffer), size, event_t::ENCODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Snappy: %s", log_t::flag_t::WARNING, "Compress failed");
			} break;
			// Если метод компрессии установлен Density
			case static_cast <uint8_t> (compressor_t::DENSITY): {
				// Выполняем компрессию данных методом Density
				driver::density(reinterpret_cast <const char *> (buffer), size, event_t::ENCODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Density: %s", log_t::flag_t::WARNING, "Compress failed");
			} break;
			// Если метод компрессии установлен Brotli
			case static_cast <uint8_t> (compressor_t::BROTLI): {
				// Выполняем компрессию данных методом Brotli
				driver::brotli(reinterpret_cast <const char *> (buffer), size, event_t::ENCODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Brotli: %s", log_t::flag_t::WARNING, "Compress failed");
			} break;
			// Если метод компрессии установлен Deflate
			case static_cast <uint8_t> (compressor_t::DEFLATE): {
				// Выполняем блокировку потоков
				const locker_t <> lock(this->_mtx);
				// Выполняем компрессию данных методом Deflate
				driver::deflate(reinterpret_cast <const char *> (buffer), size, this->_level[1], this->_gzip.wbits, this->_gzip.takeover.compress.load(std::memory_order_acquire), std::any_cast <z_stream &> (this->_gzip.buffer.compress), event_t::ENCODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Deflate: %s", log_t::flag_t::WARNING, "Compress failed");
			} break;
			// Если метод компрессии не установлен
			case static_cast <uint8_t> (compressor_t::NONE):
				// Выводим переданный буфер данных
				result.assign(reinterpret_cast <const char *> (buffer), reinterpret_cast <const char *> (buffer) + size);
			break;
		}
	}
}
/**
 * @brief Метод компрессии данных
 *
 * @param buffer     буфер данных для компрессии
 * @param size       размер данных для компрессии
 * @param compressor метод компрессии
 * @param result     буфер куда следует положить результат
 */
void awh::Transform::compress(const void * buffer, const size_t size, const compressor_t compressor, vector <uint8_t> & result) const noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Определяем метод компрессии данных
		 */
		switch(static_cast <uint8_t> (compressor)){
			// Если метод компрессии установлен Z4
			case static_cast <uint8_t> (compressor_t::LZ4): {
				// Выполняем компрессию данных методом LZ4
				driver::lz4(reinterpret_cast <const char *> (buffer), size, this->_level[0], event_t::ENCODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("LZ4: %s", log_t::flag_t::WARNING, "Compress failed");
			} break;
			// Если метод компрессии установлен LZMA
			case static_cast <uint8_t> (compressor_t::LZMA): {
				// Выполняем компрессию данных методом LZMA
				driver::lzma(reinterpret_cast <const char *> (buffer), size, event_t::ENCODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("LZMA: %s", log_t::flag_t::WARNING, "Compress failed");
			} break;
			// Если метод компрессии установлен Zstandard
			case static_cast <uint8_t> (compressor_t::ZSTD): {
				// Выполняем компрессию данных методом Zstandard
				driver::zstd(reinterpret_cast <const char *> (buffer), size, this->_level[2], event_t::ENCODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Zstandard: %s", log_t::flag_t::WARNING, "Compress failed");
			} break;
			// Если метод компрессии установлен GZip
			case static_cast <uint8_t> (compressor_t::GZIP): {
				// Выполняем компрессию данных методом GZip
				driver::gzip(reinterpret_cast <const char *> (buffer), size, this->_level[1], this->_gzip.wbits, event_t::ENCODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("GZip: %s", log_t::flag_t::WARNING, "Compress failed");
			} break;
			// Если метод компрессии установлен Bzip2
			case static_cast <uint8_t> (compressor_t::BZIP2): {
				// Выполняем компрессию данных методом Bzip2
				driver::bzip2(reinterpret_cast <const char *> (buffer), size, event_t::ENCODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Bzip2: %s", log_t::flag_t::WARNING, "Compress failed");
			} break;
			// Если метод компрессии установлен Lizard
			case static_cast <uint8_t> (compressor_t::LIZARD): {
				// Выполняем компрессию данных методом Lizard
				driver::lizard(reinterpret_cast <const char *> (buffer), size, this->_level[3], event_t::ENCODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Lizard: %s", log_t::flag_t::WARNING, "Compress failed");
			} break;
			// Если метод компрессии установлен Snappy
			case static_cast <uint8_t> (compressor_t::SNAPPY): {
				// Выполняем компрессию данных методом Snappy
				driver::snappy(reinterpret_cast <const char *> (buffer), size, event_t::ENCODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Snappy: %s", log_t::flag_t::WARNING, "Compress failed");
			} break;
			// Если метод компрессии установлен Density
			case static_cast <uint8_t> (compressor_t::DENSITY): {
				// Выполняем компрессию данных методом Density
				driver::density(reinterpret_cast <const char *> (buffer), size, event_t::ENCODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Density: %s", log_t::flag_t::WARNING, "Compress failed");
			} break;
			// Если метод компрессии установлен Brotli
			case static_cast <uint8_t> (compressor_t::BROTLI): {
				// Выполняем компрессию данных методом Brotli
				driver::brotli(reinterpret_cast <const char *> (buffer), size, event_t::ENCODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Brotli: %s", log_t::flag_t::WARNING, "Compress failed");
			} break;
			// Если метод компрессии установлен Deflate
			case static_cast <uint8_t> (compressor_t::DEFLATE): {
				// Выполняем блокировку потоков
				const locker_t <> lock(this->_mtx);
				// Выполняем компрессию данных методом Deflate
				driver::deflate(reinterpret_cast <const char *> (buffer), size, this->_level[1], this->_gzip.wbits, this->_gzip.takeover.compress.load(std::memory_order_acquire), std::any_cast <z_stream &> (this->_gzip.buffer.compress), event_t::ENCODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Deflate: %s", log_t::flag_t::WARNING, "Compress failed");
			} break;
			// Если метод компрессии не установлен
			case static_cast <uint8_t> (compressor_t::NONE):
				// Выводим переданный буфер данных
				result.assign(reinterpret_cast <const char *> (buffer), reinterpret_cast <const char *> (buffer) + size);
			break;
		}
	}
}
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
auto awh::Transform::decompress(const B & buffer, const compressor_t compressor) const noexcept -> A {
	// Результат работы функции
	A result;
	// Если буфер данных передан
	if(!buffer.empty())
		// Выполняем декомпрессию
		this->decompress(buffer.data(), buffer.size(), compressor, result);
	// Выводим результат
	return result;
}
/**
 * @brief Явный специализированный шаблон метода декомпрессии данных из строки с выводом результата в строку
 *
 */
template string awh::Transform::decompress(const string &, const compressor_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декомпрессии данных из буфера с выводом результата в строку
 *
 */
template string awh::Transform::decompress(const vector <char> &, const compressor_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декомпрессии данных из бинарного буфера с выводом результата в строку
 *
 */
template string awh::Transform::decompress(const vector <uint8_t> &, const compressor_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декомпрессии данных из строки с выводом результата в буфер
 *
 */
template vector <char> awh::Transform::decompress(const string &, const compressor_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декомпрессии данных из буфера с выводом результата в буфер
 *
 */
template vector <char> awh::Transform::decompress(const vector <char> &, const compressor_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декомпрессии данных из бинарного буфера с выводом результата в буфер
 *
 */
template vector <char> awh::Transform::decompress(const vector <uint8_t> &, const compressor_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декомпрессии данных из строки с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Transform::decompress(const string &, const compressor_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декомпрессии данных из буфера с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Transform::decompress(const vector <char> &, const compressor_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декомпрессии данных из бинарного буфера с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Transform::decompress(const vector <uint8_t> &, const compressor_t) const noexcept;
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
auto awh::Transform::decompress(const void * buffer, const size_t size, const compressor_t compressor) const noexcept -> T {
	// Результат работы функции
	T result;
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0))
		// Выполняем декомпрессию
		this->decompress(buffer, size, compressor, result);
	// Выводим результат
	return result;
}
/**
 * @brief Явный специализированный шаблон метода декомпрессии данных с выводом результата в строку
 *
 */
template string awh::Transform::decompress(const void *, const size_t, const compressor_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декомпрессии данных с выводом результата в буфер
 *
 */
template vector <char> awh::Transform::decompress(const void *, const size_t, const compressor_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декомпрессии данных с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::Transform::decompress(const void *, const size_t, const compressor_t) const noexcept;
/**
 * @brief Метод декомпрессии данных
 *
 * @param buffer     буфер данных для декомпрессии
 * @param size       размер данных для декомпрессии
 * @param compressor метод компрессии
 * @param result     строка куда следует положить результат
 */
void awh::Transform::decompress(const void * buffer, const size_t size, const compressor_t compressor, string & result) const noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Определяем метод декомпрессии данных
		 */
		switch(static_cast <uint8_t> (compressor)){
			// Если метод декомпрессии установлен LZ4
			case static_cast <uint8_t> (compressor_t::LZ4): {
				// Выполняем декомпрессию данных методом LZ4
				driver::lz4(reinterpret_cast <const char *> (buffer), size, this->_level[0], event_t::DECODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("LZ4: %s", log_t::flag_t::WARNING, "Decompress failed");
			} break;
			// Если метод декомпрессии установлен LZMA
			case static_cast <uint8_t> (compressor_t::LZMA): {
				// Выполняем декомпрессию данных методом LZMA
				driver::lzma(reinterpret_cast <const char *> (buffer), size, event_t::DECODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("LZMA: %s", log_t::flag_t::WARNING, "Decompress failed");
			} break;
			// Если метод декомпрессии установлен Zstandard
			case static_cast <uint8_t> (compressor_t::ZSTD): {
				// Выполняем декомпрессию данных методом Zstandard
				driver::zstd(reinterpret_cast <const char *> (buffer), size, this->_level[2], event_t::DECODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Zstandard: %s", log_t::flag_t::WARNING, "Decompress failed");
			} break;
			// Если метод декомпрессии установлен GZip
			case static_cast <uint8_t> (compressor_t::GZIP): {
				// Выполняем декомпрессию данных методом GZip
				driver::gzip(reinterpret_cast <const char *> (buffer), size, this->_level[1], this->_gzip.wbits, event_t::DECODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("GZip: %s", log_t::flag_t::WARNING, "Decompress failed");
			} break;
			// Если метод декомпрессии установлен Bzip2
			case static_cast <uint8_t> (compressor_t::BZIP2): {
				// Выполняем декомпрессию данных методом Bzip2
				driver::bzip2(reinterpret_cast <const char *> (buffer), size, event_t::DECODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Bzip2: %s", log_t::flag_t::WARNING, "Decompress failed");
			} break;
			// Если метод декомпрессии установлен Lizard
			case static_cast <uint8_t> (compressor_t::LIZARD): {
				// Выполняем декомпрессию данных методом Lizard
				driver::lizard(reinterpret_cast <const char *> (buffer), size, this->_level[3], event_t::DECODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Lizard: %s", log_t::flag_t::WARNING, "Decompress failed");
			} break;
			// Если метод декомпрессии установлен Snappy
			case static_cast <uint8_t> (compressor_t::SNAPPY): {
				// Выполняем декомпрессию данных методом Snappy
				driver::snappy(reinterpret_cast <const char *> (buffer), size, event_t::DECODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Snappy: %s", log_t::flag_t::WARNING, "Decompress failed");
			} break;
			// Если метод декомпрессии установлен Density
			case static_cast <uint8_t> (compressor_t::DENSITY): {
				// Выполняем декомпрессию данных методом Density
				driver::density(reinterpret_cast <const char *> (buffer), size, event_t::DECODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Density: %s", log_t::flag_t::WARNING, "Decompress failed");
			} break;
			// Если метод декомпрессии установлен Brotli
			case static_cast <uint8_t> (compressor_t::BROTLI): {
				// Выполняем декомпрессию данных методом Brotli
				driver::brotli(reinterpret_cast <const char *> (buffer), size, event_t::DECODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Brotli: %s", log_t::flag_t::WARNING, "Decompress failed");
			} break;
			// Если метод декомпрессии установлен Deflate
			case static_cast <uint8_t> (compressor_t::DEFLATE): {
				// Выполняем блокировку потоков
				const locker_t <> lock(this->_mtx);
				// Выполняем декомпрессию данных методом Deflate
				driver::deflate(reinterpret_cast <const char *> (buffer), size, this->_level[1], this->_gzip.wbits, this->_gzip.takeover.decompress.load(std::memory_order_acquire), std::any_cast <z_stream &> (this->_gzip.buffer.decompress), event_t::DECODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Deflate: %s", log_t::flag_t::WARNING, "Decompress failed");
			} break;
			// Если метод декомпрессии не установлен
			case static_cast <uint8_t> (compressor_t::NONE):
				// Выводим переданный буфер данных
				result.assign(reinterpret_cast <const char *> (buffer), reinterpret_cast <const char *> (buffer) + size);
			break;
		}
	}
}
/**
 * @brief Метод декомпрессии данных
 *
 * @param buffer     буфер данных для декомпрессии
 * @param size       размер данных для декомпрессии
 * @param compressor метод компрессии
 * @param result     буфер куда следует положить результат
 */
void awh::Transform::decompress(const void * buffer, const size_t size, const compressor_t compressor, vector <char> & result) const noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Определяем метод декомпрессии данных
		 */
		switch(static_cast <uint8_t> (compressor)){
			// Если метод декомпрессии установлен LZ4
			case static_cast <uint8_t> (compressor_t::LZ4): {
				// Выполняем декомпрессию данных методом LZ4
				driver::lz4(reinterpret_cast <const char *> (buffer), size, this->_level[0], event_t::DECODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("LZ4: %s", log_t::flag_t::WARNING, "Decompress failed");
			} break;
			// Если метод декомпрессии установлен LZMA
			case static_cast <uint8_t> (compressor_t::LZMA): {
				// Выполняем декомпрессию данных методом LZMA
				driver::lzma(reinterpret_cast <const char *> (buffer), size, event_t::DECODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("LZMA: %s", log_t::flag_t::WARNING, "Decompress failed");
			} break;
			// Если метод декомпрессии установлен Zstandard
			case static_cast <uint8_t> (compressor_t::ZSTD): {
				// Выполняем декомпрессию данных методом Zstandard
				driver::zstd(reinterpret_cast <const char *> (buffer), size, this->_level[2], event_t::DECODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Zstandard: %s", log_t::flag_t::WARNING, "Decompress failed");
			} break;
			// Если метод декомпрессии установлен GZip
			case static_cast <uint8_t> (compressor_t::GZIP): {
				// Выполняем декомпрессию данных методом GZip
				driver::gzip(reinterpret_cast <const char *> (buffer), size, this->_level[1], this->_gzip.wbits, event_t::DECODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("GZip: %s", log_t::flag_t::WARNING, "Decompress failed");
			} break;
			// Если метод декомпрессии установлен Bzip2
			case static_cast <uint8_t> (compressor_t::BZIP2): {
				// Выполняем декомпрессию данных методом Bzip2
				driver::bzip2(reinterpret_cast <const char *> (buffer), size, event_t::DECODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Bzip2: %s", log_t::flag_t::WARNING, "Decompress failed");
			} break;
			// Если метод декомпрессии установлен Lizard
			case static_cast <uint8_t> (compressor_t::LIZARD): {
				// Выполняем декомпрессию данных методом Lizard
				driver::lizard(reinterpret_cast <const char *> (buffer), size, this->_level[3], event_t::DECODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Lizard: %s", log_t::flag_t::WARNING, "Decompress failed");
			} break;
			// Если метод декомпрессии установлен Snappy
			case static_cast <uint8_t> (compressor_t::SNAPPY): {
				// Выполняем декомпрессию данных методом Snappy
				driver::snappy(reinterpret_cast <const char *> (buffer), size, event_t::DECODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Snappy: %s", log_t::flag_t::WARNING, "Decompress failed");
			} break;
			// Если метод декомпрессии установлен Density
			case static_cast <uint8_t> (compressor_t::DENSITY): {
				// Выполняем декомпрессию данных методом Density
				driver::density(reinterpret_cast <const char *> (buffer), size, event_t::DECODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Density: %s", log_t::flag_t::WARNING, "Decompress failed");
			} break;
			// Если метод декомпрессии установлен Brotli
			case static_cast <uint8_t> (compressor_t::BROTLI): {
				// Выполняем декомпрессию данных методом Brotli
				driver::brotli(reinterpret_cast <const char *> (buffer), size, event_t::DECODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Brotli: %s", log_t::flag_t::WARNING, "Decompress failed");
			} break;
			// Если метод декомпрессии установлен Deflate
			case static_cast <uint8_t> (compressor_t::DEFLATE): {
				// Выполняем блокировку потоков
				const locker_t <> lock(this->_mtx);
				// Выполняем декомпрессию данных методом Deflate
				driver::deflate(reinterpret_cast <const char *> (buffer), size, this->_level[1], this->_gzip.wbits, this->_gzip.takeover.decompress.load(std::memory_order_acquire), std::any_cast <z_stream &> (this->_gzip.buffer.decompress), event_t::DECODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Deflate: %s", log_t::flag_t::WARNING, "Decompress failed");
			} break;
			// Если метод декомпрессии не установлен
			case static_cast <uint8_t> (compressor_t::NONE):
				// Выводим переданный буфер данных
				result.assign(reinterpret_cast <const char *> (buffer), reinterpret_cast <const char *> (buffer) + size);
			break;
		}
	}
}
/**
 * @brief Метод декомпрессии данных
 *
 * @param buffer     буфер данных для декомпрессии
 * @param size       размер данных для декомпрессии
 * @param compressor метод компрессии
 * @param result     буфер куда следует положить результат
 */
void awh::Transform::decompress(const void * buffer, const size_t size, const compressor_t compressor, vector <uint8_t> & result) const noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Определяем метод декомпрессии данных
		 */
		switch(static_cast <uint8_t> (compressor)){
			// Если метод декомпрессии установлен LZ4
			case static_cast <uint8_t> (compressor_t::LZ4): {
				// Выполняем декомпрессию данных методом LZ4
				driver::lz4(reinterpret_cast <const char *> (buffer), size, this->_level[0], event_t::DECODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("LZ4: %s", log_t::flag_t::WARNING, "Decompress failed");
			} break;
			// Если метод декомпрессии установлен LZMA
			case static_cast <uint8_t> (compressor_t::LZMA): {
				// Выполняем декомпрессию данных методом LZMA
				driver::lzma(reinterpret_cast <const char *> (buffer), size, event_t::DECODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("LZMA: %s", log_t::flag_t::WARNING, "Decompress failed");
			} break;
			// Если метод декомпрессии установлен Zstandard
			case static_cast <uint8_t> (compressor_t::ZSTD): {
				// Выполняем декомпрессию данных методом Zstandard
				driver::zstd(reinterpret_cast <const char *> (buffer), size, this->_level[2], event_t::DECODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Zstandard: %s", log_t::flag_t::WARNING, "Decompress failed");
			} break;
			// Если метод декомпрессии установлен GZip
			case static_cast <uint8_t> (compressor_t::GZIP): {
				// Выполняем декомпрессию данных методом GZip
				driver::gzip(reinterpret_cast <const char *> (buffer), size, this->_level[1], this->_gzip.wbits, event_t::DECODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("GZip: %s", log_t::flag_t::WARNING, "Decompress failed");
			} break;
			// Если метод декомпрессии установлен Bzip2
			case static_cast <uint8_t> (compressor_t::BZIP2): {
				// Выполняем декомпрессию данных методом Bzip2
				driver::bzip2(reinterpret_cast <const char *> (buffer), size, event_t::DECODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Bzip2: %s", log_t::flag_t::WARNING, "Decompress failed");
			} break;
			// Если метод декомпрессии установлен Lizard
			case static_cast <uint8_t> (compressor_t::LIZARD): {
				// Выполняем декомпрессию данных методом Lizard
				driver::lizard(reinterpret_cast <const char *> (buffer), size, this->_level[3], event_t::DECODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Lizard: %s", log_t::flag_t::WARNING, "Decompress failed");
			} break;
			// Если метод декомпрессии установлен Snappy
			case static_cast <uint8_t> (compressor_t::SNAPPY): {
				// Выполняем декомпрессию данных методом Snappy
				driver::snappy(reinterpret_cast <const char *> (buffer), size, event_t::DECODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Snappy: %s", log_t::flag_t::WARNING, "Decompress failed");
			} break;
			// Если метод декомпрессии установлен Density
			case static_cast <uint8_t> (compressor_t::DENSITY): {
				// Выполняем декомпрессию данных методом Density
				driver::density(reinterpret_cast <const char *> (buffer), size, event_t::DECODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Density: %s", log_t::flag_t::WARNING, "Decompress failed");
			} break;
			// Если метод декомпрессии установлен Brotli
			case static_cast <uint8_t> (compressor_t::BROTLI): {
				// Выполняем декомпрессию данных методом Brotli
				driver::brotli(reinterpret_cast <const char *> (buffer), size, event_t::DECODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Brotli: %s", log_t::flag_t::WARNING, "Decompress failed");
			} break;
			// Если метод декомпрессии установлен Deflate
			case static_cast <uint8_t> (compressor_t::DEFLATE): {
				// Выполняем блокировку потоков
				const locker_t <> lock(this->_mtx);
				// Выполняем декомпрессию данных методом Deflate
				driver::deflate(reinterpret_cast <const char *> (buffer), size, this->_level[1], this->_gzip.wbits, this->_gzip.takeover.decompress.load(std::memory_order_acquire), std::any_cast <z_stream &> (this->_gzip.buffer.decompress), event_t::DECODE, result, this->_log);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Deflate: %s", log_t::flag_t::WARNING, "Decompress failed");
			} break;
			// Если метод декомпрессии не установлен
			case static_cast <uint8_t> (compressor_t::NONE):
				// Выводим переданный буфер данных
				result.assign(reinterpret_cast <const char *> (buffer), reinterpret_cast <const char *> (buffer) + size);
			break;
		}
	}
}
/**
 * @brief Конструктор
 *
 * @param log объект для работы с логами
 */
awh::Transform::Transform(const log_t * log) noexcept :
 _level{1, Z_DEFAULT_COMPRESSION, ZSTD_CLEVEL_DEFAULT, LIZARD_DEFAULT_CLEVEL}, _log(log) {
	// Объявляем структуру буфера GZip компрессии
	this->_gzip.buffer.compress = z_stream{};
	// Объявляем структуру буфера GZip декомпрессии
	this->_gzip.buffer.decompress = z_stream{};
	// Устанавливаем размер скользящего окна GZip по умолчанию
	this->_gzip.wbits = static_cast <int16_t> (MAX_WBITS);
}
/**
 * @brief Деструктор
 *
 */
awh::Transform::~Transform() noexcept {
	// Если выделена память для компрессора
	if(this->_gzip.takeover.compress.load(std::memory_order_acquire))
		// Завершаем работу компрессора GZip
		::deflateEnd(&std::any_cast <z_stream &> (this->_gzip.buffer.compress));
	// Если выделена память для декомпрессора
	if(this->_gzip.takeover.decompress.load(std::memory_order_acquire))
		// Завершаем работу декомпрессора GZip
		::inflateEnd(&std::any_cast <z_stream &> (this->_gzip.buffer.decompress));
}
