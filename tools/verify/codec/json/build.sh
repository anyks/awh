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
if [ ! -d "$OUTPUT/corpus" ]; then
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
OBJECTS="$OUTPUT/lexical-table.o"

# Выводим сообщение о начале сборки стенда
echo "Собираем стенд сличения JSON: $COMPILER"

# Выполняем сборку таблицы степеней пятёрки модуля разбора чисел
$COMPILER $OPTIONS -c "$ROOT/src/num/lexical/table.cpp" -o "$OUTPUT/lexical-table.o"

# Выполняем перебор всех частей кодека JSON
for PART in common encoding reader writer document; do
	# Выполняем сборку очередной части кодека JSON
	$COMPILER $OPTIONS -c "$ROOT/src/codec/json/$PART.cpp" -o "$OUTPUT/codec-$PART.o"
	# Добавляем собранное к перечню объектных файлов стенда
	OBJECTS="$OBJECTS $OUTPUT/codec-$PART.o"
done

# Выполняем сборку стенда сличения
#
# @note Объектные файлы перечисляются поимённо, а не маскою: посторонний объектный файл,
#       оставленный в каталоге сборки кем угодно, попадал бы в связывание и валил его
$COMPILER $OPTIONS "$ROOT/tools/verify/codec/json/verify.cpp" $OBJECTS -o "$OUTPUT/verify"

# Выводим сообщение об окончании сборки стенда
echo "Стенд собран: $OUTPUT/verify"
