/**
 * @file: if.cpp
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

/**
 * Подключаем заголовочный файл
 */
#include <net/if.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * getIPAddresses Метод извлечения IP-адресов
 * @param family тип протокола интернета AF_INET или AF_INET6
 */
void awh::IfNet::getIPAddresses(const int32_t family) noexcept {
	/**
	 * Если операционной системой является MacOS X, FreeBSD, NetBSD или OpenBSD
	 */
	#if __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
		// Структура параметров сетевого интерфейса
		struct ifconf ifc;
		// Структура сетевого интерфейса
		struct ifreq ifrc;
		// Создаём буфер для извлечения сетевых данных
		char buffer[IF_BUFFER_SIZE];
		// Заполняем нуляем наши буферы
		::memset(buffer, 0, sizeof(buffer));
		// Выделяем сокет для подключения
		const SOCKET fd = ::socket(family, SOCK_DGRAM, IPPROTO_IP);
		// Если файловый дескриптор не создан, выходим
		if(fd == INVALID_SOCKET){
			// Выводим сообщение об ошибке
			this->_log->print("Socket failed", log_t::flag_t::WARNING);
			// Выходим из функции
			return;
		}
		// Устанавливаем буфер для получения параметров сетевого интерфейса
		ifc.ifc_buf = buffer;
		// Устанавливаем максимальный размер буфера
		ifc.ifc_len = IF_BUFFER_SIZE;
		// Выполняем получение сетевых параметров
		if(::ioctl(fd, SIOCGIFCONF, &ifc) < 0){
			// Закрываем сетевой сокет
			this->close(fd);
			// Выводим сообщение об ошибке
			this->_log->print("IOCTL failed", log_t::flag_t::WARNING);
			// Выходим из функции
			return;
		}
		// Создаём указатели для перехода
		char * cptr = nullptr;
		// Выполняем перебор всех сетевых интерфейсов
		for(char * ptr = buffer; ptr < (buffer + ifc.ifc_len);){
			// Выполняем получение сетевого интерфейса
			struct ifreq * ifr = reinterpret_cast <struct ifreq *> (ptr);
			// Выполняем смещение указателя в цикле
			ptr += (sizeof(ifr->ifr_name) + ::max(sizeof(struct sockaddr), static_cast <size_t> (ifr->ifr_addr.sa_len)));
			// Если сетевой интерфейс отличается от IPv4 пропускаем
			if(ifr->ifr_addr.sa_family != family)
				// Выполняем пропуск
				continue;
			// Если в имени сетевого интерфейса найден разделитель, пропускаем его
			if((cptr = reinterpret_cast <char *> (strchr(ifr->ifr_name, ':'))) != nullptr) (* cptr) = 0;
			// Запоминаем текущее значение указателя
			ifrc = (* ifr);
			// Считываем флаги для сетевого интерфейса
			::ioctl(fd, SIOCGIFFLAGS, &ifrc);
			// Если флаги не соответствуют, пропускаем
			if((ifrc.ifr_flags & IFF_UP) == 0)
				// Выполняем пропуск
				continue;
			// Определяем тип интернет адреса
			switch(ifr->ifr_addr.sa_family){
				// Если мы обрабатываем IPv4
				case AF_INET:
					// Устанавливаем сетевой интерфейс в список
					this->_ips.emplace(ifr->ifr_name, inet_ntoa(reinterpret_cast <struct sockaddr_in *> (&ifr->ifr_addr)->sin_addr));
				break;
				// Если мы обрабатываем IPv6
				case AF_INET6: {
					// Создаём буфер для получения ip адреса
					char buffer[INET6_ADDRSTRLEN];
					// Заполняем нуляем наши буферы
					::memset(buffer, 0, INET6_ADDRSTRLEN);
					// Запрашиваем данные ip адреса
					inet_ntop(AF_INET6, reinterpret_cast <void *> (&reinterpret_cast <struct sockaddr_in6 *> (&ifr->ifr_addr)->sin6_addr), buffer, INET6_ADDRSTRLEN);
					// Устанавливаем сетевой интерфейс в список
					this->_ips6.emplace(ifr->ifr_name, buffer);
				} break;
			}
		}
		// Закрываем сетевой сокет
		this->close(fd);
	/**
	 * Если операционной системой является Linux или Sun Solaris
	 */
	#elif __linux__ || __sun__
		// Структура параметров сетевого интерфейса
		struct ifconf ifc;
		// Структура сетевого интерфейса
		struct ifreq ifrc;
		// Создаём буфер для извлечения сетевых данных
		char buffer[IF_BUFFER_SIZE];
		// Заполняем нуляем наши буферы
		::memset(buffer, 0, sizeof(buffer));
		// Выделяем сокет для подключения
		const SOCKET fd = ::socket(family, SOCK_DGRAM, IPPROTO_IP);
		// Если файловый дескриптор не создан, выходим
		if(fd == INVALID_SOCKET){
			// Выводим сообщение об ошибке
			this->_log->print("Socket failed", log_t::flag_t::WARNING);
			// Выходим из функции
			return;
		}
		// Устанавливаем буфер для получения параметров сетевого интерфейса
		ifc.ifc_buf = buffer;
		// Устанавливаем максимальный размер буфера
		ifc.ifc_len = IF_BUFFER_SIZE;
		// Выполняем получение сетевых параметров
		if(::ioctl(fd, SIOCGIFCONF, &ifc) < 0){
			// Закрываем сетевой сокет
			this->close(fd);
			// Выводим сообщение об ошибке
			this->_log->print("IOCTL failed", log_t::flag_t::WARNING);
			// Выходим из функции
			return;
		}
		// Создаём указатели для перехода
		char * cptr = nullptr;
		// Получаем текущее значение итератора
		struct ifreq * i = ifc.ifc_req;
		// Получаем конечное значение итератора
		const struct ifreq * const end = (i + (ifc.ifc_len / sizeof(struct ifreq)));
		// Переходим по всем сетевым интерфейсам
		for(; i != end; ++i){
			// Если сетевой интерфейс отличается от IPv4 пропускаем
			if(i->ifr_addr.sa_family != family)
				// Выполняем пропуск
				continue;
			// Если в имени сетевого интерфейса найден разделитель, пропускаем его
			if((cptr = reinterpret_cast <char *> (strchr(i->ifr_name, ':'))) != nullptr)
				// Зануляем значение
				(* cptr) = 0;
			// Запоминаем текущее значение указателя
			ifrc = (* i);
			// Считываем флаги для сетевого интерфейса
			::ioctl(fd, SIOCGIFFLAGS, &ifrc);
			// Если флаги не соответствуют, пропускаем
			if((ifrc.ifr_flags & IFF_UP) == 0)
				// Выполняем пропуск
				continue;
			// Определяем тип интернет адреса
			switch(i->ifr_addr.sa_family){
				// Если мы обрабатываем IPv4
				case AF_INET:
					// Устанавливаем сетевой интерфейс в список
					this->_ips.emplace(i->ifr_name, inet_ntoa(reinterpret_cast <struct sockaddr_in *> (&i->ifr_addr)->sin_addr));
				break;
				// Если мы обрабатываем IPv6
				case AF_INET6: {
					// Создаём буфер для получения ip адреса
					char buffer[INET6_ADDRSTRLEN];
					// Заполняем нуляем наши буферы
					::memset(buffer, 0, INET6_ADDRSTRLEN);
					// Запрашиваем данные ip адреса
					inet_ntop(AF_INET6, reinterpret_cast <void *> (&reinterpret_cast <struct sockaddr_in6 *> (&i->ifr_addr)->sin6_addr), buffer, INET6_ADDRSTRLEN);
					// Устанавливаем сетевой интерфейс в список
					this->_ips6.emplace(i->ifr_name, buffer);
				} break;
			}
		}
		// Закрываем сетевой сокет
		this->close(fd);
	/**
	 * Устанавливаем настройки для OS Windows
	 */
	#elif _WIN32 || _WIN64
		// Получаем размер буфера данных
		ULONG size = sizeof(IP_ADAPTER_ADDRESSES);
		// Выделяем память под буфер данных
		PIP_ADAPTER_ADDRESSES addr = reinterpret_cast <IP_ADAPTER_ADDRESSES *> (MALLOC(size));
		// Выполняем первоначальный вызов GetAdaptersAddresses, чтобы получить необходимый размер.
		if(GetAdaptersAddresses(family, GAA_FLAG_INCLUDE_PREFIX, nullptr, addr, &size) == ERROR_BUFFER_OVERFLOW){
			// Очищаем выделенную ранее память
			FREE(addr);
			// Выделяем ещё раз память для буфера данных
			addr = reinterpret_cast <IP_ADAPTER_ADDRESSES *> (MALLOC(size));
		}
		// Если буфер данных не существует
		if(addr == nullptr){
			// Выводим сообщение об ошибке
			this->_log->print("Memory allocation failed for IP_ADAPTER_ADDRESSES struct", log_t::flag_t::WARNING);
			// Выходим из функции
			return;
		}
		// Выполняем получения данных сетевого адаптера
		const DWORD result = GetAdaptersAddresses(family, GAA_FLAG_INCLUDE_PREFIX, nullptr, addr, &size);
		// Если данные сетевого адаптера считаны удачно
		if(result == NO_ERROR){
			// Создаём буфер для получения IPv4 адреса
			char ipv4[INET_ADDRSTRLEN];
			// Создаём буфер для получения IPv6 адреса
			char ipv6[INET6_ADDRSTRLEN];
			// В случае успеха выводим некоторую информацию из полученных данных.
			PIP_ADAPTER_ADDRESSES adapter = addr;
			// Выполняем обработку всех сетевых адаптеров
			while(adapter != nullptr){
				// Получаем данные сетевого интерфейса
				PIP_ADAPTER_UNICAST_ADDRESS host = adapter->FirstUnicastAddress;
				// Определяем тип сетевого интерфейса
				switch(host->Address.lpSockaddr->sa_family){
					// Если мы обрабатываем IPv4
					case AF_INET: {
						// Зануляем буфер данных
						::memset(ipv4, 0, INET_ADDRSTRLEN);
						// Получаем буфер IP-адреса
						SOCKADDR_IN * sin = reinterpret_cast <SOCKADDR_IN *> (host->Address.lpSockaddr);
						// Копируем данные IP-адреса в буфер
						inet_ntop(AF_INET, &(sin->sin_addr), ipv4, INET_ADDRSTRLEN);
						// Устанавливаем сетевой интерфейс в список
						this->_ips.emplace(adapter->AdapterName, ipv4);
					} break;
					// Если мы обрабатываем IPv6
					case AF_INET6: {
						// Зануляем буфер данных
						::memset(ipv6, 0, INET6_ADDRSTRLEN);
						// Получаем буфер IP-адреса
						SOCKADDR_IN6 * sin = reinterpret_cast <SOCKADDR_IN6 *> (host->Address.lpSockaddr);
						// Получаем буфер IP-адреса
						inet_ntop(AF_INET6, &(sin->sin6_addr), ipv6, INET6_ADDRSTRLEN);
						// Устанавливаем сетевой интерфейс в список
						this->_ips6.emplace(adapter->AdapterName, ipv6);
					} break;
				}
				// Выполняем смену итератора
				adapter = adapter->Next;
			}
		// Если данные адаптера прочитать не вышло
		} else {
			// Выводим сообщение об ошибке
			this->_log->print("Call to GetAdaptersAddresses failed with error: %d", log_t::flag_t::WARNING, result);
			// Если ошибка связана с отсутствием данных
			if(result == ERROR_NO_DATA)
				// Выводим сообщение об ошибке
				this->_log->print("No addresses were found for the requested parameters", log_t::flag_t::WARNING);
			// Если данные существуют
			else {
				// Создаём объект сообщения
				LPVOID message = nullptr;
				// Формируем генерарцию сообщения
				if(FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, result, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) & message, 0, nullptr)){
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::WARNING, message);
					// Очищаем выделенную память сообщения
					LocalFree(message);
				}
			}
		}
		// Очищаем выделенную ранее память
		FREE(addr);
	#endif
}
/**
 * getHWAddresses Метод извлечения MAC-адресов
 * @param family тип протокола интернета AF_INET или AF_INET6
 */
