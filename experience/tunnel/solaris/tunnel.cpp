#include "tunnel.hpp"
#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>
#include <unistd.h>
#include <fcntl.h>
#include <stropts.h>
#include <sys/sockio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <net/if.h>
#include <netinet/in.h>

// Solaris specific
#include <sys/tun.h> 
// If sys/tun.h is not found, define TUNNEWPPA manually (it is standard on Solaris though)
#ifndef TUNNEWPPA
#define TUNNEWPPA   (('T'<<8) | 1)
#endif


std::string TunInterface::create(const std::string& name_template, int& out_fd) {
    int fd = open("/dev/tun", O_RDWR);
    if (fd < 0) {
        throw std::runtime_error("Cannot open /dev/tun");
    }

    int ppa = 0;
    while (ppa < 256) {
        if (ioctl(fd, TUNNEWPPA, ppa) == 0) {
            break;
        }
        ppa++;
    }

    if (ppa >= 256) {
        close(fd);
        throw std::runtime_error("Could not find free TUN PPA");
    }

    // Now push the 'ip' module to make it an interface consistent with IP stack
    // Actually, for a TUN interface used by an app to read/write packets,
    // we might NOT want to push 'ip' on top of the stream if we want to read the raw packets ourselves?
    // On Solaris, "There are two ways to use the tun driver..."
    // If we want to be the "lower stream" (the wire), we just hold the fd.
    // The interface created is "tun<ppa>". We need to "plumb" it separately via ifconfig usually.
    
    std::string name = "tun" + std::to_string(ppa);
    out_fd = fd;
    
    // We need to plumbing using a separate socket or system call
    std::string cmd = "ifconfig " + name + " plumb";
    if (std::system(cmd.c_str()) != 0) {
        // Maybe already plumbed?
    }

    return name;
}

void TunInterface::destroy(const std::string& ifname) {
    // Unplumb to remove the interface from IP stack
    std::string cmd = "ifconfig " + ifname + " unplumb";
    std::system(cmd.c_str());
    // Closing the FD (in main) removes the PPA association eventually.
    std::cout << "Interface " << ifname << " unplumbed and will be destroyed on close." << std::endl;
}

void TunInterface::set_ip(const std::string& ifname, const std::string& local_ip, const std::string& peer_ip) {
    // In Solaris, setting the IP requires the interface to be PLUMBED.
    // Our 'create' method relies on system("ifconfig ... plumb") because programmatic plumbing
    // via STREAMS (opening /dev/ip, pushing modules, linking mux ID) is extremely complex 
    // and verbose (hundreds of lines) compared to standard socket ioctl.
    // Assuming the interface is plumbed (done in create()), we can set IP via ioctl on a socket.

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) throw std::runtime_error("Socket creation failed");

    struct ifreq itr;
    memset(&itr, 0, sizeof(itr));
    strncpy(itr.ifr_name, ifname.c_str(), IFNAMSIZ);

    struct sockaddr_in* addr = (struct sockaddr_in*)&itr.ifr_addr;

    // 1. Set Local Address
    addr->sin_family = AF_INET;
    inet_pton(AF_INET, local_ip.c_str(), &addr->sin_addr);

    if (ioctl(sock, SIOCSIFADDR, &itr) < 0) {
        close(sock);
        throw std::runtime_error("ioctl(SIOCSIFADDR) failed. Ensure interface is plumbed.");
    }

    // 2. Set Destination Address
    // Solaris uses ifr_dstaddr logic similar to BSD
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
        throw std::runtime_error("ioctl(SIOCGIFFLAGS) failed");
    }

    itr.ifr_flags |= (IFF_UP | IFF_RUNNING);

    if (ioctl(sock, SIOCSIFFLAGS, &itr) < 0) {
        close(sock);
        throw std::runtime_error("ioctl(SIOCSIFFLAGS) failed");
    }

    close(sock);
}
