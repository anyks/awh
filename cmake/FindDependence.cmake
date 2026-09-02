# Запоминаем правила поиска, действовавшие до нас
#
# Правила эти общие на весь разбор сборки, а сужаются они здесь под СВОИ
# зависимости - те, что лежат в дереве и названы по-своему. Не вернув их назад,
# файл этот навязывал бы своё правило всякому поиску, идущему после него.
#
# Ущерб от этого замерен на стенде MS Windows: сужение до «.lib» делало невидимой
# библиотеку эталона сличения (`libpcre2-8.a`), поиск её отвечал «не найдено», и
# шесть проверок модуля выражений, сличающих его с эталоном, отменялись
# препроцессором - молча, без единого довода в выводе сборки. Пропуск молчаливый
# неотличим от прохождения, и на этой системе модуль выражений не сличался с
# эталоном НИ РАЗУ.
SET(AWH_FIND_LIBRARY_PREFIXES_KEPT ${CMAKE_FIND_LIBRARY_PREFIXES})
SET(AWH_FIND_LIBRARY_SUFFIXES_KEPT ${CMAKE_FIND_LIBRARY_SUFFIXES})
SET(AWH_FIND_USE_SYSTEM_ENVIRONMENT_PATH_KEPT ${CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH})

SET(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)

