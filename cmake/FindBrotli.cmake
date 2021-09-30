set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(CMAKE_FIND_LIBRARY_PREFIXES "lib")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
endif()

# Поиск пути к заголовочным файлам
find_path(BROTLI_INCLUDE_ENCODE_DIR encode.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/brotli NO_DEFAULT_PATH)
find_path(BROTLI_INCLUDE_DECODE_DIR decode.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/brotli NO_DEFAULT_PATH)
# Поиск библиотеки Brotli
find_library(BROTLI_ENCODE_LIBRARY NAMES brotlienc-static PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)
find_library(BROTLI_DECODE_LIBRARY NAMES brotlidec-static PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)
find_library(BROTLI_COMMON_LIBRARY NAMES brotlicommon-static PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)

# Подключаем 'FindPackageHandle' для использования модуля поиска (find_package(<PackageName>))
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Brotli REQUIRED_VARS
    BROTLI_DECODE_LIBRARY
    BROTLI_ENCODE_LIBRARY
    BROTLI_COMMON_LIBRARY
    BROTLI_INCLUDE_ENCODE_DIR
    BROTLI_INCLUDE_DECODE_DIR

    FAIL_MESSAGE "Missing Brotli. Run ./build_third_party.sh first"
)

set(BROTLI_INCLUDE_DIRS
    ${BROTLI_INCLUDE_ENCODE_DIR}
    ${BROTLI_INCLUDE_DECODE_DIR}
)

set(BROTLI_LIBRARIES
    ${BROTLI_ENCODE_LIBRARY}
    ${BROTLI_DECODE_LIBRARY}
    ${BROTLI_COMMON_LIBRARY}
)
