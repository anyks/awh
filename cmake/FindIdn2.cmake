set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(CMAKE_FIND_LIBRARY_PREFIXES "lib")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
endif()

# Поиск пути к заголовочным файлам
find_path(IDN2_INCLUDE_DIR idn2.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/idn2 NO_DEFAULT_PATH)
# Поиск библиотеки IDN2
find_library(IDN2_LIBRARY NAMES idn2 PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)

# Подключаем 'FindPackageHandle' для использования модуля поиска (find_package(<PackageName>))
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Idn2 REQUIRED_VARS
    IDN2_LIBRARY
    IDN2_INCLUDE_DIR

    FAIL_MESSAGE "Missing IDN2. Run ./build_third_party.sh first"
)
