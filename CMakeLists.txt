# Устанавливаем минимальную версию CMake
cmake_minimum_required(VERSION 3.15)

# TODO: вписать рабочее название проекта
project(awh LANGUAGES CXX)

# Устанавливаем название приложения
SET(PROJECT_VENDOR_LONG "ANYKS - WEB HUB (AWH)")

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
    message(STATUS "Build with IDN: YES\n")
# Сборка модуля AWH_IDN не требуется
else (CMAKE_BUILD_IDN)
    message(STATUS "Build with IDN: NO\n")
endif (CMAKE_BUILD_IDN)

# Если активирован флаг оптимизации операционной системы
if (CMAKE_BOOSTING_NET)
    add_compile_definitions(AWH_BOOSTING_NET)
    message(STATUS "Build with NET boosting: YES\n")
endif (CMAKE_BOOSTING_NET)

# Если операционной системой является Solaris
if (${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
    # Если активирован флаг оптимизации операционной системы
    if (CMAKE_USING_SOLARIS_PORTS)
        add_compile_definitions(AWH_USING_EVENTS_PORTS)
        message(STATUS "Build with using Event Ports: YES\n")
    endif (CMAKE_USING_SOLARIS_PORTS)
endif()

# Сборка с установкой количества доступных файловых дескрипторов (по умолчанию: 131072)
if (CMAKE_MAX_COUNT_FDS)
    add_compile_definitions(AWH_MAX_COUNT_FDS=${CMAKE_MAX_COUNT_FDS})
    message(STATUS "Build with max FDs: ${CMAKE_MAX_COUNT_FDS}\n")
endif (CMAKE_MAX_COUNT_FDS)

# Сборка с установкой адреса NTP-серверов по умолчанию
if (CMAKE_DEFAULT_NTP_SERVERS)
    add_compile_definitions(AWH_NTP_SERVERS=${CMAKE_DEFAULT_NTP_SERVERS})
    message(STATUS "Build with default NTP servers: ${CMAKE_DEFAULT_NTP_SERVERS}\n")
endif (CMAKE_DEFAULT_NTP_SERVERS)

# Сборка с установкой адреса DNS-серверов IPv4 по умолчанию
if (CMAKE_DEFAULT_DNS_SERVERS)
    add_compile_definitions(AWH_IPV4_NS=${CMAKE_DEFAULT_DNS_SERVERS})
    message(STATUS "Build with default DNS servers: ${CMAKE_DEFAULT_DNS_SERVERS}\n")
endif (CMAKE_DEFAULT_DNS_SERVERS)

# Сборка с установкой адреса DNS-серверов IPv6 по умолчанию
if (CMAKE_DEFAULT_DNS6_SERVERS)
    add_compile_definitions(AWH_IPV6_NS=${CMAKE_DEFAULT_DNS6_SERVERS})
    message(STATUS "Build with default IPv6 DNS servers: ${CMAKE_DEFAULT_DNS6_SERVERS}\n")
endif (CMAKE_DEFAULT_DNS6_SERVERS)

# Если активирован флаг установки максимального размера HTTP-тела запроса/ответа (по умолчанию: 10485760 = 10Mb)
if (CMAKE_MAX_HTTP_BODY_SIZE)
    add_compile_definitions(AWH_MAX_BODY_SIZE=${CMAKE_MAX_HTTP_BODY_SIZE})
    message(STATUS "Build with max body size: ${CMAKE_MAX_HTTP_BODY_SIZE}\n")
endif (CMAKE_MAX_HTTP_BODY_SIZE)

# Если активирован флаг установки максимального размера SSL буфера (по умолчанию: 16384)
if (CMAKE_MAX_SSL_BUFFER_SIZE)
    add_compile_definitions(AWH_MAX_SSL_BUFFER_SIZE=${CMAKE_MAX_SSL_BUFFER_SIZE})
    message(STATUS "Build with max SSL buffer size: ${CMAKE_MAX_SSL_BUFFER_SIZE}\n")
endif (CMAKE_MAX_SSL_BUFFER_SIZE)

# Если активирован флаг установки максимального размера буфера на чтение данных событий (по умолчанию: 65536)
if (CMAKE_EVENT_MAX_BUFFER_SIZE)
    add_compile_definitions(AWH_EVENT_MAX_BUFFER_SIZE=${CMAKE_EVENT_MAX_BUFFER_SIZE})
    message(STATUS "Build with max event buffer size: ${CMAKE_EVENT_MAX_BUFFER_SIZE}\n")
endif (CMAKE_EVENT_MAX_BUFFER_SIZE)

# Если активирован флаг установки максимального количества опрашиваемых событий за одну итерацию (по умолчанию: 64)
if (CMAKE_MAX_POLL_EVENTS_COUNT)
    add_compile_definitions(AWH_MAX_POLL_EVENTS_COUNT=${CMAKE_MAX_POLL_EVENTS_COUNT})
    message(STATUS "Build with max poll events count: ${CMAKE_MAX_POLL_EVENTS_COUNT}\n")
endif (CMAKE_MAX_POLL_EVENTS_COUNT)

# Если активирован флаг установки максимального времени ожидания получения данных в наносекундхах (по умолчанию: 500000 наносекунд = 0.5 мс)
if (CMAKE_EVENT_MAX_LOOP_WAIT)
    add_compile_definitions(AWH_EVENT_MAX_LOOP_WAIT=${CMAKE_EVENT_MAX_LOOP_WAIT})
    message(STATUS "Build with max event loop wait: ${CMAKE_EVENT_MAX_LOOP_WAIT}\n")
endif (CMAKE_EVENT_MAX_LOOP_WAIT)

# Если активирован флаг установки максимального числа одноразовых HMAC nonce (по умолчанию: 4096)
if (CMAKE_AUTH_HMAC_NONCE_MAX)
    add_compile_definitions(AWH_AUTH_HMAC_NONCE_MAX=${CMAKE_AUTH_HMAC_NONCE_MAX})
    message(STATUS "Build with max count HMAC-nonce: ${CMAKE_AUTH_HMAC_NONCE_MAX}\n")
endif (CMAKE_AUTH_HMAC_NONCE_MAX)

# Если активирован флаг установки максимального число записей replay-защиты Digest (lncs) (по умолчанию: 4096)
if (CMAKE_AUTH_DIGEST_LNCS_MAX)
    add_compile_definitions(AWH_AUTH_DIGEST_LNCS_MAX=${CMAKE_AUTH_DIGEST_LNCS_MAX})
    message(STATUS "Build with max count Digest lncs: ${CMAKE_AUTH_DIGEST_LNCS_MAX}\n")
endif (CMAKE_AUTH_DIGEST_LNCS_MAX)

# Если активирован флаг установки максимального количества заголовков в HTTP запросе/ответе (по умолчанию: 100)
if (CMAKE_MAX_HTTP_COUNT_HEADERS)
    add_compile_definitions(AWH_MAX_COUNT_HTTP_HEADERS=${CMAKE_MAX_HTTP_COUNT_HEADERS})
    message(STATUS "Build with max count HTTP headers: ${CMAKE_MAX_HTTP_COUNT_HEADERS}\n")
endif (CMAKE_MAX_HTTP_COUNT_HEADERS)

# Если активирован флаг установки максимального количества памяти для хранения заголовков в HTTP запросе/ответе (по умолчанию: 16384 = 16 КБ)
if (CMAKE_MAX_HTTP_MEMORY_HEADERS)
    add_compile_definitions(AWH_MAX_MEMORY_HTTP_HEADERS=${CMAKE_MAX_HTTP_MEMORY_HEADERS})
    message(STATUS "Build with max memory HTTP headers: ${CMAKE_MAX_HTTP_MEMORY_HEADERS}\n")
endif (CMAKE_MAX_HTTP_MEMORY_HEADERS)

# Если активирован флаг установки размера итераций PBKDF2 для вывода ключа AES из пароля (по умолчанию: 100000)
if (CMAKE_CRYPTO_AES_ROUNDS)
    add_compile_definitions(AWH_CRYPTO_AES_ROUNDS=${CMAKE_CRYPTO_AES_ROUNDS})
    message(STATUS "Build with crypto AES rounds: ${CMAKE_CRYPTO_AES_ROUNDS}\n")
endif (CMAKE_CRYPTO_AES_ROUNDS)

# Если активирован флаг установки размера блока для потокового сжатия (по умолчанию: 65536 = 64 КБ)
if (CMAKE_COMPRESSOR_STREAM_CHUNK)
    add_compile_definitions(AWH_COMPRESSOR_STREAM_CHUNK=${CMAKE_COMPRESSOR_STREAM_CHUNK})
    message(STATUS "Build with compressor stream chunk: ${CMAKE_COMPRESSOR_STREAM_CHUNK}\n")
endif (CMAKE_COMPRESSOR_STREAM_CHUNK)

# Если активирован флаг установки максимального размера очереди (по умолчанию: 1000)
if (CMAKE_MAX_RECORDS_QUEUE)
    add_compile_definitions(AWH_MAX_RECORDS_QUEUE=${CMAKE_MAX_RECORDS_QUEUE})
    message(STATUS "Build with max count records queue: ${CMAKE_MAX_RECORDS_QUEUE}\n")
endif (CMAKE_MAX_RECORDS_QUEUE)

# Если активирован флаг установки максимального размера потребления памяти очередью (по умолчанию: 10485760 = 10Mb)
if (CMAKE_MAX_MEMORY_QUEUE)
    add_compile_definitions(AWH_MAX_MEMORY_QUEUE=${CMAKE_MAX_MEMORY_QUEUE})
    message(STATUS "Build with max memory queue usage size: ${CMAKE_MAX_MEMORY_QUEUE}\n")
endif (CMAKE_MAX_MEMORY_QUEUE)

# Если активирован флаг установки максимального размера потребления памяти буфера (по умолчанию: 10485760 = 10Mb)
if (CMAKE_MAX_MEMORY_BUFFER)
    add_compile_definitions(AWH_MAX_MEMORY_BUFFER=${CMAKE_MAX_MEMORY_BUFFER})
    message(STATUS "Build with max memory buffer usage size: ${CMAKE_MAX_MEMORY_BUFFER}\n")
endif (CMAKE_MAX_MEMORY_BUFFER)

# Если активирован флаг установки максимального размера буфера для очереди сетевых событий (по умолчанию: 65536)
if (CMAKE_NETWORK_QUEUE_BUFFER_SIZE)
    add_compile_definitions(AWH_NETWORK_QUEUE_BUFFER_SIZE=${CMAKE_NETWORK_QUEUE_BUFFER_SIZE})
    message(STATUS "Build with max network queue buffer size: ${CMAKE_NETWORK_QUEUE_BUFFER_SIZE}\n")
endif (CMAKE_NETWORK_QUEUE_BUFFER_SIZE)

# Если активирован флаг установки максимального количества свободных блоков для очереди сетевых событий (по умолчанию: 64)
if (CMAKE_NETWORK_QUEUE_BLOCK_POOL_CAP)
    add_compile_definitions(AWH_NETWORK_QUEUE_BLOCK_POOL_CAP=${CMAKE_NETWORK_QUEUE_BLOCK_POOL_CAP})
    message(STATUS "Build with max network queue block pool cap: ${CMAKE_NETWORK_QUEUE_BLOCK_POOL_CAP}\n")
endif (CMAKE_NETWORK_QUEUE_BLOCK_POOL_CAP)

# Если активирован флаг установки размера MTU для UDP сообщений в IPv4 (по умолчанию: 1400)
if (CMAKE_MTU_UDP_IPV4_PAYLOAD_SIZE)
    add_compile_definitions(AWH_MTU_UDP_IPV4_PAYLOAD_SIZE=${CMAKE_MTU_UDP_IPV4_PAYLOAD_SIZE})
    message(STATUS "Build with MTU UDP IPv4 payload size: ${CMAKE_MTU_UDP_IPV4_PAYLOAD_SIZE}\n")
endif (CMAKE_MTU_UDP_IPV4_PAYLOAD_SIZE)

# Если активирован флаг установки размера MTU для UDP сообщений в IPv6 (по умолчанию: 1380)
if (CMAKE_MTU_UDP_IPV6_PAYLOAD_SIZE)
    add_compile_definitions(AWH_MTU_UDP_IPV6_PAYLOAD_SIZE=${CMAKE_MTU_UDP_IPV6_PAYLOAD_SIZE})
    message(STATUS "Build with MTU UDP IPv6 payload size: ${CMAKE_MTU_UDP_IPV6_PAYLOAD_SIZE}\n")
endif (CMAKE_MTU_UDP_IPV6_PAYLOAD_SIZE)

# Если активирован флаг установки размера MTU для TCP сообщений в IPv4 (по умолчанию: 1440)
if (CMAKE_MTU_TCP_IPV4_PAYLOAD_SIZE)
    add_compile_definitions(AWH_MTU_TCP_IPV4_PAYLOAD_SIZE=${CMAKE_MTU_TCP_IPV4_PAYLOAD_SIZE})
    message(STATUS "Build with MTU TCP IPv4 payload size: ${CMAKE_MTU_TCP_IPV4_PAYLOAD_SIZE}\n")
endif (CMAKE_MTU_TCP_IPV4_PAYLOAD_SIZE)

# Если активирован флаг установки размера MTU для TCP сообщений в IPv6 (по умолчанию: 1420)
if (CMAKE_MTU_TCP_IPV6_PAYLOAD_SIZE)
    add_compile_definitions(AWH_MTU_TCP_IPV6_PAYLOAD_SIZE=${CMAKE_MTU_TCP_IPV6_PAYLOAD_SIZE})
    message(STATUS "Build with MTU TCP IPv6 payload size: ${CMAKE_MTU_TCP_IPV6_PAYLOAD_SIZE}\n")
endif (CMAKE_MTU_TCP_IPV6_PAYLOAD_SIZE)

# Если активирован флаг установки размера буфера чанка модуля компрессии/декомпрессии (по умолчанию: 16384)
if (CMAKE_COMPRESSOR_CHUNK_BUFFER_SIZE)
    add_compile_definitions(AWH_COMPRESSOR_CHUNK_BUFFER_SIZE=${CMAKE_COMPRESSOR_CHUNK_BUFFER_SIZE})
    message(STATUS "Build with compressor module chunk buffer size: ${CMAKE_COMPRESSOR_CHUNK_BUFFER_SIZE}\n")
endif (CMAKE_COMPRESSOR_CHUNK_BUFFER_SIZE)

# Если активирован флаг установки размера пакета для внутренних таймеров (по умолчанию: 256)
if (CMAKE_BATCH_SIZE_INTERNAL_TIMER)
    add_compile_definitions(AWH_BATCH_SIZE_INTERNAL_TIMER=${CMAKE_BATCH_SIZE_INTERNAL_TIMER})
    message(STATUS "Build with batch size for internal timers: ${CMAKE_BATCH_SIZE_INTERNAL_TIMER}\n")
endif (CMAKE_BATCH_SIZE_INTERNAL_TIMER)

# Если активирован флаг установки количества обрабатываемых таймеров за одну итерацию для внутренних таймеров (по умолчанию: 256)
if (CMAKE_COUNT_PROCESSED_INTERNAL_TIMER)
    add_compile_definitions(AWH_COUNT_PROCESSED_INTERNAL_TIMER=${CMAKE_COUNT_PROCESSED_INTERNAL_TIMER})
    message(STATUS "Build with count processed timers for internal timers: ${CMAKE_COUNT_PROCESSED_INTERNAL_TIMER}\n")
endif (CMAKE_COUNT_PROCESSED_INTERNAL_TIMER)

# Если активирован флаг установки максимального размера кучи для внутренних таймеров (по умолчанию: 50 000)
if (CMAKE_HEAP_SIZE_INTERNAL_TIMER)
    add_compile_definitions(AWH_HEAP_SIZE_INTERNAL_TIMER=${CMAKE_HEAP_SIZE_INTERNAL_TIMER})
    message(STATUS "Build with heap size for internal timers: ${CMAKE_HEAP_SIZE_INTERNAL_TIMER}\n")
endif (CMAKE_HEAP_SIZE_INTERNAL_TIMER)

# Если активирован флаг установки размера чанка для внутренних таймеров (по умолчанию: 1024, рекомендуется 4096 для высоконагруженных систем)
if (CMAKE_CHUNK_EIDS_INTERNAL_TIMER)
    add_compile_definitions(AWH_CHUNK_EIDS_INTERNAL_TIMER=${CMAKE_CHUNK_EIDS_INTERNAL_TIMER})
    message(STATUS "Build with chunk size for internal timers: ${CMAKE_CHUNK_EIDS_INTERNAL_TIMER}\n")
endif (CMAKE_CHUNK_EIDS_INTERNAL_TIMER)

# Если активирован флаг установки порога "незначительного" сдвига дедлайна для внутренних таймеров (по умолчанию: 2 мс)
if (CMAKE_SIFT_THRESHOLD_MS_INTERNAL_TIMER)
    add_compile_definitions(AWH_SIFT_THRESHOLD_MS_INTERNAL_TIMER=${CMAKE_SIFT_THRESHOLD_MS_INTERNAL_TIMER})
    message(STATUS "Build with sift threshold ms for internal timers: ${CMAKE_SIFT_THRESHOLD_MS_INTERNAL_TIMER}\n")
endif (CMAKE_SIFT_THRESHOLD_MS_INTERNAL_TIMER)

# Выводим информацию каталога где выполняется сборка
message(STATUS "Build dir: ${CMAKE_CURRENT_BINARY_DIR}\n")

# Получаем архитектуру
EXECUTE_PROCESS(COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE )
message(STATUS "Architecture: ${ARCHITECTURE}")

# Если сборка производится под операционную систему Linux
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
# Если сборка производится под операционную систему Solaris
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
    if (NOT OS_VERSION)
        EXECUTE_PROCESS(COMMAND uname -a COMMAND gawk "{print $4}" COMMAND tr -d '\n' OUTPUT_VARIABLE OS_VERSION)
    endif()
    message(STATUS "Detected OS: Solaris v${OS_VERSION}")
    SET(DEBUGGER "-ggdb3")
     # Используем Clang
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
     # Используем GCC
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--no-as-needed -ldl")
    endif()
    # Добавляем расширения Sun Solaris
    add_compile_definitions(__EXTENSIONS__)
# Если сборка производится под операционную систему macOS
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    if (NOT OS_VERSION)
        EXECUTE_PROCESS(COMMAND sw_vers -productVersion COMMAND tr -d '\n' OUTPUT_VARIABLE OS_VERSION)
    endif()
    message(STATUS "Detected OS: macOS v${OS_VERSION}")
    SET(DEBUGGER "-glldb")    
    SET(CMAKE_OSX_DEPLOYMENT_TARGET ${OS_VERSION})
# Если сборка производится под операционную систему FreeBSD
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    if (NOT OS_VERSION)
        EXECUTE_PROCESS(COMMAND uname -a COMMAND awk "{print $3}" COMMAND tr -d '\n' OUTPUT_VARIABLE OS_VERSION)
    endif()
    message(STATUS "Detected OS: FreeBSD v${OS_VERSION}")
    SET(DEBUGGER "-ggdb3")
# Если сборка производится под операционную систему NetBSD
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "NetBSD")
    message(STATUS "Detected OS: NetBSD")
    SET(DEBUGGER "-ggdb3")
# Если сборка производится под операционную систему OpenBSD
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
    message(STATUS "Detected OS: OpenBSD")
    SET(DEBUGGER "-ggdb3")
# Если сборка производится под операционную систему Windows
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    message(STATUS "Detected OS: Microsoft Windows")
    SET(DEBUGGER "-ggdb3")
# Если операционная система не определена
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

# Устанавливаем параметры игнорирования ошибок для macOS
if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    SET(AWH_PARAMS "-Wno-cpp -Wno-deprecated -Wno-unused-command-line-argument")
# Устанавливаем параметры игнорирования ошибок для Solaris
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
    SET(AWH_PARAMS "-lsocket -lnsl -lc -Wno-cpp -Wno-deprecated")
# Устанавливаем параметры игнорирования ошибок для Windows и Linux
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Windows" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    SET(AWH_PARAMS "-Wno-cpp -Wno-deprecated")
# Устанавливаем параметры игнорирования ошибок для FreeBSD, NetBSD и OpenBSD
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "NetBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
    SET(AWH_PARAMS "-lutil -lexecinfo -Wno-cpp -Wno-deprecated -Wno-unused-command-line-argument")
# Если операционная система не определена
else()
    SET(AWH_PARAMS "-Wno-unknown-attributes")
endif()

# Если процессор архитектуры ARM
if (${ARCHITECTURE} MATCHES "aarch64" OR ${ARCHITECTURE} MATCHES "arm*")
    SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -pipe -DNDEBUG")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fno-permissive -Wno-pedantic -Wno-narrowing -Wno-deprecated-declarations -Wno-exceptions -Wno-unused-result ${AWH_PARAMS}")
# Если процессор архитектуры x86_64
else()
    SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -pipe -mrdrnd -march=core2 -DNDEBUG")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fno-permissive -Wno-pedantic -Wno-narrowing -Wno-deprecated-declarations -Wno-exceptions -Wno-unused-result ${AWH_PARAMS}")
endif()

# Устанавливаем параметр отладки
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -DDEBUG_MODE ${DEBUGGER}")

# Если активирован режим отладки
if (CMAKE_BUILD_TYPE MATCHES Debug)
    # Устанавливаем уровень оптимизации
    SET(CMAKE_C_FLAGS_DISTRIBUTION "-O0")
    SET(CMAKE_CXX_FLAGS_DISTRIBUTION "-O0")

    # Если сборка производится не в динамическую библиотеку
    if(NOT CMAKE_SHARED_BUILD_LIB)
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
    endif(NOT CMAKE_SHARED_BUILD_LIB)
    # Выводим сообщение отладочной информации
    message(STATUS "C Flags: ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_DEBUG}")
    message(STATUS "C++ Flags: ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG}\n")
# Если активирован режим релиза
else()
    # Устанавливаем уровень оптимизации
    SET(CMAKE_C_FLAGS_DISTRIBUTION "-O3")
    SET(CMAKE_CXX_FLAGS_DISTRIBUTION "-O3")

    # Активируем использование TcMalloc
    add_compile_definitions(__AWH_USE_TCMALLOC__)

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

# Находим Doxygen
find_package(Doxygen QUIET)

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
    message(STATUS "Snappy include dir found: " ${SNAPPY_INCLUDE_DIR})
    message(STATUS "Lizard include dir found: " ${LIZARD_INCLUDE_DIR})
    message(STATUS "Density include dir found: " ${DENSITY_INCLUDE_DIR})
    message(STATUS "NgHttp2 include dir found: " ${NGHTTP2_INCLUDE_DIR})
    message(STATUS "BoringSSL include dir found: " ${BORINGSSL_INCLUDE_DIR})
    message(STATUS "MiniUPnPc include dir found: " ${MINIUPNPC_INCLUDE_DIR})
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
    message(STATUS "TcMalloc include dir found: " ${TCMALLOC_INCLUDE_DIR})
    message(STATUS "TcMalloc lib found: " ${TCMALLOC_LIBRARY} " " ${TCMALLOC_COMMON_LIBRARY})
else (TcMalloc_FOUND)
    message(FATAL_ERROR "TcMalloc not found")
endif (TcMalloc_FOUND)

# Если Doxygen найден
if(Doxygen_FOUND)
    # Предположим, что Doxyfile лежит в корне исходников
    SET(DOXYFILE_PATH ${CMAKE_SOURCE_DIR}/Doxyfile)

    # Проверяем, что файл существует
    if(EXISTS ${DOXYFILE_PATH})
        # Выводим сообщение, что мы нашли файл для генерации документации Doxyfile
        message(STATUS "Doxyfile found at ${DOXYFILE_PATH}\n")
        # Создаём кастомную цель
        add_custom_target(doc
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_PATH}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
    # Если файл не найден
    else()
        # Выводим сообщение об отсутствии Doxyfile
        message(STATUS "Doxyfile not found at ${DOXYFILE_PATH} — doc target skipped\n")
    # Конец проверки файла
    endif()
# Если Doxygen не найден
else()
    # Выводим сообщение об отсутствии Doxygen
    message(STATUS "Doxygen not found — 'doc' target will not be available\n")
endif()

# Выполняем подключение хидеров
include_directories(BEFORE ${DEPEND_INCLUDE_DIRS})
include_directories(BEFORE ${TCMALLOC_INCLUDE_DIR})
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/include)
include_directories(BEFORE ${CMAKE_SOURCE_DIR}/contrib/include)
include_directories(SYSTEM ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})

# Если сборка производится под операционные системы macOS, FreeBSD, NetBSD или OpenBSD
if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin" OR ${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "NetBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
    # Выполняем подключение бэкендов IO
    SET(SOURCE_FILES
        "${SOURCE_FILES}"
        "${CMAKE_SOURCE_DIR}/src/net/backend/bsd/eth.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/bsd/addr.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/bsd/iface.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/bsd/kqueue.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/bsd/socket.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/bsd/gateway.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/bsd/portmap.cpp"
    )

    # Если операционной системой является FreeBSD
    if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
        # Выполняем подключение бэкенда SCTP
        SET(SOURCE_FILES
            "${SOURCE_FILES}"
            "${CMAKE_SOURCE_DIR}/src/net/backend/bsd/sctp.cpp"
        )
    endif()
endif()

