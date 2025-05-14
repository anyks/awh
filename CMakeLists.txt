cmake_minimum_required(VERSION 3.15)

# TODO: вписать рабочее название проекта
project(awh LANGUAGES CXX)

# Устанавливаем название приложения
SET(PROJECT_VENDOR_LONG "ANYKS - WEB")

# Опции сборки
SET(PROJECT_COPYRIGHT "Copyright (c) ${CURRENT_YEAR} ${PROJECT_VENDOR_LONG}")

# Тип сборки
# cmake -DCMAKE_BUILD_TYPE=YES ..
if (NOT CMAKE_BUILD_TYPE)
    # Активируем режим отладки
    SET(CMAKE_BUILD_TYPE Debug)
endif (NOT CMAKE_BUILD_TYPE)

# Выводим сообщение о типе сборки
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}\n")

# Сборка модуля AWH_IDN
if (CMAKE_BUILD_IDN)
    add_compile_definitions(AWH_IDN)
    message(STATUS "Build AWH IDN: YES\n")
# Сборка модуля AWH_IDN не требуется
else (CMAKE_BUILD_IDN)
    message(STATUS "Build AWH IDN: NO\n")
endif (CMAKE_BUILD_IDN)

# Выводим информацию каталога где выполняется сброка
message(STATUS "Build dir: ${CMAKE_CURRENT_BINARY_DIR}\n")

# Получаем архитектуру
EXECUTE_PROCESS(COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE )
message(STATUS "Architecture: ${ARCHITECTURE}")

# Проверка версии OS
string(REGEX MATCH "Linux" PROJECT_OS_LINUX ${CMAKE_SYSTEM_NAME})
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    message(STATUS "Detected OS: Linux")
    SET(DEBUGGER "-ggdb3")
    # Используем Clang
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
     # Используем GCC
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--no-as-needed -ldl")
    # Используем Intel C++
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    # Используем Visual Studio C++
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    endif()
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    if (NOT MACOS_VERSION)
        EXECUTE_PROCESS(COMMAND sw_vers -productVersion COMMAND tr -d '\n' OUTPUT_VARIABLE MACOS_VERSION)
    endif()
    message(STATUS "Detected OS: MacOS X v${MACOS_VERSION}")
    SET(DEBUGGER "-glldb")    
    SET(CMAKE_OSX_DEPLOYMENT_TARGET ${MACOS_VERSION})
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    message(STATUS "Detected OS: FreeBSD")
    SET(DEBUGGER "-ggdb3")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "NetBSD")
    message(STATUS "Detected OS: NetBSD")
    SET(DEBUGGER "-ggdb3")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
    message(STATUS "Detected OS: OpenBSD")
    SET(DEBUGGER "-ggdb3")
else()
    SET(DEBUGGER "")
endif()

# Enable C++17
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

# Используем Clang
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Устанавливаем опции компилятора
    SET(CMAKE_C_COMPILER_ID "clang")
    SET(CMAKE_CXX_COMPILER_ID "clang++")
# Используем GCC
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # Устанавливаем опции компилятора
    SET(CMAKE_C_COMPILER_ID "GNU")
    SET(CMAKE_CXX_COMPILER_ID "GNU")
# Используем Intel C++
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    # Устанавливаем опции компилятора
    SET(CMAKE_C_COMPILER_ID "Intel")
    SET(CMAKE_CXX_COMPILER_ID "Intel")
# Используем Visual Studio C++
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # Устанавливаем опции компилятора
    SET(CMAKE_C_COMPILER_ID "MSVC")
    SET(CMAKE_CXX_COMPILER_ID "MSVC")
endif()

# Выводим информацию об установленных компиляторах
message(STATUS "Using C compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Using C++ compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}\n")

# Информация и времени компиляции
string(TIMESTAMP CURRENT_YEAR "%Y")
string(TIMESTAMP CURRENT_TIME "%d.%m.%Y %H:%M:%S")

