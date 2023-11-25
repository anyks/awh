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
 * @param bid  идентификатор брокера
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::Http1::connectCallback(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	
	cout << " +++++++++++++++ CONNECT HTTP1 " << endl;
	
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::OPEN, event_t::READ, event_t::PROXY_READ}, event_t::CONNECT)){
		// Запоминаем идентификатор брокера
		this->_bid = bid;
		// Выполняем установку идентификатора объекта
		this->_http.id(bid);
		// Снимаем флаг активации получения данных
		this->_mode = false;
		// Выполняем установку идентификатора объекта
		this->_ws1._http.id(bid);
		// Выполняем установку данных URL-адреса
		this->_ws1._scheme.url = this->_scheme.url;
		// Выполняем установку сетевого ядра
		this->_ws1._core = dynamic_cast <client::core_t *> (core);
		// Если функция обратного вызова, для вывода полученного чанка бинарных данных с сервера установлена
		if(this->_callback.is("chunks"))
			// Выполняем установку функции обратного вызова
			this->_ws1._callback.set <void (const int32_t, const vector <char> &)> ("chunks", this->_callback.get <void (const int32_t, const vector <char> &)> ("chunks"));
		// Если многопоточность активированна
		if(this->_threads > -1)
			// Выполняем инициализацию нового тредпула
			this->_ws1.multiThreads(this->_threads);
		// Если функция обратного вызова при подключении/отключении установлена
		if(this->_callback.is("active"))
			// Выполняем функцию обратного вызова
			this->_callback.call <const mode_t> ("active", mode_t::CONNECT);
	}
}
/**
 * disconnectCallback Метод обратного вызова при отключении от сервера
 * @param bid  идентификатор брокера
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::Http1::disconnectCallback(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	
	cout << " ############# DISCONNECT HTTP1 1 " << endl;
	
	// Выполняем редирект, если редирект выполнен
	if(this->redirect(bid, sid, core))
		// Выходим из функции
		return;
	
	cout << " ############# DISCONNECT HTTP1 2 " << endl;
	
	// Выполняем установку агента воркера HTTP/1.1
	this->_agent = agent_t::HTTP;
	// Если подключение является постоянным
	if(this->_scheme.alive)
		// Выполняем очистку оставшихся данных
		this->_buffer.clear();
	// Если подключение не является постоянным
	else {
		// Выполняем сброс параметров запроса
		this->flush();
		// Выполняем зануление идентификатора брокера
		this->_bid = 0;
		// Выполняем очистку списка запросов
		this->_requests.clear();
		// Очищаем адрес сервера
		this->_scheme.url.clear();
		// Если завершить работу разрешено
		if(this->_unbind)
			// Завершаем работу
			dynamic_cast <client::core_t *> (core)->stop();
	}

	cout << " ############# DISCONNECT HTTP1 3 " << endl;

	// Если функция обратного вызова при подключении/отключении установлена
	if(this->_callback.is("active"))
		// Выполняем функцию обратного вызова
		this->_callback.call <const mode_t> ("active", mode_t::DISCONNECT);
}
/**
 * readCallback Метод обратного вызова при чтении сообщения с сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param bid    идентификатор брокера
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::client::Http1::readCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (bid > 0) && (sid > 0)){
		// Флаг выполнения обработки полученных данных
		bool process = false;
		// Если установлена функция обратного вызова для вывода данных в сыром виде
		if(!(process = !this->_callback.is("raw")))
			// Выполняем функцию обратного вызова
			process = this->_callback.apply <bool, const char *, const size_t> ("raw", buffer, size);
		// Если обработка полученных данных разрешена
		if(process){
			// Создаём объект холдирования
			hold_t <event_t> hold(this->_events);
			// Если событие соответствует разрешённому
			if(hold.access({event_t::CONNECT}, event_t::READ)){
				// Определяем тип агента
				switch(static_cast <uint8_t> (this->_agent)){
					// Если протоколом агента является HTTP-клиент
					case static_cast <uint8_t> (agent_t::HTTP): {
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
								// Выполняем функцию обратного вызова
								this->_callback.call <const int32_t, const mode_t> ("stream", sid, mode_t::OPEN);
							// Добавляем полученные данные в буфер
							this->_buffer.insert(this->_buffer.end(), buffer, buffer + size);
							// Выполняем обработку полученных данных
							while(!this->_active){
								// Выполняем парсинг полученных данных
								size_t bytes = this->_http.parse(this->_buffer.data(), this->_buffer.size());
								// Если все данные получены
								if((completed = this->_http.is(http_t::state_t::END))){
									/**
									 * Если включён режим отладки
									 */
									#if defined(DEBUG_MODE)
										{
											// Получаем данные ответа
											const auto & response = this->_http.process(http_t::process_t::RESPONSE, this->_http.response());
											// Если параметры ответа получены
											if(!response.empty()){
												// Выводим заголовок ответа
												cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
												// Выводим параметры ответа
												cout << string(response.begin(), response.end()) << endl << endl;
												// Если тело ответа существует
												if(!this->_http.empty(awh::http_t::suite_t::BODY))
													// Выводим сообщение о выводе чанка тела
													cout << this->_fmk->format("<body %u>", this->_http.body().size()) << endl << endl;
												// Иначе устанавливаем перенос строки
												else cout << endl;
											}
										}
									#endif
									// Выполняем препарирование полученных данных
									switch(static_cast <uint8_t> (this->prepare(sid, bid, dynamic_cast <client::core_t *> (core)))){
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
										// this->_buffer.assign(this->_buffer.begin() + bytes, this->_buffer.end());
										vector <decltype(this->_buffer)::value_type> (this->_buffer.begin() + bytes, this->_buffer.end()).swap(this->_buffer);
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
									// Выполняем функцию обратного вызова
									this->_callback.call <const int32_t, const mode_t> ("stream", sid, mode_t::CLOSE);
								// Если функция обратного вызова установлена, выводим сообщение
								if(this->_resultCallback.is("entity"))
									// Выполняем функцию обратного вызова дисконнекта
									this->_resultCallback.bind <const int32_t, const u_int, const string, const vector <char>> ("entity");
								// Выполняем очистку функций обратного вызова
								this->_resultCallback.clear();
								// Получаем параметры запроса
								const u_int code = this->_http.response().code;
								// Если установлена функция отлова завершения запроса
								if((code >= 200) && this->_callback.is("end"))
									// Выполняем функцию обратного вызова
									this->_callback.call <const int32_t, const direct_t> ("end", sid, direct_t::RECV);
								// Выполняем очистку параметров HTTP-запроса
								this->_http.clear();
								// Выполняем сброс состояния HTTP-парсера
								this->_http.reset();
								// Если подключение выполнено и список запросов не пустой
								if((code >= 200) && (this->_bid > 0) && !this->_requests.empty())
									// Выполняем запрос на удалённый сервер
									this->submit(this->_requests.begin()->second);
							}
						}
					} break;
					// Если протоколом агента является WebSocket-клиент
					case static_cast <uint8_t> (agent_t::WEBSOCKET):
						// Выполняем переброс вызова чтения на клиент WebSocket
						this->_ws1.readCallback(buffer, size, bid, sid, core);
					break;
				}
			}
		}
	}
}
/**
 * writeCallback Метод обратного вызова при записи сообщения на клиенте
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param bid    идентификатор брокера
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::client::Http1::writeCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((bid > 0) && (sid > 0) && (core != nullptr)){
		// Определяем протокол клиента
		switch(static_cast <uint8_t> (this->_agent)){
			// Если агент является клиентом HTTP
			case static_cast <uint8_t> (agent_t::HTTP): {
				// Если установлена функция отлова завершения запроса
				if(this->_stopped && this->_callback.is("end"))
					// Выполняем функцию обратного вызова
					this->_callback.call <const int32_t, const direct_t> ("end", sid, direct_t::SEND);
			} break;
			// Если агент является клиентом WebSocket
			case static_cast <uint8_t> (agent_t::WEBSOCKET):
				// Выполняем переброс вызова записи на клиент WebSocket
				this->_ws1.writeCallback(buffer, size, bid, sid, core);
			break;
		}
	}
}
/**
 * redirect Метод выполнения редиректа если требуется
 * @param bid  идентификатор брокера
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 * @return     результат выполнения редиректа
 */
