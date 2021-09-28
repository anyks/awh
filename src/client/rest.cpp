/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <client/rest.hpp>

/**
 * chunking Метод обработки получения чанков
 * @param chunk бинарный буфер чанка
 * @param ctx   контекст объекта http
 */
void awh::Rest::chunking(const vector <char> & chunk, const http_t * ctx) noexcept {
	// Если данные получены, формируем тело сообщения
	if(!chunk.empty()) const_cast <http_t *> (ctx)->addBody(chunk.data(), chunk.size());
}
/**
 * openCallback Функция обратного вызова при подключении к серверу
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::Rest::openCallback(const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные переданы верные
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		rest_t * web = reinterpret_cast <rest_t *> (ctx);
		// Выполняем сброс состояния HTTP парсера
		web->http->clear();
		// Если список заголовков получен
		if((web->headers != nullptr) && !web->headers->empty()){
			// Переходим по всему списку заголовков
			for(auto & header : * web->headers)
				// Устанавливаем заголовок
				web->http->addHeader(header.first, header.second);
		}
		// Если тело запроса существует
		if((web->entity != nullptr) && !web->entity->empty())
			// Устанавливаем тело запроса
			web->http->addBody(web->entity->data(), web->entity->size());
		// Получаем бинарные данные REST запроса
		const auto & rest = web->http->request(web->worker.url, web->method);
		// Если бинарные данные запроса получены
		if(!rest.empty()){
			// Тело REST сообщения
			vector <char> entity;
			// Отправляем серверу сообщение
			core->write(rest.data(), rest.size(), wid);
			// Получаем данные тела запроса
			while(!(entity = web->http->chunkBody()).empty()){
				// Отправляем тело на сервер
				core->write(entity.data(), entity.size(), wid);
			}
		}
		// Выполняем сброс состояния HTTP парсера
		web->http->clear();
	}
}
/**
 * closeCallback Функция обратного вызова при отключении от сервера
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::Rest::closeCallback(const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные переданы верные
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		rest_t * web = reinterpret_cast <rest_t *> (ctx);
		// Проверяем работаем ли мы через прокси-сервер
		if(web->worker.proxy.type == proxy_t::type_t::NONE){
			// Если нужно произвести запрос заново
			if((web->res.code == 301) || (web->res.code == 308) ||
			   (web->res.code == 401) || (web->res.code == 407)){
				// Выполняем запрос заново
				core->open(web->worker.wid);
				// Выходим из функции
				return;
			}
		}
		// Выполняем разблокировку запроса
		web->locker = false;
		// Завершаем работу
		if(web->unbind) core->stop();
	}
}
/**
 * startCallback Функция обратного вызова при запуске работы
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::Rest::startCallback(const size_t wid, core_t * core, void * ctx) noexcept {
	// Выполняем подключение
	core->open(wid);
}
/**
 * openProxyCallback Функция обратного вызова при подключении к прокси-серверу
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::Rest::openProxyCallback(const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные переданы верные
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		rest_t * web = reinterpret_cast <rest_t *> (ctx);
		// Выполняем сброс состояния HTTP парсера
		web->worker.proxy.http->clear();
		// Получаем бинарные данные REST запроса
		const auto & rest = web->worker.proxy.http->proxy(web->worker.url);
		// Если бинарные данные запроса получены, отправляем на прокси-сервер
		if(!rest.empty()) core->write(rest.data(), rest.size(), wid);
		// Выполняем сброс состояния HTTP парсера
		web->worker.proxy.http->clear();
	}
}
/**
 * readCallback Функция обратного вызова при чтении сообщения с сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param wid    идентификатор воркера
 * @param core   объект биндинга TCP/IP
 * @param ctx    передаваемый контекст модуля
 */
