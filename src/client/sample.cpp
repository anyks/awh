/**
 * @file: sample.cpp
 * @date: 2022-09-01
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

/**
 * Подключаем заголовочный файл
 */
#include <client/sample.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * openEvent Метод обратного вызова при запуске работы
 * @param sid идентификатор схемы сети
 */
void awh::client::Sample::openEvent(const uint16_t sid) noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && (sid > 0)){
		// Создаём объект холдирования
		hold_t <event_t> hold(this->_events);
		// Если событие соответствует разрешённому
		if(hold.access({event_t::READ, event_t::CONNECT}, event_t::OPEN)){
			// Если подключение уже выполнено
			if(this->_scheme.status.real == scheme_t::mode_t::CONNECT){
				// Если подключение производится через, прокси-сервер
				if(this->_scheme.isProxy())
					// Выполняем запуск функции подключения для прокси-сервера
					this->proxyConnectEvent(this->_bid, sid);
				// Выполняем запуск функции подключения
				else this->connectEvent(this->_bid, sid);
			// Если биндинг уже запущен, выполняем запрос на сервер
			} else const_cast <client::core_t *> (this->_core)->open(sid);
		}
	}
}
/**
 * statusEvent Метод обратного вызова при активации ядра сервера
 * @param status флаг запуска/остановки
 */
void awh::client::Sample::statusEvent(const awh::core_t::status_t status) noexcept {
	// Если функция получения событий запуска и остановки сетевого ядра установлена
	if(this->_callbacks.is("status"))
		// Выводим функцию обратного вызова
		this->_callbacks.call <void (const awh::core_t::status_t)> ("status", status);
}
/**
 * connectEvent Метод обратного вызова при подключении к серверу
 * @param bid идентификатор брокера
 * @param sid идентификатор схемы сети
 */
void awh::client::Sample::connectEvent(const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные переданы верные
	if((bid > 0) && (sid > 0)){
		// Создаём объект холдирования
		hold_t <event_t> hold(this->_events);
		// Если событие соответствует разрешённому
		if(hold.access({event_t::OPEN, event_t::READ, event_t::PROXY_READ}, event_t::CONNECT)){
			// Запоминаем идентификатор брокера
			this->_bid = bid;
			// Если функция обратного вызова существует
			if(this->_callbacks.is("active"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const mode_t)> ("active", mode_t::CONNECT);
		}
	}
}
/**
 * disconnectEvent Метод обратного вызова при отключении от сервера
 * @param bid идентификатор брокера
 * @param sid идентификатор схемы сети
 */
void awh::client::Sample::disconnectEvent(const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && (sid > 0)){
		// Если подключение не является постоянным
		if(!this->_scheme.alive){
			// Очищаем адрес сервера
			this->_scheme.url.clear();
			// Завершаем работу базы событий
			if(this->_complete)
				// Выполняем остановку работы сетевого ядра
				const_cast <client::core_t *> (this->_core)->stop();
		}
		// Если функция обратного вызова существует
		if(this->_callbacks.is("active"))
			// Выполняем функцию обратного вызова
			this->_callbacks.call <void (const mode_t)> ("active", mode_t::DISCONNECT);
	}
}
/**
 * readEvent Метод обратного вызова при чтении сообщения с сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param bid    идентификатор брокера
 * @param sid    идентификатор схемы сети
 */
void awh::client::Sample::readEvent(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (bid > 0) && (sid > 0)){
		// Создаём объект холдирования
		hold_t <event_t> hold(this->_events);
		// Если событие соответствует разрешённому
		if(hold.access({event_t::CONNECT}, event_t::READ)){
			// Если функция обратного вызова существует
			if(this->_callbacks.is("message"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const vector <char> &)> ("message", vector <char> (buffer, buffer + size));
		}
	}
}
/**
 * enableSSLEvent Метод активации зашифрованного канала SSL
 * @param url адрес сервера для которого выполняется активация зашифрованного канала SSL
 * @param bid идентификатор брокера
 * @param sid идентификатор схемы сети
 * @return    результат активации зашифрованного канала SSL
 */
