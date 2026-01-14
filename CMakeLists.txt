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

# Сборка с установкой количества доступных файловых дескрипторов
if (CMAKE_MAX_COUNT_FDS)
    add_compile_definitions(AWH_MAX_COUNT_FDS=${CMAKE_MAX_COUNT_FDS})
    message(STATUS "Build with max FDs: ${CMAKE_MAX_COUNT_FDS}\n")
endif (CMAKE_MAX_COUNT_FDS)

# Сборка с установкой адреса DNS-сервера IPv4 по умолчанию
if (CMAKE_DEFAULT_DNS_SERVER)
    add_compile_definitions(AWH_IPV4_RESOLVER=${CMAKE_DEFAULT_DNS_SERVER})
    message(STATUS "Build with default DNS server: ${CMAKE_DEFAULT_DNS_SERVER}\n")
endif (CMAKE_DEFAULT_DNS_SERVER)

# Сборка с установкой адреса DNS-сервера IPv6 по умолчанию
if (CMAKE_DEFAULT_DNS6_SERVER)
    add_compile_definitions(AWH_IPV6_RESOLVER=${CMAKE_DEFAULT_DNS6_SERVER})
    message(STATUS "Build with default IPv6 DNS server: ${CMAKE_DEFAULT_DNS6_SERVER}\n")
endif (CMAKE_DEFAULT_DNS6_SERVER)

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

# Если активирован флаг установки максимального размера HTTP-тела запроса/ответа
if (CMAKE_MAX_HTTP_BODY_SIZE)
    add_compile_definitions(AWH_MAX_BODY_SIZE=${CMAKE_MAX_HTTP_BODY_SIZE})
    message(STATUS "Build with max body size: ${CMAKE_MAX_HTTP_BODY_SIZE}\n")
endif (CMAKE_MAX_HTTP_BODY_SIZE)

# Если активирован флаг установки максимального размера SSL буфера
if (CMAKE_MAX_SSL_BUFFER_SIZE)
    add_compile_definitions(AWH_MAX_SSL_BUFFER_SIZE=${CMAKE_MAX_SSL_BUFFER_SIZE})
    message(STATUS "Build with max SSL buffer size: ${CMAKE_MAX_SSL_BUFFER_SIZE}\n")
endif (CMAKE_MAX_SSL_BUFFER_SIZE)

# Если активирован флаг установки максимального размера буфера на четение данных событий
if (CMAKE_MAX_EVENT_BUFFER_SIZE)
    add_compile_definitions(AWH_MAX_EVENT_BUFFER_SIZE=${CMAKE_MAX_EVENT_BUFFER_SIZE})
    message(STATUS "Build with max event buffer size: ${CMAKE_MAX_EVENT_BUFFER_SIZE}\n")
endif (CMAKE_MAX_EVENT_BUFFER_SIZE)

# Если активирован флаг установки максимального количества опрашиваемых событий за одну итерацию
if (CMAKE_MAX_POLL_EVENTS_COUNT)
    add_compile_definitions(AWH_MAX_POLL_EVENTS_COUNT=${CMAKE_MAX_POLL_EVENTS_COUNT})
    message(STATUS "Build with max poll events count: ${CMAKE_MAX_POLL_EVENTS_COUNT}\n")
endif (CMAKE_MAX_POLL_EVENTS_COUNT)

# Если активирован флаг установки максимального размера очереди
if (CMAKE_MAX_RECORDS_QUEUE)
    add_compile_definitions(AWH_MAX_RECORDS_QUEUE=${CMAKE_MAX_RECORDS_QUEUE})
    message(STATUS "Build with max count records queue: ${CMAKE_MAX_RECORDS_QUEUE}\n")
endif (CMAKE_MAX_RECORDS_QUEUE)

# Если активирован флаг установки максимального размера потребления памяти очередью
if (CMAKE_MAX_MEMORY_QUEUE)
    add_compile_definitions(AWH_MAX_MEMORY_QUEUE=${CMAKE_MAX_MEMORY_QUEUE})
    message(STATUS "Build with max memory queue usage size: ${CMAKE_MAX_MEMORY_QUEUE}\n")
endif (CMAKE_MAX_MEMORY_QUEUE)

# Если активирован флаг установки максимального размера потребления памяти буфера
if (CMAKE_MAX_MEMORY_BUFFER)
    add_compile_definitions(AWH_MAX_MEMORY_BUFFER=${CMAKE_MAX_MEMORY_BUFFER})
    message(STATUS "Build with max memory buffer usage size: ${CMAKE_MAX_MEMORY_BUFFER}\n")
