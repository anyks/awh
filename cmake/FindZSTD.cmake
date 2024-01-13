set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(CMAKE_FIND_LIBRARY_PREFIXES "lib")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
endif()

# Поиск пути к заголовочным файлам
find_path(ZSTD_INCLUDE_DIR NAMES zstd.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/zstd NO_DEFAULT_PATH)
# Поиск библиотеки ZSTD
find_library(ZSTD_LIBRARY NAMES zstd PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)

# Подключаем 'FindPackageHandle' для использования модуля поиска (find_package(<PackageName>))
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZSTD REQUIRED_VARS
    ZSTD_LIBRARY
    ZSTD_INCLUDE_DIR

    FAIL_MESSAGE "Missing ZSTD. Run ./build_third_party.sh first"
)
