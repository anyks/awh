/**
 * @file: awh.cpp
 * @date: 2023-09-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2023
 */

// Подключаем заголовочный файл
#include <client/awh.hpp>

/**
 * sendTimeout Метод отправки сигнала таймаута
 */
void awh::client::AWH::sendTimeout() noexcept {
	// Выполняем отправку сигнала таймаута
	this->_http.sendTimeout();
}
/**
 * sendError Метод отправки сообщения об ошибке на сервер WebSocket
 * @param mess отправляемое сообщение об ошибке
 */
void awh::client::AWH::sendError(const ws::mess_t & mess) noexcept {
	// Выполняем отправку сообщения об ошибке
	this->_http.sendError(mess);
}
/**
 * send Метод отправки сообщения на сервер HTTP/2
 * @param agent   агент воркера
 * @param request параметры запроса на удалённый сервер
 * @return        идентификатор отправленного запроса
 */
int32_t awh::client::AWH::send(const web_t::agent_t agent, const web_t::request_t & request) noexcept {
	// Выполняем отправку сообщения на удалённый сервер
	return this->_http.send(agent, request);
}
/**
 * send Метод отправки сообщения на сервер WebSocket
 * @param message буфер сообщения в бинарном виде
 * @param size    размер сообщения в байтах
 * @param text    данные передаются в текстовом виде
 */
void awh::client::AWH::send(const char * message, const size_t size, const bool text) noexcept {
	// Выполняем отправку сообщения на WebSocket-сервер
	this->_http.send(message, size, text);
}
/**
 * pause Метод установки на паузу клиента WebSocket
 */
void awh::client::AWH::pause() noexcept {
	// Выполняем постановку клиента WebSocket на паузу
	this->_http.pause();
}
/**
 * init Метод инициализации клиента
 * @param dest     адрес назначения удалённого сервера
 * @param compress метод компрессии передаваемых сообщений
 */
