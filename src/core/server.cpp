/**
 * @file: server.cpp
 * @date: 2024-03-10
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2024
 */

// Подключаем заголовочный файл
#include <core/server.hpp>

/**
 * accept Метод вызова при подключении к серверу
 * @param fd  файловый дескриптор (сокет) подключившегося клиента
 * @param sid идентификатор схемы сети
 */
void awh::server::Core::accept(const SOCKET fd, const uint16_t sid) noexcept {
	// Если идентификатор схемы сети передан
	if((sid > 0) && (fd != INVALID_SOCKET) && (fd < MAX_SOCKETS)){
		// Выполняем поиск идентификатора схемы сети
		auto i = this->_schemes.find(sid);
		// Если идентификатор схемы сети найден, устанавливаем максимальное количество одновременных подключений
		if(i != this->_schemes.end()){
			// Получаем объект подключения
			scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
			// Определяем тип сокета
			switch(static_cast <uint8_t> (this->_settings.sonet)){
				// Если тип сокета установлен как UDP
				case static_cast <uint8_t> (scheme_t::sonet_t::UDP): {
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Если процесс является дочерним
						if(this->_pid != ::getpid()){
							// Выводим в консоль информацию
							this->_log->print("Working in child processes for \"UDP-protocol\" is not supported PID=%d", log_t::flag_t::WARNING, ::getpid());
							// Если функция обратного вызова установлена
							if(this->_callbacks.is("error"))
								// Выполняем функцию обратного вызова
								this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::ACCEPT, this->_fmk->format("Working in child processes for \"UDP-protocol\" is not supported PID=%d", ::getpid()));
							// Выходим
							break;
						// Выполняем остановку работы получения запроса на подключение
						} else {
							// Создаём бъект активного брокера подключения
							unique_ptr <awh::scheme_t::broker_t> broker(new awh::scheme_t::broker_t(sid, this->_fmk, this->_log));
							// Устанавливаем таймаут начтение данных из сокета
							broker->timeout(shm->timeouts.read, engine_t::method_t::READ);
							// Устанавливаем таймаут на запись данных в сокет
							broker->timeout(shm->timeouts.write, engine_t::method_t::WRITE);
							// Устанавливаем таймаут на подключение к серверу
							broker->timeout(shm->timeouts.connect, engine_t::method_t::CONNECT);
							// Определяем тип протокола подключения
							switch(static_cast <uint8_t> (this->_settings.family)){
								// Если тип протокола подключения IPv4
								case static_cast <uint8_t> (scheme_t::family_t::IPV4): {
									// Выполняем перебор всего списка адресов
									for(auto & host : this->_settings.network){
										// Если хост соответствует адресу IPv4
										if(this->_net.host(host) == net_t::type_t::IPV4)
											// Выполняем установку полученного хоста
											broker->_addr.network.push_back(host);
									}
								} break;
								// Если тип протокола подключения IPv6
								case static_cast <uint8_t> (scheme_t::family_t::IPV6): {
									// Выполняем перебор всего списка адресов
									for(auto & host : this->_settings.network){
										// Если хост соответствует адресу IPv4
										if(this->_net.host(host) == net_t::type_t::IPV6)
											// Выполняем установку полученного хоста
											broker->_addr.network.push_back(host);
									}
								} break;
							}
							// Устанавливаем параметры сокета
							broker->_addr.sonet(SOCK_DGRAM, IPPROTO_UDP);
							// Если unix-сокет используется
							if(this->_settings.family == scheme_t::family_t::NIX)
								// Выполняем инициализацию сокета
								broker->_addr.init(this->_settings.sockname, engine_t::type_t::SERVER);
							// Если unix-сокет не используется, выполняем инициализацию сокета
							else broker->_addr.init(shm->_host, shm->_port, (this->_settings.family == scheme_t::family_t::IPV6 ? AF_INET6 : AF_INET), engine_t::type_t::SERVER, this->_settings.ipV6only);
							// Выполняем разрешение подключения
							if(broker->_addr.accept(broker->_addr.fd, 0)){
								// Получаем адрес подключения клиента
								broker->ip(broker->_addr.ip);
								// Получаем аппаратный адрес клиента
								broker->mac(broker->_addr.mac);
								// Получаем порт подключения клиента
								broker->port(broker->_addr.port);
								// Выполняем установку желаемого протокола подключения
								broker->_ectx.proto(this->_settings.proto);
								// Выполняем получение контекста сертификата
								this->_engine.wrap(broker->_ectx, &broker->_addr);
								// Если подключение не обёрнуто
								if((broker->_addr.fd == INVALID_SOCKET) || (broker->_addr.fd >= MAX_SOCKETS)){
									// Выводим сообщение об ошибке
									this->_log->print("Wrap engine context is failed", log_t::flag_t::CRITICAL);
									// Если функция обратного вызова установлена
									if(this->_callbacks.is("error"))
										// Выполняем функцию обратного вызова
										this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::ACCEPT, "Wrap engine context is failed");
									// Выходим из функции
									return;
								}
								// Выполняем блокировку потока
								this->_mtx.accept.lock();
								// Выполняем установку базы событий
								broker->base(this->_dispatch.base);
								// Добавляем созданного брокера в список брокеров
								auto ret = shm->_brokers.emplace(broker->id(), std::forward <unique_ptr <awh::scheme_t::broker_t>> (broker));
								// Добавляем брокера в список подключений
								node_t::_brokers.emplace(ret.first->first, sid);
								// Выполняем блокировку потока
								this->_mtx.accept.unlock();
								// Переводим сокет в неблокирующий режим
								ret.first->second->_ectx.noblock();
								// Выполняем установку функции обратного вызова на получении сообщений
								ret.first->second->callback <void (const uint64_t)> ("read", std::bind(&core_t::read, this, _1));
								// Активируем получение данных с клиента
								ret.first->second->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::READ);
								// Если функция обратного вызова установлена
								if(this->_callbacks.is("connect"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const uint64_t, const uint16_t)> ("connect", ret.first->first, sid);
							// Подключение не установлено
							} else {
								// Выводим сообщение об ошибке
								this->_log->print("Accepting failed, PID=%d", log_t::flag_t::WARNING, ::getpid());
								// Если функция обратного вызова установлена
								if(this->_callbacks.is("error"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::ACCEPT, this->_fmk->format("Accepting failed, PID=%d", ::getpid()));
							}
						}
					/**
					 * Если возникает ошибка
					 */
					} catch(const bad_alloc &) {
						// Выходим из приложения
						::exit(EXIT_FAILURE);
					}
				} break;
				// Если тип сокета установлен как TCP/IP
				case static_cast <uint8_t> (scheme_t::sonet_t::TCP):
				// Если тип сокета установлен как TCP/IP TLS
				case static_cast <uint8_t> (scheme_t::sonet_t::TLS):
				// Если тип сокета установлен как SCTP DTLS
				case static_cast <uint8_t> (scheme_t::sonet_t::SCTP): {
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Если количество подключившихся клиентов, больше максимально-допустимого количества клиентов
						if(shm->_brokers.size() >= static_cast <size_t> (shm->_total)){
							// Выводим в консоль информацию
							this->_log->print("Number of simultaneous connections, cannot exceed maximum allowed number of %d", log_t::flag_t::WARNING, shm->_total);
							// Если функция обратного вызова установлена
							if(this->_callbacks.is("error"))
								// Выполняем функцию обратного вызова
								this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::ACCEPT, this->_fmk->format("Number of simultaneous connections, cannot exceed maximum allowed number of %d", shm->_total));
							// Выходим
							break;
						}
						// Создаём бъект активного брокера подключения
						unique_ptr <awh::scheme_t::broker_t> broker(new awh::scheme_t::broker_t(sid, this->_fmk, this->_log));
						// Устанавливаем время жизни подключения
						broker->_addr.alive = shm->keepAlive;
						// Устанавливаем таймаут начтение данных из сокета
						broker->timeout(shm->timeouts.read, engine_t::method_t::READ);
						// Устанавливаем таймаут на запись данных в сокет
						broker->timeout(shm->timeouts.write, engine_t::method_t::WRITE);
						// Устанавливаем таймаут на подключение к серверу
						broker->timeout(shm->timeouts.connect, engine_t::method_t::CONNECT);
						// Определяем тип сокета
						switch(static_cast <uint8_t> (this->_settings.sonet)){
							/**
							 * Если операционной системой является Linux или FreeBSD
							 */
							#if defined(__linux__) || defined(__FreeBSD__)
								// Если тип сокета установлен как SCTP
								case static_cast <uint8_t> (scheme_t::sonet_t::SCTP):
									// Устанавливаем параметры сокета
									broker->_addr.sonet(SOCK_STREAM, IPPROTO_SCTP);
								break;
							#endif
							// Для всех остальных типов сокетов
							default:
								// Устанавливаем параметры сокета
								broker->_addr.sonet(SOCK_STREAM, IPPROTO_TCP);
						}
						// Выполняем разрешение подключения
						if(broker->_addr.accept(shm->_addr)){
							// Если MAC или IP-адрес не получен, тогда выходим
							if(broker->_addr.mac.empty() || broker->_addr.ip.empty()){
								// Выполняем очистку контекста двигателя
								broker->_ectx.clear();
								// Если подключение не установлено, выводим сообщение об ошибке
								this->_log->print("Client address not received, PID=%d", log_t::flag_t::WARNING, ::getpid());
								// Если функция обратного вызова установлена
								if(this->_callbacks.is("error"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::ACCEPT, this->_fmk->format("Client address not received, PID=%d", ::getpid()));
							// Если все данные получены
							} else {
								// Получаем адрес подключения клиента
								broker->ip(broker->_addr.ip);
								// Получаем аппаратный адрес клиента
								broker->mac(broker->_addr.mac);
								// Получаем порт подключения клиента
								broker->port(broker->_addr.port);
								// Если функция обратного вызова проверки подключения установлена, выполняем проверку, если проверка не пройдена?
								if((this->_callbacks.is("accept")) && !this->_callbacks.call <bool (const string &, const string &, const u_int, const uint16_t)> ("accept", broker->ip(), broker->mac(), broker->port(), sid)){
									// Если порт установлен
									if(broker->port() > 0){
										// Определяем тип протокола подключения
										switch(static_cast <uint8_t> (this->_settings.family)){
											// Если тип протокола подключения unix-сокет
											case static_cast <uint8_t> (scheme_t::family_t::NIX): {
												// Выводим сообщение об ошибке
												this->_log->print(
													"Access to server [%s] PID=%d is denied for client [%s:%d] MAC=%s, SOCKET=%d",
													log_t::flag_t::WARNING,
													this->host(sid).c_str(),
													::getpid(),
													broker->ip().c_str(),
													broker->port(),
													broker->mac().c_str(),
													broker->_addr.fd
												);
												// Если функция обратного вызова установлена
												if(this->_callbacks.is("error"))
													// Выполняем функцию обратного вызова
													this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> (
														"error",
														log_t::flag_t::WARNING,
														error_t::ACCEPT,
														this->_fmk->format(
															"Access to server [%s] PID=%d is denied for client [%s:%d] MAC=%s, SOCKET=%d",
															this->host(sid).c_str(),
															::getpid(),
															broker->ip().c_str(),
															broker->port(),
															broker->mac().c_str(),
															broker->_addr.fd
														)
													);
											} break;
											// Если тип протокола подключения IPv4
											case static_cast <uint8_t> (scheme_t::family_t::IPV4):
											// Если тип протокола подключения IPv6
											case static_cast <uint8_t> (scheme_t::family_t::IPV6): {
												// Выводим сообщение об ошибке
												this->_log->print(
													"Access to server [%s:%d] PID=%d is denied for client [%s:%d] MAC=%s, SOCKET=%d",
													log_t::flag_t::WARNING,
													this->host(sid).c_str(),
													this->port(sid),
													::getpid(),
													broker->ip().c_str(),
													broker->port(),
													broker->mac().c_str(),
													broker->_addr.fd
												);
												// Если функция обратного вызова установлена
												if(this->_callbacks.is("error"))
													// Выполняем функцию обратного вызова
													this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> (
														"error",
														log_t::flag_t::WARNING,
														error_t::ACCEPT,
														this->_fmk->format(
															"Access to server [%s:%d] PID=%d is denied for client [%s:%d] MAC=%s, SOCKET=%d",
															this->host(sid).c_str(),
															this->port(sid),
															::getpid(),
															broker->ip().c_str(),
															broker->port(),
															broker->mac().c_str(),
															broker->_addr.fd
														)
													);
											} break;
										}
									// Если порт не установлен
									} else {
										// Определяем тип протокола подключения
										switch(static_cast <uint8_t> (this->_settings.family)){
											// Если тип протокола подключения unix-сокет
											case static_cast <uint8_t> (scheme_t::family_t::NIX): {
												// Выводим сообщение об ошибке
												this->_log->print(
													"Access to server [%s] PID=%d is denied for client [%s] MAC=%s, SOCKET=%d",
													log_t::flag_t::WARNING,
													this->host(sid).c_str(),
													::getpid(),
													broker->ip().c_str(),
													broker->mac().c_str(),
													broker->_addr.fd
												);
												// Если функция обратного вызова установлена
												if(this->_callbacks.is("error"))
													// Выполняем функцию обратного вызова
													this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> (
														"error",
														log_t::flag_t::WARNING,
														error_t::ACCEPT,
														this->_fmk->format(
															"Access to server [%s] PID=%d is denied for client [%s] MAC=%s, SOCKET=%d",
															this->host(sid).c_str(),
															::getpid(),
															broker->ip().c_str(),
															broker->mac().c_str(),
															broker->_addr.fd
														)
													);
											} break;
											// Если тип протокола подключения IPv4
											case static_cast <uint8_t> (scheme_t::family_t::IPV4):
											// Если тип протокола подключения IPv6
											case static_cast <uint8_t> (scheme_t::family_t::IPV6): {
												// Выводим сообщение об ошибке
												this->_log->print(
													"Access to server [%s:%d] PID=%d is denied for client [%s] MAC=%s, SOCKET=%d",
													log_t::flag_t::WARNING,
													this->host(sid).c_str(),
													this->port(sid),
													::getpid(),
													broker->ip().c_str(),
													broker->mac().c_str(),
													broker->_addr.fd
												);
												// Если функция обратного вызова установлена
												if(this->_callbacks.is("error"))
													// Выполняем функцию обратного вызова
													this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> (
														"error",
														log_t::flag_t::WARNING,
														error_t::ACCEPT,
														this->_fmk->format(
															"Access to server [%s:%d] PID=%d is denied for client [%s] MAC=%s, SOCKET=%d",
															this->host(sid).c_str(),
															this->port(sid),
															::getpid(),
															broker->ip().c_str(),
															broker->mac().c_str(),
															broker->_addr.fd
														)
													);
											} break;
										}
									}
									// Выполняем очистку контекста двигателя
									broker->_ectx.clear();
									// Выходим
									break;
								}
								// Выполняем установку желаемого протокола подключения
								broker->_ectx.proto(this->_settings.proto);
								// Выполняем получение контекста сертификата
								this->_engine.wrap(broker->_ectx, &broker->_addr);
								// Если мы хотим работать в зашифрованном режиме
								if(this->_settings.sonet == scheme_t::sonet_t::TLS){
									// Если сертификаты не приняты, выходим
									if(!this->_engine.encrypted(broker->_ectx)){
										// Выполняем очистку контекста двигателя
										broker->_ectx.clear();
										// Выводим сообщение об ошибке
										this->_log->print("Encryption mode cannot be activated", log_t::flag_t::CRITICAL);
										// Если функция обратного вызова установлена
										if(this->_callbacks.is("error"))
											// Выполняем функцию обратного вызова
											this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::ACCEPT, "Encryption mode cannot be activated");
										// Выходим
										break;
									}
								}
								// Если подключение не обёрнуто
								if((broker->_addr.fd == INVALID_SOCKET) || (broker->_addr.fd >= MAX_SOCKETS)){
									// Выводим сообщение об ошибке
									this->_log->print("Wrap engine context is failed", log_t::flag_t::CRITICAL);
									// Если функция обратного вызова установлена
									if(this->_callbacks.is("error"))
										// Выполняем функцию обратного вызова
										this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::ACCEPT, "Wrap engine context is failed");
									// Выходим
									break;
								}
								// Выполняем блокировку потока
								this->_mtx.accept.lock();
								// Выполняем установку базы событий
								broker->base(this->_dispatch.base);
								// Добавляем созданного брокера в список брокеров
								auto ret = shm->_brokers.emplace(broker->id(), std::forward <unique_ptr <awh::scheme_t::broker_t>> (broker));
								// Добавляем брокера в список подключений
								node_t::_brokers.emplace(ret.first->first, sid);
								// Выполняем блокировку потока
								this->_mtx.accept.unlock();
								// Переводим сокет в неблокирующий режим
								ret.first->second->_ectx.noblock();
								// Если вывод информационных данных не запрещён
								if(this->_verb){
									// Если порт установлен
									if(ret.first->second->port() > 0){
										// Определяем тип протокола подключения
										switch(static_cast <uint8_t> (this->_settings.family)){
											// Если тип протокола подключения unix-сокет
											case static_cast <uint8_t> (scheme_t::family_t::NIX): {
												// Выводим в консоль информацию
												this->_log->print(
													"Connected client [%s:%d] MAC=%s, SOCKET=%d to server [%s] PID=%d",
													log_t::flag_t::INFO,
													ret.first->second->ip().c_str(),
													ret.first->second->port(),
													ret.first->second->mac().c_str(),
													ret.first->second->_addr.fd,
													this->host(sid).c_str(),
													::getpid()
												);
											} break;
											// Если тип протокола подключения IPv4
											case static_cast <uint8_t> (scheme_t::family_t::IPV4):
											// Если тип протокола подключения IPv6
											case static_cast <uint8_t> (scheme_t::family_t::IPV6): {
												// Выводим в консоль информацию
												this->_log->print(
													"Connected client [%s:%d] MAC=%s, SOCKET=%d to server [%s:%d] PID=%d",
													log_t::flag_t::INFO,
													ret.first->second->ip().c_str(),
													ret.first->second->port(),
													ret.first->second->mac().c_str(),
													ret.first->second->_addr.fd,
													this->host(sid).c_str(),
													this->port(sid),
													::getpid()
												);
											} break;
										}
									// Если порт не установлен
									} else {
										// Определяем тип протокола подключения
										switch(static_cast <uint8_t> (this->_settings.family)){
											// Если тип протокола подключения unix-сокет
											case static_cast <uint8_t> (scheme_t::family_t::NIX): {
												// Выводим в консоль информацию
												this->_log->print(
													"Connected client [%s] MAC=%s, SOCKET=%d to server [%s] PID=%d",
													log_t::flag_t::INFO,
													ret.first->second->ip().c_str(),
													ret.first->second->mac().c_str(),
													ret.first->second->_addr.fd,
													this->host(sid).c_str(),
													::getpid()
												);
											} break;
											// Если тип протокола подключения IPv4
											case static_cast <uint8_t> (scheme_t::family_t::IPV4):
											// Если тип протокола подключения IPv6
											case static_cast <uint8_t> (scheme_t::family_t::IPV6): {
												// Выводим в консоль информацию
												this->_log->print(
													"Connected client [%s] MAC=%s, SOCKET=%d to server [%s:%d] PID=%d",
													log_t::flag_t::INFO,
													ret.first->second->ip().c_str(),
													ret.first->second->mac().c_str(),
													ret.first->second->_addr.fd,
													this->host(sid).c_str(),
													this->port(sid),
													::getpid()
												);
											} break;
										}
									}
								}
								// Выполняем установку функции обратного вызова на получении сообщений
								ret.first->second->callback <void (const uint64_t)> ("read", std::bind(&core_t::read, this, _1));
								// Активируем получение данных с клиента
								ret.first->second->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::READ);
								// Если функция обратного вызова установлена
								if(this->_callbacks.is("connect"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const uint64_t, const uint16_t)> ("connect", ret.first->first, sid);
							}
						// Если подключение не установлено
						} else {
							// Выводим сообщение об ошибке
							this->_log->print("Accepting failed, PID=%d", log_t::flag_t::WARNING, ::getpid());
							// Если функция обратного вызова установлена
							if(this->_callbacks.is("error"))
								// Выполняем функцию обратного вызова
								this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::ACCEPT, this->_fmk->format("Accepting failed, PID=%d", ::getpid()));
						}
					/**
					 * Если возникает ошибка
					 */
					} catch(const bad_alloc &) {
						// Выходим из приложения
						::exit(EXIT_FAILURE);
					}
				} break;
			}
		}
	}
}
/**
 * accept Метод вызова при активации DTLS-подключения
 * @param sid идентификатор схемы сети
 * @param bid идентификатор брокера
 */
