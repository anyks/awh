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
 * @copyright: Copyright © 2025
 */

/**
 * Подключаем заголовочный файл
 */
#include <core/server.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * accept Метод вызова при подключении к серверу
 * @param fd  файловый дескриптор (сокет) подключившегося клиента
 * @param sid идентификатор схемы сети
 */
void awh::server::Core::accept(const SOCKET fd, const uint16_t sid) noexcept {
	// Если идентификатор схемы сети передан
	if((sid > 0) && (fd != INVALID_SOCKET) && (fd < AWH_MAX_SOCKETS)){
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
						if(!this->master()){
							// Выводим в консоль информацию
							this->_log->print("Working in child processes for \"UDP-protocol\" is not supported PID=%d", log_t::flag_t::WARNING, ::getpid());
							// Если функция обратного вызова установлена
							if(this->_callback.is("error"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::ACCEPT, this->_fmk->format("Working in child processes for \"UDP-protocol\" is not supported PID=%d", ::getpid()));
							// Выходим
							break;
						// Выполняем остановку работы получения запроса на подключение
						} else {
							// Создаём бъект активного брокера подключения
							unique_ptr <awh::scheme_t::broker_t> broker(new awh::scheme_t::broker_t(sid, this->_fmk, this->_log));
							/**
							 * !!!!!! ВНИМАНИЕ !!!!!!
							 * Нельзя устанавливать таймаут на чтение и запись, так-как по истечению таймаута будет закрыт сокет сервера а не клиента
							 * Примеры установки таймаутов здесь стоят для демонстрации, что их добавлять сюда не надо!!!
							 */
							// Отключаем таймаут начтение данных из сокета
							broker->timeout(0, engine_t::method_t::READ);
							// Отключаем таймаут на запись данных в сокет
							broker->timeout(0, engine_t::method_t::WRITE);
							// Определяем тип протокола подключения
							switch(static_cast <uint8_t> (this->_settings.family)){
								// Если тип протокола подключения IPv4
								case static_cast <uint8_t> (scheme_t::family_t::IPV4): {
									// Выполняем перебор всего списка адресов
									for(auto & host : this->_settings.network){
										// Если хост соответствует адресу IPv4
										if(this->_net.host(host) == net_t::type_t::IPV4)
											// Выполняем установку полученного хоста
											broker->addr.network.push_back(host);
									}
								} break;
								// Если тип протокола подключения IPv6
								case static_cast <uint8_t> (scheme_t::family_t::IPV6): {
									// Выполняем перебор всего списка адресов
									for(auto & host : this->_settings.network){
										// Если хост соответствует адресу IPv4
										if(this->_net.host(host) == net_t::type_t::IPV6)
											// Выполняем установку полученного хоста
											broker->addr.network.push_back(host);
									}
								} break;
							}
							// Устанавливаем параметры сокета
							broker->addr.sonet(SOCK_DGRAM, IPPROTO_UDP);
							// Если unix-сокет используется
							if(this->_settings.family == scheme_t::family_t::IPC){
								// Если название unix-сокета ещё не инициализированно
								if(this->_settings.sockname.empty())
									// Выполняем установку названия unix-сокета
									this->sockname();
								// Выполняем инициализацию сокета
								broker->addr.init(this->_fmk->format("%s/%s.sock", this->_settings.sockpath.c_str(), this->_settings.sockname.c_str()), engine_t::type_t::SERVER);
							// Если unix-сокет не используется, выполняем инициализацию сокета
							} else broker->addr.init(shm->_host, shm->_port, (this->_settings.family == scheme_t::family_t::IPV6 ? AF_INET6 : AF_INET), engine_t::type_t::SERVER, this->_settings.ipV6only);
							// Выполняем разрешение подключения
							if(broker->addr.accept(broker->addr.fd, 0)){
								// Получаем адрес подключения клиента
								broker->ip(broker->addr.ip);
								// Получаем аппаратный адрес клиента
								broker->mac(broker->addr.mac);
								// Получаем порт подключения клиента
								broker->port(broker->addr.port);
								// Выполняем установку желаемого протокола подключения
								broker->ectx.proto(this->_settings.proto);
								// Выполняем получение контекста сертификата
								this->_engine.wrap(broker->ectx, &broker->addr);
								// Если подключение не обёрнуто
								if((broker->addr.fd == INVALID_SOCKET) || (broker->addr.fd >= AWH_MAX_SOCKETS)){
									// Выводим сообщение об ошибке
									this->_log->print("Wrap engine context is failed", log_t::flag_t::CRITICAL);
									// Если функция обратного вызова установлена
									if(this->_callback.is("error"))
										// Выполняем функцию обратного вызова
										this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::ACCEPT, "Wrap engine context is failed");
									// Выходим из функции
									return;
								}
								// Выполняем блокировку потока
								this->_mtx.accept.lock();
								// Выполняем установку базы событий
								broker->base(this->eventBase());
								// Добавляем созданного брокера в список брокеров
								auto ret = shm->_brokers.emplace(broker->id(), std::forward <unique_ptr <awh::scheme_t::broker_t>> (broker));
								// Добавляем брокера в список подключений
								node_t::_brokers.emplace(ret.first->first, ret.first->second.get());
								// Выполняем блокировку потока
								this->_mtx.accept.unlock();
								// Переводим сокет в неблокирующий режим
								ret.first->second->ectx.blocking(engine_t::mode_t::DISABLED);
								// Выполняем установку функции обратного вызова на получении сообщений
								ret.first->second->on <void (const uint64_t)> ("read", &core_t::read, this, _1);
								// Выполняем установку функции обратного вызова на получение сигнала закрытия подключения
								ret.first->second->on <void (const uint64_t)> ("close", static_cast <void (core_t::*)(const uint16_t, const uint64_t)> (&core_t::close), this, sid, _1);
								// Выполняем запуск работы события
								ret.first->second->start();
								// Активируем получение данных с клиента
								ret.first->second->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::READ);
								// Деактивируем ожидание записи данных
								ret.first->second->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::WRITE);
								// Выполняем создание буфера полезной нагрузки
								this->initBuffer(ret.first->second->id());
								// Если функция обратного вызова установлена
								if(this->_callback.is("connect"))
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const uint64_t, const uint16_t)> ("connect", ret.first->first, sid);
							// Подключение не установлено
							} else {
								// Выводим сообщение об ошибке
								this->_log->print("Accepting failed, PID=%d", log_t::flag_t::WARNING, ::getpid());
								// Если функция обратного вызова установлена
								if(this->_callback.is("error"))
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::ACCEPT, this->_fmk->format("Accepting failed, PID=%d", ::getpid()));
							}
						}
					/**
					 * Если возникает ошибка
					 */
					} catch(const bad_alloc &) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fd, sid), log_t::flag_t::CRITICAL, "Memory allocation error");
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
						#endif
						// Выходим из приложения
						::exit(EXIT_FAILURE);
					/**
					 * Если возникает ошибка
					 */
					} catch(const exception & error) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fd, sid), log_t::flag_t::CRITICAL, error.what());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						#endif
					}
				} break;
				// Если тип сокета установлен как TCP/IP
				case static_cast <uint8_t> (scheme_t::sonet_t::TCP):
				// Если тип сокета установлен как TCP/IP TLS
				case static_cast <uint8_t> (scheme_t::sonet_t::TLS):
				// Если тип сокета установлен как SCTP
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
							if(this->_callback.is("error"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::ACCEPT, this->_fmk->format("Number of simultaneous connections, cannot exceed maximum allowed number of %d", shm->_total));
							// Выходим
							break;
						}
						// Создаём бъект активного брокера подключения
						unique_ptr <awh::scheme_t::broker_t> broker(new awh::scheme_t::broker_t(sid, this->_fmk, this->_log));
						// Устанавливаем время жизни подключения
						broker->addr.alive = shm->keepAlive;
						// Выполняем установку времени ожидания входящих сообщений
						broker->timeouts.wait = shm->timeouts.wait;
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
									broker->addr.sonet(SOCK_STREAM, IPPROTO_SCTP);
								break;
							#endif
							// Для всех остальных типов сокетов
							default:
								// Устанавливаем параметры сокета
								broker->addr.sonet(SOCK_STREAM, IPPROTO_TCP);
						}
						// Выполняем разрешение подключения
						if(broker->addr.accept(shm->_addr)){
							// Если MAC или IP-адрес не получен, тогда выходим
							if(broker->addr.mac.empty() || broker->addr.ip.empty()){
								// Выполняем очистку контекста двигателя
								broker->ectx.clear();
								// Если подключение не установлено, выводим сообщение об ошибке
								this->_log->print("Client address not received, PID=%d", log_t::flag_t::WARNING, ::getpid());
								// Если функция обратного вызова установлена
								if(this->_callback.is("error"))
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::ACCEPT, this->_fmk->format("Client address not received, PID=%d", ::getpid()));
							// Если все данные получены
							} else {
								// Получаем адрес подключения клиента
								broker->ip(broker->addr.ip);
								// Получаем аппаратный адрес клиента
								broker->mac(broker->addr.mac);
								// Получаем порт подключения клиента
								broker->port(broker->addr.port);
								// Если функция обратного вызова проверки подключения установлена, выполняем проверку, если проверка не пройдена?
								if(this->_callback.is("accept") && !this->_callback.call <bool (const string &, const string &, const uint32_t, const uint16_t)> ("accept", broker->ip(), broker->mac(), broker->port(), sid)){
									// Если порт установлен
									if(broker->port() > 0){
										// Определяем тип протокола подключения
										switch(static_cast <uint8_t> (this->_settings.family)){
											// Если тип протокола подключения unix-сокет
											case static_cast <uint8_t> (scheme_t::family_t::IPC): {
												// Выводим сообщение об ошибке
												this->_log->print(
													"Access to server [%s] PID=%d is denied for client [%s:%d] MAC=%s, SOCKET=%d",
													log_t::flag_t::WARNING,
													this->host(sid).c_str(),
													::getpid(),
													broker->ip().c_str(),
													broker->port(),
													broker->mac().c_str(),
													broker->addr.fd
												);
												// Если функция обратного вызова установлена
												if(this->_callback.is("error"))
													// Выполняем функцию обратного вызова
													this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> (
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
															broker->addr.fd
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
													broker->addr.fd
												);
												// Если функция обратного вызова установлена
												if(this->_callback.is("error"))
													// Выполняем функцию обратного вызова
													this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> (
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
															broker->addr.fd
														)
													);
											} break;
										}
									// Если порт не установлен
									} else {
										// Определяем тип протокола подключения
										switch(static_cast <uint8_t> (this->_settings.family)){
											// Если тип протокола подключения unix-сокет
											case static_cast <uint8_t> (scheme_t::family_t::IPC): {
												// Выводим сообщение об ошибке
												this->_log->print(
													"Access to server [%s] PID=%d is denied for client [%s] MAC=%s, SOCKET=%d",
													log_t::flag_t::WARNING,
													this->host(sid).c_str(),
													::getpid(),
													broker->ip().c_str(),
													broker->mac().c_str(),
													broker->addr.fd
												);
												// Если функция обратного вызова установлена
												if(this->_callback.is("error"))
													// Выполняем функцию обратного вызова
													this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> (
														"error",
														log_t::flag_t::WARNING,
														error_t::ACCEPT,
														this->_fmk->format(
															"Access to server [%s] PID=%d is denied for client [%s] MAC=%s, SOCKET=%d",
															this->host(sid).c_str(),
															::getpid(),
															broker->ip().c_str(),
															broker->mac().c_str(),
															broker->addr.fd
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
													broker->addr.fd
												);
												// Если функция обратного вызова установлена
												if(this->_callback.is("error"))
													// Выполняем функцию обратного вызова
													this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> (
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
															broker->addr.fd
														)
													);
											} break;
										}
									}
									// Выполняем очистку контекста двигателя
									broker->ectx.clear();
									// Выходим
									break;
								}
								// Выполняем установку желаемого протокола подключения
								broker->ectx.proto(this->_settings.proto);
								// Выполняем получение контекста сертификата
								this->_engine.wrap(broker->ectx, &broker->addr);
								// Если мы хотим работать в зашифрованном режиме
								if(this->_settings.sonet == scheme_t::sonet_t::TLS){
									// Если сертификаты не приняты, выходим
									if(!this->_engine.encrypted(broker->ectx)){
										// Выполняем очистку контекста двигателя
										broker->ectx.clear();
										// Выводим сообщение об ошибке
										this->_log->print("Encryption mode cannot be activated", log_t::flag_t::CRITICAL);
										// Если функция обратного вызова установлена
										if(this->_callback.is("error"))
											// Выполняем функцию обратного вызова
											this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::ACCEPT, "Encryption mode cannot be activated");
										// Выходим
										break;
									}
								}
								// Если подключение не обёрнуто
								if((broker->addr.fd == INVALID_SOCKET) || (broker->addr.fd >= AWH_MAX_SOCKETS)){
									// Выводим сообщение об ошибке
									this->_log->print("Wrap engine context is failed", log_t::flag_t::CRITICAL);
									// Если функция обратного вызова установлена
									if(this->_callback.is("error"))
										// Выполняем функцию обратного вызова
										this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::ACCEPT, "Wrap engine context is failed");
									// Выходим
									break;
								}
								// Выполняем блокировку потока
								this->_mtx.accept.lock();
								// Выполняем установку базы событий
								broker->base(this->eventBase());
								// Добавляем созданного брокера в список брокеров
								auto ret = shm->_brokers.emplace(broker->id(), std::forward <unique_ptr <awh::scheme_t::broker_t>> (broker));
								// Добавляем брокера в список подключений
								node_t::_brokers.emplace(ret.first->first, ret.first->second.get());
								// Выполняем блокировку потока
								this->_mtx.accept.unlock();
								// Переводим сокет в неблокирующий режим
								ret.first->second->ectx.blocking(engine_t::mode_t::DISABLED);
								// Если вывод информационных данных не запрещён
								if(this->_info){
									// Если порт установлен
									if(ret.first->second->port() > 0){
										// Определяем тип протокола подключения
										switch(static_cast <uint8_t> (this->_settings.family)){
											// Если тип протокола подключения unix-сокет
											case static_cast <uint8_t> (scheme_t::family_t::IPC): {
												// Выводим в консоль информацию
												this->_log->print(
													"Connected client [%s:%d] MAC=%s, SOCKET=%d to server [%s] PID=%d",
													log_t::flag_t::INFO,
													ret.first->second->ip().c_str(),
													ret.first->second->port(),
													ret.first->second->mac().c_str(),
													ret.first->second->addr.fd,
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
													ret.first->second->addr.fd,
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
											case static_cast <uint8_t> (scheme_t::family_t::IPC): {
												// Выводим в консоль информацию
												this->_log->print(
													"Connected client [%s] MAC=%s, SOCKET=%d to server [%s] PID=%d",
													log_t::flag_t::INFO,
													ret.first->second->ip().c_str(),
													ret.first->second->mac().c_str(),
													ret.first->second->addr.fd,
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
													ret.first->second->addr.fd,
													this->host(sid).c_str(),
													this->port(sid),
													::getpid()
												);
											} break;
										}
									}
								}
								// Выполняем установку функции обратного вызова на получении сообщений
								ret.first->second->on <void (const uint64_t)> ("read", &core_t::read, this, _1);
								// Выполняем установку функции обратного вызова на отправку сообщений
								ret.first->second->on <void (const uint64_t)> ("write", static_cast <void (core_t::*)(const uint64_t)> (&core_t::write), this, _1);
								// Выполняем установку функции обратного вызова на получение сигнала закрытия подключения
								ret.first->second->on <void (const uint64_t)> ("close", static_cast <void (core_t::*)(const uint64_t)> (&core_t::close), this, _1);
								// Выполняем запуск работы события
								ret.first->second->start();
								// Активируем получение данных с клиента
								ret.first->second->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::READ);
								// Деактивируем ожидание записи данных
								ret.first->second->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::WRITE);
								// Выполняем создание буфера полезной нагрузки
								this->initBuffer(ret.first->second->id());
								// Если функция обратного вызова установлена
								if(this->_callback.is("connect"))
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const uint64_t, const uint16_t)> ("connect", ret.first->first, sid);
							}
						// Если подключение не установлено
						} else {
							// Определяем режим активации кластера
							switch(static_cast <uint8_t> (this->_clusterMode)){
								// Если кластер необходимо активировать
								case static_cast <uint8_t> (awh::scheme_t::mode_t::ENABLED):
									// Выводим сообщение об ошибке
									this->_log->print("Node PID=%d is ready to receive new clients", log_t::flag_t::INFO, ::getpid());
								break;
								// Если кластер необходимо деактивировать
								case static_cast <uint8_t> (awh::scheme_t::mode_t::DISABLED): {
									// Выводим сообщение об ошибке
									this->_log->print("Accepting failed, PID=%d", log_t::flag_t::WARNING, ::getpid());
									// Если функция обратного вызова установлена
									if(this->_callback.is("error"))
										// Выполняем функцию обратного вызова
										this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::ACCEPT, this->_fmk->format("Accepting failed, PID=%d", ::getpid()));
								} break;
							}
						}
					/**
					 * Если возникает ошибка
					 */
					} catch(const bad_alloc &) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fd, sid), log_t::flag_t::CRITICAL, "Memory allocation error");
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
						#endif
						// Выходим из приложения
						::exit(EXIT_FAILURE);
					/**
					 * Если возникает ошибка
					 */
					} catch(const exception & error) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fd, sid), log_t::flag_t::CRITICAL, error.what());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						#endif
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
				// Получаем бъект активного брокера подключения
				awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (j->second.get());
				// Останавливаем событие подключения клиента
				broker->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::READ);
				// Выполняем ожидание входящих подключений
				if(this->_engine.wait(broker->ectx)){
					// Устанавливаем параметры сокета
					broker->addr.sonet(SOCK_DGRAM, IPPROTO_UDP);
					// Если прикрепление клиента к серверу выполнено
					if(broker->addr.attach(shm->_addr)){
						// Выполняем прикрепление контекста клиента к контексту сервера
						this->_engine.attach(broker->ectx, &broker->addr);
						// Если MAC или IP-адрес не получен, тогда выходим
						if(broker->addr.mac.empty() || broker->addr.ip.empty()){
							// Выполняем очистку контекста двигателя
							broker->ectx.clear();
							// Если подключение не установлено, выводим сообщение об ошибке
							this->_log->print("Client address not received, PID=%d", log_t::flag_t::WARNING, ::getpid());
							// Если функция обратного вызова установлена
							if(this->_callback.is("error"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::ACCEPT, this->_fmk->format("Client address not received, PID=%d", ::getpid()));
						// Если все данные получены
						} else {
							// Получаем адрес подключения клиента
							broker->ip(broker->addr.ip);
							// Получаем аппаратный адрес клиента
							broker->mac(broker->addr.mac);
							// Получаем порт подключения клиента
							broker->port(broker->addr.port);
							// Если функция обратного вызова проверки подключения установлена, выполняем проверку, если проверка не пройдена?
							if(this->_callback.is("accept") && !this->_callback.call <bool (const string &, const string &, const uint32_t, const uint16_t)> ("accept", broker->ip(), broker->mac(), broker->port(), sid)){
								// Если порт установлен
								if(broker->port() > 0){
									// Определяем тип протокола подключения
									switch(static_cast <uint8_t> (this->_settings.family)){
										// Если тип протокола подключения unix-сокет
										case static_cast <uint8_t> (scheme_t::family_t::IPC): {
											// Выводим сообщение об ошибке
											this->_log->print(
												"Access to server [%s] PID=%d is denied for client [%s:%d] MAC=%s, SOCKET=%d",
												log_t::flag_t::WARNING,
												this->host(sid).c_str(),
												::getpid(),
												broker->ip().c_str(),
												broker->port(),
												broker->mac().c_str(),
												broker->addr.fd
											);
											// Если функция обратного вызова установлена
											if(this->_callback.is("error"))
												// Выполняем функцию обратного вызова
												this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> (
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
														broker->addr.fd
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
												broker->addr.fd
											);
											// Если функция обратного вызова установлена
											if(this->_callback.is("error"))
												// Выполняем функцию обратного вызова
												this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> (
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
														broker->addr.fd
													)
												);
										} break;
									}
								// Если порт не установлен
								} else {
									// Определяем тип протокола подключения
									switch(static_cast <uint8_t> (this->_settings.family)){
										// Если тип протокола подключения unix-сокет
										case static_cast <uint8_t> (scheme_t::family_t::IPC): {
											// Выводим сообщение об ошибке
											this->_log->print(
												"Access to server [%s] PID=%d is denied for client [%s] MAC=%s, SOCKET=%d",
												log_t::flag_t::WARNING,
												this->host(sid).c_str(),
												::getpid(),
												broker->ip().c_str(),
												broker->mac().c_str(),
												broker->addr.fd
											);
											// Если функция обратного вызова установлена
											if(this->_callback.is("error"))
												// Выполняем функцию обратного вызова
												this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> (
													"error",
													log_t::flag_t::WARNING,
													error_t::ACCEPT,
													this->_fmk->format(
														"Access to server [%s] PID=%d is denied for client [%s] MAC=%s, SOCKET=%d",
														this->host(sid).c_str(),
														::getpid(),
														broker->ip().c_str(),
														broker->mac().c_str(),
														broker->addr.fd
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
												broker->addr.fd
											);
											// Если функция обратного вызова установлена
											if(this->_callback.is("error"))
												// Выполняем функцию обратного вызова
												this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> (
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
														broker->addr.fd
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
							// Переводим сокет в неблокирующий режим
							broker->ectx.blocking(engine_t::mode_t::DISABLED);
							// Если вывод информационных данных не запрещён
							if(this->_info){
								// Если порт установлен
								if(broker->port() > 0){
									// Определяем тип протокола подключения
									switch(static_cast <uint8_t> (this->_settings.family)){
										// Если тип протокола подключения unix-сокет
										case static_cast <uint8_t> (scheme_t::family_t::IPC): {
											// Выводим в консоль информацию
											this->_log->print(
												"Connected client [%s:%d] MAC=%s, SOCKET=%d to server [%s] PID=%d",
												log_t::flag_t::INFO,
												broker->ip().c_str(),
												broker->port(),
												broker->mac().c_str(),
												broker->addr.fd,
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
												broker->addr.fd,
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
										case static_cast <uint8_t> (scheme_t::family_t::IPC): {
											// Выводим в консоль информацию
											this->_log->print(
												"Connected client [%s], MAC=%s, SOCKET=%d to server [%s] PID=%d",
												log_t::flag_t::INFO,
												broker->ip().c_str(),
												broker->mac().c_str(),
												broker->addr.fd,
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
												broker->addr.fd,
												this->host(sid).c_str(),
												this->port(sid),
												::getpid()
											);
										} break;
									}
								}
							}
							// Выполняем установку функции обратного вызова на получении сообщений
							broker->on <void (const uint64_t)> ("read", &core_t::read, this, _1);
							// Выполняем установку функции обратного вызова на отправку сообщений
							broker->on <void (const uint64_t)> ("write", static_cast <void (core_t::*)(const uint64_t)> (&core_t::write), this, _1);
							// Выполняем установку функции обратного вызова на получение сигнала закрытия подключения
							broker->on <void (const uint64_t)> ("close", static_cast <void (core_t::*)(const uint16_t, const uint64_t)> (&core_t::close), this, sid, _1);
							// Выполняем запуск работы события
							broker->start();
							// Активируем получение данных с клиента
							broker->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::READ);
							// Деактивируем ожидание записи данных
							broker->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::WRITE);
							// Выполняем создание буфера полезной нагрузки
							this->initBuffer(broker->id());
							// Если функция обратного вызова установлена
							if(this->_callback.is("connect"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const uint64_t, const uint16_t)> ("connect", bid, sid);
							// Выполняем создание нового таймера
							this->createTimeout(sid, bid, 100, mode_t::READ);
						}
					// Подключение не установлено
					} else {
						// Получаем текст полученной ошибки
						const string & message = this->_socket.message(AWH_ERROR());
						// Выводим сообщение об ошибке
						this->_log->print("%s, PID=%d", log_t::flag_t::WARNING, message.c_str(), ::getpid());
						// Если функция обратного вызова установлена
						if(this->_callback.is("error"))
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::ACCEPT, this->_fmk->format("%s, PID=%d", message.c_str(), ::getpid()));
						// Выполняем удаление объекта подключения
						this->close(bid);
					}
				// Проверяем наличие нового клиента ещё раз
				} else this->createTimeout(sid, bid, 10, mode_t::ACCEPT);
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
		callback_t callback(this->_log);
		// Переходим по всему списку схем сети
		for(auto & scheme : this->_schemes){
			// Если функция обратного вызова установлена
			if(this->_callback.is("open"))
				// Устанавливаем полученную функцию обратного вызова
				callback.on <void (const uint16_t)> (scheme.first, this->_callback.get <void (const uint16_t)> ("open"), scheme.first);
		}
		// Выполняем все функции обратного вызова
		callback.call();
	}
}
/**
 * closedown Метод вызова при деакцтивации базы событий
 * @param mode   флаг работы с сетевым протоколом
 * @param status флаг вывода события статуса
 */
void awh::server::Core::closedown(const bool mode, const bool status) noexcept {
	// Если требуется закрыть подключение
	if(mode)
		// Выполняем отключение всех брокеров
		this->close();
	// Выполняем функцию в базовом модуле
	node_t::closedown(mode, status);
}
/**
 * clearTimeout Метод удаления таймера ожидания получения данных
 * @param bid идентификатор брокера
 */
void awh::server::Core::clearTimeout(const uint64_t bid) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.receive);
	// Выполняем поиск активных таймаутов
	auto i = this->_receive.find(bid);
	// Если таймаут найден
	if(i != this->_receive.end()){
		// Если таймер инициализирован
		if(this->_timer != nullptr)
			// Выполняем удаление активных таймеров
			this->_timer->clear(i->second);
		// Удаляем таймаут из базы таймаутов
		this->_receive.erase(i);
	}
}
/**
 * clearTimeout Метод удаления таймера подключения или переподключения
 * @param sid идентификатор схемы сети
 */
void awh::server::Core::clearTimeout(const uint16_t sid) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.timeout);
	// Выполняем поиск активных таймаутов
	auto i = this->_timeouts.find(sid);
	// Если таймаут найден
	if(i != this->_timeouts.end()){
		// Если таймер инициализирован
		if(this->_timer != nullptr)
			// Выполняем удаление активных таймеров
			this->_timer->clear(i->second);
		// Удаляем таймаут из базы таймаутов
		this->_timeouts.erase(i);
	}
}
/**
 * createTimeout Метод создания таймаута подключения или переподключения
 * @param sid  идентификатор схемы сети
 * @param bid  идентификатор брокера
 * @param msec время ожидания получения данных в миллисекундах
 * @param mode режим создания таймера
 */
