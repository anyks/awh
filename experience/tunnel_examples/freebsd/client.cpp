#include "tun.hpp"
#include "net.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <server_ip>\n";
        return 1;
    }
    try {
        auto tun = create("tun0");
        configure_interface("tun0", "10.8.0.2", "255.255.255.0");
        std::cout << "TUN: " << tun.name << " (10.8.0.2)\n";

        int sock = udp_socket();
        struct sockaddr_in saddr{};
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(1194);
        inet_pton(AF_INET, argv[1], &saddr.sin_addr);

        run_tunnel(tun.fd, sock, (struct sockaddr*)&saddr, sizeof(saddr));
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
