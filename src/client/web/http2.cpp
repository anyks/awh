/**
 * @file: http2.cpp
 * @date: 2023-09-18
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
#include <client/web/http2.hpp>

/**
 * connectCallback Метод обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::Http2::connectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::OPEN, event_t::READ, event_t::PROXY_READ}, event_t::CONNECT)){
		// Запоминаем идентификатор адъютанта
		this->_aid = aid;
		// Выполняем инициализацию сессии HTTP/2
		web2_t::connectCallback(aid, sid, core);
		// Если режим работы с HTTP/2 протоколом не активирован
		if(!this->_upgraded){
			// Запоминаем идентификатор адъютанта
			this->_http1._aid = this->_aid;
			// Выполняем установку сетевого ядра
			this->_http1._core = this->_core;
			// Выполняем установку данных URL-адреса
			this->_http1._scheme.url = this->_scheme.url;
			// Активируем синхронный режим работы
			dynamic_cast <client::core_t *> (core)->mode(client::core_t::mode_t::SYNC);
		// Активируем асинхронный режим работы
		} else dynamic_cast <client::core_t *> (core)->mode(client::core_t::mode_t::ASYNC);
		// Выполняем установку сетевого ядра
		this->_ws2._core = this->_core;
		// Устанавливаем флаг переключения протокола на HTTP/2
		this->_ws2._upgraded = this->_upgraded;
		// Выполняем установку данных URL-адреса
		this->_ws2._scheme.url = this->_scheme.url;
		// Если функция обратного вызова при подключении/отключении установлена
		if(this->_callback.is("active"))
			// Выполняем установку функции обратного вызова
			this->_ws2._callback.set <void (const mode_t)> ("active", this->_callback.get <void (const mode_t)> ("active"));
		// Если функция обратного вызова, для вывода полученного чанка бинарных данных с сервера установлена
		if(this->_callback.is("chunks"))
			// Выполняем установку функции обратного вызова
			this->_ws2._callback.set <void (const int32_t, const vector <char> &)> ("chunks", this->_callback.get <void (const int32_t, const vector <char> &)> ("chunks"));
		// Если многопоточность активированна
		if(this->_threads > -1)
			// Выполняем инициализацию нового тредпула
			this->_ws2.multiThreads(this->_threads);
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
void awh::client::Http2::disconnectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если протокол подключения установлен как HTTP/2
	if(this->_upgraded && (this->_session != nullptr))
		// Выполняем остановку активной сессии
		nghttp2_session_terminate_session(this->_session, NGHTTP2_NO_ERROR);
	// Выполняем поиск активного воркера который необходимо перезапустить
	for(auto it = this->_workers.begin(); it != this->_workers.end(); ++it){
		// Определяем тип агента
		switch(static_cast <uint8_t> (it->second->agent)){
			// Если протоколом агента является HTTP-клиент
			case static_cast <uint8_t> (agent_t::HTTP): {
				// Если протокол подключения установлен как HTTP/2
				if(it->second->proto == engine_t::proto_t::HTTP2){
					// Если мы нашли нужный нам воркер
					if(it->second->update){
						// Получаем параметры запроса
						const auto & response = it->second->http.response();
						// Если нужно произвести запрос заново
						if(!this->_stopped && ((response.code == 201) || (response.code == 301) || (response.code == 302) ||
						  (response.code == 303) || (response.code == 307) || (response.code == 308) || (response.code == 401))){
							// Отключаем флаг HTTP/2 так-как сессия уже закрыта
							this->_upgraded = false;
							// Определяем код ответа сервера
							switch(response.code){
								// Если требуется повторить попытку авторизации
								case 401: {
									// Увеличиваем количество попыток
									this->_attempt++;
									// Выполняем установку следующего экшена на открытие подключения
									this->open();
									// Завершаем работу
									return;
								}
								// Если требуется выполнить редирект
								default: {
									// Если адрес для выполнения переадресации указан
									if(it->second->http.isHeader("location")){
										// Получаем новый адрес запроса
										const uri_t::url_t & url = it->second->http.getUrl();
										// Если адрес запроса получен
										if(!url.empty()){
											// Увеличиваем количество попыток
											this->_attempt++;
											// Устанавливаем новый адрес запроса
											this->_uri.combine(this->_scheme.url, url);
											// Выполняем поиск параметров запроса
											auto jt = this->_requests.find(it->first);
											// Если необходимые нам параметры запроса найдены
											if(jt != this->_requests.end())
												// Устанавливаем новый адрес запроса
												jt->second->url = this->_scheme.url;
											// Выполняем установку следующего экшена на открытие подключения
											this->open();
											// Завершаем работу
											return;
										}
									}
								}
							}
						}
					}
				// Если активирован режим работы с HTTP/1.1 протоколом
				} else {
					// Выполняем передачу сигнала отключения от сервера на HTTP/1.1 клиент
					this->_http1.disconnectCallback(aid, sid, core);
					// Получаем параметры запроса
					const auto & response = this->_http1._http.response();
					// Если нужно произвести запрос заново
					if((it->second->update = !this->_http1._stopped && ((response.code == 201) ||
					  (response.code == 301) || (response.code == 302) || (response.code == 303) ||
					  (response.code == 307) || (response.code == 308) || (response.code == 401)))){
						// Определяем код ответа сервера
						switch(response.code){
							// Если требуется повторить попытку авторизации
							case 401: {
								// Выполняем очистку оставшихся данных
								this->_http1._buffer.clear();
								// Получаем количество попыток
								this->_attempt = this->_http1._attempt;
								// Выполняем установку следующего экшена на открытие подключения
								this->open();
								// Завершаем работу
								return;
							}
							// Если требуется выполнить редирект
							default: {
								// Если адрес для выполнения переадресации указан
								if(this->_http1._http.isHeader("location")){
									// Выполняем очистку оставшихся данных
									this->_http1._buffer.clear();
									// Получаем количество попыток
									this->_attempt = this->_http1._attempt;
									// Устанавливаем новый адрес запроса
									this->_uri.combine(this->_scheme.url, this->_http1._scheme.url);
									// Выполняем поиск параметров запроса
									auto jt = this->_requests.find(it->first);
									// Если необходимые нам параметры запроса найдены
									if(jt != this->_requests.end())
										// Устанавливаем новый адрес запроса
										jt->second->url = this->_scheme.url;
									// Выполняем установку следующего экшена на открытие подключения
									this->open();
									// Завершаем работу
									return;
								}
							}
						}
					}
				}
			} break;
			// Если протоколом агента является WebSocket-клиент
			case static_cast <uint8_t> (agent_t::WEBSOCKET): {
				// Отключаем флаг HTTP/2 так-как сессия уже закрыта
				this->_upgraded = false;
				// Отключаем флаг HTTP/2 у WebSocket-клиента, так-как сессия уже закрыта
				this->_ws2._upgraded = false;
				// Выполняем передачу сигнала отключения от сервера на WebSocket-клиент
				this->_ws2.disconnectCallback(aid, sid, core);
				// Получаем параметры запроса
				const auto & response = this->_ws2._http.response();
				// Если нужно произвести запрос заново
				if((it->second->update = !this->_ws2._stopped && ((response.code == 301) || (response.code == 308) || (response.code == 401)))){
					// Определяем код ответа сервера
					switch(response.code){
						// Если требуется повторить попытку авторизации
						case 401: {
							// Выполняем очистку оставшихся данных
							this->_ws2._buffer.clear();
							// Получаем количество попыток
							this->_attempt = this->_ws2._attempt;
							// Выполняем установку следующего экшена на открытие подключения
							this->open();
							// Завершаем работу
							return;
						}
						// Если требуется выполнить редирект
						default: {
							// Если адрес для выполнения переадресации указан
							if(this->_ws2._http.isHeader("location")){
								// Выполняем очистку оставшихся данных
								this->_ws2._buffer.clear();
								// Получаем количество попыток
								this->_attempt = this->_ws2._attempt;
								// Устанавливаем новый адрес запроса
								this->_uri.combine(this->_scheme.url, this->_ws2._scheme.url);
								// Выполняем поиск параметров запроса
								auto jt = this->_requests.find(it->first);
								// Если необходимые нам параметры запроса найдены
								if(jt != this->_requests.end())
									// Устанавливаем новый адрес запроса
									jt->second->url = this->_scheme.url;
								// Выполняем установку следующего экшена на открытие подключения
								this->open();
								// Завершаем работу
								return;
							}
						}
					}
				}
			}
		}
	}
	// Выполняем очистку списка воркеров
	this->_workers.clear();
	// Выполняем очистку списка запросов
	this->_requests.clear();
	// Выполняем передачу сигнала отключения от сервера на WebSocket-клиент
	this->_ws2.disconnectCallback(aid, sid, core);
	// Выполняем передачу сигнала отключения от сервера на HTTP/1.1 клиент
	this->_http1.disconnectCallback(aid, sid, core);
	// Если подключение не является постоянным
	if(!this->_scheme.alive){
		// Выполняем сброс параметров запроса
		this->flush();
		// Выполняем зануление идентификатора адъютанта
		this->_aid = 0;
		// Очищаем адрес сервера
		this->_scheme.url.clear();
		// Если завершить работу разрешено
		if(this->_unbind)
			// Завершаем работу
			dynamic_cast <client::core_t *> (core)->stop();
	}
	// Отключаем флаг HTTP/2 так-как сессия уже закрыта
	this->_upgraded = false;
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
void awh::client::Http2::readCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (sid > 0)){
		// Если протокол подключения является HTTP/2
		if(core->proto(aid) == engine_t::proto_t::HTTP2){
			// Выполняем извлечение полученного чанка данных из сокета
			ssize_t bytes = nghttp2_session_mem_recv(this->_session, (const uint8_t *) buffer, size);
			// Если данные не прочитаны, выводим ошибку и выходим
			if(bytes < 0){
				// Выводим сообщение об полученной ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(static_cast <int> (bytes)));
				// Выходим из функции
				return;
			}
			// Фиксируем полученный результат
			if((bytes = nghttp2_session_send(this->_session)) != 0){
				// Выводим сообщение об полученной ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(static_cast <int> (bytes)));
				// Выходим из функции
				return;
			}
		// Если активирован режим работы с HTTP/1.1 протоколом
		} else {
			// Если активирован WebSocket-клиент
			if(this->_ws2._aid > 0)
				// Выполняем переброс вызова чтения на клиент WebSocket
				this->_ws2.readCallback(buffer, size, aid, sid, core);
			// Выполняем переброс вызова чтения на клиент HTTP/1.1
			else this->_http1.readCallback(buffer, size, aid, sid, core);
		}
	}
}
/**
 * writeCallback Метод обратного вызова при записи сообщения на клиенте
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::client::Http2::writeCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (sid > 0) && (core != nullptr))
		// Выполняем переброс вызова записи на клиент WebSocket
		this->_ws2.writeCallback(buffer, size, aid, sid, core);
}
/**
 * persistCallback Функция персистентного вызова
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::Http2::persistCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Выполняем перебор всех доступных воркеров
		for(auto & worker : this->_workers){
			// Определяем протокол клиента
			switch(static_cast <uint8_t> (worker.second->agent)){
				// Если агент является клиентом HTTP
				case static_cast <uint8_t> (agent_t::HTTP): {
					// Если переключение протокола на HTTP/2 выполнено и пинг не прошёл
					if(this->_upgraded && !this->ping())
						// Завершаем работу
						dynamic_cast <client::core_t *> (core)->close(aid);
				} break;
				// Если агент является клиентом WebSocket
				case static_cast <uint8_t> (agent_t::WEBSOCKET):
					// Выполняем переброс персистентного вызова на клиент WebSocket
					this->_ws2.persistCallback(aid, sid, core);
				break;
			}
		}
	}
}
/**
 * signalFrame Метод обратного вызова при получении фрейма заголовков HTTP/2 с сервера
 * @param frame   объект фрейма заголовков HTTP/2
 * @return        статус полученных данных
 */
