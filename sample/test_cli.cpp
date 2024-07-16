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

SOCKET fd = 0;

/**
 * Методы только для OS Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * convert Метод конвертирования строки utf-8 в строку
	 * @param str строка utf-8 для конвертирования
	 * @return    обычная строка
	 */
	static string convert(const wstring & str) noexcept {
		// Результат работы функции
		string result = "";
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если строка передана
			if(!str.empty()){
				// Если используется BOOST
				#ifdef USE_BOOST_CONVERT
					// Объявляем конвертер
					using boost::locale::conv::utf_to_utf;
					// Выполняем конвертирование в utf-8 строку
					result = utf_to_utf <char> (str.c_str(), str.c_str() + str.size());
				// Если нужно использовать стандартную библиотеку
				#else
					// Устанавливаем тип для конвертера UTF-8
					using convert_type = codecvt_utf8 <wchar_t, 0x10ffff, little_endian>;
					// Объявляем конвертер
					wstring_convert <convert_type, wchar_t> conv;
					// wstring_convert <codecvt_utf8 <wchar_t>> conv;
					// Выполняем конвертирование в utf-8 строку
					result = conv.to_bytes(str);
				#endif
			}
		// Если возникает ошибка
		} catch(const range_error & error) {
			/* Пропускаем возникшую ошибку */
		}
		// Выводим результат
		return result;
	}

	/**
	 * message Метод получения текста описания ошибки
	 * @param code код ошибки для получения сообщения
	 * @return     текст сообщения описания кода ошибки
	 */
	static string message(const int32_t code = 0) noexcept {
		// Если код ошибки не передан
		if(code == 0)
			// Выполняем получение кода ошибки
			const_cast <int32_t &> (code) = WSAGetLastError();
		// Создаём буфер сообщения ошибки
		wchar_t message[256] = {0};
		// Выполняем формирование текста ошибки
		FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, code, 0, message, 256, 0);
		// Выводим текст полученной ошибки
		return convert(message);
	}
#endif

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
				/**
				 * Методы только для nix-подобных систем
				 */
				#if !defined(_WIN32) && !defined(_WIN64)
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
				#else
					// Если в буфере закончились данные
					case WSAENOBUFS:
					// Если сеть отключена
					case WSAENETDOWN:
					// Если сокет не является сокетом
					case WSAENOTSOCK:
					// Если сокет не подключён
					case WSAENOTCONN:
					// Если произведён сброс подключения
					case WSAETIMEDOUT:
					// Если сокет отключён
					case WSAESHUTDOWN:
					// Если хост не существует
					case WSAEHOSTDOWN:
					// Если подключение сброшено
					case WSAENETRESET:
					// Если протокол повреждён для сокета
					case WSAEPROTOTYPE:
					// Если подключение сброшено
					case WSAECONNRESET:
					// Если операция не поддерживается сокетом
					case WSAEOPNOTSUPP:
					// Если протокол не поддерживается
					case WSAENOPROTOOPT:
					// Если сеть недоступна
					case WSAENETUNREACH:
					// Если протокол не поддерживается
					case WSAEPFNOSUPPORT:
					// Если протокол подключения не поддерживается
					case WSAEAFNOSUPPORT:
					// Если роутинг к хосту не существует
					case WSAEHOSTUNREACH:
					// Если подключение оборванно
					case WSAECONNABORTED:
					// Если требуется адрес назначения
					case WSAEDESTADDRREQ:
					// Если в соединении отказанно
					case WSAECONNREFUSED:
					// Если тип сокета не поддерживается
					case WSAESOCKTNOSUPPORT:
					// Если протокол не поддерживается
					case WSAEPROTONOSUPPORT: {
						// Выводим в лог сообщение
						printf("Warning read: %s", message().c_str());
				#endif
					// Требуем завершения работы
					result = 0;
				} break;
				// Для остальных ошибок
				default: {
					/**
					 * Методы только для nix-подобных систем
					 */
					#if !defined(_WIN32) && !defined(_WIN64)
						// Если защищённый режим работы запрещён
						if((errno == EWOULDBLOCK) || (errno == EINTR))
							// Выполняем пропуск попытки
							return -1;
					#else
						// Если защищённый режим работы запрещён
						if((WSAGetLastError() == EWOULDBLOCK) || (WSAGetLastError() == EINTR))
							// Выполняем пропуск попытки
							return -1;
					#endif
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
				/**
				 * Методы только для nix-подобных систем
				 */
				#if !defined(_WIN32) && !defined(_WIN64)
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
				#else
					// Если в буфере закончились данные
					case WSAENOBUFS:
					// Если сеть отключена
					case WSAENETDOWN:
					// Если сокет не является сокетом
					case WSAENOTSOCK:
					// Если сокет не подключён
					case WSAENOTCONN:
					// Если произведён сброс подключения
					case WSAETIMEDOUT:
					// Если сокет отключён
					case WSAESHUTDOWN:
					// Если хост не существует
					case WSAEHOSTDOWN:
					// Если подключение сброшено
					case WSAENETRESET:
					// Если протокол повреждён для сокета
					case WSAEPROTOTYPE:
					// Если подключение сброшено
					case WSAECONNRESET:
					// Если операция не поддерживается сокетом
					case WSAEOPNOTSUPP:
					// Если протокол не поддерживается
					case WSAENOPROTOOPT:
					// Если сеть недоступна
					case WSAENETUNREACH:
					// Если протокол не поддерживается
					case WSAEPFNOSUPPORT:
					// Если протокол подключения не поддерживается
					case WSAEAFNOSUPPORT:
					// Если роутинг к хосту не существует
					case WSAEHOSTUNREACH:
					// Если подключение оборванно
					case WSAECONNABORTED:
					// Если требуется адрес назначения
					case WSAEDESTADDRREQ:
					// Если в соединении отказанно
					case WSAECONNREFUSED:
					// Если тип сокета не поддерживается
					case WSAESOCKTNOSUPPORT:
					// Если протокол не поддерживается
					case WSAEPROTONOSUPPORT: {
						// Выводим в лог сообщение
						printf("Warning write: %s", message().c_str());
				#endif
					// Требуем завершения работы
					result = 0;
				} break;
				// Для остальных ошибок
				default: {
					/**
					 * Методы только для nix-подобных систем
					 */
					#if !defined(_WIN32) && !defined(_WIN64)
						// Если защищённый режим работы запрещён
						if((errno == EWOULDBLOCK) || (errno == EINTR))
							// Выполняем пропуск попытки
							return -1;
					#else
						// Если защищённый режим работы запрещён
						if((WSAGetLastError() == EWOULDBLOCK) || (WSAGetLastError() == EINTR))
							// Выполняем пропуск попытки
							return -1;
					#endif
				}
			}
		}
	}
	// Выводим результат
	return result;
}

