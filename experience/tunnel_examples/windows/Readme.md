Требуется: wintun.dll в той же папке, что и exe.

Скачайте wintun.dll → положите в папку с исходниками.
Соберите в Visual Studio или через MinGW:

g++ -std=c++17 -O2 -o tunnel-server server.cpp tunnel.cpp -lws2_32

Запустите от Администратора:
tunnel-server.exe
REM Выведет команду netsh — выполните её в другом окне
tunnel-client.exe <SERVER_IP>
