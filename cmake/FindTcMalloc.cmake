##
 # Каталог собранных зависимостей
 #
 # У каждой оснастки он свой: двоичный код MinGW и MSVC несовместим. Значение это
 # заводит модуль поиска Dependence, а здесь оно лишь подхватывается - порядок
 # вызова у них в дереве один, но модуль обязан работать и сам по себе
 ##
if (NOT DEFINED AWH_THIRD_PARTY)
    if (MSVC)
        SET(AWH_THIRD_PARTY "${CMAKE_SOURCE_DIR}/third_party-msvc")
    else()
        SET(AWH_THIRD_PARTY "${CMAKE_SOURCE_DIR}/third_party")
    endif()
endif()

# Запоминаем правила поиска, действовавшие до нас
SET(AWH_FIND_LIBRARY_PREFIXES_KEPT ${CMAKE_FIND_LIBRARY_PREFIXES})
SET(AWH_FIND_LIBRARY_SUFFIXES_KEPT ${CMAKE_FIND_LIBRARY_SUFFIXES})
SET(AWH_FIND_USE_SYSTEM_ENVIRONMENT_PATH_KEPT ${CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH})

SET(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)

# Если операцинная система относится к MS Windows
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    ##
     # Приставку «lib» ставит GNU-оснастка, а MSVC именует архив по цели сборки:
     # «tcmalloc_minimal.lib», «common.lib». С одной лишь приставкой «lib» поиск
     # под MSVC не нашёл бы ни того, ни другого, и сборка встала бы на отказе
     # «TcMalloc not found» при заведомо собранных архивах
     ##
    if (MSVC)
        SET(CMAKE_FIND_LIBRARY_PREFIXES "" "lib")
    else()
        SET(CMAKE_FIND_LIBRARY_PREFIXES "lib")
    endif()
    SET(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
endif()

# Поиск библиотеки TcMalloc
find_library(TCMALLOC_LIBRARY NAMES tcmalloc_minimal PATHS ${AWH_THIRD_PARTY}/lib NO_DEFAULT_PATH)
# Поиск библиотеки TcMallocCommon
find_library(TCMALLOC_COMMON_LIBRARY NAMES common PATHS ${AWH_THIRD_PARTY}/lib NO_DEFAULT_PATH)

# Подключаем 'FindPackageHandle' для использования модуля поиска (find_package(<PackageName>))
include(FindPackageHandleStandardArgs)

# Подсказка о сборке зависимостей называется по оснастке
if (MSVC)
    SET(AWH_BUILD_HINT "bat\\build_third_party.bat")
else()
    SET(AWH_BUILD_HINT "./build_third_party.sh")
endif()

find_package_handle_standard_args(TcMalloc REQUIRED_VARS
    TCMALLOC_LIBRARY
    TCMALLOC_COMMON_LIBRARY

    FAIL_MESSAGE "Missing TcMalloc. Run ${AWH_BUILD_HINT} first"
)

# Объединяем библиотеки в одну
SET(TCMALLOC_LIBRARIES
    ${TCMALLOC_LIBRARY}
    ${TCMALLOC_COMMON_LIBRARY}
)

# Возвращаем правила поиска, действовавшие до нас
SET(CMAKE_FIND_LIBRARY_PREFIXES ${AWH_FIND_LIBRARY_PREFIXES_KEPT})
SET(CMAKE_FIND_LIBRARY_SUFFIXES ${AWH_FIND_LIBRARY_SUFFIXES_KEPT})
SET(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH ${AWH_FIND_USE_SYSTEM_ENVIRONMENT_PATH_KEPT})
