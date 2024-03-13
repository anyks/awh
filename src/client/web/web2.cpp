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
		const_cast <client::core_t *> (this->_core)->write(reinterpret_cast <const char *> (buffer), size, this->_bid);
}
/**
 * frameProxySignal Метод обратного вызова при получении фрейма заголовков прокси-сервера HTTP/2
 * @param sid    идентификатор потока
 * @param direct направление передачи фрейма
 * @param type   тип полученного фрейма
 * @param flags  флаги полученного фрейма
 * @return       статус полученных данных
 */
int awh::client::Web2::frameProxySignal(const int32_t sid, const http2_t::direct_t direct, const http2_t::frame_t frame, const set <http2_t::flag_t> & flags) noexcept {
	// Если идентификатор сессии клиента совпадает
	if((this->_core != nullptr) && (this->_proxy.sid == sid)){
		// Выполняем определение типа фрейма
		switch(static_cast <uint8_t> (frame)){
			// Если мы получили входящие данные тела ответа
			case static_cast <uint8_t> (http2_t::frame_t::DATA): {
				// Если мы получили флаг завершения потока
				if(flags.count(http2_t::flag_t::END_STREAM) > 0){
					// Выполняем коммит полученного результата
					this->_scheme.proxy.http.commit();
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						{
							// Если тело ответа существует
							if(!this->_scheme.proxy.http.empty(awh::http_t::suite_t::BODY))
								// Выводим сообщение о выводе чанка тела
								cout << this->_fmk->format("<body %u>", this->_scheme.proxy.http.body().size()) << endl << endl;
							// Иначе устанавливаем перенос строки
							else cout << endl;
						}
					#endif
					// Выполняем препарирование полученных данных
					switch(static_cast <uint8_t> (this->prepareProxy(sid, this->_bid))){
						// Если необходимо выполнить пропуск обработки данных
						case static_cast <uint8_t> (status_t::SKIP):
						// Если необходимо выполнить перейти к следующему шагу
						case static_cast <uint8_t> (status_t::NEXT):
							// Завершаем работу
							return 0;
					}
					// Получаем параметры запроса
					const auto & response = this->_scheme.proxy.http.response();
					// Если функция обратного вызова установлена, выводим сообщение
					if(!this->_scheme.proxy.http.empty(awh::http_t::suite_t::BODY) && this->_callbacks.is("entity"))
						// Выполняем функцию обратного вызова дисконнекта
						this->_callbacks.call <void (const int32_t, const uint64_t, const u_int, const string, const vector <char>)> ("entity", sid, 0, response.code, response.message, this->_scheme.proxy.http.body());
					// Если функция обратного вызова на вывод полученных данных ответа сервера установлена
					if(this->_callbacks.is("complete"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const int32_t, const uint64_t, const u_int, const string &, const vector <char> &, const unordered_multimap <string, string> &)> ("complete", sid, 0, response.code, response.message, this->_scheme.proxy.http.body(), this->_scheme.proxy.http.headers());
					// Если установлена функция отлова завершения запроса
					if(this->_callbacks.is("end"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", sid, 0, direct_t::RECV);
				}
			} break;
			// Если мы получили входящие данные заголовков ответа
			case static_cast <uint8_t> (http2_t::frame_t::HEADERS): {
				// Если сессия клиента совпадает с сессией полученных даных и передача заголовков завершена
				if(flags.count(http2_t::flag_t::END_HEADERS) > 0){
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
					// Если функция обратного вызова на вывод ответа сервера на ранее выполненный запрос установлена
					if(this->_callbacks.is("response"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const int32_t, const uint64_t, const u_int, const string &)> ("response", sid, 0, response.code, response.message);
					// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
					if(this->_callbacks.is("headers"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const int32_t, const uint64_t, const u_int, const string &, const unordered_multimap <string, string> &)> ("headers", sid, 0, response.code, response.message, this->_scheme.proxy.http.headers());
					// Если мы получили флаг завершения потока
					if(flags.count(awh::http2_t::flag_t::END_STREAM) > 0){
						// Выполняем коммит полученного результата
						this->_scheme.proxy.http.commit();
						// Выполняем препарирование полученных данных
						switch(static_cast <uint8_t> (this->prepareProxy(sid, this->_bid))){
							// Если необходимо выполнить пропуск обработки данных
							case static_cast <uint8_t> (status_t::SKIP):
							// Если необходимо выполнить перейти к следующему шагу
							case static_cast <uint8_t> (status_t::NEXT):
								// Завершаем работу
								return 0;
						}
					}
				}
				// Если мы получили флаг завершения потока
				if(flags.count(awh::http2_t::flag_t::END_STREAM) > 0){
					// Если установлена функция отлова завершения запроса
					if(this->_callbacks.is("end"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", sid, 0, direct_t::RECV);
				}
			}
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
		// Выполняем очистку параметров HTTP-запроса
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
 * statusEvent Метод обратного вызова при активации ядра сервера
 * @param status флаг запуска/остановки
 */
void awh::client::Web2::statusEvent(const awh::core_t::status_t status) noexcept {
	// Если система была остановлена
	if(status == awh::core_t::status_t::STOP)
		// Выполняем удаление сессии
		this->_http2.close();
	// Выполняем передачу события в родительский объект
	web_t::statusEvent(status);
}
/**
 * connectEvent Метод обратного вызова при подключении к серверу
 * @param bid идентификатор брокера
 * @param sid идентификатор схемы сети
 */
void awh::client::Web2::connectEvent(const uint64_t bid, const uint16_t sid) noexcept {
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::OPEN, event_t::READ, event_t::PROXY_READ, event_t::CONNECT}, event_t::CONNECT))
		// Выполняем инициализацию сессии HTTP/2
		this->implementation(bid);
}
/**
 * proxyConnectEvent Метод обратного вызова при подключении к прокси-серверу
 * @param bid идентификатор брокера
 * @param sid идентификатор схемы сети
 */
void awh::client::Web2::proxyConnectEvent(const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные переданы верные
	if((bid > 0) && (sid > 0)){
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
					web_t::proxyConnectEvent(bid, sid);
				break;
				// Если прокси-сервер является HTTP/2
				case static_cast <uint8_t> (client::proxy_t::type_t::HTTPS): {
					// Если протокол подключения является HTTP/2
					if(this->_core->proto(bid) == engine_t::proto_t::HTTP2){
						// Запоминаем идентификатор брокера
						this->_bid = bid;
						// Если протокол активирован HTTPS или WSS защищённый поверх SSL
						if(this->_proxy.connect){
							// Выполняем сброс состояния HTTP-парсера
							this->_scheme.proxy.http.reset();
							// Выполняем очистку параметров HTTP-запроса
							this->_scheme.proxy.http.clear();
							// Выполняем инициализацию сессии HTTP/2
							this->implementation(bid);
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
									// Получаем бинарные данные HTTP-запроса
									const auto & buffer = this->_scheme.proxy.http.proxy(query);
									// Выводим параметры запроса
									cout << string(buffer.begin(), buffer.end()) << endl << endl;
								#endif
								// Выполняем запрос на получение заголовков
								const auto & headers = this->_scheme.proxy.http.proxy2(std::move(query));
								// Выполняем заголовки запроса на сервер
								this->_proxy.sid = this->_http2.sendHeaders(-1, headers, http2_t::flag_t::NONE);
								// Если запрос не получилось отправить
								if(this->_proxy.sid < 0){
									// Выполняем закрытие подключения
									const_cast <client::core_t *> (this->_core)->close(bid);
									// Выходим из функции
									return;
								}
							// Если инициализировать сессию HTTP/2 не удалось
							} else {
								// Выводим сообщение об ошибке подключения к прокси-серверу
								this->_log->print("Proxy server does not support the HTTP/2 protocol", log_t::flag_t::CRITICAL);
								// Если функция обратного вызова на на вывод ошибок установлена
								if(this->_callbacks.is("error"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::CRITICAL, http::error_t::PROXY_HTTP2_NO_INIT, "Proxy server does not support the HTTP/2 protocol");
								// Выполняем закрытие подключения
								const_cast <client::core_t *> (this->_core)->close(bid);
								// Выполняем завершение работы приложения
								return;
							}
						// Если протокол подключения не является защищённым подключением
						} else {
							// Выполняем переключение на работу с сервером
							this->_scheme.switchConnect();
							// Выполняем запуск работы основного модуля
							this->connectEvent(bid, sid);
						}
					// Выполняем передачу сигнала родительскому модулю
					} else web_t::proxyConnectEvent(bid, sid);
				} break;
				// Иначе завершаем работу
				default: const_cast <client::core_t *> (this->_core)->close(bid);
			}
		}
	}
}
/**
 * proxyReadEvent Метод обратного вызова при чтении сообщения с прокси-сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param bid    идентификатор брокера
 * @param sid    идентификатор схемы сети
 */
void awh::client::Web2::proxyReadEvent(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept {
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
					web_t::proxyReadEvent(buffer, size, bid, sid);
				break;
				// Если прокси-сервер является HTTP/2
				case static_cast <uint8_t> (client::proxy_t::type_t::HTTPS): {
					// Если протокол подключения является HTTP/2
					if(this->_core->proto(bid) == engine_t::proto_t::HTTP2){
						// Если прочитать данные фрейма не удалось, выходим из функции
						if(!this->_http2.frame(reinterpret_cast <const uint8_t *> (buffer), size))
							// Выходим из функции
							return;
					// Если активирован режим работы с HTTP/1.1 протоколом
					} else web_t::proxyReadEvent(buffer, size, bid, sid);
				} break;
				// Иначе завершаем работу
				default: const_cast <client::core_t *> (this->_core)->close(bid);
			}
		}
	}
}
/**
 * originCallback Метод вывода полученного списка разрешённых ресурсов для подключения
 * @param origin список разрешённых ресурсов для подключения
 */
void awh::client::Web2::originCallback(const vector <string> & origin) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callbacks.is("origin"))
		// Выполняем функцию обратного вызова
		this->_callbacks.call <void (const vector <string> &)> ("origin", origin);
}
/**
 * altsvcCallback Метод вывода полученного альтернативного сервиса от сервера
 * @param origin источник альтернативного сервиса
 * @param field  поле параметров альтернативного сервиса
 */
void awh::client::Web2::altsvcCallback(const string & origin, const string & field) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callbacks.is("altsvc"))
		// Выполняем функцию обратного вызова
		this->_callbacks.call <void (const string &, const string &)> ("altsvc", origin, field);
}
/**
 * implementation Метод выполнения активации сессии HTTP/2
 * @param bid идентификатор брокера
 */
