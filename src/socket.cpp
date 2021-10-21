/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <socket.hpp>

/**
 * serverIp Метод получения основного ip на сервере
 * @param family Тип интернет подключения (IPv4 или IPv6)
 */
const string awh::Sockets::serverIp(const int family) noexcept {
	// Определяем тип интернет протокола
	switch(family){
		// Если это IPv4
		case AF_INET: {
			// Создаем буфер для получения ip адреса
			char buffer[INET_ADDRSTRLEN];
			// Создаем сокет
			evutil_socket_t sock = socket(family, SOCK_DGRAM, 0);
			// Если сокет создан
			if(sock > -1){
				// Создаем структуру подключения сервера
				struct sockaddr_in serv;
				// Обнуляем структуру подключения
				memset(&serv, 0, sizeof(serv));
				// Создаем список dns серверов
				const vector <string> dns = IPV4_RESOLVER;
				// Указываем тип сетевого подключения IPv4
				serv.sin_family = family;
				// Устанавливаем порт DNS сервера
				serv.sin_port = htons(53);
				// Указываем адрес DNS сервера
				serv.sin_addr.s_addr = inet_addr(dns[0].c_str());
				// Выполняем подключение к серверу
				int conn = connect(sock, (const sockaddr *) &serv, sizeof(serv));
				// Если подключение удачное
				if(conn > -1){
					// Создаем структуру имени
					struct sockaddr_in name;
					// Размер структуры
					socklen_t namelen = sizeof(name);
					// Запрашиваем имя сокета
					conn = getsockname(sock, (sockaddr *) &name, &namelen);
					// Если ошибки нет
					if(conn > -1){
						// Обнуляем массив
						memset(buffer, 0, sizeof(buffer));
						// Запрашиваем данные ip адреса
						inet_ntop(family, &name.sin_addr, buffer, sizeof(buffer));
						// Выводим результат
						return buffer;
					}
				}
			}
		} break;
		// Если это IPv6
		case AF_INET6: {
			// Создаем буфер для получения ip адреса
			char buffer[INET6_ADDRSTRLEN];
			// Создаем сокет
			evutil_socket_t sock = socket(family, SOCK_DGRAM, 0);
			// Если сокет создан
			if(sock > -1){
				// Создаем структуру подключения сервера
				struct sockaddr_in6 serv;
				// Обнуляем структуру подключения
				memset(&serv, 0, sizeof(serv));
				// Создаем список dns серверов
				const vector <string> dns = IPV6_RESOLVER;
				// Указываем тип сетевого подключения IPv4
				serv.sin6_family = family;
				// Устанавливаем порт DNS сервера
				serv.sin6_port = htons(53);
				// Указываем адреса
				inet_pton(family, dns[0].c_str(), &serv.sin6_addr);
				// Устанавливаем адреса
				// inet_ntop(family, &serv.sin6_addr, buffer, sizeof(buffer));
				// Выполняем подключение к серверу
				int conn = connect(sock, (const sockaddr *) &serv, sizeof(serv));
				// Если подключение удачное
				if(conn > -1){
					// Создаем структуру имени
					struct sockaddr_in6 name;
					// Размер структуры
					socklen_t namelen = sizeof(name);
					// Запрашиваем имя сокета
					conn = getsockname(sock, (sockaddr *) &name, &namelen);
					// Если ошибки нет
					if(conn > -1){
						// Обнуляем массив
						memset(buffer, 0, sizeof(buffer));
						// Запрашиваем данные ip адреса
						inet_ntop(family, &name.sin6_addr, buffer, sizeof(buffer));
						// Выводим результат
						return buffer;
					}
				}
			}
		} break;
	}
	// Сообщаем что ничего не найдено
	return "";
}
/**
 * mac Метод определения мак адреса клиента
 * @param ctx указатель на объект подключения
 * @return    данные мак адреса
 */
