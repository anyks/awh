/**
 * @file: http1.cpp
 * @date: 2023-09-12
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
#include <client/web/http1.hpp>

/**
 * connectCallback Метод обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::Http1::connectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::OPEN, event_t::READ, event_t::PROXY_READ}, event_t::CONNECT)){
		// Запоминаем идентификатор адъютанта
		this->_aid = aid;
		// Если функция обратного вызова при подключении/отключении установлена
		if(this->_callback.is("active"))
			// Выводим функцию обратного вызова
			this->_callback.call <const mode_t> ("active", mode_t::CONNECT);
	}
}
/**
 * disconnectCallback Метод обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::Http1::disconnectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если список ответов получен
	if(!this->_requests.empty()){
		// Получаем параметры запроса
		const auto & query = this->_http.query();
		// Если нужно произвести запрос заново
		if(!this->_stopped && ((query.code == 201) || (query.code == 301) ||
		  (query.code == 302) || (query.code == 303) || (query.code == 307) ||
		  (query.code == 308) || (query.code == 401) || (query.code == 407))){
			// Если статус ответа требует произвести авторизацию или заголовок перенаправления указан
			if((query.code == 401) || (query.code == 407) || this->_http.isHeader("location")){
				// Получаем новый адрес запроса
				const uri_t::url_t & url = this->_http.getUrl();
				// Если адрес запроса получен
				if(!url.empty()){
					// Увеличиваем количество попыток
					this->_attempt++;
					// Получаем объект текущего запроса
					request_t & request = this->_requests.begin()->second;
					// Устанавливаем новый адрес запроса
					request.url = std::forward <const uri_t::url_t> (url);
					// Заменяем адрес запроса в схеме клиента
					this->_scheme.url = request.url;
					// Выполняем очистку оставшихся данных
					this->_buffer.clear();
					// Выполняем установку следующего экшена на открытие подключения
					this->open();
					// Завершаем работу
					return;
				}
			}
		}
	}
	// Если подключение является постоянным
	if(this->_scheme.alive)
		// Выполняем очистку оставшихся данных
		this->_buffer.clear();
	// Если подключение не является постоянным
	else {
		// Выполняем сброс параметров запроса
		this->flush();
		// Выполняем зануление идентификатора адъютанта
		this->_aid = 0;
		// Выполняем очистку списка запросов
		this->_requests.clear();
		// Очищаем адрес сервера
		this->_scheme.url.clear();
		// Если завершить работу разрешено
		if(this->_unbind)
			// Завершаем работу
			dynamic_cast <client::core_t *> (core)->stop();
	}
	// Если функция обратного вызова при подключении/отключении установлена
	if(this->_callback.is("active"))
		// Выводим функцию обратного вызова
		this->_callback.call <const mode_t> ("active", mode_t::DISCONNECT);
}
/**
 * readCallback Метод обратного вызова при чтении сообщения с сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::client::Http1::readCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (sid > 0)){
		// Создаём объект холдирования
		hold_t <event_t> hold(this->_events);
		// Если событие соответствует разрешённому
		if(hold.access({event_t::CONNECT}, event_t::READ)){
			// Если список ответов получен
			if(!this->_requests.empty()){
				// Флаг удачного получения данных
				bool receive = false;
				// Флаг завершения работы
				bool completed = false;
				// Добавляем полученные данные в буфер
				this->_buffer.insert(this->_buffer.end(), buffer, buffer + size);
				// Выполняем обработку полученных данных
				while(!this->_active){
					// Выполняем парсинг полученных данных
					size_t bytes = this->_http.parse(this->_buffer.data(), this->_buffer.size());
					// Если все данные получены
					if((completed = this->_http.isEnd())){
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							{
								// Получаем данные ответа
								const auto & response = this->_http.response(true);
								// Если параметры ответа получены
								if(!response.empty()){
									// Выводим заголовок ответа
									cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
									// Выводим параметры ответа
									cout << string(response.begin(), response.end()) << endl;
									// Если тело ответа существует
									if(!this->_http.body().empty())
										// Выводим сообщение о выводе чанка тела
										cout << this->_fmk->format("<body %u>", this->_http.body().size()) << endl << endl;
									// Иначе устанавливаем перенос строки
									else cout << endl;
								}
							}
						#endif
						// Выполняем препарирование полученных данных
						switch(static_cast <uint8_t> (this->prepare(this->_requests.begin()->first, aid, dynamic_cast <client::core_t *> (core)))){
							// Если необходимо выполнить остановку обработки
							case static_cast <uint8_t> (status_t::STOP):
								// Выполняем завершение работы
								goto Stop;
							// Если необходимо выполнить переход к следующему этапу обработки
							case static_cast <uint8_t> (status_t::NEXT):
								// Выполняем переход к следующему этапу обработки
								goto Next;
							// Если необходимо выполнить пропуск обработки данных
							case static_cast <uint8_t> (status_t::SKIP):
								// Завершаем работу
								return;
						}
					}
					// Устанавливаем метку продолжения обработки пайплайна
					Next:
					// Если парсер обработал какое-то количество байт
					if((receive = ((bytes > 0) && !this->_buffer.empty()))){
						// Если размер буфера больше количества удаляемых байт
						if((receive = (this->_buffer.size() >= bytes)))
							// Удаляем количество обработанных байт
							this->_buffer.assign(this->_buffer.begin() + bytes, this->_buffer.end());
							// vector <decltype(this->_buffer)::value_type> (this->_buffer.begin() + bytes, this->_buffer.end()).swap(this->_buffer);
					}
					// Если данные мы все получили, выходим
					if(!receive || this->_buffer.empty()) break;
				}
				// Устанавливаем метку завершения работы
				Stop:
				// Если получение данных выполнено
				if(completed){
					// Если функция обратного вызова установлена, выводим сообщение
					if(this->_resultCallback.is("entity"))
						// Выполняем функцию обратного вызова дисконнекта
						this->_resultCallback.bind <const int32_t, const u_int, const string, const vector <char>> ("entity");
					// Если подключение выполнено и список запросов не пустой
					if((this->_aid > 0) && !this->_requests.empty())
						// Выполняем запрос на удалённый сервер
						this->submit(this->_requests.begin()->second);
				}
			}
		}
	}
}
/**
 * response Метод получения ответа сервера
 * @param code    код ответа сервера
 * @param message сообщение ответа сервера
 */
