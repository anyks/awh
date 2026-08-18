#!/bin/sh
#
# @file build.sh
# @date 2026-08-16
#
# @license{LicenseRef-AWH-1.0}
#
# @author Yuriy Lobarev
#
# @brief Сборка стенда сличения разбора YAML с эталоном вместе с получением корпуса
#
# @details Кодек YAML опирается лишь на заголовочные файлы разбора чисел, и собрать
#          стенд можно шестью вызовами собирателя, не собирая библиотеки целиком
#
# @note Корпус забирается из набора yaml-test-suite - общепризнанного собрания текстов,
#       где для всякого известно, обязан ли разбор принять его, отвергнуть либо волен
#       решать сам. Забирается он единожды: при наличии каталога загрузка пропускается
#
# @copyright Copyright © 2026
#
# Вызов:
#   tools/verify/codec/yaml/build.sh [каталог сборки]
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
OUTPUT="${1:-/tmp/verify-yaml}"

# Получаем собиратель
COMPILER="${CXX:-c++}"

# Собираем ключи сборки стенда
OPTIONS="-O2 -std=c++20 -I$ROOT/include $FLAGS"

# Выполняем заведение каталога собранного стенда
mkdir -p "$OUTPUT"

# Выполняем снос прежде собранного стенда
#
# @note Снос обязателен: при отказе сборки прежний двоичный файл остаётся на месте
#       и прогон отчитывается успехом по коду, какого в нём уже нет
rm -f "$OUTPUT/events" "$OUTPUT/events.exe"

# Если корпус ещё не получен
if [ ! -d "$OUTPUT/corpus" ]; then
	# Выводим сообщение о получении корпуса
	echo "Забираем корпус yaml-test-suite"
	# Выполняем получение корпуса
	git clone --depth 1 -q https://github.com/yaml/yaml-test-suite "$OUTPUT/suite"
	# Выполняем заведение каталога корпуса
	mkdir -p "$OUTPUT/corpus"
	# Выполняем перенос описаний случаев в каталог корпуса
	cp "$OUTPUT/suite/src/"*.yaml "$OUTPUT/corpus/"
fi

# Собираем перечень объектных файлов стенда
OBJECTS="$OUTPUT/lexical-table.o"

# Выводим сообщение о начале сборки стенда
echo "Собираем стенд сличения YAML: $COMPILER"

# Выполняем сборку таблицы степеней пятёрки модуля разбора чисел
$COMPILER $OPTIONS -c "$ROOT/src/num/lexical/table.cpp" -o "$OUTPUT/lexical-table.o"

# Выполняем перебор всех частей кодека YAML
for PART in common encoding reader writer document value; do
	# Выполняем сборку очередной части кодека YAML
	$COMPILER $OPTIONS -c "$ROOT/src/codec/yaml/$PART.cpp" -o "$OUTPUT/codec-$PART.o"
	# Добавляем собранное к перечню объектных файлов стенда
	OBJECTS="$OBJECTS $OUTPUT/codec-$PART.o"
done

# Выполняем сборку стенда сличения
#
# @note Объектные файлы перечисляются поимённо, а не маскою: посторонний объектный файл,
#       оставленный в каталоге сборки кем угодно, попадал бы в связывание и валил его
$COMPILER $OPTIONS "$ROOT/tools/verify/codec/yaml/events.cpp" $OBJECTS -o "$OUTPUT/events"

# Выводим сообщение об окончании сборки стенда
echo "Стенд собран: $OUTPUT/events"