const string awh::Sockets::mac(struct sockaddr * ctx) noexcept {
	// Результат работы функции
	string result = "";
	// Если данные переданы
	if(ctx != nullptr){
		// Буфер для копирования мак адреса
		char buffer[256];
		// Заполняем структуру нулями
		memset(buffer, 0, sizeof(buffer));
		// Получаем указатель на мак адрес
		const u_char * ptr = (u_char *) ctx->sa_data;
		// Записываем в буфер данные мак адреса
		sprintf(
			buffer, "%02X:%02X:%02X:%02X:%02X:%02X",
			(ptr[0] & 0xff), (ptr[1] & 0xff), (ptr[2] & 0xff),
			(ptr[3] & 0xff), (ptr[4] & 0xff), (ptr[5] & 0xff)
		);
		// Выводим данные мак адреса
		result = buffer;
	}
	// Выводим результат
	return result;
}
/**
 * ip Метод получения данных ip адреса
 * @param family тип интернет протокола
 * @param ctx    указатель на объект подключения
 * @return       данные ip адреса
 */
const string awh::Sockets::ip(const int family, void * ctx) noexcept {
	// Результат работы функции
	string result = "";
	// Если данные переданы
	if(ctx != nullptr){
		// Определяем тип интернет протокола
		switch(family){
			// Если это IPv4
			case AF_INET: {
				// Создаем буфер для получения ip адреса
				char buffer[INET_ADDRSTRLEN];
				// Заполняем структуру нулями
				memset(buffer, 0, sizeof(buffer));
				// Получаем данные адреса
				struct sockaddr_in * s = reinterpret_cast <struct sockaddr_in *> (ctx);
				// Копируем полученные данные
				inet_ntop(family, &s->sin_addr, buffer, sizeof(buffer));
				// Выводим результат
				result = buffer;
			}
			// Если это IPv6
			case AF_INET6: {
				// Создаем буфер для получения ip адреса
				char buffer[INET6_ADDRSTRLEN];
				// Заполняем структуру нулями
				memset(buffer, 0, sizeof(buffer));
				// Получаем данные адреса
				struct sockaddr_in6 * s = reinterpret_cast <struct sockaddr_in6 *> (ctx);
				// Копируем полученные данные
				inet_ntop(family, &s->sin6_addr, buffer, sizeof(buffer));
				// Выводим результат
				result = buffer;
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * Методы только для OS Windows
 */
#if defined(_WIN32) || defined(_WIN64)
/**
 * nonBlocking Метод установки неблокирующего сокета
 * @param fd файловый дескриптор (сокет)
 */
void awh::Sockets::nonBlocking(const evutil_socket_t fd) noexcept {
	// Флаг режима
	u_long mode = 0;
	// Отключаем неблокирующий режим у сокета
	ioctlsocket(fd, FIONBIO, &mode);
}
/**
 * keepAlive Метод устанавливает постоянное подключение на сокет
 * @param fd  файловый дескриптор (сокет)
 * @param log объект для работы с логами
 * @return    результат работы функции
 */
const int awh::Sockets::keepAlive(const evutil_socket_t fd, const log_t * log) noexcept {
	// Результат работы функции
	int result = -1;
	{
		// Флаг устанавливаемой опции KeepAlive
		bool option = false;
		// Устанавливаем опцию сокета KeepAlive
		result = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *) &option, sizeof(option));
		// Если мы получили ошибку, выходим сообщение
		if(result == SOCKET_ERROR){
			// Выводим в лог информацию
			if(log != nullptr) log->print("setsockopt for SO_KEEPALIVE failed with error: %u", log_t::flag_t::CRITICAL, WSAGetLastError());
			// Выходим
			return -1;
		}
	}{
		// Флаг получения устанавливаемой опции KeepAlive
		int option = 0;
		// Размер флага опции KeepAlive
		int size = sizeof(option);
		// Получаем опцию сокета KeepAlive
		result = getsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *) &option, &size);
		// Если мы получили ошибку, выходим сообщение
		if(result == SOCKET_ERROR){
			// Выводим в лог информацию
			if(log != nullptr) log->print("getsockopt for SO_KEEPALIVE failed with error: %u", log_t::flag_t::CRITICAL, WSAGetLastError());
			// Выходим
			return -1;
		}
	}
	// Выводим результат
	return result;
}
/**
 * Методы только для *Nix
 */
