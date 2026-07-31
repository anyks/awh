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

/**
 * @brief Функция получения имени метода запроса для псевдо-заголовка [:method]
 *
 * @param request провайдер запроса клиента
 * @return        имя метода запроса (для UNKNOWN - оригинальное написание метода)
 *
 */
string_view awh::http::methodName(const http::request_t * request) noexcept {
	/**
	 * Определяем метод запроса клиента
	 */
	switch(static_cast <uint8_t> (request->method)){
		/**
		 * Основные методы (RFC 7231) + PATCH (RFC 5789)
		 */
		// Если метод запроса установлен как GET
		case static_cast <uint8_t> (http::method_t::GET):
			// Выводим название метода запроса
			return "GET";
		// Если метод запроса установлен как PUT
		case static_cast <uint8_t> (http::method_t::PUT):
			// Выводим название метода запроса
			return "PUT";
		// Если метод запроса установлен как DELETE
		case static_cast <uint8_t> (http::method_t::DEL):
			// Выводим название метода запроса
			return "DELETE";
		// Если метод запроса установлен как POST
		case static_cast <uint8_t> (http::method_t::POST):
			// Выводим название метода запроса
			return "POST";
		// Если метод запроса установлен как HEAD
		case static_cast <uint8_t> (http::method_t::HEAD):
			// Выводим название метода запроса
			return "HEAD";
		// Если метод запроса установлен как PATCH
		case static_cast <uint8_t> (http::method_t::PATCH):
			// Выводим название метода запроса
			return "PATCH";
		// Если метод запроса установлен как TRACE
		case static_cast <uint8_t> (http::method_t::TRACE):
			// Выводим название метода запроса
			return "TRACE";
		// Если метод запроса установлен как OPTIONS
		case static_cast <uint8_t> (http::method_t::OPTIONS):
			// Выводим название метода запроса
			return "OPTIONS";
		// Если метод запроса установлен как CONNECT
		case static_cast <uint8_t> (http::method_t::CONNECT):
			// Выводим название метода запроса
			return "CONNECT";
		/**
		 * WebDAV (RFC 4918) и расширения версионирования (RFC 3253)
		 */
		// Если метод запроса установлен как ACL
		case static_cast <uint8_t> (http::method_t::ACL):
			// Выводим название метода запроса
			return "ACL";
		// Если метод запроса установлен как COPY
		case static_cast <uint8_t> (http::method_t::COPY):
			// Выводим название метода запроса
			return "COPY";
		// Если метод запроса установлен как LOCK
		case static_cast <uint8_t> (http::method_t::LOCK):
			// Выводим название метода запроса
			return "LOCK";
		// Если метод запроса установлен как MOVE
		case static_cast <uint8_t> (http::method_t::MOVE):
			// Выводим название метода запроса
			return "MOVE";
		// Если метод запроса установлен как BIND
		case static_cast <uint8_t> (http::method_t::BIND):
			// Выводим название метода запроса
			return "BIND";
		// Если метод запроса установлен как MKCOL
		case static_cast <uint8_t> (http::method_t::MKCOL):
			// Выводим название метода запроса
			return "MKCOL";
		// Если метод запроса установлен как MERGE
		case static_cast <uint8_t> (http::method_t::MERGE):
			// Выводим название метода запроса
			return "MERGE";
		// Если метод запроса установлен как REPORT
		case static_cast <uint8_t> (http::method_t::REPORT):
			// Выводим название метода запроса
			return "REPORT";
		// Если метод запроса установлен как SEARCH
		case static_cast <uint8_t> (http::method_t::SEARCH):
			// Выводим название метода запроса
			return "SEARCH";
		// Если метод запроса установлен как UNLOCK
		case static_cast <uint8_t> (http::method_t::UNLOCK):
			// Выводим название метода запроса
			return "UNLOCK";
		// Если метод запроса установлен как REBIND
		case static_cast <uint8_t> (http::method_t::REBIND):
			// Выводим название метода запроса
			return "REBIND";
		// Если метод запроса установлен как UNBIND
		case static_cast <uint8_t> (http::method_t::UNBIND):
			// Выводим название метода запроса
			return "UNBIND";
		// Если метод запроса установлен как CHECKOUT
		case static_cast <uint8_t> (http::method_t::CHECKOUT):
			// Выводим название метода запроса
			return "CHECKOUT";
		// Если метод запроса установлен как PROPFIND
		case static_cast <uint8_t> (http::method_t::PROPFIND):
			// Выводим название метода запроса
			return "PROPFIND";
		// Если метод запроса установлен как PROPPATCH
		case static_cast <uint8_t> (http::method_t::PROPPATCH):
			// Выводим название метода запроса
			return "PROPPATCH";
		// Если метод запроса установлен как MKACTIVITY
		case static_cast <uint8_t> (http::method_t::MKACTIVITY):
			// Выводим название метода запроса
			return "MKACTIVITY";
		/**
		 * Прочие распространённые расширения
		 */
		// Если метод запроса установлен как PRI
		case static_cast <uint8_t> (http::method_t::PRI):
			// Выводим название метода запроса
			return "PRI";
		// Если метод запроса установлен как LINK
		case static_cast <uint8_t> (http::method_t::LINK):
			// Выводим название метода запроса
			return "LINK";
		// Если метод запроса установлен как PURGE
		case static_cast <uint8_t> (http::method_t::PURGE):
			// Выводим название метода запроса
			return "PURGE";
		// Если метод запроса установлен как NOTIFY
		case static_cast <uint8_t> (http::method_t::NOTIFY):
			// Выводим название метода запроса
			return "NOTIFY";
		// Если метод запроса установлен как UNLINK
		case static_cast <uint8_t> (http::method_t::UNLINK):
			// Выводим название метода запроса
			return "UNLINK";
		// Если метод запроса установлен как SOURCE
		case static_cast <uint8_t> (http::method_t::SOURCE):
			// Выводим название метода запроса
			return "SOURCE";
		// Если метод запроса установлен как M-SEARCH
		case static_cast <uint8_t> (http::method_t::MSEARCH):
			// Выводим название метода запроса
			return "M-SEARCH";
		// Если метод запроса установлен как SUBSCRIBE
		case static_cast <uint8_t> (http::method_t::SUBSCRIBE):
			// Выводим название метода запроса
			return "SUBSCRIBE";
		// Если метод запроса установлен как MKCALENDAR
		case static_cast <uint8_t> (http::method_t::MKCALENDAR):
			// Выводим название метода запроса
			return "MKCALENDAR";
		// Если метод запроса установлен как UNSUBSCRIBE
		case static_cast <uint8_t> (http::method_t::UNSUBSCRIBE):
			// Выводим название метода запроса
			return "UNSUBSCRIBE";
		// Нераспознанный метод - используем оригинальное написание
		case static_cast <uint8_t> (http::method_t::UNKNOWN):
			// Выводим оригинальное написание метода запроса
			return request->methodName;
	}
	// Метод запроса не установлен
	return "";
}
/**
 * @brief Функция разбора цели запроса, заданной в абсолютной форме (RFC 9112 §3.2.2)
 *
 * @param target    цель запроса
 * @param scheme    схема цели (представление в target)
 * @param authority авторитет цели (представление в target)
 * @param path      путь цели вместе со строкой запроса (представление в target)
 * @return          признак того, что цель задана в абсолютной форме
 *
 */