void awh::client::AWH::init(const string & dest, const awh::http_t::compress_t compress) noexcept {
	// Выполняем инициализацию клиента
	this->_http.init(dest, compress);
}
/**
 * GET Метод запроса в формате HTTP методом GET
 * @param url     адрес запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::AWH::GET(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> result;
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::GET, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * DEL Метод запроса в формате HTTP методом DEL
 * @param url     адрес запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::AWH::DEL(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> result;
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::DEL, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * PUT Метод запроса в формате HTTP методом PUT
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::AWH::PUT(const uri_t::url_t & url, const json & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Получаем тело запроса
	const string body = entity.dump();
	// Устанавливаем тепло запроса
	vector <char> result(body.begin(), body.end());
	// Добавляем заголовок типа контента
	const_cast <unordered_multimap <string, string> *> (&headers)->emplace("Content-Type", "application/json");
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::PUT, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * PUT Метод запроса в формате HTTP методом PUT
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::AWH::PUT(const uri_t::url_t & url, const vector <char> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> result = std::forward <const vector <char>> (entity);
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::PUT, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * PUT Метод запроса в формате HTTP методом PUT
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::AWH::PUT(const uri_t::url_t & url, const unordered_multimap <string, string> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Тело в формате X-WWW-Form-Urlencoded
	string body = "";
	// Переходим по всему списку тела запроса
	for(auto & param : entity){
		// Есди данные уже набраны
		if(!body.empty()) body.append("&");
		// Добавляем в список набор параметров
		body.append(this->_uri.encode(param.first));
		// Добавляем разделитель
		body.append("=");
		// Добавляем значение
		body.append(this->_uri.encode(param.second));
	}
	// Устанавливаем тепло запроса
	vector <char> result(body.begin(), body.end());
	// Добавляем заголовок типа контента
	const_cast <unordered_multimap <string, string> *> (&headers)->emplace("Content-Type", "application/x-www-form-urlencoded");
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::PUT, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * POST Метод запроса в формате HTTP методом POST
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::AWH::POST(const uri_t::url_t & url, const json & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Получаем тело запроса
	const string body = entity.dump();
	// Устанавливаем тепло запроса
	vector <char> result(body.begin(), body.end());
	// Добавляем заголовок типа контента
	const_cast <unordered_multimap <string, string> *> (&headers)->emplace("Content-Type", "application/json");
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::POST, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * POST Метод запроса в формате HTTP методом POST
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::AWH::POST(const uri_t::url_t & url, const vector <char> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> result = std::forward <const vector <char>> (entity);
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::POST, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * POST Метод запроса в формате HTTP методом POST
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::AWH::POST(const uri_t::url_t & url, const unordered_multimap <string, string> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Тело в формате X-WWW-Form-Urlencoded
	string body = "";
	// Переходим по всему списку тела запроса
	for(auto & param : entity){
		// Есди данные уже набраны
		if(!body.empty()) body.append("&");
		// Добавляем в список набор параметров
		body.append(this->_uri.encode(param.first));
		// Добавляем разделитель
		body.append("=");
		// Добавляем значение
		body.append(this->_uri.encode(param.second));
	}
	// Устанавливаем тепло запроса
	vector <char> result(body.begin(), body.end());
	// Добавляем заголовок типа контента
	const_cast <unordered_multimap <string, string> *> (&headers)->emplace("Content-Type", "application/x-www-form-urlencoded");
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::POST, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * PATCH Метод запроса в формате HTTP методом PATCH
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::AWH::PATCH(const uri_t::url_t & url, const json & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Получаем тело запроса
	const string body = entity.dump();
	// Устанавливаем тепло запроса
	vector <char> result(body.begin(), body.end());
	// Добавляем заголовок типа контента
	const_cast <unordered_multimap <string, string> *> (&headers)->emplace("Content-Type", "application/json");
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::PATCH, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * PATCH Метод запроса в формате HTTP методом PATCH
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::AWH::PATCH(const uri_t::url_t & url, const vector <char> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> result = std::forward <const vector <char>> (entity);
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::PATCH, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * PATCH Метод запроса в формате HTTP методом PATCH
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::AWH::PATCH(const uri_t::url_t & url, const unordered_multimap <string, string> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Тело в формате X-WWW-Form-Urlencoded
	string body = "";
	// Переходим по всему списку тела запроса
	for(auto & param : entity){
		// Есди данные уже набраны
		if(!body.empty()) body.append("&");
		// Добавляем в список набор параметров
		body.append(this->_uri.encode(param.first));
		// Добавляем разделитель
		body.append("=");
		// Добавляем значение
		body.append(this->_uri.encode(param.second));
	}
	// Устанавливаем тепло запроса
	vector <char> result(body.begin(), body.end());
	// Добавляем заголовок типа контента
	const_cast <unordered_multimap <string, string> *> (&headers)->emplace("Content-Type", "application/x-www-form-urlencoded");
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::PATCH, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * HEAD Метод запроса в формате HTTP методом HEAD
 * @param url     адрес запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
unordered_multimap <string, string> awh::client::AWH::HEAD(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> entity;
	// Результат работы функции
	unordered_multimap <string, string> result = std::forward <const unordered_multimap <string, string>> (headers);
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::HEAD, url, entity, result);
	// Выводим результат
	return result;
}
/**
 * TRACE Метод запроса в формате HTTP методом TRACE
 * @param url     адрес запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
unordered_multimap <string, string> awh::client::AWH::TRACE(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> entity;
	// Результат работы функции
	unordered_multimap <string, string> result = std::forward <const unordered_multimap <string, string>> (headers);
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::TRACE, url, entity, result);
	// Выводим результат
	return result;
}
/**
 * OPTIONS Метод запроса в формате HTTP методом OPTIONS
 * @param url     адрес запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
unordered_multimap <string, string> awh::client::AWH::OPTIONS(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> entity;
	// Результат работы функции
	unordered_multimap <string, string> result = std::forward <const unordered_multimap <string, string>> (headers);
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::OPTIONS, url, entity, result);
	// Выводим результат
	return result;
}
/**
 * REQUEST Метод выполнения запроса HTTP
 * @param method  метод запроса
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 */
void awh::client::AWH::REQUEST(const awh::web_t::method_t method, const uri_t::url_t & url, vector <char> & entity, unordered_multimap <string, string> & headers) noexcept {
	// Если данные запроса переданы
	if(!url.empty()){
		// Подписываемся на получение сообщения сервера
		this->on([this](const int32_t id, const u_int code, const string & message) noexcept -> void {
			// Блокируем пустую переменную
			(void) id;
			// Если возникла ошибка, выводим сообщение
			if(code >= 300)
				// Выводим сообщение о неудачном запросе
				this->_log->print("Request failed: %u %s", log_t::flag_t::WARNING, code, message.c_str());
		});
		// Подписываемся на событие коннекта и дисконнекта клиента
		this->on([method, &url, &entity, &headers, this](const web_t::mode_t mode) noexcept -> void {
			// Если подключение выполнено
			if(mode == client::web_t::mode_t::CONNECT){
				// Создаём объект запроса
				web_t::request_t request;
				// Устанавливаем адрес запроса
				request.url = url;
				// Устанавливаем метод запроса
				request.method = method;
				// Устанавливаем тепло запроса
				request.entity = std::forward <const vector <char>> (entity);
				// Запоминаем переданные заголовки
				request.headers = std::forward <const unordered_multimap <string, string>> (headers);
				// Выполняем запрос на сервер
				this->send(web_t::agent_t::HTTP, request);
			// Выполняем остановку работы модуля
			} else this->stop();
		});
		// Подписываемся на событие получения тела ответа
		this->on([&entity, this](const int32_t id, const u_int code, const string & message, const vector <char> & data) noexcept -> void {
			// Блокируем пустую переменную
			(void) id;
			// Если тело ответа получено
			if(!data.empty())
				// Формируем результат ответа
				entity.assign(data.begin(), data.end());
			// Выполняем очистку тела запроса
			else entity.clear();
			// Выполняем остановку
			this->stop();
		});
		// Подписываем на событие получения заголовков ответа
		this->on([&headers, this](const int32_t id, const u_int code, const string & message, const unordered_multimap <string, string> & data) noexcept -> void {
			// Блокируем пустую переменную
			(void) id;
			// Если заголовки ответа получены
			if(!data.empty())
				// Извлекаем полученный список заголовков
				headers = std::forward <const unordered_multimap <string, string>> (data);
		});
		// Выполняем инициализацию подключения
		this->init(this->_uri.origin(url));
		// Выполняем запуск работы
		this->start();
	}
}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param callback функция обратного вызова
 */
