cmake_minimum_required(VERSION 3.15)

# TODO: вписать рабочее название проекта
project(awh LANGUAGES CXX)

# Устанавливаем название приложения
set(PROJECT_VENDOR_LONG "ANYKS - WEB")

# Опции сборки
set(PROJECT_COPYRIGHT "Copyright (c) ${CURRENT_YEAR} ${PROJECT_VENDOR_LONG}")

# Тип сборки
# cmake -DCMAKE_BUILD_TYPE=YES ..
if(NOT CMAKE_BUILD_TYPE)
    # Активируем режим отладки
    set(CMAKE_BUILD_TYPE Debug)
endif(NOT CMAKE_BUILD_TYPE)

# Выводим сообщение о типе сборки
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}\n")

# Сборка модуля AWH_IDN
if(CMAKE_BUILD_IDN)
    add_compile_definitions(AWH_IDN)
    message(STATUS "Build AWH IDN: YES\n")
# Сборка модуля AWH_IDN не требуется
else(CMAKE_BUILD_IDN)
    message(STATUS "Build AWH IDN: NO\n")
endif(CMAKE_BUILD_IDN)

# Сборка модуля AWH_EVENT2
if(CMAKE_BUILD_EVENT2)
    add_compile_definitions(AWH_EVENT2)
    message(STATUS "Build AWH LIBEV: NO\n")
    message(STATUS "Build AWH LIBEVENT2: YES\n")
# Сборка модуля AWH_EV
else(CMAKE_BUILD_EVENT2)
    set(CMAKE_BUILD_EV "YES")
    add_compile_definitions(AWH_EV)
    message(STATUS "Build AWH LIBEV: YES\n")
    message(STATUS "Build AWH LIBEVENT2: NO\n")
endif(CMAKE_BUILD_EVENT2)

# Выводим информацию каталога где выполняется сброка
message(STATUS "Build dir: ${CMAKE_CURRENT_BINARY_DIR}\n")

# Получаем архитектуру
EXECUTE_PROCESS(COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE )
message(STATUS "Architecture: ${ARCHITECTURE}")

# Проверка версии OS
string(REGEX MATCH "Linux" PROJECT_OS_LINUX ${CMAKE_SYSTEM_NAME})
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    message(STATUS "Detected OS: Linux")
    set(DEBUGGER "-ggdb3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--no-as-needed -ldl")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    if(NOT MACOS_VERSION)
        EXECUTE_PROCESS(COMMAND sw_vers -productVersion COMMAND tr -d '\n' OUTPUT_VARIABLE MACOS_VERSION)
    endif()
    message(STATUS "Detected OS: MacOS X v${MACOS_VERSION}")
    set(DEBUGGER "-glldb")    
    set(CMAKE_OSX_DEPLOYMENT_TARGET ${MACOS_VERSION})
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    message(STATUS "Detected OS: FreeBSD")
    set(DEBUGGER "-ggdb3")
else()
    set(DEBUGGER "")
endif()

# Enable C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Устанавливаем опции компилятора
set(CMAKE_CXX_COMPILER_ID "clang++")
message(STATUS "Using C compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Using C++ compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}\n")

# Информация и времени компиляции
string(TIMESTAMP CURRENT_YEAR "%Y")
string(TIMESTAMP CURRENT_TIME "%d.%m.%Y %H:%M:%S")

# Опции компилятора
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(AWH_PARAMS "-Wno-cpp -Wno-deprecated")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(AWH_PARAMS "-Wno-cpp -Wno-deprecated")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(AWH_PARAMS "-framework Carbon -Wno-cpp -Wno-deprecated -Wno-unused-command-line-argument")
else()
    set(AWH_PARAMS "-Wno-unknown-attributes")
endif()

if(${ARCHITECTURE} MATCHES "aarch64" OR ${ARCHITECTURE} MATCHES "arm*")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -pipe -DNDEBUG")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fno-permissive -Wno-pedantic -Wno-narrowing -Wno-deprecated-declarations ${AWH_PARAMS}")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -pipe -mrdrnd -march=core2 -DNDEBUG")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fno-permissive -Wno-pedantic -Wno-narrowing -Wno-deprecated-declarations ${AWH_PARAMS}")
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -DDEBUG_MODE ${DEBUGGER}")

if(CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "C Flags: ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_DEBUG}")
    message(STATUS "C++ Flags: ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG}\n")
else()
    message(STATUS "C Flags: ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_RELEASE}")
    message(STATUS "C++ Flags: ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}\n")
endif()

# Если нужно собрать и динамическую библиотеку
if(CMAKE_SHARED_BUILD_LIB)
    if(MSVC)
        set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    endif(MSVC)
endif(CMAKE_SHARED_BUILD_LIB)

# Добавляем модули cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Выполняем поиск нужных библиотек
find_package(Dependence REQUIRED)

