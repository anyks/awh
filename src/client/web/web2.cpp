/**
 * @file: web2.cpp
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
#include <client/web/web.hpp>

/**
 * sendSignal Метод обратного вызова при отправки данных HTTP/2
 * @param buffer буфер бинарных данных
 * @param size  размер буфера данных для отправки
 */
void awh::client::Web2::sendSignal(const uint8_t * buffer, const size_t size) noexcept {
	// Выполняем отправку заголовков запроса на сервер
	const_cast <client::core_t *> (this->_core)->write((const char *) buffer, size, this->_aid);
}
/**
 * frameProxySignal Метод обратного вызова при получении фрейма заголовков прокси-сервера HTTP/2
 * @param sid   идентификатор потока
 * @param type  тип полученного фрейма
 * @param flags флаг полученного фрейма
 * @return      статус полученных данных
 */
int awh::client::Web2::frameProxySignal(const int32_t sid, const uint8_t type, const uint8_t flags) noexcept {
	// Если идентификатор сессии клиента совпадает
	if(this->_proxy.sid == sid){
		// Выполняем определение типа фрейма
		switch(type){
			// Если мы получили входящие данные тела ответа
			case NGHTTP2_DATA: {
				// Если мы получили флаг завершения потока
				if(flags & NGHTTP2_FLAG_END_STREAM){
					// Выполняем коммит полученного результата
					this->_scheme.proxy.http.commit();
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						{
							// Если тело ответа существует
							if(!this->_scheme.proxy.http.body().empty())
								// Выводим сообщение о выводе чанка тела
								cout << this->_fmk->format("<body %u>", this->_scheme.proxy.http.body().size()) << endl << endl;
							// Иначе устанавливаем перенос строки
							else cout << endl;
						}
					#endif
					// Получаем параметры запроса
					const auto & response = this->_scheme.proxy.http.response();
					// Если функция обратного вызова активности потока установлена
					if(this->_callback.is("stream"))
						// Выводим функцию обратного вызова
						this->_callback.call <const int32_t, const mode_t> ("stream", sid, mode_t::CLOSE);
					// Если функция обратного вызова установлена, выводим сообщение
					if(this->_callback.is("entity"))
						// Выполняем функцию обратного вызова дисконнекта
						this->_callback.call <const int32_t, const u_int, const string, const vector <char>> ("entity", sid, response.code, response.message, this->_scheme.proxy.http.body());
					// Завершаем работу
					const_cast <client::core_t *> (this->_core)->close(this->_aid);
				}
			} break;
			// Если мы получили входящие данные заголовков ответа
			case NGHTTP2_HEADERS: {
				// Если сессия клиента совпадает с сессией полученных даных и передача заголовков завершена
				if(flags & NGHTTP2_FLAG_END_HEADERS){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						{
							// Получаем данные ответа
							const auto & response = this->_scheme.proxy.http.process(http_t::process_t::RESPONSE, true);
							// Если параметры ответа получены
							if(!response.empty())
								// Выводим параметры ответа
								cout << string(response.begin(), response.end()) << endl;
						}
					#endif
					// Получаем параметры запроса
					const auto & response = this->_scheme.proxy.http.response();
					// Получаем статус ответа
					awh::http_t::stath_t status = this->_scheme.proxy.http.getAuth();
					// Устанавливаем ответ прокси-сервера
					this->_proxy.answer = response.code;
					// Если выполнять редиректы запрещено
					if(!this->_redirects && (status == awh::http_t::stath_t::RETRY)){
						// Если ответом сервера не является запросом авторизации
						if(response.code != 407)
							// Запрещаем выполнять редирект
							status = awh::http_t::stath_t::GOOD;
					}
					// Выполняем проверку авторизации
					switch(static_cast <uint8_t> (status)){
						// Если нужно попытаться ещё раз
						case static_cast <uint8_t> (awh::http_t::stath_t::RETRY): {
							// Если попытки повторить переадресацию ещё не закончились
							if(!(this->_stopped = (this->_attempt >= this->_attempts))){
								// Если адрес запроса получен
								if(!this->_scheme.proxy.url.empty()){
									// Если соединение является постоянным
									if(this->_scheme.proxy.http.isAlive()){
										// Увеличиваем количество попыток
										this->_attempt++;
										// Устанавливаем новый экшен выполнения
										this->proxyConnectCallback(this->_aid, this->_scheme.sid, const_cast <client::core_t *> (this->_core));
									// Если соединение должно быть закрыто
									} else const_cast <client::core_t *> (this->_core)->close(this->_aid);
									// Завершаем работу
									return 0;
								}
							}
						} break;
						// Если запрос выполнен удачно
						case static_cast <uint8_t> (awh::http_t::stath_t::GOOD): {
							// Выполняем переключение на работу с сервером
							this->_scheme.switchConnect();
							// Выполняем запуск работы основного модуля
							this->connectCallback(this->_aid, this->_scheme.sid, const_cast <client::core_t *> (this->_core));
							// Завершаем работу
							return 0;
						} break;
						// Если запрос неудачный
						case static_cast <uint8_t> (awh::http_t::stath_t::FAULT):
							// Устанавливаем флаг принудительной остановки
							this->_stopped = true;
						break;
					}
					// Если функция обратного вызова на вывод ответа сервера на ранее выполненный запрос установлена
					if(this->_callback.is("response"))
						// Выводим функцию обратного вызова
						this->_callback.call <const int32_t, const u_int, const string &> ("response", sid, response.code, response.message);
					// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
					if(this->_callback.is("headers"))
						// Выводим функцию обратного вызова
						this->_callback.call <const int32_t, const u_int, const string &, const unordered_multimap <string, string> &> ("headers", sid, response.code, response.message, this->_scheme.proxy.http.headers());
					// Завершаем работу
					const_cast <client::core_t *> (this->_core)->close(this->_aid);
				}
			} break;
		}
	}
	// Выводим результат
	return 0;
}
/**
 * chunkProxySignal Метод обратного вызова при получении чанка с прокси-сервера HTTP/2
 * @param sid    идентификатор потока
 * @param buffer буфер данных который содержит полученный чанк
 * @param size   размер полученного буфера данных чанка
 * @return       статус полученных данных
 */