void awh::client::Http1::response(const u_int code, const string & message) noexcept {
	// Если функция обратного вызова на вывод ответа сервера на ранее выполненный запрос установлена
	if(!this->_requests.empty() && this->_callback.is("response"))
		// Выводим функцию обратного вызова
		this->_callback.call <const int32_t, const u_int, const string &> ("response", this->_requests.begin()->first, code, message);
}
/**
 * header Метод получения заголовка
 * @param key   ключ заголовка
 * @param value значение заголовка
 */
void awh::client::Http1::header(const string & key, const string & value) noexcept {
	// Если функция обратного вызова на полученного заголовка с сервера установлена
	if(!this->_requests.empty() && this->_callback.is("header"))
		// Выводим функцию обратного вызова
		this->_callback.call <const int32_t, const string &, const string &> ("header", this->_requests.begin()->first, key, value);
}
/**
 * headers Метод получения заголовков
 * @param code    код ответа сервера
 * @param message сообщение ответа сервера
 * @param headers заголовки ответа сервера
 */
void awh::client::Http1::headers(const u_int code, const string & message, const unordered_multimap <string, string> & headers) noexcept {
	// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
	if(!this->_requests.empty() && this->_callback.is("headers"))
		// Выводим функцию обратного вызова
		this->_callback.call <const int32_t, const u_int, const string &, const unordered_multimap <string, string> &> ("headers", this->_requests.begin()->first, code, message, this->_http.headers());
}
/**
 * chunking Метод обработки получения чанков
 * @param chunk бинарный буфер чанка
 * @param http  объект модуля HTTP
 */
void awh::client::Http1::chunking(const vector <char> & chunk, const awh::http_t * http) noexcept {
	// Если данные получены, формируем тело сообщения
	if(!this->_requests.empty() && !chunk.empty()){
		// Выполняем добавление полученного чанка в тело ответа
		const_cast <awh::http_t *> (http)->body(chunk);
		// Если функция обратного вызова на вывода полученного чанка бинарных данных с сервера установлена
		if(this->_callback.is("chunks"))
			// Выводим функцию обратного вызова
			this->_callback.call <const int32_t, const vector <char> &> ("chunks", this->_requests.begin()->first, chunk);
	}
}
/**
 * flush Метод сброса параметров запроса
 */
