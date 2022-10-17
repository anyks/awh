/**
 * @file: client.cpp
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
#include <lib/event2/core/client.hpp>

/**
 * callback Метод обратного вызова
 * @param fd    файловый дескриптор (сокет)
 * @param event произошедшее событие
 */
void awh::client::Core::Timeout::callback(const evutil_socket_t fd, const short event) noexcept {
	// Останавливаем работу таймера
	this->event.stop();
	// Выполняем поиск идентификатора схемы сети
	auto it = this->core->schemes.find(this->sid);
	// Если идентификатор схемы сети найден
	if(it != this->core->schemes.end()){
		// Флаг запрещения выполнения операции
		bool disallow = false;
		// Если в схеме сети есть подключённые клиенты
		if(!it->second->adjutants.empty()){
			// Выполняем перебор всех подключенных адъютантов
			for(auto & adjutant : it->second->adjutants){
				// Если блокировка адъютанта не установлена
				disallow = (this->core->_locking.count(adjutant.first) > 0);
				// Если в списке есть заблокированные адъютанты, выходим из цикла
				if(disallow) break;
			}
		}
		// Если разрешено выполнять дальнейшую операцию
		if(!disallow){
			// Определяем режим работы клиента
			switch((uint8_t) this->mode){
				// Если режим работы клиента - это подключение
				case (uint8_t) scheme_t::mode_t::CONNECT:
					// Выполняем новое подключение
					this->core->connect(this->sid);
				break;
				// Если режим работы клиента - это переподключение
				case (uint8_t) scheme_t::mode_t::RECONNECT: {
					// Получаем объект схемы сети
					scheme_t * shm = (scheme_t *) const_cast <awh::scheme_t *> (it->second);
					// Устанавливаем флаг ожидания статуса
					shm->status.wait = scheme_t::mode_t::DISCONNECT;
					// Выполняем новую попытку подключиться
					this->core->reconnect(shm->sid);
				} break;
			}
		}
	}
}
/**
 * ~Timeout Деструктор
 */
awh::client::Core::Timeout::~Timeout() noexcept {
	// Останавливаем работу таймера
	this->event.stop();
}
/**
 * connect Метод создания подключения к удаленному серверу
 * @param sid идентификатор схемы сети
 */
