/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/events.hpp>

// Подключаем пространство имён
using namespace std;
using namespace awh;

/**
 * Peer Структура подключения
 */
struct Peer {
	socklen_t size;                 // Размер объекта подключения
	struct sockaddr_storage client; // Параметры подключения клиента
	struct sockaddr_storage server; // Параметры подключения сервера
	/**
	 * Peer Конструктор
	 */
	Peer() noexcept : size(0), client{}, server{} {}
} peer;

int fd = -1;

static bool noSigILL() noexcept {
	// Результат работы функции
	bool result = false;
	// Создаем структуру активации сигнала
	struct sigaction act;
	// Зануляем структуру
	::memset(&act, 0, sizeof(act));
	// Устанавливаем макрос игнорирования сигнала
	act.sa_handler = SIG_IGN;
	// Устанавливаем флаги перезагрузки
	act.sa_flags = (SA_ONSTACK | SA_RESTART | SA_SIGINFO);
	// Устанавливаем блокировку сигнала
	if(!(result = !static_cast <bool> (::sigaction(SIGILL, &act, nullptr))))
		// Выводим в лог информацию
		printf("Warning: cannot set SIG_IGN on signal SIGILL [%s]", ::strerror(errno));
	// Все удачно
	return result;
}

static bool noSigPIPE(const int fd) noexcept {
	// Результат работы функции
	bool result = false;
	// Устанавливаем параметр
	const int on = 1;
	// Устанавливаем SO_NOSIGPIPE
	if(!(result = !static_cast <bool> (::setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on)))))
		// Выводим в лог информацию
		printf("Warning: cannot set SO_NOSIGPIPE option on SOCKET=%d [%s]", fd, ::strerror(errno));
	// Выводим результат
	return result;
}

static bool keepAlive(const int fd, const int cnt, const int idle, const int intvl) noexcept {
	// Результат работы функции
	bool result = false;
	// Устанавливаем параметр
	int keepAlive = 1;
	// Активация постоянного подключения
	if(!(result = !static_cast <bool> (::setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(keepAlive))))){
		// Выводим в лог информацию
		printf("Warning: cannot set SO_KEEPALIVE option on SOCKET=%d [%s]", fd, ::strerror(errno));
		// Выходим из функции
		return result;
	}
	// Максимальное количество попыток
	if(!(result = !static_cast <bool> (::setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(cnt))))){
		// Выводим в лог информацию
		printf("Warning: cannot set TCP_KEEPCNT option on SOCKET=%d [%s]", fd, ::strerror(errno));
		// Выходим из функции
		return result;
	}
	/**
	 * Если мы работаем в MacOS X
	 */
	#ifdef __APPLE__
		// Время через которое происходит проверка подключения
		if(!(result = !static_cast <bool> (::setsockopt(fd, IPPROTO_TCP, TCP_KEEPALIVE, &idle, sizeof(idle))))){
			// Выводим в лог информацию
			printf("Warning: cannot set TCP_KEEPALIVE option on SOCKET=%d [%s]", fd, ::strerror(errno));
			// Выходим из функции
			return result;
		}
	/**
	 * Если мы работаем в FreeBSD или Linux
	 */
	#elif __linux__ || __FreeBSD__
		// Время через которое происходит проверка подключения
		if(!(result = !static_cast <bool> (::setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle))))){
			// Выводим в лог информацию
			printf("Warning: cannot set TCP_KEEPIDLE option on SOCKET=%d [%s]", fd, ::strerror(errno));
			// Выходим из функции
			return result;
		}
	#endif
	// Время между попытками
	if(!(result = !static_cast <bool> (::setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl)))))
		// Выводим в лог информацию
		printf("Warning: cannot set TCP_KEEPINTVL option on SOCKET=%d [%s]", fd, ::strerror(errno));
	// Выводим результат
	return result;
}

/**
 * reuseable Метод разрешающая повторно использовать сокет после его удаления
 * @param fd файловый дескриптор (сокет)
 * @return   результат работы функции
 */
