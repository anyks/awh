#include "tun.hpp"
#include "net.hpp"
#include <sys/select.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <cstring>

constexpr uint8_t KEEPALIVE_MARKER[] = {0x00, 0x00, 0x00, 0x00};
constexpr size_t KEEPALIVE_SIZE = sizeof(KEEPALIVE_MARKER);
inline bool is_keepalive(const uint8_t* d, size_t l) {
    return l == KEEPALIVE_SIZE && std::memcmp(d, KEEPALIVE_MARKER, KEEPALIVE_SIZE) == 0;
}

void run_tunnel(int tun_fd, int net_fd, const struct sockaddr* remote, socklen_t rlen) {
    std::vector<uint8_t> buf(2000);
    constexpr int KINT = 5, KTIMEOUT = 15;
    auto last_rx = std::chrono::steady_clock::now();
    auto last_tx = last_rx;

    while (true) {
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_rx).count() > KTIMEOUT) {
            std::cerr << "Keepalive timeout\n";
            break;
        }

        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(tun_fd, &rfds);
        FD_SET(net_fd, &rfds);
        int max_fd = (tun_fd > net_fd ? tun_fd : net_fd) + 1;

        struct timeval to;
        auto since_tx = std::chrono::duration_cast<std::chrono::seconds>(now - last_tx).count();
        to.tv_sec = (since_tx >= KINT) ? 0 : (KINT - since_tx);
        to.tv_usec = 0;

        if (select(max_fd, &rfds, nullptr, nullptr, &to) > 0) {
            if (FD_ISSET(tun_fd, &rfds)) {
                packet_t p;
                if (read_packet(tun_fd, p) > 0) {
                    sendto(net_fd, p.data.data(), p.data.size(), 0, remote, rlen);
                    last_tx = std::chrono::steady_clock::now();
                }
            }
            if (FD_ISSET(net_fd, &rfds)) {
                ssize_t n = recvfrom(net_fd, buf.data(), buf.size(), 0, nullptr, nullptr);
                if (n <= 0) break;
                last_rx = std::chrono::steady_clock::now();
                if (is_keepalive(buf.data(), n)) continue;
                packet_t p; p.data.assign(buf.begin(), buf.begin() + n);
                write_packet(tun_fd, p);
            }
        }

        now = std::chrono::steady_clock::now();
        since_tx = std::chrono::duration_cast<std::chrono::seconds>(now - last_tx).count();
        if (since_tx >= KINT) {
            sendto(net_fd, KEEPALIVE_MARKER, KEEPALIVE_SIZE, 0, remote, rlen);
            last_tx = now;
        }
    }
}
