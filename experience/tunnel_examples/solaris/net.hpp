#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>

inline int udp_socket() {
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) throw std::runtime_error("UDP socket failed");
    return s;
}

inline void udp_bind(int s, uint16_t port) {
    struct sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    a.sin_addr.s_addr = INADDR_ANY;
    if (bind(s, (struct sockaddr*)&a, sizeof(a)) < 0)
        throw std::runtime_error("bind failed");
}

inline void udp_connect(int s, const char* ip, uint16_t port) {
    struct sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &a.sin_addr) <= 0)
        throw std::runtime_error("Invalid IP");
    if (connect(s, (struct sockaddr*)&a, sizeof(a)) < 0)
        throw std::runtime_error("connect failed");
}
