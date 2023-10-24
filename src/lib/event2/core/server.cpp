/**
 * @file: server.cpp
 * @date: 2022-09-08
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2022
 */

// Подключаем заголовочный файл
#include <lib/event2/core/server.hpp>

/**
 * accept Метод вызова при подключении к серверу
 * @param fd    файловый дескриптор (сокет)
 * @param event произошедшее событие
 */
void awh::server::scheme_t::accept(const evutil_socket_t fd, const short event) noexcept {
	// Получаем объект подключения
	core_t * core = dynamic_cast <core_t *> (const_cast <awh::core_t *> (this->_core));
	// Выполняем подключение клиента
	core->accept(fd, this->sid);
}
/**
 * callback Метод обратного вызова
 * @param fd    файловый дескриптор (сокет)
 * @param event произошедшее событие
 */
void awh::server::Core::DTLS::callback(const evutil_socket_t fd, const short event) noexcept {
	// Останавливаем работу таймера
	this->event.stop();
	// Выполняем извлечение брокера
	auto it = this->core->_brokers.find(this->bid);
	// Если брокер получен
	if(it != this->core->_brokers.end()){
		// Получаем объект брокера
		awh::scheme_t::broker_t * adj = const_cast <awh::scheme_t::broker_t *> (it->second);
		// Получаем объект схемы сети
		scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (adj->parent));
		// Выполняем ожидание входящих подключений
		if(this->core->_engine.wait(adj->_ectx)){
			// Устанавливаем параметры сокета
			adj->_addr.sonet(SOCK_DGRAM, IPPROTO_UDP);
			// Если прикрепление клиента к серверу выполнено
			if(adj->_addr.attach(shm->_addr)){
				// Выполняем прикрепление контекста клиента к контексту сервера
				this->core->_engine.attach(adj->_ectx, &adj->_addr);
				// Если MAC или IP адрес не получен, тогда выходим
				if(adj->_addr.mac.empty() || adj->_addr.ip.empty()){
					// Выполняем очистку контекста двигателя
					adj->_ectx.clear();
					// Если подключение не установлено, выводим сообщение об ошибке
					this->core->_log->print("Client address not received, pid = %d", log_t::flag_t::WARNING, getpid());
					// Если функция обратного вызова установлена
					if(this->core->_callback.is("error"))
						// Выполняем функцию обратного вызова
						this->core->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::WARNING, error_t::ACCEPT, this->core->_fmk->format("Client address not received, pid = %d", getpid()));
				// Если все данные получены
				} else {
					// Получаем адрес подключения клиента
					adj->_ip = adj->_addr.ip;
					// Получаем аппаратный адрес клиента
					adj->_mac = adj->_addr.mac;
					// Получаем порт подключения клиента
					adj->_port = adj->_addr.port;
					// Если функция обратного вызова проверки подключения установлена, выполняем проверку, если проверка не пройдена?
					if((shm->callback.is("accept")) && !shm->callback.apply <bool, const string &, const string &, const u_int, const uint16_t, awh::core_t *> ("accept", adj->_ip, adj->_mac, adj->_port, shm->sid, this->core)){
						// Если порт установлен
						if(adj->_port > 0){
							// Выводим сообщение об ошибке
							this->core->_log->print(
								"Access to the server is denied for the client [%s:%d], mac = %s, socket = %d, pid = %d",
								log_t::flag_t::WARNING,
								adj->_ip.c_str(),
								adj->_port,
								adj->_mac.c_str(),
								adj->_addr.fd,
								getpid()
							);
							// Если функция обратного вызова установлена
							if(this->core->_callback.is("error"))
								// Выполняем функцию обратного вызова
								this->core->_callback.call <const log_t::flag_t, const error_t, const string &> (
									"error",
									log_t::flag_t::WARNING,
									error_t::ACCEPT,
									this->core->_fmk->format(
										"Access to the server is denied for the client [%s:%d], mac = %s, socket = %d, pid = %d",
										adj->_ip.c_str(),
										adj->_port,
										adj->_mac.c_str(),
										adj->_addr.fd,
										getpid()
									)
								);
						// Если порт не установлен
						} else {
							// Выводим сообщение об ошибке
							this->core->_log->print(
								"Access to the server is denied for the client [%s], mac = %s, socket = %d, pid = %d",
								log_t::flag_t::WARNING,
								adj->_ip.c_str(),
								adj->_mac.c_str(),
								adj->_addr.fd,
								getpid()
							);
							// Если функция обратного вызова установлена
							if(this->core->_callback.is("error"))
								// Выполняем функцию обратного вызова
								this->core->_callback.call <const log_t::flag_t, const error_t, const string &> (
									"error",
									log_t::flag_t::WARNING,
									error_t::ACCEPT,
									this->core->_fmk->format(
										"Access to the server is denied for the client [%s], mac = %s, socket = %d, pid = %d",
										adj->_ip.c_str(),
										adj->_mac.c_str(),
										adj->_addr.fd,
										getpid()
									)
								);
						}
						// Выполняем отключение брокера
						this->core->close(this->bid);
						// Выходим
						return;
					}
					// Переводим сокет в неблокирующий режим
					adj->_ectx.block();
					
					// Если вывод информационных данных не запрещён
					if(!this->core->_noinfo){
						// Если порт установлен
						if(adj->_port > 0){
							// Выводим в консоль информацию
							this->core->_log->print(
								"Connect to server client [%s:%d], mac = %s, socket = %d, pid = %d",
								log_t::flag_t::INFO,
								adj->_ip.c_str(),
								adj->_port,
								adj->_mac.c_str(),
								adj->_addr.fd, getpid()
							);
						// Если порт не установлен
						} else {
							// Выводим в консоль информацию
							this->core->_log->print(
								"Connect to server client [%s], mac = %s, socket = %d, pid = %d",
								log_t::flag_t::INFO,
								adj->_ip.c_str(),
								adj->_mac.c_str(),
								adj->_addr.fd, getpid()
							);
						}
					}
					// Запускаем чтение данных
					this->core->enabled(engine_t::method_t::READ, this->bid);
					// Если функция обратного вызова установлена
					if(shm->callback.is("connect"))
						// Выполняем функцию обратного вызова
						shm->callback.call <const uint64_t, const uint16_t, awh::core_t *> ("connect", this->bid, shm->sid, this->core);
				}
			// Подключение не установлено
			} else {
				// Выводим сообщение об ошибке
				this->core->_log->print("Accepting failed, pid = %d", log_t::flag_t::WARNING, getpid());
				// Если функция обратного вызова установлена
				if(this->core->_callback.is("error"))
					// Выполняем функцию обратного вызова
					this->core->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::WARNING, error_t::ACCEPT, this->core->_fmk->format("Accepting failed, pid = %d", getpid()));
			}
		// Запускаем таймер вновь на 100мс
		} else this->event.start(100);
	}
}
/**
 * ~DTLS Деструктор
 */
awh::server::Core::DTLS::~DTLS() noexcept {
	// Останавливаем работу таймера
	this->event.stop();
}
/**
 * cluster Метод события ЗАПУСКА/ОСТАНОВКИ кластера
 * @param sid   идентификатор схемы сети
 * @param pid   идентификатор процесса
 * @param event идентификатор события
 */
