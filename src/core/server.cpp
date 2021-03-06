/**
 * @file: server.cpp
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
#include <core/server.hpp>

/**
 * resolver Функция выполнения резолвинга домена
 * @param ip  полученный IP адрес
 * @param ctx передаваемый контекст
 */
void awh::server::Core::resolver(const string ip, void * ctx) noexcept {
	// Если передаваемый контекст передан
	if(ctx != nullptr){
		// Получаем объект воркера
		server::worker_t * wrk = reinterpret_cast <server::worker_t *> (ctx);
		// Получаем объект ядра подключения
		core_t * core = (core_t *) const_cast <awh::core_t *> (wrk->core);
		// Если IP адрес получен
		if(!ip.empty()){
			// sudo lsof -i -P | grep 1080
			// Обновляем хост сервера
			wrk->host = ip;
			// Получаем сокет сервера
			wrk->fd = core->sockaddr(wrk->host, wrk->port, core->net.family).fd;
			// Выполняем чтение сокета
			if(::listen(wrk->fd, wrk->total) < 0){
				// Выводим в консоль информацию
				if(!core->noinfo) core->log->print("listen service: pid = %u", log_t::flag_t::CRITICAL, getpid());
				// Останавливаем работу сервера
				core->stop();
				// Выходим
				return;
			}
			// Добавляем событие в базу
			wrk->ev = event_new(core->base, wrk->fd, EV_READ | EV_PERSIST, &accept, wrk);
			// Активируем событие
			event_add(wrk->ev, nullptr);
			// Выводим сообщение об активации
			if(!core->noinfo) core->log->print("run server [%s:%u]", log_t::flag_t::INFO, wrk->host.c_str(), wrk->port);
		// Если IP адрес сервера не получен
		} else {
			// Выводим в консоль информацию
			core->log->print("broken host server %s", log_t::flag_t::CRITICAL, wrk->host.c_str());
			// Останавливаем работу сервера
			core->stop();
		}
	}
}
/**
 * read Функция чтения данных с сокета сервера
 * @param bev буфер события
 * @param ctx передаваемый контекст
 */
