#!/bin/sh
#
# @file build.sh
# @date 2026-08-23
#
# @license{LicenseRef-AWH-1.0}
#
# @author Yuriy Lobarev
#
# @brief Сборка стенда сличения разбора XML с эталоном вместе с получением корпуса
#
# @details Кодек XML опирается лишь на заголовочные файлы разбора чисел, и собрать стенд
#          можно без сборки библиотеки целиком
#
# @note Корпус забирается из набора соответствия W3C - собрания из 2238 случаев, где для
#       всякого объявлено, обязан ли разбор принять его, отвергнуть либо волен решать сам.
#       Забирается он единожды: при непустом каталоге загрузка пропускается
#
# @warning Набор раздаётся одним слепком, а не хранилищем: забирается он загрузкою по
#          сети, и без выхода наружу стенд собрать нельзя
#
# @copyright Copyright © 2026
#
# Вызов:
#   tools/verify/codec/xml/build.sh [каталог сборки]
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
OUTPUT="${1:-/tmp/verify-xml}"

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
	echo "Забираем корпус соответствия W3C"
	# Выполняем заведение каталога корпуса
	mkdir -p "$OUTPUT/corpus"
	# Выполняем получение слепка набора соответствия
	curl -sSL -o "$OUTPUT/suite.zip" https://www.w3.org/XML/Test/xmlts20130923.zip
	# Выполняем распаковку слепка набора соответствия
	unzip -q -o "$OUTPUT/suite.zip" -d "$OUTPUT/corpus"
	# Выполняем снос слепка набора соответствия
	rm -f "$OUTPUT/suite.zip"
fi

# Собираем перечень объектных файлов стенда
#
# @note Кроме самого кодека стенд несёт ВЕДЕНИЕ ЖУРНАЛА и всё, на что оно опирается:
#       кодек сообщает об отказах разбора в журнал фреймворка, а тот тянет за собою часы,
#       оснастку, сетевые виды, кодировки и выделение памяти
OBJECTS="$OUTPUT/lexical-table.o $OUTPUT/sys-log.o $OUTPUT/sys-chrono.o $OUTPUT/sys-fmk.o $OUTPUT/net-nwt.o $OUTPUT/uni-normalize.o $OUTPUT/uni-table.o $OUTPUT/uni-unicode.o $OUTPUT/uni-utf8.o $OUTPUT/alloc-alloc.o $OUTPUT/alloc-cache.o $OUTPUT/alloc-central.o $OUTPUT/alloc-classes.o $OUTPUT/alloc-guard.o $OUTPUT/alloc-huge.o $OUTPUT/alloc-link.o $OUTPUT/alloc-pages.o $OUTPUT/alloc-profile.o $OUTPUT/alloc-source.o $OUTPUT/alloc-spin.o $OUTPUT/alloc-trace.o $OUTPUT/alloc-elf.o $OUTPUT/alloc-mach.o $OUTPUT/alloc-obsd.o $OUTPUT/alloc-pe.o $OUTPUT/charset.o $OUTPUT/charset-table.o"

# Выводим сообщение о начале сборки стенда
echo "Собираем стенд сличения XML: $COMPILER"

# Выполняем сборку таблицы степеней пятёрки модуля разбора чисел
$COMPILER $OPTIONS -c "$ROOT/src/num/lexical/table.cpp" -o "$OUTPUT/lexical-table.o"

#
# Выполняем сборку ведения журнала работы и опор его
#
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
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/capture/obsd.cpp" -o "$OUTPUT/alloc-obsd.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/capture/pe.cpp" -o "$OUTPUT/alloc-pe.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/charset/charset.cpp" -o "$OUTPUT/charset.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/charset/table.cpp" -o "$OUTPUT/charset-table.o"

# Выполняем перебор всех частей кодека XML
for PART in common encoding reader writer document value; do
	# Выполняем сборку очередной части кодека XML
	$COMPILER $OPTIONS -c "$ROOT/src/codec/xml/$PART.cpp" -o "$OUTPUT/codec-$PART.o"
	# Добавляем собранное к перечню объектных файлов стенда
	OBJECTS="$OBJECTS $OUTPUT/codec-$PART.o"
done

# Выполняем сборку стенда сличения
#
# @note Объектные файлы перечисляются поимённо, а не маскою: посторонний объектный файл,
#       оставленный в каталоге сборки кем угодно, попадал бы в связывание и валил его
$COMPILER $OPTIONS "$ROOT/tools/verify/codec/xml/verify.cpp" $OBJECTS -pthread -lz -o "$OUTPUT/verify"

# Выводим сообщение об окончании сборки стенда
echo "Стенд собран: $OUTPUT/verify"
