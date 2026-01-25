#include "tunnel.hpp"
#include <iostream>
#include <string>
#include <stdexcept>
#include <windows.h>
#include <wincrypt.h>
#include <iphlpapi.h>
#include <stdlib.h>
#include <vector>

// Define Wintun API types
typedef void* WINTUN_ADAPTER_HANDLE;
typedef void* WINTUN_SESSION_HANDLE;
typedef UINT64 WINTUN_LOGGER_CALLBACK; // Simplified

typedef WINTUN_ADAPTER_HANDLE (WINAPI *FWintunCreateAdapter)(LPCWSTR, LPCWSTR, const GUID*);
typedef void (WINAPI *FWintunCloseAdapter)(WINTUN_ADAPTER_HANDLE);
typedef WINTUN_SESSION_HANDLE (WINAPI *FWintunStartSession)(WINTUN_ADAPTER_HANDLE, DWORD);
typedef void (WINAPI *FWintunEndSession)(WINTUN_SESSION_HANDLE);
typedef void* (WINAPI *FWintunGetReadWaitEvent)(WINTUN_SESSION_HANDLE);
typedef BYTE* (WINAPI *FWintunReceivePacket)(WINTUN_SESSION_HANDLE, DWORD*);
typedef void (WINAPI *FWintunReleaseReceivePacket)(WINTUN_SESSION_HANDLE, const BYTE*);
typedef BYTE* (WINAPI *FWintunAllocateSendPacket)(WINTUN_SESSION_HANDLE, DWORD);
typedef void (WINAPI *FWintunSendPacket)(WINTUN_SESSION_HANDLE, const BYTE*);

// Global pointers to DLL functions
static FWintunCreateAdapter      pWintunCreateAdapter = nullptr;
static FWintunCloseAdapter       pWintunCloseAdapter = nullptr;
static FWintunStartSession       pWintunStartSession = nullptr;
static FWintunEndSession         pWintunEndSession = nullptr;
static FWintunGetReadWaitEvent   pWintunGetReadWaitEvent = nullptr;
static FWintunReceivePacket      pWintunReceivePacket = nullptr;
static FWintunReleaseReceivePacket pWintunReleaseReceivePacket = nullptr;
static FWintunAllocateSendPacket pWintunAllocateSendPacket = nullptr;
static FWintunSendPacket         pWintunSendPacket = nullptr;

static std::wstring StringToWString(const std::string& s) {
    int len;
    int slength = (int)s.length() + 1;
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0); 
    std::wstring buf(len, L'\0');
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, &buf[0], len);
    return buf;
}

std::string TunInterface::create(const std::string& name_template, TunHandle& out_handle) {
    // 1. Load Wintun DLL
    HMODULE lib = LoadLibrary("wintun.dll");
    if (!lib) throw std::runtime_error("Could not load wintun.dll. Please download it from wintun.net");

    pWintunCreateAdapter = (FWintunCreateAdapter)GetProcAddress(lib, "WintunCreateAdapter");
    pWintunCloseAdapter = (FWintunCloseAdapter)GetProcAddress(lib, "WintunCloseAdapter");
    pWintunStartSession = (FWintunStartSession)GetProcAddress(lib, "WintunStartSession");
    pWintunEndSession = (FWintunEndSession)GetProcAddress(lib, "WintunEndSession");
    pWintunGetReadWaitEvent = (FWintunGetReadWaitEvent)GetProcAddress(lib, "WintunGetReadWaitEvent");
    pWintunReceivePacket = (FWintunReceivePacket)GetProcAddress(lib, "WintunReceivePacket");
    pWintunReleaseReceivePacket = (FWintunReleaseReceivePacket)GetProcAddress(lib, "WintunReleaseReceivePacket");
    pWintunAllocateSendPacket = (FWintunAllocateSendPacket)GetProcAddress(lib, "WintunAllocateSendPacket");
    pWintunSendPacket = (FWintunSendPacket)GetProcAddress(lib, "WintunSendPacket");

    if (!pWintunCreateAdapter) throw std::runtime_error("Invalid wintun.dll");

    // 2. Create Adapter
    // Wintun defines "Example" as pool. 
    GUID guid;
    CoCreateGuid(&guid);
    
    std::string name = "tun0"; // Hardcoded for simplicity or logic to find free name
    if (name_template != "tun") {
        name = name_template;
        if (name.find("%") != std::string::npos) name = "tun0"; // fallback
    }

    std::wstring wname = StringToWString(name);
    std::wstring wtype = L"Wintun";
    
    // Create Adapter
    // WintunCreateAdapter(Name, TunnelType, GUID)
    // Note: WintunCreateAdapter might fail if name exists. 
    WINTUN_ADAPTER_HANDLE adapter = pWintunCreateAdapter(wname.c_str(), wtype.c_str(), &guid);
    if (!adapter) throw std::runtime_error("Failed to create Wintun adapter");

    // 3. Start Session (0x400000 capacity default)
    WINTUN_SESSION_HANDLE session = pWintunStartSession(adapter, 0x400000);
    if (!session) {
        pWintunCloseAdapter(adapter);
        throw std::runtime_error("Failed to start Wintun session");
    }

    out_handle.dll = lib;
    out_handle.adapter = adapter;
    out_handle.session = session;
    out_handle.read_event = (HANDLE)pWintunGetReadWaitEvent(session);

    return name;
}

