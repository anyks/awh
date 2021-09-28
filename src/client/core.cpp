/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <client/core.hpp>

/**
 * read Метод чтения данных с сокета сервера
 * @param bev буфер события
 * @param ctx передаваемый контекст
 */
void awh::CoreClient::read(struct bufferevent * bev, void * ctx) noexcept {
	// Если подключение не передано
	if((bev != nullptr) && (ctx != nullptr)){
		// Получаем объект подключения
		wrc_t * wrk = reinterpret_cast <wrc_t *> (ctx);
		// Если подключение производится через, прокси-сервер
		if(wrk->isProxy()){
			// Если функция обратного вызова для вывода записи существует
			if(wrk->readProxyFn != nullptr){
				// Получаем объект ядра системы
				core_t * core = const_cast <core_t *> (wrk->core);
				// Считываем бинарные данные запроса из буфер
				const size_t size = bufferevent_read(bev, (void *) wrk->buffer, BUFFER_CHUNK);
				// Выводим функцию обратного вызова
				wrk->readProxyFn(wrk->buffer, size, wrk->wid, const_cast <core_t *> (wrk->core), wrk->context);
				// Заполняем нулями буфер полученных данных
				memset((void *) wrk->buffer, 0, BUFFER_CHUNK);
			}
		// Если прокси-сервер не используется
		} else if(wrk->readFn != nullptr) {
			// Считываем бинарные данные запроса из буфер
			const size_t size = bufferevent_read(wrk->bev, (void *) wrk->buffer, BUFFER_CHUNK);
			// Выводим функцию обратного вызова
			wrk->readFn(wrk->buffer, size, wrk->wid, const_cast <core_t *> (wrk->core), wrk->context);
			// Заполняем нулями буфер полученных данных
			memset((void *) wrk->buffer, 0, BUFFER_CHUNK);
		}
	}
}
/**
 * write Метод записи данных в сокет сервера
 * @param bev буфер события
 * @param ctx передаваемый контекст
 */
void awh::CoreClient::write(struct bufferevent * bev, void * ctx) noexcept {
	// Если подключение не передано
	if((bev != nullptr) && (ctx != nullptr)){
		// Получаем объект подключения
		wrc_t * wrk = reinterpret_cast <wrc_t *> (ctx);
		// Получаем буферы исходящих данных
		struct evbuffer * output = bufferevent_get_output(bev);
		// Получаем размер исходящих данных
		size_t size = evbuffer_get_length(output);
		// Если данные существуют
		if(size > 0){
			// Выполняем компенсацию размера полученных данных
			size = (size > BUFFER_CHUNK ? BUFFER_CHUNK : size);
			// Копируем данные из буфера
			evbuffer_copyout(output, (void *) wrk->buffer, size);
			// Если подключение производится через, прокси-сервер
			if(wrk->isProxy()){
				// Если функция обратного вызова для вывода записи существует
				if(wrk->writeProxyFn != nullptr)
					// Выводим функцию обратного вызова
					wrk->writeProxyFn(wrk->buffer, size, wrk->wid, const_cast <core_t *> (wrk->core), wrk->context);
			// Если прокси-сервер не используется
			} else if(wrk->writeFn != nullptr)
				// Выводим функцию обратного вызова
				wrk->writeFn(wrk->buffer, size, wrk->wid, const_cast <core_t *> (wrk->core), wrk->context);
			// Заполняем нулями буфер полученных данных
			memset((void *) wrk->buffer, 0, BUFFER_CHUNK);
			// Удаляем данные из буфера
			// evbuffer_drain(output, size);
		}
	}
}
/**
 * tuning Метод тюннинга буфера событий
 * @param bev буфер события
 * @param ctx передаваемый контекст
 */