# Выполняем подключение исходников
SET(SOURCE_FILES
    "${SOURCE_FILES}"
    "${CMAKE_SOURCE_DIR}/src/sys/os.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/fs.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/fmk.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/log.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/reg.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/queue.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/buffer.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/procre.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/chrono.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/binbox.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/signals.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/version.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/crypto.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/net.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/fds.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/uri.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/nwt.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/addr.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/queue.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/tls/coder.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/tls/fingerprint.cpp"
    "${CMAKE_SOURCE_DIR}/src/units/fs.cpp"
    "${CMAKE_SOURCE_DIR}/src/units/dns.cpp"
    "${CMAKE_SOURCE_DIR}/src/units/ntp.cpp"
    "${CMAKE_SOURCE_DIR}/src/units/icmp.cpp"
    "${CMAKE_SOURCE_DIR}/src/units/unit.cpp"
    "${CMAKE_SOURCE_DIR}/src/units/timer.cpp"
    "${CMAKE_SOURCE_DIR}/src/units/client.cpp"
    "${CMAKE_SOURCE_DIR}/src/units/server.cpp"
    "${CMAKE_SOURCE_DIR}/src/units/tunnel.cpp"
    "${CMAKE_SOURCE_DIR}/src/units/cluster.cpp"
    "${CMAKE_SOURCE_DIR}/src/units/mediator.cpp"
    "${CMAKE_SOURCE_DIR}/src/units/notifier.cpp"
    "${CMAKE_SOURCE_DIR}/src/client/client.cpp"
    "${CMAKE_SOURCE_DIR}/src/client/socks5.cpp"
    "${CMAKE_SOURCE_DIR}/src/server/server.cpp"
    "${CMAKE_SOURCE_DIR}/src/server/socks5.cpp"
    "${CMAKE_SOURCE_DIR}/src/compressor/block.cpp"
    "${CMAKE_SOURCE_DIR}/src/compressor/types.cpp"
    "${CMAKE_SOURCE_DIR}/src/compressor/stream.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/socks5/socks5.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/socks5/client.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/socks5/server.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/http/headers.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/http/provider.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/http/auth/auth.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/http/auth/hmac.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/http/auth/basic.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/http/auth/digest.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/http/auth/bearer.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/http/parser/parser.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/http/parser/http1/http.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/http/parser/http2/h2.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/http/parser/http2/frame.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/http/parser/http2/hpack.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/http/parser/http2/http.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/quic/quic.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/quic/varint.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/quic/params.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/quic/crypto.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/quic/packet.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/quic/frame.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/quic/handshake.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/quic/connection.cpp"
    "${CMAKE_SOURCE_DIR}/contrib/src/cityhash/city.cc"
)

# Устанавливаем имя библиотеки
SET(AWH_LIB_NAME ${PROJECT_NAME})

# Если необходимо собрать тесты
if (CMAKE_BUILD_TESTS)
    # Задаем имя библиотеки для сбора статистики покрытия кода
    SET(AWH_LIB_GCOV_NAME ${PROJECT_NAME}_gcov)
    # Указываем что нужно собирать статистику покрытия кода тестами
    option(CMAKE_BUILD_COVERAGE "Enable code coverage instrumentation for tests" ON)
    # Если сбор статистики покрытия кода активирован
    if (CMAKE_BUILD_COVERAGE)
        # Флаги компиляции для инструментирования кода (gcov/llvm-cov)
        SET(GCC_COVERAGE_COMPILE_FLAGS --coverage)
        # Зависимости линковки для подключения рантайма сбора статистики покрытия
        SET(GCC_COVERAGE_LIBS --coverage)
    # Если сбор статистики покрытия кода отключён
    else()
        # Обнуляем флаги компиляции инструментирования кода
        SET(GCC_COVERAGE_COMPILE_FLAGS "")
        # Обнуляем зависимости линковки рантайма сбора статистики покрытия
        SET(GCC_COVERAGE_LIBS "")
    endif()
endif (CMAKE_BUILD_TESTS)

# Если нужно собрать и динамическую библиотеку
if (CMAKE_SHARED_BUILD_LIB)
    # Добавляем динамическую библиотеку
    add_library(${AWH_LIB_NAME} SHARED ${SOURCE_FILES})
    # Выполняем экспорт методов доступных извне в динамических библиотеках
    target_compile_definitions(${AWH_LIB_NAME} PRIVATE AWH_SHARED_LIBRARY_EXPORT)
# Если нужно собрать и статическую библиотеку
else (CMAKE_SHARED_BUILD_LIB)
    # Активируем сборку статической библиотеки
    add_compile_definitions(AWH_STATICLIB)
    # Добавляем статическу библиотеку
    add_library(${AWH_LIB_NAME} STATIC ${SOURCE_FILES})
endif (CMAKE_SHARED_BUILD_LIB)

# Если необходимо собрать тесты
if (CMAKE_BUILD_TESTS)
    # Собираем библиотеку с флагами gcov для сбора статистики покрытия тестами
    add_library(${AWH_LIB_GCOV_NAME} STATIC ${SOURCE_FILES})
    # Если сборка производится под операционную систему macOS
    if (APPLE AND NOT IOS)
        # Устанавливаем свойства компиляции для файловой системы
        target_compile_options(
            ${AWH_LIB_NAME} PRIVATE
            -x objective-c++ -fobjc-arc
        )
        # Устанавливаем свойства компиляции для файловой системы
        target_compile_options(
            ${AWH_LIB_GCOV_NAME} PRIVATE
            ${GCC_COVERAGE_COMPILE_FLAGS} -x objective-c++ -fobjc-arc
        )
    # Если сборка производится под операционную систему Windows
    else()
        # Включаем флаги компиляции для сбора статистики покрытия кода
        target_compile_options(${AWH_LIB_GCOV_NAME} PRIVATE ${GCC_COVERAGE_COMPILE_FLAGS})
    endif()
# Если сборка производится не для сбора тестов
else()
    # Если сборка производится под операционную систему macOS
    if (APPLE AND NOT IOS)
        # Устанавливаем свойства компиляции для файловой системы
        target_compile_options(
            ${AWH_LIB_NAME} PRIVATE
            -x objective-c++ -fobjc-arc
        )
    endif()
endif (CMAKE_BUILD_TESTS)

# Указываем что нужно собрать семплы
option(CMAKE_BUILD_SAMPLES "Build unit samples" OFF)

# Если необходимо собрать семплы
if (${CMAKE_BUILD_SAMPLES})
    # Указывает каталог для ВСЕХ исполняемых файлов
    SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/samples)

    # Устанавливаем имя приложения работы с клиентом UDS
    SET(CLIENT_APP_UDS_NAME client-uds)
    # Устанавливаем имя приложения работы с клиентом UDS версии 2
    SET(CLIENT_APP_UDS-2_NAME client-uds-2)
    # Устанавливаем имя приложения работы с клиентом TCP
    SET(CLIENT_APP_TCP_NAME client-tcp)
    # Устанавливаем имя приложения работы с клиентом TCP версии 2
    SET(CLIENT_APP_TCP-2_NAME client-tcp-2)
    # Устанавливаем имя приложения работы с клиентом UDP
    SET(CLIENT_APP_UDP_NAME client-udp)
    # Устанавливаем имя приложения работы с клиентом UDP версии 2
    SET(CLIENT_APP_UDP-2_NAME client-udp-2)
    # Устанавливаем имя приложения работы с клиентом TLS
    SET(CLIENT_APP_TLS_NAME client-tls)
    # Устанавливаем имя приложения работы с клиентом TLS версии 2
    SET(CLIENT_APP_TLS-2_NAME client-tls-2)
    # Устанавливаем имя приложения работы с клиентом DTLS
    SET(CLIENT_APP_DTLS_NAME client-dtls)
    # Устанавливаем имя приложения работы с клиентом DTLS версии 2
    SET(CLIENT_APP_DTLS-2_NAME client-dtls-2)
    # Устанавливаем имя приложения работы с клиентом TLS-Web
    SET(CLIENT_APP_TLS-WEB_NAME client-tls-web)
    # Если сборка производится под операционную систему FreeBSD или Linux
    if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        # Устанавливаем имя приложения работы с клиентом SCTP
        SET(CLIENT_APP_SCTP_NAME client-sctp)
        # Устанавливаем имя приложения работы с клиентом SCTP версии 2
        SET(CLIENT_APP_SCTP-2_NAME client-sctp-2)
    endif()
    # Указываем что нужно собрать приложение работы с клиентом
    option(CMAKE_BUILD_CLIENT_APP_SAMPLE "Build client app sample" ON)

    # Устанавливаем имя приложения работы с сервером UDS
    SET(SERVER_APP_UDS_NAME server-uds)
    # Устанавливаем имя приложения работы с сервером UDS версии 2
    SET(SERVER_APP_UDS-2_NAME server-uds-2)
    # Устанавливаем имя приложения работы с сервером TCP
    SET(SERVER_APP_TCP_NAME server-tcp)
    # Устанавливаем имя приложения работы с сервером TCP версии 2
    SET(SERVER_APP_TCP-2_NAME server-tcp-2)
    # Устанавливаем имя приложения работы с сервером UDP
    SET(SERVER_APP_UDP_NAME server-udp)
    # Устанавливаем имя приложения работы с сервером UDP версии 2
    SET(SERVER_APP_UDP-2_NAME server-udp-2)
    # Устанавливаем имя приложения работы с сервером TLS
    SET(SERVER_APP_TLS_NAME server-tls)
    # Устанавливаем имя приложения работы с сервером TLS мультисертификатным
    SET(SERVER_APP_TLS-MULTICERT_NAME server-tls-multicert)
    # Устанавливаем имя приложения работы с сервером DTLS
    SET(SERVER_APP_DTLS_NAME server-dtls)
    # Устанавливаем имя приложения работы с сервером DTLS мультисертификатным
    SET(SERVER_APP_DTLS-MULTICERT_NAME server-dtls-multicert)
    # Если сборка производится под операционную систему FreeBSD или Linux
    if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        # Устанавливаем имя приложения работы с сервером SCTP
        SET(SERVER_APP_SCTP_NAME server-sctp)
        # Устанавливаем имя приложения работы с сервером SCTP версии 2
        SET(SERVER_APP_SCTP-2_NAME server-sctp-2)
    endif()
    # Указываем что нужно собрать приложение работы с сервером
    option(CMAKE_BUILD_SERVER_APP_SAMPLE "Build server app sample" ON)

    # Устанавливаем имя приложения работы с VPN клиентом
    SET(CLIENT_APP_VPN_NAME client-vpn)
    # Устанавливаем имя приложения работы с VPN сервером
    SET(SERVER_APP_VPN_NAME server-vpn)
    # Указываем что нужно собрать приложение работы с VPN
    option(CMAKE_BUILD_VPN_APP_SAMPLE "Build VPN app sample" ON)

    # Устанавливаем имя приложения работы с SOCKS5 клиентом
    SET(CLIENT_APP_SOCKS5_NAME client-socks5)
    # Устанавливаем имя приложения работы с SOCKS5 клиентом для UDP
    SET(CLIENT_APP_SOCKS5-UDP_NAME client-socks5-udp)
    # Устанавливаем имя приложения работы с SOCKS5 клиентом для DTLS
    SET(CLIENT_APP_SOCKS5-DTLS_NAME client-socks5-dtls)
    # Устанавливаем имя приложения работы с SOCKS5 сервером
    SET(SERVER_APP_SOCKS5_NAME server-socks5)
    # Указываем что нужно собрать приложение работы с SOCKS5 клиентом и сервером
    option(CMAKE_BUILD_SOCKS5_APP_SAMPLE "Build SOCKS5 app sample" ON)

    # Устанавливаем имя приложения работы с юнитом файловой системы
    SET(UNIT_APP_FS_NAME unit-fs)
    # Указываем что нужно собрать приложение работы с юнитом файловой системы
    option(CMAKE_BUILD_UNIT_FS_APP_SAMPLE "Build Unit Filesystem app sample" ON)

    # Устанавливаем имя приложения работы с юнитом резолвером доменных имён
    SET(UNIT_APP_DNS_NAME unit-dns)
    # Указываем что нужно собрать приложение работы с юнитом резолвером доменных имён
    option(CMAKE_BUILD_UNIT_DNS_APP_SAMPLE "Build Unit DNS app sample" ON)

    # Устанавливаем имя приложения работы с юнитом синхронизации времени NTP
    SET(UNIT_APP_NTP_NAME unit-ntp)
    # Указываем что нужно собрать приложение работы с юнитом синхронизации времени NTP
    option(CMAKE_BUILD_UNIT_NTP_APP_SAMPLE "Build Unit NTP app sample" ON)

    # Устанавливаем имя приложения работы с юнитом ICMP-клиентом
    SET(UNIT_APP_ICMP_NAME unit-ping)
    # Указываем что нужно собрать приложение работы с юнитом ICMP-клиентом
    option(CMAKE_BUILD_UNIT_ICMP_APP_SAMPLE "Build Unit Ping app sample" ON)

    # Устанавливаем имя приложения работы с юнитом таймера
    SET(UNIT_APP_TIMER_NAME unit-timer)
    # Указываем что нужно собрать приложение работы с юнитом таймера
    option(CMAKE_BUILD_UNIT_TIMER_APP_SAMPLE "Build Unit Timer app sample" ON)

    # Устанавливаем имя приложения работы с юнитом кластера
    SET(UNIT_APP_CLUSTER_NAME unit-cluster)
    # Указываем что нужно собрать приложение работы с юнитом кластера
    option(CMAKE_BUILD_UNIT_CLUSTER_APP_SAMPLE "Build Unit Cluster app sample" ON)

    # Устанавливаем имя приложения работы с юнитом уведомителя
    SET(UNIT_APP_NOTIFIER_NAME unit-notifier)
    # Указываем что нужно собрать приложение работы с юнитом уведомителя
    option(CMAKE_BUILD_UNIT_NOTIFIER_APP_SAMPLE "Build Unit Notifier app sample" ON)

    # Устанавливаем имя приложения работы с IP-адресами
    SET(IP_APP_NET_NAME net-ip)
    # Указываем что нужно собрать приложение работы с IP-адресами
    option(CMAKE_BUILD_IP_NET_APP_SAMPLE "Build IP app sample" ON)

    # Устанавливаем имя приложения работы с URI
    SET(URI_APP_NET_NAME net-uri)
    # Указываем что нужно собрать приложение работы с URI
    option(CMAKE_BUILD_URI_NET_APP_SAMPLE "Build URI app sample" ON)

    # Устанавливаем имя приложения работы с Ethernet-адресами
    SET(ETH_APP_NET_NAME net-eth)
    # Указываем что нужно собрать приложение работы с Ethernet-адресами
    option(CMAKE_BUILD_ETH_NET_APP_SAMPLE "Build ETH app sample" ON)

    # Устанавливаем имя приложения работы с датой и временем
    SET(DATE_APP_SYS_NAME sys-date)
    # Указываем что нужно собрать приложение работы с датой и временем
    option(CMAKE_BUILD_DATE_SYS_APP_SAMPLE "Build Date app sample" ON)

    # Устанавливаем имя приложения работы с бинарным контейнером
    SET(BINBOX_APP_SYS_NAME sys-binbox)
    # Указываем что нужно собрать приложение работы с бинарным контейнером
    option(CMAKE_BUILD_BINBOX_SYS_APP_SAMPLE "Build BinBox app sample" ON)

    # Устанавливаем имя приложения работы с файловой системой
    SET(FILESYSTEM_APP_SYS_NAME sys-filesystem)
    # Указываем что нужно собрать приложение работы с файловой системой
    option(CMAKE_BUILD_FILESYSTEM_SYS_APP_SAMPLE "Build Filesystem app sample" ON)

    # Устанавливаем имя приложения работы с резольвером процессов
    SET(PROCRE_APP_SYS_NAME sys-procre)
    # Указываем что нужно собрать приложение работы с резольвером процессов
    option(CMAKE_BUILD_PROCRE_SYS_APP_SAMPLE "Build Process Resolver app sample" ON)

    # Устанавливаем имя приложения работы с шифрованием и хэшированием
    SET(CRYPTO_APP_SYS_NAME sys-crypto)
    # Указываем что нужно собрать приложение работы с шифрованием и хэшированием
    option(CMAKE_BUILD_CRYPTO_SYS_APP_SAMPLE "Build Crypto app sample" ON)

    # Устанавливаем имя приложения работы с HTTP-авторизацией
    SET(AUTH_APP_PROTO_NAME proto-auth)
    # Указываем что нужно собрать приложение работы с HTTP-авторизацией
    option(CMAKE_BUILD_AUTH_PROTO_APP_SAMPLE "Build HTTP Auth app sample" ON)

    # Устанавливаем имя приложения работы с HTTP-заголовками
    SET(HEADERS_APP_PROTO_NAME proto-headers)
    # Указываем что нужно собрать приложение работы с HTTP-заголовками
    option(CMAKE_BUILD_HEADERS_PROTO_APP_SAMPLE "Build HTTP Headers app sample" ON)

    # Устанавливаем имя приложения работы с парсером HTTP/1.0 и HTTP/1.1
    SET(PARSER_APP_PROTO_NAME proto-parser)
    # Устанавливаем имя приложения работы с парсером HTTP/2
    SET(PARSER2_APP_PROTO_NAME proto-parser2)
    # Указываем что нужно собрать приложение работы с парсером HTTP/1.0 и HTTP/1.1
    option(CMAKE_BUILD_PARSER_PROTO_APP_SAMPLE "Build HTTP Parser app sample" ON)
    # Указываем что нужно собрать приложение работы с парсером HTTP/2
    option(CMAKE_BUILD_PARSER2_PROTO_APP_SAMPLE "Build HTTP/2 Parser app sample" ON)

    # Устанавливаем имя приложения работы с протоколом SOCKS5
    SET(SOCKS5_APP_PROTO_NAME proto-socks5)
    # Указываем что нужно собрать приложение работы с протоколом SOCKS5
    option(CMAKE_BUILD_SOCKS5_PROTO_APP_SAMPLE "Build SOCKS5 Protocol app sample" ON)

    # Устанавливаем имя клиентского приложения работы с протоколом QUIC
    SET(QUIC_CLIENT_APP_PROTO_NAME proto-quic-client)
    # Устанавливаем имя серверного приложения работы с протоколом QUIC
    SET(QUIC_SERVER_APP_PROTO_NAME proto-quic-server)
    # Указываем что нужно собрать приложения работы с протоколом QUIC
    option(CMAKE_BUILD_QUIC_PROTO_APP_SAMPLE "Build QUIC Protocol app sample" ON)

    # Устанавливаем имя приложения блочной компрессии данных
    SET(COMPRESSOR_APP_BLOCK_NAME compressor-block)
    # Устанавливаем имя приложения потоковой компрессии данных
    SET(COMPRESSOR_APP_STREAM_NAME compressor-stream)
    # Указываем что нужно собрать приложение работы с компрессией данных
    option(CMAKE_BUILD_COMPRESSOR_SYS_APP_SAMPLE "Build Compressor app sample" ON)

    # Устанавливаем имя приложения работы с каталогами
    SET(DIR_APP_IO_NAME io-dir)
    # Указываем что нужно собрать приложение работы с каталогами
    option(CMAKE_BUILD_DIR_IO_APP_SAMPLE "Build Dir IO app sample" ON)

    # Устанавливаем имя приложения работы с файлами
    SET(FILE_APP_IO_NAME io-file)
    # Указываем что нужно собрать приложение работы с файлами
    option(CMAKE_BUILD_FILE_IO_APP_SAMPLE "Build File IO app sample" ON)

    # Устанавливаем имя приложения работы с пингами (ICMP)
    SET(PING_APP_IO_NAME io-ping)
    # Указываем что нужно собрать приложение работы с пингами (ICMP)
    option(CMAKE_BUILD_PING_IO_APP_SAMPLE "Build Ping IO app sample" ON)

    # Устанавливаем имя приложения работы с таймером
    SET(TIMER_APP_IO_NAME io-timer)
    # Указываем что нужно собрать приложение работы с таймером
    option(CMAKE_BUILD_TIMER_IO_APP_SAMPLE "Build Timer IO app sample" ON)

    # Устанавливаем имя приложения работы с пользовательскими событиями
    SET(EVENTS_APP_IO_NAME io-events)
    # Указываем что нужно собрать приложение работы с пользовательскими событиями
    option(CMAKE_BUILD_EVENTS_IO_APP_SAMPLE "Build Events IO app sample" ON)

    # Устанавливаем имя приложения работы с межпроцессорным взаимодействием (именованный канал)
    SET(IPC_PIPE_APP_IO_NAME ipc-pipe)
    # Устанавливаем имя приложения работы с межпроцессорным взаимодействием (поток)
    SET(IPC_STREAM_APP_IO_NAME ipc-stream)
    # Устанавливаем имя приложения работы с межпроцессорным взаимодействием (датаграмма)
    SET(IPC_DATAGRAM_APP_IO_NAME ipc-datagram)
    # Устанавливаем имя приложения работы с межпроцессорным взаимодействием (последовательная упаковка)
    SET(IPC_SEQPACKET_APP_IO_NAME ipc-seqpacket)
    # Указываем что нужно собрать приложение работы с межпроцессорным взаимодействием
    option(CMAKE_BUILD_IPC_IO_APP_SAMPLE "Build IPC IO app sample" ON)

    # Устанавливаем имя клиентского приложения работы с TCP
    SET(CLIENT_APP_NET_TCP_NAME net-tcp-client)
    # Устанавливаем имя серверного приложения работы с TCP
    SET(SERVER_APP_NET_TCP_NAME net-tcp-server)
    # Указываем что нужно собрать приложение работы с TCP (клиент и сервер)
    option(CMAKE_BUILD_NET_TCP_APP_SAMPLE "Build NET TCP app sample" ON)

    # Устанавливаем имя клиентского приложения работы с UDP
    SET(CLIENT_APP_NET_UDP_NAME net-udp-client)
    # Устанавливаем имя клиентского приложения работы с UDP с подключением
    SET(CLIENT_APP_NET_UDP_CONN_NAME net-udp-conn-client)
    # Устанавливаем имя серверного приложения работы с UDP
    SET(SERVER_APP_NET_UDP_NAME net-udp-server)
    # Указываем что нужно собрать приложение работы с UDP (клиент и сервер)
    option(CMAKE_BUILD_NET_UDP_APP_SAMPLE "Build NET UDP app sample" ON)

    # Устанавливаем имя клиентского приложения работы с ограничителем пропускной способности
    SET(CLIENT_APP_NET_TCP_BANDWIDTH_NAME net-bandwidth-tcp-client)
    # Устанавливаем имя серверного приложения работы с ограничителем пропускной способности
    SET(SERVER_APP_NET_TCP_BANDWIDTH_NAME net-bandwidth-tcp-server)
    # Указываем что нужно собрать приложение работы с ограничителем пропускной способности (клиент и сервер)
    option(CMAKE_BUILD_NET_BANDWIDTH_APP_SAMPLE "Build NET Bandwidth app sample" ON)

    # Устанавливаем имя клиентского приложения работы с TUN
    SET(CLIENT_APP_NET_TUN_NAME net-tun-client)
    # Устанавливаем имя серверного приложения работы с TUN
    SET(SERVER_APP_NET_TUN_NAME net-tun-server)
    # Указываем что нужно собрать приложение работы с TUN (клиент и сервер)
    option(CMAKE_BUILD_NET_TUN_APP_SAMPLE "Build NET TUN app sample" ON)

    # Устанавливаем имя клиентского приложения работы с UDP Broadcast
    SET(CLIENT_APP_NET_BROADCAST_NAME net-broadcast-client)
    # Устанавливаем имя серверного приложения работы с UDP Broadcast
    SET(SERVER_APP_NET_BROADCAST_NAME net-broadcast-server)
    # Указываем что нужно собрать приложение работы с UDP Broadcast (клиент и сервер)
    option(CMAKE_BUILD_NET_BROADCAST_APP_SAMPLE "Build NET Broadcast app sample" ON)

    # Устанавливаем имя клиентского приложения работы с UDP Multicast сценарий 1
    SET(CLIENT_APP_NET_MULTICAST1_NAME net-multicast1-client)
    # Устанавливаем имя серверного приложения работы с UDP Multicast сценарий 1
    SET(SERVER_APP_NET_MULTICAST1_NAME net-multicast1-server)
    # Устанавливаем имя клиентского приложения работы с UDP Multicast сценарий 3
    SET(CLIENT_APP_NET_MULTICAST3_NAME net-multicast3-client)
    # Устанавливаем имя серверного приложения работы с UDP Multicast сценарий 3
    SET(SERVER_APP_NET_MULTICAST3_NAME net-multicast3-server)
    # Указываем что нужно собрать приложение работы с UDP Multicast (клиент и сервер)
    option(CMAKE_BUILD_NET_MULTICAST_APP_SAMPLE "Build NET Multicast app sample" ON)

    # Устанавливаем имя клиентского приложения работы с UDP Splice
    SET(CLIENT_APP_NET_SPLICE_NAME net-splice-client)
    # Устанавливаем имя серверного приложения работы с UDP Splice
    SET(SERVER_APP_NET_SPLICE_NAME net-splice-server)
    # Указываем что нужно собрать приложение работы с UDP Splice (клиент и сервер)
    option(CMAKE_BUILD_NET_SPLICE_APP_SAMPLE "Build NET Splice app sample" ON)

    # Устанавливаем имя клиентского приложения работы с UDS-TCP
    SET(CLIENT_APP_NET_UDS_TCP_NAME net-uds-tcp-client)
    # Устанавливаем имя серверного приложения работы с UDS-TCP
    SET(SERVER_APP_NET_UDS_TCP_NAME net-uds-tcp-server)
    # Устанавливаем имя клиентского приложения работы с UDS-UDP
    SET(CLIENT_APP_NET_UDS_UDP_NAME net-uds-udp-client)
    # Устанавливаем имя серверного приложения работы с UDS-UDP
    SET(SERVER_APP_NET_UDS_UDP_NAME net-uds-udp-server)
    # Указываем что нужно собрать приложение работы с UDS (клиент и сервер)
    option(CMAKE_BUILD_NET_UDS_APP_SAMPLE "Build NET UDS app sample" ON)

    # Устанавливаем имя клиентского приложения работы с DTLS
    SET(CLIENT_APP_NET_DTLS_NAME net-dtls-client)
    # Устанавливаем имя серверного приложения работы с DTLS
    SET(SERVER_APP_NET_DTLS_NAME net-dtls-server)
    # Указываем что нужно собрать приложение работы с DTLS (клиент и сервер)
    option(CMAKE_BUILD_NET_DTLS_APP_SAMPLE "Build NET DTLS app sample" ON)

    # Устанавливаем имя клиентского приложения работы с TLS
    SET(CLIENT_APP_NET_TLS_NAME net-tls-client)
     # Устанавливаем имя серверного приложения работы с TLS
    SET(SERVER_APP_NET_TLS_NAME net-tls-server)
    # Устанавливаем имя клиентского приложения работы с TLS и HTTP/1.1 подключением
    SET(CLIENT_APP_NET_TLS_WEB_NAME net-tls-web-client)
    # Устанавливаем имя серверного приложения работы с мультисертификатным TLS
    SET(SERVER_APP_NET_TLS_MULTI_NAME net-tls-multi-server)
    # Указываем что нужно собрать приложение работы с TLS (клиент и сервер)
    option(CMAKE_BUILD_NET_TLS_APP_SAMPLE "Build NET TLS app sample" ON)

    # Если сборка производится под операционную систему FreeBSD или Linux
    if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        # Устанавливаем имя клиентского приложения работы с SCTP - TLS
        SET(CLIENT_APP_NET_SCTP_TLS_NAME net-sctp-tls-client)
        # Устанавливаем имя серверного приложения работы с SCTP - TLS
        SET(SERVER_APP_NET_SCTP_TLS_NAME net-sctp-tls-server)
        # Устанавливаем имя клиентского приложения работы с SCTP - DTLS
        SET(CLIENT_APP_NET_SCTP_DTLS_NAME net-sctp-dtls-client)
        # Устанавливаем имя серверного приложения работы с SCTP - DTLS
        SET(SERVER_APP_NET_SCTP_DTLS_NAME net-sctp-dtls-server)
        # Устанавливаем имя клиентского приложения работы с SCTP - аутентификация
        SET(CLIENT_APP_NET_SCTP_AUTH_NAME net-sctp-auth-client)
        # Устанавливаем имя серверного приложения работы с SCTP - аутентификация
        SET(SERVER_APP_NET_SCTP_AUTH_NAME net-sctp-auth-server)
        # Устанавливаем имя клиентского приложения работы с SCTP - потоковый режим
        SET(CLIENT_APP_NET_SCTP_STREAM_NAME net-sctp-stream-client)
        # Устанавливаем имя серверного приложения работы с SCTP - потоковый режим
        SET(SERVER_APP_NET_SCTP_STREAM_NAME net-sctp-stream-server)
        # Устанавливаем имя клиентского приложения работы с SCTP - пакетный режим
        SET(CLIENT_APP_NET_SCTP_SEQPACKET_NAME net-sctp-seqpacket-client)
        # Устанавливаем имя серверного приложения работы с SCTP - пакетный режим
        SET(SERVER_APP_NET_SCTP_SEQPACKET_NAME net-sctp-seqpacket-server)
        # Указываем что нужно собрать приложение работы с SCTP (клиент и сервер)
        option(CMAKE_BUILD_NET_SCTP_APP_SAMPLE "Build NET SCTP app sample" ON)
    endif()
