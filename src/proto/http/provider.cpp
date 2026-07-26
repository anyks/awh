/**
 * @file: provider.cpp
 * @date: 2026-07-09
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация провайдеров HTTP-сообщений — формирование,
 *        хранение и сериализация структуры HTTP-запроса клиента и HTTP-ответа сервера поверх контейнера заголовков
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <proto/http/provider.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Конструктор
 *
 * @param direct направление трафика (запрос/ответ)
 *
 */
awh::http::Provider::Provider(const direct_t direct) noexcept :
 version(version_t::HTTP1_1), direct(direct) {}
/**
 * @brief Конструктор
 *
 * @param direct  направление трафика (запрос/ответ)
 * @param version версия протокола
 *
 */
awh::http::Provider::Provider(const direct_t direct, const version_t version) noexcept :
 version(version), direct(direct) {}

/**
 * @brief Метод клонирования объекта запроса
 *
 * @return копия объекта запроса
 *
 */
unique_ptr <awh::http::provider_t> awh::http::Request::clone() const noexcept {
	// Результат работы функции - копия объекта запроса
	unique_ptr <provider_t> result = nullptr;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Создаём копию текущего объекта запроса без срезки производной части
		result = make_unique <request_t> (* this);
	// Если возникает ошибка выделения памяти
	} catch(const exception &) {}
	// Выводим результат
	return result;
}
/**
 * @brief Оператор [=] перемещения параметров запроса клиента
 *
 * @param request объект параметров запроса клиента
 * @return        текущие параметры запроса клиента
 *
 */
awh::http::Request & awh::http::Request::operator = (request_t && request) noexcept {
	// Перемещаем параметры URI-запроса
	this->uri = ::move(request.uri);
	// Перемещаем оригинальное написание метода HTTP-запроса
	this->methodName = ::move(request.methodName);
	// Выполняем перемещение протокола расширенного метода CONNECT
	this->protocol = ::move(request.protocol);
	// Копируем метод HTTP-запроса
	this->method = request.method;
	// Копируем версию HTTP-протокола
	this->version = request.version;
	// Сбрасываем метод HTTP-запроса на дефолтный
	request.method = method_t::NONE;
	// Сбрасываем версию HTTP-протокола на дефолтную
	request.version = version_t::HTTP1_1;
	// Возвращаем текущий объект
	return (* this);
}
/**
 * @brief Оператор [=] присванивания параметров запроса клиента
 *
 * @param request объект параметров запроса клиента
 * @return        текущие параметры запроса клиента
 *
 */
awh::http::Request & awh::http::Request::operator = (const request_t & request) noexcept {
	// Копируем параметры URI-запроса
	this->uri = request.uri;
	// Копируем оригинальное написание метода HTTP-запроса
	this->methodName = request.methodName;
	// Выполняем копирование протокола расширенного метода CONNECT
	this->protocol = request.protocol;
	// Копируем метод HTTP-запроса
	this->method = request.method;
	// Копируем версию HTTP-протокола
	this->version = request.version;
	// Возвращаем текущий объект
	return (* this);
}
/**
 * @brief Оператор [==] сравнения
 *
 * @param request объект параметров запроса клиента
 * @return        результат сравнения
 *
 */
bool awh::http::Request::operator == (const request_t & request) const noexcept {
	// Выполняем сравнение параметров запроса клиента
	return (
		(this->method == request.method) &&
		(this->version == request.version) &&
		(this->uri == request.uri) &&
		(this->methodName == request.methodName) &&
		(this->protocol == request.protocol)
	);
}
/**
 * @brief Оператор [!=] сравнения
 *
 * @param request объект параметров запроса клиента
 * @return        результат сравнения
 *
 */
bool awh::http::Request::operator != (const request_t & request) const noexcept {
	// Выполняем сравнение параметров запроса клиента
	return !((* this) == request);
}
/**
 * @brief Конструктор перемещения
 *
 * @param request объект параметров запроса клиента
 *
 */
awh::http::Request::Request(request_t && request) noexcept : provider_t(direct_t::REQUEST) {
	// Перемещаем параметры URI-запроса
	this->uri = ::move(request.uri);
	// Перемещаем оригинальное написание метода HTTP-запроса
	this->methodName = ::move(request.methodName);
	// Выполняем перемещение протокола расширенного метода CONNECT
	this->protocol = ::move(request.protocol);
	// Копируем метод HTTP-запроса
	this->method = request.method;
	// Копируем версию HTTP-протокола
	this->version = request.version;
	// Сбрасываем метод HTTP-запроса на дефолтный
	request.method = method_t::NONE;
	// Сбрасываем версию HTTP-протокола на дефолтную
	request.version = version_t::HTTP1_1;
}
/**
 * @brief Конструктор копирования
 *
 * @param request объект параметров запроса клиента
 *
 */
