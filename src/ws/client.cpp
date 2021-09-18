/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <ws/client.hpp>

// Если - это Windows
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * winSocketInit Метод инициализации WinSock
	 */
	void awh::Client::winSocketInit() const noexcept {
		// Если winSock ещё не инициализирован
		if(!this->winSock){
			// Идентификатор ошибки
			int error = 0;
			// Объект данных запроса
			WSADATA wsaData;
			// Выполняем инициализацию сетевого контекста
			if((error = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0){ // 0x202
				// Сообщаем, что сетевой контекст не поднят
				this->fmk->log("WSAStartup failed with error: %d", fmk_t::log_t::CRITICAL, this->logfile, error);
				// Выходим из приложения
				exit(EXIT_FAILURE);
			}
			// Выполняем проверку версии WinSocket
			if((2 != LOBYTE(wsaData.wVersion)) || (2 != HIBYTE(wsaData.wVersion))){
				// Сообщаем, что версия WinSocket не подходит
				this->fmk->log("%s", fmk_t::log_t::CRITICAL, this->logfile, "WSADATA version is not correct");
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
	void awh::Client::winSocketClean() const noexcept {
		// Очищаем сетевой контекст
		WSACleanup();
		// Запоминаем, что winSock не инициализирован
		this->winSock = false;
	}
#endif
/**
 * request Метод выполнения HTTP запроса
 */
void awh::Client::request() noexcept {
	// Если подключение установленно
	if(this->bev != nullptr){
		// Выполняем очистку объект HTTP запроса
		this->http->clear();
		// Устанавливаем сабпротоколы
		this->http->setSubs(this->subs);
		// Получаем копию объекта URL адреса
		uri_t::url_t url = this->url;
		// Удаляем IP адрес если домен существует
		if(!url.domain.empty()) url.ip.clear();
		// Генерируем URL адрес запроса
		const string & origin = this->uri->createOrigin(url);
		// Устанавливаем Origin запроса
		this->http->setOrigin(origin);
		// Строка HTTP запроса
		const auto & request = this->http->restRequest(this->gzip);
		// Если запрос получен
		if(!request.empty()){
			// Активируем разрешение на запись и чтение
			bufferevent_enable(this->bev, EV_WRITE | EV_READ);
			// Отправляем серверу сообщение
			bufferevent_write(this->bev, request.data(), request.size());
		}
	}
}
/**
 * delay Метод фриза потока на указанное количество секунд
 * @param seconds количество секунд для фриза потока
 */
void awh::Client::delay(const size_t seconds) const noexcept {
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
 * error Метод вывода сообщений об ошибках работы клиента
 * @param message сообщение с описанием ошибки
 */
void awh::Client::error(const mess_t & message) const noexcept {
	// Если сообщение об ошибке пришло
	if(!message.text.empty()){
		// Если тип сообщения получен
		if(!message.type.empty())
			// Выводим в лог сообщение
			this->fmk->log("%s - %s [%u]", fmk_t::log_t::WARNING, this->logfile, message.type.c_str(), message.text.c_str(), message.code);
		// Иначе выводим сообщение в упрощёном виде
		else this->fmk->log("%s [%u]", fmk_t::log_t::WARNING, this->logfile, message.text.c_str(), message.code);
		// Если функция обратного вызова установлена, выводим полученное сообщение
		if(this->errorFn != nullptr) this->errorFn(message.code, message.text, const_cast <Client *> (this));
	}
}
/**
 * extraction Метод извлечения полученных данных
 * @param buffer данные в чистом виде полученные с сервера
 * @param utf8   данные передаются в текстовом виде
 */
void awh::Client::extraction(const vector <char> & buffer, const bool utf8) const noexcept {
	// Если буфер данных передан
	if(!buffer.empty() && !this->freeze && (this->messageFn != nullptr)){
		// Если данные пришли в сжатом виде
		if(this->compressed){
			// Добавляем хвост в полученные данные
			this->hash->setTail(* const_cast <vector <char> *> (&buffer));
			// Выполняем декомпрессию полученных данных
			const auto & data = this->hash->decompress(buffer.data(), buffer.size());
			// Если данные получены
			if(!data.empty()){
				// Если нужно производить дешифрование
				if(this->crypt){
					// Выполняем шифрование переданных данных
					const auto & res = this->hash->decrypt(data.data(), data.size());
					// Отправляем полученный результат
					this->messageFn(res, utf8, const_cast <Client *> (this));
				// Отправляем полученный результат
				} else this->messageFn(data, utf8, const_cast <Client *> (this));
			// Выводим сообщение об ошибке
			} else {
				// Создаём сообщение
				mess_t mess(1007, "received data decompression error");
				// Выводим сообщение
				this->error(mess);
			}
		// Если функция обратного вызова установлена, выводим полученное сообщение
		} else {
			// Если нужно производить дешифрование
			if(this->crypt){
				// Выполняем шифрование переданных данных
				const auto & res = this->hash->decrypt(buffer.data(), buffer.size());
				// Отправляем полученный результат
				this->messageFn(res, utf8, const_cast <Client *> (this));
			// Отправляем полученный результат
			} else this->messageFn(buffer, utf8, const_cast <Client *> (this));
		}
	}
}
/**
 * connect Метод создания сокета для подключения к удаленному серверу
 * @return результат подключения
 */
const bool awh::Client::connect() noexcept {
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
				// this->bev = bufferevent_openssl_socket_new(this->base, this->fd, this->sslctx.ssl, BUFFEREVENT_SSL_CONNECTING, BEV_OPT_DEFER_CALLBACKS | BEV_OPT_UNLOCK_CALLBACKS);
				// this->bev = bufferevent_openssl_socket_new(this->base, this->fd, this->sslctx.ssl, BUFFEREVENT_SSL_CONNECTING, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
				// Разрешаем непредвиденное грязное завершение работы
				bufferevent_openssl_set_allow_dirty_shutdown(this->bev, 1);
			// Создаем буфер событий для сервера
			// } else this->bev = bufferevent_socket_new(this->base, this->fd, BEV_OPT_DEFER_CALLBACKS);
			} else this->bev = bufferevent_socket_new(this->base, this->fd, BEV_OPT_THREADSAFE);
			// Если буфер событий создан
			if(this->bev != nullptr){
				// Устанавливаем водяной знак на 1 байт (чтобы считывать данные когда они действительно приходят)
				// bufferevent_setwatermark(this->bev, EV_READ | EV_WRITE, 1, 0);
				// Устанавливаем таймаут ожидания поступления данных
				struct timeval timeout = {READ_TIMEOUT, 0};
				// Устанавливаем коллбеки
				// bufferevent_setcb(this->bev, &read, &write, &event, this);
				bufferevent_setcb(this->bev, &read, nullptr, &event, this);
				// Очищаем буферы событий при завершении работы
				bufferevent_flush(this->bev, EV_READ | EV_WRITE, BEV_FINISHED);
				// Устанавливаем таймаут получения данных
				bufferevent_set_timeouts(this->bev, &timeout, nullptr);
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
					this->fmk->log("connecting to host = %s, port = %u", fmk_t::log_t::CRITICAL, this->logfile, this->url.ip.c_str(), this->url.port);
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
				this->fmk->log("create good connect to host = %s [%s:%d], socket = %d", fmk_t::log_t::INFO, this->logfile, this->url.domain.c_str(), this->url.ip.c_str(), this->url.port, this->fd);
				// Сообщаем что все удачно
				result = true;
			// Выполняем новую попытку подключения
			} else {
				// Выводим в лог сообщение
				this->fmk->log("connecting to host = %s, port = %u", fmk_t::log_t::CRITICAL, this->logfile, this->url.ip.c_str(), this->url.port);
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
const awh::Client::socket_t awh::Client::socket(const string & ip, const u_int port, const int family) const noexcept {
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
				this->fmk->log("network not allow from server = %s, port = %u", fmk_t::log_t::CRITICAL, this->logfile, ip.c_str(), port);
				// Выходим
				return result;
			}
		}
		// Если сокет не создан то выходим
		if(result.fd < 0){
			// Выводим сообщение в консоль
			this->fmk->log("creating socket to server = %s, port = %u", fmk_t::log_t::CRITICAL, this->logfile, ip.c_str(), port);
			// Выходим
			return result;
		}
		// Если - это Unix
		#if !defined(_WIN32) && !defined(_WIN64)
			// Выполняем игнорирование сигнала неверной инструкции процессора
			sockets_t::noSigill(this->fmk, this->logfile);
			// Устанавливаем разрешение на повторное использование сокета
			sockets_t::reuseable(result.fd, this->fmk, this->logfile);
			// Отключаем сигнал записи в оборванное подключение
			sockets_t::noSigpipe(result.fd, this->fmk, this->logfile);
			// Отключаем алгоритм Нейгла для сервера и клиента
			sockets_t::tcpNodelay(result.fd, this->fmk, this->logfile);
			// Разблокируем сокет
			sockets_t::nonBlocking(result.fd, this->fmk, this->logfile);
			// Активируем keepalive
			sockets_t::keepAlive(result.fd, this->alive.keepcnt, this->alive.keepidle, this->alive.keepintvl, this->fmk, this->logfile);
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
			this->fmk->log("bind local network [%s] error", fmk_t::log_t::CRITICAL, this->logfile, host.c_str());
			// Выходим
			return result;
		}
	}
	// Выводим результат
	return result;
}
/**
 * read Метод чтения данных с сокета сервера
 * @param bev буфер события
 * @param ctx передаваемый контекст
 */
void awh::Client::read(struct bufferevent * bev, void * ctx){
	// Если данные переданы верные
	if((bev != nullptr) && (ctx != nullptr)){
		// Получаем буферы входящих данных
		struct evbuffer * input = bufferevent_get_input(bev);
		// Получаем размер входящих данных
		size_t size = evbuffer_get_length(input);
		// Если данные существуют
		if(size > 0){
			// Получаем объект подключения
			client_t * ws = reinterpret_cast <client_t *> (ctx);
			// Если рукопожатие не выполнено
			if(!ws->http->isHandshake()){
				// Количество прочитанных байт
				size_t count = 0;
				// Считываем данные из буфера до тех пор пока можешь считать
				do {
					// Считываем строки из буфера
					const char * str = evbuffer_readln(input, &count, EVBUFFER_EOL_CRLF_STRICT);
					// Если данные не добавлены
					if(!ws->http->add(str, count)){
						// Проверяем тип ответа сервера
						http_t::stath_t stath = ws->http->isAuth();
						// Определяем тип ответа
						switch((u_short) stath){
							// Если авторизация не выполнена
							case (u_short) http_t::stath_t::FAULT: {
								// Получаем код сообщения
								const u_short code = ws->http->getCode();
								// Создаём сообщение
								mess_t mess(code, ws->http->getMessage(code));
								// Выводим сообщение
								ws->error(mess);
								// Завершаем работу клиента
								ws->stop();
								// Выходим из функции
								return;
							} break;
							// Если нужно попробовать ещё раз
							case (u_short) http_t::stath_t::RETRY: {
								// Строка HTTP запроса
								const auto & request = ws->http->restRequest(ws->gzip);
								// Если запрос получен
								if(!request.empty()){
									// Активируем разрешение на запись и чтение
									bufferevent_enable(ws->bev, EV_WRITE | EV_READ);
									// Отправляем серверу сообщение
									bufferevent_write(ws->bev, request.data(), request.size());
								}
							} break;
						}
					// Если инициализация WebSocket произведена успешно
					} else {
						// Устанавливаем флаг, что мы работаем с сжатыми данными
						ws->gzip = ws->http->isGzip();
						// Выводим в лог сообщение
						ws->fmk->log("%s", fmk_t::log_t::INFO, ws->logfile, "authorization on the WebSocket server was successful");
						// Если функция обратного вызова установлена, выполняем
						if(ws->openStopFn != nullptr) ws->openStopFn(true, ws);
						// Устанавливаем таймер на контроль подключения
						ws->timerPing.setInterval([ws]{
							// Выполняем пинг сервера
							ws->ping(to_string(time(nullptr)));
						}, PING_INTERVAL);
					}
					// Если данные не найдены тогда выходим
					if(str == nullptr) break;
				// Если заголовки существуют
				} while(count > 0);
			// Если рукопожатие выполнено
			} else {
				// Смещение в буфере данных
				size_t offset = 0;
				// Создаём объект шапки фрейма
				frame_t::head_t head;
				// Если нужно выполнить автоматическое переподключение
				if(ws->reconnect){
					// Останавливаем таймер
					ws->timerConnect.stop();
					// Устанавливаем таймер на контроль подключения
					ws->timerConnect.setTimeout([bev, ws]{
						// Запрещаем запись данных клиенту
						bufferevent_disable(bev, EV_READ | EV_WRITE);
						// Завершаем работу базы событий
						event_base_loopbreak(ws->base);
						// Отключаемся от WebSocket
						ws->close();
					}, CONNECT_TIMEOUT);
				}
				// Выполняем компенсацию размера полученных данных
				size = (size > BUFFER_CHUNK ? BUFFER_CHUNK : size);
				// Копируем в буфер полученные данные
				evbuffer_copyout(input, (void *) ws->wdt, size);
				// Выполняем перебор полученных данных
				while((size - offset) > 0){
					// Выполняем чтение фрейма WebSocket
					const auto & data = ws->frame->get(head, ws->wdt + offset, size - offset);
					// Если буфер данных получен
					if(!data.empty()){
						// Проверяем состояние флагов RSV2 и RSV3
						if(head.rsv[1] || head.rsv[2]){
							// Создаём сообщение
							mess_t mess(1002, "RSV2 and RSV3 must be clear");
							// Выводим сообщение
							ws->error(mess);
							// Выполняем реконнект
							goto Reconnect;
						}
						// Если флаг компресси отключён а данные пришли сжатые
						if(head.rsv[0] && (!ws->gzip ||
						(head.optcode == frame_t::opcode_t::CONTINUATION) ||
						(((u_short) head.optcode > 0x07) && ((u_short) head.optcode < 0x0b)))){
							// Создаём сообщение
							mess_t mess(1002, "RSV1 must be clear");
							// Выводим сообщение
							ws->error(mess);
							// Выполняем реконнект
							goto Reconnect;
						}
						// Если опкоды требуют финального фрейма
						if(!head.fin && ((u_short) head.optcode > 0x07) && ((u_short) head.optcode < 0x0b)){
							// Создаём сообщение
							mess_t mess(1002, "FIN must be set");
							// Выводим сообщение
							ws->error(mess);
							// Выполняем реконнект
							goto Reconnect;
						}
						// Определяем тип ответа
						switch((u_short) head.optcode){
							// Если ответом является PING
							case (u_short) frame_t::opcode_t::PING: {
								// Получаем фрейм для отправки ответа серверу
								const auto & res = ws->frame->pong(string(data.begin(), data.end()));
								// Если фрейм ответа PONG получен
								if(!res.empty()){
									// Активируем разрешение на запись и чтение
									bufferevent_enable(ws->bev, EV_WRITE | EV_READ);
									// Отправляем серверу сообщение
									bufferevent_write(ws->bev, res.data(), res.size());
								}
							} break;
							// Если ответом является PONG
							case (u_short) frame_t::opcode_t::PONG: {
								// Если функция обратного вызова обработки PONG существует
								if(ws->pongFn != nullptr) ws->pongFn(string(data.begin(), data.end()), ws);
							} break;
							// Если ответом является TEXT
							case (u_short) frame_t::opcode_t::TEXT:
							// Если ответом является BINARY
							case (u_short) frame_t::opcode_t::BINARY: {
								// Запоминаем полученный опкод
								ws->opcode = head.optcode;
								// Запоминаем в каком виде пришли данные, в сжатом или нет
								ws->compressed = (ws->gzip && head.rsv[0]);
								// Если список фрагментированных сообщений существует
								if(!ws->fragmes.empty()){
									// Создаём сообщение
									mess_t mess(1002, "opcode for subsequent fragmented messages should not be set");
									// Выводим сообщение
									ws->error(mess);
									// Выполняем реконнект
									goto Reconnect;
								// Если сообщение является не последнем
								} else if(!head.fin)
									// Заполняем фрагментированное сообщение
									ws->fragmes.insert(ws->fragmes.end(), data.begin(), data.end());
								// Если сообщение является последним
								else ws->extraction(data, (ws->opcode == frame_t::opcode_t::TEXT));
							} break;
							// Если ответом является CONTINUATION
							case (u_short) frame_t::opcode_t::CONTINUATION: {
								// Если сообщение является не последнем
								if(!head.fin)
									// Заполняем фрагментированное сообщение
									ws->fragmes.insert(ws->fragmes.end(), data.begin(), data.end());
								// Если сообщение является последним
								else {
									// Выполняем извлечение данных
									ws->extraction(ws->fragmes, (ws->opcode == frame_t::opcode_t::TEXT));
									// Очищаем список фрагментированных сообщений
									ws->fragmes.clear();
								}
							} break;
							// Если ответом является CLOSE
							case (u_short) frame_t::opcode_t::CLOSE: {
								// Извлекаем сообщение
								const auto & mess = ws->frame->message(data);
								// Выводим сообщение
								ws->error(mess);
								// Выполняем реконнект
								goto Reconnect;
							} break;
						}
						// Увеличиваем смещение в буфере
						offset += (head.payload + head.size);
						// Удаляем данные из буфера
						evbuffer_drain(input, head.payload + head.size);
					// Выходим из цикла, данных в буфере не достаточно
					} else break;
				}
			}
			// Выходим из функции
			return;
		}
		// Устанавливаем метку реконнекта
		Reconnect:
		// Запрещаем запись данных клиенту
		bufferevent_disable(bev, EV_READ | EV_WRITE);
		// Выполняем реконнект
		event(bev, BEV_EVENT_EOF, ctx);
	}
}
/**
 * write Метод записи данных в сокет сервера
 * @param bev буфер события
 * @param ctx передаваемый объект
 */
/*
void awh::Client::write(struct bufferevent * bev, void * ctx) noexcept {
	// Если подключение передано
	if(ctx != nullptr){
		// Получаем объект подключения
		client_t * ws = reinterpret_cast <client_t *> (ctx);
		// Если буфер клиента существует
		if(ws->bev != nullptr){
			// Запрещаем запись данных клиенту
			bufferevent_disable(ws->bev, EV_READ | EV_WRITE);
			// Отключаемся
			event(ws->bev, BEV_EVENT_EOF, ctx);
		}
	}
}
*/
/**
 * event Метод обработка входящих событий с сервера
 * @param bev    буфер события
 * @param events произошедшее событие
 * @param ctx    передаваемый контекст
 */
void awh::Client::event(struct bufferevent * bev, const short events, void * ctx) noexcept {
	// Если подключение не передано
	if(ctx != nullptr){
		// Получаем объект подключения
		client_t * ws = reinterpret_cast <client_t *> (ctx);
		// Если фреймворк получен
		if(ws->fmk != nullptr){
			// Получаем текущий сокет
			const evutil_socket_t socket = bufferevent_getfd(bev);
			// Если подключение удачное
			if(events & BEV_EVENT_CONNECTED){
				// Выводим в лог сообщение
				ws->fmk->log("connect client to server [%s:%d]", fmk_t::log_t::INFO, ws->logfile, ws->url.ip.c_str(), ws->url.port);
				// Выполняем запрос на сервер
				ws->request();
			// Если это ошибка или завершение работы
			} else if(events & (BEV_EVENT_ERROR | BEV_EVENT_EOF | BEV_EVENT_TIMEOUT)) {
				// Если это ошибка
				if(events & BEV_EVENT_ERROR)
					// Выводим в лог сообщение
					ws->fmk->log("closing server [%s:%d] error: %s", fmk_t::log_t::WARNING, ws->logfile, ws->url.ip.c_str(), ws->url.port, evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
				// Если - это таймаут, выводим сообщение в лог
				else if(events & BEV_EVENT_TIMEOUT) ws->fmk->log("timeout server [%s:%d]", fmk_t::log_t::WARNING, ws->logfile, ws->url.ip.c_str(), ws->url.port);
				// Если нужно выполнить автоматическое переподключение
				if(ws->reconnect){
					// Закрываем подключение
					ws->close();
					// Выдерживаем паузу на 10 секунд
					ws->delay(10);
					// Выполняем новое подключение
					ws->connect();
				// Иначе останавливаем работу
				} else ws->stop();
			}
		}
	}
}
/**
 * close Метод закрытия соединения сервера
 */
void awh::Client::close() noexcept {
	// Очищаем буфер фрагментированного сообщения
	this->fragmes.clear();
	// Останавливаем таймер пинга сервера
	this->timerPing.stop();
	// Останавливаем таймер подключения
	this->timerConnect.stop();
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
	this->fmk->log("%s", fmk_t::log_t::INFO, this->logfile, "disconnected from the server");
	// Если - это Windows
	#if defined(_WIN32) || defined(_WIN64)
		// Очищаем сетевой контекст
		this->winSocketClean();
	#endif
}
/**
 * ping Метод проверки доступности сервера
 * @param message сообщение для отправки
 */
void awh::Client::ping(const string & message) noexcept {
	// Если подключение выполнено
	if((this->bev != nullptr) && this->mode && !this->halt){
		// Если рукопожатие выполнено
		if(this->http->isHandshake()){
			// Создаём буфер для отправки
			const auto & buffer = this->frame->ping(message);
			// Активируем разрешение на запись и чтение
			bufferevent_enable(this->bev, EV_WRITE | EV_READ);
			// Отправляем серверу сообщение
			bufferevent_write(this->bev, buffer.data(), buffer.size());
		}
	}
}
/**
 * resolve Метод выполняющая резолвинг хоста http запроса
 * @param url      параметры хоста, для которого нужно получить IP адрес
 * @param callback функция обратного вызова
 */
void awh::Client::resolve(const uri_t::url_t & url, function <void (const string &)> callback) noexcept {
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
 * init Метод инициализации WebSocket клиента
 * @param url  адрес WebSocket сервера
 * @param gzip флаг активации сжатия данных
 */
void awh::Client::init(const string & url, const bool gzip){
	// Если адрес сервера передан
	if(!url.empty()){
		// Устанавливаем флаг активации сжатия данных
		this->gzip = gzip;
		// Выполняем установку URL адреса сервера WebSocket
		this->url = this->uri->parseUrl(url);
	}
}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param callback функция обратного вызова
 */
void awh::Client::on(function <void (const bool, Client *)> callback) noexcept {
	// Устанавливаем функцию запуска и остановки
	this->openStopFn = callback;
}
/**
 * on Метод установки функции обратного вызова на событие получения PONG
 * @param callback функция обратного вызова
 */
void awh::Client::on(function <void (const string &, Client *)> callback) noexcept {
	// Устанавливаем функцию получения сообщений PONG
	this->pongFn = callback;
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибок
 * @param callback функция обратного вызова
 */
void awh::Client::on(function <void (const u_short, const string &, Client *)> callback) noexcept {
	// Устанавливаем функцию получения ошибок
	this->errorFn = callback;
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param callback функция обратного вызова
 */
void awh::Client::on(function <void (const vector <char> &, const bool, Client *)> callback) noexcept {
	// Устанавливаем функцию получения сообщений с сервера
	this->messageFn = callback;
}
/**
 * send Метод отправки сообщения на сервер
 * @param message буфер сообщения в бинарном виде
 * @param size    размер сообщения в байтах
 * @param utf8    данные передаются в текстовом виде
 */
void awh::Client::send(const char * message, const size_t size, const bool utf8) noexcept {
	// Если подключение выполнено
	if((this->bev != nullptr) && this->mode && !this->halt){
		// Если рукопожатие выполнено
		if((message != nullptr) && (size > 0) && this->http->isHandshake()){
			// Создаём объект заголовка для отправки
			frame_t::head_t head;
			// Передаём сообщение одним запросом
			head.fin = true;
			// Выполняем маскировку сообщения
			head.mask = true;
			// Указываем, что сообщение передаётся в сжатом виде
			head.rsv[0] = this->gzip;
			// Устанавливаем опкод сообщения
			head.optcode = (utf8 ? frame_t::opcode_t::TEXT : frame_t::opcode_t::BINARY);
			// Если нужно производить шифрование
			if(this->crypt){
				// Выполняем шифрование переданных данных
				const auto & res = this->hash->encrypt(message, size);
				// Заменяем сообщение для передачи
				message = res.data();
				// Заменяем размер сообщения
				(* const_cast <size_t *> (&size)) = res.size();
			}
			/**
			 * sendFn Функция отправки сообщения на сервер
			 * @param head    объект заголовков фрейма WebSocket
			 * @param message буфер сообщения в бинарном виде
			 * @param size    размер сообщения в байтах
			 */
			auto sendFn = [this](const frame_t::head_t & head, const char * message, const size_t size){
				// Если все данные переданы
				if((message != nullptr) && (size > 0)){
					// Если необходимо сжимать сообщение перед отправкой
					if(this->gzip){
						// Выполняем компрессию данных
						auto data = this->hash->compress(message, size);
						// Удаляем хвост в полученных данных
						this->hash->rmTail(data);
						// Создаём буфер для отправки
						const auto & buffer = this->frame->set(head, data.data(), data.size());
						// Отправляем серверу сообщение
						bufferevent_write(this->bev, buffer.data(), buffer.size());
					// Если сообщение перед отправкой сжимать не нужно
					} else {
						// Создаём буфер для отправки
						const auto & buffer = this->frame->set(head, message, size);
						// Отправляем серверу сообщение
						bufferevent_write(this->bev, buffer.data(), buffer.size());
					}
				}
			};
			// Активируем разрешение на запись и чтение
			bufferevent_enable(this->bev, EV_WRITE | EV_READ);
			// Если требуется фрагментация сообщения
			if(size > this->frameSize){
				// Бинарный буфер чанка данных
				vector <char> chunk(this->frameSize);
				// Смещение в бинарном буфере
				size_t start = 0, stop = this->frameSize;
				// Выполняем разбивку полезной нагрузки на сегменты
				while(stop < size){
					// Увеличиваем длину чанка
					stop += start;
					// Если длина чанка слишком большая, компенсируем
					stop = (stop > size ? size : stop);
					// Устанавливаем флаг финального сообщения
					head.fin = (stop == size);
					// Формируем чанк бинарных данных
					chunk.assign(message + start, message + stop);
					// Выполняем отправку чанка на сервер
					sendFn(head, chunk.data(), chunk.size());
					// Выполняем сброс RSV1
					head.rsv[0] = false;
					// Устанавливаем опкод сообщения
					head.optcode = frame_t::opcode_t::CONTINUATION;
					// Увеличиваем смещение в буфере
					start += this->frameSize;
				}
			// Если фрагментация сообщения не требуется
			} else sendFn(head, message, size);
		}
	}
}
/**
 * stop Метод остановки клиента
 */
void awh::Client::stop() noexcept {
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
		// Если функция обратного вызова установлена, выполняем
		if(this->openStopFn != nullptr) this->openStopFn(false, this);
	}
}
/**
 * pause Метод установки на паузу клиента
 */
void awh::Client::pause() noexcept {
	// Ставим работу клиента на паузу
	this->freeze = true;
}
/**
 * start Метод запуска клиента
 */
void awh::Client::start() noexcept {
	// Снимаем с паузы клиент
	this->freeze = false;
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
					else this->fmk->log("broken connect to host %s", fmk_t::log_t::CRITICAL, this->logfile, this->url.ip.c_str());
				}
				// Останавливаем работу системы
				this->stop();
			};
			// Создаем новую базу
			this->base = event_base_new();
			// Определяем тип подключения
			switch(this->net.family){
				// Резолвер IPv4, создаём резолвер
				case AF_INET: this->dns = new dns_t(this->fmk, this->nwk, this->base, this->net.v4.second, this->logfile); break;
				// Резолвер IPv6, создаём резолвер
				case AF_INET6: this->dns = new dns_t(this->fmk, this->nwk, this->base, this->net.v6.second, this->logfile); break;
			}
			// Если IP адрес не получен
			if(this->url.ip.empty() && !this->url.domain.empty())
				// Выполняем резолвинг домена
				this->resolve(this->url, runFn);
			// Выполняем запуск системы
			else if(!this->url.ip.empty()) runFn(this->url.ip);
			// Выводим в консоль информацию
			this->fmk->log("[+] start service: pid = %u", fmk_t::log_t::INFO, this->logfile, getpid());
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
				this->fmk->log("[=] restart service: pid = %u", fmk_t::log_t::INFO, this->logfile, getpid());
				// Выполняем переподключение
				this->start();
			// Выводим в консоль информацию
			} else this->fmk->log("[-] stop service: pid = %u", fmk_t::log_t::INFO, this->logfile, getpid());
		// Если происходит ошибка то игнорируем её
		} catch(const bad_alloc&) {
			// Выводим сообщение об ошибке
			fmk->log("%s", fmk_t::log_t::CRITICAL, this->logfile, "memory could not be allocated");
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
 * setSub Метод установки подпротокола поддерживаемого сервером
 * @param sub подпротокол для установки
 */
void awh::Client::setSub(const string & sub) noexcept {
	// Устанавливаем подпротокол
	if(!sub.empty()) this->subs.push_back(sub);
}
/**
 * setSubs Метод установки списка подпротоколов поддерживаемых сервером
 * @param subs подпротоколы для установки
 */
void awh::Client::setSubs(const vector <string> & subs) noexcept {
	// Если список подпротоколов получен
	if(!subs.empty()) this->subs = subs;
}
/**
 * setVerifySSL Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
 * @param mode флаг состояния разрешения проверки
 */
void awh::Client::setVerifySSL(const bool mode) noexcept {
	// Выполняем установку флага проверки домена
	this->ssl->setVerify(mode);
}
/**
 * setFrameSize Метод установки размеров сегментов фрейма
 * @param size минимальный размер сегмента
 */
void awh::Client::setFrameSize(const size_t size) noexcept {
	// Если размер передан, устанавливаем
	if(size > 0) this->frameSize = size;
}
/**
 * setAutoReconnect Метод установки флага автоматического переподключения
 * @param mode флаг автоматического переподключения
 */
void awh::Client::setAutoReconnect(const bool mode) noexcept {
	// Устанавливаем флаг автоматического переподключения
	this->reconnect = mode;
}
/**
 * setFamily Метод установки тип протокола интернета
 * @param family тип протокола интернета AF_INET или AF_INET6
 */
void awh::Client::setFamily(const int family) noexcept {
	// Устанавливаем тип активного интернет-подключения
	this->net.family = family;
}
/**
 * setUserAgent Метод установки User-Agent для HTTP запроса
 * @param userAgent агент пользователя для HTTP запроса
 */
void awh::Client::setUserAgent(const string & userAgent) noexcept {
	// Устанавливаем UserAgent
	if(!userAgent.empty()) this->http->setUserAgent(userAgent);
}
/**
 * setUser Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::Client::setUser(const string & login, const string & password) noexcept {
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
void awh::Client::setCA(const string & cafile, const string & capath) noexcept {
	// Устанавливаем адрес CA-файла
	this->ssl->setCA(cafile, capath);
}
/**
 * setNet Метод установки параметров сети
 * @param ip     список IP адресов компьютера с которых разрешено выходить в интернет
 * @param ns     список серверов имён, через которые необходимо производить резолвинг доменов
 * @param family тип протокола интернета AF_INET или AF_INET6
 */
void awh::Client::setNet(const vector <string> & ip, const vector <string> & ns, const int family) noexcept {
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
void awh::Client::setCrypt(const string & pass, const string & salt, const hash_t::aes_t aes) noexcept {
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
void awh::Client::setAuthType(const auth_t::type_t type, const auth_t::algorithm_t algorithm) noexcept {
	// Если объект авторизации создан
	if(this->http != nullptr) this->http->setAuthType(type, algorithm);
}
/**
 * Client Конструктор
 * @param fmk     объект фреймворка
 * @param uri     объект работы с URI
 * @param nwk     объект методов для работы с сетью
 * @param logfile адрес файла для сохранения логов
 */
awh::Client::Client(const fmk_t * fmk, const uri_t * uri, const network_t * nwk, const char * logfile) noexcept {
	try {
		// Устанавливаем зависимые модули
		this->fmk     = fmk;
		this->uri     = uri;
		this->nwk     = nwk;
		this->logfile = logfile;
		// Резервируем память для работы с данными WebSocket
		this->wdt = new char[BUFFER_CHUNK];
		// Создаём объект для работы с компрессией/декомпрессией
		this->hash = new hash_t(this->fmk, this->logfile);
		// Создаём объект для работы с фреймом WebSocket
		this->frame = new frame_t(this->fmk, this->logfile);
		// Создаём объект для работы с SSL
		this->ssl = new ssl_t(this->fmk, this->uri, this->logfile);
		// Создаём объект для работы с HTTP
		this->http = new chttp_t(this->fmk, this->uri, &this->url, this->logfile);
	// Если происходит ошибка то игнорируем её
	} catch(const bad_alloc&) {
		// Выводим сообщение об ошибке
		fmk->log("%s", fmk_t::log_t::CRITICAL, logfile, "memory could not be allocated");
		// Выходим из приложения
		exit(EXIT_FAILURE);
	}
}
/**
 * ~Client Деструктор
 */
awh::Client::~Client() noexcept {
	// Выполняем остановку сервера
	this->stop();
	// Если объект DNS резолвера создан
	if(this->dns != nullptr) delete this->dns;
	// Если объект для работы с SSL создан
	if(this->ssl != nullptr) delete this->ssl;
	// Удаляем объект работы с HTTP
	if(this->http != nullptr) delete this->http;
	// Если объект для компрессии/декомпрессии создан
	if(this->hash != nullptr) delete this->hash;
	// Если объект для работы с фреймом WebSocket создан
	if(this->frame != nullptr) delete this->frame;
	// Если объект данных WebSocket создан
	if(this->wdt != nullptr) delete [] this->wdt;
	// Если - это Windows
	#if defined(_WIN32) || defined(_WIN64)
		// Очищаем сетевой контекст
		this->winSocketClean();
	#endif
}
