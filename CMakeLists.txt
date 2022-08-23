cmake_minimum_required(VERSION 3.5)

# TODO: вписать рабочее название проекта
project(awh LANGUAGES CXX)

set(PROJECT_VENDOR_LONG "ANYKS - WEB")

# Опции сборки
# Сборка тестового ПО
option(ENABLE_CONSOLE_APP "Build test app" TRUE)

set(PROJECT_COPYRIGHT "Copyright (c) ${CURRENT_YEAR} ${PROJECT_VENDOR_LONG}")

# Тип сборки
# cmake -DCMAKE_BUILD_TYPE=YES ..
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}\n")

# Сборка модуля AWH_IDN
if(CMAKE_BUILD_IDN)
    add_compile_definitions(AWH_IDN)
    message(STATUS "Build AWH IDN: ${CMAKE_BUILD_IDN}\n")
endif()

# Получаем архитектуру
EXECUTE_PROCESS( COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE )
message( STATUS "Architecture: ${ARCHITECTURE}" )

# Проверка версии OS
string(REGEX MATCH "Linux" PROJECT_OS_LINUX ${CMAKE_SYSTEM_NAME})
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    message(STATUS "Detected OS: Linux")
    set(DEBUGGER "-ggdb3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--no-as-needed -ldl")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    message(STATUS "Detected OS: MacOS X")
    set(DEBUGGER "-glldb")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    message(STATUS "Detected OS: FreeBSD")
    set(DEBUGGER "-ggdb3")
else()
    set(DEBUGGER "")
endif()

# Enable C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Устанавливаем опции компилятора
set(CMAKE_CXX_COMPILER_ID "clang++")
message(STATUS "Using C++ compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Using C compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}\n")

# Информация и времени компиляции
string(TIMESTAMP CURRENT_YEAR "%Y")
string(TIMESTAMP CURRENT_TIME "%d.%m.%Y %H:%M:%S")

# Опции компилятора
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(AWH_PARAMS "-Wno-cpp -Wno-deprecated -Wno-c++17-compat-mangling")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(AWH_PARAMS "-Wno-cpp -Wno-deprecated ")
else()
    set(AWH_PARAMS "-Wno-unknown-attributes -Wno-c++17-compat-mangling")
endif()

if (${ARCHITECTURE} MATCHES "aarch64" OR ${ARCHITECTURE} MATCHES "arm*")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -pipe -DNDEBUG")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fno-permissive -Wno-pedantic -Wno-narrowing -Wno-deprecated-declarations ${AWH_PARAMS}")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -pipe -mfma -mrdrnd -mf16c -march=core2 -DNDEBUG")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fno-permissive -Wno-pedantic -Wno-narrowing -Wno-deprecated-declarations ${AWH_PARAMS}")
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -DDEBUG_MODE ${DEBUGGER}")

if(CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "C++ Flags: ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG}")
    message(STATUS "C Flags: ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_DEBUG}\n")