void awh::client::Http1::flush() noexcept {
	// Сбрасываем флаг принудительной остановки
	this->_active = false;
	// Снимаем флаг принудительной остановки
	this->_stopped = false;
	// Выполняем очистку оставшихся данных
	this->_buffer.clear();
}
/**
 * prepare Метод выполнения препарирования полученных данных
 * @param id   идентификатор запроса
 * @param aid  идентификатор адъютанта
 * @param core объект сетевого ядра
 * @return     результат препарирования
 */
awh::client::Web::status_t awh::client::Http1::prepare(const int32_t id, const size_t aid, client::core_t * core) noexcept {
	// Результат работы функции
	status_t result = status_t::STOP;
	// Получаем параметры запроса
	const auto & query = this->_http.query();
	// Получаем статус ответа
	awh::http_t::stath_t status = this->_http.getAuth();
	// Если выполнять редиректы запрещено
	if(!this->_redirects && (status == awh::http_t::stath_t::RETRY)){
		// Если нужно произвести запрос заново
		if((query.code == 201) || (query.code == 301) ||
		   (query.code == 302) || (query.code == 303) ||
		   (query.code == 307) || (query.code == 308))
				// Запрещаем выполнять редирект
				status = awh::http_t::stath_t::GOOD;
	}
	// Выполняем анализ результата авторизации
	switch(static_cast <uint8_t> (status)){
		// Если нужно попытаться ещё раз
		case static_cast <uint8_t> (awh::http_t::stath_t::RETRY): {
			// Если попытки повторить переадресацию ещё не закончились
			if(!(this->_stopped = (this->_attempt >= this->_attempts))){
				// Получаем новый адрес запроса
				const uri_t::url_t & url = this->_http.getUrl();
				// Если адрес запроса получен
				if(!url.empty()){
					// Выполняем проверку соответствие протоколов
					const bool schema = (this->_fmk->compare(url.schema, this->_scheme.url.schema));
					// Если соединение является постоянным
					if(schema && this->_http.isAlive()){
						// Увеличиваем количество попыток
						this->_attempt++;
						// Выполняем поиск указанного запроса
						auto it = this->_requests.find(id);
						// Если параметры активного запроса найдены
						if(it != this->_requests.end()){
							// Устанавливаем новый адрес запроса
							it->second.url = std::forward <const uri_t::url_t> (url);
							// Заменяем адрес запроса в схеме клиента
							this->_scheme.url = it->second.url;
							// Выполняем сброс параметров запроса
							this->flush();
							// Выполняем запрос на удалённый сервер
							this->submit(it->second);
							// Завершаем работу
							return status_t::SKIP;
						}
					}
					// Если нам необходимо отключиться
					core->close(aid);
					// Завершаем работу
					return status_t::SKIP;
				}
			}
		} break;
		// Если запрос выполнен удачно
		case static_cast <uint8_t> (awh::http_t::stath_t::GOOD): {
			// Выполняем сброс количества попыток
			this->_attempt = 0;
			// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
			if(this->_callback.is("entity"))
				// Устанавливаем полученную функцию обратного вызова
				this->_resultCallback.set <void (const int32_t, const u_int, const string, const vector <char>)> ("entity", this->_callback.get <void (const int32_t, const u_int, const string, const vector <char>)> ("entity"), id, query.code, query.message, this->_http.body());
			// Устанавливаем размер стопбайт
			if(!this->_http.isAlive()){
				// Выполняем очистку оставшихся данных
				this->_buffer.clear();
				// Завершаем работу
				core->close(aid);
				// Выполняем завершение работы
				return status_t::STOP;
			}
			// Если объект ещё не удалён
			if(!this->_requests.empty()){
				// Выполняем поиск указанного запроса
				auto it = this->_requests.find(id);
				// Если параметры активного запроса найдены
				if(it != this->_requests.end())
					// Выполняем удаление объекта запроса
					this->_requests.erase(it);
			}
			// Завершаем обработку
			return status_t::NEXT;
		} break;
		// Если запрос неудачный
		case static_cast <uint8_t> (awh::http_t::stath_t::FAULT): {
			// Устанавливаем флаг принудительной остановки
			this->_stopped = true;
			// Если возникла ошибка выполнения запроса
			if((query.code >= 400) && (query.code < 500)){
				// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
				if(this->_callback.is("entity"))
					// Устанавливаем полученную функцию обратного вызова
					this->_resultCallback.set <void (const int32_t, const u_int, const string, const vector <char>)> ("entity", this->_callback.get <void (const int32_t, const u_int, const string, const vector <char>)> ("entity"), id, query.code, query.message, this->_http.body());
				// Если объект ещё не удалён
				if(!this->_requests.empty()){
					// Выполняем поиск указанного запроса
					auto it = this->_requests.find(id);
					// Если параметры активного запроса найдены
					if(it != this->_requests.end())
						// Выполняем удаление объекта запроса
						this->_requests.erase(it);
				}
				// Завершаем обработку
				return status_t::NEXT;
			}
		} break;
	}
	// Выполняем очистку оставшихся данных
	this->_buffer.clear();
	// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
	if(this->_callback.is("entity"))
		// Устанавливаем полученную функцию обратного вызова
		this->_resultCallback.set <void (const int32_t, const u_int, const string, const vector <char>)> ("entity", this->_callback.get <void (const int32_t, const u_int, const string, const vector <char>)> ("entity"), id, query.code, query.message, this->_http.body());
	// Завершаем работу
	core->close(aid);
	// Выполняем завершение работы
	return status_t::STOP;
}
/** 
 * submit Метод выполнения удалённого запроса на сервер
 * @param request объект запроса на удалённый сервер
 */
