cmake_minimum_required(VERSION 3.10)
project(tun_windows)

set(CMAKE_CXX_STANDARD 17)

if(MINGW)
    add_executable(tun_windows main.cpp tunnel.cpp)
    target_link_libraries(tun_windows ws2_32 iphlpapi)
else()
    message(WARNING "This project is intended for MinGW on Windows")
endif()
