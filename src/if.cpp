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
	int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
	// Если файловый дескриптор не создан, выходим
	if(fd < 0){
		// Выводим сообщение об ошибке
		this->log->print("%s", log_t::flag_t::CRITICAL, "socket failed");
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
		this->log->print("%s", log_t::flag_t::CRITICAL, "ioctl failed");
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
		this->log->print("%s", log_t::flag_t::CRITICAL, "socket failed");
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
		this->log->print("%s", log_t::flag_t::CRITICAL, "ioctl failed");
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
		this->log->print("%s", log_t::flag_t::CRITICAL, "socket failed");
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
		this->log->print("%s", log_t::flag_t::CRITICAL, "ioctl failed");
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
					sprintf(temp, "%02hhx:%02hhx:%02hhx:%02hhx:%02x:%02hhx", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
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