void awh::server::Core::createTimeout(const uint16_t sid, const uint64_t bid, const uint32_t msec, const mode_t mode) noexcept {
	// Если данные переданы верные
	if((bid > 0) && (mode != mode_t::NONE) && (msec > 0)){
		// Если таймер не инициализирован
		if(this->_timer == nullptr){
			// Выполняем блокировку потока
			const lock_guard <recursive_mutex> lock1(this->_mtx.receive);
			// Выполняем блокировку потока
			const lock_guard <recursive_mutex> lock2(this->_mtx.timeout);
			// Выполняем инициализацию нового таймера
			this->_timer = make_unique <timer_t> (this->_fmk, this->_log);
			// Устанавливаем флаг запрещающий вывод информационных сообщений
			this->_timer->verbose(false);
			// Выполняем биндинг сетевого ядра таймера
			this->bind(dynamic_cast <awh::core_t *> (this->_timer.get()));
		}
		// Определяем режим создания таймера
		switch(static_cast <uint8_t> (mode)){
			// Если необходимо создать таймер на чтение данных
			case static_cast <uint8_t> (mode_t::READ): {
				// Если идентификатор схемы сети найден
				if(this->has(sid)){
					// Идентификатор таймера
					uint16_t tid = 0;
					// Выполняем поиск активных таймаутов
					auto i = this->_timeouts.find(sid);
					// Если таймаут найден
					if(i != this->_timeouts.end()){
						// Выполняем удаление активного таймера
						this->_timer->clear(i->second);
						// Выполняем создание нового таймаута
						i->second = tid = this->_timer->timeout(msec);
					// Если таймаут ещё не создан
					} else {
						// Выполняем блокировку потока
						const lock_guard <recursive_mutex> lock(this->_mtx.timeout);
						// Выполняем создание нового таймаута
						this->_timeouts.emplace(sid, (tid = this->_timer->timeout(msec)));
					}
					// Выполняем добавление функции обратного вызова
					this->_timer->on(tid, static_cast <void (core_t::*)(const uint64_t)> (&core_t::read), this, bid);
				}
			} break;
			// Если необходимо создать таймер на разрешение подключения
			case static_cast <uint8_t> (mode_t::ACCEPT): {
				// Если идентификатор схемы сети найден
				if(this->has(sid)){
					// Идентификатор таймера
					uint16_t tid = 0;
					// Выполняем поиск активных таймаутов
					auto i = this->_timeouts.find(sid);
					// Если таймаут найден
					if(i != this->_timeouts.end()){
						// Выполняем удаление активного таймера
						this->_timer->clear(i->second);
						// Выполняем создание нового таймаута
						i->second = tid = this->_timer->timeout(msec);
					// Если таймаут ещё не создан
					} else {
						// Выполняем блокировку потока
						const lock_guard <recursive_mutex> lock(this->_mtx.timeout);
						// Выполняем создание нового таймаута
						this->_timeouts.emplace(sid, (tid = this->_timer->timeout(msec)));
					}
					// Выполняем добавление функции обратного вызова
					this->_timer->on(tid, static_cast <void (core_t::*)(const uint16_t, const uint64_t)> (&core_t::accept), this, sid, bid);
				}
			} break;
			// Если необходимо создать таймер на ожидание входящих данных
			case static_cast <uint8_t> (mode_t::RECEIVE): {
				// Если идентификатор брокера найден
				if(this->has(bid)){
					// Идентификатор таймера
					uint16_t tid = 0;
					// Выполняем поиск активных таймаутов
					auto i = this->_receive.find(bid);
					// Если таймаут найден
					if(i != this->_receive.end()){
						// Выполняем удаление активного таймера
						this->_timer->clear(i->second);
						// Выполняем создание нового таймаута
						i->second = tid = this->_timer->timeout(msec);
					// Если таймаут ещё не создан
					} else {
						// Выполняем блокировку потока
						const lock_guard <recursive_mutex> lock(this->_mtx.receive);
						// Выполняем создание нового таймаута
						this->_receive.emplace(bid, (tid = this->_timer->timeout(msec)));
					}
					// Выполняем добавление функции обратного вызова
					this->_timer->on(tid, static_cast <void (core_t::*)(const uint64_t)> (&core_t::close), this, bid);
				}
			} break;
		}
	}
}
/**
 * rebase Метод события пересоздании процесса
 * @param sid  идентификатор схемы сети
 * @param pid  идентификатор процесса
 * @param opid идентификатор старого процесса
 */