void awh::server::Core::accept(const uint16_t sid, const uint64_t bid) noexcept {
	// Если идентификатор схемы сети и брокер подключений существуют
	if(this->has(sid) && this->has(bid)){
		// Выполняем поиск идентификатора схемы сети
		auto i = this->_schemes.find(sid);
		// Если идентификатор схемы сети найден, устанавливаем максимальное количество одновременных подключений
		if(i != this->_schemes.end()){
			// Получаем объект подключения
			scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
			// Выполняем поиск брокера подключений
			auto j = shm->_brokers.find(bid);
			// Если брокер подключения получен
			if(j != shm->_brokers.end()){
				/**
				 * Методы только для OS Windows
				 */
				#if defined(_WIN32) || defined(_WIN64)
					// Идентификатор ошибки подключения
					const bool error = ((AWH_ERROR() > 0) && (AWH_ERROR() != WSAEWOULDBLOCK) && (AWH_ERROR() != WSAEINVAL));
				/**
				 * Для всех остальных операционных систем
				 */
				#else
					// Если нужно попытаться ещё раз отправить сообщение
					const bool error = ((AWH_ERROR() > 0) && (AWH_ERROR() != EWOULDBLOCK) && (AWH_ERROR() != EINVAL));
				#endif
				// Получаем бъект активного брокера подключения
				awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (j->second.get());
				// Останавливаем событие подключения клиента
				broker->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::CONNECT);
				// Выполняем ожидание входящих подключений
				if(this->_engine.wait(broker->_ectx)){
					// Устанавливаем параметры сокета
					broker->_addr.sonet(SOCK_DGRAM, IPPROTO_UDP);
					// Если прикрепление клиента к серверу выполнено
					if(broker->_addr.attach(shm->_addr)){
						// Выполняем прикрепление контекста клиента к контексту сервера
						this->_engine.attach(broker->_ectx, &broker->_addr);
						// Если MAC или IP-адрес не получен, тогда выходим
						if(broker->_addr.mac.empty() || broker->_addr.ip.empty()){
							// Выполняем очистку контекста двигателя
							broker->_ectx.clear();
							// Если подключение не установлено, выводим сообщение об ошибке
							this->_log->print("Client address not received, PID=%d", log_t::flag_t::WARNING, ::getpid());
							// Если функция обратного вызова установлена
							if(this->_callbacks.is("error"))
								// Выполняем функцию обратного вызова
								this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::ACCEPT, this->_fmk->format("Client address not received, PID=%d", ::getpid()));
						// Если все данные получены
						} else {
							// Получаем адрес подключения клиента
							broker->ip(broker->_addr.ip);
							// Получаем аппаратный адрес клиента
							broker->mac(broker->_addr.mac);
							// Получаем порт подключения клиента
							broker->port(broker->_addr.port);
							// Если функция обратного вызова проверки подключения установлена, выполняем проверку, если проверка не пройдена?
							if((this->_callbacks.is("accept")) && !this->_callbacks.call <bool (const string &, const string &, const u_int, const uint16_t)> ("accept", broker->ip(), broker->mac(), broker->port(), sid)){
								// Если порт установлен
								if(broker->port() > 0){
									// Определяем тип протокола подключения
									switch(static_cast <uint8_t> (this->_settings.family)){
										// Если тип протокола подключения unix-сокет
										case static_cast <uint8_t> (scheme_t::family_t::NIX): {
											// Выводим сообщение об ошибке
											this->_log->print(
												"Access to server [%s] PID=%d is denied for client [%s:%d] MAC=%s, SOCKET=%d",
												log_t::flag_t::WARNING,
												this->host(sid).c_str(),
												::getpid(),
												broker->ip().c_str(),
												broker->port(),
												broker->mac().c_str(),
												broker->_addr.fd
											);
											// Если функция обратного вызова установлена
											if(this->_callbacks.is("error"))
												// Выполняем функцию обратного вызова
												this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> (
													"error",
													log_t::flag_t::WARNING,
													error_t::ACCEPT,
													this->_fmk->format(
														"Access to server [%s] PID=%d is denied for client [%s:%d] MAC=%s, SOCKET=%d",
														this->host(sid).c_str(),
														::getpid(),
														broker->ip().c_str(),
														broker->port(),
														broker->mac().c_str(),
														broker->_addr.fd
													)
												);
										} break;
										// Если тип протокола подключения IPv4
										case static_cast <uint8_t> (scheme_t::family_t::IPV4):
										// Если тип протокола подключения IPv6
										case static_cast <uint8_t> (scheme_t::family_t::IPV6): {
											// Выводим сообщение об ошибке
											this->_log->print(
												"Access to server [%s:%d] PID=%d is denied for client [%s:%d] MAC=%s, SOCKET=%d",
												log_t::flag_t::WARNING,
												this->host(sid).c_str(),
												this->port(sid),
												::getpid(),
												broker->ip().c_str(),
												broker->port(),
												broker->mac().c_str(),
												broker->_addr.fd
											);
											// Если функция обратного вызова установлена
											if(this->_callbacks.is("error"))
												// Выполняем функцию обратного вызова
												this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> (
													"error",
													log_t::flag_t::WARNING,
													error_t::ACCEPT,
													this->_fmk->format(
														"Access to server [%s:%d] PID=%d is denied for client [%s:%d] MAC=%s, SOCKET=%d",
														this->host(sid).c_str(),
														this->port(sid),
														::getpid(),
														broker->ip().c_str(),
														broker->port(),
														broker->mac().c_str(),
														broker->_addr.fd
													)
												);
										} break;
									}
								// Если порт не установлен
								} else {
									// Определяем тип протокола подключения
									switch(static_cast <uint8_t> (this->_settings.family)){
										// Если тип протокола подключения unix-сокет
										case static_cast <uint8_t> (scheme_t::family_t::NIX): {
											// Выводим сообщение об ошибке
											this->_log->print(
												"Access to server [%s] PID=%d is denied for client [%s] MAC=%s, SOCKET=%d",
												log_t::flag_t::WARNING,
												this->host(sid).c_str(),
												::getpid(),
												broker->ip().c_str(),
												broker->mac().c_str(),
												broker->_addr.fd
											);
											// Если функция обратного вызова установлена
											if(this->_callbacks.is("error"))
												// Выполняем функцию обратного вызова
												this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> (
													"error",
													log_t::flag_t::WARNING,
													error_t::ACCEPT,
													this->_fmk->format(
														"Access to server [%s] PID=%d is denied for client [%s] MAC=%s, SOCKET=%d",
														this->host(sid).c_str(),
														::getpid(),
														broker->ip().c_str(),
														broker->mac().c_str(),
														broker->_addr.fd
													)
												);
										} break;
										// Если тип протокола подключения IPv4
										case static_cast <uint8_t> (scheme_t::family_t::IPV4):
										// Если тип протокола подключения IPv6
										case static_cast <uint8_t> (scheme_t::family_t::IPV6): {
											// Выводим сообщение об ошибке
											this->_log->print(
												"Access to server [%s:%d] PID=%d is denied for client [%s] MAC=%s, SOCKET=%d",
												log_t::flag_t::WARNING,
												this->host(sid).c_str(),
												this->port(sid),
												::getpid(),
												broker->ip().c_str(),
												broker->mac().c_str(),
												broker->_addr.fd
											);
											// Если функция обратного вызова установлена
											if(this->_callbacks.is("error"))
												// Выполняем функцию обратного вызова
												this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> (
													"error",
													log_t::flag_t::WARNING,
													error_t::ACCEPT,
													this->_fmk->format(
														"Access to server [%s:%d] PID=%d is denied for client [%s] MAC=%s, SOCKET=%d",
														this->host(sid).c_str(),
														this->port(sid),
														::getpid(),
														broker->ip().c_str(),
														broker->mac().c_str(),
														broker->_addr.fd
													)
												);
										} break;
									}
								}
								// Выполняем отключение брокера
								this->close(bid);
								// Выходим
								return;
							}
							// Переводим сокет в блокирующий режим
							broker->_ectx.block();
							// Если вывод информационных данных не запрещён
							if(this->_verb){
								// Если порт установлен
								if(broker->port() > 0){
									// Определяем тип протокола подключения
									switch(static_cast <uint8_t> (this->_settings.family)){
										// Если тип протокола подключения unix-сокет
										case static_cast <uint8_t> (scheme_t::family_t::NIX): {
											// Выводим в консоль информацию
											this->_log->print(
												"Connected client [%s:%d] MAC=%s, SOCKET=%d to server [%s] PID=%d",
												log_t::flag_t::INFO,
												broker->ip().c_str(),
												broker->port(),
												broker->mac().c_str(),
												broker->_addr.fd,
												this->host(sid).c_str(),
												::getpid()
											);
										} break;
										// Если тип протокола подключения IPv4
										case static_cast <uint8_t> (scheme_t::family_t::IPV4):
										// Если тип протокола подключения IPv6
										case static_cast <uint8_t> (scheme_t::family_t::IPV6): {
											// Выводим в консоль информацию
											this->_log->print(
												"Connected client [%s:%d] MAC=%s, SOCKET=%d to server [%s:%d] PID=%d",
												log_t::flag_t::INFO,
												broker->ip().c_str(),
												broker->port(),
												broker->mac().c_str(),
												broker->_addr.fd,
												this->host(sid).c_str(),
												this->port(sid),
												::getpid()
											);
										} break;
									}
								// Если порт не установлен
								} else {
									// Определяем тип протокола подключения
									switch(static_cast <uint8_t> (this->_settings.family)){
										// Если тип протокола подключения unix-сокет
										case static_cast <uint8_t> (scheme_t::family_t::NIX): {
											// Выводим в консоль информацию
											this->_log->print(
												"Connected client [%s], MAC=%s, SOCKET=%d to server [%s] PID=%d",
												log_t::flag_t::INFO,
												broker->ip().c_str(),
												broker->mac().c_str(),
												broker->_addr.fd,
												this->host(sid).c_str(),
												::getpid()
											);
										} break;
										// Если тип протокола подключения IPv4
										case static_cast <uint8_t> (scheme_t::family_t::IPV4):
										// Если тип протокола подключения IPv6
										case static_cast <uint8_t> (scheme_t::family_t::IPV6): {
											// Выводим в консоль информацию
											this->_log->print(
												"Connected client [%s], MAC=%s, SOCKET=%d to server [%s:%d] PID=%d",
												log_t::flag_t::INFO,
												broker->ip().c_str(),
												broker->mac().c_str(),
												broker->_addr.fd,
												this->host(sid).c_str(),
												this->port(sid),
												::getpid()
											);
										} break;
									}
								}
							}
							// Выполняем установку функции обратного вызова на получении сообщений
							broker->callback <void (const uint64_t)> ("read", std::bind(&core_t::read, this, _1));
							// Активируем получение данных с клиента
							broker->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::READ);
							// Если функция обратного вызова установлена
							if(this->_callbacks.is("connect"))
								// Выполняем функцию обратного вызова
								this->_callbacks.call <void (const uint64_t, const uint16_t)> ("connect", bid, sid);
							// Выполняем поиск таймера
							auto i = this->_timers.find(sid);
							// Если таймер найден
							if(i != this->_timers.end()){
								// Выполняем создание нового таймаута на 10 миллисекунд
								const uint16_t tid = i->second->timeout(10);
								// Выполняем добавление функции обратного вызова
								i->second->set <void (const uint64_t)> (tid, std::bind(static_cast <void (core_t::*)(const uint64_t)> (&core_t::read), this, bid));
							}
						}
					// Подключение не установлено
					} else {
						// Выводим сообщение об ошибке
						this->_log->print("Accepting failed, PID=%d", log_t::flag_t::WARNING, ::getpid());
						// Если функция обратного вызова установлена
						if(this->_callbacks.is("error"))
							// Выполняем функцию обратного вызова
							this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::ACCEPT, this->_fmk->format("Accepting failed, PID=%d", ::getpid()));
					}
				// Если обнаружена ошибка сокета
				} else if(error) {
					// Получаем текст полученной ошибки
					const string & message = this->_socket.message(AWH_ERROR());
					// Выводим сообщение об ошибке
					this->_log->print("%s, PID=%d", log_t::flag_t::CRITICAL, message.c_str(), ::getpid());
					// Если функция обратного вызова установлена
					if(this->_callbacks.is("error"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::ACCEPT, this->_fmk->format("%s, PID=%d", message.c_str(), ::getpid()));
					// Выполняем удаление объекта подключения
					this->close(bid);
				// Запускаем таймер вновь на 100мс
				} else {
					// Выполняем поиск таймера
					auto i = this->_timers.find(sid);
					// Если таймер найден
					if(i != this->_timers.end()){
						// Выполняем создание нового таймаута на 10 миллисекунд
						const uint16_t tid = i->second->timeout(10);
						// Выполняем добавление функции обратного вызова
						i->second->set <void (const uint16_t, const uint64_t)> (tid, std::bind(static_cast <void (core_t::*)(const uint16_t, const uint64_t)> (&core_t::accept), this, sid, bid));
					}
				}
			}
		}
	}
}
/**
 * launching Метод вызова при активации базы событий
 * @param mode   флаг работы с сетевым протоколом
 * @param status флаг вывода события статуса
 */