# Опции компилятора
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    SET(AWH_PARAMS "-Wno-cpp -Wno-deprecated")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    SET(AWH_PARAMS "-Wno-cpp -Wno-deprecated")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    SET(AWH_PARAMS "-framework Carbon -Wno-cpp -Wno-deprecated -Wno-unused-command-line-argument")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "NetBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
    SET(AWH_PARAMS "-lutil -Wno-cpp -Wno-deprecated -Wno-unused-command-line-argument")
else()
    SET(AWH_PARAMS "-Wno-unknown-attributes")
endif()

# Если процессор архитектуры ARM
if (${ARCHITECTURE} MATCHES "aarch64" OR ${ARCHITECTURE} MATCHES "arm*")
    SET(CMAKE_CXX_FLAGS_RELEASE "-pipe -DNDEBUG")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fno-permissive -Wno-pedantic -Wno-narrowing -Wno-deprecated-declarations -Wno-exceptions -Wno-unused-result ${AWH_PARAMS}")
# Если процессор архитектуры x86_64
else()
    SET(CMAKE_CXX_FLAGS_RELEASE "-pipe -mrdrnd -march=core2 -DNDEBUG")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fno-permissive -Wno-pedantic -Wno-narrowing -Wno-deprecated-declarations -Wno-exceptions -Wno-unused-result ${AWH_PARAMS}")
endif()

# Устанавливаем параметр отладки
SET(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG_MODE ${DEBUGGER}")

# Если активирован режим отладки
if (CMAKE_BUILD_TYPE MATCHES Debug)
    # Устанавливаем уровень оптимизации
    SET(CMAKE_C_FLAGS_DISTRIBUTION "-O0")
    SET(CMAKE_CXX_FLAGS_DISTRIBUTION "-O0")

    # Если операцинная система не относится к MS Windows
    if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
        # Устанавливаем сборку профилировщика
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
        SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
        SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
        ##
        #
        # sudo apt-get install graphviz
        # sudo pip install gprof2dot
        #
        # gprof ./build/[executable-name] gmon.out > ./profile-data.txt
        #
        # gprof2dot ./profile-data.txt | dot -Tsvg -o output.svg
        ##
    endif()
    # Выводим сообщение отладочной информации
    message(STATUS "C Flags: ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_DEBUG}")
    message(STATUS "C++ Flags: ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG}\n")
# Если активирован режим релиза
else()
    # Устанавливаем уровень оптимизации
    SET(CMAKE_C_FLAGS_DISTRIBUTION "-O3")
    SET(CMAKE_CXX_FLAGS_DISTRIBUTION "-O3")

    # Выводим сообщение релизной информации
    message(STATUS "C Flags: ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_RELEASE}")
    message(STATUS "C++ Flags: ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}\n")
endif()

# Если нужно собрать и динамическую библиотеку
if (CMAKE_SHARED_BUILD_LIB)
    if (MSVC)
        SET(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    endif (MSVC)
endif (CMAKE_SHARED_BUILD_LIB)

# Добавляем модули cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Выполняем библиотек зависимостей
find_package(Dependence REQUIRED)

# Провекра Dependence
if (Dependence_FOUND)
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
    if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
        # Сборка модуля AWH_IDN
        if (CMAKE_BUILD_IDN)
            message(STATUS "IDN2 include dir found: " ${IDN2_INCLUDE_DIR})
            message(STATUS "ICONV include dir found: " ${ICONV_INCLUDE_DIR})
        endif()
    endif()
else (Dependence_FOUND)
    message(FATAL_ERROR "Dependence not found")
endif (Dependence_FOUND)

# Выполняем поиск библиотеки TcMalloc
find_package(TcMalloc REQUIRED)

# Провекра TcMalloc
if (TcMalloc_FOUND)
    message(STATUS "TcMalloc lib found: " ${TCMALLOC_LIBRARY} " " ${TCMALLOC_COMMON_LIBRARY})
else (TcMalloc_FOUND)
    message(FATAL_ERROR "TcMalloc not found")
endif (TcMalloc_FOUND)

# Выполняем подключение хидеров
include_directories(SYSTEM ${DEPEND_INCLUDE_DIRS})
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/include)
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/contrib/include)
include_directories(SYSTEM ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})

