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
    message(STATUS "Build AWH IDN: YES\n")
# Сборка модуля AWH_IDN не требуется
else (CMAKE_BUILD_IDN)
    message(STATUS "Build AWH IDN: NO\n")
endif (CMAKE_BUILD_IDN)

# Сборка установкой количества доступных файловых дескрипторов
if (CMAKE_MAX_COUNT_FDS)
    add_compile_definitions(AWH_MAX_COUNT_FDS=${CMAKE_MAX_COUNT_FDS})
    message(STATUS "Build AWH MAX FDS: ${CMAKE_MAX_COUNT_FDS}\n")
endif (CMAKE_MAX_COUNT_FDS)

# Если активирован флаг оптимизации операционной системы
if (CMAKE_BOOSTING_NET)
    add_compile_definitions(AWH_BOOSTING_NET)
    message(STATUS "Build AWH BOOSTING NET: YES\n")
endif (CMAKE_BOOSTING_NET)

# Если активирован флаг установки максимального размера HTTP-тела запроса/ответа
if (CMAKE_MAX_HTTP_BODY_SIZE)
    add_compile_definitions(AWH_MAX_BODY_SIZE=${CMAKE_MAX_HTTP_BODY_SIZE})
    message(STATUS "Build AWH MAX BODY SIZE: ${CMAKE_MAX_HTTP_BODY_SIZE}\n")
endif (CMAKE_MAX_HTTP_BODY_SIZE)

# Если активирован флаг установки максимального размера очереди
if (CMAKE_MAX_RECORDS_QUEUE)
    add_compile_definitions(AWH_MAX_RECORDS_QUEUE=${CMAKE_MAX_RECORDS_QUEUE})
    message(STATUS "Build AWH MAX RECORDS QUEUE: ${CMAKE_MAX_RECORDS_QUEUE}\n")
endif (CMAKE_MAX_RECORDS_QUEUE)

# Если активирован флаг установки максимального размера потребления памяти очередью
if (CMAKE_MAX_MEMORY_QUEUE)
    add_compile_definitions(AWH_MAX_MEMORY_QUEUE=${CMAKE_MAX_MEMORY_QUEUE})
    message(STATUS "Build AWH MAX MEMORY QUEUE: ${CMAKE_MAX_MEMORY_QUEUE}\n")
endif (CMAKE_MAX_MEMORY_QUEUE)

# Если активирован флаг установки максимального размера потребления памяти буфера
if (CMAKE_MAX_MEMORY_BUFFER)
    add_compile_definitions(AWH_MAX_MEMORY_BUFFER=${CMAKE_MAX_MEMORY_BUFFER})
    message(STATUS "Build AWH MAX MEMORY BUFFER: ${CMAKE_MAX_MEMORY_BUFFER}\n")
endif (CMAKE_MAX_MEMORY_BUFFER)

# Выводим информацию каталога где выполняется сброка
message(STATUS "Build dir: ${CMAKE_CURRENT_BINARY_DIR}\n")

# Получаем архитектуру
EXECUTE_PROCESS(COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE )

##
 # Запасной путь опознания набора команд: оснастка MSVC исполняется вне оболочки MSYS2,
 # и `uname` там нет вовсе - обращение выше молча отдаёт пустоту. Пустая строка валит
 # условие выбора флагов ниже отказом «Unknown arguments specified», причём указывает он
 # на строку условия, а не на место, где значение потерялось
 ##