void awh::server::Core::rebase(const uint16_t sid, const pid_t pid, const pid_t opid) const noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("rebase"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const uint16_t, const pid_t, const pid_t)> ("rebase", sid, pid, opid);
}
/**
 * exit Метод события завершения работы процесса
 * @param sid    идентификатор схемы сети
 * @param pid    идентификатор процесса
 * @param status статус остановки работы процесса
 */
void awh::server::Core::exit(const uint16_t sid, const pid_t pid, const int32_t status) const noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("exit"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const uint16_t, const pid_t, const int32_t)> ("exit", sid, pid, status);
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
		const cluster_t::family_t family = (this->master() ? cluster_t::family_t::MASTER : cluster_t::family_t::CHILDREN);
		// Выполняем тип возникшего события
		switch(static_cast <uint8_t> (event)){
			// Если производится запуск процесса
			case static_cast <uint8_t> (cluster_t::event_t::START): {
				// Определяем члена семейства кластера
				switch(static_cast <uint8_t> (family)){
					// Если процесс является родительским
					case static_cast <uint8_t> (cluster_t::family_t::MASTER): {
						// Если разрешено выводить информационыне уведомления
						if(this->_info)
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
									i->second->addr.fd = shm->_addr.fd;
									// Выполняем установку базы событий
									i->second->base(this->eventBase());
									// Выполняем запуск работы события
									i->second->start();
									// Активируем получение данных с клиента
									i->second->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::READ);
								// Если брокер не существует
								} else {
									/**
									 * Выполняем отлов ошибок
									 */
									try {
										// Выполняем блокировку потока
										this->_mtx.accept.lock();
										// Выполняем создание брокера подключения
										auto ret = this->_brokers.emplace(sid, make_unique <awh::scheme_t::broker_t> (sid, this->_fmk, this->_log));
										// Выполняем блокировку потока
										this->_mtx.accept.unlock();
										// Устанавливаем активный сокет сервера
										ret.first->second->addr.fd = shm->_addr.fd;
										// Выполняем установку базы событий
										ret.first->second->base(this->eventBase());
										// Выполняем установку функции обратного вызова на получении сообщений
										ret.first->second->on <void (const uint64_t)> ("read", static_cast <void (core_t::*)(const SOCKET, const uint16_t)> (&core_t::accept), this, shm->_addr.fd, sid);
										// Выполняем запуск работы события
										ret.first->second->start();
										// Активируем получение данных с клиента
										ret.first->second->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::READ);
									/**
									 * Если возникает ошибка
									 */
									} catch(const bad_alloc &) {
										/**
										 * Если включён режим отладки
										 */
										#if defined(DEBUG_MODE)
											// Выводим сообщение об ошибке
											this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sid, pid, static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL, "Memory allocation error");
										/**
										* Если режим отладки не включён
										*/
										#else
											// Выводим сообщение об ошибке
											this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
										#endif
										// Выходим из приложения
										::exit(EXIT_FAILURE);
									/**
									 * Если возникает ошибка
									 */
									} catch(const exception & error) {
										/**
										 * Если включён режим отладки
										 */
										#if defined(DEBUG_MODE)
											// Выводим сообщение об ошибке
											this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sid, pid, static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL, error.what());
										/**
										* Если режим отладки не включён
										*/
										#else
											// Выводим сообщение об ошибке
											this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
										#endif
									}
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
						// Выполняем остановку работы событий
						i->second->stop();
				}
			} break;
		}
		// Если функция обратного вызова установлена
		if(this->_callback.is("cluster"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const cluster_t::family_t, const uint16_t, const pid_t, const cluster_t::event_t)> ("cluster", family, sid, pid, event);
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
	if(this->_callback.is("message")){
		// Определяем члена семейства кластера
		switch(static_cast <uint8_t> (this->master() ? cluster_t::family_t::MASTER : cluster_t::family_t::CHILDREN)){
			// Если процесс является родительским
			case static_cast <uint8_t> (cluster_t::family_t::MASTER):
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const cluster_t::family_t, const uint16_t, const pid_t, const char *, const size_t)> ("message", cluster_t::family_t::MASTER, sid, pid, buffer, size);
			break;
			// Если процесс является дочерним
			case static_cast <uint8_t> (cluster_t::family_t::CHILDREN):
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const cluster_t::family_t, const uint16_t, const pid_t, const char *, const size_t)> ("message", cluster_t::family_t::CHILDREN, sid, pid, buffer, size);
			break;
		}
	}
}
/**
 * initDTLS Метод инициализации DTLS-брокера
 * @param sid идентификатор схемы сети
 */