void awh::server::Core::launching(const bool mode, const bool status) noexcept {
	// Выполняем функцию в базовом модуле
	node_t::launching(mode, status);
	// Если список схем сети существует
	if(mode && !this->_schemes.empty()){
		// Объект работы с функциями обратного вызова
		fn_t callback(this->_log);
		// Переходим по всему списку схем сети
		for(auto & scheme : this->_schemes){
			// Если функция обратного вызова установлена
			if(this->_callbacks.is("open"))
				// Устанавливаем полученную функцию обратного вызова
				callback.set <void (const uint16_t)> (scheme.first, this->_callbacks.get <void (const uint16_t)> ("open"), scheme.first);
		}
		// Выполняем все функции обратного вызова
		callback.bind();
	}
}
/**
 * closedown Метод вызова при деакцтивации базы событий
 * @param mode   флаг работы с сетевым протоколом
 * @param status флаг вывода события статуса
 */
void awh::server::Core::closedown(const bool mode, const bool status) noexcept {
	// Выполняем функцию в базовом модуле
	node_t::closedown(mode, status);
	// Если требуется закрыть подключение
	if(mode)
		// Выполняем отключение всех брокеров
		this->close();
}
/**
 * cluster Метод события ЗАПУСКА/ОСТАНОВКИ кластера
 * @param sid   идентификатор схемы сети
 * @param pid   идентификатор процесса
 * @param event идентификатор события
 */
void awh::server::Core::cluster(const uint16_t sid, const pid_t pid, const cluster_t::event_t event) noexcept {
	// Выполняем поиск идентификатора схемы сети
	auto i = this->_schemes.find(sid);
	// Если идентификатор схемы сети найден, устанавливаем максимальное количество одновременных подключений
	if(i != this->_schemes.end()){
		// Получаем объект подключения
		scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
		// Определяем члена семейства кластера
		const cluster_t::family_t family = (this->_pid == ::getpid() ? cluster_t::family_t::MASTER : cluster_t::family_t::CHILDREN);
		// Выполняем тип возникшего события
		switch(static_cast <uint8_t> (event)){
			// Если производится запуск процесса
			case static_cast <uint8_t> (cluster_t::event_t::START): {
				// Определяем члена семейства кластера
				switch(static_cast <uint8_t> (family)){
					// Если процесс является родительским
					case static_cast <uint8_t> (cluster_t::family_t::MASTER): {
						// Если разрешено выводить информационыне уведомления
						if(this->_verb)
							// Выводим сообщение о том, что кластер запущен
							this->_log->print("Cluster has been successfully launched", log_t::flag_t::INFO);
						// Получаем список дочерних процессов
						const auto & workers = this->_cluster.pids(sid);
						// Если список доступных процессов получен
						if(!workers.empty()){
							// Выполняем перебор всего списка доступных процессов
							for(auto & worker : workers)
								// Выполняем заполнение списка доступных воркеров
								this->_workers.emplace(sid, worker);
						}
					} break;
					// Если процесс является дочерним
					case static_cast <uint8_t> (cluster_t::family_t::CHILDREN): {
						// Определяем тип сокета
						switch(static_cast <uint8_t> (this->_settings.sonet)){
							// Если тип сокета установлен как UDP
							case static_cast <uint8_t> (scheme_t::sonet_t::UDP):
								// Выполняем активацию сервера
								this->accept(1, sid);
							break;
							// Для всех остальных типов сокетов
							default: {
								// Выполняем поиск брокера в списке активных брокеров
								auto i = this->_brokers.find(sid);
								// Если активный брокер найден
								if(i != this->_brokers.end()){
									// Устанавливаем активный сокет сервера
									i->second->_addr.fd = shm->_addr.fd;
									// Выполняем установку базы событий
									i->second->base(this->_dispatch.base);
									// Активируем получение данных с клиента
									i->second->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::ACCEPT);
								// Если брокер не существует
								} else {
									// Выполняем блокировку потока
									this->_mtx.accept.lock();
									// Выполняем создание брокера подключения
									auto ret = this->_brokers.emplace(sid, unique_ptr <awh::scheme_t::broker_t> (new awh::scheme_t::broker_t(sid, this->_fmk, this->_log)));
									// Выполняем блокировку потока
									this->_mtx.accept.unlock();
									// Устанавливаем активный сокет сервера
									ret.first->second->_addr.fd = shm->_addr.fd;
									// Выполняем установку базы событий
									ret.first->second->base(this->_dispatch.base);
									// Выполняем установку функции обратного вызова на получении сообщений
									ret.first->second->callback <void (const SOCKET, const uint16_t)> ("accept", std::bind(static_cast <void (core_t::*)(const SOCKET, const uint16_t)> (&core_t::accept), this, _1, _2));
									// Активируем получение данных с клиента
									ret.first->second->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::ACCEPT);
								}
							}
						}
					} break;
				}
			} break;
			// Если производится остановка процесса
			case static_cast <uint8_t> (cluster_t::event_t::STOP): {
				// Если тип сокета не установлен как UDP
				if(this->_settings.sonet != scheme_t::sonet_t::UDP){
					// Выполняем поиск брокера в списке активных брокеров
					auto i = this->_brokers.find(sid);
					// Если активный брокер найден
					if(i != this->_brokers.end())
						// Деактивируем получение данных с клиента
						i->second->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::ACCEPT);
				}
			} break;
		}
		// Если функция обратного вызова установлена
		if(this->_callbacks.is("cluster"))
			// Выполняем функцию обратного вызова
			this->_callbacks.call <void (const cluster_t::family_t, const uint16_t, const pid_t, const cluster_t::event_t)> ("cluster", family, sid, pid, event);
	}
}
/**
 * message Метод получения сообщений от дочерних процессоров кластера
 * @param sid    идентификатор схемы сети
 * @param pid    идентификатор процесса
 * @param buffer буфер бинарных данных
 * @param size   размер буфера бинарных данных
 */
