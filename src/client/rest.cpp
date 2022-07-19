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
	// Получаем контекст модуля
	rest_t * web = reinterpret_cast <rest_t *> (ctx);
	// Если дисконнекта ещё не произошло
	if(web->action == action_t::NONE){
		// Устанавливаем экшен выполнения
		web->action = action_t::OPEN;
		// Выполняем запуск обработчика событий
		web->handler();
	}
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
		// Устанавливаем экшен выполнения
		web->action = action_t::CONNECT;
		// Выполняем запуск обработчика событий
		web->handler();
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
		// Устанавливаем экшен выполнения
		web->action = action_t::DISCONNECT;
		// Выполняем запуск обработчика событий
		web->handler();
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
		// Устанавливаем экшен выполнения
		web->action = action_t::PROXY_CONNECT;
		// Выполняем запуск обработчика событий
		web->handler();
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
		// Если дисконнекта ещё не произошло
		if((web->action == action_t::NONE) || (web->action == action_t::READ)){
			// Устанавливаем экшен выполнения
			web->action = action_t::READ;
			// Добавляем полученные данные в буфер
			web->entity.insert(web->entity.end(), buffer, buffer + size);
			// Выполняем запуск обработчика событий
			web->handler();
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
		// Если дисконнекта ещё не произошло
		if((web->action == action_t::NONE) || (web->action == action_t::PROXY_READ)){
			// Устанавливаем экшен выполнения
			web->action = action_t::PROXY_READ;
			// Добавляем полученные данные в буфер
			web->entity.insert(web->entity.end(), buffer, buffer + size);
			// Выполняем запуск обработчика событий
			web->handler();
		}
	}
}
/**
 * handler Метод управления входящими методами
 */
void awh::client::Rest::handler() noexcept {
	// Если управляющий блокировщик не заблокирован
	if(!this->locker.mode){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->locker.mtx);
		// Выполняем блокировку обработчика
		this->locker.mode = true;
		// Устанавливаем метку обработчика
		repeat:
		// Определяем обрабатываемый экшен
		switch((uint8_t) this->action){
			// Если необходимо запустить экшен открытия подключения
			case (uint8_t) action_t::OPEN:
				// Выполняем экшен открытия подключения
				this->actionOpen();
			break;
			// Если необходимо запустить экшен обработки данных поступающих с сервера
			case (uint8_t) action_t::READ:
				// Выполняем экшен обработки данных поступающих с сервера
				this->actionRead();
			break;
			// Если необходимо запустить экшен обработки подключения к серверу
			case (uint8_t) action_t::CONNECT:
				// Выполняем экшен обработки подключения к серверу
				this->actionConnect();
			break;
			// Если необходимо запустить экшен обработки отключения от сервера
			case (uint8_t) action_t::DISCONNECT:
				// Выполняем экшен обработки отключения от сервера
				this->actionDisconnect();
			break;
			// Если необходимо запустить экшен обработки данных поступающих с прокси-сервера
			case (uint8_t) action_t::PROXY_READ:
				// Выполняем экшен обработки данных поступающих с прокси-сервера
				this->actionConnectProxy();
			break;
			// Если необходимо запустить экшен обработки подключения к прокси-серверу
			case (uint8_t) action_t::PROXY_CONNECT:
				// Выполняем экшен обработки подключения к прокси-серверу
				this->actionReadProxy();
			break;
		}
		// Если требуется запустить следующий экшен
		if(this->action != action_t::NONE)
			// Выполняем обработку заново
			goto repeat;
		// Выполняем разблокировку обработчика
		this->locker.mode = false;
	}
}
/**
 * actionOpen Метод обработки экшена открытия подключения
 */
void awh::client::Rest::actionOpen() noexcept {
	// Выполняем подключение
	const_cast <client::core_t *> (this->core)->open(this->worker.wid);
	// Если экшен соответствует, выполняем его сброс
	if(this->action == action_t::OPEN)
		// Выполняем сброс экшена
		this->action = action_t::NONE;
}
/**
 * actionRead Метод обработки экшена чтения с сервера
 */
