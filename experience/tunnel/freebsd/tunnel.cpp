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
    // Enable TUN packet information (header)
    int flag = 1;
    ioctl(fd, TUNSIFHEAD, &flag);

    out_fd = fd;
    return std::string(name);
}

void TunInterface::destroy(const std::string& ifname) {
    // Try to explicitly destroy the interface using socket IOCTL.
    // This handles cases where close(fd) might not immediately remove it 
    // or if the interface persists due to system settings.
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s >= 0) {
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, ifname.c_str(), IFNAMSIZ);
        
        // SIOCIFDESTROY destroys a cloned interface
        if (ioctl(s, SIOCIFDESTROY, &ifr) == 0) {
             std::cout << "Interface " << ifname << " destroyed." << std::endl;
        }
        close(s);
    }
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
