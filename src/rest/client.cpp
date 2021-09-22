/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <rest/client.hpp>

// Если - это Windows
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * winSocketInit Метод инициализации WinSock
	 */
	void awh::Rest::winSocketInit() const noexcept {
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
	void awh::Rest::winSocketClean() const noexcept {
		// Очищаем сетевой контекст
		WSACleanup();
		// Запоминаем, что winSock не инициализирован
		this->winSock = false;
	}
#endif
/**
 * requestProxy Метод выполнения HTTP запроса к прокси-серверу
 */
void awh::Rest::requestProxy() noexcept {

	// Создаём объект для работы с авторизацией
	auth_t auth(this->fmk, this->log);
	// Устанавливаем логин пользователя
	auth.setLogin(this->proxyUrl.user);
	// Устанавливаем пароль пользователя
	auth.setPassword(this->proxyUrl.pass);

	auth.setType(auth_t::type_t::BASIC);

	// Получаем заголовок авторизации
	const string & authHeader = auth.header();
	// Если данные авторизации получены
	if(!authHeader.empty()){
		// Получаем значение заголовка
		const_cast <string *> (&authHeader)->assign(authHeader.begin() + 15, authHeader.end() - 2);
		// Устанавливаем авторизационные параметры
		// evhttp_add_header(store, "Proxy-Authorization", authHeader.c_str());
	}

	string request = "CONNECT 2ip.ru:443 HTTP/1.1\r\n"
				"Connection: keep-alive\r\n"
				"Proxy-Connection: keep-alive\r\n"
				"Host: 2ip.ru\r\n";
	request.append(this->fmk->format("Proxy-Authorization: %s\r\n\r\n", authHeader.c_str()));

	// Если запрос получен
	if(!request.empty()){
		// Активируем разрешение на запись и чтение
		bufferevent_enable(this->evbuf.bev, EV_WRITE | EV_READ);
		// Отправляем серверу сообщение
		bufferevent_write(this->evbuf.bev, request.data(), request.size());
	}
}
/**
 * connectProxy Метод создания подключения к удаленному прокси-серверу
 * @return результат подключения
 */
const bool awh::Rest::connectProxy() noexcept {
	// Результат работы функции
	bool result = false;
	// Если объект фреймворка существует
	if(this->fmk != nullptr){
		// Размер структуры подключения
		socklen_t size = 0;
		// Объект подключения
		struct sockaddr * sin = nullptr;
		// Получаем сокет для подключения к серверу
		auto socket = this->socket(this->proxyUrl.ip, this->proxyUrl.port, this->proxyUrl.family);
		// Если сокет создан удачно
		if(socket.fd > -1){
			// Устанавливаем сокет подключения
			this->fd = socket.fd;
			// Выполняем получение контекста сертификата
			this->sslctx = this->ssl->init(this->proxyUrl);
			// Если SSL клиент разрешен
			if(this->sslctx.mode){
				// Создаем буфер событий для сервера зашифрованного подключения
				this->evbuf.bev = bufferevent_openssl_socket_new(this->evbuf.base, this->fd, this->sslctx.ssl, BUFFEREVENT_SSL_CONNECTING, BEV_OPT_THREADSAFE);
				// Разрешаем непредвиденное грязное завершение работы
				bufferevent_openssl_set_allow_dirty_shutdown(this->evbuf.bev, 1);
			// Создаем буфер событий для сервера
			} else this->evbuf.bev = bufferevent_socket_new(this->evbuf.base, this->fd, BEV_OPT_THREADSAFE);
			// Если буфер событий создан
			if(this->evbuf.bev != nullptr){
				// Устанавливаем таймаут ожидания поступления данных
				struct timeval timeout = {READ_TIMEOUT, 0};
				// Устанавливаем таймаут получения данных
				bufferevent_set_timeouts(this->evbuf.bev, &timeout, nullptr);
				// Устанавливаем коллбеки
				bufferevent_setcb(this->evbuf.bev, &readProxy, nullptr, &eventProxy, this);
				// Очищаем буферы событий при завершении работы
				bufferevent_flush(this->evbuf.bev, EV_READ | EV_WRITE, BEV_FINISHED);
				// Активируем буферы событий на чтение и запись
				bufferevent_enable(this->evbuf.bev, EV_READ | EV_WRITE);
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
				if(bufferevent_socket_connect(this->evbuf.bev, sin, size) < 0){
					// Закрываем подключение
					this->clear();
					// Выводим в лог сообщение
					this->log->print("connecting to proxy host = %s, port = %u", log_t::flag_t::CRITICAL, this->proxyUrl.ip.c_str(), this->proxyUrl.port);
					// Просто выходим
					return result;
				}
				// Выводим в лог сообщение
				this->log->print("create good connect to proxy host = %s [%s:%d], socket = %d", log_t::flag_t::INFO, this->proxyUrl.domain.c_str(), this->proxyUrl.ip.c_str(), this->proxyUrl.port, this->fd);
				// Сообщаем что все удачно
				result = true;
			// Выполняем новую попытку подключения
			} else {
				// Закрываем подключение
				this->clear();
				// Выводим в лог сообщение
				this->log->print("connecting to proxy host = %s, port = %u", log_t::flag_t::CRITICAL, this->proxyUrl.ip.c_str(), this->proxyUrl.port);
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
const awh::Rest::socket_t awh::Rest::socket(const string & ip, const u_int port, const int family) const noexcept {
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

#define BIO_get_data(b) (b)->ptr

/**
 * readProxy Метод чтения данных с сокета прокси-сервера
 * @param bev буфер события
 * @param ctx передаваемый контекст
 */
void awh::Rest::readProxy(struct bufferevent * bev, void * ctx){

	cout << " ---------------1 " << endl;

	// Если данные переданы верные
	if((bev != nullptr) && (ctx != nullptr)){
		// Получаем буферы входящих данных
		struct evbuffer * input = bufferevent_get_input(bev);
		// Получаем размер входящих данных
		size_t size = evbuffer_get_length(input);
		// Если данные существуют
		if(size > 0){
			// Получаем объект подключения
			rest_t * http = reinterpret_cast <rest_t *> (ctx);
			// Выполняем компенсацию размера полученных данных
			size = (size > BUFFER_CHUNK ? BUFFER_CHUNK : size);
			// Копируем в буфер полученные данные
			evbuffer_copyout(input, (void *) http->hdt, size);

			cout << " ---------------2 " << string(http->hdt, size) << " == " << size << endl;

			if(!http->flg){

				http->flg = true;

				// Выполняем получение контекста сертификата
				http->sslctx = http->ssl->init(* http->req.uri);

				evutil_socket_t fd = bufferevent_getfd(bev);

				/*
				BIO *bio;
				bio = BIO_new_socket((int)fd, 0);
				SSL_set_bio(http->sslctx.ssl, bio, bio);
				*/

				// struct bufferevent * bufev = BIO_get_data(bio);

				/*
				auto bio = BIO_new_socket(static_cast <int> (fd), BIO_NOCLOSE);
				BIO_set_nbio(bio, 1);
				SSL_set_bio(http->sslctx.ssl, bio, bio);
				BIO_set_nbio(bio, 0);
				*/

				http->evbuf.bev = bufferevent_openssl_socket_new(http->evbuf.base, fd, http->sslctx.ssl, BUFFEREVENT_SSL_CONNECTING, BEV_OPT_THREADSAFE | BEV_OPT_DEFER_CALLBACKS);
				// Оборачиваем текущее соединение в SSL BIO
				// http->evbuf.bev = bufferevent_openssl_filter_new(http->evbuf.base, bev, http->sslctx.ssl, BUFFEREVENT_SSL_CONNECTING, BEV_OPT_THREADSAFE | BEV_OPT_DEFER_CALLBACKS);
				// Если буфер событий не создан
				if(http->evbuf.bev == nullptr){
					// Очищаем контекст
					http->clear();
					// Сообщаем, что буфер событий не может быть создан
					http->log->print("%s", log_t::flag_t::CRITICAL, "the event buffer could not be created");
					// Завершаем работу функции
					return;
				}
				// Разрешаем пересматривать подключение
				// bufferevent_ssl_renegotiate(http->evbuf.bev);


				string request = "GET / HTTP/1.1\r\n"
					"Connection: close\r\n"
					"User-Agent: curl/7.64.1\r\n"
					"Accept: */*\r\n"
					"Host: 2ip.ru\r\n\r\n";

				// Если запрос получен
				if(!request.empty()){
					// Активируем разрешение на запись и чтение
					bufferevent_enable(http->evbuf.bev, EV_WRITE | EV_READ);
					// Отправляем серверу сообщение
					bufferevent_write(http->evbuf.bev, request.data(), request.size());
				}
			} else {

				// Получаем буферы входящих данных
				struct evbuffer * input = bufferevent_get_input(http->evbuf.bev);
				// Получаем размер входящих данных
				size_t size = evbuffer_get_length(input);
				// Если данные существуют
				if(size > 0){
					// Копируем в буфер полученные данные
					evbuffer_copyout(input, (void *) http->hdt, size);

					// auto buff = http->hash->decompressGzip(http->hdt, size);

					// if(!buff.empty()) cout << " ---------------3 " << string(buff.begin(), buff.end()) << endl;
					cout << " ---------------3 " << string(http->hdt, size) << endl;

				}
			}

			/*
			// Создаём событие подключения
			http->evbuf.evcon = evhttp_connection_base_bufferevent_new(http->evbuf.base, http->evbuf.dns, http->evbuf.bev, (!http->req.uri->ip.empty() ? http->req.uri->ip.c_str() : http->req.uri->domain.c_str()), http->req.uri->port);
			// Если событие подключения не создан
			if(http->evbuf.evcon == nullptr){
				// Очищаем контекст
				http->clear();
				// Сообщаем, что событие подключения не создано
				http->log->print("%s", log_t::flag_t::CRITICAL, "connection event not created");
				// Завершаем работу функции
				return;
			}
			// Выполняем 5 попыток запросить данные
			evhttp_connection_set_retries(http->evbuf.evcon, 5);
			// Таймаут на выполнение в 5 секунд
			evhttp_connection_set_timeout(http->evbuf.evcon, 5);
			// Заставляем выполнять подключение по указанному протоколу сети
			evhttp_connection_set_family(http->evbuf.evcon, http->req.uri->family);
			// Создаём объект выполнения REST запроса
			struct evhttp_request * req = evhttp_request_new(callback, &http->req.response);
			// Если объект REST запроса не создан
			if(req == nullptr){
				// Очищаем контекст
				http->clear();
				// Сообщаем, что событие REST запроса не создано
				http->log->print("%s", log_t::flag_t::CRITICAL, "REST request event not created");
				// Завершаем работу функции
				return;
			}
			// Создаём базу событий DNS
			http->evbuf.dns = http->dns->init((!http->req.uri->domain.empty() ? http->req.uri->domain : http->req.uri->ip), http->req.uri->family, http->evbuf.base);


			// Получаем объект заголовков
			struct evkeyvalq * store = evhttp_request_get_output_headers(req);
			// Устанавливаем режим подключения (длительное подключение)
			evhttp_add_header(store, "Connection", "keep-alive");
			// Устанавливаем хост запрашиваемого удалённого сервера
			evhttp_add_header(store, "Host", "2ip.ru");
			// Выполняем REST запрос на сервер
			const int request = evhttp_make_request(http->evbuf.evcon, req, EVHTTP_REQ_GET, "/");
			// Если запрос не выполнен
			if(request != 0){
				// Очищаем контекст
				http->clear();
				// Сообщаем, что запрос не выполнен
				http->log->print("%s", log_t::flag_t::CRITICAL, "REST request failed");
				// Завершаем работу функции
				return;
			}
			*/

			

			// Удаляем данные из буфера
			evbuffer_drain(input, size);

			// http->clear();
		}
	}
}
/**
 * eventProxy Метод обработка входящих событий с прокси-сервера
 * @param bev    буфер события
 * @param events произошедшее событие
 * @param ctx    передаваемый контекст
 */
void awh::Rest::eventProxy(struct bufferevent * bev, const short events, void * ctx) noexcept {
	// Если подключение не передано
	if(ctx != nullptr){
		// Получаем объект подключения
		rest_t * http = reinterpret_cast <rest_t *> (ctx);
		// Если фреймворк получен
		if(http->fmk != nullptr){
			// Получаем текущий сокет
			const evutil_socket_t socket = bufferevent_getfd(bev);
			// Если подключение удачное
			if(events & BEV_EVENT_CONNECTED){
				// Выводим в лог сообщение
				http->log->print("connect client to server [%s:%d]", log_t::flag_t::INFO, http->proxyUrl.ip.c_str(), http->proxyUrl.port);
				// Выполняем запрос на сервер
				http->requestProxy();
			// Если это ошибка или завершение работы
			} else if(events & (BEV_EVENT_ERROR | BEV_EVENT_EOF | BEV_EVENT_TIMEOUT)) {
				// Если это ошибка
				if(events & BEV_EVENT_ERROR)
					// Выводим в лог сообщение
					http->log->print("closing server [%s:%d] error: %s", log_t::flag_t::WARNING, http->proxyUrl.ip.c_str(), http->proxyUrl.port, evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
				// Если - это таймаут, выводим сообщение в лог
				else if(events & BEV_EVENT_TIMEOUT) http->log->print("timeout server [%s:%d]", log_t::flag_t::WARNING, http->proxyUrl.ip.c_str(), http->proxyUrl.port);
					// Закрываем подключение
					http->clear();
			}
		}
	}
}
/**
 * resolve Метод выполняющая резолвинг хоста сервера
 * @param url      параметры хоста, для которого нужно получить IP адрес
 * @param callback функция обратного вызова
 */
void awh::Rest::resolve(const uri_t::url_t & url, function <void (const string &)> callback) noexcept {
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
 * GET Метод REST запроса
 * @param url     адрес запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const string awh::Rest::GET(const uri_t::url_t & url, const unordered_map <string, string> & headers) noexcept {
	// Результат работы функции
	string result = "";
	// Если URL адрес передан
	if(!url.empty()){
		// Устанавливаем параметры запроса
		this->req.uri = &url;
		// Устанавливаем заголовки запроса
		this->req.headers = &headers;
		// Устанавливаем тип запроса
		this->req.method = EVHTTP_REQ_GET;
		/**
		 * Выполняем REST запрос в отдельном потоке, чтобы не мешать другим методам
		 */
		thread thr([&result, this]{
			// Выполняем REST запрос
			this->PROXY();
			// Проверяем на наличие ошибок
			if(!this->res.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, this->res.code, this->res.mess.c_str());
			// Если тело ответа получено
			if(!this->res.body.empty()) result = this->res.body;
		});
		// Ожидаем завершение запроса
		thr.join();
	}
	// Выводим результат
	return result;
}
/**
 * DEL Метод REST запроса
 * @param url     адрес запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const string awh::Rest::DEL(const uri_t::url_t & url, const unordered_map <string, string> & headers) noexcept {
	// Результат работы функции
	string result = "";
	// Если URL адрес передан
	if(!url.empty()){
		/**
		 * Выполняем REST запрос в отдельном потоке, чтобы не мешать другим методам
		 */
		thread thr([&result, this](const uri_t::url_t & url, const unordered_map <string, string> & headers){
			/*
			// Выполняем REST запрос
			const auto & response = this->REST(url, EVHTTP_REQ_DELETE, headers);
			// Проверяем на наличие ошибок
			if(!response.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, response.code, response.mess.c_str());
			// Если тело ответа получено
			if(!response.body.empty()) result = response.body;
			*/
		}, ref(url), ref(headers));
		// Ожидаем завершение запроса
		thr.join();
	}
	// Выводим результат
	return result;
}
/**
 * PUT Метод REST запроса в формате JSON
 * @param url     адрес запроса
 * @param body    тело запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const string awh::Rest::PUT(const uri_t::url_t & url, const json & body, const unordered_map <string, string> & headers) noexcept {
	// Результат работы функции
	string result = "";
	// Если URL адрес передан
	if(!url.empty() && !body.empty()){
		// Добавляем заголовок типа контента
		const_cast <unordered_map <string, string> *> (&headers)->emplace("Content-Type", "application/json");
		/**
		 * Выполняем REST запрос в отдельном потоке, чтобы не мешать другим методам
		 */
		thread thr([&result, this](const uri_t::url_t & url, const unordered_map <string, string> & headers, const json & body){
			/*
			// Результирующая строка
			const string bodyData = body.dump();
			// Выполняем REST запрос
			const auto & response = this->REST(url, EVHTTP_REQ_PUT, headers, bodyData);
			// Проверяем на наличие ошибок
			if(!response.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, response.code, response.mess.c_str());
			// Если тело ответа получено
			if(!response.body.empty()) result = response.body;
			*/
		}, ref(url), ref(headers), ref(body));
		// Ожидаем завершение запроса
		thr.join();
	}
	// Выводим результат
	return result;
}
/**
 * PUT Метод REST запроса в формате JSON
 * @param url     адрес запроса
 * @param body    тело запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const string awh::Rest::PUT(const uri_t::url_t & url, const string & body, const unordered_map <string, string> & headers) noexcept {
	// Результат работы функции
	string result = "";
	// Если URL адрес передан
	if(!url.empty()){
		/**
		 * Выполняем REST запрос в отдельном потоке, чтобы не мешать другим методам
		 */
		thread thr([&result, this](const uri_t::url_t & url, const unordered_map <string, string> & headers, const string & body){
			/*
			// Выполняем REST запрос
			const auto & response = this->REST(url, EVHTTP_REQ_PUT, headers, body);
			// Проверяем на наличие ошибок
			if(!response.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, response.code, response.mess.c_str());
			// Если тело ответа получено
			if(!response.body.empty()) result = response.body;
			*/
		}, ref(url), ref(headers), ref(body));
		// Ожидаем завершение запроса
		thr.join();
	}
	// Выводим результат
	return result;
}
/**
 * PUT Метод REST запроса
 * @param url     адрес запроса
 * @param body    тело запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const string awh::Rest::PUT(const uri_t::url_t & url, const unordered_multimap <string, string> & body, const unordered_map <string, string> & headers) noexcept {
	// Результат работы функции
	string result = "";
	// Если URL адрес передан
	if(!url.empty() && !body.empty()){
		/**
		 * Выполняем REST запрос в отдельном потоке, чтобы не мешать другим методам
		 */
		thread thr([&result, this](const uri_t::url_t & url, const unordered_map <string, string> & headers, const unordered_multimap <string, string> & body){
			/*
			// Результирующая строка
			string bodyData = "";
			// Переходим по всему списку тела запроса
			for(auto & param: body){
				// Есди данные уже набраны
				if(!bodyData.empty()) bodyData.append("&");
				// Добавляем в список набор параметров
				bodyData.append(this->uri->urlEncode(param.first));
				// Добавляем разделитель
				bodyData.append("=");
				// Добавляем значение
				bodyData.append(this->uri->urlEncode(param.second));
			}
			// Выполняем REST запрос
			const auto & response = this->REST(url, EVHTTP_REQ_PUT, headers, bodyData);
			// Проверяем на наличие ошибок
			if(!response.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, response.code, response.mess.c_str());
			// Если тело ответа получено
			if(!response.body.empty()) result = response.body;
			*/
		}, ref(url), ref(headers), ref(body));
		// Ожидаем завершение запроса
		thr.join();
	}
	// Выводим результат
	return result;
}
/**
 * POST Метод HTTP запроса в формате JSON
 * @param url     адрес запроса
 * @param body    тело запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const string awh::Rest::POST(const uri_t::url_t & url, const json & body, const unordered_map <string, string> & headers) noexcept {
	// Результат работы функции
	string result = "";
	// Если URL адрес передан
	if(!url.empty() && !body.empty()){
		// Добавляем заголовок типа контента
		const_cast <unordered_map <string, string> *> (&headers)->emplace("Content-Type", "application/json");
		/**
		 * Выполняем REST запрос в отдельном потоке, чтобы не мешать другим методам
		 */
		thread thr([&result, this](const uri_t::url_t & url, const unordered_map <string, string> & headers, const json & body){
			/*
			// Результирующая строка
			const string bodyData = body.dump();
			// Выполняем REST запрос
			const auto & response = this->REST(url, EVHTTP_REQ_POST, headers, bodyData);
			// Проверяем на наличие ошибок
			if(!response.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, response.code, response.mess.c_str());
			// Если тело ответа получено
			if(!response.body.empty()) result = response.body;
			*/
		}, ref(url), ref(headers), ref(body));
		// Ожидаем завершение запроса
		thr.join();
	}
	// Выводим результат
	return result;
}
/**
 * POST Метод REST запроса в формате JSON
 * @param url     адрес запроса
 * @param body    тело запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const string awh::Rest::POST(const uri_t::url_t & url, const string & body, const unordered_map <string, string> & headers) noexcept {
	// Результат работы функции
	string result = "";
	// Если URL адрес передан
	if(!url.empty()){
		/**
		 * Выполняем REST запрос в отдельном потоке, чтобы не мешать другим методам
		 */
		thread thr([&result, this](const uri_t::url_t & url, const unordered_map <string, string> & headers, const string & body){
			/*
			// Выполняем REST запрос
			const auto & response = this->REST(url, EVHTTP_REQ_POST, headers, body);
			// Проверяем на наличие ошибок
			if(!response.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, response.code, response.mess.c_str());
			// Если тело ответа получено
			if(!response.body.empty()) result = response.body;
			*/
		}, ref(url), ref(headers), ref(body));
		// Ожидаем завершение запроса
		thr.join();
	}
	// Выводим результат
	return result;
}
/**
 * POST Метод REST запроса
 * @param url     адрес запроса
 * @param body    тело запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const string awh::Rest::POST(const uri_t::url_t & url, const unordered_multimap <string, string> & body, const unordered_map <string, string> & headers) noexcept {
	// Результат работы функции
	string result = "";
	// Если URL адрес передан
	if(!url.empty() && !body.empty()){
		/**
		 * Выполняем REST запрос в отдельном потоке, чтобы не мешать другим методам
		 */
		thread thr([&result, this](const uri_t::url_t & url, const unordered_map <string, string> & headers, const unordered_multimap <string, string> & body){
			/*
			// Результирующая строка
			string bodyData = "";
			// Переходим по всему списку тела запроса
			for(auto & param: body){
				// Есди данные уже набраны
				if(!bodyData.empty()) bodyData.append("&");
				// Добавляем в список набор параметров
				bodyData.append(this->uri->urlEncode(param.first));
				// Добавляем разделитель
				bodyData.append("=");
				// Добавляем значение
				bodyData.append(this->uri->urlEncode(param.second));
			}
			// Выполняем REST запрос
			const auto & response = this->REST(url, EVHTTP_REQ_POST, headers, bodyData);
			// Проверяем на наличие ошибок
			if(!response.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, response.code, response.mess.c_str());
			// Если тело ответа получено
			if(!response.body.empty()) result = response.body;
			*/
		}, ref(url), ref(headers), ref(body));
		// Ожидаем завершение запроса
		thr.join();
	}
	// Выводим результат
	return result;
}
/**
 * PATCH Метод REST запроса в формате JSON
 * @param url     адрес запроса
 * @param body    тело запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const string awh::Rest::PATCH(const uri_t::url_t & url, const json & body, const unordered_map <string, string> & headers) noexcept {
	// Результат работы функции
	string result = "";
	// Если URL адрес передан
	if(!url.empty()){
		// Добавляем заголовок типа контента
		const_cast <unordered_map <string, string> *> (&headers)->emplace("Content-Type", "application/json");
		/**
		 * Выполняем REST запрос в отдельном потоке, чтобы не мешать другим методам
		 */
		thread thr([&result, this](const uri_t::url_t & url, const unordered_map <string, string> & headers, const json & body){
			/*
			// Результирующая строка
			const string bodyData = body.dump();
			// Выполняем REST запрос
			const auto & response = this->REST(url, EVHTTP_REQ_PATCH, headers, bodyData);
			// Проверяем на наличие ошибок
			if(!response.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, response.code, response.mess.c_str());
			// Если тело ответа получено
			if(!response.body.empty()) result = response.body;
			*/
		}, ref(url), ref(headers), ref(body));
		// Ожидаем завершение запроса
		thr.join();
	}
	// Выводим результат
	return result;
}
/**
 * PATCH Метод REST запроса в формате JSON
 * @param url     адрес запроса
 * @param body    тело запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const string awh::Rest::PATCH(const uri_t::url_t & url, const string & body, const unordered_map <string, string> & headers) noexcept {
	// Результат работы функции
	string result = "";
	// Если URL адрес передан
	if(!url.empty()){
		/**
		 * Выполняем REST запрос в отдельном потоке, чтобы не мешать другим методам
		 */
		thread thr([&result, this](const uri_t::url_t & url, const unordered_map <string, string> & headers, const string & body){
			/*
			// Выполняем REST запрос
			const auto & response = this->REST(url, EVHTTP_REQ_PATCH, headers, body);
			// Проверяем на наличие ошибок
			if(!response.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, response.code, response.mess.c_str());
			// Если тело ответа получено
			if(!response.body.empty()) result = response.body;
			*/
		}, ref(url), ref(headers), ref(body));
		// Ожидаем завершение запроса
		thr.join();
	}
	// Выводим результат
	return result;
}
/**
 * PATCH Метод REST запроса
 * @param url     адрес запроса
 * @param body    тело запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const string awh::Rest::PATCH(const uri_t::url_t & url, const unordered_multimap <string, string> & body, const unordered_map <string, string> & headers) noexcept {
	// Результат работы функции
	string result = "";
	// Если URL адрес передан
	if(!url.empty() && !body.empty()){
		/**
		 * Выполняем REST запрос в отдельном потоке, чтобы не мешать другим методам
		 */
		thread thr([&result, this](const uri_t::url_t & url, const unordered_map <string, string> & headers, const unordered_multimap <string, string> & body){
			/*
			// Результирующая строка
			string bodyData = "";
			// Переходим по всему списку тела запроса
			for(auto & param: body){
				// Есди данные уже набраны
				if(!bodyData.empty()) bodyData.append("&");
				// Добавляем в список набор параметров
				bodyData.append(this->uri->urlEncode(param.first));
				// Добавляем разделитель
				bodyData.append("=");
				// Добавляем значение
				bodyData.append(this->uri->urlEncode(param.second));
			}
			// Выполняем REST запрос
			const auto & response = this->REST(url, EVHTTP_REQ_PATCH, headers, bodyData);
			// Проверяем на наличие ошибок
			if(!response.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, response.code, response.mess.c_str());
			// Если тело ответа получено
			if(!response.body.empty()) result = response.body;
			*/
		}, ref(url), ref(headers), ref(body));
		// Ожидаем завершение запроса
		thr.join();
	}
	// Выводим результат
	return result;
}
/**
 * HEAD Метод REST запроса
 * @param url     адрес запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const unordered_map <string, string> awh::Rest::HEAD(const uri_t::url_t & url, const unordered_map <string, string> & headers) noexcept {
	// Результат работы функции
	unordered_map <string, string> result;
	// Если URL адрес передан
	if(!url.empty()){
		/**
		 * Выполняем REST запрос в отдельном потоке, чтобы не мешать другим методам
		 */
		thread thr([&result, this](const uri_t::url_t & url, const unordered_map <string, string> & headers){
			/*
			// Выполняем REST запрос
			const auto & response = this->REST(url, EVHTTP_REQ_HEAD, headers);
			// Проверяем на наличие ошибок
			if(!response.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, response.code, response.mess.c_str());
			// Если тело ответа получено
			if(!response.headers.empty()) result = response.headers;
			*/
		}, ref(url), ref(headers));
		// Ожидаем завершение запроса
		thr.join();
	}
	// Выводим результат
	return result;
}
/**
 * TRACE Метод REST запроса
 * @param url     адрес запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const unordered_map <string, string> awh::Rest::TRACE(const uri_t::url_t & url, const unordered_map <string, string> & headers) noexcept {
	// Результат работы функции
	unordered_map <string, string> result;
	// Если URL адрес передан
	if(!url.empty()){
		/**
		 * Выполняем REST запрос в отдельном потоке, чтобы не мешать другим методам
		 */
		thread thr([&result, this](const uri_t::url_t & url, const unordered_map <string, string> & headers){
			/*
			// Выполняем REST запрос
			const auto & response = this->REST(url, EVHTTP_REQ_TRACE, headers);
			// Проверяем на наличие ошибок
			if(!response.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, response.code, response.mess.c_str());
			// Если тело ответа получено
			if(!response.headers.empty()) result = response.headers;
			*/
		}, ref(url), ref(headers));
		// Ожидаем завершение запроса
		thr.join();
	}
	// Выводим результат
	return result;
}
/**
 * OPTIONS Метод REST запроса
 * @param url     адрес запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const unordered_map <string, string> awh::Rest::OPTIONS(const uri_t::url_t & url, const unordered_map <string, string> & headers) noexcept {
	// Результат работы функции
	unordered_map <string, string> result;
	// Если URL адрес передан
	if(!url.empty()){
		/**
		 * Выполняем REST запрос в отдельном потоке, чтобы не мешать другим методам
		 */
		thread thr([&result, this](const uri_t::url_t & url, const unordered_map <string, string> & headers){
			/*
			// Выполняем REST запрос
			const auto & response = this->REST(url, EVHTTP_REQ_OPTIONS, headers);
			// Проверяем на наличие ошибок
			if(!response.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, response.code, response.mess.c_str());
			// Если тело ответа получено
			if(!response.headers.empty()) result = response.headers;
			*/
		}, ref(url), ref(headers));
		// Ожидаем завершение запроса
		thr.join();
	}
	// Выводим результат
	return result;
}