void awh::server::Core::cluster(const uint16_t sid, const pid_t pid, const cluster_t::event_t event) noexcept {
	// Выполняем поиск идентификатора схемы сети
	auto it = this->_schemes.find(sid);
	// Если идентификатор схемы сети найден, устанавливаем максимальное количество одновременных подключений
	if(it != this->_schemes.end()){
		// Получаем объект подключения
		scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (it->second));
		// Определяем члена семейства кластера
		const cluster_t::family_t family = (this->_pid == getpid() ? cluster_t::family_t::MASTER : cluster_t::family_t::CHILDREN);
		// Выполняем тип возникшего события
		switch(static_cast <uint8_t> (event)){
			// Если производится запуск процесса
			case static_cast <uint8_t> (cluster_t::event_t::START): {
				// Если процесс является дочерним
				if(family == cluster_t::family_t::CHILDREN){
					// Запоминаем текущий идентификатор процесса
					this->_pid = pid;
					// Определяем тип сокета
					switch(static_cast <uint8_t> (this->_settings.sonet)){
						// Если тип сокета установлен как UDP
						case static_cast <uint8_t> (scheme_t::sonet_t::UDP):
							// Выполняем активацию сервера
							this->accept(1, it->first);
						break;
						// Для всех остальных типов сокетов
						default: {
							// Устанавливаем базу данных событий
							shm->_event.set(this->_dispatch.base);
							// Устанавливаем тип события
							shm->_event.set(shm->_addr.fd, EV_READ | EV_PERSIST);
							// Устанавливаем функцию обратного вызова
							shm->_event.set(std::bind(&scheme_t::accept, shm, _1, _2));
							// Выполняем запуск работы события
							shm->_event.start();
						}
					}
				}
			} break;
			// Если производится остановка процесса
			case static_cast <uint8_t> (cluster_t::event_t::STOP): {
				// Если тип сокета не установлен как UDP
				if(this->_settings.sonet != scheme_t::sonet_t::UDP)
					// Останавливаем чтение данных с клиента
					shm->_event.stop();
			} break;
		}
		// Если функция обратного вызова установлена
		if(this->_callback.is("cluster"))
			// Выполняем функцию обратного вызова
			this->_callback.call <const cluster_t::family_t, const uint16_t, const pid_t, const cluster_t::event_t, awh::core_t *> ("cluster", family, sid, pid, event, dynamic_cast <awh::core_t *> (this));
	}
}
/**
 * accept Метод вызова при подключении к серверу
 * @param fd  файловый дескриптор (сокет) подключившегося клиента
 * @param sid идентификатор схемы сети
 */
