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
 * @copyright: Copyright © 2025
 */

// Подключаем заголовочный файл
#include <client/web/http1.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * connectEvent Метод обратного вызова при подключении к серверу
 * @param bid идентификатор брокера
 * @param sid идентификатор схемы сети
 */
void awh::client::Http1::connectEvent(const uint64_t bid, const uint16_t sid) noexcept {
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
		// Выполняем установку сетевого ядра
		this->_ws1._core = this->_core;
		// Если многопоточность активированна
		if(this->_threads > -1)
			// Выполняем инициализацию нового тредпула
			this->_ws1.multiThreads(this->_threads);
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
void awh::client::Http1::disconnectEvent(const uint64_t bid, const uint16_t sid) noexcept {
	// Выполняем редирект, если редирект выполнен
	if(this->redirect(bid, sid))
		// Выходим из функции
		return;
	// Если агент является Websocket-ом
	if(this->_agent == agent_t::WEBSOCKET)
		// Выполняем передачу сигнала отключения от сервера на Websocket-клиент
		this->_ws1.disconnectEvent(bid, sid);
	// Выполняем очистку списка запросов
	this->_requests.clear();
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
void awh::client::Http1::readEvent(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept {
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
							// Получаем идентификатор запроса
							const uint64_t rid = this->_requests.begin()->second.id;
							// Если функция обратного вызова активности потока установлена
							if(!this->_mode && (this->_mode = this->_callbacks.is("stream")))
								// Выполняем функцию обратного вызова
								this->_callbacks.call <void (const int32_t, const uint64_t, const mode_t)> ("stream", sid, rid, mode_t::OPEN);
							// Добавляем полученные данные в буфер
							this->_buffer.push(buffer, size);
							// Выполняем обработку полученных данных
							while(this->_reading){
								// Выполняем парсинг полученных данных
								const size_t bytes = this->_http.parse(reinterpret_cast <const char *> (this->_buffer.get()), this->_buffer.size());
								// Если все данные получены
								if((bytes > 0) && (completed = this->_http.is(http_t::state_t::END))){
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
												cout << string(response.begin(), response.end()) << endl;
												// Если тело ответа существует
												if(!this->_http.empty(awh::http_t::suite_t::BODY))
													// Выводим сообщение о выводе чанка тела
													cout << this->_fmk->format("<body %zu>", this->_http.body().size()) << endl << endl;
												// Иначе устанавливаем перенос строки
												else cout << endl;
											}
										}
									#endif
									// Выполняем препарирование полученных данных
									switch(static_cast <uint8_t> (this->prepare(sid, bid))){
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
										this->_buffer.erase(bytes);
								}
								// Если данные мы все получили, выходим
								if(!receive || this->_buffer.empty())
									// Выходим из цикла
									break;
							}
							// Устанавливаем метку завершения работы
							Stop:
							// Если получение данных выполнено
							if(completed){
								// Если функция обратного вызова на получение удачного ответа установлена
								if(this->_callbacks.is("handshake"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const int32_t, const uint64_t, const agent_t)> ("handshake", sid, rid, agent_t::HTTP);
								// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
								if(this->_resultCallback.is("entity"))
									// Выполняем функцию обратного вызова
									this->_resultCallback.bind("entity");
								// Если функция обратного вызова на вывод полученных данных ответа сервера установлена
								if(this->_resultCallback.is("complete"))
									// Выполняем функцию обратного вызова
									this->_resultCallback.bind("complete");
								// Выполняем очистку функций обратного вызова
								this->_resultCallback.clear();
								// Если функция обратного вызова активности потока установлена
								if(this->_callbacks.is("stream"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const int32_t, const uint64_t, const mode_t)> ("stream", sid, rid, mode_t::CLOSE);
								// Если установлена функция отлова завершения запроса
								if(this->_callbacks.is("end"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", sid, rid, direct_t::RECV);
								// Получаем параметры запроса
								const uint32_t code = this->_http.response().code;
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
					// Если протоколом агента является Websocket-клиент
					case static_cast <uint8_t> (agent_t::WEBSOCKET):
						// Выполняем переброс вызова чтения на клиент Websocket
						this->_ws1.readEvent(buffer, size, bid, sid);
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
 */
void awh::client::Http1::writeCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные существуют
	if((bid > 0) && (sid > 0)){
		// Определяем протокол клиента
		switch(static_cast <uint8_t> (this->_agent)){
			// Если агент является клиентом HTTP
			case static_cast <uint8_t> (agent_t::HTTP): {
				// Если установлена функция отлова завершения запроса
				if(this->_stopped && this->_callbacks.is("end")){
					// Если список ответов получен
					if(!this->_requests.empty())
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", sid, this->_requests.begin()->second.id, direct_t::SEND);
				}
			} break;
			// Если агент является клиентом Websocket
			case static_cast <uint8_t> (agent_t::WEBSOCKET):
				// Выполняем переброс вызова записи на клиент Websocket
				this->_ws1.writeCallback(buffer, size, bid, sid);
			break;
		}
	}
}
/**
 * answer Метод получение статуса ответа сервера
 * @param sid    идентификатор потока
 * @param rid    идентификатор запроса
 * @param status статус ответа сервера
 */
void awh::client::Http1::answer(const int32_t sid, const uint64_t rid, const awh::http_t::status_t status) noexcept {
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
 * redirect Метод выполнения редиректа если требуется
 * @param bid идентификатор брокера
 * @param sid идентификатор схемы сети
 * @return    результат выполнения редиректа
 */
bool awh::client::Http1::redirect(const uint64_t bid, const uint16_t sid) noexcept {
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
								// Если размер выделенной памяти выше максимального размера буфера
								if(request.entity.capacity() > AWH_BUFFER_SIZE)
									// Выполняем очистку временного буфера данных
									vector <char> ().swap(request.entity);
							}
							// Выполняем установку следующего экшена на открытие подключения
							this->open();
							// Завершаем работу
							return result;
						}
					}
				}
			} break;
			// Если протоколом агента является Websocket-клиент
			case static_cast <uint8_t> (agent_t::WEBSOCKET): {
				// Выполняем переброс вызова дисконнекта на клиент Websocket
				this->_ws1.disconnectEvent(bid, sid);
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
							this->_uri.combine(this->_ws1._scheme.url, url);
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
void awh::client::Http1::response([[maybe_unused]] const uint64_t bid, const uint32_t code, const string & message) noexcept {
	// Если функция обратного вызова на вывод ответа сервера на ранее выполненный запрос установлена
	if(!this->_requests.empty() && this->_callbacks.is("response")){
		// Выполняем получение первого запроса
		auto i = this->_requests.begin();
		// Выполняем функцию обратного вызова
		this->_callbacks.call <void (const int32_t, const uint64_t, const uint32_t, const string &)> ("response", i->first, i->second.id, code, message);
	}
}
/**
 * header Метод получения заголовка
 * @param bid   идентификатор брокера
 * @param key   ключ заголовка
 * @param value значение заголовка
 */
void awh::client::Http1::header([[maybe_unused]] const uint64_t bid, const string & key, const string & value) noexcept {
	// Если функция обратного вызова на полученного заголовка с сервера установлена
	if(!this->_requests.empty() && this->_callbacks.is("header")){
		// Выполняем получение первого запроса
		auto i = this->_requests.begin();
		// Выполняем функцию обратного вызова
		this->_callbacks.call <void (const int32_t, const uint64_t, const string &, const string &)> ("header", i->first, i->second.id, key, value);
	}
}
/**
 * headers Метод получения заголовков
 * @param bid     идентификатор брокера
 * @param code    код ответа сервера
 * @param message сообщение ответа сервера
 * @param headers заголовки ответа сервера
 */
void awh::client::Http1::headers([[maybe_unused]] const uint64_t bid, const uint32_t code, const string & message, const unordered_multimap <string, string> & headers) noexcept {
	// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
	if(!this->_requests.empty() && this->_callbacks.is("headers")){
		// Выполняем получение первого запроса
		auto i = this->_requests.begin();
		// Выполняем функцию обратного вызова
		this->_callbacks.call <void (const int32_t, const uint64_t, const uint32_t, const string &, const unordered_multimap <string, string> &)> ("headers", i->first, i->second.id, code, message, headers);
	}
}
/**
 * chunking Метод обработки получения чанков
 * @param bid   идентификатор брокера
 * @param chunk бинарный буфер чанка
 * @param http  объект модуля HTTP
 */
void awh::client::Http1::chunking([[maybe_unused]] const uint64_t bid, const vector <char> & chunk, const awh::http_t * http) noexcept {
	// Если данные получены, формируем тело сообщения
	if(!this->_requests.empty() && !chunk.empty()){
		// Извлекаем параметры текущего запроса
		auto i = this->_requests.begin();
		// Если запрос получен правильно
		if(i != this->_requests.end()){
			// Если функция обратного вызова на перехват входящих чанков установлена
			if(this->_callbacks.is("chunking"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const uint64_t, const vector <char> &, const awh::http_t *)> ("chunking", i->second.id, chunk, http);
			// Если функция перехвата полученных чанков не установлена
			else if(this->_core != nullptr) {
				// Выполняем добавление полученного чанка в тело ответа
				const_cast <awh::http_t *> (http)->body(chunk);
				// Если функция обратного вызова на вывода полученного чанка бинарных данных с сервера установлена
				if(this->_callbacks.is("chunks"))
					// Выполняем функцию обратного вызова
					this->_callbacks.call <void (const int32_t, const uint64_t, const vector <char> &)> ("chunks", i->first, i->second.id, chunk);
			}
		}
	}
}
/**
 * eventCallback Метод отлавливания событий контейнера функций обратного вызова
 * @param event событие контейнера функций обратного вызова
 * @param idw   идентификатор функции обратного вызова
 * @param name  название функции обратного вызова
 * @param dump  дамп данных функции обратного вызова
 */
void awh::client::Http1::eventCallback(const fn_t::event_t event, const uint64_t idw, const string & name, const fn_t::dump_t * dump) noexcept {
	// Определяем входящее событие контейнера функций обратного вызова
	switch(static_cast <uint8_t> (event)){
		// Если событием является установка функции обратного вызова
		case static_cast <uint8_t> (fn_t::event_t::SET): {
			// Если дамп функции обратного вызова передан и событие не является событием подключения
			if((dump != nullptr) && !this->_fmk->compare(name, "active")){
				// Создаём локальный контейнер функций обратного вызова
				fn_t callbacks(this->_log);
				// Выполняем установку функции обратного вызова
				callbacks.dump(idw, * dump);
				// Если функции обратного вызова установлены
				if(!callbacks.empty())
					// Выполняем установку функций обратного вызова для Websocket-клиента
					this->_ws1.callbacks(callbacks);
				// Если функция обратного вызова на перехват полученных чанков установлена
				if(this->_fmk->compare(name, "chunking"))
					// Устанавливаем внешнюю функцию обратного вызова
					this->_http.callback <void (const int32_t, const uint64_t, const vector <char> &)> ("chunking", this->_callbacks.get <void (const int32_t, const uint64_t, const vector <char> &)> ("chunking"));
			}
		} break;
	}
}
/**
 * flush Метод сброса параметров запроса
 */
void awh::client::Http1::flush() noexcept {
	// Разрешаем чтение данных из буфера
	this->_reading = true;
	// Снимаем флаг принудительной остановки
	this->_stopped = false;
	// Выполняем очистку оставшихся данных
	this->_buffer.clear();
}
/**
 * result Метод завершения выполнения запроса
 * @param sid идентификатор запроса
 */
void awh::client::Http1::result(const int32_t sid) noexcept {
	// Если объект ещё не удалён и получен окончательный ответ
	if(!this->_requests.empty()){
		// Идентификатор запроса
		uint64_t rid = 0;
		// Выполняем поиск указанного запроса
		auto i = this->_requests.find(sid);
		// Если параметры активного запроса найдены
		if(i != this->_requests.end()){
			// Выполняем получение идентификатора запроса
			rid = i->second.id;
			// Выполняем удаление объекта запроса
			this->_requests.erase(i);
		}
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
void awh::client::Http1::pinging(const uint16_t tid) noexcept {
	// Если данные существуют
	if((tid > 0) && (this->_core != nullptr)){
		// Если разрешено выполнять пинги
		if(this->_pinging){
			// Если агент является клиентом Websocket
			if(this->_agent == agent_t::WEBSOCKET)
				// Выполняем переброс персистентного вызова на клиент Websocket
				this->_ws1.pinging(tid);
		}
	}
}
/**
 * prepare Метод выполнения препарирования полученных данных
 * @param sid идентификатор запроса
 * @param bid идентификатор брокера
 * @return    результат препарирования
 */
awh::client::Web::status_t awh::client::Http1::prepare(const int32_t sid, const uint64_t bid) noexcept {
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
	// Выполняем поиск указанного запроса
	auto i = this->_requests.find(sid);
	// Если параметры активного запроса найдены
	if(i != this->_requests.end()){
		// Если функция обратного вызова получения статуса ответа установлена
		if(this->_callbacks.is("answer"))
			// Выполняем функцию обратного вызова
			this->_callbacks.call <void (const int32_t, const uint64_t, const awh::http_t::status_t)> ("answer", sid, i->second.id, status);
	}
	// Выполняем анализ результата авторизации
	switch(static_cast <uint8_t> (status)){
		// Если нужно попытаться ещё раз
		case static_cast <uint8_t> (awh::http_t::status_t::RETRY): {
			// Если функция обратного вызова на на вывод ошибок установлена
			if(((response.code == 401) || (response.code == 407)) && this->_callbacks.is("error"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP1_RECV, "authorization failed");
			// Если попытки повторить переадресацию ещё не закончились
			if(!(this->_stopped = (this->_attempt >= this->_attempts))){
				// Выполняем поиск указанного запроса
				i = this->_requests.find(sid);
				// Если параметры активного запроса найдены
				if(i != this->_requests.end()){
					// Получаем новый адрес запроса
					const uri_t::url_t & url = this->_http.url();
					// Если URL-адрес запроса получен
					if(!url.empty()){
						// Выполняем проверку соответствие протоколов
						const bool schema = (
							(this->_fmk->compare(url.host, i->second.url.host)) &&
							(this->_fmk->compare(url.schema, i->second.url.schema))
						);
						// Если соединение является постоянным
						if(schema && this->_http.is(http_t::state_t::ALIVE)){
							// Выполняем сброс параметров запроса
							this->flush();
							// Увеличиваем количество попыток
							this->_attempt++;
							// Устанавливаем новый адрес запроса
							this->_uri.combine(i->second.url, url);
							// Выполняем запрос на удалённый сервер
							this->send(i->second);
							// Если функция обратного вызова активности потока установлена
							if(this->_callbacks.is("stream"))
								// Выполняем функцию обратного вызова
								this->_callbacks.call <void (const int32_t, const uint64_t, const mode_t)> ("stream", sid, i->second.id, mode_t::CLOSE);
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
							this->send(i->second);
							// Если функция обратного вызова активности потока установлена
							if(this->_callbacks.is("stream"))
								// Выполняем функцию обратного вызова
								this->_callbacks.call <void (const int32_t, const uint64_t, const mode_t)> ("stream", sid, i->second.id, mode_t::CLOSE);
							// Завершаем работу
							return status_t::SKIP;
						}
					}
					// Если нам необходимо отключиться
					const_cast <client::core_t *> (this->_core)->close(bid);
					// Если функция обратного вызова активности потока установлена
					if(this->_callbacks.is("stream"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const int32_t, const uint64_t, const mode_t)> ("stream", sid, i->second.id, mode_t::CLOSE);
					// Завершаем работу
					return status_t::SKIP;
				}
			}
		} break;
		// Если запрос выполнен удачно
		case static_cast <uint8_t> (awh::http_t::status_t::GOOD): {
			// Выполняем поиск указанного запроса
			i = this->_requests.find(sid);
			// Если параметры активного запроса найдены
			if(i != this->_requests.end()){
				// Выполняем сброс количества попыток
				this->_attempt = 0;
				// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
				if(!this->_http.empty(awh::http_t::suite_t::BODY) && this->_callbacks.is("entity"))
					// Устанавливаем полученную функцию обратного вызова
					this->_resultCallback.set <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>)> ("entity", this->_callbacks.get <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>)> ("entity"), sid, i->second.id, response.code, response.message, this->_http.body());
				// Если функция обратного вызова на вывод полученных данных ответа сервера установлена
				if(this->_callbacks.is("complete"))
					// Выполняем функцию обратного вызова
					this->_resultCallback.set <void (const int32_t, const uint64_t, const uint32_t, const string &, const vector <char> &, const unordered_multimap <string, string> &)> ("complete", this->_callbacks.get <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>, const unordered_multimap <string, string> &)> ("complete"), sid, i->second.id, response.code, response.message, this->_http.body(), this->_http.headers());
				// Выполняем завершение запроса
				this->result(sid);
				// Устанавливаем размер стопбайт
				if(!this->_http.is(http_t::state_t::ALIVE)){
					// Выполняем очистку оставшихся данных
					this->_buffer.clear();
					// Завершаем работу
					const_cast <client::core_t *> (this->_core)->close(bid);
					// Выполняем завершение работы
					return status_t::STOP;
				}
			}
			// Завершаем обработку
			return status_t::NEXT;
		}
		// Если запрос неудачный
		case static_cast <uint8_t> (awh::http_t::status_t::FAULT): {
			// Выполняем поиск указанного запроса
			i = this->_requests.find(sid);
			// Если параметры активного запроса найдены
			if(i != this->_requests.end()){
				// Устанавливаем флаг принудительной остановки
				this->_stopped = true;
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callbacks.is("error"))
					// Выполняем функцию обратного вызова
					this->_callbacks.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP1_RECV, this->_http.message(response.code).c_str());
				// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
				if(!this->_http.empty(awh::http_t::suite_t::BODY) && this->_callbacks.is("entity"))
					// Устанавливаем полученную функцию обратного вызова
					this->_resultCallback.set <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>)> ("entity", this->_callbacks.get <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>)> ("entity"), sid, i->second.id, response.code, response.message, this->_http.body());
				// Если функция обратного вызова на вывод полученных данных ответа сервера установлена
				if(this->_callbacks.is("complete"))
					// Выполняем функцию обратного вызова
					this->_resultCallback.set <void (const int32_t, const uint64_t, const uint32_t, const string &, const vector <char> &, const unordered_multimap <string, string> &)> ("complete", this->_callbacks.get <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>, const unordered_multimap <string, string> &)> ("complete"), sid, i->second.id, response.code, response.message, this->_http.body(), this->_http.headers());
				// Выполняем завершение запроса
				this->result(sid);
			}
			// Завершаем обработку
			return status_t::NEXT;
		}
	}
	// Выполняем поиск указанного запроса
	i = this->_requests.find(sid);
	// Если параметры активного запроса найдены
	if(i != this->_requests.end()){
		// Выполняем очистку оставшихся данных
		this->_buffer.clear();
		// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
		if(!this->_http.empty(awh::http_t::suite_t::BODY) && this->_callbacks.is("entity"))
			// Устанавливаем полученную функцию обратного вызова
			this->_resultCallback.set <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>)> ("entity", this->_callbacks.get <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>)> ("entity"), sid, i->second.id, response.code, response.message, this->_http.body());
		// Если функция обратного вызова на вывод полученных данных ответа сервера установлена
		if(this->_callbacks.is("complete"))
			// Выполняем функцию обратного вызова
			this->_resultCallback.set <void (const int32_t, const uint64_t, const uint32_t, const string &, const vector <char> &, const unordered_multimap <string, string> &)> ("complete", this->_callbacks.get <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>, const unordered_multimap <string, string> &)> ("complete"), sid, i->second.id, response.code, response.message, this->_http.body(), this->_http.headers());
		// Выполняем завершение запроса
		this->result(sid);
	}
	// Завершаем работу
	const_cast <client::core_t *> (this->_core)->close(bid);
	// Выполняем завершение работы
	return status_t::STOP;
}
/**
 * sendError Метод отправки сообщения об ошибке
 * @param mess отправляемое сообщение об ошибке
 */