void awh::Rest::readCallback(const char * buffer, const size_t size, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0)){
		// Получаем контекст модуля
		rest_t * web = reinterpret_cast <rest_t *> (ctx);
		// Выполняем парсинг полученных данных
		web->http->parse(buffer, size);
		// Если все данные получены
		if(web->http->isEnd()){
			// Получаем параметры запроса
			const auto & query = web->http->getQuery();
			// Устанавливаем код ответа
			web->res.code = query.code;
			// Устанавливаем сообщение ответа
			web->res.message = query.message;
			// Получаем статус авторизации на сервере
			const auto stath = web->http->getAuth();
			// Выполняем проверку авторизации
			switch((u_short) stath){
				// Если нужно попытаться ещё раз
				case (u_short) http_t::stath_t::RETRY: {
					// Если попытка повторить авторизацию ещё не проводилась
					if(!web->failAuth){
						// Получаем новый адрес запроса
						web->worker.url = web->http->getUrl();
						// Если адрес запроса получен
						if(!web->worker.url.empty()){
							// Запоминаем, что попытка выполнена
							web->failAuth = true;
							// Если соединение является постоянным
							if(web->http->isAlive())
								// Выполняем повторно отправку сообщения на сервер
								openCallback(wid, core, ctx);
							// Завершаем работу
							else core->close(web->worker.wid);
							// Завершаем работу
							return;
						}
					}
					// Устанавливаем код ответа
					web->res.code = 403;
					// Устанавливаем сообщение ответа
					web->res.message = web->http->getMessage(web->res.code);
				} break;
				// Если запрос выполнен удачно
				case (u_short) http_t::stath_t::GOOD: web->res.ok = true;
				// Если запрос неудачный
				case (u_short) http_t::stath_t::FAULT: {
					// Получаем тело запроса
					const auto & entity = web->http->getBody();
					// Устанавливаем заголовки ответа
					web->res.headers = web->http->getHeaders();
					// Устанавливаем тело ответа
					web->res.entity.assign(entity.begin(), entity.end());
				} break;
			}
			// Выполняем сброс количество попыток
			web->failAuth = false;
			// Завершаем работу
			core->close(web->worker.wid);
		}
	}
}
/**
 * readProxyCallback Функция обратного вызова при чтении сообщения с прокси-сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param wid    идентификатор воркера
 * @param core   объект биндинга TCP/IP
 * @param ctx    передаваемый контекст модуля
 */
