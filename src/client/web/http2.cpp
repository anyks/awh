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
void awh::client::Http2::connectCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::OPEN, event_t::READ, event_t::PROXY_READ}, event_t::CONNECT)){
		// Запоминаем идентификатор адъютанта
		this->_aid = aid;
		// Выполняем установку идентификатора объекта
		this->_http.id(aid);
		// Выполняем инициализацию сессии HTTP/2
		web2_t::connectCallback(aid, sid, core);
		// Если флаг инициализации сессии HTTP2 не установлен
		if(!this->_sessionInitialized){
			// Запоминаем идентификатор адъютанта
			this->_http1._aid = this->_aid;
			// Выполняем установку идентификатора объекта
			this->_http1._http.id(this->_aid);
			// Выполняем установку сетевого ядра
			this->_http1._core = this->_core;
			// Выполняем установку данных URL-адреса
			this->_http1._scheme.url = this->_scheme.url;
		}
		// Выполняем установку идентификатора объекта
		this->_ws2._http.id(aid);
		// Выполняем установку сетевого ядра
		this->_ws2._core = this->_core;
		// Выполняем установку данных URL-адреса
		this->_ws2._scheme.url = this->_scheme.url;
		// Выполняем установку сессии HTTP2
		this->_ws2._nghttp2.session = this->_nghttp2.session;
		// Устанавливаем флаг инициализации сессии HTTP2
		this->_ws2._sessionInitialized = this->_sessionInitialized;
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
void awh::client::Http2::disconnectCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Выполняем удаление подключения
	this->_nghttp2.close();
	// Выполняем удаление сессии подключения HTTP/2
	this->_nghttp2.free();
	// Выполняем снятие флага инициализации сессии HTTP/2
	this->_sessionInitialized = false;
	// Выполняем редирект, если редирект выполнен
	if(this->redirect(aid, sid, core))
		// Выходим из функции
		return;
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
void awh::client::Http2::readCallback(const char * buffer, const size_t size, const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (sid > 0)){
		// Если протокол подключения является HTTP/2
		if(core->proto(aid) == engine_t::proto_t::HTTP2){
			// Если флаг инициализации сессии HTTP2 установлен
			if(this->_sessionInitialized){
				// Если сессия HTTP/2 инициализированна
				if(this->_nghttp2.session != nullptr){
					// Выполняем извлечение полученного чанка данных из сокета
					ssize_t bytes = nghttp2_session_mem_recv(this->_nghttp2.session, (const uint8_t *) buffer, size);
					// Если данные не прочитаны, выводим ошибку и выходим
					if(bytes < 0){
						// Выводим сообщение об полученной ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(static_cast <int> (bytes)));
						// Если функция обратного вызова на на вывод ошибок установлена
						if(this->_callback.is("error"))
							// Выводим функцию обратного вызова
							this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_RECV, nghttp2_strerror(static_cast <int> (bytes)));
						// Выходим из функции
						return;
					}
					// Если сессия HTTP/2 инициализированна
					if(this->_nghttp2.session != nullptr){
						// Фиксируем полученный результат
						if((bytes = nghttp2_session_send(this->_nghttp2.session)) != 0){
							// Выводим сообщение об полученной ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(static_cast <int> (bytes)));
							// Если функция обратного вызова на на вывод ошибок установлена
							if(this->_callback.is("error"))
								// Выводим функцию обратного вызова
								this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_SEND, nghttp2_strerror(static_cast <int> (bytes)));
							// Выходим из функции
							return;
						}
					}
				}
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
void awh::client::Http2::writeCallback(const char * buffer, const size_t size, const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Выполняем перебор всех доступных воркеров
		for(auto & worker : this->_workers){
			// Если агент является клиентом WebSocket
			if(worker.second->agent == agent_t::WEBSOCKET){
				// Выполняем переброс вызова записи на клиент WebSocket
				this->_ws2.writeCallback(buffer, size, aid, sid, core);
				// Выполняем выход из цикла
				break;
			}
		}
	}
}
/**
 * persistCallback Функция персистентного вызова
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::Http2::persistCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Выполняем перебор всех доступных воркеров
		for(auto & worker : this->_workers){
			// Определяем протокол клиента
			switch(static_cast <uint8_t> (worker.second->agent)){
				// Если агент является клиентом HTTP
				case static_cast <uint8_t> (agent_t::HTTP): {
					// Если переключение протокола на HTTP/2 выполнено и пинг не прошёл
					if(this->_sessionInitialized && !this->ping())
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
 * chunkSignal Метод обратного вызова при получении чанка с сервера HTTP/2
 * @param sid    идентификатор потока
 * @param buffer буфер данных который содержит полученный чанк
 * @param size   размер полученного буфера данных чанка
 * @return       статус полученных данных
 */
int awh::client::Http2::chunkSignal(const int32_t sid, const uint8_t * buffer, const size_t size) noexcept {
	// Если подключение производится через, прокси-сервер
	if(this->_scheme.isProxy())
		// Выполняем обработку полученных данных чанка для прокси-сервера
		return this->chunkProxySignal(sid, buffer, size);
	// Если мы работаем с сервером напрямую
	else {
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
					case static_cast <uint8_t> (agent_t::HTTP): {
						// Добавляем полученный чанк в тело данных
						it->second->http.body(vector <char> (buffer, buffer + size));
						// Если функция обратного вызова на вывода полученного чанка бинарных данных с сервера установлена
						if(this->_callback.is("chunks"))
							// Выводим функцию обратного вызова
							this->_callback.call <const int32_t, const vector <char> &> ("chunks", sid, vector <char> (buffer, buffer + size));
					} break;
					// Если агент является клиентом WebSocket
					case static_cast <uint8_t> (agent_t::WEBSOCKET):
						// Выполняем передачу полученных данных на WebSocket-клиент
						this->_ws2.chunkSignal(sid, buffer, size);
					break;
				}
			}
		}
	}
	// Выводим результат
	return 0;
}
/**
 * frameSignal Метод обратного вызова при получении фрейма заголовков сервера HTTP/2
 * @param sid    идентификатор потока
 * @param direct направление передачи фрейма
 * @param type   тип полученного фрейма
 * @param flags  флаг полученного фрейма
 * @return       статус полученных данных
 */
