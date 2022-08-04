/**
 * @file: client.cpp
 * @date: 2021-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2021
 */

// Подключаем заголовочный файл
#include <core/client.hpp>

/**
 * callback Функция обратного вызова
 * @param timer   объект события таймера
 * @param revents идентификатор события
 */
void awh::client::Core::Timeout::callback(ev::timer & timer, int revents) noexcept {
	// Останавливаем работу таймера
	timer.stop();
	// Выполняем поиск воркера
	auto it = this->core->workers.find(this->wid);
	// Если воркер найден
	if(it != this->core->workers.end()){
		// Флаг запрещения выполнения операции
		bool disallow = false;
		// Если в воркере есть подключённые клиенты
		if(!it->second->adjutants.empty()){
			// Выполняем перебор всех подключенных адъютантов
			for(auto & adjutant : it->second->adjutants){
				// Если блокировка адъютанта не установлена
				disallow = (this->core->locking.count(adjutant.first) > 0);
				// Если в списке есть заблокированные адъютанты, выходим из цикла
				if(disallow) break;
			}
		}
		// Если разрешено выполнять дальнейшую операцию
		if(!disallow){
			// Определяем режим работы клиента
			switch((uint8_t) this->mode){
				// Если режим работы клиента - это подключение
				case (uint8_t) worker_t::mode_t::CONNECT:
					// Выполняем новое подключение
					this->core->connect(this->wid);
				break;
				// Если режим работы клиента - это переподключение
				case (uint8_t) worker_t::mode_t::RECONNECT: {
					// Получаем объект воркера
					worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (it->second);
					// Устанавливаем флаг ожидания статуса
					wrk->status.wait = worker_t::mode_t::DISCONNECT;
					// Выполняем новую попытку подключиться
					this->core->reconnect(wrk->wid);
				} break;
			}
		}
	}
}
/**
 * resolver Функция выполнения резолвинга домена
 * @param ip  полученный IP адрес
 * @param wrk объект воркера
 */
void awh::client::Core::resolver(const string & ip, worker_t * wrk) noexcept {
	// Если IP адрес получен
	if(!ip.empty()){
		// Если прокси-сервер активен
		if(wrk->isProxy())
			// Запоминаем полученный IP адрес для прокси-сервера
			wrk->proxy.url.ip = ip;
		// Запоминаем полученный IP адрес
		else wrk->url.ip = ip;
		// Получаем объект ядра подключения
		core_t * core = (core_t *) const_cast <awh::core_t *> (wrk->core);
		// Определяем режим работы клиента
		switch((uint8_t) wrk->status.wait){
			// Если режим работы клиента - это подключение
			case (uint8_t) worker_t::mode_t::CONNECT:
				// Выполняем новое подключение к серверу
				core->connect(wrk->wid);
			break;
			// Если режим работы клиента - это переподключение
			case (uint8_t) worker_t::mode_t::RECONNECT:
				// Выполняем ещё одну попытку переподключиться к серверу
				core->createTimeout(wrk->wid, worker_t::mode_t::CONNECT);
			break;
		}
		// Выходим из функции
		return;
	// Если IP адрес не получен но нужно поддерживать постоянное подключение
	} else if(wrk->alive) {
		// Если ожидание переподключения не остановлено ранее
		if(wrk->status.wait != worker_t::mode_t::DISCONNECT){
			// Получаем объект ядра подключения
			core_t * core = (core_t *) const_cast <awh::core_t *> (wrk->core);
			// Выполняем ещё одну попытку переподключиться к серверу
			core->createTimeout(wrk->wid, worker_t::mode_t::RECONNECT);
		}
		// Выходим из функции, чтобы попытаться подключиться ещё раз
		return;
	}
	// Выводим функцию обратного вызова
	if(wrk->disconnectFn != nullptr) wrk->disconnectFn(0, wrk->wid, const_cast <awh::core_t *> (wrk->core));
}
/**
 * connect Метод создания подключения к удаленному серверу
 * @param wid идентификатор воркера
 */