if (ARCHITECTURE STREQUAL "")
    # Набор команд берётся у самого CMake, раз внешнего средства нет
    SET(ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
    # Приводим написание ARM64 к общему для всего файла виду
    if (ARCHITECTURE MATCHES "^[Aa][Rr][Mm]64$")
        # Устанавливаем поддерживаемую архитектуру
        SET(ARCHITECTURE "aarch64")
    endif()
endif()

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
    # Различаем линковщик по факту, а не по названию системы: Solaris 11.4 принимает
    # --no-as-needed, а линковщик illumos (OpenIndiana Hipster 2026.04) отказывает
    # "ld: fatal: unrecognized option '--no-as-needed'" на сборке сэмплов ip, ntp, ping и dns.
    # Снято прогоном обеих ног 08.11.2026. Без --no-as-needed на illumos можно: его линковщик
    # не выбрасывает библиотеки по умолчанию, поэтому -ldl остаётся в зависимостях и так.
    # Меряем СБИРАЕМОСТЬ С ПОСЛЕДУЮЩЕЙ ЛИНКОВКОЙ: check_cxx_compiler_flag на Hipster ответил
    # "Success" при отвергнутом линкером флаге (он не линкует), поэтому он как различитель
    # не годится. Имя переменной новое сознательно: прежнее значение уже легло в кэш сборки.
    INCLUDE(CheckCXXSourceCompiles)
    SET(CMAKE_REQUIRED_LINK_OPTIONS "-Wl,--no-as-needed")
    CHECK_CXX_SOURCE_COMPILES("int main(void){return 0;}" AWH_LINKER_NO_AS_NEEDED)
    UNSET(CMAKE_REQUIRED_LINK_OPTIONS)
     # Используем Clang
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
     # Используем GCC
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        if (AWH_LINKER_NO_AS_NEEDED)
            SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--no-as-needed -ldl")
        else()
            SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ldl")
        endif()
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
    ##
     # Отладочные сведения оснастки MSVC задаются её собственным набором (/Od /Zi)
     # в блоке оснастки ниже: ключ -ggdb3 ей неведом, а неведомый ключ она принимает
     # за ИМЯ ФАЙЛА - отказ приходит тогда о ненайденном файле, а не о ключе
     ##
    if (NOT MSVC)
        SET(DEBUGGER "-ggdb3")
    endif()
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

##
 # Код библиотеки обязан быть перемещаемым: он ложится и в динамическую библиотеку.
 # Оснастке MSVC ключ -fPIC неведом, а неведомый ключ она принимает за ИМЯ ФАЙЛА,
 # поэтому признак этот ставит только оснастка GCC/Clang
 ##
if (NOT MSVC)
    # Опции компилятора
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

# Устанавливаем параметры игнорирования ошибок для MacOS X
if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    SET(AWH_PARAMS "-framework Carbon -Wno-cpp -Wno-deprecated -Wno-unused-command-line-argument")
##
 # Устанавливаем параметры игнорирования ошибок для Solaris
 # libc не указывается явно: собиратель подключает её сам после libgcc_s, а явная -lc ставила её
 # раньше, и _Unwind_Resume связывался с раскруткой libc - исключение с очисткой роняло процесс
 ##
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
    SET(AWH_PARAMS "-lsocket -lnsl -Wno-cpp -Wno-deprecated")
# Устанавливаем параметры игнорирования ошибок для Windows и Linux
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Windows" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    ##
     # Снятия предупреждений GCC оснастке MSVC неведомы, а неведомый ключ она
     # принимает за имя файла. Разряд предупреждений у неё свой (/W3) и задаётся
     # в блоке оснастки ниже, снимать же там нечего
     ##
    if (MSVC)
        SET(AWH_PARAMS "")
    else()
        SET(AWH_PARAMS "-Wno-cpp -Wno-deprecated")
    endif()
# Устанавливаем параметры игнорирования ошибок для FreeBSD, NetBSD и OpenBSD
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "NetBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
    SET(AWH_PARAMS "-lutil -Wno-cpp -Wno-deprecated -Wno-unused-command-line-argument")
# Если операционная система не определена
else()
    SET(AWH_PARAMS "-Wno-unknown-attributes")
endif()

##
 # Флаги оснастки MSVC заводятся порознь: написание у неё своё («/O2» вместо «-O3»), и
 # флаг, ей неведомый, она не отвергает, а принимает за ИМЯ ФАЙЛА - сборка отвечает
 # тогда отказом о ненайденном файле, уводя разбор далеко от настоящей причины
 ##
if (MSVC)
    # Уровень оптимизации выпускной сборки
    SET(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
    # Уровень отладочной сборки
    SET(CMAKE_CXX_FLAGS_DEBUG "/Od /DDEBUG_MODE /Zi")
    ##
     # Общие флаги
     #
     # /EHsc - обычная для C++ работа исключений; /utf-8 - исходники и строки в UTF-8,
     # без него MSVC читает их кодовой страницей системы и рвёт кириллицу в сообщениях;
     # /W3 - уровень предупреждений; /bigobj - в объектном файле AWH разделов больше,
     # чем допускает обычный предел формата
     ##
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /utf-8 /W3 /bigobj")
    ##
     # Зависимость сжатия LZMA объявляет свои имена берущимися из динамической
     # библиотеки, пока не сказано обратное
     #
     # Собирается она у нас неразделяемой, и без этого признака заголовок её помечает
     # каждое имя указанием ВЗЯТЬ его у файла рядом. Перевод отказа не даёт, а
     # связывание отвечает о неразрешённых именах вида __imp_lzma_code - о тех самых,
     # какие в архиве ЕСТЬ. Указание это ставит только оснастка MSVC, оттого и признак
     # задаётся здесь
     ##
    add_compile_definitions(LZMA_API_STATIC)
    ##
     # Снимаем разряд предупреждений об «опасных» приёмах языка C
     #
     # Оснастка MSVC объявляет устаревшими приёмы САМОГО ЯЗЫКА - getenv, sscanf,
     # strerror, - и советует взамен свои, к иным оснасткам не относящиеся. Совет этот
     # не о дефекте: приёмы эти стоят в стандарте языка, и пользуемся мы ими по
     # договору - с проверкой отказа и с заведомо ограниченной длиной.
     #
     # Заменить их на местные значило бы завести у переносимой библиотеки ветвь под
     # одну оснастку в нескольких модулях разом. Взамен снят весь разряд, и снят
     # ЕДИНОЖДЫ, чтобы это было видно: правда о том, что такие приёмы здесь есть,
     # записана в одном месте, а не растворена по коду отдельными снятиями
     ##
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    ##
     # Устаревший слой сокетов windows.h тянет за собой сам, и его надо отсечь
     #
     # Заголовок этот подключает winsock.h, если не сказано обратное, и объявления
     # сокетов приходят тогда дважды - сначала устаревшие, потом нынешние из winsock2.h.
     # Оснастка отвечает переопределением типов sockaddr и fd_set.
     #
     # Ставить признак в заголовке AWH ПОЗДНО: zlib (zconf.h) и bzip2 (bzlib.h) подключают
     # windows.h сами, а в sys/hash.hpp они стоят до fmk.hpp, то есть до всякого нашего
     # заголовка. Оттого признак задаётся на строке вызова компилятора - единственном
     # месте, которое раньше всех включов.
     #
     # Отсекается при этом и прочее, что windows.h несёт без нужды: wincrypt.h, ole2.h,
     # shellapi.h и другие. Приёмы их в AWH не зовутся, а единственный потребитель COM -
     # работа со ссылками в sys/fs.cpp - получает objbase.h и shlobj.h прямо, из sys/fs.hpp.
     # Признак NOGDI при этом НЕ ставится: GDI нужен sys/fmk.cpp, там стоит
     # CONSOLE_FONT_INFOEX и зовётся SetCurrentConsoleFontEx
     ##
    add_compile_definitions(WIN32_LEAN_AND_MEAN)
    ##
     # Макросы min и max из windows.h подменяют вызовы ::min и ::max
     #
     # Имена эти объявлены в стандартной библиотеке, и AWH зовёт их через ::, чтобы не
     # спорить с одноимёнными членами своих классов. windows.h же заводит их макросами,
     # и препроцессор переписывает вызов до разбора: оснастка отвечает о недопустимой
     # лексеме справа от «::» в sys/fmk.cpp и sys/chrono.cpp
     ##
    add_compile_definitions(NOMINMAX)
    ##
     # Подменяемый распределитель памяти вступает в дело лишь тогда, когда его символ
     # принудительно втянут в образ
     #
     # Сам по себе он никем не зовётся: gperftools перехватывает вызовы Windows правкой
     # их начал, и делает это при запуске образа. Не втяни связыватель модуль с этим
     # символом - архив распределителя был бы выброшен целиком, и подмены бы не было.
     # Ключ этот gperftools задаёт свойством ЦЕЛИ своей сборки, а мы подключаем его
     # архивы путём к файлу - свойство до нас не доходит, оттого ключ ставим сами.
     #
     # Пишется он в параметры связывания, а не в перечень библиотек: флагом CMake
     # признаёт лишь имя с «-» в начале, а «/INCLUDE:» принял бы за имя библиотеки
     #
     # В динамической конфигурации ключ ставится ТОЛЬКО самой библиотеке: распределитель
     # живёт внутри awh.dll, и второй его копии в образе программы быть не должно.
     # Правку начал функций Windows каждый экземпляр делает при своём запуске, и второй
     # заход по тем же адресам кончается отказом gperftools ещё до main - программа
     # падает с кодом 0xC0000409, не напечатав ничего
     ##
    if(NOT CMAKE_SHARED_BUILD_LIB)
        add_link_options(/INCLUDE:__tcmalloc)
    endif()
# Если процессор архитектуры ARM
elseif (${ARCHITECTURE} MATCHES "aarch64" OR ${ARCHITECTURE} MATCHES "arm*")
    SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -pipe -DNDEBUG")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fno-permissive -Wno-pedantic -Wno-narrowing -Wno-deprecated-declarations -Wno-exceptions -Wno-unused-result ${AWH_PARAMS}")
# Если процессор архитектуры x86_64
else()
    SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -pipe -mrdrnd -march=core2 -DNDEBUG")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fno-permissive -Wno-pedantic -Wno-narrowing -Wno-deprecated-declarations -Wno-exceptions -Wno-unused-result ${AWH_PARAMS}")
