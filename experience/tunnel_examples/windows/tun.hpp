#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>

// Минимальная обёртка Wintun (динамическая загрузка)
struct WintunAPI {
    HMODULE dll;
    using CreateAdapterFn = decltype(&WintunCreateAdapter);
    using CloseAdapterFn = decltype(&WintunCloseAdapter);
    using ReceivePacketFn = decltype(&WintunReceivePacket);
    using ReleaseReceivePacketFn = decltype(&WintunReleaseReceivePacket);
    using AllocateSendPacketFn = decltype(&WintunAllocateSendPacket);
    using SendPacketFn = decltype(&WintunSendPacket);

    CreateAdapterFn CreateAdapter;
    CloseAdapterFn CloseAdapter;
    ReceivePacketFn ReceivePacket;
    ReleaseReceivePacketFn ReleaseReceivePacket;
    AllocateSendPacketFn AllocateSendPacket;
    SendPacketFn SendPacket;

    WintunAPI() {
        dll = LoadLibraryA("wintun.dll");
        if (!dll) throw std::runtime_error("wintun.dll not found");
        CreateAdapter = (CreateAdapterFn)GetProcAddress(dll, "WintunCreateAdapter");
        CloseAdapter = (CloseAdapterFn)GetProcAddress(dll, "WintunCloseAdapter");
        ReceivePacket = (ReceivePacketFn)GetProcAddress(dll, "WintunReceivePacket");
        ReleaseReceivePacket = (ReleaseReceivePacketFn)GetProcAddress(dll, "WintunReleaseReceivePacket");
        AllocateSendPacket = (AllocateSendPacketFn)GetProcAddress(dll, "WintunAllocateSendPacket");
        SendPacket = (SendPacketFn)GetProcAddress(dll, "WintunSendPacket");
        if (!CreateAdapter) throw std::runtime_error("Failed to load Wintun functions");
    }
    ~WintunAPI() { if (dll) FreeLibrary(dll); }
    static WintunAPI& instance() { static WintunAPI w; return w; }
};

struct packet_t { std::vector<uint8_t> data; };

struct handle_t {
    void* adapter = nullptr;
    HANDLE rx_event = nullptr;
    std::string name;
};

inline handle_t create(const char* name = "AWH-Tunnel") {
    auto& w = WintunAPI::instance();
    GUID guid = {0x12345678, 0x1234, 0x1234, {0x12,0x34,0x12,0x34,0x12,0x34,0x12,0x34}};
    WCHAR wname[256];
    MultiByteToWideChar(CP_UTF8, 0, name, -1, wname, 256);
    void* adapter = w.CreateAdapter(wname, L"AWH", &guid);
    if (!adapter) throw std::runtime_error("WintunCreateAdapter failed");
    return {adapter, CreateEvent(nullptr, TRUE, FALSE, nullptr), std::string(name)};
}

// Упрощённая версия (полная — см. example в Wintun)
// Здесь — только отправка; приём требует отдельного потока
inline ssize_t write_packet(handle_t& h, const packet_t& pkt) {
    auto& w = WintunAPI::instance();
    BYTE* frame = w.AllocateSendPacket(h.adapter, (DWORD)pkt.data.size());
    if (!frame) return -1;
    memcpy(frame, pkt.data.data(), pkt.data.size());
    w.SendPacket(h.adapter, frame);
    return pkt.data.size();
}

// Приём не реализован здесь для краткости — см. официальный пример
// Для демонстрации будем только отправлять

inline void configure_interface(const char* ifname, const char* ip, const char* mask, const char* = nullptr) {
    std::cout << "Run as Administrator:\n";
    std::cout << "  netsh interface ipv4 set address \"" << ifname 
              << "\" static " << ip << " " << mask << "\n";
    Sleep(3000);
}