int awh::client::Http2::signalFrame(const nghttp2_frame * frame) noexcept {
	// Выполняем поиск идентификатора воркера
	auto it = this->_workers.find(frame->hd.stream_id);
	// Если необходимый нам воркер найден
	if(it != this->_workers.end()){
		// Выполняем определение типа фрейма
		switch(frame->hd.type){
			// Если мы получили входящие данные тела ответа
			case NGHTTP2_DATA: {
				// Определяем протокол клиента
				switch(static_cast <uint8_t> (it->second->agent)){
					// Если агент является клиентом HTTP
					case static_cast <uint8_t> (agent_t::HTTP): {
						// Если мы получили флаг завершения потока
						if(frame->hd.flags & NGHTTP2_FLAG_END_STREAM){
							// Выполняем коммит полученного результата
							it->second->http.commit();
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								{
									// Если тело ответа существует
									if(!it->second->http.body().empty())
										// Выводим сообщение о выводе чанка тела
										cout << this->_fmk->format("<body %u>", it->second->http.body().size()) << endl << endl;
									// Иначе устанавливаем перенос строки
									else cout << endl;
								}
							#endif
							// Выполняем препарирование полученных данных
							switch(static_cast <uint8_t> (this->prepare(frame->hd.stream_id, this->_aid, const_cast <client::core_t *> (this->_core)))){
								// Если необходимо выполнить пропуск обработки данных
								case static_cast <uint8_t> (status_t::SKIP): {
									// Если функция обратного вызова активности потока установлена
									if(this->_callback.is("stream"))
										// Выводим функцию обратного вызова
										this->_callback.call <const int32_t, const mode_t> ("stream", frame->hd.stream_id, mode_t::CLOSE);
									// Завершаем работу
									return 0;
								}
							}
							// Если функция обратного вызова активности потока установлена
							if(this->_callback.is("stream"))
								// Выводим функцию обратного вызова
								this->_callback.call <const int32_t, const mode_t> ("stream", frame->hd.stream_id, mode_t::CLOSE);
							// Если функция обратного вызова установлена, выводим сообщение
							if(it->second->callback.is("entity"))
								// Выполняем функцию обратного вызова дисконнекта
								it->second->callback.bind <const int32_t, const u_int, const string, const vector <char>> ("entity");
							// Выполняем удаление выполненного воркера
							this->_workers.erase(frame->hd.stream_id);
						}
					} break;
					// Если агент является клиентом WebSocket
					case static_cast <uint8_t> (agent_t::WEBSOCKET):
						// Выполняем передачу на WebSocket-клиент
						this->_ws2.signalFrame(frame);
					break;
				}
			} break;
			// Если мы получили входящие данные заголовков ответа
			case NGHTTP2_HEADERS: {
				// Определяем протокол клиента
				switch(static_cast <uint8_t> (it->second->agent)){
					// Если агент является клиентом HTTP
					case static_cast <uint8_t> (agent_t::HTTP): {
						// Если сессия клиента совпадает с сессией полученных даных и передача заголовков завершена
						if(frame->hd.flags & NGHTTP2_FLAG_END_HEADERS){
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								{
									// Получаем данные ответа
									const auto & response = it->second->http.process(http_t::process_t::RESPONSE, true);
									// Если параметры ответа получены
									if(!response.empty())
										// Выводим параметры ответа
										cout << string(response.begin(), response.end()) << endl;
								}
							#endif
							// Получаем параметры запроса
							const auto & response = it->second->http.response();
							// Если функция обратного вызова на вывод ответа сервера на ранее выполненный запрос установлена
							if(this->_callback.is("response"))
								// Выводим функцию обратного вызова
								this->_callback.call <const int32_t, const u_int, const string &> ("response", frame->hd.stream_id, response.code, response.message);
							// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
							if(this->_callback.is("headers"))
								// Выводим функцию обратного вызова
								this->_callback.call <const int32_t, const u_int, const string &, const unordered_multimap <string, string> &> ("headers", frame->hd.stream_id, response.code, response.message, it->second->http.headers());
						}
					} break;
					// Если агент является клиентом WebSocket
					case static_cast <uint8_t> (agent_t::WEBSOCKET):
						// Выполняем передачу на WebSocket-клиент
						this->_ws2.signalFrame(frame);
					break;
				}
			} break;
		}
	}
	// Выводим результат
	return 0;
}
/**
 * signalChunk Метод обратного вызова при получении чанка с сервера HTTP/2
 * @param sid    идентификатор потока
 * @param buffer буфер данных который содержит полученный чанк
 * @param size   размер полученного буфера данных чанка
 * @return       статус полученных данных
 */
