# Отключаем поиск в системных каталогах
SET(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)

# Флаг активации системной установки
SET(AHW_GLOBAL_INSTALLATION FALSE)

# Если операцинная система относится к MS Windows
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    # Если произведена установка глобально
    if (AHW_GLOBAL_INSTALLATION)
        # Ищем через переменную окружения
        if(DEFINED ENV{MSYS2_ROOT})
            # Устанавливаем системный путь поиска зависимостей
            SET(AWH_SEARCH_PATH "$ENV{MSYS2_ROOT}/usr")
        elseif(DEFINED ENV{MINGW_PREFIX})
            # Устанавливаем системный путь поиска зависимостей
            SET(AWH_SEARCH_PATH "$ENV{MINGW_PREFIX}/../usr")
        endif()
        # Выполняем формирование пути поиска исполняемых файлов
        SET(AHW_BIN_PATH ${AWH_SEARCH_PATH}/bin)
        # Выполняем формирование пути поиска заголовков
        SET(AHW_HEADERS_PATH ${AWH_SEARCH_PATH}/include/libawh)
    # Если произведена локальная установка
    else (AHW_GLOBAL_INSTALLATION)
        # Устанавливаем системный путь поиска зависимостей
        SET(AWH_SEARCH_PATH ${CMAKE_SOURCE_DIR}/third_party)
        # Выполняем формирование пути поиска исполняемых файлов
        SET(AHW_BIN_PATH ${AWH_SEARCH_PATH}/bin/awh)
        # Выполняем формирование пути поиска заголовков
        SET(AHW_HEADERS_PATH ${AWH_SEARCH_PATH}/include)
    endif (AHW_GLOBAL_INSTALLATION)
    # Выполняем формирование пути поиска библиотек
    SET(AHW_LIBRARY_PATH ${AWH_SEARCH_PATH}/lib)