void awh::server::Core::initDTLS(const uint16_t sid) noexcept {
	// Если тип сокета установлен как DTLS и идентификатор схемы сети передан
	if((this->_settings.sonet == scheme_t::sonet_t::DTLS) && this->has(sid)){
		// Выполняем поиск идентификатора схемы сети
		auto i = this->_schemes.find(sid);
		// Если идентификатор схемы сети найден, устанавливаем максимальное количество одновременных подключений
		if(i != this->_schemes.end()){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Получаем объект схемы сети
				scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
				// Создаём бъект активного брокера подключения
				unique_ptr <awh::scheme_t::broker_t> broker(new awh::scheme_t::broker_t(sid, this->_fmk, this->_log));
				// Получаем идентификатор брокера подключения
				const uint64_t bid = broker->id();
				// Выполняем установку желаемого протокола подключения
				broker->ectx.proto(this->_settings.proto);
				// Выполняем установку времени ожидания входящих сообщений
				broker->timeouts.wait = shm->timeouts.wait;
				// Устанавливаем таймаут начтение данных из сокета
				broker->timeout(shm->timeouts.read, engine_t::method_t::READ);
				// Устанавливаем таймаут на запись данных в сокет
				broker->timeout(shm->timeouts.write, engine_t::method_t::WRITE);
				// Выполняем блокировку потока
				this->_mtx.accept.lock();
				// Устанавливаем активный сокет сервера
				broker->addr.fd = shm->_addr.fd;
				// Выполняем получение контекста сертификата
				this->_engine.wrap(broker->ectx, &shm->_addr, engine_t::type_t::SERVER);
				// Выполняем установку базы событий
				broker->base(this->eventBase());
				// Добавляем созданного брокера в список брокеров
				auto ret = shm->_brokers.emplace(bid, std::forward <unique_ptr <awh::scheme_t::broker_t>> (broker));
				// Добавляем брокера в список подключений
				node_t::_brokers.emplace(ret.first->first, ret.first->second.get());
				// Выполняем разблокировку потока
				this->_mtx.accept.unlock();
				// Выполняем установку функции обратного вызова на получении сообщений
				ret.first->second->on <void (const uint64_t)> ("read", static_cast <void (core_t::*)(const uint16_t, const uint64_t)> (&core_t::accept), this, sid, _1);
				// Выполняем запуск работы события
				ret.first->second->start();
				// Запускаем событие подключения клиента
				ret.first->second->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::READ);
			/**
			 * Если возникает ошибка
			 */
			} catch(const bad_alloc &) {
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sid), log_t::flag_t::CRITICAL, "Memory allocation error");
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
				#endif
				// Выходим из приложения
				::exit(EXIT_FAILURE);
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sid), log_t::flag_t::CRITICAL, error.what());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			}
		}
	}
}
/**
 * master Метод проверки является ли процесс родительским
 * @return результат проверки
 */
bool awh::server::Core::master() const noexcept {
	// Выводим результат проверки
	return this->_cluster.master();
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
		const lock_guard <recursive_mutex> lock1(this->_mtx.close);
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock2(node_t::_mtx.main);
		const lock_guard <recursive_mutex> lock3(node_t::_mtx.send);
		// Объект работы с функциями обратного вызова
		callback_t callback(this->_log);
		// Переходим по всему списку схем сети
		for(auto & item : this->_schemes){
			// Выполняем удаление таймера проверки наличия клиентов
			this->clearTimeout(item.first);
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
						// Выполняем удаление таймера ожидания получения данных
						this->clearTimeout(i->first);
						// Получаем бъект активного брокера подключения
						awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (i->second.get());
						// Выполняем остановку работы событий
						broker->stop();
						// Выполняем очистку контекста двигателя
						broker->ectx.clear();
						// Удаляем брокера из списка подключений
						node_t::_brokers.erase(i->first);
						// Ещем для указанного брокера очередь полезной нагрузки
						auto j = this->_payloads.find(i->first);
						// Если для брокера очередь полезной нагрузки получена
						if(j != this->_payloads.end())
							// Выполняем удаление всей очереди
							this->_payloads.erase(j);
						// Ищем для указанного брокера список используемой памяти полезной нагрузки
						auto k = this->_available.find(i->first);
						// Если для брокера размер используемой памяти полезной нагрузки получен
						if(k != this->_available.end())
							// Выполняем удаление записи используемой памяти полезной нагрузки
							this->_available.erase(k);
						// Если функция обратного вызова установлена
						if(this->_callback.is("disconnect"))
							// Устанавливаем полученную функцию обратного вызова
							callback.on <void (const uint64_t, const uint16_t)> (i->first, this->_callback.get <void (const uint64_t, const uint16_t)> ("disconnect"), i->first, item.first);
						// Удаляем блокировку брокера
						this->_busy.erase(i->first);
						// Удаляем брокера из списка
						i = shm->_brokers.erase(i);
					// Иначе продолжаем дальше
					} else ++i;
				}
				// Останавливаем работу кластера
				this->_cluster.stop(shm->id);
			}
			// Выполняем закрытие подключение сервера
			shm->_addr.clear();
		}
		// Выполняем все функции обратного вызова
		callback.call();
	}
}
/**
 * remove Метод удаления всех активных схем сети
 */
void awh::server::Core::remove() noexcept {
	// Если список схем сети активен
	if(!this->_schemes.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock1(this->_mtx.close);
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock2(node_t::_mtx.main);
		const lock_guard <recursive_mutex> lock3(node_t::_mtx.send);
		// Если таймер инициализирован удачно
		if(this->_timer != nullptr){
			// Выполняем удаление всех таймеров
			this->_timer->clear();
			// Выполняем анбиндинг сетевого ядра таймера
			this->unbind(dynamic_cast <awh::core_t *> (this->_timer.get()));
			// Удалям активный таймер
			this->_timer.reset(nullptr);
		}
		// Объект работы с функциями обратного вызова
		callback_t callback(this->_log);
		// Переходим по всему списку брокера
		for(auto i = this->_brokers.begin(); i != this->_brokers.end();){
			// Выполняем остановку работы событий
			i->second->stop();
			// Выполняем удаление брокера подключения
			i = this->_brokers.erase(i);
		}
		// Переходим по всему списку схем сети
		for(auto i = this->_schemes.begin(); i != this->_schemes.end();){
			// Выполняем удаление таймера проверки наличия клиентов
			this->clearTimeout(i->first);
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
						// Выполняем удаление таймера ожидания получения данных
						this->clearTimeout(j->first);
						// Получаем бъект активного брокера подключения
						awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (j->second.get());
						// Выполняем остановку работы событий
						broker->stop();
						// Выполняем очистку контекста двигателя
						broker->ectx.clear();
						// Удаляем брокера из списка подключений
						node_t::_brokers.erase(j->first);
						// Ещем для указанного потока очередь полезной нагрузки
						auto k = this->_payloads.find(j->first);
						// Если для потока очередь полезной нагрузки получена
						if(k != this->_payloads.end())
							// Выполняем удаление всей очереди
							this->_payloads.erase(k);
						// Ищем для указанного брокера список используемой памяти полезной нагрузки
						auto l = this->_available.find(j->first);
						// Если для брокера размер используемой памяти полезной нагрузки получен
						if(l != this->_available.end())
							// Выполняем удаление записи используемой памяти полезной нагрузки
							this->_available.erase(l);
						// Если функция обратного вызова установлена
						if(this->_callback.is("disconnect"))
							// Устанавливаем полученную функцию обратного вызова
							callback.on <void (const uint64_t, const uint16_t)> (j->first, this->_callback.get <void (const uint64_t, const uint16_t)> ("disconnect"), j->first, i->first);
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
		callback.call();
	}
}
/**
 * close Метод закрытия подключения брокера
 * @param bid идентификатор брокера
 */
void awh::server::Core::close(const uint64_t bid) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.close);
	// Выполняем удаление таймера ожидания получения данных
	this->clearTimeout(bid);
	// Определяем тип сокета
	switch(static_cast <uint8_t> (this->_settings.sonet)){
		// Если тип сокета установлен как DTLS
		case static_cast <uint8_t> (scheme_t::sonet_t::DTLS): {
			// Если идентификатор брокера подключений существует
			if(this->has(bid))
				// Выполняем закрытие подключения
				this->close(this->broker(bid)->sid(), bid);
		} break;
		// Если тип сокета установлен как TCP/IP
		case static_cast <uint8_t> (scheme_t::sonet_t::TCP):
		// Если тип сокета установлен как TCP/IP TLS
		case static_cast <uint8_t> (scheme_t::sonet_t::TLS):
		// Если тип сокета установлен как SCTP
		case static_cast <uint8_t> (scheme_t::sonet_t::SCTP): {
			// Если блокировка брокера не установлена
			if(this->_busy.find(bid) == this->_busy.end()){
				// Выполняем блокировку брокера
				this->_busy.emplace(bid);
				// Объект работы с функциями обратного вызова
				callback_t callback(this->_log);
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
						// Выполняем остановку работы событий
						broker->stop();
						// Выполняем очистку контекста двигателя
						broker->ectx.clear();
						// Выполняем удаление параметров активного брокера
						node_t::remove(bid);
						// Если разрешено выводить информационыне уведомления
						if(this->_info)
							// Выводим информацию об удачном отключении от сервера
							this->_log->print("Disconnect client from server", log_t::flag_t::INFO);
						// Если функция обратного вызова установлена
						if(this->_callback.is("disconnect"))
							// Устанавливаем полученную функцию обратного вызова
							callback.on <void (const uint64_t, const uint16_t)> (bid, this->_callback.get <void (const uint64_t, const uint16_t)> ("disconnect"), bid, shm->id);
					}
				}
				// Удаляем блокировку брокера
				this->_busy.erase(bid);
				// Если функция обратного вызова установлена
				if(callback.is(bid))
					// Выполняем все функции обратного вызова
					callback.call <void (void)> (bid);
			}
		} break;
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
		const lock_guard <recursive_mutex> lock1(this->_mtx.close);
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock2(node_t::_mtx.main);
		const lock_guard <recursive_mutex> lock3(node_t::_mtx.send);
		// Выполняем поиск идентификатора схемы сети
		auto i = this->_schemes.find(sid);
		// Если идентификатор схемы сети найден, устанавливаем максимальное количество одновременных подключений
		if(i != this->_schemes.end()){
			// Объект работы с функциями обратного вызова
			callback_t callback(this->_log);
			// Выполняем удаление таймера проверки наличия клиентов
			this->clearTimeout(i->first);
			// Выполняем поиск активного брокера ожидания подключения
			auto j = this->_brokers.find(i->first);
			// Если брокерактивного подключения получен
			if(j != this->_brokers.end()){
				// Выполняем остановку работы событий
				j->second->stop();
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
						// Выполняем удаление таймера ожидания получения данных
						this->clearTimeout(j->first);
						// Получаем бъект активного брокера подключения
						awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (j->second.get());
						// Выполняем остановку работы событий
						broker->stop();
						// Выполняем очистку контекста двигателя
						broker->ectx.clear();
						// Удаляем брокера из списка подключений
						node_t::_brokers.erase(j->first);
						// Ещем для указанного потока очередь полезной нагрузки
						auto k = this->_payloads.find(j->first);
						// Если для потока очередь полезной нагрузки получена
						if(k != this->_payloads.end())
							// Выполняем удаление всей очереди
							this->_payloads.erase(k);
						// Ищем для указанного брокера список используемой памяти полезной нагрузки
						auto l = this->_available.find(j->first);
						// Если для брокера размер используемой памяти полезной нагрузки получен
						if(l != this->_available.end())
							// Выполняем удаление записи используемой памяти полезной нагрузки
							this->_available.erase(l);
						// Если функция обратного вызова установлена
						if(this->_callback.is("disconnect"))
							// Устанавливаем полученную функцию обратного вызова
							callback.on <void (const uint64_t, const uint16_t)> (j->first, this->_callback.get <void (const uint64_t, const uint16_t)> ("disconnect"), j->first, i->first);
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
			callback.call();
		}
	}
}
/**
 * close Метод закрытия подключения брокера по протоколу UDP
 * @param sid идентификатор схемы сети
 * @param bid идентификатор брокера
 */
