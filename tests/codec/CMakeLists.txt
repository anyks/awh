# Если операционной системой является FreeBSD
if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    # Указываем путь к установленной библиотеке GTest
    find_path(GTEST_INCLUDE_DIR gtest/gtest.h
        PATHS ${GTEST_ROOT}/include
        #NO_DEFAULT_PATH
    )

    # Ищем библиотеку GTest
    find_library(GTEST_LIBRARY gtest
        PATHS ${GTEST_ROOT}/lib
        #NO_DEFAULT_PATH
    )

    # Ищем основную библиотеку GTest
    find_library(GTEST_MAIN_LIBRARY gtest_main
        PATHS ${GTEST_ROOT}/lib
        #NO_DEFAULT_PATH
    )

    # Устанавливаем переменные, как будто find_package(GTest) сработал
    SET(GTEST_FOUND TRUE)
    SET(GTEST_INCLUDE_DIRS ${GTEST_INCLUDE_DIR})
    SET(GTEST_LIBRARIES ${GTEST_LIBRARY})
    SET(GTEST_MAIN_LIBRARIES ${GTEST_MAIN_LIBRARY})

    # Если заголовочные файлы GTest не найдены
    if(NOT GTEST_INCLUDE_DIR)
        message(FATAL_ERROR "GTest headers not found in /usr/local/include")
    endif()

    # Если библиотека GTest не найдена
    if(NOT GTEST_LIBRARY)
        message(FATAL_ERROR "GTest library not found in /usr/local/lib")
    endif()
endif()

# Unit test settings
find_package(GTest)