void awh::IfNet::getHWAddresses(const int32_t family) noexcept {
	/**
	 * Если операционной системой является MacOS X, FreeBSD, NetBSD или OpenBSD
	 */
	#if __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
		// Структура параметров сетевого интерфейса
		struct ifconf ifc;
		// Создаём буфер для извлечения сетевых данных
		char buffer[IF_BUFFER_SIZE];
		// Заполняем нуляем наши буферы
		::memset(buffer, 0, sizeof(buffer));
		// Выделяем сокет для подключения
		const SOCKET fd = ::socket(family, SOCK_DGRAM, IPPROTO_IP);
		// Если файловый дескриптор не создан, выходим
		if(fd == INVALID_SOCKET){
			// Выводим сообщение об ошибке
			this->_log->print("Socket failed", log_t::flag_t::WARNING);
			// Выходим из функции
			return;
		}
		// Устанавливаем буфер для получения параметров сетевого интерфейса
		ifc.ifc_buf = buffer;
		// Устанавливаем максимальный размер буфера
		ifc.ifc_len = IF_BUFFER_SIZE;
		// Выполняем получение сетевых параметров
		if(::ioctl(fd, SIOCGIFCONF, &ifc) < 0){
			// Закрываем сетевой сокет
			this->close(fd);
			// Выводим сообщение об ошибке
			this->_log->print("IOCTL failed", log_t::flag_t::WARNING);
			// Выходим из функции
			return;
		}
		// Временный буфер MAC-адреса
		char temp[80];
		// Выполняем смещение в буфере
		char * cplim = (buffer + ifc.ifc_len);
		// Выполняем перебор всех сетевых интерфейсов
		for(char * cp = buffer; cp < cplim;){
			// Выполняем получение сетевого интерфейса
			struct ifreq * ifr = reinterpret_cast <struct ifreq *> (cp);
			// Нам нужен реальный сетевой интерфейс
			if(ifr->ifr_addr.sa_family == AF_LINK){
				// Заполняем нуляем наши буферы
				::memset(temp, 0, sizeof(temp));
				// Инициализируем все октеты
				int32_t a = 0, b = 0, c = 0, d = 0, e = 0, f = 0;
				// Получаем сетевой интерфейс
				struct sockaddr_dl * sdl = reinterpret_cast <struct sockaddr_dl *> (&ifr->ifr_addr);
				// Копируем MAC-адрес из сетевого интерфейса
				::strcpy(temp, reinterpret_cast <char *> (ether_ntoa(reinterpret_cast <struct ether_addr *> (LLADDR(sdl)))));
				// Устанавливаем сетевые октеты
				::sscanf(temp, "%x:%x:%x:%x:%x:%x", &a, &b, &c, &d, &e, &f);
				// Формируем MAC-адрес удобный для вывода
				::sprintf(temp, "%02X:%02X:%02X:%02X:%02X:%02X", a, b, c, d, e, f);
				// Добавляем MAC-адрес в список сетевых интерфейсов
				this->_ifs.emplace(ifr->ifr_name, temp);
			}
			// Выполняем смещение указателя в цикле
			cp += (sizeof(ifr->ifr_name) + ::max(sizeof(ifr->ifr_addr), static_cast <size_t> (ifr->ifr_addr.sa_len)));
		}
		// Закрываем сетевой сокет
		this->close(fd);
	/**
	 * Если операционной системой является Linux
	 */
	#elif __linux__
		// Структура параметров сетевого интерфейса
		struct ifconf ifc;
		// Структура сетевого интерфейса
		struct ifreq ifrc;
		// Создаём буфер для извлечения сетевых данных
		char buffer[IF_BUFFER_SIZE];
		// Заполняем нуляем наши буферы
		::memset(buffer, 0, sizeof(buffer));
		// Выделяем сокет для подключения
		const SOCKET fd = ::socket(family, SOCK_DGRAM, IPPROTO_IP);
		// Если файловый дескриптор не создан, выходим
		if(fd == INVALID_SOCKET){
			// Выводим сообщение об ошибке
			this->_log->print("Socket failed", log_t::flag_t::WARNING);
			// Выходим из функции
			return;
		}
		// Устанавливаем буфер для получения параметров сетевого интерфейса
		ifc.ifc_buf = buffer;
		// Устанавливаем максимальный размер буфера
		ifc.ifc_len = IF_BUFFER_SIZE;
		// Выполняем получение сетевых параметров
		if(::ioctl(fd, SIOCGIFCONF, &ifc) < 0){
			// Закрываем сетевой сокет
			this->close(fd);
			// Выводим сообщение об ошибке
			this->_log->print("IOCTL failed", log_t::flag_t::WARNING);
			// Выходим из функции
			return;
		}
		// Временный буфер MAC-адреса
		char hardware[18];
		// Получаем текущее значение итератора
		struct ifreq * i = ifc.ifc_req;
		// Получаем конечное значение итератора
		const struct ifreq * const end = (i + (ifc.ifc_len / sizeof(struct ifreq)));
		// Переходим по всем сетевым интерфейсам
		for(; i != end; ++i){
			// Копируем название сетевого интерфейса
			strcpy(ifrc.ifr_name, i->ifr_name);
			// Выполняем подключение к сокету
			if(::ioctl(fd, SIOCGIFFLAGS, &ifrc) == 0){
				// Проверяем сетевой интерфейс (не loopback)
				if(!(ifrc.ifr_flags & IFF_LOOPBACK)){
					// Извлекаем аппаратный адрес сетевого интерфейса
					if(::ioctl(fd, SIOCGIFHWADDR, &ifrc) == 0){
						// Создаём буфер MAC-адреса
						uint8_t mac[6];
						// Заполняем нуляем наши буферы
						::memset(hardware, 0, sizeof(hardware));
						// Выполняем копирование MAC-адреса
						::memcpy(mac, ifrc.ifr_hwaddr.sa_data, 6);
						// Выполняем получение MAC-адреса
						::sprintf(hardware, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
						// Добавляем MAC-адрес в список сетевых интерфейсов
						this->_ifs.emplace(i->ifr_name, hardware);
					}
				}
			// Если подключение к сокету не удалось, выводим сообщение об ошибке
			} else this->_log->print("IOCTL failed", log_t::flag_t::WARNING);
		}
		// Закрываем сетевой сокет
		this->close(fd);
	/**
	 * Если операционной системой является Sun Solaris
	 */
	#elif __sun__
		// Структура параметров сетевого интерфейса
		struct ifconf ifc;
		// Структура сетевого интерфейса
		struct ifreq ifrc;
		// Создаём буфер для извлечения сетевых данных
		char buffer[IF_BUFFER_SIZE];
		// Заполняем нуляем наши буферы
		::memset(buffer, 0, sizeof(buffer));
		// Выделяем сокет для подключения
		const SOCKET fd = ::socket(family, SOCK_DGRAM, IPPROTO_IP);
		// Если файловый дескриптор не создан, выходим
		if(fd == INVALID_SOCKET){
			// Выводим сообщение об ошибке
			this->_log->print("Socket failed", log_t::flag_t::WARNING);
			// Выходим из функции
			return;
		}
		// Устанавливаем буфер для получения параметров сетевого интерфейса
		ifc.ifc_buf = buffer;
		// Устанавливаем максимальный размер буфера
		ifc.ifc_len = IF_BUFFER_SIZE;
		// Выполняем получение сетевых параметров
		if(::ioctl(fd, SIOCGIFCONF, &ifc) < 0){
			// Закрываем сетевой сокет
			this->close(fd);
			// Выводим сообщение об ошибке
			this->_log->print("IOCTL failed", log_t::flag_t::WARNING);
			// Выходим из функции
			return;
		}
		// Временный буфер MAC-адреса
		char hardware[18];
		// Получаем текущее значение итератора
		struct ifreq * i = ifc.ifc_req;
		// Получаем конечное значение итератора
		const struct ifreq * const end = (i + (ifc.ifc_len / sizeof(struct ifreq)));
		// Переходим по всем сетевым интерфейсам
		for(; i != end; ++i){
			// Копируем название сетевого интерфейса
			strcpy(ifrc.ifr_name, i->ifr_name);
			// Выполняем подключение к сокету
			if(::ioctl(fd, SIOCGIFFLAGS, &ifrc) == 0){
				// Проверяем сетевой интерфейс (не loopback)
				if(!(ifrc.ifr_flags & IFF_LOOPBACK)){
					// Извлекаем аппаратный адрес сетевого интерфейса
					if(::ioctl(fd, SIOCGIFHWADDR, &ifrc) == 0){
						// Создаём буфер MAC-адреса
						uint8_t mac[6];
						// Заполняем нуляем наши буферы
						::memset(hardware, 0, sizeof(hardware));
						// Выполняем копирование MAC-адреса
						::memcpy(mac, ifrc.ifr_addr.sa_data, 6);
						// Выполняем получение MAC-адреса
						::sprintf(hardware, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
						// Добавляем MAC-адрес в список сетевых интерфейсов
						this->_ifs.emplace(i->ifr_name, hardware);
					}
				}
			// Если подключение к сокету не удалось, выводим сообщение об ошибке
			} else this->_log->print("IOCTL failed", log_t::flag_t::WARNING);
		}
		// Закрываем сетевой сокет
		this->close(fd);
	/**
	 * Устанавливаем настройки для OS Windows
	 */
	#elif _WIN32 || _WIN64
		// Получаем размер буфера данных
		ULONG size = sizeof(IP_ADAPTER_ADDRESSES);
		// Выделяем память под буфер данных
		PIP_ADAPTER_ADDRESSES addr = reinterpret_cast <IP_ADAPTER_ADDRESSES *> (MALLOC(size));
		// Выполняем первоначальный вызов GetAdaptersAddresses, чтобы получить необходимый размер.
		if(GetAdaptersAddresses(family, GAA_FLAG_INCLUDE_PREFIX, nullptr, addr, &size) == ERROR_BUFFER_OVERFLOW){
			// Очищаем выделенную ранее память
			FREE(addr);
			// Выделяем ещё раз память для буфера данных
			addr = reinterpret_cast <IP_ADAPTER_ADDRESSES *> (MALLOC(size));
		}
		// Если буфер данных не существует
		if(addr == nullptr){
			// Выводим сообщение об ошибке
			this->_log->print("Memory allocation failed for IP_ADAPTER_ADDRESSES struct", log_t::flag_t::WARNING);
			// Выходим из функции
			return;
		}
		// Выполняем получения данных сетевого адаптера
		const DWORD result = GetAdaptersAddresses(family, GAA_FLAG_INCLUDE_PREFIX, nullptr, addr, &size);
		// Если данные сетевого адаптера считаны удачно
		if(result == NO_ERROR){
			// Временный буфер MAC-адреса
			char hardware[18];
			// В случае успеха выводим некоторую информацию из полученных данных.
			PIP_ADAPTER_ADDRESSES adapter = addr;
			// Выполняем обработку всех сетевых адаптеров
			while(adapter != nullptr){
				// Если MAC-адрес сетевой карты найден
				if(adapter->PhysicalAddressLength != 0){
					// Заполняем нуляем наши буферы
					::memset(hardware, 0, sizeof(hardware));
					// Выполняем получение MAC-адреса
					::sprintf(hardware, "%02X:%02X:%02X:%02X:%02X:%02X", adapter->PhysicalAddress[0], adapter->PhysicalAddress[1], adapter->PhysicalAddress[2], adapter->PhysicalAddress[3], adapter->PhysicalAddress[4], adapter->PhysicalAddress[5]);
					// Добавляем MAC-адрес в список сетевых интерфейсов
					this->_ifs.emplace(adapter->AdapterName, hardware);
				}
				// Выполняем смену итератора
				adapter = adapter->Next;
			}
		// Если данные адаптера прочитать не вышло
		} else {
			// Выводим сообщение об ошибке
			this->_log->print("Call to GetAdaptersAddresses failed with error: %d", log_t::flag_t::WARNING, result);
			// Если ошибка связана с отсутствием данных
			if(result == ERROR_NO_DATA)
				// Выводим сообщение об ошибке
				this->_log->print("No addresses were found for the requested parameters", log_t::flag_t::WARNING);
			// Если данные существуют
			else {
				// Создаём объект сообщения
				LPVOID message = nullptr;
				// Формируем генерарцию сообщения
				if(FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, result, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) & message, 0, nullptr)){
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::WARNING, message);
					// Очищаем выделенную память сообщения
					LocalFree(message);
				}
			}
		}
		// Очищаем выделенную ранее память
		FREE(addr);
	#endif
}
/**
 * Метод закрытие подключения
 * @param fd файловый дескриптор (сокет)
 */