void awh::server::Core::read(struct bufferevent * bev, void * ctx) noexcept {
	// Если подключение не передано
	if((bev != nullptr) && (ctx != nullptr)){
		// Получаем объект подключения
		awh::worker_t::adj_t * adj = reinterpret_cast <awh::worker_t::adj_t *> (ctx);
		// Получаем объект подключения
		server::worker_t * wrk = (server::worker_t *) const_cast <awh::worker_t *> (adj->parent);
		// Получаем объект ядра клиента
		const core_t * core = reinterpret_cast <const core_t *> (wrk->core);
		// Если подключение ещё существует
		if((core->adjutants.count(adj->aid) > 0) && (wrk->readFn != nullptr)){
			// Получаем буферы входящих данных
			struct evbuffer * input = bufferevent_get_input(bev);
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
					// Выводим функцию обратного вызова
					} else wrk->readFn(buffer, size, adj->aid, wrk->wid, const_cast <awh::core_t *> (wrk->core), wrk->ctx);
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
/**
 * write Функция записи данных в сокет сервера
 * @param bev буфер события
 * @param ctx передаваемый контекст
 */
void awh::server::Core::write(struct bufferevent * bev, void * ctx) noexcept {
	// Если подключение не передано
	if((bev != nullptr) && (ctx != nullptr)){
		// Получаем объект подключения
		awh::worker_t::adj_t * adj = reinterpret_cast <awh::worker_t::adj_t *> (ctx);
		// Получаем объект подключения
		server::worker_t * wrk = (server::worker_t *) const_cast <awh::worker_t *> (adj->parent);
		// Получаем объект ядра клиента
		const core_t * core = reinterpret_cast <const core_t *> (wrk->core);
		// Если подключение ещё существует
		if((core->adjutants.count(adj->aid) > 0) && (wrk->writeFn != nullptr)){
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
					// Выводим функцию обратного вызова
					wrk->writeFn(buffer, size, adj->aid, wrk->wid, const_cast <awh::core_t *> (wrk->core), wrk->ctx);
					// Выполняем удаление буфера
					delete [] buffer;
				// Если возникает ошибка
				} catch(const bad_alloc &) {
					// Выводим в лог сообщение
					adj->log->print("%s", log_t::flag_t::WARNING, "unable to allocate enough memory");
					// Выводим пустое сообщение
					wrk->writeFn(nullptr, size, adj->aid, wrk->wid, const_cast <awh::core_t *> (wrk->core), wrk->ctx);
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
void awh::server::Core::event(struct bufferevent * bev, const short events, void * ctx) noexcept {
	// Если подключение не передано
	if((ctx != nullptr) && (bev != nullptr)){
		// Получаем объект подключения
		awh::worker_t::adj_t * adj = reinterpret_cast <awh::worker_t::adj_t *> (ctx);
		// Получаем объект подключения
		server::worker_t * wrk = (server::worker_t *) const_cast <awh::worker_t *> (adj->parent);
		// Получаем объект ядра клиента
		const core_t * core = reinterpret_cast <const core_t *> (wrk->core);
		// Если подключение ещё существует
		if((core->adjutants.count(adj->aid) > 0) && (adj->fmk != nullptr)){
			// Получаем файловый дескриптор
			evutil_socket_t fd = bufferevent_getfd(bev);
			// Если это ошибка или завершение работы
			if(events & (BEV_EVENT_ERROR | BEV_EVENT_TIMEOUT | BEV_EVENT_EOF)) {
				// Если это ошибка
				if(events & BEV_EVENT_ERROR)
					// Выводим в лог сообщение
					adj->log->print("closing client, host = %s, mac = %s, socket = %d %s", log_t::flag_t::WARNING, adj->ip.c_str(), adj->mac.c_str(), fd, evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
				// Если - это таймаут, выводим сообщение в лог
				else if(events & BEV_EVENT_TIMEOUT) adj->log->print("timeout client, host = %s, mac = %s, socket = %d", log_t::flag_t::WARNING, adj->ip.c_str(), adj->mac.c_str(), fd);
				// Выполняем отключение от сервера
				const_cast <core_t *> (core)->close(adj->aid);
			}
		}
	}
}
/**
 * accept Функция подключения к серверу
 * @param fd    файловый дескриптор (сокет)
 * @param event событие на которое сработала функция обратного вызова
 * @param ctx   объект передаваемый как значение
 */
void awh::server::Core::accept(const evutil_socket_t fd, const short event, void * ctx) noexcept {
	// Если прокси существует
	if(ctx != nullptr){
		// IP и MAC адрес подключения
		string ip = "", mac = "";
		// Сокет подключившегося клиента
		evutil_socket_t socket = -1;
		// Получаем объект воркера
		server::worker_t * wrk = reinterpret_cast <server::worker_t *> (ctx);
		// Получаем объект подключения
		core_t * core = (core_t *) const_cast <awh::core_t *> (wrk->core);
		// Определяем тип подключения
		switch(core->net.family){
			// Для протокола IPv4
			case AF_INET: {
				// Структура получения
				struct sockaddr_in client;
				// Размер структуры подключения
				socklen_t len = sizeof(client);
				// Определяем разрешено ли подключение к прокси серверу
				socket = ::accept(fd, reinterpret_cast <struct sockaddr *> (&client), &len);
				// Если сокет не создан тогда выходим
				if(socket < 0) return;
				// Получаем данные подключившегося клиента
				ip = core->ifnet.ip((struct sockaddr *) &client, AF_INET);
				// Если IP адрес получен пустой, устанавливаем адрес сервера
				if(ip.compare("0.0.0.0") == 0) ip = core->ifnet.ip(AF_INET);
				// Получаем данные мак адреса клиента
				mac = core->ifnet.mac(ip, AF_INET);
			} break;
			// Для протокола IPv6
			case AF_INET6: {
				// Структура получения
				struct sockaddr_in6 client;
				// Размер структуры подключения
				socklen_t len = sizeof(client);
				// Определяем разрешено ли подключение к прокси серверу
				socket = ::accept(fd, reinterpret_cast <struct sockaddr *> (&client), &len);
				// Если сокет не создан тогда выходим
				if(socket < 0) return;
				// Получаем данные подключившегося клиента
				ip = core->ifnet.ip((struct sockaddr *) &client, AF_INET6);
				// Если IP адрес получен пустой, устанавливаем адрес сервера
				if(ip.compare("::") == 0) ip = core->ifnet.ip(AF_INET6);
				// Получаем данные мак адреса клиента
				mac = core->ifnet.mac(ip, AF_INET6);
			} break;
		}
		// Если функция обратного вызова установлена
		if(wrk->acceptFn != nullptr){
			// Выполняем проверку, разрешено ли клиенту подключиться к серверу
			if(!wrk->acceptFn(ip, mac, wrk->wid, core, wrk->ctx)){
				// Выполняем закрытие сокета
				core->close(socket);
				// Выводим в лог сообщение
				core->log->print("broken client, host = %s, mac = %s, socket = %d", log_t::flag_t::WARNING, ip.c_str(), mac.c_str(), socket);
				// Выходим из функции
				return;
			}
		}
		// Устанавливаем настройки для *Nix подобных систем
		#if !defined(_WIN32) && !defined(_WIN64)
			// Выполняем игнорирование сигнала неверной инструкции процессора
			core->socket.noSigill();
			// Отключаем сигнал записи в оборванное подключение
			core->socket.noSigpipe(socket);
		#endif
		// Отключаем алгоритм Нейгла для сервера и клиента
		core->socket.tcpNodelay(socket);
		// Переводим сокет в не блокирующий режим
		evutil_make_socket_nonblocking(socket);
		// Устанавливаем разрешение на повторное использование сокета
		evutil_make_listen_socket_reuseable(socket);
		// Создаём бъект адъютанта
		unique_ptr <awh::worker_t::adj_t> adj(new awh::worker_t::adj_t(wrk, core->fmk, core->log));
		// Выполняем получение контекста сертификата
		adj->ssl = core->ssl.init();
		// Устанавливаем первоначальное значение
		u_int mode = 0;
		// Если нужно использовать отложенные вызовы событий сокета
		if(core->defer) mode = (mode | BEV_OPT_DEFER_CALLBACKS);
		// Выполняем блокировку потока
		core->mtx.accept.lock();
		// Если SSL клиент разрешён
		if(adj->ssl.mode){
			// Создаем буфер событий для сервера зашифрованного подключения
			adj->bev = bufferevent_openssl_socket_new(core->base, socket, adj->ssl.ssl, BUFFEREVENT_SSL_ACCEPTING, mode);
			// Разрешаем непредвиденное грязное завершение работы
			bufferevent_openssl_set_allow_dirty_shutdown(adj->bev, 1);
		// Создаем буфер событий для сервера
		} else adj->bev = bufferevent_socket_new(core->base, socket, mode);
		// Выполняем блокировку потока
		core->mtx.accept.unlock();
		// Если буфер событий создан
		if(adj->bev != nullptr){
			// Запоминаем IP адрес
			adj->ip = move(ip);
			// Запоминаем MAC адрес
			adj->mac = move(mac);
			// Устанавливаем идентификатор адъютанта
			adj->aid = core->fmk->unixTimestamp();
			// Добавляем созданного адъютанта в список адъютантов
			auto ret = wrk->adjutants.emplace(adj->aid, move(adj));
			// Выполняем блокировку потока
			core->mtx.accept.lock();
			// Добавляем адъютанта в список подключений
			core->adjutants.emplace(ret.first->first, ret.first->second.get());
			// Выполняем блокировку потока
			core->mtx.accept.unlock();
			// Выполняем тюннинг буфера событий
			core->tuning(ret.first->first);
			// Выполняем функцию обратного вызова
			if(wrk->connectFn != nullptr) wrk->connectFn(ret.first->second->aid, wrk->wid, core, wrk->ctx);
			// Если флаг ожидания входящих сообщений, активирован
			if(wrk->wait){
				// Устанавливаем таймаут ожидания поступления данных
				struct timeval read = {ret.first->second->timeRead, 0};
				// Устанавливаем таймаут ожидания записи данных
				struct timeval write = {ret.first->second->timeWrite, 0};
				// Устанавливаем таймаут на отправку/получение данных
				bufferevent_set_timeouts(
					ret.first->second->bev,
					(ret.first->second->timeRead > 0 ? &read : nullptr),
					(ret.first->second->timeWrite > 0 ? &write : nullptr)
				);
			}
			// Выводим в консоль информацию
			if(!core->noinfo) core->log->print("client connect to server, host = %s, mac = %s, socket = %d", log_t::flag_t::INFO, ret.first->second->ip.c_str(), ret.first->second->mac.c_str(), socket);
		// Выводим в лог сообщение
		} else core->log->print("client connect to server, host = %s, mac = %s, socket = %d", log_t::flag_t::CRITICAL, ip.c_str(), mac.c_str(), socket);
	}
}
/**
 * thread Функция сборки чанков бинарного буфера в многопоточном режиме
 * @param adj объект адъютанта
 * @param wrk объект воркера
 */
void awh::server::Core::thread(const awh::worker_t::adj_t & adj, const server::worker_t & wrk) noexcept {	
	// Получаем объект ядра клиента
	core_t * core = (core_t *) const_cast <awh::core_t *> (wrk.core);
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(core->mtx.thread);
	// Выполняем получение буфера бинарного чанка данных
	const auto & buffer = const_cast <awh::worker_t::adj_t *> (&adj)->get();
	// Если буфер бинарных данных получен
	if(!buffer.empty())
		// Выводим функцию обратного вызова
		wrk.readFn(buffer.data(), buffer.size(), adj.aid, wrk.wid, core, wrk.ctx);
}
/**
 * tuning Метод тюннинга буфера событий
 * @param aid идентификатор адъютанта
 */
void awh::server::Core::tuning(const size_t aid) noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Получаем объект воркера
		server::worker_t * wrk = (server::worker_t *) const_cast <awh::worker_t *> (it->second->parent);
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
 * close Метод закрытия сокета
 * @param fd файловый дескриптор (сокет) для закрытия
 */
void awh::server::Core::close(const evutil_socket_t fd) noexcept {
	// Если - это Windows
	#if defined(_WIN32) || defined(_WIN64)
		// Отключаем подключение для сокета
		if(fd > 0) shutdown(fd, SD_BOTH);
	// Если - это Unix
	#else
		// Отключаем подключение для сокета
		if(fd > 0) shutdown(fd, SHUT_RDWR);
	#endif
	// Закрываем подключение
	if(fd > 0) evutil_closesocket(fd);
}
/**
 * close Метод отключения всех воркеров
 */
void awh::server::Core::close() noexcept {
	// Если список воркеров активен
	if(!this->workers.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->mtx.close);
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
				server::worker_t * wrk = (server::worker_t *) const_cast <awh::worker_t *> (worker.second);
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
void awh::server::Core::remove() noexcept {
	// Если список воркеров активен
	if(!this->workers.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->mtx.close);
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
			server::worker_t * wrk = (server::worker_t *) const_cast <awh::worker_t *> (it->second);
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
			// Выполняем закрытие сокета
			this->close(wrk->fd);
			// Сбрасываем сокет
			wrk->fd = -1;
			// Если объект событий подключения к серверу создан
			if(wrk->ev != nullptr){
				// Удаляем событие
				event_del(wrk->ev);
				// Очищаем событие
				event_free(wrk->ev);
				// Зануляем объект события
				wrk->ev = nullptr;
			}
			// Выполняем удаление воркера
			it = this->workers.erase(it);
		}
	}
}
/**
 * run Метод запуска сервера воркером
 * @param wid идентификатор воркера
 */
void awh::server::Core::run(const size_t wid) noexcept {
	// Если идентификатор воркера передан
	if(wid > 0){
		// Выполняем поиск воркера
		auto it = this->workers.find(wid);
		// Если воркер найден, устанавливаем максимальное количество одновременных подключений
		if(it != this->workers.end()){
			// Получаем объект подключения
			server::worker_t * wrk = (server::worker_t *) const_cast <awh::worker_t *> (it->second);
			// Определяем тип подключения
			switch(this->net.family){
				// Резолвер IPv4, создаём резолвер
				case AF_INET: this->dns4.resolve(wrk, wrk->host, AF_INET, resolver); break;
				// Резолвер IPv6, создаём резолвер
				case AF_INET6: this->dns6.resolve(wrk, wrk->host, AF_INET6, resolver); break;
			}
		}
	}
}
/**
 * remove Метод удаления воркера
 * @param wid идентификатор воркера
 */
void awh::server::Core::remove(const size_t wid) noexcept {
	// Если идентификатор воркера передан
	if(wid > 0){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->mtx.close);
		// Выполняем поиск воркера
		auto it = this->workers.find(wid);
		// Если воркер найден, устанавливаем максимальное количество одновременных подключений
		if(it != this->workers.end()){
			// Получаем объект воркера
			server::worker_t * wrk = (server::worker_t *) const_cast <awh::worker_t *> (it->second);
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
						// Выводим функцию обратного вызова
						if(wrk->disconnectFn != nullptr)
							// Выполняем функцию обратного вызова
							wrk->disconnectFn(jt->first, it->first, this, wrk->ctx);
						// Удаляем адъютанта из списка подключений
						this->adjutants.erase(jt->first);
						// Удаляем блокировку адъютанта
						this->locking.erase(jt->first);
						// Удаляем адъютанта из списка
						jt = wrk->adjutants.erase(jt);
					// Иначе продолжаем дальше
					} else ++jt;
				}
			}
			// Выполняем закрытие сокета
			this->close(wrk->fd);
			// Сбрасываем сокет
			wrk->fd = -1;
			// Если объект событий подключения к серверу создан
			if(wrk->ev != nullptr){
				// Удаляем событие
				event_del(wrk->ev);
				// Очищаем событие
				event_free(wrk->ev);
				// Зануляем объект события
				wrk->ev = nullptr;
			}
			// Выполняем удаление воркера
			this->workers.erase(wid);
		}
	}
}
/**
 * close Метод закрытия подключения воркера
 * @param aid идентификатор адъютанта
 */
void awh::server::Core::close(const size_t aid) noexcept {
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
			server::worker_t * wrk = (server::worker_t *) const_cast <awh::worker_t *> (adj->parent);
			// Получаем объект ядра клиента
			const core_t * core = reinterpret_cast <const core_t *> (wrk->core);
			// Выполняем блокировку буфера бинарного чанка данных
			adj->end();
			// Если событие сервера существует
			if(adj->bev != nullptr)
				// Выполняем очистку буфера событий
				this->clean(&adj->bev);
			// Выполняем удаление контекста SSL
			this->ssl.clear(adj->ssl);
			// Удаляем адъютанта из списка адъютантов
			wrk->adjutants.erase(aid);
			// Удаляем адъютанта из списка подключений
			this->adjutants.erase(aid);
			// Выводим сообщение об ошибке
			if(!core->noinfo) this->log->print("%s", log_t::flag_t::INFO, "disconnect client from server");
			// Выводим функцию обратного вызова
			if(wrk->disconnectFn != nullptr) wrk->disconnectFn(aid, wrk->wid, this, wrk->ctx);
		}
		// Удаляем блокировку адъютанта
		this->locking.erase(aid);
	}
}
/**
 * setBandwidth Метод установки пропускной способности сети
 * @param aid   идентификатор адъютанта
 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
 */