int awh::client::Web2::chunkProxySignal(const int32_t sid, const uint8_t * buffer, const size_t size) noexcept {
	// Если идентификатор сессии клиента совпадает
	if(this->_proxy.sid == sid)
		// Добавляем полученный чанк в тело данных
		this->_scheme.proxy.http.body(vector <char> (buffer, buffer + size));
	// Выводим результат
	return 0;
}
/**
 * beginProxySignal Метод начала получения фрейма заголовков HTTP/2 прокси-сервера
 * @param sid идентификатор потока
 * @return    статус полученных данных
 */
int awh::client::Web2::beginProxySignal(const int32_t sid) noexcept {
	// Если идентификатор сессии клиента совпадает
	if(this->_proxy.sid == sid)
		// Выполняем очистку параметров HTTP запроса
		this->_scheme.proxy.http.clear();
	// Выводим результат
	return 0;
}
/**
 * headerProxySignal Метод обратного вызова при получении заголовка HTTP/2 прокси-сервера
 * @param sid идентификатор потока
 * @param key данные ключа заголовка
 * @param val данные значения заголовка
 * @return    статус полученных данных
 */
int awh::client::Web2::headerProxySignal(const int32_t sid, const string & key, const string & val) noexcept {
	// Если идентификатор сессии клиента совпадает
	if(this->_proxy.sid == sid)
		// Устанавливаем полученные заголовки
		this->_scheme.proxy.http.header2(key, val);
	// Выводим результат
	return 0;
}
/**
 * eventsCallback Функция обратного вызова при активации ядра сервера
 * @param status флаг запуска/остановки
 * @param core   объект сетевого ядра
 */