bool awh::client::Http1::redirect(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Результат работы функции
	bool result = false;
	// Если редиректы разрешены
	if(this->_redirects){
		// Определяем тип агента
		switch(static_cast <uint8_t> (this->_agent)){
			// Если протоколом агента является HTTP-клиент
			case static_cast <uint8_t> (agent_t::HTTP): {
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
						
						cout << " $$$$$$$$$$$$$$ OPEN HTTP1 1 " << endl;
						
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
					if((result = this->_http.is(http_t::suite_t::HEADER, "location"))){
						// Выполняем очистку оставшихся данных
						this->_buffer.clear();
						// Получаем новый адрес запроса
						const uri_t::url_t & url = this->_http.url();
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
							
							cout << " $$$$$$$$$$$$$$ OPEN HTTP1 2 " << endl;
							
							// Выполняем установку следующего экшена на открытие подключения
							this->open();
							// Завершаем работу
							return result;
						}
					}
				}
			} break;
			// Если протоколом агента является WebSocket-клиент
			case static_cast <uint8_t> (agent_t::WEBSOCKET): {
				// Выполняем переброс вызова дисконнекта на клиент WebSocket
				this->_ws1.disconnectCallback(bid, sid, core);
				// Если список ответов получен
				if((result = !this->_ws1._stopped)){
					// Получаем параметры запроса
					const auto & response = this->_ws1._http.response();
					// Если необходимо выполнить ещё одну попытку выполнения авторизации
					if((result = (this->_proxy.answer == 407) || (response.code == 401) || (response.code == 407))){
						// Выполняем очистку оставшихся данных
						this->_ws1._buffer.clear();
						// Получаем количество попыток
						this->_attempt = this->_ws1._attempt;
						
						cout << " $$$$$$$$$$$$$$ OPEN HTTP1 3 " << endl;
						
						// Выполняем установку следующего экшена на открытие подключения
						this->open();
						// Завершаем работу
						return result;
					}
					// Выполняем определение ответа сервера
					switch(response.code){
						// Если ответ сервера: Moved Permanently
						case 301:
						// Если ответ сервера: Permanent Redirect
						case 308: break;
						// Если мы получили любой другой ответ, выходим
						default: return result;
					}
					// Если адрес для выполнения переадресации указан
					if((result = this->_ws1._http.is(http_t::suite_t::HEADER, "location"))){
						// Выполняем очистку оставшихся данных
						this->_ws1._buffer.clear();
						// Получаем новый адрес запроса
						const uri_t::url_t & url = this->_ws1._http.url();
						// Если адрес запроса получен
						if((result = !url.empty())){
							// Получаем количество попыток
							this->_attempt = this->_ws1._attempt;
							// Устанавливаем новый адрес запроса
							this->_uri.combine(this->_scheme.url, url);
							
							cout << " $$$$$$$$$$$$$$ OPEN HTTP1 4 " << endl;
							
							// Выполняем установку следующего экшена на открытие подключения
							this->open();
							// Завершаем работу
							return result;
						}
					}
				}
			} break;
		}
	}
	// Выводим результат
	return result;
}
/**
 * response Метод получения ответа сервера
 * @param bid     идентификатор брокера
 * @param code    код ответа сервера
 * @param message сообщение ответа сервера
 */