void awh::IfNet::close(const int32_t fd) const noexcept {
	// Если файловый дескриптор подключён
	if(fd != INVALID_SOCKET){
		/**
		 * Для операционной системы OS Windows
		 */
		#if _WIN32 || _WIN64
			// Выполняем закрытие сокета
			::closesocket(fd);
		/**
		 * Для операционной системы не являющейся OS Windows
		 */
		#else
			// Выполняем закрытие сокета
			::close(fd);
		#endif
	}
}
/**
 * init Метод инициализации сбора информации
 */
void awh::IfNet::init() noexcept {
	// Очищаем ранее собранные данные
	this->clear();
	// Выполняем получение IPv4 адресов
	this->getIPAddresses(AF_INET);
	// Выполняем получение MAC-IPv4 адресов
	this->getHWAddresses(AF_INET);
	// Выполняем получение IPv6 адресов
	this->getIPAddresses(AF_INET6);
	// Выполняем получение MAC-IPv6 адресов
	this->getHWAddresses(AF_INET6);
}
/**
 * clear Метод очистки собранных данных
 */
void awh::IfNet::clear() noexcept {
	// Выполняем очистку списка MAC-адресов
	this->_ifs.clear();
	// Выполняем очистку списка IPv4 адресов
	this->_ips.clear();
	// Выполняем очистку списка IPv6 адресов
	this->_ips6.clear();
}
/**
 * Метод вывода списка MAC-адресов
 * @return список MAC-адресов
 */
const std::unordered_map <string, string> & awh::IfNet::hws() const noexcept {
	// Выводим список сетевых интерфейсов
	return this->_ifs;
}
/**
 * name Метод запроса названия сетевого интерфейса
 * @param eth идентификатор сетевого интерфейса
 * @return    название сетевого интерфейса
 */
