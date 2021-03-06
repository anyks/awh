/**
 * @file: rest.cpp
 * @date: 2021-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2021
 */

// Подключаем заголовочный файл
#include <client/rest.hpp>

/**
 * chunking Метод обработки получения чанков
 * @param chunk бинарный буфер чанка
 * @param http  объект модуля HTTP
 * @param ctx   передаваемый контекст модуля
 */
void awh::client::Rest::chunking(const vector <char> & chunk, const awh::http_t * http, void * ctx) noexcept {
	// Выполняем блокировку неиспользуемой переменной
	(void) ctx;
	// Если данные получены, формируем тело сообщения
	if(!chunk.empty()) const_cast <awh::http_t *> (http)->addBody(chunk.data(), chunk.size());
}
/**
 * openCallback Функция обратного вызова при запуске работы
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::client::Rest::openCallback(const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Выполняем подключение
	reinterpret_cast <client::core_t *> (core)->open(wid);
}
/**
 * connectCallback Функция обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::client::Rest::connectCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		rest_t * web = reinterpret_cast <rest_t *> (ctx);
		// Запоминаем идентификатор адъютанта
		web->aid = aid;
		// Выполняем сброс параметров запроса
		web->flush();
		// Выполняем перебор всех подключений
		for(auto & req : web->requests){
			// Выполняем сброс состояния HTTP парсера
			web->http.reset();
			// Выполняем очистку параметров HTTP запроса
			web->http.clear();
			// Устанавливаем метод компрессии
			web->http.setCompress(web->compress);
			// Если список заголовков получен
			if(!req.headers.empty())
				// Устанавливаем заголовоки запроса
				web->http.setHeaders(req.headers);
			// Если тело запроса существует
			if(!req.entity.empty())
				// Устанавливаем тело запроса
				web->http.setBody(req.entity);
			// Получаем бинарные данные REST запроса
			const auto & request = web->http.request(req.url, req.method);
			// Если бинарные данные запроса получены
			if(!request.empty()){
				// Если включён режим отладки
				#if defined(DEBUG_MODE)
					// Выводим заголовок запроса
					cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
					// Выводим параметры запроса
					cout << string(request.begin(), request.end()) << endl << endl;
				#endif
				// Тело REST сообщения
				vector <char> entity;
				// Отправляем серверу сообщение
				core->write(request.data(), request.size(), aid);
				// Получаем данные тела запроса
				while(!(entity = web->http.payload()).empty()){
					// Если включён режим отладки
					#if defined(DEBUG_MODE)
						// Выводим сообщение о выводе чанка тела
						cout << web->fmk->format("<chunk %u>", entity.size()) << endl;
					#endif
					// Отправляем тело на сервер
					core->write(entity.data(), entity.size(), aid);
				}
			}
		}
		// Если функция обратного вызова существует
		if(web->activeFn != nullptr)
			// Выполняем функцию обратного вызова
			web->activeFn(mode_t::CONNECT, web, web->ctx.at(0));
	}
}
/**
 * disconnectCallback Функция обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::client::Rest::disconnectCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные переданы верные
	if((wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		rest_t * web = reinterpret_cast <rest_t *> (ctx);
		// Если список ответов получен
		if(!web->responses.empty() && !web->requests.empty()){
			// Получаем объект ответа
			const res_t & res = web->responses.front();
			// Если нужно произвести запрос заново
			if((res.code == 201) || (res.code == 301) ||
			   (res.code == 302) || (res.code == 303) ||
			   (res.code == 307) || (res.code == 308) ||
			   (res.code == 401) || (res.code == 407)){
				// Получаем объект запроса
				req_t & req = web->requests.front();
				// Устанавливаем новый URL адрес запроса
				req.url = web->http.getUrl();
				// Получаем новый адрес запроса для воркера
				web->worker.url = req.url;
				// Выполняем запрос заново
				reinterpret_cast <client::core_t *> (core)->open(web->worker.wid);
				// Выходим из функции
				return;
			}
		}
		// Получаем объект ответа
		res_t res = (!web->responses.empty() ? move(web->responses.front()) : res_t());
		// Если список ответов не получен, значит он был выведен ранее
		if(web->responses.empty()) res.code = 1;
		// Выполняем очистку списка запросов
		web->requests.clear();
		// Выполняем очистку списка ответов
		web->responses.clear();
		// Очищаем адрес сервера
		web->worker.url.clear();
		// Выполняем сброс параметров запроса
		web->flush();
		// Завершаем работу
		if(web->unbind) core->stop();
		// Если функция обратного вызова установлена, выводим сообщение
		if((res.code == 0) && (web->messageFn != nullptr)){
			// Устанавливаем код ответа сервера
			res.code = 500;
			// Выполняем функцию обратного вызова
			web->messageFn(res, web, web->ctx.at(1));
		}
		// Если функция обратного вызова существует
		if(web->activeFn != nullptr)
			// Выполняем функцию обратного вызова
			web->activeFn(mode_t::DISCONNECT, web, web->ctx.at(0));
	}
}
/**
 * connectProxyCallback Функция обратного вызова при подключении к прокси-серверу
 * @param aid  идентификатор адъютанта
 * @param wid  идентификатор воркера
 * @param core объект биндинга TCP/IP
 * @param ctx  передаваемый контекст модуля
 */
