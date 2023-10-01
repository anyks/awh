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
void awh::client::Http1::connectCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::OPEN, event_t::READ, event_t::PROXY_READ}, event_t::CONNECT)){
		// Запоминаем идентификатор адъютанта
		this->_aid = aid;
		// Выполняем установку идентификатора объекта
		this->_http.id(aid);
		// Снимаем флаг активации получения данных
		this->_mode = false;
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
void awh::client::Http1::disconnectCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Выполняем редирект, если редирект выполнен
	if(this->redirect())
		// Выходим из функции
		return;
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
void awh::client::Http1::readCallback(const char * buffer, const size_t size, const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
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
				// Получаем идентификатор потока
				const int32_t sid = this->_requests.begin()->first;
				// Если функция обратного вызова активности потока установлена
				if(!this->_mode && (this->_mode = this->_callback.is("stream")))
					// Выводим функцию обратного вызова
					this->_callback.call <const int32_t, const mode_t> ("stream", sid, mode_t::OPEN);
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
								const auto & response = this->_http.process(http_t::process_t::RESPONSE, true);
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
						switch(static_cast <uint8_t> (this->prepare(sid, aid, dynamic_cast <client::core_t *> (core)))){
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
					// Если функция обратного вызова активности потока установлена
					if(this->_callback.is("stream"))
						// Выводим функцию обратного вызова
						this->_callback.call <const int32_t, const mode_t> ("stream", sid, mode_t::CLOSE);
					// Если функция обратного вызова установлена, выводим сообщение
					if(this->_resultCallback.is("entity"))
						// Выполняем функцию обратного вызова дисконнекта
						this->_resultCallback.bind <const int32_t, const u_int, const string, const vector <char>> ("entity");
					// Выполняем очистку функций обратного вызова
					this->_resultCallback.clear();
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
 * redirect Метод выполнения редиректа если требуется
 * @return результат выполнения редиректа
 */
bool awh::client::Http1::redirect() noexcept {
	// Результат работы функции
	bool result = false;
	// Если список ответов получен
	if((result = !this->_stopped && !this->_requests.empty())){
		// Получаем параметры запроса
		const auto & response = this->_http.response();
		// Если необходимо выполнить ещё одну попытку выполнения авторизации
		if((result = (this->_proxy.answer == 407) || (response.code == 401) || (response.code == 407))){
			// Увеличиваем количество попыток
			this->_attempt++;
			// Выполняем очистку оставшихся данных
			this->_buffer.clear();
			// Выполняем установку следующего экшена на открытие подключения
			this->open();
			// Завершаем работу
			return result;
		}
		// Выполняем определение ответа сервера
		switch(response.code){
			// Если ответ сервера: Created
			case 201:
			// Если ответ сервера: Moved Permanently
			case 301:
			// Если ответ сервера: Found
			case 302:
			// Если ответ сервера: See Other
			case 303:
			// Если ответ сервера: Temporary Redirect
			case 307:
			// Если ответ сервера: Permanent Redirect
			case 308: break;
			// Если мы получили любой другой ответ, выходим
			default: return result;
		}
		// Если адрес для выполнения переадресации указан
		if((result = this->_http.isHeader("location"))){
			// Выполняем очистку оставшихся данных
			this->_buffer.clear();
			// Получаем новый адрес запроса
			const uri_t::url_t & url = this->_http.getUrl();
			// Если адрес запроса получен
			if((result = !url.empty())){
				// Увеличиваем количество попыток
				this->_attempt++;
				// Устанавливаем новый адрес запроса
				this->_uri.combine(this->_scheme.url, url);
				// Получаем объект текущего запроса
				request_t & request = this->_requests.begin()->second;
				// Устанавливаем новый адрес запроса
				request.url = this->_scheme.url;
				// Если необходимо метод изменить на GET и основной метод не является GET
				if(((response.code == 201) || (response.code == 303)) && (request.method != awh::web_t::method_t::GET)){
					// Выполняем очистку тела запроса
					request.entity.clear();
					// Выполняем установку метода запроса
					request.method = awh::web_t::method_t::GET;
				}
				// Выполняем установку следующего экшена на открытие подключения
				this->open();
				// Завершаем работу
				return result;
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * response Метод получения ответа сервера
 * @param aid     идентификатор адъютанта
 * @param code    код ответа сервера
 * @param message сообщение ответа сервера
 */
void awh::client::Http1::response(const uint64_t aid, const u_int code, const string & message) noexcept {
	// Выполняем неиспользуемую переменную
	(void) aid;
	// Если функция обратного вызова на вывод ответа сервера на ранее выполненный запрос установлена
	if(!this->_requests.empty() && this->_callback.is("response"))
		// Выводим функцию обратного вызова
		this->_callback.call <const int32_t, const u_int, const string &> ("response", this->_requests.begin()->first, code, message);
}
/**
 * header Метод получения заголовка
 * @param aid   идентификатор адъютанта
 * @param key   ключ заголовка
 * @param value значение заголовка
 */
void awh::client::Http1::header(const uint64_t aid, const string & key, const string & value) noexcept {
	// Выполняем неиспользуемую переменную
	(void) aid;
	// Если функция обратного вызова на полученного заголовка с сервера установлена
	if(!this->_requests.empty() && this->_callback.is("header"))
		// Выводим функцию обратного вызова
		this->_callback.call <const int32_t, const string &, const string &> ("header", this->_requests.begin()->first, key, value);
}
/**
 * headers Метод получения заголовков
 * @param aid     идентификатор адъютанта
 * @param code    код ответа сервера
 * @param message сообщение ответа сервера
 * @param headers заголовки ответа сервера
 */
void awh::client::Http1::headers(const uint64_t aid, const u_int code, const string & message, const unordered_multimap <string, string> & headers) noexcept {
	// Выполняем неиспользуемую переменную
	(void) aid;
	// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
	if(!this->_requests.empty() && this->_callback.is("headers"))
		// Выводим функцию обратного вызова
		this->_callback.call <const int32_t, const u_int, const string &, const unordered_multimap <string, string> &> ("headers", this->_requests.begin()->first, code, message, headers);
}
/**
 * chunking Метод обработки получения чанков
 * @param aid   идентификатор адъютанта
 * @param chunk бинарный буфер чанка
 * @param http  объект модуля HTTP
 */
void awh::client::Http1::chunking(const uint64_t aid, const vector <char> & chunk, const awh::http_t * http) noexcept {
	// Если данные получены, формируем тело сообщения
	if(!this->_requests.empty() && !chunk.empty()){
		// Выполняем неиспользуемую переменную
		(void) aid;
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
 * @param sid  идентификатор запроса
 * @param aid  идентификатор адъютанта
 * @param core объект сетевого ядра
 * @return     результат препарирования
 */
awh::client::Web::status_t awh::client::Http1::prepare(const int32_t sid, const uint64_t aid, client::core_t * core) noexcept {
	// Результат работы функции
	status_t result = status_t::STOP;
	// Получаем параметры ответа сервера
	const auto & response = this->_http.response();
	// Получаем статус ответа
	awh::http_t::stath_t status = this->_http.getAuth();
	// Если выполнять редиректы запрещено
	if(!this->_redirects && (status == awh::http_t::stath_t::RETRY)){
		// Если нужно произвести запрос заново
		if((response.code == 201) || (response.code == 301) ||
		   (response.code == 302) || (response.code == 303) ||
		   (response.code == 307) || (response.code == 308))
				// Запрещаем выполнять редирект
				status = awh::http_t::stath_t::GOOD;
	}
	// Выполняем анализ результата авторизации
	switch(static_cast <uint8_t> (status)){
		// Если нужно попытаться ещё раз
		case static_cast <uint8_t> (awh::http_t::stath_t::RETRY): {
			// Если попытки повторить переадресацию ещё не закончились
			if(!(this->_stopped = (this->_attempt >= this->_attempts))){
				// Выполняем поиск указанного запроса
				auto it = this->_requests.find(sid);
				// Если параметры активного запроса найдены
				if(it != this->_requests.end()){
					// Получаем новый адрес запроса
					const uri_t::url_t & url = this->_http.getUrl();
					// Если URL-адрес запроса получен
					if(!url.empty()){
						// Выполняем проверку соответствие протоколов
						const bool schema = (this->_fmk->compare(url.schema, it->second.url.schema));
						// Если соединение является постоянным
						if(schema && this->_http.isAlive()){
							// Выполняем сброс параметров запроса
							this->flush();
							// Увеличиваем количество попыток
							this->_attempt++;
							// Устанавливаем новый адрес запроса
							this->_uri.combine(it->second.url, url);
							// Выполняем запрос на удалённый сервер
							this->send(it->second);
							// Если функция обратного вызова активности потока установлена
							if(this->_callback.is("stream"))
								// Выводим функцию обратного вызова
								this->_callback.call <const int32_t, const mode_t> ("stream", sid, mode_t::CLOSE);
							// Завершаем работу
							return status_t::SKIP;
						}
					// Если URL-адрес запроса не получен
					} else {
						// Если соединение является постоянным
						if(this->_http.isAlive()){
							// Выполняем сброс параметров запроса
							this->flush();
							// Увеличиваем количество попыток
							this->_attempt++;
							// Выполняем запрос на удалённый сервер
							this->send(it->second);
							// Если функция обратного вызова активности потока установлена
							if(this->_callback.is("stream"))
								// Выводим функцию обратного вызова
								this->_callback.call <const int32_t, const mode_t> ("stream", sid, mode_t::CLOSE);
							// Завершаем работу
							return status_t::SKIP;
						}
					}
					// Если нам необходимо отключиться
					core->close(aid);
					// Если функция обратного вызова активности потока установлена
					if(this->_callback.is("stream"))
						// Выводим функцию обратного вызова
						this->_callback.call <const int32_t, const mode_t> ("stream", sid, mode_t::CLOSE);
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
				this->_resultCallback.set <void (const int32_t, const u_int, const string, const vector <char>)> ("entity", this->_callback.get <void (const int32_t, const u_int, const string, const vector <char>)> ("entity"), sid, response.code, response.message, this->_http.body());
			// Если объект ещё не удалён
			if(!this->_requests.empty()){
				// Выполняем поиск указанного запроса
				auto it = this->_requests.find(sid);
				// Если параметры активного запроса найдены
				if(it != this->_requests.end())
					// Выполняем удаление объекта запроса
					this->_requests.erase(it);
			}
			// Устанавливаем размер стопбайт
			if(!this->_http.isAlive()){
				// Выполняем очистку оставшихся данных
				this->_buffer.clear();
				// Завершаем работу
				core->close(aid);
				// Выполняем завершение работы
				return status_t::STOP;
			}
			// Завершаем обработку
			return status_t::NEXT;
		} break;
		// Если запрос неудачный
		case static_cast <uint8_t> (awh::http_t::stath_t::FAULT): {
			// Устанавливаем флаг принудительной остановки
			this->_stopped = true;
			// Если возникла ошибка выполнения запроса
			if((response.code >= 400) && (response.code < 500)){
				// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
				if(this->_callback.is("entity"))
					// Устанавливаем полученную функцию обратного вызова
					this->_resultCallback.set <void (const int32_t, const u_int, const string, const vector <char>)> ("entity", this->_callback.get <void (const int32_t, const u_int, const string, const vector <char>)> ("entity"), sid, response.code, response.message, this->_http.body());
				// Если объект ещё не удалён
				if(!this->_requests.empty()){
					// Выполняем поиск указанного запроса
					auto it = this->_requests.find(sid);
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
		this->_resultCallback.set <void (const int32_t, const u_int, const string, const vector <char>)> ("entity", this->_callback.get <void (const int32_t, const u_int, const string, const vector <char>)> ("entity"), sid, response.code, response.message, this->_http.body());
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
	if(hold.access({event_t::READ, event_t::SEND, event_t::CONNECT}, event_t::SUBMIT)){
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
			// Если метод компрессии установлен
			if(request.compress != http_t::compress_t::NONE)
				// Устанавливаем метод компрессии переданный пользователем
				this->_http.compress(request.compress);
			// Устанавливаем метод компрессии
			else this->_http.compress(this->_compress);
			// Если список заголовков получен
			if(!request.headers.empty())
				// Устанавливаем заголовоки запроса
				this->_http.headers(request.headers);
			// Если тело запроса существует
			if(!request.entity.empty())
				// Устанавливаем тело запроса
				this->_http.body(request.entity);
			// Устанавливаем новый адрес запроса
			this->_uri.combine(this->_scheme.url, request.url);
			// Создаём объек запроса
			awh::web_t::req_t query(request.method, this->_scheme.url);
			// Если метод CONNECT запрещён для прокси-сервера
			if(!this->_proxy.connect){
				// Получаем строку авторизации на проксе-сервере
				const string & auth = this->_scheme.proxy.http.getAuth(http_t::process_t::REQUEST, request.method);
				// Если строка автоирации получена
				if(!auth.empty())
					// Выполняем добавление заголовка авторизации
					this->_http.header("Proxy-Authorization", auth);
			}
			// Получаем бинарные данные WEB запроса
			const auto & buffer = this->_http.process(http_t::process_t::REQUEST, std::move(query));
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
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::READ, event_t::CONNECT}, event_t::SEND)){
		// Если это первый запрос
		if(this->_attempt == 0){
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
		// Если список запросов не пустой
		} else if(!this->_requests.empty())
			// Выполняем запрос на удалённый сервер
			this->submit(this->_requests.begin()->second);
		// Если мы получили ошибку
		else {
			// Выводим сообщение об ошибке
			this->_log->print("Number of redirect attempts has not been reset", log_t::flag_t::CRITICAL);
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP1_SEND, "Number of redirect attempts has not been reset");
		}
	}
	// Сообщаем что идентификатор не получен
	return -1;
}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param callback функция обратного вызова
 */
void awh::client::Http1::on(function <void (const mode_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
 * @param callback функция обратного вызова
 */
void awh::client::Http1::on(function <void (const awh::core_t::status_t, awh::core_t *)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * on Метод установки функции обратного вызова для перехвата полученных чанков
 * @param callback функция обратного вызова
 */
void awh::client::Http1::on(function <void (const uint64_t, const vector <char> &, const awh::http_t *)> callback) noexcept {
	// Если функция обратного вызова передана
	if(callback != nullptr)
		// Устанавливаем функцию обратного вызова для HTTP/1.1
		this->_http.on(callback);
	// Устанавливаем функцию обработки вызова для получения чанков для HTTP-клиента
	else this->_http.on(std::bind(&http1_t::chunking, this, _1, _2, _3));
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибки
 * @param callback функция обратного вызова
 */
void awh::client::Http1::on(function <void (const log_t::flag_t, const http::error_t, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * on Метод установки функция обратного вызова активности потока
 * @param callback функция обратного вызова
 */
void awh::client::Http1::on(function <void (const int32_t, const mode_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * on Метод установки функции вывода полученного чанка бинарных данных с сервера
 * @param callback функция обратного вызова
 */
void awh::client::Http1::on(function <void (const int32_t, const vector <char> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * on Метод установки функции вывода ответа сервера на ранее выполненный запрос
 * @param callback функция обратного вызова
 */
void awh::client::Http1::on(function <void (const int32_t, const u_int, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
	// Устанавливаем функцию обратного вызова для HTTP/1.1
	this->_http.on((function <void (const uint64_t, const u_int, const string &)>) std::bind(&http1_t::response, this, _1, _2, _3));
}
/**
 * on Метод установки функции вывода полученного заголовка с сервера
 * @param callback функция обратного вызова
 */
void awh::client::Http1::on(function <void (const int32_t, const string &, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
	// Устанавливаем функцию обратного вызова для HTTP/1.1
	this->_http.on((function <void (const uint64_t, const string &, const string &)>) std::bind(&http1_t::header, this, _1, _2, _3));
}
/**
 * on Метод установки функции вывода полученного тела данных с сервера
 * @param callback функция обратного вызова
 */
void awh::client::Http1::on(function <void (const int32_t, const u_int, const string &, const vector <char> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * on Метод установки функции вывода полученных заголовков с сервера
 * @param callback функция обратного вызова
 */
void awh::client::Http1::on(function <void (const int32_t, const u_int, const string &, const unordered_multimap <string, string> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
	// Устанавливаем функцию обратного вызова для HTTP/1.1
	this->_http.on((function <void (const uint64_t, const u_int, const string &, const unordered_multimap <string, string> &)>) std::bind(&http1_t::headers, this, _1, _2, _3, _4));
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
	// Устанавливаем флаг запрещающий выполнять метод CONNECT для прокси-клиента
	this->_proxy.connect = (flags.count(flag_t::PROXY_NOCONNECT) == 0);
	// Если сетевое ядро установлено
	if(this->_core != nullptr){
		// Устанавливаем флаг запрещающий вывод информационных сообщений
		const_cast <client::core_t *> (this->_core)->noInfo(flags.count(flag_t::NOT_INFO) > 0);
		// Выполняем установку флага проверки домена
		const_cast <client::core_t *> (this->_core)->verifySSL(flags.count(flag_t::VERIFY_SSL) > 0);
	}
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
 * proxy Метод установки прокси-сервера
 * @param uri    параметры прокси-сервера
 * @param family семейстово интернет протоколов (IPV4 / IPV6 / NIX)
 */
void awh::client::Http1::proxy(const string & uri, const scheme_t::family_t family) noexcept {
	// Выполняем установку параметры прокси-сервера
	web_t::proxy(uri, family);
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest-авторизации
 */
void awh::client::Http1::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Устанавливаем параметры авторизации для HTTP-клиента
	this->_http.authType(type, hash);
}
/**
 * authTypeProxy Метод установки типа авторизации прокси-сервера
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest-авторизации
 */
void awh::client::Http1::authTypeProxy(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Устанавливаем тип авторизации на проксе-сервере
	web_t::authTypeProxy(type, hash);
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
 web_t(fmk, log), _mode(false), _http(fmk, log), _resultCallback(log) {
	// Устанавливаем функцию обработки вызова для получения чанков для HTTP-клиента
	this->_http.on(std::bind(&http1_t::chunking, this, _1, _2, _3));
}
/**
 * Http1 Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::client::Http1::Http1(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 web_t(core, fmk, log), _mode(false), _http(fmk, log), _resultCallback(log) {
	// Устанавливаем функцию обработки вызова для получения чанков для HTTP-клиента
	this->_http.on(std::bind(&http1_t::chunking, this, _1, _2, _3));
}