endif()

##
 # Оснастке MSVC отладочный набор задан выше её собственным написанием, и общее
 # назначение ниже перезаписало бы его ключами GCC
 ##
if (NOT MSVC)
    # Устанавливаем параметр отладки
    SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -DDEBUG_MODE ${DEBUGGER}")
endif()

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
    message(STATUS "A-NTS include dir found: " ${NTS_INCLUDE_DIR})
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

##
 # Выполняем поиск библиотеки TcMalloc
 # Поиск оставляем необязательным: на OpenBSD аллокатор не собирается (см. build_third_party.sh),
 # и отказывать всей конфигурации из-за этого проект права не имеет. Для остальных систем
 # проверку делает ветка ниже
 ##
find_package(TcMalloc)

# Провекра TcMalloc
if (TcMalloc_FOUND)
    message(STATUS "TcMalloc lib found: " ${TCMALLOC_LIBRARY} " " ${TCMALLOC_COMMON_LIBRARY})
# Если операционной системой является OpenBSD
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
    # Удаляем библиотеку TCMalloc: распределением памяти заведёт системный malloc
    SET(TCMALLOC_LIBRARIES "")
else (TcMalloc_FOUND)
    message(FATAL_ERROR "TcMalloc not found")
endif (TcMalloc_FOUND)