void awh::client::Core::connect(const size_t wid) noexcept {
	// Если объект фреймворка существует
	if((this->fmk != nullptr) && (wid > 0)){
		// Выполняем поиск воркера
		auto it = this->workers.find(wid);
		// Если воркер найден
		if(it != this->workers.end()){
			// Получаем объект воркера
			worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (it->second);
			// Если подключение ещё не выполнено и выполнение работ разрешено
			if((wrk->status.real == worker_t::mode_t::DISCONNECT) && (wrk->status.work == worker_t::work_t::ALLOW)){
				// Запрещаем выполнение работы
				wrk->status.work = worker_t::work_t::DISALLOW;
				// Устанавливаем флаг ожидания статуса
				wrk->status.wait = worker_t::mode_t::DISCONNECT;
				// Устанавливаем статус подключения
				wrk->status.real = worker_t::mode_t::PRECONNECT;
				// Получаем URL параметры запроса
				const uri_t::url_t & url = (wrk->isProxy() ? wrk->proxy.url : wrk->url);
				// Если в воркере есть подключённые клиенты
				if(!wrk->adjutants.empty()){
					// Переходим по всему списку адъютанта
					for(auto it = wrk->adjutants.begin(); it != wrk->adjutants.end();){
						// Если блокировка адъютанта не установлена
						if(this->locking.count(it->first) < 1){
							// Получаем объект адъютанта
							awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (it->second.get());
							// Выполняем очистку буфера событий
							this->clean(it->first);
							// Выполняем очистку контекста двигателя
							adj->engine.clear();
							// Удаляем адъютанта из списка подключений
							this->adjutants.erase(it->first);
							// Удаляем адъютанта из списка
							it = wrk->adjutants.erase(it);
						// Если есть хотябы один заблокированный элемент, выходим
						} else {
							// Устанавливаем статус подключения
							wrk->status.real = worker_t::mode_t::DISCONNECT;
							// Выходим из функции
							return;
						}
					}
				}
				// Создаём бъект адъютанта
				unique_ptr <awh::worker_t::adj_t> adj(new awh::worker_t::adj_t(wrk, this->fmk, this->log));
				// Определяем тип протокола подключения
				switch((uint8_t) this->net.family){
					// Если тип протокола подключения IPv4
					case (uint8_t) family_t::IPV4:
						// Устанавливаем сеть, для выхода в интернет
						adj->addr.network.assign(
							this->net.v4.first.begin(),
							this->net.v4.first.end()
						);
					break;
					// Если тип протокола подключения IPv6
					case (uint8_t) family_t::IPV6:
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
					case (uint8_t) sonet_t::UDP:
						// Устанавливаем параметры сокета
						adj->addr.sonet(SOCK_DGRAM, IPPROTO_UDP);
					break;
					// Если тип сокета TCP
					case (uint8_t) sonet_t::TCP:
						// Устанавливаем параметры сокета
						adj->addr.sonet(SOCK_STREAM, IPPROTO_TCP);
					break;
				}
				// Если unix-сокет используется
				if(this->net.family == family_t::NIX)
					// Выполняем инициализацию сокета
					adj->addr.init(this->net.filename, engine_t::type_t::CLIENT);
				// Если unix-сокет не используется, выполняем инициализацию сокета
				else adj->addr.init(url.ip, url.port, (this->net.family == family_t::IPV6 ? AF_INET6 : AF_INET), engine_t::type_t::CLIENT);
				// Если сокет подключения получен
				if(adj->addr.fd > -1){
					// Устанавливаем идентификатор адъютанта
					adj->aid = this->fmk->unixTimestamp();


					// Выполняем получение контекста сертификата
					this->engine.wrap1(adj->engine, &adj->addr, engine_t::type_t::CLIENT);

					// Выполняем получение контекста сертификата
					// this->engine.wrap(adj->engine, &adj->addr, url);
					// Если подключение не обёрнуто
					if(adj->addr.fd < 0){
						// Запрещаем чтение данных с сервера
						adj->bev.locked.read = true;
						// Запрещаем запись данных на сервер
						adj->bev.locked.write = true;
						// Разрешаем выполнение работы
						wrk->status.work = worker_t::work_t::ALLOW;
						// Устанавливаем статус подключения
						wrk->status.real = worker_t::mode_t::DISCONNECT;
						// Устанавливаем флаг ожидания статуса
						wrk->status.wait = worker_t::mode_t::DISCONNECT;
						// Выводим сообщение об ошибке
						this->log->print("wrap engine context is failed", log_t::flag_t::CRITICAL);
						// Выполняем переподключение
						this->reconnect(wid);
						// Выходим из функции
						return;
					}
					// Выполняем блокировку потока
					this->mtx.connect.lock();
					// Добавляем созданного адъютанта в список адъютантов
					auto ret = wrk->adjutants.emplace(adj->aid, move(adj));
					// Добавляем адъютанта в список подключений
					this->adjutants.emplace(ret.first->first, ret.first->second.get());
					// Выполняем блокировку потока
					this->mtx.connect.unlock();
					// Если подключение к серверу не выполнено
					if(!ret.first->second->addr.connect()){
						// Запрещаем чтение данных с сервера
						ret.first->second->bev.locked.read = true;
						// Запрещаем запись данных на сервер
						ret.first->second->bev.locked.write = true;
						// Разрешаем выполнение работы
						wrk->status.work = worker_t::work_t::ALLOW;
						// Устанавливаем статус подключения
						wrk->status.real = worker_t::mode_t::DISCONNECT;
						// Если unix-сокет используется
						if(this->net.family == family_t::NIX)
							// Выводим ионформацию об обрыве подключении по unix-сокету
							this->log->print("connecting to socket = %s", log_t::flag_t::CRITICAL, this->net.filename.c_str());
						// Выводим ионформацию об обрыве подключении по хосту и порту
						else this->log->print("connecting to host = %s, port = %u", log_t::flag_t::CRITICAL, url.ip.c_str(), url.port);
						/*
						// Определяем тип протокола подключения
						switch((uint8_t) this->net.family){
							// Если тип протокола подключения IPv4
							case (uint8_t) family_t::IPV4: {
								// Выполняем сброс кэша резолвера
								this->dns4.flush();
								// Добавляем бракованный IPv4 адрес в список адресов
								this->dns4.setToBlackList(url.ip); 
							} break;
							// Если тип протокола подключения IPv6
							case (uint8_t) family_t::IPV6: {
								// Выполняем сброс кэша резолвера
								this->dns6.flush();
								// Добавляем бракованный IPv6 адрес в список адресов
								this->dns6.setToBlackList(url.ip);
							} break;
						}
						*/
						// Выполняем отключение от сервера
						this->close(ret.first->first);
						// Выходим из функции
						return;
					}

					this->engine.wrap3(ret.first->second->engine);

					// Разрешаем выполнение работы
					wrk->status.work = worker_t::work_t::ALLOW;
					// Если статус подключения изменился
					if(wrk->status.real != worker_t::mode_t::PRECONNECT){
						// Запрещаем чтение данных с сервера
						ret.first->second->bev.locked.read = true;
						// Запрещаем запись данных на сервер
						ret.first->second->bev.locked.write = true;
					// Если статус подключения не изменился
					} else {
						// Активируем ожидание подключения
						this->enabled(method_t::CONNECT, ret.first->first);
						// Если разрешено выводить информационные сообщения
						if(!this->noinfo){
							// Если unix-сокет используется
							if(this->net.family == family_t::NIX)
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
					if(this->net.family == family_t::NIX)
						// Выводим ионформацию об неудачном подключении к серверу по unix-сокету
						this->log->print("client cannot be started [%s]", log_t::flag_t::CRITICAL, this->net.filename.c_str());
					// Выводим ионформацию об неудачном подключении к серверу по хосту и порту
					else this->log->print("client cannot be started [%s:%u]", log_t::flag_t::CRITICAL, url.ip.c_str(), url.port);
				}
				// Если нужно выполнить автоматическое переподключение
				if(wrk->alive){
					// Разрешаем выполнение работы
					wrk->status.work = worker_t::work_t::ALLOW;
					// Устанавливаем статус подключения
					wrk->status.real = worker_t::mode_t::DISCONNECT;
					// Устанавливаем флаг ожидания статуса
					wrk->status.wait = worker_t::mode_t::DISCONNECT;
					// Выполняем переподключение
					this->reconnect(wid);
					// Выходим из функции
					return;
				// Если все попытки исчерпаны
				} else {
					// Разрешаем выполнение работы
					wrk->status.work = worker_t::work_t::ALLOW;
					// Устанавливаем статус подключения
					wrk->status.real = worker_t::mode_t::DISCONNECT;
					/*
					// Определяем тип подключения
					switch((uint8_t) this->net.family){
						// Если тип протокола подключения IPv4
						case (uint8_t) family_t::IPV4: {
							// Выполняем сброс кэша резолвера
							this->dns4.flush();
							// Добавляем бракованный IPv4 адрес в список адресов
							this->dns4.setToBlackList(url.ip); 
						} break;
						// Если тип протокола подключения IPv6
						case (uint8_t) family_t::IPV6: {
							// Выполняем сброс кэша резолвера
							this->dns6.flush();
							// Добавляем бракованный IPv6 адрес в список адресов
							this->dns6.setToBlackList(url.ip);
						} break;
					}
					*/
					// Выводим сообщение об ошибке
					if(!this->noinfo) this->log->print("%s", log_t::flag_t::INFO, "disconnected from the server");
					// Выводим функцию обратного вызова
					if(wrk->disconnectFn != nullptr) wrk->disconnectFn(0, wrk->wid, this);
				}
			}
		}
	}
}
/**
 * reconnect Метод восстановления подключения
 * @param wid идентификатор воркера
 */
void awh::client::Core::reconnect(const size_t wid) noexcept {
	// Выполняем поиск воркера
	auto it = this->workers.find(wid);
	// Если воркер найден
	if(it != this->workers.end()){
		// Получаем объект воркера
		worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (it->second);
		// Если параметры URL запроса переданы и выполнение работы разрешено
		if(!wrk->url.empty() && (wrk->status.wait == worker_t::mode_t::DISCONNECT) && (wrk->status.work == worker_t::work_t::ALLOW)){
			// Определяем тип протокола подключения
			switch((uint8_t) this->net.family){
				// Если тип протокола подключения IPv4
				case (uint8_t) family_t::IPV4:
				// Если тип протокола подключения IPv6
				case (uint8_t) family_t::IPV6: {
					// Устанавливаем флаг ожидания статуса
					wrk->status.wait = worker_t::mode_t::RECONNECT;
					// Получаем URL параметры запроса
					const uri_t::url_t & url = (wrk->isProxy() ? wrk->proxy.url : wrk->url);

					/*
					// Структура определяющая тип адреса
					struct sockaddr_in serv_addr;

					#if defined(_WIN32) || defined(_WIN64)
						// Выполняем резолвинг доменного имени
						struct hostent * server = gethostbyname(url.domain.c_str());
					#else
						// Выполняем резолвинг доменного имени
						struct hostent * server = gethostbyname2(url.domain.c_str(), AF_INET);
					#endif

					// Заполняем структуру типа адреса нулями
					memset(&serv_addr, 0, sizeof(serv_addr));
					// Устанавливаем что удаленный адрес это ИНТЕРНЕТ
					serv_addr.sin_family = AF_INET;
					// Выполняем копирование данных типа подключения
					memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
					// Получаем IP адрес
					char * ip = inet_ntoa(serv_addr.sin_addr);

					printf("IP address: %s\n", ip);
					
					
					const_cast <uri_t::url_t *> (&url)->ip = ip;
					*/
					

					// Выполняем запуск системы
					resolver(url.ip, wrk);

					/*
					// Определяем тип протокола подключения
					switch((uint8_t) this->net.family){
						// Если тип протокола подключения IPv4
						case (uint8_t) family_t::IPV4:
							// Выполняем резолвинг домена
							wrk->did = this->dns4.resolve(wrk, (!url.domain.empty() ? url.domain : url.ip), AF_INET, &resolver);
						break;
						// Если тип протокола подключения IPv6
						case (uint8_t) family_t::IPV6:
							// Выполняем резолвинг домена
							wrk->did = this->dns6.resolve(wrk, (!url.domain.empty() ? url.domain : url.ip), AF_INET6, &resolver);
						break;
					}
					*/
				} break;
				// Если тип протокола подключения unix-сокет
				case (uint8_t) family_t::NIX:
					// Выполняем подключение заново
					this->connect(wrk->wid);
				break;
			}
		}
	}
}
/**
 * createTimeout Метод создания таймаута
 * @param wid  идентификатор воркера
 * @param mode режим работы клиента
 */
void awh::client::Core::createTimeout(const size_t wid, const worker_t::mode_t mode) noexcept {
	// Выполняем поиск воркера
	auto it = this->workers.find(wid);
	// Если воркер найден
	if(it != this->workers.end()){
		// Объект таймаута
		timeout_t * timeout = nullptr;
		// Выполняем поиск существующего таймаута
		auto it = this->timeouts.find(wid);
		// Если таймаут найден
		if(it != this->timeouts.end())
			// Получаем объект таймаута
			timeout = it->second.get();
		// Если таймаут ещё не существует
		else {
			// Выполняем блокировку потока
			this->mtx.timeout.lock();
			// Получаем объект таймаута
			timeout = this->timeouts.emplace(wid, unique_ptr <timeout_t> (new timeout_t)).first->second.get();
			// Выполняем разблокировку потока
			this->mtx.timeout.unlock();
		}
		// Устанавливаем идентификатор таймаута
		timeout->wid = wid;
		// Устанавливаем режим работы клиента
		timeout->mode = mode;
		// Устанавливаем ядро клиента
		timeout->core = this;
		// Устанавливаем базу событий
		timeout->timer.set(this->dispatch.base);
		// Устанавливаем функцию обратного вызова
		timeout->timer.set <timeout_t, &timeout_t::callback> (timeout);
		// Запускаем работу таймера
		timeout->timer.start(5.);
	}
}
/**
 * sendTimeout Метод отправки принудительного таймаута
 * @param aid идентификатор адъютанта
 */
void awh::client::Core::sendTimeout(const size_t aid) noexcept {
	// Если блокировка адъютанта не установлена
	if(this->locking.count(aid) < 1){
		// Если адъютант существует
		if(this->adjutants.count(aid) > 0)
			// Выполняем отключение от сервера
			this->close(aid);
		// Если адъютант не существует
		else if(!this->workers.empty()) {
			// Выполняем блокировку потока
			const lock_guard <recursive_mutex> lock(this->mtx.reset);
			// Переходим по всему списку воркеров
			for(auto & worker : this->workers){
				// Получаем объект воркера
				worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (worker.second);
				// Если выполнение работ разрешено
				if(wrk->status.work == worker_t::work_t::ALLOW)
					// Запрещаем выполнение работы
					wrk->status.work = worker_t::work_t::DISALLOW;
				// Если работы запрещены, выходим
				else return;
				// Запрещаем воркеру выполнять перезапуск
				wrk->stop = true;
			}
			// Выполняем отключение всех подключённых адъютантов
			this->close();
			// Выполняем пинок базе событий
			this->dispatch.kick();
			// Флаг поддержания постоянного подключения
			bool alive = false;
			// Переходим по всему списку воркеров
			for(auto & worker : this->workers){
				// Получаем объект воркера
				worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (worker.second);
				// Если флаг поддержания постоянного подключения не установлен
				if(!alive && wrk->alive) alive = wrk->alive;
				// Устанавливаем статус подключения
				wrk->status.real = worker_t::mode_t::DISCONNECT;
				// Устанавливаем флаг ожидания статуса
				wrk->status.wait = worker_t::mode_t::DISCONNECT;
			}
			// Если необходимо поддерживать постоянное подключение
			if(alive){
				/*
				// Определяем тип протокола подключения
				switch((uint8_t) this->net.family){
					// Если тип протокола подключения IPv4
					case (uint8_t) family_t::IPV4: {
						// Добавляем базу событий для DNS резолвера IPv4
						this->dns4.setBase(this->dispatch.base);
						// Выполняем установку нейм-серверов для DNS резолвера IPv4
						this->dns4.replaceServers(this->net.v4.second);
					} break;
					// Если тип протокола подключения IPv6
					case (uint8_t) family_t::IPV6: {
						// Добавляем базу событий для DNS резолвера IPv6
						this->dns4.setBase(this->dispatch.base);
						// Выполняем установку нейм-серверов для DNS резолвера IPv6
						this->dns4.replaceServers(this->net.v4.second);
					} break;
				}
				*/
			}
			// Переходим по всему списку воркеров
			for(auto & worker : this->workers){
				// Получаем объект воркера
				worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (worker.second);
				// Разрешаем воркеру выполнять перезапуск
				wrk->stop = false;
				// Если выполнение работ запрещено
				if(wrk->status.work == worker_t::work_t::DISALLOW)
					// Разрешаем выполнение работы
					wrk->status.work = worker_t::work_t::ALLOW;
				// Если нужно выполнить автоматическое переподключение, выполняем новую попытку
				if(wrk->alive) this->reconnect(wrk->wid);
			}
		}
	}
}
/**
 * clearTimeout Метод удаления установленного таймаута
 * @param wid идентификатор воркера
 */
void awh::client::Core::clearTimeout(const size_t wid) noexcept {
	// Если список таймеров не пустой
	if(!this->timeouts.empty()){
		// Выполняем поиск таймера
		auto it = this->timeouts.find(wid);
		// Если таймер найден
		if(it != this->timeouts.end()){
			// Выполняем блокировку потока
			this->mtx.timeout.lock();
			// Останавливаем работу таймера
			it->second->timer.stop();
			// Выполняем разблокировку потока
			this->mtx.timeout.unlock();
		}
	}
}
/**
 * close Метод отключения всех воркеров
 */
void awh::client::Core::close() noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.close);
	// Если список активных таймеров существует
	if(!this->timeouts.empty()){
		// Переходим по всему списку активных таймеров
		for(auto & timeout : this->timeouts)
			// Останавливаем работу таймера
			timeout.second->timer.stop();
	}
	// Если список воркеров активен
	if(!this->workers.empty()){
		// Переходим по всему списку воркеров
		for(auto & worker : this->workers){
			// Если в воркере есть подключённые клиенты
			if(!worker.second->adjutants.empty()){
				// Получаем объект воркера
				worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (worker.second);
				// Устанавливаем флаг ожидания статуса
				wrk->status.wait = worker_t::mode_t::DISCONNECT;
				// Устанавливаем статус сетевого ядра
				wrk->status.real = worker_t::mode_t::DISCONNECT;
				// Если прокси-сервер активирован но уже переключён на работу с сервером
				if((wrk->proxy.type != proxy_t::type_t::NONE) && !wrk->isProxy())
					// Выполняем переключение обратно на прокси-сервер
					wrk->switchConnect();
				// Переходим по всему списку адъютанта
				for(auto it = wrk->adjutants.begin(); it != wrk->adjutants.end();){
					// Если блокировка адъютанта не установлена
					if(this->locking.count(it->first) < 1){
						// Выполняем блокировку адъютанта
						this->locking.emplace(it->first);
						// Получаем объект адъютанта
						awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (it->second.get());
						// Выполняем очистку буфера событий
						this->clean(it->first);
						// Выполняем очистку контекста двигателя
						adj->engine.clear();
						// Удаляем адъютанта из списка подключений
						this->adjutants.erase(it->first);
						// Выводим функцию обратного вызова
						if(wrk->disconnectFn != nullptr)
							// Выполняем функцию обратного вызова
							wrk->disconnectFn(it->first, worker.first, this);
						// Удаляем блокировку адъютанта
						this->locking.erase(it->first);
						// Удаляем адъютанта из списка
						it = wrk->adjutants.erase(it);
					// Иначе продолжаем дальше
					} else ++it;
				}
			}
		}
	}
}
/**
 * remove Метод удаления всех воркеров
 */