# Провекра Dependence
if(Dependence_FOUND)
    message(STATUS "Dependence lib found: " ${DEPEND_LIBRARY})
    message(STATUS "LZ4 include dir found: " ${LZ4_INCLUDE_DIR})
    message(STATUS "BZ2 include dir found: " ${BZ2_INCLUDE_DIR})
    message(STATUS "ZSTD include dir found: " ${ZSTD_INCLUDE_DIR})
    message(STATUS "LZMA include dir found: " ${LZMA_INCLUDE_DIR})
    message(STATUS "ZLib include dir found: " ${ZLIB_INCLUDE_DIR})
    message(STATUS "PCRE2 include dir found: " ${PCRE_INCLUDE_DIR})
    message(STATUS "NgHttp2 include dir found: " ${NGHTTP2_INCLUDE_DIR})
    message(STATUS "OpenSSL include dir found: " ${OPENSSL_INCLUDE_DIR})
    message(STATUS "Brotli include dir found: " ${BROTLI_INCLUDE_ENCODE_DIR} " " ${BROTLI_INCLUDE_DECODE_DIR})

    # Если операцинная система не относится к MS Windows
    if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
        message(STATUS "JeMalloc include dir found: " ${JEMALLOC_INCLUDE_DIR})
    endif()

    # Сборка модуля AWH_EVENT2
    if(CMAKE_BUILD_EVENT2)
        # Сборка модуля AWH_IDN, если операционной системой не является Windows
        if(CMAKE_BUILD_IDN AND (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows"))
            message(STATUS "IDN2 include dir found: " ${IDN2_INCLUDE_DIR})
            message(STATUS "ICONV include dir found: " ${ICONV_INCLUDE_DIR})
        endif()
        message(STATUS "LibEvent2 include dir found: " ${LIBEVENT_INCLUDE_DIR})
    # Сборка модуля AWH_EV
    else(CMAKE_BUILD_EVENT2)
        # Сборка модуля AWH_IDN, если операционной системой не является Windows
        if(CMAKE_BUILD_IDN AND (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows"))
            message(STATUS "IDN2 include dir found: " ${IDN2_INCLUDE_DIR})
            message(STATUS "ICONV include dir found: " ${ICONV_INCLUDE_DIR})
        endif()
        message(STATUS "LibEv EV include dir found: " ${LIBEV_EV_INCLUDE_DIR})
        message(STATUS "LibEv EV++ include dir found: " ${LIBEV_EVPP_INCLUDE_DIR})
        message(STATUS "LibEv EVENT include dir found: " ${LIBEV_EVENT_INCLUDE_DIR})
    endif(CMAKE_BUILD_EVENT2)
else(Dependence_FOUND)
    message(FATAL_ERROR "Dependence not found")
endif(Dependence_FOUND)

# Выполняем подключение хидеров
include_directories(SYSTEM ${DEPEND_INCLUDE_DIRS})
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/include)
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/contrib/include)
include_directories(SYSTEM ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})

# Сборка модуля AWH_EVENT2
if(CMAKE_BUILD_EVENT2)
     # Выполняем подключение исходников для LibEvent2
     set(SOURCE_FILES
        "${CMAKE_SOURCE_DIR}/src/lib/event2/sys/events.cpp"
        "${CMAKE_SOURCE_DIR}/src/lib/event2/sys/signals.cpp"
        "${CMAKE_SOURCE_DIR}/src/lib/event2/sys/cluster.cpp"
        "${CMAKE_SOURCE_DIR}/src/lib/event2/scheme/core.cpp"
        "${CMAKE_SOURCE_DIR}/src/lib/event2/core/core.cpp"
        "${CMAKE_SOURCE_DIR}/src/lib/event2/core/timer.cpp"
    )
# Сборка модуля AWH_EV
else(CMAKE_BUILD_EVENT2)
    # Выполняем подключение исходников для LibEv
    set(SOURCE_FILES
        "${CMAKE_SOURCE_DIR}/src/lib/ev/sys/signals.cpp"
        "${CMAKE_SOURCE_DIR}/src/lib/ev/sys/cluster.cpp"
        "${CMAKE_SOURCE_DIR}/src/lib/ev/scheme/core.cpp"
        "${CMAKE_SOURCE_DIR}/src/lib/ev/core/core.cpp"
        "${CMAKE_SOURCE_DIR}/src/lib/ev/core/timer.cpp"
    )
endif(CMAKE_BUILD_EVENT2)

# Выполняем подключение исходников
set(SOURCE_FILES
    "${SOURCE_FILES}"
    "${CMAKE_SOURCE_DIR}/src/sys/os.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/fs.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/ver.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/fmk.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/log.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/reg.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/buffer.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/events.cpp"
    "${CMAKE_SOURCE_DIR}/src/ws/core.cpp"
    "${CMAKE_SOURCE_DIR}/src/ws/frame.cpp"
    "${CMAKE_SOURCE_DIR}/src/ws/client.cpp"
    "${CMAKE_SOURCE_DIR}/src/ws/server.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/if.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/net.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/nwt.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/uri.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/dns.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/ntp.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/ping.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/engine.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/socket.cpp"
    "${CMAKE_SOURCE_DIR}/src/hash/hash.cpp"
    "${CMAKE_SOURCE_DIR}/src/hash/base64.cpp"
    "${CMAKE_SOURCE_DIR}/src/http/web.cpp"
    "${CMAKE_SOURCE_DIR}/src/http/core.cpp"
    "${CMAKE_SOURCE_DIR}/src/http/http2.cpp"
    "${CMAKE_SOURCE_DIR}/src/http/client.cpp"
    "${CMAKE_SOURCE_DIR}/src/http/server.cpp"
    "${CMAKE_SOURCE_DIR}/src/auth/core.cpp"
    "${CMAKE_SOURCE_DIR}/src/auth/client.cpp"
    "${CMAKE_SOURCE_DIR}/src/auth/server.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/node.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/client.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/server.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/cluster.cpp"
    "${CMAKE_SOURCE_DIR}/src/client/ws.cpp"
    "${CMAKE_SOURCE_DIR}/src/client/awh.cpp"
    "${CMAKE_SOURCE_DIR}/src/client/sample.cpp"
    "${CMAKE_SOURCE_DIR}/src/client/web/web.cpp"
    "${CMAKE_SOURCE_DIR}/src/client/web/web2.cpp"
    "${CMAKE_SOURCE_DIR}/src/client/web/ws1.cpp"
    "${CMAKE_SOURCE_DIR}/src/client/web/ws2.cpp"
    "${CMAKE_SOURCE_DIR}/src/client/web/http1.cpp"
    "${CMAKE_SOURCE_DIR}/src/client/web/http2.cpp"
    "${CMAKE_SOURCE_DIR}/src/server/ws.cpp"
    "${CMAKE_SOURCE_DIR}/src/server/awh.cpp"
    "${CMAKE_SOURCE_DIR}/src/server/web/web.cpp"
    "${CMAKE_SOURCE_DIR}/src/server/web/web2.cpp"
    "${CMAKE_SOURCE_DIR}/src/server/web/ws1.cpp"
    "${CMAKE_SOURCE_DIR}/src/server/web/ws2.cpp"
    "${CMAKE_SOURCE_DIR}/src/server/web/http1.cpp"
    "${CMAKE_SOURCE_DIR}/src/server/web/http2.cpp"
    "${CMAKE_SOURCE_DIR}/src/server/proxy.cpp"
    "${CMAKE_SOURCE_DIR}/src/server/socks5.cpp"
    "${CMAKE_SOURCE_DIR}/src/server/sample.cpp"
    "${CMAKE_SOURCE_DIR}/src/socks5/core.cpp"
    "${CMAKE_SOURCE_DIR}/src/socks5/client.cpp"
    "${CMAKE_SOURCE_DIR}/src/socks5/server.cpp"
    "${CMAKE_SOURCE_DIR}/src/scheme/ws.cpp"
    "${CMAKE_SOURCE_DIR}/src/scheme/web.cpp"
    "${CMAKE_SOURCE_DIR}/src/scheme/web2.cpp"
    "${CMAKE_SOURCE_DIR}/src/scheme/socks5.cpp"
    "${CMAKE_SOURCE_DIR}/src/scheme/client.cpp"
    "${CMAKE_SOURCE_DIR}/src/scheme/server.cpp"
    "${CMAKE_SOURCE_DIR}/src/scheme/sample.cpp"
    "${CMAKE_SOURCE_DIR}/contrib/src/cityhash/city.cc"
)

# Задаем именя библиотеки и бинарника
set(IP_APP_NAME ip)
set(DNS_APP_NAME dns)
set(NTP_APP_NAME ntp)
set(PING_APP_NAME ping)
set(TIMER_APP_NAME timer)
set(PROXY_APP_NAME proxy)
set(SOCKS5_APP_NAME socks5)
set(CLIENT_APP_NAME client)
set(SERVER_APP_NAME server)


set(SERVER_TEST_APP_NAME test_srv)
set(CLIENT_TEST_APP_NAME test_cli)

set(WS_CLIENT_APP_NAME wsClient)
set(WS_SERVER_APP_NAME wsServer)
set(WEB_CLIENT_APP_NAME webClient)
set(WEB_SERVER_APP_NAME webServer)
set(AWH_LIB_NAME ${PROJECT_NAME})

# Если операционной системой не является Windows
if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(CLUSTER_APP_NAME cluster)
endif()

# Если нужно собрать и динамическую библиотеку
if(CMAKE_SHARED_BUILD_LIB)
    # Добавляем динамическую библиотеку
    add_library(${AWH_LIB_NAME} SHARED ${SOURCE_FILES})
    target_compile_definitions(${AWH_LIB_NAME} PRIVATE AWH_SHARED_LIBRARY_EXPORT)
# Если нужно собрать и статическую библиотеку
else(CMAKE_SHARED_BUILD_LIB)
    # Активируем сборку статической библиотеки
    add_compile_definitions(AWH_STATICLIB)
    # Добавляем статическу библиотеку
    add_library(${AWH_LIB_NAME} STATIC ${SOURCE_FILES})
endif(CMAKE_SHARED_BUILD_LIB)

 # Если операционной системой является Windows
 if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    # export MSYS="error_start:%WD%dumper.exe -d -c %1 %2"
    # Устанавливаем необходимые зависимости для MS Windows
    set(WINFLAGS -lws2_32 -lgdi32 -lcrypt32 -lIphlpapi -lbcrypt -flto=auto)

    # Устанавливаем иконку для windows под MinGW.
    set(RES_FILES "awh.rc")
    set(CMAKE_RC_COMPILER_INIT windres)
    ENABLE_LANGUAGE(RC)
    SET(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")

    # Выполняем создание демонстрационных приложений
    add_executable(${IP_APP_NAME} sample/ip.cpp ${RES_FILES})
    add_executable(${DNS_APP_NAME} sample/dns.cpp ${RES_FILES})
    add_executable(${NTP_APP_NAME} sample/ntp.cpp ${RES_FILES})
    add_executable(${PING_APP_NAME} sample/ping.cpp ${RES_FILES})
    add_executable(${TIMER_APP_NAME} sample/timer.cpp ${RES_FILES})
    add_executable(${PROXY_APP_NAME} sample/proxy.cpp ${RES_FILES})
    add_executable(${SOCKS5_APP_NAME} sample/socks5.cpp ${RES_FILES})
    add_executable(${CLIENT_APP_NAME} sample/client.cpp ${RES_FILES})
    add_executable(${SERVER_APP_NAME} sample/server.cpp ${RES_FILES})
    add_executable(${WS_SERVER_APP_NAME} sample/wsServer.cpp ${RES_FILES})
    add_executable(${WS_CLIENT_APP_NAME} sample/wsClient.cpp ${RES_FILES})
    add_executable(${WEB_SERVER_APP_NAME} sample/webServer.cpp ${RES_FILES})
    add_executable(${WEB_CLIENT_APP_NAME} sample/webClient.cpp ${RES_FILES})

    add_executable(${SERVER_TEST_APP_NAME} sample/test_srv.cpp ${RES_FILES})
    add_executable(${CLIENT_TEST_APP_NAME} sample/test_cli.cpp ${RES_FILES})

# Если операционной системой является Linux, FreeBSD, MacOS X
else()
    # Выполняем создание демонстрационных приложений
    add_executable(${IP_APP_NAME} sample/ip.cpp)
    add_executable(${DNS_APP_NAME} sample/dns.cpp)
    add_executable(${NTP_APP_NAME} sample/ntp.cpp)
    add_executable(${PING_APP_NAME} sample/ping.cpp)
    add_executable(${TIMER_APP_NAME} sample/timer.cpp)
    add_executable(${PROXY_APP_NAME} sample/proxy.cpp)
    add_executable(${SOCKS5_APP_NAME} sample/socks5.cpp)
    add_executable(${CLIENT_APP_NAME} sample/client.cpp)
    add_executable(${SERVER_APP_NAME} sample/server.cpp)
    add_executable(${CLUSTER_APP_NAME} sample/cluster.cpp)
    add_executable(${WS_SERVER_APP_NAME} sample/wsServer.cpp)
    add_executable(${WS_CLIENT_APP_NAME} sample/wsClient.cpp)
    add_executable(${WEB_SERVER_APP_NAME} sample/webServer.cpp)
    add_executable(${WEB_CLIENT_APP_NAME} sample/webClient.cpp)

    add_executable(${SERVER_TEST_APP_NAME} sample/test_srv.cpp)
    add_executable(${CLIENT_TEST_APP_NAME} sample/test_cli.cpp)

endif()

# Если нужно собрать и динамическую библиотеку
if(CMAKE_SHARED_BUILD_LIB)
    # Если операционной системой является Windows
    if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")

        # Выполняем сборку динамической библиотеки
        target_link_libraries(
            ${AWH_LIB_NAME}
            PRIVATE
            ${DEPEND_LIBRARY}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения работы с IP-адресами
        target_link_libraries(
            ${IP_APP_NAME}
            ${AWH_LIB_NAME}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения WebSocket-клиент
        target_link_libraries(
            ${WS_CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения WebSocket-сервер
        target_link_libraries(
            ${WS_SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения DNS-резолвер
        target_link_libraries(
            ${DNS_APP_NAME}
            ${AWH_LIB_NAME}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения NTP-клиента
        target_link_libraries(
            ${NTP_APP_NAME}
            ${AWH_LIB_NAME}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Proxy-сервера
        target_link_libraries(
            ${PROXY_APP_NAME}
            ${AWH_LIB_NAME}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Socks5-сервера
        target_link_libraries(
            ${SOCKS5_APP_NAME}
            ${AWH_LIB_NAME}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Ping-клиента
        target_link_libraries(
            ${PING_APP_NAME}
            ${AWH_LIB_NAME}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения таймера
        target_link_libraries(
            ${TIMER_APP_NAME}
            ${AWH_LIB_NAME}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения клиента
        target_link_libraries(
            ${CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения сервера
        target_link_libraries(
            ${SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения HTTP-клиента
        target_link_libraries(
            ${WEB_CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения HTTP-сервера
        target_link_libraries(
            ${WEB_SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${WINFLAGS}
            ${AWH_IDN}
        )

    # Сборка примеров с поддержкой модуля AWH_IDN для Linux, FreeBSD, MacOS X
    elseif(CMAKE_BUILD_IDN)

        # Выполняем сборку динамической библиотеки
        target_link_libraries(
            ${AWH_LIB_NAME}
            PRIVATE
            ${DEPEND_LIBRARY}
            ${AWH_IDN}
        )
        
        # Выполняем сборку приложения работы с IP-адресами
        target_link_libraries(
            ${IP_APP_NAME}
            ${AWH_LIB_NAME}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения WebSocket-клиент
        target_link_libraries(
            ${WS_CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения WebSocket-сервер
        target_link_libraries(
            ${WS_SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения DNS-резолвер
        target_link_libraries(
            ${DNS_APP_NAME}
            ${AWH_LIB_NAME}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения NTP-клиента
        target_link_libraries(
            ${NTP_APP_NAME}
            ${AWH_LIB_NAME}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Proxy-сервера
        target_link_libraries(
            ${PROXY_APP_NAME}
            ${AWH_LIB_NAME}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Socks5-сервера
        target_link_libraries(
            ${SOCKS5_APP_NAME}
            ${AWH_LIB_NAME}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Ping-клиента
        target_link_libraries(
            ${PING_APP_NAME}
            ${AWH_LIB_NAME}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения таймера
        target_link_libraries(
            ${TIMER_APP_NAME}
            ${AWH_LIB_NAME}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения клиента
        target_link_libraries(
            ${CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения сервера
        target_link_libraries(
            ${SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения кластера
        target_link_libraries(
            ${CLUSTER_APP_NAME}
            ${AWH_LIB_NAME}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения HTTP-клиента
        target_link_libraries(
            ${WEB_CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения HTTP-сервера
        target_link_libraries(
            ${WEB_SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${AWH_IDN}
        )

    # Если операционной системой является Linux, FreeBSD, MacOS X
    else()

        # Выполняем сборку динамической библиотеки
        target_link_libraries(
            ${AWH_LIB_NAME}
            PRIVATE
            ${DEPEND_LIBRARY}
        )
        
        # Выполняем сборку приложения работы с IP-адресами
        target_link_libraries(
            ${IP_APP_NAME}
            ${AWH_LIB_NAME}
        )

        # Выполняем сборку приложения WebSocket-клиент
        target_link_libraries(
            ${WS_CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
        )

        # Выполняем сборку приложения WebSocket-сервер
        target_link_libraries(
            ${WS_SERVER_APP_NAME}
            ${AWH_LIB_NAME}
        )

        # Выполняем сборку приложения DNS-резолвер
        target_link_libraries(
            ${DNS_APP_NAME}
            ${AWH_LIB_NAME}
        )

        # Выполняем сборку приложения NTP-клиента
        target_link_libraries(
            ${NTP_APP_NAME}
            ${AWH_LIB_NAME}
        )

        # Выполняем сборку приложения Proxy-сервера
        target_link_libraries(
            ${PROXY_APP_NAME}
            ${AWH_LIB_NAME}
        )

        # Выполняем сборку приложения Socks5-сервера
        target_link_libraries(
            ${SOCKS5_APP_NAME}
            ${AWH_LIB_NAME}
        )

        # Выполняем сборку приложения Ping-клиента
        target_link_libraries(
            ${PING_APP_NAME}
            ${AWH_LIB_NAME}
        )

        # Выполняем сборку приложения таймера
        target_link_libraries(
            ${TIMER_APP_NAME}
            ${AWH_LIB_NAME}
        )

        # Выполняем сборку приложения клиента
        target_link_libraries(
            ${CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
        )

        # Выполняем сборку приложения сервера
        target_link_libraries(
            ${SERVER_APP_NAME}
            ${AWH_LIB_NAME}
        )

        # Выполняем сборку приложения кластера
        target_link_libraries(
            ${CLUSTER_APP_NAME}
            ${AWH_LIB_NAME}
        )

        # Выполняем сборку приложения HTTP-клиента
        target_link_libraries(
            ${WEB_CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
        )

        # Выполняем сборку приложения HTTP-сервера
        target_link_libraries(
            ${WEB_SERVER_APP_NAME}
            ${AWH_LIB_NAME}
        )

    endif()
# Если нужно собрать и статическую библиотеку
else(CMAKE_SHARED_BUILD_LIB)
    # Если операционной системой является Windows
    if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")

        # Выполняем сборку приложения работы с IP-адресами
        target_link_libraries(
            ${IP_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения WebSocket-клиент
        target_link_libraries(
            ${WS_CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения WebSocket-сервер
        target_link_libraries(
            ${WS_SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения DNS-резолвер
        target_link_libraries(
            ${DNS_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения NTP-клиента
        target_link_libraries(
            ${NTP_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Proxy-сервера
        target_link_libraries(
            ${PROXY_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Socks5-сервера
        target_link_libraries(
            ${SOCKS5_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Ping-клиента
        target_link_libraries(
            ${PING_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения таймера
        target_link_libraries(
            ${TIMER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения клиента
        target_link_libraries(
            ${CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения сервера
        target_link_libraries(
            ${SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения HTTP-клиента
        target_link_libraries(
            ${WEB_CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения HTTP-сервера
        target_link_libraries(
            ${WEB_SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${WINFLAGS}
            ${AWH_IDN}
        )



        # Выполняем сборку тестового отладочного сервера
        target_link_libraries(
            ${SERVER_TEST_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку тестового отладочного клиента
        target_link_libraries(
            ${CLIENT_TEST_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${WINFLAGS}
            ${AWH_IDN}
        )

    # Сборка примеров с поддержкой модуля AWH_IDN для Linux, FreeBSD, MacOS X
    elseif(CMAKE_BUILD_IDN)

        # Выполняем сборку приложения работы с IP-адресами
        target_link_libraries(
            ${IP_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения WebSocket-клиент
        target_link_libraries(
            ${WS_CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения WebSocket-сервер
        target_link_libraries(
            ${WS_SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения DNS-резолвер
        target_link_libraries(
            ${DNS_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения NTP-клиента
        target_link_libraries(
            ${NTP_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Proxy-сервера
        target_link_libraries(
            ${PROXY_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Socks5-сервера
        target_link_libraries(
            ${SOCKS5_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Ping-клиента
        target_link_libraries(
            ${PING_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения таймера
        target_link_libraries(
            ${TIMER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения клиента
        target_link_libraries(
            ${CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения сервера
        target_link_libraries(
            ${SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения кластера
        target_link_libraries(
            ${CLUSTER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения HTTP-клиента
        target_link_libraries(
            ${WEB_CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения HTTP-сервера
        target_link_libraries(
            ${WEB_SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${AWH_IDN}
        )


        # Выполняем сборку тестового отладочного сервера
        target_link_libraries(
            ${SERVER_TEST_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${AWH_IDN}
        )

        # Выполняем сборку тестового отладочного клиента
        target_link_libraries(
            ${CLIENT_TEST_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${AWH_IDN}
        )

    # Если операционной системой является Linux, FreeBSD, MacOS X
    else()

        # Выполняем сборку приложения работы с IP-адресами
        target_link_libraries(
            ${IP_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
        )

        # Выполняем сборку приложения WebSocket-клиент
        target_link_libraries(
            ${WS_CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
        )

        # Выполняем сборку приложения WebSocket-сервер
        target_link_libraries(
            ${WS_SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
        )

        # Выполняем сборку приложения DNS-резолвер
        target_link_libraries(
            ${DNS_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
        )

        # Выполняем сборку приложения NTP-клиента
        target_link_libraries(
            ${NTP_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
        )

        # Выполняем сборку приложения Proxy-сервера
        target_link_libraries(
            ${PROXY_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
        )

        # Выполняем сборку приложения Socks5-сервера
        target_link_libraries(
            ${SOCKS5_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
        )

        # Выполняем сборку приложения Ping-клиента
        target_link_libraries(
            ${PING_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
        )

        # Выполняем сборку приложения таймера
        target_link_libraries(
            ${TIMER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
        )

        # Выполняем сборку приложения клиента
        target_link_libraries(
            ${CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
        )

        # Выполняем сборку приложения сервера
        target_link_libraries(
            ${SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
        )

        # Выполняем сборку приложения кластера
        target_link_libraries(
            ${CLUSTER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
        )

        # Выполняем сборку приложения HTTP-клиента
        target_link_libraries(
            ${WEB_CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
        )

        # Выполняем сборку приложения HTTP-сервера
        target_link_libraries(
            ${WEB_SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
        )

        # Выполняем сборку тестового отладочного сервера
        target_link_libraries(
            ${SERVER_TEST_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
        )

        # Выполняем сборку тестового отладочного клиента
        target_link_libraries(
            ${CLIENT_TEST_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
        )

    endif()
endif(CMAKE_SHARED_BUILD_LIB)

# Активация формирования дампа ядра в реже отладки под MacOS X
if(CMAKE_BUILD_TYPE MATCHES Debug)
    if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
        set(entitlements_file "${CMAKE_SOURCE_DIR}/segv.entitlements")
        
        add_custom_command(TARGET "${IP_APP_NAME}" POST_BUILD
            COMMAND bash "${CMAKE_SOURCE_DIR}/core_dump.sh" "$<TARGET_FILE:${IP_APP_NAME}>" "${entitlements_file}"
            COMMENT "Sign target \"${IP_APP_NAME}\" to enable core dump on MacOS X"
            VERBATIM
        )

        add_custom_command(TARGET "${WS_CLIENT_APP_NAME}" POST_BUILD
            COMMAND bash "${CMAKE_SOURCE_DIR}/core_dump.sh" "$<TARGET_FILE:${WS_CLIENT_APP_NAME}>" "${entitlements_file}"
            COMMENT "Sign target \"${WS_CLIENT_APP_NAME}\" to enable core dump on MacOS X"
            VERBATIM
        )

        add_custom_command(TARGET "${WS_SERVER_APP_NAME}" POST_BUILD
            COMMAND bash "${CMAKE_SOURCE_DIR}/core_dump.sh" "$<TARGET_FILE:${WS_SERVER_APP_NAME}>" "${entitlements_file}"
            COMMENT "Sign target \"${WS_SERVER_APP_NAME}\" to enable core dump on MacOS X"
            VERBATIM
        )

        add_custom_command(TARGET "${DNS_APP_NAME}" POST_BUILD
            COMMAND bash "${CMAKE_SOURCE_DIR}/core_dump.sh" "$<TARGET_FILE:${DNS_APP_NAME}>" "${entitlements_file}"
            COMMENT "Sign target \"${DNS_APP_NAME}\" to enable core dump on MacOS X"
            VERBATIM
        )

        add_custom_command(TARGET "${NTP_APP_NAME}" POST_BUILD
            COMMAND bash "${CMAKE_SOURCE_DIR}/core_dump.sh" "$<TARGET_FILE:${NTP_APP_NAME}>" "${entitlements_file}"
            COMMENT "Sign target \"${NTP_APP_NAME}\" to enable core dump on MacOS X"
            VERBATIM
        )

        add_custom_command(TARGET "${PING_APP_NAME}" POST_BUILD
            COMMAND bash "${CMAKE_SOURCE_DIR}/core_dump.sh" "$<TARGET_FILE:${PING_APP_NAME}>" "${entitlements_file}"
            COMMENT "Sign target \"${PING_APP_NAME}\" to enable core dump on MacOS X"
            VERBATIM
        )

        add_custom_command(TARGET "${TIMER_APP_NAME}" POST_BUILD
            COMMAND bash "${CMAKE_SOURCE_DIR}/core_dump.sh" "$<TARGET_FILE:${TIMER_APP_NAME}>" "${entitlements_file}"
            COMMENT "Sign target \"${TIMER_APP_NAME}\" to enable core dump on MacOS X"
            VERBATIM
        )

        add_custom_command(TARGET "${PROXY_APP_NAME}" POST_BUILD
            COMMAND bash "${CMAKE_SOURCE_DIR}/core_dump.sh" "$<TARGET_FILE:${PROXY_APP_NAME}>" "${entitlements_file}"
            COMMENT "Sign target \"${PROXY_APP_NAME}\" to enable core dump on MacOS X"
            VERBATIM
        )
        
        add_custom_command(TARGET "${SOCKS5_APP_NAME}" POST_BUILD
            COMMAND bash "${CMAKE_SOURCE_DIR}/core_dump.sh" "$<TARGET_FILE:${SOCKS5_APP_NAME}>" "${entitlements_file}"
            COMMENT "Sign target \"${SOCKS5_APP_NAME}\" to enable core dump on MacOS X"
            VERBATIM
        )
        
        add_custom_command(TARGET "${CLIENT_APP_NAME}" POST_BUILD
            COMMAND bash "${CMAKE_SOURCE_DIR}/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_NAME}>" "${entitlements_file}"
            COMMENT "Sign target \"${CLIENT_APP_NAME}\" to enable core dump on MacOS X"
            VERBATIM
        )
            
        add_custom_command(TARGET "${SERVER_APP_NAME}" POST_BUILD
            COMMAND bash "${CMAKE_SOURCE_DIR}/core_dump.sh" "$<TARGET_FILE:${SERVER_APP_NAME}>" "${entitlements_file}"
            COMMENT "Sign target \"${SERVER_APP_NAME}\" to enable core dump on MacOS X"
            VERBATIM
        )

        add_custom_command(TARGET "${WEB_CLIENT_APP_NAME}" POST_BUILD
            COMMAND bash "${CMAKE_SOURCE_DIR}/core_dump.sh" "$<TARGET_FILE:${WEB_CLIENT_APP_NAME}>" "${entitlements_file}"
            COMMENT "Sign target \"${WEB_CLIENT_APP_NAME}\" to enable core dump on MacOS X"
            VERBATIM
        )
        
        add_custom_command(TARGET "${WEB_SERVER_APP_NAME}" POST_BUILD
            COMMAND bash "${CMAKE_SOURCE_DIR}/core_dump.sh" "$<TARGET_FILE:${WEB_SERVER_APP_NAME}>" "${entitlements_file}"
            COMMENT "Sign target \"${WEB_SERVER_APP_NAME}\" to enable core dump on MacOS X"
            VERBATIM
        )
        
        add_custom_command(TARGET "${CLUSTER_APP_NAME}" POST_BUILD
            COMMAND bash "${CMAKE_SOURCE_DIR}/core_dump.sh" "$<TARGET_FILE:${CLUSTER_APP_NAME}>" "${entitlements_file}"
            COMMENT "Sign target \"${CLUSTER_APP_NAME}\" to enable core dump on MacOS X"
            VERBATIM
        )

    endif()
    # Сборка модуля CMAKE_ENABLED_PVS
    if(CMAKE_ENABLED_PVS)
        include(FetchContent)
        FetchContent_Declare(
            PVS_CMakeModule
            GIT_REPOSITORY "https://github.com/viva64/pvs-studio-cmake-module.git"
            GIT_TAG        "master" 
        )
        FetchContent_MakeAvailable(PVS_CMakeModule)
        include("${pvs_cmakemodule_SOURCE_DIR}/PVS-Studio.cmake")
        # Выполняем анализ собранных приложений
        pvs_studio_add_target(TARGET ${IP_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${IP_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
        pvs_studio_add_target(TARGET ${WS_CLIENT_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${WS_CLIENT_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
        pvs_studio_add_target(TARGET ${WS_SERVER_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${WS_SERVER_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
        pvs_studio_add_target(TARGET ${DNS_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${DNS_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
        pvs_studio_add_target(TARGET ${NTP_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${NTP_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
        pvs_studio_add_target(TARGET ${PING_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${PING_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
        pvs_studio_add_target(TARGET ${TIMER_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${TIMER_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
        pvs_studio_add_target(TARGET ${PROXY_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${PROXY_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
        pvs_studio_add_target(TARGET ${SOCKS5_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SOCKS5_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
        pvs_studio_add_target(TARGET ${CLIENT_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
        pvs_studio_add_target(TARGET ${SERVER_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
        pvs_studio_add_target(TARGET ${WEB_CLIENT_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${WEB_CLIENT_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
        pvs_studio_add_target(TARGET ${WEB_SERVER_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${WEB_SERVER_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
        pvs_studio_add_target(TARGET ${CLUSTER_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLUSTER_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
    endif(CMAKE_ENABLED_PVS)
endif()

# Устанавливаем адрес установки, запрет установки в /usr/local
if("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr/local" OR "${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr/local/")
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}")
endif()

# Устанавливаем сертификаты
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/certs/" DESTINATION "${CMAKE_INSTALL_PREFIX}/certs")

# Устанавливаем хидеры
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/" DESTINATION "${CMAKE_INSTALL_PREFIX}/include/${CMAKE_PROJECT_NAME}" FILES_MATCHING PATTERN "*.hpp")
# Устанавливаем хидеры сторонних библиотек
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/contrib/include/" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h" PATTERN "*.hh" PATTERN "*.hpp")

# Устанавливаем библиотеку
if(NOT CMAKE_SHARED_BUILD_LIB)
    # Если режим отладки не активирован
    if(NOT CMAKE_BUILD_TYPE MATCHES Debug)
        # Выполняем запуск скрипта объединения библиотек
        add_custom_command(TARGET "${AWH_LIB_NAME}" POST_BUILD
            COMMAND bash "${CMAKE_SOURCE_DIR}/build_static_lib.sh" "$<TARGET_FILE:${AWH_LIB_NAME}>"
        VERBATIM)
    # Если режим отладки активирован
    else()
        # Выполняем установку библиотеки зависимостей
        install(FILES ${DEPEND_LIBRARY} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
    endif()
endif()

# Устанавливаем статическую библиотеку
install(TARGETS ${AWH_LIB_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")

# Устанавливаем исполнительные файлы
install(TARGETS ${IP_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${DNS_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${NTP_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${PING_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${TIMER_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${PROXY_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${SOCKS5_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${CLIENT_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${SERVER_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${WS_CLIENT_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${WS_SERVER_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${WEB_CLIENT_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${WEB_SERVER_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")

# Если операционной системой не является Windows
if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    install(TARGETS ${CLUSTER_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
endif()

# Выполняем запуск скрипта генерации CMAKE-файла
add_custom_command(TARGET "${AWH_LIB_NAME}" POST_BUILD
    COMMAND bash "${CMAKE_SOURCE_DIR}/build_cmake.sh" "${CMAKE_INSTALL_PREFIX}"
VERBATIM)

# Устанавливаем сгенерированный CMAKE-файл
install(FILES "${CMAKE_SOURCE_DIR}/third_party/cmake/FindAWH.cmake" DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake")
