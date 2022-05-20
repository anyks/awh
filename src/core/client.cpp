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
					core->reconnect(wrk->wid);
				break;
			}
			// Выходим из функции
			return;
		// Если IP адрес не получен но нужно поддерживать постоянное подключение
		} else if(wrk->alive) {
			// Получаем объект ядра подключения
			core_t * core = (core_t *) const_cast <awh::core_t *> (wrk->core);
			// Создаём событие на активацию базы событий
			event_assign(&core->timeout, core->base, -1, EV_TIMEOUT, &attempt, wrk);
			// Очищаем объект таймаута базы событий
			evutil_timerclear(&core->tvTimeout);
			// Устанавливаем интервал таймаута
			core->tvTimeout.tv_sec = 10;
			// Создаём событие таймаута на активацию базы событий
			event_add(&core->timeout, &core->tvTimeout);
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
			// Если данные ещё ни разу не получены
			if(!core->acquisition){
				// Определяем тип подключения
				switch(core->net.family){
					// Резолвер IPv4, очищаем чёрный список IPv4 бракованных адресов
					case AF_INET: const_cast <core_t *> (core)->dns4.clearBlackList(); break;
					// Резолвер IPv6, очищаем чёрный список IPv4 бракованных адресов
					case AF_INET6: const_cast <core_t *> (core)->dns6.clearBlackList(); break;
				}
			}
			// Если подключение ещё существует
			if((const_cast <core_t *> (core)->acquisition = (core->adjutants.count(adj->aid) > 0))){
				// Получаем буферы входящих данных
				struct evbuffer * input = bufferevent_get_input(adj->bev);
				// Получаем размер входящих данных
				const size_t size = evbuffer_get_length(input);
				// Если данные существуют
				if(size > 0){
					/*
					* Выполняем отлов ошибок
					*/
					try {
						// Создаём буфер данных
						char * buffer = new char [size];
						// Копируем в буфер полученные данные
						evbuffer_remove(input, buffer, size);
						// Если включён мультипоточный режим
						if(core->thr){
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
			// Если данные ещё ни разу не получены
			if(!core->acquisition){
				// Определяем тип подключения
				switch(core->net.family){
					// Резолвер IPv4, очищаем чёрный список IPv4 бракованных адресов
					case AF_INET: const_cast <core_t *> (core)->dns4.clearBlackList(); break;
					// Резолвер IPv6, очищаем чёрный список IPv4 бракованных адресов
					case AF_INET6: const_cast <core_t *> (core)->dns6.clearBlackList(); break;
				}
			}
			// Если подключение ещё существует
			if((const_cast <core_t *> (core)->acquisition = (core->adjutants.count(adj->aid) > 0))){
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
 * attempt Функция задержки времени на новую попытку получить IP адрес
 * @param fd    файловый дескриптор (сокет)
 * @param event произошедшее событие
 * @param ctx   передаваемый контекст
 */
void awh::client::Core::attempt(evutil_socket_t fd, short event, void * ctx) noexcept {
	// Если контекст модуля передан
	if(ctx != nullptr){
		// Получаем объект воркера
		worker_t * wrk = reinterpret_cast <worker_t *> (ctx);
		// Получаем объект ядра подключения
		core_t * core = (core_t *) const_cast <awh::core_t *> (wrk->core);
		// Выполняем удаление событие таймера
		event_del(&core->timeout);
		// Выполняем новую попытку подключиться
		core->restore(wrk->wid);
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
	if((ctx != nullptr) && (bev != nullptr)){
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
				// Если подключение удачное
				if(events & BEV_EVENT_CONNECTED){
					// Снимаем флаг получения данных
					const_cast <core_t *> (core)->acquisition = false;
					// Устанавливаем статус подключения к серверу
					wrk->status.real = client::worker_t::mode_t::CONNECT;
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
						if(!core->acquisition && !url.ip.empty()){
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
			// Запрещаем чтение запись данных серверу
			bufferevent_disable(bev, EV_WRITE | EV_READ);
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
	// Выполняем получение буфера бинарного чанка данных
	const auto & buffer = const_cast <awh::worker_t::adj_t *> (&adj)->get();
	// Если буфер бинарных данных получен и подключение установлено
	if(!buffer.empty() && (wrk.status.real == client::worker_t::mode_t::CONNECT)){
		// Выполняем блокировку потока
		const lock_guard <mutex> lock(core->locker.work);
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
			// Если подключение ещё не выполнено
			if(wrk->status.real != client::worker_t::mode_t::CONNECT){
				// Размер структуры подключения
				socklen_t size = 0;
				// Объект подключения
				struct sockaddr * sin = nullptr;
				// Получаем объект ядра клиента
				const core_t * core = reinterpret_cast <const core_t *> (wrk->core);
				// Получаем URL параметры запроса
				const uri_t::url_t & url = (wrk->isProxy() ? wrk->proxy.url : wrk->url);
				// Получаем сокет для подключения к серверу
				auto sockaddr = this->sockaddr(url.ip, url.port, this->net.family);
				// Если сокет создан удачно
				if(sockaddr.fd > -1){
					// Создаём бъект адъютанта
					unique_ptr <awh::worker_t::adj_t> adj(new awh::worker_t::adj_t(wrk, this->fmk, this->log));
					// Выполняем получение контекста сертификата
					adj->ssl = this->ssl.init(url);
					// Устанавливаем первоначальное значение
					u_int mode = BEV_OPT_THREADSAFE;
					// Если нужно использовать отложенные вызовы событий сокета
					if(this->defer) mode = (mode | BEV_OPT_DEFER_CALLBACKS);
					// Если SSL клиент разрешён
					if(adj->ssl.mode){
						// Создаем буфер событий для сервера зашифрованного подключения
						adj->bev = bufferevent_openssl_socket_new(this->base, sockaddr.fd, adj->ssl.ssl, BUFFEREVENT_SSL_CONNECTING, mode);
						// Разрешаем непредвиденное грязное завершение работы
						bufferevent_openssl_set_allow_dirty_shutdown(adj->bev, 1);
					// Создаем буфер событий для сервера
					} else adj->bev = bufferevent_socket_new(this->base, sockaddr.fd, mode);
					// Если буфер событий создан
					if(adj->bev != nullptr){
						// Устанавливаем идентификатор адъютанта
						adj->aid = this->fmk->unixTimestamp();
						// Добавляем созданного адъютанта в список адъютантов
						auto ret = wrk->adjutants.emplace(adj->aid, move(adj));
						// Добавляем адъютанта в список подключений
						this->adjutants.emplace(ret.first->first, ret.first->second.get());
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
							// Выводим в лог сообщение
							this->log->print("connecting to host = %s, port = %u", log_t::flag_t::CRITICAL, url.ip.c_str(), url.port);
							// Определяем тип подключения
							switch(this->net.family){
								// Резолвер IPv4, выполняем сброс кэша резолвера
								case AF_INET: this->dns4.flush(); break;
								// Резолвер IPv6, выполняем сброс кэша резолвера
								case AF_INET6: this->dns6.flush(); break;
							}
							// Выполняем отключение от сервера
							this->close(ret.first->first);
						}
						// Выводим в лог сообщение
						if(!core->noinfo) this->log->print("create good connect to host = %s [%s:%d], socket = %d", log_t::flag_t::INFO, url.domain.c_str(), url.ip.c_str(), url.port, sockaddr.fd);
						// Выходим из функции
						return;
					// Если подключение не выполнено, выводим в лог сообщение
					} else this->log->print("connecting to host = %s, port = %u", log_t::flag_t::CRITICAL, url.ip.c_str(), url.port);
				}
				// Если нужно выполнить автоматическое переподключение
				if(wrk->alive){
					// Выполняем переподключение
					this->restore(wid);
					// Выходим из функции
					return;
				// Если все попытки исчерпаны
				} else {
					// Определяем тип подключения
					switch(this->net.family){
						// Резолвер IPv4, выполняем сброс кэша резолвера
						case AF_INET: this->dns4.flush(); break;
						// Резолвер IPv6, выполняем сброс кэша резолвера
						case AF_INET6: this->dns6.flush(); break;
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
 * sendTimeout Метод отправки принудительного таймаута
 * @param aid идентификатор адъютанта
 */
void awh::client::Core::sendTimeout(const size_t aid) noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Получаем объект подключения
		awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (it->second);
		// Отправляем принудительный сигнал таймаута
		event(adj->bev, BEV_EVENT_TIMEOUT, adj);
	}
}
/**
 * closeAll Метод отключения всех воркеров
 */
void awh::client::Core::closeAll() noexcept {
	// Если список подключений активен
	if(!this->workers.empty()){
		// Переходим по всему списку подключений
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
					// Выполняем блокировку буфера бинарного чанка данных
					it->second->end();
					// Выполняем очистку буфера событий
					this->clean(it->second->bev);
					// Выполняем удаление контекста SSL
					this->ssl.clear(it->second->ssl);
					// Выводим функцию обратного вызова
					if(wrk->disconnectFn != nullptr)
						// Выполняем функцию обратного вызова
						wrk->disconnectFn(it->first, worker.first, this, wrk->ctx);
					// Удаляем адъютанта из списка
					it = wrk->adjutants.erase(it);
				}
			}
		}
	}
	// Выполняем очистку списка адъютантов
	this->adjutants.clear();
}
/**
 * run Метод запуска сервера воркером
 * @param wid идентификатор воркера
 */
void awh::client::Core::run(const size_t wid) noexcept {
	// Блокируем переданный идентификатор
	(void) wid;
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
			// Если параметры URL запроса переданы
			if(!wrk->url.empty()){
				// Устанавливаем флаг ожидания статуса
				wrk->status.wait = client::worker_t::mode_t::CONNECT;
				// Получаем URL параметры запроса
				const uri_t::url_t & url = (wrk->isProxy() ? wrk->proxy.url : wrk->url);
				// Если IP адрес не получен
				if(url.ip.empty() && !url.domain.empty())
					// Определяем тип подключения
					switch(this->net.family){
						// Резолвер IPv4, создаём резолвер
						case AF_INET: this->dns4.resolve(wrk, url.domain, AF_INET, &resolver); break;
						// Резолвер IPv6, создаём резолвер
						case AF_INET6: this->dns6.resolve(wrk, url.domain, AF_INET6, &resolver); break;
					}
				// Выполняем запуск системы
				else if(!url.ip.empty()) resolver(url.ip, wrk);
			}
		}
	}
}
/**
 * close Метод закрытия подключения воркера
 * @param aid идентификатор адъютанта
 */
void awh::client::Core::close(const size_t aid) noexcept {
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
		if(adj->bev != nullptr){
			// Выполняем очистку буфера событий
			this->clean(adj->bev);
			// Устанавливаем что событие удалено
			adj->bev = nullptr;
		}
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
		// Если нужно выполнить автоматическое переподключение
		if(wrk->alive) this->restore(wrk->wid);
		// Если автоматическое подключение выполнять не нужно
		else {
			// Выводим сообщение об ошибке
			if(!core->noinfo) this->log->print("%s", log_t::flag_t::INFO, "disconnected from the server");
			// Выводим функцию обратного вызова
			if(wrk->disconnectFn != nullptr) wrk->disconnectFn(aid, wrk->wid, this, wrk->ctx);
		}
	}
}
/**
 * restore Метод восстановления подключения
 * @param wid идентификатор воркера
 */
void awh::client::Core::restore(const size_t wid) noexcept {
	// Выполняем поиск воркера
	auto it = this->workers.find(wid);
	// Если воркер найден
	if(it != this->workers.end()){
		// Получаем объект воркера
		client::worker_t * wrk = (client::worker_t *) const_cast <awh::worker_t *> (it->second);
		// Если параметры URL запроса переданы
		if(!wrk->url.empty()){
			// Устанавливаем флаг ожидания статуса
			wrk->status.wait = client::worker_t::mode_t::RECONNECT;
			// Получаем URL параметры запроса
			const uri_t::url_t & url = (wrk->isProxy() ? wrk->proxy.url : wrk->url);
			// Определяем тип подключения
			switch(this->net.family){
				// Резолвер IPv4, создаём резолвер
				case AF_INET: {
					// Выполняем сброс кэша DNS резолвера
					this->dns4.flush();
					// Выполняем резолвинг домена
					this->dns4.resolve(wrk, (!url.domain.empty() ? url.domain : url.ip), AF_INET, &resolver);
				} break;
				// Резолвер IPv6, создаём резолвер
				case AF_INET6: {
					// Выполняем сброс кэша DNS резолвера
					this->dns6.flush();
					// Выполняем резолвинг домена
					this->dns6.resolve(wrk, (!url.domain.empty() ? url.domain : url.ip), AF_INET6, &resolver);
				} break;
			}
		}
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
				// Устанавливаем первоначальное значение
				u_int mode = BEV_OPT_THREADSAFE;
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
awh::client::Core::Core(const fmk_t * fmk, const log_t * log) noexcept : awh::core_t(fmk, log), acquisition(false) {
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
