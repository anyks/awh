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

std::unordered_map <string, string> ifs;

/**
 * getHWAddresses Метод извлечения MAC-адресов
 * @param family тип протокола интернета AF_INET или AF_INET6
 */
void getHWAddresses(const int32_t family) noexcept {
	// Структура параметров сетевого интерфейса
	struct ifconf ifc;
	// Структура сетевого интерфейса
	struct ifreq ifrc;
	// Создаём буфер для извлечения сетевых данных
	char buffer[IF_BUFFER_SIZE];
	// Заполняем нуляем наши буферы
	::memset(buffer, 0, sizeof(buffer));
	// Выделяем сокет для подключения
	const SOCKET fd = ::socket(family, SOCK_STREAM, IPPROTO_IP);
	// Если файловый дескриптор не создан, выходим
	if(fd == INVALID_SOCKET){
		// Выводим сообщение об ошибке
		printf("Socket failed");
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
		printf("IOCTL failed");
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
					ifs.emplace(i->ifr_name, hardware);
				}
			}
		// Если подключение к сокету не удалось, выводим сообщение об ошибке
		} else printf("IOCTL failed");
	}
	// Закрываем сетевой сокет
	::close(fd);
}

int main() {
  getHWAddresses(AF_INET);
  for(auto & item : ifs)
	cout << " ------ " << item.first << " == " << item.second << endl;
  return 0;
}