void awh::server::Core::accept(const int fd, const uint16_t sid) noexcept {
	// Если идентификатор схемы сети передан
	if((sid > 0) && (fd != INVALID_SOCKET) && (fd < MAX_SOCKETS)){
		// Выполняем поиск идентификатора схемы сети
		auto it = this->_schemes.find(sid);
		// Если идентификатор схемы сети найден, устанавливаем максимальное количество одновременных подключений
		if(it != this->_schemes.end()){
			// Получаем объект подключения
			scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (it->second));
			// Определяем тип сокета
			switch(static_cast <uint8_t> (this->_settings.sonet)){
				// Если тип сокета установлен как UDP
				case static_cast <uint8_t> (scheme_t::sonet_t::UDP): {
					// Создаём бъект брокера
					unique_ptr <awh::scheme_t::broker_t> adj(new awh::scheme_t::broker_t(shm, this->_fmk, this->_log));
					// Определяем тип протокола подключения
					switch(static_cast <uint8_t> (this->_settings.family)){
						// Если тип протокола подключения IPv4
						case static_cast <uint8_t> (scheme_t::family_t::IPV4): {
							// Выполняем перебор всего списка адресов
							for(auto & host : this->_settings.net.first){
								// Если хост соответствует адресу IPv4
								if(this->_net.host(host) == net_t::type_t::IPV4)
									// Выполняем установку полученного хоста
									adj->_addr.network.push_back(host);
							}
						} break;
						// Если тип протокола подключения IPv6
						case static_cast <uint8_t> (scheme_t::family_t::IPV6): {
							// Выполняем перебор всего списка адресов
							for(auto & host : this->_settings.net.first){
								// Если хост соответствует адресу IPv4
								if(this->_net.host(host) == net_t::type_t::IPV6)
									// Выполняем установку полученного хоста
									adj->_addr.network.push_back(host);
							}
						} break;
					}
					// Устанавливаем параметры сокета
					adj->_addr.sonet(SOCK_DGRAM, IPPROTO_UDP);
					// Если unix-сокет используется
					if(this->_settings.family == scheme_t::family_t::NIX)
						// Выполняем инициализацию сокета
						adj->_addr.init(this->_settings.filename, engine_t::type_t::SERVER);
					// Если unix-сокет не используется, выполняем инициализацию сокета
					else adj->_addr.init(shm->_host, shm->_port, (this->_settings.family == scheme_t::family_t::IPV6 ? AF_INET6 : AF_INET), engine_t::type_t::SERVER, this->_ipV6only);
					// Выполняем разрешение подключения
					if(adj->_addr.accept(adj->_addr.fd, 0)){
						// Получаем адрес подключения клиента
						adj->_ip = adj->_addr.ip;
						// Получаем аппаратный адрес клиента
						adj->_mac = adj->_addr.mac;
						// Получаем порт подключения клиента
						adj->_port = adj->_addr.port;
						// Выполняем установку желаемого протокола подключения
						adj->_ectx.proto(this->_settings.proto);
						// Устанавливаем идентификатор брокера
						adj->_bid = this->_fmk->timestamp(fmk_t::stamp_t::NANOSECONDS);
						// Выполняем получение контекста сертификата
						this->_engine.wrap(adj->_ectx, &adj->_addr);
						// Если подключение не обёрнуто
						if((adj->_addr.fd == INVALID_SOCKET) || (adj->_addr.fd >= MAX_SOCKETS)){
							// Выводим сообщение об ошибке
							this->_log->print("Wrap engine context is failed", log_t::flag_t::CRITICAL);
							// Если функция обратного вызова установлена
							if(this->_callback.is("error"))
								// Выполняем функцию обратного вызова
								this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::CRITICAL, error_t::ACCEPT, "Wrap engine context is failed");
							// Выходим из функции
							return;
						}
						// Выполняем блокировку потока
						this->_mtx.accept.lock();
						// Добавляем созданного брокера в список брокеров
						auto ret = shm->_brokers.emplace(adj->_bid, std::forward <unique_ptr <awh::scheme_t::broker_t>> (adj));
						// Добавляем брокера в список подключений
						this->_brokers.emplace(ret.first->first, ret.first->second.get());
						// Выполняем блокировку потока
						this->_mtx.accept.unlock();
						// Переводим сокет в неблокирующий режим
						ret.first->second->_ectx.block();
						// Запускаем чтение данных
						this->enabled(engine_t::method_t::READ, ret.first->first);
						// Если функция обратного вызова установлена
						if(shm->callback.is("connect"))
							// Выполняем функцию обратного вызова
							shm->callback.call <const uint64_t, const uint16_t, awh::core_t *> ("connect", ret.first->first, shm->sid, this);
					// Подключение не установлено
					} else {
						// Выводим сообщение об ошибке
						this->_log->print("Accepting failed, pid = %d", log_t::flag_t::WARNING, getpid());
						// Если функция обратного вызова установлена
						if(this->_callback.is("error"))
							// Выполняем функцию обратного вызова
							this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::WARNING, error_t::ACCEPT, this->_fmk->format("Accepting failed, pid = %d", getpid()));
					}
				} break;
				// Если подключение зашифрованно
				case static_cast <uint8_t> (scheme_t::sonet_t::DTLS): {
					// Если количество подключившихся клиентов, больше максимально-допустимого количества клиентов
					if(shm->_brokers.size() >= static_cast <size_t> (shm->_total)){
						// Выводим в консоль информацию
						this->_log->print("Number of simultaneous connections, cannot exceed the maximum allowed number of %d", log_t::flag_t::WARNING, shm->_total);
						// Если функция обратного вызова установлена
						if(this->_callback.is("error"))
							// Выполняем функцию обратного вызова
							this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::WARNING, error_t::ACCEPT, this->_fmk->format("Number of simultaneous connections, cannot exceed the maximum allowed number of %d", shm->_total));
						// Выходим
						break;
					}
					// Создаём объект для работы с DTLS
					unique_ptr <dtls_t> dtls(new dtls_t(this->_log));
					// Создаём бъект брокера
					unique_ptr <awh::scheme_t::broker_t> adj(new awh::scheme_t::broker_t(shm, this->_fmk, this->_log));
					// Устанавливаем идентификатор брокера
					adj->_bid = this->_fmk->timestamp(fmk_t::stamp_t::NANOSECONDS);
					// Устанавливаем объект сетевого ядра
					dtls->core = this;
					// Устанавливаем идентификатор брокера
					dtls->bid = adj->_bid;
					// Устанавливаем тип таймера
					dtls->event.set(-1, EV_TIMEOUT);
					// Устанавливаем базу данных событий
					dtls->event.set(this->_dispatch.base);
					// Устанавливаем функцию обратного вызова
					dtls->event.set(std::bind(&dtls_t::callback, dtls.get(), _1, _2));
					// Выполняем установку желаемого протокола подключения
					adj->_ectx.proto(this->_settings.proto);
					// Выполняем получение контекста сертификата
					this->_engine.wrap(adj->_ectx, &shm->_addr, engine_t::type_t::SERVER);
					// Выполняем блокировку потока
					this->_mtx.accept.lock();
					// Добавляем созданного брокера в список брокеров
					auto ret = shm->_brokers.emplace(adj->_bid, std::forward <unique_ptr <awh::scheme_t::broker_t>> (adj));
					// Добавляем брокера в список подключений
					this->_brokers.emplace(ret.first->first, ret.first->second.get());
					// Добавляем объект для работы с DTLS в список
					this->_dtls.emplace(ret.first->first, std::forward <unique_ptr <dtls_t>> (dtls)).first->second->event.start(100);
					// Выполняем блокировку потока
					this->_mtx.accept.unlock();
					// Останавливаем работу сервера
					shm->_event.stop();
				} break;
				// Если тип сокета установлен как TCP/IP
				case static_cast <uint8_t> (scheme_t::sonet_t::TCP):
				// Если тип сокета установлен как TCP/IP TLS
				case static_cast <uint8_t> (scheme_t::sonet_t::TLS):
				// Если тип сокета установлен как SCTP DTLS
				case static_cast <uint8_t> (scheme_t::sonet_t::SCTP): {
					// Если количество подключившихся клиентов, больше максимально-допустимого количества клиентов
					if(shm->_brokers.size() >= static_cast <size_t> (shm->_total)){
						// Выводим в консоль информацию
						this->_log->print("Number of simultaneous connections, cannot exceed the maximum allowed number of %d", log_t::flag_t::WARNING, shm->_total);
						// Если функция обратного вызова установлена
						if(this->_callback.is("error"))
							// Выполняем функцию обратного вызова
							this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::WARNING, error_t::ACCEPT, this->_fmk->format("Number of simultaneous connections, cannot exceed the maximum allowed number of %d", shm->_total));
						// Выходим
						break;
					}
					// Создаём бъект брокера
					unique_ptr <awh::scheme_t::broker_t> adj(new awh::scheme_t::broker_t(shm, this->_fmk, this->_log));
					// Устанавливаем время жизни подключения
					adj->_addr.alive = shm->keepAlive;
					// Определяем тип сокета
					switch(static_cast <uint8_t> (this->_settings.sonet)){
						/**
						 * Если операционной системой является Linux или FreeBSD
						 */
						#if defined(__linux__) || defined(__FreeBSD__)
							// Если тип сокета установлен как SCTP
							case static_cast <uint8_t> (scheme_t::sonet_t::SCTP):
								// Устанавливаем параметры сокета
								adj->_addr.sonet(SOCK_STREAM, IPPROTO_SCTP);
							break;
						#endif
						// Для всех остальных типов сокетов
						default:
							// Устанавливаем параметры сокета
							adj->_addr.sonet(SOCK_STREAM, IPPROTO_TCP);
					}
					// Выполняем разрешение подключения
					if(adj->_addr.accept(shm->_addr)){
						// Если MAC или IP адрес не получен, тогда выходим
						if(adj->_addr.mac.empty() || adj->_addr.ip.empty()){
							// Выполняем очистку контекста двигателя
							adj->_ectx.clear();
							// Если подключение не установлено, выводим сообщение об ошибке
							this->_log->print("Client address not received, pid = %d", log_t::flag_t::WARNING, getpid());
							// Если функция обратного вызова установлена
							if(this->_callback.is("error"))
								// Выполняем функцию обратного вызова
								this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::WARNING, error_t::ACCEPT, this->_fmk->format("Client address not received, pid = %d", getpid()));
						// Если все данные получены
						} else {
							// Получаем адрес подключения клиента
							adj->_ip = adj->_addr.ip;
							// Получаем аппаратный адрес клиента
							adj->_mac = adj->_addr.mac;
							// Получаем порт подключения клиента
							adj->_port = adj->_addr.port;
							// Если функция обратного вызова проверки подключения установлена, выполняем проверку, если проверка не пройдена?
							if((shm->callback.is("accept")) && !shm->callback.apply <bool, const string &, const string &, const u_int, const uint16_t, awh::core_t *> ("accept", adj->_ip, adj->_mac, adj->_port, shm->sid, this)){
								// Если порт установлен
								if(adj->_port > 0){
									// Выводим сообщение об ошибке
									this->_log->print(
										"Access to the server is denied for the client [%s:%d], mac = %s, socket = %d, pid = %d",
										log_t::flag_t::WARNING,
										adj->_ip.c_str(),
										adj->_port,
										adj->_mac.c_str(),
										adj->_addr.fd,
										getpid()
									);
									// Если функция обратного вызова установлена
									if(this->_callback.is("error"))
										// Выполняем функцию обратного вызова
										this->_callback.call <const log_t::flag_t, const error_t, const string &> (
											"error",
											log_t::flag_t::WARNING,
											error_t::ACCEPT,
											this->_fmk->format(
												"Access to the server is denied for the client [%s:%d], mac = %s, socket = %d, pid = %d",
												adj->_ip.c_str(),
												adj->_port,
												adj->_mac.c_str(),
												adj->_addr.fd,
												getpid()
											)
										);
								// Если порт не установлен
								} else {
									// Выводим сообщение об ошибке
									this->_log->print(
										"Access to the server is denied for the client [%s], mac = %s, socket = %d, pid = %d",
										log_t::flag_t::WARNING,
										adj->_ip.c_str(),
										adj->_mac.c_str(),
										adj->_addr.fd,
										getpid()
									);
									// Если функция обратного вызова установлена
									if(this->_callback.is("error"))
										// Выполняем функцию обратного вызова
										this->_callback.call <const log_t::flag_t, const error_t, const string &> (
											"error",
											log_t::flag_t::WARNING,
											error_t::ACCEPT,
											this->_fmk->format(
												"Access to the server is denied for the client [%s], mac = %s, socket = %d, pid = %d",
												adj->_ip.c_str(),
												adj->_mac.c_str(),
												adj->_addr.fd,
												getpid()
											)
										);
								}
								// Выполняем очистку контекста двигателя
								adj->_ectx.clear();
								// Выходим
								break;
							}
							// Выполняем установку желаемого протокола подключения
							adj->_ectx.proto(this->_settings.proto);
							// Устанавливаем идентификатор брокера
							adj->_bid = this->_fmk->timestamp(fmk_t::stamp_t::NANOSECONDS);
							// Выполняем получение контекста сертификата
							this->_engine.wrap(adj->_ectx, &adj->_addr);
							// Если мы хотим работать в зашифрованном режиме
							if(this->_settings.sonet == scheme_t::sonet_t::TLS){
								// Если сертификаты не приняты, выходим
								if(!this->_engine.encrypted(adj->_ectx)){
									// Выполняем очистку контекста двигателя
									adj->_ectx.clear();
									// Выводим сообщение об ошибке
									this->_log->print("Encryption mode cannot be activated", log_t::flag_t::CRITICAL);
									// Если функция обратного вызова установлена
									if(this->_callback.is("error"))
										// Выполняем функцию обратного вызова
										this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::CRITICAL, error_t::ACCEPT, "Encryption mode cannot be activated");
									// Выходим
									break;
								}
							}
							// Если подключение не обёрнуто
							if((adj->_addr.fd == INVALID_SOCKET) || (adj->_addr.fd >= MAX_SOCKETS)){
								// Выводим сообщение об ошибке
								this->_log->print("Wrap engine context is failed", log_t::flag_t::CRITICAL);
								// Если функция обратного вызова установлена
								if(this->_callback.is("error"))
									// Выполняем функцию обратного вызова
									this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::CRITICAL, error_t::ACCEPT, "Wrap engine context is failed");
								// Выходим
								break;
							}
							// Выполняем блокировку потока
							this->_mtx.accept.lock();
							// Добавляем созданного брокера в список брокеров
							auto ret = shm->_brokers.emplace(adj->_bid, std::forward <unique_ptr <awh::scheme_t::broker_t>> (adj));
							// Добавляем брокера в список подключений
							this->_brokers.emplace(ret.first->first, ret.first->second.get());
							// Выполняем блокировку потока
							this->_mtx.accept.unlock();
							// Переводим сокет в неблокирующий режим
							ret.first->second->_ectx.noblock();
							// Если вывод информационных данных не запрещён
							if(!this->_noinfo){
								// Если порт установлен
								if(ret.first->second->_port > 0){
									// Выводим в консоль информацию
									this->_log->print(
										"Connect to server client [%s:%d], mac = %s, socket = %d, pid = %d",
										log_t::flag_t::INFO,
										ret.first->second->_ip.c_str(),
										ret.first->second->_port,
										ret.first->second->_mac.c_str(),
										ret.first->second->_addr.fd, getpid()
									);
								// Если порт не установлен
								} else {
									// Выводим в консоль информацию
									this->_log->print(
										"Connect to server client [%s], mac = %s, socket = %d, pid = %d",
										log_t::flag_t::INFO,
										ret.first->second->_ip.c_str(),
										ret.first->second->_mac.c_str(),
										ret.first->second->_addr.fd, getpid()
									);
								}
							}
							// Запускаем чтение данных
							this->enabled(engine_t::method_t::READ, ret.first->first);
							// Если функция обратного вызова установлена
							if(shm->callback.is("connect"))
								// Выполняем функцию обратного вызова
								shm->callback.call <const uint64_t, const uint16_t, awh::core_t *> ("connect", ret.first->first, shm->sid, this);
						}
					// Если подключение не установлено
					} else {
						// Выводим сообщение об ошибке
						this->_log->print("Accepting failed, pid = %d", log_t::flag_t::WARNING, getpid());
						// Если функция обратного вызова установлена
						if(this->_callback.is("error"))
							// Выполняем функцию обратного вызова
							this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::WARNING, error_t::ACCEPT, this->_fmk->format("Accepting failed, pid = %d", getpid()));
					}
				} break;
			}
		}
	}
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
				for(auto it = shm->_brokers.begin(); it != shm->_brokers.end();){
					// Если блокировка брокера не установлена
					if(this->_locking.count(it->first) < 1){
						// Выполняем блокировку брокера
						this->_locking.emplace(it->first);
						// Получаем объект брокера
						awh::scheme_t::broker_t * adj = const_cast <awh::scheme_t::broker_t *> (it->second.get());
						// Выполняем очистку буфера событий
						this->clean(it->first);
						// Выполняем очистку контекста двигателя
						adj->_ectx.clear();
						// Удаляем брокера из списка подключений
						this->_brokers.erase(it->first);
						// Если функция обратного вызова установлена
						if(shm->callback.is("disconnect"))
							// Устанавливаем полученную функцию обратного вызова
							callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> (it->first, shm->callback.get <void (const uint64_t, const uint16_t, awh::core_t *)> ("disconnect"), it->first, item.first, this);
						// Если список объектов DTLS не пустой
						if(!this->_dtls.empty())
							// Удаляем объект для работы DTLS из списка
							this->_dtls.erase(it->first);
						// Удаляем блокировку брокера
						this->_locking.erase(it->first);
						// Удаляем брокера из списка
						it = shm->_brokers.erase(it);
					// Иначе продолжаем дальше
					} else ++it;
				}
				// Останавливаем работу кластера
				this->_cluster.stop(shm->sid);
			}
			// Останавливаем работу сервера
			shm->_event.stop();
			// Выполняем закрытие подключение сервера
			shm->_addr.clear();
		}
		// Выполняем все функции обратного вызова
		callback.bind <const uint64_t, const uint16_t, awh::core_t *> ();
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
		// Объект работы с функциями обратного вызова
		fn_t callback(this->_log);
		// Переходим по всему списку схем сети
		for(auto it = this->_schemes.begin(); it != this->_schemes.end();){
			// Получаем объект схемы сети
			scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (it->second));
			// Если в схеме сети есть подключённые клиенты
			if(!shm->_brokers.empty()){
				// Переходим по всему списку брокера
				for(auto jt = shm->_brokers.begin(); jt != shm->_brokers.end();){
					// Если блокировка брокера не установлена
					if(this->_locking.count(jt->first) < 1){
						// Выполняем блокировку брокера
						this->_locking.emplace(jt->first);
						// Получаем объект брокера
						awh::scheme_t::broker_t * adj = const_cast <awh::scheme_t::broker_t *> (jt->second.get());
						// Выполняем очистку буфера событий
						this->clean(jt->first);
						// Выполняем очистку контекста двигателя
						adj->_ectx.clear();
						// Удаляем брокера из списка подключений
						this->_brokers.erase(jt->first);
						// Если функция обратного вызова установлена
						if(shm->callback.is("disconnect"))
							// Устанавливаем полученную функцию обратного вызова
							callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> (jt->first, shm->callback.get <void (const uint64_t, const uint16_t, awh::core_t *)> ("disconnect"), jt->first, it->first, this);
						// Если список объектов DTLS не пустой
						if(!this->_dtls.empty())
							// Удаляем объект для работы DTLS из списка
							this->_dtls.erase(jt->first);
						// Удаляем блокировку брокера
						this->_locking.erase(jt->first);
						// Удаляем брокера из списка
						jt = shm->_brokers.erase(jt);
					// Иначе продолжаем дальше
					} else ++jt;
				}
				// Останавливаем работу кластера
				this->_cluster.stop(shm->sid);
			}
			// Останавливаем работу сервера
			shm->_event.stop();
			// Выполняем закрытие подключение сервера
			shm->_addr.clear();
			// Выполняем удаление схемы сети
			it = this->_schemes.erase(it);
		}
		// Выполняем все функции обратного вызова
		callback.bind <const uint64_t, const uint16_t, awh::core_t *> ();
	}
}
/**
 * run Метод запуска сервера
 * @param sid идентификатор схемы сети
 */
