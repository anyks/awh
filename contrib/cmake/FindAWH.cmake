set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)

# Если операцинная система относится к MS Windows
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(CMAKE_FIND_LIBRARY_PREFIXES "lib")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
endif()

# Поиск пути к заголовочным файлам
find_path(LZ4_INCLUDE_DIR NAMES lz4.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/lz4 NO_DEFAULT_PATH)
find_path(BZ2_INCLUDE_DIR NAMES bzlib.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/bz2 NO_DEFAULT_PATH)
find_path(ZSTD_INCLUDE_DIR NAMES zstd.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/zstd NO_DEFAULT_PATH)
find_path(LZMA_INCLUDE_DIR NAMES lzma.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/lzma NO_DEFAULT_PATH)
find_path(ZLIB_INCLUDE_DIR NAMES zlib.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/zlib NO_DEFAULT_PATH)
find_path(CITY_INCLUDE_DIR NAMES city.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/cityhash NO_DEFAULT_PATH)
find_path(JSON_INCLUDE_DIR NAMES json.hpp PATHS ${CMAKE_SOURCE_DIR}/third_party/include/nlohmann NO_DEFAULT_PATH)
find_path(AWH_INCLUDE_DIR NAMES server/awh.hpp PATHS ${CMAKE_SOURCE_DIR}/third_party/include/awh NO_DEFAULT_PATH)
find_path(BROTLI_INCLUDE_ENCODE_DIR NAMES encode.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/brotli NO_DEFAULT_PATH)
find_path(BROTLI_INCLUDE_DECODE_DIR NAMES decode.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/brotli NO_DEFAULT_PATH)
find_path(OPENSSL_INCLUDE_DIR NAMES openssl/opensslconf.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include NO_DEFAULT_PATH)
find_path(PCRE_INCLUDE_DIR NAMES pcre2.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/pcre2 NO_DEFAULT_PATH)
find_path(NGTCP2_INCLUDE_DIR NAMES ngtcp2.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/ngtcp2 NO_DEFAULT_PATH)
find_path(NGHTTP2_INCLUDE_DIR NAMES nghttp2.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/nghttp2 NO_DEFAULT_PATH)
find_path(NGHTTP3_INCLUDE_DIR NAMES nghttp3.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/nghttp3 NO_DEFAULT_PATH)
find_path(XML_INCLUDE_DIR NAMES libxml/xmlmemory.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/libxml2 NO_DEFAULT_PATH)

# Если операцинная система не относится к MS Windows
if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    find_path(JEMALLOC_INCLUDE_DIR NAMES jemalloc.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/jemalloc NO_DEFAULT_PATH)
endif()

