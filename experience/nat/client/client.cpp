#include <iostream>
#include <string>
#include <vector>
#include "../common/socket_utils.hpp"

#ifdef __APPLE__
#include <sys/sysctl.h>
#include <net/route.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <vector>

static inline size_t sockaddr_size(const struct sockaddr* sa) {
    if (sa->sa_family == AF_INET) {
        return sizeof(struct sockaddr_in);
    } else if (sa->sa_family == AF_INET6) {
        return sizeof(struct sockaddr_in6);
    }
    return sizeof(struct sockaddr);
}

std::string get_default_gateway() {
    int mib[6] = {CTL_NET, PF_ROUTE, 0, AF_INET, NET_RT_FLAGS, RTF_GATEWAY};
    size_t needed;
    
    if (sysctl(mib, 6, nullptr, &needed, nullptr, 0) < 0) {
        return "192.168.1.1";
    }

    std::vector<char> buf(needed);
    if (sysctl(mib, 6, buf.data(), &needed, nullptr, 0) < 0) {
        return "192.168.1.1";
    }

    struct rt_msghdr* rtm = (struct rt_msghdr*)buf.data();
    char* sa_ptr = buf.data() + sizeof(struct rt_msghdr);
    
    // Пропускаем адрес назначения (RTA_DST)
    if (rtm->rtm_addrs & RTA_DST) {
        sa_ptr += sockaddr_size((struct sockaddr*)sa_ptr);
    }
    
    // Теперь идёт шлюз (RTA_GATEWAY)
    if (rtm->rtm_addrs & RTA_GATEWAY) {
        struct sockaddr* sa = (struct sockaddr*)sa_ptr;
        if (sa->sa_family == AF_INET) {
            struct sockaddr_in* sin = (struct sockaddr_in*)sa;
            char str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &sin->sin_addr, str, INET_ADDRSTRLEN);
            return std::string(str);
        }
    }
    
    return "192.168.1.1";
}
#endif

// --- NAT-PMP ---
bool natpmp_map(uint16_t internal_port, uint16_t& external_port) {
    // === 1. Получаем шлюз (gateway) ===
    const char* gateway = "192.168.7.1"; // ← ЗАМЕНИТЕ НА ВАШ ШЛЮЗ ИЗ ВЫВОДА natpmpc!
	
	printf("Using gateway: %s == %s\n", gateway, get_default_gateway().c_str());

    // === 2. Создаём UDP-сокет ===
    auto sock = net::create_udp_socket();
    if (sock == INVALID_SOCKET) return false;

    // === 3. Шаг 1: Запрос публичного IP (обязательный!) ===
    uint8_t pub_req[2] = {0, 0}; // версия=0, опкод=0
    if (!net::send_udp(sock, std::string(reinterpret_cast<char*>(pub_req), 2), gateway, 5351)) {
        close(sock);
        return false;
    }
    auto pub_resp = net::recv_udp(sock, 2);
    if (pub_resp.size() < 12 || static_cast<uint8_t>(pub_resp[1]) != 128) {
        close(sock);
        return false;
    }

    // === 4. Шаг 2: Запрос проброса порта ===
    uint8_t map_req[12] = {0};
    map_req[0] = 0; // версия
    map_req[1] = 1; // опкод = 1 (map)
    *reinterpret_cast<uint16_t*>(map_req + 8) = htons(internal_port);  // внутренний порт
    *reinterpret_cast<uint16_t*>(map_req + 10) = htons(0);            // внешний порт = 0 (авто)

    if (!net::send_udp(sock, std::string(reinterpret_cast<char*>(map_req), 12), gateway, 5351)) {
        close(sock);
        return false;
    }

    auto map_resp = net::recv_udp(sock, 2);
    close(sock);

    // === 5. Парсим ответ ===
    if (map_resp.size() >= 16 && static_cast<uint8_t>(map_resp[1]) == 129) {
        external_port = ntohs(*reinterpret_cast<const uint16_t*>(map_resp.data() + 12));
        return true;
    }
    return false;
}

bool test_pcp() {
    const char* gateway = "192.168.7.1"; // ← ваш шлюз
    uint16_t internal_port = 54321;

    auto sock = net::create_udp_socket();
    if (sock == INVALID_SOCKET) return false;

    // PCP MAP request (RFC 6887)
    std::vector<uint8_t> req(60, 0);
    req[0] = 2; // версия PCP
    req[1] = 1; // опкод MAP
    // req[4..19] = client IP (оставляем 0 — заполнит роутер)
    // req[20..35] = nonce (можно 0)
    *reinterpret_cast<uint16_t*>(req.data() + 56) = htons(internal_port); // внутренний порт
    *reinterpret_cast<uint16_t*>(req.data() + 58) = htons(0);            // внешний порт = 0

    if (!net::send_udp(sock, std::string(reinterpret_cast<char*>(req.data()), req.size()), gateway, 5351)) {
        close(sock);
        return false;
    }

    auto resp = net::recv_udp(sock, 3);
    close(sock);

    if (resp.size() >= 60 && static_cast<uint8_t>(resp[1]) == 129) {
        uint16_t external_port = ntohs(*reinterpret_cast<const uint16_t*>(resp.data() + 56));
        std::cout << "PCP success! External port: " << external_port << "\n";
        return true;
    }
    std::cout << "PCP failed or not supported\n";
    return false;
}

