/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <if.hpp>

/**
 * Устанавливаем настройки для *Nix подобных систем
 */
#if !defined(_WIN32) && !defined(_WIN64)

/**
 * getIPAddresses Метод извлечения IP адресов
 */
void awh::IfNet::getIPAddresses() noexcept {
/**
 * Если операционной системой является MacOS X или FreeBSD
 */
#if __APPLE__ || __MACH__ || __FreeBSD__
	// Структура параметров сетевого интерфейса
	struct ifconf ifc;
	// Структура сетевого интерфейса
	struct ifreq ifrc;
	// Создаём буфер для извлечения сетевых данных
	char buffer[IF_BUFFER_SIZE];
	// Заполняем нуляем наши буферы
	memset(buffer, 0, sizeof(buffer));
	// Выделяем сокет для подключения
	int fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	// Если файловый дескриптор не создан, выходим
	if(fd < 0){
		// Выводим сообщение об ошибке
		this->log->print("%s", log_t::flag_t::WARNING, "socket failed");
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
		::close(fd);
		// Выводим сообщение об ошибке
		this->log->print("%s", log_t::flag_t::WARNING, "ioctl failed");
		// Выходим из функции
		return;
	}
	// Создаём указатели для перехода
	char * cptr = nullptr;
	// Выполняем перебор всех сетевых интерфейсов
	for(char * ptr = buffer; ptr < (buffer + ifc.ifc_len);){
		// Выполняем получение сетевого интерфейса
		struct ifreq * ifr = (struct ifreq *) ptr;
		// Выполняем смещение указателя в цикле
		ptr += (sizeof(ifr->ifr_name) + max(sizeof(struct sockaddr), (size_t) ifr->ifr_addr.sa_len));
		// Если сетевой интерфейс отличается от IPv4 пропускаем
		if((ifr->ifr_addr.sa_family != AF_INET) && (ifr->ifr_addr.sa_family != AF_INET6)) continue;
		// Если в имени сетевого интерфейса найден разделитель, пропускаем его
		if((cptr = (char *) strchr(ifr->ifr_name, ':')) != nullptr) (* cptr) = 0;
		// Запоминаем текущее значение указателя
		ifrc = (* ifr);
		// Считываем флаги для сетевого интерфейса
		::ioctl(fd, SIOCGIFFLAGS, &ifrc);
		// Если флаги не соответствуют, пропускаем
		if((ifrc.ifr_flags & IFF_UP) == 0) continue;
		// Определяем тип интернет адреса
		switch(ifr->ifr_addr.sa_family){
			// Если мы обрабатываем IPv4
			case AF_INET:
				// Устанавливаем сетевой интерфейс в список
				this->ips.emplace(ifr->ifr_name, inet_ntoa(((struct sockaddr_in *) &ifr->ifr_addr)->sin_addr));
			break;
			// Если мы обрабатываем IPv6
			case AF_INET6: {
				// Создаем буфер для получения ip адреса
				char buffer[INET6_ADDRSTRLEN];
				// Заполняем нуляем наши буферы
				memset(buffer, 0, sizeof(buffer));
				// Запрашиваем данные ip адреса
				inet_ntop(AF_INET6, (void *) &((struct sockaddr_in6 *) &ifr->ifr_addr)->sin6_addr, buffer, sizeof(buffer));
				// Устанавливаем сетевой интерфейс в список
				this->ips6.emplace(ifr->ifr_name, buffer);
			} break;
		}
	}
	// Закрываем сетевой сокет
	::close(fd);
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
	memset(buffer, 0, sizeof(buffer));
	// Выделяем сокет для подключения
	int fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	// Если файловый дескриптор не создан, выходим
	if(fd < 0){
		// Выводим сообщение об ошибке
		this->log->print("%s", log_t::flag_t::WARNING, "socket failed");
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
		::close(fd);
		// Выводим сообщение об ошибке
		this->log->print("%s", log_t::flag_t::WARNING, "ioctl failed");
		// Выходим из функции
		return;
	}
	// Создаём указатели для перехода
	char * cptr = nullptr;
	// Получаем текущее значение итератора
	struct ifreq * it = ifc.ifc_req;
	// Получаем конечное значение итератора
	const struct ifreq * const end = (it + (ifc.ifc_len / sizeof(struct ifreq)));
	// Переходим по всем сетевым интерфейсам
	for(; it != end; ++it){
		// Если сетевой интерфейс отличается от IPv4 пропускаем
		if((it->ifr_addr.sa_family != AF_INET) && (it->ifr_addr.sa_family != AF_INET6)) continue;
		// Если в имени сетевого интерфейса найден разделитель, пропускаем его
		if((cptr = (char *) strchr(it->ifr_name, ':')) != nullptr) (* cptr) = 0;
		// Запоминаем текущее значение указателя
		ifrc = (* it);
		// Считываем флаги для сетевого интерфейса
		::ioctl(fd, SIOCGIFFLAGS, &ifrc);
		// Если флаги не соответствуют, пропускаем
		if((ifrc.ifr_flags & IFF_UP) == 0) continue;
		// Определяем тип интернет адреса
		switch(it->ifr_addr.sa_family){
			// Если мы обрабатываем IPv4
			case AF_INET:
				// Устанавливаем сетевой интерфейс в список
				this->ips.emplace(it->ifr_name, inet_ntoa(((struct sockaddr_in *) &it->ifr_addr)->sin_addr));
			break;
			// Если мы обрабатываем IPv6
			case AF_INET6: {
				// Создаем буфер для получения ip адреса
				char buffer[INET6_ADDRSTRLEN];
				// Заполняем нуляем наши буферы
				memset(buffer, 0, sizeof(buffer));
				// Запрашиваем данные ip адреса
				inet_ntop(AF_INET6, (void *) &((struct sockaddr_in6 *) &it->ifr_addr)->sin6_addr, buffer, sizeof(buffer));
				// Устанавливаем сетевой интерфейс в список
				this->ips6.emplace(it->ifr_name, buffer);
			} break;
		}
	}
	// Закрываем сетевой сокет
	::close(fd);
#endif
}
/**
 * getHWAddresses Метод извлечения MAC адресов
 */
