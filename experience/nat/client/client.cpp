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

bool natpmp_unmap(uint16_t internal_port, uint16_t external_port) {
    const char* gateway = "192.168.7.1"; // Используем тот же шлюз, что и в map
    auto sock = net::create_udp_socket();
    if (sock == INVALID_SOCKET) return false;

    // Public Address Request (обязателен!)
    uint8_t pub_req[2] = {0, 0};
    net::send_udp(sock, std::string(reinterpret_cast<char*>(pub_req), 2), gateway, 5351);
    net::recv_udp(sock, 2);

    // Port Mapping Request с lifetime = 0
    uint8_t req[12] = {0};
    req[0] = 0; // version
    req[1] = 1; // opcode = map UDP (для TCP нужен opcode 2)
    // req[2..3] = reserved
    *reinterpret_cast<uint16_t*>(req + 4) = htons(internal_port);  // Internal Port
    *reinterpret_cast<uint16_t*>(req + 6) = htons(0);              // External Port (should be 0 for delete request)
    *reinterpret_cast<uint32_t*>(req + 8) = htonl(0);             // Lifetime = 0 → delete

    bool sent = net::send_udp(sock, std::string(reinterpret_cast<char*>(req), 12), gateway, 5351);
    
    // Ждем подтверждения от роутера, чтобы убедиться, что удаление обработано
    net::recv_udp(sock, 2);

    close(sock);
    return sent;
}

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
    map_req[1] = 1; // опкод = 1 (map UDP)
    map_req[2] = 0; // зарезервировано
    map_req[3] = 0; // зарезервировано
    *reinterpret_cast<uint16_t*>(map_req + 4) = htons(internal_port);  // внутренний порт
    *reinterpret_cast<uint16_t*>(map_req + 6) = htons(0);            // внешний порт = 0 (авто)
    *reinterpret_cast<uint32_t*>(map_req + 8) = htonl(3600);         // время жизни (секунды)

    if (!net::send_udp(sock, std::string(reinterpret_cast<char*>(map_req), 12), gateway, 5351)) {
        close(sock);
        return false;
    }

    auto map_resp = net::recv_udp(sock, 16);
    close(sock);

    // === 5. Парсим ответ ===
    if (map_resp.size() >= 16 && static_cast<uint8_t>(map_resp[1]) == 129) {
        // Успех (Result code = 0) проверяем в map_resp[2..3]
        uint16_t result_code = ntohs(*reinterpret_cast<const uint16_t*>(map_resp.data() + 2));
        if (result_code != 0) {
            printf("NAT-PMP Error Code: %d\n", result_code);
            return false;
        }
        // Внешний порт (Mapped External Port) - offset 10
        external_port = ntohs(*reinterpret_cast<const uint16_t*>(map_resp.data() + 10));
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

#include <random>

bool pcp_unmap(uint16_t internal_port, uint16_t external_port, const std::vector<uint8_t>& nonce, bool is_udp = true) {
    const char* gateway = "192.168.7.1"; // Используем тот же шлюз, что и в map

    // === 1. Определяем свой локальный IP (нужен для PCP) ===
    struct sockaddr_in local_addr = {0};
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s >= 0) {
        struct sockaddr_in dst = {0};
        dst.sin_family = AF_INET;
        dst.sin_port = htons(5351);
        inet_pton(AF_INET, gateway, &dst.sin_addr);
        
        if (connect(s, (struct sockaddr*)&dst, sizeof(dst)) == 0) {
            socklen_t len = sizeof(local_addr);
            getsockname(s, (struct sockaddr*)&local_addr, &len);
        }
        close(s);
    }

    auto sock = net::create_udp_socket();
    if (sock == INVALID_SOCKET) return false;

    // PCP MAP request (RFC 6887) для удаления
    uint8_t req[60] = {0};
    req[0] = 2; // version
    req[1] = 1; // opcode = MAP (для удаления используем MAP с lifetime 0)
    
    *reinterpret_cast<uint32_t*>(req + 4) = htonl(0); // lifetime = 0

    // Client IP Address (Header offset 8) -> ::ffff:192.168.x.x
    req[18] = 0xFF;
    req[19] = 0xFF;
    memcpy(req + 20, &local_addr.sin_addr, 4);

    // Mapping Nonce (Offset 24, 12 bytes) - ИСПОЛЬЗУЕМ ТОТ ЖЕ NONCE, ЧТО ПРИ СОЗДАНИИ!
    if (nonce.size() == 12) {
        memcpy(req + 24, nonce.data(), 12);
    } else {
         // Fallback on random if not provided, but likely to fail
         std::random_device rd;
         std::mt19937_64 gen(rd());
         for(int i = 24; i < 36; ++i) req[i] = (uint8_t)(gen() % 256);
    }

    // Protocol (Offset 36)
    req[36] = is_udp ? 17 : 6; 

    // Internal Port (Offset 40)
    *reinterpret_cast<uint16_t*>(req + 40) = htons(internal_port); 
    // External Port (Offset 42) - при удалении обычно 0
    *reinterpret_cast<uint16_t*>(req + 42) = htons(0);

    net::send_udp(sock, std::string(reinterpret_cast<char*>(req), 60), gateway, 5351);
    
    // Ждем подтверждения
    auto resp = net::recv_udp(sock, 2);
    
    close(sock);
    return true;
}