void awh::client::Web2::implementation(const uint64_t bid) noexcept {
	// Если флаг инициализации сессии HTTP/2 не активирован, но протокол HTTP/2 поддерживается сервером
	if(!this->_http2.is() && (this->_core->proto(bid) == engine_t::proto_t::HTTP2)){
		// Если список параметров настроек не пустой
		if(!this->_settings.empty()){
			// Создаём локальный контейнер функций обратного вызова
			fn_t callbacks(this->_log);
			// Устанавливаем функцию обработки вызова на событие получения ошибок
			callbacks.set("error", this->_callbacks);
			// Выполняем установку функции обратного вызова начала открытии потока
			callbacks.set <int (const int32_t)> ("begin", std::bind(&web2_t::beginSignal, this, _1));
			// Выполняем установку функции обратного вызова получения списка разрешённых ресурсов для подключения
			callbacks.set <void (const vector <string> &)> ("origin", std::bind(&web2_t::originCallback, this, _1));
			// Выполняем установку функции обратного вызова при отправки сообщения на сервер
			callbacks.set <void (const uint8_t *, const size_t)> ("send", std::bind(&web2_t::sendSignal, this, _1, _2));
			// Выполняем установку функции обратного вызова получения альтернативного сервиса от сервера
			callbacks.set <void (const string &, const string &)> ("altsvc", std::bind(&web2_t::altsvcCallback, this, _1, _2));
			// Выполняем установку функции обратного вызова при закрытии потока
			callbacks.set <int (const int32_t, const http2_t::error_t)> ("close", std::bind(&web2_t::closedSignal, this, _1, _2));
			// Выполняем установку функции обратного вызова при получении чанка с сервера
			callbacks.set <int (const int32_t, const uint8_t *, const size_t)> ("chunk", std::bind(&web2_t::chunkSignal, this, _1, _2, _3));
			// Выполняем установку функции обратного вызова при получении данных заголовка
			callbacks.set <int (const int32_t, const string &, const string &)> ("header", std::bind(&web2_t::headerSignal, this, _1, _2, _3));
			// Выполняем установку функции обратного вызова получения фрейма
			callbacks.set <int (const int32_t, const http2_t::direct_t, const http2_t::frame_t, const set <http2_t::flag_t> &)> ("frame", std::bind(&web2_t::frameSignal, this, _1, _2, _3, _4));
			// Выполняем установку функции обратного вызова
			this->_http2.callbacks(std::move(callbacks));
			// Выполняем инициализацию модуля NgHttp2
			this->_http2.init(http2_t::mode_t::CLIENT, this->_settings);
		}
	}
}
/**
 * prepareProxy Метод выполнения препарирования полученных данных
 * @param sid идентификатор потока
 * @param bid идентификатор брокера
 * @return    результат препарирования
 */