void awh::client::AWH::on(function <void (const web_t::mode_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибок
 * @param callback функция обратного вызова
 */
void awh::client::AWH::on(function <void (const u_int, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param callback функция обратного вызова
 */
void awh::client::AWH::on(function <void (const vector <char> &, const bool)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
 * @param callback функция обратного вызова
 */
void awh::client::AWH::on(function <void (const awh::core_t::status_t, awh::core_t *)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибки
 * @param callback функция обратного вызова
 */
void awh::client::AWH::on(function <void (const log_t::flag_t, const http::error_t, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функции обратного вызова для перехвата полученных чанков
 * @param callback функция обратного вызова
 */
void awh::client::AWH::on(function <void (const uint64_t, const vector <char> &, const awh::http_t *)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функции вывода полученного чанка бинарных данных с сервера
 * @param callback функция обратного вызова
 */
void awh::client::AWH::on(function <void (const int32_t, const vector <char> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функция обратного вызова при полном получении запроса клиента
 * @param callback функция обратного вызова
 */
void awh::client::AWH::on(function <void (const int32_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функция обратного вызова активности потока
 * @param callback функция обратного вызова
 */
void awh::client::AWH::on(function <void (const int32_t, const web_t::mode_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод выполнения редиректа с одного потока на другой (необходим для совместимости с HTTP/2)
 * @param callback функция обратного вызова
 */
void awh::client::AWH::on(function <void (const int32_t, const int32_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функции вывода ответа сервера на ранее выполненный запрос
 * @param callback функция обратного вызова
 */
void awh::client::AWH::on(function <void (const int32_t, const u_int, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функции вывода полученного заголовка с сервера
 * @param callback функция обратного вызова
 */
void awh::client::AWH::on(function <void (const int32_t, const string &, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функции вывода полученного тела данных с сервера
 * @param callback функция обратного вызова
 */
void awh::client::AWH::on(function <void (const int32_t, const u_int, const string &, const vector <char> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функции вывода полученных заголовков с сервера
 * @param callback функция обратного вызова
 */
void awh::client::AWH::on(function <void (const int32_t, const u_int, const string &, const unordered_multimap <string, string> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * open Метод открытия подключения
 */
void awh::client::AWH::open() noexcept {
	// Выполняем открытие подключения
	this->_http.open();
}
/**
 * stop Метод остановки клиента
 */
void awh::client::AWH::stop() noexcept {
	// Выполняем остановку работы модуля
	this->_http.stop();
}
/**
 * start Метод запуска клиента
 */
void awh::client::AWH::start() noexcept {
	// Выполняем запуск работы модуля
	this->_http.start();
}
/**
 * sub Метод получения выбранного сабпротокола WebSocket
 * @return выбранный сабпротокол
 */
const string & awh::client::AWH::sub() const noexcept {
	// Выполняем извлечение выбранного сабпротокола
	return this->_http.sub();
}
/**
 * sub Метод установки сабпротокола поддерживаемого сервером WebSocket
 * @param sub сабпротокол для установки
 */
void awh::client::AWH::sub(const string & sub) noexcept {
	// Выполняем установку сабпротокола
	this->_http.sub(sub);
}
/**
 * subs Метод установки списка сабпротоколов поддерживаемых сервером WebSocket
 * @param subs сабпротоколы для установки
 */
void awh::client::AWH::subs(const vector <string> & subs) noexcept {
	// Выполняем установку списка сабпротоколов
	this->_http.subs(subs);
}
/**
 * extensions Метод извлечения списка расширений WebSocket
 * @return список поддерживаемых расширений
 */
const vector <vector <string>> & awh::client::AWH::extensions() const noexcept {
	// Выполняем извлечение списка расширений
	return this->_http.extensions();
}
/**
 * extensions Метод установки списка расширений WebSocket
 * @param extensions список поддерживаемых расширений
 */
void awh::client::AWH::extensions(const vector <vector <string>> & extensions) noexcept {
	// Выполняем установку списка расширений
	this->_http.extensions(extensions);
}
/**
 * chunk Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::client::AWH::chunk(const size_t size) noexcept {
	// Выполняем установку размера чанка
	this->_http.chunk(size);
}
/**
 * segmentSize Метод установки размеров сегментов фрейма WebSocket
 * @param size минимальный размер сегмента
 */
void awh::client::AWH::segmentSize(const size_t size) noexcept {
	// Выполняем установку размера сегмента фрейма WebSocket
	this->_http.segmentSize(size);
}
/**
 * attempts Метод установки общего количества попыток
 * @param attempts общее количество попыток
 */
void awh::client::AWH::attempts(const uint8_t attempts) noexcept {
	// Выполняем установку количества попыток редиректа
	this->_http.attempts(attempts);
}
/**
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::client::AWH::mode(const set <web_t::flag_t> & flags) noexcept {
	// Выполняем установку флагов настроек модуля
	this->_http.mode(flags);
}
/**
 * compress Метод установки метода компрессии
 * @param compress метод компрессии сообщений
 */
void awh::client::AWH::compress(const awh::http_t::compress_t compress) noexcept {
	// Выполняем установку метода компрессии
	this->_http.compress(compress);
}
/**
 * user Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::client::AWH::user(const string & login, const string & password) noexcept {
	// Выполняем установку логина и пароля пользователя
	this->_http.user(login, password);
}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::client::AWH::keepAlive(const int cnt, const int idle, const int intvl) noexcept {
	// Выполняем установку жизни подключения
	this->_http.keepAlive(cnt, idle, intvl);
}
/**
 * multiThreads Метод активации многопоточности в WebSocket
 * @param threads количество потоков для активации
 * @param mode    флаг активации/деактивации мультипоточности
 */
void awh::client::AWH::multiThreads(const size_t threads, const bool mode) noexcept {
	// Выполняем активацию многопоточности при получения данных в WebSocket
	this->_http.multiThreads(threads, mode);
}
/**
 * userAgent Метод установки User-Agent для HTTP запроса
 * @param userAgent агент пользователя для HTTP запроса
 */
void awh::client::AWH::userAgent(const string & userAgent) noexcept {
	// Выполняем установку User-Agent для HTTP запроса
	this->_http.userAgent(userAgent);
}
/**
 * ident Метод установки идентификации клиента
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::client::AWH::ident(const string & id, const string & name, const string & ver) noexcept {
	// Выполняем установку данных сервиса
	this->_http.ident(id, name, ver);
}
/**
 * proxy Метод установки прокси-сервера
 * @param uri    параметры прокси-сервера
 * @param family семейстово интернет протоколов (IPV4 / IPV6 / NIX)
 */
void awh::client::AWH::proxy(const string & uri, const scheme_t::family_t family) noexcept {
	// Выполняем установку прокси-сервера
	this->_http.proxy(uri, family);
}
/**
 * crypto Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::client::AWH::crypto(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Выполняем установку параметров шифрования
	this->_http.crypto(pass, salt, cipher);
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest-авторизации
 */
void awh::client::AWH::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Выполняем установку типа авторизации
	this->_http.authType(type, hash);
}
/**
 * authTypeProxy Метод установки типа авторизации прокси-сервера
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest-авторизации
 */
void awh::client::AWH::authTypeProxy(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Выполняем установку типа авторизации на прокси-сервере
	this->_http.authTypeProxy(type, hash);
}
/**
 * bytesDetect Метод детекции сообщений по количеству байт
 * @param read  количество байт для детекции по чтению
 * @param write количество байт для детекции по записи
 */
void awh::client::AWH::bytesDetect(const scheme_t::mark_t read, const scheme_t::mark_t write) noexcept {
	// Выполняем установку детекции сообщений по количеству байт
	this->_http.bytesDetect(read, write);
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read    количество секунд для детекции по чтению
 * @param write   количество секунд для детекции по записи
 * @param connect количество секунд для детекции по подключению
 */
void awh::client::AWH::waitTimeDetect(const time_t read, const time_t write, const time_t connect) noexcept {
	// Выполняем установку детекции сообщений по количеству секунд
	this->_http.waitTimeDetect(read, write, connect);
}
