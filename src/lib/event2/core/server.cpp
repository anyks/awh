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
 * accept Функция подключения к серверу
 * @param fd    файловый дескриптор (сокет)
 * @param event произошедшее событие
 */
void awh::server::scheme_t::accept(const evutil_socket_t fd, const short event) noexcept {
	// Получаем объект подключения
	core_t * core = (core_t *) const_cast <awh::core_t *> (this->core);
	// Выполняем подключение клиента
	core->accept(fd, this->sid);
}
/**
 * callback Функция обратного вызова
 * @param fd    файловый дескриптор (сокет)
 * @param event произошедшее событие
 */
void awh::server::Core::DTLS::callback(const evutil_socket_t fd, const short event) noexcept {
	// Останавливаем работу таймера
	this->event.stop();
	// Выполняем извлечение адъютанта
	auto it = this->core->adjutants.find(this->aid);
	// Если адъютант получен
	if(it != this->core->adjutants.end()){
		// Получаем объект адъютанта
		awh::scheme_t::adj_t * adj = const_cast <awh::scheme_t::adj_t *> (it->second);
		// Получаем объект схемы сети
		scheme_t * shm = (scheme_t *) const_cast <awh::scheme_t *> (adj->parent);
		// Выполняем ожидание входящих подключений
		if(this->core->engine.wait(adj->ectx)){
			// Устанавливаем параметры сокета
			adj->addr.sonet(SOCK_DGRAM, IPPROTO_UDP);
			// Если прикрепление клиента к серверу выполнено
			if(adj->addr.attach(shm->addr)){
				// Выполняем прикрепление контекста клиента к контексту сервера
				this->core->engine.attach(adj->ectx, &adj->addr);
				// Получаем адрес подключения клиента
				adj->ip = adj->addr.ip;
				// Получаем аппаратный адрес клиента
				adj->mac = adj->addr.mac;
				// Получаем порт подключения клиента
				adj->port = adj->addr.port;
				// Если функция обратного вызова проверки подключения установлена, выполняем проверку, если проверка не пройдена?
				if((shm->callback.accept != nullptr) && !shm->callback.accept(adj->ip, adj->mac, adj->port, shm->sid, this->core)){
					// Если порт установлен
					if(adj->port > 0){
						// Выводим сообщение об ошибке
						this->core->log->print(
							"access to the server is denied for the client [%s:%d], mac = %s, socket = %d, pid = %d",
							log_t::flag_t::WARNING,
							adj->ip.c_str(),
							adj->port,
							adj->mac.c_str(),
							adj->addr.fd,
							getpid()
						);
					// Если порт не установлен
					} else {
						// Выводим сообщение об ошибке
						this->core->log->print(
							"access to the server is denied for the client [%s], mac = %s, socket = %d, pid = %d",
							log_t::flag_t::WARNING,
							adj->ip.c_str(),
							adj->mac.c_str(),
							adj->addr.fd,
							getpid()
						);
					}
					// Выполняем отключение адъютанта
					this->core->close(this->aid);
					// Выходим
					return;
				}
				// Запускаем чтение данных
				this->core->enabled(engine_t::method_t::READ, this->aid);
				// Если вывод информационных данных не запрещён
				if(!this->core->noinfo){
					// Если порт установлен
					if(adj->port > 0){
						// Выводим в консоль информацию
						this->core->log->print(
							"connect to server client [%s:%d], mac = %s, socket = %d, pid = %d",
							log_t::flag_t::INFO,
							adj->ip.c_str(),
							adj->port,
							adj->mac.c_str(),
							adj->addr.fd, getpid()
						);
					// Если порт не установлен
					} else {
						// Выводим в консоль информацию
						this->core->log->print(
							"connect to server client [%s], mac = %s, socket = %d, pid = %d",
							log_t::flag_t::INFO,
							adj->ip.c_str(),
							adj->mac.c_str(),
							adj->addr.fd, getpid()
						);
					}
				}
				// Выполняем функцию обратного вызова
				if(shm->callback.connect != nullptr) shm->callback.connect(this->aid, shm->sid, this->core);
			// Подключение не установлено, выводим сообщение об ошибке
			} else this->core->log->print("accepting failed, pid = %d", log_t::flag_t::WARNING, getpid());
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
void awh::server::Core::cluster(const size_t sid, const pid_t pid, const cluster_t::event_t event) noexcept {
	// Выполняем поиск идентификатора схемы сети
	auto it = this->schemes.find(sid);
	// Если идентификатор схемы сети найден, устанавливаем максимальное количество одновременных подключений
	if(it != this->schemes.end()){
		// Получаем объект подключения
		scheme_t * shm = (scheme_t *) const_cast <awh::scheme_t *> (it->second);
		// Выполняем тип возникшего события
		switch((uint8_t) event){
			// Если производится запуск процесса
			case (uint8_t) cluster_t::event_t::START: {
				// Если процесс является дочерним
				if(this->pid != getpid()){
					// Запоминаем текущий идентификатор процесса
					this->_pid = pid;
					// Определяем тип сокета
					switch((uint8_t) this->net.sonet){
						// Если тип сокета установлен как UDP
						case (uint8_t) scheme_t::sonet_t::UDP:
							// Выполняем активацию сервера
							this->accept(1, it->first);
						break;
						// Для всех остальных типов сокетов
						default: {
							// Устанавливаем базу данных событий
							shm->event.set(this->dispatch.base);
							// Устанавливаем тип события
							shm->event.set(shm->addr.fd, EV_READ | EV_PERSIST);
							// Устанавливаем функцию обратного вызова
							shm->event.set(std::bind(&scheme_t::accept, shm, _1, _2));
							// Выполняем запуск работы события
							shm->event.start();
						}
					}
				}
			} break;
			// Если производится остановка процесса
			case (uint8_t) cluster_t::event_t::STOP: {
				// Если тип сокета не установлен как UDP
				if(this->net.sonet != scheme_t::sonet_t::UDP)
					// Останавливаем чтение данных с клиента
					shm->event.stop();
			} break;
		}
	}
}
/**
 * accept Функция подключения к серверу
 * @param fd  файловый дескриптор (сокет) подключившегося клиента
 * @param sid идентификатор схемы сети
 */
void awh::server::Core::accept(const int fd, const size_t sid) noexcept {
	// Если идентификатор схемы сети передан
	if((sid > 0) && (fd >= 0)){
		// Выполняем поиск идентификатора схемы сети
		auto it = this->schemes.find(sid);
		// Если идентификатор схемы сети найден, устанавливаем максимальное количество одновременных подключений
		if(it != this->schemes.end()){
			// Получаем объект подключения
			scheme_t * shm = (scheme_t *) const_cast <awh::scheme_t *> (it->second);
			// Определяем тип сокета
			switch((uint8_t) this->net.sonet){
				// Если тип сокета установлен как UDP
				case (uint8_t) scheme_t::sonet_t::UDP: {
					// Создаём бъект адъютанта
					unique_ptr <awh::scheme_t::adj_t> adj(new awh::scheme_t::adj_t(shm, this->fmk, this->log));
					// Определяем тип протокола подключения
					switch((uint8_t) this->net.family){
						// Если тип протокола подключения IPv4
						case (uint8_t) scheme_t::family_t::IPV4:
							// Устанавливаем сеть, для выхода в интернет
							adj->addr.network.assign(
								this->net.v4.first.begin(),
								this->net.v4.first.end()
							);
						break;
						// Если тип протокола подключения IPv6
						case (uint8_t) scheme_t::family_t::IPV6:
							// Устанавливаем сеть, для выхода в интернет
							adj->addr.network.assign(
								this->net.v6.first.begin(),
								this->net.v6.first.end()
							);
						break;
					}
					// Устанавливаем параметры сокета
					adj->addr.sonet(SOCK_DGRAM, IPPROTO_UDP);
					// Если unix-сокет используется
					if(this->net.family == scheme_t::family_t::NIX)
						// Выполняем инициализацию сокета
						adj->addr.init(this->net.filename, engine_t::type_t::SERVER);
					// Если unix-сокет не используется, выполняем инициализацию сокета
					else adj->addr.init(shm->host, shm->port, (this->net.family == scheme_t::family_t::IPV6 ? AF_INET6 : AF_INET), engine_t::type_t::SERVER, this->_ipV6only);
					// Выполняем разрешение подключения
					if(adj->addr.accept(adj->addr.fd, 0)){
						// Получаем адрес подключения клиента
						adj->ip = adj->addr.ip;
						// Получаем аппаратный адрес клиента
						adj->mac = adj->addr.mac;
						// Получаем порт подключения клиента
						adj->port = adj->addr.port;
						// Устанавливаем идентификатор адъютанта
						adj->aid = this->fmk->nanoTimestamp();
						// Выполняем получение контекста сертификата
						this->engine.wrapServer(adj->ectx, &adj->addr);
						// Если подключение не обёрнуто
						if(adj->addr.fd < 0){
							// Выводим сообщение об ошибке
							this->log->print("wrap engine context is failed", log_t::flag_t::CRITICAL);
							// Выходим из функции
							return;
						}
						// Выполняем блокировку потока
						this->_mtx.accept.lock();
						// Добавляем созданного адъютанта в список адъютантов
						auto ret = shm->adjutants.emplace(adj->aid, move(adj));
						// Добавляем адъютанта в список подключений
						this->adjutants.emplace(ret.first->first, ret.first->second.get());
						// Выполняем блокировку потока
						this->_mtx.accept.unlock();
						// Запускаем чтение данных
						this->enabled(engine_t::method_t::READ, ret.first->first);
						// Выполняем функцию обратного вызова
						if(shm->callback.connect != nullptr) shm->callback.connect(ret.first->first, shm->sid, this);
					// Подключение не установлено, выводим сообщение об ошибке
					} else this->log->print("accepting failed, pid = %d", log_t::flag_t::WARNING, getpid());
				} break;
				// Если подключение зашифрованно
				case (uint8_t) scheme_t::sonet_t::DTLS: {
					// Если количество подключившихся клиентов, больше максимально-допустимого количества клиентов
					if(shm->adjutants.size() >= (size_t) shm->total){
						// Выводим в консоль информацию
						this->log->print("the number of simultaneous connections, cannot exceed the maximum allowed number of %d", log_t::flag_t::WARNING, shm->total);
						// Выходим
						break;
					}
					// Создаём объект для работы с DTLS
					unique_ptr <dtls_t> dtls(new dtls_t());
					// Создаём бъект адъютанта
					unique_ptr <awh::scheme_t::adj_t> adj(new awh::scheme_t::adj_t(shm, this->fmk, this->log));
					// Устанавливаем идентификатор адъютанта
					adj->aid = this->fmk->nanoTimestamp();
					// Устанавливаем объект сетевого ядра
					dtls->core = this;
					// Устанавливаем идентификатор адъютанта
					dtls->aid = adj->aid;
					// Устанавливаем тип таймера
					dtls->event.set(-1, EV_TIMEOUT);
					// Устанавливаем базу данных событий
					dtls->event.set(this->dispatch.base);
					// Устанавливаем функцию обратного вызова
					dtls->event.set(std::bind(&dtls_t::callback, dtls.get(), _1, _2));
					// Выполняем получение контекста сертификата
					this->engine.wrap(adj->ectx, &shm->addr, engine_t::type_t::SERVER);
					// Выполняем блокировку потока
					this->_mtx.accept.lock();
					// Добавляем созданного адъютанта в список адъютантов
					auto ret = shm->adjutants.emplace(adj->aid, move(adj));
					// Добавляем адъютанта в список подключений
					this->adjutants.emplace(ret.first->first, ret.first->second.get());
					// Добавляем объект для работы с DTLS в список
					this->_dtls.emplace(ret.first->first, move(dtls)).first->second->event.start(100);
					// Выполняем блокировку потока
					this->_mtx.accept.unlock();
					// Останавливаем работу сервера
					shm->event.stop();
				} break;
				// Если тип сокета установлен как TCP/IP
				case (uint8_t) scheme_t::sonet_t::TCP:
				// Если тип сокета установлен как TCP/IP TLS
				case (uint8_t) scheme_t::sonet_t::TLS:
				// Если тип сокета установлен как SCTP DTLS
				case (uint8_t) scheme_t::sonet_t::SCTP: {
					// Если количество подключившихся клиентов, больше максимально-допустимого количества клиентов
					if(shm->adjutants.size() >= (size_t) shm->total){
						// Выводим в консоль информацию
						this->log->print("the number of simultaneous connections, cannot exceed the maximum allowed number of %d", log_t::flag_t::WARNING, shm->total);
						// Выходим
						break;
					}
					// Создаём бъект адъютанта
					unique_ptr <awh::scheme_t::adj_t> adj(new awh::scheme_t::adj_t(shm, this->fmk, this->log));
					// Устанавливаем время жизни подключения
					adj->addr.alive = shm->keepAlive;
					// Определяем тип сокета
					switch((uint8_t) this->net.sonet){
						/**
						 * Если операционной системой является Linux или FreeBSD
						 */
						#if defined(__linux__) || defined(__FreeBSD__)
							// Если тип сокета установлен как SCTP
							case (uint8_t) scheme_t::sonet_t::SCTP:
								// Устанавливаем параметры сокета
								adj->addr.sonet(SOCK_STREAM, IPPROTO_SCTP);
							break;
						#endif
						// Для всех остальных типов сокетов
						default:
							// Устанавливаем параметры сокета
							adj->addr.sonet(SOCK_STREAM, IPPROTO_TCP);
					}
					// Выполняем разрешение подключения
					if(adj->addr.accept(shm->addr)){
						// Получаем адрес подключения клиента
						adj->ip = adj->addr.ip;
						// Получаем аппаратный адрес клиента
						adj->mac = adj->addr.mac;
						// Получаем порт подключения клиента
						adj->port = adj->addr.port;
						// Если функция обратного вызова проверки подключения установлена, выполняем проверку, если проверка не пройдена?
						if((shm->callback.accept != nullptr) && !shm->callback.accept(adj->ip, adj->mac, adj->port, shm->sid, this)){
							// Если порт установлен
							if(adj->port > 0){
								// Выводим сообщение об ошибке
								this->log->print(
									"access to the server is denied for the client [%s:%d], mac = %s, socket = %d, pid = %d",
									log_t::flag_t::WARNING,
									adj->ip.c_str(),
									adj->port,
									adj->mac.c_str(),
									adj->addr.fd,
									getpid()
								);
							// Если порт не установлен
							} else {
								// Выводим сообщение об ошибке
								this->log->print(
									"access to the server is denied for the client [%s], mac = %s, socket = %d, pid = %d",
									log_t::flag_t::WARNING,
									adj->ip.c_str(),
									adj->mac.c_str(),
									adj->addr.fd,
									getpid()
								);
							}
							// Выходим
							break;
						}
						// Устанавливаем идентификатор адъютанта
						adj->aid = this->fmk->nanoTimestamp();
						// Выполняем получение контекста сертификата
						this->engine.wrapServer(adj->ectx, &adj->addr);
						// Если мы хотим работать в зашифрованном режиме
						if(this->net.sonet == scheme_t::sonet_t::TLS){
							// Если сертификаты не приняты, выходим
							if(!this->engine.isTLS(adj->ectx)){
								// Выводим сообщение об ошибке
								this->log->print("encryption mode cannot be activated", log_t::flag_t::CRITICAL);
								// Выходим
								break;
							}
						}
						// Если подключение не обёрнуто
						if(adj->addr.fd < 0){
							// Выводим сообщение об ошибке
							this->log->print("wrap engine context is failed", log_t::flag_t::CRITICAL);
							// Выходим
							break;
						}
						// Выполняем блокировку потока
						this->_mtx.accept.lock();
						// Добавляем созданного адъютанта в список адъютантов
						auto ret = shm->adjutants.emplace(adj->aid, move(adj));
						// Добавляем адъютанта в список подключений
						this->adjutants.emplace(ret.first->first, ret.first->second.get());
						// Выполняем блокировку потока
						this->_mtx.accept.unlock();
						// Запускаем чтение данных
						this->enabled(engine_t::method_t::READ, ret.first->first);
						// Если вывод информационных данных не запрещён
						if(!this->noinfo){
							// Если порт установлен
							if(ret.first->second->port > 0){
								// Выводим в консоль информацию
								this->log->print(
									"connect to server client [%s:%d], mac = %s, socket = %d, pid = %d",
									log_t::flag_t::INFO,
									ret.first->second->ip.c_str(),
									ret.first->second->port,
									ret.first->second->mac.c_str(),
									ret.first->second->addr.fd, getpid()
								);
							// Если порт не установлен
							} else {
								// Выводим в консоль информацию
								this->log->print(
									"connect to server client [%s], mac = %s, socket = %d, pid = %d",
									log_t::flag_t::INFO,
									ret.first->second->ip.c_str(),
									ret.first->second->mac.c_str(),
									ret.first->second->addr.fd, getpid()
								);
							}
						}
						// Выполняем функцию обратного вызова
						if(shm->callback.connect != nullptr) shm->callback.connect(ret.first->first, shm->sid, this);
					// Если подключение не установлено, выводим сообщение об ошибке
					} else this->log->print("accepting failed, pid = %d", log_t::flag_t::WARNING, getpid());
				} break;
			}
		}
	}
}
/**
 * close Метод отключения всех адъютантов
 */
void awh::server::Core::close() noexcept {
	// Если список схем сети активен
	if(!this->schemes.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.close);
		// Переходим по всему списку схем сети
		for(auto & item : this->schemes){
			// Получаем объект схемы сети
			scheme_t * shm = (scheme_t *) const_cast <awh::scheme_t *> (item.second);
			// Если в схеме сети есть подключённые клиенты
			if(!shm->adjutants.empty()){
				// Переходим по всему списку адъютанта
				for(auto it = shm->adjutants.begin(); it != shm->adjutants.end();){
					// Если блокировка адъютанта не установлена
					if(this->_locking.count(it->first) < 1){
						// Выполняем блокировку адъютанта
						this->_locking.emplace(it->first);
						// Получаем объект адъютанта
						awh::scheme_t::adj_t * adj = const_cast <awh::scheme_t::adj_t *> (it->second.get());
						// Выполняем очистку буфера событий
						this->clean(it->first);
						// Выполняем очистку контекста двигателя
						adj->ectx.clear();
						// Удаляем адъютанта из списка подключений
						this->adjutants.erase(it->first);
						// Выводим функцию обратного вызова
						if(shm->callback.disconnect != nullptr)
							// Выполняем функцию обратного вызова
							shm->callback.disconnect(it->first, item.first, this);
						// Если список объектов DTLS не пустой
						if(!this->_dtls.empty())
							// Удаляем объект для работы DTLS из списка
							this->_dtls.erase(it->first);
						// Удаляем блокировку адъютанта
						this->_locking.erase(it->first);
						// Удаляем адъютанта из списка
						it = shm->adjutants.erase(it);
					// Иначе продолжаем дальше
					} else ++it;
				}
				// Останавливаем работу кластера
				this->_cluster.stop(shm->sid);
			}
			// Останавливаем работу сервера
			shm->event.stop();
			// Выполняем закрытие подключение сервера
			shm->addr.close();
		}
	}
}
/**
 * remove Метод удаления всех активных схем сети
 */
void awh::server::Core::remove() noexcept {
	// Если список схем сети активен
	if(!this->schemes.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.close);
		// Переходим по всему списку схем сети
		for(auto it = this->schemes.begin(); it != this->schemes.end();){
			// Получаем объект схемы сети
			scheme_t * shm = (scheme_t *) const_cast <awh::scheme_t *> (it->second);
			// Если в схеме сети есть подключённые клиенты
			if(!shm->adjutants.empty()){
				// Переходим по всему списку адъютанта
				for(auto jt = shm->adjutants.begin(); jt != shm->adjutants.end();){
					// Если блокировка адъютанта не установлена
					if(this->_locking.count(jt->first) < 1){
						// Выполняем блокировку адъютанта
						this->_locking.emplace(jt->first);
						// Получаем объект адъютанта
						awh::scheme_t::adj_t * adj = const_cast <awh::scheme_t::adj_t *> (jt->second.get());
						// Выполняем очистку буфера событий
						this->clean(jt->first);
						// Выполняем очистку контекста двигателя
						adj->ectx.clear();
						// Удаляем адъютанта из списка подключений
						this->adjutants.erase(jt->first);
						// Выводим функцию обратного вызова
						if(shm->callback.disconnect != nullptr)
							// Выполняем функцию обратного вызова
							shm->callback.disconnect(jt->first, it->first, this);
						// Если список объектов DTLS не пустой
						if(!this->_dtls.empty())
							// Удаляем объект для работы DTLS из списка
							this->_dtls.erase(jt->first);
						// Удаляем блокировку адъютанта
						this->_locking.erase(jt->first);
						// Удаляем адъютанта из списка
						jt = shm->adjutants.erase(jt);
					// Иначе продолжаем дальше
					} else ++jt;
				}
				// Останавливаем работу кластера
				this->_cluster.stop(shm->sid);
			}
			// Останавливаем работу сервера
			shm->event.stop();
			// Выполняем закрытие подключение сервера
			shm->addr.close();
			// Выполняем удаление схемы сети
			it = this->schemes.erase(it);
		}
	}
}
/**
 * run Метод запуска сервера
 * @param sid идентификатор схемы сети
 */
void awh::server::Core::run(const size_t sid) noexcept {
	// Если идентификатор схемы сети передан
	if(sid > 0){		
		// Выполняем поиск идентификатора схемы сети
		auto it = this->schemes.find(sid);
		// Если идентификатор схемы сети найден, устанавливаем максимальное количество одновременных подключений
		if(it != this->schemes.end()){			
			// Устанавливаем базу событий кластера
			this->_cluster.base(this->dispatch.base);
			// Выполняем инициализацию кластера
			this->_cluster.init(it->first, this->_clusterSize);
			// Устанавливаем флаг автоматического перезапуска упавших процессов
			this->_cluster.restart(it->first, this->_clusterAutoRestart);
			// Получаем объект схемы сети
			scheme_t * shm = (scheme_t *) const_cast <awh::scheme_t *> (it->second);
			// Если хост сервера не указан
			if(shm->host.empty()){
				// Объект для работы с сетевым интерфейсом
				ifnet_t ifnet(this->fmk, this->log);
				// Определяем тип протокола подключения
				switch((uint8_t) this->net.family){
					// Если тип протокола подключения unix-сокет
					case (uint8_t) scheme_t::family_t::NIX:
					// Если тип протокола подключения IPv4
					case (uint8_t) scheme_t::family_t::IPV4: {
						// Обновляем хост сервера
						shm->host = ifnet.ip(AF_INET);
						// Выполняем запуск сервера
						this->resolving(shm->sid, shm->host, AF_INET, 0);
					} break;
					// Если тип протокола подключения IPv6
					case (uint8_t) scheme_t::family_t::IPV6: {
						// Обновляем хост сервера
						shm->host = ifnet.ip(AF_INET6);
						// Выполняем запуск сервера
						this->resolving(shm->sid, shm->host, AF_INET6, 0);
					} break;
				}
			// Если хост сервера является доменным именем
			} else {			
				// Устанавливаем событие на получение данных с DNS сервера
				this->dns.on(std::bind(&scheme_t::resolving, shm, _1, _2, _3));
				// Определяем тип протокола подключения
				switch((uint8_t) this->net.family){
					// Если тип протокола подключения unix-сокет
					case (uint8_t) scheme_t::family_t::NIX:
					// Если тип протокола подключения IPv4
					case (uint8_t) scheme_t::family_t::IPV4:
						// Выполняем резолвинг домена
						this->dns.resolve(shm->host, AF_INET);
					break;
					// Если тип протокола подключения IPv6
					case (uint8_t) scheme_t::family_t::IPV6:
						// Выполняем резолвинг домена
						this->dns.resolve(shm->host, AF_INET6);
					break;
				}
			}
		}
	}
}
/**
 * remove Метод удаления схемы сети
 * @param sid идентификатор схемы сети
 */
void awh::server::Core::remove(const size_t sid) noexcept {
	// Если идентификатор схемы сети передан
	if(sid > 0){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.close);
		// Выполняем поиск идентификатора схемы сети
		auto it = this->schemes.find(sid);
		// Если идентификатор схемы сети найден, устанавливаем максимальное количество одновременных подключений
		if(it != this->schemes.end()){
			// Получаем объект схемы сети
			scheme_t * shm = (scheme_t *) const_cast <awh::scheme_t *> (it->second);
			// Если в схеме сети есть подключённые клиенты
			if(!shm->adjutants.empty()){
				// Переходим по всему списку адъютанта
				for(auto jt = shm->adjutants.begin(); jt != shm->adjutants.end();){
					// Если блокировка адъютанта не установлена
					if(this->_locking.count(jt->first) < 1){
						// Выполняем блокировку адъютанта
						this->_locking.emplace(jt->first);
						// Получаем объект адъютанта
						awh::scheme_t::adj_t * adj = const_cast <awh::scheme_t::adj_t *> (jt->second.get());
						// Выполняем очистку буфера событий
						this->clean(jt->first);
						// Выполняем очистку контекста двигателя
						adj->ectx.clear();
						// Выводим функцию обратного вызова
						if(shm->callback.disconnect != nullptr)
							// Выполняем функцию обратного вызова
							shm->callback.disconnect(jt->first, it->first, this);
						// Удаляем адъютанта из списка подключений
						this->adjutants.erase(jt->first);
						// Если список объектов DTLS не пустой
						if(!this->_dtls.empty())
							// Удаляем объект для работы DTLS из списка
							this->_dtls.erase(jt->first);
						// Удаляем блокировку адъютанта
						this->_locking.erase(jt->first);
						// Удаляем адъютанта из списка
						jt = shm->adjutants.erase(jt);
					// Иначе продолжаем дальше
					} else ++jt;
				}
			}
			// Останавливаем работу сервера
			shm->event.stop();
			// Выполняем закрытие подключение сервера
			shm->addr.close();
			// Выполняем удаление схемы сети
			this->schemes.erase(sid);
		}
	}
}
/**
 * close Метод закрытия подключения адъютанта
 * @param aid идентификатор адъютанта
 */
