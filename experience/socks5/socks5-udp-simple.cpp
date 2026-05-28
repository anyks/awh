#include <arpa/inet.h>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <random>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

namespace {

constexpr const char * kProxyHost = "217.29.53.105";
constexpr uint16_t kProxyPort = 11613;
constexpr const char * kProxyUser = "8J0sHd";
constexpr const char * kProxyPass = "G4DfSK";

constexpr const char * kDnsServerIp = "77.88.8.8";
constexpr uint16_t kDnsServerPort = 53;
constexpr const char * kDomain = "anyks.com";

bool sendAll(int fd, const uint8_t * data, size_t size) {
    size_t sent = 0;
    while (sent < size) {
        const ssize_t rc = send(fd, data + sent, size - sent, 0);
        if (rc <= 0) {
            return false;
        }
        sent += static_cast<size_t>(rc);
    }
    return true;
}

bool recvExact(int fd, uint8_t * data, size_t size) {
    size_t received = 0;
    while (received < size) {
        const ssize_t rc = recv(fd, data + received, size - received, 0);
        if (rc <= 0) {
            return false;
        }
        received += static_cast<size_t>(rc);
    }
    return true;
}

int connectTcp(const std::string & host, uint16_t port) {
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo * result = nullptr;
    const std::string portStr = std::to_string(port);
    if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0) {
        return -1;
    }

    int fd = -1;
    for (addrinfo * rp = result; rp != nullptr; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd == -1) {
            continue;
        }
        if (connect(fd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;
        }
        close(fd);
        fd = -1;
    }

    freeaddrinfo(result);
    return fd;
}

std::vector<uint8_t> buildDnsQuery(uint16_t txid, const std::string & domain) {
    std::vector<uint8_t> q;
    q.reserve(64);

    // DNS header (12 bytes)
    q.push_back(static_cast<uint8_t>((txid >> 8) & 0xFF));
    q.push_back(static_cast<uint8_t>(txid & 0xFF));
    q.push_back(0x01); // recursion desired
    q.push_back(0x00);
    q.push_back(0x00); q.push_back(0x01); // QDCOUNT
    q.push_back(0x00); q.push_back(0x00); // ANCOUNT
    q.push_back(0x00); q.push_back(0x00); // NSCOUNT
    q.push_back(0x00); q.push_back(0x00); // ARCOUNT

    // QNAME
    size_t start = 0;
    while (start < domain.size()) {
        const size_t dot = domain.find('.', start);
        const size_t end = (dot == std::string::npos) ? domain.size() : dot;
        const size_t len = end - start;
        if (len == 0 || len > 63) {
            return {};
        }
        q.push_back(static_cast<uint8_t>(len));
        for (size_t i = start; i < end; ++i) {
            q.push_back(static_cast<uint8_t>(domain[i]));
        }
        if (dot == std::string::npos) {
            break;
        }
        start = dot + 1;
    }
    q.push_back(0x00);

    // QTYPE=A, QCLASS=IN
    q.push_back(0x00); q.push_back(0x01);
    q.push_back(0x00); q.push_back(0x01);

    return q;
}

bool skipDnsName(const std::vector<uint8_t> & p, size_t & off) {
    size_t jumps = 0;
    while (off < p.size()) {
        const uint8_t len = p[off];
        if (len == 0) {
            ++off;
            return true;
        }
        // Compression pointer: 11xxxxxx xxxxxxxx
        if ((len & 0xC0) == 0xC0) {
            if (off + 1 >= p.size()) {
                return false;
            }
            off += 2;
            return true;
        }
        if ((len & 0xC0) != 0 || off + 1 + len > p.size()) {
            return false;
        }
        off += 1 + len;
        if (++jumps > 255) {
            return false;
        }
    }
    return false;
}

