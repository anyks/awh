/**
 * @file: engine.cpp
 * @date: 2022-08-03
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2022
 */

// Подключаем заголовочный файл
#include <net/engine.hpp>

// Буфер секретного слова печенок
u_char awh::Engine::_cookies[16];
// Флаг инициализации куков
bool awh::Engine::_cookieInit = false;

/**
 * client Метод извлечения данных клиента
 */
void awh::Engine::Address::client() noexcept {
	// Определяем тип подключения
	switch(this->_peer.client.ss_family){
		// Для протокола IPv4
		case AF_INET:
		// Для протокола IPv6
		case AF_INET6: {
			// Буфер для получения IP адреса
			char buffer[INET6_ADDRSTRLEN];
			// Выполняем зануление буфера данных
			memset(buffer, 0, sizeof(buffer));
			// Определяем тип протокола интернета
			switch(this->_peer.client.ss_family){
				// Если протокол интернета IPv4
				case AF_INET: {
					// Получаем порт клиента
					this->port = ntohs(((struct sockaddr_in *) &this->_peer.client)->sin_port);
					// Получаем IP адрес
					this->ip = inet_ntop(AF_INET, &((struct sockaddr_in *) &this->_peer.client)->sin_addr, buffer, sizeof(buffer));
				} break;
				// Если протокол интернета IPv6
				case AF_INET6: {
					// Получаем порт клиента
					this->port = ntohs(((struct sockaddr_in6 *) &this->_peer.client)->sin6_port);
					// Получаем IP адрес
					this->ip = inet_ntop(AF_INET6, &((struct sockaddr_in6 *) &this->_peer.client)->sin6_addr, buffer, sizeof(buffer));
				} break;
			}
			// Получаем данные подключившегося клиента
			string ip = this->_ifnet.ip((struct sockaddr *) &this->_peer.client, this->_peer.client.ss_family);
			// Если IP адрес получен пустой
			if((ip.compare("0.0.0.0") == 0) || (ip.compare("::") == 0)){
				// Получаем IP адрес локального сервера
				ip = this->_ifnet.ip(this->_peer.client.ss_family);
				// Получаем данные MAC адреса внутреннего клиента
				this->mac = this->_ifnet.mac(ip, this->_peer.client.ss_family);
			// Получаем данные MAC адреса внешнего клиента
			} else this->mac = this->_ifnet.mac(this->ip, this->_peer.client.ss_family);
		} break;
		/**
		 * Если операционной системой не является Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Для протокола unix-сокета
			case AF_UNIX: {
				// Устанавливаем адрес сервера
				this->ip = this->_ifnet.ip(AF_INET);
				// Получаем данные мак адреса клиента
				this->mac = this->_ifnet.mac(this->ip, AF_INET);
			} break;
		#endif
	}
}
/**
 * list Метод активации прослушивания сокета
 * @return результат выполнения операции
 */
bool awh::Engine::Address::list() noexcept {
	// Результат работы функции
	bool result = false;
	// Определяем тип сокета
	switch(this->_type){
		// Если сокет установлен TCP/IP
		case SOCK_STREAM: {
			// Выполняем слушать порт сервера
			if(!(result = (::listen(this->fd, SOMAXCONN) == 0))){
				// Выводим сообщени об активном сервисе
				this->_log->print("listen service: pid = %u", log_t::flag_t::CRITICAL, getpid());
				// Выходим из функции
				return result;
			}
		} break;
		// Если сокет установлен UDP
		case SOCK_DGRAM: return true;
	}
	// Выводим результат
	return result;
}
/**
 * close Метод закрытия подключения
 * @return результат выполнения операции
 */
bool awh::Engine::Address::close() noexcept {
	// Результат работы функции
	bool result = false;
	// Если файловый дескриптор подключён
	if((result = (this->fd > -1))){
		/**
		 * Если операционной системой является Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Запрещаем работу с сокетом
			shutdown(this->fd, SD_BOTH);
			// Выполняем закрытие сокета
			closesocket(this->fd);
		/**
		 * Если операционной системой является Nix-подобная
		 */
		#else
			// Запрещаем работу с сокетом
			shutdown(this->fd, SHUT_RDWR);
			// Выполняем закрытие сокета
			::close(this->fd);
		#endif
		// Выполняем сброс сокета
		this->fd = -1;
		// Сбрасываем флаг инициализации
		this->_tls = false;
		// Зануляем структуру клиента
		memset(&this->_peer.client, 0, sizeof(this->_peer.client));
		// Зануляем структуру сервера
		memset(&this->_peer.server, 0, sizeof(this->_peer.server));
	}
	// Выводим результат
	return result;
}
/**
 * connect Метод выполнения подключения
 * @return результат выполнения операции
 */
bool awh::Engine::Address::connect() noexcept {
	// Устанавливаем статус отключения
	this->status = status_t::DISCONNECTED;
	// Если сокет установлен TCP/IP
	if(this->_type == SOCK_STREAM){
		// Определяем тип подключения
		switch(this->_peer.server.ss_family){
			// Для протокола IPv4
			case AF_INET:
				// Запоминаем размер структуры
				this->_peer.size = sizeof(struct sockaddr_in);
			break;
			// Для протокола IPv6
			case AF_INET6:
				// Запоминаем размер структуры
				this->_peer.size = sizeof(struct sockaddr_in6);
			break;
			/**
			 * Если операционной системой не является Windows
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				// Для протокола unix-сокета
				case AF_UNIX:
					// Получаем размер объекта сокета
					this->_peer.size = (
						offsetof(struct sockaddr_un, sun_path) +
						strlen(((struct sockaddr_un *) (&this->_peer.server))->sun_path)
					);
				break;
			#endif
		}
		// Если подключение не выполненно то сообщаем об этом, выполняем подключение к удаленному серверу
		if((this->_peer.size > 0) && (::connect(this->fd, (struct sockaddr *) (&this->_peer.server), this->_peer.size) == 0))
			// Устанавливаем статус подключения
			this->status = status_t::CONNECTED;
	// Если сокет установлен UDP
	} else if(this->_type == SOCK_DGRAM) {
		// Если подключение зашифрованно, значит мы должны использовать DTLS
		if(this->_tls){
			// Определяем тип подключения
			switch(this->_peer.server.ss_family){
				// Для протокола IPv4
				case AF_INET:
					// Запоминаем размер структуры
					this->_peer.size = sizeof(struct sockaddr_in);
				break;
				// Для протокола IPv6
				case AF_INET6:
					// Запоминаем размер структуры
					this->_peer.size = sizeof(struct sockaddr_in6);
				break;
			}
			// Если подключение не выполненно то сообщаем об этом, выполняем подключение к удаленному серверу
			if((this->_peer.size > 0) && (::connect(this->fd, (struct sockaddr *) (&this->_peer.server), this->_peer.size) == 0))
				// Устанавливаем статус подключения
				this->status = status_t::CONNECTED;
		// Устанавливаем статус подключения
		} else this->status = status_t::CONNECTED;
	}
	// Если подключение выполнено
	if(this->status == status_t::CONNECTED)
		// Выполняем извлечение данных клиента
		this->client();
	// Выводим результат
	return (this->status == status_t::CONNECTED);
}
/**
 * attach Метод прикрепления клиента к серверу
 * @param addr объект подключения сервера
 */
bool awh::Engine::Address::attach(Address & addr) noexcept {
	// Устанавливаем статус отключения
	this->status = status_t::DISCONNECTED;
	// Определяем тип подключения
	switch(addr._peer.server.ss_family){
		// Для протокола IPv4
		case AF_INET:
			// Запоминаем размер структуры
			this->_peer.size = sizeof(struct sockaddr_in);
		break;
		// Для протокола IPv6
		case AF_INET6:
			// Запоминаем размер структуры
			this->_peer.size = sizeof(struct sockaddr_in6);
		break;
	}
	// Создаем сокет подключения
	this->fd = ::socket(addr._peer.server.ss_family, addr._type, 0);
	// Устанавливаем разрешение на повторное использование сокета
	this->_socket.reuseable(this->fd);
	// Выполняем копирование объекта подключения клиента
	memcpy(&this->_peer.client, &addr._peer.client, sizeof(struct sockaddr_storage));
	// Выполняем копирование объекта подключения сервера
	memcpy(&this->_peer.server, &addr._peer.server, sizeof(struct sockaddr_storage));
	// Выполняем бинд на сокет
	if((this->_peer.size > 0) && (::bind(this->fd, (struct sockaddr *) (&this->_peer.server), this->_peer.size) < 0)){
		// Хост подключённого клиента
		const string & client = this->_ifnet.ip((struct sockaddr *) &this->_peer.client, this->_peer.client.ss_family);
		// Выводим в лог сообщение
		this->_log->print("bind client for UDP protocol [%s]", log_t::flag_t::CRITICAL, client.c_str());
		// Выходим
		return false;
	}
	// Если подключение не выполненно то сообщаем об этом, выполняем подключение к удаленному серверу
	if(::connect(this->fd, (struct sockaddr *) (&this->_peer.client), this->_peer.size) == 0){
		// Выполняем извлечение данных клиента
		this->client();
		// Устанавливаем статус подключения
		this->status = status_t::ATTACHED;
	}
	// Выводим результат
	return (this->status == status_t::ATTACHED);
}
/**
 * accept Метод согласования подключения
 * @param addr объект подключения сервера
 * @return     результат выполнения операции
 */
bool awh::Engine::Address::accept(Address & addr) noexcept {
	// Выполняем вызов метода согласование
	return this->accept(addr.fd, addr._peer.server.ss_family);
}
/**
 * accept Метод согласования подключения
 * @param fd     файловый дескриптор сервера
 * @param family семейство сокета (AF_INET / AF_INET6 / AF_UNIX)
 * @return       результат выполнения операции
 */
bool awh::Engine::Address::accept(const int fd, const int family) noexcept {
	// Устанавливаем статус отключения
	this->status = status_t::DISCONNECTED;
	// Определяем тип сокета
	switch(this->_type){
		// Если сокет установлен TCP/IP
		case SOCK_STREAM: {
			// Определяем тип подключения
			switch(family){
				// Для протокола IPv4
				case AF_INET: {
					// Создаём объект клиента
					struct sockaddr_in client;
					// Очищаем всю структуру для клиента
					memset(&client, 0, sizeof(client));
					// Устанавливаем протокол интернета
					client.sin_family = family;
					// Запоминаем размер структуры
					this->_peer.size = sizeof(client);
					// Выполняем копирование объекта подключения клиента
					memcpy(&this->_peer.client, &client, this->_peer.size);
				} break;
				// Для протокола IPv6
				case AF_INET6: {
					// Создаём объект клиента
					struct sockaddr_in6 client;
					// Очищаем всю структуру для клиента
					memset(&client, 0, sizeof(client));
					// Устанавливаем протокол интернета
					client.sin6_family = family;
					// Запоминаем размер структуры
					this->_peer.size = sizeof(client);
					// Выполняем копирование объекта подключения клиента
					memcpy(&this->_peer.client, &client, this->_peer.size);
				} break;
				/**
				 * Если операционной системой не является Windows
				 */
				#if !defined(_WIN32) && !defined(_WIN64)
					// Для протокола unix-сокета
					case AF_UNIX: {
						// Создаём объект подключения для клиента
						struct sockaddr_un client;
						// Очищаем всю структуру для клиента
						memset(&client, 0, sizeof(client));
						// Устанавливаем протокол интернета
						client.sun_family = family;
						// Запоминаем размер структуры
						this->_peer.size = sizeof(client);
						// Выполняем копирование объект подключения клиента в сторейдж
						memcpy(&this->_peer.client, &client, sizeof(client));
					} break;
				#endif
			}
			// Определяем разрешено ли подключение к прокси серверу
			this->fd = ::accept(fd, (struct sockaddr *) (&this->_peer.client), &this->_peer.size);
			// Если сокет не создан тогда выходим
			if(this->fd < 0) return false;
		} break;
		// Если сокет установлен UDP
		case SOCK_DGRAM: this->fd = fd; break;
	}
	// Определяем тип подключения
	switch(this->_peer.client.ss_family){
		// Для протокола IPv4
		case AF_INET:
		// Для протокола IPv6
		case AF_INET6: {
			/**
			 * Если операционной системой является Nix-подобная
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				// Выполняем игнорирование сигнала неверной инструкции процессора
				this->_socket.noSigill();
				// Если сокет установлен TCP/IP
				if(this->_type == SOCK_STREAM){
					// Отключаем сигнал записи в оборванное подключение
					this->_socket.noSigpipe(this->fd);
					// Активируем KeepAlive
					this->_socket.keepAlive(this->fd, this->alive.cnt, this->alive.idle, this->alive.intvl);
				}
			/**
			 * Если операционной системой является MS Windows
			 */
			#else
				// Если сокет установлен TCP/IP
				if(this->_type == SOCK_STREAM)
					// Активируем KeepAlive
					this->_socket.keepAlive(this->fd);
			#endif
			// Если сокет установлен TCP/IP
			if(this->_type == SOCK_STREAM){
				// Устанавливаем разрешение на повторное использование сокета
				this->_socket.reuseable(this->fd);
				// Отключаем алгоритм Нейгла для сервера и клиента
				this->_socket.tcpNodelay(this->fd);
				// Переводим сокет в не блокирующий режим
				this->_socket.nonBlocking(this->fd);
				// Отключаем алгоритм Нейгла
				BIO_set_tcp_ndelay(this->fd, 1);
			}
		} break;
		/**
		 * Если операционной системой не является Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Для протокола unix-сокета
			case AF_UNIX: {
				// Выполняем игнорирование сигнала неверной инструкции процессора
				this->_socket.noSigill();
				// Если сокет установлен TCP/IP
				if(this->_type == SOCK_STREAM){
					// Отключаем сигнал записи в оборванное подключение
					this->_socket.noSigpipe(this->fd);
					// Устанавливаем разрешение на повторное использование сокета
					this->_socket.reuseable(this->fd);
					// Переводим сокет в не блокирующий режим
					this->_socket.nonBlocking(this->fd);
				}
			} break;
		#endif
	}
	// Выполняем извлечение данных клиента
	this->client();
	// Устанавливаем статус подключения
	this->status = status_t::ACCEPTED;
	// Выводим результат
	return true;
}
/**
 * sonet Метод установки параметров сокета
 * @param type     тип сокета (SOCK_STREAM / SOCK_DGRAM)
 * @param protocol протокол сокета (IPPROTO_TCP / IPPROTO_UDP)
 */
void awh::Engine::Address::sonet(const int type, const int protocol) noexcept {
	// Устанавливаем тип сокета
	this->_type = type;
	// Устанавливаем протокол сокета
	this->_protocol = protocol;
}
/**
 * init Метод инициализации адресного пространства сокета
 * @param unixsocket адрес unxi-сокета в файловой системе
 * @param type       тип приложения (клиент или сервер)
 */
void awh::Engine::Address::init(const string & unixsocket, const type_t type) noexcept {
	// Если unix-сокет передан
	if(!unixsocket.empty()){		
		/**
		 * Если операционной системой не является Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Если приложение является сервером
			if(type == type_t::SERVER){
				// Если сокет в файловой системе уже существует, удаляем его
				if(fs_t::issock(unixsocket))
					// Удаляем файл сокета
					::unlink(unixsocket.c_str());
			}
			// Создаем сокет подключения
			this->fd = ::socket(AF_UNIX, this->_type, 0);
			// Если сокет не создан то выходим
			if(this->fd < 0){
				// Выводим сообщение в консоль
				this->_log->print("creating socket %s", log_t::flag_t::CRITICAL, unixsocket.c_str());
				// Выходим
				return;
			}
			// Выполняем игнорирование сигнала неверной инструкции процессора
			this->_socket.noSigill();
			// Устанавливаем разрешение на повторное использование сокета
			this->_socket.reuseable(this->fd);
			// Если сокет установлен TCP/IP
			if(this->_type == SOCK_STREAM){
				// Отключаем сигнал записи в оборванное подключение
				this->_socket.noSigpipe(this->fd);
				// Переводим сокет в не блокирующий режим
				this->_socket.nonBlocking(this->fd);
			}
			// Создаём объект подключения для клиента
			struct sockaddr_un client;
			// Создаём объект подключения для сервера
			struct sockaddr_un server;
			// Зануляем изначальную структуру данных клиента
			memset(&client, 0, sizeof(client));
			// Зануляем изначальную структуру данных сервера
			memset(&server, 0, sizeof(server));
			// Устанавливаем размер объекта подключения
			this->_peer.size = sizeof(struct sockaddr_un);
			// Определяем тип сокета
			switch(this->_type){
				// Если сокет установлен TCP/IP
				case SOCK_STREAM: {
					// Устанавливаем протокол интернета
					server.sun_family = AF_UNIX;
					// Очищаем всю структуру для сервера
					memset(&server.sun_path, 0, sizeof(server.sun_path));
					// Копируем адрес сокета сервера
					strncpy(server.sun_path, unixsocket.c_str(), sizeof(server.sun_path));
					// Выполняем копирование объект подключения сервера в сторейдж
					memcpy(&this->_peer.server, &server, sizeof(server));
				} break;
				// Если сокет установлен UDP
				case SOCK_DGRAM: {
					// Устанавливаем протокол интернета для клиента
					client.sun_family = AF_UNIX;
					// Устанавливаем протокол интернета для сервера
					server.sun_family = AF_UNIX;
					// Очищаем всю структуру для клиента
					memset(&client.sun_path, 0, sizeof(client.sun_path));
					// Очищаем всю структуру для сервера
					memset(&server.sun_path, 0, sizeof(server.sun_path));
					// Выполняем поиск последнего слеша разделителя
					const size_t pos = unixsocket.rfind("/");
					// Если разделитель найден
					if(pos != string::npos){
						// Получаем название файла unix-сокета
						const string name = unixsocket.substr(pos + 1);
						// Получаем путь к файлу unix-сокета
						const string path = unixsocket.substr(0, pos + 1);
						// Создаём адрес unix-сокета клиента
						const string & clientName = this->_fmk->format("%sc_%s", path.c_str(), name.c_str());
						// Создаём адрес unix-сокета сервера
						const string & serverName = this->_fmk->format("%ss_%s", path.c_str(), name.c_str());
						// Определяем тип приложения
						switch((uint8_t) type){
							// Если приложение является клиентом
							case (uint8_t) type_t::CLIENT: {
								// Если сокет в файловой системе уже существует, удаляем его
								if(fs_t::issock(clientName))
									// Удаляем файл сокета
									::unlink(clientName.c_str());
							} break;
							// Если приложение является сервером
							case (uint8_t) type_t::SERVER: {
								// Если сокет в файловой системе уже существует, удаляем его
								if(fs_t::issock(serverName))
									// Удаляем файл сокета
									::unlink(serverName.c_str());
							} break;
						}
						// Копируем адрес сокета сервера
						strncpy(server.sun_path, serverName.c_str(), sizeof(server.sun_path));
						// Выполняем копирование объект подключения сервера в сторейдж
						memcpy(&this->_peer.server, &server, sizeof(server));
						// Если приложение является клиентом
						if(type == type_t::CLIENT){
							// Копируем адрес сокета клиента
							strncpy(client.sun_path, clientName.c_str(), sizeof(client.sun_path));
							// Выполняем копирование объект подключения клиента в сторейдж
							memcpy(&this->_peer.client, &client, sizeof(client));
							// Получаем размер объекта сокета
							const socklen_t size = (offsetof(struct sockaddr_un, sun_path) + strlen(client.sun_path));
							// Выполняем бинд на сокет
							if(::bind(this->fd, (struct sockaddr *) &this->_peer.client, size) < 0){
								// Выводим в лог сообщение
								this->_log->print("bind local network for client [%s]", log_t::flag_t::CRITICAL, clientName.c_str());
								// Выходим
								return;
							}
						}
					}
				} break;
			}
			// Если приложение является сервером
			if(type == type_t::SERVER){
				// Получаем размер объекта сокета
				const socklen_t size = (offsetof(struct sockaddr_un, sun_path) + strlen(server.sun_path));
				// Выполняем бинд на сокет
				if(::bind(this->fd, (struct sockaddr *) &this->_peer.server, size) < 0)
					// Выводим в лог сообщение
					this->_log->print("bind local network for server [%s]", log_t::flag_t::CRITICAL, unixsocket.c_str());
			}
		#endif
	}
}
/**
 * init Метод инициализации адресного пространства сокета
 * @param ip     адрес для которого нужно создать сокет
 * @param port   порт сервера для которого нужно создать сокет
 * @param family семейство сокета (AF_INET / AF_INET6 / AF_UNIX)
 * @param type   тип приложения (клиент или сервер)
 * @param onlyV6 флаг разрешающий использовать только IPv6 подключение
 * @return       параметры подключения к серверу
 */
void awh::Engine::Address::init(const string & ip, const u_int port, const int family, const type_t type, const bool onlyV6) noexcept {
	// Если IP адрес передан
	if(!ip.empty() && (port > 0) && (port <= 65535) && !this->network.empty()){
		// Если список сетевых интерфейсов установлен
		if((family == AF_INET) || (family == AF_INET6)){
			// Адрес сервера для биндинга
			string host = "";
			// Определяем тип подключения
			switch(family){
				// Для протокола IPv4
				case AF_INET: {
					// Определяем тип приложения
					switch((uint8_t) type){
						// Если приложение является клиентом
						case (uint8_t) type_t::CLIENT: {
							// Если количество элементов больше 1
							if(this->network.size() > 1){
								// рандомизация генератора случайных чисел
								srand(time(0));
								// Получаем ip адрес
								host = this->network.at(rand() % this->network.size());
							// Выводим только первый элемент
							} else host = this->network.front();
							// Создаём объект клиента
							struct sockaddr_in client;
							// Очищаем всю структуру для клиента
							memset(&client, 0, sizeof(client));
							// Устанавливаем протокол интернета
							client.sin_family = family;
							// Устанавливаем произвольный порт для локального подключения
							client.sin_port = htons(0);
							// Устанавливаем адрес для локальго подключения
							client.sin_addr.s_addr = inet_addr(host.c_str());
							// Запоминаем размер структуры
							this->_peer.size = sizeof(client);
							// Выполняем копирование объекта подключения клиента
							memcpy(&this->_peer.client, &client, this->_peer.size);
						} break;
						// Если приложение является сервером
						case (uint8_t) type_t::SERVER: {
							// Создаём объект клиента
							struct sockaddr_in client;
							// Очищаем всю структуру для клиента
							memset(&client, 0, sizeof(client));
							// Устанавливаем протокол интернета
							client.sin_family = family;
							// Выполняем копирование объекта подключения клиента
							memcpy(&this->_peer.client, &client, sizeof(client));
						} break;
					}
					// Создаём объект сервера
					struct sockaddr_in server;
					// Очищаем всю структуру для сервера
					memset(&server, 0, sizeof(server));
					// Устанавливаем протокол интернета
					server.sin_family = family;
					// Устанавливаем порт для локального подключения
					server.sin_port = htons(port);
					// Устанавливаем адрес для удаленного подключения
					server.sin_addr.s_addr = inet_addr(ip.c_str());
					// Если ядро является сервером
					if(type == type_t::SERVER)
						// Запоминаем размер структуры
						this->_peer.size = sizeof(server);
					// Выполняем копирование объекта подключения сервера
					memcpy(&this->_peer.server, &server, this->_peer.size);
					// Обнуляем серверную структуру
					memset(&((struct sockaddr_in *) (&this->_peer.server))->sin_zero, 0, sizeof(server.sin_zero));
				} break;
				// Для протокола IPv6
				case AF_INET6: {
					// Определяем тип приложения
					switch((uint8_t) type){
						// Если приложение является клиентом
						case (uint8_t) type_t::CLIENT: {
							// Если количество элементов больше 1
							if(this->network.size() > 1){
								// рандомизация генератора случайных чисел
								srand(time(0));
								// Получаем ip адрес
								host = this->network.at(rand() % this->network.size());
							// Выводим только первый элемент
							} else host = this->network.front();
							// Переводим ip адрес в полноценный вид
							host = move(this->_nwk.setLowIp6(host));
							// Создаём объект клиента
							struct sockaddr_in6 client;
							// Очищаем всю структуру для клиента
							memset(&client, 0, sizeof(client));
							// Устанавливаем протокол интернета
							client.sin6_family = family;
							// Устанавливаем произвольный порт для локального подключения
							client.sin6_port = htons(0);
							// Указываем адрес IPv6 для клиента
							inet_pton(family, host.c_str(), &client.sin6_addr);
							// inet_ntop(family, &client.sin6_addr, hostClient, sizeof(hostClient));
							// Запоминаем размер структуры
							this->_peer.size = sizeof(client);
							// Выполняем копирование объекта подключения клиента
							memcpy(&this->_peer.client, &client, this->_peer.size);
						} break;
						// Если приложение является сервером
						case (uint8_t) type_t::SERVER: {
							// Создаём объект клиента
							struct sockaddr_in6 client;
							// Очищаем всю структуру для клиента
							memset(&client, 0, sizeof(client));
							// Устанавливаем протокол интернета
							client.sin6_family = family;
							// Выполняем копирование объекта подключения клиента
							memcpy(&this->_peer.client, &client, sizeof(client));
						} break;
					}
					// Создаём объект сервера
					struct sockaddr_in6 server;
					// Очищаем всю структуру для сервера
					memset(&server, 0, sizeof(server));
					// Устанавливаем протокол интернета
					server.sin6_family = family;
					// Устанавливаем порт для локального подключения
					server.sin6_port = htons(port);
					// Указываем адрес IPv6 для сервера
					inet_pton(family, ip.c_str(), &server.sin6_addr);
					// inet_ntop(family, &server.sin6_addr, hostServer, sizeof(hostServer));
					// Если приложение является сервером
					if(type == type_t::SERVER)
						// Запоминаем размер структуры
						this->_peer.size = sizeof(server);
					// Выполняем копирование объекта подключения сервера
					memcpy(&this->_peer.server, &server, this->_peer.size);
				} break;
				// Если тип сети не определен
				default: {
					// Выводим сообщение в консоль
					this->_log->print("network not allow from server = %s, port = %u", log_t::flag_t::CRITICAL, ip.c_str(), port);
					// Выходим
					return;
				}
			}
			// Создаем сокет подключения
			this->fd = ::socket(family, this->_type, this->_protocol);
			// Если сокет не создан то выходим
			if(this->fd < 0){
				// Выводим сообщение в консоль
				this->_log->print("creating socket to server = %s, port = %u", log_t::flag_t::CRITICAL, ip.c_str(), port);
				// Выходим
				return;
			}
			/**
			 * Если операционной системой является Nix-подобная
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				// Выполняем игнорирование сигнала неверной инструкции процессора
				this->_socket.noSigill();
				// Если сокет установлен TCP/IP
				if(this->_type == SOCK_STREAM)
					// Отключаем сигнал записи в оборванное подключение
					this->_socket.noSigpipe(this->fd);
				// Если приложение является сервером
				if(type == type_t::SERVER){
					// Включаем отображение сети IPv4 в IPv6
					if(family == AF_INET6) this->_socket.ipV6only(this->fd, onlyV6);
				// Если приложение является клиентом и сокет установлен TCP/IP
				} else if(this->_type == SOCK_STREAM)
					// Активируем KeepAlive
					this->_socket.keepAlive(this->fd, this->alive.cnt, this->alive.idle, this->alive.intvl);
			/**
			 * Если операционной системой является MS Windows
			 */
			#else
				// Если приложение является сервером
				if(type == type_t::SERVER){
					// Включаем отображение сети IPv4 в IPv6
					if(family == AF_INET6) this->_socket.ipV6only(this->fd, onlyV6);
				// Если приложение является клиентом и сокет установлен TCP/IP
				} else if(this->_type == SOCK_STREAM)
					// Активируем KeepAlive
					this->_socket.keepAlive(this->fd);
			#endif
			// Если сокет установлен TCP/IP
			if(this->_type == SOCK_STREAM){
				// Если приложение является сервером
				if(type == type_t::SERVER)
					// Переводим сокет в не блокирующий режим
					this->_socket.nonBlocking(this->fd);
				// Отключаем алгоритм Нейгла для сервера и клиента
				this->_socket.tcpNodelay(this->fd);
				// Отключаем алгоритм Нейгла
				BIO_set_tcp_ndelay(this->fd, 1);
				// Устанавливаем разрешение на закрытие сокета при неиспользовании
				// this->_socket.closeonexec(this->fd);
			}
			// Устанавливаем разрешение на повторное использование сокета
			this->_socket.reuseable(this->fd);
			// Определяем тип запускаемого приложения
			switch((uint8_t) type){
				// Если приложение является сервером
				case (uint8_t) type_t::SERVER: {
					// Получаем настоящий хост сервера
					host = this->_ifnet.ip(family);
					// Выполняем бинд на сокет
					if(::bind(this->fd, (struct sockaddr *) (&this->_peer.server), this->_peer.size) < 0)
						// Выводим в лог сообщение
						this->_log->print("bind local network [%s]", log_t::flag_t::CRITICAL, host.c_str());
				} break;
				// Если приложение является клиентом
				case (uint8_t) type_t::CLIENT: {
					// Выполняем бинд на сокет
					if(::bind(this->fd, (struct sockaddr *) (&this->_peer.client), this->_peer.size) < 0)
						// Выводим в лог сообщение
						this->_log->print("bind local network [%s]", log_t::flag_t::CRITICAL, host.c_str());
				} break;
			}
		}
	}
}
/**
 * ~Address Деструктор
 */
awh::Engine::Address::~Address() noexcept {
	// Выполняем отключение
	this->close();
}
/**
 * error Метод вывода информации об ошибке
 * @param status статус ошибки
 */
void awh::Engine::Context::error(const int status) const noexcept {
	// Если защищённый режим работы разрешён
	if(this->_tls){
		// Получаем данные описание ошибки
		const int error = SSL_get_error(this->_ssl, status);
		// Определяем тип ошибки
		switch(error){
			// Если был возвращён ноль
			case SSL_ERROR_ZERO_RETURN: {
				// Если удалённая сторона произвела закрытие подключения
				if(SSL_get_shutdown(this->_ssl) & SSL_RECEIVED_SHUTDOWN)
					// Выводим в лог сообщение
					this->_log->print("the remote side closed the connection", log_t::flag_t::INFO);
			} break;
			// Если произошла ошибка вызова
			case SSL_ERROR_SYSCALL: {
				// Получаем данные описание ошибки
				u_long error = ERR_get_error();
				// Если ошибка получена
				if(error != 0){
					// Выводим в лог сообщение
					this->_log->print("%s", log_t::flag_t::CRITICAL, ERR_error_string(error, nullptr));
					/**
					 * Выполняем извлечение остальных ошибок
					 */
					do {
						// Выводим в лог сообщение
						this->_log->print("%s", log_t::flag_t::CRITICAL, ERR_error_string(error, nullptr));
					// Если ещё есть ошибки
					} while((error = ERR_get_error()));
				// Если данные записаны неверно
				} else if(status == -1)
					// Выводим в лог сообщение
					this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
			} break;
			// Для всех остальных ошибок
			default: {
				// Получаем данные описание ошибки
				u_long error = 0;
				// Выполняем чтение ошибок OpenSSL
				while((error = ERR_get_error()))
					// Выводим в лог сообщение
					this->_log->print("%s", log_t::flag_t::CRITICAL, ERR_error_string(error, nullptr));
			}
		}
	// Если произошла ошибка
	} else if(status == -1) {
		// Определяем тип ошибки
		switch(errno){
			// Если произведена неудачная запись в PIPE
			case EPIPE:
				// Выводим в лог сообщение
				this->_log->print("EPIPE", log_t::flag_t::WARNING);
			break;
			// Если произведён сброс подключения
			case ECONNRESET:
				// Выводим в лог сообщение
				this->_log->print("ECONNRESET", log_t::flag_t::WARNING);
			break;
			// Для остальных ошибок
			default:
				// Выводим в лог сообщение
				this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
		}
	}
}
/**
 * clear Метод очистки контекста
 */
void awh::Engine::Context::clear() noexcept {
	// Если сокет активен
	if(this->_addr != nullptr)
		// Выполняем закрытие подключения
		this->_addr->close();
	// Если объект верификации домена создан
	if(this->_verify != nullptr){
		// Удаляем объект верификации
		delete this->_verify;
		// Зануляем объект верификации
		this->_verify = nullptr;
	}
	// Если контекст SSL создан
	if(this->_ssl != nullptr){
		// Выключаем получение данных SSL
		SSL_shutdown(this->_ssl);
		// Очищаем объект SSL
		// SSL_clear(this->_ssl);
		// Освобождаем выделенную память
		SSL_free(this->_ssl);
		// Зануляем контекст сервера
		this->_ssl = nullptr;
	}
	// Если контекст SSL сервер был поднят
	if(this->_ctx != nullptr){
		// Очищаем контекст сервера
		SSL_CTX_free(this->_ctx);
		// Зануляем контекст сервера
		this->_ctx = nullptr;
	}
	/*
	// Если BIO создано
	if(this->_bio != nullptr){
		// Выполняем очистку BIO
		BIO_free(this->_bio);
		// Зануляем контекст BIO
		this->_bio = nullptr;
	}
	*/
	// Сбрасываем флаг инициализации
	this->_tls = false;
	// Сбрасываем флаг вывода подробной информации
	this->_verb = false;
	// Зануляем объект подключения
	this->_addr = nullptr;
}
/**
 * info Метод вывода информации о сертификате
 */
void awh::Engine::Context::info() const noexcept {
	// Если версия OpenSSL не соответствует указанной при сборке
	if(OpenSSL_version_num() != OPENSSL_VERSION_NUMBER){
		// Выводим в лог сообщение
		this->_log->print(
			"OpenSSL version mismatch!\r\n"
			"Compiled against %s\r\n"
			"Linked against   %s",
			log_t::flag_t::WARNING,
			OPENSSL_VERSION_TEXT,
			OpenSSL_version(OPENSSL_VERSION)
		);
		// Если мажорная и минорная версия OpenSSL не совпадают
		if((OpenSSL_version_num() >> 20) != (OPENSSL_VERSION_NUMBER >> 20)){
			// Выводим в лог сообщение
			this->_log->print("major and minor version numbers must match, exiting", log_t::flag_t::CRITICAL);
			// Выходим из приложения
			exit(EXIT_FAILURE);
		}
	// Если всё хорошо, выводим версию OpenSSL
	} else this->_log->print("Using %s", log_t::flag_t::INFO, OpenSSL_version(OPENSSL_VERSION));
	// Если версия OpenSSL ниже установленной минимальной
	if(OPENSSL_VERSION_NUMBER < 0x1010102fL){
		// Выводим в лог сообщение
		this->_log->print("%s is unsupported, use OpenSSL Version 1.1.1a or higher", log_t::flag_t::CRITICAL, OpenSSL_version(OPENSSL_VERSION));
		// Выходим из приложения
		exit(EXIT_FAILURE);
	}
	// Если объект подключения создан и сертификат передан
	if(this->_ssl != nullptr){
		// Выполняем получение сертификата сервера
		X509 * cert = SSL_get_peer_certificate(this->_ssl);
		// Если сертификат сервера получен
		if(cert != nullptr){
			// Буфер данных для получения данных
			char buffer[256];
			// Выводим начальный разделитель
			printf ("------------------------------------------------------------\n\n");
			// Выводим заголовок
			printf("Peer certificates:\n");
			// Получаем название сертификата
			X509_NAME_oneline(X509_get_subject_name(cert), buffer, sizeof(buffer));
			// Выводим название сертификата
			printf("Subject: %s\n", buffer);
			// Получаем эмитента выпустившего сертификат
			X509_NAME_oneline(X509_get_issuer_name(cert), buffer, sizeof(buffer));
			// Выводим эмитента сертификата
			printf("Issuer: %s\n", buffer);
			// Выводим параметры шифрования
			printf("Cipher: %s\n", SSL_CIPHER_get_name(SSL_get_current_cipher(this->_ssl)));
			// Выводим конечный разделитель
			printf ("\n------------------------------------------------------------\n\n");
			// Очищаем объект сертификата
			X509_free(cert);
		}
	}
}
/**
 * read Метод чтения данных из сокета
 * @param buffer буфер данных для чтения
 * @param size   размер буфера данных
 * @return       количество считанных байт
 */
int64_t awh::Engine::Context::read(char * buffer, const size_t size) noexcept {
	// Результат работы функции
	int64_t result = 0;
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0) && (this->_type != type_t::NONE) && (this->_addr->fd > -1)){
		// Выполняем зануление буфера
		memset(buffer, 0, size);
		// Если защищённый режим работы разрешён
		if(this->_tls){
			// Выполняем очистку ошибок OpenSSL
			ERR_clear_error();
			// Если подключение ещё активно
			if(!(SSL_get_shutdown(this->_ssl) & SSL_RECEIVED_SHUTDOWN)){
				// Если подключение выполнено
				if((result = ((this->_type == type_t::SERVER) ? SSL_accept(this->_ssl) : SSL_connect(this->_ssl))) > 0){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Если подробная информация ещё не выведена
						if(!this->_verb){
							// Запоминаем, что подробная информация уже была выведена
							this->_verb = true;
							// Выводим информацию об OpenSSL
							this->info();
						}
					#endif
					// Определяем сокет подключения
					switch(this->_addr->_type){
						// Если сокет установлен как TCP/IP
						case SOCK_STREAM:
							// Выполняем чтение из защищённого сокета
							result = SSL_read(this->_ssl, buffer, size);
						break;
						// Если сокет установлен UDP
						case SOCK_DGRAM:
							// Выполняем чтение из защищённого сокета
							result = BIO_read(this->_bio, buffer, size);
						break;
					}
				}
			}
		// Выполняем чтение из буфера данных стандартным образом
		} else {
			// Если сокет установлен как TCP/IP
			if(this->_addr->_type == SOCK_STREAM)
				// Выполняем чтение данных из TCP/IP сокета
				result = ::recv(this->_addr->fd, buffer, size, 0);
			// Если сокет установлен UDP
			else if(this->_addr->_type == SOCK_DGRAM) {
				// Создаём объект подключения
				struct sockaddr * addr = nullptr;
				// Определяем тип подключения
				switch((uint8_t) this->_addr->status){
					// Если статус установлен как подключение клиентом
					case (uint8_t) addr_t::status_t::CONNECTED:
						// Запоминаем полученную структуру
						addr = (struct sockaddr *) (&this->_addr->_peer.server);
					break;
					// Если статус установлен как разрешение подключения к серверу
					case (uint8_t) addr_t::status_t::ACCEPTED:
						// Запоминаем полученную структуру
						addr = (struct sockaddr *) (&this->_addr->_peer.client);
					break;
				}
				// Выполняем чтение данных из сокета
				result = ::recvfrom(this->_addr->fd, buffer, size, 0, addr, &this->_addr->_peer.size);
			}
		}
		// Если данные прочитать не удалось
		if(result <= 0){
			// Получаем статус сокета
			const int status = this->_addr->_socket.isBlocking(this->_addr->fd);			
			// Если сокет находится в блокирующем режиме
			if((result < 0) && (status != 0))
				// Выполняем обработку ошибок
				this->error(result);
			// Если произошла ошибка
			else if((result < 0) && (status == 0)) {
				// Если произошёл системный сигнал попробовать ещё раз
				if(errno == EINTR) return -2;
				// Если защищённый режим работы разрешён
				if(this->_tls){
					// Получаем данные описание ошибки
					if(SSL_get_error(this->_ssl, result) == SSL_ERROR_WANT_READ)
						// Выполняем пропуск попытки
						return -1;
					// Иначе выводим сообщение об ошибке
					else this->error(result);
				// Если защищённый режим работы запрещён
				} else if(errno == EAGAIN) return -1;
				// Иначе просто закрываем подключение
				result = 0;
			}
			// Если подключение разорвано или сокет находится в блокирующем режиме
			if((result == 0) || (status != 0))
				// Выполняем отключение от сервера
				result = 0;
			// Если произошло отключение
			if(result == 0) this->_addr->status = addr_t::status_t::DISCONNECTED;
		}
	}
	// Выводим результат
	return result;
}
/**
 * write Метод записи данных в сокет
 * @param buffer буфер данных для записи
 * @param size   размер буфера данных
 * @return       количество записанных байт
 */
int64_t awh::Engine::Context::write(const char * buffer, const size_t size) noexcept {
	// Результат работы функции
	int64_t result = 0;
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0) && (this->_type != type_t::NONE) && (this->_addr->fd > -1)){
		// Если защищённый режим работы разрешён
		if(this->_tls){
			// Выполняем очистку ошибок OpenSSL
			ERR_clear_error();
			// Если подключение ещё активно
			if(!(SSL_get_shutdown(this->_ssl) & SSL_RECEIVED_SHUTDOWN)){
				// Если подключение выполнено
				if((result = ((this->_type == type_t::SERVER) ? SSL_accept(this->_ssl) : SSL_connect(this->_ssl))) > 0){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Если подробная информация ещё не выведена
						if(!this->_verb){
							// Запоминаем, что подробная информация уже была выведена
							this->_verb = true;
							// Выводим информацию об OpenSSL
							this->info();
						}
					#endif
					// Определяем сокет подключения
					switch(this->_addr->_type){
						// Если сокет установлен как TCP/IP
						case SOCK_STREAM:
							// Выполняем отправку сообщения через защищённый канал
							result = SSL_write(this->_ssl, buffer, size);
						break;
						// Если сокет установлен UDP
						case SOCK_DGRAM:
							// Выполняем отправку сообщения через защищённый канал
							result = BIO_write(this->_bio, buffer, size);
						break;
					}
				}
			}
		// Выполняем отправку сообщения в сокет
		} else {
			// Если сокет установлен как TCP/IP
			if(this->_addr->_type == SOCK_STREAM)
				// Выполняем отправку данных в TCP/IP сокет
				result = ::send(this->_addr->fd, buffer, size, 0);
			// Если сокет установлен UDP
			else if(this->_addr->_type == SOCK_DGRAM) {
				// Создаём объект подключения
				struct sockaddr * addr = nullptr;
				// Определяем тип подключения
				switch((uint8_t) this->_addr->status){
					// Если статус установлен как подключение клиентом
					case (uint8_t) addr_t::status_t::CONNECTED: {
						// Запоминаем полученную структуру
						addr = (struct sockaddr *) (&this->_addr->_peer.server);
						/**
						 * Если операционной системой не является Windows
						 */
						#if !defined(_WIN32) && !defined(_WIN64)
							// Для протокола unix-сокета
							if(this->_addr->_peer.server.ss_family == AF_UNIX)
								// Получаем размер объекта сокета
								this->_addr->_peer.size = (
									offsetof(struct sockaddr_un, sun_path) +
									strlen(((struct sockaddr_un *) (&this->_addr->_peer.server))->sun_path)
								);
						#endif
					} break;
					// Если статус установлен как разрешение подключения к серверу
					case (uint8_t) addr_t::status_t::ACCEPTED: {
						// Запоминаем полученную структуру
						addr = (struct sockaddr *) (&this->_addr->_peer.client);
						/**
						 * Если операционной системой не является Windows
						 */
						#if !defined(_WIN32) && !defined(_WIN64)
							// Для протокола unix-сокета
							if(this->_addr->_peer.client.ss_family == AF_UNIX)
								// Получаем размер объекта сокета
								this->_addr->_peer.size = (
									offsetof(struct sockaddr_un, sun_path) +
									strlen(((struct sockaddr_un *) (&this->_addr->_peer.client))->sun_path)
								);
						#endif
					} break;
				}
				// Выполняем запись данных в сокет
				result = ::sendto(this->_addr->fd, buffer, size, 0, addr, this->_addr->_peer.size);
			}
		}
		// Если данные записать не удалось
		if(result <= 0){
			// Получаем статус сокета
			const int status = this->_addr->_socket.isBlocking(this->_addr->fd);
			// Если сокет находится в блокирующем режиме
			if((result < 0) && (status != 0))
				// Выполняем обработку ошибок
				this->error(result);
			// Если произошла ошибка
			else if((result < 0) && (status == 0)) {
				// Если произошёл системный сигнал попробовать ещё раз
				if(errno == EINTR) return -2;
				// Если защищённый режим работы разрешён
				if(this->_tls){
					// Получаем данные описание ошибки
					if(SSL_get_error(this->_ssl, result) == SSL_ERROR_WANT_WRITE)
						// Выполняем пропуск попытки
						return -1;
					// Иначе выводим сообщение об ошибке
					else this->error(result);
				// Если защищённый режим работы запрещён
				} else if(errno == EAGAIN) return -1;
				// Иначе просто закрываем подключение
				result = 0;
			}
			// Если подключение разорвано или сокет находится в блокирующем режиме
			if((result == 0) || (status != 0))
				// Выполняем отключение от сервера
				result = 0;
			// Если произошло отключение
			if(result == 0) this->_addr->status = addr_t::status_t::DISCONNECTED;
		}
	}
	// Выводим результат
	return result;
}
/**
 * block Метод установки блокирующего сокета
 * @return результат работы функции
 */
int awh::Engine::Context::block() noexcept {
	// Результат работы функции
	int result = 0;
	// Если защищённый режим работы разрешён
	if(this->_tls && (this->_addr->fd > -1)){
		// Переводим сокет в блокирующий режим
		this->_addr->_socket.blocking(this->_addr->fd);
		// Устанавливаем блокирующий режим ввода/вывода для сокета
		BIO_set_nbio(this->_bio, 0);
		// Флаг необходимо установить только для неблокирующего сокета
		SSL_clear_mode(this->_ssl, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
	}
	// Выводим результат
	return result;
}
/**
 * noblock Метод установки неблокирующего сокета
 * @return результат работы функции
 */
int awh::Engine::Context::noblock() noexcept {
	// Результат работы функции
	int result = 0;
	// Если файловый дескриптор активен
	if(this->_tls && (this->_addr->fd > -1)){
		// Переводим сокет в не блокирующий режим
		this->_addr->_socket.nonBlocking(this->_addr->fd);
		// Устанавливаем неблокирующий режим ввода/вывода для сокета
		BIO_set_nbio(this->_bio, 1);
		// Флаг необходимо установить только для неблокирующего сокета
		SSL_set_mode(this->_ssl, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
	}
	// Выводим результат
	return result;
}
/**
 * isblock Метод проверки на то, является ли сокет заблокированным
 * @return результат работы функции
 */
int awh::Engine::Context::isblock() noexcept {
	// Выводим результат проверки
	return (this->_addr->fd > -1 ? this->_addr->_socket.isBlocking(this->_addr->fd) : -1);
}
/**
 * timeout Метод установки таймаута
 * @param msec   количество миллисекунд
 * @param method метод для установки таймаута
 * @return       результат установки таймаута
 */
int awh::Engine::Context::timeout(const time_t msec, const method_t method) noexcept {
	// Определяем тип метода
	switch((uint8_t) method){
		// Если установлен метод чтения
		case (uint8_t) method_t::READ:
			// Выполняем установку таймера на чтение данных из сокета
			return (this->_addr->fd > -1 ? this->_addr->_socket.readTimeout(this->_addr->fd, msec) : -1);
		// Если установлен метод записи
		case (uint8_t) method_t::WRITE:
			// Выполняем установку таймера на запись данных в сокет
			return (this->_addr->fd > -1 ? this->_addr->_socket.writeTimeout(this->_addr->fd, msec) : -1);
	}
	// Сообщаем, что операция не выполнена
	return -1;
}
/**
 * buffer Метод установки размеров буфера
 * @param read  размер буфера на чтение
 * @param write размер буфера на запись
 * @param total максимальное количество подключений
 * @return      результат работы функции
 */
int awh::Engine::Context::buffer(const int read, const int write, const u_int total) noexcept {
	// Если подключение выполнено
	return (this->_addr->fd > -1 ? this->_addr->_socket.bufferSize(this->_addr->fd, read, write, total) : -1);
}
 /**
 * ~Context Деструктор
 */
awh::Engine::Context::~Context() noexcept {
	// Выполняем очистку выделенных ранее данных
	this->clear();
}
/**
 * rawEqual Метод проверки на эквивалентность доменных имен
 * @param first  первое доменное имя
 * @param second второе доменное имя
 * @return       результат проверки
 */
const bool awh::Engine::rawEqual(const string & first, const string & second) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если данные переданы
	if(!first.empty() && !second.empty())
		// Проверяем совпадение строки
		result = (first.compare(second) == 0);
	// Выводим результат
	return result;
}
/**
 * rawNequal Метод проверки на не эквивалентность доменных имен
 * @param first  первое доменное имя
 * @param second второе доменное имя
 * @param max    количество начальных символов для проверки
 * @return       результат проверки
 */
const bool awh::Engine::rawNequal(const string & first, const string & second, const size_t max) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если данные переданы
	if(!first.empty() && !second.empty()){
		// Получаем новые значения
		string first1 = first;
		string second1 = second;
		// Получаем новые значения переменных
		const string & first2 = first1.replace(max, first1.length() - max, "");
		const string & second2 = second1.replace(max, second1.length() - max, "");
		// Проверяем совпадение строки
		result = (first2.compare(second2) == 0);
	}
	// Выводим результат
	return result;
}
/**
 * hostmatch Метод проверки эквивалентности доменного имени с учетом шаблона
 * @param host доменное имя
 * @param patt шаблон домена
 * @return     результат проверки
 */
const bool awh::Engine::hostmatch(const string & host, const string & patt) const noexcept {
	// Результат работы функции
	bool result = true;
	// Если данные переданы
	if(!host.empty() && !patt.empty()){
		// Запоминаем шаблон и хоста
		string hostLabel = host;
		string pattLabel = patt;
		// Позиция звездочки в шаблоне
		const size_t pattWildcard = patt.find('*');
		// Ищем звездочку в шаблоне не найдена
		if(pattWildcard == string::npos)
			// Выполняем проверку эквивалентности доменных имён
			return this->rawEqual(patt, host);
		// Определяем конец шаблона
		const size_t pattLabelEnd = patt.find('.');
		// Если это конец тогда запрещаем активацию шаблона
		if((pattLabelEnd == string::npos) || (pattWildcard > pattLabelEnd) || this->rawNequal(patt, "xn--", 4))
			// Выполняем проверку эквивалентности доменных имён
			return this->rawEqual(patt, host);
		// Выполняем поиск точки в название хоста
		const size_t hostLabelEnd = host.find('.');
		// Если хост не найден
		if((pattLabelEnd != string::npos) && (hostLabelEnd != string::npos)){
			// Обрезаем строку шаблона
			const string & p = pattLabel.replace(0, pattLabelEnd, "");
			// Обрезаем строку хоста
			const string & h = hostLabel.replace(0, hostLabelEnd, "");
			// Выполняем сравнение
			if(!this->rawEqual(p, h)) return false;
		// Выходим
		} else return false;
		// Если диапазоны точки в шаблоне и хосте отличаются тогда выходим
		if(hostLabelEnd < pattLabelEnd) return false;
		// Получаем размер префикса и суфикса
		const size_t prefixlen = pattWildcard;
		const size_t suffixlen = (pattLabelEnd - (pattWildcard + 1));
		// Обрезаем строку шаблона
		const string & p = pattLabel.replace(0, pattWildcard + 1, "");
		// Обрезаем строку хоста
		const string & h = hostLabel.replace(0, hostLabelEnd - suffixlen, "");
		// Проверяем эквивалент результата
		return (this->rawNequal(patt, host, prefixlen) && this->rawNequal(p, h, suffixlen));
	}
	// Выводим результат
	return result;
}
/**
 * certHostcheck Метод проверки доменного имени по шаблону
 * @param host доменное имя
 * @param patt шаблон домена
 * @return     результат проверки
 */
const bool awh::Engine::certHostcheck(const string & host, const string & patt) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если данные переданы
	if(!host.empty() && !patt.empty())
		// Проверяем эквивалентны ли домен и шаблон
		result = (this->rawEqual(host, patt) || this->hostmatch(host, patt));
	// Выводим результат
	return result;
}
/**
 * verifyCert Функция обратного вызова для проверки валидности сертификата
 * @param ok   результат получения сертификата
 * @param x509 данные сертификата
 * @return     результат проверки
 */
int awh::Engine::verifyCert(const int ok, X509_STORE_CTX * x509) noexcept {
	// Буфер данных для получения данных
	char buffer[256];
	// Если проверка не выполнена
	if(!ok){
		// Получаем данные ошибки
		int error = X509_STORE_CTX_get_error(x509);
		// Получаем глубину ошибки
		int depth = X509_STORE_CTX_get_error_depth(x509);
		// Выполняем извлечение сертификата
		X509 * cert = X509_STORE_CTX_get_current_cert(x509);
		// Выводим начальный разделитель
		printf ("------------------------------------------------------------\n\n");
		// Выводим заголовок
		printf("Current certificate verification:\n");
		// Получаем название сертификата
		X509_NAME_oneline(X509_get_subject_name(cert), buffer, sizeof(buffer));
		// Выводим название сертификата
		printf("Subject: %s\n", buffer);
		// Получаем эмитента выпустившего сертификат
		X509_NAME_oneline(X509_get_issuer_name(cert), buffer, sizeof(buffer));
		// Выводим эмитента сертификата
		printf("Issuer: %s\n", buffer);
		// Выводим информацию о ошибке
		printf("Error: %s\n", X509_verify_cert_error_string(error));
		// Выводим конечный разделитель
		printf ("\n------------------------------------------------------------\n\n");
		// Очищаем объект сертификата
		X509_free(cert);
	}
	// Выводим результат
	return ok;
}
/**
 * verifyHost Функция обратного вызова для проверки валидности хоста
 * @param x509 данные сертификата
 * @param ctx  передаваемый контекст
 * @return     результат проверки
 */
int awh::Engine::verifyHost(X509_STORE_CTX * x509, void * ctx) noexcept {
	// Если объекты переданы верно
	if((x509 != nullptr) && (ctx != nullptr)){
		// Буфер данных сертификатов из хранилища
		char buffer[256];
		// Заполняем структуру нулями
		memset(buffer, 0, sizeof(buffer));
		// Ошибка проверки сертификата
		string status = "X509VerifyCertFailed";
		// Выполняем проверку сертификата
		const int ok = X509_verify_cert(x509);
		// Запрашиваем данные сертификата
		X509 * cert = X509_STORE_CTX_get_current_cert(x509);
		// Результат проверки домена
		engine_t::validate_t validate = engine_t::validate_t::Error;
		// Получаем объект подключения
		const verify_t * verify = reinterpret_cast <const verify_t *> (ctx);
		// Если проверка сертификата прошла удачно
		if(ok){
			// Выполняем проверку на соответствие хоста с данными хостов у сертификата
			validate = verify->engine->validateHostname(verify->host.c_str(), cert);
			// Определяем полученную ошибку
			switch((uint8_t) validate){
				case (uint8_t) engine_t::validate_t::MatchFound:           status = "MatchFound";           break;
				case (uint8_t) engine_t::validate_t::MatchNotFound:        status = "MatchNotFound";        break;
				case (uint8_t) engine_t::validate_t::NoSANPresent:         status = "NoSANPresent";         break;
				case (uint8_t) engine_t::validate_t::MalformedCertificate: status = "MalformedCertificate"; break;
				case (uint8_t) engine_t::validate_t::Error:                status = "Error";                break;
				default:                                                   status = "WTF!";
			}
		}
		// Запрашиваем имя домена
		X509_NAME_oneline(X509_get_subject_name(cert), buffer, sizeof(buffer));
		// Очищаем выделенную память
		X509_free(cert);
		// Если домен найден в записях сертификата (т.е. сертификат соответствует данному домену)
		if(validate == engine_t::validate_t::MatchFound){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим в лог сообщение
				verify->engine->_log->print("https server [%s] has this certificate, which looks good to me: %s", log_t::flag_t::INFO, verify->host.c_str(), buffer);
			#endif
			// Выводим сообщение, что проверка пройдена
			return 1;
		// Если ресурс не найден тогда выводим сообщение об ошибке
		} else verify->engine->_log->print("%s for hostname '%s' [%s]", log_t::flag_t::CRITICAL, status.c_str(), verify->host.c_str(), buffer);
	}
	// Выводим сообщение, что проверка не пройдена
	return 0;
}
/**
 * generateCookie Функция обратного вызова для генерации куков
 * @param ssl    объект SSL
 * @param cookie данные куков
 * @param size   количество символов
 * @return       результат проверки
 */
int awh::Engine::generateCookie(SSL * ssl, u_char * cookie, u_int * size) noexcept {
	// Смещение в буфере и размер сгенерированных печенок
	u_int offset = 0, length = 0;
	// Буфер под генерацию печенок
	u_char result[EVP_MAX_MD_SIZE];
	// Создаём объединение адресов
	union {
		struct sockaddr_in s4;      // Объект IPv4
		struct sockaddr_in6 s6;     // Объект IPv6
		struct sockaddr_storage ss; // Объект хранилища
	} peer;
	// Если печенки еще не проинициализированны
	if(!_cookieInit){
		// Выполняем произвольно генерацию байт в буфере печенок
		if(!(_cookieInit = RAND_bytes(_cookies, sizeof(_cookies)))){
			// Создаём объект фреймворка
			fmk_t fmk;
			// Выводим в лог сообщение
			log_t(&fmk).print("setting random cookie secret", log_t::flag_t::CRITICAL);
			// Выходим и сообщаем, что генерация куков не удалась
			return 0;
		}
	}
	// Выполняем чтение из подключения информации
	(void) BIO_dgram_get_peer(SSL_get_rbio(ssl), &peer);
	// Выполняем определение протокола интернета
	switch(peer.ss.ss_family){
		// Для протокола IPv4
		case AF_INET:
			// Увеличиваем смещение на размер структуры данных протокола IPv4
			offset += sizeof(struct in_addr);
		break;
		// Для протокола IPv6
		case AF_INET6:
			// Увеличиваем смещение на размер структуры данных протокола IPv6
			offset += sizeof(struct in6_addr);
		break;
		// Если производится работа с другими протоколами, выходим
		default: OPENSSL_assert(0);
	}
	// Увеличиваем смещение на размер буфера входящих данных
	offset += sizeof(u_short);
	// Выполняем выделение память для буфера данных
	u_char * buffer = (u_char *) OPENSSL_malloc(offset);
	// Если память для буфера данных не выделена
	if(buffer == nullptr){
		// Создаём объект фреймворка
		fmk_t fmk;
		// Выводим в лог сообщение
		log_t(&fmk).print("out of memory cookie", log_t::flag_t::CRITICAL);
		// Выходим и сообщаем, что генерация куков не удалась
		return 0;
	}
	// Выполняем определение протокола интернета
	switch(peer.ss.ss_family){
		// Для протокола IPv4
		case AF_INET: {
			// Выполняем чтение в буфер данных данные порта
			memcpy(buffer, &peer.s4.sin_port, sizeof(u_short));
			// Выполняем чтение в буфер данных данные структуры подключения
			memcpy(buffer + sizeof(peer.s4.sin_port), &peer.s4.sin_addr, sizeof(struct in_addr));
		} break;
		// Для протокола IPv6
		case AF_INET6: {
			// Выполняем чтение в буфер данных данные порта
			memcpy(buffer, &peer.s6.sin6_port, sizeof(u_short));
			// Выполняем чтение в буфер данных данные структуры подключения
			memcpy(buffer + sizeof(u_short), &peer.s6.sin6_addr, sizeof(struct in6_addr));
		} break;
		// Если производится работа с другими протоколами, выходим
		default: OPENSSL_assert(0);
	}
	// Выполняем расчёт HMAC в буфере, с использованием секретного ключа
	HMAC(EVP_sha1(), (const void *) _cookies, sizeof(_cookies), (const u_char *) buffer, offset, result, &length);
	// Очищаем ранее выделенную память
	OPENSSL_free(buffer);
	// Выполняем копирование полученного результата в буфер печенок
	memcpy(cookie, result, length);
	// Устанавливаем размер буфера печенок
	(* size) = length;
	// Выводим положительный ответ
	return 1;
}
/**
 * verifyCookie Функция обратного вызова для проверки куков
 * @param ssl    объект SSL
 * @param cookie данные куков
 * @param size   количество символов
 * @return       результат проверки
 */
int awh::Engine::verifyCookie(SSL * ssl, const u_char * cookie, u_int size) noexcept {
	// Смещение в буфере и размер сгенерированных печенок
	u_int offset = 0, length = 0;
	// Буфер под генерацию печенок
	u_char result[EVP_MAX_MD_SIZE];
	// Создаём объединение адресов
	union {
		struct sockaddr_in s4;      // Объект IPv4
		struct sockaddr_in6 s6;     // Объект IPv6
		struct sockaddr_storage ss; // Объект хранилища
	} peer;
	// Если печенки не проинициализированы, значит куки не валидные
	if(!_cookieInit) return 0;
	// Выполняем чтение из подключения информации
	(void) BIO_dgram_get_peer(SSL_get_rbio(ssl), &peer);
	// Выполняем определение протокола интернета
	switch(peer.ss.ss_family){
		// Для протокола IPv4
		case AF_INET:
			// Увеличиваем смещение на размер структуры данных протокола IPv4
			offset += sizeof(struct in_addr);
		break;
		// Для протокола IPv6
		case AF_INET6:
			// Увеличиваем смещение на размер структуры данных протокола IPv6
			offset += sizeof(struct in6_addr);
		break;
		// Если производится работа с другими протоколами, выходим
		default: OPENSSL_assert(0);
	}
	// Увеличиваем смещение на размер буфера входящих данных
	offset += sizeof(u_short);
	// Выполняем выделение память для буфера данных
	u_char * buffer = (u_char *) OPENSSL_malloc(offset);
	// Если память для буфера данных не выделена
	if(buffer == nullptr){
		// Создаём объект фреймворка
		fmk_t fmk;
		// Выводим в лог сообщение
		log_t(&fmk).print("out of memory cookie", log_t::flag_t::CRITICAL);
		// Выходим и сообщаем, что генерация куков не удалась
		return 0;
	}
	// Выполняем определение протокола интернета
	switch(peer.ss.ss_family){
		// Для протокола IPv4
		case AF_INET: {
			// Выполняем чтение в буфер данных данные порта
			memcpy(buffer, &peer.s4.sin_port, sizeof(u_short));
			// Выполняем чтение в буфер данных данные структуры подключения
			memcpy(buffer + sizeof(u_short), &peer.s4.sin_addr, sizeof(struct in_addr));
		} break;
		// Для протокола IPv6
		case AF_INET6: {
			// Выполняем чтение в буфер данных данные порта
			memcpy(buffer, &peer.s6.sin6_port, sizeof(u_short));
			// Выполняем чтение в буфер данных данные структуры подключения
			memcpy(buffer + sizeof(u_short), &peer.s6.sin6_addr, sizeof(struct in6_addr));
		} break;
		// Если производится работа с другими протоколами, выходим
		default: OPENSSL_assert(0);
	}
	// Выполняем расчёт HMAC в буфере, с использованием секретного ключа
	HMAC(EVP_sha1(), (const void *) _cookies, sizeof(_cookies), (const u_char *) buffer, offset, result, &length);
	// Очищаем ранее выделенную память
	OPENSSL_free(buffer);
	// Выполняем проверку печенок, если печенки совпадают, значит всё хорошо
	if((size == length) && (memcmp(result, cookie, length) == 0))
		// Выходим из функции с удачей
		return 1;
	// Выходим из функции с неудачей
	return 0;
}
/**
 * generateCookie Функция обратного вызова для генерации куков
 * @param ssl    объект SSL
 * @param cookie данные куков
 * @param size   количество символов
 * @return       результат проверки
 */
int awh::Engine::generateStatelessCookie(SSL * ssl, u_char * cookie, size_t * size) noexcept {
	// Размер буфера с печенками
	u_int length = 0;
	// Выполняем генерацию печенок
	const int result = generateCookie(ssl, cookie, &length);
	// Получаем размер буфера с печенками
	(* size) = length;
	// Выводим результат работы функции
	return result;
}
/**
 * verifyCookie Функция обратного вызова для проверки куков
 * @param ssl    объект SSL
 * @param cookie данные куков
 * @param size   количество символов
 * @return       результат проверки
 */
int awh::Engine::verifyStatelessCookie(SSL * ssl, const u_char * cookie, size_t size) noexcept {
	// Выполняем проверку печенок
	return verifyCookie(ssl, cookie, (u_int) size);
}
/**
 * matchesCommonName Метод проверки доменного имени по данным из сертификата
 * @param host доменное имя
 * @param cert сертификат
 * @return     результат проверки
 */
const awh::Engine::validate_t awh::Engine::matchesCommonName(const string & host, const X509 * cert) const noexcept {
	// Результат работы функции
	validate_t result = validate_t::MatchNotFound;
	// Если данные переданы
	if(!host.empty() && (cert != nullptr)){
		// Получаем индекс имени по NID
		const int cnl = X509_NAME_get_index_by_NID(X509_get_subject_name((X509 *) cert), NID_commonName, -1);
		// Если индекс не получен тогда выходим
		if(cnl < 0) return validate_t::Error;
		// Извлекаем поле CN
		X509_NAME_ENTRY * cne = X509_NAME_get_entry(X509_get_subject_name((X509 *) cert), cnl);
		// Если поле не получено тогда выходим
		if(cne == nullptr) return validate_t::Error;
		// Конвертируем CN поле в C строку
		ASN1_STRING * cna = X509_NAME_ENTRY_get_data(cne);
		// Если строка не сконвертирована тогда выходим
		if(cna == nullptr) return validate_t::Error;
		// Извлекаем название в виде строки
		const string cn((char *) ASN1_STRING_get0_data(cna), ASN1_STRING_length(cna));
		// Сравниваем размеры полученных строк
		if(size_t(ASN1_STRING_length(cna)) != cn.length()) return validate_t::MalformedCertificate;
		// Выполняем рукопожатие
		if(this->certHostcheck(host, cn)) return validate_t::MatchFound;
	}
	// Выводим результат
	return result;
}
/**
 * matchSubjectName Метод проверки доменного имени по списку доменных имен из сертификата
 * @param host доменное имя
 * @param cert сертификат
 * @return     результат проверки
 */
const awh::Engine::validate_t awh::Engine::matchSubjectName(const string & host, const X509 * cert) const noexcept {
	// Результат работы функции
	validate_t result = validate_t::MatchNotFound;
	// Если данные переданы
	if(!host.empty() && (cert != nullptr)){
		// Получаем имена
		STACK_OF(GENERAL_NAME) * sn = reinterpret_cast <STACK_OF(GENERAL_NAME) *> (X509_get_ext_d2i((X509 *) cert, NID_subject_alt_name, nullptr, nullptr));
		// Если имена не получены тогда выходим
		if(sn == nullptr) return validate_t::NoSANPresent;
		// Получаем количество имен
		const int sanNamesNb = sk_GENERAL_NAME_num(sn);
		// Переходим по всему списку
		for(int i = 0; i < sanNamesNb; i++){
			// Получаем имя из списка
			const GENERAL_NAME * cn = sk_GENERAL_NAME_value(sn, i);
			// Проверяем тип имени
			if(cn->type == GEN_DNS){
				// Получаем dns имя
				const string dns((char *) ASN1_STRING_get0_data(cn->d.dNSName), ASN1_STRING_length(cn->d.dNSName));
				// Если размер имени не совпадает
				if(size_t(ASN1_STRING_length(cn->d.dNSName)) != dns.length()){
					// Запоминаем результат
					result = validate_t::MalformedCertificate;
					// Выходим из цикла
					break;
				// Если размер имени и dns имя совпадает
				} else if(this->certHostcheck(host, dns)){
					// Запоминаем результат что домен найден
					result = validate_t::MatchFound;
					// Выходим из цикла
					break;
				}
			}
		}
		// Очищаем список имен
		sk_GENERAL_NAME_pop_free(sn, GENERAL_NAME_free);
	}
	// Выводим результат
	return result;
}
/**
 * validateHostname Метод проверки доменного имени
 * @param host доменное имя
 * @param cert сертификат
 * @return     результат проверки
 */
const awh::Engine::validate_t awh::Engine::validateHostname(const string & host, const X509 * cert) const noexcept {
	// Результат работы функции
	validate_t result = validate_t::Error;
	// Если данные переданы
	if(!host.empty() && (cert != nullptr)){
		// Выполняем проверку имени хоста по списку доменов у сертификата
		result = this->matchSubjectName(host, cert);
		// Если у сертификата только один домен
		if(result == validate_t::NoSANPresent) result = this->matchesCommonName(host, cert);
	}
	// Выводим результат
	return result;
}
/**
 * storeCA Метод инициализации магазина доверенных сертификатов
 * @param ctx объект контекста SSL
 * @return    результат инициализации
 */
bool awh::Engine::storeCA(SSL_CTX * ctx) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если контекст SSL передан
	if(ctx != nullptr){
		// Если доверенный сертификат (CA-файл) найден и адрес файла указан
		if(!this->_ca.empty() && (fs_t::isfile(this->_ca) || fs_t::isdir(this->_path))){
			// Определяем путь где хранятся сертификаты
			const char * path = (!this->_path.empty() ? this->_path.c_str() : nullptr);
			// Выполняем проверку
			if(SSL_CTX_load_verify_locations(ctx, this->_ca.c_str(), path) != 1){
				// Выводим в лог сообщение
				this->_log->print("%s", log_t::flag_t::CRITICAL, "ssl verify locations is not allow");
				// Выходим
				return result;
			}
			// Если каталог получен
			if(path != nullptr){
				// Получаем полный адрес
				const string & trustdir = fs_t::realPath(this->_path);
				// Если адрес существует
				if(fs_t::isdir(trustdir) && !fs_t::isfile(this->_ca)){
					/**
					 * Если операционной системой является MS Windows
					 */
					#if defined(_WIN32) || defined(_WIN64)
						// Выполняем сплит адреса
						const auto & params = this->_uri->split(trustdir);
						// Если диск получен
						if(!params.front().empty()){
							// Выполняем сплит адреса
							auto path = this->_uri->splitPath(params.back(), FS_SEPARATOR);
							// Добавляем адрес файла в список
							path.push_back(this->_ca);
							// Формируем полный адарес файла
							string filename = this->_fmk->format("%s:%s", params.front().c_str(), this->_uri->joinPath(path, FS_SEPARATOR).c_str());
							// Выполняем проверку доверенного сертификата
							if(!filename.empty()){
								// Выполняем декодирование адреса файла
								filename = this->_uri->decode(filename);
								// Если адрес файла существует
								if((result = fs_t::isfile(filename))){
									// Выполняем проверку доверенного сертификата
									SSL_CTX_set_client_CA_list(ctx, SSL_load_client_CA_file(filename.c_str()));
									// Переходим к следующей итерации
									goto Next;
								}
							}
						}
						// Выполняем очистку адреса доверенного сертификата
						this->_ca.clear();
					/**
					 * Если операционной системой является Nix-подобная
					 */
					#else
						// Выполняем сплит адреса
						auto path = this->_uri->splitPath(trustdir, FS_SEPARATOR);
						// Добавляем адрес файла в список
						path.push_back(this->_ca);
						// Формируем полный адарес файла
						string filename = this->_uri->joinPath(path, FS_SEPARATOR);
						// Выполняем проверку доверенного сертификата
						if(!filename.empty()){
							// Выполняем декодирование адреса файла
							filename = this->_uri->decode(filename);
							// Если адрес файла существует
							if((result = fs_t::isfile(filename))){
								// Выполняем проверку CA файла
								SSL_CTX_set_client_CA_list(ctx, SSL_load_client_CA_file(filename.c_str()));
								// Переходим к следующей итерации
								goto Next;
							}
						}
						// Выполняем очистку адреса доверенного сертификата
						this->_ca.clear();
					#endif
				// Если адрес файла существует
				} else if((result = fs_t::isfile(this->_ca)))
					// Выполняем проверку доверенного сертификата
					SSL_CTX_set_client_CA_list(ctx, SSL_load_client_CA_file(this->_ca.c_str()));
				// Выполняем очистку адреса доверенного сертификата
				else this->_ca.clear();
			// Если адрес файла существует
			} else if((result = fs_t::isfile(this->_ca)))
				// Выполняем проверку доверенного сертификата
				SSL_CTX_set_client_CA_list(ctx, SSL_load_client_CA_file(this->_ca.c_str()));
		// Выполняем очистку адреса доверенного сертификата
		} else this->_ca.clear();
		// Метка следующей итерации
		Next:
		// Если доверенный сертификат не указан
		if(this->_ca.empty()){
			// Получаем данные стора
			X509_STORE * store = SSL_CTX_get_cert_store(ctx);
			/**
			 * Если операционной системой является MS Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				/**
				 * addCertToStoreFn Функция проверки параметров сертификата
				 * @param store стор с сертификатами для работы
				 * @param name  название параметра сертификата
				 * @return      результат проверки
				 */
				auto addCertToStoreFn = [this](X509_STORE * store = nullptr, const char * name = nullptr) noexcept -> int {
					// Результат работы функции
					int result = 0;
					// Если объекты переданы верно
					if((store != nullptr) && (name != nullptr)){
						// Контекст сертификата
						PCCERT_CONTEXT ctx = nullptr;
						// Получаем данные системного стора
						HCERTSTORE sys = CertOpenSystemStore(0, name);
						// Если системный стор не получен
						if(!sys){
							// Выводим в лог сообщение
							this->_log->print("%s", log_t::flag_t::CRITICAL, "failed to open system certificate store");
							// Выходим
							return -1;
						}
						// Перебираем все сертификаты в системном сторе
						while((ctx = CertEnumCertificatesInStore(sys, ctx))){
							// Получаем сертификат
							X509 * cert = d2i_X509(nullptr, (const u_char **) &ctx->pbCertEncoded, ctx->cbCertEncoded);
							// Если сертификат получен
							if(cert != nullptr){
								// Добавляем сертификат в стор
								X509_STORE_add_cert(store, cert);
								// Очищаем выделенную память
								X509_free(cert);
							// Если сертификат не получен
							} else {
								// Формируем результат ответа
								result = -1;
								// Выводим в лог сообщение
								this->_log->print("%s failed", log_t::flag_t::CRITICAL, "d2i_X509");
								// Выходим из цикла
								break;
							}
						}
						// Закрываем системный стор
						CertCloseStore(sys, 0);
					}
					// Выходим
					return result;
				};
				// Проверяем существует ли путь
				if((addCertToStoreFn(store, "CA") < 0) || (addCertToStoreFn(store, "AuthRoot") < 0) || (addCertToStoreFn(store, "ROOT") < 0)) return result;
			#endif
			// Если стор не устанавливается, тогда выводим ошибку
			if(!(result = (X509_STORE_set_default_paths(store) == 1)))
				// Выводим в лог сообщение
				this->_log->print("%s", log_t::flag_t::CRITICAL, "set default paths for x509 store is not allow");
		}
	}
	// Выводим результат
	return result;
}
/**
 * isTLS Метод проверки на активацию режима шифрования
 * @param ctx  контекст подключения
 * @return     результат проверки
 */
bool awh::Engine::isTLS(ctx_t & ctx) const noexcept {
	// Выводим результат проверки
	return ctx._tls;
}
/**
 * wait Метод ожидания рукопожатия
 * @param target контекст назначения
 */
void awh::Engine::wait(ctx_t & target) noexcept {
	// Определяем тип входящего сокета
	switch(target._addr->_type){
		// Если сокет установлен TCP/IP
		case SOCK_STREAM:
			// Выполняем ожидание подключения
			while(SSL_stateless(target._ssl) < 1);
		break;
		// Если сокет установлен UDP
		case SOCK_DGRAM: {
			// Выполняем зануление структуры подключения клиента
			memset(&target._addr->_peer.client, 0, sizeof(struct sockaddr_storage));
			// Выполняем ожидание подключения
			while(DTLSv1_listen(target._ssl, (BIO_ADDR *) &target._addr->_peer.client) < 1);
		} break;
	}
}
/**
 * attach Метод прикрепления контекста клиента к контексту сервера
 * @param target  контекст назначения
 * @param address объект подключения
 * @return        объект SSL контекста
 */
void awh::Engine::attach(ctx_t & target, addr_t * address) noexcept {
	// Если данные переданы
	if(address != nullptr){
		// Устанавливаем файловый дескриптор
		target._addr = address;
		// Устанавливаем тип приложения
		target._type = type_t::SERVER;
		// Если тип подключения является клиент
		if((target._addr->fd > -1) && target._tls){
			// Извлекаем BIO cthdthf
			target._bio = SSL_get_rbio(target._ssl);
			// Устанавливаем сокет клиента
			BIO_set_fd(target._bio, target._addr->fd, BIO_NOCLOSE);
			// Выполняем установку объекта подключения в BIO
			BIO_ctrl(target._bio, BIO_CTRL_DGRAM_SET_CONNECTED, 0, (struct sockaddr *) &target._addr->_peer.client);
		}
	}
}
/**
 * wrap Метод обертывания файлового дескриптора для сервера
 * @param target  контекст назначения
 * @param address объект подключения
 * @param type    тип активного приложения
 * @return        объект SSL контекста
 */
void awh::Engine::wrap(ctx_t & target, addr_t * address, const type_t type) noexcept {
	// Если данные переданы
	if(address != nullptr){
		// Устанавливаем тип приложения
		target._type = type;
		// Устанавливаем файловый дескриптор
		target._addr = address;
		// Если объект фреймворка существует
		if(target._addr->fd > -1){
			// Активируем рандомный генератор
			if(RAND_poll() < 1){
				// Выводим в лог сообщение
				this->_log->print("%s", log_t::flag_t::CRITICAL, "rand poll is not allow");
				// Выходим
				return;
			}
			// Определяем тип сокета
			switch(target._addr->_type){
				// Если тип сокета - диграммы
				case SOCK_DGRAM: {
					// Определяем тип активного приложения
					switch((uint8_t) type){
						// Если приложение является клиентом
						case (uint8_t) type_t::CLIENT:
							// Получаем контекст OpenSSL
							target._ctx = SSL_CTX_new(DTLSv1_client_method());
						break;
						// Если приложение является сервером
						case (uint8_t) type_t::SERVER:
							// Получаем контекст OpenSSL
							target._ctx = SSL_CTX_new(DTLSv1_server_method());
						break;
					}
				} break;
				// Если тип сокета - потоки
				case SOCK_STREAM: {
					// Определяем тип активного приложения
					switch((uint8_t) type){
						// Если приложение является клиентом
						case (uint8_t) type_t::CLIENT:
							// Получаем контекст OpenSSL
							target._ctx = SSL_CTX_new(TLSv1_2_client_method());
						break;
						// Если приложение является сервером
						case (uint8_t) type_t::SERVER:
							// Получаем контекст OpenSSL
							target._ctx = SSL_CTX_new(TLSv1_2_server_method());
						break;
					}
				} break;
			}
			// Если контекст не создан
			if(target._ctx == nullptr){
				// Выводим в лог сообщение
				this->_log->print("%s", log_t::flag_t::CRITICAL, "context ssl is not initialization");
				// Выходим
				return;
			}
			// Устанавливаем опции запроса
			SSL_CTX_set_options(target._ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
			// Устанавливаем минимально-возможную версию TLS
			SSL_CTX_set_min_proto_version(target._ctx, 0);
			// Устанавливаем максимально-возможную версию TLS
			SSL_CTX_set_max_proto_version(target._ctx, TLS1_3_VERSION);
			// Если нужно установить основные алгоритмы шифрования
			if(!this->_cipher.empty()){
				// Устанавливаем все основные алгоритмы шифрования
				if(SSL_CTX_set_cipher_list(target._ctx, this->_cipher.c_str()) < 1){
					// Очищаем созданный контекст
					target.clear();
					// Выводим в лог сообщение
					this->_log->print("%s", log_t::flag_t::CRITICAL, "set ssl ciphers");
					// Выходим
					return;
				}
				// Если приложение является сервером
				if(type == type_t::SERVER)
					// Заставляем серверные алгоритмы шифрования использовать в приоритете
					SSL_CTX_set_options(target._ctx, SSL_OP_CIPHER_SERVER_PREFERENCE);
			}
			// Устанавливаем поддерживаемые кривые
			if(SSL_CTX_set_ecdh_auto(target._ctx, 1) < 1){
				// Очищаем созданный контекст
				target.clear();
				// Выводим в лог сообщение
				this->_log->print("%s", log_t::flag_t::CRITICAL, "set ssl ecdh");
				// Выходим
				return;
			}
			// Если приложение является сервером
			if(type == type_t::SERVER)
				// Выполняем отключение SSL кеша
				SSL_CTX_set_session_cache_mode(target._ctx, SSL_SESS_CACHE_OFF);
			// Если цепочка сертификатов установлена
			if(!this->_chain.empty()){
				// Определяем тип активного приложения
				switch((uint8_t) type){
					// Если приложение является клиентом
					case (uint8_t) type_t::CLIENT:
						// Если цепочка сертификатов не установлена
						if(SSL_CTX_use_certificate_file(target._ctx, this->_chain.c_str(), SSL_FILETYPE_PEM) < 1){
							// Выводим в лог сообщение
							this->_log->print("%s", log_t::flag_t::CRITICAL, "certificate cannot be set");
							// Очищаем созданный контекст
							target.clear();
							// Выходим
							return;
						}
					break;
					// Если приложение является сервером
					case (uint8_t) type_t::SERVER:
						// Если цепочка сертификатов не установлена
						if(SSL_CTX_use_certificate_chain_file(target._ctx, this->_chain.c_str()) < 1){
							// Выводим в лог сообщение
							this->_log->print("%s", log_t::flag_t::CRITICAL, "certificate cannot be set");
							// Очищаем созданный контекст
							target.clear();
							// Выходим
							return;
						}
					break;
				}
			}
			// Если приватный ключ установлен
			if(!this->_privkey.empty()){
				// Если приватный ключ не может быть установлен
				if(SSL_CTX_use_PrivateKey_file(target._ctx, this->_privkey.c_str(), SSL_FILETYPE_PEM) < 1){
					// Выводим в лог сообщение
					this->_log->print("%s", log_t::flag_t::CRITICAL, "private key cannot be set");
					// Очищаем созданный контекст
					target.clear();
					// Выходим
					return;
				}
				// Если приватный ключ недействителен
				if(SSL_CTX_check_private_key(target._ctx) < 1){
					// Выводим в лог сообщение
					this->_log->print("%s", log_t::flag_t::CRITICAL, "private key is not valid");
					// Очищаем созданный контекст
					target.clear();
					// Выходим
					return;
				}
			}
			// Если нужно произвести проверку
			if(this->_verify){
				// Устанавливаем глубину проверки
				SSL_CTX_set_verify_depth(target._ctx, 2);
				// Выполняем проверку сертификата клиента
				SSL_CTX_set_verify(target._ctx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, &verifyCert);
			// Запрещаем выполнять првоерку сертификата пользователя
			} else SSL_CTX_set_verify(target._ctx, SSL_VERIFY_NONE, nullptr);
			// Устанавливаем, что мы должны читать как можно больше входных байтов
			SSL_CTX_set_read_ahead(target._ctx, 1);
			// Если приложение является сервером
			if(type == type_t::SERVER){
				// Определяем тип сокета
				switch(target._addr->_type){
					// Если тип сокета - диграммы
					case SOCK_DGRAM: {
						// Выполняем проверку файлов печенок
						SSL_CTX_set_cookie_verify_cb(target._ctx, &verifyCookie);
						// Выполняем генерацию файлов печенок
						SSL_CTX_set_cookie_generate_cb(target._ctx, &generateCookie);
					} break;
					// Если тип сокета - потоки
					case SOCK_STREAM: {
						// Выполняем проверку файлов печенок
						SSL_CTX_set_stateless_cookie_verify_cb(target._ctx, &verifyStatelessCookie);
						// Выполняем генерацию файлов печенок
						SSL_CTX_set_stateless_cookie_generate_cb(target._ctx, &generateStatelessCookie);
					} break;
				}
			}
			// Создаем SSL объект
			target._ssl = SSL_new(target._ctx);
			// Если объект не создан
			if(!(target._tls = (target._ssl != nullptr))){
				// Очищаем созданный контекст
				target.clear();
				// Выводим в лог сообщение
				this->_log->print("%s", log_t::flag_t::CRITICAL, "ssl initialization is not allow");
				// Выходим
				return;
			}
			// Устанавливаем флаг активации TLS
			target._addr->_tls = target._tls;
			// Определяем тип сокета
			switch(target._addr->_type){
				// Если тип сокета - диграммы
				case SOCK_DGRAM:
					// Выполняем обёртывание сокета UDP в BIO SSL
					target._bio = BIO_new_dgram(target._addr->fd, BIO_NOCLOSE);
				break;
				// Если тип сокета - потоки
				case SOCK_STREAM:
					// Выполняем обёртывание сокета в BIO SSL
					target._bio = BIO_new_socket(target._addr->fd, BIO_NOCLOSE);
				break;
			}
			// Если BIO SSL создано
			if(target._bio != nullptr){
				// Выполняем установку BIO SSL
				SSL_set_bio(target._ssl, target._bio, target._bio);
				// Определяем тип активного приложения
				switch((uint8_t) type){
					// Если приложение является клиентом
					case (uint8_t) type_t::CLIENT: {
						// Если тип сокета - диграммы
						if(target._addr->_type == SOCK_DGRAM)
							// Выполняем установку объекта подключения в BIO
							BIO_ctrl(target._bio, BIO_CTRL_DGRAM_SET_CONNECTED, 0, (struct sockaddr *) &target._addr->_peer.server);
					} break;
					// Если приложение является сервером
					case (uint8_t) type_t::SERVER: {
						// Устанавливаем неблокирующий режим ввода/вывода для сокета
						target.noblock();
						// Включаем обмен куками
						SSL_set_options(target._ssl, SSL_OP_COOKIE_EXCHANGE);
					} break;
				}
			// Если BIO SSL не создано
			} else {
				// Очищаем созданный контекст
				target.clear();
				// Выводим сообщение об ошибке
				this->_log->print("BIO new socket is failed", log_t::flag_t::CRITICAL);
				// Выходим из функции
				return;
			}
		// Очищаем созданный контекст
		} else target.clear();
	}
}
/**
 * wrapServer Метод обертывания файлового дескриптора для сервера
 * @param target контекст назначения
 * @param source исходный контекст
 * @return       объект SSL контекста
 */
void awh::Engine::wrapServer(ctx_t & target, ctx_t & source) noexcept {
	// Если объект ещё не обёрнут в SSL контекст
	if(!source._tls && (source._addr != nullptr))
		// Выполняем обёртывание уже активного SSL контекста
		this->wrapServer(target, source._addr);
}
/**
 * wrapServer Метод обертывания файлового дескриптора для сервера
 * @param target  контекст назначения
 * @param address объект подключения
 * @return        объект SSL контекста
 */
void awh::Engine::wrapServer(ctx_t & target, addr_t * address) noexcept {
	// Если данные переданы
	if(address != nullptr){
		// Устанавливаем файловый дескриптор
		target._addr = address;
		// Устанавливаем тип приложения
		target._type = type_t::SERVER;
		// Проверяем семейство протоколов сервера
		switch(target._addr->_peer.client.ss_family){
			// Если семейство протоколов IPv4
			case AF_INET:
			// Если семейство протоколов IPv6
			case AF_INET6: break;
			// Если семейство протоколов другое, выходим
			default: return;
		}
		// Если тип сокетов установлен не как потоковые, выходим
		if(target._addr->_type != SOCK_STREAM) return;
		// Если объект фреймворка существует
		if((target._addr->fd > -1) && !this->_privkey.empty() && !this->_chain.empty()){
			// Активируем рандомный генератор
			if(RAND_poll() < 1){
				// Выводим в лог сообщение
				this->_log->print("%s", log_t::flag_t::CRITICAL, "rand poll is not allow");
				// Выходим
				return;
			}
			// Получаем контекст OpenSSL
			target._ctx = SSL_CTX_new(TLSv1_2_server_method());
			// Если контекст не создан
			if(target._ctx == nullptr){
				// Выводим в лог сообщение
				this->_log->print("%s", log_t::flag_t::CRITICAL, "context ssl is not initialization");
				// Выходим
				return;
			}
			// Устанавливаем опции запроса
			SSL_CTX_set_options(target._ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
			// Устанавливаем минимально-возможную версию TLS
			SSL_CTX_set_min_proto_version(target._ctx, 0);
			// Устанавливаем максимально-возможную версию TLS
			SSL_CTX_set_max_proto_version(target._ctx, TLS1_3_VERSION);
			// Если нужно установить основные алгоритмы шифрования
			if(!this->_cipher.empty()){
				// Устанавливаем все основные алгоритмы шифрования
				if(SSL_CTX_set_cipher_list(target._ctx, this->_cipher.c_str()) < 1){
					// Очищаем созданный контекст
					target.clear();
					// Выводим в лог сообщение
					this->_log->print("%s", log_t::flag_t::CRITICAL, "set ssl ciphers");
					// Выходим
					return;
				}
				// Заставляем серверные алгоритмы шифрования использовать в приоритете
				SSL_CTX_set_options(target._ctx, SSL_OP_CIPHER_SERVER_PREFERENCE);
			}
			// Устанавливаем поддерживаемые кривые
			if(SSL_CTX_set_ecdh_auto(target._ctx, 1) < 1){
				// Очищаем созданный контекст
				target.clear();
				// Выводим в лог сообщение
				this->_log->print("%s", log_t::flag_t::CRITICAL, "set ssl ecdh");
				// Выходим
				return;
			}
			// Выполняем инициализацию доверенного сертификата
			if(!this->storeCA(target._ctx)){
				// Очищаем созданный контекст
				target.clear();
				// Выходим
				return;
			}
			// Устанавливаем флаг quiet shutdown
			// SSL_CTX_set_quiet_shutdown(target._ctx, 1);
			// Запускаем кэширование
			SSL_CTX_set_session_cache_mode(target._ctx, SSL_SESS_CACHE_SERVER | SSL_SESS_CACHE_NO_INTERNAL);
			// Если цепочка сертификатов установлена
			if(!this->_chain.empty()){
				// Если цепочка сертификатов не установлена
				if(SSL_CTX_use_certificate_chain_file(target._ctx, this->_chain.c_str()) < 1){
					// Выводим в лог сообщение
					this->_log->print("%s", log_t::flag_t::CRITICAL, "certificate cannot be set");
					// Очищаем созданный контекст
					target.clear();
					// Выходим
					return;
				}
			}
			// Если приватный ключ установлен
			if(!this->_privkey.empty()){
				// Если приватный ключ не может быть установлен
				if(SSL_CTX_use_PrivateKey_file(target._ctx, this->_privkey.c_str(), SSL_FILETYPE_PEM) < 1){
					// Выводим в лог сообщение
					this->_log->print("%s", log_t::flag_t::CRITICAL, "private key cannot be set");
					// Очищаем созданный контекст
					target.clear();
					// Выходим
					return;
				}
				// Если приватный ключ недействителен
				if(SSL_CTX_check_private_key(target._ctx) < 1){
					// Выводим в лог сообщение
					this->_log->print("%s", log_t::flag_t::CRITICAL, "private key is not valid");
					// Очищаем созданный контекст
					target.clear();
					// Выходим
					return;
				}
			}
			// Если доверенный сертификат недействителен
			if(SSL_CTX_set_default_verify_file(target._ctx) < 1){
				// Выводим в лог сообщение
				this->_log->print("%s", log_t::flag_t::CRITICAL, "trusted certificate is invalid");
				// Очищаем созданный контекст
				target.clear();
				// Выходим
				return;
			}
			// Заставляем OpenSSL автоматические повторные попытки после событий сеанса TLS
			SSL_CTX_set_mode(target._ctx, SSL_MODE_AUTO_RETRY);
			// Запрещаем выполнять првоерку сертификата пользователя
			SSL_CTX_set_verify(target._ctx, SSL_VERIFY_NONE, nullptr);
			// Создаем SSL объект
			target._ssl = SSL_new(target._ctx);
			// Если объект не создан
			if(!(target._tls = (target._ssl != nullptr))){
				// Очищаем созданный контекст
				target.clear();
				// Выводим в лог сообщение
				this->_log->print("%s", log_t::flag_t::CRITICAL, "ssl initialization is not allow");
				// Выходим
				return;
			}
			// Проверяем рукопожатие
			if(SSL_do_handshake(target._ssl) < 1){
				// Выполняем проверку рукопожатия
				const long verify = SSL_get_verify_result(target._ssl);
				// Если рукопожатие не выполнено
				if(verify != X509_V_OK){
					// Очищаем созданный контекст
					target.clear();
					// Выводим в лог сообщение
					this->_log->print("certificate chain validation failed: %s", log_t::flag_t::CRITICAL, X509_verify_cert_error_string(verify));
					// Выходим
					return;
				}
			}
			// Устанавливаем флаг активации TLS
			target._addr->_tls = target._tls;
			// Выполняем обёртывание сокета в BIO SSL
			target._bio = BIO_new_socket(target._addr->fd, BIO_NOCLOSE);
			// Если BIO SSL создано
			if(target._bio != nullptr){
				// Устанавливаем неблокирующий режим ввода/вывода для сокета
				target.noblock();
				// Выполняем установку BIO SSL
				SSL_set_bio(target._ssl, target._bio, target._bio);
			// Если BIO SSL не создано
			} else {
				// Очищаем созданный контекст
				target.clear();
				// Выводим сообщение об ошибке
				this->_log->print("BIO new socket is failed", log_t::flag_t::CRITICAL);
				// Выходим из функции
				return;
			}
		}
	}
}
/**
 * wrapClient Метод обертывания файлового дескриптора для клиента
 * @param target контекст назначения
 * @param source исходный контекст
 * @param url    параметры URL адреса для инициализации
 * @return       объект SSL контекста
 */
void awh::Engine::wrapClient(ctx_t & target, ctx_t & source, const uri_t::url_t & url) noexcept {
	// Если объект ещё не обёрнут в SSL контекст
	if(!source._tls && (source._addr != nullptr))
		// Выполняем обёртывание уже активного SSL контекста
		this->wrapClient(target, source._addr, url);
}
/**
 * wrapClient Метод обертывания файлового дескриптора для клиента
 * @param target  контекст назначения
 * @param address объект подключения
 * @param url     параметры URL адреса для инициализации
 * @return        объект SSL контекста
 */
void awh::Engine::wrapClient(ctx_t & target, addr_t * address, const uri_t::url_t & url) noexcept {
	// Если данные переданы
	if((address != nullptr) && !url.empty()){
		// Устанавливаем файловый дескриптор
		target._addr = address;
		// Устанавливаем тип приложения
		target._type = type_t::CLIENT;
		// Если объект фреймворка существует
		if((target._addr->fd > -1) && (!url.domain.empty() || !url.ip.empty()) && ((url.schema.compare("https") == 0) || (url.schema.compare("wss") == 0))){
			// Активируем рандомный генератор
			if(RAND_poll() < 1){
				// Выводим в лог сообщение
				this->_log->print("%s", log_t::flag_t::CRITICAL, "rand poll is not allow");
				// Выходим
				return;
			}
			// Получаем контекст OpenSSL
			target._ctx = SSL_CTX_new(TLSv1_2_client_method());
			// Если контекст не создан
			if(target._ctx == nullptr){
				// Выводим в лог сообщение
				this->_log->print("%s", log_t::flag_t::CRITICAL, "context ssl is not initialization");
				// Выходим
				return;
			}
			// Устанавливаем опции запроса
			SSL_CTX_set_options(target._ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
			// Выполняем инициализацию доверенного сертификата
			if(!this->storeCA(target._ctx)){
				// Очищаем созданный контекст
				target.clear();
				// Выходим
				return;
			}
			// Если нужно установить основные алгоритмы шифрования
			if(!this->_cipher.empty()){
				// Устанавливаем все основные алгоритмы шифрования
				if(SSL_CTX_set_cipher_list(target._ctx, this->_cipher.c_str()) < 1){
					// Очищаем созданный контекст
					target.clear();
					// Выводим в лог сообщение
					this->_log->print("%s", log_t::flag_t::CRITICAL, "set ssl ciphers");
					// Выходим
					return;
				}
			}
			// Заставляем OpenSSL автоматические повторные попытки после событий сеанса TLS
			SSL_CTX_set_mode(target._ctx, SSL_MODE_AUTO_RETRY);
			// Если цепочка сертификатов установлена
			if(!this->_chain.empty()){
				// Если цепочка сертификатов не установлена
				if(SSL_CTX_use_certificate_file(target._ctx, this->_chain.c_str(), SSL_FILETYPE_PEM) < 1){
					// Выводим в лог сообщение
					this->_log->print("%s", log_t::flag_t::CRITICAL, "certificate cannot be set");
					// Очищаем созданный контекст
					target.clear();
					// Выходим
					return;
				}
			}
			// Если приватный ключ установлен
			if(!this->_privkey.empty()){
				// Если приватный ключ не может быть установлен
				if(SSL_CTX_use_PrivateKey_file(target._ctx, this->_privkey.c_str(), SSL_FILETYPE_PEM) < 1){
					// Выводим в лог сообщение
					this->_log->print("%s", log_t::flag_t::CRITICAL, "private key cannot be set");
					// Очищаем созданный контекст
					target.clear();
					// Выходим
					return;
				}
				// Если приватный ключ недействителен
				if(SSL_CTX_check_private_key(target._ctx) < 1){
					// Выводим в лог сообщение
					this->_log->print("%s", log_t::flag_t::CRITICAL, "private key is not valid");
					// Очищаем созданный контекст
					target.clear();
					// Выходим
					return;
				}
			}
			// Если нужно произвести проверку
			if(this->_verify && !url.domain.empty()){
				// Создаём объект проверки домена
				target._verify = new verify_t(url.domain, this);
				// Выполняем проверку сертификата
				SSL_CTX_set_verify(target._ctx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, nullptr);
				// Выполняем проверку всех дочерних сертификатов
				SSL_CTX_set_cert_verify_callback(target._ctx, &verifyHost, target._verify);
				// Устанавливаем глубину проверки
				SSL_CTX_set_verify_depth(target._ctx, 4);
			// Запрещаем выполнять првоерку сертификата пользователя
			} else SSL_CTX_set_verify(target._ctx, SSL_VERIFY_NONE, nullptr);
			// Создаем SSL объект
			target._ssl = SSL_new(target._ctx);
			// Если объект не создан
			if(!(target._tls = (target._ssl != nullptr))){
				// Очищаем созданный контекст
				target.clear();
				// Выводим в лог сообщение
				this->_log->print("%s", log_t::flag_t::CRITICAL, "ssl initialization is not allow");
				// Выходим
				return;
			}
			/**
			 * Если нужно установить TLS расширение
			 */
			#ifdef SSL_CTRL_SET_TLSEXT_HOSTNAME
				// Устанавливаем имя хоста для SNI расширения
				SSL_set_tlsext_host_name(target._ssl, (!url.domain.empty() ? url.domain : url.ip).c_str());
			#endif
			// Активируем верификацию доменного имени
			if(X509_VERIFY_PARAM_set1_host(SSL_get0_param(target._ssl), (!url.domain.empty() ? url.domain : url.ip).c_str(), 0) < 1){
				// Очищаем созданный контекст
				target.clear();
				// Выводим в лог сообщение
				this->_log->print("%s", log_t::flag_t::CRITICAL, "domain ssl verification failed");
				// Выходим
				return;
			}
			// Проверяем рукопожатие
			if(SSL_do_handshake(target._ssl) < 1){
				// Выполняем проверку рукопожатия
				const long verify = SSL_get_verify_result(target._ssl);
				// Если рукопожатие не выполнено
				if(verify != X509_V_OK){
					// Очищаем созданный контекст
					target.clear();
					// Выводим в лог сообщение
					this->_log->print("certificate chain validation failed: %s", log_t::flag_t::CRITICAL, X509_verify_cert_error_string(verify));
				}
			}
			// Устанавливаем флаг активации TLS
			target._addr->_tls = target._tls;
			// Выполняем обёртывание сокета TCP в BIO SSL
			target._bio = BIO_new_socket(target._addr->fd, BIO_NOCLOSE);
			// Если BIO SSL создано
			if(target._bio != nullptr){
				// Устанавливаем блокирующий режим ввода/вывода для сокета
				target.block();
				// Выполняем установку BIO SSL
				SSL_set_bio(target._ssl, target._bio, target._bio);
			// Если BIO SSL не создано
			} else {
				// Очищаем созданный контекст
				target.clear();
				// Выводим сообщение об ошибке
				this->_log->print("BIO new socket is failed", log_t::flag_t::CRITICAL);
				// Выходим из функции
				return;
			}
		}
	}
}
/**
 * verifyEnable Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
 * @param mode флаг состояния разрешения проверки
 */
void awh::Engine::verifyEnable(const bool mode) noexcept {
	// Устанавливаем флаг проверки
	this->_verify = mode;
}
/**
 * ciphers Метод установки алгоритмов шифрования
 * @param ciphers список алгоритмов шифрования для установки
 */
void awh::Engine::ciphers(const vector <string> & ciphers) noexcept {
	// Если список алгоритмов шифрования передан
	if(!ciphers.empty()){
		// Очищаем установленный список алгоритмов шифрования
		this->_cipher.clear();
		// Переходим по всему списку алгоритмов шифрования
		for(auto & cip : ciphers){
			// Если список алгоритмов шифрования уже не пустой, вставляем разделитель
			if(!this->_cipher.empty())
				// Устанавливаем разделитель
				this->_cipher.append(":");
			// Устанавливаем алгоритм шифрования
			this->_cipher.append(cip);
		}
	}
}
/**
 * ca Метод установки доверенного сертификата (CA-файла)
 * @param trusted адрес доверенного сертификата (CA-файла)
 * @param path    адрес каталога где находится сертификат (CA-файл)
 */
void awh::Engine::ca(const string & trusted, const string & path) noexcept {
	// Если адрес CA-файла передан
	if(!trusted.empty()){
		// Устанавливаем адрес доверенного сертификата (CA-файла)
		this->_ca = fs_t::realPath(trusted);
		// Если адрес каталога с доверенным сертификатом (CA-файлом) передан, устанавливаем и его
		if(!path.empty() && fs_t::isdir(path))
			// Устанавливаем адрес каталога с доверенным сертификатом (CA-файлом)
			this->_path = fs_t::realPath(path);
	}
}
/**
 * setCert Метод установки файлов сертификата
 * @param chain файл цепочки сертификатов
 * @param key   приватный ключ сертификата (если требуется)
 */
void awh::Engine::certificate(const string & chain, const string & key) noexcept {
	// Устанавливаем приватный ключ сертификата
	this->_privkey = key;
	// Устанавливаем файл полной цепочки сертификатов
	this->_chain = chain;
}
/**
 * Engine Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 * @param uri объект работы с URI
 */
awh::Engine::Engine(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept :
 _verify(true), _cipher(""), _chain(""), _privkey(""), _path(""),
 _ca(SSL_CA_FILE), _fmk(fmk), _uri(uri), _log(log) {
	// Выполняем модификацию доверенного сертификата (CA-файла)
	this->_ca = fs_t::realPath(this->_ca);
	// Выполняем установку алгоритмов шифрования
	this->ciphers({
		"ECDHE-RSA-AES128-GCM-SHA256",
		"ECDHE-ECDSA-AES128-GCM-SHA256",
		"ECDHE-RSA-AES256-GCM-SHA384",
		"ECDHE-ECDSA-AES256-GCM-SHA384",
		"DHE-RSA-AES128-GCM-SHA256",
		"DHE-DSS-AES128-GCM-SHA256",
		"kEDH+AESGCM",
		"ECDHE-RSA-AES128-SHA256",
		"ECDHE-ECDSA-AES128-SHA256",
		"ECDHE-RSA-AES128-SHA",
		"ECDHE-ECDSA-AES128-SHA",
		"ECDHE-RSA-AES256-SHA384",
		"ECDHE-ECDSA-AES256-SHA384",
		"ECDHE-RSA-AES256-SHA",
		"ECDHE-ECDSA-AES256-SHA",
		"DHE-RSA-AES128-SHA256",
		"DHE-RSA-AES128-SHA",
		"DHE-DSS-AES128-SHA256",
		"DHE-RSA-AES256-SHA256",
		"DHE-DSS-AES256-SHA",
		"DHE-RSA-AES256-SHA",
		"AES128-GCM-SHA256",
		"AES256-GCM-SHA384",
		"AES128-SHA256",
		"AES256-SHA256",
		"AES128-SHA",
		"AES256-SHA",
		"AES",
		"CAMELLIA",
		"DES-CBC3-SHA",
		"!aNULL",
		"!eNULL",
		"!EXPORT",
		"!DES",
		"!RC4",
		"!MD5",
		"!PSK",
		"!aECDH",
		"!EDH-DSS-DES-CBC3-SHA",
		"!EDH-RSA-DES-CBC3-SHA",
		"!KRB5-DES-CBC3-SHA"
	});
	/**
	 * Если версия OPENSSL старая
	 */
	#if (OPENSSL_VERSION_NUMBER < 0x10100000L) || (defined(LIBRESSL_VERSION_NUMBER) && LIBRESSL_VERSION_NUMBER < 0x20700000L)
		// Выполняем инициализацию OpenSSL
		SSL_library_init();
		ERR_load_crypto_strings();
		SSL_load_error_strings();
		OpenSSL_add_all_algorithms();
	/**
	 * Для более свежей версии
	 */
	#else
		// Выполняем инициализацию OpenSSL
		OPENSSL_init_ssl(OPENSSL_INIT_SSL_DEFAULT, nullptr);
	#endif
}
/**
 * ~Engine Деструктор
 */
awh::Engine::~Engine() noexcept {
	/**
	 * Если версия OpenSSL старая
	 */
	#if (OPENSSL_VERSION_NUMBER < 0x10100000L) || (defined(LIBRESSL_VERSION_NUMBER) && LIBRESSL_VERSION_NUMBER < 0x20700000L)
		// Выполняем освобождение памяти
		EVP_cleanup();
		ERR_free_strings();
		/**
		 * Если версия OpenSSL старая
		 */
		#if OPENSSL_VERSION_NUMBER < 0x10000000L
			// Освобождаем стейт
			ERR_remove_state(0);
		/**
		 * Если версия OpenSSL более новая
		 */
		#else
			// Освобождаем стейт для потока
			ERR_remove_thread_state(nullptr);
		#endif
		// Освобождаем оставшиеся данные
		CRYPTO_cleanup_all_ex_data();
		sk_SSL_COMP_free(SSL_COMP_get_compression_methods());
	#endif
}