void awh::client::Core::remove() noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.close);
	// Если список воркеров активен
	if(!this->workers.empty()){
		// Если список активных таймеров существует
		if(!this->timeouts.empty()){
			// Переходим по всему списку активных таймеров
			for(auto it = this->timeouts.begin(); it != this->timeouts.end();){
				// Выполняем блокировку потока
				this->mtx.timeout.lock();
				// Останавливаем работу таймера
				it->second->timer.stop();
				// Выполняем удаление текущего таймаута
				it = this->timeouts.erase(it);
				// Выполняем разблокировку потока
				this->mtx.timeout.unlock();
			}
		}
		// Переходим по всему списку воркеров
		for(auto it = this->workers.begin(); it != this->workers.end();){
			// Получаем объект воркера
			worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (it->second);
			// Устанавливаем флаг ожидания статуса
			wrk->status.wait = worker_t::mode_t::DISCONNECT;
			// Устанавливаем статус сетевого ядра
			wrk->status.real = worker_t::mode_t::DISCONNECT;
			// Если в воркере есть подключённые клиенты
			if(!wrk->adjutants.empty()){
				// Переходим по всему списку адъютанта
				for(auto jt = wrk->adjutants.begin(); jt != wrk->adjutants.end();){
					// Если блокировка адъютанта не установлена
					if(this->locking.count(jt->first) < 1){
						// Выполняем блокировку адъютанта
						this->locking.emplace(jt->first);
						// Получаем объект адъютанта
						awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (jt->second.get());
						// Выполняем очистку буфера событий
						this->clean(jt->first);
						// Выполняем очистку контекста двигателя
						adj->engine.clear();
						// Удаляем адъютанта из списка подключений
						this->adjutants.erase(jt->first);
						// Выводим функцию обратного вызова
						if(wrk->disconnectFn != nullptr)
							// Выполняем функцию обратного вызова
							wrk->disconnectFn(jt->first, it->first, this);
						// Удаляем блокировку адъютанта
						this->locking.erase(jt->first);
						// Удаляем адъютанта из списка
						jt = wrk->adjutants.erase(jt);
					// Иначе продолжаем дальше
					} else ++jt;
				}
			}
			// Выполняем удаление воркера
			it = this->workers.erase(it);
		}
	}
}
/**
 * open Метод открытия подключения воркером
 * @param wid идентификатор воркера
 */
