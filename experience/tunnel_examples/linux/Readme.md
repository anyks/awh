mkdir build && cd build
cmake .. && make
sudo ./tunnel-server
# В другом терминале:
sudo ./tunnel-client <SERVER_IP>
ping 10.8.0.1
