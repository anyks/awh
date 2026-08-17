/**
 * @file ttlprobe.cpp
 *
 * @brief Щуп доставки групповой рассылки при нулевом пределе числа переходов
 *
 * @details Проверки перенаправления портов ведут обмен устройством петли лишь затем,
 *          чтобы просьба обнаружения не ушла к настоящим приборам сети. Щуп сличает
 *          с этим иной путь: рассылка идёт устройством местной сети, но с пределом
 *          числа переходов, равным нулю. Такая просьба машину не покидает, а до
 *          гнёзд этой же машины доходить обязана
 *
 * Печатает три опыта: петлёй (как сейчас), местной сетью с пределом 0 (предлагаемое)
 * и местной сетью с пределом 1 (для сличения - этот наружу уходит)
 */

#include <cstdio>
#include <cstring>
#include <string>

#if _WIN32 || _WIN64
	#include <sys/win32.hpp>
	#include <iphlpapi.h>
#else
	#include <unistd.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <ifaddrs.h>
	#include <net/if.h>
#endif

#include "../../posix.hpp"

/**
 * @brief Функция получения адреса машины в местной сети
 */
static std::string address() noexcept {
	struct ifaddrs * list = nullptr;
	if(::getifaddrs(&list) != 0) return std::string();
	std::string result;
	for(struct ifaddrs * item = list; item != nullptr; item = item->ifa_next){
		if((item->ifa_addr == nullptr) || (item->ifa_addr->sa_family != AF_INET)) continue;
		if((item->ifa_flags & IFF_LOOPBACK) || !(item->ifa_flags & IFF_UP)) continue;
		const uint32_t value = ntohl(reinterpret_cast <struct sockaddr_in *> (item->ifa_addr)->sin_addr.s_addr);
		if((value >> 16) == 0xA9FE) continue;
		if(!(((value >> 24) == 10) || (((value >> 20) & 0xFFF) == 0xAC1) || ((value >> 16) == 0xC0A8))) continue;
		result.assign(::inet_ntoa(reinterpret_cast <struct sockaddr_in *> (item->ifa_addr)->sin_addr));
		break;
	}
	::freeifaddrs(list);
	return result;
}

/**
 * @brief Функция опыта доставки рассылки заданным устройством и пределом переходов
 *
 * @param iface адрес устройства, каким ведётся рассылка
 * @param hops  предел числа переходов
 * @return      доставлена ли рассылка
 */
static bool delivers(const std::string & iface, const unsigned char hops) noexcept {
	const int rx = ::socket(AF_INET, SOCK_DGRAM, 0);
	if(rx < 0) return false;
	const int tx = ::socket(AF_INET, SOCK_DGRAM, 0);
	if(tx < 0){ ::closesocket(rx); return false; }
	bool delivered = false;
	int yes = 1;
	::setsockopt(rx, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast <const char *> (&yes), sizeof(yes));
	struct sockaddr_in bound; ::memset(&bound, 0, sizeof(bound));
	bound.sin_family = AF_INET;
	bound.sin_port = 0;
	bound.sin_addr.s_addr = htonl(INADDR_ANY);
	if(::bind(rx, reinterpret_cast <struct sockaddr *> (&bound), sizeof(bound)) == 0){
		socklen_t length = sizeof(bound);
		::getsockname(rx, reinterpret_cast <struct sockaddr *> (&bound), &length);
		struct ip_mreq group; ::memset(&group, 0, sizeof(group));
		group.imr_multiaddr.s_addr = ::inet_addr("239.255.255.250");
		group.imr_interface.s_addr = ::inet_addr(iface.c_str());
		if(::setsockopt(rx, IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast <const char *> (&group), sizeof(group)) == 0){
			struct in_addr device; device.s_addr = ::inet_addr(iface.c_str());
			::setsockopt(tx, IPPROTO_IP, IP_MULTICAST_IF, reinterpret_cast <const char *> (&device), sizeof(device));
			const unsigned char loop = 1;
			::setsockopt(tx, IPPROTO_IP, IP_MULTICAST_LOOP, reinterpret_cast <const char *> (&loop), sizeof(loop));
			if(::setsockopt(tx, IPPROTO_IP, IP_MULTICAST_TTL, reinterpret_cast <const char *> (&hops), sizeof(hops)) != 0)
				::printf("    предел переходов установить не удалось\n");
			struct sockaddr_in target; ::memset(&target, 0, sizeof(target));
			target.sin_family = AF_INET;
			target.sin_port = bound.sin_port;
			target.sin_addr.s_addr = ::inet_addr("239.255.255.250");
			::setReceiveTimeout(rx, 700);
			if(::sendto(tx, "PROBE", 5, 0, reinterpret_cast <struct sockaddr *> (&target), sizeof(target)) > 0){
				char buffer[16];
				delivered = (::recv(rx, buffer, sizeof(buffer), 0) > 0);
			} else ::printf("    отправка отказала\n");
		} else ::printf("    вступление в группу отказало\n");
	}
	::closesocket(tx);
	::closesocket(rx);
	return delivered;
}

int main(){
	#if _WIN32 || _WIN64
		WSADATA data;
		::WSAStartup(MAKEWORD(2, 2), &data);
	#endif
	const std::string local = address();
	::printf("адрес местной сети: %s\n\n", (local.empty() ? "НЕТ" : local.c_str()));
	::printf("петля,        предел 1: %s\n", (delivers("127.0.0.1", 1) ? "доставлена" : "НЕ ДОСТАВЛЕНА"));
	if(!local.empty()){
		::printf("местная сеть, предел 0: %s\n", (delivers(local, 0) ? "доставлена" : "НЕ ДОСТАВЛЕНА"));
		::printf("местная сеть, предел 1: %s\n", (delivers(local, 1) ? "доставлена" : "НЕ ДОСТАВЛЕНА"));
	}
	return 0;
}
