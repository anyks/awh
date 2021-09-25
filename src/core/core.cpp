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
 * connect Метод создания подключения к удаленному серверу
 * @return результат подключения
 */
const bool awh::Core::connect() noexcept {
	// Результат работы функции
	bool result = false;
	// Если объект фреймворка существует
	if((this->fmk != nullptr) && !this->halt){
		// Размер структуры подключения
		socklen_t size = 0;
		// Объект подключения
		struct sockaddr * sin = nullptr;
		// Получаем сокет для подключения к серверу
		auto socket = this->socket(this->url.ip, this->url.port, this->net.family);
		// Если сокет создан удачно
		if(socket.fd > -1){
			// Устанавливаем сокет подключения
			this->fd = socket.fd;
			// Выполняем получение контекста сертификата
			this->sslctx = this->ssl->init(this->url);
			// Если SSL клиент разрешен
			if(this->sslctx.mode){
				// Создаем буфер событий для сервера зашифрованного подключения
				this->bev = bufferevent_openssl_socket_new(this->base, this->fd, this->sslctx.ssl, BUFFEREVENT_SSL_CONNECTING, BEV_OPT_THREADSAFE);
				// this->bev = bufferevent_openssl_socket_new(this->base, this->fd, this->sslctx.ssl, BUFFEREVENT_SSL_CONNECTING, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
				// Разрешаем непредвиденное грязное завершение работы
				bufferevent_openssl_set_allow_dirty_shutdown(this->bev, 1);
			// Создаем буфер событий для сервера
			} else this->bev = bufferevent_socket_new(this->base, this->fd, BEV_OPT_THREADSAFE);
			// } else this->bev = bufferevent_socket_new(this->base, this->fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
			// Если буфер событий создан
			if(this->bev != nullptr){
				// Устанавливаем коллбеки
				bufferevent_setcb(this->bev, &read, nullptr, &event, this);
				// Очищаем буферы событий при завершении работы
				bufferevent_flush(this->bev, EV_READ | EV_WRITE, BEV_FINISHED);
				// Если флаг ожидания входящих сообщений, активирован
				if(this->wait){
					// Устанавливаем таймаут ожидания поступления данных
					struct timeval readTimeout = {READ_TIMEOUT, 0};
					// Устанавливаем таймаут ожидания записи данных
					struct timeval writeTimeout = {WRITE_TIMEOUT, 0};
					// Устанавливаем таймаут получения данных
					bufferevent_set_timeouts(this->bev, &readTimeout, &writeTimeout);
				}
				// Устанавливаем водяной знак на 1 байт (чтобы считывать данные когда они действительно приходят)
				// bufferevent_setwatermark(this->bev, EV_READ | EV_WRITE, 1, 0);
				// Активируем буферы событий на чтение и запись
				bufferevent_enable(this->bev, EV_READ | EV_WRITE);
				// Определяем тип подключения
				switch(this->net.family){
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
				if(bufferevent_socket_connect(this->bev, sin, size) < 0){
					// Выводим в лог сообщение
					this->log->print("connecting to host = %s, port = %u", log_t::flag_t::CRITICAL, this->url.ip.c_str(), this->url.port);
					// Если нужно выполнить автоматическое переподключение
					if(this->reconnect){
						// Выполняем отключение
						this->close();
						// Выдерживаем паузу в 10 секунд
						this->delay(10);
						// Выполняем новое подключение
						return this->connect();
					// Если автоматическое подключение выполнять не нужно
					} else {
						// Иначе останавливаем работу
						this->stop();
						// Просто выходим
						return result;
					}
				}
				// Выводим в лог сообщение
				this->log->print("create good connect to host = %s [%s:%d], socket = %d", log_t::flag_t::INFO, this->url.domain.c_str(), this->url.ip.c_str(), this->url.port, this->fd);
				// Сообщаем что все удачно
				result = true;
			// Выполняем новую попытку подключения
			} else {
				// Выводим в лог сообщение
				this->log->print("connecting to host = %s, port = %u", log_t::flag_t::CRITICAL, this->url.ip.c_str(), this->url.port);
				// Если нужно выполнить автоматическое переподключение
				if(this->reconnect){
					// Выполняем отключение
					this->close();
					// Выдерживаем паузу в 10 секунд
					this->delay(10);
					// Выполняем новое подключение
					return this->connect();
				// Иначе останавливаем работу
				} else this->stop();
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
 * chunking Метод обработки получения чанков
 * @param chunk бинарный буфер чанка
 * @param ctx   контекст объекта http
 */
void awh::Core::chunking(const vector <char> & chunk, const http_t * ctx) noexcept {
	// Если данные получены, формируем тело сообщения
	if(!chunk.empty()) const_cast <http_t *> (ctx)->addBody(chunk.data(), chunk.size());
}
/**
 * event Метод обработка входящих событий с сервера
 * @param bev    буфер события
 * @param events произошедшее событие
 * @param ctx    передаваемый контекст
 */
void awh::Core::event(struct bufferevent * bev, const short events, void * ctx) noexcept {
	// Если подключение не передано
	if(ctx != nullptr){
		// Получаем объект подключения
		core_t * core = reinterpret_cast <core_t *> (ctx);
		// Если фреймворк получен
		if(core->fmk != nullptr){
			// Получаем текущий сокет
			const evutil_socket_t socket = bufferevent_getfd(bev);
			// Если подключение удачное
			if(events & BEV_EVENT_CONNECTED){
				// Выводим в лог сообщение
				core->log->print("connect client to server [%s:%d]", log_t::flag_t::INFO, core->url.ip.c_str(), core->url.port);
				// Выполняем запрос на сервер
				core->request();
			// Если это ошибка или завершение работы
			} else if(events & (BEV_EVENT_ERROR | BEV_EVENT_EOF | BEV_EVENT_TIMEOUT)) {
				// Если это ошибка
				if(events & BEV_EVENT_ERROR)
					// Выводим в лог сообщение
					core->log->print("closing server [%s:%d] error: %s", log_t::flag_t::WARNING, core->url.ip.c_str(), core->url.port, evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
				// Если - это таймаут, выводим сообщение в лог
				else if(events & BEV_EVENT_TIMEOUT) core->log->print("timeout server [%s:%d]", log_t::flag_t::WARNING, core->url.ip.c_str(), core->url.port);
				// Если нужно выполнить автоматическое переподключение
				if(core->reconnect){
					// Закрываем подключение
					core->close();
					// Выдерживаем паузу на 10 секунд
					core->delay(10);
					// Выполняем новое подключение
					core->connect();
				// Иначе останавливаем работу
				} else core->stop();
			}
		}
	}
}
/**
 * stop Метод остановки клиента
 */
void awh::Core::stop() noexcept {
	// Если система уже запущена
	if(this->mode && !this->halt){
		// Запоминаем, что работа остановлена
		this->halt = true;
		// Запрещаем работу WebSocket
		this->mode = false;
		// Запрещаем запись данных клиенту
		bufferevent_disable(this->bev, EV_READ | EV_WRITE);
		// Завершаем работу базы событий
		event_base_loopbreak(this->base);
		// Закрываем подключение сервера
		this->close();
	}
}
/**
 * close Метод закрытия соединения сервера
 */
void awh::Core::close() noexcept {
	// Если событие сервера существует
	if(this->bev != nullptr){
		// Запрещаем чтение запись данных серверу
		bufferevent_disable(this->bev, EV_WRITE | EV_READ);
		// Если - это Windows
		#if defined(_WIN32) || defined(_WIN64)
			// Отключаем подключение для сокета
			if(this->fd > 0) shutdown(this->fd, SD_BOTH);
		// Если - это Unix
		#else
			// Отключаем подключение для сокета
			if(this->fd > 0) shutdown(this->fd, SHUT_RDWR);
		#endif
		// Закрываем подключение
		if(this->fd > 0) evutil_closesocket(this->fd);
		// Удаляем буфер события
		bufferevent_free(this->bev);
		// Устанавливаем что событие удалено
		this->bev = nullptr;
	}
	// Выполняем удаление контекста SSL
	this->ssl->clear(this->sslctx);
	// Зануляем сокет
	this->fd = -1;
	// Выводим сообщение об ошибке
	this->log->print("%s", log_t::flag_t::INFO, "disconnected from the server");
	// Если - это Windows
	#if defined(_WIN32) || defined(_WIN64)
		// Очищаем сетевой контекст
		this->winSocketClean();
	#endif
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
			// Отключаем флаг остановки работы
			this->halt = false;
			/**
			 * runFn Функция выполнения запуска системы
			 * @param ip полученный адрес сервера резолвером
			 */
			auto runFn = [this](const string & ip) noexcept {
				// Если IP адрес получен
				if(!ip.empty()){
					// Запоминаем IP адрес
					this->url.ip = ip;
					// Если подключение выполнено
					if(this->connect()) return;
					// Сообщаем, что подключение не удалось и выводим сообщение
					else this->log->print("broken connect to host %s", log_t::flag_t::CRITICAL, this->url.ip.c_str());
				}
				// Останавливаем работу системы
				this->stop();
			};
			// Создаем новую базу
			this->base = event_base_new();
			// Определяем тип подключения
			switch(this->net.family){
				// Резолвер IPv4, создаём резолвер
				case AF_INET: this->dns = new dns_t(this->fmk, this->log, this->nwk, this->base, this->net.v4.second); break;
				// Резолвер IPv6, создаём резолвер
				case AF_INET6: this->dns = new dns_t(this->fmk, this->log, this->nwk, this->base, this->net.v6.second); break;
			}
			// Если IP адрес не получен
			if(this->url.ip.empty() && !this->url.domain.empty())
				// Выполняем резолвинг домена
				this->resolve(this->url, runFn);
			// Выполняем запуск системы
			else if(!this->url.ip.empty()) runFn(this->url.ip);
			// Выводим в консоль информацию
			this->log->print("[+] start service: pid = %u", log_t::flag_t::INFO, getpid());
			// Активируем перебор базы событий
			event_base_loop(this->base, EVLOOP_NO_EXIT_ON_EMPTY);
			// Удаляем dns резолвер
			delete this->dns;
			// Зануляем DNS объект
			this->dns = nullptr;
			// Удаляем объект базы событий
			event_base_free(this->base);
			// Очищаем все глобальные переменные
			libevent_global_shutdown();
			// Если остановка не выполнена
			if(this->reconnect && !this->halt){
				// Останавливаем работу WebSocket
				this->mode = false;
				// Выводим в консоль информацию
				this->log->print("[=] restart service: pid = %u", log_t::flag_t::INFO, getpid());
				// Выполняем переподключение
				this->start();
			// Выводим в консоль информацию
			} else this->log->print("[-] stop service: pid = %u", log_t::flag_t::INFO, getpid());
		// Если происходит ошибка то игнорируем её
		} catch(const bad_alloc&) {
			// Выводим сообщение об ошибке
			this->log->print("%s", log_t::flag_t::CRITICAL, "memory could not be allocated");
			// Если нужно выполнять переподключение
			if(this->reconnect){
				// Останавливаем работу WebSocket
				this->mode = false;
				// Выполняем переподключение
				this->start();
			}
		}
	}
}
/**
 * resolve Метод выполняющая резолвинг хоста http запроса
 * @param url      параметры хоста, для которого нужно получить IP адрес
 * @param callback функция обратного вызова
 */
void awh::Core::resolve(const uri_t::url_t & url, function <void (const string &)> callback) noexcept {
	// Если доменное имя указано
	if(!url.domain.empty())
		// Выполняем резолвинг домена
		this->dns->resolve(url.domain, url.family, callback);
	// Если доступен IP адрес
	else if(!url.ip.empty()) callback(url.ip);
	// Иначе возвращаем пустоту
	else callback("");
}
/**
 * setChunkingFn Метод установки функции обратного вызова для получения чанков
 * @param callback функция обратного вызова
 */
void awh::Core::setChunkingFn(function <void (const vector <char> &, const http_t *)> callback) noexcept {
	// Устанавливаем функцию обработки вызова для получения чанков
	this->http->setChunkingFn(callback);
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
 * setChunkSize Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::Core::setChunkSize(const size_t size) noexcept {
	// Устанавливаем размер чанка
	this->http->setChunkSize(size);
}
/**
 * setWaitMessage Метод установки флага ожидания входящих сообщений
 * @param mode флаг состояния разрешения проверки
 */
void awh::Core::setWaitMessage(const bool mode) noexcept {
	// Устанавливаем флаг ожидания входящих сообщений
	this->wait = mode;
}
/**
 * setAutoReconnect Метод установки флага автоматического переподключения
 * @param mode флаг автоматического переподключения
 */
void awh::Core::setAutoReconnect(const bool mode) noexcept {
	// Устанавливаем флаг автоматического переподключения
	this->reconnect = mode;
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
 * setUserAgent Метод установки User-Agent для HTTP запроса
 * @param userAgent агент пользователя для HTTP запроса
 */
void awh::Core::setUserAgent(const string & userAgent) noexcept {
	// Устанавливаем UserAgent
	if(!userAgent.empty()) this->http->setUserAgent(userAgent);
}
/**
 * setCompress Метод установки метода сжатия
 * @param метод сжатия сообщений
 */
void awh::Core::setCompress(const http_t::compress_t compress) noexcept {
	// Устанавливаем метод сжатия
	this->http->setCompress(compress);
}
/**
 * setUser Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::Core::setUser(const string & login, const string & password) noexcept {
	// Если пользователь и пароль переданы
	if(!login.empty() && !password.empty())
		// Устанавливаем логин и пароль пользователя
		this->http->setUser(login, password);
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
 * setServ Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::Core::setServ(const string & id, const string & name, const string & ver) noexcept {
	// Устанавливаем данные сервиса
	this->http->setServ(id, name, ver);
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
 * setCrypt Метод установки параметров шифрования
 * @param pass пароль шифрования передаваемых данных
 * @param salt соль шифрования передаваемых данных
 * @param aes  размер шифрования передаваемых данных
 */
void awh::Core::setCrypt(const string & pass, const string & salt, const hash_t::aes_t aes) noexcept {
	// Устанавливаем параметры шифрования
	this->http->setCrypt(pass, salt, aes);
}
/**
 * setAuthType Метод установки типа авторизации
 * @param type      тип авторизации
 * @param algorithm алгоритм шифрования для Digest авторизации
 */
void awh::Core::setAuthType(const auth_t::type_t type, const auth_t::algorithm_t algorithm) noexcept {
	// Если объект авторизации создан
	this->http->setAuthType(type, algorithm);
}
/**
 * Core Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 * @param uri объект работы с URI
 * @param nwk объект методов для работы с сетью
 */
awh::Core::Core(const fmk_t * fmk, const log_t * log, const uri_t * uri, const network_t * nwk) noexcept {
	try {
		// Устанавливаем зависимые модули
		this->fmk = fmk;
		this->log = log;
		this->uri = uri;
		this->nwk = nwk;
		// Резервируем память для работы с буфером данных WebSocket
		this->wdt = new char[BUFFER_CHUNK];
		// Создаём объект для работы с SSL
		this->ssl = new ssl_t(this->fmk, this->log, this->uri);
		// Создаём объект для работы с HTTP
		this->http = new http_t(this->fmk, this->log, this->uri);
		// Устанавливаем функцию обработки вызова для получения чанков
		this->http->setChunkingFn(&chunking);
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
	// Если объект DNS резолвера создан
	if(this->dns != nullptr) delete this->dns;
	// Если объект для работы с SSL создан
	if(this->ssl != nullptr) delete this->ssl;
	// Удаляем объект работы с HTTP
	if(this->http != nullptr) delete this->http;
	// Если буфер данных WebSocket создан
	if(this->wdt != nullptr) delete [] this->wdt;
	// Если - это Windows
	#if defined(_WIN32) || defined(_WIN64)
		// Очищаем сетевой контекст
		this->winSocketClean();
	#endif
}