endif (CMAKE_MAX_MEMORY_BUFFER)

# Выводим информацию каталога где выполняется сброка
message(STATUS "Build dir: ${CMAKE_CURRENT_BINARY_DIR}\n")

# Получаем архитектуру
EXECUTE_PROCESS(COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE )
message(STATUS "Architecture: ${ARCHITECTURE}")

# Если операционной системой является Linux
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
# Если операционной системой является Solaris
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
# Если операционной системой является MacOS X
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    if (NOT OS_VERSION)
        EXECUTE_PROCESS(COMMAND sw_vers -productVersion COMMAND tr -d '\n' OUTPUT_VARIABLE OS_VERSION)
    endif()
    message(STATUS "Detected OS: MacOS X v${OS_VERSION}")
    SET(DEBUGGER "-glldb")    
    SET(CMAKE_OSX_DEPLOYMENT_TARGET ${OS_VERSION})
# Если операционной системой является FreeBSD
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    if (NOT OS_VERSION)
        EXECUTE_PROCESS(COMMAND uname -a COMMAND awk "{print $3}" COMMAND tr -d '\n' OUTPUT_VARIABLE OS_VERSION)
    endif()
    message(STATUS "Detected OS: FreeBSD v${OS_VERSION}")
    SET(DEBUGGER "-ggdb3")
# Если операционной системой является NetBSD
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "NetBSD")
    message(STATUS "Detected OS: NetBSD")
    SET(DEBUGGER "-ggdb3")
# Если операционной системой является OpenBSD
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
    message(STATUS "Detected OS: OpenBSD")
    SET(DEBUGGER "-ggdb3")
# Если операционной системой является Windows
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

# Устанавливаем параметры игнорирования ошибок для MacOS X
if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    SET(AWH_PARAMS "-framework Carbon -Wno-cpp -Wno-deprecated -Wno-unused-command-line-argument")
# Устанавливаем параметры игнорирования ошибок для Solaris
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
    SET(AWH_PARAMS "-lsocket -lnsl -lc -Wno-cpp -Wno-deprecated")
# Устанавливаем параметры игнорирования ошибок для Windows и Linux
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Windows" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    SET(AWH_PARAMS "-Wno-cpp -Wno-deprecated")
# Устанавливаем параметры игнорирования ошибок для FreeBSD, NetBSD и OpenBSD
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "NetBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
    SET(AWH_PARAMS "-lutil -Wno-cpp -Wno-deprecated -Wno-unused-command-line-argument")
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
    message(STATUS "TcMalloc include dir found: " ${TCMALLOC_INCLUDE_DIR})
    message(STATUS "TcMalloc lib found: " ${TCMALLOC_LIBRARY} " " ${TCMALLOC_COMMON_LIBRARY})
else (TcMalloc_FOUND)
    message(FATAL_ERROR "TcMalloc not found")
endif (TcMalloc_FOUND)

# Если необходимо собрать тесты
if (CMAKE_BUILD_TESTS)
    # Выполняем поиск библиотеки OpenSSL
    find_package(OpenSSL REQUIRED)

    # Провекра OpenSSL
    if (OpenSSL_FOUND)
        message(STATUS "OpenSSL lib found: " ${OPENSSL_LIBRARY} " " ${OPENSSL_CRYPTO_LIBRARY})
    else (OpenSSL_FOUND)
        message(FATAL_ERROR "OpenSSL not found")
    endif (OpenSSL_FOUND)
endif (CMAKE_BUILD_TESTS)

# Если Doxygen найден
if(Doxygen_FOUND)
    # Предположим, что Doxyfile лежит в корне исходников
    SET(DOXYFILE_PATH ${CMAKE_SOURCE_DIR}/Doxyfile)

    # Проверяем, что файл существует
    if(EXISTS ${DOXYFILE_PATH})
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
        message(STATUS "Doxyfile not found at ${DOXYFILE_PATH} — doc target skipped")
    # Конец проверки файла
    endif()
# Если Doxygen не найден
else()
    # Выводим сообщение об отсутствии Doxygen
    message(STATUS "Doxygen not found — 'doc' target will not be available")
endif()

# Выполняем подключение хидеров
include_directories(BEFORE ${DEPEND_INCLUDE_DIRS})
include_directories(BEFORE ${TCMALLOC_INCLUDE_DIR})
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/include)
include_directories(BEFORE ${CMAKE_SOURCE_DIR}/contrib/include)
include_directories(SYSTEM ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})