int awh::client::Http2::frameSignal(const int32_t sid, const nghttp2_t::direct_t direct, const uint8_t type, const uint8_t flags) noexcept {
	// Определяем направление передачи фрейма
	switch(static_cast <uint8_t> (direct)){
		// Если производится передача фрейма на сервер
		case static_cast <uint8_t> (nghttp2_t::direct_t::SEND): {
			// Если мы получили флаг завершения потока
			if(flags & NGHTTP2_FLAG_END_STREAM){
				// Выполняем поиск идентификатора воркера
				auto it = this->_workers.find(sid);
				// Если необходимый нам воркер найден
				if(it != this->_workers.end()){
					// Если агент является клиентом WebSocket
					if(it->second->agent == agent_t::WEBSOCKET)
						// Выполняем передачу на WebSocket-клиент
						this->_ws2.frameSignal(sid, direct, type, flags);
				}
				// Если установлена функция отлова завершения запроса
				if(this->_callback.is("end"))
					// Выводим функцию обратного вызова
					this->_callback.call <const int32_t, const direct_t> ("end", sid, direct_t::SEND);
			}
		} break;
		// Если производится получения фрейма с сервера
		case static_cast <uint8_t> (nghttp2_t::direct_t::RECV): {
			// Если подключение производится через, прокси-сервер
			if(this->_scheme.isProxy())
				// Выполняем обработку полученных данных фрейма для прокси-сервера
				return this->frameProxySignal(sid, direct, type, flags);
			// Если мы работаем с сервером напрямую
			else {
				// Выполняем поиск идентификатора воркера
				auto it = this->_workers.find(sid);
				// Если необходимый нам воркер найден
				if(it != this->_workers.end()){
					// Выполняем определение типа фрейма
					switch(type){
						// Если мы получили входящие данные тела ответа
						case NGHTTP2_DATA: {
							// Определяем протокол клиента
							switch(static_cast <uint8_t> (it->second->agent)){
								// Если агент является клиентом HTTP
								case static_cast <uint8_t> (agent_t::HTTP): {
									// Если мы получили флаг завершения потока
									if(flags & NGHTTP2_FLAG_END_STREAM){
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
										switch(static_cast <uint8_t> (this->prepare(sid, this->_aid, const_cast <client::core_t *> (this->_core)))){
											// Если необходимо выполнить пропуск обработки данных
											case static_cast <uint8_t> (status_t::SKIP):
												// Завершаем работу
												return 0;
										}
										// Если функция обратного вызова установлена, выводим сообщение
										if(it->second->callback.is("entity"))
											// Выполняем функцию обратного вызова дисконнекта
											it->second->callback.bind <const int32_t, const u_int, const string, const vector <char>> ("entity");
										// Если функция обратного вызова на получение удачного ответа установлена
										if(this->_callback.is("handshake"))
											// Выполняем функцию обратного вызова
											this->_callback.call <const int32_t, const agent_t> ("handshake", sid, it->second->agent);
										// Выполняем удаление выполненного воркера
										this->_workers.erase(sid);
										// Если установлена функция отлова завершения запроса
										if(this->_callback.is("end"))
											// Выводим функцию обратного вызова
											this->_callback.call <const int32_t, const direct_t> ("end", sid, direct_t::RECV);
										// Завершаем работу
										return 0;
									}
								} break;
								// Если агент является клиентом WebSocket
								case static_cast <uint8_t> (agent_t::WEBSOCKET):
									// Выполняем передачу на WebSocket-клиент
									return this->_ws2.frameSignal(sid, direct, type, flags);
							}
						} break;
						// Если мы получили входящие данные заголовков ответа
						case NGHTTP2_HEADERS: {
							// Определяем протокол клиента
							switch(static_cast <uint8_t> (it->second->agent)){
								// Если агент является клиентом HTTP
								case static_cast <uint8_t> (agent_t::HTTP): {
									// Если сессия клиента совпадает с сессией полученных даных и передача заголовков завершена
									if(flags & NGHTTP2_FLAG_END_HEADERS){
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
											this->_callback.call <const int32_t, const u_int, const string &> ("response", sid, response.code, response.message);
										// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
										if(this->_callback.is("headers"))
											// Выводим функцию обратного вызова
											this->_callback.call <const int32_t, const u_int, const string &, const unordered_multimap <string, string> &> ("headers", sid, response.code, response.message, it->second->http.headers());
									}
									// Если мы получили флаг завершения потока
									if(flags & NGHTTP2_FLAG_END_STREAM){
										// Если установлена функция отлова завершения запроса
										if(this->_callback.is("end"))
											// Выводим функцию обратного вызова
											this->_callback.call <const int32_t, const direct_t> ("end", sid, direct_t::RECV);
									}
									// Завершаем работу
									return 0;
								} break;
								// Если агент является клиентом WebSocket
								case static_cast <uint8_t> (agent_t::WEBSOCKET):
									// Выполняем передачу на WebSocket-клиент
									return this->_ws2.frameSignal(sid, direct, type, flags);
							}
						} break;
					}
				}
			}
		} break;
	}
	// Выводим результат
	return 0;
}
/**
 * beginSignal Метод начала получения фрейма заголовков HTTP/2 сервера
 * @param sid идентификатор потока
 * @return    статус полученных данных
 */
