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
 * @copyright: Copyright © 2021
 */

// Подключаем заголовочный файл
#include <ws/client.hpp>

/**
 * commit Метод применения полученных результатов
 */
void awh::client::WS::commit() noexcept {
	// Сбрасываем флаг шифрования
	this->crypt = false;
	// Список доступных расширений
	vector <string> extensions;
	// Выполняем проверку авторизации
	this->stath = this->checkAuth();
	// Если ключ соответствует
	if(this->stath == stath_t::GOOD)
		// Устанавливаем стейт рукопожатия
		this->state = state_t::GOOD;
	// Поменяем данные как бракованные
	else this->state = state_t::BROKEN;
	// Отключаем сжатие ответа с сервера
	this->_compress = compress_t::NONE;
	// Получаем значение заголовка Sec-Websocket-Extensions
	const string & ext = this->web.header("sec-websocket-extensions");
	// Если заголовок найден
	if(!ext.empty()){
		// Выполняем разделение параметров расширений
		if(!this->fmk->split(ext, ";", extensions).empty()){
			// Выполняем включение перехвата контекста
			this->_server.takeover = true;
			this->_client.takeover = true;
			// Ищем поддерживаемые заголовки
			for(auto & val : extensions){
				// Если нужно производить шифрование данных
				if((this->crypt = this->fmk->exists("permessage-encrypt=", val))){
					// Определяем размер шифрования
					switch(::stoi(val.substr(19))){
						// Если шифрование произведено 128 битным ключём
						case 128: this->hash.cipher(hash_t::cipher_t::AES128); break;
						// Если шифрование произведено 192 битным ключём
						case 192: this->hash.cipher(hash_t::cipher_t::AES192); break;
						// Если шифрование произведено 256 битным ключём
						case 256: this->hash.cipher(hash_t::cipher_t::AES256); break;
					}
				// Если клиент просит отключить перехват контекста сжатия для сервера
				} else if(this->fmk->compare(val, "server_no_context_takeover"))
					// Выполняем отключение перехвата контекста
					this->_server.takeover = false;
				// Если клиент просит отключить перехват контекста сжатия для клиента
				else if(this->fmk->compare(val, "client_no_context_takeover"))
					// Выполняем отключение перехвата контекста
					this->_client.takeover = false;
				// Если получены заголовки требующие сжимать передаваемые фреймы методом Deflate
				else if(this->fmk->compare(val, "permessage-deflate") || this->fmk->compare(val, "perframe-deflate"))
					// Устанавливаем требование выполнять декомпрессию полезной нагрузки
					this->_compress = compress_t::DEFLATE;
				// Если получены заголовки требующие сжимать передаваемые фреймы методом GZip
				else if(this->fmk->compare(val, "permessage-gzip") || this->fmk->compare(val, "perframe-gzip"))
					// Устанавливаем требование выполнять декомпрессию полезной нагрузки
					this->_compress = compress_t::GZIP;
				// Если получены заголовки требующие сжимать передаваемые фреймы методом Brotli
				else if(this->fmk->compare(val, "permessage-br") || this->fmk->compare(val, "perframe-br"))
					// Устанавливаем требование выполнять декомпрессию полезной нагрузки
					this->_compress = compress_t::BROTLI;
				// Если размер скользящего окна для клиента получен
				else if(this->fmk->exists("client_max_window_bits=", val))
					// Устанавливаем размер скользящего окна
					this->_client.wbit = ::stoi(val.substr(23));
				// Если разрешено использовать максимальный размер скользящего окна для клиента
				else if(this->fmk->compare(val, "client_max_window_bits"))
					// Устанавливаем максимальный размер скользящего окна
					this->_client.wbit = GZIP_MAX_WBITS;
				// Если размер скользящего окна для сервера получен
				else if(this->fmk->exists("server_max_window_bits=", val))
					// Устанавливаем размер скользящего окна
					this->_server.wbit = ::stoi(val.substr(23));
				// Если разрешено использовать максимальный размер скользящего окна для сервера
				else if(this->fmk->compare(val, "server_max_window_bits"))
					// Устанавливаем максимальный размер скользящего окна
					this->_server.wbit = GZIP_MAX_WBITS;
			}
		}
	}
	// Ищем подпротокол сервера
	const string & sub = this->web.header("sec-websocket-protocol");
	// Если подпротокол найден, устанавливаем его
	if(!sub.empty()) this->_sub = sub;
}
/**
 * checkKey Метод проверки ключа сервера
 * @return результат проверки
 */
bool awh::client::WS::checkKey() noexcept {
	// Результат работы функции
	bool result = false;
	// Получаем параметры ключа сервера
	const string & auth = this->web.header("sec-websocket-accept");
	// Если параметры авторизации найдены
	if(!auth.empty()){
		// Получаем ключ для проверки
		const string & key = this->sha1();
		// Если ключи не соответствуют, запрещаем работу
		result = this->fmk->compare(key, auth);
	}
	// Выводим результат
	return result;
}
/**
 * checkVer Метод проверки на версию протокола
 * @return результат проверки соответствия
 */
bool awh::client::WS::checkVer() noexcept {
	// Сообщаем, что версия соответствует
	return true;
}
/**
 * checkAuth Метод проверки авторизации
 * @return результат проверки авторизации
 */
awh::Http::stath_t awh::client::WS::checkAuth() noexcept {
	// Результат работы функции
	http_t::stath_t result = http_t::stath_t::FAULT;
	// Получаем объект параметров ответа
	const web_t::res_t & response = this->web.response();
	// Проверяем код ответа
	switch(response.code){
		// Если требуется авторизация
		case 401: {
			// Определяем тип авторизации
			switch(static_cast <uint8_t> (this->auth.client.type())){
				// Если производится авторизация DIGEST
				case static_cast <uint8_t> (awh::auth_t::type_t::DIGEST): {
					// Получаем параметры авторизации
					const string & auth = this->web.header("www-authenticate");
					// Если параметры авторизации найдены
					if(!auth.empty()){
						// Устанавливаем заголовок HTTP в параметры авторизации
						this->auth.client.header(auth);
						// Просим повторить авторизацию ещё раз
						result = http_t::stath_t::RETRY;
					}
				} break;
				// Если производится авторизация BASIC
				case static_cast <uint8_t> (awh::auth_t::type_t::BASIC):
					// Просим повторить авторизацию ещё раз
					result = http_t::stath_t::RETRY;
				break;
			}
		} break;
		// Если нужно произвести редирект
		case 301:
		case 302:
		case 303:
		case 307:
		case 308: {
			// Получаем параметры переадресации
			const string & location = this->web.header("location");
			// Если адрес перенаправления найден
			if(!location.empty()){
				// Получаем объект параметров запроса
				web_t::req_t request = this->web.request();
				// Выполняем парсинг полученного URL-адреса
				request.url = this->uri.parse(location);
				// Выполняем установку параметров запроса
				this->web.request(std::move(request));
				// Просим повторить авторизацию ещё раз
				result = http_t::stath_t::RETRY;
			}
		} break;
		// Сообщаем, что авторизация прошла успешно
		case 101: result = http_t::stath_t::GOOD; break;
	}
	// Выводим результат
	return result;
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
		this->auth.client.user(user);
		// Устанавливаем пароль пользователя
		this->auth.client.pass(pass);
	}
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::client::WS::authType(const awh::auth_t::type_t type, const awh::auth_t::hash_t hash) noexcept {
	// Устанавливаем тип авторизации
	this->auth.client.type(type, hash);
}