void awh::server::Core::run(const uint16_t sid) noexcept {
	// Если идентификатор схемы сети передан
	if(sid > 0){		
		// Выполняем поиск идентификатора схемы сети
		auto it = this->_schemes.find(sid);
		// Если идентификатор схемы сети найден, устанавливаем максимальное количество одновременных подключений
		if(it != this->_schemes.end()){			
			// Устанавливаем базу событий кластера
			this->_cluster.base(this->_dispatch.base);
			// Выполняем инициализацию кластера
			this->_cluster.init(it->first, this->_clusterSize);
			// Устанавливаем флаг автоматического перезапуска упавших процессов
			this->_cluster.restart(it->first, this->_clusterAutoRestart);
			// Получаем объект схемы сети
			scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (it->second));
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
						this->resolving(shm->sid, shm->_host, AF_INET);
					} break;
					// Если тип протокола подключения IPv6
					case static_cast <uint8_t> (scheme_t::family_t::IPV6): {
						// Обновляем хост сервера
						shm->_host = ifnet.ip(AF_INET6);
						// Выполняем подключения к полученному IP-адресу
						this->resolving(shm->sid, shm->_host, AF_INET6);
					} break;
				}
			// Если хост сервера является доменным именем
			} else {			
				// Определяем тип протокола подключения
				switch(static_cast <uint8_t> (this->_settings.family)){
					// Если тип протокола подключения unix-сокет
					case static_cast <uint8_t> (scheme_t::family_t::NIX):
					// Если тип протокола подключения IPv4
					case static_cast <uint8_t> (scheme_t::family_t::IPV4): {
						// Выполняем резолвинг домена
						const string & ip = this->_dns.resolve(AF_INET, shm->_host);
						// Выполняем подключения к полученному IP-адресу
						this->resolving(shm->sid, ip, AF_INET);
					} break;
					// Если тип протокола подключения IPv6
					case static_cast <uint8_t> (scheme_t::family_t::IPV6): {
						// Выполняем резолвинг домена
						const string & ip = this->_dns.resolve(AF_INET6, shm->_host);
						// Выполняем подключения к полученному IP-адресу
						this->resolving(shm->sid, ip, AF_INET6);
					} break;
				}
			}
		}
	}
}
/**
 * remove Метод удаления схемы сети
 * @param sid идентификатор схемы сети
 */
