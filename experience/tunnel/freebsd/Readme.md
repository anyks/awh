# FreeBSD TUN/TAP Example

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
sudo ./tun_freebsd server 10.0.0.1 10.0.0.2 2049 0.0.0.0 2049 udp
```

### UDP Client
```bash
# Replace 1.2.3.4 with the actual Server IP
sudo ./tun_freebsd client 10.0.0.2 10.0.0.1 2049 1.2.3.4 2049 udp
```

## Note
Uses `/dev/tun` cloning device. The example assumes `AF_INET` header (4 bytes) is present on the TUN interface.

> sudo ipfw add 10 allow ip from any to any via tun*
