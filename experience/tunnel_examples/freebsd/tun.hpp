#pragma once
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_tun.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <stdexcept>
#include <vector>
#include <netinet/in.h>
#include <arpa/inet.h>

struct packet_t { std::vector<uint8_t> data; };

struct handle_t {
    int fd = -1;
    std::string name;
};

inline handle_t create(const char* desired_name = "tun0") {
    for (int i = 0; i < 255; ++i) {
        std::string path = "/dev/tun" + std::to_string(i);
        int fd = open(path.c_str(), O_RDWR);
        if (fd >= 0) {
            int mode = IFF_TUN;
            ioctl(fd, TUNSIFMODE, &mode);
            int one = 1;
            ioctl(fd, TUNSIFHEAD, &one);
            return {fd, "tun" + std::to_string(i)};
        }
    }
    throw std::runtime_error("No free tun device");
}

// read_packet, write_packet — как в Linux
inline ssize_t read_packet(handle_t& h, packet_t& pkt) {
    pkt.data.resize(2000);
    ssize_t n = read(h.fd, pkt.data.data(), pkt.data.size());
    if (n > 0) pkt.data.resize(n);
    return n;
}
inline ssize_t write_packet(handle_t& h, const packet_t& pkt) {
    return write(h.fd, pkt.data.data(), pkt.data.size());
}

// configure_interface — как в Linux
inline void configure_interface(const char* ifname, const char* ip, const char* mask, const char* = nullptr) {
    // точно как в Linux
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) throw std::runtime_error("socket failed");
    struct ifreq ifr {};
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    auto set_addr = [&](const char* addr_str, unsigned long cmd) -> bool {
        struct sockaddr_in* sin = (struct sockaddr_in*)&ifr.ifr_addr;
        sin->sin_family = AF_INET;
        return inet_pton(AF_INET, addr_str, &sin->sin_addr) > 0 &&
               ioctl(sock, cmd, &ifr) == 0;
    };
    if (!set_addr(ip, SIOCSIFADDR) || !set_addr(mask, SIOCSIFNETMASK)) {
        close(sock);
        throw std::runtime_error("configure interface failed");
    }
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
        ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
        ioctl(sock, SIOCSIFFLAGS, &ifr);
    }
    close(sock);
}