void awh::client::Rest::actionRead() noexcept {
	// Результат работы функции
	res_t result;
	// Флаг завершения работы
	bool completed = false;
	// Получаем объект биндинга ядра TCP/IP
	client::core_t * core = const_cast <client::core_t *> (this->core);
	// Выполняем обработку полученных данных
	while(!this->active){
		// Получаем объект запроса
		req_t & request = this->requests.front();
		// Получаем объект ответа
		res_t & response = this->responses.front();
		// Выполняем парсинг полученных данных
		size_t bytes = this->http.parse(this->entity.data(), this->entity.size());
		// Если все данные получены
		if((response.ok = completed = this->http.isEnd())){
			// Получаем параметры запроса
			auto query = this->http.getQuery();
			// Устанавливаем код ответа
			response.code = query.code;
			// Устанавливаем сообщение ответа
			response.message = query.message;
			// Если включён режим отладки
			#if defined(DEBUG_MODE)
				{
					// Получаем данные ответа
					const auto & response = this->http.response(true);
					// Если параметры ответа получены
					if(!response.empty()){
						// Выводим заголовок ответа
						cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
						// Выводим параметры ответа
						cout << string(response.begin(), response.end()) << endl;
						// Если тело ответа существует
						if(!this->http.getBody().empty())
							// Выводим сообщение о выводе чанка тела
							cout << this->fmk->format("<body %u>", this->http.getBody().size()) << endl << endl;
						// Иначе устанавливаем перенос строки
						else cout << endl;
					}
				}
			#endif
			// Получаем статус ответа
			awh::http_t::stath_t status = this->http.getAuth();
			// Если выполнять редиректы запрещено
			if(!this->redirects && (status == awh::http_t::stath_t::RETRY)){
				// Если нужно произвести запрос заново
				if((response.code == 201) || (response.code == 301) ||
				   (response.code == 302) || (response.code == 303) ||
				   (response.code == 307) || (response.code == 308))
						// Запрещаем выполнять редирект
						status = awh::http_t::stath_t::GOOD;
			}
			// Выполняем анализ результата авторизации
			switch((uint8_t) status){
				// Если нужно попытаться ещё раз
				case (uint8_t) awh::http_t::stath_t::RETRY: {
					// Если попытка повторить авторизацию ещё не проводилась
					if(!request.failAuth){
						// Получаем новый адрес запроса
						request.url = this->http.getUrl();
						// Если адрес запроса получен
						if(!request.url.empty()){
							// Выполняем очистку оставшихся данных
							this->entity.clear();
							// Запоминаем, что попытка выполнена
							request.failAuth = true;
							// Если соединение является постоянным
							if(this->http.isAlive())
								// Устанавливаем новый экшен выполнения
								this->action = action_t::CONNECT;
							// Если нам необходимо отключиться
							else {
								// Если экшен соответствует, выполняем его сброс
								if(this->action == action_t::READ)
									// Выполняем сброс экшена
									this->action = action_t::NONE;
								// Получаем новый адрес запроса для воркера
								this->worker.url = request.url;
								// Завершаем работу
								core->close(this->aid);
							}
							// Завершаем работу
							return;
						}
					}
					// Устанавливаем код ответа
					response.code = 403;
				} break;
				// Если запрос выполнен удачно
				case (uint8_t) awh::http_t::stath_t::GOOD: {					
					// Получаем объект ответа
					result = response;
					// Если функция обратного вызова установлена, выводим сообщение
					if(this->messageFn != nullptr){
						// Получаем тело запроса
						const auto & entity = this->http.getBody();
						// Получаем заголовки ответа
						const auto & headers = this->http.getHeaders();
						// Устанавливаем тело ответа
						result.entity.assign(entity.begin(), entity.end());
						// Устанавливаем заголовки ответа
						result.headers = move(* const_cast <unordered_multimap <string, string> *> (&headers));
					}
					// Устанавливаем размер стопбайт
					if(!this->http.isAlive()){
						// Выполняем очистку оставшихся данных
						this->entity.clear();
						// Завершаем работу
						core->close(this->aid);
						// Выполняем завершение работы
						goto Stop;
					}
					// Выполняем сброс состояния HTTP парсера
					this->http.reset();
					// Выполняем очистку параметров HTTP запроса
					this->http.clear();
					// Если объект ещё не удалён
					if(!this->requests.empty())
						// Выполняем удаление объекта запроса
						this->requests.erase(this->requests.begin());
					// Если объект ещё не удалён
					if(!this->responses.empty())
						// Выполняем удаление объекта ответа
						this->responses.erase(this->responses.begin());
					// Завершаем обработку
					goto Next;
				} break;
				// Если запрос неудачный
				case (uint8_t) awh::http_t::stath_t::FAULT: {
					// Запрещаем бесконечный редирект при запросе авторизации
					if((response.code == 401) || (response.code == 407)) response.code = 403;
				} break;
			}
			// Выполняем очистку оставшихся данных
			this->entity.clear();
			// Если функция обратного вызова установлена, выводим сообщение
			if(this->messageFn != nullptr){
				// Получаем объект ответа
				result = response;
				// Получаем тело запроса
				const auto & entity = this->http.getBody();
				// Получаем заголовки ответа
				const auto & headers = this->http.getHeaders();
				// Устанавливаем тело ответа
				result.entity.assign(entity.begin(), entity.end());
				// Устанавливаем заголовки ответа
				result.headers = move(* const_cast <unordered_multimap <string, string> *> (&headers));
				// Завершаем работу
				core->close(this->aid);
			// Завершаем работу
			} else core->close(this->aid);
			// Выходим из функции
			return;
		}
		// Устанавливаем метку продолжения обработки пайплайна
		Next:
		// Если парсер обработал какое-то количество байт
		if((bytes > 0) && !this->entity.empty()){
			// Если размер буфера больше количества удаляемых байт
			if(this->entity.size() >= bytes)
				// Удаляем количество обработанных байт
				vector <decltype(this->entity)::value_type> (this->entity.begin() + bytes, this->entity.end()).swap(this->entity);
			// Если байт в буфере меньше, просто очищаем буфер
			else this->entity.clear();
			// Если данных для обработки не осталось, выходим
			if(this->entity.empty()) break;
		// Если данных для обработки недостаточно, выходим
		} else break;
	}
	// Устанавливаем метку завершения работы
	Stop:
	// Если экшен соответствует, выполняем его сброс
	if(this->action == action_t::READ)
		// Выполняем сброс экшена
		this->action = action_t::NONE;
	// Если функция обратного вызова установлена, выводим сообщение
	if(completed && (this->messageFn != nullptr))
		// Выполняем функцию обратного вызова
		this->messageFn(result, this, this->ctx.at(1));
}
/**
 * actionConnect Метод обработки экшена подключения к серверу
 */
