#!/bin/sh
#
# @file stand.sh
# @date 2026-08-19
#
# @license{LicenseRef-AWH-1.0}
#
# @author Yuriy Lobarev
#
# @brief Отдельный стенд замеров кодека JSON — сборка набора замеров без библиотеки
#        целиком, ради снятия показателей на отладочных стендах
#
# @details Пороги набора замеров назначаются по САМОЙ МЕДЛЕННОЙ машине, а не по рабочей:
#          порог, снятый на быстрой, отказывает на медленной без всякой регрессии.
#          Полная же сборка библиотеки на стендах занимает десятки минут и тянет за
#          собою третью сторону, тогда как замерить требуется один кодек
#
# @note Каркас замеров опирается лишь на заголовок счётчика системных вызовов, а сам
#       счётчик разыскивается во время выполнения: без него набор печатает «нет
#       счётчика системных вызовов» и работает дальше
#
# @warning Замеры собираются только с оптимизацией: отладочная сборка замедляет всё
#          равномерно и изображает точечную регрессию там, где её нет
#
# @copyright Copyright © 2026
#
# Вызов:
#   benchmark/codec/json/stand.sh [корень дерева] [каталог сборки]
#
# Переменные окружения:
#   CXX   — собиратель, по умолчанию «c++»
#   FLAGS — добавочные ключи сборки
#

# Прекращаем работу при первом же отказе
set -e

# Получаем корень дерева исходных текстов
ROOT="${1:-$(cd "$(dirname "$0")/../../.." && pwd)}"

# Получаем каталог собранного стенда
OUTPUT="${2:-/tmp/awh-json-bench}"

# Получаем собиратель
COMPILER="${CXX:-c++}"

# Собираем ключи сборки стенда
OPTIONS="-O2 -std=c++17 -I$ROOT/include -I$ROOT/tools/benchmark/syscount $FLAGS"