endif (${CMAKE_BUILD_SAMPLES})

# Обнуляем флаги для сборки семплов
SET(AWH_FLAGS "")

# Если сборка производится под операционную систему Windows
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    # export MSYS="error_start:%WD%dumper.exe -d -c %1 %2"
    # Устанавливаем необходимые зависимости для MS Windows
    SET(AWH_FLAGS -lstdc++ -lws2_32 -lgdi32 -lcrypt32 -lIphlpapi -lbcrypt -flto=auto)

    # Устанавливаем иконку для windows под MinGW.
    SET(RES_FILES ${CMAKE_PROJECT_NAME}.rc)
    SET(CMAKE_RC_COMPILER_INIT windres)
    ENABLE_LANGUAGE(RC)
    SET(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")

    # Если необходимо собрать семплы
    if (${CMAKE_BUILD_SAMPLES})
        ##
         # Функция создания семплов
         #
         # Аргументы:
         #   name - имя исполняемого файла
         #   src  - исходный файл
         #   dir  - каталог для исполняемого файла
         ##
        function(add_sample name src dir)
            # Выполняем создание приложения работы с IP-адресами
            add_executable(${name} ${src} ${RES_FILES})
            # Указываем каталог для исполняемого файла
            set_target_properties(${name} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dir}
            )
        endfunction()
    endif (${CMAKE_BUILD_SAMPLES})
# Если сборка производится под операционные системы Linux, FreeBSD, NetBSD, OpenBSD, macOS или Solaris
else()
    # Если сборка производится под операционную систему macOS
    if (APPLE AND NOT IOS)
        # Устанавливаем необходимые зависимости для macOS
        SET(AWH_FLAGS "-framework Security -framework CoreFoundation -framework Foundation")
    endif()

    # Если необходимо собрать семплы
    if (${CMAKE_BUILD_SAMPLES})
        ##
         # Функция создания семплов
         #
         # Аргументы:
         #   name - имя исполняемого файла
         #   src  - исходный файл
         #   dir  - каталог для исполняемого файла
         ##
        function(add_sample name src dir)
            # Выполняем создание приложения работы с IP-адресами
            add_executable(${name} ${src})
            # Указываем каталог для исполняемого файла
            set_target_properties(${name} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dir}
            )
        endfunction()
    endif (${CMAKE_BUILD_SAMPLES})
endif()

