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
 * openCallback Функция обратного вызова при запуске работы
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::Rest::openCallback(const size_t wid, core_t * core, void * ctx) noexcept {
	// Выполняем подключение
	core->open(wid);
}
/**
 * connectCallback Функция обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::Rest::connectCallback(const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		restCli_t * web = reinterpret_cast <restCli_t *> (ctx);
		// Выполняем сброс состояния HTTP парсера
		web->http.reset();
		// Выполняем очистку параметров HTTP запроса
		web->http.clear();
		// Устанавливаем код сообщения
		web->res.code = 404;
		// Получаем само сообщение
		web->res.message = web->http.getMessage(web->res.code);
		// Если список заголовков получен
		if(!web->headers.empty()){
			// Переходим по всему списку заголовков
			for(auto & header : web->headers)
				// Устанавливаем заголовок
				web->http.addHeader(header.first, header.second);
		}
		// Если тело запроса существует
		if(!web->entity.empty())
			// Устанавливаем тело запроса
			web->http.addBody(web->entity.data(), web->entity.size());
		// Получаем бинарные данные REST запроса
		const auto & request = web->http.request(web->worker.url, web->method);
		// Если бинарные данные запроса получены
		if(!request.empty()){
			// Тело REST сообщения
			vector <char> entity;
			// Отправляем серверу сообщение
			core->write(request.data(), request.size(), aid);
			// Получаем данные тела запроса
			while(!(entity = web->http.chunkBody()).empty()){
				// Отправляем тело на сервер
				core->write(entity.data(), entity.size(), aid);
			}
		}
	}
}
/**
 * disconnectCallback Функция обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::Rest::disconnectCallback(const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные переданы верные
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		restCli_t * web = reinterpret_cast <restCli_t *> (ctx);
		// Если нужно произвести запрос заново
		if((web->res.code == 301) || (web->res.code == 308) ||
		   (web->res.code == 401) || (web->res.code == 407)){
			// Выполняем запрос заново
			core->open(web->worker.wid);
			// Выходим из функции
			return;
		}
		// Если код пришёл нулевой, восстанавливаем его
		if(web->res.code == 0){
			// Устанавливаем код сообщения
			web->res.code = 404;
			// Получаем само сообщение
			web->res.message = web->http.getMessage(web->res.code);
		}
		// Если функция обратного вызова установлена, выводим сообщение
		if(web->messageFn != nullptr)
			// Выполняем функцию обратного вызова
			web->messageFn(web->res, web->ctx);
		// Завершаем работу
		if(web->unbind) core->stop();
	}
}
/**
 * connectProxyCallback Функция обратного вызова при подключении к прокси-серверу
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::Rest::connectProxyCallback(const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		restCli_t * web = reinterpret_cast <restCli_t *> (ctx);
		// Определяем тип прокси-сервера
		switch((uint8_t) web->worker.proxy.type){
			// Если прокси-сервер является Socks5
			case (uint8_t) proxy_t::type_t::SOCKS5: {
				// Выполняем сброс состояния Socks5 парсера
				web->worker.proxy.socks5.reset();
				// Устанавливаем URL адрес запроса
				web->worker.proxy.socks5.setUrl(web->worker.url);
				// Выполняем создание буфера запроса
				web->worker.proxy.socks5.parse();
				// Получаем данные запроса
				const auto & socks5 = web->worker.proxy.socks5.get();
				// Если данные получены
				if(!socks5.empty()) core->write(socks5.data(), socks5.size(), aid);
			} break;
			// Если прокси-сервер является HTTP
			case (uint8_t) proxy_t::type_t::HTTP: {
				// Выполняем сброс состояния HTTP парсера
				web->worker.proxy.http.reset();
				// Выполняем очистку параметров HTTP запроса
				web->worker.proxy.http.clear();
				// Получаем бинарные данные REST запроса
				const auto & rest = web->worker.proxy.http.proxy(web->worker.url);
				// Если бинарные данные запроса получены, отправляем на прокси-сервер
				if(!rest.empty()) core->write(rest.data(), rest.size(), aid);
			} break;
			// Иначе завершаем работу
			default: core->close(aid);
		}
	}
}
/**
 * readCallback Функция обратного вызова при чтении сообщения с сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param wid    идентификатор воркера
 * @param core   объект биндинга TCP/IP
 * @param ctx    передаваемый контекст модуля
 */
