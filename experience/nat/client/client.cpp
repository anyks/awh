#include <iostream>
#include <string>
#include <vector>
#include "../common/socket_utils.hpp"

// --- NAT-PMP ---
bool natpmp_map(uint16_t internal_port, uint16_t& external_port) {
    auto sock = net::create_udp_socket();
    if (sock == INVALID_SOCKET) return false;

    // NAT-PMP запрос (RFC 6886)
    uint8_t req[12] = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    *(uint16_t*)(req + 8) = htons(internal_port);
    *(uint16_t*)(req + 10) = htons(0); // lifetime = 0 (default)

    if (!net::send_udp(sock, std::string((char*)req, 12), "192.168.1.1", 5351)) {
        close(sock);
        return false;
    }

    auto resp = net::recv_udp(sock, 3);
    close(sock);

    if (resp.size() >= 16 && (uint8_t)resp[1] == 128) {
        external_port = ntohs(*(uint16_t*)(resp.data() + 12));
        return true;
    }
    return false;
}

// --- PCP ---
bool pcp_map(uint16_t internal_port, uint16_t& external_port) {
    auto sock = net::create_udp_socket();
    if (sock == INVALID_SOCKET) return false;

    // PCP запрос (RFC 6887)
    uint8_t req[60] = {2, 1, 0, 0}; // version=2, opcode=1 (MAP)
    // lifetime = 0
    // client IP = 0 (will be filled by router)
    // nonce = 0
    *(uint16_t*)(req + 56) = htons(internal_port);
    *(uint16_t*)(req + 58) = htons(0); // remote port = 0

    if (!net::send_udp(sock, std::string((char*)req, 60), "192.168.1.1", 5351)) {
        close(sock);
        return false;
    }

    auto resp = net::recv_udp(sock, 3);
    close(sock);

    if (resp.size() >= 60 && (uint8_t)resp[1] == 129) {
        external_port = ntohs(*(uint16_t*)(resp.data() + 56));
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
    uint16_t internal_port = 54321;
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

    // Отправляем "Hello World" на сервер
    auto sock = net::create_udp_socket();
    net::send_udp(sock, "Hello World", "SERVER_PUBLIC_IP", 12345);
    close(sock);

    net::cleanup_sockets();
    return 0;
}