void awh::client::Web2::eventsCallback(const awh::core_t::status_t status, awh::core_t * core) noexcept {
	// Если данные существуют
	if(core != nullptr){
		// Если система была остановлена
		if(status == awh::core_t::status_t::STOP){
			// Выполняем удаление сессии
			this->_nghttp2.free();
			// Снимаем флаг инициализации сессии HTTP2
			this->_sessionInitialized = false;
		}
		// Если функция получения событий запуска и остановки сетевого ядра установлена
		if(this->_callback.is("events"))
			// Выводим функцию обратного вызова
			this->_callback.call <const awh::core_t::status_t, awh::core_t *> ("events", status, core);
	}
}
/**
 * connectCallback Метод обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::Web2::connectCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::OPEN, event_t::READ, event_t::PROXY_READ, event_t::CONNECT}, event_t::CONNECT))
		// Выполняем инициализацию сессии HTTP/2
		this->implementation(aid, dynamic_cast <client::core_t *> (core));
}
/**
 * proxyConnectCallback Метод обратного вызова при подключении к прокси-серверу
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::Web2::proxyConnectCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Создаём объект холдирования
		hold_t <event_t> hold(this->_events);
		// Если событие соответствует разрешённому
		if(hold.access({event_t::OPEN, event_t::PROXY_READ}, event_t::PROXY_CONNECT)){
			// Определяем тип прокси-сервера
			switch(static_cast <uint8_t> (this->_scheme.proxy.type)){
				// Если прокси-сервер является HTTP/1.1
				case static_cast <uint8_t> (client::proxy_t::type_t::HTTP):
				// Если прокси-сервер является Socks5
				case static_cast <uint8_t> (client::proxy_t::type_t::SOCKS5):
					// Выполняем передачу сигнала родительскому модулю
					web_t::proxyConnectCallback(aid, sid, core);
				break;
				// Если прокси-сервер является HTTP/2
				case static_cast <uint8_t> (client::proxy_t::type_t::HTTPS): {
					// Если протокол подключения является HTTP/2
					if(core->proto(aid) == engine_t::proto_t::HTTP2){
						// Запоминаем идентификатор адъютанта
						this->_aid = aid;
						// Если протокол активирован HTTPS или WSS защищённый поверх TLS
						if(this->_proxy.connect){
							// Выполняем сброс состояния HTTP парсера
							this->_scheme.proxy.http.reset();
							// Выполняем очистку параметров HTTP запроса
							this->_scheme.proxy.http.clear();
							// Выполняем инициализацию сессии HTTP/2
							this->implementation(aid, dynamic_cast <client::core_t *> (core));
							// Если флаг инициализации сессии HTTP2 установлен
							if(this->_sessionInitialized){
								// Список заголовков для запроса
								vector <nghttp2_nv> nva;
								// Создаём объек запроса
								awh::web_t::req_t query(awh::web_t::method_t::CONNECT, this->_scheme.url);
								/**
								 * Если включён режим отладки
								 */
								#if defined(DEBUG_MODE)
									// Выводим заголовок запроса
									cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST PROXY ^^^^^^^^^\x1B[0m" << endl;
									// Получаем бинарные данные WEB запроса
									const auto & buffer = this->_scheme.proxy.http.proxy(query);
									// Выводим параметры запроса
									cout << string(buffer.begin(), buffer.end()) << endl;
								#endif
								// Выполняем запрос на получение заголовков
								const auto & headers = this->_scheme.proxy.http.proxy2(std::move(query));
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
								// Если тело запроса не существует, выполняем установленный запрос
								this->_proxy.sid = nghttp2_submit_request(this->_nghttp2.session, nullptr, nva.data(), nva.size(), nullptr, this);
								// Если запрос не получилось отправить
								if(this->_proxy.sid < 0){
									// Выводим в лог сообщение
									this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(this->_proxy.sid));
									// Если функция обратного вызова на на вывод ошибок установлена
									if(this->_callback.is("error"))
										// Выводим функцию обратного вызова
										this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::PROXY_HTTP2_SUBMIT, nghttp2_strerror(this->_proxy.sid));
									// Выполняем закрытие подключения
									dynamic_cast <client::core_t *> (core)->close(aid);
									// Выходим из функции
									return;
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
											this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::PROXY_HTTP2_SEND, nghttp2_strerror(rv));
										// Выполняем закрытие подключения
										dynamic_cast <client::core_t *> (core)->close(aid);
										// Выходим из функции
										return;
									}
								}
							// Если инициализировать сессию HTTP/2 не удалось
							} else {
								// Выводим сообщение об ошибке подключения к прокси-серверу
								this->_log->print("Proxy server does not support the HTTP/2 protocol", log_t::flag_t::CRITICAL);
								// Если функция обратного вызова на на вывод ошибок установлена
								if(this->_callback.is("error"))
									// Выводим функцию обратного вызова
									this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::PROXY_HTTP2_NO_INIT, "Proxy server does not support the HTTP/2 protocol");
								// Выполняем закрытие подключения
								dynamic_cast <client::core_t *> (core)->close(aid);
								// Выполняем завершение работы приложения
								return;
							}
						// Если протокол подключения не является защищённым подключением
						} else {
							// Выполняем переключение на работу с сервером
							this->_scheme.switchConnect();
							// Выполняем запуск работы основного модуля
							this->connectCallback(aid, sid, core);
						}
					// Выполняем передачу сигнала родительскому модулю
					} else web_t::proxyConnectCallback(aid, sid, core);
				} break;
				// Иначе завершаем работу
				default: dynamic_cast <client::core_t *> (core)->close(aid);
			}
		}
	}
}
/**
 * proxyReadCallback Метод обратного вызова при чтении сообщения с прокси-сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::client::Web2::proxyReadCallback(const char * buffer, const size_t size, const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (sid > 0)){
		// Создаём объект холдирования
		hold_t <event_t> hold(this->_events);
		// Если событие соответствует разрешённому
		if(hold.access({event_t::PROXY_CONNECT}, event_t::PROXY_READ)){
			// Определяем тип прокси-сервера
			switch(static_cast <uint8_t> (this->_scheme.proxy.type)){
				// Если прокси-сервер является HTTP
				case static_cast <uint8_t> (client::proxy_t::type_t::HTTP):
				// Если прокси-сервер является Socks5
				case static_cast <uint8_t> (client::proxy_t::type_t::SOCKS5):
					// Выполняем передачу сигнала родительскому модулю
					web_t::proxyReadCallback(buffer, size, aid, sid, core);
				break;
				// Если прокси-сервер является HTTP/2
				case static_cast <uint8_t> (client::proxy_t::type_t::HTTPS): {
					// Если протокол подключения является HTTP/2
					if(core->proto(aid) == engine_t::proto_t::HTTP2){
						// Выполняем извлечение полученного чанка данных из сокета
						ssize_t bytes = nghttp2_session_mem_recv(this->_nghttp2.session, (const uint8_t *) buffer, size);
						// Если данные не прочитаны, выводим ошибку и выходим
						if(bytes < 0){
							// Выводим сообщение об полученной ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(static_cast <int> (bytes)));
							// Если функция обратного вызова на на вывод ошибок установлена
							if(this->_callback.is("error"))
								// Выводим функцию обратного вызова
								this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::PROXY_HTTP2_RECV, nghttp2_strerror(static_cast <int> (bytes)));
							// Выходим из функции
							return;
						}
						// Фиксируем полученный результат
						if((bytes = nghttp2_session_send(this->_nghttp2.session)) != 0){
							// Выводим сообщение об полученной ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(static_cast <int> (bytes)));
							// Если функция обратного вызова на на вывод ошибок установлена
							if(this->_callback.is("error"))
								// Выводим функцию обратного вызова
								this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::PROXY_HTTP2_SEND, nghttp2_strerror(static_cast <int> (bytes)));
							// Выходим из функции
							return;
						}
					// Если активирован режим работы с HTTP/1.1 протоколом
					} else web_t::proxyReadCallback(buffer, size, aid, sid, core);
				} break;
				// Иначе завершаем работу
				default: dynamic_cast <client::core_t *> (core)->close(aid);
			}
		}
	}
}
/**
 * implementation Метод выполнения активации сессии HTTP/2
 * @param aid  идентификатор адъютанта
 * @param core объект сетевого ядра
 */
