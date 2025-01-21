/**
 * @file: hash.cpp
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

// Подключаем заголовочный файл
#include <sys/hash.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Устанавливаем уровень сжатия
 */
static constexpr uint16_t DEFAULT_MEM_LEVEL = 4;
/**
 * Размер буфера чанка в байтах
 */
static constexpr uint32_t CHUNK_BUFFER_SIZE = 0x4000;

/**
 * Шаблон функции хэширования текста
 * @tparam T сигнатура функции
 */
template <typename T>
/**
 * hashing Функция хэширования текста
 * @param text   текст для хэширования
 * @param type   тип хэш-суммы
 * @param result результат хэширования
 */
static void hashing(const string & text, const awh::hash_t::type_t type, T & result) noexcept {
	// Если текст для хэширования передан
	if(!text.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем очистку блока с результатом
			result.clear();
			// Буфер промежуточных значений
			vector <u_char> digest;
			// Определяем тип хэш-суммы
			switch(static_cast <uint8_t> (type)){
				// Если тип хэш-суммы указан как MD5
				case static_cast <uint8_t> (awh::hash_t::type_t::MD5): {
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
						::sprintf(&result[i * 2], "%02x", static_cast <uint32_t> (digest[i]));
					// Удаляем последний символ
					result.pop_back();
				} break;
				// Если тип хэш-суммы указан как SHA1
				case static_cast <uint8_t> (awh::hash_t::type_t::SHA1): {
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
						::sprintf(&result[i * 2], "%02x", static_cast <uint32_t> (digest[i]));
					// Удаляем последний символ
					result.pop_back();
				} break;
				// Если тип хэш-суммы указан как SHA224
				case static_cast <uint8_t> (awh::hash_t::type_t::SHA224): {
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
						::sprintf(&result[i * 2], "%02x", static_cast <uint32_t> (digest[i]));
					// Удаляем последний символ
					result.pop_back();
				} break;
				// Если тип хэш-суммы указан как SHA256
				case static_cast <uint8_t> (awh::hash_t::type_t::SHA256): {
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
						::sprintf(&result[i * 2], "%02x", static_cast <uint32_t> (digest[i]));
					// Удаляем последний символ
					result.pop_back();
				} break;
				// Если тип хэш-суммы указан как SHA384
				case static_cast <uint8_t> (awh::hash_t::type_t::SHA384): {
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
						::sprintf(&result[i * 2], "%02x", static_cast <uint32_t> (digest[i]));
					// Удаляем последний символ
					result.pop_back();
				} break;
				// Если тип хэш-суммы указан как SHA512
				case static_cast <uint8_t> (awh::hash_t::type_t::SHA512): {
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
						::sprintf(&result[i * 2], "%02x", static_cast <uint32_t> (digest[i]));
					// Удаляем последний символ
					result.pop_back();
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception &) {
			// Выполняем очистку блока с результатом
			result.clear();
		}
	}
}
/**
 * Шаблон функции хэширования текста с ключом
 * @tparam T сигнатура функции
 */
template <typename T>
/**
 * hmac Функция хэширования текста с ключом
 * @param key    ключ для подписи
 * @param text   текст для хэширования
 * @param type   тип хэш-суммы
 * @param result результат хэширования
 */
static void hmac(const string & key, const string & text, const awh::hash_t::type_t type, T & result) noexcept {
	// Если ключ и текст для хэширования переданы
	if(!key.empty() && !text.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем очистку блока с результатом
			result.clear();
			// Определяем тип хэш-суммы
			switch(static_cast <uint8_t> (type)){
				// Если тип хэш-суммы указан как HMAC_MD5
				case static_cast <uint8_t> (awh::hash_t::type_t::MD5): {
					// Выделяем память для буфера данных
					result.resize(33, 0);
					// Выполняем получение подписи
					const u_char * digest = ::HMAC(::EVP_md5(), key.data(), key.size(), reinterpret_cast <const u_char *> (text.data()), text.size(), nullptr, nullptr);
					// Заполняем строку данными MD5
					for(uint8_t i = 0; i < 16; i++)
						// Формируем данные MD5-хэша
						::sprintf(&result[i * 2], "%02x", static_cast <uint32_t> (digest[i]));
					// Удаляем последний символ
					result.pop_back();
				} break;
				// Если тип хэш-суммы указан как HMAC_SHA1
				case static_cast <uint8_t> (awh::hash_t::type_t::SHA1): {
					// Выделяем память для буфера данных
					result.resize(41, 0);
					// Выполняем получение подписи
					const u_char * digest = ::HMAC(::EVP_sha1(), key.data(), key.size(), reinterpret_cast <const u_char *> (text.data()), text.size(), nullptr, nullptr);
					// Заполняем строку данными SHA1
					for(uint8_t i = 0; i < 20; i++)
						// Формируем данные SHA1-хэша
						::sprintf(&result[i * 2], "%02x", static_cast <uint32_t> (digest[i]));
					// Удаляем последний символ
					result.pop_back();
				} break;
				// Если тип хэш-суммы указан как HMAC_SHA224
				case static_cast <uint8_t> (awh::hash_t::type_t::SHA224): {
					// Выделяем память для буфера данных
					result.resize(57, 0);
					// Выполняем получение подписи
					const u_char * digest = ::HMAC(::EVP_sha224(), key.data(), key.size(), reinterpret_cast <const u_char *> (text.data()), text.size(), nullptr, nullptr);
					// Заполняем строку данными SHA224
					for(uint8_t i = 0; i < 28; i++)
						// Формируем данные SHA224-хэша
						::sprintf(&result[i * 2], "%02x", static_cast <uint32_t> (digest[i]));
					// Удаляем последний символ
					result.pop_back();
				} break;
				// Если тип хэш-суммы указан как HMAC_SHA256
				case static_cast <uint8_t> (awh::hash_t::type_t::SHA256): {
					// Выделяем память для буфера данных
					result.resize(65, 0);
					// Выполняем получение подписи
					const u_char * digest = ::HMAC(::EVP_sha256(), key.data(), key.size(), reinterpret_cast <const u_char *> (text.data()), text.size(), nullptr, nullptr);
					// Заполняем строку данными SHA256
					for(uint8_t i = 0; i < 32; i++)
						// Формируем данные SHA256-хэша
						::sprintf(&result[i * 2], "%02x", static_cast <uint32_t> (digest[i]));
					// Удаляем последний символ
					result.pop_back();
				} break;
				// Если тип хэш-суммы указан как HMAC_SHA384
				case static_cast <uint8_t> (awh::hash_t::type_t::SHA384): {
					// Выделяем память для буфера данных
					result.resize(97, 0);
					// Выполняем получение подписи
					const u_char * digest = ::HMAC(::EVP_sha384(), key.data(), key.size(), reinterpret_cast <const u_char *> (text.data()), text.size(), nullptr, nullptr);
					// Заполняем строку данными SHA384
					for(uint8_t i = 0; i < 48; i++)
						// Формируем данные SHA384-хэша
						::sprintf(&result[i * 2], "%02x", static_cast <uint32_t> (digest[i]));
					// Удаляем последний символ
					result.pop_back();
				} break;
				// Если тип хэш-суммы указан как HMAC_SHA512
				case static_cast <uint8_t> (awh::hash_t::type_t::SHA512): {
					// Выделяем память для буфера данных
					result.resize(129, 0);
					// Выполняем получение подписи
					const u_char * digest = ::HMAC(::EVP_sha512(), key.data(), key.size(), reinterpret_cast <const u_char *> (text.data()), text.size(), nullptr, nullptr);
					// Заполняем строку данными SHA512
					for(uint8_t i = 0; i < 64; i++)
						// Формируем данные SHA512-хэша
						::sprintf(&result[i * 2], "%02x", static_cast <uint32_t> (digest[i]));
					// Удаляем последний символ
					result.pop_back();
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception &) {
			// Выполняем очистку блока с результатом
			result.clear();
		}
	}
}
/**
 * Шаблон функции хэширования данных
 * @tparam T сигнатура функции
 */
template <typename T>
/**
 * hashing Функция хэширования данных
 * @param buffer буфер данных для шифрования
 * @param size   размер данных для шифрования
 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
 * @param state  объект стейта шифрования
 * @param result строка куда следует положить результат
 */
static void hashing(const char * buffer, const size_t size, const awh::hash_t::cipher_t cipher, const awh::hash_t::event_t event, awh::hash_t::state_t & state, T & result) noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем очистку блока с результатом
			result.clear();
			// Определяем тип шифрования
			switch(static_cast <uint8_t> (cipher)){
				// Если производится работы с BASE64
				case static_cast <uint8_t> (awh::hash_t::cipher_t::BASE64): {
					// Инициализируем объект BASE64
					BIO * b64 = ::BIO_new(BIO_f_base64());
					// Если объект BASE64 инициализирован
					if(b64 != nullptr){
						// Инициализируем объект BIO
						BIO * bio = ::BIO_new(BIO_s_mem());
						// Если объект BIO инициализирован
						if(bio != nullptr){
							// Размер обработанных данных
							ssize_t length = 0;
							// Устанавливаем флаги
							::BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
							// Записываем параметры
							::BIO_push(b64, bio);
							// Определяем событие кодирование или декодирование
							switch(static_cast <uint8_t> (event)){
								// Если производится кодирование данных
								case static_cast <uint8_t> (awh::hash_t::event_t::ENCODE): {
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
								case static_cast <uint8_t> (awh::hash_t::event_t::DECODE): {
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
							// Выполняем сброс результата
							else result.clear();
							// Очищаем всю выделенную память
							::BIO_free_all(bio);
						}
						// Очищаем объект BASE64
						::BIO_free(b64);
					}
				} break;
				// Если производится работы с AES128
				case static_cast <uint8_t> (awh::hash_t::cipher_t::AES128):
				// Если производится работы с AES192
				case static_cast <uint8_t> (awh::hash_t::cipher_t::AES192):
				// Если производится работы с AES256
				case static_cast <uint8_t> (awh::hash_t::cipher_t::AES256): {
					// Смещение в бинарном буфере
					size_t offset = 0;
					// Размер записываемых данных
					size_t length = 0;
					// Определяем размер данных для считывания
					size_t actual = size;
					// Выделяем память для буфера данных
					vector <u_char> output(AES_BLOCK_SIZE, 0);
					/**
					 * Выполняем шифровку всех данных
					 */
					do {
						// Максимальный размер считываемых данных
						length = (actual > AES_BLOCK_SIZE ? AES_BLOCK_SIZE : actual);
						// Определяем событие кодирование или декодирование
						switch(static_cast <uint8_t> (event)){
							// Если производится кодирование данных
							case static_cast <uint8_t> (awh::hash_t::event_t::ENCODE):
								// Выполняем сжатие данных
								::AES_cfb128_encrypt(reinterpret_cast <const u_char *> (buffer) + offset, output.data(), length, &state.key, state.ivec, &state.num, AES_ENCRYPT);
							break;
							// Если производится декодирование данных
							case static_cast <uint8_t> (awh::hash_t::event_t::DECODE):
								// Выполняем сжатие данных
								::AES_cfb128_encrypt(reinterpret_cast <const u_char *> (buffer) + offset, output.data(), length, &state.key, state.ivec, &state.num, AES_DECRYPT);
							break;
						}
						// Увеличиваем смещение
						offset += length;
						// Вычитаем считанные данные
						actual -= length;
						// Выполняем добавление полученных данных
						result.insert(result.end(), output.data(), output.data() + length);
					// Если данные ещё не зашифрованны
					} while(actual > 0);
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception &) {
			// Выполняем очистку блока с результатом
			result.clear();
		}
	}
}
/**
 * Шаблон функции работы с компрессором LZma
 * @tparam T сигнатура функции
 */
template <typename T>
/**
 * lzma Функция работы с компрессором LZma
 * @param buffer буфер данных для компрессии
 * @param size   размер данных для компрессии
 * @param event  событие выполнения операции
 * @param result строка куда следует положить результат
 */
static void lzma(const char * buffer, const size_t size, const awh::hash_t::event_t event, T & result) noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 80)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем очистку блока с результатом
			result.clear();
			// Определяем событие выполнения операции
			switch(static_cast <uint8_t> (event)){
				// Если необходимо выполнить компрессию данных
				case static_cast <uint8_t> (awh::hash_t::event_t::ENCODE): {
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
						// Выходим из функции
						return;
					}
					// Корректируем размер результирующего буфера
					result.resize(actual);
				} break;
				// Если необходимо выполнить декомпрессию данных
				case static_cast <uint8_t> (awh::hash_t::event_t::DECODE): {
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
					// Выходим из функции
					return;
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception &) {
			// Выполняем очистку блока с результатом
			result.clear();
		}
	}
}
/**
 * Шаблон функции работы с компрессором BZip2
 * @tparam T сигнатура функции
 */
template <typename T>
/**
 * bzip2 Функция работы с компрессором BZip2
 * @param buffer буфер данных для компрессии
 * @param size   размер данных для компрессии
 * @param event  событие выполнения операции
 * @param result строка куда следует положить результат
 */
static void bzip2(const char * buffer, const size_t size, const awh::hash_t::event_t event, T & result) noexcept {
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
			// Определяем событие выполнения операции
			switch(static_cast <uint8_t> (event)){
				// Если необходимо выполнить компрессию данных
				case static_cast <uint8_t> (awh::hash_t::event_t::ENCODE): {
					// Выполняем инициализацию потока
					if(::BZ2_bzCompressInit(&stream, 5, 0, 0) != BZ_OK){
						// Выполняем очистку результата
						result.clear();
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
					stream.next_out = result.data();
					// Устанавливаем максимальный размер буфера
					stream.avail_out = static_cast <uint32_t> (result.size());
					// Выполняем компрессию буфера бинарных данных
					while((rv = ::BZ2_bzCompress(&stream, BZ_FINISH)) != BZ_STREAM_END){
						// Выполняем ещё одну попытку компрессии
						rv = ::BZ2_bzCompress(&stream, BZ_FINISH);
						// Если произошла ошибка компрессии
						if((rv != BZ_FINISH_OK) && (rv != BZ_STREAM_END))
							// Выходим из цикла
							break;
					}
					// Если данные обработаны удачно
					if((rv == BZ_FINISH_OK) || (rv == BZ_STREAM_END))
						// Добавляем оставшиеся данные в список
						result.erase(result.begin() + (result.size() - stream.avail_out), result.end());
					// Выполняем очистку буфера данных
					else result.clear();
					// Выполняем очистку объекта потока
					::BZ2_bzCompressEnd(&stream);
				} break;
				// Если необходимо выполнить декомпрессию данных
				case static_cast <uint8_t> (awh::hash_t::event_t::DECODE): {
					// Выполняем инициализацию потока
					if(::BZ2_bzDecompressInit(&stream, 0, 0) != BZ_OK){
						// Выполняем очистку результата
						result.clear();
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
						stream.next_out = (result.data() + stream.total_out_lo32);
						// Устанавливаем максимальный размер буфера
						stream.avail_out = (actual - stream.total_out_lo32);
						// Выполняем декомпрессию
						rv = ::BZ2_bzDecompress(&stream);
						// Если мы завершили сбор данных
						if((rv == BZ_STREAM_END) || (rv == BZ_FINISH_OK))
							// Выходим из цикла
							break;
					// Если данные ещё не извлечены
					} while(rv == BZ_OK);
					// Если данные обработаны удачно
					if((rv == BZ_FINISH_OK) || (rv == BZ_STREAM_END))
						// Добавляем оставшиеся данные в список
						result.erase(result.begin() + (result.size() - stream.avail_out), result.end());
					// Выполняем очистку буфера данных
					else result.clear();
					// Выполняем очистку объекта потока
					::BZ2_bzDecompressEnd(&stream);
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception &) {
			// Выполняем очистку блока с результатом
			result.clear();
		}
	}
}
/**
 * Шаблон функции работы с компрессором Brotli
 * @tparam T сигнатура функции
 */
template <typename T>
/**
 * brotli Функция работы с компрессором Brotli
 * @param buffer буфер данных для компрессии
 * @param size   размер данных для компрессии
 * @param event  событие выполнения операции
 * @param result строка куда следует положить результат
 */
static void brotli(const char * buffer, const size_t size, const awh::hash_t::event_t event, T & result) noexcept {
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
			vector <uint8_t> data(CHUNK_BUFFER_SIZE, 0);
			// Получаем бинарный буфер входящих данных
			const uint8_t * nextInput = reinterpret_cast <const uint8_t *> (buffer);
			// Определяем событие выполнения операции
			switch(static_cast <uint8_t> (event)){
				// Если необходимо выполнить компрессию данных
				case static_cast <uint8_t> (awh::hash_t::event_t::ENCODE): {
					// Инициализируем стейт энкодера Brotli
					BrotliEncoderState * encoder = ::BrotliEncoderCreateInstance(nullptr, nullptr, nullptr);
					// Выполняем сжатие данных
					while(!::BrotliEncoderIsFinished(encoder)){
						// Получаем размер буфера закодированных бинарных данных
						size_t sizeOutput = data.size();
						// Получаем буфер закодированных бинарных данных
						uint8_t * nextOutput = data.data();
						// Если сжатие данных закончено, то завершаем работу
						if(!::BrotliEncoderCompressStream(encoder, BROTLI_OPERATION_FINISH, &sizeInput, &nextInput, &sizeOutput, &nextOutput, nullptr))
							// Выходим из цикла
							break;
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
				case static_cast <uint8_t> (awh::hash_t::event_t::DECODE): {
					// Полный размер обработанных данных
					size_t total = 0, size = 0;
					// Активируем работу декодера
					BrotliDecoderResult rbr = BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT;
					// Инициализируем стейт декодера Brotli
					BrotliDecoderState * decoder = ::BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
					// Если декодеру есть с чем работать
					while(rbr == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT){
						// Получаем размер буфера декодированных бинарных данных
						size_t sizeOutput = data.size();
						// Получаем буфер декодированных бинарных данных
						char * nextOutput = reinterpret_cast <char *> (data.data());
						// Выполняем декодирование бинарных данных
						rbr = ::BrotliDecoderDecompressStream(decoder, &sizeInput, &nextInput, &sizeOutput, reinterpret_cast <uint8_t **> (&nextOutput), &total);
						// Если декодирование данных не выполнено
						if(rbr == BROTLI_DECODER_RESULT_ERROR)
							// Выходим из цикла
							break;
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
					if((rbr != BROTLI_DECODER_RESULT_SUCCESS) && (rbr != BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT))
						// Выполняем очистку результата
						result.clear();
					// Освобождаем память декодера
					::BrotliDecoderDestroyInstance(decoder);
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception &) {
			// Выполняем очистку блока с результатом
			result.clear();
		}
	}
}
/**
 * Шаблон функции работы с компрессором Lz4
 * @tparam T сигнатура функции
 */
template <typename T>
/**
 * lz4 Функция работы с компрессором Lz4
 * @param buffer буфер данных
 * @param size   размер данных
 * @param level  уровень компрессии
 * @param event  событие выполнения операции
 * @param result строка куда следует положить результат
 */
static void lz4(const char * buffer, const size_t size, const uint32_t level, const awh::hash_t::event_t event, T & result) noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем очистку блока с результатом
			result.clear();
			// Определяем событие выполнения операции
			switch(static_cast <uint8_t> (event)){
				// Если необходимо выполнить компрессию данных
				case static_cast <uint8_t> (awh::hash_t::event_t::ENCODE): {
					// Выполняем получение размер результирующего буфера
					int32_t actual = ::LZ4_compressBound(size);
					// Если размер выделен
					if(actual <= 0){
						// Выполняем очистку результата
						result.clear();
						// Выходим из функции
						return;
					}
					// Выделяем буфер памяти нужного нам размера
					result.resize(actual, 0);
					// Выполняем компрессию буфера бинарных данных
					actual = ::LZ4_compress_fast(buffer, result.data(), size, actual, level);
					// Если компрессия не выполнена
					if((actual <= 0) || (static_cast <uint32_t> (actual) > static_cast <uint32_t> (size + size / 10))){
						// Выполняем очистку результата
						result.clear();
						// Выходим из функции
						return;
					}
					// Корректируем размер результирующего буфера
					result.resize(actual);
				} break;
				// Если необходимо выполнить декомпрессию данных
				case static_cast <uint8_t> (awh::hash_t::event_t::DECODE): {
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
						actual = ::LZ4_decompress_safe(buffer, result.data(), size, actual);
						// Если компрессия не выполнена из-за отсутствия памяти
						if(actual < 0)
							// Выполняем увеличение множителя
							factor++;
						// Если компрессия не выполнена
						else if(actual == 0){
							// Выполняем очистку результата
							result.clear();
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
		} catch(const exception &) {
			// Выполняем очистку блока с результатом
			result.clear();
		}
	}
}
/**
 * Шаблон функции работы с компрессором Zstandard
 * @tparam T сигнатура функции
 */
template <typename T>
/**
 * zstd Функция работы с компрессором Zstandard
 * @param buffer буфер данных
 * @param size   размер данных
 * @param level  уровень компрессии
 * @param event  событие выполнения операции
 * @param result строка куда следует положить результат
 */
static void zstd(const char * buffer, const size_t size, const uint32_t level, const awh::hash_t::event_t event, T & result) noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем очистку блока с результатом
			result.clear();
			// Определяем событие выполнения операции
			switch(static_cast <uint8_t> (event)){
				// Если необходимо выполнить компрессию данных
				case static_cast <uint8_t> (awh::hash_t::event_t::ENCODE): {
					// Выполняем создание контекста потока
					ZSTD_CStream * ctx = ::ZSTD_createCStream();
					// Если контекст потока создан
					if(ctx == nullptr){
						// Выполняем очистку результата
						result.clear();
						// Выходим из функции
						return;
					}
					// Выполняем инициализацию потока
					size_t status = ::ZSTD_initCStream(ctx, level);
					// Если мы получили ошибку инициализации
					if(::ZSTD_isError(status)){
						// Выполняем удаление потока
						::ZSTD_freeCStream(ctx);
						// Выводим сообщение об ошибке
						::fprintf(stderr, "Zstandard: %s\n", ::ZSTD_getErrorName(status));
						// Выполняем очистку результата
						result.clear();
						// Выходим из функции
						return;
					}
					// Инициализируем переменные смещения в буфере и актуальный размер данных
					size_t offset = 0, actual = 0;
					// Получаем длину итогового буфера данных
					const size_t length = ::ZSTD_CStreamOutSize();
					// Выполняем инициализацию итогового буфера данных
					const auto data = make_unique <char []> (length);
					// Выполняем создание буфера исходящих данных
					ZSTD_outBuffer output = {data.get(), length, 0};
					// Выполняем обработку всех входящих данных
					while(offset < size){
						// Определяем актуальный размер данных
						actual = (((size - offset) > static_cast <size_t> (::ZSTD_CStreamInSize())) ? static_cast <size_t> (::ZSTD_CStreamInSize()) : (size - offset));
						// Выполняем создание буфера данных для входящих сжатых данных
						ZSTD_inBuffer input = {buffer + offset, actual, 0};
						// Выполняем обработку до тех пор пока все не обработаем
						while(input.pos < input.size){
							// Сбрасываем позицию буфера
							output.pos = 0;
							// Выполняем компрессию полученных данных
							status = ::ZSTD_compressStream(ctx, &output, &input);
							// Если мы получили ошибку инициализации
							if(::ZSTD_isError(status)){
								// Выполняем удаление потока
								::ZSTD_freeCStream(ctx);
								// Выводим сообщение об ошибке
								::fprintf(stderr, "Zstandard: %s\n", ::ZSTD_getErrorName(status));
								// Выполняем очистку результата
								result.clear();
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
						// Выполняем удаление потока
						::ZSTD_freeCStream(ctx);
						// Выводим сообщение об ошибке
						::fprintf(stderr, "Zstandard: %s\n", ::ZSTD_getErrorName(status));
						// Выполняем очистку результата
						result.clear();
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
						// Выводим сообщение об ошибке
						::fprintf(stderr, "Zstandard: %s\n", ::ZSTD_getErrorName(actual));
						// Выполняем очистку результата
						result.clear();
						// Выходим из функции
						return;
					}
					// Корректируем размер результирующего буфера
					result.resize(actual);
					*/
				} break;
				// Если необходимо выполнить декомпрессию данных
				case static_cast <uint8_t> (awh::hash_t::event_t::DECODE): {
					// Выполняем создание контекста потока
					ZSTD_DStream * ctx = ::ZSTD_createDStream();
					// Если контекст потока создан
					if(ctx == nullptr){
						// Выполняем очистку результата
						result.clear();
						// Выходим из функции
						return;
					}
					// Выполняем инициализацию потока
					size_t status = ::ZSTD_initDStream(ctx);
					// Если мы получили ошибку инициализации
					if(::ZSTD_isError(status)){
						// Выполняем удаление потока
						::ZSTD_freeDStream(ctx);
						// Выводим сообщение об ошибке
						::fprintf(stderr, "Zstandard: %s\n", ::ZSTD_getErrorName(status));
						// Выполняем очистку результата
						result.clear();
						// Выходим из функции
						return;
					}
					// Инициализируем переменные смещения в буфере и актуальный размер данных
					size_t offset = 0, actual = 0;
					// Получаем длину итогового буфера данных
					const size_t length = ::ZSTD_DStreamOutSize();
					// Выполняем инициализацию итогового буфера данных
					const auto data = make_unique <char []> (length);
					// Выполняем создание буфера исходящих данных
					ZSTD_outBuffer output = {data.get(), length, 0};
					// Выполняем обработку всех входящих данных
					while(offset < size){
						// Определяем актуальный размер данных
						actual = (((size - offset) > static_cast <size_t> (::ZSTD_DStreamInSize())) ? static_cast <size_t> (::ZSTD_DStreamInSize()) : (size - offset));
						// Выполняем создание буфера данных для входящих сжатых данных
						ZSTD_inBuffer input = {buffer + offset, actual, 0};
						// Выполняем обработку до тех пор пока все не обработаем
						while(input.pos < input.size){
							// Сбрасываем позицию буфера
							output.pos = 0;
							// Выполняем декомпрессию полученных данных
							status = ::ZSTD_decompressStream(ctx, &output, &input);
							// Если мы получили ошибку инициализации
							if(::ZSTD_isError(status)){
								// Выполняем удаление потока
								::ZSTD_freeDStream(ctx);
								// Выводим сообщение об ошибке
								::fprintf(stderr, "Zstandard: %s\n", ::ZSTD_getErrorName(status));
								// Выполняем очистку результата
								result.clear();
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
						// Выводим сообщение об ошибке
						::fprintf(stderr, "Zstandard: %s\n", ::ZSTD_getErrorName(actual));
						// Выполняем очистку результата
						result.clear();
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
		} catch(const exception &) {
			// Выполняем очистку блока с результатом
			result.clear();
		}
	}
}
/**
 * Шаблон функции работы с компрессором GZip
 * @tparam T сигнатура функции
 */
template <typename T>
/**
 * gzip Функция работы с компрессором GZip
 * @param buffer буфер данных
 * @param size   размер данных
 * @param level  уровень компрессии
 * @param wbit   размер скользящего окна
 * @param event  событие выполнения операции
 * @param result строка куда следует положить результат
 */
static void gzip(const char * buffer, const size_t size, const uint32_t level, const int16_t wbit, const awh::hash_t::event_t event, T & result) noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем очистку блока с результатом
			result.clear();
			// Создаем поток zip
			z_stream zs;
			// Заполняем его нулями
			::memset(&zs, 0, sizeof(zs));
			// Определяем событие выполнения операции
			switch(static_cast <uint8_t> (event)){
				// Если необходимо выполнить компрессию данных
				case static_cast <uint8_t> (awh::hash_t::event_t::ENCODE): {
					// Результирующий размер данных
					int32_t rv = Z_OK;
					// Если поток инициализировать не удалось, выходим
					if(::deflateInit2(&zs, level, Z_DEFLATED, wbit | 16, MOD_GZIP_ZLIB_CFACTOR, Z_DEFAULT_STRATEGY) == Z_OK){
						// Указываем размер входного буфера
						zs.avail_in = static_cast <uint32_t> (size);
						// Заполняем входные данные буфера
						zs.next_in = reinterpret_cast <Bytef *> (const_cast <char *> (buffer));
						// Выделяем память на результирующий буфер
						result.resize(size, 0);
						/**
						 * Выполняем компрессию всех данных
						 */
						do {
							// Устанавливаем буфер для получения результата
							zs.next_out = reinterpret_cast <Bytef *> (result.data() + zs.total_out);
							// Устанавливаем максимальный размер буфера
							zs.avail_out = (static_cast <uint32_t> (size) - zs.total_out);
							// Выполняем сжатие
							rv = ::deflate(&zs, Z_FINISH);
							// Если произошла ошибка компрессии
							if((rv != Z_OK) && (rv != Z_STREAM_END))
								// Выходим из цикла
								break;
						// Если данные ещё не сжаты
						} while(rv == Z_OK);
					}
					// Если данные обработаны удачно
					if((rv == Z_OK) || (rv == Z_STREAM_END))
						// Добавляем оставшиеся данные в список
						result.erase(result.begin() + (result.size() - zs.avail_out), result.end());
					// Выполняем очистку буфера данных
					else result.clear();
					// Завершаем сжатие
					::deflateEnd(&zs);
				} break;
				// Если необходимо выполнить декомпрессию данных
				case static_cast <uint8_t> (awh::hash_t::event_t::DECODE): {
					// Результирующий размер данных
					int32_t rv = Z_OK;
					// Если поток инициализировать не удалось, выходим
					if(::inflateInit2(&zs, wbit | 16) == Z_OK){
						// Указываем размер входного буфера
						zs.avail_in = static_cast <uint32_t> (size);
						// Заполняем входные данные буфера
						zs.next_in = reinterpret_cast <Bytef *> (const_cast <char *> (buffer));
						// Размер буфера извлечённых данных
						uint32_t actual = (static_cast <uint32_t> (size) * 2);
						// Выделяем память на результирующий буфер
						result.resize(actual, 0);
						/**
						 * Выполняем декомпрессию всех данных
						 */
						do {
							// Если место для извлечения данных закончилось
							if((actual - zs.total_out) == 0){
								// Увеличиваем буфер исходящих данных в два раза
								actual *= 2;
								// Выделяем пмять для буфера извлечения данных
								result.resize(actual, 0);
							}
							// Устанавливаем буфер для получения результата
							zs.next_out = reinterpret_cast <Bytef *> (result.data() + zs.total_out);
							// Устанавливаем максимальный размер буфера
							zs.avail_out = (actual - zs.total_out);
							// Выполняем расжатие
							rv = ::inflate(&zs, 0);
							// Если мы завершили сбор данных
							if(rv == Z_STREAM_END)
								// Выходим из цикла
								break;
						// Если данные ещё не извлечены
						} while(rv == Z_OK);
					}
					// Если данные обработаны удачно
					if((rv == Z_OK) || (rv == Z_STREAM_END))
						// Добавляем оставшиеся данные в список
						result.erase(result.begin() + (result.size() - zs.avail_out), result.end());
					// Выполняем очистку буфера данных
					else result.clear();
					// Завершаем расжатие
					::inflateEnd(&zs);
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception &) {
			// Выполняем очистку блока с результатом
			result.clear();
		}
	}
}
/**
 * Шаблон функции работы с компрессором Deflate
 * @tparam T сигнатура функции
 */
template <typename T>
/**
 * deflate Функция работы с компрессором Deflate
 * @param buffer   буфер данных для компрессии
 * @param size     размер данных для компрессии
 * @param level    уровень компрессии
 * @param wbit     размер скользящего окна
 * @param takeOver флаг переиспользования контекста
 * @param stream   объект потока для работы компрессора
 * @param event    событие выполнения операции
 * @param result   строка куда следует положить результат
 */
static void deflate(const char * buffer, const size_t size, const uint32_t level, const int16_t wbit, const bool takeOver, z_stream & stream, const awh::hash_t::event_t event, T & result) noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем очистку блока с результатом
			result.clear();
			// Результат проверки декомпрессии
			int32_t rv = Z_OK;
			// Создаем поток zip
			z_stream zs = {0};
			// Обнуляем структуру
			zs.zfree  = Z_NULL;
			zs.zalloc = Z_NULL;
			zs.opaque = Z_NULL;
			// Буфер выходных данных
			vector <Bytef> tmp(size, 0);
			// Определяем событие выполнения операции
			switch(static_cast <uint8_t> (event)){
				// Если необходимо выполнить компрессию данных
				case static_cast <uint8_t> (awh::hash_t::event_t::ENCODE): {
					// Если поток инициализировать не удалось, выходим
					if(takeOver || (::deflateInit2(&zs, level, Z_DEFLATED, -1 * wbit, DEFAULT_MEM_LEVEL, Z_HUFFMAN_ONLY) == Z_OK)){
						// Если поток декомпрессора не создан ранее
						if(!takeOver){
							// Устанавливаем количество доступных данных
							zs.avail_in = static_cast <uint32_t> (size);
							// Устанавливаем буфер с данными для шифрования
							zs.next_in = const_cast <Bytef *> (reinterpret_cast <const Bytef *> (buffer));
						// Если нужно переиспользовать поток декомпрессора
						} else {
							// Устанавливаем количество доступных данных
							stream.avail_in = static_cast <uint32_t> (size);
							// Устанавливаем буфер с данными для шифрования
							stream.next_in = const_cast <Bytef *> (reinterpret_cast <const Bytef *> (buffer));
						}
						/**
						 * Выполняем компрессию всех данных
						 */
						do {
							// Если поток декомпрессора не создан ранее
							if(!takeOver){
								// Устанавливаем буфер для записи шифрованных данных
								zs.next_out = tmp.data();
								// Устанавливаем количество доступных данных для записи
								zs.avail_out = static_cast <uint32_t> (tmp.size());
								// Выполняем сжатие данных
								rv = ::deflate(&zs, Z_SYNC_FLUSH);
							// Если нужно переиспользовать поток декомпрессора
							} else {
								// Устанавливаем буфер для записи шифрованных данных
								stream.next_out = tmp.data();
								// Устанавливаем количество доступных данных для записи
								stream.avail_out = static_cast <uint32_t> (tmp.size());
								// Выполняем сжатие данных
								rv = ::deflate(&stream, Z_FULL_FLUSH);
							}
							// Если данные обработаны удачно
							if((rv == Z_OK) || (rv == Z_STREAM_END))
								// Добавляем оставшиеся данные в список
								result.insert(result.end(), tmp.begin(), tmp.begin() + (static_cast <uint32_t> (tmp.size()) - (!takeOver ? zs.avail_out : stream.avail_out)));
							// Если данные не могут быть обработанны, то выходим
							else break;
						// Если ещё не все данные сжаты
						} while(rv != Z_STREAM_END);
						// Закрываем поток
						if(!takeOver)
							// Завершаем работу
							::deflateEnd(&zs);
					}
				} break;
				// Если необходимо выполнить декомпрессию данных
				case static_cast <uint8_t> (awh::hash_t::event_t::DECODE): {
					// Если поток инициализировать не удалось, выходим
					if(takeOver || (::inflateInit2(&zs, -1 * wbit) == Z_OK)){
						// Если поток декомпрессора не создан ранее
						if(!takeOver){
							// Устанавливаем количество доступных данных
							zs.avail_in = static_cast <uint32_t> (size);
							// Копируем входящий буфер для дешифровки
							zs.next_in = const_cast <Bytef *> (reinterpret_cast <const Bytef *> (buffer));
						// Если нужно переиспользовать поток декомпрессора
						} else {
							// Устанавливаем количество доступных данных
							stream.avail_in = static_cast <uint32_t> (size);
							// Копируем входящий буфер для дешифровки
							stream.next_in = const_cast <Bytef *> (reinterpret_cast <const Bytef *> (buffer));
						}
						/**
						 * Выполняем декомпрессию всех данных
						 */
						do {
							// Если поток декомпрессора не создан ранее
							if(!takeOver){
								// Устанавливаем буфер для записи дешифрованных данных
								zs.next_out = tmp.data();
								// Устанавливаем количество доступных данных для записи
								zs.avail_out = static_cast <uint32_t> (tmp.size());
								// Выполняем декомпрессию данных
								rv = ::inflate(&zs, Z_NO_FLUSH);
							// Если нужно переиспользовать поток декомпрессора
							} else {
								// Устанавливаем буфер для записи дешифрованных данных
								stream.next_out = tmp.data();
								// Устанавливаем количество доступных данных для записи
								stream.avail_out = static_cast <uint32_t> (tmp.size());
								// Выполняем декомпрессию данных
								rv = ::inflate(&stream, Z_SYNC_FLUSH);
							}
							// Если данные обработаны удачно
							if((rv == Z_OK) || (rv == Z_STREAM_END))
								// Добавляем оставшиеся данные в список
								result.insert(result.end(), tmp.begin(), tmp.begin() + (static_cast <uint32_t> (tmp.size()) - (!takeOver ? zs.avail_out : stream.avail_out)));
							// Если данные не могут быть обработанны, то выходим
							else break;
						// Если ещё не все данные извлечены
						} while(rv != Z_STREAM_END);
						// Очищаем выделенную память для декомпрессора
						if(!takeOver)
							// Завершаем работу
							::inflateEnd(&zs);
					}
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception &) {
			// Выполняем очистку блока с результатом
			result.clear();
		}
	}
}
/**
 * cipher Метод инициализации AES шифрования
 * @param cipher тип шифрования (AES128, AES192, AES256)
 * @return       результат инициализации
 */
bool awh::Hash::cipher(const cipher_t cipher) noexcept {
	// Формируем массивы для шифрования
	vector <u_char> iv, key;
	// Создаем тип шифрования
	const EVP_CIPHER * evp = EVP_enc_null();
	// Определяем длину шифрования
	switch(static_cast <uint16_t> (cipher)){
		// Устанавливаем шифрование в 128
		case static_cast <uint16_t> (cipher_t::AES128): {
			// Устанавливаем размер массива IV
			iv.resize(8, 0);
			// Устанавливаем размер массива KEY
			key.resize(16, 0);
			// Устанавливаем функцию шифрования
			evp = ::EVP_aes_128_ecb();
		} break;
		// Устанавливаем шифрование в 192
		case static_cast <uint16_t> (cipher_t::AES192): {
			// Устанавливаем размер массива IV
			iv.resize(12, 0);
			// Устанавливаем размер массива KEY
			key.resize(24, 0);
			// Устанавливаем функцию шифрования
			evp = ::EVP_aes_192_ecb();
		} break;
		// Устанавливаем шифрование в 256
		case static_cast <uint16_t> (cipher_t::AES256): {
			// Устанавливаем размер массива IV
			iv.resize(16, 0);
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
	if(ctx != nullptr){
		// Привязываем контекст к типу шифрования
		::EVP_EncryptInit_ex(ctx, evp, nullptr, nullptr, nullptr);
		/*
		// Выделяем нужное количество памяти
		vector <u_char> iv(::EVP_CIPHER_CTX_iv_length(ctx), 0);
		vector <u_char> key(::EVP_CIPHER_CTX_key_length(ctx), 0);
		*/
		// Выполняем инициализацию ключа
		const int32_t ok = EVP_BytesToKey(
			evp, ::EVP_sha256(),
			(this->_salt.empty() ? nullptr : reinterpret_cast <u_char *> (const_cast <hash_t *> (this)->_salt.data())),
			reinterpret_cast <u_char *> (const_cast <hash_t *> (this)->_password.data()),
			this->_password.length(), this->_rounds, key.data(), iv.data()
		);
		// Очищаем контекст
		::EVP_CIPHER_CTX_free(ctx);
		// Если инициализация не произошла
		if(ok == 0)
			// Выходим из функции
			return false;
		// Устанавливаем ключ шифрования
		if(::AES_set_encrypt_key(key.data(), key.size() * 8, &this->_state.key) != 0)
			// Выходим из функции
			return false;
		// Обнуляем номер
		this->_state.num = 0;
		// Заполняем половину структуры нулями
		::memset(this->_state.ivec, 0, sizeof(this->_state.ivec));
		// Копируем данные шифрования
		::memcpy(this->_state.ivec, iv.data(), iv.size());
		// Выполняем шифрование
		// ::AES_encrypt(this->_state.ivec, this->_state.count, &this->_state.key);
	// Выводим сообщение об ошибке
	} else this->_log->print("%s", log_t::flag_t::CRITICAL, "Context for encryption/decryption could not be initialized");
	// Сообщаем что всё удачно
	return true;
}
/**
 * rmTail Метод удаления хвостовых данных
 * @param buffer буфер для удаления хвоста
 */
void awh::Hash::rmTail(vector <char> & buffer) const noexcept {
	// Если сообщение является финальным
	if(buffer.size() > sizeof(this->_btype)){
		// Выполняем поиск хвостового списка байт для удаления
		auto i = search(buffer.begin(), buffer.end(), this->_btype, this->_btype + sizeof(this->_btype));
		// Удаляем хвостовой список байт из буфера данных
		buffer.erase(i, buffer.end());
	}
}
/**
 * setTail Метод добавления хвостовых данных
 * @param buffer буфер для добавления хвоста
 */
void awh::Hash::setTail(vector <char> & buffer) const noexcept {
	// Добавляем хвостовой буфер в полезную нагрузку
	buffer.insert(buffer.end(), this->_btype, this->_btype + sizeof(this->_btype));
}
/**
 * hashing Метод хэширования текста
 * @param text   текст для хэширования
 * @param type   тип хэш-суммы
 * @param result строка куда следует положить результат
 */
void awh::Hash::hashing(const string & text, const type_t type, string & result) const noexcept {
	// Если текст для хэширования передан
	if(!text.empty()){
		// Выполняем хэширование
		::hashing(text, type, result);
		// Если хэширование не вышло
		if(result.empty())
			// Выводим сообщение об ошибке
			this->_log->print("Text hashing \"%s\" could not be performed", log_t::flag_t::WARNING, text.c_str());
	}
}
/**
 * hashing Метод хэширования текста
 * @param text   текст для хэширования
 * @param type   тип хэш-суммы
 * @param result буфер куда следует положить результат
 */
void awh::Hash::hashing(const string & text, const type_t type, vector <char> & result) const noexcept {
	// Если текст для хэширования передан
	if(!text.empty()){
		// Выполняем хэширование
		::hashing(text, type, result);
		// Если хэширование не вышло
		if(result.empty())
			// Выводим сообщение об ошибке
			this->_log->print("Text hashing \"%s\" could not be performed", log_t::flag_t::WARNING, text.c_str());
	}
}
/**
 * hmac Метод хэширования текста с ключом
 * @param key    ключ для подписи
 * @param text   текст для хэширования
 * @param type   тип хэш-суммы
 * @param result строка куда следует положить результат
 */
void awh::Hash::hmac(const string & key, const string & text, const type_t type, string & result) const noexcept {
	// Если ключ и текст для хэширования переданы
	if(!key.empty() && !text.empty()){
		// Выполняем хэширование
		::hmac(key, text, type, result);
		// Если хэширование не вышло
		if(result.empty())
			// Выводим сообщение об ошибке
			this->_log->print("Key \"%s\" and text \"%s\" hashing  could not be performed", log_t::flag_t::WARNING, key.c_str(), text.c_str());
	}
}
/**
 * hmac Метод хэширования текста с ключом
 * @param key    ключ для подписи
 * @param text   текст для хэширования
 * @param type   тип хэш-суммы
 * @param result буфер куда следует положить результат
 */
void awh::Hash::hmac(const string & key, const string & text, const type_t type, vector <char> & result) const noexcept {
	// Если ключ и текст для хэширования переданы
	if(!key.empty() && !text.empty()){
		// Выполняем хэширование
		::hmac(key, text, type, result);
		// Если хэширование не вышло
		if(result.empty())
			// Выводим сообщение об ошибке
			this->_log->print("Key \"%s\" and text \"%s\" hashing  could not be performed", log_t::flag_t::WARNING, key.c_str(), text.c_str());
	}
}
/**
 * encode Метод кодирования
 * @param buffer буфер данных для шифрования
 * @param size   размер данных для шифрования
 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
 * @param result строка куда следует положить результат
 */
void awh::Hash::encode(const char * buffer, const size_t size, const cipher_t cipher, string & result) const noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		// Определяем тип шифрования
		switch(static_cast <uint8_t> (cipher)){
			// Если производится работы с BASE64
			case static_cast <uint8_t> (hash_t::cipher_t::BASE64): {
				// Выполняем кодирование строки BASE64
				::hashing(buffer, size, cipher, event_t::ENCODE, const_cast <state_t &> (this->_state), result);
				// Если кодирование не вышло
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Unable to encode \"%s\" string data into BASE64 format", log_t::flag_t::WARNING, string(buffer, size).c_str());
			} break;
			// Если производится работы с AES128
			case static_cast <uint8_t> (hash_t::cipher_t::AES128):
			// Если производится работы с AES192
			case static_cast <uint8_t> (hash_t::cipher_t::AES192):
			// Если производится работы с AES256
			case static_cast <uint8_t> (hash_t::cipher_t::AES256): {
				// Если пароль установлен
				if(!this->_password.empty()){
					// Выполняем инициализацию AES
					if(const_cast <hash_t *> (this)->cipher(cipher))
						// Выполняем шифрование данных
						::hashing(buffer, size, cipher, event_t::ENCODE, const_cast <state_t &> (this->_state), result);
				}
				// Если кодирование не вышло
				if(result.empty()){
					// Выводим сообщение об ошибке
					this->_log->print("Unable to encode data into AES", log_t::flag_t::WARNING);
					// Выводим тот же самый буфер как он был передан
					result.assign(buffer, buffer + size);
				}
			} break;
		}
	}
}
/**
 * encode Метод кодирования
 * @param buffer буфер данных для шифрования
 * @param size   размер данных для шифрования
 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
 * @param result буфер куда следует положить результат
 */
void awh::Hash::encode(const char * buffer, const size_t size, const cipher_t cipher, vector <char> & result) const noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		// Определяем тип шифрования
		switch(static_cast <uint8_t> (cipher)){
			// Если производится работы с BASE64
			case static_cast <uint8_t> (hash_t::cipher_t::BASE64): {
				// Выполняем кодирование строки BASE64
				::hashing(buffer, size, cipher, event_t::ENCODE, const_cast <state_t &> (this->_state), result);
				// Если кодирование не вышло
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Unable to encode \"%s\" string data into BASE64 format", log_t::flag_t::WARNING, string(buffer, size).c_str());
			} break;
			// Если производится работы с AES128
			case static_cast <uint8_t> (hash_t::cipher_t::AES128):
			// Если производится работы с AES192
			case static_cast <uint8_t> (hash_t::cipher_t::AES192):
			// Если производится работы с AES256
			case static_cast <uint8_t> (hash_t::cipher_t::AES256): {
				// Если пароль установлен
				if(!this->_password.empty()){
					// Выполняем инициализацию AES
					if(const_cast <hash_t *> (this)->cipher(cipher))
						// Выполняем шифрование данных
						::hashing(buffer, size, cipher, event_t::ENCODE, const_cast <state_t &> (this->_state), result);
				}
				// Если кодирование не вышло
				if(result.empty()){
					// Выводим сообщение об ошибке
					this->_log->print("Unable to encode data into AES", log_t::flag_t::WARNING);
					// Выводим тот же самый буфер как он был передан
					result.assign(buffer, buffer + size);
				}
			} break;
		}
	}
}
/**
 * decode Метод декодирования
 * @param buffer буфер данных для шифрования
 * @param size   размер данных для шифрования
 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
 * @param result строка куда следует положить результат
 */
void awh::Hash::decode(const char * buffer, const size_t size, const cipher_t cipher, string & result) const noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		// Определяем тип шифрования
		switch(static_cast <uint8_t> (cipher)){
			// Если производится работы с BASE64
			case static_cast <uint8_t> (hash_t::cipher_t::BASE64): {
				// Выполняем декодирование строки BASE64
				::hashing(buffer, size, cipher, event_t::DECODE, const_cast <state_t &> (this->_state), result);
				// Если декодирование не вышло
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Unable to extract data from BASE64 encoded \"%s\" hash", log_t::flag_t::WARNING, string(buffer, size).c_str());
			} break;
			// Если производится работы с AES128
			case static_cast <uint8_t> (hash_t::cipher_t::AES128):
			// Если производится работы с AES192
			case static_cast <uint8_t> (hash_t::cipher_t::AES192):
			// Если производится работы с AES256
			case static_cast <uint8_t> (hash_t::cipher_t::AES256): {
				// Если пароль установлен
				if(!this->_password.empty()){
					// Выполняем инициализацию AES
					if(const_cast <hash_t *> (this)->cipher(cipher))
						// Выполняем шифрование данных
						::hashing(buffer, size, cipher, event_t::DECODE, const_cast <state_t &> (this->_state), result);
				}
				// Если кодирование не вышло
				if(result.empty()){
					// Выводим сообщение об ошибке
					this->_log->print("Unable to decode data from AES", log_t::flag_t::WARNING);
					// Выводим тот же самый буфер как он был передан
					result.assign(buffer, buffer + size);
				}
			} break;
		}
	}
}
/**
 * decode Метод декодирования
 * @param buffer буфер данных для шифрования
 * @param size   размер данных для шифрования
 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
 * @param result буфер куда следует положить результат
 */