void awh::server::Core::close(const uint16_t sid, const uint64_t bid) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.close);
	// Выполняем удаление таймера ожидания получения данных
	this->clearTimeout(bid);
	// Определяем тип сокета
	switch(static_cast <uint8_t> (this->_settings.sonet)){
		// Если тип сокета установлен как UDP
		case static_cast <uint8_t> (scheme_t::sonet_t::UDP): {
			// Если блокировка брокера не установлена
			if(this->_busy.find(bid) == this->_busy.end()){
				// Выполняем блокировку брокера
				this->_busy.emplace(bid);
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
						// Выполняем остановку работы событий
						broker->stop();
						// Выполняем очистку контекста двигателя
						broker->ectx.clear();
						// Выполняем удаление параметров активного брокера
						node_t::remove(bid);
						// Если разрешено выводить информационыне уведомления
						if(this->_info)
							// Выводим информацию об удачном отключении от сервера
							this->_log->print("Disconnect server", log_t::flag_t::INFO);
					}
				}
				// Удаляем блокировку брокера
				this->_busy.erase(bid);
				// Выполняем активацию сервера
				this->accept(1, sid);
			}
		} break;
		// Если тип сокета установлен как DTLS
		case static_cast <uint8_t> (scheme_t::sonet_t::DTLS): {
			// Если блокировка брокера не установлена
			if(this->_busy.find(bid) == this->_busy.end()){
				// Выполняем блокировку брокера
				this->_busy.emplace(bid);
				// Если идентификатор брокера подключений существует
				if(this->has(bid)){
					// Создаём бъект активного брокера подключения
					awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (this->broker(bid));
					// Выполняем поиск идентификатора схемы сети
					auto i = this->_schemes.find(broker->sid());
					// Если идентификатор схемы сети найден
					if(i != this->_schemes.end()){
						// Выполняем удаление таймера проверки наличия клиентов
						this->clearTimeout(i->first);
						// Получаем объект схемы сети
						scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
						// Выполняем остановку работы событий
						broker->stop();
						// Выполняем очистку контекста двигателя
						broker->ectx.clear();
						// Выполняем удаление параметров активного брокера
						node_t::remove(bid);
						// Если разрешено выводить информационыне уведомления
						if(this->_info)
							// Выводим информацию об удачном отключении от сервера
							this->_log->print("Disconnect client from server", log_t::flag_t::INFO);
						// Если функция обратного вызова установлена
						if(this->_callback.is("disconnect"))
							// Устанавливаем полученную функцию обратного вызова
							this->_callback.call <void (const uint64_t, const uint16_t)> ("disconnect", bid, sid);
						// Выполняем удаление старого подключения
						shm->_addr.clear();
						// Если сокет подключения получен
						if(this->create(sid))
							// Выполняем создание нового DTLS-брокера
							this->initDTLS(sid);
						// Если сокет не создан, выводим в консоль информацию
						else {
							// Если unix-сокет используется
							if(this->_settings.family == scheme_t::family_t::IPC){
								// Выводим информацию об незапущенном сервере на unix-сокете
								this->_log->print("Server [%s/%s.sock] cannot be started", log_t::flag_t::CRITICAL, this->_settings.sockpath.c_str(), this->_settings.sockname.c_str());
								// Если функция обратного вызова установлена
								if(this->_callback.is("error"))
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::START, this->_fmk->format("Server [%s/%s.sock] cannot be started", this->_settings.sockpath.c_str(), this->_settings.sockname.c_str()));
							// Если используется хост и порт
							} else {
								// Выводим сообщение об незапущенном сервере за порту
								this->_log->print("Server [%s:%u] cannot be started", log_t::flag_t::CRITICAL, shm->_host.c_str(), shm->_port);
								// Если функция обратного вызова установлена
								if(this->_callback.is("error"))
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::START, this->_fmk->format("Server [%s:%u] cannot be started", shm->_host.c_str(), shm->_port));
							}
						}
						// Удаляем блокировку брокера
						this->_busy.erase(bid);
					}
				}
			}
		} break;
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
					case static_cast <uint8_t> (scheme_t::family_t::IPC):
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
					case static_cast <uint8_t> (net_t::type_t::FQDN): {
						// Если объект DNS-резолвера установлен
						if(this->_dns != nullptr) {
							// Определяем тип протокола подключения
							switch(static_cast <uint8_t> (this->_settings.family)){
								// Если тип протокола подключения unix-сокет
								case static_cast <uint8_t> (scheme_t::family_t::IPC):
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
			if(this->_settings.family == scheme_t::family_t::IPC){
				// Если название unix-сокета ещё не инициализированно
				if(this->_settings.sockname.empty())
					// Выполняем установку названия unix-сокета
					this->sockname();
				// Выполняем инициализацию сокета
				shm->_addr.init(this->_fmk->format("%s/%s.sock", this->_settings.sockpath.c_str(), this->_settings.sockname.c_str()), engine_t::type_t::SERVER);
			// Если unix-сокет не используется, выполняем инициализацию сокета
			} else shm->_addr.init(shm->_host, shm->_port, (this->_settings.family == scheme_t::family_t::IPV6 ? AF_INET6 : AF_INET), engine_t::type_t::SERVER, this->_settings.ipV6only);
			// Если сокет подключения получен
			if((shm->_addr.fd != INVALID_SOCKET) && (shm->_addr.fd < AWH_MAX_SOCKETS))
				// Выполняем прослушивание порта
				result = static_cast <bool> (shm->_addr.list());
		}
	}
	// Выводим результат создания сервера
	return result;
}
/**
 * host Метод получения хоста сервера
 * @param sid идентификатор схемы сети
 * @return    хост на котором висит сервер
 */
