#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdexcept>

#pragma comment(lib, "ws2_32.lib")

inline void init_winsock() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0)
        throw std::runtime_error("WSAStartup failed");
}

inline SOCKET udp_socket() {
    init_winsock();
    SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == INVALID_SOCKET) throw std::runtime_error("UDP socket failed");
    return s;
}

// ... остальное аналогично, но с SOCKET и closesocket()


inline void udp_bind(SOCKET s, uint16_t port) {
    struct sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    a.sin_addr.s_addr = INADDR_ANY;
    if (bind(s, (struct sockaddr*)&a, sizeof(a)) == SOCKET_ERROR)
        throw std::runtime_error("bind failed");
}

inline void udp_connect(SOCKET s, const char* ip, uint16_t port) {
    struct sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &a.sin_addr) <= 0)
        throw std::runtime_error("Invalid IP");
    if (connect(s, (struct sockaddr*)&a, sizeof(a)) == SOCKET_ERROR)
        throw std::runtime_error("connect failed");
}