static bool reuseable(const int fd) noexcept {
	// Результат работы функции
	bool result = false;
	// Устанавливаем параметр
	const int on = 1;
	// Разрешаем повторно использовать тот же host:port после отключения
	if(!(result = !static_cast <bool> (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast <const char *> (&on), sizeof(on))))){
		// Выводим в лог информацию
		printf("Warning: cannot set SO_REUSEADDR option on SOCKET=%d [%s]", fd, ::strerror(errno));
		// Выходим из функции
		return result;
	}
	/**
	 * Если операционная система не является Linux
	 */
	#if !defined(__linux__)
		// Разрешаем повторно использовать тот же host:port после отключения
		if(!(result = !static_cast <bool> (::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast <const char *> (&on), sizeof(on)))))
			// Выводим в лог информацию
			printf("Warning: cannot set SO_REUSEPORT option on SOCKET=%d [%s]", fd, ::strerror(errno));
	#endif
	// Выводим результат
	return result;
}

static int bufferSize(const int fd, const bool mode = true) noexcept {
	// Результат работы функции
	int result = 0;
	// Размер результата
	socklen_t size = sizeof(result);
	// Если необходимо получить размер буфера на чтение
	if(mode){
		// Считываем установленный размер буфера
		if(static_cast <bool> (::getsockopt(fd, SOL_SOCKET, SO_RCVBUF, reinterpret_cast <char *> (&result), &size)))
			// Выводим в лог информацию
			printf("Warning: get buffer read size wrong on SOCKET=%d [%s]", fd, ::strerror(errno));
	// Если необходимо получить размер буфера на запись
	} else {
		// Считываем установленный размер буфера
		if(static_cast <bool> (::getsockopt(fd, SOL_SOCKET, SO_SNDBUF, reinterpret_cast <char *> (&result), &size)))
			// Выводим в лог информацию
			printf("Warning: get buffer write size wrong on SOCKET=%d [%s]", fd, ::strerror(errno));
	}
	// Выводим результат
	return result;
}

static bool blocking(const int fd, const bool mode = true) noexcept {
	// Результат работы функции
	bool result = false;
	// Флаги файлового дескриптора
	int flags = 0;		
	// Получаем флаги файлового дескриптора
	if(!(result = ((flags = ::fcntl(fd, F_GETFL, nullptr)) >= 0))){
		// Выводим в лог информацию
		printf("Warning: cannot get BLOCK option on SOCKET=%d [%s]", fd, ::strerror(errno));
		// Выходим из функции
		return result;
	}
	// Если необходимо перевести сокет в блокирующий режим
	if(mode){
		// Если флаг уже установлен
		if(flags & O_NONBLOCK){
			// Устанавливаем неблокирующий режим
			if(!(result = (::fcntl(fd, F_SETFL, flags ^ O_NONBLOCK) >= 0)))
				// Выводим в лог информацию
				printf("Warning: cannot set BLOCK option on SOCKET=%d [%s]", fd, ::strerror(errno));
		}
	// Если необходимо перевести сокет в неблокирующий режим
	} else {
		// Если флаг ещё не установлен
		if(!(flags & O_NONBLOCK)){
			// Устанавливаем неблокирующий режим
			if(!(result = (::fcntl(fd, F_SETFL, flags | O_NONBLOCK) >= 0)))
				// Выводим в лог информацию
				printf("Warning: cannot set NON_BLOCK option on SOCKET=%d [%s]", fd, ::strerror(errno));
		}
	}
	// Выводим результат
	return result;
}

static string get_host(const int family) noexcept {
	// Определяем тип подключения
	switch(family){
		// Для протокола IPv6
		case AF_INET6: return "::";
		// Для протокола IPv4
		case AF_INET: return "0.0.0.0";
	}
	// Выводим результат
	return "";
}

