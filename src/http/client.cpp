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
 * status Метод проверки текущего статуса
 * @return результат проверки текущего статуса
 */
awh::Http::status_t awh::client::Http::status() noexcept {
	// Результат работы функции
	status_t result = status_t::FAULT;
	// Получаем объект параметров запроса
	const web_t::res_t & response = this->_web.response();
	// Проверяем код ответа
	switch(response.code){
		// Если требуется авторизация
		case 401:
		case 407: {
			// Определяем тип авторизации
			switch(static_cast <uint8_t> (this->_auth.client.type())){
				// Если производится авторизация DIGEST
				case static_cast <uint8_t> (awh::auth_t::type_t::DIGEST): {
					// Получаем параметры авторизации
					const string & auth = this->_web.header(response.code == 401 ? "www-authenticate" : "proxy-authenticate");
					// Если параметры авторизации найдены
					if(!auth.empty()){
						// Устанавливаем заголовок HTTP в параметры авторизации
						this->_auth.client.header(auth);
						// Просим повторить авторизацию ещё раз
						result = status_t::RETRY;
					}
				} break;
				// Если производится авторизация BASIC
				case static_cast <uint8_t> (awh::auth_t::type_t::BASIC):
					// Просим повторить авторизацию ещё раз
					result = status_t::RETRY;
				break;
			}
		} break;
		// Если нужно произвести редирект
		case 201:
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
				this->_web.request(std::move(request));
				// Просим повторить авторизацию ещё раз
				result = status_t::RETRY;
			}
		} break;
		// Сообщаем, что авторизация прошла успешно
		case 100:
		case 101:
		case 200:
		case 202:
		case 203:
		case 204:
		case 205:
		case 206: result = status_t::GOOD; break;
	}
	// Выводим результат
	return result;
}
/**
 * user Метод установки параметров авторизации
 * @param user логин пользователя для авторизации на сервере
 * @param pass пароль пользователя для авторизации на сервере
 */
void awh::client::Http::user(const string & user, const string & pass) noexcept {
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
void awh::client::Http::authType(const awh::auth_t::type_t type, const awh::auth_t::hash_t hash) noexcept {
	// Устанавливаем тип авторизации
	this->_auth.client.type(type, hash);
}
/**
 * Http Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::client::Http::Http(const fmk_t * fmk, const log_t * log) noexcept : awh::http_t(fmk, log) {
	// Выполняем установку идентичность клиента к протоколу HTTP
	this->_identity = identity_t::HTTP;
	// Устанавливаем тип HTTP-парсера
	this->_web.hid(web_t::hid_t::CLIENT);
}