awh::client::Web::status_t awh::client::Web2::prepareProxy(const int32_t sid, const uint64_t bid) noexcept {
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
			// Если функция обратного вызова на на вывод ошибок установлена
			if((response.code == 407) && this->_callbacks.is("error"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_RECV, "authorization failed");
			// Если попытки повторить переадресацию ещё не закончились
			if(!(this->_stopped = (this->_attempt >= this->_attempts))){
				// Если адрес запроса получен
				if(!this->_scheme.proxy.url.empty()){
					// Если соединение является постоянным
					if(this->_scheme.proxy.http.is(http_t::state_t::ALIVE)){
						// Увеличиваем количество попыток
						this->_attempt++;
						// Устанавливаем новый экшен выполнения
						this->proxyConnectEvent(bid, this->_scheme.id);
					// Если соединение не является постоянным, выполняем закрытие подключения
					} else this->close(bid);
					// Завершаем работу
					return status_t::SKIP;
				}
			}
		} break;
		// Если запрос выполнен удачно
		case static_cast <uint8_t> (awh::http_t::status_t::GOOD): {
			// Выполняем переключение на работу с сервером
			this->_scheme.switchConnect();
			// Выполняем запуск работы основного модуля
			this->connectEvent(bid, this->_scheme.id);
			// Переходим к следующему этапу
			return status_t::NEXT;
		} break;
		// Если запрос неудачный
		case static_cast <uint8_t> (awh::http_t::status_t::FAULT):
			// Устанавливаем флаг принудительной остановки
			this->_stopped = true;
		break;
	}
	// Выполняем закрытие подключения
	this->close(bid);
	// Выполняем завершение работы
	return status_t::STOP;
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
 * close Метод выполнения закрытия подключения
 * @param bid идентификатор брокера
 */