# Устанавливаем параметры игнорирования ошибок для FreeBSD, NetBSD и OpenBSD
if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin" OR ${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "NetBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
    # Выполняем подключение бэкендов IO
    SET(SOURCE_FILES
        "${SOURCE_FILES}"
        "${CMAKE_SOURCE_DIR}/src/net/backend/bsd/eth.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/bsd/kqueue.cpp"
    )
endif()

# Выполняем подключение исходников
SET(SOURCE_FILES
    "${SOURCE_FILES}"
    "${CMAKE_SOURCE_DIR}/src/sys/os.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/fmk.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/log.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/reg.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/queue.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/buffer.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/chrono.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/tls.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/fds.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/nwt.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/addr.cpp"
    "${CMAKE_SOURCE_DIR}/contrib/src/cityhash/city.cc"
)

# Устанавливаем имя библиотеки
SET(AWH_LIB_NAME ${PROJECT_NAME})

# Если необходимо собрать тесты
if (CMAKE_BUILD_TESTS)
    # Задаем имя библиотеки для сбора статистики покрытия кода
    SET(AWH_LIB_GCOV_NAME ${PROJECT_NAME}_gcov)
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
    # Включаем флаги компиляции для сбора статистики покрытия кода
    target_compile_options(${AWH_LIB_GCOV_NAME} PRIVATE ${GCC_COVERAGE_COMPILE_FLAGS})
endif (CMAKE_BUILD_TESTS)

# Указываем что нужно собрать семплы
option(CMAKE_BUILD_SAMPLES "Build unit samples" OFF)

# Если необходимо собрать семплы
if (${CMAKE_BUILD_SAMPLES})
    # Указывает каталог для ВСЕХ исполняемых файлов
    SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/samples)

    # Задаем именя библиотеки и бинарника
    
    #SET(IPC_APP_NAME ipc)
    #SET(CLIENT_APP_NAME client)
    #SET(SERVER_APP_NAME server)
    
    # Устанавливаем имя приложения работы с IP-адресами
    SET(IP_APP_NET_NAME net-ip)
    # Указываем что нужно собрать приложение работы с IP-адресами
    option(CMAKE_BUILD_IP_NET_APP_SAMPLE "Build IP app sample" ON)

    # Устанавливаем имя приложения работы с датой и временем
    SET(DATE_APP_SYS_NAME sys-date)
    # Указываем что нужно собрать приложение работы с датой и временем
    option(CMAKE_BUILD_DATE_SYS_APP_SAMPLE "Build Date app sample" ON)

    # Устанавливаем имя приложения работы с каталогами
    SET(DIR_APP_IO_NAME io-dir)
    # Указываем что нужно собрать приложение работы с каталогами
    option(CMAKE_BUILD_DIR_IO_APP_SAMPLE "Build Dir IO app sample" ON)

    # Устанавливаем имя приложения работы с файлами
    SET(FILE_APP_IO_NAME io-file)
    # Указываем что нужно собрать приложение работы с файлами
    option(CMAKE_BUILD_FILE_IO_APP_SAMPLE "Build File IO app sample" ON)

    # Устанавливаем имя приложения работы с таймером
    SET(TIMER_APP_IO_NAME io-timer)
    # Указываем что нужно собрать приложение работы с таймером
    option(CMAKE_BUILD_TIMER_IO_APP_SAMPLE "Build Timer IO app sample" ON)

    # Устанавливаем имя приложения работы с пользовательскими событиями
    SET(EVENTS_APP_IO_NAME io-events)
    # Указываем что нужно собрать приложение работы с пользовательскими событиями
    option(CMAKE_BUILD_EVENTS_IO_APP_SAMPLE "Build Events IO app sample" ON)
endif (${CMAKE_BUILD_SAMPLES})