void awh::client::Core::open(const size_t wid) noexcept {
	// Если идентификатор воркера передан
	if(wid > 0){
		// Выполняем поиск воркера
		auto it = this->workers.find(wid);
		// Если воркер найден
		if(it != this->workers.end()){
			// Получаем объект воркера
			worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (it->second);
			// Если параметры URL запроса переданы и выполнение работы разрешено
			if(!wrk->url.empty() && (wrk->status.wait == worker_t::mode_t::DISCONNECT) && (wrk->status.work == worker_t::work_t::ALLOW)){
				// Определяем тип протокола подключения
				switch((uint8_t) this->net.family){
					// Если тип протокола подключения IPv4
					case (uint8_t) family_t::IPV4:
					// Если тип протокола подключения IPv6
					case (uint8_t) family_t::IPV6: {
						// Устанавливаем флаг ожидания статуса
						wrk->status.wait = worker_t::mode_t::CONNECT;
						// Получаем URL параметры запроса
						const uri_t::url_t & url = (wrk->isProxy() ? wrk->proxy.url : wrk->url);

						/*
						// Структура определяющая тип адреса
						struct sockaddr_in serv_addr;

						#if defined(_WIN32) || defined(_WIN64)
							// Выполняем резолвинг доменного имени
							struct hostent * server = gethostbyname(url.domain.c_str());
						#else
							// Выполняем резолвинг доменного имени
							struct hostent * server = gethostbyname2(url.domain.c_str(), AF_INET);
						#endif

						// Заполняем структуру типа адреса нулями
						memset(&serv_addr, 0, sizeof(serv_addr));
						// Устанавливаем что удаленный адрес это ИНТЕРНЕТ
						serv_addr.sin_family = AF_INET;
						// Выполняем копирование данных типа подключения
						memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
						// Получаем IP адрес
						char * ip = inet_ntoa(serv_addr.sin_addr);

						printf("IP address: %s\n", ip);

						const_cast <uri_t::url_t *> (&url)->ip = ip;
						*/
						
						
						
						// Выполняем запуск системы
						resolver(url.ip, wrk);

						/*
						// Если IP адрес не получен
						if(url.ip.empty() && !url.domain.empty())
							// Определяем тип протокола подключения
							switch((uint8_t) this->net.family){
								// Если тип протокола подключения IPv4
								case (uint8_t) family_t::IPV4:
									// Выполняем резолвинг доменного имени
									wrk->did = this->dns4.resolve(wrk, url.domain, AF_INET, &resolver);
								break;
								// Если тип протокола подключения IPv6
								case (uint8_t) family_t::IPV6:
									// Выполняем резолвинг доменного имени
									wrk->did = this->dns6.resolve(wrk, url.domain, AF_INET6, &resolver);
								break;
							}
						// Выполняем запуск системы
						else if(!url.ip.empty()) resolver(url.ip, wrk);
						*/
					} break;
					// Если тип протокола подключения unix-сокет
					case (uint8_t) family_t::NIX:
						// Выполняем подключение заново
						this->connect(wrk->wid);
					break;
				}
			}
		}
	}
}
/**
 * remove Метод удаления воркера из биндинга
 * @param wid идентификатор воркера
 */
