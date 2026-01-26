#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include "tunnel.hpp"

// Simple usage: ./tun_linux <server|client> <tun_ip> <tun_peer_ip> <local_port> <remote_ip> <remote_port> [proto: udp/tcp]

void cleanup(int tun_fd, int sock_fd, const std::string& tun_name) {
    if (tun_fd >= 0) close(tun_fd);
    if (sock_fd >= 0) close(sock_fd);
    TunInterface::destroy(tun_name);
}

int main(int argc, char** argv) {
    if (argc < 7) {
        std::cerr << "Usage: " << argv[0] << " <mode: server|client> <tun_local_ip> <tun_peer_ip> <local_port> <remote_ip> <remote_port> [protocol: udp|tcp|seq]" << std::endl;
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
        // 1. Create TUN
        std::cout << "Creating tunnel interface..." << std::endl;
        tun_name = TunInterface::create("tun", tun_fd);
        std::cout << "Created interface: " << tun_name << std::endl;

        // 2. Configure IP
        TunInterface::set_ip(tun_name, tun_local, tun_peer);
        TunInterface::up(tun_name);

        // 3. Create Network Socket
        int sock_type = SOCK_DGRAM;
        if (protocol == "tcp") sock_type = SOCK_STREAM;
        else if (protocol == "seq") sock_type = SOCK_SEQPACKET;

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

        if (mode == "client" && (sock_type == SOCK_STREAM || sock_type == SOCK_SEQPACKET)) {
            std::cout << "Connecting to " << remote_ip << ":" << remote_port << "..." << std::endl;
            if (connect(net_fd, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0) {
                throw std::runtime_error("Connect failed");
            }
            std::cout << "Connected." << std::endl;
        } else if (mode == "server" && (sock_type == SOCK_STREAM || sock_type == SOCK_SEQPACKET)) {
             if (listen(net_fd, 1) < 0) throw std::runtime_error("Listen failed");
             std::cout << "Waiting for connection..." << std::endl;
             int client_fd = accept(net_fd, NULL, NULL);
             if (client_fd < 0) throw std::runtime_error("Accept failed");
             close(net_fd); // Close listening socket, use client_fd
             net_fd = client_fd;
             std::cout << "Client connected." << std::endl;
        }

        // 4. Transport Loop
        std::cout << "Starting forwarding loop. Press Ctrl+C to stop." << std::endl;
        char buffer[2048];
        struct sockaddr_in sender_addr;
        socklen_t sender_len = sizeof(sender_addr);

        while (true) {
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(tun_fd, &fds);
            FD_SET(net_fd, &fds);
            int max_fd = (tun_fd > net_fd) ? tun_fd : net_fd;

            int ret = select(max_fd + 1, &fds, NULL, NULL, NULL);
            if (ret < 0) {
                if (errno == EINTR) continue;
                break;
            }

            // Packet from TUN -> Network
            if (FD_ISSET(tun_fd, &fds)) {
                ssize_t n = read(tun_fd, buffer, sizeof(buffer));
                if (n > 0) {
					// Извлекаем destination IP (байты 16-19 в IPv4)
    				uint32_t dst_ip = *(uint32_t*)(buffer + 16);
                    if (sock_type == SOCK_DGRAM) {
                        sendto(net_fd, buffer, n, 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
                    } else {
                        send(net_fd, buffer, n, 0);
                    }
                }
            }

            // Packet from Network -> TUN
            if (FD_ISSET(net_fd, &fds)) {
                ssize_t n;
                if (sock_type == SOCK_DGRAM) {
                    n = recvfrom(net_fd, buffer, sizeof(buffer), 0, (struct sockaddr*)&sender_addr, &sender_len);
                     if (mode == "server") {
                        memcpy(&remote_addr, &sender_addr, sizeof(remote_addr));
                    }
                } else {
                    n = recv(net_fd, buffer, sizeof(buffer), 0);
                    if (n == 0) break; // Connection closed
                }

                if (n > 0) {
                    write(tun_fd, buffer, n);
                }
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    cleanup(tun_fd, net_fd, tun_name);
    return 0;
}
