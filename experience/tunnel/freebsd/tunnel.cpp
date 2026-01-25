#include "tunnel.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_tun.h>
#include <netinet/in.h>
#include <netinet/in_var.h> // Required for in_aliasreq
#include <arpa/inet.h>
#include <sys/stat.h>

std::string TunInterface::create(const std::string& name_template, int& out_fd) {
    // FreeBSD: Opening /dev/tun cloning device creates a new interface
    int fd = open("/dev/tun", O_RDWR);
    if (fd < 0) {
        throw std::runtime_error("Cannot open /dev/tun");
    }

    // Get the name of the created interface
    char name[100];
    if (fdevname_r(fd, name, sizeof(name)) == NULL) {
        close(fd);
        throw std::runtime_error("fdevname_r failed");
    }

    // Default mode usually includes address family header (4 bytes)
    // We can try to turn it off with TUNSIFHEAD ioctl(fd, TUNSIFHEAD, &zero) if needed,
    // but the macOS implementation handles the header, so we can keep it consistent if desired.
    // However, for simplicity, let's try to disable the header info to make it like Linux (pure IP packet).
    int flag = 0;
    if (ioctl(fd, TUNSIFHEAD, &flag) < 0) {
        // If we can't disable it, we must handle it in main.cpp
        // We will assume we failed to disable it if error, or it succeeded.
        // Actually, let's Enable it explicitly to be deterministic.
        flag = 1;
        ioctl(fd, TUNSIFHEAD, &flag); 
    }

    out_fd = fd;
    return std::string(name);
}

void TunInterface::destroy(const std::string& ifname) {
    std::cout << "Interface " << ifname << " will be destroyed when the process exits." << std::endl;
}

void TunInterface::set_ip(const std::string& ifname, const std::string& local_ip, const std::string& peer_ip) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) throw std::runtime_error("Socket configuration failed");

    struct in_aliasreq ifra;
    memset(&ifra, 0, sizeof(ifra));
    strncpy(ifra.ifra_name, ifname.c_str(), IFNAMSIZ);

    // 1. Set Local IP
    struct sockaddr_in* addr = (struct sockaddr_in*)&ifra.ifra_addr;
    addr->sin_family = AF_INET;
    addr->sin_len = sizeof(struct sockaddr_in);
    inet_pton(AF_INET, local_ip.c_str(), &addr->sin_addr);

    // 2. Set Destination Address (Peer)
    // For P-t-P interfaces, ifra_broadaddr is the destination address
    addr = (struct sockaddr_in*)&ifra.ifra_broadaddr;
    addr->sin_family = AF_INET;
    addr->sin_len = sizeof(struct sockaddr_in);
    inet_pton(AF_INET, peer_ip.c_str(), &addr->sin_addr);

    // 3. Set Mask (Required for SIOCAIFADDR, usually /32 for tunnels)
    addr = (struct sockaddr_in*)&ifra.ifra_mask;
    addr->sin_family = AF_INET;
    addr->sin_len = sizeof(struct sockaddr_in);
    addr->sin_addr.s_addr = 0xFFFFFFFF; // 255.255.255.255

    if (ioctl(sock, SIOCAIFADDR, &ifra) < 0) {
        close(sock);
        throw std::runtime_error("ioctl(SIOCAIFADDR) failed");
    }

    close(sock);
}

void TunInterface::up(const std::string& ifname) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) throw std::runtime_error("Socket configuration failed");

    struct ifreq itr;
    memset(&itr, 0, sizeof(itr));
    strncpy(itr.ifr_name, ifname.c_str(), IFNAMSIZ);

    if (ioctl(sock, SIOCGIFFLAGS, &itr) < 0) {
        close(sock);
        throw std::runtime_error("ioctl(SIOCGIFFLAGS) failed");
    }

    itr.ifr_flags |= (IFF_UP | IFF_RUNNING);

    if (ioctl(sock, SIOCSIFFLAGS, &itr) < 0) {
        close(sock);
        throw std::runtime_error("ioctl(SIOCSIFFLAGS) failed");
    }
    
    // Set MTU
    itr.ifr_mtu = 1400;
    ioctl(sock, SIOCSIFMTU, &itr);

    close(sock);
}
