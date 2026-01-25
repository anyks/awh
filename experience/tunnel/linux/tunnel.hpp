#ifndef LINUX_TUNNEL_HPP
#define LINUX_TUNNEL_HPP

#include <string>
#include <cstdint>

class TunInterface {
public:
    static std::string create(const std::string& name_template, int& out_fd);
    static void destroy(const std::string& ifname);
    static void set_ip(const std::string& ifname, const std::string& local_ip, const std::string& peer_ip);
    static void up(const std::string& ifname);
};

#endif // LINUX_TUNNEL_HPP