/**
 * callback Функция вывода результата получения данных
 * @param req объект REST запроса
 * @param ctx контекст родительского объекта
 */
void awh::Rest::callback(struct evhttp_request * req, void * ctx) noexcept {
	// Если контекст объекта ответа сервера получен
	if(ctx != nullptr){

		cout << " ###################!! " << req << endl;

		// Создаём объект ответа сервера
		rest_t * http = reinterpret_cast <rest_t *> (ctx);
		/**
		 * Выполняем обработку ошибок
		 */
		try {
			// Буфер для получения данных
			char buffer[256];
			// Зануляем буфер данных
			memset(buffer, 0, sizeof(buffer));
			// Если параметры получены
			if((req != nullptr) && evhttp_request_get_response_code(req)){
				// Количество полученных данных
				size_t size = 0;
				// Получаем объект заголовков
				struct evkeyvalq * headers = evhttp_request_get_input_headers(req);
				// Получаем первый заголовок
				struct evkeyval * header = headers->tqh_first;
				// Перебираем все полученные заголовки
				while(header){
					// Собираем все доступные заголовки
					http->res.headers.emplace(http->fmk->toLower(header->key), header->value);
					// Переходим к следующему заголовку
					header = header->next.tqe_next;
				}
				// Получаем код ответа сервера
				http->res.code = evhttp_request_get_response_code(req);
				// Получаем текст ответа сервера
				http->res.mess = evhttp_request_get_response_code_line(req);
				// Считываем в буфер тело ответа сервера
				while((size = evbuffer_remove(evhttp_request_get_input_buffer(req), buffer, sizeof(buffer))) > 0){
					/**
					 * Получаем произвольные фрагменты по 256 байт.
					 * Это не строки, поэтому мы не можем получать их построчно.
					 */
					http->res.body.append(buffer, size);
				}
			// Если объект REST запроса не получен
			} else {
				// Ошибка на сервере
				size_t error = 0;
				// Флаг получения информации об ошибке
				bool mode = false;
				/**
				 * Если ответ не получен, это означает, что произошла ошибка,
				 * но, к сожалению, нам остается только догадываться, в чем могла быть ошибка.
				 */
				const int code = EVUTIL_SOCKET_ERROR();
				// Сообщаем, что на сервере произошла ошибкаа
				http->log->print("%s", log_t::flag_t::CRITICAL, "some request failed - no idea which one though!");
				/**
				 * Выводим очередь ошибок OpenSSL,
				 * которую libevent получил для нас, если такие имеются.
				 */
				while((error = bufferevent_get_openssl_error(http->evbuf.bev))){
					// Запоминаем, что описание ошибки получено
					mode = true;
					// Зануляем буфер данных
					memset(buffer, 0, sizeof(buffer));
					// Получаем описание ошибки
					ERR_error_string_n(error, buffer, sizeof(buffer));
					// Выводим ошибку
					http->log->print("%s", log_t::flag_t::CRITICAL, buffer);
				}
				/**
				 * Если очередь ошибок OpenSSL пустая, возможно,
				 * это была ошибка сокета, попробуем получить описание.
				 */
				if(!mode) http->log->print("socket error = %s (%d)", log_t::flag_t::CRITICAL, evutil_socket_error_to_string(code), code);
			}
			// Разблокируем базу событий
			event_base_loopbreak(bufferevent_get_base(http->evbuf.bev));
		// Если происходит ошибка то игнорируем её
		} catch(exception & error) {
			// Выводим сообщение об ошибке
			http->log->print("%s", log_t::flag_t::CRITICAL, error.what());
		}
	}
}