void awh::server::Core::remove(const uint16_t sid) noexcept {
	// Если идентификатор схемы сети передан
	if(sid > 0){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.close);
		// Выполняем поиск идентификатора схемы сети
		auto it = this->_schemes.find(sid);
		// Если идентификатор схемы сети найден, устанавливаем максимальное количество одновременных подключений
		if(it != this->_schemes.end()){
			// Объект работы с функциями обратного вызова
			fn_t callback(this->_log);
			// Получаем объект схемы сети
			scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (it->second));
			// Если в схеме сети есть подключённые клиенты
			if(!shm->_brokers.empty()){
				// Переходим по всему списку брокера
				for(auto jt = shm->_brokers.begin(); jt != shm->_brokers.end();){
					// Если блокировка брокера не установлена
					if(this->_locking.count(jt->first) < 1){
						// Выполняем блокировку брокера
						this->_locking.emplace(jt->first);
						// Получаем объект брокера
						awh::scheme_t::broker_t * adj = const_cast <awh::scheme_t::broker_t *> (jt->second.get());
						// Выполняем очистку буфера событий
						this->clean(jt->first);
						// Выполняем очистку контекста двигателя
						adj->_ectx.clear();
						// Если функция обратного вызова установлена
						if(shm->callback.is("disconnect"))
							// Устанавливаем полученную функцию обратного вызова
							callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> (jt->first, shm->callback.get <void (const uint64_t, const uint16_t, awh::core_t *)> ("disconnect"), jt->first, it->first, this);
						// Удаляем брокера из списка подключений
						this->_brokers.erase(jt->first);
						// Если список объектов DTLS не пустой
						if(!this->_dtls.empty())
							// Удаляем объект для работы DTLS из списка
							this->_dtls.erase(jt->first);
						// Удаляем блокировку брокера
						this->_locking.erase(jt->first);
						// Удаляем брокера из списка
						jt = shm->_brokers.erase(jt);
					// Иначе продолжаем дальше
					} else ++jt;
				}
			}
			// Останавливаем работу сервера
			shm->_event.stop();
			// Выполняем закрытие подключение сервера
			shm->_addr.clear();
			// Выполняем удаление схемы сети
			this->_schemes.erase(sid);
			// Выполняем все функции обратного вызова
			callback.bind <const uint64_t, const uint16_t, awh::core_t *> ();
		}
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
		if(this->_locking.count(bid) < 1){
			// Выполняем блокировку брокера
			this->_locking.emplace(bid);
			// Объект работы с функциями обратного вызова
			fn_t callback(this->_log);
			// Выполняем извлечение брокера
			auto it = this->_brokers.find(bid);
			// Если брокер получен
			if(it != this->_brokers.end()){
				// Получаем объект брокера
				awh::scheme_t::broker_t * adj = const_cast <awh::scheme_t::broker_t *> (it->second);
				// Получаем объект схемы сети
				scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (adj->parent));
				// Получаем объект ядра клиента
				const core_t * core = reinterpret_cast <const core_t *> (shm->_core);
				// Выполняем очистку буфера событий
				this->clean(bid);
				// Выполняем очистку контекста двигателя
				adj->_ectx.clear();
				// Удаляем брокера из списка брокеров
				shm->_brokers.erase(bid);
				// Удаляем брокера из списка подключений
				this->_brokers.erase(bid);
				// Если разрешено выводить информационыне уведомления
				if(!core->_noinfo)
					// Выводим информацию об удачном отключении от сервера
					this->_log->print("%s", log_t::flag_t::INFO, "Disconnect client from server");
				// Если функция обратного вызова установлена
				if(shm->callback.is("disconnect"))
					// Устанавливаем полученную функцию обратного вызова
					callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> (bid, shm->callback.get <void (const uint64_t, const uint16_t, awh::core_t *)> ("disconnect"), bid, shm->sid, this);
				// Если тип сокета установлен как DTLS, запускаем ожидание новых подключений
				if(this->_settings.sonet == scheme_t::sonet_t::DTLS){
					// Если функция обратного вызова установлена
					if(callback.is(bid)){
						// Выполняем все функции обратного вызова
						callback.bind <const uint64_t, const uint16_t, awh::core_t *> (bid);
						// Очищаем список функций обратного вызова
						callback.rm(bid);
					}
					// Выполняем закрытие подключение сервера
					shm->_addr.clear();
					// Если список объектов DTLS не пустой
					if(!this->_dtls.empty())
						// Удаляем объект для работы DTLS из списка
						this->_dtls.erase(bid);
					// Выполняем запуск сервера вновь
					this->run(shm->sid);
				}
			}
			// Удаляем блокировку брокера
			this->_locking.erase(bid);
			// Если функция обратного вызова установлена
			if(callback.is(bid))
				// Выполняем все функции обратного вызова
				callback.bind <const uint64_t, const uint16_t, awh::core_t *> (bid);
		}
	}
}
/**
 * read Метод чтения данных для брокера
 * @param bid идентификатор брокера
 */