int awh::client::Http2::signalChunk(const int32_t sid, const uint8_t * buffer, const size_t size) noexcept {
	// Выполняем поиск идентификатора воркера
	auto it = this->_workers.find(sid);
	// Если необходимый нам воркер найден
	if(it != this->_workers.end()){
		// Если функция обратного вызова на перехват входящих чанков установлена
		if(this->_callback.is("chunking"))
			// Выводим функцию обратного вызова
			this->_callback.call <const vector <char> &, const awh::http_t *> ("chunking", vector <char> (buffer, buffer + size), &it->second->http);
		// Если функция перехвата полученных чанков не установлена
		else {
			// Определяем протокол клиента
			switch(static_cast <uint8_t> (it->second->agent)){
				// Если агент является клиентом HTTP
				case static_cast <uint8_t> (agent_t::HTTP):
					// Добавляем полученный чанк в тело данных
					it->second->http.body(vector <char> (buffer, buffer + size));
				break;
				// Если агент является клиентом WebSocket
				case static_cast <uint8_t> (agent_t::WEBSOCKET):
					// Выполняем передачу полученных данных на WebSocket-клиент
					this->_ws2.signalChunk(sid, buffer, size);
				break;
			}
		}
	}
	// Выводим результат
	return 0;
}
/**
 * signalBeginHeaders Метод начала получения фрейма заголовков HTTP/2
 * @param sid идентификатор потока
 * @return    статус полученных данных
 */
int awh::client::Http2::signalBeginHeaders(const int32_t sid) noexcept {
	// Выполняем поиск идентификатора воркера
	auto it = this->_workers.find(sid);
	// Если необходимый нам воркер найден
	if(it != this->_workers.end()){
		// Определяем протокол клиента
		switch(static_cast <uint8_t> (it->second->agent)){
			// Если агент является клиентом HTTP
			case static_cast <uint8_t> (agent_t::HTTP): {
				// Выполняем очистку параметров HTTP запроса
				it->second->http.clear();
				// Если функция обратного вызова активности потока установлена
				if(this->_callback.is("stream"))
					// Выводим функцию обратного вызова
					this->_callback.call <const int32_t, const mode_t> ("stream", sid, mode_t::OPEN);
			} break;
			// Если агент является клиентом WebSocket
			case static_cast <uint8_t> (agent_t::WEBSOCKET):
				// Выполняем инициализации заголовков на WebSocket-клиенте
				this->_ws2.signalBeginHeaders(sid);
			break;
		}
	}
	// Выводим результат
	return 0;
}
/**
 * signalStreamClosed Метод завершения работы потока
 * @param sid   идентификатор потока
 * @param error флаг ошибки HTTP/2 если присутствует
 * @return      статус полученных данных
 */
