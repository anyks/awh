set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(CMAKE_FIND_LIBRARY_PREFIXES "lib")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
endif()

# Поиск пути к заголовочным файлам
find_path(USRSCTP_INCLUDE_DIR usrsctp.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/usrsctp NO_DEFAULT_PATH)
# Поиск библиотек UsrSCTP
find_library(USRSCTP_LIBRARY NAMES usrsctp PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)

# Подключаем 'FindPackageHandle' для использования модуля поиска (find_package(<PackageName>))
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(UsrSCTP REQUIRED_VARS
    USRSCTP_LIBRARY
    USRSCTP_INCLUDE_DIR

    FAIL_MESSAGE "Missing UsrSCTP. Run ./build_third_party.sh first"
)
