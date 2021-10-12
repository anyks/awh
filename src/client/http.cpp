/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <client/http.hpp>

/**
 * setUser Метод установки параметров авторизации
 * @param user логин пользователя для авторизации на сервере
 * @param pass пароль пользователя для авторизации на сервере
 */
void awh::HttpClient::setUser(const string & user, const string & pass) noexcept {
	// Если пользователь и пароль переданы
	if(!user.empty() && !pass.empty()){
		// Устанавливаем логин пользователя
		reinterpret_cast <authCli_t *> (this->auth.get())->setUser(user);
		// Устанавливаем пароль пользователя
		reinterpret_cast <authCli_t *> (this->auth.get())->setPass(pass);
	}
}
/**
 * setAuthType Метод установки типа авторизации
 * @param type тип авторизации
 * @param alg  алгоритм шифрования для Digest авторизации
 */
void awh::HttpClient::setAuthType(const auth_t::type_t type, const auth_t::alg_t alg) noexcept {
	// Если объект авторизации создан
	if(this->auth != nullptr)
		// Устанавливаем тип авторизации
		reinterpret_cast <authCli_t *> (this->auth.get())->setType(type, alg);
}
/**
 * HttpClient Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 * @param uri объект работы с URI
 */
awh::HttpClient::HttpClient(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept : http_t(fmk, log, uri) {
	// Создаём объект для работы с авторизацией
	this->auth = unique_ptr <auth_t> (new authCli_t(fmk, log));
}