void awh::server::Core::read(const uint64_t bid) noexcept {
	// Если данные переданы
	if(this->working() && (bid > 0)){
		// Выполняем извлечение брокера
		auto it = this->_brokers.find(bid);
		// Если брокер получен
		if(it != this->_brokers.end()){
			// Получаем объект брокера
			awh::scheme_t::broker_t * adj = const_cast <awh::scheme_t::broker_t *> (it->second);
			// Если сокет подключения активен
			if((adj->_addr.fd != INVALID_SOCKET) && (adj->_addr.fd < MAX_SOCKETS)){
				// Останавливаем чтение данных с клиента
				adj->_bev.events.read.stop();
				// Устанавливаем текущий метод режима работы
				adj->_method = engine_t::method_t::READ;
				// Получаем объект схемы сети
				scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (adj->parent));
				// Получаем максимальный размер буфера
				const int64_t size = adj->_ectx.buffer(engine_t::method_t::READ);
				// Если размер буфера получен
				if(size > 0){
					// Количество полученных байт
					int64_t bytes = -1;
					// Определяем тип сокета
					switch(static_cast <uint8_t> (this->_settings.sonet)){
						// Если тип сокета установлен как TCP/IP
						case static_cast <uint8_t> (scheme_t::sonet_t::TCP):
						// Если тип сокета установлен как TCP/IP TLS
						case static_cast <uint8_t> (scheme_t::sonet_t::TLS):
						// Если тип сокета установлен как SCTP
						case static_cast <uint8_t> (scheme_t::sonet_t::SCTP):
							// Переводим сокет в неблокирующий режим
							adj->_ectx.noblock();
						break;
					}
					// Создаём буфер входящих данных
					unique_ptr <char []> buffer(new char [size]);
					// Выполняем чтение данных с сокета
					do {
						// Если подключение выполнено и чтение данных разрешено
						if(!adj->_bev.locked.read){
							// Если тип сокета установлен как UDP или DTLS
							if((this->_settings.sonet == scheme_t::sonet_t::UDP) || (this->_settings.sonet == scheme_t::sonet_t::DTLS)){
								// Если флаг ожидания входящих сообщений, активирован
								if(adj->_timeouts.read > 0)
									// Выполняем установку таймаута ожидания
									adj->_ectx.timeout(adj->_timeouts.read * 1000, engine_t::method_t::READ);
								// Если флаг ожидания исходящих сообщений, активирован
								if(adj->_timeouts.write > 0)
									// Выполняем установку таймаута ожидания
									adj->_ectx.timeout(adj->_timeouts.write * 1000, engine_t::method_t::WRITE);
							}
							// Выполняем обнуление буфера данных
							::memset(buffer.get(), 0, size);
							// Выполняем получение сообщения от клиента
							bytes = adj->_ectx.read(buffer.get(), size);
							// Если время ожидания чтения данных установлено
							if(shm->wait && (adj->_timeouts.read > 0))
								// Запускаем работу таймера
								adj->_bev.timers.read.start(adj->_timeouts.read * 1000);
							// Останавливаем таймаут ожидания на чтение из сокета
							else adj->_bev.timers.read.stop();
							// Если данные получены
							if(bytes > 0){
								// Если данные считанные из буфера, больше размера ожидающего буфера
								if((adj->_marker.read.max > 0) && (bytes >= adj->_marker.read.max)){
									// Смещение в буфере и отправляемый размер данных
									size_t offset = 0, actual = 0;
									// Выполняем пересылку всех полученных данных
									while((bytes - offset) > 0){
										// Определяем размер отправляемых данных
										actual = ((bytes - offset) >= adj->_marker.read.max ? adj->_marker.read.max : (bytes - offset));
										// Если функция обратного вызова на получение данных установлена
										if(shm->callback.is("read"))
											// Выводим функцию обратного вызова
											shm->callback.call <const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *> ("read", buffer.get() + offset, actual, bid, shm->sid, reinterpret_cast <awh::core_t *> (this));
										// Увеличиваем смещение в буфере
										offset += actual;
									}
								// Если данных достаточно и функция обратного вызова на получение данных установлена
								} else if(shm->callback.is("read"))
									// Выводим функцию обратного вызова
									shm->callback.call <const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *> ("read", buffer.get(), bytes, bid, shm->sid, reinterpret_cast <awh::core_t *> (this));
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
					} while(this->method(bid) == engine_t::method_t::READ);
					// Если тип сокета не установлен как UDP, запускаем чтение дальше
					if((this->_settings.sonet != scheme_t::sonet_t::UDP) && (this->_brokers.count(bid) > 0))
						// Запускаем событие на чтение базы событий
						adj->_bev.events.read.start();
				// Выполняем отключение клиента
				} else this->close(bid);
			}
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
	if(this->working() && (bid > 0) && (buffer != nullptr) && (size > 0)){
		// Выполняем извлечение брокера
		auto it = this->_brokers.find(bid);
		// Если брокер получен
		if(it != this->_brokers.end()){
			// Получаем объект брокера
			awh::scheme_t::broker_t * adj = const_cast <awh::scheme_t::broker_t *> (it->second);
			// Если сокет подключения активен
			if((adj->_addr.fd != INVALID_SOCKET) && (adj->_addr.fd < MAX_SOCKETS)){
				// Определяем тип сокета
				switch(static_cast <uint8_t> (this->_settings.sonet)){
					// Если тип сокета установлен как TCP/IP
					case static_cast <uint8_t> (scheme_t::sonet_t::TCP):
					// Если тип сокета установлен как TCP/IP TLS
					case static_cast <uint8_t> (scheme_t::sonet_t::TLS):
					// Если тип сокета установлен как SCTP
					case static_cast <uint8_t> (scheme_t::sonet_t::SCTP):
						// Переводим сокет в неблокирующий режим
						adj->_ectx.block();
					break;
				}
				// Устанавливаем текущий метод режима работы
				adj->_method = engine_t::method_t::WRITE;
				// Получаем объект схемы сети
				scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (adj->parent));
				// Если данных достаточно для записи в сокет
				if(size >= adj->_marker.write.min){
					// Количество полученных байт
					int64_t bytes = -1;
					// Cмещение в буфере и отправляемый размер данных
					size_t offset = 0, actual = 0, left = 0;
					// Получаем максимальный размер буфера
					int64_t max = adj->_ectx.buffer(engine_t::method_t::WRITE);
					// Если максимальное установленное значение больше размеров буфера для записи, корректируем
					max = ((max > 0) && (adj->_marker.write.max > max) ? max : adj->_marker.write.max);
					// Активируем ожидание записи данных
					this->enabled(engine_t::method_t::WRITE, bid);
					// Выполняем отправку данных пока всё не отправим
					while((size - offset) > 0){
						// Получаем общий размер буфера данных
						left = (size - offset);
						// Определяем размер отправляемых данных
						actual = (left >= max ? max : left);
						// Выполняем отправку сообщения клиенту
						bytes = adj->_ectx.write(buffer + offset, actual);
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
					// Останавливаем ожидание записи данных
					this->disabled(engine_t::method_t::WRITE, bid);
					// Если функция обратного вызова на запись данных установлена
					if(shm->callback.is("write"))
						// Выводим функцию обратного вызова
						shm->callback.call <const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *> ("write", buffer, offset, bid, shm->sid, reinterpret_cast <awh::core_t *> (this));
				// Если данных недостаточно для записи в сокет
				} else {
					// Останавливаем ожидание записи данных
					this->disabled(engine_t::method_t::WRITE, bid);
					// Если функция обратного вызова на запись данных установлена
					if(shm->callback.is("write"))
						// Выводим функцию обратного вызова
						shm->callback.call <const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *> ("write", nullptr, 0, bid, shm->sid, reinterpret_cast <awh::core_t *> (this));
				}
				// Если тип сокета установлен как UDP, и данных для записи больше нет, запускаем чтение
				if((this->_settings.sonet == scheme_t::sonet_t::UDP) && (this->_brokers.count(bid) > 0))
					// Запускаем событие на чтение базы событий
					adj->_bev.events.read.start();
			}
		}
	}
}
/**
 * timeout Метод вызова при срабатывании таймаута
 * @param bid идентификатор брокера
 */
void awh::server::Core::timeout(const uint64_t bid) noexcept {
	// Выполняем извлечение брокера
	auto it = this->_brokers.find(bid);
	// Если брокер получен
	if(it != this->_brokers.end()){
		// Получаем объект брокера
		awh::scheme_t::broker_t * adj = const_cast <awh::scheme_t::broker_t *> (it->second);
		// Получаем объект схемы сети
		scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (adj->parent));
		// Определяем тип протокола подключения
		switch(static_cast <uint8_t> (this->_settings.family)){
			// Если тип протокола подключения IPv4
			case static_cast <uint8_t> (scheme_t::family_t::IPV4):
			// Если тип протокола подключения IPv6
			case static_cast <uint8_t> (scheme_t::family_t::IPV6): {
				// Выводим сообщение в лог, о таймауте подключения
				this->_log->print("Timeout host = %s, mac = %s", log_t::flag_t::WARNING, adj->_ip.c_str(), adj->_mac.c_str());
				// Если функция обратного вызова установлена
				if(this->_callback.is("error"))
					// Выполняем функцию обратного вызова
					this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::WARNING, error_t::TIMEOUT, this->_fmk->format("Timeout host = %s, mac = %s", adj->_ip.c_str(), adj->_mac.c_str()));
			} break;
			// Если тип протокола подключения unix-сокет
			case static_cast <uint8_t> (scheme_t::family_t::NIX): {
				// Выводим сообщение в лог, о таймауте подключения
				this->_log->print("Timeout host %s", log_t::flag_t::WARNING, this->_settings.filename.c_str());
				// Если функция обратного вызова установлена
				if(this->_callback.is("error"))
					// Выполняем функцию обратного вызова
					this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::WARNING, error_t::TIMEOUT, this->_fmk->format("Timeout host %s", this->_settings.filename.c_str()));
			} break;
		}
		// Останавливаем чтение данных
		this->disabled(engine_t::method_t::READ, bid);
		// Останавливаем запись данных
		this->disabled(engine_t::method_t::WRITE, bid);
		// Выполняем отключение клиента
		this->close(bid);
	}
}
/**
 * resolving Метод получения IP адреса доменного имени
 * @param sid    идентификатор схемы сети
 * @param ip     адрес интернет-подключения
 * @param family тип интернет-протокола AF_INET, AF_INET6
 */