# Если операционной системой является Windows
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    # export MSYS="error_start:%WD%dumper.exe -d -c %1 %2"
    # Устанавливаем необходимые зависимости для MS Windows
    SET(WINFLAGS -lstdc++ -lws2_32 -lgdi32 -lcrypt32 -lIphlpapi -lbcrypt -flto=auto)

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
    
        # Если нужно собрать тесты для работы с ОС
        if (${CMAKE_BUILD_IP_NET_APP_SAMPLE})
            # Выполняем создание приложения работы с IP-адресами
            add_sample(${IP_APP_NET_NAME} sample/net/ip.cpp net)
        endif (${CMAKE_BUILD_IP_NET_APP_SAMPLE})

        # Если нужно собрать тесты для работы с датой и временем
        if (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})
            # Выполняем создание приложения работы с датой и временем
            add_sample(${DATE_APP_SYS_NAME} sample/sys/date.cpp sys)
        endif (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})

        # Если нужно собрать тесты для работы с каталогами
        if (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})
            # Выполняем создание приложения работы с каталогами
            add_sample(${DIR_APP_IO_NAME} sample/io/dir.cpp io)
        endif (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})

        # Если нужно собрать тесты для работы с файлами
        if (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})
            # Выполняем создание приложения работы с файлами
            add_sample(${FILE_APP_IO_NAME} sample/io/file.cpp io)
        endif (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})

        # Если нужно собрать тесты для работы с таймером
        if (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})
            # Выполняем создание приложения работы с таймером
            add_sample(${TIMER_APP_IO_NAME} sample/io/timer.cpp io)
        endif (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})

        # Если нужно собрать тесты для работы с пользовательскими событиями
        if (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})
            # Выполняем создание приложения работы с пользовательскими событиями
            add_sample(${EVENTS_APP_IO_NAME} sample/io/events.cpp io)
        endif (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})

        #add_executable(${IPC_APP_NAME} sample/ipc.cpp ${RES_FILES})
        #add_executable(${CLIENT_APP_NAME} sample/client.cpp ${RES_FILES})
        #add_executable(${SERVER_APP_NAME} sample/server.cpp ${RES_FILES})
    endif (${CMAKE_BUILD_SAMPLES})
# Если операционной системой является Linux, FreeBSD, NetBSD, OpenBSD, MacOS X, Solaris
else()
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
    
        # Если нужно собрать тесты для работы с ОС
        if (${CMAKE_BUILD_IP_NET_APP_SAMPLE})
            # Выполняем создание приложения работы с IP-адресами
            add_sample(${IP_APP_NET_NAME} sample/net/ip.cpp net)
        endif (${CMAKE_BUILD_IP_NET_APP_SAMPLE})

        # Если нужно собрать тесты для работы с датой и временем
        if (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})
            # Выполняем создание приложения работы с датой и временем
            add_sample(${DATE_APP_SYS_NAME} sample/sys/date.cpp sys)
        endif (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})

        # Если нужно собрать тесты для работы с каталогами
        if (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})
            # Выполняем создание приложения работы с каталогами
            add_sample(${DIR_APP_IO_NAME} sample/io/dir.cpp io)
        endif (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})

        # Если нужно собрать тесты для работы с файлами
        if (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})
            # Выполняем создание приложения работы с файлами
            add_sample(${FILE_APP_IO_NAME} sample/io/file.cpp io)
        endif (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})

         # Если нужно собрать тесты для работы с таймером
        if (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})
            # Выполняем создание приложения работы с таймером
            add_sample(${TIMER_APP_IO_NAME} sample/io/timer.cpp io)
        endif (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})

        # Если нужно собрать тесты для работы с пользовательскими событиями
        if (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})
            # Выполняем создание приложения работы с пользовательскими событиями
            add_sample(${EVENTS_APP_IO_NAME} sample/io/events.cpp io)
        endif (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})
        
        #add_executable(${IPC_APP_NAME} sample/ipc.cpp)
        #add_executable(${CLIENT_APP_NAME} sample/client.cpp)
        #add_executable(${SERVER_APP_NAME} sample/server.cpp)
    endif (${CMAKE_BUILD_SAMPLES})
endif()

