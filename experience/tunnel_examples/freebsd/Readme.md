pkg install cmake
mkdir build && cd build
cmake .. && make
sudo ./tunnel-server
sudo ./tunnel-client <SERVER_IP>