void awh::client::Core::remove(const size_t wid) noexcept {
	// Если идентификатор воркера передан
	if(wid > 0){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->mtx.close);
		// Выполняем поиск воркера
		auto it = this->workers.find(wid);
		// Если воркер найден
		if(it != this->workers.end()){
			// Выполняем удаление уоркера из списка
			this->workers.erase(it);
			// Выполняем поиск активного таймаута
			auto it = this->timeouts.find(wid);
			// Если таймаут найден, удаляем его
			if(it != this->timeouts.end()){
				// Выполняем блокировку потока
				this->mtx.timeout.lock();
				// Останавливаем работу таймера
				it->second->timer.stop();
				// Выполняем удаление текущего таймаута
				this->timeouts.erase(it);
				// Выполняем разблокировку потока
				this->mtx.timeout.unlock();
			}
		}
	}
}
/**
 * close Метод закрытия подключения воркера
 * @param aid идентификатор адъютанта
 */
void awh::client::Core::close(const size_t aid) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.close);
	// Если блокировка адъютанта не установлена
	if(this->locking.count(aid) < 1){
		// Выполняем блокировку адъютанта
		this->locking.emplace(aid);
		// Выполняем извлечение адъютанта
		auto it = this->adjutants.find(aid);
		// Если адъютант получен
		if(it != this->adjutants.end()){
			// Получаем объект адъютанта
			awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (it->second);
			// Получаем объект воркера
			worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (adj->parent);
			// Получаем объект ядра клиента
			const core_t * core = reinterpret_cast <const core_t *> (wrk->core);
			// Выполняем очистку буфера событий
			this->clean(aid);
			// Удаляем установленный таймаут, если он существует
			this->clearTimeout(wrk->wid);
			// Если прокси-сервер активирован но уже переключён на работу с сервером
			if((wrk->proxy.type != proxy_t::type_t::NONE) && !wrk->isProxy())
				// Выполняем переключение обратно на прокси-сервер
				wrk->switchConnect();
			// Выполняем очистку контекста двигателя
			adj->engine.clear();
			// Удаляем адъютанта из списка адъютантов
			wrk->adjutants.erase(aid);
			// Удаляем адъютанта из списка подключений
			this->adjutants.erase(aid);
			// Устанавливаем флаг ожидания статуса
			wrk->status.wait = worker_t::mode_t::DISCONNECT;
			// Устанавливаем статус сетевого ядра
			wrk->status.real = worker_t::mode_t::DISCONNECT;
			// Если не нужно выполнять принудительную остановку работы воркера
			if(!wrk->stop){
				// Если нужно выполнить автоматическое переподключение
				if(wrk->alive) this->reconnect(wrk->wid);
				// Если автоматическое подключение выполнять не нужно
				else {
					// Выводим сообщение об ошибке
					if(!core->noinfo) this->log->print("%s", log_t::flag_t::INFO, "disconnected from the server");
					// Выводим функцию обратного вызова
					if(wrk->disconnectFn != nullptr) wrk->disconnectFn(aid, wrk->wid, this);
				}
			}
		}
		// Удаляем блокировку адъютанта
		this->locking.erase(aid);
	}
}
/**
 * switchProxy Метод переключения с прокси-сервера
 * @param aid идентификатор адъютанта
 */