void awh::client::Http1::response(const uint64_t bid, const u_int code, const string & message) noexcept {
	// Выполняем неиспользуемую переменную
	(void) bid;
	// Если функция обратного вызова на вывод ответа сервера на ранее выполненный запрос установлена
	if(!this->_requests.empty() && this->_callback.is("response"))
		// Выполняем функцию обратного вызова
		this->_callback.call <const int32_t, const u_int, const string &> ("response", this->_requests.begin()->first, code, message);
}
/**
 * header Метод получения заголовка
 * @param bid   идентификатор брокера
 * @param key   ключ заголовка
 * @param value значение заголовка
 */
void awh::client::Http1::header(const uint64_t bid, const string & key, const string & value) noexcept {
	// Выполняем неиспользуемую переменную
	(void) bid;
	// Если функция обратного вызова на полученного заголовка с сервера установлена
	if(!this->_requests.empty() && this->_callback.is("header"))
		// Выполняем функцию обратного вызова
		this->_callback.call <const int32_t, const string &, const string &> ("header", this->_requests.begin()->first, key, value);
}
/**
 * headers Метод получения заголовков
 * @param bid     идентификатор брокера
 * @param code    код ответа сервера
 * @param message сообщение ответа сервера
 * @param headers заголовки ответа сервера
 */
void awh::client::Http1::headers(const uint64_t bid, const u_int code, const string & message, const unordered_multimap <string, string> & headers) noexcept {
	// Выполняем неиспользуемую переменную
	(void) bid;
	// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
	if(!this->_requests.empty() && this->_callback.is("headers"))
		// Выполняем функцию обратного вызова
		this->_callback.call <const int32_t, const u_int, const string &, const unordered_multimap <string, string> &> ("headers", this->_requests.begin()->first, code, message, headers);
}
/**
 * chunking Метод обработки получения чанков
 * @param bid   идентификатор брокера
 * @param chunk бинарный буфер чанка
 * @param http  объект модуля HTTP
 */