#else
/**
 * noSigill Метод блокировки сигнала SIGILL
 * @param log объект для работы с логами
 * @return    результат работы функции
 */
const int awh::Sockets::noSigill(const log_t * log) noexcept {
	// Создаем структуру активации сигнала
	struct sigaction act;
	// Зануляем структуру
	memset(&act, 0, sizeof(act));
	// Устанавливаем макрос игнорирования сигнала
	act.sa_handler = SIG_IGN;
	// Устанавливаем флаги перезагрузки
	act.sa_flags = (SA_ONSTACK | SA_RESTART | SA_SIGINFO);
	// Устанавливаем блокировку сигнала
	if(sigaction(SIGILL, &act, nullptr)){
		// Выводим в лог информацию
		if(log != nullptr) log->print("%s", log_t::flag_t::CRITICAL, "cannot set SIG_IGN on signal SIGILL");
		// Выходим
		return -1;
	}
	// Все удачно
	return 0;
}
/**
 * tcpCork Метод активации tcp_cork
 * @param fd  файловый дескриптор (сокет)
 * @param log объект для работы с логами
 * @return    результат работы функции
 */
const int awh::Sockets::tcpCork(const evutil_socket_t fd, const log_t * log) noexcept {
	// Устанавливаем параметр
	int tcpCork = 1;
	// Если это Linux
	#ifdef __linux__
		// Устанавливаем TCP_CORK
		if(setsockopt(fd, IPPROTO_TCP, TCP_CORK, &tcpCork, sizeof(tcpCork)) < 0){
			// Выводим в лог информацию
			if(log != nullptr) log->print("cannot set TCP_CORK option on socket %d", log_t::flag_t::CRITICAL, fd);
			// Выходим
			return -1;
		}
	// Если это FreeBSD или MacOS X
	#elif __APPLE__ || __MACH__ || __FreeBSD__
		// Устанавливаем TCP_NOPUSH
		if(setsockopt(fd, IPPROTO_TCP, TCP_NOPUSH, &tcpCork, sizeof(tcpCork)) < 0){
			// Выводим в лог информацию
			if(log != nullptr) log->print("cannot set TCP_NOPUSH option on socket %d", log_t::flag_t::CRITICAL, fd);
			// Выходим
			return -1;
		}
	#endif
	// Все удачно
	return 0;
}
/**
 * noSigpipe Метод игнорирования отключения сигнала записи в убитый сокет
 * @param fd  файловый дескриптор (сокет)
 * @param log объект для работы с логами
 * @return    результат работы функции
 */
const int awh::Sockets::noSigpipe(const evutil_socket_t fd, const log_t * log) noexcept {
	// Если это Linux
	#ifdef __linux__
		// Создаем структуру активации сигнала
		struct sigaction act;
		// Зануляем структуру
		memset(&act, 0, sizeof(act));
		// Устанавливаем макрос игнорирования сигнала
		act.sa_handler = SIG_IGN;
		// Устанавливаем флаг перезагрузки
		act.sa_flags = SA_RESTART;
		// Устанавливаем блокировку сигнала
		if(sigaction(SIGPIPE, &act, nullptr)){
			// Выводим в лог информацию
			if(log != nullptr) log->print("%s", log_t::flag_t::CRITICAL, "cannot set SIG_IGN on signal SIGPIPE");
			// Выходим
			return -1;
		}
		// Отключаем вывод сигнала записи в пустой сокет
		// if(signal(SIGPIPE, SIG_IGN) == SIG_ERR) return -1;
	// Если это FreeBSD или MacOS X
	#elif __APPLE__ || __MACH__ || __FreeBSD__
		// Устанавливаем параметр
		int noSigpipe = 1;
		// Устанавливаем SO_NOSIGPIPE
		if(setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &noSigpipe, sizeof(noSigpipe)) < 0){
			// Выводим в лог информацию
			if(log != nullptr) log->print("cannot set SO_NOSIGPIPE option on socket %d", log_t::flag_t::CRITICAL, fd);
			// Выходим
			return -1;
		}
	#endif
	// Все удачно
	return 0;
}
/**
 * nonBlocking Метод установки неблокирующего сокета
 * @param fd  файловый дескриптор (сокет)
 * @param log объект для работы с логами
 * @return    результат работы функции
 */