// --- PCP ---
bool pcp_map(uint16_t internal_port, uint16_t& external_port) {
    const char* gateway = "192.168.7.1"; // ← ВАШ ШЛЮЗ!

    auto sock = net::create_udp_socket();
    if (sock == INVALID_SOCKET) return false;

    // PCP MAP request (RFC 6887)
    uint8_t req[60] = {0};
    req[0] = 2; // версия PCP
    req[1] = 1; // опкод MAP
    // req[2] = reserved (0)
    req[3] = 6; // протокол: 6 = TCP, 17 = UDP
    // req[4..7] = lifetime (в секундах). 0 = по умолчанию (обычно 2 часа)
    // req[8..23] = client IP (оставляем 0 — заполнит роутер)
    // req[24..39] = nonce (можно 0)
    // req[40..55] = remote peer IP (0.0.0.0 для любого)
    *(uint16_t*)(req + 56) = htons(internal_port); // внутренний порт
    *(uint16_t*)(req + 58) = htons(0);            // внешний порт = 0 (авто)

    if (!net::send_udp(sock, std::string(reinterpret_cast<char*>(req), 60), gateway, 5351)) {
        close(sock);
        return false;
    }

    auto resp = net::recv_udp(sock, 3);
    close(sock);

    if (resp.size() >= 60 && static_cast<uint8_t>(resp[1]) == 129) {
        external_port = ntohs(*reinterpret_cast<const uint16_t*>(resp.data() + 56));
        return true;
    }
    return false;
}

// --- UPnP ---
#ifdef __APPLE__
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#include <miniupnpc/upnperrors.h>

bool upnp_map(uint16_t internal_port, uint16_t& external_port) {
    // 1. Обнаружение IGD
    UPNPDev* devlist = upnpDiscover(2000, nullptr, nullptr, 0, 0, 2, nullptr);
    if (!devlist) {
        std::cerr << "No UPnP device found\n";
        return false;
    }

    // 2. Подготовка структур
    UPNPUrls urls = {0};
    IGDdatas data = {0};
    char lan_addr[64] = {0};

    // 3. Получение валидного IGD
    int r = UPNP_GetValidIGD(devlist, &urls, &data, lan_addr, sizeof(lan_addr), nullptr, 0);
    freeUPNPDevlist(devlist);

    if (r != 1) {
        std::cerr << "No valid IGD found (code: " << r << ")\n";
        FreeUPNPUrls(&urls); // освободить даже при ошибке
        return false;
    }

    // 4. Проброс порта
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", internal_port);

    int r2 = UPNP_AddPortMapping(
        urls.controlURL,
        data.first.servicetype,
        port_str,           // external port
        port_str,           // internal port
        lan_addr,           // internal client
        "NAT Punch Demo",   // description
        "TCP",              // protocol
        nullptr,            // remote host
        "0"                 // lease duration
    );

    // 5. Очистка
    FreeUPNPUrls(&urls);
    // IGDdatas не требует free() — все данные внутри urls или временные

    if (r2 != UPNPCOMMAND_SUCCESS) {
        std::cerr << "UPnP AddPortMapping failed: " << strupnperror(r2) << "\n";
        return false;
    }

    external_port = internal_port;
    std::cout << "UPnP: mapped " << lan_addr << ":" << internal_port << " -> public:" << external_port << "\n";
    return true;
}
#endif

// --- Основная логика ---
int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: client --method [upnp|natpmp|pcp]\n";
        return 1;
    }

    std::string method = argv[2];
    uint16_t internal_port = 607;
    uint16_t external_port = 0;

    net::init_sockets();

    bool mapped = false;
    if (method == "upnp") {
#ifdef __APPLE__
        mapped = upnp_map(internal_port, external_port);
#else
        std::cerr << "UPnP supported only on macOS\n";
#endif
    } else if (method == "natpmp") {
        mapped = natpmp_map(internal_port, external_port);
    } else if (method == "pcp") {
        mapped = pcp_map(internal_port, external_port);
    }

    if (!mapped) {
        std::cerr << "Failed to map port\n";
        return 1;
    }

    std::cout << "Mapped port: " << external_port << "\n";

	// Слушаем локальный порт
	int listen_sock = socket(AF_INET, SOCK_DGRAM, 0);
	sockaddr_in local{};
	local.sin_family = AF_INET;
	local.sin_port = htons(internal_port);
	local.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(listen_sock, (sockaddr*)&local, sizeof(local));

    // Отправляем "Hello World" на сервер
    auto sock = net::create_udp_socket();
    net::send_udp(sock, "Hello World", "89.169.31.66", 2049);
    close(sock);

	// Ждём ответ от сервера
	char buf[256];
	recvfrom(listen_sock, buf, sizeof(buf), 0, nullptr, nullptr);
	std::cout << "Received from server: " << buf << "\n";
	close(listen_sock);

    net::cleanup_sockets();
    return 0;
}
