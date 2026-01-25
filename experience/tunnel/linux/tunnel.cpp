#include "tunnel.hpp"
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <arpa/inet.h>
#include <net/route.h>

std::string TunInterface::create(const std::string& name_template, int& out_fd) {
    int fd = open("/dev/net/tun", O_RDWR);
    if (fd < 0) {
        throw std::runtime_error("Cannot open /dev/net/tun: " + std::string(strerror(errno)));
    }

    struct ifreq itr;
    memset(&itr, 0, sizeof(itr));
    itr.ifr_flags = IFF_TUN | IFF_NO_PI; // Tunnel device, no packet info
    
    // Check if user wants automatic numbering "tun" -> "tun%d"
    std::string name = name_template;
    if (name.find("%d") == std::string::npos) {
        // Simple heuristic: if it doesn't have %d, append it strictly for the ioctl call
        // if user passed "tun", we want "tun%d" behavior for the kernel to pick next free method
        // BUT user requirement says: "input 'tun' ... automatically compute... tun0, tun1".
        // The Linux kernel supports this if we pass "tun%d" in ifr_name.
        if (name == "tun") {
            strncpy(itr.ifr_name, "tun%d", IFNAMSIZ);
        } else {
             strncpy(itr.ifr_name, name.c_str(), IFNAMSIZ);
        }
    } else {
        strncpy(itr.ifr_name, name.c_str(), IFNAMSIZ);
    }

    if (ioctl(fd, TUNSETIFF, (void*)&itr) < 0) {
        close(fd);
        throw std::runtime_error("ioctl(TUNSETIFF) failed: " + std::string(strerror(errno)));
    }

    out_fd = fd;
    return std::string(itr.ifr_name);
}

void TunInterface::destroy(const std::string& ifname) {
    // On Linux, TUN interfaces created without IFF_PERSIST are destroyed when the file descriptor is closed.
    // However, if we strive for persistence or explicit removal, we can use TUNSETPERSIST.
    // If the interface is not persistent, closing the fd is enough. 
    // The requirement says "create" then "remove". If we just run a program, closing fd is implicit.
    // But let's assume we might want to ensure it's removed if it was made persistent.
    
    int fd = open("/dev/net/tun", O_RDWR);
    if (fd < 0) return;

    struct ifreq itr;
    memset(&itr, 0, sizeof(itr));
    strncpy(itr.ifr_name, ifname.c_str(), IFNAMSIZ);
    itr.ifr_flags = IFF_TUN;

    // Try to attach to existing to clear persist flag, although we usually hold the FD.
    // If this function is called from the same process holding the FD, we can just close the FD?
    // Let's implement 'Software' removal by ensuring the interface is down or just logging.
    // Real removal of non-persistent interface: close FD.
    // If persistent:
    // ioctl(fd, TUNSETPERSIST, 0); 
    
    // Since our create logic doesn't set IFF_TUN_PERSIST, just closing the fd (which happens when program exits)
    // is sufficient. But for the sake of the API:
    // We can't easily "delete" it if other processes hold it, or if it is not persistent.
    
    // We will just print here, because main() will close the FD.
    std::cout << "Interface " << ifname << " will be destroyed when the file descriptor is closed." << std::endl;
    close(fd);
}

void TunInterface::set_ip(const std::string& ifname, const std::string& local_ip, const std::string& peer_ip) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) throw std::runtime_error("Socket creation failed");

    struct ifreq itr;
    memset(&itr, 0, sizeof(itr));
    strncpy(itr.ifr_name, ifname.c_str(), IFNAMSIZ);

    // Set IP
    struct sockaddr_in* addr = (struct sockaddr_in*)&itr.ifr_addr;
    addr->sin_family = AF_INET;
    inet_pton(AF_INET, local_ip.c_str(), &addr->sin_addr);
    if (ioctl(sock, SIOCSIFADDR, &itr) < 0) {
        close(sock);
        throw std::runtime_error("ioctl(SIOCSIFADDR) failed");
    }

    // Set Peer IP (Point-to-Point)
    memset(&itr, 0, sizeof(itr));
    strncpy(itr.ifr_name, ifname.c_str(), IFNAMSIZ);
    addr = (struct sockaddr_in*)&itr.ifr_dstaddr;
    addr->sin_family = AF_INET;
    inet_pton(AF_INET, peer_ip.c_str(), &addr->sin_addr);
    
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

    if (ioctl(sock, SIOCGIFFLAGS, &itr) < 0) {
        close(sock);
        throw std::runtime_error("Unable to get flags");
    }

    itr.ifr_flags |= IFF_UP | IFF_RUNNING;
    if (ioctl(sock, SIOCSIFFLAGS, &itr) < 0) {
        close(sock);
        throw std::runtime_error("Unable to set flags UP");
    } else {
        std::cout << "Interface " << ifname << " is UP." << std::endl;
    }
    
    // Set MTU often helps
    memset(&itr, 0, sizeof(itr));
    strncpy(itr.ifr_name, ifname.c_str(), IFNAMSIZ);
    itr.ifr_mtu = 1400; 
    ioctl(sock, SIOCSIFMTU, &itr);

    close(sock);
}