void awh::server::Core::resolving(const uint16_t sid, const string & ip, const int family) noexcept {
	// Если идентификатор схемы сети передан
	if(sid > 0){
		// Выполняем поиск идентификатора схемы сети
		auto it = this->_schemes.find(sid);
		// Если идентификатор схемы сети найден, устанавливаем максимальное количество одновременных подключений
		if(it != this->_schemes.end()){
			// Получаем объект схемы сети
			scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (it->second));
			// Если IP адрес получен
			if(!ip.empty()){
				// sudo lsof -i -P | grep 1080
				// Обновляем хост сервера
				shm->_host = ip;
				// Определяем тип сокета
				switch(static_cast <uint8_t> (this->_settings.sonet)){
					// Если тип сокета установлен как UDP
					case static_cast <uint8_t> (scheme_t::sonet_t::UDP): {
						// Если разрешено выводить информационные сообщения
						if(!this->_noinfo){
							// Если unix-сокет используется
							if(this->_settings.family == scheme_t::family_t::NIX)
								// Выводим информацию о запущенном сервере на unix-сокете
								this->_log->print("Start server [%s]", log_t::flag_t::INFO, this->_settings.filename.c_str());
							// Если unix-сокет не используется, выводим сообщение о запущенном сервере за порту
							else this->_log->print("Start server [%s:%u]", log_t::flag_t::INFO, shm->_host.c_str(), shm->_port);
						}
						// Если операционная система является Windows или количество процессов всего один
						if(this->_cluster.count(shm->sid) == 1)
							// Выполняем активацию сервера
							this->accept(1, shm->sid);
						// Выполняем запуск кластера
						else this->_cluster.start(shm->sid);
						// Выходим из функции
						return;
					} break;
					// Для всех остальных типов сокетов
					default: {
						// Определяем тип протокола подключения
						switch(static_cast <uint8_t> (this->_settings.family)){
							// Если тип протокола подключения IPv4
							case static_cast <uint8_t> (scheme_t::family_t::IPV4): {
								// Выполняем перебор всего списка адресов
								for(auto & host : this->_settings.net.first){
									// Если хост соответствует адресу IPv4
									if(this->_net.host(host) == net_t::type_t::IPV4)
										// Выполняем установку полученного хоста
										shm->_addr.network.push_back(host);
								}
							} break;
							// Если тип протокола подключения IPv6
							case static_cast <uint8_t> (scheme_t::family_t::IPV6): {
								// Выполняем перебор всего списка адресов
								for(auto & host : this->_settings.net.first){
									// Если хост соответствует адресу IPv4
									if(this->_net.host(host) == net_t::type_t::IPV6)
										// Выполняем установку полученного хоста
										shm->_addr.network.push_back(host);
								}
							} break;
						}
						// Определяем тип сокета
						switch(static_cast <uint8_t> (this->_settings.sonet)){
							// Если тип сокета установлен как UDP TLS
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
							shm->_addr.init(this->_settings.filename, engine_t::type_t::SERVER);
						// Если unix-сокет не используется, выполняем инициализацию сокета
						else shm->_addr.init(shm->_host, shm->_port, (this->_settings.family == scheme_t::family_t::IPV6 ? AF_INET6 : AF_INET), engine_t::type_t::SERVER, this->_ipV6only);
						// Если сокет подключения получен
						if((shm->_addr.fd != INVALID_SOCKET) && (shm->_addr.fd < MAX_SOCKETS)){
							// Если повесить прослушку на порт не вышло, выходим из условия
							if(!shm->_addr.list()) break;
							// Если разрешено выводить информационные сообщения
							if(!this->_noinfo){
								// Если unix-сокет используется
								if(this->_settings.family == scheme_t::family_t::NIX)
									// Выводим информацию о запущенном сервере на unix-сокете
									this->_log->print("Start server [%s]", log_t::flag_t::INFO, this->_settings.filename.c_str());
								// Если unix-сокет не используется, выводим сообщение о запущенном сервере за порту
								else this->_log->print("Start server [%s:%u]", log_t::flag_t::INFO, shm->_host.c_str(), shm->_port);
							}
							// Если операционная система является Windows или количество процессов всего один
							if(this->_cluster.count(shm->sid) == 1){
								// Устанавливаем базу данных событий
								shm->_event.set(this->_dispatch.base);
								// Устанавливаем тип события
								shm->_event.set(shm->_addr.fd, EV_READ | EV_PERSIST);
								// Устанавливаем функцию обратного вызова
								shm->_event.set(std::bind(&scheme_t::accept, shm, _1, _2));
								// Выполняем запуск работы события
								shm->_event.start();
							// Выполняем запуск кластера
							} else this->_cluster.start(shm->sid);
							// Выходим из функции
							return;
						// Если сокет не создан, выводим в консоль информацию
						} else {
							// Если unix-сокет используется
							if(this->_settings.family == scheme_t::family_t::NIX){
								// Выводим информацию об незапущенном сервере на unix-сокете
								this->_log->print("Server cannot be started [%s]", log_t::flag_t::CRITICAL, this->_settings.filename.c_str());
								// Если функция обратного вызова установлена
								if(this->_callback.is("error"))
									// Выполняем функцию обратного вызова
									this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::CRITICAL, error_t::START, this->_fmk->format("Server cannot be started [%s]", this->_settings.filename.c_str()));
							// Если используется хост и порт
							} else {
								// Выводим сообщение об незапущенном сервере за порту
								this->_log->print("Server cannot be started [%s:%u]", log_t::flag_t::CRITICAL, shm->_host.c_str(), shm->_port);
								// Если функция обратного вызова установлена
								if(this->_callback.is("error"))
									// Выполняем функцию обратного вызова
									this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::CRITICAL, error_t::START, this->_fmk->format("Server cannot be started [%s:%u]", shm->_host.c_str(), shm->_port));
							}
						}
					}
				}
			// Если IP адрес сервера не получен
			} else {
				// Выводим в консоль информацию
				this->_log->print("Broken host server %s", log_t::flag_t::CRITICAL, shm->_host.c_str());
				// Если функция обратного вызова установлена
				if(this->_callback.is("error"))
					// Выполняем функцию обратного вызова
					this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::CRITICAL, error_t::START, this->_fmk->format("Broken host server %s", shm->_host.c_str()));
			}
			// Останавливаем работу сервера
			this->stop();
		}
	}
}
/**
 * bandWidth Метод установки пропускной способности сети
 * @param bid   идентификатор брокера
 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
 */
