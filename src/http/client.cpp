/**
 * @file: client.cpp
 * @date: 2025-10-07
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
#include <http/client.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Метод проверки выполнения рукопожатия
 *
 * @return результат выполнения рукопожатия
 */
awh::Http::handshake_t awh::client::Http::handshake() noexcept {
	// Результат работы функции
	handshake_t result = handshake_t::FAULT;
	// Получаем объект параметров запроса
	const web_t::res_t & response = this->_web.response();
	/**
	 * Проверяем код ответа
	 */
	switch(response.code){
		// Если требуется авторизация
		case 401:
		case 407: {
			// Получаем параметры авторизации
			const string & auth = this->_web.header(response.code == 401 ? "WWW-Authenticate" : "Proxy-Authenticate");
			// Если параметры авторизации найдены
			if(!auth.empty()){
				// Устанавливаем заголовок HTTP в параметры авторизации
				this->_auth.client.header(auth);
				// Просим повторить авторизацию ещё раз
				result = handshake_t::RETRY;
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
			const string & location = this->_web.header("Location");
			// Если адрес перенаправления найден
			if(!location.empty()){
				// Получаем объект параметров запроса
				web_t::req_t request = this->_web.request();
				// Выполняем парсинг полученного URL-адреса
				request.url = this->_uri.parse(location);
				// Выполняем установку параметров запроса
				this->_web.request(request);
				// Просим повторить авторизацию ещё раз
				result = handshake_t::RETRY;
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
		case 206:
			// Устанавливаем статус удачного рукопожатия
			result = handshake_t::GOOD;
		break;
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод извлечения параметров авторизации
 *
 * @return параметры модуля авторизации
 */
awh::client::auth_t::settings_t awh::client::Http::authorization() const noexcept {
	// Выполняем извлечение параметров авторизации
	return this->_auth.client.settings();
}
/**
 * @brief Метод установки параметров авторизации
 *
 * @param settings параметры авторизации для установки
 */
void awh::client::Http::authorization(const client::auth_t::settings_t & settings) noexcept {
	// Выполняем установку параметров авторизации
	this->_auth.client.settings(settings);
}
/**
 * @brief Метод установки параметров авторизации
 *
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
 * @brief Метод установки типа авторизации
 *
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::client::Http::authType(const awh::auth_t::type_t type, const awh::auth_t::hash_t hash) noexcept {
	// Устанавливаем тип авторизации
	this->_auth.client.type(type, hash);
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::client::Http::Http(const fmk_t * fmk, const log_t * log) noexcept :
 awh::http_t(identity_t::HTTP, fmk, log) {
	// Устанавливаем тип HTTP-парсера
	this->_web.hid(web_t::hid_t::CLIENT);
}
