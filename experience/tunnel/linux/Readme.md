# Linux TUN/TAP Example

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Running

You need root privileges to manage network interfaces.

### UDP Example

**Server (Listen on port 5000, Tun IP 10.0.0.1)**
```bash
# Replace 0.0.0.0 with client IP if known, or leave as 0.0.0.0 to learn dynamically
sudo ./tun_linux server 10.0.0.1 10.0.0.2 5000 0.0.0.0 5000 udp
```

**Client (Remote IP, Tun IP 10.0.0.2)**
```bash
# Replace 1.2.3.4 with the actual Server IP
sudo ./tun_linux client 10.0.0.2 10.0.0.1 5000 1.2.3.4 5000 udp
```

### Verification (Ping)
From Client:
```bash
ping 10.0.0.1
```
From Server:
```bash
ping 10.0.0.2
```

## Supported Protocols
- **udp** (SOCK_DGRAM)
- **tcp** (SOCK_STREAM)
- **seq** (SOCK_SEQPACKET) (Ensure kernel support)
