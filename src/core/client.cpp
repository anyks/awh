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
				// Если требуется использовать unix-сокет
				if(this->isSetUnixSocket()){
					/**
					 * Если операционной системой не является Windows
					 */
					#if !defined(_WIN32) && !defined(_WIN64)
						// Если в воркере есть подключённые клиенты
						if(!wrk->adjutants.empty()){
							// Переходим по всему списку адъютанта
							for(auto it = wrk->adjutants.begin(); it != wrk->adjutants.end();){
								// Если блокировка адъютанта не установлена
								if(this->locking.count(it->first) < 1){
									// Выполняем очистку буфера событий
									this->clean(it->first);
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
						// Получаем сокет для подключения к серверу
						auto sockaddr = this->sockaddr();
						// Если сокет создан удачно
						if(sockaddr.socket > -1){
							// Создаём бъект адъютанта
							unique_ptr <awh::worker_t::adj_t> adj(new awh::worker_t::adj_t(wrk, this->fmk, this->log));
							// Если статус подключения не изменился
							if(wrk->status.real == worker_t::mode_t::PRECONNECT){
								// Выполняем блокировку потока
								this->mtx.connect.lock();
								// Запоминаем файловый дескриптор
								adj->bev.socket = sockaddr.socket;
								// Устанавливаем идентификатор адъютанта
								adj->aid = this->fmk->unixTimestamp();
								// Добавляем созданного адъютанта в список адъютантов
								auto ret = wrk->adjutants.emplace(adj->aid, move(adj));
								// Добавляем адъютанта в список подключений
								this->adjutants.emplace(ret.first->first, ret.first->second.get());
								// Выполняем блокировку потока
								this->mtx.connect.unlock();
								// Запоминаем полученную структуру
								struct sockaddr * sun = reinterpret_cast <struct sockaddr *> (&sockaddr.unix);
								// Получаем размер объекта сокета
								const socklen_t size = (offsetof(struct sockaddr_un, sun_path) + strlen(sockaddr.unix.sun_path));
								// Если подключение не выполненно то сообщаем об этом, выполняем подключение к удаленному серверу
								if(::connect(ret.first->second->bev.socket, sun, size) != 0){									
									// Запрещаем чтение данных с сервера
									ret.first->second->bev.locked.read = true;
									// Запрещаем запись данных на сервер
									ret.first->second->bev.locked.write = true;
									// Разрешаем выполнение работы
									wrk->status.work = worker_t::work_t::ALLOW;
									// Устанавливаем статус подключения
									wrk->status.real = worker_t::mode_t::DISCONNECT;
									// Выводим в лог сообщение
									this->log->print("connecting to host = %s", log_t::flag_t::CRITICAL, this->unixSocket.c_str());
									// Выполняем отключение от сервера
									this->close(ret.first->first);
									// Выходим из функции
									return;
								}
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
									// Выводим в лог сообщение
									if(!this->noinfo) this->log->print("good host = %s, socket = %d", log_t::flag_t::INFO, this->unixSocket.c_str(), sockaddr.socket);
								}
								// Выходим из функции
								return;
							}
						}
					#endif
				// Если unix-сокет не используется
				} else {
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
								wrk->status.real = worker_t::mode_t::DISCONNECT;
								// Выходим из функции
								return;
							}
						}
					}
					// Получаем сокет для подключения к серверу
					auto sockaddr = this->sockaddr(url.ip, url.port, this->net.family);
					// Если сокет создан удачно
					if(sockaddr.socket > -1){
						// Создаём бъект адъютанта
						unique_ptr <awh::worker_t::adj_t> adj(new awh::worker_t::adj_t(wrk, this->fmk, this->log));
						// Если статус подключения не изменился
						if(wrk->status.real == worker_t::mode_t::PRECONNECT){
							// Размер структуры подключения
							socklen_t size = 0;
							// Объект подключения
							struct sockaddr * sin = nullptr;
							// Выполняем удаление контекста SSL
							this->ssl.clear(adj->ssl);
							// Выполняем получение контекста сертификата
							adj->ssl = this->ssl.init(url);
							// Выполняем блокировку потока
							this->mtx.connect.lock();
							// Запоминаем файловый дескриптор
							adj->bev.socket = sockaddr.socket;
							// Если защищённый режим работы разрешён
							if(adj->ssl.mode){
								// Выполняем обёртывание сокета в BIO SSL
								BIO * bio = BIO_new_socket(adj->bev.socket, BIO_NOCLOSE);
								// Если BIO SSL создано
								if(bio != nullptr){
									// Устанавливаем блокирующий режим ввода/вывода для сокета
									BIO_set_nbio(bio, 0);
									// Выполняем установку BIO SSL
									SSL_set_bio(adj->ssl.ssl, bio, bio);
									// Выполняем активацию клиента SSL
									SSL_set_connect_state(adj->ssl.ssl);
								// Если BIO SSL не создано
								} else {
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
									this->log->print("BIO new socket is failed", log_t::flag_t::CRITICAL);
									// Выполняем переподключение
									this->reconnect(wid);
									// Выходим из функции
									return;
								}
							}
							// Устанавливаем идентификатор адъютанта
							adj->aid = this->fmk->unixTimestamp();
							// Добавляем созданного адъютанта в список адъютантов
							auto ret = wrk->adjutants.emplace(adj->aid, move(adj));
							// Добавляем адъютанта в список подключений
							this->adjutants.emplace(ret.first->first, ret.first->second.get());
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
							// Если подключение не выполненно то сообщаем об этом, выполняем подключение к удаленному серверу
							if(::connect(ret.first->second->bev.socket, sin, size) != 0){
								// Запрещаем чтение данных с сервера
								ret.first->second->bev.locked.read = true;
								// Запрещаем запись данных на сервер
								ret.first->second->bev.locked.write = true;
								// Разрешаем выполнение работы
								wrk->status.work = worker_t::work_t::ALLOW;
								// Устанавливаем статус подключения
								wrk->status.real = worker_t::mode_t::DISCONNECT;
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
								// Выводим в лог сообщение
								if(!this->noinfo) this->log->print("good host = %s [%s:%d], socket = %d", log_t::flag_t::INFO, url.domain.c_str(), url.ip.c_str(), url.port, sockaddr.socket);
							}
							// Выходим из функции
							return;
						}
					}
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
					// Если unix-сокет не используется
					if(!this->isSetUnixSocket()){
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
					}
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
			// Если требуется использовать unix-сокет
			if(this->isSetUnixSocket())
				// Выполняем подключение заново
				this->connect(wrk->wid);
			// Если unix-сокет использовать не требуется
			else {
				// Устанавливаем флаг ожидания статуса
				wrk->status.wait = worker_t::mode_t::RECONNECT;
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
			if(alive && !this->isSetUnixSocket()){
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
				// Переходим по всему списку адъютанта
				for(auto it = wrk->adjutants.begin(); it != wrk->adjutants.end();){
					// Если блокировка адъютанта не установлена
					if(this->locking.count(it->first) < 1){
						// Выполняем блокировку адъютанта
						this->locking.emplace(it->first);
						// Выполняем очистку буфера событий
						this->clean(it->first);
						// Если unix-сокет не используется
						if(!this->isSetUnixSocket())
							// Выполняем удаление контекста SSL
							this->ssl.clear(it->second->ssl);
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
						// Если unix-сокет не используется
						if(!this->isSetUnixSocket())
							// Выполняем удаление контекста SSL
							this->ssl.clear(adj->ssl);
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
				// Если требуется использовать unix-сокет
				if(this->isSetUnixSocket())
					// Выполняем подключение заново
					this->connect(wrk->wid);
				// Если unix-сокет использовать не требуется
				else {
					// Устанавливаем флаг ожидания статуса
					wrk->status.wait = worker_t::mode_t::CONNECT;
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
			// Если unix-сокет не используется
			if(!this->isSetUnixSocket())
				// Выполняем удаление контекста SSL
				this->ssl.clear(adj->ssl);
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
	// Если unix-сокет не используется
	if(!this->isSetUnixSocket()){
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
			if((wrk->proxy.type != proxy_t::type_t::NONE) && (adj->bev.socket >= 0) && wrk->isProxy()){
				// Выполняем переключение на работу с сервером
				wrk->switchConnect();
				// Выполняем удаление контекста SSL
				this->ssl.clear(adj->ssl);
				// Выполняем получение контекста сертификата
				adj->ssl = this->ssl.init(wrk->url);
				// Если защищённый режим работы разрешён
				if(adj->ssl.mode){
					// Выполняем обёртывание сокета в BIO SSL
					BIO * bio = BIO_new_socket(adj->bev.socket, BIO_NOCLOSE);
					// Если BIO SSL создано
					if(bio != nullptr){
						// Устанавливаем блокирующий режим ввода/вывода для сокета
						BIO_set_nbio(bio, 0);
						// Выполняем установку BIO SSL
						SSL_set_bio(adj->ssl.ssl, bio, bio);
						// Выполняем активацию клиента SSL
						SSL_set_connect_state(adj->ssl.ssl);
						// Останавливаем чтение данных
						this->disabled(method_t::READ, it->first);
						// Останавливаем запись данных
						this->disabled(method_t::WRITE, it->first);
						// Активируем ожидание подключения
						this->enabled(method_t::CONNECT, it->first);
					// Выводим сообщение об ошибке
					} else this->log->print("BIO new socket is failed", log_t::flag_t::CRITICAL);
					// Выходим из функции
					return;
				}
			}
			// Если функция обратного вызова установлена, сообщаем, что мы подключились
			if(wrk->connectFn != nullptr) wrk->connectFn(it->first, wrk->wid, this);
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
		// Если unix-сокет не используется
		if(!this->isSetUnixSocket()){
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
		// Если используется unix-сокет, вводим сообщение в лог, о таймауте подключения
		} else this->log->print("timeout host %s", log_t::flag_t::WARNING, this->unixSocket.c_str());
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
			// Если unix-сокет не используется
			if(!this->isSetUnixSocket()){
				// Получаем URL параметры запроса
				const uri_t::url_t & url = (wrk->isProxy() ? wrk->proxy.url : wrk->url);
				// Получаем хост сервера
				const string & host = (!url.ip.empty() ? url.ip : url.domain);

				/*
				// Определяем тип подключения
				switch(this->net.family){
					// Резолвер IPv4, создаём резолвер
					case AF_INET:
						// Выполняем отмену ранее выполненных запросов DNS
						this->dns4.cancel(wrk->did);
					break;
					// Резолвер IPv6, создаём резолвер
					case AF_INET6:
						// Выполняем отмену ранее выполненных запросов DNS
						this->dns6.cancel(wrk->did);
					break;
				}
				*/

				// Запускаем чтение данных
				this->enabled(method_t::READ, it->first, wrk->wait);
				// Выводим в лог сообщение
				if(!this->noinfo) this->log->print("connect client to server [%s:%d]", log_t::flag_t::INFO, host.c_str(), url.port);
				// Если подключение производится через, прокси-сервер
				if(wrk->isProxy()){
					// Выполняем функцию обратного вызова для прокси-сервера
					if(wrk->connectProxyFn != nullptr) wrk->connectProxyFn(it->first, wrk->wid, const_cast <awh::core_t *> (wrk->core));
				// Выполняем функцию обратного вызова
				} else if(wrk->connectFn != nullptr) wrk->connectFn(it->first, wrk->wid, const_cast <awh::core_t *> (wrk->core));
			// Если используется unix-сокет
			} else {
				// Запускаем чтение данных
				this->enabled(method_t::READ, it->first, wrk->wait);
				// Выводим в лог сообщение
				if(!this->noinfo) this->log->print("connect client to server [%s]", log_t::flag_t::INFO, this->unixSocket.c_str());
				// Выполняем функцию обратного вызова
				if(wrk->connectFn != nullptr) wrk->connectFn(it->first, wrk->wid, const_cast <awh::core_t *> (wrk->core));
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
					// Останавливаем таймаут ожидания на чтение из сокета
					adj->bev.timer.read.stop();
					// Выполняем перебор бесконечным циклом пока это разрешено
					while(!adj->bev.locked.read && (wrk->status.real == worker_t::mode_t::CONNECT)){
						// Выполняем зануление буфера
						memset(buffer, 0, sizeof(buffer));
						// Если дочерние активные подключения есть и сокет блокирующий
						if(!this->cores.empty() && (this->socket.isBlocking(adj->bev.socket) == 1)){
							// Переводим сокет в не блокирующий режим
							this->socket.nonBlocking(adj->bev.socket);
							// Если защищённый режим работы разрешён
							if(adj->ssl.mode){
								// Получаем BIO подключения
								BIO * bio = SSL_get_wbio(adj->ssl.ssl);
								// Устанавливаем блокирующий режим ввода/вывода для сокета
								if(bio != nullptr) BIO_set_nbio(bio, 1);
								// Флаг необходимо установить только для неблокирующего сокета
								SSL_set_mode(adj->ssl.ssl, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
							}
						}
						// Если защищённый режим работы разрешён
						if(adj->ssl.mode){
							// Выполняем очистку ошибок OpenSSL
							ERR_clear_error();
							// Выполняем чтение из защищённого сокета
							bytes = SSL_read(adj->ssl.ssl, buffer, sizeof(buffer));
						// Выполняем чтение данных из сокета
						} else bytes = recv(adj->bev.socket, buffer, sizeof(buffer), 0);
						// Останавливаем таймаут ожидания на чтение из сокета
						adj->bev.timer.read.stop();
						// Выполняем принудительное исполнение таймеров
						if(this->socket.isBlocking(adj->bev.socket) != 0) this->executeTimers();
						// Если время ожидания записи данных установлено
						if(adj->timeouts.read > 0)
							// Запускаем ожидание чтения данных с сервера
							adj->bev.timer.read.start(adj->timeouts.read);
						// Запускаем чтение данных снова
						if(bytes != 0) adj->bev.event.read.start();
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
									// Если unix-сокет не используется
									if(!this->isSetUnixSocket()){
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
									// Если функция обратного вызова установлена
									} else if(wrk->readFn != nullptr)
										// Выводим функцию обратного вызова
										wrk->readFn(buffer + offset, actual, aid, wrk->wid, reinterpret_cast <awh::core_t *> (this));
									// Увеличиваем смещение в буфере
									offset += actual;
								}
							// Если данных достаточно
							} else {
								// Если unix-сокет не используется
								if(!this->isSetUnixSocket()){
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
								// Если функция обратного вызова установлена
								} else if(wrk->readFn != nullptr)
									// Выводим функцию обратного вызова
									wrk->readFn(buffer, bytes, aid, wrk->wid, reinterpret_cast <awh::core_t *> (this));
							}
						// Если данные не могут быть прочитаны
						} else {
							// Получаем статус сокета
							const int status = this->socket.isBlocking(adj->bev.socket);
							// Если сокет находится в блокирующем режиме
							if((bytes < 0) && (status != 0))
								// Выполняем обработку ошибок
								this->error(bytes, aid);
							// Если произошла ошибка
							else if((bytes < 0) && (status == 0)) {
								// Если произошёл системный сигнал попробовать ещё раз
								if(errno == EINTR) continue;
								// Если защищённый режим работы разрешён
								if(adj->ssl.mode){
									// Получаем данные описание ошибки
									if(SSL_get_error(adj->ssl.ssl, bytes) == SSL_ERROR_WANT_READ)
										// Выполняем пропуск попытки
										break;
									// Иначе выводим сообщение об ошибке
									else this->error(bytes, aid);
								// Если защищённый режим работы запрещён
								} else if(errno == EAGAIN) break;
								// Иначе просто закрываем подключение
								this->close(aid);
							}
							// Если подключение разорвано или сокет находится в блокирующем режиме
							if((bytes == 0) || (status != 0))
								// Выполняем отключение от сервера
								this->close(aid);
						}
						// Выходим из цикла
						break;
					}
				} break;
				// Если производится запись данных
				case (uint8_t) method_t::WRITE: {
					// Останавливаем таймаут ожидания на запись в сокет
					adj->bev.timer.write.stop();
					// Если данных достаточно для записи в сокет
					if(adj->buffer.size() >= adj->marker.write.min){
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
							// Если защищённый режим работы разрешён
							if(adj->ssl.mode){
								// Выполняем очистку ошибок OpenSSL
								ERR_clear_error();
								// Выполняем отправку сообщения через защищённый канал
								bytes = SSL_write(adj->ssl.ssl, adj->buffer.data() + offset, actual);
							// Выполняем отправку сообщения в сокет
							} else bytes = send(adj->bev.socket, adj->buffer.data() + offset, actual, 0);
							// Останавливаем таймаут ожидания на запись в сокет
							adj->bev.timer.write.stop();
							// Выполняем принудительное исполнение таймеров
							if(this->socket.isBlocking(adj->bev.socket) != 0) this->executeTimers();
							// Если время ожидания записи данных установлено
							if(adj->timeouts.write > 0)
								// Запускаем ожидание запись данных на сервер
								adj->bev.timer.write.start(adj->timeouts.write);
							// Если байты не были записаны в сокет
							if(bytes <= 0){
								// Получаем статус сокета
								const int status = this->socket.isBlocking(adj->bev.socket);
								// Если сокет находится в блокирующем режиме
								if((bytes < 0) && (status != 0))
									// Выполняем обработку ошибок
									this->error(bytes, aid);
								// Если произошла ошибка
								else if((bytes < 0) && (status == 0)) {
									// Если произошёл системный сигнал попробовать ещё раз
									if(errno == EINTR) continue;
									// Если защищённый режим работы разрешён
									if(adj->ssl.mode){
										// Получаем данные описание ошибки
										if(SSL_get_error(adj->ssl.ssl, bytes) == SSL_ERROR_WANT_WRITE)
											// Выполняем пропуск попытки
											continue;
										// Иначе выводим сообщение об ошибке
										else this->error(bytes, aid);
									// Если защищённый режим работы запрещён
									} else if(errno == EAGAIN) continue;
									// Иначе просто закрываем подключение
									this->close(aid);
								}
								// Если подключение разорвано или сокет находится в блокирующем режиме
								if((bytes == 0) || (status != 0))
									// Выполняем отключение от сервера
									this->close(aid);
								// Выходим из цикла
								break;
							}
							// Увеличиваем смещение в буфере
							offset += actual;
						}
						// Получаем буфер отправляемых данных
						const vector <char> buffer = move(adj->buffer);
						// Останавливаем запись данных
						this->disabled(method_t::WRITE, aid);
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
			// Получаем размер буфера на чтение
			const int rcv = (!read.empty() ? this->fmk->sizeBuffer(read) : 0);
			// Получаем размер буфера на запись
			const int snd = (!write.empty() ? this->fmk->sizeBuffer(write) : 0);
			// Устанавливаем размер буфера
			if(adj->bev.socket > 0) this->socket.bufferSize(adj->bev.socket, rcv, snd, 1);
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
	this->type = type_t::CLIENT;
}
/**
 * ~Core Деструктор
 */
awh::client::Core::~Core() noexcept {
	// Выполняем остановку клиента
	this->stop();
}
