#!/bin/sh
#
# @file build.sh
# @date 2026-08-16
#
# @license{LicenseRef-AWH-1.0}
#
# @author Yuriy Lobarev
#
# @brief Сборка стенда сличения разбора JSON с эталоном вместе с получением корпуса
#
# @details Кодек JSON опирается лишь на заголовочные файлы разбора чисел, и собрать
#          стенд можно шестью вызовами собирателя, не собирая библиотеки целиком
#
# @note Корпус забирается из набора JSONTestSuite - общепризнанного собрания текстов,
#       где для всякого известно, обязан ли разбор принять его, отвергнуть либо волен
#       решать сам. Забирается он единожды: при наличии каталога загрузка пропускается
#
# @copyright Copyright © 2026
#
# Вызов:
#   tools/verify/codec/json/build.sh [каталог сборки]
#
# Переменные окружения:
#   CXX   — собиратель, по умолчанию «c++»
#   FLAGS — добавочные ключи сборки
#

# Прекращаем работу при первом же отказе
set -e

# Получаем корень дерева исходных текстов
ROOT=$(cd "$(dirname "$0")/../../../.." && pwd)

# Получаем каталог собранного стенда
OUTPUT="${1:-/tmp/verify-json}"

# Получаем собиратель
COMPILER="${CXX:-c++}"

# Собираем ключи сборки стенда
OPTIONS="-O2 -std=c++17 -I$ROOT/include $FLAGS"

# Выполняем заведение каталога собранного стенда
mkdir -p "$OUTPUT"

# Выполняем снос прежде собранного стенда
#
# @note Снос обязателен: при отказе сборки прежний двоичный файл остаётся на месте
#       и прогон отчитывается успехом по коду, какого в нём уже нет
rm -f "$OUTPUT/verify" "$OUTPUT/verify.exe"

# Если корпус ещё не получен
#
# @warning Судить по НАЛИЧИЮ каталога нельзя: пустой каталог, оставшийся от прерванной
#          загрузки, отменял забор корпуса, и поверка отчитывалась «расхождений нет»
#          по нулю сличённых текстов. Признаком служит непустота каталога
if [ -z "$(ls -A "$OUTPUT/corpus" 2>/dev/null)" ]; then
	# Выполняем снос остатков прерванной загрузки
	rm -rf "$OUTPUT/corpus" "$OUTPUT/transform" "$OUTPUT/suite"
	# Выводим сообщение о получении корпуса
	echo "Забираем корпус JSONTestSuite"
	# Выполняем получение корпуса
	git clone --depth 1 -q https://github.com/nst/JSONTestSuite "$OUTPUT/suite"
	# Выполняем заведение каталога корпуса
	mkdir -p "$OUTPUT/corpus"
	# Выполняем перенос текстов разбора в каталог корпуса
	cp "$OUTPUT/suite/test_parsing/"* "$OUTPUT/corpus/"
	# Выполняем заведение каталога набора преобразований
	mkdir -p "$OUTPUT/transform"
	# Выполняем перенос текстов преобразований в свой каталог
	cp "$OUTPUT/suite/test_transform/"* "$OUTPUT/transform/"
fi

# Собираем перечень объектных файлов стенда
#
# @note Кроме самого кодека стенд несёт ВЕДЕНИЕ ЖУРНАЛА и всё, на что оно опирается:
#       кодек сообщает об отказах разбора в журнал фреймворка, а тот тянет за собою часы,
#       оснастку, сетевые виды, кодировки и выделение памяти
OBJECTS="$OUTPUT/lexical-table.o $OUTPUT/codec-numeric.o $OUTPUT/sys-log.o $OUTPUT/sys-chrono.o $OUTPUT/sys-fmk.o $OUTPUT/net-nwt.o $OUTPUT/sys-fs.o $OUTPUT/sys-os.o $OUTPUT/sys-signals.o $OUTPUT/sys-procre.o $OUTPUT/net-addr.o $OUTPUT/net-net.o $OUTPUT/uni-normalize.o $OUTPUT/uni-table.o $OUTPUT/uni-unicode.o $OUTPUT/uni-utf8.o $OUTPUT/alloc-alloc.o $OUTPUT/alloc-cache.o $OUTPUT/alloc-central.o $OUTPUT/alloc-classes.o $OUTPUT/alloc-guard.o $OUTPUT/alloc-huge.o $OUTPUT/alloc-link.o $OUTPUT/alloc-pages.o $OUTPUT/alloc-profile.o $OUTPUT/alloc-source.o $OUTPUT/alloc-spin.o $OUTPUT/alloc-trace.o $OUTPUT/alloc-elf.o $OUTPUT/alloc-mach.o $OUTPUT/alloc-obsd.o $OUTPUT/alloc-pe.o $OUTPUT/charset.o $OUTPUT/charset-table.o"