void awh::Rest::readCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (wid > 0)){
		// Получаем контекст модуля
		restCli_t * web = reinterpret_cast <restCli_t *> (ctx);
		// Выполняем парсинг полученных данных
		web->http.parse(buffer, size);
		// Если все данные получены
		if(web->http.isEnd()){
			// Получаем параметры запроса
			const auto & query = web->http.getQuery();
			// Устанавливаем код ответа
			web->res.code = query.code;
			// Устанавливаем сообщение ответа
			web->res.message = query.message;
			// Выполняем проверку авторизации
			switch((uint8_t) web->http.getAuth()){
				// Если нужно попытаться ещё раз
				case (uint8_t) http_t::stath_t::RETRY: {
					// Если попытка повторить авторизацию ещё не проводилась
					if(!web->failAuth){
						// Получаем новый адрес запроса
						web->worker.url = web->http.getUrl();
						// Если адрес запроса получен
						if(!web->worker.url.empty()){
							// Запоминаем, что попытка выполнена
							web->failAuth = true;
							// Если соединение является постоянным
							if(web->http.isAlive())
								// Выполняем повторно отправку сообщения на сервер
								connectCallback(aid, wid, core, ctx);
							// Завершаем работу
							else core->close(aid);
							// Завершаем работу
							return;
						}
					}
					// Устанавливаем код ответа
					web->res.code = 403;
				} break;
				// Если запрос выполнен удачно
				case (uint8_t) http_t::stath_t::GOOD: web->res.ok = true;
				// Если запрос неудачный
				case (uint8_t) http_t::stath_t::FAULT: {
					// Запрещаем бесконечный редирект при запросе авторизации
					if((web->res.code == 401) || (web->res.code == 407))
						// Устанавливаем код ответа
						web->res.code = 403;
					// Получаем тело запроса
					const auto & entity = web->http.getBody();
					// Устанавливаем заголовки ответа
					web->res.headers = web->http.getHeaders();
					// Устанавливаем тело ответа
					web->res.entity.assign(entity.begin(), entity.end());
				} break;
			}
			// Выполняем сброс количество попыток
			web->failAuth = false;
			// Завершаем работу
			core->close(aid);
		}
	}
}
/**
 * readProxyCallback Функция обратного вызова при чтении сообщения с прокси-сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param wid    идентификатор воркера
 * @param core   объект биндинга TCP/IP
 * @param ctx    передаваемый контекст модуля
 */