void awh::Hash::decode(const char * buffer, const size_t size, const cipher_t cipher, vector <char> & result) const noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		// Определяем тип шифрования
		switch(static_cast <uint8_t> (cipher)){
			// Если производится работы с BASE64
			case static_cast <uint8_t> (hash_t::cipher_t::BASE64): {
				// Выполняем декодирование строки BASE64
				::hashing(buffer, size, cipher, event_t::DECODE, const_cast <state_t &> (this->_state), result);
				// Если декодирование не вышло
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Unable to extract data from BASE64 encoded \"%s\" hash", log_t::flag_t::WARNING, string(buffer, size).c_str());
			} break;
			// Если производится работы с AES128
			case static_cast <uint8_t> (hash_t::cipher_t::AES128):
			// Если производится работы с AES192
			case static_cast <uint8_t> (hash_t::cipher_t::AES192):
			// Если производится работы с AES256
			case static_cast <uint8_t> (hash_t::cipher_t::AES256): {
				// Если пароль установлен
				if(!this->_password.empty()){
					// Выполняем инициализацию AES
					if(const_cast <hash_t *> (this)->cipher(cipher))
						// Выполняем шифрование данных
						::hashing(buffer, size, cipher, event_t::DECODE, const_cast <state_t &> (this->_state), result);
				}
				// Если кодирование не вышло
				if(result.empty()){
					// Выводим сообщение об ошибке
					this->_log->print("Unable to decode data from AES", log_t::flag_t::WARNING);
					// Выводим тот же самый буфер как он был передан
					result.assign(buffer, buffer + size);
				}
			} break;
		}
	}
}
/**
 * compress Метод компрессии данных
 * @param buffer буфер данных для компрессии
 * @param size   размер данных для компрессии
 * @param method метод компрессии
 * @param result строка куда следует положить результат
 */
