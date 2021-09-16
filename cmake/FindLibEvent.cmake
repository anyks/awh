set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(CMAKE_FIND_LIBRARY_PREFIXES "lib")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
endif()

# Поиск пути к заголовочным файлам
find_path(LIBEVENT_INCLUDE_DIR event2/event.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include NO_DEFAULT_PATH)
# Поиск библиотеки LibEvent2
find_library(LIBEVENT_LIBRARY NAMES event PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)
find_library(LIBEVENT_CORE NAMES event_core PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)
find_library(LIBEVENT_EXTRA NAMES event_extra PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)
find_library(LIBEVENT_SSL NAMES event_openssl PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)
if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    find_library(LIBEVENT_THREAD NAMES event_pthreads PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)
endif()

# Подключаем 'FindPackageHandle' для использования модуля поиска (find_package(<PackageName>))
include(FindPackageHandleStandardArgs)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    find_package_handle_standard_args(LibEvent REQUIRED_VARS
        LIBEVENT_LIBRARY
        LIBEVENT_CORE
        LIBEVENT_EXTRA
        LIBEVENT_SSL
    )

    set(LIBEVENT_LIBRARIES
        ${LIBEVENT_LIBRARY}
        ${LIBEVENT_CORE}
        ${LIBEVENT_EXTRA}
        ${LIBEVENT_SSL}
    )
else()
    find_package_handle_standard_args(LibEvent REQUIRED_VARS
        LIBEVENT_LIBRARY
        LIBEVENT_CORE
        LIBEVENT_EXTRA
        LIBEVENT_SSL
        LIBEVENT_THREAD

        FAIL_MESSAGE "Missing LibEvent2. Run ./build_third_party.sh first"
    )

    set(LIBEVENT_LIBRARIES
        ${LIBEVENT_LIBRARY}
        ${LIBEVENT_CORE}
        ${LIBEVENT_EXTRA}
        ${LIBEVENT_SSL}
        ${LIBEVENT_THREAD}
    )
endif()
