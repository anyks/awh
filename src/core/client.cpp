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
 * read Функция чтения данных с сокета сервера
 * @param bev буфер события
 * @param ctx передаваемый контекст
 */
void awh::client::Core::read(struct bufferevent * bev, void * ctx) noexcept {
	// Если подключение не передано
	if((bev != nullptr) && (ctx != nullptr)){
		// Получаем объект подключения
		awh::worker_t::adj_t * adj = reinterpret_cast <awh::worker_t::adj_t *> (ctx);
		// Получаем объект подключения
		client::worker_t * wrk = (client::worker_t *) const_cast <awh::worker_t *> (adj->parent);
		// Получаем объект ядра клиента
		const core_t * core = reinterpret_cast <const core_t *> (wrk->core);
		// Если подключение установлено
		if(wrk->status.real == client::worker_t::mode_t::CONNECT){
			// Если подключение ещё существует
			if((wrk->acquisition = (core->adjutants.count(adj->aid) > 0))){
				// Получаем буферы входящих данных
				struct evbuffer * input = bufferevent_get_input(adj->bev);
				// Получаем размер входящих данных
				const size_t size = evbuffer_get_length(input);
				// Если данные существуют
				if(size > 0){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Создаём буфер данных
						char * buffer = new char [size];
						// Копируем в буфер полученные данные
						evbuffer_remove(input, buffer, size);
						// Если включён мультипоточный режим
						if(core->multi){
							// Добавляем буфер бинарного чанка данных
							adj->add(buffer, size);
							// Добавляем полученные данные буфера в пул потоков
							const_cast <core_t *> (core)->pool.push(&thread, ref(* adj), ref(* wrk));
						// Если мультипоточный режим не включён
						} else {
							// Если подключение производится через, прокси-сервер
							if(wrk->isProxy()){
								// Если функция обратного вызова для вывода записи существует
								if(wrk->readProxyFn != nullptr)
									// Выводим функцию обратного вызова
									wrk->readProxyFn(buffer, size, adj->aid, wrk->wid, const_cast <awh::core_t *> (wrk->core), wrk->ctx);
							// Если прокси-сервер не используется
							} else if(wrk->readFn != nullptr)
								// Выводим функцию обратного вызова
								wrk->readFn(buffer, size, adj->aid, wrk->wid, const_cast <awh::core_t *> (wrk->core), wrk->ctx);
						}
						// Удаляем выделенную память буфера
						delete [] buffer;
					// Если возникает ошибка
					} catch(const bad_alloc &) {
						// Выводим в лог сообщение
						adj->log->print("%s", log_t::flag_t::CRITICAL, "unable to allocate enough memory");
						// Выходим из приложения
						exit(EXIT_FAILURE);
					}
				}
			}
		}
	}
}
/**
 * write Функция записи данных в сокет сервера
 * @param bev буфер события
 * @param ctx передаваемый контекст
 */
