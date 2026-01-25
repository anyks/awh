# Установите компилятор и cmake
pkg install developer/gcc developer/build/cmake

mkdir build && cd build
cmake .. && make

# Запустите сервер:
sudo ./tunnel-server
# Выведет: Run manually: ifconfig tun0 10.8.0.1/24 up
# Выполните эту команду в другом терминале, затем продолжится работа

# Клиент:
sudo ./tunnel-client <SERVER_IP>
# Аналогично — выполните ifconfig вручную
