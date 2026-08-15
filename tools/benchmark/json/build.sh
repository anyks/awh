#!/usr/bin/env bash

# Получаем корневую дирректорию репозитория (каталог скрипта на три уровня ниже)
readonly ROOT=$(cd "$(dirname "$0")/../../.." && pwd)

# Каталог стендов сравнения
readonly STANDS="$ROOT/tools/benchmark/json"

# Каталог исходных текстов сравниваемых реализаций
readonly VENDOR="$ROOT/submodules"

# Каталог собранных стендов
readonly OUTPUT="${1:-/tmp/rival-json}"

# Каталог сборки библиотеки AWH с оптимизацией
readonly RELEASE="${2:-$ROOT/build-release}"

# Флаги сборки стендов
#
# Уровень оптимизации совпадает с уровнем сборки библиотеки в режиме Release:
# сравнивать реализации, собранные с разной оптимизацией, бессмысленно
readonly FLAGS="-O3 -DNDEBUG -std=c++17 -Wall -Wextra"

# Если исходные тексты сравниваемых реализаций не получены
for MODULE in yyjson simdjson nlohmann rapidjson; do
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

# Выводим сообщение о сборке стенда реализации yyjson
echo "Build \"yyjson\""

# Выполняем сборку стенда реализации yyjson
#
# Исходный текст реализации собирается вместе со стендом: собранной библиотеки в
# подмодуле нет, а собирать её отдельно значило бы сравнивать разные уровни оптимизации
cc -O3 -DNDEBUG -Wall -c -I"$VENDOR/yyjson/src" -o "$OUTPUT/yyjson.o" "$VENDOR/yyjson/src/yyjson.c" || exit 1
c++ $FLAGS -I"$STANDS" -I"$VENDOR/yyjson/src" \
	"$STANDS/yyjson.cpp" "$OUTPUT/yyjson.o" -o "$OUTPUT/yyjson" || exit 1

# Выводим сообщение о сборке стенда реализации simdjson
echo "Build \"simdjson\""

# Выполняем сборку стенда реализации simdjson
#
# Собирается однофайловая сборка реализации: она и предлагается её создателями
# для встраивания, и собирается быстрее полного дерева исходных текстов
c++ $FLAGS -I"$STANDS" -I"$VENDOR/simdjson/singleheader" \
	"$STANDS/simdjson.cpp" "$VENDOR/simdjson/singleheader/simdjson.cpp" -o "$OUTPUT/simdjson" || exit 1

# Выводим сообщение о сборке стенда реализации nlohmann/json
echo "Build \"nlohmann\""

# Выполняем сборку стенда реализации nlohmann/json
c++ $FLAGS -I"$STANDS" -I"$VENDOR/nlohmann/include" \
	"$STANDS/nlohmann.cpp" -o "$OUTPUT/nlohmann" || exit 1

# Выводим сообщение о сборке стенда реализации RapidJSON
echo "Build \"rapidjson\""

# Выполняем сборку стенда реализации RapidJSON
c++ $FLAGS -I"$STANDS" -I"$VENDOR/rapidjson/include" \
	"$STANDS/rapidjson.cpp" -o "$OUTPUT/rapidjson" || exit 1

# Каталог установленной библиотеки Boost
#
# Реализация Boost.JSON берётся из установленной библиотеки, а не из подмодуля:
# подмодуль Boost сложен из подмодулей поменьше, и один лишь Boost.JSON из него
# не собирается - за ним тянется два десятка иных частей библиотеки
readonly BOOST="${BOOST_ROOT:-/opt/homebrew}"

# Если установленная библиотека Boost.JSON найдена
if [ -f "$BOOST/include/boost/json.hpp" ]; then
	# Выводим сообщение о сборке стенда реализации Boost.JSON
	echo "Build \"boost\""

	# Выполняем сборку стенда реализации Boost.JSON
	c++ $FLAGS -I"$STANDS" -I"$BOOST/include" -L"$BOOST/lib" \
		"$STANDS/boost.cpp" -lboost_json -lboost_container -o "$OUTPUT/boost" || exit 1
# Если установленная библиотека Boost.JSON не найдена
else
	# Выводим сообщение о пропуске стенда реализации Boost.JSON
	echo "Skip \"boost\": Boost.JSON is not installed in \"$BOOST\""
fi

# Выводим сообщение об успешной сборке стендов
echo ""
echo "Stands are placed in \"$OUTPUT\""