void awh::Rest::readProxyCallback(const char * buffer, const size_t size, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0)){
		// Получаем контекст модуля
		rest_t * web = reinterpret_cast <rest_t *> (ctx);
		// Выполняем парсинг полученных данных
		web->worker.proxy.http->parse(buffer, size);
		// Если все данные получены
		if(web->worker.proxy.http->isEnd()){
			// Получаем параметры запроса
			const auto & query = web->worker.proxy.http->getQuery();
			// Устанавливаем код ответа
			web->res.code = query.code;
			// Устанавливаем сообщение ответа
			web->res.message = query.message;
			// Получаем статус авторизации на сервере
			const auto stath = web->worker.proxy.http->getAuth();
			// Выполняем проверку авторизации
			switch((u_short) stath){
				// Если нужно попытаться ещё раз
				case (u_short) http_t::stath_t::RETRY: {
					// Если попытка повторить авторизацию ещё не проводилась
					if(!web->failAuth){
						// Получаем новый адрес запроса
						web->worker.proxy.url = web->worker.proxy.http->getUrl();
						// Если адрес запроса получен
						if(!web->worker.proxy.url.empty()){
							// Запоминаем, что попытка выполнена
							web->failAuth = true;
							// Если соединение является постоянным
							if(web->http->isAlive())
								// Выполняем повторно отправку сообщения на сервер
								openProxyCallback(wid, core, ctx);
							// Завершаем работу
							else core->close(web->worker.wid);
							// Завершаем работу
							return;
						}
					}
					// Устанавливаем код ответа
					web->res.code = 403;
					// Устанавливаем сообщение ответа
					web->res.message = web->worker.proxy.http->getMessage(web->res.code);
				} break;
				// Если запрос выполнен удачно
				case (u_short) http_t::stath_t::GOOD: {
					// Выполняем сброс количество попыток
					web->failAuth = false;
					// Меняем код ответа на всяккий случай
					web->res.code = 404;
					// Устанавливаем сообщение ответа
					web->res.message = web->worker.proxy.http->getMessage(web->res.code);
					// Выполняем переключение на работу с сервером
					core->switchProxy(web->worker.wid);
					// Завершаем работу
					return;
				} break;
				// Если запрос неудачный
				case (u_short) http_t::stath_t::FAULT: {
					// Получаем тело запроса
					const auto & entity = web->worker.proxy.http->getBody();
					// Устанавливаем заголовки ответа
					web->res.headers = web->worker.proxy.http->getHeaders();
					// Устанавливаем тело ответа
					web->res.entity.assign(entity.begin(), entity.end());
				} break;
			}
			// Выполняем сброс количество попыток
			web->failAuth = false;
			// Завершаем работу
			core->close(web->worker.wid);
		}
	}
}
/**
 * GET Метод REST запроса
 * @param url     адрес запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const vector <char> & awh::Rest::GET(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Если данные запроса переданы
	if(!url.empty()){
		// Выполняем REST запрос на сервер
		const auto & res = this->REST(url, http_t::method_t::GET, {}, headers);
		// Проверяем на наличие ошибок
		if(!res.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
	}
	// Выводим результат
	return this->res.entity;
}
/**
 * DEL Метод запроса в формате HTTP методом DEL
 * @param url     адрес запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
const vector <char> & awh::Rest::DEL(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Если данные запроса переданы
	if(!url.empty()){
		// Выполняем REST запрос на сервер
		const auto & res = this->REST(url, http_t::method_t::DELETE, {}, headers);
		// Проверяем на наличие ошибок
		if(!res.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
	}
	// Выводим результат
	return this->res.entity;
}
/**
 * PUT Метод запроса в формате HTTP методом PUT
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
const vector <char> & awh::Rest::PUT(const uri_t::url_t & url, const json & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Если данные запроса переданы
	if(!url.empty()){
		// Получаем тело запроса
		const string body = entity.dump();
		// Добавляем заголовок типа контента
		const_cast <unordered_multimap <string, string> *> (&headers)->emplace("Content-Type", "application/json");
		// Выполняем REST запрос на сервер
		const auto & res = this->REST(url, http_t::method_t::PUT, vector <char> (body.begin(), body.end()), headers);
		// Проверяем на наличие ошибок
		if(!res.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
	}
	// Выводим результат
	return this->res.entity;
}
/**
 * PUT Метод запроса в формате HTTP методом PUT
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
const vector <char> & awh::Rest::PUT(const uri_t::url_t & url, const vector <char> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Если данные запроса переданы
	if(!url.empty()){
		// Выполняем REST запрос на сервер
		const auto & res = this->REST(url, http_t::method_t::PUT, entity, headers);
		// Проверяем на наличие ошибок
		if(!res.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
	}
	// Выводим результат
	return this->res.entity;
}
/**
 * PUT Метод запроса в формате HTTP методом PUT
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
const vector <char> & awh::Rest::PUT(const uri_t::url_t & url, const unordered_multimap <string, string> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Если данные запроса переданы
	if(!url.empty()){
		// Тело в формате X-WWW-Form-Urlencoded
		string body = "";
		// Переходим по всему списку тела запроса
		for(auto & param : entity){
			// Есди данные уже набраны
			if(!body.empty()) body.append("&");
			// Добавляем в список набор параметров
			body.append(this->uri->urlEncode(param.first));
			// Добавляем разделитель
			body.append("=");
			// Добавляем значение
			body.append(this->uri->urlEncode(param.second));
		}
		// Добавляем заголовок типа контента
		const_cast <unordered_multimap <string, string> *> (&headers)->emplace("Content-Type", "application/x-www-form-urlencoded");
		// Выполняем REST запрос на сервер
		const auto & res = this->REST(url, http_t::method_t::PUT, vector <char> (body.begin(), body.end()), headers);
		// Проверяем на наличие ошибок
		if(!res.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
	}
	// Выводим результат
	return this->res.entity;
}
/**
 * POST Метод запроса в формате HTTP методом POST
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
const vector <char> & awh::Rest::POST(const uri_t::url_t & url, const json & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Если данные запроса переданы
	if(!url.empty()){
		// Получаем тело запроса
		const string body = entity.dump();
		// Добавляем заголовок типа контента
		const_cast <unordered_multimap <string, string> *> (&headers)->emplace("Content-Type", "application/json");
		// Выполняем REST запрос на сервер
		const auto & res = this->REST(url, http_t::method_t::POST, vector <char> (body.begin(), body.end()), headers);
		// Проверяем на наличие ошибок
		if(!res.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
	}
	// Выводим результат
	return this->res.entity;
}
/**
 * POST Метод запроса в формате HTTP методом POST
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
const vector <char> & awh::Rest::POST(const uri_t::url_t & url, const vector <char> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Если данные запроса переданы
	if(!url.empty()){
		// Выполняем REST запрос на сервер
		const auto & res = this->REST(url, http_t::method_t::POST, entity, headers);
		// Проверяем на наличие ошибок
		if(!res.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
	}
	// Выводим результат
	return this->res.entity;
}
/**
 * POST Метод запроса в формате HTTP методом POST
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
const vector <char> & awh::Rest::POST(const uri_t::url_t & url, const unordered_multimap <string, string> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Если данные запроса переданы
	if(!url.empty()){
		// Тело в формате X-WWW-Form-Urlencoded
		string body = "";
		// Переходим по всему списку тела запроса
		for(auto & param : entity){
			// Есди данные уже набраны
			if(!body.empty()) body.append("&");
			// Добавляем в список набор параметров
			body.append(this->uri->urlEncode(param.first));
			// Добавляем разделитель
			body.append("=");
			// Добавляем значение
			body.append(this->uri->urlEncode(param.second));
		}
		// Добавляем заголовок типа контента
		const_cast <unordered_multimap <string, string> *> (&headers)->emplace("Content-Type", "application/x-www-form-urlencoded");
		// Выполняем REST запрос на сервер
		const auto & res = this->REST(url, http_t::method_t::POST, vector <char> (body.begin(), body.end()), headers);
		// Проверяем на наличие ошибок
		if(!res.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
	}
	// Выводим результат
	return this->res.entity;
}
/**
 * PATCH Метод запроса в формате HTTP методом PATCH
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
const vector <char> & awh::Rest::PATCH(const uri_t::url_t & url, const json & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Если данные запроса переданы
	if(!url.empty()){
		// Получаем тело запроса
		const string body = entity.dump();
		// Добавляем заголовок типа контента
		const_cast <unordered_multimap <string, string> *> (&headers)->emplace("Content-Type", "application/json");
		// Выполняем REST запрос на сервер
		const auto & res = this->REST(url, http_t::method_t::PATCH, vector <char> (body.begin(), body.end()), headers);
		// Проверяем на наличие ошибок
		if(!res.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
	}
	// Выводим результат
	return this->res.entity;
}
/**
 * PATCH Метод запроса в формате HTTP методом PATCH
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
const vector <char> & awh::Rest::PATCH(const uri_t::url_t & url, const vector <char> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Если данные запроса переданы
	if(!url.empty()){
		// Выполняем REST запрос на сервер
		const auto & res = this->REST(url, http_t::method_t::PATCH, entity, headers);
		// Проверяем на наличие ошибок
		if(!res.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
	}
	// Выводим результат
	return this->res.entity;
}
/**
 * PATCH Метод запроса в формате HTTP методом PATCH
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
const vector <char> & awh::Rest::PATCH(const uri_t::url_t & url, const unordered_multimap <string, string> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Если данные запроса переданы
	if(!url.empty()){
		// Тело в формате X-WWW-Form-Urlencoded
		string body = "";
		// Переходим по всему списку тела запроса
		for(auto & param : entity){
			// Есди данные уже набраны
			if(!body.empty()) body.append("&");
			// Добавляем в список набор параметров
			body.append(this->uri->urlEncode(param.first));
			// Добавляем разделитель
			body.append("=");
			// Добавляем значение
			body.append(this->uri->urlEncode(param.second));
		}
		// Добавляем заголовок типа контента
		const_cast <unordered_multimap <string, string> *> (&headers)->emplace("Content-Type", "application/x-www-form-urlencoded");
		// Выполняем REST запрос на сервер
		const auto & res = this->REST(url, http_t::method_t::PATCH, vector <char> (body.begin(), body.end()), headers);
		// Проверяем на наличие ошибок
		if(!res.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
	}
	// Выводим результат
	return this->res.entity;
}
/**
 * HEAD Метод запроса в формате HTTP методом HEAD
 * @param url     адрес запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
const unordered_multimap <string, string> & awh::Rest::HEAD(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Если данные запроса переданы
	if(!url.empty()){
		// Выполняем REST запрос на сервер
		const auto & res = this->REST(url, http_t::method_t::HEAD, {}, headers);
		// Проверяем на наличие ошибок
		if(!res.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
	}
	// Выводим результат
	return this->res.headers;
}
/**
 * TRACE Метод запроса в формате HTTP методом TRACE
 * @param url     адрес запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
const unordered_multimap <string, string> & awh::Rest::TRACE(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Если данные запроса переданы
	if(!url.empty()){
		// Выполняем REST запрос на сервер
		const auto & res = this->REST(url, http_t::method_t::TRACE, {}, headers);
		// Проверяем на наличие ошибок
		if(!res.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
	}
	// Выводим результат
	return this->res.headers;
}
/**
 * OPTIONS Метод запроса в формате HTTP методом OPTIONS
 * @param url     адрес запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
const unordered_multimap <string, string> & awh::Rest::OPTIONS(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Если данные запроса переданы
	if(!url.empty()){
		// Выполняем REST запрос на сервер
		const auto & res = this->REST(url, http_t::method_t::OPTIONS, {}, headers);
		// Проверяем на наличие ошибок
		if(!res.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
	}
	// Выводим результат
	return this->res.headers;
}
/**
 * REST Метод запроса в формате HTTP указанным методом
 * @param url     адрес запроса
 * @param method  метод запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат выполнения запроса
 */