else()
    message(STATUS "C++ Flags: ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
    message(STATUS "C Flags: ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_RELEASE}\n")
endif()

# Добавляем модули cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Выполняем поиск нужных библиотек
find_package(Zlib REQUIRED)
find_package(Brotli REQUIRED)
find_package(LibEv REQUIRED)
find_package(OpenSSL REQUIRED)

# Сборка модуля AWH_IDN, если операционной системой не является Windows
if(CMAKE_BUILD_IDN AND (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows"))
    find_package(Iconv REQUIRED)
    find_package(Idn2 REQUIRED)
endif()

# Провекра OpenSSL
if (OpenSSL_FOUND)
    message(STATUS "OpenSSL libs found: " ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})
    message(STATUS "OpenSSL include dir found: " ${OPENSSL_INCLUDE_DIR})
else()
    message(FATAL_ERROR "OpenSSL not found")
endif()

# Провекра ZLib
if (Zlib_FOUND)
    message(STATUS "ZLib lib found: " ${ZLIB_LIBRARY})
    message(STATUS "ZLib include dir found: " ${ZLIB_INCLUDE_DIR})
else()
    message(FATAL_ERROR "ZLib not found")
endif()

# Провекра Brotli
if (Brotli_FOUND)
    message(STATUS "Brotli lib found: " ${BROTLI_LIBRARIES})
    message(STATUS "Brotli include dir found: " ${BROTLI_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "Brotli not found")
endif()

# Провекра LibEv
if (LibEv_FOUND)
    message(STATUS "LibEv libs found: " ${LIBEV_LIBRARY})
    message(STATUS "LibEv include dir found: " ${LIBEV_INCLUDE_DIR})
else()
    message(FATAL_ERROR "LibEv not found")
endif()

# Сборка модуля AWH_IDN, если операционной системой не является Windows
if(CMAKE_BUILD_IDN AND (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows"))
    # Провекра ICONV
    if (Iconv_FOUND)
        message(STATUS "ICONV libs found: " ${ICONV_LIBRARY})
        message(STATUS "ICONV include dir found: " ${ICONV_INCLUDE_DIR})
    else()
        message(FATAL_ERROR "ICONV not found")
    endif()
    # Провекра IDN2
    if (Idn2_FOUND)
        message(STATUS "IDN2 libs found: " ${IDN2_LIBRARY})
        message(STATUS "IDN2 include dir found: " ${IDN2_INCLUDE_DIR})
    else()
        message(FATAL_ERROR "IDN2 not found")
    endif()
endif()

# Выполняем подключение хидеров
include_directories(${ZLIB_INCLUDE_DIR})
include_directories(${LIBEV_INCLUDE_DIR})
include_directories(${BROTLI_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/contrib/include)

# Сборка модуля AWH_IDN, если операционной системой не является Windows
if(CMAKE_BUILD_IDN AND (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows"))
    include_directories(${ICONV_INCLUDE_DIR})
    include_directories(${IDN2_INCLUDE_DIR})
endif()

# Выполняем подключение исходников
set(SOURCE_FILES
    "${CMAKE_SOURCE_DIR}/src/sys/fmk.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/log.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/signals.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/cluster.cpp"
    "${CMAKE_SOURCE_DIR}/src/ws/ws.cpp"
    "${CMAKE_SOURCE_DIR}/src/ws/frame.cpp"
    "${CMAKE_SOURCE_DIR}/src/ws/client.cpp"
    "${CMAKE_SOURCE_DIR}/src/ws/server.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/if.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/nwt.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/nwk.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/dns.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/uri.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/engine.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/socket.cpp"
    "${CMAKE_SOURCE_DIR}/src/hash/hash.cpp"
    "${CMAKE_SOURCE_DIR}/src/hash/base64.cpp"
    "${CMAKE_SOURCE_DIR}/src/http/web.cpp"
    "${CMAKE_SOURCE_DIR}/src/http/core.cpp"
    "${CMAKE_SOURCE_DIR}/src/http/proxy.cpp"
    "${CMAKE_SOURCE_DIR}/src/http/client.cpp"
    "${CMAKE_SOURCE_DIR}/src/http/server.cpp"
    "${CMAKE_SOURCE_DIR}/src/auth/core.cpp"
    "${CMAKE_SOURCE_DIR}/src/auth/client.cpp"
    "${CMAKE_SOURCE_DIR}/src/auth/server.cpp"
    "${CMAKE_SOURCE_DIR}/src/client/ws.cpp"
    "${CMAKE_SOURCE_DIR}/src/client/rest.cpp"
    "${CMAKE_SOURCE_DIR}/src/server/ws.cpp"
    "${CMAKE_SOURCE_DIR}/src/server/rest.cpp"
#    "${CMAKE_SOURCE_DIR}/src/server/proxy.cpp"
    "${CMAKE_SOURCE_DIR}/src/server/socks5.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/core.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/client.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/server.cpp"
    "${CMAKE_SOURCE_DIR}/src/socks5/core.cpp"
    "${CMAKE_SOURCE_DIR}/src/socks5/client.cpp"
    "${CMAKE_SOURCE_DIR}/src/socks5/server.cpp"
    "${CMAKE_SOURCE_DIR}/src/worker/ws.cpp"
    "${CMAKE_SOURCE_DIR}/src/worker/core.cpp"
    "${CMAKE_SOURCE_DIR}/src/worker/rest.cpp"
#    "${CMAKE_SOURCE_DIR}/src/worker/proxy.cpp"
    "${CMAKE_SOURCE_DIR}/src/worker/socks5.cpp"
    "${CMAKE_SOURCE_DIR}/src/worker/client.cpp"
    "${CMAKE_SOURCE_DIR}/src/worker/server.cpp"
)

# Задаем именя библиотеки и бинарника
set(WSC_APP_NAME wsc)
set(WSS_APP_NAME wss)
set(WEB_APP_NAME web)
set(REST_APP_NAME rest)
#set(TIMER_APP_NAME timer)
#set(PROXY_APP_NAME proxy)
set(SOCKS5_APP_NAME socks5)
set(CURRENT_LIB_NAME ${PROJECT_NAME})
set(CURRENT_APP_NAME ${PROJECT_NAME}_bin)

# Делаем либу
add_library(${CURRENT_LIB_NAME} STATIC ${SOURCE_FILES})

## Если установлен флаг ENABLE_CONSOLE_APP собираем консольное приложение
if (ENABLE_CONSOLE_APP)

    if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
        set(WINFLAGS -lws2_32 -lgdi32 -lcrypt32 -lIphlpapi -lbcrypt)
    else()
        set(WINFLAGS "")
    endif()

    # Устанавливаем иконку для windows под MinGW.
    set(RES_FILES "")
    if(MINGW)
        set(RES_FILES "awh.rc")
        set(CMAKE_RC_COMPILER_INIT windres)
        ENABLE_LANGUAGE(RC)
        SET(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
    endif(MINGW)

    add_executable(${WSC_APP_NAME} sample/wsc.cpp ${RES_FILES})
    add_executable(${WSS_APP_NAME} sample/wss.cpp ${RES_FILES})
    add_executable(${WEB_APP_NAME} sample/web.cpp ${RES_FILES})
    add_executable(${REST_APP_NAME} sample/rest.cpp ${RES_FILES})
#    add_executable(${TIMER_APP_NAME} sample/timer.cpp ${RES_FILES})
#    add_executable(${PROXY_APP_NAME} sample/proxy.cpp ${RES_FILES})
    add_executable(${SOCKS5_APP_NAME} sample/socks5.cpp ${RES_FILES})

    # Сборка примеров с поддержкой модуля AWH_IDN, если операционной системой не является Windows
    if(CMAKE_BUILD_IDN AND (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows"))
        target_link_libraries(${WSC_APP_NAME}
            ${CURRENT_LIB_NAME}
            ${ICONV_LIBRARY}
            ${IDN2_LIBRARY}
            ${ZLIB_LIBRARY}
            ${LIBEV_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${WINFLAGS}
        )

        target_link_libraries(${WSS_APP_NAME}
            ${CURRENT_LIB_NAME}
            ${ICONV_LIBRARY}
            ${IDN2_LIBRARY}
            ${ZLIB_LIBRARY}
            ${LIBEV_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${WINFLAGS}
        )

        target_link_libraries(${REST_APP_NAME}
            ${CURRENT_LIB_NAME}
            ${ICONV_LIBRARY}
            ${IDN2_LIBRARY}
            ${ZLIB_LIBRARY}
            ${LIBEV_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${WINFLAGS}
        )

        target_link_libraries(${WEB_APP_NAME}
            ${CURRENT_LIB_NAME}
            ${ICONV_LIBRARY}
            ${IDN2_LIBRARY}
            ${ZLIB_LIBRARY}
            ${LIBEV_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${WINFLAGS}
        )

        target_link_libraries(${SOCKS5_APP_NAME}
            ${CURRENT_LIB_NAME}
            ${ICONV_LIBRARY}
            ${IDN2_LIBRARY}
            ${ZLIB_LIBRARY}
            ${LIBEV_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${WINFLAGS}
        )
    # Сборка примеров в обычном виде
    else()
        target_link_libraries(${WSC_APP_NAME}
            ${CURRENT_LIB_NAME}
            ${ZLIB_LIBRARY}
            ${LIBEV_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${WINFLAGS}
        )

        target_link_libraries(${WSS_APP_NAME}
            ${CURRENT_LIB_NAME}
            ${ZLIB_LIBRARY}
            ${LIBEV_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        target_link_libraries(${REST_APP_NAME}
            ${CURRENT_LIB_NAME}
            ${ZLIB_LIBRARY}
            ${LIBEV_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${WINFLAGS}
        )

        target_link_libraries(${WEB_APP_NAME}
            ${CURRENT_LIB_NAME}
            ${ZLIB_LIBRARY}
            ${LIBEV_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${WINFLAGS}
        )

        target_link_libraries(${SOCKS5_APP_NAME}
            ${CURRENT_LIB_NAME}
            ${ZLIB_LIBRARY}
            ${LIBEV_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${WINFLAGS}
        )

    #    target_link_libraries(${TIMER_APP_NAME}
    #        ${CURRENT_LIB_NAME}
    #        ${ZLIB_LIBRARY}
    #        ${BROTLI_LIBRARIES}
    #        ${OPENSSL_LIBRARIES}
    #        ${LIBEVENT_LIBRARIES}
    #        ${WINFLAGS}
    #    )

    #    target_link_libraries(${PROXY_APP_NAME}
    #        ${CURRENT_LIB_NAME}
    #        ${ZLIB_LIBRARY}
    #        ${BROTLI_LIBRARIES}
    #        ${OPENSSL_LIBRARIES}
    #        ${LIBEVENT_LIBRARIES}
    #        ${WINFLAGS}
    #    )
    endif()
endif()

# Устанавливаем адрес установки, запрет установки в /usr/local
if("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr/local" OR "${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr/local/")
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}")
endif()

# Устанавливаем хидеры
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include" DESTINATION "${CMAKE_INSTALL_PREFIX}/include/${CMAKE_PROJECT_NAME}" FILES_MATCHING PATTERN "*.hpp")
# Устанавливаем библиотеку
install(TARGETS ${CURRENT_LIB_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
# Устанавливаем исполнительные файлы
install(TARGETS ${WSC_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${WSS_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${WEB_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${REST_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
#install(TARGETS ${TIMER_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
#install(TARGETS ${PROXY_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${SOCKS5_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")

# Устанавливаем хидеры сторонних библиотек
install(DIRECTORY "${ZLIB_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${BROTLI_INCLUDE_ENCODE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${LIBEV_INCLUDE_DIR}/libev" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${OPENSSL_INCLUDE_DIR}/openssl" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")

# Сборка модуля AWH_IDN, если операционной системой не является Windows
if(CMAKE_BUILD_IDN AND (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows"))
    install(DIRECTORY "${ICONV_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
    install(DIRECTORY "${IDN2_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
endif()

# Устанавливаем стороние библиотеки
install(FILES ${ZLIB_LIBRARY} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
install(FILES ${LIBEV_LIBRARY} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
install(FILES ${BROTLI_LIBRARIES} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
install(FILES ${OPENSSL_LIBRARIES} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")

# Сборка модуля AWH_IDN, если операционной системой не является Windows
if(CMAKE_BUILD_IDN AND (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows"))
    install(FILES ${ICONV_LIBRARY} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
    install(FILES ${IDN2_LIBRARY} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
endif()
