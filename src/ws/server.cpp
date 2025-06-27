/**
 * @file: server.cpp
 * @date: 2021-12-19
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
 * Подключаем заголовочный файл
 */
#include <ws/server.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * commit Метод применения полученных результатов
 */
void awh::server::WS::commit() noexcept {
	// Если данные ещё не зафиксированы
	if(this->_status == status_t::NONE){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем проверку авторизации
			this->_status = this->status();
			// Если ключ соответствует
			if(this->_status == status_t::GOOD)
				// Устанавливаем стейт рукопожатия
				this->_state = state_t::GOOD;
			// Поменяем данные как бракованные
			else this->_state = state_t::BROKEN;
			{
				// Список доступных расширений
				vector <string> extensions;
				// Отключаем сжатие ответа с сервера
				this->_compressors.selected = compressor_t::NONE;
				// Отключаем сжатие тела сообщения
				http_t::_compressors.selected = compressor_t::NONE;
				// Переходим по всему списку заголовков
				for(auto & header : this->_web.headers()){
					// Если заголовок получен с описанием методов компрессии
					if(this->_fmk->compare(header.first, "accept-encoding")){
						// Если список поддерживаемых протоколов установлен
						if(!http_t::_compressors.supports.empty()){
							// Если конкретный метод сжатия не запрашивается
							if(this->_fmk->compare(header.second, "*"))
								// Устанавливаем флаг метода компрессии
								http_t::_compressors.selected = http_t::_compressors.supports.rbegin()->second;
							// Если запрашиваются конкретные методы сжатия
							else {
								// Если найден запрашиваемый метод компрессии LZ4
								if(this->_fmk->exists("lz4", header.second)) {
									// Выполняем поиск в списке доступных компрессоров запрашиваемый компрессор
									if(this->_fmk->findInMap(compressor_t::LZ4, http_t::_compressors.supports) != http_t::_compressors.supports.end())
										// Устанавливаем флаг метода компрессии
										http_t::_compressors.selected = compressor_t::LZ4;
									// Выполняем сброс типа компрессии
									else http_t::_compressors.selected = compressor_t::NONE;
								// Если найден запрашиваемый метод компрессии Zstandard
								} else if(this->_fmk->exists("zstd", header.second)) {
									// Выполняем поиск в списке доступных компрессоров запрашиваемый компрессор
									if(this->_fmk->findInMap(compressor_t::ZSTD, http_t::_compressors.supports) != http_t::_compressors.supports.end())
										// Устанавливаем флаг метода компрессии
										http_t::_compressors.selected = compressor_t::ZSTD;
									// Выполняем сброс типа компрессии
									else http_t::_compressors.selected = compressor_t::NONE;
								// Если найден запрашиваемый метод компрессии LZma
								} else if(this->_fmk->exists("xz", header.second)) {
									// Выполняем поиск в списке доступных компрессоров запрашиваемый компрессор
									if(this->_fmk->findInMap(compressor_t::LZMA, http_t::_compressors.supports) != http_t::_compressors.supports.end())
										// Устанавливаем флаг метода компрессии
										http_t::_compressors.selected = compressor_t::LZMA;
									// Выполняем сброс типа компрессии
									else http_t::_compressors.selected = compressor_t::NONE;
								// Если найден запрашиваемый метод компрессии Brotli
								} else if(this->_fmk->exists("br", header.second)){
									// Выполняем поиск в списке доступных компрессоров запрашиваемый компрессор
									if(this->_fmk->findInMap(compressor_t::BROTLI, http_t::_compressors.supports) != http_t::_compressors.supports.end())
										// Устанавливаем флаг метода компрессии
										http_t::_compressors.selected = compressor_t::BROTLI;
									// Выполняем сброс типа компрессии
									else http_t::_compressors.selected = compressor_t::NONE;
								// Если найден запрашиваемый метод компрессии BZip2
								} else if(this->_fmk->exists("bzip2", header.second)){
									// Выполняем поиск в списке доступных компрессоров запрашиваемый компрессор
									if(this->_fmk->findInMap(compressor_t::BZIP2, http_t::_compressors.supports) != http_t::_compressors.supports.end())
										// Устанавливаем флаг метода компрессии
										http_t::_compressors.selected = compressor_t::BZIP2;
									// Выполняем сброс типа компрессии
									else http_t::_compressors.selected = compressor_t::NONE;
								// Если найден запрашиваемый метод компрессии GZip
								} else if(this->_fmk->exists("gzip", header.second)) {
									// Выполняем поиск в списке доступных компрессоров запрашиваемый компрессор
									if(this->_fmk->findInMap(compressor_t::GZIP, http_t::_compressors.supports) != http_t::_compressors.supports.end())
										// Устанавливаем флаг метода компрессии
										http_t::_compressors.selected = compressor_t::GZIP;
									// Выполняем сброс типа компрессии
									else http_t::_compressors.selected = compressor_t::NONE;
								// Если найден запрашиваемый метод компрессии Deflate
								} else if(this->_fmk->exists("deflate", header.second)) {
									// Выполняем поиск в списке доступных компрессоров запрашиваемый компрессор
									if(this->_fmk->findInMap(compressor_t::DEFLATE, http_t::_compressors.supports) != http_t::_compressors.supports.end())
										// Устанавливаем флаг метода компрессии
										http_t::_compressors.selected = compressor_t::DEFLATE;
									// Выполняем сброс типа компрессии
									else http_t::_compressors.selected = compressor_t::NONE;
								}
							}
						}
					// Если заголовок сабпротокола найден
					} else if(this->_fmk->compare(header.first, "sec-websocket-protocol")) {
						// Проверяем, соответствует ли желаемый подпротокол нашему установленному
						if(this->_supportedProtocols.find(header.second) != this->_supportedProtocols.end())
							// Устанавливаем выбранный подпротокол
							this->_selectedProtocols.emplace(header.second);
					// Если заголовок расширения найден
					} else if(this->_fmk->compare(header.first, "sec-websocket-extensions")) {
						// Запись названия расширения
						string extension = "";
						// Выполняем перебор записи расширения
						for(auto & letter : header.second){
							// Определяем чему соответствует буква
							switch(letter){
								// Если буква соответствует разделителю расширения
								case ';': {
									// Если слово собранно
									if(!extension.empty() && !this->extractExtension(extension))
										// Выполняем добавление слова в список записей
										extensions.push_back(::move(extension));
									// Выполняем очистку слова записи
									extension.clear();
									// Если список записей собран
									if(!extensions.empty()){
										// Выполняем добавление списка записей в список расширений
										this->_extensions.push_back(::move(extensions));
										// Выполняем очистку списка расширений
										extensions.clear();
									}
								} break;
								// Если буква соответствует разделителю группы расширений
								case ',': {
									// Если слово собранно
									if(!extension.empty() && !this->extractExtension(extension))
										// Выполняем добавление слова в список записей
										extensions.push_back(::move(extension));
									// Выполняем очистку слова записи
									extension.clear();
								} break;
								// Если буква соответствует пробелу
								case ' ': break;
								// Если буква соответствует знаку табуляции
								case '\t': break;
								// Если буква соответствует букве
								default: extension.append(1, letter);
							}
						}
						// Если слово собранно
						if(!extension.empty() && !this->extractExtension(extension))
							// Выполняем добавление слова в список записей
							extensions.push_back(::move(extension));
						// Выполняем очистку слова записи
						extension.clear();
					// Если заголовок получен зашифрованных данных
					} else if(this->_fmk->compare(header.first, "x-awh-encryption")) {
						// Если заголовок найден
						if((http_t::_crypted = !header.second.empty())){
							/**
							 * Выполняем отлов ошибок
							 */
							try {
								// Определяем размер шифрования
								switch(static_cast <uint16_t> (::stoi(header.second))){
									// Если шифрование произведено 128 битным ключём
									case 128: this->_cipher = hash_t::cipher_t::AES128; break;
									// Если шифрование произведено 192 битным ключём
									case 192: this->_cipher = hash_t::cipher_t::AES192; break;
									// Если шифрование произведено 256 битным ключём
									case 256: this->_cipher = hash_t::cipher_t::AES256; break;
								}
							/**
							 * Если возникает ошибка
							 */
							} catch(const exception &) {
								// Если шифрование произведено 128 битным ключём
								this->_cipher = hash_t::cipher_t::AES128;
							}
						}
					}
				}
				// Если список записей собран
				if(!extensions.empty())
					// Выполняем добавление списка записей в список расширений
					this->_extensions.push_back(::move(extensions));
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * status Метод проверки текущего статуса
 * @return результат проверки текущего статуса
 */
awh::Http::status_t awh::server::WS::status() noexcept {
	// Результат работы функции
	http_t::status_t result = http_t::status_t::FAULT;
	// Если авторизация требуется
	if(this->_auth.server.type() != awh::auth_t::type_t::NONE){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Получаем параметры авторизации
			const string & auth = this->_web.header("authorization");
			// Если параметры авторизации найдены
			if(!auth.empty()){
				// Метод HTTP-запроса
				string method = "";
				// Устанавливаем заголовок HTTP в параметры авторизации
				this->_auth.server.header(auth);
				// Определяем метод запроса
				switch(static_cast <uint8_t> (this->_web.request().method)){
					// Если метод запроса указан как GET
					case static_cast <uint8_t> (web_t::method_t::GET):
						// Устанавливаем метод запроса
						method = "get";
					break;
					// Если метод запроса указан как CONNECT
					case static_cast <uint8_t> (web_t::method_t::CONNECT):
						// Устанавливаем метод запроса
						method = "connect";
					break;
				}
				// Выполняем проверку авторизации
				if(this->_auth.server.check(method))
					// Устанавливаем успешный результат авторизации
					result = http_t::status_t::GOOD;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	// Сообщаем, что авторизация прошла успешно
	} else result = http_t::status_t::GOOD;
	// Выводим результат
	return result;
}
/**
 * check Метод проверки шагов рукопожатия
 * @param flag флаг выполнения проверки
 * @return     результат проверки соответствия
 */
bool awh::server::WS::check(const flag_t flag) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Определяем флаг выполнения проверки
		switch(static_cast <uint8_t> (flag)){
			// Если требуется выполнить проверку соответствие ключа
			case static_cast <uint8_t> (flag_t::KEY): {
				// Получаем параметры ключа клиента
				this->_key = this->_web.header("sec-websocket-key");
				// Выводим результат
				return !this->_key.empty();
			}
			// Если требуется выполнить проверку версию протокола
			case static_cast <uint8_t> (flag_t::VERSION): {
				// Переходим по всему списку заголовков
				for(auto & header : this->_web.headers()){
					// Если заголовок найден
					if(this->_fmk->compare(header.first, "sec-websocket-version")){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Проверяем, совпадает ли желаемая версия протокола
							return (static_cast <uint8_t> (::stoi(header.second)) == static_cast <uint8_t> (WS_VERSION));
						/**
						 * Если возникает ошибка
						 */
						} catch(const exception &) {
							// Сообщяем, что проверка не прошла
							return false;
						}
					}
				}
			} break;
			// Если требуется выполнить проверки на переключение протокола
			case static_cast <uint8_t> (flag_t::UPGRADE):
				// Выполняем проверку переключения протокола
				return ws_core_t::check(flag);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если функция обратного вызова на на вывод ошибок установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (flag)), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return false;
}
/**
 * realm Метод установки название сервера
 * @param realm название сервера
 */
void awh::server::WS::realm(const string & realm) noexcept {
	// Если название сервера передано
	if(!realm.empty())
		// Устанавливаем название сервера
		this->_auth.server.realm(realm);
}
/**
 * opaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::server::WS::opaque(const string & opaque) noexcept {
	// Если временный ключ сессии сервера передан
	if(!opaque.empty())
		// Устанавливаем временный ключ сессии
		this->_auth.server.opaque(opaque);
}
/**
 * dataAuth Метод извлечения данных авторизации
 * @return данные модуля авторизации
 */
awh::server::auth_t::data_t awh::server::WS::dataAuth() const noexcept {
	// Выполняем извлечение данных авторизации
	return this->_auth.server.data();
}
/**
 * dataAuth Метод установки данных авторизации
 * @param data данные авторизации для установки
 */
void awh::server::WS::dataAuth(const server::auth_t::data_t & data) noexcept {
	// Выполняем установку данных авторизации
	this->_auth.server.data(data);
}
/**
 * extractPassCallback Метод добавления функции извлечения пароля
 * @param callback функция обратного вызова для извлечения пароля
 */
void awh::server::WS::extractPassCallback(function <string (const string &)> callback) noexcept {
	// Устанавливаем внешнюю функцию
	this->_auth.server.extractPassCallback(callback);
}
/**
 * authCallback Метод добавления функции обработки авторизации
 * @param callback функция обратного вызова для обработки авторизации
 */
void awh::server::WS::authCallback(function <bool (const string &, const string &)> callback) noexcept {
	// Устанавливаем внешнюю функцию
	this->_auth.server.authCallback(callback);
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::server::WS::authType(const awh::auth_t::type_t type, const awh::auth_t::hash_t hash) noexcept {
	// Устанавливаем тип авторизации
	this->_auth.server.type(type, hash);
}
/**
 * WS Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::WS::WS(const fmk_t * fmk, const log_t * log) noexcept : ws_core_t(fmk, log) {
	// Выполняем установку списка поддерживаемых компрессоров
	http_t::compressors({
		compressor_t::ZSTD,
		compressor_t::BROTLI,
		compressor_t::GZIP,
		compressor_t::DEFLATE
	});
	// Выполняем установку идентичность клиента к протоколу WebSocket
	this->_identity = identity_t::WS;
	// Устанавливаем тип HTTP-модуля (Сервер)
	this->_web.hid(awh::web_t::hid_t::SERVER);
}
