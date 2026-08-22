#!/usr/bin/env bash

# Получаем корневую дирректорию репозитория (каталог скрипта на три уровня ниже)
readonly ROOT=$(cd "$(dirname "$0")/../../.." && pwd)

# Каталог стендов сравнения
readonly STANDS="$ROOT/tools/benchmark/hash"

# Каталог исходных текстов библиотеки CityHash
readonly VENDOR="$ROOT/submodules/cityhash/src"

# Каталог собранных стендов
readonly OUTPUT="${1:-/tmp/rival-hash}"

# Каталог сборки библиотеки AWH с оптимизацией
readonly RELEASE="${2:-$ROOT/build-release}"

# Флаги сборки стендов
#
# Уровень оптимизации совпадает с уровнем сборки библиотеки в режиме Release:
# сравнивать реализации, собранные с разной оптимизацией, бессмысленно
readonly FLAGS="-O3 -DNDEBUG -Wall -Wextra"

# Получаем версию OS
OS=$(uname -a | awk '{print $1}')

# Если исходные тексты библиотеки CityHash не получены
if [ ! -f "$VENDOR/city.cc" ]; then
	# Выводим сообщение о необходимости получения исходных текстов
	echo "CityHash sources are missing, run:"
	echo "  git submodule update --init submodules/cityhash"
	exit 1
fi

# Выполняем создание каталога собранных стендов
mkdir -p "$OUTPUT" || exit 1

##
# Стенд хэширования библиотеки CityHash
#
# Исходные тексты подмодуля не изменяются: параметры сборки, которые обычно
# порождает сценарий configure, подставляются каталогом обвязки стенда
#
echo "Build \"cityhash\""
c++ -std=c++17 $FLAGS -c -o "$OUTPUT/city.o" "$VENDOR/city.cc" \
	-I"$STANDS/cityhash" -I"$VENDOR" || exit 1
c++ -std=c++17 $FLAGS -o "$OUTPUT/cityhash" "$STANDS/cityhash.cpp" "$OUTPUT/city.o" \
	-I"$VENDOR" || exit 1

##
# Стенд хэширования библиотеки AWH
#
# Собирается по той же обвязке замера, что и стенд CityHash: разница в обвязке
# иначе оказалась бы неотделима от разницы в самих реализациях
#
if [ -f "$RELEASE/libawh.a" ]; then
	echo "Build \"awh\""
	c++ -std=gnu++17 $FLAGS -pthread -DAWH_STATICLIB \
		-o "$OUTPUT/awh" "$STANDS/awh.cpp" \
		-I"$ROOT/third_party/include/pcre2" -I"$ROOT/third_party/include" -isystem "$ROOT/include" \
		"$RELEASE/libawh.a" "$ROOT/third_party/lib/libdependence.a" \
		$(if [ "$OS" = "Darwin" ]; then echo "-framework Foundation"; fi) 2> /dev/null || exit 1
else
	# Выводим сообщение об отсутствии сборки библиотеки с оптимизацией
	echo "Skip \"awh\": build the library first with"
	echo "  cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release"
	echo "  cmake --build build-release --target awh -j 8"
fi

# Выводим сообщение о завершении сборки стендов
echo "Stands are placed in \"$OUTPUT\""
