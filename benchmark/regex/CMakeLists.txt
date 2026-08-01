# Определяем имя для бенчмарков
SET(NAME "regex")

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

    # Указываем что нужно собрать бенчмарки для подмодуля сопоставления
    option(CMAKE_BUILD_${BENCHMARK_NAME}_MATCHING_BENCHMARK "Build Matching benchmark for ${BENCHMARK_NAME}" ON)

    # Если нужно собрать бенчмарки для подмодуля сопоставления
    if (${CMAKE_BUILD_${BENCHMARK_NAME}_MATCHING_BENCHMARK})
        # Выводим сообщение о сборке бенчмарков для подмодуля сопоставления
        message(STATUS "Build benchmarks Matching submodule: Yes\n")
        # Дополняем список источников бенчмарками для подмодуля сопоставления
        list(APPEND ${PROJECT_NAME}_BENCHMARK_SOURCES
            matching/matching.cpp
        )
    # Если не нужно собрать бенчмарки для подмодуля сопоставления
    else()
        # Выводим сообщение о сборке бенчмарков для подмодуля сопоставления
        message(STATUS "Build benchmarks Matching submodule: No\n")
    endif (${CMAKE_BUILD_${BENCHMARK_NAME}_MATCHING_BENCHMARK})

    # Эталонная реализация PCRE2 подключается только бенчмарками сравнения: сравнение
    # с нею и есть их предмет, тогда как самой библиотеке она не нужна
    include_directories(${PCRE_INCLUDE_DIR})

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
            ${PCRE_LIBRARY}
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
