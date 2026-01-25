#include "tunnel.hpp"
#include <iostream>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/kern_control.h>
#include <sys/sys_domain.h>
#include <net/if_utun.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdio>
#include <net/if.h>

std::string TunInterface::create(const std::string& name_template, int& out_fd) {
    // macOS uses UTUN controller
    struct ctl_info ctlInfo;
    memset(&ctlInfo, 0, sizeof(ctlInfo));
    strncpy(ctlInfo.ctl_name, UTUN_CONTROL_NAME, sizeof(ctlInfo.ctl_name));

    int fd = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
    if (fd < 0) throw std::runtime_error("socket(PF_SYSTEM) failed");

    if (ioctl(fd, CTLIOCGINFO, &ctlInfo) == -1) {
        close(fd);
        throw std::runtime_error("ioctl(CTLIOCGINFO) failed");
    }

    struct sockaddr_ctl sc;
    memset(&sc, 0, sizeof(sc));
    sc.sc_id = ctlInfo.ctl_id;
    sc.sc_len = sizeof(sc);
    sc.sc_family = AF_SYSTEM;
    sc.ss_sysaddr = AF_SYS_CONTROL;
    
    // Attempt to find a free unit number manually or let kernel decide.
    // If we want to implement the "tun -> tun0, tun1" logic:
    // With UTUN, binding with sc_unit = 0 lets the kernel pick the next available 'utunX'.
    // sc_unit = X + 1 requests utunX.
    
    sc.sc_unit = 0; // Automatic
    
    if (connect(fd, (struct sockaddr *)&sc, sizeof(sc)) == -1) {
        close(fd);
        throw std::runtime_error("connect(UTUN) failed");
    }

    // Get the interface name
    char ifname[IFNAMSIZ];
    socklen_t len = IFNAMSIZ;
    if (getsockopt(fd, SYSPROTO_CONTROL, UTUN_OPT_IFNAME, ifname, &len) == -1) {
        close(fd);
        throw std::runtime_error("getsockopt(UTUN_OPT_IFNAME) failed");
    }

    out_fd = fd;
    return std::string(ifname);
}

void TunInterface::destroy(const std::string& ifname) {
    // Like Linux, closing the FD destroys the interface.
    std::cout << "Interface " << ifname << " will be destroyed when the process exits or file descriptor is closed." << std::endl;
}

void TunInterface::set_ip(const std::string& ifname, const std::string& local_ip, const std::string& peer_ip) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) throw std::runtime_error("Socket creation failed");

    struct ifreq itr;
    memset(&itr, 0, sizeof(itr));
    strncpy(itr.ifr_name, ifname.c_str(), IFNAMSIZ);

    struct sockaddr_in* addr = (struct sockaddr_in*)&itr.ifr_addr;
    
    // 1. Set Local IP
    addr->sin_family = AF_INET;
    addr->sin_len = sizeof(struct sockaddr_in);
    inet_pton(AF_INET, local_ip.c_str(), &addr->sin_addr);
    if (ioctl(sock, SIOCSIFADDR, &itr) < 0) {
        close(sock);
        throw std::runtime_error("ioctl(SIOCSIFADDR) failed");
    }

    // 2. Set Peer IP (Destination Address)
    addr->sin_family = AF_INET;
    addr->sin_len = sizeof(struct sockaddr_in);
    inet_pton(AF_INET, peer_ip.c_str(), &addr->sin_addr);
    
    // In macOS struct ifreq has ifr_dstaddr union member
    if (ioctl(sock, SIOCSIFDSTADDR, &itr) < 0) {
        close(sock);
        throw std::runtime_error("ioctl(SIOCSIFDSTADDR) failed");
    }

    close(sock);
}

void TunInterface::up(const std::string& ifname) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) throw std::runtime_error("Socket creation failed");

    struct ifreq itr;
    memset(&itr, 0, sizeof(itr));
    strncpy(itr.ifr_name, ifname.c_str(), IFNAMSIZ);

    // Get current flags
    if (ioctl(sock, SIOCGIFFLAGS, &itr) < 0) {
        close(sock);
        throw std::runtime_error("ioctl(SIOCGIFFLAGS) failed");
    }

    // Add UP and RUNNING
    itr.ifr_flags |= (IFF_UP | IFF_RUNNING);
    
    if (ioctl(sock, SIOCSIFFLAGS, &itr) < 0) {
        close(sock);
        throw std::runtime_error("ioctl(SIOCSIFFLAGS) failed");
    }

    // Set MTU
    itr.ifr_mtu = 1400;
    if (ioctl(sock, SIOCSIFMTU, &itr) < 0) {
        // Warning only
        std::cerr << "Warning: Failed to set MTU" << std::endl;
    }

    close(sock);
}