const awh::Rest::res_t & awh::Rest::REST(const uri_t::url_t & url, http_t::method_t method, const vector <char> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Если параметры и метод запроса переданы
	if(!url.empty() && (method != http_t::method_t::NONE)){
		// Выполняем очистку воркера
		worker.clear();
		// Устанавливаем URL адрес запроса
		worker.url = url;
		// Выполняем блокировку ожидания выполнения запроса
		this->locker = true;
		// Устанавливаем метод запроса
		this->method = method;
		// Устанавливаем тело запроса
		this->entity = &entity;
		// Запоминаем переданные заголовки
		this->headers = &headers;
		// Если биндинг не запущен
		if(!this->core->isStart()){
			// Выполняем запуск биндинга
			const_cast <core_t *> (this->core)->start();
			// Если код пришёл нулевой, восстанавливаем его
			if(this->res.code == 0){
				// Устанавливаем код сообщения
				this->res.code = 404;
				// Получаем само сообщение
				this->res.message = this->http->getMessage(this->res.code);
			}
		// Если биндинг уже запущен
		} else {
			// Выполняем запрос на сервер
			const_cast <core_t *> (this->core)->open(this->worker.wid);
			// Ожидаем появление результата
			while(this->locker){
				/** ... Продолжаем работу до тех, пор пока не получим ответ ... **/
			}
		}
	}
	// Выводим результат
	return this->res;
}
/**
 * setChunkingFn Метод установки функции обратного вызова для получения чанков
 * @param callback функция обратного вызова
 */