string awh::IfNet::name(const string & eth) const noexcept {
	// Результат работы функции
	string result = eth;
	// Если сетевой интерфейс получен
	if(!eth.empty()){
		/**
		 * Для операционной системы OS Windows
		 */
		#if _WIN32 || _WIN64
			// Получаем размер буфера данных
			ULONG size = sizeof(IP_ADAPTER_INFO);
			// Выделяем память под буфер данных
			PIP_ADAPTER_INFO addr = reinterpret_cast <IP_ADAPTER_INFO *> (MALLOC(sizeof(IP_ADAPTER_INFO)));
			// Если буфер данных не существует
			if(addr == nullptr){
				// Выводим сообщение об ошибке
				this->_log->print("Error allocating memory needed to call GetAdaptersinfo", log_t::flag_t::WARNING);
				// Выходим из функции
				return result;
			}
			// Выполняем первоначальный вызов GetAdaptersInfo, чтобы получить необходимый размер
			if(GetAdaptersInfo(addr, &size) == ERROR_BUFFER_OVERFLOW){
				// Очищаем выделенную ранее память
				FREE(addr);
				// Выделяем ещё раз память для буфера данных
				addr = reinterpret_cast <IP_ADAPTER_INFO *> (MALLOC(size));
				// Если буфер данных не существует
				if(addr == nullptr){
					// Выводим сообщение об ошибке
					this->_log->print("Error allocating memory needed to call GetAdaptersinfo", log_t::flag_t::WARNING);
					// Выходим из функции
					return result;
				}
			}
			// Результат получения данных адаптера
			DWORD info = 0;
			// Получаем информацию о сетевом адаптере
			if((info = GetAdaptersInfo(addr, &size)) == NO_ERROR){
				// В случае успеха выводим некоторую информацию из полученных данных
				PIP_ADAPTER_INFO adapter = addr;
				// Выполняем обработку всех сетевых адаптеров
				while(adapter != nullptr){
					// Если сетевой адаптер найден
					if(strcmp(adapter->AdapterName, eth.c_str()) == 0){
						// Получаем название сетевого интерфейса
						result = adapter->Description;
						// Выходим из цикла
						break;
					}
					// Выполняем смену итератора
					adapter = adapter->Next;
				}
			// Выводим сообщение об ошибке
			} else this->_log->print("GetAdaptersInfo failed with error: %d", log_t::flag_t::WARNING, info);
			// Очищаем выделенную ранее память
			if(addr != nullptr)
				// Удаляем выделенную память
				FREE(addr);
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * mac Метод получения MAC-адреса по IP-адресу клиента
 * @param ip     адрес интернет-подключения клиента
 * @param family тип протокола интернета AF_INET или AF_INET6
 * @return       аппаратный адрес сетевого интерфейса клиента
 */
string awh::IfNet::mac(const string & ip, const int32_t family) const noexcept {
	// Результат работы функции
	string result = "";
	// Если IP-адрес передан
	if(!ip.empty()){
		/**
		 * Если операционной системой является MacOS X, FreeBSD, NetBSD или OpenBSD
		 */
		#if __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
			// Определяем тип протокола интернета
			switch(family){
				// Если запрашиваемый адрес IPv4
				case AF_INET: {
					// Создаём массив параметров сетевого интерфейса
					int32_t mib[6];
					// Размер буфера данных
					size_t size = 0;
					// Параметры итератора в буфере
					char * i = nullptr, * end = nullptr;
					// Объекты для работы с сетевым интерфейсом
					struct rt_msghdr * rtm      = nullptr;
					struct sockaddr_dl * sdl    = nullptr;
					struct sockaddr_inarp * sin = nullptr;
					// Устанавливаем парарметры сетевого интерфейса
					mib[0] = CTL_NET;
					mib[1] = PF_ROUTE;
					mib[2] = 0;
					mib[3] = family;
					mib[4] = NET_RT_FLAGS;
					/**
					 * Если операционной системой является NetBSD или OpenBSD
					 */
					#if __NetBSD__ || __OpenBSD__
						mib[5] = RTF_LLDATA;
					/**
					 * Если операционной системой является MacOS X или FreeBSD
					 */
					#else
						mib[5] = RTF_LLINFO;
					#endif
					// Выполняем получение размера буфера
					if(::sysctl(mib, 6, nullptr, &size, nullptr, 0) < 0){
						// Выводим сообщение об ошибке
						this->_log->print("Route sysctl estimate", log_t::flag_t::WARNING);
						// Выходим из функции
						return result;
					}
					// Создаём буфер данных сетевого интерфейса
					vector <char> buffer(size);
					// Выполняем получение данных сетевого интерфейса
					if(::sysctl(mib, 6, buffer.data(), &size, nullptr, 0) < 0){
						// Выводим сообщение об ошибке
						this->_log->print("Actual retrieval of routing table", log_t::flag_t::WARNING);
						// Выходим из функции
						return result;
					}
					// Получаем конечное значение итератора
					end = (buffer.data() + size);
					// Получаем числовое значение IP-адреса
					const uint32_t addr = inet_addr(ip.c_str());
					// Переходим по всем сетевым интерфейсам
					for(i = buffer.data(); i < end; i += rtm->rtm_msglen){
						// Получаем указатель сетевого интерфейса
						rtm = reinterpret_cast <struct rt_msghdr *> (i);
						// Получаем текущее значение активного подключения
						sin = reinterpret_cast <struct sockaddr_inarp *> (rtm + 1);
						// Получаем текущее значение аппаратного сетевого адреса
						sdl = reinterpret_cast <struct sockaddr_dl *> (sin + 1);
						// Если сетевой интерфейс отличается от IPv4 пропускаем
						if(sin->sin_family != family)
							// Выполняем пропуск
							continue;
						// Если искомый IP-адрес не совпадает, пропускаем
						if(addr != sin->sin_addr.s_addr)
							// Выполняем пропуск
							continue;
						// Если сетевой интерфейс получен
						if(sdl->sdl_alen > 0x0){
							// Выделяем память для MAC-адреса
							char hardware[18];
							// Заполняем нуляем наши буферы
							::memset(hardware, 0, sizeof(hardware));
							// Извлекаем MAC-адрес
							const uint8_t * cp = reinterpret_cast <uint8_t *> (LLADDR(sdl));
							// Выполняем получение MAC-адреса
							::sprintf(hardware, "%02X:%02X:%02X:%02X:%02X:%02X", cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]);
							// Получаем результат MAC-адреса
							result = hardware;
							// Выходим из цикла
							break;
						}
					}
				} break;
				// Если запрашиваемый адрес IPv6
				case AF_INET6: {
					// Создаём массив параметров сетевого интерфейса
					int32_t mib[6];
					// Размер буфера данных
					size_t size = 0;
					// Создаём объект подключения
					struct sockaddr_in6 addr;
					// Объекты для работы с сетевым интерфейсом
					struct rt_msghdr * rtm    = nullptr;
					struct sockaddr_dl * sdl  = nullptr;
					struct sockaddr_in6 * sin = nullptr;
					// Параметры итератора в буфере
					char * i = nullptr, * end = nullptr;
					// Устанавливаем парарметры сетевого интерфейса
					mib[0] = CTL_NET;
					mib[1] = PF_ROUTE;
					mib[2] = 0;
					mib[3] = family;
					mib[4] = NET_RT_FLAGS;
					/**
					 * Если операционной системой является NetBSD или OpenBSD
					 */
					#if __NetBSD__ || __OpenBSD__
						mib[5] = RTF_LLDATA;
					/**
					 * Если операционной системой является MacOS X или FreeBSD
					 */
					#else
						mib[5] = RTF_LLINFO;
					#endif
					// Заполняем нулями структуру объекта подключения
					::memset(&addr, 0, sizeof(addr));
					// Устанавливаем протокол интернета
					addr.sin6_family = family;
					// Указываем адрес IPv6 для сервера
					inet_pton(family, ip.c_str(), &addr.sin6_addr);
					// Выполняем получение размера буфера
					if(::sysctl(mib, 6, nullptr, &size, nullptr, 0) < 0){
						// Выводим сообщение об ошибке
						this->_log->print("Route sysctl estimate", log_t::flag_t::WARNING);
						// Выходим из функции
						return result;
					}
					// Создаём буфер данных сетевого интерфейса
					vector <char> buffer(size);
					// Выполняем получение данных сетевого интерфейса
					if(::sysctl(mib, 6, buffer.data(), &size, nullptr, 0) < 0){
						// Выводим сообщение об ошибке
						this->_log->print("Actual retrieval of routing table", log_t::flag_t::WARNING);
						// Выходим из функции
						return result;
					}
					// Получаем конечное значение итератора
					end = (buffer.data() + size);
					// Переходим по всем сетевым интерфейсам
					for(i = buffer.data(); i < end; i += rtm->rtm_msglen){
						// Получаем указатель сетевого интерфейса
						rtm = reinterpret_cast <struct rt_msghdr *> (i);
						// Если версия RTM протокола не соответствует, пропускаем
						if(rtm->rtm_version != RTM_VERSION)
							// Выполняем пропуск
							continue;
						// Получаем текущее значение активного подключения
						sin = reinterpret_cast <struct sockaddr_in6 *> (i + sizeof(rt_msghdr));
						/**
						 * Если мы работаем с KAME
						 */
						#ifdef __KAME__
							{
								// Получаем текущий адрес IPv6
								struct in6_addr * in6 = &sin->sin6_addr;
								// Проверяем вид интерфейса, если интерфейс локальный и скоуп-ID не установлен
								if((IN6_IS_ADDR_LINKLOCAL(in6) || IN6_IS_ADDR_MC_LINKLOCAL(in6)) && (sin->sin6_scope_id == 0)){
									// Принудительно устанавливаем скоуп-ID
									sin->sin6_scope_id = static_cast <uint32_t> (ntohs(* reinterpret_cast <uint16_t *> (&in6->s6_addr[2])));
									// Выполняем зануление третьего хексета
									(* reinterpret_cast <uint16_t *> (&in6->s6_addr[2])) = 0;
								}
							}
						#endif
						// Получаем текущее значение аппаратного сетевого адреса
						sdl = reinterpret_cast <struct sockaddr_dl *> (reinterpret_cast <char *> (sin) + ROUNDUP(sin->sin6_len));
						// Если версия сетевого протокола отличается от IPv4, то пропускаем
						if(sdl->sdl_family != AF_LINK)
							// Выполняем пропуск
							continue;
						/*
						// Если RTM не соответствует хосту, пропускаем
						if(!(rtm->rtm_flags & RTF_HOST))
							// Выполняем пропуск
							continue;
						*/
						// Проверяем соответствует ли IP-адрес - тому, что мы ищем
						if(!IN6_ARE_ADDR_EQUAL(&addr.sin6_addr, &sin->sin6_addr))
							// Выполняем пропуск
							continue;
						// Если сетевой интерфейс получен
						if(sdl->sdl_alen > 0x0){
							// Выделяем память для MAC-адреса
							char hardware[18];
							// Заполняем нуляем наши буферы
							::memset(hardware, 0, sizeof(hardware));
							// Извлекаем MAC-адрес
							const uint8_t * cp = reinterpret_cast <uint8_t *> (LLADDR(sdl));
							// Выполняем получение MAC-адреса
							::sprintf(hardware, "%02X:%02X:%02X:%02X:%02X:%02X", cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]);
							// Получаем результат MAC-адреса
							result = hardware;
							// Выходим из цикла
							break;
						}
					}
				} break;
			}
		/**
		 * Если операционной системой является Linux
		 */
		#elif __linux__
			// Определяем тип протокола интернета
			switch(family){
				// Если запрашиваемый адрес IPv4
				case AF_INET: {
					// Числовое значение IP-адреса
					uint32_t addr = 0;
					// Флаг найденнго MAC-адреса
					bool found = false;
					// Создаём объект MAC-адреса
					struct sockaddr mac;
					// Создаём объект подключения IPv4
					struct sockaddr_in sin;
					// Заполняем нулями структуру объекта MAC-адреса
					::memset(&mac, 0, sizeof(mac));
					// Заполняем нулями структуру объекта подключения IPv4
					::memset(&sin, 0, sizeof(sin));
					// Устанавливаем протокол интернета
					sin.sin_family = family;
					// Устанавливаем IP-адрес
					sin.sin_addr.s_addr = inet_addr(ip.c_str());
					// Выполняем копирование IP-адреса
					if(inet_pton(family, ip.c_str(), &sin.sin_addr) != 1){
						// Выводим сообщение об ошибке
						this->_log->print("Invalid IPv4 address", log_t::flag_t::WARNING);
						// Выходим из функции
						return result;
					}
					// Получаем числовое значение IP-адреса
					addr = sin.sin_addr.s_addr;
					// Объект работы с сетевой картой
					struct ifaddrs * headIfa = nullptr;
					// Считываем данные сетевой карты
					if(getifaddrs(&headIfa) == -1){
						// Очищаем объект сетевой карты
						freeifaddrs(headIfa);
						// Выводим сообщение об ошибке
						this->_log->print("Invalid ifaddrs", log_t::flag_t::WARNING);
						// Выходим из функции
						return result;
					}
					// Выделяем сокет для подключения
					const SOCKET fd = ::socket(family, SOCK_DGRAM, IPPROTO_IP);
					// Если файловый дескриптор не создан, выходим
					if(fd == INVALID_SOCKET){
						// Выводим сообщение об ошибке
						this->_log->print("Socket failed", log_t::flag_t::WARNING);
						// Выходим из функции
						return result;
					}
					// Сетевые адреса в цифровом виде
					uint32_t ifaddr = 0, netmask = 0, dstaddr = 0;
					// Переходим по всем сетевым интерфейсам
					for(struct ifaddrs * ifa = headIfa; ifa != nullptr; ifa = ifa->ifa_next){
						// Пропускаем локальные сетевые интерфейсы
						if(0 == strncmp(ifa->ifa_name, "lo", 2))
							// Выполняем пропуск
							continue;
						// Пропускаем локальные сетевые интерфейсы
						if(nullptr != strchr(ifa->ifa_name, ':'))
							// Выполняем пропуск
							continue;
						// Если сетевой интерфейс не соответствует, пропускаем
						if((ifa->ifa_addr == nullptr) || (ifa->ifa_flags & IFF_POINTOPOINT) || (ifa->ifa_addr->sa_family != family))
							// Выполняем пропуск
							continue;
						// Получаем адреса сетевого интерфейса в цифровом виде
						ifaddr  = reinterpret_cast <struct sockaddr_in *> (ifa->ifa_addr)->sin_addr.s_addr;
						netmask = reinterpret_cast <struct sockaddr_in *> (ifa->ifa_netmask)->sin_addr.s_addr;
						dstaddr = reinterpret_cast <struct sockaddr_in *> (ifa->ifa_dstaddr)->sin_addr.s_addr;
						// Если искомый IP-адрес найден
						if(((netmask == 0xFFFFFFFF) && (addr == dstaddr)) || ((ifaddr & netmask) == (addr & netmask))){
							// Искомый IP-адрес соответствует данному серверу
							if(ifaddr == addr){
								// Структура сетевого интерфейса
								struct ifreq ifreq;
								// Копируем название сетевого интерфейса
								strncpy(ifreq.ifr_name, ifa->ifa_name, IFNAMSIZ);
								// Извлекаем аппаратный адрес сетевого интерфейса
								if((found = (::ioctl(fd, SIOCGIFHWADDR, &ifreq) != -1)))
									// Копируем данные MAC-адреса
									::memcpy(&mac, &ifreq.ifr_hwaddr, sizeof(mac));
								// Выходим из цикла
								break;
							}
							// Создаём структуру сетевого интерфейса
							struct arpreq arpreq;
							// Заполняем структуру сетевого интерфейса нулями
							::memset(&arpreq, 0, sizeof(arpreq));
							// Устанавливаем искомый IP-адрес
							::memcpy(&(arpreq.arp_pa), &sin, sizeof(sin));
							// Копируем название сетевого интерфейса
							strncpy(arpreq.arp_dev, ifa->ifa_name, IFNAMSIZ);
							// Подключаем сетевой интерфейс к сокету
							if(::ioctl(fd, SIOCGARP, &arpreq) == -1){
								// Пропускаем если ошибка не значительная
								if(AWH_ERROR() == ENXIO)
									// Выполняем пропуск
									continue;
								// Выходим из цикла
								else break;
							}
							// Если мы нашли наш MAC-адрес
							if((found = (arpreq.arp_flags & ATF_COM))){
								// Копируем данные MAC-адреса
								::memcpy(&mac, &arpreq.arp_ha, sizeof(mac));
								// Выходим из цикла
								break;
							}
						}
					}
					// Очищаем объект сетевой карты
					freeifaddrs(headIfa);
					// Если MAC-адрес получен
					if(found){
						// Выделяем память для MAC-адреса
						char hardware[18];
						// Заполняем нуляем наши буферы
						::memset(hardware, 0, sizeof(hardware));
						// Извлекаем MAC-адрес
						const uint8_t * cp = reinterpret_cast <uint8_t *> (mac.sa_data);
						// Выполняем получение MAC-адреса
						::sprintf(hardware, "%02X:%02X:%02X:%02X:%02X:%02X", cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]);
						// Получаем результат MAC-адреса
						result = hardware;
					}
					// Закрываем сетевой сокет
					this->close(fd);
				} break;
				// Если запрашиваемый адрес IPv6
				case AF_INET6: {
					// Создаём объект подключения IPv6
					struct sockaddr_in6 sin;
					// Заполняем нулями структуру объекта подключения IPv6
					::memset(&sin, 0, sizeof(sin));
					// Устанавливаем протокол интернета
					sin.sin6_family = family;
					// Выполняем копирование IP-адреса
					if(inet_pton(family, ip.c_str(), &sin.sin6_addr) != 1){
						// Выводим сообщение об ошибке
						this->_log->print("Invalid IPv6 address", log_t::flag_t::WARNING);
						// Выходим из функции
						return result;
					}
					// Создаём буфер для получения текущего IPv6 адреса
					char target[INET6_ADDRSTRLEN];
					// Заполняем структуру нулями текущего адреса
					::memset(target, 0, INET6_ADDRSTRLEN);
					// Заполняем буфер данными текущего адреса IPv6
					inet_ntop(family, &sin.sin6_addr, target, INET6_ADDRSTRLEN);
					// Объект работы с сетевой картой
					struct ifaddrs * headIfa = nullptr;
					// Считываем данные сетевой карты
					if(getifaddrs(&headIfa) == -1){
						// Очищаем объект сетевой карты
						freeifaddrs(headIfa);
						// Выводим сообщение об ошибке
						this->_log->print("Invalid ifaddrs", log_t::flag_t::WARNING);
						// Выходим из функции
						return result;
					}
					// Выделяем сокет для подключения
					const SOCKET fd = ::socket(AF_INET6, SOCK_RAW, IPPROTO_IPV6);
					// Если файловый дескриптор не создан, выходим
					if(fd == INVALID_SOCKET){
						// Выводим сообщение об ошибке
						this->_log->print("Socket failed", log_t::flag_t::WARNING);
						// Выходим из функции
						return result;
					}
					// Выделяем память для MAC-адреса
					char hardware[18];
					// Создаём буфер для получения текущего ip адреса
					char host[INET6_ADDRSTRLEN];
					// Переходим по всем сетевым интерфейсам
					for(struct ifaddrs * ifa = headIfa; ifa != nullptr; ifa = ifa->ifa_next){
						// Пропускаем локальные сетевые интерфейсы
						if(0 == strncmp(ifa->ifa_name, "lo", 2))
							// Выполняем пропуск
							continue;
						// Пропускаем локальные сетевые интерфейсы
						if(nullptr != strchr(ifa->ifa_name, ':'))
							// Выполняем пропуск
							continue;
						// Если сетевой интерфейс не соответствует, пропускаем
						if((ifa->ifa_addr == nullptr) || (ifa->ifa_flags & IFF_POINTOPOINT) || (ifa->ifa_addr->sa_family != family))
							// Выполняем пропуск
							continue;
						// Заполняем структуры нулями сетевых адресов
						::memset(host, 0, INET6_ADDRSTRLEN);
						// Заполняем буфер данными сетевых адресов IPv6
						inet_ntop(family, &reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_addr)->sin6_addr, host, INET6_ADDRSTRLEN);
						// Искомый IP-адрес соответствует данному серверу
						if(::memcmp(host, target, INET6_ADDRSTRLEN) == 0){
							// Структура сетевого интерфейса
							struct ifreq ifreq;
							// Заполняем нуляем наши буферы
							::memset(hardware, 0, sizeof(hardware));
							// Копируем название сетевого интерфейса
							strncpy(ifreq.ifr_name, ifa->ifa_name, IFNAMSIZ);
							// Извлекаем аппаратный адрес сетевого интерфейса
							if(::ioctl(fd, SIOCGIFHWADDR, &ifreq) != -1){
								// Создаём объект MAC-адреса
								struct sockaddr mac;
								// Заполняем нулями структуру объекта MAC-адреса
								::memset(&mac, 0, sizeof(mac));
								// Копируем данные MAC-адреса
								::memcpy(&mac, &ifreq.ifr_hwaddr, sizeof(mac));
								// Извлекаем MAC-адрес
								const uint8_t * cp = reinterpret_cast <uint8_t *> (mac.sa_data);
								// Выполняем получение MAC-адреса
								::sprintf(hardware, "%02X:%02X:%02X:%02X:%02X:%02X", cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]);
								// Получаем результат MAC-адреса
								result = hardware;
							// Если извлечь MAC-адрес не вышло, извлекаем его напрямую
							} else {
								// Создаём буфер для MAC-адреса
								uint8_t mac[6];
								// Извлекаем данные MAC-адреса
								mac[0] = reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_addr)->sin6_addr.s6_addr[8] ^ 0x02;
								mac[1] = reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_addr)->sin6_addr.s6_addr[9];
								mac[2] = reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_addr)->sin6_addr.s6_addr[10];
								mac[3] = reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_addr)->sin6_addr.s6_addr[13];
								mac[4] = reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_addr)->sin6_addr.s6_addr[14];
								mac[5] = reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_addr)->sin6_addr.s6_addr[15];
								// Выполняем получение MAC-адреса
								::sprintf(hardware, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
								// Получаем результат MAC-адреса
								result = hardware;
							}
							// Выходим из цикла
							break;
						}
					}
					// Очищаем объект сетевой карты
					freeifaddrs(headIfa);
					// Закрываем сетевой сокет
					this->close(fd);
				} break;
			}
		/**
		 * Если операционной системой является Sun Solaris
		 */
		#elif __sun__
			// Определяем тип протокола интернета
			switch(family){
				// Если запрашиваемый адрес IPv4
				case AF_INET: {
					// Числовое значение IP-адреса
					uint32_t addr = 0;
					// Флаг найденнго MAC-адреса
					bool found = false;
					// Создаём объект MAC-адреса
					struct sockaddr mac;
					// Создаём объект подключения IPv4
					struct sockaddr_in sin;
					// Заполняем нулями структуру объекта MAC-адреса
					::memset(&mac, 0, sizeof(mac));
					// Заполняем нулями структуру объекта подключения IPv4
					::memset(&sin, 0, sizeof(sin));
					// Устанавливаем протокол интернета
					sin.sin_family = family;
					// Устанавливаем IP-адрес
					sin.sin_addr.s_addr = inet_addr(ip.c_str());
					// Выполняем копирование IP-адреса
					if(inet_pton(family, ip.c_str(), &sin.sin_addr) != 1){
						// Выводим сообщение об ошибке
						this->_log->print("Invalid IPv4 address", log_t::flag_t::WARNING);
						// Выходим из функции
						return result;
					}
					// Получаем числовое значение IP-адреса
					addr = sin.sin_addr.s_addr;
					// Объект работы с сетевой картой
					struct ifaddrs * headIfa = nullptr;
					// Считываем данные сетевой карты
					if(getifaddrs(&headIfa) == -1){
						// Очищаем объект сетевой карты
						freeifaddrs(headIfa);
						// Выводим сообщение об ошибке
						this->_log->print("Invalid ifaddrs", log_t::flag_t::WARNING);
						// Выходим из функции
						return result;
					}
					// Выделяем сокет для подключения
					const SOCKET fd = ::socket(family, SOCK_DGRAM, IPPROTO_IP);
					// Если файловый дескриптор не создан, выходим
					if(fd == INVALID_SOCKET){
						// Выводим сообщение об ошибке
						this->_log->print("Socket failed", log_t::flag_t::WARNING);
						// Выходим из функции
						return result;
					}
					// Сетевые адреса в цифровом виде
					uint32_t ifaddr = 0, netmask = 0, dstaddr = 0;
					// Переходим по всем сетевым интерфейсам
					for(struct ifaddrs * ifa = headIfa; ifa != nullptr; ifa = ifa->ifa_next){
						// Пропускаем локальные сетевые интерфейсы
						if(0 == strncmp(ifa->ifa_name, "lo", 2))
							// Выполняем пропуск
							continue;
						// Пропускаем локальные сетевые интерфейсы
						if(nullptr != strchr(ifa->ifa_name, ':'))
							// Выполняем пропуск
							continue;
						// Если сетевой интерфейс не соответствует, пропускаем
						if((ifa->ifa_addr == nullptr) || (ifa->ifa_flags & IFF_POINTOPOINT) || (ifa->ifa_addr->sa_family != family))
							// Выполняем пропуск
							continue;
						// Получаем адреса сетевого интерфейса в цифровом виде
						ifaddr  = reinterpret_cast <struct sockaddr_in *> (ifa->ifa_addr)->sin_addr.s_addr;
						netmask = reinterpret_cast <struct sockaddr_in *> (ifa->ifa_netmask)->sin_addr.s_addr;
						dstaddr = reinterpret_cast <struct sockaddr_in *> (ifa->ifa_dstaddr)->sin_addr.s_addr;
						// Если искомый IP-адрес найден
						if(((netmask == 0xFFFFFFFF) && (addr == dstaddr)) || ((ifaddr & netmask) == (addr & netmask))){
							// Искомый IP-адрес соответствует данному серверу
							if(ifaddr == addr){
								// Структура сетевого интерфейса
								struct ifreq ifreq;
								// Копируем название сетевого интерфейса
								strncpy(ifreq.ifr_name, ifa->ifa_name, IFNAMSIZ);
								// Извлекаем аппаратный адрес сетевого интерфейса
								if((found = (::ioctl(fd, SIOCGIFHWADDR, &ifreq) != -1)))
									// Копируем данные MAC-адреса
									::memcpy(&mac, &ifreq.ifr_addr, sizeof(mac));
								// Выходим из цикла
								break;
							}
							// Создаём структуру сетевого интерфейса
							struct arpreq arpreq;
							// Заполняем структуру сетевого интерфейса нулями
							::memset(&arpreq, 0, sizeof(arpreq));
							// Устанавливаем искомый IP-адрес
							::memcpy(&(arpreq.arp_pa), &sin, sizeof(sin));
							// Подключаем сетевой интерфейс к сокету
							if(::ioctl(fd, SIOCGARP, &arpreq) == -1){
								// Пропускаем если ошибка не значительная
								if(AWH_ERROR() == ENXIO)
									// Выполняем пропуск
									continue;
								// Выходим из цикла
								else break;
							}
							// Если мы нашли наш MAC-адрес
							if((found = (arpreq.arp_flags & ATF_COM))){
								// Копируем данные MAC-адреса
								::memcpy(&mac, &arpreq.arp_ha, sizeof(mac));
								// Выходим из цикла
								break;
							}
						}
					}
					// Очищаем объект сетевой карты
					freeifaddrs(headIfa);
					// Если MAC-адрес получен
					if(found){
						// Выделяем память для MAC-адреса
						char hardware[18];
						// Заполняем нуляем наши буферы
						::memset(hardware, 0, sizeof(hardware));
						// Извлекаем MAC-адрес
						const uint8_t * cp = reinterpret_cast <uint8_t *> (mac.sa_data);
						// Выполняем получение MAC-адреса
						::sprintf(hardware, "%02X:%02X:%02X:%02X:%02X:%02X", cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]);
						// Получаем результат MAC-адреса
						result = hardware;
					}
					// Закрываем сетевой сокет
					this->close(fd);
				} break;
				// Если запрашиваемый адрес IPv6
				case AF_INET6: {
					// Создаём объект подключения IPv6
					struct sockaddr_in6 sin;
					// Заполняем нулями структуру объекта подключения IPv6
					::memset(&sin, 0, sizeof(sin));
					// Устанавливаем протокол интернета
					sin.sin6_family = family;
					// Выполняем копирование IP-адреса
					if(inet_pton(family, ip.c_str(), &sin.sin6_addr) != 1){
						// Выводим сообщение об ошибке
						this->_log->print("Invalid IPv6 address", log_t::flag_t::WARNING);
						// Выходим из функции
						return result;
					}
					// Создаём буфер для получения текущего IPv6 адреса
					char target[INET6_ADDRSTRLEN];
					// Заполняем структуру нулями текущего адреса
					::memset(target, 0, INET6_ADDRSTRLEN);
					// Заполняем буфер данными текущего адреса IPv6
					inet_ntop(family, &sin.sin6_addr, target, INET6_ADDRSTRLEN);
					// Объект работы с сетевой картой
					struct ifaddrs * headIfa = nullptr;
					// Считываем данные сетевой карты
					if(getifaddrs(&headIfa) == -1){
						// Очищаем объект сетевой карты
						freeifaddrs(headIfa);
						// Выводим сообщение об ошибке
						this->_log->print("Invalid ifaddrs", log_t::flag_t::WARNING);
						// Выходим из функции
						return result;
					}
					// Выделяем сокет для подключения
					const SOCKET fd = ::socket(AF_INET6, SOCK_RAW, IPPROTO_IPV6);
					// Если файловый дескриптор не создан, выходим
					if(fd == INVALID_SOCKET){
						// Выводим сообщение об ошибке
						this->_log->print("Socket failed", log_t::flag_t::WARNING);
						// Выходим из функции
						return result;
					}
					// Выделяем память для MAC-адреса
					char hardware[18];
					// Создаём буфер для получения текущего ip адреса
					char host[INET6_ADDRSTRLEN];
					// Переходим по всем сетевым интерфейсам
					for(struct ifaddrs * ifa = headIfa; ifa != nullptr; ifa = ifa->ifa_next){
						// Пропускаем локальные сетевые интерфейсы
						if(0 == strncmp(ifa->ifa_name, "lo", 2))
							// Выполняем пропуск
							continue;
						// Пропускаем локальные сетевые интерфейсы
						if(nullptr != strchr(ifa->ifa_name, ':'))
							// Выполняем пропуск
							continue;
						// Если сетевой интерфейс не соответствует, пропускаем
						if((ifa->ifa_addr == nullptr) || (ifa->ifa_flags & IFF_POINTOPOINT) || (ifa->ifa_addr->sa_family != family))
							// Выполняем пропуск
							continue;
						// Заполняем структуры нулями сетевых адресов
						::memset(host, 0, INET6_ADDRSTRLEN);
						// Заполняем буфер данными сетевых адресов IPv6
						inet_ntop(family, &reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_addr)->sin6_addr, host, INET6_ADDRSTRLEN);
						// Искомый IP-адрес соответствует данному серверу
						if(::memcmp(host, target, INET6_ADDRSTRLEN) == 0){
							// Структура сетевого интерфейса
							struct ifreq ifreq;
							// Заполняем нуляем наши буферы
							::memset(hardware, 0, sizeof(hardware));
							// Копируем название сетевого интерфейса
							strncpy(ifreq.ifr_name, ifa->ifa_name, IFNAMSIZ);
							// Извлекаем аппаратный адрес сетевого интерфейса
							if(::ioctl(fd, SIOCGIFHWADDR, &ifreq) != -1){
								// Создаём объект MAC-адреса
								struct sockaddr mac;
								// Заполняем нулями структуру объекта MAC-адреса
								::memset(&mac, 0, sizeof(mac));
								// Копируем данные MAC-адреса
								::memcpy(&mac, &ifreq.ifr_addr, sizeof(mac));
								// Извлекаем MAC-адрес
								const uint8_t * cp = reinterpret_cast <uint8_t *> (mac.sa_data);
								// Выполняем получение MAC-адреса
								::sprintf(hardware, "%02X:%02X:%02X:%02X:%02X:%02X", cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]);
								// Получаем результат MAC-адреса
								result = hardware;
							// Если извлечь MAC-адрес не вышло, извлекаем его напрямую
							} else {
								// Создаём буфер для MAC-адреса
								uint8_t mac[6];
								// Извлекаем данные MAC-адреса
								mac[0] = reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_addr)->sin6_addr.s6_addr[8] ^ 0x02;
								mac[1] = reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_addr)->sin6_addr.s6_addr[9];
								mac[2] = reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_addr)->sin6_addr.s6_addr[10];
								mac[3] = reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_addr)->sin6_addr.s6_addr[13];
								mac[4] = reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_addr)->sin6_addr.s6_addr[14];
								mac[5] = reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_addr)->sin6_addr.s6_addr[15];
								// Выполняем получение MAC-адреса
								::sprintf(hardware, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
								// Получаем результат MAC-адреса
								result = hardware;
							}
							// Выходим из цикла
							break;
						}
					}
					// Очищаем объект сетевой карты
					freeifaddrs(headIfa);
					// Закрываем сетевой сокет
					this->close(fd);
				} break;
			}
		/**
		 * Устанавливаем настройки для OS Windows
		 */
		#elif _WIN32 || _WIN64
			// Определяем тип протокола интернета
			switch(family){
				// Если запрашиваемый адрес IPv4
				case AF_INET: {
					// Размер буфера данных
					ULONG size = 6;
					// Буфер данных
					ULONG buffer[2];
					// Объект IP-адреса назначения
					struct in_addr destip;
					// Устанавливаем IP-адрес назначения
					destip.s_addr = inet_addr(ip.c_str());
					// Отправляем запрос на указанный адрес
					SendARP(static_cast <IPAddr> (destip.S_un.S_addr), 0, buffer, &size);
					// Если MAC-адрес получен
					if(size > 0){
						// Выделяем память для MAC-адреса
						char hardware[18];
						// Заполняем нуляем наши буферы
						::memset(hardware, 0, sizeof(hardware));
						// Получаем данные MAC-адреса
						BYTE * mac = reinterpret_cast <BYTE *> (&buffer);
						// Выполняем получение MAC-адреса
						::sprintf(hardware, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
						// Получаем результат MAC-адреса
						result = hardware;
					}
				} break;
				// Если запрашиваемый адрес IPv6
				case AF_INET6: {
					// Получаем объект таблицы подключений
					PMIB_IPNET_TABLE2 pipTable = nullptr;
					// Считываем данные таблицы подключений
					const u_long status = GetIpNetTable2(AF_INET6, &pipTable);
					// Если данные таблицы не получены
					if(status != NO_ERROR){
						// Выводим сообщение об ошибке
						this->_log->print("GetIpNetTable for IPv4 table returned error: %ld", log_t::flag_t::WARNING, status);
						// Выходим из функции
						return result;
					}
					// Если список подключений получен
					if(pipTable->NumEntries > 0){
						// Создаём объект подключения IPv6
						struct sockaddr_in6 sin;
						// Заполняем нулями структуру объекта подключения IPv6
						::memset(&sin, 0, sizeof(sin));
						// Устанавливаем протокол интернета
						sin.sin6_family = family;
						// Выполняем копирование IP-адреса
						if(inet_pton(family, ip.c_str(), &sin.sin6_addr) != 1){
							// Выводим сообщение об ошибке
							this->_log->print("Invalid IPv6 address", log_t::flag_t::WARNING);
							// Выходим из функции
							return result;
						}
						// Создаём буфер для получения текущего ip адреса
						char host[INET6_ADDRSTRLEN];
						// Создаём буфер для получения текущего IPv6 адреса
						char target[INET6_ADDRSTRLEN];
						// Заполняем структуру нулями текущего адреса
						::memset(target, 0, INET6_ADDRSTRLEN);
						// Заполняем буфер данными текущего адреса IPv6
						inet_ntop(family, &sin.sin6_addr, target, INET6_ADDRSTRLEN);
						// Переходим по всему списку подключений
						for(uint32_t i = 0; static_cast <uint32_t> (i) < pipTable->NumEntries; i++){
							// Заполняем нуляем наши буферы
							::memset(host, 0, INET6_ADDRSTRLEN);
							// Запрашиваем данные ip адреса
							inet_ntop(AF_INET6, reinterpret_cast <void *> (&pipTable->Table[i].Address.Ipv6.sin6_addr), host, INET6_ADDRSTRLEN);
							// Если искомый IP-адрес найден
							if(::memcmp(target, host, INET6_ADDRSTRLEN) == 0){
								// Если MAC-адрес получен
								if(pipTable->Table[i].PhysicalAddressLength > 0){
									// Выделяем память для MAC-адреса
									char hardware[18];
									// Заполняем нуляем наши буферы
									::memset(hardware, 0, sizeof(hardware));
									// Получаем данные MAC-адреса
									BYTE * mac = reinterpret_cast <BYTE *> (pipTable->Table[i].PhysicalAddress);
									// Выполняем получение MAC-адреса
									::sprintf(hardware, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
									// Получаем результат MAC-адреса
									result = hardware;
								}
								// Выходим из цикла
								break;
							}
						}
					}
				} break;
			}
		#endif
	}
	// Если MAC-адрес не получен
	if(result.empty()){
		// Имя сетевого интерфейса
		string ifrName = "";
		// Выполняем обновление списка IP-адресов
		const_cast <ifnet_t *> (this)->getIPAddresses(family);
		// Выполняем обновление списка MAC-адресов
		const_cast <ifnet_t *> (this)->getHWAddresses(family);
		// Определяем тип протокола интернета
		switch(family){
			// Если протокол интернета IPv4
			case AF_INET: {
				// Получаем IP-адрес в числовом виде
				uint32_t addr = inet_addr(ip.c_str());
				// Выполняем перебор всех локальных IP-адресов
				for(auto & item : this->_ips){
					// Если IP-адрес соответствует
					if(inet_addr(item.second.c_str()) == addr){
						// Запоминаем название сетевого интерфейса
						ifrName = item.first;
						// Выходим из цикла
						break;
					}
				}
			} break;
			// Если протокол интернета IPv6
			case AF_INET6: {
				// Создаём объект подключения
				struct sockaddr_in6 addr1, addr2;
				// Заполняем нулями структуру объекта подключения
				::memset(&addr1, 0, sizeof(addr1));
				// Устанавливаем протокол интернета
				addr1.sin6_family = family;
				// Указываем адрес IPv6 для сервера
				inet_pton(family, ip.c_str(), &addr1.sin6_addr);
				// Выполняем перебор всех локальных IP-адресов
				for(auto & item : this->_ips6){
					// Заполняем нулями структуру объекта подключения
					::memset(&addr2, 0, sizeof(addr2));
					// Устанавливаем протокол интернета
					addr2.sin6_family = family;
					// Указываем адрес IPv6 для сервера
					inet_pton(family, item.second.c_str(), &addr2.sin6_addr);
					// Если IP-адрес соответствует
					if(IN6_ARE_ADDR_EQUAL(&addr1.sin6_addr, &addr2.sin6_addr)){
						// Запоминаем название сетевого интерфейса
						ifrName = item.first;
						// Выходим из цикла
						break;
					}
				}
			} break;
		}
		// Если название сетевого интерфейса получено
		if(!ifrName.empty()){
			// Выполняем поиск MAC-адреса
			auto i = this->_ifs.find(ifrName);
			// Если MAC-адрес получен
			if(i != this->_ifs.end())
				// Выполняем получение результата
				result = i->second;
		}
	}
	// Выводим результат
	return result;
}
/**
 * mac Метод определения мак адреса клиента
 * @param sin    объект подключения
 * @param family тип протокола интернета AF_INET или AF_INET6
 * @return       данные мак адреса
 */