void awh::IfNet::getHWAddresses() noexcept {
/**
 * Если операционной системой является MacOS X или FreeBSD
 */
#if __APPLE__ || __MACH__ || __FreeBSD__
	// Структура параметров сетевого интерфейса
	struct ifconf ifc;
	// Временный буфер MAC адреса
	char temp[80];
	// Создаём буфер для извлечения сетевых данных
	char buffer[IF_BUFFER_SIZE];
	// Заполняем нуляем наши буферы
	memset(buffer, 0, sizeof(buffer));
	// Выделяем сокет для подключения
	int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
	// Если файловый дескриптор не создан, выходим
	if(fd < 0){
		// Выводим сообщение об ошибке
		this->log->print("%s", log_t::flag_t::WARNING, "socket failed");
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
		::close(fd);
		// Выводим сообщение об ошибке
		this->log->print("%s", log_t::flag_t::WARNING, "ioctl failed");
		// Выходим из функции
		return;
	}
	// Выполняем смещение в буфере
	char * cplim = (buffer + ifc.ifc_len);
	// Выполняем перебор всех сетевых интерфейсов
	for(char * cp = buffer; cp < cplim;){
		// Выполняем получение сетевого интерфейса
		struct ifreq * ifr = (struct ifreq *) cp;
		// Нам нужен реальный сетевой интерфейс
		if(ifr->ifr_addr.sa_family == AF_LINK){
			// Заполняем нуляем наши буферы
			memset(temp, 0, sizeof(temp));
			// Инициализируем все октеты
			int a = 0, b = 0, c = 0, d = 0, e = 0, f = 0;
			// Получаем сетевой интерфейс
			struct sockaddr_dl * sdl = (struct sockaddr_dl *) &ifr->ifr_addr;
			// Копируем MAC адрес из сетевого интерфейса
			strcpy(temp, (char *) ether_ntoa((struct ether_addr*) LLADDR(sdl)));
			// Устанавливаем сетевые октеты
			sscanf(temp, "%x:%x:%x:%x:%x:%x", &a, &b, &c, &d, &e, &f);
			// Формируем MAC адрес удобный для вывода
			sprintf(temp, "%02X:%02X:%02X:%02X:%02X:%02X", a, b, c, d, e, f);
			// Добавляем MAC адрес в список сетевых интерфейсов
			this->ifs.emplace(ifr->ifr_name, temp);
		}
		// Выполняем смещение указателя в цикле
		cp += (sizeof(ifr->ifr_name) + max(sizeof(ifr->ifr_addr), (size_t) ifr->ifr_addr.sa_len));
	}
	// Закрываем сетевой сокет
	::close(fd);
/**
 * Если операционной системой является Linux
 */
#elif __linux__
	// Структура параметров сетевого интерфейса
	struct ifconf ifc;
	// Структура сетевого интерфейса
	struct ifreq ifrc;
	// Временный буфер MAC адреса
	char temp[18];
	// Создаём буфер для извлечения сетевых данных
	char buffer[IF_BUFFER_SIZE];
	// Заполняем нуляем наши буферы
	memset(buffer, 0, sizeof(buffer));
	// Выделяем сокет для подключения
	int fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	// Если файловый дескриптор не создан, выходим
	if(fd < 0){
		// Выводим сообщение об ошибке
		this->log->print("%s", log_t::flag_t::WARNING, "socket failed");
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
		::close(fd);
		// Выводим сообщение об ошибке
		this->log->print("%s", log_t::flag_t::WARNING, "ioctl failed");
		// Выходим из функции
		return;
	}
	// Получаем текущее значение итератора
	struct ifreq * it = ifc.ifc_req;
	// Получаем конечное значение итератора
	const struct ifreq * const end = (it + (ifc.ifc_len / sizeof(struct ifreq)));
	// Переходим по всем сетевым интерфейсам
	for(; it != end; ++it){
		// Копируем название сетевого интерфейса
		strcpy(ifrc.ifr_name, it->ifr_name);
		// Выполняем подключение к сокету
		if(::ioctl(fd, SIOCGIFFLAGS, &ifrc) == 0){
			// Проверяем сетевой интерфейс (не loopback)
			if(!(ifrc.ifr_flags & IFF_LOOPBACK)){
				// Извлекаем аппаратный адрес сетевого интерфейса
				if(::ioctl(fd, SIOCGIFHWADDR, &ifrc) == 0){
					// Создаём буфер MAC-адреса
					u_char mac[6];
					// Заполняем нуляем наши буферы
					memset(temp, 0, sizeof(temp));
					// Выполняем копирование MAC-адреса
					memcpy(mac, ifrc.ifr_hwaddr.sa_data, 6);
					// Выполняем получение MAC-адреса
					sprintf(temp, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
					// Добавляем MAC адрес в список сетевых интерфейсов
					this->ifs.emplace(it->ifr_name, temp);
				}
			}
		// Если подключение к сокету не удалось, выводим сообщение об ошибке
		} else this->log->print("%s", log_t::flag_t::WARNING, "ioctl failed");
	}
	// Закрываем сетевой сокет
	::close(fd);
#endif
}
/**
 * init Метод инициализации сбора информации
 */
void awh::IfNet::init() noexcept {
	// Очищаем ранее собранные данные
	this->clear();
	// Выполняем получение IP адресов
	this->getIPAddresses();
	// Выполняем получение MAC адресов
	this->getHWAddresses();
}
/**
 * clear Метод очистки собранных данных
 */
void awh::IfNet::clear() noexcept {
	// Выполняем очистку списка MAC адресов
	this->ifs.clear();
	// Выполняем очистку списка IPv4 адресов
	this->ips.clear();
	// Выполняем очистку списка IPv6 адресов
	this->ips6.clear();
}
/**
 * Метод вывода списка MAC адресов
 * @return список MAC адресов
 */
const unordered_map <string, string> & awh::IfNet::hws() const noexcept {
	// Выводим список сетевых интерфейсов
	return this->ifs;
}
/**
 * mac Метод получения MAC адреса по IP адресу клиента
 * @param ip     адрес интернет-подключения клиента
 * @param family тип протокола интернета AF_INET или AF_INET6
 * @return       аппаратный адрес сетевого интерфейса клиента
 */
const string awh::IfNet::mac(const string & ip, const int family) const noexcept {
	// Результат работы функции
	string result = "";
	// Если IP адрес передан
	if(!ip.empty()){
/**
 * Если операционной системой является MacOS X или FreeBSD
 */
#if __APPLE__ || __MACH__ || __FreeBSD__
		// Создаём массив параметров сетевого интерфейса
		int mib[6];
		// Размер буфера данных
		size_t size = 0;
		// Параметры итератора в буфере
		char * it = nullptr, * end = nullptr;
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
		mib[5] = RTF_LLINFO;
		// Выполняем получение размера буфера
		if(::sysctl(mib, 6, nullptr, &size, nullptr, 0) < 0){
			// Выводим сообщение об ошибке
			this->log->print("%s", log_t::flag_t::WARNING, "route sysctl estimate");
			// Выходим из функции
			return result;
		}
		// Создаём буфер данных сетевого интерфейса
		vector <char> buffer(size);
		// Выполняем получение данных сетевого интерфейса
		if(::sysctl(mib, 6, buffer.data(), &size, nullptr, 0) < 0){
			// Выводим сообщение об ошибке
			this->log->print("%s", log_t::flag_t::WARNING, "actual retrieval of routing table");
			// Выходим из функции
			return result;
		}
		// Получаем конечное значение итератора
		end = (buffer.data() + size);
		// Создаем буфер для получения проверяемого IPv6 адреса
		char host[INET6_ADDRSTRLEN];
		// Создаем буфер для получения текущего IPv6 адреса
		char target[INET6_ADDRSTRLEN];
		// Если запрашиваемый адрес IPv6
		if(family == AF_INET6){
			// Создаём объект подключения
			struct sockaddr_in6 sin;
			// Заполняем нулями структуру объекта подключения
			memset(&sin, 0, sizeof(sin));
			// Заполняем структуру нулями текущего адреса
			memset(target, 0, sizeof(target));
			// Устанавливаем протокол интернета
			sin.sin6_family = family;
			// Указываем адрес IPv6 для сервера
			inet_pton(family, ip.c_str(), &sin.sin6_addr);
			// Заполняем буфер данными текущего адреса IPv6
			inet_ntop(family, &sin.sin6_addr, target, sizeof(target));
		}
		// Получаем числовое значение IP адреса
		const uint32_t addr = (family == AF_INET ? inet_addr(ip.c_str()) : 0);
		// Переходим по всем сетевым интерфейсам
		for(it = buffer.data(); it < end; it += rtm->rtm_msglen){
			// Получаем указатель сетевого интерфейса
			rtm = (struct rt_msghdr *) it;
			// Получаем текущее значение активного подключения
			sin = (struct sockaddr_inarp *) (rtm + 1);
			// Получаем текущее значение аппаратного сетевого адреса
			sdl = (struct sockaddr_dl *) (sin + 1);
			// Если запрашиваемый адрес IPv6
			if(family == AF_INET6){
				// Если сетевой интерфейс отличается от IPv4 пропускаем
				if(reinterpret_cast <struct sockaddr_in6 *> (sin)->sin6_family != family) continue;
				// Заполняем структуру нулями проверяемого хоста
				memset(host, 0, sizeof(host));
				// Копируем полученные данные
				inet_ntop(family, &reinterpret_cast <struct sockaddr_in6 *> (sin)->sin6_addr, host, sizeof(host));
				// Если искомый IP адрес не совпадает, пропускаем
				if(strcmp(host, target) != 0) continue;
			// Если запрашиваемый адрес IPv4
			} else if(family == AF_INET) {
				// Если сетевой интерфейс отличается от IPv4 пропускаем
				if(sin->sin_family != family) continue;
				// Если искомый IP адрес не совпадает, пропускаем
				if(addr != sin->sin_addr.s_addr) continue;
			}
			// Если сетевой интерфейс получен
			if(sdl->sdl_alen > 0){
				// Выделяем память для MAC адреса
				char temp[18];
				// Заполняем нуляем наши буферы
				memset(temp, 0, sizeof(temp));
				// Извлекаем MAC адрес
				const u_char * cp = (u_char *) LLADDR(sdl);
				// Выполняем получение MAC-адреса
				sprintf(temp, "%02X:%02X:%02X:%02X:%02X:%02X", cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]);
				// Получаем результат MAC адреса
				result = move(temp);
				// Выходим из цикла
				break;
			}
		}
/**
 * Если операционной системой является Linux
 */
#elif __linux__
		// Числовое значение IP адреса
		uint32_t addr = 0;
		// Флаг найденнго MAC адреса
		bool found = false;
		// Создаём объект MAC-адреса
		struct sockaddr mac;
		// Создаём объект подключения IPv4
		struct sockaddr_in sin;
		// Создаём объект подключения IPv6
		struct sockaddr_in6 sin6;
		// Создаем буфер для получения текущего IPv6 адреса
		char target[INET6_ADDRSTRLEN];
		// Заполняем нулями структуру объекта MAC-адреса
		memset(&mac, 0, sizeof(mac));
		// Если запрашиваемый адрес IPv6
		if(family == AF_INET6){
			// Заполняем нулями структуру объекта подключения IPv6
			memset(&sin6, 0, sizeof(sin6));
			// Устанавливаем протокол интернета
			sin6.sin6_family = family;
			// Выполняем копирование IP адреса
			if(inet_pton(family, ip.c_str(), &sin6.sin6_addr) != 1){
				// Выводим сообщение об ошибке
				this->log->print("%s", log_t::flag_t::WARNING, "invalid IPv6 address");
				// Выходим из функции
				return result;
			}
			// Заполняем структуру нулями текущего адреса
			memset(target, 0, sizeof(target));
			// Заполняем буфер данными текущего адреса IPv6
			inet_ntop(family, &sin6.sin6_addr, target, sizeof(target));
		// Если запрашиваемый адрес IPv4
		} else if(family == AF_INET) {
			// Заполняем нулями структуру объекта подключения IPv4
			memset(&sin, 0, sizeof(sin));
			// Устанавливаем протокол интернета
			sin.sin_family = family;
			// Устанавливаем IP адрес
			sin.sin_addr.s_addr = inet_addr(ip.c_str());
			// Выполняем копирование IP адреса
			if(inet_pton(family, ip.c_str(), &sin.sin_addr) != 1){
				// Выводим сообщение об ошибке
				this->log->print("%s", log_t::flag_t::WARNING, "invalid IPv4 address");
				// Выходим из функции
				return result;
			}
			// Получаем числовое значение IP адреса
			addr = sin.sin_addr.s_addr;
		}
		// Объект работы с сетевой картой
		struct ifaddrs * headIfa = nullptr;
		// Считываем данные сетевой карты
		if(getifaddrs(&headIfa) == -1){
			// Очищаем объект сетевой карты
			freeifaddrs(headIfa);
			// Выводим сообщение об ошибке
			this->log->print("%s", log_t::flag_t::WARNING, "invalid ifaddrs");
			// Выходим из функции
			return result;
		}
		// Выделяем сокет для подключения
		int fd = ::socket(family, SOCK_DGRAM, IPPROTO_IP);
		// Если файловый дескриптор не создан, выходим
		if(fd < 0){
			// Выводим сообщение об ошибке
			this->log->print("%s", log_t::flag_t::WARNING, "socket failed");
			// Выходим из функции
			return result;
		}
		// Если запрашиваемый адрес IPv6
		if(family == AF_INET6){
			// Создаем буферы сетевых адресов
			char ifaddr[INET6_ADDRSTRLEN];
			char dstaddr[INET6_ADDRSTRLEN];
			// Переходим по всем сетевым интерфейсам
			for(struct ifaddrs * ifa = headIfa; ifa != nullptr; ifa = ifa->ifa_next){
				// Если сетевой интерфейс не соответствует, пропускаем
				if((ifa->ifa_addr == nullptr) || (ifa->ifa_flags & IFF_POINTOPOINT) || (ifa->ifa_addr->sa_family != family)) continue;
				// Заполняем структуры нулями сетевых адресов
				memset(ifaddr,  0, sizeof(ifaddr));
				memset(dstaddr, 0, sizeof(dstaddr));
				// Заполняем буфер данными сетевых адресов IPv6
				inet_ntop(family, &((struct sockaddr_in6 *) ifa->ifa_addr)->sin6_addr, ifaddr, sizeof(ifaddr));
				inet_ntop(family, &((struct sockaddr_in6 *) ifa->ifa_dstaddr)->sin6_addr, dstaddr, sizeof(dstaddr));
				// Если искомый IP адрес найден
				if(strcmp(dstaddr, target) == 0){
					// Искомый IP адрес соответствует данному серверу
					if(strcmp(ifaddr, target) == 0){
						// Структура сетевого интерфейса
						struct ifreq ifreq;
						// Копируем название сетевого интерфейса
						strncpy(ifreq.ifr_name, ifa->ifa_name, IFNAMSIZ);
						// Извлекаем аппаратный адрес сетевого интерфейса
						if((found = (::ioctl(fd, SIOCGIFHWADDR, &ifreq) != -1)))
							// Копируем данные MAC адреса
							memcpy(&mac, &ifreq.ifr_hwaddr, sizeof(mac));
						// Выходим из цикла
						break;
					}
					// Создаём структуру сетевого интерфейса
					struct arpreq arpreq;
					// Заполняем структуру сетевого интерфейса нулями
					memset(&arpreq, 0, sizeof(arpreq));
					// Устанавливаем искомый IP адрес
					memcpy(&(arpreq.arp_pa), &sin, sizeof(sin));
					// Копируем название сетевого интерфейса
					strncpy(arpreq.arp_dev, ifa->ifa_name, IFNAMSIZ);
					// Подключаем сетевой интерфейс к сокету
					if(::ioctl(fd, SIOCGARP, &arpreq) == -1){
						// Пропускаем если ошибка не значительная
						if(errno == ENXIO) continue;
						// Выходим из цикла
						else break;
					}
					// Если мы нашли наш MAC адрес
					if((found = (arpreq.arp_flags & ATF_COM))){
						// Копируем данные MAC адреса
						memcpy(&mac, &arpreq.arp_ha, sizeof(mac));
						// Выходим из цикла
						break;
					}
				}
			}
		// Если запрашиваемый адрес IPv4
		} else if(family == AF_INET) {
			// Сетевые адреса в цифровом виде
			uint32_t ifaddr = 0, netmask = 0, dstaddr = 0;
			// Переходим по всем сетевым интерфейсам
			for(struct ifaddrs * ifa = headIfa; ifa != nullptr; ifa = ifa->ifa_next){
				// Если сетевой интерфейс не соответствует, пропускаем
				if((ifa->ifa_addr == nullptr) || (ifa->ifa_flags & IFF_POINTOPOINT) || (ifa->ifa_addr->sa_family != family)) continue;
				// Получаем адреса сетевого интерфейса в цифровом виде
				ifaddr  = ((struct sockaddr_in *) ifa->ifa_addr)->sin_addr.s_addr;
				netmask = ((struct sockaddr_in *) ifa->ifa_netmask)->sin_addr.s_addr;
				dstaddr = ((struct sockaddr_in *) ifa->ifa_dstaddr)->sin_addr.s_addr;
				// Если искомый IP адрес найден
				if(((netmask == 0xFFFFFFFF) && (addr == dstaddr)) || ((ifaddr & netmask) == (addr & netmask))){
					// Искомый IP адрес соответствует данному серверу
					if(ifaddr == addr){
						// Структура сетевого интерфейса
						struct ifreq ifreq;
						// Копируем название сетевого интерфейса
						strncpy(ifreq.ifr_name, ifa->ifa_name, IFNAMSIZ);
						// Извлекаем аппаратный адрес сетевого интерфейса
						if((found = (::ioctl(fd, SIOCGIFHWADDR, &ifreq) != -1)))
							// Копируем данные MAC адреса
							memcpy(&mac, &ifreq.ifr_hwaddr, sizeof(mac));
						// Выходим из цикла
						break;
					}
					// Создаём структуру сетевого интерфейса
					struct arpreq arpreq;
					// Заполняем структуру сетевого интерфейса нулями
					memset(&arpreq, 0, sizeof(arpreq));
					// Устанавливаем искомый IP адрес
					memcpy(&(arpreq.arp_pa), &sin, sizeof(sin));
					// Копируем название сетевого интерфейса
					strncpy(arpreq.arp_dev, ifa->ifa_name, IFNAMSIZ);
					// Подключаем сетевой интерфейс к сокету
					if(::ioctl(fd, SIOCGARP, &arpreq) == -1){
						// Пропускаем если ошибка не значительная
						if(errno == ENXIO) continue;
						// Выходим из цикла
						else break;
					}
					// Если мы нашли наш MAC адрес
					if((found = (arpreq.arp_flags & ATF_COM))){
						// Копируем данные MAC адреса
						memcpy(&mac, &arpreq.arp_ha, sizeof(mac));
						// Выходим из цикла
						break;
					}
				}
			}
		}
		// Очищаем объект сетевой карты
		freeifaddrs(headIfa);
		// Если MAC адрес получен
		if(found){
			// Выделяем память для MAC адреса
			char temp[18];
			// Заполняем нуляем наши буферы
			memset(temp, 0, sizeof(temp));
			// Извлекаем MAC адрес
			const u_char * cp = (u_char *) mac.sa_data;
			// Выполняем получение MAC-адреса
			sprintf(temp, "%02X:%02X:%02X:%02X:%02X:%02X", cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]);
			// Получаем результат MAC адреса
			result = move(temp);
		}
		// Закрываем сетевой сокет
		::close(fd);
#endif
	}
	// Выводим результат
	return result;
}
/**
 * Метод вывода IP адреса соответствующего сетевому интерфейсу
 * @param eth    название сетевого интерфейса
 * @param family тип протокола интернета AF_INET или AF_INET6
 * @return       IP адрес соответствующий сетевому интерфейсу
 */
const string & awh::IfNet::ip(const string & eth, const int family) const noexcept {
	// Результат работы функции
	static const string result = "";
	// Если сетевой интерфейс получен
	if(!eth.empty()){
		// Определяем тип интернет адреса
		switch(family){
			// Если мы обрабатываем IPv4
			case AF_INET: {
				// Выполняем поиск сетевого интерфейса
				auto it = this->ips.find(eth);
				// Если сетевой интерфейс получен, выводим IP адрес
				if(it != this->ips.end()) return it->second;
			} break;
			// Если мы обрабатываем IPv6
			case AF_INET6: {
				// Выполняем поиск сетевого интерфейса
				auto it = this->ips6.find(eth);
				// Если сетевой интерфейс получен, выводим IP адрес
				if(it != this->ips6.end()) return it->second;
			} break;
		}
	}
	// Выводим результат
	return result;
}

#endif // NOT WINDOWS