awh::http::Request::Request(const request_t & request) noexcept : provider_t(direct_t::REQUEST) {
	// Копируем параметры URI-запроса
	this->uri = request.uri;
	// Копируем оригинальное написание метода HTTP-запроса
	this->methodName = request.methodName;
	// Выполняем копирование протокола расширенного метода CONNECT
	this->protocol = request.protocol;
	// Копируем метод HTTP-запроса
	this->method = request.method;
	// Копируем версию HTTP-протокола
	this->version = request.version;
}
/**
 * @brief Конструктор
 *
 */
awh::http::Request::Request() noexcept :
 provider_t(direct_t::REQUEST, version_t::HTTP1_1), uri{""}, method(method_t::NONE), methodName{""}, protocol{""} {}
/**
 * @brief Конструктор
 *
 * @param uri параметры URI-запроса
 *
 */
awh::http::Request::Request(const string & uri) noexcept :
 provider_t(direct_t::REQUEST, version_t::HTTP1_1), uri{uri}, method(method_t::NONE), methodName{""}, protocol{""} {}
/**
 * @brief Конструктор
 *
 * @param method метод запроса клиента
 *
 */
awh::http::Request::Request(const method_t method) noexcept :
 provider_t(direct_t::REQUEST, version_t::HTTP1_1), uri{""}, method(method), methodName{""}, protocol{""} {}
/**
 * @brief Конструктор
 *
 * @param version версия протокола
 *
 */
awh::http::Request::Request(const version_t version) noexcept :
 provider_t(direct_t::REQUEST, version), uri{""}, method(method_t::NONE), methodName{""}, protocol{""} {}
/**
 * @brief Конструктор
 *
 * @param method метод запроса клиента
 * @param uri    параметры URI-запроса
 *
 */
awh::http::Request::Request(const method_t method, const string & uri) noexcept :
 provider_t(direct_t::REQUEST, version_t::HTTP1_1), uri{uri}, method(method), methodName{""}, protocol{""} {}
/**
 * @brief Конструктор
 *
 * @param version версия протокола
 * @param uri     параметры URI-запроса
 *
 */
awh::http::Request::Request(const version_t version, const string & uri) noexcept :
 provider_t(direct_t::REQUEST, version), uri{uri}, method(method_t::NONE), methodName{""}, protocol{""} {}
/**
 * @brief Конструктор
 *
 * @param version версия протокола
 * @param method  метод запроса клиента
 *
 */
awh::http::Request::Request(const version_t version, const method_t method) noexcept :
 provider_t(direct_t::REQUEST, version), uri{""}, method(method), methodName{""}, protocol{""} {}
/**
 * @brief Конструктор
 *
 * @param version версия протокола
 * @param method  метод запроса клиента
 * @param uri     параметры URI-запроса
 *
 */
awh::http::Request::Request(const version_t version, const method_t method, const string & uri) noexcept :
 provider_t(direct_t::REQUEST, version), uri{uri}, method(method), methodName{""}, protocol{""} {}

/**
 * @brief Метод клонирования объекта ответа
 *
 * @return копия объекта ответа
 *
 */
unique_ptr <awh::http::provider_t> awh::http::Response::clone() const noexcept {
	// Результат работы функции - копия объекта ответа
	unique_ptr <provider_t> result = nullptr;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Создаём копию текущего объекта ответа без срезки производной части
		result = make_unique <response_t> (* this);
	// Если возникает ошибка выделения памяти
	} catch(const exception &) {}
	// Выводим результат
	return result;
}
/**
 * @brief Оператор [=] перемещения параметров ответа сервера
 *
 * @param response объект параметров ответа сервера
 * @return         текущие параметры ответа сервера
 *
 */
awh::http::Response & awh::http::Response::operator = (response_t && response) noexcept {
	// Перемещаем сообщение сервера
	this->message = ::move(response.message);
	// Копируем код ответа сервера
	this->code = response.code;
	// Копируем версию HTTP-протокола
	this->version = response.version;
	// Сбрасываем код ответа сервера на дефолтный
	response.code = 0;
	// Сбрасываем версию HTTP-протокола на дефолтную
	response.version = version_t::HTTP1_1;
	// Возвращаем текущий объект
	return (* this);
}
/**
 * @brief Оператор [=] присванивания параметров ответа сервера
 *
 * @param response объект параметров ответа сервера
 * @return         текущие параметры ответа сервера
 *
 */
