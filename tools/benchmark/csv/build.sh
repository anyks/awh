#!/usr/bin/env bash

# Получаем корневую дирректорию репозитория (каталог скрипта на три уровня ниже)
readonly ROOT=$(cd "$(dirname "$0")/../../.." && pwd)

# Каталог стендов сравнения
readonly STANDS="$ROOT/tools/benchmark/csv"

# Каталог исходных текстов сравниваемых реализаций
readonly VENDOR="$ROOT/submodules"

# Каталог собранных стендов
readonly OUTPUT="${1:-/tmp/rival-csv}"

# Каталог сборки библиотеки AWH с оптимизацией
readonly RELEASE="${2:-$ROOT/build-release}"

# Флаги сборки стендов
#
# Уровень оптимизации совпадает с уровнем сборки библиотеки в режиме Release:
# сравнивать реализации, собранные с разной оптимизацией, бессмысленно
readonly FLAGS="-O3 -DNDEBUG -std=c++17 -Wall -Wextra"

# Если исходные тексты сравниваемых реализаций не получены
for MODULE in csv-parser csv2 fast-cpp-csv-parser libcsv rapidcsv; do
	# Если очередной подмодуль отсутствует
	if [ ! -d "$VENDOR/$MODULE" ]; then
		# Выводим сообщение об отсутствии подмодуля
		echo "Submodule \"$MODULE\" is missing in \"$VENDOR\""
		exit 1
	fi
done

# Если библиотека AWH с оптимизацией ещё не собрана
if [ ! -f "$RELEASE/libawh.a" ]; then
	# Выводим сообщение о необходимости сборки библиотеки
	echo "Build AWH in Release mode first: cmake -B \"$RELEASE\" -DCMAKE_BUILD_TYPE=Release && make -C \"$RELEASE\" awh"
	exit 1
fi

# Выполняем создание каталога собранных стендов
mkdir -p "$OUTPUT"

# Выводим сообщение о сборке стенда библиотеки AWH
echo "Build \"awh\""

# Выполняем сборку стенда библиотеки AWH
c++ $FLAGS -Wno-reserved-user-defined-literal -I"$ROOT/include" -I"$STANDS" \
	"$STANDS/awh.cpp" "$RELEASE/libawh.a" -o "$OUTPUT/awh" || exit 1

# Выводим сообщение о сборке стенда реализации libcsv
echo "Build \"libcsv\""

# Выполняем сборку стенда реализации libcsv
#
# Исходный текст реализации собирается вместе со стендом: собранной библиотеки в
# подмодуле нет, а собирать её отдельно значило бы сравнивать разные уровни оптимизации
cc -O3 -DNDEBUG -Wall -c -I"$VENDOR/libcsv" -o "$OUTPUT/libcsv.o" "$VENDOR/libcsv/libcsv.c" || exit 1
c++ $FLAGS -I"$STANDS" -I"$VENDOR/libcsv" \
	"$STANDS/libcsv.cpp" "$OUTPUT/libcsv.o" -o "$OUTPUT/libcsv" || exit 1

# Выводим сообщение о сборке стенда реализации csv2
echo "Build \"csv2\""

# Выполняем сборку стенда реализации csv2
c++ $FLAGS -I"$STANDS" -I"$VENDOR/csv2/include" \
	"$STANDS/csv2.cpp" -o "$OUTPUT/csv2" || exit 1

# Выводим сообщение о сборке стенда реализации Fast C++ CSV Parser
echo "Build \"fast-cpp\""

# Выполняем сборку стенда реализации Fast C++ CSV Parser
c++ $FLAGS -I"$STANDS" -I"$VENDOR/fast-cpp-csv-parser" -pthread \
	"$STANDS/fast-cpp.cpp" -o "$OUTPUT/fast-cpp" || exit 1

# Выводим сообщение о сборке стенда реализации rapidcsv
echo "Build \"rapidcsv\""

# Выполняем сборку стенда реализации rapidcsv
c++ $FLAGS -I"$STANDS" -I"$VENDOR/rapidcsv/src" \
	"$STANDS/rapidcsv.cpp" -o "$OUTPUT/rapidcsv" || exit 1

# Выводим сообщение о сборке стенда реализации Vince's CSV Parser
echo "Build \"csv-parser\""

# Исходные тексты реализации Vince's CSV Parser
readonly PARSER_SOURCES="$VENDOR/csv-parser/include/internal"

# Выполняем сборку стенда реализации Vince's CSV Parser
c++ $FLAGS -I"$STANDS" -I"$VENDOR/csv-parser/include" \
	"$STANDS/csv-parser.cpp" \
	"$PARSER_SOURCES/col_names.cpp" "$PARSER_SOURCES/csv_format.cpp" \
	"$PARSER_SOURCES/csv_reader.cpp" "$PARSER_SOURCES/csv_reader_iterator.cpp" \
	"$PARSER_SOURCES/csv_row.cpp" "$PARSER_SOURCES/csv_utility.cpp" \
	"$PARSER_SOURCES"/parser/*.cpp \
	-pthread -o "$OUTPUT/csv-parser" || exit 1

# Выводим сообщение об успешной сборке стендов
echo ""
echo "Stands are placed in \"$OUTPUT\""