void awh::CoreClient::tuning(struct bufferevent * bev, void * ctx) noexcept {
	// Если подключение не передано
	if((bev != nullptr) && (ctx != nullptr)){
		// Получаем объект подключения
		wrc_t * wrk = reinterpret_cast <wrc_t *> (ctx);
		// Устанавливаем коллбеки
		bufferevent_setcb(wrk->bev, &read, &write, &event, wrk);
		// Очищаем буферы событий при завершении работы
		bufferevent_flush(wrk->bev, EV_READ | EV_WRITE, BEV_FINISHED);
		// Если флаг ожидания входящих сообщений, активирован
		if(wrk->wait){
			// Устанавливаем таймаут ожидания поступления данных
			struct timeval readTimeout = {wrk->timeRead, 0};
			// Устанавливаем таймаут ожидания записи данных
			struct timeval writeTimeout = {wrk->timeWrite, 0};
			// Устанавливаем таймаут получения данных
			bufferevent_set_timeouts(wrk->bev, &readTimeout, &writeTimeout);
		}
		/**
		 * Водяной знак на N байт (чтобы считывать данные когда они действительно приходят)
		 */
		// Устанавливаем размер считываемых данных
		bufferevent_setwatermark(wrk->bev, EV_READ, 0, wrk->byteRead);
		// Устанавливаем размер записываемых данных
		bufferevent_setwatermark(wrk->bev, EV_WRITE, 0, wrk->byteWrite);
		// Активируем буферы событий на чтение и запись
		bufferevent_enable(wrk->bev, EV_READ | EV_WRITE);
	}
}
/**
 * event Метод обработка входящих событий с сервера
 * @param bev    буфер события
 * @param events произошедшее событие
 * @param ctx    передаваемый контекст
 */
