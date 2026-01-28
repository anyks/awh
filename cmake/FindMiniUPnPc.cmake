SET(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)

# Если операцинная система относится к MS Windows
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    SET(CMAKE_FIND_LIBRARY_PREFIXES "lib")
    SET(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
endif()

# Поиск библиотеки MiniUPnPc
find_library(MINIUPNPC_LIBRARY NAMES miniupnpc PATHS /opt/homebrew/opt/miniupnpc/lib /usr/local/lib NO_DEFAULT_PATH)
# Поиск пути к заголовочным файлам
find_path(MINIUPNPC_INCLUDE_DIR NAMES miniupnpc/miniupnpc.h PATHS /opt/homebrew/opt/miniupnpc/include /usr/local/include NO_DEFAULT_PATH)

# Подключаем 'FindPackageHandle' для использования модуля поиска (find_package(<PackageName>))
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MiniUPnPc REQUIRED_VARS
    MINIUPNPC_LIBRARY
    MINIUPNPC_INCLUDE_DIR

    FAIL_MESSAGE "Missing MiniUPnPc. Run ./build_third_party.sh first"
)
