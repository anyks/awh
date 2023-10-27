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
	// Если сетевое ядро уже инициализированно
	if(this->_core != nullptr)
		// Выполняем отправку заголовков запроса на сервер
		const_cast <client::core_t *> (this->_core)->write((const char *) buffer, size, this->_bid);
}
/**
 * frameProxySignal Метод обратного вызова при получении фрейма заголовков прокси-сервера HTTP/2
 * @param sid    идентификатор потока
 * @param direct направление передачи фрейма
 * @param type   тип полученного фрейма
 * @param flags  флаг полученного фрейма
 * @return       статус полученных данных
 */
int awh::client::Web2::frameProxySignal(const int32_t sid, const http2_t::direct_t direct, const uint8_t type, const uint8_t flags) noexcept {
	// Если идентификатор сессии клиента совпадает
	if((this->_core != nullptr) && (this->_proxy.sid == sid)){
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
					// Если сетевое ядро инициализированно
					if(this->_core != nullptr)
						// Завершаем работу
						const_cast <client::core_t *> (this->_core)->close(this->_bid);
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
							const auto & response = this->_scheme.proxy.http.process(http_t::process_t::RESPONSE, this->_scheme.proxy.http.response());
							// Если параметры ответа получены
							if(!response.empty())
								// Выводим параметры ответа
								cout << string(response.begin(), response.end()) << endl << endl;
						}
					#endif
					// Получаем параметры запроса
					const auto & response = this->_scheme.proxy.http.response();
					// Получаем статус ответа
					awh::http_t::status_t status = this->_scheme.proxy.http.auth();
					// Устанавливаем ответ прокси-сервера
					this->_proxy.answer = response.code;
					// Если выполнять редиректы запрещено
					if(!this->_redirects && (status == awh::http_t::status_t::RETRY)){
						// Если ответом сервера не является запросом авторизации
						if(response.code != 407)
							// Запрещаем выполнять редирект
							status = awh::http_t::status_t::GOOD;
					}
					// Выполняем проверку авторизации
					switch(static_cast <uint8_t> (status)){
						// Если нужно попытаться ещё раз
						case static_cast <uint8_t> (awh::http_t::status_t::RETRY): {
							// Если попытки повторить переадресацию ещё не закончились
							if(!(this->_stopped = (this->_attempt >= this->_attempts))){
								// Если адрес запроса получен
								if(!this->_scheme.proxy.url.empty()){
									// Если соединение является постоянным
									if(this->_scheme.proxy.http.is(http_t::state_t::ALIVE)){
										// Увеличиваем количество попыток
										this->_attempt++;
										// Устанавливаем новый экшен выполнения
										this->proxyConnectCallback(this->_bid, this->_scheme.sid, const_cast <client::core_t *> (this->_core));
									// Если соединение должно быть закрыто
									} else const_cast <client::core_t *> (this->_core)->close(this->_bid);
									// Завершаем работу
									return 0;
								}
							}
						} break;
						// Если запрос выполнен удачно
						case static_cast <uint8_t> (awh::http_t::status_t::GOOD): {
							// Выполняем переключение на работу с сервером
							this->_scheme.switchConnect();
							// Выполняем запуск работы основного модуля
							this->connectCallback(this->_bid, this->_scheme.sid, const_cast <client::core_t *> (this->_core));
							// Завершаем работу
							return 0;
						} break;
						// Если запрос неудачный
						case static_cast <uint8_t> (awh::http_t::status_t::FAULT):
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
					// Если сетевое ядро инициализированно
					if(this->_core != nullptr)
						// Завершаем работу
						const_cast <client::core_t *> (this->_core)->close(this->_bid);
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
		if(status == awh::core_t::status_t::STOP)
			// Выполняем удаление сессии
			this->_http2.close();
		// Выполняем передачу события в родительский объект
		web_t::eventsCallback(status, core);
	}
}
/**
 * connectCallback Метод обратного вызова при подключении к серверу
 * @param bid  идентификатор брокера
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::Web2::connectCallback(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::OPEN, event_t::READ, event_t::PROXY_READ, event_t::CONNECT}, event_t::CONNECT))
		// Выполняем инициализацию сессии HTTP/2
		this->implementation(bid, dynamic_cast <client::core_t *> (core));
}
/**
 * proxyConnectCallback Метод обратного вызова при подключении к прокси-серверу
 * @param bid  идентификатор брокера
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::Web2::proxyConnectCallback(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((bid > 0) && (sid > 0) && (core != nullptr)){
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
					web_t::proxyConnectCallback(bid, sid, core);
				break;
				// Если прокси-сервер является HTTP/2
				case static_cast <uint8_t> (client::proxy_t::type_t::HTTPS): {
					// Если протокол подключения является HTTP/2
					if(core->proto(bid) == engine_t::proto_t::HTTP2){
						// Запоминаем идентификатор брокера
						this->_bid = bid;
						// Если протокол активирован HTTPS или WSS защищённый поверх SSL
						if(this->_proxy.connect){
							// Выполняем сброс состояния HTTP парсера
							this->_scheme.proxy.http.reset();
							// Выполняем очистку параметров HTTP запроса
							this->_scheme.proxy.http.clear();
							// Выполняем инициализацию сессии HTTP/2
							this->implementation(bid, dynamic_cast <client::core_t *> (core));
							// Если флаг инициализации сессии HTTP/2 установлен
							if(this->_http2.is()){
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
									cout << string(buffer.begin(), buffer.end()) << endl << endl;
								#endif
								// Выполняем запрос на получение заголовков
								const auto & headers = this->_scheme.proxy.http.proxy2(std::move(query));
								// Выполняем заголовки запроса на сервер
								this->_proxy.sid = this->_http2.sendHeaders(-1, headers, http2_t::flag_t::END_STREAM);
								// Если запрос не получилось отправить
								if(this->_proxy.sid < 0){
									// Выполняем закрытие подключения
									dynamic_cast <client::core_t *> (core)->close(bid);
									// Выходим из функции
									return;
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
								dynamic_cast <client::core_t *> (core)->close(bid);
								// Выполняем завершение работы приложения
								return;
							}
						// Если протокол подключения не является защищённым подключением
						} else {
							// Выполняем переключение на работу с сервером
							this->_scheme.switchConnect();
							// Выполняем запуск работы основного модуля
							this->connectCallback(bid, sid, core);
						}
					// Выполняем передачу сигнала родительскому модулю
					} else web_t::proxyConnectCallback(bid, sid, core);
				} break;
				// Иначе завершаем работу
				default: dynamic_cast <client::core_t *> (core)->close(bid);
			}
		}
	}
}
/**
 * proxyReadCallback Метод обратного вызова при чтении сообщения с прокси-сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param bid    идентификатор брокера
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::client::Web2::proxyReadCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (bid > 0) && (sid > 0)){
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
					web_t::proxyReadCallback(buffer, size, bid, sid, core);
				break;
				// Если прокси-сервер является HTTP/2
				case static_cast <uint8_t> (client::proxy_t::type_t::HTTPS): {
					// Если протокол подключения является HTTP/2
					if(core->proto(bid) == engine_t::proto_t::HTTP2){
						// Если прочитать данные фрейма не удалось, выходим из функции
						if(!this->_http2.frame((const uint8_t *) buffer, size))
							// Выходим из функции
							return;
					// Если активирован режим работы с HTTP/1.1 протоколом
					} else web_t::proxyReadCallback(buffer, size, bid, sid, core);
				} break;
				// Иначе завершаем работу
				default: dynamic_cast <client::core_t *> (core)->close(bid);
			}
		}
	}
}
/**
 * implementation Метод выполнения активации сессии HTTP/2
 * @param bid  идентификатор брокера
 * @param core объект сетевого ядра
 */
