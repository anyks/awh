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
 * key Метод генерации ключа для WebSocket
 * @return сгенерированный ключ для WebSocket
 */
const string awh::Client::key() const noexcept {
	// Результат работы функции
	string result = "";
	// Выполняем перехват ошибки
	try {
		// Создаём контейнер
		string nonce = "";
		// Адаптер для работы с случайным распределением
		random_device rd;
		// Резервируем память
		nonce.reserve(16);
		// Формируем равномерное распределение целых чисел в выходном инклюзивно-эксклюзивном диапазоне
		uniform_int_distribution <u_short> dist(0, 255);
		// Формируем бинарный ключ из случайных значений
		for(size_t c = 0; c < 16; c++) nonce += static_cast <char> (dist(rd));
		// Выполняем создание ключа
		result = base64_t().encode(nonce);
	// Выполняем прехват ошибки
	} catch(const exception & error) {
		// Выводим в лог сообщение
		this->fmk->log("%s", fmk_t::log_t::CRITICAL, this->logfile, error.what());
		// Выполняем повторно генерацию ключа
		result = this->key();
	}
	// Выводим результат
	return result;
}
/**
 * date Метод получения текущей даты для HTTP запроса
 * @return текущая дата
 */
const string awh::Client::date() const noexcept {
	// Создаём буфер данных
	char buffer[1000];
	// Получаем текущее время
	time_t now = time(nullptr);
	// Извлекаем текущее время
	struct tm tm = * gmtime(&now);
	// Зануляем буфер
	memset(buffer, 0, sizeof(buffer));
	// Получаем формат времени
	strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S %Z", &tm);
	// Выводим результат
	return buffer;
}
/**
 * request Метод выполнения HTTP запроса
 */
