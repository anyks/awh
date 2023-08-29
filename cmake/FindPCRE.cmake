set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(CMAKE_FIND_LIBRARY_PREFIXES "lib")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
endif()

# Поиск пути к заголовочным файлам
find_path(PCRE_INCLUDE_DIR NAMES pcre.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/pcre NO_DEFAULT_PATH)
# Поиск библиотеки PCRE
find_library(PCRE_LIBRARY NAMES pcre PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)
# Поиск библиотеки PCRE_CPP
find_library(PCRE_CPP_LIBRARY NAMES pcrecpp PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)
# Поиск библиотеки PCRE_POSIX
find_library(PCRE_POSIX_LIBRARY NAMES pcreposix PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)

# Подключаем 'FindPackageHandle' для использования модуля поиска (find_package(<PackageName>))
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PCRE REQUIRED_VARS
    PCRE_LIBRARY
    PCRE_CPP_LIBRARY
    PCRE_POSIX_LIBRARY
    PCRE_INCLUDE_DIR

    FAIL_MESSAGE "Missing PCRE. Run ./build_third_party.sh first"
)

set(PCRE_LIBRARIES
    ${PCRE_LIBRARY}
    ${PCRE_CPP_LIBRARY}
    ${PCRE_POSIX_LIBRARY}
)