static bool accept(const int sock, const int family) noexcept {
	// Заполняем структуру клиента нулями
	::memset(&peer.client, 0, sizeof(peer.client));
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
			peer.size = sizeof(client);
			// Выполняем копирование объекта подключения клиента
			::memcpy(&peer.client, &client, peer.size);
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
			peer.size = sizeof(client);
			// Выполняем копирование объекта подключения клиента
			::memcpy(&peer.client, &client, peer.size);
		} break;
	}
	// Определяем разрешено ли подключение к прокси серверу
	fd = ::accept(sock, reinterpret_cast <struct sockaddr *> (&peer.client), &peer.size);
	// Если сокет не создан тогда выходим
	if((fd == INVALID_SOCKET) || (fd >= MAX_SOCKETS))
		// Выходим из функции
		return false;
	// Определяем тип подключения
	switch(peer.client.ss_family){
		// Для протокола IPv4
		case AF_INET:
		// Для протокола IPv6
		case AF_INET6: {
			// Выполняем игнорирование сигнала неверной инструкции процессора
			noSigILL();
			// Отключаем сигнал записи в оборванное подключение
			noSigPIPE(fd);
			// Активируем KeepAlive
			keepAlive(fd, 3, 1, 2);
			// Устанавливаем разрешение на повторное использование сокета
			reuseable(fd);
			// Переводим сокет в не блокирующий режим
			blocking(fd, false);
		} break;
	}
	// Выводим результат
	return true;
}

static int64_t read(char * buffer, const size_t size) noexcept {
	// Результат работы функции
	int64_t result = 0;
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0) && ((fd != INVALID_SOCKET) && (fd < MAX_SOCKETS))){
		// Выполняем чтение данных из TCP/IP сокета
		result = ::recv(fd, buffer, size, 0);
		// Если данные прочитать не удалось
		if(result <= 0){
			// Определяем тип ошибки
			switch(errno){
				// Если ошибка не обнаружена, выходим
				case 0: break;
				/**
				 * Если мы работаем в MacOS X
				 */
				#ifdef __APPLE__
					// Если операция не поддерживается сокетом
					case EOPNOTSUPP:
				#endif
				// Если сработало событие таймаута
				case ETIME:
				// Если ошибка протокола
				case EPROTO:
				// Если в буфере закончились данные
				case ENOBUFS:
				// Если операция не поддерживается сокетом
				case ENOTSUP:
				// Если сеть отключена
				case ENETDOWN:
				// Если сокет не является сокетом
				case ENOTSOCK:
				// Если сокет не подключён
				case ENOTCONN:
				// Если хост не существует
				case EHOSTDOWN:
				// Если сокет отключён
				case ESHUTDOWN:
				// Если произведён сброс подключения
				case ETIMEDOUT:
				// Если подключение сброшено
				case ENETRESET:
				// Если операция отменена
				case ECANCELED:
				// Если подключение сброшено
				case ECONNRESET:
				// Если протокол повреждён для сокета
				case EPROTOTYPE:
				// Если протокол не поддерживается
				case ENOPROTOOPT:
				// Если сеть недоступна
				case ENETUNREACH:
				// Если протокол не поддерживается
				case EPFNOSUPPORT:
				// Если протокол подключения не поддерживается
				case EAFNOSUPPORT:
				// Если роутинг к хосту не существует
				case EHOSTUNREACH:
				// Если подключение оборванно
				case ECONNABORTED:
				// Если требуется адрес назначения
				case EDESTADDRREQ:
				// Если в соединении отказанно
				case ECONNREFUSED:
				// Если тип сокета не поддерживается
				case ESOCKTNOSUPPORT:
				// Если протокол не поддерживается
				case EPROTONOSUPPORT: {
					// Выводим в лог сообщение
					printf("Warning read: %s", ::strerror(errno));
					// Требуем завершения работы
					result = 0;
				} break;
				// Для остальных ошибок
				default: {
					// Если защищённый режим работы запрещён
					if((errno == EWOULDBLOCK) || (errno == EINTR))
						// Выполняем пропуск попытки
						return -1;
				}
			}
		}
	}
	// Выводим результат
	return result;
}