// --- PCP ---
bool pcp_map(uint16_t internal_port, uint16_t& external_port, std::vector<uint8_t>& out_nonce) {
    const char* gateway = "192.168.7.1"; // ← ВАШ ШЛЮЗ!

    // === 1. Определяем свой локальный IP (нужен для PCP) ===
    struct sockaddr_in local_addr = {0};
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s >= 0) {
        struct sockaddr_in dst = {0}; // Целевой адрес для "подключения" (чтобы узнать свой IP)
        dst.sin_family = AF_INET;
        dst.sin_port = htons(5351);
        inet_pton(AF_INET, gateway, &dst.sin_addr);
        
        // connect на UDP просто устанавливает default destination и выбирает интерфейс
        if (connect(s, (struct sockaddr*)&dst, sizeof(dst)) == 0) {
            socklen_t len = sizeof(local_addr);
            getsockname(s, (struct sockaddr*)&local_addr, &len);
        }
        close(s);
    }

    if (local_addr.sin_addr.s_addr == 0) {
        std::cerr << "Failed to determine local IP for PCP\n";
        return false;
    }
    
    char local_ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &local_addr.sin_addr, local_ip_str, INET_ADDRSTRLEN);
    printf("Local IP for PCP: %s\n", local_ip_str);

    auto sock = net::create_udp_socket();
    if (sock == INVALID_SOCKET) return false;

    // PCP MAP request (RFC 6887)
    uint8_t req[60] = {0};
    req[0] = 2; // version
    req[1] = 1; // opcode = MAP (Client Request)
    
    // Lifetime (Header offset 4)
    *reinterpret_cast<uint32_t*>(req + 4) = htonl(3600); // 1 час жизни

    // Client IP Address (Header offset 8) -> ::ffff:192.168.x.x
    // IPv4-mapped IPv6 address
    req[18] = 0xFF;
    req[19] = 0xFF;
    memcpy(req + 20, &local_addr.sin_addr, 4);

    // Mapping Nonce (Offset 24, 12 bytes) - Randomize it
    out_nonce.resize(12);
    for(int i = 0; i < 12; ++i) out_nonce[i] = req[24 + i] = rand() % 255;

    // Protocol (Offset 36)
    req[36] = 17; // протокол: 17 = UDP, 6 = TCP

    // Internal Port (Offset 40)
    *reinterpret_cast<uint16_t*>(req + 40) = htons(internal_port); 
    // External Port (Offset 42)
    *reinterpret_cast<uint16_t*>(req + 42) = htons(0); // 0 = авто

    if (!net::send_udp(sock, std::string(reinterpret_cast<char*>(req), 60), gateway, 5351)) {
        close(sock);
        return false;
    }

    auto resp = net::recv_udp(sock, 60);
    close(sock);

    if (resp.size() >= 60 && static_cast<uint8_t>(resp[1]) == 129) { // 129 = MAP Opcode Response
        uint8_t result_code = resp[3]; // Result Code
        if (result_code != 0) {
             printf("PCP Error Code: %d\n", result_code);
             return false;
        }
        // Assigned External Port (Offset 42)
        external_port = ntohs(*reinterpret_cast<const uint16_t*>(resp.data() + 42));

		printf("EXTERNAL PORT %d\n", external_port);

        return true;
    }
    return false;
}

