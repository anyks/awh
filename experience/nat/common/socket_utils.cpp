#include "socket_utils.hpp"
#include <cstring>
#include <iostream>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/select.h>
    #include <errno.h>
#endif

namespace net {

SOCKET create_udp_socket() {
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) return sock;

    // Разрешить reuse
    int yes = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(yes));

    return sock;
}

bool send_udp(SOCKET sock, const std::string& msg, const char* ip, uint16_t port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    ssize_t sent = sendto(sock, msg.data(), msg.size(), 0,
                          (sockaddr*)&addr, sizeof(addr));
    return sent == (ssize_t)msg.size();
}

std::string recv_udp(SOCKET sock, uint16_t timeout_sec) {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);

    timeval tv{};
    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;

    int activity = select((int)sock + 1, &readfds, nullptr, nullptr, &tv);
    if (activity <= 0) return {};

    char buffer[1024];
    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    ssize_t n = recvfrom(sock, buffer, sizeof(buffer)-1, 0, (sockaddr*)&addr, &addr_len);
    if (n <= 0) return {};
    buffer[n] = '\0';
    return std::string(buffer, n);
}

} // namespace net
