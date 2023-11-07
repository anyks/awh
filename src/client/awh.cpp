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
 * proto Метод извлечения поддерживаемого протокола подключения
 * @return поддерживаемый протокол подключения (HTTP1_1, HTTP2)
 */
awh::engine_t::proto_t awh::client::AWH::proto() const noexcept {
	// Выполняем определение активного HTTP-протокола
	return this->_http.proto();
}
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
 * sendMessage Метод отправки сообщения на сервер
 * @param message передаваемое сообщения в бинарном виде
 * @param text    данные передаются в текстовом виде
 */
void awh::client::AWH::sendMessage(const vector <char> & message, const bool text) noexcept {
	// Выполняем отправку сообщения на WebSocket-сервер
	this->_http.sendMessage(message, text);
}
/**
 * send Метод отправки сообщения на сервер HTTP/2
 * @param request параметры запроса на удалённый сервер
 * @return        идентификатор отправленного запроса
 */
int32_t awh::client::AWH::send(const web_t::request_t & request) noexcept {
	// Выполняем отправку сообщения на удалённый сервер
	return this->_http.send(request);
}
/**
 * send Метод отправки тела сообщения на сервер
 * @param id     идентификатор потока HTTP
 * @param buffer буфер бинарных данных передаваемых на сервер
 * @param size   размер сообщения в байтах
 * @param end    флаг последнего сообщения после которого поток закрывается
 * @return       результат отправки данных указанному клиенту
 */
bool awh::client::AWH::send(const int32_t id, const char * buffer, const size_t size, const bool end) noexcept {
	// Выполняем отправку данных на удалённый сервер HTTP/2
	return this->_http.send(id, buffer, size, end);
}
/**
 * send Метод отправки заголовков на сервер
 * @param id      идентификатор потока HTTP
 * @param url     адрес запроса на сервере
 * @param method  метод запроса на сервере
 * @param headers заголовки отправляемые на сервер
 * @param end     размер сообщения в байтах
 * @return        идентификатор нового запроса
 */
int32_t awh::client::AWH::send(const int32_t id, const uri_t::url_t & url, const awh::web_t::method_t method, const unordered_multimap <string, string> & headers, const bool end) noexcept {
	// Выполняем отправку заголовков на удалённый сервер HTTP/2
	return this->_http.send(id, url, method, headers, end);
}
/**
 * windowUpdate2 Метод HTTP/2 обновления размера окна фрейма
 * @param id   идентификатор потока
 * @param size размер нового окна
 * @return     результат установки размера офна фрейма
 */
bool awh::client::AWH::windowUpdate2(const int32_t id, const int32_t size) noexcept {
	// Выполняем обновление размера окна фрейма
	return this->_http.windowUpdate2(id, size);
}
/**
 * send2 Метод HTTP/2 отправки сообщения на сервер
 * @param id     идентификатор потока
 * @param buffer буфер бинарных данных передаваемых на сервер
 * @param size   размер сообщения в байтах
 * @param flag   флаг передаваемого потока по сети
 * @return       результат отправки данных указанному клиенту
 */
bool awh::client::AWH::send2(const int32_t id, const char * buffer, const size_t size, const awh::http2_t::flag_t flag) noexcept {
	// Выполняем отправку сообщения на сервер
	return this->_http.send2(id, buffer, size, flag);
}
/**
 * send2 Метод HTTP/2 отправки заголовков на сервер
 * @param id      идентификатор потока
 * @param headers заголовки отправляемые на сервер
 * @param flag    флаг передаваемого потока по сети
 * @return        идентификатор нового запроса
 */
