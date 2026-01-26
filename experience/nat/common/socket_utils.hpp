#pragma once
#include <string>
#include <cstdint>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    using socklen_t = int;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    using SOCKET = int;
#endif

namespace net {

inline bool init_sockets() {
#ifdef _WIN32
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
#else
    return true;
#endif
}

inline void cleanup_sockets() {
#ifdef _WIN32
    WSACleanup();
#endif
}

SOCKET create_udp_socket();
bool send_udp(SOCKET sock, const std::string& msg, const char* ip, uint16_t port);
std::string recv_udp(SOCKET sock, uint16_t timeout_sec = 5);

} // namespace net