void awh::client::Web2::implementation(const uint64_t bid, client::core_t * core) noexcept {
	// Если флаг инициализации сессии HTTP/2 не активирован, но протокол HTTP/2 поддерживается сервером
	if(!this->_http2.is() && (core->proto(bid) == engine_t::proto_t::HTTP2)){
		// Если список параметров настроек не пустой
		if(!this->_settings.empty()){
			// Создаём параметры сессии и активируем запрет использования той самой неудачной системы приоритизации из первой итерации HTTP/2.
			vector <nghttp2_settings_entry> iv = {{NGHTTP2_SETTINGS_NO_RFC7540_PRIORITIES, 1}};
			// Выполняем переход по всему списку настроек
			for(auto & setting : this->_settings){
				// Определяем тип настройки
				switch(static_cast <uint8_t> (setting.first)){
					// Если мы получили разрешение присылать push-уведомления
					case static_cast <uint8_t> (settings_t::ENABLE_PUSH):
						// Устанавливаем разрешение присылать push-уведомления
						iv.push_back({NGHTTP2_SETTINGS_ENABLE_PUSH, setting.second});
					break;
					// Если мы получили максимальный размер фрейма
					case static_cast <uint8_t> (settings_t::FRAME_SIZE):
						/**
						 * Устанавливаем максимальный размер кадра в октетах, который собеседнику разрешено отправлять.
						 * Значение по умолчанию, оно же минимальное — 16 384=214 октетов.
						 */
						iv.push_back({NGHTTP2_SETTINGS_MAX_FRAME_SIZE, setting.second});
					break;
					// Если мы получили максимальный размер таблицы заголовков
					case static_cast <uint8_t> (settings_t::HEADER_TABLE_SIZE):
						// Устанавливаем максимальный размер таблицы заголовков
						iv.push_back({NGHTTP2_SETTINGS_HEADER_TABLE_SIZE, setting.second});
					break;
					// Если мы получили максимальный размер окна полезной нагрузки
					case static_cast <uint8_t> (settings_t::WINDOW_SIZE):
						// Устанавливаем элемент управления потоком (flow control)
						iv.push_back({NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE, setting.second});
					break;
					// Если мы получили максимальное количество потоков
					case static_cast <uint8_t> (settings_t::STREAMS):
						/**
						 * Устанавливаем максимальное количество потоков, которое собеседнику разрешается использовать одновременно.
						 * Считаются только открытые потоки, то есть по которым что-то ещё передаётся.
						 * Можно указать значение 0: тогда собеседник не сможет отправлять новые сообщения, пока сторона его снова не увеличит.
						 */
						iv.push_back({NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, setting.second});
					break;
				}
			}
			// Выполняем установку функции обратного вызова начала открытии потока
			this->_http2.on((function <int (const int32_t)>) std::bind(&web2_t::beginSignal, this, _1));
			// Выполняем установку функции обратного вызова при отправки сообщения на сервер
			this->_http2.on((function <void (const uint8_t *, const size_t)>) std::bind(&web2_t::sendSignal, this, _1, _2));
			// Выполняем установку функции обратного вызова при закрытии потока
			this->_http2.on((function <int (const int32_t, const uint32_t)>) std::bind(&web2_t::closedSignal, this, _1, _2));
			// Выполняем установку функции обратного вызова при получении чанка с сервера
			this->_http2.on((function <int (const int32_t, const uint8_t *, const size_t)>) std::bind(&web2_t::chunkSignal, this, _1, _2, _3));
			// Выполняем установку функции обратного вызова при получении данных заголовка
			this->_http2.on((function <int (const int32_t, const string &, const string &)>) std::bind(&web2_t::headerSignal, this, _1, _2, _3));
			// Выполняем установку функции обратного вызова получения фрейма
			this->_http2.on((function <int (const int32_t, const http2_t::direct_t, const uint8_t, const uint8_t)>) std::bind(&web2_t::frameSignal, this, _1, _2, _3, _4));
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем установку функции обратного вызова на событие получения ошибки
				this->_http2.on(this->_callback.get <void (const log_t::flag_t, const http::error_t, const string &)> ("error"));
			// Выполняем инициализацию модуля NgHttp2
			this->_http2.init(http2_t::mode_t::CLIENT, std::move(iv));
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
	if(hold.access({event_t::CONNECT, event_t::READ, event_t::SEND}, event_t::SEND))
		// Выполняем пинг удалённого сервера
		return this->_http2.ping();
	// Выводим результат
	return false;
}
/**
 * windowUpdate Метод обновления размера окна фрейма
 * @param id   идентификатор потока
 * @param size размер нового окна
 * @return     результат установки размера офна фрейма
 */
bool awh::client::Web2::windowUpdate(const int32_t id, const int32_t size) noexcept {
	// Результат работы функции
	bool result = false;
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::CONNECT, event_t::READ, event_t::SEND}, event_t::SEND)){
		// Если флаг инициализации сессии HTTP/2 установлен и подключение выполнено
		if((result = ((this->_core != nullptr) && this->_core->working() && (size > 0)))){
			// Выполняем отправку нового размера окна фрейма
			if(!(result = this->_http2.windowUpdate(id, size))){
				// Выполняем закрытие подключения
				const_cast <client::core_t *> (this->_core)->close(this->_bid);
				// Выходим из функции
				return result;
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * send Метод отправки сообщения на сервер
 * @param id     идентификатор потока
 * @param buffer буфер бинарных данных передаваемых на сервер
 * @param size   размер сообщения в байтах
 * @param flag   флаг передаваемого потока по сети
 * @return       результат отправки данных указанному клиенту
 */
bool awh::client::Web2::send(const int32_t id, const char * buffer, const size_t size, const http2_t::flag_t flag) noexcept {
	// Результат работы функции
	bool result = false;
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::CONNECT, event_t::READ, event_t::SEND}, event_t::SEND)){
		// Если флаг инициализации сессии HTTP/2 установлен и подключение выполнено
		if((result = ((this->_core != nullptr) && this->_core->working() && (buffer != nullptr) && (size > 0)))){
			// Выполняем отправку тела запроса на сервер
			if(!(result = this->_http2.sendData(id, reinterpret_cast <const uint8_t *> (buffer), size, flag))){
				// Выполняем закрытие подключения
				const_cast <client::core_t *> (this->_core)->close(this->_bid);
				// Выходим из функции
				return result;
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * send Метод отправки заголовков на сервер
 * @param id      идентификатор потока
 * @param headers заголовки отправляемые на сервер
 * @param flag    флаг передаваемого потока по сети
 * @return        идентификатор нового запроса
 */
int32_t awh::client::Web2::send(const int32_t id, const vector <pair <string, string>> & headers, const http2_t::flag_t flag) noexcept {
	// Результат работы функции
	int32_t result = -1;
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::CONNECT, event_t::READ, event_t::SEND}, event_t::SEND)){
		// Если флаг инициализации сессии HTTP/2 установлен и подключение выполнено
		if((this->_core != nullptr) && this->_core->working() && !headers.empty()){
			// Выполняем отправку заголовков запроса на сервер
			result = this->_http2.sendHeaders(id, headers, flag);
			// Если запрос не получилось отправить
			if(result < 0){
				// Выполняем закрытие подключения
				const_cast <client::core_t *> (this->_core)->close(this->_bid);
				// Выходим из функции
				return result;
			}
		}
	}
	// Выводим результат
	return result;
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
	// Если флаг разрешения принимать push-уведомления не установлено
	if(this->_settings.count(settings_t::ENABLE_PUSH) == 0)
		// Выполняем установку флага отключения принёма push-уведомлений
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
 * ident Метод установки идентификации клиента
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::client::Web2::ident(const string & id, const string & name, const string & ver) noexcept {
	// Если данные сервиса переданы
	if(!id.empty() && !name.empty() && !ver.empty()){
		// Выполняем установку данных сервиса у родительского класса
		web_t::ident(id, name, ver);
		// Запоминаем идентификатор сервиса
		this->_ident.id = id;
		// Запоминаем версию сервиса
		this->_ident.ver = ver;
		// Запоминаем название сервиса
		this->_ident.name = name;
	}
}
/**
 * Web2 Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::client::Web2::Web2(const fmk_t * fmk, const log_t * log) noexcept :
 web_t(fmk, log), _http2(fmk, log), _login{""}, _password{""}, _userAgent{""}, _chunkSize(BUFFER_CHUNK) {
	// Выполняем установку список настроек протокола HTTP/2
	this->settings();
}
/**
 * Web2 Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::client::Web2::Web2(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 web_t(core, fmk, log), _http2(fmk, log), _login{""}, _password{""}, _userAgent{""}, _chunkSize(BUFFER_CHUNK) {
	// Выполняем установку список настроек протокола HTTP/2
	this->settings();
}