void awh::client::Http1::submit(const request_t & request) noexcept {
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::READ, event_t::CONNECT}, event_t::SUBMIT)){
		// Если подключение выполнено
		if(this->_aid > 0){
			// Выполняем сброс параметров запроса
			this->flush();
			// Выполняем сброс состояния HTTP парсера
			this->_http.reset();
			// Выполняем очистку параметров HTTP запроса
			this->_http.clear();
			// Выполняем очистку функций обратного вызова
			this->_resultCallback.clear();
			// Выполняем установку URL-адреса запроса
			this->_scheme.url = request.url;
			// Устанавливаем метод компрессии
			this->_http.compress(this->_compress);
			// Если список заголовков получен
			if(!request.headers.empty())
				// Устанавливаем заголовоки запроса
				this->_http.headers(request.headers);
			// Если тело запроса существует
			if(!request.entity.empty())
				// Устанавливаем тело запроса
				this->_http.body(request.entity);
			// Получаем бинарные данные WEB запроса
			const auto & buffer = this->_http.request(this->_scheme.url, request.method);
			// Если бинарные данные запроса получены
			if(!buffer.empty()){
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим заголовок запроса
					cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
					// Выводим параметры запроса
					cout << string(buffer.begin(), buffer.end()) << endl << endl;
				#endif
				// Тело WEB сообщения
				vector <char> entity;
				// Получаем объект биндинга ядра TCP/IP
				client::core_t * core = const_cast <client::core_t *> (this->_core);
				// Выполняем отправку заголовков запроса на сервер
				core->write(buffer.data(), buffer.size(), this->_aid);
				// Получаем данные тела запроса
				while(!(entity = this->_http.payload()).empty()){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим сообщение о выводе чанка тела
						cout << this->_fmk->format("<chunk %u>", entity.size()) << endl;
					#endif
					// Выполняем отправку тела запроса на сервер
					core->write(entity.data(), entity.size(), this->_aid);
				}
			}
		}
	}
}
/**
 * send Метод отправки сообщения на сервер
 * @param request параметры запроса на удалённый сервер
 * @return        идентификатор отправленного запроса
 */
int32_t awh::client::Http1::send(const request_t & request) noexcept {
	// Результат работы функции
	int32_t result = (this->_requests.size() + 1);
	// Выполняем добавление активного запроса
	this->_requests.emplace(result, request);
	// Если В списке запросов ещё нет активных запросов
	if(this->_requests.size() == 1)
		// Выполняем запрос на удалённый сервер
		this->submit(request);
	// Выводим результат
	return result;
}
/**
 * on Метод установки функции обратного вызова для перехвата полученных чанков
 * @param callback функция обратного вызова
 */
void awh::client::Http1::on(function <void (const vector <char> &, const awh::http_t *)> callback) noexcept {
	// Если функция обратного вызова передана
	if(callback != nullptr)
		// Устанавливаем функцию обратного вызова для HTTP/1.1
		this->_http.on(callback);
	// Устанавливаем функцию обработки вызова для получения чанков для HTTP-клиента
	else this->_http.on(std::bind(&http1_t::chunking, this, _1, _2));
}
/**
 * on Метод установки функции вывода ответа сервера на ранее выполненный запрос
 * @param callback функция обратного вызова
 */
