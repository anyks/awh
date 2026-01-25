# Windows MinGW + Wintun Example

## Prerequisites
1.  **MinGW-w64** compiler.
2.  **wintun.dll**: You must download `wintun.dll` (standard version) from [wintun.net](https://www.wintun.net/) or copy it from a WireGuard installation.
    *   Place `wintun.dll` in the same directory as the executable.

## Building

```bash
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
mingw32-make
```

## Running

Run as Administrator. Wintun will automatically create an interface.

### UDP Server
```bash
tun_windows.exe server 10.0.0.1 24 5000 <client_ip> 5000 udp
```
*Note: The second argument is the Mask Length (CIDR) for Wintun.*

### UDP Client
```bash
tun_windows.exe client 10.0.0.2 24 5000 <server_ip> 5000 udp
```