void awh::server::Core::message(const uint16_t sid, const pid_t pid, const char * buffer, const size_t size) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callbacks.is("message")){
		// Определяем члена семейства кластера
		switch(static_cast <uint8_t> (this->_pid == ::getpid() ? cluster_t::family_t::MASTER : cluster_t::family_t::CHILDREN)){
			// Если процесс является родительским
			case static_cast <uint8_t> (cluster_t::family_t::MASTER):
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const cluster_t::family_t, const uint16_t, const pid_t, const char *, const size_t)> ("message", cluster_t::family_t::MASTER, sid, pid, buffer, size);
			break;
			// Если процесс является дочерним
			case static_cast <uint8_t> (cluster_t::family_t::CHILDREN):
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const cluster_t::family_t, const uint16_t, const pid_t, const char *, const size_t)> ("message", cluster_t::family_t::CHILDREN, sid, pid, buffer, size);
			break;
		}
	}
}
/**
 * disable Метод остановки активности брокера подключения
 * @param bid идентификатор брокера
 */
void awh::server::Core::disable(const uint64_t bid) noexcept {
	// Если брокер существует
	if(this->has(bid)){
		// Выполняем извлечение брокера подключений
		const scheme_t::broker_t * broker = this->broker(bid);
		// Останавливаем событие чтение данных
		const_cast <scheme_t::broker_t *> (broker)->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::READ);
		// Останавливаем событие запись данных
		const_cast <scheme_t::broker_t *> (broker)->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::WRITE);
		// Выполняем блокировку на чтение/запись данных
		const_cast <scheme_t::broker_t *> (broker)->_bev.locked = scheme_t::locked_t();
	}
}
/**
 * dtlsBroker Метод инициализации DTLS-брокера
 * @param sid идентификатор схемы сети
 */
void awh::server::Core::dtlsBroker(const uint16_t sid) noexcept {
	// Если тип сокета установлен как DTLS и идентификатор схемы сети передан
	if((this->_settings.sonet == scheme_t::sonet_t::DTLS) && this->has(sid)){
		// Выполняем поиск идентификатора схемы сети
		auto i = this->_schemes.find(sid);
		// Если идентификатор схемы сети найден, устанавливаем максимальное количество одновременных подключений
		if(i != this->_schemes.end()){
			// Получаем объект схемы сети
			scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
			// Создаём бъект активного брокера подключения
			unique_ptr <awh::scheme_t::broker_t> broker(new awh::scheme_t::broker_t(sid, this->_fmk, this->_log));
			// Получаем идентификатор брокера подключения
			const uint64_t bid = broker->id();
			// Выполняем установку желаемого протокола подключения
			broker->_ectx.proto(this->_settings.proto);
			// Устанавливаем таймаут начтение данных из сокета
			broker->timeout(shm->timeouts.read, engine_t::method_t::READ);
			// Устанавливаем таймаут на запись данных в сокет
			broker->timeout(shm->timeouts.write, engine_t::method_t::WRITE);
			// Выполняем блокировку потока
			this->_mtx.accept.lock();
			// Устанавливаем активный сокет сервера
			broker->_addr.fd = shm->_addr.fd;
			// Выполняем получение контекста сертификата
			this->_engine.wrap(broker->_ectx, &shm->_addr, engine_t::type_t::SERVER);
			// Выполняем установку базы событий
			broker->base(this->_dispatch.base);
			// Добавляем созданного брокера в список брокеров
			auto ret = shm->_brokers.emplace(bid, std::forward <unique_ptr <awh::scheme_t::broker_t>> (broker));
			// Добавляем брокера в список подключений
			node_t::_brokers.emplace(ret.first->first, sid);
			// Выполняем разблокировку потока
			this->_mtx.accept.unlock();
			// Выполняем поиск таймера
			auto j = this->_timers.find(sid);
			// Если таймер ещё не создан
			if(j == this->_timers.end()){
				// Выполняем блокировку потока
				const lock_guard <recursive_mutex> lock(this->_mtx.timer);
				// Выполняем создание нового таймера
				auto ret = this->_timers.emplace(sid, unique_ptr <timer_t> (new timer_t(this->_fmk, this->_log)));
				// Устанавливаем флаг запрещающий вывод информационных сообщений
				ret.first->second->verbose(false);
				// Выполняем биндинг сетевого ядра таймера
				this->bind(dynamic_cast <awh::core_t *> (ret.first->second.get()));
			}
			// Выполняем установку функции обратного вызова на получении сообщений
			ret.first->second->callback <void (const uint64_t)> ("connect", std::bind(static_cast <void (core_t::*)(const uint16_t, const uint64_t)> (&core_t::accept), this, sid, _1));
			// Запускаем событие подключения клиента
			ret.first->second->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::CONNECT);
		}
	}
}
/**
 * stop Метод остановки клиента
 */
void awh::server::Core::stop() noexcept {
	// Если система уже запущена
	if(this->working()){
		// Выполняем закрытие подключения
		this->close();
		// Определяем режим активации кластера
		switch(static_cast <uint8_t> (this->_clusterMode)){
			// Если кластер необходимо активировать
			case static_cast <uint8_t> (awh::scheme_t::mode_t::ENABLED): {
				// Выполняем перебор всего списка сетевых схем
				for(auto & scheme : this->_schemes){
					// Если кластер работает
					if(this->_cluster.working(scheme.first))
						// Выполняем остановку работы кластеров
						this->_cluster.stop(scheme.first);
				}
			} break;
		}
		// Выполняем остановку работы сервера
		node_t::stop();
		// Выполняем очистку списка подключённых дочерних процессов
		this->_workers.clear();
	}
}
/**
 * start Метод запуска клиента
 */
void awh::server::Core::start() noexcept {
	// Если система ещё не запущена
	if(!this->working())
		// Выполняем запуск работы сервера
		node_t::start();
}
/**
 * close Метод отключения всех брокеров
 */
void awh::server::Core::close() noexcept {
	// Если список схем сети активен
	if(!this->_schemes.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.close);
		// Объект работы с функциями обратного вызова
		fn_t callback(this->_log);
		// Переходим по всему списку схем сети
		for(auto & item : this->_schemes){
			// Получаем объект схемы сети
			scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (item.second));
			// Если в схеме сети есть подключённые клиенты
			if(!shm->_brokers.empty()){
				// Переходим по всему списку брокера
				for(auto i = shm->_brokers.begin(); i != shm->_brokers.end();){
					// Если блокировка брокера не установлена
					if(this->_busy.find(i->first) == this->_busy.end()){
						// Выполняем блокировку брокера
						this->_busy.emplace(i->first);
						// Получаем бъект активного брокера подключения
						awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (i->second.get());
						// Выполняем очистку буфера событий
						this->disable(i->first);
						// Выполняем очистку контекста двигателя
						broker->_ectx.clear();
						// Удаляем брокера из списка подключений
						node_t::_brokers.erase(i->first);
						// Если функция обратного вызова установлена
						if(this->_callbacks.is("disconnect"))
							// Устанавливаем полученную функцию обратного вызова
							callback.set <void (const uint64_t, const uint16_t)> (i->first, this->_callbacks.get <void (const uint64_t, const uint16_t)> ("disconnect"), i->first, item.first);
						// Удаляем блокировку брокера
						this->_busy.erase(i->first);
						// Удаляем брокера из списка
						i = shm->_brokers.erase(i);
					// Иначе продолжаем дальше
					} else ++i;
				}
				// Выполняем поиск брокера в списке активных брокеров
				auto i = this->_brokers.find(shm->id);
				// Если активный брокер найден
				if(i != this->_brokers.end())
					// Деактивируем получение данных с клиента
					i->second->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::ACCEPT);
				// Останавливаем работу кластера
				this->_cluster.stop(shm->id);
			}
			// Выполняем закрытие подключение сервера
			shm->_addr.clear();
		}
		// Выполняем все функции обратного вызова
		callback.bind();
	}
}
/**
 * remove Метод удаления всех активных схем сети
 */
void awh::server::Core::remove() noexcept {
	// Если список схем сети активен
	if(!this->_schemes.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.close);
		// Если список таймеров не пустой
		if(!this->_timers.empty()){
			// Выполняем перебор всех таймеров
			for(auto i = this->_timers.begin(); i != this->_timers.end();){
				// Выполняем блокировку потока
				const lock_guard <recursive_mutex> lock(this->_mtx.timer);
				// Останавливаем работу таймера
				i->second->clear();
				// Выполняем анбиндинг сетевого ядра таймера
				this->unbind(dynamic_cast <awh::core_t *> (i->second.get()));
				// Выполняем удаление таймера
				i = this->_timers.erase(i);
			}
		}
		// Объект работы с функциями обратного вызова
		fn_t callback(this->_log);
		// Переходим по всему списку брокера
		for(auto i = this->_brokers.begin(); i != this->_brokers.end();){
			// Активируем получение данных с клиента
			i->second->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::ACCEPT);
			// Выполняем удаление брокера подключения
			i = this->_brokers.erase(i);
		}
		// Переходим по всему списку схем сети
		for(auto i = this->_schemes.begin(); i != this->_schemes.end();){
			// Получаем объект схемы сети
			scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
			// Если в схеме сети есть подключённые клиенты
			if(!shm->_brokers.empty()){
				// Переходим по всему списку брокера
				for(auto j = shm->_brokers.begin(); j != shm->_brokers.end();){
					// Если блокировка брокера не установлена
					if(this->_busy.find(j->first) == this->_busy.end()){
						// Выполняем блокировку брокера
						this->_busy.emplace(j->first);
						// Получаем бъект активного брокера подключения
						awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (j->second.get());
						// Выполняем очистку буфера событий
						this->disable(j->first);
						// Выполняем очистку контекста двигателя
						broker->_ectx.clear();
						// Удаляем брокера из списка подключений
						node_t::_brokers.erase(j->first);
						// Если функция обратного вызова установлена
						if(this->_callbacks.is("disconnect"))
							// Устанавливаем полученную функцию обратного вызова
							callback.set <void (const uint64_t, const uint16_t)> (j->first, this->_callbacks.get <void (const uint64_t, const uint16_t)> ("disconnect"), j->first, i->first);
						// Удаляем блокировку брокера
						this->_busy.erase(j->first);
						// Удаляем брокера из списка
						j = shm->_brokers.erase(j);
					// Иначе продолжаем дальше
					} else ++j;
				}
				// Останавливаем работу кластера
				this->_cluster.stop(shm->id);
			}
			// Выполняем закрытие подключение сервера
			shm->_addr.clear();
			// Выполняем удаление схемы сети
			i = this->_schemes.erase(i);
		}
		// Выполняем все функции обратного вызова
		callback.bind();
	}
}
/**
 * close Метод закрытия подключения брокера
 * @param bid идентификатор брокера
 */
