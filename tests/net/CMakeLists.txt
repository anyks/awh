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
    SET(NAME "net")

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

        # Указываем что нужно собрать тесты для работы с адресами
        option(CMAKE_BUILD_${UNIT_TEST_NAME}_ADDR_TEST "Build unit ADDR test for ${UNIT_TEST_NAME}" ON)
        # Указываем что нужно собрать тесты для парсинга сетевых протоколов
        option(CMAKE_BUILD_${UNIT_TEST_NAME}_NWT_TEST "Build unit NWT test for ${UNIT_TEST_NAME}" ON)
        # Указываем что нужно собрать тесты для работы с URI
        option(CMAKE_BUILD_${UNIT_TEST_NAME}_URI_TEST "Build unit URI test for ${UNIT_TEST_NAME}" ON)
        # Указываем что нужно собрать тесты для работы с файловыми дескрипторами
        option(CMAKE_BUILD_${UNIT_TEST_NAME}_FDS_TEST "Build unit FDS test for ${UNIT_TEST_NAME}" ON)
        # Указываем что нужно собрать тесты для работы с Ethernet
        option(CMAKE_BUILD_${UNIT_TEST_NAME}_ETH_TEST "Build unit ETH test for ${UNIT_TEST_NAME}" ON)
        # Указываем что нужно собрать тесты для работы с модулем ввода-вывода
        option(CMAKE_BUILD_${UNIT_TEST_NAME}_IO_TEST "Build unit IO test for ${UNIT_TEST_NAME}" ON)
        # Указываем что нужно собрать тесты для работы с очередями
        option(CMAKE_BUILD_${UNIT_TEST_NAME}_QUEUE_TEST "Build unit QUEUE test for ${UNIT_TEST_NAME}" ON)

        # Если нужно собрать тесты для работы с адресами
        if (${CMAKE_BUILD_${UNIT_TEST_NAME}_ADDR_TEST})
            # Выводим сообщение о сборке тестов для модуля работы с сетевыми адресами
            message(STATUS "Build tests ADDR module: Yes\n")
            # Дополняем список источников тестами для модуля работы с сетевыми адресами
            list(APPEND ${PROJECT_NAME}_UNITTEST_SOURCES
                addr/addr.cpp
                addr/static.cpp
                addr/parameterized.cpp
            )
        # Если не нужно собрать тесты для работы с адресами
        else()
            # Выводим сообщение о сборке тестов для модуля работы с сетевыми адресами
            message(STATUS "Build tests ADDR module: No\n")
        endif(${CMAKE_BUILD_${UNIT_TEST_NAME}_ADDR_TEST})

        # Если нужно собрать тесты для парсинга сетевых протоколов
        if (${CMAKE_BUILD_${UNIT_TEST_NAME}_NWT_TEST})
            # Выводим сообщение о сборке тестов для модуля парсинга сетевых протоколов
            message(STATUS "Build tests NWT module: Yes\n")
            # Дополняем список источников тестами для парсинга сетевых протоколов
            list(APPEND ${PROJECT_NAME}_UNITTEST_SOURCES
                nwt/nwt.cpp
                nwt/static.cpp
                nwt/parameterized.cpp
            )
        # Если не нужно собрать тесты для парсинга сетевых протоколов
        else()
            # Выводим сообщение о сборке тестов для модуля парсинга сетевых протоколов
            message(STATUS "Build tests NWT module: No\n")
        endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_NWT_TEST})

        # Если нужно собрать тесты для работы с URI
        if (${CMAKE_BUILD_${UNIT_TEST_NAME}_URI_TEST})
            # Выводим сообщение о сборке тестов для модуля работы с URI
            message(STATUS "Build tests URI module: Yes\n")
            # Дополняем список источников тестами для модуля работы с URI
            list(APPEND ${PROJECT_NAME}_UNITTEST_SOURCES
                uri/uri.cpp
                uri/static.cpp
                uri/parameterized.cpp
            )
        # Если не нужно собрать тесты для работы с URI
        else()
            # Выводим сообщение о сборке тестов для модуля работы с URI
            message(STATUS "Build tests URI module: No\n")
        endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_URI_TEST})

        # Если нужно собрать тесты для работы с файловыми дескрипторами
        if (${CMAKE_BUILD_${UNIT_TEST_NAME}_FDS_TEST})
            # Выводим сообщение о сборке тестов для работы с файловыми дескрипторами
            message(STATUS "Build tests FDS module: Yes\n")
            # Дополняем список источников тестами для работы с файловыми дескрипторами
            list(APPEND ${PROJECT_NAME}_UNITTEST_SOURCES
                fds/fds.cpp
                fds/static.cpp
            )
        # Если не нужно собрать тесты для работы с файловыми дескрипторами
        else()
            # Выводим сообщение о сборке тестов для модуля работы с файловыми дескрипторами
            message(STATUS "Build tests FDS module: No\n")
        endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_FDS_TEST})

        # Если нужно собрать тесты для работы с Ethernet
        if (${CMAKE_BUILD_${UNIT_TEST_NAME}_ETH_TEST})
            # Выводим сообщение о сборке тестов для работы с Ethernet
            message(STATUS "Build tests ETH module: Yes\n")
            # Дополняем список источников тестами для работы с Ethernet
            list(APPEND ${PROJECT_NAME}_UNITTEST_SOURCES
                eth/eth.cpp
                eth/static.cpp
                eth/addr.cpp
                eth/iface.cpp
                eth/gateway.cpp
                eth/socket.cpp
                eth/portmap.cpp
                eth/sctp.cpp
            )
        # Если не нужно собрать тесты для работы с Ethernet
        else()
            # Выводим сообщение о сборке тестов для модуля работы с Ethernet
            message(STATUS "Build tests ETH module: No\n")
        endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_ETH_TEST})

        # Если нужно собрать тесты для работы с модулем ввода-вывода
        if (${CMAKE_BUILD_${UNIT_TEST_NAME}_IO_TEST})
            # Выводим сообщение о сборке тестов для работы с модулем ввода-вывода
            message(STATUS "Build tests IO module: Yes\n")
            # Дополняем список источников тестами для работы с модулем ввода-вывода
            list(APPEND ${PROJECT_NAME}_UNITTEST_SOURCES
                io/io.cpp
                io/static.cpp
                io/parameterized.cpp
            )
        # Если не нужно собрать тесты для работы с модулем ввода-вывода
        else()
            # Выводим сообщение о сборке тестов для модуля работы с модулем ввода-вывода
            message(STATUS "Build tests IO module: No\n")
        endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_IO_TEST})

        # Если нужно собрать тесты для работы с сетевыми очередями
        if (${CMAKE_BUILD_${UNIT_TEST_NAME}_QUEUE_TEST})
            # Выводим сообщение о сборке тестов для модуля работы с сетевыми очередями
            message(STATUS "Build tests Network QUEUE module: Yes\n")
            # Дополняем список источников тестами для модуля работы с сетевыми очередями
            list(APPEND ${PROJECT_NAME}_UNITTEST_SOURCES
                queue/queue.cpp
                queue/static.cpp
                queue/parameterized.cpp
            )
        # Если не нужно собрать тесты для работы с сетевыми очередями
        else()
            # Выводим сообщение о сборке тестов для модуля работы с сетевыми очередями
            message(STATUS "Build tests Network QUEUE module: No\n")
        endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_QUEUE_TEST})

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
