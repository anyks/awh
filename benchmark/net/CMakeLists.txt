# Определяем имя для бенчмарков
SET(NAME "net")

# Переводим название бенчмарка в верхний регистр
string(TOUPPER "${NAME}" BENCHMARK_NAME)

# Указываем что нужно собрать бенчмарки
option(CMAKE_BUILD_${BENCHMARK_NAME}_BENCHMARKS "Build benchmarks for ${BENCHMARK_NAME}" ON)

# Если необходимо собрать бенчмарки
if (${CMAKE_BUILD_${BENCHMARK_NAME}_BENCHMARKS})
    # Определяем постфикс для бенчмарков
    SET(POSTFIX "_BENCHMARK_${NAME}")

    # Enable C++17
    SET(CMAKE_CXX_STANDARD 17)
    SET(CMAKE_CXX_STANDARD_REQUIRED ON)

    # Убираем ненужные и глупые варинги которые не имеют ничего общего с реальностью
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-implicitly-unsigned-literal -Wno-reserved-user-defined-literal")

    # Источники для сборки бенчмарков
    SET(${PROJECT_NAME}_BENCHMARK_SOURCES ../main.cpp)

    # Указываем что нужно собрать бенчмарки для подмодуля сетевого движка
    option(CMAKE_BUILD_${BENCHMARK_NAME}_IO_BENCHMARK "Build IO benchmark for ${BENCHMARK_NAME}" ON)

    # Если нужно собрать бенчмарки для подмодуля сетевого движка
    if (${CMAKE_BUILD_${BENCHMARK_NAME}_IO_BENCHMARK})
        # Выводим сообщение о сборке бенчмарков для подмодуля сетевого движка
        message(STATUS "Build benchmarks IO submodule: Yes\n")
        # Дополняем список источников бенчмарками для подмодуля сетевого движка
        list(APPEND ${PROJECT_NAME}_BENCHMARK_SOURCES
            io/io.cpp
            io/echo.cpp
            io/stream.cpp
            io/accept.cpp
            io/timers.cpp
            io/deadlines.cpp
            io/bandwidth.cpp
            io/idle.cpp
        )
    # Если не нужно собрать бенчмарки для подмодуля сетевого движка
    else()
        # Выводим сообщение о сборке бенчмарков для подмодуля сетевого движка
        message(STATUS "Build benchmarks IO submodule: No\n")
    endif (${CMAKE_BUILD_${BENCHMARK_NAME}_IO_BENCHMARK})

    # Указываем что нужно собрать бенчмарки для подмодуля работы с адресами
    option(CMAKE_BUILD_${BENCHMARK_NAME}_ADDR_BENCHMARK "Build ADDR benchmark for ${BENCHMARK_NAME}" ON)

    # Если нужно собрать бенчмарки для подмодуля работы с адресами
    if (${CMAKE_BUILD_${BENCHMARK_NAME}_ADDR_BENCHMARK})
        # Выводим сообщение о сборке бенчмарков для подмодуля работы с адресами
        message(STATUS "Build benchmarks ADDR submodule: Yes\n")
        # Дополняем список источников бенчмарками для подмодуля работы с адресами
        list(APPEND ${PROJECT_NAME}_BENCHMARK_SOURCES
            addr/addr.cpp
        )
    # Если не нужно собрать бенчмарки для подмодуля работы с адресами
    else()
        # Выводим сообщение о сборке бенчмарков для подмодуля работы с адресами
        message(STATUS "Build benchmarks ADDR submodule: No\n")
    endif (${CMAKE_BUILD_${BENCHMARK_NAME}_ADDR_BENCHMARK})

    # Указываем что нужно собрать бенчмарки для подмодуля работы с идентификаторами ресурсов
    option(CMAKE_BUILD_${BENCHMARK_NAME}_URI_BENCHMARK "Build URI benchmark for ${BENCHMARK_NAME}" ON)

    # Если нужно собрать бенчмарки для подмодуля работы с идентификаторами ресурсов
    if (${CMAKE_BUILD_${BENCHMARK_NAME}_URI_BENCHMARK})
        # Выводим сообщение о сборке бенчмарков для подмодуля работы с идентификаторами ресурсов
        message(STATUS "Build benchmarks URI submodule: Yes\n")
        # Дополняем список источников бенчмарками для подмодуля работы с идентификаторами ресурсов
        list(APPEND ${PROJECT_NAME}_BENCHMARK_SOURCES
            uri/uri.cpp
            uri/parse.cpp
            uri/print.cpp
            uri/etag.cpp
            uri/access.cpp
            uri/resolve.cpp
        )
    # Если не нужно собрать бенчмарки для подмодуля работы с идентификаторами ресурсов
    else()
        # Выводим сообщение о сборке бенчмарков для подмодуля работы с идентификаторами ресурсов
        message(STATUS "Build benchmarks URI submodule: No\n")
    endif (${CMAKE_BUILD_${BENCHMARK_NAME}_URI_BENCHMARK})

    # Указывает каталог для ВСЕХ исполняемых файлов
    SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/benchmarks)

    # Обнуляем флаги для сборки бенчмарков
    SET(AWH_FLAGS "")

    # Если сборка производится под операционную систему Windows
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
        # Устанавливаем необходимые зависимости для MS Windows
        SET(AWH_FLAGS -lws2_32 -lgdi32 -lcrypt32 -lIphlpapi -lbcrypt -flto=auto)
        # Стандартная библиотека C++ здесь НЕ упоминается намеренно: собиратель C++ подключает её
        # сам, а упомянутая отдельно она подтягивается ОТДЕЛЬНЫМ файлом и спорит со статической,
        # какою набор связывается ради работоспособности учёта выделений памяти. См. корневой
        # CMakeLists.txt
    # Если сборка производится под операционные системы Linux, FreeBSD, NetBSD, OpenBSD, macOS или Solaris
    else()
        # Если сборка производится под операционную систему macOS
        if (APPLE AND NOT IOS)
            # Устанавливаем необходимые зависимости для macOS
            SET(AWH_FLAGS "-framework Foundation")
        endif()
    endif()

    # Сборка бинарного приложения бенчмарков
    add_executable(
        ${PROJECT_NAME}${POSTFIX}
        ${${PROJECT_NAME}_BENCHMARK_SOURCES}
    )

    # Указываем, какое имя файла использовать при сборке
    set_target_properties(
        ${PROJECT_NAME}${POSTFIX} PROPERTIES
        OUTPUT_NAME ${NAME}
    )

    # Если нужно собрать и динамическую библиотеку
    if (CMAKE_SHARED_BUILD_LIB)
        target_link_libraries(${PROJECT_NAME}${POSTFIX} PUBLIC
            ${CMAKE_DL_LIBS}
            ${PROJECT_NAME}
            ${AWH_FLAGS}
        )
    # Если нужно собрать и статическую библиотеку
    else (CMAKE_SHARED_BUILD_LIB)
        target_link_libraries(${PROJECT_NAME}${POSTFIX} PUBLIC
            ${CMAKE_DL_LIBS}
            ${PROJECT_NAME}
            ${DEPEND_LIBRARY}
            ${PCRE_LIBRARY}
            ${AWH_FLAGS}
        )
    endif (CMAKE_SHARED_BUILD_LIB)

    # Если сборка производится под операционную систему macOS
    if (APPLE AND NOT IOS)
        # Включаем компиляцию в режиме Objective-C++
        target_compile_options(${PROJECT_NAME}${POSTFIX} PRIVATE -x objective-c++ -fobjc-arc)
    endif()

    # Оснастка счёта системных вызовов собирается вместе с набором: показатели по
    # количеству обращений к ядру снимаются ею, и расхождение версий между набором
    # и подставной библиотекой обесценило бы их
    #
    # Зависимость ставится лишь при наличии самой цели: у MS Windows способа подмены
    # нет и цель не заводится вовсе, а ссылка на незаведённую цель роняет НАСТРОЙКУ
    # сборки целиком - "Generate step failed"
    if (TARGET awh_BENCHMARK_syscount)
        add_dependencies(${PROJECT_NAME}${POSTFIX} awh_BENCHMARK_syscount)
    endif()

    # Если сборка производится под операционную систему MS Windows средствами MinGW
    if (MINGW)
        # Подключаем стандартную библиотеку статически ради работоспособности учёта выделений памяти
        #
        # Учёт этот ведётся заменой оператора выделения памяти в самом бинарном приложении, а у MS
        # Windows всякая динамическая библиотека несёт свой собственный оператор: выделения, сделанные
        # внутри libstdc++-6.dll, замене не видны вовсе. Учёт при этом не отказывает, а МОЛЧА
        # НЕДОСЧИТЫВАЕТ - показатель "не более" отчитывается "уложился" при любом пороге. Замерено на
        # стенде MSYS2 MinGW64 (14.08.2026) на наборе codec
        target_link_options(${PROJECT_NAME}${POSTFIX} PRIVATE -static-libstdc++ -static-libgcc)
    endif()

    # Регистрируем цель запуска бенчмарков с проверкой порогов
    add_custom_target(
        benchmark-${NAME}
        COMMAND ${CMAKE_BINARY_DIR}/benchmarks/${NAME}
        DEPENDS ${PROJECT_NAME}${POSTFIX}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running ${NAME} benchmarks with performance thresholds"
    )

    # Если сборка производится под операционную систему macOS
    if (APPLE AND NOT IOS)
        # Регистрируем цель запуска бенчмарков со счётом системных вызовов.
        # Показатели по количеству обращений к ядру без внедрённой подставной
        # библиотеки выводятся как пропущенные, поэтому для них нужен отдельный
        # прогон: внедрение замедляет каждый перехваченный вызов, и показатели по
        # времени с ним сравнивать с обычным прогоном нельзя
        add_custom_target(
            benchmark-${NAME}-syscalls
            COMMAND ${CMAKE_COMMAND} -E env
                DYLD_INSERT_LIBRARIES=${CMAKE_BINARY_DIR}/benchmarks/libsyscount${CMAKE_SHARED_LIBRARY_SUFFIX}
                ${CMAKE_BINARY_DIR}/benchmarks/${NAME}
            DEPENDS ${PROJECT_NAME}${POSTFIX} ${AWH_SYSCOUNT_TARGET}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Running ${NAME} benchmarks with system call accounting"
        )
    # Если сборка производится под операционную систему Linux
    elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        # Та же цель для Linux. Отличается она одним - именем переменной окружения,
        # которой загрузчик внедряет подставную библиотеку: у dyld это
        # DYLD_INSERT_LIBRARIES, у загрузчика ELF - LD_PRELOAD
        add_custom_target(
            benchmark-${NAME}-syscalls
            COMMAND ${CMAKE_COMMAND} -E env
                LD_PRELOAD=${CMAKE_BINARY_DIR}/benchmarks/libsyscount${CMAKE_SHARED_LIBRARY_SUFFIX}
                ${CMAKE_BINARY_DIR}/benchmarks/${NAME}
            DEPENDS ${PROJECT_NAME}${POSTFIX} ${AWH_SYSCOUNT_TARGET}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Running ${NAME} benchmarks with system call accounting"
        )
    endif()
# Если не нужно собрать бенчмарки
else()
    # Выводим сообщение о сборке бенчмарков
    message(STATUS "Build benchmarks ${NAME} module: No\n")
endif (${CMAKE_BUILD_${BENCHMARK_NAME}_BENCHMARKS})