void awh::Rest::readProxyCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (wid > 0)){
		// Получаем контекст модуля
		restCli_t * web = reinterpret_cast <restCli_t *> (ctx);
		// Определяем тип прокси-сервера
		switch((uint8_t) web->worker.proxy.type){
			// Если прокси-сервер является Socks5
			case (uint8_t) proxy_t::type_t::SOCKS5: {
				// Если данные не получены
				if(!web->worker.proxy.socks5.isEnd()){
					// Выполняем парсинг входящих данных
					web->worker.proxy.socks5.parse(buffer, size);
					// Получаем данные запроса
					const auto & socks5 = web->worker.proxy.socks5.get();
					// Если данные получены
					if(!socks5.empty()) core->write(socks5.data(), socks5.size(), aid);
					// Если данные все получены
					else if(web->worker.proxy.socks5.isEnd()) {
						// Если рукопожатие выполнено
						if(web->worker.proxy.socks5.isHandshake()){
							// Выполняем переключение на работу с сервером
							reinterpret_cast <coreCli_t *> (core)->switchProxy(aid);
							// Завершаем работу
							return;
						// Если рукопожатие не выполнено
						} else {
							// Устанавливаем код ответа
							web->res.code = web->worker.proxy.socks5.getCode();
							// Устанавливаем сообщение ответа
							web->res.message = web->worker.proxy.socks5.getMessage(web->res.code);
							// Завершаем работу
							core->close(aid);
						}
					}
				}
			} break;
			// Если прокси-сервер является HTTP
			case (uint8_t) proxy_t::type_t::HTTP: {
				// Выполняем парсинг полученных данных
				web->worker.proxy.http.parse(buffer, size);
				// Если все данные получены
				if(web->worker.proxy.http.isEnd()){
					// Получаем параметры запроса
					const auto & query = web->worker.proxy.http.getQuery();
					// Устанавливаем код ответа
					web->res.code = query.code;
					// Устанавливаем сообщение ответа
					web->res.message = query.message;
					// Выполняем проверку авторизации
					switch((uint8_t) web->worker.proxy.http.getAuth()){
						// Если нужно попытаться ещё раз
						case (uint8_t) http_t::stath_t::RETRY: {
							// Если попытка повторить авторизацию ещё не проводилась
							if(!web->failAuth){
								// Получаем новый адрес запроса
								web->worker.proxy.url = web->worker.proxy.http.getUrl();
								// Если адрес запроса получен
								if(!web->worker.proxy.url.empty()){
									// Запоминаем, что попытка выполнена
									web->failAuth = true;
									// Если соединение является постоянным
									if(web->http.isAlive())
										// Выполняем повторно отправку сообщения на сервер
										connectProxyCallback(aid, wid, core, ctx);
									// Завершаем работу
									else core->close(aid);
									// Завершаем работу
									return;
								}
							}
							// Устанавливаем код ответа
							web->res.code = 403;
						} break;
						// Если запрос выполнен удачно
						case (uint8_t) http_t::stath_t::GOOD: {
							// Выполняем сброс количество попыток
							web->failAuth = false;
							// Выполняем переключение на работу с сервером
							reinterpret_cast <coreCli_t *> (core)->switchProxy(aid);
							// Завершаем работу
							return;
						} break;
						// Если запрос неудачный
						case (uint8_t) http_t::stath_t::FAULT: {
							// Запрещаем бесконечный редирект при запросе авторизации
							if((web->res.code == 401) || (web->res.code == 407))
								// Устанавливаем код ответа
								web->res.code = 403;
							// Получаем тело запроса
							const auto & entity = web->worker.proxy.http.getBody();
							// Устанавливаем заголовки ответа
							web->res.headers = web->worker.proxy.http.getHeaders();
							// Устанавливаем тело ответа
							web->res.entity.assign(entity.begin(), entity.end());
						} break;
					}
					// Выполняем сброс количество попыток
					web->failAuth = false;
					// Завершаем работу
					core->close(aid);
				}
			} break;
			// Иначе завершаем работу
			default: core->close(aid);
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
		// Устанавливаем функцию обратного вызова
		this->setMessageCallback(this, [](const res_t & res, void * ctx) noexcept {
			// Проверяем на наличие ошибок
			if(!res.ok){
				// Получаем объект работы с REST запросами
				restCli_t * web = reinterpret_cast <restCli_t *> (ctx);
				// Выводим сообщение о неудачном запросе
				web->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
			}
		});
		// Выполняем REST запрос на сервер
		this->REST(url, http_t::method_t::GET, {}, headers);
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
		// Устанавливаем функцию обратного вызова
		this->setMessageCallback(this, [](const res_t & res, void * ctx) noexcept {
			// Проверяем на наличие ошибок
			if(!res.ok){
				// Получаем объект работы с REST запросами
				restCli_t * web = reinterpret_cast <restCli_t *> (ctx);
				// Выводим сообщение о неудачном запросе
				web->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
			}
		});
		// Выполняем REST запрос на сервер
		this->REST(url, http_t::method_t::DEL, {}, headers);
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
		// Устанавливаем функцию обратного вызова
		this->setMessageCallback(this, [](const res_t & res, void * ctx) noexcept {
			// Проверяем на наличие ошибок
			if(!res.ok){
				// Получаем объект работы с REST запросами
				restCli_t * web = reinterpret_cast <restCli_t *> (ctx);
				// Выводим сообщение о неудачном запросе
				web->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
			}
		});
		// Выполняем REST запрос на сервер
		this->REST(url, http_t::method_t::PUT, vector <char> (body.begin(), body.end()), headers);
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
		// Устанавливаем функцию обратного вызова
		this->setMessageCallback(this, [](const res_t & res, void * ctx) noexcept {
			// Проверяем на наличие ошибок
			if(!res.ok){
				// Получаем объект работы с REST запросами
				restCli_t * web = reinterpret_cast <restCli_t *> (ctx);
				// Выводим сообщение о неудачном запросе
				web->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
			}
		});
		// Выполняем REST запрос на сервер
		this->REST(url, http_t::method_t::PUT, entity, headers);
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
			body.append(this->uri.urlEncode(param.first));
			// Добавляем разделитель
			body.append("=");
			// Добавляем значение
			body.append(this->uri.urlEncode(param.second));
		}
		// Добавляем заголовок типа контента
		const_cast <unordered_multimap <string, string> *> (&headers)->emplace("Content-Type", "application/x-www-form-urlencoded");
		// Устанавливаем функцию обратного вызова
		this->setMessageCallback(this, [](const res_t & res, void * ctx) noexcept {
			// Проверяем на наличие ошибок
			if(!res.ok){
				// Получаем объект работы с REST запросами
				restCli_t * web = reinterpret_cast <restCli_t *> (ctx);
				// Выводим сообщение о неудачном запросе
				web->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
			}
		});
		// Выполняем REST запрос на сервер
		this->REST(url, http_t::method_t::PUT, vector <char> (body.begin(), body.end()), headers);
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
		// Устанавливаем функцию обратного вызова
		this->setMessageCallback(this, [](const res_t & res, void * ctx) noexcept {
			// Проверяем на наличие ошибок
			if(!res.ok){
				// Получаем объект работы с REST запросами
				restCli_t * web = reinterpret_cast <restCli_t *> (ctx);
				// Выводим сообщение о неудачном запросе
				web->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
			}
		});
		// Выполняем REST запрос на сервер
		this->REST(url, http_t::method_t::POST, vector <char> (body.begin(), body.end()), headers);
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
		// Устанавливаем функцию обратного вызова
		this->setMessageCallback(this, [](const res_t & res, void * ctx) noexcept {
			// Проверяем на наличие ошибок
			if(!res.ok){
				// Получаем объект работы с REST запросами
				restCli_t * web = reinterpret_cast <restCli_t *> (ctx);
				// Выводим сообщение о неудачном запросе
				web->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
			}
		});
		// Выполняем REST запрос на сервер
		this->REST(url, http_t::method_t::POST, entity, headers);
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
			body.append(this->uri.urlEncode(param.first));
			// Добавляем разделитель
			body.append("=");
			// Добавляем значение
			body.append(this->uri.urlEncode(param.second));
		}
		// Добавляем заголовок типа контента
		const_cast <unordered_multimap <string, string> *> (&headers)->emplace("Content-Type", "application/x-www-form-urlencoded");
		// Устанавливаем функцию обратного вызова
		this->setMessageCallback(this, [](const res_t & res, void * ctx) noexcept {
			// Проверяем на наличие ошибок
			if(!res.ok){
				// Получаем объект работы с REST запросами
				restCli_t * web = reinterpret_cast <restCli_t *> (ctx);
				// Выводим сообщение о неудачном запросе
				web->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
			}
		});
		// Выполняем REST запрос на сервер
		this->REST(url, http_t::method_t::POST, vector <char> (body.begin(), body.end()), headers);
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
		// Устанавливаем функцию обратного вызова
		this->setMessageCallback(this, [](const res_t & res, void * ctx) noexcept {
			// Проверяем на наличие ошибок
			if(!res.ok){
				// Получаем объект работы с REST запросами
				restCli_t * web = reinterpret_cast <restCli_t *> (ctx);
				// Выводим сообщение о неудачном запросе
				web->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
			}
		});
		// Выполняем REST запрос на сервер
		this->REST(url, http_t::method_t::PATCH, vector <char> (body.begin(), body.end()), headers);
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
		// Устанавливаем функцию обратного вызова
		this->setMessageCallback(this, [](const res_t & res, void * ctx) noexcept {
			// Проверяем на наличие ошибок
			if(!res.ok){
				// Получаем объект работы с REST запросами
				restCli_t * web = reinterpret_cast <restCli_t *> (ctx);
				// Выводим сообщение о неудачном запросе
				web->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
			}
		});
		// Выполняем REST запрос на сервер
		this->REST(url, http_t::method_t::PATCH, entity, headers);
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
			body.append(this->uri.urlEncode(param.first));
			// Добавляем разделитель
			body.append("=");
			// Добавляем значение
			body.append(this->uri.urlEncode(param.second));
		}
		// Добавляем заголовок типа контента
		const_cast <unordered_multimap <string, string> *> (&headers)->emplace("Content-Type", "application/x-www-form-urlencoded");
		// Устанавливаем функцию обратного вызова
		this->setMessageCallback(this, [](const res_t & res, void * ctx) noexcept {
			// Проверяем на наличие ошибок
			if(!res.ok){
				// Получаем объект работы с REST запросами
				restCli_t * web = reinterpret_cast <restCli_t *> (ctx);
				// Выводим сообщение о неудачном запросе
				web->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
			}
		});
		// Выполняем REST запрос на сервер
		this->REST(url, http_t::method_t::PATCH, vector <char> (body.begin(), body.end()), headers);
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
		// Устанавливаем функцию обратного вызова
		this->setMessageCallback(this, [](const res_t & res, void * ctx) noexcept {
			// Проверяем на наличие ошибок
			if(!res.ok){
				// Получаем объект работы с REST запросами
				restCli_t * web = reinterpret_cast <restCli_t *> (ctx);
				// Выводим сообщение о неудачном запросе
				web->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
			}
		});
		// Выполняем REST запрос на сервер
		this->REST(url, http_t::method_t::HEAD, {}, headers);
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
		// Устанавливаем функцию обратного вызова
		this->setMessageCallback(this, [](const res_t & res, void * ctx) noexcept {
			// Проверяем на наличие ошибок
			if(!res.ok){
				// Получаем объект работы с REST запросами
				restCli_t * web = reinterpret_cast <restCli_t *> (ctx);
				// Выводим сообщение о неудачном запросе
				web->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
			}
		});
		// Выполняем REST запрос на сервер
		this->REST(url, http_t::method_t::TRACE, {}, headers);
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
		// Устанавливаем функцию обратного вызова
		this->setMessageCallback(this, [](const res_t & res, void * ctx) noexcept {
			// Проверяем на наличие ошибок
			if(!res.ok){
				// Получаем объект работы с REST запросами
				restCli_t * web = reinterpret_cast <restCli_t *> (ctx);
				// Выводим сообщение о неудачном запросе
				web->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
			}
		});
		// Выполняем REST запрос на сервер
		this->REST(url, http_t::method_t::OPTIONS, {}, headers);
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
 */
