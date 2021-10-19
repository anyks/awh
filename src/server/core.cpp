/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <server/core.hpp>

/**
 * read Метод чтения данных с сокета сервера
 * @param bev буфер события
 * @param ctx передаваемый контекст
 */
void awh::CoreServer::read(struct bufferevent * bev, void * ctx) noexcept {
	// Если подключение не передано
	if((bev != nullptr) && (ctx != nullptr)){
		// Получаем объект подключения
		worker_t::adj_t * adj = reinterpret_cast <worker_t::adj_t *> (ctx);
		// Получаем объект подключения
		workSrv_t * wrk = (workSrv_t *) const_cast <worker_t *> (adj->parent);
		// Если подключение ещё существует
		if(wrk->core->adjutants.count(adj->aid) > 0){
			// Если функция обратного вызова установлена
			if(wrk->readFn != nullptr){
				// Заполняем нулями буфер полученных данных
				memset((void *) adj->buffer, 0, BUFFER_CHUNK);
				// Считываем бинарные данные запроса из буфер
				const size_t size = bufferevent_read(bev, (void *) adj->buffer, BUFFER_CHUNK);
				// Выводим функцию обратного вызова
				wrk->readFn(adj->buffer, size, adj->aid, wrk->wid, const_cast <core_t *> (wrk->core), wrk->ctx);
			}
		}
	}
}
/**
 * write Метод записи данных в сокет сервера
 * @param bev буфер события
 * @param ctx передаваемый контекст
 */
void awh::CoreServer::write(struct bufferevent * bev, void * ctx) noexcept {
	// Если подключение не передано
	if((bev != nullptr) && (ctx != nullptr)){
		// Получаем объект подключения
		worker_t::adj_t * adj = reinterpret_cast <worker_t::adj_t *> (ctx);
		// Получаем объект подключения
		workSrv_t * wrk = (workSrv_t *) const_cast <worker_t *> (adj->parent);
		// Если подключение ещё существует
		if(wrk->core->adjutants.count(adj->aid) > 0){
			// Получаем буферы исходящих данных
			struct evbuffer * output = bufferevent_get_output(bev);
			// Получаем размер исходящих данных
			const size_t size = evbuffer_get_length(output);
			// Если данные существуют
			if(size > 0){
				// Если функция обратного вызова установлена
				if(wrk->writeFn != nullptr){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Создаём буфер входящих данных
						char * buffer = new char[size];
						// Копируем в буфер полученные данные
						evbuffer_copyout(output, buffer, size);
						// Выводим функцию обратного вызова
						wrk->writeFn(buffer, size, adj->aid, wrk->wid, const_cast <core_t *> (wrk->core), wrk->ctx);
						// Выполняем удаление буфера
						delete [] buffer;
					// Если возникает ошибка
					} catch(const bad_alloc &) {
						// Выводим пустое сообщение
						wrk->writeFn(nullptr, size, adj->aid, wrk->wid, const_cast <core_t *> (wrk->core), wrk->ctx);
					}
				}
				// Удаляем данные из буфера
				evbuffer_drain(output, size);
			}
		}
	}
}
/**
 * event Метод обработка входящих событий с сервера
 * @param bev    буфер события
 * @param events произошедшее событие
 * @param ctx    передаваемый контекст
 */