void awh::client::Core::connect(const size_t sid) noexcept {
	// Если объект фреймворка существует
	if((this->fmk != nullptr) && (sid > 0)){
		// Выполняем поиск идентификатора схемы сети
		auto it = this->schemes.find(sid);
		// Если идентификатор схемы сети найден
		if(it != this->schemes.end()){
			// Получаем объект схемы сети
			scheme_t * shm = (scheme_t *) const_cast <awh::scheme_t *> (it->second);
			// Если подключение ещё не выполнено и выполнение работ разрешено
			if((shm->status.real == scheme_t::mode_t::DISCONNECT) && (shm->status.work == scheme_t::work_t::ALLOW)){
				// Запрещаем выполнение работы
				shm->status.work = scheme_t::work_t::DISALLOW;
				// Устанавливаем флаг ожидания статуса
				shm->status.wait = scheme_t::mode_t::DISCONNECT;
				// Устанавливаем статус подключения
				shm->status.real = scheme_t::mode_t::PRECONNECT;
				// Получаем URL параметры запроса
				const uri_t::url_t & url = (shm->isProxy() ? shm->proxy.url : shm->url);
				// Получаем семейство интернет-протоколов
				const scheme_t::family_t family = (shm->isProxy() ? shm->proxy.family : this->net.family);
				// Если в схеме сети есть подключённые клиенты
				if(!shm->adjutants.empty()){
					// Переходим по всему списку адъютанта
					for(auto it = shm->adjutants.begin(); it != shm->adjutants.end();){
						// Если блокировка адъютанта не установлена
						if(this->_locking.count(it->first) < 1){
							// Получаем объект адъютанта
							awh::scheme_t::adj_t * adj = const_cast <awh::scheme_t::adj_t *> (it->second.get());
							// Выполняем очистку буфера событий
							this->clean(it->first);
							// Выполняем очистку контекста двигателя
							adj->ectx.clear();
							// Удаляем адъютанта из списка подключений
							this->adjutants.erase(it->first);
							// Удаляем адъютанта из списка
							it = shm->adjutants.erase(it);
						// Если есть хотябы один заблокированный элемент, выходим
						} else {
							// Устанавливаем статус подключения
							shm->status.real = scheme_t::mode_t::DISCONNECT;
							// Выходим из функции
							return;
						}
					}
				}
				// Создаём бъект адъютанта
				unique_ptr <awh::scheme_t::adj_t> adj(new awh::scheme_t::adj_t(shm, this->fmk, this->log));
				// Устанавливаем время жизни подключения
				adj->addr.alive = shm->keepAlive;
				// Определяем тип протокола подключения
				switch((uint8_t) family){
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
				// Определяем тип сокета
				switch((uint8_t) this->net.sonet){
					// Если тип сокета UDP
					case (uint8_t) scheme_t::sonet_t::UDP:
					// Если тип сокета UDP TLS
					case (uint8_t) scheme_t::sonet_t::DTLS:
						// Устанавливаем параметры сокета
						adj->addr.sonet(SOCK_DGRAM, IPPROTO_UDP);
					break;
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
					// Если тип сокета TCP
					case (uint8_t) scheme_t::sonet_t::TCP:
					// Если тип сокета TCP TLS
					case (uint8_t) scheme_t::sonet_t::TLS:
						// Устанавливаем параметры сокета
						adj->addr.sonet(SOCK_STREAM, IPPROTO_TCP);
					break;
				}
				// Если unix-сокет используется
				if(family == scheme_t::family_t::NIX)
					// Выполняем инициализацию сокета
					adj->addr.init(this->net.filename, engine_t::type_t::CLIENT);
				// Если unix-сокет не используется, выполняем инициализацию сокета
				else adj->addr.init(url.ip, url.port, (family == scheme_t::family_t::IPV6 ? AF_INET6 : AF_INET), engine_t::type_t::CLIENT);
				// Если сокет подключения получен
				if(adj->addr.fd != INVALID_SOCKET){
					// Устанавливаем идентификатор адъютанта
					adj->aid = this->fmk->nanoTimestamp();
					// Если подключение выполняется по защищённому каналу DTLS
					if(this->net.sonet == scheme_t::sonet_t::DTLS)
						// Выполняем получение контекста сертификата
						this->engine.wrap(adj->ectx, &adj->addr, engine_t::type_t::CLIENT);
					// Если подключение выполняется не по защищённому каналу DTLS
					else {
						// Получаем хост сервера
						const string & host = (!url.domain.empty() ? url.domain : (!url.ip.empty() ? url.ip : ""));
						// Если функция обратного вызова активации шифрованного TLS канала установлена
						if((shm->callback.is("tls")))
							// Выполняем активацию шифрованного TLS канала
							this->engine.tls(shm->callback.apply <bool, const uri_t::url_t &, const size_t, const size_t, awh::core_t *> ("tls", url, adj->aid, shm->sid, this), adj->ectx);
						// Выполняем активацию контекста подключения
						this->engine.wrapClient(adj->ectx, &adj->addr, host);
					}
					// Если мы хотим работать в зашифрованном режиме
					if(!shm->isProxy() && (this->net.sonet == scheme_t::sonet_t::TLS)){
						// Если сертификаты не приняты, выходим
						if(!this->engine.tls(adj->ectx)){
							// Разрешаем выполнение работы
							shm->status.work = scheme_t::work_t::ALLOW;
							// Устанавливаем статус подключения
							shm->status.real = scheme_t::mode_t::DISCONNECT;
							// Выводим сообщение об ошибке
							this->log->print("encryption mode cannot be activated", log_t::flag_t::CRITICAL);
							// Выводим сообщение об ошибке
							if(!this->noinfo) this->log->print("%s", log_t::flag_t::INFO, "disconnected from the server");
							// Если функция обратного вызова установлена
							if(shm->callback.is("disconnect"))
								// Выполняем функцию обратного вызова
								shm->callback.call <const size_t, const size_t, awh::core_t *> ("disconnect", 0, shm->sid, this);
							// Выходим из функции
							return;
						}
					}
					// Если подключение не обёрнуто
					if(adj->addr.fd == INVALID_SOCKET){
						// Запрещаем чтение данных с сервера
						adj->bev.locked.read = true;
						// Запрещаем запись данных на сервер
						adj->bev.locked.write = true;
						// Разрешаем выполнение работы
						shm->status.work = scheme_t::work_t::ALLOW;
						// Устанавливаем статус подключения
						shm->status.real = scheme_t::mode_t::DISCONNECT;
						// Устанавливаем флаг ожидания статуса
						shm->status.wait = scheme_t::mode_t::DISCONNECT;
						// Выводим сообщение об ошибке
						this->log->print("wrap engine context is failed", log_t::flag_t::CRITICAL);
						// Выполняем переподключение
						this->reconnect(sid);
						// Выходим из функции
						return;
					}
					// Выполняем блокировку потока
					this->_mtx.connect.lock();
					// Добавляем созданного адъютанта в список адъютантов
					auto ret = shm->adjutants.emplace(adj->aid, std::forward <unique_ptr <awh::scheme_t::adj_t>> (adj));
					// Добавляем адъютанта в список подключений
					this->adjutants.emplace(ret.first->first, ret.first->second.get());
					// Выполняем блокировку потока
					this->_mtx.connect.unlock();
					// Если подключение к серверу не выполнено
					if(!ret.first->second->addr.connect()){
						// Запрещаем чтение данных с сервера
						ret.first->second->bev.locked.read = true;
						// Запрещаем запись данных на сервер
						ret.first->second->bev.locked.write = true;
						// Разрешаем выполнение работы
						shm->status.work = scheme_t::work_t::ALLOW;
						// Устанавливаем статус подключения
						shm->status.real = scheme_t::mode_t::DISCONNECT;
						// Если unix-сокет используется
						if(family == scheme_t::family_t::NIX)
							// Выводим ионформацию об обрыве подключении по unix-сокету
							this->log->print("connecting to socket = %s", log_t::flag_t::CRITICAL, this->net.filename.c_str());
						// Выводим ионформацию об обрыве подключении по хосту и порту
						else this->log->print("connecting to host = %s, port = %u", log_t::flag_t::CRITICAL, url.ip.c_str(), url.port);
						// Выполняем сброс кэша резолвера
						this->dns.flush();
						// Определяем тип подключения
						switch((uint8_t) family){
							// Если тип протокола подключения IPv4
							case (uint8_t) scheme_t::family_t::IPV4:
								// Добавляем бракованный IPv4 адрес в список адресов
								this->dns.setToBlackList(AF_INET, url.ip); 
							break;
							// Если тип протокола подключения IPv6
							case (uint8_t) scheme_t::family_t::IPV6:
								// Добавляем бракованный IPv6 адрес в список адресов
								this->dns.setToBlackList(AF_INET6, url.ip);
							break;
						}
						// Выполняем отключение от сервера
						this->close(ret.first->first);
						// Выходим из функции
						return;
					}
					// Получаем адрес подключения клиента
					ret.first->second->ip = url.ip;
					// Получаем порт подключения клиента
					ret.first->second->port = url.port;
					// Получаем аппаратный адрес клиента
					ret.first->second->mac = ret.first->second->addr.mac;
					// Разрешаем выполнение работы
					shm->status.work = scheme_t::work_t::ALLOW;
					// Если статус подключения изменился
					if(shm->status.real != scheme_t::mode_t::PRECONNECT){
						// Запрещаем чтение данных с сервера
						ret.first->second->bev.locked.read = true;
						// Запрещаем запись данных на сервер
						ret.first->second->bev.locked.write = true;
					// Если статус подключения не изменился
					} else {
						// Активируем ожидание подключения
						this->enabled(engine_t::method_t::CONNECT, ret.first->first);
						// Если разрешено выводить информационные сообщения
						if(!this->noinfo){
							// Если unix-сокет используется
							if(family == scheme_t::family_t::NIX)
								// Выводим ионформацию об удачном подключении к серверу по unix-сокету
								this->log->print("good host %s, socket = %d", log_t::flag_t::INFO, this->net.filename.c_str(), ret.first->second->addr.fd);
							// Выводим ионформацию об удачном подключении к серверу по хосту и порту
							else this->log->print("good host %s [%s:%d], socket = %d", log_t::flag_t::INFO, url.domain.c_str(), url.ip.c_str(), url.port, ret.first->second->addr.fd);
						}
					}
					// Выходим из функции
					return;
				// Если сокет не создан, выводим в консоль информацию
				} else {
					// Если unix-сокет используется
					if(family == scheme_t::family_t::NIX)
						// Выводим ионформацию об неудачном подключении к серверу по unix-сокету
						this->log->print("client cannot be started [%s]", log_t::flag_t::CRITICAL, this->net.filename.c_str());
					// Выводим ионформацию об неудачном подключении к серверу по хосту и порту
					else this->log->print("client cannot be started [%s:%u]", log_t::flag_t::CRITICAL, url.ip.c_str(), url.port);
				}
				// Если нужно выполнить автоматическое переподключение
				if(shm->alive){
					// Разрешаем выполнение работы
					shm->status.work = scheme_t::work_t::ALLOW;
					// Устанавливаем статус подключения
					shm->status.real = scheme_t::mode_t::DISCONNECT;
					// Устанавливаем флаг ожидания статуса
					shm->status.wait = scheme_t::mode_t::DISCONNECT;
					// Выполняем переподключение
					this->reconnect(sid);
					// Выходим из функции
					return;
				// Если все попытки исчерпаны
				} else {
					// Разрешаем выполнение работы
					shm->status.work = scheme_t::work_t::ALLOW;
					// Устанавливаем статус подключения
					shm->status.real = scheme_t::mode_t::DISCONNECT;
					// Выполняем сброс кэша резолвера
					this->dns.flush();
					// Определяем тип подключения
					switch((uint8_t) family){
						// Если тип протокола подключения IPv4
						case (uint8_t) scheme_t::family_t::IPV4:
							// Добавляем бракованный IPv4 адрес в список адресов
							this->dns.setToBlackList(AF_INET, url.ip); 
						break;
						// Если тип протокола подключения IPv6
						case (uint8_t) scheme_t::family_t::IPV6:
							// Добавляем бракованный IPv6 адрес в список адресов
							this->dns.setToBlackList(AF_INET6, url.ip);
						break;
					}
					// Выводим сообщение об ошибке
					if(!this->noinfo) this->log->print("%s", log_t::flag_t::INFO, "disconnected from the server");
					// Если функция обратного вызова установлена
					if(shm->callback.is("disconnect"))
						// Выполняем функцию обратного вызова
						shm->callback.call <const size_t, const size_t, awh::core_t *> ("disconnect", 0, shm->sid, this);
				}
			}
		}
	}
}
/**
 * reconnect Метод восстановления подключения
 * @param sid идентификатор схемы сети
 */
void awh::client::Core::reconnect(const size_t sid) noexcept {
	// Выполняем поиск идентификатора схемы сети
	auto it = this->schemes.find(sid);
	// Если идентификатор схемы сети найден
	if(it != this->schemes.end()){
		// Получаем объект схемы сети
		scheme_t * shm = (scheme_t *) const_cast <awh::scheme_t *> (it->second);
		// Если параметры URL запроса переданы и выполнение работы разрешено
		if(!shm->url.empty() && (shm->status.wait == scheme_t::mode_t::DISCONNECT) && (shm->status.work == scheme_t::work_t::ALLOW)){
			// Получаем семейство интернет-протоколов
			const scheme_t::family_t family = (shm->isProxy() ? shm->proxy.family : this->net.family);
			// Определяем тип протокола подключения
			switch((uint8_t) family){
				// Если тип протокола подключения IPv4
				case (uint8_t) scheme_t::family_t::IPV4:
				// Если тип протокола подключения IPv6
				case (uint8_t) scheme_t::family_t::IPV6: {
					// Устанавливаем флаг ожидания статуса
					shm->status.wait = scheme_t::mode_t::RECONNECT;
					// Получаем URL параметры запроса
					const uri_t::url_t & url = (shm->isProxy() ? shm->proxy.url : shm->url);
					// Если IP адрес не получен
					if(url.ip.empty() && !url.domain.empty()){
						// Устанавливаем событие на получение данных с DNS сервера
						this->dns.on(std::bind(&scheme_t::resolving, shm, _1, _2, _3));
						// Определяем тип протокола подключения
						switch((uint8_t) family){
							// Если тип протокола подключения IPv4
							case (uint8_t) scheme_t::family_t::IPV4:
								// Выполняем резолвинг домена
								shm->did = this->dns.resolve(url.domain, AF_INET);
							break;
							// Если тип протокола подключения IPv6
							case (uint8_t) scheme_t::family_t::IPV6:
								// Выполняем резолвинг домена
								shm->did = this->dns.resolve(url.domain, AF_INET6);
							break;
						}
					// Выполняем запуск системы
					} else if(!url.ip.empty()) {
						// Определяем тип протокола подключения
						switch((uint8_t) family){
							// Если тип протокола подключения IPv4
							case (uint8_t) scheme_t::family_t::IPV4:
								// Выполняем резолвинг домена
								this->resolving(shm->sid, url.ip, AF_INET, 0);
							break;
							// Если тип протокола подключения IPv6
							case (uint8_t) scheme_t::family_t::IPV6:
								// Выполняем резолвинг домена
								this->resolving(shm->sid, url.ip, AF_INET6, 0);
							break;
						}
					}
				} break;
				// Если тип протокола подключения unix-сокет
				case (uint8_t) scheme_t::family_t::NIX:
					// Выполняем подключение заново
					this->connect(shm->sid);
				break;
			}
		}
	}
}
/**
 * createTimeout Метод создания таймаута
 * @param sid  идентификатор схемы сети
 * @param mode режим работы клиента
 */
void awh::client::Core::createTimeout(const size_t sid, const scheme_t::mode_t mode) noexcept {
	// Выполняем поиск идентификатора схемы сети
	auto it = this->schemes.find(sid);
	// Если идентификатор схемы сети найден
	if(it != this->schemes.end()){
		// Объект таймаута
		timeout_t * timeout = nullptr;
		// Выполняем поиск существующего таймаута
		auto it = this->_timeouts.find(sid);
		// Если таймаут найден
		if(it != this->_timeouts.end())
			// Получаем объект таймаута
			timeout = it->second.get();
		// Если таймаут ещё не существует
		else {
			// Выполняем блокировку потока
			this->_mtx.timeout.lock();
			// Получаем объект таймаута
			timeout = this->_timeouts.emplace(sid, unique_ptr <timeout_t> (new timeout_t)).first->second.get();
			// Выполняем разблокировку потока
			this->_mtx.timeout.unlock();
		}
		// Устанавливаем идентификатор таймаута
		timeout->sid = sid;
		// Устанавливаем режим работы клиента
		timeout->mode = mode;
		// Устанавливаем ядро клиента
		timeout->core = this;
		// Устанавливаем тип таймера
		timeout->event.set(-1, EV_TIMEOUT);
		// Устанавливаем базу данных событий
		timeout->event.set(this->dispatch.base);
		// Устанавливаем функцию обратного вызова
		timeout->event.set(std::bind(&timeout_t::callback, timeout, _1, _2));
		// Выполняем запуск работы таймера
		timeout->event.start(5000);
	}
}
/**
 * sendTimeout Метод отправки принудительного таймаута
 * @param aid идентификатор адъютанта
 */
void awh::client::Core::sendTimeout(const size_t aid) noexcept {
	// Если блокировка адъютанта не установлена
	if(this->_locking.count(aid) < 1){
		// Если адъютант существует
		if(this->adjutants.count(aid) > 0)
			// Выполняем отключение от сервера
			this->close(aid);
		// Если адъютант не существует
		else if(!this->schemes.empty()) {
			// Выполняем блокировку потока
			const lock_guard <recursive_mutex> lock(this->_mtx.reset);
			// Переходим по всему списку схем сети
			for(auto & item : this->schemes){
				// Получаем объект схемы сети
				scheme_t * shm = (scheme_t *) const_cast <awh::scheme_t *> (item.second);
				// Если выполнение работ разрешено
				if(shm->status.work == scheme_t::work_t::ALLOW)
					// Запрещаем выполнение работы
					shm->status.work = scheme_t::work_t::DISALLOW;
				// Если работы запрещены, выходим
				else return;
				// Запрещаем выполнять перезапуск
				shm->stop = true;
			}
			// Выполняем отключение всех подключённых адъютантов
			this->close();
			// Выполняем пинок базе событий
			this->dispatch.kick();
			// Флаг поддержания постоянного подключения
			bool alive = false;
			// Переходим по всему списку схем сети
			for(auto & item : this->schemes){
				// Получаем объект схемы сети
				scheme_t * shm = (scheme_t *) const_cast <awh::scheme_t *> (item.second);
				// Если флаг поддержания постоянного подключения не установлен
				if(!alive && shm->alive) alive = shm->alive;
				// Устанавливаем статус подключения
				shm->status.real = scheme_t::mode_t::DISCONNECT;
				// Устанавливаем флаг ожидания статуса
				shm->status.wait = scheme_t::mode_t::DISCONNECT;
			}
			// Если необходимо поддерживать постоянное подключение
			if(alive){
				// Добавляем базу событий для DNS резолвера
				this->dns.base(this->dispatch.base);
				// Определяем тип протокола подключения
				switch((uint8_t) this->net.family){
					// Если тип протокола подключения IPv4
					case (uint8_t) scheme_t::family_t::IPV4:
						// Выполняем установку нейм-серверов для DNS резолвера IPv4
						this->dns.replace(AF_INET, this->net.v4.second);
					break;
					// Если тип протокола подключения IPv6
					case (uint8_t) scheme_t::family_t::IPV6:
						// Выполняем установку нейм-серверов для DNS резолвера IPv6
						this->dns.replace(AF_INET6, this->net.v4.second);
					break;
				}
			}
			// Переходим по всему списку схем сети
			for(auto & item : this->schemes){
				// Получаем объект схемы сети
				scheme_t * shm = (scheme_t *) const_cast <awh::scheme_t *> (item.second);
				// Разрешаем выполнять перезапуск
				shm->stop = false;
				// Если выполнение работ запрещено
				if(shm->status.work == scheme_t::work_t::DISALLOW)
					// Разрешаем выполнение работы
					shm->status.work = scheme_t::work_t::ALLOW;
				// Если нужно выполнить автоматическое переподключение, выполняем новую попытку
				if(shm->alive) this->reconnect(item.first);
			}
		}
	}
}
/**
 * clearTimeout Метод удаления установленного таймаута
 * @param sid идентификатор схемы сети
 */
void awh::client::Core::clearTimeout(const size_t sid) noexcept {
	// Если список таймеров не пустой
	if(!this->_timeouts.empty()){
		// Выполняем поиск таймера
		auto it = this->_timeouts.find(sid);
		// Если таймер найден
		if(it != this->_timeouts.end()){
			// Выполняем блокировку потока
			this->_mtx.timeout.lock();
			// Останавливаем работу таймера
			it->second->event.stop();
			// Выполняем разблокировку потока
			this->_mtx.timeout.unlock();
		}
	}
}
/**
 * close Метод отключения всех адъютантов
 */
void awh::client::Core::close() noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.close);
	// Если список активных таймеров существует
	if(!this->_timeouts.empty()){
		// Переходим по всему списку активных таймеров
		for(auto & timeout : this->_timeouts)
			// Останавливаем работу таймера
			timeout.second->event.stop();
	}
	// Если список схем сети активен
	if(!this->schemes.empty()){
		// Объект работы с функциями обратного вызова
		fn_t callback(this->log);
		// Переходим по всему списку схем сети
		for(auto & item : this->schemes){
			// Если в схеме сети есть подключённые клиенты
			if(!item.second->adjutants.empty()){
				// Получаем объект схемы сети
				scheme_t * shm = (scheme_t *) const_cast <awh::scheme_t *> (item.second);
				// Устанавливаем флаг ожидания статуса
				shm->status.wait = scheme_t::mode_t::DISCONNECT;
				// Устанавливаем статус сетевого ядра
				shm->status.real = scheme_t::mode_t::DISCONNECT;
				// Если прокси-сервер активирован но уже переключён на работу с сервером
				if((shm->proxy.type != proxy_t::type_t::NONE) && !shm->isProxy())
					// Выполняем переключение обратно на прокси-сервер
					shm->switchConnect();
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
						// Если функция обратного вызова установлена
						if(shm->callback.is("disconnect"))
							// Устанавливаем полученную функцию обратного вызова
							callback.set <void (const size_t, const size_t, awh::core_t *)> (to_string(it->first), shm->callback.get <void (const size_t, const size_t, awh::core_t *)> ("disconnect"), it->first, item.first, this);
						// Удаляем блокировку адъютанта
						this->_locking.erase(it->first);
						// Удаляем адъютанта из списка
						it = shm->adjutants.erase(it);
					// Иначе продолжаем дальше
					} else ++it;
				}
			}
		}
		// Выполняем все функции обратного вызова
		callback.bind <const size_t, const size_t, awh::core_t *> ();
	}
}
/**
 * remove Метод удаления всех схем сети
 */
