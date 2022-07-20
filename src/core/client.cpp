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
 * connect Функция обратного вызова при подключении к серверу
 * @param watcher объект события подключения
 * @param revents идентификатор события
 */
void awh::worker_t::adj_t::connect(ev::io & watcher, int revents) noexcept {

	cout << " ±±±±±±±±±±±± CONNECT " << this->bev.locked.write << endl;

	// Выполняем остановку чтения
	watcher.stop();
	// Останавливаем таймаут ожидания на запись в сокет
	this->bev.timer.write.stop();
	// Получаем объект подключения
	client::worker_t * wrk = (client::worker_t *) const_cast <awh::worker_t *> (this->parent);
	// Если подключение ещё существует
	if((this->fmk != nullptr) && (wrk->core != nullptr)){
		// Получаем объект ядра клиента
		const client::core_t * core = reinterpret_cast <const client::core_t *> (wrk->core);
		// Если список адъютантов не пустой и адъютант найден
		if(!core->adjutants.empty() && (core->adjutants.count(this->aid) > 0)){
			// Получаем URL параметры запроса
			const uri_t::url_t & url = (wrk->isProxy() ? wrk->proxy.url : wrk->url);
			// Получаем хост сервера
			const string & host = (!url.ip.empty() ? url.ip : url.domain);
			// Если подключение удачное и работа воркера разрешена
			if(wrk->status.work == client::worker_t::work_t::ALLOW){
				// Снимаем флаг получения данных
				wrk->acquisition = false;
				// Устанавливаем статус подключения к серверу
				wrk->status.real = client::worker_t::mode_t::CONNECT;
				// Устанавливаем флаг ожидания статуса
				wrk->status.wait = client::worker_t::mode_t::DISCONNECT;
				// Выполняем очистку существующих таймаутов
				const_cast <client::core_t *> (core)->clearTimeout(wrk->wid);

				/*
				// Определяем тип подключения
				switch(core->net.family){
					// Резолвер IPv4, создаём резолвер
					case AF_INET:
						// Выполняем отмену ранее выполненных запросов DNS
						const_cast <client::core_t *> (core)->dns4.cancel(wrk->did);
					break;
					// Резолвер IPv6, создаём резолвер
					case AF_INET6:
						// Выполняем отмену ранее выполненных запросов DNS
						const_cast <client::core_t *> (core)->dns6.cancel(wrk->did);
					break;
				}
				*/

				cout << " $$$$$$$$$$$$$$$$$$$$$$ SOCKET1= " << this->bev.socket << endl;

				// Устанавливаем сокет для записи
				this->bev.event.read.set(this->bev.socket, ev::READ);
				// Устанавливаем сокет для записи
				this->bev.event.write.set(this->bev.socket, ev::WRITE);
				// Устанавливаем базу событий
				this->bev.event.read.set(const_cast <client::core_t *> (core)->base);
				// Устанавливаем базу событий
				this->bev.event.write.set(const_cast <client::core_t *> (core)->base);
				// Устанавливаем событие на чтение данных подключения
				this->bev.event.read.set <awh::worker_t::adj_t, &awh::worker_t::adj_t::read> (this);
				// Устанавливаем событие на запись данных подключения
				this->bev.event.write.set <awh::worker_t::adj_t, &awh::worker_t::adj_t::write> (this);
				// Запускаем чтение данных с сервера
				this->bev.event.read.start();
				// Запускаем запись данных на сервер
				this->bev.event.write.start();
				// Если флаг ожидания входящих сообщений, активирован
				if(wrk->wait){
					// Если время ожидания чтения данных установлено
					if(this->timeRead > 0){
						// Устанавливаем базу событий
						this->bev.timer.read.set(const_cast <client::core_t *> (core)->base);
						// Устанавливаем событие на чтение данных подключения
						this->bev.timer.read.set <awh::worker_t::adj_t, &awh::worker_t::adj_t::timeout> (this);
						// Запускаем ожидание чтения данных с сервера
						this->bev.timer.read.start(this->timeRead);
					}
					// Если время ожидания записи данных установлено
					if(this->timeWrite > 0){
						// Устанавливаем базу событий
						this->bev.timer.write.set(const_cast <client::core_t *> (core)->base);
						// Устанавливаем событие на запись данных подключения
						this->bev.timer.write.set <awh::worker_t::adj_t, &awh::worker_t::adj_t::timeout> (this);
						// Запускаем ожидание записи данных на сервер
						this->bev.timer.write.start(this->timeWrite);
					}
				}
				// Выводим в лог сообщение
				if(!core->noinfo) this->log->print("connect client to server [%s:%d]", log_t::flag_t::INFO, host.c_str(), url.port);
				// Если подключение производится через, прокси-сервер
				if(wrk->isProxy()){
					// Выполняем функцию обратного вызова для прокси-сервера
					if(wrk->connectProxyFn != nullptr) wrk->connectProxyFn(this->aid, wrk->wid, const_cast <awh::core_t *> (wrk->core), wrk->ctx);
				// Выполняем функцию обратного вызова
				} else if(wrk->connectFn != nullptr) wrk->connectFn(this->aid, wrk->wid, const_cast <awh::core_t *> (wrk->core), wrk->ctx);

				cout << " $$$$$$$$$$$$$$$$$$$$$$ SOCKET2= " << this->bev.socket << endl;

				// Выходим из функции
				return;
			}
		}
		// Выполняем отключение от сервера
		const_cast <client::core_t *> (core)->close(this->aid);
	}
}
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
				case (uint8_t) client::worker_t::mode_t::CONNECT:
					// Выполняем новое подключение
					this->core->connect(this->wid);
				break;
				// Если режим работы клиента - это переподключение
				case (uint8_t) client::worker_t::mode_t::RECONNECT: {
					// Получаем объект воркера
					client::worker_t * wrk = (client::worker_t *) const_cast <awh::worker_t *> (it->second);
					// Устанавливаем флаг ожидания статуса
					wrk->status.wait = client::worker_t::mode_t::DISCONNECT;
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
 * @param ctx передаваемый контекст
 */
void awh::client::Core::resolver(const string ip, void * ctx) noexcept {
	// Если передаваемый контекст передан
	if(ctx != nullptr){
		// Получаем объект воркера
		client::worker_t * wrk = reinterpret_cast <client::worker_t *> (ctx);
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
				case (uint8_t) client::worker_t::mode_t::CONNECT:
					// Выполняем новое подключение к серверу
					core->connect(wrk->wid);
				break;
				// Если режим работы клиента - это переподключение
				case (uint8_t) client::worker_t::mode_t::RECONNECT:
					// Выполняем ещё одну попытку переподключиться к серверу
					core->createTimeout(wrk->wid, client::worker_t::mode_t::CONNECT);
				break;
			}
			// Выходим из функции
			return;
		// Если IP адрес не получен но нужно поддерживать постоянное подключение
		} else if(wrk->alive) {
			// Если ожидание переподключения не остановлено ранее
			if(wrk->status.wait != client::worker_t::mode_t::DISCONNECT){
				// Получаем объект ядра подключения
				core_t * core = (core_t *) const_cast <awh::core_t *> (wrk->core);
				// Выполняем ещё одну попытку переподключиться к серверу
				core->createTimeout(wrk->wid, client::worker_t::mode_t::RECONNECT);
			}
			// Выходим из функции, чтобы попытаться подключиться ещё раз
			return;
		}
		// Выводим функцию обратного вызова
		if(wrk->disconnectFn != nullptr) wrk->disconnectFn(0, wrk->wid, const_cast <awh::core_t *> (wrk->core), wrk->ctx);
	}
}
/**
 * tuning Метод тюннинга буфера событий
 * @param aid идентификатор адъютанта
 */