string awh::server::Core::host(const uint16_t sid) const noexcept {
	// Определяем тип протокола подключения
	switch(static_cast <uint8_t> (this->_settings.family)){
		// Если тип протокола подключения unix-сокет
		case static_cast <uint8_t> (scheme_t::family_t::IPC):
			// Выводим название unix-сокета
			return this->_fmk->format("%s/%s.sock", this->_settings.sockpath.c_str(), this->_settings.sockname.c_str());
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
	return "";
}
/**
 * port Метод получения порта сервера
 * @param sid идентификатор схемы сети
 * @return    порт сервера который он прослушивает
 */
uint32_t awh::server::Core::port(const uint16_t sid) const noexcept {
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
 * send Метод асинхронной отправки буфера данных в сокет
 * @param buffer буфер для записи данных
 * @param size   размер записываемых данных
 * @param bid    идентификатор брокера
 * @return       результат отправки сообщения
 */
bool awh::server::Core::send(const char * buffer, const size_t size, const uint64_t bid) noexcept {
	// Результат работы функции
	bool result = false;
	// Если данные переданы
	if(this->working() && this->has(bid)){
		// Флаг активация ожидания готовности сокета
		bool waiting = false;
		// Определяем режим отправки сообщений
		switch(static_cast <uint8_t> (this->_sending)){
			// Если установлен флаг отправки отложенных сообщений
			case static_cast <uint8_t> (sending_t::DEFFER):
				// Выполняем отправку сообщения асинхронным методом
				waiting = result = node_t::send(buffer, size, bid);
			break;
			// Если установлен флаг отправки мгновенных сообщений
			case static_cast <uint8_t> (sending_t::INSTANT): {
				// Ещем для указанного потока очередь полезной нагрузки
				auto i = this->_payloads.find(bid);
				// Если для потока очередь полезной нагрузки получена
				if((waiting = ((i != this->_payloads.end()) && !i->second->empty())))
					// Выполняем отправку сообщения асинхронным методом
					result = node_t::send(buffer, size, bid);
				// Если очередь ещё не существует
				else {
					// Выполняем отправку данных клиенту
					const size_t bytes = this->write(buffer, size, bid);
					// Если данные отправлены не полностью
					if((waiting = (bytes < size)))
						// Выполняем отправку сообщения асинхронным методом
						result = node_t::send(buffer + bytes, size - bytes, bid);
					// Если все данные добавлены успешно
					else result = (bytes == size);
				}
			} break;
		}
		// Если необходимо активировать ожидание готовности сокета для записи
		if(waiting && this->has(bid)){
			// Создаём бъект активного брокера подключения
			awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (this->broker(bid));
			// Если сокет подключения активен
			if((broker->addr.fd != INVALID_SOCKET) && (broker->addr.fd < AWH_MAX_SOCKETS))
				// Запускаем ожидание записи данных
				broker->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::WRITE);
		}
	}
	// Сообщаем, что отправить сообщение неудалось
	return result;
}
/**
 * send Метод отправки сообщения родительскому процессу
 * @param wid идентификатор воркера
 */
void awh::server::Core::send(const uint16_t wid) noexcept {
	// Определяем члена семейства кластера
	switch(static_cast <uint8_t> (this->master() ? cluster_t::family_t::MASTER : cluster_t::family_t::CHILDREN)){
		// Если процесс является родительским
		case static_cast <uint8_t> (cluster_t::family_t::MASTER): {
			// Выводим сообщение в лог, потому что вещание доступно только из родительского процесса
			this->_log->print("Send message is only available from children process", log_t::flag_t::WARNING);
			// Если функция обратного вызова установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::CLUSTER, "Send message is only available from children process");
		} break;
		// Если процесс является дочерним
		case static_cast <uint8_t> (cluster_t::family_t::CHILDREN):
			// Выполняем отправку сообщения родительскому процессу
			this->_cluster.send(wid);
		break;
	}
}
/**
 * send Метод отправки сообщения родительскому процессу
 * @param wid    идентификатор воркера
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 */
void awh::server::Core::send(const uint16_t wid, const char * buffer, const size_t size) noexcept {
	// Определяем члена семейства кластера
	switch(static_cast <uint8_t> (this->master() ? cluster_t::family_t::MASTER : cluster_t::family_t::CHILDREN)){
		// Если процесс является родительским
		case static_cast <uint8_t> (cluster_t::family_t::MASTER): {
			// Выводим сообщение в лог, потому что вещание доступно только из родительского процесса
			this->_log->print("Send message is only available from children process", log_t::flag_t::WARNING);
			// Если функция обратного вызова установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::CLUSTER, "Send message is only available from children process");
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
 * @param wid идентификатор воркера
 * @param pid идентификатор процесса для получения сообщения
 */
void awh::server::Core::send(const uint16_t wid, const pid_t pid) noexcept {
	// Определяем члена семейства кластера
	switch(static_cast <uint8_t> (this->master() ? cluster_t::family_t::MASTER : cluster_t::family_t::CHILDREN)){
		// Если процесс является родительским
		case static_cast <uint8_t> (cluster_t::family_t::MASTER):
			// Выполняем отправку сообщения дочернему процессу
			this->_cluster.send(wid, pid);
		break;
		// Если процесс является дочерним
		case static_cast <uint8_t> (cluster_t::family_t::CHILDREN): {
			// Выводим сообщение в лог, потому что вещание доступно только из родительского процесса
			this->_log->print("Send message is only available from master process", log_t::flag_t::WARNING);
			// Если функция обратного вызова установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::CLUSTER, "Send message is only available from master process");
		} break;
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
	switch(static_cast <uint8_t> (this->master() ? cluster_t::family_t::MASTER : cluster_t::family_t::CHILDREN)){
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
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::CLUSTER, "Send message is only available from master process");
		} break;
	}
}
/**
 * broadcast Метод отправки сообщения всем дочерним процессам
 * @param wid идентификатор воркера
 */
void awh::server::Core::broadcast(const uint16_t wid) noexcept {
	// Определяем члена семейства кластера
	switch(static_cast <uint8_t> (this->master() ? cluster_t::family_t::MASTER : cluster_t::family_t::CHILDREN)){
		// Если процесс является родительским
		case static_cast <uint8_t> (cluster_t::family_t::MASTER):
			// Выполняем отправку сообщения всем дочерним процессам
			this->_cluster.broadcast(wid);
		break;
		// Если процесс является дочерним
		case static_cast <uint8_t> (cluster_t::family_t::CHILDREN): {
			// Выводим сообщение в лог, потому что вещание доступно только из родительского процесса
			this->_log->print("Broadcast message is only available from master process", log_t::flag_t::WARNING);
			// Если функция обратного вызова установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::CLUSTER, "Broadcast is only available from master process");
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
	switch(static_cast <uint8_t> (this->master() ? cluster_t::family_t::MASTER : cluster_t::family_t::CHILDREN)){
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
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::CLUSTER, "Broadcast is only available from master process");
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
		if((broker->addr.fd != INVALID_SOCKET) && (broker->addr.fd < AWH_MAX_SOCKETS)){
			// Выполняем поиск идентификатора схемы сети
			auto i = this->_schemes.find(broker->sid());
			// Если идентификатор схемы сети найден
			if(i != this->_schemes.end()){
				// Выполняем отключение приёма данных на этот сокет
				broker->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::READ);
				// Определяем тип сокета
				switch(static_cast <uint8_t> (this->_settings.sonet)){
					// Если тип сокета установлен как TCP/IP
					case static_cast <uint8_t> (scheme_t::sonet_t::TCP):
					// Если тип сокета установлен как TCP/IP TLS
					case static_cast <uint8_t> (scheme_t::sonet_t::TLS):
					// Если тип сокета установлен как SCTP
					case static_cast <uint8_t> (scheme_t::sonet_t::SCTP):
						// Переводим сокет в неблокирующий режим
						broker->ectx.blocking(engine_t::mode_t::DISABLED);
					break;
					// Если тип сокета установлен как UDP
					case static_cast <uint8_t> (scheme_t::sonet_t::UDP):
					// Если тип сокета установлен как DTLS
					case static_cast <uint8_t> (scheme_t::sonet_t::DTLS):
						// Переводим сокет в блокирующий режим
						broker->ectx.blocking(engine_t::mode_t::ENABLED);
					break;
				}
				/**
				 * Выполняем чтение данных с сокета
				 */
				do {
					// Если подключение выполнено и чтение данных разрешено
					if(broker->buffer.size > 0){
						// Определяем тип сокета
						switch(static_cast <uint8_t> (this->_settings.sonet)){
							// Если тип сокета установлен как DTLS
							case static_cast <uint8_t> (scheme_t::sonet_t::DTLS):
								// Выполняем установку таймаута ожидания чтения из сокета
								broker->ectx.timeout(static_cast <uint32_t> (broker->timeouts.read) * 1000, engine_t::method_t::READ);
							break;
						}
						// Выполняем получение сообщения от клиента
						const int64_t bytes = broker->ectx.read(broker->buffer.data.get(), broker->buffer.size);
						// Если данные получены
						if(bytes > 0){
							// Если таймер ожидания получения данных установлен
							if((broker->timeouts.wait > 0) && (this->_settings.sonet != scheme_t::sonet_t::DTLS))
								// Выполняем удаление таймаута
								this->clearTimeout(bid);
							// Если данных достаточно и функция обратного вызова на получение данных установлена
							if(this->_callback.is("read"))
								// Выводим функцию обратного вызова
								this->_callback.call <void (const char *, const size_t, const uint64_t, const uint16_t)> ("read", broker->buffer.data.get(), static_cast <size_t> (bytes), bid, i->first);
							// Если тип сокета установлен как UDP
							if(this->_settings.sonet == scheme_t::sonet_t::DTLS){
								// Если подключение ещё не разорванно
								if(this->has(bid)){
									// Если таймер ожидания получения данных установлен
									if(broker->timeouts.read > 0)
										// Выполняем создание таймаута ожидания получения данных
										this->createTimeout(i->first, bid, static_cast <uint32_t> (broker->timeouts.read) * 1000, mode_t::RECEIVE);
								}
								// Выходим из цикла
								break;
							}
						// Если данные небыли получены
						} else if(bytes <= 0) {
							// Если чтение не выполнена, закрываем подключение
							if(bytes == 0)
								// Выполняем закрытие подключения
								this->close(bid);
							// Выходим из цикла
							break;
						}
					// Если запись не выполнена, входим
					} else break;
				// Выполняем чтение до тех пор, пока всё не прочитаем
				} while(this->has(bid));
				// Если подключение ещё не разорванно
				if(this->has(bid)){
					// Определяем тип сокета
					switch(static_cast <uint8_t> (this->_settings.sonet)){
						// Если тип сокета установлен как UDP
						case static_cast <uint8_t> (scheme_t::sonet_t::UDP):
						// Если тип сокета установлен как DTLS
						case static_cast <uint8_t> (scheme_t::sonet_t::DTLS):
							// Переводим сокет в неблокирующий режим
							broker->ectx.blocking(engine_t::mode_t::DISABLED);
						break;
					}
					// Если время ожиданий входящих сообщений установлено
					if((broker->timeouts.wait > 0) && (this->_settings.sonet != scheme_t::sonet_t::DTLS))
						// Выполняем создание таймаута ожидания получения данных
						this->createTimeout(i->first, bid, static_cast <uint32_t> (broker->timeouts.wait) * 1000, mode_t::RECEIVE);
					// Выполняем активацию отслеживания получения данных с этого сокета
					broker->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::READ);
				}
			// Если схема сети не существует
			} else {
				// Выводим сообщение в лог, о таймауте подключения
				this->_log->print("Connection Broker %llu does not belong to a non-existent network diagram", log_t::flag_t::CRITICAL, bid);
				// Если функция обратного вызова установлена
				if(this->_callback.is("error"))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::PROTOCOL, this->_fmk->format("Connection Broker %llu does not belong to a non-existent network diagram", bid));
			}
		// Если файловый дескриптор сломан, значит с памятью что-то не то
		} else if(broker->addr.fd > AWH_MAX_SOCKETS) {
			// Удаляем из памяти объект брокера
			node_t::remove(bid);
			// Выводим в лог сообщение
			this->_log->print("Socket for read is not initialized", log_t::flag_t::WARNING);
			// Если функция обратного вызова установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::PROTOCOL, "Socket for read is not initialized");
		}
	}
}
/**
 * write Метод записи данных в брокер
 * @param bid идентификатор брокера
 */
void awh::server::Core::write(const uint64_t bid) noexcept {
	// Если данные переданы
	if(this->working() && this->has(bid)){
		// Создаём бъект активного брокера подключения
		awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (this->broker(bid));
		// Если брокер подключения получен
		if(broker != nullptr){
			// Останавливаем детектирования возможности записи в сокет
			broker->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::WRITE);
			// Ещем для указанного потока очередь полезной нагрузки
			auto i = this->_payloads.find(bid);
			// Если для потока очередь полезной нагрузки получена
			if((i != this->_payloads.end()) && !i->second->empty()){
				// Выполняем запись в сокет
				const size_t bytes = this->write(reinterpret_cast <const char *> (i->second->get()), i->second->size(), bid);
				// Если данные записаны удачно
				if((bytes > 0) && this->has(bid))
					// Выполняем освобождение памяти хранения полезной нагрузки
					this->erase(bid, bytes);
				// Если опередей полезной нагрузки нет, отключаем событие ожидания записи
				if(this->_payloads.find(bid) != this->_payloads.end()){
					// Если сокет подключения активен
					if((broker->addr.fd != INVALID_SOCKET) && (broker->addr.fd < AWH_MAX_SOCKETS))
						// Запускаем ожидание записи данных
						broker->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::WRITE);
				}
			}
		}
	}
}
/**
 * write Метод записи буфера данных в сокет
 * @param buffer буфер для записи данных
 * @param size   размер записываемых данных
 * @param bid    идентификатор брокера
 * @return       количество отправленных байт
 */
size_t awh::server::Core::write(const char * buffer, const size_t size, const uint64_t bid) noexcept {
	// Результат работы функции
	size_t result = 0;
	// Если данные переданы
	if(this->working() && this->has(bid) && (buffer != nullptr) && (size > 0)){
		// Создаём бъект активного брокера подключения
		awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (this->broker(bid));
		// Если сокет подключения активен
		if((broker->addr.fd != INVALID_SOCKET) && (broker->addr.fd < AWH_MAX_SOCKETS)){
			// Останавливаем детектирования возможности записи в сокет
			broker->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::WRITE);
			// Выполняем поиск идентификатора схемы сети
			auto i = this->_schemes.find(broker->sid());
			// Если идентификатор схемы сети найден
			if(i != this->_schemes.end()){
				// Получаем максимальный размер буфера
				const int32_t max = broker->ectx.buffer(engine_t::method_t::WRITE);
				// Если в буфере нет места
				if(max > 0){
					// Определяем правило передачи данных
					switch(static_cast <uint8_t> (this->_transfer)){
						// Если передавать данные необходимо синхронно
						case static_cast <uint8_t> (transfer_t::SYNC): {
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
								// Если тип сокета установлен как DTLS
								case static_cast <uint8_t> (scheme_t::sonet_t::DTLS):
									// Переводим сокет в блокирующий режим
									broker->ectx.blocking(engine_t::mode_t::ENABLED);
								break;
							}
						} break;
						// Если передавать данные необходимо асинхронно
						case static_cast <uint8_t> (transfer_t::ASYNC): {
							// Определяем тип сокета
							switch(static_cast <uint8_t> (this->_settings.sonet)){
								// Если тип сокета установлен как UDP
								case static_cast <uint8_t> (scheme_t::sonet_t::UDP):
								// Если тип сокета установлен как DTLS
								case static_cast <uint8_t> (scheme_t::sonet_t::DTLS):
									// Переводим сокет в блокирующий режим
									broker->ectx.blocking(engine_t::mode_t::ENABLED);
								break;
							}
						} break;
					}
					// Определяем тип сокета
					switch(static_cast <uint8_t> (this->_settings.sonet)){
						// Если тип сокета установлен как TCP/IP
						case static_cast <uint8_t> (scheme_t::sonet_t::TCP):
						// Если тип сокета установлен как TCP/IP TLS
						case static_cast <uint8_t> (scheme_t::sonet_t::TLS):
						// Если тип сокета установлен как DTLS
						case static_cast <uint8_t> (scheme_t::sonet_t::DTLS):
						// Если тип сокета установлен как SCTP
						case static_cast <uint8_t> (scheme_t::sonet_t::SCTP):
							// Выполняем установку таймаута ожидания записи в сокет
							broker->ectx.timeout(static_cast <uint32_t> (broker->timeouts.write) * 1000, engine_t::method_t::WRITE);
						break;
					}
					// Выполняем отправку сообщения клиенту
					const int64_t bytes = broker->ectx.write(buffer, (size >= static_cast <size_t> (max) ? static_cast <size_t> (max) : size));
					// Если данные удачно отправленны
					if(bytes > 0)
						// Запоминаем количество записанных байт
						result = static_cast <size_t> (bytes);
					// Если запись не выполнена, закрываем подключение
					else if(bytes == 0)
						// Выполняем закрытие подключения
						this->close(bid);
					// Если дисконнекта не произошло
					if(bytes != 0){
						// Определяем правило передачи данных
						switch(static_cast <uint8_t> (this->_transfer)){
							// Если передавать данные необходимо синхронно
							case static_cast <uint8_t> (transfer_t::SYNC): {
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
									// Если тип сокета установлен как DTLS
									case static_cast <uint8_t> (scheme_t::sonet_t::DTLS):
										// Переводим сокет в неблокирующий режим
										broker->ectx.blocking(engine_t::mode_t::DISABLED);
									break;
								}
							} break;
							// Если передавать данные необходимо асинхронно
							case static_cast <uint8_t> (transfer_t::ASYNC): {
								// Определяем тип сокета
								switch(static_cast <uint8_t> (this->_settings.sonet)){
									// Если тип сокета установлен как UDP
									case static_cast <uint8_t> (scheme_t::sonet_t::UDP):
									// Если тип сокета установлен как DTLS
									case static_cast <uint8_t> (scheme_t::sonet_t::DTLS):
										// Переводим сокет в неблокирующий режим
										broker->ectx.blocking(engine_t::mode_t::DISABLED);
									break;
								}
							} break;
						}
					}
					// Если данные отправлены удачно и функция обратного вызова установлена
					if((bytes > 0) && this->_callback.is("write"))
						// Выводим функцию обратного вызова
						this->_callback.call <void (const char *, const size_t, const uint64_t, const uint16_t)> ("write", buffer, static_cast <size_t> (bytes), bid, i->first);
				}
			// Если схема сети не существует
			} else {
				// Выводим сообщение в лог, о таймауте подключения
				this->_log->print("Connection Broker %llu does not belong to a non-existent network diagram", log_t::flag_t::CRITICAL, bid);
				// Если функция обратного вызова установлена
				if(this->_callback.is("error"))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::PROTOCOL, this->_fmk->format("Connection Broker %llu does not belong to a non-existent network diagram", bid));
			}
		// Если файловый дескриптор сломан, значит с памятью что-то не то
		} else if(broker->addr.fd > AWH_MAX_SOCKETS) {
			// Удаляем из памяти объект брокера
			node_t::remove(bid);
			// Выводим в лог сообщение
			this->_log->print("Socket for write is not initialized", log_t::flag_t::WARNING);
			// Если функция обратного вызова установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::PROTOCOL, "Socket for write is not initialized");
		}
	}
	// Выводим результат
	return result;
}
/**
 * work Метод активации параметров запуска сервера
 * @param sid    идентификатор схемы сети
 * @param ip     адрес интернет-подключения
 * @param family тип интернет-протокола AF_INET, AF_INET6
 */
