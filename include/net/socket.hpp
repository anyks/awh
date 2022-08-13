/**
 * @file: socket.hpp
 * @date: 2021-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2021
 */

#ifndef __AWH_SOCKET__
#define __AWH_SOCKET__

#ifndef UNICODE
	#define UNICODE
#endif

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif

/**
 * Стандартная библиотека
 */
#include <set>
#include <cstdio>
#include <string>
#include <cstring>
#include <stdlib.h>

/**
 * Методы только для OS Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	#include <time.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <getopt.h>
	// Используем библиотеку ws2_32.lib
	#pragma comment(lib, "Ws2_32.lib")
/**
 * Для всех остальных операционных систем
 */
#else
	#include <ctime>
	#include <vector>
	#include <fcntl.h>
	#include <signal.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <arpa/inet.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <sys/resource.h>
#endif

/**
 * Если операционной системой является Linux или FreeBSD
 */
#if defined(__linux__) || defined(__FreeBSD__)
	#include <netinet/sctp.h>
#endif

/**
 * Наши модули
 */
#include <sys/log.hpp>

// Устанавливаем область видимости
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Socket Класс работы с сетевыми сокетами
	 */
	typedef class Socket {
		private:
			// Создаём объект работы с логами
			const log_t * _log;
		public:
			/**
			 * noSigill Метод блокировки сигнала SIGILL
			 * @return результат работы функции
			 */
			int noSigill() const noexcept;
			/**
			 * tcpCork Метод активации tcp_cork
			 * @param fd файловый дескриптор (сокет)
			 * @return   результат работы функции
			 */
			int tcpCork(const int fd) const noexcept;
			/**
			 * blocking Метод установки блокирующего сокета
			 * @param fd файловый дескриптор (сокет)
			 * @return   результат работы функции
			 */
			int blocking(const int fd) const noexcept;
			/**
			 * noSigpipe Метод игнорирования отключения сигнала записи в убитый сокет
			 * @param fd файловый дескриптор (сокет)
			 * @return   результат работы функции
			 */
			int noSigpipe(const int fd) const noexcept;
			/**
			 * reuseable Метод разрешающая повторно использовать сокет после его удаления
			 * @param fd файловый дескриптор (сокет)
			 * @return   результат работы функции
			 */
			int reuseable(const int fd) const noexcept;
			/**
			 * tcpNodelay Метод отключения алгоритма Нейгла
			 * @param fd файловый дескриптор (сокет)
			 * @return   результат работы функции
			 */
			int tcpNodelay(const int fd) const noexcept;
			/**
			 * isBlocking Метод проверки сокета блокирующий режим
			 * @param fd файловый дескриптор (сокет)
			 * @return   результат работы функции
			 */
			int isBlocking(const int fd) const noexcept;
			/**
			 * sctpEvents Метод активации получения событий SCTP для сокета
			 * @param fd файловый дескриптор (сокет)
			 * @return   результат работы функции
			 */
			int sctpEvents(const int fd) const noexcept;
			/**
			 * closeonexec Метод разрешения закрывать сокет, после запуска
			 * @param fd файловый дескриптор (сокет)
			 * @return   результат работы функции
			 */
			int closeonexec(const int fd) const noexcept;
			/**
			 * nonBlocking Метод установки неблокирующего сокета
			 * @param fd файловый дескриптор (сокет)
			 * @return   результат работы функции
			 */
			int nonBlocking(const int fd) const noexcept;
			/**
			 * readTimeout Метод установки таймаута на чтение из сокета
			 * @param fd   файловый дескриптор (сокет)
			 * @param msec время таймаута в миллисекундах
			 * @return     результат работы функции
			 */
			int readTimeout(const int fd, const time_t msec) const noexcept;
			/**
			 * writeTimeout Метод установки таймаута на запись в сокет
			 * @param fd   файловый дескриптор (сокет)
			 * @param msec время таймаута в миллисекундах
			 * @return     результат работы функции
			 */
			int writeTimeout(const int fd, const time_t msec) const noexcept;
			/**
			 * ipV6only Метод включающая или отключающая режим отображения IPv4 на IPv6
			 * @param fd   файловый дескриптор (сокет)
			 * @param mode активация или деактивация режима
			 * @return     результат работы функции
			 */
			int ipV6only(const int fd, const bool mode = false) const noexcept;
			/**
			 * keepAlive Метод устанавливает постоянное подключение на сокет
			 * @param fd    файловый дескриптор (сокет)
			 * @param cnt   максимальное количество попыток
			 * @param idle  время через которое происходит проверка подключения
			 * @param intvl время между попытками
			 * @return      результат работы функции
			 */
			int keepAlive(const int fd, const int cnt = 0, const int idle = 0, const int intvl = 0) const noexcept;
			/**
			 * bufferSize Метод установки размеров буфера
			 * @param fd    файловый дескриптор (сокет)
			 * @param read  размер буфера на чтение
			 * @param write размер буфера на запись
			 * @param total максимальное количество подключений
			 * @return      результат работы функции
			 */
			int bufferSize(const int fd, const int read = 0, const int write = 0, const u_int total = 0) const noexcept;
		public:
			/**
			 * Socket Конструктор
			 * @param log объект для работы с логами
			 */
			Socket(const log_t * log) noexcept : _log(log) {}
			/**
			 * ~Socket Деструктор
			 */
			~Socket() noexcept {}
	} socket_t;
};

#endif // __AWH_SOCKET__