void awh::Client::request() noexcept {
	// Если подключение установленно
	if(this->bev != nullptr){
		// Получаем копию URL данных
		auto url = this->url;
		// Удаляем IP адрес если домен существует
		if(!url.domain.empty()) url.ip.clear();
		// Генерируем URL адрес запроса
		const string & origin = this->uri->createOrigin(url);
		// Получаем путь HTTP запроса
		const string & path = this->uri->joinPath(this->url.path);
		// Получаем параметры запроса
		const string & params = this->uri->joinParams(this->url.params);
		// Получаем хост запроса
		const string & host = (!url.domain.empty() ? url.domain : url.ip);
		// Если хост получен
		if(!host.empty() && !path.empty()){
			// Список желаемых подпротоколов и желаемая компрессия
			string subs = "", compress = "";
			// Получаем параметры авторизации
			const string & auth = this->auth->header();
			// Формируем HTTP запрос
			const string & query = this->fmk->format("%s%s", path.c_str(), (!params.empty() ? params.c_str() : ""));
			// Если подпротоколы существуют
			if(!this->http.subs.empty()){
				// Если количество подпротоколов больше 5-ти
				if(this->http.subs.size() > 5){
					// Переходим по всему списку подпротоколов
					for(auto & sub : this->http.subs){
						// Если подпротокол уже не пустой, добавляем разделитель
						if(!subs.empty()) subs.append(", ");
						// Добавляем в список желаемый подпротокол
						subs.append(sub);
					}
					// Формируем заголовок
					subs.insert(0, "Sec-WebSocket-Protocol: ");
					// Формируем сепаратор
					subs.append("\r\n");
				// Если подпротоколов слишком много
				} else {
					// Переходим по всему списку подпротоколов
					for(auto & sub : this->http.subs){
						// Формируем список подпротоколов
						subs.append(this->fmk->format("Sec-WebSocket-Protocol: %s\r\n", sub.c_str()));
					}
				}
			}
			// Если требуется компрессия данных
			if(this->gzip)
				// Устанавливаем параметры желаемой компрессии
				compress = "Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits\r\n";
			// Строка HTTP запроса
			const string & request = this->fmk->format(
				"GET %s %s\r\n"
				"Host: %s\r\n"
				"Date: %s\r\n"
				"Origin: %s\r\n"
				"User-Agent: %s\r\n"
				"Connection: Upgrade\r\n"
				"Upgrade: websocket\r\n"
				"Sec-WebSocket-Version: %u\r\n"
				"Sec-WebSocket-Key: %s\r\n"
				"%s%s%s\r\n",
				query.c_str(), HTTP_VERSION,
				host.c_str(), this->date().c_str(),
				origin.c_str(), this->userAgent.c_str(),
				WS_VERSION, this->key().c_str(),
				(!compress.empty() ? compress.c_str() : ""),
				(!subs.empty() ? subs.c_str() : ""),
				(!auth.empty() ? auth.c_str() : "")
			);
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
		const size_t size = evbuffer_get_length(input);
		// Если данные существуют
		if(size > 0){
			// Получаем объект подключения
			client_t * ws = reinterpret_cast <client_t *> (ctx);
			// Если статус режима работы находится на этапе сборки данных
			if(ws->http.state != http_t::state_t::HANDSHAKE){
				// Количество прочитанных байт
				size_t count = 0;
				// Если статус режим работы находится на этапе получения ответа
				if(ws->http.state == http_t::state_t::RESPONSE)
					// Очищаем выбранный подпротокол
					ws->http.clear();
				// Считываем данные из буфера до тех пор пока можешь считать
				do {
					// Считываем строки из буфера
					const char * str = evbuffer_readln(input, &count, EVBUFFER_EOL_CRLF_STRICT);
					// Если данные не найдены тогда выходим
					if(str == nullptr) break;
					// Получаем строку ответа
					const string response(str, count);
					// Определяем статус режима работы
					switch((u_short) ws->http.state){
						// Если - это режим ожидания ответа сервера
						case (u_short) http_t::state_t::RESPONSE: {
							// Выполняем поиск первого пробела
							size_t first = response.find(" ");
							// Если пробел получен
							if(first == 8){
								// Получаем версию протокол запроса
								ws->http = stod(response.substr(5, first));
								// Выполняем поиск второго пробела
								const size_t second = response.find(" ", first + 1);
								// Если пробел получен
								if(second != string::npos){
									// Выполняем смену стейта
									ws->http.inc();
									// Получаем сообщение сервера
									ws->http = response.substr(second + 1);
									// Получаем код ответа
									ws->http = (u_int) stoi(response.substr(first + 1, second));
								}
							}
						} break;
						// Если - это режим получения заголовков
						case (u_short) http_t::state_t::HEADERS: {
							// Выполняем поиск пробела
							const size_t pos = response.find(": ");
							// Если разделитель найден
							if(pos != string::npos){
								// Получаем значение заголовка
								const string & val = response.substr(pos + 2);
								// Получаем ключ заголовка
								const string & key = ws->fmk->toLower(response.substr(0, pos));
								// Добавляем заголовок в список заголовков
								if(!key.empty() && !val.empty())
									// Добавляем заголовок в список
									ws->http = make_pair(key, val);
							}

						} break;
					}
				// Если заголовки существуют
				} while(count > 0);

				/**
				 * checkUpgradeFn Функция проверки параметров HTTP запроса на легитимность
				 * @return результат проверки
				 */
				auto checkUpgradeFn = [ws]() -> bool {
					// Результат работы функции
					bool result = false;
					// Если список заголовков получен
					if(!ws->http.headers.empty()){
						// Выполняем поиск заголовка смены протокола
						auto it = ws->http.headers.find("upgrade");
						// Выполняем поиск заголовка с параметрами подключения
						auto jt = ws->http.headers.find("connection");
						// Если заголовки расширений найдены
						if((it != ws->http.headers.end()) && (jt != ws->http.headers.end())){
							// Получаем значение заголовка Upgrade
							const string & upgrade = ws->fmk->toLower(it->second);
							// Получаем значение заголовка Connection
							const string & connection = ws->fmk->toLower(jt->second);
							// Если заголовки соответствуют
							result = ((upgrade.compare("websocket") == 0) && (connection.compare("upgrade") == 0));
						}
					}
					// Выводим результат
					return result;
				};
				/**
				 * checkExtensionsFn Функция проверки расширений протокола
				 */
				auto checkExtensionsFn = [ws]{
					// Отключаем сжатие ответа с сервера
					ws->gzip = false;
					// Список расширений
					vector <wstring> extensions;
					// Выполняем поиск расширений
					auto it = ws->http.headers.find("sec-websocket-extensions");
					// Если заголовки расширений найдены
					if(it != ws->http.headers.end()){
						// Выполняем разделение параметров расширений
						ws->fmk->split(it->second, ";", extensions);
						// Если список параметров получен
						if(!extensions.empty()){
							// Ищем поддерживаемые заголовки
							for(auto & val : extensions){
								// Если получены заголовки требующие сжимать передаваемые фреймы
								if((val.compare(L"permessage-deflate") == 0) || (val.compare(L"perframe-deflate") == 0))
									// Устанавливаем требование выполнять компрессию полезной нагрузки
									ws->gzip = true;
								// Если размер окна в битах получено
								// else if(val.find(L"client_max_window_bits") != wstring::npos)
									// Устанавливаем максимальный размер окна для сжатия в GZIP
								//	ws->auth->setWbit(stoi(val.substr(23)));
							}
						}
					}
				};
				/**
				 * checkKeyFn Функция проверки ключа сервера
				 * @return результат проверки
				 */
				/*
				auto checkKeyFn = [ws]() -> bool {
					// Результат работы функции
					bool result = false;
					// Получаем параметры ключа сервера
					auto it = ws->http.headers.find("sec-websocket-accept");
					// Если параметры авторизации найдены
					if(it != ws->http.headers.end()){
						// Получаем ключ для проверки
						const string & key = this->generateKey(this->client.key);
						// Если ключи не соответствуют, запрещаем работу
						result = (key.compare(it->second) == 0);
					}
					// Выводим результат
					return result;
				};
				*/
				/**
				 * checkAuthFn Функция проверки авторизации пользователя на сервере
				 * @return результат проверки авторизации
				 */
				auto checkAuthFn = [ws]() -> bool {
					// Результат работы функции
					bool result = false;
					// Если требуется авторизация
					if(ws->http.code == 401){
						/**
						// Получаем параметры авторизации
						auto it = ws->http.headers.find("www-authenticate");
						// Если параметры авторизации найдены
						if(it != ws->http.headers.end())
							// Устанавливаем заголовок HTTP в параметры авторизации
							this->client.auth.setHeader(it->second);
						*/
					// Иначе разрешаем авторизацию
					} else result = true;
					// Выводим результат
					return result;
				};
				/**
				 * selectSubFn Функция выбора подпротокола для дальнейшей работы
				 */
				auto selectSubFn = [ws]{
					// Ищем подпротокол сервера
					auto it = ws->http.headers.find("sec-websocket-protocol");
					// Если подпротокол найден, устанавливаем его
					if(it != ws->http.headers.end()) ws->http.sub = it->second;
				};
				// Если HTTP заголовки переданы правильно
				if(!checkUpgradeFn()){
					// Останавливаем работу клиента
					ws->stop();
					// Вызываем функцию обратного вызова
					// callback(this->status, code_t::NOUPGRADE, "Http response is not supported by the client", true);
					// Выходим из функции
					return;
				}
				// Выполняем проверку расширений протокола
				checkExtensionsFn();
				/*
				// Выполняем проверку на ключ сервера
				if(!checkKeyFn()){
					// Вызываем функцию обратного вызова
					// callback(this->status, code_t::NOKEY, "Server key is invalid", true);
					// Выходим из функции
					return;
				}
				*/
				// Если авторизация не прошла
				if(!checkAuthFn()){
					// Вызываем функцию обратного вызова
					// callback(this->status, code_t::NOAUTH, "Login or password not found", true);
					// Выходим из функции
					return;
				}
				// Выполняем поиск подпротокола для дальнейшей работы
				selectSubFn();
				// Выполняем смену стейта
				ws->http.inc();
			// Если рукопожатие выполнено
			} else {
				cout << " ============ " << ws->http.version << " == " << ws->http.code << " == " << ws->http.message << endl;

				for(auto & item : ws->http.headers){
					cout << " +++++++++++ " << item.first << " == " << item.second << endl;
				}
			}
		}
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
	// Останавливаем таймер подключения
	this->timer.stop();
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
	}
}
/**
 * pause Метод установки на паузу клиента
 */
void awh::Client::pause() noexcept {

}
/**
 * start Метод запуска клиента
 */
void awh::Client::start() noexcept {
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
	if(!sub.empty()) this->http.subs.emplace(sub);
}
/**
 * setSubs Метод установки списка подпротоколов поддерживаемых сервером
 * @param subs подпротоколы для установки
 */
void awh::Client::setSubs(const vector <string> & subs) noexcept {
	// Если список подпротоколов получен
	if(!subs.empty()){
		// Переходим по всем подпротоколам
		for(auto & sub : subs)
			// Устанавливаем подпротокол
			this->setSub(sub);
	}
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
	if(!userAgent.empty()) this->userAgent = userAgent;
}
/**
 * setFrameSize Метод установки размеров сегментов фрейма
 * @param min минимальный размер сегмента
 * @param max максимальный размер сегмента
 */
void awh::Client::setFrameSize(const size_t min, const size_t max) noexcept {
	// Устанавливаем минимальный размер фрейма
	if((min > 0) && (min < max)) this->min = min;
	// Устанавливаем максимальный размер фрейма
	if(max > this->min) this->max = max;
	// Если максимальное значение меньше минимального, корректируем
	if(this->max < this->min){
		// Минимальный размер сегмента
		this->min = MIN_FRAME_SIZE;
		// Максимальный размер сегмента
		this->max = MAX_FRAME_SIZE;
		// Выводим сообщение об ошибке
		this->fmk->log("%s", fmk_t::log_t::WARNING, this->logfile, "the set maximum value is less than the set minimum value");
	}
}
/**
 * setUser Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::Client::setUser(const string & login, const string & password) noexcept {
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
 * setAuthType Метод установки типа авторизации
 * @param type      тип авторизации
 * @param algorithm алгоритм шифрования для Digest авторизации
 */
void awh::Client::setAuthType(const auth_t::type_t type, const auth_t::algorithm_t algorithm) noexcept {
	// Если объект авторизации создан
	if(this->auth != nullptr) this->auth->setType(type, algorithm);
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
		// Создаём объект для работы с авторизацией
		this->auth = new auth_t(this->fmk, this->logfile);
		// Создаём объект для работы с SSL
		this->ssl = new ssl_t(this->fmk, this->uri, this->logfile);
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
	// Удаляем объект авторизации
	if(this->auth != nullptr) delete this->auth;
	// Если - это Windows
	#if defined(_WIN32) || defined(_WIN64)
		// Очищаем сетевой контекст
		this->winSocketClean();
	#endif
}
