#!/usr/bin/env bash

# Получаем корневую дирректорию репозитория (каталог скрипта на три уровня ниже)
readonly ROOT=$(cd "$(dirname "$0")/../../.." && pwd)

# Каталог стендов сверки
readonly STANDS="$ROOT/tools/verify/unicode"

# Каталог собранных стендов
readonly OUTPUT="${1:-/tmp/verify-unicode}"

# Каталог собранной эталонной реализации
readonly VENDOR="$ROOT/third_party"

# Флаги сборки стендов
#
# Уровень оптимизации совпадает с уровнем сборки библиотеки в режиме Release:
# сверять реализации, собранные с разной оптимизацией, бессмысленно
readonly FLAGS="-std=c++2a -O3 -DNDEBUG -Wall -Wextra"

# Набор стендов сверки
readonly PLAIN="normalize"

# Если эталонная реализация не собрана
if [ ! -f "$VENDOR/lib/libdependence.a" ]; then
	# Выводим сообщение о необходимости сборки зависимостей
	echo "Reference libunistring is missing, run:"
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
		-I"$ROOT/include" -I"$VENDOR/include" \
		"$STANDS/$STAND.cpp" "$ROOT"/src/unicode/*.cpp \
		-o "$OUTPUT/$STAND" \
		"$VENDOR/lib/libdependence.a" || exit 1
done

# Выводим сообщение о завершении сборки стендов сверки
echo ""
echo "Stands are built in $OUTPUT"
