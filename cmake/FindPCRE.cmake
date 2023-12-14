set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(CMAKE_FIND_LIBRARY_PREFIXES "lib")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
endif()

# Поиск пути к заголовочным файлам
find_path(PCRE_INCLUDE_DIR NAMES pcre2.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/pcre2 NO_DEFAULT_PATH)
# Поиск библиотеки PCRE 8
find_library(PCRE_LIBRARY_8 NAMES pcre2-8 PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)
# Поиск библиотеки PCRE 16
find_library(PCRE_LIBRARY_16 NAMES pcre2-16 PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)
# Поиск библиотеки PCRE 32
find_library(PCRE_LIBRARY_32 NAMES pcre2-32 PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)
# Поиск библиотеки PCRE_POSIX
find_library(PCRE_POSIX_LIBRARY NAMES pcre2-posix PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)

# Подключаем 'FindPackageHandle' для использования модуля поиска (find_package(<PackageName>))
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PCRE REQUIRED_VARS
    PCRE_POSIX_LIBRARY
    PCRE_LIBRARY_8
    PCRE_LIBRARY_16
    PCRE_LIBRARY_32
    PCRE_INCLUDE_DIR

    FAIL_MESSAGE "Missing PCRE. Run ./build_third_party.sh first"
)

set(PCRE_LIBRARIES
    ${PCRE_POSIX_LIBRARY}
    ${PCRE_LIBRARY_8}
    ${PCRE_LIBRARY_16}
    ${PCRE_LIBRARY_32}
)
