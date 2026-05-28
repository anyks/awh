#!/usr/bin/env python3
import socket
import struct
import sys

# === НАСТРОЙКИ ===
PROXY_IP    = "217.29.53.105"
PROXY_PORT  = 11613          # Порт управления SOCKS5
PROXY_USER  = "8J0sHd"  # ← Укажите логин
PROXY_PASS  = "G4DfSK"   # ← Укажите пароль
TARGET_IP   = "77.88.8.8"   # DNS для теста
TARGET_PORT = 53
TIMEOUT     = 5.0           # Секунды ожидания
# =================

def main():
    print(f"[*] Подключение к SOCKS5: {PROXY_IP}:{PROXY_PORT}")
    
    tcp = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    tcp.settimeout(TIMEOUT)
    try:
        tcp.connect((PROXY_IP, PROXY_PORT))
    except Exception as e:
        print(f"[✗] Не удалось подключиться по TCP: {e}")
        return

    try:
        # 1. Handshake: предлагаем NO_AUTH(0x00) и USERPASS(0x02)
        tcp.sendall(b'\x05\x02\x00\x02')
        resp = tcp.recv(2)
        if len(resp) < 2:
            print("[✗] Нет ответа на handshake")
            return

        method = resp[1]
        if method == 0x00:
            print("[✓] Аутентификация не требуется")
        elif method == 0x02:
            print("[*] Сервер требует аутентификацию (Username/Password)...")
            if not PROXY_USER or PROXY_PASS is None:
                print("[✗] Заполните PROXY_USER и PROXY_PASS в настройках!")
                return

            user_b = PROXY_USER.encode('utf-8')
            pass_b = PROXY_PASS.encode('utf-8')
            
            # RFC 1929: VER(1) + ULEN(1) + UNAME + PLEN(1) + PASSWD
            auth_req = b'\x01' + bytes([len(user_b)]) + user_b + bytes([len(pass_b)]) + pass_b
            tcp.sendall(auth_req)
            
            auth_resp = tcp.recv(2)
            if len(auth_resp) < 2 or auth_resp[1] != 0x00:
                print(f"[✗] Аутентификация не пройдена (STATUS=0x{auth_resp[1]:02x})")
                return
            print("[✓] Аутентификация успешна")
        elif method == 0xFF:
            print("[✗] Сервер не поддерживает предложенные методы")
            return
        else:
            print(f"[✗] Неизвестный метод: 0x{method:02x}")
            return

        # 2. UDP ASSOCIATE Request
        print("[*] Отправка UDP ASSOCIATE...")
        tcp.sendall(b'\x05\x03\x00\x01\x00\x00\x00\x00\x00\x00')
        resp = tcp.recv(10)
        if len(resp) < 10:
            print("[✗] Короткий ответ на UDP ASSOCIATE")
            return

        rep = resp[1]
        if rep != 0x00:
            codes = {1:"general failure", 2:"not allowed", 3:"net unreachable",
                     4:"host unreachable", 5:"refused", 6:"TTL expired", 7:"cmd not supported"}
            print(f"[✗] Сервер отклонил UDP ASSOCIATE: REP=0x{rep:02x} ({codes.get(rep, 'unknown')})")
            return

        bnd_ip   = socket.inet_ntoa(resp[4:8])
        bnd_port = struct.unpack('>H', resp[8:10])[0]
        if bnd_ip == '0.0.0.0':
            bnd_ip = PROXY_IP
        print(f"[✓] UDP-релей выделен: {bnd_ip}:{bnd_port}")

        # 3. Отправка тестового пакета
        udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        udp.settimeout(TIMEOUT)
        
        udp_header = b'\x00\x00\x00\x01' + socket.inet_aton(TARGET_IP) + struct.pack('>H', TARGET_PORT)
        dns_query  = b'\x12\x34\x01\x00\x00\x01\x00\x00\x00\x00\x00\x00\x05anyks\x03com\x00\x00\x01\x00\x01'
        packet     = udp_header + dns_query

        print(f"[*] Отправка {len(packet)} байт на {bnd_ip}:{bnd_port}...")
        udp.sendto(packet, (bnd_ip, bnd_port))

        # 4. Ожидание ответа
        try:
            data, addr = udp.recvfrom(2048)
            print(f"[✓] Получен ответ от {addr}: {len(data)} байт")
            if len(data) > 12 and data[10:12] == b'\x12\x34' and (data[12] & 0x80):
                print("[🟢] УСПЕХ: SOCKS5 UDP работает корректно!")
            else:
                print("[🟡] Получены данные, но структура не похожа на DNS-ответ.")
        except socket.timeout:
            print("[🔴] ТАЙМАУТ: Сервер не ответил по UDP.")
        except Exception as e:
            print(f"[✗] Ошибка UDP: {e}")
    finally:
        udp.close()
        tcp.close()

if __name__ == "__main__":
    main()