void awh::Hash::compress(const char * buffer, const size_t size, const method_t method, string & result) const noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		// Определяем метод компрессии данных
		switch(static_cast <uint8_t> (method)){
			// Если метод компрессии установлен Lz4
			case static_cast <uint8_t> (method_t::LZ4): {
				// Выполняем компрессию данных методом Lz4
				::lz4(buffer, size, this->_level[0], event_t::ENCODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Lz4: %s", log_t::flag_t::WARNING, "compress failed");
			} break;
			// Если метод компрессии установлен LZma
			case static_cast <uint8_t> (method_t::LZMA): {
				// Выполняем компрессию данных методом LZma
				::lzma(buffer, size, event_t::ENCODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("LZma: %s", log_t::flag_t::WARNING, "compress failed");
			} break;
			// Если метод компрессии установлен Zstandard
			case static_cast <uint8_t> (method_t::ZSTD): {
				// Выполняем компрессию данных методом Zstandard
				::zstd(buffer, size, this->_level[2], event_t::ENCODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Zstandard: %s", log_t::flag_t::WARNING, "compress failed");
			} break;
			// Если метод компрессии установлен GZip
			case static_cast <uint8_t> (method_t::GZIP): {
				// Выполняем компрессию данных методом GZip
				::gzip(buffer, size, this->_level[1], this->_wbit, event_t::ENCODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("GZip: %s", log_t::flag_t::WARNING, "compress failed");
			} break;
			// Если метод компрессии установлен BZip2
			case static_cast <uint8_t> (method_t::BZIP2): {
				// Выполняем компрессию данных методом BZip2
				::bzip2(buffer, size, event_t::ENCODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("BZip2: %s", log_t::flag_t::WARNING, "compress failed");
			} break;
			// Если метод компрессии установлен Brotli
			case static_cast <uint8_t> (method_t::BROTLI): {
				// Выполняем компрессию данных методом Brotli
				::brotli(buffer, size, event_t::ENCODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Brotli: %s", log_t::flag_t::WARNING, "compress failed");
			} break;
			// Если метод компрессии установлен Deflate
			case static_cast <uint8_t> (method_t::DEFLATE): {
				// Выполняем компрессию данных методом Deflate
				::deflate(buffer, size, this->_level[1], this->_wbit, this->_takeOverCompress, this->_zdef, event_t::ENCODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Deflate: %s", log_t::flag_t::WARNING, "compress failed");
			} break;
			// Если метод компрессии не установлен
			case static_cast <uint8_t> (method_t::NONE):
				// Выводим переданный буфер данных
				result.assign(buffer, buffer + size);
			break;
		}
	}
}
/**
 * compress Метод компрессии данных
 * @param buffer буфер данных для компрессии
 * @param size   размер данных для компрессии
 * @param method метод компрессии
 * @param result буфер куда следует положить результат
 */
void awh::Hash::compress(const char * buffer, const size_t size, const method_t method, vector <char> & result) const noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		// Определяем метод компрессии данных
		switch(static_cast <uint8_t> (method)){
			// Если метод компрессии установлен Lz4
			case static_cast <uint8_t> (method_t::LZ4): {
				// Выполняем компрессию данных методом Lz4
				::lz4(buffer, size, this->_level[0], event_t::ENCODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Lz4: %s", log_t::flag_t::WARNING, "compress failed");
			} break;
			// Если метод компрессии установлен LZma
			case static_cast <uint8_t> (method_t::LZMA): {
				// Выполняем компрессию данных методом LZma
				::lzma(buffer, size, event_t::ENCODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("LZma: %s", log_t::flag_t::WARNING, "compress failed");
			} break;
			// Если метод компрессии установлен Zstandard
			case static_cast <uint8_t> (method_t::ZSTD): {
				// Выполняем компрессию данных методом Zstandard
				::zstd(buffer, size, this->_level[2], event_t::ENCODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Zstandard: %s", log_t::flag_t::WARNING, "compress failed");
			} break;
			// Если метод компрессии установлен GZip
			case static_cast <uint8_t> (method_t::GZIP): {
				// Выполняем компрессию данных методом GZip
				::gzip(buffer, size, this->_level[1], this->_wbit, event_t::ENCODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("GZip: %s", log_t::flag_t::WARNING, "compress failed");
			} break;
			// Если метод компрессии установлен BZip2
			case static_cast <uint8_t> (method_t::BZIP2): {
				// Выполняем компрессию данных методом BZip2
				::bzip2(buffer, size, event_t::ENCODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("BZip2: %s", log_t::flag_t::WARNING, "compress failed");
			} break;
			// Если метод компрессии установлен Brotli
			case static_cast <uint8_t> (method_t::BROTLI): {
				// Выполняем компрессию данных методом Brotli
				::brotli(buffer, size, event_t::ENCODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Brotli: %s", log_t::flag_t::WARNING, "compress failed");
			} break;
			// Если метод компрессии установлен Deflate
			case static_cast <uint8_t> (method_t::DEFLATE): {
				// Выполняем компрессию данных методом Deflate
				::deflate(buffer, size, this->_level[1], this->_wbit, this->_takeOverCompress, this->_zdef, event_t::ENCODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Deflate: %s", log_t::flag_t::WARNING, "compress failed");
			} break;
			// Если метод компрессии не установлен
			case static_cast <uint8_t> (method_t::NONE):
				// Выводим переданный буфер данных
				result.assign(buffer, buffer + size);
			break;
		}
	}
}
/**
 * decompress Метод декомпрессии данных
 * @param buffer буфер данных для декомпрессии
 * @param size   размер данных для декомпрессии
 * @param method метод компрессии
 * @param result строка куда следует положить результат
 */
void awh::Hash::decompress(const char * buffer, const size_t size, const method_t method, string & result) const noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		// Определяем метод декомпрессии данных
		switch(static_cast <uint8_t> (method)){
			// Если метод декомпрессии установлен Lz4
			case static_cast <uint8_t> (method_t::LZ4): {
				// Выполняем декомпрессию данных методом Lz4
				::lz4(buffer, size, this->_level[0], event_t::DECODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Lz4: %s", log_t::flag_t::WARNING, "decompress failed");
			} break;
			// Если метод декомпрессии установлен LZma
			case static_cast <uint8_t> (method_t::LZMA): {
				// Выполняем декомпрессию данных методом LZma
				::lzma(buffer, size, event_t::DECODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("LZma: %s", log_t::flag_t::WARNING, "decompress failed");
			} break;
			// Если метод декомпрессии установлен Zstandard
			case static_cast <uint8_t> (method_t::ZSTD): {
				// Выполняем декомпрессию данных методом Zstandard
				::zstd(buffer, size, this->_level[2], event_t::DECODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Zstandard: %s", log_t::flag_t::WARNING, "decompress failed");
			} break;
			// Если метод декомпрессии установлен GZip
			case static_cast <uint8_t> (method_t::GZIP): {
				// Выполняем декомпрессию данных методом GZip
				::gzip(buffer, size, this->_level[1], this->_wbit, event_t::DECODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("GZip: %s", log_t::flag_t::WARNING, "decompress failed");
			} break;
			// Если метод декомпрессии установлен BZip2
			case static_cast <uint8_t> (method_t::BZIP2): {
				// Выполняем декомпрессию данных методом BZip2
				::bzip2(buffer, size, event_t::DECODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("BZip2: %s", log_t::flag_t::WARNING, "decompress failed");
			} break;
			// Если метод декомпрессии установлен Brotli
			case static_cast <uint8_t> (method_t::BROTLI): {
				// Выполняем декомпрессию данных методом Brotli
				::brotli(buffer, size, event_t::DECODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Brotli: %s", log_t::flag_t::WARNING, "decompress failed");
			} break;
			// Если метод декомпрессии установлен Deflate
			case static_cast <uint8_t> (method_t::DEFLATE): {
				// Выполняем декомпрессию данных методом Deflate
				::deflate(buffer, size, this->_level[1], this->_wbit, this->_takeOverDecompress, this->_zinf, event_t::DECODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Deflate: %s", log_t::flag_t::WARNING, "decompress failed");
			} break;
			// Если метод декомпрессии не установлен
			case static_cast <uint8_t> (method_t::NONE):
				// Выводим переданный буфер данных
				result.assign(buffer, buffer + size);
			break;
		}
	}
}
/**
 * decompress Метод декомпрессии данных
 * @param buffer буфер данных для декомпрессии
 * @param size   размер данных для декомпрессии
 * @param method метод компрессии
 * @param result буфер куда следует положить результат
 */
void awh::Hash::decompress(const char * buffer, const size_t size, const method_t method, vector <char> & result) const noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		// Определяем метод декомпрессии данных
		switch(static_cast <uint8_t> (method)){
			// Если метод декомпрессии установлен Lz4
			case static_cast <uint8_t> (method_t::LZ4): {
				// Выполняем декомпрессию данных методом Lz4
				::lz4(buffer, size, this->_level[0], event_t::DECODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Lz4: %s", log_t::flag_t::WARNING, "decompress failed");
			} break;
			// Если метод декомпрессии установлен LZma
			case static_cast <uint8_t> (method_t::LZMA): {
				// Выполняем декомпрессию данных методом LZma
				::lzma(buffer, size, event_t::DECODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("LZma: %s", log_t::flag_t::WARNING, "decompress failed");
			} break;
			// Если метод декомпрессии установлен Zstandard
			case static_cast <uint8_t> (method_t::ZSTD): {
				// Выполняем декомпрессию данных методом Zstandard
				::zstd(buffer, size, this->_level[2], event_t::DECODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Zstandard: %s", log_t::flag_t::WARNING, "decompress failed");
			} break;
			// Если метод декомпрессии установлен GZip
			case static_cast <uint8_t> (method_t::GZIP): {
				// Выполняем декомпрессию данных методом GZip
				::gzip(buffer, size, this->_level[1], this->_wbit, event_t::DECODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("GZip: %s", log_t::flag_t::WARNING, "decompress failed");
			} break;
			// Если метод декомпрессии установлен BZip2
			case static_cast <uint8_t> (method_t::BZIP2): {
				// Выполняем декомпрессию данных методом BZip2
				::bzip2(buffer, size, event_t::DECODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("BZip2: %s", log_t::flag_t::WARNING, "decompress failed");
			} break;
			// Если метод декомпрессии установлен Brotli
			case static_cast <uint8_t> (method_t::BROTLI): {
				// Выполняем декомпрессию данных методом Brotli
				::brotli(buffer, size, event_t::DECODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Brotli: %s", log_t::flag_t::WARNING, "decompress failed");
			} break;
			// Если метод декомпрессии установлен Deflate
			case static_cast <uint8_t> (method_t::DEFLATE): {
				// Выполняем декомпрессию данных методом Deflate
				::deflate(buffer, size, this->_level[1], this->_wbit, this->_takeOverDecompress, this->_zinf, event_t::DECODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Deflate: %s", log_t::flag_t::WARNING, "decompress failed");
			} break;
			// Если метод декомпрессии не установлен
			case static_cast <uint8_t> (method_t::NONE):
				// Выводим переданный буфер данных
				result.assign(buffer, buffer + size);
			break;
		}
	}
}
/**
 * wbit Метод установки размера скользящего окна
 * @param wbit размер скользящего окна
 */
void awh::Hash::wbit(const int16_t wbit) noexcept {
	// Устанавливаем размер скользящего окна
	this->_wbit = wbit;
	// Выполняем пересборку контекстов LZ77 для компрессии
	this->takeoverCompress(this->_takeOverCompress);
	// Выполняем пересборку контекстов LZ77 для декомпрессии
	this->takeoverDecompress(this->_takeOverDecompress);
}
/**
 * round Метод установки количества раундов шифрования
 * @param round количество раундов шифрования
 */
void awh::Hash::round(const int32_t round) noexcept {
	// Устанавливаем количество раундов шифрования
	this->_rounds = round;
}
/**
 * level Метод установки уровня компрессии
 * @param level уровень компрессии
 */
void awh::Hash::level(const level_t level) noexcept {
	// Определяем переданный уровень компрессии
	switch(static_cast <uint8_t> (level)){
		// Выполняем установку максимального уровня компрессии
		case static_cast <uint8_t> (level_t::BEST): {
			// Выполняем установку уровня максимальной компрессии Lz4
			this->_level[0] = 0;
			// Выполняем установку уровня компрессии GZip
			this->_level[1] = Z_BEST_COMPRESSION;
			// Выполняем установку уровня максимальной компрессии Zstandard
			this->_level[2] = 100;
		} break;
		// Выполняем установку уровень компрессии на максимальную производительность
		case static_cast <uint8_t> (level_t::SPEED): {
			// Выполняем установку уровня максимальной компрессии Lz4
			this->_level[0] = 3;
			// Выполняем установку уровня компрессии GZip
			this->_level[1] = Z_BEST_SPEED;
			// Выполняем установку уровня максимальной компрессии Zstandard
			this->_level[2] = ZSTD_CLEVEL_DEFAULT;
		} break;
		// Выполняем установку нормального уровня компрессии
		case static_cast <uint8_t> (level_t::NORMAL): {
			// Выполняем установку уровня максимальной компрессии Lz4
			this->_level[0] = 1;
			// Выполняем установку уровня компрессии GZip
			this->_level[1] = Z_DEFAULT_COMPRESSION;
			// Выполняем установку уровня максимальной компрессии Zstandard
			this->_level[2] = 22;
		} break;
	}
}
/**
 * salt Метод установки соли шифрования
 * @param salt соль для шифрования
 */
void awh::Hash::salt(const string & salt) noexcept {
	// Если соль передана
	this->_salt = salt;
}
/**
 * password Метод установки пароля шифрования
 * @param password пароль шифрования
 */
void awh::Hash::password(const string & password) noexcept {
	// Если пароль передан
	this->_password = password;
}
/**
 * takeoverCompress Метод установки флага переиспользования контекста компрессии
 * @param flag флаг переиспользования контекста компрессии
 */
void awh::Hash::takeoverCompress(const bool flag) noexcept {
	// Если флаг установлен
	if(this->_takeOverCompress && !flag)
		// Очищаем выделенную память для компрессора
		::deflateEnd(&this->_zdef);
	// Если флаг установлен
	if(!this->_takeOverCompress && flag){
		// Заполняем его нулями потока для компрессора
		::memset(&this->_zdef, 0, sizeof(this->_zdef));
		// Обнуляем структуру потока для компрессора
		this->_zdef.zalloc = Z_NULL;
		this->_zdef.zfree  = Z_NULL;
		this->_zdef.opaque = Z_NULL;
		// Если поток инициализировать не удалось, выходим
		if(::deflateInit2(&this->_zdef, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -1 * this->_wbit, DEFAULT_MEM_LEVEL, Z_HUFFMAN_ONLY) != Z_OK){
			// Выводим сообщение об ошибке
			this->_log->print("Deflate stream is not create", log_t::flag_t::CRITICAL);
			/**
			 * Если операционной системой является Nix-подобная
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				// Выходим из приложения
				::raise(SIGINT);
			/**
			 * Если операционной системой является MS Windows
			 */
			#else
				// Выходим из приложения
				::exit(EXIT_FAILURE);
			#endif
		}
	}
	// Устанавливаем переданный флаг
	this->_takeOverCompress = flag;
}
/**
 * takeoverDecompress Метод установки флага переиспользования контекста декомпрессии
 * @param flag флаг переиспользования контекста декомпрессии
 */
void awh::Hash::takeoverDecompress(const bool flag) noexcept {
	// Если флаг установлен
	if(this->_takeOverDecompress && !flag)
		// Очищаем выделенную память для декомпрессора
		::inflateEnd(&this->_zinf);
	// Если флаг установлен
	if(!this->_takeOverDecompress && flag){
		// Заполняем его нулями потока для декомпрессора
		::memset(&this->_zinf, 0, sizeof(this->_zinf));
		// Обнуляем структуру потока для декомпрессора
		this->_zinf.avail_in = 0;
		this->_zinf.zalloc   = Z_NULL;
		this->_zinf.zfree    = Z_NULL;
		this->_zinf.opaque   = Z_NULL;
		this->_zinf.next_in  = Z_NULL;
		// Если поток инициализировать не удалось, выходим
		if(::inflateInit2(&this->_zinf, -1 * this->_wbit) != Z_OK){
			// Выводим сообщение об ошибке
			this->_log->print("Inflate stream is not create", log_t::flag_t::CRITICAL);
			/**
			 * Если операционной системой является Nix-подобная
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				// Выходим из приложения
				::raise(SIGINT);
			/**
			 * Если операционной системой является MS Windows
			 */
			#else
				// Выходим из приложения
				::exit(EXIT_FAILURE);
			#endif
		}
	}
	// Устанавливаем переданный флаг
	this->_takeOverDecompress = flag;
}
/**
 * ~Hash Деструктор
 */
awh::Hash::~Hash() noexcept {
	// Очищаем выделенную память для компрессора
	if(this->_takeOverCompress)
		// Завершаем работу
		::deflateEnd(&this->_zdef);
	// Очищаем выделенную память для декомпрессора
	if(this->_takeOverDecompress)
		// Завершаем работу
		::inflateEnd(&this->_zinf);
}
