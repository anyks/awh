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
 * getIPAddresses Метод извлечения IP адресов
 * @param family тип протокола интернета AF_INET или AF_INET6
 */
void awh::IfNet::getIPAddresses(const int family) noexcept {
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
	int fd = ::socket(family, SOCK_DGRAM, IPPROTO_IP);
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
		if(ifr->ifr_addr.sa_family != family) continue;
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
				memset(buffer, 0, INET6_ADDRSTRLEN);
				// Запрашиваем данные ip адреса
				inet_ntop(AF_INET6, (void *) &((struct sockaddr_in6 *) &ifr->ifr_addr)->sin6_addr, buffer, INET6_ADDRSTRLEN);
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
	int fd = ::socket(family, SOCK_DGRAM, IPPROTO_IP);
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
		if(it->ifr_addr.sa_family != family) continue;
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
				memset(buffer, 0, INET6_ADDRSTRLEN);
				// Запрашиваем данные ip адреса
				inet_ntop(AF_INET6, (void *) &((struct sockaddr_in6 *) &it->ifr_addr)->sin6_addr, buffer, INET6_ADDRSTRLEN);
				// Устанавливаем сетевой интерфейс в список
				this->ips6.emplace(it->ifr_name, buffer);
			} break;
		}
	}
	// Закрываем сетевой сокет
	::close(fd);
/**
 * Устанавливаем настройки для OS Windows
 */