# Если активирован режим отладки
if (CMAKE_BUILD_TYPE MATCHES Debug)
    # Удаляем библиотеку TCMalloc
    SET(TCMALLOC_LIBRARIES "")
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
        )

        # Если необходимо собрать семплы
        if (${CMAKE_BUILD_SAMPLES})
            # Если нужно собрать тесты для работы с ОС
            if (${CMAKE_BUILD_IP_NET_APP_SAMPLE})
                # Выполняем сборку приложения работы с IP-адресами
                target_link_libraries(
                    ${IP_APP_NET_NAME}
                    ${AWH_LIB_NAME}
                    ${TCMALLOC_LIBRARIES}
                    ${WINFLAGS}
                )
            endif (${CMAKE_BUILD_IP_NET_APP_SAMPLE})

            # Если нужно собрать тесты для работы с датой и временем
            if (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})
                # Выполняем сборку приложения даты и времени
                target_link_libraries(
                    ${DATE_APP_SYS_NAME}
                    ${AWH_LIB_NAME}
                    ${TCMALLOC_LIBRARIES}
                    ${WINFLAGS}
                )
            endif (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})

            # Если нужно собрать тесты для работы с каталогами
            if (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})
                # Выполняем сборку приложения работы с каталогами
                target_link_libraries(
                    ${DIR_APP_IO_NAME}
                    ${AWH_LIB_NAME}
                    ${TCMALLOC_LIBRARIES}
                    ${WINFLAGS}
                )
            endif (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})

            # Если нужно собрать тесты для работы с файлами
            if (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})
                # Выполняем сборку приложения работы с файлами
                target_link_libraries(
                    ${FILE_APP_IO_NAME}
                    ${AWH_LIB_NAME}
                    ${TCMALLOC_LIBRARIES}
                    ${WINFLAGS}
                )
            endif (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})

            # Если нужно собрать тесты для работы с таймером
            if (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})
                # Выполняем сборку приложения таймера
                target_link_libraries(
                    ${TIMER_APP_IO_NAME}
                    ${AWH_LIB_NAME}
                    ${TCMALLOC_LIBRARIES}
                    ${WINFLAGS}
                )
            endif (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})

            # Если нужно собрать тесты для работы с пользовательскими событиями
            if (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})
                # Выполняем сборку приложения пользовательских событий
                target_link_libraries(
                    ${EVENTS_APP_IO_NAME}
                    ${AWH_LIB_NAME}
                    ${TCMALLOC_LIBRARIES}
                    ${WINFLAGS}
                )
            endif (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})

            # Выполняем сборку приложения межпроцессного взаимодействия
            #target_link_libraries(
            #    ${IPC_APP_NAME}
            #    ${AWH_LIB_NAME}
            #    ${TCMALLOC_LIBRARIES}
            #    ${WINFLAGS}
            #)

            # Выполняем сборку приложения клиента
            #target_link_libraries(
            #    ${CLIENT_APP_NAME}
            #    ${AWH_LIB_NAME}
            #    ${TCMALLOC_LIBRARIES}
            #    ${WINFLAGS}
            #)

            # Выполняем сборку приложения сервера
            #target_link_libraries(
            #    ${SERVER_APP_NAME}
            #    ${AWH_LIB_NAME}
            #    ${TCMALLOC_LIBRARIES}
            #    ${WINFLAGS}
            #)
        endif (${CMAKE_BUILD_SAMPLES})
    # Если операционной системой является Linux, FreeBSD, NetBSD, OpenBSD, MacOS X, Solaris
    else()
        # Выполняем сборку динамической библиотеки
        target_link_libraries(
            ${AWH_LIB_NAME}
            PRIVATE
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
        )

        # Если необходимо собрать семплы
        if (${CMAKE_BUILD_SAMPLES})
            # Если нужно собрать тесты для работы с ОС
            if (${CMAKE_BUILD_IP_NET_APP_SAMPLE})
                # Выполняем сборку приложения работы с IP-адресами
                target_link_libraries(
                    ${IP_APP_NET_NAME}
                    ${AWH_LIB_NAME}
                    ${TCMALLOC_LIBRARIES}
                )
            endif (${CMAKE_BUILD_IP_NET_APP_SAMPLE})

            # Если нужно собрать тесты для работы с датой и временем
            if (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})
                # Выполняем сборку приложения даты и времени
                target_link_libraries(
                    ${DATE_APP_SYS_NAME}
                    ${AWH_LIB_NAME}
                    ${TCMALLOC_LIBRARIES}
                )
            endif (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})

            # Если нужно собрать тесты для работы с каталогами
            if (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})
                # Выполняем сборку приложения работы с каталогами
                target_link_libraries(
                    ${DIR_APP_IO_NAME}
                    ${AWH_LIB_NAME}
                    ${TCMALLOC_LIBRARIES}
                )
            endif (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})

            # Если нужно собрать тесты для работы с файлами
            if (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})
                # Выполняем сборку приложения работы с файлами
                target_link_libraries(
                    ${FILE_APP_IO_NAME}
                    ${AWH_LIB_NAME}
                    ${TCMALLOC_LIBRARIES}
                )
            endif (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})

            # Если нужно собрать тесты для работы с таймером
            if (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})
                # Выполняем сборку приложения таймера
                target_link_libraries(
                    ${TIMER_APP_IO_NAME}
                    ${AWH_LIB_NAME}
                    ${TCMALLOC_LIBRARIES}
                )
            endif (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})

            # Если нужно собрать тесты для работы с пользовательскими событиями
            if (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})
                # Выполняем сборку приложения пользовательских событий
                target_link_libraries(
                    ${EVENTS_APP_IO_NAME}
                    ${AWH_LIB_NAME}
                    ${TCMALLOC_LIBRARIES}
                )
            endif (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})

            # Выполняем сборку приложения межпроцессного взаимодействия
            #target_link_libraries(
            #    ${IPC_APP_NAME}
            #    ${AWH_LIB_NAME}
            #    ${TCMALLOC_LIBRARIES}
            #)

            # Выполняем сборку приложения клиента
            #target_link_libraries(
            #    ${CLIENT_APP_NAME}
            #    ${AWH_LIB_NAME}
            #    ${TCMALLOC_LIBRARIES}
            #)

            # Выполняем сборку приложения сервера
            #target_link_libraries(
            #    ${SERVER_APP_NAME}
            #    ${AWH_LIB_NAME}
            #    ${TCMALLOC_LIBRARIES}
            #)
        endif (${CMAKE_BUILD_SAMPLES})
    endif()