void awh::client::Http1::chunking(const uint64_t bid, const vector <char> & chunk, const awh::http_t * http) noexcept {
	// Если данные получены, формируем тело сообщения
	if(!this->_requests.empty() && !chunk.empty()){
		// Выполняем неиспользуемую переменную
		(void) bid;
		// Выполняем добавление полученного чанка в тело ответа
		const_cast <awh::http_t *> (http)->body(chunk);
		// Если функция обратного вызова на вывода полученного чанка бинарных данных с сервера установлена
		if(this->_callback.is("chunks"))
			// Выполняем функцию обратного вызова
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
 * pinging Метод таймера выполнения пинга удалённого сервера
 * @param tid  идентификатор таймера
 * @param core объект сетевого ядра
 */
void awh::client::Http1::pinging(const uint16_t tid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((tid > 0) && (core != nullptr)){
		// Если агент является клиентом WebSocket
		if(this->_agent == agent_t::WEBSOCKET)
			// Выполняем переброс персистентного вызова на клиент WebSocket
			this->_ws1.pinging(tid, core);
	}
}
/**
 * prepare Метод выполнения препарирования полученных данных
 * @param sid  идентификатор запроса
 * @param bid  идентификатор брокера
 * @param core объект сетевого ядра
 * @return     результат препарирования
 */
awh::client::Web::status_t awh::client::Http1::prepare(const int32_t sid, const uint64_t bid, client::core_t * core) noexcept {
	// Результат работы функции
	status_t result = status_t::STOP;
	// Получаем параметры ответа сервера
	const auto & response = this->_http.response();
	// Получаем статус ответа
	awh::http_t::status_t status = this->_http.auth();
	// Если выполнять редиректы запрещено
	if(!this->_redirects && (status == awh::http_t::status_t::RETRY)){
		// Если нужно произвести запрос заново
		if((response.code == 201) || (response.code == 301) ||
		   (response.code == 302) || (response.code == 303) ||
		   (response.code == 307) || (response.code == 308))
				// Запрещаем выполнять редирект
				status = awh::http_t::status_t::GOOD;
	}
	// Выполняем анализ результата авторизации
	switch(static_cast <uint8_t> (status)){
		// Если нужно попытаться ещё раз
		case static_cast <uint8_t> (awh::http_t::status_t::RETRY): {
			
			
			cout << " ***************** HTTP1 " << this->_redirects << endl;
			
			// Если функция обратного вызова на на вывод ошибок установлена
			if(((response.code == 401) || (response.code == 407)) && this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP1_RECV, "authorization failed");
			// Если попытки повторить переадресацию ещё не закончились
			if(!(this->_stopped = (this->_attempt >= this->_attempts))){
				// Выполняем поиск указанного запроса
				auto it = this->_requests.find(sid);
				// Если параметры активного запроса найдены
				if(it != this->_requests.end()){
					// Получаем новый адрес запроса
					const uri_t::url_t & url = this->_http.url();
					// Если URL-адрес запроса получен
					if(!url.empty()){
						// Выполняем проверку соответствие протоколов
						const bool schema = (
							(this->_fmk->compare(url.host, it->second.url.host)) &&
							(this->_fmk->compare(url.schema, it->second.url.schema))
						);
						// Если соединение является постоянным
						if(schema && this->_http.is(http_t::state_t::ALIVE)){
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
								// Выполняем функцию обратного вызова
								this->_callback.call <const int32_t, const mode_t> ("stream", sid, mode_t::CLOSE);
							// Завершаем работу
							return status_t::SKIP;
						}
					// Если URL-адрес запроса не получен
					} else {
						// Если активирован режим работы прокси-сервера и требуется авторизация
						if((response.code == 407) && this->_proxy.mode){
							// Выполняем сброс заголовков прокси-сервера
							this->_scheme.proxy.http.clear();
							// Выполняем перебор всех полученных заголовков
							for(auto & item : this->_http.headers()){
								// Если заголовок соответствует прокси-серверу
								if(this->_fmk->exists("proxy-", item.first))
									// Выполняем добавление заголовков прокси-сервера
									this->_scheme.proxy.http.header(item.first, item.second);
							}
							// Устанавливаем статус ответа прокси-серверу
							this->_scheme.proxy.http.response(response);
							// Выполняем фиксацию полученного результата
							this->_scheme.proxy.http.commit();
						}
						// Если соединение является постоянным
						if(this->_http.is(http_t::state_t::ALIVE)){
							// Выполняем сброс параметров запроса
							this->flush();
							// Увеличиваем количество попыток
							this->_attempt++;
							// Выполняем запрос на удалённый сервер
							this->send(it->second);
							// Если функция обратного вызова активности потока установлена
							if(this->_callback.is("stream"))
								// Выполняем функцию обратного вызова
								this->_callback.call <const int32_t, const mode_t> ("stream", sid, mode_t::CLOSE);
							// Завершаем работу
							return status_t::SKIP;
						}
					}
					// Если нам необходимо отключиться
					core->close(bid);
					// Если функция обратного вызова активности потока установлена
					if(this->_callback.is("stream"))
						// Выполняем функцию обратного вызова
						this->_callback.call <const int32_t, const mode_t> ("stream", sid, mode_t::CLOSE);
					// Завершаем работу
					return status_t::SKIP;
				}
			}
		} break;
		// Если запрос выполнен удачно
		case static_cast <uint8_t> (awh::http_t::status_t::GOOD): {
			// Выполняем сброс количества попыток
			this->_attempt = 0;
			// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
			if(!this->_http.empty(awh::http_t::suite_t::BODY) && this->_callback.is("entity"))
				// Устанавливаем полученную функцию обратного вызова
				this->_resultCallback.set <void (const int32_t, const u_int, const string, const vector <char>)> ("entity", this->_callback.get <void (const int32_t, const u_int, const string, const vector <char>)> ("entity"), sid, response.code, response.message, this->_http.body());
			// Если объект ещё не удалён и получен окончательный ответ
			if((response.code >= 200) && !this->_requests.empty()){
				// Выполняем поиск указанного запроса
				auto it = this->_requests.find(sid);
				// Если параметры активного запроса найдены
				if(it != this->_requests.end())
					// Выполняем удаление объекта запроса
					this->_requests.erase(it);
			}
			// Если функция обратного вызова на получение удачного ответа установлена
			if((response.code >= 200) && this->_callback.is("handshake"))
				// Выполняем функцию обратного вызова
				this->_callback.call <const int32_t, const agent_t> ("handshake", sid, agent_t::HTTP);
			// Устанавливаем размер стопбайт
			if(!this->_http.is(http_t::state_t::ALIVE)){
				// Выполняем очистку оставшихся данных
				this->_buffer.clear();
				// Завершаем работу
				core->close(bid);
				// Выполняем завершение работы
				return status_t::STOP;
			}
			// Завершаем обработку
			return status_t::NEXT;
		} break;
		// Если запрос неудачный
		case static_cast <uint8_t> (awh::http_t::status_t::FAULT): {
			// Устанавливаем флаг принудительной остановки
			this->_stopped = true;
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP1_RECV, this->_http.message(response.code).c_str());
			// Если возникла ошибка выполнения запроса
			if((response.code >= 400) && (response.code < 500)){
				// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
				if(!this->_http.empty(awh::http_t::suite_t::BODY) && this->_callback.is("entity"))
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
	if(!this->_http.empty(awh::http_t::suite_t::BODY) && this->_callback.is("entity"))
		// Устанавливаем полученную функцию обратного вызова
		this->_resultCallback.set <void (const int32_t, const u_int, const string, const vector <char>)> ("entity", this->_callback.get <void (const int32_t, const u_int, const string, const vector <char>)> ("entity"), sid, response.code, response.message, this->_http.body());
	// Завершаем работу
	core->close(bid);
	// Выполняем завершение работы
	return status_t::STOP;
}
/**
 * sendError Метод отправки сообщения об ошибке
 * @param mess отправляемое сообщение об ошибке
 */
void awh::client::Http1::sendError(const ws::mess_t & mess) noexcept {
	// Выполняем отправку сообщения ошибки на WebSocket-сервер
	this->_ws1.sendError(mess);
}
/**
 * sendMessage Метод отправки сообщения на сервер
 * @param message передаваемое сообщения в бинарном виде
 * @param text    данные передаются в текстовом виде
 */
void awh::client::Http1::sendMessage(const vector <char> & message, const bool text) noexcept {
	// Выполняем отправку сообщения на WebSocket-сервер
	this->_ws1.sendMessage(message, text);
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
		if((this->_core != nullptr) && (this->_bid > 0)){
			// Выполняем сброс параметров запроса
			this->flush();
			// Выполняем очистку параметров HTTP-запроса
			this->_http.clear();
			// Выполняем сброс состояния HTTP-парсера
			this->_http.reset();
			// Выполняем очистку функций обратного вызова
			this->_resultCallback.clear();
			// Если список компрессоров передан
			if(!request.compressors.empty())
				// Устанавливаем список поддерживаемых компрессоров
				this->_http.compressors(request.compressors);
			// Устанавливаем список поддерживаемых компрессоров
			else this->_http.compressors(this->_compressors);
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
			// Если активирован режим прокси-сервера
			if(this->_proxy.mode){
				// Активируем точную установку хоста
				this->_http.precise(!this->_proxy.connect);
				// Выполняем извлечение заголовка авторизации на прокси-сервера
				const string & header = this->_scheme.proxy.http.auth(http_t::process_t::REQUEST, query);
				// Если заголовок авторизации получен
				if(!header.empty())
					// Выполняем установку заголовка авторизации на прокси-сервере
					this->_http.header("Proxy-Authorization", header);
				// Если установлено постоянное подключение к прокси-серверу
				if(this->_scheme.proxy.http.is(http_t::state_t::ALIVE))
					// Устанавливаем постоянное подключение к прокси-серверу
					this->_http.header("Proxy-Connection", "keep-alive");
				// Устанавливаем закрытие подключения к прокси-серверу
				else this->_http.header("Proxy-Connection", "close");
			}
			// Получаем бинарные данные HTTP-запроса
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
				core->write(buffer.data(), buffer.size(), this->_bid);
				// Получаем данные тела запроса
				while(!(entity = this->_http.payload()).empty()){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим сообщение о выводе чанка тела
						cout << this->_fmk->format("<chunk %zu>", entity.size()) << endl << endl;
					#endif
					// Выполняем отправку тела запроса на сервер
					core->write(entity.data(), entity.size(), this->_bid);
				}
			}
			// Если установлена функция отлова завершения запроса
			if(this->_callback.is("end"))
				// Выполняем функцию обратного вызова
				this->_callback.call <const int32_t, const direct_t> ("end", this->_requests.begin()->first, direct_t::SEND);
		}
	}
}
/**
 * send Метод отправки сообщения на сервер
 * @param request параметры запроса на удалённый сервер
 * @return        идентификатор отправленного запроса
 */
int32_t awh::client::Http1::send(const request_t & request) noexcept {
	
	cout << " ================== SEND HTTP1 " << endl;
	
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::READ, event_t::CONNECT}, event_t::SEND)){
		// Если сетевое ядро уже инициализированно
		if(this->_core != nullptr){
			// Если WebSocket ещё не активирован
			if(this->_agent != agent_t::WEBSOCKET){
				// Если это первый запрос
				if(this->_attempt == 0){
					// Если список заголовков установлен
					if(!request.headers.empty()){
						// Выполняем перебор всего списка заголовков
						for(auto & item : request.headers){
							// Если заголовок соответствует смене протокола на WebSocket
							if(this->_fmk->compare(item.first, "upgrade") && this->_fmk->compare(item.second, "websocket")){
								// Если протокол WebSocket разрешён для подключения
								if(this->_webSocket)
									// Выполняем установку агента воркера WebSocket
									this->_agent = agent_t::WEBSOCKET;
								// Если протокол WebSocket запрещён
								else {
									// Выводим сообщение об ошибке
									this->_log->print("Websocket protocol is prohibited for connection", log_t::flag_t::WARNING);
									// Если функция обратного вызова на на вывод ошибок установлена
									if(this->_callback.is("error"))
										// Выполняем функцию обратного вызова
										this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::WARNING, http::error_t::HTTP1_SEND, "Websocket protocol is prohibited for connection");
									// Выходим из функции
									return -1;
								}
								// Выходим из цикла
								break;
							}
						}
					}
					// Результат работы функции
					int32_t result = (this->_requests.size() + 1);
					// Определяем тип агента
					switch(static_cast <uint8_t> (this->_agent)){
						// Если протоколом агента является HTTP-клиент
						case static_cast <uint8_t> (agent_t::HTTP): {
							// Выполняем добавление активного запроса
							this->_requests.emplace(result, request);
							// Если В списке запросов ещё нет активных запросов
							if(this->_requests.size() == 1)
								// Выполняем запрос на удалённый сервер
								this->submit(request);
						} break;
						// Если протоколом агента является WebSocket-клиент
						case static_cast <uint8_t> (agent_t::WEBSOCKET): {
							// Если HTTP-заголовки установлены
							if(!request.headers.empty())
								// Выполняем установку HTTP-заголовков
								this->_ws1.setHeaders(request.headers);
							// Если список доступных компрессоров установлен
							if(!request.compressors.empty())
								// Устанавливаем список поддерживаемых компрессоров
								this->_ws1._compressors = request.compressors;
							// Устанавливаем список поддерживаемых компрессоров
							else this->_ws1._compressors = this->_compressors;
							// Устанавливаем новый адрес запроса
							this->_uri.combine(this->_ws1._scheme.url, request.url);
							// Выполняем установку подключения с WebSocket-сервером
							this->_ws1.connectCallback(this->_bid, this->_scheme.sid, dynamic_cast <awh::core_t *> (const_cast <client::core_t *> (this->_core)));
							// Выводим идентификатор подключения
							result = 1;
						} break;
					}
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
						// Выполняем функцию обратного вызова
						this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP1_SEND, "Number of redirect attempts has not been reset");
				}
			// Выводим сообщение об ошибке
			} else {
				// Выводим сообщение об ошибке
				this->_log->print("Websocket protocol is already activated", log_t::flag_t::WARNING);
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выполняем функцию обратного вызова
					this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::WARNING, http::error_t::HTTP1_SEND, "Websocket protocol is already activated");
			}
		}
	}
	// Сообщаем что идентификатор не получен
	return -1;
}
/**
 * send Метод отправки данных в бинарном виде серверу
 * @param buffer буфер бинарных данных передаваемых серверу
 * @param size   размер сообщения в байтах
 */
