#!/usr/bin/env bash

# Получаем корневую дирректорию репозитория (каталог скрипта на три уровня ниже)
readonly ROOT=$(cd "$(dirname "$0")/../../.." && pwd)

# Каталог стендов сверки
readonly STANDS="$ROOT/tools/verify/encoding/idna"

# Каталог собранных стендов
readonly OUTPUT="${1:-/tmp/verify-idna}"

# Флаги сборки стендов
#
# Уровень оптимизации совпадает с уровнем сборки библиотеки в режиме Release:
# сверять реализации, собранные с разной оптимизацией, бессмысленно
readonly FLAGS="-std=c++2a -O3 -DNDEBUG -Wall -Wextra"

# Набор стендов сверки
readonly PLAIN="conformance"

# Если набор соответствия не получен
if [ ! -f "$ROOT/submodules/libidn2/tests/IdnaTest.txt" ]; then
	# Выводим сообщение о необходимости получения набора соответствия
	echo "IdnaTest.txt is missing, run:"
	echo "  ./sh/build_third_party.sh"
	exit 1
fi

# Выполняем создание каталога собранных стендов
mkdir -p "$OUTPUT" || exit 1

# Выполняем перебор стендов сверки
for STAND in $PLAIN; do
	# Выводим сообщение о сборке стенда сверки
	echo "Building stand: $STAND"
	# Выполняем сборку стенда сверки
	g++ $FLAGS \
		-I"$ROOT/include" \
		"$STANDS/$STAND.cpp" "$ROOT"/src/encoding/idna/*.cpp "$ROOT"/src/encoding/unicode/*.cpp \
		-o "$OUTPUT/$STAND" || exit 1
done

# Выводим сообщение о завершении сборки стендов сверки
echo ""
echo "Stands are built in $OUTPUT"