bool parseFirstARecord(const std::vector<uint8_t> & dnsPayload, std::string & outIp) {
    if (dnsPayload.size() < 12) {
        return false;
    }

    const uint16_t qdcount = static_cast<uint16_t>((dnsPayload[4] << 8) | dnsPayload[5]);
    const uint16_t ancount = static_cast<uint16_t>((dnsPayload[6] << 8) | dnsPayload[7]);

    size_t off = 12;

    for (uint16_t i = 0; i < qdcount; ++i) {
        if (!skipDnsName(dnsPayload, off) || off + 4 > dnsPayload.size()) {
            return false;
        }
        off += 4;
    }

    for (uint16_t i = 0; i < ancount; ++i) {
        if (!skipDnsName(dnsPayload, off) || off + 10 > dnsPayload.size()) {
            return false;
        }

        const uint16_t type = static_cast<uint16_t>((dnsPayload[off] << 8) | dnsPayload[off + 1]);
        const uint16_t klass = static_cast<uint16_t>((dnsPayload[off + 2] << 8) | dnsPayload[off + 3]);
        const uint16_t rdlen = static_cast<uint16_t>((dnsPayload[off + 8] << 8) | dnsPayload[off + 9]);
        off += 10;

        if (off + rdlen > dnsPayload.size()) {
            return false;
        }

        if (type == 1 && klass == 1 && rdlen == 4) {
            char ipbuf[INET_ADDRSTRLEN]{};
            if (inet_ntop(AF_INET, dnsPayload.data() + off, ipbuf, sizeof(ipbuf)) != nullptr) {
                outIp = ipbuf;
                return true;
            }
            return false;
        }

        off += rdlen;
    }

    return false;
}

} // namespace