# Если необходимо собрать семплы
if (${CMAKE_BUILD_SAMPLES})
    # Если нужно собрать семплы для работы с клиентом
    if (${CMAKE_BUILD_CLIENT_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample CLIENT: Yes\n")
        # Выполняем создание приложения работы с клиентом UDS
        add_sample(${CLIENT_APP_UDS_NAME} sample/client/uds.cpp client)
        # Выполняем создание приложения работы с клиентом UDS версии 2
        add_sample(${CLIENT_APP_UDS-2_NAME} sample/client/uds2.cpp client)
        # Выполняем создание приложения работы с клиентом TCP
        add_sample(${CLIENT_APP_TCP_NAME} sample/client/tcp.cpp client)
        # Выполняем создание приложения работы с клиентом TCP версии 2
        add_sample(${CLIENT_APP_TCP-2_NAME} sample/client/tcp2.cpp client)
        # Выполняем создание приложения работы с клиентом UDP
        add_sample(${CLIENT_APP_UDP_NAME} sample/client/udp.cpp client)
        # Выполняем создание приложения работы с клиентом UDP версии 2
        add_sample(${CLIENT_APP_UDP-2_NAME} sample/client/udp2.cpp client)
        # Выполняем создание приложения работы с клиентом TLS
        add_sample(${CLIENT_APP_TLS_NAME} sample/client/tls.cpp client)
        # Выполняем создание приложения работы с клиентом TLS версии 2
        add_sample(${CLIENT_APP_TLS-2_NAME} sample/client/tls2.cpp client)
        # Выполняем создание приложения работы с клиентом DTLS
        add_sample(${CLIENT_APP_DTLS_NAME} sample/client/dtls.cpp client)
        # Выполняем создание приложения работы с клиентом DTLS версии 2
        add_sample(${CLIENT_APP_DTLS-2_NAME} sample/client/dtls2.cpp client)
        # Выполняем создание приложения работы с клиентом TLS-Web
        add_sample(${CLIENT_APP_TLS-WEB_NAME} sample/client/tls-web.cpp client)
        # Если сборка производится под операционную систему FreeBSD или Linux
        if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
            # Выполняем создание приложения работы с клиентом SCTP
            add_sample(${CLIENT_APP_SCTP_NAME} sample/client/sctp.cpp client)
            # Выполняем создание приложения работы с клиентом SCTP версии 2
            add_sample(${CLIENT_APP_SCTP-2_NAME} sample/client/sctp2.cpp client)
        endif()
    # Если не нужно собирать семплы для работы с клиентом
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample CLIENT: No\n")
    endif (${CMAKE_BUILD_CLIENT_APP_SAMPLE})

    # Если нужно собрать семплы для работы с сервером
    if (${CMAKE_BUILD_SERVER_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample SERVER: Yes\n")
        # Выполняем создание приложения работы с сервером UDS
        add_sample(${SERVER_APP_UDS_NAME} sample/server/uds.cpp server)
        # Выполняем создание приложения работы с сервером UDS версии 2
        add_sample(${SERVER_APP_UDS-2_NAME} sample/server/uds2.cpp server)
        # Выполняем создание приложения работы с сервером TCP
        add_sample(${SERVER_APP_TCP_NAME} sample/server/tcp.cpp server)
        # Выполняем создание приложения работы с сервером TCP версии 2
        add_sample(${SERVER_APP_TCP-2_NAME} sample/server/tcp2.cpp server)
        # Выполняем создание приложения работы с сервером UDP
        add_sample(${SERVER_APP_UDP_NAME} sample/server/udp.cpp server)
        # Выполняем создание приложения работы с сервером UDP версии 2
        add_sample(${SERVER_APP_UDP-2_NAME} sample/server/udp2.cpp server)
        # Выполняем создание приложения работы с сервером TLS
        add_sample(${SERVER_APP_TLS_NAME} sample/server/tls.cpp server)
        # Выполняем создание приложения работы с сервером TLS мультисертификатным
        add_sample(${SERVER_APP_TLS-MULTICERT_NAME} sample/server/tls-multicert.cpp server)
        # Выполняем создание приложения работы с сервером DTLS
        add_sample(${SERVER_APP_DTLS_NAME} sample/server/dtls.cpp server)
        # Выполняем создание приложения работы с сервером DTLS мультисертификатным
        add_sample(${SERVER_APP_DTLS-MULTICERT_NAME} sample/server/dtls-multicert.cpp server)
        # Если сборка производится под операционную систему FreeBSD или Linux
        if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
            # Выполняем создание приложения работы с сервером SCTP
            add_sample(${SERVER_APP_SCTP_NAME} sample/server/sctp.cpp server)
            # Выполняем создание приложения работы с сервером SCTP версии 2
            add_sample(${SERVER_APP_SCTP-2_NAME} sample/server/sctp2.cpp server)
        endif()
    # Если не нужно собирать семплы для работы с сервером
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample SERVER: No\n")
    endif (${CMAKE_BUILD_SERVER_APP_SAMPLE})

    # Если нужно собрать семплы для работы с юнитом VPN
    if (${CMAKE_BUILD_VPN_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample VPN: Yes\n")
        # Выполняем создание приложения работы с VPN клиентом
        add_sample(${CLIENT_APP_VPN_NAME} sample/client/vpn.cpp client)
        # Выполняем создание приложения работы с VPN сервером
        add_sample(${SERVER_APP_VPN_NAME} sample/server/vpn.cpp server)
    # Если не нужно собирать семплы для работы с VPN
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample VPN: No\n")
    endif (${CMAKE_BUILD_VPN_APP_SAMPLE})

    # Если нужно собрать семплы для работы с юнитом SOCKS5
    if (${CMAKE_BUILD_SOCKS5_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample SOCKS5: Yes\n")
        # Выполняем создание приложения работы с SOCKS5 клиентом
        add_sample(${CLIENT_APP_SOCKS5_NAME} sample/client/socks5.cpp client)
        # Выполняем создание приложения работы с SOCKS5 клиентом для UDP
        add_sample(${CLIENT_APP_SOCKS5-UDP_NAME} sample/client/socks5-udp.cpp client)
        # Выполняем создание приложения работы с SOCKS5 клиентом для DTLS
        add_sample(${CLIENT_APP_SOCKS5-DTLS_NAME} sample/client/socks5-dtls.cpp client)
        # Выполняем создание приложения работы с SOCKS5 сервером
        add_sample(${SERVER_APP_SOCKS5_NAME} sample/server/socks5.cpp server)
    # Если не нужно собирать семплы для работы с SOCKS5
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample SOCKS5: No\n")
    endif (${CMAKE_BUILD_SOCKS5_APP_SAMPLE})

    # Если нужно собрать семплы для работы с юнитом файловой системы
    if (${CMAKE_BUILD_UNIT_FS_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UNIT Filesystem: Yes\n")
        # Выполняем создание приложения работы с юнитом файловой системы
        add_sample(${UNIT_APP_FS_NAME} sample/units/fs.cpp units)
    # Если не нужно собирать семплы для работы с юнитом файловой системы
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UNIT Filesystem: No\n")
    endif (${CMAKE_BUILD_UNIT_FS_APP_SAMPLE})

    # Если нужно собрать семплы для работы с юнитом резолвером доменных имён
    if (${CMAKE_BUILD_UNIT_DNS_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UNIT DNS: Yes\n")
        # Выполняем создание приложения работы с юнитом резолвером доменных имён
        add_sample(${UNIT_APP_DNS_NAME} sample/units/dns.cpp units)
    # Если не нужно собирать семплы для работы с юнитом резолвером доменных имён
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UNIT DNS: No\n")
    endif (${CMAKE_BUILD_UNIT_DNS_APP_SAMPLE})

    # Если нужно собрать семплы для работы с юнитом синхронизации времени NTP
    if (${CMAKE_BUILD_UNIT_NTP_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UNIT NTP: Yes\n")
        # Выполняем создание приложения работы с юнитом синхронизации времени NTP
        add_sample(${UNIT_APP_NTP_NAME} sample/units/ntp.cpp units)
    # Если не нужно собирать семплы для работы с юнитом синхронизации времени NTP
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UNIT NTP: No\n")
    endif (${CMAKE_BUILD_UNIT_NTP_APP_SAMPLE})

    # Если нужно собрать семплы для работы с юнитом ICMP-клиентом
    if (${CMAKE_BUILD_UNIT_ICMP_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UNIT ICMP: Yes\n")
        # Выполняем создание приложения работы с юнитом ICMP-клиентом
        add_sample(${UNIT_APP_ICMP_NAME} sample/units/ping.cpp units)
    # Если не нужно собирать семплы для работы с юнитом ICMP-клиентом
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UNIT ICMP: No\n")
    endif (${CMAKE_BUILD_UNIT_ICMP_APP_SAMPLE})

    # Если нужно собрать семплы для работы с юнитом таймера
    if (${CMAKE_BUILD_UNIT_TIMER_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UNIT Timer: Yes\n")
        # Выполняем создание приложения работы с юнитом таймера
        add_sample(${UNIT_APP_TIMER_NAME} sample/units/timer.cpp units)
    # Если не нужно собирать семплы для работы с юнитом таймера
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UNIT Timer: No\n")
    endif (${CMAKE_BUILD_UNIT_TIMER_APP_SAMPLE})

    # Если нужно собрать семплы для работы с юнитом кластера
    if (${CMAKE_BUILD_UNIT_CLUSTER_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UNIT Cluster: Yes\n")
        # Выполняем создание приложения работы с юнитом кластера
        add_sample(${UNIT_APP_CLUSTER_NAME} sample/units/cluster.cpp units)
    # Если не нужно собирать семплы для работы с юнитом кластера
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UNIT Cluster: No\n")
    endif (${CMAKE_BUILD_UNIT_CLUSTER_APP_SAMPLE})

    # Если нужно собрать семплы для работы с юнитом уведомителя
    if (${CMAKE_BUILD_UNIT_NOTIFIER_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UNIT Notifier: Yes\n")
        # Выполняем создание приложения работы с юнитом уведомителя
        add_sample(${UNIT_APP_NOTIFIER_NAME} sample/units/notifier.cpp units)
    # Если не нужно собирать семплы для работы с юнитом уведомителя
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UNIT Notifier: No\n")
    endif (${CMAKE_BUILD_UNIT_NOTIFIER_APP_SAMPLE})

    # Если нужно собрать семплы для работы с датой и временем
    if (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample DATE: Yes\n")
        # Выполняем создание приложения работы с датой и временем
        add_sample(${DATE_APP_SYS_NAME} sample/sys/date.cpp sys)
    # Если не нужно собирать семплы для работы с датой и временем
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample DATE: No\n")
    endif (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})

    # Если нужно собрать семплы для работы с бинарным контейнером
    if (${CMAKE_BUILD_BINBOX_SYS_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample BinBox: Yes\n")
        # Выполняем создание приложения работы с бинарным контейнером
        add_sample(${BINBOX_APP_SYS_NAME} sample/sys/binbox.cpp sys)
    # Если не нужно собирать семплы для работы с бинарным контейнером
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample BinBox: No\n")
    endif (${CMAKE_BUILD_BINBOX_SYS_APP_SAMPLE})

    # Если нужно собрать семплы для работы с файловой системой
    if (${CMAKE_BUILD_FILESYSTEM_SYS_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample Filesystem: Yes\n")
        # Выполняем создание приложения работы с файловой системой
        add_sample(${FILESYSTEM_APP_SYS_NAME} sample/sys/fs.cpp sys)
    # Если не нужно собирать семплы для работы с файловой системой
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample Filesystem: No\n")
    endif (${CMAKE_BUILD_FILESYSTEM_SYS_APP_SAMPLE})

    # Если нужно собрать семплы для работы с резольвером процессов
    if (${CMAKE_BUILD_PROCRE_SYS_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample Process Resolver: Yes\n")
        # Выполняем создание приложения работы с резольвером процессов
        add_sample(${PROCRE_APP_SYS_NAME} sample/sys/procre.cpp sys)
    # Если не нужно собирать семплы для работы с резольвером процессов
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample Process Resolver: No\n")
    endif (${CMAKE_BUILD_PROCRE_SYS_APP_SAMPLE})

    # Если нужно собрать семплы для работы с шифрованием и хэшированием
    if (${CMAKE_BUILD_CRYPTO_SYS_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample Crypto: Yes\n")
        # Выполняем создание приложения работы с шифрованием и хэшированием
        add_sample(${CRYPTO_APP_SYS_NAME} sample/sys/crypto.cpp sys)
    # Если не нужно собирать семплы для работы с шифрованием и хэшированием
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample Crypto: No\n")
    endif (${CMAKE_BUILD_CRYPTO_SYS_APP_SAMPLE})

    # Если нужно собрать семплы для работы с HTTP-авторизацией
    if (${CMAKE_BUILD_AUTH_PROTO_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample HTTP Auth: Yes\n")
        # Выполняем создание приложения работы с HTTP-авторизацией
        add_sample(${AUTH_APP_PROTO_NAME} sample/proto/http/auth.cpp proto)
    # Если не нужно собирать семплы для работы с HTTP-авторизацией
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample HTTP Auth: No\n")
    endif (${CMAKE_BUILD_AUTH_PROTO_APP_SAMPLE})

    # Если нужно собрать семплы для работы с HTTP-заголовками
    if (${CMAKE_BUILD_HEADERS_PROTO_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample HTTP Headers: Yes\n")
        # Выполняем создание приложения работы с HTTP-заголовками
        add_sample(${HEADERS_APP_PROTO_NAME} sample/proto/http/headers.cpp proto)
    # Если не нужно собирать семплы для работы с HTTP-заголовками
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample HTTP Headers: No\n")
    endif (${CMAKE_BUILD_HEADERS_PROTO_APP_SAMPLE})

    # Если нужно собрать семплы для работы с парсером HTTP/1.0 и HTTP/1.1
    if (${CMAKE_BUILD_PARSER_PROTO_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample HTTP Parser: Yes\n")
        # Выполняем создание приложения работы с парсером HTTP/1.0 и HTTP/1.1
        add_sample(${PARSER_APP_PROTO_NAME} sample/proto/http/parser.cpp proto)
    # Если не нужно собирать семплы для работы с парсером HTTP/1.0 и HTTP/1.1
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample HTTP Parser: No\n")
    endif (${CMAKE_BUILD_PARSER_PROTO_APP_SAMPLE})

    # Если нужно собрать семплы для работы с парсером HTTP/2
    if (${CMAKE_BUILD_PARSER2_PROTO_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample HTTP/2 Parser: Yes\n")
        # Выполняем создание приложения работы с парсером HTTP/2
        add_sample(${PARSER2_APP_PROTO_NAME} sample/proto/http/parser2.cpp proto)
    # Если не нужно собирать семплы для работы с парсером HTTP/2
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample HTTP/2 Parser: No\n")
    endif (${CMAKE_BUILD_PARSER2_PROTO_APP_SAMPLE})

    # Если нужно собрать семплы для работы с протоколом SOCKS5
    if (${CMAKE_BUILD_SOCKS5_PROTO_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample SOCKS5 Protocol: Yes\n")
        # Выполняем создание приложения работы с протоколом SOCKS5
        add_sample(${SOCKS5_APP_PROTO_NAME} sample/proto/socks5/socks5.cpp proto)
    # Если не нужно собирать семплы для работы с протоколом SOCKS5
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample SOCKS5 Protocol: No\n")
    endif (${CMAKE_BUILD_SOCKS5_PROTO_APP_SAMPLE})

    # Если нужно собрать семплы для работы с протоколом QUIC
    if (${CMAKE_BUILD_QUIC_PROTO_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample QUIC Protocol: Yes\n")
        # Выполняем создание приложения работы с QUIC клиентом
        add_sample(${QUIC_CLIENT_APP_PROTO_NAME} sample/proto/quic/client.cpp proto/quic)
        # Выполняем создание приложения работы с QUIC сервером
        add_sample(${QUIC_SERVER_APP_PROTO_NAME} sample/proto/quic/server.cpp proto/quic)
    # Если не нужно собирать семплы для работы с протоколом QUIC
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample QUIC Protocol: No\n")
    endif (${CMAKE_BUILD_QUIC_PROTO_APP_SAMPLE})

    # Если нужно собрать семплы для работы с компрессией данных
    if (${CMAKE_BUILD_COMPRESSOR_SYS_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample Compressor: Yes\n")
        # Выполняем создание приложения блочной компрессии данных
        add_sample(${COMPRESSOR_APP_BLOCK_NAME} sample/compressor/block.cpp compressor)
        # Выполняем создание приложения потоковой компрессии данных
        add_sample(${COMPRESSOR_APP_STREAM_NAME} sample/compressor/stream.cpp compressor)
    # Если не нужно собирать семплы для работы с компрессией данных
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample Compressor: No\n")
    endif (${CMAKE_BUILD_COMPRESSOR_SYS_APP_SAMPLE})

    # Если нужно собрать семплы для работы с каталогами
    if (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample DIR (File System): Yes\n")
        # Выполняем создание приложения работы с каталогами
        add_sample(${DIR_APP_IO_NAME} sample/io/dir.cpp io)
    # Если не нужно собирать семплы для работы с каталогами
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample DIR (File System): No\n")
    endif (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})

    # Если нужно собрать семплы для работы с файлами
    if (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample FILE (File System): Yes\n")
        # Выполняем создание приложения работы с файлами
        add_sample(${FILE_APP_IO_NAME} sample/io/file.cpp io)
    # Если не нужно собирать семплы для работы с файлами
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample FILE (File System): No\n")
    endif (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})

    # Если нужно собрать семплы для работы с пингами (ICMP)
    if (${CMAKE_BUILD_PING_IO_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample PING (ICMP): Yes\n")
        # Выполняем создание приложения работы с пингами (ICMP)
        add_sample(${PING_APP_IO_NAME} sample/io/ping.cpp io)
    # Если не нужно собирать семплы для работы с пингами (ICMP)
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample PING (ICMP): No\n")
    endif (${CMAKE_BUILD_PING_IO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с таймером
    if (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample TIMER: Yes\n")
        # Выполняем создание приложения работы с таймером
        add_sample(${TIMER_APP_IO_NAME} sample/io/timer.cpp io)
    # Если не нужно собирать семплы для работы с таймером
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample TIMER: No\n")
    endif (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})

    # Если нужно собрать семплы для работы с пользовательскими событиями
    if (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample USER Events: Yes\n")
        # Выполняем создание приложения работы с пользовательскими событиями
        add_sample(${EVENTS_APP_IO_NAME} sample/io/events.cpp io)
    # Если не нужно собирать семплы для работы с пользовательскими событиями
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample USER Events: No\n")
    endif (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})
    
    # Если нужно собрать семплы для работы с межпроцессорным взаимодействием
    if (${CMAKE_BUILD_IPC_IO_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample IPC: Yes\n")
        # Выполняем создание приложения работы с межпроцессорным взаимодействием (именованный канал)
        add_sample(${IPC_PIPE_APP_IO_NAME} sample/ipc/pipe.cpp ipc)
        # Выполняем создание приложения работы с межпроцессорным взаимодействием (поток)
        add_sample(${IPC_STREAM_APP_IO_NAME} sample/ipc/stream.cpp ipc)
        # Выполняем создание приложения работы с межпроцессорным взаимодействием (датаграмма)
        add_sample(${IPC_DATAGRAM_APP_IO_NAME} sample/ipc/datagram.cpp ipc)
        # Выполняем создание приложения работы с межпроцессорным взаимодействием (последовательная упаковка)
        add_sample(${IPC_SEQPACKET_APP_IO_NAME} sample/ipc/seqpacket.cpp ipc)
    # Если не нужно собирать семплы для работы с межпроцессорным взаимодействием
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample IPC: No\n")
    endif (${CMAKE_BUILD_IPC_IO_APP_SAMPLE})

    # Если нужно собрать семплы для работы с IP-адресами
    if (${CMAKE_BUILD_IP_NET_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample IP network: Yes\n")
        # Выполняем создание приложения работы с IP-адресами
        add_sample(${IP_APP_NET_NAME} sample/net/ip.cpp net)
    # Если не нужно собирать семплы для работы с IP-адресами
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample IP network: No\n")
    endif (${CMAKE_BUILD_IP_NET_APP_SAMPLE})

    # Если нужно собрать семплы для работы с URI
    if (${CMAKE_BUILD_URI_NET_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample URI network: Yes\n")
        # Выполняем создание приложения работы с URI
        add_sample(${URI_APP_NET_NAME} sample/net/uri.cpp net)
    # Если не нужно собирать семплы для работы с URI
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample URI network: No\n")
    endif (${CMAKE_BUILD_URI_NET_APP_SAMPLE})

    # Если нужно собрать семплы для работы с Ethernet-адресами
    if (${CMAKE_BUILD_ETH_NET_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample Ethernet network: Yes\n")
        # Выполняем создание приложения работы с Ethernet-адресами
        add_sample(${ETH_APP_NET_NAME} sample/net/eth.cpp net)
    # Если не нужно собирать семплы для работы с Ethernet-адресами
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample Ethernet network: No\n")
    endif (${CMAKE_BUILD_ETH_NET_APP_SAMPLE})

    # Если нужно собрать семплы для работы с TCP (клиент и сервер)
    if (${CMAKE_BUILD_NET_TCP_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample TCP Client/Server: Yes\n")
        # Выполняем создание приложения работы с TCP клиент
        add_sample(${CLIENT_APP_NET_TCP_NAME} sample/net/tcp/client.cpp net/tcp)
        # Выполняем создание приложения работы с TCP сервер
        add_sample(${SERVER_APP_NET_TCP_NAME} sample/net/tcp/server.cpp net/tcp)
    # Если не нужно собирать семплы для работы с TCP (клиент и сервер)
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample TCP Client/Server: No\n")
    endif (${CMAKE_BUILD_NET_TCP_APP_SAMPLE})

    # Если нужно собрать семплы для работы с UDP (клиент и сервер)
    if (${CMAKE_BUILD_NET_UDP_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UDP Client/Server: Yes\n")
        # Выполняем создание приложения работы с UDP клиент
        add_sample(${CLIENT_APP_NET_UDP_NAME} sample/net/udp/client.cpp net/udp)
        # Выполняем создание приложения работы с UDP с подключением клиент
        add_sample(${CLIENT_APP_NET_UDP_CONN_NAME} sample/net/udp/client-conn.cpp net/udp)
        # Выполняем создание приложения работы с UDP сервер
        add_sample(${SERVER_APP_NET_UDP_NAME} sample/net/udp/server.cpp net/udp)
    # Если не нужно собирать семплы для работы с UDP (клиент и сервер)
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UDP Client/Server: No\n")
    endif (${CMAKE_BUILD_NET_UDP_APP_SAMPLE})

    # Если нужно собрать семплы для работы с ограничителем пропускной способности (клиент и сервер)
    if (${CMAKE_BUILD_NET_BANDWIDTH_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample Bandwidth Client/Server: Yes\n")
        # Выполняем создание приложения работы с ограничителем пропускной способности клиента
        add_sample(${CLIENT_APP_NET_TCP_BANDWIDTH_NAME} sample/net/bandwidth/client.cpp net/bandwidth)
        # Выполняем создание приложения работы с ограничителем пропускной способности сервера
        add_sample(${SERVER_APP_NET_TCP_BANDWIDTH_NAME} sample/net/bandwidth/server.cpp net/bandwidth)
    # Если не нужно собирать семплы для работы с ограничителем пропускной способности (клиент и сервер)
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample Bandwidth Client/Server: No\n")
    endif (${CMAKE_BUILD_NET_BANDWIDTH_APP_SAMPLE})

    # Если нужно собрать семплы для работы с TUN (клиент и сервер)
    if (${CMAKE_BUILD_NET_TUN_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample TUN Client/Server: Yes\n")
        # Выполняем создание приложения работы с TUN клиент
        add_sample(${CLIENT_APP_NET_TUN_NAME} sample/net/tun/client.cpp net/tun)
        # Выполняем создание приложения работы с TUN сервер
        add_sample(${SERVER_APP_NET_TUN_NAME} sample/net/tun/server.cpp net/tun)
    # Если не нужно собирать семплы для работы с TUN (клиент и сервер)
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample TUN Client/Server: No\n")
    endif (${CMAKE_BUILD_NET_TUN_APP_SAMPLE})

    # Если нужно собрать семплы для работы с UDP Broadcast (клиент и сервер)
    if (${CMAKE_BUILD_NET_BROADCAST_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UDP Broadcast Client/Server: Yes\n")
        # Выполняем создание приложения работы с UDP Broadcast клиент
        add_sample(${CLIENT_APP_NET_BROADCAST_NAME} sample/net/broadcast/client.cpp net/broadcast)
        # Выполняем создание приложения работы с UDP Broadcast сервер
        add_sample(${SERVER_APP_NET_BROADCAST_NAME} sample/net/broadcast/server.cpp net/broadcast)
    # Если не нужно собирать семплы для работы с UDP Broadcast (клиент и сервер)
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UDP Broadcast Client/Server: No\n")
    endif (${CMAKE_BUILD_NET_BROADCAST_APP_SAMPLE})

    # Если нужно собрать семплы для работы с UDP Multicast (клиент и сервер)
    if (${CMAKE_BUILD_NET_MULTICAST_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UDP Multicast Client/Server: Yes\n")
        # Выполняем создание приложения работы с UDP Multicast клиент сценарий 1
        add_sample(${CLIENT_APP_NET_MULTICAST1_NAME} sample/net/multicast/client1.cpp net/multicast)
        # Выполняем создание приложения работы с UDP Multicast сервер сценарий 1
        add_sample(${SERVER_APP_NET_MULTICAST1_NAME} sample/net/multicast/server1.cpp net/multicast)
        # Выполняем создание приложения работы с UDP Multicast клиент сценарий 3
        add_sample(${CLIENT_APP_NET_MULTICAST3_NAME} sample/net/multicast/client3.cpp net/multicast)
        # Выполняем создание приложения работы с UDP Multicast сервер сценарий 3
        add_sample(${SERVER_APP_NET_MULTICAST3_NAME} sample/net/multicast/server3.cpp net/multicast)
    # Если не нужно собирать семплы для работы с UDP Multicast (клиент и сервер)
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UDP Multicast Client/Server: No\n")
    endif (${CMAKE_BUILD_NET_MULTICAST_APP_SAMPLE})

    # Если нужно собрать семплы для работы с UDP Splice (клиент и сервер)
    if (${CMAKE_BUILD_NET_SPLICE_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UDP Splice Client/Server: Yes\n")
        # Выполняем создание приложения работы с UDP Splice клиент
        add_sample(${CLIENT_APP_NET_SPLICE_NAME} sample/net/splice/client.cpp net/splice)
        # Выполняем создание приложения работы с UDP Splice сервер
        add_sample(${SERVER_APP_NET_SPLICE_NAME} sample/net/splice/server.cpp net/splice)
    # Если не нужно собирать семплы для работы с UDP Splice (клиент и сервер)
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UDP Splice Client/Server: No\n")
    endif (${CMAKE_BUILD_NET_SPLICE_APP_SAMPLE})

    # Если нужно собрать семплы для работы с UDS (клиент и сервер)
    if (${CMAKE_BUILD_NET_UDS_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UDS TCP/UDP Client/Server: Yes\n")
        # Выполняем создание приложения работы с UDS-TCP клиент
        add_sample(${CLIENT_APP_NET_UDS_TCP_NAME} sample/net/uds/client-tcp.cpp net/uds)
        # Выполняем создание приложения работы с UDS-TCP сервер
        add_sample(${SERVER_APP_NET_UDS_TCP_NAME} sample/net/uds/server-tcp.cpp net/uds)
        # Выполняем создание приложения работы с UDS-UDP клиент
        add_sample(${CLIENT_APP_NET_UDS_UDP_NAME} sample/net/uds/client-udp.cpp net/uds)
        # Выполняем создание приложения работы с UDS-UDP сервер
        add_sample(${SERVER_APP_NET_UDS_UDP_NAME} sample/net/uds/server-udp.cpp net/uds)
    # Если не нужно собирать семплы для работы с UDS (клиент и сервер)
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UDS TCP/UDP Client/Server: No\n")
    endif (${CMAKE_BUILD_NET_UDS_APP_SAMPLE})

    # Если нужно собрать семплы для работы с DTLS (клиент и сервер)
    if (${CMAKE_BUILD_NET_DTLS_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample DTLS Client/Server: Yes\n")
        # Выполняем создание приложения работы с DTLS клиент
        add_sample(${CLIENT_APP_NET_DTLS_NAME} sample/net/dtls/client.cpp net/dtls)
        # Выполняем создание приложения работы с DTLS сервер
        add_sample(${SERVER_APP_NET_DTLS_NAME} sample/net/dtls/server.cpp net/dtls)
    # Если не нужно собирать семплы для работы с DTLS (клиент и сервер)
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample DTLS Client/Server: No\n")
    endif (${CMAKE_BUILD_NET_DTLS_APP_SAMPLE})

    # Если нужно собрать семплы для работы с TLS (клиент и сервер)
    if (${CMAKE_BUILD_NET_TLS_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample TLS Client/Server: Yes\n")
        # Выполняем создание приложения работы с TLS клиент
        add_sample(${CLIENT_APP_NET_TLS_NAME} sample/net/tls/client.cpp net/tls)
        # Выполняем создание приложения работы с TLS сервер
        add_sample(${SERVER_APP_NET_TLS_NAME} sample/net/tls/server.cpp net/tls)
        # Выполняем создание приложения работы с TLS клиент и HTTP/1.1 подключением
        add_sample(${CLIENT_APP_NET_TLS_WEB_NAME} sample/net/tls/client-web.cpp net/tls)
        # Выполняем создание приложения работы с TLS сервер и мультисертификатом
        add_sample(${SERVER_APP_NET_TLS_MULTI_NAME} sample/net/tls/server-multi.cpp net/tls)
    # Если не нужно собирать семплы для работы с TLS (клиент и сервер)
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample TLS Client/Server: No\n")
    endif (${CMAKE_BUILD_NET_TLS_APP_SAMPLE})

    # Если сборка производится под операционную систему FreeBSD или Linux
    if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        # Если нужно собрать семплы для работы с SCTP (клиент и сервер)
        if (${CMAKE_BUILD_NET_SCTP_APP_SAMPLE})
            # Выводим сообщение о сборке семпла
            message(STATUS "Build sample SCTP Client/Server: Yes\n")
            # Выполняем создание приложения работы с SCTP клиент - TLS
            add_sample(${CLIENT_APP_NET_SCTP_TLS_NAME} sample/net/sctp/client-tls.cpp net/sctp)
            # Выполняем создание приложения работы с SCTP сервер - TLS
            add_sample(${SERVER_APP_NET_SCTP_TLS_NAME} sample/net/sctp/server-tls.cpp net/sctp)
            # Выполняем создание приложения работы с SCTP клиент - DTLS
            add_sample(${CLIENT_APP_NET_SCTP_DTLS_NAME} sample/net/sctp/client-dtls.cpp net/sctp)
            # Выполняем создание приложения работы с SCTP сервер - DTLS
            add_sample(${SERVER_APP_NET_SCTP_DTLS_NAME} sample/net/sctp/server-dtls.cpp net/sctp)
            # Выполняем создание приложения работы с SCTP клиент - AUTH
            add_sample(${CLIENT_APP_NET_SCTP_AUTH_NAME} sample/net/sctp/client-auth.cpp net/sctp)
            # Выполняем создание приложения работы с SCTP сервер - AUTH
            add_sample(${SERVER_APP_NET_SCTP_AUTH_NAME} sample/net/sctp/server-auth.cpp net/sctp)
            # Выполняем создание приложения работы с SCTP клиент - STREAM
            add_sample(${CLIENT_APP_NET_SCTP_STREAM_NAME} sample/net/sctp/client-stream.cpp net/sctp)
            # Выполняем создание приложения работы с SCTP сервер - STREAM
            add_sample(${SERVER_APP_NET_SCTP_STREAM_NAME} sample/net/sctp/server-stream.cpp net/sctp)
            # Выполняем создание приложения работы с SCTP клиент - SEQPACKET
            add_sample(${CLIENT_APP_NET_SCTP_SEQPACKET_NAME} sample/net/sctp/client-seqpacket.cpp net/sctp)
            # Выполняем создание приложения работы с SCTP сервер - SEQPACKET
            add_sample(${SERVER_APP_NET_SCTP_SEQPACKET_NAME} sample/net/sctp/server-seqpacket.cpp net/sctp)
        # Если не нужно собирать семплы для работы с SCTP (клиент и сервер)
        else()
            # Выводим сообщение о сборке семпла
            message(STATUS "Build sample SCTP Client/Server: No\n")
        endif (${CMAKE_BUILD_NET_SCTP_APP_SAMPLE})
    endif()
endif (${CMAKE_BUILD_SAMPLES})

# Если активирован режим отладки
if (CMAKE_BUILD_TYPE MATCHES Debug)
    # Удаляем библиотеку TCMalloc
    SET(TCMALLOC_LIBRARIES "")
endif()

# Если нужно собрать и динамическую библиотеку
if (CMAKE_SHARED_BUILD_LIB)
    # Выполняем сборку динамической библиотеки
    target_link_libraries(
        ${AWH_LIB_NAME}
        PRIVATE
        ${DEPEND_LIBRARY}
        ${TCMALLOC_LIBRARIES}
        ${AWH_FLAGS}
    )

    # Если необходимо собрать семплы
    if (${CMAKE_BUILD_SAMPLES})
        # Если нужно собрать семплы для работы с клиентом
        if (${CMAKE_BUILD_CLIENT_APP_SAMPLE})
            # Выполняем сборку приложения работы с клиентом UDS
            target_link_libraries(
                ${CLIENT_APP_UDS_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом UDS версии 2
            target_link_libraries(
                ${CLIENT_APP_UDS-2_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом TCP
            target_link_libraries(
                ${CLIENT_APP_TCP_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом TCP версии 2
            target_link_libraries(
                ${CLIENT_APP_TCP-2_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом UDP
            target_link_libraries(
                ${CLIENT_APP_UDP_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом UDP версии 2
            target_link_libraries(
                ${CLIENT_APP_UDP-2_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом TLS
            target_link_libraries(
                ${CLIENT_APP_TLS_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом TLS версии 2
            target_link_libraries(
                ${CLIENT_APP_TLS-2_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом DTLS
            target_link_libraries(
                ${CLIENT_APP_DTLS_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом DTLS версии 2
            target_link_libraries(
                ${CLIENT_APP_DTLS-2_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом TLS-Web
            target_link_libraries(
                ${CLIENT_APP_TLS-WEB_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Если сборка производится под операционную систему FreeBSD или Linux
            if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
                # Выполняем сборку приложения работы с клиентом SCTP
                target_link_libraries(
                    ${CLIENT_APP_SCTP_NAME}
                    ${AWH_LIB_NAME}
                    ${TCMALLOC_LIBRARIES}
                    ${AWH_FLAGS}
                )

                # Выполняем сборку приложения работы с клиентом SCTP версии 2
                target_link_libraries(
                    ${CLIENT_APP_SCTP-2_NAME}
                    ${AWH_LIB_NAME}
                    ${TCMALLOC_LIBRARIES}
                    ${AWH_FLAGS}
                )
            endif()
        endif (${CMAKE_BUILD_CLIENT_APP_SAMPLE})

        # Если нужно собрать семплы для работы с сервером
        if (${CMAKE_BUILD_SERVER_APP_SAMPLE})
            # Выполняем сборку приложения работы с сервером UDS
            target_link_libraries(
                ${SERVER_APP_UDS_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером UDS версии 2
            target_link_libraries(
                ${SERVER_APP_UDS-2_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером TCP
            target_link_libraries(
                ${SERVER_APP_TCP_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером TCP версии 2
            target_link_libraries(
                ${SERVER_APP_TCP-2_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером UDP
            target_link_libraries(
                ${SERVER_APP_UDP_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером UDP версии 2
            target_link_libraries(
                ${SERVER_APP_UDP-2_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером TLS
            target_link_libraries(
                ${SERVER_APP_TLS_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером TLS мультисертификатным
            target_link_libraries(
                ${SERVER_APP_TLS-MULTICERT_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером DTLS
            target_link_libraries(
                ${SERVER_APP_DTLS_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером DTLS мультисертификатным
            target_link_libraries(
                ${SERVER_APP_DTLS-MULTICERT_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Если сборка производится под операционную систему FreeBSD или Linux
            if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
                # Выполняем сборку приложения работы с сервером SCTP
                target_link_libraries(
                    ${SERVER_APP_SCTP_NAME}
                    ${AWH_LIB_NAME}
                    ${TCMALLOC_LIBRARIES}
                    ${AWH_FLAGS}
                )

                # Выполняем сборку приложения работы с сервером SCTP версии 2
                target_link_libraries(
                    ${SERVER_APP_SCTP-2_NAME}
                    ${AWH_LIB_NAME}
                    ${TCMALLOC_LIBRARIES}
                    ${AWH_FLAGS}
                )
            endif()
        endif (${CMAKE_BUILD_SERVER_APP_SAMPLE})

        # Если нужно собрать семплы для работы с юнитом VPN
        if (${CMAKE_BUILD_VPN_APP_SAMPLE})
            # Выполняем сборку приложения работы с юнитом VPN клиентом
            target_link_libraries(
                ${CLIENT_APP_VPN_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с юнитом VPN сервером
            target_link_libraries(
                ${SERVER_APP_VPN_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_VPN_APP_SAMPLE})

        # Если нужно собрать семплы для работы с юнитом SOCKS5
        if (${CMAKE_BUILD_SOCKS5_APP_SAMPLE})
            # Выполняем сборку приложения работы с юнитом SOCKS5 клиентом
            target_link_libraries(
                ${CLIENT_APP_SOCKS5_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с юнитом SOCKS5 клиентом для UDP
            target_link_libraries(
                ${CLIENT_APP_SOCKS5-UDP_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с юнитом SOCKS5 клиентом для DTLS
            target_link_libraries(
                ${CLIENT_APP_SOCKS5-DTLS_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с юнитом SOCKS5 сервером
            target_link_libraries(
               ${SERVER_APP_SOCKS5_NAME}
               ${AWH_LIB_NAME}
               ${TCMALLOC_LIBRARIES}
               ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_SOCKS5_APP_SAMPLE})
    
        # Если нужно собрать семплы для работы с юнитом файловой системы
        if (${CMAKE_BUILD_UNIT_FS_APP_SAMPLE})
            # Выполняем сборку приложения работы с юнитом файловой системы
            target_link_libraries(
                ${UNIT_APP_FS_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_UNIT_FS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с юнитом резолвером доменных имён
        if (${CMAKE_BUILD_UNIT_DNS_APP_SAMPLE})
            # Выполняем сборку приложения работы с юнитом резолвером доменных имён
            target_link_libraries(
                ${UNIT_APP_DNS_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_UNIT_DNS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с юнитом синхронизации времени NTP
        if (${CMAKE_BUILD_UNIT_NTP_APP_SAMPLE})
            # Выполняем сборку приложения работы с юнитом синхронизации времени NTP
            target_link_libraries(
                ${UNIT_APP_NTP_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_UNIT_NTP_APP_SAMPLE})

        # Если нужно собрать семплы для работы с юнитом ICMP-клиентом
        if (${CMAKE_BUILD_UNIT_ICMP_APP_SAMPLE})
            # Выполняем сборку приложения работы с юнитом ICMP-клиентом
            target_link_libraries(
                ${UNIT_APP_ICMP_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_UNIT_ICMP_APP_SAMPLE})

        # Если нужно собрать семплы для работы с юнитом таймера
        if (${CMAKE_BUILD_UNIT_TIMER_APP_SAMPLE})
            # Выполняем сборку приложения работы с юнитом таймера
            target_link_libraries(
                ${UNIT_APP_TIMER_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_UNIT_TIMER_APP_SAMPLE})
    
        # Если нужно собрать семплы для работы с юнитом кластера
        if (${CMAKE_BUILD_UNIT_CLUSTER_APP_SAMPLE})
            # Выполняем сборку приложения работы с юнитом кластера
            target_link_libraries(
                ${UNIT_APP_CLUSTER_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_UNIT_CLUSTER_APP_SAMPLE})

        # Если нужно собрать семплы для работы с юнитом уведомителя
        if (${CMAKE_BUILD_UNIT_NOTIFIER_APP_SAMPLE})
            # Выполняем сборку приложения работы с юнитом уведомителя
            target_link_libraries(
                ${UNIT_APP_NOTIFIER_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_UNIT_NOTIFIER_APP_SAMPLE})
    
        # Если нужно собрать семплы для работы с IP-адресами
        if (${CMAKE_BUILD_IP_NET_APP_SAMPLE})
            # Выполняем сборку приложения работы с IP-адресами
            target_link_libraries(
                ${IP_APP_NET_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_IP_NET_APP_SAMPLE})

        # Если нужно собрать семплы для работы с URI
        if (${CMAKE_BUILD_URI_NET_APP_SAMPLE})
            # Выполняем сборку приложения работы с URI
            target_link_libraries(
                ${URI_APP_NET_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_URI_NET_APP_SAMPLE})

        # Если нужно собрать семплы для работы с Ethernet-адресами
        if (${CMAKE_BUILD_ETH_NET_APP_SAMPLE})
            # Выполняем сборку приложения работы с Ethernet-адресами
            target_link_libraries(
                ${ETH_APP_NET_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_ETH_NET_APP_SAMPLE})

        # Если нужно собрать семплы для работы с датой и временем
        if (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})
            # Выполняем сборку приложения даты и времени
            target_link_libraries(
                ${DATE_APP_SYS_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с бинарным контейнером
        if (${CMAKE_BUILD_BINBOX_SYS_APP_SAMPLE})
            # Выполняем сборку приложения работы с бинарным контейнером
            target_link_libraries(
                ${BINBOX_APP_SYS_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_BINBOX_SYS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с файловой системой
        if (${CMAKE_BUILD_FILESYSTEM_SYS_APP_SAMPLE})
            # Выполняем сборку приложения файловой системы
            target_link_libraries(
                ${FILESYSTEM_APP_SYS_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_FILESYSTEM_SYS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с резольвером процессов
        if (${CMAKE_BUILD_PROCRE_SYS_APP_SAMPLE})
            # Выполняем сборку приложения резольвера процессов
            target_link_libraries(
                ${PROCRE_APP_SYS_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_PROCRE_SYS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с шифрованием и хэшированием
        if (${CMAKE_BUILD_CRYPTO_SYS_APP_SAMPLE})
            # Выполняем сборку приложения шифрования и хэширования
            target_link_libraries(
                ${CRYPTO_APP_SYS_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_CRYPTO_SYS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с HTTP-авторизацией
        if (${CMAKE_BUILD_AUTH_PROTO_APP_SAMPLE})
            # Выполняем сборку приложения работы с HTTP-авторизацией
            target_link_libraries(
                ${AUTH_APP_PROTO_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_AUTH_PROTO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с HTTP-заголовками
        if (${CMAKE_BUILD_HEADERS_PROTO_APP_SAMPLE})
            # Выполняем сборку приложения работы с HTTP-заголовками
            target_link_libraries(
                ${HEADERS_APP_PROTO_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_HEADERS_PROTO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с парсером HTTP/1.0 и HTTP/1.1
        if (${CMAKE_BUILD_PARSER_PROTO_APP_SAMPLE})
            # Выполняем сборку приложения работы с парсером HTTP/1.0 и HTTP/1.1
            target_link_libraries(
                ${PARSER_APP_PROTO_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_PARSER_PROTO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с парсером HTTP/2
        if (${CMAKE_BUILD_PARSER2_PROTO_APP_SAMPLE})
            # Выполняем сборку приложения работы с парсером HTTP/2
            target_link_libraries(
                ${PARSER2_APP_PROTO_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_PARSER2_PROTO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с протоколом SOCKS5
        if (${CMAKE_BUILD_SOCKS5_PROTO_APP_SAMPLE})
            # Выполняем сборку приложения работы с протоколом SOCKS5
            target_link_libraries(
                ${SOCKS5_APP_PROTO_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_SOCKS5_PROTO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с протоколом QUIC
        if (${CMAKE_BUILD_QUIC_PROTO_APP_SAMPLE})
            # Выполняем сборку приложения работы с QUIC клиентом
            target_link_libraries(
                ${QUIC_CLIENT_APP_PROTO_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения работы с QUIC сервером
            target_link_libraries(
                ${QUIC_SERVER_APP_PROTO_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_QUIC_PROTO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с компрессией данных
        if (${CMAKE_BUILD_COMPRESSOR_SYS_APP_SAMPLE})
            # Выполняем сборку приложения компрессии данных
            target_link_libraries(
                ${COMPRESSOR_APP_BLOCK_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
            target_link_libraries(
                ${COMPRESSOR_APP_STREAM_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_COMPRESSOR_SYS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с каталогами
        if (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})
            # Выполняем сборку приложения работы с каталогами
            target_link_libraries(
                ${DIR_APP_IO_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с файлами
        if (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})
            # Выполняем сборку приложения работы с файлами
            target_link_libraries(
                ${FILE_APP_IO_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с пингами (ICMP)
        if (${CMAKE_BUILD_PING_IO_APP_SAMPLE})
            # Выполняем сборку приложения работы с пингами (ICMP)
            target_link_libraries(
                ${PING_APP_IO_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_PING_IO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с таймером
        if (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})
            # Выполняем сборку приложения таймера
            target_link_libraries(
                ${TIMER_APP_IO_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с пользовательскими событиями
        if (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})
            # Выполняем сборку приложения пользовательских событий
            target_link_libraries(
                ${EVENTS_APP_IO_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с межпроцессорным взаимодействием
        if (${CMAKE_BUILD_IPC_IO_APP_SAMPLE})
            # Выполняем сборку приложения работы с межпроцессорным взаимодействием (именованный канал)
            target_link_libraries(
                ${IPC_PIPE_APP_IO_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с межпроцессорным взаимодействием (поток)
            target_link_libraries(
                ${IPC_STREAM_APP_IO_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с межпроцессорным взаимодействием (датаграмма)
            target_link_libraries(
                ${IPC_DATAGRAM_APP_IO_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с межпроцессорным взаимодействием (последовательная упаковка)
            target_link_libraries(
                ${IPC_SEQPACKET_APP_IO_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_IPC_IO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с TCP (клиент и сервер)
        if (${CMAKE_BUILD_NET_TCP_APP_SAMPLE})
            # Выполняем сборку приложения клиента
            target_link_libraries(
                ${CLIENT_APP_NET_TCP_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера
            target_link_libraries(
                ${SERVER_APP_NET_TCP_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_TCP_APP_SAMPLE})

        # Если нужно собрать семплы для работы с UDP (клиент и сервер)
        if (${CMAKE_BUILD_NET_UDP_APP_SAMPLE})
            # Выполняем сборку приложения клиента
            target_link_libraries(
                ${CLIENT_APP_NET_UDP_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения клиента с подключением
            target_link_libraries(
                ${CLIENT_APP_NET_UDP_CONN_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера
            target_link_libraries(
                ${SERVER_APP_NET_UDP_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_UDP_APP_SAMPLE})

        # Если нужно собрать семплы для работы с ограничителем пропускной способности (клиент и сервер)
        if (${CMAKE_BUILD_NET_BANDWIDTH_APP_SAMPLE})
            # Выполняем сборку приложения клиента
            target_link_libraries(
                ${CLIENT_APP_NET_TCP_BANDWIDTH_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера
            target_link_libraries(
                ${SERVER_APP_NET_TCP_BANDWIDTH_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_BANDWIDTH_APP_SAMPLE})

        # Если нужно собрать семплы для работы с TUN (клиент и сервер)
        if (${CMAKE_BUILD_NET_TUN_APP_SAMPLE})
           # Выполняем сборку приложения клиента
            target_link_libraries(
                ${CLIENT_APP_NET_TUN_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера
            target_link_libraries(
                ${SERVER_APP_NET_TUN_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_TUN_APP_SAMPLE})

        # Если нужно собрать семплы для работы с UDP Broadcast (клиент и сервер)
        if (${CMAKE_BUILD_NET_BROADCAST_APP_SAMPLE})
            # Выполняем сборку приложения клиента
            target_link_libraries(
                ${CLIENT_APP_NET_BROADCAST_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера
            target_link_libraries(
                ${SERVER_APP_NET_BROADCAST_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_BROADCAST_APP_SAMPLE})

        # Если нужно собрать семплы для работы с UDP Multicast (клиент и сервер)
        if (${CMAKE_BUILD_NET_MULTICAST_APP_SAMPLE})
            # Выполняем сборку приложения клиента сценарий 1
            target_link_libraries(
                ${CLIENT_APP_NET_MULTICAST1_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера сценарий 1
            target_link_libraries(
                ${SERVER_APP_NET_MULTICAST1_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения клиента сценарий 3
            target_link_libraries(
                ${CLIENT_APP_NET_MULTICAST3_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера сценарий 3
            target_link_libraries(
                ${SERVER_APP_NET_MULTICAST3_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_MULTICAST_APP_SAMPLE})

        # Если нужно собрать семплы для работы с UDP Splice (клиент и сервер)
        if (${CMAKE_BUILD_NET_SPLICE_APP_SAMPLE})
            # Выполняем сборку приложения клиента
            target_link_libraries(
                ${CLIENT_APP_NET_SPLICE_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера
            target_link_libraries(
                ${SERVER_APP_NET_SPLICE_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_SPLICE_APP_SAMPLE})

        # Если нужно собрать семплы для работы с UDS (клиент и сервер)
        if (${CMAKE_BUILD_NET_UDS_APP_SAMPLE})
            # Выполняем сборку приложения клиента UDS-TCP
            target_link_libraries(
                ${CLIENT_APP_NET_UDS_TCP_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера UDS-TCP
            target_link_libraries(
                ${SERVER_APP_NET_UDS_TCP_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения клиента UDS-UDP
            target_link_libraries(
                ${CLIENT_APP_NET_UDS_UDP_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера UDS-UDP
            target_link_libraries(
                ${SERVER_APP_NET_UDS_UDP_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_UDS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с DTLS (клиент и сервер)
        if (${CMAKE_BUILD_NET_DTLS_APP_SAMPLE})
            # Выполняем сборку приложения клиента
            target_link_libraries(
                ${CLIENT_APP_NET_DTLS_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера
            target_link_libraries(
                ${SERVER_APP_NET_DTLS_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_DTLS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с TLS (клиент и сервер)
        if (${CMAKE_BUILD_NET_TLS_APP_SAMPLE})
            # Выполняем сборку приложения клиента работы с TLS
            target_link_libraries(
                ${CLIENT_APP_NET_TLS_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера работы с TLS
            target_link_libraries(
                ${SERVER_APP_NET_TLS_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения клиента работы с TLS и HTTP/1.1 подключением
            target_link_libraries(
                ${CLIENT_APP_NET_TLS_WEB_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера работы с TLS и мультисертификатом
            target_link_libraries(
                ${SERVER_APP_NET_TLS_MULTI_NAME}
                ${AWH_LIB_NAME}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_TLS_APP_SAMPLE})

        # Если сборка производится под операционную систему FreeBSD или Linux
        if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
            # Если нужно собрать семплы для работы с SCTP (клиент и сервер)
            if (${CMAKE_BUILD_NET_SCTP_APP_SAMPLE})
                # Выполняем сборку приложения клиента работы с TLS
                target_link_libraries(
                    ${CLIENT_APP_NET_SCTP_TLS_NAME}
                    ${AWH_LIB_NAME}
                    ${TCMALLOC_LIBRARIES}
                )

                # Выполняем сборку приложения сервера работы с TLS
                target_link_libraries(
                    ${SERVER_APP_NET_SCTP_TLS_NAME}
                    ${AWH_LIB_NAME}
                    ${TCMALLOC_LIBRARIES}
                )

                # Выполняем сборку приложения клиента работы с DTLS
                target_link_libraries(
                    ${CLIENT_APP_NET_SCTP_DTLS_NAME}
                    ${AWH_LIB_NAME}
                    ${TCMALLOC_LIBRARIES}
                )

                # Выполняем сборку приложения сервера работы с DTLS
                target_link_libraries(
                    ${SERVER_APP_NET_SCTP_DTLS_NAME}
                    ${AWH_LIB_NAME}
                    ${TCMALLOC_LIBRARIES}
                )

                # Выполняем сборку приложения клиента работы с SCTP - AUTH
                target_link_libraries(
                    ${CLIENT_APP_NET_SCTP_AUTH_NAME}
                    ${AWH_LIB_NAME}
                    ${TCMALLOC_LIBRARIES}
                )

                # Выполняем сборку приложения сервера работы с SCTP - AUTH
                target_link_libraries(
                    ${SERVER_APP_NET_SCTP_AUTH_NAME}
                    ${AWH_LIB_NAME}
                    ${TCMALLOC_LIBRARIES}
                )

                # Выполняем сборку приложения клиента работы с SCTP - STREAM
                target_link_libraries(
                    ${CLIENT_APP_NET_SCTP_STREAM_NAME}
                    ${AWH_LIB_NAME}
                    ${TCMALLOC_LIBRARIES}
                )

                # Выполняем сборку приложения сервера работы с SCTP - STREAM
                target_link_libraries(
                    ${SERVER_APP_NET_SCTP_STREAM_NAME}
                    ${AWH_LIB_NAME}
                    ${TCMALLOC_LIBRARIES}
                )

                # Выполняем сборку приложения клиента работы с SCTP - SEQPACKET
                target_link_libraries(
                    ${CLIENT_APP_NET_SCTP_SEQPACKET_NAME}
                    ${AWH_LIB_NAME}
                    ${TCMALLOC_LIBRARIES}
                )

                # Выполняем сборку приложения сервера работы с SCTP - SEQPACKET
                target_link_libraries(
                    ${SERVER_APP_NET_SCTP_SEQPACKET_NAME}
                    ${AWH_LIB_NAME}
                    ${TCMALLOC_LIBRARIES}
                )
            endif (${CMAKE_BUILD_NET_SCTP_APP_SAMPLE})
        endif()
    endif (${CMAKE_BUILD_SAMPLES})
# Если нужно собрать и статическую библиотеку
else (CMAKE_SHARED_BUILD_LIB)
    # Если необходимо собрать семплы
    if (${CMAKE_BUILD_SAMPLES})
        # Если нужно собрать семплы для работы с клиентом
        if (${CMAKE_BUILD_CLIENT_APP_SAMPLE})
            # Выполняем сборку приложения работы с клиентом UDS
            target_link_libraries(
                ${CLIENT_APP_UDS_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом UDS версии 2
            target_link_libraries(
                ${CLIENT_APP_UDS-2_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом TCP
            target_link_libraries(
                ${CLIENT_APP_TCP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом TCP версии 2
            target_link_libraries(
                ${CLIENT_APP_TCP-2_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом UDP
            target_link_libraries(
                ${CLIENT_APP_UDP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом UDP версии 2
            target_link_libraries(
                ${CLIENT_APP_UDP-2_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом TLS
            target_link_libraries(
                ${CLIENT_APP_TLS_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом TLS версии 2
            target_link_libraries(
                ${CLIENT_APP_TLS-2_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом DTLS
            target_link_libraries(
                ${CLIENT_APP_DTLS_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом DTLS версии 2
            target_link_libraries(
                ${CLIENT_APP_DTLS-2_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом TLS-Web
            target_link_libraries(
                ${CLIENT_APP_TLS-WEB_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Если сборка производится под операционную систему FreeBSD или Linux
            if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
                # Выполняем сборку приложения работы с клиентом SCTP
                target_link_libraries(
                    ${CLIENT_APP_SCTP_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                    ${TCMALLOC_LIBRARIES}
                    ${AWH_FLAGS}
                )

                # Выполняем сборку приложения работы с клиентом SCTP версии 2
                target_link_libraries(
                    ${CLIENT_APP_SCTP-2_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                    ${TCMALLOC_LIBRARIES}
                    ${AWH_FLAGS}
                )
            endif()
        endif (${CMAKE_BUILD_CLIENT_APP_SAMPLE})

        # Если нужно собрать семплы для работы с сервером
        if (${CMAKE_BUILD_SERVER_APP_SAMPLE})
            # Выполняем сборку приложения работы с сервером UDS
            target_link_libraries(
                ${SERVER_APP_UDS_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером UDS версии 2
            target_link_libraries(
                ${SERVER_APP_UDS-2_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером TCP
            target_link_libraries(
                ${SERVER_APP_TCP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером TCP версии 2
            target_link_libraries(
                ${SERVER_APP_TCP-2_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером UDP
            target_link_libraries(
                ${SERVER_APP_UDP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером UDP версии 2
            target_link_libraries(
                ${SERVER_APP_UDP-2_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером TLS
            target_link_libraries(
                ${SERVER_APP_TLS_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером TLS мультисертификатным
            target_link_libraries(
                ${SERVER_APP_TLS-MULTICERT_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером DTLS
            target_link_libraries(
                ${SERVER_APP_DTLS_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером DTLS мультисертификатным
            target_link_libraries(
                ${SERVER_APP_DTLS-MULTICERT_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Если сборка производится под операционную систему FreeBSD или Linux
            if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
                # Выполняем сборку приложения работы с сервером SCTP
                target_link_libraries(
                    ${SERVER_APP_SCTP_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                    ${TCMALLOC_LIBRARIES}
                    ${AWH_FLAGS}
                )

                # Выполняем сборку приложения работы с сервером SCTP версии 2
                target_link_libraries(
                    ${SERVER_APP_SCTP-2_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                    ${TCMALLOC_LIBRARIES}
                    ${AWH_FLAGS}
                )
            endif()
        endif (${CMAKE_BUILD_SERVER_APP_SAMPLE})

        # Если нужно собрать семплы для работы с юнитом VPN
        if (${CMAKE_BUILD_VPN_APP_SAMPLE})
            # Выполняем сборку приложения работы с юнитом VPN клиентом
            target_link_libraries(
                ${CLIENT_APP_VPN_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с юнитом VPN сервером
            target_link_libraries(
                ${SERVER_APP_VPN_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_VPN_APP_SAMPLE})

        # Если нужно собрать семплы для работы с юнитом SOCKS5
        if (${CMAKE_BUILD_SOCKS5_APP_SAMPLE})
            # Выполняем сборку приложения работы с юнитом SOCKS5 клиентом
            target_link_libraries(
                ${CLIENT_APP_SOCKS5_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с юнитом SOCKS5 клиентом для UDP
            target_link_libraries(
                ${CLIENT_APP_SOCKS5-UDP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с юнитом SOCKS5 клиентом для DTLS
            target_link_libraries(
                ${CLIENT_APP_SOCKS5-DTLS_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с юнитом SOCKS5 сервером
            target_link_libraries(
               ${SERVER_APP_SOCKS5_NAME}
               ${AWH_LIB_NAME}
               ${DEPEND_LIBRARY}
               ${TCMALLOC_LIBRARIES}
               ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_SOCKS5_APP_SAMPLE})

        # Если нужно собрать семплы для работы с юнитом файловой системы
        if (${CMAKE_BUILD_UNIT_FS_APP_SAMPLE})
            # Выполняем сборку приложения работы с юнитом файловой системы
            target_link_libraries(
                ${UNIT_APP_FS_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_UNIT_FS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с юнитом резолвером доменных имён
        if (${CMAKE_BUILD_UNIT_DNS_APP_SAMPLE})
            # Выполняем сборку приложения работы с юнитом резолвером доменных имён
            target_link_libraries(
                ${UNIT_APP_DNS_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_UNIT_DNS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с юнитом синхронизации времени NTP
        if (${CMAKE_BUILD_UNIT_NTP_APP_SAMPLE})
            # Выполняем сборку приложения работы с юнитом синхронизации времени NTP
            target_link_libraries(
                ${UNIT_APP_NTP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_UNIT_NTP_APP_SAMPLE})

        # Если нужно собрать семплы для работы с юнитом ICMP-клиентом
        if (${CMAKE_BUILD_UNIT_ICMP_APP_SAMPLE})
            # Выполняем сборку приложения работы с юнитом ICMP-клиентом
            target_link_libraries(
                ${UNIT_APP_ICMP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_UNIT_ICMP_APP_SAMPLE})

        # Если нужно собрать семплы для работы с юнитом таймера
        if (${CMAKE_BUILD_UNIT_TIMER_APP_SAMPLE})
            # Выполняем сборку приложения работы с юнитом таймера
            target_link_libraries(
                ${UNIT_APP_TIMER_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_UNIT_TIMER_APP_SAMPLE})

        # Если нужно собрать семплы для работы с юнитом кластера
        if (${CMAKE_BUILD_UNIT_CLUSTER_APP_SAMPLE})
            # Выполняем сборку приложения работы с юнитом кластера
            target_link_libraries(
                ${UNIT_APP_CLUSTER_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_UNIT_CLUSTER_APP_SAMPLE})

        # Если нужно собрать семплы для работы с юнитом уведомителя
        if (${CMAKE_BUILD_UNIT_NOTIFIER_APP_SAMPLE})
            # Выполняем сборку приложения работы с юнитом уведомителя
            target_link_libraries(
                ${UNIT_APP_NOTIFIER_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_UNIT_NOTIFIER_APP_SAMPLE})
    
        # Если нужно собрать семплы для работы с IP-адресами
        if (${CMAKE_BUILD_IP_NET_APP_SAMPLE})
            # Выполняем сборку приложения работы с IP-адресами
            target_link_libraries(
                ${IP_APP_NET_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_IP_NET_APP_SAMPLE})

        # Если нужно собрать семплы для работы с URI
        if (${CMAKE_BUILD_URI_NET_APP_SAMPLE})
            # Выполняем сборку приложения работы с URI
            target_link_libraries(
                ${URI_APP_NET_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_URI_NET_APP_SAMPLE})

        # Если нужно собрать семплы для работы с Ethernet-адресами
        if (${CMAKE_BUILD_ETH_NET_APP_SAMPLE})
            # Выполняем сборку приложения работы с Ethernet-адресами
            target_link_libraries(
                ${ETH_APP_NET_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_ETH_NET_APP_SAMPLE})

        # Если нужно собрать семплы для работы с датой и временем
        if (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})
            # Выполняем сборку приложения даты и времени
            target_link_libraries(
                ${DATE_APP_SYS_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с бинарным контейнером
        if (${CMAKE_BUILD_BINBOX_SYS_APP_SAMPLE})
            # Выполняем сборку приложения работы с бинарным контейнером
            target_link_libraries(
                ${BINBOX_APP_SYS_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_BINBOX_SYS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с файловой системой
        if (${CMAKE_BUILD_FILESYSTEM_SYS_APP_SAMPLE})
            # Выполняем сборку приложения файловой системы
            target_link_libraries(
                ${FILESYSTEM_APP_SYS_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_FILESYSTEM_SYS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с резольвером процессов
        if (${CMAKE_BUILD_PROCRE_SYS_APP_SAMPLE})
            # Выполняем сборку приложения резольвера процессов
            target_link_libraries(
                ${PROCRE_APP_SYS_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_PROCRE_SYS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с шифрованием и хэшированием
        if (${CMAKE_BUILD_CRYPTO_SYS_APP_SAMPLE})
            # Выполняем сборку приложения шифрования и хэширования
            target_link_libraries(
                ${CRYPTO_APP_SYS_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_CRYPTO_SYS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с HTTP-авторизацией
        if (${CMAKE_BUILD_AUTH_PROTO_APP_SAMPLE})
            # Выполняем сборку приложения работы с HTTP-авторизацией
            target_link_libraries(
                ${AUTH_APP_PROTO_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_AUTH_PROTO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с HTTP-заголовками
        if (${CMAKE_BUILD_HEADERS_PROTO_APP_SAMPLE})
            # Выполняем сборку приложения работы с HTTP-заголовками
            target_link_libraries(
                ${HEADERS_APP_PROTO_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_HEADERS_PROTO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с парсером HTTP/1.0 и HTTP/1.1
        if (${CMAKE_BUILD_PARSER_PROTO_APP_SAMPLE})
            # Выполняем сборку приложения работы с парсером HTTP/1.0 и HTTP/1.1
            target_link_libraries(
                ${PARSER_APP_PROTO_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_PARSER_PROTO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с парсером HTTP/2
        if (${CMAKE_BUILD_PARSER2_PROTO_APP_SAMPLE})
            # Выполняем сборку приложения работы с парсером HTTP/2
            target_link_libraries(
                ${PARSER2_APP_PROTO_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_PARSER2_PROTO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с протоколом SOCKS5
        if (${CMAKE_BUILD_SOCKS5_PROTO_APP_SAMPLE})
            # Выполняем сборку приложения работы с протоколом SOCKS5
            target_link_libraries(
                ${SOCKS5_APP_PROTO_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_SOCKS5_PROTO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с протоколом QUIC
        if (${CMAKE_BUILD_QUIC_PROTO_APP_SAMPLE})
            # Выполняем сборку приложения работы с QUIC клиентом
            target_link_libraries(
                ${QUIC_CLIENT_APP_PROTO_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения работы с QUIC сервером
            target_link_libraries(
                ${QUIC_SERVER_APP_PROTO_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_QUIC_PROTO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с компрессией данных
        if (${CMAKE_BUILD_COMPRESSOR_SYS_APP_SAMPLE})
            # Выполняем сборку приложения компрессии данных
            target_link_libraries(
                ${COMPRESSOR_APP_BLOCK_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
            target_link_libraries(
                ${COMPRESSOR_APP_STREAM_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_COMPRESSOR_SYS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с каталогами
        if (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})
            # Выполняем сборку приложения работы с каталогами
            target_link_libraries(
                ${DIR_APP_IO_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с файлами
        if (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})
            # Выполняем сборку приложения работы с файлами
            target_link_libraries(
                ${FILE_APP_IO_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с пингами (ICMP)
        if (${CMAKE_BUILD_PING_IO_APP_SAMPLE})
            # Выполняем сборку приложения работы с пингами (ICMP)
            target_link_libraries(
                ${PING_APP_IO_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_PING_IO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с таймером
        if (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})
            # Выполняем сборку приложения таймера
            target_link_libraries(
                ${TIMER_APP_IO_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с пользовательскими событиями
        if (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})
            # Выполняем сборку приложения пользовательских событий
            target_link_libraries(
                ${EVENTS_APP_IO_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с межпроцессорным взаимодействием
        if (${CMAKE_BUILD_IPC_IO_APP_SAMPLE})
            # Выполняем сборку приложения работы с межпроцессорным взаимодействием (именованный канал)
            target_link_libraries(
                ${IPC_PIPE_APP_IO_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с межпроцессорным взаимодействием (поток)
            target_link_libraries(
                ${IPC_STREAM_APP_IO_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с межпроцессорным взаимодействием (датаграмма)
            target_link_libraries(
                ${IPC_DATAGRAM_APP_IO_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с межпроцессорным взаимодействием (последовательная упаковка)
            target_link_libraries(
                ${IPC_SEQPACKET_APP_IO_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_IPC_IO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с TCP (клиент и сервер)
        if (${CMAKE_BUILD_NET_TCP_APP_SAMPLE})
            # Выполняем сборку приложения клиента
            target_link_libraries(
                ${CLIENT_APP_NET_TCP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера
            target_link_libraries(
                ${SERVER_APP_NET_TCP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_TCP_APP_SAMPLE})

        # Если нужно собрать семплы для работы с UDP (клиент и сервер)
        if (${CMAKE_BUILD_NET_UDP_APP_SAMPLE})
            # Выполняем сборку приложения клиента
            target_link_libraries(
                ${CLIENT_APP_NET_UDP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения клиента с подключением
            target_link_libraries(
                ${CLIENT_APP_NET_UDP_CONN_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера
            target_link_libraries(
                ${SERVER_APP_NET_UDP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_UDP_APP_SAMPLE})

        # Если нужно собрать семплы для работы с ограничителем пропускной способности (клиент и сервер)
        if (${CMAKE_BUILD_NET_BANDWIDTH_APP_SAMPLE})
            # Выполняем сборку приложения клиента
            target_link_libraries(
                ${CLIENT_APP_NET_TCP_BANDWIDTH_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера
            target_link_libraries(
                ${SERVER_APP_NET_TCP_BANDWIDTH_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_BANDWIDTH_APP_SAMPLE})

        # Если нужно собрать семплы для работы с TUN (клиент и сервер)
        if (${CMAKE_BUILD_NET_TUN_APP_SAMPLE})
           # Выполняем сборку приложения клиента
            target_link_libraries(
                ${CLIENT_APP_NET_TUN_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера
            target_link_libraries(
                ${SERVER_APP_NET_TUN_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_TUN_APP_SAMPLE})

        # Если нужно собрать семплы для работы с UDP Broadcast (клиент и сервер)
        if (${CMAKE_BUILD_NET_BROADCAST_APP_SAMPLE})
            # Выполняем сборку приложения клиента
            target_link_libraries(
                ${CLIENT_APP_NET_BROADCAST_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера
            target_link_libraries(
                ${SERVER_APP_NET_BROADCAST_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_BROADCAST_APP_SAMPLE})

        # Если нужно собрать семплы для работы с UDP Multicast (клиент и сервер)
        if (${CMAKE_BUILD_NET_MULTICAST_APP_SAMPLE})
            # Выполняем сборку приложения клиента сценарий 1
            target_link_libraries(
                ${CLIENT_APP_NET_MULTICAST1_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера сценарий 1
            target_link_libraries(
                ${SERVER_APP_NET_MULTICAST1_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения клиента сценарий 3
            target_link_libraries(
                ${CLIENT_APP_NET_MULTICAST3_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера сценарий 3
            target_link_libraries(
                ${SERVER_APP_NET_MULTICAST3_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_MULTICAST_APP_SAMPLE})

        # Если нужно собрать семплы для работы с UDP Splice (клиент и сервер)
        if (${CMAKE_BUILD_NET_SPLICE_APP_SAMPLE})
            # Выполняем сборку приложения клиента
            target_link_libraries(
                ${CLIENT_APP_NET_SPLICE_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера
            target_link_libraries(
                ${SERVER_APP_NET_SPLICE_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_SPLICE_APP_SAMPLE})

        # Если нужно собрать семплы для работы с UDS (клиент и сервер)
        if (${CMAKE_BUILD_NET_UDS_APP_SAMPLE})
            # Выполняем сборку приложения клиента UDS-TCP
            target_link_libraries(
                ${CLIENT_APP_NET_UDS_TCP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера UDS-TCP
            target_link_libraries(
                ${SERVER_APP_NET_UDS_TCP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения клиента UDS-UDP
            target_link_libraries(
                ${CLIENT_APP_NET_UDS_UDP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера UDS-UDP
            target_link_libraries(
                ${SERVER_APP_NET_UDS_UDP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_UDS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с DTLS (клиент и сервер)
        if (${CMAKE_BUILD_NET_DTLS_APP_SAMPLE})
            # Выполняем сборку приложения клиента
            target_link_libraries(
                ${CLIENT_APP_NET_DTLS_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера
            target_link_libraries(
                ${SERVER_APP_NET_DTLS_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_DTLS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с TLS (клиент и сервер)
        if (${CMAKE_BUILD_NET_TLS_APP_SAMPLE})
            # Выполняем сборку приложения клиента работы с TLS
            target_link_libraries(
                ${CLIENT_APP_NET_TLS_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера работы с TLS
            target_link_libraries(
                ${SERVER_APP_NET_TLS_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения клиента работы с TLS и HTTP/1.1 подключением
            target_link_libraries(
                ${CLIENT_APP_NET_TLS_WEB_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера работы с TLS и мультисертификатом
            target_link_libraries(
                ${SERVER_APP_NET_TLS_MULTI_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${TCMALLOC_LIBRARIES}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_TLS_APP_SAMPLE})

        # Если сборка производится под операционную систему FreeBSD или Linux
        if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
            # Если нужно собрать семплы для работы с SCTP (клиент и сервер)
            if (${CMAKE_BUILD_NET_SCTP_APP_SAMPLE})
                # Выполняем сборку приложения клиента работы с TLS
                target_link_libraries(
                    ${CLIENT_APP_NET_SCTP_TLS_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                    ${TCMALLOC_LIBRARIES}
                )

                # Выполняем сборку приложения сервера работы с TLS
                target_link_libraries(
                    ${SERVER_APP_NET_SCTP_TLS_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                    ${TCMALLOC_LIBRARIES}
                )

                # Выполняем сборку приложения клиента работы с DTLS
                target_link_libraries(
                    ${CLIENT_APP_NET_SCTP_DTLS_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                    ${TCMALLOC_LIBRARIES}
                )

                # Выполняем сборку приложения сервера работы с DTLS
                target_link_libraries(
                    ${SERVER_APP_NET_SCTP_DTLS_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                    ${TCMALLOC_LIBRARIES}
                )

                # Выполняем сборку приложения клиента работы с SCTP - AUTH
                target_link_libraries(
                    ${CLIENT_APP_NET_SCTP_AUTH_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                    ${TCMALLOC_LIBRARIES}
                )

                # Выполняем сборку приложения сервера работы с SCTP - AUTH
                target_link_libraries(
                    ${SERVER_APP_NET_SCTP_AUTH_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                    ${TCMALLOC_LIBRARIES}
                )

                # Выполняем сборку приложения клиента работы с SCTP - STREAM
                target_link_libraries(
                    ${CLIENT_APP_NET_SCTP_STREAM_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                    ${TCMALLOC_LIBRARIES}
                )

                # Выполняем сборку приложения сервера работы с SCTP - STREAM
                target_link_libraries(
                    ${SERVER_APP_NET_SCTP_STREAM_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                    ${TCMALLOC_LIBRARIES}
                )

                # Выполняем сборку приложения клиента работы с SCTP - SEQPACKET
                target_link_libraries(
                    ${CLIENT_APP_NET_SCTP_SEQPACKET_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                    ${TCMALLOC_LIBRARIES}
                )

                # Выполняем сборку приложения сервера работы с SCTP - SEQPACKET
                target_link_libraries(
                    ${SERVER_APP_NET_SCTP_SEQPACKET_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                    ${TCMALLOC_LIBRARIES}
                )
            endif (${CMAKE_BUILD_NET_SCTP_APP_SAMPLE})
        endif()
    endif (${CMAKE_BUILD_SAMPLES})
endif (CMAKE_SHARED_BUILD_LIB)

# Если необходимо собрать семплы
if (${CMAKE_BUILD_SAMPLES})
    # Активация формирования дампа ядра в реже отладки под macOS
    if (CMAKE_BUILD_TYPE MATCHES Debug)
        # Если сборка производится под операционную систему macOS
        if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
            # Указываем путь к файлу с правами для разрешения формирования дампа ядра на macOS
            SET(entitlements_file "${CMAKE_SOURCE_DIR}/segv.entitlements")

            # Если нужно собрать семплы для работы с клиентом
            if (${CMAKE_BUILD_CLIENT_APP_SAMPLE})
                # Выполняем сборку приложения работы с клиентом UDS
                add_custom_command(TARGET "${CLIENT_APP_UDS_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_UDS_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CLIENT_APP_UDS_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с клиентом UDS версии 2
                add_custom_command(TARGET "${CLIENT_APP_UDS-2_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_UDS-2_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CLIENT_APP_UDS-2_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с клиентом TCP
                add_custom_command(TARGET "${CLIENT_APP_TCP_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_TCP_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CLIENT_APP_TCP_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с клиентом TCP версии 2
                add_custom_command(TARGET "${CLIENT_APP_TCP-2_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_TCP-2_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CLIENT_APP_TCP-2_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с клиентом UDP
                add_custom_command(TARGET "${CLIENT_APP_UDP_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_UDP_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CLIENT_APP_UDP_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с клиентом UDP версии 2
                add_custom_command(TARGET "${CLIENT_APP_UDP-2_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_UDP-2_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CLIENT_APP_UDP-2_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с клиентом TLS
                add_custom_command(TARGET "${CLIENT_APP_TLS_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_TLS_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CLIENT_APP_TLS_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с клиентом TLS версии 2
                add_custom_command(TARGET "${CLIENT_APP_TLS-2_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_TLS-2_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CLIENT_APP_TLS-2_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с клиентом DTLS
                add_custom_command(TARGET "${CLIENT_APP_DTLS_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_DTLS_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CLIENT_APP_DTLS_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с клиентом DTLS версии 2
                add_custom_command(TARGET "${CLIENT_APP_DTLS-2_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_DTLS-2_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CLIENT_APP_DTLS-2_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            
                # Выполняем сборку приложения работы с клиентом TLS-Web
                add_custom_command(TARGET "${CLIENT_APP_TLS-WEB_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_TLS-WEB_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CLIENT_APP_TLS-WEB_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Если сборка производится под операционную систему FreeBSD или Linux
                if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
                    # Выполняем сборку приложения работы с клиентом SCTP
                    add_custom_command(TARGET "${CLIENT_APP_SCTP_NAME}" POST_BUILD
                        COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_SCTP_NAME}>" "${entitlements_file}"
                        COMMENT "Sign target \"${CLIENT_APP_SCTP_NAME}\" to enable core dump on macOS"
                        VERBATIM
                    )

                    # Выполняем сборку приложения работы с клиентом SCTP версии 2
                    add_custom_command(TARGET "${CLIENT_APP_SCTP-2_NAME}" POST_BUILD
                        COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_SCTP-2_NAME}>" "${entitlements_file}"
                        COMMENT "Sign target \"${CLIENT_APP_SCTP-2_NAME}\" to enable core dump on macOS"
                        VERBATIM
                    )
                endif()
            endif (${CMAKE_BUILD_CLIENT_APP_SAMPLE})

            # Если нужно собрать семплы для работы с сервером
            if (${CMAKE_BUILD_SERVER_APP_SAMPLE})
                # Выполняем сборку приложения работы с сервером UDS
                add_custom_command(TARGET "${SERVER_APP_UDS_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${SERVER_APP_UDS_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${SERVER_APP_UDS_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с сервером UDS версии 2
                add_custom_command(TARGET "${SERVER_APP_UDS-2_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${SERVER_APP_UDS-2_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${SERVER_APP_UDS-2_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с сервером TCP
                add_custom_command(TARGET "${SERVER_APP_TCP_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${SERVER_APP_TCP_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${SERVER_APP_TCP_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с сервером TCP версии 2
                add_custom_command(TARGET "${SERVER_APP_TCP-2_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${SERVER_APP_TCP-2_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${SERVER_APP_TCP-2_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с сервером UDP
                add_custom_command(TARGET "${SERVER_APP_UDP_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${SERVER_APP_UDP_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${SERVER_APP_UDP_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с сервером UDP версии 2
                add_custom_command(TARGET "${SERVER_APP_UDP-2_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${SERVER_APP_UDP-2_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${SERVER_APP_UDP-2_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с сервером TLS
                add_custom_command(TARGET "${SERVER_APP_TLS_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${SERVER_APP_TLS_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${SERVER_APP_TLS_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с сервером TLS мультисертификатным
                add_custom_command(TARGET "${SERVER_APP_TLS-MULTICERT_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${SERVER_APP_TLS-MULTICERT_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${SERVER_APP_TLS-MULTICERT_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с сервером DTLS
                add_custom_command(TARGET "${SERVER_APP_DTLS_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${SERVER_APP_DTLS_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${SERVER_APP_DTLS_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с сервером DTLS мультисертификатным
                add_custom_command(TARGET "${SERVER_APP_DTLS-MULTICERT_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${SERVER_APP_DTLS-MULTICERT_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${SERVER_APP_DTLS-MULTICERT_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Если сборка производится под операционную систему FreeBSD или Linux
                if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
                    # Выполняем сборку приложения работы с сервером SCTP
                    add_custom_command(TARGET "${SERVER_APP_SCTP_NAME}" POST_BUILD
                        COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${SERVER_APP_SCTP_NAME}>" "${entitlements_file}"
                        COMMENT "Sign target \"${SERVER_APP_SCTP_NAME}\" to enable core dump on macOS"
                        VERBATIM
                    )

                    # Выполняем сборку приложения работы с сервером SCTP версии 2
                    add_custom_command(TARGET "${SERVER_APP_SCTP-2_NAME}" POST_BUILD
                        COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${SERVER_APP_SCTP-2_NAME}>" "${entitlements_file}"
                        COMMENT "Sign target \"${SERVER_APP_SCTP-2_NAME}\" to enable core dump on macOS"
                        VERBATIM
                    )
                endif()
            endif (${CMAKE_BUILD_SERVER_APP_SAMPLE})

            # Если нужно собрать семплы для работы с юнитом VPN
            if (${CMAKE_BUILD_VPN_APP_SAMPLE})
                # Выполняем сборку приложения работы с юнитом VPN клиентом
                add_custom_command(TARGET "${CLIENT_APP_VPN_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_VPN_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CLIENT_APP_VPN_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с юнитом VPN сервером
                add_custom_command(TARGET "${SERVER_APP_VPN_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${SERVER_APP_VPN_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${SERVER_APP_VPN_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_VPN_APP_SAMPLE})

            # Если нужно собрать семплы для работы с юнитом SOCKS5
            if (${CMAKE_BUILD_SOCKS5_APP_SAMPLE})
                # Выполняем сборку приложения работы с юнитом SOCKS5 клиентом
                add_custom_command(TARGET "${CLIENT_APP_SOCKS5_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_SOCKS5_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CLIENT_APP_SOCKS5_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с юнитом SOCKS5 клиентом для UDP
                add_custom_command(TARGET "${CLIENT_APP_SOCKS5-UDP_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_SOCKS5-UDP_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CLIENT_APP_SOCKS5-UDP_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с юнитом SOCKS5 клиентом для DTLS
                add_custom_command(TARGET "${CLIENT_APP_SOCKS5-DTLS_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_SOCKS5-DTLS_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CLIENT_APP_SOCKS5-DTLS_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с юнитом SOCKS5 сервером
                add_custom_command(TARGET "${SERVER_APP_SOCKS5_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${SERVER_APP_SOCKS5_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${SERVER_APP_SOCKS5_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_SOCKS5_APP_SAMPLE})

            # Если нужно собрать семплы для работы с юнитом файловой системы
            if (${CMAKE_BUILD_UNIT_FS_APP_SAMPLE})
                # Выполняем сборку приложения работы с юнитом файловой системы
                add_custom_command(TARGET "${UNIT_APP_FS_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${UNIT_APP_FS_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${UNIT_APP_FS_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_UNIT_FS_APP_SAMPLE})

            # Если нужно собрать семплы для работы с юнитом резолвером доменных имён
            if (${CMAKE_BUILD_UNIT_DNS_APP_SAMPLE})
                # Выполняем сборку приложения работы с юнитом резолвером доменных имён
                add_custom_command(TARGET "${UNIT_APP_DNS_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${UNIT_APP_DNS_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${UNIT_APP_DNS_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_UNIT_DNS_APP_SAMPLE})

            # Если нужно собрать семплы для работы с юнитом синхронизации времени NTP
            if (${CMAKE_BUILD_UNIT_NTP_APP_SAMPLE})
                # Выполняем сборку приложения работы с юнитом синхронизации времени NTP
                add_custom_command(TARGET "${UNIT_APP_NTP_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${UNIT_APP_NTP_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${UNIT_APP_NTP_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_UNIT_NTP_APP_SAMPLE})

            # Если нужно собрать семплы для работы с юнитом ICMP-клиентом
            if (${CMAKE_BUILD_UNIT_ICMP_APP_SAMPLE})
                # Выполняем сборку приложения работы с юнитом ICMP-клиентом
                add_custom_command(TARGET "${UNIT_APP_ICMP_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${UNIT_APP_ICMP_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${UNIT_APP_ICMP_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_UNIT_ICMP_APP_SAMPLE})

            # Если нужно собрать семплы для работы с юнитом таймера
            if (${CMAKE_BUILD_UNIT_TIMER_APP_SAMPLE})
                # Выполняем сборку приложения работы с юнитом таймера
                add_custom_command(TARGET "${UNIT_APP_TIMER_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${UNIT_APP_TIMER_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${UNIT_APP_TIMER_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_UNIT_TIMER_APP_SAMPLE})

            # Если нужно собрать семплы для работы с юнитом кластера
            if (${CMAKE_BUILD_UNIT_CLUSTER_APP_SAMPLE})
                # Выполняем сборку приложения работы с юнитом кластера
                add_custom_command(TARGET "${UNIT_APP_CLUSTER_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${UNIT_APP_CLUSTER_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${UNIT_APP_CLUSTER_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_UNIT_CLUSTER_APP_SAMPLE})

            # Если нужно собрать семплы для работы с юнитом уведомителя
            if (${CMAKE_BUILD_UNIT_NOTIFIER_APP_SAMPLE})
                # Выполняем сборку приложения работы с юнитом уведомителя
                add_custom_command(TARGET "${UNIT_APP_NOTIFIER_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${UNIT_APP_NOTIFIER_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${UNIT_APP_NOTIFIER_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_UNIT_NOTIFIER_APP_SAMPLE})
            
            # Если нужно собрать семплы для работы с IP-адресами
            if (${CMAKE_BUILD_IP_NET_APP_SAMPLE})
                # Выполняем сборку приложения работы с IP-адресами
                add_custom_command(TARGET "${IP_APP_NET_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${IP_APP_NET_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${IP_APP_NET_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_IP_NET_APP_SAMPLE})

            # Если нужно собрать семплы для работы с URI
            if (${CMAKE_BUILD_URI_NET_APP_SAMPLE})
                # Выполняем сборку приложения работы с URI
                add_custom_command(TARGET "${URI_APP_NET_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${URI_APP_NET_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${URI_APP_NET_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_URI_NET_APP_SAMPLE})

            # Если нужно собрать семплы для работы с Ethernet-адресами
            if (${CMAKE_BUILD_ETH_NET_APP_SAMPLE})
                # Выполняем сборку приложения работы с Ethernet-адресами
                add_custom_command(TARGET "${ETH_APP_NET_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${ETH_APP_NET_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${ETH_APP_NET_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_ETH_NET_APP_SAMPLE})

            # Если нужно собрать семплы для работы с датой и временем
            if (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})
                # Выполняем сборку приложения даты и времени
                add_custom_command(TARGET "${DATE_APP_SYS_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${DATE_APP_SYS_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${DATE_APP_SYS_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})

            # Если нужно собрать семплы для работы с бинарным контейнером
            if (${CMAKE_BUILD_BINBOX_SYS_APP_SAMPLE})
                # Выполняем сборку приложения работы с бинарным контейнером
                add_custom_command(TARGET "${BINBOX_APP_SYS_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${BINBOX_APP_SYS_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${BINBOX_APP_SYS_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_BINBOX_SYS_APP_SAMPLE})

            # Если нужно собрать семплы для работы с файловой системой
            if (${CMAKE_BUILD_FILESYSTEM_SYS_APP_SAMPLE})
                # Выполняем сборку приложения файловой системы
                add_custom_command(TARGET "${FILESYSTEM_APP_SYS_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${FILESYSTEM_APP_SYS_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${FILESYSTEM_APP_SYS_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_FILESYSTEM_SYS_APP_SAMPLE})

            # Если нужно собрать семплы для работы с резольвером процессов
            if (${CMAKE_BUILD_PROCRE_SYS_APP_SAMPLE})
                # Выполняем сборку приложения резольвера процессов
                add_custom_command(TARGET "${PROCRE_APP_SYS_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${PROCRE_APP_SYS_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${PROCRE_APP_SYS_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_PROCRE_SYS_APP_SAMPLE})

            # Если нужно собрать семплы для работы с шифрованием и хэшированием
            if (${CMAKE_BUILD_CRYPTO_SYS_APP_SAMPLE})
                # Выполняем сборку приложения шифрования и хэширования
                add_custom_command(TARGET "${CRYPTO_APP_SYS_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CRYPTO_APP_SYS_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CRYPTO_APP_SYS_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_CRYPTO_SYS_APP_SAMPLE})

            # Если нужно собрать семплы для работы с HTTP-авторизацией
            if (${CMAKE_BUILD_AUTH_PROTO_APP_SAMPLE})
                # Выполняем сборку приложения работы с HTTP-авторизацией
                add_custom_command(TARGET "${AUTH_APP_PROTO_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${AUTH_APP_PROTO_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${AUTH_APP_PROTO_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_AUTH_PROTO_APP_SAMPLE})

            # Если нужно собрать семплы для работы с HTTP-заголовками
            if (${CMAKE_BUILD_HEADERS_PROTO_APP_SAMPLE})
                # Выполняем сборку приложения работы с HTTP-заголовками
                add_custom_command(TARGET "${HEADERS_APP_PROTO_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${HEADERS_APP_PROTO_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${HEADERS_APP_PROTO_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_HEADERS_PROTO_APP_SAMPLE})

            # Если нужно собрать семплы для работы с парсером HTTP/1.0 и HTTP/1.1
            if (${CMAKE_BUILD_PARSER_PROTO_APP_SAMPLE})
                # Выполняем сборку приложения работы с парсером HTTP/1.0 и HTTP/1.1
                add_custom_command(TARGET "${PARSER_APP_PROTO_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${PARSER_APP_PROTO_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${PARSER_APP_PROTO_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_PARSER_PROTO_APP_SAMPLE})

            # Если нужно собрать семплы для работы с парсером HTTP/2
            if (${CMAKE_BUILD_PARSER2_PROTO_APP_SAMPLE})
                # Выполняем сборку приложения работы с парсером HTTP/2
                add_custom_command(TARGET "${PARSER2_APP_PROTO_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${PARSER2_APP_PROTO_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${PARSER2_APP_PROTO_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_PARSER2_PROTO_APP_SAMPLE})

            # Если нужно собрать семплы для работы с протоколом SOCKS5
            if (${CMAKE_BUILD_SOCKS5_PROTO_APP_SAMPLE})
                # Выполняем сборку приложения работы с протоколом SOCKS5
                add_custom_command(TARGET "${SOCKS5_APP_PROTO_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${SOCKS5_APP_PROTO_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${SOCKS5_APP_PROTO_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_SOCKS5_PROTO_APP_SAMPLE})

            # Если нужно собрать семплы для работы с протоколом QUIC
            if (${CMAKE_BUILD_QUIC_PROTO_APP_SAMPLE})
                # Выполняем сборку приложения работы с QUIC клиентом
                add_custom_command(TARGET "${QUIC_CLIENT_APP_PROTO_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${QUIC_CLIENT_APP_PROTO_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${QUIC_CLIENT_APP_PROTO_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с QUIC сервером
                add_custom_command(TARGET "${QUIC_SERVER_APP_PROTO_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${QUIC_SERVER_APP_PROTO_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${QUIC_SERVER_APP_PROTO_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_QUIC_PROTO_APP_SAMPLE})

            # Если нужно собрать семплы для работы с компрессией данных
            if (${CMAKE_BUILD_COMPRESSOR_SYS_APP_SAMPLE})
                # Выполняем сборку приложения блочной компрессии данных
                add_custom_command(TARGET "${COMPRESSOR_APP_BLOCK_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${COMPRESSOR_APP_BLOCK_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${COMPRESSOR_APP_BLOCK_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
                # Выполняем сборку приложения потоковой компрессии данных
                add_custom_command(TARGET "${COMPRESSOR_APP_STREAM_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${COMPRESSOR_APP_STREAM_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${COMPRESSOR_APP_STREAM_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_COMPRESSOR_SYS_APP_SAMPLE})

            # Если нужно собрать семплы для работы с каталогами
            if (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})
                # Выполняем сборку приложения работы с каталогами
                add_custom_command(TARGET "${DIR_APP_IO_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${DIR_APP_IO_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${DIR_APP_IO_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})

            # Если нужно собрать семплы для работы с файлами
            if (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})
                # Выполняем сборку приложения работы с файлами
                add_custom_command(TARGET "${FILE_APP_IO_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${FILE_APP_IO_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${FILE_APP_IO_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})

            # Если нужно собрать семплы для работы с пингами (ICMP)
            if (${CMAKE_BUILD_PING_IO_APP_SAMPLE})
                # Выполняем сборку приложения работы с пингами (ICMP)
                add_custom_command(TARGET "${PING_APP_IO_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${PING_APP_IO_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${PING_APP_IO_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_PING_IO_APP_SAMPLE})

            # Если нужно собрать семплы для работы с таймером
            if (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})
                # Выполняем сборку приложения таймера
                add_custom_command(TARGET "${TIMER_APP_IO_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${TIMER_APP_IO_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${TIMER_APP_IO_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})

            # Если нужно собрать семплы для работы с пользовательскими событиями
            if (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})
                # Выполняем сборку приложения пользовательских событий
                add_custom_command(TARGET "${EVENTS_APP_IO_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${EVENTS_APP_IO_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${EVENTS_APP_IO_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})

            # Если нужно собрать семплы для работы с межпроцессорным взаимодействием
            if (${CMAKE_BUILD_IPC_IO_APP_SAMPLE})
                # Выполняем сборку приложения работы с межпроцессорным взаимодействием (именованный канал)
                add_custom_command(TARGET "${IPC_PIPE_APP_IO_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${IPC_PIPE_APP_IO_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${IPC_PIPE_APP_IO_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с межпроцессорным взаимодействием (поток)
                add_custom_command(TARGET "${IPC_STREAM_APP_IO_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${IPC_STREAM_APP_IO_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${IPC_STREAM_APP_IO_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            
                # Выполняем сборку приложения работы с межпроцессорным взаимодействием (датаграмма)
                add_custom_command(TARGET "${IPC_DATAGRAM_APP_IO_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${IPC_DATAGRAM_APP_IO_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${IPC_DATAGRAM_APP_IO_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с межпроцессорным взаимодействием (последовательная упаковка)
                add_custom_command(TARGET "${IPC_SEQPACKET_APP_IO_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${IPC_SEQPACKET_APP_IO_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${IPC_SEQPACKET_APP_IO_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_IPC_IO_APP_SAMPLE})

            # Если нужно собрать семплы для работы с TCP (клиент и сервер)
            if (${CMAKE_BUILD_NET_TCP_APP_SAMPLE})
                # Выполняем сборку приложения работы с TCP клиентом
                add_custom_command(TARGET "${CLIENT_APP_NET_TCP_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_NET_TCP_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CLIENT_APP_NET_TCP_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с TCP сервером
                add_custom_command(TARGET "${SERVER_APP_NET_TCP_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${SERVER_APP_NET_TCP_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${SERVER_APP_NET_TCP_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_NET_TCP_APP_SAMPLE})

            # Если нужно собрать семплы для работы с UDP (клиент и сервер)
            if (${CMAKE_BUILD_NET_UDP_APP_SAMPLE})
                # Выполняем сборку приложения работы с UDP клиентом
                add_custom_command(TARGET "${CLIENT_APP_NET_UDP_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_NET_UDP_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CLIENT_APP_NET_UDP_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с UDP клиентом с подключением
                add_custom_command(TARGET "${CLIENT_APP_NET_UDP_CONN_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_NET_UDP_CONN_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CLIENT_APP_NET_UDP_CONN_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с UDP сервером
                add_custom_command(TARGET "${SERVER_APP_NET_UDP_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${SERVER_APP_NET_UDP_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${SERVER_APP_NET_UDP_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_NET_UDP_APP_SAMPLE})

            # Если нужно собрать семплы для работы с ограничителем пропускной способности (клиент и сервер)
            if (${CMAKE_BUILD_NET_BANDWIDTH_APP_SAMPLE})
                # Выполняем сборку приложения работы с ограничителем пропускной способности TCP клиента
                add_custom_command(TARGET "${CLIENT_APP_NET_TCP_BANDWIDTH_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_NET_TCP_BANDWIDTH_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CLIENT_APP_NET_TCP_BANDWIDTH_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с ограничителем пропускной способности TCP сервера
                add_custom_command(TARGET "${SERVER_APP_NET_TCP_BANDWIDTH_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${SERVER_APP_NET_TCP_BANDWIDTH_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${SERVER_APP_NET_TCP_BANDWIDTH_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_NET_BANDWIDTH_APP_SAMPLE})

            # Если нужно собрать семплы для работы с TUN (клиент и сервер)
            if (${CMAKE_BUILD_NET_TUN_APP_SAMPLE})
                # Выполняем сборку приложения работы с TUN клиентом
                add_custom_command(TARGET "${CLIENT_APP_NET_TUN_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_NET_TUN_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CLIENT_APP_NET_TUN_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с TUN сервером
                add_custom_command(TARGET "${SERVER_APP_NET_TUN_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${SERVER_APP_NET_TUN_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${SERVER_APP_NET_TUN_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_NET_TUN_APP_SAMPLE})

            # Если нужно собрать семплы для работы с UDP Broadcast (клиент и сервер)
            if (${CMAKE_BUILD_NET_BROADCAST_APP_SAMPLE})
                # Выполняем сборку приложения работы с UDP Broadcast клиентом
                add_custom_command(TARGET "${CLIENT_APP_NET_BROADCAST_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_NET_BROADCAST_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CLIENT_APP_NET_BROADCAST_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с UDP Broadcast сервером
                add_custom_command(TARGET "${SERVER_APP_NET_BROADCAST_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${SERVER_APP_NET_BROADCAST_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${SERVER_APP_NET_BROADCAST_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_NET_BROADCAST_APP_SAMPLE})

            # Если нужно собрать семплы для работы с UDP Multicast (клиент и сервер)
            if (${CMAKE_BUILD_NET_MULTICAST_APP_SAMPLE})
                # Выполняем сборку приложения работы с UDP Multicast клиентом сценарий 1
                add_custom_command(TARGET "${CLIENT_APP_NET_MULTICAST1_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_NET_MULTICAST1_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CLIENT_APP_NET_MULTICAST1_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с UDP Multicast сервером сценарий 1
                add_custom_command(TARGET "${SERVER_APP_NET_MULTICAST1_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${SERVER_APP_NET_MULTICAST1_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${SERVER_APP_NET_MULTICAST1_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с UDP Multicast клиентом сценарий 3
                add_custom_command(TARGET "${CLIENT_APP_NET_MULTICAST3_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_NET_MULTICAST3_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CLIENT_APP_NET_MULTICAST3_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с UDP Multicast сервером сценарий 3
                add_custom_command(TARGET "${SERVER_APP_NET_MULTICAST3_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${SERVER_APP_NET_MULTICAST3_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${SERVER_APP_NET_MULTICAST3_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_NET_MULTICAST_APP_SAMPLE})

            # Если нужно собрать семплы для работы с UDP Splice (клиент и сервер)
            if (${CMAKE_BUILD_NET_SPLICE_APP_SAMPLE})
                # Выполняем сборку приложения работы с UDP Splice клиентом
                add_custom_command(TARGET "${CLIENT_APP_NET_SPLICE_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_NET_SPLICE_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CLIENT_APP_NET_SPLICE_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с UDP Splice сервером
                add_custom_command(TARGET "${SERVER_APP_NET_SPLICE_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${SERVER_APP_NET_SPLICE_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${SERVER_APP_NET_SPLICE_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_NET_SPLICE_APP_SAMPLE})

            # Если нужно собрать семплы для работы с UDS (клиент и сервер)
            if (${CMAKE_BUILD_NET_UDS_APP_SAMPLE})
                # Выполняем сборку приложения работы с UDS-TCP клиентом
                add_custom_command(TARGET "${CLIENT_APP_NET_UDS_TCP_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_NET_UDS_TCP_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CLIENT_APP_NET_UDS_TCP_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с UDS-TCP сервером
                add_custom_command(TARGET "${SERVER_APP_NET_UDS_TCP_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${SERVER_APP_NET_UDS_TCP_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${SERVER_APP_NET_UDS_TCP_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с UDS-UDP клиентом
                add_custom_command(TARGET "${CLIENT_APP_NET_UDS_UDP_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_NET_UDS_UDP_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CLIENT_APP_NET_UDS_UDP_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с UDS-UDP сервером
                add_custom_command(TARGET "${SERVER_APP_NET_UDS_UDP_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${SERVER_APP_NET_UDS_UDP_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${SERVER_APP_NET_UDS_UDP_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_NET_UDS_APP_SAMPLE})

            # Если нужно собрать семплы для работы с DTLS (клиент и сервер)
            if (${CMAKE_BUILD_NET_DTLS_APP_SAMPLE})
                # Выполняем сборку приложения работы с DTLS клиентом
                add_custom_command(TARGET "${CLIENT_APP_NET_DTLS_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_NET_DTLS_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CLIENT_APP_NET_DTLS_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с DTLS сервером
                add_custom_command(TARGET "${SERVER_APP_NET_DTLS_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${SERVER_APP_NET_DTLS_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${SERVER_APP_NET_DTLS_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_NET_DTLS_APP_SAMPLE})

            # Если нужно собрать семплы для работы с TLS (клиент и сервер)
            if (${CMAKE_BUILD_NET_TLS_APP_SAMPLE})
                # Выполняем сборку приложения работы с TLS клиентом
                add_custom_command(TARGET "${CLIENT_APP_NET_TLS_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_NET_TLS_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CLIENT_APP_NET_TLS_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с TLS сервером
                add_custom_command(TARGET "${SERVER_APP_NET_TLS_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${SERVER_APP_NET_TLS_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${SERVER_APP_NET_TLS_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с TLS и HTTP/1.1 клиентом
                add_custom_command(TARGET "${CLIENT_APP_NET_TLS_WEB_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_NET_TLS_WEB_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CLIENT_APP_NET_TLS_WEB_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )

                # Выполняем сборку приложения работы с TLS и мультисертификатом сервером
                add_custom_command(TARGET "${SERVER_APP_NET_TLS_MULTI_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${SERVER_APP_NET_TLS_MULTI_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${SERVER_APP_NET_TLS_MULTI_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_NET_TLS_APP_SAMPLE})
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

            # Если нужно собрать семплы для работы с клиентом
            if (${CMAKE_BUILD_CLIENT_APP_SAMPLE})
                # Выполняем анализ приложения работы с клиентом UDS
                pvs_studio_add_target(TARGET ${CLIENT_APP_UDS_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_UDS_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с клиентом UDS версия 2
                pvs_studio_add_target(TARGET ${CLIENT_APP_UDS-2_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_UDS-2_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с клиентом TCP
                pvs_studio_add_target(TARGET ${CLIENT_APP_TCP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_TCP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с клиентом TCP версия 2
                pvs_studio_add_target(TARGET ${CLIENT_APP_TCP-2_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_TCP-2_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с клиентом UDP
                pvs_studio_add_target(TARGET ${CLIENT_APP_UDP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_UDP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с клиентом UDP версия 2
                pvs_studio_add_target(TARGET ${CLIENT_APP_UDP-2_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_UDP-2_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с клиентом TLS
                pvs_studio_add_target(TARGET ${CLIENT_APP_TLS_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_TLS_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с клиентом TLS версия 2
                pvs_studio_add_target(TARGET ${CLIENT_APP_TLS-2_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_TLS-2_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с клиентом DTLS
                pvs_studio_add_target(TARGET ${CLIENT_APP_DTLS_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_DTLS_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с клиентом DTLS версия 2
                pvs_studio_add_target(TARGET ${CLIENT_APP_DTLS-2_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_DTLS-2_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с клиентом TLS-Web
                pvs_studio_add_target(TARGET ${CLIENT_APP_TLS-WEB_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_TLS-WEB_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Если сборка производится под операционную систему FreeBSD или Linux
                if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
                    # Выполняем анализ приложения работы с клиентом SCTP
                    pvs_studio_add_target(TARGET ${CLIENT_APP_SCTP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_SCTP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                    # Выполняем анализ приложения работы с клиентом SCTP версия 2
                    pvs_studio_add_target(TARGET ${CLIENT_APP_SCTP-2_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_SCTP-2_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                endif()
            endif (${CMAKE_BUILD_CLIENT_APP_SAMPLE})

            # Если нужно собрать семплы для работы с сервером
            if (${CMAKE_BUILD_SERVER_APP_SAMPLE})
                # Выполняем анализ приложения работы с сервером UDS
                pvs_studio_add_target(TARGET ${SERVER_APP_UDS_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_UDS_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с сервером UDS версия 2
                pvs_studio_add_target(TARGET ${SERVER_APP_UDS-2_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_UDS-2_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с сервером TCP
                pvs_studio_add_target(TARGET ${SERVER_APP_TCP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_TCP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с сервером TCP версия 2
                pvs_studio_add_target(TARGET ${SERVER_APP_TCP-2_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_TCP-2_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с сервером UDP
                pvs_studio_add_target(TARGET ${SERVER_APP_UDP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_UDP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с сервером UDP версия 2
                pvs_studio_add_target(TARGET ${SERVER_APP_UDP-2_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_UDP-2_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с сервером TLS
                pvs_studio_add_target(TARGET ${SERVER_APP_TLS_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_TLS_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с сервером TLS мультисертификатным
                pvs_studio_add_target(TARGET ${SERVER_APP_TLS-MULTICERT_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_TLS-MULTICERT_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с сервером DTLS
                pvs_studio_add_target(TARGET ${SERVER_APP_DTLS_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_DTLS_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с сервером DTLS мультисертификатным
                pvs_studio_add_target(TARGET ${SERVER_APP_DTLS-MULTICERT_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_DTLS-MULTICERT_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Если сборка производится под операционную систему FreeBSD или Linux
                if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
                    # Выполняем анализ приложения работы с сервером SCTP
                    pvs_studio_add_target(TARGET ${SERVER_APP_SCTP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_SCTP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                    # Выполняем анализ приложения работы с сервером SCTP версия 2
                    pvs_studio_add_target(TARGET ${SERVER_APP_SCTP-2_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_SCTP-2_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                endif()
            endif (${CMAKE_BUILD_SERVER_APP_SAMPLE})

            # Если нужно собрать семплы для работы с юнитом VPN
            if (${CMAKE_BUILD_VPN_APP_SAMPLE})
                # Выполняем анализ приложения работы с юнитом VPN клиентом
                pvs_studio_add_target(TARGET ${CLIENT_APP_VPN_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_VPN_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с юнитом VPN сервером
                pvs_studio_add_target(TARGET ${SERVER_APP_VPN_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_VPN_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_VPN_APP_SAMPLE})

            # Если нужно собрать семплы для работы с юнитом SOCKS5
            if (${CMAKE_BUILD_SOCKS5_APP_SAMPLE})
                # Выполняем анализ приложения работы с юнитом SOCKS5 клиентом
                pvs_studio_add_target(TARGET ${CLIENT_APP_SOCKS5_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_SOCKS5_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с юнитом SOCKS5 клиентом для UDP
                pvs_studio_add_target(TARGET ${CLIENT_APP_SOCKS5-UDP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_SOCKS5-UDP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с юнитом SOCKS5 клиентом для DTLS
                pvs_studio_add_target(TARGET ${CLIENT_APP_SOCKS5-DTLS_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_SOCKS5-DTLS_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с юнитом SOCKS5 сервером
                pvs_studio_add_target(TARGET ${SERVER_APP_SOCKS5_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_SOCKS5_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_SOCKS5_APP_SAMPLE})

            # Если нужно собрать семплы для работы с юнитом файловой системы
            if (${CMAKE_BUILD_UNIT_FS_APP_SAMPLE})
                # Выполняем анализ приложения работы с юнитом файловой системы
                pvs_studio_add_target(TARGET ${UNIT_APP_FS_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${UNIT_APP_FS_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_UNIT_FS_APP_SAMPLE})

            # Если нужно собрать семплы для работы с юнитом резолвером доменных имён
            if (${CMAKE_BUILD_UNIT_DNS_APP_SAMPLE})
                # Выполняем анализ приложения работы с юнитом резолвером доменных имён
                pvs_studio_add_target(TARGET ${UNIT_APP_DNS_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${UNIT_APP_DNS_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_UNIT_DNS_APP_SAMPLE})

            # Если нужно собрать семплы для работы с юнитом синхронизации времени NTP
            if (${CMAKE_BUILD_UNIT_NTP_APP_SAMPLE})
                # Выполняем анализ приложения работы с юнитом синхронизации времени NTP
                pvs_studio_add_target(TARGET ${UNIT_APP_NTP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${UNIT_APP_NTP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_UNIT_NTP_APP_SAMPLE})

            # Если нужно собрать семплы для работы с юнитом ICMP-клиентом
            if (${CMAKE_BUILD_UNIT_ICMP_APP_SAMPLE})
                # Выполняем анализ приложения работы с юнитом ICMP-клиентом
                pvs_studio_add_target(TARGET ${UNIT_APP_ICMP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${UNIT_APP_ICMP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_UNIT_ICMP_APP_SAMPLE})

            # Если нужно собрать семплы для работы с юнитом таймера
            if (${CMAKE_BUILD_UNIT_TIMER_APP_SAMPLE})
                # Выполняем анализ приложения работы с юнитом таймера
                pvs_studio_add_target(TARGET ${UNIT_APP_TIMER_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${UNIT_APP_TIMER_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_UNIT_TIMER_APP_SAMPLE})

            # Если нужно собрать семплы для работы с юнитом кластера
            if (${CMAKE_BUILD_UNIT_CLUSTER_APP_SAMPLE})
                # Выполняем анализ приложения работы с юнитом кластера
                pvs_studio_add_target(TARGET ${UNIT_APP_CLUSTER_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${UNIT_APP_CLUSTER_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_UNIT_CLUSTER_APP_SAMPLE})

            # Если нужно собрать семплы для работы с юнитом уведомителя
            if (${CMAKE_BUILD_UNIT_NOTIFIER_APP_SAMPLE})
                # Выполняем анализ приложения работы с юнитом уведомителя
                pvs_studio_add_target(TARGET ${UNIT_APP_NOTIFIER_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${UNIT_APP_NOTIFIER_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_UNIT_NOTIFIER_APP_SAMPLE})

            # Если нужно собрать семплы для работы с IP-адресами
            if (${CMAKE_BUILD_IP_NET_APP_SAMPLE})
                # Выполняем анализ приложения работы с IP-адресами
                pvs_studio_add_target(TARGET ${IP_APP_NET_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${IP_APP_NET_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_IP_NET_APP_SAMPLE})

            # Если нужно собрать семплы для работы с URI
            if (${CMAKE_BUILD_URI_NET_APP_SAMPLE})
                # Выполняем анализ приложения работы с URI
                pvs_studio_add_target(TARGET ${URI_APP_NET_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${URI_APP_NET_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_URI_NET_APP_SAMPLE})

            # Если нужно собрать семплы для работы с Ethernet-адресами
            if (${CMAKE_BUILD_ETH_NET_APP_SAMPLE})
                # Выполняем анализ приложения работы с Ethernet-адресами
                pvs_studio_add_target(TARGET ${ETH_APP_NET_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${ETH_APP_NET_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_ETH_NET_APP_SAMPLE})

            # Если нужно собрать семплы для работы с датой и временем
            if (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})
                # Выполняем анализ приложения даты и времени
                pvs_studio_add_target(TARGET ${DATE_APP_SYS_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${DATE_APP_SYS_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})

            # Если нужно собрать семплы для работы с бинарным контейнером
            if (${CMAKE_BUILD_BINBOX_SYS_APP_SAMPLE})
                # Выполняем анализ приложения работы с бинарным контейнером
                pvs_studio_add_target(TARGET ${BINBOX_APP_SYS_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${BINBOX_APP_SYS_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_BINBOX_SYS_APP_SAMPLE})

            # Если нужно собрать семплы для работы с файловой системой
            if (${CMAKE_BUILD_FILESYSTEM_SYS_APP_SAMPLE})
                # Выполняем анализ приложения файловой системы
                pvs_studio_add_target(TARGET ${FILESYSTEM_APP_SYS_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${FILESYSTEM_APP_SYS_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_FILESYSTEM_SYS_APP_SAMPLE})

            # Если нужно собрать семплы для работы с резольвером процессов
            if (${CMAKE_BUILD_PROCRE_SYS_APP_SAMPLE})
                # Выполняем анализ приложения файловой системы
                pvs_studio_add_target(TARGET ${PROCRE_APP_SYS_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${PROCRE_APP_SYS_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_PROCRE_SYS_APP_SAMPLE})

            # Если нужно собрать семплы для работы с шифрованием и хэшированием
            if (${CMAKE_BUILD_CRYPTO_SYS_APP_SAMPLE})
                # Выполняем анализ приложения даты и времени
                pvs_studio_add_target(TARGET ${CRYPTO_APP_SYS_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CRYPTO_APP_SYS_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_CRYPTO_SYS_APP_SAMPLE})

            # Если нужно собрать семплы для работы с компрессией данных
            if (${CMAKE_BUILD_COMPRESSOR_SYS_APP_SAMPLE})
                # Выполняем анализ приложения блочной компрессии данных
                pvs_studio_add_target(TARGET ${COMPRESSOR_APP_BLOCK_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${COMPRESSOR_APP_BLOCK_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения потоковой компрессии данных
                pvs_studio_add_target(TARGET ${COMPRESSOR_APP_STREAM_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${COMPRESSOR_APP_STREAM_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_COMPRESSOR_SYS_APP_SAMPLE})

            # Если нужно собрать семплы для работы с каталогами
            if (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})
                # Выполняем анализ приложения работы с каталогами
                pvs_studio_add_target(TARGET ${DIR_APP_IO_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${DIR_APP_IO_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})

            # Если нужно собрать семплы для работы с файлами
            if (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})
                # Выполняем анализ приложения работы с файлами
                pvs_studio_add_target(TARGET ${FILE_APP_IO_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${FILE_APP_IO_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})

            # Если нужно собрать семплы для работы с пингами (ICMP)
            if (${CMAKE_BUILD_PING_IO_APP_SAMPLE})
                # Выполняем анализ приложения пинга (ICMP)
                pvs_studio_add_target(TARGET ${PING_APP_IO_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${PING_APP_IO_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_PING_IO_APP_SAMPLE})

            # Если нужно собрать семплы для работы с таймером
            if (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})
                # Выполняем анализ приложения таймера
                pvs_studio_add_target(TARGET ${TIMER_APP_IO_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${TIMER_APP_IO_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})

            # Если нужно собрать семплы для работы с пользовательскими событиями
            if (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})
                # Выполняем анализ приложения пользовательских событий
                pvs_studio_add_target(TARGET ${EVENTS_APP_IO_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${EVENTS_APP_IO_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})

            # Если нужно собрать семплы для работы с межпроцессорным взаимодействием
            if (${CMAKE_BUILD_IPC_IO_APP_SAMPLE})
                # Выполняем анализ приложения работы с межпроцессорным взаимодействием (именованный канал)
                pvs_studio_add_target(TARGET ${IPC_PIPE_APP_IO_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${IPC_PIPE_APP_IO_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с межпроцессорным взаимодействием (поток)
                pvs_studio_add_target(TARGET ${IPC_STREAM_APP_IO_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${IPC_STREAM_APP_IO_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с межпроцессорным взаимодействием (датаграмма)
                pvs_studio_add_target(TARGET ${IPC_DATAGRAM_APP_IO_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${IPC_DATAGRAM_APP_IO_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с межпроцессорным взаимодействием (последовательная упаковка)
                pvs_studio_add_target(TARGET ${IPC_SEQPACKET_APP_IO_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${IPC_SEQPACKET_APP_IO_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_IPC_IO_APP_SAMPLE})

            # Если нужно собрать семплы для работы с TCP (клиент и сервер)
            if (${CMAKE_BUILD_NET_TCP_APP_SAMPLE})
                # Выполняем анализ приложения работы с TCP клиентом
                pvs_studio_add_target(TARGET ${CLIENT_APP_NET_TCP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_NET_TCP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с TCP сервером
                pvs_studio_add_target(TARGET ${SERVER_APP_NET_TCP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_NET_TCP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_NET_TCP_APP_SAMPLE})

            # Если нужно собрать семплы для работы с UDP (клиент и сервер)
            if (${CMAKE_BUILD_NET_UDP_APP_SAMPLE})
                # Выполняем анализ приложения работы с UDP клиентом
                pvs_studio_add_target(TARGET ${CLIENT_APP_NET_UDP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_NET_UDP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с UDP клиентом с подключением
                pvs_studio_add_target(TARGET ${CLIENT_APP_NET_UDP_CONN_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_NET_UDP_CONN_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с UDP сервером
                pvs_studio_add_target(TARGET ${SERVER_APP_NET_UDP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_NET_UDP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_NET_UDP_APP_SAMPLE})

            # Если нужно собрать семплы для работы с ограничителем пропускной способности (клиент и сервер)
            if (${CMAKE_BUILD_NET_BANDWIDTH_APP_SAMPLE})
               # Выполняем анализ приложения работы с ограничителем пропускной способности TCP клиента
                pvs_studio_add_target(TARGET ${CLIENT_APP_NET_TCP_BANDWIDTH_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_NET_TCP_BANDWIDTH_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с ограничителем пропускной способности TCP сервера
                pvs_studio_add_target(TARGET ${SERVER_APP_NET_TCP_BANDWIDTH_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_NET_TCP_BANDWIDTH_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_NET_BANDWIDTH_APP_SAMPLE})

            # Если нужно собрать семплы для работы с TUN (клиент и сервер)
            if (${CMAKE_BUILD_NET_TUN_APP_SAMPLE})
                # Выполняем анализ приложения работы с TUN клиентом
                pvs_studio_add_target(TARGET ${CLIENT_APP_NET_TUN_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_NET_TUN_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с TUN сервером
                pvs_studio_add_target(TARGET ${SERVER_APP_NET_TUN_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_NET_TUN_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_NET_TUN_APP_SAMPLE})

            # Если нужно собрать семплы для работы с UDP Broadcast (клиент и сервер)
            if (${CMAKE_BUILD_NET_BROADCAST_APP_SAMPLE})
                # Выполняем анализ приложения работы с UDP Broadcast клиентом
                pvs_studio_add_target(TARGET ${CLIENT_APP_NET_BROADCAST_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_NET_BROADCAST_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с UDP Broadcast сервером
                pvs_studio_add_target(TARGET ${SERVER_APP_NET_BROADCAST_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_NET_BROADCAST_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_NET_BROADCAST_APP_SAMPLE})

            # Если нужно собрать семплы для работы с UDP Multicast (клиент и сервер)
            if (${CMAKE_BUILD_NET_MULTICAST_APP_SAMPLE})
                # Выполняем анализ приложения работы с UDP Multicast клиентом сценарий 1
                pvs_studio_add_target(TARGET ${CLIENT_APP_NET_MULTICAST1_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_NET_MULTICAST1_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с UDP Multicast сервером сценарий 1
                pvs_studio_add_target(TARGET ${SERVER_APP_NET_MULTICAST1_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_NET_MULTICAST1_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с UDP Multicast клиентом сценарий 3
                pvs_studio_add_target(TARGET ${CLIENT_APP_NET_MULTICAST3_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_NET_MULTICAST3_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с UDP Multicast сервером сценарий 3
                pvs_studio_add_target(TARGET ${SERVER_APP_NET_MULTICAST3_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_NET_MULTICAST3_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_NET_MULTICAST_APP_SAMPLE})

            # Если нужно собрать семплы для работы с UDP Splice (клиент и сервер)
            if (${CMAKE_BUILD_NET_SPLICE_APP_SAMPLE})
                # Выполняем анализ приложения работы с UDP Splice клиентом
                pvs_studio_add_target(TARGET ${CLIENT_APP_NET_SPLICE_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_NET_SPLICE_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с UDP Splice сервером
                pvs_studio_add_target(TARGET ${SERVER_APP_NET_SPLICE_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_NET_SPLICE_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_NET_SPLICE_APP_SAMPLE})

            # Если нужно собрать семплы для работы с UDS (клиент и сервер)
            if (${CMAKE_BUILD_NET_UDS_APP_SAMPLE})
                # Выполняем анализ приложения работы с UDS-TCP клиентом
                pvs_studio_add_target(TARGET ${CLIENT_APP_NET_UDS_TCP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_NET_UDS_TCP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с UDS-TCP сервером
                pvs_studio_add_target(TARGET ${SERVER_APP_NET_UDS_TCP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_NET_UDS_TCP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с UDS-UDP клиентом
                pvs_studio_add_target(TARGET ${CLIENT_APP_NET_UDS_UDP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_NET_UDS_UDP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с UDS-UDP сервером
                pvs_studio_add_target(TARGET ${SERVER_APP_NET_UDS_UDP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_NET_UDS_UDP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_NET_UDS_APP_SAMPLE})

            # Если нужно собрать семплы для работы с DTLS (клиент и сервер)
            if (${CMAKE_BUILD_NET_DTLS_APP_SAMPLE})
                # Выполняем анализ приложения работы с DTLS клиентом
                pvs_studio_add_target(TARGET ${CLIENT_APP_NET_DTLS_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_NET_DTLS_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с DTLS сервером
                pvs_studio_add_target(TARGET ${SERVER_APP_NET_DTLS_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_NET_DTLS_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_NET_DTLS_APP_SAMPLE})

            # Если нужно собрать семплы для работы с TLS (клиент и сервер)
            if (${CMAKE_BUILD_NET_TLS_APP_SAMPLE})
                # Выполняем анализ приложения работы с TLS клиентом
                pvs_studio_add_target(TARGET ${CLIENT_APP_NET_TLS_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_NET_TLS_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с TLS сервером
                pvs_studio_add_target(TARGET ${SERVER_APP_NET_TLS_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_NET_TLS_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с TLS и HTTP/1.1 клиентом
                pvs_studio_add_target(TARGET ${CLIENT_APP_NET_TLS_WEB_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_NET_TLS_WEB_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                # Выполняем анализ приложения работы с TLS и мультисертификатом сервером
                pvs_studio_add_target(TARGET ${SERVER_APP_NET_TLS_MULTI_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_NET_TLS_MULTI_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_NET_TLS_APP_SAMPLE})

            # Если сборка производится под операционную систему FreeBSD или Linux
            if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
                # Если нужно собрать семплы для работы с SCTP (клиент и сервер)
                if (${CMAKE_BUILD_NET_SCTP_APP_SAMPLE})
                    # Выполняем анализ приложения работы с TLS клиентом
                    pvs_studio_add_target(TARGET ${CLIENT_APP_NET_SCTP_TLS_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_NET_SCTP_TLS_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                    # Выполняем анализ приложения работы с TLS сервером
                    pvs_studio_add_target(TARGET ${SERVER_APP_NET_SCTP_TLS_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_NET_SCTP_TLS_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                    # Выполняем анализ приложения работы с DTLS клиентом
                    pvs_studio_add_target(TARGET ${CLIENT_APP_NET_SCTP_DTLS_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_NET_SCTP_DTLS_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                    # Выполняем анализ приложения работы с DTLS сервером
                    pvs_studio_add_target(TARGET ${SERVER_APP_NET_SCTP_DTLS_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_NET_SCTP_DTLS_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                    # Выполняем анализ приложения работы с клиентом SCTP - AUTH
                    pvs_studio_add_target(TARGET ${CLIENT_APP_NET_SCTP_AUTH_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_NET_SCTP_AUTH_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                    # Выполняем анализ приложения работы с сервером SCTP - AUTH
                    pvs_studio_add_target(TARGET ${SERVER_APP_NET_SCTP_AUTH_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_NET_SCTP_AUTH_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                    # Выполняем анализ приложения работы с клиентом SCTP - STREAM
                    pvs_studio_add_target(TARGET ${CLIENT_APP_NET_SCTP_STREAM_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_NET_SCTP_STREAM_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                    # Выполняем анализ приложения работы с сервером SCTP - STREAM
                    pvs_studio_add_target(TARGET ${SERVER_APP_NET_SCTP_STREAM_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_NET_SCTP_STREAM_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                    # Выполняем анализ приложения работы с клиентом SCTP - SEQPACKET
                    pvs_studio_add_target(TARGET ${CLIENT_APP_NET_SCTP_SEQPACKET_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_NET_SCTP_SEQPACKET_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                    # Выполняем анализ приложения работы с сервером SCTP - SEQPACKET
                    pvs_studio_add_target(TARGET ${SERVER_APP_NET_SCTP_SEQPACKET_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_NET_SCTP_SEQPACKET_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                endif (${CMAKE_BUILD_NET_SCTP_APP_SAMPLE})
            endif()
        endif (CMAKE_ENABLED_PVS)
    endif()
endif (${CMAKE_BUILD_SAMPLES})

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
            COMMAND bash "${CMAKE_SOURCE_DIR}/sh/build_static_lib.sh" "$<TARGET_FILE:${AWH_LIB_NAME}>"
        VERBATIM)
    # Если режим отладки активирован
    else(NOT CMAKE_BUILD_TYPE MATCHES Debug)
        # Выполняем установку библиотеки зависимостей
        install(FILES ${DEPEND_LIBRARY} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
    endif(NOT CMAKE_BUILD_TYPE MATCHES Debug)
endif()

# Если необходимо собрать тесты
if (CMAKE_BUILD_TESTS)
    # Устанавливаем библиотеку
    if (NOT CMAKE_SHARED_BUILD_LIB)
        # Если режим отладки не активирован
        if (NOT CMAKE_BUILD_TYPE MATCHES Debug)
            # Выполняем запуск скрипта объединения библиотек
            add_custom_command(TARGET "${AWH_LIB_GCOV_NAME}" POST_BUILD
                COMMAND bash "${CMAKE_SOURCE_DIR}/sh/build_static_lib.sh" "$<TARGET_FILE:${AWH_LIB_GCOV_NAME}>"
            VERBATIM)
        # Если режим отладки активирован
        else()
            # Выполняем установку библиотеки зависимостей
            install(FILES ${DEPEND_LIBRARY} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
        endif()
    endif()

    # СБорка тестов и бенчмарков
    add_subdirectory(tests)
    # add_subdirectory(benchmark)

    # Если сбор статистики покрытия кода активирован
    if (CMAKE_BUILD_COVERAGE)
        # Ищем утилиту gcovr для формирования отчёта о покрытии
        find_program(GCOVR_EXECUTABLE gcovr)
        # Если утилита gcovr найдена
        if (GCOVR_EXECUTABLE)
            # Если сборка производится под операционную систему macOS
            if (APPLE)
                # На macOS данные покрытия clang разбираются через llvm-cov из состава Xcode
                SET(AWH_GCOV_TOOL "xcrun llvm-cov gcov")
            # Если сборка производится под прочие операционные системы
            else()
                # На остальных системах используется штатный gcov
                SET(AWH_GCOV_TOOL "gcov")
            endif()
            # Каталог для HTML-отчёта о покрытии
            SET(AWH_COVERAGE_REPORT_DIR "${CMAKE_BINARY_DIR}/coverage")
            # Цель формирования отчёта о покрытии (прогон тестов + HTML-отчёт)
            add_custom_target(coverage
                # Запускаем скрипт прогона тестов и формирования отчёта о покрытии
                COMMAND ${CMAKE_COMMAND}
                    -DSRC_DIR=${CMAKE_SOURCE_DIR}
                    -DBIN_DIR=${CMAKE_BINARY_DIR}
                    -DREPORT_DIR=${AWH_COVERAGE_REPORT_DIR}
                    -DGCOVR=${GCOVR_EXECUTABLE}
                    -DGCOV_TOOL=${AWH_GCOV_TOOL}
                    "-DTEST_BINS=$<TARGET_FILE:${PROJECT_NAME}_UNITTEST_net>;$<TARGET_FILE:${PROJECT_NAME}_UNITTEST_sys>;$<TARGET_FILE:${PROJECT_NAME}_UNITTEST_proto>"
                    -P ${CMAKE_SOURCE_DIR}/tests/coverage.cmake
                # Перед формированием отчёта собираем тесты
                DEPENDS ${PROJECT_NAME}_UNITTEST_net ${PROJECT_NAME}_UNITTEST_sys ${PROJECT_NAME}_UNITTEST_proto
                # Рабочий каталог
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                # Описание цели
                COMMENT "Running unit tests and generating code coverage report"
                VERBATIM
            )
            # Цель сброса накопленных счётчиков покрытия
            add_custom_target(coverage-clean
                # Удаляем все накопленные данные покрытия
                COMMAND find ${CMAKE_BINARY_DIR} -name *.gcda -delete
                # Рабочий каталог
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                # Описание цели
                COMMENT "Removing collected coverage counters (*.gcda)"
                VERBATIM
            )
        # Если утилита gcovr не найдена
        else()
            # Выводим сообщение об отсутствии утилиты gcovr
            message(STATUS "gcovr not found: target 'coverage' is disabled\n")
        endif()
    endif()
endif (CMAKE_BUILD_TESTS)

# Если сборка производится под операционную систему Windows
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    # Если собрана динамическая библиотека
    if (CMAKE_SHARED_BUILD_LIB)
        # Устанавливаем статическую библиотеку AWH
        install(TARGETS ${AWH_LIB_NAME}
            # .lib (import) — для линковки
            ARCHIVE DESTINATION lib RENAME ${CMAKE_PROJECT_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX} # awh.lib → lib/libawh.dll.a
            # .dll (Windows) или .so (Linux) — исполняемый модуль
            RUNTIME DESTINATION bin/${CMAKE_PROJECT_NAME} RENAME ${CMAKE_PROJECT_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX} # awh.dll → bin/awh/libawh.dll
        )
    # Если собрана статическая библиотека
    else (CMAKE_SHARED_BUILD_LIB)
        # Устанавливаем статическую библиотеку AWH
        install(TARGETS ${AWH_LIB_NAME} ARCHIVE DESTINATION lib RENAME ${CMAKE_PROJECT_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX})
    endif (CMAKE_SHARED_BUILD_LIB)
# Если сборка производится под операционную систему принадлежащей к Nix-подобной группе (Linux, FreeBSD, Darwin и т.д.)
else()
    # Устанавливаем статическую библиотеку AWH
    install(TARGETS ${AWH_LIB_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
endif()

# Устанавливаем статическую библиотеку TcMalloc
install(FILES ${TCMALLOC_LIBRARIES} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")

# Если необходимо собрать семплы
if (${CMAKE_BUILD_SAMPLES})
    # Если нужно собрать семплы для работы с клиентом
    if (${CMAKE_BUILD_CLIENT_APP_SAMPLE})
        # Устанавливаем приложение работы с клиентом UDS
        install(TARGETS ${CLIENT_APP_UDS_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение работы с клиентом UDS версии 2
        install(TARGETS ${CLIENT_APP_UDS-2_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение работы с клиентом TCP
        install(TARGETS ${CLIENT_APP_TCP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение работы с клиентом TCP версии 2
        install(TARGETS ${CLIENT_APP_TCP-2_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение работы с клиентом UDP
        install(TARGETS ${CLIENT_APP_UDP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение работы с клиентом UDP версии 2
        install(TARGETS ${CLIENT_APP_UDP-2_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение работы с клиентом TLS
        install(TARGETS ${CLIENT_APP_TLS_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение работы с клиентом TLS версии 2
        install(TARGETS ${CLIENT_APP_TLS-2_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение работы с клиентом DTLS
        install(TARGETS ${CLIENT_APP_DTLS_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение работы с клиентом DTLS версии 2
        install(TARGETS ${CLIENT_APP_DTLS-2_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение работы с клиентом TLS-Web
        install(TARGETS ${CLIENT_APP_TLS-WEB_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Если сборка производится под операционную систему FreeBSD или Linux
        if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
            # Устанавливаем приложение работы с клиентом SCTP
            install(TARGETS ${CLIENT_APP_SCTP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
            # Устанавливаем приложение работы с клиентом SCTP версии 2
            install(TARGETS ${CLIENT_APP_SCTP-2_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        endif()
    endif (${CMAKE_BUILD_CLIENT_APP_SAMPLE})

    # Если нужно собрать семплы для работы с сервером
    if (${CMAKE_BUILD_SERVER_APP_SAMPLE})
        # Устанавливаем приложение работы с сервером UDS
        install(TARGETS ${SERVER_APP_UDS_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение работы с сервером UDS версии 2
        install(TARGETS ${SERVER_APP_UDS-2_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение работы с сервером TCP
        install(TARGETS ${SERVER_APP_TCP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение работы с сервером TCP версии 2
        install(TARGETS ${SERVER_APP_TCP-2_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение работы с сервером UDP
        install(TARGETS ${SERVER_APP_UDP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение работы с сервером UDP версии 2
        install(TARGETS ${SERVER_APP_UDP-2_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение работы с сервером TLS
        install(TARGETS ${SERVER_APP_TLS_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение работы с сервером TLS мультисертификатным
        install(TARGETS ${SERVER_APP_TLS-MULTICERT_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение работы с сервером DTLS
        install(TARGETS ${SERVER_APP_DTLS_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение работы с сервером DTLS мультисертификатным
        install(TARGETS ${SERVER_APP_DTLS-MULTICERT_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Если сборка производится под операционную систему FreeBSD или Linux
        if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
            # Устанавливаем приложение работы с сервером SCTP
            install(TARGETS ${SERVER_APP_SCTP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
            # Устанавливаем приложение работы с сервером SCTP версии 2
            install(TARGETS ${SERVER_APP_SCTP-2_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        endif()
    endif (${CMAKE_BUILD_SERVER_APP_SAMPLE})

    # Если нужно собрать семплы для работы с юнитом VPN
    if (${CMAKE_BUILD_VPN_APP_SAMPLE})
        # Устанавливаем приложение работы с юнитом VPN клиентом
        install(TARGETS ${CLIENT_APP_VPN_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение работы с юнитом VPN сервером
        install(TARGETS ${SERVER_APP_VPN_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_VPN_APP_SAMPLE})

    # Если нужно собрать семплы для работы с юнитом SOCKS5
    if (${CMAKE_BUILD_SOCKS5_APP_SAMPLE})
        # Устанавливаем приложение работы с юнитом SOCKS5 клиентом
        install(TARGETS ${CLIENT_APP_SOCKS5_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение работы с юнитом SOCKS5 клиентом для UDP
        install(TARGETS ${CLIENT_APP_SOCKS5-UDP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение работы с юнитом SOCKS5 клиентом для DTLS
        install(TARGETS ${CLIENT_APP_SOCKS5-DTLS_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение работы с юнитом SOCKS5 сервером
        install(TARGETS ${SERVER_APP_SOCKS5_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_SOCKS5_APP_SAMPLE})

    # Если нужно собрать семплы для работы с юнитом файловой системы
    if (${CMAKE_BUILD_UNIT_FS_APP_SAMPLE})
        # Устанавливаем приложение работы с юнитом файловой системы
        install(TARGETS ${UNIT_APP_FS_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_UNIT_FS_APP_SAMPLE})

    # Если нужно собрать семплы для работы с юнитом резолвером доменных имён
    if (${CMAKE_BUILD_UNIT_DNS_APP_SAMPLE})
        # Устанавливаем приложение работы с юнитом резолвером доменных имён
        install(TARGETS ${UNIT_APP_DNS_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_UNIT_DNS_APP_SAMPLE})

    # Если нужно собрать семплы для работы с юнитом синхронизации времени NTP
    if (${CMAKE_BUILD_UNIT_NTP_APP_SAMPLE})
        # Устанавливаем приложение работы с юнитом синхронизации времени NTP
        install(TARGETS ${UNIT_APP_NTP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_UNIT_NTP_APP_SAMPLE})

    # Если нужно собрать семплы для работы с юнитом ICMP-клиентом
    if (${CMAKE_BUILD_UNIT_ICMP_APP_SAMPLE})
        # Устанавливаем приложение работы с юнитом ICMP-клиентом
        install(TARGETS ${UNIT_APP_ICMP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_UNIT_ICMP_APP_SAMPLE})

    # Если нужно собрать семплы для работы с юнитом таймера
    if (${CMAKE_BUILD_UNIT_TIMER_APP_SAMPLE})
        # Устанавливаем приложение работы с юнитом таймера
        install(TARGETS ${UNIT_APP_TIMER_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_UNIT_TIMER_APP_SAMPLE})

    # Если нужно собрать семплы для работы с юнитом кластера
    if (${CMAKE_BUILD_UNIT_CLUSTER_APP_SAMPLE})
        # Устанавливаем приложение работы с юнитом кластера
        install(TARGETS ${UNIT_APP_CLUSTER_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_UNIT_CLUSTER_APP_SAMPLE})

    # Если нужно собрать семплы для работы с юнитом уведомителя
    if (${CMAKE_BUILD_UNIT_NOTIFIER_APP_SAMPLE})
        # Устанавливаем приложение работы с юнитом уведомителя
        install(TARGETS ${UNIT_APP_NOTIFIER_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_UNIT_NOTIFIER_APP_SAMPLE})

    # Если нужно собрать семплы для работы с IP-адресами
    if (${CMAKE_BUILD_IP_NET_APP_SAMPLE})
        # Устанавливаем приложение работы с IP-адресами
        install(TARGETS ${IP_APP_NET_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_IP_NET_APP_SAMPLE})

    # Если нужно собрать семплы для работы с URI
    if (${CMAKE_BUILD_URI_NET_APP_SAMPLE})
        # Устанавливаем приложение работы с URI
        install(TARGETS ${URI_APP_NET_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_URI_NET_APP_SAMPLE})

    # Если нужно собрать семплы для работы с Ethernet-адресами
    if (${CMAKE_BUILD_ETH_NET_APP_SAMPLE})
        # Устанавливаем приложение работы с Ethernet-адресами
        install(TARGETS ${ETH_APP_NET_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_ETH_NET_APP_SAMPLE})

    # Если нужно собрать семплы для работы с датой и временем
    if (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})
        # Устанавливаем приложение работы с датой и временем
        install(TARGETS ${DATE_APP_SYS_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})

    # Если нужно собрать семплы для работы с бинарным контейнером
    if (${CMAKE_BUILD_BINBOX_SYS_APP_SAMPLE})
        # Устанавливаем приложение работы с бинарным контейнером
        install(TARGETS ${BINBOX_APP_SYS_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_BINBOX_SYS_APP_SAMPLE})

    # Если нужно собрать семплы для работы с файловой системой
    if (${CMAKE_BUILD_FILESYSTEM_SYS_APP_SAMPLE})
        # Устанавливаем приложение работы с файловой системой
        install(TARGETS ${FILESYSTEM_APP_SYS_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_FILESYSTEM_SYS_APP_SAMPLE})

    # Если нужно собрать семплы для работы с резольвером процессов
    if (${CMAKE_BUILD_PROCRE_SYS_APP_SAMPLE})
        # Устанавливаем приложение работы с резольвером процессов
        install(TARGETS ${PROCRE_APP_SYS_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_PROCRE_SYS_APP_SAMPLE})

    # Если нужно собрать семплы для работы с шифрованием и хэшированием
    if (${CMAKE_BUILD_CRYPTO_SYS_APP_SAMPLE})
        # Устанавливаем приложение работы с шифрованием и хэшированием
        install(TARGETS ${CRYPTO_APP_SYS_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_CRYPTO_SYS_APP_SAMPLE})

    # Если нужно собрать семплы для работы с HTTP-авторизацией
    if (${CMAKE_BUILD_AUTH_PROTO_APP_SAMPLE})
        # Устанавливаем приложение работы с HTTP-авторизацией
        install(TARGETS ${AUTH_APP_PROTO_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_AUTH_PROTO_APP_SAMPLE})

    # Если нужно собрать семплы для работы с HTTP-заголовками
    if (${CMAKE_BUILD_HEADERS_PROTO_APP_SAMPLE})
        # Устанавливаем приложение работы с HTTP-заголовками
        install(TARGETS ${HEADERS_APP_PROTO_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_HEADERS_PROTO_APP_SAMPLE})

    # Если нужно собрать семплы для работы с парсером HTTP/1.0 и HTTP/1.1
    if (${CMAKE_BUILD_PARSER_PROTO_APP_SAMPLE})
        # Устанавливаем приложение работы с парсером HTTP/1.0 и HTTP/1.1
        install(TARGETS ${PARSER_APP_PROTO_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_PARSER_PROTO_APP_SAMPLE})

    # Если нужно собрать семплы для работы с парсером HTTP/2
    if (${CMAKE_BUILD_PARSER2_PROTO_APP_SAMPLE})
        # Устанавливаем приложение работы с парсером HTTP/2
        install(TARGETS ${PARSER2_APP_PROTO_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_PARSER2_PROTO_APP_SAMPLE})

    # Если нужно собрать семплы для работы с протоколом SOCKS5
    if (${CMAKE_BUILD_SOCKS5_PROTO_APP_SAMPLE})
        # Устанавливаем приложение работы с протоколом SOCKS5
        install(TARGETS ${SOCKS5_APP_PROTO_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_SOCKS5_PROTO_APP_SAMPLE})

    # Если нужно собрать семплы для работы с протоколом QUIC
    if (${CMAKE_BUILD_QUIC_PROTO_APP_SAMPLE})
        # Устанавливаем приложение работы с QUIC клиентом
        install(TARGETS ${QUIC_CLIENT_APP_PROTO_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение работы с QUIC сервером
        install(TARGETS ${QUIC_SERVER_APP_PROTO_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_QUIC_PROTO_APP_SAMPLE})

    # Если нужно собрать семплы для работы с компрессией данных
    if (${CMAKE_BUILD_COMPRESSOR_SYS_APP_SAMPLE})
        # Устанавливаем приложение блочной компрессии данных
        install(TARGETS ${COMPRESSOR_APP_BLOCK_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение потоковой компрессии данных
        install(TARGETS ${COMPRESSOR_APP_STREAM_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_COMPRESSOR_SYS_APP_SAMPLE})

    # Если нужно собрать семплы для работы с каталогами
    if (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})
        # Устанавливаем приложение работы с каталогами
        install(TARGETS ${DIR_APP_IO_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})

    # Если нужно собрать семплы для работы с файлами
    if (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})
        # Устанавливаем приложение работы с файлами
        install(TARGETS ${FILE_APP_IO_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})

    # Если нужно собрать семплы для работы с пингами (ICMP)
    if (${CMAKE_BUILD_PING_IO_APP_SAMPLE})
        # Устанавливаем приложение работы с пингами (ICMP)
        install(TARGETS ${PING_APP_IO_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_PING_IO_APP_SAMPLE})

    # Если нужно собрать семплы для работы с таймером
    if (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})
        # Устанавливаем приложение работы с таймером
        install(TARGETS ${TIMER_APP_IO_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})

    # Если нужно собрать семплы для работы с пользовательскими событиями
    if (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})
        # Устанавливаем приложение работы с пользовательскими событиями
        install(TARGETS ${EVENTS_APP_IO_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})

    # Если нужно собрать семплы для работы с межпроцессорным взаимодействием
    if (${CMAKE_BUILD_IPC_IO_APP_SAMPLE})
        # Устанавливаем приложение работы с межпроцессорным взаимодействием (именованный канал)
        install(TARGETS ${IPC_PIPE_APP_IO_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение работы с межпроцессорным взаимодействием (поток)
        install(TARGETS ${IPC_STREAM_APP_IO_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение работы с межпроцессорным взаимодействием (датаграмма)
        install(TARGETS ${IPC_DATAGRAM_APP_IO_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение работы с межпроцессорным взаимодействием (последовательная упаковка)
        install(TARGETS ${IPC_SEQPACKET_APP_IO_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_IPC_IO_APP_SAMPLE})

    # Если нужно собрать семплы для работы с TCP (клиент и сервер)
    if (${CMAKE_BUILD_NET_TCP_APP_SAMPLE})
        # Устанавливаем приложение клиента TCP
        install(TARGETS ${CLIENT_APP_NET_TCP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение сервера TCP
        install(TARGETS ${SERVER_APP_NET_TCP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_NET_TCP_APP_SAMPLE})

    # Если нужно собрать семплы для работы с UDP (клиент и сервер)
    if (${CMAKE_BUILD_NET_UDP_APP_SAMPLE})
        # Устанавливаем приложение клиента UDP
        install(TARGETS ${CLIENT_APP_NET_UDP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение клиента UDP с подключением
        install(TARGETS ${CLIENT_APP_NET_UDP_CONN_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение сервера UDP
        install(TARGETS ${SERVER_APP_NET_UDP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_NET_UDP_APP_SAMPLE})

    # Если нужно собрать семплы для работы с ограничителем пропускной способности (клиент и сервер)
    if (${CMAKE_BUILD_NET_BANDWIDTH_APP_SAMPLE})
        # Устанавливаем приложение клиента TCP с ограничителем пропускной способности
        install(TARGETS ${CLIENT_APP_NET_TCP_BANDWIDTH_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение сервера TCP с ограничителем пропускной способности
        install(TARGETS ${SERVER_APP_NET_TCP_BANDWIDTH_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_NET_BANDWIDTH_APP_SAMPLE})

    # Если нужно собрать семплы для работы с TUN (клиент и сервер)
    if (${CMAKE_BUILD_NET_TUN_APP_SAMPLE})
        # Устанавливаем приложение клиента TUN
        install(TARGETS ${CLIENT_APP_NET_TUN_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение сервера TUN
        install(TARGETS ${SERVER_APP_NET_TUN_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_NET_TUN_APP_SAMPLE})

    # Если нужно собрать семплы для работы с UDP Broadcast (клиент и сервер)
    if (${CMAKE_BUILD_NET_BROADCAST_APP_SAMPLE})
        # Устанавливаем приложение клиента UDP Broadcast
        install(TARGETS ${CLIENT_APP_NET_BROADCAST_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение сервера UDP Broadcast
        install(TARGETS ${SERVER_APP_NET_BROADCAST_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_NET_BROADCAST_APP_SAMPLE})

    # Если нужно собрать семплы для работы с UDP Multicast (клиент и сервер)
    if (${CMAKE_BUILD_NET_MULTICAST_APP_SAMPLE})
        # Устанавливаем приложение клиента UDP Multicast сценарий 1
        install(TARGETS ${CLIENT_APP_NET_MULTICAST1_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение сервера UDP Multicast сценарий 1
        install(TARGETS ${SERVER_APP_NET_MULTICAST1_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение клиента UDP Multicast сценарий 3
        install(TARGETS ${CLIENT_APP_NET_MULTICAST3_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение сервера UDP Multicast сценарий 3
        install(TARGETS ${SERVER_APP_NET_MULTICAST3_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_NET_MULTICAST_APP_SAMPLE})

    # Если нужно собрать семплы для работы с UDP Splice (клиент и сервер)
    if (${CMAKE_BUILD_NET_SPLICE_APP_SAMPLE})
        # Устанавливаем приложение клиента UDP Splice
        install(TARGETS ${CLIENT_APP_NET_SPLICE_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение сервера UDP Splice
        install(TARGETS ${SERVER_APP_NET_SPLICE_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_NET_SPLICE_APP_SAMPLE})

    # Если нужно собрать семплы для работы с UDS (клиент и сервер)
    if (${CMAKE_BUILD_NET_UDS_APP_SAMPLE})
        # Устанавливаем приложение клиента UDS-TCP
        install(TARGETS ${CLIENT_APP_NET_UDS_TCP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение сервера UDS-TCP
        install(TARGETS ${SERVER_APP_NET_UDS_TCP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение клиента UDS-UDP
        install(TARGETS ${CLIENT_APP_NET_UDS_UDP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение сервера UDS-UDP
        install(TARGETS ${SERVER_APP_NET_UDS_UDP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_NET_UDS_APP_SAMPLE})

    # Если нужно собрать семплы для работы с DTLS (клиент и сервер)
    if (${CMAKE_BUILD_NET_DTLS_APP_SAMPLE})
        # Устанавливаем приложение клиента DTLS
        install(TARGETS ${CLIENT_APP_NET_DTLS_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение сервера DTLS
        install(TARGETS ${SERVER_APP_NET_DTLS_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_NET_DTLS_APP_SAMPLE})

    # Если нужно собрать семплы для работы с TLS (клиент и сервер)
    if (${CMAKE_BUILD_NET_TLS_APP_SAMPLE})
        # Устанавливаем приложение клиента TLS
        install(TARGETS ${CLIENT_APP_NET_TLS_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение сервера TLS
        install(TARGETS ${SERVER_APP_NET_TLS_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение клиента TLS с поддержкой веб-серверов
        install(TARGETS ${CLIENT_APP_NET_TLS_WEB_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        # Устанавливаем приложение сервера TLS с поддержкой мультисертификатов
        install(TARGETS ${SERVER_APP_NET_TLS_MULTI_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_NET_TLS_APP_SAMPLE})

    # Если сборка производится под операционную систему FreeBSD или Linux
    if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        # Если нужно собрать семплы для работы с SCTP (клиент и сервер)
        if (${CMAKE_BUILD_NET_SCTP_APP_SAMPLE})
            # Устанавливаем приложение клиента TLS
            install(TARGETS ${CLIENT_APP_NET_SCTP_TLS_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
            # Устанавливаем приложение сервера TLS
            install(TARGETS ${SERVER_APP_NET_SCTP_TLS_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
            # Устанавливаем приложение клиента DTLS
            install(TARGETS ${CLIENT_APP_NET_SCTP_DTLS_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
            # Устанавливаем приложение сервера DTLS
            install(TARGETS ${SERVER_APP_NET_SCTP_DTLS_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
            # Устанавливаем приложение клиента SCTP - AUTH
            install(TARGETS ${CLIENT_APP_NET_SCTP_AUTH_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
            # Устанавливаем приложение сервера SCTP - AUTH
            install(TARGETS ${SERVER_APP_NET_SCTP_AUTH_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
            # Устанавливаем приложение клиента SCTP - STREAM
            install(TARGETS ${CLIENT_APP_NET_SCTP_STREAM_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
            # Устанавливаем приложение сервера SCTP - STREAM
            install(TARGETS ${SERVER_APP_NET_SCTP_STREAM_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
            # Устанавливаем приложение клиента SCTP - SEQPACKET
            install(TARGETS ${CLIENT_APP_NET_SCTP_SEQPACKET_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
            # Устанавливаем приложение сервера SCTP - SEQPACKET
            install(TARGETS ${SERVER_APP_NET_SCTP_SEQPACKET_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        endif (${CMAKE_BUILD_NET_SCTP_APP_SAMPLE})
    endif()

    # Если сборка производится под операционную систему Windows
    if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
        #install(TARGETS ${CLUSTER_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif()
endif (${CMAKE_BUILD_SAMPLES})

# Выполняем запуск скрипта генерации CMAKE-файла
add_custom_command(TARGET "${AWH_LIB_NAME}" POST_BUILD
    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/build_cmake.sh" "${CMAKE_INSTALL_PREFIX}"
VERBATIM)

# Устанавливаем сгенерированный CMAKE-файл
install(FILES "${CMAKE_SOURCE_DIR}/third_party/cmake/FindAWH.cmake" DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake")

# =============================================================================
# === MACOS X FRAMEWORK TARGET ==================================================
# =============================================================================
if (APPLE AND NOT IOS)
    SET(FRAMEWORK_OUTPUT_PATH "${CMAKE_BINARY_DIR}/dist/${AWH_LIB_NAME}.framework")

    # 1. Формируем список библиотек для объединения
    # Основная библиотека awh
    SET(ALL_LIBS "$<TARGET_FILE:${AWH_LIB_NAME}>")
    
    # Если режим отладки активирован
    if (CMAKE_BUILD_TYPE MATCHES Debug)
        # Добавляем библиотеку зависимостей, если она найдена
        if(DEFINED DEPEND_LIBRARY AND EXISTS "${DEPEND_LIBRARY}")
            list(APPEND ALL_LIBS "${DEPEND_LIBRARY}")
            message(STATUS "Framework will include: ${AWH_LIB_NAME} + Dependence")
        else()
            message(WARNING "DEPEND_LIBRARY not found, framework will include only ${AWH_LIB_NAME}")
        endif()
    endif()

    # 2. Преобразуем список CMake в строку для Bash (разделитель - пробел)
    string(REPLACE ";" " " ALL_LIBS_STR "${ALL_LIBS}")

    # 3. Файлы, которые будут созданы (для отслеживания изменений)
    SET(FRAMEWORK_BYPRODUCTS
        "${FRAMEWORK_OUTPUT_PATH}/Headers"
        "${FRAMEWORK_OUTPUT_PATH}/Modules"
        "${FRAMEWORK_OUTPUT_PATH}/Resources"
        "${FRAMEWORK_OUTPUT_PATH}/${AWH_LIB_NAME}"
        "${FRAMEWORK_OUTPUT_PATH}/Versions/Current"
    )

    # 4. Создаем цель 'framework'
    add_custom_target(framework
        COMMAND ${CMAKE_COMMAND} -E echo "📦 Building macOS Framework..."
        COMMAND bash "${CMAKE_SOURCE_DIR}/sh/build_framework.sh" 
            "${AWH_LIB_NAME}"             # Arg 1: Имя фреймворка
            "${CMAKE_BINARY_DIR}"         # Arg 2: Build dir
            "${CMAKE_BINARY_DIR}/dist"    # Arg 3: Install dir
            "${CMAKE_SOURCE_DIR}/include" # Arg 4: Include dir
            "${ALL_LIBS_STR}"             # Arg 5: Список .a библиотек через пробел
        BYPRODUCTS ${FRAMEWORK_BYPRODUCTS}
        DEPENDS ${AWH_LIB_NAME}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Packaging ${AWH_LIB_NAME} into macOS Framework"
        VERBATIM
    )

    # Выводим сообщение об удачной сборке framework
    message(STATUS "✅ Target 'framework' added for macOS build")

    # Устанавливаем сгенерированную версию framework
    install(DIRECTORY "${CMAKE_BINARY_DIR}/dist/${AWH_LIB_NAME}.framework"
        DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
        USE_SOURCE_PERMISSIONS
    )
endif()