const int awh::Sockets::nonBlocking(const evutil_socket_t fd, const log_t * log) noexcept {
	// Получаем флаги файлового дескриптора
	int flags = fcntl(fd, F_GETFL);
	// Если флаги не установлены, выходим
	if(flags < 0) return flags;
	// Устанавливаем флаг, запрещающий блокировку файлового дескриптора
	flags |= O_NONBLOCK;
	// Устанавливаем неблокирующий режим
	if(fcntl(fd, F_SETFL, flags) < 0){
		// Выводим в лог информацию
		if(log != nullptr) log->print("cannot set NON_BLOCK option on socket %d", log_t::flag_t::CRITICAL, fd);
		// Выходим
		return -1;
	}
	// Все удачно
	return 0;
}
/**
 * keepAlive Метод устанавливает постоянное подключение на сокет
 * @param fd    файловый дескриптор (сокет)
 * @param cnt   максимальное количество попыток
 * @param idle  время через которое происходит проверка подключения
 * @param intvl время между попытками
 * @param log   объект для работы с логами
 * @return      результат работы функции
 */
const int awh::Sockets::keepAlive(const evutil_socket_t fd, const int cnt, const int idle, const int intvl, const log_t * log) noexcept {
	// Устанавливаем параметр
	int keepAlive = 1;
	// Активация постоянного подключения
	if(setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int))){
		// Выводим в лог информацию
		if(log != nullptr) log->print("cannot set SO_KEEPALIVE option on socket %d", log_t::flag_t::CRITICAL, fd);
		// Выходим
		return -1;
	}
	// Максимальное количество попыток
	if(setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(int))){
		// Выводим в лог информацию
		if(log != nullptr) log->print("cannot set TCP_KEEPCNT option on socket %d", log_t::flag_t::CRITICAL, fd);
		// Выходим
		return -1;
	}
	// Если это MacOS X
	#ifdef __APPLE__
		// Время через которое происходит проверка подключения
		if(setsockopt(fd, IPPROTO_TCP, TCP_KEEPALIVE, &idle, sizeof(int))){
			// Выводим в лог информацию
			if(log != nullptr) log->print("cannot set TCP_KEEPALIVE option on socket %d", log_t::flag_t::CRITICAL, fd);
			// Выходим
			return -1;
		}
	// Если это FreeBSD или Linux
	#elif __linux__ || __FreeBSD__
		// Время через которое происходит проверка подключения
		if(setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(int))){
			// Выводим в лог информацию
			if(log != nullptr) log->print("cannot set TCP_KEEPIDLE option on socket %d", log_t::flag_t::CRITICAL, fd);
			// Выходим
			return -1;
		}
	#endif
	// Время между попытками
	if(setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(int))){
		// Выводим в лог информацию
		if(log != nullptr) log->print("cannot set TCP_KEEPINTVL option on socket %d", log_t::flag_t::CRITICAL, fd);
		// Выходим
		return -1;
	}
	// Все удачно
	return 0;
}
#endif
/**
 * reuseable Метод разрешающая повторно использовать сокет после его удаления
 * @param fd  файловый дескриптор (сокет)
 * @param log объект для работы с логами
 * @return    результат работы функции
 */