int awh::client::Http2::signalStreamClosed(const int32_t sid, const uint32_t error) noexcept {
	// Выполняем поиск идентификатора воркера
	auto it = this->_workers.find(sid);
	// Если необходимый нам воркер найден
	if(it != this->_workers.end()){
		// Флаг отключения от сервера
		bool stop = false;
		// Определяем тип получаемой ошибки
		switch(error){
			// Если получена ошибка протокола
			case 0x1:
				// Выводим информацию о закрытии сессии с ошибкой
				this->_log->print("Stream %d closed with error=%s", log_t::flag_t::WARNING, it->first, "PROTOCOL_ERROR");
			break;
			// Если получена ошибка реализации
			case 0x2: {
				// Выполняем установку флага остановки
				stop = true;
				// Выводим информацию о закрытии сессии с ошибкой
				this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, it->first, "INTERNAL_ERROR");
			} break;
			// Если получена ошибка превышения предела управления потоком
			case 0x3: {
				// Выполняем установку флага остановки
				stop = true;
				// Выводим информацию о закрытии сессии с ошибкой
				this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, it->first, "FLOW_CONTROL_ERROR");
			} break;
			// Если установка не подтверждённа
			case 0x4: {
				// Выполняем установку флага остановки
				stop = true;
				// Выводим информацию о закрытии сессии с ошибкой
				this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, it->first, "SETTINGS_TIMEOUT");
			} break;
			// Если получен кадр для завершения потока
			case 0x5:
				// Выводим информацию о закрытии сессии с ошибкой
				this->_log->print("Stream %d closed with error=%s", log_t::flag_t::WARNING, it->first, "STREAM_CLOSED");
			break;
			// Если размер кадра некорректен
			case 0x6: {
				// Выполняем установку флага остановки
				stop = true;
				// Выводим информацию о закрытии сессии с ошибкой
				this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, it->first, "FRAME_SIZE_ERROR");
			} break;
			// Если поток не обработан
			case 0x7: {
				// Выполняем установку флага остановки
				stop = true;
				// Выводим информацию о закрытии сессии с ошибкой
				this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, it->first, "REFUSED_STREAM");
			} break;
			// Если поток аннулирован
			case 0x8: {
				// Выполняем установку флага остановки
				stop = true;
				// Выводим информацию о закрытии сессии с ошибкой
				this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, it->first, "CANCEL");
			} break;
			// Если состояние компрессии не обновлено
			case 0x9:
				// Выводим информацию о закрытии сессии с ошибкой
				this->_log->print("Stream %d closed with error=%s", log_t::flag_t::WARNING, it->first, "COMPRESSION_ERROR");
			break;
			// Если получена ошибка TCP-соединения для метода CONNECT
			case 0xa: {
				// Выполняем установку флага остановки
				stop = true;
				// Выводим информацию о закрытии сессии с ошибкой
				this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, it->first, "CONNECT_ERROR");
			} break;
			// Если превышена емкость для обработки
			case 0xb: {
				// Выполняем установку флага остановки
				stop = true;
				// Выводим информацию о закрытии сессии с ошибкой
				this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, it->first, "ENHANCE_YOUR_CALM");
			} break;
			// Если согласованные параметры TLS не приемлемы
			case 0xc: {
				// Выполняем установку флага остановки
				stop = true;
				// Выводим информацию о закрытии сессии с ошибкой
				this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, it->first, "INADEQUATE_SECURITY");
			} break;
			// Если для запроса используется HTTP/1.1
			case 0xd: {
				// Выполняем установку флага остановки
				stop = true;
				// Выводим информацию о закрытии сессии с ошибкой
				this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, it->first, "HTTP_1_1_REQUIRED");
			} break;
		}
		// Выполняем удаление указанного воркера
		this->_workers.erase(it);
		// Выполняем удаление параметра запроса
		this->_requests.erase(sid);
		// Если разрешено выполнить остановку
		if(stop){
			// Отключаем флаг HTTP/2 так-как сессия уже закрыта
			this->_upgraded = false;
			// Если сессия HTTP/2 закрыта не удачно
			if(nghttp2_session_terminate_session(this->_session, NGHTTP2_NO_ERROR) != 0){
				// Выполняем отключение от сервера
				const_cast <client::core_t *> (this->_core)->close(this->_aid);
				// Выводим сообщение об ошибке
				return NGHTTP2_ERR_CALLBACK_FAILURE;
			// Выполняем отключение от сервера
			} else const_cast <client::core_t *> (this->_core)->close(this->_aid);
		}
	}
	// Выводим результат
	return 0;
}
/**
 * signalHeader Метод обратного вызова при получении заголовка HTTP/2
 * @param sid идентификатор потока
 * @param key данные ключа заголовка
 * @param val данные значения заголовка
 * @return    статус полученных данных
 */
int awh::client::Http2::signalHeader(const int32_t sid, const string & key, const string & val) noexcept {
	// Выполняем поиск идентификатора воркера
	auto it = this->_workers.find(sid);
	// Если необходимый нам воркер найден
	if(it != this->_workers.end()){
		// Определяем протокол клиента
		switch(static_cast <uint8_t> (it->second->agent)){
			// Если агент является клиентом HTTP
			case static_cast <uint8_t> (agent_t::HTTP): {
				// Устанавливаем полученные заголовки
				it->second->http.header2(key, val);
				// Если функция обратного вызова на полученного заголовка с сервера установлена
				if(this->_callback.is("header"))
					// Выводим функцию обратного вызова
					this->_callback.call <const int32_t, const string &, const string &> ("header", sid, key, val);
			} break;
			// Если агент является клиентом WebSocket
			case static_cast <uint8_t> (agent_t::WEBSOCKET):
				// Выполняем отправку полученных заголовков на WebSocket-клиент
				this->_ws2.signalHeader(sid, key, val);
			break;
		}
	}
	// Выводим результат
	return 0;
}
/**
 * flush Метод сброса параметров запроса
 */
void awh::client::Http2::flush() noexcept {
	// Сбрасываем флаг принудительной остановки
	this->_active = false;
	// Снимаем флаг принудительной остановки
	this->_stopped = false;
}
/**
 * update Метод обновления параметров запроса для переадресации
 * @param request параметры запроса на удалённый сервер
 * @return        предыдущий идентификатор потока, если произошла переадресация
 */