void awh::CoreServer::event(struct bufferevent * bev, const short events, void * ctx) noexcept {
	// Если подключение не передано
	if((ctx != nullptr) && (bev != nullptr)){
		// Получаем объект подключения
		worker_t::adj_t * adj = reinterpret_cast <worker_t::adj_t *> (ctx);
		// Получаем объект подключения
		workSrv_t * wrk = (workSrv_t *) const_cast <worker_t *> (adj->parent);
		// Если подключение ещё существует
		if((wrk->core->adjutants.count(adj->aid) > 0) && (adj->fmk != nullptr)){
			// Получаем файловый дескриптор
			evutil_socket_t fd = bufferevent_getfd(bev);
			// Если это ошибка или завершение работы
			if(events & (BEV_EVENT_ERROR | BEV_EVENT_EOF | BEV_EVENT_TIMEOUT)) {
				// Если это ошибка
				if(events & BEV_EVENT_ERROR)
					// Выводим в лог сообщение
					adj->log->print("closing client, host = %s, mac = %s, socket = %d %s", log_t::flag_t::WARNING, adj->ip.c_str(), adj->mac.c_str(), fd, evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
				// Если - это таймаут, выводим сообщение в лог
				else if(events & BEV_EVENT_TIMEOUT) adj->log->print("timeout client, host = %s, mac = %s, socket = %d", log_t::flag_t::WARNING, adj->ip.c_str(), adj->mac.c_str(), fd);
				// Запрещаем чтение запись данных серверу
				bufferevent_disable(bev, EV_WRITE | EV_READ);
				// Выполняем отключение от сервера
				const_cast <core_t *> (wrk->core)->close(adj->aid);
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
void awh::CoreServer::accept(const evutil_socket_t fd, const short event, void * ctx) noexcept {
	// Если прокси существует
	if(ctx != nullptr){
		// IP и MAC адрес подключения
		string ip = "", mac = "";
		// Сокет подключившегося клиента
		evutil_socket_t socket = -1;
		// Получаем объект воркера
		workSrv_t * wrk = reinterpret_cast <workSrv_t *> (ctx);
		// Получаем объект подключения
		coreSrv_t * core = (coreSrv_t *) const_cast <core_t *> (wrk->core);
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
				ip = sockets_t::ip(AF_INET, &client);
				// Получаем данные мак адреса клиента
				mac = sockets_t::mac(reinterpret_cast <struct sockaddr *> (&client));
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
				ip = sockets_t::ip(AF_INET6, &client);
				// Получаем данные мак адреса клиента
				mac = sockets_t::mac(reinterpret_cast <struct sockaddr *> (&client));
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
		// Если - это Unix
		#if !defined(_WIN32) && !defined(_WIN64)
			// Устанавливаем разрешение на повторное использование сокета
			sockets_t::reuseable(socket, core->log);
			// Отключаем сигнал записи в оборванное подключение
			sockets_t::noSigpipe(socket, core->log);
			// Отключаем алгоритм Нейгла для сервера и клиента
			sockets_t::tcpNodelay(socket, core->log);
			// Устанавливаем неблокирующий режим для сокета
			sockets_t::nonBlocking(socket, core->log);
		// Если - это Windows
		#else
			// Переводим сокет в блокирующий режим
			// sockets_t::blocking(socket);
			evutil_make_socket_nonblocking(socket);
			// evutil_make_socket_closeonexec(socket);
			evutil_make_listen_socket_reuseable(socket);
		#endif
		// Создаём бъект адъютанта
		worker_t::adj_t adj = worker_t::adj_t(wrk, core->fmk, core->log);
		// Устанавливаем первоначальное значение
		u_int mode = BEV_OPT_THREADSAFE;
		// Если нужно использовать отложенные вызовы событий сокета
		if(core->defer) mode = (mode | BEV_OPT_DEFER_CALLBACKS);
		// Создаем буфер событий для подключившегося клиента
		adj.bev = bufferevent_socket_new(core->base, socket, mode);
		// Если буфер событий создан
		if(adj.bev != nullptr){
			// Запоминаем IP адрес
			adj.ip = move(ip);
			// Запоминаем MAC адрес
			adj.mac = move(mac);
			// Устанавливаем идентификатор адъютанта
			adj.aid = core->fmk->unixTimestamp();
			// Добавляем созданного адъютанта в список адъютантов
			auto ret = wrk->adjutants.emplace(adj.aid, move(adj));
			// Добавляем адъютанта в список подключений
			core->adjutants.emplace(ret.first->second.aid, &ret.first->second);
			// Выполняем тюннинг буфера событий
			core->tuning(ret.first->second.aid);
			// Выполняем функцию обратного вызова
			if(wrk->connectFn != nullptr) wrk->connectFn(adj.aid, wrk->wid, core, wrk->ctx);
			// Если флаг ожидания входящих сообщений, активирован
			if(wrk->wait){
				// Устанавливаем таймаут ожидания поступления данных
				struct timeval read = {adj.timeRead, 0};
				// Устанавливаем таймаут ожидания записи данных
				struct timeval write = {adj.timeWrite, 0};
				// Устанавливаем таймаут на отправку/получение данных
				bufferevent_set_timeouts(
					adj.bev,
					(adj.timeRead > 0 ? &read : nullptr),
					(adj.timeWrite > 0 ? &write : nullptr)
				);
			}
			// Выводим в консоль информацию
			if(!core->noinfo) core->log->print("client connect to server, host = %s, mac = %s, socket = %d", log_t::flag_t::INFO, ret.first->second.ip.c_str(), ret.first->second.mac.c_str(), socket);
		// Выводим в лог сообщение
		} else core->log->print("client connect to server, host = %s, mac = %s, socket = %d", log_t::flag_t::CRITICAL, ip.c_str(), mac.c_str(), socket);
	}
}
/**
 * tuning Метод тюннинга буфера событий
 * @param aid идентификатор адъютанта
 */
void awh::CoreServer::tuning(const size_t aid) noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Получаем объект воркера
		workSrv_t * wrk = (workSrv_t *) const_cast <worker_t *> (it->second->parent);
		// Устанавливаем время ожидания поступления данных
		const_cast <worker_t::adj_t *> (it->second)->timeRead = wrk->timeRead;
		// Устанавливаем время ожидания записи данных
		const_cast <worker_t::adj_t *> (it->second)->timeWrite = wrk->timeWrite;
		// Устанавливаем размер детектируемых байт на чтение
		const_cast <worker_t::adj_t *> (it->second)->markRead = wrk->markRead;
		// Устанавливаем размер детектируемых байт на запись
		const_cast <worker_t::adj_t *> (it->second)->markWrite = wrk->markWrite;
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
void awh::CoreServer::connect(const size_t wid) noexcept {
	// Блокируем переданный идентификатор
	(void) wid;
}
/**
 * close Метод закрытия сокета
 * @param fd файловый дескриптор (сокет) для закрытия
 */
void awh::CoreServer::close(const evutil_socket_t fd) noexcept {
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
 * removeAll Метод удаления всех воркеров
 */
void awh::CoreServer::removeAll() noexcept {
	// Переходим по всему списку подключений
	for(auto it = this->workers.begin(); it != this->workers.end();){
		// Получаем объект воркера
		workSrv_t * wrk = (workSrv_t *) const_cast <worker_t *> (it->second);
		// Если в воркере есть подключённые клиенты
		if(!wrk->adjutants.empty()){
			// Переходим по всему списку адъютанта
			for(auto jt = wrk->adjutants.begin(); jt != wrk->adjutants.end();){
				// Выполняем очистку буфера событий
				this->clean(jt->second.bev);
				// Выводим функцию обратного вызова
				if(wrk->disconnectFn != nullptr)
					// Выполняем функцию обратного вызова
					wrk->disconnectFn(jt->first, it->first, this, wrk->ctx);
				// Удаляем адъютанта из списка подключений
				this->adjutants.erase(jt->first);
				// Удаляем адъютанта из списка
				jt = wrk->adjutants.erase(jt);
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
/**
 * remove Метод удаления воркера
 * @param wid идентификатор воркера
 */
void awh::CoreServer::remove(const size_t wid) noexcept {
	// Если идентификатор воркера передан
	if(wid > 0){
		// Выполняем поиск воркера
		auto it = this->workers.find(wid);
		// Если воркер найден, устанавливаем максимальное количество одновременных подключений
		if(it != this->workers.end()){
			// Получаем объект воркера
			workSrv_t * wrk = (workSrv_t *) const_cast <worker_t *> (it->second);
			// Если в воркере есть подключённые клиенты
			if(!wrk->adjutants.empty()){
				// Переходим по всему списку адъютанта
				for(auto jt = wrk->adjutants.begin(); jt != wrk->adjutants.end();){
					// Выполняем очистку буфера событий
					this->clean(jt->second.bev);
					// Выводим функцию обратного вызова
					if(wrk->disconnectFn != nullptr)
						// Выполняем функцию обратного вызова
						wrk->disconnectFn(jt->first, it->first, this, wrk->ctx);
					// Удаляем адъютанта из списка подключений
					this->adjutants.erase(jt->first);
					// Удаляем адъютанта из списка
					jt = wrk->adjutants.erase(jt);
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
 * run Метод запуска сервера воркером
 * @param wid идентификатор воркера
 */
void awh::CoreServer::run(const size_t wid) noexcept {
	// Если идентификатор воркера передан
	if(wid > 0){
		// Выполняем поиск воркера
		auto it = this->workers.find(wid);
		// Если воркер найден, устанавливаем максимальное количество одновременных подключений
		if(it != this->workers.end()){
			// Получаем объект подключения
			workSrv_t * wrk = (workSrv_t *) const_cast <worker_t *> (it->second);
			/**
			 * runFn Функция выполнения запуска системы
			 * @param ip полученный адрес сервера резолвером
			 */
			auto runFn = [wrk, this](const string & ip) noexcept {
				// Если IP адрес получен
				if(!ip.empty()){
					// sudo lsof -i -P | grep 1080
					// Обновляем хост сервера
					wrk->host = ip;
					// Получаем сокет сервера
					wrk->fd = this->socket(wrk->host, wrk->port, this->net.family).fd;
					// Выполняем чтение сокета
					if(::listen(wrk->fd, wrk->total) < 0){
						// Выводим в консоль информацию
						if(!this->noinfo) this->log->print("listen service: pid = %u", log_t::flag_t::CRITICAL, getpid());
						// Останавливаем работу сервера
						this->stop();
						// Выходим
						return;
					}
					// Добавляем событие в базу
					wrk->ev = event_new(this->base, wrk->fd, EV_READ | EV_PERSIST, &accept, wrk);
					// Активируем событие
					event_add(wrk->ev, nullptr);
					// Выводим сообщение об активации
					if(!this->noinfo) this->log->print("run server [%s:%u]", log_t::flag_t::INFO, wrk->host.c_str(), wrk->port);
				// Если IP адрес сервера не получен
				} else {
					// Выводим в консоль информацию
					this->log->print("broken host server %s", log_t::flag_t::CRITICAL, wrk->host.c_str());
					// Останавливаем работу сервера
					this->stop();
				}
			};
			// Определяем тип подключения
			switch(this->net.family){
				// Резолвер IPv4, создаём резолвер
				case AF_INET: this->dns4.resolve(wrk->host, AF_INET, runFn); break;
				// Резолвер IPv6, создаём резолвер
				case AF_INET6: this->dns6.resolve(wrk->host, AF_INET6, runFn); break;
			}
		}
	}
}
/**
 * open Метод открытия подключения воркером
 * @param wid идентификатор воркера
 */
void awh::CoreServer::open(const size_t wid) noexcept {
	// Блокируем переданный идентификатор
	(void) wid;
}
/**
 * close Метод закрытия подключения воркера
 * @param aid идентификатор адъютанта
 */
void awh::CoreServer::close(const size_t aid) noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Получаем объект воркера
		workSrv_t * wrk = (workSrv_t *) const_cast <worker_t *> (it->second->parent);
		// Если событие сервера существует
		if(it->second->bev != nullptr){
			// Выполняем очистку буфера событий
			this->clean(it->second->bev);
			// Устанавливаем что событие удалено
			const_cast <worker_t::adj_t *> (it->second)->bev = nullptr;
		}
		// Удаляем адъютанта из списка адъютантов
		wrk->adjutants.erase(aid);
		// Удаляем адъютанта из списка подключений
		this->adjutants.erase(aid);
		// Выводим сообщение об ошибке
		if(!wrk->core->noinfo) this->log->print("%s", log_t::flag_t::INFO, "disconnect client from server");
		// Выводим функцию обратного вызова
		if(wrk->disconnectFn != nullptr) wrk->disconnectFn(aid, wrk->wid, this, wrk->ctx);
	}
}
/**
 * setIpV6only Метод установки флага использования только сети IPv6
 * @param mode флаг для установки
 */
void awh::CoreServer::setIpV6only(const bool mode) noexcept {
	// Устанавливаем флаг использования только сети IPv6
	this->ipV6only = mode;
}
/**
 * setTotal Метод установки максимального количества одновременных подключений
 * @param wid   идентификатор воркера
 * @param total максимальное количество одновременных подключений
 */
void awh::CoreServer::setTotal(const size_t wid, const u_short total) noexcept {
	// Если идентификатор воркера передан
	if(wid > 0){
		// Выполняем поиск воркера
		auto it = this->workers.find(wid);
		// Если воркер найден, устанавливаем максимальное количество одновременных подключений
		if(it != this->workers.end())
			// Устанавливаем максимальное количество одновременных подключений
			((workSrv_t *) const_cast <worker_t *> (it->second))->total = total;
	}
}
/**
 * init Метод инициализации сервера
 * @param wid  идентификатор воркера
 * @param port порт сервера
 * @param host хост сервера
 */
void awh::CoreServer::init(const size_t wid, const u_int port, const string & host) noexcept {
	// Если идентификатор воркера передан
	if(wid > 0){
		// Выполняем поиск воркера
		auto it = this->workers.find(wid);
		// Если воркер найден, устанавливаем максимальное количество одновременных подключений
		if(it != this->workers.end()){
			// Получаем объект подключения
			workSrv_t * wrk = (workSrv_t *) const_cast <worker_t *> (it->second);
			// Если порт передан, устанавливаем
			if(port > 0) wrk->port = port;
			// Если хост передан, устанавливаем
			if(!host.empty()) wrk->host = host;
			// Иначе получаем IP адрес сервера автоматически
			else wrk->host = sockets_t::serverIp(this->net.family);
		}
	}
}
/**
 * CoreServer Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::CoreServer::CoreServer(const fmk_t * fmk, const log_t * log) noexcept : core_t(fmk, log) {
	// Устанавливаем тип запускаемого ядра
	this->type = type_t::SERVER;
}
/**
 * ~CoreServer Деструктор
 */
awh::CoreServer::~CoreServer() noexcept {
	// Выполняем остановку сервера
	this->stop();
}
