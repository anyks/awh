/**
 * @file: server.cpp
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
#include <http/server2.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Метод проверки выполнения рукопожатия
 *
 * @return результат выполнения рукопожатия
 */
awh::Http::handshake_t awh::server::Http::handshake() noexcept {
	// Результат работы функции
	handshake_t result = handshake_t::FAULT;
	// Если авторизация требуется
	if(this->_auth.server.type() != awh::auth_t::type_t::NONE){
		// Параметры авторизации
		string auth = "";
		/**
		 * Определяем идентичность сервера
		 */
		switch(static_cast <uint8_t> (this->_session.identity)){
			// Если сервер соответствует WebSocket-серверу
			case static_cast <uint8_t> (identity_t::WS):
			// Если сервер соответствует HTTP-серверу
			case static_cast <uint8_t> (identity_t::HTTP):
				// Получаем параметры авторизации
				auth = this->_web.header("Authorization");
			break;
			// Если сервер соответствует PROXY-серверу
			case static_cast <uint8_t> (identity_t::PROXY):
				// Получаем параметры авторизации
				auth = this->_web.header("Proxy-Authorization");
			break;
		}
		// Если параметры авторизации найдены
		if(!auth.empty()){
			// Метод HTTP запроса
			string method = "";
			// Устанавливаем заголовок HTTP в параметры авторизации
			this->_auth.server.header(auth);
			/**
			 * Определяем метод запроса
			 */
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
				result = handshake_t::GOOD;
		}
	// Сообщаем, что авторизация прошла успешно
	} else result = handshake_t::GOOD;
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки название сервера
 *
 * @param realm название сервера
 */
void awh::server::Http::realm(const string & realm) noexcept {
	// Если название сервера передано
	if(!realm.empty())
		// Устанавливаем название сервера
		this->_auth.server.realm(realm);
}
/**
 * @brief Метод установки временного ключа сессии сервера
 *
 * @param opaque временный ключ сессии сервера
 */
void awh::server::Http::opaque(const string & opaque) noexcept {
	// Если временный ключ сессии сервера передан
	if(!opaque.empty())
		// Устанавливаем временный ключ сессии
		this->_auth.server.opaque(opaque);
}
/**
 * @brief Метод извлечения параметров авторизации
 *
 * @return параметры модуля авторизации
 */
awh::server::auth_t::settings_t awh::server::Http::authorization() const noexcept {
	// Выполняем извлечение параметров авторизации
	return this->_auth.server.settings();
}
/**
 * @brief Метод установки параметров авторизации
 *
 * @param settings параметры авторизации для установки
 */
void awh::server::Http::authorization(const server::auth_t::settings_t & settings) noexcept {
	// Выполняем установку параметров авторизации
	this->_auth.server.settings(settings);
}
/**
 * @brief Метод добавления функции извлечения пароля
 *
 * @param callback функция обратного вызова для извлечения пароля
 */
void awh::server::Http::authCallback(function <string (const string &)> callback) noexcept {
	// Устанавливаем внешнюю функцию
	this->_auth.server.callback(::move(callback));
}
/**
 * @brief Метод добавления функции обработки авторизации
 *
 * @param callback функция обратного вызова для обработки авторизации
 */
void awh::server::Http::authCallback(function <bool (const string &, const string &)> callback) noexcept {
	// Устанавливаем внешнюю функцию
	this->_auth.server.callback(::move(callback));
}
/**
 * @brief Метод установки типа авторизации
 *
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::server::Http::authType(const awh::auth_t::type_t type, const awh::auth_t::hash_t hash) noexcept {
	// Устанавливаем тип авторизации
	this->_auth.server.type(type, hash);
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::Http::Http(const fmk_t * fmk, const log_t * log) noexcept :
 awh::http_t(identity_t::HTTP, fmk, log) {
	// Устанавливаем тип HTTP-парсера
	this->_web.hid(web_t::hid_t::SERVER);
}
