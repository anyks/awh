/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <server/http.hpp>

/**
 * setRealm Метод установки название сервера
 * @param realm название сервера
 */
void awh::HttpServer::setRealm(const string & realm) noexcept {
	// Если название сервера передано
	if(!realm.empty()) reinterpret_cast <authSrv_t *> (this->auth)->setRealm(realm);
}
/**
 * setNonce Метод установки уникального ключа клиента выданного сервером
 * @param nonce уникальный ключ клиента
 */
void awh::HttpServer::setNonce(const string & nonce) noexcept {
	// Если уникальный ключ клиента передан
	if(!nonce.empty()) reinterpret_cast <authSrv_t *> (this->auth)->setNonce(nonce);
}
/**
 * setOpaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::HttpServer::setOpaque(const string & opaque) noexcept {
	// Если временный ключ сессии сервера передан
	if(!opaque.empty()) reinterpret_cast <authSrv_t *> (this->auth)->setOpaque(opaque);
}
/**
 * setExtractPassCallback Метод добавления функции извлечения пароля
 * @param callback функция обратного вызова для извлечения пароля
 */
void awh::HttpServer::setExtractPassCallback(function <string (const string &)> callback) noexcept {
	// Устанавливаем внешнюю функцию
	reinterpret_cast <authSrv_t *> (this->auth)->setExtractPassCallback(callback);
}
/**
 * setAuthCallback Метод добавления функции обработки авторизации
 * @param callback функция обратного вызова для обработки авторизации
 */
void awh::HttpServer::setAuthCallback(function <bool (const string &, const string &)> callback) noexcept {
	// Устанавливаем внешнюю функцию
	reinterpret_cast <authSrv_t *> (this->auth)->setAuthCallback(callback);
}
/**
 * HttpServer Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 * @param uri объект работы с URI
 */
awh::HttpServer::HttpServer(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept : http_t(fmk, log, uri) {
	try {
		// Создаём объект для работы с авторизацией
		this->auth = new authSrv_t(fmk, log);
	// Если происходит ошибка то игнорируем её
	} catch(const bad_alloc&) {
		// Выводим сообщение об ошибке
		log->print("%s", log_t::flag_t::CRITICAL, "memory could not be allocated");
		// Выходим из приложения
		exit(EXIT_FAILURE);
	}
}
/**
 * ~HttpServer Деструктор
 */
awh::HttpServer::~HttpServer() noexcept {
	// Удаляем объект авторизации
	if(this->auth != nullptr) delete this->auth;
}
