set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(CMAKE_FIND_LIBRARY_PREFIXES "lib")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
endif()

# Поиск пути к заголовочным файлам
find_path(NGHTTP2_INCLUDE_DIR NAMES nghttp2.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/nghttp2 NO_DEFAULT_PATH)
# Поиск библиотеки NgHttp2
find_library(NGHTTP2_LIBRARY NAMES nghttp2 PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)

# Подключаем 'FindPackageHandle' для использования модуля поиска (find_package(<PackageName>))
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NgHttp2 REQUIRED_VARS
    NGHTTP2_LIBRARY
    NGHTTP2_INCLUDE_DIR

    FAIL_MESSAGE "Missing NgHttp2. Run ./build_third_party.sh first"
)
