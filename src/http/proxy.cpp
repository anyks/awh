/**
 * @file: proxy.cpp
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
#include <http/proxy.hpp>

/**
 * checkAuth Метод проверки авторизации
 * @return результат проверки авторизации
 */
awh::Http::stath_t awh::HttpProxy::checkAuth() noexcept {
	// Результат работы функции
	stath_t result = stath_t::FAULT;
	// Если авторизация требуется
	if(this->auth.server.type() != awh::auth_t::type_t::NONE){
		// Получаем параметры авторизации
		const string & auth = this->web.header("proxy-authorization");
		// Если параметры авторизации найдены
		if(!auth.empty()){
			// Метод HTTP запроса
			string method = "";
			// Устанавливаем заголовок HTTP в параметры авторизации
			this->auth.server.header(auth);
			// Определяем метод запроса
			switch(static_cast <uint8_t> (this->web.request().method)){
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
			if(this->auth.server.check(method))
				// Устанавливаем успешный результат авторизации
				result = http_t::stath_t::GOOD;
		}
	// Сообщаем, что авторизация прошла успешно
	} else result = http_t::stath_t::GOOD;
	// Выводим результат
	return result;
}
/**
 * realm Метод установки название сервера
 * @param realm название сервера
 */
void awh::HttpProxy::realm(const string & realm) noexcept {
	// Если название сервера передано
	if(!realm.empty())
		// Выполняем установку названия сервера
		this->auth.server.realm(realm);
}
/**
 * opaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::HttpProxy::opaque(const string & opaque) noexcept {
	// Если временный ключ сессии сервера передан
	if(!opaque.empty())
		// Выполняем установку ключа сессии
		this->auth.server.opaque(opaque);
}
/**
 * extractPassCallback Метод добавления функции извлечения пароля
 * @param callback функция обратного вызова для извлечения пароля
 */
void awh::HttpProxy::extractPassCallback(function <string (const string &)> callback) noexcept {
	// Устанавливаем внешнюю функцию
	this->auth.server.extractPassCallback(callback);
}
/**
 * authCallback Метод добавления функции обработки авторизации
 * @param callback функция обратного вызова для обработки авторизации
 */
void awh::HttpProxy::authCallback(function <bool (const string &, const string &)> callback) noexcept {
	// Устанавливаем внешнюю функцию
	this->auth.server.authCallback(callback);
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::HttpProxy::authType(const awh::auth_t::type_t type, const awh::auth_t::hash_t hash) noexcept {
	// Устанавливаем тип авторизации
	this->auth.server.type(type, hash);
}
/**
 * HttpProxy Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::HttpProxy::HttpProxy(const fmk_t * fmk, const log_t * log) noexcept : http_t(fmk, log) {
	// Устанавливаем тип HTTP парсера
	this->web.init(web_t::hid_t::SERVER);
	// Устанавливаем тип HTTP модуля
	this->httpType = web_t::hid_t::SERVER;
}
