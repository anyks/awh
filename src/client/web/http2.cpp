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
 * connectEvent Метод обратного вызова при подключении к серверу
 * @param bid идентификатор брокера
 * @param sid идентификатор схемы сети
 */
void awh::client::Http2::connectEvent(const uint64_t bid, const uint16_t sid) noexcept {
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::OPEN, event_t::READ, event_t::PROXY_READ, event_t::PROXY_CONNECT}, event_t::CONNECT)){
		// Запоминаем идентификатор брокера
		this->_bid = bid;
		// Выполняем установку идентификатора объекта
		this->_http.id(bid);
		// Выполняем инициализацию сессии HTTP/2
		web2_t::connectEvent(bid, sid);
		// Если флаг инициализации сессии HTTP/2 не установлен
		if(!this->_http2.is()){
			// Запоминаем идентификатор брокера
			this->_http1._bid = this->_bid;
			// Выполняем установку сетевого ядра
			this->_http1._core = this->_core;
			// Выполняем установку идентификатора объекта
			this->_http1._http.id(this->_bid);
			// Выполняем установку данных URL-адреса
			this->_http1._scheme.url = this->_scheme.url;
		}
		// Выполняем установку идентификатора объекта
		this->_ws2._http.id(bid);
		// Выполняем установку сетевого ядра
		this->_ws2._core = this->_core;
		// Выполняем установку сессии HTTP/2
		this->_ws2._http2 = this->_http2;
		// Выполняем установку данных URL-адреса
		this->_ws2._scheme.url = this->_scheme.url;
		// Если многопоточность активированна
		if(this->_threads > -1)
			// Выполняем инициализацию нового тредпула
			this->_ws2.multiThreads(this->_threads);
		// Если функция обратного вызова при подключении/отключении установлена
		if(this->_callbacks.is("active"))
			// Выполняем функцию обратного вызова
			this->_callbacks.call <void (const mode_t)> ("active", mode_t::CONNECT);
	}
}
/**
 * disconnectEvent Метод обратного вызова при отключении от сервера
 * @param bid идентификатор брокера
 * @param sid идентификатор схемы сети
 */
void awh::client::Http2::disconnectEvent(const uint64_t bid, const uint16_t sid) noexcept {
	// Выполняем удаление подключения
	this->_http2.close();
	// Выполняем установку сессии HTTP/2
	this->_ws2._http2 = nullptr;
	// Выполняем редирект, если редирект выполнен
	if(this->redirect(bid, sid))
		// Выходим из функции
		return;
	// Выполняем очистку списка редиректов
	this->_route.clear();
	// Выполняем очистку списка воркеров
	this->_workers.clear();
	// Выполняем очистку списка запросов
	this->_requests.clear();
	// Выполняем передачу сигнала отключения от сервера на Websocket-клиент
	this->_ws2.disconnectEvent(bid, sid);
	// Выполняем передачу сигнала отключения от сервера на HTTP/1.1 клиент
	this->_http1.disconnectEvent(bid, sid);
	// Если подключение не является постоянным
	if(!this->_scheme.alive){
		// Выполняем сброс параметров запроса
		this->flush();
		// Выполняем зануление идентификатора брокера
		this->_bid = 0;
		// Очищаем адрес сервера
		this->_scheme.url.clear();
		// Если завершить работу разрешено
		if(this->_complete && (this->_core != nullptr))
			// Завершаем работу
			const_cast <client::core_t *> (this->_core)->stop();
	}
	// Если функция обратного вызова при подключении/отключении установлена
	if(this->_callbacks.is("active"))
		// Выполняем функцию обратного вызова
		this->_callbacks.call <void (const mode_t)> ("active", mode_t::DISCONNECT);
}
/**
 * readEvent Метод обратного вызова при чтении сообщения с сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param bid    идентификатор брокера
 * @param sid    идентификатор схемы сети
 */