void awh::server::Core::close(const uint64_t bid) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.close);
	// Если тип сокета установлен как не UDP, останавливаем чтение
	if(this->_settings.sonet != scheme_t::sonet_t::UDP){
		// Если блокировка брокера не установлена
		if(this->_busy.find(bid) == this->_busy.end()){
			// Выполняем блокировку брокера
			this->_busy.emplace(bid);
			// Объект работы с функциями обратного вызова
			fn_t callback(this->_log);
			// Если идентификатор брокера подключений существует
			if(this->has(bid)){
				// Создаём бъект активного брокера подключения
				awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (this->broker(bid));
				// Выполняем поиск идентификатора схемы сети
				auto i = this->_schemes.find(broker->sid());
				// Если идентификатор схемы сети найден
				if(i != this->_schemes.end()){
					// Получаем объект схемы сети
					scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
					// Выполняем очистку буфера событий
					this->disable(bid);
					// Выполняем очистку контекста двигателя
					broker->_ectx.clear();
					// Удаляем брокера из списка брокеров
					shm->_brokers.erase(bid);
					// Удаляем брокера из списка подключений
					node_t::_brokers.erase(bid);
					// Если разрешено выводить информационыне уведомления
					if(this->_verb)
						// Выводим информацию об удачном отключении от сервера
						this->_log->print("Disconnect client from server", log_t::flag_t::INFO);
					// Если функция обратного вызова установлена
					if(this->_callbacks.is("disconnect"))
						// Устанавливаем полученную функцию обратного вызова
						callback.set <void (const uint64_t, const uint16_t)> (bid, this->_callbacks.get <void (const uint64_t, const uint16_t)> ("disconnect"), bid, shm->id);
					// Если тип сокета установлен как DTLS, запускаем ожидание новых подключений
					if(this->_settings.sonet == scheme_t::sonet_t::DTLS){
						// Если функция обратного вызова установлена
						if(callback.is(bid))
							// Выполняем все функции обратного вызова
							callback.bind(bid);
						// Выполняем удаление старого подключения
						shm->_addr.clear();
						// Если сокет подключения получен
						if(this->create(shm->id))
							// Выполняем создание нового DTLS-брокера
							this->dtlsBroker(shm->id);
						// Если сокет не создан, выводим в консоль информацию
						else {
							// Если unix-сокет используется
							if(this->_settings.family == scheme_t::family_t::NIX){
								// Выводим информацию об незапущенном сервере на unix-сокете
								this->_log->print("Server cannot be init [%s]", log_t::flag_t::CRITICAL, this->_settings.sockname.c_str());
								// Если функция обратного вызова установлена
								if(this->_callbacks.is("error"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::START, this->_fmk->format("Server cannot be init [%s]", this->_settings.sockname.c_str()));
							// Если используется хост и порт
							} else {
								// Выводим сообщение об незапущенном сервере за порту
								this->_log->print("Server cannot be init [%s:%u]", log_t::flag_t::CRITICAL, shm->_host.c_str(), shm->_port);
								// Если функция обратного вызова установлена
								if(this->_callbacks.is("error"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::START, this->_fmk->format("Server cannot be init [%s:%u]", shm->_host.c_str(), shm->_port));
							}
						}
						// Удаляем блокировку брокера
						this->_busy.erase(bid);
						// Выходим из функции
						return;
					}
				}
			}
			// Удаляем блокировку брокера
			this->_busy.erase(bid);
			// Если функция обратного вызова установлена
			if(callback.is(bid))
				// Выполняем все функции обратного вызова
				callback.bind(bid);
		}
	}
}
/**
 * remove Метод удаления схемы сети
 * @param sid идентификатор схемы сети
 */
void awh::server::Core::remove(const uint16_t sid) noexcept {
	// Если идентификатор схемы сети существует
	if(this->has(sid)){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.close);
		// Выполняем поиск идентификатора схемы сети
		auto i = this->_schemes.find(sid);
		// Если идентификатор схемы сети найден, устанавливаем максимальное количество одновременных подключений
		if(i != this->_schemes.end()){
			// Если список таймеров не пустой
			if(!this->_timers.empty()){
				// Выполняем поиск таймера
				auto i = this->_timers.find(sid);
				// Если таймер найден удачно
				if(i != this->_timers.end()){
					// Выполняем блокировку потока
					const lock_guard <recursive_mutex> lock(this->_mtx.timer);
					// Останавливаем работу таймера
					i->second->clear();
					// Выполняем анбиндинг сетевого ядра таймера
					this->unbind(dynamic_cast <awh::core_t *> (i->second.get()));
					// Выполняем удаление таймера
					this->_timers.erase(i);
				}
			}
			// Объект работы с функциями обратного вызова
			fn_t callback(this->_log);
			// Выполняем поиск активного брокера ожидания подключения
			auto j = this->_brokers.find(sid);
			// Если брокерактивного подключения получен
			if(j != this->_brokers.end()){
				// Активируем получение данных с клиента
				j->second->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::ACCEPT);
				// Выполняем удаление брокера подключения
				this->_brokers.erase(j);
			}
			// Получаем объект схемы сети
			scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
			// Если в схеме сети есть подключённые клиенты
			if(!shm->_brokers.empty()){
				// Переходим по всему списку брокера
				for(auto j = shm->_brokers.begin(); j != shm->_brokers.end();){
					// Если блокировка брокера не установлена
					if(this->_busy.find(j->first) == this->_busy.end()){
						// Выполняем блокировку брокера
						this->_busy.emplace(j->first);
						// Получаем бъект активного брокера подключения
						awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (j->second.get());
						// Выполняем очистку буфера событий
						this->disable(j->first);
						// Выполняем очистку контекста двигателя
						broker->_ectx.clear();
						// Если функция обратного вызова установлена
						if(this->_callbacks.is("disconnect"))
							// Устанавливаем полученную функцию обратного вызова
							callback.set <void (const uint64_t, const uint16_t)> (j->first, this->_callbacks.get <void (const uint64_t, const uint16_t)> ("disconnect"), j->first, i->first);
						// Удаляем брокера из списка подключений
						node_t::_brokers.erase(j->first);
						// Удаляем блокировку брокера
						this->_busy.erase(j->first);
						// Удаляем брокера из списка
						j = shm->_brokers.erase(j);
					// Иначе продолжаем дальше
					} else ++j;
				}
			}
			// Выполняем закрытие подключение сервера
			shm->_addr.clear();
			// Выполняем удаление схемы сети
			this->_schemes.erase(sid);
			// Выполняем все функции обратного вызова
			callback.bind();
		}
	}
}
/**
 * launch Метод запуска сервера
 * @param sid идентификатор схемы сети
 */
void awh::server::Core::launch(const uint16_t sid) noexcept {
	// Если идентификатор схемы сети существует
	if(this->has(sid)){
		// Выполняем поиск идентификатора схемы сети
		auto i = this->_schemes.find(sid);
		// Если идентификатор схемы сети найден, устанавливаем максимальное количество одновременных подключений
		if(i != this->_schemes.end()){
			// Определяем режим активации кластера
			switch(static_cast <uint8_t> (this->_clusterMode)){
				// Если кластер необходимо активировать
				case static_cast <uint8_t> (awh::scheme_t::mode_t::ENABLED): {
					// Устанавливаем базу событий кластера
					this->_cluster.base(this->_dispatch.base);
					// Устанавливаем флаг отслеживания упавших процессов
					this->_cluster.trackCrash(this->_clusterAutoRestart);
					// Устанавливаем флаг автоматического перезапуска упавших процессов
					this->_cluster.restart(sid, this->_clusterAutoRestart);
					// Если количество процессов установленно
					if(this->_clusterSize >= 0)
						// Выполняем инициализацию кластера
						this->_cluster.init(sid, static_cast <uint16_t> (this->_clusterSize));
				} break;
			}
			// Получаем объект схемы сети
			scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
			// Если хост сервера не указан
			if(shm->_host.empty()){
				// Объект для работы с сетевым интерфейсом
				ifnet_t ifnet(this->_fmk, this->_log);
				// Определяем тип протокола подключения
				switch(static_cast <uint8_t> (this->_settings.family)){
					// Если тип протокола подключения unix-сокет
					case static_cast <uint8_t> (scheme_t::family_t::NIX):
					// Если тип протокола подключения IPv4
					case static_cast <uint8_t> (scheme_t::family_t::IPV4): {
						// Обновляем хост сервера
						shm->_host = ifnet.ip(AF_INET);
						// Выполняем подключения к полученному IP-адресу
						this->work(sid, shm->_host, AF_INET);
					} break;
					// Если тип протокола подключения IPv6
					case static_cast <uint8_t> (scheme_t::family_t::IPV6): {
						// Обновляем хост сервера
						shm->_host = ifnet.ip(AF_INET6);
						// Выполняем подключения к полученному IP-адресу
						this->work(sid, shm->_host, AF_INET6);
					} break;
				}
			// Если хост сервера является доменным именем
			} else {
				// Определяем тип передаваемого сервера
				switch(static_cast <uint8_t> (this->_net.host(shm->_host))){
					// Если домен является IPv4-адресом
					case static_cast <uint8_t> (net_t::type_t::IPV4):
						// Выполняем подключения к полученному IP-адресу
						this->work(sid, shm->_host, AF_INET);
					break;
					// Если домен является IPv6-адресом
					case static_cast <uint8_t> (net_t::type_t::IPV6):
						// Выполняем подключения к полученному IP-адресу
						this->work(sid, shm->_host, AF_INET6);
					break;
					// Если домен является адресом в файловой системе
					case static_cast <uint8_t> (net_t::type_t::FS): {
						// Запоминаем переданный адрес
						const string host = shm->_host;
						// Объект для работы с сетевым интерфейсом
						ifnet_t ifnet(this->_fmk, this->_log);
						// Обновляем хост сервера
						shm->_host = ifnet.ip(AF_INET);
						// Выводим сообщение об незапущенном сервере за порту
						this->_log->print("File system address [%s] was passed as server host, so we will use the address [%s] as server host", log_t::flag_t::WARNING, host.c_str(), shm->_host.c_str());
						// Выполняем подключения к полученному IP-адресу
						this->work(sid, shm->_host, AF_INET);
					} break;
					// Если домен является аппаратным адресом сетевого интерфейса
					case static_cast <uint8_t> (net_t::type_t::MAC): {
						// Запоминаем переданный адрес
						const string host = shm->_host;
						// Объект для работы с сетевым интерфейсом
						ifnet_t ifnet(this->_fmk, this->_log);
						// Обновляем хост сервера
						shm->_host = ifnet.ip(AF_INET);
						// Выводим сообщение об незапущенном сервере за порту
						this->_log->print("MAC-address [%s] was passed as server host, so we will use the address [%s] as server host", log_t::flag_t::WARNING, host.c_str(), shm->_host.c_str());
						// Выполняем подключения к полученному IP-адресу
						this->work(sid, shm->_host, AF_INET);
					} break;
					// Если домен является URL-адресом
					case static_cast <uint8_t> (net_t::type_t::URL): {
						// Запоминаем переданный адрес
						const string host = shm->_host;
						// Объект для работы с сетевым интерфейсом
						ifnet_t ifnet(this->_fmk, this->_log);
						// Обновляем хост сервера
						shm->_host = ifnet.ip(AF_INET);
						// Выводим сообщение об незапущенном сервере за порту
						this->_log->print("URL-address [%s] was passed as server host, so we will use the address [%s] as server host", log_t::flag_t::WARNING, host.c_str(), shm->_host.c_str());
						// Выполняем подключения к полученному IP-адресу
						this->work(sid, shm->_host, AF_INET);
					} break;
					// Если домен является адресом/Маски сети
					case static_cast <uint8_t> (net_t::type_t::NETWORK): {
						// Запоминаем переданный адрес
						const string host = shm->_host;
						// Объект для работы с сетевым интерфейсом
						ifnet_t ifnet(this->_fmk, this->_log);
						// Обновляем хост сервера
						shm->_host = ifnet.ip(AF_INET);
						// Выводим сообщение об незапущенном сервере за порту
						this->_log->print("Network-address [%s] was passed as server host, so we will use the address [%s] as server host", log_t::flag_t::WARNING, host.c_str(), shm->_host.c_str());
						// Выполняем подключения к полученному IP-адресу
						this->work(sid, shm->_host, AF_INET);
					} break;
					// Если домен является доменной зоной
					case static_cast <uint8_t> (net_t::type_t::ZONE): {
						// Если объект DNS-резолвера установлен
						if(this->_dns != nullptr) {
							// Определяем тип протокола подключения
							switch(static_cast <uint8_t> (this->_settings.family)){
								// Если тип протокола подключения unix-сокет
								case static_cast <uint8_t> (scheme_t::family_t::NIX):
								// Если тип протокола подключения IPv4
								case static_cast <uint8_t> (scheme_t::family_t::IPV4): {
									// Выполняем резолвинг домена
									const string & ip = const_cast <dns_t *> (this->_dns)->resolve(AF_INET, shm->_host);
									// Выполняем подключения к полученному IP-адресу
									this->work(sid, ip, AF_INET);
								} break;
								// Если тип протокола подключения IPv6
								case static_cast <uint8_t> (scheme_t::family_t::IPV6): {
									// Выполняем резолвинг домена
									const string & ip = const_cast <dns_t *> (this->_dns)->resolve(AF_INET6, shm->_host);
									// Выполняем подключения к полученному IP-адресу
									this->work(sid, ip, AF_INET6);
								} break;
							}
						}
					} break;
					// Если в качестве хоста был передан мусор
					default: {
						// Запоминаем переданный адрес
						const string host = shm->_host;
						// Объект для работы с сетевым интерфейсом
						ifnet_t ifnet(this->_fmk, this->_log);
						// Обновляем хост сервера
						shm->_host = ifnet.ip(AF_INET);
						// Выводим сообщение об незапущенном сервере за порту
						this->_log->print("Garbage [%s] was passed as server host, so we will use the address [%s] as server host", log_t::flag_t::WARNING, host.c_str(), shm->_host.c_str());
						// Выполняем подключения к полученному IP-адресу
						this->work(sid, shm->_host, AF_INET);
					}
				}
			}
		}
	}
}
/**
 * create Метод создания сервера
 * @param sid идентификатор схемы сети
 * @return    результат создания сервера
 */