void awh::server::Core::work(const uint16_t sid, const string & ip, const int32_t family) noexcept {
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
						if(this->_info){
							// Если unix-сокет используется
							if(this->_settings.family == scheme_t::family_t::IPC)
								// Выводим информацию о запущенном сервере на unix-сокете
								this->_log->print("Server [%s/%s.sock] has been started successfully", log_t::flag_t::INFO, this->_settings.sockpath.c_str(), this->_settings.sockname.c_str());
							// Если unix-сокет не используется, выводим сообщение о запущенном сервере за порту
							else this->_log->print("Server [%s:%u] has been started successfully", log_t::flag_t::INFO, shm->_host.c_str(), shm->_port);
						}
						// Определяем режим активации кластера
						switch(static_cast <uint8_t> (this->_clusterMode)){
							// Если кластер необходимо активировать
							case static_cast <uint8_t> (awh::scheme_t::mode_t::ENABLED): {
								// Выводим в консоль информацию
								this->_log->print("Working in cluster mode for \"UDP-protocol\" is not supported PID=%d", log_t::flag_t::WARNING, ::getpid());
								// Если функция обратного вызова установлена
								if(this->_callback.is("error"))
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::START, this->_fmk->format("Working in cluster mode for \"UDP-protocol\" is not supported PID=%d", ::getpid()));
							} break;
						}
						// Если функция обратного вызова установлена
						if(this->_callback.is("launched")){
							// Если unix-сокет используется
							if(this->_settings.family == scheme_t::family_t::IPC)
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const string &, const uint32_t)> ("launched", this->_fmk->format("%s/%s.sock", this->_settings.sockpath.c_str(), this->_settings.sockname.c_str()), 0);
							// Выполняем функцию обратного вызова
							else this->_callback.call <void (const string &, const uint32_t)> ("launched", shm->_host, shm->_port);
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
							if(this->_info){
								// Если unix-сокет используется
								if(this->_settings.family == scheme_t::family_t::IPC)
									// Выводим информацию о запущенном сервере на unix-сокете
									this->_log->print("Server [%s/%s.sock] has been started successfully", log_t::flag_t::INFO, this->_settings.sockpath.c_str(), this->_settings.sockname.c_str());
								// Если unix-сокет не используется, выводим сообщение о запущенном сервере за порту
								else this->_log->print("Server [%s:%u] has been started successfully", log_t::flag_t::INFO, shm->_host.c_str(), shm->_port);
							}
							// Определяем режим активации кластера
							switch(static_cast <uint8_t> (this->_clusterMode)){
								// Если кластер необходимо активировать
								case static_cast <uint8_t> (awh::scheme_t::mode_t::ENABLED): {
									// Выводим в консоль информацию
									this->_log->print("Working in cluster mode for \"DTLS-protocol\" is not supported PID=%d", log_t::flag_t::WARNING, ::getpid());
									// Если функция обратного вызова установлена
									if(this->_callback.is("error"))
										// Выполняем функцию обратного вызова
										this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::START, this->_fmk->format("Working in cluster mode for \"DTLS-protocol\" is not supported PID=%d", ::getpid()));
								} break;
							}
							// Если функция обратного вызова установлена
							if(this->_callback.is("launched")){
								// Если unix-сокет используется
								if(this->_settings.family == scheme_t::family_t::IPC)
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const string &, const uint32_t)> ("launched", this->_fmk->format("%s/%s.sock", this->_settings.sockpath.c_str(), this->_settings.sockname.c_str()), 0);
								// Выполняем функцию обратного вызова
								else this->_callback.call <void (const string &, const uint32_t)> ("launched", shm->_host, shm->_port);
							}
							// Выполняем создание нового DTLS-брокера
							this->initDTLS(sid);
							// Выходим из функции
							return;
						// Если сокет не создан, выводим в консоль информацию
						} else {
							// Если unix-сокет используется
							if(this->_settings.family == scheme_t::family_t::IPC){
								// Выводим информацию об незапущенном сервере на unix-сокете
								this->_log->print("Server [%s/%s.sock] has been started successfully", log_t::flag_t::CRITICAL, this->_settings.sockpath.c_str(), this->_settings.sockname.c_str());
								// Если функция обратного вызова установлена
								if(this->_callback.is("error"))
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::START, this->_fmk->format("Server cannot be started [%s/%s.sock]", this->_settings.sockpath.c_str(), this->_settings.sockname.c_str()));
							// Если используется хост и порт
							} else {
								// Выводим сообщение об незапущенном сервере за порту
								this->_log->print("Server [%s:%u] has been started successfully", log_t::flag_t::CRITICAL, shm->_host.c_str(), shm->_port);
								// Если функция обратного вызова установлена
								if(this->_callback.is("error"))
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::START, this->_fmk->format("Server cannot be started [%s:%u]", shm->_host.c_str(), shm->_port));
							}
						}
					} break;
					// Для всех остальных типов сокетов
					default: {
						// Если сокет подключения получен
						if(this->create(sid)){
							// Если разрешено выводить информационные сообщения
							if(this->_info){
								// Если unix-сокет используется
								if(this->_settings.family == scheme_t::family_t::IPC)
									// Выводим информацию о запущенном сервере на unix-сокете
									this->_log->print("Server [%s/%s.sock] has been started successfully", log_t::flag_t::INFO, this->_settings.sockpath.c_str(), this->_settings.sockname.c_str());
								// Если unix-сокет не используется, выводим сообщение о запущенном сервере за порту
								else this->_log->print("Server [%s:%u] has been started successfully", log_t::flag_t::INFO, shm->_host.c_str(), shm->_port);
							}
							// Если функция обратного вызова установлена
							if(this->_callback.is("launched")){
								// Если unix-сокет используется
								if(this->_settings.family == scheme_t::family_t::IPC)
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const string &, const uint32_t)> ("launched", this->_fmk->format("%s/%s.sock", this->_settings.sockpath.c_str(), this->_settings.sockname.c_str()), 0);
								// Выполняем функцию обратного вызова
								else this->_callback.call <void (const string &, const uint32_t)> ("launched", shm->_host, shm->_port);
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
										i->second->addr.fd = shm->_addr.fd;
										// Выполняем установку базы событий
										i->second->base(this->eventBase());
										// Выполняем запуск работы события
										i->second->start();
										// Активируем получение данных с клиента
										i->second->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::READ);
									// Если брокер не существует
									} else {
										/**
										 * Выполняем отлов ошибок
										 */
										try {
											// Выполняем блокировку потока
											this->_mtx.accept.lock();
											// Выполняем создание брокера подключения
											auto ret = this->_brokers.emplace(sid, make_unique <awh::scheme_t::broker_t> (sid, this->_fmk, this->_log));
											// Выполняем блокировку потока
											this->_mtx.accept.unlock();
											// Устанавливаем активный сокет сервера
											ret.first->second->addr.fd = shm->_addr.fd;
											// Выполняем установку базы событий
											ret.first->second->base(this->eventBase());
											// Выполняем установку функции обратного вызова на получении сообщений
											ret.first->second->on <void (const uint64_t)> ("read", static_cast <void (core_t::*)(const SOCKET, const uint16_t)> (&core_t::accept), this, shm->_addr.fd, sid);
											// Выполняем запуск работы события
											ret.first->second->start();
											// Активируем получение данных с клиента
											ret.first->second->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::READ);
										/**
										 * Если возникает ошибка
										 */
										} catch(const bad_alloc &) {
											/**
											 * Если включён режим отладки
											 */
											#if defined(DEBUG_MODE)
												// Выводим сообщение об ошибке
												this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sid, ip, family), log_t::flag_t::CRITICAL, "Memory allocation error");
											/**
											* Если режим отладки не включён
											*/
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
											#endif
											// Выходим из приложения
											::exit(EXIT_FAILURE);
										/**
										 * Если возникает ошибка
										 */
										} catch(const exception & error) {
											/**
											 * Если включён режим отладки
											 */
											#if defined(DEBUG_MODE)
												// Выводим сообщение об ошибке
												this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sid, ip, family), log_t::flag_t::CRITICAL, error.what());
											/**
											* Если режим отладки не включён
											*/
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
											#endif
										}
									}
								} break;
							}
							// Выходим из функции
							return;
						// Если сокет не создан, выводим в консоль информацию
						} else {
							// Если unix-сокет используется
							if(this->_settings.family == scheme_t::family_t::IPC){
								// Выводим информацию об незапущенном сервере на unix-сокете
								this->_log->print("Server [%s/%s.sock] has been started successfully", log_t::flag_t::CRITICAL, this->_settings.sockpath.c_str(), this->_settings.sockname.c_str());
								// Если функция обратного вызова установлена
								if(this->_callback.is("error"))
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::START, this->_fmk->format("Server cannot be started [%s/%s.sock]", this->_settings.sockpath.c_str(), this->_settings.sockname.c_str()));
							// Если используется хост и порт
							} else {
								// Выводим сообщение об незапущенном сервере за порту
								this->_log->print("Server [%s:%u] has been started successfully", log_t::flag_t::CRITICAL, shm->_host.c_str(), shm->_port);
								// Если функция обратного вызова установлена
								if(this->_callback.is("error"))
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::START, this->_fmk->format("Server cannot be started [%s:%u]", shm->_host.c_str(), shm->_port));
							}
						}
					}
				}
			// Если IP-адрес сервера не получен
			} else {
				// Выводим в консоль информацию
				this->_log->print("Broken server host %s", log_t::flag_t::CRITICAL, shm->_host.c_str());
				// Если функция обратного вызова установлена
				if(this->_callback.is("error"))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::START, this->_fmk->format("Broken server host %s", shm->_host.c_str()));
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
 * callback Метод установки функций обратного вызова
 * @param callback функции обратного вызова
 */