void awh::Rest::clear() noexcept {
	// Если база событий существует
	if(this->evbuf.base != nullptr)
		// Завершаем работу базы событий
		event_base_loopbreak(this->evbuf.base);
	// Если объект подключения существует
	if(this->evbuf.evcon != nullptr){
		// Удаляем событие подключения
		evhttp_connection_free(this->evbuf.evcon);
		// Зануляем объект подключения
		this->evbuf.evcon = nullptr;
	}
	// Если событие сервера существует
	if(this->evbuf.bev != nullptr){
		// Получаем файловый дескриптор
		const evutil_socket_t fd = bufferevent_getfd(this->evbuf.bev);
		// Если - это Windows
		#if defined(_WIN32) || defined(_WIN64)
			// Отключаем подключение для сокета
			if(fd > 0) shutdown(fd, SD_BOTH);
		// Если - это Unix
		#else
			// Отключаем подключение для сокета
			if(fd > 0) shutdown(fd, SHUT_RDWR);
		#endif
		// Запрещаем чтение запись данных серверу
		bufferevent_disable(this->evbuf.bev, EV_WRITE | EV_READ);
		// Закрываем подключение
		if(fd > 0) evutil_closesocket(fd);
		// Удаляем сокет буфера событий
		// bufferevent_setfd(this->evbuf.bev, -1);
		// Удаляем буфер события
		bufferevent_free(this->evbuf.bev);
		// Зануляем буфер событий
		this->evbuf.bev = nullptr;
	}
	// Если объект DNS существует
	if(this->evbuf.dns != nullptr){
		// Удаляем базу dns
		evdns_base_free(this->evbuf.dns, 0);
		// Зануляем базу dns
		this->evbuf.dns = nullptr;
	}
	// Выполняем удаление контекста SSL
	this->ssl->clear(this->sslctx);
}