bool awh::server::Core::create(const uint16_t sid) noexcept {
	// Результат работы функции
	bool result = false;
	// Если идентификатор схемы сети передан
	if(this->has(sid)){
		// Выполняем поиск идентификатора схемы сети
		auto i = this->_schemes.find(sid);
		// Если идентификатор схемы сети найден, устанавливаем максимальное количество одновременных подключений
		if(i != this->_schemes.end()){
			// Получаем объект схемы сети
			scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
			// Определяем тип протокола подключения
			switch(static_cast <uint8_t> (this->_settings.family)){
				// Если тип протокола подключения IPv4
				case static_cast <uint8_t> (scheme_t::family_t::IPV4): {
					// Выполняем перебор всего списка адресов
					for(auto & host : this->_settings.network){
						// Если хост соответствует адресу IPv4
						if(this->_net.host(host) == net_t::type_t::IPV4)
							// Выполняем установку полученного хоста
							shm->_addr.network.push_back(host);
					}
				} break;
				// Если тип протокола подключения IPv6
				case static_cast <uint8_t> (scheme_t::family_t::IPV6): {
					// Выполняем перебор всего списка адресов
					for(auto & host : this->_settings.network){
						// Если хост соответствует адресу IPv4
						if(this->_net.host(host) == net_t::type_t::IPV6)
							// Выполняем установку полученного хоста
							shm->_addr.network.push_back(host);
					}
				} break;
			}
			// Определяем тип сокета
			switch(static_cast <uint8_t> (this->_settings.sonet)){
				// Если тип сокета установлен как DTLS
				case static_cast <uint8_t> (scheme_t::sonet_t::DTLS):
					// Устанавливаем параметры сокета
					shm->_addr.sonet(SOCK_DGRAM, IPPROTO_UDP);
				break;
				/**
				 * Если операционной системой является Linux или FreeBSD
				 */
				#if defined(__linux__) || defined(__FreeBSD__)
					// Если тип сокета установлен как SCTP
					case static_cast <uint8_t> (scheme_t::sonet_t::SCTP):
						// Устанавливаем параметры сокета
						shm->_addr.sonet(SOCK_STREAM, IPPROTO_SCTP);
					break;
				#endif
				// Для всех остальных типов сокетов
				default:
					// Устанавливаем параметры сокета
					shm->_addr.sonet(SOCK_STREAM, IPPROTO_TCP);
			}
			// Если unix-сокет используется
			if(this->_settings.family == scheme_t::family_t::NIX)
				// Выполняем инициализацию сокета
				shm->_addr.init(this->_settings.sockname, engine_t::type_t::SERVER);
			// Если unix-сокет не используется, выполняем инициализацию сокета
			else shm->_addr.init(shm->_host, shm->_port, (this->_settings.family == scheme_t::family_t::IPV6 ? AF_INET6 : AF_INET), engine_t::type_t::SERVER, this->_settings.ipV6only);
			// Если сокет подключения получен
			if((shm->_addr.fd != INVALID_SOCKET) && (shm->_addr.fd < MAX_SOCKETS))
				// Выполняем прослушивание порта
				result = static_cast <bool> (shm->_addr.list());
		}
	}
	// Выводим результат создания сервера
	return result;
}
/**
 * port Метод получения порта сервера
 * @param sid идентификатор схемы сети
 * @return    порт сервера который он прослушивает
 */
u_int awh::server::Core::port(const uint16_t sid) const noexcept {
	// Выполняем поиск идентификатора схемы сети
	auto i = this->_schemes.find(sid);
	// Если идентификатор схемы сети найден, устанавливаем максимальное количество одновременных подключений
	if(i != this->_schemes.end()){
		// Получаем объект схемы сети
		scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
		// Выводим порт сервера который он прослушивает
		return shm->_port;
	}
	// Выводим неустановленный порт
	return 0;
}
/**
 * host Метод получения хоста сервера
 * @param sid идентификатор схемы сети
 * @return    хост на котором висит сервер
 */