awh::http::Response & awh::http::Response::operator = (const response_t & response) noexcept {
	// Копируем сообщение сервера
	this->message = response.message;
	// Копируем код ответа сервера
	this->code = response.code;
	// Копируем версию HTTP-протокола
	this->version = response.version;
	// Возвращаем текущий объект
	return (* this);
}
/**
 * @brief Оператор [==] сравнения
 *
 * @param response объект параметров ответа сервера
 * @return         результат сравнения
 *
 */
bool awh::http::Response::operator == (const response_t & response) const noexcept {
	// Выполняем сравнение параметров ответа сервера
	return (
		(this->code == response.code) &&
		(this->version == response.version) &&
		(this->message == response.message)
	);
}
/**
 * @brief Оператор [!=] сравнения
 *
 * @param response объект параметров ответа сервера
 * @return         результат сравнения
 *
 */
bool awh::http::Response::operator != (const response_t & response) const noexcept {
	// Выполняем сравнение параметров ответа сервера
	return !((* this) == response);
}
/**
 * @brief Конструктор перемещения
 *
 * @param response объект параметров ответа сервера
 *
 */
awh::http::Response::Response(response_t && response) noexcept : provider_t(direct_t::RESPONSE, version_t::HTTP1_1) {
	// Перемещаем сообщение сервера
	this->message = ::move(response.message);
	// Копируем код ответа сервера
	this->code = response.code;
	// Копируем версию HTTP-протокола
	this->version = response.version;
	// Сбрасываем код ответа сервера на дефолтный
	response.code = 0;
	// Сбрасываем версию HTTP-протокола на дефолтную
	response.version = version_t::HTTP1_1;
}
/**
 * @brief Конструктор копирования
 *
 * @param response объект параметров ответа сервера
 *
 */
awh::http::Response::Response(const response_t & response) noexcept : provider_t(direct_t::RESPONSE, version_t::HTTP1_1) {
	// Копируем сообщение сервера
	this->message = response.message;
	// Копируем код ответа сервера
	this->code = response.code;
	// Копируем версию HTTP-протокола
	this->version = response.version;
}
/**
 * @brief Конструктор
 *
 */
awh::http::Response::Response() noexcept :
 provider_t(direct_t::RESPONSE, version_t::HTTP1_1), code(0), message{""} {}
/**
 * @brief Конструктор
 *
 * @param code код ответа сервера
 *
 */
awh::http::Response::Response(const uint16_t code) noexcept :
 provider_t(direct_t::RESPONSE, version_t::HTTP1_1), code(code), message{""} {}
/**
 * @brief Конструктор
 *
 * @param message сообщение сервера
 *
 */
awh::http::Response::Response(const string & message) noexcept :
 provider_t(direct_t::RESPONSE, version_t::HTTP1_1), code(0), message{message} {}
/**
 * @brief Конструктор
 *
 * @param version версия протокола
 *
 */
awh::http::Response::Response(const version_t version) noexcept :
 provider_t(direct_t::RESPONSE, version), code(0), message{""} {}
/**
 * @brief Конструктор
 *
 * @param code    код ответа сервера
 * @param message сообщение сервера
 *
 */
awh::http::Response::Response(const uint16_t code, const string & message) noexcept :
 provider_t(direct_t::RESPONSE, version_t::HTTP1_1), code(code), message{message} {}
/**
 * @brief Конструктор
 *
 * @param version версия протокола
 * @param code    код ответа сервера
 *
 */
awh::http::Response::Response(const version_t version, const uint16_t code) noexcept :
 provider_t(direct_t::RESPONSE, version), code(code), message{""} {}
/**
 * @brief Конструктор
 *
 * @param version версия протокола
 * @param message сообщение сервера
 *
 */
awh::http::Response::Response(const version_t version, const string & message) noexcept :
 provider_t(direct_t::RESPONSE, version), code(0), message{message} {}
/**
 * @brief Конструктор
 *
 * @param version версия протокола
 * @param code    код ответа сервера
 * @param message сообщение сервера
 *
 */
awh::http::Response::Response(const version_t version, const uint16_t code, const string & message) noexcept :
 provider_t(direct_t::RESPONSE, version), code(code), message{message} {}