void awh::Rest::setChunkingFn(function <void (const vector <char> &, const http_t *)> callback) noexcept {
	// Устанавливаем функцию обработки вызова для получения чанков
	this->http->setChunkingFn(callback);
}
/**
 * setBytesDetect Метод детекции сообщений по количеству байт
 * @param read  количество байт для детекции по чтению
 * @param write количество байт для детекции по записи
 */
void awh::Rest::setBytesDetect(const size_t read, const size_t write) noexcept {
	// Устанавливаем количество байт на чтение
	this->worker.byteRead = read;
	// Устанавливаем количество байт на запись
	this->worker.byteWrite = write;
}
/**
 * setWaitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::Rest::setWaitTimeDetect(const time_t read, const time_t write) noexcept {
	// Устанавливаем количество секунд на чтение
	this->worker.timeRead = read;
	// Устанавливаем количество секунд на запись
	this->worker.timeWrite = write;
}
/**
 * setUnbind Метод установки флага анбиндинга
 * @param mode флаг анбиндинга после завершения запроса
 */
void awh::Rest::setUnbind(const bool mode) noexcept {
	// Устанавливаем флаг анбиндинга
	this->unbind = mode;
}
/**
 * setProxy Метод установки прокси-сервера
 * @param uri параметры прокси-сервера
 */