const string & awh::server::Core::host(const uint16_t sid) const noexcept {
	// Результат работы функции
	static const string result = "";
	// Определяем тип протокола подключения
	switch(static_cast <uint8_t> (this->_settings.family)){
		// Если тип протокола подключения unix-сокет
		case static_cast <uint8_t> (scheme_t::family_t::NIX):
			// Выводим название unix-сокета
			return this->_settings.sockname;
		// Если адрес хоста принадлежит другому типу
		default: {
			// Выполняем поиск идентификатора схемы сети
			auto i = this->_schemes.find(sid);
			// Если идентификатор схемы сети найден, устанавливаем максимальное количество одновременных подключений
			if(i != this->_schemes.end()){
				// Получаем объект схемы сети
				scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
				// Выводим хост на котором висит сервер
				return shm->_host;
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * workers Метод получения списка доступных воркеров
 * @param sid идентификатор схемы сети
 * @return    список доступных воркеров
 */
set <pid_t> awh::server::Core::workers(const uint16_t sid) const noexcept {
	// Результат работы функции
	set <pid_t> result;
	// Если список дочерних воркеров получен
	if(!this->_workers.empty()){
		// Выполняем перебор списка дочерних воркеров
		const auto & range = this->_workers.equal_range(sid);
		// Выполняем перебор всего списка указанных заголовков
		for(auto i = range.first; i != range.second; ++i)
			// Выполняем заполнение списка полученных воркеров
			result.emplace(i->second);
	}
	// Выводим результат
	return result;
}
/**
 * send Метод отправки сообщения родительскому процессу
 * @param wid    идентификатор воркера
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 */
void awh::server::Core::send(const uint16_t wid, const char * buffer, const size_t size) noexcept {
	// Определяем члена семейства кластера
	switch(static_cast <uint8_t> (this->_pid == ::getpid() ? cluster_t::family_t::MASTER : cluster_t::family_t::CHILDREN)){
		// Если процесс является родительским
		case static_cast <uint8_t> (cluster_t::family_t::MASTER): {
			// Выводим сообщение в лог, потому что вещание доступно только из родительского процесса
			this->_log->print("Send message is only available from children process", log_t::flag_t::WARNING);
			// Если функция обратного вызова установлена
			if(this->_callbacks.is("error"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::CLUSTER, "Send message is only available from children process");
		} break;
		// Если процесс является дочерним
		case static_cast <uint8_t> (cluster_t::family_t::CHILDREN):
			// Выполняем отправку сообщения родительскому процессу
			this->_cluster.send(wid, buffer, size);
		break;
	}
}
/**
 * send Метод отправки сообщения дочернему процессу
 * @param wid    идентификатор воркера
 * @param pid    идентификатор процесса для получения сообщения
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 */
void awh::server::Core::send(const uint16_t wid, const pid_t pid, const char * buffer, const size_t size) noexcept {
	// Определяем члена семейства кластера
	switch(static_cast <uint8_t> (this->_pid == ::getpid() ? cluster_t::family_t::MASTER : cluster_t::family_t::CHILDREN)){
		// Если процесс является родительским
		case static_cast <uint8_t> (cluster_t::family_t::MASTER):
			// Выполняем отправку сообщения дочернему процессу
			this->_cluster.send(wid, pid, buffer, size);
		break;
		// Если процесс является дочерним
		case static_cast <uint8_t> (cluster_t::family_t::CHILDREN): {
			// Выводим сообщение в лог, потому что вещание доступно только из родительского процесса
			this->_log->print("Send message is only available from master process", log_t::flag_t::WARNING);
			// Если функция обратного вызова установлена
			if(this->_callbacks.is("error"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::CLUSTER, "Send message is only available from master process");
		} break;
	}
}
/**
 * broadcast Метод отправки сообщения всем дочерним процессам
 * @param wid    идентификатор воркера
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 */
void awh::server::Core::broadcast(const uint16_t wid, const char * buffer, const size_t size) noexcept {
	// Определяем члена семейства кластера
	switch(static_cast <uint8_t> (this->_pid == ::getpid() ? cluster_t::family_t::MASTER : cluster_t::family_t::CHILDREN)){
		// Если процесс является родительским
		case static_cast <uint8_t> (cluster_t::family_t::MASTER):
			// Выполняем отправку сообщения всем дочерним процессам
			this->_cluster.broadcast(wid, buffer, size);
		break;
		// Если процесс является дочерним
		case static_cast <uint8_t> (cluster_t::family_t::CHILDREN): {
			// Выводим сообщение в лог, потому что вещание доступно только из родительского процесса
			this->_log->print("Broadcast message is only available from master process", log_t::flag_t::WARNING);
			// Если функция обратного вызова установлена
			if(this->_callbacks.is("error"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::CLUSTER, "Broadcast is only available from master process");
		} break;
	}
}
/**
 * read Метод чтения данных для брокера
 * @param bid идентификатор брокера
 */
void awh::server::Core::read(const uint64_t bid) noexcept {
	// Если данные переданы
	if(this->working() && this->has(bid)){
		// Создаём бъект активного брокера подключения
		awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (this->broker(bid));
		// Если сокет подключения активен
		if((broker->_addr.fd != INVALID_SOCKET) && (broker->_addr.fd < MAX_SOCKETS)){
			// Останавливаем чтение данных с клиента
			broker->_bev.events.read.stop();
			// Выполняем поиск идентификатора схемы сети
			auto i = this->_schemes.find(broker->sid());
			// Если идентификатор схемы сети найден
			if(i != this->_schemes.end()){
				// Получаем объект схемы сети
				scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
				// Получаем максимальный размер буфера
				const int64_t size = broker->_ectx.buffer(engine_t::method_t::READ);
				// Если размер буфера получен
				if(size > 0){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Количество полученных байт
						int64_t bytes = -1;
						// Создаём буфер входящих данных
						unique_ptr <char []> buffer(new char [size]);
						// Выполняем чтение данных с сокета
						do {
							// Если подключение выполнено и чтение данных разрешено
							if(!broker->_bev.locked.read){
								// Выполняем обнуление буфера данных
								::memset(buffer.get(), 0, size);
								// Выполняем получение сообщения от клиента
								bytes = broker->_ectx.read(buffer.get(), size);
								// Если данные получены
								if(bytes > 0){
									// Если данные считанные из буфера, больше размера ожидающего буфера
									if((broker->_marker.read.max > 0) && (bytes >= broker->_marker.read.max)){
										// Смещение в буфере и отправляемый размер данных
										size_t offset = 0, actual = 0;
										// Выполняем пересылку всех полученных данных
										while((bytes - offset) > 0){
											// Определяем размер отправляемых данных
											actual = ((bytes - offset) >= broker->_marker.read.max ? broker->_marker.read.max : (bytes - offset));
											// Если функция обратного вызова на получение данных установлена
											if(this->_callbacks.is("read"))
												// Выводим функцию обратного вызова
												this->_callbacks.call <void (const char *, const size_t, const uint64_t, const uint16_t)> ("read", buffer.get() + offset, actual, bid, shm->id);
											// Увеличиваем смещение в буфере
											offset += actual;
										}
									// Если данных достаточно и функция обратного вызова на получение данных установлена
									} else if(this->_callbacks.is("read"))
										// Выводим функцию обратного вызова
										this->_callbacks.call <void (const char *, const size_t, const uint64_t, const uint16_t)> ("read", buffer.get(), bytes, bid, shm->id);
								// Если данные небыли получены
								} else if(bytes <= 0) {
									// Если чтение не выполнена, закрываем подключение
									if(bytes == 0)
										// Выполняем закрытие подключения
										this->close(bid);
									// Выходим из цикла
									break;
								}
							// Выходим из цикла
							} else break;
						// Выполняем чтение до тех пор, пока всё не прочитаем
						} while(this->has(bid));
						// Если тип сокета не установлен как UDP, запускаем чтение дальше
						if((this->_settings.sonet != scheme_t::sonet_t::UDP) && this->has(bid))
							// Запускаем событие на чтение базы событий
							broker->_bev.events.read.start();
					/**
					 * Если возникает ошибка
					 */
					} catch(const bad_alloc &) {
						// Выходим из приложения
						::exit(EXIT_FAILURE);
					}
				// Выполняем отключение клиента
				} else this->close(bid);
			// Если схема сети не существует
			} else {
				// Выводим сообщение в лог, о таймауте подключения
				this->_log->print("Connection Broker %llu does not belong to a non-existent network diagram", log_t::flag_t::CRITICAL, bid);
				// Если функция обратного вызова установлена
				if(this->_callbacks.is("error"))
					// Выполняем функцию обратного вызова
					this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::PROTOCOL, this->_fmk->format("Connection Broker %llu does not belong to a non-existent network diagram", bid));
			}
		// Если файловый дескриптор сломан, значит с памятью что-то не то
		} else if(broker->_addr.fd > 65535) {
			// Удаляем из памяти объект брокера
			node_t::remove(bid);
			// Выводим в лог сообщение
			this->_log->print("Socket for read is not initialized", log_t::flag_t::WARNING);
			// Если функция обратного вызова установлена
			if(this->_callbacks.is("error"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::PROTOCOL, "Socket for read is not initialized");
		}
	}
}
/**
 * write Метод записи буфера данных в сокет
 * @param buffer буфер для записи данных
 * @param size   размер записываемых данных
 * @param bid    идентификатор брокера
 */
void awh::server::Core::write(const char * buffer, const size_t size, const uint64_t bid) noexcept {
	// Если данные переданы
	if(this->working() && this->has(bid) && (buffer != nullptr) && (size > 0)){
		// Создаём бъект активного брокера подключения
		awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (this->broker(bid));
		// Если сокет подключения активен
		if((broker->_addr.fd != INVALID_SOCKET) && (broker->_addr.fd < MAX_SOCKETS)){
			// Выполняем поиск идентификатора схемы сети
			auto i = this->_schemes.find(broker->sid());
			// Если идентификатор схемы сети найден
			if(i != this->_schemes.end()){
				// Определяем тип сокета
				switch(static_cast <uint8_t> (this->_settings.sonet)){
					// Если тип сокета установлен как UDP
					case static_cast <uint8_t> (scheme_t::sonet_t::UDP):
					// Если тип сокета установлен как TCP/IP
					case static_cast <uint8_t> (scheme_t::sonet_t::TCP):
					// Если тип сокета установлен как TCP/IP TLS
					case static_cast <uint8_t> (scheme_t::sonet_t::TLS):
					// Если тип сокета установлен как SCTP
					case static_cast <uint8_t> (scheme_t::sonet_t::SCTP):
						// Переводим сокет в блокирующий режим
						broker->_ectx.block();
					break;
				}
				// Получаем объект схемы сети
				scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
				// Если данных достаточно для записи в сокет
				if(size >= broker->_marker.write.min){
					// Количество полученных байт
					int64_t bytes = -1;
					// Cмещение в буфере и отправляемый размер данных
					size_t offset = 0, actual = 0, left = 0;
					// Получаем максимальный размер буфера
					int64_t max = broker->_ectx.buffer(engine_t::method_t::WRITE);
					// Если максимальное установленное значение больше размеров буфера для записи, корректируем
					max = ((max > 0) && (broker->_marker.write.max > max) ? max : broker->_marker.write.max);
					// Активируем ожидание записи данных
					broker->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::WRITE);
					// Выполняем отправку данных пока всё не отправим
					while((size - offset) > 0){
						// Получаем общий размер буфера данных
						left = (size - offset);
						// Определяем размер отправляемых данных
						actual = (left >= max ? max : left);
						// Выполняем установку таймаута ожидания записи в сокет
						broker->_ectx.timeout(broker->_timeouts.write * 1000, engine_t::method_t::WRITE);
						// Выполняем отправку сообщения клиенту
						bytes = broker->_ectx.write(buffer + offset, actual);
						// Если данные небыли записаны
						if(bytes <= 0){
							// Если запись не выполнена, закрываем подключение
							if(bytes == 0)
								// Выполняем закрытие подключения
								this->close(bid);
							// Выходим из цикла
							break;
						}
						// Увеличиваем смещение в буфере
						offset += bytes;
					}
					// Если дисконнекта от сервера не произошло
					if(bytes > 0){
						// Определяем тип сокета
						switch(static_cast <uint8_t> (this->_settings.sonet)){
							// Если тип сокета установлен как UDP
							case static_cast <uint8_t> (scheme_t::sonet_t::UDP):
							// Если тип сокета установлен как TCP/IP
							case static_cast <uint8_t> (scheme_t::sonet_t::TCP):
							// Если тип сокета установлен как TCP/IP TLS
							case static_cast <uint8_t> (scheme_t::sonet_t::TLS):
							// Если тип сокета установлен как SCTP
							case static_cast <uint8_t> (scheme_t::sonet_t::SCTP):
								// Переводим сокет в неблокирующий режим
								broker->_ectx.noblock();
							break;
						}
						// Останавливаем запись данных
						broker->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::WRITE);
					}
					// Если функция обратного вызова на запись данных установлена
					if(this->_callbacks.is("write"))
						// Выводим функцию обратного вызова
						this->_callbacks.call <void (const char *, const size_t, const uint64_t, const uint16_t)> ("write", buffer, offset, bid, shm->id);
				// Если данных недостаточно для записи в сокет
				} else {
					// Останавливаем запись данных
					broker->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::WRITE);
					// Если функция обратного вызова на запись данных установлена
					if(this->_callbacks.is("write"))
						// Выводим функцию обратного вызова
						this->_callbacks.call <void (const char *, const size_t, const uint64_t, const uint16_t)> ("write", nullptr, 0, bid, shm->id);
				}
				// Если тип сокета установлен как UDP, и данных для записи больше нет, запускаем чтение
				if((this->_settings.sonet == scheme_t::sonet_t::UDP) && this->has(bid))
					// Запускаем событие на чтение базы событий
					broker->_bev.events.read.start();
			// Если схема сети не существует
			} else {
				// Выводим сообщение в лог, о таймауте подключения
				this->_log->print("Connection Broker %llu does not belong to a non-existent network diagram", log_t::flag_t::CRITICAL, bid);
				// Если функция обратного вызова установлена
				if(this->_callbacks.is("error"))
					// Выполняем функцию обратного вызова
					this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::PROTOCOL, this->_fmk->format("Connection Broker %llu does not belong to a non-existent network diagram", bid));
			}
		// Если файловый дескриптор сломан, значит с памятью что-то не то
		} else if(broker->_addr.fd > 65535) {
			// Удаляем из памяти объект брокера
			node_t::remove(bid);
			// Выводим в лог сообщение
			this->_log->print("Socket for write is not initialized", log_t::flag_t::WARNING);
			// Если функция обратного вызова установлена
			if(this->_callbacks.is("error"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::PROTOCOL, "Socket for write is not initialized");
		}
	}
}
/**
 * work Метод активации параметров запуска сервера
 * @param sid    идентификатор схемы сети
 * @param ip     адрес интернет-подключения
 * @param family тип интернет-протокола AF_INET, AF_INET6
 */
void awh::server::Core::work(const uint16_t sid, const string & ip, const int family) noexcept {
	// Если идентификатор схемы сети передан
	if(this->has(sid)){
		// Выполняем поиск идентификатора схемы сети
		auto i = this->_schemes.find(sid);
		// Если идентификатор схемы сети найден, устанавливаем максимальное количество одновременных подключений
		if(i != this->_schemes.end()){
			// Получаем объект схемы сети
			scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
			// Если IP-адрес получен
			if(!ip.empty()){
				// sudo lsof -i -P | grep 1080
				// Обновляем хост сервера
				shm->_host = ip;
				// Определяем тип сокета
				switch(static_cast <uint8_t> (this->_settings.sonet)){
					// Если тип сокета установлен как UDP
					case static_cast <uint8_t> (scheme_t::sonet_t::UDP): {
						// Если разрешено выводить информационные сообщения
						if(this->_verb){
							// Если unix-сокет используется
							if(this->_settings.family == scheme_t::family_t::NIX)
								// Выводим информацию о запущенном сервере на unix-сокете
								this->_log->print("Start server [%s]", log_t::flag_t::INFO, this->_settings.sockname.c_str());
							// Если unix-сокет не используется, выводим сообщение о запущенном сервере за порту
							else this->_log->print("Start server [%s:%u]", log_t::flag_t::INFO, shm->_host.c_str(), shm->_port);
						}
						// Определяем режим активации кластера
						switch(static_cast <uint8_t> (this->_clusterMode)){
							// Если кластер необходимо активировать
							case static_cast <uint8_t> (awh::scheme_t::mode_t::ENABLED): {
								// Выводим в консоль информацию
								this->_log->print("Working in cluster mode for \"UDP-protocol\" is not supported PID=%d", log_t::flag_t::WARNING, ::getpid());
								// Если функция обратного вызова установлена
								if(this->_callbacks.is("error"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::START, this->_fmk->format("Working in cluster mode for \"UDP-protocol\" is not supported PID=%d", ::getpid()));
							} break;
						}
						// Выполняем активацию сервера
						this->accept(1, sid);
						// Выходим из функции
						return;
					} break;
					// Если тип сокета установлен как DTLS
					case static_cast <uint8_t> (scheme_t::sonet_t::DTLS): {
						// Если сокет подключения получен
						if(this->create(sid)){
							// Если разрешено выводить информационные сообщения
							if(this->_verb){
								// Если unix-сокет используется
								if(this->_settings.family == scheme_t::family_t::NIX)
									// Выводим информацию о запущенном сервере на unix-сокете
									this->_log->print("Start server [%s]", log_t::flag_t::INFO, this->_settings.sockname.c_str());
								// Если unix-сокет не используется, выводим сообщение о запущенном сервере за порту
								else this->_log->print("Start server [%s:%u]", log_t::flag_t::INFO, shm->_host.c_str(), shm->_port);
							}
							// Определяем режим активации кластера
							switch(static_cast <uint8_t> (this->_clusterMode)){
								// Если кластер необходимо активировать
								case static_cast <uint8_t> (awh::scheme_t::mode_t::ENABLED): {
									// Выводим в консоль информацию
									this->_log->print("Working in cluster mode for \"DTLS-protocol\" is not supported PID=%d", log_t::flag_t::WARNING, ::getpid());
									// Если функция обратного вызова установлена
									if(this->_callbacks.is("error"))
										// Выполняем функцию обратного вызова
										this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::START, this->_fmk->format("Working in cluster mode for \"DTLS-protocol\" is not supported PID=%d", ::getpid()));
								} break;
							}
							// Выполняем создание нового DTLS-брокера
							this->dtlsBroker(sid);
							// Выходим из функции
							return;
						// Если сокет не создан, выводим в консоль информацию
						} else {
							// Если unix-сокет используется
							if(this->_settings.family == scheme_t::family_t::NIX){
								// Выводим информацию об незапущенном сервере на unix-сокете
								this->_log->print("Server cannot be started [%s]", log_t::flag_t::CRITICAL, this->_settings.sockname.c_str());
								// Если функция обратного вызова установлена
								if(this->_callbacks.is("error"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::START, this->_fmk->format("Server cannot be started [%s]", this->_settings.sockname.c_str()));
							// Если используется хост и порт
							} else {
								// Выводим сообщение об незапущенном сервере за порту
								this->_log->print("Server cannot be started [%s:%u]", log_t::flag_t::CRITICAL, shm->_host.c_str(), shm->_port);
								// Если функция обратного вызова установлена
								if(this->_callbacks.is("error"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::START, this->_fmk->format("Server cannot be started [%s:%u]", shm->_host.c_str(), shm->_port));
							}
						}
					} break;
					// Для всех остальных типов сокетов
					default: {
						// Если сокет подключения получен
						if(this->create(sid)){
							// Если разрешено выводить информационные сообщения
							if(this->_verb){
								// Если unix-сокет используется
								if(this->_settings.family == scheme_t::family_t::NIX)
									// Выводим информацию о запущенном сервере на unix-сокете
									this->_log->print("Start server [%s]", log_t::flag_t::INFO, this->_settings.sockname.c_str());
								// Если unix-сокет не используется, выводим сообщение о запущенном сервере за порту
								else this->_log->print("Start server [%s:%u]", log_t::flag_t::INFO, shm->_host.c_str(), shm->_port);
							}
							// Определяем режим активации кластера
							switch(static_cast <uint8_t> (this->_clusterMode)){
								// Если кластер необходимо активировать
								case static_cast <uint8_t> (awh::scheme_t::mode_t::ENABLED):
									// Выполняем запуск кластера
									this->_cluster.start(sid);
								break;
								// Если кластер необходимо деактивировать
								case static_cast <uint8_t> (awh::scheme_t::mode_t::DISABLED): {
									// Выполняем поиск брокера в списке активных брокеров
									auto i = this->_brokers.find(sid);
									// Если активный брокер найден
									if(i != this->_brokers.end()){
										// Устанавливаем активный сокет сервера
										i->second->_addr.fd = shm->_addr.fd;
										// Выполняем установку базы событий
										i->second->base(this->_dispatch.base);
										// Активируем получение данных с клиента
										i->second->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::ACCEPT);
									// Если брокер не существует
									} else {
										// Выполняем блокировку потока
										this->_mtx.accept.lock();
										// Выполняем создание брокера подключения
										auto ret = this->_brokers.emplace(sid, unique_ptr <awh::scheme_t::broker_t> (new awh::scheme_t::broker_t(sid, this->_fmk, this->_log)));
										// Выполняем блокировку потока
										this->_mtx.accept.unlock();
										// Устанавливаем активный сокет сервера
										ret.first->second->_addr.fd = shm->_addr.fd;
										// Выполняем установку базы событий
										ret.first->second->base(this->_dispatch.base);
										// Выполняем установку функции обратного вызова на получении сообщений
										ret.first->second->callback <void (const SOCKET, const uint16_t)> ("accept", std::bind(static_cast <void (core_t::*)(const SOCKET, const uint16_t)> (&core_t::accept), this, _1, _2));
										// Активируем получение данных с клиента
										ret.first->second->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::ACCEPT);
									}
								} break;
							}
							// Выходим из функции
							return;
						// Если сокет не создан, выводим в консоль информацию
						} else {
							// Если unix-сокет используется
							if(this->_settings.family == scheme_t::family_t::NIX){
								// Выводим информацию об незапущенном сервере на unix-сокете
								this->_log->print("Server cannot be started [%s]", log_t::flag_t::CRITICAL, this->_settings.sockname.c_str());
								// Если функция обратного вызова установлена
								if(this->_callbacks.is("error"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::START, this->_fmk->format("Server cannot be started [%s]", this->_settings.sockname.c_str()));
							// Если используется хост и порт
							} else {
								// Выводим сообщение об незапущенном сервере за порту
								this->_log->print("Server cannot be started [%s:%u]", log_t::flag_t::CRITICAL, shm->_host.c_str(), shm->_port);
								// Если функция обратного вызова установлена
								if(this->_callbacks.is("error"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::START, this->_fmk->format("Server cannot be started [%s:%u]", shm->_host.c_str(), shm->_port));
							}
						}
					}
				}
			// Если IP-адрес сервера не получен
			} else {
				// Выводим в консоль информацию
				this->_log->print("Broken host server %s", log_t::flag_t::CRITICAL, shm->_host.c_str());
				// Если функция обратного вызова установлена
				if(this->_callbacks.is("error"))
					// Выполняем функцию обратного вызова
					this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::START, this->_fmk->format("Broken host server %s", shm->_host.c_str()));
			}
			// Останавливаем работу сервера
			this->stop();
		}
	}
}
/**
 * ipV6only Метод установки флага использования только сети IPv6
 * @param mode флаг для установки
 */
void awh::server::Core::ipV6only(const bool mode) noexcept {
	// Выполняем установку флаг использования только сети IPv6
	this->_settings.ipV6only = mode;
}
/**
 * callbacks Метод установки функций обратного вызова
 * @param callbacks функции обратного вызова
 */
void awh::server::Core::callbacks(const fn_t & callbacks) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Устанавливаем функций обратного вызова
	awh::core_t::callbacks(callbacks);
	// Выполняем установку функции обратного вызова на событие запуска и остановки процессов кластера
	this->_callbacks.set("cluster", callbacks);
}
/**
 * total Метод установки максимального количества одновременных подключений
 * @param sid   идентификатор схемы сети
 * @param total максимальное количество одновременных подключений
 */
void awh::server::Core::total(const uint16_t sid, const u_short total) noexcept {
	// Если идентификатор схемы сети передан
	if(this->has(sid)){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.main);
		// Выполняем поиск идентификатора схемы сети
		auto i = this->_schemes.find(sid);
		// Если идентификатор схемы сети найден, устанавливаем максимальное количество одновременных подключений
		if(i != this->_schemes.end())
			// Устанавливаем максимальное количество одновременных подключений
			(dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second)))->_total = total;
	}
}
/**
 * clusterAutoRestart Метод установки флага перезапуска процессов
 * @param sid  идентификатор схемы сети
 * @param mode флаг перезапуска процессов
 */
