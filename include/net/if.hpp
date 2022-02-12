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
 * @copyright: Copyright © 2021
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
 * Устанавливаем настройки для *Nix подобных систем
 */
#if !defined(_WIN32) && !defined(_WIN64)

/**
 * Стандартные библиотеки
 */
#include <errno.h>
#include <unistd.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>

/**
 * Если операционной системой является MacOS X или FreeBSD
 */
#if __APPLE__ || __MACH__ || __FreeBSD__
	/**
	 * Стандартные библиотеки
	 */
	#include <netdb.h>
	#include <net/if_dl.h>
	#include <net/ethernet.h>
	#include <netinet/if_ether.h>
	#include <sys/sockio.h>
	#include <sys/sysctl.h>
	#include <net/route.h>
	// Создаём функцию округления
	#define ROUNDUP(a) \
		((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
/**
 * Если операционной системой является Linux
 */
#elif __linux__
	/**
	 * Стандартные библиотеки
	 */
	#include <ifaddrs.h>
	#include <stddef.h>
	#include <stdbool.h>
	#include <net/if_arp.h>
#endif

/**
 * Если операционной системой является OS Windows
 */
#else
	/**
	 * Стандартные библиотеки
	 */
	#include <stdio.h>
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
 * Наши модули
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * IfNet Класс работы с сетевыми интерфейсами
	 */
	typedef class IfNet {
		private:
			// Результат работы функции
			string result = "";
		private:
			// Список сетевых интерфейсов
			unordered_map <string, string> ifs;
			// Список интернет-адресов
			unordered_map <string, string> ips;
			// Список интернет-адресов
			unordered_map <string, string> ips6;
		private:
			// Максимальная длина сетевого интерфейса
			static constexpr u_short MAX_ADDRS = 32;
			// Максимальный размер сетевого буфера
			static constexpr u_short IF_BUFFER_SIZE = 4000;
		private:
			// Создаём объект фреймворка
			const fmk_t * fmk = nullptr;
			// Создаём объект работы с логами
			const log_t * log = nullptr;
		private:
			/**
			 * getIPAddresses Метод извлечения IP адресов
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 */
			void getIPAddresses(const int family = AF_INET) noexcept;
			/**
			 * getHWAddresses Метод извлечения MAC адресов
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 */
			void getHWAddresses(const int family = AF_INET) noexcept;
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
			 * Метод вывода списка MAC адресов
			 * @return список MAC адресов
			 */
			const unordered_map <string, string> & hws() const noexcept;
		public:
			/**
			 * name Метод запроса названия сетевого интерфейса
			 * @param eth идентификатор сетевого интерфейса
			 * @return    название сетевого интерфейса
			 */
			const string name(const string & eth) const noexcept;
		public:
			/**
			 * mac Метод получения MAC адреса по IP адресу клиента
			 * @param ip     адрес интернет-подключения клиента
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 * @return       аппаратный адрес сетевого интерфейса клиента
			 */
			const string mac(const string & ip, const int family = AF_INET) const noexcept;
			/**
			 * mac Метод определения мак адреса клиента
			 * @param sin    объект подключения
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 * @return       данные мак адреса
			 */
			const string mac(struct sockaddr * sin, const int family = AF_INET) const noexcept;
		public:
			/**
			 * ip Метод получения основного IP адреса на сервере
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 */
			const string ip(const int family = AF_INET) const noexcept;
			/**
			 * ip Метод вывода IP адреса соответствующего сетевому интерфейсу
			 * @param eth    идентификатор сетевого интерфейса
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 * @return       IP адрес соответствующий сетевому интерфейсу
			 */
			const string & ip(const string & eth, const int family = AF_INET) const noexcept;
			/**
			 * ip Метод получения IP адреса из подключения
			 * @param sin    объект подключения
			 * @param family тип интернет протокола
			 * @return       данные ip адреса
			 */
			const string ip(struct sockaddr * sin, const int family = AF_INET) const noexcept;
		public:
			/**
			 * IfNet Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			IfNet(const fmk_t * fmk, const log_t * log) noexcept : fmk(fmk), log(log) {}
			/**
			 * ~IfNet Деструктор
			 */
			~IfNet() noexcept {}
	} ifnet_t;
};

#endif // __AWH_IFNET__