bool awh::client::Sample::enableSSLEvent([[maybe_unused]] const uri_t::url_t & url, [[maybe_unused]] const uint64_t bid, [[maybe_unused]] const uint16_t sid) noexcept {
	// Если объект сетевого ядра установлен
	if(this->_core != nullptr){
		// Получаем тип активного протокола
		const scheme_t::sonet_t sonet = this->_core->sonet();
		// Выполняем проверку, выполняется подключение к серверу в защищённом рижеме или нет
		return ((sonet == scheme_t::sonet_t::TLS) || (sonet == scheme_t::sonet_t::SCTP));
	}
	// Выводим значение по умолчанию
	return false;
}
/**
 * chunking Метод обработки получения чанков
 * @param bid   идентификатор брокера
 * @param chunk бинарный буфер чанка
 * @param http  объект модуля HTTP
 */
void awh::client::Sample::chunking([[maybe_unused]] const uint64_t bid, const vector <char> & chunk, const awh::http_t * http) noexcept {
	// Если данные получены, формируем тело сообщения
	if(!chunk.empty())
		// Выполняем добавление полученного чанка в тело ответа
		const_cast <awh::http_t *> (http)->body(chunk);
}
/**
 * proxyConnectEvent Метод обратного вызова при подключении к прокси-серверу
 * @param bid идентификатор брокера
 * @param sid идентификатор схемы сети
 */
