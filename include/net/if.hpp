/**
 * @file: if.hpp
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

#ifndef __AWH_IFNET__
#define __AWH_IFNET__

/**
 * Стандартные библиотеки
 */
#include <cmath>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <unordered_map>

/**
 * Для операционной системы не являющейся OS Windows
 */
#if !defined(_WIN32) && !defined(_WIN64)
	#define SOCKET int32_t
	#define INVALID_SOCKET -1
#endif

/**
 * Для операционной системы не являющейся OS Windows
 */
#if !defined(_WIN32) && !defined(_WIN64)
	/**
	 * Стандартные библиотеки
	 */
	#include <unistd.h>
	#include <net/if.h>
	#include <arpa/inet.h>
	#include <sys/types.h>
	#include <sys/ioctl.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	/**
	 * Для операционной системы MacOS X, FreeBSD, NetBSD или OpenBSD
	 */
	#if __APPLE__ || __MACH__ || __FreeBSD__
		/**
		 * Стандартные библиотеки
		 */
		#include <net/ethernet.h>
	#endif
	/**
	 * Для операционной системы MacOS X, FreeBSD, NetBSD или OpenBSD
	 */
	#if __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
		/**
		 * Стандартные библиотеки
		 */
		#include <netdb.h>
		#include <net/if_dl.h>
		#include <netinet/if_ether.h>
		#include <sys/sockio.h>
		#include <sys/sysctl.h>
		#include <net/route.h>
		// Создаём функцию округления
		#define ROUNDUP(a) \
			((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
	/**
	 * Для операционной системы Linux
	 */
	#elif __linux__
		/**
		 * Стандартные библиотеки
		 */
		#include <cstddef>
		#include <ifaddrs.h>
		#include <stdbool.h>
		#include <net/if_arp.h>
	#endif
/**
 * Для операционной системы OS Windows
 */
#else
	/**
	 * Стандартные библиотеки
	 */
	#include <ws2tcpip.h>
	#include <winsock2.h>
	#include <iphlpapi.h>
	// Используем библиотеку ws2_32.lib
	#pragma comment(lib, "Ws2_32.lib")
	/**
	 * Создаём привычные нам функции выделения памяти
	 */
	#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
	#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))
#endif

/**
 * Для систем Apple выполняем фиксацию ошибки переопределения макроса ошибки события
 */
#ifdef __APPLE__
	// Если макрос пределён
	#ifdef EV_ERROR
		// Снимаем определение макроса
		#undef EV_ERROR
	#endif
#endif

/**
 * Наши модули
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * IfNet Класс работы с сетевыми интерфейсами
	 */
	typedef class AWHSHARED_EXPORT IfNet {
		private:
			// Список сетевых интерфейсов
			unordered_map <string, string> _ifs;
			// Список интернет-адресов
			unordered_map <string, string> _ips;
			// Список интернет-адресов
			unordered_map <string, string> _ips6;
		private:
			// Максимальная длина сетевого интерфейса
			static constexpr uint16_t MAX_ADDRS = 32;
			// Максимальный размер сетевого буфера
			static constexpr uint16_t IF_BUFFER_SIZE = 4000;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		private:
			/**
			 * getIPAddresses Метод извлечения IP-адресов
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 */
			void getIPAddresses(const int32_t family = AF_INET) noexcept;
			/**
			 * getHWAddresses Метод извлечения MAC-адресов
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 */
			void getHWAddresses(const int32_t family = AF_INET) noexcept;
		private:
			/**
			 * Метод закрытие подключения
			 * @param fd файловый дескриптор (сокет)
			 */
			void close(const int32_t fd) const noexcept;
		public:
			/**
			 * init Метод инициализации сбора информации
			 */
			void init() noexcept;
			/**
			 * clear Метод очистки собранных данных
			 */
			void clear() noexcept;
		public:
			/**
			 * Метод вывода списка MAC-адресов
			 * @return список MAC-адресов
			 */
			const unordered_map <string, string> & hws() const noexcept;
		public:
			/**
			 * name Метод запроса названия сетевого интерфейса
			 * @param eth идентификатор сетевого интерфейса
			 * @return    название сетевого интерфейса
			 */
			string name(const string & eth) const noexcept;
		public:
			/**
			 * mac Метод получения MAC-адреса по IP-адресу клиента
			 * @param ip     адрес интернет-подключения клиента
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 * @return       аппаратный адрес сетевого интерфейса клиента
			 */
			string mac(const string & ip, const int32_t family = AF_INET) const noexcept;
			/**
			 * mac Метод определения мак адреса клиента
			 * @param sin    объект подключения
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 * @return       данные мак адреса
			 */
			string mac(struct sockaddr * sin, const int32_t family = AF_INET) const noexcept;
		public:
			/**
			 * ip Метод получения основного IP-адреса на сервере
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 */
			string ip(const int32_t family = AF_INET) const noexcept;
			/**
			 * ip Метод получения IP-адреса из подключения
			 * @param sin    объект подключения
			 * @param family тип интернет протокола
			 * @return       данные ip адреса
			 */
			string ip(struct sockaddr * sin, const int32_t family = AF_INET) const noexcept;
			/**
			 * ip Метод вывода IP-адреса соответствующего сетевому интерфейсу
			 * @param eth    идентификатор сетевого интерфейса
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 * @return       IP-адрес соответствующий сетевому интерфейсу
			 */
			const string & ip(const string & eth, const int32_t family = AF_INET) const noexcept;
		public:
			/**
			 * IfNet Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			IfNet(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {}
			/**
			 * ~IfNet Деструктор
			 */
			~IfNet() noexcept {}
	} ifnet_t;
};

#endif // __AWH_IFNET__