void awh::client::Core::switchProxy(const size_t aid) noexcept {
	// Если подключение производится по IPv4 или IPv6 и по хосту с портом
	if((this->net.sonet == sonet_t::TCP) && ((this->net.family == family_t::IPV4) || (this->net.family == family_t::IPV6))){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->mtx.proxy);
		// Выполняем извлечение адъютанта
		auto it = this->adjutants.find(aid);
		// Если адъютант получен
		if(it != this->adjutants.end()){
			// Получаем объект адъютанта
			awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (it->second);
			// Получаем объект воркера
			worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (adj->parent);
			// Если прокси-сервер активирован но ещё не переключён на работу с сервером
			if((wrk->proxy.type != proxy_t::type_t::NONE) && wrk->isProxy()){
				// Выполняем переключение на работу с сервером
				wrk->switchConnect();
				// Выполняем получение контекста сертификата
				this->engine.wrap(adj->engine, adj->engine, wrk->url);
				// Если подключение не обёрнуто
				if(adj->addr.fd < 0){
					// Выводим сообщение об ошибке
					this->log->print("wrap engine context is failed", log_t::flag_t::CRITICAL);
					// Выходим из функции
					return;
				}
				// Останавливаем чтение данных
				this->disabled(method_t::READ, it->first);
				// Останавливаем запись данных
				this->disabled(method_t::WRITE, it->first);
				// Активируем ожидание подключения
				this->enabled(method_t::CONNECT, it->first);
			}
		}
	}
}
/**
 * timeout Функция обратного вызова при срабатывании таймаута
 * @param aid идентификатор адъютанта
 */
void awh::client::Core::timeout(const size_t aid) noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Получаем объект адъютанта
		awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (it->second);
		// Получаем объект подключения
		worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (adj->parent);
		// Определяем тип протокола подключения
		switch((uint8_t) this->net.family){
			// Если тип протокола подключения IPv4
			case (uint8_t) family_t::IPV4:
			// Если тип протокола подключения IPv6
			case (uint8_t) family_t::IPV6: {
				// Получаем URL параметры запроса
				const uri_t::url_t & url = (wrk->isProxy() ? wrk->proxy.url : wrk->url);
				// Если данные ещё ни разу не получены
				if(!wrk->acquisition && !url.ip.empty()){
					/*
					// Определяем тип протокола подключения
					switch((uint8_t) this->net.family){
						// Резолвер IPv4, добавляем бракованный IPv4 адрес в список адресов
						case (uint8_t) family_t::IPV4: this->dns4.setToBlackList(url.ip); break;
						// Резолвер IPv6, добавляем бракованный IPv6 адрес в список адресов
						case (uint8_t) family_t::IPV6: this->dns6.setToBlackList(url.ip); break;
					}
					*/
				}			
				// Выводим сообщение в лог, о таймауте подключения
				this->log->print("timeout host %s [%s%d]", log_t::flag_t::WARNING, url.domain.c_str(), (!url.ip.empty() ? (url.ip + ":").c_str() : ""), url.port);
			} break;
			// Если тип протокола подключения unix-сокет
			case (uint8_t) family_t::NIX:
				// Выводим сообщение в лог, о таймауте подключения
				this->log->print("timeout host %s", log_t::flag_t::WARNING, this->net.filename.c_str());
			break;
		}
		// Останавливаем чтение данных
		this->disabled(method_t::READ, it->first);
		// Останавливаем запись данных
		this->disabled(method_t::WRITE, it->first);
		// Выполняем отключение от сервера
		this->close(aid);
	}
}
/**
 * connected Функция обратного вызова при удачном подключении к серверу
 * @param aid идентификатор адъютанта
 */
