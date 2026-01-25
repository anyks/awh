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
sudo ./tun_macos server 10.0.0.1 10.0.0.2 5000 0.0.0.0 5000 udp
```

### UDP Client
```bash
# Replace 1.2.3.4 with the actual Server IP
sudo ./tun_macos client 10.0.0.2 10.0.0.1 5000 1.2.3.4 5000 udp
```

## Note
MacOS uses `utun` devices. The interface name will likely be `utun0`, `utun1`, etc.
The code automatically handles the 4-byte protocol family header required by `utun`.
