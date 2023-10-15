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
 * close Метод закрытия подключения
 * @return результат выполнения операции
 */
bool awh::Engine::Address::close() noexcept {
	// Результат работы функции
	bool result = false;
	// Если файловый дескриптор подключён
	if((result = (this->fd != INVALID_SOCKET) && (this->fd < MAX_SOCKETS))){
		/**
		 * Если операционной системой является Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Если тип сокета не диграммы
			if(this->_type != SOCK_DGRAM)
				// Запрещаем работу с сокетом
				shutdown(this->fd, SD_BOTH);
			// Выполняем закрытие сокета
			closesocket(this->fd);
		/**
		 * Если операционной системой является Nix-подобная
		 */
		#else
			// Если тип сокета не диграммы
			if(this->_type != SOCK_DGRAM)
				// Запрещаем работу с сокетом
				shutdown(this->fd, SHUT_RDWR);
			// Выполняем закрытие сокета
			::close(this->fd);
		#endif
		// Выполняем сброс файлового дескриптора
		this->fd = INVALID_SOCKET;
	}
	// Выводим результат
	return result;
}
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
			::memset(buffer, 0, sizeof(buffer));
			// Определяем тип протокола интернета
			switch(this->_peer.client.ss_family){
				// Если протокол интернета IPv4
				case AF_INET: {
					// Получаем порт клиента
					this->port = ntohs(reinterpret_cast <struct sockaddr_in *> (&this->_peer.client)->sin_port);
					// Получаем IP адрес
					this->ip = inet_ntop(AF_INET, &reinterpret_cast <struct sockaddr_in *> (&this->_peer.client)->sin_addr, buffer, sizeof(buffer));
				} break;
				// Если протокол интернета IPv6
				case AF_INET6: {
					// Получаем порт клиента
					this->port = ntohs(reinterpret_cast <struct sockaddr_in6 *> (&this->_peer.client)->sin6_port);
					// Получаем IP адрес
					this->ip = inet_ntop(AF_INET6, &reinterpret_cast <struct sockaddr_in6 *> (&this->_peer.client)->sin6_addr, buffer, sizeof(buffer));
				} break;
			}
			// Получаем данные подключившегося клиента
			string ip = this->_ifnet.ip(reinterpret_cast <struct sockaddr *> (&this->_peer.client), this->_peer.client.ss_family);
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
				this->_log->print("Listen service: pid = %u", log_t::flag_t::CRITICAL, getpid());
				// Выходим из функции
				return result;
			}
			/**
			 * Если операционной системой является Linux или FreeBSD
			 */
			#if defined(__linux__) || defined(__FreeBSD__)
				// Если протокол интернета установлен как SCTP
				if(this->_protocol == IPPROTO_SCTP){
					// Выполняем инициализацию SCTP протокола
					this->_socket.eventsSCTP(this->fd);
					/**
					 * Создаём BIO, чтобы установить все необходимые параметры для
					 * следующего соединения, например. SCTP-АУТЕНТИФИКАЦИЯ.
					 * Не будет использоваться.
					 */
					this->_bio = BIO_new_dgram_sctp(this->fd, BIO_NOCLOSE);
					// Если BIO не создано, выходим
					if(this->_bio == nullptr){
						// Выводим в лог информацию
						this->_log->print("Unable to create BIO for SCTP protocol", log_t::flag_t::CRITICAL);
						// Выходим из приложения
						exit(EXIT_FAILURE);
					}
				}
			#endif
		} break;
		// Если сокет установлен UDP
		case SOCK_DGRAM: return true;
	}
	// Выводим результат
	return result;
}
/**
 * clear Метод очистки параметров подключения
 * @return результат выполнения операции
 */