# Выводим сообщение о начале сборки стенда
echo "Собираем стенд сличения JSON: $COMPILER"

# Выполняем сборку таблицы степеней пятёрки модуля разбора чисел
$COMPILER $OPTIONS -c "$ROOT/src/num/lexical/table.cpp" -o "$OUTPUT/lexical-table.o"

#
# Выполняем сборку ведения журнала работы и опор его
#
# @warning Ключ «-Wno-c++11-narrowing» приложен только к ЧУЖИМ файлам: свои им глушить
#          нельзя, сужение у себя обязано оставаться отказом сборки
#
#
# Сборка перевода чисел, общего всем кодекам
#
# @note Тела `awh::codec::convert` и `numeric` несёт `src/codec/numeric.cpp`, а
#       перечень частей тут ведётся ВРУЧНУЮ: без него связывание отвечает
#       отсутствием девяти тел разом
#
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/codec/numeric.cpp" -o "$OUTPUT/codec-numeric.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/log.cpp" -o "$OUTPUT/sys-log.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/chrono.cpp" -o "$OUTPUT/sys-chrono.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/fmk.cpp" -o "$OUTPUT/sys-fmk.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/net/nwt.cpp" -o "$OUTPUT/net-nwt.o"
#
# Сборка слоёв файловой системы, хода процессов и сети
#
# @note Кодеки читают и пишут файлы ходом `fs_t`, журнал опирается на сигналы, а
#       сигналы - на сведения о процессе. Перечень частей тут ведётся ВРУЧНУЮ и эти
#       файлы миновал: связывание отвечает телом за телом, начиная с
#       `awh::Filesystem::write`
#
# @warning Слой файловой системы у macOS написан на Objective-C++ и зовёт
#          `NSFileManager`: обычным C++ он не собирается, а связывается с основой
#          Foundation. Отбор этот повторяет CMakeLists.txt, где тем же файлам и
#          только им назначены эти ключи
#
if [ "$(uname -s)" = "Darwin" ]; then
	$COMPILER $OPTIONS -Wno-c++11-narrowing -x objective-c++ -fobjc-arc -c "$ROOT/src/sys/fs.cpp" -o "$OUTPUT/sys-fs.o"
else
	$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/fs.cpp" -o "$OUTPUT/sys-fs.o"
fi
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/os.cpp" -o "$OUTPUT/sys-os.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/signals.cpp" -o "$OUTPUT/sys-signals.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/procre.cpp" -o "$OUTPUT/sys-procre.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/net/addr.cpp" -o "$OUTPUT/net-addr.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/net/net.cpp" -o "$OUTPUT/net-net.o"
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
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/capture/obsd.cpp" -o "$OUTPUT/alloc-obsd.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/capture/pe.cpp" -o "$OUTPUT/alloc-pe.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/charset/charset.cpp" -o "$OUTPUT/charset.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/charset/table.cpp" -o "$OUTPUT/charset-table.o"

# Выполняем перебор всех частей кодека JSON
for PART in common encoding reader writer document value; do
	# Выполняем сборку очередной части кодека JSON
	$COMPILER $OPTIONS -c "$ROOT/src/codec/json/$PART.cpp" -o "$OUTPUT/codec-$PART.o"
	# Добавляем собранное к перечню объектных файлов стенда
	OBJECTS="$OBJECTS $OUTPUT/codec-$PART.o"
done

# Выполняем сборку стенда сличения
#
# @note Объектные файлы перечисляются поимённо, а не маскою: посторонний объектный файл,
#       оставленный в каталоге сборки кем угодно, попадал бы в связывание и валил его
#
# Библиотеки системы, связыванию потребные
#
# @note Winsock тянет `sys/log` из `FileSink::rotate()`, без него под MinGW сборка
#       валится на `undefined symbol: WSAGetLastError`; Foundation нужен слою
#       файловой системы macOS на `NSFileManager` да `NSURL`
#
case "$(uname -s)" in
	MINGW*|MSYS*|CYGWIN*) SYSTEM_LIBS="-lws2_32" ;;
	Darwin) SYSTEM_LIBS="-framework Foundation" ;;
	*) SYSTEM_LIBS="" ;;
esac

$COMPILER $OPTIONS "$ROOT/tools/verify/codec/json/verify.cpp" $OBJECTS $SYSTEM_LIBS -pthread -lz -o "$OUTPUT/verify"

# Выводим сообщение об окончании сборки стенда
echo "Стенд собран: $OUTPUT/verify"
