SET(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    SET(CMAKE_FIND_LIBRARY_PREFIXES "lib")
    SET(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
endif()

# Поиск библиотеки TcMalloc
find_library(TCMALLOC_LIBRARY NAMES tcmalloc_minimal PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)
# Поиск библиотеки TcMallocCommon
find_library(TCMALLOC_COMMON_LIBRARY NAMES common PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)

# Подключаем 'FindPackageHandle' для использования модуля поиска (find_package(<PackageName>))
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TcMalloc REQUIRED_VARS
    TCMALLOC_LIBRARY
    TCMALLOC_COMMON_LIBRARY

    FAIL_MESSAGE "Missing TcMalloc. Run ./build_third_party.sh first"
)

# Объединяем библиотеки в одну
SET(TCMALLOC_LIBRARIES
    ${TCMALLOC_LIBRARY}
    ${TCMALLOC_COMMON_LIBRARY}
)