void awh::server::Core::callback(const callback_t & callback) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Устанавливаем функций обратного вызова
	awh::core_t::callback(callback);
	// Выполняем установку функции обратного вызова при завершении работы процесса кластера
	this->_callback.set("exit", callback);
	// Выполняем установку функции обратного вызова при открытии подключения
	this->_callback.set("open", callback);
	// Выполняем установку функции обратного вызова при чтении данных из сокета
	this->_callback.set("read", callback);
	// Выполняем установку функции обратного вызова при записи данных в сокет
	this->_callback.set("write", callback);
	// Выполняем установку функции обратного вызова при возникновении ошибки
	this->_callback.set("error", callback);
	// Выполняем установку функции обратного вызова при проверки подключения клиента
	this->_callback.set("accept", callback);
	// Выполняем установку функции обратного вызова при пересоздании процесса кластера
	this->_callback.set("rebase", callback);
	// Выполняем установку функции обратного вызова при активации работы кластера
	this->_callback.set("cluster", callback);
	// Выполняем установку функции обратного вызова при подключении клиента к серверу
	this->_callback.set("connect", callback);
	// Выполняем установку функции обратного вызова при получении сообщения кластера
	this->_callback.set("message", callback);
	// Выполняем установку функции обратного вызова для выполнения события запуска сервера
	this->_callback.set("launched", callback);
	// Выполняем установку функции обратного вызова при освобождении буфера хранения полезной нагрузки
	this->_callback.set("available", callback);
	// Выполняем установку функции обратного вызова при заполнении буфера хранения полезной нагрузки
	this->_callback.set("unavailable", callback);
	// Выполняем установку функции обратного вызова при отключении клиента от сервера
	this->_callback.set("disconnect", callback);
}
/**
 * transferRule Метод установки правила передачи данных
 * @param transfer правило передачи данных
 */
void awh::server::Core::transferRule(const transfer_t transfer) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Выполняем установку правила передачи данных
	this->_transfer = transfer;
}
/**
 * total Метод установки максимального количества одновременных подключений
 * @param sid   идентификатор схемы сети
 * @param total максимальное количество одновременных подключений
 */
void awh::server::Core::total(const uint16_t sid, const uint16_t total) noexcept {
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
 * clusterName Метод установки названия кластера
 * @param name название кластера для установки
 */
void awh::server::Core::clusterName(const string & name) noexcept {
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.main);
		// Выполняем установку названия кластера
		this->_cluster.name(name);
	/**
	 * Для операционной системы OS Windows
	 */
	#else
		// Выводим предупредительное сообщение в лог
		this->_log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
		// Если функция обратного вызова установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::OS_BROKEN, "MS Windows OS, does not support cluster mode");
	#endif
}
/**
 * clusterAutoRestart Метод установки флага перезапуска процессов
 * @param mode флаг перезапуска процессов
 */
void awh::server::Core::clusterAutoRestart(const bool mode) noexcept {
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.main);
		// Разрешаем автоматический перезапуск упавших процессов
		this->_clusterAutoRestart = mode;
	/**
	 * Для операционной системы OS Windows
	 */
	#else
		// Выводим предупредительное сообщение в лог
		this->_log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
		// Если функция обратного вызова установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::OS_BROKEN, "MS Windows OS, does not support cluster mode");
	#endif
}
/**
 * clusterTransfer Метод установки режима передачи данных
 * @param transfer режим передачи данных
 */
void awh::server::Core::clusterTransfer(const cluster_t::transfer_t transfer) noexcept {
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.main);
		// Выполняем установку режима передачи данных
		this->_cluster.transfer(transfer);
	/**
	 * Для операционной системы OS Windows
	 */
	#else
		// Выводим предупредительное сообщение в лог
		this->_log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
		// Если функция обратного вызова установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::OS_BROKEN, "MS Windows OS, does not support cluster mode");
	#endif
}
/**
 * clusterBandwidth Метод установки пропускной способности сети кластера
 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
 */
void awh::server::Core::clusterBandwidth(const string & read, const string & write) noexcept {
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.main);
		// Выполняем установку пропускной способности сети кластера
		this->_cluster.bandwidth(read, write);
	/**
	 * Для операционной системы OS Windows
	 */
	#else
		// Выводим предупредительное сообщение в лог
		this->_log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
		// Если функция обратного вызова установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::OS_BROKEN, "MS Windows OS, does not support cluster mode");
	#endif
}
/**
 * cluster Метод проверки активации кластера
 * @return режим активации кластера
 */
awh::scheme_t::mode_t awh::server::Core::cluster() const noexcept {
	// Выводим режим активации кластера
	return this->_clusterMode;
}
/**
 * cluster Метод установки количества процессов кластера
 * @param mode флаг активации/деактивации кластера
 * @param size количество рабочих процессов
 */
void awh::server::Core::cluster(const awh::scheme_t::mode_t mode, const int16_t size) noexcept {
	/**
	 * Для операционной системы не являющейся OS Windows
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
				} else if(size > (static_cast <int16_t> (thread::hardware_concurrency()) * 2))
					// Устанавливаем количество рабочих процессов кластера
					this->_clusterSize = static_cast <int16_t> (thread::hardware_concurrency());
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
	 * Для операционной системы OS Windows
	 */
	#else
		// Выводим предупредительное сообщение в лог
		this->_log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
		// Если функция обратного вызова установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::OS_BROKEN, "MS Windows OS, does not support cluster mode");
	#endif
}
/**
 * init Метод инициализации сервера
 * @param sid  идентификатор схемы сети
 * @param port порт сервера
 * @param host хост сервера
 */
void awh::server::Core::init(const uint16_t sid, const uint32_t port, const string & host) noexcept {
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
					case static_cast <uint8_t> (scheme_t::family_t::IPC):
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
 * bandwidth Метод установки пропускной способности сети
 * @param bid   идентификатор брокера
 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
 */
void awh::server::Core::bandwidth(const uint64_t bid, const string & read, const string & write) noexcept {
	// Если идентификатор брокера подключений существует
	if((bid > 0) && this->has(bid)){
		// Создаём бъект активного брокера подключения
		awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (this->broker(bid));
		// Определяем тип сокета
		switch(static_cast <uint8_t> (this->_settings.sonet)){
			// Если тип сокета установлен как TCP/IP
			case static_cast <uint8_t> (scheme_t::sonet_t::TCP):
			// Если тип сокета установлен как TCP/IP TLS
			case static_cast <uint8_t> (scheme_t::sonet_t::TLS):
			// Если тип сокета установлен как DTLS
			case static_cast <uint8_t> (scheme_t::sonet_t::DTLS):
			// Если тип сокета установлен как SCTP
			case static_cast <uint8_t> (scheme_t::sonet_t::SCTP):
				// Устанавливаем размер буфера
				broker->ectx.buffer(
					static_cast <int32_t> (!read.empty() ? this->_fmk->sizeBuffer(read) : AWH_BUFFER_SIZE_RCV),
					static_cast <int32_t> (!write.empty() ? this->_fmk->sizeBuffer(write) : AWH_BUFFER_SIZE_SND)
				);
			break;
			// Если тип сокета установлен как UDP
			case static_cast <uint8_t> (scheme_t::sonet_t::UDP): {
				// Выполняем поиск идентификатора схемы сети
				auto i = this->_schemes.find(broker->sid());
				// Если идентификатор схемы сети найден
				if(i != this->_schemes.end()){
					// Получаем объект схемы сети
					scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
					// Выполняем установку размера буфера сокета для чтения данных
					this->_socket.bufferSize(shm->_addr.fd, static_cast <int32_t> (!read.empty() ? this->_fmk->sizeBuffer(read) : AWH_BUFFER_SIZE_RCV), socket_t::mode_t::READ);
					// Выполняем установку размера буфера сокета для записи данных
					this->_socket.bufferSize(shm->_addr.fd, static_cast <int32_t> (!write.empty() ? this->_fmk->sizeBuffer(write) : AWH_BUFFER_SIZE_SND), socket_t::mode_t::WRITE);
				}
			} break;
		}
		// Выполняем создание буфера полезной нагрузки
		this->initBuffer(bid);
	}
}
/**
 * waitMessage Метод ожидания входящих сообщений
 * @param bid идентификатор брокера
 * @param sec интервал времени в секундах
 */
void awh::server::Core::waitMessage(const uint64_t bid, const uint16_t sec) noexcept {
	// Если идентификатор брокера подключения передан
	if(bid > 0){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.receive);
		// Выполняем удаление таймаута
		this->clearTimeout(bid);
		// Создаём бъект активного брокера подключения
		awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (this->broker(bid));
		// Если сокет подключения активен
		if((broker->addr.fd != INVALID_SOCKET) && (broker->addr.fd < AWH_MAX_SOCKETS))
			// Выполняем установку времени ожидания входящих сообщений
			broker->timeouts.wait = sec;
	}
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param bid     идентификатор брокера
 * @param read    количество секунд для детекции по чтению
 * @param write   количество секунд для детекции по записи
 * @param connect количество секунд для детекции по подключению
 */
void awh::server::Core::waitTimeDetect(const uint64_t bid, const uint16_t read, const uint16_t write, const uint16_t connect) noexcept {
	// Если идентификатор брокера подключения передан
	if(bid > 0){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.receive);
		// Выполняем удаление таймаута
		this->clearTimeout(bid);
		// Создаём бъект активного брокера подключения
		awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (this->broker(bid));
		// Если сокет подключения активен
		if((broker->addr.fd != INVALID_SOCKET) && (broker->addr.fd < AWH_MAX_SOCKETS)){
			// Устанавливаем количество секунд на чтение
			broker->timeouts.read = read;
			// Устанавливаем количество секунд на запись
			broker->timeouts.write = write;
			// Устанавливаем количество секунд на подключение
			broker->timeouts.connect = connect;
		}
	}
}
/**
 * Core Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::Core::Core(const fmk_t * fmk, const log_t * log) noexcept :
 awh::node_t(fmk, log), _socket(fmk, log), _cluster(this, fmk, log),
 _clusterSize(-1), _clusterAutoRestart(false),
 _clusterMode(awh::scheme_t::mode_t::DISABLED), _timer(nullptr) {
	// Устанавливаем тип запускаемого ядра
	this->_type = engine_t::type_t::SERVER;
	// Устанавливаем функцию получения события завершения работы процесса
	this->_cluster.on <void (const uint16_t, const pid_t, const int32_t)> ("exit", &core_t::exit, this, _1, _2, _3);
	// Устанавливаем функцию получения события пересоздании процесса
	this->_cluster.on <void (const uint16_t, const pid_t, const pid_t)> ("rebase", &core_t::rebase, this, _1, _2, _3);
	// Устанавливаем функцию получения сообщений процессов кластера
	this->_cluster.on <void (const uint16_t, const pid_t, const char *, const size_t)> ("message", &core_t::message, this, _1, _2, _3, _4);
	// Устанавливаем функцию получения статуса кластера
	this->_cluster.on <void (const uint16_t, const pid_t, const cluster_t::event_t)> ("process", static_cast <void (core_t::*)(const uint16_t, const pid_t, const cluster_t::event_t)> (&core_t::cluster), this, _1, _2, _3);
}
/**
 * Core Конструктор
 * @param dns объект DNS-резолвера
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::Core::Core(const dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept :
 awh::node_t(dns, fmk, log), _socket(fmk, log), _cluster(this, fmk, log),
 _transfer(transfer_t::SYNC), _clusterSize(-1), _clusterAutoRestart(false),
 _clusterMode(awh::scheme_t::mode_t::DISABLED), _timer(nullptr) {
	// Устанавливаем тип запускаемого ядра
	this->_type = engine_t::type_t::SERVER;
	// Устанавливаем функцию получения события завершения работы процесса
	this->_cluster.on <void (const uint16_t, const pid_t, const int32_t)> ("exit", &core_t::exit, this, _1, _2, _3);
	// Устанавливаем функцию получения события пересоздании процесса
	this->_cluster.on <void (const uint16_t, const pid_t, const pid_t)> ("rebase", &core_t::rebase, this, _1, _2, _3);
	// Устанавливаем функцию получения сообщений процессов кластера
	this->_cluster.on <void (const uint16_t, const pid_t, const char *, const size_t)> ("message", &core_t::message, this, _1, _2, _3, _4);
	// Устанавливаем функцию получения статуса кластера
	this->_cluster.on <void (const uint16_t, const pid_t, const cluster_t::event_t)> ("process", static_cast <void (core_t::*)(const uint16_t, const pid_t, const cluster_t::event_t)> (&core_t::cluster), this, _1, _2, _3);
}
