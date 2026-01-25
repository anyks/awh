#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <sys/uio.h> // Required for readv/writev on FreeBSD
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include "tunnel.hpp"

// Identical to MacOS main.cpp regarding logic (handling 4-byte header if enabled)
// In tunnel.cpp we enabled TUNSIFHEAD = 1 to be deterministic.

void cleanup(int tun_fd, int sock_fd, const std::string& tun_name) {
    if (tun_fd >= 0) close(tun_fd);
    if (sock_fd >= 0) close(sock_fd);
    TunInterface::destroy(tun_name);
}

int main(int argc, char** argv) {
    if (argc < 7) {
        std::cerr << "Usage: " << argv[0] << " <mode: server|client> <tun_local_ip> <tun_peer_ip> <local_port> <remote_ip> <remote_port> [protocol]" << std::endl;
        return 1;
    }

    std::string mode = argv[1];
    std::string tun_local = argv[2];
    std::string tun_peer = argv[3];
    int local_port = std::stoi(argv[4]);
    std::string remote_ip = argv[5];
    int remote_port = std::stoi(argv[6]);
    std::string protocol = (argc > 7) ? argv[7] : "udp";

    int tun_fd = -1;
    int net_fd = -1;
    std::string tun_name;

    try {
        tun_name = TunInterface::create("tun", tun_fd);
        std::cout << "Created interface: " << tun_name << std::endl;

        TunInterface::set_ip(tun_name, tun_local, tun_peer);
        TunInterface::up(tun_name);

        int sock_type = (protocol == "tcp") ? SOCK_STREAM : SOCK_DGRAM;
        net_fd = socket(AF_INET, sock_type, 0);
        if (net_fd < 0) throw std::runtime_error("Socket creation failed");

        int opt = 1;
        setsockopt(net_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in local_addr;
        memset(&local_addr, 0, sizeof(local_addr));
        local_addr.sin_family = AF_INET;
        local_addr.sin_port = htons(local_port);
        local_addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(net_fd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
            throw std::runtime_error("Bind failed");
        }

        struct sockaddr_in remote_addr;
        memset(&remote_addr, 0, sizeof(remote_addr));
        remote_addr.sin_family = AF_INET;
        remote_addr.sin_port = htons(remote_port);
        inet_pton(AF_INET, remote_ip.c_str(), &remote_addr.sin_addr);

        if (mode == "client" && sock_type == SOCK_STREAM) {
             if (connect(net_fd, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0) throw std::runtime_error("Connect failed");
        } else if (mode == "server" && sock_type == SOCK_STREAM) {
             listen(net_fd, 1);
             int client_fd = accept(net_fd, NULL, NULL);
             if (client_fd < 0) throw std::runtime_error("Accept failed");
             close(net_fd); 
             net_fd = client_fd;
        }

        char buffer[2048];
        struct sockaddr_in sender_addr;
        socklen_t sender_len = sizeof(sender_addr);

        while (true) {
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(tun_fd, &fds);
            FD_SET(net_fd, &fds);
            int max_fd = (tun_fd > net_fd) ? tun_fd : net_fd;

            if (select(max_fd + 1, &fds, NULL, NULL, NULL) < 0) break;

            if (FD_ISSET(tun_fd, &fds)) {
                // Tun READ (from Kernel):
                // If TUNSIFHEAD=0, we read pure IP.
                // If TUNSIFHEAD=1 (default on create/open), we read 4 bytes family + IP.
                // In tunnel.cpp we explicitly set TUNSIFHEAD=0.
                
                ssize_t n = read(tun_fd, buffer, sizeof(buffer));
                if (n > 0) {
                     if (sock_type == SOCK_DGRAM)
                     {
                        sendto(net_fd, buffer, n, 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
                        std::cout << "Sent " << n << " bytes to network" << std::endl;
                     }
                    else
                        send(net_fd, buffer, n, 0);
                }
            }

            if (FD_ISSET(net_fd, &fds)) {
                // Network READ (from Socket):
                ssize_t n;
                if (sock_type == SOCK_DGRAM) {
                    n = recvfrom(net_fd, buffer, sizeof(buffer), 0, (struct sockaddr*)&sender_addr, &sender_len);
                    if (mode == "server") {
                        memcpy(&remote_addr, &sender_addr, sizeof(remote_addr));
                    }
                    std::cout << "Received " << n << " bytes from network" << std::endl;
                }
                else {
                    n = recv(net_fd, buffer, sizeof(buffer), 0);
                }
                
                if (n <= 0) break;

                // Tun WRITE (to Kernel):
                // If TUNSIFHEAD=0, we write pure IP.
                write(tun_fd, buffer, n);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    cleanup(tun_fd, net_fd, tun_name);
    return 0;
}
