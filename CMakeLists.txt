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

##
 # Указываем что проверки нужно собрать под санитайзерами
 #
 # @note Ключ объявляется здесь, а не рядом с проверками: от него зависит выбор
 #       распределителя памяти, а тот назначается прежде
 ##
option(CMAKE_BUILD_SANITIZE "Build unit tests with sanitizers" OFF)

# Набор включаемых санитайзеров
SET(CMAKE_BUILD_SANITIZE_LIST "address,undefined" CACHE STRING "Sanitizers enabled for unit tests")

# Если сборка производится под платформы Apple
if (APPLE)
    ##
     # Определяем, является ли целевая платформа настольной macOS.
     #
     # Переменную IOS CMake выставляет только для iOS, поэтому условие вида
     # "APPLE AND NOT IOS" считает настольной системой ещё и tvOS, watchOS и
     # visionOS, и они получают правила macOS - вплоть до линковки Foundation,
     # который им не нужен. Отличать их приходится по имени системы.
     ##
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
        SET(AWH_TARGET_MACOS YES)
    else()
        SET(AWH_TARGET_MACOS NO)
    endif()

    ##
     # Отказ от разбора alias-файлов macOS.
     #
     # Разбор включён по умолчанию: alias есть в настольной файловой системе, и
     # приложение вправе ожидать, что фреймворк его понимает. Отключать имеет
     # смысл там, где процесс не обращается к Foundation больше ни для чего -
     # службы и утилиты, - потому что только этот разбор и тянет в процесс связку
     # libobjc, CoreFoundation и Foundation.
     #
     # cmake -DCMAKE_NO_MACOS_ALIAS=YES ..
     ##
    if (AWH_TARGET_MACOS)
        if (CMAKE_NO_MACOS_ALIAS)
            add_compile_definitions(AWH_NO_MACOS_ALIAS)
            message(STATUS "Build with macOS alias resolution: NO\n")
        else()
            message(STATUS "Build with macOS alias resolution: YES\n")
        endif()
    endif()
endif()

# Если необходимо задать имя AWH
if (CMAKE_AWH_NAME)
    add_compile_definitions(AWH_NAME=${CMAKE_AWH_NAME})
    message(STATUS "Build with name: ${CMAKE_AWH_NAME}\n")
endif (CMAKE_AWH_NAME)

# Если необходимо задать короткое имя AWH
if (CMAKE_AWH_SHORT_NAME)
    add_compile_definitions(AWH_SHORT_NAME=${CMAKE_AWH_SHORT_NAME})
    message(STATUS "Build with short name: ${CMAKE_AWH_SHORT_NAME}\n")
endif (CMAKE_AWH_SHORT_NAME)

# Если необходимо задать версию AWH
if (CMAKE_AWH_VERSION)
    add_compile_definitions(AWH_VERSION=${CMAKE_AWH_VERSION})
    message(STATUS "Build with version: ${CMAKE_AWH_VERSION}\n")
endif (CMAKE_AWH_VERSION)

# Если необходимо задать основной хост AWH
if (CMAKE_AWH_HOST)
    add_compile_definitions(AWH_HOST=${CMAKE_AWH_HOST})
    message(STATUS "Build with host: ${CMAKE_AWH_HOST}\n")
endif (CMAKE_AWH_HOST)

# Если необходимо задать адрес сайта AWH
if (CMAKE_AWH_SITE)
    add_compile_definitions(AWH_SITE=${CMAKE_AWH_SITE})
    message(STATUS "Build with site: ${CMAKE_AWH_SITE}\n")
endif (CMAKE_AWH_SITE)

# Если необходимо задать адрес электронной почты AWH
if (CMAKE_AWH_EMAIL)
    add_compile_definitions(AWH_EMAIL=${CMAKE_AWH_EMAIL})
    message(STATUS "Build with email: ${CMAKE_AWH_EMAIL}\n")
endif (CMAKE_AWH_EMAIL)

# Если необходимо задать контактный адрес AWH
if (CMAKE_AWH_CONTACT)
    add_compile_definitions(AWH_CONTACT=${CMAKE_AWH_CONTACT})
    message(STATUS "Build with contact: ${CMAKE_AWH_CONTACT}\n")
endif (CMAKE_AWH_CONTACT)

# Если необходимо задать локаль AWH
if (CMAKE_AWH_LOCALE)
    add_compile_definitions(AWH_LOCALE=${CMAKE_AWH_LOCALE})
    message(STATUS "Build with locale: ${CMAKE_AWH_LOCALE}\n")
endif (CMAKE_AWH_LOCALE)

 # Приведение доменных имён модулем awh::idna вместо средств операционной системы
 #
 # Средствами операционной системы доменные имена приводятся лишь на MS Windows,
 # где приведение задано прежним изданием стандарта и с приложением по обработке
 # доменных имён сходится не во всём. Признак переводит приведение на модуль,
 # чем поведение на всех операционных системах делается одинаковым. На прочих
 # операционных системах приведение выполняется модулем всегда.
 ##
if (CMAKE_BUILD_IDNA)
    add_compile_definitions(AWH_IDNA)
    message(STATUS "Build with own IDNA processing: YES\n")
# Приведение доменных имён средствами операционной системы
else (CMAKE_BUILD_IDNA)
    message(STATUS "Build with own IDNA processing: NO\n")
endif (CMAKE_BUILD_IDNA)

# Если активирован флаг оптимизации операционной системы
if (CMAKE_BOOSTING_NET)
    add_compile_definitions(AWH_BOOSTING_NET)
    message(STATUS "Build with NET boosting: YES\n")
endif (CMAKE_BOOSTING_NET)

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

##
 # Настройки массива опрашиваемых событий.
 #
 # Размер массива подбирается по нагрузке сам: он удваивается, когда опрос вернул
 # его заполненным целиком, и ужимается вдвое после долгого затишья. Задавать
 # приходится только границы этого подбора и длительность затишья.
 ##

# Если активирован флаг установки начального размера массива опрашиваемых событий (по умолчанию: 64)
if (CMAKE_MAX_POLL_EVENTS_COUNT)
    add_compile_definitions(AWH_MAX_POLL_EVENTS_COUNT=${CMAKE_MAX_POLL_EVENTS_COUNT})
    message(STATUS "Build with initial poll events count: ${CMAKE_MAX_POLL_EVENTS_COUNT}\n")
endif (CMAKE_MAX_POLL_EVENTS_COUNT)

##
 # Если активирован флаг установки числа заводимых вперёд принятий входящих связей (по умолчанию: 8)
 #
 # Величина эта касается одного лишь движка на портах завершения: у kqueue и epoll
 # принятие заранее не заводится вовсе, там ядро сообщает о готовности слушающего
 # сокета, а сокет принятой связи заводит сам вызов принятия
 ##
if (CMAKE_COUNT_PREPOSTED_ACCEPTS)
    add_compile_definitions(AWH_COUNT_PREPOSTED_ACCEPTS=${CMAKE_COUNT_PREPOSTED_ACCEPTS})
    message(STATUS "Build with pre-posted accepts count: ${CMAKE_COUNT_PREPOSTED_ACCEPTS}\n")
endif (CMAKE_COUNT_PREPOSTED_ACCEPTS)

# Если активирован флаг установки верхнего предела роста массива опрашиваемых событий (по умолчанию: 4096)
if (CMAKE_MAX_POLL_EVENTS_LIMIT)
    add_compile_definitions(AWH_MAX_POLL_EVENTS_LIMIT=${CMAKE_MAX_POLL_EVENTS_LIMIT})
    message(STATUS "Build with max poll events limit: ${CMAKE_MAX_POLL_EVENTS_LIMIT}\n")
endif (CMAKE_MAX_POLL_EVENTS_LIMIT)

# Если активирован флаг установки длительности затишья для сжатия массива событий (по умолчанию: 1024 опроса)
if (CMAKE_POLL_EVENTS_IDLE_ROUNDS)
    add_compile_definitions(AWH_POLL_EVENTS_IDLE_ROUNDS=${CMAKE_POLL_EVENTS_IDLE_ROUNDS})
    message(STATUS "Build with poll events idle rounds: ${CMAKE_POLL_EVENTS_IDLE_ROUNDS}\n")
endif (CMAKE_POLL_EVENTS_IDLE_ROUNDS)

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

# Если активирован флаг установки размера блока для потокового сжатия (по умолчанию: 16384 = 16 КБ)
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

# Если активирован флаг установки количества идентификаторов для внутренних таймеров (по умолчанию: 8)
if (CMAKE_COUNT_IDS_INTERNAL_TIMER)
    add_compile_definitions(AWH_COUNT_IDS_INTERNAL_TIMER=${CMAKE_COUNT_IDS_INTERNAL_TIMER})
    message(STATUS "Build with count ids for internal timers: ${CMAKE_COUNT_IDS_INTERNAL_TIMER}\n")
endif (CMAKE_COUNT_IDS_INTERNAL_TIMER)

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

##
 # У MS Windows набор команд читается из названия системы, а не у uname -m
 #
 # Оболочка MSYS2 исполняется под подражанием x86-64, и uname -m отвечает x86_64 даже
 # на машине ARM64 - вслед за ним и uname -p, отдающий unknown. Строка же названия
 # системы оканчивается на ARM64 при любой из оболочек MSYS2, а у машины x86-64
 # суффикса не несёт вовсе - проверено опытом на обоих стендах Windows
 #
 # Цена ошибки не отвлечённая: приняв ARM64 за x86-64, сборка подставляет -mrdrnd и
 # -march=core2, и компилятор отвечает "unsupported option '-mrdrnd' for target
 # 'aarch64-w64-windows-gnu'"
 #
 # Тем же признаком пользуется и sh/build_third_party.sh, чтобы опознание было единым
 ##
EXECUTE_PROCESS(COMMAND uname -s OUTPUT_VARIABLE SYSTEM_NAME OUTPUT_STRIP_TRAILING_WHITESPACE)
if (SYSTEM_NAME MATCHES "ARM64")
    SET(ARCHITECTURE "aarch64")
endif()

message(STATUS "Architecture: ${ARCHITECTURE}")

# Если сборка производится под операционную систему Linux
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    message(STATUS "Detected OS: Linux")
    SET(DEBUGGER "-ggdb3")
    ##
     # Подключаем библиотеку SCTP
     #
     # Вызовы SCTP у Linux вынесены в отдельную библиотеку (lksctp-tools), тогда как
     # FreeBSD, Solaris и illumos держат их в самой стандартной библиотеке и ничего
     # подключать не требуют. Разница эта здешняя, а не общая.
     #
     # Подключается она здесь, а не списком библиотек ниже, оттого что наборы тестов
     # заводятся раньше того списка и до него не достают
     #
     # ПОРЯДОК ЗДЕСЬ ЗНАЧИМ. Флаги компилятора попадают в строку связывания ДО
     # объектных файлов, а связыватель разбирает её слева направо. Библиотека,
     # разбираемая раньше объектов, никому на тот миг ещё не нужна, и Debian с его
     # умолчанием --as-needed отбрасывает её целиком: сборка валится россыпью
     # "undefined reference to sctp_connectx". Прочие дистрибутивы это переживали
     # лишь потому, что умолчание у них иное. Признак --no-as-needed обязан стоять
     # ПЕРЕД самой библиотекой, иначе он до неё не относится - проверено опытом на
     # стенде Debian 12.15 (12.08.2026): порядок "-lsctp объект" не связывается,
     # "-Wl,--no-as-needed -lsctp объект" связывается
     ##
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--no-as-needed -lsctp")
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
        ##
         # Признак --no-as-needed здесь не задаётся намеренно
         #
         # Приём этот принадлежит компоновщику GNU, а у Sun Solaris и illumos работает
         # свой - Solaris Link Editors. Solaris 11.4 признак этот принимает, а illumos
         # отвечает отказом "ld: fatal: unrecognized option '--no-as-needed'", и наборы
         # проверок там не компонуются вовсе. Проверено опытом на обоих стендах
         #
         # Смысла он тут и не несёт: назначение его - удержать библиотеку, которую
         # компоновщик счёл лишней, а нужна была лишь одна такая - libdl. У этих же
         # систем dlopen лежит в libc, и замер показал, что сборка проходит и вовсе
         # без -ldl. Библиотека оставлена ради явности, признак убран
         ##
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ldl")
    endif()
    # Добавляем расширения Sun Solaris
    add_compile_definitions(__EXTENSIONS__)
    ##
     # Выполняем проверку наличия вызова заведения связи SCTP
     #
     # Вызов этот отдаёт опознаватель связи сразу, при её заведении, и только им
     # можно слать по сокету упорядоченных сообщений: прежний вызов отправки
     # опознавателя среди доводов не несёт вовсе, а выбирать связь по адресу
     # получателя эти системы позволяют лишь первой посылке
     #
     # Системы Sun здесь расходятся: Solaris 11.4 вызов несёт, а illumos не несёт
     # ни в заголовках, ни в самой библиотеке - проверено на обоих стендах. Оттого
     # наличие его выясняется сборкой, а не признаком системы: под обеими __sun
     # объявлен одинаково
     ##
    INCLUDE(CheckCXXSymbolExists)
    SET(CMAKE_REQUIRED_LIBRARIES sctp)
    CHECK_CXX_SYMBOL_EXISTS(sctp_connectx "netinet/sctp.h" AWH_HAVE_SCTP_CONNECTX)
    UNSET(CMAKE_REQUIRED_LIBRARIES)

    # Если вызов заведения связи SCTP системой поддерживается
    if (AWH_HAVE_SCTP_CONNECTX)
        # Выполняем объявление признака наличия вызова заведения связи SCTP
        ADD_DEFINITIONS(-DAWH_HAVE_SCTP_CONNECTX)
    endif()
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
##
 # Если сборка производится под операционную систему DragonFly BSD
 #
 # Система эта ответвилась от FreeBSD 4.8 и очередь оповещений несёт родную - kqueue,
 # оттого и движок ей подходит тот же, что и прочим BSD. Расходится она с FreeBSD в
 # двух местах: SCTP там нет вовсе (проверено на стенде - ни заголовка, ни настроек
 # в sysctl), а основной собиратель системы - gcc 8.3 от 2019 года, который наш код
 # не осиливает, потому собирать следует собирателем из портов
 ##
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "DragonFly")
    ##
     # Версия берётся из uname -r, а не разбором uname -a
     #
     # Поля в выводе uname -a у DragonFly расставлены иначе, нежели у FreeBSD: третьим
     # там идёт не номер выпуска, а название самой системы, и разбор отдавал версию
     # "DragonFly" вместо "6.4-RELEASE". Проверено на стенде 12.08.2026
     ##
    if (NOT OS_VERSION)
        EXECUTE_PROCESS(COMMAND uname -r COMMAND tr -d '\n' OUTPUT_VARIABLE OS_VERSION)
    endif()
    message(STATUS "Detected OS: DragonFly BSD v${OS_VERSION}")
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
    SET(AWH_PARAMS "-Wno-unused-command-line-argument")
# Устанавливаем параметры игнорирования ошибок для Solaris
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
    ##
     # Библиотека libdladm нужна модулю сетевых интерфейсов: приёмов заведения и
     # сноса устройств передачи данных, какие есть у BSD, эти системы не имеют
     #
     # Библиотека libsctp несёт приёмы протокола SCTP - sctp_sendmsg, sctp_recvmsg,
     # sctp_peeloff и sctp_connectx. У FreeBSD они лежат в самой libc, здесь же
     # вынесены отдельно, и без неё сборка валится на неразрешённых именах уже при
     # сборке набора проверок, тогда как сама библиотека собирается молча. Ставить
     # её отдельно, как это делается у Linux, не требуется - она входит в состав обеих систем
     ##
    SET(AWH_PARAMS "-lsocket -lnsl -ldladm -lsctp -lc")
# Устанавливаем параметры игнорирования ошибок для Windows и Linux
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Windows" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    SET(AWH_PARAMS "")
# Устанавливаем параметры игнорирования ошибок для FreeBSD, DragonFly, NetBSD и OpenBSD
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "DragonFly" OR ${CMAKE_SYSTEM_NAME} STREQUAL "NetBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
    SET(AWH_PARAMS "-lutil -lexecinfo -Wno-unused-command-line-argument")
# Если операционная система не определена
else()
    SET(AWH_PARAMS "-Wno-unknown-attributes")
endif()

# Если процессор архитектуры ARM
if (${ARCHITECTURE} MATCHES "aarch64" OR ${ARCHITECTURE} MATCHES "arm*")
    SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -pipe -DNDEBUG")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall -Wextra ${AWH_PARAMS}")