void awh::server::Core::bandWidth(const uint64_t bid, const string & read, const string & write) noexcept {
	// Выполняем извлечение брокера
	auto it = this->_brokers.find(bid);
	// Если брокер получен
	if(it != this->_brokers.end()){
		/**
		 * Если операционной системой является Nix-подобная
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Получаем объект брокера
			awh::scheme_t::broker_t * adj = const_cast <awh::scheme_t::broker_t *> (it->second);
			// Устанавливаем размер буфера
			adj->_ectx.buffer(
				(!read.empty() ? this->_fmk->sizeBuffer(read) : 0),
				(!write.empty() ? this->_fmk->sizeBuffer(write) : 0),
				reinterpret_cast <const scheme_t *> (adj->parent)->_total
			);
		/**
		 * Если операционной системой является MS Windows
		 */
		#else
			// Блокируем вывод переменных
			(void) read;
			(void) write;
		#endif
	}
}
/**
 * clusterSize Метод установки количества процессов кластера
 * @param size количество рабочих процессов
 */
void awh::server::Core::clusterSize(const uint16_t size) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.main);
		// Устанавливаем количество рабочих процессов кластера
		this->_clusterSize = size;
	/**
	 * Если операционной системой является Windows
	 */
	#else
		// Выводим предупредительное сообщение в лог
		this->_log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
		// Если функция обратного вызова установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::WARNING, error_t::OS_BROKEN, "MS Windows OS, does not support cluster mode");
	#endif
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
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::WARNING, error_t::OS_BROKEN, "MS Windows OS, does not support cluster mode");
	#endif
}
/**
 * ipV6only Метод установки флага использования только сети IPv6
 * @param mode флаг для установки
 */
void awh::server::Core::ipV6only(const bool mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Устанавливаем флаг использования только сети IPv6
	this->_ipV6only = mode;
}
/**
 * total Метод установки максимального количества одновременных подключений
 * @param sid   идентификатор схемы сети
 * @param total максимальное количество одновременных подключений
 */
void awh::server::Core::total(const uint16_t sid, const u_short total) noexcept {
	// Если идентификатор схемы сети передан
	if(sid > 0){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.main);
		// Выполняем поиск идентификатора схемы сети
		auto it = this->_schemes.find(sid);
		// Если идентификатор схемы сети найден, устанавливаем максимальное количество одновременных подключений
		if(it != this->_schemes.end())
			// Устанавливаем максимальное количество одновременных подключений
			(dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (it->second)))->_total = total;
	}
}
/**
 * init Метод инициализации сервера
 * @param sid  идентификатор схемы сети
 * @param port порт сервера
 * @param host хост сервера
 */
void awh::server::Core::init(const uint16_t sid, const u_int port, const string & host) noexcept {
	// Если идентификатор схемы сети передан
	if(sid > 0){
		// Выполняем поиск идентификатора схемы сети
		auto it = this->_schemes.find(sid);
		// Если идентификатор схемы сети найден, устанавливаем максимальное количество одновременных подключений
		if(it != this->_schemes.end()){
			// Выполняем блокировку потока
			const lock_guard <recursive_mutex> lock(this->_mtx.main);
			// Получаем объект схемы сети
			scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (it->second));
			// Если порт передан, устанавливаем
			if(port > 0)
				// Устанавливаем порт
				shm->_port = port;
			// Если хост передан, устанавливаем
			if(!host.empty())
				// Устанавливаем хост
				shm->_host = host;
			// Иначе получаем IP адрес сервера автоматически
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
 * on Метод установки функции обратного вызова при краше приложения
 * @param callback функция обратного вызова для установки
 */
void awh::server::Core::on(function <void (const int)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	awh::core_t::on(callback);
}
/**
 * on Метод установки функции обратного вызова при запуске/остановки работы модуля
 * @param callback функция обратного вызова для установки
 */
void awh::server::Core::on(function <void (const status_t, awh::core_t *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	awh::core_t::on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибки
 * @param callback функция обратного вызова
 */
void awh::server::Core::on(function <void (const log_t::flag_t, const error_t, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	awh::core_t::on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие запуска и остановки процессов кластера
 * @param callback функция обратного вызова
 */
void awh::server::Core::on(function <void (const cluster_t::family_t, const uint16_t, const pid_t, const cluster_t::event_t, awh::core_t *)> callback) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const cluster_t::family_t, const uint16_t, const pid_t, const cluster_t::event_t, Core *)> ("cluster", callback);
}
/**
 * Core Конструктор
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
 * @param sonet  тип сокета подключения (TCP / UDP)
 */
awh::server::Core::Core(const fmk_t * fmk, const log_t * log, const scheme_t::family_t family, const scheme_t::sonet_t sonet) noexcept :
 awh::core_t(fmk, log, family, sonet), _pid(0), _cluster(fmk, log), _ipV6only(false), _clusterSize(1), _clusterAutoRestart(false) {
	// Устанавливаем тип запускаемого ядра
	this->_type = engine_t::type_t::SERVER;
	// Устанавливаем функцию получения статуса кластера
	this->_cluster.on(std::bind(&core_t::cluster, this, _1, _2, _3));
}
/**
 * ~Core Деструктор
 */
awh::server::Core::~Core() noexcept {
	// Выполняем остановку сервера
	this->stop();
}
