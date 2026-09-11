#!/bin/sh
#
# @file stand.sh
# @date 2026-08-19
#
# @license{LicenseRef-AWH-1.0}
#
# @author Yuriy Lobarev
#
# @brief Отдельный стенд проверок кодека TOML — сборка набора проверок без библиотеки
#        целиком, ради прогона на отладочных стендах
#
# @details Кодек TOML опирается лишь на заголовочные файлы разбора чисел, и собрать его
#          проверки можно пятью вызовами собирателя. Полная сборка библиотеки на стендах
#          занимает десятки минут и тянет за собою третью сторону, тогда как проверить
#          требуется один кодек
#
# @note Из тела библиотеки нужен один-единственный файл — таблица степеней пятёрки
#       модуля разбора чисел: она лежит в теле, а не в заголовке
#
# @warning Перечень частей кодека держится здесь ВРУЧНУЮ, а не маскою: часть, к перечню
#          не приписанная, из стенда молча выпадает, и прогон отчитывается успехом по
#          коду, какого в нём нет. Заведётся у кодека владеющее значение — приписать
#          сюда `value` обеими строками, и в кодеке, и в проверках
#
# @copyright Copyright © 2026
#
# Вызов:
#   tests/codec/toml/stand.sh [корень дерева] [каталог сборки]
#
# Переменные окружения:
#   CXX        — собиратель, по умолчанию «c++»
#   GTEST_ROOT — корень набора GoogleTest, по умолчанию «/usr»
#   FLAGS      — добавочные ключи сборки
#

# Прекращаем работу при первом же отказе
set -e

# Получаем корень дерева исходных текстов
ROOT="${1:-$(cd "$(dirname "$0")/../../.." && pwd)}"

# Получаем каталог собранного стенда
OUTPUT="${2:-/tmp/awh-toml-stand}"

# Получаем корень набора GoogleTest
GTEST="${GTEST_ROOT:-/usr}"

# Получаем собиратель
COMPILER="${CXX:-c++}"

# Собираем ключи сборки стенда
OPTIONS="-O2 -std=c++17 -I$ROOT/include -I$GTEST/include $FLAGS"

#
# Путь к библиотеке языка C++ того собирателя, каким собран стенд
#
# @note Путь этот прописывается в двоичный файл: у DragonFly рядом стоят несколько
#       собирателей, и стенд, собранный `g++14`, при запуске подхватывал `libstdc++`
#       от gcc11 и отваливался с «version GLIBCXX_3.4.32 not found». У проверок беда
#       эта хуже, чем у замеров: незапустившийся стенд отчитывается отказом прогона, и
#       дефект пойдут искать в кодеке, какого там нет
#
# @warning Путь берётся лишь тогда, когда собиратель отдаёт его полным: `clang` на
#          выдачу этого вопроса отвечает одним лишь именем файла, и `dirname` от него
#          дал бы текущий каталог
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
rm -f "$OUTPUT/toml-tests" "$OUTPUT/toml-tests.exe"

# Собираем перечень объектных файлов стенда
OBJECTS="$OUTPUT/lexical-table.o $OUTPUT/sys-log.o $OUTPUT/sys-chrono.o $OUTPUT/sys-fmk.o $OUTPUT/sys-fs.o $OUTPUT/sys-os.o $OUTPUT/charset.o $OUTPUT/charset-table.o $OUTPUT/net-nwt.o $OUTPUT/alloc-alloc.o $OUTPUT/alloc-cache.o $OUTPUT/alloc-central.o $OUTPUT/alloc-classes.o $OUTPUT/alloc-guard.o $OUTPUT/alloc-huge.o $OUTPUT/alloc-link.o $OUTPUT/alloc-pages.o $OUTPUT/alloc-profile.o $OUTPUT/alloc-source.o $OUTPUT/alloc-spin.o $OUTPUT/alloc-trace.o $OUTPUT/alloc-elf.o $OUTPUT/alloc-mach.o $OUTPUT/alloc-pe.o $OUTPUT/uni-normalize.o $OUTPUT/uni-table.o $OUTPUT/uni-unicode.o $OUTPUT/uni-utf8.o"

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
	#
	# @note Разбор ярлыков в «src/sys/fs.cpp» поднимает COM - «CoCreateInstance»
	#       живёт в «ole32», а опознаватели «IID_IShellLinkW» и «IID_IPersistFile»
	#       в «uuid». Без обеих связывание отказывает десятью нераскрытыми именами
	#
	MINGW*|MSYS*|CYGWIN*) SYSTEM_LIBS="-lws2_32 -lole32 -luuid" ;;
	#
	# @note Разбор alias-файлов в «src/sys/fs.cpp» зовёт Foundation, и без неё
	#       связывание отказывает на средствах Objective-C
	#
	Darwin) SYSTEM_LIBS="-framework Foundation" ;;
	SunOS) SYSTEM_LIBS="-lsocket -lnsl" ;;
	*) SYSTEM_LIBS="" ;;
esac

# Выводим сообщение о начале сборки стенда
echo "Собираем стенд проверок TOML: $COMPILER"