# Если процессор архитектуры x86_64
else()
    SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -pipe -mrdrnd -march=core2 -DNDEBUG")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall -Wextra ${AWH_PARAMS}")
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
        ##
         # Стандартная библиотека musl исключена третьей причиной
         #
         # Признак профилирования требует особого стартового объекта gcrt1.o, а musl
         # его не поставляет вовсе - профилирования gprof она не поддерживает. Связывание
         # валится с "cannot find gcrt1.o", причём на самом последнем шаге, когда всё уже
         # собрано. Проверено на стенде Alpine 3.24 (12.08.2026)
         #
         # Опознаётся musl по цели собирателя: у Alpine она оканчивается на -musl.
         # Признака в самом CMake для этого нет, а по названию системы musl от glibc
         # не отличить - обе зовутся Linux
         ##
        SET(AWH_LIBC_MUSL FALSE)
        ##
         # Определяем, что сборка производится под Linux
         #
         ##
        if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
            EXECUTE_PROCESS(COMMAND ${CMAKE_CXX_COMPILER} -dumpmachine OUTPUT_VARIABLE AWH_COMPILER_TARGET OUTPUT_STRIP_TRAILING_WHITESPACE)
            ##
             # Определяем, что сборка производится под musl
             #
             # Проверка на musl нужна, потому что gprof там не поддерживается. Проверено
             # на стенде Alpine 3.24 (12.08.2026): сборка с признаком -pg валится с
             # "cannot find gcrt1.o", а без признака - работает
             ##
            if (AWH_COMPILER_TARGET MATCHES "musl")
                SET(AWH_LIBC_MUSL TRUE)
                message(STATUS "Detected libc: musl (${AWH_COMPILER_TARGET}) — gprof profiling is not available")
            endif()
        endif()
        ##
         # Сборка профилировщика gprof.
         #
         # Платформы Apple исключены: их компилятор флаг принимает, но профиль не
         # ведёт - gmon.out не появляется, - а во флагах компоновки он вдобавок
         # достаётся сборке, где компилировать нечего, и та отвечает
         # предупреждением о неиспользованном аргументе. Подавление предупреждений
         # прописано только во флагах C++, поэтому счётчик системных вызовов,
         # собираемый как библиотека на C, оставался с ним один на один
         #
         # OpenBSD исключена по иной причине: там признак профилирования подключает
         # статическую библиотеку профилирования вместо разделяемой libc, а
         # библиотека потоков остаётся разделяемой и своих символов не находит -
         # загрузчик отвечает отказом ещё до начала работы. Проверяется это тремя
         # строками: программа с одним потоком, собранная с признаком, падает, а без
         # него работает. К самому коду это отношения не имеет
         #
         # NetBSD исключена по причине, близкой к OpenBSD, но не той же: признак
         # профилирования подключает статическую libc, и её подготовка памяти
         # потока (__libc_static_tls_setup) обращается к dl_iterate_phdr,
         # который в такой сборке остаётся заглушкой - загрузчик валится по
         # обращению к пустому адресу ещё до начала работы. Проверено теми же
         # тремя строками на стенде NetBSD 10.1/amd64 (13.08.2026): вывод одной
         # строки с признаком даёт отказ по сигналу, без признака - работает
         ##
        ##
         # Если сборка производится не под Windows, macOS, OpenBSD или NetBSD
         ##
        if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows" AND NOT APPLE AND NOT ${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD" AND NOT ${CMAKE_SYSTEM_NAME} STREQUAL "NetBSD" AND NOT AWH_LIBC_MUSL)
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
    message(STATUS "Snappy include dir found: " ${SNAPPY_INCLUDE_DIR})
    message(STATUS "Lizard include dir found: " ${LIZARD_INCLUDE_DIR})
    message(STATUS "Density include dir found: " ${DENSITY_INCLUDE_DIR})
    message(STATUS "NgHttp2 include dir found: " ${NGHTTP2_INCLUDE_DIR})
    message(STATUS "BoringSSL include dir found: " ${BORINGSSL_INCLUDE_DIR})
    message(STATUS "Brotli include dir found: " ${BROTLI_INCLUDE_ENCODE_DIR} " " ${BROTLI_INCLUDE_DECODE_DIR})
# Если библиотека зависимостей не найдена
else (Dependence_FOUND)
    # Выводим сообщение об отсутствии библиотеки зависимостей
    message(FATAL_ERROR "Dependence not found")
endif (Dependence_FOUND)

# Если Doxygen найден
if(Doxygen_FOUND)
    # Предположим, что Doxyfile лежит в корне исходников
    SET(DOXYFILE_PATH ${CMAKE_SOURCE_DIR}/Doxyfile)

    # Проверяем, что файл существует
    if(EXISTS ${DOXYFILE_PATH})
        # Выводим сообщение, что мы нашли файл для генерации документации Doxyfile
        message(STATUS "Doxyfile found at ${DOXYFILE_PATH}\n")

        # Языки, для которых собирается документация: суффикс файла настроек -> имя языка
        SET(DOXYGEN_LANGUAGES ru en)

        # Список целей по языкам, собираемый ниже для общей цели «doc»
        SET(DOXYGEN_LANGUAGE_TARGETS)

        # Перебираем все языки документации
        foreach(DOXYGEN_LANGUAGE IN LISTS DOXYGEN_LANGUAGES)
            # Формируем путь к файлу настроек конкретного языка
            SET(DOXYFILE_LANGUAGE_PATH ${CMAKE_SOURCE_DIR}/Doxyfile.${DOXYGEN_LANGUAGE})

            # Проверяем, что файл настроек языка существует
            if(EXISTS ${DOXYFILE_LANGUAGE_PATH})
                # Создаём цель сборки документации на конкретном языке
                add_custom_target(doc-${DOXYGEN_LANGUAGE}
                    # Каталог назначения создаём сами: Doxygen заводит лишь один уровень вложенности
                    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/doc/${DOXYGEN_LANGUAGE}
                    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_LANGUAGE_PATH}
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                    COMMENT "Generating API documentation with Doxygen [${DOXYGEN_LANGUAGE}]"
                    VERBATIM
                )
                # Запоминаем цель для общей сборки
                LIST(APPEND DOXYGEN_LANGUAGE_TARGETS doc-${DOXYGEN_LANGUAGE})
            # Если файл настроек языка не найден
            else()
                # Выводим сообщение об отсутствии файла настроек языка
                message(STATUS "Doxyfile.${DOXYGEN_LANGUAGE} not found — doc-${DOXYGEN_LANGUAGE} target skipped\n")
            # Конец проверки файла настроек языка
            endif()
        # Конец перебора языков документации
        endforeach()

        # Если хотя бы одна языковая цель создана
        if(DOXYGEN_LANGUAGE_TARGETS)
            # Создаём общую цель, собирающую документацию на всех языках
            add_custom_target(doc DEPENDS ${DOXYGEN_LANGUAGE_TARGETS})
        # Если языковых файлов настроек нет вовсе
        else()
            # Создаём кастомную цель на основе базового файла настроек
            add_custom_target(doc
                COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_PATH}
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                COMMENT "Generating API documentation with Doxygen"
                VERBATIM
            )
        # Конец проверки языковых целей
        endif()
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

##
 # Выполняем подключение хидеров
 #
 # @warning Пометка SYSTEM здесь роздана НАОБОРОТ здравому смыслу: наши собственные
 #          заголовки числятся системными (предупреждения в них глохнут), а третья
 #          сторона - обычными (её шум идёт в вывод сборки). Правка этого разобрана
 #          26.08.2026 и ОТВЕРГНУТА ОПЫТОМ, а не забыта:
 #
 #          - выгоды нет вовсе: скрытых предупреждений в наших заголовках не нашлось ни
 #            одного (202 исходника, -Wall -Wextra, сличение обоих укладов);
 #          - цена немалая: перевод третьей стороны на SYSTEM переставляет её каталоги
 #            ПОСЛЕ всех обычных в строке собирателя, и заголовки BoringSSL перестают
 #            находиться - сборка встала с 40 ошибками вида «unknown type name 'CBB'».
 #
 #          Сличение имён заголовков этого не предсказало и предсказать не могло: у нас с
 #          третьей стороной одноимённых нет вовсе (221 против 252), а спорят они не с
 #          нами, а с заголовками САМОЙ СИСТЕМЫ. Кто возьмётся за это снова - пусть
 #          начинает с них
 #
 # @note Порядок слов у CMake строгий: BEFORE первым, SYSTEM вторым. С обратным CMake
 #       принимает ключевое слово за имя каталога, и в строке сборки появляется
 #       `-isystem <корень>/BEFORE` - наблюдалось
 #
 ##
include_directories(BEFORE ${DEPEND_INCLUDE_DIRS})
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/include)
include_directories(BEFORE ${CMAKE_SOURCE_DIR}/contrib/include)
include_directories(SYSTEM ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})

# Если сборка производится под операционные системы macOS, FreeBSD, DragonFly, NetBSD или OpenBSD
if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin" OR ${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "DragonFly" OR ${CMAKE_SYSTEM_NAME} STREQUAL "NetBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
    # Выполняем подключение бэкендов IO
    SET(SOURCE_FILES
        "${SOURCE_FILES}"
        "${CMAKE_SOURCE_DIR}/src/net/backend/bsd/eth.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/bsd/addr.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/bsd/iface.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/bsd/kqueue.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/bsd/socket.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/bsd/gateway.cpp"
    )

    # Если операционной системой является FreeBSD
    if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
        # Выполняем подключение бэкенда SCTP
        SET(SOURCE_FILES
            "${SOURCE_FILES}"
            "${CMAKE_SOURCE_DIR}/src/net/backend/bsd/sctp.cpp"
        )
    endif()
##
 # Если сборка производится под операционные системы Solaris или illumos
 #
 # Набор модулей здесь пока заполняется: они переносятся с bsd по одному, с тем же
 # самым API, и подключаются по мере готовности. Движка ввода-вывода тут ещё нет,
 # потому наборы тестов net, proto и unit под эти системы не собираются
 ##
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
    # Выполняем подключение бэкендов IO
    SET(SOURCE_FILES
        "${SOURCE_FILES}"
        "${CMAKE_SOURCE_DIR}/src/net/backend/sun/eth.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/sun/socket.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/sun/addr.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/sun/iface.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/sun/gateway.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/sun/sctp.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/sun/ports.cpp"
    )
# Если сборка производится под операционную систему Linux
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    # Выполняем подключение бэкендов IO
    SET(SOURCE_FILES
        "${SOURCE_FILES}"
        "${CMAKE_SOURCE_DIR}/src/net/backend/gnu/eth.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/gnu/addr.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/gnu/iface.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/gnu/gateway.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/gnu/sctp.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/gnu/socket.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/gnu/netlink.cpp"
    )

    ##
     # Механизм ожидания событий под Linux
     #
     # Движков под Linux два, и собирается ровно один из них. Задано это одним
     # выбором, а не двумя независимыми признаками, намеренно: два признака дают
     # недопустимое состояние «включены оба», которое пришлось бы ловить проверкой,
     # тогда как выбор одного из перечня недопустимого состояния не имеет вовсе.
     #
     # Умолчанием взят epoll: io_uring есть далеко не у всякого ядра - в том числе
     # у Android, - и движок, требующий его, собрался бы не везде
     ##
    SET(AWH_ENGINE "epoll" CACHE STRING "Event Waiting Mechanism under Linux: epoll or io_uring")
    SET_PROPERTY(CACHE AWH_ENGINE PROPERTY STRINGS "epoll" "io_uring")

    # Если выбран механизм ожидания epoll
    if (${AWH_ENGINE} STREQUAL "epoll")
        message(STATUS "Engine: epoll")

        # Выполняем подключение движка ввода-вывода
        SET(SOURCE_FILES
            "${SOURCE_FILES}"
            "${CMAKE_SOURCE_DIR}/src/net/backend/gnu/epoll.cpp"
        )
    ##
     # Если выбран механизм ожидания io_uring
     #
     # Наличие самих операций проверяется пробой при запуске, а не здесь: заголовок
     # дистрибутива отстаёт от ядра (на Ubuntu 24.10 он не знает операций, которые
     # ядро поддерживает), поэтому сборка судит лишь о наличии заголовка, а состав
     # возможностей выясняет движок. Устройство описано в src/net/backend/gnu/IO_URING.md
     ##
    elseif (${AWH_ENGINE} STREQUAL "io_uring")
        ##
         # Выполняем проверку наличия заголовка io_uring
         #
         # Проверка ведётся компилятором C++, а не C: в проекте включён только C++
         # (project(awh LANGUAGES CXX)), и проверка компилятором C отвечала бы отказом
         # всегда - не оттого, что заголовка нет, а оттого, что проверять его нечем
         ##
        INCLUDE(CheckIncludeFileCXX)
        CHECK_INCLUDE_FILE_CXX("linux/io_uring.h" AWH_HAVE_IO_URING_H)

        # Если заголовок io_uring в системе отсутствует
        if (NOT AWH_HAVE_IO_URING_H)
            message(FATAL_ERROR "AWH: заголовок linux/io_uring.h не найден, соберите с -DAWH_ENGINE=epoll")
        endif()

        message(STATUS "Engine: io_uring")

        # Выполняем объявление признака сборки движка io_uring
        ADD_DEFINITIONS(-DAWH_ENGINE_IO_URING)

        # Выполняем подключение движка ввода-вывода
        SET(SOURCE_FILES
            "${SOURCE_FILES}"
            "${CMAKE_SOURCE_DIR}/src/net/backend/gnu/io_uring.cpp"
        )
    # Если выбран механизм ожидания, которого нет
    else()
        message(FATAL_ERROR "AWH: неизвестный механизм ожидания '${AWH_ENGINE}', допустимы epoll и io_uring")
    endif()
##
 # Если сборка производится под операционную систему MS Windows
 #
 # Состояние переноса движка ввода-вывода описано в src/net/backend/win/README.md.
 # Сборка ведётся только в MSYS2 MinGW 64bit, вариант с MSVC не рассматривается
 ##
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    # Выполняем подключение бэкендов IO
    SET(SOURCE_FILES
        "${SOURCE_FILES}"
        "${CMAKE_SOURCE_DIR}/src/net/backend/win/message.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/win/eth.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/win/iocp.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/win/socket.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/win/iface.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/win/tunnel.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/win/gateway.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/win/qos.cpp"
        "${CMAKE_SOURCE_DIR}/src/net/backend/win/addr.cpp"
    )
endif()

##
 # Выполняем подключение захвата выделения памяти процесса
 #
 # Цепочка своя, а не общая с движком ввода-вывода: тот делит системы по механизму
 # ожидания и держит macOS вместе с BSD в одной ветви, а захват делится ИНАЧЕ - по
 # приёму подмены. Уложи мы захват в ту же цепочку, ветвь macOS оказалась бы за уже
 # выбранной ветвью BSD и не срабатывала бы вовсе
 ##
##
 # Если сборка производится под операционную систему MS Windows
 #
 # Формат PE подмены имён при связывании не даёт, и распределитель заслоняет собою
 # выделение памяти, переписывая начала функций библиотеки времени исполнения.
 # Подробности и намеренные решения - в include/alloc/pe.hpp
 ##
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    # Выполняем подключение захвата переписыванием входа функций
    SET(SOURCE_FILES
        "${SOURCE_FILES}"
        "${CMAKE_SOURCE_DIR}/src/alloc/capture/pe.cpp"
    )
##
 # Если сборка производится под операционную систему macOS
 #
 # Имена там связываются двухуровнево, и обращение из библиотеки времени исполнения
 # нашим определением не заслоняется вовсе. Подробности - в include/alloc/zone.hpp
 ##
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    # Выполняем подключение захвата зоной
    SET(SOURCE_FILES
        "${SOURCE_FILES}"
        "${CMAKE_SOURCE_DIR}/src/alloc/capture/mach.cpp"
    )
##
 # Если сборка производится под операционную систему семейства ELF
 #
 # Приём один на Linux, все BSD и Solaris: формат ELF позволяет заслонить имя
 # библиотеки времени исполнения нашим определением, и делить его на семейства значило
 # бы трижды написать одно. Подробности - в include/alloc/elf.hpp
 ##