void awh::Rest::setProxy(const string & uri) noexcept {
	// Если URI параметры переданы
	if(!uri.empty()){
		// Устанавливаем параметры прокси-сервера
		this->worker.proxy.url = this->uri->parseUrl(uri);
		// Если данные параметров прокси-сервера получены
		if(!this->worker.proxy.url.empty()){
			// Если протокол подключения SOCKS5
			if(this->worker.proxy.url.schema.compare("socks5") == 0)
				// Устанавливаем тип прокси-сервера
				this->worker.proxy.type = proxy_t::type_t::SOCKS5;
			// Если протокол подключения HTTP
			else if((this->worker.proxy.url.schema.compare("http") == 0) ||
			(this->worker.proxy.url.schema.compare("https") == 0))
				// Устанавливаем тип прокси-сервера
				this->worker.proxy.type = proxy_t::type_t::HTTP;
			// Если требуется авторизация на прокси-сервере
			if(!this->worker.proxy.url.user.empty() && !this->worker.proxy.url.pass.empty())
				// Устанавливаем данные пользователя
				this->worker.proxy.http->setUser(this->worker.proxy.url.user, this->worker.proxy.url.pass);
		}
	}
}
/**
 * setKeepAlive Метод установки флага автоматического поддержания подключения
 * @param mode флаг автоматического поддержания подключения
 */
void awh::Rest::setKeepAlive(const bool mode) noexcept {
	// Устанавливаем флаг поддержания автоматического подключения
	this->worker.alive = mode;
}
/**
 * setChunkSize Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::Rest::setChunkSize(const size_t size) noexcept {
	// Устанавливаем размер чанка
	this->http->setChunkSize(size);
}
/**
 * setWaitMessage Метод установки флага ожидания входящих сообщений
 * @param mode флаг состояния разрешения проверки
 */
void awh::Rest::setWaitMessage(const bool mode) noexcept {
	// Устанавливаем флаг ожидания входящих сообщений
	this->worker.wait = mode;
}
/**
 * setAttempts Метод установки количества попыток переподключения
 * @param count количество попыток переподключения
 */
void awh::Rest::setAttempts(const u_short count) noexcept {
	// Устанавливаем количество попыток переподключения
	this->worker.attempts.second = count;
}
/**
 * setUserAgent Метод установки User-Agent для HTTP запроса
 * @param userAgent агент пользователя для HTTP запроса
 */
void awh::Rest::setUserAgent(const string & userAgent) noexcept {
	// Устанавливаем UserAgent
	if(!userAgent.empty()){
		// Устанавливаем пользовательского агента
		this->http->setUserAgent(userAgent);
		// Устанавливаем пользовательского агента для прокси-сервера
		this->worker.proxy.http->setUserAgent(userAgent);
	}
}
/**
 * setCompress Метод установки метода сжатия
 * @param метод сжатия сообщений
 */
