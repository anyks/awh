/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_IFNET__
#define __AWH_IFNET__

/**
 * Устанавливаем настройки для *Nix подобных систем
 */
#if !defined(_WIN32) && !defined(_WIN64)

/**
 * Стандартная библиотека
 */
#include <cmath>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <unordered_map>
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
	#include <netdb.h>
	#include <net/if_dl.h>
	#include <net/ethernet.h>
	#include <netinet/if_ether.h>
	#include <sys/sockio.h>
	#include <sys/sysctl.h>
	#include <net/route.h>
#endif

/**
 * Наши модули
 */
#include <fmk.hpp>
#include <log.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * IfNet Класс работы с сетевыми интерфейсами
	 */
	typedef class IfNet {
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
			 */
			void getIPAddresses() noexcept;
			/**
			 * getHWAddresses Метод извлечения MAC адресов
			 */
			void getHWAddresses() noexcept;
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
			 * mac Метод получения MAC адреса по IP адресу клиента
			 * @param ip     адрес интернет-подключения клиента
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 * @return       аппаратный адрес сетевого интерфейса клиента
			 */
			const string mac(const string & ip, const int family = AF_INET) const noexcept;
			/**
			 * Метод вывода IP адреса соответствующего сетевому интерфейсу
			 * @param eth    название сетевого интерфейса
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 * @return       IP адрес соответствующий сетевому интерфейсу
			 */
			const string & ip(const string & eth, const int family = AF_INET) const noexcept;
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

#endif // NOT WINDOWS

#endif // __AWH_IFNET__