void awh::Rest::makeHeaders(struct evhttp_request * req, const uri_t::url_t & url, const unordered_map <string, string> & headers, void * ctx) noexcept {
	// Если контекст модуля и объект запроса получены
	if((ctx != nullptr) && (req != nullptr)){
		// Список существующих заголовков
		set <header_t> availableHeaders;
		// Создаём объект контекст модуля
		rest_t * http = reinterpret_cast <rest_t *> (ctx);
		// Получаем объект заголовков
		struct evkeyvalq * store = evhttp_request_get_output_headers(req);
		// Устанавливаем параметры REST запроса
		const_cast <auth_t *> (http->auth)->setUri(http->uri->createUrl(url));
		// Переходим по всему списку заголовков
		for(auto & header : headers){
			// Получаем анализируемый заголовок
			const string & head = http->fmk->toLower(header.first);
			// Если заголовок Host передан, запоминаем , что мы его нашли
			if(head.compare("host") == 0) availableHeaders.emplace(header_t::HOST);
			// Если заголовок Accept передан, запоминаем , что мы его нашли
			if(head.compare("accept") == 0) availableHeaders.emplace(header_t::ACCEPT);
			// Если заголовок Origin перадан, запоминаем, что мы его нашли
			else if(head.compare("origin") == 0) availableHeaders.emplace(header_t::ORIGIN);
			// Если заголовок User-Agent передан, запоминаем, что мы его нашли
			else if(head.compare("user-agent") == 0) availableHeaders.emplace(header_t::USERAGENT);
			// Если заголовок Connection перадан, запоминаем, что мы его нашли
			else if(head.compare("connection") == 0) availableHeaders.emplace(header_t::CONNECTION);
			// Если заголовок Accept-Language передан, запоминаем, что мы его нашли
			else if(head.compare("accept-language") == 0) availableHeaders.emplace(header_t::ACCEPTLANGUAGE);
			// Добавляем заголовок в запрос
			evhttp_add_header(store, header.first.c_str(), header.second.c_str());
		}
		// Устанавливаем Host если не передан
		if(availableHeaders.count(header_t::HOST) < 1)
			// Устанавливаем заголовок запроса
			evhttp_add_header(store, "Host", (!url.domain.empty() ? url.domain : url.ip).c_str());
		// Устанавливаем Origin если не передан
		if(availableHeaders.count(header_t::ORIGIN) < 1)
			// Устанавливаем заголовок запроса
			evhttp_add_header(store, "Origin", http->uri->createOrigin(url).c_str());
		// Устанавливаем User-Agent если не передан
		if(availableHeaders.count(header_t::USERAGENT) < 1)
			// Устанавливаем заголовок запроса
			evhttp_add_header(store, "User-Agent", http->userAgent.c_str());
		// Устанавливаем Connection если не передан
		if(availableHeaders.count(header_t::CONNECTION) < 1)
			// Устанавливаем заголовок запроса
			evhttp_add_header(store, "Connection", "keep-alive");
		// Устанавливаем Accept-Language если не передан
		if(availableHeaders.count(header_t::ACCEPTLANGUAGE) < 1)
			// Устанавливаем заголовок запроса
			evhttp_add_header(store, "Accept-Language", "*");
		// Устанавливаем Accept если не передан
		if(availableHeaders.count(header_t::ACCEPT) < 1)
			// Устанавливаем заголовок запроса
			evhttp_add_header(store, "Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9");
		// Если нужно произвести сжатие контента
		if(http->zip != zip_t::NONE)
			// Устанавливаем заголовок запроса
			evhttp_add_header(store, "Accept-Encoding", "gzip, deflate");
		// Получаем заголовок авторизации
		const string & authHeader = http->auth->header();
		// Если данные авторизации получены
		if(!authHeader.empty()){
			// Получаем значение заголовка
			const_cast <string *> (&authHeader)->assign(authHeader.begin() + 15, authHeader.end() - 2);
			// Устанавливаем авторизационные параметры
			evhttp_add_header(store, "Authorization", authHeader.c_str());
		}
	}
}

void awh::Rest::makeBody(struct evhttp_request * req, const string & body, void * ctx) noexcept {
	// Если контекст модуля и объект запроса получены
	if((ctx != nullptr) && (req != nullptr) && !body.empty()){
		// Создаём объект контекст модуля
		rest_t * http = reinterpret_cast <rest_t *> (ctx);
		// Получаем буфер тела запроса
		struct evbuffer * buffer = evhttp_request_get_output_buffer(req);
		// Получаем объект заголовков
		struct evkeyvalq * store = evhttp_request_get_output_headers(req);
		// Если нужно производить шифрование
		if(http->crypt){
			// Выполняем шифрование переданных данных
			const auto & res = http->hash->encrypt(body.data(), body.size());
			// Если данные зашифрованы, заменяем тело данных
			if(!res.empty()){
				// Заменяем тело запроса на зашифрованное
				const_cast <string *> (&body)->assign(res.begin(), res.end());
				// Устанавливаем заголовок шифрования
				evhttp_add_header(store, "X-AWH-Encryption", to_string((u_int) http->hash->getAES()).c_str());
			}
		}
		// Определяем метод сжатия тела сообщения
		switch((u_short) http->zip){
			// Если сжимать тело не нужно
			case (u_short) zip_t::NONE:
				// Добавляем в буфер, тело запроса
				evbuffer_add(buffer, body.data(), body.size());
				// Если передавать тело запроса чанками не нужно, устанавливаем размер тела
				if(!http->chunked) evhttp_add_header(store, "Content-Length", to_string(body.size()).c_str());
			break;
			// Если нужно сжать тело методом GZIP
			case (u_short) zip_t::GZIP: {
				// Выполняем сжатие тела сообщения
				const auto & gzip = http->hash->compressGzip(body.data(), body.size());
				// Добавляем в буфер, тело запроса
				evbuffer_add(buffer, gzip.data(), gzip.size());
				// Указываем метод, которым было выполненно сжатие тела
				evhttp_add_header(store, "Content-Encoding", "gzip");
				// Если передавать тело запроса чанками не нужно, устанавливаем размер тела
				if(!http->chunked) evhttp_add_header(store, "Content-Length", to_string(gzip.size()).c_str());
			} break;
			// Если нужно сжать тело методом DEFLATE
			case (u_short) zip_t::DEFLATE: {
				// Выполняем сжатие тела сообщения
				auto deflate = http->hash->compress(body.data(), body.size());
				// Удаляем хвост в полученных данных
				http->hash->rmTail(deflate);
				// Добавляем в буфер, тело запроса
				evbuffer_add(buffer, deflate.data(), deflate.size());
				// Указываем метод, которым было выполненно сжатие тела
				evhttp_add_header(store, "Content-Encoding", "deflate");
				// Если передавать тело запроса чанками не нужно, устанавливаем размер тела
				if(!http->chunked) evhttp_add_header(store, "Content-Length", to_string(deflate.size()).c_str());
			} break;
		}
		// Если нужно передавать тело в виде чанков
		if(http->chunked) evhttp_add_header(store, "Transfer-Encoding", "chunked");
	}
}


typedef struct ProxyData {
	evhttp_cmd_type type;
	const string * body;
	const awh::uri_t::url_t * url;
	const unordered_map <string, string> * headers;
	awh::Rest::res_t * response;
} proxyData_t;

/**
 * write Метод записи данных в сокет сервера
 * @param bev буфер события
 * @param ctx передаваемый объект
 */
static void read(struct bufferevent * bev, void * ctx) noexcept {
	// Получаем буферы входящих данных
		struct evbuffer * input = bufferevent_get_input(bev);
		// Получаем размер входящих данных
		size_t size = evbuffer_get_length(input);

		char * buffer = new char[size];
		// Копируем в буфер полученные данные
		evbuffer_copyout(input, buffer, size);

		cout << " @@@@@@@@@@@1 " << string(buffer, size) << " == " << size << endl;

		delete [] buffer;
}


/**
 * write Метод записи данных в сокет сервера
 * @param bev буфер события
 * @param ctx передаваемый объект
 */
static void write(struct bufferevent * bev, void * ctx) noexcept {
	// Если подключение передано
	if(ctx != nullptr){
		// Получаем буферы входящих данных
		struct evbuffer * output = bufferevent_get_output(bev);
		// Получаем размер входящих данных
		size_t size = evbuffer_get_length(output);

		char * buffer = new char[size];
		// Копируем в буфер полученные данные
		evbuffer_copyout(output, buffer, size);

		cout << " @@@@@@@@@@@2 " << string(buffer, size) << " == " << size << endl;

		delete [] buffer;
	}
}

SSL *
ssl_setup_socket(int sock, SSL_CTX * ctx)
{
  SSL *ssl;
  BIO *bio;

  ssl = SSL_new(ctx);
  bio = BIO_new_socket(sock, BIO_NOCLOSE);
  BIO_set_nbio(bio, 1);
  SSL_set_bio(ssl, bio, bio);
  return ssl;
}



/**
 * proxy Функция вывода результата получения данных
 * @param req объект REST запроса
 * @param ctx контекст родительского объекта
 */