void awh::client::Core::remove() noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.close);
	// Если список схем сети активен
	if(!this->schemes.empty()){
		// Объект работы с функциями обратного вызова
		fn_t callback(this->log);
		// Если список активных таймеров существует
		if(!this->_timeouts.empty()){
			// Переходим по всему списку активных таймеров
			for(auto it = this->_timeouts.begin(); it != this->_timeouts.end();){
				// Выполняем блокировку потока
				this->_mtx.timeout.lock();
				// Выполняем удаление текущего таймаута
				it = this->_timeouts.erase(it);
				// Выполняем разблокировку потока
				this->_mtx.timeout.unlock();
			}
		}
		// Переходим по всему списку схем сети
		for(auto it = this->schemes.begin(); it != this->schemes.end();){
			// Получаем объект схемы сети
			scheme_t * shm = (scheme_t *) const_cast <awh::scheme_t *> (it->second);
			// Устанавливаем флаг ожидания статуса
			shm->status.wait = scheme_t::mode_t::DISCONNECT;
			// Устанавливаем статус сетевого ядра
			shm->status.real = scheme_t::mode_t::DISCONNECT;
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
						// Если функция обратного вызова установлена
						if(shm->callback.is("disconnect"))
							// Устанавливаем полученную функцию обратного вызова
							callback.set <void (const size_t, const size_t, awh::core_t *)> (to_string(jt->first), shm->callback.get <void (const size_t, const size_t, awh::core_t *)> ("disconnect"), jt->first, it->first, this);
						// Удаляем блокировку адъютанта
						this->_locking.erase(jt->first);
						// Удаляем адъютанта из списка
						jt = shm->adjutants.erase(jt);
					// Иначе продолжаем дальше
					} else ++jt;
				}
			}
			// Выполняем удаление схемы сети
			it = this->schemes.erase(it);
		}
		// Выполняем все функции обратного вызова
		callback.bind <const size_t, const size_t, awh::core_t *> ();
	}
}
/**
 * async Метод активации асинхронного режима работы
 * @param mode флаг активации асинхронного режима работы
 */