void awh::client::Http2::readEvent(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (bid > 0) && (sid > 0)){
		// Флаг выполнения обработки полученных данных
		bool process = false;
		// Если установлена функция обратного вызова для вывода данных в сыром виде
		if(!(process = !this->_callbacks.is("raw")))
			// Выполняем функцию обратного вызова
			process = this->_callbacks.call <bool (const char *, const size_t)> ("raw", buffer, size);
		// Если обработка полученных данных разрешена
		if(process){
			// Если протокол подключения является HTTP/2
			if(this->_core->proto(bid) == engine_t::proto_t::HTTP2){
				// Если прочитать данные фрейма не удалось, выходим из функции
				if(!this->_http2.frame(reinterpret_cast <const uint8_t *> (buffer), size)){
					// Выполняем закрытие подключения
					web2_t::close(bid);
					// Выходим из функции
					return;
				}
			// Если активирован режим работы с HTTP/1.1 протоколом
			} else {
				// Если активирован Websocket-клиент
				if(this->_ws2._bid > 0)
					// Выполняем переброс вызова чтения на клиент Websocket
					this->_ws2.readEvent(buffer, size, bid, sid);
				// Выполняем переброс вызова чтения на клиент HTTP/1.1
				else this->_http1.readEvent(buffer, size, bid, sid);
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
 */
void awh::client::Http2::writeCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные существуют
	if((bid > 0) && (sid > 0)){
		// Выполняем перебор всех доступных воркеров
		for(auto & worker : this->_workers){
			// Определяем протокол клиента
			switch(static_cast <uint8_t> (worker.second->agent)){
				// Если агент является клиентом HTTP
				case static_cast <uint8_t> (agent_t::HTTP): {
					// Если переключение протокола на HTTP/2 не выполнено
					if(worker.second->proto != engine_t::proto_t::HTTP2)
						// Выполняем переброс вызова записи на клиент HTTP/1.1
						this->_http1.writeCallback(buffer, size, bid, sid);
				} break;
				// Если агент является клиентом Websocket
				case static_cast <uint8_t> (agent_t::WEBSOCKET):
					// Выполняем переброс вызова записи на клиент Websocket
					this->_ws2.writeCallback(buffer, size, bid, sid);
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
int32_t awh::client::Http2::chunkSignal(const int32_t sid, const uint8_t * buffer, const size_t size) noexcept {
	// Если подключение производится через, прокси-сервер
	if(this->_scheme.isProxy())
		// Выполняем обработку полученных данных чанка для прокси-сервера
		return this->chunkProxySignal(sid, buffer, size);
	// Если мы работаем с сервером напрямую
	else {
		// Выполняем поиск идентификатора воркера
		auto i = this->_workers.find(sid);
		// Если необходимый нам воркер найден
		if(i != this->_workers.end()){
			// Если функция обратного вызова на перехват входящих чанков установлена
			if(this->_callbacks.is("chunking"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const uint64_t, const vector <char> &, const awh::http_t *)> ("chunking", i->second->id, vector <char> (buffer, buffer + size), &i->second->http);
			// Если функция перехвата полученных чанков не установлена
			else {
				// Определяем протокол клиента
				switch(static_cast <uint8_t> (i->second->agent)){
					// Если агент является клиентом HTTP
					case static_cast <uint8_t> (agent_t::HTTP): {
						// Добавляем полученный чанк в тело данных
						i->second->http.payload(vector <char> (buffer, buffer + size));
						// Обновляем время отправленного пинга
						this->_sendPing = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
						// Если функция обратного вызова на вывода полученного чанка бинарных данных с сервера установлена
						if(this->_callbacks.is("chunks"))
							// Выполняем функцию обратного вызова
							this->_callbacks.call <void (const int32_t, const uint64_t, const vector <char> &)> ("chunks", sid, i->second->id, vector <char> (buffer, buffer + size));
					} break;
					// Если агент является клиентом Websocket
					case static_cast <uint8_t> (agent_t::WEBSOCKET):
						// Выполняем передачу полученных данных на Websocket-клиент
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
 * @param flags  флаги полученного фрейма
 * @return       статус полученных данных
 */
int32_t awh::client::Http2::frameSignal(const int32_t sid, const awh::http2_t::direct_t direct, const awh::http2_t::frame_t frame, const set <awh::http2_t::flag_t> & flags) noexcept {
	// Определяем направление передачи фрейма
	switch(static_cast <uint8_t> (direct)){
		// Если производится передача фрейма на сервер
		case static_cast <uint8_t> (awh::http2_t::direct_t::SEND): {
			// Если мы получили флаг завершения потока
			if(flags.find(awh::http2_t::flag_t::END_STREAM) != flags.end()){
				// Выполняем поиск идентификатора воркера
				auto i = this->_workers.find(sid);
				// Если необходимый нам воркер найден
				if(i != this->_workers.end()){
					// Если агент является клиентом Websocket
					if(i->second->agent == agent_t::WEBSOCKET)
						// Выполняем передачу на Websocket-клиент
						this->_ws2.frameSignal(sid, direct, frame, flags);
					// Если установлена функция отлова завершения запроса
					if(this->_callbacks.is("end"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", sid, i->second->id, direct_t::SEND);
				}
			}
		} break;
		// Если производится получения фрейма с сервера
		case static_cast <uint8_t> (awh::http2_t::direct_t::RECV): {
			// Если подключение производится через, прокси-сервер
			if(this->_scheme.isProxy())
				// Выполняем обработку полученных данных фрейма для прокси-сервера
				return this->frameProxySignal(sid, direct, frame, flags);
			// Если мы работаем с сервером напрямую
			else if(this->_core != nullptr) {
				// Выполняем поиск идентификатора воркера
				auto i = this->_workers.find(sid);
				// Если необходимый нам воркер найден
				if(i != this->_workers.end()){
					// Выполняем определение типа фрейма
					switch(static_cast <uint8_t> (frame)){
						// Если производится сброс подключения
						case static_cast <uint8_t> (awh::http2_t::frame_t::RST_STREAM):
							// Выводим сообщение об ошибке
							this->_log->print("Connection was reset by the server", log_t::flag_t::WARNING);
						break;
						// Если получено push-уведомление от сервера
						case static_cast <uint8_t> (awh::http2_t::frame_t::PUSH_PROMISE): {
							// Если сессия клиента совпадает с сессией полученных даных и передача заголовков завершена
							if(flags.find(awh::http2_t::flag_t::END_HEADERS) != flags.end()){
								// Получаем данные запроса
								const auto & query = i->second->http.request();
								/**
								 * Если включён режим отладки
								 */
								#if defined(DEBUG_MODE)
									{
										// Получаем данные запроса
										const auto & request = i->second->http.process(http_t::process_t::REQUEST, query);
										// Если параметры запроса получены
										if(!request.empty())
											// Выводим параметры запроса
											cout << string(request.begin(), request.end()) << endl;
									}
								#endif
								// Если функция обратного вызова на вывода запроса клиента к серверу
								if(this->_callbacks.is("request"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &)> ("request", sid, i->second->id, query.method, query.url);
								// Если функция обратного вызова на вывода push-уведомления
								if(this->_callbacks.is("push"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> ("push", sid, i->second->id, query.method, query.url, i->second->http.headers());
							}
						} break;
						// Если мы получили входящие данные тела ответа
						case static_cast <uint8_t> (awh::http2_t::frame_t::DATA): {
							// Определяем протокол клиента
							switch(static_cast <uint8_t> (i->second->agent)){
								// Если агент является клиентом HTTP
								case static_cast <uint8_t> (agent_t::HTTP): {
									// Если мы получили флаг завершения потока
									if(flags.find(awh::http2_t::flag_t::END_STREAM) != flags.end()){
										// Выполняем фиксацию полученного результата
										i->second->http.commit();
										// Получаем идентификатор запроса
										const uint64_t rid = i->second->id;
										/**
										 * Если включён режим отладки
										 */
										#if defined(DEBUG_MODE)
											{
												// Если тело ответа существует
												if(!i->second->http.empty(awh::http_t::suite_t::BODY))
													// Выводим сообщение о выводе чанка тела
													cout << this->_fmk->format("<body %zu>", i->second->http.body().size()) << endl << endl;
												// Иначе устанавливаем перенос строки
												else cout << endl;
											}
										#endif
										// Выполняем препарирование полученных данных
										switch(static_cast <uint8_t> (this->prepare(sid, this->_bid))){
											// Если необходимо выполнить пропуск обработки данных
											case static_cast <uint8_t> (status_t::SKIP):
												// Завершаем работу
												return 0;
										}
										// Выполняем очистку параметров HTTP-запроса
										this->_http.clear();
										// Выполняем сброс состояния HTTP-парсера
										this->_http.reset();
										// Выполняем очистку параметров HTTP-запроса у конкретного потока
										i->second->http.clear();
										// Выполняем сброс состояния HTTP-запроса у конкретного потока
										i->second->http.reset();
										// Если функция обратного вызова на получение удачного ответа установлена
										if(this->_callbacks.is("handshake"))
											// Выполняем функцию обратного вызова
											this->_callbacks.call <void (const int32_t, const uint64_t, const agent_t)> ("handshake", sid, rid, i->second->agent);
										// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
										if(i->second->callback.is("entity"))
											// Выполняем функцию обратного вызова дисконнекта
											i->second->callback.bind("entity");
										// Если функция обратного вызова на вывод полученных данных ответа сервера установлена
										if(i->second->callback.is("complete"))
											// Выполняем функцию обратного вызова
											i->second->callback.bind("complete");
										// Выполняем завершение запроса
										this->result(sid, rid);
										// Если установлена функция отлова завершения запроса
										if(this->_callbacks.is("end"))
											// Выполняем функцию обратного вызова
											this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", sid, rid, direct_t::RECV);
										// Завершаем работу
										return 0;
									}
								} break;
								// Если агент является клиентом Websocket
								case static_cast <uint8_t> (agent_t::WEBSOCKET):
									// Выполняем передачу на Websocket-клиент
									return this->_ws2.frameSignal(sid, direct, frame, flags);
							}
						} break;
						// Если мы получили входящие данные заголовков ответа
						case static_cast <uint8_t> (awh::http2_t::frame_t::HEADERS): {
							// Определяем протокол клиента
							switch(static_cast <uint8_t> (i->second->agent)){
								// Если агент является клиентом HTTP
								case static_cast <uint8_t> (agent_t::HTTP): {
									// Если мы получили флаг завершения потока
									if(flags.find(awh::http2_t::flag_t::END_STREAM) != flags.end()){
										// Если функция обратного вызова на получение удачного ответа установлена
										if(this->_callbacks.is("handshake"))
											// Выполняем функцию обратного вызова
											this->_callbacks.call <void (const int32_t, const uint64_t, const agent_t)> ("handshake", sid, i->second->id, i->second->agent);
									}
									// Если сессия клиента совпадает с сессией полученных даных и передача заголовков завершена
									if(flags.find(awh::http2_t::flag_t::END_HEADERS) != flags.end()){
										// Флаг полученных трейлеров из ответа сервера
										bool trailers = false;
										// Если трейлеры получены в ответе сервера
										if((trailers = i->second->http.is(http_t::suite_t::HEADER, "trailer"))){
											// Получаем идентификатор запроса
											const uint64_t rid = i->second->id;
											// Выполняем извлечение списка заголовков трейлеров
											const auto & range = i->second->http.headers().equal_range("trailer");
											// Выполняем перебор всех полученных заголовков трейлеров
											for(auto j = range.first; j != range.second; ++j){
												// Если такой заголовок трейлера не получен
												if(!i->second->http.is(http_t::suite_t::HEADER, j->second)){
													// Если мы получили флаг завершения потока
													if(flags.find(awh::http2_t::flag_t::END_STREAM) != flags.end()){
														// Выводим сообщение об ошибке, что трейлер не существует
														this->_log->print("Trailer \"%s\" does not exist", log_t::flag_t::WARNING, j->second.c_str());
														// Если функция обратного вызова на на вывод ошибок установлена
														if(this->_callbacks.is("error"))
															// Выполняем функцию обратного вызова
															this->_callbacks.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::HTTP2_RECV, this->_fmk->format("Trailer \"%s\" does not exist", j->second.c_str()));
														// Выполняем завершение запроса
														this->result(sid, rid);
														// Если установлена функция отлова завершения запроса
														if(this->_callbacks.is("end"))
															// Выполняем функцию обратного вызова
															this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", sid, rid, direct_t::RECV);
													}
													// Завершаем работу
													return 0;
												}
											}
											// Выполняем удаление заголовка трейлеров
											i->second->http.rm(http_t::suite_t::HEADER, "trailer");
										}
										/**
										 * Если включён режим отладки
										 */
										#if defined(DEBUG_MODE)
											{
												// Получаем данные ответа
												const auto & response = i->second->http.process(http_t::process_t::RESPONSE, i->second->http.response());
												// Если параметры ответа получены
												if(!response.empty())
													// Выводим параметры ответа
													cout << string(response.begin(), response.end()) << endl;
											}
										#endif
										// Получаем параметры запроса
										const auto & response = i->second->http.response();
										// Если метод CONNECT запрещён для прокси-сервера
										if(this->_proxy.mode && !this->_proxy.connect){
											// Выполняем сброс заголовков прокси-сервера
											this->_scheme.proxy.http.clear();
											// Выполняем перебор всех полученных заголовков
											for(auto & item : i->second->http.headers()){
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
										// Если функция обратного вызова на вывод ответа сервера на ранее выполненный запрос установлена
										if(this->_callbacks.is("response"))
											// Выполняем функцию обратного вызова
											this->_callbacks.call <void (const int32_t, const uint64_t, const uint32_t, const string &)> ("response", sid, i->second->id, response.code, response.message);
										// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
										if(this->_callbacks.is("headers"))
											// Выполняем функцию обратного вызова
											this->_callbacks.call <void (const int32_t, const uint64_t, const uint32_t, const string &, const unordered_multimap <string, string> &)> ("headers", sid, i->second->id, response.code, response.message, i->second->http.headers());
										// Если трейлеры получены с сервера
										if(trailers)
											// Выполняем извлечение полученных данных полезной нагрузки
											return this->frameSignal(sid, awh::http2_t::direct_t::RECV, awh::http2_t::frame_t::DATA, flags);
										// Если мы получили флаг завершения потока
										else if(flags.find(awh::http2_t::flag_t::END_STREAM) != flags.end()) {
											// Выполняем фиксацию полученного результата
											i->second->http.commit();
											// Если функция обратного вызова на вывод полученных данных ответа сервера установлена
											if(this->_callbacks.is("complete"))
												// Выполняем функцию обратного вызова
												this->_callbacks.call <void (const int32_t, const uint64_t, const uint32_t, const string &, const vector <char> &, const unordered_multimap <string, string> &)> ("complete", sid, i->second->id, response.code, response.message, i->second->http.body(), i->second->http.headers());
											// Выполняем препарирование полученных данных
											switch(static_cast <uint8_t> (this->prepare(sid, this->_bid))){
												// Если необходимо выполнить пропуск обработки данных
												case static_cast <uint8_t> (status_t::SKIP):
													// Завершаем работу
													return 0;
											}
										}
									}
									// Если мы получили флаг завершения потока
									if(flags.find(awh::http2_t::flag_t::END_STREAM) != flags.end()){
										// Получаем идентификатор запроса
										const uint64_t rid = i->second->id;
										// Выполняем завершение запроса
										this->result(sid, rid);
										// Если установлена функция отлова завершения запроса
										if(this->_callbacks.is("end"))
											// Выполняем функцию обратного вызова
											this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", sid, rid, direct_t::RECV);
									}
									// Завершаем работу
									return 0;
								} break;
								// Если агент является клиентом Websocket
								case static_cast <uint8_t> (agent_t::WEBSOCKET):
									// Выполняем передачу на Websocket-клиент
									return this->_ws2.frameSignal(sid, direct, frame, flags);
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
 * closedSignal Метод завершения работы потока
 * @param sid   идентификатор потока
 * @param error флаг ошибки если присутствует
 * @return      статус полученных данных
 */
int32_t awh::client::Http2::closedSignal(const int32_t sid, const awh::http2_t::error_t error) noexcept {
	// Выполняем поиск идентификатора воркера
	auto i = this->_workers.find(sid);
	// Если необходимый нам воркер найден
	if(i != this->_workers.end()){
		// Если флаг инициализации сессии HTTP/2 установлен
		if((this->_core != nullptr) && (error != awh::http2_t::error_t::NONE) && this->_http2.is())
			// Выполняем закрытие подключения
			web2_t::close(this->_bid);
		// Если функция обратного вызова активности потока установлена
		if(this->_callbacks.is("stream"))
			// Выполняем функцию обратного вызова
			this->_callbacks.call <void (const int32_t, const uint64_t, const mode_t)> ("stream", sid, i->second->id, mode_t::CLOSE);
	// Если функция обратного вызова активности потока установлена
	} else if(this->_callbacks.is("stream"))
		// Выполняем функцию обратного вызова
		this->_callbacks.call <void (const int32_t, const uint64_t, const mode_t)> ("stream", sid, 0, mode_t::CLOSE);
	// Выводим результат
	return 0;
}
/**
 * beginSignal Метод начала получения фрейма заголовков HTTP/2 сервера
 * @param sid идентификатор потока
 * @return    статус полученных данных
 */
int32_t awh::client::Http2::beginSignal(const int32_t sid) noexcept {
	// Если подключение производится через, прокси-сервер
	if(this->_scheme.isProxy())
		// Выполняем обработку сигнала начала получения заголовков для прокси-сервера
		return this->beginProxySignal(sid);
	// Если мы работаем с сервером напрямую
	else {
		// Выполняем поиск идентификатора воркера
		auto i = this->_workers.find(sid);
		// Если необходимый нам воркер найден
		if(i != this->_workers.end()){
			// Определяем протокол клиента
			switch(static_cast <uint8_t> (i->second->agent)){
				// Если агент является клиентом HTTP
				case static_cast <uint8_t> (agent_t::HTTP): {
					// Выполняем очистку параметров HTTP-запроса
					i->second->http.clear();
					// Если функция обратного вызова активности потока установлена
					if(this->_callbacks.is("stream"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const int32_t, const uint64_t, const mode_t)> ("stream", sid, i->second->id, mode_t::OPEN);
				} break;
				// Если агент является клиентом Websocket
				case static_cast <uint8_t> (agent_t::WEBSOCKET):
					// Выполняем инициализации заголовков на Websocket-клиенте
					this->_ws2.beginSignal(sid);
				break;
			}
		}
	}
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
int32_t awh::client::Http2::headerSignal(const int32_t sid, const string & key, const string & val) noexcept {
	// Если подключение производится через, прокси-сервер
	if(this->_scheme.isProxy())
		// Выполняем обработку полученных заголовков для прокси-сервера
		return this->headerProxySignal(sid, key, val);
	// Если мы работаем с сервером напрямую
	else {
		// Выполняем поиск идентификатора воркера
		auto i = this->_workers.find(sid);
		// Если необходимый нам воркер найден
		if(i != this->_workers.end()){
			// Определяем протокол клиента
			switch(static_cast <uint8_t> (i->second->agent)){
				// Если агент является клиентом HTTP
				case static_cast <uint8_t> (agent_t::HTTP): {
					// Устанавливаем полученные заголовки
					i->second->http.header2(key, val);
					// Если функция обратного вызова на полученного заголовка с сервера установлена
					if(this->_callbacks.is("header"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const int32_t, const uint64_t, const string &, const string &)> ("header", sid, i->second->id, key, val);
				} break;
				// Если агент является клиентом Websocket
				case static_cast <uint8_t> (agent_t::WEBSOCKET):
					// Выполняем отправку полученных заголовков на Websocket-клиент
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
 * @param rid    идентификатор запроса
 * @param direct направление передачи данных
 */
void awh::client::Http2::end(const int32_t sid, const uint64_t rid, const direct_t direct) noexcept {
	// Определяем направление передачи данных
	switch(static_cast <uint8_t> (direct)){
		// Если направление передачи данных отправка на сервер
		case static_cast <uint8_t> (direct_t::SEND):
			/** Здесь мы пока ничего не выполняем **/
		break;
		// Если направление передачи данных получение с сервера
		case static_cast <uint8_t> (direct_t::RECV):
			// Выполняем завершение запроса
			this->result(sid, rid);
		break;
	}
	// Если установлена функция отлова завершения запроса
	if(this->_callbacks.is("end"))
		// Выполняем функцию обратного вызова
		this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", sid, rid, direct);
}
/**
 * answer Метод получение статуса ответа сервера
 * @param sid    идентификатор потока
 * @param rid    идентификатор запроса
 * @param status статус ответа сервера
 */
void awh::client::Http2::answer(const int32_t sid, const uint64_t rid, const awh::http_t::status_t status) noexcept {
	// Если статус входящего сообщения является положительным
	if(status == awh::http_t::status_t::GOOD)
		// Выполняем сброс количества попыток
		this->_attempt = 0;
	// Если функция обратного вызова получения статуса ответа установлена
	if(this->_callbacks.is("answer"))
		// Выполняем функцию обратного вызова
		this->_callbacks.call <void (const int32_t, const uint64_t, const awh::http_t::status_t)> ("answer", sid, rid, status);
}
/**
 * redirect Метод выполнения смены потоков
 * @param from идентификатор предыдущего потока
 * @param to   идентификатор нового потока
 */
void awh::client::Http2::redirect(const int32_t from, const int32_t to) noexcept {
	// Выполняем поиск воркера предыдущего потока
	auto i = this->_workers.find(from);
	// Если воркер для предыдущего потока найден
	if(i != this->_workers.end()){
		// Выполняем установку объекта воркера
		auto ret = this->_workers.emplace(to, unique_ptr <worker_t> (new worker_t(this->_fmk, this->_log)));
		// Выполняем очистку параметров HTTP-запроса
		ret.first->second->http.clear();
		// Выполняем сброс состояния HTTP-парсера
		ret.first->second->http.reset();
		// Выполняем установку типа агента
		ret.first->second->agent = i->second->agent;
		// Выполняем установку активный прототип интернета
		ret.first->second->proto = i->second->proto;
		// Выполняем установку флага обновления данных
		ret.first->second->update = i->second->update;
	}
	// Если функция обратного вызова на вывод редиректа потоков установлена
	if((from != to) && this->_callbacks.is("redirect"))
		// Выполняем функцию обратного вызова
		this->_callbacks.call <void (const int32_t, const int32_t)> ("redirect", from, to);
}
/**
 * redirect Метод выполнения редиректа если требуется
 * @param bid идентификатор брокера
 * @param sid идентификатор схемы сети
 * @return    результат выполнения редиректа
 */
bool awh::client::Http2::redirect(const uint64_t bid, const uint16_t sid) noexcept {
	// Результат работы функции
	bool result = false;
	// Если список ответов получен
	if(this->_redirects && !this->_stopped && !this->_requests.empty() && !this->_workers.empty()){
		// Выполняем поиск активного воркера который необходимо перезапустить
		for(auto i = this->_workers.begin(); i != this->_workers.end(); ++i){
			// Определяем тип агента
			switch(static_cast <uint8_t> (i->second->agent)){
				// Если протоколом агента является HTTP-клиент
				case static_cast <uint8_t> (agent_t::HTTP): {
					// Если протокол подключения установлен как HTTP/2
					if(i->second->proto == engine_t::proto_t::HTTP2){
						// Если мы нашли нужный нам воркер
						if(i->second->update){
							// Если список ответов получен
							if((result = !this->_stopped)){
								// Получаем параметры запроса
								const auto & response = i->second->http.response();
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
								if((result = i->second->http.is(http_t::suite_t::HEADER, "location"))){
									// Получаем новый адрес запроса
									const uri_t::url_t & url = i->second->http.url();
									// Если адрес запроса получен
									if((result = !url.empty())){
										// Увеличиваем количество попыток
										this->_attempt++;
										// Устанавливаем новый адрес запроса
										this->_uri.combine(this->_scheme.url, url);
										// Выполняем поиск параметров запроса
										auto j = this->_requests.find(i->first);
										// Если необходимые нам параметры запроса найдены
										if((result = (j != this->_requests.end()))){
											// Устанавливаем новый адрес запроса
											j->second->url = this->_scheme.url;
											// Если необходимо метод изменить на GET и основной метод не является GET
											if(((response.code == 201) || (response.code == 303)) && (j->second->method != awh::web_t::method_t::GET)){
												// Выполняем очистку тела запроса
												j->second->entity.clear();
												// Выполняем установку метода запроса
												j->second->method = awh::web_t::method_t::GET;
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
						this->_http1.disconnectEvent(bid, sid);
						// Если список ответов получен
						if((result = i->second->update = !this->_http1._stopped)){
							// Получаем параметры запроса
							const auto & response = this->_http1._http.response();
							// Если необходимо выполнить ещё одну попытку выполнения авторизации
							if((result = i->second->update = (this->_proxy.answer == 407) || (response.code == 401) || (response.code == 407))){
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
							if((result = i->second->update = this->_http1._http.is(http_t::suite_t::HEADER, "location"))){
								// Выполняем очистку оставшихся данных
								this->_http1._buffer.clear();
								// Получаем новый адрес запроса
								const uri_t::url_t & url = this->_http1._http.url();
								// Если адрес запроса получен
								if((result = i->second->update = !url.empty())){
									// Получаем количество попыток
									this->_attempt = this->_http1._attempt;
									// Устанавливаем новый адрес запроса
									this->_uri.combine(this->_scheme.url, url);
									// Выполняем поиск параметров запроса
									auto j = this->_requests.find(i->first);
									// Если необходимые нам параметры запроса найдены
									if((result = i->second->update = (j != this->_requests.end()))){
										// Устанавливаем новый адрес запроса
										j->second->url = this->_scheme.url;
										// Если необходимо метод изменить на GET и основной метод не является GET
										if(((response.code == 201) || (response.code == 303)) && (j->second->method != awh::web_t::method_t::GET)){
											// Выполняем очистку тела запроса
											j->second->entity.clear();
											// Выполняем установку метода запроса
											j->second->method = awh::web_t::method_t::GET;
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
				// Если протоколом агента является Websocket-клиент
				case static_cast <uint8_t> (agent_t::WEBSOCKET): {
					// Выполняем переброс вызова дисконнекта на клиент Websocket
					this->_ws2.disconnectEvent(bid, sid);
					// Если список ответов получен
					if((result = i->second->update = !this->_ws2._stopped)){
						// Получаем параметры запроса
						const auto & response = this->_ws2._http.response();
						// Если необходимо выполнить ещё одну попытку выполнения авторизации
						if((result = i->second->update = (this->_proxy.answer == 407) || (response.code == 401) || (response.code == 407))){
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
						if((result = i->second->update = this->_ws2._http.is(http_t::suite_t::HEADER, "location"))){
							// Выполняем очистку оставшихся данных
							this->_ws2._buffer.clear();
							// Получаем новый адрес запроса
							const uri_t::url_t & url = this->_ws2._http.url();
							// Если адрес запроса получен
							if((result = i->second->update = !url.empty())){
								// Получаем количество попыток
								this->_attempt = this->_ws2._attempt;
								// Устанавливаем новый адрес запроса
								this->_uri.combine(this->_scheme.url, url);
								// Выполняем поиск параметров запроса
								auto j = this->_requests.find(i->first);
								// Если необходимые нам параметры запроса найдены
								if((result = i->second->update = (j != this->_requests.end())))
									// Устанавливаем новый адрес запроса
									j->second->url = this->_scheme.url;
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
 * eventCallback Метод отлавливания событий контейнера функций обратного вызова
 * @param event событие контейнера функций обратного вызова
 * @param idw   идентификатор функции обратного вызова
 * @param name  название функции обратного вызова
 * @param dump  дамп данных функции обратного вызова
 */
void awh::client::Http2::eventCallback(const fn_t::event_t event, const uint64_t idw, const string & name, const fn_t::dump_t * dump) noexcept {
	// Определяем входящее событие контейнера функций обратного вызова
	switch(static_cast <uint8_t> (event)){
		// Если событием является установка функции обратного вызова
		case static_cast <uint8_t> (fn_t::event_t::SET): {
			// Если переменная не является закрытием потока, редиректом и не является событием подключения
			if((dump != nullptr) && !this->_fmk->compare(name, "end") && !this->_fmk->compare(name, "redirect") && !this->_fmk->compare(name, "active")){
				// Создаём локальный контейнер функций обратного вызова
				fn_t callbacks(this->_log);
				// Выполняем установку функции обратного вызова
				callbacks.dump(idw, * dump);
				// Если функции обратного вызова установлены
				if(!callbacks.empty()){
					// Выполняем установку функций обратного вызова для Websocket-клиента
					this->_ws2.callbacks(callbacks);
					// Выполняем установку функции обратного вызова для HTTP-клиента
					this->_http1.callbacks(std::move(callbacks));
				}
			}
		} break;
	}
}
/**
 * flush Метод сброса параметров запроса
 */
void awh::client::Http2::flush() noexcept {
	// Разрешаем чтение данных из буфера
	this->_reading = true;
	// Снимаем флаг принудительной остановки
	this->_stopped = false;
}
/**
 * result Метод завершения выполнения запроса
 * @param sid идентификатор потока
 * @param rid идентификатор запроса
 */
void awh::client::Http2::result(const int32_t sid, const uint64_t rid) noexcept {
	// Выполняем поиск идентификатора воркера
	auto i = this->_workers.find(sid);
	// Если необходимый нам воркер найден
	if(i != this->_workers.end()){
		// Выполняем удаление выполненного воркера
		this->_workers.erase(i);
		// Выполняем поиск идентификатора запроса
		auto j = this->_requests.find(sid);
		// Если необходимый нам запрос найден
		if(j != this->_requests.end())
			// Выполняем удаление параметров запроса
			this->_requests.erase(j);
		// Если функция обратного вызова при завершении запроса установлена
		if(this->_callbacks.is("result"))
			// Выполняем функцию обратного вызова
			this->_callbacks.call <void (const int32_t, const uint64_t)> ("result", sid, rid);
	}
}
/**
 * pinging Метод таймера выполнения пинга удалённого сервера
 * @param tid идентификатор таймера
 */
void awh::client::Http2::pinging(const uint16_t tid) noexcept {
	// Если данные существуют
	if((tid > 0) && (this->_core != nullptr)){
		// Если разрешено выполнять пинги
		if(this->_pinging){
			// Выполняем перебор всех доступных воркеров
			for(auto & worker : this->_workers){
				// Определяем протокол клиента
				switch(static_cast <uint8_t> (worker.second->agent)){
					// Если агент является клиентом HTTP
					case static_cast <uint8_t> (agent_t::HTTP): {
						// Получаем текущий штамп времени
						const time_t stamp = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
						// Если время с предыдущего пинга прошло больше половины времени пинга
						if((stamp - this->_sendPing) > (PING_INTERVAL / 2)){
							// Если переключение протокола на HTTP/2 выполнено и пинг не прошёл
							if(!this->ping())
								// Выполняем закрытие подключения
								web2_t::close(this->_bid);
							// Обновляем время отправленного пинга
							else this->_sendPing = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
						}
					} break;
					// Если агент является клиентом Websocket
					case static_cast <uint8_t> (agent_t::WEBSOCKET):
						// Выполняем переброс персистентного вызова на клиент Websocket
						this->_ws2.pinging(tid);
					break;
				}
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
		for(auto i = this->_workers.begin(); i != this->_workers.end(); ++i){
			// Если мы нашли нужный нам воркер
			if(i->second->update){
				// Выполняем поиск параметров запроса
				auto j = this->_requests.find(i->first);
				// Если необходимые нам параметры запроса найдены
				if(j != this->_requests.end()){
					// Устанавливаем текущий идентификатор
					result = j->first;
					// Если объект запроса не является одним и тем же
					if(dynamic_cast <request_t *> (&request) != j->second.get()){
						// Выполняем копирование метода запроса
						request.method = j->second->method;
						// Если список заголовков получен
						if(!j->second->headers.empty())
							// Выполняем установку заголовков запроса
							request.headers = j->second->headers;
						// Выполняем очистку полученных заголовков
						else request.headers.clear();
						// Получаем адрес URL-запроса
						const string & url = this->_uri.url(j->second->url);
						// Выполняем поиск активного редиректа
						auto k = this->_route.find(url);
						// Если активный редирект найден
						if(k != this->_route.end()){
							// Если хост запроса не установлен
							if(request.url.host.empty())
								// Выполняем установку адреса URL-запроса
								this->_uri.create(request.url, j->second->url);
							// Если адреса перенаправлений отличаются
							else if(!this->_fmk->compare(k->second, this->_uri.url(request.url))) {
								// Получаем адрес URL-запроса
								const string & url = this->_uri.url(request.url);
								// Выполняем установку IP-адреса
								request.url.ip = j->second->url.ip;
								// Выполняем установку порта сервера
								request.url.port = j->second->url.port;
								// Выполняем установку хоста сервера
								request.url.host = j->second->url.host;
								// Выполняем установку схемы протокола
								request.url.schema = j->second->url.schema;
								// Выполняем установку доменного имени
								request.url.domain = j->second->url.domain;
								// Выполняем добавление активного редиректа
								this->_route.emplace(this->_uri.url(request.url), url);
							// Выполняем замену адреса запроса
							} else request.url = j->second->url;
						// Если активный редирект ещё не выполнялся раньше
						} else {
							// Выполняем добавление активного редиректа
							this->_route.emplace(url, this->_uri.url(request.url));
							// Выполняем замену адреса запроса
							request.url = j->second->url;
						}
						// Выполняем поиск заголовка хоста
						for(auto i = request.headers.begin(); i != request.headers.end();){
							// Если заголовок хоста найден
							if(this->_fmk->compare("host", i->first)){
								// Выполняем удаление заголовка
								request.headers.erase(i);
								// Выходим из цикла
								break;
							// Продолжаем перебор заголовков дальше
							} else ++i;
						}
						// Если тело запроса существует
						if(!j->second->entity.empty())
							// Устанавливаем тело запроса
							request.entity.assign(j->second->entity.begin(), j->second->entity.end());
						// Выполняем очистку полученных данных тела запроса
						else request.entity.clear();
					}
					// Выполняем извлечение полученных данных запроса
					i->second->http.mapping(http_t::process_t::REQUEST, this->_http);
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
 * @param sid идентификатор запроса
 * @param bid идентификатор брокера
 * @return    результат препарирования
 */
awh::client::Web::status_t awh::client::Http2::prepare(const int32_t sid, const uint64_t bid) noexcept {
	// Выполняем поиск текущего воркера
	auto i = this->_workers.find(sid);
	// Если искомый воркер найден
	if(i != this->_workers.end()){
		// Получаем параметры запроса
		const auto & response = i->second->http.response();
		// Получаем статус ответа
		awh::http_t::status_t status = i->second->http.auth();
		// Если выполнять редиректы запрещено
		if(!this->_redirects && (status == awh::http_t::status_t::RETRY)){
			// Если нужно произвести запрос заново
			if((response.code == 201) || (response.code == 301) ||
			   (response.code == 302) || (response.code == 303) ||
			   (response.code == 307) || (response.code == 308))
					// Запрещаем выполнять редирект
					status = awh::http_t::status_t::GOOD;
		}
		// Если функция обратного вызова получения статуса ответа установлена
		if(this->_callbacks.is("answer"))
			// Выполняем функцию обратного вызова
			this->_callbacks.call <void (const int32_t, const uint64_t, const awh::http_t::status_t)> ("answer", sid, i->second->id, status);
		// Выполняем анализ результата авторизации
		switch(static_cast <uint8_t> (status)){
			// Если нужно попытаться ещё раз
			case static_cast <uint8_t> (awh::http_t::status_t::RETRY): {
				// Если функция обратного вызова на на вывод ошибок установлена
				if((response.code == 401) && this->_callbacks.is("error"))
					// Выполняем функцию обратного вызова
					this->_callbacks.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_RECV, "authorization failed");
				// Если попытки повторить переадресацию ещё не закончились
				if(!(this->_stopped = (this->_attempt >= this->_attempts))){
					// Выполняем поиск параметров запроса
					auto j = this->_requests.find(sid);
					// Если параметры запроса получены
					if((i->second->update = (j != this->_requests.end()))){
						// Получаем новый адрес запроса
						const uri_t::url_t & url = i->second->http.url();
						// Если адрес запроса получен
						if(!url.empty()){
							// Выполняем проверку соответствие протоколов
							const bool schema = (
								(this->_fmk->compare(url.host, j->second->url.host)) &&
								(this->_fmk->compare(url.schema, j->second->url.schema))
							);
							// Если соединение является постоянным
							if(schema && i->second->http.is(http_t::state_t::ALIVE)){
								// Увеличиваем количество попыток
								this->_attempt++;
								// Устанавливаем новый адрес запроса
								this->_uri.combine(j->second->url, url);
								// Отправляем повторный запрос
								this->send(* j->second.get());
								// Завершаем работу
								return status_t::SKIP;
							}
							// Выполняем закрытие подключения
							web2_t::close(bid);
							// Завершаем работу
							return status_t::SKIP;
						// Если URL-адрес запроса не получен
						} else {
							// Если соединение является постоянным
							if(i->second->http.is(http_t::state_t::ALIVE)){
								// Увеличиваем количество попыток
								this->_attempt++;
								// Отправляем повторный запрос
								this->send(* j->second.get());
								// Завершаем работу
								return status_t::SKIP;
							}
							// Выполняем закрытие подключения
							web2_t::close(bid);
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
				if(!i->second->http.empty(awh::http_t::suite_t::BODY) && this->_callbacks.is("entity"))
					// Устанавливаем полученную функцию обратного вызова
					i->second->callback.set <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>)> ("entity", this->_callbacks.get <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>)> ("entity"), sid, i->second->id, response.code, response.message, i->second->http.body());
				// Если функция обратного вызова на вывод полученных данных ответа сервера установлена
				if(this->_callbacks.is("complete"))
					// Выполняем функцию обратного вызова
					i->second->callback.set <void (const int32_t, const uint64_t, const uint32_t, const string &, const vector <char> &, const unordered_multimap <string, string> &)> ("complete", this->_callbacks.get <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>, const unordered_multimap <string, string> &)> ("complete"), sid, i->second->id, response.code, response.message, i->second->http.body(), i->second->http.headers());
				// Устанавливаем размер стопбайт
				if(!i->second->http.is(http_t::state_t::ALIVE)){
					// Выполняем закрытие подключения
					web2_t::close(bid);
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
				if(this->_callbacks.is("error"))
					// Выполняем функцию обратного вызова
					this->_callbacks.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_RECV, this->_http.message(response.code).c_str());
				// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
				if(!i->second->http.empty(awh::http_t::suite_t::BODY) && this->_callbacks.is("entity"))
					// Устанавливаем полученную функцию обратного вызова
					i->second->callback.set <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>)> ("entity", this->_callbacks.get <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>)> ("entity"), sid, i->second->id, response.code, response.message, i->second->http.body());
				// Если функция обратного вызова на вывод полученных данных ответа сервера установлена
				if(this->_callbacks.is("complete"))
					// Выполняем функцию обратного вызова
					i->second->callback.set <void (const int32_t, const uint64_t, const uint32_t, const string &, const vector <char> &, const unordered_multimap <string, string> &)> ("complete", this->_callbacks.get <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>, const unordered_multimap <string, string> &)> ("complete"), sid, i->second->id, response.code, response.message, i->second->http.body(), i->second->http.headers());
				// Завершаем обработку
				return status_t::NEXT;
			} break;
		}
		// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
		if(!i->second->http.empty(awh::http_t::suite_t::BODY) && this->_callbacks.is("entity"))
			// Устанавливаем полученную функцию обратного вызова
			i->second->callback.set <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>)> ("entity", this->_callbacks.get <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>)> ("entity"), sid, i->second->id, response.code, response.message, i->second->http.body());
		// Если функция обратного вызова на вывод полученных данных ответа сервера установлена
		if(this->_callbacks.is("complete"))
			// Выполняем функцию обратного вызова
			i->second->callback.set <void (const int32_t, const uint64_t, const uint32_t, const string &, const vector <char> &, const unordered_multimap <string, string> &)> ("complete", this->_callbacks.get <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>, const unordered_multimap <string, string> &)> ("complete"), sid, i->second->id, response.code, response.message, i->second->http.body(), i->second->http.headers());
		// Выполняем закрытие подключения
		web2_t::close(bid);
	}
	// Выполняем завершение работы
	return status_t::STOP;
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
			// Если найден воркер Websocket-клиента
			if(worker.second->agent == agent_t::WEBSOCKET){
				// Выполняем отправку сообщения ошибки на Websocket-сервер
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
 * @return        результат отправки сообщения
 */
bool awh::client::Http2::sendMessage(const vector <char> & message, const bool text) noexcept {
	// Если список воркеров активен
	if(!this->_workers.empty()){
		// Выполняем перебор всего списка воркеров
		for(auto & worker : this->_workers){
			// Если найден воркер Websocket-клиента
			if(worker.second->agent == agent_t::WEBSOCKET)
				// Выполняем отправку сообщения на Websocket-сервер
				return this->_ws2.sendMessage(message, text);
		}
	}
	// Сообщаем что ничего не найдено
	return false;
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
			// Если идентификатор запроса не установлен
			if(request.id == 0)
				// Выполняем генерацию идентификатора запроса
				const_cast <request_t &> (request).id = this->_fmk->timestamp(fmk_t::stamp_t::NANOSECONDS);
			// Если требуется выполнить подключение к Websocket-клиенту
			if(request.agent == agent_t::WEBSOCKET){
				// Если протокол Websocket запрещён
				if(!this->_webSocket){
					// Выводим сообщение об ошибке
					this->_log->print("Websocket protocol is prohibited for connection", log_t::flag_t::WARNING);
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callbacks.is("error"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::HTTP1_SEND, "Websocket protocol is prohibited for connection");
					// Если флаг инициализации сессии HTTP/2 установлен
					if(this->_http2.is())
						// Выполняем закрытие подключения
						web2_t::close(this->_bid);
					// Выполняем отключение в обычном режиме
					else const_cast <client::core_t *> (this->_core)->close(this->_bid);
					// Выходим из функции
					return sid;
				}
			}
			// Определяем тип агента
			switch(static_cast <uint8_t> (request.agent)){
				// Если протоколом агента является HTTP-клиент
				case static_cast <uint8_t> (agent_t::HTTP): {
					// Если флаг инициализации сессии HTTP/2 установлен
					if(this->_http2.is()){
						// Выполняем очистку параметров HTTP-запроса
						this->_http.clear();
						// Выполняем сброс состояния HTTP-парсера
						this->_http.reset();
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
							}
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								// Выводим заголовок запроса
								cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
								// Получаем бинарные данные HTTP-запроса
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
							if((sid > -1) && (sid != result) && this->_callbacks.is("redirect"))
								// Выполняем функцию обратного вызова
								this->_callbacks.call <void (const int32_t, const int32_t)> ("redirect", sid, result);
							// Если тело запроса существует
							if(!request.entity.empty()){
								// Тело HTTP-запроса
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
									if(this->_http.empty(awh::http_t::suite_t::BODY))
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
						// Если количество попыток ещё достаточно
						if(this->_attempt < this->_attempts){
							// Увеличиваем количество попыток
							this->_attempt++;
							// Если в списке больше запросов нет
							if(this->_http1._requests.empty())
								// Выполняем сброс количества попыток в модуле протокола HTTP/1.1
								this->_http1._attempt = 0;
							// Если нужно удалить предыдущие коннекты
							if(sid > 0){
								// Выполняем удаление указанного воркера
								this->_workers.erase(sid);
								// Выполняем удаление параметра запроса
								this->_requests.erase(sid);
							}
							// Если активирован режим прокси-сервера
							if(this->_proxy.mode){
								// Создаём объек запроса
								awh::web_t::req_t query(request.method, this->_scheme.url);
								// Выполняем извлечение заголовка авторизации на прокси-сервера
								const string & header = this->_scheme.proxy.http.auth(http_t::process_t::REQUEST, query);
								// Если заголовок авторизации получен
								if(!header.empty())
									// Выполняем сброс заголовков прокси-сервера
									this->_http1._scheme.proxy.http.dataAuth(this->_scheme.proxy.http.dataAuth());
								// Если заголовок параметров подключения не установлен
								if(request.headers.find("Proxy-Connection") == request.headers.end()){
									// Если установлено постоянное подключение к прокси-серверу
									if(this->_scheme.proxy.http.is(http_t::state_t::ALIVE))
										// Устанавливаем постоянное подключение к прокси-серверу
										const_cast <unordered_multimap <string, string> &> (request.headers).emplace("Proxy-Connection", "keep-alive");
									// Устанавливаем закрытие подключения к прокси-серверу
									else const_cast <unordered_multimap <string, string> &> (request.headers).emplace("Proxy-Connection", "close");
								}
							}
						// Если попытки исчерпаны, выходим из функции
						} else return result;
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
				// Если протоколом агента является Websocket-клиент
				case static_cast <uint8_t> (agent_t::WEBSOCKET): {
					// Устанавливаем идентификатор запроса
					this->_ws2._rid = request.id;
					// Выполняем обновление полученных данных, с целью выполнения редиректа если требуется
					sid = this->update(* const_cast <request_t *> (&request));
					// Если активирован режим прокси-сервера
					if(this->_proxy.mode){
						// Создаём объек запроса
						awh::web_t::req_t query(request.method, this->_scheme.url);
						// Выполняем извлечение заголовка авторизации на прокси-сервера
						const string & header = this->_scheme.proxy.http.auth(http_t::process_t::REQUEST, query);
						// Если заголовок авторизации получен
						if(!header.empty())
							// Выполняем сброс заголовков прокси-сервера
							this->_ws2._scheme.proxy.http.dataAuth(this->_scheme.proxy.http.dataAuth());
						// Если заголовок параметров подключения не установлен
						if(request.headers.find("Proxy-Connection") == request.headers.end()){
							// Если установлено постоянное подключение к прокси-серверу
							if(this->_scheme.proxy.http.is(http_t::state_t::ALIVE))
								// Устанавливаем постоянное подключение к прокси-серверу
								const_cast <unordered_multimap <string, string> &> (request.headers).emplace("Proxy-Connection", "keep-alive");
							// Устанавливаем закрытие подключения к прокси-серверу
							else const_cast <unordered_multimap <string, string> &> (request.headers).emplace("Proxy-Connection", "close");
						}
					}
					// Если флаг инициализации сессии HTTP/2 установлен
					if(this->_http2.is()){
						// Если протокол ещё не установлен
						if(request.headers.find(":protocol") == request.headers.end())
							// Выполняем установку протокола Websocket
							const_cast <request_t &> (request).headers.emplace(":protocol", "websocket");
					}
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
					// Выполняем установку подключения с Websocket-сервером
					this->_ws2.connectEvent(this->_bid, this->_scheme.id);
					// Выводим идентификатор подключения
					result = this->_ws2._sid;
				} break;
			}
			// Если идентификатор подключения получен
			if(result > 0){
				{
					// Добавляем полученный запрос в список запросов
					auto ret = this->_requests.emplace(result, unique_ptr <request_t> (new request_t));
					// Выполняем установку идентификатора запроса
					ret.first->second->id = request.id;
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
					// Выполняем установку идентификатора запроса
					ret.first->second->id = request.id;
					// Выполняем установку типа агента
					ret.first->second->agent = request.agent;
					// Выполняем очистку параметров HTTP-запроса
					ret.first->second->http.clear();
					// Выполняем сброс состояния HTTP-парсера
					ret.first->second->http.reset();
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
					// Если функция обратного вызова на событие получения ошибок установлена
					if(this->_callbacks.is("error"))
						// Устанавливаем функцию обработки вызова на событие получения ошибок
						ret.first->second->http.callback <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", std::bind(&http2_t::errors, this, _1, _2, _3, _4));
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
 * send Метод отправки данных в бинарном виде серверу
 * @param buffer буфер бинарных данных передаваемых серверу
 * @param size   размер сообщения в байтах
 * @return       результат отправки сообщения
 */
bool awh::client::Http2::send(const char * buffer, const size_t size) noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working() && (buffer != nullptr) && (size > 0))
		// Выполняем отправку заголовков запроса серверу
		return const_cast <client::core_t *> (this->_core)->send(buffer, size, this->_bid);
	// Сообщаем что ничего не найдено
	return false;
}
/**
 * send Метод отправки тела сообщения на сервер
 * @param sid    идентификатор потока HTTP
 * @param buffer буфер бинарных данных передаваемых на сервер
 * @param size   размер сообщения в байтах
 * @param end    флаг последнего сообщения после которого поток закрывается
 * @return       результат отправки данных указанному клиенту
 */
bool awh::client::Http2::send(const int32_t sid, const char * buffer, const size_t size, const bool end) noexcept {
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
					if(end && this->_http.empty(awh::http_t::suite_t::BODY))
						// Устанавливаем флаг завершения потока
						flag = awh::http2_t::flag_t::END_STREAM;
					// Выполняем отправку данных на удалённый сервер
					result = web2_t::send(sid, entity.data(), entity.size(), flag);
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
 * @param sid     идентификатор потока HTTP
 * @param url     адрес запроса на сервере
 * @param method  метод запроса на сервере
 * @param headers заголовки отправляемые на сервер
 * @param end     размер сообщения в байтах
 * @return        идентификатор нового запроса
 */
int32_t awh::client::Http2::send(const int32_t sid, const uri_t::url_t & url, const awh::web_t::method_t method, const unordered_multimap <string, string> & headers, const bool end) noexcept {
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
				// Выполняем очистку параметров HTTP-запроса
				this->_http.clear();
				// Выполняем сброс состояния HTTP-парсера
				this->_http.reset();
				// Устанавливаем заголовоки запроса
				this->_http.headers(headers);
				// Устанавливаем новый адрес запроса
				this->_uri.combine(this->_scheme.url, url);
				// Создаём объек запроса
				awh::web_t::req_t query(2.0f, method, this->_scheme.url);
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
						// Получаем бинарные данные HTTP-запроса
						const auto & buffer = this->_http.process(http_t::process_t::REQUEST, query);
						// Выводим параметры запроса
						cout << string(buffer.begin(), buffer.end()) << endl << endl;
					#endif
					// Выполняем заголовки запроса на сервер
					result = web2_t::send(sid, headers, (end ? awh::http2_t::flag_t::END_STREAM : awh::http2_t::flag_t::NONE));
				}
			// Если протокол HTTP/2 не активирован, передаём запрос через протокол HTTP/1.1
			} else result = this->_http1.send(url, method, headers, end);
		}
	}
	// Выводим значение по умолчанию
	return result;
}
/**
 * send2 Метод HTTP/2 отправки сообщения на сервер
 * @param sid    идентификатор потока HTTP/2
 * @param buffer буфер бинарных данных передаваемых на сервер
 * @param size   размер сообщения в байтах
 * @param flag   флаг передаваемого потока по сети
 * @return       результат отправки данных указанному клиенту
 */
bool awh::client::Http2::send2(const int32_t sid, const char * buffer, const size_t size, const awh::http2_t::flag_t flag) noexcept {
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::READ, event_t::CONNECT}, event_t::SEND)){
		// Если заголовки запроса переданы и флаг инициализации сессии HTTP/2 установлен
		if((buffer != nullptr) && (size > 0) && this->_http2.is())
			// Выполняем отправку сообщения на сервер
			return web2_t::send(sid, buffer, size, flag);
	}
	// Выводим результат
	return false;
}
/**
 * send2 Метод HTTP/2 отправки заголовков на сервер
 * @param sid     идентификатор потока HTTP/2
 * @param headers заголовки отправляемые на сервер
 * @param flag    флаг передаваемого потока по сети
 * @return        идентификатор нового запроса
 */
int32_t awh::client::Http2::send2(const int32_t sid, const vector <pair <string, string>> & headers, const awh::http2_t::flag_t flag) noexcept {
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::READ, event_t::CONNECT}, event_t::SEND)){
		// Если заголовки запроса переданы и флаг инициализации сессии HTTP/2 установлен
		if(!headers.empty() && this->_http2.is())
			// Выполняем отправку заголовков на сервер
			return web2_t::send(sid, headers, flag);
	}
	// Выводим результат
	return -1;
}
/**
 * pause Метод установки на паузу клиента
 */
void awh::client::Http2::pause() noexcept {
	// Если список воркеров активен
	if(!this->_workers.empty()){
		// Выполняем перебор всего списка воркеров
		for(auto & worker : this->_workers){
			// Если найден воркер Websocket-клиента
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
 * waitPong Метод установки времени ожидания ответа WebSocket-сервера
 * @param time время ожидания в миллисекундах
 */
void awh::client::Http2::waitPong(const time_t time) noexcept {
	// Если время ожидания передано
	if(time > 0)
		// Выполняем установку времени ожидания
		this->_ws2.waitPong(time);
}
/**
 * callbacks Метод установки функций обратного вызова
 * @param callbacks функции обратного вызова
 */
void awh::client::Http2::callbacks(const fn_t & callbacks) noexcept {
	// Выполняем добавление функций обратного вызова в основноной модуль
	web2_t::callbacks(callbacks);
	// Выполняем установку функции обратного вызова выполнения редиректа с одного потока на другой (необходим для совместимости с HTTP/2)
	this->_callbacks.set("redirect", callbacks);
	{
		// Создаём локальный контейнер функций обратного вызова
		fn_t callbacks(this->_log);
		// Выполняем установку функции обратного вызова для вывода бинарных данных в сыром виде полученных с сервера
		callbacks.set("raw", this->_callbacks);
		// Выполняем установку функции обратного вызова на событие получения ошибки
		callbacks.set("error", this->_callbacks);
		// Выполняем установку функции обратного вызова для вывода полученного тела данных с сервера
		callbacks.set("entity", this->_callbacks);
		// Выполняем установку функции обратного вызова для вывода полученного списка разрешённых ресурсов для подключения
		callbacks.set("origin", this->_callbacks);
		// Выполняем установку функции обратного вызова для вывода полученного альтернативного сервиса от сервера
		callbacks.set("altsvc", this->_callbacks);
		// Выполняем установку функции обратного вызова активности потока
		callbacks.set("stream", this->_callbacks);
		// Выполняем установку функции обратного вызова для вывода полученного чанка бинарных данных с сервера
		callbacks.set("chunks", this->_callbacks);
		// Выполняем установку функции обратного вызова для вывода полученного заголовка с сервера
		callbacks.set("header", this->_callbacks);
		// Выполняем установку функции обратного вызова для вывода полученных заголовков с сервера
		callbacks.set("headers", this->_callbacks);
		// Выполняем установку функции обратного вызова для вывода ответа сервера на ранее выполненный запрос
		callbacks.set("response", this->_callbacks);
		// Выполняем установку функции обратного вызова для перехвата полученных чанков
		callbacks.set("chunking", this->_callbacks);
		// Выполняем установку функции завершения выполнения запроса
		callbacks.set("complete", this->_callbacks);
		// Выполняем установку функции обратного вызова при выполнении рукопожатия
		callbacks.set("handshake", this->_callbacks);
		// Выполняем установку функции обратного вызова на событие получения ошибок Websocket
		callbacks.set("errorWebsocket", this->_callbacks);
		// Выполняем установку функции обратного вызова на событие получения сообщений Websocket
		callbacks.set("messageWebsocket", this->_callbacks);
		// Если функции обратного вызова установлены
		if(!callbacks.empty())
			// Выполняем установку функций обратного вызова для Websocket-клиента
			this->_ws2.callbacks(std::move(callbacks));
	}{
		// Создаём локальный контейнер функций обратного вызова
		fn_t callbacks(this->_log);
		// Выполняем установку функции обратного вызова для вывода бинарных данных в сыром виде полученных с сервера
		callbacks.set("raw", this->_callbacks);
		// Выполняем установку функции обратного вызова на событие получения ошибки
		callbacks.set("error", this->_callbacks);
		// Выполняем установку функции обратного вызова для вывода полученного тела данных с сервера
		callbacks.set("entity", this->_callbacks);
		// Выполняем установку функции обратного вызова активности потока
		callbacks.set("stream", this->_callbacks);
		// Выполняем установку функции обратного вызова для вывода полученного чанка бинарных данных с сервера
		callbacks.set("chunks", this->_callbacks);
		// Выполняем установку функции обратного вызова длявывода полученного заголовка с сервера
		callbacks.set("header", this->_callbacks);
		// Выполняем установку функции обратного вызова для вывода полученных заголовков с сервера
		callbacks.set("headers", this->_callbacks);
		// Выполняем установку функции обратного вызова для вывода ответа сервера на ранее выполненный запрос
		callbacks.set("response", this->_callbacks);
		// Выполняем установку функции обратного вызова для перехвата полученных чанков
		callbacks.set("chunking", this->_callbacks);
		// Выполняем установку функции завершения выполнения запроса
		callbacks.set("complete", this->_callbacks);
		// Выполняем установку функции обратного вызова при выполнении рукопожатия
		callbacks.set("handshake", this->_callbacks);
		// Если функции обратного вызова установлены
		if(!callbacks.empty())
			// Выполняем установку функции обратного вызова для HTTP-клиента
			this->_http1.callbacks(std::move(callbacks));
	}
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
	// Выводим список расширений принадлежащих Websocket-клиенту
	return this->_ws2.extensions();
}
/**
 * extensions Метод установки списка расширений
 * @param extensions список поддерживаемых расширений
 */
void awh::client::Http2::extensions(const vector <vector <string>> & extensions) noexcept {
	// Выполняем установку списка доступных расширений для Websocket-клиента
	this->_ws2.extensions(extensions);
}
/**
 * chunk Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::client::Http2::chunk(const size_t size) noexcept {
	// Устанавливаем размер чанка
	web2_t::chunk(size);
	// Устанавливаем размер чанка для Websocket-клиента
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
		// Устанавливаем размер сегментов фрейма для Websocket-клиента
		this->_ws2.segmentSize(size);
}
/**
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::client::Http2::mode(const set <flag_t> & flags) noexcept {
	// Устанавливаем флаги настроек модуля для Websocket-клиента
	this->_ws2.mode(flags);
	// Устанавливаем флаги настроек модуля для HTTP/2 клиента
	this->_http1.mode(flags);
	// Выполняем установку флагов настроек модуля
	web2_t::mode(flags);
	// Устанавливаем флаг разрешающий выполнять подключение к протоколу Websocket
	this->_webSocket = (flags.find(flag_t::WEBSOCKET_ENABLE) != flags.end());
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
		// Устанавливаем функцию записи данных
		const_cast <client::core_t *> (this->_core)->callback <void (const char *, const size_t, const uint64_t, const uint16_t)> ("write", std::bind(&http2_t::writeCallback, this, _1, _2, _3, _4));
	// Если объект сетевого ядра не передан но ранее оно было добавлено
	} else if(this->_core != nullptr) {
		// Если многопоточность активированна
		if(this->_threads <= 0){
			// Если многопоточность активированна
			if(this->_ws2._thr.is() || this->_ws2._ws1._thr.is()){
				// Выполняем завершение всех активных потоков
				this->_ws2._thr.stop();
				// Выполняем завершение всех активных потоков
				this->_ws2._ws1._thr.stop();
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
	// Устанавливаем логин и пароль пользователя для Websocket-клиента
	this->_ws2.user(login, password);
	// Устанавливаем логин и пароль пользователя для HTTP-арсера
	this->_http.user(login, password);
	// Устанавливаем логин и пароль пользователя для HTTP-клиента
	this->_http1.user(login, password);
}
/**
 * userAgent Метод установки User-Agent для HTTP-запроса
 * @param userAgent агент пользователя для HTTP-запроса
 */
void awh::client::Http2::userAgent(const string & userAgent) noexcept {
	// Устанавливаем UserAgent
	if(!userAgent.empty()){
		// Устанавливаем пользовательского агента у родительского класса
		web2_t::userAgent(userAgent);
		// Устанавливаем пользовательского агента для Websocket-клиента
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
		// Устанавливаем данные сервиса для Websocket-клиента
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
	// Выполняем установку параметры прокси-сервера для Websocket-клиента
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
	// Устанавливаем параметры авторизации для Websocket-клиента
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
	// Устанавливаем тип авторизации на проксе-сервере для Websocket-клиента
	this->_ws2.authTypeProxy(type, hash);
	// Устанавливаем тип авторизации на проксе-сервере для HTTP-клиента
	this->_http1.authTypeProxy(type, hash);
}
/**
 * crypted Метод получения флага шифрования
 * @param sid идентификатор потока
 * @return    результат проверки
 */
bool awh::client::Http2::crypted(const int32_t sid) const noexcept {
	// Выполняем поиск воркера подключения
	auto i = this->_workers.find(sid);
	// Если искомый воркер найден
	if(i != this->_workers.end()){
		// Определяем протокол клиента
		switch(static_cast <uint8_t> (i->second->agent)){
			// Если агент является клиентом HTTP
			case static_cast <uint8_t> (agent_t::HTTP): {
				// Если переключение протокола на HTTP/2 не выполнено
				if(i->second->proto != engine_t::proto_t::HTTP2)
					// Выполняем получение флага шифрования для протокола HTTP/1.1
					return this->_http1.crypted();
				// Выполняем получение флага шифрования для протокола HTTP/2
				else return i->second->http.crypted();
			}
			// Если агент является клиентом Websocket
			case static_cast <uint8_t> (agent_t::WEBSOCKET):
				// Выполняем получение флага шифрования для протокола Websocket
				return this->_ws2.crypted();
		}
	}
	// Выводим результат
	return false;
}
/**
 * encryption Метод активации шифрования
 * @param mode флаг активации шифрования
 */
void awh::client::Http2::encryption(const bool mode) noexcept {
	// Устанавливаем флаг шифрования у родительского объекта
	web2_t::encryption(mode);
	// Устанавливаем флаг шифрования для Websocket-клиента
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
	// Устанавливаем параметры шифрования для Websocket-клиента
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
	// Выполняем установку перехвата событий завершения запроса для HTTP-клиента
	this->_http1.callback <void (const int32_t, const uint64_t)> ("result", std::bind(&http2_t::result, this, _1, _2));
	// Выполняем установку перехвата событий получения статуса овтета сервера для HTTP-клиента
	this->_http1.callback <void (const int32_t, const uint64_t, const awh::http_t::status_t)> ("answer", std::bind(&http2_t::answer, this, _1, _2, _3));
	// Выполняем установку функции обратного вызова для Websocket-клиента
	this->_ws2.callback <void (const int32_t, const uint64_t, const direct_t)> ("end", std::bind(&http2_t::end, this, _1, _2, _3));
	// Выполняем установку перехвата событий получения статуса овтета сервера для Websocket-клиента
	this->_ws2.callback <void (const int32_t, const uint64_t, const awh::http_t::status_t)> ("answer", std::bind(&http2_t::answer, this, _1, _2, _3));
	// Выполняем установку функции обратного вызова перехвата события редиректа
	this->_ws2.callback <void (const int32_t, const int32_t)> ("redirect", std::bind(static_cast <void (http2_t::*)(const int32_t, const int32_t)> (&http2_t::redirect), this, _1, _2));
	// Устанавливаем функцию обработки вызова на событие получения ошибок
	this->_http.callback <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", std::bind(&http2_t::errors, this, _1, _2, _3, _4));
}
/**
 * Http2 Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::client::Http2::Http2(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 web2_t(core, fmk, log), _ws2(fmk, log), _http1(fmk, log), _http(fmk, log), _webSocket(false), _threads(-1) {
	// Выполняем установку перехвата событий завершения запроса для HTTP-клиента
	this->_http1.callback <void (const int32_t, const uint64_t)> ("result", std::bind(&http2_t::result, this, _1, _2));
	// Выполняем установку перехвата событий получения статуса овтета сервера для HTTP-клиента
	this->_http1.callback <void (const int32_t, const uint64_t, const awh::http_t::status_t)> ("answer", std::bind(&http2_t::answer, this, _1, _2, _3));
	// Выполняем установку функции обратного вызова для Websocket-клиента
	this->_ws2.callback <void (const int32_t, const uint64_t, const direct_t)> ("end", std::bind(&http2_t::end, this, _1, _2, _3));
	// Выполняем установку перехвата событий получения статуса овтета сервера для Websocket-клиента
	this->_ws2.callback <void (const int32_t, const uint64_t, const awh::http_t::status_t)> ("answer", std::bind(&http2_t::answer, this, _1, _2, _3));
	// Выполняем установку функции обратного вызова перехвата события редиректа
	this->_ws2.callback <void (const int32_t, const int32_t)> ("redirect", std::bind(static_cast <void (http2_t::*)(const int32_t, const int32_t)> (&http2_t::redirect), this, _1, _2));
	// Устанавливаем функцию обработки вызова на событие получения ошибок
	this->_http.callback <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", std::bind(&http2_t::errors, this, _1, _2, _3, _4));
	// Устанавливаем функцию записи данных
	const_cast <client::core_t *> (this->_core)->callback <void (const char *, const size_t, const uint64_t, const uint16_t)> ("write", std::bind(&http2_t::writeCallback, this, _1, _2, _3, _4));
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
		this->_ws2._thr.stop();
}
