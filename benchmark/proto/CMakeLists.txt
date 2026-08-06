# Определяем имя для бенчмарков
SET(NAME "proto")

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

    # Указываем что нужно собрать бенчмарки для подмодуля протокола QUIC
    option(CMAKE_BUILD_${BENCHMARK_NAME}_QUIC_BENCHMARK "Build QUIC benchmark for ${BENCHMARK_NAME}" ON)

    # Если нужно собрать бенчмарки для подмодуля протокола QUIC
    if (${CMAKE_BUILD_${BENCHMARK_NAME}_QUIC_BENCHMARK})
        # Выводим сообщение о сборке бенчмарков для подмодуля протокола QUIC
        message(STATUS "Build benchmarks QUIC protocol submodule: Yes\n")
        # Дополняем список источников бенчмарками для подмодуля протокола QUIC
        list(APPEND ${PROJECT_NAME}_BENCHMARK_SOURCES
            quic/quic.cpp
            quic/throughput.cpp
        )
    # Если не нужно собрать бенчмарки для подмодуля протокола QUIC
    else()
        # Выводим сообщение о сборке бенчмарков для подмодуля протокола QUIC
        message(STATUS "Build benchmarks QUIC protocol submodule: No\n")
    endif (${CMAKE_BUILD_${BENCHMARK_NAME}_QUIC_BENCHMARK})

    # Указываем что нужно собрать бенчмарки для подмодуля контейнера HTTP-заголовков
    option(CMAKE_BUILD_${BENCHMARK_NAME}_HTTP_HEADERS_BENCHMARK "Build HTTP headers benchmark for ${BENCHMARK_NAME}" ON)

    # Если нужно собрать бенчмарки для подмодуля контейнера HTTP-заголовков
    if (${CMAKE_BUILD_${BENCHMARK_NAME}_HTTP_HEADERS_BENCHMARK})
        # Выводим сообщение о сборке бенчмарков для подмодуля контейнера HTTP-заголовков
        message(STATUS "Build benchmarks HTTP headers submodule: Yes\n")
        # Дополняем список источников бенчмарками для подмодуля контейнера HTTP-заголовков
        list(APPEND ${PROJECT_NAME}_BENCHMARK_SOURCES
            headers/headers.cpp
            headers/container.cpp
        )
    # Если не нужно собрать бенчмарки для подмодуля контейнера HTTP-заголовков
    else()
        # Выводим сообщение о сборке бенчмарков для подмодуля контейнера HTTP-заголовков
        message(STATUS "Build benchmarks HTTP headers submodule: No\n")
    endif (${CMAKE_BUILD_${BENCHMARK_NAME}_HTTP_HEADERS_BENCHMARK})

    # Указываем что нужно собрать бенчмарки для подмодуля перенаправления портов
    option(CMAKE_BUILD_${BENCHMARK_NAME}_PORTMAP_BENCHMARK "Build port mapping benchmark for ${BENCHMARK_NAME}" ON)

    # Если нужно собрать бенчмарки для подмодуля перенаправления портов
    if (${CMAKE_BUILD_${BENCHMARK_NAME}_PORTMAP_BENCHMARK})
        # Выводим сообщение о сборке бенчмарков для подмодуля перенаправления портов
        message(STATUS "Build benchmarks port mapping submodule: Yes\n")
        # Дополняем список источников бенчмарками для подмодуля перенаправления портов
        list(APPEND ${PROJECT_NAME}_BENCHMARK_SOURCES
            portmap/portmap.cpp
        )
    # Если не нужно собрать бенчмарки для подмодуля перенаправления портов
    else()
        # Выводим сообщение о сборке бенчмарков для подмодуля перенаправления портов
        message(STATUS "Build benchmarks port mapping submodule: No\n")
    endif (${CMAKE_BUILD_${BENCHMARK_NAME}_PORTMAP_BENCHMARK})

    # Указываем что нужно собрать бенчмарки для подмодуля протокола HTTP/1.x
    option(CMAKE_BUILD_${BENCHMARK_NAME}_HTTP1_BENCHMARK "Build HTTP/1.x benchmark for ${BENCHMARK_NAME}" ON)

    # Если нужно собрать бенчмарки для подмодуля протокола HTTP/1.x
    if (${CMAKE_BUILD_${BENCHMARK_NAME}_HTTP1_BENCHMARK})
        # Выводим сообщение о сборке бенчмарков для подмодуля протокола HTTP/1.x
        message(STATUS "Build benchmarks HTTP/1.x protocol submodule: Yes\n")
        # Дополняем список источников бенчмарками для подмодуля протокола HTTP/1.x
        list(APPEND ${PROJECT_NAME}_BENCHMARK_SOURCES
            http1/http1.cpp
            http1/throughput.cpp
        )
    # Если не нужно собрать бенчмарки для подмодуля протокола HTTP/1.x
    else()
        # Выводим сообщение о сборке бенчмарков для подмодуля протокола HTTP/1.x
        message(STATUS "Build benchmarks HTTP/1.x protocol submodule: No\n")
    endif (${CMAKE_BUILD_${BENCHMARK_NAME}_HTTP1_BENCHMARK})

    # Указываем что нужно собрать бенчмарки для подмодуля протокола HTTP/2
    option(CMAKE_BUILD_${BENCHMARK_NAME}_HTTP2_BENCHMARK "Build HTTP/2 benchmark for ${BENCHMARK_NAME}" ON)

    # Если нужно собрать бенчмарки для подмодуля протокола HTTP/2
    if (${CMAKE_BUILD_${BENCHMARK_NAME}_HTTP2_BENCHMARK})
        # Выводим сообщение о сборке бенчмарков для подмодуля протокола HTTP/2
        message(STATUS "Build benchmarks HTTP/2 protocol submodule: Yes\n")
        # Дополняем список источников бенчмарками для подмодуля протокола HTTP/2
        list(APPEND ${PROJECT_NAME}_BENCHMARK_SOURCES
            http2/http2.cpp
            http2/hpack.cpp
            http2/throughput.cpp
        )
    # Если не нужно собрать бенчмарки для подмодуля протокола HTTP/2
    else()
        # Выводим сообщение о сборке бенчмарков для подмодуля протокола HTTP/2
        message(STATUS "Build benchmarks HTTP/2 protocol submodule: No\n")
    endif (${CMAKE_BUILD_${BENCHMARK_NAME}_HTTP2_BENCHMARK})

    # Указываем что нужно собрать бенчмарки для подмодуля протокола HTTP/3
    option(CMAKE_BUILD_${BENCHMARK_NAME}_HTTP3_BENCHMARK "Build HTTP/3 benchmark for ${BENCHMARK_NAME}" ON)

    # Если нужно собрать бенчмарки для подмодуля протокола HTTP/3
    if (${CMAKE_BUILD_${BENCHMARK_NAME}_HTTP3_BENCHMARK})
        # Выводим сообщение о сборке бенчмарков для подмодуля протокола HTTP/3
        message(STATUS "Build benchmarks HTTP/3 protocol submodule: Yes\n")
        # Дополняем список источников бенчмарками для подмодуля протокола HTTP/3
        list(APPEND ${PROJECT_NAME}_BENCHMARK_SOURCES
            http3/http3.cpp
            http3/qpack.cpp
            http3/throughput.cpp
        )
    # Если не нужно собрать бенчмарки для подмодуля протокола HTTP/3
    else()
        # Выводим сообщение о сборке бенчмарков для подмодуля протокола HTTP/3
        message(STATUS "Build benchmarks HTTP/3 protocol submodule: No\n")
    endif (${CMAKE_BUILD_${BENCHMARK_NAME}_HTTP3_BENCHMARK})

    # Указывает каталог для ВСЕХ исполняемых файлов
    SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/benchmarks)

    # Обнуляем флаги для сборки бенчмарков
    SET(AWH_FLAGS "")

    # Если сборка производится под операционную систему Windows
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
        # Устанавливаем необходимые зависимости для MS Windows
        SET(AWH_FLAGS -lstdc++ -lws2_32 -lgdi32 -lcrypt32 -lIphlpapi -lbcrypt -flto=auto)
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
            ${TCMALLOC_LIBRARIES}
            ${AWH_FLAGS}
        )
    # Если нужно собрать и статическую библиотеку
    else (CMAKE_SHARED_BUILD_LIB)
        target_link_libraries(${PROJECT_NAME}${POSTFIX} PUBLIC
            ${CMAKE_DL_LIBS}
            ${PROJECT_NAME}
            ${DEPEND_LIBRARY}
            ${PCRE_LIBRARY}
            ${TCMALLOC_LIBRARIES}
            ${AWH_FLAGS}
        )
    endif (CMAKE_SHARED_BUILD_LIB)

    # Если сборка производится под операционную систему macOS
    if (APPLE AND NOT IOS)
        # Включаем компиляцию в режиме Objective-C++
        target_compile_options(${PROJECT_NAME}${POSTFIX} PRIVATE -x objective-c++ -fobjc-arc)
    endif()

    # Регистрируем цель запуска бенчмарков с проверкой порогов
    add_custom_target(
        benchmark-${NAME}
        COMMAND ${CMAKE_BINARY_DIR}/benchmarks/${NAME}
        DEPENDS ${PROJECT_NAME}${POSTFIX}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running ${NAME} benchmarks with performance thresholds"
    )
# Если не нужно собрать бенчмарки
else()
    # Выводим сообщение о сборке бенчмарков
    message(STATUS "Build benchmarks ${NAME} module: No\n")
endif (${CMAKE_BUILD_${BENCHMARK_NAME}_BENCHMARKS})
