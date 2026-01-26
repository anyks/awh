cmake_minimum_required(VERSION 3.15)
project(nat_punch LANGUAGES CXX)

option(BUILD_CLIENT "Build client" OFF)
option(BUILD_SERVER "Build server" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(BUILD_CLIENT)
    add_subdirectory(client)
endif()

if(BUILD_SERVER)
    add_subdirectory(server)
endif()