void awh::client::Core::write(struct bufferevent * bev, void * ctx) noexcept {
	// Если подключение не передано
	if((bev != nullptr) && (ctx != nullptr)){
		// Получаем объект подключения
		awh::worker_t::adj_t * adj = reinterpret_cast <awh::worker_t::adj_t *> (ctx);
		// Получаем объект подключения
		client::worker_t * wrk = (client::worker_t *) const_cast <awh::worker_t *> (adj->parent);
		// Получаем объект ядра клиента
		const core_t * core = reinterpret_cast <const core_t *> (wrk->core);
		// Если подключение установлено
		if(wrk->status.real == client::worker_t::mode_t::CONNECT){
			// Если подключение ещё существует
			if((wrk->acquisition = (core->adjutants.count(adj->aid) > 0))){
				// Получаем буферы исходящих данных
				struct evbuffer * output = bufferevent_get_output(bev);
				// Получаем размер исходящих данных
				const size_t size = evbuffer_get_length(output);
				// Если данные существуют
				if(size > 0){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Создаём буфер входящих данных
						char * buffer = new char [size];
						// Копируем в буфер полученные данные
						evbuffer_remove(output, buffer, size);
						// Если подключение производится через, прокси-сервер
						if(wrk->isProxy()){
							// Если функция обратного вызова для вывода записи существует
							if(wrk->writeProxyFn != nullptr)
								// Выводим функцию обратного вызова
								wrk->writeProxyFn(buffer, size, adj->aid, wrk->wid, const_cast <awh::core_t *> (wrk->core), wrk->ctx);
						// Если прокси-сервер не используется
						} else if(wrk->writeFn != nullptr)
							// Выводим функцию обратного вызова
							wrk->writeFn(buffer, size, adj->aid, wrk->wid, const_cast <awh::core_t *> (wrk->core), wrk->ctx);
						// Выполняем удаление буфера
						delete [] buffer;
					// Если возникает ошибка
					} catch(const bad_alloc &) {
						// Выводим в лог сообщение
						adj->log->print("%s", log_t::flag_t::WARNING, "unable to allocate enough memory");
						// Если подключение производится через, прокси-сервер
						if(wrk->isProxy()){
							// Если функция обратного вызова для вывода записи существует
							if(wrk->writeProxyFn != nullptr)
								// Выводим функцию обратного вызова
								wrk->writeProxyFn(nullptr, size, adj->aid, wrk->wid, const_cast <awh::core_t *> (wrk->core), wrk->ctx);
						// Если прокси-сервер не используется
						} else if(wrk->writeFn != nullptr)
							// Выводим пустое сообщение
							wrk->writeFn(nullptr, size, adj->aid, wrk->wid, const_cast <awh::core_t *> (wrk->core), wrk->ctx);
					}
				}
			}
		}
	}
}
/**
 * reconnect Функция задержки времени на реконнект
 * @param fd    файловый дескриптор (сокет)
 * @param event произошедшее событие
 * @param ctx   передаваемый контекст
 */
void awh::client::Core::reconnect(evutil_socket_t fd, short event, void * ctx) noexcept {
	// Если контекст модуля передан
	if(ctx != nullptr){
		// Получаем объект таймера
		core_t::timeout_t * tm = reinterpret_cast <core_t::timeout_t *> (ctx);
		// Выполняем поиск воркера
		auto it = tm->core->workers.find(tm->wid);
		// Если воркер найден
		if(it != tm->core->workers.end()){
			// Флаг запрещения выполнения операции
			bool disallow = false;
			// Очищаем объект таймаута базы событий
			evutil_timerclear(&tm->tv);
			// Выполняем удаление событие таймера
			event_del(&tm->ev);
			// Если в воркере есть подключённые клиенты
			if(!it->second->adjutants.empty()){
				// Выполняем перебор всех подключенных адъютантов
				for(auto & adjutant : it->second->adjutants){
					// Если блокировка адъютанта не установлена
					disallow = (tm->core->locking.count(adjutant.first) > 0);
					// Если в списке есть заблокированные адъютанты, выходим из цикла
					if(disallow) break;
				}
			}
			// Если разрешено выполнять дальнейшую операцию
			if(!disallow){
				// Определяем режим работы клиента
				switch((uint8_t) tm->mode){
					// Если режим работы клиента - это подключение
					case (uint8_t) client::worker_t::mode_t::CONNECT:
						// Выполняем новое подключение
						tm->core->connect(tm->wid);
					break;
					// Если режим работы клиента - это переподключение
					case (uint8_t) client::worker_t::mode_t::RECONNECT: {
						// Получаем объект воркера
						client::worker_t * wrk = (client::worker_t *) const_cast <awh::worker_t *> (it->second);
						// Устанавливаем флаг ожидания статуса
						wrk->status.wait = client::worker_t::mode_t::DISCONNECT;
						// Выполняем новую попытку подключиться
						tm->core->reconnect(wrk->wid);
					} break;
				}
			}
		}
	}
}
/**
 * event Функция обработка входящих событий с сервера
 * @param bev    буфер события
 * @param events произошедшее событие
 * @param ctx    передаваемый контекст
 */
