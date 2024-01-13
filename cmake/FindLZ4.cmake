set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(CMAKE_FIND_LIBRARY_PREFIXES "lib")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
endif()

# Поиск пути к заголовочным файлам
find_path(LZ4_INCLUDE_DIR NAMES lz4.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/lz4 NO_DEFAULT_PATH)
# Поиск библиотеки LZ4
find_library(LZ4_LIBRARY NAMES lz4 PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)

# Подключаем 'FindPackageHandle' для использования модуля поиска (find_package(<PackageName>))
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LZ4 REQUIRED_VARS
    LZ4_LIBRARY
    LZ4_INCLUDE_DIR

    FAIL_MESSAGE "Missing LZ4. Run ./build_third_party.sh first"
)
