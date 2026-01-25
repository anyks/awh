#pragma once
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stropts.h>
#include <sys/tihdr.h>
#include <sys/timod.h>
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

inline handle_t create(const char* = nullptr) {
    for (int i = 0; i < 255; ++i) {
        std::string path = "/dev/tun" + std::to_string(i);
        int fd = open(path.c_str(), O_RDWR);
        if (fd >= 0) {
            if (ioctl(fd, I_PUSH, "ip") != 0) {
                close(fd);
                continue;
            }
            return {fd, "tun" + std::to_string(i)};
        }
    }
    throw std::runtime_error("No free tun device on Solaris");
}

inline ssize_t read_packet(handle_t& h, packet_t& pkt) {
    pkt.data.resize(2000);
    ssize_t n = read(h.fd, pkt.data.data(), pkt.data.size());
    if (n > 0) pkt.data.resize(n);
    return n;
}
inline ssize_t write_packet(handle_t& h, const packet_t& pkt) {
    return write(h.fd, pkt.data.data(), pkt.data.size());
}

// На Solaris IP настраивается вручную
inline void configure_interface(const char* ifname, const char* ip, const char* mask, const char* = nullptr) {
    std::cout << "Run manually:\n";
    std::cout << "  ifconfig " << ifname << " " << ip << "/" 
              << (mask ? (std::string(mask) == "255.255.255.0" ? "24" : "??") : "24") << " up\n";
    sleep(3);
}
