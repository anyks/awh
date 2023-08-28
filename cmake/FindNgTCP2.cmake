set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(CMAKE_FIND_LIBRARY_PREFIXES "lib")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
endif()

# Поиск пути к заголовочным файлам
find_path(NGTCP2_INCLUDE_DIR NAMES ngtcp2.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/ngtcp2 NO_DEFAULT_PATH)
# Поиск библиотеки NgTCP2
find_library(NGTCP2_LIBRARY NAMES ngtcp2 PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)

# Подключаем 'FindPackageHandle' для использования модуля поиска (find_package(<PackageName>))
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NgTCP2 REQUIRED_VARS
    NGTCP2_LIBRARY
    NGTCP2_INCLUDE_DIR

    FAIL_MESSAGE "Missing NgTCP2. Run ./build_third_party.sh first"
)