void awh::server::Core::close(const size_t aid) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.close);
	// Если тип сокета установлен как не UDP, останавливаем чтение
	if(this->net.sonet != scheme_t::sonet_t::UDP){
		// Если блокировка адъютанта не установлена
		if(this->_locking.count(aid) < 1){
			// Выполняем блокировку адъютанта
			this->_locking.emplace(aid);
			// Выполняем извлечение адъютанта
			auto it = this->adjutants.find(aid);
			// Если адъютант получен
			if(it != this->adjutants.end()){
				// Получаем объект адъютанта
				awh::scheme_t::adj_t * adj = const_cast <awh::scheme_t::adj_t *> (it->second);
				// Получаем объект схемы сети
				scheme_t * shm = (scheme_t *) const_cast <awh::scheme_t *> (adj->parent);
				// Получаем объект ядра клиента
				const core_t * core = reinterpret_cast <const core_t *> (shm->core);
				// Выполняем очистку буфера событий
				this->clean(aid);
				// Выполняем очистку контекста двигателя
				adj->ectx.clear();
				// Удаляем адъютанта из списка адъютантов
				shm->adjutants.erase(aid);
				// Удаляем адъютанта из списка подключений
				this->adjutants.erase(aid);
				// Выводим сообщение об ошибке
				if(!core->noinfo) this->log->print("%s", log_t::flag_t::INFO, "disconnect client from server");
				// Выводим функцию обратного вызова
				if(shm->callback.disconnect != nullptr) shm->callback.disconnect(aid, shm->sid, this);
				// Если тип сокета установлен как DTLS, запускаем ожидание новых подключений
				if(this->net.sonet == scheme_t::sonet_t::DTLS){
					// Очищаем контекст сервера
					shm->ectx.clear();
					// Если список объектов DTLS не пустой
					if(!this->_dtls.empty())
						// Удаляем объект для работы DTLS из списка
						this->_dtls.erase(aid);
					// Выполняем запуск сервера вновь
					this->run(shm->sid);
				}
			}
			// Удаляем блокировку адъютанта
			this->_locking.erase(aid);
		}
	}
}
/**
 * timeout Функция обратного вызова при срабатывании таймаута
 * @param aid идентификатор адъютанта
 */
