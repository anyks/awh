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
sudo ./tun_freebsd server 10.0.0.1 10.0.0.2 5000 <client_ip> 5000 udp
```

### UDP Client
```bash
sudo ./tun_freebsd client 10.0.0.2 10.0.0.1 5000 <server_ip> 5000 udp
```

## Note
Uses `/dev/tun` cloning device. The example assumes `AF_INET` header (4 bytes) is present on the TUN interface.
