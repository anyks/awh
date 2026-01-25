cmake_minimum_required(VERSION 3.10)
project(tun_solaris)

set(CMAKE_CXX_STANDARD 17)

add_executable(tun_solaris main.cpp tunnel.cpp)
target_link_libraries(tun_solaris socket nsl)
