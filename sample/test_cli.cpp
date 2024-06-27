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

int fd = 0;

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

/**
 * connect Метод выполнения подключения
 * @return результат выполнения операции
 */
static bool connect() noexcept {
	// Результат работы функции
	bool result = false;
	// Определяем тип подключения
	switch(peer.server.ss_family){
		// Для протокола IPv4
		case AF_INET:
			// Запоминаем размер структуры
			peer.size = sizeof(struct sockaddr_in);
		break;
		// Для протокола IPv6
		case AF_INET6:
			// Запоминаем размер структуры
			peer.size = sizeof(struct sockaddr_in6);
		break;
	}
	// Если подключение не выполненно то сообщаем об этом, выполняем подключение к удаленному серверу
	result = ((peer.size > 0) && (::connect(fd, reinterpret_cast <struct sockaddr *> (&peer.server), peer.size) == 0));
	// Выводим результат
	return result;
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
						// Устанавливаем произвольный порт для локального подключения
						client.sin_port = htons(0);
						// Устанавливаем адрес для локальго подключения
						client.sin_addr.s_addr = inet_addr(host.c_str());
						// Запоминаем размер структуры
						peer.size = sizeof(client);
						// Выполняем копирование объекта подключения клиента
						::memcpy(&peer.client, &client, peer.size);
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
					// Выполняем копирование объекта подключения сервера
					::memcpy(&peer.server, &server, peer.size);
					// Обнуляем серверную структуру
					::memset(&(reinterpret_cast <struct sockaddr_in *> (&peer.server)->sin_zero), 0, sizeof(server.sin_zero));
				} break;
				// Для протокола IPv6
				case AF_INET6: {
					// Определяем тип приложения
					{
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
						peer.size = sizeof(client);
						// Выполняем копирование объекта подключения клиента
						::memcpy(&peer.client, &client, peer.size);
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
			// Активируем KeepAlive
			keepAlive(fd, 3, 1, 2);
			// Устанавливаем разрешение на повторное использование сокета
			reuseable(fd);
			// Выполняем бинд на сокет
			if(::bind(fd, reinterpret_cast <struct sockaddr *> (&peer.client), peer.size) < 0)
				// Выводим в лог сообщение
				printf("Error: %s CLIENT=[%s]", ::strerror(errno), host.c_str());
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

	if(connect()){
		cout << " Is Connected " << endl;

		base_t base(&fmk, &log);

		event_t event(&fmk, &log);

		auto processFn = [&](const SOCKET fd, const base_t::event_type_t type) noexcept -> void {
			// Определяем тип события
			switch(static_cast <uint8_t> (type)){
				case static_cast <uint8_t> (base_t::event_type_t::CLOSE): {
					event.stop();
					::close(fd);
					base.stop();
				} break;
				case static_cast <uint8_t> (base_t::event_type_t::READ): {

					ssize_t size = bufferSize(fd);

					char * buffer = new char[size];

					cout << " Reading = " << (size = read(buffer, size)) << endl;

					cout << " Result = " << string(buffer, size) << endl;

					delete [] buffer;

					event.stop();
					::close(fd);
					base.stop();

				} break;
				case static_cast <uint8_t> (base_t::event_type_t::WRITE): {
					const string message = "Hello World";

					cout << " Writing = " << write(message.c_str(), message.size()) << endl;

					event.mode(base_t::event_type_t::READ, base_t::event_mode_t::ENABLED);
				} break;
			}
		};

		event.set(&base);
		event.set(processFn);
		event.set(fd);
		event.start();
		event.mode(base_t::event_type_t::WRITE, base_t::event_mode_t::ENABLED);
		event.mode(base_t::event_type_t::CLOSE, base_t::event_mode_t::ENABLED);

		base.start();

	}

	// Выводим результат
	return 0;
}
