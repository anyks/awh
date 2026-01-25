#pragma once
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

struct packet_t { std::vector<uint8_t> data; };

struct handle_t {
    int fd = -1;
    std::string name;
};

inline handle_t create(const char* desired_name = "tun0") {
    int fd = open("/dev/net/tun", O_RDWR);
    if (fd < 0) throw std::runtime_error("Cannot open /dev/net/tun");
    struct ifreq ifr {};
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    strncpy(ifr.ifr_name, desired_name, IFNAMSIZ - 1);
    if (ioctl(fd, TUNSETIFF, &ifr) < 0) {
        close(fd);
        throw std::runtime_error("TUNSETIFF failed");
    }
    return {fd, std::string(ifr.ifr_name)};
}

inline ssize_t read_packet(int tun_fd, packet_t& pkt) {
    pkt.data.resize(2000);
    ssize_t n = read(tun_fd, pkt.data.data(), pkt.data.size());
    if (n > 0) pkt.data.resize(n);
    return n;
}

inline ssize_t write_packet(int tun_fd, const packet_t& pkt) {
    return write(tun_fd, pkt.data.data(), pkt.data.size());
}

inline void configure_interface(const char* ifname, const char* ip, const char* mask, const char* = nullptr) {
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