void awh::client::Web2::implementation(const uint64_t aid, client::core_t * core) noexcept {
	// Если флаг инициализации сессии HTTP2 не активирован, но протокол HTTP/2 поддерживается сервером
	if(!this->_sessionInitialized && (core->proto(aid) == engine_t::proto_t::HTTP2)){
		// Если список параметров настроек не пустой
		if(!this->_settings.empty()){
			// Создаём параметры сессии подключения с HTTP/2 сервером
			vector <nghttp2_settings_entry> iv;
			// Выполняем переход по всему списку настроек
			for(auto & setting : this->_settings){
				// Определяем тип настройки
				switch(static_cast <uint8_t> (setting.first)){
					// Если мы получили разрешение присылать пуш-уведомления
					case static_cast <uint8_t> (settings_t::ENABLE_PUSH):
						// Устанавливаем разрешение присылать пуш-уведомления
						iv.push_back({NGHTTP2_SETTINGS_ENABLE_PUSH, setting.second});
					break;
					// Если мы получили максимальный размер фрейма
					case static_cast <uint8_t> (settings_t::FRAME_SIZE):
						// Устанавливаем максимальный размер фрейма
						iv.push_back({NGHTTP2_SETTINGS_MAX_FRAME_SIZE, setting.second});
					break;
					// Если мы получили максимальный размер таблицы заголовков
					case static_cast <uint8_t> (settings_t::HEADER_TABLE_SIZE):
						// Устанавливаем максимальный размер таблицы заголовков
						iv.push_back({NGHTTP2_SETTINGS_HEADER_TABLE_SIZE, setting.second});
					break;
					// Если мы получили максимальный размер окна полезной нагрузки
					case static_cast <uint8_t> (settings_t::WINDOW_SIZE):
						// Устанавливаем максимальный размер окна полезной нагрузки
						iv.push_back({NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE, setting.second});
					break;
					// Если мы получили максимальное количество потоков
					case static_cast <uint8_t> (settings_t::STREAMS):
						// Устанавливаем максимальное количество потоков
						iv.push_back({NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, setting.second});
					break;
				}
			}
			// Выполняем установку функции обратного вызова начала открытии потока
			this->_nghttp2.on((function <int (const int32_t)>) std::bind(&web2_t::beginSignal, this, _1));
			// Выполняем установку функции обратного вызова при отправки сообщения на сервер
			this->_nghttp2.on((function <void (const uint8_t *, const size_t)>) std::bind(&web2_t::sendSignal, this, _1, _2));
			// Выполняем установку функции обратного вызова при закрытии потока
			this->_nghttp2.on((function <int (const int32_t, const uint32_t)>) std::bind(&web2_t::closedSignal, this, _1, _2));
			// Выполняем установку функции обратного вызова получения фрейма HTTP/2
			this->_nghttp2.on((function <int (const int32_t, const uint8_t, const uint8_t)>) std::bind(&web2_t::frameSignal, this, _1, _2, _3));
			// Выполняем установку функции обратного вызова при получении чанка с сервера
			this->_nghttp2.on((function <int (const int32_t, const uint8_t *, const size_t)>) std::bind(&web2_t::chunkSignal, this, _1, _2, _3));
			// Выполняем установку функции обратного вызова при получении данных заголовка
			this->_nghttp2.on((function <int (const int32_t, const string &, const string &)>) std::bind(&web2_t::headerSignal, this, _1, _2, _3));
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем установку функции обратного вызова на событие получения ошибки
				this->_nghttp2.on(this->_callback.get <void (const log_t::flag_t, const http::error_t, const string &)> ("error"));
			// Выполняем инициализацию модуля NgHttp2
			this->_sessionInitialized = this->_nghttp2.init(std::move(iv));
		}
	}
}
/**
 * ping Метод выполнения пинга сервера
 * @return результат работы пинга
 */