# Если нужно собрать и статическую библиотеку
else (CMAKE_SHARED_BUILD_LIB)
    # Если операционной системой является Windows
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
        # Если необходимо собрать семплы
        if (${CMAKE_BUILD_SAMPLES})
            # Если нужно собрать тесты для работы с ОС
            if (${CMAKE_BUILD_IP_NET_APP_SAMPLE})
                # Выполняем сборку приложения работы с IP-адресами
                target_link_libraries(
                    ${IP_APP_NET_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                    ${TCMALLOC_LIBRARIES}
                    ${WINFLAGS}
                )
            endif (${CMAKE_BUILD_IP_NET_APP_SAMPLE})

            # Если нужно собрать тесты для работы с датой и временем
            if (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})
                # Выполняем сборку приложения даты и времени
                target_link_libraries(
                    ${DATE_APP_SYS_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                    ${TCMALLOC_LIBRARIES}
                    ${WINFLAGS}
                )
            endif (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})

            # Если нужно собрать тесты для работы с каталогами
            if (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})
                # Выполняем сборку приложения работы с каталогами
                target_link_libraries(
                    ${DIR_APP_IO_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                    ${TCMALLOC_LIBRARIES}
                    ${WINFLAGS}
                )
            endif (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})

            # Если нужно собрать тесты для работы с файлами
            if (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})
                # Выполняем сборку приложения работы с файлами
                target_link_libraries(
                    ${FILE_APP_IO_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                    ${TCMALLOC_LIBRARIES}
                    ${WINFLAGS}
                )
            endif (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})

            # Если нужно собрать тесты для работы с таймером
            if (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})
                # Выполняем сборку приложения таймера
                target_link_libraries(
                    ${TIMER_APP_IO_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                    ${TCMALLOC_LIBRARIES}
                    ${WINFLAGS}
                )
            endif (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})

            # Если нужно собрать тесты для работы с пользовательскими событиями
            if (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})
                # Выполняем сборку приложения пользовательских событий
                target_link_libraries(
                    ${EVENTS_APP_IO_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                    ${TCMALLOC_LIBRARIES}
                    ${WINFLAGS}
                )
            endif (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})

            # Выполняем сборку приложения межпроцессного взаимодействия
            #target_link_libraries(
            #    ${IPC_APP_NAME}
            #    ${AWH_LIB_NAME}
            #    ${DEPEND_LIBRARY}
            #    ${TCMALLOC_LIBRARIES}
            #    ${WINFLAGS}
            #)

            # Выполняем сборку приложения клиента
            #target_link_libraries(
            #    ${CLIENT_APP_NAME}
            #    ${AWH_LIB_NAME}
            #    ${DEPEND_LIBRARY}
            #    ${TCMALLOC_LIBRARIES}
            #    ${WINFLAGS}
            #)

            # Выполняем сборку приложения сервера
            #target_link_libraries(
            #    ${SERVER_APP_NAME}
            #    ${AWH_LIB_NAME}
            #    ${DEPEND_LIBRARY}
            #    ${TCMALLOC_LIBRARIES}
            #    ${WINFLAGS}
            #)
        endif (${CMAKE_BUILD_SAMPLES})
    # Если операционной системой является Linux, FreeBSD, NetBSD, OpenBSD, MacOS X, Solaris
    else()
        # Если необходимо собрать семплы
        if (${CMAKE_BUILD_SAMPLES})
            # Если нужно собрать тесты для работы с ОС
            if (${CMAKE_BUILD_IP_NET_APP_SAMPLE})
                # Выполняем сборку приложения работы с IP-адресами
                target_link_libraries(
                    ${IP_APP_NET_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                    ${TCMALLOC_LIBRARIES}
                )
            endif (${CMAKE_BUILD_IP_NET_APP_SAMPLE})

            # Если нужно собрать тесты для работы с датой и временем
            if (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})
                # Выполняем сборку приложения даты и времени
                target_link_libraries(
                    ${DATE_APP_SYS_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                    ${TCMALLOC_LIBRARIES}
                )
            endif (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})

            # Если нужно собрать тесты для работы с каталогами
            if (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})
                # Выполняем сборку приложения работы с каталогами
                target_link_libraries(
                    ${DIR_APP_IO_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                    ${TCMALLOC_LIBRARIES}
                )
            endif (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})

            # Если нужно собрать тесты для работы с файлами
            if (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})
                # Выполняем сборку приложения работы с файлами
                target_link_libraries(
                    ${FILE_APP_IO_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                    ${TCMALLOC_LIBRARIES}
                )
            endif (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})

            # Если нужно собрать тесты для работы с таймером
            if (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})
                # Выполняем сборку приложения таймера
                target_link_libraries(
                    ${TIMER_APP_IO_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                    ${TCMALLOC_LIBRARIES}
                )
            endif (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})

            # Если нужно собрать тесты для работы с пользовательскими событиями
            if (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})
                # Выполняем сборку приложения пользовательских событий
                target_link_libraries(
                    ${EVENTS_APP_IO_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                    ${TCMALLOC_LIBRARIES}
                )
            endif (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})

            # Выполняем сборку приложения межпроцессного взаимодействия
            #target_link_libraries(
            #    ${IPC_APP_NAME}
            #    ${AWH_LIB_NAME}
            #    ${DEPEND_LIBRARY}
            #    ${TCMALLOC_LIBRARIES}
            #)

            # Выполняем сборку приложения клиента
            #target_link_libraries(
            #    ${CLIENT_APP_NAME}
            #    ${AWH_LIB_NAME}
            #    ${DEPEND_LIBRARY}
            #    ${TCMALLOC_LIBRARIES}
            #)

            # Выполняем сборку приложения сервера
            #target_link_libraries(
            #    ${SERVER_APP_NAME}
            #    ${AWH_LIB_NAME}
            #    ${DEPEND_LIBRARY}
            #    ${TCMALLOC_LIBRARIES}
            #)
        endif (${CMAKE_BUILD_SAMPLES})
    endif()