void awh::client::Sample::proxyConnectEvent(const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && (bid > 0) && (sid > 0)){
		// Создаём объект холдирования
		hold_t <event_t> hold(this->_events);
		// Если событие соответствует разрешённому
		if(hold.access({event_t::OPEN, event_t::PROXY_READ, event_t::PROXY_CONNECT}, event_t::PROXY_CONNECT)){
			// Запоминаем идентификатор брокера
			this->_bid = bid;
			// Определяем тип прокси-сервера
			switch(static_cast <uint8_t> (this->_scheme.proxy.type)){
				// Если прокси-сервер является Socks5
				case static_cast <uint8_t> (client::proxy_t::type_t::SOCKS5): {
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
						const_cast <client::core_t *> (this->_core)->send(buffer.data(), buffer.size(), bid);
				} break;
				// Если прокси-сервер является HTTP
				case static_cast <uint8_t> (client::proxy_t::type_t::HTTP): {
					// Выполняем сброс состояния HTTP-парсера
					this->_scheme.proxy.http.reset();
					// Выполняем очистку параметров HTTP-запроса
					this->_scheme.proxy.http.clear();
					// Создаём объек запроса
					awh::web_t::req_t request(awh::web_t::method_t::CONNECT, this->_scheme.url);
					// Получаем бинарные данные WEB запроса
					const auto & buffer = this->_scheme.proxy.http.proxy(request);
					// Если бинарные данные запроса получены
					if(!buffer.empty()){
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим заголовок запроса
							std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST PROXY ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
							// Выводим параметры запроса
							std::cout << string(buffer.begin(), buffer.end()) << std::endl << std::endl << std::flush;
						#endif
						// Выполняем отправку сообщения на сервер
						const_cast <client::core_t *> (this->_core)->send(buffer.data(), buffer.size(), bid);
					}
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
void awh::client::Sample::proxyReadEvent(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные существуют
	if((this->_core != nullptr) && (buffer != nullptr) && (size > 0) && (bid > 0) && (sid > 0)){
		// Создаём объект холдирования
		hold_t <event_t> hold(this->_events);
		// Если событие соответствует разрешённому
		if(hold.access({event_t::PROXY_CONNECT, event_t::PROXY_READ}, event_t::PROXY_READ)){
			// Добавляем полученные данные в буфер
			this->_buffer.push(buffer, size);
			// Определяем тип прокси-сервера
			switch(static_cast <uint8_t> (this->_scheme.proxy.type)){
				// Если прокси-сервер является Socks5
				case static_cast <uint8_t> (client::proxy_t::type_t::SOCKS5): {
					// Если данные не получены
					if(!this->_scheme.proxy.socks5.is(socks5_t::state_t::END)){
						// Выполняем парсинг входящих данных
						this->_scheme.proxy.socks5.parse(reinterpret_cast <const char *> (this->_buffer.get()), this->_buffer.size());
						// Получаем данные запроса
						const auto & buffer = this->_scheme.proxy.socks5.get();
						// Если данные получены
						if(!buffer.empty()){
							// Выполняем очистку буфера данных
							this->_buffer.clear();
							// Выполняем отправку запроса на сервер
							const_cast <client::core_t *> (this->_core)->send(buffer.data(), buffer.size(), bid);
							// Завершаем работу
							return;
						// Если данные все получены
						} else if(this->_scheme.proxy.socks5.is(socks5_t::state_t::END)) {
							// Выполняем очистку буфера данных
							this->_buffer.clear();
							// Если рукопожатие выполнено
							if(this->_scheme.proxy.socks5.is(socks5_t::state_t::HANDSHAKE)){
								// Выполняем переключение на работу с сервером
								const_cast <client::core_t *> (this->_core)->switchProxy(bid);
								// Выполняем запуск функции подключения
								this->connectEvent(bid, sid);
								// Завершаем работу
								return;
							// Если рукопожатие не выполнено
							} else {
								// Устанавливаем код ответа
								const uint32_t code = this->_scheme.proxy.socks5.code();
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
										std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE PROXY ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
										// Выводим параметры ответа
										std::cout << string(answer.begin(), answer.end()) << std::endl << std::endl << std::flush;
									}
								#endif
								// Завершаем работу
								const_cast <client::core_t *> (this->_core)->close(bid);
								// Завершаем работу
								return;
							}
						}
					}
				} break;
				// Если прокси-сервер является HTTP
				case static_cast <uint8_t> (client::proxy_t::type_t::HTTP): {
					// Выполняем обработку полученных данных
					while(this->_reading){
						// Выполняем парсинг полученных данных
						const size_t bytes = this->_scheme.proxy.http.parse(reinterpret_cast <const char *> (this->_buffer.get()), this->_buffer.size());
						// Если все данные получены
						if((bytes > 0) && this->_scheme.proxy.http.is(http_t::state_t::END)){
							// Выполняем очистку буфера данных
							this->_buffer.clear();
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								{
									// Получаем данные ответа
									const auto & response = this->_scheme.proxy.http.process(http_t::process_t::RESPONSE, this->_scheme.proxy.http.response());
									// Если параметры ответа получены
									if(!response.empty()){
										// Выводим заголовок ответа
										std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE PROXY ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
										// Выводим параметры ответа
										std::cout << string(response.begin(), response.end()) << std::endl << std::endl << std::flush;
										// Если тело ответа существует
										if(!this->_scheme.proxy.http.empty(awh::http_t::suite_t::BODY))
											// Выводим сообщение о выводе чанка тела
											std::cout << this->_fmk->format("<body %u>", this->_scheme.proxy.http.body().size()) << std::endl << std::endl << std::flush;
									}
								}
							#endif
							// Получаем параметры запроса
							const auto & response = this->_scheme.proxy.http.response();
							// Получаем статус ответа
							awh::http_t::status_t status = this->_scheme.proxy.http.auth();
							// Устанавливаем ответ прокси-сервера
							this->_proxy.answer = response.code;
							// Если выполнять редиректы запрещено
							if(status == awh::http_t::status_t::RETRY){
								// Если ответом сервера является положительным
								if(response.code == 200)
									// Запрещаем выполнять редирект
									status = awh::http_t::status_t::GOOD;
							}
							// Выполняем проверку авторизации
							switch(static_cast <uint8_t> (status)){
								// Если нужно попытаться ещё раз
								case static_cast <uint8_t> (awh::http_t::status_t::RETRY): {
									// Если попытки повторить переадресацию ещё не закончились
									if(this->_attempt < this->_attempts){
										// Если адрес запроса получен
										if(!this->_scheme.proxy.url.empty()){
											// Если соединение является постоянным
											if(this->_scheme.proxy.http.is(http_t::state_t::ALIVE)){
												// Увеличиваем количество попыток
												this->_attempt++;
												// Устанавливаем новый экшен выполнения
												this->proxyConnectEvent(bid, sid);
											// Если соединение должно быть закрыто
											} else const_cast <client::core_t *> (this->_core)->close(bid);
											// Завершаем работу
											return;
										}
									}
								} break;
								// Если запрос выполнен удачно
								case static_cast <uint8_t> (awh::http_t::status_t::GOOD): {
									// Выполняем переключение на работу с сервером
									const_cast <client::core_t *> (this->_core)->switchProxy(bid);
									// Выполняем запуск функции подключения
									this->connectEvent(bid, sid);
									// Завершаем работу
									return;
								}
								// Если запрос неудачный
								case static_cast <uint8_t> (awh::http_t::status_t::FAULT):
									// Выводим в лог сообщение
									this->_log->print("Proxy server error", log_t::flag_t::CRITICAL);
								break;
							}
							// Завершаем работу
							const_cast <client::core_t *> (this->_core)->close(bid);
							// Завершаем работу
							return;
						}
						// Если парсер обработал какое-то количество байт
						if((bytes > 0) && !this->_buffer.empty()){
							// Если размер буфера больше количества удаляемых байт
							if(this->_buffer.size() >= bytes)
								// Удаляем количество обработанных байт
								this->_buffer.erase(bytes);
						}
						// Если данные мы все получили, выходим
						if(this->_buffer.empty())
							// Выходим из цикла
							break;
					}
				} break;
				// Иначе завершаем работу
				default: const_cast <client::core_t *> (this->_core)->close(bid);
			}
		}
	}
}
/**
 * stop Метод остановки клиента
 */
void awh::client::Sample::stop() noexcept {
	// Запрещаем чтение данных из буфера
	this->_reading = false;
	// Выполняем очистку буфера данных
	this->_buffer.clear();
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working()){
		// Очищаем адрес сервера
		this->_scheme.url.clear();
		// Если завершить работу разрешено
		if(this->_complete && (this->_core != nullptr))
			// Завершаем работу, если разрешено остановить
			const_cast <client::core_t *> (this->_core)->stop();
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
			const_cast <client::core_t *> (this->_core)->close(this->_bid);
			// Восстанавливаем предыдущее значение флага
			this->_scheme.alive = alive;
		}
	}
}
/**
 * start Метод запуска клиента
 */
void awh::client::Sample::start() noexcept {
	// Если объект сетевого ядра установлен
	if(this->_core != nullptr){
		// Разрешаем чтение данных из буфера
		this->_reading = true;
		// Выполняем очистку буфера данных
		this->_buffer.clear();
		// Если биндинг не запущен
		if(!this->_core->working())
			// Выполняем запуск биндинга
			const_cast <client::core_t *> (this->_core)->start();
		// Если биндинг уже запущен, выполняем запрос на сервер
		else const_cast <client::core_t *> (this->_core)->open(this->_scheme.id);
	}
}
/**
 * close Метод закрытия подключения клиента
 */
void awh::client::Sample::close() noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working())
		// Завершаем работу, если разрешено остановить
		const_cast <client::core_t *> (this->_core)->close(this->_bid);
}
/**
 * init Метод инициализации Rest брокера
 * @param socket unix-сокет для биндинга
 */
