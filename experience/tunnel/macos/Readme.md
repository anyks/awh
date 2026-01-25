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
sudo ./tun_macos server 10.0.0.1 10.0.0.2 5000 <client_ip> 5000 udp
```

### UDP Client
```bash
sudo ./tun_macos client 10.0.0.2 10.0.0.1 5000 <server_ip> 5000 udp
```

## Note
MacOS uses `utun` devices. The interface name will likely be `utun0`, `utun1`, etc.
The code automatically handles the 4-byte protocol family header required by `utun`.
