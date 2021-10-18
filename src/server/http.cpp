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
 * checkAuth Метод проверки авторизации
 * @return результат проверки авторизации
 */
awh::Http::stath_t awh::HttpServer::checkAuth() noexcept {
	// Результат работы функции
	stath_t result = stath_t::FAULT;
	// Если авторизация требуется
	if(this->authSrv.getType() != auth_t::type_t::NONE){
		// Получаем параметры авторизации
		auto it = this->headers.find("authorization");
		// Если параметры авторизации найдены
		if(it != this->headers.end()){
			// Устанавливаем заголовок HTTP в параметры авторизации
			this->authSrv.setHeader(it->second);
			// Выполняем проверку авторизации
			if(this->authSrv.check())
				// Устанавливаем успешный результат авторизации
				result = http_t::stath_t::GOOD;
		}
	// Сообщаем, что авторизация прошла успешно
	} else result = http_t::stath_t::GOOD;
	// Выводим результат
	return result;
}
/**
 * setRealm Метод установки название сервера
 * @param realm название сервера
 */
void awh::HttpServer::setRealm(const string & realm) noexcept {
	// Если название сервера передано
	if(!realm.empty()) this->authSrv.setRealm(realm);
}
/**
 * setOpaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::HttpServer::setOpaque(const string & opaque) noexcept {
	// Если временный ключ сессии сервера передан
	if(!opaque.empty()) this->authSrv.setOpaque(opaque);
}
/**
 * setExtractPassCallback Метод добавления функции извлечения пароля
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова для извлечения пароля
 */
void awh::HttpServer::setExtractPassCallback(void * ctx, function <string (const string &, void *)> callback) noexcept {
	// Устанавливаем внешнюю функцию
	this->authSrv.setExtractPassCallback(ctx, callback);
}
/**
 * setAuthCallback Метод добавления функции обработки авторизации
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова для обработки авторизации
 */
void awh::HttpServer::setAuthCallback(void * ctx, function <bool (const string &, const string &, void *)> callback) noexcept {
	// Устанавливаем внешнюю функцию
	this->authSrv.setAuthCallback(ctx, callback);
}
/**
 * setAuthType Метод установки типа авторизации
 * @param type тип авторизации
 * @param alg  алгоритм шифрования для Digest авторизации
 */
void awh::HttpServer::setAuthType(const auth_t::type_t type, const auth_t::alg_t alg) noexcept {
	// Устанавливаем тип авторизации
	this->authSrv.setType(type, alg);
}
