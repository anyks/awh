set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(CMAKE_FIND_LIBRARY_PREFIXES "lib")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
endif()

# Поиск пути к заголовочным файлам
find_path(ICONV_INCLUDE_DIR NAMES iconv.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/iconv NO_DEFAULT_PATH)
# Поиск библиотеки ICONV
find_library(ICONV_LIBRARY NAMES iconv PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)
# Поиск библиотеки ICONV_CHARSET
find_library(ICONV_CHARSET_LIBRARY NAMES charset PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)

# Подключаем 'FindPackageHandle' для использования модуля поиска (find_package(<PackageName>))
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Iconv REQUIRED_VARS
    ICONV_LIBRARY
    ICONV_CHARSET_LIBRARY
    ICONV_INCLUDE_DIR

    FAIL_MESSAGE "Missing ICONV. Run ./build_third_party.sh first"
)

set(ICONV_LIBRARIES
    ${ICONV_LIBRARY}
    ${ICONV_CHARSET_LIBRARY}
)
