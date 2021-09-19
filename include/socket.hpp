/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_SOCKETS__
#define __AWH_SOCKETS__

/**
 * Стандартная библиотека
 */
#include <cstdio>
#include <string>
#include <cstring>
#include <stdlib.h>
#include <event2/listener.h>

// Если - это Windows
#if defined(_WIN32) || defined(_WIN64)
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <getopt.h>
// Если - это Unix
#else
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
 * Наши модули
 */
#include <log.hpp>

// Устанавливаем область видимости
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * Sockets Структура функций для работы с сокетами
	 */
	typedef struct Sockets {
		/**
		 * serverIp Метод получения основного ip на сервере
		 * @param family Тип интернет подключения (IPv4 или IPv6)
		 */
		static const string serverIp(const int family) noexcept;
		/**
		 * mac Метод определения мак адреса клиента
		 * @param ctx указатель на объект подключения
		 * @return    данные мак адреса
		 */
		static const string mac(struct sockaddr * ctx = nullptr) noexcept;
		/**
		 * ip Метод получения данных ip адреса
		 * @param family тип интернет протокола
		 * @param ctx    указатель на объект подключения
		 * @return       данные ip адреса
		 */
		static const string ip(const int family, void * ctx = nullptr) noexcept;
		/**
		 * Если - это Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			/**
			 * blocking Функция установки режима блокировки сокета
			 * @param fd файловый дескриптор (сокет)
			 */
			static void blocking(const SOCKET fd = -1) noexcept;
		/**
		 * Если - это Unix
		 */
		#else
			/**
			 * noSigill Метод блокировки сигнала SIGILL
			 * @param log объект для работы с логами
			 * @return    результат работы функции
			 */
			static const int noSigill(const log_t * log = nullptr) noexcept;
			/**
			 * tcpCork Метод активации tcp_cork
			 * @param fd  файловый дескриптор (сокет)
			 * @param log объект для работы с логами
			 * @return    результат работы функции
			 */
			static const int tcpCork(const evutil_socket_t fd = -1, const log_t * log = nullptr) noexcept;
			/**
			 * reuseable Метод разрешающая повторно использовать сокет после его удаления
			 * @param fd  файловый дескриптор (сокет)
			 * @param log объект для работы с логами
			 * @return    результат работы функции
			 */
			static const int reuseable(const evutil_socket_t fd = -1, const log_t * log = nullptr) noexcept;
			/**
			 * noSigpipe Метод игнорирования отключения сигнала записи в убитый сокет
			 * @param fd  файловый дескриптор (сокет)
			 * @param log объект для работы с логами
			 * @return    результат работы функции
			 */
			static const int noSigpipe(const evutil_socket_t fd = -1, const log_t * log = nullptr) noexcept;
			/**
			 * tcpNodelay Метод отключения алгоритма Нейгла
			 * @param fd  файловый дескриптор (сокет)
			 * @param log объект для работы с логами
			 * @return    результат работы функции
			 */
			static const int tcpNodelay(const evutil_socket_t fd = -1, const log_t * log = nullptr) noexcept;
			/**
			 * nonBlocking Метод установки неблокирующего сокета
			 * @param fd  файловый дескриптор (сокет)
			 * @param log объект для работы с логами
			 * @return    результат работы функции
			 */
			static const int nonBlocking(const evutil_socket_t fd = -1, const log_t * log = nullptr) noexcept;
			/**
			 * ipV6only Метод включающая или отключающая режим отображения IPv4 на IPv6
			 * @param fd   файловый дескриптор (сокет)
			 * @param mode активация или деактивация режима
			 * @param log  объект для работы с логами
			 * @return     результат работы функции
			 */
			static const int ipV6only(const evutil_socket_t fd = -1, const bool mode = false, const log_t * log = nullptr) noexcept;
			/**
			 * keepAlive Метод устанавливает постоянное подключение на сокет
			 * @param fd    файловый дескриптор (сокет)
			 * @param cnt   максимальное количество попыток
			 * @param idle  время через которое происходит проверка подключения
			 * @param intvl время между попытками
			 * @param log   объект для работы с логами
			 * @return      результат работы функции
			 */
			static const int keepAlive(const evutil_socket_t fd = -1, const int cnt = 0, const int idle = 0, const int intvl = 0, const log_t * log = nullptr) noexcept;
			/**
			 * bufferSize Метод установки размеров буфера
			 * @param fd         файловый дескриптор (сокет)
			 * @param read_size  размер буфера на чтение
			 * @param write_size размер буфера на запись
			 * @param maxcon     максимальное количество подключений
			 * @param log        объект для работы с логами
			 * @return           результат работы функции
			 */
			static const int bufferSize(const evutil_socket_t fd = -1, const int read_size = 0, const int write_size = 0, const u_int maxcon = 0, const log_t * log = nullptr) noexcept;
		#endif
	} sockets_t;
};

#endif // __AWH_SOCKETS__