string awh::IfNet::mac(struct sockaddr * sin, const int32_t family) const noexcept {
	// Результат работы функции
	string result = "";
	// Если данные переданы
	if(sin != nullptr){
		// Выделяем память для MAC-адреса
		char hardware[18];
		// Заполняем нуляем наши буферы
		::memset(hardware, 0, sizeof(hardware));
		// Определяем тип интернет протокола
		switch(family){
			// Если это IPv4
			case AF_INET: {
				// Извлекаем MAC-адрес
				const uint8_t * cp = reinterpret_cast <uint8_t *> (sin->sa_data);
				// Выполняем получение MAC-адреса
				::sprintf(hardware, "%02X:%02X:%02X:%02X:%02X:%02X", cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]);
				// Выводим данные мак адреса
				result = hardware;
			} break;
			// Если это IPv6
			case AF_INET6: {
				// Создаём буфер для MAC-адреса
				uint8_t mac[6];
				// Извлекаем данные MAC-адреса
				mac[0] = reinterpret_cast <struct sockaddr_in6 *> (sin)->sin6_addr.s6_addr[8] ^ 0x02;
				mac[1] = reinterpret_cast <struct sockaddr_in6 *> (sin)->sin6_addr.s6_addr[9];
				mac[2] = reinterpret_cast <struct sockaddr_in6 *> (sin)->sin6_addr.s6_addr[10];
				mac[3] = reinterpret_cast <struct sockaddr_in6 *> (sin)->sin6_addr.s6_addr[13];
				mac[4] = reinterpret_cast <struct sockaddr_in6 *> (sin)->sin6_addr.s6_addr[14];
				mac[5] = reinterpret_cast <struct sockaddr_in6 *> (sin)->sin6_addr.s6_addr[15];
				// Выполняем получение MAC-адреса
				::sprintf(hardware, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
				// Получаем результат MAC-адреса
				result = hardware;
			} break;
		}
	}
	// Выводим результат
	return result;
}
/**
 * ip Метод получения основного IP-адреса на сервере
 * @param family тип протокола интернета AF_INET или AF_INET6
 */
