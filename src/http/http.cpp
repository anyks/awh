/**
 * @file: http.cpp
 * @date: 2025-10-06
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

/**
 * Подключаем модуль работы с временем
 */
#include <ctime>
#include <cmath>
#include <iomanip>

/**
 * Подключаем заголовочный файл
 */
#include <http/http.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * Заменяем функцию gmtime_r на gmtime_s
	 */
	#define gmtime_r(T, Tm) (gmtime_s(Tm, T) ? nullptr : Tm)
#endif

/**
 * @brief Метод инициализации модуля
 *
 */
void awh::Http::init() noexcept {
	// Выполняем заполнение сообщений ответов
	this->_responses = {
		{0, "Not Answer"},
		{100, "Continue"},
		{101, "Switching Protocols"},
		{102, "Processing"},
		{103, "Early Hints"},
		{200, "OK"},
		{201, "Created"},
		{202, "Accepted"},
		{203, "Non-Authoritative Information"},
		{204, "No Content"},
		{205, "Reset Content"},
		{206, "Partial Content"},
		{300, "Multiple Choice"},
		{301, "Moved Permanently"},
		{302, "Found"},
		{303, "See Other"},
		{304, "Not Modified"},
		{305, "Use Proxy"},
		{306, "Switch Proxy"},
		{307, "Temporary Redirect"},
		{308, "Permanent Redirect"},
		{400, "Bad Request"},
		{401, "Authentication Required"},
		{402, "Payment Required"},
		{403, "Forbidden"},
		{404, "Not Found"},
		{405, "Method Not Allowed"},
		{406, "Not Acceptable"},
		{407, "Proxy Authentication Required"},
		{408, "Request Timeout"},
		{409, "Conflict"},
		{410, "Gone"},
		{411, "Length Required"},
		{412, "Precondition Failed"},
		{413, "Request Entity Too Large"},
		{414, "Request-URI Too Long"},
		{415, "Unsupported Media Type"},
		{416, "Requested Range Not Satisfiable"},
		{417, "Expectation Failed"},
		{500, "Internal Server Error"},
		{501, "Not Implemented"},
		{502, "Bad Gateway"},
		{503, "Service Unavailable"},
		{504, "Gateway Timeout"},
		{505, "HTTP Version Not Supported"}
	};
	// Устанавливаем функцию обратного вызова для получения чанков
	this->_web.on <void (const uint32_t, const buffer_t &, const web_t *)> ("binary", &awh::Http::chunking, this, _1, _2, _3);
}
/**
 * @brief Функция выбора типа компрессора
 *
 * @param compressor название компрессора в текстовом виде
 * @return           результат работы функции
 */
bool awh::Http::matchingCompressor(const string & compressor) noexcept {
	// Отключаем сжатие тела сообщения
	this->_compressors.current = compressor_t::NONE;
	// Если данные пришли сжатые методом LZ4
	if(this->_fmk->compare("lz4", compressor))
		// Устанавливаем тип компрессии полезной нагрузки
		return static_cast <bool> (this->_compressors.current = compressor_t::LZ4);
	// Если данные пришли сжатые методом Zstandard
	else if(this->_fmk->compare("zstd", compressor))
		// Устанавливаем тип компрессии полезной нагрузки
		return static_cast <bool> (this->_compressors.current = compressor_t::ZSTD);
	// Если данные пришли сжатые методом LZma
	else if(this->_fmk->compare("xz", compressor))
		// Устанавливаем тип компрессии полезной нагрузки
		return static_cast <bool> (this->_compressors.current = compressor_t::LZMA);
	// Если данные пришли сжатые методом Brotli
	else if(this->_fmk->compare("br", compressor))
		// Устанавливаем тип компрессии полезной нагрузки
		return static_cast <bool> (this->_compressors.current = compressor_t::BROTLI);
	// Если данные пришли сжатые методом BZip2
	else if(this->_fmk->compare("bzip2", compressor))
		// Устанавливаем тип компрессии полезной нагрузки
		return static_cast <bool> (this->_compressors.current = compressor_t::BZIP2);
	// Если данные пришли сжатые методом GZip
	else if(this->_fmk->compare("gzip", compressor))
		// Устанавливаем тип компрессии полезной нагрузки
		return static_cast <bool> (this->_compressors.current = compressor_t::GZIP);
	// Если данные пришли сжатые методом Deflate
	else if(this->_fmk->compare("deflate", compressor))
		// Устанавливаем тип компрессии полезной нагрузки
		return static_cast <bool> (this->_compressors.current = compressor_t::DEFLATE);
	// Выводим результат
	return false;
}
/**
 * @brief Метод вывода полученных чанков полезной нагрузки
 *
 * @param id     идентификатор объекта
 * @param buffer буфер данных чанка полезной нагрузки
 * @param web    объект HTTP-парсера
 */