bool awh::client::Web2::ping() noexcept {
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::CONNECT, event_t::READ}, event_t::SEND)){
		// Если флаг инициализации сессии HTTP2 установлен
		if(this->_sessionInitialized){
			// Результат выполнения поерации
			int rv = -1;
			// Выполняем пинг удалённого сервера
			if((rv = nghttp2_submit_ping(this->_nghttp2.session, 0, nullptr)) != 0){
				// Выводим сообщение об полученной ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выводим функцию обратного вызова
					this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_PING, nghttp2_strerror(rv));
				// Выходим из функции
				return false;
			}
			// Фиксируем отправленный результат
			if((rv = nghttp2_session_send(this->_nghttp2.session)) != 0){
				// Выводим сообщение об полученной ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выводим функцию обратного вызова
					this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_SEND, nghttp2_strerror(rv));
				// Выходим из функции
				return false;
			}
			// Выводим результат
			return true;
		}
	}
	// Выводим результат
	return false;
}
/**
 * send Метод отправки сообщения на сервер
 * @param id      идентификатор потока HTTP/2
 * @param message сообщение передаваемое на сервер
 * @param size    размер сообщения в байтах
 * @param end     флаг последнего сообщения после которого поток закрывается
 */
void awh::client::Web2::send(const int32_t id, const char * message, const size_t size, const bool end) noexcept {
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::CONNECT, event_t::READ, event_t::SEND}, event_t::SEND)){
		// Если флаг инициализации сессии HTTP2 установлен и подключение выполнено
		if(this->_sessionInitialized && this->_core->working() && (message != nullptr) && (size > 0)){
			// Список файловых дескрипторов
			int fds[2];
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
				const_cast <client::core_t *> (this->_core)->close(this->_aid);
				// Выходим из функции
				return;
			}
			/**
			 * Методы только для OS Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Если данные небыли записаны в сокет
				if(static_cast <int> (_write(fds[1], message, size)) != static_cast <int> (size)){
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
					const_cast <client::core_t *> (this->_core)->close(this->_aid);
					// Выходим из функции
					return;
				}
			/**
			 * Для всех остальных операционных систем
			 */
			#else
				// Если данные небыли записаны в сокет
				if(static_cast <int> (::write(fds[1], message, size)) != static_cast <int> (size)){
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
					const_cast <client::core_t *> (this->_core)->close(this->_aid);
					// Выходим из функции
					return;
				}
			#endif
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
			{
				// Результат фиксации сессии
				int rv = -1;
				// Выполняем формирование данных фрейма для отправки
				if((rv = nghttp2_submit_data(this->_nghttp2.session, (end ? NGHTTP2_FLAG_END_STREAM : NGHTTP2_FLAG_NONE), id, &data)) != 0){
					// Выводим сообщение об полученной ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callback.is("error"))
						// Выводим функцию обратного вызова
						this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_SUBMIT, nghttp2_strerror(rv));
					// Выходим из функции
					return;
				}
				// Фиксируем отправленный результат
				if((rv = nghttp2_session_send(this->_nghttp2.session)) != 0){
					// Выводим сообщение об полученной ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callback.is("error"))
						// Выводим функцию обратного вызова
						this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_SEND, nghttp2_strerror(rv));
					// Выходим из функции
					return;
				}
			}
		}
	}
}
/**
 * settings Модуль установки настроек протокола HTTP/2
 * @param settings список настроек протокола HTTP/2
 */
