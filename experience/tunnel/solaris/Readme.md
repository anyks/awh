# Solaris 11 / OpenIndiana TUN/TAP Example

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Running

Root privileges are required. This example requires the `tun` driver to be loaded.

### UDP Server
```bash
pfexec ./tun_solaris server 10.0.0.1 10.0.0.2 5000 <client_ip> 5000 udp
```

### UDP Client
```bash
pfexec ./tun_solaris client 10.0.0.2 10.0.0.1 5000 <server_ip> 5000 udp
```

## Note
The application attempts to create `tun0`, `tun1` by iterating PPAs. It calls `ifconfig plumb` automatically.
Ensure you have permissions to plumb interfaces.