string awh::IfNet::ip(const int32_t family) const noexcept {
	// Результат рарботы функции
	string result = "";
	// Создаем сокет
	const SOCKET fd = ::socket(family, SOCK_DGRAM, IPPROTO_IP);
	// Если сокет создан
	if(fd != INVALID_SOCKET){
		// Определяем тип интернет протокола
		switch(family){
			// Если это IPv4
			case AF_INET: {
				// Создаем список dns серверов
				vector <string> dns = IPV4_RESOLVER;
				// Создаем структуру подключения сервера
				struct sockaddr_in serv;
				// Обнуляем структуру подключения
				::memset(&serv, 0, sizeof(serv));
				// Указываем тип сетевого подключения IPv4
				serv.sin_family = family;
				// Устанавливаем порт DNS сервера
				serv.sin_port = htons(53);
				// Указываем адрес DNS сервера
				serv.sin_addr.s_addr = inet_addr(dns.front().c_str());
				// Выполняем подключение к серверу
				int32_t conn = ::connect(fd, reinterpret_cast <const sockaddr *> (&serv), sizeof(serv));
				// Если подключение удачное
				if(conn > -1){
					// Создаем структуру имени
					struct sockaddr_in name;
					// Размер структуры
					socklen_t size = sizeof(name);
					// Запрашиваем имя сокета
					conn = ::getsockname(fd, reinterpret_cast <sockaddr *> (&name), &size);
					// Если ошибки нет
					if(conn > -1){
						// Создаем буфер для получения ip адреса
						char buffer[INET_ADDRSTRLEN];
						// Обнуляем массив
						::memset(buffer, 0, sizeof(buffer));
						// Запрашиваем данные ip адреса
						inet_ntop(family, &name.sin_addr, buffer, sizeof(buffer));
						// Выводим результат
						result = buffer;
					}
				}
			} break;
			// Если это IPv6
			case AF_INET6: {
				// Создаем список dns серверов
				vector <string> dns = IPV6_RESOLVER;
				// Создаем структуру подключения сервера
				struct sockaddr_in6 serv;
				// Обнуляем структуру подключения
				::memset(&serv, 0, sizeof(serv));
				// Указываем тип сетевого подключения IPv4
				serv.sin6_family = family;
				// Устанавливаем порт DNS сервера
				serv.sin6_port = htons(53);
				// Указываем адреса
				inet_pton(family, dns.front().c_str(), &serv.sin6_addr);
				// Выполняем подключение к серверу
				int32_t conn = ::connect(fd, reinterpret_cast <const sockaddr *> (&serv), sizeof(serv));
				// Если подключение удачное
				if(conn > -1){
					// Создаем структуру имени
					struct sockaddr_in6 name;
					// Размер структуры
					socklen_t size = sizeof(name);
					// Запрашиваем имя сокета
					conn = ::getsockname(fd, reinterpret_cast <sockaddr *> (&name), &size);
					// Если ошибки нет
					if(conn > -1){
						// Создаем буфер для получения ip адреса
						char buffer[INET6_ADDRSTRLEN];
						// Обнуляем массив
						::memset(buffer, 0, sizeof(buffer));
						// Запрашиваем данные ip адреса
						inet_ntop(family, &name.sin6_addr, buffer, sizeof(buffer));
						// Выводим результат
						result = buffer;
					}
				}
			} break;
		}
		// Закрываем сетевой сокет
		this->close(fd);
	}
	// Сообщаем что ничего не найдено
	return result;
}
/**
 * ip Метод получения IP-адреса из подключения
 * @param sin    объект подключения
 * @param family тип интернет протокола
 * @return       данные ip адреса
 */
