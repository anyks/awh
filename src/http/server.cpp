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
 * @copyright: Copyright © 2021
 */

// Подключаем заголовочный файл
#include <http/server.hpp>

/**
 * status Метод проверки текущего статуса
 * @return результат проверки текущего статуса
 */
awh::Http::status_t awh::server::Http::status() noexcept {
	// Результат работы функции
	status_t result = status_t::FAULT;
	// Если авторизация требуется
	if(this->_auth.server.type() != awh::auth_t::type_t::NONE){
		// Параметры авторизации
		string auth = "";
		// Определяем идентичность сервера
		switch(static_cast <uint8_t> (this->_identity)){
			// Если сервер соответствует WebSocket-серверу
			case static_cast <uint8_t> (identity_t::WS):
			// Если сервер соответствует HTTP-серверу
			case static_cast <uint8_t> (identity_t::HTTP):
				// Получаем параметры авторизации
				auth = this->_web.header("authorization");
			break;
			// Если сервер соответствует PROXY-серверу
			case static_cast <uint8_t> (identity_t::PROXY):
				// Получаем параметры авторизации
				auth = this->_web.header("proxy-authorization");
			break;
		}
		// Если параметры авторизации найдены
		if(!auth.empty()){
			// Метод HTTP запроса
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
				// Если метод запроса указан как PUT
				case static_cast <uint8_t> (web_t::method_t::PUT):
					// Устанавливаем метод запроса
					method = "put";
				break;
				// Если метод запроса указан как POST
				case static_cast <uint8_t> (web_t::method_t::POST):
					// Устанавливаем метод запроса
					method = "post";
				break;
				// Если метод запроса указан как HEAD
				case static_cast <uint8_t> (web_t::method_t::HEAD):
					// Устанавливаем метод запроса
					method = "head";
				break;
				// Если метод запроса указан как DELETE
				case static_cast <uint8_t> (web_t::method_t::DEL):
					// Устанавливаем метод запроса
					method = "delete";
				break;
				// Если метод запроса указан как PATCH
				case static_cast <uint8_t> (web_t::method_t::PATCH):
					// Устанавливаем метод запроса
					method = "patch";
				break;
				// Если метод запроса указан как TRACE
				case static_cast <uint8_t> (web_t::method_t::TRACE):
					// Устанавливаем метод запроса
					method = "trace";
				break;
				// Если метод запроса указан как OPTIONS
				case static_cast <uint8_t> (web_t::method_t::OPTIONS):
					// Устанавливаем метод запроса
					method = "options";
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
	// Сообщаем, что авторизация прошла успешно
	} else result = http_t::status_t::GOOD;
	// Выводим результат
	return result;
}
/**
 * realm Метод установки название сервера
 * @param realm название сервера
 */
void awh::server::Http::realm(const string & realm) noexcept {
	// Если название сервера передано
	if(!realm.empty())
		// Устанавливаем название сервера
		this->_auth.server.realm(realm);
}
/**
 * opaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::server::Http::opaque(const string & opaque) noexcept {
	// Если временный ключ сессии сервера передан
	if(!opaque.empty())
		// Устанавливаем временный ключ сессии
		this->_auth.server.opaque(opaque);
}
/**
 * extractPassCallback Метод добавления функции извлечения пароля
 * @param callback функция обратного вызова для извлечения пароля
 */
void awh::server::Http::extractPassCallback(function <string (const string &)> callback) noexcept {
	// Устанавливаем внешнюю функцию
	this->_auth.server.extractPassCallback(callback);
}
/**
 * authCallback Метод добавления функции обработки авторизации
 * @param callback функция обратного вызова для обработки авторизации
 */
void awh::server::Http::authCallback(function <bool (const string &, const string &)> callback) noexcept {
	// Устанавливаем внешнюю функцию
	this->_auth.server.authCallback(callback);
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::server::Http::authType(const awh::auth_t::type_t type, const awh::auth_t::hash_t hash) noexcept {
	// Устанавливаем тип авторизации
	this->_auth.server.type(type, hash);
}
/**
 * Http Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::Http::Http(const fmk_t * fmk, const log_t * log) noexcept : awh::http_t(fmk, log) {
	// Выполняем установку идентичность сервера к протоколу HTTP
	this->_identity = identity_t::HTTP;
	// Устанавливаем тип HTTP-парсера
	this->_web.hid(web_t::hid_t::SERVER);
}