bool awh::http::splitTarget(const string_view target, string_view & scheme, string_view & authority, string_view & path) noexcept {
	// Выполняем сброс выдаваемых составляющих цели запроса
	scheme = string_view{};
	// Выполняем сброс авторитета цели запроса
	authority = string_view{};
	// Выполняем сброс пути цели запроса
	path = string_view{};
	// Выполняем поиск разделителя схемы: его наличие и означает абсолютную форму цели
	const size_t separator = target.find("://");
	/**
	 * Цель без разделителя схемы задана в происхождённой форме либо является
	 * целью метода CONNECT вида [host:port] - разбирать в ней нечего
	 */
	if(separator == string_view::npos)
		// Цель задана не в абсолютной форме
		return false;
	// Извлекаем схему цели запроса
	scheme = target.substr(0, separator);
	// Определяем начало компонента авторитета
	const size_t start = (separator + 3);
	/**
	 * Ищем конец компонента авторитета: им является первый из символов '/', '?'
	 * и '#' (RFC 3986 §3.2). Поиск одного лишь '/' уводил бы строку запроса
	 * внутрь авторитета, если путь в цели отсутствует
	 */
	const size_t border = target.find_first_of("/?#", start);
	// Если путь в цели запроса присутствует
	if(border != string_view::npos){
		// Извлекаем компонент авторитета цели запроса
		authority = target.substr(start, (border - start));
		// Извлекаем путь цели запроса вместе со строкой запроса
		path = target.substr(border);
	// Иначе авторитет занимает остаток цели, а путь в ней отсутствует
	} else authority = target.substr(start);
	// Цель задана в абсолютной форме
	return true;
}
/**
 * @brief Функция приведения пути цели запроса к виду псевдо-заголовка [:path]
 *
 * @param path   путь цели запроса
 * @param buffer буфер под дополненный путь
 * @return       путь, пригодный для псевдо-заголовка
 *
 */
string_view awh::http::targetPath(const string_view path, string & buffer) noexcept {
	// Если путь цели запроса отсутствует - псевдо-заголовок принимает корневой путь
	if(path.empty())
		// Выводим корневой путь запроса
		return "/";
	// Если путь начинается с разделителя - дополнять его незачем
	if(path.front() == '/')
		// Выводим путь цели запроса без изменений
		return path;
	/**
	 * Звёздочка - самостоятельная форма цели запроса: ею метод OPTIONS обращается
	 * к серверу целиком, а не к какому-либо его ресурсу (RFC 9112 §3.2.4). В таком
	 * запросе псевдо-заголовок пути обязан нести именно звёздочку (RFC 9113 §8.3.1,
	 * RFC 9114 §4.3.1), и дополнение разделителем превратило бы обращение к серверу
	 * в обращение к ресурсу с названием из звёздочки
	 */
	if(path == "*")
		// Выводим звёздочную форму цели запроса без изменений
		return path;
	/**
	 * Путь, начинающийся со строки запроса либо якоря, дополняется корневым
	 * разделителем: пустой и не начинающийся с '/' псевдо-заголовок запрещён
	 */
	buffer.assign(1, '/');
	// Дописываем в буфер сам путь цели запроса
	buffer.append(path);
	// Выводим дополненный путь цели запроса
	return buffer;
}