# Если операцинная система относится к Nix-подобной
else()
    # Если произведена установка глобально
    if (AHW_GLOBAL_INSTALLATION)
        # Если операцинная система относится к MacOS X
        if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
            # Устанавливаем системный путь поиска зависимостей
            SET(AWH_SEARCH_PATH /usr/local)
            # Выполняем формирование пути поиска библиотек
            SET(AHW_LIBRARY_PATH ${AWH_SEARCH_PATH}/lib)
        # Если операцинная система относится к Solaris
        elseif (${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
            # Устанавливаем системный путь поиска зависимостей
            SET(AWH_SEARCH_PATH /usr)
            # Выполняем формирование пути поиска библиотек
            SET(AHW_LIBRARY_PATH ${AWH_SEARCH_PATH}/lib/amd64)
       # Если операцинная система относится к Linux
        elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
            # Устанавливаем системный путь поиска зависимостей
            SET(AWH_SEARCH_PATH /usr)
            # Выполняем формирование пути поиска библиотек
            SET(AHW_LIBRARY_PATH ${AWH_SEARCH_PATH}/lib)
        # Если операцинная система относится к FreeBSD, NetBSD и OpenBSD
        elseif (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "NetBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
            # Устанавливаем системный путь поиска зависимостей
            SET(AWH_SEARCH_PATH /usr/local)
            # Выполняем формирование пути поиска библиотек
            SET(AHW_LIBRARY_PATH ${AWH_SEARCH_PATH}/lib)
        # Если операционная система не определена
        else()
            # Выводим сообщение, что операционная система не поддерживается
            message(FATAL_ERROR "Operating system is not supported")
        endif()
        # Выполняем формирование пути поиска заголовков
        SET(AHW_HEADERS_PATH ${AWH_SEARCH_PATH}/include/libawh)
    # Если произведена локальная установка
    else (AHW_GLOBAL_INSTALLATION)
        # Устанавливаем системный путь поиска зависимостей
        SET(AWH_SEARCH_PATH ${CMAKE_SOURCE_DIR}/third_party)
        # Выполняем формирование пути поиска библиотек
        SET(AHW_LIBRARY_PATH ${AWH_SEARCH_PATH}/lib)
        # Выполняем формирование пути поиска заголовков
        SET(AHW_HEADERS_PATH ${AWH_SEARCH_PATH}/include)
    endif (AHW_GLOBAL_INSTALLATION)
endif()

# Поиск пути к заголовочным файлам
find_path(LZ4_INCLUDE_DIR NAMES lz4.h PATHS ${AHW_HEADERS_PATH}/lz4 NO_DEFAULT_PATH)
find_path(BZ2_INCLUDE_DIR NAMES bzlib.h PATHS ${AHW_HEADERS_PATH}/bz2 NO_DEFAULT_PATH)
find_path(ZSTD_INCLUDE_DIR NAMES zstd.h PATHS ${AHW_HEADERS_PATH}/zstd NO_DEFAULT_PATH)
find_path(LZMA_INCLUDE_DIR NAMES lzma.h PATHS ${AHW_HEADERS_PATH}/lzma NO_DEFAULT_PATH)
find_path(ZLIB_INCLUDE_DIR NAMES zlib.h PATHS ${AHW_HEADERS_PATH}/zlib NO_DEFAULT_PATH)
find_path(CITY_INCLUDE_DIR NAMES cityhash/city.h PATHS ${AHW_HEADERS_PATH} NO_DEFAULT_PATH)
find_path(AWH_INCLUDE_DIR NAMES awh/server/awh.hpp PATHS ${AHW_HEADERS_PATH} NO_DEFAULT_PATH)
find_path(BROTLI_INCLUDE_ENCODE_DIR NAMES encode.h PATHS ${AHW_HEADERS_PATH}/brotli NO_DEFAULT_PATH)
find_path(BROTLI_INCLUDE_DECODE_DIR NAMES decode.h PATHS ${AHW_HEADERS_PATH}/brotli NO_DEFAULT_PATH)
find_path(OPENSSL_INCLUDE_DIR NAMES openssl/opensslconf.h PATHS ${AHW_HEADERS_PATH} NO_DEFAULT_PATH)
find_path(PCRE_INCLUDE_DIR NAMES pcre2.h PATHS ${AHW_HEADERS_PATH}/pcre2 NO_DEFAULT_PATH)
find_path(NGHTTP2_INCLUDE_DIR NAMES nghttp2.h PATHS ${AHW_HEADERS_PATH}/nghttp2 NO_DEFAULT_PATH)

# Сборка модуля AWH_IDN, если операционной системой не является Windows
if (CMAKE_BUILD_IDN AND (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows"))
    find_path(IDN2_INCLUDE_DIR NAMES idn2.h PATHS ${AHW_HEADERS_PATH}/idn2 NO_DEFAULT_PATH)
    find_path(ICONV_INCLUDE_DIR NAMES iconv.h PATHS ${AHW_HEADERS_PATH}/iconv NO_DEFAULT_PATH)
endif()

# Если операцинная система относится к MS Windows
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    # Устанавливаем префикс поиска библиотеки
    SET(CMAKE_FIND_LIBRARY_PREFIXES "lib")
    # Если нужно загрузить динамическую библиотеку
    if (CMAKE_SHARED_LIB_AWH)
        # Устанавливаем расширение поиска библиотеки
        SET(CMAKE_FIND_LIBRARY_SUFFIXES ".dll.a")
        # Ищем саму DLL
        find_file(AWH_LIBRARY_DLL NAMES libawh.dll PATHS ${AHW_BIN_PATH} NO_DEFAULT_PATH)
        # Поиск библиотеки AWH
        find_library(AWH_LIBRARY NAMES awh PATHS ${AHW_LIBRARY_PATH} NO_DEFAULT_PATH)
    # Если нужно загрузить статическую библиотеку
    else (CMAKE_SHARED_LIB_AWH)
        # Устанавливаем расширение поиска библиотеки
        SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
        # Поиск библиотеки AWH
        find_library(AWH_LIBRARY NAMES awh PATHS ${AHW_LIBRARY_PATH} NO_DEFAULT_PATH)
    endif (CMAKE_SHARED_LIB_AWH)
# Если операцинная система относится к Nix-подобной
else()
    # Если нужно загрузить динамическую библиотеку
    if (CMAKE_SHARED_LIB_AWH)
        # Устанавливаем расширение поиска библиотеки
        SET(CMAKE_FIND_LIBRARY_SUFFIXES ".so")
    # Если нужно загрузить статическую библиотеку
    else (CMAKE_SHARED_LIB_AWH)
        # Устанавливаем расширение поиска библиотеки
        SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    endif (CMAKE_SHARED_LIB_AWH)
    # Поиск библиотеки AWH
    find_library(AWH_LIBRARY NAMES awh PATHS ${AHW_LIBRARY_PATH} NO_DEFAULT_PATH)
endif()

# Если активирован режим отладки
if (CMAKE_AWH_BUILD_DEBUG)
    # Поиск библиотеки AWH
    find_library(DEPEND_LIBRARY NAMES dependence PATHS ${AHW_LIBRARY_PATH} NO_DEFAULT_PATH)
endif()

# Подключаем 'FindPackageHandle' для использования модуля поиска (find_package(<PackageName>))
include(FindPackageHandleStandardArgs)

# Сборка модуля AWH_IDN, если операционной системой не является Windows
if (CMAKE_BUILD_IDN AND (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows"))
    # Если активирован режим отладки
    if (CMAKE_AWH_BUILD_DEBUG)
        # Выполняем проверку на существование зависимостей
        find_package_handle_standard_args(AWH REQUIRED_VARS
            DEPEND_LIBRARY
            AWH_LIBRARY
            LZ4_INCLUDE_DIR
            BZ2_INCLUDE_DIR
            ZSTD_INCLUDE_DIR
            LZMA_INCLUDE_DIR
            ZLIB_INCLUDE_DIR
            BROTLI_INCLUDE_ENCODE_DIR
            BROTLI_INCLUDE_DECODE_DIR
            AWH_INCLUDE_DIR
            CITY_INCLUDE_DIR
            OPENSSL_INCLUDE_DIR
            PCRE_INCLUDE_DIR
            NGHTTP2_INCLUDE_DIR
            IDN2_INCLUDE_DIR
            ICONV_INCLUDE_DIR

            FAIL_MESSAGE "AWH library is not found"
        )
        # Формируем список библиотек
        SET(AWH_LIBRARIES ${DEPEND_LIBRARY} ${AWH_LIBRARY})
    # Если режим отладки не активирован
    else()
        # Выполняем проверку на существование зависимостей
        find_package_handle_standard_args(AWH REQUIRED_VARS
            AWH_LIBRARY
            LZ4_INCLUDE_DIR
            BZ2_INCLUDE_DIR
            ZSTD_INCLUDE_DIR
            LZMA_INCLUDE_DIR
            ZLIB_INCLUDE_DIR
            BROTLI_INCLUDE_ENCODE_DIR
            BROTLI_INCLUDE_DECODE_DIR
            AWH_INCLUDE_DIR
            CITY_INCLUDE_DIR
            OPENSSL_INCLUDE_DIR
            PCRE_INCLUDE_DIR
            NGHTTP2_INCLUDE_DIR
            IDN2_INCLUDE_DIR
            ICONV_INCLUDE_DIR

            FAIL_MESSAGE "AWH library is not found"
        )
        # Формируем список библиотек
        SET(AWH_LIBRARIES ${AWH_LIBRARY})
    endif()
    # Формируем список заголовочных файлов
    SET(AWH_INCLUDE_DIRS
        ${LZ4_INCLUDE_DIR}
        ${BZ2_INCLUDE_DIR}
        ${ZSTD_INCLUDE_DIR}
        ${LZMA_INCLUDE_DIR}
        ${ZLIB_INCLUDE_DIR}
        ${BROTLI_INCLUDE_ENCODE_DIR}
        ${AWH_INCLUDE_DIR}
        ${CITY_INCLUDE_DIR}
        ${OPENSSL_INCLUDE_DIR}
        ${PCRE_INCLUDE_DIR}
        ${NGHTTP2_INCLUDE_DIR}
        ${IDN2_INCLUDE_DIR}
        ${ICONV_INCLUDE_DIR}
    )
    # Выполняем установку указанного списка заголовочных файлов зависимостей
    install(DIRECTORY "${IDN2_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
    install(DIRECTORY "${ICONV_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
# Если операцинная система относится к MS Windows
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    # Если активирован режим отладки
    if (CMAKE_AWH_BUILD_DEBUG)
        # Выполняем проверку на существование зависимостей
        find_package_handle_standard_args(AWH REQUIRED_VARS
            DEPEND_LIBRARY
            AWH_LIBRARY
            LZ4_INCLUDE_DIR
            BZ2_INCLUDE_DIR
            ZSTD_INCLUDE_DIR
            LZMA_INCLUDE_DIR
            ZLIB_INCLUDE_DIR
            BROTLI_INCLUDE_ENCODE_DIR
            BROTLI_INCLUDE_DECODE_DIR
            AWH_INCLUDE_DIR
            CITY_INCLUDE_DIR
            OPENSSL_INCLUDE_DIR
            PCRE_INCLUDE_DIR
            NGHTTP2_INCLUDE_DIR

            FAIL_MESSAGE "AWH library is not found"
        )
        # Формируем список библиотек
        SET(AWH_LIBRARIES ${DEPEND_LIBRARY} ${AWH_LIBRARY})
    # Если режим отладки не активирован
    else()
        # Если нужно загрузить динамическую библиотеку
        if (CMAKE_SHARED_LIB_AWH)
            # Выполняем проверку на существование зависимостей
            find_package_handle_standard_args(AWH REQUIRED_VARS
                AWH_LIBRARY
                AWH_LIBRARY_DLL
                LZ4_INCLUDE_DIR
                BZ2_INCLUDE_DIR
                ZSTD_INCLUDE_DIR
                LZMA_INCLUDE_DIR
                ZLIB_INCLUDE_DIR
                BROTLI_INCLUDE_ENCODE_DIR
                BROTLI_INCLUDE_DECODE_DIR
                AWH_INCLUDE_DIR
                CITY_INCLUDE_DIR
                OPENSSL_INCLUDE_DIR
                PCRE_INCLUDE_DIR
                NGHTTP2_INCLUDE_DIR

                FAIL_MESSAGE "AWH library is not found"
            )
        # Если нужно загрузить статическую библиотеку
        else (CMAKE_SHARED_LIB_AWH)
            # Выполняем проверку на существование зависимостей
            find_package_handle_standard_args(AWH REQUIRED_VARS
                AWH_LIBRARY
                LZ4_INCLUDE_DIR
                BZ2_INCLUDE_DIR
                ZSTD_INCLUDE_DIR
                LZMA_INCLUDE_DIR
                ZLIB_INCLUDE_DIR
                BROTLI_INCLUDE_ENCODE_DIR
                BROTLI_INCLUDE_DECODE_DIR
                AWH_INCLUDE_DIR
                CITY_INCLUDE_DIR
                OPENSSL_INCLUDE_DIR
                PCRE_INCLUDE_DIR
                NGHTTP2_INCLUDE_DIR

                FAIL_MESSAGE "AWH library is not found"
            )
        endif (CMAKE_SHARED_LIB_AWH)
        # Формируем список библиотек
        SET(AWH_LIBRARIES ${AWH_LIBRARY})
    endif()
    # Формируем список заголовочных файлов
    SET(AWH_INCLUDE_DIRS
        ${LZ4_INCLUDE_DIR}
        ${BZ2_INCLUDE_DIR}
        ${ZSTD_INCLUDE_DIR}
        ${LZMA_INCLUDE_DIR}
        ${ZLIB_INCLUDE_DIR}
        ${BROTLI_INCLUDE_ENCODE_DIR}
        ${AWH_INCLUDE_DIR}
        ${CITY_INCLUDE_DIR}
        ${OPENSSL_INCLUDE_DIR}
        ${PCRE_INCLUDE_DIR}
        ${NGHTTP2_INCLUDE_DIR}
    )
# Если операцинная система относится к Nix-подобной
else()
    # Если активирован режим отладки
    if (CMAKE_AWH_BUILD_DEBUG)
        # Выполняем проверку на существование зависимостей
        find_package_handle_standard_args(AWH REQUIRED_VARS
            DEPEND_LIBRARY
            AWH_LIBRARY
            LZ4_INCLUDE_DIR
            BZ2_INCLUDE_DIR
            ZSTD_INCLUDE_DIR
            LZMA_INCLUDE_DIR
            ZLIB_INCLUDE_DIR
            BROTLI_INCLUDE_ENCODE_DIR
            BROTLI_INCLUDE_DECODE_DIR
            AWH_INCLUDE_DIR
            CITY_INCLUDE_DIR
            OPENSSL_INCLUDE_DIR
            PCRE_INCLUDE_DIR
            NGHTTP2_INCLUDE_DIR

            FAIL_MESSAGE "AWH library is not found"
        )
        # Формируем список библиотек
        SET(AWH_LIBRARIES ${DEPEND_LIBRARY} ${AWH_LIBRARY})
    # Если режим отладки не активирован
    else()
        # Выполняем проверку на существование зависимостей
        find_package_handle_standard_args(AWH REQUIRED_VARS
            AWH_LIBRARY
            LZ4_INCLUDE_DIR
            BZ2_INCLUDE_DIR
            ZSTD_INCLUDE_DIR
            LZMA_INCLUDE_DIR
            ZLIB_INCLUDE_DIR
            BROTLI_INCLUDE_ENCODE_DIR
            BROTLI_INCLUDE_DECODE_DIR
            AWH_INCLUDE_DIR
            CITY_INCLUDE_DIR
            OPENSSL_INCLUDE_DIR
            PCRE_INCLUDE_DIR
            NGHTTP2_INCLUDE_DIR

            FAIL_MESSAGE "AWH library is not found"
        )
        # Формируем список библиотек
        SET(AWH_LIBRARIES ${AWH_LIBRARY})
    endif()
    # Формируем список заголовочных файлов
    SET(AWH_INCLUDE_DIRS
        ${LZ4_INCLUDE_DIR}
        ${BZ2_INCLUDE_DIR}
        ${ZSTD_INCLUDE_DIR}
        ${LZMA_INCLUDE_DIR}
        ${ZLIB_INCLUDE_DIR}
        ${BROTLI_INCLUDE_ENCODE_DIR}
        ${AWH_INCLUDE_DIR}
        ${CITY_INCLUDE_DIR}
        ${OPENSSL_INCLUDE_DIR}
        ${PCRE_INCLUDE_DIR}
        ${NGHTTP2_INCLUDE_DIR}
    )
endif()

# Если операцинная система относится к MS Windows
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    # Если нужно загрузить динамическую библиотеку
    if (CMAKE_SHARED_LIB_AWH)
        # Устанавливаем библиотеку AWH
        install(FILES ${AWH_LIBRARY} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
        # Устанавливаем библиотеку AWH DLL
        install(FILES ${AWH_LIBRARY_DLL} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
    # Если нужно загрузить статическую библиотеку
    else (CMAKE_SHARED_LIB_AWH)
        # Устанавливаем библиотеку AWH
        install(FILES ${AWH_LIBRARY} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
    endif (CMAKE_SHARED_LIB_AWH)
# Если операцинная система относится к Nix-подобной
else()
    # Устанавливаем библиотеку AWH
    install(FILES ${AWH_LIBRARY} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
endif()

# Если активирован режим отладки
if (CMAKE_AWH_BUILD_DEBUG)
    # Устанавливаем статическую библиотеку
    install(FILES ${DEPEND_LIBRARY} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
endif()

# Выполняем установку оставшихся заголовочных файлов зависимостей
install(DIRECTORY "${LZ4_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${BZ2_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${ZSTD_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${LZMA_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${ZLIB_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${PCRE_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${NGHTTP2_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${AWH_INCLUDE_DIR}/awh" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY "${BROTLI_INCLUDE_ENCODE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${CITY_INCLUDE_DIR}/cityhash" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${OPENSSL_INCLUDE_DIR}/openssl" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
