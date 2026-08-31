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
    SET(NAME "regex")

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

        # Выводим сообщение о сборке тестов для модуля lexical
        message(STATUS "Build tests REGEX module: Yes\n")

        # Источники для сборки юнит-тестов
        SET(${PROJECT_NAME}_UNITTEST_SOURCES
            ../main.cpp
            static.cpp
            matching.cpp
            automaton.cpp
            assembly.cpp
            emitter.cpp
            codegen.cpp
            engine.cpp
            reference.cpp
            interface.cpp
            storage.cpp
            grok.cpp
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

        # Выполняем поиск эталонной реализации в системе
        #
        # Зависимостью библиотеки эталон не является - модуль выражений его
        # собою и заменил, - отчего переменные его пусты, а сличение с ним
        # отменялось препроцессором молча. Пропуск молчаливый неотличим
        # от прохождения: расхождение по захвату прохода пустого жило
        # незамеченным именно оттого. Ищется эталон в системе, и найденным
        # сличение включается само; ненайденным - проверки пропускаются
        # с причиною, как и прежде.
        # Имена переменных берутся свои намеренно: «PCRE_LIBRARY» присоединяется
        # безусловно и целями иными (проверки сети в том числе), и значение
        # «NOTFOUND», поиском в кэш записанное, сломало бы им сборку на всякой
        # машине, эталона не имеющей. Пустая же переменная им безобидна.
        #
        # Источник эталона у сличения ОДИН с набором замеров - подмодуль дерева.
        # Собирается он общим сценарием, и пути его передаются сюда явно:
        #   sh/reference/pcre2.sh                     # печатает две строки
        #   cmake .. -DAWH_REFERENCE_INCLUDE_DIR=<первая> -DAWH_REFERENCE_LIBRARY=<вторая>
        # Поиск же в системе оставлен запасным путём: версия системного эталона
        # случайна, и расхождение с нею пришлось бы разбирать всякий раз заново,
        # гадая - наш ли это изъян или иное поведение иной версии соперника.
        if (NOT AWH_REFERENCE_INCLUDE_DIR)
            # Выполняем поиск заголовочного файла эталонной реализации
            find_path(AWH_REFERENCE_INCLUDE_DIR NAMES pcre2.h)

            # Выполняем поиск библиотеки эталонной реализации
            find_library(AWH_REFERENCE_LIBRARY NAMES pcre2-8)
        endif()

        # Если эталонная реализация сборке доступна
        #
        # Присоединяется эталон ПЕРЕД библиотеками прочими намеренно: среди них
        # стоит «PCRE_LIBRARY» - библиотека системная, к цели присоединяемая
        # наследием, - и связывание берёт из двух ту, что в строке раньше.
        # Оттого сличение шло с системной 10.42, тогда как заголовки брались
        # от подмодуля 10.48: правила разбора у изданий разные, и расхождение
        # выходило не нашим изъяном, а разницей изданий. Несогласие заголовка
        # с библиотекой опасно и само по себе.
        if (AWH_REFERENCE_LIBRARY AND AWH_REFERENCE_INCLUDE_DIR)
            # Присоединяем библиотеку эталонной реализации к проверкам
            target_link_libraries(${PROJECT_NAME}${POSTFIX} PUBLIC ${AWH_REFERENCE_LIBRARY})
        endif()

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

        # Если эталонная реализация сборке доступна
        #
        # Сличение с эталоном пропускалось молча: библиотека эталона к цели
        # присоединялась, а признак сборки не задавался, отчего обе проверки
        # сличения отменялись препроцессором. Пропуск молчаливый неотличим
        # от прохождения, и расхождение по захвату прохода пустого жило
        # незамеченным именно оттого.
        if (AWH_REFERENCE_LIBRARY AND AWH_REFERENCE_INCLUDE_DIR)
            # Добавляем каталог заголовочных файлов эталонной реализации
            target_include_directories(${PROJECT_NAME}${POSTFIX} PRIVATE ${AWH_REFERENCE_INCLUDE_DIR})

            # Включаем признак сборки со сличением с эталонной реализацией
            target_compile_definitions(${PROJECT_NAME}${POSTFIX} PRIVATE AWH_TEST_PCRE2)
        endif()

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
        # Выводим сообщение о сборке тестов для модуля lexical
        message(STATUS "Build tests REGEX module: No\n")
    endif (${CMAKE_BUILD_${UNIT_TEST_NAME}_TESTS})
endif()