endif (CMAKE_SHARED_BUILD_LIB)

# Если необходимо собрать семплы
if (${CMAKE_BUILD_SAMPLES})
    # Активация формирования дампа ядра в реже отладки под MacOS X
    if (CMAKE_BUILD_TYPE MATCHES Debug)
        if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
            SET(entitlements_file "${CMAKE_SOURCE_DIR}/segv.entitlements")
            # Если нужно собрать тесты для работы с ОС
            if (${CMAKE_BUILD_IP_NET_APP_SAMPLE})
                add_custom_command(TARGET "${IP_APP_NET_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${IP_APP_NET_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${IP_APP_NET_NAME}\" to enable core dump on MacOS X"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_IP_NET_APP_SAMPLE})

            # Если нужно собрать тесты для работы с датой и временем
            if (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})
                add_custom_command(TARGET "${DATE_APP_SYS_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${DATE_APP_SYS_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${DATE_APP_SYS_NAME}\" to enable core dump on MacOS X"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})

            # Если нужно собрать тесты для работы с каталогами
            if (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})
                add_custom_command(TARGET "${DIR_APP_IO_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${DIR_APP_IO_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${DIR_APP_IO_NAME}\" to enable core dump on MacOS X"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})

            # Если нужно собрать тесты для работы с файлами
            if (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})
                add_custom_command(TARGET "${FILE_APP_IO_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${FILE_APP_IO_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${FILE_APP_IO_NAME}\" to enable core dump on MacOS X"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})

            # Если нужно собрать тесты для работы с таймером
            if (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})
                add_custom_command(TARGET "${TIMER_APP_IO_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${TIMER_APP_IO_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${TIMER_APP_IO_NAME}\" to enable core dump on MacOS X"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})

            # Если нужно собрать тесты для работы с пользовательскими событиями
            if (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})
                add_custom_command(TARGET "${EVENTS_APP_IO_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${EVENTS_APP_IO_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${EVENTS_APP_IO_NAME}\" to enable core dump on MacOS X"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})

            #add_custom_command(TARGET "${IPC_APP_NAME}" POST_BUILD
            #    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${IPC_APP_NAME}>" "${entitlements_file}"
            #    COMMENT "Sign target \"${IPC_APP_NAME}\" to enable core dump on MacOS X"
            #    VERBATIM
            #)

            #add_custom_command(TARGET "${CLIENT_APP_NAME}" POST_BUILD
            #    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CLIENT_APP_NAME}>" "${entitlements_file}"
            #    COMMENT "Sign target \"${CLIENT_APP_NAME}\" to enable core dump on MacOS X"
            #    VERBATIM
            #)

            #add_custom_command(TARGET "${SERVER_APP_NAME}" POST_BUILD
            #    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${SERVER_APP_NAME}>" "${entitlements_file}"
            #    COMMENT "Sign target \"${SERVER_APP_NAME}\" to enable core dump on MacOS X"
            #    VERBATIM
            #)

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

            # Если нужно собрать тесты для работы с ОС
            if (${CMAKE_BUILD_IP_NET_APP_SAMPLE})
                # Выполняем анализ приложения работы с IP-адресами
                pvs_studio_add_target(TARGET ${IP_APP_NET_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${IP_APP_NET_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_IP_NET_APP_SAMPLE})

            # Если нужно собрать тесты для работы с датой и временем
            if (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})
                # Выполняем анализ приложения даты и времени
                pvs_studio_add_target(TARGET ${DATE_APP_SYS_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${DATE_APP_SYS_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})

            # Если нужно собрать тесты для работы с каталогами
            if (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})
                # Выполняем анализ приложения работы с каталогами
                pvs_studio_add_target(TARGET ${DIR_APP_IO_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${DIR_APP_IO_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})

            # Если нужно собрать тесты для работы с файлами
            if (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})
                # Выполняем анализ приложения работы с файлами
                pvs_studio_add_target(TARGET ${FILE_APP_IO_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${FILE_APP_IO_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})

            # Если нужно собрать тесты для работы с таймером
            if (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})
                # Выполняем анализ приложения таймера
                pvs_studio_add_target(TARGET ${TIMER_APP_IO_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${TIMER_APP_IO_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})

            # Если нужно собрать тесты для работы с пользовательскими событиями
            if (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})
                # Выполняем анализ приложения пользовательских событий
                pvs_studio_add_target(TARGET ${EVENTS_APP_IO_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${EVENTS_APP_IO_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})

            #pvs_studio_add_target(TARGET ${IPC_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${IPC_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            #pvs_studio_add_target(TARGET ${CLIENT_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CLIENT_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            #pvs_studio_add_target(TARGET ${SERVER_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SERVER_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
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
    else()
        # Выполняем установку библиотеки зависимостей
        install(FILES ${DEPEND_LIBRARY} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
    endif()
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
endif (CMAKE_BUILD_TESTS)

# Если операционной системой является Windows
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
# Если операцинная система относится к Nix-подобной
else()
    # Устанавливаем статическую библиотеку AWH
    install(TARGETS ${AWH_LIB_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
endif()

# Устанавливаем статическую библиотеку TcMalloc
install(FILES ${TCMALLOC_LIBRARIES} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")

# Если необходимо собрать семплы
if (${CMAKE_BUILD_SAMPLES})
    # Если нужно собрать тесты для работы с ОС
    if (${CMAKE_BUILD_IP_NET_APP_SAMPLE})
        # Устанавливаем приложение работы с IP-адресами
        install(TARGETS ${IP_APP_NET_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_IP_NET_APP_SAMPLE})

    # Если нужно собрать тесты для работы с датой и временем
    if (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})
        # Устанавливаем приложение работы с датой и временем
        install(TARGETS ${DATE_APP_SYS_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})

    # Если нужно собрать тесты для работы с каталогами
    if (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})
        # Устанавливаем приложение работы с каталогами
        install(TARGETS ${DIR_APP_IO_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})

    # Если нужно собрать тесты для работы с файлами
    if (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})
        # Устанавливаем приложение работы с файлами
        install(TARGETS ${FILE_APP_IO_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})

    # Если нужно собрать тесты для работы с таймером
    if (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})
        # Устанавливаем приложение работы с таймером
        install(TARGETS ${TIMER_APP_IO_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})

    # Если нужно собрать тесты для работы с пользовательскими событиями
    if (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})
        # Устанавливаем приложение работы с пользовательскими событиями
        install(TARGETS ${EVENTS_APP_IO_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})

    #install(TARGETS ${IPC_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    #install(TARGETS ${CLIENT_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    #install(TARGETS ${SERVER_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")

    # Если операционной системой не является Windows
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
