SET(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)

# Если операцинная система относится к MS Windows
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    SET(CMAKE_FIND_LIBRARY_PREFIXES "lib")
    SET(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
endif()

# Поиск пути к заголовочным файлам
find_path(LZ4_INCLUDE_DIR NAMES lz4.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/lz4 NO_DEFAULT_PATH)
find_path(BZ2_INCLUDE_DIR NAMES bzlib.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/bz2 NO_DEFAULT_PATH)
find_path(ZSTD_INCLUDE_DIR NAMES zstd.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/zstd NO_DEFAULT_PATH)
find_path(LZMA_INCLUDE_DIR NAMES lzma.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/lzma NO_DEFAULT_PATH)
find_path(ZLIB_INCLUDE_DIR NAMES zlib.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/zlib NO_DEFAULT_PATH)
find_path(BROTLI_INCLUDE_ENCODE_DIR NAMES encode.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/brotli NO_DEFAULT_PATH)
find_path(BROTLI_INCLUDE_DECODE_DIR NAMES decode.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/brotli NO_DEFAULT_PATH)
find_path(OPENSSL_INCLUDE_DIR NAMES openssl/opensslconf.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include NO_DEFAULT_PATH)
find_path(PCRE_INCLUDE_DIR NAMES pcre2.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/pcre2 NO_DEFAULT_PATH)
find_path(NGHTTP2_INCLUDE_DIR NAMES nghttp2.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/nghttp2 NO_DEFAULT_PATH)

# Сборка модуля AWH_IDN, если операционной системой не является Windows
if (CMAKE_BUILD_IDN AND (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows"))
    find_path(IDN2_INCLUDE_DIR NAMES idn2.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/idn2 NO_DEFAULT_PATH)
    find_path(ICONV_INCLUDE_DIR NAMES iconv.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/iconv NO_DEFAULT_PATH)
endif()

# Поиск библиотеки Dependence
find_library(DEPEND_LIBRARY NAMES dependence PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)

# Подключаем 'FindPackageHandle' для использования модуля поиска (find_package(<PackageName>))
include(FindPackageHandleStandardArgs)

# Сборка модуля AWH_IDN, если операционной системой не является Windows
if (CMAKE_BUILD_IDN AND (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows"))
    # Выполняем проверку на существование зависимостей
    find_package_handle_standard_args(Dependence REQUIRED_VARS
        DEPEND_LIBRARY
        LZ4_INCLUDE_DIR
        BZ2_INCLUDE_DIR
        ZSTD_INCLUDE_DIR
        LZMA_INCLUDE_DIR
        ZLIB_INCLUDE_DIR
        BROTLI_INCLUDE_ENCODE_DIR
        BROTLI_INCLUDE_DECODE_DIR
        OPENSSL_INCLUDE_DIR
        PCRE_INCLUDE_DIR
        NGHTTP2_INCLUDE_DIR
        IDN2_INCLUDE_DIR
        ICONV_INCLUDE_DIR

        FAIL_MESSAGE "Missing Dependence. Run ./build_third_party.sh first"
    )
    # Формируем список заголовочных файлов
    SET(DEPEND_INCLUDE_DIRS
        ${LZ4_INCLUDE_DIR}
        ${BZ2_INCLUDE_DIR}
        ${ZSTD_INCLUDE_DIR}
        ${LZMA_INCLUDE_DIR}
        ${ZLIB_INCLUDE_DIR}
        ${BROTLI_INCLUDE_ENCODE_DIR}
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
    # Выполняем проверку на существование зависимостей
    find_package_handle_standard_args(Dependence REQUIRED_VARS
        DEPEND_LIBRARY
        LZ4_INCLUDE_DIR
        BZ2_INCLUDE_DIR
        ZSTD_INCLUDE_DIR
        LZMA_INCLUDE_DIR
        ZLIB_INCLUDE_DIR
        BROTLI_INCLUDE_ENCODE_DIR
        BROTLI_INCLUDE_DECODE_DIR
        OPENSSL_INCLUDE_DIR
        PCRE_INCLUDE_DIR
        NGHTTP2_INCLUDE_DIR

        FAIL_MESSAGE "Missing Dependence. Run ./build_third_party.sh first"
    )
    # Формируем список заголовочных файлов
    SET(DEPEND_INCLUDE_DIRS
        ${LZ4_INCLUDE_DIR}
        ${BZ2_INCLUDE_DIR}
        ${ZSTD_INCLUDE_DIR}
        ${LZMA_INCLUDE_DIR}
        ${ZLIB_INCLUDE_DIR}
        ${BROTLI_INCLUDE_ENCODE_DIR}
        ${OPENSSL_INCLUDE_DIR}
        ${PCRE_INCLUDE_DIR}
        ${NGHTTP2_INCLUDE_DIR}
    )
# Если операцинная система относится к Nix-подобной
else()
    # Выполняем проверку на существование зависимостей
    find_package_handle_standard_args(Dependence REQUIRED_VARS
        DEPEND_LIBRARY
        LZ4_INCLUDE_DIR
        BZ2_INCLUDE_DIR
        ZSTD_INCLUDE_DIR
        LZMA_INCLUDE_DIR
        ZLIB_INCLUDE_DIR
        BROTLI_INCLUDE_ENCODE_DIR
        BROTLI_INCLUDE_DECODE_DIR
        OPENSSL_INCLUDE_DIR
        PCRE_INCLUDE_DIR
        NGHTTP2_INCLUDE_DIR

        FAIL_MESSAGE "Missing Dependence. Run ./build_third_party.sh first"
    )
    # Формируем список заголовочных файлов
    SET(DEPEND_INCLUDE_DIRS
        ${LZ4_INCLUDE_DIR}
        ${BZ2_INCLUDE_DIR}
        ${ZSTD_INCLUDE_DIR}
        ${LZMA_INCLUDE_DIR}
        ${ZLIB_INCLUDE_DIR}
        ${BROTLI_INCLUDE_ENCODE_DIR}
        ${OPENSSL_INCLUDE_DIR}
        ${PCRE_INCLUDE_DIR}
        ${NGHTTP2_INCLUDE_DIR}
    )
endif()

# Выполняем установку оставшихся заголовочных файлов зависимостей
install(DIRECTORY "${LZ4_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${BZ2_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${ZSTD_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${LZMA_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${ZLIB_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${PCRE_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${NGHTTP2_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${BROTLI_INCLUDE_ENCODE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${OPENSSL_INCLUDE_DIR}/openssl" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