# Сборка модуля AWH_IDN, если операционной системой не является Windows
if(CMAKE_BUILD_IDN AND (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows"))
    find_path(IDN2_INCLUDE_DIR NAMES idn2.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/idn2 NO_DEFAULT_PATH)
    find_path(ICONV_INCLUDE_DIR NAMES iconv.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/iconv NO_DEFAULT_PATH)
endif()

# Сборка модуля AWH_EVENT2
if(CMAKE_BUILD_EVENT2)
    add_compile_definitions(AWH_EVENT2)
    find_path(LIBEVENT_INCLUDE_DIR NAMES event2/event.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/libevent2 NO_DEFAULT_PATH)
# Сборка модуля AWH_EV
else(CMAKE_BUILD_EVENT2)
    add_compile_definitions(AWH_EV)
    find_path(LIBEV_EV_INCLUDE_DIR NAMES libev/ev.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include NO_DEFAULT_PATH)
    find_path(LIBEV_EVPP_INCLUDE_DIR NAMES libev/ev++.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include NO_DEFAULT_PATH)
    find_path(LIBEV_EVENT_INCLUDE_DIR NAMES libev/event.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include NO_DEFAULT_PATH)
endif(CMAKE_BUILD_EVENT2)

# Поиск библиотеки AWH
find_library(AWH_LIBRARY NAMES awh PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)

# Если активирован режим отладки
if(CMAKE_AWH_BUILD_DEBUG)
    # Поиск библиотеки AWH
    find_library(DEPEND_LIBRARY NAMES dependence PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)
endif()

# Подключаем 'FindPackageHandle' для использования модуля поиска (find_package(<PackageName>))
include(FindPackageHandleStandardArgs)

# Сборка модуля AWH_EVENT2
if(CMAKE_BUILD_EVENT2)
    # Сборка модуля AWH_IDN, если операционной системой не является Windows
    if(CMAKE_BUILD_IDN AND (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows"))
        # Если активирован режим отладки
        if(CMAKE_AWH_BUILD_DEBUG)
            # Выполняем проверку на существование зависимостей
            find_package_handle_standard_args(AWH REQUIRED_VARS
                AWH_LIBRARY
                DEPEND_LIBRARY
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
                JEMALLOC_INCLUDE_DIR
                NGTCP2_INCLUDE_DIR
                NGHTTP2_INCLUDE_DIR
                NGHTTP3_INCLUDE_DIR
                XML_INCLUDE_DIR
                JSON_INCLUDE_DIR
                IDN2_INCLUDE_DIR
                ICONV_INCLUDE_DIR
                LIBEVENT_INCLUDE_DIR

                FAIL_MESSAGE "AWH library is not found"
            )
            # Формируем список библиотек
            set(AWH_LIBRARIES ${AWH_LIBRARY} ${DEPEND_LIBRARY})
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
                JEMALLOC_INCLUDE_DIR
                NGTCP2_INCLUDE_DIR
                NGHTTP2_INCLUDE_DIR
                NGHTTP3_INCLUDE_DIR
                XML_INCLUDE_DIR
                JSON_INCLUDE_DIR
                IDN2_INCLUDE_DIR
                ICONV_INCLUDE_DIR
                LIBEVENT_INCLUDE_DIR

                FAIL_MESSAGE "AWH library is not found"
            )
            # Формируем список библиотек
            set(AWH_LIBRARIES ${AWH_LIBRARY})
        endif()
        # Формируем список заголовочных файлов
        set(AWH_INCLUDE_DIRS
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
            ${JEMALLOC_INCLUDE_DIR}
            ${NGTCP2_INCLUDE_DIR}
            ${NGHTTP2_INCLUDE_DIR}
            ${NGHTTP3_INCLUDE_DIR}
            ${XML_INCLUDE_DIR}
            ${JSON_INCLUDE_DIR}
            ${IDN2_INCLUDE_DIR}
            ${ICONV_INCLUDE_DIR}
            ${LIBEVENT_INCLUDE_DIR}
        )
        # Выполняем установку указанного списка заголовочных файлов зависимостей
        install(DIRECTORY "${IDN2_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
        install(DIRECTORY "${ICONV_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
        install(DIRECTORY "${LIBEVENT_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
    # Если операцинная система относится к MS Windows
    elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
        # Если активирован режим отладки
        if(CMAKE_AWH_BUILD_DEBUG)
            # Выполняем проверку на существование зависимостей
            find_package_handle_standard_args(AWH REQUIRED_VARS
                AWH_LIBRARY
                DEPEND_LIBRARY
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
                NGTCP2_INCLUDE_DIR
                NGHTTP2_INCLUDE_DIR
                NGHTTP3_INCLUDE_DIR
                XML_INCLUDE_DIR
                JSON_INCLUDE_DIR
                LIBEVENT_INCLUDE_DIR

                FAIL_MESSAGE "AWH library is not found"
            )
            # Формируем список библиотек
            set(AWH_LIBRARIES ${AWH_LIBRARY} ${DEPEND_LIBRARY})
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
                NGTCP2_INCLUDE_DIR
                NGHTTP2_INCLUDE_DIR
                NGHTTP3_INCLUDE_DIR
                XML_INCLUDE_DIR
                JSON_INCLUDE_DIR
                LIBEVENT_INCLUDE_DIR

                FAIL_MESSAGE "AWH library is not found"
            )
            # Формируем список библиотек
            set(AWH_LIBRARIES ${AWH_LIBRARY})
        endif()
        # Формируем список заголовочных файлов
        set(AWH_INCLUDE_DIRS
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
            ${NGTCP2_INCLUDE_DIR}
            ${NGHTTP2_INCLUDE_DIR}
            ${NGHTTP3_INCLUDE_DIR}
            ${XML_INCLUDE_DIR}
            ${JSON_INCLUDE_DIR}
            ${LIBEVENT_INCLUDE_DIR}
        )
        # Выполняем установку указанного списка заголовочных файлов зависимостей
        install(DIRECTORY "${LIBEVENT_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
    # Если операцинная система относится к Nix-подобной
    else()
        # Если активирован режим отладки
        if(CMAKE_AWH_BUILD_DEBUG)
            # Выполняем проверку на существование зависимостей
            find_package_handle_standard_args(AWH REQUIRED_VARS
                AWH_LIBRARY
                DEPEND_LIBRARY
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
                JEMALLOC_INCLUDE_DIR
                NGTCP2_INCLUDE_DIR
                NGHTTP2_INCLUDE_DIR
                NGHTTP3_INCLUDE_DIR
                XML_INCLUDE_DIR
                JSON_INCLUDE_DIR
                LIBEVENT_INCLUDE_DIR

                FAIL_MESSAGE "AWH library is not found"
            )
            # Формируем список библиотек
            set(AWH_LIBRARIES ${AWH_LIBRARY} ${DEPEND_LIBRARY})
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
                JEMALLOC_INCLUDE_DIR
                NGTCP2_INCLUDE_DIR
                NGHTTP2_INCLUDE_DIR
                NGHTTP3_INCLUDE_DIR
                XML_INCLUDE_DIR
                JSON_INCLUDE_DIR
                LIBEVENT_INCLUDE_DIR

                FAIL_MESSAGE "AWH library is not found"
            )
            # Формируем список библиотек
            set(AWH_LIBRARIES ${AWH_LIBRARY})
        endif()
        # Формируем список заголовочных файлов
        set(AWH_INCLUDE_DIRS
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
            ${JEMALLOC_INCLUDE_DIR}
            ${NGTCP2_INCLUDE_DIR}
            ${NGHTTP2_INCLUDE_DIR}
            ${NGHTTP3_INCLUDE_DIR}
            ${XML_INCLUDE_DIR}
            ${JSON_INCLUDE_DIR}
            ${LIBEVENT_INCLUDE_DIR}
        )
        # Выполняем установку указанного списка заголовочных файлов зависимостей
        install(DIRECTORY "${LIBEVENT_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
    endif()
# Сборка модуля AWH_EV
else(CMAKE_BUILD_EVENT2)
    # Сборка модуля AWH_IDN, если операционной системой не является Windows
    if(CMAKE_BUILD_IDN AND (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows"))
        # Если активирован режим отладки
        if(CMAKE_AWH_BUILD_DEBUG)
            # Выполняем проверку на существование зависимостей
            find_package_handle_standard_args(AWH REQUIRED_VARS
                AWH_LIBRARY
                DEPEND_LIBRARY
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
                JEMALLOC_INCLUDE_DIR
                NGTCP2_INCLUDE_DIR
                NGHTTP2_INCLUDE_DIR
                NGHTTP3_INCLUDE_DIR
                XML_INCLUDE_DIR
                JSON_INCLUDE_DIR
                IDN2_INCLUDE_DIR
                ICONV_INCLUDE_DIR
                LIBEV_EV_INCLUDE_DIR
                LIBEV_EVPP_INCLUDE_DIR
                LIBEV_EVENT_INCLUDE_DIR

                FAIL_MESSAGE "AWH library is not found"
            )
            # Формируем список библиотек
            set(AWH_LIBRARIES ${AWH_LIBRARY} ${DEPEND_LIBRARY})
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
                JEMALLOC_INCLUDE_DIR
                NGTCP2_INCLUDE_DIR
                NGHTTP2_INCLUDE_DIR
                NGHTTP3_INCLUDE_DIR
                XML_INCLUDE_DIR
                JSON_INCLUDE_DIR
                IDN2_INCLUDE_DIR
                ICONV_INCLUDE_DIR
                LIBEV_EV_INCLUDE_DIR
                LIBEV_EVPP_INCLUDE_DIR
                LIBEV_EVENT_INCLUDE_DIR

                FAIL_MESSAGE "AWH library is not found"
            )
            # Формируем список библиотек
            set(AWH_LIBRARIES ${AWH_LIBRARY})
        endif()
        # Формируем список заголовочных файлов
        set(AWH_INCLUDE_DIRS
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
            ${JEMALLOC_INCLUDE_DIR}
            ${NGTCP2_INCLUDE_DIR}
            ${NGHTTP2_INCLUDE_DIR}
            ${NGHTTP3_INCLUDE_DIR}
            ${XML_INCLUDE_DIR}
            ${JSON_INCLUDE_DIR}
            ${IDN2_INCLUDE_DIR}
            ${ICONV_INCLUDE_DIR}
            ${LIBEV_EV_INCLUDE_DIR}
        )
        # Выполняем установку указанного списка заголовочных файлов зависимостей
        install(DIRECTORY "${IDN2_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
        install(DIRECTORY "${ICONV_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
        install(DIRECTORY "${LIBEV_EV_INCLUDE_DIR}/libev" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
    # Если операцинная система относится к MS Windows
    elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
        # Если активирован режим отладки
        if(CMAKE_AWH_BUILD_DEBUG)
            # Выполняем проверку на существование зависимостей
            find_package_handle_standard_args(AWH REQUIRED_VARS
                AWH_LIBRARY
                DEPEND_LIBRARY
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
                NGTCP2_INCLUDE_DIR
                NGHTTP2_INCLUDE_DIR
                NGHTTP3_INCLUDE_DIR
                XML_INCLUDE_DIR
                JSON_INCLUDE_DIR
                LIBEV_EV_INCLUDE_DIR
                LIBEV_EVPP_INCLUDE_DIR
                LIBEV_EVENT_INCLUDE_DIR

                FAIL_MESSAGE "AWH library is not found"
            )
            # Формируем список библиотек
            set(AWH_LIBRARIES ${AWH_LIBRARY} ${DEPEND_LIBRARY})
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
                NGTCP2_INCLUDE_DIR
                NGHTTP2_INCLUDE_DIR
                NGHTTP3_INCLUDE_DIR
                XML_INCLUDE_DIR
                JSON_INCLUDE_DIR
                LIBEV_EV_INCLUDE_DIR
                LIBEV_EVPP_INCLUDE_DIR
                LIBEV_EVENT_INCLUDE_DIR

                FAIL_MESSAGE "AWH library is not found"
            )
            # Формируем список библиотек
            set(AWH_LIBRARIES ${AWH_LIBRARY})
        endif()
        # Формируем список заголовочных файлов
        set(AWH_INCLUDE_DIRS
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
            ${NGTCP2_INCLUDE_DIR}
            ${NGHTTP2_INCLUDE_DIR}
            ${NGHTTP3_INCLUDE_DIR}
            ${XML_INCLUDE_DIR}
            ${JSON_INCLUDE_DIR}
            ${LIBEV_EV_INCLUDE_DIR}
        )
        # Выполняем установку указанного списка заголовочных файлов зависимостей
        install(DIRECTORY "${LIBEV_EV_INCLUDE_DIR}/libev" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
    # Если операцинная система относится к Nix-подобной
    else()
        # Если активирован режим отладки
        if(CMAKE_AWH_BUILD_DEBUG)
            # Выполняем проверку на существование зависимостей
            find_package_handle_standard_args(AWH REQUIRED_VARS
                AWH_LIBRARY
                DEPEND_LIBRARY
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
                JEMALLOC_INCLUDE_DIR
                NGTCP2_INCLUDE_DIR
                NGHTTP2_INCLUDE_DIR
                NGHTTP3_INCLUDE_DIR
                XML_INCLUDE_DIR
                JSON_INCLUDE_DIR
                LIBEV_EV_INCLUDE_DIR
                LIBEV_EVPP_INCLUDE_DIR
                LIBEV_EVENT_INCLUDE_DIR

                FAIL_MESSAGE "AWH library is not found"
            )
            # Формируем список библиотек
            set(AWH_LIBRARIES ${AWH_LIBRARY} ${DEPEND_LIBRARY})
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
                JEMALLOC_INCLUDE_DIR
                NGTCP2_INCLUDE_DIR
                NGHTTP2_INCLUDE_DIR
                NGHTTP3_INCLUDE_DIR
                XML_INCLUDE_DIR
                JSON_INCLUDE_DIR
                LIBEV_EV_INCLUDE_DIR
                LIBEV_EVPP_INCLUDE_DIR
                LIBEV_EVENT_INCLUDE_DIR

                FAIL_MESSAGE "AWH library is not found"
            )
            # Формируем список библиотек
            set(AWH_LIBRARIES ${AWH_LIBRARY})
        endif()
        # Формируем список заголовочных файлов
        set(AWH_INCLUDE_DIRS
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
            ${JEMALLOC_INCLUDE_DIR}
            ${NGTCP2_INCLUDE_DIR}
            ${NGHTTP2_INCLUDE_DIR}
            ${NGHTTP3_INCLUDE_DIR}
            ${XML_INCLUDE_DIR}
            ${JSON_INCLUDE_DIR}
            ${LIBEV_EV_INCLUDE_DIR}
        )
        # Выполняем установку указанного списка заголовочных файлов зависимостей
        install(DIRECTORY "${LIBEV_EV_INCLUDE_DIR}/libev" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
    endif()
endif(CMAKE_BUILD_EVENT2)

# Устанавливаем библиотеку AWH
install(FILES ${AWH_LIBRARY} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")

# Если активирован режим отладки
if(CMAKE_AWH_BUILD_DEBUG)
    # Устанавливаем статическую библиотеку
    install(FILES ${DEPEND_LIBRARY} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")    
endif()

# Выполняем установку оставшихся заголовочных файлов зависимостей
install(DIRECTORY "${AWH_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY "${JSON_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY "${LZ4_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${BZ2_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${XML_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${CITY_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${ZSTD_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${LZMA_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${ZLIB_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${PCRE_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${BROTLI_INCLUDE_ENCODE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${OPENSSL_INCLUDE_DIR}/openssl" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")

# Если операцинная система не относится к MS Windows
if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    install(DIRECTORY "${JEMALLOC_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
endif()