int awh::client::Http2::beginSignal(const int32_t sid) noexcept {
	// Если подключение производится через, прокси-сервер
	if(this->_scheme.isProxy())
		// Выполняем обработку сигнала начала получения заголовков для прокси-сервера
		return this->beginProxySignal(sid);
	// Если мы работаем с сервером напрямую
	else {
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
					this->_ws2.beginSignal(sid);
				break;
			}
		}
	}
	// Выводим результат
	return 0;
}
/**
 * closedSignal Метод завершения работы потока
 * @param sid   идентификатор потока
 * @param error флаг ошибки HTTP/2 если присутствует
 * @return      статус полученных данных
 */
int awh::client::Http2::closedSignal(const int32_t sid, const uint32_t error) noexcept {
	// Выполняем поиск идентификатора воркера
	auto it = this->_workers.find(sid);
	// Если необходимый нам воркер найден
	if(it != this->_workers.end()){
		// Определяем тип получаемой ошибки
		switch(error){
			// Если получена ошибка протокола
			case 0x1: {
				// Выводим информацию о закрытии сессии с ошибкой
				this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, it->first, "PROTOCOL_ERROR");
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выводим функцию обратного вызова
					this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_PROTOCOL, this->_fmk->format("Stream %d closed with error=%s", it->first, "PROTOCOL_ERROR"));
			} break;
			// Если получена ошибка реализации
			case 0x2: {
				// Выводим информацию о закрытии сессии с ошибкой
				this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, it->first, "INTERNAL_ERROR");
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выводим функцию обратного вызова
					this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_INTERNAL, this->_fmk->format("Stream %d closed with error=%s", it->first, "INTERNAL_ERROR"));
			} break;
			// Если получена ошибка превышения предела управления потоком
			case 0x3: {
				// Выводим информацию о закрытии сессии с ошибкой
				this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, it->first, "FLOW_CONTROL_ERROR");
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выводим функцию обратного вызова
					this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_FLOW_CONTROL, this->_fmk->format("Stream %d closed with error=%s", it->first, "FLOW_CONTROL_ERROR"));
			} break;
			// Если установка не подтверждённа
			case 0x4: {
				// Выводим информацию о закрытии сессии с ошибкой
				this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, it->first, "SETTINGS_TIMEOUT");
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выводим функцию обратного вызова
					this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_SETTINGS_TIMEOUT, this->_fmk->format("Stream %d closed with error=%s", it->first, "SETTINGS_TIMEOUT"));
			} break;
			// Если получен кадр для завершения потока
			case 0x5: {
				// Выводим информацию о закрытии сессии с ошибкой
				this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, it->first, "STREAM_CLOSED");
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выводим функцию обратного вызова
					this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_STREAM_CLOSED, this->_fmk->format("Stream %d closed with error=%s", it->first, "STREAM_CLOSED"));
			} break;
			// Если размер кадра некорректен
			case 0x6: {
				// Выводим информацию о закрытии сессии с ошибкой
				this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, it->first, "FRAME_SIZE_ERROR");
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выводим функцию обратного вызова
					this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_FRAME_SIZE, this->_fmk->format("Stream %d closed with error=%s", it->first, "FRAME_SIZE_ERROR"));
			} break;
			// Если поток не обработан
			case 0x7: {
				// Выводим информацию о закрытии сессии с ошибкой
				this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, it->first, "REFUSED_STREAM");
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выводим функцию обратного вызова
					this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_REFUSED_STREAM, this->_fmk->format("Stream %d closed with error=%s", it->first, "REFUSED_STREAM"));
			} break;
			// Если поток аннулирован
			case 0x8: {
				// Выводим информацию о закрытии сессии с ошибкой
				this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, it->first, "CANCEL");
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выводим функцию обратного вызова
					this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_CANCEL, this->_fmk->format("Stream %d closed with error=%s", it->first, "CANCEL"));
			} break;
			// Если состояние компрессии не обновлено
			case 0x9: {
				// Выводим информацию о закрытии сессии с ошибкой
				this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, it->first, "COMPRESSION_ERROR");
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выводим функцию обратного вызова
					this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_COMPRESSION, this->_fmk->format("Stream %d closed with error=%s", it->first, "COMPRESSION_ERROR"));
			} break;
			// Если получена ошибка TCP-соединения для метода CONNECT
			case 0xA: {
				// Выводим информацию о закрытии сессии с ошибкой
				this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, it->first, "CONNECT_ERROR");
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выводим функцию обратного вызова
					this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_CONNECT, this->_fmk->format("Stream %d closed with error=%s", it->first, "CONNECT_ERROR"));
			} break;
			// Если превышена емкость для обработки
			case 0xB: {
				// Выводим информацию о закрытии сессии с ошибкой
				this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, it->first, "ENHANCE_YOUR_CALM");
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выводим функцию обратного вызова
					this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_ENHANCE_YOUR_CALM, this->_fmk->format("Stream %d closed with error=%s", it->first, "ENHANCE_YOUR_CALM"));
			} break;
			// Если согласованные параметры TLS не приемлемы
			case 0xC: {
				// Выводим информацию о закрытии сессии с ошибкой
				this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, it->first, "INADEQUATE_SECURITY");
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выводим функцию обратного вызова
					this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_INADEQUATE_SECURITY, this->_fmk->format("Stream %d closed with error=%s", it->first, "INADEQUATE_SECURITY"));
			} break;
			// Если для запроса используется HTTP/1.1
			case 0xD: {
				// Выводим информацию о закрытии сессии с ошибкой
				this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, it->first, "HTTP_1_1_REQUIRED");
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выводим функцию обратного вызова
					this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_HTTP_1_1_REQUIRED, this->_fmk->format("Stream %d closed with error=%s", it->first, "HTTP_1_1_REQUIRED"));
			} break;
		}
		// Выполняем удаление указанного воркера
		this->_workers.erase(it);
		// Выполняем удаление параметра запроса
		this->_requests.erase(sid);
		// Если функция обратного вызова активности потока установлена
		if(this->_callback.is("stream"))
			// Выводим функцию обратного вызова
			this->_callback.call <const int32_t, const mode_t> ("stream", sid, mode_t::CLOSE);
		// Если флаг инициализации сессии HTTP2 установлен
		if((error > 0x0) && this->_sessionInitialized){
			// Выполняем снятие флага инициализации сессии HTTP2
			this->_sessionInitialized = !this->_sessionInitialized;
			// Если закрытие подключения не выполнено
			if(!this->_nghttp2.close()){
				// Выполняем отключение от сервера
				const_cast <client::core_t *> (this->_core)->close(this->_aid);
				// Выводим сообщение об ошибке
				return NGHTTP2_ERR_CALLBACK_FAILURE;
			// Выполняем отключение от сервера
			} else const_cast <client::core_t *> (this->_core)->close(this->_aid);
		}
	// Если функция обратного вызова активности потока установлена
	} else if(this->_callback.is("stream"))
		// Выводим функцию обратного вызова
		this->_callback.call <const int32_t, const mode_t> ("stream", sid, mode_t::CLOSE);
	// Выводим результат
	return 0;
}
/**
 * headerSignal Метод обратного вызова при получении заголовка HTTP/2 сервера
 * @param sid идентификатор потока
 * @param key данные ключа заголовка
 * @param val данные значения заголовка
 * @return    статус полученных данных
 */
