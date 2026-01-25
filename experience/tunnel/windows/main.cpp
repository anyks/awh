#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include "tunnel.hpp"

#pragma comment(lib, "ws2_32.lib")

// Types for dynamic loading to keep main clean, but simplified:
typedef BYTE* (WINAPI *FWintunReceivePacket_t)(void*, DWORD*);
typedef void (WINAPI *FWintunReleaseReceivePacket_t)(void*, const BYTE*);
typedef BYTE* (WINAPI *FWintunAllocateSendPacket_t)(void*, DWORD);
typedef void (WINAPI *FWintunSendPacket_t)(void*, const BYTE*);

void cleanup(TunHandle& tun, SOCKET sock) {
    TunInterface::destroy(tun);
    if (sock != INVALID_SOCKET) closesocket(sock);
    WSACleanup();
}

int main(int argc, char** argv) {
     if (argc < 7) {
        std::cerr << "Usage: " << argv[0] << " <mode: server|client> <tun_local_ip> <tun_peer_ip> <local_port> <remote_ip> <remote_port> [protocol]" << std::endl;
        std::cerr << "Note: Wintun.dll must be present in the directory." << std::endl;
        return 1;
    }

    std::string mode = argv[1];
    std::string tun_local = argv[2];
    std::string tun_peer = argv[3]; // Used as mask length for Wintun setup? Or just Peer? 
    // Example: 10.0.0.1 24 (not peer ip if wintun wants mask)
    // Actually the user requirements say "local address of tunnel and destination address".
    // For Wintun, usually you set IP/Mask. I'll treat 2nd arg as mask-length if < 32, or generate /30.
    // Simplifying: User passes 2 IPs. I will calculate mask 30 or similar.
    // Wait, the set_ip function I wrote for Wintun usage takes mask_len.
    // I will hardcode /30 or /24 for simplicity or try to parse.
    std::string mask_len = "30"; 

    int local_port = std::stoi(argv[4]);
    std::string remote_ip = argv[5];
    int remote_port = std::stoi(argv[6]);
    std::string protocol = (argc > 7) ? argv[7] : "udp";

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    TunHandle tun_handle = {0};
    SOCKET net_sock = INVALID_SOCKET;

    try {
        std::string name = TunInterface::create("tun", tun_handle);
        std::cout << "Created Wintun adapter: " << name << std::endl;

        TunInterface::set_ip(name, tun_local, mask_len);

        // Network Socket
        int sq_type = (protocol == "tcp") ? SOCK_STREAM : SOCK_DGRAM;
        net_sock = socket(AF_INET, sq_type, 0);
        
        struct sockaddr_in local;
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = INADDR_ANY;
        local.sin_port = htons(local_port);
        
        if (bind(net_sock, (struct sockaddr*)&local, sizeof(local)) < 0) throw std::runtime_error("Bind failed");

        struct sockaddr_in remote;
        remote.sin_family = AF_INET;
        remote.sin_addr.s_addr = inet_addr(remote_ip.c_str());
        remote.sin_port = htons(remote_port);

        if (mode == "client" && sq_type == SOCK_STREAM) {
            if (connect(net_sock, (struct sockaddr*)&remote, sizeof(remote)) < 0) throw std::runtime_error("Connect failed");
        } else if (mode == "server" && sq_type == SOCK_STREAM) {
            listen(net_sock, 1);
            std::cout << "Waiting for connection..." << std::endl;
            SOCKET client = accept(net_sock, NULL, NULL);
            closesocket(net_sock);
            net_sock = client;
            std::cout << "Connected." << std::endl;
        }
        
        std::cout << "Tunnel running via Wintun..." << std::endl;

        // Load Function pointers locally for speed/convenience or assume globals available?
        // Since main.cpp doesn't see tunnel.cpp static globals, we can:
        // 1. Expose them in tunnel.hpp
        // 2. Or re-load them (inefficient)
        // 3. Or just use GetProcAddress on tun_handle.dll
        FWintunReceivePacket_t ReceivePacket = (FWintunReceivePacket_t)GetProcAddress(tun_handle.dll, "WintunReceivePacket");
        FWintunReleaseReceivePacket_t ReleasePacket = (FWintunReleaseReceivePacket_t)GetProcAddress(tun_handle.dll, "WintunReleaseReceivePacket");
        FWintunAllocateSendPacket_t AllocatePacket = (FWintunAllocateSendPacket_t)GetProcAddress(tun_handle.dll, "WintunAllocateSendPacket");
        FWintunSendPacket_t SendPacket = (FWintunSendPacket_t)GetProcAddress(tun_handle.dll, "WintunSendPacket");

        HANDLE events[2];
        events[0] = tun_handle.read_event;
        events[1] = WSACreateEvent();
        WSAEventSelect(net_sock, events[1], FD_READ | FD_CLOSE);

        char net_buf[4096];

        while (true) {
            DWORD wait = WaitForMultipleObjects(2, events, FALSE, INFINITE);
            if (wait == WAIT_OBJECT_0) { // TUN Data Available
                DWORD packetSize;
                BYTE* packet = ReceivePacket(tun_handle.session, &packetSize);
                if (packet) {
                    if (sq_type == SOCK_DGRAM)
                        sendto(net_sock, (const char*)packet, packetSize, 0, (struct sockaddr*)&remote, sizeof(remote));
                    else 
                        send(net_sock, (const char*)packet, packetSize, 0);
                    
                    ReleasePacket(tun_handle.session, packet);
                }
            }
            else if (wait == WAIT_OBJECT_0 + 1) { // Socket Data
                WSANETWORKEVENTS ne;
                WSAEnumNetworkEvents(net_sock, events[1], &ne);
                if (ne.lNetworkEvents & FD_READ) {
                    int n = recv(net_sock, net_buf, sizeof(net_buf), 0);
                    if (n > 0) {
                        // Write to Wintun
                        BYTE* packet = AllocatePacket(tun_handle.session, n);
                        if (packet) {
                            memcpy(packet, net_buf, n);
                            SendPacket(tun_handle.session, packet);
                        }
                    }
                }
                if (ne.lNetworkEvents & FD_CLOSE) break;
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    cleanup(tun_handle, net_sock);
    return 0;
}