void awh::Rest::proxyFn(struct evhttp_request * req, void * ctx){
	

	rest_t * http = reinterpret_cast <rest_t *> (ctx);

	cout << " !!!!!!!!!1 " << req << endl;

	cout << " !!!!!!!!!!! " << evhttp_request_get_response_code(req) << endl;

	// Получаем объект заголовков
	struct evkeyvalq * headers = evhttp_request_get_input_headers(req);
	// Получаем первый заголовок
	struct evkeyval * header = headers->tqh_first;
	// Перебираем все полученные заголовки
	while(header){
		// Собираем все доступные заголовки
		cout << " **************!8 " << header->key << " == " << header->value << " == " << evhttp_request_get_response_code(req) << endl;
		// Переходим к следующему заголовку
		header = header->next.tqe_next;
	}

	/*
	// Если объект DNS существует
	if(http->dns != nullptr){
		// Удаляем базу dns
		evdns_base_free(http->dns, 0);
		// Зануляем базу dns
		http->dns = nullptr;
	}
	*/

	/*
	// Сокет подключения
	evutil_socket_t fd = -1;
	// Создаём DNS резолвер
	dns_t resolver(http->fmk, http->log, http->nwk);
	// Определяем тип сети
	switch(obj->url->family){
		// Если - это IPv4
		case AF_INET: {
			// Добавляем список серверов в резолвер
			resolver.setNameServers(http->net.v4.second);
			// Создаём базу событий DNS
			http->dns = resolver.init(obj->url->domain, AF_INET, http->base);
			// Создаем сокет подключения
			fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		} break;
		// Если - это IPv6
		case AF_INET6: {
			// Добавляем список серверов в резолвер
			resolver.setNameServers(http->net.v6.second);
			// Создаём базу событий DNS
			http->dns = resolver.init(obj->url->domain, AF_INET6, http->base);
			// Создаем сокет подключения
			fd = ::socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
		} break;
	}
	*/

	/*
	evutil_socket_t fd = bufferevent_getfd(http->bev);

	bufferevent_setfd(http->bev, -1);
	bufferevent_free(http->bev);
	*/
	

	// Выполняем получение контекста сертификата
	// http->sslctx = http->ssl->init(* obj->url);
	// bufferevent_ssl_renegotiate(http->bev);

	// http->bev = bufferevent_openssl_socket_new(http->base, fd, http->sslctx2.ssl, BUFFEREVENT_SSL_CONNECTING, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);

	// http->bev2 = bufferevent_openssl_socket_new(http->base, fd, http->sslctx.ssl, BUFFEREVENT_SSL_CONNECTING, BEV_OPT_THREADSAFE | BEV_OPT_DEFER_CALLBACKS);
	
	
	// Получаем файловый дескриптор
	// struct evbuffer * output = evhttp_request_get_output_buffer(req);
	// struct bufferevent * bev = evhttp_connection_get_bufferevent(struct evhttp_connection *evcon)
	// const evutil_socket_t fd = bufferevent_getfd(http->bev);

	// auto bev3 = bufferevent_socket_new(http->base, fd, BEV_OPT_THREADSAFE | BEV_OPT_DEFER_CALLBACKS);

	

	// auto evcon = evhttp_request_get_connection(req);

	// bufferevent * bev = evhttp_connection_get_bufferevent(evcon);

	// bufferevent_ssl_renegotiate(http->bev);

	// http->bev2 = bufferevent_socket_new(http->base, fd, BEV_OPT_THREADSAFE | BEV_OPT_DEFER_CALLBACKS);
	// http->bev = bufferevent_openssl_filter_new(http->base, http->bev, http->sslctx.ssl, BUFFEREVENT_SSL_OPEN, BEV_OPT_DEFER_CALLBACKS);



	// bufferevent_ssl_renegotiate(http->bev);


	// Устанавливаем коллбеки
	// bufferevent_setcb(http->bev, &read, &write, nullptr, http);
	

	/*
	// Выполняем получение контекста сертификата
	http->sslctx = http->ssl->init(* obj->url);

	cout << " --------- " << http->sslctx.ctx << endl;

	// Создаем ssl объект
	// SSL * ssl = SSL_new(http->sslctx.ctx);
	*/

	// evutil_socket_t fd = bufferevent_getfd(http->bev);

	// SSL * ssl = ssl_setup_socket(fd, http->sslctx.ctx);
	

	//if(ssl){
		
		/*
		auto flags = fcntl(fd, F_GETFL, 0);
		fcntl(fd, F_SETFL, true ? (flags | O_NONBLOCK) : (flags & (~O_NONBLOCK)));

 		// set_nonblocking(sock, true);
		auto bio = BIO_new_socket(static_cast <int> (fd), BIO_NOCLOSE);
		BIO_set_nbio(bio, 1);
		SSL_set_bio(ssl, bio, bio);

		//
		if (!setup(http->sslctx.ssl) || SSL_connect_or_accept(http->sslctx.ssl) != 1) {
			SSL_shutdown(http->sslctx.ssl);
			SSL_free(http->sslctx.ssl);
			return;
		}
		//
		BIO_set_nbio(bio, 0);
		*/

		// SSL_accept(ssl);

		/*

		cout << " !!!!!!!!1 " << fd << endl;

		SSL_set_verify(ssl, SSL_VERIFY_NONE, nullptr);

		const long verify = SSL_get_verify_result(ssl);

		cout << " !!!!!!!!2 " << endl;

		if(verify != X509_V_OK) return;

		auto server_cert = SSL_get_peer_certificate(ssl);

		cout << " !!!!!!!!3 " << server_cert << endl;

		// if(server_cert == nullptr) return;

		cout << " !!!!!!!!4 " << endl;

		// X509_free(server_cert);

		cout << " !!!!!!!!5 " << endl;

		SSL_set_tlsext_host_name(ssl, "2ip.ru");

		cout << " !!!!!!!!6 " << endl;

		// Проверяем рукопожатие
		if(SSL_do_handshake(ssl) <= 0){

			cout << " +++++++++++ SSL2 " << endl;

			// Выполняем проверку рукопожатия
			const long verify = SSL_get_verify_result(ssl);
			// Если рукопожатие не выполнено
			if(verify != X509_V_OK) cout << " +++++++++++ SSL3 " << endl;

		}

		flags = fcntl(fd, F_GETFL, 0);
		fcntl(fd, F_SETFL, false ? (flags | O_NONBLOCK) : (flags & (~O_NONBLOCK)));

		// bufferevent_setfd(http->bev, fd);

		// Разрешаем грязное отключение
		// bufferevent_openssl_set_allow_dirty_shutdown(http->bev, 1);
		*/
	// }

	// Активируем разрешение на запись и чтение
	/// bufferevent_enable(http->bev, EV_WRITE | EV_READ);
	// Отправляем серверу сообщение
	// bufferevent_write(http->bev, request.c_str(), request.size());

	// ::write(fd, request.data(), request.size());


	/*
	// Выполняем получение контекста сертификата
	http->sslctx = http->ssl->init(* obj->url);
	// Если защищённое соединение не нужно, создаём буфер событий
	if(http->sslctx.mode){
		evutil_socket_t fd = bufferevent_getfd(http->bev);
		// Создаём буфер событий в защищённом подключении
		http->bev = bufferevent_openssl_socket_new(http->base, fd, http->sslctx.ssl, BUFFEREVENT_SSL_CONNECTING, BEV_OPT_DEFER_CALLBACKS);
		// Разрешаем грязное отключение
		bufferevent_openssl_set_allow_dirty_shutdown(http->bev, 1);
	}
	// Если буфер событий не создан
	if(http->bev == nullptr){
		// Очищаем контекст
		http->clear();
		// Сообщаем, что буфер событий не может быть создан
		http->log->print("%s", log_t::flag_t::CRITICAL, "the event buffer could not be created");
		// Завершаем работу функции
		return;
	}
	*/

	/*
	// Создаём событие подключения
	http->evcon2 = evhttp_connection_base_bufferevent_new(http->base, http->dns, http->bev, (!obj->url->ip.empty() ? obj->url->ip.c_str() : obj->url->domain.c_str()), obj->url->port);
	// Если событие подключения не создан
	if(http->evcon2 == nullptr){
		// Очищаем контекст
		http->clear();
		// Сообщаем, что событие подключения не создано
		http->log->print("%s", log_t::flag_t::CRITICAL, "connection event not created");
		// Завершаем работу функции
		return;
	}
	// Выполняем 5 попыток запросить данные
	evhttp_connection_set_retries(http->evcon2, 5);
	// Таймаут на выполнение в 5 секунд
	evhttp_connection_set_timeout(http->evcon2, 5);
	// Заставляем выполнять подключение по указанному протоколу сети
	evhttp_connection_set_family(http->evcon2, obj->url->family);
	*/

	/*
	// Создаём объект выполнения REST запроса
	http->req2 = evhttp_request_new(callback, obj->response);
	// Если объект REST запроса не создан
	if(http->req2 == nullptr){
		// Очищаем контекст
		http->clear();
		// Сообщаем, что событие REST запроса не создано
		http->log->print("%s", log_t::flag_t::CRITICAL, "REST request event is not created");
		// Завершаем работу функции
		return;
	}
	*/

	
	// req = evhttp_request_new(callback, NULL);

	/*
	// Получаем объект заголовков
	struct evkeyvalq * store = evhttp_request_get_output_headers(req);

	evhttp_add_header(store, "Connection", "close");
	evhttp_add_header(store, "Host", (!obj->url->domain.empty() ? obj->url->domain : obj->url->ip).c_str());
	*/

	// Устанавливаем коллбеки
	// bufferevent_setcb(http->bev, &read, &write, nullptr, http);
	// Активируем разрешение на запись и чтение
	// bufferevent_enable(http->bev, EV_WRITE | EV_READ);

	/*
	const string request = "GET https://anyks.com HTTP/1.1\r\n"
							"Connection: close\r\n"
							"Host: anyks.com\r\n\r\n";
	
	
	// Отправляем серверу сообщение
	bufferevent_write(http->bev, request.data(), request.size());
	*/
	/*
	struct evbuffer * output = evhttp_request_get_output_buffer(req);

	evbuffer_add_printf(output, "%s", request.c_str());

	// Получаем размер входящих данных
	size_t size = evbuffer_get_length(output);

	char * buffer = new char[size];
	// Копируем в буфер полученные данные
	evbuffer_copyout(output, buffer, size);

	cout << " @@@@@@@@@@@1 " << string(buffer, size) << endl;

	

	evbuffer_write(output, bufferevent_getfd(http->bev));

	delete [] buffer;
	*/

	// evbuffer_add_vprintf (struct evbuffer *buf, const char *fmt, va_list ap)

	// evhttp_make_request(http->evcon, req, EVHTTP_REQ_GET, http->uri->createUrl(* obj->url).c_str());


	/*
	cout << " ^^^^^^^^^^^^^^^^^^ " << http->uri->createUrl(* obj->url) << endl;

	makeHeaders(http->req2, * obj->url, * obj->headers, (void *) http);

	makeBody(http->req2, * obj->body, (void *) http);
	*/

	/*
	// Создаём объект выполнения REST запроса
	req = evhttp_request_new(callback, http);
	// Если объект REST запроса не создан
	if(req == nullptr){
		// Очищаем контекст
		http->clear();
		// Сообщаем, что событие REST запроса не создано
		http->log->print("%s", log_t::flag_t::CRITICAL, "REST request event is not created");
		// Завершаем работу функции
		return;
	}

	// Получаем объект заголовков
	struct evkeyvalq * store = evhttp_request_get_output_headers(req);
	// Устанавливаем режим подключения (длительное подключение)
	evhttp_add_header(store, "Connection", "keep-alive");
	// Устанавливаем хост запрашиваемого удалённого сервера
	evhttp_add_header(store, "Host", "2ip.ru");

	// Выполняем получение контекста сертификата
	http->sslctx = http->ssl->init(* http->req.uri);


	evutil_socket_t fd = bufferevent_getfd(http->evbuf.bev);

	auto bio = BIO_new_socket(static_cast <int> (fd), BIO_NOCLOSE);
	BIO_set_nbio(bio, 1);
	SSL_set_bio(http->sslctx.ssl, bio, bio);
	BIO_set_nbio(bio, 0);

	// SSL_accept(ssl);


	// Выполняем REST запрос на сервер
	const int request = evhttp_make_request(http->evbuf.evcon, req, EVHTTP_REQ_GET, "/");
	// Если запрос не выполнен
	if(request != 0){
		// Очищаем контекст
		http->clear();
		// Сообщаем, что запрос не выполнен
		http->log->print("%s", log_t::flag_t::CRITICAL, "REST request failed");
		// Завершаем работу функции
		return;
	}
	*/

	// Разблокируем базу событий
	// event_base_loopbreak(bufferevent_get_base(http->bev));
	// 
	// 
	

	// Выполняем получение контекста сертификата
	http->sslctx = http->ssl->init(* http->req.uri);

	struct bufferevent * bev = evhttp_connection_get_bufferevent(http->evbuf.evcon);

	evutil_socket_t fd = bufferevent_getfd(bev);

	http->evbuf.bev = bufferevent_openssl_socket_new(http->evbuf.base, fd, http->sslctx.ssl, BUFFEREVENT_SSL_CONNECTING, BEV_OPT_THREADSAFE | BEV_OPT_DEFER_CALLBACKS);
	// Оборачиваем текущее соединение в SSL BIO
	// http->evbuf.bev = bufferevent_openssl_filter_new(http->evbuf.base, bev, http->sslctx.ssl, BUFFEREVENT_SSL_CONNECTING, BEV_OPT_THREADSAFE | BEV_OPT_DEFER_CALLBACKS);
	// Если буфер событий не создан
	if(http->evbuf.bev == nullptr){
		// Очищаем контекст
		http->clear();
		// Сообщаем, что буфер событий не может быть создан
		http->log->print("%s", log_t::flag_t::CRITICAL, "the event buffer could not be created");
		// Завершаем работу функции
		return;
	}
	// Разрешаем пересматривать подключение
	// bufferevent_ssl_renegotiate(http->evbuf.bev);


	// Создаём объект выполнения REST запроса
	struct evhttp_request * req2 = evhttp_request_new(callback, http);
	// Если объект REST запроса не создан
	if(req2 == nullptr){
		// Очищаем контекст
		http->clear();
		// Сообщаем, что событие REST запроса не создано
		http->log->print("%s", log_t::flag_t::CRITICAL, "REST request event is not created");
		// Завершаем работу функции
		return;
	}

	/*
	// Получаем объект заголовков
	struct evkeyvalq * store = evhttp_request_get_output_headers(req2);

	evhttp_add_header(store, "Connection", "close");
	evhttp_add_header(store, "Host", "2ip.ru");
	evhttp_add_header(store, "Accept", "*//*");
	evhttp_add_header(store, "User-Agent", "curl/7.64.1");

	evhttp_make_request(http->evbuf.evcon, req2, EVHTTP_REQ_GET, "/");
	*/

	http->flg = true;




	string request = "GET / HTTP/1.1\r\n"
		"Connection: close\r\n"
		"User-Agent: curl/7.64.1\r\n"
		"Accept: */*\r\n"
		"Host: 2ip.ru\r\n\r\n";

	// Если запрос получен
	if(!request.empty()){

		// Устанавливаем коллбеки
		bufferevent_setcb(http->evbuf.bev, &readProxy, nullptr, &eventProxy, http);
		// Очищаем буферы событий при завершении работы
		bufferevent_flush(http->evbuf.bev, EV_READ | EV_WRITE, BEV_FINISHED);
		// Активируем буферы событий на чтение и запись
		bufferevent_enable(http->evbuf.bev, EV_READ | EV_WRITE);

		// Активируем разрешение на запись и чтение
		bufferevent_enable(http->evbuf.bev, EV_WRITE | EV_READ);
		// Отправляем серверу сообщение
		bufferevent_write(http->evbuf.bev, request.data(), request.size());
	}

};



