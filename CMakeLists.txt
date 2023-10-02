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
EXECUTE_PROCESS( COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE )
message(STATUS "Architecture: ${ARCHITECTURE}")

# Проверка версии OS
string(REGEX MATCH "Linux" PROJECT_OS_LINUX ${CMAKE_SYSTEM_NAME})
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    message(STATUS "Detected OS: Linux")
    set(DEBUGGER "-ggdb3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--no-as-needed -ldl")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    EXECUTE_PROCESS(COMMAND sw_vers -productVersion COMMAND tr -d '\n' OUTPUT_VARIABLE MACOS_VERSION)
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

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(AWH_PARAMS "-Wno-cpp -Wno-deprecated")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(AWH_PARAMS "-Wno-cpp -Wno-deprecated ")
else()
    set(AWH_PARAMS "-Wno-unknown-attributes")
endif()

if (${ARCHITECTURE} MATCHES "aarch64" OR ${ARCHITECTURE} MATCHES "arm*")
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

# Добавляем модули cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Выполняем поиск нужных библиотек
find_package(XML REQUIRED)
find_package(Zlib REQUIRED)
find_package(Brotli REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(NgTCP2 REQUIRED)
find_package(NgHttp2 REQUIRED)
find_package(NgHttp3 REQUIRED)
find_package(JeMalloc REQUIRED)

# Если операционной системой не является Windows
if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    # Выполняем поиск нужных библиотек
    find_package(PCRE REQUIRED)
endif()

# Сборка модуля AWH_IDN, если операционной системой не является Windows
if(CMAKE_BUILD_IDN AND (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows"))
    # Выполняем поиск нужных библиотек
    find_package(Iconv REQUIRED)
    find_package(Idn2 REQUIRED)
endif()

# Сборка модуля AWH_EVENT2
if(CMAKE_BUILD_EVENT2)
    find_package(LibEvent REQUIRED)
# Сборка модуля AWH_EV
else(CMAKE_BUILD_EVENT2)
    find_package(LibEv REQUIRED)
endif(CMAKE_BUILD_EVENT2)

# Провекра XML
if (XML_FOUND)
    message(STATUS "XML lib found: " ${XML_LIBRARY})
    message(STATUS "XML include dir found: " ${XML_INCLUDE_DIR})
else(XML_FOUND)
    message(FATAL_ERROR "XML not found")
endif(XML_FOUND)

# Провекра ZLib
if (Zlib_FOUND)
    message(STATUS "ZLib lib found: " ${ZLIB_LIBRARY})
    message(STATUS "ZLib include dir found: " ${ZLIB_INCLUDE_DIR})
else(Zlib_FOUND)
    message(FATAL_ERROR "ZLib not found")
endif(Zlib_FOUND)

# Провекра Brotli
if (Brotli_FOUND)
    message(STATUS "Brotli lib found: " ${BROTLI_ENCODE_LIBRARY} " " ${BROTLI_DECODE_LIBRARY} " " ${BROTLI_COMMON_LIBRARY})
    message(STATUS "Brotli include dir found: " ${BROTLI_INCLUDE_ENCODE_DIR} " " ${BROTLI_INCLUDE_DECODE_DIR})
else(Brotli_FOUND)
    message(FATAL_ERROR "Brotli not found")
endif(Brotli_FOUND)

# Провекра OpenSSL
if (OpenSSL_FOUND)
    message(STATUS "OpenSSL libs found: " ${OPENSSL_SSL_LIBRARY} " " ${OPENSSL_CRYPTO_LIBRARY})
    message(STATUS "OpenSSL include dir found: " ${OPENSSL_INCLUDE_DIR})
else(OpenSSL_FOUND)
    message(FATAL_ERROR "OpenSSL not found")
endif(OpenSSL_FOUND)

# Провекра NgTCP2
if (NgTCP2_FOUND)
    message(STATUS "NgTCP2 lib found: " ${NGTCP2_LIBRARY})
    message(STATUS "NgTCP2 include dir found: " ${NGTCP2_INCLUDE_DIR})
else(NgTCP2_FOUND)
    message(FATAL_ERROR "NgTCP2 not found")
endif(NgTCP2_FOUND)

# Провекра NgHttp2
if (NgHttp2_FOUND)
    message(STATUS "NgHttp2 lib found: " ${NGHTTP2_LIBRARY})
    message(STATUS "NgHttp2 include dir found: " ${NGHTTP2_INCLUDE_DIR})
else(NgHttp2_FOUND)
    message(FATAL_ERROR "NgHttp2 not found")
endif(NgHttp2_FOUND)

# Провекра NgHttp3
if (NgHttp3_FOUND)
    message(STATUS "NgHttp3 lib found: " ${NGHTTP3_LIBRARY})
    message(STATUS "NgHttp3 include dir found: " ${NGHTTP3_INCLUDE_DIR})
else(NgHttp3_FOUND)
    message(FATAL_ERROR "NgHttp3 not found")
endif(NgHttp3_FOUND)

# Провекра JeMalloc
if (JeMalloc_FOUND)
    message(STATUS "JeMalloc lib found: " ${JEMALLOC_LIBRARY})
    message(STATUS "JeMalloc include dir found: " ${JEMALLOC_INCLUDE_DIR})
else(JeMalloc_FOUND)
    message(FATAL_ERROR "JeMalloc not found")
endif(JeMalloc_FOUND)

# Если операционной системой не является Windows
if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    # Провекра PCRE
    if (PCRE_FOUND)
        message(STATUS "PCRE lib found: " ${PCRE_LIBRARY} " " ${PCRE_CPP_LIBRARY} " " ${PCRE_POSIX_LIBRARY})
        message(STATUS "PCRE include dir found: " ${PCRE_INCLUDE_DIR})
    else(PCRE_FOUND)
        message(FATAL_ERROR "PCRE not found")
    endif(PCRE_FOUND)
endif()

# Сборка модуля AWH_IDN, если операционной системой не является Windows
if(CMAKE_BUILD_IDN AND (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows"))
    # Провекра ICONV
    if (Iconv_FOUND)
        message(STATUS "ICONV libs found: " ${ICONV_LIBRARY} " " ${ICONV_CHARSET_LIBRARY})
        message(STATUS "ICONV include dir found: " ${ICONV_INCLUDE_DIR})
    else(Iconv_FOUND)
        message(FATAL_ERROR "ICONV not found")
    endif(Iconv_FOUND)
    # Провекра IDN2
    if (Idn2_FOUND)
        message(STATUS "IDN2 libs found: " ${IDN2_LIBRARY})
        message(STATUS "IDN2 include dir found: " ${IDN2_INCLUDE_DIR})
    else(Idn2_FOUND)
        message(FATAL_ERROR "IDN2 not found")
    endif(Idn2_FOUND)
endif()

# Сборка модуля AWH_EVENT2
if(CMAKE_BUILD_EVENT2)
    # Провекра LibEvent2
    if (LibEvent_FOUND)
        set(EVENT_LIBRARY ${LIBEVENT_LIBRARIES})
        message(STATUS "LibEvent2 libs found: " ${LIBEVENT_LIBRARY} " " ${LIBEVENT_CORE} " " ${LIBEVENT_EXTRA} " " ${LIBEVENT_SSL})
        message(STATUS "LibEvent2 include dir found: " ${LIBEVENT_INCLUDE_DIR})
    else(LibEvent_FOUND)
        message(FATAL_ERROR "LibEvent2 not found")
    endif(LibEvent_FOUND)
# Сборка модуля AWH_EV
else()
    # Провекра LibEv
    if (LibEv_FOUND)
        set(EVENT_LIBRARY ${LIBEV_LIBRARY})
        message(STATUS "LibEv libs found: " ${LIBEV_LIBRARY})
        message(STATUS "LibEv include dir found: " ${LIBEV_INCLUDE_DIR})
    else(LibEv_FOUND)
        message(FATAL_ERROR "LibEv not found")
    endif(LibEv_FOUND)
endif()

# Выполняем подключение хидеров
include_directories(SYSTEM ${XML_INCLUDE_DIR})
include_directories(SYSTEM ${ZLIB_INCLUDE_DIR})
include_directories(SYSTEM ${NGTCP2_INCLUDE_DIR})
include_directories(SYSTEM ${NGHTTP2_INCLUDE_DIR})
include_directories(SYSTEM ${NGHTTP3_INCLUDE_DIR})
include_directories(SYSTEM ${BROTLI_INCLUDE_DIRS})
include_directories(SYSTEM ${OPENSSL_INCLUDE_DIR})
include_directories(SYSTEM ${JEMALLOC_INCLUDE_DIR})
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/include)
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/contrib/include)
include_directories(SYSTEM ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})

# Если операционной системой не является Windows
if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    # Выполняем подключение хидеров
    include_directories(${PCRE_INCLUDE_DIR})
endif()

# Сборка модуля AWH_IDN, если операционной системой не является Windows
if(CMAKE_BUILD_IDN AND (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows"))
    include_directories(${ICONV_INCLUDE_DIR})
    include_directories(${IDN2_INCLUDE_DIR})
endif()

# Сборка модуля AWH_EVENT2
if(CMAKE_BUILD_EVENT2)
    include_directories(${LIBEVENT_INCLUDE_DIR})
# Сборка модуля AWH_EV
else(CMAKE_BUILD_EVENT2)
    include_directories(${LIBEV_INCLUDE_DIR})
endif(CMAKE_BUILD_EVENT2)

# Сборка модуля AWH_EVENT2
if(CMAKE_BUILD_EVENT2)
     # Выполняем подключение исходников для LibEvent2
     set(SOURCE_FILES
        "${CMAKE_SOURCE_DIR}/src/lib/event2/sys/events.cpp"
        "${CMAKE_SOURCE_DIR}/src/lib/event2/sys/signals.cpp"
        "${CMAKE_SOURCE_DIR}/src/lib/event2/sys/cluster.cpp"
        "${CMAKE_SOURCE_DIR}/src/lib/event2/core/core.cpp"
        "${CMAKE_SOURCE_DIR}/src/lib/event2/core/client.cpp"
        "${CMAKE_SOURCE_DIR}/src/lib/event2/core/server.cpp"
    )
# Сборка модуля AWH_EV
else(CMAKE_BUILD_EVENT2)
    # Выполняем подключение исходников для LibEv
    set(SOURCE_FILES
        "${CMAKE_SOURCE_DIR}/src/lib/ev/sys/signals.cpp"
        "${CMAKE_SOURCE_DIR}/src/lib/ev/sys/cluster.cpp"
        "${CMAKE_SOURCE_DIR}/src/lib/ev/core/core.cpp"
        "${CMAKE_SOURCE_DIR}/src/lib/ev/core/client.cpp"
        "${CMAKE_SOURCE_DIR}/src/lib/ev/core/server.cpp"
    )
endif(CMAKE_BUILD_EVENT2)

# Выполняем подключение исходников
set(SOURCE_FILES
    "${SOURCE_FILES}"
    "${CMAKE_SOURCE_DIR}/src/sys/fs.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/fmk.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/log.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/reg.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/idw.cpp"
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
    "${CMAKE_SOURCE_DIR}/src/http/proxy.cpp"
    "${CMAKE_SOURCE_DIR}/src/http/client.cpp"
    "${CMAKE_SOURCE_DIR}/src/http/server.cpp"
    "${CMAKE_SOURCE_DIR}/src/http/nghttp2.cpp"
    "${CMAKE_SOURCE_DIR}/src/auth/core.cpp"
    "${CMAKE_SOURCE_DIR}/src/auth/client.cpp"
    "${CMAKE_SOURCE_DIR}/src/auth/server.cpp"
    "${CMAKE_SOURCE_DIR}/src/cluster/core.cpp"
    "${CMAKE_SOURCE_DIR}/src/client/ws.cpp"
    "${CMAKE_SOURCE_DIR}/src/client/awh.cpp"
    "${CMAKE_SOURCE_DIR}/src/client/sample.cpp"
    "${CMAKE_SOURCE_DIR}/src/client/web/web.cpp"
    "${CMAKE_SOURCE_DIR}/src/client/web/web2.cpp"
    "${CMAKE_SOURCE_DIR}/src/client/web/ws1.cpp"
    "${CMAKE_SOURCE_DIR}/src/client/web/ws2.cpp"
    "${CMAKE_SOURCE_DIR}/src/client/web/http1.cpp"
    "${CMAKE_SOURCE_DIR}/src/client/web/http2.cpp"
    "${CMAKE_SOURCE_DIR}/src/server/awh.cpp"
    "${CMAKE_SOURCE_DIR}/src/server/proxy.cpp"
    "${CMAKE_SOURCE_DIR}/src/server/socks5.cpp"
    "${CMAKE_SOURCE_DIR}/src/server/sample.cpp"
    "${CMAKE_SOURCE_DIR}/src/server/websocket.cpp"
    "${CMAKE_SOURCE_DIR}/src/server/web/web.cpp"
    "${CMAKE_SOURCE_DIR}/src/server/web/web2.cpp"
    "${CMAKE_SOURCE_DIR}/src/server/web/http1.cpp"
    "${CMAKE_SOURCE_DIR}/src/socks5/core.cpp"
    "${CMAKE_SOURCE_DIR}/src/socks5/client.cpp"
    "${CMAKE_SOURCE_DIR}/src/socks5/server.cpp"
    "${CMAKE_SOURCE_DIR}/src/scheme/core.cpp"
    "${CMAKE_SOURCE_DIR}/src/scheme/web.cpp"
    "${CMAKE_SOURCE_DIR}/src/scheme/proxy.cpp"
    "${CMAKE_SOURCE_DIR}/src/scheme/socks5.cpp"
    "${CMAKE_SOURCE_DIR}/src/scheme/client.cpp"
    "${CMAKE_SOURCE_DIR}/src/scheme/server.cpp"
    "${CMAKE_SOURCE_DIR}/src/scheme/sample.cpp"
    "${CMAKE_SOURCE_DIR}/src/scheme/websocket.cpp"
    "${CMAKE_SOURCE_DIR}/contrib/src/bigint/BigInteger.cc"
    "${CMAKE_SOURCE_DIR}/contrib/src/bigint/BigUnsigned.cc"
    "${CMAKE_SOURCE_DIR}/contrib/src/bigint/BigIntegerUtils.cc"
    "${CMAKE_SOURCE_DIR}/contrib/src/bigint/BigUnsignedInABase.cc"
    "${CMAKE_SOURCE_DIR}/contrib/src/bigint/BigIntegerAlgorithms.cc"
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
set(WS_CLIENT_APP_NAME wsClient)
set(WS_SERVER_APP_NAME wsServer)
set(WEB_CLIENT_APP_NAME webClient)
set(WEB_SERVER_APP_NAME webServer)
set(CURRENT_LIB_NAME_STATIC ${PROJECT_NAME})
set(CURRENT_LIB_NAME_SHARED ${PROJECT_NAME})
set(CURRENT_APP_NAME ${PROJECT_NAME}_bin)

# Если операционной системой не является Windows
if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(CLUSTER_APP_NAME cluster)
endif()

# Если нужно собрать и динамическую библиотеку
if (CMAKE_SHARED_BUILD_LIB)
    # Добавляем динамическую библиотеку
    add_library(${CURRENT_LIB_NAME_SHARED} SHARED ${SOURCE_FILES})
    target_compile_definitions(${CURRENT_LIB_NAME_SHARED} PRIVATE AWH_SHARED_LIBRARY_EXPORT)
# Если нужно собрать и статическую библиотеку
else(CMAKE_SHARED_BUILD_LIB)
    # Активируем сборку статической библиотеки
    add_compile_definitions(AWH_STATICLIB)
    # Добавляем статическу библиотеку
    add_library(${CURRENT_LIB_NAME_STATIC} STATIC ${SOURCE_FILES})
endif(CMAKE_SHARED_BUILD_LIB)

 # Если операционной системой является Windows
 if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    # Устанавливаем необходимые зависимости для MS Windows
    set(WINFLAGS -lws2_32 -lgdi32 -lcrypt32 -lIphlpapi -lbcrypt)

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
    add_executable(${WS_CLIENT_APP_NAME} sample/wsClient2.cpp)
    add_executable(${WEB_SERVER_APP_NAME} sample/webServer.cpp)
    add_executable(${WEB_CLIENT_APP_NAME} sample/webClient.cpp)

endif()

# Если нужно собрать и динамическую библиотеку
if (CMAKE_SHARED_BUILD_LIB)
    # Если операционной системой является Windows
    if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")

        # Выполняем сборку динамической библиотеки
        target_link_libraries(
            ${CURRENT_LIB_NAME_SHARED}
            PRIVATE
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения работы с IP-адресами
        target_link_libraries(
            ${IP_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения WebSocket-клиент
        target_link_libraries(
            ${WS_CLIENT_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения WebSocket-сервер
        target_link_libraries(
            ${WS_SERVER_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения DNS-резолвер
        target_link_libraries(
            ${DNS_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения NTP-клиента
        target_link_libraries(
            ${NTP_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Socks5-сервера
        target_link_libraries(
            ${SOCKS5_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Proxy-сервера
        target_link_libraries(
            ${PROXY_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Ping-клиента
        target_link_libraries(
            ${PING_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения таймера
        target_link_libraries(
            ${TIMER_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения клиента
        target_link_libraries(
            ${CLIENT_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения сервера
        target_link_libraries(
            ${SERVER_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения HTTP-клиента
        target_link_libraries(
            ${WEB_CLIENT_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения HTTP-сервера
        target_link_libraries(
            ${WEB_SERVER_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
            ${WINFLAGS}
            ${AWH_IDN}
        )

    # Сборка примеров с поддержкой модуля AWH_IDN для Linux, FreeBSD, MacOS X
    elseif(CMAKE_BUILD_IDN)

        # Выполняем сборку динамической библиотеки
        target_link_libraries(
            ${CURRENT_LIB_NAME_SHARED}
            PRIVATE
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${IDN2_LIBRARY}
            ${ICONV_LIBRARIES}
            ${PCRE_LIBRARIES}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${AWH_IDN}
        )
        
        # Выполняем сборку приложения работы с IP-адресами
        target_link_libraries(
            ${IP_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения WebSocket-клиент
        target_link_libraries(
            ${WS_CLIENT_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения WebSocket-сервер
        target_link_libraries(
            ${WS_SERVER_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения DNS-резолвер
        target_link_libraries(
            ${DNS_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения NTP-клиента
        target_link_libraries(
            ${NTP_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Socks5-сервера
        target_link_libraries(
            ${SOCKS5_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Proxy-сервера
        target_link_libraries(
            ${PROXY_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Ping-клиента
        target_link_libraries(
            ${PING_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения таймера
        target_link_libraries(
            ${TIMER_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения клиента
        target_link_libraries(
            ${CLIENT_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения сервера
        target_link_libraries(
            ${SERVER_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения кластера
        target_link_libraries(
            ${CLUSTER_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения HTTP-клиента
        target_link_libraries(
            ${WEB_CLIENT_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения HTTP-сервера
        target_link_libraries(
            ${WEB_SERVER_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
            ${AWH_IDN}
        )

    # Если операционной системой является Linux, FreeBSD, MacOS X
    else()

        # Выполняем сборку динамической библиотеки
        target_link_libraries(
            ${CURRENT_LIB_NAME_SHARED}
            PRIVATE
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${PCRE_LIBRARIES}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
        )
        
        # Выполняем сборку приложения работы с IP-адресами
        target_link_libraries(
            ${IP_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
        )

        # Выполняем сборку приложения WebSocket-клиент
        target_link_libraries(
            ${WS_CLIENT_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
        )

        # Выполняем сборку приложения WebSocket-сервер
        target_link_libraries(
            ${WS_SERVER_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
        )

        # Выполняем сборку приложения DNS-резолвер
        target_link_libraries(
            ${DNS_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
        )

        # Выполняем сборку приложения NTP-клиента
        target_link_libraries(
            ${NTP_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
        )

        # Выполняем сборку приложения Socks5-сервера
        target_link_libraries(
            ${SOCKS5_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
        )

        # Выполняем сборку приложения Proxy-сервера
        target_link_libraries(
            ${PROXY_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
        )

        # Выполняем сборку приложения Ping-клиента
        target_link_libraries(
            ${PING_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
        )

        # Выполняем сборку приложения таймера
        target_link_libraries(
            ${TIMER_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
        )

        # Выполняем сборку приложения клиента
        target_link_libraries(
            ${CLIENT_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
        )

        # Выполняем сборку приложения сервера
        target_link_libraries(
            ${SERVER_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
        )

        # Выполняем сборку приложения кластера
        target_link_libraries(
            ${CLUSTER_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
        )

        # Выполняем сборку приложения HTTP-клиента
        target_link_libraries(
            ${WEB_CLIENT_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
        )

        # Выполняем сборку приложения HTTP-сервера
        target_link_libraries(
            ${WEB_SERVER_APP_NAME}
            ${CURRENT_LIB_NAME_SHARED}
        )

    endif()
# Если нужно собрать и статическую библиотеку
else(CMAKE_SHARED_BUILD_LIB)
    # Если операционной системой является Windows
    if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")

        # Выполняем сборку приложения работы с IP-адресами
        target_link_libraries(
            ${IP_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения WebSocket-клиент
        target_link_libraries(
            ${WS_CLIENT_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения WebSocket-сервер
        target_link_libraries(
            ${WS_SERVER_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения DNS-резолвер
        target_link_libraries(
            ${DNS_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения NTP-клиента
        target_link_libraries(
            ${NTP_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Socks5-сервера
        target_link_libraries(
            ${SOCKS5_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Proxy-сервера
        target_link_libraries(
            ${PROXY_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Ping-клиента
        target_link_libraries(
            ${PING_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения таймера
        target_link_libraries(
            ${TIMER_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения клиента
        target_link_libraries(
            ${CLIENT_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения сервера
        target_link_libraries(
            ${SERVER_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения HTTP-клиента
        target_link_libraries(
            ${WEB_CLIENT_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения HTTP-сервера
        target_link_libraries(
            ${WEB_SERVER_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

    # Сборка примеров с поддержкой модуля AWH_IDN для Linux, FreeBSD, MacOS X
    elseif(CMAKE_BUILD_IDN)

        # Выполняем сборку приложения работы с IP-адресами
        target_link_libraries(
            ${IP_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${IDN2_LIBRARY}
            ${ICONV_LIBRARIES}
            ${PCRE_LIBRARIES}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения WebSocket-клиент
        target_link_libraries(
            ${WS_CLIENT_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${IDN2_LIBRARY}
            ${ICONV_LIBRARIES}
            ${PCRE_LIBRARIES}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения WebSocket-сервер
        target_link_libraries(
            ${WS_SERVER_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${IDN2_LIBRARY}
            ${ICONV_LIBRARIES}
            ${PCRE_LIBRARIES}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения DNS-резолвер
        target_link_libraries(
            ${DNS_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${IDN2_LIBRARY}
            ${ICONV_LIBRARIES}
            ${PCRE_LIBRARIES}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения NTP-клиента
        target_link_libraries(
            ${NTP_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${IDN2_LIBRARY}
            ${ICONV_LIBRARIES}
            ${PCRE_LIBRARIES}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Socks5-сервера
        target_link_libraries(
            ${SOCKS5_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${IDN2_LIBRARY}
            ${ICONV_LIBRARIES}
            ${PCRE_LIBRARIES}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Proxy-сервера
        target_link_libraries(
            ${PROXY_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${IDN2_LIBRARY}
            ${ICONV_LIBRARIES}
            ${PCRE_LIBRARIES}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Ping-клиента
        target_link_libraries(
            ${PING_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${IDN2_LIBRARY}
            ${ICONV_LIBRARIES}
            ${PCRE_LIBRARIES}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения таймера
        target_link_libraries(
            ${TIMER_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${IDN2_LIBRARY}
            ${ICONV_LIBRARIES}
            ${PCRE_LIBRARIES}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения клиента
        target_link_libraries(
            ${CLIENT_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${IDN2_LIBRARY}
            ${ICONV_LIBRARIES}
            ${PCRE_LIBRARIES}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения сервера
        target_link_libraries(
            ${SERVER_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${IDN2_LIBRARY}
            ${ICONV_LIBRARIES}
            ${PCRE_LIBRARIES}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения кластера
        target_link_libraries(
            ${CLUSTER_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${IDN2_LIBRARY}
            ${ICONV_LIBRARIES}
            ${PCRE_LIBRARIES}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения HTTP-клиента
        target_link_libraries(
            ${WEB_CLIENT_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${IDN2_LIBRARY}
            ${ICONV_LIBRARIES}
            ${PCRE_LIBRARIES}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения HTTP-сервера
        target_link_libraries(
            ${WEB_SERVER_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${IDN2_LIBRARY}
            ${ICONV_LIBRARIES}
            ${PCRE_LIBRARIES}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${AWH_IDN}
        )

    # Если операционной системой является Linux, FreeBSD, MacOS X
    else()

        # Выполняем сборку приложения работы с IP-адресами
        target_link_libraries(
            ${IP_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${PCRE_LIBRARIES}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
        )

        # Выполняем сборку приложения WebSocket-клиент
        target_link_libraries(
            ${WS_CLIENT_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${PCRE_LIBRARIES}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
        )

        # Выполняем сборку приложения WebSocket-сервер
        target_link_libraries(
            ${WS_SERVER_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${PCRE_LIBRARIES}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
        )

        # Выполняем сборку приложения DNS-резолвер
        target_link_libraries(
            ${DNS_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${PCRE_LIBRARIES}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
        )

        # Выполняем сборку приложения NTP-клиента
        target_link_libraries(
            ${NTP_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${PCRE_LIBRARIES}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
        )

        # Выполняем сборку приложения Socks5-сервера
        target_link_libraries(
            ${SOCKS5_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${PCRE_LIBRARIES}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
        )

        # Выполняем сборку приложения Proxy-сервера
        target_link_libraries(
            ${PROXY_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${PCRE_LIBRARIES}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
        )

        # Выполняем сборку приложения Ping-клиента
        target_link_libraries(
            ${PING_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${PCRE_LIBRARIES}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
        )

        # Выполняем сборку приложения таймера
        target_link_libraries(
            ${TIMER_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${PCRE_LIBRARIES}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
        )

        # Выполняем сборку приложения клиента
        target_link_libraries(
            ${CLIENT_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${PCRE_LIBRARIES}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
        )

        # Выполняем сборку приложения сервера
        target_link_libraries(
            ${SERVER_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${PCRE_LIBRARIES}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
        )

        # Выполняем сборку приложения кластера
        target_link_libraries(
            ${CLUSTER_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${PCRE_LIBRARIES}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
        )

        # Выполняем сборку приложения HTTP-клиента
        target_link_libraries(
            ${WEB_CLIENT_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${PCRE_LIBRARIES}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
        )

        # Выполняем сборку приложения HTTP-сервера
        target_link_libraries(
            ${WEB_SERVER_APP_NAME}
            ${CURRENT_LIB_NAME_STATIC}
            ${EVENT_LIBRARY}
            ${XML_LIBRARY}
            ${ZLIB_LIBRARY}
            ${PCRE_LIBRARIES}
            ${NGTCP2_LIBRARY}
            ${NGHTTP2_LIBRARY}
            ${NGHTTP3_LIBRARY}
            ${JEMALLOC_LIBRARY}
            ${BROTLI_LIBRARIES}
            ${OPENSSL_LIBRARIES}
        )

    endif()
endif(CMAKE_SHARED_BUILD_LIB)

# Активация формирования дампа ядра в реже отладки под MacOS X
if(CMAKE_BUILD_TYPE MATCHES Debug)
    if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
        set(entitlements_file "${CMAKE_SOURCE_DIR}/segv.entitlements")
        execute_process(COMMAND /usr/libexec/PlistBuddy -c "Add :com.apple.security.get-task-allow bool true" "${entitlements_file}")
        
        add_custom_command(TARGET "${IP_APP_NAME}" POST_BUILD
            COMMAND codesign -s - -f --entitlements "${entitlements_file}" "$<TARGET_FILE:${IP_APP_NAME}>"
            COMMAND ulimit -c unlimited
            COMMENT "sign target \"${IP_APP_NAME}\" to enable core dump on MacOS X"
            VERBATIM
        )

        add_custom_command(TARGET "${WS_CLIENT_APP_NAME}" POST_BUILD
            COMMAND codesign -s - -f --entitlements "${entitlements_file}" "$<TARGET_FILE:${WS_CLIENT_APP_NAME}>"
            COMMAND ulimit -c unlimited
            COMMENT "sign target \"${WS_CLIENT_APP_NAME}\" to enable core dump on MacOS X"
            VERBATIM
        )

        add_custom_command(TARGET "${WS_SERVER_APP_NAME}" POST_BUILD
            COMMAND codesign -s - -f --entitlements "${entitlements_file}" "$<TARGET_FILE:${WS_SERVER_APP_NAME}>"
            COMMAND ulimit -c unlimited
            COMMENT "sign target \"${WS_SERVER_APP_NAME}\" to enable core dump on MacOS X"
            VERBATIM
        )

        add_custom_command(TARGET "${DNS_APP_NAME}" POST_BUILD
            COMMAND codesign -s - -f --entitlements "${entitlements_file}" "$<TARGET_FILE:${DNS_APP_NAME}>"
            COMMAND ulimit -c unlimited
            COMMENT "sign target \"${DNS_APP_NAME}\" to enable core dump on MacOS X"
            VERBATIM
        )

        add_custom_command(TARGET "${NTP_APP_NAME}" POST_BUILD
            COMMAND codesign -s - -f --entitlements "${entitlements_file}" "$<TARGET_FILE:${NTP_APP_NAME}>"
            COMMAND ulimit -c unlimited
            COMMENT "sign target \"${NTP_APP_NAME}\" to enable core dump on MacOS X"
            VERBATIM
        )

        add_custom_command(TARGET "${PING_APP_NAME}" POST_BUILD
            COMMAND codesign -s - -f --entitlements "${entitlements_file}" "$<TARGET_FILE:${PING_APP_NAME}>"
            COMMAND ulimit -c unlimited
            COMMENT "sign target \"${PING_APP_NAME}\" to enable core dump on MacOS X"
            VERBATIM
        )

        add_custom_command(TARGET "${TIMER_APP_NAME}" POST_BUILD
            COMMAND codesign -s - -f --entitlements "${entitlements_file}" "$<TARGET_FILE:${TIMER_APP_NAME}>"
            COMMAND ulimit -c unlimited
            COMMENT "sign target \"${TIMER_APP_NAME}\" to enable core dump on MacOS X"
            VERBATIM
        )

        add_custom_command(TARGET "${PROXY_APP_NAME}" POST_BUILD
            COMMAND codesign -s - -f --entitlements "${entitlements_file}" "$<TARGET_FILE:${PROXY_APP_NAME}>"
            COMMAND ulimit -c unlimited
            COMMENT "sign target \"${PROXY_APP_NAME}\" to enable core dump on MacOS X"
            VERBATIM
        )
        
        add_custom_command(TARGET "${SOCKS5_APP_NAME}" POST_BUILD
            COMMAND codesign -s - -f --entitlements "${entitlements_file}" "$<TARGET_FILE:${SOCKS5_APP_NAME}>"
            COMMAND ulimit -c unlimited
            COMMENT "sign target \"${SOCKS5_APP_NAME}\" to enable core dump on MacOS X"
            VERBATIM
        )
        
        add_custom_command(TARGET "${CLIENT_APP_NAME}" POST_BUILD
            COMMAND codesign -s - -f --entitlements "${entitlements_file}" "$<TARGET_FILE:${CLIENT_APP_NAME}>"
            COMMAND ulimit -c unlimited
            COMMENT "sign target \"${CLIENT_APP_NAME}\" to enable core dump on MacOS X"
            VERBATIM
        )
            
        add_custom_command(TARGET "${SERVER_APP_NAME}" POST_BUILD
            COMMAND codesign -s - -f --entitlements "${entitlements_file}" "$<TARGET_FILE:${SERVER_APP_NAME}>"
            COMMAND ulimit -c unlimited
            COMMENT "sign target \"${SERVER_APP_NAME}\" to enable core dump on MacOS X"
            VERBATIM
        )

        add_custom_command(TARGET "${WEB_CLIENT_APP_NAME}" POST_BUILD
            COMMAND codesign -s - -f --entitlements "${entitlements_file}" "$<TARGET_FILE:${WEB_CLIENT_APP_NAME}>"
            COMMAND ulimit -c unlimited
            COMMENT "sign target \"${WEB_CLIENT_APP_NAME}\" to enable core dump on MacOS X"
            VERBATIM
        )
        
        add_custom_command(TARGET "${WEB_SERVER_APP_NAME}" POST_BUILD
            COMMAND codesign -s - -f --entitlements "${entitlements_file}" "$<TARGET_FILE:${WEB_SERVER_APP_NAME}>"
            COMMAND ulimit -c unlimited
            COMMENT "sign target \"${WEB_SERVER_APP_NAME}\" to enable core dump on MacOS X"
            VERBATIM
        )
        
        add_custom_command(TARGET "${CLUSTER_APP_NAME}" POST_BUILD
            COMMAND codesign -s - -f --entitlements "${entitlements_file}" "$<TARGET_FILE:${CLUSTER_APP_NAME}>"
            COMMAND ulimit -c unlimited
            COMMENT "sign target \"${CLUSTER_APP_NAME}\" to enable core dump on MacOS X"
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

# Устанавливаем основные зависимости
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/contrib/include" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.hh")
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/contrib/include" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.hpp")

# Устанавливаем хидеры
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include" DESTINATION "${CMAKE_INSTALL_PREFIX}/include/${CMAKE_PROJECT_NAME}" FILES_MATCHING PATTERN "*.hpp")

# Устанавливаем библиотеку
if(CMAKE_SHARED_BUILD_LIB)
    # Устанавливаем динамическую библиотеку
    install(TARGETS ${CURRENT_LIB_NAME_SHARED} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
else(CMAKE_SHARED_BUILD_LIB)
    # Устанавливаем статическую библиотеку
    install(TARGETS ${CURRENT_LIB_NAME_STATIC} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
endif(CMAKE_SHARED_BUILD_LIB)

# Устанавливаем исполнительные файлы
install(TARGETS ${IP_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${WS_CLIENT_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${WS_SERVER_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${DNS_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${NTP_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${PING_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${TIMER_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${PROXY_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${SOCKS5_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${CLIENT_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${SERVER_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${WEB_CLIENT_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${WEB_SERVER_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")

# Если операционной системой не является Windows
if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    install(TARGETS ${CLUSTER_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
endif()

# Устанавливаем хидеры сторонних библиотек
install(DIRECTORY "${XML_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${ZLIB_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${NGTCP2_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${NGHTTP2_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${NGHTTP3_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${JEMALLOC_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${BROTLI_INCLUDE_ENCODE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${OPENSSL_INCLUDE_DIR}/openssl" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")

# Если операционной системой не является Windows
if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    # Устанавливаем хидеры сторонних библиотек
    install(DIRECTORY "${PCRE_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
endif()

# Сборка модуля AWH_EVENT2
if(CMAKE_BUILD_EVENT2)
    install(DIRECTORY "${LIBEVENT_INCLUDE_DIR}/event2" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
# Сборка модуля AWH_EV
else(CMAKE_BUILD_EVENT2)
    install(DIRECTORY "${LIBEV_INCLUDE_DIR}/libev" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
endif(CMAKE_BUILD_EVENT2)

# Сборка модуля AWH_IDN, если операционной системой не является Windows
if(CMAKE_BUILD_IDN AND (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows"))
    install(DIRECTORY "${IDN2_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
    install(DIRECTORY "${ICONV_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
endif()

# Устанавливаем статические библиотеки
if(NOT CMAKE_SHARED_BUILD_LIB)
    # Устанавливаем стороние библиотеки
    install(FILES ${XML_LIBRARY} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
    install(FILES ${ZLIB_LIBRARY} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
    install(FILES ${NGTCP2_LIBRARY} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
    install(FILES ${NGHTTP2_LIBRARY} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
    install(FILES ${NGHTTP3_LIBRARY} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
    install(FILES ${JEMALLOC_LIBRARY} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
    install(FILES ${BROTLI_LIBRARIES} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
    install(FILES ${OPENSSL_LIBRARIES} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")

    # Если операционной системой не является Windows
    if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
        # Устанавливаем стороние библиотеки
        install(FILES ${PCRE_LIBRARIES} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
    endif()

    # Сборка модуля AWH_EVENT2
    if(CMAKE_BUILD_EVENT2)
        install(FILES ${LIBEVENT_LIBRARIES} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
    # Сборка модуля AWH_EV
    else(CMAKE_BUILD_EVENT2)
        install(FILES ${LIBEV_LIBRARY} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
    endif(CMAKE_BUILD_EVENT2)

    # Сборка модуля AWH_IDN, если операционной системой не является Windows
    if(CMAKE_BUILD_IDN AND (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows"))
        install(FILES ${IDN2_LIBRARY} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
        install(FILES ${ICONV_LIBRARIES} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
    endif()
endif(NOT CMAKE_SHARED_BUILD_LIB)