void awh::client::Core::async(const bool mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Устанавливаем флаг активации асинхронного режима работы
	this->_async = mode;
}
/**
 * open Метод открытия подключения
 * @param sid идентификатор схемы сети
 */
void awh::client::Core::open(const size_t sid) noexcept {
	// Если идентификатор схемы сети передан
	if(sid > 0){
		// Выполняем поиск идентификатора схемы сети
		auto it = this->schemes.find(sid);
		// Если идентификатор схемы сети найден
		if(it != this->schemes.end()){
			// Получаем объект схемы сети
			scheme_t * shm = (scheme_t *) const_cast <awh::scheme_t *> (it->second);
			// Если параметры URL запроса переданы и выполнение работы разрешено
			if(!shm->url.empty() && (shm->status.wait == scheme_t::mode_t::DISCONNECT) && (shm->status.work == scheme_t::work_t::ALLOW)){
				// Получаем семейство интернет-протоколов
				const scheme_t::family_t family = (shm->isProxy() ? shm->proxy.family : this->net.family);
				// Определяем тип протокола подключения
				switch((uint8_t) family){
					// Если тип протокола подключения IPv4
					case (uint8_t) scheme_t::family_t::IPV4:
					// Если тип протокола подключения IPv6
					case (uint8_t) scheme_t::family_t::IPV6: {
						// Устанавливаем флаг ожидания статуса
						shm->status.wait = scheme_t::mode_t::CONNECT;
						// Получаем URL параметры запроса
						const uri_t::url_t & url = (shm->isProxy() ? shm->proxy.url : shm->url);
						// Если IP адрес не получен
						if(url.ip.empty() && !url.domain.empty()){
							// Устанавливаем событие на получение данных с DNS сервера
							this->dns.on(std::bind(&scheme_t::resolving, shm, _1, _2, _3));
							// Определяем тип протокола подключения
							switch((uint8_t) this->net.family){
								// Если тип протокола подключения IPv4
								case (uint8_t) scheme_t::family_t::IPV4:
									// Выполняем резолвинг домена
									shm->did = this->dns.resolve(url.domain, AF_INET);
								break;
								// Если тип протокола подключения IPv6
								case (uint8_t) scheme_t::family_t::IPV6:
									// Выполняем резолвинг домена
									shm->did = this->dns.resolve(url.domain, AF_INET6);
								break;
							}
						// Выполняем запуск системы
						} else if(!url.ip.empty()) {
							// Определяем тип протокола подключения
							switch((uint8_t) this->net.family){
								// Если тип протокола подключения IPv4
								case (uint8_t) scheme_t::family_t::IPV4:
									// Выполняем резолвинг домена
									this->resolving(shm->sid, url.ip, AF_INET, 0);
								break;
								// Если тип протокола подключения IPv6
								case (uint8_t) scheme_t::family_t::IPV6:
									// Выполняем резолвинг домена
									this->resolving(shm->sid, url.ip, AF_INET6, 0);
								break;
							}
						}
					} break;
					// Если тип протокола подключения unix-сокет
					case (uint8_t) scheme_t::family_t::NIX: {
						// Если требуется подключение через прокси-сервер
						if(shm->isProxy())
							// Создаём unix-сокет
							this->unixSocket(shm->proxy.url.host);
						// Выполняем подключение заново
						this->connect(shm->sid);
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
void awh::client::Core::remove(const size_t sid) noexcept {
	// Если идентификатор схемы сети передан
	if(sid > 0){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.close);
		// Выполняем поиск схемы сети
		auto it = this->schemes.find(sid);
		// Если идентификатор схемы сети найден
		if(it != this->schemes.end()){
			// Объект работы с функциями обратного вызова
			fn_t callback(this->log);
			// Получаем объект схемы сети
			scheme_t * shm = (scheme_t *) const_cast <awh::scheme_t *> (it->second);
			// Устанавливаем флаг ожидания статуса
			shm->status.wait = scheme_t::mode_t::DISCONNECT;
			// Устанавливаем статус сетевого ядра
			shm->status.real = scheme_t::mode_t::DISCONNECT;
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
						// Если функция обратного вызова установлена
						if(shm->callback.is("disconnect"))
							// Устанавливаем полученную функцию обратного вызова
							callback.set <void (const size_t, const size_t, awh::core_t *)> (to_string(jt->first), shm->callback.get <void (const size_t, const size_t, awh::core_t *)> ("disconnect"), jt->first, it->first, this);
						// Удаляем блокировку адъютанта
						this->_locking.erase(jt->first);
						// Удаляем адъютанта из списка
						jt = shm->adjutants.erase(jt);
					// Иначе продолжаем дальше
					} else ++jt;
				}
			}
			// Выполняем удаление уоркера из списка
			this->schemes.erase(it);
			// Выполняем поиск активного таймаута
			auto it = this->_timeouts.find(sid);
			// Если таймаут найден, удаляем его
			if(it != this->_timeouts.end()){
				// Выполняем блокировку потока
				this->_mtx.timeout.lock();
				// Выполняем удаление текущего таймаута
				this->_timeouts.erase(it);
				// Выполняем разблокировку потока
				this->_mtx.timeout.unlock();
			}
			// Выполняем все функции обратного вызова
			callback.bind <const size_t, const size_t, awh::core_t *> ();
		}
	}
}
/**
 * close Метод закрытия подключения
 * @param aid идентификатор адъютанта
 */
void awh::client::Core::close(const size_t aid) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.close);
	// Если блокировка адъютанта не установлена
	if(this->_locking.count(aid) < 1){
		// Выполняем блокировку адъютанта
		this->_locking.emplace(aid);
		// Объект работы с функциями обратного вызова
		fn_t callback(this->log);
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
			// Удаляем установленный таймаут, если он существует
			this->clearTimeout(shm->sid);
			// Если прокси-сервер активирован но уже переключён на работу с сервером
			if((shm->proxy.type != proxy_t::type_t::NONE) && !shm->isProxy())
				// Выполняем переключение обратно на прокси-сервер
				shm->switchConnect();
			// Выполняем очистку контекста двигателя
			adj->ectx.clear();
			// Удаляем адъютанта из списка адъютантов
			shm->adjutants.erase(aid);
			// Удаляем адъютанта из списка подключений
			this->adjutants.erase(aid);
			// Устанавливаем флаг ожидания статуса
			shm->status.wait = scheme_t::mode_t::DISCONNECT;
			// Устанавливаем статус сетевого ядра
			shm->status.real = scheme_t::mode_t::DISCONNECT;
			// Если не нужно выполнять принудительную остановку работы схемы сети
			if(!shm->stop){
				// Если нужно выполнить автоматическое переподключение
				if(shm->alive) this->reconnect(shm->sid);
				// Если автоматическое подключение выполнять не нужно
				else {
					// Выводим сообщение об ошибке
					if(!core->noinfo) this->log->print("%s", log_t::flag_t::INFO, "disconnected from the server");
					// Если функция обратного вызова установлена
					if(shm->callback.is("disconnect"))
						// Устанавливаем полученную функцию обратного вызова
						callback.set <void (const size_t, const size_t, awh::core_t *)> (to_string(it->first), shm->callback.get <void (const size_t, const size_t, awh::core_t *)> ("disconnect"), aid, shm->sid, this);
				}
			}
		}
		// Удаляем блокировку адъютанта
		this->_locking.erase(aid);
		// Выполняем все функции обратного вызова
		callback.bind <const size_t, const size_t, awh::core_t *> ();
	}
}
/**
 * switchProxy Метод переключения с прокси-сервера
 * @param aid идентификатор адъютанта
 */
