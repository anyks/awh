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
 * @copyright: Copyright © 2025
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
 * Максимальное количество доступных сокетов
 */
#define AWH_MAX_SOCKETS 0x5F5E100

/**
 * Для операционной системы не являющейся OS Windows
 */
#if !_WIN32 && !_WIN64
	#define SOCKET int32_t
	#define INVALID_SOCKET -1
#endif

/**
 * Стандартные модули
 */
#include <cstdio>
#include <string>
#include <cstring>
#include <cstdlib>

/**
 * Для операционной системы OS Windows
 */
#if _WIN32 || _WIN64
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <getopt.h>
	#include <mswsock.h>
	// Используем библиотеку ws2_32.lib
	#pragma comment(lib, "Ws2_32.lib")
/**
 * Для операционной системы не являющейся OS Windows
 */
#else
	#include <vector>
	#include <csignal>
	#include <fcntl.h>
	#include <unistd.h>
	#include <sys/ioctl.h>
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
#if __linux__ || __FreeBSD__
	#include <netinet/sctp.h>
/**
 * Для операционной системы Sun Solaris
 */
#elif __sun__
	#include <sys/termios.h>
#endif

/**
 * Наши модули
 */
#include "../sys/fmk.hpp"
#include "../sys/log.hpp"

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * Socket Класс работы с сетевыми сокетами
	 */
	typedef class AWHSHARED_EXPORT Socket {
		public:
			/**
			 * Режимы работы с сокетами
			 */
			enum class mode_t : uint8_t {
				NONE     = 0x00, // Флаг сокета не установлен
				READ     = 0x01, // Параметры сокета на чтение
				WRITE    = 0x02, // Параметры сокета на запись
				ENABLED  = 0x03, // Активация режима работы сокета
				DISABLED = 0x04  // Деактивация режима работы сокета
			};
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * noSigILL Метод блокировки сигнала SIGILL
			 * @return результат работы функции
			 */
			bool noSigILL() const noexcept;
		public:
			/**
			 * events Метод активации получения событий SCTP для сокета
			 * @param fd файловый дескриптор (сокет)
			 * @return   результат работы функции
			 */
			bool events(const SOCKET fd) const noexcept;
			/**
			 * noSigPIPE Метод игнорирования отключения сигнала записи в убитый сокет
			 * @param fd файловый дескриптор (сокет)
			 * @return   результат работы функции
			 */
			bool noSigPIPE(const SOCKET fd) const noexcept;
			/**
			 * reuseable Метод разрешающая повторно использовать сокет после его удаления
			 * @param fd файловый дескриптор (сокет)
			 * @return   результат работы функции
			 */
			bool reuseable(const SOCKET fd) const noexcept;
			/**
			 * closeOnExec Метод разрешения закрывать сокет, после запуска
			 * @param fd файловый дескриптор (сокет)
			 * @return   результат работы функции
			 */
			bool closeOnExec(const SOCKET fd) const noexcept;
		public:
			/**
			 * blocking Метод проверки сокета блокирующий режим
			 * @param fd файловый дескриптор (сокет)
			 * @return   результат работы функции
			 */
			bool blocking(const SOCKET fd) const noexcept;
			/**
			 * blocking Метод установки блокирующего сокета
			 * @param fd   файловый дескриптор (сокет)
			 * @param mode режим установки типа сокета
			 * @return     результат работы функции
			 */
			bool blocking(const SOCKET fd, const mode_t mode) const noexcept;
		public:
			/**
			 * cork Метод активации TCP/CORK
			 * @param fd   файловый дескриптор (сокет)
			 * @param mode режим установки типа сокета
			 * @return     результат работы функции
			 */
			bool cork(const SOCKET fd, const mode_t mode) const noexcept;
			/**
			 * nodelay Метод отключения алгоритма Нейгла
			 * @param fd   файловый дескриптор (сокет)
			 * @param mode режим установки типа сокета
			 * @return     результат работы функции
			 */
			bool nodelay(const SOCKET fd, const mode_t mode) const noexcept;
		public:
			/**
			 * error Метод получения кода ошибки
			 * @param fd файловый дескриптор (сокет)
			 * @return   код ошибки на сокете если присутствует
			 */
			int32_t error(const SOCKET fd) const noexcept;
			/**
			 * message Метод получения текста описания ошибки
			 * @param code код ошибки для получения сообщения
			 * @return     текст сообщения описания кода ошибки
			 */
			string message(const int32_t code = 0) const noexcept;
		public:
			/**
			 * onlyIPv6 Метод включающая или отключающая режим отображения IPv4 на IPv6
			 * @param fd   файловый дескриптор (сокет)
			 * @param mode режим активации или деактивации
			 * @return     результат работы функции
			 */
			bool onlyIPv6(const SOCKET fd, const mode_t mode = mode_t::DISABLED) const noexcept;
		public:
			/**
			 * timeout Метод установки таймаута на чтение из сокета
			 * @param fd   файловый дескриптор (сокет)
			 * @param msec время таймаута в миллисекундах
			 * @param mode режим установки типа сокета
			 * @return     результат работы функции
			 */
			bool timeout(const SOCKET fd, const uint32_t msec, const mode_t mode) const noexcept;
		public:
			/**
			 * timeToLive Метод установки времени жизни сокета
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 * @param fd     файловый дескриптор (сокет)
			 * @param ttl    время жизни файлового дескриптора в секундах (сокета)
			 * @return       результат установки времени жизни
			 */
			bool timeToLive(const int32_t family, const SOCKET fd, const int32_t ttl) const noexcept;
		public:
			/**
			 * isBind Метод проверки на занятость порта
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 * @param type   тип сокета SOCK_DGRAM или SOCK_STREAM
			 * @param port   номер порта для проверки
			 * @return       результат проверки
			 */
			bool isBind(const int32_t family, const int32_t type, const uint32_t port) const noexcept;
		public:
			/**
			 * keepAlive Метод устанавливает постоянное подключение на сокет
			 * @param fd    файловый дескриптор (сокет)
			 * @param cnt   максимальное количество попыток
			 * @param idle  время через которое происходит проверка подключения
			 * @param intvl время между попытками
			 * @return      результат работы функции
			 */
			bool keepAlive(const SOCKET fd, const int32_t cnt = 0, const int32_t idle = 0, const int32_t intvl = 0) const noexcept;
		public:
			/**
			 * listen Метод проверки сокета на прослушиваемость
			 * @param fd файловый дескриптор (сокет)
			 * @return   результат проверки сокета
			 */
			bool listen(const SOCKET fd) const noexcept;
		public:
			/**
			 * availability Метод проверки количества находящихся байт в сокете
			 * @param fd   файловый дескриптор (сокет)
			 * @param mode режим проверки типа сокета
			 * @return     запрашиваемый размер буфера
			 */
			u_long availability(const SOCKET fd, const mode_t mode) const noexcept;
		public:
			/**
			 * bufferSize Метод получения размера буфера
			 * @param fd   файловый дескриптор (сокет)
			 * @param mode режим проверки типа сокета
			 * @return     запрашиваемый размер буфера
			 */
			int32_t bufferSize(const SOCKET fd, const mode_t mode) const noexcept;
			/**
			 * bufferSize Метод установки размеров буфера
			 * @param fd   файловый дескриптор (сокет)
			 * @param size устанавливаемый размер буфера
			 * @param mode режим проверки типа сокета
			 * @return     результат работы функции
			 */
			bool bufferSize(const SOCKET fd, const int32_t size, const mode_t mode) const noexcept;
		public:
			/**
			 * Socket Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Socket(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {}
			/**
			 * ~Socket Деструктор
			 */
			~Socket() noexcept {}
	} socket_t;
};

#endif // __AWH_SOCKET__