int32_t awh::client::Http2::update(request_t & request) noexcept {
	// Результат работы функции
	int32_t result = -1;
	// Если выполняется редирект
	if(!this->_workers.empty() && (this->_attempt > 0)){
		// Выполняем перебор всех доступных воркеров
		for(auto it = this->_workers.begin(); it != this->_workers.end(); ++it){
			// Если мы нашли нужный нам воркер
			if(it->second->update){
				// Выполняем поиск параметров запроса
				auto jt = this->_requests.find(it->first);
				// Если необходимые нам параметры запроса найдены
				if(jt != this->_requests.end()){
					// Устанавливаем текущий идентификатор
					result = jt->first;
					// Если объект запроса не является одним и тем же
					if(reinterpret_cast <request_t *> (&request) != jt->second.get()){
						// Выполняем установку адреса URL-запроса
						request.url = jt->second->url;
						// Выполняем копирование метода запроса
						request.method = jt->second->method;
						// Если список заголовков получен
						if(!jt->second->headers.empty())
							// Выполняем установку заголовков запроса
							request.headers = jt->second->headers;
						// Если тело запроса существует
						if(!jt->second->entity.empty())
							// Устанавливаем тело запроса
							this->_http.body(jt->second->entity);
					}
					// Выходим из цикла
					break;
				}
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * prepare Метод выполнения препарирования полученных данных
 * @param sid  идентификатор запроса
 * @param aid  идентификатор адъютанта
 * @param core объект сетевого ядра
 * @return     результат препарирования
 */
awh::client::Web::status_t awh::client::Http2::prepare(const int32_t sid, const size_t aid, client::core_t * core) noexcept {
	// Выполняем поиск текущего воркера
	auto it = this->_workers.find(sid);
	// Если искомый воркер найден
	if(it != this->_workers.end()){
		// Получаем параметры запроса
		const auto & response = it->second->http.response();
		// Получаем статус ответа
		awh::http_t::stath_t status = it->second->http.getAuth();
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
					// Выполняем поиск параметров запроса
					auto jt = this->_requests.find(sid);
					// Если параметры запроса получены
					if((it->second->update = (jt != this->_requests.end()))){
						// Получаем новый адрес запроса
						const uri_t::url_t & url = it->second->http.getUrl();
						// Если адрес запроса получен
						if(!url.empty()){
							// Выполняем проверку соответствие протоколов
							const bool schema = (this->_fmk->compare(url.schema, jt->second->url.schema));
							// Если соединение является постоянным
							if(schema && it->second->http.isAlive()){
								// Увеличиваем количество попыток
								this->_attempt++;
								// Устанавливаем новый адрес запроса
								this->_uri.combine(jt->second->url, url);
								// Отправляем повторный запрос
								this->send(it->second->agent, * jt->second.get());
								// Завершаем работу
								return status_t::SKIP;
							}
							// Если нам необходимо отключиться
							core->close(aid);
							// Завершаем работу
							return status_t::SKIP;
						// Если URL-адрес запроса не получен
						} else {
							// Если соединение является постоянным
							if(it->second->http.isAlive()){
								// Увеличиваем количество попыток
								this->_attempt++;
								// Отправляем повторный запрос
								this->send(it->second->agent, * jt->second.get());
								// Завершаем работу
								return status_t::SKIP;
							}
							// Если нам необходимо отключиться
							core->close(aid);
							// Завершаем работу
							return status_t::SKIP;
						}
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
					it->second->callback.set <void (const u_int, const string, const vector <char>)> ("entity", this->_callback.get <void (const u_int, const string, const vector <char>)> ("entity"), response.code, response.message, it->second->http.body());
				// Устанавливаем размер стопбайт
				if(!it->second->http.isAlive()){
					// Завершаем работу
					core->close(aid);
					// Выполняем удаление параметров запроса
					this->_requests.erase(sid);
					// Выполняем завершение работы
					return status_t::STOP;
				}
				// Выполняем удаление параметров запроса
				this->_requests.erase(sid);
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
						it->second->callback.set <void (const u_int, const string, const vector <char>)> ("entity", this->_callback.get <void (const u_int, const string, const vector <char>)> ("entity"), response.code, response.message, it->second->http.body());
					// Выполняем удаление параметров запроса
					this->_requests.erase(sid);
					// Завершаем обработку
					return status_t::NEXT;
				}
			} break;
		}
		// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
		if(this->_callback.is("entity"))
			// Устанавливаем полученную функцию обратного вызова
			it->second->callback.set <void (const u_int, const string, const vector <char>)> ("entity", this->_callback.get <void (const u_int, const string, const vector <char>)> ("entity"), response.code, response.message, it->second->http.body());
		// Завершаем работу
		core->close(aid);
		// Выполняем удаление параметров запроса
		this->_requests.erase(sid);
	}
	// Выполняем завершение работы
	return status_t::STOP;
}
/**
 * stream Метод вывода статус потока
 * @param sid  идентификатор потока
 * @param mode активный статус потока
 */
void awh::client::Http2::stream(const int32_t sid, const mode_t mode) noexcept {
	// Если произошло закрытие потока
	if(mode == mode_t::CLOSE){
		// Выполняем поиск идентификатора воркера
		auto it = this->_workers.find(sid);
		// Если необходимый нам воркер найден
		if(it != this->_workers.end()){
			// Если редирект не выполняется в данный момент
			if(!it->second->update){
				// Выполняем удаление указанного воркера
				this->_workers.erase(it);
				// Выполняем удаление параметра запроса
				this->_requests.erase(sid);
			}
		}
	}
	// Если функция обратного вызова активности потока установлена
	if(this->_callback.is("stream"))
		// Выводим функцию обратного вызова
		this->_callback.call <const int32_t, const mode_t> ("stream", sid, mode);
}
/**
 * sendError Метод отправки сообщения об ошибке
 * @param mess отправляемое сообщение об ошибке
 */
void awh::client::Http2::sendError(const ws::mess_t & mess) noexcept {
	// Если список воркеров активен
	if(!this->_workers.empty()){
		// Выполняем перебор всего списка воркеров
		for(auto & worker : this->_workers){
			// Если найден воркер WebSocket-клиента
			if(worker.second->agent == agent_t::WEBSOCKET){
				// Выполняем отправку сообщения ошибки на WebSocket-сервер
				this->_ws2.sendError(mess);
				// Выходим из цикла
				break;
			}
		}
	}
}
/**
 * send Метод отправки сообщения на сервер
 * @param agent   агент воркера
 * @param request параметры запроса на удалённый сервер
 * @return        идентификатор отправленного запроса
 */
int32_t awh::client::Http2::send(const agent_t agent, const request_t & request) noexcept {
	// Результат работы функции
	int32_t result = -1;
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::READ, event_t::CONNECT}, event_t::SEND)){
		// Если подключение выполнено
		if(this->_aid > 0){
			// Идентификатор предыдущего потока
			int32_t sid = -1;
			// Определяем тип агента
			switch(static_cast <uint8_t> (agent)){
				// Если протоколом агента является HTTP-клиент
				case static_cast <uint8_t> (agent_t::HTTP): {
					// Если активирован режим работы с HTTP/2 протоколом
					if(this->_upgraded){
						// Выполняем сброс состояния HTTP парсера
						this->_http.reset();
						// Выполняем очистку параметров HTTP запроса
						this->_http.clear();
						// Устанавливаем метод компрессии
						this->_http.compress(this->_compress);
						{
							// Список заголовков для запроса
							vector <nghttp2_nv> nva;
							// Выполняем обновление полученных данных, с целью выполнения редиректа если требуется
							sid = this->update(* const_cast <request_t *> (&request));
							// Если список заголовков получен
							if(!request.headers.empty())
								// Устанавливаем заголовоки запроса
								this->_http.headers(request.headers);
							// Если тело запроса существует
							if(!request.entity.empty())
								// Устанавливаем тело запроса
								this->_http.body(request.entity);
							// Создаём объек запроса
							awh::web_t::req_t query(2.0f, request.method, request.url);
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								// Выводим заголовок запроса
								cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
								// Получаем бинарные данные WEB запроса
								const auto & buffer = this->_http.process(http_t::process_t::REQUEST, query);
								// Выводим параметры запроса
								cout << string(buffer.begin(), buffer.end()) << endl;
							#endif
							// Выполняем запрос на получение заголовков
							const auto & headers = this->_http.process2(http_t::process_t::REQUEST, std::move(query));
							// Выполняем перебор всех заголовков HTTP/2 запроса
							for(auto & header : headers){
								// Выполняем добавление метода запроса
								nva.push_back({
									(uint8_t *) header.first.c_str(),
									(uint8_t *) header.second.c_str(),
									header.first.size(),
									header.second.size(),
									NGHTTP2_NV_FLAG_NONE
								});
							}
							// Получаем объект биндинга ядра TCP/IP
							client::core_t * core = const_cast <client::core_t *> (this->_core);
							// Если тело запроса существует
							if(!request.entity.empty()){
								// Список файловых дескрипторов
								int fds[2];
								// Тело WEB сообщения
								vector <char> entity;
								/**
								 * Методы только для OS Windows
								 */
								#if defined(_WIN32) || defined(_WIN64)
									// Выполняем инициализацию файловых дескрипторов для обмена сообщениями
									const int rv = _pipe(fds, 4096, O_BINARY);
								/**
								 * Для всех остальных операционных систем
								 */
								#else
									// Выполняем инициализацию файловых дескрипторов для обмена сообщениями
									const int rv = ::pipe(fds);
								#endif
								// Выполняем подписку на основной канал передачи данных
								if(rv != 0){
									// Выводим в лог сообщение
									this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
									// Выполняем закрытие подключения
									core->close(this->_aid);
									// Выходим из функции
									return result;
								}
								// Получаем данные тела запроса
								while(!(entity = this->_http.payload()).empty()){
									/**
									 * Если включён режим отладки
									 */
									#if defined(DEBUG_MODE)
										// Выводим сообщение о выводе чанка тела
										cout << this->_fmk->format("<chunk %u>", entity.size()) << endl << endl;
									#endif
									/**
									 * Методы только для OS Windows
									 */
									#if defined(_WIN32) || defined(_WIN64)
										// Если данные небыли записаны в сокет
										if(static_cast <int> (_write(fds[1], entity.data(), entity.size())) != static_cast <int> (entity.size())){
											// Выполняем закрытие сокета для чтения
											_close(fds[0]);
											// Выполняем закрытие сокета для записи
											_close(fds[1]);
											// Выводим в лог сообщение
											this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
											// Выполняем закрытие подключения
											core->close(this->_aid);
											// Выходим из функции
											return result;
										}
									/**
									 * Для всех остальных операционных систем
									 */
									#else
										// Если данные небыли записаны в сокет
										if(static_cast <int> (::write(fds[1], entity.data(), entity.size())) != static_cast <int> (entity.size())){
											// Выполняем закрытие сокета для чтения
											::close(fds[0]);
											// Выполняем закрытие сокета для записи
											::close(fds[1]);
											// Выводим в лог сообщение
											this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
											// Выполняем закрытие подключения
											core->close(this->_aid);
											// Выходим из функции
											return result;
										}
									#endif
								}
								/**
								 * Методы только для OS Windows
								 */
								#if defined(_WIN32) || defined(_WIN64)
									// Выполняем закрытие подключения
									_close(fds[1]);
								/**
								 * Для всех остальных операционных систем
								 */
								#else
									// Выполняем закрытие подключения
									::close(fds[1]);
								#endif
								// Создаём объект передачи данных тела полезной нагрузки
								nghttp2_data_provider data;
								// Зануляем передаваемый контекст
								data.source.ptr = nullptr;
								// Устанавливаем файловый дескриптор
								data.source.fd = fds[0];
								// Устанавливаем функцию обратного вызова
								data.read_callback = &http2_t::onRead;
								// Выполняем запрос на удалённый сервер
								result = nghttp2_submit_request(this->_session, nullptr, nva.data(), nva.size(), &data, this);
							// Если тело запроса не существует, выполняем установленный запрос
							} else result = nghttp2_submit_request(this->_session, nullptr, nva.data(), nva.size(), nullptr, this);
							// Если запрос не получилось отправить
							if(result < 0){
								// Выводим в лог сообщение
								this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(result));
								// Выполняем закрытие подключения
								core->close(this->_aid);
								// Выходим из функции
								return result;
							}{
								// Если функция обратного вызова на вывод редиректа потоков установлена
								if((sid > -1) && this->_callback.is("redirect"))
									// Выводим функцию обратного вызова
									this->_callback.call <const int32_t, const int32_t> ("redirect", sid, result);
							}{
								// Результат фиксации сессии
								int rv = -1;
								// Фиксируем отправленный результат
								if((rv = nghttp2_session_send(this->_session)) != 0){
									// Выводим сообщение об полученной ошибке
									this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
									// Выходим из функции
									return -1;
								}
							}
						}
					// Если активирован режим работы с HTTP/1.1 протоколом
					} else {
						// Выполняем обновление полученных данных, с целью выполнения редиректа если требуется
						sid = this->update(* const_cast <request_t *> (&request));
						// Выполняем отправку на сервер запроса
						result = this->_http1.send(request);
					}
				} break;
				// Если протоколом агента является WebSocket-клиент
				case static_cast <uint8_t> (agent_t::WEBSOCKET): {
					// Выполняем обновление полученных данных, с целью выполнения редиректа если требуется
					sid = this->update(* const_cast <request_t *> (&request));
					// Выполняем установку подключения с WebSocket-сервером
					this->_ws2.connectCallback(this->_aid, this->_scheme.sid, dynamic_cast <awh::core_t *> (const_cast <client::core_t *> (this->_core)));
					// Выводим идентификатор подключения
					result = this->_ws2._sid;
				} break;
			}
			// Если идентификатор подключения получен
			if(result > 0){
				{
					// Добавляем полученный запрос в список запросов
					auto ret = this->_requests.emplace(result, unique_ptr <request_t> (new request_t));
					// Выполняем копирование URL-адреса
					ret.first->second->url = request.url;
					// Выполняем копирование метода запроса
					ret.first->second->method = request.method;
					// Если заголовки запроса переданы
					if(!request.headers.empty())
						// Выполняем копирование заголовков запроса
						ret.first->second->headers = request.headers;
					// Если тело запроса передано
					if(!request.entity.empty())
						// Выполняем копирование тела запроса
						ret.first->second->entity.assign(request.entity.begin(), request.entity.end());
				}{
					// Выполняем установку объекта воркера
					auto ret = this->_workers.emplace(result, unique_ptr <worker_t> (new worker_t(this->_fmk, this->_log)));
					// Выполняем установку типа агента
					ret.first->second->agent = agent;
					// Выполняем сброс состояния HTTP парсера
					ret.first->second->http.reset();
					// Выполняем очистку параметров HTTP запроса
					ret.first->second->http.clear();
					// Устанавливаем метод компрессии
					ret.first->second->http.compress(this->_compress);
					// Если активирован режим работы с HTTP/2 протоколом
					if(this->_upgraded)
						// Выполняем смену активного протокола на HTTP/2
						ret.first->second->proto = engine_t::proto_t::HTTP2;
					// Если размер одного чанка установлен
					if(this->_chunkSize > 0)
						// Устанавливаем размер чанка
						ret.first->second->http.chunk(this->_chunkSize);
					// Если User-Agent установлен
					if(!this->_userAgent.empty())
						// Устанавливаем пользовательского агента
						ret.first->second->http.userAgent(this->_userAgent);
					// Устанавливаем параметры авторизации для HTTP-клиента
					ret.first->second->http.authType(this->_authType, this->_authHash);
					// Если логин пользователя и пароль установлены
					if(!this->_login.empty() && !this->_password.empty())
						// Устанавливаем логин и пароль пользователя
						ret.first->second->http.user(this->_login, this->_password);
					// Если параметры сервиса установлены
					if(!this->_serv.id.empty() && !this->_serv.name.empty() && !this->_serv.ver.empty())
						// Устанавливаем данные сервиса
						ret.first->second->http.serv(this->_serv.id, this->_serv.name, this->_serv.ver);
					// Если пароль для шифрования передан
					if(!this->_crypto.pass.empty())
						// Устанавливаем параметры шифрования для HTTP-клиента
						ret.first->second->http.crypto(this->_crypto.pass, this->_crypto.salt, this->_crypto.cipher);
				}
			}
			// Если идентификатор устаревшего запроса найден
			if(!this->_upgraded && (result != sid) && (sid > 0)){
				// Выполняем удаление указанного воркера
				this->_workers.erase(sid);
				// Выполняем удаление параметра запроса
				this->_requests.erase(sid);
			}
		}
	}
	// Сообщаем что идентификатор не получен
	return result;
}
/**
 * send Метод отправки сообщения на сервер
 * @param message буфер сообщения в бинарном виде
 * @param size    размер сообщения в байтах
 * @param utf8    данные передаются в текстовом виде
 */