void awh::Http::chunking(const uint32_t id, const buffer_t & buffer, const web_t * web) noexcept {
	// Если функция обратного вызова на вывод полученного чанка установлена
	if(this->_callback.is("chunking"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const uint32_t, const buffer_t &, const http_t *)> ("chunking", id, buffer, this);
}
/**
 * @brief Метод выполнения шифрования полезной нагрузки
 *
 */
void awh::Http::encrypt() noexcept {
	// Если полезная нагрузка не зашифрована
	if(!this->_encrypt.crypted && this->_encrypt.enabled){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Получаем данные тела
			buffer_t & body = this->_web.body();
			// Если тело сообщения получено
			if(!body.empty()){
				// Выполняем шифрование полезной нагрузки
				auto result = this->_hash.encode <vector <char>> (
					static_cast <const char *> (body),
					static_cast <size_t> (body),
					this->_encrypt.cipher
				);
				// Если шифрование выполнено
				if((this->_encrypt.crypted = !result.empty()))
					// Формируем новое тело сообщения
					body = ::move(result);
				// Если шифрование не выполнено
				else {
					// Выводим сообщение об ошибке
					this->_log->print("Encryption module has failed", log_t::flag_t::WARNING);
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callback.is("error"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, "Encryption module has failed");
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод выполнения дешифровани полезной нагрузки
 *
 */
void awh::Http::decrypt() noexcept {
	// Если полезная нагрузка зашифрованна
	if(this->_encrypt.crypted){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Получаем данные тела
			buffer_t & body = this->_web.body();
			// Если тело сообщения получено
			if(!body.empty()){
				// Выполняем дешифрование полезной нагрузки
				auto result = this->_hash.decode <vector <char>> (
					static_cast <const char *> (body),
					static_cast <size_t> (body),
					this->_encrypt.cipher
				);
				// Если дешифрование выполнено
				if(!(this->_encrypt.crypted = result.empty()))
					// Формируем новое тело сообщения
					body = ::move(result);
				// Если дешифрование не выполнено
				else {
					// Выводим сообщение об ошибке
					this->_log->print("Decryption module has failed", log_t::flag_t::WARNING);
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callback.is("error"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, "Decryption module has failed");
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод выполнения декомпрессии полезной нагрузки
 *
 */
void awh::Http::compress() noexcept {
	// Если полезную нагрузку необходимо сжать
	if(this->_compressors.current == compressor_t::NONE){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Получаем данные тела
			buffer_t & body = this->_web.body();
			// Если тело сообщения получено
			if(!body.empty()){
				/**
				 * Определяем метод компрессии полезной нагрузки
				 */
				switch(static_cast <uint8_t> (this->_compressors.selected)){
					// Если полезную нагрузку необходимо сжать методом LZ4
					case static_cast <uint8_t> (compressor_t::LZ4): {
						// Выполняем компрессию полезной нагрузки
						auto result = this->_hash.compress <vector <char>> (
							static_cast <const char *> (body),
							static_cast <size_t> (body),
							hash_t::method_t::LZ4
						);
						// Если компрессия выполнена
						if(!result.empty()){
							// Формируем новое тело сообщения
							body = ::move(result);
							// Устанавливаем флаг компрессии
							this->_compressors.current = compressor_t::LZ4;
						// Если компрессия не выполнена
						} else {
							// Выводим сообщение об ошибке
							this->_log->print("LZ4 compression module has failed", log_t::flag_t::WARNING);
							// Если функция обратного вызова на на вывод ошибок установлена
							if(this->_callback.is("error"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, "LZ4 compression module has failed");
						}
					} break;
					// Если полезную нагрузку необходимо сжать методом Zstandard
					case static_cast <uint8_t> (compressor_t::ZSTD): {
						// Выполняем компрессию полезной нагрузки
						auto result = this->_hash.compress <vector <char>> (
							static_cast <const char *> (body),
							static_cast <size_t> (body),
							hash_t::method_t::ZSTD
						);
						// Если компрессия выполнена
						if(!result.empty()){
							// Формируем новое тело сообщения
							body = ::move(result);
							// Устанавливаем флаг компрессии
							this->_compressors.current = compressor_t::ZSTD;
						// Если компрессия не выполнена
						} else {
							// Выводим сообщение об ошибке
							this->_log->print("Zstandard compression module has failed", log_t::flag_t::WARNING);
							// Если функция обратного вызова на на вывод ошибок установлена
							if(this->_callback.is("error"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, "Zstandard compression module has failed");
						}
					} break;
					// Если полезную нагрузку необходимо сжать методом LZma
					case static_cast <uint8_t> (compressor_t::LZMA): {
						// Выполняем компрессию полезной нагрузки
						auto result = this->_hash.compress <vector <char>> (
							static_cast <const char *> (body),
							static_cast <size_t> (body),
							hash_t::method_t::LZMA
						);
						// Если компрессия выполнена
						if(!result.empty()){
							// Формируем новое тело сообщения
							body = ::move(result);
							// Устанавливаем флаг компрессии
							this->_compressors.current = compressor_t::LZMA;
						// Если компрессия не выполнена
						} else {
							// Выводим сообщение об ошибке
							this->_log->print("LZma compression module has failed", log_t::flag_t::WARNING);
							// Если функция обратного вызова на на вывод ошибок установлена
							if(this->_callback.is("error"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, "LZma compression module has failed");
						}
					} break;
					// Если полезную нагрузку необходимо сжать методом Brotli
					case static_cast <uint8_t> (compressor_t::BROTLI): {
						// Выполняем компрессию полезной нагрузки
						auto result = this->_hash.compress <vector <char>> (
							static_cast <const char *> (body),
							static_cast <size_t> (body),
							hash_t::method_t::BROTLI
						);
						// Если компрессия выполнена
						if(!result.empty()){
							// Формируем новое тело сообщения
							body = ::move(result);
							// Устанавливаем флаг компрессии
							this->_compressors.current = compressor_t::BROTLI;
						// Если компрессия не выполнена
						} else {
							// Выводим сообщение об ошибке
							this->_log->print("Brotli compression module has failed", log_t::flag_t::WARNING);
							// Если функция обратного вызова на на вывод ошибок установлена
							if(this->_callback.is("error"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, "Brotli compression module has failed");
						}
					} break;
					// Если полезную нагрузку необходимо сжать методом BZip2
					case static_cast <uint8_t> (compressor_t::BZIP2): {
						// Выполняем компрессию полезной нагрузки
						auto result = this->_hash.compress <vector <char>> (
							static_cast <const char *> (body),
							static_cast <size_t> (body),
							hash_t::method_t::BZIP2
						);
						// Если компрессия выполнена
						if(!result.empty()){
							// Формируем новое тело сообщения
							body = ::move(result);
							// Устанавливаем флаг компрессии
							this->_compressors.current = compressor_t::BZIP2;
						// Если компрессия не выполнена
						} else {
							// Выводим сообщение об ошибке
							this->_log->print("BZip2 compression module has failed", log_t::flag_t::WARNING);
							// Если функция обратного вызова на на вывод ошибок установлена
							if(this->_callback.is("error"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, "BZip2 compression module has failed");
						}
					} break;
					// Если полезную нагрузку необходимо сжать методом GZip
					case static_cast <uint8_t> (compressor_t::GZIP): {
						// Выполняем компрессию полезной нагрузки
						auto result = this->_hash.compress <vector <char>> (
							static_cast <const char *> (body),
							static_cast <size_t> (body),
							hash_t::method_t::GZIP
						);
						// Если компрессия выполнена
						if(!result.empty()){
							// Формируем новое тело сообщения
							body = ::move(result);
							// Устанавливаем флаг компрессии
							this->_compressors.current = compressor_t::GZIP;
						// Если компрессия не выполнена
						} else {
							// Выводим сообщение об ошибке
							this->_log->print("GZip compression module has failed", log_t::flag_t::WARNING);
							// Если функция обратного вызова на на вывод ошибок установлена
							if(this->_callback.is("error"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, "GZip compression module has failed");
						}
					} break;
					// Если полезную нагрузку необходимо сжать методом Deflate
					case static_cast <uint8_t> (compressor_t::DEFLATE): {
						// Выполняем компрессию полезной нагрузки
						auto result = this->_hash.compress <vector <char>> (
							static_cast <const char *> (body),
							static_cast <size_t> (body),
							hash_t::method_t::DEFLATE
						);
						// Если компрессия выполнена
						if(!result.empty()){
							// Удаляем хвост в полученных данных
							this->_hash.rmTail(result);
							// Формируем новое тело сообщения
							body = ::move(result);
							// Устанавливаем флаг компрессии
							this->_compressors.current = compressor_t::DEFLATE;
						// Если компрессия не выполнена
						} else {
							// Выводим сообщение об ошибке
							this->_log->print("Deflate compression module has failed", log_t::flag_t::WARNING);
							// Если функция обратного вызова на на вывод ошибок установлена
							if(this->_callback.is("error"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, "Deflate compression module has failed");
						}
					} break;
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод выполнения компрессии полезной нагрузки
 *
 */
void awh::Http::decompress() noexcept {
	// Если полезную нагрузку необходимо извлечь
	if(this->_compressors.current != compressor_t::NONE){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Получаем данные тела
			buffer_t & body = this->_web.body();
			// Если тело сообщения получено
			if(!body.empty()){
				/**
				 * Определяем метод компрессии полезной нагрузки
				 */
				switch(static_cast <uint8_t> (this->_compressors.current)){
					// Если полезную нагрузку нужно извлечь методом LZ4
					case static_cast <uint8_t> (compressor_t::LZ4): {
						// Выполняем декомпрессию полезной нагрузки
						auto result = this->_hash.decompress <vector <char>> (
							static_cast <const char *> (body),
							static_cast <size_t> (body),
							hash_t::method_t::LZ4
						);
						// Если декомпрессия выполнена
						if(!result.empty()){
							// Формируем новое тело сообщения
							body = ::move(result);
							// Снимаем флаг компрессии
							this->_compressors.current = compressor_t::NONE;
						}
					} break;
					// Если полезную нагрузку нужно извлечь методом Zstandard
					case static_cast <uint8_t> (compressor_t::ZSTD): {
						// Выполняем декомпрессию полезной нагрузки
						auto result = this->_hash.decompress <vector <char>> (
							static_cast <const char *> (body),
							static_cast <size_t> (body),
							hash_t::method_t::ZSTD
						);
						// Если декомпрессия выполнена
						if(!result.empty()){
							// Формируем новое тело сообщения
							body = ::move(result);
							// Снимаем флаг компрессии
							this->_compressors.current = compressor_t::NONE;
						}
					} break;
					// Если полезную нагрузку нужно извлечь методом LZma
					case static_cast <uint8_t> (compressor_t::LZMA): {
						// Выполняем декомпрессию полезной нагрузки
						auto result = this->_hash.decompress <vector <char>> (
							static_cast <const char *> (body),
							static_cast <size_t> (body),
							hash_t::method_t::LZMA
						);
						// Если декомпрессия выполнена
						if(!result.empty()){
							// Формируем новое тело сообщения
							body = ::move(result);
							// Снимаем флаг компрессии
							this->_compressors.current = compressor_t::NONE;
						}
					} break;
					// Если полезную нагрузку нужно извлечь методом Brotli
					case static_cast <uint8_t> (compressor_t::BROTLI): {
						// Выполняем декомпрессию данных
						auto result = this->_hash.decompress <vector <char>> (
							static_cast <const char *> (body),
							static_cast <size_t> (body),
							hash_t::method_t::BROTLI
						);
						// Если декомпрессия выполнена
						if(!result.empty()){
							// Формируем новое тело сообщения
							body = ::move(result);
							// Снимаем флаг компрессии
							this->_compressors.current = compressor_t::NONE;
						}
					} break;
					// Если полезную нагрузку нужно извлечь методом BZip2
					case static_cast <uint8_t> (compressor_t::BZIP2): {
						// Выполняем декомпрессию полезной нагрузки
						auto result = this->_hash.decompress <vector <char>> (
							static_cast <const char *> (body),
							static_cast <size_t> (body),
							hash_t::method_t::BZIP2
						);
						// Если декомпрессия выполнена
						if(!result.empty()){
							// Формируем новое тело сообщения
							body = ::move(result);
							// Снимаем флаг компрессии
							this->_compressors.current = compressor_t::NONE;
						}
					} break;
					// Если полезную нагрузку нужно извлечь методом GZip
					case static_cast <uint8_t> (compressor_t::GZIP): {
						// Выполняем декомпрессию полезной нагрузки
						auto result = this->_hash.decompress <vector <char>> (
							static_cast <const char *> (body),
							static_cast <size_t> (body),
							hash_t::method_t::GZIP
						);
						// Если декомпрессия выполнена
						if(!result.empty()){
							// Формируем новое тело сообщения
							body = ::move(result);
							// Снимаем флаг компрессии
							this->_compressors.current = compressor_t::NONE;
						}
					} break;
					// Если полезную нагрузку нужно извлечь методом Deflate
					case static_cast <uint8_t> (compressor_t::DEFLATE): {
						// Получаем данные тела в бинарном виде
						vector <char> buffer = ::move(const_cast <vector <char> &> (static_cast <const vector <char> &> (body)));
						// Добавляем хвост в полученные данные
						this->_hash.setTail(buffer);
						// Выполняем декомпрессию полезной нагрузки
						const auto & result = this->_hash.decompress <vector <char>> (buffer.data(), buffer.size(), hash_t::method_t::DEFLATE);
						// Если декомпрессия выполнена
						if(!result.empty()){
							// Формируем новое тело сообщения
							body = ::move(result);
							// Снимаем флаг компрессии
							this->_compressors.current = compressor_t::NONE;
						}
					} break;
				}
				/**
				 * Определяем метод компрессии полезной нагрузки
				 */
				switch(static_cast <uint8_t> (this->_compressors.current)){
					// Если метод компрессии не изменился и остался LZ4
					case static_cast <uint8_t> (compressor_t::LZ4): {
						// Сообщаем, что переданное тело содержит ошибки
						this->_log->print("LZ4 decompression module has failed", log_t::flag_t::WARNING);
						// Если функция обратного вызова на на вывод ошибок установлена
						if(this->_callback.is("error"))
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, "LZ4 decompression module has failed");
					} break;
					// Если метод компрессии не изменился и остался Zstandard
					case static_cast <uint8_t> (compressor_t::ZSTD): {
						// Сообщаем, что переданное тело содержит ошибки
						this->_log->print("Zstandard decompression module has failed", log_t::flag_t::WARNING);
						// Если функция обратного вызова на на вывод ошибок установлена
						if(this->_callback.is("error"))
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, "Zstandard decompression module has failed");
					} break;
					// Если метод компрессии не изменился и остался LZma
					case static_cast <uint8_t> (compressor_t::LZMA): {
						// Сообщаем, что переданное тело содержит ошибки
						this->_log->print("LZma decompression module has failed", log_t::flag_t::WARNING);
						// Если функция обратного вызова на на вывод ошибок установлена
						if(this->_callback.is("error"))
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, "LZma decompression module has failed");
					} break;
					// Если метод компрессии не изменился и остался Brotli
					case static_cast <uint8_t> (compressor_t::BROTLI): {
						// Сообщаем, что переданное тело содержит ошибки
						this->_log->print("Brotli decompression module has failed", log_t::flag_t::WARNING);
						// Если функция обратного вызова на на вывод ошибок установлена
						if(this->_callback.is("error"))
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, "Brotli decompression module has failed");
					} break;
					// Если метод компрессии не изменился и остался BZip2
					case static_cast <uint8_t> (compressor_t::BZIP2): {
						// Сообщаем, что переданное тело содержит ошибки
						this->_log->print("BZip2 decompression module has failed", log_t::flag_t::WARNING);
						// Если функция обратного вызова на на вывод ошибок установлена
						if(this->_callback.is("error"))
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, "BZip2 decompression module has failed");
					} break;
					// Если метод компрессии не изменился и остался GZip
					case static_cast <uint8_t> (compressor_t::GZIP): {
						// Сообщаем, что переданное тело содержит ошибки
						this->_log->print("GZip decompression module has failed", log_t::flag_t::WARNING);
						// Если функция обратного вызова на на вывод ошибок установлена
						if(this->_callback.is("error"))
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, "GZip decompression module has failed");
					} break;
					// Если метод компрессии не изменился и остался Deflate
					case static_cast <uint8_t> (compressor_t::DEFLATE): {
						// Сообщаем, что переданное тело содержит ошибки
						this->_log->print("DEFLATE decompression module has failed", log_t::flag_t::WARNING);
						// Если функция обратного вызова на на вывод ошибок установлена
						if(this->_callback.is("error"))
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, "Deflate decompression module has failed");
					} break;
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод применения полученных результатов
 *
 */
void awh::Http::commit() noexcept {
	// Если рукопожатие ещё не выполнено
	if(this->_session.handshake == handshake_t::NONE){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем получение статуса рукопожатия
			this->_session.handshake = this->handshake();
			// Если рукопожатие выполнено
			if(this->_session.handshake == handshake_t::GOOD)
				// Устанавливаем стейт рукопожатия
				this->_session.state = state_t::GOOD;
			// Устанавливаем режим бракованных данных
			else this->_session.state = state_t::BROKEN;
			// Получаем заголовок шифрования
			const string & encrypt = this->_web.header("X-AWH-Encryption");
			// Если заголовок найден
			if((this->_encrypt.crypted = !encrypt.empty())){
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					/**
					 * Определяем размер шифрования
					 */
					switch(static_cast <uint16_t> (::stoi(encrypt))){
						// Если шифрование произведено 128 битным ключём
						case 128: this->_encrypt.cipher = hash_t::cipher_t::AES128; break;
						// Если шифрование произведено 192 битным ключём
						case 192: this->_encrypt.cipher = hash_t::cipher_t::AES192; break;
						// Если шифрование произведено 256 битным ключём
						case 256: this->_encrypt.cipher = hash_t::cipher_t::AES256; break;
					}
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception &) {
					// Если шифрование произведено 128 битным ключём
					this->_encrypt.cipher = hash_t::cipher_t::AES128;
				}
			}
			// Отключаем сжатие тела сообщения
			this->_compressors.current = compressor_t::NONE;
			// Выполняем извлечение заголовков HTTP-протокола
			headers_t & headers = this->_web.headers();
			// Если заголовок с параметрами контента получен
			if(!headers.empty() && headers.has("Content-Encoding")){
				// Список компрессоров которым выполненно сжатие
				vector <string> compressors, temp;
				// Выполняем перебор всего списка указанных заголовков
				for(auto & header : headers.range("Content-Encoding")){
					// Выполняем извлечение списка компрессоров
					this->_fmk->split(header, ",", temp);
					// Если список компрессоров получен
					if(!temp.empty())
						// Добавляем в общий список компрессоров
						compressors.insert(compressors.end(), temp.begin(), temp.end());
				}
				// Если список компрессоров получен
				if(!compressors.empty()){
					// Если компрессоров в списке больше 1-го
					if(compressors.size() > 1){
						// Выполняем перебор всех компрессоров в обратном порядке
						for(auto i = compressors.rbegin(); i != (compressors.rend() - 1); ++i){
							// Выполняем определение типа компрессора
							if(this->matchingCompressor(* i))
								// Выполняем декомпрессию
								this->decompress();
						}
					}
					// Выполняем определение типа компрессора
					this->matchingCompressor(compressors.front());
				}
			}
			/**
			 * Определяем к какому сервису относится модуль
			 */
			switch(static_cast <uint8_t> (this->_web.hid())){
				// Если модуль соответствует клиенту
				case static_cast <uint8_t> (web_t::hid_t::CLIENT): {
					// Если заголовок с параметрами передачи контента получен и контент не зашифрован
					if(headers.has("Transfer-Encoding")){
						// Список компрессоров которым выполненно сжатие
						vector <string> compressors, temp;
						// Выполняем перебор всего списка указанных заголовков
						for(auto & header : headers.range("Transfer-Encoding")){
							// Выполняем извлечение списка компрессоров
							this->_fmk->split(header, ",", temp);
							// Если список компрессоров получен
							if(!temp.empty())
								// Добавляем в общий список компрессоров
								compressors.insert(compressors.end(), temp.begin(), temp.end());
						}
						// Если список компрессоров получен
						if(!compressors.empty()){
							// Если компрессоров в списке больше 1-го
							if(compressors.size() > 1){
								// Выполняем перебор всех компрессоров в обратном порядке
								for(auto i = compressors.rbegin(); i != (compressors.rend() - 1); ++i){
									// Выполняем определение типа компрессора
									if(this->matchingCompressor(* i))
										// Выполняем декомпрессию
										this->decompress();
									// Если мы получили параметр передачи данных чанками
									else if(this->_fmk->compare("chunked", * i))
										// Выполняем активацию передачу данных чанками
										this->_transfer.chunking = true;
								}
							}
							// Выполняем определение типа компрессора
							if(!this->matchingCompressor(compressors.front())){
								// Если мы получили параметр передачи данных чанками
								if(this->_fmk->compare("chunked", compressors.front()))
									// Выполняем активацию передачу данных чанками
									this->_transfer.chunking = true;
							}
						}
						// Выполняем удаление заголовка
						headers.erase("Transfer-Encoding");
						// Если активирован режим чанкинга
						if(this->_transfer.chunking)
							// Выполняем корректировку значения заголовка
							headers.emplace("Transfer-Encoding", "chunked");
					}
					// Если тело полезной нагрузки получено в сжатом виде
					if(this->_compressors.current != compressor_t::NONE)
						// Устанавливаем флаг метода компрессии
						this->_compressors.selected = this->_compressors.current;
				} break;
				// Если модуль соответствует серверу
				case static_cast <uint8_t> (web_t::hid_t::SERVER): {
					// Отключаем сжатие тела сообщения
					this->_compressors.selected = compressor_t::NONE;
					// Если список поддерживаемых протоколов установлен
					if(!this->_compressors.supports.empty()){
						// Если заголовок с запрашиваемым контентом существует
						if(headers.has("Accept-Encoding")){
							// Список компрессоров которым выполненно сжатие
							vector <string> compressors;
							// Список запрашиваемых компрессоров клиентом
							std::multimap <float, compressor_t> requested;
							// Выполняем перебор всего списка указанных заголовков
							for(auto & header : headers.range("Accept-Encoding")){
								// Если конкретный метод сжатия запрашивается любой
								if(this->_fmk->compare("*", header)){
									// Выполняем перебор всего списка доступных компрессоров
									for(auto & compressor : this->_compressors.supports)
										// Добавляем в список запрашиваемых компрессоров
										requested.emplace(compressor.first, compressor.second);
								// Если указан конкретный метод компрессии
								} else {
									// Выполняем извлечение списка компрессоров
									this->_fmk->split(header, ",", compressors);
									// Если список компрессоров получен
									if(!compressors.empty()){
										// Вес запрашиваемого компрессора
										float weight = 1.f;
										// Выполняем перебор списка запрашиваемых компрессоров
										for(auto & compressor : compressors){
											// Если найден вес компрессора
											if(this->_fmk->exists(";q=", compressor)){
												// Выполняем поиск разделителя
												const size_t pos = compressor.rfind(';');
												// Если разделитель найден
												if((pos != string::npos) && (compressor.size() >= (pos + 4))){
													// Получаем вес компрессора
													const string & second = compressor.substr(pos + 3);
													// Если вес указан верный
													if(!second.empty() && (this->_fmk->is(second, fmk_t::check_t::DECIMAL) || this->_fmk->is(second, fmk_t::check_t::NUMBER))){
														// Изавлекаем название компрессора
														const string & first = compressor.substr(0, pos);
														// Если данные пришли сжатые методом LZ4
														if(this->_fmk->compare("lz4", first))
															// Добавляем в список полученный компрессор
															requested.emplace(::stof(second), compressor_t::LZ4);
														// Если данные пришли сжатые методом Zstandard
														else if(this->_fmk->compare("zstd", first))
															// Добавляем в список полученный компрессор
															requested.emplace(::stof(second), compressor_t::ZSTD);
														// Если данные пришли сжатые методом LZma
														else if(this->_fmk->compare("xz", first))
															// Добавляем в список полученный компрессор
															requested.emplace(::stof(second), compressor_t::LZMA);
														// Если данные пришли сжатые методом Brotli
														else if(this->_fmk->compare("br", first))
															// Добавляем в список полученный компрессор
															requested.emplace(::stof(second), compressor_t::BROTLI);
														// Если данные пришли сжатые методом BZip2
														else if(this->_fmk->compare("bzip2", first))
															// Добавляем в список полученный компрессор
															requested.emplace(::stof(second), compressor_t::BZIP2);
														// Если данные пришли сжатые методом GZip
														else if(this->_fmk->compare("gzip", first))
															// Добавляем в список полученный компрессор
															requested.emplace(::stof(second), compressor_t::GZIP);
														// Если данные пришли сжатые методом Deflate
														else if(this->_fmk->compare("deflate", first))
															// Добавляем в список полученный компрессор
															requested.emplace(::stof(second), compressor_t::DEFLATE);
													// Если вес компрессора указан не является числом
													} else {
														// Выводим сообщение об ошибке
														this->_log->print("Weight of the requested %s compressor is not a number [%s]", log_t::flag_t::WARNING, compressor.substr(0, pos).c_str(), second.c_str());
														// Если функция обратного вызова на на вывод ошибок установлена
														if(this->_callback.is("error"))
															// Выполняем функцию обратного вызова
															this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, this->_fmk->format("Weight of the requested %s compressor is not a number [%s]", compressor.substr(0, pos).c_str(), second.c_str()));
													}
												// Если мы получили данные в неверном формате
												} else {
													// Выводим сообщение об ошибке
													this->_log->print("We received data in the wrong format [%s]", log_t::flag_t::WARNING, compressor.c_str());
													// Если функция обратного вызова на на вывод ошибок установлена
													if(this->_callback.is("error"))
														// Выполняем функцию обратного вызова
														this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, this->_fmk->format("We received data in the wrong format [%s]", compressor.c_str()));
												}
											// Если вес компрессора не установлен
											} else {
												// Если данные пришли сжатые методом LZ4
												if(this->_fmk->compare("lz4", compressor))
													// Добавляем в список полученный компрессор
													requested.emplace(weight, compressor_t::LZ4);
												// Если данные пришли сжатые методом Zstandard
												else if(this->_fmk->compare("zstd", compressor))
													// Добавляем в список полученный компрессор
													requested.emplace(weight, compressor_t::ZSTD);
												// Если данные пришли сжатые методом LZma
												else if(this->_fmk->compare("xz", compressor))
													// Добавляем в список полученный компрессор
													requested.emplace(weight, compressor_t::LZMA);
												// Если данные пришли сжатые методом Brotli
												else if(this->_fmk->compare("br", compressor))
													// Добавляем в список полученный компрессор
													requested.emplace(weight, compressor_t::BROTLI);
												// Если данные пришли сжатые методом BZip2
												else if(this->_fmk->compare("bzip2", compressor))
													// Добавляем в список полученный компрессор
													requested.emplace(weight, compressor_t::BZIP2);
												// Если данные пришли сжатые методом GZip
												else if(this->_fmk->compare("gzip", compressor))
													// Добавляем в список полученный компрессор
													requested.emplace(weight, compressor_t::GZIP);
												// Если данные пришли сжатые методом Deflate
												else if(this->_fmk->compare("deflate", compressor))
													// Добавляем в список полученный компрессор
													requested.emplace(weight, compressor_t::DEFLATE);
												// Выполняем уменьшение веса выбранного компрессора
												weight -= .1f;
											}
										}
									}
								}
							}
							// Если список запрашиваемых компрессоров получен
							if(!requested.empty()){
								// Выполняем перебор списка запрашиваемых компрессоров
								for(auto i = requested.rbegin(); i != requested.rend(); ++i){
									// Выполняем поиск в списке доступных компрессоров запрашиваемый компрессор
									if(this->_fmk->findInMap(i->second, this->_compressors.supports) != this->_compressors.supports.end()){
										// Устанавливаем флаг метода компрессии
										this->_compressors.selected = i->second;
										// Выходим из цикла
										break;
									}
								}
							}
						}
					}
					// Если заголовок с параметрами передачи данных Transfer-Encoding существует
					if((this->_transfer.enabled = headers.has("te"))){
						// Если версия протокола подключения выше чем HTTP/1.1
						if(this->_web.request().version > 1.1){
							// Выполняем активации получение трейлеров
							this->_transfer.trailers = true;
							// Выполняем активацию передачу данных чанками
							this->_transfer.chunking = true;
						// Если версия протокола подключения не выше HTTP/1.1
						} else {
							// Список компрессоров запроса для Transfer-Encoding
							vector <string> compressors;
							// Список запрашиваемых компрессоров клиентом
							std::multimap <float, compressor_t> requested;
							// Выполняем перебор всего списка указанных заголовков
							for(auto & header : headers.range("te")){
								// Выполняем извлечение списка компрессоров
								this->_fmk->split(header, ",", compressors);
								// Если список компрессоров получен
								if(!compressors.empty()){
									// Вес запрашиваемого компрессора
									float weight = 1.f;
									// Выполняем перебор списка компрессоров
									for(auto & compressor : compressors){
										// Если найден вес компрессора
										if(this->_fmk->exists(";q=", compressor)){
											// Выполняем поиск разделителя
											const size_t pos = compressor.rfind(';');
											// Если разделитель найден
											if((pos != string::npos) && (compressor.size() >= (pos + 4))){
												// Получаем вес компрессора
												const string & second = compressor.substr(pos + 3);
												// Если вес указан верный
												if(!second.empty() && (this->_fmk->is(second, fmk_t::check_t::DECIMAL) || this->_fmk->is(second, fmk_t::check_t::NUMBER))){
													// Изавлекаем название компрессора
													const string & first = compressor.substr(0, pos);
													// Если данные пришли сжатые методом LZ4
													if(this->_fmk->compare("lz4", first))
														// Добавляем в список полученный компрессор
														requested.emplace(::stof(second), compressor_t::LZ4);
													// Если данные пришли сжатые методом Zstandard
													else if(this->_fmk->compare("zstd", first))
														// Добавляем в список полученный компрессор
														requested.emplace(::stof(second), compressor_t::ZSTD);
													// Если данные пришли сжатые методом LZma
													else if(this->_fmk->compare("xz", first))
														// Добавляем в список полученный компрессор
														requested.emplace(::stof(second), compressor_t::LZMA);
													// Если данные пришли сжатые методом Brotli
													else if(this->_fmk->compare("br", first))
														// Добавляем в список полученный компрессор
														requested.emplace(::stof(second), compressor_t::BROTLI);
													// Если данные пришли сжатые методом BZip2
													else if(this->_fmk->compare("bzip2", first))
														// Добавляем в список полученный компрессор
														requested.emplace(::stof(second), compressor_t::BZIP2);
													// Если данные пришли сжатые методом GZip
													else if(this->_fmk->compare("gzip", first))
														// Добавляем в список полученный компрессор
														requested.emplace(::stof(second), compressor_t::GZIP);
													// Если данные пришли сжатые методом Deflate
													else if(this->_fmk->compare("deflate", first))
														// Добавляем в список полученный компрессор
														requested.emplace(::stof(second), compressor_t::DEFLATE);
													// Если получен параметр разрешающий использовать трейлеры
													else if(this->_fmk->compare("trailers", first))
														// Выполняем активации получение трейлеров
														this->_transfer.trailers = true;
													// Если получен параметр разрешающий обмениваться чанками
													else if(this->_fmk->compare("chunked", first))
														// Выполняем активацию передачу данных чанками
														this->_transfer.chunking = true;
												// Если вес компрессора указан не является числом
												} else {
													// Выводим сообщение об ошибке
													this->_log->print("Weight of the requested %s compressor is not a number [%s]", log_t::flag_t::WARNING, compressor.substr(0, pos).c_str(), second.c_str());
													// Если функция обратного вызова на на вывод ошибок установлена
													if(this->_callback.is("error"))
														// Выполняем функцию обратного вызова
														this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, this->_fmk->format("Weight of the requested %s compressor is not a number [%s]", compressor.substr(0, pos).c_str(), second.c_str()));
												}
											// Если мы получили данные в неверном формате
											} else {
												// Выводим сообщение об ошибке
												this->_log->print("We received data in the wrong format [%s]", log_t::flag_t::WARNING, compressor.c_str());
												// Если функция обратного вызова на на вывод ошибок установлена
												if(this->_callback.is("error"))
													// Выполняем функцию обратного вызова
													this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, this->_fmk->format("We received data in the wrong format [%s]", compressor.c_str()));
											}
										// Если вес компрессора не установлен
										} else {
											// Если данные пришли сжатые методом LZ4
											if(this->_fmk->compare("lz4", compressor))
												// Добавляем в список полученный компрессор
												requested.emplace(weight, compressor_t::LZ4);
											// Если данные пришли сжатые методом Zstandard
											else if(this->_fmk->compare("zstd", compressor))
												// Добавляем в список полученный компрессор
												requested.emplace(weight, compressor_t::ZSTD);
											// Если данные пришли сжатые методом LZma
											else if(this->_fmk->compare("xz", compressor))
												// Добавляем в список полученный компрессор
												requested.emplace(weight, compressor_t::LZMA);
											// Если данные пришли сжатые методом Brotli
											else if(this->_fmk->compare("br", compressor))
												// Добавляем в список полученный компрессор
												requested.emplace(weight, compressor_t::BROTLI);
											// Если данные пришли сжатые методом BZip2
											else if(this->_fmk->compare("bzip2", compressor))
												// Добавляем в список полученный компрессор
												requested.emplace(weight, compressor_t::BZIP2);
											// Если данные пришли сжатые методом GZip
											else if(this->_fmk->compare("gzip", compressor))
												// Добавляем в список полученный компрессор
												requested.emplace(weight, compressor_t::GZIP);
											// Если данные пришли сжатые методом Deflate
											else if(this->_fmk->compare("deflate", compressor))
												// Добавляем в список полученный компрессор
												requested.emplace(weight, compressor_t::DEFLATE);
											// Если получен параметр разрешающий использовать трейлеры
											else if(this->_fmk->compare("trailers", compressor))
												// Выполняем активации получение трейлеров
												this->_transfer.trailers = true;
											// Если получен параметр разрешающий обмениваться чанками
											else if(this->_fmk->compare("chunked", compressor))
												// Выполняем активацию передачу данных чанками
												this->_transfer.chunking = true;
											// Выполняем уменьшение веса выбранного компрессора
											weight -= .1f;
										}
									}
								}
							}
							// Если список запрашиваемых компрессоров получен
							if(!requested.empty() && !this->_compressors.supports.empty()){
								// Выполняем перебор списка запрашиваемых компрессоров
								for(auto i = requested.rbegin(); i != requested.rend(); ++i){
									// Выполняем поиск в списке доступных компрессоров запрашиваемый компрессор
									if(this->_fmk->findInMap(i->second, this->_compressors.supports) != this->_compressors.supports.end()){
										// Устанавливаем флаг метода компрессии
										this->_compressors.selected = i->second;
										// Выходим из цикла
										break;
									}
								}
							}
							// Выполняем удаление заголовка
							headers.erase("te");
							// Выполняем извлечение данных заголовка подключения
							string connection = headers["Connection"];
							// Если заголовок подключения получен
							if(!connection.empty()){
								// Переводим значение в нижний регистр
								this->_fmk->transform(connection, fmk_t::transform_t::LOWER);
								// Выполняем поиск заголовка Transfer-Encoding
								const size_t pos = connection.find("te");
								// Если заголовок найден
								if(pos != string::npos){
									// Выполняем удаление значение TE из заголовка
									connection.erase(pos, 2);
									// Если первый символ является запятой, удаляем
									if(connection.front() == ',')
										// Удаляем запятую
										connection.erase(0, 1);
									// Выполняем удаление лишних пробелов
									this->_fmk->transform(connection, fmk_t::transform_t::TRIM);
									// Выполняем удаление заголовка
									headers.erase("Connection");
									// Если значение подключение получено
									if(!connection.empty())
										// Устанавливаем отредактированное подключение
										headers.emplace("Connection", connection);
									// Иначе заменяем значение подключение по умолчанию
									else headers.emplace("Connection", "keep-alive");
								}
							}
						}
					}
				} break;
			}
			// Выполняем фиксацию полученных данных
			this->_web.commit();
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод проверки текущего статуса рукопожатия
 *
 * @return текущий статус рукопожатия
 */
awh::Http::handshake_t awh::Http::status() const noexcept {
	// Выводим результат рукопожатия
	return this->_session.handshake;
}
/**
 * @brief Метод получения идентификатора объекта
 *
 * @return идентификатор объекта
 */
uint32_t awh::Http::id() const noexcept {
	// Выводим идентификатор объекта
	return this->_web.id();
}
/**
 * @brief Метод установки идентификатора объекта
 *
 * @param id идентификатор объекта
 */
void awh::Http::id(const uint32_t id) noexcept {
	// Выполняем установку идентификатора объекта
	this->_web.id(id);
}
/**
 * @brief Метод извлечения идентичности протокола модуля
 *
 * @return флаг идентичности протокола модуля
 */
awh::Http::identity_t awh::Http::identity() const noexcept {
	// Выводим флаг идентичности протокола
	return this->_session.identity;
}
/**
 * @brief Метод установки идентичности протокола модуля
 *
 * @param identity идентичность протокола модуля
 */
void awh::Http::identity(const identity_t identity) noexcept {
	// Выполняем установку флага идентичности протокола модуля
	this->_session.identity = identity;
}
/**
 * @brief Метод сброса параметров запроса
 *
 */
void awh::Http::reset() noexcept {
	// Выполняем сброс данных парсера
	this->_web.reset();
	// Выполняем сброс стейта текущего запроса
	this->_session.state = state_t::NONE;
	// Выполняем сброс статуса рукопожатия
	this->_session.handshake = handshake_t::NONE;
}
/**
 * @brief Метод очистки собранных данных
 *
 */
void awh::Http::clear() noexcept {
	// Выполняем очистку данных парсера
	this->_web.clear();
	// Очищаем список установленных трейлеров
	this->_trailers.clear();
	// Выполняем сброс чёрного списка HTTP-заголовков
	this->_blacklist.clear();
	// Выполняем сброс флага формирования чанков
	this->_transfer.chunking = false;
	// Снимаем флаг зашифрованной полезной нагрузки
	this->_encrypt.crypted = false;
	// Снимаем флаг сжатой полезной нагрузки
	this->_compressors.current = compressor_t::NONE;
}
/**
 * @brief Метод очистки данных HTTP-юнита
 *
 * @param unit HTTP-юнит данные которого очищаются
 */
void awh::Http::clear(const web_t::unit_t unit) noexcept {
	/**
	 * Определяем текущий HTTP-юнит с которым производится работа
	 */
	switch(static_cast <uint8_t> (unit)){
		// Если производится работы с HTTP-телом
		case static_cast <uint8_t> (web_t::unit_t::BODY): {
			// Снимаем флаг зашифрованной полезной нагрузки
			this->_encrypt.crypted = false;
			// Снимаем флаг сжатой полезной нагрузки
			this->_compressors.current = compressor_t::NONE;
		} break;
		// Если производится работа с HTTP-заголовками
		case static_cast <uint8_t> (web_t::unit_t::HEADERS):
			// Выполняем сброс флага формирования чанков
			this->_transfer.chunking = false;
		break;
	}
	// Выполняем очистку данных тела
	this->_web.clear(unit);
}
/**
 * @brief Метод проверки существования данных
 * 
 * @param unit HTTP-юнит наличие данных которого проверяются
 * @return     результат проверки
 */
bool awh::Http::empty(const web_t::unit_t unit) noexcept {
	// Выполняем проверку существования данных
	return this->_web.empty(unit);
}
/**
 * @brief Метод установки флага точной установки хоста
 *
 * @param mode флаг для установки
 */
void awh::Http::exactHost(const bool mode) noexcept {
	// Выполняем установку флага точной установки хоста
	this->_session.exactHost = mode;
}
/**
 * @brief Метод установки размера чанка
 *
 * @param size размер чанка для установки
 */
void awh::Http::chunkSize(const size_t size) noexcept {
	// Устанавливаем размер чанка
	if(size >= 100)
		// Выполняем установку размера чанка
		this->_session.chunkSize = size;
}
/**
 * @brief Метод проверки существования данных в чёрном списке
 *
 */
bool awh::Http::emptyBlacklist() const noexcept {
	// Выполняем проверку наличия заголовков в чёрном списке
	return this->_blacklist.empty();
}
/**
 * @brief delInBlacklist Метод удаления заголовка HTTP-протокола из чёрного списка
 *
 * @param name название заголовка HTTP-протокола для удаления
 * @return     результат удаления
 */
bool awh::Http::delInBlacklist(const string name) noexcept {
	// Результат работы функции
	bool result = false;
	// Если название заголовка HTTP-протокола передано
	if(!name.empty()){
		// Выполняем поиск заголовка в чёрном списке
		auto i = this->_blacklist.find(this->_fmk->transform(name, fmk_t::transform_t::LOWER));
		// Если заголовок в чёрном списке найден
		if((result = (i != this->_blacklist.end())))
			// Выполняем удаление заголовка HTTP-протокола из чёрного списка
			this->_blacklist.erase(i);
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод добавления заголовка в чёрный список
 *
 * @param name название заголовка HTTP-протокола
 */
bool awh::Http::addToBlacklist(const string name) noexcept {
	// Если название заголовка HTTP-протокола передано
	if(!name.empty())
		// Выполняем добавление заголовка в чёрный список
		return this->_blacklist.emplace(this->_fmk->transform(name, fmk_t::transform_t::LOWER)).second;
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Проверка заголовка HTTP-протокола находится ли он в чёрном списке
 *
 * @param name название заголовка HTTP-протокола для проверки
 * @return     результат проверки
 */
bool awh::Http::isInBlacklist(const string name) const noexcept {
	// Если название заголовка HTTP-протокола передано
	if(!name.empty())
		// Выполняем проверку наличия заголовка в чёрном списке
		return (this->_blacklist.find(this->_fmk->transform(name, fmk_t::transform_t::LOWER)) != this->_blacklist.end());
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод проверки активного состояния
 *
 * @param state состояние которое необходимо проверить
 */
bool awh::Http::state(const state_t state) const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем запрашиваемое состояние
		 */
		switch(static_cast <uint8_t> (state)){
			// Если проверяется режим завершения сбора данных
			case static_cast <uint8_t> (state_t::END):
				// Выводрим результат проверки
				return (
					(this->_session.state == state_t::GOOD) ||
					(this->_session.state == state_t::BROKEN) ||
					(this->_session.state == state_t::HANDSHAKE)
				);
			// Если проверяется режим удачного выполнения запроса
			case static_cast <uint8_t> (state_t::GOOD):
				// Выводрим результат проверки
				return (this->_session.state == state_t::GOOD);
			// Если проверяется режим уставновки постоянного подключения
			case static_cast <uint8_t> (state_t::ALIVE): {
				/**
				 * Определяем идентичность сервера
				 */
				switch(static_cast <uint8_t> (this->_session.identity)){
					// Если сервер соответствует Websocket-серверу
					case static_cast <uint8_t> (identity_t::WS):
					// Если сервер соответствует HTTP-серверу
					case static_cast <uint8_t> (identity_t::HTTP): {
						// Запрашиваем заголовок подключения
						const string & header = this->_web.header("Connection");
						// Если заголовок подключения найден
						if(!header.empty())
							// Выполняем проверку является ли соединение закрытым
							return !this->_fmk->exists("close", header);
						// Если заголовок подключения не найден
						else {
							// Переходим по всему списку заголовков
							for(auto & header : const_cast <http_t *> (this)->_web.headers()){
								// Если заголовок найден
								if(this->_fmk->compare("Connection", header.first))
									// Выполняем проверку является ли соединение закрытым
									return !this->_fmk->exists("close", header.second);
							}
						}
					} break;
					// Если сервер соответствует PROXY-серверу
					case static_cast <uint8_t> (identity_t::PROXY): {
						// Запрашиваем заголовок подключения
						const string & header = this->_web.header("Proxy-Connection");
						// Если заголовок подключения найден
						if(!header.empty())
							// Выполняем проверку является ли соединение закрытым
							return !this->_fmk->exists("close", header);
						// Если заголовок подключения не найден
						else {
							// Переходим по всему списку заголовков
							for(auto & header : const_cast <http_t *> (this)->_web.headers()){
								// Если заголовок найден
								if(this->_fmk->compare("Proxy-Connection", header.first))
									// Выполняем проверку является ли соединение закрытым
									return !this->_fmk->exists("close", header.second);
							}
						}
					} break;
				}
				// Сообщаем, что подключение постоянное
				return true;
			}
			// Если проверяется режим бракованных данных
			case static_cast <uint8_t> (state_t::BROKEN):
				// Выводрим результат проверки
				return (this->_session.state == state_t::BROKEN);
			// Если проверяется режим выполненного рукопожатия
			case static_cast <uint8_t> (state_t::HANDSHAKE):
				// Выполняем проверку на удачное рукопожатие
				return (this->_session.state == state_t::HANDSHAKE);
			// Если проверяется режим флага запраса трейлеров
			case static_cast <uint8_t> (state_t::TRAILERS):
				// Выводим проверку на установку флага запроса передачи трейлеров
				return this->_transfer.trailers;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если функция обратного вызова на на вывод ошибок установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (state)), log_t::flag_t::WARNING, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::WARNING, error.what());
		#endif
	}
	// Выводим результат
	return false;
}
/**
 * @brief Проверка заголовка HTTP-протокола является ли он стандартным
 *
 * @param name название заголовка HTTP-протокола для проверки
 * @return     результат проверки
 */
bool awh::Http::standard(const string & name) const noexcept {
	// Выводим результат проверки
	return this->_web.standard(name);
}
/**
 * @brief Метод извлечения параметров запроса
 *
 * @return установленные параметры запроса
 */
const awh::uri_t::url_t & awh::Http::url() const noexcept {
	// Выводим параметры запроса
	return this->_web.request().url;
}
/**
 * @brief Метод получения объекта запроса на сервер
 *
 * @return объект запроса на сервер
 */
const awh::web_t::req_t & awh::Http::request() const noexcept {
	// Выводим объект запроса на сервер
	return this->_web.request();
}
/**
 * @brief Метод добавления объекта запроса на сервер
 *
 * @param req объект запроса на сервер
 */
void awh::Http::request(const web_t::req_t & req) noexcept {
	// Устанавливаем объект запроса на сервер
	this->_web.request(req);
}
/**
 * @brief Метод получения объекта ответа сервера
 *
 * @return объект ответа сервера
 */
const awh::web_t::res_t & awh::Http::response() const noexcept {
	// Выводим объект ответа сервера
	return this->_web.response();
}
/**
 * @brief Метод добавления объекта ответа сервера
 *
 * @param res объект ответа сервера
 */
void awh::Http::response(const web_t::res_t & res) noexcept {
	// Устанавливаем объект ответа сервера
	this->_web.response(res);
}
/**
 * @brief Метод получения текущей даты для заголовка HTTP-протокола
 *
 * @param date дата в формате UnixTimestamp
 * @return     штамп времени в текстовом виде
 */
string awh::Http::date(const uint64_t date) const noexcept {
	// Результат работы функции
	string result = "";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Создаем структуру времени
		std::tm tm = {};
		// Создаём объект потока
		std::stringstream ss;
		// Преобразуем дату в нужный нам формат
		time_t value = static_cast <time_t> (date);
		// Если штамп времени передан в числовом виде
		if(value == 0)
			// Формируем время по умолчанию
			value = ::time(nullptr);
		// Получаем текущее значение размерности даты
		const uint8_t current = static_cast <uint8_t> (::floor(::log10(static_cast <long double> (value))));
		// Получаем размерность актуальной размерности даты
		const uint8_t actual = static_cast <uint8_t> (::floor(::log10(static_cast <long double> (::time(nullptr)))));
		// Если текущий размер выше актуального
		if(current > actual)
			// Переводим указанные единицы в секунды
			value /= static_cast <time_t> (::pow(static_cast <long double> (10), static_cast <long double> (current - actual)));
		// Формируем локальное время
		gmtime_r(&value, &tm);
		// Выполняем извлечение даты
		ss << ::put_time(&tm, "%a, %d %b %Y %H:%M:%S GMT");
		// Выводим полученное значение даты
		return ss.str();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(date), log_t::flag_t::WARNING, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::WARNING, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод получения сообщения ответа HTTP-протокола
 *
 * @param code код сообщения ответа для получения
 * @return     соответствующее коду сообщения ответа HTTP-протокола
 */
const string & awh::Http::message(const uint32_t code) const noexcept {
	/**
	 * Подробнее: https://developer.mozilla.org/ru/docs/Web/HTTP/Status
	 */
	// Результат работы функции
	static const string result = "";
	// Выполняем поиск кода сообщения
	auto i = this->_responses.find(code);
	// Если код сообщения найден
	if(i != this->_responses.end())
		// Выводим сообщение на код ответа
		return i->second;
	// Выводим результат
	return result;
}
/**
 * @brief Метод маппинга полученных данных
 *
 * @param flag флаг выполняемого процесса
 * @param http объект для маппинга
 */
void awh::Http::mapping(const process_t flag, Http & http) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем очистку списка заголовков
		http.clear(web_t::unit_t::HEADERS);
		// Устанавливаем идентификатор объекта
		http.id(this->_web.id());
		// Устанавливаем активный стейт объекта
		http._session.state = this->_session.state;
		// Устанавливаем размер одного чанка
		http._session.chunkSize = this->_session.chunkSize;
		// Устанавливаем статус рукопожатия
		http._session.handshake = this->_session.handshake;
		// Устанавливаем параметры Transfer-Encoding
		http._transfer = this->_transfer;
		// Устанавливаем флаг зашифрованной полезной нагрузки
		http._encrypt.crypted = this->_encrypt.crypted;
		// Устанавливаем флаг шифрования объекта
		http._encrypt.enabled = this->_encrypt.enabled;
		// Устанавливаем флаг компрессии полезной нагрузки
		http._compressors.current = this->_compressors.current;
		// Устанавливаем выбранный метод компрессии
		http._compressors.selected = this->_compressors.selected;
		// Выполняем установку списка поддерживаемых компрессоров
		http._compressors.supports = this->_compressors.supports;
		// Извлекаем список заголовков
		headers_t & headers = this->_web.headers();
		// Если заголовки получены, выполняем установку
		if(!headers.empty())
			// Выполняем установку заголовков
			http.headers(headers);
		// Устанавливаем параметры идентификации сервиса
		http.agent(
			this->_agent.id,
			this->_agent.name,
			this->_agent.version
		);
		// Если нужно сформировать данные запроса
		if(flag == process_t::REQUEST){
			// Устанавливаем User-Agent, если он установлен
			http._agent.user = this->_agent.user;
			// Получаем ответ с удалённого сервера
			const web_t::res_t & response = this->_web.response();
			// Если авторизация установлена как Digest
			if(this->_auth.client.type() == awh::auth_t::type_t::DIGEST){
				/**
				 * Проверяем код ответа
				 */
				switch(response.code){
					// Если требуется авторизация для сервера
					case 401: {
						// Получаем параметры авторизации
						const string & auth = this->_web.header("WWW-Authenticate");
						// Если параметры авторизации найдены
						if(!auth.empty())
							// Устанавливаем HTTP-заголовок в параметры авторизации
							http._auth.client.header(auth);
					} break;
					// Если требуется авторизация для прокси-сервера
					case 407: {
						// Получаем параметры авторизации
						const string & auth = this->_web.header("Proxy-Authenticate");
						// Если параметры авторизации найдены
						if(!auth.empty())
							// Устанавливаем HTTP-заголовок в параметры авторизации
							http._auth.client.header(auth);
					} break;
				}
			}
			/**
			 * Проверяем код ответа
			 */
			switch(response.code){
				// Если нужно произвести редирект
				case 201:
				case 301:
				case 302:
				case 303:
				case 307:
				case 308: {
					// Получаем параметры переадресации
					const string & location = this->_web.header("Location");
					// Если адрес перенаправления найден
					if(!location.empty()){
						// Получаем объект параметров запроса
						web_t::req_t request = this->_web.request();
						// Выполняем парсинг полученного URL-адреса
						request.url = this->_uri.parse(location);
						// Выполняем установку параметров запроса
						http._web.request(::move(request));
					}
				} break;
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если функция обратного вызова на на вывод ошибок установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (flag)), log_t::flag_t::WARNING, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::WARNING, error.what());
		#endif
	}
}
/**
 * @brief Метод парсинга сырых данных
 *
 * @param buffer буфер данных для обработки
 * @param size   размер буфера данных
 * @return       размер обработанных данных
 */
size_t awh::Http::parse(const char * buffer, const size_t size) noexcept {
	// Результат работы функции
	size_t result = 0;
	// Если мы ещё не зафиксировали изменения и парсинг данных необходим
	if(!this->state(state_t::END)){
		// Выполняем парсинг сырых данных
		result = this->_web.parse(buffer, size);
		// Если парсинг выполнен
		if(this->_web.finish())
			// Выполняем коммит полученного результата
			this->commit();
	}
	// Выводим реузльтат
	return result;
}
/**
 * @brief Метод получения бинарного дампа
 *
 * @return бинарный дамп данных
 */
awh::buffer_t awh::Http::dump() const noexcept {
	// Результат работы функции
	buffer_t result(this->_fmk, this->_log);
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Устанавливаем параметры активной сессии
		result.push(&this->_session, sizeof(this->_session));
		// Устанавливаем активные параметры шифрования
		result.push(&this->_encrypt, sizeof(this->_encrypt));
		// Устанавливаем параметры Transfer-Encoding
		result.push(&this->_transfer, sizeof(this->_transfer));
		// Устанавливаем метод компрессии хранимых данных
		result.push(&this->_compressors.current, sizeof(this->_compressors.current));
		// Устанавливаем метод компрессии отправляемых данных
		result.push(&this->_compressors.selected, sizeof(this->_compressors.selected));
		// Устанавливаем количество поддерживаемых компрессоров
		result.push(this->_compressors.supports.size());
		// Если список поддерживаемых компрессоров не пустой
		if(!this->_compressors.supports.empty()){
			// Выполняем перебор всех поддерживаемых компрессоров
			for(auto & compressor : this->_compressors.supports){
				// Выполняем установку веса компрессора
				result.push(compressor.first);
				// Выполняем установку идентификатора компрессора
				result.push(&compressor.second, sizeof(compressor.second));
			}
		}
		// Устанавливаем размер идентификатора сервиса
		result.push(this->_agent.id.length());
		// Устанавливаем данные идентификатора сервиса
		result.push(this->_agent.id);
		// Устанавливаем размер версии модуля приложения
		result.push(this->_agent.version.length());
		// Устанавливаем данные версии модуля приложения
		result.push(this->_agent.version);
		// Устанавливаем размер названия сервиса
		result.push(this->_agent.name.length());
		// Устанавливаем данные названия сервиса
		result.push(this->_agent.name);
		// Устанавливаем размер User-Agent для HTTP-запроса
		result.push(this->_agent.user.length());
		// Устанавливаем данные User-Agent для HTTP-запроса
		result.push(this->_agent.user);
		// Устанавливаем количество записей чёрного списка
		result.push(this->_blacklist.size());
		// Выполняем переход по всему чёрному списку
		for(auto & header : this->_blacklist){
			// Устанавливаем размер заголовка из чёрного списка
			result.push(header.length());
			// Устанавливаем данные заголовка из чёрного списка
			result.push(header);
		}
		// Получаем дамп данных модуля WEB
		buffer_t dump = this->_web.dump();
		// Устанавливаем размер буфера WEB данных
		result.push(dump.size());
		// Устанавливаем данные буфера WEB данных
		result.push(dump);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если функция обратного вызова на на вывод ошибок установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::WARNING, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки бинарного дампа
 *
 * @param data бинарный дамп данных
 */
void awh::Http::dump(const buffer_t & data) noexcept {
	// Если данные бинарного дампа переданы
	if(!data.empty())
		// Выполняем установку дампа данных
		this->dump(static_cast <const char *> (data), static_cast <size_t> (data));
}
/**
 * @brief Метод установки бинарного дампа
 *
 * @param buffer буфер бинарных данных
 * @param size   размер бинарных данных
 */
void awh::Http::dump(const char * buffer, const size_t size) noexcept {
	// Если данные бинарного дампа переданы
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Длина строки, количество элементов и смещение в буфере
			size_t length = 0, count = 0, offset = 0;
			// Извлекаем параметры активной сессии
			::memcpy(&this->_session, buffer + offset, sizeof(this->_session));
			// Выполняем смещение в буфере
			offset += sizeof(this->_session);
			// Извлекаем активные параметры шифрования
			::memcpy(&this->_encrypt, buffer + offset, sizeof(this->_encrypt));
			// Выполняем смещение в буфере
			offset += sizeof(this->_encrypt);
			// Извлекаем параметры Transfer-Encoding
			::memcpy(&this->_transfer, buffer + offset, sizeof(this->_transfer));
			// Выполняем смещение в буфере
			offset += sizeof(this->_transfer);
			// Извлекаем метод компрессии хранимых данных
			::memcpy(&this->_compressors.current, buffer + offset, sizeof(this->_compressors.current));
			// Выполняем смещение в буфере
			offset += sizeof(this->_compressors.current);
			// Извлекаем метод компрессии отправляемых данных
			::memcpy(&this->_compressors.selected, buffer + offset, sizeof(this->_compressors.selected));
			// Выполняем смещение в буфере
			offset += sizeof(this->_compressors.selected);
			// Выполняем получение количества поддерживаемых компрессоров
			::memcpy(&count, buffer + offset, sizeof(count));
			// Выполняем смещение в буфере
			offset += sizeof(count);
			// Выполняем очистку списку поддерживаемых компрессоров
			this->_compressors.supports.clear();
			// Если количество компрессоров больше нуля
			if(count > 0){
				// Вес компрессора
				float weight = .0f;
				// Идентификатор компрессора
				compressor_t compressor = compressor_t::NONE;
				// Выполняем последовательную установку всех компрессоров
				for(size_t i = 0; i < count; i++){
					// Выполняем получение веса компрессора
					::memcpy(&weight, buffer + offset, sizeof(weight));
					// Выполняем смещение в буфере
					offset += sizeof(weight);
					// Выполняем получение идентификатора компрессора
					::memcpy(&compressor, buffer + offset, sizeof(compressor));
					// Выполняем смещение в буфере
					offset += sizeof(compressor);
					// Выполняем установку метода компрессора
					this->_compressors.supports.emplace(weight, compressor);
				}
			}
			// Выполняем получение размера идентификатора сервиса
			::memcpy(&length, buffer + offset, sizeof(length));
			// Выполняем смещение в буфере
			offset += sizeof(length);
			// Если размер получен
			if(length > 0){
				// Выделяем память для данных идентификатора сервиса
				this->_agent.id.resize(length, 0);
				// Выполняем получение данных идентификатора сервиса
				::memcpy(this->_agent.id.data(), buffer + offset, length);
				// Выполняем смещение в буфере
				offset += length;
			}
			// Выполняем получение размера версии модуля приложения
			::memcpy(&length, buffer + offset, sizeof(length));
			// Выполняем смещение в буфере
			offset += sizeof(length);
			// Если размер получен
			if(length > 0){
				// Выделяем память для данных версии модуля приложения
				this->_agent.version.resize(length, 0);
				// Выполняем получение данных версии модуля приложения
				::memcpy(this->_agent.version.data(), buffer + offset, length);
				// Выполняем смещение в буфере
				offset += length;
			}
			// Выполняем получение размера названия сервиса
			::memcpy(&length, buffer + offset, sizeof(length));
			// Выполняем смещение в буфере
			offset += sizeof(length);
			// Если размер получен
			if(length > 0){
				// Выделяем память для данных названия сервиса
				this->_agent.name.resize(length, 0);
				// Выполняем получение данных названия сервиса
				::memcpy(this->_agent.name.data(), buffer + offset, length);
				// Выполняем смещение в буфере
				offset += length;
			}
			// Выполняем получение размера User-Agent для HTTP-запроса
			::memcpy(&length, buffer + offset, sizeof(length));
			// Выполняем смещение в буфере
			offset += sizeof(length);
			// Если размер получен
			if(length > 0){
				// Выделяем память для данных User-Agent для HTTP-запроса
				this->_agent.user.resize(length, 0);
				// Выполняем получение данных User-Agent для HTTP-запроса
				::memcpy(this->_agent.user.data(), buffer + offset, length);
				// Выполняем смещение в буфере
				offset += length;
			}
			// Выполняем получение количества записей чёрного списка
			::memcpy(&count, buffer + offset, sizeof(count));
			// Выполняем смещение в буфере
			offset += sizeof(count);
			// Выполняем сброс заголовков чёрного списка
			this->_blacklist.clear();
			// Если количество элементов получено
			if(count > 0){
				// Выполняем последовательную загрузку всех заголовков
				for(size_t i = 0; i < count; i++){
					// Выполняем получение размера заголовка из чёрного списка
					::memcpy(&length, buffer + offset, sizeof(length));
					// Выполняем смещение в буфере
					offset += sizeof(length);
					// Если размер получен
					if(length > 0){
						// Выделяем память для заголовка чёрного списка
						string header(length, 0);
						// Выполняем получение заголовка чёрного списка
						::memcpy(header.data(), buffer + offset, length);
						// Выполняем смещение в буфере
						offset += length;
						// Если заголовок чёрного списка получен
						if(!header.empty())
							// Выполняем добавление заголовка чёрного списка
							this->_blacklist.emplace(::move(header));
					}
				}
			}
			// Выполняем получение размера дампа WEB данных
			::memcpy(&length, buffer + offset, sizeof(length));
			// Выполняем смещение в буфере
			offset += sizeof(length);
			// Если размер получен
			if(length > 0){
				// Выполняем установку буфера модуля Web
				this->_web.dump(buffer + offset, length);
				// Выполняем смещение в буфере
				offset += length;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::WARNING, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод получения данных тела HTTP-протокола
 *
 * @return буфер данных тела HTTP-протокола
 */
awh::buffer_t & awh::Http::body() noexcept {
	// Выполняем дешифрование полезной нагрузки
	this->decrypt();
	// Выполняем декомпрессию полезной нагрузки
	this->decompress();
	// Выводим данные тела
	return this->_web.body();
}
/**
 * @brief Метод переноса данных тела HTTP-протокола
 *
 * @param body буфер тела HTTP-протокола для переноса
 */
void awh::Http::body(buffer_t && body) noexcept {
	// Если буфер тела сообщения передан
	if(!body.empty()){
		// Выполняем дешифрование полезной нагрузки
		this->decrypt();
		// Выполняем декомпрессию полезной нагрузки
		this->decompress();
		// Добавляем данные телал сообщения
		this->_web.body(::move(body));
	}
}
/**
 * @brief Метод установки данных тела HTTP-протокола
 *
 * @param body буфер тела HTTP-протокола для установки
 */
void awh::Http::body(const buffer_t & body) noexcept {
	// Если буфер тела сообщения передан
	if(!body.empty())
		// Выполняем добавление бинарных данных тела сообщения
		this->body(static_cast <const char *> (body), static_cast <size_t> (body));
}
/**
 * @brief Метод перемещения данных тела HTTP-протокола
 *
 * @param body буфер тела HTTP-протокола для установки
 */
void awh::Http::body(vector <char> && body) noexcept {
	// Если буфер тела сообщения передан
	if(!body.empty()){
		// Выполняем дешифрование полезной нагрузки
		this->decrypt();
		// Выполняем декомпрессию полезной нагрузки
		this->decompress();
		// Добавляем данные телал сообщения
		this->_web.body(::move(body));
	}
}
/**
 * @brief Метод установки данных тела HTTP-протокола
 *
 * @param body буфер тела HTTP-протокола для установки
 */
void awh::Http::body(const vector <char> & body) noexcept {
	// Если буфер тела сообщения передан
	if(!body.empty())
		// Выполняем добавление бинарных данных тела сообщения
		this->body(body.data(), body.size());
}
/**
 * @brief Метод добавления данных тела HTTP-протокола
 *
 * @param buffer буфер тела HTTP-протокола для добавления
 * @param size   размер буфера теля HTTP-протокола для добавления
 */
void awh::Http::body(const char * buffer, const size_t size) noexcept {
	// Если данные тела сообщения переданы
	if((buffer != nullptr) && (size > 0)){
		// Выполняем дешифрование полезной нагрузки
		this->decrypt();
		// Выполняем декомпрессию полезной нагрузки
		this->decompress();
		// Добавляем данные телал сообщения
		this->_web.body(buffer, size);
	}
}
/**
 * @brief Метод чтения чанка полезной нагрузки
 *
 * @return актуальный чанк полезной нагрузки
 */
awh::buffer_t awh::Http::chunk() noexcept {
	// Результат работы функции
	buffer_t result(this->_fmk, this->_log);
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем компрессию полезной нагрузки
		this->compress();
		// Выполняем шифрование полезной нагрузки
		this->encrypt();
		// Получаем собранные данные тела
		buffer_t & body = const_cast <buffer_t &> (this->_web.body());
		// Если данные тела ещё существуют
		if(!body.empty()){
			// Версия протокола HTTP
			double version = 1.1;
			/**
			 * Определяем тип HTTP-модуля
			 */
			switch(static_cast <uint8_t> (this->_web.hid())){
				// Если мы работаем с клиентом
				case static_cast <uint8_t> (web_t::hid_t::CLIENT):
					// Выполняем получение версии HTTP-протокола
					version = this->_web.request().version;
				break;
				// Если мы работаем с сервером
				case static_cast <uint8_t> (web_t::hid_t::SERVER):
					// Выполняем получение версии HTTP-протокола
					version = this->_web.response().version;
				break;
			}
			// Если нужно тело выводить в виде чанков
			if((version > 1.) && this->_transfer.chunking){
				// Если версия протокола интернета выше 1.1
				if(version > 1.1){
					// Если тело сообщения больше размера чанка
					if(body.size() >= this->_session.chunkSize){
						// Формируем результат
						result.push(static_cast <const char *> (body), static_cast <size_t> (this->_session.chunkSize));
						// Удаляем полученные данные в теле сообщения
						body.erase(this->_session.chunkSize);
					// Если тело сообщения полностью убирается в размер чанка
					} else result = ::move(body);
				// Выполняем сборку чанков для протокола HTTP/1.1
				} else {
					// Если тело сообщения больше размера чанка
					if(body.size() >= this->_session.chunkSize){
						// Получаем размер чанка в 16-й форме записи
						const string & chunk = this->_fmk->itoa <size_t> (this->_session.chunkSize, 16);
						// Добавляем разделитель
						result.push("\r\n");
						// Формируем тело чанка
						result.push(static_cast <const char *> (body), this->_session.chunkSize);
						// Удаляем полученные данные в теле сообщения
						body.erase(this->_session.chunkSize);
						// Добавляем конец запроса
						result.push("\r\n");
					// Если тело сообщения полностью убирается в размер чанка
					} else {
						// Получаем размер чанка в 16-й форме записи
						const string & chunk = this->_fmk->itoa <size_t> (body.size(), 16);
						// Добавляем разделитель
						result.push("\r\n");
						// Формируем тело чанка
						result.push(body);
						// Очищаем данные тела
						body.clear();
						/**
						 * Определяем тип HTTP-модуля
						 */
						switch(static_cast <uint8_t> (this->_web.hid())){
							// Если мы работаем с клиентом
							case static_cast <uint8_t> (web_t::hid_t::CLIENT):
								// Добавляем конец запроса
								result.push("\r\n0\r\n\r\n");
							break;
							// Если мы работаем с сервером
							case static_cast <uint8_t> (web_t::hid_t::SERVER): {
								// Если нужно отправить трейлеры
								if(!this->_trailers.empty())
									// Добавляем конец запроса
									result.push("\r\n0\r\n");
								// Добавляем конец запроса
								else result.push("\r\n0\r\n\r\n");
							} break;
						}
					}
				}
			// Выводим данные тела как есть
			} else {
				// Если тело сообщения больше размера чанка
				if(body.size() >= this->_session.chunkSize){
					// Получаем нужный нам размер данных
					result.push(static_cast <const char *> (body), static_cast <size_t> (this->_session.chunkSize));
					// Удаляем полученные данные в теле сообщения
					body.erase(static_cast <size_t> (this->_session.chunkSize));
				// Если тело сообщения полностью убирается в размер чанка
				} else result = ::move(body);
			}
		}
		// Если тело передаваемых данных уже пустое
		if(body.empty()){
			// Снимаем флаг зашифрованной полезной нагрузки
			this->_encrypt.crypted = false;
			// Снимаем флаг сжатой полезной нагрузки
			this->_compressors.current = compressor_t::NONE;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если функция обратного вызова на на вывод ошибок установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::WARNING, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод добавления чанка полезной нагрузки
 *
 * @param buffer буфер чанка полезной нагрузки
 */
void awh::Http::chunk(const buffer_t & buffer) noexcept {
	// Если буфер чанка полезной нагрузки передан
	if(!buffer.empty())
		// Выполняем добавление буфера чанка полезной нагрузки
		this->chunk(static_cast <const char *> (buffer), static_cast <size_t> (buffer));
}
/**
 * @brief Метод добавления чанка полезной нагрузки
 *
 * @param buffer буфер чанка полезной нагрузки
 */
void awh::Http::chunk(const vector <char> & buffer) noexcept {
	// Если буфер чанка полезной нагрузки передан
	if(!buffer.empty())
		// Выполняем добавление буфера чанка полезной нагрузки
		this->chunk(buffer.data(), buffer.size());
}
/**
 * @brief Метод добавления чанка полезной нагрузки
 *
 * @param buffer буфер чанка полезной нагрузки
 * @param size   размер буфера теля для добавления
 */
void awh::Http::chunk(const char * buffer, const size_t size) noexcept {
	// Если буфер чанка полезной нагрузки передан
	if((buffer != nullptr) && (size > 0))
		// Выполняем добавление буфера чанка полезной нагрузки
		this->_web.body(buffer, size);
}
/**
 * @brief Метод получения списка заголовков HTTP-протокола
 *
 * @return список существующих заголовков HTTP-протокола
 */
awh::headers_t & awh::Http::headers() noexcept {
	// Выводим список доступных заголовков
	return this->_web.headers();
}
/**
 * @brief Метод переноса списка заголовков HTTP-протокола
 *
 * @param headers список заголовков HTTP-протокола для переноса
 */
void awh::Http::headers(headers_t && headers) noexcept {
	// Если заголовки HTTP-протокола переданы
	if(!headers.empty()){
		// Выполняем установку HTTP-протокола
		this->_web.headers(::move(headers));
		// Если мы работаем с клиентом
		if(this->_web.hid() == web_t::hid_t::SERVER){
			// Если список трейлеров установлен
			if(this->_transfer.trailers && !this->_trailers.empty()){
				// Название трейлера для добавления
				string name = "";
				// Выполняем перебор всех трейлеров
				for(auto & trailer : this->_trailers){
					// Выполняем получение названия трейлера
					name = trailer.first;
					// Добавляем заголовок названия трейлера
					this->_web.header("Trailer", this->_fmk->transform(name, fmk_t::transform_t::SMART));
				}
			}
		}
	}
}
/**
 * @brief Метод установки списка заголовков HTTP-протокола
 *
 * @param headers список заголовков HTTP-протокола для установки
 */
void awh::Http::headers(const headers_t & headers) noexcept {
	// Если заголовки HTTP-протокола переданы
	if(!headers.empty()){
		// Выполняем установку HTTP-протокола
		this->_web.headers(headers);
		// Если мы работаем с клиентом
		if(this->_web.hid() == web_t::hid_t::SERVER){
			// Если список трейлеров установлен
			if(this->_transfer.trailers && !this->_trailers.empty()){
				// Название трейлера для добавления
				string name = "";
				// Выполняем перебор всех трейлеров
				for(auto & trailer : this->_trailers){
					// Выполняем получение названия трейлера
					name = trailer.first;
					// Добавляем заголовок названия трейлера
					this->_web.header("Trailer", this->_fmk->transform(name, fmk_t::transform_t::SMART));
				}
			}
		}
	}
}
/**
 * @brief Метод получения данных заголовка HTTP-протокола
 *
 * @param name название заголовка HTTP-протокола
 * @return     содержимое заголовка HTTP-протокола
 */
const string & awh::Http::header(const string & name) const noexcept {
	// Выполняем вывод запрашиваемого заголовка HTTP-протокола
	return this->_web.header(name);
}
/**
 * @brief Метод добавления заголовка HTTP-протокола
 *
 * @param name    название заголовка HTTP-протокола
 * @param content содержимое заголовка HTTP-протокола
 */
void awh::Http::header(const string & name, const string & content) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если название и значение заголовка HTTP-протокола переданы
		if(!name.empty() && !content.empty()){
			// Если заголовок содержит двоеточие
			if(name.front() == ':'){
				// Определяем соответствует ли ключ методу запроса
				if(this->_fmk->compare(":method", name)){
					// Получаем объект параметров запроса
					web_t::req_t request = this->_web.request();
					// Устанавливаем версию протокола
					request.version = 2.;
					// Если метод является GET запросом
					if(this->_fmk->compare("GET", content))
						// Выполняем установку метода запроса GET
						request.method = web_t::method_t::GET;
					// Если метод является PUT запросом
					else if(this->_fmk->compare("PUT", content))
						// Выполняем установку метода запроса PUT
						request.method = web_t::method_t::PUT;
					// Если метод является POST запросом
					else if(this->_fmk->compare("POST", content))
						// Выполняем установку метода запроса POST
						request.method = web_t::method_t::POST;
					// Если метод является HEAD запросом
					else if(this->_fmk->compare("HEAD", content))
						// Выполняем установку метода запроса HEAD
						request.method = web_t::method_t::HEAD;
					// Если метод является PATCH запросом
					else if(this->_fmk->compare("PATCH", content))
						// Выполняем установку метода запроса PATCH
						request.method = web_t::method_t::PATCH;
					// Если метод является TRACE запросом
					else if(this->_fmk->compare("TRACE", content))
						// Выполняем установку метода запроса TRACE
						request.method = web_t::method_t::TRACE;
					// Если метод является DELETE запросом
					else if(this->_fmk->compare("DELETE", content))
						// Выполняем установку метода запроса DELETE
						request.method = web_t::method_t::DEL;
					// Если метод является OPTIONS запросом
					else if(this->_fmk->compare("OPTIONS", content))
						// Выполняем установку метода запроса OPTIONS
						request.method = web_t::method_t::OPTIONS;
					// Если метод является CONNECT запросом
					else if(this->_fmk->compare("CONNECT", content))
						// Выполняем установку метода запроса CONNECT
						request.method = web_t::method_t::CONNECT;
					// Выполняем сохранение параметров запроса
					this->_web.request(::move(request));
					// Выходим из функции
					return;
				// Если ключ запроса соответствует пути запроса
				} else if(this->_fmk->compare(":path", name)) {
					// Получаем объект параметров запроса
					web_t::req_t request = this->_web.request();
					// Выполняем установку пути запроса
					this->_uri.create(request.url, this->_uri.parse(content));
					// Выполняем сохранение параметров запроса
					this->_web.request(::move(request));
					// Выходим из функции
					return;
				// Если ключ заголовка соответствует протоколу подключения
				} else if(this->_fmk->compare(":protocol", name)) {
					/**
					 * Определяем тип HTTP-модуля
					 */
					switch(static_cast <uint8_t> (this->_web.hid())){
						// Если мы работаем с клиентом
						case static_cast <uint8_t> (web_t::hid_t::CLIENT):
							// Выводим сообщение о невозможности установки трейлера
							this->_log->print("Client cannot support header [%s=%s]", log_t::flag_t::WARNING, name.c_str(), content.c_str());
						break;
						// Если мы работаем с сервером
						case static_cast <uint8_t> (web_t::hid_t::SERVER): {
							// Если протокол принадлежит Websocket
							if(this->_fmk->compare("websocket", content))
								// Выполняем установку типа протокола для переключению на Websocket
								this->_web.upgrade(web_t::proto_t::WEBSOCKET);
							// Если протокол принадлежит HTTP/2
							else if(this->_fmk->compare("HTTP/2.0", content) || this->_fmk->compare("h2c", content))
								// Выполняем установку типа протокола для переключению на HTTP/2
								this->_web.upgrade(web_t::proto_t::HTTP2);
							// Устанавливаем тип протокола как неизвестный
							else this->_web.upgrade(web_t::proto_t::UNKNOWN);
							// Если сервер является Web-сервером и протокол соответствует Websocket-у
							if((this->_session.identity == identity_t::HTTP) && (this->_web.upgrade() == web_t::proto_t::WEBSOCKET))
								// Выполняем установку идентичность протоколу Websocket
								this->_session.identity = identity_t::WS;
						} break;
					}
					// Выходим из функции
					return;
				// Если ключ заголовка соответствует схеме протокола
				} else if(this->_fmk->compare(":scheme", name)) {
					// Получаем объект параметров запроса
					web_t::req_t request = this->_web.request();
					// Выполняем установку схемы запроса
					request.url.schema = content;
					// Если протокол подключения защищённый а порт установлен неправильный
					if(this->_fmk->compare("https", content)){
						/**
						 * Определяем тип установленного порта
						 */
						switch(request.url.port){
							// Если порт не установлен
							case 0:
							// Если HTTP-порт установлен незащищённый то исправляем его
							case SERVER_PORT: request.url.port = SERVER_SEC_PORT; break;
							// Если PROXY-порт установлен незащищённый то исправляем его
							case SERVER_PROXY_PORT: request.url.port = SERVER_PROXY_SEC_PORT; break;
						}
					}
					// Выполняем сохранение параметров запроса
					this->_web.request(::move(request));
					// Выходим из функции
					return;
				// Если ключ соответствует доменному имени
				} else if(this->_fmk->compare(":authority", name)) {
					// Создаём объект работы с IP-адресами
					net_t net(this->_log);
					// Устанавливаем хост
					this->_web.header("Host", content);
					// Получаем объект параметров запроса
					web_t::req_t request = this->_web.request();
					// Получаем хост запрашиваемого сервера
					request.url.host = content;
					// Если данные хоста ещё не установлены
					if(request.url.schema.empty() || (request.url.port == 0)){
						// Выполняем установку схемы запроса
						request.url.schema = "http";
						// Выполняем установку порта по умолчанию
						request.url.port = SERVER_PORT;
					}
					// Выполняем поиск разделителя
					const size_t pos = request.url.host.rfind(':');
					// Если разделитель найден
					if(pos != string::npos){
						// Получаем порт сервера
						const string & port = request.url.host.substr(pos + 1);
						// Если данные порта являются числом
						if(this->_fmk->is(port, fmk_t::check_t::NUMBER)){
							/**
							 * Выполняем отлов ошибок
							 */
							try {
								// Выполняем установку порта сервера
								request.url.port = static_cast <uint32_t> (::stoi(port));
								// Выполняем получение хоста сервера
								request.url.host = request.url.host.substr(0, pos);
								// Если порт установлен как 443
								if(request.url.port == 443)
									// Выполняем установку защищённую схему запроса
									request.url.schema = "https";
							/**
							 * Если возникает ошибка
							 */
							} catch(const exception &) {
								// Выполняем установку порта сервера
								request.url.port = 0;
							}
						}
					}
					/**
					 * Определяем тип домена
					 */
					switch(static_cast <uint8_t> (net.host(request.url.host))){
						// Если передан IP-адрес сети IPv4
						case static_cast <uint8_t> (net_t::type_t::IPV4): {
							// Выполняем установку семейства IP-адресов
							request.url.family = AF_INET;
							// Выполняем установку IPv4-адреса
							request.url.ip = request.url.host;
						} break;
						// Если передан IP-адрес сети IPv6
						case static_cast <uint8_t> (net_t::type_t::IPV6): {
							// Выполняем установку семейства IP-адресов
							request.url.family = AF_INET6;
							// Выполняем установку IPv6-адреса
							request.url.ip = net = request.url.host;
						} break;
						// Если передана доменная зона
						case static_cast <uint8_t> (net_t::type_t::FQDN):
							// Выполняем установку IPv6-адреса
							request.url.domain = this->_fmk->transform(request.url.host, fmk_t::transform_t::LOWER);
						break;
					}
					// Выполняем сохранение параметров запроса
					this->_web.request(::move(request));
					// Выходим из функции
					return;
				// Если ключ соответствует статусу ответа
				} else if(this->_fmk->compare(":status", name)) {
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Получаем объект параметров ответа
						web_t::res_t response = this->_web.response();
						// Устанавливаем версию протокола
						response.version = 2.;
						// Выполняем установку статуса ответа
						response.code = static_cast <uint32_t> (::stoi(content));
						// Выполняем формирование текста ответа
						response.message = this->message(response.code);
						// Выполняем сохранение параметров ответа
						this->_web.response(::move(response));
					/**
					 * Если возникает ошибка
					 */
					} catch(const exception &) {
						// Получаем объект параметров ответа
						web_t::res_t response = this->_web.response();
						// Выполняем установку статуса ответа
						response.code = 500;
						// Устанавливаем версию протокола
						response.version = 2.;
						// Выполняем формирование текста ответа
						response.message = this->message(response.code);
						// Выполняем сохранение параметров ответа
						this->_web.response(::move(response));
					}
					// Выходим из функции
					return;
				}
			}
			// Устанавливаем заголовок HTTP-протокола
			this->_web.header(name, content);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если функция обратного вызова на на вывод ошибок установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name, content), log_t::flag_t::WARNING, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::WARNING, error.what());
		#endif
	}
}
/**
 * @brief Метод получение типа протокола для переключения
 *
 * @return тип протокола для переключения
 */
const awh::web_t::proto_t awh::Http::upgrade() const noexcept {
	// Выводим название протокола для переключения
	return this->_web.upgrade();
}
/**
 * @brief Метод установки типа протокола для переключения
 *
 * @param upgrade тип протокола для переключения
 */
void awh::Http::upgrade(const web_t::proto_t upgrade) noexcept {
	// Выполняем установку название протокола для переключения
	this->_web.upgrade(upgrade);
}
/**
 * @brief Метод извлечения выбранного метода компрессии
 *
 * @return метод компрессии
 */
awh::Http::compressor_t awh::Http::compression() const noexcept {
	// Выполняем извлечение выбранного метода компрессии
	return this->_compressors.selected;
}
/**
 * @brief Метод установки выбранного метода компрессии
 *
 * @param compressor метод компрессии
 */
void awh::Http::compression(const compressor_t compressor) noexcept {
	// Выполняем установку выбранного метода компрессии
	this->_compressors.selected = compressor;
}
/**
 * @brief Метод установки списка поддерживаемых компрессоров
 *
 * @param compressors методы компрессии данных полезной нагрузки
 */
void awh::Http::compressors(const vector <compressor_t> & compressors) noexcept {
	// Если список архиваторов передан
	if(!compressors.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Вес запрашиваемого компрессора
			float weight = 1.f;
			// Выполняем очистку списка доступных компрессоров
			this->_compressors.supports.clear();
			// Выполняем перебор списка запрашиваемых компрессоров
			for(auto & compressor : compressors){
				// Выполняем установку полученного компрессера
				this->_compressors.supports.emplace(weight, compressor);
				// Выполняем уменьшение веса компрессора
				weight -= .1f;
			}
			/**
			 * Здесь нам не нужно выбирать конкретный компрессор как это делаем в модуле Websocket
			 * Так-как в запросе должно улететь определённое количество поддерживаемых компрессоров, а иначе улетит только один
			 */
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод получения агента сервера для HTTP-протокола
 *
 * @param flag флаг выполняемого процесса
 * @return     сформированный агент
 */
string awh::Http::agent(const process_t flag) const noexcept {
	// Результат работы функции
	string result = "";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем флаг выполняемого процесса
		 */
		switch(static_cast <uint8_t> (flag)){
			// Если нужно сформировать данные запроса
			case static_cast <uint8_t> (process_t::REQUEST): {
				// Название операционной системы
				const char * os = nullptr;
				/**
				 * Определяем название операционной системы
				 */
				switch(static_cast <uint8_t> (this->_os.family())){
					// Если операционной системой является Unix
					case static_cast <uint8_t> (os_t::family_t::UNIX):
						// Устанавливаем название операционной системы
						os = "Unix";
					break;
					// Если операционной системой является Linux
					case static_cast <uint8_t> (os_t::family_t::LINUX):
						// Устанавливаем название операционной системы
						os = "Linux";
					break;
					// Если операционной системой является неизвестной
					case static_cast <uint8_t> (os_t::family_t::NONE):
						// Устанавливаем название операционной системы
						os = "Unknown";
					break;
					// Если операционной системой является Windows
					case static_cast <uint8_t> (os_t::family_t::WIND32):
					case static_cast <uint8_t> (os_t::family_t::WIND64):
						// Устанавливаем название операционной системы
						os = "Windows";
					break;
					// Если операционной системой является MacOS X
					case static_cast <uint8_t> (os_t::family_t::MACOSX):
						// Устанавливаем название операционной системы
						os = "MacOS X";
					break;
					// Если операционной системой является FreeBSD
					case static_cast <uint8_t> (os_t::family_t::FREEBSD):
						// Устанавливаем название операционной системы
						os = "FreeBSD";
					break;
					// Если операционной системой является NetBSD
					case static_cast <uint8_t> (os_t::family_t::NETBSD):
						// Устанавливаем название операционной системы
						os = "NetBSD";
					break;
					// Если операционной системой является OpenBSD
					case static_cast <uint8_t> (os_t::family_t::OPENBSD):
						// Устанавливаем название операционной системы
						os = "OpenBSD";
					break;
					// Если операционной системой является Sun Solaris
					case static_cast <uint8_t> (os_t::family_t::SOLARIS):
						// Устанавливаем название операционной системы
						os = "Solaris";
					break;
				}
				// Выполняем генерацию Юзер-агента клиента выполняющего HTTP-запрос
				result = this->_fmk->format(
					"%s (%s; %s/%s)",
					this->_agent.name.c_str(), os,
					this->_agent.id.c_str(),
					this->_agent.version.c_str()
				);
			} break;
			// Если нужно сформировать данные ответа
			case static_cast <uint8_t> (process_t::RESPONSE):
				// Выполняем установку агента парсера
				result = this->_fmk->format(
					"%s/%s",
					this->_agent.id.c_str(),
					this->_agent.version.c_str()
				);
			break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если функция обратного вызова на на вывод ошибок установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (flag)), log_t::flag_t::WARNING, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::WARNING, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки пользовательского агента для HTTP-протокола
 *
 * @param agent пользовательский агент для HTTP-протокола
 */
void awh::Http::agent(const string & agent) noexcept {
	// Устанавливаем User-Agent
	if(!agent.empty())
		// Выполняем установку User-Agent
		this->_agent.user = agent;
}
/**
 * @brief Метод установки агента сервера для HTTP-протокола
 *
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::Http::agent(const string & id, const string & name, const string & ver) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если идентификатор сервиса передан
		if(!id.empty())
			// Устанавливаем идентификатор сервиса
			this->_agent.id = id;
		// Если название сервиса передано
		if(!name.empty())
			// Устанавливаем название сервиса
			this->_agent.name = name;
		// Если версия сервиса передана
		if(!ver.empty())
			// Устанавливаем версию сервиса
			this->_agent.version = ver;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если функция обратного вызова на на вывод ошибок установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, name, ver), log_t::flag_t::WARNING, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::WARNING, error.what());
		#endif
	}
}
/**
 * @brief Метод получения количества установленных трейлеров
 *
 * @return количество установленных трейлеров
 */
size_t awh::Http::trailersCount() const noexcept {
	// Выводим список установленных трейлеров
	return this->_trailers.size();
}
/**
 * @brief Метод получения буфера отправляемых трейлеров
 *
 * @return буфер данных ответа в бинарном виде
 */
awh::buffer_t awh::Http::trailers() const noexcept {
	// Результат работы функции
	buffer_t result(this->_fmk, this->_log);
	// Если разрешено добавление трейлеров
	if(this->_transfer.trailers){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если список трейлеров получен
			if(!this->_trailers.empty()){
				// Получаем первый трейлер из списка
				auto i = this->_trailers.begin();
				// Получаем название заголовка
				const string name = i->first;
				// Переводим заголовок в нормальный режим
				this->_fmk->transform(name, fmk_t::transform_t::SMART);
				// Сформированный отправляемый ответ
				string response = this->_fmk->format("%s: %s\r\n", name.c_str(), i->second.c_str());
				// Выполняем удаление отправляемого трейлера из списка
				const_cast <http_t *> (this)->_trailers.erase(i);
				// Если трейлеров в списке больше нет
				if(this->_trailers.empty())
					// Выполняем добавление конца запроса
					response.append("\r\n");
				// Устанавливаем результат
				result.push(response);
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод получения буфера отправляемых трейлеров (для протокола HTTP/2)
 *
 * @return буфер данных ответа в бинарном виде
 */
vector <std::pair <string, string>> awh::Http::trailers2() const noexcept {
	// Результат работы функции
	vector <std::pair <string, string>> result;
	// Если разрешено добавление трейлеров
	if(this->_transfer.trailers){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если список трейлеров получен
			if(!this->_trailers.empty()){
				// Переходим по всему списку доступных трейлеров
				for(auto i = this->_trailers.begin(); i != this->_trailers.end();){
					// Устанавливаем трейлер в список для отправки
					result.push_back(std::make_pair(i->first, i->second));
					// Выполняем удаление отправляемого трейлера из списка
					i = const_cast <http_t *> (this)->_trailers.erase(i);
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки трейлера
 *
 * @param name    название заголовка HTTP-протокола
 * @param content содержимое заголовка HTTP-протокола
 */
void awh::Http::trailer(const string & name, const string & content) noexcept {
	// Если название и содержимое заголовка HTTP-протокола переданы
	if(!name.empty() && !content.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем тип HTTP-модуля
			 */
			switch(static_cast <uint8_t> (this->_web.hid())){
				// Если мы работаем с клиентом
				case static_cast <uint8_t> (web_t::hid_t::CLIENT): {
					// Выводим сообщение, что клиент не может отправлять трейлеры
					this->_log->print("Add trailer [%s=%s] failed because the client cannot send trailers", log_t::flag_t::WARNING, name.c_str(), content.c_str());
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callback.is("error"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, this->_fmk->format("Add trailer [%s=%s] failed because the client cannot send trailers", name.c_str(), content.c_str()));
				} break;
				// Если мы работаем с сервером
				case static_cast <uint8_t> (web_t::hid_t::SERVER): {
					// Если разрешено добавление трейлеров
					if(this->_transfer.trailers){
						// Добавляем заголовок названия трейлера
						this->_web.header("Trailer", name);
						// Выполняем добавление заголовка в список трейлеров
						this->_trailers.emplace(this->_fmk->transform(name, fmk_t::transform_t::LOWER), content);
					// Если добавление трейлеров не запрашивалось клиентом
					} else {
						// Выводим сообщение о невозможности установки трейлера
						this->_log->print("It is impossible to add a [%s=%s] trailer because the client did not request the transfer of trailers", log_t::flag_t::WARNING, name.c_str(), content.c_str());
						// Если функция обратного вызова на на вывод ошибок установлена
						if(this->_callback.is("error"))
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, this->_fmk->format("It is impossible to add a [%s=%s] trailer because the client did not request the transfer of trailers", name.c_str(), content.c_str()));
					}
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name, content), log_t::flag_t::WARNING, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод извлечения строки авторизации
 *
 * @param flag флаг выполняемого процесса
 * @param prov параметры провайдера обмена сообщениями
 * @return     строка авторизации на удалённом сервере
 */
string awh::Http::auth(const process_t flag, const web_t::provider_t & prov) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем флаг выполняемого процесса
		 */
		switch(static_cast <uint8_t> (flag)){
			// Если нужно сформировать данные запроса
			case static_cast <uint8_t> (process_t::REQUEST): {
				// Получаем объект ответа клиенту
				const web_t::req_t & req = static_cast <const web_t::req_t &> (prov);
				// Если параметры HTTP-запроса переданы
				if(!req.url.empty() && (req.method != web_t::method_t::NONE)){
					// Устанавливаем параметры HTTP-запроса
					this->_auth.client.uri(this->_uri.url(req.url));
					/**
					 * Определяем метод запроса
					 */
					switch(static_cast <uint8_t> (req.method)){
						// Если метод запроса указан как GET
						case static_cast <uint8_t> (web_t::method_t::GET):
							// Получаем параметры авторизации
							return this->_auth.client.auth("get");
						// Если метод запроса указан как PUT
						case static_cast <uint8_t> (web_t::method_t::PUT):
							// Получаем параметры авторизации
							return this->_auth.client.auth("put");
						// Если метод запроса указан как POST
						case static_cast <uint8_t> (web_t::method_t::POST):
							// Получаем параметры авторизации
							return this->_auth.client.auth("post");
						// Если метод запроса указан как HEAD
						case static_cast <uint8_t> (web_t::method_t::HEAD):
							// Получаем параметры авторизации
							return this->_auth.client.auth("head");
						// Если метод запроса указан как DELETE
						case static_cast <uint8_t> (web_t::method_t::DEL):
							// Получаем параметры авторизации
							return this->_auth.client.auth("delete");
						// Если метод запроса указан как PATCH
						case static_cast <uint8_t> (web_t::method_t::PATCH):
							// Получаем параметры авторизации
							return this->_auth.client.auth("patch");
						// Если метод запроса указан как TRACE
						case static_cast <uint8_t> (web_t::method_t::TRACE):
							// Получаем параметры авторизации
							return this->_auth.client.auth("trace");
						// Если метод запроса указан как OPTIONS
						case static_cast <uint8_t> (web_t::method_t::OPTIONS):
							// Получаем параметры авторизации
							return this->_auth.client.auth("options");
						// Если метод запроса указан как CONNECT
						case static_cast <uint8_t> (web_t::method_t::CONNECT):
							// Получаем параметры авторизации
							return this->_auth.client.auth("connect");
					}
				}
			} break;
			// Если нужно сформировать данные ответа
			case static_cast <uint8_t> (process_t::RESPONSE):
				// Получаем параметры авторизации
				return this->_auth.server;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если функция обратного вызова на на вывод ошибок установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (flag)), log_t::flag_t::WARNING, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::WARNING, error.what());
		#endif
	}
	// Выводим результат
	return "";
}
/**
 * @brief Метод извлечения список протоколов к которому принадлежит заголовок HTTP-протокола
 *
 * @param name название заголовка HTTP-протокола
 * @return     список соответствующих протоколов
 */
const std::set <awh::web_t::proto_t> & awh::Http::proto(const string & name) const noexcept {
	// Выполняем извлечение списка протоколов к которому принадлежит заголовок
	return this->_web.proto(name);
}
/**
 * @brief Метод создания запроса для авторизации на прокси-сервере
 *
 * @param req объект параметров HTTP-запроса
 * @return    буфер данных запроса в бинарном виде
 */
awh::buffer_t awh::Http::proxy(const web_t::req_t & req) noexcept {
	// Результат работы функции
	buffer_t result(this->_fmk, this->_log);
	// Если хост сервера получен
	if(!req.url.host.empty() && (req.url.port > 0) && (req.method == web_t::method_t::CONNECT)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Добавляем в чёрный список заголовок Accept
			this->addToBlacklist("Accept");
			// Добавляем в чёрный список заголовок Accept-Language
			this->addToBlacklist("Accept-Language");
			// Добавляем в чёрный список заголовок Accept-Encoding
			this->addToBlacklist("Accept-Encoding");
			// Извлекаем заголовки HTTP-протокола
			headers_t & headers = this->_web.headers();
			// Если заголовок подключения ещё не существует
			if(!headers.has("Connection"))
				// Добавляем поддержку постоянного подключения
				headers.emplace("Connection", "keep-alive");
			// Если заголовок подключения прокси ещё не существует
			if(!headers.has("Proxy-Connection"))
				// Добавляем поддержку постоянного подключения для прокси-сервера
				headers.emplace("Proxy-Connection", "keep-alive");
			// Устанавливаем параметры HTTP-запроса
			this->_auth.client.uri(this->_uri.url(req.url));
			// Устанавливаем парарметр запроса
			this->_web.request(req);
			// Выполняем создание запроса
			result = ::move(this->process(process_t::REQUEST, dynamic_cast <const web_t::provider_t &> (req)));
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод создания запроса для авторизации на прокси-сервере (для протокола HTTP/2)
 *
 * @param req объект параметров HTTP-запроса
 * @return    буфер данных запроса в бинарном виде
 */
vector <std::pair <string, string>> awh::Http::proxy2(const web_t::req_t & req) noexcept {
	// Если хост сервера получен
	if(!req.url.host.empty() && (req.url.port > 0) && (req.method == web_t::method_t::CONNECT)){
		// Добавляем в чёрный список заголовок Accept
		this->addToBlacklist("Accept");
		// Добавляем в чёрный список заголовок Accept-Language
		this->addToBlacklist("Accept-Language");
		// Добавляем в чёрный список заголовок Accept-Encoding
		this->addToBlacklist("Accept-Encoding");
		// Добавляем заголовок протокола подключения
		this->header(":protocol", "proxy");
		// Устанавливаем параметры HTTP-запроса
		this->_auth.client.uri(this->_uri.url(req.url));
		// Устанавливаем парарметр запроса
		this->_web.request(req);
		// Выполняем создание запроса
		return this->process2(process_t::REQUEST, dynamic_cast <const web_t::provider_t &> (req));
	}
	// Выводим результат
	return vector <std::pair <string, string>> ();
}
/**
 * @brief Метод создания отрицательного ответа
 *
 * @param res объект параметров HTTP-ответа
 * @return    буфер данных ответа в бинарном виде
 */
awh::buffer_t awh::Http::reject(const web_t::res_t & res) noexcept {
	// Результат работы функции
	buffer_t result(this->_fmk, this->_log);
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если текст сообщения не установлен
		if(res.message.empty())
			// Выполняем установку сообщения
			const_cast <web_t::res_t &> (res).message = this->message(res.code);
		// Если сообщение получено
		if(!res.message.empty()){
			// Извлекаем заголовки HTTP-протокола
			headers_t & headers = this->_web.headers();
			// Выполняем очистку заголовков HTTP-протокола
			headers.clear();
			/**
			 * Определяем код ответа авторизационных данных
			 */
			switch(res.code){
				// Если код ответа соответствует авторизации на HTTP-сервере
				case 401: {
					// Если заголовок подключения ещё не существует
					if(!headers.has("Connection"))
						// Добавляем заголовок постоянного подключения
						headers.emplace("Connection", "keep-alive");
				} break;
				// Если код ответа соответствует авторизации на PROXY-сервере
				case 407: {
					// Если заголовок подключения ещё не существует
					if(!headers.has("Connection"))
						// Добавляем заголовок постоянного подключения на HTTP-сервере
						headers.emplace("Connection", "keep-alive");
					// Если заголовок подключения ещё не существует
					if(!headers.has("Proxy-Connection"))
						// Добавляем заголовок постоянного подключения на PROXY-сервере
						headers.emplace("Proxy-Connection", "keep-alive");
				} break;
				// Для всех остальных кодов ответа
				default: {
					/**
					 * Определяем идентичность сервера
					 */
					switch(static_cast <uint8_t> (this->_session.identity)){
						// Если сервер соответствует Websocket-серверу
						case static_cast <uint8_t> (identity_t::WS):
						// Если сервер соответствует HTTP-серверу
						case static_cast <uint8_t> (identity_t::HTTP): {
							// Если заголовок подключения ещё не существует
							if(!headers.has("Connection"))
								// Добавляем заголовок закрытия подключения
								headers.emplace("Connection", "close");
						} break;
						// Если сервер соответствует PROXY-серверу
						case static_cast <uint8_t> (identity_t::PROXY): {
							// Если заголовок подключения ещё не существует
							if(!headers.has("Connection"))
								// Добавляем заголовок закрытия подключения на HTTP-сервере
								headers.emplace("Connection", "close");
							// Если заголовок подключения ещё не существует
							if(!headers.has("Proxy-Connection"))
								// Добавляем заголовок закрытия подключения на PROXY-сервере
								headers.emplace("Proxy-Connection", "close");
						} break;
					}
				}
			}
			// Добавляем заголовок тип контента
			headers.emplace("Content-type", "text/html; charset=utf-8");
			// Если запрос должен содержать тело сообщения
			if((res.code >= 200) && (res.code != 204) && (res.code != 304) && (res.code != 308)){
				// Получаем данные тела
				buffer_t & body = this->_web.body();
				// Если тело ответа не установлено, устанавливаем своё
				if(body.empty()){
					// Формируем тело ответа
					const string & response = this->_fmk->format(
						"<html>\n<head>\n<title>%u %s</title>\n</head>\n<body>\n<h2>%u %s</h2>\n</body>\n</html>\n",
						res.code, res.message.c_str(), res.code, res.message.c_str()
					);
					// Добавляем тело сообщения
					body.push(response);
				}
				// Добавляем заголовок тела сообщения
				headers.emplace("Content-Length", std::to_string(body.size()));
			}
			// Устанавливаем парарметр ответа
			this->_web.response(res);
			// Выводим результат
			result = ::move(this->process(process_t::RESPONSE, dynamic_cast <const web_t::provider_t &> (res)));
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если функция обратного вызова на на вывод ошибок установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::WARNING, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод создания отрицательного ответа (для протокола HTTP/2)
 *
 * @param res объект параметров HTTP-ответа
 * @return    буфер данных ответа в бинарном виде
 */
vector <std::pair <string, string>> awh::Http::reject2(const web_t::res_t & res) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если текст сообщения не установлен
		if(res.message.empty())
			// Выполняем установку сообщения
			const_cast <web_t::res_t &> (res).message = this->message(res.code);
		// Если сообщение получено
		if(!res.message.empty()){
			// Извлекаем заголовки HTTP-протокола
			headers_t & headers = this->_web.headers();
			// Выполняем очистку заголовков HTTP-протокола
			headers.clear();
			/**
			 * Определяем код ответа авторизационных данных
			 */
			switch(res.code){
				// Если код ответа соответствует авторизации на HTTP-сервере
				case 401:
					// Добавляем заголовок постоянного подключения
					headers.emplace("Connection", "keep-alive");
				break;
				// Если код ответа соответствует авторизации на PROXY-сервере
				case 407: {
					// Добавляем заголовок постоянного подключения на HTTP-сервере
					headers.emplace("Connection", "keep-alive");
					// Добавляем заголовок постоянного подключения на PROXY-сервере
					headers.emplace("Proxy-Connection", "keep-alive");
				} break;
				// Для всех остальных кодов ответа
				default: {
					/**
					 * Определяем идентичность сервера
					 */
					switch(static_cast <uint8_t> (this->_session.identity)){
						// Если сервер соответствует Websocket-серверу
						case static_cast <uint8_t> (identity_t::WS):
						// Если сервер соответствует HTTP-серверу
						case static_cast <uint8_t> (identity_t::HTTP):
							// Добавляем заголовок закрытия подключения
							headers.emplace("Connection", "close");
						break;
						// Если сервер соответствует PROXY-серверу
						case static_cast <uint8_t> (identity_t::PROXY): {
							// Добавляем заголовок закрытия подключения на HTTP-сервере
							headers.emplace("Connection", "close");
							// Добавляем заголовок закрытия подключения на PROXY-сервере
							headers.emplace("Proxy-Connection", "close");
						} break;
					}
				}
			}
			// Добавляем заголовок тип контента
			headers.emplace("Content-type", "text/html; charset=utf-8");
			// Если запрос должен содержать тело сообщения
			if((res.code >= 200) && (res.code != 204) && (res.code != 304) && (res.code != 308)){
				// Получаем данные тела
				buffer_t & body = this->_web.body();
				// Если тело ответа не установлено, устанавливаем своё
				if(body.empty()){
					// Формируем тело ответа
					const string & response = this->_fmk->format(
						"<html>\n<head>\n<title>%u %s</title>\n</head>\n<body>\n<h2>%u %s</h2>\n</body>\n</html>\n",
						res.code, res.message.c_str(), res.code, res.message.c_str()
					);
					// Добавляем тело сообщения
					body.push(response);
				}
				// Добавляем заголовок тела сообщения
				headers.emplace("Content-Length", std::to_string(body.size()));
			}
			// Устанавливаем парарметр ответа
			this->_web.response(res);
			// Выводим результат
			return this->process2(process_t::RESPONSE, dynamic_cast <const web_t::provider_t &> (res));
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если функция обратного вызова на на вывод ошибок установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::WARNING, error.what());
		#endif
	}
	// Выводим результат
	return vector <std::pair <string, string>> ();
}
/**
 * @brief Метод создания выполняемого процесса в бинарном виде
 *
 * @param flag флаг выполняемого процесса
 * @param prov параметры провайдера обмена сообщениями
 * @return     буфер данных в бинарном виде
 */
awh::buffer_t awh::Http::process(const process_t flag, const web_t::provider_t & prov) noexcept {
	// Результат работы функции
	buffer_t result(this->_fmk, this->_log);
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем флаг выполняемого процесса
		 */
		switch(static_cast <uint8_t> (flag)){
			// Если нужно сформировать данные запроса
			case static_cast <uint8_t> (process_t::REQUEST): {
				// Получаем объект ответа клиенту
				const web_t::req_t & req = static_cast <const web_t::req_t &> (prov);
				// Если параметры HTTP-запроса переданы
				if(!req.url.empty() && (req.method != web_t::method_t::NONE)){
					/**
					 * Определяем метод запроса
					 */
					switch(static_cast <uint8_t> (req.method)){
						// Если метод запроса указан как GET
						case static_cast <uint8_t> (web_t::method_t::GET):
							// Формируем GET запрос
							result.push(this->_fmk->format("GET %s HTTP/%s\r\n", this->_uri.query(req.url).c_str(), this->_fmk->noexp(req.version, true).c_str()));
						break;
						// Если метод запроса указан как PUT
						case static_cast <uint8_t> (web_t::method_t::PUT):
							// Формируем PUT запрос
							result.push(this->_fmk->format("PUT %s HTTP/%s\r\n", this->_uri.query(req.url).c_str(), this->_fmk->noexp(req.version, true).c_str()));
						break;
						// Если метод запроса указан как POST
						case static_cast <uint8_t> (web_t::method_t::POST):
							// Формируем POST запрос
							result.push(this->_fmk->format("POST %s HTTP/%s\r\n", this->_uri.query(req.url).c_str(), this->_fmk->noexp(req.version, true).c_str()));
						break;
						// Если метод запроса указан как HEAD
						case static_cast <uint8_t> (web_t::method_t::HEAD):
							// Формируем HEAD запрос
							result.push(this->_fmk->format("HEAD %s HTTP/%s\r\n", this->_uri.query(req.url).c_str(), this->_fmk->noexp(req.version, true).c_str()));
						break;
						// Если метод запроса указан как PATCH
						case static_cast <uint8_t> (web_t::method_t::PATCH):
							// Формируем PATCH запрос
							result.push(this->_fmk->format("PATCH %s HTTP/%s\r\n", this->_uri.query(req.url).c_str(), this->_fmk->noexp(req.version, true).c_str()));
						break;
						// Если метод запроса указан как TRACE
						case static_cast <uint8_t> (web_t::method_t::TRACE):
							// Формируем TRACE запрос
							result.push(this->_fmk->format("TRACE %s HTTP/%s\r\n", this->_uri.query(req.url).c_str(), this->_fmk->noexp(req.version, true).c_str()));
						break;
						// Если метод запроса указан как DELETE
						case static_cast <uint8_t> (web_t::method_t::DEL):
							// Формируем DELETE запрос
							result.push(this->_fmk->format("DELETE %s HTTP/%s\r\n", this->_uri.query(req.url).c_str(), this->_fmk->noexp(req.version, true).c_str()));
						break;
						// Если метод запроса указан как OPTIONS
						case static_cast <uint8_t> (web_t::method_t::OPTIONS):
							// Формируем OPTIONS запрос
							result.push(this->_fmk->format("OPTIONS %s HTTP/%s\r\n", this->_uri.query(req.url).c_str(), this->_fmk->noexp(req.version, true).c_str()));
						break;
						// Если метод запроса указан как CONNECT
						case static_cast <uint8_t> (web_t::method_t::CONNECT): {
							// Формируем CONNECT запрос
							result.push(this->_fmk->format("CONNECT %s HTTP/%s\r\n", this->_fmk->format("%s:%u", req.url.host.c_str(), req.url.port).c_str(), this->_fmk->noexp(req.version, true).c_str()));
						} break;
					}
					/**
					 * Определяем тип HTTP-модуля
					 */
					switch(static_cast <uint8_t> (this->_web.hid())){
						// Если мы работаем с клиентом
						case static_cast <uint8_t> (web_t::hid_t::CLIENT): {
							/**
							 * Типы основных заголовков
							 */
							bool available[15] = {
								false, // te
								false, // Host
								false, // Accept
								false, // Origin
								false, // User-Agent
								false, // Connection
								false, // Proxy-Connection
								false, // Content-Length
								false, // Accept-Language
								false, // Accept-Encoding
								false, // Content-Encoding
								false, // Transfer-Encoding
								false, // X-AWH-Encryption
								false, // Authorization
								false  // Proxy-Authorization
							};
							// Размер тела сообщения
							size_t length = 0;
							// Устанавливаем парарметры запроса
							this->_web.request(req);
							// Устанавливаем параметры HTTP-запроса
							this->_auth.client.uri(this->_uri.url(req.url));
							// Список системных заголовков
							std::unordered_set <string> systemHeaders;
							// Получаем список доступных заголовков
							headers_t & headers = this->_web.headers();
							// Переходим по всему списку заголовков
							for(auto & header : headers){
								// Если заголовок не находится в чёрном списке и не является системным
								bool allow = (!this->isInBlacklist(header.first) && (systemHeaders.find(header.first) == systemHeaders.end()));
								// Выполняем перебор всех обязательных заголовков
								for(uint8_t i = 0; i < 14; i++){
									// Если заголовок уже найден пропускаем его
									if(available[i])
										// Продолжаем поиск дальше
										continue;
									/**
									 * Выполняем првоерку заголовка
									 */
									switch(i){
										case 0:  available[i] = this->_fmk->compare("te", header.first);                  break;
										case 1:  available[i] = this->_fmk->compare("host", header.first);                break;
										case 2:  available[i] = this->_fmk->compare("accept", header.first);              break;
										case 3:  available[i] = this->_fmk->compare("origin", header.first);              break;
										case 4:  available[i] = this->_fmk->compare("user-agent", header.first);          break;
										case 5:  available[i] = this->_fmk->compare("connection", header.first);          break;
										case 6:  available[i] = this->_fmk->compare("proxy-connection", header.first);    break;
										case 8:  available[i] = this->_fmk->compare("accept-language", header.first);     break;
										case 9:  available[i] = this->_fmk->compare("accept-encoding", header.first);     break;
										case 10: available[i] = this->_fmk->compare("content-encoding", header.first);    break;
										case 11: available[i] = this->_fmk->compare("transfer-encoding", header.first);   break;
										case 12: available[i] = this->_fmk->compare("x-awh-encryption", header.first);    break;
										case 13: available[i] = this->_fmk->compare("authorization", header.first);       break;
										case 14: available[i] = this->_fmk->compare("proxy-authorization", header.first); break;
										case 7: {
											// Запоминаем, что мы нашли заголовок размера тела
											available[i] = this->_fmk->compare("content-length", header.first);
											// Устанавливаем размер тела сообщения
											if(available[i]){
												/**
												 * Выполняем отлов ошибок
												 */
												try {
													// Устанавливаем длину передаваемого текста
													length = static_cast <size_t> (::stoull(header.second));
												/**
												 * Если возникает ошибка
												 */
												} catch(const exception &) {
													// Устанавливаем длину передаваемого текста
													length = 0;
												}
											}
										} break;
									}
									// Если заголовок разрешён для вывода
									if(allow){
										/**
										 * Выполняем првоерку заголовка
										 */
										switch(i){
											case 0:
											case 5:
											case 6:
											case 7:
											case 10:
											case 11:
											case 12: allow = !available[i]; break;
											case 1: allow = (req.method != web_t::method_t::CONNECT); break;
										}
										// Если заголовок запрещён к выводу
										if(!allow)
											// Добавляем заголовко в список системных
											systemHeaders.emplace(header.first);
									}
								}
								// Если заголовок не является запрещённым, добавляем заголовок в запрос
								if(allow){
									// Получаем название заголовка
									string name = header.first;
									// Переводим заголовок в нормальный режим
									this->_fmk->transform(name, fmk_t::transform_t::SMART);
									// Формируем строку запроса
									result.push(this->_fmk->format("%s: %s\r\n", name.c_str(), header.second.c_str()));
								}
							}
							// Устанавливаем Host если не передан и метод подключения не является CONNECT
							if(!available[1] && !this->isInBlacklist("Host") && (req.method != web_t::method_t::CONNECT)){
								// Если флаг точной установки хоста не установлен
								if(!this->_session.exactHost)
									// Добавляем заголовок в запрос
									result.push(this->_fmk->format("Host: %s\r\n", req.url.host.c_str()));
								// Добавляем заголовок в запрос
								else result.push(this->_fmk->format("Host: %s:%u\r\n", req.url.host.c_str(), req.url.port));
							}
							// Устанавливаем Accept если не передан
							if(!available[2] && (req.method != web_t::method_t::CONNECT) && !this->isInBlacklist("Accept"))
								// Добавляем заголовок в запрос
								result.push(this->_fmk->format("Accept: %s\r\n", HTTP_HEADER_ACCEPT));
							// Устанавливаем Connection если не передан
							if(!available[5] && !this->isInBlacklist("Connection")){
								// Если нужно вставить заголовок TE и он не находится в чёрном списке
								if(available[0] && !this->isInBlacklist("TE"))
									// Добавляем заголовок в запрос
									result.push(this->_fmk->format("Connection: TE, %s\r\n", HTTP_HEADER_CONNECTION));
								// Добавляем заголовок в запрос
								else result.push(this->_fmk->format("Connection: %s\r\n", HTTP_HEADER_CONNECTION));
							// Если заголовок Connection уже передан и не находится в чёрном списке
							} else if(!this->isInBlacklist("Connection")) {
								// Поулчаем заголовок Connection
								const string & header = headers["Connection"];
								// Если нужно вставить заголовок TE и он не находится в чёрном списке
								if(available[0] && !this->isInBlacklist("TE") && !this->_fmk->exists("TE", header))
									// Добавляем заголовок в запрос
									result.push(this->_fmk->format("Connection: TE, %s\r\n", header.c_str()));
								// Добавляем заголовок в запрос
								else result.push(this->_fmk->format("Connection: %s\r\n", header.c_str()));
							}
							// Устанавливаем Proxy-Connection если не передан
							if(!available[6] && !this->isInBlacklist("Proxy-Connection")){
								// Если сервер соответствует PROXY-серверу
								if(this->_session.identity == identity_t::PROXY)
									// Добавляем заголовок в запрос
									result.push(this->_fmk->format("Proxy-Connection: %s\r\n", HTTP_HEADER_CONNECTION));
							// Если заголовок Proxy-Connection уже передан и не находится в чёрном списке
							} else if(!this->isInBlacklist("Proxy-Connection"))
								// Добавляем заголовок в запрос
								result.push(this->_fmk->format("Proxy-Connection: %s\r\n", headers["Proxy-Connection"].c_str()));
							// Устанавливаем Accept-Language если не передан
							if(!available[8] && (req.method != web_t::method_t::CONNECT) && !this->isInBlacklist("Accept-Language"))
								// Добавляем заголовок в запрос
								result.push(this->_fmk->format("Accept-Language: %s\r\n", HTTP_HEADER_ACCEPTLANGUAGE));
							// Если нужно запросить компрессию в удобном нам виде
							if(!available[9] &&
							  (req.method != web_t::method_t::CONNECT) &&
							  (!this->_compressors.supports.empty() ||
							  (this->_compressors.selected != compressor_t::NONE)) &&
							  !this->isInBlacklist("Accept-Encoding")){
								// Если компрессор уже выбран
								if(this->_compressors.selected != compressor_t::NONE){
									/**
									 * Определяем метод сжатия который поддерживает клиент
									 */
									switch(static_cast <uint8_t> (this->_compressors.selected)){
										// Если клиент поддерживает методот сжатия LZ4
										case static_cast <uint8_t> (compressor_t::LZ4):
											// Добавляем заголовок в запрос
											result.push(this->_fmk->format("Accept-Encoding: %s\r\n", "lz4"));
										break;
										// Если клиент поддерживает методот сжатия Zstandard
										case static_cast <uint8_t> (compressor_t::ZSTD):
											// Добавляем заголовок в запрос
											result.push(this->_fmk->format("Accept-Encoding: %s\r\n", "zstd"));
										break;
										// Если клиент поддерживает методот сжатия LZma
										case static_cast <uint8_t> (compressor_t::LZMA):
											// Добавляем заголовок в запрос
											result.push(this->_fmk->format("Accept-Encoding: %s\r\n", "xz"));
										break;
										// Если клиент поддерживает методот сжатия Brotli
										case static_cast <uint8_t> (compressor_t::BROTLI):
											// Добавляем заголовок в запрос
											result.push(this->_fmk->format("Accept-Encoding: %s\r\n", "br"));
										break;
										// Если клиент поддерживает методот сжатия BZip2
										case static_cast <uint8_t> (compressor_t::BZIP2):
											// Добавляем заголовок в запрос
											result.push(this->_fmk->format("Accept-Encoding: %s\r\n", "bzip2"));
										break;
										// Если клиент поддерживает методот сжатия GZip
										case static_cast <uint8_t> (compressor_t::GZIP):
											// Добавляем заголовок в запрос
											result.push(this->_fmk->format("Accept-Encoding: %s\r\n", "gzip"));
										break;
										// Если клиент поддерживает методот сжатия Deflate
										case static_cast <uint8_t> (compressor_t::DEFLATE):
											// Добавляем заголовок в запрос
											result.push(this->_fmk->format("Accept-Encoding: %s\r\n", "deflate"));
										break;
									}
								// Если список компрессоров установлен
								} else if(!this->_compressors.supports.empty()) {
									// Строка со списком компрессоров
									string compressors = "";
									// Выполняем перебор всего списка компрессоров
									for(auto i = this->_compressors.supports.rbegin(); i != this->_compressors.supports.rend(); ++i){
										// Если список компрессоров уже не пустой
										if(!compressors.empty())
											// Выполняем добавление разделителя
											compressors.append(", ");
										/**
										 * Определяем метод сжатия который поддерживает клиент
										 */
										switch(static_cast <uint8_t> (i->second)){
											// Если клиент поддерживает методот сжатия LZ4
											case static_cast <uint8_t> (compressor_t::LZ4):
												// Добавляем компрессор в список
												compressors.append("lz4");
											break;
											// Если клиент поддерживает методот сжатия Zstandard
											case static_cast <uint8_t> (compressor_t::ZSTD):
												// Добавляем компрессор в список
												compressors.append("zstd");
											break;
											// Если клиент поддерживает методот сжатия LZma
											case static_cast <uint8_t> (compressor_t::LZMA):
												// Добавляем компрессор в список
												compressors.append("xz");
											break;
											// Если клиент поддерживает методот сжатия Brotli
											case static_cast <uint8_t> (compressor_t::BROTLI):
												// Добавляем компрессор в список
												compressors.append("br");
											break;
											// Если клиент поддерживает методот сжатия BZip2
											case static_cast <uint8_t> (compressor_t::BZIP2):
												// Добавляем компрессор в список
												compressors.append("bzip2");
											break;
											// Если клиент поддерживает методот сжатия GZip
											case static_cast <uint8_t> (compressor_t::GZIP):
												// Добавляем компрессор в список
												compressors.append("gzip");
											break;
											// Если клиент поддерживает методот сжатия Deflate
											case static_cast <uint8_t> (compressor_t::DEFLATE):
												// Добавляем компрессор в список
												compressors.append("deflate");
											break;
										}
									}
									// Если список компрессоров получен
									if(!compressors.empty())
										// Добавляем заголовок в запрос
										result.push(this->_fmk->format("Accept-Encoding: %s\r\n", compressors.c_str()));
								}
							}
							// Устанавливаем User-Agent если не передан
							if(!available[4] && !this->isInBlacklist("User-Agent")){
								// Если User-Agent установлен стандартный
								if(this->_fmk->compare(this->_agent.user, HTTP_HEADER_AGENT))
									// Выполняем генерацию Юзер-агента клиента выполняющего HTTP-запрос
									this->_agent.user = this->agent(flag);
								// Добавляем заголовок в запрос
								result.push(this->_fmk->format("User-Agent: %s\r\n", this->_agent.user.c_str()));
							}
							// Если заголовок авторизации не передан
							if(!available[13] && (this->_session.identity != identity_t::PROXY)){
								// Метод HTTP-запроса
								string method = "";
								/**
								 * Определяем метод запроса
								 */
								switch(static_cast <uint8_t> (req.method)){
									// Если метод запроса указан как GET
									case static_cast <uint8_t> (web_t::method_t::GET):
										// Устанавливаем метод HTTP-протокола
										method = "get";
									break;
									// Если метод запроса указан как PUT
									case static_cast <uint8_t> (web_t::method_t::PUT):
										// Устанавливаем метод HTTP-протокола
										method = "put";
									break;
									// Если метод запроса указан как POST
									case static_cast <uint8_t> (web_t::method_t::POST):
										// Устанавливаем метод HTTP-протокола
										method = "post";
									break;
									// Если метод запроса указан как HEAD
									case static_cast <uint8_t> (web_t::method_t::HEAD):
										// Устанавливаем метод HTTP-протокола
										method = "head";
									break;
									// Если метод запроса указан как DELETE
									case static_cast <uint8_t> (web_t::method_t::DEL):
										// Устанавливаем метод HTTP-протокола
										method = "delete";
									break;
									// Если метод запроса указан как PATCH
									case static_cast <uint8_t> (web_t::method_t::PATCH):
										// Устанавливаем метод HTTP-протокола
										method = "patch";
									break;
									// Если метод запроса указан как TRACE
									case static_cast <uint8_t> (web_t::method_t::TRACE):
										// Устанавливаем метод HTTP-протокола
										method = "trace";
									break;
									// Если метод запроса указан как OPTIONS
									case static_cast <uint8_t> (web_t::method_t::OPTIONS):
										// Устанавливаем метод HTTP-протокола
										method = "options";
									break;
									// Если метод запроса указан как CONNECT
									case static_cast <uint8_t> (web_t::method_t::CONNECT):
										// Устанавливаем метод HTTP-протокола
										method = "connect";
									break;
								}
								// Если заголовок авторизации на прокси-сервере не запрещён
								if(!this->isInBlacklist("Authorization")){
									// Получаем параметры авторизации
									const string & auth = this->_auth.client.auth(method);
									// Если данные авторизации получены
									if(!auth.empty())
										// Выполняем установку параметров авторизации
										result.push(this->_fmk->format("Authorization: %s\r\n", auth.c_str()));
								}
							}
							// Если заголовок авторизации на прокси-сервере не передан
							if(!available[14] && (this->_session.identity == identity_t::PROXY)){
								// Метод HTTP-запроса
								string method = "";
								/**
								 * Определяем метод запроса
								 */
								switch(static_cast <uint8_t> (req.method)){
									// Если метод запроса указан как GET
									case static_cast <uint8_t> (web_t::method_t::GET):
										// Устанавливаем метод HTTP-протокола
										method = "get";
									break;
									// Если метод запроса указан как PUT
									case static_cast <uint8_t> (web_t::method_t::PUT):
										// Устанавливаем метод HTTP-протокола
										method = "put";
									break;
									// Если метод запроса указан как POST
									case static_cast <uint8_t> (web_t::method_t::POST):
										// Устанавливаем метод HTTP-протокола
										method = "post";
									break;
									// Если метод запроса указан как HEAD
									case static_cast <uint8_t> (web_t::method_t::HEAD):
										// Устанавливаем метод HTTP-протокола
										method = "head";
									break;
									// Если метод запроса указан как DELETE
									case static_cast <uint8_t> (web_t::method_t::DEL):
										// Устанавливаем метод HTTP-протокола
										method = "delete";
									break;
									// Если метод запроса указан как PATCH
									case static_cast <uint8_t> (web_t::method_t::PATCH):
										// Устанавливаем метод HTTP-протокола
										method = "patch";
									break;
									// Если метод запроса указан как TRACE
									case static_cast <uint8_t> (web_t::method_t::TRACE):
										// Устанавливаем метод HTTP-протокола
										method = "trace";
									break;
									// Если метод запроса указан как OPTIONS
									case static_cast <uint8_t> (web_t::method_t::OPTIONS):
										// Устанавливаем метод HTTP-протокола
										method = "options";
									break;
									// Если метод запроса указан как CONNECT
									case static_cast <uint8_t> (web_t::method_t::CONNECT):
										// Устанавливаем метод HTTP-протокола
										method = "connect";
									break;
								}
								// Если заголовок авторизации на прокси-сервере не запрещён
								if(!this->isInBlacklist("Proxy-Authorization")){
									// Получаем параметры авторизации
									const string & auth = this->_auth.client.auth(method);
									// Если данные авторизации получены
									if(!auth.empty())
										// Выполняем установку параметров авторизации
										result.push(this->_fmk->format("Proxy-Authorization: %s\r\n", auth.c_str()));
								}
							}
							// Если нужно вставить заголовок TE и он не находится в чёрном списке
							if(available[0] && !this->isInBlacklist("TE")){
								// Если список заголовков получен
								if(!headers.empty()){
									// Строка отправляемого заголовка
									string header = "";
									// Выполняем перебор всего списка указанных заголовков
									for(auto & item : headers.range("te")){
										// Если заголовок уже собран
										if(!header.empty())
											// Добавляем разделитель
											header.append(", ");
										// Добавляем заголовок в список
										header.append(this->_fmk->transform(item, fmk_t::transform_t::LOWER));
									}
									// Если заголовок собран
									if(!header.empty())
										// Добавляем заголовок параметров Transfer-Encoding в запрос
										result.push(this->_fmk->format("TE: %s\r\n", header.c_str()));
								}
							}
							// Если запрос является PUT, POST, PATCH
							if((req.method == web_t::method_t::PUT) || (req.method == web_t::method_t::POST) || (req.method == web_t::method_t::PATCH)){
								// Получаем тело HTTP-протокола
								buffer_t & body = this->_web.body();
								// Если заголовок не запрещён
								if(!this->isInBlacklist("Date"))
									// Добавляем заголовок даты в запрос
									result.push(this->_fmk->format("Date: %s\r\n", this->date().c_str()));
								// Если тело запроса существует
								if(!body.empty()){
									// Выполняем компрессию полезной нагрузки
									this->compress();
									// Выполняем шифрование полезной нагрузки
									this->encrypt();
									// Проверяем нужно ли передать тело разбив на чанки
									this->_transfer.chunking = (
										this->_encrypt.crypted ||
										(this->_compressors.current != compressor_t::NONE)
									);
									// Заменяем размер тела данных
									if(!this->_transfer.chunking)
										// Устанавливаем размер тела сообщения
										length = body.size();
									// Если данные зашифрованы, устанавливаем соответствующие заголовки
									if(this->_encrypt.crypted)
										// Устанавливаем X-AWH-Encryption
										result.push(this->_fmk->format("X-AWH-Encryption: %u\r\n", static_cast <uint16_t> (this->_encrypt.cipher)));
									/**
									 * Определяем метод компрессии полезной нагрузки
									 */
									switch(static_cast <uint8_t> (this->_compressors.current)){
										// Если нужно сжать тело методом LZ4
										case static_cast <uint8_t> (compressor_t::LZ4):
											// Устанавливаем Content-Encoding если не передан
											result.push(this->_fmk->format("Content-Encoding: %s\r\n", "lz4"));
										break;
										// Если нужно сжать тело методом Zstandard
										case static_cast <uint8_t> (compressor_t::ZSTD):
											// Устанавливаем Content-Encoding если не передан
											result.push(this->_fmk->format("Content-Encoding: %s\r\n", "zstd"));
										break;
										// Если нужно сжать тело методом LZma
										case static_cast <uint8_t> (compressor_t::LZMA):
											// Устанавливаем Content-Encoding если не передан
											result.push(this->_fmk->format("Content-Encoding: %s\r\n", "xz"));
										break;
										// Если нужно сжать тело методом Brotli
										case static_cast <uint8_t> (compressor_t::BROTLI):
											// Устанавливаем Content-Encoding если не передан
											result.push(this->_fmk->format("Content-Encoding: %s\r\n", "br"));
										break;
										// Если нужно сжать тело методом BZip2
										case static_cast <uint8_t> (compressor_t::BZIP2):
											// Устанавливаем Content-Encoding если не передан
											result.push(this->_fmk->format("Content-Encoding: %s\r\n", "bzip2"));
										break;
										// Если нужно сжать тело методом GZip
										case static_cast <uint8_t> (compressor_t::GZIP):
											// Устанавливаем Content-Encoding если не передан
											result.push(this->_fmk->format("Content-Encoding: %s\r\n", "gzip"));
										break;
										// Если нужно сжать тело методом Deflate
										case static_cast <uint8_t> (compressor_t::DEFLATE):
											// Устанавливаем Content-Encoding если не передан
											result.push(this->_fmk->format("Content-Encoding: %s\r\n", "deflate"));
										break;
									}
									// Если данные необходимо разбивать на чанки
									if(this->_transfer.chunking && !this->isInBlacklist("Transfer-Encoding"))
										// Устанавливаем заголовок Transfer-Encoding
										result.push(this->_fmk->format("Transfer-Encoding: %s\r\n", "chunked"));
									// Если заголовок размера передаваемого тела, не запрещён
									else if(!this->isInBlacklist("Content-Length") && ((length > 0) || headers.has("Content-Length")))
										// Устанавливаем размер передаваемого тела Content-Length
										result.push(this->_fmk->format("Content-Length: %zu\r\n", length));
								// Если тело запроса не существует
								} else {
									// Проверяем нужно ли передать тело разбив на чанки
									this->_transfer.chunking = (
										this->_encrypt.enabled ||
										(this->_compressors.selected != compressor_t::NONE)
									);
									// Если данные зашифрованы, устанавливаем соответствующие заголовки
									if(this->_encrypt.enabled && !this->isInBlacklist("X-AWH-Encryption"))
										// Устанавливаем X-AWH-Encryption
										result.push(this->_fmk->format("X-AWH-Encryption: %u\r\n", static_cast <uint16_t> (this->_encrypt.cipher)));
									// Устанавливаем Content-Encoding если не передан
									if(!this->isInBlacklist("Content-Encoding")){
										/**
										 * Определяем метод компрессии полезной нагрузки
										 */
										switch(static_cast <uint8_t> (this->_compressors.selected)){
											// Если полезная нагрузка сжата методом LZ4
											case static_cast <uint8_t> (compressor_t::LZ4):
												// Устанавливаем Content-Encoding если не передан
												result.push(this->_fmk->format("Content-Encoding: %s\r\n", "lz4"));
											break;
											// Если полезная нагрузка сжата методом Zstandard
											case static_cast <uint8_t> (compressor_t::ZSTD):
												// Устанавливаем Content-Encoding если не передан
												result.push(this->_fmk->format("Content-Encoding: %s\r\n", "zstd"));
											break;
											// Если полезная нагрузка сжата методом LZma
											case static_cast <uint8_t> (compressor_t::LZMA):
												// Устанавливаем Content-Encoding если не передан
												result.push(this->_fmk->format("Content-Encoding: %s\r\n", "xz"));
											break;
											// Если полезная нагрузка сжата методом Brotli
											case static_cast <uint8_t> (compressor_t::BROTLI):
												// Устанавливаем Content-Encoding если не передан
												result.push(this->_fmk->format("Content-Encoding: %s\r\n", "br"));
											break;
											// Если полезная нагрузка сжата методом BZip2
											case static_cast <uint8_t> (compressor_t::BZIP2):
												// Устанавливаем Content-Encoding если не передан
												result.push(this->_fmk->format("Content-Encoding: %s\r\n", "bzip2"));
											break;
											// Если полезная нагрузка сжата методом GZip
											case static_cast <uint8_t> (compressor_t::GZIP):
												// Устанавливаем Content-Encoding если не передан
												result.push(this->_fmk->format("Content-Encoding: %s\r\n", "gzip"));
											break;
											// Если полезная нагрузка сжата методом Deflate
											case static_cast <uint8_t> (compressor_t::DEFLATE):
												// Устанавливаем Content-Encoding если не передан
												result.push(this->_fmk->format("Content-Encoding: %s\r\n", "deflate"));
											break;
										}
									}
									// Если данные необходимо разбивать на чанки
									if(this->_transfer.chunking && !this->isInBlacklist("Transfer-Encoding") && headers.has("Transfer-Encoding"))
										// Устанавливаем заголовок Transfer-Encoding
										result.push(this->_fmk->format("Transfer-Encoding: %s\r\n", "chunked"));
									// Если заголовок размера передаваемого тела, не запрещён
									else if(!this->isInBlacklist("Content-Length") && ((length > 0) || headers.has("Content-Length")))
										// Устанавливаем размер передаваемого тела Content-Length
										result.push(this->_fmk->format("Content-Length: %zu\r\n", length));
								}
							// Если запрос не содержит тела запроса
							} else {
								// Если данные зашифрованы, устанавливаем соответствующие заголовки
								if((this->_transfer.chunking = (this->_encrypt.enabled && !this->isInBlacklist("X-AWH-Encryption"))))
									// Устанавливаем X-AWH-Encryption
									result.push(this->_fmk->format("X-AWH-Encryption: %u\r\n", static_cast <uint16_t> (this->_encrypt.cipher)));
								// Устанавливаем Content-Encoding если заголовок есть в запросе
								if(available[10] && !this->isInBlacklist("Content-Encoding")){
									/**
									 * Определяем метод компрессии полезной нагрузки
									 */
									switch(static_cast <uint8_t> (this->_compressors.selected)){
										// Если полезная нагрузка сжата методом LZ4
										case static_cast <uint8_t> (compressor_t::LZ4):
											// Устанавливаем Content-Encoding если не передан
											result.push(this->_fmk->format("Content-Encoding: %s\r\n", "lz4"));
										break;
										// Если полезная нагрузка сжата методом Zstandard
										case static_cast <uint8_t> (compressor_t::ZSTD):
											// Устанавливаем Content-Encoding если не передан
											result.push(this->_fmk->format("Content-Encoding: %s\r\n", "zstd"));
										break;
										// Если полезная нагрузка сжата методом LZma
										case static_cast <uint8_t> (compressor_t::LZMA):
											// Устанавливаем Content-Encoding если не передан
											result.push(this->_fmk->format("Content-Encoding: %s\r\n", "xz"));
										break;
										// Если полезная нагрузка сжата методом Brotli
										case static_cast <uint8_t> (compressor_t::BROTLI):
											// Устанавливаем Content-Encoding если не передан
											result.push(this->_fmk->format("Content-Encoding: %s\r\n", "br"));
										break;
										// Если полезная нагрузка сжата методом BZip2
										case static_cast <uint8_t> (compressor_t::BZIP2):
											// Устанавливаем Content-Encoding если не передан
											result.push(this->_fmk->format("Content-Encoding: %s\r\n", "bzip2"));
										break;
										// Если полезная нагрузка сжата методом GZip
										case static_cast <uint8_t> (compressor_t::GZIP):
											// Устанавливаем Content-Encoding если не передан
											result.push(this->_fmk->format("Content-Encoding: %s\r\n", "gzip"));
										break;
										// Если полезная нагрузка сжата методом Deflate
										case static_cast <uint8_t> (compressor_t::DEFLATE):
											// Устанавливаем Content-Encoding если не передан
											result.push(this->_fmk->format("Content-Encoding: %s\r\n", "deflate"));
										break;
									}
									// Проверяем нужно ли передать тело разбив на чанки
									this->_transfer.chunking = (this->_compressors.selected != compressor_t::NONE);
								}
								// Очищаем тела сообщения
								this->clear(web_t::unit_t::BODY);
							}
						} break;
						// Если мы работаем с сервером
						case static_cast <uint8_t> (web_t::hid_t::SERVER): {
							// Название заголовка
							string name = "";
							// Переходим по всему списку заголовков
							for(auto & header : this->_web.headers()){
								// Если метод не является CONNECT или заголовок Host не установлен
								if((req.method != web_t::method_t::CONNECT) || !this->_fmk->compare("host", header.first)){
									// Устанавливаем название заголовка
									name = header.first;
									// Переводим заголовок в нормальный режим
									this->_fmk->transform(name, fmk_t::transform_t::SMART);
									// Формируем строку запроса
									result.push(this->_fmk->format("%s: %s\r\n", name.c_str(), header.second.c_str()));
								}
							}
						} break;
					}
					// Устанавливаем завершающий разделитель
					result.push("\r\n");
				}
			} break;
			// Если нужно сформировать данные ответа
			case static_cast <uint8_t> (process_t::RESPONSE): {
				// Получаем объект ответа клиенту
				const web_t::res_t & res = static_cast <const web_t::res_t &> (prov);
				// Если текст сообщения не установлен
				if(res.message.empty())
					// Выполняем установку сообщения
					const_cast <web_t::res_t &> (res).message = this->message(res.code);
				// Если сообщение получено
				if(!res.message.empty()){
					// Данные HTTP-ответа
					result.push(this->_fmk->format("HTTP/%s %u %s\r\n", this->_fmk->noexp(res.version, true).c_str(), res.code, res.message.c_str()));
					/**
					 * Определяем тип HTTP-модуля
					 */
					switch(static_cast <uint8_t> (this->_web.hid())){
						// Если мы работаем с клиентом
						case static_cast <uint8_t> (web_t::hid_t::CLIENT): {
							// Название заголовка
							string name = "";
							// Переходим по всему списку заголовков
							for(auto & header : this->_web.headers()){
								// Устанавливаем название заголовка
								name = header.first;
								// Переводим заголовок в нормальный режим
								this->_fmk->transform(name, fmk_t::transform_t::SMART);
								// Формируем строку ответа
								result.push(this->_fmk->format("%s: %s\r\n", name.c_str(), header.second.c_str()));
							}
						} break;
						// Если мы работаем с сервером
						case static_cast <uint8_t> (web_t::hid_t::SERVER): {
							/**
							 * Типы основных заголовков
							 */
							bool available[12] = {
								false, // Date
								false, // Server
								false, // Connection
								false, // Proxy-Connection
								false, // X-Powered-By
								false, // Content-Type
								false, // Content-Length
								false, // Content-Encoding
								false, // Transfer-Encoding
								false, // X-AWH-Encryption
								false, // WWW-Authenticate
								false  // Proxy-Authenticate
							};
							// Размер тела сообщения
							size_t length = 0;
							// Устанавливаем парарметры ответа
							this->_web.response(res);
							// Список системных заголовков
							std::unordered_set <string> systemHeaders;
							// Получаем список доступных заголовков
							headers_t & headers = this->_web.headers();
							// Переходим по всему списку заголовков
							for(auto & header : headers){
								// Если заголовок не находится в чёрном списке и не является системным
								bool allow = (!this->isInBlacklist(header.first) && (systemHeaders.find(header.first) == systemHeaders.end()));
								// Выполняем перебор всех обязательных заголовков
								for(uint8_t i = 0; i < 12; i++){
									// Если заголовок уже найден пропускаем его
									if(available[i])
										// Продолжаем поиск дальше
										continue;
									/**
									 * Выполняем првоерку заголовка
									 */
									switch(i){
										case 0:  available[i] = this->_fmk->compare("date", header.first);               break;
										case 1:  available[i] = this->_fmk->compare("server", header.first);             break;
										case 2:  available[i] = this->_fmk->compare("connection", header.first);         break;
										case 3:  available[i] = this->_fmk->compare("proxy-connection", header.first);   break;
										case 4:  available[i] = this->_fmk->compare("x-powered-by", header.first);       break;
										case 5:  available[i] = this->_fmk->compare("content-type", header.first);       break;
										case 7:  available[i] = this->_fmk->compare("content-encoding", header.first);   break;
										case 8:  available[i] = this->_fmk->compare("transfer-encoding", header.first);  break;
										case 9:  available[i] = this->_fmk->compare("x-awh-encryption", header.first);   break;
										case 10: available[i] = this->_fmk->compare("www-authenticate", header.first);   break;
										case 11: available[i] = this->_fmk->compare("proxy-authenticate", header.first); break;
										case 6: {
											// Запоминаем, что мы нашли заголовок размера тела
											available[i] = this->_fmk->compare("content-length", header.first);
											// Устанавливаем размер тела сообщения
											if(available[i]){
												/**
												 * Выполняем отлов ошибок
												 */
												try {
													// Устанавливаем длину передаваемого текста
													length = static_cast <size_t> (::stoull(header.second));
												/**
												 * Если возникает ошибка
												 */
												} catch(const exception &) {
													// Устанавливаем длину передаваемого текста
													length = 0;
												}
											}
										} break;
									}
									// Если заголовок разрешён для вывода
									if(allow){
										/**
										 * Выполняем првоерку заголовка
										 */
										switch(i){
											case 6:
											case 7:
											case 8:
											case 9: allow = !available[i]; break;
										}
										// Если ответ является информационным
										if((((res.code >= 100) && (res.code < 200)) || (res.code == 204)) && available[i]){
											/**
											 * Запрещяем указанным заголовкам формирование
											 */
											switch(i){
												case 0:
												case 5:
												case 6:
												case 7:
												case 8:
												case 9:
												case 10:
												case 11: allow = false; break;
											}
										}
										// Если заголовок запрещён к выводу
										if(!allow)
											// Добавляем заголовко в список системных
											systemHeaders.emplace(header.first);
									}
								}
								// Если заголовок не является запрещённым, добавляем заголовок в ответ
								if(allow){
									// Получаем название заголовка
									string name = header.first;
									// Переводим заголовок в нормальный режим
									this->_fmk->transform(name, fmk_t::transform_t::SMART);
									// Формируем строку ответа
									result.push(this->_fmk->format("%s: %s\r\n", name.c_str(), header.second.c_str()));
								}
							}
							// Если заголовок не запрещён
							if(!available[1] && !this->isInBlacklist("Server"))
								// Добавляем название сервера в ответ
								result.push(this->_fmk->format("Server: %s\r\n", this->_agent.name.c_str()));
							// Устанавливаем Connection если не передан
							if(!available[2] && !this->isInBlacklist("Connection"))
								// Добавляем заголовок в ответ
								result.push(this->_fmk->format("Connection: %s\r\n", HTTP_HEADER_CONNECTION));
							// Устанавливаем Proxy-Connection если не передан
							if(!available[3] && !this->isInBlacklist("Proxy-Connection")){
								// Если клиент соответствует PROXY-клиенту
								if(this->_session.identity == identity_t::PROXY)
									// Добавляем заголовок в ответ
									result.push(this->_fmk->format("Proxy-Connection: %s\r\n", HTTP_HEADER_CONNECTION));
							}
							// Если заголовок не запрещён
							if(!available[4] && !this->isInBlacklist("X-Powered-By"))
								// Добавляем название рабочей системы в ответ
								result.push(this->_fmk->format("X-Powered-By: %s\r\n", this->agent(flag).c_str()));
							// Если заголовок авторизации не передан
							if(((res.code == 401) && !available[10]) || ((res.code == 407) && !available[11])){
								// Получаем параметры авторизации
								const string & auth = this->_auth.server;
								// Если параметры авторизации получены
								if(!auth.empty()){
									/**
									 * Определяем код авторизации
									 */
									switch(res.code){
										// Если авторизация производится для Web-Сервера
										case 401: {
											// Если заголовок не запрещён
											if(!this->isInBlacklist("WWW-Authenticate"))
												// Добавляем параметры авторизации
												result.push(this->_fmk->format("WWW-Authenticate: %s\r\n", auth.c_str()));
										} break;
										// Если авторизация производится для Прокси-Сервера
										case 407: {
											// Если заголовок не запрещён
											if(!this->isInBlacklist("Proxy-Authenticate"))
												// Добавляем параметры авторизации
												result.push(this->_fmk->format("Proxy-Authenticate: %s\r\n", auth.c_str()));
										} break;
									}
								}
							}
							// Если сервер соответствует Websocket-серверу
							if(this->_session.identity == identity_t::WS){
								// Если заголовок не запрещён
								if(!available[0] && !this->isInBlacklist("Date")){
									// Запоминаем, что заголовок даты уже указан
									available[0] = !available[0];
									// Добавляем заголовок даты в ответ
									result.push(this->_fmk->format("Date: %s\r\n", this->date().c_str()));
								}
							}
							// Если запрос должен содержать тело и тело ответа существует
							if((res.code >= 200) && (res.code != 204) && (res.code != 304) && (res.code != 308)){
								// Устанавливаем Content-Type если не передан
								if(!available[5] && ((this->_session.identity == identity_t::HTTP) || (res.code >= 400)) && !this->isInBlacklist("Content-Type"))
									// Добавляем заголовок в ответ
									result.push(this->_fmk->format("Content-Type: %s\r\n", HTTP_HEADER_CONTENTTYPE));
								// Извлекаем тело HTTP-протокола
								buffer_t body = this->_web.body();
								// Если тело запроса существует
								if(!body.empty()){
									// Выполняем компрессию полезной нагрузки
									this->compress();
									// Выполняем шифрование полезной нагрузки
									this->encrypt();
									// Проверяем нужно ли передать тело разбив на чанки
									this->_transfer.chunking = (
										this->_encrypt.crypted ||
										this->_transfer.trailers ||
										(this->_compressors.current != compressor_t::NONE)
									);
									// Если заголовок не запрещён
									if(!available[0] && !this->isInBlacklist("Date"))
										// Добавляем заголовок даты в ответ
										result.push(this->_fmk->format("Date: %s\r\n", this->date().c_str()));
									// Заменяем размер тела данных
									if(!this->_transfer.chunking)
										// Устанавливаем размер тела сообщения
										length = body.size();
									// Если данные зашифрованы, устанавливаем соответствующие заголовки
									if(this->_encrypt.crypted)
										// Устанавливаем X-AWH-Encryption
										result.push(this->_fmk->format("X-AWH-Encryption: %u\r\n", static_cast <uint16_t> (this->_encrypt.cipher)));
									{
										// Название компрессора
										string compressor = "";
										/**
										 * Определяем метод компрессии полезной нагрузки
										 */
										switch(static_cast <uint8_t> (this->_compressors.current)){
											// Если полезная нагрузка сжата методом LZ4
											case static_cast <uint8_t> (compressor_t::LZ4):
												// Устанавливаем название компрессора lz4
												compressor = "lz4";
											break;
											// Если полезная нагрузка сжата методом Zstandard
											case static_cast <uint8_t> (compressor_t::ZSTD):
												// Устанавливаем название компрессора zstd
												compressor = "zstd";
											break;
											// Если полезная нагрузка сжата методом LZma
											case static_cast <uint8_t> (compressor_t::LZMA):
												// Устанавливаем название компрессора xz
												compressor = "xz";
											break;
											// Если полезная нагрузка сжата методом Brotli
											case static_cast <uint8_t> (compressor_t::BROTLI):
												// Устанавливаем название компрессора br
												compressor = "br";
											break;
											// Если полезная нагрузка сжата методом BZip2
											case static_cast <uint8_t> (compressor_t::BZIP2):
												// Устанавливаем название компрессора bzip2
												compressor = "bzip2";
											break;
											// Если полезная нагрузка сжата методом GZip
											case static_cast <uint8_t> (compressor_t::GZIP):
												// Устанавливаем название компрессора gzip
												compressor = "gzip";
											break;
											// Если полезная нагрузка сжата методом Deflate
											case static_cast <uint8_t> (compressor_t::DEFLATE):
												// Устанавливаем название компрессора deflate
												compressor = "deflate";
											break;
										}
										// Если компрессор получен
										if(!compressor.empty()){
											// Если активирован режим отправки через Transfer-Encoding
											if(this->_transfer.enabled && !this->isInBlacklist("Transfer-Encoding")){
												// Если активирован режим передачи чанками
												if(this->_transfer.chunking)
													// Устанавливаем Transfer-Encoding если не передан
													result.push(this->_fmk->format("Transfer-Encoding: %s, chunked\r\n", compressor.c_str()));
												// Устанавливаем Transfer-Encoding если не передан
												else result.push(this->_fmk->format("Transfer-Encoding: %s\r\n", compressor.c_str()));
											// Устанавливаем Content-Encoding если не передан
											} else result.push(this->_fmk->format("Content-Encoding: %s\r\n", compressor.c_str()));
										// Если активирован режим передачи чанками
										} else if(this->_transfer.enabled && this->_transfer.chunking && !this->isInBlacklist("Transfer-Encoding"))
											// Устанавливаем заголовок Transfer-Encoding
											result.push(this->_fmk->format("Transfer-Encoding: %s\r\n", "chunked"));
									}
									// Если данные необходимо разбивать на чанки
									if(this->_transfer.chunking && !this->isInBlacklist("Transfer-Encoding")){
										// Если режим отправки шифровани через Transfer-Encoding не активирован
										if(!this->_transfer.enabled)
											// Устанавливаем заголовок Transfer-Encoding
											result.push(this->_fmk->format("Transfer-Encoding: %s\r\n", "chunked"));
									// Если заголовок размера передаваемого тела, не запрещён
									} else if(!this->isInBlacklist("Content-Length") && ((length > 0) || headers.has("Content-Length")))
										// Устанавливаем размер передаваемого тела Content-Length
										result.push(this->_fmk->format("Content-Length: %zu\r\n", length));
								// Если тело запроса не существует
								} else {
									// Проверяем нужно ли передать тело разбив на чанки
									this->_transfer.chunking = (
										this->_encrypt.enabled ||
										this->_transfer.trailers ||
										(this->_compressors.selected != compressor_t::NONE)
									);
									// Если заголовок не запрещён
									if(!available[0] && !this->isInBlacklist("Date"))
										// Добавляем заголовок даты в ответ
										result.push(this->_fmk->format("Date: %s\r\n", this->date().c_str()));
									// Если данные зашифрованы, устанавливаем соответствующие заголовки
									if(this->_encrypt.enabled && !this->isInBlacklist("X-AWH-Encryption"))
										// Устанавливаем X-AWH-Encryption
										result.push(this->_fmk->format("X-AWH-Encryption: %u\r\n", static_cast <uint16_t> (this->_encrypt.cipher)));
									{
										// Название компрессора
										string compressor = "";
										/**
										 * Определяем метод компрессии полезной нагрузки
										 */
										switch(static_cast <uint8_t> (this->_compressors.selected)){
											// Если полезная нагрузка сжата методом LZ4
											case static_cast <uint8_t> (compressor_t::LZ4):
												// Устанавливаем название компрессора lz4
												compressor = "lz4";
											break;
											// Если полезная нагрузка сжата методом Zstandard
											case static_cast <uint8_t> (compressor_t::ZSTD):
												// Устанавливаем название компрессора zstd
												compressor = "zstd";
											break;
											// Если полезная нагрузка сжата методом LZma
											case static_cast <uint8_t> (compressor_t::LZMA):
												// Устанавливаем название компрессора xz
												compressor = "xz";
											break;
											// Если полезная нагрузка сжата методом Brotli
											case static_cast <uint8_t> (compressor_t::BROTLI):
												// Устанавливаем название компрессора br
												compressor = "br";
											break;
											// Если полезная нагрузка сжата методом BZip2
											case static_cast <uint8_t> (compressor_t::BZIP2):
												// Устанавливаем название компрессора bzip2
												compressor = "bzip2";
											break;
											// Если полезная нагрузка сжата методом GZip
											case static_cast <uint8_t> (compressor_t::GZIP):
												// Устанавливаем название компрессора gzip
												compressor = "gzip";
											break;
											// Если полезная нагрузка сжата методом Deflate
											case static_cast <uint8_t> (compressor_t::DEFLATE):
												// Устанавливаем название компрессора deflate
												compressor = "deflate";
											break;
										}
										// Если компрессор получен
										if(!compressor.empty()){
											// Если активирован режим отправки через Transfer-Encoding
											if(this->_transfer.enabled && !this->isInBlacklist("Transfer-Encoding")){
												// Если активирован режим передачи чанками
												if(this->_transfer.chunking)
													// Устанавливаем Transfer-Encoding если не передан
													result.push(this->_fmk->format("Transfer-Encoding: %s, chunked\r\n", compressor.c_str()));
												// Устанавливаем Transfer-Encoding если не передан
												else result.push(this->_fmk->format("Transfer-Encoding: %s\r\n", compressor.c_str()));
											// Если Content-Encoding не запрещён в запросе
											} else if(!this->isInBlacklist("Content-Encoding"))
												// Устанавливаем Content-Encoding если не передан
												result.push(this->_fmk->format("Content-Encoding: %s\r\n", compressor.c_str()));
										// Если активирован режим передачи чанками
										} else if(this->_transfer.enabled && this->_transfer.chunking && available[8] && !this->isInBlacklist("Transfer-Encoding"))
											// Устанавливаем заголовок Transfer-Encoding
											result.push(this->_fmk->format("Transfer-Encoding: %s\r\n", "chunked"));
									}
									// Если данные необходимо разбивать на чанки
									if(this->_transfer.chunking && available[8] && !this->isInBlacklist("Transfer-Encoding")){
										// Если режим отправки шифровани через Transfer-Encoding не активирован
										if(!this->_transfer.enabled)
											// Устанавливаем заголовок Transfer-Encoding
											result.push(this->_fmk->format("Transfer-Encoding: %s\r\n", "chunked"));
									// Если заголовок размера передаваемого тела, не запрещён
									} else if(!this->isInBlacklist("Content-Length") && ((length > 0) || headers.has("Content-Length")))
										// Устанавливаем размер передаваемого тела Content-Length
										result.push(this->_fmk->format("Content-Length: %zu\r\n", length));
								}
							// Очищаем тела сообщения
							} else this->clear(web_t::unit_t::BODY);
						} break;
					}
					// Устанавливаем завершающий разделитель
					result.push("\r\n");
				}
			} break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если функция обратного вызова на на вывод ошибок установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (flag)), log_t::flag_t::WARNING, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::WARNING, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод создания выполняемого процесса в бинарном виде (для протокола HTTP/2)
 *
 * @param flag флаг выполняемого процесса
 * @param prov параметры провайдера обмена сообщениями
 * @return     буфер данных в бинарном виде
 */
vector <std::pair <string, string>> awh::Http::process2(const process_t flag, const web_t::provider_t & prov) noexcept {
	// Результат работы функции
	vector <std::pair <string, string>> result;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем флаг выполняемого процесса
		 */
		switch(static_cast <uint8_t> (flag)){
			// Если нужно сформировать данные запроса
			case static_cast <uint8_t> (process_t::REQUEST): {
				// Получаем объект ответа клиенту
				const web_t::req_t & req = static_cast <const web_t::req_t &> (prov);
				// Если параметры запроса получены
				if(!req.url.empty() && (req.method != web_t::method_t::NONE)){
					/**
					 * Определяем метод запроса
					 */
					switch(static_cast <uint8_t> (req.method)){
						// Если метод запроса указан как GET
						case static_cast <uint8_t> (web_t::method_t::GET):
							// Формируем GET запрос
							result.push_back(std::make_pair(":method", "GET"));
						break;
						// Если метод запроса указан как PUT
						case static_cast <uint8_t> (web_t::method_t::PUT):
							// Формируем PUT запрос
							result.push_back(std::make_pair(":method", "PUT"));
						break;
						// Если метод запроса указан как POST
						case static_cast <uint8_t> (web_t::method_t::POST):
							// Формируем POST запрос
							result.push_back(std::make_pair(":method", "POST"));
						break;
						// Если метод запроса указан как HEAD
						case static_cast <uint8_t> (web_t::method_t::HEAD):
							// Формируем HEAD запрос
							result.push_back(std::make_pair(":method", "HEAD"));
						break;
						// Если метод запроса указан как PATCH
						case static_cast <uint8_t> (web_t::method_t::PATCH):
							// Формируем PATCH запрос
							result.push_back(std::make_pair(":method", "PATCH"));
						break;
						// Если метод запроса указан как TRACE
						case static_cast <uint8_t> (web_t::method_t::TRACE):
							// Формируем TRACE запрос
							result.push_back(std::make_pair(":method", "TRACE"));
						break;
						// Если метод запроса указан как DELETE
						case static_cast <uint8_t> (web_t::method_t::DEL):
							// Формируем DELETE запрос
							result.push_back(std::make_pair(":method", "DELETE"));
						break;
						// Если метод запроса указан как OPTIONS
						case static_cast <uint8_t> (web_t::method_t::OPTIONS):
							// Формируем OPTIONS запрос
							result.push_back(std::make_pair(":method", "OPTIONS"));
						break;
						// Если метод запроса указан как CONNECT
						case static_cast <uint8_t> (web_t::method_t::CONNECT):
							// Формируем CONNECT запрос
							result.push_back(std::make_pair(":method", "CONNECT"));
						break;
					}
					// Выполняем установку схемы протокола
					result.push_back(std::make_pair(":scheme", "https"));
					// Если метод подключения установлен как CONNECT
					if(this->_session.exactHost || (req.method == web_t::method_t::CONNECT))
						// Формируем URI запроса
						result.push_back(std::make_pair(":authority", this->_fmk->format("%s:%u", req.url.host.c_str(), req.url.port)));
					// Если метод подключения не является методом CONNECT, выполняем установку хоста сервера
					else result.push_back(std::make_pair(":authority", req.url.host));
					// Выполняем установку пути запроса
					result.push_back(std::make_pair(":path", this->_uri.query(req.url)));
					// Получаем список доступных заголовков
					headers_t & headers = this->_web.headers();
					// Переходим по всему списку заголовков
					for(auto & header : headers){
						// Если заголовок является системным
						if(header.first.front() == ':')
							// Формируем строку запроса
							result.push_back(std::make_pair(this->_fmk->transform(header.first, fmk_t::transform_t::LOWER), header.second));
					}
					/**
					 * Определяем тип HTTP-модуля
					 */
					switch(static_cast <uint8_t> (this->_web.hid())){
						// Если мы работаем с клиентом
						case static_cast <uint8_t> (web_t::hid_t::CLIENT): {
							/**
							 * Типы основных заголовков
							 */
							bool available[15] = {
								false, // te
								false, // Host
								false, // Accept
								false, // Origin
								false, // User-Agent
								false, // Connection
								false, // Proxy-Connection
								false, // Content-Length
								false, // Accept-Language
								false, // Accept-Encoding
								false, // Content-Encoding
								false, // Transfer-Encoding
								false, // X-AWH-Encryption
								false, // authorization
								false  // Proxy-Authorization
							};
							// Устанавливаем парарметры запроса
							this->_web.request(req);
							// Устанавливаем параметры HTTP-запроса
							this->_auth.client.uri(this->_uri.url(req.url));
							// Список системных заголовков
							std::unordered_set <string> systemHeaders;
							// Переходим по всему списку заголовков
							for(auto & header : headers){
								// Если заголовок не является системным
								if(header.first.front() != ':'){
									// Если заголовок не находится в чёрном списке и не является системным
									bool allow = (!this->isInBlacklist(header.first) && (systemHeaders.find(header.first) == systemHeaders.end()));
									// Выполняем перебор всех обязательных заголовков
									for(uint8_t i = 0; i < 14; i++){
										// Если заголовок уже найден пропускаем его
										if(available[i])
											// Продолжаем поиск дальше
											continue;
										/**
										 * Выполняем првоерку заголовка
										 */
										switch(i){
											case 0:  available[i] = this->_fmk->compare("te", header.first);                  break;
											case 1:  available[i] = this->_fmk->compare("host", header.first);                break;
											case 2:  available[i] = this->_fmk->compare("accept", header.first);              break;
											case 3:  available[i] = this->_fmk->compare("origin", header.first);              break;
											case 4:  available[i] = this->_fmk->compare("user-agent", header.first);          break;
											case 5:  available[i] = this->_fmk->compare("connection", header.first);          break;
											case 6:  available[i] = this->_fmk->compare("proxy-connection", header.first);    break;
											case 7:  available[i] = this->_fmk->compare("content-length", header.first);      break;
											case 8:  available[i] = this->_fmk->compare("accept-language", header.first);     break;
											case 9:  available[i] = this->_fmk->compare("accept-encoding", header.first);     break;
											case 10: available[i] = this->_fmk->compare("content-encoding", header.first);    break;
											case 11: available[i] = this->_fmk->compare("transfer-encoding", header.first);   break;
											case 12: available[i] = this->_fmk->compare("x-awh-encryption", header.first);    break;
											case 13: available[i] = this->_fmk->compare("authorization", header.first);       break;
											case 14: available[i] = this->_fmk->compare("proxy-authorization", header.first); break;
										}
										// Если заголовок разрешён для вывода
										if(allow){
											/**
											 * Выполняем првоерку заголовка
											 */
											switch(i){
												case 0:
												case 1:
												case 5:
												case 6:
												case 7:
												case 10:
												case 11:
												case 12: allow = !available[i]; break;
											}
											// Если заголовок запрещён к выводу
											if(!allow)
												// Добавляем заголовко в список системных
												systemHeaders.emplace(header.first);
										}
									}
									// Если заголовок не является запрещённым, добавляем заголовок в запрос
									if(allow)
										// Формируем строку запроса
										result.push_back(std::make_pair(this->_fmk->transform(header.first, fmk_t::transform_t::LOWER), header.second));
								}
							}
							// Устанавливаем Accept если не передан
							if(!available[2] && (req.method != web_t::method_t::CONNECT) && !this->isInBlacklist("accept"))
								// Добавляем заголовок в запрос
								result.push_back(std::make_pair("accept", HTTP_HEADER_ACCEPT));
							// Устанавливаем Accept-Language если не передан
							if(!available[8] && (req.method != web_t::method_t::CONNECT) && !this->isInBlacklist("accept-language"))
								// Добавляем заголовок в запрос
								result.push_back(std::make_pair("accept-language", HTTP_HEADER_ACCEPTLANGUAGE));
							// Если нужно запросить компрессию в удобном нам виде
							if(!available[9] && (req.method != web_t::method_t::CONNECT) &&
							  (!this->_compressors.supports.empty() ||
							  (this->_compressors.selected != compressor_t::NONE)) && !this->isInBlacklist("accept-encoding")){
								// Если компрессор уже выбран
								if(this->_compressors.selected != compressor_t::NONE){
									/**
									 * Определяем метод сжатия который поддерживает клиент
									 */
									switch(static_cast <uint8_t> (this->_compressors.selected)){
										// Если клиент поддерживает методот сжатия LZ4
										case static_cast <uint8_t> (compressor_t::LZ4):
											// Добавляем заголовок в запрос
											result.push_back(std::make_pair("accept-encoding", "lz4"));
										break;
										// Если клиент поддерживает методот сжатия Zstandard
										case static_cast <uint8_t> (compressor_t::ZSTD):
											// Добавляем заголовок в запрос
											result.push_back(std::make_pair("accept-encoding", "zstd"));
										break;
										// Если клиент поддерживает методот сжатия LZma
										case static_cast <uint8_t> (compressor_t::LZMA):
											// Добавляем заголовок в запрос
											result.push_back(std::make_pair("accept-encoding", "xz"));
										break;
										// Если клиент поддерживает методот сжатия Brotli
										case static_cast <uint8_t> (compressor_t::BROTLI):
											// Добавляем заголовок в запрос
											result.push_back(std::make_pair("accept-encoding", "br"));
										break;
										// Если клиент поддерживает методот сжатия BZip2
										case static_cast <uint8_t> (compressor_t::BZIP2):
											// Добавляем заголовок в запрос
											result.push_back(std::make_pair("accept-encoding", "bzip2"));
										break;
										// Если клиент поддерживает методот сжатия GZip
										case static_cast <uint8_t> (compressor_t::GZIP):
											// Добавляем заголовок в запрос
											result.push_back(std::make_pair("accept-encoding", "gzip"));
										break;
										// Если клиент поддерживает методот сжатия Deflate
										case static_cast <uint8_t> (compressor_t::DEFLATE):
											// Добавляем заголовок в запрос
											result.push_back(std::make_pair("accept-encoding", "deflate"));
										break;
									}
								// Если список компрессоров установлен
								} else if(!this->_compressors.supports.empty()) {
									// Строка со списком компрессоров
									string compressors = "";
									// Выполняем перебор всего списка компрессоров
									for(auto i = this->_compressors.supports.rbegin(); i != this->_compressors.supports.rend(); ++i){
										// Если список компрессоров уже не пустой
										if(!compressors.empty())
											// Выполняем добавление разделителя
											compressors.append(", ");
										/**
										 * Определяем метод сжатия который поддерживает клиент
										 */
										switch(static_cast <uint8_t> (i->second)){
											// Если клиент поддерживает методот сжатия LZ4
											case static_cast <uint8_t> (compressor_t::LZ4):
												// Добавляем компрессор в список
												compressors.append("lz4");
											break;
											// Если клиент поддерживает методот сжатия Zstandard
											case static_cast <uint8_t> (compressor_t::ZSTD):
												// Добавляем компрессор в список
												compressors.append("zstd");
											break;
											// Если клиент поддерживает методот сжатия LZma
											case static_cast <uint8_t> (compressor_t::LZMA):
												// Добавляем компрессор в список
												compressors.append("xz");
											break;
											// Если клиент поддерживает методот сжатия Brotli
											case static_cast <uint8_t> (compressor_t::BROTLI):
												// Добавляем компрессор в список
												compressors.append("br");
											break;
											// Если клиент поддерживает методот сжатия BZip2
											case static_cast <uint8_t> (compressor_t::BZIP2):
												// Добавляем компрессор в список
												compressors.append("bzip2");
											break;
											// Если клиент поддерживает методот сжатия GZip
											case static_cast <uint8_t> (compressor_t::GZIP):
												// Добавляем компрессор в список
												compressors.append("gzip");
											break;
											// Если клиент поддерживает методот сжатия Deflate
											case static_cast <uint8_t> (compressor_t::DEFLATE):
												// Добавляем компрессор в список
												compressors.append("deflate");
											break;
										}
									}
									// Если список компрессоров получен
									if(!compressors.empty())
										// Добавляем заголовок в запрос
										result.push_back(std::make_pair("accept-encoding", compressors));
								}
							}
							// Устанавливаем User-Agent если не передан
							if(!available[4] && !this->isInBlacklist("user-agent")){
								// Если User-Agent установлен стандартный
								if(this->_fmk->compare(this->_agent.user, HTTP_HEADER_AGENT))
									// Выполняем генерацию Юзер-агента клиента выполняющего HTTP-запрос
									this->_agent.user = this->agent(flag);
								// Добавляем заголовок в запрос
								result.push_back(std::make_pair("user-agent", this->_agent.user));
							}
							// Если заголовок авторизации не передан
							if(!available[13] && (this->_session.identity != identity_t::PROXY)){
								// Метод HTTP-запроса
								string method = "";
								/**
								 * Определяем метод запроса
								 */
								switch(static_cast <uint8_t> (req.method)){
									// Если метод запроса указан как GET
									case static_cast <uint8_t> (web_t::method_t::GET):
										// Устанавливаем метод HTTP-протокола
										method = "get";
									break;
									// Если метод запроса указан как PUT
									case static_cast <uint8_t> (web_t::method_t::PUT):
										// Устанавливаем метод HTTP-протокола
										method = "put";
									break;
									// Если метод запроса указан как POST
									case static_cast <uint8_t> (web_t::method_t::POST):
										// Устанавливаем метод HTTP-протокола
										method = "post";
									break;
									// Если метод запроса указан как HEAD
									case static_cast <uint8_t> (web_t::method_t::HEAD):
										// Устанавливаем метод HTTP-протокола
										method = "head";
									break;
									// Если метод запроса указан как DELETE
									case static_cast <uint8_t> (web_t::method_t::DEL):
										// Устанавливаем метод HTTP-протокола
										method = "delete";
									break;
									// Если метод запроса указан как PATCH
									case static_cast <uint8_t> (web_t::method_t::PATCH):
										// Устанавливаем метод HTTP-протокола
										method = "patch";
									break;
									// Если метод запроса указан как TRACE
									case static_cast <uint8_t> (web_t::method_t::TRACE):
										// Устанавливаем метод HTTP-протокола
										method = "trace";
									break;
									// Если метод запроса указан как OPTIONS
									case static_cast <uint8_t> (web_t::method_t::OPTIONS):
										// Устанавливаем метод HTTP-протокола
										method = "options";
									break;
									// Если метод запроса указан как CONNECT
									case static_cast <uint8_t> (web_t::method_t::CONNECT):
										// Устанавливаем метод HTTP-протокола
										method = "connect";
									break;
								}
								// Если заголовок авторизации на прокси-сервере не запрещён
								if(!this->isInBlacklist("authorization")){
									// Получаем параметры авторизации
									const string & auth = this->_auth.client.auth(method);
									// Если данные авторизации получены
									if(!auth.empty())
										// Выполняем установку заголовка
										result.push_back(std::make_pair("authorization", auth));
								}
							}
							// Если заголовок авторизации на прокси-сервере не передан
							if(!available[14] && (this->_session.identity == identity_t::PROXY)){
								// Метод HTTP-запроса
								string method = "";
								/**
								 * Определяем метод запроса
								 */
								switch(static_cast <uint8_t> (req.method)){
									// Если метод запроса указан как GET
									case static_cast <uint8_t> (web_t::method_t::GET):
										// Устанавливаем метод HTTP-протокола
										method = "get";
									break;
									// Если метод запроса указан как PUT
									case static_cast <uint8_t> (web_t::method_t::PUT):
										// Устанавливаем метод HTTP-протокола
										method = "put";
									break;
									// Если метод запроса указан как POST
									case static_cast <uint8_t> (web_t::method_t::POST):
										// Устанавливаем метод HTTP-протокола
										method = "post";
									break;
									// Если метод запроса указан как HEAD
									case static_cast <uint8_t> (web_t::method_t::HEAD):
										// Устанавливаем метод HTTP-протокола
										method = "head";
									break;
									// Если метод запроса указан как DELETE
									case static_cast <uint8_t> (web_t::method_t::DEL):
										// Устанавливаем метод HTTP-протокола
										method = "delete";
									break;
									// Если метод запроса указан как PATCH
									case static_cast <uint8_t> (web_t::method_t::PATCH):
										// Устанавливаем метод HTTP-протокола
										method = "patch";
									break;
									// Если метод запроса указан как TRACE
									case static_cast <uint8_t> (web_t::method_t::TRACE):
										// Устанавливаем метод HTTP-протокола
										method = "trace";
									break;
									// Если метод запроса указан как OPTIONS
									case static_cast <uint8_t> (web_t::method_t::OPTIONS):
										// Устанавливаем метод HTTP-протокола
										method = "options";
									break;
									// Если метод запроса указан как CONNECT
									case static_cast <uint8_t> (web_t::method_t::CONNECT):
										// Устанавливаем метод HTTP-протокола
										method = "connect";
									break;
								}
								// Если заголовок авторизации на прокси-сервере не запрещён
								if(!this->isInBlacklist("proxy-authorization")){
									// Получаем параметры авторизации
									const string & auth = this->_auth.client.auth(method);
									// Если данные авторизации получены
									if(!auth.empty())
										// Выполняем установку заголовка
										result.push_back(std::make_pair("proxy-authorization", auth));
								}
							}
							// Если нужно вставить заголовок TE и он не находится в чёрном списке
							if(available[0] && !this->isInBlacklist("te"))
								// Устанавливаем Transfer-Encoding в запрос
								result.push_back(std::make_pair("te", "trailers"));
							// Если запрос является PUT, POST, PATCH
							if((req.method == web_t::method_t::PUT) || (req.method == web_t::method_t::POST) || (req.method == web_t::method_t::PATCH)){
								// Если заголовок не запрещён
								if(!this->isInBlacklist("date"))
									// Добавляем заголовок даты в запрос
									result.push_back(std::make_pair("date", this->date()));
								// Если тело запроса существует
								if(!this->_web.body().empty()){
									// Выполняем компрессию полезной нагрузки
									this->compress();
									// Выполняем шифрование полезной нагрузки
									this->encrypt();
									// Проверяем нужно ли передать тело разбив на чанки
									this->_transfer.chunking = (this->_encrypt.crypted || (this->_compressors.current != compressor_t::NONE));
									// Если данные зашифрованы, устанавливаем соответствующие заголовки
									if(this->_encrypt.crypted)
										// Устанавливаем X-AWH-Encryption
										result.push_back(std::make_pair("x-awh-encryption", std::to_string(static_cast <uint16_t> (this->_encrypt.cipher))));
									/**
									 * Определяем метод компрессии полезной нагрузки
									 */
									switch(static_cast <uint8_t> (this->_compressors.current)){
										// Если нужно сжать тело методом LZ4
										case static_cast <uint8_t> (compressor_t::LZ4):
											// Устанавливаем Content-Encoding если не передан
											result.push_back(std::make_pair("content-encoding", "lz4"));
										break;
										// Если нужно сжать тело методом Zstandard
										case static_cast <uint8_t> (compressor_t::ZSTD):
											// Устанавливаем Content-Encoding если не передан
											result.push_back(std::make_pair("content-encoding", "zstd"));
										break;
										// Если нужно сжать тело методом LZma
										case static_cast <uint8_t> (compressor_t::LZMA):
											// Устанавливаем Content-Encoding если не передан
											result.push_back(std::make_pair("content-encoding", "xz"));
										break;
										// Если нужно сжать тело методом Brotli
										case static_cast <uint8_t> (compressor_t::BROTLI):
											// Устанавливаем Content-Encoding если не передан
											result.push_back(std::make_pair("content-encoding", "br"));
										break;
										// Если нужно сжать тело методом BZip2
										case static_cast <uint8_t> (compressor_t::BZIP2):
											// Устанавливаем Content-Encoding если не передан
											result.push_back(std::make_pair("content-encoding", "bzip2"));
										break;
										// Если нужно сжать тело методом GZip
										case static_cast <uint8_t> (compressor_t::GZIP):
											// Устанавливаем Content-Encoding если не передан
											result.push_back(std::make_pair("content-encoding", "gzip"));
										break;
										// Если нужно сжать тело методом Deflate
										case static_cast <uint8_t> (compressor_t::DEFLATE):
											// Устанавливаем Content-Encoding если не передан
											result.push_back(std::make_pair("content-encoding", "deflate"));
										break;
									}
								// Если тело запроса не существует
								} else {
									// Проверяем нужно ли передать тело разбив на чанки
									this->_transfer.chunking = (this->_encrypt.enabled || (this->_compressors.selected != compressor_t::NONE));
									// Если данные зашифрованы, устанавливаем соответствующие заголовки
									if(this->_encrypt.enabled && !this->isInBlacklist("x-awh-encryption"))
										// Устанавливаем X-AWH-Encryption
										result.push_back(std::make_pair("x-awh-encryption", std::to_string(static_cast <uint16_t> (this->_encrypt.cipher))));
									// Устанавливаем Content-Encoding если не передан
									if(!this->isInBlacklist("content-encoding")){
										/**
										 * Определяем метод компрессии полезной нагрузки
										 */
										switch(static_cast <uint8_t> (this->_compressors.selected)){
											// Если полезная нагрузка сжата методом LZ4
											case static_cast <uint8_t> (compressor_t::LZ4):
												// Устанавливаем Content-Encoding если не передан
												result.push_back(std::make_pair("content-encoding", "lz4"));
											break;
											// Если полезная нагрузка сжата методом Zstandard
											case static_cast <uint8_t> (compressor_t::ZSTD):
												// Устанавливаем Content-Encoding если не передан
												result.push_back(std::make_pair("content-encoding", "zstd"));
											break;
											// Если полезная нагрузка сжата методом LZma
											case static_cast <uint8_t> (compressor_t::LZMA):
												// Устанавливаем Content-Encoding если не передан
												result.push_back(std::make_pair("content-encoding", "xz"));
											break;
											// Если полезная нагрузка сжата методом Brotli
											case static_cast <uint8_t> (compressor_t::BROTLI):
												// Устанавливаем Content-Encoding если не передан
												result.push_back(std::make_pair("content-encoding", "br"));
											break;
											// Если полезная нагрузка сжата методом BZip2
											case static_cast <uint8_t> (compressor_t::BZIP2):
												// Устанавливаем Content-Encoding если не передан
												result.push_back(std::make_pair("content-encoding", "bzip2"));
											break;
											// Если полезная нагрузка сжата методом GZip
											case static_cast <uint8_t> (compressor_t::GZIP):
												// Устанавливаем Content-Encoding если не передан
												result.push_back(std::make_pair("content-encoding", "gzip"));
											break;
											// Если полезная нагрузка сжата методом Deflate
											case static_cast <uint8_t> (compressor_t::DEFLATE):
												// Устанавливаем Content-Encoding если не передан
												result.push_back(std::make_pair("content-encoding", "deflate"));
											break;
										}
									}
								}
							// Если запрос не содержит тела запроса
							} else {
								// Если данные зашифрованы, устанавливаем соответствующие заголовки
								if((this->_transfer.chunking = (this->_encrypt.enabled && !this->isInBlacklist("x-awh-encryption"))))
									// Устанавливаем X-AWH-Encryption
									result.push_back(std::make_pair("x-awh-encryption", std::to_string(static_cast <uint16_t> (this->_encrypt.cipher))));
								// Устанавливаем Content-Encoding если заголовок есть в запросе
								if(available[10] && !this->isInBlacklist("content-encoding")){
									/**
									 * Определяем метод компрессии полезной нагрузки
									 */
									switch(static_cast <uint8_t> (this->_compressors.selected)){
										// Если полезная нагрузка сжата методом LZ4
										case static_cast <uint8_t> (compressor_t::LZ4):
											// Устанавливаем Content-Encoding если не передан
											result.push_back(std::make_pair("content-encoding", "lz4"));
										break;
										// Если полезная нагрузка сжата методом Zstandard
										case static_cast <uint8_t> (compressor_t::ZSTD):
											// Устанавливаем Content-Encoding если не передан
											result.push_back(std::make_pair("content-encoding", "zstd"));
										break;
										// Если полезная нагрузка сжата методом LZma
										case static_cast <uint8_t> (compressor_t::LZMA):
											// Устанавливаем Content-Encoding если не передан
											result.push_back(std::make_pair("content-encoding", "xz"));
										break;
										// Если полезная нагрузка сжата методом Brotli
										case static_cast <uint8_t> (compressor_t::BROTLI):
											// Устанавливаем Content-Encoding если не передан
											result.push_back(std::make_pair("content-encoding", "br"));
										break;
										// Если полезная нагрузка сжата методом BZip2
										case static_cast <uint8_t> (compressor_t::BZIP2):
											// Устанавливаем Content-Encoding если не передан
											result.push_back(std::make_pair("content-encoding", "bzip2"));
										break;
										// Если полезная нагрузка сжата методом GZip
										case static_cast <uint8_t> (compressor_t::GZIP):
											// Устанавливаем Content-Encoding если не передан
											result.push_back(std::make_pair("content-encoding", "gzip"));
										break;
										// Если полезная нагрузка сжата методом Deflate
										case static_cast <uint8_t> (compressor_t::DEFLATE):
											// Устанавливаем Content-Encoding если не передан
											result.push_back(std::make_pair("content-encoding", "deflate"));
										break;
									}
									// Проверяем нужно ли передать тело разбив на чанки
									this->_transfer.chunking = (this->_compressors.selected != compressor_t::NONE);
								}
								// Очищаем тела сообщения
								this->clear(web_t::unit_t::BODY);
							}
						} break;
						// Если мы работаем с сервером
						case static_cast <uint8_t> (web_t::hid_t::SERVER): {
							// Переходим по всему списку заголовков
							for(auto & header : this->_web.headers()){
								// Если заголовок не является системным
								if(header.first.front() != ':')
									// Формируем строку запроса
									result.push_back(std::make_pair(this->_fmk->transform(header.first, fmk_t::transform_t::LOWER), header.second));
							}
						} break;
					}
				}
			} break;
			// Если нужно сформировать данные ответа
			case static_cast <uint8_t> (process_t::RESPONSE): {
				// Получаем объект ответа клиенту
				const web_t::res_t & res = static_cast <const web_t::res_t &> (prov);
				// Если текст сообщения не установлен
				if(res.message.empty())
					// Выполняем установку сообщения
					const_cast <web_t::res_t &> (res).message = this->message(res.code);
				// Если сообщение получено
				if(!res.message.empty()){
					// Данные HTTP-ответа
					result.push_back(std::make_pair(":status", std::to_string(res.code)));
					/**
					 * Определяем тип HTTP-модуля
					 */
					switch(static_cast <uint8_t> (this->_web.hid())){
						// Если мы работаем с клиентом
						case static_cast <uint8_t> (web_t::hid_t::CLIENT): {
							// Переходим по всему списку заголовков
							for(auto & header : this->_web.headers())
								// Формируем строку ответа
								result.push_back(std::make_pair(this->_fmk->transform(header.first, fmk_t::transform_t::LOWER), header.second));
						} break;
						// Если мы работаем с сервером
						case static_cast <uint8_t> (web_t::hid_t::SERVER): {
							/**
							 * Типы основных заголовков
							 */
							bool available[12] = {
								false, // Date
								false, // Server
								false, // Connection
								false, // Proxy-Connection
								false, // X-Powered-By
								false, // Content-Type
								false, // Content-Length
								false, // Content-Encoding
								false, // Transfer-Encoding
								false, // X-AWH-Encryption
								false, // WWW-Authenticate
								false  // Proxy-Authenticate
							};
							// Устанавливаем параметры ответа
							this->_web.response(res);
							// Список системных заголовков
							std::unordered_set <string> systemHeaders;
							// Получаем список доступных заголовков
							headers_t & headers = this->_web.headers();
							// Переходим по всему списку заголовков
							for(auto & header : headers){
								// Если заголовок не находится в чёрном списке и не является системным
								bool allow = (!this->isInBlacklist(header.first) && (systemHeaders.find(header.first) == systemHeaders.end()));
								// Выполняем перебор всех обязательных заголовков
								for(uint8_t i = 0; i < 12; i++){
									// Если заголовок уже найден пропускаем его
									if(available[i])
										// Продолжаем поиск дальше
										continue;
									/**
									 * Выполняем првоерку заголовка
									 */
									switch(i){
										case 0:  available[i] = this->_fmk->compare("date", header.first);               break;
										case 1:  available[i] = this->_fmk->compare("server", header.first);             break;
										case 2:  available[i] = this->_fmk->compare("connection", header.first);         break;
										case 3:  available[i] = this->_fmk->compare("proxy-connection", header.first);   break;
										case 4:  available[i] = this->_fmk->compare("x-powered-by", header.first);       break;
										case 5:  available[i] = this->_fmk->compare("content-type", header.first);       break;
										case 6:  available[i] = this->_fmk->compare("content-length", header.first);     break;
										case 7:  available[i] = this->_fmk->compare("content-encoding", header.first);   break;
										case 8:  available[i] = this->_fmk->compare("transfer-encoding", header.first);  break;
										case 9:  available[i] = this->_fmk->compare("x-awh-encryption", header.first);   break;
										case 10: available[i] = this->_fmk->compare("www-authenticate", header.first);   break;
										case 11: available[i] = this->_fmk->compare("proxy-authenticate", header.first); break;
									}
									// Если заголовок разрешён для вывода
									if(allow){
										/**
										 * Выполняем првоерку заголовка
										 */
										switch(i){
											case 2:
											case 3:
											case 6:
											case 7:
											case 8:
											case 9: allow = !available[i]; break;
										}
										// Если ответ является информационным
										if((((res.code >= 100) && (res.code < 200)) || (res.code == 204)) && available[i]){
											/**
											 * Запрещяем указанным заголовкам формирование
											 */
											switch(i){
												case 0:
												case 5:
												case 6:
												case 7:
												case 8:
												case 9:
												case 10:
												case 11: allow = false; break;
											}
										}
										// Если заголовок запрещён к выводу
										if(!allow)
											// Добавляем заголовко в список системных
											systemHeaders.emplace(header.first);
									}
								}
								// Если заголовок не является запрещённым, добавляем заголовок в ответ
								if(allow)
									// Формируем строку ответа
									result.push_back(std::make_pair(this->_fmk->transform(header.first, fmk_t::transform_t::LOWER), header.second));
							}
							// Если заголовок не запрещён
							if(!available[1] && !this->isInBlacklist("server"))
								// Добавляем название сервера в ответ
								result.push_back(std::make_pair("server", this->_agent.name));
							// Если заголовок не запрещён
							if(!available[4] && !this->isInBlacklist("x-powered-by"))
								// Добавляем название рабочей системы в ответ
								result.push_back(std::make_pair("x-powered-by", this->agent(flag)));
							// Если заголовок авторизации не передан
							if(((res.code == 401) && !available[10]) || ((res.code == 407) && !available[11])){
								// Получаем параметры авторизации
								const string & auth = this->_auth.server;
								// Если параметры авторизации получены
								if(!auth.empty()){
									/**
									 * Определяем код авторизации
									 */
									switch(res.code){
										// Если авторизация производится для Web-Сервера
										case 401: {
											// Если заголовок не запрещён
											if(!this->isInBlacklist("www-authenticate"))
												// Добавляем параметры авторизации
												result.push_back(std::make_pair("www-authenticate", auth));
										} break;
										// Если авторизация производится для Прокси-Сервера
										case 407: {
											// Если заголовок не запрещён
											if(!this->isInBlacklist("proxy-authenticate"))
												// Добавляем параметры авторизации
												result.push_back(std::make_pair("proxy-authenticate", auth));
										} break;
									}
								}
							}
							// Если сервер соответствует Websocket-серверу
							if(this->_session.identity == identity_t::WS){
								// Если заголовок не запрещён
								if(!available[0] && !this->isInBlacklist("date")){
									// Запоминаем, что заголовок даты уже указан
									available[0] = !available[0];
									// Добавляем заголовок даты в ответ
									result.push_back(std::make_pair("date", this->date()));
								}
							}
							// Если запрос должен содержать тело и тело ответа существует
							if((res.code >= 200) && (res.code != 204) && (res.code != 304) && (res.code != 308)){
								// Устанавливаем Content-Type если не передан
								if(!available[5] && ((this->_session.identity == identity_t::HTTP) || (res.code >= 400)) && !this->isInBlacklist("content-type"))
									// Добавляем заголовок в ответ
									result.push_back(std::make_pair("content-type", HTTP_HEADER_CONTENTTYPE));
								// Если тело запроса существует
								if(!this->_web.body().empty()){
									// Выполняем компрессию полезной нагрузки
									this->compress();
									// Выполняем шифрование полезной нагрузки
									this->encrypt();
									// Проверяем нужно ли передать тело разбив на чанки
									this->_transfer.chunking = (this->_encrypt.crypted || (this->_compressors.current != compressor_t::NONE));
									// Если заголовок не запрещён
									if(!available[0] && !this->isInBlacklist("date"))
										// Добавляем заголовок даты в ответ
										result.push_back(std::make_pair("date", this->date()));
									// Если данные зашифрованы, устанавливаем соответствующие заголовки
									if(this->_encrypt.crypted)
										// Устанавливаем X-AWH-Encryption
										result.push_back(std::make_pair("x-awh-encryption", std::to_string(static_cast <uint16_t> (this->_encrypt.cipher))));
									/**
									 * Определяем метод компрессии полезной нагрузки
									 */
									switch(static_cast <uint8_t> (this->_compressors.current)){
										// Если полезная нагрузка сжата методом LZ4
										case static_cast <uint8_t> (compressor_t::LZ4):
											// Устанавливаем Content-Encoding если не передан
											result.push_back(std::make_pair("content-encoding", "lz4"));
										break;
										// Если полезная нагрузка сжата методом Zstandard
										case static_cast <uint8_t> (compressor_t::ZSTD):
											// Устанавливаем Content-Encoding если не передан
											result.push_back(std::make_pair("content-encoding", "zstd"));
										break;
										// Если полезная нагрузка сжата методом LZma
										case static_cast <uint8_t> (compressor_t::LZMA):
											// Устанавливаем Content-Encoding если не передан
											result.push_back(std::make_pair("content-encoding", "xz"));
										break;
										// Если полезная нагрузка сжата методом Brotli
										case static_cast <uint8_t> (compressor_t::BROTLI):
											// Устанавливаем Content-Encoding если не передан
											result.push_back(std::make_pair("content-encoding", "br"));
										break;
										// Если полезная нагрузка сжата методом BZip2
										case static_cast <uint8_t> (compressor_t::BZIP2):
											// Устанавливаем Content-Encoding если не передан
											result.push_back(std::make_pair("content-encoding", "bzip2"));
										break;
										// Если полезная нагрузка сжата методом GZip
										case static_cast <uint8_t> (compressor_t::GZIP):
											// Устанавливаем Content-Encoding если не передан
											result.push_back(std::make_pair("content-encoding", "gzip"));
										break;
										// Если полезная нагрузка сжата методом Deflate
										case static_cast <uint8_t> (compressor_t::DEFLATE):
											// Устанавливаем Content-Encoding если не передан
											result.push_back(std::make_pair("content-encoding", "deflate"));
										break;
									}
								// Если тело запроса не существует
								} else {
									// Проверяем нужно ли передать тело разбив на чанки
									this->_transfer.chunking = (this->_encrypt.enabled || (this->_compressors.selected != compressor_t::NONE));
									// Если заголовок не запрещён
									if(!available[0] && !this->isInBlacklist("date"))
										// Добавляем заголовок даты в ответ
										result.push_back(std::make_pair("date", this->date()));
									// Если данные зашифрованы, устанавливаем соответствующие заголовки
									if(this->_encrypt.enabled && !this->isInBlacklist("x-awh-encryption"))
										// Устанавливаем X-AWH-Encryption
										result.push_back(std::make_pair("x-awh-encryption", std::to_string(static_cast <uint16_t> (this->_encrypt.cipher))));
									// Устанавливаем Content-Encoding если не передан
									if(!this->isInBlacklist("content-encoding")){
										/**
										 * Определяем метод компрессии полезной нагрузки
										 */
										switch(static_cast <uint8_t> (this->_compressors.selected)){
											// Если полезная нагрузка сжата методом LZ4
											case static_cast <uint8_t> (compressor_t::LZ4):
												// Устанавливаем Content-Encoding если не передан
												result.push_back(std::make_pair("content-encoding", "lz4"));
											break;
											// Если полезная нагрузка сжата методом Zstandard
											case static_cast <uint8_t> (compressor_t::ZSTD):
												// Устанавливаем Content-Encoding если не передан
												result.push_back(std::make_pair("content-encoding", "zstd"));
											break;
											// Если полезная нагрузка сжата методом LZma
											case static_cast <uint8_t> (compressor_t::LZMA):
												// Устанавливаем Content-Encoding если не передан
												result.push_back(std::make_pair("content-encoding", "xz"));
											break;
											// Если полезная нагрузка сжата методом Brotli
											case static_cast <uint8_t> (compressor_t::BROTLI):
												// Устанавливаем Content-Encoding если не передан
												result.push_back(std::make_pair("content-encoding", "br"));
											break;
											// Если полезная нагрузка сжата методом BZip2
											case static_cast <uint8_t> (compressor_t::BZIP2):
												// Устанавливаем Content-Encoding если не передан
												result.push_back(std::make_pair("content-encoding", "bzip2"));
											break;
											// Если полезная нагрузка сжата методом GZip
											case static_cast <uint8_t> (compressor_t::GZIP):
												// Устанавливаем Content-Encoding если не передан
												result.push_back(std::make_pair("content-encoding", "gzip"));
											break;
											// Если полезная нагрузка сжата методом Deflate
											case static_cast <uint8_t> (compressor_t::DEFLATE):
												// Устанавливаем Content-Encoding если не передан
												result.push_back(std::make_pair("content-encoding", "deflate"));
											break;
										}
									}
								}
							// Очищаем тела сообщения
							} else this->clear(web_t::unit_t::BODY);
						} break;
					}
				}
			} break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если функция обратного вызова на на вывод ошибок установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (flag)), log_t::flag_t::WARNING, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::WARNING, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки функций обратного вызова
 *
 * @param callback функции обратного вызова
 */
void awh::Http::callback(const callback_t & callback) noexcept {
	// Выполняем установку функции обратного вызова на событие получения ошибки
	this->_callback.set("error", callback);
	// Устанавливаем функции обратного вызова
	this->_web.callback(callback);
}
/**
 * @brief Метод проверки на зашифрованные данные
 *
 * @return флаг проверки на зашифрованные данные
 */
bool awh::Http::crypted() const noexcept {
	// Выводим результат проверки
	return this->_encrypt.crypted;
}
/**
 * @brief Метод активации шифрования
 *
 * @param mode флаг активации шифрования
 */
void awh::Http::encryption(const bool mode) noexcept {
	// Устанавливаем флаг шифрования
	this->_encrypt.enabled = mode;
}
/**
 * @brief Метод установки параметров шифрования
 *
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::Http::encryption(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Если пароль шифрования передан
	if(!pass.empty()){
		// Устанавливаем соль шифрования
		this->_hash.salt(salt);
		// Устанавливаем пароль шифрования
		this->_hash.password(pass);
		// Устанавливаем размер шифрования
		this->_encrypt.cipher = cipher;
	}
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Http::Http(const fmk_t * fmk, const log_t * log) noexcept :
 _uri(fmk, log), _web(fmk, log), _auth(fmk, log),
 _hash(log), _callback(log), _fmk(fmk), _log(log) {
	// Выполняем инициализацию модуля
	this->init();
}
/**
 * @brief Конструктор
 *
 * @param identity идентичность протокола модуля
 * @param fmk      объект фреймворка
 * @param log      объект для работы с логами
 */
awh::Http::Http(const identity_t identity, const fmk_t * fmk, const log_t * log) noexcept :
 _uri(fmk, log), _web(fmk, log), _auth(fmk, log),
 _hash(log), _callback(log), _fmk(fmk), _log(log) {
	// Выполняем инициализацию модуля
	this->init();
	// Устанавливаем идентичность протокола модуля
	this->_session.identity = identity;
}