#elif defined(_WIN32) || defined(_WIN64)
	// Получаем размер буфера данных
	ULONG size = sizeof(IP_ADAPTER_ADDRESSES);
	// Выделяем память под буфер данных
	PIP_ADAPTER_ADDRESSES addr = (IP_ADAPTER_ADDRESSES *) MALLOC(size);
	// Выполняем первоначальный вызов GetAdaptersAddresses, чтобы получить необходимый размер.
	if(GetAdaptersAddresses(family, GAA_FLAG_INCLUDE_PREFIX, nullptr, addr, &size) == ERROR_BUFFER_OVERFLOW){
		// Очищаем выделенную ранее память
		FREE(addr);
		// Выделяем ещё раз память для буфера данных
		addr = (IP_ADAPTER_ADDRESSES *) MALLOC(size);
	}
	// Если буфер данных не существует
	if(addr == nullptr){
		// Выводим сообщение об ошибке
		this->log->print("%s", log_t::flag_t::WARNING, "memory allocation failed for IP_ADAPTER_ADDRESSES struct");
		// Выходим из функции
		return;
	}
	// Выполняем получения данных сетевого адаптера
	const DWORD result = GetAdaptersAddresses(family, GAA_FLAG_INCLUDE_PREFIX, nullptr, addr, &size);
	// Если данные сетевого адаптера считаны удачно
	if(result == NO_ERROR){
		// Создаем буфер для получения IPv4 адреса
		char ipv4[INET_ADDRSTRLEN];
		// Создаем буфер для получения IPv6 адреса
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
					memset(ipv4, 0, INET_ADDRSTRLEN);
					// Получаем буфер IP адреса
					SOCKADDR_IN * sin = reinterpret_cast <SOCKADDR_IN *> (host->Address.lpSockaddr);
					// Копируем данные IP адреса в буфер
					inet_ntop(AF_INET, &(sin->sin_addr), ipv4, INET_ADDRSTRLEN);
					// Устанавливаем сетевой интерфейс в список
					this->ips.emplace(adapter->AdapterName, ipv4);
				} break;
				// Если мы обрабатываем IPv6
				case AF_INET6: {
					// Зануляем буфер данных
					memset(ipv6, 0, INET6_ADDRSTRLEN);
					// Получаем буфер IP адреса
					SOCKADDR_IN6 * sin = reinterpret_cast <SOCKADDR_IN6 *> (host->Address.lpSockaddr);
					// Получаем буфер IP адреса
					inet_ntop(AF_INET6, &(sin->sin6_addr), ipv6, INET6_ADDRSTRLEN);
					// Устанавливаем сетевой интерфейс в список
					this->ips6.emplace(adapter->AdapterName, ipv6);
				} break;
			}
			// Выполняем смену итератора
			adapter = adapter->Next;
		}
	// Если данные адаптера прочитать не вышло
	} else {
		// Выводим сообщение об ошибке
		this->log->print("сall to GetAdaptersAddresses failed with error: %d", log_t::flag_t::WARNING, result);
		// Если ошибка связана с отсутствием данных
		if(result == ERROR_NO_DATA)
			// Выводим сообщение об ошибке
			this->log->print("%s", log_t::flag_t::WARNING, "no addresses were found for the requested parameters");
		// Если данные существуют
		else {
			// Создаём объект сообщения
			LPVOID message = nullptr;
			// Формируем генерарцию сообщения
			if(FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, result, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) & message, 0, nullptr)){
				// Выводим сообщение об ошибке
				this->log->print("%s", log_t::flag_t::WARNING, message);
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
 * getHWAddresses Метод извлечения MAC адресов
 * @param family тип протокола интернета AF_INET или AF_INET6
 */
void awh::IfNet::getHWAddresses(const int family) noexcept {
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
	int fd = ::socket(family, SOCK_DGRAM, 0);
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
	int fd = ::socket(family, SOCK_DGRAM, IPPROTO_IP);
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
					// Создаём буфер MAC адреса
					u_char mac[6];
					// Заполняем нуляем наши буферы
					memset(temp, 0, sizeof(temp));
					// Выполняем копирование MAC адреса
					memcpy(mac, ifrc.ifr_hwaddr.sa_data, 6);
					// Выполняем получение MAC адреса
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
/**
 * Устанавливаем настройки для OS Windows
 */
#elif defined(_WIN32) || defined(_WIN64)
	// Получаем размер буфера данных
	ULONG size = sizeof(IP_ADAPTER_ADDRESSES);
	// Выделяем память под буфер данных
	PIP_ADAPTER_ADDRESSES addr = (IP_ADAPTER_ADDRESSES *) MALLOC(size);
	// Выполняем первоначальный вызов GetAdaptersAddresses, чтобы получить необходимый размер.
	if(GetAdaptersAddresses(family, GAA_FLAG_INCLUDE_PREFIX, nullptr, addr, &size) == ERROR_BUFFER_OVERFLOW){
		// Очищаем выделенную ранее память
		FREE(addr);
		// Выделяем ещё раз память для буфера данных
		addr = (IP_ADAPTER_ADDRESSES *) MALLOC(size);
	}
	// Если буфер данных не существует
	if(addr == nullptr){
		// Выводим сообщение об ошибке
		this->log->print("%s", log_t::flag_t::WARNING, "memory allocation failed for IP_ADAPTER_ADDRESSES struct");
		// Выходим из функции
		return;
	}
	// Выполняем получения данных сетевого адаптера
	const DWORD result = GetAdaptersAddresses(family, GAA_FLAG_INCLUDE_PREFIX, nullptr, addr, &size);
	// Если данные сетевого адаптера считаны удачно
	if(result == NO_ERROR){
		// Временный буфер MAC адреса
		char temp[18];
		// В случае успеха выводим некоторую информацию из полученных данных.
		PIP_ADAPTER_ADDRESSES adapter = addr;
		// Выполняем обработку всех сетевых адаптеров
		while(adapter != nullptr){
			// Если MAC адрес сетевой карты найден
			if(adapter->PhysicalAddressLength != 0){
				// Заполняем нуляем наши буферы
				memset(temp, 0, sizeof(temp));
				// Выполняем получение MAC адреса
				sprintf(temp, "%02X:%02X:%02X:%02X:%02X:%02X", adapter->PhysicalAddress[0], adapter->PhysicalAddress[1], adapter->PhysicalAddress[2], adapter->PhysicalAddress[3], adapter->PhysicalAddress[4], adapter->PhysicalAddress[5]);
				// Добавляем MAC адрес в список сетевых интерфейсов
				this->ifs.emplace(adapter->AdapterName, temp);
			}
			// Выполняем смену итератора
			adapter = adapter->Next;
		}
	// Если данные адаптера прочитать не вышло
	} else {
		// Выводим сообщение об ошибке
		this->log->print("сall to GetAdaptersAddresses failed with error: %d", log_t::flag_t::WARNING, result);
		// Если ошибка связана с отсутствием данных
		if(result == ERROR_NO_DATA)
			// Выводим сообщение об ошибке
			this->log->print("%s", log_t::flag_t::WARNING, "no addresses were found for the requested parameters");
		// Если данные существуют
		else {
			// Создаём объект сообщения
			LPVOID message = nullptr;
			// Формируем генерарцию сообщения
			if(FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, result, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) & message, 0, nullptr)){
				// Выводим сообщение об ошибке
				this->log->print("%s", log_t::flag_t::WARNING, message);
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
 * init Метод инициализации сбора информации
 */
void awh::IfNet::init() noexcept {
	// Очищаем ранее собранные данные
	this->clear();
	// Выполняем получение IPv4 адресов
	this->getIPAddresses(AF_INET);
	// Выполняем получение MAC IPv4 адресов
	this->getHWAddresses(AF_INET);
	// Выполняем получение IPv6 адресов
	this->getIPAddresses(AF_INET6);
	// Выполняем получение MAC IPv6 адресов
	this->getHWAddresses(AF_INET6);
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
 * name Метод запроса названия сетевого интерфейса
 * @param eth идентификатор сетевого интерфейса
 * @return    название сетевого интерфейса
 */
const string awh::IfNet::name(const string & eth) const noexcept {
	// Результат работы функции
	string result = eth;
	// Если сетевой интерфейс получен
	if(!eth.empty()){
/**
 * Устанавливаем настройки для OS Windows
 */
#if defined(_WIN32) || defined(_WIN64)
		// Получаем размер буфера данных
		ULONG size = sizeof(IP_ADAPTER_INFO);
		// Выделяем память под буфер данных
		PIP_ADAPTER_INFO addr = (IP_ADAPTER_INFO *) MALLOC(sizeof(IP_ADAPTER_INFO));
		// Если буфер данных не существует
		if(addr == nullptr){
			// Выводим сообщение об ошибке
			this->log->print("%s", log_t::flag_t::WARNING, "error allocating memory needed to call GetAdaptersinfo");
			// Выходим из функции
			return result;
		}
		// Выполняем первоначальный вызов GetAdaptersInfo, чтобы получить необходимый размер.
		if(GetAdaptersInfo(addr, &size) == ERROR_BUFFER_OVERFLOW){
			// Очищаем выделенную ранее память
			FREE(addr);
			// Выделяем ещё раз память для буфера данных
			addr = (IP_ADAPTER_INFO *) MALLOC(size);
			// Если буфер данных не существует
			if(addr == nullptr){
				// Выводим сообщение об ошибке
				this->log->print("%s", log_t::flag_t::WARNING, "error allocating memory needed to call GetAdaptersinfo");
				// Выходим из функции
				return result;
			}
		}
		// Результат получения данных адаптера
		DWORD info = 0;
		// Получаем информацию о сетевом адаптере
		if((info = GetAdaptersInfo(addr, &size)) == NO_ERROR){
			// В случае успеха выводим некоторую информацию из полученных данных.
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
		} else this->log->print("GetAdaptersInfo failed with error: %d", log_t::flag_t::WARNING, info);
		// Очищаем выделенную ранее память
		if(addr != nullptr) FREE(addr);
#endif
	}
	// Выводим результат
	return result;
}

#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

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
	// Если запрашиваемый адрес IPv6
	if(family == AF_INET6){
		// Создаём массив параметров сетевого интерфейса
		int mib[6];
		// Размер буфера данных
		size_t size = 0;
		// Создаём объект подключения
		struct sockaddr_in6 addr;
		// Объекты для работы с сетевым интерфейсом
		struct rt_msghdr * rtm    = nullptr;
		struct sockaddr_dl * sdl  = nullptr;
		struct sockaddr_in6 * sin = nullptr;
		// Параметры итератора в буфере
		char * it = nullptr, * end = nullptr;
		// Устанавливаем парарметры сетевого интерфейса
		mib[0] = CTL_NET;
		mib[1] = PF_ROUTE;
		mib[2] = 0;
		mib[3] = family;
		mib[4] = NET_RT_FLAGS;
		mib[5] = RTF_LLINFO;
		// Заполняем нулями структуру объекта подключения
		memset(&addr, 0, sizeof(addr));
		// Устанавливаем протокол интернета
		addr.sin6_family = family;
		// Указываем адрес IPv6 для сервера
		inet_pton(family, ip.c_str(), &addr.sin6_addr);
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
		// Переходим по всем сетевым интерфейсам
		for(it = buffer.data(); it < end; it += rtm->rtm_msglen){
			// Получаем указатель сетевого интерфейса
			rtm = (struct rt_msghdr *) it;
			// Если версия RTM протокола не соответствует, пропускаем
			if(rtm->rtm_version != RTM_VERSION) continue;
			// Получаем текущее значение активного подключения
			sin = (struct sockaddr_in6 *)(it + sizeof(rt_msghdr));
/**
 * Если мы работаем с KAME
 */
#ifdef __KAME__
			{
				// Получаем текущий адрес IPv6
				struct in6_addr * in6 = &sin->sin6_addr;
				// Проверяем вид интерфейса, если интерфейс локальный и скоуп-ID не установлен
				if((IN6_IS_ADDR_LINKLOCAL(in6) || IN6_IS_ADDR_MC_LINKLOCAL(in6) || IN6_IS_ADDR_MC_INTFACELOCAL(in6)) && (sin->sin6_scope_id == 0)){
					// Принудительно устанавливаем скоуп-ID
					sin->sin6_scope_id = (u_int32_t) ntohs(* (u_short *) &in6->s6_addr[2]);
					// Выполняем зануление третьего хексета
					* (u_short *) &in6->s6_addr[2] = 0;
				}
			}
#endif
			// Получаем текущее значение аппаратного сетевого адреса
			sdl = (struct sockaddr_dl *)((char *)sin + ROUNDUP(sin->sin6_len));
			// Если версия сетевого протокола отличается от IPv4, то пропускаем
			if(sdl->sdl_family != AF_LINK) continue;
			// Если RTM не соответствует хосту, пропускаем
			// if(!(rtm->rtm_flags & RTF_HOST)) continue;
			// Проверяем соответствует ли IP адрес - тому, что мы ищем
			// if(!IN6_ARE_ADDR_EQUAL(&addr.sin6_addr, &sin->sin6_addr)) continue;


			// Создаем буфер для получения IPv6 адреса
			char host[INET6_ADDRSTRLEN];
			// Заполняем структуру нулями проверяемого хоста
			memset(host, 0, INET6_ADDRSTRLEN);
			// Копируем полученные данные
			inet_ntop(family, &reinterpret_cast <struct sockaddr_in6 *> (sin)->sin6_addr, host, INET6_ADDRSTRLEN);

			cout << " +++++++++++++++++ " << host << endl;


			/*
			// Если сетевой интерфейс получен
			if(sdl->sdl_alen > 0x00){
				// Выделяем память для MAC адреса
				char temp[18];
				// Заполняем нуляем наши буферы
				memset(temp, 0, sizeof(temp));
				// Извлекаем MAC адрес
				const u_char * cp = (u_char *) LLADDR(sdl);
				// Выполняем получение MAC адреса
				sprintf(temp, "%02X:%02X:%02X:%02X:%02X:%02X", cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]);
				// Получаем результат MAC адреса
				result = move(temp);
				// Выходим из цикла
				break;
			}
			*/
		}
	// Если запрашиваемый адрес IPv4
	} else if(family == AF_INET) {
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
			// Если сетевой интерфейс отличается от IPv4 пропускаем
			if(sin->sin_family != family) continue;
			// Если искомый IP адрес не совпадает, пропускаем
			if(addr != sin->sin_addr.s_addr) continue;
			// Если сетевой интерфейс получен
			if(sdl->sdl_alen > 0x00){
				// Выделяем память для MAC адреса
				char temp[18];
				// Заполняем нуляем наши буферы
				memset(temp, 0, sizeof(temp));
				// Извлекаем MAC адрес
				const u_char * cp = (u_char *) LLADDR(sdl);
				// Выполняем получение MAC адреса
				sprintf(temp, "%02X:%02X:%02X:%02X:%02X:%02X", cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]);
				// Получаем результат MAC адреса
				result = move(temp);
				// Выходим из цикла
				break;
			}
		}
	}
/**
 * Если операционной системой является Linux
 */
#elif __linux__
	// Если запрашиваемый адрес IPv6
	if(family == AF_INET6){
		/*
		// Числовое значение IP адреса
		uint32_t addr = 0;
		// Флаг найденнго MAC адреса
		bool found = false;
		// Создаём объект MAC адреса
		struct sockaddr mac;
		// Создаём объект подключения IPv6
		struct sockaddr_in6 sin6;
		// Создаем буфер для получения текущего IPv6 адреса
		char target[INET6_ADDRSTRLEN];
		// Заполняем нулями структуру объекта MAC адреса
		memset(&mac, 0, sizeof(mac));
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
		memset(target, 0, INET6_ADDRSTRLEN);
		// Заполняем буфер данными текущего адреса IPv6
		inet_ntop(family, &sin6.sin6_addr, target, INET6_ADDRSTRLEN);
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
		// Создаем буферы сетевых адресов
		char ifaddr[INET6_ADDRSTRLEN];
		char dstaddr[INET6_ADDRSTRLEN];
		// Переходим по всем сетевым интерфейсам
		for(struct ifaddrs * ifa = headIfa; ifa != nullptr; ifa = ifa->ifa_next){
			// Если сетевой интерфейс не соответствует, пропускаем
			if((ifa->ifa_addr == nullptr) || (ifa->ifa_flags & IFF_POINTOPOINT) || (ifa->ifa_addr->sa_family != family)) continue;
			// Заполняем структуры нулями сетевых адресов
			memset(ifaddr,  0, INET6_ADDRSTRLEN);
			memset(dstaddr, 0, INET6_ADDRSTRLEN);
			// Заполняем буфер данными сетевых адресов IPv6
			inet_ntop(family, &((struct sockaddr_in6 *) ifa->ifa_addr)->sin6_addr, ifaddr, INET6_ADDRSTRLEN);
			inet_ntop(family, &((struct sockaddr_in6 *) ifa->ifa_dstaddr)->sin6_addr, dstaddr, INET6_ADDRSTRLEN);
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
			// Выполняем получение MAC адреса
			sprintf(temp, "%02X:%02X:%02X:%02X:%02X:%02X", cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]);
			// Получаем результат MAC адреса
			result = move(temp);
		}
		// Закрываем сетевой сокет
		::close(fd);
		*/
	// Если запрашиваемый адрес IPv4
	} else if(family == AF_INET) {
		// Числовое значение IP адреса
		uint32_t addr = 0;
		// Флаг найденнго MAC адреса
		bool found = false;
		// Создаём объект MAC адреса
		struct sockaddr mac;
		// Создаём объект подключения IPv4
		struct sockaddr_in sin;
		// Заполняем нулями структуру объекта MAC адреса
		memset(&mac, 0, sizeof(mac));
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
			// Выполняем получение MAC адреса
			sprintf(temp, "%02X:%02X:%02X:%02X:%02X:%02X", cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]);
			// Получаем результат MAC адреса
			result = move(temp);
		}
		// Закрываем сетевой сокет
		::close(fd);
	}
