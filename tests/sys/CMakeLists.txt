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
if(${GTest_FOUND})
    # Определяем имя для юнит-тестов
    SET(NAME "sys")

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

        # Указываем что нужно собрать тесты для работы с ОС
        option(CMAKE_BUILD_${UNIT_TEST_NAME}_OS_TEST "Build unit OS test for ${UNIT_TEST_NAME}" ON)
        # Указываем что нужно собрать тесты для работы с фреймворком
        option(CMAKE_BUILD_${UNIT_TEST_NAME}_FMK_TEST "Build unit FMK test for ${UNIT_TEST_NAME}" ON)
        # Указываем что нужно собрать тесты для работы с логированием
        option(CMAKE_BUILD_${UNIT_TEST_NAME}_LOG_TEST "Build unit LOG test for ${UNIT_TEST_NAME}" ON)
        # Указываем что нужно собрать тесты для работы с регулярными выражениями
        option(CMAKE_BUILD_${UNIT_TEST_NAME}_REG_TEST "Build unit REG test for ${UNIT_TEST_NAME}" ON)
        # Указываем что нужно собрать тесты для работы с очередями
        option(CMAKE_BUILD_${UNIT_TEST_NAME}_QUEUE_TEST "Build unit QUEUE test for ${UNIT_TEST_NAME}" ON)
        # Указываем что нужно собрать тесты для работы с буферами
        option(CMAKE_BUILD_${UNIT_TEST_NAME}_BUFFER_TEST "Build unit BUFFER test for ${UNIT_TEST_NAME}" ON)
        # Указываем что нужно собрать тесты для работы с датой и временем
        option(CMAKE_BUILD_${UNIT_TEST_NAME}_CHRONO_TEST "Build unit CHRONO test for ${UNIT_TEST_NAME}" ON)
        # Указываем что нужно собрать тесты для работы с холдером (блокировкой) доступа к методам и функциям
        option(CMAKE_BUILD_${UNIT_TEST_NAME}_HOLDER_TEST "Build unit HOLDER test for ${UNIT_TEST_NAME}" ON)
        # Указываем что нужно собрать тесты для работы с версионированием
        option(CMAKE_BUILD_${UNIT_TEST_NAME}_VERSION_TEST "Build unit VERSION test for ${UNIT_TEST_NAME}" ON)
        # Указываем что нужно собрать тесты для работы с криптографией
        option(CMAKE_BUILD_${UNIT_TEST_NAME}_CRYPTO_TEST "Build unit CRYPTO test for ${UNIT_TEST_NAME}" ON)
        # Указываем что нужно собрать тесты для работы с компрессором
        option(CMAKE_BUILD_${UNIT_TEST_NAME}_COMPRESSOR_TEST "Build unit COMPRESSOR test for ${UNIT_TEST_NAME}" ON)
        # Указываем что нужно собрать тесты для работы с пуллам потоков
        option(CMAKE_BUILD_${UNIT_TEST_NAME}_THREADPOOL_TEST "Build unit THREADPOOL test for ${UNIT_TEST_NAME}" ON)
        # Указываем что нужно собрать тесты для работы с файловой системой
        option(CMAKE_BUILD_${UNIT_TEST_NAME}_FS_TEST "Build unit FS test for ${UNIT_TEST_NAME}" ON)
        # Указываем что нужно собрать тесты для работы с функциями обратного вызова
        option(CMAKE_BUILD_${UNIT_TEST_NAME}_CALLBACK_TEST "Build unit CALLBACK test for ${UNIT_TEST_NAME}" ON)
        # Указываем что нужно собрать тесты для работы с процессами
        option(CMAKE_BUILD_${UNIT_TEST_NAME}_PROCRE_TEST "Build unit PROCRE test for ${UNIT_TEST_NAME}" ON)
        # Указываем что нужно собрать тесты для работы с сигналами
        option(CMAKE_BUILD_${UNIT_TEST_NAME}_SIGNALS_TEST "Build unit SIGNALS test for ${UNIT_TEST_NAME}" ON)
        # Указываем что нужно собрать тесты для работы с длинными числами
        option(CMAKE_BUILD_${UNIT_TEST_NAME}_BIGNUM_TEST "Build unit BIGNUM test for ${UNIT_TEST_NAME}" ON)

        # Если нужно собрать тесты для работы с ОС
        if (${CMAKE_BUILD_${UNIT_TEST_NAME}_OS_TEST})
            # Выводим сообщение о сборке тестов для модуля работы с операционной системой
            message(STATUS "Build tests OS module: Yes\n")
            # Дополняем список источников тестами для модуля работы с операционной системой
            list(APPEND ${PROJECT_NAME}_UNITTEST_SOURCES
                os/os.cpp
                os/static.cpp
            )
        # Если не нужно собрать тесты для работы с ОС
        else()
            # Выводим сообщение о сборке тестов для модуля работы с операционной системой
            message(STATUS "Build tests OS module: No\n")
        endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_OS_TEST})

        # Если нужно собрать тесты для работы с фреймворком
        if (${CMAKE_BUILD_${UNIT_TEST_NAME}_FMK_TEST})
            # Выводим сообщение о сборке тестов для модуля работы с фреймворком
            message(STATUS "Build tests FMK module: Yes\n")
            # Дополняем список источников тестами для модуля работы с фреймворком
            list(APPEND ${PROJECT_NAME}_UNITTEST_SOURCES
                fmk/fmk.cpp
                fmk/static.cpp
                fmk/parameterized.cpp
            )
        # Если не нужно собрать тесты для работы с фреймворком
        else()
            # Выводим сообщение о сборке тестов для модуля работы с фреймворком
            message(STATUS "Build tests FMK module: No\n")
        endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_FMK_TEST})

        # Если нужно собрать тесты для работы с логированием
        if (${CMAKE_BUILD_${UNIT_TEST_NAME}_LOG_TEST})
            # Выводим сообщение о сборке тестов для модуля работы с логированием
            message(STATUS "Build tests LOG module: Yes\n")
            # Дополняем список источников тестами для модуля работы с логированием
            list(APPEND ${PROJECT_NAME}_UNITTEST_SOURCES
                log/log.cpp
                log/static.cpp
                log/parameterized.cpp
            )
        # Если не нужно собрать тесты для работы с логированием
        else()
            # Выводим сообщение о сборке тестов для модуля работы с логированием
            message(STATUS "Build tests LOG module: No\n")
        endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_LOG_TEST})

        # Если нужно собрать тесты для работы с регулярными выражениями
        if (${CMAKE_BUILD_${UNIT_TEST_NAME}_REG_TEST})
            # Выводим сообщение о сборке тестов для модуля работы с регулярными выражениями
            message(STATUS "Build tests REG module: Yes\n")
            # Дополняем список источников тестами для модуля работы с регулярными выражениями
            list(APPEND ${PROJECT_NAME}_UNITTEST_SOURCES
                reg/reg.cpp
                reg/static.cpp
                reg/parameterized.cpp
            )
        # Если не нужно собрать тесты для работы с регулярными выражениями
        else()
            # Выводим сообщение о сборке тестов для модуля работы с регулярными выражениями
            message(STATUS "Build tests REG module: No\n")
        endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_REG_TEST})

        # Если нужно собрать тесты для работы с очередями
        if (${CMAKE_BUILD_${UNIT_TEST_NAME}_QUEUE_TEST})
            # Выводим сообщение о сборке тестов для модуля работы с очередями
            message(STATUS "Build tests QUEUE module: Yes\n")
            # Дополняем список источников тестами для модуля работы с очередями
            list(APPEND ${PROJECT_NAME}_UNITTEST_SOURCES
                queue/queue.cpp
                queue/static.cpp
                queue/parameterized.cpp
            )
        # Если не нужно собрать тесты для работы с очередями
        else()
            # Выводим сообщение о сборке тестов для модуля работы с очередями
            message(STATUS "Build tests QUEUE module: No\n")
        endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_QUEUE_TEST})

        # Если нужно собрать тесты для работы с буферами
        if (${CMAKE_BUILD_${UNIT_TEST_NAME}_BUFFER_TEST})
            # Выводим сообщение о сборке тестов для модуля работы с буферами
            message(STATUS "Build tests BUFFER module: Yes\n")
            # Дополняем список источников тестами для модуля работы с буферами
            list(APPEND ${PROJECT_NAME}_UNITTEST_SOURCES
                buffer/buffer.cpp
                buffer/static.cpp
                buffer/parameterized.cpp
            )
        # Если не нужно собрать тесты для работы с буферами
        else()
            # Выводим сообщение о сборке тестов для модуля работы с буферами
            message(STATUS "Build tests BUFFER module: No\n")
        endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_BUFFER_TEST})

        # Если нужно собрать тесты для работы с датой и временем
        if (${CMAKE_BUILD_${UNIT_TEST_NAME}_CHRONO_TEST})
            # Выводим сообщение о сборке тестов для модуля работы с датой и временем
            message(STATUS "Build tests CHRONO module: Yes\n")
            # Дополняем список источников тестами для модуля работы с датой и временем
            list(APPEND ${PROJECT_NAME}_UNITTEST_SOURCES
                chrono/chrono.cpp
                chrono/format.cpp
                chrono/parse.cpp
                chrono/roundtrip.cpp
                chrono/zones.cpp
                chrono/syslog.cpp
                chrono/substitution.cpp
                chrono/validate.cpp
                chrono/duration.cpp
                chrono/calendar.cpp
                chrono/parameterized.cpp
            )
        # Если не нужно собрать тесты для работы с датой и временем
        else()
            # Выводим сообщение о сборке тестов для модуля работы с датой и временем
            message(STATUS "Build tests CHRONO module: No\n")
        endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_CHRONO_TEST})

        # Если нужно собрать тесты для работы с холдером (блокировкой) доступа к методам и функциям
        if (${CMAKE_BUILD_${UNIT_TEST_NAME}_HOLDER_TEST})
            # Выводим сообщение о сборке тестов для модуля работы с холдером (блокировкой) доступа к методам и функциям
            message(STATUS "Build tests HOLDER module: Yes\n")
            # Дополняем список источников тестами для модуля работы с холдером (блокировкой) доступа к методам и функциям
            list(APPEND ${PROJECT_NAME}_UNITTEST_SOURCES
                holder/holder.cpp
                holder/parameterized.cpp
            )
        # Если не нужно собрать тесты для работы с холдером (блокировкой) доступа к методам и функциям
        else()
            # Выводим сообщение о сборке тестов для модуля работы с холдером (блокировкой) доступа к методам и функциям
            message(STATUS "Build tests HOLDER module: No\n")
        endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_HOLDER_TEST})

        # Если нужно собрать тесты для работы с версионированием
        if (${CMAKE_BUILD_${UNIT_TEST_NAME}_VERSION_TEST})
            # Выводим сообщение о сборке тестов для модуля работы с версионированием
            message(STATUS "Build tests VERSION module: Yes\n")
            # Дополняем список источников тестами для модуля работы с версионированием
            list(APPEND ${PROJECT_NAME}_UNITTEST_SOURCES
                version/version.cpp
                version/static.cpp
                version/parameterized.cpp
            )
        # Если не нужно собрать тесты для работы с версионированием
        else()
            # Выводим сообщение о сборке тестов для модуля работы с версионированием
            message(STATUS "Build tests VERSION module: No\n")
        endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_VERSION_TEST})

        # Если нужно собрать тесты для работы с криптографией
        if (${CMAKE_BUILD_${UNIT_TEST_NAME}_CRYPTO_TEST})
            # Выводим сообщение о сборке тестов для модуля работы с криптографией
            message(STATUS "Build tests CRYPTO module: Yes\n")
            # Дополняем список источников тестами для модуля работы с криптографией
            list(APPEND ${PROJECT_NAME}_UNITTEST_SOURCES
                crypto/crypto.cpp
                crypto/parameterized.cpp
            )
        # Если не нужно собрать тесты для работы с криптографией
        else()
            # Выводим сообщение о сборке тестов для модуля работы с криптографией
            message(STATUS "Build tests CRYPTO module: No\n")
        endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_CRYPTO_TEST})

        # Если нужно собрать тесты для работы с компрессором
        if (${CMAKE_BUILD_${UNIT_TEST_NAME}_COMPRESSOR_TEST})
            # Выводим сообщение о сборке тестов для модуля работы с компрессором
            message(STATUS "Build tests COMPRESSOR module: Yes\n")
            # Дополняем список источников тестами для модуля работы с компрессором
            list(APPEND ${PROJECT_NAME}_UNITTEST_SOURCES
                compressor/compressor.cpp
                compressor/parameterized.cpp
                compressor/stream.cpp
            )
        # Если не нужно собрать тесты для работы с компрессором
        else()
            # Выводим сообщение о сборке тестов для модуля работы с компрессором
            message(STATUS "Build tests COMPRESSOR module: No\n")
        endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_COMPRESSOR_TEST})

        # Если нужно собрать тесты для работы с пуллам потоков
        if (${CMAKE_BUILD_${UNIT_TEST_NAME}_THREADPOOL_TEST})
            # Выводим сообщение о сборке тестов для модуля работы с пуллам потоков
            message(STATUS "Build tests THREADPOOL module: Yes\n")
            # Дополняем список источников тестами для модуля работы с пуллам потоков
            list(APPEND ${PROJECT_NAME}_UNITTEST_SOURCES
                threadpool/threadpool.cpp
                threadpool/static.cpp
                threadpool/parameterized.cpp
            )
        # Если не нужно собрать тесты для работы с пуллам потоков
        else()
            # Выводим сообщение о сборке тестов для модуля работы с пуллам потоков
            message(STATUS "Build tests THREADPOOL module: No\n")
        endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_THREADPOOL_TEST})

        # Если нужно собрать тесты для работы с файловой системой
        if (${CMAKE_BUILD_${UNIT_TEST_NAME}_FS_TEST})
            # Выводим сообщение о сборке тестов для модуля работы с файловой системой
            message(STATUS "Build tests FS module: Yes\n")
            # Дополняем список источников тестами для модуля работы с файловой системой
            list(APPEND ${PROJECT_NAME}_UNITTEST_SOURCES
                fs/fs.cpp
                fs/static.cpp
                fs/parameterized.cpp
            )
        # Если не нужно собрать тесты для работы с файловой системой
        else()
            # Выводим сообщение о сборке тестов для модуля работы с файловой системой
            message(STATUS "Build tests FS module: No\n")
        endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_FS_TEST})

        # Если нужно собрать тесты для работы с функциями обратного вызова
        if (${CMAKE_BUILD_${UNIT_TEST_NAME}_CALLBACK_TEST})
            # Выводим сообщение о сборке тестов для модуля работы с функциями обратного вызова
            message(STATUS "Build tests CALLBACK module: Yes\n")
            # Дополняем список источников тестами для модуля работы с функциями обратного вызова
            list(APPEND ${PROJECT_NAME}_UNITTEST_SOURCES
                callback/callback.cpp
                callback/static.cpp
                callback/parameterized.cpp
            )
        # Если не нужно собрать тесты для работы с функциями обратного вызова
        else()
            # Выводим сообщение о сборке тестов для модуля работы с функциями обратного вызова
            message(STATUS "Build tests CALLBACK module: No\n")
        endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_CALLBACK_TEST})

        # Если нужно собрать тесты для работы с процессами
        if (${CMAKE_BUILD_${UNIT_TEST_NAME}_PROCRE_TEST})
            # Выводим сообщение о сборке тестов для модуля работы с процессами
            message(STATUS "Build tests PROCRE module: Yes\n")
            # Дополняем список источников тестами для модуля работы с процессами
            list(APPEND ${PROJECT_NAME}_UNITTEST_SOURCES
                procre/procre.cpp
                procre/static.cpp
                procre/parameterized.cpp
            )
        # Если не нужно собрать тесты для работы с процессами
        else()
            # Выводим сообщение о сборке тестов для модуля работы с процессами
            message(STATUS "Build tests PROCRE module: No\n")
        endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_PROCRE_TEST})

        # Если нужно собрать тесты для работы с сигналами
        if (${CMAKE_BUILD_${UNIT_TEST_NAME}_SIGNALS_TEST})
            # Выводим сообщение о сборке тестов для модуля работы с сигналами
            message(STATUS "Build tests SIGNALS module: Yes\n")
            # Дополняем список источников тестами для модуля работы с сигналами
            list(APPEND ${PROJECT_NAME}_UNITTEST_SOURCES
                signals/signals.cpp
                signals/static.cpp
                signals/parameterized.cpp
            )
        # Если не нужно собрать тесты для работы с сигналами
        else()
            # Выводим сообщение о сборке тестов для модуля работы с сигналами
            message(STATUS "Build tests SIGNALS module: No\n")
        endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_SIGNALS_TEST})

        # Если нужно собрать тесты для работы с длинными числами
        if (${CMAKE_BUILD_${UNIT_TEST_NAME}_BIGNUM_TEST})
            # Выводим сообщение о сборке тестов для модуля работы с длинными числами
            message(STATUS "Build tests BIGNUM module: Yes\n")
            # Дополняем список источников тестами для модуля работы с длинными числами
            list(APPEND ${PROJECT_NAME}_UNITTEST_SOURCES
                bignum/bignum.cpp
                bignum/static.cpp
                bignum/real.cpp
                bignum/strings.cpp
                bignum/rounding.cpp
                bignum/stress.cpp
                bignum/parameterized.cpp
            )
        # Если не нужно собрать тесты для работы с длинными числами
        else()
            # Выводим сообщение о сборке тестов для модуля работы с длинными числами
            message(STATUS "Build tests BIGNUM module: No\n")
        endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_BIGNUM_TEST})

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
