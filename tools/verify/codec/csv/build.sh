#!/usr/bin/env bash

# Получаем корневую дирректорию репозитория (каталог скрипта на четыре уровня ниже)
readonly ROOT=$(cd "$(dirname "$0")/../../../.." && pwd)

# Каталог стендов сверки
readonly STANDS="$ROOT/tools/verify/codec/csv"

# Каталог собранных стендов
readonly OUTPUT="${1:-/tmp/verify-csv}"

# Флаги сборки стендов
#
# Уровень оптимизации совпадает с уровнем сборки библиотеки в режиме Release:
# сверять реализации, собранные с разной оптимизацией, бессмысленно
readonly FLAGS="-std=c++2a -O3 -DNDEBUG -Wall -Wextra"

# Набор стендов сверки
readonly PLAIN="dump"

# Выполняем создание каталога собранных стендов
mkdir -p "$OUTPUT" || exit 1

# Выполняем перебор стендов сверки
for STAND in $PLAIN; do
	# Выводим сообщение о сборке стенда сверки
	echo "Building stand: $STAND"
	# Выполняем сборку стенда сверки
	g++ $FLAGS \
		-Wno-c++11-narrowing \
		-I"$ROOT/include" \
		"$STANDS/$STAND.cpp" "$ROOT"/src/codec/csv/*.cpp "$ROOT"/src/num/lexical/*.cpp \
		"$ROOT"/src/sys/log.cpp "$ROOT"/src/sys/chrono.cpp "$ROOT"/src/sys/fmk.cpp \
		"$ROOT"/src/net/nwt.cpp "$ROOT"/src/encoding/unicode/*.cpp "$ROOT"/src/encoding/charset/*.cpp \
		"$ROOT"/src/alloc/*.cpp "$ROOT"/src/alloc/capture/*.cpp -lz \
		-o "$OUTPUT/$STAND" || exit 1
done

# Выполняем составление корпуса таблиц для сверки
echo "Building corpus"
python3 "$STANDS/corpus.py" "$OUTPUT/corpus" || exit 1

# Выводим сообщение о завершении сборки стендов сверки
echo ""
echo "Stands are built in $OUTPUT"
echo "Run: python3 $STANDS/compare.py $OUTPUT/corpus $OUTPUT/dump"