# Если операцинная система относится к MS Windows
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    SET(CMAKE_FIND_LIBRARY_PREFIXES "lib")
    SET(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
endif()

# Поиск пути к заголовочным файлам
find_path(LZ4_INCLUDE_DIR NAMES lz4.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/lz4 NO_DEFAULT_PATH)
find_path(BZ2_INCLUDE_DIR NAMES bzlib.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/bz2 NO_DEFAULT_PATH)
find_path(ZSTD_INCLUDE_DIR NAMES zstd.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/zstd NO_DEFAULT_PATH)
find_path(LZMA_INCLUDE_DIR NAMES lzma.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/lzma NO_DEFAULT_PATH)
find_path(ZLIB_INCLUDE_DIR NAMES zlib.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/zlib NO_DEFAULT_PATH)
find_path(SNAPPY_INCLUDE_DIR NAMES snappy.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/snappy NO_DEFAULT_PATH)
find_path(DENSITY_INCLUDE_DIR NAMES density_api.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/density NO_DEFAULT_PATH)
find_path(BROTLI_INCLUDE_ENCODE_DIR NAMES encode.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/brotli NO_DEFAULT_PATH)
find_path(BROTLI_INCLUDE_DECODE_DIR NAMES decode.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/brotli NO_DEFAULT_PATH)
find_path(LIZARD_INCLUDE_DIR NAMES lizard_compress.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include/lizard NO_DEFAULT_PATH)
find_path(BORINGSSL_INCLUDE_DIR NAMES openssl/opensslconf.h PATHS ${CMAKE_SOURCE_DIR}/third_party/include NO_DEFAULT_PATH)

# Поиск библиотеки Dependence
find_library(DEPEND_LIBRARY NAMES dependence PATHS ${CMAKE_SOURCE_DIR}/third_party/lib NO_DEFAULT_PATH)

# Подключаем 'FindPackageHandle' для использования модуля поиска (find_package(<PackageName>))
include(FindPackageHandleStandardArgs)

# Если операцинная система относится к MS Windows
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    # Выполняем проверку на существование зависимостей
    find_package_handle_standard_args(Dependence REQUIRED_VARS
        DEPEND_LIBRARY
        LZ4_INCLUDE_DIR
        BZ2_INCLUDE_DIR
        ZSTD_INCLUDE_DIR
        LZMA_INCLUDE_DIR
        ZLIB_INCLUDE_DIR
        SNAPPY_INCLUDE_DIR
        LIZARD_INCLUDE_DIR
        DENSITY_INCLUDE_DIR
        BROTLI_INCLUDE_ENCODE_DIR
        BROTLI_INCLUDE_DECODE_DIR
        BORINGSSL_INCLUDE_DIR

        FAIL_MESSAGE "Missing Dependence. Run ./build_third_party.sh first"
    )
    # Формируем список заголовочных файлов
    SET(DEPEND_INCLUDE_DIRS
        ${LZ4_INCLUDE_DIR}
        ${BZ2_INCLUDE_DIR}
        ${ZSTD_INCLUDE_DIR}
        ${LZMA_INCLUDE_DIR}
        ${ZLIB_INCLUDE_DIR}
        ${SNAPPY_INCLUDE_DIR}
        ${LIZARD_INCLUDE_DIR}
        ${DENSITY_INCLUDE_DIR}
        ##
         # Каталог заголовков Brotli в список НЕ входит намеренно: обращения к нему
         # идут с приставкой - «brotli/encode.h», «brotli/decode.h», - и разрешаются
         # каталогом BORINGSSL_INCLUDE_DIR, который и есть «third_party/include»
         #
         # Выставленный же наружу, этот каталог заслоняет системные заголовки своими:
         # в нём лежит «port.h», и у Sun Solaris с illumos он подменял собой заголовок
         # очереди оповещений ядра. Сборка при этом валилась не на подмене, а на
         # неизвестных именах вроде port_associate, и причина была вовсе не видна
         ##
        ${BORINGSSL_INCLUDE_DIR}
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
        SNAPPY_INCLUDE_DIR
        LIZARD_INCLUDE_DIR
        DENSITY_INCLUDE_DIR
        BROTLI_INCLUDE_ENCODE_DIR
        BROTLI_INCLUDE_DECODE_DIR
        BORINGSSL_INCLUDE_DIR

        FAIL_MESSAGE "Missing Dependence. Run ./build_third_party.sh first"
    )
    # Формируем список заголовочных файлов
    SET(DEPEND_INCLUDE_DIRS
        ${LZ4_INCLUDE_DIR}
        ${BZ2_INCLUDE_DIR}
        ${ZSTD_INCLUDE_DIR}
        ${LZMA_INCLUDE_DIR}
        ${ZLIB_INCLUDE_DIR}
        ${SNAPPY_INCLUDE_DIR}
        ${LIZARD_INCLUDE_DIR}
        ${DENSITY_INCLUDE_DIR}
        ##
         # Каталог заголовков Brotli в список НЕ входит намеренно: обращения к нему
         # идут с приставкой - «brotli/encode.h», «brotli/decode.h», - и разрешаются
         # каталогом BORINGSSL_INCLUDE_DIR, который и есть «third_party/include»
         #
         # Выставленный же наружу, этот каталог заслоняет системные заголовки своими:
         # в нём лежит «port.h», и у Sun Solaris с illumos он подменял собой заголовок
         # очереди оповещений ядра. Сборка при этом валилась не на подмене, а на
         # неизвестных именах вроде port_associate, и причина была вовсе не видна
         ##
        ${BORINGSSL_INCLUDE_DIR}
    )
endif()

# Выполняем установку оставшихся заголовочных файлов зависимостей
install(DIRECTORY "${LZ4_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${BZ2_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${ZSTD_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${LZMA_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${ZLIB_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${SNAPPY_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${LIZARD_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${DENSITY_INCLUDE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${BROTLI_INCLUDE_ENCODE_DIR}" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${BORINGSSL_INCLUDE_DIR}/openssl" DESTINATION "${CMAKE_INSTALL_PREFIX}/include" FILES_MATCHING PATTERN "*.h")

# Возвращаем правила поиска, действовавшие до нас
#
# Возврат стоит здесь, а не у потребителей: своё сужение обязан снимать тот, кто его
# завёл, - иначе всякому поиску во всём дереве пришлось бы знать о нём и защищаться
SET(CMAKE_FIND_LIBRARY_PREFIXES ${AWH_FIND_LIBRARY_PREFIXES_KEPT})
SET(CMAKE_FIND_LIBRARY_SUFFIXES ${AWH_FIND_LIBRARY_SUFFIXES_KEPT})
SET(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH ${AWH_FIND_USE_SYSTEM_ENVIRONMENT_PATH_KEPT})