void awh::server::Core::setBandwidth(const size_t aid, const string & read, const string & write) noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Получаем объект адъютанта
		awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (it->second);
		// Получаем объект воркера
		server::worker_t * wrk = (server::worker_t *) const_cast <awh::worker_t *> (adj->parent);
		// Получаем размер буфера на чтение
		const int rcv = (!read.empty() ? this->fmk->sizeBuffer(read) : 0);
		// Получаем размер буфера на запись
		const int snd = (!write.empty() ? this->fmk->sizeBuffer(write) : 0);
		// Получаем файловый дескриптор
		evutil_socket_t fd = bufferevent_getfd(adj->bev);
		// Устанавливаем размер буфера
		if(fd > 0) this->socket.bufferSize(fd, rcv, snd, wrk->total);
	}
}
/**
 * setIpV6only Метод установки флага использования только сети IPv6
 * @param mode флаг для установки
 */
void awh::server::Core::setIpV6only(const bool mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.system);
	// Устанавливаем флаг использования только сети IPv6
	this->ipV6only = mode;
}
/**
 * setTotal Метод установки максимального количества одновременных подключений
 * @param wid   идентификатор воркера
 * @param total максимальное количество одновременных подключений
 */
void awh::server::Core::setTotal(const size_t wid, const u_short total) noexcept {
	// Если идентификатор воркера передан
	if(wid > 0){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->mtx.system);
		// Выполняем поиск воркера
		auto it = this->workers.find(wid);
		// Если воркер найден, устанавливаем максимальное количество одновременных подключений
		if(it != this->workers.end())
			// Устанавливаем максимальное количество одновременных подключений
			((server::worker_t *) const_cast <awh::worker_t *> (it->second))->total = total;
	}
}
/**
 * init Метод инициализации сервера
 * @param wid  идентификатор воркера
 * @param port порт сервера
 * @param host хост сервера
 */
