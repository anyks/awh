/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <core/core.hpp>

// Если - это Windows
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * winSocketInit Метод инициализации WinSock
	 */
	void awh::Core::winSocketInit() const noexcept {
		// Если winSock ещё не инициализирован
		if(!this->winSock){
			// Идентификатор ошибки
			int error = 0;
			// Объект данных запроса
			WSADATA wsaData;
			// Выполняем инициализацию сетевого контекста
			if((error = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0){ // 0x202
				// Сообщаем, что сетевой контекст не поднят
				this->log->print("WSAStartup failed with error: %d", log_t::flag_t::CRITICAL, error);
				// Выходим из приложения
				exit(EXIT_FAILURE);
			}
			// Выполняем проверку версии WinSocket
			if((2 != LOBYTE(wsaData.wVersion)) || (2 != HIBYTE(wsaData.wVersion))){
				// Сообщаем, что версия WinSocket не подходит
				this->log->print("%s", log_t::flag_t::CRITICAL, "WSADATA version is not correct");
				// Очищаем сетевой контекст
				this->winSocketClean();
				// Выходим из приложения
				exit(EXIT_FAILURE);
			}
			// Запоминаем, что winSock уже инициализирован
			this->winSock = true;
		}
	}
	/**
	 * winSocketClean Метод очистки WinSock
	 */
	void awh::Core::winSocketClean() const noexcept {
		// Очищаем сетевой контекст
		WSACleanup();
		// Запоминаем, что winSock не инициализирован
		this->winSock = false;
	}
#endif
/**
 * delay Метод фриза потока на указанное количество секунд
 * @param seconds количество секунд для фриза потока
 */
void awh::Core::delay(const size_t seconds) const noexcept {
	// Если количество секунд передано
	if(seconds){
		// Если операционной системой является Windows
		#if defined(_WIN32) || defined(_WIN64)
			// Выполняем фриз потока
			::Sleep(seconds * 1000);
		// Для всех остальных операционных систем
		#else
			// Выполняем фриз потока
			::sleep(seconds);
		#endif
	}
}
/**
 * read Метод чтения данных с сокета сервера
 * @param bev буфер события
 * @param ctx передаваемый контекст
 */
void awh::Core::read(struct bufferevent * bev, void * ctx) noexcept {
	// Если подключение не передано
	if((bev != nullptr) && (ctx != nullptr)){
		// Получаем объект подключения
		worker_t * wrk = reinterpret_cast <worker_t *> (ctx);
		// Если функция обратного вызова для вывода записи существует
		if((wrk != nullptr) && (wrk->readFn != nullptr)){
			// Считываем бинарные данные запроса из буфер
			const size_t size = bufferevent_read(bev, (void *) wrk->buffer, BUFFER_CHUNK);
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
void awh::Core::write(struct bufferevent * bev, void * ctx) noexcept {
	// Если подключение не передано
	if((bev != nullptr) && (ctx != nullptr)){
		// Получаем объект подключения
		worker_t * wrk = reinterpret_cast <worker_t *> (ctx);
		// Если функция обратного вызова для вывода записи существует
		if(wrk->writeFn != nullptr){
			// Получаем буферы исходящих данных
			struct evbuffer * output = bufferevent_get_output(bev);
			// Получаем размер исходящих данных
			size_t size = evbuffer_get_length(output);
			// Если данные существуют
			if((size > 0) && (wrk != nullptr)){
				// Выполняем компенсацию размера полученных данных
				size = (size > BUFFER_CHUNK ? BUFFER_CHUNK : size);
				// Копируем данные из буфера
				evbuffer_copyout(output, (void *) wrk->buffer, size);
				// Выводим функцию обратного вызова
				wrk->writeFn(wrk->buffer, size, wrk->wid, const_cast <core_t *> (wrk->core), wrk->context);
				// Заполняем нулями буфер полученных данных
				memset((void *) wrk->buffer, 0, BUFFER_CHUNK);
			}
			// Удаляем данные из буфера
			// evbuffer_drain(output, size);
		}
	}
}
/**
 * event Метод обработка входящих событий с сервера
 * @param bev    буфер события
 * @param events произошедшее событие
 * @param ctx    передаваемый контекст
 */
void awh::Core::event(struct bufferevent * bev, const short events, void * ctx) noexcept {
	// Если подключение не передано
	if((ctx != nullptr) && (bev != nullptr)){
		// Получаем объект подключения
		worker_t * wrk = reinterpret_cast <worker_t *> (ctx);
		// Если фреймворк получен
		if(wrk->core->fmk != nullptr){
			// Получаем текущий сокет
			const evutil_socket_t socket = bufferevent_getfd(bev);
			// Если подключение удачное
			if(events & BEV_EVENT_CONNECTED){
				// Сбрасываем количество попыток подключений
				wrk->attempts.first = 0;
				// Выводим в лог сообщение
				wrk->core->log->print("connect client to server [%s:%d]", log_t::flag_t::INFO, wrk->url.ip.c_str(), wrk->url.port);
				// Если функция обратного вызова запуска установлена
				if(wrk->openFn != nullptr) wrk->openFn(wrk->wid, const_cast <core_t *> (wrk->core), wrk->context);
			// Если это ошибка или завершение работы
			} else if(events & (BEV_EVENT_ERROR | BEV_EVENT_EOF | BEV_EVENT_TIMEOUT)) {
				// Если это ошибка
				if(events & BEV_EVENT_ERROR)
					// Выводим в лог сообщение
					wrk->core->log->print("closing server [%s:%d] error: %s", log_t::flag_t::WARNING, wrk->url.ip.c_str(), wrk->url.port, evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
				// Если - это таймаут, выводим сообщение в лог
				else if(events & BEV_EVENT_TIMEOUT) wrk->core->log->print("timeout server [%s:%d]", log_t::flag_t::WARNING, wrk->url.ip.c_str(), wrk->url.port);
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
const bool awh::Core::connect(const worker_t * worker) noexcept {
	// Результат работы функции
	bool result = false;
	// Если объект фреймворка существует
	if((this->fmk != nullptr) && (worker != nullptr)){
		// Размер структуры подключения
		socklen_t size = 0;
		// Объект подключения
		struct sockaddr * sin = nullptr;
		// Получаем объект воркера
		worker_t * wrk = const_cast <worker_t *> (worker);
		// Получаем сокет для подключения к серверу
		auto socket = this->socket(wrk->url.ip, wrk->url.port, wrk->url.family);
		// Если сокет создан удачно
		if(socket.fd > -1){
			// Устанавливаем сокет подключения
			wrk->fd = socket.fd;
			// Выполняем получение контекста сертификата
			wrk->ssl = this->ssl->init(wrk->url);
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
				bufferevent_setwatermark(wrk->bev, EV_READ, wrk->byteRead, 0);
				// Устанавливаем размер записываемых данных
				bufferevent_setwatermark(wrk->bev, EV_WRITE, wrk->byteWrite, 0);
				// Активируем буферы событий на чтение и запись
				bufferevent_enable(wrk->bev, EV_READ | EV_WRITE);
				// Определяем тип подключения
				switch(wrk->url.family){
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
					this->log->print("connecting to host = %s, port = %u", log_t::flag_t::CRITICAL, wrk->url.ip.c_str(), wrk->url.port);
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
				this->log->print("create good connect to host = %s [%s:%d], socket = %d", log_t::flag_t::INFO, wrk->url.domain.c_str(), wrk->url.ip.c_str(), wrk->url.port, wrk->fd);
				// Сообщаем что все удачно
				result = true;
			// Выполняем новую попытку подключения
			} else {
				// Выводим в лог сообщение
				this->log->print("connecting to host = %s, port = %u", log_t::flag_t::CRITICAL, wrk->url.ip.c_str(), wrk->url.port);
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
 * socket Метод создания сокета
 * @param ip     адрес для которого нужно создать сокет
 * @param port   порт сервера для которого нужно создать сокет
 * @param family тип протокола интернета AF_INET или AF_INET6
 * @return       параметры подключения к серверу
 */
const awh::Core::socket_t awh::Core::socket(const string & ip, const u_int port, const int family) const noexcept {
	// Результат работы функции
	socket_t result;
	// Если IP адрес передан
	if(!ip.empty() && (port > 0) && (port <= 65535)){
		// Адрес сервера для биндинга
		string host = "";
		// Размер структуры подключения
		socklen_t size = 0;
		// Объект подключения
		struct sockaddr * sin = nullptr;
		// Определяем тип подключения
		switch(family){
			// Для протокола IPv4
			case AF_INET: {
				// Получаем список ip адресов
				auto ips = this->net.v4.first;
				// Если количество элементов больше 1
				if(ips.size() > 1){
					// рандомизация генератора случайных чисел
					srand(time(0));
					// Получаем ip адрес
					host = ips.at(rand() % ips.size());
				// Выводим только первый элемент
				} else host = ips.front();
				// Очищаем всю структуру для клиента
				memset(&result.client, 0, sizeof(result.client));
				// Очищаем всю структуру для сервера
				memset(&result.server, 0, sizeof(result.server));
				// Устанавливаем протокол интернета
				result.client.sin_family = AF_INET;
				result.server.sin_family = AF_INET;
				// Устанавливаем произвольный порт для локального подключения
				result.client.sin_port = htons(0);
				// Устанавливаем порт для локального подключения
				result.server.sin_port = htons(port);
				// Устанавливаем адрес для локальго подключения
				result.client.sin_addr.s_addr = inet_addr(host.c_str());
				// Устанавливаем адрес для удаленного подключения
				result.server.sin_addr.s_addr = inet_addr(ip.c_str());
				// Обнуляем серверную структуру
				memset(&result.server.sin_zero, 0, sizeof(result.server.sin_zero));
				// Запоминаем размер структуры
				size = sizeof(result.client);
				// Запоминаем полученную структуру
				sin = reinterpret_cast <struct sockaddr *> (&result.client);
				// Создаем сокет подключения
				result.fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			} break;
			// Для протокола IPv6
			case AF_INET6: {
				// Получаем список ip адресов
				auto ips = this->net.v6.first;
				// Если количество элементов больше 1
				if(ips.size() > 1){
					// рандомизация генератора случайных чисел
					srand(time(0));
					// Получаем ip адрес
					host = ips.at(rand() % ips.size());
				// Выводим только первый элемент
				} else host = ips.front();
				// Переводим ip адрес в полноценный вид
				host = move(this->nwk->setLowIp6(host));
				// Буфер содержащий адрес IPv6
				// char hostClient[INET6_ADDRSTRLEN], hostServer[INET6_ADDRSTRLEN];
				// Очищаем всю структуру для клиента
				memset(&result.client6, 0, sizeof(result.client6));
				// Очищаем всю структуру для сервера
				memset(&result.server6, 0, sizeof(result.server6));
				// Неважно, IPv4 или IPv6
				result.client6.sin6_family = AF_INET6;
				result.server6.sin6_family = AF_INET6;
				// Устанавливаем произвольный порт для локального подключения
				result.client6.sin6_port = htons(0);
				// Устанавливаем порт для локального подключения
				result.server6.sin6_port = htons(port);
				// Указываем адреса
				inet_pton(AF_INET6, host.c_str(), &result.client6.sin6_addr);
				inet_pton(AF_INET6, ip.c_str(), &result.server6.sin6_addr);
				// Устанавливаем адреса
				// inet_ntop(AF_INET6, &result.client6.sin6_addr, hostClient, sizeof(hostClient));
				// inet_ntop(AF_INET6, &result.server6.sin6_addr, hostServer, sizeof(hostServer));
				// Запоминаем размер структуры
				size = sizeof(result.client6);
				// Запоминаем полученную структуру
				sin = reinterpret_cast <struct sockaddr *> (&result.client6);
				// Создаем сокет подключения
				result.fd = ::socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
			} break;
			// Если тип сети не определен
			default: {
				// Выводим сообщение в консоль
				this->log->print("network not allow from server = %s, port = %u", log_t::flag_t::CRITICAL, ip.c_str(), port);
				// Выходим
				return result;
			}
		}
		// Если сокет не создан то выходим
		if(result.fd < 0){
			// Выводим сообщение в консоль
			this->log->print("creating socket to server = %s, port = %u", log_t::flag_t::CRITICAL, ip.c_str(), port);
			// Выходим
			return result;
		}
		// Если - это Unix
		#if !defined(_WIN32) && !defined(_WIN64)
			// Выполняем игнорирование сигнала неверной инструкции процессора
			sockets_t::noSigill(this->log);
			// Устанавливаем разрешение на повторное использование сокета
			sockets_t::reuseable(result.fd, this->log);
			// Отключаем сигнал записи в оборванное подключение
			sockets_t::noSigpipe(result.fd, this->log);
			// Отключаем алгоритм Нейгла для сервера и клиента
			sockets_t::tcpNodelay(result.fd, this->log);
			// Разблокируем сокет
			sockets_t::nonBlocking(result.fd, this->log);
			// Активируем keepalive
			sockets_t::keepAlive(result.fd, this->alive.keepcnt, this->alive.keepidle, this->alive.keepintvl, this->log);
		// Если - это Windows
		#else
			// Выполняем инициализацию WinSock
			this->winSocketInit();
			// Переводим сокет в блокирующий режим
			// sockets_t::blocking(result.fd);
			evutil_make_socket_nonblocking(result.fd);
			// evutil_make_socket_closeonexec(result.fd);
			evutil_make_listen_socket_reuseable(result.fd);
		#endif
		// Выполняем бинд на сокет
		if(::bind(result.fd, sin, size) < 0){
			// Выводим в лог сообщение
			this->log->print("bind local network [%s] error", log_t::flag_t::CRITICAL, host.c_str());
			// Выходим
			return result;
		}
	}
	// Выводим результат
	return result;
}
/**
 * close Метод закрытия подключения воркера
 * @param worker воркер для закрытия подключения
 */
void awh::Core::close(const worker_t * worker) noexcept {
	// Если воркер получен
	if(worker != nullptr){
		// Получаем объект воркера
		worker_t * wrk = const_cast <worker_t *> (worker);
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
void awh::Core::stop() noexcept {
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
void awh::Core::start() noexcept {
	// Если система ещё не запущена
	if(!this->mode){
		try {
			// Разрешаем работу WebSocket
			this->mode = true;
			// Создаем новую базу
			this->base = event_base_new();
			// Резолвер IPv4, создаём резолвер
			this->dns4 = new dns_t(this->fmk, this->log, this->nwk, this->base, this->net.v4.second);
			// Резолвер IPv6, создаём резолвер
			this->dns6 = new dns_t(this->fmk, this->log, this->nwk, this->base, this->net.v6.second);
			// Выводим в консоль информацию
			this->log->print("[+] start service: pid = %u", log_t::flag_t::INFO, getpid());
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
 * isStart Метод проверки на запуск бинда TCP/IP
 * @return результат проверки
 */
bool awh::Core::isStart() const noexcept {
	// Выводим результат проверки
	return this->mode;
}
/**
 * add Метод добавления воркера в биндинг
 * @param worker воркер для добавления
 * @return       идентификатор воркера в биндинге
 */
size_t awh::Core::add(const worker_t * worker) noexcept {
	// Результат работы функции
	size_t result = 0;
	// Если воркер передан и URL адрес существует
	if(worker != nullptr){
		// Получаем объект воркера
		worker_t * wrk = const_cast <worker_t *> (worker);
		// Получаем идентификатор воркера
		result = (1 + this->workers.size());
		// Устанавливаем родительский объект
		wrk->core = this;
		// Устанавливаем идентификатор воркера
		wrk->wid = result;
		// Добавляем воркер в список
		this->workers.emplace(result, wrk);
	}
	// Выводим результат
	return result;
}
/**
 * closeAll Метод отключения всех воркеров
 */
void awh::Core::closeAll() noexcept {
	// Если список воркеров активен
	if(!this->workers.empty()){
		// Переходим по всему списку воркеров
		for(auto & worker : this->workers)
			// Выполняем закрытие подключения
			this->close(worker.second);
	}
}
/**
 * removeAll Метод удаления всех воркеров
 */
void awh::Core::removeAll() noexcept {
	// Выполняем удаление всех воркеров
	this->workers.clear();
}
/**
 * open Метод открытия подключения воркером
 * @param wid идентификатор воркера
 */
void awh::Core::open(const size_t wid) noexcept {
	// Если идентификатор воркера передан
	if((wid > 0) && (this->dns4 != nullptr) && (this->dns6 != nullptr)){
		// Выполняем поиск воркера
		auto it = this->workers.find(wid);
		// Если воркер найден
		if((it != this->workers.end()) && !it->second->url.empty()){
			// Получаем объект воркера
			worker_t * wrk = const_cast <worker_t *> (it->second);
			/**
			 * runFn Функция выполнения запуска системы
			 * @param ip полученный адрес сервера резолвером
			 */
			auto runFn = [wrk, this](const string & ip) noexcept {
				// Если IP адрес получен
				if(!ip.empty()){
					// Запоминаем IP адрес
					wrk->url.ip = ip;
					// Если подключение выполнено
					if(this->connect(wrk)) return;
					// Сообщаем, что подключение не удалось и выводим сообщение
					else this->log->print("broken connect to host %s", log_t::flag_t::CRITICAL, wrk->url.ip.c_str());
					// Если файловый дескриптор очищен, зануляем его
					if(wrk->fd == -1) wrk->fd = 0;
				}
				// Отключаем воркер
				this->close(wrk);
			};
			// Если IP адрес не получен
			if(wrk->url.ip.empty() && !wrk->url.domain.empty()){
				// Определяем тип подключения
				switch(wrk->url.family){
					// Резолвер IPv4, создаём резолвер
					case AF_INET: this->dns4->resolve(wrk->url.domain, wrk->url.family, runFn); break;
					// Резолвер IPv6, создаём резолвер
					case AF_INET6: this->dns6->resolve(wrk->url.domain, wrk->url.family, runFn); break;
				}
			// Выполняем запуск системы
			} else if(!wrk->url.ip.empty()) runFn(wrk->url.ip);
		}
	}
}
/**
 * close Метод закрытия подключения воркером
 * @param wid идентификатор воркера
 */
void awh::Core::close(const size_t wid) noexcept {
	// Если идентификатор воркера передан
	if(wid > 0){
		// Выполняем поиск воркера
		auto it = this->workers.find(wid);
		// Если воркер найден
		if(it != this->workers.end()) this->close(it->second);
	}
}
/**
 * remove Метод удаления воркера из биндинга
 * @param wid идентификатор воркера
 */
void awh::Core::remove(const size_t wid) noexcept {
	// Если идентификатор воркера передан
	if(wid > 0){
		// Выполняем поиск воркера
		auto it = this->workers.find(wid);
		// Если воркер найден
		if(it != this->workers.end()) this->workers.erase(it);
	}
}
/**
 * write Метод записи буфера данных воркером
 * @param buffer буфер для записи данных
 * @param size   размер записываемых данных
 * @param wid    идентификатор воркера
 */
void awh::Core::write(const char * buffer, const size_t size, const size_t wid) noexcept {
	// Если данные переданы
	if((buffer != nullptr) && (size > 0) && (wid > 0)){
		// Выполняем поиск воркера
		auto it = this->workers.find(wid);
		// Если воркер найден
		if(it != this->workers.end()){
			// Получаем объект воркера
			worker_t * wrk = const_cast <worker_t *> (it->second);
			// Активируем разрешение на запись и чтение
			bufferevent_enable(wrk->bev, EV_WRITE | EV_READ);
			// Устанавливаем размер записываемых данных
			bufferevent_setwatermark(wrk->bev, EV_WRITE, (wrk->byteWrite > 0 ? wrk->byteWrite : size), 0);
			// Отправляем серверу сообщение
			bufferevent_write(wrk->bev, buffer, size);
		}
	}
}
/**
 * setVerifySSL Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
 * @param mode флаг состояния разрешения проверки
 */
void awh::Core::setVerifySSL(const bool mode) noexcept {
	// Выполняем установку флага проверки домена
	this->ssl->setVerify(mode);
}
/**
 * setFamily Метод установки тип протокола интернета
 * @param family тип протокола интернета AF_INET или AF_INET6
 */
void awh::Core::setFamily(const int family) noexcept {
	// Устанавливаем тип активного интернет-подключения
	this->net.family = family;
}
/**
 * setCA Метод установки CA-файла корневого SSL сертификата
 * @param cafile адрес CA-файла
 * @param capath адрес каталога где находится CA-файл
 */
void awh::Core::setCA(const string & cafile, const string & capath) noexcept {
	// Устанавливаем адрес CA-файла
	this->ssl->setCA(cafile, capath);
}
/**
 * setNet Метод установки параметров сети
 * @param ip     список IP адресов компьютера с которых разрешено выходить в интернет
 * @param ns     список серверов имён, через которые необходимо производить резолвинг доменов
 * @param family тип протокола интернета AF_INET или AF_INET6
 */
void awh::Core::setNet(const vector <string> & ip, const vector <string> & ns, const int family) noexcept {
	// Устанавливаем тип активного интернет-подключения
	this->net.family = family;
	// Определяем тип интернет-протокола
	switch(this->net.family){
		// Если - это интернет-протокол IPv4
		case AF_INET: {
			// Если IP адреса переданы, устанавливаем их
			if(!ip.empty()) this->net.v4.first.assign(ip.cbegin(), ip.cend());
			// Если сервера имён переданы, устанавливаем их
			if(!ns.empty()) this->net.v4.second.assign(ns.cbegin(), ns.cend());
		} break;
		// Если - это интернет-протокол IPv6
		case AF_INET6: {
			// Если IP адреса переданы, устанавливаем их
			if(!ip.empty()) this->net.v6.first.assign(ip.cbegin(), ip.cend());
			// Если сервера имён переданы, устанавливаем их
			if(!ns.empty()) this->net.v6.second.assign(ns.cbegin(), ns.cend());
		} break;
	}
}
/**
 * Core Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Core::Core(const fmk_t * fmk, const log_t * log) noexcept {
	try {
		// Устанавливаем зависимые модули
		this->fmk = fmk;
		this->log = log;
		// Создаём объект для работы с сетью
		this->nwk = new network_t(this->fmk);
		// Создаём объект URI
		this->uri = new uri_t(this->fmk, this->nwk);
		// Создаём объект для работы с SSL
		this->ssl = new ssl_t(this->fmk, this->log, this->uri);
	// Если происходит ошибка то игнорируем её
	} catch(const bad_alloc&) {
		// Выводим сообщение об ошибке
		log->print("%s", log_t::flag_t::CRITICAL, "memory could not be allocated");
		// Выходим из приложения
		exit(EXIT_FAILURE);
	}
}
/**
 * ~Core Деструктор
 */
awh::Core::~Core() noexcept {
	// Выполняем остановку сервера
	this->stop();
	// Если объект для работы с SSL создан
	if(this->ssl != nullptr) delete this->ssl;
	// Удаляем объект для работы с сетью
	if(this->nwk != nullptr) delete this->nwk;
	// Удаляем объект для работы с URI
	if(this->uri != nullptr) delete this->uri;
	// Если объект DNS IPv4 резолвера создан
	if(this->dns4 != nullptr) delete this->dns4;
	// Если объект DNS IPv6 резолвера создан
	if(this->dns6 != nullptr) delete this->dns6;
	// Если - это Windows
	#if defined(_WIN32) || defined(_WIN64)
		// Очищаем сетевой контекст
		this->winSocketClean();
	#endif
}