static void init(const string & ip, const u_int port, const int family, socket_t * mod) noexcept {
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
				/**
				 * Методы только для nix-подобных систем
				 */
				#if !defined(_WIN32) && !defined(_WIN64)
					// Выводим сообщение в консоль
					printf("Error: %s [%s:%u]", ::strerror(errno), ip.c_str(), port);
				#else
					// Выводим сообщени об активном сервисе
					printf("Error: %s [%s:%u]", message().c_str(), ip.c_str(), port);
				#endif
				// Выходим
				return;
			}
			/**
			 * Методы только для nix-подобных систем
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				// Выполняем игнорирование сигнала неверной инструкции процессора
				mod->noSigILL();
				// Отключаем сигнал записи в оборванное подключение
				mod->noSigPIPE(fd);
			#endif
			// Активируем KeepAlive
			mod->keepAlive(fd, 3, 1, 2);
			// Устанавливаем разрешение на повторное использование сокета
			mod->reuseable(fd);
			// Выполняем бинд на сокет
			if(::bind(fd, reinterpret_cast <struct sockaddr *> (&peer.client), peer.size) < 0){
				/**
				 * Методы только для nix-подобных систем
				 */
				#if !defined(_WIN32) && !defined(_WIN64)
					// Выводим в лог сообщение
					printf("Error: %s CLIENT=[%s]", ::strerror(errno), host.c_str());
				#else
					// Выводим в лог сообщение
					printf("Error: %s CLIENT=[%s]", message().c_str(), host.c_str());
				#endif
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
	log.name("Test client");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");

	base_t base(&fmk, &log);

	socket_t mod(&fmk, &log);

	init("127.0.0.1", 2222, AF_INET, &mod);

	if(connect()){
		cout << " Is Connected " << endl;

		event_t event(event_t::type_t::EVENT, &fmk, &log);

		auto processFn = [&](const SOCKET fd, const base_t::event_type_t type) noexcept -> void {
			// Определяем тип события
			switch(static_cast <uint8_t> (type)){
				case static_cast <uint8_t> (base_t::event_type_t::CLOSE): {
					event.stop();
					::close(fd);
					base.stop();
				} break;
				case static_cast <uint8_t> (base_t::event_type_t::READ): {

					ssize_t size = mod.bufferSize(fd, socket_t::mode_t::READ);

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
					event.mode(base_t::event_type_t::WRITE, base_t::event_mode_t::DISABLED);
				} break;
			}
		};

		event = fd;
		event = &base;
		event = processFn;
		event.start();
		event.mode(base_t::event_type_t::WRITE, base_t::event_mode_t::ENABLED);
		event.mode(base_t::event_type_t::CLOSE, base_t::event_mode_t::ENABLED);

		base.start();

	}

	// Выводим результат
	return 0;
}
