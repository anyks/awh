# Определяем имя для бенчмарков
SET(NAME "codec")

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

    # Указываем что нужно собрать бенчмарки для контейнера XML
    option(CMAKE_BUILD_${BENCHMARK_NAME}_XML_BENCHMARK "Build XML benchmark for ${BENCHMARK_NAME}" ON)

    # Если нужно собрать бенчмарки для контейнера XML
    if (${CMAKE_BUILD_${BENCHMARK_NAME}_XML_BENCHMARK})
        # Выводим сообщение о сборке бенчмарков для контейнера XML
        message(STATUS "Build benchmarks XML submodule: Yes\n")
        # Дополняем список источников бенчмарками для контейнера XML
        list(APPEND ${PROJECT_NAME}_BENCHMARK_SOURCES
            xml/xml.cpp
            xml/reader.cpp
            xml/writer.cpp
            xml/document.cpp
        )
    # Если не нужно собрать бенчмарки для контейнера XML
    else()
        # Выводим сообщение о сборке бенчмарков для контейнера XML
        message(STATUS "Build benchmarks XML submodule: No\n")
    endif (${CMAKE_BUILD_${BENCHMARK_NAME}_XML_BENCHMARK})

    # Указываем что нужно собрать бенчмарки для контейнера JSON
    option(CMAKE_BUILD_${BENCHMARK_NAME}_JSON_BENCHMARK "Build JSON benchmark for ${BENCHMARK_NAME}" ON)

    # Если нужно собрать бенчмарки для контейнера JSON
    if (${CMAKE_BUILD_${BENCHMARK_NAME}_JSON_BENCHMARK})
        # Выводим сообщение о сборке бенчмарков для контейнера JSON
        message(STATUS "Build benchmarks JSON submodule: Yes\n")
        # Дополняем список источников бенчмарками для контейнера JSON
        list(APPEND ${PROJECT_NAME}_BENCHMARK_SOURCES
            json/json.cpp
            json/reader.cpp
            json/writer.cpp
            json/document.cpp
        )
    # Если не нужно собрать бенчмарки для контейнера JSON
    else()
        # Выводим сообщение о сборке бенчмарков для контейнера JSON
        message(STATUS "Build benchmarks JSON submodule: No\n")
    endif (${CMAKE_BUILD_${BENCHMARK_NAME}_JSON_BENCHMARK})

    # Указываем что нужно собрать бенчмарки для контейнера INI
    option(CMAKE_BUILD_${BENCHMARK_NAME}_INI_BENCHMARK "Build INI benchmark for ${BENCHMARK_NAME}" ON)

    # Если нужно собрать бенчмарки для контейнера INI
    if (${CMAKE_BUILD_${BENCHMARK_NAME}_INI_BENCHMARK})
        # Выводим сообщение о сборке бенчмарков для контейнера INI
        message(STATUS "Build benchmarks INI submodule: Yes\n")
        # Дополняем список источников бенчмарками для контейнера INI
        list(APPEND ${PROJECT_NAME}_BENCHMARK_SOURCES
            ini/ini.cpp
            ini/reader.cpp
            ini/writer.cpp
            ini/document.cpp
        )
    # Если не нужно собрать бенчмарки для контейнера INI
    else()
        # Выводим сообщение о сборке бенчмарков для контейнера INI
        message(STATUS "Build benchmarks INI submodule: No\n")
    endif (${CMAKE_BUILD_${BENCHMARK_NAME}_INI_BENCHMARK})

    # Указываем что нужно собрать бенчмарки для контейнера CSV
    option(CMAKE_BUILD_${BENCHMARK_NAME}_CSV_BENCHMARK "Build CSV benchmark for ${BENCHMARK_NAME}" ON)

    # Если нужно собрать бенчмарки для контейнера CSV
    if (${CMAKE_BUILD_${BENCHMARK_NAME}_CSV_BENCHMARK})
        # Выводим сообщение о сборке бенчмарков для контейнера CSV
        message(STATUS "Build benchmarks CSV submodule: Yes\n")
        # Дополняем список источников бенчмарками для контейнера CSV
        list(APPEND ${PROJECT_NAME}_BENCHMARK_SOURCES
            csv/csv.cpp
            csv/reader.cpp
            csv/writer.cpp
            csv/document.cpp
        )
    # Если не нужно собрать бенчмарки для контейнера CSV
    else()
        # Выводим сообщение о сборке бенчмарков для контейнера CSV
        message(STATUS "Build benchmarks CSV submodule: No\n")
    endif (${CMAKE_BUILD_${BENCHMARK_NAME}_CSV_BENCHMARK})

    # Указываем что нужно собрать бенчмарки для контейнера TOML
    option(CMAKE_BUILD_${BENCHMARK_NAME}_TOML_BENCHMARK "Build TOML benchmark for ${BENCHMARK_NAME}" ON)

    # Если нужно собрать бенчмарки для контейнера TOML
    if (${CMAKE_BUILD_${BENCHMARK_NAME}_TOML_BENCHMARK})
        # Выводим сообщение о сборке бенчмарков для контейнера TOML
        message(STATUS "Build benchmarks TOML submodule: Yes\n")
        # Дополняем список источников бенчмарками для контейнера TOML
        list(APPEND ${PROJECT_NAME}_BENCHMARK_SOURCES
            toml/toml.cpp
            toml/reader.cpp
            toml/writer.cpp
            toml/document.cpp
        )
    # Если не нужно собрать бенчмарки для контейнера TOML
    else()
        # Выводим сообщение о сборке бенчмарков для контейнера TOML
        message(STATUS "Build benchmarks TOML submodule: No\n")
    endif (${CMAKE_BUILD_${BENCHMARK_NAME}_TOML_BENCHMARK})

    # Указываем что нужно собрать бенчмарки для контейнера YAML
    option(CMAKE_BUILD_${BENCHMARK_NAME}_YAML_BENCHMARK "Build YAML benchmark for ${BENCHMARK_NAME}" ON)

    # Если нужно собрать бенчмарки для контейнера YAML
    if (${CMAKE_BUILD_${BENCHMARK_NAME}_YAML_BENCHMARK})
        # Выводим сообщение о сборке бенчмарков для контейнера YAML
        message(STATUS "Build benchmarks YAML submodule: Yes\n")
        # Дополняем список источников бенчмарками для контейнера YAML
        list(APPEND ${PROJECT_NAME}_BENCHMARK_SOURCES
            yaml/yaml.cpp
            yaml/reader.cpp
            yaml/writer.cpp
            yaml/document.cpp
        )
    # Если не нужно собрать бенчмарки для контейнера YAML
    else()
        # Выводим сообщение о сборке бенчмарков для контейнера YAML
        message(STATUS "Build benchmarks YAML submodule: No\n")
    endif (${CMAKE_BUILD_${BENCHMARK_NAME}_YAML_BENCHMARK})

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

    # Если сборка производится под операционную систему MS Windows средствами MinGW
    if (MINGW)
        # Подключаем стандартную библиотеку статически ради работоспособности учёта выделений памяти
        #
        # Учёт этот ведётся заменой оператора выделения памяти в самом бинарном приложении, а у MS
        # Windows всякая динамическая библиотека несёт свой собственный оператор: выделения, сделанные
        # внутри libstdc++-6.dll, замене не видны вовсе. Учёт при этом не отказывает, а МОЛЧА
        # НЕДОСЧИТЫВАЕТ - показатель "не более" отчитывается "уложился" при любом пороге. Замерено на
        # стенде MSYS2 MinGW64 (14.08.2026): 22 выделения против 64, а на записи ровно нуль
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
# Если не нужно собрать бенчмарки
else()
    # Выводим сообщение о сборке бенчмарков
    message(STATUS "Build benchmarks ${NAME} module: No\n")
endif (${CMAKE_BUILD_${BENCHMARK_NAME}_BENCHMARKS})
