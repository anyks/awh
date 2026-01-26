#include <iostream>
#include <string>
#include "../common/socket_utils.hpp"

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: server <bind_ip> <port>\n";
        return 1;
    }

    const char* bind_ip = argv[1];
    uint16_t port = (uint16_t)std::stoi(argv[2]);

    net::init_sockets();

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) return 1;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, bind_ip, &addr.sin_addr);

    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Bind failed\n";
        return 1;
    }

    std::cout << "Server listening on " << bind_ip << ":" << port << "\n";

    char buffer[1024];
    sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    ssize_t n = recvfrom(sock, buffer, sizeof(buffer)-1, 0, (sockaddr*)&client_addr, &addr_len);
    if (n > 0) {
        buffer[n] = '\0';
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        std::cout << "Received from " << client_ip << ": " << buffer << "\n";
    }

    close(sock);
    net::cleanup_sockets();
    return 0;
}