else()
    # Выполняем подключение захвата подменой имён
    SET(SOURCE_FILES
        "${SOURCE_FILES}"
        "${CMAKE_SOURCE_DIR}/src/alloc/capture/elf.cpp"
    )
    ##
     # Если сборка производится под операционную систему OpenBSD
     #
     # Библиотека времени исполнения зовёт там выдачу памяти напрямую, по внутреннему
     # имени, и подмена именами до неё не достаёт. Определения внутренних имён отдают
     # нам и эти вызовы - при статическом связывании. Подробности в самом файле
     ##
    if (${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
        # Выполняем подключение внутренних имён распределителя libc
        SET(SOURCE_FILES
            "${SOURCE_FILES}"
            "${CMAKE_SOURCE_DIR}/src/alloc/capture/obsd.cpp"
        )
        ##
         # Ключ полного перехвата у OpenBSD
         #
         # Внутренние имена libc отдаются нам лишь при СТАТИЧЕСКОМ связывании: у
         # разделяемой libc вызовы связаны внутри неё самой ещё при её сборке. Режим
         # этот потребитель выбирает сам, и объявить его обязан сборкой - опознать его
         # в работе не выходит, подробности в src/alloc/cache.cpp
         #
         # Собирать надлежит вместе с -static у самой программы
         ##
        option(CMAKE_BUILD_ALLOCATOR_OPENBSD_FULL "Total interposition on OpenBSD (requires static linking)" OFF)
        if (CMAKE_BUILD_ALLOCATOR_OPENBSD_FULL)
            # Включаем полный перехват
            add_definitions(-DAWH_ALLOC_OPENBSD_FULL)
            message(STATUS "OpenBSD total interposition is ENABLED (link your program with -static)")
        endif()
    endif()
endif()

##
 # Ключ временного отключения распределителя
 #
 # У систем ELF распределитель встаёт на место системного САМИМ СВЯЗЫВАНИЕМ: наш файл
 # определяет знак `malloc`, и связывающий берёт его, звал кто `capture` или нет.
 # Вызовом это не отключается - только несовпадением имён, то есть при сборке
 #
 # Ключ этот - подпорка на время доводки модуля. Собранная с ним библиотека оставляет
 # системе её собственный распределитель: cmake -DCMAKE_BUILD_ALLOCATOR=OFF ..
 ##
option(CMAKE_BUILD_ALLOCATOR "Capture process memory allocation by the built-in allocator" ON)
if (NOT CMAKE_BUILD_ALLOCATOR)
    # Запрещаем подмену имён библиотеки времени исполнения
    add_definitions(-DAWH_ALLOC_DISABLED)
    message(STATUS "Built-in allocator capture is DISABLED")
endif()

##
 # Ключи снятия возможностей ради скорости
 #
 # Объявлять снятие надлежит СБОРКОЙ, а не настройкой в работе: проверка признака на
 # горячем пути стоила бы ровно того, что снимается
 #
 # Выгода обоих ключей АРХИТЕКТУРНО ЗАВИСИМА, и у учёта она меняет ЗНАК. Замерено на
 # сценарии «один разряд», наносекунд на действие:
 #
 #   система          заслон списка   учёт расхода
 #   macOS ARM64      нет выгоды      МИНУС 30 %
 #   Debian x86-64    плюс 4.5 %      плюс 12-15 %
 #   FreeBSD amd64    плюс 6 %        плюс 12 %
 #
 # У ARM64 обращение к счётчику потока держит его строку кэша горячей, и быстрый путь
 # ею же и пользуется: снятие учёта там не ускоряет, а замедляет
 #
 # Умолчание у обоих - возможность ВКЛЮЧЕНА: распределитель общего назначения обязан
 # быть безопасным и наблюдаемым, пока у него не попросили иного. Снимать их вправе
 # тот, кто замерил выигрыш на СВОЕЙ машине
 ##
option(CMAKE_BUILD_ALLOCATOR_MANGLE "Mangle free list pointers to detect heap corruption" ON)
# Если снятие заслона списка свободных блоков разрешено
if (NOT CMAKE_BUILD_ALLOCATOR_MANGLE)
    # Снимаем заслон списка свободных блоков
    add_definitions(-DAWH_ALLOC_NO_MANGLE)
    # Выводим сообщение о снятии заслона списка свободных блоков
    message(STATUS "Allocator free list mangling is DISABLED (heap corruption goes undetected)")
endif()

# Активируем учёт расхода памяти по умолчанию: он безопасен и наблюдаем, пока не попросили иного
option(CMAKE_BUILD_ALLOCATOR_ACCOUNTING "Account allocated and peak memory usage" ON)
# Если снятие учёта расхода памяти разрешено
if (NOT CMAKE_BUILD_ALLOCATOR_ACCOUNTING)
    # Снимаем учёт расхода памяти
    add_definitions(-DAWH_ALLOC_NO_ACCOUNTING)
    # Выводим сообщение о снятии учёта расхода памяти
    message(STATUS "Allocator memory accounting is DISABLED (ALLOCATED and PEAK report zero)")
endif()

# Выполняем подключение исходников
SET(SOURCE_FILES
    "${SOURCE_FILES}"
    "${CMAKE_SOURCE_DIR}/src/alloc/alloc.cpp"
    "${CMAKE_SOURCE_DIR}/src/alloc/source.cpp"
    "${CMAKE_SOURCE_DIR}/src/alloc/pages.cpp"
    "${CMAKE_SOURCE_DIR}/src/alloc/classes.cpp"
    "${CMAKE_SOURCE_DIR}/src/alloc/spin.cpp"
    "${CMAKE_SOURCE_DIR}/src/alloc/link.cpp"
    "${CMAKE_SOURCE_DIR}/src/alloc/central.cpp"
    "${CMAKE_SOURCE_DIR}/src/alloc/cache.cpp"
    "${CMAKE_SOURCE_DIR}/src/alloc/huge.cpp"
    "${CMAKE_SOURCE_DIR}/src/alloc/guard.cpp"
    "${CMAKE_SOURCE_DIR}/src/alloc/trace.cpp"
    "${CMAKE_SOURCE_DIR}/src/alloc/profile.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/os.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/fs.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/fmk.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/log.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/procre.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/fiber.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/chrono.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/signals.cpp"
    "${CMAKE_SOURCE_DIR}/src/sys/version.cpp"
    "${CMAKE_SOURCE_DIR}/src/container/queue.cpp"
    "${CMAKE_SOURCE_DIR}/src/container/buffer.cpp"
    "${CMAKE_SOURCE_DIR}/src/container/binbox.cpp"
    "${CMAKE_SOURCE_DIR}/src/num/bignum.cpp"
    "${CMAKE_SOURCE_DIR}/src/num/lexical/table.cpp"
    "${CMAKE_SOURCE_DIR}/src/encoding/unicode/utf8.cpp"
    "${CMAKE_SOURCE_DIR}/src/encoding/charset/table.cpp"
    "${CMAKE_SOURCE_DIR}/src/encoding/charset/charset.cpp"
    "${CMAKE_SOURCE_DIR}/src/encoding/unicode/table.cpp"
    "${CMAKE_SOURCE_DIR}/src/encoding/unicode/unicode.cpp"
    "${CMAKE_SOURCE_DIR}/src/encoding/unicode/normalize.cpp"
    "${CMAKE_SOURCE_DIR}/src/encoding/idna/table.cpp"
    "${CMAKE_SOURCE_DIR}/src/encoding/idna/punycode.cpp"
    "${CMAKE_SOURCE_DIR}/src/encoding/idna/context.cpp"
    "${CMAKE_SOURCE_DIR}/src/encoding/idna/idna.cpp"
    "${CMAKE_SOURCE_DIR}/src/regex/probe.cpp"
    "${CMAKE_SOURCE_DIR}/src/regex/prefilter.cpp"
    "${CMAKE_SOURCE_DIR}/src/regex/text.cpp"
    "${CMAKE_SOURCE_DIR}/src/regex/parser.cpp"
    "${CMAKE_SOURCE_DIR}/src/regex/compiler.cpp"
    "${CMAKE_SOURCE_DIR}/src/regex/pike.cpp"
    "${CMAKE_SOURCE_DIR}/src/regex/dfa.cpp"
    "${CMAKE_SOURCE_DIR}/src/regex/backtrack.cpp"
    "${CMAKE_SOURCE_DIR}/src/regex/assembly.cpp"
    "${CMAKE_SOURCE_DIR}/src/regex/emitter.cpp"
    "${CMAKE_SOURCE_DIR}/src/regex/emitterx64.cpp"
    "${CMAKE_SOURCE_DIR}/src/regex/codegen.cpp"
    "${CMAKE_SOURCE_DIR}/src/regex/engine.cpp"
    "${CMAKE_SOURCE_DIR}/src/regex/regex.cpp"
    "${CMAKE_SOURCE_DIR}/src/regex/storage.cpp"
    "${CMAKE_SOURCE_DIR}/src/regex/grok/table.cpp"
    "${CMAKE_SOURCE_DIR}/src/regex/grok/grok.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/numeric.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/bridge.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/xml/common.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/xml/encoding.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/xml/reader.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/xml/document.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/xml/writer.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/xml/value.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/cef/common.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/cef/reader.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/cef/writer.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/cef/document.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/cef/dictionary.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/ini/common.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/ini/encoding.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/ini/reader.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/ini/writer.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/ini/document.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/ini/value.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/csv/common.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/csv/encoding.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/csv/reader.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/csv/writer.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/csv/document.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/csv/value.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/json/common.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/json/encoding.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/json/reader.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/json/writer.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/json/document.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/json/value.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/yaml/common.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/yaml/encoding.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/yaml/reader.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/yaml/writer.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/yaml/document.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/yaml/value.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/toml/common.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/toml/encoding.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/toml/reader.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/toml/writer.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/toml/document.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/toml/value.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/abc/common.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/abc/header.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/abc/chunk.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/abc/index.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/abc/container.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/abc/signature.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/abc/schedule.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/abc/editor.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/abc/storage.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/abc/encoding.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/abc/reader.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/abc/writer.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/abc/document.cpp"
    "${CMAKE_SOURCE_DIR}/src/codec/abc/value.cpp"
    "${CMAKE_SOURCE_DIR}/src/args/common.cpp"
    "${CMAKE_SOURCE_DIR}/src/args/lexer.cpp"
    "${CMAKE_SOURCE_DIR}/src/args/schema.cpp"
    "${CMAKE_SOURCE_DIR}/src/args/args.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/net.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/fds.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/uri.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/nwt.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/addr.cpp"
    "${CMAKE_SOURCE_DIR}/src/net/queue.cpp"
    "${CMAKE_SOURCE_DIR}/src/cryptography/hash.cpp"
    "${CMAKE_SOURCE_DIR}/src/cryptography/crypto.cpp"
    "${CMAKE_SOURCE_DIR}/src/cryptography/vault.cpp"
    "${CMAKE_SOURCE_DIR}/src/cryptography/tls/coder.cpp"
    "${CMAKE_SOURCE_DIR}/src/cryptography/tls/fingerprint.cpp"
    "${CMAKE_SOURCE_DIR}/src/unit/fs.cpp"
    "${CMAKE_SOURCE_DIR}/src/unit/dns.cpp"
    "${CMAKE_SOURCE_DIR}/src/unit/ntp.cpp"
    "${CMAKE_SOURCE_DIR}/src/unit/icmp.cpp"
    "${CMAKE_SOURCE_DIR}/src/unit/portmap.cpp"
    "${CMAKE_SOURCE_DIR}/src/unit/quic.cpp"
    "${CMAKE_SOURCE_DIR}/src/unit/unit.cpp"
    "${CMAKE_SOURCE_DIR}/src/unit/timer.cpp"
    "${CMAKE_SOURCE_DIR}/src/unit/client.cpp"
    "${CMAKE_SOURCE_DIR}/src/unit/server.cpp"
    "${CMAKE_SOURCE_DIR}/src/unit/tunnel.cpp"
    "${CMAKE_SOURCE_DIR}/src/unit/cluster.cpp"
    "${CMAKE_SOURCE_DIR}/src/unit/mediator.cpp"
    "${CMAKE_SOURCE_DIR}/src/unit/notifier.cpp"
    "${CMAKE_SOURCE_DIR}/src/client/client.cpp"
    "${CMAKE_SOURCE_DIR}/src/client/socks5.cpp"
    "${CMAKE_SOURCE_DIR}/src/server/server.cpp"
    "${CMAKE_SOURCE_DIR}/src/server/socks5.cpp"
    "${CMAKE_SOURCE_DIR}/src/compressor/block.cpp"
    "${CMAKE_SOURCE_DIR}/src/compressor/types.cpp"
    "${CMAKE_SOURCE_DIR}/src/compressor/stream.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/portmap/natpmp.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/portmap/pcp.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/portmap/ssdp.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/portmap/device.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/portmap/soap.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/portmap/upnp.cpp"
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
    "${CMAKE_SOURCE_DIR}/src/proto/http/parser/http3/h3.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/http/parser/http3/frame.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/http/parser/http3/qpack.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/http/parser/http3/http.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/quic/quic.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/quic/varint.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/quic/params.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/quic/crypto.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/quic/packet.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/quic/frame.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/quic/handshake.cpp"
    "${CMAKE_SOURCE_DIR}/src/proto/quic/connection.cpp"
)

##
 # Запрещаем собирателю числить malloc-семейство встроенным
 #
 # Файл этот определяет malloc, free, calloc и realloc. Числя их встроенными,
 # собиратель узнаёт в нашем `calloc` пару «выделение плюс обнуление» и подменяет её
 # вызовом... `calloc`, то есть себя самого - программа встаёт навсегда. Проверено
 # опытом на Solaris: при -O2 щуп там вставал, при -O0 проходил целиком
 ##
if (NOT MSVC)
    set_source_files_properties(
        "${CMAKE_SOURCE_DIR}/src/alloc/alloc.cpp"
        PROPERTIES COMPILE_OPTIONS "-fno-builtin-malloc;-fno-builtin-calloc;-fno-builtin-realloc;-fno-builtin-free"
    )
endif()

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

    ##
     # Сборка проверок под санитайзерами.
     #
     # Санитайзеры включаются одним ключом на весь набор проверок: библиотека проверок
     # собирается из тех же исходников, что и рабочая, и флаги её сборки идут в каждую
     # цель проверок через те же переменные, что и сбор статистики покрытия. Отдельного
     # дерева сборки для этого не нужно, но каталог сборки следует брать свой: объектные
     # файлы с санитайзерами и без несовместимы.
     #
     #   cmake -DCMAKE_BUILD_SANITIZE=YES -DCMAKE_BUILD_COVERAGE=NO ..
     #
     # Набор санитайзеров задаётся ключом CMAKE_BUILD_SANITIZE_LIST: умолчанием берутся
     # проверки обращения к памяти и неопределённого поведения, а проверку состязаний
     # (thread) включают отдельно - с адресной она несовместима.
     ##
    # Если сборка проверок под санитайзерами активирована
    if (CMAKE_BUILD_SANITIZE)
        ##
         # Дополняем флаги компиляции проверок флагами санитайзеров
         #
         # @note Кадровый указатель сохраняется намеренно: без него отчёт санитайзера
         #       остаётся без внятного следа вызовов, и место дефекта приходится искать
         #       заново
         ##
        SET(GCC_COVERAGE_COMPILE_FLAGS ${GCC_COVERAGE_COMPILE_FLAGS} -fsanitize=${CMAKE_BUILD_SANITIZE_LIST} -fno-omit-frame-pointer -g)
        # Дополняем зависимости линковки проверок рантаймом санитайзеров
        SET(GCC_COVERAGE_LIBS ${GCC_COVERAGE_LIBS} -fsanitize=${CMAKE_BUILD_SANITIZE_LIST})

        # Выводим сообщение о сборке проверок под санитайзерами
        message(STATUS "Build unit tests with sanitizers: ${CMAKE_BUILD_SANITIZE_LIST}\n")
    endif (CMAKE_BUILD_SANITIZE)
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
    ##
     # Заводим учёт путей исполнения сопоставления у библиотеки проверок.
     #
     # Ускорители модуля регулярных выражений вердикта не меняют: отключение всякого
     # из них оставляет итог верным, а поиск замедляет, - оттого сличение вердиктов
     # такого отключения не замечает вовсе. Проба гашения показала, что семь путей
     # исполнения гасились, не сорвав ни одной проверки набора, и порождённый машинный
     # код был среди них. Учёт этот позволяет проверкам удостоверять, что путь пройден.
     #
     # Признак ставится библиотеке проверок, а не рабочей: приращение счётчика
     # обходится в десятки наносекунд у сопоставления, укладывающегося в двадцать.
     ##
    target_compile_definitions(${AWH_LIB_GCOV_NAME} PRIVATE AWH_REGEX_PROBING)
endif (CMAKE_BUILD_TESTS)

##
 # Меняем язык компиляции для единственного файла, которому это нужно.
 #
 # Objective-C в проекте требует один фрагмент - разбор alias-файлов macOS в
 # src/sys/fs.cpp. Прежде флаг применялся ко всей библиотеке, и следствий у этого
 # было три: один и тот же код собирался под macOS как Objective-C++, а под iOS и
 # Android как C++ (то есть расхождение в языке между платформами, на которых
 # фреймворк обязан вести себя одинаково); автоматическое управление памятью
 # Objective-C распространялось на семь десятков файлов, которые под него никто не
 # писал; и полтора десятка единиц трансляции получали ссылки на среду
 # Objective-C без всякой надобности.
 ##
if (APPLE AND AWH_TARGET_MACOS AND NOT CMAKE_NO_MACOS_ALIAS)
    set_source_files_properties(
        "${CMAKE_SOURCE_DIR}/src/sys/fs.cpp"
        PROPERTIES COMPILE_OPTIONS "-x;objective-c++;-fobjc-arc"
    )
endif()

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
    # Если сборка производится под систему с поддержкой SCTP: FreeBSD, Linux, Solaris или illumos
    if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
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
    # Если сборка производится под систему с поддержкой SCTP: FreeBSD, Linux, Solaris или illumos
    if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
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

    # Устанавливаем имя приложения работы с юнитом перенаправления портов
    SET(UNIT_APP_PORTMAP_NAME unit-portmap)
    # Указываем что нужно собрать приложение работы с юнитом перенаправления портов
    option(CMAKE_BUILD_UNIT_PORTMAP_APP_SAMPLE "Build Unit Portmap app sample" ON)

    # Устанавливаем имя приложения присмотра за перенаправлением порта
    SET(UNIT_APP_WATCHDOG_NAME unit-watchdog)
    # Указываем что нужно собрать приложение присмотра за перенаправлением порта
    option(CMAKE_BUILD_UNIT_WATCHDOG_APP_SAMPLE "Build Unit Portmap watchdog app sample" ON)

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

    # Устанавливаем имя приложения работы с распределителем памяти
    SET(ALLOC_APP_NAME alloc)
    # Указываем что нужно собрать приложение работы с распределителем памяти
    option(CMAKE_BUILD_ALLOC_APP_SAMPLE "Build Allocator app sample" ON)

    # Устанавливаем имя приложения работы со своим источником страниц
    SET(SOURCE_APP_NAME alloc-source)
    # Указываем что нужно собрать приложение работы со своим источником страниц
    option(CMAKE_BUILD_SOURCE_APP_SAMPLE "Build Allocator page source app sample" ON)

    # Устанавливаем имя приложения настройки распределителя под нагрузку
    SET(TUNING_APP_NAME alloc-tuning)
    # Указываем что нужно собрать приложение настройки распределителя под нагрузку
    option(CMAKE_BUILD_TUNING_APP_SAMPLE "Build Allocator tuning app sample" ON)

    # Устанавливаем имя приложения поиска дефектов памяти прикладного кода
    SET(DEBUG_APP_NAME alloc-debug)
    # Указываем что нужно собрать приложение поиска дефектов памяти прикладного кода
    option(CMAKE_BUILD_DEBUG_APP_SAMPLE "Build Allocator debugging app sample" ON)

    # Устанавливаем имя приложения работы с укрытой памятью
    SET(KEEPER_APP_NAME keeper)
    # Указываем что нужно собрать приложение работы с укрытой памятью
    option(CMAKE_BUILD_KEEPER_APP_SAMPLE "Build Keeper app sample" ON)

    # Устанавливаем имя приложения работы с датой и временем
    SET(DATE_APP_SYS_NAME sys-date)
    # Указываем что нужно собрать приложение работы с датой и временем
    option(CMAKE_BUILD_DATE_SYS_APP_SAMPLE "Build Date app sample" ON)

    # Устанавливаем имя приложения работы с бинарным контейнером
    # Устанавливаем имя приложения работы с регулярными выражениями
    SET(EXPRESSION_APP_REGEX_NAME regex-expression)
    # Устанавливаем имя приложения разбора текста по шаблонам Grok
    SET(GROK_APP_REGEX_NAME regex-grok)
    # Указываем что нужно собрать приложение работы с регулярными выражениями
    option(CMAKE_BUILD_EXPRESSION_REGEX_APP_SAMPLE "Build regular expression app sample" ON)
    # Указываем что нужно собрать приложение разбора текста по шаблонам Grok
    option(CMAKE_BUILD_GROK_REGEX_APP_SAMPLE "Build Grok app sample" ON)

    SET(BINBOX_APP_CONTAINER_NAME container-binbox)
    # Указываем что нужно собрать приложение работы с бинарным контейнером
    option(CMAKE_BUILD_BINBOX_CONTAINER_APP_SAMPLE "Build BinBox app sample" ON)

    # Устанавливаем имя приложения работы с файловой системой
    SET(FILESYSTEM_APP_SYS_NAME sys-filesystem)
    # Указываем что нужно собрать приложение работы с файловой системой
    option(CMAKE_BUILD_FILESYSTEM_SYS_APP_SAMPLE "Build Filesystem app sample" ON)

    # Устанавливаем имя приложения работы с резольвером процессов
    SET(PROCRE_APP_SYS_NAME sys-procre)
    # Указываем что нужно собрать приложение работы с резольвером процессов
    option(CMAKE_BUILD_PROCRE_SYS_APP_SAMPLE "Build Process Resolver app sample" ON)

    # Устанавливаем имя приложения работы с волокнами
    SET(FIBER_APP_SYS_NAME sys-fiber)
    # Указываем что нужно собрать приложение работы с волокнами
    option(CMAKE_BUILD_FIBER_SYS_APP_SAMPLE "Build Fiber app sample" ON)

    # Устанавливаем имя приложения работы с шифрованием и хэшированием
    SET(CRYPTO_APP_CRYPTOGRAPHY_NAME cryptography-crypto)
    # Указываем что нужно собрать приложение работы с шифрованием и хэшированием
    option(CMAKE_BUILD_CRYPTO_CRYPTOGRAPHY_APP_SAMPLE "Build Crypto app sample" ON)

    # Устанавливаем имя приложения работы с хэшированием данных
    SET(HASH_APP_CRYPTOGRAPHY_NAME cryptography-hash)
    # Указываем что нужно собрать приложение работы с хэшированием данных
    option(CMAKE_BUILD_HASH_CRYPTOGRAPHY_APP_SAMPLE "Build Hash app sample" ON)

    # Устанавливаем имя приложения работы с электронной подписью
    SET(SIGNATURE_APP_CRYPTOGRAPHY_NAME cryptography-signature)
    # Указываем что нужно собрать приложение работы с электронной подписью
    option(CMAKE_BUILD_SIGNATURE_CRYPTOGRAPHY_APP_SAMPLE "Build Signature app sample" ON)

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
    SET(QUIC_CLIENT_APP_PROTO_NAME client-quic)
    # Устанавливаем имя серверного приложения работы с протоколом QUIC
    SET(QUIC_SERVER_APP_PROTO_NAME server-quic)
    # Устанавливаем имя клиентского приложения проверки QUIC-прокси на объединении данных (splice)
    SET(QUIC_PROXY_CLIENT_APP_PROTO_NAME client-quic-proxy)
    # Устанавливаем имя серверного приложения QUIC-прокси на объединении данных (splice)
    SET(QUIC_PROXY_SERVER_APP_PROTO_NAME server-quic-proxy)
    # Указываем что нужно собрать приложения работы с протоколом QUIC
    option(CMAKE_BUILD_QUIC_PROTO_APP_SAMPLE "Build QUIC Protocol app sample" ON)

    # Устанавливаем имя приложения потокового чтения текста разметки
    SET(CODEC_APP_XML_READER_NAME codec-xml-reader)
    # Устанавливаем имя приложения работы с деревом разметки
    SET(CODEC_APP_XML_DOCUMENT_NAME codec-xml-document)
    # Устанавливаем имя приложения записи текста разметки
    SET(CODEC_APP_XML_WRITER_NAME codec-xml-writer)
    # Устанавливаем имя приложения получения содержимого разметки числами
    SET(CODEC_APP_XML_NUMERIC_NAME codec-xml-numeric)
    # Указываем что нужно собрать приложение работы с контейнером XML
    option(CMAKE_BUILD_CODEC_XML_APP_SAMPLE "Build XML codec app sample" ON)

    # Устанавливаем имя приложения потокового чтения текста настроек
    SET(CODEC_APP_INI_READER_NAME codec-ini-reader)
    # Устанавливаем имя приложения работы с деревом настроек
    SET(CODEC_APP_INI_DOCUMENT_NAME codec-ini-document)
    # Устанавливаем имя приложения записи текста настроек
    SET(CODEC_APP_INI_WRITER_NAME codec-ini-writer)
    # Устанавливаем имя приложения перевода настроек между видами записи
    SET(CODEC_APP_BRIDGE_NAME codec-bridge)
    # Указываем что нужно собрать приложение перевода настроек между видами записи
    option(CMAKE_BUILD_CODEC_BRIDGE_APP_SAMPLE "Build codec bridge app sample" ON)
    # Устанавливаем имя приложения работы с параметрами запуска
    SET(ARGS_APP_NAME args)
    # Указываем что нужно собрать приложение работы с параметрами запуска
    option(CMAKE_BUILD_ARGS_APP_SAMPLE "Build args app sample" ON)
    # Указываем что нужно собрать приложение работы с контейнером INI
    option(CMAKE_BUILD_CODEC_INI_APP_SAMPLE "Build INI codec app sample" ON)

    # Устанавливаем имя приложения потокового чтения текста настроек TOML
    SET(CODEC_APP_TOML_READER_NAME codec-toml-reader)
    # Устанавливаем имя приложения работы с деревом настроек TOML
    SET(CODEC_APP_TOML_DOCUMENT_NAME codec-toml-document)
    # Устанавливаем имя приложения записи текста настроек TOML
    SET(CODEC_APP_TOML_WRITER_NAME codec-toml-writer)
    # Указываем что нужно собрать приложение работы с контейнером TOML
    option(CMAKE_BUILD_CODEC_TOML_APP_SAMPLE "Build TOML codec app sample" ON)

    # Устанавливаем имя приложения потокового чтения текста таблицы CSV
    SET(CODEC_APP_CSV_READER_NAME codec-csv-reader)
    # Устанавливаем имя приложения работы с деревом таблицы CSV
    SET(CODEC_APP_CSV_DOCUMENT_NAME codec-csv-document)
    # Устанавливаем имя приложения записи текста таблицы CSV
    SET(CODEC_APP_CSV_WRITER_NAME codec-csv-writer)
    # Указываем что нужно собрать приложение работы с контейнером CSV
    option(CMAKE_BUILD_CODEC_CSV_APP_SAMPLE "Build CSV codec app sample" ON)

    # Устанавливаем имя приложения потокового чтения текста документа YAML
    SET(CODEC_APP_YAML_READER_NAME codec-yaml-reader)
    # Устанавливаем имя приложения работы с деревом документа YAML
    SET(CODEC_APP_YAML_DOCUMENT_NAME codec-yaml-document)
    # Устанавливаем имя приложения записи текста документа YAML
    SET(CODEC_APP_YAML_WRITER_NAME codec-yaml-writer)
    # Указываем что нужно собрать приложение работы с контейнером YAML
    option(CMAKE_BUILD_CODEC_YAML_APP_SAMPLE "Build YAML codec app sample" ON)

    # Устанавливаем имя приложения потокового чтения бинарной записи ABC
    SET(CODEC_APP_ABC_READER_NAME codec-abc-reader)
    # Устанавливаем имя приложения работы с деревом документа ABC
    SET(CODEC_APP_ABC_DOCUMENT_NAME codec-abc-document)
    # Устанавливаем имя приложения сборки бинарной записи ABC
    SET(CODEC_APP_ABC_WRITER_NAME codec-abc-writer)
    # Устанавливаем имя приложения работы с контейнером ABC
    SET(CODEC_APP_ABC_CONTAINER_NAME codec-abc-container)
    # Указываем что нужно собрать приложение работы с контейнером ABC
    option(CMAKE_BUILD_CODEC_ABC_APP_SAMPLE "Build ABC codec app sample" ON)

    # Устанавливаем имя приложения потокового чтения текста документа JSON
    SET(CODEC_APP_JSON_READER_NAME codec-json-reader)
    # Устанавливаем имя приложения работы с деревом документа JSON
    SET(CODEC_APP_JSON_DOCUMENT_NAME codec-json-document)
    # Устанавливаем имя приложения записи текста документа JSON
    SET(CODEC_APP_JSON_WRITER_NAME codec-json-writer)
    # Указываем что нужно собрать приложение работы с контейнером JSON
    option(CMAKE_BUILD_CODEC_JSON_APP_SAMPLE "Build JSON codec app sample" ON)

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

    # Если сборка производится под систему с поддержкой SCTP: FreeBSD, Linux, Solaris или illumos
    if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
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
        # Устанавливаем имя приложения переноса файла по SCTP - клиент
        SET(CLIENT_APP_NET_SCTP_TRANSFER_NAME net-sctp-transfer-client)
        # Устанавливаем имя приложения переноса файла по SCTP - сервер
        SET(SERVER_APP_NET_SCTP_TRANSFER_NAME net-sctp-transfer-server)
        # Устанавливаем имя приложения проверки многодомности SCTP
        SET(APP_NET_SCTP_MULTIHOME_NAME net-sctp-multihome)
        # Указываем что нужно собрать приложение работы с SCTP (клиент и сервер)
        option(CMAKE_BUILD_NET_SCTP_APP_SAMPLE "Build NET SCTP app sample" ON)
    endif()
endif (${CMAKE_BUILD_SAMPLES})

# Обнуляем флаги для сборки семплов
SET(AWH_FLAGS "")

# Если сборка производится под операционную систему Windows
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    # export MSYS="error_start:%WD%dumper.exe -d -c %1 %2"
    ##
     # Устанавливаем необходимые зависимости для MS Windows
     #
     # Назначение каждой:
     #   ws2_32   - сокеты Winsock2 и порты завершения ввода-вывода
     #   Iphlpapi - сведения о сетевых устройствах и перевод названия устройства в номер
     #   psapi    - сведения о памяти процесса, нужны модулям os и procre
     #   dbghelp  - разбор стека вызовов в отладочной сборке взамен execinfo
     #   crypt32  - хранилище сертификатов системы
     #   bcrypt   - источник случайных чисел системы
     #   gdi32    - требуется зависимостями, самой библиотекой графика не используется
     #
     # Стандартная библиотека C++ здесь НЕ упоминается намеренно: собиратель C++ подключает
     # её сам, а упомянутая отдельно она подтягивается ОТДЕЛЬНЫМ файлом. Цели, связываемые со
     # стандартной библиотекой статически (наборы замеров - ради работоспособности учёта
     # выделений памяти), получали тогда обе разом, и связывание валилось сотнями "multiple
     # definition of std::__throw_logic_error". Упоминание распространялось на всякую цель,
     # связанную с библиотекой, и отбор его в наборе замеров не спасал. Обнаружено на стенде
     # MSYS2 MinGW64 (14.08.2026)
     ##
    SET(AWH_FLAGS -lws2_32 -lIphlpapi -lpsapi -ldbghelp -lcrypt32 -lbcrypt -lgdi32 -flto=auto)

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
    ##
     # Устанавливаем необходимые зависимости для настольной macOS.
     #
     # Foundation нужен единственному потребителю - разбору alias-файлов, - и
     # линкуется только вместе с ним. Прежде здесь перечислялись ещё Security и
     # CoreFoundation: на них не ссылается ни одна единица трансляции проекта и ни
     # одна из сторонних библиотек, а CoreFoundation вдобавок загружается сам как
     # зависимость Foundation.
     ##
    if (APPLE AND AWH_TARGET_MACOS AND NOT CMAKE_NO_MACOS_ALIAS)
        SET(AWH_FLAGS "-framework Foundation")
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
        # Если сборка производится под систему с поддержкой SCTP: FreeBSD, Linux, Solaris или illumos
        if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
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
        # Если сборка производится под систему с поддержкой SCTP: FreeBSD, Linux, Solaris или illumos
        if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
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
        add_sample(${UNIT_APP_FS_NAME} sample/unit/fs.cpp unit)
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
        add_sample(${UNIT_APP_DNS_NAME} sample/unit/dns.cpp unit)
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
        add_sample(${UNIT_APP_NTP_NAME} sample/unit/ntp.cpp unit)
    # Если не нужно собирать семплы для работы с юнитом синхронизации времени NTP
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UNIT NTP: No\n")
    endif (${CMAKE_BUILD_UNIT_NTP_APP_SAMPLE})

    # Если нужно собрать семплы для работы с юнитом перенаправления портов
    if (${CMAKE_BUILD_UNIT_PORTMAP_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UNIT PORTMAP: Yes\n")
        # Выполняем создание приложения работы с юнитом перенаправления портов
        add_sample(${UNIT_APP_PORTMAP_NAME} sample/unit/portmap.cpp unit)
    # Если не нужно собирать семплы для работы с юнитом перенаправления портов
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UNIT PORTMAP: No\n")
    endif (${CMAKE_BUILD_UNIT_PORTMAP_APP_SAMPLE})

    # Если нужно собрать семплы присмотра за перенаправлением порта
    if (${CMAKE_BUILD_UNIT_WATCHDOG_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UNIT WATCHDOG: Yes\n")
        # Выполняем создание приложения присмотра за перенаправлением порта
        add_sample(${UNIT_APP_WATCHDOG_NAME} sample/unit/watchdog.cpp unit)
    # Если не нужно собирать семплы присмотра за перенаправлением порта
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UNIT WATCHDOG: No\n")
    endif (${CMAKE_BUILD_UNIT_WATCHDOG_APP_SAMPLE})

    # Если нужно собрать семплы для работы с юнитом ICMP-клиентом
    if (${CMAKE_BUILD_UNIT_ICMP_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UNIT ICMP: Yes\n")
        # Выполняем создание приложения работы с юнитом ICMP-клиентом
        add_sample(${UNIT_APP_ICMP_NAME} sample/unit/ping.cpp unit)
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
        add_sample(${UNIT_APP_TIMER_NAME} sample/unit/timer.cpp unit)
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
        add_sample(${UNIT_APP_CLUSTER_NAME} sample/unit/cluster.cpp unit)
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
        add_sample(${UNIT_APP_NOTIFIER_NAME} sample/unit/notifier.cpp unit)
    # Если не нужно собирать семплы для работы с юнитом уведомителя
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample UNIT Notifier: No\n")
    endif (${CMAKE_BUILD_UNIT_NOTIFIER_APP_SAMPLE})

    # Если нужно собрать семплы для работы с распределителем памяти
    if (${CMAKE_BUILD_ALLOC_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample ALLOC: Yes\n")
        # Выполняем создание приложения работы с распределителем памяти
        add_sample(${ALLOC_APP_NAME} sample/alloc/alloc.cpp alloc)
    # Если не нужно собирать семплы для работы с распределителем памяти
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample ALLOC: No\n")
    endif (${CMAKE_BUILD_ALLOC_APP_SAMPLE})

    # Если нужно собрать семплы для работы со своим источником страниц
    if (${CMAKE_BUILD_SOURCE_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample ALLOC SOURCE: Yes\n")
        # Выполняем создание приложения работы со своим источником страниц
        add_sample(${SOURCE_APP_NAME} sample/alloc/source.cpp alloc)
    # Если не нужно собирать семплы для работы со своим источником страниц
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample ALLOC SOURCE: No\n")
    endif (${CMAKE_BUILD_SOURCE_APP_SAMPLE})

    # Если нужно собрать семплы настройки распределителя под нагрузку
    if (${CMAKE_BUILD_TUNING_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample ALLOC TUNING: Yes\n")
        # Выполняем создание приложения настройки распределителя под нагрузку
        add_sample(${TUNING_APP_NAME} sample/alloc/tuning.cpp alloc)
    # Если не нужно собирать семплы настройки распределителя под нагрузку
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample ALLOC TUNING: No\n")
    endif (${CMAKE_BUILD_TUNING_APP_SAMPLE})

    # Если нужно собрать семплы поиска дефектов памяти прикладного кода
    if (${CMAKE_BUILD_DEBUG_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample ALLOC DEBUG: Yes\n")
        # Выполняем создание приложения поиска дефектов памяти прикладного кода
        add_sample(${DEBUG_APP_NAME} sample/alloc/debug.cpp alloc)
    # Если не нужно собирать семплы поиска дефектов памяти прикладного кода
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample ALLOC DEBUG: No\n")
    endif (${CMAKE_BUILD_DEBUG_APP_SAMPLE})

    # Если нужно собрать семплы для работы с укрытой памятью
    if (${CMAKE_BUILD_KEEPER_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample KEEPER: Yes\n")
        # Выполняем создание приложения работы с укрытой памятью
        add_sample(${KEEPER_APP_NAME} sample/alloc/keeper.cpp alloc)
    # Если не нужно собирать семплы для работы с укрытой памятью
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample KEEPER: No\n")
    endif (${CMAKE_BUILD_KEEPER_APP_SAMPLE})

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

    # Если нужно собрать семплы для работы с регулярными выражениями
    if (${CMAKE_BUILD_EXPRESSION_REGEX_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample Regular expression: Yes\n")
        # Выполняем создание приложения работы с регулярными выражениями
        add_sample(${EXPRESSION_APP_REGEX_NAME} sample/regex/expression.cpp regex)
    # Если не нужно собирать семплы для работы с регулярными выражениями
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample Regular expression: No\n")
    endif (${CMAKE_BUILD_EXPRESSION_REGEX_APP_SAMPLE})

    # Если нужно собрать семплы разбора текста по шаблонам Grok
    if (${CMAKE_BUILD_GROK_REGEX_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample Grok: Yes\n")
        # Выполняем создание приложения разбора текста по шаблонам Grok
        add_sample(${GROK_APP_REGEX_NAME} sample/regex/grok.cpp regex)
    # Если не нужно собирать семплы разбора текста по шаблонам Grok
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample Grok: No\n")
    endif (${CMAKE_BUILD_GROK_REGEX_APP_SAMPLE})

    # Если нужно собрать семплы для работы с бинарным контейнером
    if (${CMAKE_BUILD_BINBOX_CONTAINER_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample BinBox: Yes\n")
        # Выполняем создание приложения работы с бинарным контейнером
        add_sample(${BINBOX_APP_CONTAINER_NAME} sample/container/binbox.cpp container)
    # Если не нужно собирать семплы для работы с бинарным контейнером
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample BinBox: No\n")
    endif (${CMAKE_BUILD_BINBOX_CONTAINER_APP_SAMPLE})

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

    # Если нужно собрать семплы для работы с волокнами
    if (${CMAKE_BUILD_FIBER_SYS_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample Fiber: Yes\n")
        # Выполняем создание приложения работы с волокнами
        add_sample(${FIBER_APP_SYS_NAME} sample/sys/fiber.cpp sys)
    # Если не нужно собирать семплы для работы с волокнами
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample Fiber: No\n")
    endif (${CMAKE_BUILD_FIBER_SYS_APP_SAMPLE})
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
    if (${CMAKE_BUILD_CRYPTO_CRYPTOGRAPHY_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample Crypto: Yes\n")
        # Выполняем создание приложения работы с шифрованием и хэшированием
        add_sample(${CRYPTO_APP_CRYPTOGRAPHY_NAME} sample/cryptography/crypto.cpp cryptography)
    # Если не нужно собирать семплы для работы с шифрованием и хэшированием
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample Crypto: No\n")
    endif (${CMAKE_BUILD_CRYPTO_CRYPTOGRAPHY_APP_SAMPLE})

    # Если нужно собрать семплы для работы с хэшированием данных
    if (${CMAKE_BUILD_HASH_CRYPTOGRAPHY_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample Hash: Yes\n")
        # Выполняем создание приложения работы с хэшированием данных
        add_sample(${HASH_APP_CRYPTOGRAPHY_NAME} sample/cryptography/hash.cpp cryptography)
    # Если не нужно собирать семплы для работы с хэшированием данных
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample Hash: No\n")
    endif (${CMAKE_BUILD_HASH_CRYPTOGRAPHY_APP_SAMPLE})

    # Если нужно собрать семплы для работы с электронной подписью
    if (${CMAKE_BUILD_SIGNATURE_CRYPTOGRAPHY_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample Signature: Yes\n")
        # Выполняем создание приложения работы с электронной подписью
        add_sample(${SIGNATURE_APP_CRYPTOGRAPHY_NAME} sample/cryptography/signature.cpp cryptography)
    # Если не нужно собирать семплы для работы с электронной подписью
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample Signature: No\n")
    endif (${CMAKE_BUILD_SIGNATURE_CRYPTOGRAPHY_APP_SAMPLE})

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
        add_sample(${QUIC_CLIENT_APP_PROTO_NAME} sample/client/quic.cpp client)
        # Выполняем создание приложения работы с QUIC сервером
        add_sample(${QUIC_SERVER_APP_PROTO_NAME} sample/server/quic.cpp server)
        # Выполняем создание клиентского приложения проверки QUIC-прокси на объединении данных (splice)
        add_sample(${QUIC_PROXY_CLIENT_APP_PROTO_NAME} sample/client/quic-proxy.cpp client)
        # Выполняем создание серверного приложения QUIC-прокси на объединении данных (splice)
        add_sample(${QUIC_PROXY_SERVER_APP_PROTO_NAME} sample/server/quic-proxy.cpp server)
    # Если не нужно собирать семплы для работы с протоколом QUIC
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample QUIC Protocol: No\n")
    endif (${CMAKE_BUILD_QUIC_PROTO_APP_SAMPLE})

    # Если нужно собрать семплы для работы с компрессией данных
    if (${CMAKE_BUILD_CODEC_XML_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample XML Codec: Yes\n")
        # Выполняем создание приложения потокового чтения текста разметки
        add_sample(${CODEC_APP_XML_READER_NAME} sample/codec/xml-reader.cpp codec)
        # Выполняем создание приложения работы с деревом разметки
        add_sample(${CODEC_APP_XML_DOCUMENT_NAME} sample/codec/xml-document.cpp codec)
        # Выполняем создание приложения записи текста разметки
        add_sample(${CODEC_APP_XML_WRITER_NAME} sample/codec/xml-writer.cpp codec)
        # Выполняем создание приложения получения содержимого разметки числами
        add_sample(${CODEC_APP_XML_NUMERIC_NAME} sample/codec/xml-numeric.cpp codec)
    # Если не нужно собирать семплы для работы с контейнером XML
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample XML Codec: No\n")
    endif (${CMAKE_BUILD_CODEC_XML_APP_SAMPLE})

    # Если нужно собрать семплы для работы с контейнером INI
    if (${CMAKE_BUILD_CODEC_INI_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample INI Codec: Yes\n")
        # Выполняем создание приложения потокового чтения текста настроек
        add_sample(${CODEC_APP_INI_READER_NAME} sample/codec/ini-reader.cpp codec)
        # Выполняем создание приложения работы с деревом настроек
        add_sample(${CODEC_APP_INI_DOCUMENT_NAME} sample/codec/ini-document.cpp codec)
        # Выполняем создание приложения записи текста настроек
        add_sample(${CODEC_APP_INI_WRITER_NAME} sample/codec/ini-writer.cpp codec)
    # Если не нужно собирать семплы для работы с контейнером INI
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample INI Codec: No\n")
    endif (${CMAKE_BUILD_CODEC_INI_APP_SAMPLE})

    # Если нужно собрать семпл перевода настроек между видами записи
    if (${CMAKE_BUILD_CODEC_BRIDGE_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample Codec Bridge: Yes\n")
        # Выполняем создание приложения перевода настроек между видами записи
        add_sample(${CODEC_APP_BRIDGE_NAME} sample/codec/bridge.cpp codec)
    # Если не нужно собирать семпл перевода настроек между видами записи
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample Codec Bridge: No\n")
    endif()

    # Если нужно собрать семпл работы с параметрами запуска приложения
    if (${CMAKE_BUILD_ARGS_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample Args: Yes\n")
        # Выполняем создание приложения работы с параметрами запуска
        add_sample(${ARGS_APP_NAME} sample/args/args.cpp args)
    # Если не нужно собирать семпл работы с параметрами запуска приложения
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample Args: No\n")
    endif()

    # Если нужно собрать семплы для работы с контейнером ABC
    if (${CMAKE_BUILD_CODEC_ABC_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample ABC Codec: Yes\n")
        # Выполняем создание приложения потокового чтения бинарной записи
        add_sample(${CODEC_APP_ABC_READER_NAME} sample/codec/abc-reader.cpp codec)
        # Выполняем создание приложения работы с деревом документа
        add_sample(${CODEC_APP_ABC_DOCUMENT_NAME} sample/codec/abc-document.cpp codec)
        # Выполняем создание приложения сборки бинарной записи
        add_sample(${CODEC_APP_ABC_WRITER_NAME} sample/codec/abc-writer.cpp codec)
        # Выполняем создание приложения работы с контейнером
        add_sample(${CODEC_APP_ABC_CONTAINER_NAME} sample/codec/abc-container.cpp codec)
    # Если не нужно собирать семплы для работы с контейнером ABC
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample ABC Codec: No\n")
    endif (${CMAKE_BUILD_CODEC_ABC_APP_SAMPLE})

    # Если нужно собрать семплы для работы с контейнером JSON
    if (${CMAKE_BUILD_CODEC_JSON_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample JSON Codec: Yes\n")
        # Выполняем создание приложения потокового чтения текста документа
        add_sample(${CODEC_APP_JSON_READER_NAME} sample/codec/json-reader.cpp codec)
        # Выполняем создание приложения работы с деревом документа
        add_sample(${CODEC_APP_JSON_DOCUMENT_NAME} sample/codec/json-document.cpp codec)
        # Выполняем создание приложения записи текста документа
        add_sample(${CODEC_APP_JSON_WRITER_NAME} sample/codec/json-writer.cpp codec)
    # Если не нужно собирать семплы для работы с контейнером JSON
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample JSON Codec: No\n")
    endif (${CMAKE_BUILD_CODEC_JSON_APP_SAMPLE})

    # Если нужно собрать семплы для работы с контейнером TOML
    if (${CMAKE_BUILD_CODEC_TOML_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample TOML Codec: Yes\n")
        # Выполняем создание приложения потокового чтения текста настроек
        add_sample(${CODEC_APP_TOML_READER_NAME} sample/codec/toml-reader.cpp codec)
        # Выполняем создание приложения работы с деревом настроек
        add_sample(${CODEC_APP_TOML_DOCUMENT_NAME} sample/codec/toml-document.cpp codec)
        # Выполняем создание приложения записи текста настроек
        add_sample(${CODEC_APP_TOML_WRITER_NAME} sample/codec/toml-writer.cpp codec)
    # Если не нужно собирать семплы для работы с контейнером TOML
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample TOML Codec: No\n")
    endif (${CMAKE_BUILD_CODEC_TOML_APP_SAMPLE})

    # Если нужно собрать семплы для работы с контейнером YAML
    if (${CMAKE_BUILD_CODEC_YAML_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample YAML Codec: Yes\n")
        # Выполняем создание приложения потокового чтения текста документа
        add_sample(${CODEC_APP_YAML_READER_NAME} sample/codec/yaml-reader.cpp codec)
        # Выполняем создание приложения работы с деревом документа
        add_sample(${CODEC_APP_YAML_DOCUMENT_NAME} sample/codec/yaml-document.cpp codec)
        # Выполняем создание приложения записи текста документа
        add_sample(${CODEC_APP_YAML_WRITER_NAME} sample/codec/yaml-writer.cpp codec)
    # Если не нужно собирать семплы для работы с контейнером YAML
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample YAML Codec: No\n")
    endif (${CMAKE_BUILD_CODEC_YAML_APP_SAMPLE})

    # Если нужно собрать семплы для работы с контейнером CSV
    if (${CMAKE_BUILD_CODEC_CSV_APP_SAMPLE})
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample CSV Codec: Yes\n")
        # Выполняем создание приложения потокового чтения текста таблицы
        add_sample(${CODEC_APP_CSV_READER_NAME} sample/codec/csv-reader.cpp codec)
        # Выполняем создание приложения работы с деревом таблицы
        add_sample(${CODEC_APP_CSV_DOCUMENT_NAME} sample/codec/csv-document.cpp codec)
        # Выполняем создание приложения записи текста таблицы
        add_sample(${CODEC_APP_CSV_WRITER_NAME} sample/codec/csv-writer.cpp codec)
    # Если не нужно собирать семплы для работы с контейнером CSV
    else()
        # Выводим сообщение о сборке семпла
        message(STATUS "Build sample CSV Codec: No\n")
    endif (${CMAKE_BUILD_CODEC_CSV_APP_SAMPLE})

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

    # Если сборка производится под систему с поддержкой SCTP: FreeBSD, Linux, Solaris или illumos
    if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
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
            # Выполняем создание приложения переноса файла по SCTP - клиент
            add_sample(${CLIENT_APP_NET_SCTP_TRANSFER_NAME} sample/net/sctp/transfer-client.cpp net/sctp)
            # Выполняем создание приложения переноса файла по SCTP - сервер
            add_sample(${SERVER_APP_NET_SCTP_TRANSFER_NAME} sample/net/sctp/transfer-server.cpp net/sctp)
            # Выполняем создание приложения проверки многодомности SCTP
            add_sample(${APP_NET_SCTP_MULTIHOME_NAME} sample/net/sctp/multihome.cpp net/sctp)
        # Если не нужно собирать семплы для работы с SCTP (клиент и сервер)
        else()
            # Выводим сообщение о сборке семпла
            message(STATUS "Build sample SCTP Client/Server: No\n")
        endif (${CMAKE_BUILD_NET_SCTP_APP_SAMPLE})
    endif()
endif (${CMAKE_BUILD_SAMPLES})

# Если нужно собрать и динамическую библиотеку
if (CMAKE_SHARED_BUILD_LIB)
    # Выполняем сборку динамической библиотеки
    target_link_libraries(
        ${AWH_LIB_NAME}
        PRIVATE
        ${DEPEND_LIBRARY}
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
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом UDS версии 2
            target_link_libraries(
                ${CLIENT_APP_UDS-2_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом TCP
            target_link_libraries(
                ${CLIENT_APP_TCP_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом TCP версии 2
            target_link_libraries(
                ${CLIENT_APP_TCP-2_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом UDP
            target_link_libraries(
                ${CLIENT_APP_UDP_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом UDP версии 2
            target_link_libraries(
                ${CLIENT_APP_UDP-2_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом TLS
            target_link_libraries(
                ${CLIENT_APP_TLS_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом TLS версии 2
            target_link_libraries(
                ${CLIENT_APP_TLS-2_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом DTLS
            target_link_libraries(
                ${CLIENT_APP_DTLS_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом DTLS версии 2
            target_link_libraries(
                ${CLIENT_APP_DTLS-2_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом TLS-Web
            target_link_libraries(
                ${CLIENT_APP_TLS-WEB_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Если сборка производится под систему с поддержкой SCTP: FreeBSD, Linux, Solaris или illumos
            if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
                # Выполняем сборку приложения работы с клиентом SCTP
                target_link_libraries(
                    ${CLIENT_APP_SCTP_NAME}
                    ${AWH_LIB_NAME}
                    ${AWH_FLAGS}
                )

                # Выполняем сборку приложения работы с клиентом SCTP версии 2
                target_link_libraries(
                    ${CLIENT_APP_SCTP-2_NAME}
                    ${AWH_LIB_NAME}
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
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером UDS версии 2
            target_link_libraries(
                ${SERVER_APP_UDS-2_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером TCP
            target_link_libraries(
                ${SERVER_APP_TCP_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером TCP версии 2
            target_link_libraries(
                ${SERVER_APP_TCP-2_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером UDP
            target_link_libraries(
                ${SERVER_APP_UDP_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером UDP версии 2
            target_link_libraries(
                ${SERVER_APP_UDP-2_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером TLS
            target_link_libraries(
                ${SERVER_APP_TLS_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером TLS мультисертификатным
            target_link_libraries(
                ${SERVER_APP_TLS-MULTICERT_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером DTLS
            target_link_libraries(
                ${SERVER_APP_DTLS_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером DTLS мультисертификатным
            target_link_libraries(
                ${SERVER_APP_DTLS-MULTICERT_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Если сборка производится под систему с поддержкой SCTP: FreeBSD, Linux, Solaris или illumos
            if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
                # Выполняем сборку приложения работы с сервером SCTP
                target_link_libraries(
                    ${SERVER_APP_SCTP_NAME}
                    ${AWH_LIB_NAME}
                    ${AWH_FLAGS}
                )

                # Выполняем сборку приложения работы с сервером SCTP версии 2
                target_link_libraries(
                    ${SERVER_APP_SCTP-2_NAME}
                    ${AWH_LIB_NAME}
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
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с юнитом VPN сервером
            target_link_libraries(
                ${SERVER_APP_VPN_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_VPN_APP_SAMPLE})

        # Если нужно собрать семплы для работы с юнитом SOCKS5
        if (${CMAKE_BUILD_SOCKS5_APP_SAMPLE})
            # Выполняем сборку приложения работы с юнитом SOCKS5 клиентом
            target_link_libraries(
                ${CLIENT_APP_SOCKS5_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с юнитом SOCKS5 клиентом для UDP
            target_link_libraries(
                ${CLIENT_APP_SOCKS5-UDP_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с юнитом SOCKS5 клиентом для DTLS
            target_link_libraries(
                ${CLIENT_APP_SOCKS5-DTLS_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с юнитом SOCKS5 сервером
            target_link_libraries(
               ${SERVER_APP_SOCKS5_NAME}
               ${AWH_LIB_NAME}
               ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_SOCKS5_APP_SAMPLE})
    
        # Если нужно собрать семплы для работы с юнитом файловой системы
        if (${CMAKE_BUILD_UNIT_FS_APP_SAMPLE})
            # Выполняем сборку приложения работы с юнитом файловой системы
            target_link_libraries(
                ${UNIT_APP_FS_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_UNIT_FS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с юнитом резолвером доменных имён
        if (${CMAKE_BUILD_UNIT_DNS_APP_SAMPLE})
            # Выполняем сборку приложения работы с юнитом резолвером доменных имён
            target_link_libraries(
                ${UNIT_APP_DNS_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_UNIT_DNS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с юнитом синхронизации времени NTP
        if (${CMAKE_BUILD_UNIT_NTP_APP_SAMPLE})
            # Выполняем сборку приложения работы с юнитом синхронизации времени NTP
            target_link_libraries(
                ${UNIT_APP_NTP_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_UNIT_NTP_APP_SAMPLE})

        # Если нужно собрать семплы для работы с юнитом перенаправления портов
        if (${CMAKE_BUILD_UNIT_PORTMAP_APP_SAMPLE})
            # Выполняем сборку приложения работы с юнитом перенаправления портов
            target_link_libraries(
                ${UNIT_APP_PORTMAP_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_UNIT_PORTMAP_APP_SAMPLE})

        # Если нужно собрать семплы присмотра за перенаправлением порта
        if (${CMAKE_BUILD_UNIT_WATCHDOG_APP_SAMPLE})
            # Выполняем сборку приложения присмотра за перенаправлением порта
            target_link_libraries(
                ${UNIT_APP_WATCHDOG_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_UNIT_WATCHDOG_APP_SAMPLE})

        # Если нужно собрать семплы для работы с юнитом ICMP-клиентом
        if (${CMAKE_BUILD_UNIT_ICMP_APP_SAMPLE})
            # Выполняем сборку приложения работы с юнитом ICMP-клиентом
            target_link_libraries(
                ${UNIT_APP_ICMP_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_UNIT_ICMP_APP_SAMPLE})

        # Если нужно собрать семплы для работы с юнитом таймера
        if (${CMAKE_BUILD_UNIT_TIMER_APP_SAMPLE})
            # Выполняем сборку приложения работы с юнитом таймера
            target_link_libraries(
                ${UNIT_APP_TIMER_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_UNIT_TIMER_APP_SAMPLE})
    
        # Если нужно собрать семплы для работы с юнитом кластера
        if (${CMAKE_BUILD_UNIT_CLUSTER_APP_SAMPLE})
            # Выполняем сборку приложения работы с юнитом кластера
            target_link_libraries(
                ${UNIT_APP_CLUSTER_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_UNIT_CLUSTER_APP_SAMPLE})

        # Если нужно собрать семплы для работы с юнитом уведомителя
        if (${CMAKE_BUILD_UNIT_NOTIFIER_APP_SAMPLE})
            # Выполняем сборку приложения работы с юнитом уведомителя
            target_link_libraries(
                ${UNIT_APP_NOTIFIER_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_UNIT_NOTIFIER_APP_SAMPLE})
    
        # Если нужно собрать семплы для работы с IP-адресами
        if (${CMAKE_BUILD_IP_NET_APP_SAMPLE})
            # Выполняем сборку приложения работы с IP-адресами
            target_link_libraries(
                ${IP_APP_NET_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_IP_NET_APP_SAMPLE})

        # Если нужно собрать семплы для работы с URI
        if (${CMAKE_BUILD_URI_NET_APP_SAMPLE})
            # Выполняем сборку приложения работы с URI
            target_link_libraries(
                ${URI_APP_NET_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_URI_NET_APP_SAMPLE})

        # Если нужно собрать семплы для работы с Ethernet-адресами
        if (${CMAKE_BUILD_ETH_NET_APP_SAMPLE})
            # Выполняем сборку приложения работы с Ethernet-адресами
            target_link_libraries(
                ${ETH_APP_NET_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_ETH_NET_APP_SAMPLE})

        # Если нужно собрать семплы для работы с распределителем памяти
        if (${CMAKE_BUILD_ALLOC_APP_SAMPLE})
            # Выполняем сборку приложения распределителя памяти
            target_link_libraries(
                ${ALLOC_APP_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_ALLOC_APP_SAMPLE})

        # Если нужно собрать семплы для работы со своим источником страниц
        if (${CMAKE_BUILD_SOURCE_APP_SAMPLE})
            # Выполняем сборку приложения своего источника страниц
            target_link_libraries(
                ${SOURCE_APP_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_SOURCE_APP_SAMPLE})

        # Если нужно собрать семплы настройки распределителя под нагрузку
        if (${CMAKE_BUILD_TUNING_APP_SAMPLE})
            # Выполняем сборку приложения настройки распределителя
            target_link_libraries(
                ${TUNING_APP_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_TUNING_APP_SAMPLE})

        # Если нужно собрать семплы поиска дефектов памяти прикладного кода
        if (${CMAKE_BUILD_DEBUG_APP_SAMPLE})
            # Выполняем сборку приложения поиска дефектов памяти
            target_link_libraries(
                ${DEBUG_APP_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_DEBUG_APP_SAMPLE})

        # Если нужно собрать семплы для работы с укрытой памятью
        if (${CMAKE_BUILD_KEEPER_APP_SAMPLE})
            # Выполняем сборку приложения укрытой памяти
            target_link_libraries(
                ${KEEPER_APP_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_KEEPER_APP_SAMPLE})

        # Если нужно собрать семплы для работы с датой и временем
        if (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})
            # Выполняем сборку приложения даты и времени
            target_link_libraries(
                ${DATE_APP_SYS_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с регулярными выражениями
        if (${CMAKE_BUILD_EXPRESSION_REGEX_APP_SAMPLE})
            # Выполняем сборку приложения работы с регулярными выражениями
            target_link_libraries(
                ${EXPRESSION_APP_REGEX_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_EXPRESSION_REGEX_APP_SAMPLE})

        # Если нужно собрать семплы разбора текста по шаблонам Grok
        if (${CMAKE_BUILD_GROK_REGEX_APP_SAMPLE})
            # Выполняем сборку приложения разбора текста по шаблонам Grok
            target_link_libraries(
                ${GROK_APP_REGEX_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_GROK_REGEX_APP_SAMPLE})

        # Если нужно собрать семплы для работы с бинарным контейнером
        if (${CMAKE_BUILD_BINBOX_CONTAINER_APP_SAMPLE})
            # Выполняем сборку приложения работы с бинарным контейнером
            target_link_libraries(
                ${BINBOX_APP_CONTAINER_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_BINBOX_CONTAINER_APP_SAMPLE})

        # Если нужно собрать семплы для работы с файловой системой
        if (${CMAKE_BUILD_FILESYSTEM_SYS_APP_SAMPLE})
            # Выполняем сборку приложения файловой системы
            target_link_libraries(
                ${FILESYSTEM_APP_SYS_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_FILESYSTEM_SYS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с резольвером процессов
        if (${CMAKE_BUILD_PROCRE_SYS_APP_SAMPLE})
            # Выполняем сборку приложения резольвера процессов
            target_link_libraries(
                ${PROCRE_APP_SYS_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_PROCRE_SYS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с волокнами
        if (${CMAKE_BUILD_FIBER_SYS_APP_SAMPLE})
            # Выполняем сборку приложения работы с волокнами
            target_link_libraries(
                ${FIBER_APP_SYS_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_FIBER_SYS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с шифрованием и хэшированием
        if (${CMAKE_BUILD_CRYPTO_CRYPTOGRAPHY_APP_SAMPLE})
            # Выполняем сборку приложения шифрования и хэширования
            target_link_libraries(
                ${CRYPTO_APP_CRYPTOGRAPHY_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_CRYPTO_CRYPTOGRAPHY_APP_SAMPLE})

        # Если нужно собрать семплы для работы с хэшированием данных
        if (${CMAKE_BUILD_HASH_CRYPTOGRAPHY_APP_SAMPLE})
            # Выполняем сборку приложения хэширования данных
            target_link_libraries(
                ${HASH_APP_CRYPTOGRAPHY_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_HASH_CRYPTOGRAPHY_APP_SAMPLE})

        # Если нужно собрать семплы для работы с электронной подписью
        if (${CMAKE_BUILD_SIGNATURE_CRYPTOGRAPHY_APP_SAMPLE})
            # Выполняем сборку приложения работы с электронной подписью
            target_link_libraries(
                ${SIGNATURE_APP_CRYPTOGRAPHY_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_SIGNATURE_CRYPTOGRAPHY_APP_SAMPLE})

        # Если нужно собрать семплы для работы с HTTP-авторизацией
        if (${CMAKE_BUILD_AUTH_PROTO_APP_SAMPLE})
            # Выполняем сборку приложения работы с HTTP-авторизацией
            target_link_libraries(
                ${AUTH_APP_PROTO_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_AUTH_PROTO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с HTTP-заголовками
        if (${CMAKE_BUILD_HEADERS_PROTO_APP_SAMPLE})
            # Выполняем сборку приложения работы с HTTP-заголовками
            target_link_libraries(
                ${HEADERS_APP_PROTO_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_HEADERS_PROTO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с парсером HTTP/1.0 и HTTP/1.1
        if (${CMAKE_BUILD_PARSER_PROTO_APP_SAMPLE})
            # Выполняем сборку приложения работы с парсером HTTP/1.0 и HTTP/1.1
            target_link_libraries(
                ${PARSER_APP_PROTO_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_PARSER_PROTO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с парсером HTTP/2
        if (${CMAKE_BUILD_PARSER2_PROTO_APP_SAMPLE})
            # Выполняем сборку приложения работы с парсером HTTP/2
            target_link_libraries(
                ${PARSER2_APP_PROTO_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_PARSER2_PROTO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с протоколом SOCKS5
        if (${CMAKE_BUILD_SOCKS5_PROTO_APP_SAMPLE})
            # Выполняем сборку приложения работы с протоколом SOCKS5
            target_link_libraries(
                ${SOCKS5_APP_PROTO_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_SOCKS5_PROTO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с протоколом QUIC
        if (${CMAKE_BUILD_QUIC_PROTO_APP_SAMPLE})
            # Выполняем сборку приложения работы с QUIC клиентом
            target_link_libraries(
                ${QUIC_CLIENT_APP_PROTO_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения работы с QUIC сервером
            target_link_libraries(
                ${QUIC_SERVER_APP_PROTO_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
            # Выполняем сборку клиентского приложения проверки QUIC-прокси на объединении данных (splice)
            target_link_libraries(
                ${QUIC_PROXY_CLIENT_APP_PROTO_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
            # Выполняем сборку серверного приложения QUIC-прокси на объединении данных (splice)
            target_link_libraries(
                ${QUIC_PROXY_SERVER_APP_PROTO_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_QUIC_PROTO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с контейнером XML
        if (${CMAKE_BUILD_CODEC_XML_APP_SAMPLE})
            # Выполняем сборку приложения потокового чтения текста разметки
            target_link_libraries(
                ${CODEC_APP_XML_READER_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения работы с деревом разметки
            target_link_libraries(
                ${CODEC_APP_XML_DOCUMENT_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения записи текста разметки
            target_link_libraries(
                ${CODEC_APP_XML_WRITER_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения получения содержимого разметки числами
            target_link_libraries(
                ${CODEC_APP_XML_NUMERIC_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_CODEC_XML_APP_SAMPLE})

        # Если нужно собрать семплы для работы с контейнером INI
        if (${CMAKE_BUILD_CODEC_INI_APP_SAMPLE})
            # Выполняем сборку приложения потокового чтения текста настроек
            target_link_libraries(
                ${CODEC_APP_INI_READER_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения работы с деревом настроек
            target_link_libraries(
                ${CODEC_APP_INI_DOCUMENT_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения записи текста настроек
            target_link_libraries(
                ${CODEC_APP_INI_WRITER_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_CODEC_INI_APP_SAMPLE})

        # Если нужно собрать семпл перевода настроек между видами записи
        if (${CMAKE_BUILD_CODEC_BRIDGE_APP_SAMPLE})
            # Выполняем сборку приложения перевода настроек между видами записи
            target_link_libraries(
                ${CODEC_APP_BRIDGE_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_CODEC_BRIDGE_APP_SAMPLE})

        # Если нужно собрать семпл работы с параметрами запуска приложения
        if (${CMAKE_BUILD_ARGS_APP_SAMPLE})
            # Выполняем сборку приложения работы с параметрами запуска
            target_link_libraries(
                ${ARGS_APP_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_ARGS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с контейнером TOML
        if (${CMAKE_BUILD_CODEC_TOML_APP_SAMPLE})
            # Выполняем сборку приложения потокового чтения текста настроек
            target_link_libraries(
                ${CODEC_APP_TOML_READER_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения работы с деревом настроек
            target_link_libraries(
                ${CODEC_APP_TOML_DOCUMENT_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения записи текста настроек
            target_link_libraries(
                ${CODEC_APP_TOML_WRITER_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_CODEC_TOML_APP_SAMPLE})

        # Если нужно собрать семплы для работы с контейнером YAML
        if (${CMAKE_BUILD_CODEC_YAML_APP_SAMPLE})
            # Выполняем сборку приложения потокового чтения текста документа
            target_link_libraries(
                ${CODEC_APP_YAML_READER_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения работы с деревом документа
            target_link_libraries(
                ${CODEC_APP_YAML_DOCUMENT_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения записи текста документа
            target_link_libraries(
                ${CODEC_APP_YAML_WRITER_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_CODEC_YAML_APP_SAMPLE})

        # Если нужно собрать семплы для работы с контейнером CSV
        if (${CMAKE_BUILD_CODEC_CSV_APP_SAMPLE})
            # Выполняем сборку приложения потокового чтения текста таблицы
            target_link_libraries(
                ${CODEC_APP_CSV_READER_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения работы с деревом таблицы
            target_link_libraries(
                ${CODEC_APP_CSV_DOCUMENT_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения записи текста таблицы
            target_link_libraries(
                ${CODEC_APP_CSV_WRITER_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_CODEC_CSV_APP_SAMPLE})

        # Если нужно собрать семплы для работы с контейнером JSON
        if (${CMAKE_BUILD_CODEC_JSON_APP_SAMPLE})
            # Выполняем сборку приложения потокового чтения текста документа
            target_link_libraries(
                ${CODEC_APP_JSON_READER_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения работы с деревом документа
            target_link_libraries(
                ${CODEC_APP_JSON_DOCUMENT_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения записи текста документа
            target_link_libraries(
                ${CODEC_APP_JSON_WRITER_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_CODEC_JSON_APP_SAMPLE})

        # Если нужно собрать семплы для работы с контейнером ABC
        if (${CMAKE_BUILD_CODEC_ABC_APP_SAMPLE})
            # Выполняем сборку приложения потокового чтения бинарной записи
            target_link_libraries(
                ${CODEC_APP_ABC_READER_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения работы с деревом документа
            target_link_libraries(
                ${CODEC_APP_ABC_DOCUMENT_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения сборки бинарной записи
            target_link_libraries(
                ${CODEC_APP_ABC_WRITER_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения работы с контейнером
            target_link_libraries(
                ${CODEC_APP_ABC_CONTAINER_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_CODEC_ABC_APP_SAMPLE})

        # Если нужно собрать семплы для работы с компрессией данных
        if (${CMAKE_BUILD_COMPRESSOR_SYS_APP_SAMPLE})
            # Выполняем сборку приложения компрессии данных
            target_link_libraries(
                ${COMPRESSOR_APP_BLOCK_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
            target_link_libraries(
                ${COMPRESSOR_APP_STREAM_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_COMPRESSOR_SYS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с каталогами
        if (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})
            # Выполняем сборку приложения работы с каталогами
            target_link_libraries(
                ${DIR_APP_IO_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_DIR_IO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с файлами
        if (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})
            # Выполняем сборку приложения работы с файлами
            target_link_libraries(
                ${FILE_APP_IO_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_FILE_IO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с пингами (ICMP)
        if (${CMAKE_BUILD_PING_IO_APP_SAMPLE})
            # Выполняем сборку приложения работы с пингами (ICMP)
            target_link_libraries(
                ${PING_APP_IO_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_PING_IO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с таймером
        if (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})
            # Выполняем сборку приложения таймера
            target_link_libraries(
                ${TIMER_APP_IO_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_TIMER_IO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с пользовательскими событиями
        if (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})
            # Выполняем сборку приложения пользовательских событий
            target_link_libraries(
                ${EVENTS_APP_IO_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_EVENTS_IO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с межпроцессорным взаимодействием
        if (${CMAKE_BUILD_IPC_IO_APP_SAMPLE})
            # Выполняем сборку приложения работы с межпроцессорным взаимодействием (именованный канал)
            target_link_libraries(
                ${IPC_PIPE_APP_IO_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с межпроцессорным взаимодействием (поток)
            target_link_libraries(
                ${IPC_STREAM_APP_IO_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с межпроцессорным взаимодействием (датаграмма)
            target_link_libraries(
                ${IPC_DATAGRAM_APP_IO_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с межпроцессорным взаимодействием (последовательная упаковка)
            target_link_libraries(
                ${IPC_SEQPACKET_APP_IO_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_IPC_IO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с TCP (клиент и сервер)
        if (${CMAKE_BUILD_NET_TCP_APP_SAMPLE})
            # Выполняем сборку приложения клиента
            target_link_libraries(
                ${CLIENT_APP_NET_TCP_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера
            target_link_libraries(
                ${SERVER_APP_NET_TCP_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_TCP_APP_SAMPLE})

        # Если нужно собрать семплы для работы с UDP (клиент и сервер)
        if (${CMAKE_BUILD_NET_UDP_APP_SAMPLE})
            # Выполняем сборку приложения клиента
            target_link_libraries(
                ${CLIENT_APP_NET_UDP_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения клиента с подключением
            target_link_libraries(
                ${CLIENT_APP_NET_UDP_CONN_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера
            target_link_libraries(
                ${SERVER_APP_NET_UDP_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_UDP_APP_SAMPLE})

        # Если нужно собрать семплы для работы с ограничителем пропускной способности (клиент и сервер)
        if (${CMAKE_BUILD_NET_BANDWIDTH_APP_SAMPLE})
            # Выполняем сборку приложения клиента
            target_link_libraries(
                ${CLIENT_APP_NET_TCP_BANDWIDTH_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера
            target_link_libraries(
                ${SERVER_APP_NET_TCP_BANDWIDTH_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_BANDWIDTH_APP_SAMPLE})

        # Если нужно собрать семплы для работы с TUN (клиент и сервер)
        if (${CMAKE_BUILD_NET_TUN_APP_SAMPLE})
           # Выполняем сборку приложения клиента
            target_link_libraries(
                ${CLIENT_APP_NET_TUN_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера
            target_link_libraries(
                ${SERVER_APP_NET_TUN_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_TUN_APP_SAMPLE})

        # Если нужно собрать семплы для работы с UDP Broadcast (клиент и сервер)
        if (${CMAKE_BUILD_NET_BROADCAST_APP_SAMPLE})
            # Выполняем сборку приложения клиента
            target_link_libraries(
                ${CLIENT_APP_NET_BROADCAST_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера
            target_link_libraries(
                ${SERVER_APP_NET_BROADCAST_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_BROADCAST_APP_SAMPLE})

        # Если нужно собрать семплы для работы с UDP Multicast (клиент и сервер)
        if (${CMAKE_BUILD_NET_MULTICAST_APP_SAMPLE})
            # Выполняем сборку приложения клиента сценарий 1
            target_link_libraries(
                ${CLIENT_APP_NET_MULTICAST1_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера сценарий 1
            target_link_libraries(
                ${SERVER_APP_NET_MULTICAST1_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения клиента сценарий 3
            target_link_libraries(
                ${CLIENT_APP_NET_MULTICAST3_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера сценарий 3
            target_link_libraries(
                ${SERVER_APP_NET_MULTICAST3_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_MULTICAST_APP_SAMPLE})

        # Если нужно собрать семплы для работы с UDP Splice (клиент и сервер)
        if (${CMAKE_BUILD_NET_SPLICE_APP_SAMPLE})
            # Выполняем сборку приложения клиента
            target_link_libraries(
                ${CLIENT_APP_NET_SPLICE_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера
            target_link_libraries(
                ${SERVER_APP_NET_SPLICE_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_SPLICE_APP_SAMPLE})

        # Если нужно собрать семплы для работы с UDS (клиент и сервер)
        if (${CMAKE_BUILD_NET_UDS_APP_SAMPLE})
            # Выполняем сборку приложения клиента UDS-TCP
            target_link_libraries(
                ${CLIENT_APP_NET_UDS_TCP_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера UDS-TCP
            target_link_libraries(
                ${SERVER_APP_NET_UDS_TCP_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения клиента UDS-UDP
            target_link_libraries(
                ${CLIENT_APP_NET_UDS_UDP_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера UDS-UDP
            target_link_libraries(
                ${SERVER_APP_NET_UDS_UDP_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_UDS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с DTLS (клиент и сервер)
        if (${CMAKE_BUILD_NET_DTLS_APP_SAMPLE})
            # Выполняем сборку приложения клиента
            target_link_libraries(
                ${CLIENT_APP_NET_DTLS_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера
            target_link_libraries(
                ${SERVER_APP_NET_DTLS_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_DTLS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с TLS (клиент и сервер)
        if (${CMAKE_BUILD_NET_TLS_APP_SAMPLE})
            # Выполняем сборку приложения клиента работы с TLS
            target_link_libraries(
                ${CLIENT_APP_NET_TLS_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера работы с TLS
            target_link_libraries(
                ${SERVER_APP_NET_TLS_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения клиента работы с TLS и HTTP/1.1 подключением
            target_link_libraries(
                ${CLIENT_APP_NET_TLS_WEB_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера работы с TLS и мультисертификатом
            target_link_libraries(
                ${SERVER_APP_NET_TLS_MULTI_NAME}
                ${AWH_LIB_NAME}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_TLS_APP_SAMPLE})

        # Если сборка производится под систему с поддержкой SCTP: FreeBSD, Linux, Solaris или illumos
        if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
            # Если нужно собрать семплы для работы с SCTP (клиент и сервер)
            if (${CMAKE_BUILD_NET_SCTP_APP_SAMPLE})
                # Выполняем сборку приложения клиента работы с TLS
                target_link_libraries(
                    ${CLIENT_APP_NET_SCTP_TLS_NAME}
                    ${AWH_LIB_NAME}
                )

                # Выполняем сборку приложения сервера работы с TLS
                target_link_libraries(
                    ${SERVER_APP_NET_SCTP_TLS_NAME}
                    ${AWH_LIB_NAME}
                )

                # Выполняем сборку приложения клиента работы с DTLS
                target_link_libraries(
                    ${CLIENT_APP_NET_SCTP_DTLS_NAME}
                    ${AWH_LIB_NAME}
                )

                # Выполняем сборку приложения сервера работы с DTLS
                target_link_libraries(
                    ${SERVER_APP_NET_SCTP_DTLS_NAME}
                    ${AWH_LIB_NAME}
                )

                # Выполняем сборку приложения клиента работы с SCTP - AUTH
                target_link_libraries(
                    ${CLIENT_APP_NET_SCTP_AUTH_NAME}
                    ${AWH_LIB_NAME}
                )

                # Выполняем сборку приложения сервера работы с SCTP - AUTH
                target_link_libraries(
                    ${SERVER_APP_NET_SCTP_AUTH_NAME}
                    ${AWH_LIB_NAME}
                )

                # Выполняем сборку приложения клиента работы с SCTP - STREAM
                target_link_libraries(
                    ${CLIENT_APP_NET_SCTP_STREAM_NAME}
                    ${AWH_LIB_NAME}
                )

                # Выполняем сборку приложения сервера работы с SCTP - STREAM
                target_link_libraries(
                    ${SERVER_APP_NET_SCTP_STREAM_NAME}
                    ${AWH_LIB_NAME}
                )

                # Выполняем сборку приложения клиента работы с SCTP - SEQPACKET
                target_link_libraries(
                    ${CLIENT_APP_NET_SCTP_SEQPACKET_NAME}
                    ${AWH_LIB_NAME}
                )

                # Выполняем сборку приложения сервера работы с SCTP - SEQPACKET
                target_link_libraries(
                    ${SERVER_APP_NET_SCTP_SEQPACKET_NAME}
                    ${AWH_LIB_NAME}
                )

                # Выполняем сборку приложения клиента переноса файла по SCTP
                target_link_libraries(
                    ${CLIENT_APP_NET_SCTP_TRANSFER_NAME}
                    ${AWH_LIB_NAME}
                )

                # Выполняем сборку приложения сервера переноса файла по SCTP
                target_link_libraries(
                    ${SERVER_APP_NET_SCTP_TRANSFER_NAME}
                    ${AWH_LIB_NAME}
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
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом UDS версии 2
            target_link_libraries(
                ${CLIENT_APP_UDS-2_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом TCP
            target_link_libraries(
                ${CLIENT_APP_TCP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом TCP версии 2
            target_link_libraries(
                ${CLIENT_APP_TCP-2_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом UDP
            target_link_libraries(
                ${CLIENT_APP_UDP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом UDP версии 2
            target_link_libraries(
                ${CLIENT_APP_UDP-2_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом TLS
            target_link_libraries(
                ${CLIENT_APP_TLS_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом TLS версии 2
            target_link_libraries(
                ${CLIENT_APP_TLS-2_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом DTLS
            target_link_libraries(
                ${CLIENT_APP_DTLS_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом DTLS версии 2
            target_link_libraries(
                ${CLIENT_APP_DTLS-2_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с клиентом TLS-Web
            target_link_libraries(
                ${CLIENT_APP_TLS-WEB_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )

            # Если сборка производится под систему с поддержкой SCTP: FreeBSD, Linux, Solaris или illumos
            if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
                # Выполняем сборку приложения работы с клиентом SCTP
                target_link_libraries(
                    ${CLIENT_APP_SCTP_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                    ${AWH_FLAGS}
                )

                # Выполняем сборку приложения работы с клиентом SCTP версии 2
                target_link_libraries(
                    ${CLIENT_APP_SCTP-2_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
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
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером UDS версии 2
            target_link_libraries(
                ${SERVER_APP_UDS-2_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером TCP
            target_link_libraries(
                ${SERVER_APP_TCP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером TCP версии 2
            target_link_libraries(
                ${SERVER_APP_TCP-2_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером UDP
            target_link_libraries(
                ${SERVER_APP_UDP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером UDP версии 2
            target_link_libraries(
                ${SERVER_APP_UDP-2_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером TLS
            target_link_libraries(
                ${SERVER_APP_TLS_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером TLS мультисертификатным
            target_link_libraries(
                ${SERVER_APP_TLS-MULTICERT_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером DTLS
            target_link_libraries(
                ${SERVER_APP_DTLS_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с сервером DTLS мультисертификатным
            target_link_libraries(
                ${SERVER_APP_DTLS-MULTICERT_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )

            # Если сборка производится под систему с поддержкой SCTP: FreeBSD, Linux, Solaris или illumos
            if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
                # Выполняем сборку приложения работы с сервером SCTP
                target_link_libraries(
                    ${SERVER_APP_SCTP_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                    ${AWH_FLAGS}
                )

                # Выполняем сборку приложения работы с сервером SCTP версии 2
                target_link_libraries(
                    ${SERVER_APP_SCTP-2_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
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
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с юнитом VPN сервером
            target_link_libraries(
                ${SERVER_APP_VPN_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
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
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с юнитом SOCKS5 клиентом для UDP
            target_link_libraries(
                ${CLIENT_APP_SOCKS5-UDP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с юнитом SOCKS5 клиентом для DTLS
            target_link_libraries(
                ${CLIENT_APP_SOCKS5-DTLS_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с юнитом SOCKS5 сервером
            target_link_libraries(
               ${SERVER_APP_SOCKS5_NAME}
               ${AWH_LIB_NAME}
               ${DEPEND_LIBRARY}
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
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_UNIT_NTP_APP_SAMPLE})

        # Если нужно собрать семплы для работы с юнитом перенаправления портов
        if (${CMAKE_BUILD_UNIT_PORTMAP_APP_SAMPLE})
            # Выполняем сборку приложения работы с юнитом перенаправления портов
            target_link_libraries(
                ${UNIT_APP_PORTMAP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_UNIT_PORTMAP_APP_SAMPLE})

        # Если нужно собрать семплы присмотра за перенаправлением порта
        if (${CMAKE_BUILD_UNIT_WATCHDOG_APP_SAMPLE})
            # Выполняем сборку приложения присмотра за перенаправлением порта
            target_link_libraries(
                ${UNIT_APP_WATCHDOG_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_UNIT_WATCHDOG_APP_SAMPLE})

        # Если нужно собрать семплы для работы с юнитом ICMP-клиентом
        if (${CMAKE_BUILD_UNIT_ICMP_APP_SAMPLE})
            # Выполняем сборку приложения работы с юнитом ICMP-клиентом
            target_link_libraries(
                ${UNIT_APP_ICMP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
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
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_ETH_NET_APP_SAMPLE})

        # Если нужно собрать семплы для работы с распределителем памяти
        if (${CMAKE_BUILD_ALLOC_APP_SAMPLE})
            # Выполняем сборку приложения распределителя памяти
            target_link_libraries(
                ${ALLOC_APP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_ALLOC_APP_SAMPLE})

        # Если нужно собрать семплы для работы со своим источником страниц
        if (${CMAKE_BUILD_SOURCE_APP_SAMPLE})
            # Выполняем сборку приложения своего источника страниц
            target_link_libraries(
                ${SOURCE_APP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_SOURCE_APP_SAMPLE})

        # Если нужно собрать семплы настройки распределителя под нагрузку
        if (${CMAKE_BUILD_TUNING_APP_SAMPLE})
            # Выполняем сборку приложения настройки распределителя
            target_link_libraries(
                ${TUNING_APP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_TUNING_APP_SAMPLE})

        # Если нужно собрать семплы поиска дефектов памяти прикладного кода
        if (${CMAKE_BUILD_DEBUG_APP_SAMPLE})
            # Выполняем сборку приложения поиска дефектов памяти
            target_link_libraries(
                ${DEBUG_APP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_DEBUG_APP_SAMPLE})

        # Если нужно собрать семплы для работы с укрытой памятью
        if (${CMAKE_BUILD_KEEPER_APP_SAMPLE})
            # Выполняем сборку приложения укрытой памяти
            target_link_libraries(
                ${KEEPER_APP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_KEEPER_APP_SAMPLE})

        # Если нужно собрать семплы для работы с датой и временем
        if (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})
            # Выполняем сборку приложения даты и времени
            target_link_libraries(
                ${DATE_APP_SYS_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с регулярными выражениями
        if (${CMAKE_BUILD_EXPRESSION_REGEX_APP_SAMPLE})
            # Выполняем сборку приложения работы с регулярными выражениями
            target_link_libraries(
                ${EXPRESSION_APP_REGEX_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_EXPRESSION_REGEX_APP_SAMPLE})

        # Если нужно собрать семплы разбора текста по шаблонам Grok
        if (${CMAKE_BUILD_GROK_REGEX_APP_SAMPLE})
            # Выполняем сборку приложения разбора текста по шаблонам Grok
            target_link_libraries(
                ${GROK_APP_REGEX_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_GROK_REGEX_APP_SAMPLE})

        # Если нужно собрать семплы для работы с бинарным контейнером
        if (${CMAKE_BUILD_BINBOX_CONTAINER_APP_SAMPLE})
            # Выполняем сборку приложения работы с бинарным контейнером
            target_link_libraries(
                ${BINBOX_APP_CONTAINER_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_BINBOX_CONTAINER_APP_SAMPLE})

        # Если нужно собрать семплы для работы с файловой системой
        if (${CMAKE_BUILD_FILESYSTEM_SYS_APP_SAMPLE})
            # Выполняем сборку приложения файловой системы
            target_link_libraries(
                ${FILESYSTEM_APP_SYS_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
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
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_PROCRE_SYS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с волокнами
        if (${CMAKE_BUILD_FIBER_SYS_APP_SAMPLE})
            # Выполняем сборку приложения работы с волокнами
            target_link_libraries(
                ${FIBER_APP_SYS_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_FIBER_SYS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с шифрованием и хэшированием
        if (${CMAKE_BUILD_CRYPTO_CRYPTOGRAPHY_APP_SAMPLE})
            # Выполняем сборку приложения шифрования и хэширования
            target_link_libraries(
                ${CRYPTO_APP_CRYPTOGRAPHY_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_CRYPTO_CRYPTOGRAPHY_APP_SAMPLE})

        # Если нужно собрать семплы для работы с хэшированием данных
        if (${CMAKE_BUILD_HASH_CRYPTOGRAPHY_APP_SAMPLE})
            # Выполняем сборку приложения хэширования данных
            target_link_libraries(
                ${HASH_APP_CRYPTOGRAPHY_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_HASH_CRYPTOGRAPHY_APP_SAMPLE})

        # Если нужно собрать семплы для работы с электронной подписью
        if (${CMAKE_BUILD_SIGNATURE_CRYPTOGRAPHY_APP_SAMPLE})
            # Выполняем сборку приложения работы с электронной подписью
            target_link_libraries(
                ${SIGNATURE_APP_CRYPTOGRAPHY_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_SIGNATURE_CRYPTOGRAPHY_APP_SAMPLE})

        # Если нужно собрать семплы для работы с HTTP-авторизацией
        if (${CMAKE_BUILD_AUTH_PROTO_APP_SAMPLE})
            # Выполняем сборку приложения работы с HTTP-авторизацией
            target_link_libraries(
                ${AUTH_APP_PROTO_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
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
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения работы с QUIC сервером
            target_link_libraries(
                ${QUIC_SERVER_APP_PROTO_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
            # Выполняем сборку клиентского приложения проверки QUIC-прокси на объединении данных (splice)
            target_link_libraries(
                ${QUIC_PROXY_CLIENT_APP_PROTO_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
            # Выполняем сборку серверного приложения QUIC-прокси на объединении данных (splice)
            target_link_libraries(
                ${QUIC_PROXY_SERVER_APP_PROTO_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_QUIC_PROTO_APP_SAMPLE})

        # Если нужно собрать семплы для работы с контейнером XML
        if (${CMAKE_BUILD_CODEC_XML_APP_SAMPLE})
            # Выполняем сборку приложения потокового чтения текста разметки
            target_link_libraries(
                ${CODEC_APP_XML_READER_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения работы с деревом разметки
            target_link_libraries(
                ${CODEC_APP_XML_DOCUMENT_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения записи текста разметки
            target_link_libraries(
                ${CODEC_APP_XML_WRITER_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения получения содержимого разметки числами
            target_link_libraries(
                ${CODEC_APP_XML_NUMERIC_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_CODEC_XML_APP_SAMPLE})

        # Если нужно собрать семплы для работы с контейнером INI
        if (${CMAKE_BUILD_CODEC_INI_APP_SAMPLE})
            # Выполняем сборку приложения потокового чтения текста настроек
            target_link_libraries(
                ${CODEC_APP_INI_READER_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения работы с деревом настроек
            target_link_libraries(
                ${CODEC_APP_INI_DOCUMENT_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения записи текста настроек
            target_link_libraries(
                ${CODEC_APP_INI_WRITER_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_CODEC_INI_APP_SAMPLE})

        # Если нужно собрать семпл перевода настроек между видами записи
        if (${CMAKE_BUILD_CODEC_BRIDGE_APP_SAMPLE})
            # Выполняем сборку приложения перевода настроек между видами записи
            target_link_libraries(
                ${CODEC_APP_BRIDGE_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_CODEC_BRIDGE_APP_SAMPLE})

        # Если нужно собрать семпл работы с параметрами запуска приложения
        if (${CMAKE_BUILD_ARGS_APP_SAMPLE})
            # Выполняем сборку приложения работы с параметрами запуска
            target_link_libraries(
                ${ARGS_APP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_ARGS_APP_SAMPLE})

        # Если нужно собрать семплы для работы с контейнером TOML
        if (${CMAKE_BUILD_CODEC_TOML_APP_SAMPLE})
            # Выполняем сборку приложения потокового чтения текста настроек
            target_link_libraries(
                ${CODEC_APP_TOML_READER_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения работы с деревом настроек
            target_link_libraries(
                ${CODEC_APP_TOML_DOCUMENT_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения записи текста настроек
            target_link_libraries(
                ${CODEC_APP_TOML_WRITER_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_CODEC_TOML_APP_SAMPLE})

        # Если нужно собрать семплы для работы с контейнером YAML
        if (${CMAKE_BUILD_CODEC_YAML_APP_SAMPLE})
            # Выполняем сборку приложения потокового чтения текста документа
            target_link_libraries(
                ${CODEC_APP_YAML_READER_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения работы с деревом документа
            target_link_libraries(
                ${CODEC_APP_YAML_DOCUMENT_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения записи текста документа
            target_link_libraries(
                ${CODEC_APP_YAML_WRITER_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_CODEC_YAML_APP_SAMPLE})

        # Если нужно собрать семплы для работы с контейнером CSV
        if (${CMAKE_BUILD_CODEC_CSV_APP_SAMPLE})
            # Выполняем сборку приложения потокового чтения текста таблицы
            target_link_libraries(
                ${CODEC_APP_CSV_READER_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения работы с деревом таблицы
            target_link_libraries(
                ${CODEC_APP_CSV_DOCUMENT_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения записи текста таблицы
            target_link_libraries(
                ${CODEC_APP_CSV_WRITER_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_CODEC_CSV_APP_SAMPLE})

        # Если нужно собрать семплы для работы с контейнером JSON
        if (${CMAKE_BUILD_CODEC_JSON_APP_SAMPLE})
            # Выполняем сборку приложения потокового чтения текста документа
            target_link_libraries(
                ${CODEC_APP_JSON_READER_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения работы с деревом документа
            target_link_libraries(
                ${CODEC_APP_JSON_DOCUMENT_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения записи текста документа
            target_link_libraries(
                ${CODEC_APP_JSON_WRITER_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_CODEC_JSON_APP_SAMPLE})

        # Если нужно собрать семплы для работы с контейнером ABC
        if (${CMAKE_BUILD_CODEC_ABC_APP_SAMPLE})
            # Выполняем сборку приложения потокового чтения бинарной записи
            target_link_libraries(
                ${CODEC_APP_ABC_READER_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения работы с деревом документа
            target_link_libraries(
                ${CODEC_APP_ABC_DOCUMENT_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения сборки бинарной записи
            target_link_libraries(
                ${CODEC_APP_ABC_WRITER_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
            # Выполняем сборку приложения работы с контейнером
            target_link_libraries(
                ${CODEC_APP_ABC_CONTAINER_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_CODEC_ABC_APP_SAMPLE})

        # Если нужно собрать семплы для работы с компрессией данных
        if (${CMAKE_BUILD_COMPRESSOR_SYS_APP_SAMPLE})
            # Выполняем сборку приложения компрессии данных
            target_link_libraries(
                ${COMPRESSOR_APP_BLOCK_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
            target_link_libraries(
                ${COMPRESSOR_APP_STREAM_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
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
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с межпроцессорным взаимодействием (поток)
            target_link_libraries(
                ${IPC_STREAM_APP_IO_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с межпроцессорным взаимодействием (датаграмма)
            target_link_libraries(
                ${IPC_DATAGRAM_APP_IO_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения работы с межпроцессорным взаимодействием (последовательная упаковка)
            target_link_libraries(
                ${IPC_SEQPACKET_APP_IO_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
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
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера
            target_link_libraries(
                ${SERVER_APP_NET_TCP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
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
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения клиента с подключением
            target_link_libraries(
                ${CLIENT_APP_NET_UDP_CONN_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера
            target_link_libraries(
                ${SERVER_APP_NET_UDP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
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
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера
            target_link_libraries(
                ${SERVER_APP_NET_TCP_BANDWIDTH_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
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
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера
            target_link_libraries(
                ${SERVER_APP_NET_TUN_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
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
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера
            target_link_libraries(
                ${SERVER_APP_NET_BROADCAST_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
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
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера сценарий 1
            target_link_libraries(
                ${SERVER_APP_NET_MULTICAST1_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения клиента сценарий 3
            target_link_libraries(
                ${CLIENT_APP_NET_MULTICAST3_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера сценарий 3
            target_link_libraries(
                ${SERVER_APP_NET_MULTICAST3_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
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
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера
            target_link_libraries(
                ${SERVER_APP_NET_SPLICE_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
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
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера UDS-TCP
            target_link_libraries(
                ${SERVER_APP_NET_UDS_TCP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения клиента UDS-UDP
            target_link_libraries(
                ${CLIENT_APP_NET_UDS_UDP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера UDS-UDP
            target_link_libraries(
                ${SERVER_APP_NET_UDS_UDP_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
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
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера
            target_link_libraries(
                ${SERVER_APP_NET_DTLS_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
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
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера работы с TLS
            target_link_libraries(
                ${SERVER_APP_NET_TLS_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения клиента работы с TLS и HTTP/1.1 подключением
            target_link_libraries(
                ${CLIENT_APP_NET_TLS_WEB_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )

            # Выполняем сборку приложения сервера работы с TLS и мультисертификатом
            target_link_libraries(
                ${SERVER_APP_NET_TLS_MULTI_NAME}
                ${AWH_LIB_NAME}
                ${DEPEND_LIBRARY}
                ${AWH_FLAGS}
            )
        endif (${CMAKE_BUILD_NET_TLS_APP_SAMPLE})

        # Если сборка производится под систему с поддержкой SCTP: FreeBSD, Linux, Solaris или illumos
        if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
            # Если нужно собрать семплы для работы с SCTP (клиент и сервер)
            if (${CMAKE_BUILD_NET_SCTP_APP_SAMPLE})
                # Выполняем сборку приложения клиента работы с TLS
                target_link_libraries(
                    ${CLIENT_APP_NET_SCTP_TLS_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                )

                # Выполняем сборку приложения сервера работы с TLS
                target_link_libraries(
                    ${SERVER_APP_NET_SCTP_TLS_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                )

                # Выполняем сборку приложения клиента работы с DTLS
                target_link_libraries(
                    ${CLIENT_APP_NET_SCTP_DTLS_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                )

                # Выполняем сборку приложения сервера работы с DTLS
                target_link_libraries(
                    ${SERVER_APP_NET_SCTP_DTLS_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                )

                # Выполняем сборку приложения клиента работы с SCTP - AUTH
                target_link_libraries(
                    ${CLIENT_APP_NET_SCTP_AUTH_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                )

                # Выполняем сборку приложения сервера работы с SCTP - AUTH
                target_link_libraries(
                    ${SERVER_APP_NET_SCTP_AUTH_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                )

                # Выполняем сборку приложения клиента работы с SCTP - STREAM
                target_link_libraries(
                    ${CLIENT_APP_NET_SCTP_STREAM_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                )

                # Выполняем сборку приложения сервера работы с SCTP - STREAM
                target_link_libraries(
                    ${SERVER_APP_NET_SCTP_STREAM_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                )

                # Выполняем сборку приложения клиента работы с SCTP - SEQPACKET
                target_link_libraries(
                    ${CLIENT_APP_NET_SCTP_SEQPACKET_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                )

                # Выполняем сборку приложения сервера работы с SCTP - SEQPACKET
                target_link_libraries(
                    ${SERVER_APP_NET_SCTP_SEQPACKET_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                )

                # Выполняем сборку приложения клиента переноса файла по SCTP
                target_link_libraries(
                    ${CLIENT_APP_NET_SCTP_TRANSFER_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
                )

                # Выполняем сборку приложения сервера переноса файла по SCTP
                target_link_libraries(
                    ${SERVER_APP_NET_SCTP_TRANSFER_NAME}
                    ${AWH_LIB_NAME}
                    ${DEPEND_LIBRARY}
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

                # Если сборка производится под систему с поддержкой SCTP: FreeBSD, Linux, Solaris или illumos
                if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
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

                # Если сборка производится под систему с поддержкой SCTP: FreeBSD, Linux, Solaris или illumos
                if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
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

            # Если нужно собрать семплы для работы с распределителем памяти
            if (${CMAKE_BUILD_ALLOC_APP_SAMPLE})
                # Выполняем сборку приложения распределителя памяти
                add_custom_command(TARGET "${ALLOC_APP_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${ALLOC_APP_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${ALLOC_APP_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_ALLOC_APP_SAMPLE})

            # Если нужно собрать семплы поиска дефектов памяти прикладного кода
            if (${CMAKE_BUILD_DEBUG_APP_SAMPLE})
                # Подписываем приложение поиска дефектов памяти
                add_custom_command(TARGET "${DEBUG_APP_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${DEBUG_APP_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${DEBUG_APP_NAME}\" to enable core dump on macOS"
                )
            endif (${CMAKE_BUILD_DEBUG_APP_SAMPLE})

            # Если нужно собрать семплы для работы с укрытой памятью
            if (${CMAKE_BUILD_KEEPER_APP_SAMPLE})
                # Выполняем сборку приложения укрытой памяти
                add_custom_command(TARGET "${SOURCE_APP_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${SOURCE_APP_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${SOURCE_APP_NAME}\" to enable core dump on macOS"
                )
                add_custom_command(TARGET "${TUNING_APP_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${TUNING_APP_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${TUNING_APP_NAME}\" to enable core dump on macOS"
                )
                add_custom_command(TARGET "${KEEPER_APP_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${KEEPER_APP_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${KEEPER_APP_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_KEEPER_APP_SAMPLE})

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
            if (${CMAKE_BUILD_BINBOX_CONTAINER_APP_SAMPLE})
                # Выполняем сборку приложения работы с бинарным контейнером
                add_custom_command(TARGET "${BINBOX_APP_CONTAINER_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${BINBOX_APP_CONTAINER_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${BINBOX_APP_CONTAINER_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_BINBOX_CONTAINER_APP_SAMPLE})

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
            if (${CMAKE_BUILD_CRYPTO_CRYPTOGRAPHY_APP_SAMPLE})
                # Выполняем сборку приложения шифрования и хэширования
                add_custom_command(TARGET "${CRYPTO_APP_CRYPTOGRAPHY_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${CRYPTO_APP_CRYPTOGRAPHY_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${CRYPTO_APP_CRYPTOGRAPHY_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_CRYPTO_CRYPTOGRAPHY_APP_SAMPLE})

            # Если нужно собрать семплы для работы с хэшированием данных
            if (${CMAKE_BUILD_HASH_CRYPTOGRAPHY_APP_SAMPLE})
                # Выполняем сборку приложения хэширования данных
                add_custom_command(TARGET "${HASH_APP_CRYPTOGRAPHY_NAME}" POST_BUILD
                    COMMAND bash "${CMAKE_SOURCE_DIR}/sh/core_dump.sh" "$<TARGET_FILE:${HASH_APP_CRYPTOGRAPHY_NAME}>" "${entitlements_file}"
                    COMMENT "Sign target \"${HASH_APP_CRYPTOGRAPHY_NAME}\" to enable core dump on macOS"
                    VERBATIM
                )
            endif (${CMAKE_BUILD_HASH_CRYPTOGRAPHY_APP_SAMPLE})

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
                # Если сборка производится под систему с поддержкой SCTP: FreeBSD, Linux, Solaris или illumos
                if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
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
                # Если сборка производится под систему с поддержкой SCTP: FreeBSD, Linux, Solaris или illumos
                if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
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

            # Если нужно собрать семплы для работы с распределителем памяти
            if (${CMAKE_BUILD_ALLOC_APP_SAMPLE})
                # Выполняем анализ приложения распределителя памяти
                pvs_studio_add_target(TARGET ${ALLOC_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${ALLOC_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_ALLOC_APP_SAMPLE})

            # Если нужно собрать семплы поиска дефектов памяти прикладного кода
            if (${CMAKE_BUILD_DEBUG_APP_SAMPLE})
                # Выполняем анализ приложения поиска дефектов памяти
                pvs_studio_add_target(TARGET ${DEBUG_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${DEBUG_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_DEBUG_APP_SAMPLE})

            # Если нужно собрать семплы для работы с укрытой памятью
            if (${CMAKE_BUILD_KEEPER_APP_SAMPLE})
                # Выполняем анализ приложения укрытой памяти
                pvs_studio_add_target(TARGET ${SOURCE_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${SOURCE_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                pvs_studio_add_target(TARGET ${TUNING_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${TUNING_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
                pvs_studio_add_target(TARGET ${KEEPER_APP_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${KEEPER_APP_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_KEEPER_APP_SAMPLE})

            # Если нужно собрать семплы для работы с датой и временем
            if (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})
                # Выполняем анализ приложения даты и времени
                pvs_studio_add_target(TARGET ${DATE_APP_SYS_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${DATE_APP_SYS_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})

            # Если нужно собрать семплы для работы с бинарным контейнером
            if (${CMAKE_BUILD_BINBOX_CONTAINER_APP_SAMPLE})
                # Выполняем анализ приложения работы с бинарным контейнером
                pvs_studio_add_target(TARGET ${BINBOX_APP_CONTAINER_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${BINBOX_APP_CONTAINER_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_BINBOX_CONTAINER_APP_SAMPLE})

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
            if (${CMAKE_BUILD_CRYPTO_CRYPTOGRAPHY_APP_SAMPLE})
                # Выполняем анализ приложения даты и времени
                pvs_studio_add_target(TARGET ${CRYPTO_APP_CRYPTOGRAPHY_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${CRYPTO_APP_CRYPTOGRAPHY_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_CRYPTO_CRYPTOGRAPHY_APP_SAMPLE})

            # Если нужно собрать семплы для работы с хэшированием данных
            if (${CMAKE_BUILD_HASH_CRYPTOGRAPHY_APP_SAMPLE})
                # Выполняем анализ приложения хэширования данных
                pvs_studio_add_target(TARGET ${HASH_APP_CRYPTOGRAPHY_NAME}.analyze ALL OUTPUT FORMAT json ANALYZE ${HASH_APP_CRYPTOGRAPHY_NAME} MODE GA:1,2 LOG target.err ARGS -e ${CMAKE_CURRENT_BINARY_DIR})
            endif (${CMAKE_BUILD_HASH_CRYPTOGRAPHY_APP_SAMPLE})

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

            # Если сборка производится под систему с поддержкой SCTP: FreeBSD, Linux, Solaris или illumos
            if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
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

    ##
     # Каталог пакетов систем BSD добавляется к путям поиска
     #
     # Набор GTest ставится туда, куда его кладёт хранилище пакетов системы: у NetBSD и
     # OpenBSD это /usr/pkg, у FreeBSD - /usr/local. Ни того, ни другого нет среди путей,
     # где find_package() ищет по умолчанию, и поиск отвечал отказом
     # "Could NOT find GTest (missing: GTEST_LIBRARY GTEST_INCLUDE_DIR GTEST_MAIN_LIBRARY)"
     #
     # Отказ этот сборку не прерывал: наборы просто не заводились, а сборка отвечала
     # успехом - и молчание это выглядело как «наборы прошли», хотя их не было вовсе.
     # Проверено опытом на стенде NetBSD 11.0 aarch64
     ##
    if (${CMAKE_SYSTEM_NAME} STREQUAL "NetBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
        # Добавляем каталог пакетов pkgsrc к путям поиска
        list(APPEND CMAKE_PREFIX_PATH "/usr/pkg")
    elseif (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "DragonFly")
        # Добавляем каталог пакетов портов к путям поиска
        list(APPEND CMAKE_PREFIX_PATH "/usr/local")
    endif()

    ##
     # Отсутствие GTest объявляется отказом, а не пропускается молча
     #
     # Наборы просили собрать явно ключом CMAKE_BUILD_TESTS, и молчаливый их пропуск
     # означал бы, что просьба обслужена, - тогда как ни одной проверки не собрано
     ##
    find_package(GTest)

    # Если набор GTest не найден
    if (NOT GTest_FOUND AND NOT GTEST_FOUND)
        message(FATAL_ERROR "GTest not found: unit tests were requested but cannot be built. Install it (pkgin install googletest, pkg install googletest, apt install libgtest-dev libgmock-dev, dnf install gtest-devel gmock-devel) or configure without CMAKE_BUILD_TESTS")
    endif()

    # Сборка тестов
    add_subdirectory(tests)

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

# Если необходимо собрать бенчмарки производительности
if (CMAKE_BUILD_BENCHMARKS)
    ##
     # Бенчмарки собираются отдельно от тестов и линкуются с обычной библиотекой:
     # сборка тестов инструментирована для сбора статистики покрытия кода, а
     # инструментирование искажает измерения производительности до бессмысленности
     ##
    add_subdirectory(benchmark)
endif (CMAKE_BUILD_BENCHMARKS)

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
        # Если сборка производится под систему с поддержкой SCTP: FreeBSD, Linux, Solaris или illumos
        if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
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
        # Если сборка производится под систему с поддержкой SCTP: FreeBSD, Linux, Solaris или illumos
        if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
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

    # Если нужно собрать семплы для работы с распределителем памяти
    if (${CMAKE_BUILD_ALLOC_APP_SAMPLE})
        # Устанавливаем приложение работы с распределителем памяти
        install(TARGETS ${ALLOC_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_ALLOC_APP_SAMPLE})

    # Если нужно собрать семплы поиска дефектов памяти прикладного кода
    if (${CMAKE_BUILD_DEBUG_APP_SAMPLE})
        # Устанавливаем приложение поиска дефектов памяти прикладного кода
        install(TARGETS ${DEBUG_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_DEBUG_APP_SAMPLE})

    # Если нужно собрать семплы для работы с укрытой памятью
    if (${CMAKE_BUILD_KEEPER_APP_SAMPLE})
        # Устанавливаем приложение работы с укрытой памятью
        install(TARGETS ${SOURCE_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        install(TARGETS ${TUNING_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
        install(TARGETS ${KEEPER_APP_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_KEEPER_APP_SAMPLE})

    # Если нужно собрать семплы для работы с датой и временем
    if (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})
        # Устанавливаем приложение работы с датой и временем
        install(TARGETS ${DATE_APP_SYS_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_DATE_SYS_APP_SAMPLE})

    # Если нужно собрать семплы для работы с бинарным контейнером
    if (${CMAKE_BUILD_BINBOX_CONTAINER_APP_SAMPLE})
        # Устанавливаем приложение работы с бинарным контейнером
        install(TARGETS ${BINBOX_APP_CONTAINER_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_BINBOX_CONTAINER_APP_SAMPLE})

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
    if (${CMAKE_BUILD_CRYPTO_CRYPTOGRAPHY_APP_SAMPLE})
        # Устанавливаем приложение работы с шифрованием и хэшированием
        install(TARGETS ${CRYPTO_APP_CRYPTOGRAPHY_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_CRYPTO_CRYPTOGRAPHY_APP_SAMPLE})

    # Если нужно собрать семплы для работы с хэшированием данных
    if (${CMAKE_BUILD_HASH_CRYPTOGRAPHY_APP_SAMPLE})
        # Устанавливаем приложение работы с хэшированием данных
        install(TARGETS ${HASH_APP_CRYPTOGRAPHY_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_PROJECT_NAME}")
    endif (${CMAKE_BUILD_HASH_CRYPTOGRAPHY_APP_SAMPLE})

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

    # Если сборка производится под систему с поддержкой SCTP: FreeBSD, Linux, Solaris или illumos
    if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
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
