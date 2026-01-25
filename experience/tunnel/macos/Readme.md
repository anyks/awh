# MacOS TUN/TAP Example

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Running

Root privileges are required.

### UDP Server
```bash
# Replace 0.0.0.0 with client IP if known, or leave as 0.0.0.0 to learn dynamically
sudo ./tun_macos server 10.0.0.1 10.0.0.2 2049 0.0.0.0 2049 udp
```

### UDP Client
```bash
# Replace 1.2.3.4 with the actual Server IP
sudo ./tun_macos client 10.0.0.2 10.0.0.1 2049 1.2.3.4 2049 udp
```

## Note
MacOS uses `utun` devices. The interface name will likely be `utun0`, `utun1`, etc.
The code automatically handles the 4-byte protocol family header required by `utun`.


## Set Gateway
```bash
# Удаляем установленный роутинг
sudo route delete -net 10.0.0.0/8 2>/dev/null

# Добавьте маршрут до всей подсети через utun6
sudo route add -net 10.0.0.0/8 -interface utun6

# Или
sudo route add -host 10.0.0.1 -interface utun6
```

### Gateway c++
#### route_macos.hpp
```c++
// route_macos.hpp
#pragma once
#include <sys/socket.h>
#include <net/route.h>
#include <netinet/in.h>
#include <string>
#include <stdexcept>
#include <cstring>

namespace route {

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

inline void add_route_to_interface(const char* network, int prefix_len, const char* ifname) {
    int s = socket(PF_ROUTE, SOCK_RAW, 0);
    if (s < 0) throw std::runtime_error("Failed to open PF_ROUTE socket");

    struct {
        struct rt_msghdr m_rtm;
        char m_space[512];
    } msg = {};

    auto sin = [](in_addr_t addr) -> struct sockaddr_in {
        struct sockaddr_in sa = {};
        sa.sin_family = AF_INET;
        sa.sin_len = sizeof(sa);
        sa.sin_addr.s_addr = addr;
        return sa;
    };

    // Заполняем заголовок
    msg.m_rtm.rtm_version = RTM_VERSION;
    msg.m_rtm.rtm_type = RTM_ADD;
    msg.m_rtm.rtm_flags = RTF_UP | RTF_GATEWAY | RTF_STATIC;
    msg.m_rtm.rtm_addrs = RTA_DST | RTA_GATEWAY;
    msg.m_rtm.rtm_pid = getpid();
    msg.m_rtm.rtm_seq = 1;
    msg.m_rtm.rtm_msglen = sizeof(msg.m_rtm);

    // DST: сеть (например, 10.0.0.0)
    in_addr_t net_addr = inet_network(network); // возвращает host-order!
    net_addr = htonl(net_addr << (32 - prefix_len)); // маскируем до префикса

    struct sockaddr_in* dst = (struct sockaddr_in*)&msg.m_space[0];
    *dst = sin(net_addr);
    msg.m_rtm.rtm_msglen += sizeof(*dst);

    // GATEWAY: указываем интерфейс через sockaddr_dl
    struct sockaddr_dl* sdl = (struct sockaddr_dl*)(dst + 1);
    sdl->sdl_len = sizeof(*sdl);
    sdl->sdl_family = AF_LINK;
    sdl->sdl_index = if_nametoindex(ifname);
    if (sdl->sdl_index == 0) {
        close(s);
        throw std::runtime_error("Invalid interface name");
    }
    msg.m_rtm.rtm_msglen += sizeof(*sdl);

    if (write(s, &msg, msg.m_rtm.rtm_msglen) < 0) {
        close(s);
        throw std::runtime_error("Failed to add route");
    }

    close(s);
}

inline void delete_route_to_interface(const char* network, int prefix_len, const char* ifname) {
    int s = socket(PF_ROUTE, SOCK_RAW, 0);
    if (s < 0) throw std::runtime_error("Failed to open PF_ROUTE socket");

    struct {
        struct rt_msghdr m_rtm;
        char m_space[512];
    } msg = {};

    auto sin = [](in_addr_t addr) -> struct sockaddr_in {
        struct sockaddr_in sa = {};
        sa.sin_family = AF_INET;
        sa.sin_len = sizeof(sa);
        sa.sin_addr.s_addr = addr;
        return sa;
    };

    msg.m_rtm.rtm_version = RTM_VERSION;
    msg.m_rtm.rtm_type = RTM_DELETE;
    msg.m_rtm.rtm_flags = RTF_UP | RTF_GATEWAY | RTF_STATIC;
    msg.m_rtm.rtm_addrs = RTA_DST | RTA_GATEWAY;
    msg.m_rtm.rtm_pid = getpid();
    msg.m_rtm.rtm_seq = 1;
    msg.m_rtm.rtm_msglen = sizeof(msg.m_rtm);

    in_addr_t net_addr = inet_network(network);
    net_addr = htonl(net_addr << (32 - prefix_len));

    struct sockaddr_in* dst = (struct sockaddr_in*)&msg.m_space[0];
    *dst = sin(net_addr);
    msg.m_rtm.rtm_msglen += sizeof(*dst);

    struct sockaddr_dl* sdl = (struct sockaddr_dl*)(dst + 1);
    sdl->sdl_len = sizeof(*sdl);
    sdl->sdl_family = AF_LINK;
    sdl->sdl_index = if_nametoindex(ifname);
    if (sdl->sdl_index == 0) {
        close(s);
        throw std::runtime_error("Invalid interface name");
    }
    msg.m_rtm.rtm_msglen += sizeof(*sdl);

    if (write(s, &msg, msg.m_rtm.rtm_msglen) < 0) {
        // Игнорируем ошибку "not found"
    }

    close(s);
}

} // namespace route
```

#### main.cpp
```c++
#include "route_macos.hpp"

int main() {
    try {
        auto tun = create_utun(); // ваша функция создания utun
        configure_interface(tun.name.c_str(), "10.0.0.2", "255.0.0.0", "10.0.0.1");

        // Удаляем старый маршрут (если есть)
        route::delete_route_to_interface("10.0.0.0", 8, tun.name.c_str());

        // Добавляем новый маршрут
        route::add_route_to_interface("10.0.0.0", 8, tun.name.c_str());

        // ... запуск тоннеля ...

        // При завершении (опционально):
        route::delete_route_to_interface("10.0.0.0", 8, tun.name.c_str());

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
```

#### Notify:

- **PF_ROUTE** — низкоуровневый интерфейс ядра macOS/BSD для управления маршрутами.
- **RTA_DST** — адрес назначения (10.0.0.0/8),
- **RTA_GATEWAY** — шлюз; для интерфейса используется sockaddr_dl с sdl_index = if_nametoindex("utun6"),
- Флаг **RTF_GATEWAY** здесь означает «использовать этот маршрут как шлюз», но для P2P это стандартная практика.

#### ⚠️ Важно
- Требуются права root (как и при вызове route).
- Этот код работает только на macOS / BSD.
- Для Linux используется Netlink (rtnetlink), но вы просили macOS.

## Test client
```bash
nc -u 10.0.0.1 8000
Hello World!!!
```
