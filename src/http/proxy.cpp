/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
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
	if(this->authSrv.getType() != auth_t::type_t::NONE){
		// Получаем параметры авторизации
		const string & auth = this->web.getHeader("proxy-authorization");
		// Если параметры авторизации найдены
		if(!auth.empty()){
			// Метод HTTP запроса
			string method = "";
			// Устанавливаем заголовок HTTP в параметры авторизации
			this->authSrv.setHeader(auth);
			// Определяем метод запроса
			switch((uint8_t) this->web.getQuery().method){
				// Если метод запроса указан как GET
				case (uint8_t) web_t::method_t::GET: method = "get"; break;
				// Если метод запроса указан как PUT
				case (uint8_t) web_t::method_t::PUT: method = "put"; break;
				// Если метод запроса указан как POST
				case (uint8_t) web_t::method_t::POST: method = "post"; break;
				// Если метод запроса указан как HEAD
				case (uint8_t) web_t::method_t::HEAD: method = "head"; break;
				// Если метод запроса указан как PATCH
				case (uint8_t) web_t::method_t::PATCH: method = "patch"; break;
				// Если метод запроса указан как TRACE
				case (uint8_t) web_t::method_t::TRACE: method = "trace"; break;
				// Если метод запроса указан как DELETE
				case (uint8_t) web_t::method_t::DEL: method = "delete"; break;
				// Если метод запроса указан как OPTIONS
				case (uint8_t) web_t::method_t::OPTIONS: method = "options"; break;
				// Если метод запроса указан как CONNECT
				case (uint8_t) web_t::method_t::CONNECT: method = "connect"; break;
			}
			// Выполняем проверку авторизации
			if(this->authSrv.check(method))
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
void awh::HttpProxy::setRealm(const string & realm) noexcept {
	// Если название сервера передано
	if(!realm.empty()) this->authSrv.setRealm(realm);
}
/**
 * setOpaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::HttpProxy::setOpaque(const string & opaque) noexcept {
	// Если временный ключ сессии сервера передан
	if(!opaque.empty()) this->authSrv.setOpaque(opaque);
}
/**
 * setExtractPassCallback Метод добавления функции извлечения пароля
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова для извлечения пароля
 */
void awh::HttpProxy::setExtractPassCallback(void * ctx, function <string (const string &, void *)> callback) noexcept {
	// Устанавливаем внешнюю функцию
	this->authSrv.setExtractPassCallback(ctx, callback);
}
/**
 * setAuthCallback Метод добавления функции обработки авторизации
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова для обработки авторизации
 */
void awh::HttpProxy::setAuthCallback(void * ctx, function <bool (const string &, const string &, void *)> callback) noexcept {
	// Устанавливаем внешнюю функцию
	this->authSrv.setAuthCallback(ctx, callback);
}
/**
 * setAuthType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::HttpProxy::setAuthType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Устанавливаем тип авторизации
	this->authSrv.setType(type, hash);
}
/**
 * HttpProxy Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 * @param uri объект работы с URI
 */
awh::HttpProxy::HttpProxy(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept : http_t(fmk, log, uri) {
	// Устанавливаем тип HTTP модуля
	this->web.init(web_t::hid_t::SERVER);
}
