#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <sys/uio.h> // Required for readv/writev on FreeBSD
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <signal.h>
#include "tunnel.hpp"

// Global flag for signal handling
volatile sig_atomic_t keep_running = 1;

void signal_handler(int signo) {
    keep_running = 0;
}

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
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

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

        uint8_t buffer[2048];
        struct sockaddr_in sender_addr;
        socklen_t sender_len = sizeof(sender_addr);

        while (keep_running) {
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(tun_fd, &fds);
            FD_SET(net_fd, &fds);
            int max_fd = (tun_fd > net_fd) ? tun_fd : net_fd;

            if (select(max_fd + 1, &fds, NULL, NULL, NULL) < 0) break;

            if (FD_ISSET(tun_fd, &fds)) {
                // Reading from tun with TUNSIFHEAD=1 expectation
				/**
				 * В iov[0] (4 байта): address family (AF_INET = 2, AF_INET6 = 30)
				 * В iov[1]: сырой IP-пакет (начиная с IPv4/IPv6 заголовка)
				 * На FreeBSD: family = AF_INET (2) или AF_INET6 (30)
				 */
                uint32_t family = 0;
                struct iovec iov[2];
                iov[0].iov_base = &family;
                iov[0].iov_len = sizeof(family);
                iov[1].iov_base = buffer;
                iov[1].iov_len = sizeof(buffer);

                ssize_t n = readv(tun_fd, iov, 2);
                if (n > 0) {
                     // If we are getting IP packet directly (mode 0), family would look like 0x4500....
                     // If we are getting Header (mode 1), family should be 0x02000000 (AF_INET in Net Order) or 0x00000002.
                     
                     if (n > (ssize_t)sizeof(family)) {
                        ssize_t payload_len = n - sizeof(family);

						if (payload_len >= 20) {
							// Определяем версию по первым 4 битам
							uint8_t version = (buffer[0] >> 4) & 0x0F;

							if (version == 4) {
								if (n < 20) return; // недостаточно для IPv4
								uint32_t src_ip = *(uint32_t*)(buffer + 12); // source IP
       							uint32_t dst_ip = *(uint32_t*)(buffer + 16); // destination IP
								// обработка IPv4

								// Важно: dst_ip в network byte order (big-endian)
								// Если нужно в host order — используйте ntohl()
								printf("Destination IP: %d.%d.%d.%d\n",
									(dst_ip >> 24) & 0xFF,
									(dst_ip >> 16) & 0xFF,
									(dst_ip >> 8)  & 0xFF,
									(dst_ip)       & 0xFF);

								if (sock_type == SOCK_DGRAM) {
									sendto(net_fd, buffer, payload_len, 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
								} else {
									send(net_fd, buffer, payload_len, 0);
								}
							}
							else if (version == 6) {
								if (n < 40) return; // недостаточно для IPv6
								uint8_t* src_ip6 = buffer + 8;   // source address (16 байт)
        						uint8_t* dst_ip6 = buffer + 24;  // destination address (16 байт)
								// обработка IPv6

								if (sock_type == SOCK_DGRAM) {
									sendto(net_fd, buffer, payload_len, 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
								} else {
									send(net_fd, buffer, payload_len, 0);
								}
							}
						}
                     }
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
                }
                else {
                    n = recv(net_fd, buffer, sizeof(buffer), 0);
                }
                
                if (n <= 0) break;

                // Tun WRITE (to Kernel):
                // We assume Mode 1 (Header). FreeBSD expects Network Byte Order (htonl).
                uint32_t family = htonl(AF_INET);
                struct iovec iov[2];
                iov[0].iov_base = &family;
                iov[0].iov_len = sizeof(family);
                iov[1].iov_base = buffer;
                iov[1].iov_len = n;
                writev(tun_fd, iov, 2);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    cleanup(tun_fd, net_fd, tun_name);
    return 0;
}
