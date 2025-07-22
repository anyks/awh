// g++ --std=c++11 mac.cpp -o ./mac -lstdc++
// net0   192.168.53.3         255.255.255.255 SPLA     bc:24:11:4a:94:1f

#include <cmath>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <unordered_map>

/**
 * Стандартные библиотеки
 */
#include <unistd.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <netdb.h>
#include <net/if_dl.h>
#include <netinet/if_ether.h>
#include <sys/sockio.h>
#include <net/route.h>

#define SOCKET int32_t
#define INVALID_SOCKET -1

// Создаём функцию округления
#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

using namespace std;

// Максимальная длина сетевого интерфейса
static constexpr uint16_t MAX_ADDRS = 32;
// Максимальный размер сетевого буфера
static constexpr uint16_t IF_BUFFER_SIZE = 4000;

/**
 * mac Функция получения MAC-адреса по IP-адресу клиента
 * @param ip     адрес интернет-подключения клиента
 * @param family тип протокола интернета AF_INET или AF_INET6
 * @return       аппаратный адрес сетевого интерфейса клиента
 */
string mac(const string & ip, const int32_t family) noexcept {
	// Результат работы функции
	string result = "";
	// Если IP-адрес передан
	if(!ip.empty()){
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
					printf("Invalid IPv4 address");
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
					printf("Invalid ifaddrs");
					// Выходим из функции
					return result;
				}
				// Выделяем сокет для подключения
				const SOCKET fd = ::socket(family, SOCK_DGRAM, IPPROTO_IP);
				// Если файловый дескриптор не создан, выходим
				if(fd == INVALID_SOCKET){
					// Выводим сообщение об ошибке
					printf("Socket failed");
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
						// Копируем название сетевого интерфейса
						// strncpy(arpreq.arp_dev, ifa->ifa_name, IFNAMSIZ);
						// Подключаем сетевой интерфейс к сокету
						if(::ioctl(fd, SIOCGARP, &arpreq) == -1){
							// Пропускаем если ошибка не значительная
							if(errno == ENXIO)
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
				::close(fd);
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
					printf("Invalid IPv6 address");
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
					printf("Invalid ifaddrs");
					// Выходим из функции
					return result;
				}
				// Выделяем сокет для подключения
				const SOCKET fd = ::socket(AF_INET6, SOCK_RAW, IPPROTO_IPV6);
				// Если файловый дескриптор не создан, выходим
				if(fd == INVALID_SOCKET){
					// Выводим сообщение об ошибке
					printf("Socket failed");
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
				::close(fd);
			} break;
		}
	}
	// Выводим результат
	return result;
}

int main() {
	cout << " ------ " << mac("192.168.53.3", AF_INET) << endl;
	return 0;
}