void awh::client::Http1::send(const char * buffer, const size_t size) noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working() && (buffer != nullptr) && (size > 0))
		// Выполняем отправку заголовков запроса серверу
		const_cast <client::core_t *> (this->_core)->write(buffer, size, this->_bid);
}
/**
 * send Метод отправки тела сообщения на сервер
 * @param buffer буфер бинарных данных передаваемых на сервер
 * @param size   размер сообщения в байтах
 * @param end    флаг последнего сообщения после которого поток закрывается
 * @return       результат отправки данных указанному клиенту
 */
bool awh::client::Http1::send(const char * buffer, const size_t size, const bool end) noexcept {
	// Результат работы функции
	bool result = false;
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::READ, event_t::CONNECT}, event_t::SEND)){
		// Если данные переданы верные
		if((result = ((this->_core != nullptr) && (buffer != nullptr) && (size > 0)))){
			// Тело WEB сообщения
			vector <char> entity;
			// Выполняем сброс данных тела
			this->_http.clear(http_t::suite_t::BODY);
			// Устанавливаем тело запроса
			this->_http.body(vector <char> (buffer, buffer + size));
			// Получаем данные тела запроса
			while(!(entity = this->_http.payload()).empty()){
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим сообщение о выводе чанка тела
					cout << this->_fmk->format("<chunk %zu>", entity.size()) << endl << endl;
				#endif
				// Устанавливаем флаг закрытия подключения
				this->_stopped = (end && this->_http.empty(awh::http_t::suite_t::BODY));
				// Выполняем отправку тела запроса на сервер
				const_cast <client::core_t *> (this->_core)->write(entity.data(), entity.size(), this->_bid);
			}
		}
	}
	// Выводим значение по умолчанию
	return result;
}
/**
 * send Метод отправки заголовков на сервер
 * @param url     адрес запроса на сервере
 * @param method  метод запроса на сервере
 * @param headers заголовки отправляемые на сервер
 * @param end     размер сообщения в байтах
 * @return        идентификатор нового запроса
 */