void awh::server::Core::timeout(const size_t aid) noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Получаем объект адъютанта
		awh::scheme_t::adj_t * adj = const_cast <awh::scheme_t::adj_t *> (it->second);
		// Получаем объект схемы сети
		scheme_t * shm = (scheme_t *) const_cast <awh::scheme_t *> (adj->parent);
		// Определяем тип протокола подключения
		switch((uint8_t) this->net.family){
			// Если тип протокола подключения IPv4
			case (uint8_t) scheme_t::family_t::IPV4:
			// Если тип протокола подключения IPv6
			case (uint8_t) scheme_t::family_t::IPV6:
				// Выводим сообщение в лог, о таймауте подключения
				this->log->print("timeout host = %s, mac = %s", log_t::flag_t::WARNING, adj->ip.c_str(), adj->mac.c_str());
			break;
			// Если тип протокола подключения unix-сокет
			case (uint8_t) scheme_t::family_t::NIX:
				// Выводим сообщение в лог, о таймауте подключения
				this->log->print("timeout host %s", log_t::flag_t::WARNING, this->net.filename.c_str());
			break;
		}
		// Останавливаем чтение данных
		this->disabled(engine_t::method_t::READ, it->first);
		// Останавливаем запись данных
		this->disabled(engine_t::method_t::WRITE, it->first);
		// Выполняем отключение клиента
		this->close(aid);
	}
}
/**
 * write Функция обратного вызова при записи данных в сокет
 * @param method метод режима работы
 * @param aid    идентификатор адъютанта
 */