void awh::client::Core::switchProxy(const size_t aid) noexcept {
	// Определяем тип производимого подключения
	switch((uint8_t) this->net.sonet){
		// Если подключение производится по протоколу TCP
		case (uint8_t) scheme_t::sonet_t::TCP:
		// Если подключение производится по протоколу TLS
		case (uint8_t) scheme_t::sonet_t::TLS:
		// Если подключение производится по протоколу SCTP
		case (uint8_t) scheme_t::sonet_t::SCTP: break;
		// Если активирован любой другой протокол, выходим из функции
		default: return;
	}	
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.proxy);
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Получаем объект адъютанта
		awh::scheme_t::adj_t * adj = const_cast <awh::scheme_t::adj_t *> (it->second);
		// Получаем объект схемы сети
		scheme_t * shm = (scheme_t *) const_cast <awh::scheme_t *> (adj->parent);
		// Если прокси-сервер активирован но ещё не переключён на работу с сервером
		if((shm->proxy.type != proxy_t::type_t::NONE) && shm->isProxy()){
			// Выполняем переключение на работу с сервером
			shm->switchConnect();
			// Получаем хост сервера
			const string & host = (!shm->url.domain.empty() ? shm->url.domain : (!shm->url.ip.empty() ? shm->url.ip : ""));
			// Если функция обратного вызова активации шифрованного TLS канала установлена
			if((shm->callback.is("tls")))
				// Выполняем активацию шифрованного TLS канала
				this->engine.tls(shm->callback.apply <bool, const uri_t::url_t &, const size_t, const size_t, awh::core_t *> ("tls", shm->url, aid, shm->sid, this), adj->ectx);
			// Выполняем получение контекста сертификата
			this->engine.wrapClient(adj->ectx, adj->ectx, host);
			// Если подключение не обёрнуто
			if(adj->addr.fd == INVALID_SOCKET){
				// Выводим сообщение об ошибке
				this->log->print("wrap engine context is failed", log_t::flag_t::CRITICAL);
				// Выходим из функции
				return;
			}
			// Останавливаем чтение данных
			this->disabled(engine_t::method_t::READ, it->first);
			// Останавливаем запись данных
			this->disabled(engine_t::method_t::WRITE, it->first);
			// Активируем ожидание подключения
			this->enabled(engine_t::method_t::CONNECT, it->first);
		}
	}
}
/**
 * timeout Метод вызова при срабатывании таймаута
 * @param aid идентификатор адъютанта
 */