static int64_t write(const char * buffer, const size_t size) noexcept {
	// Результат работы функции
	int64_t result = 0;
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0) && ((fd != INVALID_SOCKET) && (fd < MAX_SOCKETS))){
		// Выполняем отправку данных в TCP/IP сокет
		result = ::send(fd, buffer, size, 0);
		// Если данные записать не удалось
		if(result <= 0){
			// Определяем тип ошибки
			switch(errno){
				// Если ошибка не обнаружена, выходим
				case 0: break;
				/**
				 * Если мы работаем в MacOS X
				 */
				#ifdef __APPLE__
					// Если операция не поддерживается сокетом
					case EOPNOTSUPP:
				#endif
				// Если сработало событие таймаута
				case ETIME:
				// Если ошибка протокола
				case EPROTO:
				// Если в буфере закончились данные
				case ENOBUFS:
				// Если операция не поддерживается сокетом
				case ENOTSUP:
				// Если сеть отключена
				case ENETDOWN:
				// Если сокет не является сокетом
				case ENOTSOCK:
				// Если сокет не подключён
				case ENOTCONN:
				// Если хост не существует
				case EHOSTDOWN:
				// Если сокет отключён
				case ESHUTDOWN:
				// Если произведён сброс подключения
				case ETIMEDOUT:
				// Если подключение сброшено
				case ENETRESET:
				// Если операция отменена
				case ECANCELED:
				// Если подключение сброшено
				case ECONNRESET:
				// Если протокол повреждён для сокета
				case EPROTOTYPE:
				// Если протокол не поддерживается
				case ENOPROTOOPT:
				// Если сеть недоступна
				case ENETUNREACH:
				// Если протокол не поддерживается
				case EPFNOSUPPORT:
				// Если протокол подключения не поддерживается
				case EAFNOSUPPORT:
				// Если роутинг к хосту не существует
				case EHOSTUNREACH:
				// Если подключение оборванно
				case ECONNABORTED:
				// Если требуется адрес назначения
				case EDESTADDRREQ:
				// Если в соединении отказанно
				case ECONNREFUSED:
				// Если тип сокета не поддерживается
				case ESOCKTNOSUPPORT:
				// Если протокол не поддерживается
				case EPROTONOSUPPORT: {
					// Выводим в лог сообщение
					printf("Warning write: %s", ::strerror(errno));
					// Требуем завершения работы
					result = 0;
				} break;
				// Для остальных ошибок
				default: {
					// Если защищённый режим работы запрещён
					if((errno == EWOULDBLOCK) || (errno == EINTR))
						// Выполняем пропуск попытки
						return -1;
				}
			}
		}
	}
	// Выводим результат
	return result;
}

static bool listing(int fd) noexcept {
	// Результат работы функции
	bool result = false;
	// Выполняем слушать порт сервера
	if(!(result = (::listen(fd, SOMAXCONN) == 0))){
		// Выводим сообщени об активном сервисе
		printf("Error: %s", ::strerror(errno));
		// Выходим из функции
		return result;
	}
	// Выводим результат
	return result;
}