# Выполняем подключение хидеров
include_directories(BEFORE ${DEPEND_INCLUDE_DIRS})
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/include)
include_directories(BEFORE ${CMAKE_SOURCE_DIR}/contrib/include)
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
    "${CMAKE_SOURCE_DIR}/src/events/fds.cpp"
    "${CMAKE_SOURCE_DIR}/src/events/base.cpp"
    "${CMAKE_SOURCE_DIR}/src/events/event.cpp"
    "${CMAKE_SOURCE_DIR}/src/events/watch.cpp"
    "${CMAKE_SOURCE_DIR}/src/events/notifier.cpp"
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

##
 # Для оснастки MSVC добавляем свой обход каталогов: заголовка dirent.h она не несёт
 ##
if (MSVC)
    LIST(APPEND SOURCE_FILES "${CMAKE_SOURCE_DIR}/src/sys/dirent.cpp")
endif()

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
    # Выполняем экспорт методов доступных извне в динамических библиотеках
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
    ##
     # Заголовки A-NTS по умолчанию объявляют методы как __declspec(dllimport), и тогда
     # линковка статичного libnts ищет символы с приставкой __imp_ и отказывает. Сборка
     # зависимостей делает A-NTS именно статической, поэтому на Windows объявляем это библиотеке:
     # её global.hpp при ANTS_STATICLIB оставляет объявления без атрибута импорта
     ##
    add_compile_definitions(ANTS_STATICLIB)
    ##
     # Заголовки nghttp2 на Windows тоже объявляют имена берущимися из динамической
     # библиотеки, пока не сказано обратное. Признак этот nghttp2 ставит СВОЕЙ цели
     # сборки, а мы подключаем его архив путём к файлу - свойство цели до нас не
     # доходит, и связывание отвечает о неразрешённых именах вида __imp_nghttp2_*
     ##
    add_compile_definitions(NGHTTP2_STATICLIB)
    ##
     # export MSYS="error_start:%WD%dumper.exe -d -c %1 %2"
     # Устанавливаем необходимые зависимости для MS Windows. Тройка ole32/shell32/uuid нужна
     # src/sys/fs.cpp: разрешение ярлыков идёт через COM, где CoInitialize и CoCreateInstance
     # живут в ole32, IID_IShellLinkW — в shell32, а IID_IPersistFile с CLSID_ShellLink — в uuid.
     # Без них линковка обрывается на нераспознанных __imp_Co* и GUID-символах
     ##
    if (MSVC)
        ##
         # Перечень системных библиотек оснастка MSVC требует полными именами файлов:
         # неведомый ей ключ она не отвергает, а принимает за ИМЯ ФАЙЛА, и «-lws2_32»
         # искала бы на диске. Стандартная библиотека C++ здесь не называется вовсе:
         # MSVC подключает её сама по языку файла, а названная вручную она даёт
         # двойные определения своих символов. Ускорение связывания (-flto) тоже
         # принадлежит лишь GCC/Clang
         ##
        SET(WINFLAGS ws2_32.lib gdi32.lib crypt32.lib Iphlpapi.lib bcrypt.lib ole32.lib shell32.lib uuid.lib)
    else()
        SET(WINFLAGS -lstdc++ -lws2_32 -lgdi32 -lcrypt32 -lIphlpapi -lbcrypt -lole32 -lshell32 -luuid -flto=auto)
    endif()

    # Устанавливаем иконку для windows
    SET(RES_FILES ${CMAKE_PROJECT_NAME}.rc)
    ##
     # Описание значка читает оснастка Windows, и письмо вызова у неё своё: windres
     # просит «-O coff -i файл -o объект», а rc.exe этих ключей не знает вовсе и
     # принял бы их за имена файлов. Подменять вызов поэтому можно лишь там, где
     # читает windres; MSVC вызывает rc.exe сама
     ##
    if (NOT MSVC)
        SET(CMAKE_RC_COMPILER_INIT windres)
        ENABLE_LANGUAGE(RC)
        SET(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
    else()
        ENABLE_LANGUAGE(RC)
    endif()

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
# Если операционной системой является Linux, FreeBSD, NetBSD, OpenBSD, MacOS X, Solaris
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

        # Втягиваем распределитель памяти в саму библиотеку: программы ниже получают
        # его уже готовым внутри awh.dll и своего не несут
        target_link_options(${AWH_LIB_NAME} PRIVATE /INCLUDE:__tcmalloc)

        # Выполняем сборку приложения работы с IP-адресами
        target_link_libraries(
            ${IP_APP_NAME}
            ${AWH_LIB_NAME}
            ${WINFLAGS}
        )

        # Выполняем сборку приложения WebSocket-клиент
        target_link_libraries(
            ${WS_CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${WINFLAGS}
        )

        # Выполняем сборку приложения WebSocket-сервер
        target_link_libraries(
            ${WS_SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${WINFLAGS}
        )

        # Выполняем сборку приложения DNS-резолвер
        target_link_libraries(
            ${DNS_APP_NAME}
            ${AWH_LIB_NAME}
            ${WINFLAGS}
        )

        # Выполняем сборку приложения NTP-клиента
        target_link_libraries(
            ${NTP_APP_NAME}
            ${AWH_LIB_NAME}
            ${WINFLAGS}
        )

        # Выполняем сборку приложения Proxy-сервера
        target_link_libraries(
            ${PROXY_APP_NAME}
            ${AWH_LIB_NAME}
            ${WINFLAGS}
        )

        # Выполняем сборку приложения Socks5-сервера
        target_link_libraries(
            ${SOCKS5_APP_NAME}
            ${AWH_LIB_NAME}
            ${WINFLAGS}
        )

        # Выполняем сборку приложения Ping-клиента
        target_link_libraries(
            ${PING_APP_NAME}
            ${AWH_LIB_NAME}
            ${WINFLAGS}
        )

        # Выполняем сборку приложения даты и времени
        target_link_libraries(
            ${DATE_APP_NAME}
            ${AWH_LIB_NAME}
            ${WINFLAGS}
        )

        # Выполняем сборку приложения таймера
        target_link_libraries(
            ${TIMER_APP_NAME}
            ${AWH_LIB_NAME}
            ${WINFLAGS}
        )

        # Выполняем сборку приложения клиента
        target_link_libraries(
            ${CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${WINFLAGS}
        )

        # Выполняем сборку приложения сервера
        target_link_libraries(
            ${SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${WINFLAGS}
        )

        # Выполняем сборку приложения HTTP-клиента
        target_link_libraries(
            ${WEB_CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${WINFLAGS}
        )

        # Выполняем сборку приложения HTTP-сервера
        target_link_libraries(
            ${WEB_SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${WINFLAGS}
        )

        # Выполняем сборку приложения дознавателя
        target_link_libraries(
            ${INVESTIGATOR_APP_NAME}
            ${AWH_LIB_NAME}
            ${WINFLAGS}
        )

    # Если операционной системой является Linux, FreeBSD, NetBSD, OpenBSD, MacOS X, Solaris
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
        )

        # Выполняем сборку приложения WebSocket-клиент
        target_link_libraries(
            ${WS_CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
        )

        # Выполняем сборку приложения WebSocket-сервер
        target_link_libraries(
            ${WS_SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
        )

        # Выполняем сборку приложения DNS-резолвер
        target_link_libraries(
            ${DNS_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
        )

        # Выполняем сборку приложения NTP-клиента
        target_link_libraries(
            ${NTP_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
        )

        # Выполняем сборку приложения Proxy-сервера
        target_link_libraries(
            ${PROXY_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
        )

        # Выполняем сборку приложения Socks5-сервера
        target_link_libraries(
            ${SOCKS5_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
        )

        # Выполняем сборку приложения Ping-клиента
        target_link_libraries(
            ${PING_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
        )

        # Выполняем сборку приложения даты и времени
        target_link_libraries(
            ${DATE_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
        )

        # Выполняем сборку приложения таймера
        target_link_libraries(
            ${TIMER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
        )

        # Выполняем сборку приложения клиента
        target_link_libraries(
            ${CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
        )

        # Выполняем сборку приложения сервера
        target_link_libraries(
            ${SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
        )

        # Выполняем сборку приложения HTTP-клиента
        target_link_libraries(
            ${WEB_CLIENT_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
        )

        # Выполняем сборку приложения HTTP-сервера
        target_link_libraries(
            ${WEB_SERVER_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
        )

        # Выполняем сборку приложения дознавателя
        target_link_libraries(
            ${INVESTIGATOR_APP_NAME}
            ${AWH_LIB_NAME}
            ${DEPEND_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${WINFLAGS}
        )

    # Если операционной системой является Linux, FreeBSD, NetBSD, OpenBSD, MacOS X, Solaris
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

##
 # Устанавливаем адрес установки, запрет установки в /usr/local
 #
 # Адреса назначения ниже заданы относительными: CMake отсчитывает их от
 # CMAKE_INSTALL_PREFIX, и потому их переносит cmake --install --prefix. Абсолютный
 # адрес этот ключ молча игнорирует - установка ушла бы в место, названное при
 # настройке, а не в место, названное при установке
 ##
if ("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr/local" OR "${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr/local/")
    SET(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}")
endif()

# Устанавливаем сертификаты
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/certs/" DESTINATION "certs")

# Устанавливаем хидеры
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/" DESTINATION "include/${CMAKE_PROJECT_NAME}" FILES_MATCHING PATTERN "*.hpp")
# Устанавливаем хидеры сторонних библиотек
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/contrib/include/" DESTINATION "include" FILES_MATCHING PATTERN "*.h" PATTERN "*.hh" PATTERN "*.hpp")

# Устанавливаем библиотеку
if (NOT CMAKE_SHARED_BUILD_LIB)
    # Если режим отладки не активирован
    if (NOT CMAKE_BUILD_TYPE MATCHES Debug)
        ##
         # Сборщик берётся по оснастке: под MSVC оболочки bash на машине может не быть
         # вовсе, а слияние там делает lib.exe - он принимает архивы прямо, без
         # разбора на модули, и потому обходится без GNU ar и ranlib
         ##
        if (MSVC)
            # Выполняем запуск скрипта объединения библиотек
            add_custom_command(TARGET "${AWH_LIB_NAME}" POST_BUILD
                COMMAND "${CMAKE_SOURCE_DIR}/bat/build_static_lib.bat" "$<TARGET_FILE:${AWH_LIB_NAME}>"
            VERBATIM)
        else()
            add_custom_command(TARGET "${AWH_LIB_NAME}" POST_BUILD
                COMMAND bash "${CMAKE_SOURCE_DIR}/build_static_lib.sh" "$<TARGET_FILE:${AWH_LIB_NAME}>"
            VERBATIM)
        endif()
    # Если режим отладки активирован
    else()
        # Выполняем установку библиотеки зависимостей
        install(FILES ${DEPEND_LIBRARY} DESTINATION "lib")
    endif()
endif()

# Если операционной системой является Windows
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    # Если собрана динамическая библиотека
    if (CMAKE_SHARED_BUILD_LIB)
        ##
         # Имя файла задаёт оснастка, а не сборщик: переименование RENAME относится
         # к установке FILES и PROGRAMS, а в установке целей CMake его молча
         # пропускает - замерено на 4.3.2 и 4.4.2, отказом это не становится.
         # MSVC кладёт lib/awh.lib и bin/awh/awh.dll, GNU-оснастка -
         # lib/libawh.dll.a и bin/awh/libawh.dll; модуль поиска ищет оба набора
         ##
        # Устанавливаем библиотеку AWH
        install(TARGETS ${AWH_LIB_NAME}
            # Библиотека импорта - для линковки
            ARCHIVE DESTINATION lib
            # Исполняемый модуль - для запуска
            RUNTIME DESTINATION bin/${CMAKE_PROJECT_NAME}
        )
    # Если собрана статическая библиотека
    else (CMAKE_SHARED_BUILD_LIB)
        # Устанавливаем статическую библиотеку AWH: MSVC - lib/awh.lib, GNU-оснастка - lib/libawh.a
        install(TARGETS ${AWH_LIB_NAME} ARCHIVE DESTINATION lib)
    endif (CMAKE_SHARED_BUILD_LIB)
# Если операцинная система относится к Nix-подобной
else()
    # Устанавливаем статическую библиотеку AWH
    install(TARGETS ${AWH_LIB_NAME} DESTINATION "lib")
endif()

# Устанавливаем статическую библиотеку TcMalloc
install(FILES ${TCMALLOC_LIBRARIES} DESTINATION "lib")

# Устанавливаем исполнительные файлы
install(TARGETS ${IP_APP_NAME} DESTINATION "bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${DNS_APP_NAME} DESTINATION "bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${NTP_APP_NAME} DESTINATION "bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${PING_APP_NAME} DESTINATION "bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${DATE_APP_NAME} DESTINATION "bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${TIMER_APP_NAME} DESTINATION "bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${PROXY_APP_NAME} DESTINATION "bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${SOCKS5_APP_NAME} DESTINATION "bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${CLIENT_APP_NAME} DESTINATION "bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${SERVER_APP_NAME} DESTINATION "bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${WS_CLIENT_APP_NAME} DESTINATION "bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${WS_SERVER_APP_NAME} DESTINATION "bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${WEB_CLIENT_APP_NAME} DESTINATION "bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${WEB_SERVER_APP_NAME} DESTINATION "bin/${CMAKE_PROJECT_NAME}")
install(TARGETS ${INVESTIGATOR_APP_NAME} DESTINATION "bin/${CMAKE_PROJECT_NAME}")

# Если операционной системой не является Windows
if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    install(TARGETS ${CLUSTER_APP_NAME} DESTINATION "bin/${CMAKE_PROJECT_NAME}")
endif()

##
 # Как и сборщик выше: под MSVC правку файла поиска делает bat-сборщик, под остальными
 # оснастками - прежний скрипт оболочки
 ##
if (MSVC)
    # Выполняем запуск скрипта генерации CMAKE-файла
    add_custom_command(TARGET "${AWH_LIB_NAME}" POST_BUILD
        COMMAND "${CMAKE_SOURCE_DIR}/bat/build_cmake.bat" "${CMAKE_INSTALL_PREFIX}"
    VERBATIM)
else()
    # Выполняем запуск скрипта генерации CMAKE-файла
    add_custom_command(TARGET "${AWH_LIB_NAME}" POST_BUILD
        COMMAND bash "${CMAKE_SOURCE_DIR}/build_cmake.sh" "${CMAKE_INSTALL_PREFIX}"
    VERBATIM)
endif()

# Устанавливаем сгенерированный CMAKE-файл
install(FILES "${AWH_THIRD_PARTY}/cmake/FindAWH.cmake" DESTINATION "cmake")