int32_t awh::client::AWH::send2(const int32_t id, const vector <pair <string, string>> & headers, const awh::http2_t::flag_t flag) noexcept {
	// Выполняем отправку заголовков на сервер
	return this->_http.send2(id, headers, flag);
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
 * @param dest        адрес назначения удалённого сервера
 * @param compressors список поддерживаемых компрессоров
 */
void awh::client::AWH::init(const string & dest, const vector <awh::http_t::compress_t> & compressors) noexcept {
	// Выполняем инициализацию клиента
	this->_http.init(dest, compressors);
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
	// Выполняем HTTP-запрос на сервер
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
	// Выполняем HTTP-запрос на сервер
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
	// Выполняем HTTP-запрос на сервер
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
	// Выполняем HTTP-запрос на сервер
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
	// Выполняем HTTP-запрос на сервер
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
	// Выполняем HTTP-запрос на сервер
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
	// Выполняем HTTP-запрос на сервер
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
	// Выполняем HTTP-запрос на сервер
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
	// Выполняем HTTP-запрос на сервер
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
	// Выполняем HTTP-запрос на сервер
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
	// Выполняем HTTP-запрос на сервер
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
	// Выполняем HTTP-запрос на сервер
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
	// Выполняем HTTP-запрос на сервер
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
	// Выполняем HTTP-запрос на сервер
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
		/**
		 * Подписываемся на получение сообщения сервера
		 * @param id      идентификатор потока
		 * @param code    код ответа сервера
		 * @param message сообщение ответа сервера
		 */
		this->on([this](const int32_t id, const u_int code, const string & message) noexcept -> void {
			// Блокируем пустую переменную
			(void) id;
			// Если возникла ошибка, выводим сообщение
			if(code >= 300)
				// Выводим сообщение о неудачном запросе
				this->_log->print("Request failed: %u %s", log_t::flag_t::WARNING, code, message.c_str());
		});
		/**
		 * Подписываемся на завершение выполнения запроса
		 * @param id     идентификатор потока
		 * @param direct направление передачи данных
		 */
		this->on([this](const int32_t id, const web_t::direct_t direct) noexcept -> void {
			// Блокируем пустую переменную
			(void) id;
			// Если мы получили данные
			if(direct == web_t::direct_t::RECV)
				// Выполняем остановку
				this->stop();
		});
		/**
		 * Подписываемся на событие коннекта и дисконнекта клиента
		 * @param mode событие модуля HTTP
		 */
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
				this->send(request);
			// Выполняем остановку работы модуля
			} else this->stop();
		});
		/**
		 * Подписываемся на событие получения тела ответа
		 * @param id      идентификатор потока
		 * @param code    код ответа сервера
		 * @param message сообщение ответа сервера
		 * @param data    данные полученного тела сообщения
		 */
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
		/**
		 * Подписываем на событие получения заголовков ответа
		 * @param id      идентификатор потока
		 * @param code    код ответа сервера
		 * @param message сообщение ответа сервера
		 * @param data    данные полученных заголовков сообщения
		 */
		this->on([&headers, this](const int32_t id, const u_int code, const string & message, const unordered_multimap <string, string> & data) noexcept -> void {
			// Блокируем пустую переменную
			(void) id;
			// Если заголовки ответа получены
			if(!data.empty())
				// Извлекаем полученный список заголовков
				headers = std::forward <const unordered_multimap <string, string>> (data);
		});
		// Выполняем инициализацию подключения
		this->init(this->_uri.origin(url), {
			awh::http_t::compress_t::BROTLI,
			awh::http_t::compress_t::GZIP,
			awh::http_t::compress_t::DEFLATE
		});
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
 * on Метод выполнения редиректа с одного потока на другой (необходим для совместимости с HTTP/2)
 * @param callback функция обратного вызова
 */