static void init(const string & ip, const u_int port, const int family) noexcept {
	// Если IP адрес передан
	if(!ip.empty() && (port <= 65535)){
		// Если список сетевых интерфейсов установлен
		if((family == AF_INET) || (family == AF_INET6)){
			// Получаем хост текущего компьютера
			const string & host = get_host(family);
			// Определяем тип подключения
			switch(family){
				// Для протокола IPv4
				case AF_INET: {
					{
						// Создаём объект клиента
						struct sockaddr_in client;
						// Очищаем всю структуру для клиента
						::memset(&client, 0, sizeof(client));
						// Устанавливаем протокол интернета
						client.sin_family = family;
						// Выполняем копирование объекта подключения клиента
						::memcpy(&peer.client, &client, sizeof(client));
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
					// Запоминаем размер структуры
					peer.size = sizeof(server);
					// Выполняем копирование объекта подключения сервера
					::memcpy(&peer.server, &server, peer.size);
					// Обнуляем серверную структуру
					::memset(&(reinterpret_cast <struct sockaddr_in *> (&peer.server)->sin_zero), 0, sizeof(server.sin_zero));
				} break;
				// Для протокола IPv6
				case AF_INET6: {
					{
						// Создаём объект клиента
						struct sockaddr_in6 client;
						// Очищаем всю структуру для клиента
						::memset(&client, 0, sizeof(client));
						// Устанавливаем протокол интернета
						client.sin6_family = family;
						// Выполняем копирование объекта подключения клиента
						::memcpy(&peer.client, &client, sizeof(client));
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
					// Запоминаем размер структуры
					peer.size = sizeof(server);
					// Выполняем копирование объекта подключения сервера
					::memcpy(&peer.server, &server, peer.size);
				} break;
				// Если тип сети не определен
				default: {
					// Выводим сообщение в консоль
					printf("Error: network not allow from server [%s:%u]", ip.c_str(), port);
					// Выходим
					return;
				}
			}
			// Создаем сокет подключения
			fd = ::socket(family, SOCK_STREAM, IPPROTO_TCP);
			// Если сокет не создан то выходим
			if((fd == INVALID_SOCKET) || (fd >= MAX_SOCKETS)){
				// Выводим сообщение в консоль
				printf("Error: %s [%s:%u]", ::strerror(errno), ip.c_str(), port);
				// Выходим
				return;
			}
			// Выполняем игнорирование сигнала неверной инструкции процессора
			noSigILL();
			// Отключаем сигнал записи в оборванное подключение
			noSigPIPE(fd);
			/*
			// Включаем отображение сети IPv4 в IPv6
			if(family == AF_INET6)
				// Выполняем активацию работы только IPv6 сети
				onlyIPv6(fd, onlyV6 ? socket_t::mode_t::ENABLE : socket_t::mode_t::DISABLE);
			*/
			// Переводим сокет в не блокирующий режим
			blocking(fd, false);
			// Устанавливаем разрешение на повторное использование сокета
			reuseable(fd);
			// Выполняем бинд на сокет
			if(::bind(fd, reinterpret_cast <struct sockaddr *> (&peer.server), peer.size) < 0){
				// Выводим в лог сообщение
				printf("Error: %s SERVER=[%s]", ::strerror(errno), host.c_str());
				// Выходим из приложения
				::exit(EXIT_FAILURE);
			}
		}
	}
}

/**
 * main Главная функция приложения
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 */
int main(int argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Устанавливаем название сервиса
	log.name("Test server");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");

	init("127.0.0.1", 2222, AF_INET);

	const int sock = fd;

	if(listing(sock)){

		cout << " *********** LISTING " << sock << endl;

		base_t base(&fmk, &log);

		event_t server(&fmk, &log);
		event_t client(&fmk, &log);

		auto acceptFn = [&](const SOCKET sock, const base_t::event_type_t event) noexcept -> void {
			if(accept(sock, AF_INET)){
				cout << " Accept " << sock << " to " << fd << endl;

				client.set(fd);
				client.start();

				client.mode(base_t::event_type_t::READ, base_t::event_mode_t::ENABLED);
				client.mode(base_t::event_type_t::CLOSE, base_t::event_mode_t::ENABLED);

			}
		};

		auto processFn = [&](const SOCKET fd, const base_t::event_type_t event) noexcept -> void {
			// Определяем тип события
			switch(static_cast <uint8_t> (event)){
				case static_cast <uint8_t> (base_t::event_type_t::CLOSE): {
					client.stop();
					::close(fd);
				} break;
				case static_cast <uint8_t> (base_t::event_type_t::READ): {
					cout << " Read " << fd << endl;

					ssize_t size = bufferSize(fd);

					char * buffer = new char[size];

					cout << " Reading = " << (size = read(buffer, size)) << endl;

					if(size > 0){
						cout << " Result = " << string(buffer, size) << endl;
						client.mode(base_t::event_type_t::WRITE, base_t::event_mode_t::ENABLED);
					} else if(size == 0) {
						client.stop();
						::close(fd);
					}

					delete [] buffer;
				} break;
				case static_cast <uint8_t> (base_t::event_type_t::WRITE): {
					cout << " Write " << fd << endl;

					const string message = "Anyks is good!";

					ssize_t size = 0;

					cout << " Writing = " << (size = write(message.c_str(), message.size())) << endl;

					if(size > 0){
						client.mode(base_t::event_type_t::WRITE, base_t::event_mode_t::DISABLED);
					} else if(size == 0) {
						client.stop();
						::close(fd);
					}

				} break;
			}
		};

		client.set(&base);
		client.set(processFn);

		server.set(&base);
		server.set(acceptFn);
		server.set(sock);
		server.start();

		server.mode(base_t::event_type_t::READ, base_t::event_mode_t::ENABLED);

		base.start();

	}

	// Выводим результат
	return 0;
}
