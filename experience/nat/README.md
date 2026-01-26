# NAT Port Mapping Demo (UPnP / NAT-PMP / PCP)

Демонстрация проброса порта через UPnP, NAT-PMP и PCP.

## Требования

### Клиент (macOS)
- macOS 10.15+
- Xcode Command Line Tools
- Библиотека `miniupnpc` (установить через Homebrew)

### Сервер (FreeBSD / Linux)
- FreeBSD 13+ или Linux
- GCC/Clang

### Протоколы
- UPnP IGD: использует miniupnpc
- NAT-PMP: реализован вручную (RFC 6886)
- PCP: реализован вручную (RFC 6887)

## Установка зависимостей (macOS)

```bash
brew install miniupnpc
```

## На клиенте (macOS):

```bash
mkdir build && cd build
cmake .. -DBUILD_CLIENT=ON
make
./client/client --method upnp    # или natpmp, pcp
```

## На сервере (FreeBSD/Linux):

```bash
mkdir build && cd build
cmake .. -DBUILD_SERVER=ON
make
./server/server 0.0.0.0 12345
```