string awh::IfNet::ip(struct sockaddr * sin, const int32_t family) const noexcept {
	// Результат работы функции
	string result = "";
	// Если данные переданы
	if(sin != nullptr){
		// Определяем тип интернет протокола
		switch(family){
			// Если это IPv4
			case AF_INET: {
				// Создаем буфер для получения ip адреса
				char buffer[INET_ADDRSTRLEN];
				// Заполняем структуру нулями
				::memset(buffer, 0, sizeof(buffer));
				// Получаем данные адреса
				struct sockaddr_in * s = reinterpret_cast <struct sockaddr_in *> (sin);
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
				::memset(buffer, 0, sizeof(buffer));
				// Получаем данные адреса
				struct sockaddr_in6 * s = reinterpret_cast <struct sockaddr_in6 *> (sin);
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
 * ip Метод вывода IP-адреса соответствующего сетевому интерфейсу
 * @param eth    идентификатор сетевого интерфейса
 * @param family тип протокола интернета AF_INET или AF_INET6
 * @return       IP-адрес соответствующий сетевому интерфейсу
 */
const string & awh::IfNet::ip(const string & eth, const int32_t family) const noexcept {
	// Результат работы функции
	static const string result = "";
	// Если сетевой интерфейс получен
	if(!eth.empty()){
		// Определяем тип интернет адреса
		switch(family){
			// Если мы обрабатываем IPv4
			case AF_INET: {
				// Выполняем поиск сетевого интерфейса
				auto i = this->_ips.find(eth);
				// Если сетевой интерфейс получен, выводим IP-адрес
				if(i != this->_ips.end())
					// Выводим полученный результат
					return i->second;
			} break;
			// Если мы обрабатываем IPv6
			case AF_INET6: {
				// Выполняем поиск сетевого интерфейса
				auto i = this->_ips6.find(eth);
				// Если сетевой интерфейс получен, выводим IP-адрес
				if(i != this->_ips6.end())
					// Выводим полученный результат
					return i->second;
			} break;
		}
	}
	// Выводим результат
	return result;
}