void awh::client::AWH::on(function <void (const int32_t, const int32_t)> callback) noexcept {
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
 * on Метод установки функция обратного вызова при выполнении рукопожатия
 * @param callback функция обратного вызова
 */
void awh::client::AWH::on(function <void (const int32_t, const web_t::agent_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функции обратного вызова при завершении запроса
 * @param callback функция обратного вызова
 */
void awh::client::AWH::on(function <void (const int32_t, const web_t::direct_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функции обратного вызова при получении источника подключения
 * @param callback функция обратного вызова
 */
void awh::client::AWH::on(function <void (const vector <string> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функции обратного вызова при получении альтернативных сервисов
 * @param callback функция обратного вызова
 */
void awh::client::AWH::on(function <void (const string &, const string &)> callback) noexcept {
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
 * on Метод установки функции вывода запроса клиента к серверу
 * @param callback функция обратного вызова
 */
void awh::client::AWH::on(function <void (const int32_t, const awh::web_t::method_t, const uri_t::url_t &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->_http.on(callback);
}
/**
 * on Метод установки функции обратного вызова на вывода push-уведомления
 * @param callback функция обратного вызова
 */
void awh::client::AWH::on(function <void (const int32_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> callback) noexcept {
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
 * subprotocol Метод установки поддерживаемого сабпротокола
 * @param subprotocol сабпротокол для установки
 */
void awh::client::AWH::subprotocol(const string & subprotocol) noexcept {
	// Выполняем установку поддерживаемого сабпротокола
	this->_http.subprotocol(subprotocol);
}
/**
 * subprotocol Метод получения списка выбранных сабпротоколов
 * @return список выбранных сабпротоколов
 */
const set <string> & awh::client::AWH::subprotocols() const noexcept {
	// Выполняем извлечение списка выбранных сабпротоколов
	return this->_http.subprotocols();
}
/**
 * subprotocols Метод установки списка поддерживаемых сабпротоколов
 * @param subprotocols сабпротоколы для установки
 */
void awh::client::AWH::subprotocols(const set <string> & subprotocols) noexcept {
	// Выполняем установку поддерживаемых сабпротоколов
	this->_http.subprotocols(subprotocols);
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
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::client::AWH::mode(const set <web_t::flag_t> & flags) noexcept {
	// Выполняем установку флагов настроек модуля
	this->_http.mode(flags);
}
/**
 * settings Модуль установки настроек протокола HTTP/2
 * @param settings список настроек протокола HTTP/2
 */
void awh::client::AWH::settings(const map <web2_t::settings_t, uint32_t> & settings) noexcept {
	// Выполняем установку списока настроек протокола HTTP/2
	this->_http.settings(settings);
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
 * hosts Метод загрузки файла со списком хостов
 * @param filename адрес файла для загрузки
 */
void awh::client::AWH::hosts(const string & filename) noexcept {
	// Если адрес файла с хостами в операционной системе передан
	if(!filename.empty())
		// Выполняем установку адреса файла хостов в операционной системе
		this->_dns.hosts(filename);
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
 * compressors Метод установки списка поддерживаемых компрессоров
 * @param compressors список поддерживаемых компрессоров
 */
void awh::client::AWH::compressors(const vector <awh::http_t::compress_t> & compressors) noexcept {
	// Выполняем установку списка поддерживаемых компрессоров
	this->_http.compressors(compressors);
}
/**
 * multiThreads Метод активации многопоточности в WebSocket
 * @param count количество потоков для активации
 * @param mode  флаг активации/деактивации мультипоточности
 */
void awh::client::AWH::multiThreads(const uint16_t count, const bool mode) noexcept {
	// Выполняем активацию многопоточности при получения данных в WebSocket
	this->_http.multiThreads(count, mode);
}
/**
 * userAgent Метод установки User-Agent для HTTP-запроса
 * @param userAgent агент пользователя для HTTP-запроса
 */
void awh::client::AWH::userAgent(const string & userAgent) noexcept {
	// Выполняем установку User-Agent для HTTP-запроса
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
 * flushDNS Метод сброса кэша DNS-резолвера
 * @return результат работы функции
 */
bool awh::client::AWH::flushDNS() noexcept {
	// Выполняем сброс кэша DNS-резолвера
	return this->_dns.flush();
}
/**
 * timeoutDNS Метод установки времени ожидания выполнения запроса
 * @param sec интервал времени выполнения запроса в секундах
 */
void awh::client::AWH::timeoutDNS(const uint8_t sec) noexcept {
	// Если время ожидания выполнения DNS-запроса передано
	if(sec > 0)
		// Выполняем установку времени ожидания получения данных с DNS-сервера
		this->_dns.timeout(sec);
}
/**
 * timeToLiveDNS Метод установки времени жизни DNS-кэша
 * @param ttl время жизни DNS-кэша в миллисекундах
 */
void awh::client::AWH::timeToLiveDNS(const time_t ttl) noexcept {
	// Если значение времени жизни DNS-кэша передано
	if(ttl > 0)
		// Выполняем установку времени жизни DNS-кэша
		this->_dns.timeToLive(ttl);
}
/**
 * prefixDNS Метод установки префикса переменной окружения для извлечения серверов имён
 * @param prefix префикс переменной окружения для установки
 */
void awh::client::AWH::prefixDNS(const string & prefix) noexcept {
	// Если префикс переменной окружения для извлечения серверов имён передан
	if(!prefix.empty())
		// Выполняем установку префикса переменной окружения
		this->_dns.prefix(prefix);
}
/**
 * clearDNSBlackList Метод очистки чёрного списка
 * @param domain доменное имя для которого очищается чёрный список
 */
void awh::client::AWH::clearDNSBlackList(const string & domain) noexcept {
	// Если доменное имя для удаления из чёрного списока передано
	if(!domain.empty())
		// Выполняем удаление доменного имени из чёрного списока
		this->_dns.clearBlackList(domain);
}
/**
 * delInDNSBlackList Метод удаления IP-адреса из чёрного списока
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для удаления из чёрного списка
 */
void awh::client::AWH::delInDNSBlackList(const string & domain, const string & ip) noexcept {
	// Если доменное имя для удаления из чёрного списока и соответствующий ему IP-адрес переданы
	if(!domain.empty() && !ip.empty())
		// Выполняем удаление доменного имени из чёрного списока
		this->_dns.delInBlackList(domain, ip);
}
/**
 * setToDNSBlackList Метод добавления IP-адреса в чёрный список
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для добавления в чёрный список
 */
void awh::client::AWH::setToDNSBlackList(const string & domain, const string & ip) noexcept {
	// Если доменное имя для добавление в чёрный список и соответствующий ему IP-адрес переданы
	if(!domain.empty() && !ip.empty())
		// Выполняем установку доменного имени в чёрный список
		this->_dns.setToBlackList(domain, ip);
}
/**
 * encryption Метод активации шифрования
 * @param mode флаг активации шифрования
 */
void awh::client::AWH::encryption(const bool mode) noexcept {
	// Выполняем установку флага шифрования
	this->_http.encryption(mode);
}
/**
 * encryption Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::client::AWH::encryption(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Выполняем установку параметров шифрования
	this->_http.encryption(pass, salt, cipher);
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
/**
 * network Метод установки параметров сети
 * @param ips    список IP-адресов компьютера с которых разрешено выходить в интернет
 * @param ns     список серверов имён, через которые необходимо производить резолвинг доменов
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
 */
void awh::client::AWH::network(const vector <string> & ips, const vector <string> & ns, const scheme_t::family_t family) noexcept {
	// Если список IP-адресов передан
	if(!ips.empty()){
		// Определяем тип протокола интернета
		switch(static_cast <uint8_t> (family)){
			// Если протокол интернета соответствует IPv4
			case static_cast <uint8_t> (scheme_t::family_t::IPV4):
				// Добавляем список IP-адресов через которые нужно выходить в интернет
				this->_dns.network(AF_INET, ips);
			break;
			// Если протокол интернета соответствует IPv6
			case static_cast <uint8_t> (scheme_t::family_t::IPV6):
				// Добавляем список IP-адресов через которые нужно выходить в интернет
				this->_dns.network(AF_INET6, ips);
			break;
		}
		// Устанавливаем параметры сети для сетевого ядра
		const_cast <client::core_t *> (this->_core)->network(ips, family);
	}
	// Если список DNS-серверов передан
	if(!ns.empty()){
		// Определяем тип протокола интернета
		switch(static_cast <uint8_t> (family)){
			// Если протокол интернета соответствует IPv4
			case static_cast <uint8_t> (scheme_t::family_t::IPV4):
				// Выполняем установку списка DNS-серверов
				this->_dns.replace(AF_INET, ns);
			break;
			// Если протокол интернета соответствует IPv6
			case static_cast <uint8_t> (scheme_t::family_t::IPV6):
				// Выполняем установку списка DNS-серверов
				this->_dns.replace(AF_INET6, ns);
			break;
		}
	}
}
/**
 * AWH Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::client::AWH::AWH(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 _uri(fmk), _dns(fmk, log), _http(core, fmk, log), _fmk(fmk), _log(log), _core(core) {
	// Выполняем установку DNS-резолвера
	const_cast <client::core_t *> (core)->resolver(&this->_dns);
}