void awh::client::Web2::close(const uint64_t bid) noexcept {
	// Выполняем установку функции обратного вызова триггера, для закрытия соединения после завершения всех процессов
	this->_http2.callback <void (void)> (1, std::bind(static_cast <void (client::core_t::*)(const uint64_t)> (&client::core_t::close), const_cast <client::core_t *> (this->_core), bid));
}
/**
 * windowUpdate Метод обновления размера окна фрейма
 * @param sid  идентификатор потока
 * @param size размер нового окна
 * @return     результат установки размера офна фрейма
 */
bool awh::client::Web2::windowUpdate(const int32_t sid, const int32_t size) noexcept {
	// Результат работы функции
	bool result = false;
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::CONNECT, event_t::READ, event_t::SEND}, event_t::SEND)){
		// Если флаг инициализации сессии HTTP/2 установлен и подключение выполнено
		if((result = ((this->_core != nullptr) && this->_core->working() && (size > 0)))){
			// Выполняем отправку нового размера окна фрейма
			if(!(result = this->_http2.windowUpdate(sid, size))){
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
 * @param sid    идентификатор потока
 * @param buffer буфер бинарных данных передаваемых на сервер
 * @param size   размер сообщения в байтах
 * @param flag   флаг передаваемого потока по сети
 * @return       результат отправки данных указанному клиенту
 */
bool awh::client::Web2::send(const int32_t sid, const char * buffer, const size_t size, const http2_t::flag_t flag) noexcept {
	// Результат работы функции
	bool result = false;
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::CONNECT, event_t::READ, event_t::SEND}, event_t::SEND)){
		// Если флаг инициализации сессии HTTP/2 установлен и подключение выполнено
		if((result = ((this->_core != nullptr) && this->_core->working() && (buffer != nullptr) && (size > 0)))){
			// Выполняем отправку тела запроса на сервер
			if(!(result = this->_http2.sendData(sid, reinterpret_cast <const uint8_t *> (buffer), size, flag))){
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
 * @param sid     идентификатор потока
 * @param headers заголовки отправляемые на сервер
 * @param flag    флаг передаваемого потока по сети
 * @return        идентификатор нового запроса
 */
int32_t awh::client::Web2::send(const int32_t sid, const vector <pair <string, string>> & headers, const http2_t::flag_t flag) noexcept {
	// Результат работы функции
	int32_t result = -1;
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::CONNECT, event_t::READ, event_t::SEND}, event_t::SEND)){
		// Если флаг инициализации сессии HTTP/2 установлен и подключение выполнено
		if((this->_core != nullptr) && this->_core->working() && !headers.empty()){
			// Выполняем отправку заголовков запроса на сервер
			result = this->_http2.sendHeaders(sid, headers, flag);
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
void awh::client::Web2::settings(const map <http2_t::settings_t, uint32_t> & settings) noexcept {
	// Если список настроек протокола HTTP/2 передан
	if(!settings.empty())
		// Выполняем установку списка настроек
		this->_settings = settings;
	// Если максимальное количество потоков не установлено
	if(this->_settings.count(http2_t::settings_t::STREAMS) == 0)
		// Выполняем установку максимального количества потоков
		this->_settings.emplace(http2_t::settings_t::STREAMS, http2_t::CONCURRENT_STREAMS);
	// Если максимальный размер фрейма не установлен
	if(this->_settings.count(http2_t::settings_t::FRAME_SIZE) == 0)
		// Выполняем установку максимального размера фрейма
		this->_settings.emplace(http2_t::settings_t::FRAME_SIZE, http2_t::MAX_FRAME_SIZE_MIN);
	// Если максимальный размер фрейма установлен
	else {
		// Выполняем извлечение максимального размера фрейма
		auto it = this->_settings.find(http2_t::settings_t::FRAME_SIZE);
		// Если максимальный размер фрейма больше самого максимального значения
		if(it->second > http2_t::MAX_FRAME_SIZE_MAX)
			// Выполняем корректировку максимального размера фрейма
			it->second = http2_t::MAX_FRAME_SIZE_MAX;
		// Если максимальный размер фрейма меньше самого минимального значения
		else if(it->second < http2_t::MAX_FRAME_SIZE_MIN)
			// Выполняем корректировку максимального размера фрейма
			it->second = http2_t::MAX_FRAME_SIZE_MIN;
	}
	// Если максимальный размер окна фрейма не установлен
	if(this->_settings.count(http2_t::settings_t::WINDOW_SIZE) == 0)
		// Выполняем установку максимального размера окна фрейма
		this->_settings.emplace(http2_t::settings_t::WINDOW_SIZE, http2_t::MAX_WINDOW_SIZE);
	// Если максимальный размер буфера полезной нагрузки не установлен
	if(this->_settings.count(http2_t::settings_t::PAYLOAD_SIZE) == 0)
		// Выполняем установку максимального размера буфера полезной нагрузки
		this->_settings.emplace(http2_t::settings_t::PAYLOAD_SIZE, http2_t::MAX_PAYLOAD_SIZE);
	// Если максимальный размер блока заголовоков не установлен
	if(this->_settings.count(http2_t::settings_t::HEADER_TABLE_SIZE) == 0)
		// Выполняем установку максимального размера блока заголовоков
		this->_settings.emplace(http2_t::settings_t::HEADER_TABLE_SIZE, http2_t::HEADER_TABLE_SIZE);
	// Если флаг разрешения принимать push-уведомления не установлено
	if(this->_settings.count(http2_t::settings_t::ENABLE_PUSH) == 0)
		// Выполняем установку флага отключения принёма push-уведомлений
		this->_settings.emplace(http2_t::settings_t::ENABLE_PUSH, 0);
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
	// Если установлен флаг запрещающий переключение контекста SSL
	this->_nossl = (flags.count(flag_t::NO_INIT_SSL) > 0);
	// Устанавливаем флаг разрешающий выполнять редиректы
	this->_redirects = (flags.count(flag_t::REDIRECTS) > 0);
	// Устанавливаем флаг поддержания автоматического подключения
	this->_scheme.alive = (flags.count(flag_t::ALIVE) > 0);
	// Устанавливаем флаг ожидания входящих сообщений
	this->_scheme.wait = (flags.count(flag_t::WAIT_MESS) > 0);
	// Устанавливаем флаг разрешающий выполнять метод CONNECT для прокси-клиента
	this->_proxy.connect = (flags.count(flag_t::CONNECT_METHOD_ENABLE) > 0);
	// Если сетевое ядро установлено
	if(this->_core != nullptr)
		// Устанавливаем флаг запрещающий вывод информационных сообщений
		const_cast <client::core_t *> (this->_core)->verbose(flags.count(flag_t::NOT_INFO) == 0);
}
/**
 * userAgent Метод установки User-Agent для HTTP-запроса
 * @param userAgent агент пользователя для HTTP-запроса
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