void awh::Rest::REST(const uri_t::url_t & url, http_t::method_t method, vector <char> entity, unordered_multimap <string, string> headers) noexcept {
	// Если параметры и метод запроса переданы
	if(!url.empty() && (method != http_t::method_t::NONE) && (this->messageFn != nullptr)){
		// Выполняем сброс предыдущего результата
		this->res = res_t();
		// Выполняем очистку воркера
		this->worker.clear();
		// Устанавливаем метод запроса
		this->method = method;
		// Устанавливаем URL адрес запроса
		this->worker.url = url;
		// Устанавливаем тело запроса
		this->entity = move(entity);
		// Запоминаем переданные заголовки
		this->headers = move(headers);
		// Устанавливаем метод сжатия
		this->http.setCompress(this->compress);
		// Если биндинг не запущен
		if(!this->core->working())
			// Выполняем запуск биндинга
			const_cast <coreCli_t *> (this->core)->start();
		// Если биндинг уже запущен, выполняем запрос на сервер
		else const_cast <coreCli_t *> (this->core)->open(this->worker.wid);
	}
}
/**
 * setChunkingFn Метод установки функции обратного вызова для получения чанков
 * @param callback функция обратного вызова
 */
void awh::Rest::setChunkingFn(function <void (const vector <char> &, const http_t *)> callback) noexcept {
	// Устанавливаем функцию обработки вызова для получения чанков
	this->http.setChunkingFn(callback);
}
/**
 * setMessageCallback Метод установки функции обратного вызова при получении сообщения
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова
 */