void awh::client::Sample::init(const string & socket) noexcept {
	// Если unix-сокет передан
	if((this->_core != nullptr) && !socket.empty()){
		// Выполняем очистку схемы сети
		this->_scheme.clear();
		// Устанавливаем URL адрес запроса (как заглушка)
		this->_scheme.url = this->_uri.parse("http://unixsocket");
		/**
		 * Если операционной системой не является Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Выполняем установку unix-сокет
			const_cast <client::core_t *> (this->_core)->sockname(socket);
		#endif
	}
}
/**
 * init Метод инициализации Rest брокера
 * @param port порт сервера
 * @param host хост сервера
 */
void awh::client::Sample::init(const uint32_t port, const string & host) noexcept {
	// Если параметры подключения переданы
	if((port > 0) && !host.empty()){
		// Выполняем очистку схемы сети
		this->_scheme.clear();
		// Устанавливаем порт сервера
		this->_scheme.url.port = port;
		// Устанавливаем хост сервера
		this->_scheme.url.host = host;
		// Определяем тип передаваемого сервера
		switch(static_cast <uint8_t> (this->_net.host(host))){
			// Если хост является доменом или IPv4-адресом
			case static_cast <uint8_t> (net_t::type_t::IPV4):
				// Устанавливаем IP адрес
				this->_scheme.url.ip = host;
			break;
			// Если хост является IPv6-адресом, переводим IP-адрес в полную форму
			case static_cast <uint8_t> (net_t::type_t::IPV6): {
				// Создаём объкт для работы с адресами
				net_t net(this->_log);
				// Устанавливаем IP-адрес
				this->_scheme.url.ip = net = host;
			} break;
			// Если хост является доменной зоной
			case static_cast <uint8_t> (net_t::type_t::FQDN):
				// Устанавливаем доменное имя
				this->_scheme.url.domain = host;
			break;
		}
	}
}
/**
 * callbacks Метод установки функций обратного вызова
 * @param callbacks функции обратного вызова
 */
void awh::client::Sample::callbacks(const fn_t & callbacks) noexcept {
	// Выполняем установку функции обратного вызова при подключении/отключении
	this->_callbacks.set("active", callbacks);
	// Выполняем установку функции обратного вызова при получения событий запуска и остановки сетевого ядра
	this->_callbacks.set("status", callbacks);
	// Выполняем установку функции обратного вызова при получении сообщения
	this->_callbacks.set("message", callbacks);
}
/**
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::client::Sample::mode(const set <flag_t> & flags) noexcept {
	// Если объект сетевого ядра установлен
	if(this->_core != nullptr){
		// Устанавливаем флаг анбиндинга ядра сетевого модуля
		this->_complete = (flags.find(flag_t::NOT_STOP) == flags.end());
		// Устанавливаем флаг поддержания автоматического подключения
		this->_scheme.alive = (flags.find(flag_t::ALIVE) != flags.end());
		// Устанавливаем флаг запрещающий вывод информационных сообщений
		const_cast <client::core_t *> (this->_core)->verbose(flags.find(flag_t::NOT_INFO) == flags.end());
	}
}
/**
 * cork Метод отключения/включения алгоритма TCP/CORK
 * @param mode режим применимой операции
 * @return     результат выполенния операции
 */
bool awh::client::Sample::cork(const engine_t::mode_t mode) noexcept {
	// Если объект сетевого ядра установлен
	if(this->_core != nullptr)
		// Выполняем отключение/включение алгоритма TCP/CORK
		return const_cast <client::core_t *> (this->_core)->cork(this->_bid, mode);
	// Сообщаем, что ничего не установлено
	return false;
}
/**
 * nodelay Метод отключения/включения алгоритма Нейгла
 * @param mode режим применимой операции
 * @return     результат выполенния операции
 */
bool awh::client::Sample::nodelay(const engine_t::mode_t mode) noexcept {
	// Если объект сетевого ядра установлен
	if(this->_core != nullptr)
		// Выполняем отключение/включение алгоритма Нейгла
		return const_cast <client::core_t *> (this->_core)->nodelay(this->_bid, mode);
	// Сообщаем, что ничего не установлено
	return false;
}
/**
 * response Метод отправки сообщения брокеру
 * @param buffer буфер бинарных данных для отправки
 * @param size   размер бинарных данных для отправки
 */
void awh::client::Sample::send(const char * buffer, const size_t size) noexcept {
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::CONNECT, event_t::READ}, event_t::SEND)){
		// Если подключение выполнено
		if((this->_core != nullptr) && this->_core->working()){
			// Если включён режим отладки
			#if defined(DEBUG_MODE)
				// Выводим заголовок ответа
				std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
				// Выводим параметры ответа
				std::cout << string(buffer, size) << std::endl << std::flush;
			#endif
			// Отправляем тело на сервер
			const_cast <client::core_t *> (this->_core)->write(buffer, size, this->_bid);
		}
	}
}
/**
 * bandwidth Метод установки пропускной способности сети
 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
 */