void awh::CoreClient::event(struct bufferevent * bev, const short events, void * ctx) noexcept {
	// Если подключение не передано
	if((ctx != nullptr) && (bev != nullptr)){
		// Получаем объект подключения
		wrc_t * wrk = reinterpret_cast <wrc_t *> (ctx);
		// Если фреймворк получен
		if(wrk->core->fmk != nullptr){
			// Получаем URL параметры запроса
			const uri_t::url_t & url = (wrk->isProxy() ? wrk->proxy.url : wrk->url);
			// Получаем хост сервера
			const string & host = (!url.ip.empty() ? url.ip : url.domain);
			// Если подключение удачное
			if(events & BEV_EVENT_CONNECTED){
				// Сбрасываем количество попыток подключений
				wrk->attempts.first = 0;
				// Выводим в лог сообщение
				wrk->core->log->print("connect client to server [%s:%d]", log_t::flag_t::INFO, host.c_str(), url.port);
				// Если подключение производится через, прокси-сервер
				if(wrk->isProxy()){
					// Выполняем функцию обратного вызова для прокси-сервера
					if(wrk->openProxyFn != nullptr) wrk->openProxyFn(wrk->wid, const_cast <core_t *> (wrk->core), wrk->context);
				// Выполняем функцию обратного вызова
				} else if(wrk->openFn != nullptr) wrk->openFn(wrk->wid, const_cast <core_t *> (wrk->core), wrk->context);
			// Если это ошибка или завершение работы
			} else if(events & (BEV_EVENT_ERROR | BEV_EVENT_EOF | BEV_EVENT_TIMEOUT)) {
				// Если это ошибка
				if(events & BEV_EVENT_ERROR)
					// Выводим в лог сообщение
					wrk->core->log->print("closing server [%s:%d] error: %s", log_t::flag_t::WARNING, host.c_str(), url.port, evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
				// Если - это таймаут, выводим сообщение в лог
				else if(events & BEV_EVENT_TIMEOUT) wrk->core->log->print("timeout server [%s:%d]", log_t::flag_t::WARNING, host.c_str(), url.port);
				// Если нужно выполнить автоматическое переподключение
				if(wrk->alive && (wrk->attempts.first <= wrk->attempts.second)){
					// Выполняем отключение
					const_cast <core_t *> (wrk->core)->close(wrk);
					// Увеличиваем колпичество попыток
					wrk->attempts.first++;
					// Выдерживаем паузу в 3 секунды
					const_cast <core_t *> (wrk->core)->delay(3);
					// Выполняем новое подключение
					const_cast <core_t *> (wrk->core)->connect(wrk);
				// Если автоматическое подключение выполнять не нужно
				} else const_cast <core_t *> (wrk->core)->close(wrk);
			}
		}
	}
}
/**
 * connect Метод создания подключения к удаленному серверу
 * @param worker воркер для подключения
 * @return       результат подключения
 */
bool awh::CoreClient::connect(const worker_t * worker) noexcept {
	// Результат работы функции
	bool result = false;
	// Если объект фреймворка существует
	if((this->fmk != nullptr) && (worker != nullptr)){
		// Размер структуры подключения
		socklen_t size = 0;
		// Объект подключения
		struct sockaddr * sin = nullptr;
		// Получаем объект воркера
		wrc_t * wrk = (wrc_t *) const_cast <worker_t *> (worker);
		// Получаем URL параметры запроса
		const uri_t::url_t & url = (wrk->isProxy() ? wrk->proxy.url : wrk->url);
		// Получаем сокет для подключения к серверу
		auto socket = this->socket(url.ip, url.port, url.family);
		// Если сокет создан удачно
		if(socket.fd > -1){
			// Устанавливаем сокет подключения
			wrk->fd = socket.fd;
			// Выполняем получение контекста сертификата
			wrk->ssl = this->ssl->init(url);
			// Если SSL клиент разрешен
			if(wrk->ssl.mode){
				// Создаем буфер событий для сервера зашифрованного подключения
				wrk->bev = bufferevent_openssl_socket_new(this->base, wrk->fd, wrk->ssl.ssl, BUFFEREVENT_SSL_CONNECTING, BEV_OPT_THREADSAFE);
				// this->bev = bufferevent_openssl_socket_new(this->base, wrk->fd, wrk->ssl.ssl, BUFFEREVENT_SSL_CONNECTING, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
				// Разрешаем непредвиденное грязное завершение работы
				bufferevent_openssl_set_allow_dirty_shutdown(wrk->bev, 1);
			// Создаем буфер событий для сервера
			} else wrk->bev = bufferevent_socket_new(this->base, wrk->fd, BEV_OPT_THREADSAFE);
			// } else wrk->bev = bufferevent_socket_new(this->base, wrk->fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
			// Если буфер событий создан
			if(wrk->bev != nullptr){
				// Выполняем тюннинг буфера событий
				tuning(wrk->bev, wrk);
				// Определяем тип подключения
				switch(url.family){
					// Для протокола IPv4
					case AF_INET: {
						// Запоминаем размер структуры
						size = sizeof(socket.server);
						// Запоминаем полученную структуру
						sin = reinterpret_cast <struct sockaddr *> (&socket.server);
					} break;
					// Для протокола IPv6
					case AF_INET6: {
						// Запоминаем размер структуры
						size = sizeof(socket.server6);
						// Запоминаем полученную структуру
						sin = reinterpret_cast <struct sockaddr *> (&socket.server6);
					} break;
				}
				// Выполняем подключение к удаленному серверу, если подключение не выполненно то сообщаем об этом
				if(bufferevent_socket_connect(wrk->bev, sin, size) < 0){
					// Выводим в лог сообщение
					this->log->print("connecting to host = %s, port = %u", log_t::flag_t::CRITICAL, url.ip.c_str(), url.port);
					// Если нужно выполнить автоматическое переподключение
					if(wrk->alive && (wrk->attempts.first <= wrk->attempts.second)){
						// Выполняем отключение
						this->close(wrk);
						// Увеличиваем колпичество попыток
						wrk->attempts.first++;
						// Выдерживаем паузу в 3 секунды
						this->delay(3);
						// Выполняем новое подключение
						return this->connect(wrk);
					// Если автоматическое подключение выполнять не нужно
					} else {
						// Иначе останавливаем работу
						this->close(wrk);
						// Просто выходим
						return result;
					}
				}
				// Выводим в лог сообщение
				this->log->print("create good connect to host = %s [%s:%d], socket = %d", log_t::flag_t::INFO, url.domain.c_str(), url.ip.c_str(), url.port, wrk->fd);
				// Сообщаем что все удачно
				result = true;
			// Выполняем новую попытку подключения
			} else {
				// Выводим в лог сообщение
				this->log->print("connecting to host = %s, port = %u", log_t::flag_t::CRITICAL, url.ip.c_str(), url.port);
				// Если нужно выполнить автоматическое переподключение
				if(wrk->alive && (wrk->attempts.first <= wrk->attempts.second)){
					// Выполняем отключение
					this->close(wrk);
					// Увеличиваем колпичество попыток
					wrk->attempts.first++;
					// Выдерживаем паузу в 3 секунды
					this->delay(3);
					// Выполняем новое подключение
					return this->connect(wrk);
				// Иначе останавливаем работу
				} else this->close(wrk);
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * close Метод закрытия подключения воркера
 * @param worker воркер для закрытия подключения
 */
void awh::CoreClient::close(const worker_t * worker) noexcept {
	// Если воркер получен
	if(worker != nullptr){
		// Получаем объект воркера
		wrc_t * wrk = (wrc_t *) const_cast <worker_t *> (worker);
		// Если сокет существует
		if(wrk->fd > -1){
			// Если событие сервера существует
			if(wrk->bev != nullptr){
				// Запрещаем чтение запись данных серверу
				bufferevent_disable(wrk->bev, EV_WRITE | EV_READ);
				// Если - это Windows
				#if defined(_WIN32) || defined(_WIN64)
					// Отключаем подключение для сокета
					if(wrk->fd > 0) shutdown(wrk->fd, SD_BOTH);
				// Если - это Unix
				#else
					// Отключаем подключение для сокета
					if(wrk->fd > 0) shutdown(wrk->fd, SHUT_RDWR);
				#endif
				// Закрываем подключение
				if(wrk->fd > 0) evutil_closesocket(wrk->fd);
				// Удаляем буфер события
				bufferevent_free(wrk->bev);
				// Устанавливаем что событие удалено
				wrk->bev = nullptr;
			}
			// Выполняем удаление контекста SSL
			this->ssl->clear(wrk->ssl);
			// Выполняем сброс файлового дескриптора
			wrk->fd = -1;
			// Выводим сообщение об ошибке
			this->log->print("%s", log_t::flag_t::INFO, "disconnected from the server");
			// Выводим функцию обратного вызова
			if(wrk->closeFn != nullptr) wrk->closeFn(wrk->wid, this, wrk->context);
		}
	}
}
/**
 * stop Метод остановки клиента
 */
void awh::CoreClient::stop() noexcept {
	// Если система уже запущена
	if(this->mode){
		// Запрещаем работу WebSocket
		this->mode = false;
		// Выполняем отключение всех воркеров
		this->closeAll();
		// Выполняем удаление всех воркеров
		this->removeAll();
		// Завершаем работу базы событий
		event_base_loopbreak(this->base);
		// Если - это Windows
		#if defined(_WIN32) || defined(_WIN64)
			// Очищаем сетевой контекст
			this->winSocketClean();
		#endif
	}
}
/**
 * start Метод запуска клиента
 */
void awh::CoreClient::start() noexcept {
	// Если система ещё не запущена
	if(!this->mode && !this->locker){
		try {
			// Разрешаем работу WebSocket
			this->mode = true;
			// Создаем новую базу
			this->base = event_base_new();
			// Резолвер IPv4, создаём резолвер
			this->dns4 = new dns_t(this->fmk, this->log, this->nwk, this->base, this->net.v4.second);
			// Резолвер IPv6, создаём резолвер
			this->dns6 = new dns_t(this->fmk, this->log, this->nwk, this->base, this->net.v6.second);
			// Если список воркеров существует
			if(!this->workers.empty()){
				// Переходим по всему списку воркеров
				for(auto & worker : this->workers){
					// Если функция обратного вызова установлена
					if(worker.second->startFn != nullptr)
						// Выполняем функцию обратного вызова
						worker.second->startFn(worker.first, this, worker.second->context);
				}
			}
			// Если функция обратного вызова установлена, выполняем
			if(this->startFn != nullptr) this->startFn(this->base, this, this->ctx);
			// Выводим в консоль информацию
			this->log->print("[+] start service: pid = %u", log_t::flag_t::INFO, getpid());
			// Запускаем работу базы событий
			event_base_loop(this->base, EVLOOP_NO_EXIT_ON_EMPTY);
			// Удаляем dns IPv4 резолвер
			delete this->dns4;
			// Удаляем dns IPv6 резолвер
			delete this->dns6;
			// Зануляем DNS IPv4 объект
			this->dns4 = nullptr;
			// Зануляем DNS IPv6 объект
			this->dns6 = nullptr;
			// Удаляем объект базы событий
			event_base_free(this->base);
			// Очищаем все глобальные переменные
			libevent_global_shutdown();
			// Если функция обратного вызова установлена, выполняем
			if(this->stopFn != nullptr) this->stopFn(this, this->ctx);
			// Выводим в консоль информацию
			this->log->print("[-] stop service: pid = %u", log_t::flag_t::INFO, getpid());
		// Если происходит ошибка то игнорируем её
		} catch(const bad_alloc&) {
			// Выводим сообщение об ошибке
			this->log->print("%s", log_t::flag_t::CRITICAL, "memory could not be allocated");
		}
	}
}
/**
 * add Метод добавления воркера в биндинг
 * @param worker воркер для добавления
 * @return       идентификатор воркера в биндинге
 */
size_t awh::CoreClient::add(const worker_t * worker) noexcept {
	// Результат работы функции
	size_t result = 0;
	// Выполняем блокировку потока
	this->bloking.lock();
	// Если воркер передан и URL адрес существует
	if(worker != nullptr){
		// Получаем объект воркера
		wrc_t * wrk = (wrc_t *) const_cast <worker_t *> (worker);
		// Получаем идентификатор воркера
		result = (1 + this->workers.size());
		// Устанавливаем родительский объект
		wrk->core = this;
		// Устанавливаем идентификатор воркера
		wrk->wid = result;
		// Добавляем воркер в список
		this->workers.emplace(result, wrk);
	}
	// Выполняем разблокировку потока
	this->bloking.unlock();
	// Выводим результат
	return result;
}
/**
 * open Метод открытия подключения воркером
 * @param wid идентификатор воркера
 */
void awh::CoreClient::open(const size_t wid) noexcept {
	// Если идентификатор воркера передан
	if((wid > 0) && (this->dns4 != nullptr) && (this->dns6 != nullptr)){
		// Выполняем поиск воркера
		auto it = this->workers.find(wid);
		// Если воркер найден
		if((it != this->workers.end()) && !it->second->url.empty()){
			// Получаем объект воркера
			wrc_t * wrk = (wrc_t *) const_cast <worker_t *> (it->second);
			// Получаем URL параметры запроса
			const uri_t::url_t & url = (wrk->isProxy() ? wrk->proxy.url : wrk->url);
			/**
			 * runFn Функция выполнения запуска системы
			 * @param ip полученный адрес сервера резолвером
			 */
			auto runFn = [wrk, this](const string & ip) noexcept {
				// Если IP адрес получен
				if(!ip.empty()){
					// Если прокси-сервер активен
					if(wrk->isProxy())
						// Запоминаем полученный IP адрес для прокси-сервера
						wrk->proxy.url.ip = ip;
					// Запоминаем полученный IP адрес
					else wrk->url.ip = ip;
					// Если подключение выполнено
					if(this->connect(wrk)) return;
					// Сообщаем, что подключение не удалось и выводим сообщение
					else this->log->print("broken connect to host %s", log_t::flag_t::CRITICAL, ip.c_str());
					// Если файловый дескриптор очищен, зануляем его
					if(wrk->fd == -1) wrk->fd = 0;
				}
				// Отключаем воркер
				this->close(wrk);
			};
			// Если IP адрес не получен
			if(url.ip.empty() && !url.domain.empty())
				// Определяем тип подключения
				switch(url.family){
					// Резолвер IPv4, создаём резолвер
					case AF_INET: this->dns4->resolve(url.domain, url.family, runFn); break;
					// Резолвер IPv6, создаём резолвер
					case AF_INET6: this->dns6->resolve(url.domain, url.family, runFn); break;
				}
			// Выполняем запуск системы
			else if(!url.ip.empty()) runFn(url.ip);
			// Иначе завершаем работу
			else this->close(wrk);
		}
	}
}
/**
 * switchProxy Метод переключения с прокси-сервера
 * @param wid идентификатор воркера
 */
void awh::CoreClient::switchProxy(const size_t wid) noexcept {
	// Если идентификатор воркера передан
	if(wid > 0){
		// Выполняем поиск воркера
		auto it = this->workers.find(wid);
		// Если воркер найден
		if(it != this->workers.end()){
			// Получаем объект воркера
			wrc_t * wrk = (wrc_t *) const_cast <worker_t *> (it->second);
			// Выполняем переключение типа подключения
			wrk->switchConnect();
			// Выполняем получение контекста сертификата
			wrk->ssl = this->ssl->init(wrk->url);
			// Если SSL клиент разрешен
			if(wrk->ssl.mode){
				// Выполняем переход на защищённое подключение
				struct bufferevent * bev = bufferevent_openssl_filter_new(this->base, wrk->bev, wrk->ssl.ssl, BUFFEREVENT_SSL_CONNECTING, BEV_OPT_THREADSAFE);
				// Если буфер событий создан
				if(bev != nullptr){
					// Устанавливаем новый буфер событий
					wrk->bev = bev;
					// Разрешаем непредвиденное грязное завершение работы
					bufferevent_openssl_set_allow_dirty_shutdown(wrk->bev, 1);
					// Выполняем тюннинг буфера событий
					tuning(wrk->bev, wrk);
				}
			// Выполняем функцию обратного вызова
			} else if(wrk->openFn != nullptr) wrk->openFn(wrk->wid, this, wrk->context);
		}
	}
}
/**
 * write Метод записи буфера данных воркером
 * @param buffer буфер для записи данных
 * @param size   размер записываемых данных
 * @param wid    идентификатор воркера
 */
void awh::CoreClient::write(const char * buffer, const size_t size, const size_t wid) noexcept {
	// Если данные переданы
	if((buffer != nullptr) && (size > 0) && (wid > 0)){
		// Выполняем поиск воркера
		auto it = this->workers.find(wid);
		// Если воркер найден
		if(it != this->workers.end()){
			// Получаем объект воркера
			wrc_t * wrk = (wrc_t *) const_cast <worker_t *> (it->second);
			// Получаем количество байт для детекции
			const size_t bytes = (wrk->byteWrite > 0 ? wrk->byteWrite : size);
			// Активируем разрешение на запись и чтение
			bufferevent_enable(wrk->bev, EV_WRITE | EV_READ);
			// Устанавливаем размер записываемых данных
			bufferevent_setwatermark(wrk->bev, EV_WRITE, bytes, bytes);
			// Отправляем серверу сообщение
			bufferevent_write(wrk->bev, buffer, size);
		}
	}
}
/**
 * ~CoreClient Деструктор
 */
awh::CoreClient::~CoreClient() noexcept {
	// Выполняем остановку сервера
	this->stop();
}
