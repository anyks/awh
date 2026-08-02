#!/usr/bin/env bash

# Получаем корневую дирректорию репозитория (каталог скрипта на три уровня ниже)
readonly ROOT=$(cd "$(dirname "$0")/../../.." && pwd)

# Каталог стендов сравнения
readonly STANDS="$ROOT/tools/benchmark/xml"

# Каталог исходных текстов сравниваемых реализаций
readonly VENDOR="$ROOT/third_party/rival/xml"

# Каталог собранных стендов
readonly OUTPUT="${1:-/tmp/rival-xml}"

# Каталог сборки библиотеки AWH с оптимизацией
readonly RELEASE="${2:-$ROOT/build-release}"

# Флаги сборки стендов
#
# Уровень оптимизации совпадает с уровнем сборки библиотеки в режиме Release:
# сравнивать реализации, собранные с разной оптимизацией, бессмысленно
readonly FLAGS="-O3 -DNDEBUG -std=c++17 -Wall -Wextra"

# Если исходные тексты сравниваемых реализаций ещё не получены
if [ ! -f "$VENDOR/pugixml.cpp" ]; then
	# Выводим сообщение о необходимости получения исходных текстов
	echo "Run \"$STANDS/fetch.sh\" first"
	exit 1
fi

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
c++ $FLAGS -Wno-reserved-user-defined-literal -I"$ROOT/include" \
	"$STANDS/awh.cpp" "$RELEASE/libawh.a" -o "$OUTPUT/awh" || exit 1

# Выводим сообщение о сборке стенда реализации Expat
echo "Build \"expat\""

# Выполняем сборку стенда реализации Expat
c++ $FLAGS -I"$STANDS" "$STANDS/expat.cpp" -lexpat -o "$OUTPUT/expat" || exit 1

# Выводим сообщение о сборке стенда реализации libxml2
echo "Build \"libxml2\""

# Выполняем сборку стенда реализации libxml2
c++ $FLAGS -I"$STANDS" $(xml2-config --cflags) $(xml2-config --libs | sed 's/-I[^ ]*//g') \
	"$STANDS/libxml2.cpp" -o "$OUTPUT/libxml2" || exit 1

# Выводим сообщение о сборке стенда реализации pugixml
echo "Build \"pugixml\""

# Выполняем сборку стенда реализации pugixml
c++ $FLAGS -I"$STANDS" -I"$VENDOR" "$STANDS/pugixml.cpp" "$VENDOR/pugixml.cpp" -o "$OUTPUT/pugixml" || exit 1

# Выводим сообщение о сборке стенда реализации RapidXML
echo "Build \"rapidxml\""

# Выполняем сборку стенда реализации RapidXML
c++ $FLAGS -I"$STANDS" -I"$VENDOR" "$STANDS/rapidxml.cpp" -o "$OUTPUT/rapidxml" || exit 1

# Выводим сообщение об успешной сборке стендов
echo ""
echo "Stands are placed in \"$OUTPUT\""