void awh::server::Core::clusterAutoRestart(const uint16_t sid, const bool mode) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.main);
		// Разрешаем автоматический перезапуск упавших процессов
		this->_clusterAutoRestart = mode;
	/**
	 * Если операционной системой является Windows
	 */
	#else
		// Выводим предупредительное сообщение в лог
		this->_log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
		// Если функция обратного вызова установлена
		if(this->_callbacks.is("error"))
			// Выполняем функцию обратного вызова
			this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::OS_BROKEN, "MS Windows OS, does not support cluster mode");
	#endif
}
/**
 * cluster Метод установки количества процессов кластера
 * @param mode флаг активации/деактивации кластера
 * @param size количество рабочих процессов
 */
void awh::server::Core::cluster(const awh::scheme_t::mode_t mode, const int16_t size) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.main);
		// Активируем режим работы кластера
		this->_clusterMode = mode;
		// Определяем режим активации кластера
		switch(static_cast <uint8_t> (mode)){
			// Если кластер необходимо активировать
			case static_cast <uint8_t> (awh::scheme_t::mode_t::ENABLED): {
				// Если количество процессов установлено меньше 2-х, отключаем работу кластера
				if((size < 2) && (size != 0)){
					// Устанавливаем количество рабочих процессов кластера
					this->_clusterSize = -1;
					// Деактивируем режим работы кластера
					this->_clusterMode = awh::scheme_t::mode_t::DISABLED;
				// Если количество воркеров установленно больше чем разрешено
				} else if(size > (static_cast <int16_t> (std::thread::hardware_concurrency()) * 2))
					// Устанавливаем количество рабочих процессов кластера
					this->_clusterSize = static_cast <int16_t> (std::thread::hardware_concurrency());
				// Устанавливаем количество рабочих процессов кластера
				else this->_clusterSize = size;
			} break;
			// Если кластер необходимо деактивировать
			case static_cast <uint8_t> (awh::scheme_t::mode_t::DISABLED):
				// Устанавливаем количество рабочих процессов кластера
				this->_clusterSize = -1;
			break;
		}
	/**
	 * Если операционной системой является Windows
	 */
	#else
		// Выводим предупредительное сообщение в лог
		this->_log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
		// Если функция обратного вызова установлена
		if(this->_callbacks.is("error"))
			// Выполняем функцию обратного вызова
			this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::OS_BROKEN, "MS Windows OS, does not support cluster mode");
	#endif
}
/**
 * init Метод инициализации сервера
 * @param sid  идентификатор схемы сети
 * @param port порт сервера
 * @param host хост сервера
 */
void awh::server::Core::init(const uint16_t sid, const u_int port, const string & host) noexcept {
	// Если идентификатор схемы сети передан
	if(this->has(sid)){
		// Выполняем поиск идентификатора схемы сети
		auto i = this->_schemes.find(sid);
		// Если идентификатор схемы сети найден, устанавливаем максимальное количество одновременных подключений
		if(i != this->_schemes.end()){
			// Выполняем блокировку потока
			const lock_guard <recursive_mutex> lock(this->_mtx.main);
			// Получаем объект схемы сети
			scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
			// Если порт передан, устанавливаем
			if(port > 0){
				// Устанавливаем порт
				shm->_port = port;
				// Определяем тип сокета
				switch(static_cast <uint8_t> (this->_settings.sonet)){
					// Если тип сокета установлен как UDP
					case static_cast <uint8_t> (scheme_t::sonet_t::UDP):
					// Если тип сокета установлен как TCP/IP
					case static_cast <uint8_t> (scheme_t::sonet_t::TCP): {
						// Определяем тип установленного порта
						switch(shm->_port){
							// Если порт HTTP установлен незащищённый то исправляем его
							case SERVER_SEC_PORT:
								// Устанавливаем порт по умолчанию
								shm->_port = SERVER_PORT;
							break;
							// Если порт PROXY установлен незащищённый то исправляем его
							case SERVER_PROXY_SEC_PORT:
								// Устанавливаем порт по умолчанию
								shm->_port = SERVER_PROXY_PORT;
							break;
						}
					} break;
					// Если тип сокета установлен как TCP/IP TLS
					case static_cast <uint8_t> (scheme_t::sonet_t::TLS):
					// Если подключение зашифрованно
					case static_cast <uint8_t> (scheme_t::sonet_t::DTLS):
					// Если тип сокета установлен как SCTP
					case static_cast <uint8_t> (scheme_t::sonet_t::SCTP): {
						// Определяем тип установленного порта
						switch(shm->_port){
							// Если порт HTTP установлен незащищённый то исправляем его
							case SERVER_PORT:
								// Устанавливаем порт по умолчанию
								shm->_port = SERVER_SEC_PORT;
							break;
							// Если порт PROXY установлен незащищённый то исправляем его
							case SERVER_PROXY_PORT:
								// Устанавливаем порт по умолчанию
								shm->_port = SERVER_PROXY_SEC_PORT;
							break;
						}
					} break;
				}
			// Если порт сервера не установлен
			} else {
				// Определяем тип сокета
				switch(static_cast <uint8_t> (this->_settings.sonet)){
					// Если тип сокета установлен как UDP
					case static_cast <uint8_t> (scheme_t::sonet_t::UDP):
					// Если тип сокета установлен как TCP/IP
					case static_cast <uint8_t> (scheme_t::sonet_t::TCP):
						// Устанавливаем порт по умолчанию
						shm->_port = SERVER_PORT;
					break;
					// Если тип сокета установлен как TCP/IP TLS
					case static_cast <uint8_t> (scheme_t::sonet_t::TLS):
					// Если подключение зашифрованно
					case static_cast <uint8_t> (scheme_t::sonet_t::DTLS):
					// Если тип сокета установлен как SCTP
					case static_cast <uint8_t> (scheme_t::sonet_t::SCTP):
						// Устанавливаем порт по умолчанию
						shm->_port = SERVER_SEC_PORT;
					break;
				}
			}
			// Если хост передан, устанавливаем
			if(!host.empty())
				// Устанавливаем хост
				shm->_host = host;
			// Иначе получаем IP-адрес сервера автоматически
			else {
				// Объект для работы с сетевым интерфейсом
				ifnet_t ifnet(this->_fmk, this->_log);
				// Определяем тип протокола подключения
				switch(static_cast <uint8_t> (this->_settings.family)){
					// Если тип протокола подключения unix-сокет
					case static_cast <uint8_t> (scheme_t::family_t::NIX):
					// Если тип протокола подключения IPv4
					case static_cast <uint8_t> (scheme_t::family_t::IPV4):
						// Обновляем хост сервера
						shm->_host = ifnet.ip(AF_INET);
					break;
					// Если тип протокола подключения IPv6
					case static_cast <uint8_t> (scheme_t::family_t::IPV6):
						// Обновляем хост сервера
						shm->_host = ifnet.ip(AF_INET6);
					break;
				}
			}
		}
	}
}
/**
 * Core Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::Core::Core(const fmk_t * fmk, const log_t * log) noexcept :
 awh::node_t(fmk, log), _pid(::getpid()), _cluster(fmk, log), _socket(fmk, log),
 _clusterSize(-1), _clusterAutoRestart(false), _clusterMode(awh::scheme_t::mode_t::DISABLED) {
	// Устанавливаем тип запускаемого ядра
	this->_type = engine_t::type_t::SERVER;
	// Отключаем отслеживание упавших процессов
	this->_cluster.trackCrash(this->_clusterAutoRestart);
	// Устанавливаем функцию получения сообщений процессов кластера
	this->_cluster.callback <void (const uint16_t, const pid_t, const char *, const size_t)> ("message", std::bind(&core_t::message, this, _1, _2, _3, _4));
	// Устанавливаем функцию получения статуса кластера
	this->_cluster.callback <void (const uint16_t, const pid_t, const cluster_t::event_t)> ("process", std::bind(static_cast <void (core_t::*)(const uint16_t, const pid_t, const cluster_t::event_t)> (&core_t::cluster), this, _1, _2, _3));
}
/**
 * Core Конструктор
 * @param dns объект DNS-резолвера
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::Core::Core(const dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept :
 awh::node_t(dns, fmk, log), _pid(::getpid()), _cluster(fmk, log), _socket(fmk, log),
 _clusterSize(-1), _clusterAutoRestart(false), _clusterMode(awh::scheme_t::mode_t::DISABLED) {
	// Устанавливаем тип запускаемого ядра
	this->_type = engine_t::type_t::SERVER;
	// Отключаем отслеживание упавших процессов
	this->_cluster.trackCrash(this->_clusterAutoRestart);
	// Устанавливаем функцию получения сообщений процессов кластера
	this->_cluster.callback <void (const uint16_t, const pid_t, const char *, const size_t)> ("message", std::bind(&core_t::message, this, _1, _2, _3, _4));
	// Устанавливаем функцию получения статуса кластера
	this->_cluster.callback <void (const uint16_t, const pid_t, const cluster_t::event_t)> ("process", std::bind(static_cast <void (core_t::*)(const uint16_t, const pid_t, const cluster_t::event_t)> (&core_t::cluster), this, _1, _2, _3));
}