void awh::server::Core::transfer(const engine_t::method_t method, const size_t aid) noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Получаем объект адъютанта
		awh::scheme_t::adj_t * adj = const_cast <awh::scheme_t::adj_t *> (it->second);
		// Получаем объект схемы сети
		scheme_t * shm = (scheme_t *) const_cast <awh::scheme_t *> (adj->parent);
		// Определяем метод работы
		switch((uint8_t) method){
			// Если производится чтение данных
			case (uint8_t) engine_t::method_t::READ: {
				// Количество полученных байт
				int64_t bytes = -1;
				// Создаём буфер входящих данных
				char buffer[BUFFER_SIZE];
				// Останавливаем чтение данных с клиента
				adj->bev.events.read.stop();
				// Выполняем перебор бесконечным циклом пока это разрешено
				while(!adj->bev.locked.read){
					// Выполняем получение сообщения от клиента
					bytes = adj->ectx.read(buffer, sizeof(buffer));
					// Если время ожидания чтения данных установлено
					if(shm->wait && (adj->timeouts.read > 0))
						// Запускаем работу таймера
						adj->bev.timers.read.start(adj->timeouts.read * 1000);
					// Останавливаем таймаут ожидания на чтение из сокета
					else adj->bev.timers.read.stop();
					// Если данные получены
					if(bytes > 0){
						// Если данные считанные из буфера, больше размера ожидающего буфера
						if((adj->marker.write.max > 0) && (bytes >= adj->marker.write.max)){
							// Смещение в буфере и отправляемый размер данных
							size_t offset = 0, actual = 0;
							// Выполняем пересылку всех полученных данных
							while((bytes - offset) > 0){
								// Определяем размер отправляемых данных
								actual = ((bytes - offset) >= adj->marker.write.max ? adj->marker.write.max : (bytes - offset));
								// Если функция обратного вызова на получение данных установлена
								if(shm->callback.read != nullptr)
									// Выводим функцию обратного вызова
									shm->callback.read(buffer + offset, actual, aid, shm->sid, reinterpret_cast <awh::core_t *> (this));
								// Увеличиваем смещение в буфере
								offset += actual;
							}
						// Если данных достаточно и функция обратного вызова на получение данных установлена
						} else if(shm->callback.read != nullptr)
							// Выводим функцию обратного вызова
							shm->callback.read(buffer, bytes, aid, shm->sid, reinterpret_cast <awh::core_t *> (this));
					// Если данные не могут быть прочитаны
					} else {
						// Если нужно повторить попытку
						if(bytes == -2) continue;
						// Если нужно выйти из цикла
						else if(bytes == -1) break;
						// Если нужно завершить работу
						else if(bytes == 0) {
							// Выполняем отключение клиента
							this->close(aid);
							// Выходим из функции
							return;
						}
					}
					// Выходим из цикла
					break;
				}
				// Если тип сокета не установлен как UDP, запускаем чтение дальше
				if((this->net.sonet != scheme_t::sonet_t::UDP) && (this->adjutants.count(aid) > 0))
					// Запускаем событие на чтение базы событий
					adj->bev.events.read.start();
			} break;
			// Если производится запись данных
			case (uint8_t) engine_t::method_t::WRITE: {
				// Останавливаем работу таймера
				adj->bev.timers.write.stop();
				// Если данных достаточно для записи в сокет
				if(adj->buffer.size() >= adj->marker.write.min){
					// Получаем буфер отправляемых данных
					vector <char> buffer;
					// Количество полученных байт
					int64_t bytes = -1;
					// Cмещение в буфере и отправляемый размер данных
					size_t offset = 0, actual = 0, size = 0;
					// Выполняем отправку данных пока всё не отправим
					while(!adj->bev.locked.write && ((adj->buffer.size() - offset) > 0)){
						// Получаем общий размер буфера данных
						size = (adj->buffer.size() - offset);
						// Определяем размер отправляемых данных
						actual = ((size >= adj->marker.write.max) ? adj->marker.write.max : size);
						// Выполняем отправку сообщения клиенту
						bytes = adj->ectx.write(adj->buffer.data() + offset, actual);
						// Если данные записаны удачно
						if(bytes > 0)
							// Добавляем записанные байты в буфер
							buffer.insert(buffer.end(), adj->buffer.data() + offset, (adj->buffer.data() + offset) + bytes);
						// Если время ожидания записи данных установлено
						if(adj->timeouts.write > 0)
							// Запускаем работу таймера
							adj->bev.timers.write.start(adj->timeouts.write * 1000);
						// Останавливаем таймаут ожидания на запись в сокет
						else adj->bev.timers.write.stop();
						// Если нужно повторить попытку
						if(bytes == -2) continue;
						// Если нужно выйти из цикла
						else if(bytes == -1) break;
						// Если нужно завершить работу
						else if(bytes == 0) {
							// Выполняем отключение клиента
							this->close(aid);
							// Выходим из функции
							return;
						}
						// Увеличиваем смещение в буфере
						offset += actual;
					}
					// Если данных в буфере больше чем количество записанных байт
					if(adj->buffer.size() > offset)
						// Выполняем удаление из буфера данных количество отправленных байт
						adj->buffer.assign(adj->buffer.begin() + offset, adj->buffer.end());
					// Иначе просто очищаем буфер данных
					else adj->buffer.clear();
					// Останавливаем запись данных
					if(adj->buffer.empty()) this->disabled(engine_t::method_t::WRITE, aid);
					// Если функция обратного вызова на запись данных установлена
					if(shm->callback.write != nullptr)
						// Выводим функцию обратного вызова
						shm->callback.write(buffer.data(), buffer.size(), aid, shm->sid, reinterpret_cast <awh::core_t *> (this));
				// Если данных недостаточно для записи в сокет
				} else {
					// Останавливаем запись данных
					this->disabled(engine_t::method_t::WRITE, aid);
					// Если функция обратного вызова на запись данных установлена
					if(shm->callback.write != nullptr)
						// Выводим функцию обратного вызова
						shm->callback.write(nullptr, 0, aid, shm->sid, reinterpret_cast <awh::core_t *> (this));
				}
				// Если тип сокета установлен как UDP, и данных для записи больше нет, запускаем чтение
				if(adj->buffer.empty() && (this->net.sonet == scheme_t::sonet_t::UDP) && (this->adjutants.count(aid) > 0))
					// Запускаем событие на чтение базы событий
					adj->bev.events.read.start();
			} break;
		}
	}
}
/**
 * resolving Метод получения IP адреса доменного имени
 * @param sid    идентификатор схемы сети
 * @param ip     адрес интернет-подключения
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param did    идентификатор DNS запроса
 */
