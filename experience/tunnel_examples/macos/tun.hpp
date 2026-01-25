#pragma once
#include <sys/kern_control.h>
#include <sys/sys_domain.h>
#include <net/if_utun.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdexcept>
#include <vector>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>

struct packet_t { std::vector<uint8_t> data; };

struct handle_t {
    int fd = -1;
    std::string name;
};

inline handle_t create(const char* = nullptr) {
    int fd = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
    if (fd < 0) throw std::runtime_error("PF_SYSTEM socket failed");

    struct ctl_info ci;
    memset(&ci, 0, sizeof(ci));
    strncpy(ci.ctl_name, UTUN_CONTROL_NAME, sizeof(ci.ctl_name) - 1);

    if (ioctl(fd, CTLIOCGINFO, &ci) != 0) {
        close(fd);
        throw std::runtime_error("CTLIOCGINFO failed");
    }

    struct sockaddr_ctl sc;
    memset(&sc, 0, sizeof(sc));
    sc.sc_len = sizeof(sc);
    sc.sc_family = AF_SYSTEM;
    sc.ss_sysaddr = AF_SYS_CONTROL;
    sc.sc_id = ci.ctl_id;
    sc.sc_unit = 0;

    if (connect(fd, (struct sockaddr*)&sc, sizeof(sc)) != 0) {
        close(fd);
        throw std::runtime_error("utun connect failed");
    }

    char name[100];
    socklen_t len = sizeof(name);
    if (getsockopt(fd, SYSPROTO_CONTROL, UTUN_OPT_IFNAME, name, &len) != 0) {
        close(fd);
        throw std::runtime_error("get utun name failed");
    }

    return {fd, std::string(name)};
}

inline ssize_t read_packet(int tun_fd, packet_t& pkt) {
    pkt.data.resize(2000);
    ssize_t n = read(tun_fd, pkt.data.data(), pkt.data.size());
    if (n > 4 && pkt.data[0] == 0x02 && pkt.data[1] == 0x00) {
        pkt.data.erase(pkt.data.begin(), pkt.data.begin() + 4);
        return n - 4;
    }
    return -1;
}

inline ssize_t write_packet(int tun_fd, const packet_t& pkt) {
    if (pkt.data.empty()) return -1;
    uint8_t prefix[4] = {0x02, 0x00, 0x00, 0x00};
    ssize_t n1 = write(tun_fd, prefix, 4);
    if (n1 != 4) return -1;
    ssize_t n2 = write(tun_fd, pkt.data.data(), pkt.data.size());
    return (n2 > 0) ? n1 + n2 : -1;
}

inline void configure_interface(const char* ifname, const char* ip, const char* mask, const char* dst) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) throw std::runtime_error("socket failed");

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);

    auto set_addr = [&](const char* addr_str, unsigned long cmd) -> bool {
        struct sockaddr_in* sin = (struct sockaddr_in*)&ifr.ifr_addr;
        memset(sin, 0, sizeof(*sin));
        sin->sin_family = AF_INET;
        return inet_pton(AF_INET, addr_str, &sin->sin_addr) > 0 &&
               ioctl(sock, cmd, &ifr) == 0;
    };

    if (!set_addr(ip, SIOCSIFADDR)) {
        close(sock);
        throw std::runtime_error("SIOCSIFADDR failed");
    }
    if (!set_addr(mask, SIOCSIFNETMASK)) {
        close(sock);
        throw std::runtime_error("SIOCSIFNETMASK failed");
    }
    if (dst) {
        if (!set_addr(dst, SIOCSIFDSTADDR)) {
            close(sock);
            throw std::runtime_error("SIOCSIFDSTADDR failed (required on macOS)");
        }
    }

    if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
        ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
        if (ioctl(sock, SIOCSIFFLAGS, &ifr) != 0) {
            close(sock);
            throw std::runtime_error("SIOCSIFFLAGS failed");
        }
    }

    close(sock);
}
