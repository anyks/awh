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
# Replace 0.0.0.0 with client IP if known, or leave as 0.0.0.0 to learn dynamically
pfexec ./tun_solaris server 10.0.0.1 10.0.0.2 2049 0.0.0.0 2049 udp
```

### UDP Client
```bash
# Replace 1.2.3.4 with the actual Server IP
pfexec ./tun_solaris client 10.0.0.2 10.0.0.1 2049 1.2.3.4 2049 udp
```

## Note
The application attempts to create `tun0`, `tun1` by iterating PPAs. It calls `ifconfig plumb` automatically.
Ensure you have permissions to plumb interfaces.
