##
 # Каталог собранных зависимостей
 #
 # У каждой оснастки он свой: двоичный код MinGW и MSVC несовместим, а часть
 # зависимостей собирает заголовки ПОД ОСНАСТКУ (zlib решает наличие unistd.h именно
 # при сборке). Общий каталог означал бы, что последняя собранная зависимость молча
 # ломает сборку соседней оснасткой
 ##
if (NOT DEFINED AWH_THIRD_PARTY)
    if (MSVC)
        SET(AWH_THIRD_PARTY "${CMAKE_SOURCE_DIR}/third_party-msvc")
    else()
        SET(AWH_THIRD_PARTY "${CMAKE_SOURCE_DIR}/third_party")
    endif()
endif()

##
 # Запоминаем правила поиска, действовавшие до нас
 #
 # Модуль поиска вызывается из общего дерева, и сужение правил ниже осталось бы
 # действовать на ВСЕ последующие поиски проекта: суженный до «.lib» перечень
 # расширений однажды уже спрятал архив с именем «libpcre2-8.a», и шесть проверок
 # сравнения выражений на Windows молча перестали собираться
 ##
SET(AWH_FIND_LIBRARY_PREFIXES_KEPT ${CMAKE_FIND_LIBRARY_PREFIXES})
SET(AWH_FIND_LIBRARY_SUFFIXES_KEPT ${CMAKE_FIND_LIBRARY_SUFFIXES})
SET(AWH_FIND_USE_SYSTEM_ENVIRONMENT_PATH_KEPT ${CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH})

SET(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)

