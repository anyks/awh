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
sudo ./tun_macos server 10.0.0.1 10.0.0.2 2049 0.0.0.0 2049 udp
```

### UDP Client
```bash
# Replace 1.2.3.4 with the actual Server IP
sudo ./tun_macos client 10.0.0.2 10.0.0.1 2049 1.2.3.4 2049 udp
```

## Note
MacOS uses `utun` devices. The interface name will likely be `utun0`, `utun1`, etc.
The code automatically handles the 4-byte protocol family header required by `utun`.


# Удалите возможные старые маршруты
```bash
sudo route delete -net 10.0.0.0/8 2>/dev/null

# Добавьте маршрут до всей подсети через utun6
sudo route add -net 10.0.0.0/8 -interface utun6

# Или
sudo route add -host 10.0.0.1 -interface utun6
```

## Test client
```bash
nc -u 10.0.0.1 8000
Hello World!!!
```