int awh::client::Http2::headerSignal(const int32_t sid, const string & key, const string & val) noexcept {
	// Если подключение производится через, прокси-сервер
	if(this->_scheme.isProxy())
		// Выполняем обработку полученных заголовков для прокси-сервера
		return this->headerProxySignal(sid, key, val);
	// Если мы работаем с сервером напрямую
	else {
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
					this->_ws2.headerSignal(sid, key, val);
				break;
			}
		}
	}
	// Выводим результат
	return 0;
}
/**
 * redirect Метод выполнения редиректа если требуется
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 * @return     результат выполнения редиректа
 */
bool awh::client::Http2::redirect(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Результат работы функции
	bool result = false;
	// Если список ответов получен
	if(!this->_stopped && !this->_requests.empty() && !this->_workers.empty()){
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
							// Если список ответов получен
							if((result = !this->_stopped)){
								// Получаем параметры запроса
								const auto & response = it->second->http.response();
								// Если необходимо выполнить ещё одну попытку выполнения авторизации
								if((result = (this->_proxy.answer == 407) || (response.code == 401) || (response.code == 407))){
									// Увеличиваем количество попыток
									this->_attempt++;
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
								if((result = it->second->http.isHeader("location"))){
									// Получаем новый адрес запроса
									const uri_t::url_t & url = it->second->http.getUrl();
									// Если адрес запроса получен
									if((result = !url.empty())){
										// Увеличиваем количество попыток
										this->_attempt++;
										// Устанавливаем новый адрес запроса
										this->_uri.combine(this->_scheme.url, url);
										// Выполняем поиск параметров запроса
										auto jt = this->_requests.find(it->first);
										// Если необходимые нам параметры запроса найдены
										if((result = (jt != this->_requests.end()))){
											// Устанавливаем новый адрес запроса
											jt->second->url = this->_scheme.url;
											// Если необходимо метод изменить на GET и основной метод не является GET
											if(((response.code == 201) || (response.code == 303)) && (jt->second->method != awh::web_t::method_t::GET)){
												// Выполняем очистку тела запроса
												jt->second->entity.clear();
												// Выполняем установку метода запроса
												jt->second->method = awh::web_t::method_t::GET;
											}
										}
										// Выполняем установку следующего экшена на открытие подключения
										this->open();
										// Завершаем работу
										return result;
									}
								}
							}
						}
					// Если активирован режим работы с HTTP/1.1 протоколом
					} else {
						// Выполняем передачу сигнала отключения от сервера на HTTP/1.1 клиент
						this->_http1.disconnectCallback(aid, sid, core);
						// Если список ответов получен
						if((result = it->second->update = !this->_http1._stopped)){
							// Получаем параметры запроса
							const auto & response = this->_http1._http.response();
							// Если необходимо выполнить ещё одну попытку выполнения авторизации
							if((result = it->second->update = (this->_proxy.answer == 407) || (response.code == 401) || (response.code == 407))){
								// Выполняем очистку оставшихся данных
								this->_http1._buffer.clear();
								// Получаем количество попыток
								this->_attempt = this->_http1._attempt;
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
							if((result = it->second->update = this->_http1._http.isHeader("location"))){
								// Выполняем очистку оставшихся данных
								this->_http1._buffer.clear();
								// Получаем новый адрес запроса
								const uri_t::url_t & url = this->_http1._http.getUrl();
								// Если адрес запроса получен
								if((result = it->second->update = !url.empty())){
									// Получаем количество попыток
									this->_attempt = this->_http1._attempt;
									// Устанавливаем новый адрес запроса
									this->_uri.combine(this->_scheme.url, url);
									// Выполняем поиск параметров запроса
									auto jt = this->_requests.find(it->first);
									// Если необходимые нам параметры запроса найдены
									if((result = it->second->update = (jt != this->_requests.end()))){
										// Устанавливаем новый адрес запроса
										jt->second->url = this->_scheme.url;
										// Если необходимо метод изменить на GET и основной метод не является GET
										if(((response.code == 201) || (response.code == 303)) && (jt->second->method != awh::web_t::method_t::GET)){
											// Выполняем очистку тела запроса
											jt->second->entity.clear();
											// Выполняем установку метода запроса
											jt->second->method = awh::web_t::method_t::GET;
										}
									}
									// Выполняем установку следующего экшена на открытие подключения
									this->open();
									// Завершаем работу
									return result;
								}
							}
						}
					}
				} break;
				// Если протоколом агента является WebSocket-клиент
				case static_cast <uint8_t> (agent_t::WEBSOCKET): {
					// Снимаем флаг инициализации сессии HTTP2
					this->_ws2._sessionInitialized = false;
					// Выполняем переброс вызова дисконнекта на клиент WebSocket
					this->_ws2.disconnectCallback(aid, sid, core);
					// Если список ответов получен
					if((result = it->second->update = !this->_ws2._stopped)){
						// Получаем параметры запроса
						const auto & response = this->_ws2._http.response();
						// Если необходимо выполнить ещё одну попытку выполнения авторизации
						if((result = it->second->update = (this->_proxy.answer == 407) || (response.code == 401) || (response.code == 407))){
							// Выполняем очистку оставшихся данных
							this->_ws2._buffer.clear();
							// Получаем количество попыток
							this->_attempt = this->_ws2._attempt;
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
						if((result = it->second->update = this->_ws2._http.isHeader("location"))){
							// Выполняем очистку оставшихся данных
							this->_ws2._buffer.clear();
							// Получаем новый адрес запроса
							const uri_t::url_t & url = this->_ws2._http.getUrl();
							// Если адрес запроса получен
							if((result = it->second->update = !url.empty())){
								// Получаем количество попыток
								this->_attempt = this->_ws2._attempt;
								// Устанавливаем новый адрес запроса
								this->_uri.combine(this->_scheme.url, url);
								// Выполняем поиск параметров запроса
								auto jt = this->_requests.find(it->first);
								// Если необходимые нам параметры запроса найдены
								if((result = it->second->update = (jt != this->_requests.end())))
									// Устанавливаем новый адрес запроса
									jt->second->url = this->_scheme.url;
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
	}
	// Выводим результат
	return result;
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
					if(dynamic_cast <request_t *> (&request) != jt->second.get()){
						// Выполняем установку адреса URL-запроса
						request.url = jt->second->url;
						// Выполняем копирование метода запроса
						request.method = jt->second->method;
						// Если список заголовков получен
						if(!jt->second->headers.empty())
							// Выполняем установку заголовков запроса
							request.headers = jt->second->headers;
						// Выполняем очистку полученных заголовков
						else request.headers.clear();
						// Если тело запроса существует
						if(!jt->second->entity.empty())
							// Устанавливаем тело запроса
							request.entity.assign(jt->second->entity.begin(), jt->second->entity.end());
						// Выполняем очистку полученных данных тела запроса
						else request.entity.clear();
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
awh::client::Web::status_t awh::client::Http2::prepare(const int32_t sid, const uint64_t aid, client::core_t * core) noexcept {
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
				// Если функция обратного вызова на на вывод ошибок установлена
				if((response.code == 401) && this->_callback.is("error"))
					// Выводим функцию обратного вызова
					this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_RECV, "authorization failed");
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
								this->send(* jt->second.get());
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
								this->send(* jt->second.get());
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
				if(!it->second->http.body().empty() && this->_callback.is("entity"))
					// Устанавливаем полученную функцию обратного вызова
					it->second->callback.set <void (const int32_t, const u_int, const string, const vector <char>)> ("entity", this->_callback.get <void (const int32_t, const u_int, const string, const vector <char>)> ("entity"), sid, response.code, response.message, it->second->http.body());
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
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выводим функцию обратного вызова
					this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_RECV, this->_http.message(response.code).c_str());
				// Если возникла ошибка выполнения запроса
				if((response.code >= 400) && (response.code < 500)){
					// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
					if(!it->second->http.body().empty() && this->_callback.is("entity"))
						// Устанавливаем полученную функцию обратного вызова
						it->second->callback.set <void (const int32_t, const u_int, const string, const vector <char>)> ("entity", this->_callback.get <void (const int32_t, const u_int, const string, const vector <char>)> ("entity"), sid, response.code, response.message, it->second->http.body());
					// Выполняем удаление параметров запроса
					this->_requests.erase(sid);
					// Завершаем обработку
					return status_t::NEXT;
				}
			} break;
		}
		// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
		if(!it->second->http.body().empty() && this->_callback.is("entity"))
			// Устанавливаем полученную функцию обратного вызова
			it->second->callback.set <void (const int32_t, const u_int, const string, const vector <char>)> ("entity", this->_callback.get <void (const int32_t, const u_int, const string, const vector <char>)> ("entity"), sid, response.code, response.message, it->second->http.body());
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
 * @param request параметры запроса на удалённый сервер
 * @return        идентификатор отправленного запроса
 */
int32_t awh::client::Http2::send(const request_t & request) noexcept {
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
			// Агент воркера выполнения запроса
			agent_t agent = agent_t::HTTP;
			// Если список заголовков установлен
			if(!request.headers.empty()){
				// Выполняем перебор всего списка заголовков
				for(auto & item : request.headers){
					// Если заголовок соответствует смене протокола на WebSocket
					if(this->_fmk->compare(item.first, "upgrade") && this->_fmk->compare(item.second, "websocket")){
						// Если протокол WebSocket разрешён для подключения
						if(this->_webSocket){
							// Выполняем установку агента воркера WebSocket
							agent = agent_t::WEBSOCKET;
							// Если флаг инициализации сессии HTTP2 установлен
							if(this->_sessionInitialized){
								// Если протокол ещё не установлен
								if(request.headers.count(":protocol") < 1)
									// Выполняем установку протокола WebSocket
									const_cast <request_t &> (request).headers.emplace(":protocol", item.second);
								// Выполняем удаление заголовка Upgrade
								const_cast <request_t &> (request).headers.erase(item.first);
							}
						// Если протокол WebSocket запрещён
						} else {
							// Выводим сообщение об ошибке
							this->_log->print("Websocket protocol is prohibited for connection", log_t::flag_t::WARNING);
							// Если функция обратного вызова на на вывод ошибок установлена
							if(this->_callback.is("error"))
								// Выводим функцию обратного вызова
								this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::WARNING, http::error_t::HTTP1_SEND, "Websocket protocol is prohibited for connection");
							// Выходим из функции
							return sid;
						}
						// Выходим из цикла
						break;
					}
				}
			}
			// Определяем тип агента
			switch(static_cast <uint8_t> (agent)){
				// Если протоколом агента является HTTP-клиент
				case static_cast <uint8_t> (agent_t::HTTP): {
					// Если флаг инициализации сессии HTTP2 установлен
					if(this->_sessionInitialized){
						// Выполняем сброс состояния HTTP парсера
						this->_http.reset();
						// Выполняем очистку параметров HTTP запроса
						this->_http.clear();
						// Если метод компрессии установлен
						if(request.compress != http_t::compress_t::NONE)
							// Устанавливаем метод компрессии переданный пользователем
							this->_http.compress(request.compress);
						// Устанавливаем метод компрессии
						else this->_http.compress(this->_compress);
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
							// Устанавливаем новый адрес запроса
							this->_uri.combine(this->_scheme.url, request.url);
							// Создаём объек запроса
							awh::web_t::req_t query(2.0f, request.method, this->_scheme.url);
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
							// Если метод CONNECT запрещён для прокси-сервера
							if(!this->_proxy.connect){
								// Получаем строку авторизации на проксе-сервере
								const string & auth = this->_scheme.proxy.http.getAuth(http_t::process_t::REQUEST, request.method);
								// Если строка автоирации получена
								if(!auth.empty())
									// Выполняем добавление заголовка авторизации
									this->_http.header("Proxy-Authorization", auth);
							}
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
									// Если функция обратного вызова на на вывод ошибок установлена
									if(this->_callback.is("error"))
										// Выводим функцию обратного вызова
										this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_PIPE_INIT, strerror(errno));
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
											// Если функция обратного вызова на на вывод ошибок установлена
											if(this->_callback.is("error"))
												// Выводим функцию обратного вызова
												this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_PIPE_WRITE, strerror(errno));
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
											// Если функция обратного вызова на на вывод ошибок установлена
											if(this->_callback.is("error"))
												// Выводим функцию обратного вызова
												this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_PIPE_WRITE, strerror(errno));
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
								data.read_callback = &nghttp2_t::read;
								// Выполняем запрос на удалённый сервер
								result = nghttp2_submit_request(this->_nghttp2.session, nullptr, nva.data(), nva.size(), &data, this);
							// Если тело запроса не существует, выполняем установленный запрос
							} else result = nghttp2_submit_request(this->_nghttp2.session, nullptr, nva.data(), nva.size(), nullptr, this);
							// Если запрос не получилось отправить
							if(result < 0){
								// Выводим в лог сообщение
								this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(result));
								// Если функция обратного вызова на на вывод ошибок установлена
								if(this->_callback.is("error"))
									// Выводим функцию обратного вызова
									this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_SUBMIT, nghttp2_strerror(result));
								// Выполняем закрытие подключения
								core->close(this->_aid);
								// Выходим из функции
								return result;
							}{
								// Если функция обратного вызова на вывод редиректа потоков установлена
								if((sid > -1) && (sid != result) && this->_callback.is("redirect"))
									// Выводим функцию обратного вызова
									this->_callback.call <const int32_t, const int32_t> ("redirect", sid, result);
							}{
								// Результат фиксации сессии
								int rv = -1;
								// Фиксируем отправленный результат
								if((rv = nghttp2_session_send(this->_nghttp2.session)) != 0){
									// Выводим сообщение об полученной ошибке
									this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
									// Если функция обратного вызова на на вывод ошибок установлена
									if(this->_callback.is("error"))
										// Выводим функцию обратного вызова
										this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_SEND, nghttp2_strerror(rv));
									// Выполняем закрытие подключения
									core->close(this->_aid);
									// Выходим из функции
									return -1;
								}
							}
						}
					// Если активирован режим работы с HTTP/1.1 протоколом
					} else {
						// Выполняем обновление полученных данных, с целью выполнения редиректа если требуется
						sid = this->update(* const_cast <request_t *> (&request));
						// Если метод компрессии не установлен
						if(request.compress == http_t::compress_t::NONE)
							// Устанавливаем метод компрессии
							this->_http1._compress = this->_compress;
						// Выполняем отправку на сервер запроса
						result = this->_http1.send(request);
					}
				} break;
				// Если протоколом агента является WebSocket-клиент
				case static_cast <uint8_t> (agent_t::WEBSOCKET): {
					// Выполняем обновление полученных данных, с целью выполнения редиректа если требуется
					sid = this->update(* const_cast <request_t *> (&request));
					// Если HTTP-заголовки установлены
					if(!request.headers.empty())
						// Выполняем установку HTTP-заголовков
						this->_ws2.setHeaders(request.headers);
					// Если метод компрессии установлен
					if(request.compress != http_t::compress_t::NONE)
						// Устанавливаем метод компрессии переданный пользователем
						this->_ws2._compress = request.compress;
					// Устанавливаем метод компрессии
					else this->_ws2._compress = this->_compress;
					// Устанавливаем новый адрес запроса
					this->_uri.combine(this->_ws2._scheme.url, request.url);
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
					// Выполняем установку идентификатора объекта
					ret.first->second->http.id(this->_aid);
					// Если метод компрессии установлен
					if(request.compress != http_t::compress_t::NONE)
						// Устанавливаем метод компрессии переданный пользователем
						ret.first->second->http.compress(request.compress);
					// Устанавливаем метод компрессии
					else ret.first->second->http.compress(this->_compress);
					// Если флаг инициализации сессии HTTP2 установлен
					if(this->_sessionInitialized)
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
					if(!this->_ident.id.empty() || !this->_ident.name.empty() || !this->_ident.ver.empty())
						// Устанавливаем данные сервиса
						ret.first->second->http.ident(this->_ident.id, this->_ident.name, this->_ident.ver);
					// Если пароль для шифрования передан
					if(!this->_crypto.pass.empty())
						// Устанавливаем параметры шифрования для HTTP-клиента
						ret.first->second->http.crypto(this->_crypto.pass, this->_crypto.salt, this->_crypto.cipher);
				}
			}
			// Если идентификатор устаревшего запроса найден
			if(!this->_sessionInitialized && (result != sid) && (sid > 0)){
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
 * @param text    данные передаются в текстовом виде
 */
void awh::client::Http2::send(const char * message, const size_t size, const bool text) noexcept {
	// Если список воркеров активен
	if(!this->_workers.empty()){
		// Выполняем перебор всего списка воркеров
		for(auto & worker : this->_workers){
			// Если найден воркер WebSocket-клиента
			if(worker.second->agent == agent_t::WEBSOCKET){
				// Выполняем отправку сообщения на WebSocket-сервер
				this->_ws2.send(message, size, text);
				// Выходим из цикла
				break;
			}
		}
	}
}
/**
 * send Метод отправки сообщения на сервер
 * @param id     идентификатор потока HTTP/2
 * @param buffer буфер бинарных данных передаваемых на сервер
 * @param size   размер сообщения в байтах
 * @param end    флаг последнего сообщения после которого поток закрывается
 */
void awh::client::Http2::send(const int32_t id, const char * buffer, const size_t size, const bool end) noexcept {
	// Выполняем отправку данных на удалённый сервер
	web2_t::send(id, buffer, size, end);
}
/**
 * send Метод отправки заголовков на сервер
 * @param id      идентификатор потока HTTP/2
 * @param headers заголовки отправляемые на сервер
 * @param end     размер сообщения в байтах
 * @return        флаг последнего сообщения после которого поток закрывается
 */
int32_t awh::client::Http2::send(const int32_t id, const vector <pair <string, string>> & headers, const bool end) noexcept {
	// Выполняем отправку заголовков на сервер
	return web2_t::send(id, headers, end);
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
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws2.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param callback функция обратного вызова
 */
void awh::client::Http2::on(function <void (const vector <char> &, const bool)> callback) noexcept {
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws2.on(callback);
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
 * on Метод установки функции обратного вызова для перехвата полученных чанков
 * @param callback функция обратного вызова
 */
void awh::client::Http2::on(function <void (const uint64_t, const vector <char> &, const awh::http_t *)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP/1.1 клиента
	this->_http1.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибки
 * @param callback функция обратного вызова
 */
void awh::client::Http2::on(function <void (const log_t::flag_t, const http::error_t, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP/1.1 клиента
	this->_http1.on(callback);
}
/**
 * on Метод выполнения редиректа с одного потока на другой (необходим для совместимости с HTTP/2)
 * @param callback функция обратного вызова
 */
void awh::client::Http2::on(function <void (const int32_t, const int32_t)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const int32_t, const int32_t)> ("redirect", callback);
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
 * on Метод установки функция обратного вызова при выполнении рукопожатия
 * @param callback функция обратного вызова
 */
void awh::client::Http2::on(function <void (const int32_t, const agent_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP/1.1 клиента
	this->_http1.on(callback);
}
/**
 * on Метод установки функции обратного вызова при завершении запроса
 * @param callback функция обратного вызова
 */
void awh::client::Http2::on(function <void (const int32_t, const direct_t)> callback) noexcept {
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
 * subprotocol Метод установки поддерживаемого сабпротокола
 * @param subprotocol сабпротокол для установки
 */
void awh::client::Http2::subprotocol(const string & subprotocol) noexcept {
	// Выполняем установку поддерживаемого сабпротокола
	this->_ws2.subprotocol(subprotocol);
}
/**
 * subprotocol Метод получения списка выбранных сабпротоколов
 * @return список выбранных сабпротоколов
 */
const set <string> & awh::client::Http2::subprotocols() const noexcept {
	// Выполняем извлечение списка выбранных сабпротоколов
	return this->_ws2.subprotocols();
}
/**
 * subprotocols Метод установки списка поддерживаемых сабпротоколов
 * @param subprotocols сабпротоколы для установки
 */
void awh::client::Http2::subprotocols(const set <string> & subprotocols) noexcept {
	// Выполняем установку поддерживаемых сабпротоколов
	this->_ws2.subprotocols(subprotocols);
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
	if(size > 0)
		// Устанавливаем размер сегментов фрейма для WebSocket-клиента
		this->_ws2.segmentSize(size);
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
	// Устанавливаем флаг разрешающий выполнять подключение к протоколу WebSocket
	this->_webSocket = (flags.count(flag_t::WEBSOCKET_ENABLE) > 0);
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
		// Активируем персистентный запуск для работы пингов
		const_cast <client::core_t *> (this->_core)->persistEnable(true);
		// Устанавливаем функцию активации ядра клиента
		const_cast <client::core_t *> (this->_core)->on(std::bind(&http2_t::eventsCallback, this, _1, _2));
		// Если многопоточность активированна
		if(this->_threads > 0)
			// Устанавливаем простое чтение базы событий
			const_cast <client::core_t *> (this->_core)->easily(true);
	// Если объект сетевого ядра не передан но ранее оно было добавлено
	} else if(this->_core != nullptr) {
		// Если многопоточность активированна
		if(this->_threads <= 0){
			// Если многопоточность активированна
			if(this->_ws2._thr.is() || this->_ws2._ws1._thr.is()){
				// Выполняем завершение всех активных потоков
				this->_ws2._thr.wait();
				// Выполняем завершение всех активных потоков
				this->_ws2._ws1._thr.wait();
			}
			// Снимаем режим простого чтения базы событий
			const_cast <client::core_t *> (this->_core)->easily(false);
		}
		// Деактивируем персистентный запуск для работы пингов
		const_cast <client::core_t *> (this->_core)->persistEnable(false);
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
 * ident Метод установки идентификации клиента
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::client::Http2::ident(const string & id, const string & name, const string & ver) noexcept {
	// Если данные сервиса переданы
	if(!id.empty() && !name.empty() && !ver.empty()){
		// Выполняем установку данных сервиса у родительского класса
		web2_t::ident(id, name, ver);
		// Устанавливаем данные сервиса для WebSocket-клиента
		this->_ws2.ident(id, name, ver);
		// Устанавливаем данные сервиса для HTTP-парсера
		this->_http.ident(id, name, ver);
		// Устанавливаем данные сервиса для HTTP-клиента
		this->_http1.ident(id, name, ver);
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
 * proxy Метод установки прокси-сервера
 * @param uri    параметры прокси-сервера
 * @param family семейстово интернет протоколов (IPV4 / IPV6 / NIX)
 */
void awh::client::Http2::proxy(const string & uri, const scheme_t::family_t family) noexcept {
	// Выполняем установку параметры прокси-сервера
	web2_t::proxy(uri, family);
	// Выполняем установку параметры прокси-сервера для WebSocket-клиента
	this->_ws2.proxy(uri, family);
	// Выполняем установку параметры прокси-сервера для HTTP-клиента
	this->_http1.proxy(uri, family);
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
 * authTypeProxy Метод установки типа авторизации прокси-сервера
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest-авторизации
 */
void awh::client::Http2::authTypeProxy(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Устанавливаем тип авторизации на проксе-сервере
	web2_t::authTypeProxy(type, hash);
	// Устанавливаем тип авторизации на проксе-сервере для WebSocket-клиента
	this->_ws2.authTypeProxy(type, hash);
	// Устанавливаем тип авторизации на проксе-сервере для HTTP-клиента
	this->_http1.authTypeProxy(type, hash);
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
 web2_t(fmk, log), _ws2(fmk, log), _http1(fmk, log), _http(fmk, log), _webSocket(false), _threads(-1) {
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws2.on((function <void (const int32_t, const mode_t)>) std::bind(&http2_t::stream, this, _1, _2));
	// Выполняем установку функции обратного вызова для HTTP/1.1 клиента
	this->_http1.on((function <void (const int32_t, const mode_t)>) std::bind(&http2_t::stream, this, _1, _2));
	// Устанавливаем функцию персистентного вызова
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("persist", std::bind(&http2_t::persistCallback, this, _1, _2, _3));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("write", std::bind(&http2_t::writeCallback, this, _1, _2, _3, _4, _5));
}
/**
 * Http2 Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::client::Http2::Http2(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 web2_t(core, fmk, log), _ws2(fmk, log), _http1(fmk, log), _http(fmk, log), _webSocket(false), _threads(-1) {
	// Активируем персистентный запуск для работы пингов
	const_cast <client::core_t *> (this->_core)->persistEnable(true);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws2.on((function <void (const int32_t, const mode_t)>) std::bind(&http2_t::stream, this, _1, _2));
	// Выполняем установку функции обратного вызова для HTTP/1.1 клиента
	this->_http1.on((function <void (const int32_t, const mode_t)>) std::bind(&http2_t::stream, this, _1, _2));
	// Устанавливаем функцию персистентного вызова
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("persist", std::bind(&http2_t::persistCallback, this, _1, _2, _3));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("write", std::bind(&http2_t::writeCallback, this, _1, _2, _3, _4, _5));
}
/**
 * ~Http2 Деструктор
 */
awh::client::Http2::~Http2() noexcept {
	// Снимаем адрес сетевого ядра
	this->_ws2._core = nullptr;
	// Снимаем активную сессию для зависимого модуля WebSocket
	this->_ws2._nghttp2.session = nullptr;
	// Снимаем флаг инициализации сессии HTTP2
	this->_ws2._sessionInitialized = false;
	// Если многопоточность активированна
	if(this->_ws2._thr.is())
		// Выполняем завершение всех активных потоков
		this->_ws2._thr.wait();
	// Если многопоточность активированна
	if(this->_ws2._ws1._thr.is())
		// Выполняем завершение всех активных потоков
		this->_ws2._ws1._thr.wait();
}