bool awh::Engine::Address::clear() noexcept {
	// Результат работы функции
	bool result = false;
	// Если BIO создано
	if(this->_bio != nullptr){
		// Выполняем очистку BIO
		BIO_free(this->_bio);
		// Зануляем контекст BIO
		this->_bio = nullptr;
	}
	// Сбрасываем флаг инициализации
	this->_encrypted = false;
	// Зануляем структуру клиента
	::memset(&this->_peer.client, 0, sizeof(this->_peer.client));
	// Зануляем структуру сервера
	::memset(&this->_peer.server, 0, sizeof(this->_peer.server));
	// Закрываем основной сокет
	return this->close();
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
						strlen(reinterpret_cast <struct sockaddr_un *> (&this->_peer.server)->sun_path)
					);
				break;
			#endif
		}
		/**
		 * Если операционной системой является Linux или FreeBSD
		 */
		#if defined(__linux__) || defined(__FreeBSD__)
			// Если протокол интернета установлен как SCTP
			if(this->_protocol == IPPROTO_SCTP)
				// Выполняем инициализацию SCTP протокола
				this->_socket.eventsSCTP(this->fd);
		#endif
		// Если подключение не выполненно то сообщаем об этом, выполняем подключение к удаленному серверу
		if((this->_peer.size > 0) && (::connect(this->fd, reinterpret_cast <struct sockaddr *> (&this->_peer.server), this->_peer.size) == 0))
			// Устанавливаем статус подключения
			this->status = status_t::CONNECTED;
	// Если сокет установлен UDP
	} else if(this->_type == SOCK_DGRAM) {
		// Если подключение зашифрованно, значит мы должны использовать DTLS
		if(this->_encrypted){
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
			if((this->_peer.size > 0) && (::connect(this->fd, reinterpret_cast <struct sockaddr *> (&this->_peer.server), this->_peer.size) == 0))
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
 * host Метод извлечения хоста компьютера
 * @param family семейство сокета (AF_INET / AF_INET6)
 * @return       хост компьютера с которого производится запрос
 */
string awh::Engine::Address::host(const int family) const noexcept {
	// Результат работы функции
	string result = "";
	// Если список сетей установлен
	if(!this->network.empty()){
		// Если количество элементов больше 1
		if(this->network.size() > 1){
			// Подключаем устройство генератора
			mt19937 random(const_cast <addr_t *> (this)->_randev());
			// Выполняем генерирование случайного числа
			uniform_int_distribution <mt19937::result_type> dist6(0, this->network.size() - 1);
			// Получаем ip адрес
			result = this->network.at(dist6(random));
		}
		// Выводим только первый элемент
		result = this->network.front();
	}
	// Если IP-адрес не установлен
	if(result.empty()){
		// Определяем тип подключения
		switch(family){
			// Для протокола IPv6
			case AF_INET6: return "::";
			// Для протокола IPv4
			case AF_INET: return "0.0.0.0";
		}
	}
	// Выводим результат
	return result;
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
	::memcpy(&this->_peer.client, &addr._peer.client, sizeof(struct sockaddr_storage));
	// Выполняем копирование объекта подключения сервера
	::memcpy(&this->_peer.server, &addr._peer.server, sizeof(struct sockaddr_storage));
	// Выполняем бинд на сокет
	if((this->_peer.size > 0) && (::bind(this->fd, reinterpret_cast <struct sockaddr *> (&this->_peer.server), this->_peer.size) < 0)){
		// Хост подключённого клиента
		const string & client = this->_ifnet.ip(reinterpret_cast <struct sockaddr *> (&this->_peer.client), this->_peer.client.ss_family);
		// Выводим в лог сообщение
		this->_log->print("Bind client for UDP protocol [%s]", log_t::flag_t::CRITICAL, client.c_str());
		// Выходим
		return false;
	}
	// Если подключение не выполненно то сообщаем об этом, выполняем подключение к удаленному серверу
	if(::connect(this->fd, reinterpret_cast <struct sockaddr *> (&this->_peer.client), this->_peer.size) == 0){
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
bool awh::Engine::Address::accept(const SOCKET fd, const int family) noexcept {
	// Устанавливаем статус отключения
	this->status = status_t::DISCONNECTED;
	// Заполняем структуру клиента нулями
	::memset(&this->_peer.client, 0, sizeof(this->_peer.client));
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
					::memset(&client, 0, sizeof(client));
					// Устанавливаем протокол интернета
					client.sin_family = family;
					// Запоминаем размер структуры
					this->_peer.size = sizeof(client);
					// Выполняем копирование объекта подключения клиента
					::memcpy(&this->_peer.client, &client, this->_peer.size);
				} break;
				// Для протокола IPv6
				case AF_INET6: {
					// Создаём объект клиента
					struct sockaddr_in6 client;
					// Очищаем всю структуру для клиента
					::memset(&client, 0, sizeof(client));
					// Устанавливаем протокол интернета
					client.sin6_family = family;
					// Запоминаем размер структуры
					this->_peer.size = sizeof(client);
					// Выполняем копирование объекта подключения клиента
					::memcpy(&this->_peer.client, &client, this->_peer.size);
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
						::memset(&client, 0, sizeof(client));
						// Устанавливаем протокол интернета
						client.sun_family = family;
						// Запоминаем размер структуры
						this->_peer.size = sizeof(client);
						// Выполняем копирование объект подключения клиента в сторейдж
						::memcpy(&this->_peer.client, &client, sizeof(client));
					} break;
				#endif
			}
			// Определяем разрешено ли подключение к прокси серверу
			this->fd = ::accept(fd, reinterpret_cast <struct sockaddr *> (&this->_peer.client), &this->_peer.size);
			// Если сокет не создан тогда выходим
			if((this->fd == INVALID_SOCKET) || (this->fd >= MAX_SOCKETS))
				// Выходим из функции
				return false;
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
				this->_socket.noSigILL();
				// Если сокет установлен TCP/IP
				if(this->_type == SOCK_STREAM){
					// Отключаем сигнал записи в оборванное подключение
					this->_socket.noSigPIPE(this->fd);
					/**
					 * Если операционной системой является Linux или FreeBSD
					 */
					#if defined(__linux__) || defined(__FreeBSD__)
						// Если протокол интернета установлен как SCTP
						if(this->_protocol != IPPROTO_SCTP)
							// Активируем KeepAlive
							this->_socket.keepAlive(this->fd, this->alive.cnt, this->alive.idle, this->alive.intvl);
					/**
					 * Если операционной системой не является Linux
					 */
					#else
						// Активируем KeepAlive
						this->_socket.keepAlive(this->fd, this->alive.cnt, this->alive.idle, this->alive.intvl);
					#endif
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
				// Переводим сокет в не блокирующий режим
				this->_socket.setBlocking(this->fd, socket_t::mode_t::NOBLOCK);
				/**
				 * Если операционной системой является Linux или FreeBSD
				 */
				#if defined(__linux__) || defined(__FreeBSD__)
					// Если протокол интернета установлен как SCTP
					if(this->_protocol != IPPROTO_SCTP){
						// Отключаем алгоритм Нейгла для сервера и клиента
						this->_socket.nodelayTCP(this->fd);
						// Отключаем алгоритм Нейгла
						BIO_set_tcp_ndelay(this->fd, 1);
					}
				/**
				 * Если операционной системой не является Linux
				 */
				#else
					// Отключаем алгоритм Нейгла для сервера и клиента
					this->_socket.nodelayTCP(this->fd);
					// Отключаем алгоритм Нейгла
					BIO_set_tcp_ndelay(this->fd, 1);
				#endif
			}
		} break;
		/**
		 * Если операционной системой не является Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Для протокола unix-сокета
			case AF_UNIX: {
				// Выполняем игнорирование сигнала неверной инструкции процессора
				this->_socket.noSigILL();
				// Если сокет установлен TCP/IP
				if(this->_type == SOCK_STREAM){
					// Отключаем сигнал записи в оборванное подключение
					this->_socket.noSigPIPE(this->fd);
					// Устанавливаем разрешение на повторное использование сокета
					this->_socket.reuseable(this->fd);
					// Переводим сокет в не блокирующий режим
					this->_socket.setBlocking(this->fd, socket_t::mode_t::NOBLOCK);
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
 * @param protocol протокол сокета (IPPROTO_TCP / IPPROTO_UDP / IPPROTO_SCTP)
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
				if(this->_fs.isSock(unixsocket))
					// Удаляем файл сокета
					::unlink(unixsocket.c_str());
			}
			// Создаем сокет подключения
			this->fd = ::socket(AF_UNIX, this->_type, 0);
			// Если сокет не создан то выходим
			if((this->fd == INVALID_SOCKET) || (this->fd >= MAX_SOCKETS)){
				// Выводим сообщение в консоль
				this->_log->print("Creating socket %s", log_t::flag_t::CRITICAL, unixsocket.c_str());
				// Выходим
				return;
			}
			// Выполняем игнорирование сигнала неверной инструкции процессора
			this->_socket.noSigILL();
			// Устанавливаем разрешение на повторное использование сокета
			this->_socket.reuseable(this->fd);
			// Если сокет установлен TCP/IP
			if(this->_type == SOCK_STREAM){
				// Отключаем сигнал записи в оборванное подключение
				this->_socket.noSigPIPE(this->fd);
				// Переводим сокет в не блокирующий режим
				this->_socket.setBlocking(this->fd, socket_t::mode_t::NOBLOCK);
			}
			// Создаём объект подключения для клиента
			struct sockaddr_un client;
			// Создаём объект подключения для сервера
			struct sockaddr_un server;
			// Зануляем изначальную структуру данных клиента
			::memset(&client, 0, sizeof(client));
			// Зануляем изначальную структуру данных сервера
			::memset(&server, 0, sizeof(server));
			// Устанавливаем размер объекта подключения
			this->_peer.size = sizeof(struct sockaddr_un);
			// Определяем тип сокета
			switch(this->_type){
				// Если сокет установлен TCP/IP
				case SOCK_STREAM: {
					// Устанавливаем протокол интернета
					server.sun_family = AF_UNIX;
					// Очищаем всю структуру для сервера
					::memset(&server.sun_path, 0, sizeof(server.sun_path));
					// Копируем адрес сокета сервера
					::strncpy(server.sun_path, unixsocket.c_str(), sizeof(server.sun_path));
					// Выполняем копирование объект подключения сервера в сторейдж
					::memcpy(&this->_peer.server, &server, sizeof(server));
				} break;
				// Если сокет установлен UDP
				case SOCK_DGRAM: {
					// Устанавливаем протокол интернета для клиента
					client.sun_family = AF_UNIX;
					// Устанавливаем протокол интернета для сервера
					server.sun_family = AF_UNIX;
					// Очищаем всю структуру для клиента
					::memset(&client.sun_path, 0, sizeof(client.sun_path));
					// Очищаем всю структуру для сервера
					::memset(&server.sun_path, 0, sizeof(server.sun_path));
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
						switch(static_cast <uint8_t> (type)){
							// Если приложение является клиентом
							case static_cast <uint8_t> (type_t::CLIENT): {
								// Если сокет в файловой системе уже существует, удаляем его
								if(this->_fs.isSock(clientName))
									// Удаляем файл сокета
									::unlink(clientName.c_str());
							} break;
							// Если приложение является сервером
							case static_cast <uint8_t> (type_t::SERVER): {
								// Если сокет в файловой системе уже существует, удаляем его
								if(this->_fs.isSock(serverName))
									// Удаляем файл сокета
									::unlink(serverName.c_str());
							} break;
						}
						// Копируем адрес сокета сервера
						::strncpy(server.sun_path, serverName.c_str(), sizeof(server.sun_path));
						// Выполняем копирование объект подключения сервера в сторейдж
						::memcpy(&this->_peer.server, &server, sizeof(server));
						// Если приложение является клиентом
						if(type == type_t::CLIENT){
							// Копируем адрес сокета клиента
							::strncpy(client.sun_path, clientName.c_str(), sizeof(client.sun_path));
							// Выполняем копирование объект подключения клиента в сторейдж
							::memcpy(&this->_peer.client, &client, sizeof(client));
							// Получаем размер объекта сокета
							const socklen_t size = (offsetof(struct sockaddr_un, sun_path) + strlen(client.sun_path));
							// Выполняем бинд на сокет
							if(::bind(this->fd, reinterpret_cast <struct sockaddr *> (&this->_peer.client), size) < 0){
								// Выводим в лог сообщение
								this->_log->print("Bind local network for client [%s]", log_t::flag_t::CRITICAL, clientName.c_str());
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
				if(::bind(this->fd, reinterpret_cast <struct sockaddr *> (&this->_peer.server), size) < 0)
					// Выводим в лог сообщение
					this->_log->print("Bind local network for server [%s]", log_t::flag_t::CRITICAL, unixsocket.c_str());
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
	if(!ip.empty() && (port <= 65535) && !this->network.empty()){
		// Если список сетевых интерфейсов установлен
		if((family == AF_INET) || (family == AF_INET6)){
			// Получаем хост текущего компьютера
			const string & host = this->host(family);
			// Определяем тип подключения
			switch(family){
				// Для протокола IPv4
				case AF_INET: {
					// Определяем тип приложения
					switch(static_cast <uint8_t> (type)){
						// Если приложение является клиентом
						case static_cast <uint8_t> (type_t::CLIENT): {
							// Создаём объект клиента
							struct sockaddr_in client;
							// Очищаем всю структуру для клиента
							::memset(&client, 0, sizeof(client));
							// Устанавливаем протокол интернета
							client.sin_family = family;
							// Устанавливаем произвольный порт для локального подключения
							client.sin_port = htons(0);
							// Устанавливаем адрес для локальго подключения
							client.sin_addr.s_addr = inet_addr(host.c_str());
							// Запоминаем размер структуры
							this->_peer.size = sizeof(client);
							// Выполняем копирование объекта подключения клиента
							::memcpy(&this->_peer.client, &client, this->_peer.size);
						} break;
						// Если приложение является сервером
						case static_cast <uint8_t> (type_t::SERVER): {
							// Создаём объект клиента
							struct sockaddr_in client;
							// Очищаем всю структуру для клиента
							::memset(&client, 0, sizeof(client));
							// Устанавливаем протокол интернета
							client.sin_family = family;
							// Выполняем копирование объекта подключения клиента
							::memcpy(&this->_peer.client, &client, sizeof(client));
						} break;
					}
					// Создаём объект сервера
					struct sockaddr_in server;
					// Очищаем всю структуру для сервера
					::memset(&server, 0, sizeof(server));
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
					::memcpy(&this->_peer.server, &server, this->_peer.size);
					// Обнуляем серверную структуру
					::memset(&reinterpret_cast <struct sockaddr_in *> (&this->_peer.server)->sin_zero, 0, sizeof(server.sin_zero));
				} break;
				// Для протокола IPv6
				case AF_INET6: {
					// Определяем тип приложения
					switch(static_cast <uint8_t> (type)){
						// Если приложение является клиентом
						case static_cast <uint8_t> (type_t::CLIENT): {
							// Создаём объект клиента
							struct sockaddr_in6 client;
							// Очищаем всю структуру для клиента
							::memset(&client, 0, sizeof(client));
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
							::memcpy(&this->_peer.client, &client, this->_peer.size);
						} break;
						// Если приложение является сервером
						case static_cast <uint8_t> (type_t::SERVER): {
							// Создаём объект клиента
							struct sockaddr_in6 client;
							// Очищаем всю структуру для клиента
							::memset(&client, 0, sizeof(client));
							// Устанавливаем протокол интернета
							client.sin6_family = family;
							// Выполняем копирование объекта подключения клиента
							::memcpy(&this->_peer.client, &client, sizeof(client));
						} break;
					}
					// Создаём объект сервера
					struct sockaddr_in6 server;
					// Очищаем всю структуру для сервера
					::memset(&server, 0, sizeof(server));
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
					::memcpy(&this->_peer.server, &server, this->_peer.size);
				} break;
				// Если тип сети не определен
				default: {
					// Выводим сообщение в консоль
					this->_log->print("Network not allow from server = %s, port = %u", log_t::flag_t::CRITICAL, ip.c_str(), port);
					// Выходим
					return;
				}
			}
			// Создаем сокет подключения
			this->fd = ::socket(family, this->_type, this->_protocol);
			// Если сокет не создан то выходим
			if((this->fd == INVALID_SOCKET) || (this->fd >= MAX_SOCKETS)){
				// Выводим сообщение в консоль
				this->_log->print("Creating socket to server = %s, port = %u", log_t::flag_t::CRITICAL, ip.c_str(), port);
				// Выходим
				return;
			}
			/**
			 * Если операционной системой является Nix-подобная
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				// Выполняем игнорирование сигнала неверной инструкции процессора
				this->_socket.noSigILL();
				// Если сокет установлен TCP/IP
				if(this->_type == SOCK_STREAM)
					// Отключаем сигнал записи в оборванное подключение
					this->_socket.noSigPIPE(this->fd);
				// Если приложение является сервером
				if(type == type_t::SERVER){
					// Включаем отображение сети IPv4 в IPv6
					if(family == AF_INET6) this->_socket.onlyIPv6(this->fd, onlyV6);
				// Если приложение является клиентом и сокет установлен TCP/IP
				} else if(this->_type == SOCK_STREAM) {
					/**
					 * Если операционной системой является Linux или FreeBSD
					 */
					#if defined(__linux__) || defined(__FreeBSD__)
						// Если протокол интернета установлен как SCTP
						if(this->_protocol != IPPROTO_SCTP)
							// Активируем KeepAlive
							this->_socket.keepAlive(this->fd, this->alive.cnt, this->alive.idle, this->alive.intvl);
					/**
					 * Если операционной системой не является Linux
					 */
					#else
						// Активируем KeepAlive
						this->_socket.keepAlive(this->fd, this->alive.cnt, this->alive.idle, this->alive.intvl);
					#endif
				}
			/**
			 * Если операционной системой является MS Windows
			 */
			#else
				// Если приложение является сервером
				if(type == type_t::SERVER){
					// Включаем отображение сети IPv4 в IPv6
					if(family == AF_INET6) this->_socket.onlyIPv6(this->fd, onlyV6);
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
					this->_socket.setBlocking(this->fd, socket_t::mode_t::NOBLOCK);
				/**
				 * Если операционной системой является Linux или FreeBSD
				 */
				#if defined(__linux__) || defined(__FreeBSD__)
					// Если протокол интернета установлен как SCTP
					if(this->_protocol != IPPROTO_SCTP){
						// Отключаем алгоритм Нейгла для сервера и клиента
						this->_socket.nodelayTCP(this->fd);
						// Отключаем алгоритм Нейгла
						BIO_set_tcp_ndelay(this->fd, 1);
					}
				/**
				 * Если операционной системой не является Linux
				 */
				#else
					// Отключаем алгоритм Нейгла для сервера и клиента
					this->_socket.nodelayTCP(this->fd);
					// Отключаем алгоритм Нейгла
					BIO_set_tcp_ndelay(this->fd, 1);
				#endif
				// Устанавливаем разрешение на закрытие сокета при неиспользовании
				// this->_socket.closeonexec(this->fd);
			}
			// Устанавливаем разрешение на повторное использование сокета
			this->_socket.reuseable(this->fd);
			// Определяем тип запускаемого приложения
			switch(static_cast <uint8_t> (type)){
				// Если приложение является сервером
				case static_cast <uint8_t> (type_t::SERVER): {
					// Выполняем бинд на сокет
					if(::bind(this->fd, reinterpret_cast <struct sockaddr *> (&this->_peer.server), this->_peer.size) < 0)
						// Выводим в лог сообщение
						this->_log->print("Bind local network [%s]", log_t::flag_t::CRITICAL, host.c_str());
				} break;
				// Если приложение является клиентом
				case static_cast <uint8_t> (type_t::CLIENT): {
					// Выполняем бинд на сокет
					if(::bind(this->fd, reinterpret_cast <struct sockaddr *> (&this->_peer.client), this->_peer.size) < 0)
						// Выводим в лог сообщение
						this->_log->print("Bind local network [%s]", log_t::flag_t::CRITICAL, host.c_str());
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
	this->clear();
}
/**
 * error Метод вывода информации об ошибке
 * @param status статус ошибки
 */
void awh::Engine::Context::error(const int status) const noexcept {
	// Если защищённый режим работы разрешён
	if(this->_encrypted){
		// Получаем данные описание ошибки
		const int error = SSL_get_error(this->_ssl, status);
		// Определяем тип ошибки
		switch(error){
			// Если сертификат неизвестный
			case SSL_ERROR_SSL:
				// Выводим в лог сообщение
				this->_log->print("%s", log_t::flag_t::CRITICAL, ERR_error_string(error, nullptr));
			break;
			// Если был возвращён ноль
			case SSL_ERROR_ZERO_RETURN: {
				// Если удалённая сторона произвела закрытие подключения
				if(SSL_get_shutdown(this->_ssl) & SSL_RECEIVED_SHUTDOWN)
					// Выводим в лог сообщение
					this->_log->print("Remote side closed the connection", log_t::flag_t::INFO);
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
				} else if((status == -1) && (errno != 0))
					// Выводим в лог сообщение
					this->_log->print("%s", log_t::flag_t::WARNING, strerror(errno));
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
			// Если ошибка не обнаружена, выходим
			case 0: break;
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
	// Если сокет активен
	if(this->_addr != nullptr)
		// Выполняем закрытие подключения
		this->_addr->clear();
	// Сбрасываем флаг вывода подробной информации
	this->_verb = false;
	// Зануляем объект подключения
	this->_addr = nullptr;
	// Сбрасываем флаг инициализации
	this->_encrypted = false;
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
			this->_log->print("Major and minor version numbers must match, exiting", log_t::flag_t::CRITICAL);
			// Выходим из приложения
			exit(EXIT_FAILURE);
		}
	// Если всё хорошо, выводим версию OpenSSL
	} else this->_log->print("Using %s", log_t::flag_t::INFO, OpenSSL_version(OPENSSL_VERSION));
	// Если версия OpenSSL ниже версии 1.1.1b
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
			// X509_free(cert);
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
	if((buffer != nullptr) && (this->_addr != nullptr) && (size > 0) &&
	   (this->_type != type_t::NONE) && (this->_addr->fd != INVALID_SOCKET) && (this->_addr->fd < MAX_SOCKETS)){
		// Если защищённый режим работы разрешён
		if(this->_encrypted && (this->_ssl != nullptr)){
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
				switch(static_cast <uint8_t> (this->_addr->status)){
					// Если статус установлен как подключение клиентом
					case static_cast <uint8_t> (addr_t::status_t::CONNECTED):
						// Запоминаем полученную структуру
						addr = reinterpret_cast <struct sockaddr *> (&this->_addr->_peer.server);
					break;
					// Если статус установлен как разрешение подключения к серверу
					case static_cast <uint8_t> (addr_t::status_t::ACCEPTED):
						// Запоминаем полученную структуру
						addr = reinterpret_cast <struct sockaddr *> (&this->_addr->_peer.client);
					break;
				}
				// Метка повторного получения данных
				Read:
				// Выполняем чтение данных из сокета
				result = ::recvfrom(this->_addr->fd, buffer, size, 0, addr, &this->_addr->_peer.size);
				// Если нужно попытаться ещё раз получить сообщение
				if((result <= 0) && (errno == EAGAIN))
					// Повторяем попытку получить ещё раз
					goto Read;
			}
		}
		// Если данные прочитать не удалось
		if(result <= 0){
			// Получаем статус сокета
			const bool status = this->isblock();
			// Если сокет находится в блокирующем режиме
			if((result < 0) && status)
				// Выполняем обработку ошибок
				this->error(result);
			// Если произошла ошибка
			else if((result < 0) && !status) {
				// Если произошёл системный сигнал попробовать ещё раз
				if(errno == EINTR)
					// Пробуем повторно получить данные
					return -2;
				// Если защищённый режим работы разрешён
				if(this->_encrypted && (this->_ssl != nullptr)){
					// Получаем данные описание ошибки
					if(SSL_get_error(this->_ssl, result) == SSL_ERROR_WANT_READ)
						// Выполняем пропуск попытки
						return -1;
					// Иначе выводим сообщение об ошибке
					else this->error(result);
				// Если защищённый режим работы запрещён
				} else if(errno == EAGAIN)
					// Выполняем пропуск попытки
					return -1;
				// Иначе просто закрываем подключение
				result = 0;
			}
			// Если подключение разорвано или сокет находится в блокирующем режиме
			if((result == 0) || status)
				// Выполняем отключение от сервера
				result = 0;
			// Если произошло отключение
			if(result == 0)
				// Устанавливаем статус отключён
				this->_addr->status = addr_t::status_t::DISCONNECTED;
		// Если данные получены удачно
		} else {
			/**
			 * Если операционной системой является Linux или FreeBSD и включён режим отладки
			 */
			#if (defined(__linux__) || defined(__FreeBSD__)) && defined(DEBUG_MODE)
				// Если протокол интернета установлен как SCTP
				if((this->_addr->_protocol == IPPROTO_SCTP) && (SSL_get_error(this->_ssl, result) == SSL_ERROR_NONE)){
					// Создаём объект получения информационных событий
					struct bio_dgram_sctp_rcvinfo info;
					// Выполняем зануление объекта информационного события
					::memset(&info, 0, sizeof(info));
					// Выполняем извлечение события
					BIO_ctrl(this->_bio, BIO_CTRL_DGRAM_SCTP_GET_RCVINFO, sizeof(info), &info);
					// Выводим в лог информационное сообщение
					this->_log->print("Read %d bytes, stream: %u, ssn: %u, ppid: %u, tsn: %u", log_t::flag_t::INFO, static_cast <int> (result), info.rcv_sid, info.rcv_ssn, info.rcv_ppid, info.rcv_tsn);
				}
			#endif
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
	if((buffer != nullptr) && (this->_addr != nullptr) && (size > 0) &&
	   (this->_type != type_t::NONE) && (this->_addr->fd != INVALID_SOCKET) && (this->_addr->fd < MAX_SOCKETS)){
		// Если защищённый режим работы разрешён
		if(this->_encrypted && (this->_ssl != nullptr)){
			// Выполняем очистку ошибок OpenSSL
			ERR_clear_error();
			// Если подключение ещё активно
			if(!(SSL_get_shutdown(this->_ssl) & SSL_RECEIVED_SHUTDOWN)){
				// Если подключение выполнено
				if((result = ((this->_type == type_t::SERVER) ? SSL_accept(this->_ssl) : SSL_connect(this->_ssl))) > 0){
					/**
					 * Если операционной системой является Linux или FreeBSD
					 */
					#if defined(__linux__) || defined(__FreeBSD__)
						// Если протокол интернета установлен как SCTP
						if((this->_addr->_protocol == IPPROTO_SCTP) && (this->_addr->status == addr_t::status_t::CONNECTED)){
							// Создаём объект получения информационных событий
							struct bio_dgram_sctp_sndinfo info;
							// Выполняем зануление объекта информационного события
							::memset(&info, 0, sizeof(info));
							// Выполняем установку события
							BIO_ctrl(this->_bio, BIO_CTRL_DGRAM_SCTP_SET_SNDINFO, sizeof(info), &info);
						}
					#endif
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
				switch(static_cast <uint8_t> (this->_addr->status)){
					// Если статус установлен как подключение клиентом
					case static_cast <uint8_t> (addr_t::status_t::CONNECTED): {
						// Запоминаем полученную структуру
						addr = reinterpret_cast <struct sockaddr *> (&this->_addr->_peer.server);
						/**
						 * Если операционной системой не является Windows
						 */
						#if !defined(_WIN32) && !defined(_WIN64)
							// Для протокола unix-сокета
							if(this->_addr->_peer.server.ss_family == AF_UNIX)
								// Получаем размер объекта сокета
								this->_addr->_peer.size = (
									offsetof(struct sockaddr_un, sun_path) +
									strlen(reinterpret_cast <struct sockaddr_un *> (&this->_addr->_peer.server)->sun_path)
								);
						#endif
					} break;
					// Если статус установлен как разрешение подключения к серверу
					case static_cast <uint8_t> (addr_t::status_t::ACCEPTED): {
						// Запоминаем полученную структуру
						addr = reinterpret_cast <struct sockaddr *> (&this->_addr->_peer.client);
						/**
						 * Если операционной системой не является Windows
						 */
						#if !defined(_WIN32) && !defined(_WIN64)
							// Для протокола unix-сокета
							if(this->_addr->_peer.client.ss_family == AF_UNIX)
								// Получаем размер объекта сокета
								this->_addr->_peer.size = (
									offsetof(struct sockaddr_un, sun_path) +
									strlen(reinterpret_cast <struct sockaddr_un *> (&this->_addr->_peer.client)->sun_path)
								);
						#endif
					} break;
				}
				// Метка отправки данных
				Send:
				// Выполняем запись данных в сокет
				result = ::sendto(this->_addr->fd, buffer, size, 0, addr, this->_addr->_peer.size);
				// Если нужно попытаться ещё раз отправить сообщение
				if((result <= 0) && (errno == EAGAIN))
					// Повторяем попытку отправить ещё раз
					goto Send;
			}
		}
		// Если данные записать не удалось
		if(result <= 0){
			// Получаем статус сокета
			const bool status = this->isblock();
			// Если сокет находится в блокирующем режиме
			if((result < 0) && status)
				// Выполняем обработку ошибок
				this->error(result);
			// Если произошла ошибка
			else if((result < 0) && !status) {
				// Если произошёл системный сигнал попробовать ещё раз
				if(errno == EINTR)
					// Пробуем повторно получить данные
					return -2;
				// Если защищённый режим работы разрешён
				if(this->_encrypted){
					// Получаем данные описание ошибки
					if(SSL_get_error(this->_ssl, result) == SSL_ERROR_WANT_WRITE)
						// Выполняем пропуск попытки
						return -1;
					// Иначе выводим сообщение об ошибке
					else this->error(result);
				// Если защищённый режим работы запрещён
				} else if(errno == EAGAIN)
					// Выполняем пропуск попытки
					return -1;
				// Иначе просто закрываем подключение
				result = 0;
			}
			// Если подключение разорвано или сокет находится в блокирующем режиме
			if((result == 0) || status)
				// Выполняем отключение от сервера
				result = 0;
			// Если произошло отключение
			if(result == 0)
				// Устанавливаем статус отключён
				this->_addr->status = addr_t::status_t::DISCONNECTED;
		// Если данные отправлены удачно
		} else {
			/**
			 * Если операционной системой является Linux или FreeBSD и включён режим отладки
			 */
			#if (defined(__linux__) || defined(__FreeBSD__)) && defined(DEBUG_MODE)
				// Если протокол интернета установлен как SCTP
				if((this->_addr->_protocol == IPPROTO_SCTP) && (SSL_get_error(this->_ssl, result) == SSL_ERROR_NONE)){
					// Определяем тип подключения
					switch(static_cast <uint8_t> (this->_addr->status)){
						// Если статус установлен как подключение клиентом
						case static_cast <uint8_t> (addr_t::status_t::CONNECTED): {
							// Создаём объект получения информационных событий
							struct bio_dgram_sctp_sndinfo info;
							// Выполняем зануление объекта информационного события
							::memset(&info, 0, sizeof(info));
							// Выполняем извлечение события
							BIO_ctrl(this->_bio, BIO_CTRL_DGRAM_SCTP_GET_SNDINFO, sizeof(info), &info);
							// Выводим в лог информационное сообщение
							this->_log->print("Wrote %d bytes, stream: %u, ppid: %u", log_t::flag_t::INFO, static_cast <int> (result), info.snd_sid, info.snd_ppid);
						} break;
						// Если статус установлен как разрешение подключения к серверу
						case static_cast <uint8_t> (addr_t::status_t::ACCEPTED): {
							// Создаём объект получения информационных событий
							struct bio_dgram_sctp_rcvinfo info;
							// Выполняем зануление объекта информационного события
							::memset(&info, 0, sizeof(info));
							// Выполняем извлечение события
							BIO_ctrl(this->_bio, BIO_CTRL_DGRAM_SCTP_GET_SNDINFO, sizeof(info), &info);
							// Выводим в лог информационное сообщение
							this->_log->print("Wrote %d bytes, stream: %u, ssn: %u, ppid: %u, tsn: %u", log_t::flag_t::INFO, static_cast <int> (result), info.rcv_sid, info.rcv_ssn, info.rcv_ppid, info.rcv_tsn);
						} break;
					}
				}
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * block Метод установки блокирующего сокета
 * @return результат работы функции
 */
bool awh::Engine::Context::block() noexcept {
	// Если адрес присвоен
	if(this->_addr != nullptr){
		// Если защищённый режим работы разрешён
		if((this->_addr->fd != INVALID_SOCKET) && (this->_addr->fd < MAX_SOCKETS)){
			// Переводим сокет в блокирующий режим
			this->_addr->_async = !this->_addr->_socket.setBlocking(this->_addr->fd, socket_t::mode_t::BLOCK);
			// Если шифрование включено
			if(this->_encrypted && (this->_ssl != nullptr)){
				// Устанавливаем блокирующий режим ввода/вывода для сокета
				BIO_set_nbio(this->_bio, 0);
				// Флаг необходимо установить только для неблокирующего сокета
				SSL_clear_mode(this->_ssl, SSL_MODE_ENABLE_PARTIAL_WRITE);
			}
		}
		// Выводим результат
		return !this->_addr->_async;
	}
	// Сообщаем что сокет в блокирующем режиме
	return true;
}
/**
 * noblock Метод установки неблокирующего сокета
 * @return результат работы функции
 */
bool awh::Engine::Context::noblock() noexcept {
	// Если адрес присвоен
	if(this->_addr != nullptr){
		// Если файловый дескриптор активен
		if((this->_addr->fd != INVALID_SOCKET) && (this->_addr->fd < MAX_SOCKETS)){
			// Переводим сокет в не блокирующий режим
			this->_addr->_async = this->_addr->_socket.setBlocking(this->_addr->fd, socket_t::mode_t::NOBLOCK);
			// Если шифрование включено
			if(this->_encrypted && (this->_ssl != nullptr)){
				// Устанавливаем неблокирующий режим ввода/вывода для сокета
				BIO_set_nbio(this->_bio, 1);
				// Флаг необходимо установить только для неблокирующего сокета
				SSL_set_mode(this->_ssl, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
			}
		}
		// Выводим результат
		return this->_addr->_async;
	}
	// Сообщаем что сокет в блокирующем режиме
	return false;
}
/**
 * isblock Метод проверки на то, является ли сокет заблокированным
 * @return результат работы функции
 */
bool awh::Engine::Context::isblock() noexcept {
	// Если адрес присвоен
	if(this->_addr != nullptr)
		// Выводим результат проверки
		return !this->_addr->_async;
	// Сообщаем что сокет в блокирующем режиме
	return true;
}
/**
 * proto Метод извлечения поддерживаемого протокола
 * @return поддерживаемый протокол подключения
 */
awh::Engine::proto_t awh::Engine::Context::proto() const noexcept {
	// Извлекаем поддерживаемый протокол
	return this->_proto;
}
/**
 * proto Метод установки поддерживаемого протокола
 * @param proto устанавливаемый протокол
 */
void awh::Engine::Context::proto(const proto_t proto) noexcept {
	// Выполняем установку поддерживаемого протокола
	this->_proto = proto;
}
/**
 * timeout Метод установки таймаута
 * @param msec   количество миллисекунд
 * @param method метод для установки таймаута
 * @return       результат установки таймаута
 */
bool awh::Engine::Context::timeout(const time_t msec, const method_t method) noexcept {
	// Если адрес присвоен
	if(this->_addr != nullptr){
		// Определяем тип метода
		switch(static_cast <uint8_t> (method)){
			// Если установлен метод чтения
			case static_cast <uint8_t> (method_t::READ):
				// Выполняем установку таймера на чтение данных из сокета
				return (
					(this->_addr->fd != INVALID_SOCKET) && (this->_addr->fd < MAX_SOCKETS) ?
					this->_addr->_socket.timeout(this->_addr->fd, msec, socket_t::mode_t::READ) : false
				);
			// Если установлен метод записи
			case static_cast <uint8_t> (method_t::WRITE):
				// Выполняем установку таймера на запись данных в сокет
				return (
					(this->_addr->fd != INVALID_SOCKET) && (this->_addr->fd < MAX_SOCKETS) ?
					this->_addr->_socket.timeout(this->_addr->fd, msec, socket_t::mode_t::WRITE) : false
				);
		}
	}
	// Сообщаем, что операция не выполнена
	return false;
}
/**
 * buffer Метод получения размеров буфера
 * @param method метод для выполнения операции с буфером
 * @return       размер буфера
 */
int awh::Engine::Context::buffer(const method_t method) const noexcept {
	// Результат работы функции
	int result = 0;
	// Если адрес присвоен
	if(this->_addr != nullptr){
		// Определяем метод для работы с буфером
		switch(static_cast <uint8_t> (method)){
			// Если метод чтения
			case static_cast <uint8_t> (method_t::READ):
				// Получаем размер буфера для чтения
				result = (
					(this->_addr->fd != INVALID_SOCKET) && (this->_addr->fd < MAX_SOCKETS) ?
					this->_addr->_socket.bufferSize(this->_addr->fd, socket_t::mode_t::READ) : 0
				);
			break;
			// Если метод записи
			case static_cast <uint8_t> (method_t::WRITE):
				// Получаем размер буфера для записи
				result = (
					(this->_addr->fd != INVALID_SOCKET) && (this->_addr->fd < MAX_SOCKETS) ?
					this->_addr->_socket.bufferSize(this->_addr->fd, socket_t::mode_t::WRITE) : 0
				);
			break;
		}
	}
	// Выводим результат
	return result;
}
/**
 * buffer Метод установки размеров буфера
 * @param read  размер буфера на чтение
 * @param write размер буфера на запись
 * @param total максимальное количество подключений
 * @return      результат работы функции
 */
bool awh::Engine::Context::buffer(const int read, const int write, const u_int total) noexcept {
	// Если адрес присвоен
	if(this->_addr != nullptr)
		// Если подключение выполнено
		return (
			(this->_addr->fd != INVALID_SOCKET) && (this->_addr->fd < MAX_SOCKETS) ?
			this->_addr->_socket.bufferSize(this->_addr->fd, read, total, socket_t::mode_t::READ) &&
			this->_addr->_socket.bufferSize(this->_addr->fd, write, total, socket_t::mode_t::WRITE) : false
		);
	// Иначе возвращаем неустановленный размер буфера
	return false;
}
/**
 * selectProto Метод выполнения выбора следующего протокола
 * @param out     буфер назначения
 * @param outSize размер буфера назначения
 * @param in      буфер входящих данных
 * @param inSize  размер буфера входящих данных
 * @param key     ключ копирования
 * @param keySize размер ключа для копирования
 * @return        результат переключения протокола
 */
bool awh::Engine::Context::selectProto(u_char ** out, u_char * outSize, const u_char * in, u_int inSize, const char * key, u_int keySize) const noexcept {
	// Выполняем перебор всех данных в входящем буфере
	for(u_int i = 0; (i + keySize) <= inSize; i += (u_int) (in[i] + 1)){
		// Если данные ключа скопированны удачно
		if(::memcmp(&in[i], key, keySize) == 0){
			// Выполняем установку размеров исходящего буфера
			(* outSize) = in[i];
			// Выполняем установку полученных данных в исходящий буфер
			(* out) = const_cast <u_char *> (&in[i + 1]);
			// Выходим из функции
			return true;
		}
	}
	// Выводим результат
	return false;
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
bool awh::Engine::rawEqual(const string & first, const string & second) const noexcept {
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
bool awh::Engine::rawNequal(const string & first, const string & second, const size_t max) const noexcept {
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
bool awh::Engine::hostmatch(const string & host, const string & patt) const noexcept {
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
			if(!this->rawEqual(p, h))
				// Выходим из функции
				return false;
		// Выходим
		} else return false;
		// Если диапазоны точки в шаблоне и хосте отличаются тогда выходим
		if(hostLabelEnd < pattLabelEnd)
			// Выходим из функции
			return false;
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
bool awh::Engine::certHostcheck(const string & host, const string & patt) const noexcept {
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
 * Если операционной системой является Linux или FreeBSD
 */
#if defined(__linux__) || defined(__FreeBSD__)
	/**
	 * notificationsSCTP Функция обработки нотификации SCTP
	 * @param bio    объект подключения BIO
	 * @param ctx    промежуточный передаваемый контекст
	 * @param buffer буфер передаваемых данных
	 */
	void awh::Engine::notificationsSCTP(BIO * bio, void * ctx, void * buffer) noexcept {
		// Если данные переданы
		if((bio != nullptr) && (ctx != nullptr) && (buffer != nullptr)){
			// Получаем объект модуля подключения
			ctx_t * context = reinterpret_cast <ctx_t *> (ctx);
			// Создаём объект событий SCTP
			union sctp_notification * snp = reinterpret_cast <union sctp_notification *> (buffer);
			// Определяем тип события
			switch(snp->sn_header.sn_type){
				// Если произошло событие изменения ассоциации
				case SCTP_ASSOC_CHANGE: {
					// Получаем ассоциацию
					struct sctp_assoc_change * sac = &snp->sn_assoc_change;
					// Выводим в лог информационное сообщение
					context->_log->print("Assoc_change: state = %hu, error = %hu, instr = %hu, outstr = %hu", log_t::flag_t::INFO, sac->sac_state, sac->sac_error, sac->sac_inbound_streams, sac->sac_outbound_streams);
				} break;
				// Если изменился адрес подключения клиента
				case SCTP_PEER_ADDR_CHANGE: {
					// Адрес интернет-подключения
					string ip = "";
					// Объект данных подключения
					char buffer[INET6_ADDRSTRLEN];
					// Выполняем зануление буфера данных
					::memset(buffer, 0, sizeof(buffer));
					// Создаём объединение адресов
					union {
						struct sockaddr_in s4;      // Объект IPv4
						struct sockaddr_in6 s6;     // Объект IPv6
						struct sockaddr_storage ss; // Объект хранилища
					} peer;
					// Получаем данные изменившегося адреса
					struct sctp_paddr_change * spc = &snp->sn_paddr_change;
					// Устанавливаем новое значение подключения
					peer.ss = spc->spc_aaddr;
					// Определяем семейство интернет-протокола
					switch(peer.ss.ss_family){
						// Если подключение производится по IPv4
						case AF_INET:
							// Получаем IP адрес IPv4
							ip = inet_ntop(AF_INET, &peer.s4.sin_addr, buffer, sizeof(buffer));
						break;
						// Если подключение производится по IPv6
						case AF_INET6:
							// Получаем IP адрес IPv6
							ip = inet_ntop(AF_INET6, &peer.s6.sin6_addr, buffer, sizeof(buffer));
						break;
					}
					// Выводим в лог информационное сообщение
					context->_log->print("Intf_change: ip = %s, state = %d, error = %d", log_t::flag_t::INFO, ip.c_str(), spc->spc_state, spc->spc_error);
				} break;
				// Если произошла ошибка удалённого подключения
				case SCTP_REMOTE_ERROR: {
					// Получаем данные ошибки удалённого подключения
					struct sctp_remote_error * sre = &snp->sn_remote_error;
					// Выводим в лог информационное сообщение
					context->_log->print("Remote_error: err = %hu, len = %hu", log_t::flag_t::INFO, ntohs(sre->sre_error), ntohs(sre->sre_length));
				} break;
				// Если произошло событие неудачной отправки
				case SCTP_SEND_FAILED: {
					// Получаем объект ошибки
					struct sctp_send_failed * ssf = &snp->sn_send_failed;
					// Выводим в лог информационное сообщение
					context->_log->print("Sendfailed: err = %d, len = %u", log_t::flag_t::INFO, ssf->ssf_error, ssf->ssf_length);
				} break;
				// Если произошло событие отключения подключения
				case SCTP_SHUTDOWN_EVENT:
					// Выводим в лог информационное сообщение
					context->_log->print("Shutdown event", log_t::flag_t::INFO);
				break;
				// Если произошло событие адаптации
				case SCTP_ADAPTATION_INDICATION:
					// Выводим в лог информационное сообщение
					context->_log->print("Adaptation event", log_t::flag_t::INFO);
				break;
				// Если произошло сообщение частичной передачи данных
				case SCTP_PARTIAL_DELIVERY_EVENT:
					// Выводим в лог информационное сообщение
					context->_log->print("Partial delivery", log_t::flag_t::INFO);
				break;
				/**
				 * Если требуется аутентификация
				 */
				#ifdef SCTP_AUTHENTICATION_EVENT
					// Если произошло событие аутентификации
					case SCTP_AUTHENTICATION_EVENT:
						// Выводим в лог информационное сообщение
						context->_log->print("Authentication event", log_t::flag_t::INFO);
					break;
				#endif
				/**
				 * Если требуется отображение сухих событий
				 */
				#ifdef SCTP_SENDER_DRY_EVENT
					// Отправитель прислал сухое событие
					case SCTP_SENDER_DRY_EVENT:
						// Выводим в лог информационное сообщение
						context->_log->print("Sender dry event", log_t::flag_t::INFO);
					break;
				#endif
				// Если произошло неизвестное событие
				default:
					// Выводим в лог информационное сообщение
					context->_log->print("Unknown type: %hu", log_t::flag_t::INFO, snp->sn_header.sn_type);
			}
		}
	}
#endif
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
		// X509_free(cert);
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
		::memset(buffer, 0, sizeof(buffer));
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
			switch(static_cast <uint8_t> (validate)){
				case static_cast <uint8_t> (engine_t::validate_t::MatchFound):           status = "MatchFound";           break;
				case static_cast <uint8_t> (engine_t::validate_t::MatchNotFound):        status = "MatchNotFound";        break;
				case static_cast <uint8_t> (engine_t::validate_t::NoSANPresent):         status = "NoSANPresent";         break;
				case static_cast <uint8_t> (engine_t::validate_t::MalformedCertificate): status = "MalformedCertificate"; break;
				case static_cast <uint8_t> (engine_t::validate_t::Error):                status = "Error";                break;
				default:                                                                 status = "WTF!";
			}
		}
		// Запрашиваем имя домена
		X509_NAME_oneline(X509_get_subject_name(cert), buffer, sizeof(buffer));
		// Очищаем выделенную память
		// X509_free(cert);
		// Если домен найден в записях сертификата (т.е. сертификат соответствует данному домену)
		if(validate == engine_t::validate_t::MatchFound){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим в лог сообщение
				verify->engine->_log->print("HTTPS server [%s] has this certificate, which looks good to me: %s", log_t::flag_t::INFO, verify->host.c_str(), buffer);
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
 * OpenSSL собран без следующих переговорщиков по протоколам
 */
#ifndef OPENSSL_NO_NEXTPROTONEG
	/**
	 * nextProto Функция обратного вызова сервера для переключения на следующий протокол
	 * @param ssl  объект SSL
	 * @param data данные буфера данных протокола
	 * @param len  размер буфера данных протокола
	 * @param ctx  передаваемый контекст
	 * @return     результат переключения протокола
	 */
	int awh::Engine::nextProto(SSL * ssl, const u_char ** data, u_int * len, void * ctx) noexcept {
		// Если объекты переданы верно
		if((ssl != nullptr) && (ctx != nullptr)){
			// Блокируем неиспользуемую переменную
			(void) ssl;
			// Выполняем установку буфера данных
			(* data) = reinterpret_cast <ctx_t *> (ctx)->protocols.data();
			// Выполняем установку размер буфера данных протокола
			(* len) = static_cast <u_int> (reinterpret_cast <ctx_t *> (ctx)->protocols.size());
			// Выводим результат
			return SSL_TLSEXT_ERR_OK;
		}
		// Выводим результат
		return SSL_TLSEXT_ERR_NOACK;
	}
	/**
	 * selectNextProtoClient Функция обратного вызова клиента для расширения NPN TLS. Выполняется проверка, что сервер объявил протокол HTTP/2, который поддерживает библиотека nghttp2.
	 * @param ssl     объект SSL
	 * @param out     буфер исходящего протокола
	 * @param outSize размер буфера исходящего протокола
	 * @param in      буфер входящего протокола
	 * @param inSize  размер буфера входящего протокола
	 * @param ctx     передаваемый контекст
	 * @return        результат выбора протокола
	 */
	int awh::Engine::selectNextProtoClient(SSL * ssl, u_char ** out, u_char * outSize, const u_char * in, u_int inSize, void * ctx) noexcept {
		// Если объекты переданы верно
		if((ssl != nullptr) && (ctx != nullptr)){
			// Блокируем неиспользуемую переменную
			(void) ssl;
			// Получаем объект контекста модуля
			ctx_t * context = reinterpret_cast <ctx_t *> (ctx);
			// Если протокол переключить получилось на HTTP/2
			if(context->selectProto(out, outSize, in, inSize, "\x2h2", 2))
				// Выводим результат
				return SSL_TLSEXT_ERR_OK;
			// Если протокол переключить не получилось
			else {
				// Выполняем переключение протокола обратно на HTTP/1.1
				context->selectProto(out, outSize, in, inSize, "\x8http/1.1", 8);
				// Выполняем переключение протокола на HTTP/1.1
				context->_proto = proto_t::HTTP1_1;
			}
			
		}
		// Выводим результат
		return SSL_TLSEXT_ERR_NOACK;
	}
#endif // !OPENSSL_NO_NEXTPROTONEG
/**
 * Если версия OpenSSL соответствует или выше версии 1.0.2
 */
#if OPENSSL_VERSION_NUMBER >= 0x10002000L
	/**
	 * selectNextProtoServer Функция обратного вызова сервера для расширения NPN TLS. Выполняется проверка, что сервер объявил протокол HTTP/2, который поддерживает библиотека nghttp2.
	 * @param ssl     объект SSL
	 * @param out     буфер исходящего протокола
	 * @param outSize размер буфера исходящего протокола
	 * @param in      буфер входящего протокола
	 * @param inSize  размер буфера входящего протокола
	 * @param ctx     передаваемый контекст
	 * @return        результат выбора протокола
	 */
	int awh::Engine::selectNextProtoServer(SSL * ssl, const u_char ** out, u_char * outSize, const u_char * in, u_int inSize, void * ctx) noexcept {
		// Если объекты переданы верно
		if((ssl != nullptr) && (ctx != nullptr)){
			// Блокируем неиспользуемую переменную
			(void) ssl;
			// Получаем объект контекста модуля
			ctx_t * context = reinterpret_cast <ctx_t *> (ctx);
			// Если протокол переключить получилось на HTTP/2
			if(context->selectProto(const_cast <u_char **> (out), outSize, in, inSize, "\x2h2", 2))
				// Выводим результат
				return SSL_TLSEXT_ERR_OK;
			// Если протокол переключить не получилось
			else {
				// Выполняем переключение протокола обратно на HTTP/1.1
				context->selectProto(const_cast <u_char **> (out), outSize, in, inSize, "\x8http/1.1", 8);
				// Выполняем переключение протокола на HTTP/1.1
				context->_proto = proto_t::HTTP1_1;
			}
		}
		// Выводим результат
		return SSL_TLSEXT_ERR_NOACK;
	}
#endif // OPENSSL_VERSION_NUMBER >= 0x10002000L
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
			log_t(&fmk).print("Setting random cookie secret", log_t::flag_t::CRITICAL);
			// Выходим и сообщаем, что генерация куков не удалась
			return 0;
		}
	}
	// Выполняем чтение из подключения информации
	BIO_dgram_get_peer(SSL_get_rbio(ssl), &peer);
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
	u_char * buffer = reinterpret_cast <u_char *> (OPENSSL_malloc(offset));
	// Если память для буфера данных не выделена
	if(buffer == nullptr){
		// Создаём объект фреймворка
		fmk_t fmk;
		// Выводим в лог сообщение
		log_t(&fmk).print("Out of memory cookie", log_t::flag_t::CRITICAL);
		// Выходим и сообщаем, что генерация куков не удалась
		return 0;
	}
	// Выполняем определение протокола интернета
	switch(peer.ss.ss_family){
		// Для протокола IPv4
		case AF_INET: {
			// Выполняем чтение в буфер данных данные порта
			::memcpy(buffer, &peer.s4.sin_port, sizeof(u_short));
			// Выполняем чтение в буфер данных данные структуры подключения
			::memcpy(buffer + sizeof(peer.s4.sin_port), &peer.s4.sin_addr, sizeof(struct in_addr));
		} break;
		// Для протокола IPv6
		case AF_INET6: {
			// Выполняем чтение в буфер данных данные порта
			::memcpy(buffer, &peer.s6.sin6_port, sizeof(u_short));
			// Выполняем чтение в буфер данных данные структуры подключения
			::memcpy(buffer + sizeof(u_short), &peer.s6.sin6_addr, sizeof(struct in6_addr));
		} break;
		// Если производится работа с другими протоколами, выходим
		default: OPENSSL_assert(0);
	}
	// Выполняем расчёт HMAC в буфере, с использованием секретного ключа
	HMAC(EVP_sha1(), reinterpret_cast <void *> (_cookies), sizeof(_cookies), buffer, offset, result, &length);
	// Очищаем ранее выделенную память
	OPENSSL_free(buffer);
	// Выполняем копирование полученного результата в буфер печенок
	::memcpy(cookie, result, length);
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
	if(!_cookieInit)
		// Выходим из функции
		return 0;
	// Выполняем чтение из подключения информации
	BIO_dgram_get_peer(SSL_get_rbio(ssl), &peer);
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
	u_char * buffer = reinterpret_cast <u_char *> (OPENSSL_malloc(offset));
	// Если память для буфера данных не выделена
	if(buffer == nullptr){
		// Создаём объект фреймворка
		fmk_t fmk;
		// Выводим в лог сообщение
		log_t(&fmk).print("Out of memory cookie", log_t::flag_t::CRITICAL);
		// Выходим и сообщаем, что генерация куков не удалась
		return 0;
	}
	// Выполняем определение протокола интернета
	switch(peer.ss.ss_family){
		// Для протокола IPv4
		case AF_INET: {
			// Выполняем чтение в буфер данных данные порта
			::memcpy(buffer, &peer.s4.sin_port, sizeof(u_short));
			// Выполняем чтение в буфер данных данные структуры подключения
			::memcpy(buffer + sizeof(u_short), &peer.s4.sin_addr, sizeof(struct in_addr));
		} break;
		// Для протокола IPv6
		case AF_INET6: {
			// Выполняем чтение в буфер данных данные порта
			::memcpy(buffer, &peer.s6.sin6_port, sizeof(u_short));
			// Выполняем чтение в буфер данных данные структуры подключения
			::memcpy(buffer + sizeof(u_short), &peer.s6.sin6_addr, sizeof(struct in6_addr));
		} break;
		// Если производится работа с другими протоколами, выходим
		default: OPENSSL_assert(0);
	}
	// Выполняем расчёт HMAC в буфере, с использованием секретного ключа
	HMAC(EVP_sha1(), reinterpret_cast <void *> (_cookies), sizeof(_cookies), buffer, offset, result, &length);
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
	return verifyCookie(ssl, cookie, static_cast <u_int> (size));
}
/**
 * matchesCommonName Метод проверки доменного имени по данным из сертификата
 * @param host доменное имя
 * @param cert сертификат
 * @return     результат проверки
 */
awh::Engine::validate_t awh::Engine::matchesCommonName(const string & host, const X509 * cert) const noexcept {
	// Результат работы функции
	validate_t result = validate_t::MatchNotFound;
	// Если данные переданы
	if(!host.empty() && (cert != nullptr)){
		// Получаем индекс имени по NID
		const int cnl = X509_NAME_get_index_by_NID(X509_get_subject_name(const_cast <X509 *> (cert)), NID_commonName, -1);
		// Если индекс не получен тогда выходим
		if(cnl < 0)
			// Выводим сформированную ошибку
			return validate_t::Error;
		// Извлекаем поле CN
		X509_NAME_ENTRY * cne = X509_NAME_get_entry(X509_get_subject_name(const_cast <X509 *> (cert)), cnl);
		// Если поле не получено тогда выходим
		if(cne == nullptr)
			// Выводим сформированную ошибку
			return validate_t::Error;
		// Конвертируем CN поле в C строку
		ASN1_STRING * cna = X509_NAME_ENTRY_get_data(cne);
		// Если строка не сконвертирована тогда выходим
		if(cna == nullptr)
			// Выводим сформированную ошибку
			return validate_t::Error;
		// Извлекаем название в виде строки
		const string cn(reinterpret_cast <char *> (const_cast <u_char *> (ASN1_STRING_get0_data(cna))), ASN1_STRING_length(cna));
		// Сравниваем размеры полученных строк
		if((size_t) ASN1_STRING_length(cna) != cn.length())
			// Выводим сформированную ошибку
			return validate_t::MalformedCertificate;
		// Выполняем рукопожатие
		if(this->certHostcheck(host, cn))
			// Выводим сформированную ошибку
			return validate_t::MatchFound;
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
awh::Engine::validate_t awh::Engine::matchSubjectName(const string & host, const X509 * cert) const noexcept {
	// Результат работы функции
	validate_t result = validate_t::MatchNotFound;
	// Если данные переданы
	if(!host.empty() && (cert != nullptr)){
		// Получаем имена
		STACK_OF(GENERAL_NAME) * sn = reinterpret_cast <STACK_OF(GENERAL_NAME) *> (X509_get_ext_d2i(const_cast <X509 *> (cert), NID_subject_alt_name, nullptr, nullptr));
		// Если имена не получены тогда выходим
		if(sn == nullptr)
			// Выполняем формирвоание ошибки
			return validate_t::NoSANPresent;
		// Получаем количество имен
		const int sanNamesNb = sk_GENERAL_NAME_num(sn);
		// Переходим по всему списку
		for(int i = 0; i < sanNamesNb; i++){
			// Получаем имя из списка
			const GENERAL_NAME * cn = sk_GENERAL_NAME_value(sn, i);
			// Проверяем тип имени
			if(cn->type == GEN_DNS){
				// Получаем dns имя
				const string dns(reinterpret_cast <char *> (const_cast <u_char *> (ASN1_STRING_get0_data(cn->d.dNSName))), ASN1_STRING_length(cn->d.dNSName));
				// Если размер имени не совпадает
				if((size_t) ASN1_STRING_length(cn->d.dNSName) != dns.length()){
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
awh::Engine::validate_t awh::Engine::validateHostname(const string & host, const X509 * cert) const noexcept {
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
		if(!this->_ca.empty() && (this->_fs.isFile(this->_ca) || this->_fs.isDir(this->_path))){
			// Определяем путь где хранятся сертификаты
			const char * path = (!this->_path.empty() ? this->_path.c_str() : nullptr);
			// Выполняем проверку
			if(SSL_CTX_load_verify_locations(ctx, this->_ca.c_str(), path) != 1){
				// Выводим в лог сообщение
				this->_log->print("SSL verify locations is not allow", log_t::flag_t::CRITICAL);
				// Выходим
				return result;
			}
			// Если каталог получен
			if(path != nullptr){
				// Получаем полный адрес
				const string & trustdir = this->_fs.realPath(this->_path);
				// Если адрес существует
				if(this->_fs.isDir(trustdir) && !this->_fs.isFile(this->_ca)){
					/**
					 * Если операционной системой является MS Windows
					 */
					#if defined(_WIN32) || defined(_WIN64)
						// Выполняем сплит адреса
						const auto & params = this->_uri->split(trustdir);
						// Если путь и хост получен
						if((params.count(uri_t::flag_t::HOST) > 0) && (params.count(uri_t::flag_t::PATH) > 0)){
							// Выполняем сплит адреса
							auto path = this->_uri->splitPath(params.at(uri_t::flag_t::PATH), FS_SEPARATOR);
							// Добавляем адрес файла в список
							path.push_back(this->_ca);
							// Формируем полный адарес файла
							string filename = this->_fmk->format("%s:%s", params.at(uri_t::flag_t::HOST).c_str(), this->_uri->joinPath(path, FS_SEPARATOR).c_str());
							// Выполняем проверку доверенного сертификата
							if(!filename.empty()){
								// Выполняем декодирование адреса файла
								filename = this->_uri->decode(filename);
								// Если адрес файла существует
								if((result = this->_fs.isFile(filename))){
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
							if((result = this->_fs.isFile(filename))){
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
				} else if((result = this->_fs.isFile(this->_ca)))
					// Выполняем проверку доверенного сертификата
					SSL_CTX_set_client_CA_list(ctx, SSL_load_client_CA_file(this->_ca.c_str()));
				// Выполняем очистку адреса доверенного сертификата
				else this->_ca.clear();
			// Если адрес файла существует
			} else if((result = this->_fs.isFile(this->_ca)))
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
							this->_log->print("Failed to open system certificate store", log_t::flag_t::CRITICAL);
							// Выходим
							return -1;
						}
						// Перебираем все сертификаты в системном сторе
						while((ctx = CertEnumCertificatesInStore(sys, ctx))){
							// Выполняем создание сертификата
							X509 * cert = X509_new();
							// Если сертификат не создан
							if(cert != nullptr){
								// Формируем результат ответа
								result = -1;
								// Выводим в лог сообщение
								this->_log->print("%s failed", log_t::flag_t::CRITICAL, "d2i_X509");
								// Выходим из цикла
								break;
							// Если сертификат создан
							} else if(cert != nullptr) {
								// Добавляем сертификат в стор
								X509_STORE_add_cert(store, d2i_X509(&cert, &(reinterpret_cast <u_char *> (ctx->pbCertEncoded)), ctx->cbCertEncoded));
								// Очищаем выделенную память
								X509_free(cert);
							}
						}
						// Закрываем системный стор
						CertCloseStore(sys, 0);
					}
					// Выводим сформированный результат
					return result;
				};
				// Проверяем существует ли путь
				if((addCertToStoreFn(store, "CA") < 0) || (addCertToStoreFn(store, "AuthRoot") < 0) || (addCertToStoreFn(store, "ROOT") < 0))
					// Выводим сформированный результат
					return result;
			#endif
			// Если стор не устанавливается, тогда выводим ошибку
			if(!(result = (X509_STORE_set_default_paths(store) == 1)))
				// Выводим в лог сообщение
				this->_log->print("Set default paths for x509 store is not allow", log_t::flag_t::CRITICAL);
		}
	}
	// Выводим результат
	return result;
}
/**
 * wait Метод ожидания рукопожатия
 * @param target контекст назначения
 * @return       результат проверки
 */
bool awh::Engine::wait(ctx_t & target) noexcept {
	// Определяем тип входящего сокета
	switch(target._addr->_type){
		// Если сокет установлен TCP/IP
		case SOCK_STREAM:
			// Выполняем ожидание подключения
			return (SSL_stateless(target._ssl) > 0);
		break;
		// Если сокет установлен UDP
		case SOCK_DGRAM: {
			// Выполняем зануление структуры подключения клиента
			::memset(&target._addr->_peer.client, 0, sizeof(struct sockaddr_storage));
			// Выполняем ожидание подключения
			return (DTLSv1_listen(target._ssl, reinterpret_cast <BIO_ADDR *> (&target._addr->_peer.client)) > 0);
		} break;
	}
	// Сообщаем, что ничего не обработано
	return false;
}
/**
 * encrypted Метод проверки на активацию режима шифрования
 * @param ctx  контекст подключения
 * @return     результат проверки
 */
bool awh::Engine::encrypted(ctx_t & ctx) const noexcept {
	// Выводим результат проверки
	return ctx._encrypted;
}
/**
 * encrypted Метод установки флага режима шифрования
 * @param mode флаг режима шифрования
 * @param ctx  контекст подключения
 */
void awh::Engine::encrypted(const bool mode, ctx_t & ctx) noexcept {
	// Устанавливаем флаг шифрования
	ctx._encrypted = mode;
}
/**
 * proto Метод извлечения активного протокола
 * @param target контекст назначения
 * @return       метод активного протокола
 */
awh::Engine::proto_t awh::Engine::proto(ctx_t & target) const noexcept {
	// Результат работы функции
	proto_t result = proto_t::NONE;
	// Если контекст SSL инициализирован
	if(target._ssl != nullptr){
		// Если подключение выполнено
		if(target._type == type_t::SERVER ? SSL_accept(target._ssl) : SSL_connect(target._ssl)){
			// Определяет желаемый активный протокол
			switch(static_cast <uint8_t> (target._proto)){
				// Если протокол соответствует сырому
				case static_cast <uint8_t> (proto_t::RAW):
				// Если протокол соответствует HTTP/1
				case static_cast <uint8_t> (proto_t::HTTP1):
				// Если протокол соответствует HTTP/1.1
				case static_cast <uint8_t> (proto_t::HTTP1_1):
					// Устанавливаем активный протокол
					result = target._proto;
				break;
				// Если протокол соответствует SPDY/1
				case static_cast <uint8_t> (proto_t::SPDY1):
				// Если протокол соответствует HTTP/2
				case static_cast <uint8_t> (proto_t::HTTP2):
				// Если протокол соответствует HTTP/3
				case static_cast <uint8_t> (proto_t::HTTP3): {
					// Размер строки протокола
					u_int size = 0;
					// Строка протокола для сравнения
					const u_char * alpn = nullptr;
					/**
					 * OpenSSL собран без следующих переговорщиков по протоколам
					 */
					#ifndef OPENSSL_NO_NEXTPROTONEG
						// Выполняем извлечение следующего протокола
						SSL_get0_next_proto_negotiated(target._ssl, &alpn, &size);
					#endif // !OPENSSL_NO_NEXTPROTONEG
					/**
					 * Если версия OpenSSL соответствует или выше версии 1.0.2
					 */
					#if OPENSSL_VERSION_NUMBER >= 0x10002000L
						// Если протокол не был извлечён
						if(alpn == nullptr)
							// Выполняем извлечение выбранного протокола
							SSL_get0_alpn_selected(target._ssl, &alpn, &size);
					#endif // OPENSSL_VERSION_NUMBER >= 0x10002000L
					// Определяет желаемый активный протокол
					switch(static_cast <uint8_t> (target._proto)){
						// Если протокол соответствует SPDY/1
						case static_cast <uint8_t> (proto_t::SPDY1): {
							// Если активный протокол не соответствует протоколу SPDY/1
							if((alpn == nullptr) || (size != 6) || (::memcmp("spdy/1", alpn, 6) != 0))
								// Устанавливаем активный протокол
								result = proto_t::HTTP1_1;
							// Устанавливаем протокол как есть
							else result = target._proto;
						} break;
						// Если протокол соответствует HTTP/2
						case static_cast <uint8_t> (proto_t::HTTP2): {
							// Если активный протокол не соответствует протоколу HTTP/2
							if((alpn == nullptr) || (size != 2) || (::memcmp("h2", alpn, 2) != 0))
								// Устанавливаем активный протокол
								result = proto_t::HTTP1_1;
							// Устанавливаем протокол как есть
							else result = target._proto;
						} break;
						// Если протокол соответствует HTTP/3
						case static_cast <uint8_t> (proto_t::HTTP3):
							// Если активный протокол не соответствует протоколу HTTP/3
							if((alpn == nullptr) || (size != 2) || (::memcmp("h3", alpn, 2) != 0))
								// Устанавливаем активный протокол
								result = proto_t::HTTP1_1;
							// Устанавливаем протокол как есть
							else result = target._proto;
						break;
					}
				} break;
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * httpUpgrade Метод активации протокола HTTP
 * @param target контекст назначения
 */
void awh::Engine::httpUpgrade(ctx_t & target) const noexcept {
	// Если список поддерживаемых протоколов не установлен
	if(target.protocols.empty()){
		// Создаём идентификатор протокола HTTP/2
		const string http2 = "h2";
		// Создаём идентификатор протокола HTTP/3
		const string http3 = "h3";
		// Создаём идентификатор протокола SPDY/1
		const string spdy1 = "spdy/1";
		// Создаём идентификатор протокола HTTP/1
		const string http1 = "http/1";
		// Создаём идентификатор протокола HTTP/1.1
		const string http1_1 = "http/1.1";
		// Если протоколом является HTTP, выполняем переключение на него
		switch(static_cast <uint8_t> (target._proto)){
			// Если протокол соответствует SPDY/1
			case static_cast <uint8_t> (proto_t::SPDY1): {
				// Устанавливаем количество символов для протокола SPDY/1
				target.protocols.push_back(static_cast <u_char> (spdy1.size()));
				// Устанавливаем идентификатор протокола SPDY/1
				target.protocols.insert(target.protocols.end(), spdy1.begin(), spdy1.end());
				// Устанавливаем количество символов для протокола HTTP/1
				target.protocols.push_back(static_cast <u_char> (http1.size()));
				// Устанавливаем идентификатор протокола HTTP/1
				target.protocols.insert(target.protocols.end(), http1.begin(), http1.end());
				// Устанавливаем количество символов для протокола HTTP/1.1
				target.protocols.push_back(static_cast <u_char> (http1_1.size()));
				// Устанавливаем идентификатор протокола HTTP/1.1
				target.protocols.insert(target.protocols.end(), http1_1.begin(), http1_1.end());
			} break;
			// Если протокол соответствует HTTP/1
			case static_cast <uint8_t> (proto_t::HTTP1): {
				// Устанавливаем количество символов для протокола HTTP/1
				target.protocols.push_back(static_cast <u_char> (http1.size()));
				// Устанавливаем идентификатор протокола HTTP/1
				target.protocols.insert(target.protocols.end(), http1.begin(), http1.end());
			} break;
			// Если протокол соответствует HTTP/2
			case static_cast <uint8_t> (proto_t::HTTP2): {
				// Устанавливаем количество символов для протокола HTTP/2
				target.protocols.push_back(static_cast <u_char> (http2.size()));
				// Устанавливаем идентификатор протокола HTTP/2
				target.protocols.insert(target.protocols.end(), http2.begin(), http2.end());
				// Устанавливаем количество символов для протокола SPDY/1
				target.protocols.push_back(static_cast <u_char> (spdy1.size()));
				// Устанавливаем идентификатор протокола SPDY/1
				target.protocols.insert(target.protocols.end(), spdy1.begin(), spdy1.end());
				// Устанавливаем количество символов для протокола HTTP/1
				target.protocols.push_back(static_cast <u_char> (http1.size()));
				// Устанавливаем идентификатор протокола HTTP/1
				target.protocols.insert(target.protocols.end(), http1.begin(), http1.end());
				// Устанавливаем количество символов для протокола HTTP/1.1
				target.protocols.push_back(static_cast <u_char> (http1_1.size()));
				// Устанавливаем идентификатор протокола HTTP/1.1
				target.protocols.insert(target.protocols.end(), http1_1.begin(), http1_1.end());
			} break;
			// Если протокол соответствует HTTP/3
			case static_cast <uint8_t> (proto_t::HTTP3): {
				// Устанавливаем количество символов для протокола HTTP/2
				target.protocols.push_back(static_cast <u_char> (http2.size()));
				// Устанавливаем идентификатор протокола HTTP/2
				target.protocols.insert(target.protocols.end(), http2.begin(), http2.end());
				// Устанавливаем количество символов для протокола HTTP/3
				target.protocols.push_back(static_cast <u_char> (http3.size()));
				// Устанавливаем идентификатор протокола HTTP/3
				target.protocols.insert(target.protocols.end(), http3.begin(), http3.end());
				// Устанавливаем количество символов для протокола SPDY/1
				target.protocols.push_back(static_cast <u_char> (spdy1.size()));
				// Устанавливаем идентификатор протокола SPDY/1
				target.protocols.insert(target.protocols.end(), spdy1.begin(), spdy1.end());
				// Устанавливаем количество символов для протокола HTTP/1
				target.protocols.push_back(static_cast <u_char> (http1.size()));
				// Устанавливаем идентификатор протокола HTTP/1
				target.protocols.insert(target.protocols.end(), http1.begin(), http1.end());
				// Устанавливаем количество символов для протокола HTTP/1.1
				target.protocols.push_back(static_cast <u_char> (http1_1.size()));
				// Устанавливаем идентификатор протокола HTTP/1.1
				target.protocols.insert(target.protocols.end(), http1_1.begin(), http1_1.end());
			} break;
			// Если протокол соответствует HTTP/1.1
			case static_cast <uint8_t> (proto_t::HTTP1_1): {
				// Устанавливаем количество символов для протокола HTTP/1
				target.protocols.push_back(static_cast <u_char> (http1.size()));
				// Устанавливаем идентификатор протокола HTTP/1
				target.protocols.insert(target.protocols.end(), http1.begin(), http1.end());
				// Устанавливаем количество символов для протокола HTTP/1.1
				target.protocols.push_back(static_cast <u_char> (http1_1.size()));
				// Устанавливаем идентификатор протокола HTTP/1.1
				target.protocols.insert(target.protocols.end(), http1_1.begin(), http1_1.end());
			} break;
		}
	}
	// Определяем тип приложения
	switch(static_cast <uint8_t> (target._type)){
		// Если приложение является клиентом
		case static_cast <uint8_t> (type_t::CLIENT): {
			/**
			 * OpenSSL собран без следующих переговорщиков по протоколам
			 */
			#ifndef OPENSSL_NO_NEXTPROTONEG
				// Устанавливаем функцию обратного вызова для переключения протокола на HTTP
				SSL_CTX_set_next_proto_select_cb(target._ctx, &engine_t::selectNextProtoClient, &target);
			#endif // !OPENSSL_NO_NEXTPROTONEG
			/**
			 * Если версия OpenSSL соответствует или выше версии 1.0.2
			 */
			#if OPENSSL_VERSION_NUMBER >= 0x10002000L
				// Выполняем установку доступных протоколов передачи данных
				SSL_CTX_set_alpn_protos(target._ctx, target.protocols.data(), static_cast <u_int> (target.protocols.size()));
			#endif // OPENSSL_VERSION_NUMBER >= 0x10002000L
		} break;
		// Если приложение является сервером
		case static_cast <uint8_t> (type_t::SERVER): {
			/**
			 * OpenSSL собран без следующих переговорщиков по протоколам
			 */
			#ifndef OPENSSL_NO_NEXTPROTONEG
				// Выполняем установку функцию обратного вызова при выборе следующего протокола
				SSL_CTX_set_next_protos_advertised_cb(target._ctx, &engine_t::nextProto, &target);
			#endif // !OPENSSL_NO_NEXTPROTONEG
			/**
			 * Если версия OpenSSL соответствует или выше версии 1.0.2
			 */
			#if OPENSSL_VERSION_NUMBER >= 0x10002000L
				// Устанавливаем функцию обратного вызова для переключения протокола на HTTP/2
				SSL_CTX_set_alpn_select_cb(target._ctx, &engine_t::selectNextProtoServer, &target);
			#endif // OPENSSL_VERSION_NUMBER >= 0x10002000L
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
		if((target._addr->fd != INVALID_SOCKET) && (target._addr->fd < MAX_SOCKETS) && target._encrypted){
			// Извлекаем BIO cthdthf
			target._bio = SSL_get_rbio(target._ssl);
			// Устанавливаем сокет клиента
			BIO_set_fd(target._bio, target._addr->fd, BIO_NOCLOSE);
			// Выполняем установку объекта подключения в BIO
			BIO_ctrl(target._bio, BIO_CTRL_DGRAM_SET_CONNECTED, 0, reinterpret_cast <struct sockaddr *> (&target._addr->_peer.client));
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
		if((target._addr->fd != INVALID_SOCKET) && (target._addr->fd < MAX_SOCKETS)){
			// Определяем тип сокета
			switch(target._addr->_type){
				// Если тип сокета - диграммы
				case SOCK_DGRAM: {
					// Определяем тип активного приложения
					switch(static_cast <uint8_t> (type)){
						// Если приложение является клиентом
						case static_cast <uint8_t> (type_t::CLIENT):
							// Получаем контекст OpenSSL
							target._ctx = SSL_CTX_new(DTLSv1_2_client_method());
						break;
						// Если приложение является сервером
						case static_cast <uint8_t> (type_t::SERVER):
							// Получаем контекст OpenSSL
							target._ctx = SSL_CTX_new(DTLSv1_2_server_method());
						break;
					}
				} break;
				// Если тип сокета - потоки
				case SOCK_STREAM: {
					// Определяем тип активного приложения
					switch(static_cast <uint8_t> (type)){
						// Если приложение является клиентом
						case static_cast <uint8_t> (type_t::CLIENT):
							// Получаем контекст OpenSSL
							target._ctx = SSL_CTX_new(TLSv1_2_client_method());
						break;
						// Если приложение является сервером
						case static_cast <uint8_t> (type_t::SERVER):
							// Получаем контекст OpenSSL
							target._ctx = SSL_CTX_new(TLSv1_2_server_method());
						break;
					}
				} break;
			}
			// Если контекст не создан
			if(target._ctx == nullptr){
				// Выводим в лог сообщение
				this->_log->print("Context SSL is not initialization: %s", log_t::flag_t::CRITICAL, ERR_error_string(ERR_get_error(), nullptr));
				// Выходим
				return;
			}
			// Устанавливаем опции запроса
			SSL_CTX_set_options(target._ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_COMPRESSION);
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
					this->_log->print("Set SSL ciphers: %s", log_t::flag_t::CRITICAL, ERR_error_string(ERR_get_error(), nullptr));
					// Выходим
					return;
				}
				// Если приложение является сервером
				if(type == type_t::SERVER)
					// Заставляем серверные алгоритмы шифрования использовать в приоритете
					SSL_CTX_set_options(target._ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_COMPRESSION | SSL_OP_CIPHER_SERVER_PREFERENCE | SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);
			}
			/**
			 * Если версия OpenSSL соответствует или выше версии 3.0.0
			 */
			#if OPENSSL_VERSION_NUMBER >= 0x30000000L
				// Выполняем установку кривых P-256, доступны также (P-384 и P-521)
				if(SSL_CTX_set1_curves_list(target._ctx, "P-256") != 1){
					// Выводим в лог сообщение
					this->_log->print("Set SSL curves list failed: %s", log_t::flag_t::CRITICAL, ERR_error_string(ERR_get_error(), nullptr));
					// Выходим
					return;
				}
			/**
			 * Если версия OpenSSL ниже версии 3.0.0
			 */
			#else 
				{
					// Выполняем создание объекта кривой P-256, доступны также (P-384 и P-521)
					EC_KEY * ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
					// Если кривые не получилось установить
					if(ecdh == nullptr){
						// Выводим в лог сообщение
						this->_log->print("Set new SSL curv name failed: %s", log_t::flag_t::CRITICAL, ERR_error_string(ERR_get_error(), nullptr));
						// Выходим
						return;
					}
					// Выполняем установку кривых P-256
					SSL_CTX_set_tmp_ecdh(target._ctx, ecdh);
					// Выполняем очистку объекта кривой
					EC_KEY_free(ecdh);
				}
			#endif
			// Если приложение является сервером
			if(type == type_t::SERVER){
				// Получаем идентификатор процесса
				const pid_t pid = getpid();
				// Если протоколом является HTTP, выполняем переключение на него
				switch(static_cast <uint8_t> (target._proto)){
					// Если протокол соответствует SPDY/1
					case static_cast <uint8_t> (proto_t::SPDY1):
					// Если протокол соответствует HTTP/2
					case static_cast <uint8_t> (proto_t::HTTP2):
					// Если протокол соответствует HTTP/3
					case static_cast <uint8_t> (proto_t::HTTP3):
						// Выполняем переключение протокола подключения
						this->httpUpgrade(target);
					break;
				}
				// Выполняем установку идентификатора сессии
				if(SSL_CTX_set_session_id_context(target._ctx, reinterpret_cast <const u_char *> (&pid), sizeof(pid)) < 1){
					// Очищаем созданный контекст
					target.clear();
					// Выводим в лог сообщение
					this->_log->print("Failed to set session ID", log_t::flag_t::CRITICAL);
					// Выходим
					return;
				}
			// Если приложение является клиентом
			} else {
				// Если протоколом является HTTP, выполняем переключение на него
				switch(static_cast <uint8_t> (target._proto)){
					// Если протокол соответствует SPDY/1
					case static_cast <uint8_t> (proto_t::SPDY1):
					// Если протокол соответствует HTTP/1
					case static_cast <uint8_t> (proto_t::HTTP1):
					// Если протокол соответствует HTTP/2
					case static_cast <uint8_t> (proto_t::HTTP2):
					// Если протокол соответствует HTTP/3
					case static_cast <uint8_t> (proto_t::HTTP3):
					// Если протокол соответствует HTTP/1.1
					case static_cast <uint8_t> (proto_t::HTTP1_1):
						// Выполняем переключение протокола подключения
						this->httpUpgrade(target);
					break;
				}
			}
			// Устанавливаем поддерживаемые кривые
			if(SSL_CTX_set_ecdh_auto(target._ctx, 1) < 1){
				// Очищаем созданный контекст
				target.clear();
				// Выводим в лог сообщение
				this->_log->print("Set SSL ECDH: %s", log_t::flag_t::CRITICAL, ERR_error_string(ERR_get_error(), nullptr));
				// Выходим
				return;
			}
			// Устанавливаем флаг режима автоматического повтора получения следующих данных
			SSL_CTX_set_mode(target._ctx, SSL_MODE_AUTO_RETRY);
			// Устанавливаем флаг очистки буферов на чтение и запись когда они не требуются
			SSL_CTX_set_mode(target._ctx, SSL_MODE_RELEASE_BUFFERS);
			// Если приложение является сервером
			if(type == type_t::SERVER)
				// Выполняем отключение SSL кеша
				SSL_CTX_set_session_cache_mode(target._ctx, SSL_SESS_CACHE_OFF);
			// Если цепочка сертификатов установлена
			if(!this->_chain.empty()){
				// Определяем тип активного приложения
				switch(static_cast <uint8_t> (type)){
					// Если приложение является клиентом
					case static_cast <uint8_t> (type_t::CLIENT):
						// Если цепочка сертификатов не установлена
						if(SSL_CTX_use_certificate_file(target._ctx, this->_chain.c_str(), SSL_FILETYPE_PEM) < 1){
							// Выводим в лог сообщение
							this->_log->print("Certificate cannot be set", log_t::flag_t::CRITICAL);
							// Очищаем созданный контекст
							target.clear();
							// Выходим
							return;
						}
					break;
					// Если приложение является сервером
					case static_cast <uint8_t> (type_t::SERVER):
						// Если цепочка сертификатов не установлена
						if(SSL_CTX_use_certificate_chain_file(target._ctx, this->_chain.c_str()) < 1){
							// Выводим в лог сообщение
							this->_log->print("Certificate cannot be set", log_t::flag_t::CRITICAL);
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
					this->_log->print("Private key cannot be set", log_t::flag_t::CRITICAL);
					// Очищаем созданный контекст
					target.clear();
					// Выходим
					return;
				}
				// Если приватный ключ недействителен
				if(SSL_CTX_check_private_key(target._ctx) < 1){
					// Выводим в лог сообщение
					this->_log->print("Private key is not valid", log_t::flag_t::CRITICAL);
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
			if(!(target._encrypted = (target._ssl != nullptr))){
				// Очищаем созданный контекст
				target.clear();
				// Выводим в лог сообщение
				this->_log->print("Could not create SSL/TLS session object: %s", log_t::flag_t::CRITICAL, ERR_error_string(ERR_get_error(), nullptr));
				// Выходим
				return;
			}
			// Устанавливаем флаг активации TLS
			target._addr->_encrypted = target._encrypted;
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
				switch(static_cast <uint8_t> (type)){
					// Если приложение является клиентом
					case static_cast <uint8_t> (type_t::CLIENT): {
						// Если тип сокета - диграммы
						if(target._addr->_type == SOCK_DGRAM)
							// Выполняем установку объекта подключения в BIO
							BIO_ctrl(target._bio, BIO_CTRL_DGRAM_SET_CONNECTED, 0, reinterpret_cast <struct sockaddr *> (&target._addr->_peer.server));
					} break;
					// Если приложение является сервером
					case static_cast <uint8_t> (type_t::SERVER): {
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
	if(!source._encrypted && (source._addr != nullptr))
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
		// Если тип сокетов установлен не как потоковые
		if(target._addr->_type != SOCK_STREAM)
			// Выходим из функции
			return;
		// Если объект фреймворка существует
		if((target._addr->fd != INVALID_SOCKET) && (target._addr->fd < MAX_SOCKETS) && !this->_privkey.empty() && !this->_chain.empty()){
			/**
			 * Если операционной системой является Linux или FreeBSD
			 */
			#if defined(__linux__) || defined(__FreeBSD__)
				// Определяем тип протокола подключения
				switch(target._addr->_protocol){
					// Если протокол подключения UDP
					case IPPROTO_UDP:
					// Если протокол подключения SCTP
					case IPPROTO_SCTP:
						// Получаем контекст OpenSSL
						target._ctx = SSL_CTX_new(DTLSv1_2_server_method());
					break;
					// Если протокол подключения TCP
					case IPPROTO_TCP:
						// Получаем контекст OpenSSL
						target._ctx = SSL_CTX_new(TLSv1_2_server_method());
					break;
				}
			/**
			 * Если операционная система не является Linux
			 */
			#else
				// Получаем контекст OpenSSL
				target._ctx = SSL_CTX_new(TLSv1_2_server_method());
			#endif
			// Если контекст не создан
			if(target._ctx == nullptr){
				// Выводим в лог сообщение
				this->_log->print("Context SSL is not initialization: %s", log_t::flag_t::CRITICAL, ERR_error_string(ERR_get_error(), nullptr));
				// Выходим
				return;
			}
			// Устанавливаем опции запроса
			SSL_CTX_set_options(target._ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_COMPRESSION | SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);
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
					this->_log->print("Set SSL ciphers: %s", log_t::flag_t::CRITICAL, ERR_error_string(ERR_get_error(), nullptr));
					// Выходим
					return;
				}
				// Заставляем серверные алгоритмы шифрования использовать в приоритете
				SSL_CTX_set_options(target._ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_COMPRESSION | SSL_OP_CIPHER_SERVER_PREFERENCE | SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);
			}
			// Получаем идентификатор процесса
			const pid_t pid = getpid();
			/**
			 * Если версия OpenSSL соответствует или выше версии 3.0.0
			 */
			#if OPENSSL_VERSION_NUMBER >= 0x30000000L
				// Выполняем установку кривых P-256, доступны также (P-384 и P-521)
				if(SSL_CTX_set1_curves_list(target._ctx, "P-256") != 1){
					// Выводим в лог сообщение
					this->_log->print("Set SSL curves list failed: %s", log_t::flag_t::CRITICAL, ERR_error_string(ERR_get_error(), nullptr));
					// Выходим
					return;
				}
			/**
			 * Если версия OpenSSL ниже версии 3.0.0
			 */
			#else 
				{
					// Выполняем создание объекта кривой P-256, доступны также (P-384 и P-521)
					EC_KEY * ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
					// Если кривые не получилось установить
					if(ecdh == nullptr){
						// Выводим в лог сообщение
						this->_log->print("Set new SSL curv name failed: %s", log_t::flag_t::CRITICAL, ERR_error_string(ERR_get_error(), nullptr));
						// Выходим
						return;
					}
					// Выполняем установку кривых P-256
					SSL_CTX_set_tmp_ecdh(target._ctx, ecdh);
					// Выполняем очистку объекта кривой
					EC_KEY_free(ecdh);
				}
			#endif
			// Если протоколом является HTTP, выполняем переключение на него
			switch(static_cast <uint8_t> (target._proto)){
				// Если протокол соответствует SPDY/1
				case static_cast <uint8_t> (proto_t::SPDY1):
				// Если протокол соответствует HTTP/2
				case static_cast <uint8_t> (proto_t::HTTP2):
				// Если протокол соответствует HTTP/3
				case static_cast <uint8_t> (proto_t::HTTP3):
					// Выполняем переключение протокола подключения
					this->httpUpgrade(target);
				break;
			}
			// Выполняем установку идентификатора сессии
			if(SSL_CTX_set_session_id_context(target._ctx, reinterpret_cast <const u_char *> (&pid), sizeof(pid)) < 1){
				// Очищаем созданный контекст
				target.clear();
				// Выводим в лог сообщение
				this->_log->print("Failed to set session ID", log_t::flag_t::CRITICAL);
				// Выходим
				return;
			}
			// Устанавливаем поддерживаемые кривые
			if(SSL_CTX_set_ecdh_auto(target._ctx, 1) < 1){
				// Очищаем созданный контекст
				target.clear();
				// Выводим в лог сообщение
				this->_log->print("Set SSL ECDH: %s", log_t::flag_t::CRITICAL, ERR_error_string(ERR_get_error(), nullptr));
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
			// Заставляем OpenSSL автоматические повторные попытки после событий сеанса TLS
			SSL_CTX_set_mode(target._ctx, SSL_MODE_AUTO_RETRY);
			// Устанавливаем флаг очистки буферов на чтение и запись когда они не требуются
			SSL_CTX_set_mode(target._ctx, SSL_MODE_RELEASE_BUFFERS);
			// Запускаем кэширование
			SSL_CTX_set_session_cache_mode(target._ctx, SSL_SESS_CACHE_SERVER | SSL_SESS_CACHE_NO_INTERNAL);
			// Если цепочка сертификатов установлена
			if(!this->_chain.empty()){
				// Если цепочка сертификатов не установлена
				if(SSL_CTX_use_certificate_chain_file(target._ctx, this->_chain.c_str()) < 1){
					// Выводим в лог сообщение
					this->_log->print("Certificate cannot be set", log_t::flag_t::CRITICAL);
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
					this->_log->print("Private key cannot be set", log_t::flag_t::CRITICAL);
					// Очищаем созданный контекст
					target.clear();
					// Выходим
					return;
				}
				// Если приватный ключ недействителен
				if(SSL_CTX_check_private_key(target._ctx) < 1){
					// Выводим в лог сообщение
					this->_log->print("Private key is not valid", log_t::flag_t::CRITICAL);
					// Очищаем созданный контекст
					target.clear();
					// Выходим
					return;
				}
			}
			// Если доверенный сертификат недействителен
			if(SSL_CTX_set_default_verify_file(target._ctx) < 1){
				// Выводим в лог сообщение
				this->_log->print("Trusted certificate is invalid", log_t::flag_t::CRITICAL);
				// Очищаем созданный контекст
				target.clear();
				// Выходим
				return;
			}
			// Если нужно произвести проверку
			if(this->_verify){
				// Устанавливаем глубину проверки
				SSL_CTX_set_verify_depth(target._ctx, 2);
				// Выполняем проверку сертификата клиента
				SSL_CTX_set_verify(target._ctx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, &verifyCert);
			// Запрещаем выполнять првоерку сертификата пользователя
			} else SSL_CTX_set_verify(target._ctx, SSL_VERIFY_NONE, nullptr);
			// Создаем SSL объект
			target._ssl = SSL_new(target._ctx);
			// Если объект не создан
			if(!(target._encrypted = (target._ssl != nullptr))){
				// Очищаем созданный контекст
				target.clear();
				// Выводим в лог сообщение
				this->_log->print("Could not create SSL/TLS session object: %s", log_t::flag_t::CRITICAL, ERR_error_string(ERR_get_error(), nullptr));
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
					this->_log->print("Certificate chain validation failed: %s", log_t::flag_t::CRITICAL, X509_verify_cert_error_string(verify));
					// Выходим
					return;
				}
			}
			// Устанавливаем флаг активации TLS
			target._addr->_encrypted = target._encrypted;
			/**
			 * Если операционной системой является Linux или FreeBSD
			 */
			#if defined(__linux__) || defined(__FreeBSD__)
				// Определяем тип протокола подключения
				switch(target._addr->_protocol){
					// Если протокол подключения UDP
					case IPPROTO_UDP:
						// Выполняем обёртывание сокета UDP в BIO SSL
						target._bio = BIO_new_dgram(target._addr->fd, BIO_NOCLOSE);
					break;
					// Если протокол подключения SCTP
					case IPPROTO_SCTP:
						// Выполняем обёртывание сокета в BIO SSL
						target._bio = BIO_new_dgram_sctp(target._addr->fd, BIO_NOCLOSE);
					break;
					// Если протокол подключения TCP
					case IPPROTO_TCP:
						// Выполняем обёртывание сокета в BIO SSL
						target._bio = BIO_new_socket(target._addr->fd, BIO_NOCLOSE);
					break;
				}
			/**
			 * Если операционная система не является Linux
			 */
			#else
				// Выполняем обёртывание сокета в BIO SSL
				target._bio = BIO_new_socket(target._addr->fd, BIO_NOCLOSE);
			#endif
			// Если BIO SSL создано
			if(target._bio != nullptr){
				// Устанавливаем неблокирующий режим ввода/вывода для сокета
				target.noblock();
				// Выполняем установку BIO SSL
				SSL_set_bio(target._ssl, target._bio, target._bio);
				/**
				 * Если операционной системой является Linux или FreeBSD и включён режим отладки
				 */
				#if (defined(__linux__) || defined(__FreeBSD__)) && defined(DEBUG_MODE)
					// Если протокол интернета установлен как SCTP
					if(target._addr->_protocol == IPPROTO_SCTP)
						// Устанавливаем функцию нотификации
						BIO_dgram_sctp_notification_cb(target._bio, &notificationsSCTP, &target);
				#endif
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
 * @param host   хост удалённого сервера
 * @return       объект SSL контекста
 */
void awh::Engine::wrapClient(ctx_t & target, ctx_t & source, const string & host) noexcept {
	// Если объект ещё не обёрнут в SSL контекст
	if((source._ssl == nullptr) && (source._addr != nullptr))
		// Выполняем обёртывание уже активного SSL контекста
		this->wrapClient(target, source._addr, host);
}
/**
 * wrapClient Метод обертывания файлового дескриптора для клиента
 * @param target  контекст назначения
 * @param address объект подключения
 * @param host    хост удалённого сервера
 * @return        объект SSL контекста
 */
void awh::Engine::wrapClient(ctx_t & target, addr_t * address, const string & host) noexcept {
	// Если данные переданы
	if((address != nullptr) && !host.empty()){
		// Устанавливаем файловый дескриптор
		target._addr = address;
		// Устанавливаем тип приложения
		target._type = type_t::CLIENT;
		// Если объект фреймворка существует
		if((target._addr->fd != INVALID_SOCKET) && (target._addr->fd < MAX_SOCKETS) &&
		  ((!this->_privkey.empty() && !this->_chain.empty()) || this->encrypted(target))){
			/**
			 * Если операционной системой является Linux или FreeBSD
			 */
			#if defined(__linux__) || defined(__FreeBSD__)
				// Определяем тип протокола подключения
				switch(target._addr->_protocol){
					// Если протокол подключения UDP
					case IPPROTO_UDP:
					// Если протокол подключения SCTP
					case IPPROTO_SCTP:
						// Получаем контекст OpenSSL
						target._ctx = SSL_CTX_new(DTLSv1_2_client_method());
					break;
					// Если протокол подключения TCP
					case IPPROTO_TCP:
						// Получаем контекст OpenSSL
						target._ctx = SSL_CTX_new(TLSv1_2_client_method());
					break;
				}
			/**
			 * Если операционная система не является Linux
			 */
			#else
				// Получаем контекст OpenSSL
				target._ctx = SSL_CTX_new(TLSv1_2_client_method());
			#endif
			// Если контекст не создан
			if(target._ctx == nullptr){
				// Выводим в лог сообщение
				this->_log->print("Context SSL is not initialization: %s", log_t::flag_t::CRITICAL, ERR_error_string(ERR_get_error(), nullptr));
				// Выходим
				return;
			}
			// Устанавливаем опции запроса
			SSL_CTX_set_options(target._ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_COMPRESSION);
			/**
			 * Если версия OpenSSL соответствует или выше версии 3.0.0
			 */
			#if OPENSSL_VERSION_NUMBER >= 0x30000000L
				// Выполняем установку кривых P-256, доступны также (P-384 и P-521)
				if(SSL_CTX_set1_curves_list(target._ctx, "P-256") != 1){
					// Выводим в лог сообщение
					this->_log->print("Set SSL curves list failed: %s", log_t::flag_t::CRITICAL, ERR_error_string(ERR_get_error(), nullptr));
					// Выходим
					return;
				}
			/**
			 * Если версия OpenSSL ниже версии 3.0.0
			 */
			#else 
				{
					// Выполняем создание объекта кривой P-256, доступны также (P-384 и P-521)
					EC_KEY * ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
					// Если кривые не получилось установить
					if(ecdh == nullptr){
						// Выводим в лог сообщение
						this->_log->print("Set new SSL curv name failed: %s", log_t::flag_t::CRITICAL, ERR_error_string(ERR_get_error(), nullptr));
						// Выходим
						return;
					}
					// Выполняем установку кривых P-256
					SSL_CTX_set_tmp_ecdh(target._ctx, ecdh);
					// Выполняем очистку объекта кривой
					EC_KEY_free(ecdh);
				}
			#endif
			// Если протоколом является HTTP, выполняем переключение на него
			switch(static_cast <uint8_t> (target._proto)){
				// Если протокол соответствует SPDY/1
				case static_cast <uint8_t> (proto_t::SPDY1):
				// Если протокол соответствует HTTP/1
				case static_cast <uint8_t> (proto_t::HTTP1):
				// Если протокол соответствует HTTP/2
				case static_cast <uint8_t> (proto_t::HTTP2):
				// Если протокол соответствует HTTP/3
				case static_cast <uint8_t> (proto_t::HTTP3):
				// Если протокол соответствует HTTP/1.1
				case static_cast <uint8_t> (proto_t::HTTP1_1):
					// Выполняем переключение протокола подключения
					this->httpUpgrade(target);
				break;
			}
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
					this->_log->print("Set SSL ciphers: %s", log_t::flag_t::CRITICAL, ERR_error_string(ERR_get_error(), nullptr));
					// Выходим
					return;
				}
			}
			// Заставляем OpenSSL автоматические повторные попытки после событий сеанса TLS
			SSL_CTX_set_mode(target._ctx, SSL_MODE_AUTO_RETRY);
			// Устанавливаем флаг очистки буферов на чтение и запись когда они не требуются
			SSL_CTX_set_mode(target._ctx, SSL_MODE_RELEASE_BUFFERS);
			// Если цепочка сертификатов установлена
			if(!this->_chain.empty()){
				// Если цепочка сертификатов не установлена
				if(SSL_CTX_use_certificate_file(target._ctx, this->_chain.c_str(), SSL_FILETYPE_PEM) < 1){
					// Выводим в лог сообщение
					this->_log->print("Certificate cannot be set", log_t::flag_t::CRITICAL);
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
					this->_log->print("Private key cannot be set", log_t::flag_t::CRITICAL);
					// Очищаем созданный контекст
					target.clear();
					// Выходим
					return;
				}
				// Если приватный ключ недействителен
				if(SSL_CTX_check_private_key(target._ctx) < 1){
					// Выводим в лог сообщение
					this->_log->print("Private key is not valid", log_t::flag_t::CRITICAL);
					// Очищаем созданный контекст
					target.clear();
					// Выходим
					return;
				}
			}
			// Если нужно произвести проверку
			if(this->_verify){
				// Создаём объект проверки домена
				target._verify = new verify_t(host, this);
				// Выполняем проверку сертификата
				SSL_CTX_set_verify(target._ctx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, nullptr);
				// Выполняем проверку всех дочерних сертификатов
				SSL_CTX_set_cert_verify_callback(target._ctx, &verifyHost, target._verify);
				// Устанавливаем глубину проверки
				SSL_CTX_set_verify_depth(target._ctx, 4);
			// Запрещаем выполнять првоерку сертификата пользователя
			} else SSL_CTX_set_verify(target._ctx, SSL_VERIFY_NONE, nullptr);
			// Устанавливаем, что мы должны читать как можно больше входных байтов
			SSL_CTX_set_read_ahead(target._ctx, 1);
			// Создаем SSL объект
			target._ssl = SSL_new(target._ctx);
			// Если объект не создан
			if(!(target._encrypted = (target._ssl != nullptr))){
				// Очищаем созданный контекст
				target.clear();
				// Выводим в лог сообщение
				this->_log->print("Could not create SSL/TLS session object: %s", log_t::flag_t::CRITICAL, ERR_error_string(ERR_get_error(), nullptr));
				// Выходим
				return;
			}
			/**
			 * Если нужно установить TLS расширение
			 */
			#ifdef SSL_CTRL_SET_TLSEXT_HOSTNAME
				// Устанавливаем имя хоста для SNI расширения
				SSL_set_tlsext_host_name(target._ssl, host.c_str());
			#endif
			// Активируем верификацию доменного имени
			if(X509_VERIFY_PARAM_set1_host(SSL_get0_param(target._ssl), host.c_str(), 0) < 1){
				// Очищаем созданный контекст
				target.clear();
				// Выводим в лог сообщение
				this->_log->print("Host SSL verification failed", log_t::flag_t::CRITICAL);
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
					this->_log->print("Certificate chain validation failed: %s", log_t::flag_t::CRITICAL, X509_verify_cert_error_string(verify));
				}
			}
			// Устанавливаем флаг активации TLS
			target._addr->_encrypted = target._encrypted;
			/**
			 * Если операционной системой является Linux или FreeBSD
			 */
			#if defined(__linux__) || defined(__FreeBSD__)
				// Определяем тип протокола подключения
				switch(target._addr->_protocol){
					// Если протокол подключения UDP
					case IPPROTO_UDP:
						// Выполняем обёртывание сокета UDP в BIO SSL
						target._bio = BIO_new_dgram(target._addr->fd, BIO_NOCLOSE);
					break;
					// Если протокол подключения SCTP
					case IPPROTO_SCTP:
						// Выполняем обёртывание сокета в BIO SSL
						target._bio = BIO_new_dgram_sctp(target._addr->fd, BIO_NOCLOSE);
					break;
					// Если протокол подключения TCP
					case IPPROTO_TCP:
						// Выполняем обёртывание сокета в BIO SSL
						target._bio = BIO_new_socket(target._addr->fd, BIO_NOCLOSE);
					break;
				}
			/**
			 * Если операционная система не является Linux
			 */
			#else
				// Выполняем обёртывание сокета в BIO SSL
				target._bio = BIO_new_socket(target._addr->fd, BIO_NOCLOSE);
			#endif
			// Если BIO SSL создано
			if(target._bio != nullptr){
				// Устанавливаем блокирующий режим ввода/вывода для сокета
				target.block();
				// Выполняем установку BIO SSL
				SSL_set_bio(target._ssl, target._bio, target._bio);
				/**
				 * Если операционной системой является Linux или FreeBSD и включён режим отладки
				 */
				#if (defined(__linux__) || defined(__FreeBSD__)) && defined(DEBUG_MODE)
					// Если протокол интернета установлен как SCTP
					if(target._addr->_protocol == IPPROTO_SCTP)
						// Устанавливаем функцию нотификации
						BIO_dgram_sctp_notification_cb(target._bio, &notificationsSCTP, &target);
				#endif
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
		this->_ca = this->_fs.realPath(trusted);
		// Если адрес каталога с доверенным сертификатом (CA-файлом) передан, устанавливаем и его
		if(!path.empty() && this->_fs.isDir(path))
			// Устанавливаем адрес каталога с доверенным сертификатом (CA-файлом)
			this->_path = this->_fs.realPath(path);
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
 _verify(true), _fs(fmk, log), _cipher(""), _chain(""), _privkey(""),
 _path(""), _ca(SSL_CA_FILE), _fmk(fmk), _uri(uri), _log(log) {
	// Выполняем модификацию доверенного сертификата (CA-файла)
	this->_ca = this->_fs.realPath(this->_ca);
	// Выполняем установку алгоритмов шифрования
	this->ciphers({
		"ECDHE+AESGCM",
		"ECDHE+CHACHA20",
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
		"DHE+AESGCM",
		"DHE+CHACHA20",
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
	 * Если версия OPENSSL ниже версии 1.1.0
	 */
	#if (OPENSSL_VERSION_NUMBER < 0x10100000L) || (defined(LIBRESSL_VERSION_NUMBER) && (LIBRESSL_VERSION_NUMBER < 0x20700000L))
		// Выполняем конфигурацию OpenSSL
		OPENSSL_config(nullptr);
		// Выполняем инициализацию OpenSSL
		SSL_library_init();
	/**
	 * Для более свежей версии
	 */
	#else
		// Выполняем инициализацию OpenSSL
		OPENSSL_init_ssl(OPENSSL_INIT_SSL_DEFAULT, nullptr);
	#endif
	// Выполняем загрузки описаний ошибок шифрования
	ERR_load_crypto_strings();
	// Выполняем загрузки описаний ошибок OpenSSL
	SSL_load_error_strings();
	// Добавляем все алгоритмы шифрования
	OpenSSL_add_all_algorithms();
	// Активируем рандомный генератор
	if(RAND_poll() < 1){
		// Выводим в лог сообщение
		this->_log->print("Rand poll is not allow", log_t::flag_t::CRITICAL);
		// Выходим из приложения
		exit(EXIT_FAILURE);
	}
}
/**
 * ~Engine Деструктор
 */
awh::Engine::~Engine() noexcept {
	/**
	 * Если версия OPENSSL ниже версии 1.1.0
	 */
	#if (OPENSSL_VERSION_NUMBER < 0x10100000L) || (defined(LIBRESSL_VERSION_NUMBER) && LIBRESSL_VERSION_NUMBER < 0x20700000L)
		// Выполняем освобождение памяти
		EVP_cleanup();
		ERR_free_strings();
		/**
		 * Если версия OPENSSL ниже версии 1.0.0
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
		// Выполняем освобождение памяти для методов компрессии
		sk_SSL_COMP_free(SSL_COMP_get_compression_methods());
	#endif
}
