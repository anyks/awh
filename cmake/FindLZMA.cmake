set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(CMAKE_FIND_LIBRARY_PREFIXES "lib")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
endif()

# Поиск пути к заголовочным файлам
find_path(LZMA_INCLUDE_DIR NAMES lzma.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/lzma NO_DEFAULT_PATH)
# Поиск библиотеки LZMA
find_library(LZMA_LIBRARY NAMES lzma PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)

# Подключаем 'FindPackageHandle' для использования модуля поиска (find_package(<PackageName>))
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LZMA REQUIRED_VARS
    LZMA_LIBRARY
    LZMA_INCLUDE_DIR

    FAIL_MESSAGE "Missing LZMA. Run ./build_third_party.sh first"
)