# Если операцинная система относится к MS Windows
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    ##
     # Приставку «lib» ставит GNU-оснастка, а MSVC именует архив по цели сборки:
     # «nts.lib», «tcmalloc_minimal.lib». Пустая приставка в перечне поэтому обязана
     # стоять рядом с «lib», иначе под MSVC не нашлось бы ничего, кроме слияния
     ##
    if (MSVC)
        SET(CMAKE_FIND_LIBRARY_PREFIXES "" "lib")
    else()
        SET(CMAKE_FIND_LIBRARY_PREFIXES "lib")
    endif()
    SET(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
endif()

##
 # Подсказка о сборке зависимостей называется по оснастке: под MSVC её запускают
 # bat-сборщиком, под остальными - скриптом оболочки
 ##
if (MSVC)
    SET(AWH_BUILD_HINT "bat\\build_third_party.bat")
else()
    SET(AWH_BUILD_HINT "./build_third_party.sh")
endif()

# Поиск пути к заголовочным файлам
find_path(LZ4_INCLUDE_DIR NAMES lz4.h PATHS ${AWH_THIRD_PARTY}/include/lz4 NO_DEFAULT_PATH)
find_path(BZ2_INCLUDE_DIR NAMES bzlib.h PATHS ${AWH_THIRD_PARTY}/include/bz2 NO_DEFAULT_PATH)
find_path(ZSTD_INCLUDE_DIR NAMES zstd.h PATHS ${AWH_THIRD_PARTY}/include/zstd NO_DEFAULT_PATH)
find_path(LZMA_INCLUDE_DIR NAMES lzma.h PATHS ${AWH_THIRD_PARTY}/include/lzma NO_DEFAULT_PATH)
find_path(ZLIB_INCLUDE_DIR NAMES zlib.h PATHS ${AWH_THIRD_PARTY}/include/zlib NO_DEFAULT_PATH)
find_path(BROTLI_INCLUDE_ENCODE_DIR NAMES encode.h PATHS ${AWH_THIRD_PARTY}/include/brotli NO_DEFAULT_PATH)
find_path(BROTLI_INCLUDE_DECODE_DIR NAMES decode.h PATHS ${AWH_THIRD_PARTY}/include/brotli NO_DEFAULT_PATH)
find_path(NTS_INCLUDE_DIR NAMES nts/client/client.hpp PATHS ${AWH_THIRD_PARTY}/include NO_DEFAULT_PATH)

##
 # Внутренние включения самой A-NTS идут без префикса nts/ (например, <sys/global.hpp>),
 # поэтому на путь ставится и каталог заголовков библиотеки, и его родитель:
 # модули AWH обращаются к библиотеке в форме <nts/digest/hash.hpp>
 ##
SET(NTS_INTERNAL_DIR "${NTS_INCLUDE_DIR}/nts")
find_path(PCRE_INCLUDE_DIR NAMES pcre2.h PATHS ${AWH_THIRD_PARTY}/include/pcre2 NO_DEFAULT_PATH)
find_path(NGHTTP2_INCLUDE_DIR NAMES nghttp2.h PATHS ${AWH_THIRD_PARTY}/include/nghttp2 NO_DEFAULT_PATH)

# Сборка модуля AWH_IDN, если операционной системой не является Windows
if (CMAKE_BUILD_IDN AND (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows"))
    find_path(IDN2_INCLUDE_DIR NAMES idn2.h PATHS ${AWH_THIRD_PARTY}/include/idn2 NO_DEFAULT_PATH)
    find_path(ICONV_INCLUDE_DIR NAMES iconv.h PATHS ${AWH_THIRD_PARTY}/include/iconv NO_DEFAULT_PATH)
endif()

# Поиск библиотеки Dependence
find_library(DEPEND_LIBRARY NAMES dependence PATHS ${AWH_THIRD_PARTY}/lib NO_DEFAULT_PATH)

# Подключаем 'FindPackageHandle' для использования модуля поиска (find_package(<PackageName>))
include(FindPackageHandleStandardArgs)

# Сборка модуля AWH_IDN, если операционной системой не является Windows
if (CMAKE_BUILD_IDN AND (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows"))
    # Выполняем проверку на существование зависимостей
    find_package_handle_standard_args(Dependence REQUIRED_VARS
        DEPEND_LIBRARY
        LZ4_INCLUDE_DIR
        BZ2_INCLUDE_DIR
        ZSTD_INCLUDE_DIR
        LZMA_INCLUDE_DIR
        ZLIB_INCLUDE_DIR
        BROTLI_INCLUDE_ENCODE_DIR
        BROTLI_INCLUDE_DECODE_DIR
        NTS_INCLUDE_DIR
        PCRE_INCLUDE_DIR
        NGHTTP2_INCLUDE_DIR
        IDN2_INCLUDE_DIR
        ICONV_INCLUDE_DIR

        FAIL_MESSAGE "Missing Dependence. Run ${AWH_BUILD_HINT} first"
    )
    # Формируем список заголовочных файлов
    SET(DEPEND_INCLUDE_DIRS
        ${LZ4_INCLUDE_DIR}
        ${BZ2_INCLUDE_DIR}
        ${ZSTD_INCLUDE_DIR}
        ${LZMA_INCLUDE_DIR}
        ${ZLIB_INCLUDE_DIR}
        ${BROTLI_INCLUDE_ENCODE_DIR}
        ${NTS_INCLUDE_DIR}
        ${NTS_INTERNAL_DIR}
        ${PCRE_INCLUDE_DIR}
        ${NGHTTP2_INCLUDE_DIR}
        ${IDN2_INCLUDE_DIR}
        ${ICONV_INCLUDE_DIR}
    )
    # Выполняем установку указанного списка заголовочных файлов зависимостей
    install(DIRECTORY "${IDN2_INCLUDE_DIR}" DESTINATION "include" FILES_MATCHING PATTERN "*.h")
    install(DIRECTORY "${ICONV_INCLUDE_DIR}" DESTINATION "include" FILES_MATCHING PATTERN "*.h")
# Если операцинная система относится к MS Windows
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    # Выполняем проверку на существование зависимостей
    find_package_handle_standard_args(Dependence REQUIRED_VARS
        DEPEND_LIBRARY
        LZ4_INCLUDE_DIR
        BZ2_INCLUDE_DIR
        ZSTD_INCLUDE_DIR
        LZMA_INCLUDE_DIR
        ZLIB_INCLUDE_DIR
        BROTLI_INCLUDE_ENCODE_DIR
        BROTLI_INCLUDE_DECODE_DIR
        NTS_INCLUDE_DIR
        PCRE_INCLUDE_DIR
        NGHTTP2_INCLUDE_DIR

        FAIL_MESSAGE "Missing Dependence. Run ${AWH_BUILD_HINT} first"
    )
    # Формируем список заголовочных файлов
    SET(DEPEND_INCLUDE_DIRS
        ${LZ4_INCLUDE_DIR}
        ${BZ2_INCLUDE_DIR}
        ${ZSTD_INCLUDE_DIR}
        ${LZMA_INCLUDE_DIR}
        ${ZLIB_INCLUDE_DIR}
        ${BROTLI_INCLUDE_ENCODE_DIR}
        ${NTS_INCLUDE_DIR}
        ${NTS_INTERNAL_DIR}
        ${PCRE_INCLUDE_DIR}
        ${NGHTTP2_INCLUDE_DIR}
    )
# Если операцинная система относится к Nix-подобной
else()
    # Выполняем проверку на существование зависимостей
    find_package_handle_standard_args(Dependence REQUIRED_VARS
        DEPEND_LIBRARY
        LZ4_INCLUDE_DIR
        BZ2_INCLUDE_DIR
        ZSTD_INCLUDE_DIR
        LZMA_INCLUDE_DIR
        ZLIB_INCLUDE_DIR
        BROTLI_INCLUDE_ENCODE_DIR
        BROTLI_INCLUDE_DECODE_DIR
        NTS_INCLUDE_DIR
        PCRE_INCLUDE_DIR
        NGHTTP2_INCLUDE_DIR

        FAIL_MESSAGE "Missing Dependence. Run ${AWH_BUILD_HINT} first"
    )
    # Формируем список заголовочных файлов
    SET(DEPEND_INCLUDE_DIRS
        ${LZ4_INCLUDE_DIR}
        ${BZ2_INCLUDE_DIR}
        ${ZSTD_INCLUDE_DIR}
        ${LZMA_INCLUDE_DIR}
        ${ZLIB_INCLUDE_DIR}
        ${BROTLI_INCLUDE_ENCODE_DIR}
        ${NTS_INCLUDE_DIR}
        ${NTS_INTERNAL_DIR}
        ${PCRE_INCLUDE_DIR}
        ${NGHTTP2_INCLUDE_DIR}
    )
endif()

# Выполняем установку оставшихся заголовочных файлов зависимостей
install(DIRECTORY "${LZ4_INCLUDE_DIR}" DESTINATION "include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${BZ2_INCLUDE_DIR}" DESTINATION "include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${ZSTD_INCLUDE_DIR}" DESTINATION "include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${LZMA_INCLUDE_DIR}" DESTINATION "include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${ZLIB_INCLUDE_DIR}" DESTINATION "include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${PCRE_INCLUDE_DIR}" DESTINATION "include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${NGHTTP2_INCLUDE_DIR}" DESTINATION "include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${BROTLI_INCLUDE_ENCODE_DIR}" DESTINATION "include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${NTS_INCLUDE_DIR}/nts" DESTINATION "include" FILES_MATCHING PATTERN "*.hpp")

# Возвращаем правила поиска, действовавшие до нас
SET(CMAKE_FIND_LIBRARY_PREFIXES ${AWH_FIND_LIBRARY_PREFIXES_KEPT})
SET(CMAKE_FIND_LIBRARY_SUFFIXES ${AWH_FIND_LIBRARY_SUFFIXES_KEPT})
SET(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH ${AWH_FIND_USE_SYSTEM_ENVIRONMENT_PATH_KEPT})
