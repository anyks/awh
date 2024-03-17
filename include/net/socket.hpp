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
 * Максимальное количество доступных сокетов
 */
#define MAX_SOCKETS 65535

/**
 * Если операционной системой является Nix-подобная
 */
#if !defined(_WIN32) && !defined(_WIN64)
	#define SOCKET int
	#define INVALID_SOCKET -1
#endif

/**
 * Стандартные модули
 */
#include <set>
#include <ctime>
#include <cstdio>
#include <string>
#include <cstring>
#include <cstdlib>

/**
 * Методы только для OS Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <getopt.h>
	// Используем библиотеку ws2_32.lib
	#pragma comment(lib, "Ws2_32.lib")
/**
 * Для всех остальных операционных систем
 */
#else
	#include <cerrno>
	#include <vector>
	#include <csignal>
	#include <fcntl.h>
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
#include <sys/fmk.hpp>
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
		public:
			/**
			 * mode_t Флаги для работы с сокетами
			 */
			enum class mode_t : uint8_t {
				NONE    = 0x00, // Флаг сокета не установлен
				READ    = 0x01, // Параметры сокета на на чтение
				WRITE   = 0x02, // Параметры сокета на на запись
				BLOCK   = 0x03, // Сокет должен быть блокирующим
				NOBLOCK = 0x04  // Сокет должен быть не блокирующим
			};
		private:
			// Создаём объект фреймворка
			const fmk_t * _fmk;
			// Создаём объект работы с логами
			const log_t * _log;
		public:
			/**
			 * noSigILL Метод блокировки сигнала SIGILL
			 * @return результат работы функции
			 */
			bool noSigILL() const noexcept;
		public:
			/**
			 * cork Метод активации tcp_cork
			 * @param fd файловый дескриптор (сокет)
			 * @return   результат работы функции
			 */
			bool cork(const SOCKET fd) const noexcept;
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
			 * @param mode флаг установки типа сокета
			 * @return     результат работы функции
			 */
			bool blocking(const SOCKET fd, const mode_t mode) const noexcept;
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
			 * nodelay Метод отключения алгоритма Нейгла
			 * @param fd файловый дескриптор (сокет)
			 * @return   результат работы функции
			 */
			bool nodelay(const SOCKET fd) const noexcept;
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
			 * onlyIPv6 Метод включающая или отключающая режим отображения IPv4 на IPv6
			 * @param fd   файловый дескриптор (сокет)
			 * @param mode активация или деактивация режима
			 * @return     результат работы функции
			 */
			bool onlyIPv6(const SOCKET fd, const bool mode = false) const noexcept;
		public:
			/**
			 * timeout Метод установки таймаута на чтение из сокета
			 * @param fd   файловый дескриптор (сокет)
			 * @param msec время таймаута в миллисекундах
			 * @param mode флаг установки типа сокета
			 * @return     результат работы функции
			 */
			bool timeout(const SOCKET fd, const time_t msec, const mode_t mode) const noexcept;
		public:
			/**
			 * timeToLive Метод установки времени жизни сокета
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 * @param fd     файловый дескриптор (сокет)
			 * @param ttl    время жизни файлового дескриптора в секундах (сокета)
			 * @return       результат установки времени жизни
			 */
			bool timeToLive(const int family, const SOCKET fd, const int ttl) const noexcept;
		public:
			/**
			 * keepAlive Метод устанавливает постоянное подключение на сокет
			 * @param fd    файловый дескриптор (сокет)
			 * @param cnt   максимальное количество попыток
			 * @param idle  время через которое происходит проверка подключения
			 * @param intvl время между попытками
			 * @return      результат работы функции
			 */
			bool keepAlive(const SOCKET fd, const int cnt = 0, const int idle = 0, const int intvl = 0) const noexcept;
		public:
			/**
			 * bufferSize Метод получения размера буфера
			 * @param fd   файловый дескриптор (сокет)
			 * @param mode флаг установки типа сокета
			 * @return     запрашиваемый размер буфера
			 */
			int bufferSize(const SOCKET fd, const mode_t mode) const noexcept;
			/**
			 * bufferSize Метод установки размеров буфера
			 * @param fd    файловый дескриптор (сокет)
			 * @param size  устанавливаемый размер буфера
			 * @param total максимальное количество подключений
			 * @param mode  флаг установки типа сокета
			 * @return      результат работы функции
			 */
			bool bufferSize(const SOCKET fd, const int size, const u_int total, const mode_t mode) const noexcept;
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