# Выполняем подключение исходников
SET(SOURCE_FILES
    "${SOURCE_FILES}"
    "${CMAKE_SOURCE_DIR}/src/sys/os.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/fs.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/ver.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/fmk.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/log.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/reg.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/hash.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/queue.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/chrono.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/buffer.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/signals.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/investigator.cpp"
    "${CMAKE_SOURCE_DIR}/src/cluster/cmp.cpp"
    "${CMAKE_SOURCE_DIR}/src/cluster/cluster.cpp"
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
    "${CMAKE_SOURCE_DIR}/src/http/web.cpp"
    "${CMAKE_SOURCE_DIR}/src/http/core.cpp"
    "${CMAKE_SOURCE_DIR}/src/http/http2.cpp"
    "${CMAKE_SOURCE_DIR}/src/http/client.cpp"
    "${CMAKE_SOURCE_DIR}/src/http/server.cpp"
    "${CMAKE_SOURCE_DIR}/src/auth/core.cpp"
    "${CMAKE_SOURCE_DIR}/src/auth/client.cpp"
    "${CMAKE_SOURCE_DIR}/src/auth/server.cpp"
    "${CMAKE_SOURCE_DIR}/src/events/evpipe.cpp"
    "${CMAKE_SOURCE_DIR}/src/events/evbase.cpp"
    "${CMAKE_SOURCE_DIR}/src/events/evtimer.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/core.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/node.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/timer.cpp"
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
    "${CMAKE_SOURCE_DIR}/src/scheme/core.cpp"
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
SET(IP_APP_NAME ip)
SET(DNS_APP_NAME dns)
SET(NTP_APP_NAME ntp)
SET(PING_APP_NAME ping)
SET(DATE_APP_NAME date)
SET(TIMER_APP_NAME timer)
SET(PROXY_APP_NAME proxy)
SET(SOCKS5_APP_NAME socks5)
SET(CLIENT_APP_NAME client)
SET(SERVER_APP_NAME server)
SET(WS_CLIENT_APP_NAME wsClient)
SET(WS_SERVER_APP_NAME wsServer)
SET(WEB_CLIENT_APP_NAME webClient)
SET(WEB_SERVER_APP_NAME webServer)
SET(INVESTIGATOR_APP_NAME investigator)
SET(AWH_LIB_NAME ${PROJECT_NAME})

# Если операционной системой не является Windows
if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    SET(CLUSTER_APP_NAME cluster)
endif()

# Если нужно собрать и динамическую библиотеку
if (CMAKE_SHARED_BUILD_LIB)
    # Добавляем динамическую библиотеку
    add_library(${AWH_LIB_NAME} SHARED ${SOURCE_FILES})
    target_compile_definitions(${AWH_LIB_NAME} PRIVATE AWH_SHARED_LIBRARY_EXPORT)
# Если нужно собрать и статическую библиотеку
else (CMAKE_SHARED_BUILD_LIB)
    # Активируем сборку статической библиотеки
    add_compile_definitions(AWH_STATICLIB)
    # Добавляем статическу библиотеку
    add_library(${AWH_LIB_NAME} STATIC ${SOURCE_FILES})
endif (CMAKE_SHARED_BUILD_LIB)

 # Если операционной системой является Windows
 if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    # export MSYS="error_start:%WD%dumper.exe -d -c %1 %2"
    # Устанавливаем необходимые зависимости для MS Windows
    SET(WINFLAGS -lstdc++ -lws2_32 -lgdi32 -lcrypt32 -lIphlpapi -lbcrypt -flto=auto)

    # Устанавливаем иконку для windows под MinGW.
    SET(RES_FILES "awh.rc")
    SET(CMAKE_RC_COMPILER_INIT windres)
    ENABLE_LANGUAGE(RC)
    SET(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")

    # Выполняем создание демонстрационных приложений
    add_executable(${IP_APP_NAME} sample/ip.cpp ${RES_FILES})
    add_executable(${DNS_APP_NAME} sample/dns.cpp ${RES_FILES})
    add_executable(${NTP_APP_NAME} sample/ntp.cpp ${RES_FILES})
    add_executable(${PING_APP_NAME} sample/ping.cpp ${RES_FILES})
    add_executable(${DATE_APP_NAME} sample/date.cpp ${RES_FILES})
    add_executable(${TIMER_APP_NAME} sample/timer.cpp ${RES_FILES})
    add_executable(${PROXY_APP_NAME} sample/proxy.cpp ${RES_FILES})
    add_executable(${SOCKS5_APP_NAME} sample/socks5.cpp ${RES_FILES})
    add_executable(${CLIENT_APP_NAME} sample/client.cpp ${RES_FILES})
    add_executable(${SERVER_APP_NAME} sample/server.cpp ${RES_FILES})
    add_executable(${WS_SERVER_APP_NAME} sample/wsServer.cpp ${RES_FILES})
    add_executable(${WS_CLIENT_APP_NAME} sample/wsClient.cpp ${RES_FILES})
    add_executable(${WEB_SERVER_APP_NAME} sample/webServer.cpp ${RES_FILES})
    add_executable(${WEB_CLIENT_APP_NAME} sample/webClient.cpp ${RES_FILES})
    add_executable(${INVESTIGATOR_APP_NAME} sample/investigator.cpp ${RES_FILES})
# Если операционной системой является Linux, FreeBSD, NetBSD, OpenBSD, MacOS X
else()
    # Выполняем создание демонстрационных приложений
    add_executable(${IP_APP_NAME} sample/ip.cpp)
    add_executable(${DNS_APP_NAME} sample/dns.cpp)
    add_executable(${NTP_APP_NAME} sample/ntp.cpp)
    add_executable(${PING_APP_NAME} sample/ping.cpp)
    add_executable(${DATE_APP_NAME} sample/date.cpp)
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
    add_executable(${INVESTIGATOR_APP_NAME} sample/investigator.cpp)
endif()

# Если нужно собрать и динамическую библиотеку
if (CMAKE_SHARED_BUILD_LIB)
    # Если операционной системой является Windows
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")

        # Выполняем сборку динамической библиотеки
        target_link_libraries(
            ${AWH_LIB_NAME}
            PRIVATE
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения работы с IP-адресами
        target_link_libraries(
            ${IP_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения WebSocket-клиент
        target_link_libraries(
            ${WS_CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения WebSocket-сервер
        target_link_libraries(
            ${WS_SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения DNS-резолвер
        target_link_libraries(
            ${DNS_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения NTP-клиента
        target_link_libraries(
            ${NTP_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Proxy-сервера
        target_link_libraries(
            ${PROXY_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Socks5-сервера
        target_link_libraries(
            ${SOCKS5_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Ping-клиента
        target_link_libraries(
            ${PING_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения даты и времени
        target_link_libraries(
            ${DATE_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения таймера
        target_link_libraries(
            ${TIMER_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения клиента
        target_link_libraries(
            ${CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения сервера
        target_link_libraries(
            ${SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения HTTP-клиента
        target_link_libraries(
            ${WEB_CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения HTTP-сервера
        target_link_libraries(
            ${WEB_SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения дознавателя
        target_link_libraries(
            ${INVESTIGATOR_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

    # Сборка примеров с поддержкой модуля AWH_IDN для Linux, FreeBSD, NetBSD, OpenBSD, MacOS X
    elseif (CMAKE_BUILD_IDN)

        # Выполняем сборку динамической библиотеки
        target_link_libraries(
            ${AWH_LIB_NAME}
            PRIVATE
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )
        
        # Выполняем сборку приложения работы с IP-адресами
        target_link_libraries(
            ${IP_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения WebSocket-клиент
        target_link_libraries(
            ${WS_CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения WebSocket-сервер
        target_link_libraries(
            ${WS_SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения DNS-резолвер
        target_link_libraries(
            ${DNS_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения NTP-клиента
        target_link_libraries(
            ${NTP_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Proxy-сервера
        target_link_libraries(
            ${PROXY_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Socks5-сервера
        target_link_libraries(
            ${SOCKS5_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Ping-клиента
        target_link_libraries(
            ${PING_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения даты и времени
        target_link_libraries(
            ${DATE_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения таймера
        target_link_libraries(
            ${TIMER_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения клиента
        target_link_libraries(
            ${CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения сервера
        target_link_libraries(
            ${SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения кластера
        target_link_libraries(
            ${CLUSTER_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения HTTP-клиента
        target_link_libraries(
            ${WEB_CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения HTTP-сервера
        target_link_libraries(
            ${WEB_SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения дознавателя
        target_link_libraries(
            ${INVESTIGATOR_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

    # Если операционной системой является Linux, FreeBSD, NetBSD, OpenBSD, MacOS X
    else()

        # Выполняем сборку динамической библиотеки
        target_link_libraries(
            ${AWH_LIB_NAME}
            PRIVATE
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
        )
        
        # Выполняем сборку приложения работы с IP-адресами
        target_link_libraries(
            ${IP_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
        )

        # Выполняем сборку приложения WebSocket-клиент
        target_link_libraries(
            ${WS_CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
        )

        # Выполняем сборку приложения WebSocket-сервер
        target_link_libraries(
            ${WS_SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
        )

        # Выполняем сборку приложения DNS-резолвер
        target_link_libraries(
            ${DNS_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
        )

        # Выполняем сборку приложения NTP-клиента
        target_link_libraries(
            ${NTP_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
        )

        # Выполняем сборку приложения Proxy-сервера
        target_link_libraries(
            ${PROXY_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
        )

        # Выполняем сборку приложения Socks5-сервера
        target_link_libraries(
            ${SOCKS5_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
        )

        # Выполняем сборку приложения Ping-клиента
        target_link_libraries(
            ${PING_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
        )

        # Выполняем сборку приложения даты и времени
        target_link_libraries(
            ${DATE_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
        )

        # Выполняем сборку приложения таймера
        target_link_libraries(
            ${TIMER_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
        )

        # Выполняем сборку приложения клиента
        target_link_libraries(
            ${CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
        )

        # Выполняем сборку приложения сервера
        target_link_libraries(
            ${SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
        )

        # Выполняем сборку приложения кластера
        target_link_libraries(
            ${CLUSTER_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
        )

        # Выполняем сборку приложения HTTP-клиента
        target_link_libraries(
            ${WEB_CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
        )

        # Выполняем сборку приложения HTTP-сервера
        target_link_libraries(
            ${WEB_SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
        )

        # Выполняем сборку приложения дознавателя
        target_link_libraries(
            ${INVESTIGATOR_APP_NAME}
            ${AWH_LIB_NAME}
            ${TCMALLOC_LIBRARIES}
        )

    endif()
# Если нужно собрать и статическую библиотеку
else (CMAKE_SHARED_BUILD_LIB)
    # Если операционной системой является Windows
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")

        # Выполняем сборку приложения работы с IP-адресами
        target_link_libraries(
            ${IP_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения WebSocket-клиент
        target_link_libraries(
            ${WS_CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения WebSocket-сервер
        target_link_libraries(
            ${WS_SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения DNS-резолвер
        target_link_libraries(
            ${DNS_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения NTP-клиента
        target_link_libraries(
            ${NTP_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Proxy-сервера
        target_link_libraries(
            ${PROXY_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Socks5-сервера
        target_link_libraries(
            ${SOCKS5_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Ping-клиента
        target_link_libraries(
            ${PING_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения даты и времени
        target_link_libraries(
            ${DATE_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения таймера
        target_link_libraries(
            ${TIMER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения клиента
        target_link_libraries(
            ${CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения сервера
        target_link_libraries(
            ${SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения HTTP-клиента
        target_link_libraries(
            ${WEB_CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения HTTP-сервера
        target_link_libraries(
            ${WEB_SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения дознавателя
        target_link_libraries(
            ${INVESTIGATOR_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
            ${AWH_IDN}
        )

    # Сборка примеров с поддержкой модуля AWH_IDN для Linux, FreeBSD, NetBSD, OpenBSD, MacOS X
    elseif (CMAKE_BUILD_IDN)

        # Выполняем сборку приложения работы с IP-адресами
        target_link_libraries(
            ${IP_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения WebSocket-клиент
        target_link_libraries(
            ${WS_CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения WebSocket-сервер
        target_link_libraries(
            ${WS_SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения DNS-резолвер
        target_link_libraries(
            ${DNS_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения NTP-клиента
        target_link_libraries(
            ${NTP_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Proxy-сервера
        target_link_libraries(
            ${PROXY_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Socks5-сервера
        target_link_libraries(
            ${SOCKS5_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения Ping-клиента
        target_link_libraries(
            ${PING_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения даты и времени
        target_link_libraries(
            ${DATE_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения таймера
        target_link_libraries(
            ${TIMER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения клиента
        target_link_libraries(
            ${CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения сервера
        target_link_libraries(
            ${SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения кластера
        target_link_libraries(
            ${CLUSTER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения HTTP-клиента
        target_link_libraries(
            ${WEB_CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения HTTP-сервера
        target_link_libraries(
            ${WEB_SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

        # Выполняем сборку приложения дознавателя
        target_link_libraries(
            ${INVESTIGATOR_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${AWH_IDN}
        )

    # Если операционной системой является Linux, FreeBSD, NetBSD, OpenBSD, MacOS X
    else()

        # Выполняем сборку приложения работы с IP-адресами
        target_link_libraries(
            ${IP_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
        )

        # Выполняем сборку приложения WebSocket-клиент
        target_link_libraries(
            ${WS_CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
        )

        # Выполняем сборку приложения WebSocket-сервер
        target_link_libraries(
            ${WS_SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
        )

        # Выполняем сборку приложения DNS-резолвер
        target_link_libraries(
            ${DNS_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
        )

        # Выполняем сборку приложения NTP-клиента
        target_link_libraries(
            ${NTP_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
        )

        # Выполняем сборку приложения Proxy-сервера
        target_link_libraries(
            ${PROXY_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
        )

        # Выполняем сборку приложения Socks5-сервера
        target_link_libraries(
            ${SOCKS5_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
        )

        # Выполняем сборку приложения Ping-клиента
        target_link_libraries(
            ${PING_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
        )

        # Выполняем сборку приложения даты и времени
        target_link_libraries(
            ${DATE_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
        )

        # Выполняем сборку приложения таймера
        target_link_libraries(
            ${TIMER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
        )

        # Выполняем сборку приложения клиента
        target_link_libraries(
            ${CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
        )

        # Выполняем сборку приложения сервера
        target_link_libraries(
            ${SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
        )

        # Выполняем сборку приложения кластера
        target_link_libraries(
            ${CLUSTER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
        )

        # Выполняем сборку приложения HTTP-клиента
        target_link_libraries(
            ${WEB_CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
        )

        # Выполняем сборку приложения HTTP-сервера
        target_link_libraries(
            ${WEB_SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
        )

        # Выполняем сборку приложения дознавателя
        target_link_libraries(
            ${INVESTIGATOR_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
        )

    endif()
endif (CMAKE_SHARED_BUILD_LIB)

# Активация формирования дампа ядра в реже отладки под MacOS X
if (CMAKE_BUILD_TYPE MATCHES Debug)
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
        SET(entitlements_file "${CMAKE_SOURCE_DIR}/segv.entitlements")
        
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

        add_custom_command(TARGET "${DATE_APP_NAME}" POST_BUILD
            COMMAND bash "${CMAKE_SOURCE_DIR}/core_dump.sh" "$<TARGET_FILE:${DATE_APP_NAME}>" "${entitlements_file}"
            COMMENT "Sign target \"${DATE_APP_NAME}\" to enable core dump on MacOS X"
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

        add_custom_command(TARGET "${INVESTIGATOR_APP_NAME}" POST_BUILD
            COMMAND bash "${CMAKE_SOURCE_DIR}/core_dump.sh" "$<TARGET_FILE:${INVESTIGATOR_APP_NAME}>" "${entitlements_file}"
            COMMENT "Sign target \"${INVESTIGATOR_APP_NAME}\" to enable core dump on MacOS X"
            VERBATIM
        )

    endif()
    # Сборка модуля CMAKE_ENABLED_PVS
    if (CMAKE_ENABLED_PVS)
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
        pvs_studio_add_target(TARGET ${DATE_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${DATE_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
        pvs_studio_add_target(TARGET ${TIMER_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${TIMER_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
        pvs_studio_add_target(TARGET ${PROXY_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${PROXY_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
        pvs_studio_add_target(TARGET ${SOCKS5_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SOCKS5_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
        pvs_studio_add_target(TARGET ${CLIENT_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
        pvs_studio_add_target(TARGET ${SERVER_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
        pvs_studio_add_target(TARGET ${WEB_CLIENT_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${WEB_CLIENT_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
        pvs_studio_add_target(TARGET ${WEB_SERVER_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${WEB_SERVER_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
        pvs_studio_add_target(TARGET ${CLUSTER_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLUSTER_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
        pvs_studio_add_target(TARGET ${INVESTIGATOR_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${INVESTIGATOR_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})

    endif (CMAKE_ENABLED_PVS)
endif()

# Устанавливаем адрес установки, запрет установки в /usr/local
if ("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr/local" OR "${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr/local/")
    SET(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}")
endif()

# Устанавливаем сертификаты
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/certs/" DESTINATION "${CMAKE_INSTALL_PREFIX}/certs")

# Устанавливаем хидеры
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/" DESTINATION "${CMAKE_INSTALL_PREFIX}/include/${CMAKE_PROJECT_NAME}" FILES_MATCHING PATTERN "*.hpp")
# Устанавливаем хидеры сторонних библиотек
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/contrib/include/" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h" PATTERN "*.hh" PATTERN "*.hpp")

# Устанавливаем библиотеку
if (NOT CMAKE_SHARED_BUILD_LIB)
    # Если режим отладки не активирован
    if (NOT CMAKE_BUILD_TYPE MATCHES Debug)
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

# Устанавливаем статическую библиотеку AWH
install(TARGETS ${AWH_LIB_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
# Устанавливаем статическую библиотеку TcMalloc
install(FILES ${TCMALLOC_LIBRARIES} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")

# Устанавливаем исполнительные файлы
install(TARGETS ${IP_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${DNS_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${NTP_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${PING_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${DATE_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${TIMER_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${PROXY_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${SOCKS5_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${CLIENT_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${SERVER_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${WS_CLIENT_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${WS_SERVER_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${WEB_CLIENT_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${WEB_SERVER_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${INVESTIGATOR_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")

# Если операционной системой не является Windows
if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    install(TARGETS ${CLUSTER_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
endif()

# Выполняем запуск скрипта генерации CMAKE-файла
add_custom_command(TARGET "${AWH_LIB_NAME}" POST_BUILD
    COMMAND bash "${CMAKE_SOURCE_DIR}/build_cmake.sh" "${CMAKE_INSTALL_PREFIX}"
VERBATIM)

# Устанавливаем сгенерированный CMAKE-файл
install(FILES "${CMAKE_SOURCE_DIR}/third_party/cmake/FindAWH.cmake" DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake")
