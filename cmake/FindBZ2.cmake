set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(CMAKE_FIND_LIBRARY_PREFIXES "lib")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
endif()

# Поиск пути к заголовочным файлам
find_path(BZ2_INCLUDE_DIR NAMES bzlib.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/bz2 NO_DEFAULT_PATH)
# Поиск библиотеки BZ2
find_library(BZ2_LIBRARY NAMES bz2_static PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)

# Подключаем 'FindPackageHandle' для использования модуля поиска (find_package(<PackageName>))
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BZ2 REQUIRED_VARS
    BZ2_LIBRARY
    BZ2_INCLUDE_DIR

    FAIL_MESSAGE "Missing BZ2. Run ./build_third_party.sh first"
)