void awh::client::Core::connected(const size_t aid) noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Получаем объект адъютанта
		awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (it->second);
		// Получаем объект подключения
		worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (adj->parent);
		// Если подключение удачное и работа воркера разрешена
		if(wrk->status.work == worker_t::work_t::ALLOW){
			// Снимаем флаг получения данных
			wrk->acquisition = false;
			// Устанавливаем статус подключения к серверу
			wrk->status.real = worker_t::mode_t::CONNECT;
			// Устанавливаем флаг ожидания статуса
			wrk->status.wait = worker_t::mode_t::DISCONNECT;
			// Выполняем очистку существующих таймаутов
			this->clearTimeout(wrk->wid);
			// Определяем тип протокола подключения
			switch((uint8_t) this->net.family){
				// Если тип протокола подключения IPv4
				case (uint8_t) family_t::IPV4:
				// Если тип протокола подключения IPv6
				case (uint8_t) family_t::IPV6: {
					// Получаем URL параметры запроса
					const uri_t::url_t & url = (wrk->isProxy() ? wrk->proxy.url : wrk->url);
					// Получаем хост сервера
					const string & host = (!url.ip.empty() ? url.ip : url.domain);

					/*
					// Определяем тип протокола подключения
					switch((uint8_t) this->net.family){
						// Резолвер IPv4, создаём резолвер
						case (uint8_t) family_t::IPV4:
							// Выполняем отмену ранее выполненных запросов DNS
							this->dns4.cancel(wrk->did);
						break;
						// Резолвер IPv6, создаём резолвер
						case (uint8_t) family_t::IPV6:
							// Выполняем отмену ранее выполненных запросов DNS
							this->dns6.cancel(wrk->did);
						break;
					}
					*/

					// Запускаем чтение данных
					this->enabled(method_t::READ, it->first);
					// Выводим в лог сообщение
					if(!this->noinfo) this->log->print("connect client to server [%s:%d]", log_t::flag_t::INFO, host.c_str(), url.port);
					// Если подключение производится через, прокси-сервер
					if(wrk->isProxy()){
						// Выполняем функцию обратного вызова для прокси-сервера
						if(wrk->connectProxyFn != nullptr) wrk->connectProxyFn(it->first, wrk->wid, const_cast <awh::core_t *> (wrk->core));
					// Выполняем функцию обратного вызова
					} else if(wrk->connectFn != nullptr) wrk->connectFn(it->first, wrk->wid, const_cast <awh::core_t *> (wrk->core));
				} break;
				// Если тип протокола подключения unix-сокет
				case (uint8_t) family_t::NIX: {
					// Запускаем чтение данных
					this->enabled(method_t::READ, it->first);
					// Выводим в лог сообщение
					if(!this->noinfo) this->log->print("connect client to server [%s]", log_t::flag_t::INFO, this->net.filename.c_str());
					// Выполняем функцию обратного вызова
					if(wrk->connectFn != nullptr) wrk->connectFn(it->first, wrk->wid, const_cast <awh::core_t *> (wrk->core));
				} break;
			}
			// Выходим из функции
			return;
		}
		// Выполняем отключение от сервера
		this->close(it->first);
	}
}
/**
 * write Функция обратного вызова при записи данных в сокет
 * @param method метод режима работы
 * @param aid    идентификатор адъютанта
 */