void awh::client::Sample::bandwidth(const string & read, const string & write) noexcept {
	// Если объект сетевого ядра установлен
	if(this->_core != nullptr)
		// Выполняем установку пропускной способности сети
		const_cast <client::core_t *> (this->_core)->bandwidth(this->_bid, read, write);
}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::client::Sample::keepAlive(const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept {
	// Выполняем установку максимального количества попыток
	this->_scheme.keepAlive.cnt = cnt;
	// Выполняем установку интервала времени в секундах через которое происходит проверка подключения
	this->_scheme.keepAlive.idle = idle;
	// Выполняем установку интервала времени в секундах между попытками
	this->_scheme.keepAlive.intvl = intvl;
}
/**
 * waitMessage Метод ожидания входящих сообщений
 * @param sec интервал времени в секундах
 */
void awh::client::Sample::waitMessage(const time_t sec) noexcept {
	// Устанавливаем время ожидания получения данных
	this->_scheme.timeouts.wait = sec;
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read    количество секунд для детекции по чтению
 * @param write   количество секунд для детекции по записи
 * @param connect количество секунд для детекции по подключению
 */
void awh::client::Sample::waitTimeDetect(const time_t read, const time_t write, const time_t connect) noexcept {
	// Устанавливаем количество секунд на чтение
	this->_scheme.timeouts.read = read;
	// Устанавливаем количество секунд на запись
	this->_scheme.timeouts.write = write;
	// Устанавливаем количество секунд на подключение
	this->_scheme.timeouts.connect = connect;
}
/**
 * userAgentProxy Метод установки User-Agent для HTTP-запроса прокси-сервера
 * @param userAgent агент пользователя для HTTP-запроса
 */
void awh::client::Sample::userAgentProxy(const string & userAgent) noexcept {
	// Устанавливаем UserAgent
	if(!userAgent.empty())
		// Устанавливаем пользовательского агента для прокси-сервера
		this->_scheme.proxy.http.userAgent(userAgent);
}
/**
 * identProxy Метод установки идентификации клиента прокси-сервера
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::client::Sample::identProxy(const string & id, const string & name, const string & ver) noexcept {
	// Устанавливаем данные сервиса для прокси-сервера
	this->_scheme.proxy.http.ident(id, name, ver);
}
/**
 * proxy Метод активации/деактивации прокси-склиента
 * @param work флаг активации/деактивации прокси-клиента
 */
void awh::client::Sample::proxy(const client::scheme_t::work_t work) noexcept {
	// Если прокси-сервер установлен
	if(this->_scheme.proxy.type != client::proxy_t::type_t::NONE)
		// Выполняем активацию прокси-сервера
		this->_scheme.activateProxy(work);
	// Снимаем флаг активации прокси-клиента
	else this->_scheme.proxy.mode = false;
}
/**
 * proxy Метод установки прокси-сервера
 * @param uri    параметры прокси-сервера
 * @param family семейстово интернет протоколов (IPV4 / IPV6 / NIX)
 */
void awh::client::Sample::proxy(const string & uri, const scheme_t::family_t family) noexcept {
	// Если URI параметры переданы
	if((this->_proxy.mode = !uri.empty())){
		// Устанавливаем семейство интернет протоколов
		this->_scheme.proxy.family = family;
		// Устанавливаем параметры прокси-сервера
		this->_scheme.proxy.url = this->_uri.parse(uri);
		// Если данные параметров прокси-сервера получены
		if(!this->_scheme.proxy.url.empty()){
			// Если протокол подключения SOCKS5
			if(this->_fmk->compare(this->_scheme.proxy.url.schema, "socks5")){
				// Устанавливаем тип прокси-сервера
				this->_scheme.proxy.type = client::proxy_t::type_t::SOCKS5;
				// Если требуется авторизация на прокси-сервере
				if(!this->_scheme.proxy.url.user.empty() && !this->_scheme.proxy.url.pass.empty())
					// Устанавливаем данные пользователя
					this->_scheme.proxy.socks5.user(this->_scheme.proxy.url.user, this->_scheme.proxy.url.pass);
			// Если протокол подключения HTTP/1.1
			} else if(this->_fmk->compare(this->_scheme.proxy.url.schema, "http")) {
				// Устанавливаем тип прокси-сервера
				this->_scheme.proxy.type = client::proxy_t::type_t::HTTP;
				// Если требуется авторизация на прокси-сервере
				if(!this->_scheme.proxy.url.user.empty() && !this->_scheme.proxy.url.pass.empty())
					// Устанавливаем данные пользователя
					this->_scheme.proxy.http.user(this->_scheme.proxy.url.user, this->_scheme.proxy.url.pass);
			}
		}
	}
}
/**
 * authTypeProxy Метод установки типа авторизации прокси-сервера
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest-авторизации
 */
void awh::client::Sample::authTypeProxy(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Если объект авторизации создан
	this->_scheme.proxy.http.authType(type, hash);
}
/**
 * Sample Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::client::Sample::Sample(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 _bid(0), _reading(false), _complete(true), _attempt(0), _attempts(15), _net(log),
 _uri(fmk, log), _callbacks(log), _scheme(fmk, log), _buffer(log), _fmk(fmk), _log(log), _core(core) {
	// Если объект сетевого ядра установлен
	if(this->_core != nullptr){
		// Устанавливаем функцию обработки вызова для получения чанков для HTTP-клиента
		this->_scheme.proxy.http.callback <void (const uint64_t, const vector <char> &, const awh::http_t *)> ("chunking", std::bind(&sample_t::chunking, this, _1, _2, _3));
		// Добавляем схему сети в сетевое ядро
		const_cast <client::core_t *> (this->_core)->scheme(&this->_scheme);
		// Устанавливаем событие на запуск системы
		const_cast <client::core_t *> (this->_core)->callback <void (const uint16_t)> ("open", std::bind(&sample_t::openEvent, this, _1));
		// Выполняем установку функций обратного вызова для клиента
		const_cast <client::core_t *> (this->_core)->callback <void (const awh::core_t::status_t)> ("status", std::bind(&sample_t::statusEvent, this, _1));
		// Устанавливаем событие подключения
		const_cast <client::core_t *> (this->_core)->callback <void (const uint64_t, const uint16_t)> ("connect", std::bind(&sample_t::connectEvent, this, _1, _2));
		// Устанавливаем событие отключения
		const_cast <client::core_t *> (this->_core)->callback <void (const uint64_t, const uint16_t)> ("disconnect", std::bind(&sample_t::disconnectEvent, this, _1, _2));
		// Устанавливаем событие на подключение к прокси-серверу
		const_cast <client::core_t *> (this->_core)->callback <void (const uint64_t, const uint16_t)> ("connectProxy", std::bind(&sample_t::proxyConnectEvent, this, _1, _2));
		// Устанавливаем событие на активацию шифрованного SSL канала
		const_cast <client::core_t *> (this->_core)->callback <bool (const uri_t::url_t &, const uint64_t, const uint16_t)> ("ssl", std::bind(&sample_t::enableSSLEvent, this, _1, _2, _3));
		// Устанавливаем функцию чтения данных
		const_cast <client::core_t *> (this->_core)->callback <void (const char *, const size_t, const uint64_t, const uint16_t)> ("read", std::bind(&sample_t::readEvent, this, _1, _2, _3, _4));
		// Устанавливаем событие на чтение данных с прокси-сервера
		const_cast <client::core_t *> (this->_core)->callback <void (const char *, const size_t, const uint64_t, const uint16_t)> ("readProxy", std::bind(&sample_t::proxyReadEvent, this, _1, _2, _3, _4));
	}
}
