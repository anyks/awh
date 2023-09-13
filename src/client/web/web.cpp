/**
 * @file: web.cpp
 * @date: 2023-09-11
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
 * openCallback Метод обратного вызова при запуске работы
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::Web::openCallback(const size_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((sid > 0) && (core != nullptr)){
		// Создаём объект холдирования
		hold_t <event_t> hold(this->_events);
		// Если событие соответствует разрешённому
		if(hold.access({event_t::READ, event_t::CONNECT, event_t::DISCONNECT}, event_t::OPEN)){
			// Если подключение уже выполнено
			if(this->_scheme.status.real == scheme_t::mode_t::CONNECT){
				// Если подключение производится через, прокси-сервер
				if(this->_scheme.isProxy())
					// Выполняем запуск функции подключения для прокси-сервера
					this->proxyConnectCallback(this->_aid, sid, core);
				// Выполняем запуск функции подключения
				else this->connectCallback(this->_aid, sid, core);
			// Если биндинг уже запущен, выполняем запрос на сервер
			} else dynamic_cast <client::core_t *> (core)->open(sid);
		}
	}
}
/**
 * eventsCallback Функция обратного вызова при активации ядра сервера
 * @param status флаг запуска/остановки
 * @param core   объект сетевого ядра
 */
void awh::client::Web::eventsCallback(const awh::core_t::status_t status, awh::core_t * core) noexcept {
	// Если данные существуют
	if(core != nullptr){
		// Если функция получения событий запуска и остановки сетевого ядра установлена
		if(this->_callback.is("events"))
			// Выводим функцию обратного вызова
			this->_callback.call <const awh::core_t::status_t, awh::core_t *> ("events", status, core);
	}
}
/**
 * proxyConnectCallback Метод обратного вызова при подключении к прокси-серверу
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::Web::proxyConnectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Создаём объект холдирования
		hold_t <event_t> hold(this->_events);
		// Если событие соответствует разрешённому
		if(hold.access({event_t::OPEN, event_t::PROXY_READ}, event_t::PROXY_CONNECT)){
			// Запоминаем идентификатор адъютанта
			this->_aid = aid;
			// Определяем тип прокси-сервера
			switch(static_cast <uint8_t> (this->_scheme.proxy.type)){
				// Если прокси-сервер является Socks5
				case static_cast <uint8_t> (proxy_t::type_t::SOCKS5): {
					// Выполняем сброс состояния Socks5 парсера
					this->_scheme.proxy.socks5.reset();
					// Устанавливаем URL адрес запроса
					this->_scheme.proxy.socks5.url(this->_scheme.url);
					// Выполняем создание буфера запроса
					this->_scheme.proxy.socks5.parse();
					// Получаем данные запроса
					const auto & buffer = this->_scheme.proxy.socks5.get();
					// Если данные получены
					if(!buffer.empty())
						// Выполняем отправку сообщения на сервер
						dynamic_cast <client::core_t *> (core)->write(buffer.data(), buffer.size(), this->_aid);
				} break;
				// Если прокси-сервер является HTTP
				case static_cast <uint8_t> (proxy_t::type_t::HTTP): {
					// Выполняем сброс состояния HTTP парсера
					this->_scheme.proxy.http.reset();
					// Выполняем очистку параметров HTTP запроса
					this->_scheme.proxy.http.clear();
					// Получаем бинарные данные WEB запроса
					const auto & buffer = this->_scheme.proxy.http.proxy(this->_scheme.url);
					// Если бинарные данные запроса получены
					if(!buffer.empty()){
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим заголовок запроса
							cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST PROXY ^^^^^^^^^\x1B[0m" << endl;
							// Выводим параметры запроса
							cout << string(buffer.begin(), buffer.end()) << endl;
						#endif
						// Выполняем отправку сообщения на сервер
						dynamic_cast <client::core_t *> (core)->write(buffer.data(), buffer.size(), this->_aid);
					}
				} break;
				// Иначе завершаем работу
				default: dynamic_cast <client::core_t *> (core)->close(this->_aid);
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
void awh::client::Web::proxyReadCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (sid > 0)){
		// Создаём объект холдирования
		hold_t <event_t> hold(this->_events);
		// Если событие соответствует разрешённому
		if(hold.access({event_t::PROXY_CONNECT}, event_t::PROXY_READ)){
			// Добавляем полученные данные в буфер
			this->_buffer.insert(this->_buffer.end(), buffer, buffer + size);
			// Определяем тип прокси-сервера
			switch(static_cast <uint8_t> (this->_scheme.proxy.type)){
				// Если прокси-сервер является Socks5
				case static_cast <uint8_t> (proxy_t::type_t::SOCKS5): {
					// Если данные не получены
					if(!this->_scheme.proxy.socks5.isEnd()){
						// Выполняем парсинг входящих данных
						this->_scheme.proxy.socks5.parse(this->_buffer.data(), this->_buffer.size());
						// Получаем данные запроса
						const auto & buffer = this->_scheme.proxy.socks5.get();
						// Если данные получены
						if(!buffer.empty()){
							// Выполняем очистку буфера данных
							this->_buffer.clear();
							// Выполняем отправку запроса на сервер
							dynamic_cast <client::core_t *> (core)->write(buffer.data(), buffer.size(), this->_aid);
							// Завершаем работу
							return;
						// Если данные все получены
						} else if(this->_scheme.proxy.socks5.isEnd()) {
							// Выполняем очистку буфера данных
							this->_buffer.clear();
							// Если рукопожатие выполнено
							if(this->_scheme.proxy.socks5.isHandshake()){
								// Выполняем переключение на работу с сервером
								dynamic_cast <client::core_t *> (core)->switchProxy(this->_aid);
								// Завершаем работу
								return;
							// Если рукопожатие не выполнено
							} else {
								// Устанавливаем код ответа
								const u_int code = this->_scheme.proxy.socks5.code();
								// Устанавливаем сообщение ответа
								const string & message = this->_scheme.proxy.socks5.message(code);
								/**
								 * Если включён режим отладки
								 */
								#if defined(DEBUG_MODE)
									// Если заголовки получены
									if(!message.empty()){
										// Данные WEB ответа
										const string & answer = this->_fmk->format("SOCKS5 %u %s\r\n", code, message.c_str());
										// Выводим заголовок ответа
										cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE PROXY ^^^^^^^^^\x1B[0m" << endl;
										// Выводим параметры ответа
										cout << string(answer.begin(), answer.end()) << endl;
									}
								#endif
								// Если функция обратного вызова на вывод ответа сервера на ранее выполненный запрос установлена
								if(this->_callback.is("response"))
									// Выводим функцию обратного вызова
									this->_callback.call <const int32_t, const u_int, const string &> ("response", 1, code, message);
								// Завершаем работу
								dynamic_cast <client::core_t *> (core)->close(this->_aid);
								// Завершаем работу
								return;
							}
						}
					}
				} break;
				// Если прокси-сервер является HTTP
				case static_cast <uint8_t> (proxy_t::type_t::HTTP): {
					// Выполняем парсинг полученных данных
					this->_scheme.proxy.http.parse(this->_buffer.data(), this->_buffer.size());
					// Если все данные получены
					if(this->_scheme.proxy.http.isEnd()){
						// Выполняем очистку буфера данных
						this->_buffer.clear();
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							{
								// Получаем данные ответа
								const auto & response = this->_scheme.proxy.http.response(true);
								// Если параметры ответа получены
								if(!response.empty()){
									// Выводим заголовок ответа
									cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE PROXY ^^^^^^^^^\x1B[0m" << endl;
									// Выводим параметры ответа
									cout << string(response.begin(), response.end()) << endl;
									// Если тело ответа существует
									if(!this->_scheme.proxy.http.body().empty())
										// Выводим сообщение о выводе чанка тела
										cout << this->_fmk->format("<body %u>", this->_scheme.proxy.http.body().size()) << endl;
								}
							}
						#endif
						// Получаем параметры запроса
						const auto & query = this->_scheme.proxy.http.query();
						// Получаем статус ответа
						awh::http_t::stath_t status = this->_scheme.proxy.http.getAuth();
						// Если выполнять редиректы запрещено
						if(!this->_redirects && (status == awh::http_t::stath_t::RETRY)){
							// Если нужно произвести запрос заново
							if((query.code == 201) || (query.code == 301) ||
							   (query.code == 302) || (query.code == 303) ||
							   (query.code == 307) || (query.code == 308))
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
										// Увеличиваем количество попыток
										this->_attempt++;
										// Если соединение является постоянным
										if(this->_scheme.proxy.http.isAlive())
											// Устанавливаем новый экшен выполнения
											this->proxyConnectCallback(aid, sid, core);
										// Если соединение должно быть закрыто
										else dynamic_cast <client::core_t *> (core)->close(this->_aid);
										// Завершаем работу
										return;
									}
								}
							} break;
							// Если запрос выполнен удачно
							case static_cast <uint8_t> (awh::http_t::stath_t::GOOD): {
								// Выполняем сброс количества попыток
								this->_attempt = 0;
								// Выполняем переключение на работу с сервером
								dynamic_cast <client::core_t *> (core)->switchProxy(this->_aid);
								// Завершаем работу
								return;
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
							this->_callback.call <const int32_t, const u_int, const string &> ("response", 1, query.code, query.message);
						// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
						if(this->_callback.is("headers"))
							// Выводим функцию обратного вызова
							this->_callback.call <const int32_t, const u_int, const string &, const unordered_multimap <string, string> &> ("headers", 1, query.code, query.message, this->_scheme.proxy.http.headers());
						// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
						if(this->_callback.is("entity"))
							// Выводим функцию обратного вызова
							this->_callback.call <const int32_t, const u_int, const string &, const vector <char> &> ("entity", 1, query.code, query.message, this->_scheme.proxy.http.body());
						// Завершаем работу
						dynamic_cast <client::core_t *> (core)->close(this->_aid);
						// Завершаем работу
						return;
					}
				} break;
				// Иначе завершаем работу
				default: dynamic_cast <client::core_t *> (core)->close(this->_aid);
			}
		}
	}
}
/**
 * enableTLSCallback Метод активации зашифрованного канала TLS
 * @param url  адрес сервера для которого выполняется активация зашифрованного канала TLS
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 * @return     результат активации зашифрованного канала TLS
 */