void awh::client::Web2::settings(const map <settings_t, uint32_t> & settings) noexcept {
	// Если список настроек протокола HTTP/2 передан
	if(!settings.empty())
		// Выполняем установку списка настроек
		this->_settings = settings;
	// Если максимальное количество потоков не установлено
	if(this->_settings.count(settings_t::STREAMS) == 0)
		// Выполняем установку максимального количества потоков
		this->_settings.emplace(settings_t::STREAMS, CONCURRENT_STREAMS);
	// Если максимальный размер фрейма не установлен
	if(this->_settings.count(settings_t::FRAME_SIZE) == 0)
		// Выполняем установку максимального размера фрейма
		this->_settings.emplace(settings_t::FRAME_SIZE, MAX_FRAME_SIZE_MIN);
	// Если максимальный размер фрейма установлен
	else {
		// Выполняем извлечение максимального размера фрейма
		auto it = this->_settings.find(settings_t::FRAME_SIZE);
		// Если максимальный размер фрейма больше самого максимального значения
		if(it->second > MAX_FRAME_SIZE_MAX)
			// Выполняем корректировку максимального размера фрейма
			it->second = MAX_FRAME_SIZE_MAX;
		// Если максимальный размер фрейма меньше самого минимального значения
		else if(it->second < MAX_FRAME_SIZE_MIN)
			// Выполняем корректировку максимального размера фрейма
			it->second = MAX_FRAME_SIZE_MIN;
	}
	// Если максимальный размер окна фрейма не установлен
	if(this->_settings.count(settings_t::WINDOW_SIZE) == 0)
		// Выполняем установку максимального размера окна фрейма
		this->_settings.emplace(settings_t::WINDOW_SIZE, MAX_WINDOW_SIZE);
	// Если максимальный размер блока заголовоков не установлен
	if(this->_settings.count(settings_t::HEADER_TABLE_SIZE) == 0)
		// Выполняем установку максимального размера блока заголовоков
		this->_settings.emplace(settings_t::HEADER_TABLE_SIZE, HEADER_TABLE_SIZE);
	// Если флаг разрешения принимать пуш-уведомления не установлено
	if(this->_settings.count(settings_t::ENABLE_PUSH) == 0)
		// Выполняем установку флага отключения принёма пуш-уведомлений
		this->_settings.emplace(settings_t::ENABLE_PUSH, 0);
}
/**
 * chunk Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::client::Web2::chunk(const size_t size) noexcept {
	// Если размер чанка передан
	if(size >= 100)
		// Выполняем установку размера чанка
		this->_chunkSize = size;
}
/**
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::client::Web2::mode(const set <flag_t> & flags) noexcept {
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
		// Активируем персистентный запуск для работы пингов
		const_cast <client::core_t *> (this->_core)->persistEnable(this->_scheme.alive);
		// Устанавливаем флаг запрещающий вывод информационных сообщений
		const_cast <client::core_t *> (this->_core)->noInfo(flags.count(flag_t::NOT_INFO) > 0);
		// Выполняем установку флага проверки домена
		const_cast <client::core_t *> (this->_core)->verifySSL(flags.count(flag_t::VERIFY_SSL) > 0);
	}
}
/**
 * userAgent Метод установки User-Agent для HTTP запроса
 * @param userAgent агент пользователя для HTTP запроса
 */