void awh::client::Http2::send(const char * message, const size_t size, const bool utf8) noexcept {
	// Если список воркеров активен
	if(!this->_workers.empty()){
		// Выполняем перебор всего списка воркеров
		for(auto & worker : this->_workers){
			// Если найден воркер WebSocket-клиента
			if(worker.second->agent == agent_t::WEBSOCKET){
				// Выполняем отправку сообщения на WebSocket-сервер
				this->_ws2.send(message, size, utf8);
				// Выходим из цикла
				break;
			}
		}
	}
}
/**
 * pause Метод установки на паузу клиента
 */
void awh::client::Http2::pause() noexcept {
	// Если список воркеров активен
	if(!this->_workers.empty()){
		// Выполняем перебор всего списка воркеров
		for(auto & worker : this->_workers){
			// Если найден воркер WebSocket-клиента
			if(worker.second->agent == agent_t::WEBSOCKET){
				// Выполняем постановку на паузу
				this->_ws2.pause();
				// Выходим из цикла
				break;
			}
		}
	}
}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param callback функция обратного вызова
 */
void awh::client::Http2::on(function <void (const mode_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибок
 * @param callback функция обратного вызова
 */
void awh::client::Http2::on(function <void (const u_int, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для получения входящих ошибок
	this->_callback.set <void (const u_int, const string &)> ("error", callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws2.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param callback функция обратного вызова
 */
void awh::client::Http2::on(function <void (const vector <char> &, const bool)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для получения входящих сообщений
	this->_callback.set <void (const vector <char> &, const bool)> ("message", callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws2.on(callback);
}
/**
 * on Метод установки функции обратного вызова для перехвата полученных чанков
 * @param callback функция обратного вызова
 */
void awh::client::Http2::on(function <void (const vector <char> &, const awh::http_t *)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP/1.1 клиента
	this->_http1.on(callback);
}
/**
 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
 * @param callback функция обратного вызова
 */
void awh::client::Http2::on(function <void (const awh::core_t::status_t, awh::core_t *)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
}
/**
 * on Метод установки функция обратного вызова активности потока
 * @param callback функция обратного вызова
 */
void awh::client::Http2::on(function <void (const int32_t, const mode_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
}
/**
 * on Метод выполнения редиректа с одного потока на другой (необходим для совместимости с HTTP/2)
 * @param callback функция обратного вызова
 */
void awh::client::Http2::on(function <void (const int32_t, const int32_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP/1.1 клиента
	this->_http1.on(callback);
}
/**
 * on Метод установки функции вывода полученного чанка бинарных данных с сервера
 * @param callback функция обратного вызова
 */
void awh::client::Http2::on(function <void (const int32_t, const vector <char> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP/1.1 клиента
	this->_http1.on(callback);
}
/**
 * on Метод установки функции вывода ответа сервера на ранее выполненный запрос
 * @param callback функция обратного вызова
 */
void awh::client::Http2::on(function <void (const int32_t, const u_int, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP/1.1 клиента
	this->_http1.on(callback);
}
/**
 * on Метод установки функции вывода полученного заголовка с сервера
 * @param callback функция обратного вызова
 */
void awh::client::Http2::on(function <void (const int32_t, const string &, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP/1.1 клиента
	this->_http1.on(callback);
}
/**
 * on Метод установки функции вывода полученного тела данных с сервера
 * @param callback функция обратного вызова
 */
void awh::client::Http2::on(function <void (const int32_t, const u_int, const string &, const vector <char> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP/1.1 клиента
	this->_http1.on(callback);
}
/**
 * on Метод установки функции вывода полученных заголовков с сервера
 * @param callback функция обратного вызова
 */
void awh::client::Http2::on(function <void (const int32_t, const u_int, const string &, const unordered_multimap <string, string> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP/1.1 клиента
	this->_http1.on(callback);
}
/**
 * sub Метод получения выбранного сабпротокола
 * @return выбранный сабпротокол
 */
const string & awh::client::Http2::sub() const noexcept {
	// Выводим список сабпротоколов принадлежащих WebSocket-клиенту
	return this->_ws2.sub();
}
/**
 * sub Метод установки сабпротокола поддерживаемого сервером
 * @param sub сабпротокол для установки
 */
void awh::client::Http2::sub(const string & sub) noexcept {
	// Если сабпротокол передан
	if(!sub.empty())
		// Устанавливаем сабпротокол
		this->_ws2.sub(sub);
}
/**
 * subs Метод установки списка сабпротоколов поддерживаемых сервером
 * @param subs сабпротоколы для установки
 */
void awh::client::Http2::subs(const vector <string> & subs) noexcept {
	// Если список сабпротоколов получен
	if(!subs.empty())
		// Устанавливаем список сабсабпротоколов для WebSocket-клиента
		this->_ws2.subs(subs);
}
/**
 * extensions Метод извлечения списка расширений
 * @return список поддерживаемых расширений
 */
const vector <vector <string>> & awh::client::Http2::extensions() const noexcept {
	// Выводим список расширений принадлежащих WebSocket-клиенту
	return this->_ws2.extensions();
}
/**
 * extensions Метод установки списка расширений
 * @param extensions список поддерживаемых расширений
 */
void awh::client::Http2::extensions(const vector <vector <string>> & extensions) noexcept {
	// Выполняем установку списка доступных расширений для WebSocket-клиента
	this->_ws2.extensions(extensions);
}
/**
 * chunk Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::client::Http2::chunk(const size_t size) noexcept {
	// Устанавливаем размер чанка
	web2_t::chunk(size);
	// Устанавливаем размер чанка для WebSocket-клиента
	this->_ws2.chunk(size);
	// Устанавливаем размер чанка для HTTP-парсера
	this->_http.chunk(size);
	// Устанавливаем размер чанка для HTTP-клиента
	this->_http1.chunk(size);
}
/**
 * segmentSize Метод установки размеров сегментов фрейма
 * @param size минимальный размер сегмента
 */
void awh::client::Http2::segmentSize(const size_t size) noexcept {
	// Если размер передан, устанавливаем
	if(size > 0){
		// Устанавливаем размер сегментов фрейма
		this->_frameSize = size;
		// Устанавливаем размер сегментов фрейма для WebSocket-клиента
		this->_ws2.segmentSize(size);
	}
}
/**
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::client::Http2::mode(const set <flag_t> & flags) noexcept {
	// Устанавливаем флаги настроек модуля для WebSocket-клиента
	this->_ws2.mode(flags);
	// Устанавливаем флаги настроек модуля для HTTP/2 клиента
	this->_http1.mode(flags);
	// Выполняем установку флагов настроек модуля
	web2_t::mode(flags);
	// Если протокол подключения желательно установить HTTP/2
	if(this->_core->proto() == engine_t::proto_t::HTTP2){
		// Флаг активации работы персистентного вызова
		bool enable = this->_scheme.alive;
		// Если необходимо выполнить отключение персистентного вызова
		if(!enable && !this->_workers.empty()){
			// Выполняем переход по всему списку воркеров
			for(auto & worker : this->_workers){
				// Если среди списка воркеров найден клиент WebSocket
				if((enable = (worker.second->agent == agent_t::WEBSOCKET)))
					// Выходим из цикла
					break;
			}
		}
		// Активируем персистентный запуск для работы пингов
		const_cast <client::core_t *> (this->_core)->persistEnable(enable);
	}
}
/**
 * core Метод установки сетевого ядра
 * @param core объект сетевого ядра
 */
void awh::client::Http2::core(const client::core_t * core) noexcept {
	// Если объект сетевого ядра передан
	if(core != nullptr){
		// Выполняем установку объекта сетевого ядра
		this->_core = core;
		// Добавляем схемы сети в сетевое ядро
		const_cast <client::core_t *> (this->_core)->add(&this->_scheme);
		// Если протокол подключения желательно установить HTTP/2
		if(this->_core->proto() == engine_t::proto_t::HTTP2){
			// Активируем персистентный запуск для работы пингов
			const_cast <client::core_t *> (this->_core)->persistEnable(true);
			// Активируем асинхронный режим работы
			const_cast <client::core_t *> (this->_core)->mode(client::core_t::mode_t::ASYNC);
		}
		// Устанавливаем функцию активации ядра клиента
		const_cast <client::core_t *> (this->_core)->callback(std::bind(&http2_t::eventsCallback, this, _1, _2));
		// Если многопоточность активированна
		if(this->_threads > 0)
			// Устанавливаем простое чтение базы событий
			const_cast <client::core_t *> (this->_core)->easily(true);
	// Если объект сетевого ядра не передан но ранее оно было добавлено
	} else if(this->_core != nullptr) {
		// Если многопоточность активированна
		if(this->_threads <= 0)
			// Снимаем режим простого чтения базы событий
			const_cast <client::core_t *> (this->_core)->easily(false);
		// Отключаем функцию активации ядра клиента
		const_cast <client::core_t *> (this->_core)->callback(nullptr);
		// Если протокол подключения желательно установить HTTP/2
		if(this->_core->proto() == engine_t::proto_t::HTTP2){
			// Деактивируем персистентный запуск для работы пингов
			const_cast <client::core_t *> (this->_core)->persistEnable(false);
			// Активируем асинхронный режим работы
			const_cast <client::core_t *> (this->_core)->mode(client::core_t::mode_t::SYNC);
		}
		// Удаляем схему сети из сетевого ядра
		const_cast <client::core_t *> (this->_core)->remove(this->_scheme.sid);
		// Выполняем установку объекта сетевого ядра
		this->_core = core;
	}
}
/**
 * user Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::client::Http2::user(const string & login, const string & password) noexcept {
	// Устанавливаем логин и пароль пользователя для WebSocket-клиента
	this->_ws2.user(login, password);
	// Устанавливаем логин и пароль пользователя для HTTP-арсера
	this->_http.user(login, password);
	// Устанавливаем логин и пароль пользователя для HTTP-клиента
	this->_http1.user(login, password);
}
/**
 * userAgent Метод установки User-Agent для HTTP запроса
 * @param userAgent агент пользователя для HTTP запроса
 */
void awh::client::Http2::userAgent(const string & userAgent) noexcept {
	// Устанавливаем UserAgent
	if(!userAgent.empty()){
		// Устанавливаем пользовательского агента у родительского класса
		web2_t::userAgent(userAgent);
		// Устанавливаем пользовательского агента для WebSocket-клиента
		this->_ws2.userAgent(userAgent);
		// Устанавливаем пользовательского агента для HTTP-парсера
		this->_http.userAgent(userAgent);
		// Устанавливаем пользовательского агента для HTTP-клиента
		this->_http1.userAgent(userAgent);
	}
}
/**
 * serv Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::client::Http2::serv(const string & id, const string & name, const string & ver) noexcept {
	// Если данные сервиса переданы
	if(!id.empty() && !name.empty() && !ver.empty()){
		// Выполняем установку данных сервиса у родительского класса
		web2_t::serv(id, name, ver);
		// Устанавливаем данные сервиса для WebSocket-клиента
		this->_ws2.serv(id, name, ver);
		// Устанавливаем данные сервиса для HTTP-парсера
		this->_http.serv(id, name, ver);
		// Устанавливаем данные сервиса для HTTP-клиента
		this->_http1.serv(id, name, ver);
	}
}
/**
 * multiThreads Метод активации многопоточности
 * @param threads количество потоков для активации
 * @param mode    флаг активации/деактивации мультипоточности
 */
void awh::client::Http2::multiThreads(const size_t threads, const bool mode) noexcept {
	// Если необходимо активировать мультипоточность
	if(mode)
		// Выполняем установку количества ядер мультипоточности
		this->_threads = static_cast <ssize_t> (threads);
	// Если необходимо отключить мультипоточность
	else this->_threads = -1;
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest-авторизации
 */
void awh::client::Http2::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Устанавливаем параметры авторизации для WebSocket-клиента
	this->_ws2.authType(type, hash);
	// Устанавливаем параметры авторизации для HTTP-парсера
	this->_http.authType(type, hash);
	// Устанавливаем параметры авторизации для HTTP-клиента
	this->_http1.authType(type, hash);
}
/**
 * crypto Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::client::Http2::crypto(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Устанавливаем параметры шифрования для WebSocket-клиента
	this->_ws2.crypto(pass, salt, cipher);
	// Устанавливаем параметры шифрования для HTTP-парсера
	this->_http.crypto(pass, salt, cipher);
	// Устанавливаем параметры шифрования для HTTP-клиента
	this->_http1.crypto(pass, salt, cipher);
}
/**
 * Http2 Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::client::Http2::Http2(const fmk_t * fmk, const log_t * log) noexcept :
 web2_t(fmk, log), _ws2(fmk, log), _http1(fmk, log), _http(fmk, log), _threads(-1), _frameSize(0) {
	// Устанавливаем функцию персистентного вызова
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("persist", std::bind(&http2_t::persistCallback, this, _1, _2, _3));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const size_t, const size_t, awh::core_t *)> ("write", std::bind(&http2_t::writeCallback, this, _1, _2, _3, _4, _5));
}
/**
 * Http2 Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::client::Http2::Http2(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 web2_t(core, fmk, log), _ws2(fmk, log), _http1(fmk, log), _http(fmk, log), _threads(-1), _frameSize(0) {
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws2.on((function <void (const int32_t, const mode_t)>) std::bind(&http2_t::stream, this, _1, _2));
	// Выполняем установку функции обратного вызова для HTTP/1.1 клиента
	this->_http1.on((function <void (const int32_t, const mode_t)>) std::bind(&http2_t::stream, this, _1, _2));
	// Устанавливаем функцию персистентного вызова
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("persist", std::bind(&http2_t::persistCallback, this, _1, _2, _3));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const size_t, const size_t, awh::core_t *)> ("write", std::bind(&http2_t::writeCallback, this, _1, _2, _3, _4, _5));
	// Если протокол подключения желательно установить HTTP/2
	if(this->_core->proto() == engine_t::proto_t::HTTP2){
		// Активируем персистентный запуск для работы пингов
		const_cast <client::core_t *> (this->_core)->persistEnable(true);
		// Активируем асинхронный режим работы
		const_cast <client::core_t *> (this->_core)->mode(client::core_t::mode_t::ASYNC);
	}
}