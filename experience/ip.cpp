// g++ --std=c++11 ip.cpp -o ./ip -lstdc++

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

std::unordered_map <string, string> ips;
std::unordered_map <string, string> ips6;

/**
 * getIPAddresses Метод извлечения IP-адресов
 * @param family тип протокола интернета AF_INET или AF_INET6
 */
void getIPAddresses(const int32_t family) noexcept {
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
				ips.emplace(i->ifr_name, inet_ntoa(reinterpret_cast <struct sockaddr_in *> (&i->ifr_addr)->sin_addr));
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
				ips6.emplace(i->ifr_name, buffer);
			} break;
		}
	}
	// Закрываем сетевой сокет
	::close(fd);
}

int main() {
  getIPAddresses(AF_INET);
  for(auto & item : ips)
	cout << " ------ " << item.first << " == " << item.second << endl;
  return 0;
}
