#!/bin/sh
#
# @file stand.sh
# @date 2026-08-18
#
# @license{LicenseRef-AWH-1.0}
#
# @author Yuriy Lobarev
#
# @brief Отдельный стенд сличения кодеков — сборка проверок согласия договора между
#        кодеками JSON, XML и YAML без библиотеки целиком, ради прогона на стендах
#
# @details Расхождение договора невидимо изнутри кодека: набор всякого из них
#          самодостаточен и проходит целиком, тогда как неправ из них один. Оттого
#          сличение и вынесено отдельным стендом, а не оставлено в наборе одного кодека
#
# @note Стенд этот собирает три кодека разом и оттого дороже одиночных: полторы минуты
#       против полуминуты. Гонять его надлежит вместе с ними, а не вместо них
#
# @copyright Copyright © 2026
#
# Вызов:
#   tests/codec/stand.sh [корень дерева] [каталог сборки]
#
# Переменные окружения:
#   CXX        — собиратель, по умолчанию «c++»
#   GTEST_ROOT — корень набора GoogleTest, по умолчанию «/usr»
#   FLAGS      — добавочные ключи сборки
#

# Прекращаем работу при первом же отказе
set -e

# Получаем корень дерева исходных текстов
ROOT="${1:-$(cd "$(dirname "$0")/../.." && pwd)}"

# Получаем каталог собранного стенда
OUTPUT="${2:-/tmp/awh-contract-stand}"

# Получаем корень набора GoogleTest
GTEST="${GTEST_ROOT:-/usr}"

# Получаем собиратель
COMPILER="${CXX:-c++}"

# Собираем ключи сборки стенда
OPTIONS="-O2 -std=c++17 -I$ROOT/include -I$GTEST/include $FLAGS"

# Выполняем заведение каталога собранного стенда
mkdir -p "$OUTPUT"

# Выполняем снос прежде собранного стенда
#
# @note Снос обязателен: при отказе сборки прежний двоичный файл остаётся на месте
#       и прогон отчитывается успехом по коду, какого в нём уже нет
rm -f "$OUTPUT/contract-tests" "$OUTPUT/contract-tests.exe"

# Собираем перечень объектных файлов стенда
OBJECTS="$OUTPUT/lexical-table.o $OUTPUT/contract.o"

# Выводим сообщение о начале сборки стенда
echo "Собираем стенд сличения кодеков: $COMPILER"

# Выполняем сборку таблицы степеней пятёрки модуля разбора чисел
$COMPILER $OPTIONS -c "$ROOT/src/num/lexical/table.cpp" -o "$OUTPUT/lexical-table.o"

# Выполняем сборку проверок сличения кодеков
$COMPILER $OPTIONS -c "$ROOT/tests/codec/contract.cpp" -o "$OUTPUT/contract.o"

#
# Выполняем перебор всех сличаемых кодеков вместе с составом частей каждого
#
# @warning Перечень этот держится ВРУЧНУЮ и обязан отвечать перечню кодеков, какие зовёт
#          `contract.cpp`. Разойдись они - стенд не свяжется вовсе, а раскладка по
#          машинам напечатает «СБОРКА ОТКАЗАЛА» и пойдёт дальше: проверка, ради какой
#          стенд и заведён, окажется не прогнанной ни разу, и молчание это неотличимо от
#          согласия. Так и вышло при внесении TOML и INI в щуп
#
# @note Состав частей у кодеков разный: владеющее значение есть пока не у всех, и часть
#       `value` перечисляется лишь у тех, у кого она заведена
#
for ENTRY in "json:common encoding reader writer document value"              "xml:common encoding reader writer document value"              "yaml:common encoding reader writer document value"              "toml:common encoding reader writer document value"              "ini:common encoding reader writer document value"; do
	# Получаем название очередного сличаемого кодека
	CODEC="${ENTRY%%:*}"
	# Выполняем перебор всех частей очередного кодека
	for PART in ${ENTRY#*:}; do
		# Выполняем сборку очередной части кодека
		$COMPILER $OPTIONS -c "$ROOT/src/codec/$CODEC/$PART.cpp" -o "$OUTPUT/$CODEC-$PART.o"
		# Добавляем собранное к перечню объектных файлов стенда
		OBJECTS="$OBJECTS $OUTPUT/$CODEC-$PART.o"
	done
done

# Выполняем связывание стенда сличения
#
# @note Объектные файлы перечисляются поимённо, а не маскою: посторонний объектный файл,
#       оставленный в каталоге сборки кем угодно, попадал бы в связывание и валил его
#       повтором имён
$COMPILER $OPTIONS $OBJECTS -L"$GTEST/lib" -lgtest -lgtest_main -pthread -o "$OUTPUT/contract-tests"

# Выводим сообщение об окончании сборки стенда
echo "Стенд собран: $OUTPUT/contract-tests"
