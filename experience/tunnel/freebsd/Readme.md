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

## Firewall
```bash
sudo ipfw add 10 allow ip from any to any via tun*
```

## Destroy tun interface
```bash
sudo ifconfig tun0 destroy
```

## Test server
```bash
nc -u -l 10.0.0.1 8000
```

## ✅ Правильная настройка для VPN-сервера
Сервер должен быть шлюзом для целой подсети, а не точкой к одному клиенту.

### Шаг 1: Назначьте серверу IP из подсети

```bash
# Сервер получает 10.8.0.1 в подсети /24
$ ifconfig tun0 10.8.0.1 10.8.0.1 netmask 255.255.255.0
```

> 💡 Обратите внимание: destination = сам себе (10.8.0.1). Это говорит ядру: «Я — шлюз для всей подсети 10.8.0.0/24».

### Шаг 2: Включите IP forwarding

```bash
$ sysctl net.inet.ip.forwarding=1
```

### Шаг 3: На клиенте назначьте IP из той же подсети

```bash
# Клиент A
$ ifconfig tun0 10.8.0.2 10.8.0.1 netmask 255.255.255.0

# Клиент B
$ ifconfig tun0 10.8.0.3 10.8.0.1 netmask 255.255.255.0
```

#### 🔄 Как теперь работает трафик
- Клиент A (10.8.0.2) → пингует Клиента B (10.8.0.3)
- Пакет уходит на сервер (потому что маска /24)
- Сервер получает пакет → ядро видит: «10.8.0.3 — из моей подсети»
- Ядро пытается доставить его локально, но не находит интерфейса с 10.8.0.3
- Ваше приложение перехватывает этот пакет через read(tun_fd, ...)
- Вы смотрите destination IP = 10.8.0.3 → отправляете нужному клиенту

> ✅ Сервер не "знает" клиентов заранее — он просто маршрутизирует по IP.

#### 🧠 Почему destination = self?

**На FreeBSD (и других BSD):**
- Если вы указываете destination = self, ядро считает интерфейс multi-point (как Ethernet),
- Если destination ≠ self, ядро считает его point-to-point (только один хост).

> 📌 Это критическое отличие от Linux, где ip addr add 10.8.0.1/24 dev tun0 автоматически создаёт multi-point интерфейс.

#### ✅ Итог

**Для VPN-сервера на FreeBSD:**

```bash
$ ifconfig tun0 10.8.0.1 10.8.0.1 netmask 255.255.255.0
```

> — destination должен быть равен самому себе, чтобы ядро принимало пакеты для всей подсети.