const int awh::Sockets::reuseable(const evutil_socket_t fd, const log_t * log) noexcept {
	// Устанавливаем параметр
	int reuseaddr = 1;
	// Разрешаем повторно использовать тот же host:port после отключения
	if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &reuseaddr, sizeof(reuseaddr)) < 0){
		// Выводим в лог информацию
		if(log != nullptr) log->print("cannot set SO_REUSEADDR option on socket %d", log_t::flag_t::CRITICAL, fd);
		// Выходим
		return -1;
	}
	// Все удачно
	return 0;
}
/**
 * tcpNodelay Метод отключения алгоритма Нейгла
 * @param fd  файловый дескриптор (сокет)
 * @param log объект для работы с логами
 * @return    результат работы функции
 */
const int awh::Sockets::tcpNodelay(const evutil_socket_t fd, const log_t * log) noexcept {
	// Устанавливаем параметр
	int tcpNodelay = 1;
	// Устанавливаем TCP_NODELAY
	if(setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *) &tcpNodelay, sizeof(tcpNodelay)) < 0){
		// Выводим в лог информацию
		if(log != nullptr) log->print("cannot set TCP_NODELAY option on socket %d", log_t::flag_t::CRITICAL, fd);
		// Выходим
		return -1;
	}
	// Все удачно
	return 0;
}
/**
 * ipV6only Метод включающая или отключающая режим отображения IPv4 на IPv6
 * @param fd   файловый дескриптор (сокет)
 * @param mode активация или деактивация режима
 * @param log  объект для работы с логами
 * @return     результат работы функции
 */
const int awh::Sockets::ipV6only(const evutil_socket_t fd, const bool mode, const log_t * log) noexcept {
	// Устанавливаем параметр
	int only6 = (mode ? 1 : 0);
	// Разрешаем повторно использовать тот же host:port после отключения
	if(setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (char *) &only6, sizeof(only6)) < 0){
		// Выводим в лог информацию
		if(log != nullptr) log->print("cannot set IPV6_V6ONLY option on socket %d", log_t::flag_t::CRITICAL, fd);
		// Выходим
		return -1;
	}
	// Все удачно
	return 0;
}
/**
 * bufferSize Метод установки размеров буфера
 * @param fd    файловый дескриптор (сокет)
 * @param read  размер буфера на чтение
 * @param write размер буфера на запись
 * @param total максимальное количество подключений
 * @param log   объект для работы с логами
 * @return      результат работы функции
 */
const int awh::Sockets::bufferSize(const evutil_socket_t fd, const int read, const int write, const u_int total, const log_t * log) noexcept {
	// Определяем размер массива опции
	socklen_t rlen = sizeof(read);
	socklen_t wlen = sizeof(write);
	// Получаем переданные размеры
	int readSize  = (read > 0 ? read : BUFFER_SIZE_RCV);
	int writeSize = (write > 0 ? write : BUFFER_SIZE_SND);
	// Устанавливаем размер буфера для сокета на чтение
	if(readSize > 0){
		// Выполняем перерасчет размера буфера
		readSize = (readSize / total);
		// Устанавливаем размер буфера
		setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *) &readSize, rlen);
	}
	// Устанавливаем размер буфера для сокета на запись
	if(writeSize > 0){
		// Выполняем перерасчет размера буфера
		writeSize = (writeSize / total);
		// Устанавливаем размер буфера
		setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *) &writeSize, wlen);
	}
	// Считываем установленный размер буфера
	if((getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *) &readSize, &rlen) < 0) ||
	   (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *) &writeSize, &wlen) < 0)){
		// Выводим в лог информацию
		if(log != nullptr) log->print("get buffer wrong on socket %d", log_t::flag_t::CRITICAL, fd);
		// Выходим
		return -1;
	}
	// Все удачно
	return 0;
}
