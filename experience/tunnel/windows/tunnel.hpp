#ifndef WINDOWS_TUNNEL_HPP
#define WINDOWS_TUNNEL_HPP

#include <string>
#include <windows.h>

#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")

// Handle wrapper for Wintun
struct TunHandle {
    HMODULE dll;          // Handle to wintun.dll
    void* adapter;        // WINTUN_ADAPTER_HANDLE
    void* session;        // WINTUN_SESSION_HANDLE
    HANDLE read_event;
};

class TunInterface {
public:
    static std::string create(const std::string& name_template, TunHandle& out_handle);
    static void destroy(TunHandle& handle);
    static void set_ip(const std::string& ifname, const std::string& local_ip, const std::string& mask_len); // mask as length (24)
};

#endif // WINDOWS_TUNNEL_HPP