void awh::client::Http1::sendError(const ws::mess_t & mess) noexcept {
	// Выполняем отправку сообщения ошибки на Websocket-сервер
	this->_ws1.sendError(mess);
}
/**
 * sendMessage Метод отправки сообщения на сервер
 * @param message передаваемое сообщения в бинарном виде
 * @param text    данные передаются в текстовом виде
 * @return        результат отправки сообщения
 */
bool awh::client::Http1::sendMessage(const vector <char> & message, const bool text) noexcept {
	// Выполняем отправку сообщения на Websocket-сервер
	return this->_ws1.sendMessage(message, text);
}
/**
 * sendMessage Метод отправки сообщения на сервер
 * @param message передаваемое сообщения в бинарном виде
 * @param size    размер передаваемого сообещния
 * @param text    данные передаются в текстовом виде
 * @return        результат отправки сообщения
 */
bool awh::client::Http1::sendMessage(const char * message, const size_t size, const bool text) noexcept {
	// Выполняем отправку сообщения на Websocket-сервер
	return this->_ws1.sendMessage(message, size, text);
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
			const auto & buffer = this->_http.process(http_t::process_t::REQUEST, query);
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
				core->send(buffer.data(), buffer.size(), this->_bid);
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
					core->send(entity.data(), entity.size(), this->_bid);
				}
			}
			// Если установлена функция отлова завершения запроса
			if(this->_callbacks.is("end"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", this->_requests.begin()->first, this->_requests.begin()->second.id, direct_t::SEND);
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
		// Если сетевое ядро уже инициализированно
		if(this->_core != nullptr){
			// Если идентификатор запроса не установлен
			if(request.id == 0)
				// Выполняем генерацию идентификатора запроса
				const_cast <request_t &> (request).id = this->_fmk->timestamp(fmk_t::stamp_t::NANOSECONDS);
			// Если Websocket ещё не активирован
			if(this->_agent != agent_t::WEBSOCKET){
				// Если это первый запрос
				if(this->_attempt == 0){
					// Выполняем установку агента воркера Websocket
					this->_agent = request.agent;
					// Если требуется выполнить подключение к Websocket-клиенту
					if(this->_agent == agent_t::WEBSOCKET){
						// Если протокол Websocket запрещён
						if(!this->_webSocket){
							// Выводим сообщение об ошибке
							this->_log->print("Websocket protocol is prohibited for connection", log_t::flag_t::WARNING);
							// Если функция обратного вызова на на вывод ошибок установлена
							if(this->_callbacks.is("error"))
								// Выполняем функцию обратного вызова
								this->_callbacks.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::HTTP1_SEND, "Websocket protocol is prohibited for connection");
							// Выполняем отключение в обычном режиме
							const_cast <client::core_t *> (this->_core)->close(this->_bid);
							// Выходим из функции
							return -1;
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
						// Если протоколом агента является Websocket-клиент
						case static_cast <uint8_t> (agent_t::WEBSOCKET): {
							// Устанавливаем номер запроса
							this->_ws1._sid = result;
							// Устанавливаем идентификатор запроса
							this->_ws1._rid = request.id;
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
							// Если активирован режим прокси-сервера
							if(this->_proxy.mode)
								// Выполняем сброс заголовков прокси-сервера
								this->_ws1._scheme.proxy.http.dataAuth(this->_scheme.proxy.http.dataAuth());
							// Выполняем установку подключения с Websocket-сервером
							this->_ws1.connectEvent(this->_bid, this->_scheme.id);
						} break;
					}
					// Выводим результат
					return result;
				// Если список запросов не пустой
				} else if(!this->_requests.empty()) {
					// Выполняем запрос на удалённый сервер
					this->submit(this->_requests.begin()->second);
					// Выводим идентификатор подключения
					return this->_requests.size();
				// Если мы получили ошибку
				} else {
					// Выводим сообщение об ошибке
					this->_log->print("Number of redirect attempts has not been reset", log_t::flag_t::CRITICAL);
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callbacks.is("error"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP1_SEND, "Number of redirect attempts has not been reset");
					// Выполняем отключение в обычном режиме
					const_cast <client::core_t *> (this->_core)->close(this->_bid);
				}
			// Выводим сообщение об ошибке
			} else {
				// Выводим сообщение об ошибке
				this->_log->print("Websocket protocol is already activated", log_t::flag_t::WARNING);
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callbacks.is("error"))
					// Выполняем функцию обратного вызова
					this->_callbacks.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::HTTP1_SEND, "Websocket protocol is already activated");
				// Выполняем отключение в обычном режиме
				const_cast <client::core_t *> (this->_core)->close(this->_bid);
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
 * @return       результат отправки сообщения
 */
bool awh::client::Http1::send(const char * buffer, const size_t size) noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working() && (buffer != nullptr) && (size > 0))
		// Выполняем отправку заголовков запроса серверу
		return const_cast <client::core_t *> (this->_core)->send(buffer, size, this->_bid);
	// Сообщаем что ничего не найдено
	return false;
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
				const_cast <client::core_t *> (this->_core)->send(entity.data(), entity.size(), this->_bid);
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
			awh::web_t::req_t request(method, this->_scheme.url);
			// Если активирован режим прокси-сервера
			if(this->_proxy.mode){
				// Активируем точную установку хоста
				this->_http.precise(!this->_proxy.connect);
				// Выполняем извлечение заголовка авторизации на прокси-сервера
				const string & header = this->_scheme.proxy.http.auth(http_t::process_t::REQUEST, request);
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
			const auto & headers = this->_http.process(http_t::process_t::REQUEST, request);
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
				const_cast <client::core_t *> (this->_core)->send(headers.data(), headers.size(), this->_bid);
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
 * waitPong Метод установки времени ожидания ответа WebSocket-сервера
 * @param sec время ожидания в секундах
 */
void awh::client::Http1::waitPong(const time_t sec) noexcept {
	// Выполняем установку времени ожидания
	this->_ws1.waitPong(sec);
}
/**
 * pingInterval Метод установки интервала времени выполнения пингов
 * @param sec интервал времени выполнения пингов в секундах
 */
void awh::client::Http1::pingInterval(const time_t sec) noexcept {
	// Выполняем установку интервала времени выполнения пингов в секундах
	this->_ws1.pingInterval(sec);
}
/**
 * callbacks Метод установки функций обратного вызова
 * @param callbacks функции обратного вызова
 */
void awh::client::Http1::callbacks(const fn_t & callbacks) noexcept {
	// Выполняем добавление функций обратного вызова в основноной модуль
	web_t::callbacks(callbacks);
	{
		// Создаём локальный контейнер функций обратного вызова
		fn_t callbacks(this->_log);
		// Выполняем установку функции обратного вызова для вывода бинарных данных в сыром виде полученных с сервера
		callbacks.set("raw", this->_callbacks);
		// Выполняем установку функции обратного вызова при завершении запроса
		callbacks.set("end", this->_callbacks);
		// Выполняем установку функции обратного вызова на событие получения ошибки
		callbacks.set("error", this->_callbacks);
		// Выполняем установку функции обратного вызова для вывода полученного тела данных с сервера
		callbacks.set("entity", this->_callbacks);
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
			this->_ws1.callbacks(callbacks);
	}
	// Если функция обратного вызова на перехват полученных чанков установлена
	if(this->_callbacks.is("chunking"))
		// Устанавливаем внешнюю функцию обратного вызова
		this->_http.callback <void (const int32_t, const uint64_t, const vector <char> &)> ("chunking", this->_callbacks.get <void (const int32_t, const uint64_t, const vector <char> &)> ("chunking"));
	// Устанавливаем функцию обработки вызова для получения чанков для HTTP-клиента
	else this->_http.callback <void (const uint64_t, const vector <char> &, const awh::http_t *)> ("chunking", bind(&http1_t::chunking, this, _1, _2, _3));
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
	// Выводим список расширений принадлежащих Websocket-клиенту
	return this->_ws1.extensions();
}
/**
 * extensions Метод установки списка расширений
 * @param extensions список поддерживаемых расширений
 */
void awh::client::Http1::extensions(const vector <vector <string>> & extensions) noexcept {
	// Выполняем установку списка доступных расширений для Websocket-клиента
	this->_ws1.extensions(extensions);
}
/**
 * chunk Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::client::Http1::chunk(const size_t size) noexcept {
	// Устанавливаем размер чанка для Websocket-клиента
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
		// Устанавливаем размер сегментов фрейма для Websocket-клиента
		this->_ws1.segmentSize(size);
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
		// Устанавливаем функцию записи данных
		const_cast <client::core_t *> (this->_core)->callback <void (const char *, const size_t, const uint64_t, const uint16_t)> ("write", bind(&http1_t::writeCallback, this, _1, _2, _3, _4));
	// Если объект сетевого ядра не передан но ранее оно было добавлено
	} else if(this->_core != nullptr) {
		// Если многопоточность активированна
		if(this->_threads <= 0){
			// Если многопоточность активированна
			if(this->_ws1._thr.is())
				// Выполняем завершение всех активных потоков
				this->_ws1._thr.stop();
			// Снимаем режим простого чтения базы событий
			const_cast <client::core_t *> (this->_core)->easily(false);
		}
		// Выполняем передачу настроек сетевого ядра в родительский модуль
		web_t::core(core);
	}
}
/**
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::client::Http1::mode(const set <flag_t> & flags) noexcept {
	// Устанавливаем флаги настроек модуля для Websocket-клиента
	this->_ws1.mode(flags);
	// Активируем выполнение пинга
	this->_pinging = (flags.find(flag_t::NOT_PING) == flags.end());
	// Если установлен флаг запрещающий переключение контекста SSL
	this->_nossl = (flags.find(flag_t::NO_INIT_SSL) != flags.end());
	// Устанавливаем флаг анбиндинга ядра сетевого модуля
	this->_complete = (flags.find(flag_t::NOT_STOP) == flags.end());
	// Устанавливаем флаг разрешающий выполнять редиректы
	this->_redirects = (flags.find(flag_t::REDIRECTS) != flags.end());
	// Устанавливаем флаг разрешающий выполнять подключение к протоколу Websocket
	this->_webSocket = (flags.find(flag_t::WEBSOCKET_ENABLE) != flags.end());
	// Устанавливаем флаг поддержания автоматического подключения
	this->_scheme.alive = (flags.find(flag_t::ALIVE) != flags.end());
	// Устанавливаем флаг разрешающий выполнять метод CONNECT для прокси-клиента
	this->_proxy.connect = (flags.find(flag_t::CONNECT_METHOD_ENABLE) != flags.end());
	// Если сетевое ядро установлено
	if(this->_core != nullptr)
		// Устанавливаем флаг запрещающий вывод информационных сообщений
		const_cast <client::core_t *> (this->_core)->verbose(flags.find(flag_t::NOT_INFO) == flags.end());
}
/**
 * user Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::client::Http1::user(const string & login, const string & password) noexcept {
	// Устанавливаем логин и пароль пользователя для Websocket-клиента
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
		// Устанавливаем пользовательского агента для Websocket-клиента
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
		// Устанавливаем данные сервиса для Websocket-клиента
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
 * proxy Метод активации/деактивации прокси-склиента
 * @param work флаг активации/деактивации прокси-клиента
 */
void awh::client::Http1::proxy(const client::scheme_t::work_t work) noexcept {
	// Устанавливаем флаг активации/деактивации прокси-клиента
	web_t::proxy(work);
	// Устанавливаем флаг активации/деактивации прокси-клиента для Websocket-клиента
	this->_ws1.proxy(work);
}
/**
 * proxy Метод установки прокси-сервера
 * @param uri    параметры прокси-сервера
 * @param family семейстово интернет протоколов (IPV4 / IPV6 / NIX)
 */
void awh::client::Http1::proxy(const string & uri, const scheme_t::family_t family) noexcept {
	// Выполняем установку параметры прокси-сервера
	web_t::proxy(uri, family);
	// Выполняем установку параметры прокси-сервера для Websocket-клиента
	this->_ws1.proxy(uri, family);
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest-авторизации
 */
void awh::client::Http1::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Устанавливаем параметры авторизации для Websocket-клиента
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
	// Устанавливаем тип авторизации на проксе-сервере для Websocket-клиента
	this->_ws1.authTypeProxy(type, hash);
}
/**
 * crypted Метод получения флага шифрования
 * @return результат проверки
 */
bool awh::client::Http1::crypted() const noexcept {
	// Определяем протокол клиента
	switch(static_cast <uint8_t> (this->_agent)){
		// Если агент является клиентом HTTP
		case static_cast <uint8_t> (agent_t::HTTP):
			// Выполняем получение флага шифрования
			return this->_http.crypted();
		// Если агент является клиентом Websocket
		case static_cast <uint8_t> (agent_t::WEBSOCKET):
			// Выполняем получение флага шифрования для протокола Websocket
			return this->_ws1.crypted();
	}
	// Выводим результат
	return false;
}
/**
 * encryption Метод активации шифрования
 * @param mode флаг активации шифрования
 */
void awh::client::Http1::encryption(const bool mode) noexcept {
	// Устанавливаем флаг шифрования в родительском модуле
	web_t::encryption(mode);
	// Устанавливаем флаг шифрования для Websocket-клиента
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
	// Устанавливаем параметры шифрования для Websocket-клиента
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
	// Выполняем установку перехвата событий получения статуса овтета сервера для Websocket-клиента
	this->_ws1.callback <void (const int32_t, const uint64_t, const awh::http_t::status_t)> ("answer", bind(&http1_t::answer, this, _1, _2, _3));
	// Устанавливаем функцию обработки вызова для вывода полученного заголовка с сервера
	this->_http.callback <void (const uint64_t, const string &, const string &)> ("header", bind(&http1_t::header, this, _1, _2, _3));
	// Устанавливаем функцию обработки вызова для вывода ответа сервера на ранее выполненный запрос
	this->_http.callback <void (const uint64_t, const uint32_t, const string &)> ("response", bind(&http1_t::response, this, _1, _2, _3));
	// Устанавливаем функцию обработки вызова для получения чанков для HTTP-клиента
	this->_http.callback <void (const uint64_t, const vector <char> &, const awh::http_t *)> ("chunking", bind(&http1_t::chunking, this, _1, _2, _3));
	// Устанавливаем функцию обработки вызова на событие получения ошибок
	this->_http.callback <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", bind(&http1_t::errors, this, _1, _2, _3, _4));
	// Устанавливаем функцию обработки вызова для вывода полученных заголовков с сервера
	this->_http.callback <void (const uint64_t, const uint32_t, const string &, const unordered_multimap <string, string> &)> ("headersResponse", bind(&http1_t::headers, this, _1, _2, _3, _4));
}
/**
 * Http1 Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::client::Http1::Http1(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 web_t(core, fmk, log), _mode(false), _webSocket(false), _ws1(fmk, log), _http(fmk, log), _agent(agent_t::HTTP), _threads(-1), _resultCallback(log) {
	// Выполняем установку перехвата событий получения статуса овтета сервера для Websocket-клиента
	this->_ws1.callback <void (const int32_t, const uint64_t, const awh::http_t::status_t)> ("answer", bind(&http1_t::answer, this, _1, _2, _3));
	// Устанавливаем функцию обработки вызова для вывода полученного заголовка с сервера
	this->_http.callback <void (const uint64_t, const string &, const string &)> ("header", bind(&http1_t::header, this, _1, _2, _3));
	// Устанавливаем функцию обработки вызова для вывода ответа сервера на ранее выполненный запрос
	this->_http.callback <void (const uint64_t, const uint32_t, const string &)> ("response", bind(&http1_t::response, this, _1, _2, _3));
	// Устанавливаем функцию обработки вызова для получения чанков для HTTP-клиента
	this->_http.callback <void (const uint64_t, const vector <char> &, const awh::http_t *)> ("chunking", bind(&http1_t::chunking, this, _1, _2, _3));
	// Устанавливаем функцию обработки вызова на событие получения ошибок
	this->_http.callback <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", bind(&http1_t::errors, this, _1, _2, _3, _4));
	// Устанавливаем функцию обработки вызова для вывода полученных заголовков с сервера
	this->_http.callback <void (const uint64_t, const uint32_t, const string &, const unordered_multimap <string, string> &)> ("headersResponse", bind(&http1_t::headers, this, _1, _2, _3, _4));
	// Устанавливаем функцию записи данных
	const_cast <client::core_t *> (this->_core)->callback <void (const char *, const size_t, const uint64_t, const uint16_t)> ("write", bind(&http1_t::writeCallback, this, _1, _2, _3, _4));
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
		this->_ws1._thr.stop();
}
