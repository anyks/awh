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
#include <http/client.hpp>

/**
 * checkAuth Метод проверки авторизации
 * @return результат проверки авторизации
 */
awh::Http::stath_t awh::client::Http::checkAuth() noexcept {
	// Результат работы функции
	stath_t result = stath_t::FAULT;
	// Получаем объект параметров запроса
	web_t::query_t query = this->web.getQuery();
	// Проверяем код ответа
	switch(query.code){
		// Если требуется авторизация
		case 401:
		case 407: {
			// Если попытки провести аутентификацию ещё небыло, пробуем ещё раз
			if(!this->failAuth && (this->auth.client.getType() == awh::auth_t::type_t::DIGEST)){
				// Получаем параметры авторизации
				const string & auth = this->web.getHeader(query.code == 401 ? "www-authenticate" : "proxy-authenticate");
				// Если параметры авторизации найдены
				if((this->failAuth = !auth.empty())){
					// Устанавливаем заголовок HTTP в параметры авторизации
					this->auth.client.setHeader(auth);
					// Просим повторить авторизацию ещё раз
					result = stath_t::RETRY;
				}
			}
		} break;
		// Если нужно произвести редирект
		case 301:
		case 308: {
			// Получаем параметры переадресации
			const string & location = this->web.getHeader("location");
			// Если адрес перенаправления найден
			if(!location.empty()){
				// Выполняем парсинг URL
				uri_t::url_t tmp = this->uri->parseUrl(location);
				// Если параметры URL существуют
				if(!this->url.params.empty())
					// Переходим по всему списку параметров
					for(auto & param : this->url.params) tmp.params.emplace(param);
				// Меняем IP адрес сервера
				const_cast <uri_t::url_t *> (&this->url)->ip = move(tmp.ip);
				// Меняем порт сервера
				const_cast <uri_t::url_t *> (&this->url)->port = move(tmp.port);
				// Меняем на путь сервере
				const_cast <uri_t::url_t *> (&this->url)->path = move(tmp.path);
				// Меняем доменное имя сервера
				const_cast <uri_t::url_t *> (&this->url)->domain = move(tmp.domain);
				// Меняем протокол запроса сервера
				const_cast <uri_t::url_t *> (&this->url)->schema = move(tmp.schema);
				// Устанавливаем новый список параметров
				const_cast <uri_t::url_t *> (&this->url)->params = move(tmp.params);
				// Просим повторить авторизацию ещё раз
				result = stath_t::RETRY;
			}
		} break;
		// Сообщаем, что авторизация прошла успешно
		case 100:
		case 101:
		case 200:
		case 201:
		case 202:
		case 203:
		case 204:
		case 205:
		case 206: result = stath_t::GOOD; break;
	}
	// Выводим результат
	return result;
}
/**
 * setUser Метод установки параметров авторизации
 * @param user логин пользователя для авторизации на сервере
 * @param pass пароль пользователя для авторизации на сервере
 */
void awh::client::Http::setUser(const string & user, const string & pass) noexcept {
	// Если пользователь и пароль переданы
	if(!user.empty() && !pass.empty()){
		// Устанавливаем логин пользователя
		this->auth.client.setUser(user);
		// Устанавливаем пароль пользователя
		this->auth.client.setPass(pass);
	}
}
/**
 * setAuthType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::client::Http::setAuthType(const awh::auth_t::type_t type, const awh::auth_t::hash_t hash) noexcept {
	// Устанавливаем тип авторизации
	this->auth.client.setType(type, hash);
}
/**
 * Http Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 * @param uri объект работы с URI
 */
awh::client::Http::Http(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept : awh::http_t(fmk, log, uri) {
	// Устанавливаем тип HTTP парсера
	this->web.init(web_t::hid_t::CLIENT);
	// Устанавливаем тип HTTP модуля
	this->httpType = web_t::hid_t::CLIENT;
}