void awh::server::Core::resolving(const size_t sid, const string & ip, const int family, const size_t did) noexcept {
	// Если идентификатор схемы сети передан
	if(sid > 0){
		// Выполняем поиск идентификатора схемы сети
		auto it = this->schemes.find(sid);
		// Если идентификатор схемы сети найден, устанавливаем максимальное количество одновременных подключений
		if(it != this->schemes.end()){
			// Получаем объект схемы сети
			scheme_t * shm = (scheme_t *) const_cast <awh::scheme_t *> (it->second);
			// Если IP адрес получен
			if(!ip.empty()){
				// sudo lsof -i -P | grep 1080
				// Обновляем хост сервера
				shm->host = ip;
				// Определяем тип сокета
				switch((uint8_t) this->net.sonet){
					// Если тип сокета установлен как UDP
					case (uint8_t) scheme_t::sonet_t::UDP: {
						// Если разрешено выводить информационные сообщения
						if(!this->noinfo){
							// Если unix-сокет используется
							if(this->net.family == scheme_t::family_t::NIX)
								// Выводим информацию о запущенном сервере на unix-сокете
								this->log->print("run server [%s]", log_t::flag_t::INFO, this->net.filename.c_str());
							// Если unix-сокет не используется, выводим сообщение о запущенном сервере за порту
							else this->log->print("run server [%s:%u]", log_t::flag_t::INFO, shm->host.c_str(), shm->port);
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
						switch((uint8_t) this->net.family){
							// Если тип протокола подключения IPv4
							case (uint8_t) scheme_t::family_t::IPV4:
								// Устанавливаем сеть, для выхода в интернет
								shm->addr.network.assign(
									this->net.v4.first.begin(),
									this->net.v4.first.end()
								);
							break;
							// Если тип протокола подключения IPv6
							case (uint8_t) scheme_t::family_t::IPV6:
								// Устанавливаем сеть, для выхода в интернет
								shm->addr.network.assign(
									this->net.v6.first.begin(),
									this->net.v6.first.end()
								);
							break;
						}
						// Определяем тип сокета
						switch((uint8_t) this->net.sonet){
							// Если тип сокета установлен как UDP TLS
							case (uint8_t) scheme_t::sonet_t::DTLS:
								// Устанавливаем параметры сокета
								shm->addr.sonet(SOCK_DGRAM, IPPROTO_UDP);
							break;
							/**
							 * Если операционной системой является Linux или FreeBSD
							 */
							#if defined(__linux__) || defined(__FreeBSD__)
								// Если тип сокета установлен как SCTP
								case (uint8_t) scheme_t::sonet_t::SCTP:
									// Устанавливаем параметры сокета
									shm->addr.sonet(SOCK_STREAM, IPPROTO_SCTP);
								break;
							#endif
							// Для всех остальных типов сокетов
							default:
								// Устанавливаем параметры сокета
								shm->addr.sonet(SOCK_STREAM, IPPROTO_TCP);
						}
						// Если unix-сокет используется
						if(this->net.family == scheme_t::family_t::NIX)
							// Выполняем инициализацию сокета
							shm->addr.init(this->net.filename, engine_t::type_t::SERVER);
						// Если unix-сокет не используется, выполняем инициализацию сокета
						else shm->addr.init(shm->host, shm->port, (this->net.family == scheme_t::family_t::IPV6 ? AF_INET6 : AF_INET), engine_t::type_t::SERVER, this->_ipV6only);
						// Если сокет подключения получен
						if(shm->addr.fd > -1){
							// Если повесить прослушку на порт не вышло, выходим из условия
							if(!shm->addr.list()) break;
							// Если разрешено выводить информационные сообщения
							if(!this->noinfo){
								// Если unix-сокет используется
								if(this->net.family == scheme_t::family_t::NIX)
									// Выводим информацию о запущенном сервере на unix-сокете
									this->log->print("run server [%s]", log_t::flag_t::INFO, this->net.filename.c_str());
								// Если unix-сокет не используется, выводим сообщение о запущенном сервере за порту
								else this->log->print("run server [%s:%u]", log_t::flag_t::INFO, shm->host.c_str(), shm->port);
							}
							// Если операционная система является Windows или количество процессов всего один
							if(this->_cluster.count(shm->sid) == 1){
								// Устанавливаем базу данных событий
								shm->event.set(this->dispatch.base);
								// Устанавливаем тип события
								shm->event.set(shm->addr.fd, EV_READ | EV_PERSIST);
								// Устанавливаем функцию обратного вызова
								shm->event.set(std::bind(&scheme_t::accept, shm, _1, _2));
								// Выполняем запуск работы события
								shm->event.start();
							// Выполняем запуск кластера
							} else this->_cluster.start(shm->sid);
							// Выходим из функции
							return;
						// Если сокет не создан, выводим в консоль информацию
						} else {
							// Если unix-сокет используется
							if(this->net.family == scheme_t::family_t::NIX)
								// Выводим информацию об незапущенном сервере на unix-сокете
								this->log->print("server cannot be started [%s]", log_t::flag_t::CRITICAL, this->net.filename.c_str());
							// Если unix-сокет не используется, выводим сообщение об незапущенном сервере за порту
							else this->log->print("server cannot be started [%s:%u]", log_t::flag_t::CRITICAL, shm->host.c_str(), shm->port);
						}
					}
				}
			// Если IP адрес сервера не получен, выводим в консоль информацию
			} else this->log->print("broken host server %s", log_t::flag_t::CRITICAL, shm->host.c_str());
			// Останавливаем работу сервера
			this->stop();
		}
	}
}
/**
 * bandWidth Метод установки пропускной способности сети
 * @param aid   идентификатор адъютанта
 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
 */
void awh::server::Core::bandWidth(const size_t aid, const string & read, const string & write) noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		/**
		 * Если операционной системой является Nix-подобная
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Получаем объект адъютанта
			awh::scheme_t::adj_t * adj = const_cast <awh::scheme_t::adj_t *> (it->second);
			// Устанавливаем размер буфера
			adj->ectx.buffer(
				(!read.empty() ? this->fmk->sizeBuffer(read) : 0),
				(!write.empty() ? this->fmk->sizeBuffer(write) : 0),
				reinterpret_cast <const scheme_t *> (adj->parent)->total
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
void awh::server::Core::clusterSize(const size_t size) noexcept {
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
		this->log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
	#endif
}
/**
 * clusterAutoRestart Метод установки флага перезапуска процессов
 * @param sid  идентификатор схемы сети
 * @param mode флаг перезапуска процессов
 */
void awh::server::Core::clusterAutoRestart(const size_t sid, const bool mode) noexcept {
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
		this->log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
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
void awh::server::Core::total(const size_t sid, const u_short total) noexcept {
	// Если идентификатор схемы сети передан
	if(sid > 0){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.main);
		// Выполняем поиск идентификатора схемы сети
		auto it = this->schemes.find(sid);
		// Если идентификатор схемы сети найден, устанавливаем максимальное количество одновременных подключений
		if(it != this->schemes.end())
			// Устанавливаем максимальное количество одновременных подключений
			((scheme_t *) const_cast <awh::scheme_t *> (it->second))->total = total;
	}
}
/**
 * init Метод инициализации сервера
 * @param sid  идентификатор схемы сети
 * @param port порт сервера
 * @param host хост сервера
 */
void awh::server::Core::init(const size_t sid, const u_int port, const string & host) noexcept {
	// Если идентификатор схемы сети передан
	if(sid > 0){
		// Выполняем поиск идентификатора схемы сети
		auto it = this->schemes.find(sid);
		// Если идентификатор схемы сети найден, устанавливаем максимальное количество одновременных подключений
		if(it != this->schemes.end()){
			// Выполняем блокировку потока
			const lock_guard <recursive_mutex> lock(this->_mtx.main);
			// Получаем объект схемы сети
			scheme_t * shm = (scheme_t *) const_cast <awh::scheme_t *> (it->second);
			// Если порт передан, устанавливаем
			if(port > 0) shm->port = port;
			// Если хост передан, устанавливаем
			if(!host.empty()) shm->host = host;
			// Иначе получаем IP адрес сервера автоматически
			else {
				// Объект для работы с сетевым интерфейсом
				ifnet_t ifnet(this->fmk, this->log);
				// Определяем тип протокола подключения
				switch((uint8_t) this->net.family){
					// Если тип протокола подключения unix-сокет
					case (uint8_t) scheme_t::family_t::NIX:
					// Если тип протокола подключения IPv4
					case (uint8_t) scheme_t::family_t::IPV4:
						// Обновляем хост сервера
						shm->host = ifnet.ip(AF_INET);
					break;
					// Если тип протокола подключения IPv6
					case (uint8_t) scheme_t::family_t::IPV6:
						// Обновляем хост сервера
						shm->host = ifnet.ip(AF_INET6);
					break;
				}
			}
		}
	}
}
/**
 * Core Конструктор
 * @param main   флаг основого приложения
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
 * @param sonet  тип сокета подключения (TCP / UDP)
 */
awh::server::Core::Core(const bool main, const fmk_t * fmk, const log_t * log, const scheme_t::family_t family, const scheme_t::sonet_t sonet) noexcept :
 awh::core_t(main, fmk, log, family, sonet), _pid(0), _cluster(fmk, log), _ipV6only(false), _clusterSize(1), _clusterAutoRestart(false) {
	// Устанавливаем тип запускаемого ядра
	this->type = engine_t::type_t::SERVER;
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
