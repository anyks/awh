#include "tun.hpp"
#include "net.hpp"
#include <iostream>
#include <cstring>

int main() {
    try {
        auto tun = create("tun0");
        configure_interface("tun0", "10.8.0.1", "255.255.255.0");
        std::cout << "TUN: " << tun.name << " (10.8.0.1)\n";

        SOCKET sock = udp_socket();
        udp_bind(sock, 1194);

        char buf[1];
        struct sockaddr_storage caddr;
        socklen_t clen = sizeof(caddr);
        recvfrom(sock, buf, 1, MSG_PEEK, (struct sockaddr*)&caddr, &clen);
        std::cout << "Client connected\n";

        run_tunnel(tun.fd, sock, (struct sockaddr*)&caddr, clen);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