void awh::client::Http1::on(function <void (const int32_t, const u_int, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
	// Устанавливаем функцию обратного вызова для HTTP/1.1
	this->_http.on((function <void (const u_int, const string &)>) std::bind(&http1_t::response, this, _1, _2));
}
/**
 * on Метод установки функции вывода полученного заголовка с сервера
 * @param callback функция обратного вызова
 */
void awh::client::Http1::on(function <void (const int32_t, const string &, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
	// Устанавливаем функцию обратного вызова для HTTP/1.1
	this->_http.on((function <void (const string &, const string &)>) std::bind(&http1_t::header, this, _1, _2));
}
/**
 * on Метод установки функции вывода полученных заголовков с сервера
 * @param callback функция обратного вызова
 */
void awh::client::Http1::on(function <void (const int32_t, const u_int, const string &, const unordered_multimap <string, string> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
	// Устанавливаем функцию обратного вызова для HTTP/1.1
	this->_http.on((function <void (const u_int, const string &, const unordered_multimap <string, string> &)>) std::bind(&http1_t::headers, this, _1, _2, _3));
}
/**
 * chunk Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::client::Http1::chunk(const size_t size) noexcept {
	// Устанавливаем размер чанка
	this->_http.chunk(size);
}
/**
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::client::Http1::mode(const set <flag_t> & flags) noexcept {
	// Устанавливаем флаг анбиндинга ядра сетевого модуля
	this->_unbind = (flags.count(flag_t::NOT_STOP) == 0);
	// Устанавливаем флаг поддержания автоматического подключения
	this->_scheme.alive = (flags.count(flag_t::ALIVE) > 0);
	// Устанавливаем флаг разрешающий выполнять редиректы
	this->_redirects = (flags.count(flag_t::REDIRECTS) > 0);
	// Устанавливаем флаг ожидания входящих сообщений
	this->_scheme.wait = (flags.count(flag_t::WAIT_MESS) > 0);
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	const_cast <client::core_t *> (this->_core)->noInfo(flags.count(flag_t::NOT_INFO) > 0);
	// Выполняем установку флага проверки домена
	const_cast <client::core_t *> (this->_core)->verifySSL(flags.count(flag_t::VERIFY_SSL) > 0);
}
/**
 * user Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::client::Http1::user(const string & login, const string & password) noexcept {
	// Устанавливаем логин и пароль пользователя
	this->_http.user(login, password);
}
/**
 * userAgent Метод установки User-Agent для HTTP запроса
 * @param userAgent агент пользователя для HTTP запроса
 */
void awh::client::Http1::userAgent(const string & userAgent) noexcept {
	// Устанавливаем UserAgent
	if(!userAgent.empty()){
		// Устанавливаем пользовательского агента у родительского класса
		web_t::userAgent(userAgent);
		// Устанавливаем пользовательского агента
		this->_http.userAgent(userAgent);
	}
}
/**
 * serv Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::client::Http1::serv(const string & id, const string & name, const string & ver) noexcept {
	// Если данные сервиса переданы
	if(!id.empty() && !name.empty() && !ver.empty()){
		// Выполняем установку данных сервиса у родительского класса
		web_t::serv(id, name, ver);
		// Устанавливаем данные сервиса
		this->_http.serv(id, name, ver);
	}
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest-авторизации
 */
void awh::client::Http1::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Если объект авторизации создан
	this->_http.authType(type, hash);
}
/**
 * crypto Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::client::Http1::crypto(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Устанавливаем параметры шифрования для HTTP-клиента
	this->_http.crypto(pass, salt, cipher);
}
/**
 * Http1 Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::client::Http1::Http1(const fmk_t * fmk, const log_t * log) noexcept :
 web_t(fmk, log), _http(fmk, log, &_uri), _resultCallback(log) {
	// Устанавливаем функцию обработки вызова для получения чанков для HTTP-клиента
	this->_http.on(std::bind(&http1_t::chunking, this, _1, _2));
}
/**
 * Http1 Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::client::Http1::Http1(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 web_t(core, fmk, log), _http(fmk, log, &_uri), _resultCallback(log) {
	// Устанавливаем функцию обработки вызова для получения чанков для HTTP-клиента
	this->_http.on(std::bind(&http1_t::chunking, this, _1, _2));
}