void awh::client::Core::timeout(const size_t aid) noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Получаем объект адъютанта
		awh::scheme_t::adj_t * adj = const_cast <awh::scheme_t::adj_t *> (it->second);
		// Получаем объект подключения
		scheme_t * shm = (scheme_t *) const_cast <awh::scheme_t *> (adj->parent);
		// Получаем семейство интернет-протоколов
		const scheme_t::family_t family = (shm->isProxy() ? shm->proxy.family : this->net.family);
		// Определяем тип протокола подключения
		switch((uint8_t) family){
			// Если тип протокола подключения IPv4
			case (uint8_t) scheme_t::family_t::IPV4:
			// Если тип протокола подключения IPv6
			case (uint8_t) scheme_t::family_t::IPV6: {
				// Получаем URL параметры запроса
				const uri_t::url_t & url = (shm->isProxy() ? shm->proxy.url : shm->url);
				// Если данные ещё ни разу не получены
				if(!shm->acquisition && !url.ip.empty()){
					// Определяем тип протокола подключения
					switch((uint8_t) family){
						// Резолвер IPv4, добавляем бракованный IPv4 адрес в список адресов
						case (uint8_t) scheme_t::family_t::IPV4: this->dns.setToBlackList(AF_INET, url.ip); break;
						// Резолвер IPv6, добавляем бракованный IPv6 адрес в список адресов
						case (uint8_t) scheme_t::family_t::IPV6: this->dns.setToBlackList(AF_INET6, url.ip); break;
					}
				}			
				// Выводим сообщение в лог, о таймауте подключения
				this->log->print("timeout host %s [%s%d]", log_t::flag_t::WARNING, url.domain.c_str(), (!url.ip.empty() ? (url.ip + ":").c_str() : ""), url.port);
			} break;
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
		// Выполняем отключение от сервера
		this->close(aid);
	}
}
/**
 * connected Метод вызова при удачном подключении к серверу
 * @param aid идентификатор адъютанта
 */
void awh::client::Core::connected(const size_t aid) noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Получаем объект адъютанта
		awh::scheme_t::adj_t * adj = const_cast <awh::scheme_t::adj_t *> (it->second);
		// Получаем объект подключения
		scheme_t * shm = (scheme_t *) const_cast <awh::scheme_t *> (adj->parent);
		// Если подключение удачное и работа разрешена
		if(shm->status.work == scheme_t::work_t::ALLOW){
			// Снимаем флаг получения данных
			shm->acquisition = false;
			// Устанавливаем статус подключения к серверу
			shm->status.real = scheme_t::mode_t::CONNECT;
			// Устанавливаем флаг ожидания статуса
			shm->status.wait = scheme_t::mode_t::DISCONNECT;
			// Выполняем очистку существующих таймаутов
			this->clearTimeout(shm->sid);
			// Получаем семейство интернет-протоколов
			const scheme_t::family_t family = (shm->isProxy() ? shm->proxy.family : this->net.family);
			// Определяем тип протокола подключения
			switch((uint8_t) family){
				// Если тип протокола подключения IPv4
				case (uint8_t) scheme_t::family_t::IPV4:
				// Если тип протокола подключения IPv6
				case (uint8_t) scheme_t::family_t::IPV6: {
					// Получаем URL параметры запроса
					const uri_t::url_t & url = (shm->isProxy() ? shm->proxy.url : shm->url);
					// Получаем хост сервера
					const string & host = (!url.ip.empty() ? url.ip : url.domain);
					// Выполняем отмену ранее выполненных запросов DNS
					this->dns.cancel(shm->did);
					// Запускаем чтение данных
					this->enabled(engine_t::method_t::READ, it->first);
					// Выводим в лог сообщение
					if(!this->noinfo) this->log->print("connect client to server [%s:%d]", log_t::flag_t::INFO, host.c_str(), url.port);
				} break;
				// Если тип протокола подключения unix-сокет
				case (uint8_t) scheme_t::family_t::NIX: {
					// Запускаем чтение данных
					this->enabled(engine_t::method_t::READ, it->first);
					// Выводим в лог сообщение
					if(!this->noinfo) this->log->print("connect client to server [%s]", log_t::flag_t::INFO, this->net.filename.c_str());
				} break;
			}
			// Если подключение производится через, прокси-сервер
			if(shm->isProxy()){
				// Если функция обратного вызова для прокси-сервера
				if(shm->callback.is("connectProxy"))
					// Выполняем функцию обратного вызова
					shm->callback.call <const size_t, const size_t, awh::core_t *> ("connectProxy", it->first, shm->sid, const_cast <awh::core_t *> (shm->core));
			// Если функция обратного вызова установлена
			} else if(shm->callback.is("connect"))
				// Выполняем функцию обратного вызова
				shm->callback.call <const size_t, const size_t, awh::core_t *> ("connect", it->first, shm->sid, const_cast <awh::core_t *> (shm->core));
			// Выходим из функции
			return;
		}
		// Выполняем отключение от сервера
		this->close(it->first);
	}
}
/**
 * transfer Метед передачи данных между клиентом и сервером
 * @param method метод режима работы
 * @param aid    идентификатор адъютанта
 */