#
# Выполняем прописывание пути к библиотеке стандартных средств собирателя
#
# @details Собранное одним собирателем при запуске подхватывает библиотеку стандартных
#          средств ИНОГО, если тот стоит в путях поиска раньше. У DragonFly это и
#          случается: код собирается `g++14` из портов, а при запуске подхватывается
#          `libstdc++` от `gcc11`, и двоичный файл валится с «version GLIBCXX_3.4.32
#          required ... not found». Сборка при этом проходит успешно, и отказ виден лишь
#          при запуске
#
# @warning Оговорка в `case` обязательна и молчаливую порчу предотвращает: `clang`
#          отвечает на тот же вопрос ОДНИМ ИМЕНЕМ файла без пути, и `dirname` от него дал
#          бы текущий каталог. Прописав его путём поиска, мы получили бы двоичный файл,
#          ищущий библиотеку рядом с собою, - то есть замену одной беды на другую, худшую
#
# @note Отказ этот молчалив по существу раскладки: машина отчитывается пройденной, а
#       показателей с неё ноль, и дно считается по одиннадцати машинам вместо двенадцати
#
STDLIB="$($COMPILER -print-file-name=libstdc++.so 2>/dev/null)"
case "$STDLIB" in
	/*) OPTIONS="$OPTIONS -Wl,-rpath,$(cd "$(dirname "$STDLIB")" && pwd)" ;;
esac

# Выполняем заведение каталога собранного стенда
mkdir -p "$OUTPUT"

# Выполняем снос прежде собранного стенда
#
# @note Снос обязателен: при отказе сборки прежний двоичный файл остаётся на месте
#       и прогон отчитывается успехом по коду, какого в нём уже нет
rm -f "$OUTPUT/json-bench" "$OUTPUT/json-bench.exe"

# Собираем перечень объектных файлов стенда
OBJECTS="$OUTPUT/lexical-table.o $OUTPUT/main.o $OUTPUT/sys-log.o $OUTPUT/sys-chrono.o $OUTPUT/sys-fmk.o $OUTPUT/net-nwt.o $OUTPUT/uni-normalize.o $OUTPUT/uni-table.o $OUTPUT/uni-unicode.o $OUTPUT/uni-utf8.o $OUTPUT/alloc-alloc.o $OUTPUT/alloc-cache.o $OUTPUT/alloc-central.o $OUTPUT/alloc-classes.o $OUTPUT/alloc-guard.o $OUTPUT/alloc-huge.o $OUTPUT/alloc-link.o $OUTPUT/alloc-pages.o $OUTPUT/alloc-profile.o $OUTPUT/alloc-source.o $OUTPUT/alloc-spin.o $OUTPUT/alloc-trace.o $OUTPUT/alloc-elf.o $OUTPUT/alloc-mach.o $OUTPUT/alloc-pe.o $OUTPUT/charset.o $OUTPUT/charset-table.o"

##
# Внутренние имена распределителя libc берутся ТОЛЬКО под OpenBSD
#
# Файл «src/alloc/capture/obsd.cpp» собственной охраны по системе не несёт - её
# несёт сборщик: CMakeLists.txt подключает его в перечень исходных текстов лишь
# при `CMAKE_SYSTEM_NAME STREQUAL "OpenBSD"`. Стенд обязан повторять этот отбор:
# собранный безусловно, он под MinGW валит связывание по `posix_memalign` и
# `aligned_alloc`, каких у той библиотеки времени исполнения нет вовсе
##
if [ "$(uname -s)" = "OpenBSD" ]; then
	OBJECTS="$OBJECTS $OUTPUT/alloc-obsd.o"
fi

##
# Системные библиотеки, каких требует ядро библиотеки
#
# У MS Windows журнал зовёт `WSAGetLastError`: посредник `__awh_strerror__` разбирает
# сетевые коды отказов, каких `strerror` от MinGW не знает. Живёт этот вызов в
# «ws2_32», и без неё связывание стенда отказывает
##
case "$(uname -s)" in
	MINGW*|MSYS*|CYGWIN*) SYSTEM_LIBS="-lws2_32" ;;
	*) SYSTEM_LIBS="" ;;
esac

# Выводим сообщение о начале сборки стенда
echo "Собираем стенд замеров JSON: $COMPILER"

# Выполняем сборку таблицы степеней пятёрки модуля разбора чисел
$COMPILER $OPTIONS -c "$ROOT/src/num/lexical/table.cpp" -o "$OUTPUT/lexical-table.o"

#
# Выполняем сборку ведения журнала работы и опор его
#
# @note Кодек сообщает об отказах разбора и записи в журнал фреймворка, и стенд обязан
#       нести его с собою вместе со всем, на что он опирается
# @warning Ключ «-Wno-c++11-narrowing» приложен только к ЧУЖИМ файлам: свои им глушить
#          нельзя, сужение у себя обязано оставаться отказом сборки
#
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/log.cpp" -o "$OUTPUT/sys-log.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/chrono.cpp" -o "$OUTPUT/sys-chrono.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/fmk.cpp" -o "$OUTPUT/sys-fmk.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/net/nwt.cpp" -o "$OUTPUT/net-nwt.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/unicode/normalize.cpp" -o "$OUTPUT/uni-normalize.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/unicode/table.cpp" -o "$OUTPUT/uni-table.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/unicode/unicode.cpp" -o "$OUTPUT/uni-unicode.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/unicode/utf8.cpp" -o "$OUTPUT/uni-utf8.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/alloc.cpp" -o "$OUTPUT/alloc-alloc.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/cache.cpp" -o "$OUTPUT/alloc-cache.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/central.cpp" -o "$OUTPUT/alloc-central.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/classes.cpp" -o "$OUTPUT/alloc-classes.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/guard.cpp" -o "$OUTPUT/alloc-guard.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/huge.cpp" -o "$OUTPUT/alloc-huge.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/link.cpp" -o "$OUTPUT/alloc-link.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/pages.cpp" -o "$OUTPUT/alloc-pages.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/profile.cpp" -o "$OUTPUT/alloc-profile.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/source.cpp" -o "$OUTPUT/alloc-source.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/spin.cpp" -o "$OUTPUT/alloc-spin.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/trace.cpp" -o "$OUTPUT/alloc-trace.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/capture/elf.cpp" -o "$OUTPUT/alloc-elf.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/capture/mach.cpp" -o "$OUTPUT/alloc-mach.o"
if [ "$(uname -s)" = "OpenBSD" ]; then
	$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/capture/obsd.cpp" -o "$OUTPUT/alloc-obsd.o"
fi
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/capture/pe.cpp" -o "$OUTPUT/alloc-pe.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/charset/charset.cpp" -o "$OUTPUT/charset.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/charset/table.cpp" -o "$OUTPUT/charset-table.o"

# Выполняем сборку точки входа набора замеров
$COMPILER $OPTIONS -c "$ROOT/benchmark/main.cpp" -o "$OUTPUT/main.o"

# Выполняем перебор всех частей кодека JSON
for PART in common encoding reader writer document value; do
	# Выполняем сборку очередной части кодека JSON
	$COMPILER $OPTIONS -c "$ROOT/src/codec/json/$PART.cpp" -o "$OUTPUT/codec-$PART.o"
	# Добавляем собранное к перечню объектных файлов стенда
	OBJECTS="$OBJECTS $OUTPUT/codec-$PART.o"
done

# Выполняем перебор всех частей набора замеров кодека JSON
for PART in json reader writer document value; do
	# Выполняем сборку очередной части набора замеров
	$COMPILER $OPTIONS -c "$ROOT/benchmark/codec/json/$PART.cpp" -o "$OUTPUT/bench-$PART.o"
	# Добавляем собранное к перечню объектных файлов стенда
	OBJECTS="$OBJECTS $OUTPUT/bench-$PART.o"
done

# Выполняем связывание стенда замеров
#
# @note Объектные файлы перечисляются поимённо, а не маскою: посторонний объектный файл,
#       оставленный в каталоге сборки кем угодно, попадал бы в связывание и валил его
#       повтором имён
$COMPILER $OPTIONS $OBJECTS -pthread $SYSTEM_LIBS -lz -o "$OUTPUT/json-bench"

# Выводим сообщение об окончании сборки стенда
echo "Стенд собран: $OUTPUT/json-bench"
