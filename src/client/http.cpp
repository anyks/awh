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
		reinterpret_cast <authCli_t *> (this->auth)->setUser(user);
		// Устанавливаем пароль пользователя
		reinterpret_cast <authCli_t *> (this->auth)->setPass(pass);
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
		reinterpret_cast <authCli_t *> (this->auth)->setType(type, alg);
}
/**
 * HttpClient Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 * @param uri объект работы с URI
 */
awh::HttpClient::HttpClient(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept : http_t(fmk, log, uri) {
	try {
		// Создаём объект для работы с авторизацией
		this->auth = new authCli_t(fmk, log);
	// Если происходит ошибка то игнорируем её
	} catch(const bad_alloc&) {
		// Выводим сообщение об ошибке
		log->print("%s", log_t::flag_t::CRITICAL, "memory could not be allocated");
		// Выходим из приложения
		exit(EXIT_FAILURE);
	}
}
/**
 * ~HttpClient Деструктор
 */
awh::HttpClient::~HttpClient() noexcept {
	// Удаляем объект авторизации
	if(this->auth != nullptr) delete this->auth;
}
