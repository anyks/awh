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
 * @param bid  идентификатор брокера
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::Http2::connectCallback(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::OPEN, event_t::READ, event_t::PROXY_READ}, event_t::CONNECT)){
		// Запоминаем идентификатор брокера
		this->_bid = bid;
		// Выполняем установку идентификатора объекта
		this->_http.id(bid);
		// Выполняем инициализацию сессии HTTP/2
		web2_t::connectCallback(bid, sid, core);
		// Если флаг инициализации сессии HTTP/2 не установлен
		if(!this->_http2.is()){
			// Запоминаем идентификатор брокера
			this->_http1._bid = this->_bid;
			// Выполняем установку идентификатора объекта
			this->_http1._http.id(this->_bid);
			// Выполняем установку данных URL-адреса
			this->_http1._scheme.url = this->_scheme.url;
			// Выполняем установку сетевого ядра
			this->_http1._core = dynamic_cast <client::core_t *> (core);
		}
		// Выполняем установку идентификатора объекта
		this->_ws2._http.id(bid);
		// Выполняем установку сессии HTTP/2
		this->_ws2._http2 = this->_http2;
		// Выполняем установку данных URL-адреса
		this->_ws2._scheme.url = this->_scheme.url;
		// Выполняем установку сетевого ядра
		this->_ws2._core = dynamic_cast <client::core_t *> (core);
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
 * @param bid  идентификатор брокера
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::Http2::disconnectCallback(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Выполняем удаление подключения
	this->_http2.close();
	// Выполняем установку сессии HTTP/2
	this->_ws2._http2 = nullptr;
	// Выполняем редирект, если редирект выполнен
	if(this->redirect(bid, sid, core))
		// Выходим из функции
		return;
	// Выполняем очистку списка воркеров
	this->_workers.clear();
	// Выполняем очистку списка запросов
	this->_requests.clear();
	// Выполняем передачу сигнала отключения от сервера на WebSocket-клиент
	this->_ws2.disconnectCallback(bid, sid, core);
	// Выполняем передачу сигнала отключения от сервера на HTTP/1.1 клиент
	this->_http1.disconnectCallback(bid, sid, core);
	// Если подключение не является постоянным
	if(!this->_scheme.alive){
		// Выполняем сброс параметров запроса
		this->flush();
		// Выполняем зануление идентификатора брокера
		this->_bid = 0;
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
 * @param bid    идентификатор брокера
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::client::Http2::readCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (bid > 0) && (sid > 0)){
		// Если протокол подключения является HTTP/2
		if(core->proto(bid) == engine_t::proto_t::HTTP2){
			// Если прочитать данные фрейма не удалось, выходим из функции
			if(!this->_http2.frame((const uint8_t *) buffer, size)){
				// Выполняем установку функции обратного вызова триггера, для закрытия соединения после завершения всех процессов
				this->_http2.on((function <void (void)>) std::bind(static_cast <void (client::core_t::*)(const uint64_t)> (&client::core_t::close), dynamic_cast <client::core_t *> (core), bid));
				// Выходим из функции
				return;
			}
		// Если активирован режим работы с HTTP/1.1 протоколом
		} else {
			// Если активирован WebSocket-клиент
			if(this->_ws2._bid > 0)
				// Выполняем переброс вызова чтения на клиент WebSocket
				this->_ws2.readCallback(buffer, size, bid, sid, core);
			// Выполняем переброс вызова чтения на клиент HTTP/1.1
			else this->_http1.readCallback(buffer, size, bid, sid, core);
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
void awh::client::Http2::writeCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((bid > 0) && (sid > 0) && (core != nullptr)){
		// Выполняем перебор всех доступных воркеров
		for(auto & worker : this->_workers){
			// Определяем протокол клиента
			switch(static_cast <uint8_t> (worker.second->agent)){
				// Если агент является клиентом HTTP
				case static_cast <uint8_t> (agent_t::HTTP): {
					// Если переключение протокола на HTTP/2 не выполнено
					if(worker.second->proto != engine_t::proto_t::HTTP2)
						// Выполняем переброс вызова записи на клиент HTTP/1.1
						this->_http1.writeCallback(buffer, size, bid, sid, core);
				} break;
				// Если агент является клиентом WebSocket
				case static_cast <uint8_t> (agent_t::WEBSOCKET):
					// Выполняем переброс вызова записи на клиент WebSocket
					this->_ws2.writeCallback(buffer, size, bid, sid, core);
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
						it->second->http.payload(vector <char> (buffer, buffer + size));
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
int awh::client::Http2::frameSignal(const int32_t sid, const awh::http2_t::direct_t direct, const uint8_t type, const uint8_t flags) noexcept {
	// Определяем направление передачи фрейма
	switch(static_cast <uint8_t> (direct)){
		// Если производится передача фрейма на сервер
		case static_cast <uint8_t> (awh::http2_t::direct_t::SEND): {
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
		case static_cast <uint8_t> (awh::http2_t::direct_t::RECV): {
			// Если подключение производится через, прокси-сервер
			if(this->_scheme.isProxy())
				// Выполняем обработку полученных данных фрейма для прокси-сервера
				return this->frameProxySignal(sid, direct, type, flags);
			// Если мы работаем с сервером напрямую
			else if(this->_core != nullptr) {
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
										switch(static_cast <uint8_t> (this->prepare(sid, this->_bid, const_cast <client::core_t *> (this->_core)))){
											// Если необходимо выполнить пропуск обработки данных
											case static_cast <uint8_t> (status_t::SKIP):
												// Завершаем работу
												return 0;
										}
										// Выполняем очистку параметров HTTP запроса
										this->_http.clear();
										// Выполняем очистку параметров HTTP-апроса у конкретного потока
										it->second->http.clear();
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
										// Флаг полученных трейлеров из ответа сервера
										bool trailers = false;
										// Если трейлеры получены в ответе сервера
										if((trailers = it->second->http.is(http_t::suite_t::HEADER, "trailer"))){
											// Выполняем извлечение списка заголовков трейлеров
											const auto & range = it->second->http.headers().equal_range("trailer");
											// Выполняем перебор всех полученных заголовков трейлеров
											for(auto jt = range.first; jt != range.second; ++jt){
												// Если такой заголовок трейлера не получен
												if(!it->second->http.is(http_t::suite_t::HEADER, jt->second)){
													// Если мы получили флаг завершения потока
													if(flags & NGHTTP2_FLAG_END_STREAM){
														// Выводим сообщение об ошибке, что трейлер не существует
														this->_log->print("Trailer \"%s\" does not exist", log_t::flag_t::WARNING, jt->second.c_str());
														// Если функция обратного вызова на на вывод ошибок установлена
														if(this->_callback.is("error"))
															// Выводим функцию обратного вызова
															this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::WARNING, http::error_t::HTTP2_RECV, this->_fmk->format("Trailer \"%s\" does not exist", jt->second.c_str()));
														// Выполняем удаление выполненного воркера
														this->_workers.erase(sid);
														// Если установлена функция отлова завершения запроса
														if(this->_callback.is("end"))
															// Выводим функцию обратного вызова
															this->_callback.call <const int32_t, const direct_t> ("end", sid, direct_t::RECV);
													}
													// Завершаем работу
													return 0;
												}
											}
											// Выполняем удаление заголовка трейлеров
											it->second->http.rm(http_t::suite_t::HEADER, "trailer");
										}
										/**
										 * Если включён режим отладки
										 */
										#if defined(DEBUG_MODE)
											{
												// Получаем данные ответа
												const auto & response = it->second->http.process(http_t::process_t::RESPONSE, it->second->http.response());
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
										// Если трейлеры получены с сервера
										if(trailers)
											// Выполняем извлечение полученных данных полезной нагрузки
											return this->frameSignal(sid, awh::http2_t::direct_t::RECV, NGHTTP2_DATA, flags);
										// Если мы получили флаг завершения потока
										else if(flags & NGHTTP2_FLAG_END_STREAM) {
											// Выполняем препарирование полученных данных
											switch(static_cast <uint8_t> (this->prepare(sid, this->_bid, const_cast <client::core_t *> (this->_core)))){
												// Если необходимо выполнить пропуск обработки данных
												case static_cast <uint8_t> (status_t::SKIP):
													// Завершаем работу
													return 0;
											}
										}
									}
									// Если мы получили флаг завершения потока
									if(flags & NGHTTP2_FLAG_END_STREAM){
										// Выполняем удаление выполненного воркера
										this->_workers.erase(sid);
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
			// Если согласованные параметры SSL не приемлемы
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
		// Если флаг инициализации сессии HTTP/2 установлен
		if((this->_core != nullptr) && (error > 0x0) && this->_http2.is())
			// Выполняем установку функции обратного вызова триггера, для закрытия соединения после завершения всех процессов
			this->_http2.on((function <void (void)>) std::bind(static_cast <void (client::core_t::*)(const uint64_t)> (&client::core_t::close), const_cast <client::core_t *> (this->_core), this->_bid));
		// Если функция обратного вызова активности потока установлена
		if(this->_callback.is("stream"))
			// Выводим функцию обратного вызова
			this->_callback.call <const int32_t, const mode_t> ("stream", sid, mode_t::CLOSE);
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
 * end Метод завершения работы потока
 * @param sid    идентификатор потока
 * @param direct направление передачи данных
 */
void awh::client::Http2::end(const int32_t sid, const direct_t direct) noexcept {
	// Определяем направление передачи данных
	switch(static_cast <uint8_t> (direct)){
		// Если направление передачи данных отправка на сервер
		case static_cast <uint8_t> (direct_t::SEND):
			/** Здесь мы пока ничего не выполняем **/
		break;
		// Если направление передачи данных получение с сервера
		case static_cast <uint8_t> (direct_t::RECV):
			// Выполняем удаление выполненного воркера
			this->_workers.erase(sid);
		break;
	}
	// Если установлена функция отлова завершения запроса
	if(this->_callback.is("end"))
		// Выводим функцию обратного вызова
		this->_callback.call <const int32_t, const direct_t> ("end", sid, direct);
}
/**
 * redirect Метод выполнения смены потоков
 * @param from идентификатор предыдущего потока
 * @param to   идентификатор нового потока
 */
void awh::client::Http2::redirect(const int32_t from, const int32_t to) noexcept {
	// Выполняем поиск воркера предыдущего потока
	auto it = this->_workers.find(from);
	// Если воркер для предыдущего потока найден
	if(it != this->_workers.end()){
		// Выполняем установку объекта воркера
		auto ret = this->_workers.emplace(to, unique_ptr <worker_t> (new worker_t(this->_fmk, this->_log)));
		// Выполняем сброс состояния HTTP парсера
		ret.first->second->http.reset();
		// Выполняем очистку параметров HTTP запроса
		ret.first->second->http.clear();
		// Выполняем установку типа агента
		ret.first->second->agent = it->second->agent;
		// Выполняем установку активный прототип интернета
		ret.first->second->proto = it->second->proto;
		// Выполняем установку флага обновления данных
		ret.first->second->update = it->second->update;
	}
	// Если функция обратного вызова на вывод редиректа потоков установлена
	if((from != to) && this->_callback.is("redirect"))
		// Выводим функцию обратного вызова
		this->_callback.call <const int32_t, const int32_t> ("redirect", from, to);
}
/**
 * redirect Метод выполнения редиректа если требуется
 * @param bid  идентификатор брокера
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 * @return     результат выполнения редиректа
 */
bool awh::client::Http2::redirect(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
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
								if((result = it->second->http.is(http_t::suite_t::HEADER, "location"))){
									// Получаем новый адрес запроса
									const uri_t::url_t & url = it->second->http.url();
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
						this->_http1.disconnectCallback(bid, sid, core);
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
							if((result = it->second->update = this->_http1._http.is(http_t::suite_t::HEADER, "location"))){
								// Выполняем очистку оставшихся данных
								this->_http1._buffer.clear();
								// Получаем новый адрес запроса
								const uri_t::url_t & url = this->_http1._http.url();
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
					// Выполняем переброс вызова дисконнекта на клиент WebSocket
					this->_ws2.disconnectCallback(bid, sid, core);
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
						if((result = it->second->update = this->_ws2._http.is(http_t::suite_t::HEADER, "location"))){
							// Выполняем очистку оставшихся данных
							this->_ws2._buffer.clear();
							// Получаем новый адрес запроса
							const uri_t::url_t & url = this->_ws2._http.url();
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
 * pinging Метод таймера выполнения пинга удалённого сервера
 * @param tid  идентификатор таймера
 * @param core объект сетевого ядра
 */
void awh::client::Http2::pinging(const uint16_t tid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((tid > 0) && (core != nullptr) && (this->_core != nullptr)){
		// Выполняем перебор всех доступных воркеров
		for(auto & worker : this->_workers){
			// Определяем протокол клиента
			switch(static_cast <uint8_t> (worker.second->agent)){
				// Если агент является клиентом HTTP
				case static_cast <uint8_t> (agent_t::HTTP): {
					// Если переключение протокола на HTTP/2 выполнено и пинг не прошёл
					if(!this->ping())
						// Выполняем установку функции обратного вызова триггера, для закрытия соединения после завершения всех процессов
						this->_http2.on((function <void (void)>) std::bind(static_cast <void (client::core_t::*)(const uint64_t)> (&client::core_t::close), const_cast <client::core_t *> (this->_core), this->_bid));
				} break;
				// Если агент является клиентом WebSocket
				case static_cast <uint8_t> (agent_t::WEBSOCKET):
					// Выполняем переброс персистентного вызова на клиент WebSocket
					this->_ws2.pinging(tid, core);
				break;
			}
		}
	}
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
					// Выполняем извлечение полученных данных запроса
					it->second->http.mapping(http_t::process_t::REQUEST, this->_http);
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
 * @param bid  идентификатор брокера
 * @param core объект сетевого ядра
 * @return     результат препарирования
 */
awh::client::Web::status_t awh::client::Http2::prepare(const int32_t sid, const uint64_t bid, client::core_t * core) noexcept {
	// Выполняем поиск текущего воркера
	auto it = this->_workers.find(sid);
	// Если искомый воркер найден
	if(it != this->_workers.end()){
		// Получаем параметры запроса
		const auto & response = it->second->http.response();
		// Получаем статус ответа
		awh::http_t::status_t status = it->second->http.auth();
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
						const uri_t::url_t & url = it->second->http.url();
						// Если адрес запроса получен
						if(!url.empty()){
							// Выполняем проверку соответствие протоколов
							const bool schema = (
								(this->_fmk->compare(url.host, jt->second->url.host)) &&
								(this->_fmk->compare(url.schema, jt->second->url.schema))
							);
							// Если соединение является постоянным
							if(schema && it->second->http.is(http_t::state_t::ALIVE)){
								// Увеличиваем количество попыток
								this->_attempt++;
								// Устанавливаем новый адрес запроса
								this->_uri.combine(jt->second->url, url);
								// Отправляем повторный запрос
								this->send(* jt->second.get());
								// Завершаем работу
								return status_t::SKIP;
							}
							// Выполняем установку функции обратного вызова триггера, для закрытия соединения после завершения всех процессов
							this->_http2.on((function <void (void)>) std::bind(static_cast <void (client::core_t::*)(const uint64_t)> (&client::core_t::close), core, bid));
							// Завершаем работу
							return status_t::SKIP;
						// Если URL-адрес запроса не получен
						} else {
							// Если соединение является постоянным
							if(it->second->http.is(http_t::state_t::ALIVE)){
								// Увеличиваем количество попыток
								this->_attempt++;
								// Отправляем повторный запрос
								this->send(* jt->second.get());
								// Завершаем работу
								return status_t::SKIP;
							}
							// Выполняем установку функции обратного вызова триггера, для закрытия соединения после завершения всех процессов
							this->_http2.on((function <void (void)>) std::bind(static_cast <void (client::core_t::*)(const uint64_t)> (&client::core_t::close), core, bid));
							// Завершаем работу
							return status_t::SKIP;
						}
					}
				}
			} break;
			// Если запрос выполнен удачно
			case static_cast <uint8_t> (awh::http_t::status_t::GOOD): {
				// Выполняем сброс количества попыток
				this->_attempt = 0;
				// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
				if(!it->second->http.body().empty() && this->_callback.is("entity"))
					// Устанавливаем полученную функцию обратного вызова
					it->second->callback.set <void (const int32_t, const u_int, const string, const vector <char>)> ("entity", this->_callback.get <void (const int32_t, const u_int, const string, const vector <char>)> ("entity"), sid, response.code, response.message, it->second->http.body());
				// Устанавливаем размер стопбайт
				if(!it->second->http.is(http_t::state_t::ALIVE)){
					// Выполняем установку функции обратного вызова триггера, для закрытия соединения после завершения всех процессов
					this->_http2.on((function <void (void)>) std::bind(static_cast <void (client::core_t::*)(const uint64_t)> (&client::core_t::close), core, bid));
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
			case static_cast <uint8_t> (awh::http_t::status_t::FAULT): {
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
		// Выполняем установку функции обратного вызова триггера, для закрытия соединения после завершения всех процессов
		this->_http2.on((function <void (void)>) std::bind(static_cast <void (client::core_t::*)(const uint64_t)> (&client::core_t::close), core, bid));
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
 * sendMessage Метод отправки сообщения на сервер
 * @param message передаваемое сообщения в бинарном виде
 * @param text    данные передаются в текстовом виде
 */
void awh::client::Http2::sendMessage(const vector <char> & message, const bool text) noexcept {
	// Если список воркеров активен
	if(!this->_workers.empty()){
		// Выполняем перебор всего списка воркеров
		for(auto & worker : this->_workers){
			// Если найден воркер WebSocket-клиента
			if(worker.second->agent == agent_t::WEBSOCKET){
				// Выполняем отправку сообщения на WebSocket-сервер
				this->_ws2.sendMessage(message, text);
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
		if((this->_core != nullptr) && (this->_bid > 0)){
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
							// Если флаг инициализации сессии HTTP/2 установлен
							if(this->_http2.is()){
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
					// Если флаг инициализации сессии HTTP/2 установлен
					if(this->_http2.is()){
						// Выполняем сброс состояния HTTP парсера
						this->_http.reset();
						// Выполняем очистку параметров HTTP запроса
						this->_http.clear();
						// Если список компрессоров передан
						if(!request.compressors.empty())
							// Устанавливаем список поддерживаемых компрессоров
							this->_http.compressors(request.compressors);
						// Устанавливаем список поддерживаемых компрессоров
						else this->_http.compressors(this->_compressors);
						{
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
							// Если метод CONNECT запрещён для прокси-сервера
							if(!this->_proxy.connect){
								// Выполняем извлечение заголовка авторизации на прокси-сервера
								const string & header = this->_scheme.proxy.http.auth(http_t::process_t::REQUEST, query);
								// Если заголовок авторизации получен
								if(!header.empty())
									// Выполняем установки заголовка авторизации на прокси-сервере
									this->_http.header("Proxy-Authorization", header);
							}
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								// Выводим заголовок запроса
								cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
								// Получаем бинарные данные WEB запроса
								const auto & buffer = this->_http.process(http_t::process_t::REQUEST, query);
								// Выводим параметры запроса
								cout << string(buffer.begin(), buffer.end()) << endl << endl;
							#endif
							// Выполняем запрос на получение заголовков
							const auto & headers = this->_http.process2(http_t::process_t::REQUEST, std::move(query));
							// Флаг отправляемого фрейма
							awh::http2_t::flag_t flag = awh::http2_t::flag_t::NONE;
							// Если тело запроса не существует
							if(request.entity.empty())
								// Устанавливаем флаг завершения потока
								flag = awh::http2_t::flag_t::END_STREAM;
							// Выполняем заголовки запроса на сервер
							result = web2_t::send(-1, headers, flag);
							// Если запрос не получилось отправить
							if(result < 0)
								// Выходим из функции
								return result;
							// Если функция обратного вызова на вывод редиректа потоков установлена
							if((sid > -1) && (sid != result) && this->_callback.is("redirect"))
								// Выводим функцию обратного вызова
								this->_callback.call <const int32_t, const int32_t> ("redirect", sid, result);
							// Если тело запроса существует
							if(!request.entity.empty()){
								// Тело WEB запроса
								vector <char> entity;
								// Получаем данные тела запроса
								while(!(entity = this->_http.payload()).empty()){
									/**
									 * Если включён режим отладки
									 */
									#if defined(DEBUG_MODE)
										// Выводим сообщение о выводе чанка тела
										cout << this->_fmk->format("<chunk %zu>", entity.size()) << endl << endl;
									#endif
									// Если нужно установить флаг закрытия потока
									if(this->_http.body().empty())
										// Устанавливаем флаг завершения потока
										flag = awh::http2_t::flag_t::END_STREAM;
									// Выполняем отправку тела запроса на сервер
									if(!web2_t::send(result, entity.data(), entity.size(), flag))
										// Выходим из функции
										return -1;
								}
							}
						}
					// Если активирован режим работы с HTTP/1.1 протоколом
					} else {
						// Выполняем обновление полученных данных, с целью выполнения редиректа если требуется
						sid = this->update(* const_cast <request_t *> (&request));
						// Если список доступных компрессоров установлен
						if(!request.compressors.empty())
							// Устанавливаем список поддерживаемых компрессоров
							this->_http1._compressors = request.compressors;
						// Устанавливаем список поддерживаемых компрессоров
						else this->_http1._compressors = this->_compressors;
						// Если список запросов уже заполнен
						if(!this->_http1._requests.empty())
							// Выполняем замену активного запроса
							this->_http1._requests.begin()->second = request;
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
					// Если список доступных компрессоров установлен
					if(!request.compressors.empty())
						// Устанавливаем список поддерживаемых компрессоров
						this->_ws2._compressors = request.compressors;
					// Устанавливаем список поддерживаемых компрессоров
					else this->_ws2._compressors = this->_compressors;
					// Устанавливаем новый адрес запроса
					this->_uri.combine(this->_ws2._scheme.url, request.url);
					// Выполняем установку подключения с WebSocket-сервером
					this->_ws2.connectCallback(this->_bid, this->_scheme.sid, dynamic_cast <awh::core_t *> (const_cast <client::core_t *> (this->_core)));
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
					ret.first->second->http.id(this->_bid);
					// Если сервер требует авторизацию
					if(this->_service.type != auth_t::type_t::NONE){
						// Устанавливаем параметры авторизации для клиента
						ret.first->second->http.authType(this->_service.type, this->_service.hash);
						// Устанавливаем логин и пароль пользователя для клиента
						ret.first->second->http.user(this->_service.login, this->_service.password);
					}
					// Если список доступных компрессоров установлен
					if(!request.compressors.empty())
						// Устанавливаем список поддерживаемых компрессоров
						ret.first->second->http.compressors(request.compressors);
					// Устанавливаем список поддерживаемых компрессоров
					else ret.first->second->http.compressors(this->_compressors);
					// Если флаг инициализации сессии HTTP/2 установлен
					if(this->_http2.is())
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
					// Если логин пользователя и пароль установлены
					if(!this->_login.empty() && !this->_password.empty())
						// Устанавливаем логин и пароль пользователя
						ret.first->second->http.user(this->_login, this->_password);
					// Если параметры сервиса установлены
					if(!this->_ident.id.empty() || !this->_ident.name.empty() || !this->_ident.ver.empty())
						// Устанавливаем данные сервиса
						ret.first->second->http.ident(this->_ident.id, this->_ident.name, this->_ident.ver);
					// Если шифрование активированно
					if(this->_encryption.mode){
						// Устанавливаем флаг шифрования
						ret.first->second->http.encryption(this->_encryption.mode);
						// Устанавливаем параметры шифрования для HTTP-клиента
						ret.first->second->http.encryption(this->_encryption.pass, this->_encryption.salt, this->_encryption.cipher);
					}
				}
			}
			// Если идентификатор устаревшего запроса найден
			if(((result > 0) && (sid > 0) && (result != sid)) && !this->_http2.is()){
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
 * send Метод отправки тела сообщения на сервер
 * @param id     идентификатор потока HTTP
 * @param buffer буфер бинарных данных передаваемых на сервер
 * @param size   размер сообщения в байтах
 * @param end    флаг последнего сообщения после которого поток закрывается
 * @return       результат отправки данных указанному клиенту
 */
bool awh::client::Http2::send(const int32_t id, const char * buffer, const size_t size, const bool end) noexcept {
	// Результат работы функции
	bool result = false;
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::READ, event_t::CONNECT}, event_t::SEND)){
		// Если данные переданы верные
		if((result = ((buffer != nullptr) && (size > 0)))){
			// Если флаг инициализации сессии HTTP/2 установлен
			if(this->_http2.is()){
				// Тело WEB сообщения
				vector <char> entity;
				// Выполняем сброс данных тела
				this->_http.clear(http_t::suite_t::BODY);
				// Устанавливаем тело запроса
				this->_http.body(vector <char> (buffer, buffer + size));
				// Флаг отправляемого фрейма
				awh::http2_t::flag_t flag = awh::http2_t::flag_t::NONE;
				// Получаем данные тела запроса
				while(!(entity = this->_http.payload()).empty()){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим сообщение о выводе чанка тела
						cout << this->_fmk->format("<chunk %zu>", entity.size()) << endl << endl;
					#endif
					// Если нужно установить флаг закрытия потока
					if(end && this->_http.body().empty())
						// Устанавливаем флаг завершения потока
						flag = awh::http2_t::flag_t::END_STREAM;
					// Выполняем отправку данных на удалённый сервер
					result = web2_t::send(id, entity.data(), entity.size(), flag);
				}
			// Если протокол HTTP/2 не активирован, передаём запрос через протокол HTTP/1.1
			} else result = this->_http1.send(buffer, size, end);
		}
	}
	// Выводим значение по умолчанию
	return result;
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
int32_t awh::client::Http2::send(const int32_t id, const uri_t::url_t & url, const awh::web_t::method_t method, const unordered_multimap <string, string> & headers, const bool end) noexcept {
	// Результат работы функции
	int32_t result = -1;
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::READ, event_t::CONNECT}, event_t::SEND)){
		// Если заголовки запроса переданы
		if(!headers.empty()){
			// Если флаг инициализации сессии HTTP/2 установлен
			if(this->_http2.is()){
				// Выполняем очистку параметров HTTP запроса
				this->_http.clear();
				// Устанавливаем заголовоки запроса
				this->_http.headers(headers);
				// Устанавливаем новый адрес запроса
				this->_uri.combine(this->_scheme.url, url);
				// Создаём объек запроса
				awh::web_t::req_t query(2.0f, method, this->_scheme.url);
				// Если метод CONNECT запрещён для прокси-сервера
				if(!this->_proxy.connect){
					// Выполняем извлечение заголовка авторизации на прокси-сервера
					const string & header = this->_scheme.proxy.http.auth(http_t::process_t::REQUEST, query);
					// Если заголовок авторизации получен
					if(!header.empty())
						// Выполняем установки заголовка авторизации на прокси-сервере
						this->_http.header("Proxy-Authorization", header);
				}
				// Выполняем запрос на получение заголовков
				const auto & headers = this->_http.process2(http_t::process_t::REQUEST, query);
				// Если заголовки запроса получены
				if(!headers.empty()){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим заголовок запроса
						cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
						// Получаем бинарные данные WEB запроса
						const auto & buffer = this->_http.process(http_t::process_t::REQUEST, query);
						// Выводим параметры запроса
						cout << string(buffer.begin(), buffer.end()) << endl << endl;
					#endif
					// Выполняем заголовки запроса на сервер
					result = web2_t::send(id, headers, (end ? awh::http2_t::flag_t::END_STREAM : awh::http2_t::flag_t::NONE));
				}
			// Если протокол HTTP/2 не активирован, передаём запрос через протокол HTTP/1.1
			} else result = this->_http1.send(url, method, headers, end);
		}
	}
	// Выводим значение по умолчанию
	return result;
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
			if(this->_ws2._thr.is() || this->_ws2._ws1._thr.is()){
				// Выполняем завершение всех активных потоков
				this->_ws2._thr.wait();
				// Выполняем завершение всех активных потоков
				this->_ws2._ws1._thr.wait();
			}
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
void awh::client::Http2::user(const string & login, const string & password) noexcept {
	// Устанавливаем логин пользователя для авторизации на сервере
	this->_service.login = login;
	// Устанавливаем пароль пользователя для авторизации на сервере
	this->_service.password = password;
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
 * @param count количество потоков для активации
 * @param mode  флаг активации/деактивации мультипоточности
 */
void awh::client::Http2::multiThreads(const int16_t count, const bool mode) noexcept {
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
	// Устанавливаем алгоритм шифрования для Digest-авторизации
	this->_service.hash = hash;
	// Устанавливаем тип авторизации
	this->_service.type = type;
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
 * encryption Метод активации шифрования
 * @param mode флаг активации шифрования
 */
void awh::client::Http2::encryption(const bool mode) noexcept {
	// Устанавливаем флаг шифрования у родительского объекта
	web2_t::encryption(mode);
	// Устанавливаем флаг шифрования для WebSocket-клиента
	this->_ws2.encryption(mode);
	// Устанавливаем флаг шифрования для HTTP-парсера
	this->_http.encryption(mode);
	// Устанавливаем флаг шифрования для HTTP-клиента
	this->_http1.encryption(mode);
}
/**
 * encryption Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::client::Http2::encryption(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Устанавливаем параметры шифрования у родительского объекта
	web2_t::encryption(pass, salt, cipher);
	// Устанавливаем параметры шифрования для WebSocket-клиента
	this->_ws2.encryption(pass, salt, cipher);
	// Устанавливаем параметры шифрования для HTTP-парсера
	this->_http.encryption(pass, salt, cipher);
	// Устанавливаем параметры шифрования для HTTP-клиента
	this->_http1.encryption(pass, salt, cipher);
}
/**
 * Http2 Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::client::Http2::Http2(const fmk_t * fmk, const log_t * log) noexcept :
 web2_t(fmk, log), _ws2(fmk, log), _http1(fmk, log), _http(fmk, log), _webSocket(false), _threads(-1) {
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws2.on(std::bind(&http2_t::end, this, _1, _2));
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws2.on((function <void (const int32_t, const mode_t)>) std::bind(&http2_t::stream, this, _1, _2));
	// Выполняем установку функции обратного вызова перехвата события редиректа
	this->_ws2.on((function <void (const int32_t, const int32_t)>) std::bind(static_cast <void (http2_t::*)(const int32_t, const int32_t)> (&http2_t::redirect), this, _1, _2));
	// Выполняем установку функции обратного вызова для HTTP/1.1 клиента
	this->_http1.on((function <void (const int32_t, const mode_t)>) std::bind(&http2_t::stream, this, _1, _2));
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
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws2.on(std::bind(&http2_t::end, this, _1, _2));
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws2.on((function <void (const int32_t, const mode_t)>) std::bind(&http2_t::stream, this, _1, _2));
	// Выполняем установку функции обратного вызова перехвата события редиректа
	this->_ws2.on((function <void (const int32_t, const int32_t)>) std::bind(static_cast <void (http2_t::*)(const int32_t, const int32_t)> (&http2_t::redirect), this, _1, _2));
	// Выполняем установку функции обратного вызова для HTTP/1.1 клиента
	this->_http1.on((function <void (const int32_t, const mode_t)>) std::bind(&http2_t::stream, this, _1, _2));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("write", std::bind(&http2_t::writeCallback, this, _1, _2, _3, _4, _5));
}
/**
 * ~Http2 Деструктор
 */
awh::client::Http2::~Http2() noexcept {
	// Снимаем адрес сетевого ядра
	this->_ws2._core = nullptr;
	// Выполняем установку сессии HTTP/2
	this->_ws2._http2 = nullptr;
	// Если многопоточность активированна
	if(this->_ws2._thr.is())
		// Выполняем завершение всех активных потоков
		this->_ws2._thr.wait();
}
