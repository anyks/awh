# Сборка бенчмарков производительности

# Каталог оснастки счёта системных вызовов
SET(AWH_SYSCOUNT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../tools/benchmark/syscount)

# Подставная библиотека счёта системных вызовов написана на чистом C, а библиотека
# собирается только как C++, поэтому язык включается здесь. Причина выбора языка в
# самой библиотеке: она описывает двоичный контракт, общий с эталонными стендами
# конкурентов, и любые особенности имён и раскладки C++ этому контракту вредят
enable_language(C)

# Двоичный контракт счётчика подключается точкой входа набора, поэтому путь к нему
# нужен всем модулям бенчмарков без исключения
include_directories(${AWH_SYSCOUNT_DIR})

# Счётчик системных вызовов заводится только там, где работает способ подмены
#
# Подмена выполняется загрузчиком: `LD_PRELOAD` у Linux и `DYLD_INSERT_LIBRARIES` у
# macOS. У MS Windows ни того, ни другого нет, а сам счётчик опирается на заголовки
# POSIX, каких там не бывает - сборка валилась отказом «ifaddrs.h: No such file or
# directory», и цель эта останавливала сборку ВСЕГО набора замеров. Набор отсутствие
# счётчика переносит: он разыскивает точку входа во время выполнения и при её
# отсутствии печатает «нет счётчика системных вызовов». Обнаружено на стенде MSYS2
# MinGW64 (14.08.2026)
# Название цели счётчика для ссылок из наборов: пустое там, где цель не заводится
SET(AWH_SYSCOUNT_TARGET "")

if (NOT (WIN32 OR MINGW OR CYGWIN))
    SET(AWH_SYSCOUNT_TARGET awh_BENCHMARK_syscount)

    # Подставная библиотека счёта системных вызовов. Собирается именно разделяемой
    # библиотекой, а не модулем: загрузчик macOS внедряет через DYLD_INSERT_LIBRARIES
    # только полноценные динамические библиотеки, а собранный модулем пакет (bundle)
    # отвергает. С набором она при этом не связывается ни на каком этапе - внедряется
    # в уже собранный процесс, а набор отыскивает её точку входа во время выполнения
    add_library(awh_BENCHMARK_syscount SHARED ${AWH_SYSCOUNT_DIR}/syscount.c)

    # Подставная библиотека написана на чистом C: режим Objective-C++, включённый для
    # остальных целей набора, к ней не применяется
    set_target_properties(
        awh_BENCHMARK_syscount PROPERTIES
        PREFIX "lib"
        OUTPUT_NAME "syscount"
        SUFFIX "${CMAKE_SHARED_LIBRARY_SUFFIX}"
        C_STANDARD 11
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/benchmarks
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/benchmarks
    )

    # Наружу выставляется единственный символ точки входа: всё остальное скрыто, чтобы
    # подмена не задевала символы измеряемого процесса. Исключение - сами подменяющие
    # функции на Linux: там подмена выполняется совпадением имени, и скрытый символ
    # подменять нечего, оттого им открытая видимость и проставлена поимённо
    target_compile_options(awh_BENCHMARK_syscount PRIVATE -O2 -Wall -Wextra -fvisibility=hidden)

    # Разыскание подлинных функций на Linux выполняется средствами загрузчика: подмена
    # там делается совпадением имени, и подлинная функция по имени недоступна. У macOS
    # подмена идёт таблицей загрузчика, подлинная функция доступна по имени, и
    # библиотека разыскания ему не нужна вовсе
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        target_link_libraries(awh_BENCHMARK_syscount ${CMAKE_DL_LIBS})
    endif()
endif()

add_subdirectory(sys)
add_subdirectory(cryptography)
add_subdirectory(net)
add_subdirectory(proto)
add_subdirectory(num)
add_subdirectory(codec)
add_subdirectory(regex)