void awh::client::Web2::userAgent(const string & userAgent) noexcept {
	// Устанавливаем UserAgent
	if(!userAgent.empty()){
		// Устанавливаем пользовательского агента у родительского класса
		web_t::userAgent(userAgent);
		// Устанавливаем пользовательского агента
		this->_userAgent = userAgent;
	}
}
/**
 * user Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::client::Web2::user(const string & login, const string & password) noexcept {
	// Если логин пользователя передан
	if(!login.empty())
		// Выполняем установку логина пользователя
		this->_login = login;
	// Если пароль пользователя передан
	if(!password.empty())
		// Выполняем установку пароля пользователя
		this->_password = password;
}
/**
 * serv Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::client::Web2::serv(const string & id, const string & name, const string & ver) noexcept {
	// Если данные сервиса переданы
	if(!id.empty() && !name.empty() && !ver.empty()){
		// Выполняем установку данных сервиса у родительского класса
		web_t::serv(id, name, ver);
		// Запоминаем идентификатор сервиса
		this->_serv.id = id;
		// Запоминаем версию сервиса
		this->_serv.ver = ver;
		// Запоминаем название сервиса
		this->_serv.name = name;
	}
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest-авторизации
 */
void awh::client::Web2::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Выполняем установку типа авторизации
	this->_authType = type;
	// Выполняем установку алгоритма шифрования для Digest-авторизации
	this->_authHash = hash;
}
/**
 * crypto Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::client::Web2::crypto(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Если пароль для шифрования передаваемых данных получен
	if(!pass.empty())
		// Выполняем установку пароля шифрования передаваемых данных
		this->_crypto.pass = pass;
	// Если соль шифрования переданных данных получен
	if(!salt.empty())
		// Выполняем установку соли шифрования передаваемых данных
		this->_crypto.salt = salt;
	// Выполняем установку размера шифрования передаваемых данных
	this->_crypto.cipher = cipher;
}
/**
 * Web2 Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::client::Web2::Web2(const fmk_t * fmk, const log_t * log) noexcept :
 web_t(fmk, log), _nghttp2(fmk, log),
 _login{""}, _password{""}, _userAgent{""},
 _chunkSize(BUFFER_CHUNK), _authType(auth_t::type_t::BASIC),
 _authHash(auth_t::hash_t::MD5), _sessionInitialized(false) {
	// Выполняем установку список настроек протокола HTTP/2
	this->settings();
	// Устанавливаем функцию персистентного вызова
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("persist", std::bind(&web2_t::persistCallback, this, _1, _2, _3));
}
/**
 * Web2 Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::client::Web2::Web2(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 web_t(core, fmk, log), _nghttp2(fmk, log),
 _login{""}, _password{""}, _userAgent{""},
 _chunkSize(BUFFER_CHUNK), _authType(auth_t::type_t::BASIC),
 _authHash(auth_t::hash_t::MD5), _sessionInitialized(false) {
	// Выполняем установку список настроек протокола HTTP/2
	this->settings();
	// Устанавливаем функцию персистентного вызова
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("persist", std::bind(&web2_t::persistCallback, this, _1, _2, _3));
}