void awh::Rest::setMessageCallback(void * ctx, function <void (const res_t &, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx = ctx;
	// Устанавливаем функцию обратного вызова
	this->messageFn = callback;
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
 * setBytesDetect Метод детекции сообщений по количеству байт
 * @param read  количество байт для детекции по чтению
 * @param write количество байт для детекции по записи
 */
void awh::Rest::setBytesDetect(const worker_t::mark_t read, const worker_t::mark_t write) noexcept {
	// Устанавливаем количество байт на чтение
	this->worker.markRead = read;
	// Устанавливаем количество байт на запись
	this->worker.markWrite = write;
}
/**
 * setMode Метод установки флага модуля
 * @param flag флаг модуля для установки
 */
void awh::Rest::setMode(const u_short flag) noexcept {
	// Устанавливаем флаг анбиндинга ядра сетевого модуля
	this->unbind = !(flag & (uint8_t) flag_t::NOTSTOP);
	// Устанавливаем флаг ожидания входящих сообщений
	this->worker.wait = (flag & (uint8_t) flag_t::WAITMESS);
	// Устанавливаем флаг поддержания автоматического подключения
	this->worker.alive = (flag & (uint8_t) flag_t::KEEPALIVE);
	// Устанавливаем флаг отложенных вызовов событий сокета
	const_cast <coreCli_t *> (this->core)->setDefer(flag & (uint8_t) flag_t::DEFER);
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	const_cast <coreCli_t *> (this->core)->setNoInfo(flag & (uint8_t) flag_t::NOINFO);
	// Выполняем установку флага проверки домена
	const_cast <coreCli_t *> (this->core)->setVerifySSL(flag & (uint8_t) flag_t::VERIFYSSL);
}
/**
 * setProxy Метод установки прокси-сервера
 * @param uri параметры прокси-сервера
 */
void awh::Rest::setProxy(const string & uri) noexcept {
	// Если URI параметры переданы
	if(!uri.empty()){
		// Устанавливаем параметры прокси-сервера
		this->worker.proxy.url = this->uri.parseUrl(uri);
		// Если данные параметров прокси-сервера получены
		if(!this->worker.proxy.url.empty()){
			// Если протокол подключения SOCKS5
			if(this->worker.proxy.url.schema.compare("socks5") == 0){
				// Устанавливаем тип прокси-сервера
				this->worker.proxy.type = proxy_t::type_t::SOCKS5;
				// Если требуется авторизация на прокси-сервере
				if(!this->worker.proxy.url.user.empty() && !this->worker.proxy.url.pass.empty())
					// Устанавливаем данные пользователя
					this->worker.proxy.socks5.setUser(this->worker.proxy.url.user, this->worker.proxy.url.pass);
			// Если протокол подключения HTTP
			} else if((this->worker.proxy.url.schema.compare("http") == 0) || (this->worker.proxy.url.schema.compare("https") == 0)) {
				// Устанавливаем тип прокси-сервера
				this->worker.proxy.type = proxy_t::type_t::HTTP;
				// Если требуется авторизация на прокси-сервере
				if(!this->worker.proxy.url.user.empty() && !this->worker.proxy.url.pass.empty())
					// Устанавливаем данные пользователя
					this->worker.proxy.http.setUser(this->worker.proxy.url.user, this->worker.proxy.url.pass);
			}
		}
	}
}
/**
 * setChunkSize Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::Rest::setChunkSize(const size_t size) noexcept {
	// Устанавливаем размер чанка
	this->http.setChunkSize(size);
}
/**
 * setAttempts Метод установки количества попыток переподключения
 * @param count количество попыток переподключения
 */
void awh::Rest::setAttempts(const u_short count) noexcept {
	// Устанавливаем количество попыток переподключения
	this->worker.attempts = count;
}
/**
 * setUserAgent Метод установки User-Agent для HTTP запроса
 * @param userAgent агент пользователя для HTTP запроса
 */
void awh::Rest::setUserAgent(const string & userAgent) noexcept {
	// Устанавливаем UserAgent
	if(!userAgent.empty()){
		// Устанавливаем пользовательского агента
		this->http.setUserAgent(userAgent);
		// Устанавливаем пользовательского агента для прокси-сервера
		this->worker.proxy.http.setUserAgent(userAgent);
	}
}
/**
 * setCompress Метод установки метода сжатия
 * @param метод сжатия сообщений
 */
void awh::Rest::setCompress(const http_t::compress_t compress) noexcept {
	// Устанавливаем метод компрессии
	this->compress = compress;
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
		this->http.setUser(login, password);
}
/**
 * setServ Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::Rest::setServ(const string & id, const string & name, const string & ver) noexcept {
	// Устанавливаем данные сервиса
	this->http.setServ(id, name, ver);
	// Устанавливаем данные сервиса для прокси-сервера
	this->worker.proxy.http.setServ(id, name, ver);
}
/**
 * setCrypt Метод установки параметров шифрования
 * @param pass пароль шифрования передаваемых данных
 * @param salt соль шифрования передаваемых данных
 * @param aes  размер шифрования передаваемых данных
 */
void awh::Rest::setCrypt(const string & pass, const string & salt, const hash_t::aes_t aes) noexcept {
	// Устанавливаем параметры шифрования
	this->http.setCrypt(pass, salt, aes);
}
/**
 * setAuthType Метод установки типа авторизации
 * @param type тип авторизации
 * @param alg  алгоритм шифрования для Digest авторизации
 */
void awh::Rest::setAuthType(const auth_t::type_t type, const auth_t::alg_t alg) noexcept {
	// Если объект авторизации создан
	this->http.setAuthType(type, alg);
}
/**
 * setAuthTypeProxy Метод установки типа авторизации прокси-сервера
 * @param type тип авторизации
 * @param alg  алгоритм шифрования для Digest авторизации
 */
void awh::Rest::setAuthTypeProxy(const auth_t::type_t type, const auth_t::alg_t alg) noexcept {
	// Если объект авторизации создан
	this->worker.proxy.http.setAuthType(type, alg);
}
/**
 * Rest Конструктор
 * @param core объект биндинга TCP/IP
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::Rest::Rest(const coreCli_t * core, const fmk_t * fmk, const log_t * log) noexcept : nwk(fmk), uri(fmk, &nwk), http(fmk, log, &uri), core(core), fmk(fmk), log(log), worker(fmk, log), compress(http_t::compress_t::NONE) {
	// Устанавливаем контекст сообщения
	this->worker.ctx = this;
	// Устанавливаем функцию обработки вызова для получения чанков
	this->http.setChunkingFn(&chunking);
	// Устанавливаем событие на запуск системы
	this->worker.openFn = openCallback;
	// Устанавливаем функцию чтения данных
	this->worker.readFn = readCallback;
	// Устанавливаем событие подключения
	this->worker.connectFn = connectCallback;
	// Устанавливаем событие на чтение данных с прокси-сервера
	this->worker.readProxyFn = readProxyCallback;
	// Устанавливаем событие отключения
	this->worker.disconnectFn = disconnectCallback;
	// Устанавливаем событие на подключение к прокси-серверу
	this->worker.connectProxyFn = connectProxyCallback;
	// Добавляем воркер в биндер TCP/IP
	const_cast <coreCli_t *> (this->core)->add(&this->worker);
}