void awh::server::Core::init(const size_t wid, const u_int port, const string & host) noexcept {
	// Если идентификатор воркера передан
	if(wid > 0){
		// Выполняем поиск воркера
		auto it = this->workers.find(wid);
		// Если воркер найден, устанавливаем максимальное количество одновременных подключений
		if(it != this->workers.end()){
			// Выполняем блокировку потока
			const lock_guard <recursive_mutex> lock(this->mtx.system);
			// Получаем объект подключения
			server::worker_t * wrk = (server::worker_t *) const_cast <awh::worker_t *> (it->second);
			// Если порт передан, устанавливаем
			if(port > 0) wrk->port = port;
			// Если хост передан, устанавливаем
			if(!host.empty()) wrk->host = host;
			// Иначе получаем IP адрес сервера автоматически
			else wrk->host = this->ifnet.ip(this->net.family);
		}
	}
}
/**
 * setCert Метод установки файлов сертификата
 * @param cert  корневой сертификат
 * @param key   приватный ключ сертификата
 * @param chain файл цепочки сертификатов
 */
void awh::server::Core::setCert(const string & cert, const string & key, const string & chain) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.system);
	// Устанавливаем файлы сертификата
	this->ssl.setCert(cert, key, chain);
}
/**
 * Core Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::Core::Core(const fmk_t * fmk, const log_t * log) noexcept : awh::core_t(fmk, log), ifnet(fmk, log) {
	// Устанавливаем тип запускаемого ядра
	this->type = type_t::SERVER;
}
/**
 * ~Core Деструктор
 */
awh::server::Core::~Core() noexcept {
	// Выполняем остановку сервера
	this->stop();
}
