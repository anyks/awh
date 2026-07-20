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
    SET(NAME "proto")

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

        # Источники для сборки юнит-тестов
        SET(${PROJECT_NAME}_UNITTEST_SOURCES ../main.cpp)

        # Указываем что нужно собрать тесты для подмодуля провайдера HTTP-запроса/ответа
        option(CMAKE_BUILD_${UNIT_TEST_NAME}_HTTP_PROVIDER_TEST "Build unit HTTP provider test for ${UNIT_TEST_NAME}" ON)
        # Указываем что нужно собрать тесты для подмодуля контейнера HTTP-заголовков
        option(CMAKE_BUILD_${UNIT_TEST_NAME}_HTTP_HEADERS_TEST "Build unit HTTP headers test for ${UNIT_TEST_NAME}" ON)
        # Указываем что нужно собрать тесты для подмодуля HTTP-авторизации
        option(CMAKE_BUILD_${UNIT_TEST_NAME}_HTTP_AUTH_TEST "Build unit HTTP auth test for ${UNIT_TEST_NAME}" ON)
        # Указываем что нужно собрать тесты для подмодуля парсера HTTP/1.0 и HTTP/1.1
        option(CMAKE_BUILD_${UNIT_TEST_NAME}_HTTP_PARSER_TEST "Build unit HTTP parser test for ${UNIT_TEST_NAME}" ON)
        # Указываем что нужно собрать тесты для подмодуля парсера HTTP/2
        option(CMAKE_BUILD_${UNIT_TEST_NAME}_HTTP_PARSER2_TEST "Build unit HTTP/2 parser test for ${UNIT_TEST_NAME}" ON)

        # Если нужно собрать тесты для подмодуля провайдера HTTP-запроса/ответа
        if (${CMAKE_BUILD_${UNIT_TEST_NAME}_HTTP_PROVIDER_TEST})
            # Выводим сообщение о сборке тестов для подмодуля провайдера HTTP-запроса/ответа
            message(STATUS "Build tests HTTP provider submodule: Yes\n")
            # Дополняем список источников тестами для подмодуля провайдера HTTP-запроса/ответа
            list(APPEND ${PROJECT_NAME}_UNITTEST_SOURCES
                http/provider/provider.cpp
                http/provider/static.cpp
                http/provider/parameterized.cpp
            )
        # Если не нужно собрать тесты для подмодуля провайдера HTTP-запроса/ответа
        else()
            # Выводим сообщение о сборке тестов для подмодуля провайдера HTTP-запроса/ответа
            message(STATUS "Build tests HTTP provider submodule: No\n")
        endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_HTTP_PROVIDER_TEST})

        # Если нужно собрать тесты для подмодуля контейнера HTTP-заголовков
        if (${CMAKE_BUILD_${UNIT_TEST_NAME}_HTTP_HEADERS_TEST})
            # Выводим сообщение о сборке тестов для подмодуля контейнера HTTP-заголовков
            message(STATUS "Build tests HTTP headers submodule: Yes\n")
            # Дополняем список источников тестами для подмодуля контейнера HTTP-заголовков
            list(APPEND ${PROJECT_NAME}_UNITTEST_SOURCES
                http/headers/headers.cpp
                http/headers/static.cpp
                http/headers/parameterized.cpp
            )
        # Если не нужно собрать тесты для подмодуля контейнера HTTP-заголовков
        else()
            # Выводим сообщение о сборке тестов для подмодуля контейнера HTTP-заголовков
            message(STATUS "Build tests HTTP headers submodule: No\n")
        endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_HTTP_HEADERS_TEST})

        # Если нужно собрать тесты для подмодуля HTTP-авторизации
        if (${CMAKE_BUILD_${UNIT_TEST_NAME}_HTTP_AUTH_TEST})
            # Выводим сообщение о сборке тестов для подмодуля HTTP-авторизации
            message(STATUS "Build tests HTTP auth submodule: Yes\n")
            # Дополняем список источников тестами для подмодуля HTTP-авторизации
            list(APPEND ${PROJECT_NAME}_UNITTEST_SOURCES
                http/auth/auth.cpp
                http/auth/static.cpp
                http/auth/parameterized.cpp
            )
        # Если не нужно собрать тесты для подмодуля HTTP-авторизации
        else()
            # Выводим сообщение о сборке тестов для подмодуля HTTP-авторизации
            message(STATUS "Build tests HTTP auth submodule: No\n")
        endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_HTTP_AUTH_TEST})

        # Если нужно собрать тесты для подмодуля парсера HTTP/1.0 и HTTP/1.1
        if (${CMAKE_BUILD_${UNIT_TEST_NAME}_HTTP_PARSER_TEST})
            # Выводим сообщение о сборке тестов для подмодуля парсера HTTP/1.0 и HTTP/1.1
            message(STATUS "Build tests HTTP parser submodule: Yes\n")
            # Дополняем список источников тестами для подмодуля парсера HTTP/1.0 и HTTP/1.1
            list(APPEND ${PROJECT_NAME}_UNITTEST_SOURCES
                http/parser/http1/http1.cpp
                http/parser/http1/static.cpp
                http/parser/http1/sender.cpp
                http/parser/http1/parameterized.cpp
            )
        # Если не нужно собрать тесты для подмодуля парсера HTTP/1.0 и HTTP/1.1
        else()
            # Выводим сообщение о сборке тестов для подмодуля парсера HTTP/1.0 и HTTP/1.1
            message(STATUS "Build tests HTTP parser submodule: No\n")
        endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_HTTP_PARSER_TEST})

        # Если нужно собрать тесты для подмодуля парсера HTTP/2
        if (${CMAKE_BUILD_${UNIT_TEST_NAME}_HTTP_PARSER2_TEST})
            # Выводим сообщение о сборке тестов для подмодуля парсера HTTP/2
            message(STATUS "Build tests HTTP/2 parser submodule: Yes\n")
            # Дополняем список источников тестами для подмодуля парсера HTTP/2
            list(APPEND ${PROJECT_NAME}_UNITTEST_SOURCES
                http/parser/http2/http2.cpp
                http/parser/http2/static.cpp
                http/parser/http2/parameterized.cpp
            )
        # Если не нужно собрать тесты для подмодуля парсера HTTP/2
        else()
            # Выводим сообщение о сборке тестов для подмодуля парсера HTTP/2
            message(STATUS "Build tests HTTP/2 parser submodule: No\n")
        endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_HTTP_PARSER2_TEST})

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
            SET(AWH_FLAGS -lstdc++ -lws2_32 -lgdi32 -lcrypt32 -lIphlpapi -lbcrypt -flto=auto)

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
                ${TCMALLOC_LIBRARIES}
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
                    ${TCMALLOC_LIBRARIES}
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
                    ${TCMALLOC_LIBRARIES}
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
        # Выводим сообщение о сборке тестов для модуля работы с фреймворком
        message(STATUS "Build tests ${NAME} module: No\n")
    endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_TESTS})
endif()