int32_t awh::client::Http1::send(const uri_t::url_t & url, const awh::web_t::method_t method, const unordered_multimap <string, string> & headers, const bool end) noexcept {
	// Результат работы функции
	int32_t result = -1;
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::READ, event_t::CONNECT}, event_t::SEND)){
		// Если заголовки запроса переданы
		if((this->_core != nullptr) && !headers.empty()){
			// Выполняем очистку параметров HTTP-запроса
			this->_http.clear();
			// Выполняем сброс состояния HTTP-парсера
			this->_http.reset();
			// Устанавливаем заголовоки запроса
			this->_http.headers(headers);
			// Устанавливаем новый адрес запроса
			this->_uri.combine(this->_scheme.url, url);
			// Создаём объек запроса
			awh::web_t::req_t query(method, this->_scheme.url);
			// Если активирован режим прокси-сервера
			if(this->_proxy.mode){
				// Активируем точную установку хоста
				this->_http.precise(!this->_proxy.connect);
				// Выполняем извлечение заголовка авторизации на прокси-сервера
				const string & header = this->_scheme.proxy.http.auth(http_t::process_t::REQUEST, query);
				// Если заголовок авторизации получен
				if(!header.empty())
					// Выполняем установки заголовка авторизации на прокси-сервере
					this->_http.header("Proxy-Authorization", header);
				// Если установлено постоянное подключение к прокси-серверу
				if(this->_scheme.proxy.http.is(http_t::state_t::ALIVE))
					// Устанавливаем постоянное подключение к прокси-серверу
					this->_http.header("Proxy-Connection", "keep-alive");
				// Устанавливаем закрытие подключения к прокси-серверу
				else this->_http.header("Proxy-Connection", "close");
			}
			// Получаем бинарные данные HTTP-запроса
			const auto & headers = this->_http.process(http_t::process_t::REQUEST, std::move(query));
			// Если заголовки запроса получены
			if(!headers.empty()){
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим заголовок запроса
					cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
					// Выводим параметры запроса
					cout << string(headers.begin(), headers.end()) << endl << endl;
				#endif
				// Устанавливаем флаг закрытия подключения
				this->_stopped = end;
				// Выполняем отправку заголовков запроса на сервер
				const_cast <client::core_t *> (this->_core)->write(headers.data(), headers.size(), this->_bid);
				// Устанавливаем результат
				result = 1;
			}
		}
	}
	// Выводим значение по умолчанию
	return result;
}
/**
 * pause Метод установки на паузу клиента
 */
