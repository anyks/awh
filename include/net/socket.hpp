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

/**
 * Для операционной системы не являющейся MS Windows
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
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <getopt.h>
	#include <mswsock.h>
	// Используем библиотеку ws2_32.lib
	#pragma comment(lib, "Ws2_32.lib")
/**
 * Для операционной системы не являющейся MS Windows
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
 * @brief пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief Класс работы с сетевыми сокетами
	 *
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
			 * @brief Метод блокировки сигнала SIGILL
			 *
			 * @return результат работы функции
			 */
			bool noSigILL() const noexcept;
		public:
			/**
			 * @brief Метод активации получения событий SCTP для сокета
			 *
			 * @param sock сетевой сокет
			 * @return     результат работы функции
			 */
			bool events(const SOCKET sock) const noexcept;
			/**
			 * @brief Метод игнорирования отключения сигнала записи в убитый сокет
			 *
			 * @param sock сетевой сокет
			 * @return     результат работы функции
			 */
			bool noSigPIPE(const SOCKET sock) const noexcept;
			/**
			 * @brief Метод разрешающая повторно использовать сокет после его удаления
			 *
			 * @param sock сетевой сокет
			 * @return     результат работы функции
			 */
			bool reuseable(const SOCKET sock) const noexcept;
			/**
			 * @brief Метод разрешения закрывать сокет, после запуска
			 *
			 * @param sock сетевой сокет
			 * @return     результат работы функции
			 */
			bool closeOnExec(const SOCKET sock) const noexcept;
		public:
			/**
			 * @brief Метод проверки сокета блокирующий режим
			 *
			 * @param sock сетевой сокет
			 * @return     результат работы функции
			 */
			bool blocking(const SOCKET sock) const noexcept;
			/**
			 * @brief Метод установки блокирующего сокета
			 *
			 * @param sock сетевой сокет
			 * @param mode режим установки типа сокета
			 * @return     результат работы функции
			 */
			bool blocking(const SOCKET sock, const mode_t mode) const noexcept;
		public:
			/**
			 * @brief Метод активации TCP/CORK
			 *
			 * @param sock сетевой сокет
			 * @param mode режим установки типа сокета
			 * @return     результат работы функции
			 */
			bool cork(const SOCKET sock, const mode_t mode) const noexcept;
			/**
			 * @brief Метод отключения алгоритма Нейгла
			 *
			 * @param sock сетевой сокет
			 * @param mode режим установки типа сокета
			 * @return     результат работы функции
			 */
			bool nodelay(const SOCKET sock, const mode_t mode) const noexcept;
		public:
			/**
			 * @brief Метод получения кода ошибки
			 *
			 * @param sock сетевой сокет
			 * @return     код ошибки на сокете если присутствует
			 */
			int32_t error(const SOCKET sock) const noexcept;
			/**
			 * @brief Метод получения текста описания ошибки
			 *
			 * @param code код ошибки для получения сообщения
			 * @return     текст сообщения описания кода ошибки
			 */
			string message(const int32_t code = 0) const noexcept;
		public:
			/**
			 * @brief Метод включающая или отключающая режим отображения IPv4 на IPv6
			 *
			 * @param sock сетевой сокет
			 * @param mode режим активации или деактивации
			 * @return     результат работы функции
			 */
			bool onlyIPv6(const SOCKET sock, const mode_t mode = mode_t::DISABLED) const noexcept;
		public:
			/**
			 * @brief Метод установки таймаута на чтение из сокета
			 *
			 * @param sock сетевой сокет
			 * @param msec время таймаута в миллисекундах
			 * @param mode режим установки типа сокета
			 * @return     результат работы функции
			 */
			bool timeout(const SOCKET sock, const uint32_t msec, const mode_t mode) const noexcept;
		public:
			/**
			 * @brief Метод установки времени жизни сокета
			 *
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 * @param sock   сетевой сокет
			 * @param ttl    время жизни сетевого сокета в секундах
			 * @return       результат установки времени жизни
			 */
			bool timeToLive(const int32_t family, const SOCKET sock, const int32_t ttl) const noexcept;
		public:
			/**
			 * @brief Метод проверки на занятость порта
			 *
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 * @param type   тип сокета SOCK_DGRAM или SOCK_STREAM
			 * @param port   номер порта для проверки
			 * @return       результат проверки
			 */
			bool isBind(const int32_t family, const int32_t type, const uint32_t port) const noexcept;
		public:
			/**
			 * @brief Метод устанавливает постоянное подключение на сокет
			 *
			 * @param sock  сетевой сокет
			 * @param cnt   максимальное количество попыток
			 * @param idle  время через которое происходит проверка подключения
			 * @param intvl время между попытками
			 * @return      результат работы функции
			 */
			bool keepAlive(const SOCKET sock, const int32_t cnt = 0, const int32_t idle = 0, const int32_t intvl = 0) const noexcept;
		public:
			/**
			 * @brief Метод проверки сокета на прослушиваемость
			 *
			 * @param sock сетевой сокет
			 * @return     результат проверки сокета
			 */
			bool listen(const SOCKET sock) const noexcept;
		public:
			/**
			 * @brief Метод проверки количества находящихся байт в сокете
			 *
			 * @param sock сетевой сокет
			 * @param mode режим проверки типа сокета
			 * @return     запрашиваемый размер буфера
			 */
			u_long availability(const SOCKET sock, const mode_t mode) const noexcept;
		public:
			/**
			 * @brief Метод получения размера буфера
			 *
			 * @param sock сетевой сокет
			 * @param mode режим проверки типа сокета
			 * @return     запрашиваемый размер буфера
			 */
			int32_t bufferSize(const SOCKET sock, const mode_t mode) const noexcept;
			/**
			 * @brief Метод установки размеров буфера
			 *
			 * @param sock сетевой сокет
			 * @param size устанавливаемый размер буфера
			 * @param mode режим проверки типа сокета
			 * @return     результат работы функции
			 */
			bool bufferSize(const SOCKET sock, const int32_t size, const mode_t mode) const noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Socket(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {}
			/**
			 * @brief Деструктор
			 *
			 */
			~Socket() noexcept {}
	} socket_t;
};

#endif // __AWH_SOCKET__