void awh::client::Rest::connectProxyCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (wid > 0) && (core != nullptr) && (ctx != nullptr)){
		// Получаем контекст модуля
		rest_t * web = reinterpret_cast <rest_t *> (ctx);
		// Запоминаем идентификатор адъютанта
		web->aid = aid;
		// Получаем объект запроса
		req_t & req = web->requests.front();
		// Определяем тип прокси-сервера
		switch((uint8_t) web->worker.proxy.type){
			// Если прокси-сервер является Socks5
			case (uint8_t) proxy_t::type_t::SOCKS5: {
				// Выполняем сброс состояния Socks5 парсера
				web->worker.proxy.socks5.reset();
				// Устанавливаем URL адрес запроса
				web->worker.proxy.socks5.setUrl(req.url);
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
				const auto & proxy = web->worker.proxy.http.proxy(req.url);
				// Если бинарные данные запроса получены
				if(!proxy.empty()){
					// Если включён режим отладки
					#if defined(DEBUG_MODE)
						// Выводим заголовок запроса
						cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST PROXY ^^^^^^^^^\x1B[0m" << endl;
						// Выводим параметры запроса
						cout << string(proxy.begin(), proxy.end()) << endl;
					#endif
					// Отправляем на прокси-сервер
					core->write(proxy.data(), proxy.size(), aid);
				}
			} break;
			// Иначе завершаем работу
			default: reinterpret_cast <client::core_t *> (core)->close(aid);
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
void awh::client::Rest::readCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (wid > 0)){
		// Получаем контекст модуля
		rest_t * web = reinterpret_cast <rest_t *> (ctx);
		// Добавляем полученные данные в буфер
		web->entity.insert(web->entity.end(), buffer, buffer + size);
		// Выполняем обработку полученных данных
		while(!web->active){
			// Получаем объект запроса
			req_t & req = web->requests.front();
			// Получаем объект ответа
			res_t & res = web->responses.front();
			// Выполняем парсинг полученных данных
			size_t bytes = web->http.parse(web->entity.data(), web->entity.size());
			// Если все данные получены
			if((res.ok = web->http.isEnd())){
				// Получаем параметры запроса
				auto query = web->http.getQuery();
				// Устанавливаем код ответа
				res.code = query.code;
				// Устанавливаем сообщение ответа
				res.message = query.message;
				// Если включён режим отладки
				#if defined(DEBUG_MODE)
					// Получаем данные ответа
					const auto & response = web->http.response(true);
					// Если параметры ответа получены
					if(!response.empty()){
						// Выводим заголовок ответа
						cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
						// Выводим параметры ответа
						cout << string(response.begin(), response.end()) << endl;
						// Если тело ответа существует
						if(!web->http.getBody().empty())
							// Выводим сообщение о выводе чанка тела
							cout << web->fmk->format("<body %u>", web->http.getBody().size()) << endl << endl;
						// Иначе устанавливаем перенос строки
						else cout << endl;
					}
				#endif
				// Выполняем анализ результата авторизации
				switch((uint8_t) web->http.getAuth()){
					// Если нужно попытаться ещё раз
					case (uint8_t) awh::http_t::stath_t::RETRY: {
						// Если попытка повторить авторизацию ещё не проводилась
						if(!req.failAuth){
							// Получаем новый адрес запроса
							req.url = web->http.getUrl();
							// Если адрес запроса получен
							if(!req.url.empty()){
								// Выполняем очистку оставшихся данных
								web->entity.clear();
								// Запоминаем, что попытка выполнена
								req.failAuth = true;
								// Если соединение является постоянным
								if(web->http.isAlive())
									// Выполняем повторно отправку сообщения на сервер
									connectCallback(aid, wid, core, ctx);
								// Если нам необходимо отключиться
								else {
									// Получаем новый адрес запроса для воркера
									web->worker.url = req.url;
									// Завершаем работу
									reinterpret_cast <client::core_t *> (core)->close(aid);
								}
								// Завершаем работу
								return;
							}
						}
						// Устанавливаем код ответа
						res.code = 403;
					} break;
					// Если запрос выполнен удачно
					case (uint8_t) awh::http_t::stath_t::GOOD: {
						// Получаем объект ответа
						res_t res = move(web->responses.front());
						// Если функция обратного вызова установлена, выводим сообщение
						if(web->messageFn != nullptr){
							// Получаем тело запроса
							const auto & entity = web->http.getBody();
							// Получаем заголовки ответа
							const auto & headers = web->http.getHeaders();
							// Устанавливаем тело ответа
							res.entity.assign(entity.begin(), entity.end());
							// Устанавливаем заголовки ответа
							res.headers = move(* const_cast <unordered_multimap <string, string> *> (&headers));
						}
						// Устанавливаем размер стопбайт
						if(!web->http.isAlive()){
							// Выполняем очистку оставшихся данных
							web->entity.clear();
							// Если функция обратного вызова установлена, выводим сообщение
							if(web->messageFn != nullptr)
								// Выполняем функцию обратного вызова
								web->messageFn(res, web, web->ctx.at(1));
							// Завершаем работу
							reinterpret_cast <client::core_t *> (core)->close(aid);
							// Выходим из функции
							return;
						}
						// Выполняем сброс состояния HTTP парсера
						web->http.reset();
						// Выполняем очистку параметров HTTP запроса
						web->http.clear();
						// Если объект ещё не удалён
						if(!web->requests.empty())
							// Выполняем удаление объекта запроса
							web->requests.erase(web->requests.begin());
						// Если объект ещё не удалён
						if(!web->responses.empty())
							// Выполняем удаление объекта ответа
							web->responses.erase(web->responses.begin());
						// Если функция обратного вызова установлена, выводим сообщение
						if(web->messageFn != nullptr)
							// Выполняем функцию обратного вызова
							web->messageFn(res, web, web->ctx.at(1));
						// Завершаем обработку
						goto Next;
					} break;
					// Если запрос неудачный
					case (uint8_t) awh::http_t::stath_t::FAULT: {
						// Запрещаем бесконечный редирект при запросе авторизации
						if((res.code == 401) || (res.code == 407)) res.code = 403;
					} break;
				}
				// Выполняем очистку оставшихся данных
				web->entity.clear();
				// Если функция обратного вызова установлена, выводим сообщение
				if(web->messageFn != nullptr){
					// Получаем тело запроса
					const auto & entity = web->http.getBody();
					// Получаем заголовки ответа
					const auto & headers = web->http.getHeaders();
					// Устанавливаем тело ответа
					res.entity.assign(entity.begin(), entity.end());
					// Устанавливаем заголовки ответа
					res.headers = move(* const_cast <unordered_multimap <string, string> *> (&headers));
					// Выполняем функцию обратного вызова
					web->messageFn(res, web, web->ctx.at(1));
				}
				// Завершаем работу
				reinterpret_cast <client::core_t *> (core)->close(aid);
				// Выходим из функции
				return;
			}
			// Устанавливаем метку продолжения обработки пайплайна
			Next:
			// Если парсер обработал какое-то количество байт
			if((bytes > 0) && !web->entity.empty()){
				// Если размер буфера больше количества удаляемых байт
				if(web->entity.size() >= bytes)
					// Удаляем количество обработанных байт
					vector <decltype(web->entity)::value_type> (web->entity.begin() + bytes, web->entity.end()).swap(web->entity);
				// Если байт в буфере меньше, просто очищаем буфер
				else web->entity.clear();
				// Если данных для обработки не осталось, выходим
				if(web->entity.empty()) break;
			// Если данных для обработки недостаточно, выходим
			} else break;
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
void awh::client::Rest::readProxyCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (wid > 0)){
		// Получаем контекст модуля
		rest_t * web = reinterpret_cast <rest_t *> (ctx);
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
							reinterpret_cast <client::core_t *> (core)->switchProxy(aid);
							// Завершаем работу
							return;
						// Если рукопожатие не выполнено
						} else {
							// Получаем объект ответа
							res_t & res = web->responses.front();
							// Устанавливаем код ответа
							res.code = web->worker.proxy.socks5.getCode();
							// Устанавливаем сообщение ответа
							res.message = web->worker.proxy.socks5.getMessage(res.code);
							// Если включён режим отладки
							#if defined(DEBUG_MODE)
								// Если заголовки получены
								if(!res.message.empty()){
									// Данные REST ответа
									const string & response = web->fmk->format("SOCKS5 %u %s\r\n", res.code, res.message.c_str());
									// Выводим заголовок ответа
									cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE PROXY ^^^^^^^^^\x1B[0m" << endl;
									// Выводим параметры ответа
									cout << string(response.begin(), response.end()) << endl;
								}
							#endif
							// Если функция обратного вызова установлена, выводим сообщение
							if(web->messageFn != nullptr)
								// Выполняем функцию обратного вызова
								web->messageFn(res, web, web->ctx.at(1));
							// Завершаем работу
							reinterpret_cast <client::core_t *> (core)->close(aid);
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
					// Получаем объект запроса
					req_t & req = web->requests.front();
					// Получаем объект ответа
					res_t & res = web->responses.front();
					// Устанавливаем код ответа
					res.code = query.code;
					// Устанавливаем сообщение ответа
					res.message = query.message;
					// Если включён режим отладки
					#if defined(DEBUG_MODE)
						// Получаем данные ответа
						const auto & response = web->worker.proxy.http.response(true);
						// Если параметры ответа получены
						if(!response.empty()){
							// Выводим заголовок ответа
							cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE PROXY ^^^^^^^^^\x1B[0m" << endl;
							// Выводим параметры ответа
							cout << string(response.begin(), response.end()) << endl;
							// Если тело ответа существует
							if(!web->worker.proxy.http.getBody().empty())
								// Выводим сообщение о выводе чанка тела
								cout << web->fmk->format("<body %u>", web->worker.proxy.http.getBody().size())  << endl;
						}
					#endif
					// Выполняем проверку авторизации
					switch((uint8_t) web->worker.proxy.http.getAuth()){
						// Если нужно попытаться ещё раз
						case (uint8_t) awh::http_t::stath_t::RETRY: {
							// Если попытка повторить авторизацию ещё не проводилась
							if(!req.failAuth){
								// Получаем новый адрес запроса
								web->worker.proxy.url = web->worker.proxy.http.getUrl();
								// Если адрес запроса получен
								if(!web->worker.proxy.url.empty()){
									// Запоминаем, что попытка выполнена
									req.failAuth = true;
									// Если соединение является постоянным
									if(web->worker.proxy.http.isAlive())
										// Выполняем повторно отправку сообщения на сервер
										connectProxyCallback(aid, wid, core, ctx);
									// Завершаем работу
									else reinterpret_cast <client::core_t *> (core)->close(aid);
									// Завершаем работу
									return;
								}
							}
							// Устанавливаем код ответа
							res.code = 403;
						} break;
						// Если запрос выполнен удачно
						case (uint8_t) awh::http_t::stath_t::GOOD: {
							// Выполняем сброс количество попыток
							req.failAuth = false;
							// Выполняем переключение на работу с сервером
							reinterpret_cast <client::core_t *> (core)->switchProxy(aid);
							// Завершаем работу
							return;
						} break;
						// Если запрос неудачный
						case (uint8_t) awh::http_t::stath_t::FAULT: {
							// Запрещаем бесконечный редирект при запросе авторизации
							if((res.code == 401) || (res.code == 407))
								// Устанавливаем код ответа
								res.code = 403;
							// Получаем тело запроса
							const auto & entity = web->worker.proxy.http.getBody();
							// Устанавливаем заголовки ответа
							res.headers = web->worker.proxy.http.getHeaders();
							// Устанавливаем тело ответа
							res.entity.assign(entity.begin(), entity.end());
						} break;
					}
					// Если функция обратного вызова установлена, выводим сообщение
					if(web->messageFn != nullptr)
						// Выполняем функцию обратного вызова
						web->messageFn(res, web, web->ctx.at(1));
					// Завершаем работу
					reinterpret_cast <client::core_t *> (core)->close(aid);
				}
			} break;
			// Иначе завершаем работу
			default: reinterpret_cast <client::core_t *> (core)->close(aid);
		}
	}
}
/**
 * flush Метод сброса параметров запроса
 */
void awh::client::Rest::flush() noexcept {
	// Сбрасываем флаг принудительной остановки
	this->active = false;
	// Выполняем очистку оставшихся данных
	this->entity.clear();
}
/**
 * start Метод запуска клиента
 */
void awh::client::Rest::start() noexcept {
	// Если биндинг не запущен
	if(!this->core->working())
		// Выполняем запуск биндинга
		const_cast <client::core_t *> (this->core)->start();
	// Если биндинг уже запущен, выполняем запрос на сервер
	else const_cast <client::core_t *> (this->core)->open(this->worker.wid);
}
/**
 * stop Метод остановки клиента
 */
void awh::client::Rest::stop() noexcept {
	// Устанавливаем флаг принудительной остановки
	this->active = true;
	// Если подключение выполнено
	if(this->core->working()){
		// Выполняем очистку списка запросов
		this->requests.clear();
		// Выполняем очистку списка ответов
		this->responses.clear();
		// Очищаем адрес сервера
		this->worker.url.clear();
		// Завершаем работу, если разрешено остановить
		const_cast <client::core_t *> (this->core)->stop();
	}
}
/**
 * close Метод закрытия подключения клиента
 */
void awh::client::Rest::close() noexcept {
	// Устанавливаем флаг принудительной остановки
	this->active = true;
	// Если подключение выполнено
	if(this->core->working())
		// Завершаем работу, если разрешено остановить
		const_cast <client::core_t *> (this->core)->close(this->aid);
}
/**
 * GET Метод REST запроса
 * @param url     адрес запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
vector <char> awh::client::Rest::GET(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Результат работы функции
	vector <char> result;
	// Если данные запроса переданы
	if(!url.empty()){
		// Создаём объект запроса
		req_t req;
		// Устанавливаем URL адрес запроса
		req.url = url;
		// Запоминаем переданные заголовки
		req.headers = headers;
		// Устанавливаем метод запроса
		req.method = web_t::method_t::GET;
		// Подписываемся на событие получения сообщения
		this->on(&result, [](const res_t & res, rest_t * web, void * ctx){
			// Проверяем на наличие ошибок
			if(res.code >= 300)
				// Выводим сообщение о неудачном запросе
				web->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
			// Если тело ответа получено
			if(!res.entity.empty())
				// Формируем результат ответа
				reinterpret_cast <vector <char> *> (ctx)->assign(res.entity.begin(), res.entity.end());
			// Выполняем остановку
			web->stop();
		});
		// Выполняем формирование REST запроса
		this->REST({move(req)});
		// Запускаем выполнение запроса
		this->start();
	}
	// Выводим результат
	return result;
}
/**
 * DEL Метод запроса в формате HTTP методом DEL
 * @param url     адрес запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::Rest::DEL(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Результат работы функции
	vector <char> result;
	// Если данные запроса переданы
	if(!url.empty()){
		// Создаём объект запроса
		req_t req;
		// Устанавливаем URL адрес запроса
		req.url = url;
		// Запоминаем переданные заголовки
		req.headers = headers;
		// Устанавливаем метод запроса
		req.method = web_t::method_t::DEL;
		// Подписываемся на событие получения сообщения
		this->on(&result, [](const res_t & res, rest_t * web, void * ctx){
			// Проверяем на наличие ошибок
			if(res.code >= 300)
				// Выводим сообщение о неудачном запросе
				web->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
			// Если тело ответа получено
			if(!res.entity.empty())
				// Формируем результат ответа
				reinterpret_cast <vector <char> *> (ctx)->assign(res.entity.begin(), res.entity.end());
			// Выполняем остановку
			web->stop();
		});
		// Выполняем формирование REST запроса
		this->REST({move(req)});
		// Запускаем выполнение запроса
		this->start();
	}
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
vector <char> awh::client::Rest::PUT(const uri_t::url_t & url, const json & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Результат работы функции
	vector <char> result;
	// Если данные запроса переданы
	if(!url.empty() && !entity.empty()){
		// Создаём объект запроса
		req_t req;
		// Устанавливаем URL адрес запроса
		req.url = url;
		// Запоминаем переданные заголовки
		req.headers = headers;
		// Устанавливаем метод запроса
		req.method = web_t::method_t::PUT;
		// Получаем тело запроса
		const string body = entity.dump();
		// Устанавливаем тепло запроса
		req.entity.assign(body.begin(), body.end());
		// Добавляем заголовок типа контента
		req.headers.emplace("Content-Type", "application/json");
		// Подписываемся на событие получения сообщения
		this->on(&result, [](const res_t & res, rest_t * web, void * ctx){
			// Проверяем на наличие ошибок
			if(res.code >= 300)
				// Выводим сообщение о неудачном запросе
				web->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
			// Если тело ответа получено
			if(!res.entity.empty())
				// Формируем результат ответа
				reinterpret_cast <vector <char> *> (ctx)->assign(res.entity.begin(), res.entity.end());
			// Выполняем остановку
			web->stop();
		});
		// Выполняем формирование REST запроса
		this->REST({move(req)});
		// Запускаем выполнение запроса
		this->start();
	}
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
vector <char> awh::client::Rest::PUT(const uri_t::url_t & url, const vector <char> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Результат работы функции
	vector <char> result;
	// Если данные запроса переданы
	if(!url.empty()){
		// Создаём объект запроса
		req_t req;
		// Устанавливаем URL адрес запроса
		req.url = url;
		// Устанавливаем тепло запроса
		req.entity = entity;
		// Запоминаем переданные заголовки
		req.headers = headers;
		// Устанавливаем метод запроса
		req.method = web_t::method_t::PUT;
		// Подписываемся на событие получения сообщения
		this->on(&result, [](const res_t & res, rest_t * web, void * ctx){
			// Проверяем на наличие ошибок
			if(res.code >= 300)
				// Выводим сообщение о неудачном запросе
				web->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
			// Если тело ответа получено
			if(!res.entity.empty())
				// Формируем результат ответа
				reinterpret_cast <vector <char> *> (ctx)->assign(res.entity.begin(), res.entity.end());
			// Выполняем остановку
			web->stop();
		});
		// Выполняем формирование REST запроса
		this->REST({move(req)});
		// Запускаем выполнение запроса
		this->start();
	}
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
vector <char> awh::client::Rest::PUT(const uri_t::url_t & url, const unordered_multimap <string, string> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Результат работы функции
	vector <char> result;
	// Если данные запроса переданы
	if(!url.empty() && !entity.empty()){
		// Создаём объект запроса
		req_t req;
		// Устанавливаем URL адрес запроса
		req.url = url;
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
		// Запоминаем переданные заголовки
		req.headers = headers;
		// Устанавливаем метод запроса
		req.method = web_t::method_t::PUT;
		// Устанавливаем тепло запроса
		req.entity.assign(body.begin(), body.end());
		// Добавляем заголовок типа контента
		req.headers.emplace("Content-Type", "application/x-www-form-urlencoded");
		// Подписываемся на событие получения сообщения
		this->on(&result, [](const res_t & res, rest_t * web, void * ctx){
			// Проверяем на наличие ошибок
			if(res.code >= 300)
				// Выводим сообщение о неудачном запросе
				web->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
			// Если тело ответа получено
			if(!res.entity.empty())
				// Формируем результат ответа
				reinterpret_cast <vector <char> *> (ctx)->assign(res.entity.begin(), res.entity.end());
			// Выполняем остановку
			web->stop();
		});
		// Выполняем формирование REST запроса
		this->REST({move(req)});
		// Запускаем выполнение запроса
		this->start();
	}
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
vector <char> awh::client::Rest::POST(const uri_t::url_t & url, const json & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Результат работы функции
	vector <char> result;
	// Если данные запроса переданы
	if(!url.empty() && !entity.empty()){
		// Создаём объект запроса
		req_t req;
		// Устанавливаем URL адрес запроса
		req.url = url;
		// Запоминаем переданные заголовки
		req.headers = headers;
		// Устанавливаем метод запроса
		req.method = web_t::method_t::POST;
		// Получаем тело запроса
		const string body = entity.dump();
		// Устанавливаем тепло запроса
		req.entity.assign(body.begin(), body.end());
		// Добавляем заголовок типа контента
		req.headers.emplace("Content-Type", "application/json");
		// Подписываемся на событие получения сообщения
		this->on(&result, [](const res_t & res, rest_t * web, void * ctx){
			// Проверяем на наличие ошибок
			if(res.code >= 300)
				// Выводим сообщение о неудачном запросе
				web->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
			// Если тело ответа получено
			if(!res.entity.empty())
				// Формируем результат ответа
				reinterpret_cast <vector <char> *> (ctx)->assign(res.entity.begin(), res.entity.end());
			// Выполняем остановку
			web->stop();
		});
		// Выполняем формирование REST запроса
		this->REST({move(req)});
		// Запускаем выполнение запроса
		this->start();
	}
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
vector <char> awh::client::Rest::POST(const uri_t::url_t & url, const vector <char> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Результат работы функции
	vector <char> result;
	// Если данные запроса переданы
	if(!url.empty()){
		// Создаём объект запроса
		req_t req;
		// Устанавливаем URL адрес запроса
		req.url = url;
		// Устанавливаем тепло запроса
		req.entity = entity;
		// Запоминаем переданные заголовки
		req.headers = headers;
		// Устанавливаем метод запроса
		req.method = web_t::method_t::POST;
		// Подписываемся на событие получения сообщения
		this->on(&result, [](const res_t & res, rest_t * web, void * ctx){
			// Проверяем на наличие ошибок
			if(res.code >= 300)
				// Выводим сообщение о неудачном запросе
				web->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
			// Если тело ответа получено
			if(!res.entity.empty())
				// Формируем результат ответа
				reinterpret_cast <vector <char> *> (ctx)->assign(res.entity.begin(), res.entity.end());
			// Выполняем остановку
			web->stop();
		});
		// Выполняем формирование REST запроса
		this->REST({move(req)});
		// Запускаем выполнение запроса
		this->start();
	}
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
vector <char> awh::client::Rest::POST(const uri_t::url_t & url, const unordered_multimap <string, string> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Результат работы функции
	vector <char> result;
	// Если данные запроса переданы
	if(!url.empty() && !entity.empty()){
		// Создаём объект запроса
		req_t req;
		// Устанавливаем URL адрес запроса
		req.url = url;
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
		// Запоминаем переданные заголовки
		req.headers = headers;
		// Устанавливаем метод запроса
		req.method = web_t::method_t::POST;
		// Устанавливаем тепло запроса
		req.entity.assign(body.begin(), body.end());
		// Добавляем заголовок типа контента
		req.headers.emplace("Content-Type", "application/x-www-form-urlencoded");
		// Подписываемся на событие получения сообщения
		this->on(&result, [](const res_t & res, rest_t * web, void * ctx){
			// Проверяем на наличие ошибок
			if(res.code >= 300)
				// Выводим сообщение о неудачном запросе
				web->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
			// Если тело ответа получено
			if(!res.entity.empty())
				// Формируем результат ответа
				reinterpret_cast <vector <char> *> (ctx)->assign(res.entity.begin(), res.entity.end());
			// Выполняем остановку
			web->stop();
		});
		// Выполняем формирование REST запроса
		this->REST({move(req)});
		// Запускаем выполнение запроса
		this->start();
	}
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
vector <char> awh::client::Rest::PATCH(const uri_t::url_t & url, const json & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Результат работы функции
	vector <char> result;
	// Если данные запроса переданы
	if(!url.empty() && !entity.empty()){
		// Создаём объект запроса
		req_t req;
		// Устанавливаем URL адрес запроса
		req.url = url;
		// Запоминаем переданные заголовки
		req.headers = headers;
		// Устанавливаем метод запроса
		req.method = web_t::method_t::PATCH;
		// Получаем тело запроса
		const string body = entity.dump();
		// Устанавливаем тепло запроса
		req.entity.assign(body.begin(), body.end());
		// Добавляем заголовок типа контента
		req.headers.emplace("Content-Type", "application/json");
		// Подписываемся на событие получения сообщения
		this->on(&result, [](const res_t & res, rest_t * web, void * ctx){
			// Проверяем на наличие ошибок
			if(res.code >= 300)
				// Выводим сообщение о неудачном запросе
				web->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
			// Если тело ответа получено
			if(!res.entity.empty())
				// Формируем результат ответа
				reinterpret_cast <vector <char> *> (ctx)->assign(res.entity.begin(), res.entity.end());
			// Выполняем остановку
			web->stop();
		});
		// Выполняем формирование REST запроса
		this->REST({move(req)});
		// Запускаем выполнение запроса
		this->start();
	}
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
vector <char> awh::client::Rest::PATCH(const uri_t::url_t & url, const vector <char> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Результат работы функции
	vector <char> result;
	// Если данные запроса переданы
	if(!url.empty()){
		// Создаём объект запроса
		req_t req;
		// Устанавливаем URL адрес запроса
		req.url = url;
		// Устанавливаем тепло запроса
		req.entity = entity;
		// Запоминаем переданные заголовки
		req.headers = headers;
		// Устанавливаем метод запроса
		req.method = web_t::method_t::PATCH;
		// Подписываемся на событие получения сообщения
		this->on(&result, [](const res_t & res, rest_t * web, void * ctx){
			// Проверяем на наличие ошибок
			if(res.code >= 300)
				// Выводим сообщение о неудачном запросе
				web->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
			// Если тело ответа получено
			if(!res.entity.empty())
				// Формируем результат ответа
				reinterpret_cast <vector <char> *> (ctx)->assign(res.entity.begin(), res.entity.end());
			// Выполняем остановку
			web->stop();
		});
		// Выполняем формирование REST запроса
		this->REST({move(req)});
		// Запускаем выполнение запроса
		this->start();
	}
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
vector <char> awh::client::Rest::PATCH(const uri_t::url_t & url, const unordered_multimap <string, string> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Результат работы функции
	vector <char> result;
	// Если данные запроса переданы
	if(!url.empty() && !entity.empty()){
		// Создаём объект запроса
		req_t req;
		// Устанавливаем URL адрес запроса
		req.url = url;
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
		// Запоминаем переданные заголовки
		req.headers = headers;
		// Устанавливаем метод запроса
		req.method = web_t::method_t::PATCH;
		// Устанавливаем тепло запроса
		req.entity.assign(body.begin(), body.end());
		// Добавляем заголовок типа контента
		req.headers.emplace("Content-Type", "application/x-www-form-urlencoded");
		// Подписываемся на событие получения сообщения
		this->on(&result, [](const res_t & res, rest_t * web, void * ctx){
			// Проверяем на наличие ошибок
			if(res.code >= 300)
				// Выводим сообщение о неудачном запросе
				web->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
			// Если тело ответа получено
			if(!res.entity.empty())
				// Формируем результат ответа
				reinterpret_cast <vector <char> *> (ctx)->assign(res.entity.begin(), res.entity.end());
			// Выполняем остановку
			web->stop();
		});
		// Выполняем формирование REST запроса
		this->REST({move(req)});
		// Запускаем выполнение запроса
		this->start();
	}
	// Выводим результат
	return result;
}
/**
 * HEAD Метод запроса в формате HTTP методом HEAD
 * @param url     адрес запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
unordered_multimap <string, string> awh::client::Rest::HEAD(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Результат работы функции
	unordered_multimap <string, string> result;
	// Если данные запроса переданы
	if(!url.empty()){
		// Создаём объект запроса
		req_t req;
		// Устанавливаем URL адрес запроса
		req.url = url;
		// Запоминаем переданные заголовки
		req.headers = headers;
		// Устанавливаем метод запроса
		req.method = web_t::method_t::HEAD;
		// Подписываемся на событие получения сообщения
		this->on(&result, [](const res_t & res, rest_t * web, void * ctx){
			// Проверяем на наличие ошибок
			if(res.code >= 300)
				// Выводим сообщение о неудачном запросе
				web->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
			// Если заголовки ответа получены
			if(!res.headers.empty())
				// Формируем результат ответа
				(* reinterpret_cast <unordered_multimap <string, string> *> (ctx)) = res.headers;
			// Выполняем остановку
			web->stop();
		});
		// Выполняем формирование REST запроса
		this->REST({move(req)});
		// Запускаем выполнение запроса
		this->start();
	}
	// Выводим результат
	return result;
}
/**
 * TRACE Метод запроса в формате HTTP методом TRACE
 * @param url     адрес запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
unordered_multimap <string, string> awh::client::Rest::TRACE(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Результат работы функции
	unordered_multimap <string, string> result;
	// Если данные запроса переданы
	if(!url.empty()){
		// Создаём объект запроса
		req_t req;
		// Устанавливаем URL адрес запроса
		req.url = url;
		// Запоминаем переданные заголовки
		req.headers = headers;
		// Устанавливаем метод запроса
		req.method = web_t::method_t::TRACE;
		// Подписываемся на событие получения сообщения
		this->on(&result, [](const res_t & res, rest_t * web, void * ctx){
			// Проверяем на наличие ошибок
			if(res.code >= 300)
				// Выводим сообщение о неудачном запросе
				web->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
			// Если заголовки ответа получены
			if(!res.headers.empty())
				// Формируем результат ответа
				(* reinterpret_cast <unordered_multimap <string, string> *> (ctx)) = res.headers;
			// Выполняем остановку
			web->stop();
		});
		// Выполняем формирование REST запроса
		this->REST({move(req)});
		// Запускаем выполнение запроса
		this->start();
	}
	// Выводим результат
	return result;
}
/**
 * OPTIONS Метод запроса в формате HTTP методом OPTIONS
 * @param url     адрес запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
unordered_multimap <string, string> awh::client::Rest::OPTIONS(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Результат работы функции
	unordered_multimap <string, string> result;
	// Если данные запроса переданы
	if(!url.empty()){
		// Создаём объект запроса
		req_t req;
		// Устанавливаем URL адрес запроса
		req.url = url;
		// Запоминаем переданные заголовки
		req.headers = headers;
		// Устанавливаем метод запроса
		req.method = web_t::method_t::OPTIONS;
		// Подписываемся на событие получения сообщения
		this->on(&result, [](const res_t & res, rest_t * web, void * ctx){
			// Проверяем на наличие ошибок
			if(res.code >= 300)
				// Выводим сообщение о неудачном запросе
				web->log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
			// Если заголовки ответа получены
			if(!res.headers.empty())
				// Формируем результат ответа
				(* reinterpret_cast <unordered_multimap <string, string> *> (ctx)) = res.headers;
			// Выполняем остановку
			web->stop();
		});
		// Выполняем формирование REST запроса
		this->REST({move(req)});
		// Запускаем выполнение запроса
		this->start();
	}
	// Выводим результат
	return result;
}
/**
 * REST Метод запроса в формате HTTP
 * @param request список запросов
 */
void awh::client::Rest::REST(const vector <req_t> & request) noexcept {
	// Если список запросов передан
	if(!request.empty() && (this->messageFn != nullptr)){
		// Выполняем очистку воркера
		this->worker.clear();
		// Очищаем список запросов
		this->requests.clear();
		// Выполняем очистку списка ответов
		this->responses.clear();
		// Устанавливаем адрес подключения
		this->worker.url = request.front().url;
		// Добавляем объект ответа в список ответов
		this->responses.assign(request.size(), res_t());
		// Добавляем объект запроса в список запросов
		this->requests.assign(request.begin(), request.end());
	}
}
/**
 * sendTimeout Метод отправки сигнала таймаута
 */
void awh::client::Rest::sendTimeout() noexcept {
	// Если подключение выполнено
	if(this->core->working())
		// Отправляем сигнал принудительного таймаута
		const_cast <client::core_t *> (this->core)->sendTimeout(this->aid);
}
/**
 * on Метод установки функции обратного вызова при подключении/отключении
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова
 */
void awh::client::Rest::on(void * ctx, function <void (const mode_t, Rest *, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(0) = ctx;
	// Устанавливаем функцию обратного вызова
	this->activeFn = callback;
}
/**
 * on Метод установки функции обратного вызова при получении сообщения
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова
 */
void awh::client::Rest::on(void * ctx, function <void (const res_t &, Rest *, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(1) = ctx;
	// Устанавливаем функцию обратного вызова
	this->messageFn = callback;
}
/**
 * on Метод установки функции обратного вызова для получения чанков
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова
 */
void awh::client::Rest::on(void * ctx, function <void (const vector <char> &, const awh::http_t *, void *)> callback) noexcept {
	// Устанавливаем функцию обработки вызова для получения чанков
	this->http.setChunkingFn(ctx, callback);
}
/**
 * setWaitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::client::Rest::setWaitTimeDetect(const time_t read, const time_t write) noexcept {
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
void awh::client::Rest::setBytesDetect(const worker_t::mark_t read, const worker_t::mark_t write) noexcept {
	// Устанавливаем количество байт на чтение
	this->worker.markRead = read;
	// Устанавливаем количество байт на запись
	this->worker.markWrite = write;
}
/**
 * setMode Метод установки флага модуля
 * @param flag флаг модуля для установки
 */
void awh::client::Rest::setMode(const u_short flag) noexcept {
	// Устанавливаем флаг анбиндинга ядра сетевого модуля
	this->unbind = !(flag & (uint8_t) flag_t::NOTSTOP);
	// Устанавливаем флаг ожидания входящих сообщений
	this->worker.wait = (flag & (uint8_t) flag_t::WAITMESS);
	// Устанавливаем флаг поддержания автоматического подключения
	this->worker.alive = (flag & (uint8_t) flag_t::KEEPALIVE);
	// Устанавливаем флаг отложенных вызовов событий сокета
	const_cast <client::core_t *> (this->core)->setDefer(flag & (uint8_t) flag_t::DEFER);
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	const_cast <client::core_t *> (this->core)->setNoInfo(flag & (uint8_t) flag_t::NOINFO);
	// Выполняем установку флага проверки домена
	const_cast <client::core_t *> (this->core)->setVerifySSL(flag & (uint8_t) flag_t::VERIFYSSL);
}
/**
 * setProxy Метод установки прокси-сервера
 * @param uri параметры прокси-сервера
 */
void awh::client::Rest::setProxy(const string & uri) noexcept {
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
void awh::client::Rest::setChunkSize(const size_t size) noexcept {
	// Устанавливаем размер чанка
	this->http.setChunkSize(size);
}
/**
 * setUserAgent Метод установки User-Agent для HTTP запроса
 * @param userAgent агент пользователя для HTTP запроса
 */
void awh::client::Rest::setUserAgent(const string & userAgent) noexcept {
	// Устанавливаем UserAgent
	if(!userAgent.empty()){
		// Устанавливаем пользовательского агента
		this->http.setUserAgent(userAgent);
		// Устанавливаем пользовательского агента для прокси-сервера
		this->worker.proxy.http.setUserAgent(userAgent);
	}
}
/**
 * setCompress Метод установки метода компрессии
 * @param compress метод компрессии сообщений
 */
void awh::client::Rest::setCompress(const awh::http_t::compress_t compress) noexcept {
	// Устанавливаем метод компрессии
	this->compress = compress;
}
/**
 * setUser Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::client::Rest::setUser(const string & login, const string & password) noexcept {
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
void awh::client::Rest::setServ(const string & id, const string & name, const string & ver) noexcept {
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
void awh::client::Rest::setCrypt(const string & pass, const string & salt, const hash_t::aes_t aes) noexcept {
	// Устанавливаем параметры шифрования
	this->http.setCrypt(pass, salt, aes);
}
/**
 * setAuthType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::client::Rest::setAuthType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Если объект авторизации создан
	this->http.setAuthType(type, hash);
}
/**
 * setAuthTypeProxy Метод установки типа авторизации прокси-сервера
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::client::Rest::setAuthTypeProxy(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Если объект авторизации создан
	this->worker.proxy.http.setAuthType(type, hash);
}
/**
 * Rest Конструктор
 * @param core объект биндинга TCP/IP
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::client::Rest::Rest(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept : nwk(fmk), uri(fmk, &nwk), http(fmk, log, &uri), core(core), fmk(fmk), log(log), worker(fmk, log), compress(awh::http_t::compress_t::NONE) {
	// Устанавливаем контекст сообщения
	this->worker.ctx = this;
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
	// Устанавливаем функцию обработки вызова для получения чанков
	this->http.setChunkingFn(this, &chunking);
	// Добавляем воркер в биндер TCP/IP
	const_cast <client::core_t *> (this->core)->add(&this->worker);
}