void awh::client::Core::tuning(const size_t aid) noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Получаем объект адъютанта
		awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (it->second);
		// Получаем объект воркера
		client::worker_t * wrk = (client::worker_t *) const_cast <awh::worker_t *> (adj->parent);
		// Получаем объект ядра клиента
		const core_t * core = reinterpret_cast <const core_t *> (wrk->core);
		// Разрешаем чтение данных с сокета
		adj->bev.locked.read = false;
		// Разрешаем запись данных в сокет
		adj->bev.locked.write = false;
		// Устанавливаем время ожидания поступления данных
		adj->timeRead = wrk->timeRead;
		// Устанавливаем время ожидания записи данных
		adj->timeWrite = wrk->timeWrite;
		// Устанавливаем размер детектируемых байт на чтение
		adj->markRead = wrk->markRead;
		// Устанавливаем размер детектируемых байт на запись
		adj->markWrite = wrk->markWrite;
		// Устанавливаем базу событий
		adj->bev.event.write.set(const_cast <core_t *> (core)->base);
		// Устанавливаем сокет для записи
		adj->bev.event.write.set(adj->bev.socket, ev::WRITE);
		// Устанавливаем событие подключения
		adj->bev.event.write.set <awh::worker_t::adj_t, &awh::worker_t::adj_t::connect> (adj);
	}
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
			client::worker_t * wrk = (client::worker_t *) const_cast <awh::worker_t *> (it->second);
			// Если подключение ещё не выполнено и выполнение работ разрешено
			if((wrk->status.real == client::worker_t::mode_t::DISCONNECT) && (wrk->status.work == client::worker_t::work_t::ALLOW)){
				// Размер структуры подключения
				socklen_t size = 0;
				// Объект подключения
				struct sockaddr * sin = nullptr;
				// Запрещаем выполнение работы
				wrk->status.work = client::worker_t::work_t::DISALLOW;
				// Устанавливаем флаг ожидания статуса
				wrk->status.wait = client::worker_t::mode_t::DISCONNECT;
				// Устанавливаем статус подключения
				wrk->status.real = client::worker_t::mode_t::PRECONNECT;
				// Получаем объект ядра клиента
				const core_t * core = reinterpret_cast <const core_t *> (wrk->core);
				// Получаем URL параметры запроса
				const uri_t::url_t & url = (wrk->isProxy() ? wrk->proxy.url : wrk->url);
				// Если в воркере есть подключённые клиенты
				if(!wrk->adjutants.empty()){
					// Переходим по всему списку адъютанта
					for(auto it = wrk->adjutants.begin(); it != wrk->adjutants.end();){
						// Если блокировка адъютанта не установлена
						if(this->locking.count(it->first) < 1){
							// Выполняем очистку буфера событий
							this->clean(it->first);
							// Выполняем удаление контекста SSL
							this->ssl.clear(it->second->ssl);
							// Удаляем адъютанта из списка подключений
							this->adjutants.erase(it->first);
							// Удаляем адъютанта из списка
							it = wrk->adjutants.erase(it);
						// Если есть хотябы один заблокированный элемент, выходим
						} else {
							// Устанавливаем статус подключения
							wrk->status.real = client::worker_t::mode_t::DISCONNECT;
							// Выходим из функции
							return;
						}
					}
				}
				// Получаем сокет для подключения к серверу
				auto sockaddr = this->sockaddr(url.ip, url.port, this->net.family);
				// Если сокет создан удачно
				if(sockaddr.fd > -1){
					// Создаём бъект адъютанта
					unique_ptr <awh::worker_t::adj_t> adj(new awh::worker_t::adj_t(wrk, this->fmk, this->log));
					// Если статус подключения не изменился
					if(wrk->status.real == client::worker_t::mode_t::PRECONNECT){

						this->ssl.clear(adj->ssl);

						// Выполняем получение контекста сертификата
						adj->ssl = this->ssl.init(url);
						// Выполняем блокировку потока
						this->mtx.connect.lock();
						// Запоминаем файловый дескриптор
						adj->bev.socket = sockaddr.fd;

						cout << " !!!!!!!!!!!!!!!!!! SOCKET= " << adj->bev.socket << " === " << std::this_thread::get_id() << endl;

						// Если SSL клиент разрешён
						if(adj->ssl.mode){

							cout << " -----------------SSL MODE " << endl;

							BIO * bio = SSL_get_wbio(adj->ssl.ssl);

							int have_fd = -1;
							if(bio) have_fd = BIO_get_fd(bio, nullptr);

							if(have_fd >= 0){
								if(adj->bev.socket < 0){
									adj->bev.socket = (int) have_fd;
								} else if (have_fd == (long) adj->bev.socket) {
									/* We already know the fd from the SSL; do nothing */
								} else {
									/* We specified an fd different from that of the SSL.
									This is probably an error on our part.  Fail. */
									// goto err;
								}
								BIO_set_close(bio, 0);
							} else {
								/* The SSL isn't configured with a BIO with an fd. */
								if (adj->bev.socket >= 0) {
									/* ... and we have an fd we want to use. */
									bio = BIO_new_socket(adj->bev.socket, 0);
									BIO_set_nbio(bio, 1);
									SSL_set_bio(adj->ssl.ssl, bio, bio);
								} else {
									/* Leave the fd unset. */
								}
							}
							SSL_set_mode(adj->ssl.ssl, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

							SSL_set_connect_state(adj->ssl.ssl);
						}

						/*
						// Если SSL клиент разрешён
						if(adj->ssl.mode){
							// Создаем буфер событий для сервера зашифрованного подключения
							adj->bev = bufferevent_openssl_socket_new(this->base, sockaddr.fd, adj->ssl.ssl, BUFFEREVENT_SSL_CONNECTING, mode);
							// Разрешаем непредвиденное грязное завершение работы
							bufferevent_openssl_set_allow_dirty_shutdown(adj->bev, 1);
						// Создаем буфер событий для сервера
						} else adj->bev = bufferevent_socket_new(this->base, sockaddr.fd, mode);
						*/

						// Устанавливаем идентификатор адъютанта
						adj->aid = this->fmk->unixTimestamp();

						cout << " @@@@@@@@@@@@@@@@@@@@@@ CONNECT1 " << adj->aid << endl;

						// Добавляем созданного адъютанта в список адъютантов
						auto ret = wrk->adjutants.emplace(adj->aid, move(adj));
						// Добавляем адъютанта в список подключений
						this->adjutants.emplace(ret.first->first, ret.first->second.get());
						// Выполняем тюннинг буфера событий
						this->tuning(ret.first->first);
						// Выполняем блокировку потока
						this->mtx.connect.unlock();
						// Определяем тип подключения
						switch(this->net.family){
							// Для протокола IPv4
							case AF_INET: {
								// Запоминаем размер структуры
								size = sizeof(sockaddr.server);
								// Запоминаем полученную структуру
								sin = reinterpret_cast <struct sockaddr *> (&sockaddr.server);
							} break;
							// Для протокола IPv6
							case AF_INET6: {
								// Запоминаем размер структуры
								size = sizeof(sockaddr.server6);
								// Запоминаем полученную структуру
								sin = reinterpret_cast <struct sockaddr *> (&sockaddr.server6);
							} break;
						}
						// Выполняем подключение к удаленному серверу, если подключение не выполненно то сообщаем об этом
						if(::connect(ret.first->second->bev.socket, sin, sizeof(struct sockaddr_in)) != 0){
							// Разрешаем выполнение работы
							wrk->status.work = client::worker_t::work_t::ALLOW;
							// Устанавливаем статус подключения
							wrk->status.real = client::worker_t::mode_t::DISCONNECT;
							// Запрещаем чтение данных с сервера
							ret.first->second->bev.locked.read = true;
							// Запрещаем запись данных на сервер
							ret.first->second->bev.locked.write = true;
							// Выводим в лог сообщение
							this->log->print("connecting to host = %s, port = %u", log_t::flag_t::CRITICAL, url.ip.c_str(), url.port);
							/*
							// Определяем тип подключения
							switch(this->net.family){
								// Для резолвера IPv4
								case AF_INET: {
									// Выполняем сброс кэша резолвера
									this->dns4.flush();
									// Добавляем бракованный IPv4 адрес в список адресов
									this->dns4.setToBlackList(url.ip); 
								} break;
								// Для резолвера IPv6
								case AF_INET6: {
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
						// Разрешаем выполнение работы
						wrk->status.work = client::worker_t::work_t::ALLOW;

						cout << " @@@@@@@@@@@@@@@@@@@@@@ CONNECT2 " << ret.first->second->bev.locked.write << endl;

						// Если статус подключения изменился
						if(wrk->status.real != client::worker_t::mode_t::PRECONNECT){
							// Запрещаем чтение данных с сервера
							ret.first->second->bev.locked.read = true;
							// Запрещаем запись данных на сервер
							ret.first->second->bev.locked.write = true;
						// Если статус подключения не изменился
						} else {

							cout << " @@@@@@@@@@@@@@@@@@@@@@ CONNECT3 " << ret.first->second->bev.locked.write << endl;

							// Выполняем запуск подключения
							ret.first->second->bev.event.write.start();
							// Если время ожидания записи данных установлено
							if(ret.first->second->timeWrite > 0){
								// Устанавливаем базу событий
								ret.first->second->bev.timer.write.set(this->base);
								// Устанавливаем событие на запись данных подключения
								ret.first->second->bev.timer.write.set <awh::worker_t::adj_t, &awh::worker_t::adj_t::timeout> (ret.first->second.get());
								// Запускаем запись данных на сервер
								ret.first->second->bev.timer.write.start(ret.first->second->timeWrite);
							}
							// Выводим в лог сообщение
							if(!core->noinfo) this->log->print("good host = %s [%s:%d], socket = %d", log_t::flag_t::INFO, url.domain.c_str(), url.ip.c_str(), url.port, sockaddr.fd);
						}
						// Выходим из функции
						return;
					}
				}
				// Если нужно выполнить автоматическое переподключение
				if(wrk->alive){
					// Разрешаем выполнение работы
					wrk->status.work = client::worker_t::work_t::ALLOW;
					// Устанавливаем статус подключения
					wrk->status.real = client::worker_t::mode_t::DISCONNECT;
					// Устанавливаем флаг ожидания статуса
					wrk->status.wait = client::worker_t::mode_t::DISCONNECT;
					// Выполняем переподключение
					this->reconnect(wid);
					// Выходим из функции
					return;
				// Если все попытки исчерпаны
				} else {
					// Разрешаем выполнение работы
					wrk->status.work = client::worker_t::work_t::ALLOW;
					// Устанавливаем статус подключения
					wrk->status.real = client::worker_t::mode_t::DISCONNECT;
					/*
					// Определяем тип подключения
					switch(this->net.family){
						// Для резолвера IPv4
						case AF_INET: {
							// Выполняем сброс кэша резолвера
							this->dns4.flush();
							// Добавляем бракованный IPv4 адрес в список адресов
							this->dns4.setToBlackList(url.ip); 
						} break;
						// Для резолвера IPv6
						case AF_INET6: {
							// Выполняем сброс кэша резолвера
							this->dns6.flush();
							// Добавляем бракованный IPv6 адрес в список адресов
							this->dns6.setToBlackList(url.ip);
						} break;
					}
					*/
					// Выводим сообщение об ошибке
					if(!core->noinfo) this->log->print("%s", log_t::flag_t::INFO, "disconnected from the server");
					// Выводим функцию обратного вызова
					if(wrk->disconnectFn != nullptr) wrk->disconnectFn(0, wrk->wid, this, wrk->ctx);
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
		client::worker_t * wrk = (client::worker_t *) const_cast <awh::worker_t *> (it->second);
		// Если параметры URL запроса переданы и выполнение работы разрешено
		if(!wrk->url.empty() && (wrk->status.wait == client::worker_t::mode_t::DISCONNECT) && (wrk->status.work == client::worker_t::work_t::ALLOW)){
			// Устанавливаем флаг ожидания статуса
			wrk->status.wait = client::worker_t::mode_t::RECONNECT;
			// Получаем URL параметры запроса
			const uri_t::url_t & url = (wrk->isProxy() ? wrk->proxy.url : wrk->url);


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
			// Выполняем запуск системы
			resolver(url.ip, wrk);

			/*
			// Определяем тип подключения
			switch(this->net.family){
				// Резолвер IPv4, создаём резолвер
				case AF_INET:
					// Выполняем резолвинг домена
					wrk->did = this->dns4.resolve(wrk, (!url.domain.empty() ? url.domain : url.ip), AF_INET, &resolver);
				break;
				// Резолвер IPv6, создаём резолвер
				case AF_INET6:
					// Выполняем резолвинг домена
					wrk->did = this->dns6.resolve(wrk, (!url.domain.empty() ? url.domain : url.ip), AF_INET6, &resolver);
				break;
			}
			*/
		}
	}
}
/**
 * createTimeout Метод создания таймаута
 * @param wid  идентификатор воркера
 * @param mode режим работы клиента
 */
void awh::client::Core::createTimeout(const size_t wid, const client::worker_t::mode_t mode) noexcept {
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
		timeout->timer.set(this->base);
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
				client::worker_t * wrk = (client::worker_t *) const_cast <awh::worker_t *> (worker.second);
				// Если выполнение работ разрешено
				if(wrk->status.work == client::worker_t::work_t::ALLOW)
					// Запрещаем выполнение работы
					wrk->status.work = client::worker_t::work_t::DISALLOW;
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
				client::worker_t * wrk = (client::worker_t *) const_cast <awh::worker_t *> (worker.second);
				// Если флаг поддержания постоянного подключения не установлен
				if(!alive && wrk->alive) alive = wrk->alive;
				// Устанавливаем статус подключения
				wrk->status.real = client::worker_t::mode_t::DISCONNECT;
				// Устанавливаем флаг ожидания статуса
				wrk->status.wait = client::worker_t::mode_t::DISCONNECT;
			}
			// Если необходимо поддерживать постоянное подключение
			if(alive){
				/*
				// Определяем тип подключения
				switch(this->net.family){
					// Резолвер IPv4, создаём резолвер
					case AF_INET: {
						// Добавляем базу событий для DNS резолвера IPv4
						this->dns4.setBase(this->base);
						// Выполняем установку нейм-серверов для DNS резолвера IPv4
						this->dns4.replaceServers(this->net.v4.second);
					} break;
					// Резолвер IPv6, создаём резолвер
					case AF_INET6: {
						// Добавляем базу событий для DNS резолвера IPv6
						this->dns4.setBase(this->base);
						// Выполняем установку нейм-серверов для DNS резолвера IPv6
						this->dns4.replaceServers(this->net.v4.second);
					} break;
				}
				*/
			}
			// Переходим по всему списку воркеров
			for(auto & worker : this->workers){
				// Получаем объект воркера
				client::worker_t * wrk = (client::worker_t *) const_cast <awh::worker_t *> (worker.second);
				// Разрешаем воркеру выполнять перезапуск
				wrk->stop = false;
				// Если выполнение работ запрещено
				if(wrk->status.work == client::worker_t::work_t::DISALLOW)
					// Разрешаем выполнение работы
					wrk->status.work = client::worker_t::work_t::ALLOW;
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
				client::worker_t * wrk = (client::worker_t *) const_cast <awh::worker_t *> (worker.second);
				// Устанавливаем флаг ожидания статуса
				wrk->status.wait = client::worker_t::mode_t::DISCONNECT;
				// Устанавливаем статус сетевого ядра
				wrk->status.real = client::worker_t::mode_t::DISCONNECT;
				// Переходим по всему списку адъютанта
				for(auto it = wrk->adjutants.begin(); it != wrk->adjutants.end();){
					// Если блокировка адъютанта не установлена
					if(this->locking.count(it->first) < 1){
						// Выполняем блокировку адъютанта
						this->locking.emplace(it->first);
						// Выполняем очистку буфера событий
						this->clean(it->first);
						// Выполняем удаление контекста SSL
						this->ssl.clear(it->second->ssl);
						// Удаляем адъютанта из списка подключений
						this->adjutants.erase(it->first);
						// Выводим функцию обратного вызова
						if(wrk->disconnectFn != nullptr)
							// Выполняем функцию обратного вызова
							wrk->disconnectFn(it->first, worker.first, this, wrk->ctx);
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
			client::worker_t * wrk = (client::worker_t *) const_cast <awh::worker_t *> (it->second);
			// Устанавливаем флаг ожидания статуса
			wrk->status.wait = client::worker_t::mode_t::DISCONNECT;
			// Устанавливаем статус сетевого ядра
			wrk->status.real = client::worker_t::mode_t::DISCONNECT;
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
						// Выполняем удаление контекста SSL
						this->ssl.clear(adj->ssl);
						// Удаляем адъютанта из списка подключений
						this->adjutants.erase(jt->first);
						// Выводим функцию обратного вызова
						if(wrk->disconnectFn != nullptr)
							// Выполняем функцию обратного вызова
							wrk->disconnectFn(jt->first, it->first, this, wrk->ctx);
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
			client::worker_t * wrk = (client::worker_t *) const_cast <awh::worker_t *> (it->second);
			// Если параметры URL запроса переданы и выполнение работы разрешено
			if(!wrk->url.empty() && (wrk->status.wait == client::worker_t::mode_t::DISCONNECT) && (wrk->status.work == client::worker_t::work_t::ALLOW)){
				// Устанавливаем флаг ожидания статуса
				wrk->status.wait = client::worker_t::mode_t::CONNECT;
				// Получаем URL параметры запроса
				const uri_t::url_t & url = (wrk->isProxy() ? wrk->proxy.url : wrk->url);



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
				// Выполняем запуск системы
				resolver(url.ip, wrk);

				/*
				// Если IP адрес не получен
				if(url.ip.empty() && !url.domain.empty())
					// Определяем тип подключения
					switch(this->net.family){
						// Резолвер IPv4, создаём резолвер
						case AF_INET: wrk->did = this->dns4.resolve(wrk, url.domain, AF_INET, &resolver); break;
						// Резолвер IPv6, создаём резолвер
						case AF_INET6: wrk->did = this->dns6.resolve(wrk, url.domain, AF_INET6, &resolver); break;
					}
				// Выполняем запуск системы
				else if(!url.ip.empty()) resolver(url.ip, wrk);
				*/
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

	cout << " +++++++++++++++++++CLOSE1 " << aid << endl;

	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.close);

	cout << " +++++++++++++++++++CLOSE2 " << aid << endl;

	// Если блокировка адъютанта не установлена
	if(this->locking.count(aid) < 1){
		// Выполняем блокировку адъютанта
		this->locking.emplace(aid);
		// Выполняем извлечение адъютанта
		auto it = this->adjutants.find(aid);

		cout << " +++++++++++++++++++CLOSE3 " << aid << endl;

		// Если адъютант получен
		if(it != this->adjutants.end()){
			// Получаем объект адъютанта
			awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (it->second);
			// Получаем объект воркера
			client::worker_t * wrk = (client::worker_t *) const_cast <awh::worker_t *> (adj->parent);
			// Получаем объект ядра клиента
			const core_t * core = reinterpret_cast <const core_t *> (wrk->core);

			cout << " +++++++++++++++++++CLOSE4 " << endl;

			// Выполняем очистку буфера событий
			this->clean(aid);
			// Удаляем установленный таймаут, если он существует
			this->clearTimeout(wrk->wid);
			// Если прокси-сервер активирован но уже переключён на работу с сервером
			if((wrk->proxy.type != proxy_t::type_t::NONE) && !wrk->isProxy())
				// Выполняем переключение обратно на прокси-сервер
				wrk->switchConnect();
			// Выполняем удаление контекста SSL
			this->ssl.clear(adj->ssl);


			cout << " +++++++++++++++++++CLOSE5 " << endl;
			
			// Удаляем адъютанта из списка адъютантов
			wrk->adjutants.erase(aid);
			// Удаляем адъютанта из списка подключений
			this->adjutants.erase(aid);

			cout << " +++++++++++++++++++CLOSE6 " << endl;
			
			// Устанавливаем флаг ожидания статуса
			wrk->status.wait = client::worker_t::mode_t::DISCONNECT;
			// Устанавливаем статус сетевого ядра
			wrk->status.real = client::worker_t::mode_t::DISCONNECT;
			// Если не нужно выполнять принудительную остановку работы воркера
			if(!wrk->stop){
				// Если нужно выполнить автоматическое переподключение
				if(wrk->alive) this->reconnect(wrk->wid);
				// Если автоматическое подключение выполнять не нужно
				else {
					// Выводим сообщение об ошибке
					if(!core->noinfo) this->log->print("%s", log_t::flag_t::INFO, "disconnected from the server");

					cout << " +++++++++++++++++++CLOSE7 " << endl;

					// Выводим функцию обратного вызова
					if(wrk->disconnectFn != nullptr) wrk->disconnectFn(aid, wrk->wid, this, wrk->ctx);
				}
			}

			cout << " +++++++++++++++++++CLOSE8 " << endl;
		}
		// Удаляем блокировку адъютанта
		this->locking.erase(aid);
	}

	cout << " +++++++++++++++++++CLOSE8 " << endl;
}
/**
 * switchProxy Метод переключения с прокси-сервера
 * @param aid идентификатор адъютанта
 */
void awh::client::Core::switchProxy(const size_t aid) noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Получаем объект адъютанта
		awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (it->second);
		// Получаем объект воркера
		client::worker_t * wrk = (client::worker_t *) const_cast <awh::worker_t *> (adj->parent);
		// Если прокси-сервер активирован но ещё не переключён на работу с сервером
		if((wrk->proxy.type != proxy_t::type_t::NONE) && (adj->bev.socket >= 0) && wrk->isProxy()){
			// Выполняем переключение на работу с сервером
			wrk->switchConnect();

			this->ssl.clear(adj->ssl);

			// Выполняем получение контекста сертификата
			adj->ssl = this->ssl.init(wrk->url);
			// Если SSL клиент разрешен
			if(adj->ssl.mode){
				// Выполняем блокировку потока
				this->mtx.proxy.lock();
				

				BIO * bio = BIO_new_socket(adj->bev.socket, 0);
				BIO_set_nbio(bio, 1);
				SSL_set_bio(adj->ssl.ssl, bio, bio);

				SSL_set_mode(adj->ssl.ssl, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

				SSL_set_connect_state(adj->ssl.ssl);

				// Выполняем тюннинг буфера событий
				this->tuning(aid);


				/*
				// Устанавливаем первоначальное значение
				u_int mode = 0;
				// Если нужно использовать отложенные вызовы событий сокета
				if(this->defer) mode = (mode | BEV_OPT_DEFER_CALLBACKS);
				// Выполняем переход на защищённое подключение
				struct bufferevent * bev = bufferevent_openssl_filter_new(this->base, adj->bev, adj->ssl.ssl, BUFFEREVENT_SSL_CONNECTING, mode);
				// Если буфер событий создан
				if(bev != nullptr){
					// Устанавливаем новый буфер событий
					adj->bev = bev;
					// Разрешаем непредвиденное грязное завершение работы
					bufferevent_openssl_set_allow_dirty_shutdown(adj->bev, 1);
					// Выполняем тюннинг буфера событий
					this->tuning(aid);
				// Отключаемся от сервера
				} else this->close(aid);
				*/
				// Выполняем разблокировку потока
				this->mtx.proxy.unlock();
				// Выходим из функции
				return;
			}
		}
		// Если функция обратного вызова установлена, сообщаем, что мы подключились
		if(wrk->connectFn != nullptr) wrk->connectFn(aid, wrk->wid, this, wrk->ctx);
	}
}
/**
 * timeout Функция обратного вызова при срабатывании таймаута
 * @param aid идентификатор адъютанта
 */
void awh::client::Core::timeout(const size_t aid) noexcept {
	
	cout << " !!!!!!!!!!!! TIMEOUT " << aid << endl;
	
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Получаем объект адъютанта
		awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (it->second);
		// Получаем объект подключения
		client::worker_t * wrk = (client::worker_t *) const_cast <awh::worker_t *> (adj->parent);
		// Получаем URL параметры запроса
		const uri_t::url_t & url = (wrk->isProxy() ? wrk->proxy.url : wrk->url);
		// Если данные ещё ни разу не получены
		if(!wrk->acquisition && !url.ip.empty()){
			/*
			// Определяем тип подключения
			switch(core->net.family){
				// Резолвер IPv4, добавляем бракованный IPv4 адрес в список адресов
				case AF_INET: this->dns4.setToBlackList(url.ip); break;
				// Резолвер IPv6, добавляем бракованный IPv6 адрес в список адресов
				case AF_INET6: this->dns6.setToBlackList(url.ip); break;
			}
			*/
		}
		// Выводим сообщение в лог, о таймауте подключения
		this->log->print("timeout host %s [%s%d]", log_t::flag_t::WARNING, url.domain.c_str(), (!url.ip.empty() ? (url.ip + ":").c_str() : ""), url.port);
		// Останавливаем чтение данных
		adj->bev.event.read.stop();
		// Останавливаем запись данных
		adj->bev.event.write.stop();
		// Выполняем отключение от сервера
		this->close(aid);
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
		client::worker_t * wrk = (client::worker_t *) const_cast <awh::worker_t *> (adj->parent);
		// Если подключение установлено
		if((wrk->acquisition = (wrk->status.real == client::worker_t::mode_t::CONNECT))){
			// Определяем метод работы
			switch((uint8_t) method){
				// Если производится чтение данных
				case (uint8_t) method_t::READ: {
					// Количество полученных байт
					int64_t bytes = -1;
					// Создаём буфер входящих данных
					char buffer[BUFFER_SIZE];
					// Останавливаем таймаут ожидания на чтение из сокета
					adj->bev.timer.read.stop();
					// Выполняем чтение всех данных из сокета
					while(wrk->status.real == client::worker_t::mode_t::CONNECT){
						// Выполняем зануление буфера
						memset(buffer, 0, sizeof(buffer));

						cout << " ^^^^^^^^^^^^^^^^^ READ1 " << endl;

						// Если SSL клиент разрешён
						if(adj->ssl.mode)
							// Выполняем чтение из защищённого сокета
							bytes = SSL_read(adj->ssl.ssl, buffer, sizeof(buffer));
						// Выполняем чтение данных из сокета
						else bytes = recv(adj->bev.socket, buffer, sizeof(buffer), 0);
						// Останавливаем таймаут ожидания на чтение из сокета
						adj->bev.timer.read.stop();

						cout << " ^^^^^^^^^^^^^^^^^ READ2 " << endl;

						// Если данные получены
						if(bytes > 0){
							// Если время ожидания записи данных установлено
							if(adj->timeRead > 0)
								// Запускаем ожидание чтения данных с сервера
								adj->bev.timer.read.start(adj->timeRead);
							// Если данные считанные из буфера, больше размера ожидающего буфера
							if((adj->markWrite.max > 0) && (bytes >= adj->markWrite.max)){
								// Смещение в буфере и отправляемый размер данных
								size_t offset = 0, actual = 0;
								// Выполняем пересылку всех полученных данных
								while((bytes - offset) > 0){
									// Определяем размер отправляемых данных
									actual = ((bytes - offset) >= adj->markWrite.max ? adj->markWrite.max : (bytes - offset));
									// Если подключение производится через, прокси-сервер
									if(wrk->isProxy()){
										// Если функция обратного вызова для вывода записи существует
										if(wrk->readProxyFn != nullptr)
											// Выводим функцию обратного вызова
											wrk->readProxyFn(buffer + offset, actual, aid, wrk->wid, reinterpret_cast <awh::core_t *> (this), wrk->ctx);
									// Если прокси-сервер не используется
									} else if(wrk->readFn != nullptr)
										// Выводим функцию обратного вызова
										wrk->readFn(buffer + offset, actual, aid, wrk->wid, reinterpret_cast <awh::core_t *> (this), wrk->ctx);
									// Увеличиваем смещение в буфере
									offset += actual;
								}
							// Если данных достаточно
							} else {
								// Если подключение производится через, прокси-сервер
								if(wrk->isProxy()){
									// Если функция обратного вызова для вывода записи существует
									if(wrk->readProxyFn != nullptr)
										// Выводим функцию обратного вызова
										wrk->readProxyFn(buffer, bytes, aid, wrk->wid, reinterpret_cast <awh::core_t *> (this), wrk->ctx);
								// Если прокси-сервер не используется
								} else if(wrk->readFn != nullptr)
									// Выводим функцию обратного вызова
									wrk->readFn(buffer, bytes, aid, wrk->wid, reinterpret_cast <awh::core_t *> (this), wrk->ctx);
							}
						// Если данные не могут быть прочитаны
						} else {

							cout << " +++++++++++++++++++READ2.1 " << endl;

							// Выполняем обработку ошибок
							this->error(bytes, aid);
							// Выполняем отключение от сервера
							this->close(aid);
							// Выходим из цикла
							break;
						}
					}

					cout << " +++++++++++++++++++READ2.2 " << endl;
				} break;
				// Если производится запись данных
				case (uint8_t) method_t::WRITE: {

					cout << " +++++++++++++++++++WRITE " << adj->timeWrite << endl;

					// Останавливаем таймаут ожидания на запись в сокет
					adj->bev.timer.write.stop();
					// Если время ожидания записи данных установлено
					if(adj->timeWrite > 0)
						// Запускаем ожидание записи данных на сервер
						adj->bev.timer.write.start(adj->timeWrite);
				} break;
			}
		// Если подключение завершено
		} else {
			// Останавливаем таймаут ожидания на чтение из сокета
			adj->bev.timer.read.stop();
			// Останавливаем таймаут ожидания на запись в сокет
			adj->bev.timer.write.stop();
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
		// Если - это Unix
		#if !defined(_WIN32) && !defined(_WIN64)
			// Получаем объект адъютанта
			awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (it->second);
			// Получаем размер буфера на чтение
			const int rcv = (!read.empty() ? this->fmk->sizeBuffer(read) : 0);
			// Получаем размер буфера на запись
			const int snd = (!write.empty() ? this->fmk->sizeBuffer(write) : 0);
			// Устанавливаем размер буфера
			if(adj->bev.socket > 0) this->socket.bufferSize(adj->bev.socket, rcv, snd, 1);
		// Если - это Windows
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
	this->type = type_t::CLIENT;
}
/**
 * ~Core Деструктор
 */
awh::client::Core::~Core() noexcept {
	// Выполняем остановку клиента
	this->stop();
}