void awh::client::Core::event(struct bufferevent * bev, const short events, void * ctx) noexcept {
	// Если подключение не передано
	if(ctx != nullptr){
		// Получаем объект подключения
		awh::worker_t::adj_t * adj = reinterpret_cast <awh::worker_t::adj_t *> (ctx);
		// Получаем объект подключения
		client::worker_t * wrk = (client::worker_t *) const_cast <awh::worker_t *> (adj->parent);
		// Если подключение ещё существует
		if((adj->fmk != nullptr) && (wrk->core != nullptr)){
			// Получаем объект ядра клиента
			const core_t * core = reinterpret_cast <const core_t *> (wrk->core);
			// Если список адъютантов не пустой и адъютант найден
		 	if(!core->adjutants.empty() && (core->adjutants.count(adj->aid) > 0)){
				// Получаем URL параметры запроса
				const uri_t::url_t & url = (wrk->isProxy() ? wrk->proxy.url : wrk->url);
				// Получаем хост сервера
				const string & host = (!url.ip.empty() ? url.ip : url.domain);
				// Если подключение удачное и работа воркера разрешена
				if((bev != nullptr) && (events & BEV_EVENT_CONNECTED) && (wrk->status.work == client::worker_t::work_t::ALLOW)){
					// Снимаем флаг получения данных
					wrk->acquisition = false;
					// Выполняем очистку существующих таймаутов
					const_cast <core_t *> (core)->clearTimeout(wrk->wid);
					// Устанавливаем статус подключения к серверу
					wrk->status.real = client::worker_t::mode_t::CONNECT;
					// Устанавливаем флаг ожидания статуса
					wrk->status.wait = client::worker_t::mode_t::DISCONNECT;
					// Определяем тип подключения
					switch(core->net.family){
						// Резолвер IPv4, создаём резолвер
						case AF_INET:
							// Выполняем отмену ранее выполненных запросов DNS
							const_cast <core_t *> (core)->dns4.cancel(wrk->did);
						break;
						// Резолвер IPv6, создаём резолвер
						case AF_INET6:
							// Выполняем отмену ранее выполненных запросов DNS
							const_cast <core_t *> (core)->dns6.cancel(wrk->did);
						break;
					}
					// Выводим в лог сообщение
					if(!core->noinfo) adj->log->print("connect client to server [%s:%d]", log_t::flag_t::INFO, host.c_str(), url.port);
					// Если подключение производится через, прокси-сервер
					if(wrk->isProxy()){
						// Выполняем функцию обратного вызова для прокси-сервера
						if(wrk->connectProxyFn != nullptr) wrk->connectProxyFn(adj->aid, wrk->wid, const_cast <awh::core_t *> (wrk->core), wrk->ctx);
					// Выполняем функцию обратного вызова
					} else if(wrk->connectFn != nullptr) wrk->connectFn(adj->aid, wrk->wid, const_cast <awh::core_t *> (wrk->core), wrk->ctx);
					// Если флаг ожидания входящих сообщений, активирован
					if(wrk->wait){
						// Устанавливаем таймаут ожидания поступления данных
						struct timeval read = {adj->timeRead, 0};
						// Устанавливаем таймаут ожидания записи данных
						struct timeval write = {adj->timeWrite, 0};
						// Устанавливаем таймаут на отправку/получение данных
						bufferevent_set_timeouts(
							adj->bev,
							(adj->timeRead > 0 ? &read : nullptr),
							(adj->timeWrite > 0 ? &write : nullptr)
						);
					}
					// Выходим из функции
					return;
				// Если это ошибка или завершение работы
				} else if(events & (BEV_EVENT_ERROR | BEV_EVENT_TIMEOUT | BEV_EVENT_EOF)) {
					// Если это ошибка
					if(events & BEV_EVENT_ERROR)
						// Выводим в лог сообщение
						adj->log->print("closing server [%s:%d] %s", log_t::flag_t::WARNING, host.c_str(), url.port, evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
					// Если - это таймаут, выводим сообщение в лог
					else if(events & BEV_EVENT_TIMEOUT) {
						// Если данные ещё ни разу не получены
						if(!wrk->acquisition && !url.ip.empty()){
							// Определяем тип подключения
							switch(core->net.family){
								// Резолвер IPv4, добавляем бракованный IPv4 адрес в список адресов
								case AF_INET: const_cast <core_t *> (core)->dns4.setToBlackList(url.ip); break;
								// Резолвер IPv6, добавляем бракованный IPv6 адрес в список адресов
								case AF_INET6: const_cast <core_t *> (core)->dns6.setToBlackList(url.ip); break;
							}
						}
						// Выводим сообщение в лог, о таймауте подключения
						adj->log->print("timeout server [%s:%d]", log_t::flag_t::WARNING, host.c_str(), url.port);
					}
				}
			}
			// Выполняем отключение от сервера
			const_cast <core_t *> (core)->close(adj->aid);
		}
	}
}
/**
 * thread Функция сборки чанков бинарного буфера в многопоточном режиме
 * @param adj объект адъютанта
 * @param wrk объект воркера
 */
void awh::client::Core::thread(const awh::worker_t::adj_t & adj, const client::worker_t & wrk) noexcept {
	// Получаем объект ядра клиента
	core_t * core = (core_t *) const_cast <awh::core_t *> (wrk.core);
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(core->mtx.thread);
	// Выполняем получение буфера бинарного чанка данных
	const auto & buffer = const_cast <awh::worker_t::adj_t *> (&adj)->get();
	// Если буфер бинарных данных получен и подключение установлено
	if(!buffer.empty() && (wrk.status.real == client::worker_t::mode_t::CONNECT)){
		// Если подключение производится через, прокси-сервер
		if(wrk.isProxy()){
			// Если функция обратного вызова для вывода записи существует
			if(wrk.readProxyFn != nullptr)
				// Выводим функцию обратного вызова
				wrk.readProxyFn(buffer.data(), buffer.size(), adj.aid, wrk.wid, core, wrk.ctx);
		// Если прокси-сервер не используется
		} else if(wrk.readFn != nullptr)
			// Выводим функцию обратного вызова
			wrk.readFn(buffer.data(), buffer.size(), adj.aid, wrk.wid, core, wrk.ctx);
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
		// Получаем объект воркера
		client::worker_t * wrk = (client::worker_t *) const_cast <awh::worker_t *> (it->second->parent);
		// Устанавливаем время ожидания поступления данных
		const_cast <awh::worker_t::adj_t *> (it->second)->timeRead = wrk->timeRead;
		// Устанавливаем время ожидания записи данных
		const_cast <awh::worker_t::adj_t *> (it->second)->timeWrite = wrk->timeWrite;
		// Устанавливаем размер детектируемых байт на чтение
		const_cast <awh::worker_t::adj_t *> (it->second)->markRead = wrk->markRead;
		// Устанавливаем размер детектируемых байт на запись
		const_cast <awh::worker_t::adj_t *> (it->second)->markWrite = wrk->markWrite;
		// Устанавливаем коллбеки
		bufferevent_setcb(it->second->bev, &read, &write, &event, (void *) it->second);
		// Очищаем буферы событий при завершении работы
		bufferevent_flush(it->second->bev, EV_READ | EV_WRITE, BEV_FINISHED);
		/**
		 * Водяной знак на N байт (чтобы считывать данные когда они действительно приходят)
		 */
		// Устанавливаем размер считываемых данных
		bufferevent_setwatermark(it->second->bev, EV_READ, it->second->markRead.min, it->second->markRead.max);
		// Устанавливаем размер записываемых данных
		bufferevent_setwatermark(it->second->bev, EV_WRITE, it->second->markWrite.min, it->second->markWrite.max);
		// Активируем буферы событий на чтение и запись
		bufferevent_enable(it->second->bev, EV_READ | EV_WRITE);
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
							// Выполняем блокировку буфера бинарного чанка данных
							it->second->end();
							// Выполняем очистку буфера событий
							this->clean(&it->second->bev);
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
						// Выполняем получение контекста сертификата
						adj->ssl = this->ssl.init(url);
						// Устанавливаем первоначальное значение
						u_int mode = 0;
						// Если нужно использовать отложенные вызовы событий сокета
						if(this->defer) mode = (mode | BEV_OPT_DEFER_CALLBACKS);
						// Выполняем блокировку потока
						this->mtx.connect.lock();
						// Если SSL клиент разрешён
						if(adj->ssl.mode){
							// Создаем буфер событий для сервера зашифрованного подключения
							adj->bev = bufferevent_openssl_socket_new(this->base, sockaddr.fd, adj->ssl.ssl, BUFFEREVENT_SSL_CONNECTING, mode);
							// Разрешаем непредвиденное грязное завершение работы
							bufferevent_openssl_set_allow_dirty_shutdown(adj->bev, 1);
						// Создаем буфер событий для сервера
						} else adj->bev = bufferevent_socket_new(this->base, sockaddr.fd, mode);
						// Выполняем блокировку потока
						this->mtx.connect.unlock();
						// Если буфер событий создан
						if(adj->bev != nullptr){
							// Устанавливаем идентификатор адъютанта
							adj->aid = this->fmk->unixTimestamp();
							// Добавляем созданного адъютанта в список адъютантов
							auto ret = wrk->adjutants.emplace(adj->aid, move(adj));
							// Выполняем блокировку потока
							this->mtx.connect.lock();
							// Добавляем адъютанта в список подключений
							this->adjutants.emplace(ret.first->first, ret.first->second.get());
							// Выполняем блокировку потока
							this->mtx.connect.unlock();
							// Выполняем тюннинг буфера событий
							tuning(ret.first->first);
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
							if(bufferevent_socket_connect(ret.first->second->bev, sin, size) < 0){
								// Разрешаем выполнение работы
								wrk->status.work = client::worker_t::work_t::ALLOW;
								// Устанавливаем статус подключения
								wrk->status.real = client::worker_t::mode_t::DISCONNECT;
								// Запрещаем чтение запись данных серверу
								bufferevent_disable(ret.first->second->bev, EV_WRITE | EV_READ);
								// Выводим в лог сообщение
								this->log->print("connecting to host = %s, port = %u", log_t::flag_t::CRITICAL, url.ip.c_str(), url.port);
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
								// Выполняем отключение от сервера
								this->close(ret.first->first);
								// Выходим из функции
								return;
							}
							// Разрешаем выполнение работы
							wrk->status.work = client::worker_t::work_t::ALLOW;
							// Выводим в лог сообщение
							if(!core->noinfo) this->log->print("create good connect to host = %s [%s:%d], socket = %d", log_t::flag_t::INFO, url.domain.c_str(), url.ip.c_str(), url.port, sockaddr.fd);
							// Если статус подключения изменился
							if((ret.first->second->bev != nullptr) && (wrk->status.real != client::worker_t::mode_t::PRECONNECT))
								// Запрещаем чтение запись данных серверу
								bufferevent_disable(ret.first->second->bev, EV_WRITE | EV_READ);
							// Выходим из функции
							return;
						// Если подключение не выполнено, выводим в лог сообщение
						} else this->log->print("connecting to host = %s, port = %u", log_t::flag_t::CRITICAL, url.ip.c_str(), url.port);
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
			timeout = &it->second;
		// Если таймаут ещё не существует
		else {
			// Выполняем блокировку потока
			this->mtx.timeout.lock();
			// Получаем объект таймаута
			timeout = &this->timeouts.emplace(wid, timeout_t()).first->second;
			// Выполняем разблокировку потока
			this->mtx.timeout.unlock();
		}
		// Устанавливаем идентификатор таймаута
		timeout->wid = wid;
		// Устанавливаем режим работы клиента
		timeout->mode = mode;
		// Устанавливаем ядро клиента
		timeout->core = this;
		// Создаём событие на активацию базы событий
		event_assign(&timeout->ev, this->base, -1, EV_TIMEOUT, &reconnect, timeout);
		// Очищаем объект таймаута базы событий
		evutil_timerclear(&timeout->tv);
		// Устанавливаем интервал таймаута
		timeout->tv.tv_sec = 10;
		// Создаём событие таймаута на активацию базы событий
		event_add(&timeout->ev, &timeout->tv);
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
			// Флаг поддержания постоянного подключения
			bool alive = false;
			// Выполняем пинок базе событий
			this->dispatch.kick();
			// Выполняем отключение всех подключённых адъютантов
			this->close();
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
			// Очищаем объект таймаута базы событий
			evutil_timerclear(&it->second.tv);
			// Выполняем удаление событие таймера
			event_del(&it->second.ev);
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
		for(auto & timeout : this->timeouts){
			// Очищаем объект таймаута базы событий
			evutil_timerclear(&timeout.second.tv);
			// Выполняем удаление событие таймера
			event_del(&timeout.second.ev);
		}
	}
	// Если список воркеров активен
	if(!this->workers.empty()){
		// Если список подключённых ядер не пустой
		if(!this->cores.empty()){
			// Переходим по всем списка подключённым ядрам и устанавливаем новую базу событий
			for(auto & core : this->cores)
				// Выполняем отключение всех клиентов у подключённых ядер
				core.first->close();
		}
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
						// Выполняем блокировку буфера бинарного чанка данных
						it->second->end();
						// Выполняем очистку буфера событий
						this->clean(&it->second->bev);
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
				// Очищаем объект таймаута базы событий
				evutil_timerclear(&it->second.tv);
				// Выполняем удаление событие таймера
				event_del(&it->second.ev);
				// Выполняем удаление текущего таймаута
				it = this->timeouts.erase(it);
				// Выполняем разблокировку потока
				this->mtx.timeout.unlock();
			}
		}
		// Если список подключённых ядер не пустой
		if(!this->cores.empty()){
			// Переходим по всем списка подключённым ядрам и устанавливаем новую базу событий
			for(auto & core : this->cores)
				// Выполняем удаление всех воркеров у подключённых ядер
				core.first->remove();
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
						// Выполняем блокировку буфера бинарного чанка данных
						adj->end();
						// Выполняем очистку буфера событий
						this->clean(&adj->bev);
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
				// Очищаем объект таймаута базы событий
				evutil_timerclear(&it->second.tv);
				// Выполняем удаление событие таймера
				event_del(&it->second.ev);
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
			client::worker_t * wrk = (client::worker_t *) const_cast <awh::worker_t *> (adj->parent);
			// Получаем объект ядра клиента
			const core_t * core = reinterpret_cast <const core_t *> (wrk->core);
			// Выполняем блокировку буфера бинарного чанка данных
			adj->end();
			// Если событие сервера существует
			if(adj->bev != nullptr)
				// Выполняем очистку буфера событий
				this->clean(&adj->bev);
			// Удаляем установленный таймаут, если он существует
			this->clearTimeout(wrk->wid);
			// Если прокси-сервер активирован но уже переключён на работу с сервером
			if((wrk->proxy.type != proxy_t::type_t::NONE) && !wrk->isProxy())
				// Выполняем переключение обратно на прокси-сервер
				wrk->switchConnect();
			// Выполняем удаление контекста SSL
			this->ssl.clear(adj->ssl);
			// Удаляем адъютанта из списка адъютантов
			wrk->adjutants.erase(aid);
			// Удаляем адъютанта из списка подключений
			this->adjutants.erase(aid);
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
					// Выводим функцию обратного вызова
					if(wrk->disconnectFn != nullptr) wrk->disconnectFn(aid, wrk->wid, this, wrk->ctx);
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
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Получаем объект адъютанта
		awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (it->second);
		// Получаем объект воркера
		client::worker_t * wrk = (client::worker_t *) const_cast <awh::worker_t *> (adj->parent);
		// Если прокси-сервер активирован но ещё не переключён на работу с сервером
		if((wrk->proxy.type != proxy_t::type_t::NONE) && wrk->isProxy()){
			// Выполняем переключение на работу с сервером
			wrk->switchConnect();
			// Выполняем получение контекста сертификата
			adj->ssl = this->ssl.init(wrk->url);
			// Если SSL клиент разрешен
			if(adj->ssl.mode){
				// Выполняем блокировку потока
				this->mtx.proxy.lock();
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
			// Получаем файловый дескриптор
			evutil_socket_t fd = bufferevent_getfd(adj->bev);
			// Устанавливаем размер буфера
			if(fd > 0) this->socket.bufferSize(fd, rcv, snd, 1);
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