/**
 * PROXY Метод выполнения REST запроса на сервер через прокси-сервер
 * @return результат REST запроса
 */
void awh::Rest::PROXY2() noexcept {
	try {
		/**
		 * runFn Функция выполнения запуска системы
		 * @param ip полученный адрес сервера резолвером
		 */
		auto runFn = [this](const string & ip) noexcept {
			// Если IP адрес получен
			if(!ip.empty()){
				// Запоминаем IP адрес
				this->proxyUrl.ip = ip;
				// Если подключение к прокси-серверу выполнено
				if(this->connectProxy()) return;
				// Сообщаем, что подключение не удалось и выводим сообщение
				else this->log->print("broken connect to proxy host %s", log_t::flag_t::CRITICAL, this->proxyUrl.ip.c_str());
			}
			// Останавливаем работу
			this->clear();
		};
		// Создаем новую базу
		this->evbuf.base = event_base_new();
		// Определяем тип подключения
		switch(this->net.family){
			// Резолвер IPv4, создаём резолвер
			case AF_INET: this->dns = new dns_t(this->fmk, this->log, this->nwk, this->evbuf.base, this->net.v4.second); break;
			// Резолвер IPv6, создаём резолвер
			case AF_INET6: this->dns = new dns_t(this->fmk, this->log, this->nwk, this->evbuf.base, this->net.v6.second); break;
		}
		// Если IP адрес не получен
		if(this->proxyUrl.ip.empty() && !this->proxyUrl.domain.empty())
			// Выполняем резолвинг домена
			this->resolve(this->proxyUrl, runFn);
		// Выполняем запуск системы
		else if(!this->proxyUrl.ip.empty()) runFn(this->proxyUrl.ip);
		// Активируем перебор базы событий
		event_base_loop(this->evbuf.base, EVLOOP_NO_EXIT_ON_EMPTY);
		// Если DNS сервер ещё не удалён
		if(this->dns != nullptr){
			// Удаляем dns резолвер
			delete this->dns;
			// Зануляем DNS объект
			this->dns = nullptr;
		}
		// Если база событий существует
		if(this->evbuf.base != nullptr){
			// Удаляем объект базы событий
			event_base_free(this->evbuf.base);
			// Зануляем базу событий
			this->evbuf.base = nullptr;
		}
		// Очищаем все глобальные переменные
		libevent_global_shutdown();
		// Останавливаем работу
		this->clear();
	// Если происходит ошибка то игнорируем её
	} catch(const bad_alloc&) {
		// Останавливаем работу
		this->clear();
		// Выводим сообщение об ошибке
		this->log->print("%s", log_t::flag_t::CRITICAL, "memory could not be allocated");
	}
}



/**
 * PROXY Метод выполнения REST запроса на сервер через прокси-сервер
 */
void awh::Rest::PROXY() noexcept {
	// Если URL адрес получен
	if(!this->req.uri->empty()){
		try {
			// Создаём базу событий
			this->evbuf.base = event_base_new();
			// Если база событий создана
			if(this->evbuf.base != nullptr){

				cout << " _____________ " << this->proxyUrl.domain << " == " << this->proxyUrl.ip << " === " << this->proxyUrl.port << endl;

				// Сокет подключения
				evutil_socket_t fd = -1;
				// Создаём DNS резолвер
				dns_t resolver(this->fmk, this->log, this->nwk);
				// Определяем тип сети
				switch(this->proxyUrl.family){
					// Если - это IPv4
					case AF_INET: {
						// Добавляем список серверов в резолвер
						resolver.setNameServers(this->net.v4.second);
						// Создаём базу событий DNS
						this->evbuf.dns = resolver.init((!this->proxyUrl.domain.empty() ? this->proxyUrl.domain : this->proxyUrl.ip), AF_INET, this->evbuf.base);
						// Создаем сокет подключения
						fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
					} break;
					// Если - это IPv6
					case AF_INET6: {
						// Добавляем список серверов в резолвер
						resolver.setNameServers(this->net.v6.second);
						// Создаём базу событий DNS
						this->evbuf.dns = resolver.init((!this->proxyUrl.domain.empty() ? this->proxyUrl.domain : this->proxyUrl.ip), AF_INET6, this->evbuf.base);
						// Создаем сокет подключения
						fd = ::socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
					} break;
				}
				// Если файловый дескриптор создан
				if(fd > -1){
					/*
					// Если - это Unix
					#if !defined(_WIN32) && !defined(_WIN64)
						// Выполняем игнорирование сигнала неверной инструкции процессора
						sockets_t::noSigill(this->log);
						// Устанавливаем разрешение на повторное использование сокета
						sockets_t::reuseable(fd, this->log);
						// Отключаем сигнал записи в оборванное подключение
						sockets_t::noSigpipe(fd, this->log);
						// Отключаем алгоритм Нейгла для сервера и клиента
						sockets_t::tcpNodelay(fd, this->log);
						// Разблокируем сокет
						sockets_t::nonBlocking(fd, this->log);
						// Активируем keepalive
						sockets_t::keepAlive(fd, this->alive.keepcnt, this->alive.keepidle, this->alive.keepintvl, this->log);
					// Если - это Windows
					#else
						// Выполняем инициализацию WinSock
						this->winSocketInit();
						// Переводим сокет в блокирующий режим
						// sockets_t::blocking(fd);
						evutil_make_socket_nonblocking(fd);
						// evutil_make_socket_closeonexec(fd);
						evutil_make_listen_socket_reuseable(fd);
					#endif
					*/
				} else return;


				this->evbuf.bev = bufferevent_socket_new(this->evbuf.base, fd, BEV_OPT_THREADSAFE | BEV_OPT_DEFER_CALLBACKS);

				/*
				// Выполняем получение контекста сертификата
				// this->sslctx = this->ssl->init(this->proxyUrl);
				this->sslctx = this->ssl->init(url);
				// Если защищённое соединение не нужно, создаём буфер событий
				if(!this->sslctx.mode) this->bev = bufferevent_socket_new(this->base, fd, BEV_OPT_THREADSAFE | BEV_OPT_DEFER_CALLBACKS);
				// Если требуется защищённое соединение
				else {
					// Создаём буфер событий в защищённом подключении
					this->bev = bufferevent_openssl_socket_new(this->base, fd, this->sslctx.ssl, BUFFEREVENT_SSL_OPEN, BEV_OPT_THREADSAFE | BEV_OPT_DEFER_CALLBACKS);
					// Разрешаем грязное отключение
					bufferevent_openssl_set_allow_dirty_shutdown(this->bev, 1);

					cout << " $$$$$$$$$$$$$$ " << this->bev << endl;
				}
				*/
				/*
				// Если буфер событий не создан
				if(this->bev == nullptr){
					// Очищаем контекст
					this->clear();
					// Сообщаем, что буфер событий не может быть создан
					this->log->print("%s", log_t::flag_t::CRITICAL, "the event buffer could not be created");
					// Завершаем работу функции
					return result;
				}
				// Создаём событие подключения
				this->evcon = evhttp_connection_base_bufferevent_new(this->base, this->dns, this->bev, (!this->proxyUrl.ip.empty() ? this->proxyUrl.ip.c_str() : this->proxyUrl.domain.c_str()), this->proxyUrl.port);

				// this->evcon = evhttp_connection_base_new(this->base, nullptr, this->proxyUrl.ip.c_str(), this->proxyUrl.port);

				// Если событие подключения не создан
				if(this->evcon == nullptr){
					// Очищаем контекст
					this->clear();
					// Сообщаем, что событие подключения не создано
					this->log->print("%s", log_t::flag_t::CRITICAL, "connection event not created");
					// Завершаем работу функции
					return result;
				}
				*/
				// Выполняем 5 попыток запросить данные
				// evhttp_connection_set_retries(this->evcon, 5);
				// Таймаут на выполнение в 5 секунд
				// evhttp_connection_set_timeout(this->evcon, 5);
				// Заставляем выполнять подключение по указанному протоколу сети
				// evhttp_connection_set_family(this->evcon, this->proxyUrl.family);



				// Создаём объект выполнения REST запроса
				auto req = evhttp_request_new(proxyFn, this);
				// Если объект REST запроса не создан
				if(req == nullptr){
					// Очищаем контекст
					this->clear();
					// Сообщаем, что событие REST запроса не создано
					this->log->print("%s", log_t::flag_t::CRITICAL, "proxy request event is not created");
					// Завершаем работу функции
					return;
				}
				// Получаем объект заголовков
				struct evkeyvalq * store = evhttp_request_get_output_headers(req);
				// Устанавливаем режим подключения (длительное подключение)
				evhttp_add_header(store, "Connection", "keep-alive");
				// Устанавливаем режим подключения к прокси-серверу (длительное подключение)
				evhttp_add_header(store, "Proxy-Connection", "keep-alive");
				// Устанавливаем хост запрашиваемого удалённого сервера
				// evhttp_add_header(store, "Host", (!url.domain.empty() ? url.domain : url.ip).c_str());
				evhttp_add_header(store, "Host", "2ip.ru:443");



				// Создаём объект для работы с авторизацией
				auth_t auth(this->fmk, this->log);

				// Устанавливаем логин пользователя
				auth.setLogin(this->proxyUrl.user);
				// Устанавливаем пароль пользователя
				auth.setPassword(this->proxyUrl.pass);

				auth.setType(auth_t::type_t::BASIC);

				// Получаем заголовок авторизации
				const string & authHeader = auth.header();

				// cout << " ============= " << this->proxyUrl.user << " == " << this->proxyUrl.pass << " == " << authHeader << endl;

				// Если данные авторизации получены
				if(!authHeader.empty()){
					// Получаем значение заголовка
					const_cast <string *> (&authHeader)->assign(authHeader.begin() + 15, authHeader.end() - 2);
					// Устанавливаем авторизационные параметры
					evhttp_add_header(store, "Proxy-Authorization", authHeader.c_str());
				}




				// Получаем первый заголовок
				struct evkeyval * header = store->tqh_first;
				// Перебираем все полученные заголовки
				while(header){
					// Собираем все доступные заголовки
					cout << " **************11 " << header->key << " == " << header->value << endl;
					// Переходим к следующему заголовку
					header = header->next.tqe_next;
				}

				// cout << " **************12 " << this->uri->createUrl(url) << endl;

				
				

				// Выполняем подключение к прокси-серверу
				// int request = evhttp_make_request(this->evcon, this->req, EVHTTP_REQ_CONNECT, "2ip.ru:443"); //this->uri->createUrl(url).c_str());
				
				/*
				string request = "CONNECT 2ip.ru:443 HTTP/1.1\r\n"
							"Connection: keep-alive\r\n"
							"Proxy-Connection: keep-alive\r\n"
							"Host: 2ip.ru\r\n";
				request.append(this->fmk->format("Proxy-Authorization: %s\r\n\r\n", authHeader.c_str()));
				*/
				
				// Устанавливаем коллбеки
				// bufferevent_setcb(this->bev, &read, &write, nullptr, this);

				this->evbuf.evcon = evhttp_connection_base_bufferevent_new(this->evbuf.base, this->evbuf.dns, this->evbuf.bev, (!this->proxyUrl.ip.empty() ? this->proxyUrl.ip.c_str() : this->proxyUrl.domain.c_str()), this->proxyUrl.port);

				evhttp_make_request(this->evbuf.evcon, req, EVHTTP_REQ_CONNECT, "2ip.ru:443");

				
				// Активируем разрешение на запись и чтение
				// bufferevent_enable(this->bev, EV_WRITE | EV_READ);
				// Отправляем серверу сообщение
				// bufferevent_write(this->bev, request.c_str(), request.size());
				// Блокируем базу событий
				event_base_dispatch(this->evbuf.base);

				/*
				const string request11 = "GET / HTTP/1.1\r\n"
							"Connection: close\r\n"
							"Host: 2ip.ru\r\n\r\n";
				 */

				/*
				// Если запрос не выполнен
				if(request != 0){
					// Очищаем контекст
					this->clear();
					// Сообщаем, что запрос не выполнен
					this->log->print("%s", log_t::flag_t::CRITICAL, "PROXY request failed");
					// Завершаем работу функции
					return result;
				}

				// Блокируем базу событий
				event_base_dispatch(this->base);


				// Создаём объект выполнения REST запроса
				auto req = evhttp_request_new(proxyFn, &proxyData);
				// Если объект REST запроса не создан
				if(req == nullptr){
					// Очищаем контекст
					this->clear();
					// Сообщаем, что событие REST запроса не создано
					this->log->print("%s", log_t::flag_t::CRITICAL, "proxy request event is not created");
					// Завершаем работу функции
					return result;
				}

				// Получаем объект заголовков
				store = evhttp_request_get_output_headers(req);
				// Устанавливаем режим подключения (длительное подключение)
				evhttp_add_header(store, "Connection", "keep-alive");
				// Устанавливаем хост запрашиваемого удалённого сервера
				// evhttp_add_header(store, "Host", (!url.domain.empty() ? url.domain : url.ip).c_str());
				evhttp_add_header(store, "Host", "2ip.ru");
				
				// Выполняем подключение к прокси-серверу
				request = evhttp_make_request(this->evcon, req, EVHTTP_REQ_GET, "/");
				// Если запрос не выполнен
				if(request != 0){
					// Очищаем контекст
					this->clear();
					// Сообщаем, что запрос не выполнен
					this->log->print("%s", log_t::flag_t::CRITICAL, "PROXY request failed");
					// Завершаем работу функции
					return result;
				}


				// Отправляем серверу сообщение
				// bufferevent_write(this->bev, request11.c_str(), request11.size());

				// Блокируем базу событий
				event_base_dispatch(this->base);
				*/

			// Выводим сообщение в лог
			} else this->log->print("%s", log_t::flag_t::CRITICAL, "the event base could not be created");

			// Очищаем контекст
			this->clear();
			// Если код ответа не требует авторизации, разрешаем дальнейшие попытки
			if(this->res.code != 401) this->checkAuth = false;
			// Определяем, был ли ответ сервера удачным
			this->res.ok = ((this->res.code >= 200) && (this->res.code <= 206) || (this->res.code == 100));
			// Если запрос выполнен успешно
			if(this->res.ok){
				// Проверяем пришли ли сжатые данные
				auto it = this->res.headers.find("content-encoding");
				// Если данные пришли зашифрованные
				if(it != this->res.headers.end()){
					// Если данные пришли сжатые методом GZIP
					if(it->second.compare("gzip") == 0){
						// Выполняем декомпрессию данных
						const auto & body = this->hash->decompressGzip(this->res.body.data(), this->res.body.size());
						// Заменяем полученное тело
						this->res.body.assign(body.begin(), body.end());
					// Если данные пришли сжатые методом Deflate
					} else if(it->second.compare("deflate") == 0) {
						// Получаем данные тела в бинарном виде
						vector <char> buffer(this->res.body.begin(), this->res.body.end());
						// Добавляем хвост в полученные данные
						this->hash->setTail(buffer);
						// Выполняем декомпрессию данных
						const auto & body = this->hash->decompress(buffer.data(), buffer.size());
						// Заменяем полученное тело
						this->res.body.assign(body.begin(), body.end());
					}
				}
				// Выполняем поиск заголовка шифрования
				it = this->res.headers.find("x-awh-encryption");
				// Если данные пришли зашифрованные
				if(it != this->res.headers.end()){
					// Определяем размер шифрования
					switch(stoi(it->second)){
						// Если шифрование произведено 128 битным ключём
						case 128: this->hash->setAES(hash_t::aes_t::AES128); break;
						// Если шифрование произведено 192 битным ключём
						case 192: this->hash->setAES(hash_t::aes_t::AES192); break;
						// Если шифрование произведено 256 битным ключём
						case 256: this->hash->setAES(hash_t::aes_t::AES256); break;
					}
					// Выполняем дешифрование полученных данных
					const auto & res = this->hash->decrypt(this->res.body.data(), this->res.body.size());
					// Если данные расшифрованны, заменяем тело данных
					if(!res.empty()) this->res.body.assign(res.begin(), res.end());
				}
			// Если запрос не был выполнен успешно
			} else {
				// Определяем код ответа
				switch(this->res.code){
					// Если требуется авторизация
					case 401: {
						// Если попытки провести аутентификацию ещё небыло, пробуем ещё раз
						if(!this->checkAuth && (this->auth->getType() == auth_t::type_t::DIGEST)){
							// Получаем параметры авторизации
							auto it = this->res.headers.find("www-authenticate");
							// Если параметры авторизации найдены
							if((this->checkAuth = (it != this->res.headers.end()))){
								// Устанавливаем заголовок HTTP в параметры авторизации
								this->auth->setHeader(it->second);
								// Просим повторить авторизацию ещё раз
								this->REST();

								return;
							}
						}
					} break;
					// Если требуется провести перенаправление
					case 301:
					case 308: {
						// Получаем адрес перенаправления
						auto it = this->res.headers.find("location");
						// Если данные пришли зашифрованные
						if(it != this->res.headers.end()){
							// Выполняем парсинг URL
							uri_t::url_t tmp = this->uri->parseUrl(it->second);
							// Если параметры URL существуют
							if(!this->req.uri->params.empty())
								// Переходим по всему списку параметров
								for(auto & param : this->req.uri->params) tmp.params.emplace(param);
							// Меняем IP адрес сервера
							const_cast <uri_t::url_t *> (this->req.uri)->ip = move(tmp.ip);
							// Меняем порт сервера
							const_cast <uri_t::url_t *> (this->req.uri)->port = move(tmp.port);
							// Меняем на путь сервере
							const_cast <uri_t::url_t *> (this->req.uri)->path = move(tmp.path);
							// Меняем доменное имя сервера
							const_cast <uri_t::url_t *> (this->req.uri)->domain = move(tmp.domain);
							// Меняем протокол запроса сервера
							const_cast <uri_t::url_t *> (this->req.uri)->schema = move(tmp.schema);
							// Устанавливаем новый список параметров
							const_cast <uri_t::url_t *> (this->req.uri)->params = move(tmp.params);
							// Выполняем новый запрос
							this->REST();

							return;
						}
					} break;
				}
			}

		// Если происходит ошибка то игнорируем её
		} catch(exception & error) {
			// Выводим сообщение об ошибке
			this->log->print("%s", log_t::flag_t::CRITICAL, error.what());
		}
	}
}