void awh::client::Core::transfer(const engine_t::method_t method, const size_t aid) noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Получаем объект адъютанта
		awh::scheme_t::adj_t * adj = const_cast <awh::scheme_t::adj_t *> (it->second);
		// Получаем объект подключения
		scheme_t * shm = (scheme_t *) const_cast <awh::scheme_t *> (adj->parent);
		// Если подключение установлено
		if((shm->acquisition = (shm->status.real == scheme_t::mode_t::CONNECT))){
			// Устанавливаем текущий метод режима работы
			adj->method = method;
			// Определяем метод работы
			switch((uint8_t) adj->method){
				// Если производится чтение данных
				case (uint8_t) engine_t::method_t::READ: {
					// Останавливаем чтение данных с клиента
					adj->bev.events.read.stop();
					// Получаем максимальный размер буфера
					const int64_t size = adj->ectx.buffer(engine_t::method_t::READ);
					// Если размер буфера получен
					if(size > 0){
						// Количество полученных байт
						int64_t bytes = -1;
						// Создаём буфер входящих данных
						unique_ptr <char []> buffer(new char [size]);
						// Если нужно использовать асинхронный режим работы
						if(this->_async)
							// Переводим сокет в неблокирующий режим
							adj->ectx.noblock();
						// Выполняем чтение данных с сокета
						do {
							// Если подключение выполнено
							if(!adj->bev.locked.read && (shm->status.real == scheme_t::mode_t::CONNECT)){
								// Выполняем обнуление буфера данных
								memset(buffer.get(), 0, size);
								// Выполняем получение сообщения от клиента
								bytes = adj->ectx.read(buffer.get(), size);
								// Если время ожидания чтения данных установлено
								if(shm->wait && (adj->timeouts.read > 0))
									// Запускаем работу таймера
									adj->bev.timers.read.start(adj->timeouts.read * 1000);
								// Останавливаем таймаут ожидания на чтение из сокета
								else adj->bev.timers.read.stop();
								// Если данные получены
								if(bytes > 0){
									// Если данные считанные из буфера, больше размера ожидающего буфера
									if((adj->marker.read.max > 0) && (bytes >= adj->marker.read.max)){
										// Смещение в буфере и отправляемый размер данных
										size_t offset = 0, actual = 0;
										// Выполняем пересылку всех полученных данных
										while((bytes - offset) > 0){
											// Определяем размер отправляемых данных
											actual = ((bytes - offset) >= adj->marker.read.max ? adj->marker.read.max : (bytes - offset));
											// Если подключение производится через, прокси-сервер
											if(shm->isProxy()){
												// Если функция обратного вызова для вывода записи существует
												if(shm->callback.is("readProxy"))
													// Выводим функцию обратного вызова
													shm->callback.call <const char *, const size_t, const size_t, const size_t, awh::core_t *> ("readProxy", buffer.get() + offset, actual, aid, shm->sid, reinterpret_cast <awh::core_t *> (this));
											// Если прокси-сервер не используется
											} else if(shm->callback.is("read"))
												// Выводим функцию обратного вызова
												shm->callback.call <const char *, const size_t, const size_t, const size_t, awh::core_t *> ("read", buffer.get() + offset, actual, aid, shm->sid, reinterpret_cast <awh::core_t *> (this));
											// Увеличиваем смещение в буфере
											offset += actual;
										}
									// Если данных достаточно
									} else {
										// Если подключение производится через, прокси-сервер
										if(shm->isProxy()){
											// Если функция обратного вызова для вывода записи существует
											if(shm->callback.is("readProxy"))
												// Выводим функцию обратного вызова
												shm->callback.call <const char *, const size_t, const size_t, const size_t, awh::core_t *> ("readProxy", buffer.get(), bytes, aid, shm->sid, reinterpret_cast <awh::core_t *> (this));
										// Если прокси-сервер не используется
										} else if(shm->callback.is("read"))
											// Выводим функцию обратного вызова
											shm->callback.call <const char *, const size_t, const size_t, const size_t, awh::core_t *> ("read", buffer.get(), bytes, aid, shm->sid, reinterpret_cast <awh::core_t *> (this));
									}
								// Если данные не получены
								} else {
									// Если режим работы асинхронный
									if(this->_async){
										// Если нужно повторить запись
										if(bytes == -2){
											// Если подключение ещё существует
											if((this->adjutants.find(aid) != this->adjutants.end()) && (adj->method == engine_t::method_t::READ) && adj->bev.locked.write)
												// Продолжаем попытку снова
												continue;
											// Если запись не выполнена, входим
											else break;
										// Если запись не выполнена, входим
										} else break;
									// Если режим работы синхронный
									} else {
										// Если произошёл дисконнект
										if(bytes == 0){
											// Выполняем отключение клиента
											this->close(aid);
											// Выходим из функции
											return;
										// Если запись не выполнена, входим
										} else break;
									}
								}
							// Выходим из цикла
							} else break;
						// Выполняем чтение до тех пор, пока всё не прочитаем
						} while((this->adjutants.find(aid) != this->adjutants.end()) && (adj->method == engine_t::method_t::READ) && adj->bev.locked.write);
						// Если тип сокета не установлен как UDP, запускаем чтение дальше
						if((this->net.sonet != scheme_t::sonet_t::UDP) && (this->adjutants.count(aid) > 0))
							// Запускаем чтение данных с клиента
							adj->bev.events.read.start();
					// Выполняем отключение клиента
					} else this->close(aid);
				} break;
				// Если производится запись данных
				case (uint8_t) engine_t::method_t::WRITE: {
					// Останавливаем работу таймера
					adj->bev.timers.write.stop();
					// Выполняем отправку всех данных
					for(;;){
						// Если данных достаточно для записи в сокет
						if(!adj->buffer.empty() && (adj->buffer.size() >= adj->marker.write.min)){
							// Количество полученных байт
							int64_t bytes = -1;
							// Cмещение в буфере и отправляемый размер данных
							size_t offset = 0, actual = 0, size = 0;
							// Получаем максимальный размер буфера
							int64_t max = adj->ectx.buffer(engine_t::method_t::WRITE);
							// Если максимальное установленное значение больше размеров буфера для записи, корректируем
							max = ((max > 0) && (adj->marker.write.max > max) ? max : adj->marker.write.max);
							// Получаем буфер отправляемых данных
							const vector <char> buffer = std::forward <vector <char>> (adj->buffer);
							// Выполняем отправку данных пока всё не отправим
							while(!adj->bev.locked.write && ((buffer.size() - offset) > 0)){
								// Получаем общий размер буфера данных
								size = (buffer.size() - offset);
								// Определяем размер отправляемых данных
								actual = (size >= max ? max : size);
								// Выполняем отправку сообщения клиенту
								bytes = adj->ectx.write(buffer.data() + offset, actual);
								// Если время ожидания записи данных установлено
								if(adj->timeouts.write > 0)
									// Запускаем работу таймера
									adj->bev.timers.write.start(adj->timeouts.write * 1000);
								// Останавливаем таймаут ожидания на запись в сокет
								else adj->bev.timers.write.stop();
								// Если данные небыли записаны
								if(bytes <= 0){
									// Если режим работы асинхронный
									if(this->_async){
										// Если нужно повторить запись
										if(bytes == -2)
											// Продолжаем попытку снова
											continue;
										// Если запись не выполнена, входим
										else break;
									// Если режим работы синхронный
									} else {
										// Если произошёл дисконнект
										if(bytes == 0){
											// Выполняем отключение клиента
											this->close(aid);
											// Выходим из функции
											return;
										// Если запись не выполнена, входим
										} else break;
									}
								}
								// Увеличиваем смещение в буфере
								offset += bytes;
							}
							// Останавливаем запись данных
							this->disabled(engine_t::method_t::WRITE, aid);
							// Если функция обратного вызова на запись данных установлена
							if(shm->callback.is("write"))
								// Выводим функцию обратного вызова
								shm->callback.call <const char *, const size_t, const size_t, const size_t, awh::core_t *> ("write", (!buffer.empty() ? buffer.data() : nullptr), offset, aid, shm->sid, reinterpret_cast <awh::core_t *> (this));
						// Если данных недостаточно для записи в сокет
						} else {
							// Останавливаем запись данных
							this->disabled(engine_t::method_t::WRITE, aid);
							// Если функция обратного вызова на запись данных установлена
							if(shm->callback.is("write"))
								// Выводим функцию обратного вызова
								shm->callback.call <const char *, const size_t, const size_t, const size_t, awh::core_t *> ("write", nullptr, 0, aid, shm->sid, reinterpret_cast <awh::core_t *> (this));
						}
						// Если адъютант ещё существует и подключён
						if((this->adjutants.count(aid) > 0) && adj->bev.locked.write &&
						   !adj->buffer.empty() && (adj->buffer.size() >= adj->marker.write.min)){
							// Снимаем блокировку с записи данных
							adj->bev.locked.write = !adj->bev.locked.write;
							// Выполняем запись в сокет оставшихся данных из буфера
							continue;
						}
						// Выходим из цикла
						break;
					}
					// Если тип сокета установлен как UDP, и данных для записи больше нет, запускаем чтение
					if((this->net.sonet == scheme_t::sonet_t::UDP) && (this->adjutants.count(aid) > 0) && adj->buffer.empty())
						// Запускаем чтение данных с клиента
						adj->bev.events.read.start();
				} break;
			}
		// Если подключение завершено
		} else {
			// Останавливаем чтение данных
			this->disabled(engine_t::method_t::READ, it->first);
			// Останавливаем запись данных
			this->disabled(engine_t::method_t::WRITE, it->first);
			// Выполняем отключение от сервера
			this->close(aid);
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
void awh::client::Core::resolving(const size_t sid, const string & ip, const int family, const size_t did) noexcept {
	// Если идентификатор схемы сети передан
	if(sid > 0){
		// Выполняем поиск идентификатора схемы сети
		auto it = this->schemes.find(sid);
		// Если идентификатор схемы сети найден
		if(it != this->schemes.end()){
			// Получаем объект схемы сети
			scheme_t * shm = (scheme_t *) const_cast <awh::scheme_t *> (it->second);
			// Если IP адрес получен
			if(!ip.empty()){
				// Если прокси-сервер активен
				if(shm->isProxy())
					// Запоминаем полученный IP адрес для прокси-сервера
					shm->proxy.url.ip = ip;
				// Запоминаем полученный IP адрес
				else shm->url.ip = ip;
				// Определяем режим работы клиента
				switch((uint8_t) shm->status.wait){
					// Если режим работы клиента - это подключение
					case (uint8_t) scheme_t::mode_t::CONNECT:
						// Выполняем новое подключение к серверу
						this->connect(shm->sid);
					break;
					// Если режим работы клиента - это переподключение
					case (uint8_t) scheme_t::mode_t::RECONNECT:
						// Выполняем ещё одну попытку переподключиться к серверу
						this->createTimeout(shm->sid, scheme_t::mode_t::CONNECT);
					break;
				}
				// Выходим из функции
				return;
			// Если IP адрес не получен но нужно поддерживать постоянное подключение
			} else if(shm->alive) {
				// Если ожидание переподключения не остановлено ранее
				if(shm->status.wait != scheme_t::mode_t::DISCONNECT)
					// Выполняем ещё одну попытку переподключиться к серверу
					this->createTimeout(shm->sid, scheme_t::mode_t::RECONNECT);
				// Выходим из функции, чтобы попытаться подключиться ещё раз
				return;
			}
			// Если функция обратного вызова установлена
			if(shm->callback.is("disconnect"))
				// Выполняем функцию обратного вызова
				shm->callback.call <const size_t, const size_t, awh::core_t *> ("disconnect", 0, shm->sid, this);
		}
	}
}
/**
 * bandWidth Метод установки пропускной способности сети
 * @param aid   идентификатор адъютанта
 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
 */
void awh::client::Core::bandWidth(const size_t aid, const string & read, const string & write) noexcept {
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
				(!write.empty() ? this->fmk->sizeBuffer(write) : 0), 1
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
 * Core Конструктор
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
 * @param sonet  тип сокета подключения (TCP / UDP)
 */
awh::client::Core::Core(const fmk_t * fmk, const log_t * log, const scheme_t::family_t family, const scheme_t::sonet_t sonet) noexcept : awh::core_t(fmk, log, family, sonet), _async(false) {
	// Устанавливаем тип запускаемого ядра
	this->type = engine_t::type_t::CLIENT;
}
/**
 * Core Конструктор
 * @param affiliation принадлежность модуля
 * @param fmk         объект фреймворка
 * @param log         объект для работы с логами
 * @param family      тип протокола интернета (IPV4 / IPV6 / NIX)
 * @param sonet       тип сокета подключения (TCP / UDP)
 */
awh::client::Core::Core(const affiliation_t affiliation, const fmk_t * fmk, const log_t * log, const scheme_t::family_t family, const scheme_t::sonet_t sonet) noexcept : awh::core_t(affiliation, fmk, log, family, sonet), _async(false) {
	// Устанавливаем тип запускаемого ядра
	this->type = engine_t::type_t::CLIENT;
}
/**
 * ~Core Деструктор
 */
awh::client::Core::~Core() noexcept {
	// Выполняем остановку клиента
	this->stop();
}
