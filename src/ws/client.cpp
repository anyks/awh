/**
 * @file: client.cpp
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
#include <ws/client.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * commit Метод применения полученных результатов
 */
void awh::client::WS::commit() noexcept {
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
				http_t::_compressors.current = compressor_t::NONE;
				// Отключаем сжатие тела сообщения
				http_t::_compressors.selected = compressor_t::NONE;
				// Переходим по всему списку заголовков
				for(auto & header : this->_web.headers()){
					// Если заголовок получен с описанием методов компрессии
					if(this->_fmk->compare(header.first, "content-encoding")){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Список компрессоров которым выполненно сжатие
							vector <string> compressors;
							// Выполняем извлечение списка компрессоров
							this->_fmk->split(header.second, ",", compressors);
							// Если список компрессоров получен
							if(!compressors.empty()){
								/**
								 * extractFn Функция выбора типа компрессора
								 * @param compressor название компрессора в текстовом виде
								 */
								auto extractFn = [this](const string & compressor) noexcept -> void {
									// Если данные пришли сжатые методом LZ4
									if(this->_fmk->compare(compressor, "lz4"))
										// Устанавливаем тип компрессии полезной нагрузки
										http_t::_compressors.current = compressor_t::LZ4;
									// Если данные пришли сжатые методом Zstandard
									else if(this->_fmk->compare(compressor, "zstd"))
										// Устанавливаем тип компрессии полезной нагрузки
										http_t::_compressors.current = compressor_t::ZSTD;
									// Если данные пришли сжатые методом LZma
									else if(this->_fmk->compare(compressor, "xz"))
										// Устанавливаем тип компрессии полезной нагрузки
										http_t::_compressors.current = compressor_t::LZMA;
									// Если данные пришли сжатые методом Brotli
									else if(this->_fmk->compare(compressor, "br"))
										// Устанавливаем тип компрессии полезной нагрузки
										http_t::_compressors.current = compressor_t::BROTLI;
									// Если данные пришли сжатые методом BZip2
									else if(http_t::_fmk->compare(compressor, "bzip2"))
										// Устанавливаем тип компрессии полезной нагрузки
										http_t::_compressors.current = compressor_t::BZIP2;
									// Если данные пришли сжатые методом GZip
									else if(http_t::_fmk->compare(compressor, "gzip"))
										// Устанавливаем тип компрессии полезной нагрузки
										http_t::_compressors.current = compressor_t::GZIP;
									// Если данные пришли сжатые методом Deflate
									else if(this->_fmk->compare(compressor, "deflate"))
										// Устанавливаем тип компрессии полезной нагрузки
										http_t::_compressors.current = compressor_t::DEFLATE;
								};
								// Если компрессоров в списке больше 1-го
								if(compressors.size() > 1){
									// Выполняем перебор всех компрессоров
									for(size_t i = (compressors.size() - 1); i > 0; i--){
										// Выполняем определение типа компрессора
										extractFn(compressors.at(i));
										// Выполняем декомпрессию
										this->decompress();
									}
								}
								// Выполняем определение типа компрессора
								extractFn(compressors.front());
								// Устанавливаем флаг в каком виде у нас хранится полезная нагрузка
								http_t::_compressors.selected = http_t::_compressors.current;
							}
						/**
						 * Если возникает ошибка
						 */
						} catch(const exception & error) {
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
					// Если заголовок расширения найден
					} else if(this->_fmk->compare(header.first, "sec-websocket-extensions")) {
						/**
						 * Выполняем отлов ошибок
						 */
						try {
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
						/**
						 * Если возникает ошибка
						 */
						} catch(const exception & error) {
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
					// Если заголовок сабпротокола найден
					} else if(this->_fmk->compare(header.first, "sec-websocket-protocol")) {
						// Проверяем, соответствует ли желаемый подпротокол нашему установленному
						if(this->_supportedProtocols.find(header.second) != this->_supportedProtocols.end())
							// Устанавливаем выбранный подпротокол
							this->_selectedProtocols.emplace(header.second);
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
			if(this->_callbacks.is("error"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
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
awh::Http::status_t awh::client::WS::status() noexcept {
	// Результат работы функции
	http_t::status_t result = http_t::status_t::FAULT;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Получаем объект параметров ответа
		const web_t::res_t & response = this->_web.response();
		// Проверяем код ответа
		switch(response.code){
			// Если требуется авторизация
			case 401:
			case 407: {
				// Получаем параметры авторизации
				const string & auth = this->_web.header(response.code == 401 ? "www-authenticate" : "proxy-authenticate");
				// Если параметры авторизации найдены
				if(!auth.empty()){
					// Устанавливаем заголовок HTTP в параметры авторизации
					this->_auth.client.header(auth);
					// Просим повторить авторизацию ещё раз
					result = http_t::status_t::RETRY;
				}
			} break;
			// Если нужно произвести редирект
			case 301:
			case 302:
			case 303:
			case 307:
			case 308: {
				// Получаем параметры переадресации
				const string & location = this->_web.header("location");
				// Если адрес перенаправления найден
				if(!location.empty()){
					// Получаем объект параметров запроса
					web_t::req_t request = this->_web.request();
					// Выполняем парсинг полученного URL-адреса
					request.url = this->_uri.parse(location);
					// Выполняем установку параметров запроса
					this->_web.request(request);
					// Просим повторить авторизацию ещё раз
					result = http_t::status_t::RETRY;
				}
			} break;
			// Сообщаем, что авторизация прошла успешно если протокол соответствует HTTP/1.1
			case 101: result = (response.version < 2.f ? http_t::status_t::GOOD : result); break;
			// Сообщаем, что авторизация прошла успешно если протокол соответствует HTTP/2
			case 200: result = (response.version >= 2.f ? http_t::status_t::GOOD : result); break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если функция обратного вызова на на вывод ошибок установлена
		if(this->_callbacks.is("error"))
			// Выполняем функцию обратного вызова
			this->_callbacks.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
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
	// Выводим результат
	return result;
}
/**
 * check Метод проверки шагов рукопожатия
 * @param flag флаг выполнения проверки
 * @return     результат проверки соответствия
 */
bool awh::client::WS::check(const flag_t flag) noexcept {
	// Определяем флаг выполнения проверки
	switch(static_cast <uint8_t> (flag)){
		// Если требуется выполнить проверку соответствие ключа
		case static_cast <uint8_t> (flag_t::KEY): {
			// Получаем параметры ключа сервера
			const string & auth = this->_web.header("sec-websocket-accept");
			// Если параметры авторизации найдены
			if(!auth.empty()){
				// Получаем ключ для проверки
				const string & key = this->sha1();
				// Если ключи не соответствуют, запрещаем работу
				return this->_fmk->compare(key, auth);
			}
		} break;
		// Если требуется выполнить проверку версию протокола
		case static_cast <uint8_t> (flag_t::VERSION):
			// Сообщаем, что версия соответствует
			return true;
		// Если требуется выполнить проверки на переключение протокола
		case static_cast <uint8_t> (flag_t::UPGRADE):
			// Выполняем проверку переключения протокола
			return ws_core_t::check(flag);
	}
	// Выводим результат
	return false;
}
/**
 * dataAuth Метод извлечения данных авторизации
 * @return данные модуля авторизации
 */
awh::client::auth_t::data_t awh::client::WS::dataAuth() const noexcept {
	// Выполняем извлечение данных авторизации
	return this->_auth.client.data();
}
/**
 * dataAuth Метод установки данных авторизации
 * @param data данные авторизации для установки
 */
void awh::client::WS::dataAuth(const client::auth_t::data_t & data) noexcept {
	// Выполняем установку данных авторизации
	this->_auth.client.data(data);
}
/**
 * user Метод установки параметров авторизации
 * @param user логин пользователя для авторизации на сервере
 * @param pass пароль пользователя для авторизации на сервере
 */
void awh::client::WS::user(const string & user, const string & pass) noexcept {
	// Если пользователь и пароль переданы
	if(!user.empty() && !pass.empty()){
		// Устанавливаем логин пользователя
		this->_auth.client.user(user);
		// Устанавливаем пароль пользователя
		this->_auth.client.pass(pass);
	}
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::client::WS::authType(const awh::auth_t::type_t type, const awh::auth_t::hash_t hash) noexcept {
	// Устанавливаем тип авторизации
	this->_auth.client.type(type, hash);
}
/**
 * WS Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::client::WS::WS(const fmk_t * fmk, const log_t * log) noexcept : ws_core_t(fmk, log) {
	// Выполняем установку списка поддерживаемых компрессоров
	http_t::compressors({
		compressor_t::ZSTD,
		compressor_t::BROTLI,
		compressor_t::GZIP,
		compressor_t::DEFLATE
	});
	// Выполняем установку идентичность клиента к протоколу WebSocket
	this->_identity = identity_t::WS;
	// Устанавливаем тип HTTP-модуля (Клиент)
	this->_web.hid(awh::web_t::hid_t::CLIENT);
}