/**
 * REST Метод выполнения REST запроса на сервер
 */
void awh::Rest::REST() noexcept {
	// Если URL адрес получен
	if(!this->req.uri->empty()){
		try {
			// Создаём базу событий
			this->evbuf.base = event_base_new();
			// Если база событий создана
			if(this->evbuf.base != nullptr){
				// Сокет подключения
				evutil_socket_t fd = -1;
				// Создаём DNS резолвер
				dns_t resolver(this->fmk, this->log, this->nwk);
				// Определяем тип сети
				switch(this->req.uri->family){
					// Если - это IPv4
					case AF_INET: {
						// Добавляем список серверов в резолвер
						resolver.setNameServers(this->net.v4.second);
						// Создаём базу событий DNS
						this->evbuf.dns = resolver.init((!this->req.uri->domain.empty() ? this->req.uri->domain : this->req.uri->ip), AF_INET, this->evbuf.base);
						// Создаем сокет подключения
						fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
					} break;
					// Если - это IPv6
					case AF_INET6: {
						// Добавляем список серверов в резолвер
						resolver.setNameServers(this->net.v6.second);
						// Создаём базу событий DNS
						this->evbuf.dns = resolver.init((!this->req.uri->domain.empty() ? this->req.uri->domain : this->req.uri->ip), AF_INET6, this->evbuf.base);
						// Создаем сокет подключения
						fd = ::socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
					} break;
				}
				// Если файловый дескриптор создан
				if(fd > -1){
					// Если - это Unix
					#if !defined(_WIN32) && !defined(_WIN64)
						// Выполняем игнорирование сигнала неверной инструкции процессора
						sockets_t::noSigill(this->log);
						// Устанавливаем разрешение на повторное использование сокета
						sockets_t::reuseable(fd, this->log);
						// Отключаем сигнал записи в оборванное подключение
						sockets_t::noSigpipe(fd, this->log);
						// Отключаем алгоритм Нейгла для сервера и клиента
						sockets_t::tcpNodelay(fd, this->log);
						// Разблокируем сокет
						sockets_t::nonBlocking(fd, this->log);
						// Активируем keepalive
						sockets_t::keepAlive(fd, this->alive.keepcnt, this->alive.keepidle, this->alive.keepintvl, this->log);
					// Если - это Windows
					#else
						// Выполняем инициализацию WinSock
						this->winSocketInit();
						// Переводим сокет в блокирующий режим
						// sockets_t::blocking(fd);
						evutil_make_socket_nonblocking(fd);
						// evutil_make_socket_closeonexec(fd);
						evutil_make_listen_socket_reuseable(fd);
					#endif
				} else return;


				// Выполняем получение контекста сертификата
				this->sslctx = this->ssl->init(* this->req.uri);
				// Если защищённое соединение не нужно, создаём буфер событий
				if(!this->sslctx.mode) this->evbuf.bev = bufferevent_socket_new(this->evbuf.base, fd, BEV_OPT_THREADSAFE | BEV_OPT_DEFER_CALLBACKS);
				// Если требуется защищённое соединение
				else {
					// Создаём буфер событий в защищённом подключении
					this->evbuf.bev = bufferevent_openssl_socket_new(this->evbuf.base, fd, this->sslctx.ssl, BUFFEREVENT_SSL_CONNECTING, BEV_OPT_THREADSAFE | BEV_OPT_DEFER_CALLBACKS);
					// Разрешаем грязное отключение
					bufferevent_openssl_set_allow_dirty_shutdown(this->evbuf.bev, 1);
				}
				// Если буфер событий не создан
				if(this->evbuf.bev == nullptr){
					// Очищаем контекст
					this->clear();
					// Сообщаем, что буфер событий не может быть создан
					this->log->print("%s", log_t::flag_t::CRITICAL, "the event buffer could not be created");
					// Завершаем работу функции
					return;
				}
				// Создаём событие подключения
				this->evbuf.evcon = evhttp_connection_base_bufferevent_new(this->evbuf.base, this->evbuf.dns, this->evbuf.bev, (!this->req.uri->ip.empty() ? this->req.uri->ip.c_str() : this->req.uri->domain.c_str()), this->req.uri->port);
				// Если событие подключения не создан
				if(this->evbuf.evcon == nullptr){
					// Очищаем контекст
					this->clear();
					// Сообщаем, что событие подключения не создано
					this->log->print("%s", log_t::flag_t::CRITICAL, "connection event not created");
					// Завершаем работу функции
					return;
				}
				// Выполняем 5 попыток запросить данные
				evhttp_connection_set_retries(this->evbuf.evcon, 5);
				// Таймаут на выполнение в 5 секунд
				evhttp_connection_set_timeout(this->evbuf.evcon, 5);
				// Заставляем выполнять подключение по указанному протоколу сети
				evhttp_connection_set_family(this->evbuf.evcon, this->req.uri->family);
				// Создаём объект выполнения REST запроса
				struct evhttp_request * req = evhttp_request_new(callback, this);
				// Если объект REST запроса не создан
				if(req == nullptr){
					// Очищаем контекст
					this->clear();
					// Сообщаем, что событие REST запроса не создано
					this->log->print("%s", log_t::flag_t::CRITICAL, "REST request event not created");
					// Завершаем работу функции
					return;
				}


				makeHeaders(req, * this->req.uri, * this->req.headers, (void *) this);

				makeBody(req, * this->req.body, (void *) this);


				// Выполняем REST запрос на сервер
				const int request = evhttp_make_request(this->evbuf.evcon, req, this->req.method, this->uri->createUrl(* this->req.uri).c_str());
				// Если запрос не выполнен
				if(request != 0){
					// Очищаем контекст
					this->clear();
					// Сообщаем, что запрос не выполнен
					this->log->print("%s", log_t::flag_t::CRITICAL, "REST request failed");
					// Завершаем работу функции
					return;
				}
				// Блокируем базу событий
				event_base_dispatch(this->evbuf.base);
			// Выводим сообщение в лог
			} else this->log->print("%s", log_t::flag_t::CRITICAL, "the event base could not be created");
			// Очищаем контекст
			this->clear();
			// Если код ответа не требует авторизации, разрешаем дальнейшие попытки
			if(this->res.code != 401) this->checkAuth = false;
			// Определяем, был ли ответ сервера удачным
			this->res.ok = ((this->res.code >= 200) && (this->res.code <= 206) || (this->res.code == 100));
			// Если запрос выполнен успешно
			if(this->res.ok){
				// Проверяем пришли ли сжатые данные
				auto it = this->res.headers.find("content-encoding");
				// Если данные пришли зашифрованные
				if(it != this->res.headers.end()){
					// Если данные пришли сжатые методом GZIP
					if(it->second.compare("gzip") == 0){
						// Выполняем декомпрессию данных
						const auto & body = this->hash->decompressGzip(this->res.body.data(), this->res.body.size());
						// Заменяем полученное тело
						this->res.body.assign(body.begin(), body.end());
					// Если данные пришли сжатые методом Deflate
					} else if(it->second.compare("deflate") == 0) {
						// Получаем данные тела в бинарном виде
						vector <char> buffer(this->res.body.begin(), this->res.body.end());
						// Добавляем хвост в полученные данные
						this->hash->setTail(buffer);
						// Выполняем декомпрессию данных
						const auto & body = this->hash->decompress(buffer.data(), buffer.size());
						// Заменяем полученное тело
						this->res.body.assign(body.begin(), body.end());
					}
				}
				// Выполняем поиск заголовка шифрования
				it = this->res.headers.find("x-awh-encryption");
				// Если данные пришли зашифрованные
				if(it != this->res.headers.end()){
					// Определяем размер шифрования
					switch(stoi(it->second)){
						// Если шифрование произведено 128 битным ключём
						case 128: this->hash->setAES(hash_t::aes_t::AES128); break;
						// Если шифрование произведено 192 битным ключём
						case 192: this->hash->setAES(hash_t::aes_t::AES192); break;
						// Если шифрование произведено 256 битным ключём
						case 256: this->hash->setAES(hash_t::aes_t::AES256); break;
					}
					// Выполняем дешифрование полученных данных
					const auto & res = this->hash->decrypt(this->res.body.data(), this->res.body.size());
					// Если данные расшифрованны, заменяем тело данных
					if(!res.empty()) this->res.body.assign(res.begin(), res.end());
				}
			// Если запрос не был выполнен успешно
			} else {
				// Определяем код ответа
				switch(this->res.code){
					// Если требуется авторизация
					case 401: {
						// Если попытки провести аутентификацию ещё небыло, пробуем ещё раз
						if(!this->checkAuth && (this->auth->getType() == auth_t::type_t::DIGEST)){
							// Получаем параметры авторизации
							auto it = this->res.headers.find("www-authenticate");
							// Если параметры авторизации найдены
							if((this->checkAuth = (it != this->res.headers.end()))){
								// Устанавливаем заголовок HTTP в параметры авторизации
								this->auth->setHeader(it->second);
								// Просим повторить авторизацию ещё раз
								this->REST();

								return;
							}
						}
					} break;
					// Если требуется провести перенаправление
					case 301:
					case 308: {
						// Получаем адрес перенаправления
						auto it = this->res.headers.find("location");
						// Если данные пришли зашифрованные
						if(it != this->res.headers.end()){
							// Выполняем парсинг URL
							uri_t::url_t tmp = this->uri->parseUrl(it->second);
							// Если параметры URL существуют
							if(!this->req.uri->params.empty())
								// Переходим по всему списку параметров
								for(auto & param : this->req.uri->params) tmp.params.emplace(param);
							// Меняем IP адрес сервера
							const_cast <uri_t::url_t *> (this->req.uri)->ip = move(tmp.ip);
							// Меняем порт сервера
							const_cast <uri_t::url_t *> (this->req.uri)->port = move(tmp.port);
							// Меняем на путь сервере
							const_cast <uri_t::url_t *> (this->req.uri)->path = move(tmp.path);
							// Меняем доменное имя сервера
							const_cast <uri_t::url_t *> (this->req.uri)->domain = move(tmp.domain);
							// Меняем протокол запроса сервера
							const_cast <uri_t::url_t *> (this->req.uri)->schema = move(tmp.schema);
							// Устанавливаем новый список параметров
							const_cast <uri_t::url_t *> (this->req.uri)->params = move(tmp.params);
							// Выполняем новый запрос
							this->REST();

							return;
						}
					} break;
				}
			}
		// Если происходит ошибка то игнорируем её
		} catch(exception & error) {
			// Выводим сообщение об ошибке
			this->log->print("%s", log_t::flag_t::CRITICAL, error.what());
		}
	}
}
/**
 * setZip Метод активации работы с сжатым контентом
 * @param method метод установки формата сжатия
 */