# Если библиотека GTest найдена
if (${GTest_FOUND})
    # Определяем имя для юнит-тестов
    SET(NAME "codec")

    # Переводим название теста в верхний регистр
    string(TOUPPER "${NAME}" UNIT_TEST_NAME)

    # Указываем что нужно собрать тесты
    option(CMAKE_BUILD_${UNIT_TEST_NAME}_TESTS "Build unit tests for ${UNIT_TEST_NAME}" ON)

    # Если необходимо собрать тесты
    if (${CMAKE_BUILD_${UNIT_TEST_NAME}_TESTS})
        # Определяем постфикс для юнит-тестов
        SET(POSTFIX "_UNITTEST_${NAME}")

        # Enable C++17
        SET(CMAKE_CXX_STANDARD 17)
        SET(CMAKE_CXX_STANDARD_REQUIRED ON)

        # Убираем ненужные и глупые варинги которые не имеют ничего общего с реальностью
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-implicitly-unsigned-literal -Wno-reserved-user-defined-literal")

        # Выводим сообщение о сборке тестов для модуля codec
        message(STATUS "Build tests CODEC module: Yes\n")

        # Источники для сборки юнит-тестов
        SET(${PROJECT_NAME}_UNITTEST_SOURCES
            ../main.cpp
            contract.cpp
            bridge.cpp
            xml/common.cpp
            xml/encoding.cpp
            xml/reader.cpp
            xml/document.cpp
            xml/writer.cpp
            xml/value.cpp
            cef/common.cpp
            cef/reader.cpp
            cef/writer.cpp
            cef/document.cpp
            cef/dictionary.cpp
            ini/common.cpp
            ini/encoding.cpp
            ini/reader.cpp
            ini/writer.cpp
            ini/document.cpp
            ini/value.cpp
            csv/common.cpp
            csv/encoding.cpp
            csv/reader.cpp
            csv/writer.cpp
            csv/document.cpp
            csv/value.cpp
            toml/common.cpp
            toml/encoding.cpp
            toml/reader.cpp
            toml/writer.cpp
            toml/document.cpp
            toml/value.cpp
            json/common.cpp
            json/encoding.cpp
            json/reader.cpp
            json/writer.cpp
            json/document.cpp
            json/value.cpp
            yaml/common.cpp
            yaml/encoding.cpp
            yaml/reader.cpp
            yaml/writer.cpp
            yaml/document.cpp
            yaml/value.cpp
            abc/common.cpp
            abc/header.cpp
            abc/chunk.cpp
            abc/index.cpp
            abc/container.cpp
            abc/signature.cpp
            abc/schedule.cpp
            abc/editor.cpp
            abc/storage.cpp
            abc/encoding.cpp
            abc/reader.cpp
            abc/writer.cpp
            abc/document.cpp
            abc/value.cpp
        )

        # Библиотеки GTest для линковки
        SET(${PROJECT_NAME}_GTEST_LIBS
            GTest::gtest
            GTest::gmock
        )

        # Указывает каталог для ВСЕХ исполняемых файлов
        SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/unit-tests)

        # Обнуляем флаги для сборки семплов
        SET(AWH_FLAGS "")

        # Если сборка производится под операционную систему Windows
        if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
            # export MSYS="error_start:%WD%dumper.exe -d -c %1 %2"
            # Устанавливаем необходимые зависимости для MS Windows
            SET(AWH_FLAGS -lstdc++ -lws2_32 -lIphlpapi -lpsapi -ldbghelp -lsynchronization -lcrypt32 -lbcrypt -lgdi32 -flto=auto)

            # Устанавливаем иконку для windows под MinGW.
            SET(RES_FILES ${CMAKE_PROJECT_NAME}.rc)
            SET(CMAKE_RC_COMPILER_INIT windres)
            ENABLE_LANGUAGE(RC)
            SET(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
        # Если сборка производится под операционные системы Linux, FreeBSD, NetBSD, OpenBSD, macOS или Solaris
        else()
            # Если сборка производится под операционную систему macOS
            if (APPLE AND NOT IOS)
                # Устанавливаем необходимые зависимости для macOS
                SET(AWH_FLAGS "-framework Foundation")
            endif()
        endif()

        # Сборка бинарного приложения тестов
        add_executable(
            ${PROJECT_NAME}${POSTFIX}
            ${${PROJECT_NAME}_UNITTEST_SOURCES}
        )

        # Указываем, какое имя файла использовать при сборке
        set_target_properties(
            ${PROJECT_NAME}${POSTFIX} PROPERTIES
            OUTPUT_NAME ${NAME}
        )

        # Если нужно собрать и динамическую библиотеку
        if (CMAKE_SHARED_BUILD_LIB)
            target_link_libraries(${PROJECT_NAME}${POSTFIX} PUBLIC
                ${${PROJECT_NAME}_GTEST_LIBS}
                ${AWH_LIB_GCOV_NAME}
                ${GCC_COVERAGE_LIBS}
                ${AWH_FLAGS}
            )
        # Если нужно собрать и статическую библиотеку
        else (CMAKE_SHARED_BUILD_LIB)
            # Если режим отладки не активирован
            if (NOT CMAKE_BUILD_TYPE MATCHES Debug)
                target_link_libraries(${PROJECT_NAME}${POSTFIX} PUBLIC
                    ${${PROJECT_NAME}_GTEST_LIBS}
                    ${AWH_LIB_GCOV_NAME}
                    ${PCRE_LIBRARY}
                    ${GCC_COVERAGE_LIBS}
                    ${AWH_FLAGS}
                )
            # Если режим отладки активирован
            else()
                target_link_libraries(${PROJECT_NAME}${POSTFIX} PUBLIC
                    ${${PROJECT_NAME}_GTEST_LIBS}
                    ${AWH_LIB_GCOV_NAME}
                    ${DEPEND_LIBRARY}
                    ${PCRE_LIBRARY}
                    ${GCC_COVERAGE_LIBS}
                    ${AWH_FLAGS}
                )
            endif()
        endif (CMAKE_SHARED_BUILD_LIB)

        # Если сборка производится под операционную систему macOS
        if (APPLE AND NOT IOS)
            # Включаем флаги компиляции для сбора статистики покрытия кода
            target_compile_options(
                ${PROJECT_NAME}${POSTFIX} PRIVATE
                ${GCC_COVERAGE_COMPILE_FLAGS} -x objective-c++ -fobjc-arc
            )
        # Если сборка производится под операционную систему Windows
        else()
            # Включаем флаги компиляции для сбора статистики покрытия кода
            target_compile_options(${PROJECT_NAME}${POSTFIX} PRIVATE ${GCC_COVERAGE_COMPILE_FLAGS})
        endif()
    # Если не нужно собрать тесты
    else()
        # Выводим сообщение о сборке тестов для модуля codec
        message(STATUS "Build tests CODEC module: No\n")
    endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_TESTS})
endif()