void awh::client::Http1::pause() noexcept {
	// Выполняем постановку на паузу
	this->_ws1.pause();
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
 * on Метод установки функции обратного вызова на событие получения ошибок
 * @param callback функция обратного вызова
 */
void awh::client::Http1::on(function <void (const u_int, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param callback функция обратного вызова
 */
void awh::client::Http1::on(function <void (const vector <char> &, const bool)> callback) noexcept {
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws1.on(callback);
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
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws1.on(callback);
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
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws1.on(callback);
	// Устанавливаем функцию обратного вызова для вывода ошибок
	this->_http.on(std::bind(&http1_t::errors, this, _1, _2, _3, _4));
}
/**
 * on Метод установки функции вывода бинарных данных в сыром виде полученных с клиента
 * @param callback функция обратного вызова
 */
void awh::client::Http1::on(function <bool (const char *, const size_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws1.on(callback);
}
/**
 * on Метод установки функция обратного вызова активности потока
 * @param callback функция обратного вызова
 */
void awh::client::Http1::on(function <void (const int32_t, const mode_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws1.on(callback);
}
/**
 * on Метод установки функция обратного вызова при выполнении рукопожатия
 * @param callback функция обратного вызова
 */
void awh::client::Http1::on(function <void (const int32_t, const agent_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции обратного вызова при завершении запроса
 * @param callback функция обратного вызова
 */
void awh::client::Http1::on(function <void (const int32_t, const direct_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции вывода полученного чанка бинарных данных с сервера
 * @param callback функция обратного вызова
 */
void awh::client::Http1::on(function <void (const int32_t, const vector <char> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции вывода ответа сервера на ранее выполненный запрос
 * @param callback функция обратного вызова
 */
void awh::client::Http1::on(function <void (const int32_t, const u_int, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws1.on(callback);
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
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws1.on(callback);
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
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции вывода полученных заголовков с сервера
 * @param callback функция обратного вызова
 */
void awh::client::Http1::on(function <void (const int32_t, const u_int, const string &, const unordered_multimap <string, string> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws1.on(callback);
	// Устанавливаем функцию обратного вызова для HTTP/1.1
	this->_http.on((function <void (const uint64_t, const u_int, const string &, const unordered_multimap <string, string> &)>) std::bind(&http1_t::headers, this, _1, _2, _3, _4));
}
/**
 * subprotocol Метод установки поддерживаемого сабпротокола
 * @param subprotocol сабпротокол для установки
 */
void awh::client::Http1::subprotocol(const string & subprotocol) noexcept {
	// Выполняем установку поддерживаемого сабпротокола
	this->_ws1.subprotocol(subprotocol);
}
/**
 * subprotocol Метод получения списка выбранных сабпротоколов
 * @return список выбранных сабпротоколов
 */
const set <string> & awh::client::Http1::subprotocols() const noexcept {
	// Выполняем извлечение списка выбранных сабпротоколов
	return this->_ws1.subprotocols();
}
/**
 * subprotocols Метод установки списка поддерживаемых сабпротоколов
 * @param subprotocols сабпротоколы для установки
 */
void awh::client::Http1::subprotocols(const set <string> & subprotocols) noexcept {
	// Выполняем установку поддерживаемых сабпротоколов
	this->_ws1.subprotocols(subprotocols);
}
/**
 * extensions Метод извлечения списка расширений
 * @return список поддерживаемых расширений
 */
const vector <vector <string>> & awh::client::Http1::extensions() const noexcept {
	// Выводим список расширений принадлежащих WebSocket-клиенту
	return this->_ws1.extensions();
}
/**
 * extensions Метод установки списка расширений
 * @param extensions список поддерживаемых расширений
 */
void awh::client::Http1::extensions(const vector <vector <string>> & extensions) noexcept {
	// Выполняем установку списка доступных расширений для WebSocket-клиента
	this->_ws1.extensions(extensions);
}
/**
 * chunk Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::client::Http1::chunk(const size_t size) noexcept {
	// Устанавливаем размер чанка для WebSocket-клиента
	this->_ws1.chunk(size);
	// Устанавливаем размер чанка
	this->_http.chunk(size);
}
/**
 * segmentSize Метод установки размеров сегментов фрейма
 * @param size минимальный размер сегмента
 */
void awh::client::Http1::segmentSize(const size_t size) noexcept {
	// Если размер передан, устанавливаем
	if(size > 0)
		// Устанавливаем размер сегментов фрейма для WebSocket-клиента
		this->_ws1.segmentSize(size);
}
/**
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::client::Http1::mode(const set <flag_t> & flags) noexcept {
	// Устанавливаем флаги настроек модуля для WebSocket-клиента
	this->_ws1.mode(flags);
	// Устанавливаем флаг анбиндинга ядра сетевого модуля
	this->_unbind = (flags.count(flag_t::NOT_STOP) == 0);
	// Устанавливаем флаг поддержания автоматического подключения
	this->_scheme.alive = (flags.count(flag_t::ALIVE) > 0);
	// Устанавливаем флаг разрешающий выполнять редиректы
	this->_redirects = (flags.count(flag_t::REDIRECTS) > 0);
	// Если установлен флаг запрещающий переключение контекста SSL
	this->_noinitssl = (flags.count(flag_t::NO_INIT_SSL) > 0);
	// Устанавливаем флаг ожидания входящих сообщений
	this->_scheme.wait = (flags.count(flag_t::WAIT_MESS) > 0);
	// Устанавливаем флаг разрешающий выполнять подключение к протоколу WebSocket
	this->_webSocket = (flags.count(flag_t::WEBSOCKET_ENABLE) > 0);
	// Устанавливаем флаг разрешающий выполнять метод CONNECT для прокси-клиента
	this->_proxy.connect = (flags.count(flag_t::CONNECT_METHOD_ENABLE) > 0);
	// Если сетевое ядро установлено
	if(this->_core != nullptr){
		// Устанавливаем флаг запрещающий вывод информационных сообщений
		const_cast <client::core_t *> (this->_core)->noInfo(flags.count(flag_t::NOT_INFO) > 0);
		// Выполняем установку флага проверки домена
		const_cast <client::core_t *> (this->_core)->verifySSL(flags.count(flag_t::VERIFY_SSL) > 0);
	}
}
/**
 * core Метод установки сетевого ядра
 * @param core объект сетевого ядра
 */
void awh::client::Http1::core(const client::core_t * core) noexcept {
	// Если объект сетевого ядра передан
	if(core != nullptr){
		// Выполняем передачу настроек сетевого ядра в родительский модуль
		web_t::core(core);
		// Если многопоточность активированна
		if(this->_threads > 0)
			// Устанавливаем простое чтение базы событий
			const_cast <client::core_t *> (this->_core)->easily(true);
	// Если объект сетевого ядра не передан но ранее оно было добавлено
	} else if(this->_core != nullptr) {
		// Если многопоточность активированна
		if(this->_threads <= 0){
			// Если многопоточность активированна
			if(this->_ws1._thr.is())
				// Выполняем завершение всех активных потоков
				this->_ws1._thr.wait();
			// Снимаем режим простого чтения базы событий
			const_cast <client::core_t *> (this->_core)->easily(false);
		}
		// Выполняем передачу настроек сетевого ядра в родительский модуль
		web_t::core(core);
	}
}
/**
 * user Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::client::Http1::user(const string & login, const string & password) noexcept {
	// Устанавливаем логин и пароль пользователя для WebSocket-клиента
	this->_ws1.user(login, password);
	// Устанавливаем логин и пароль пользователя
	this->_http.user(login, password);
}
/**
 * userAgent Метод установки User-Agent для HTTP-запроса
 * @param userAgent агент пользователя для HTTP-запроса
 */
void awh::client::Http1::userAgent(const string & userAgent) noexcept {
	// Устанавливаем UserAgent
	if(!userAgent.empty()){
		// Устанавливаем пользовательского агента у родительского класса
		web_t::userAgent(userAgent);
		// Устанавливаем пользовательского агента для WebSocket-клиента
		this->_ws1.userAgent(userAgent);
		// Устанавливаем пользовательского агента
		this->_http.userAgent(userAgent);
	}
}
/**
 * ident Метод установки идентификации клиента
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::client::Http1::ident(const string & id, const string & name, const string & ver) noexcept {
	// Если данные сервиса переданы
	if(!id.empty() && !name.empty() && !ver.empty()){
		// Выполняем установку данных сервиса у родительского класса
		web_t::ident(id, name, ver);
		// Устанавливаем данные сервиса для WebSocket-клиента
		this->_ws1.ident(id, name, ver);
		// Устанавливаем данные сервиса
		this->_http.ident(id, name, ver);
	}
}
/**
 * multiThreads Метод активации многопоточности
 * @param count количество потоков для активации
 * @param mode  флаг активации/деактивации мультипоточности
 */
void awh::client::Http1::multiThreads(const int16_t count, const bool mode) noexcept {
	// Если необходимо активировать мультипоточность
	if(mode)
		// Выполняем установку количества ядер мультипоточности
		this->_threads = count;
	// Если необходимо отключить мультипоточность
	else this->_threads = -1;
}
/**
 * proxy Метод установки прокси-сервера
 * @param uri    параметры прокси-сервера
 * @param family семейстово интернет протоколов (IPV4 / IPV6 / NIX)
 */
void awh::client::Http1::proxy(const string & uri, const scheme_t::family_t family) noexcept {
	// Выполняем установку параметры прокси-сервера
	web_t::proxy(uri, family);
	// Выполняем установку параметры прокси-сервера для WebSocket-клиента
	this->_ws1.proxy(uri, family);
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest-авторизации
 */
void awh::client::Http1::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Устанавливаем параметры авторизации для WebSocket-клиента
	this->_ws1.authType(type, hash);
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
	// Устанавливаем тип авторизации на проксе-сервере для WebSocket-клиента
	this->_ws1.authTypeProxy(type, hash);
}
/**
 * encryption Метод активации шифрования
 * @param mode флаг активации шифрования
 */
void awh::client::Http1::encryption(const bool mode) noexcept {
	// Устанавливаем флаг шифрования в родительском модуле
	web_t::encryption(mode);
	// Устанавливаем флаг шифрования для WebSocket-клиента
	this->_ws1.encryption(mode);
	// Устанавливаем флаг шифрования для HTTP-клиента
	this->_http.encryption(mode);
}
/**
 * encryption Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::client::Http1::encryption(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Устанавливаем параметры шифрования в родительском модуле
	web_t::encryption(pass, salt, cipher);
	// Устанавливаем параметры шифрования для WebSocket-клиента
	this->_ws1.encryption(pass, salt, cipher);
	// Устанавливаем параметры шифрования для HTTP-клиента
	this->_http.encryption(pass, salt, cipher);
}
/**
 * Http1 Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::client::Http1::Http1(const fmk_t * fmk, const log_t * log) noexcept :
 web_t(fmk, log), _mode(false), _webSocket(false), _ws1(fmk, log), _http(fmk, log), _agent(agent_t::HTTP), _threads(-1), _resultCallback(log) {
	// Устанавливаем функцию обработки вызова для получения чанков для HTTP-клиента
	this->_http.on(std::bind(&http1_t::chunking, this, _1, _2, _3));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("write", std::bind(&http1_t::writeCallback, this, _1, _2, _3, _4, _5));
}
/**
 * Http1 Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::client::Http1::Http1(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 web_t(core, fmk, log), _mode(false), _webSocket(false), _ws1(fmk, log), _http(fmk, log), _agent(agent_t::HTTP), _threads(-1), _resultCallback(log) {
	// Устанавливаем функцию обработки вызова для получения чанков для HTTP-клиента
	this->_http.on(std::bind(&http1_t::chunking, this, _1, _2, _3));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("write", std::bind(&http1_t::writeCallback, this, _1, _2, _3, _4, _5));
}
/**
 * ~Http1 Деструктор
 */
awh::client::Http1::~Http1() noexcept {
	// Снимаем адрес сетевого ядра
	this->_ws1._core = nullptr;
	// Если многопоточность активированна
	if(this->_ws1._thr.is())
		// Выполняем завершение всех активных потоков
		this->_ws1._thr.wait();
}