void awh::Rest::setZip(const zip_t method) noexcept {
	// Устанавливаем флаг сжатого контента
	this->zip = method;
}
/**
 * setChunked Метод активации режима передачи тела запроса чанками
 * @param mode флаг активации режима передачи тела запроса чанками
 */
void awh::Rest::setChunked(const bool mode) noexcept {
	// Устанавливаем флаг активации режима передачи тела запроса чанками
	this->chunked = mode;
}
/**
 * setVerifySSL Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
 * @param mode флаг состояния разрешения проверки
 */
void awh::Rest::setVerifySSL(const bool mode) noexcept {
	// Выполняем установку флага проверки домена
	this->ssl->setVerify(mode);
}
/**
 * setFamily Метод установки тип протокола интернета
 * @param family тип протокола интернета AF_INET или AF_INET6
 */
void awh::Rest::setFamily(const int family) noexcept {
	// Устанавливаем тип активного интернет-подключения
	this->net.family = family;
}
/**
 * setUserAgent Метод установки User-Agent для HTTP запроса
 * @param userAgent агент пользователя для HTTP запроса
 */
void awh::Rest::setUserAgent(const string & userAgent) noexcept {
	// Устанавливаем UserAgent
	if(!userAgent.empty()) this->userAgent = userAgent;
}
/**
 * setUser Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::Rest::setUser(const string & login, const string & password) noexcept {
	// Если пользователь и пароль переданы
	if(!login.empty() && !password.empty()){
		// Устанавливаем логин пользователя
		this->auth->setLogin(login);
		// Устанавливаем пароль пользователя
		this->auth->setPassword(password);
	}
}
/**
 * setCA Метод установки CA-файла корневого SSL сертификата
 * @param cafile адрес CA-файла
 * @param capath адрес каталога где находится CA-файл
 */
void awh::Rest::setCA(const string & cafile, const string & capath) noexcept {
	// Устанавливаем адрес CA-файла
	this->ssl->setCA(cafile, capath);
}
/**
 * setProxy Метод установки прокси-сервера
 * @param uri  параметры подключения к прокси-серверу
 * @param type тип используемого прокси-сервера
 */
void awh::Rest::setProxy(const string & uri, const proxy_t type) noexcept {
	// Если данные прокси-сервера получены
	if(!uri.empty()){
		// Устанавливаем тип прокси-сервера
		this->proxyType = type;
		// Выполняем парсинг URI прокси-сервера
		this->proxyUrl = this->uri->parseUrl(uri);
	}
}
/**
 * setNet Метод установки параметров сети
 * @param ip     список IP адресов компьютера с которых разрешено выходить в интернет
 * @param ns     список серверов имён, через которые необходимо производить резолвинг доменов
 * @param family тип протокола интернета AF_INET или AF_INET6
 */
void awh::Rest::setNet(const vector <string> & ip, const vector <string> & ns, const int family) noexcept {
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
void awh::Rest::setCrypt(const string & pass, const string & salt, const hash_t::aes_t aes) noexcept {
	// Устанавливаем флаг шифрования
	this->crypt = !pass.empty();
	// Устанавливаем размер шифрования
	this->hash->setAES(aes);
	// Устанавливаем соль шифрования
	this->hash->setSalt(salt);
	// Устанавливаем пароль шифрования
	this->hash->setPassword(pass);
}
/**
 * setAuthType Метод установки типа авторизации
 * @param type      тип авторизации
 * @param algorithm алгоритм шифрования для Digest авторизации
 */
void awh::Rest::setAuthType(const auth_t::type_t type, const auth_t::algorithm_t algorithm) noexcept {
	// Если объект авторизации создан
	if(this->auth != nullptr) this->auth->setType(type, algorithm);
}
/**
 * Rest Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 * @param uri объект работы с URI
 * @param nwk объект методов для работы с сетью
 */
awh::Rest::Rest(const fmk_t * fmk, const log_t * log, const uri_t * uri, const network_t * nwk) noexcept {
	try {
		// Устанавливаем зависимые модули
		this->fmk = fmk;
		this->log = log;
		this->uri = uri;
		this->nwk = nwk;
		// Резервируем память для работы с буфером данных прокси-сервера
		this->hdt = new char[BUFFER_CHUNK];
		// Создаём объект для работы с компрессией/декомпрессией
		this->hash = new hash_t(this->fmk, this->log);
		// Создаём объект для работы с авторизацией
		this->auth = new auth_t(this->fmk, this->log);
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
 * ~Rest Деструктор
 */
awh::Rest::~Rest() noexcept {
	// Выполняем очистку памяти
	this->clear();
	// Если объект DNS резолвера создан
	if(this->dns != nullptr) delete this->dns;
	// Если объект для работы с SSL создан
	if(this->ssl != nullptr) delete this->ssl;
	// Удаляем объект работы с авторизацией
	if(this->auth != nullptr) delete this->auth;
	// Если объект для компрессии/декомпрессии создан
	if(this->hash != nullptr) delete this->hash;
	// Если буфер данных прокси-сервера создан
	if(this->hdt != nullptr) delete [] this->hdt;
	// Если - это Windows
	#if defined(_WIN32) || defined(_WIN64)
		// Очищаем сетевой контекст
		this->winSocketClean();
	#endif
}