void awh::client::Rest::actionConnect() noexcept {
	// Выполняем сброс параметров запроса
	this->flush();
	// Получаем объект биндинга ядра TCP/IP
	client::core_t * core = const_cast <client::core_t *> (this->core);
	// Выполняем перебор всех подключений
	for(auto & req : this->requests){
		// Выполняем сброс состояния HTTP парсера
		this->http.reset();
		// Выполняем очистку параметров HTTP запроса
		this->http.clear();
		// Устанавливаем метод компрессии
		this->http.setCompress(this->compress);
		// Если список заголовков получен
		if(!req.headers.empty())
			// Устанавливаем заголовоки запроса
			this->http.setHeaders(req.headers);
		// Если тело запроса существует
		if(!req.entity.empty())
			// Устанавливаем тело запроса
			this->http.setBody(req.entity);
		// Получаем бинарные данные REST запроса
		const auto & request = this->http.request(req.url, req.method);
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
			core->write(request.data(), request.size(), this->aid);
			// Получаем данные тела запроса
			while(!(entity = this->http.payload()).empty()){
				// Если включён режим отладки
				#if defined(DEBUG_MODE)
					// Выводим сообщение о выводе чанка тела
					cout << this->fmk->format("<chunk %u>", entity.size()) << endl;
				#endif
				// Отправляем тело на сервер
				core->write(entity.data(), entity.size(), this->aid);
			}
		}
	}
	// Если экшен соответствует, выполняем его сброс
	if(this->action == action_t::CONNECT)
		// Выполняем сброс экшена
		this->action = action_t::NONE;
	// Если функция обратного вызова существует
	if(this->activeFn != nullptr)
		// Выполняем функцию обратного вызова
		this->activeFn(mode_t::CONNECT, this, this->ctx.at(0));
}
/**
 * actionReadProxy Метод обработки экшена чтения с прокси-сервера
 */