void awh::client::Core::transfer(const method_t method, const size_t aid) noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Получаем объект адъютанта
		awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (it->second);
		// Получаем объект подключения
		worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (adj->parent);
		// Если подключение установлено
		if((wrk->acquisition = (wrk->status.real == worker_t::mode_t::CONNECT))){
			// Определяем метод работы
			switch((uint8_t) method){
				// Если производится чтение данных
				case (uint8_t) method_t::READ: {
					// Количество полученных байт
					int64_t bytes = -1;
					// Создаём буфер входящих данных
					char buffer[BUFFER_SIZE];
					// Если тип сокета установлен как UDP, останавливаем чтение
					if(this->net.sonet == sonet_t::UDP)
						// Останавливаем чтение данных с клиента
						adj->bev.event.read.stop();
					// Выполняем перебор бесконечным циклом пока это разрешено
					while(!adj->bev.locked.read && (wrk->status.real == worker_t::mode_t::CONNECT)){
						// Если дочерние активные подключения есть и сокет блокирующий
						if((this->cores > 0) && (adj->engine.isblock() == 1))
							// Переводим BIO в не блокирующий режим
							adj->engine.noblock();
						// Выполняем получение сообщения от клиента
						bytes = adj->engine.read(buffer, sizeof(buffer));
						// Если время ожидания чтения данных установлено
						if(wrk->wait && (adj->timeouts.read > 0)){
							// Устанавливаем время ожидания на получение данных
							adj->bev.timer.read.repeat = adj->timeouts.read;
							// Запускаем повторное ожидание
							adj->bev.timer.read.again();
						// Останавливаем таймаут ожидания на чтение из сокета
						} else adj->bev.timer.read.stop();
						// Выполняем принудительное исполнение таймеров
						if(adj->engine.isblock() != 0) this->executeTimers();
						/**
						 * Если операционной системой является MS Windows
						 */
						#if defined(_WIN32) || defined(_WIN64)
							// Запускаем чтение данных снова (Для Windows)
							if((bytes != 0) && (this->net.sonet == sonet_t::TCP))
								// Запускаем чтение снова
								adj->bev.event.read.start();
						#endif
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
									// Определяем тип протокола подключения
									switch((uint8_t) this->net.family){
										// Если тип протокола подключения IPv4
										case (uint8_t) family_t::IPV4:
										// Если тип протокола подключения IPv6
										case (uint8_t) family_t::IPV6: {
											// Если подключение производится через, прокси-сервер
											if(wrk->isProxy()){
												// Если функция обратного вызова для вывода записи существует
												if(wrk->readProxyFn != nullptr)
													// Выводим функцию обратного вызова
													wrk->readProxyFn(buffer + offset, actual, aid, wrk->wid, reinterpret_cast <awh::core_t *> (this));
											// Если прокси-сервер не используется
											} else if(wrk->readFn != nullptr)
												// Выводим функцию обратного вызова
												wrk->readFn(buffer + offset, actual, aid, wrk->wid, reinterpret_cast <awh::core_t *> (this));
										} break;
										// Если тип протокола подключения unix-сокет
										case (uint8_t) family_t::NIX: {
											// Если функция обратного вызова установлена
											if(wrk->readFn != nullptr)
												// Выводим функцию обратного вызова
												wrk->readFn(buffer + offset, actual, aid, wrk->wid, reinterpret_cast <awh::core_t *> (this));
										} break;
									}
									// Увеличиваем смещение в буфере
									offset += actual;
								}
							// Если данных достаточно
							} else {
								// Определяем тип протокола подключения
								switch((uint8_t) this->net.family){
									// Если тип протокола подключения IPv4
									case (uint8_t) family_t::IPV4:
									// Если тип протокола подключения IPv6
									case (uint8_t) family_t::IPV6: {
										// Если подключение производится через, прокси-сервер
										if(wrk->isProxy()){
											// Если функция обратного вызова для вывода записи существует
											if(wrk->readProxyFn != nullptr)
												// Выводим функцию обратного вызова
												wrk->readProxyFn(buffer, bytes, aid, wrk->wid, reinterpret_cast <awh::core_t *> (this));
										// Если прокси-сервер не используется
										} else if(wrk->readFn != nullptr)
											// Выводим функцию обратного вызова
											wrk->readFn(buffer, bytes, aid, wrk->wid, reinterpret_cast <awh::core_t *> (this));
									} break;
									// Если тип протокола подключения unix-сокет
									case (uint8_t) family_t::NIX: {
										// Если функция обратного вызова установлена
										if(wrk->readFn != nullptr)
											// Выводим функцию обратного вызова
											wrk->readFn(buffer, bytes, aid, wrk->wid, reinterpret_cast <awh::core_t *> (this));
									} break;
								}
							}
						// Если данные не могут быть прочитаны
						} else {
							// Если нужно повторить попытку
							if(bytes == -2) continue;
							// Если нужно выйти из цикла
							else if(bytes == -1) break;
							// Если нужно завершить работу
							else if(bytes == 0) {
								// Выполняем отключение от сервера
								this->close(aid);
								// Выходим из цикла
								break;
							}
						}
						// Выходим из цикла
						break;
					}
				} break;
				// Если производится запись данных
				case (uint8_t) method_t::WRITE: {
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
							bytes = adj->engine.write(adj->buffer.data() + offset, actual);
							// Если данные записаны удачно
							if(bytes > 0)
								// Добавляем записанные байты в буфер
								buffer.insert(buffer.end(), adj->buffer.data() + offset, (adj->buffer.data() + offset) + bytes);
							// Выполняем принудительное исполнение таймеров
							if(adj->engine.isblock() != 0) this->executeTimers();
							// Если время ожидания записи данных установлено
							if(adj->timeouts.write > 0){
								// Устанавливаем время ожидания на запись данных
								adj->bev.timer.write.repeat = adj->timeouts.write;
								// Запускаем повторное ожидание
								adj->bev.timer.write.again();
							// Останавливаем таймаут ожидания на запись в сокет
							} else adj->bev.timer.write.stop();
							// Если нужно повторить попытку
							if(bytes == -2) continue;
							// Если нужно выйти из цикла
							else if(bytes == -1) break;
							// Если нужно завершить работу
							else if(bytes == 0) {
								// Выполняем отключение клиента
								this->close(aid);
								// Выходим из цикла
								break;
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
						if(adj->buffer.empty()) this->disabled(method_t::WRITE, aid);
						// Если функция обратного вызова на запись данных установлена
						if(wrk->writeFn != nullptr)
							// Выводим функцию обратного вызова
							wrk->writeFn(buffer.data(), buffer.size(), aid, wrk->wid, reinterpret_cast <awh::core_t *> (this));
					// Если данных недостаточно для записи в сокет
					} else {
						// Останавливаем запись данных
						this->disabled(method_t::WRITE, aid);
						// Если функция обратного вызова на запись данных установлена
						if(wrk->writeFn != nullptr)
							// Выводим функцию обратного вызова
							wrk->writeFn(nullptr, 0, aid, wrk->wid, reinterpret_cast <awh::core_t *> (this));
					}
					// Если тип сокета установлен как UDP, и данных для записи больше нет, запускаем чтение
					if(adj->buffer.empty() && (this->net.sonet == sonet_t::UDP))
						// Запускаем чтение данных с клиента
						adj->bev.event.read.start();
				} break;
			}
		// Если подключение завершено
		} else {
			// Останавливаем чтение данных
			this->disabled(method_t::READ, it->first);
			// Останавливаем запись данных
			this->disabled(method_t::WRITE, it->first);
			// Выполняем отключение от сервера
			this->close(aid);
		}
	}
}
/**
 * setBandwidth Метод установки пропускной способности сети
 * @param aid   идентификатор адъютанта
 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
 */
void awh::client::Core::setBandwidth(const size_t aid, const string & read, const string & write) noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		/**
		 * Если операционной системой является Nix-подобная
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Получаем объект адъютанта
			awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (it->second);
			// Устанавливаем размер буфера
			adj->engine.buffer(
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
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::client::Core::Core(const fmk_t * fmk, const log_t * log) noexcept : awh::core_t(fmk, log) {
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
