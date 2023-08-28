set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(CMAKE_FIND_LIBRARY_PREFIXES "lib")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
endif()

# Поиск пути к заголовочным файлам
find_path(JEMALLOC_INCLUDE_DIR NAMES jemalloc.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/jemalloc NO_DEFAULT_PATH)
# Поиск библиотеки JeMalloc
find_library(JEMALLOC_LIBRARY NAMES jemalloc PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)

# Подключаем 'FindPackageHandle' для использования модуля поиска (find_package(<PackageName>))
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JeMalloc REQUIRED_VARS
    JEMALLOC_LIBRARY
    JEMALLOC_INCLUDE_DIR

    FAIL_MESSAGE "Missing JeMalloc. Run ./build_third_party.sh first"
)