void awh::client::Rest::actionReadProxy() noexcept {
	// Получаем объект биндинга ядра TCP/IP
	client::core_t * core = const_cast <client::core_t *> (this->core);
	// Определяем тип прокси-сервера
	switch((uint8_t) this->worker.proxy.type){
		// Если прокси-сервер является Socks5
		case (uint8_t) proxy_t::type_t::SOCKS5: {
			// Если данные не получены
			if(!this->worker.proxy.socks5.isEnd()){
				// Выполняем парсинг входящих данных
				this->worker.proxy.socks5.parse(this->entity.data(), this->entity.size());
				// Получаем данные запроса
				const auto & socks5 = this->worker.proxy.socks5.get();
				// Если данные получены
				if(!socks5.empty()) core->write(socks5.data(), socks5.size(), this->aid);
				// Если данные все получены
				else if(this->worker.proxy.socks5.isEnd()) {
					// Выполняем очистку буфера данных
					this->entity.clear();
					// Если рукопожатие выполнено
					if(this->worker.proxy.socks5.isHandshake()){
						// Выполняем переключение на работу с сервером
						core->switchProxy(this->aid);
						// Если экшен соответствует, выполняем его сброс
						if(this->action == action_t::PROXY_READ)
							// Выполняем сброс экшена
							this->action = action_t::NONE;
						// Завершаем работу
						return;
					// Если рукопожатие не выполнено
					} else {
						// Получаем объект ответа
						res_t & response = this->responses.front();
						// Устанавливаем код ответа
						response.code = this->worker.proxy.socks5.getCode();
						// Устанавливаем сообщение ответа
						response.message = this->worker.proxy.socks5.getMessage(response.code);
						// Если включён режим отладки
						#if defined(DEBUG_MODE)
							// Если заголовки получены
							if(!response.message.empty()){
								// Данные REST ответа
								const string & message = this->fmk->format("SOCKS5 %u %s\r\n", response.code, response.message.c_str());
								// Выводим заголовок ответа
								cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE PROXY ^^^^^^^^^\x1B[0m" << endl;
								// Выводим параметры ответа
								cout << string(message.begin(), message.end()) << endl;
							}
						#endif
						// Если экшен соответствует, выполняем его сброс
						if(this->action == action_t::PROXY_READ)
							// Выполняем сброс экшена
							this->action = action_t::NONE;
						// Если функция обратного вызова установлена, выводим сообщение
						if(this->messageFn != nullptr){
							// Получаем результат ответа
							res_t result = response;
							// Завершаем работу
							core->close(this->aid);
							// Выполняем функцию обратного вызова
							this->messageFn(result, this, this->ctx.at(1));
						// Завершаем работу
						} else core->close(this->aid);
					}
				}
			}
		} break;
		// Если прокси-сервер является HTTP
		case (uint8_t) proxy_t::type_t::HTTP: {
			// Выполняем парсинг полученных данных
			this->worker.proxy.http.parse(this->entity.data(), this->entity.size());
			// Если все данные получены
			if(this->worker.proxy.http.isEnd()){
				// Выполняем очистку буфера данных
				this->entity.clear();
				// Получаем параметры запроса
				const auto & query = this->worker.proxy.http.getQuery();
				// Получаем объект запроса
				req_t & request = this->requests.front();
				// Получаем объект ответа
				res_t & response = this->responses.front();
				// Устанавливаем код ответа
				response.code = query.code;
				// Устанавливаем сообщение ответа
				response.message = query.message;
				// Если включён режим отладки
				#if defined(DEBUG_MODE)
					{
						// Получаем данные ответа
						const auto & response = this->worker.proxy.http.response(true);
						// Если параметры ответа получены
						if(!response.empty()){
							// Выводим заголовок ответа
							cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE PROXY ^^^^^^^^^\x1B[0m" << endl;
							// Выводим параметры ответа
							cout << string(response.begin(), response.end()) << endl;
							// Если тело ответа существует
							if(!this->worker.proxy.http.getBody().empty())
								// Выводим сообщение о выводе чанка тела
								cout << this->fmk->format("<body %u>", this->worker.proxy.http.getBody().size())  << endl;
						}
					}
				#endif
				// Получаем статус ответа
				awh::http_t::stath_t status = this->worker.proxy.http.getAuth();
				// Если выполнять редиректы запрещено
				if(!this->redirects && (status == awh::http_t::stath_t::RETRY)){
					// Если нужно произвести запрос заново
					if((response.code == 201) || (response.code == 301) ||
					   (response.code == 302) || (response.code == 303) ||
					   (response.code == 307) || (response.code == 308))
						// Запрещаем выполнять редирект
						status = awh::http_t::stath_t::GOOD;
				}
				// Выполняем проверку авторизации
				switch((uint8_t) status){
					// Если нужно попытаться ещё раз
					case (uint8_t) awh::http_t::stath_t::RETRY: {
						// Если попытка повторить авторизацию ещё не проводилась
						if(!request.failAuth){
							// Получаем новый адрес запроса
							this->worker.proxy.url = this->worker.proxy.http.getUrl();
							// Если адрес запроса получен
							if(!this->worker.proxy.url.empty()){
								// Запоминаем, что попытка выполнена
								request.failAuth = true;
								// Если соединение является постоянным
								if(this->worker.proxy.http.isAlive())
									// Устанавливаем новый экшен выполнения
									this->action = action_t::PROXY_CONNECT;
								// Завершаем работу
								else {
									// Если экшен соответствует, выполняем его сброс
									if(this->action == action_t::PROXY_READ)
										// Выполняем сброс экшена
										this->action = action_t::NONE;
									// Завершаем работу
									core->close(this->aid);
								}
								// Завершаем работу
								return;
							}
						}
						// Устанавливаем код ответа
						response.code = 403;
					} break;
					// Если запрос выполнен удачно
					case (uint8_t) awh::http_t::stath_t::GOOD: {
						// Выполняем сброс количество попыток
						request.failAuth = false;
						// Выполняем переключение на работу с сервером
						core->switchProxy(this->aid);
						// Если экшен соответствует, выполняем его сброс
						if(this->action == action_t::PROXY_READ)
							// Выполняем сброс экшена
							this->action = action_t::NONE;
						// Завершаем работу
						return;
					} break;
					// Если запрос неудачный
					case (uint8_t) awh::http_t::stath_t::FAULT: {
						// Запрещаем бесконечный редирект при запросе авторизации
						if((response.code == 401) || (response.code == 407))
							// Устанавливаем код ответа
							response.code = 403;
						// Получаем тело запроса
						const auto & entity = this->worker.proxy.http.getBody();
						// Устанавливаем заголовки ответа
						response.headers = this->worker.proxy.http.getHeaders();
						// Устанавливаем тело ответа
						response.entity.assign(entity.begin(), entity.end());
					} break;
				}
				// Если экшен соответствует, выполняем его сброс
				if(this->action == action_t::PROXY_READ)
					// Выполняем сброс экшена
					this->action = action_t::NONE;
				// Если функция обратного вызова установлена, выводим сообщение
				if(this->messageFn != nullptr){
					// Получаем результат ответа
					res_t result = response;
					// Завершаем работу
					core->close(this->aid);
					// Выполняем функцию обратного вызова
					this->messageFn(result, this, this->ctx.at(1));
				// Завершаем работу
				} else core->close(this->aid);
			}
		} break;
		// Иначе завершаем работу
		default: {
			// Если экшен соответствует, выполняем его сброс
			if(this->action == action_t::PROXY_READ)
				// Выполняем сброс экшена
				this->action = action_t::NONE;
			// Завершаем работу
			core->close(this->aid);
		}
	}
}
/**
 * actionDisconnect Метод обработки экшена отключения от сервера
 */
void awh::client::Rest::actionDisconnect() noexcept {
	// Получаем объект биндинга ядра TCP/IP
	client::core_t * core = const_cast <client::core_t *> (this->core);
	// Если список ответов получен
	if(!this->responses.empty() && !this->requests.empty()){
		// Получаем объект ответа
		const res_t & response = this->responses.front();
		// Если нужно произвести запрос заново
		if((response.code == 201) || (response.code == 301) ||
			(response.code == 302) || (response.code == 303) ||
			(response.code == 307) || (response.code == 308) ||
			(response.code == 401) || (response.code == 407)){
			// Если статус ответа требует произвести авторизацию или заголовок перенаправления указан
			if((response.code == 401) || (response.code == 407) || this->http.isHeader("location")){
				// Получаем объект запроса
				req_t & request = this->requests.front();
				// Устанавливаем новый URL адрес запроса
				request.url = this->http.getUrl();
				// Получаем новый адрес запроса для воркера
				this->worker.url = request.url;
				// Выполняем установку следующего экшена на открытие подключения
				this->action = action_t::OPEN;
				// Выходим из функции
				return;
			}
		}
	}
	// Получаем объект ответа
	res_t response = (!this->responses.empty() ? move(this->responses.front()) : res_t());
	// Если список ответов не получен, значит он был выведен ранее
	if(this->responses.empty())
		// Устанавливаем код сообщения по умолчанию
		response.code = 1;
	// Выполняем очистку списка запросов
	this->requests.clear();
	// Выполняем очистку списка ответов
	this->responses.clear();
	// Очищаем адрес сервера
	this->worker.url.clear();
	// Выполняем сброс параметров запроса
	this->flush();
	// Завершаем работу
	if(this->unbind) core->stop();
	// Если экшен соответствует, выполняем его сброс
	if(this->action == action_t::DISCONNECT)
		// Выполняем сброс экшена
		this->action = action_t::NONE;
	// Если функция обратного вызова установлена, выводим сообщение
	if((response.code == 0) && (this->messageFn != nullptr)){
		// Устанавливаем код ответа сервера
		response.code = 500;
		// Выполняем функцию обратного вызова
		this->messageFn(response, this, this->ctx.at(1));
	}
	// Если функция обратного вызова существует
	if(this->activeFn != nullptr)
		// Выполняем функцию обратного вызова
		this->activeFn(mode_t::DISCONNECT, this, this->ctx.at(0));
}
/**
 * actionConnectProxy Метод обработки экшена подключения к прокси-серверу
 */
void awh::client::Rest::actionConnectProxy() noexcept {
	// Получаем объект запроса
	req_t & request = this->requests.front();
	// Получаем объект биндинга ядра TCP/IP
	client::core_t * core = const_cast <client::core_t *> (this->core);
	// Определяем тип прокси-сервера
	switch((uint8_t) this->worker.proxy.type){
		// Если прокси-сервер является Socks5
		case (uint8_t) proxy_t::type_t::SOCKS5: {
			// Выполняем сброс состояния Socks5 парсера
			this->worker.proxy.socks5.reset();
			// Устанавливаем URL адрес запроса
			this->worker.proxy.socks5.setUrl(request.url);
			// Выполняем создание буфера запроса
			this->worker.proxy.socks5.parse();
			// Получаем данные запроса
			const auto & socks5 = this->worker.proxy.socks5.get();
			// Если данные получены
			if(!socks5.empty()) core->write(socks5.data(), socks5.size(), this->aid);
		} break;
		// Если прокси-сервер является HTTP
		case (uint8_t) proxy_t::type_t::HTTP: {
			// Выполняем сброс состояния HTTP парсера
			this->worker.proxy.http.reset();
			// Выполняем очистку параметров HTTP запроса
			this->worker.proxy.http.clear();
			// Получаем бинарные данные REST запроса
			const auto & proxy = this->worker.proxy.http.proxy(request.url);
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
				core->write(proxy.data(), proxy.size(), this->aid);
			}
		} break;
		// Иначе завершаем работу
		default: core->close(this->aid);
	}
	// Если экшен соответствует, выполняем его сброс
	if(this->action == action_t::PROXY_CONNECT)
		// Выполняем сброс экшена
		this->action = action_t::NONE;
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
	// Если минимальный размер данных для чтения, не установлен
	if(this->worker.markRead.min == 0)
		// Устанавливаем размер минимальных для чтения данных по умолчанию
		this->worker.markRead.min = BUFFER_READ_MIN;
	// Если максимальный размер данных для записи не установлен, устанавливаем по умолчанию
	if(this->worker.markWrite.max == 0)
		// Устанавливаем размер максимальных записываемых данных по умолчанию
		this->worker.markWrite.max = BUFFER_WRITE_MAX;
}
/**
 * setMode Метод установки флага модуля
 * @param flag флаг модуля для установки
 */
void awh::client::Rest::setMode(const u_short flag) noexcept {
	// Устанавливаем флаг анбиндинга ядра сетевого модуля
	this->unbind = !(flag & (uint8_t) flag_t::NOTSTOP);
	// Устанавливаем флаг разрешающий выполнять редиректы
	this->redirects = (flag & (uint8_t) flag_t::REDIRECTS);
	// Устанавливаем флаг ожидания входящих сообщений
	this->worker.wait = (flag & (uint8_t) flag_t::WAITMESS);
	// Устанавливаем флаг поддержания автоматического подключения
	this->worker.alive = (flag & (uint8_t) flag_t::KEEPALIVE);
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
awh::client::Rest::Rest(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept : nwk(fmk), uri(fmk, &nwk), http(fmk, log, &uri), core(core), fmk(fmk), log(log), worker(fmk, log), action(action_t::NONE), compress(awh::http_t::compress_t::NONE) {
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