bool awh::client::Web::enableTLSCallback(const uri_t::url_t & url, const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Блокируем переменные которые не используем
	(void) aid;
	(void) sid;
	(void) core;
	// Выполняем проверку, выполняется подключение к серверу в защищённом рижеме или нет
	return (!url.empty() && (this->_fmk->compare(url.schema, "https") || this->_fmk->compare(url.schema, "wss")));
}
/**
 * chunking Метод обработки получения чанков
 * @param chunk бинарный буфер чанка
 * @param http  объект модуля HTTP
 */
void awh::client::Web::chunking(const vector <char> & chunk, const awh::http_t * http) noexcept {
	// Если данные получены, формируем тело сообщения
	if(!chunk.empty())
		// Выполняем добавление полученного чанка в тело ответа
		const_cast <awh::http_t *> (http)->body(chunk);
}
/**
 * init Метод инициализации WEB клиента
 * @param dest     адрес назначения удалённого сервера
 * @param compress метод компрессии передаваемых сообщений
 */
void awh::client::Web::init(const string & dest, const http_t::compress_t compress) noexcept {
	// Если unix-сокет установлен
	if(this->_core->family() == scheme_t::family_t::NIX){
		// Выполняем очистку схемы сети
		this->_scheme.clear();
		// Устанавливаем метод компрессии сообщений
		this->_compress = compress;
		// Устанавливаем unix-сокет адрес в файловой системе
		this->_scheme.url = this->_uri.parse(this->_fmk->format("unix:%s.sock", dest.c_str()));
		/**
		 * Если операционной системой не является Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Выполняем установку unix-сокета 
			const_cast <client::core_t *> (this->_core)->unixSocket(dest);
		#endif
	// Выполняем установку unix-сокет
	} else {
		// Если адрес сервера передан
		if(!dest.empty()){
			// Выполняем очистку схемы сети
			this->_scheme.clear();
			// Устанавливаем URL-адрес запроса
			this->_scheme.url = this->_uri.parse(dest);
			/**
			 * Если операционной системой не является Windows
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				// Удаляем unix-сокет ранее установленный
				const_cast <client::core_t *> (this->_core)->removeUnixSocket();
			#endif
		}
	}
	// Устанавливаем метод компрессии сообщений
	this->_compress = compress;
}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param callback функция обратного вызова
 */
void awh::client::Web::on(function <void (const mode_t)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const mode_t)> ("active", callback);
}
/**
 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
 * @param callback функция обратного вызова
 */
void awh::client::Web::on(function <void (const awh::core_t::status_t, awh::core_t *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const awh::core_t::status_t, awh::core_t *)> ("events", callback);
}
/**
 * on Метод установки функции вывода полученного чанка бинарных данных с сервера
 * @param callback функция обратного вызова
 */
void awh::client::Web::on(function <void (const int32_t, const vector <char> &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const int32_t, const vector <char> &)> ("chunks", callback);
}
/**
 * on Метод установки функции вывода полученного тела данных с сервера
 * @param callback функция обратного вызова
 */
void awh::client::Web::on(function <void (const int32_t, const u_int, const string &, const vector <char> &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const int32_t, const u_int, const string &, const vector <char> &)> ("entity", callback);
}
/**
 * on Метод установки функции обратного вызова для перехвата полученных чанков
 * @param callback функция обратного вызова
 */
void awh::client::Web::on(function <void (const vector <char> &, const awh::http_t *)> callback) noexcept {
	/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ Подключить к динамическим объектам HTTP
	// Устанавливаем функцию обратного вызова для WebSocket/1.1
	this->_ws.http.on(callback);
	// Устанавливаем функцию обратного вызова для HTTP/1.1
	this->_web.http.on(callback);
	*/
	// Устанавливаем функцию обратного вызова для HTTP/2 и WebSocket/2
	this->_callback.set <void (const vector <char> &, const awh::http_t *)> ("chunking", callback);
}
/**
 * on Метод установки функции вывода ответа сервера на ранее выполненный запрос
 * @param callback функция обратного вызова
 */
void awh::client::Web::on(function <void (const int32_t, const u_int, const string &)> callback) noexcept {
	/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ Подключить к динамическим объектам HTTP
	// Устанавливаем функцию обратного вызова для HTTP/1.1
	this->_ws.http.on(callback);
	this->_web.http.on(callback);
	*/
	// Устанавливаем функцию обратного вызова для HTTP/2
	this->_callback.set <void (const int32_t, const u_int, const string &)> ("response", callback);
}
/**
 * on Метод установки функции вывода полученного заголовка с сервера
 * @param callback функция обратного вызова
 */
void awh::client::Web::on(function <void (const int32_t, const string &, const string &)> callback) noexcept {
	/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ Подключить к динамическим объектам HTTP
	// Устанавливаем функцию обратного вызова для HTTP/1.1
	this->_ws.http.on(callback);
	this->_web.http.on(callback);
	*/
	// Устанавливаем функцию обратного вызова для HTTP/2
	this->_callback.set <void (const int32_t, const string &, const string &)> ("header", callback);
}
/**
 * on Метод установки функции вывода полученных заголовков с сервера
 * @param callback функция обратного вызова
 */
void awh::client::Web::on(function <void (const int32_t, const u_int, const string &, const unordered_multimap <string, string> &)> callback) noexcept {
	/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ Подключить к динамическим объектам HTTP
	// Устанавливаем функцию обратного вызова для HTTP/1.1
	this->_ws.http.on(callback);
	this->_web.http.on(callback);
	*/
	// Устанавливаем функцию обратного вызова для HTTP/2
	this->_callback.set <void (const int32_t, const u_int, const string &, const unordered_multimap <string, string> &)> ("headers", callback);
}
/**
 * sendTimeout Метод отправки сигнала таймаута
 */
void awh::client::Web::sendTimeout() noexcept {
	// Если подключение выполнено
	if(this->_core->working())
		// Отправляем сигнал принудительного таймаута
		const_cast <client::core_t *> (this->_core)->sendTimeout(this->_aid);
}
/**
 * open Метод открытия подключения
 */
void awh::client::Web::open() noexcept {
	// Выполняем открытие подключения на удалённом сервере
	this->openCallback(this->_scheme.sid, dynamic_cast <awh::core_t *> (const_cast <client::core_t *> (this->_core)));
}
/**
 * stop Метод остановки клиента
 */
void awh::client::Web::stop() noexcept {
	// Устанавливаем флаг принудительной остановки
	this->_active = true;
	// Если подключение выполнено
	if(this->_core->working()){
		// Очищаем адрес сервера
		this->_scheme.url.clear();
		// Завершаем работу, если разрешено остановить
		if(this->_unbind) const_cast <client::core_t *> (this->_core)->stop();
		// Если завершать работу запрещено, просто отключаемся
		else {
			/**
			 * Если установлено постоянное подключение
			 * нам нужно заблокировать автоматический реконнект.
			 */
			// Считываем значение флага
			const bool alive = this->_scheme.alive;
			// Выполняем отключение флага постоянного подключения
			this->_scheme.alive = false;
			// Выполняем отключение клиента
			const_cast <client::core_t *> (this->_core)->close(this->_aid);
			// Восстанавливаем предыдущее значение флага
			this->_scheme.alive = alive;
		}
	}
}
/**
 * start Метод запуска клиента
 */
void awh::client::Web::start() noexcept {
	// Если адрес URL запроса передан
	if(!this->_scheme.url.empty()){
		// Если биндинг не запущен
		if(!this->_core->working())
			// Выполняем запуск биндинга
			const_cast <client::core_t *> (this->_core)->start();
		// Если биндинг уже запущен, выполняем запрос на сервер
		else this->open();
	}
}
/**
 * bytesDetect Метод детекции сообщений по количеству байт
 * @param read  количество байт для детекции по чтению
 * @param write количество байт для детекции по записи
 */
void awh::client::Web::bytesDetect(const scheme_t::mark_t read, const scheme_t::mark_t write) noexcept {
	// Устанавливаем количество байт на чтение
	this->_scheme.marker.read = read;
	// Устанавливаем количество байт на запись
	this->_scheme.marker.write = write;
	// Если минимальный размер данных для чтения, не установлен
	if(this->_scheme.marker.read.min == 0)
		// Устанавливаем размер минимальных для чтения данных по умолчанию
		this->_scheme.marker.read.min = BUFFER_READ_MIN;
	// Если максимальный размер данных для записи не установлен, устанавливаем по умолчанию
	if(this->_scheme.marker.write.max == 0)
		// Устанавливаем размер максимальных записываемых данных по умолчанию
		this->_scheme.marker.write.max = BUFFER_WRITE_MAX;
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read    количество секунд для детекции по чтению
 * @param write   количество секунд для детекции по записи
 * @param connect количество секунд для детекции по подключению
 */
void awh::client::Web::waitTimeDetect(const time_t read, const time_t write, const time_t connect) noexcept {
	// Устанавливаем количество секунд на чтение
	this->_scheme.timeouts.read = read;
	// Устанавливаем количество секунд на запись
	this->_scheme.timeouts.write = write;
	// Устанавливаем количество секунд на подключение
	this->_scheme.timeouts.connect = connect;
}
/**
 * proxy Метод установки прокси-сервера
 * @param uri    параметры прокси-сервера
 * @param family семейстово интернет протоколов (IPV4 / IPV6 / NIX)
 */
void awh::client::Web::proxy(const string & uri, const scheme_t::family_t family) noexcept {
	// Если URI параметры переданы
	if(!uri.empty()){
		// Устанавливаем семейство интернет протоколов
		this->_scheme.proxy.family = family;
		// Устанавливаем параметры прокси-сервера
		this->_scheme.proxy.url = this->_uri.parse(uri);
		// Если данные параметров прокси-сервера получены
		if(!this->_scheme.proxy.url.empty()){
			// Если протокол подключения SOCKS5
			if(this->_fmk->compare(this->_scheme.proxy.url.schema, "socks5")){
				// Устанавливаем тип прокси-сервера
				this->_scheme.proxy.type = proxy_t::type_t::SOCKS5;
				// Если требуется авторизация на прокси-сервере
				if(!this->_scheme.proxy.url.user.empty() && !this->_scheme.proxy.url.pass.empty())
					// Устанавливаем данные пользователя
					this->_scheme.proxy.socks5.user(this->_scheme.proxy.url.user, this->_scheme.proxy.url.pass);
			// Если протокол подключения HTTP
			} else if(this->_fmk->compare(this->_scheme.proxy.url.schema, "http") || this->_fmk->compare(this->_scheme.proxy.url.schema, "https")) {
				// Устанавливаем тип прокси-сервера
				this->_scheme.proxy.type = proxy_t::type_t::HTTP;
				// Если требуется авторизация на прокси-сервере
				if(!this->_scheme.proxy.url.user.empty() && !this->_scheme.proxy.url.pass.empty())
					// Устанавливаем данные пользователя
					this->_scheme.proxy.http.user(this->_scheme.proxy.url.user, this->_scheme.proxy.url.pass);
			}
		}
	}
}
/**
 * attempts Метод установки общего количества попыток
 * @param attempts общее количество попыток
 */
void awh::client::Web::attempts(const uint8_t attempts) noexcept {
	// Если количество попыток передано, устанавливаем его
	if(attempts > 0) this->_attempts = attempts;
}
/**
 * core Метод установки сетевого ядра
 * @param core объект сетевого ядра
 */
void awh::client::Web::core(const client::core_t * core) noexcept {
	// Если объект сетевого ядра передан
	if(core != nullptr){
		// Выполняем установку объекта сетевого ядра
		this->_core = core;
		// Добавляем схемы сети в сетевое ядро
		const_cast <client::core_t *> (this->_core)->add(&this->_scheme);
		// Устанавливаем функцию активации ядра клиента
		const_cast <client::core_t *> (this->_core)->callback(std::bind(&web_t::eventsCallback, this, _1, _2));
	// Если объект сетевого ядра не передан но ранее оно было добавлено
	} else if(this->_core != nullptr) {
		// Устанавливаем функцию активации ядра клиента
		const_cast <client::core_t *> (this->_core)->callback(nullptr);
		// Выполняем установку объекта сетевого ядра
		this->_core = core;
	}
}
/**
 * compress Метод установки метода компрессии
 * @param compress метод компрессии сообщений
 */
void awh::client::Web::compress(const http_t::compress_t compress) noexcept {
	// Устанавливаем метод компрессии
	this->_compress = compress;
}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::client::Web::keepAlive(const int cnt, const int idle, const int intvl) noexcept {
	// Выполняем установку максимального количества попыток
	this->_scheme.keepAlive.cnt = cnt;
	// Выполняем установку интервала времени в секундах через которое происходит проверка подключения
	this->_scheme.keepAlive.idle = idle;
	// Выполняем установку интервала времени в секундах между попытками
	this->_scheme.keepAlive.intvl = intvl;
}
/**
 * userAgent Метод установки User-Agent для HTTP запроса
 * @param userAgent агент пользователя для HTTP запроса
 */
void awh::client::Web::userAgent(const string & userAgent) noexcept {
	// Устанавливаем UserAgent
	if(!userAgent.empty()){
		/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ Подключить к динамическим объектам HTTP
		// Устанавливаем пользовательского агента
		this->_ws.http.userAgent(userAgent);
		// Устанавливаем пользовательского агента
		this->_web.http.userAgent(userAgent);
		*/
		// Устанавливаем пользовательского агента для прокси-сервера
		this->_scheme.proxy.http.userAgent(userAgent);
	}
}
/**
 * serv Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::client::Web::serv(const string & id, const string & name, const string & ver) noexcept {
	/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ Подключить к динамическим объектам HTTP
	// Устанавливаем данные сервиса
	this->_ws.http.serv(id, name, ver);
	// Устанавливаем данные сервиса
	this->_web.http.serv(id, name, ver);
	*/
	// Устанавливаем данные сервиса для прокси-сервера
	this->_scheme.proxy.http.serv(id, name, ver);
}
/**
 * authTypeProxy Метод установки типа авторизации прокси-сервера
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest-авторизации
 */
void awh::client::Web::authTypeProxy(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Если объект авторизации создан
	this->_scheme.proxy.http.authType(type, hash);
}
/**
 * Web Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::client::Web::Web(const fmk_t * fmk, const log_t * log) noexcept :
 _aid(0), _uri(fmk), _callback(log), _scheme(fmk, log),
 _unbind(true), _active(false), _stopped(false), _redirects(false),
 _attempt(0), _attempts(15), _compress(awh::http_t::compress_t::NONE), _fmk(fmk), _log(log) {
	// Устанавливаем функцию обработки вызова для получения чанков для HTTP-клиента
	this->_scheme.proxy.http.on(std::bind(&web_t::chunking, this, _1, _2));
	// Устанавливаем событие на запуск системы
	this->_scheme.callback.set <void (const size_t, awh::core_t *)> ("open", std::bind(&web_t::openCallback, this, _1, _2));
	// Устанавливаем событие подключения
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("connect", std::bind(&web_t::connectCallback, this, _1, _2, _3));
	// Устанавливаем событие отключения
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("disconnect", std::bind(&web_t::disconnectCallback, this, _1, _2, _3));
	// Устанавливаем событие на подключение к прокси-серверу
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("connectProxy", std::bind(&web_t::proxyConnectCallback, this, _1, _2, _3));
	// Устанавливаем функцию чтения данных
	this->_scheme.callback.set <void (const char *, const size_t, const size_t, const size_t, awh::core_t *)> ("read", std::bind(&web_t::readCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем событие на чтение данных с прокси-сервера
	this->_scheme.callback.set <void (const char *, const size_t, const size_t, const size_t, awh::core_t *)> ("readProxy", std::bind(&web_t::proxyReadCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем событие на активацию шифрованного TLS канала
	this->_scheme.callback.set <bool (const uri_t::url_t &, const size_t, const size_t, awh::core_t *)> ("tls", std::bind(&web_t::enableTLSCallback, this, _1, _2, _3, _4));
}
/**
 * Web Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::client::Web::Web(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 _aid(0), _uri(fmk), _callback(log), _scheme(fmk, log),
 _unbind(true), _active(false), _stopped(false), _redirects(false),
 _attempt(0), _attempts(15), _compress(awh::http_t::compress_t::NONE),
 _fmk(fmk), _log(log), _core(core) {
	// Устанавливаем функцию обработки вызова для получения чанков для HTTP-клиента
	this->_scheme.proxy.http.on(std::bind(&web_t::chunking, this, _1, _2));
	// Устанавливаем событие на запуск системы
	this->_scheme.callback.set <void (const size_t, awh::core_t *)> ("open", std::bind(&web_t::openCallback, this, _1, _2));
	// Устанавливаем событие подключения
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("connect", std::bind(&web_t::connectCallback, this, _1, _2, _3));
	// Устанавливаем событие отключения
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("disconnect", std::bind(&web_t::disconnectCallback, this, _1, _2, _3));
	// Устанавливаем событие на подключение к прокси-серверу
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("connectProxy", std::bind(&web_t::proxyConnectCallback, this, _1, _2, _3));
	// Устанавливаем функцию чтения данных
	this->_scheme.callback.set <void (const char *, const size_t, const size_t, const size_t, awh::core_t *)> ("read", std::bind(&web_t::readCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем событие на чтение данных с прокси-сервера
	this->_scheme.callback.set <void (const char *, const size_t, const size_t, const size_t, awh::core_t *)> ("readProxy", std::bind(&web_t::proxyReadCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем событие на активацию шифрованного TLS канала
	this->_scheme.callback.set <bool (const uri_t::url_t &, const size_t, const size_t, awh::core_t *)> ("tls", std::bind(&web_t::enableTLSCallback, this, _1, _2, _3, _4));
	// Добавляем схемы сети в сетевое ядро
	const_cast <client::core_t *> (this->_core)->add(&this->_scheme);
	// Устанавливаем функцию активации ядра клиента
	const_cast <client::core_t *> (this->_core)->callback(std::bind(&web_t::eventsCallback, this, _1, _2));
}