/**
 * Устанавливаем настройки для OS Windows
 */
#elif defined(_WIN32) || defined(_WIN64)
	// Если запрашиваемый адрес IPv6
	if(family == AF_INET6){
		/** NDP IPv6 MAC Address */
	// Если запрашиваемый адрес IPv4
	} else if(family == AF_INET) {
		// Размер буфера данных
		ULONG size = 6;
		// Буфер данных
		ULONG buffer[2];
		// Объект IP адреса назначения
		struct in_addr destip;
		// Устанавливаем IP адрес назначения
		destip.s_addr = inet_addr(ip.c_str());
		// Отправляем запрос на указанный адрес
		SendARP((IPAddr) destip.S_un.S_addr, 0, buffer, &size);
		// Если MAC адрес получен
		if(size > 0){
			// Выделяем память для MAC адреса
			char temp[18];
			// Заполняем нуляем наши буферы
			memset(temp, 0, sizeof(temp));
			// Получаем данные MAC адреса
			BYTE * mac = (BYTE *) &buffer;
			// Выполняем получение MAC адреса
			sprintf(temp, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
			// Получаем результат MAC адреса
			result = move(temp);
		}
	}
#endif
	}
	// Выводим результат
	return result;
}
/**
 * Метод вывода IP адреса соответствующего сетевому интерфейсу
 * @param eth    идентификатор сетевого интерфейса
 * @param family тип протокола интернета AF_INET или AF_INET6
 * @return       IP адрес соответствующий сетевому интерфейсу
 */
const string & awh::IfNet::ip(const string & eth, const int family) const noexcept {
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
	return this->result;
}
