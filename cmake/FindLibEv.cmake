set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(CMAKE_FIND_LIBRARY_PREFIXES "lib")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
endif()

# Поиск пути к заголовочным файлам
find_path(LIBEV_INCLUDE_DIR NAMES libev/ev.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include NO_DEFAULT_PATH)
find_path(LIBEV_INCLUDE_DIR NAMES libev/ev++.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include NO_DEFAULT_PATH)
find_path(LIBEV_INCLUDE_DIR NAMES libev/event.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include NO_DEFAULT_PATH)

# Поиск библиотеки LibEv
find_library(LIBEV_LIBRARY NAMES ev PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)

# Подключаем 'FindPackageHandle' для использования модуля поиска (find_package(<PackageName>))
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibEv REQUIRED_VARS
    LIBEV_LIBRARY
    LIBEV_INCLUDE_DIR

    FAIL_MESSAGE "Missing LibEv. Run ./build_third_party.sh first"
)