int main() {
    std::cout << "SOCKS5 UDP test started" << std::endl;

    const int tcp = connectTcp(kProxyHost, kProxyPort);
    if (tcp < 0) {
        std::cerr << "[ERROR] TCP connect failed: " << strerror(errno) << std::endl;
        return 1;
    }

    // 1) SOCKS5 greeting: only username/password auth
    {
        const uint8_t greeting[] = {0x05, 0x01, 0x02};
        if (!sendAll(tcp, greeting, sizeof(greeting))) {
            std::cerr << "[ERROR] Failed to send SOCKS greeting" << std::endl;
            close(tcp);
            return 1;
        }

        uint8_t resp[2]{};
        if (!recvExact(tcp, resp, sizeof(resp))) {
            std::cerr << "[ERROR] Failed to read SOCKS greeting response" << std::endl;
            close(tcp);
            return 1;
        }

        if (resp[0] != 0x05 || resp[1] != 0x02) {
            std::cerr << "[ERROR] Proxy does not accept username/password auth" << std::endl;
            close(tcp);
            return 1;
        }
    }

    // 2) Username/password auth (RFC 1929)
    {
        const std::string user = kProxyUser;
        const std::string pass = kProxyPass;
        if (user.empty() || user.size() > 255 || pass.empty() || pass.size() > 255) {
            std::cerr << "[ERROR] Invalid credentials length" << std::endl;
            close(tcp);
            return 1;
        }

        std::vector<uint8_t> auth;
        auth.reserve(3 + user.size() + pass.size());
        auth.push_back(0x01);
        auth.push_back(static_cast<uint8_t>(user.size()));
        auth.insert(auth.end(), user.begin(), user.end());
        auth.push_back(static_cast<uint8_t>(pass.size()));
        auth.insert(auth.end(), pass.begin(), pass.end());

        if (!sendAll(tcp, auth.data(), auth.size())) {
            std::cerr << "[ERROR] Failed to send auth request" << std::endl;
            close(tcp);
            return 1;
        }

        uint8_t authResp[2]{};
        if (!recvExact(tcp, authResp, sizeof(authResp))) {
            std::cerr << "[ERROR] Failed to read auth response" << std::endl;
            close(tcp);
            return 1;
        }

        if (authResp[0] != 0x01 || authResp[1] != 0x00) {
            std::cerr << "[ERROR] Auth failed" << std::endl;
            close(tcp);
            return 1;
        }
    }

    // 3) UDP ASSOCIATE request (use planned destination 77.88.8.8:53)
    std::string udpRelayIp;
    uint16_t udpRelayPort = 0;
    {
        in_addr dnsAssocAddr{};
        if (inet_pton(AF_INET, kDnsServerIp, &dnsAssocAddr) != 1) {
            std::cerr << "[ERROR] Invalid DNS server IP for UDP ASSOCIATE" << std::endl;
            close(tcp);
            return 1;
        }
        const uint8_t * dnsAssocAddrBytes = reinterpret_cast<const uint8_t *>(&dnsAssocAddr.s_addr);
        const uint8_t req[] = {
            0x05, 0x03, 0x00, 0x01,
            dnsAssocAddrBytes[0], dnsAssocAddrBytes[1], dnsAssocAddrBytes[2], dnsAssocAddrBytes[3],
            static_cast<uint8_t>((kDnsServerPort >> 8) & 0xFF),
            static_cast<uint8_t>(kDnsServerPort & 0xFF)
        };

        if (!sendAll(tcp, req, sizeof(req))) {
            std::cerr << "[ERROR] Failed to send UDP ASSOCIATE request" << std::endl;
            close(tcp);
            return 1;
        }

        uint8_t head[4]{};
        if (!recvExact(tcp, head, sizeof(head))) {
            std::cerr << "[ERROR] Failed to read UDP ASSOCIATE header" << std::endl;
            close(tcp);
            return 1;
        }

        if (head[0] != 0x05 || head[1] != 0x00) {
            std::cerr << "[ERROR] UDP ASSOCIATE rejected, REP=" << static_cast<int>(head[1]) << std::endl;
            close(tcp);
            return 1;
        }

        const uint8_t atyp = head[3];
        if (atyp == 0x01) {
            uint8_t ipv4[4]{};
            uint8_t port[2]{};
            if (!recvExact(tcp, ipv4, sizeof(ipv4)) || !recvExact(tcp, port, sizeof(port))) {
                std::cerr << "[ERROR] Failed to read IPv4 relay address" << std::endl;
                close(tcp);
                return 1;
            }
            char ipbuf[INET_ADDRSTRLEN]{};
            if (inet_ntop(AF_INET, ipv4, ipbuf, sizeof(ipbuf)) == nullptr) {
                std::cerr << "[ERROR] Invalid relay IPv4 address" << std::endl;
                close(tcp);
                return 1;
            }
            udpRelayIp = ipbuf;
            udpRelayPort = static_cast<uint16_t>((port[0] << 8) | port[1]);
        } else if (atyp == 0x03) {
            uint8_t nlen = 0;
            if (!recvExact(tcp, &nlen, 1)) {
                std::cerr << "[ERROR] Failed to read relay hostname length" << std::endl;
                close(tcp);
                return 1;
            }
            std::vector<uint8_t> host(nlen);
            uint8_t port[2]{};
            if (!recvExact(tcp, host.data(), host.size()) || !recvExact(tcp, port, sizeof(port))) {
                std::cerr << "[ERROR] Failed to read relay hostname" << std::endl;
                close(tcp);
                return 1;
            }
            udpRelayIp.assign(host.begin(), host.end());
            udpRelayPort = static_cast<uint16_t>((port[0] << 8) | port[1]);
        } else {
            std::cerr << "[ERROR] Unsupported relay address type: " << static_cast<int>(atyp) << std::endl;
            close(tcp);
            return 1;
        }

        if (udpRelayIp == "0.0.0.0") {
            udpRelayIp = kProxyHost;
        }

        std::cout << "UDP relay endpoint: " << udpRelayIp << ":" << udpRelayPort << std::endl;
    }

    // 4) Build DNS query and SOCKS5 UDP packet
    std::random_device rd;
    const uint16_t txid = static_cast<uint16_t>(rd());
    const std::vector<uint8_t> dnsQuery = buildDnsQuery(txid, kDomain);
    if (dnsQuery.empty()) {
        std::cerr << "[ERROR] Failed to build DNS query" << std::endl;
        close(tcp);
        return 1;
    }

    std::vector<uint8_t> udpPacket;
    udpPacket.reserve(10 + dnsQuery.size());
    udpPacket.push_back(0x00); // RSV
    udpPacket.push_back(0x00); // RSV
    udpPacket.push_back(0x00); // FRAG
    udpPacket.push_back(0x01); // ATYP IPv4

    in_addr dnsAddr{};
    if (inet_pton(AF_INET, kDnsServerIp, &dnsAddr) != 1) {
        std::cerr << "[ERROR] Invalid DNS server IP" << std::endl;
        close(tcp);
        return 1;
    }
    const uint8_t * dnsAddrBytes = reinterpret_cast<const uint8_t *>(&dnsAddr.s_addr);
    udpPacket.insert(udpPacket.end(), dnsAddrBytes, dnsAddrBytes + 4);
    udpPacket.push_back(static_cast<uint8_t>((kDnsServerPort >> 8) & 0xFF));
    udpPacket.push_back(static_cast<uint8_t>(kDnsServerPort & 0xFF));
    udpPacket.insert(udpPacket.end(), dnsQuery.begin(), dnsQuery.end());

    // 5) Send via UDP relay and wait response
    int udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp < 0) {
        std::cerr << "[ERROR] Failed to create UDP socket: " << strerror(errno) << std::endl;
        close(tcp);
        return 1;
    }

    timeval tv{};
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(udp, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    sockaddr_in relayAddr{};
    relayAddr.sin_family = AF_INET;
    relayAddr.sin_port = htons(udpRelayPort);
    if (inet_pton(AF_INET, udpRelayIp.c_str(), &relayAddr.sin_addr) != 1) {
        std::cerr << "[ERROR] Relay endpoint is not IPv4 literal: " << udpRelayIp << std::endl;
        close(udp);
        close(tcp);
        return 1;
    }

    const ssize_t sent = sendto(
        udp,
        udpPacket.data(),
        udpPacket.size(),
        0,
        reinterpret_cast<sockaddr *>(&relayAddr),
        sizeof(relayAddr)
    );

    if (sent < 0 || static_cast<size_t>(sent) != udpPacket.size()) {
        std::cerr << "[ERROR] Failed to send UDP packet to relay: " << strerror(errno) << std::endl;
        close(udp);
        close(tcp);
        return 1;
    }

    std::vector<uint8_t> recvBuf(2048);
    ssize_t n = -1;
    for (int attempt = 1; attempt <= 3; ++attempt) {
        const ssize_t retrySent = sendto(
            udp,
            udpPacket.data(),
            udpPacket.size(),
            0,
            reinterpret_cast<sockaddr *>(&relayAddr),
            sizeof(relayAddr)
        );
        if (retrySent < 0 || static_cast<size_t>(retrySent) != udpPacket.size()) {
            std::cerr << "[ERROR] Failed to send UDP packet to relay: " << strerror(errno) << std::endl;
            close(udp);
            close(tcp);
            return 1;
        }

        sockaddr_in from{};
        socklen_t fromLen = sizeof(from);
        n = recvfrom(
            udp,
            recvBuf.data(),
            recvBuf.size(),
            0,
            reinterpret_cast<sockaddr *>(&from),
            &fromLen
        );
        if (n > 0) {
            break;
        }
    }

    if (n <= 0) {
        std::cerr << "[ERROR] No UDP response from relay after retries (timeout or error): " << strerror(errno) << std::endl;
        close(udp);
        close(tcp);
        return 1;
    }

    recvBuf.resize(static_cast<size_t>(n));

    // 6) Parse SOCKS5 UDP header from response
    if (recvBuf.size() < 10 || recvBuf[0] != 0x00 || recvBuf[1] != 0x00 || recvBuf[2] != 0x00) {
        std::cerr << "[ERROR] Invalid SOCKS5 UDP response header" << std::endl;
        close(udp);
        close(tcp);
        return 1;
    }

    size_t off = 3;
    const uint8_t atyp = recvBuf[off++];
    if (atyp == 0x01) {
        off += 4;
    } else if (atyp == 0x03) {
        if (off >= recvBuf.size()) {
            std::cerr << "[ERROR] Truncated UDP response (domain length)" << std::endl;
            close(udp);
            close(tcp);
            return 1;
        }
        off += 1 + recvBuf[off];
    } else if (atyp == 0x04) {
        off += 16;
    } else {
        std::cerr << "[ERROR] Unknown ATYP in UDP response" << std::endl;
        close(udp);
        close(tcp);
        return 1;
    }

    if (off + 2 > recvBuf.size()) {
        std::cerr << "[ERROR] Truncated UDP response (port)" << std::endl;
        close(udp);
        close(tcp);
        return 1;
    }
    off += 2;

    if (off >= recvBuf.size()) {
        std::cerr << "[ERROR] Empty DNS payload" << std::endl;
        close(udp);
        close(tcp);
        return 1;
    }

    std::vector<uint8_t> dnsResp(recvBuf.begin() + static_cast<long>(off), recvBuf.end());
    if (dnsResp.size() < 12) {
        std::cerr << "[ERROR] DNS response too short" << std::endl;
        close(udp);
        close(tcp);
        return 1;
    }

    const uint16_t rxid = static_cast<uint16_t>((dnsResp[0] << 8) | dnsResp[1]);
    const uint16_t flags = static_cast<uint16_t>((dnsResp[2] << 8) | dnsResp[3]);
    const uint16_t rcode = static_cast<uint16_t>(flags & 0x000F);
    const bool qr = (flags & 0x8000) != 0;

    if (!qr || rxid != txid || rcode != 0) {
        std::cerr << "[ERROR] DNS response validation failed"
                  << " (qr=" << qr
                  << ", txid=0x" << std::hex << txid << ", rxid=0x" << rxid << std::dec
                  << ", rcode=" << rcode << ")"
                  << std::endl;
        close(udp);
        close(tcp);
        return 1;
    }

    std::string answerIp;
    if (!parseFirstARecord(dnsResp, answerIp)) {
        std::cerr << "[ERROR] DNS response received, but no A record found for " << kDomain << std::endl;
        close(udp);
        close(tcp);
        return 1;
    }

    std::cout << "[OK] SOCKS5 UDP works. DNS A(" << kDomain << ") = " << answerIp << std::endl;

    close(udp);
    close(tcp);
    return 0;
}