void TunInterface::destroy(TunHandle& handle) {
    if (handle.session && pWintunEndSession) pWintunEndSession(handle.session);
    if (handle.adapter && pWintunCloseAdapter) pWintunCloseAdapter(handle.adapter);
    if (handle.dll) FreeLibrary(handle.dll);
}

#include <netioapi.h> // For IP Helper API functions

// Helper to calculate prefix length from mask string or integer
UINT8 GetPrefixLength(const std::string& mask_or_len) {
    // If it contains '.', it's a mask like 255.255.255.0
    if (mask_or_len.find('.') != std::string::npos) {
        unsigned long mask = inet_addr(mask_or_len.c_str());
        UINT8 len = 0;
        while (mask) {
            if (mask & 1) len++;
            mask >>= 1;
        }
        return len > 0 ? len : 24; // Approximation or assume little endian check needed
    }
    // Otherwise assume it is "24", "30" etc.
    try {
        return (UINT8)std::stoi(mask_or_len);
    } catch(...) {
        return 24;
    }
}

void TunInterface::set_ip(const std::string& ifname, const std::string& local_ip, const std::string& mask_len) {
    // Use IP Helper API (CreateUnicastIpAddressEntry)
    
    // 1. Get Interface LUID or Index
    std::wstring wname = StringToWString(ifname);
    NET_LUID luid;
    if (ConvertInterfaceAliasToLuid(wname.c_str(), &luid) != NO_ERROR) {
        throw std::runtime_error("Failed to find interface LUID for: " + ifname);
    }

    // 2. Initialize IP Row
    MIB_UNICASTIPADDRESS_ROW row;
    InitializeUnicastIpAddressEntry(&row);
    row.InterfaceLuid = luid;
    row.Address.si_family = AF_INET;
    
    // Parse IP
    if (inet_pton(AF_INET, local_ip.c_str(), &row.Address.Ipv4.sin_addr) != 1) {
        throw std::runtime_error("Invalid IP address");
    }

    // Parse Prefix
    row.OnLinkPrefixLength = GetPrefixLength(mask_len);
    row.DadState = IpDadStatePreferred; // Skip DAD for quicker up

    // 3. Apply
    // We try to delete existing first? Wintun starts empty, so Create is enough.
    DWORD ret = CreateUnicastIpAddressEntry(&row);
    if (ret != NO_ERROR && ret != ERROR_OBJECT_ALREADY_EXISTS) {
        throw std::runtime_error("CreateUnicastIpAddressEntry failed: " + std::to_string(ret));
    }
    
    std::cout << "IP Address configured via IP Helper API." << std::endl;
}