# Выполняем сборку таблицы степеней пятёрки модуля разбора чисел
$COMPILER $OPTIONS -c "$ROOT/src/num/lexical/table.cpp" -o "$OUTPUT/lexical-table.o"

#
# Выполняем сборку ведения журнала работы и опоры его на средства системы
#
# @note Кодек сообщает об отказах разбора в журнал фреймворка, и стенд обязан нести
#       его с собою. Иного пути нет: договор кодека принимает «log_t», и подделка его
#       заглушкою мерила бы не тот код, что собирается в библиотеку
#
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/log.cpp" -o "$OUTPUT/sys-log.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/chrono.cpp" -o "$OUTPUT/sys-chrono.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/fmk.cpp" -o "$OUTPUT/sys-fmk.o"
##
# Ход «fs_t» зовёт «os_t» при смене владельца файла, и без «src/sys/os.cpp» связывание
# отказывает на средствах опознания пользователя и группы
##
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/os.cpp" -o "$OUTPUT/sys-os.o"
##
# Работа с файловой системой ведётся ходом «fs_t», и стенд обязан его собирать: с 11.09.2026
# кодеки INI, TOML и YAML зовут «sys/fs» вместо прямых ходов к файловой системе
#
# @warning Под macOS файл этот собирается как Objective-C++, а не как C++: разбор
#          alias-файлов зовёт Foundation, и сборка обычным ходом валится сотнями
#          отказов в системных заголовках. Отбор этот повторяет CMakeLists.txt, где
#          тому же файлу и только ему назначены «-x objective-c++ -fobjc-arc»
##
if [ "$(uname -s)" = "Darwin" ]; then
	$COMPILER $OPTIONS -Wno-c++11-narrowing -x objective-c++ -fobjc-arc -c "$ROOT/src/sys/fs.cpp" -o "$OUTPUT/sys-fs.o"
else
	$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/fs.cpp" -o "$OUTPUT/sys-fs.o"
fi
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

##
# Сличаем перечень частей кодека с деревом исходных текстов
#
# Перечень держится вручную, и часть, к нему не приписанная, из стенда молча выпадает:
# прогон отчитывается успехом по коду, какого в нём нет. Предупреждение об этом стояло
# здесь с самого начала - теперь оно проверяется
##
PARTS="common encoding reader writer document value"
PRESENT=$(ls "$ROOT/src/codec/toml"/*.cpp | while read -r FILE; do basename "$FILE" .cpp; done | sort | tr '\n' ' ')
LISTED=$(for PART in $PARTS; do echo "$PART"; done | sort | tr '\n' ' ')
if [ "$PRESENT" != "$LISTED" ]; then
	echo "Перечень частей кодека разошёлся с деревом исходных текстов" >&2
	echo "  в стенде: $LISTED" >&2
	echo "  в дереве: $PRESENT" >&2
	exit 5
fi

# Выполняем перебор всех частей кодека TOML
for PART in $PARTS; do
	# Выполняем сборку очередной части кодека TOML
	$COMPILER $OPTIONS -c "$ROOT/src/codec/toml/$PART.cpp" -o "$OUTPUT/codec-$PART.o"
	# Выполняем сборку проверок очередной части кодека TOML
	$COMPILER $OPTIONS -c "$ROOT/tests/codec/toml/$PART.cpp" -o "$OUTPUT/test-$PART.o"
	# Добавляем собранное к перечню объектных файлов стенда
	OBJECTS="$OBJECTS $OUTPUT/codec-$PART.o $OUTPUT/test-$PART.o"
done

##
# Собираем общие части кодеков, к самому кодеку не принадлежащие
#
# Части эти лежат в «src/codec» и делятся между кодеками
#
# @note Часть «replace» изъята 11.09.2026: модуль «codec/replace» удалён владельцем, а
#       подмена целевого файла временным ведётся ныне ходом «fs_t::replaceAddress».
#       Перечень этот держится ВРУЧНУЮ, и оттого снос модуля валит стенд связыванием -
#       маска подхватила бы всякий посторонний файл, потому перечень и ручной
##
SHARED="numeric"

# Выполняем перебор всех общих частей кодеков
for PART in $SHARED; do
	# Выполняем сборку очередной общей части кодеков
	$COMPILER $OPTIONS -c "$ROOT/src/codec/$PART.cpp" -o "$OUTPUT/shared-$PART.o"
	# Добавляем собранное к перечню объектных файлов стенда
	OBJECTS="$OBJECTS $OUTPUT/shared-$PART.o"
done

# Выполняем связывание стенда проверок
#
# @note Объектные файлы перечисляются поимённо, а не маскою: посторонний объектный файл,
#       оставленный в каталоге сборки кем угодно, попадал бы в связывание и валил его
#       повтором имён
$COMPILER $OPTIONS $OBJECTS -L"$GTEST/lib" -lgtest -lgtest_main -pthread $SYSTEM_LIBS -lz -o "$OUTPUT/toml-tests"

# Выводим сообщение об окончании сборки стенда
echo "Стенд собран: $OUTPUT/toml-tests"