// --- UPnP ---
#ifdef __APPLE__
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#include <miniupnpc/upnperrors.h>


void list_upnp_mappings() {
    UPNPDev* devlist = upnpDiscover(2000, nullptr, nullptr, 0, 0, 2, nullptr);
    if (!devlist) return;

    UPNPUrls urls = {0};
    IGDdatas data = {0};
    char lan_addr[64] = {0};
    int r = UPNP_GetValidIGD(devlist, &urls, &data, lan_addr, sizeof(lan_addr), nullptr, 0);
    freeUPNPDevlist(devlist);
    if (r != 1) {
        FreeUPNPUrls(&urls);
        return;
    }

    int i = 0;
    while (true) {
        char ext_port[16] = {0};
        char int_client[64] = {0};
        char int_port[16] = {0};
        char protocol[16] = {0};
        char desc[128] = {0};
        char enabled[16] = {0};
        char rhost[64] = {0};
        char duration[16] = {0}; // ← БЫЛО: unsigned int, СТАЛО: char[]

        int ret = UPNP_GetGenericPortMappingEntry(
            urls.controlURL,
            data.first.servicetype,
            std::to_string(i).c_str(),
            ext_port, int_client, int_port, protocol, desc, enabled, rhost, duration
        );

        if (ret != 0) break; // нет больше записей

        std::cout << "Mapping " << i << ": "
                  << protocol << " " << ext_port << " -> " << int_client << ":" << int_port
                  << " (" << desc << ", lease=" << duration << "s)\n";
        i++;
    }

    FreeUPNPUrls(&urls);
}

bool upnp_unmap(uint16_t external_port, bool is_udp = true) {
    UPNPDev* devlist = upnpDiscover(2000, nullptr, nullptr, 0, 0, 2, nullptr);
    if (!devlist) return false;

    UPNPUrls urls;
    IGDdatas data;
    char lan_addr[64];
    int r = UPNP_GetValidIGD(devlist, &urls, &data, lan_addr, sizeof(lan_addr), nullptr, 0);
    freeUPNPDevlist(devlist);
    if (r != 1) return false;

    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", external_port);

    int ret = UPNP_DeletePortMapping(
        urls.controlURL,
        data.first.servicetype,
        port_str,
        is_udp ? "UDP" : "TCP",
        nullptr // remote host
    );

    FreeUPNPUrls(&urls);
    return (ret == UPNPCOMMAND_SUCCESS);
}

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
        "UDP",              // protocol (UDP or TCP)
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
    uint16_t internal_port = 54323;
    uint16_t external_port = 0;
    std::vector<uint8_t> pcp_nonce;

    net::init_sockets();

	list_upnp_mappings();

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
        mapped = pcp_map(internal_port, external_port, pcp_nonce);
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
	//char buf[256];
	//recvfrom(listen_sock, buf, sizeof(buf), 0, nullptr, nullptr);
	//std::cout << "Received from server: " << buf << "\n";
	close(listen_sock);


	if (method == "upnp") {
#ifdef __APPLE__
        upnp_unmap(external_port, true);
#endif
    } else if (method == "natpmp") {
       natpmp_unmap(internal_port, external_port);
    } else if (method == "pcp") {
        pcp_unmap(internal_port, external_port, pcp_nonce, true);
    }

    net::cleanup_sockets();
    return 0;
}