void awh::Rest::setCompress(const http_t::compress_t compress) noexcept {
	// Устанавливаем метод сжатия
	this->http->setCompress(compress);
}
/**
 * setUser Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::Rest::setUser(const string & login, const string & password) noexcept {
	// Если пользователь и пароль переданы
	if(!login.empty() && !password.empty())
		// Устанавливаем логин и пароль пользователя
		this->http->setUser(login, password);
}
/**
 * setServ Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::Rest::setServ(const string & id, const string & name, const string & ver) noexcept {
	// Устанавливаем данные сервиса
	this->http->setServ(id, name, ver);
	// Устанавливаем данные сервиса для прокси-сервера
	this->worker.proxy.http->setServ(id, name, ver);
}
/**
 * setCrypt Метод установки параметров шифрования
 * @param pass пароль шифрования передаваемых данных
 * @param salt соль шифрования передаваемых данных
 * @param aes  размер шифрования передаваемых данных
 */
void awh::Rest::setCrypt(const string & pass, const string & salt, const hash_t::aes_t aes) noexcept {
	// Устанавливаем параметры шифрования
	this->http->setCrypt(pass, salt, aes);
}
/**
 * setAuthType Метод установки типа авторизации
 * @param type      тип авторизации
 * @param algorithm алгоритм шифрования для Digest авторизации
 */
void awh::Rest::setAuthType(const auth_t::type_t type, const auth_t::algorithm_t algorithm) noexcept {
	// Если объект авторизации создан
	this->http->setAuthType(type, algorithm);
}
/**
 * setAuthTypeProxy Метод установки типа авторизации прокси-сервера
 * @param type      тип авторизации
 * @param algorithm алгоритм шифрования для Digest авторизации
 */
void awh::Rest::setAuthTypeProxy(const auth_t::type_t type, const auth_t::algorithm_t algorithm) noexcept {
	// Если объект авторизации создан
	this->worker.proxy.http->setAuthType(type, algorithm);
}
/**
 * Rest Конструктор
 * @param core объект биндинга TCP/IP
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::Rest::Rest(const core_t * core, const fmk_t * fmk, const log_t * log) noexcept : core(core), fmk(fmk), log(log), worker(fmk, log) {
	try {
		// Устанавливаем зависимые модули
		this->fmk = fmk;
		this->log = log;
		// Устанавливаем контекст сообщения
		this->worker.context = this;
		// Создаём объект для работы с сетью
		this->nwk = new network_t(this->fmk);
		// Создаём объект URI
		this->uri = new uri_t(this->fmk, this->nwk);
		// Создаём объект для работы с HTTP
		this->http = new http_t(this->fmk, this->log, this->uri);
		// Устанавливаем функцию обработки вызова для получения чанков
		this->http->setChunkingFn(&chunking);
		// Устанавливаем событие подключения
		this->worker.openFn = openCallback;
		// Устанавливаем функцию чтения данных
		this->worker.readFn = readCallback;
		// Устанавливаем событие отключения
		this->worker.closeFn = closeCallback;
		// Устанавливаем событие на запуск системы
		this->worker.startFn = startCallback;
		// Устанавливаем событие на подключение к прокси-серверу
		this->worker.openProxyFn = openProxyCallback;
		// Устанавливаем событие на чтение данных с прокси-сервера
		this->worker.readProxyFn = readProxyCallback;
		// Добавляем воркер в биндер TCP/IP
		const_cast <core_t *> (this->core)->add(&this->worker);
	// Если происходит ошибка то игнорируем её
	} catch(const bad_alloc&) {
		// Выводим сообщение об ошибке
		log->print("%s", log_t::flag_t::CRITICAL, "memory could not be allocated");
		// Выходим из приложения
		exit(EXIT_FAILURE);
	}
}
/**
 * ~Rest Деструктор
 */
awh::Rest::~Rest() noexcept {
	// Удаляем объект для работы с сетью
	if(this->nwk != nullptr) delete this->nwk;
	// Удаляем объект для работы с URI
	if(this->uri != nullptr) delete this->uri;
	// Удаляем объект работы с HTTP
	if(this->http != nullptr) delete this->http;
}
