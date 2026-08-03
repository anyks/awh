#!/usr/bin/env bash

# Получаем корневую дирректорию репозитория (каталог скрипта на три уровня ниже)
readonly ROOT=$(cd "$(dirname "$0")/../../.." && pwd)

# Каталог стендов сравнения
readonly STANDS="$ROOT/tools/benchmark/http3"

# Каталог исходных текстов сравниваемых реализаций
readonly VENDOR="$ROOT/third_party/rival/http3"

# Каталог собранных стендов
readonly OUTPUT="${1:-/tmp/rival-http3}"

# Каталог сборки библиотеки AWH с оптимизацией
readonly RELEASE="${2:-$ROOT/build-release}"

# Флаги сборки стендов
#
# Уровень оптимизации совпадает с уровнем сборки библиотеки в режиме Release:
# сравнивать реализации, собранные с разной оптимизацией, бессмысленно
readonly FLAGS="-O3 -DNDEBUG -Wall -Wextra"

# Получаем версию OS
OS=$(uname -a | awk '{print $1}')

# Каталог сборки эталонной реализации из подмодуля
readonly NGHTTP3_BUILD="$ROOT/build-nghttp3"

##
# Эталонная реализация берётся из подмодуля, а не системной
#
# Системная версия у каждой ОС своя, а сравнение обязано вестись с одним и тем же
# кодом везде: иначе разница в показателях между машинами окажется неотделима
# от разницы в версиях эталона
#
if [ ! -f "$NGHTTP3_BUILD/lib/libnghttp3.a" ] && [ -f "$ROOT/submodules/nghttp3/CMakeLists.txt" ]; then
	# Выводим сообщение о сборке эталонной реализации
	echo "Build \"nghttp3\" from submodule"
	# У nghttp3 есть собственный вложенный подмодуль lib/sfparse
	git -C "$ROOT" submodule update --init --recursive submodules/nghttp3 > /dev/null 2>&1
	# Выполняем конфигурацию сборки только библиотеки
	cmake -S "$ROOT/submodules/nghttp3" -B "$NGHTTP3_BUILD" -DCMAKE_BUILD_TYPE=Release \
		-DENABLE_LIB_ONLY=ON -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=OFF \
		-DBUILD_TESTING=OFF > /dev/null 2>&1 || exit 1
	# Выполняем сборку библиотеки
	cmake --build "$NGHTTP3_BUILD" -j 8 > /dev/null 2>&1 || exit 1
fi

# Если эталонная реализация не найдена
if [ ! -f "$NGHTTP3_BUILD/lib/libnghttp3.a" ]; then
	# Выводим сообщение о необходимости получения эталонной реализации
	echo "Fetch the submodule first: git submodule update --init --recursive submodules/nghttp3"
	exit 1
fi

# Заголовочные файлы лежат в двух местах: исходные тексты и сгенерированный version.h
readonly NGHTTP3_INCLUDE="-I$ROOT/submodules/nghttp3/lib/includes -I$NGHTTP3_BUILD/lib/includes"

# Библиотека эталонной реализации
readonly NGHTTP3_LIBRARY="$NGHTTP3_BUILD/lib/libnghttp3.a"

# Выполняем создание каталога собранных стендов
mkdir -p "$OUTPUT" || exit 1

##
# Стенд эталонной реализации nghttp3
#
# Библиотека берётся из подмодуля и собирается тем же компилятором с той же
# оптимизацией, что и остальные стенды
#
echo "Build \"nghttp3\""
c++ -std=c++17 $FLAGS -pthread -o "$OUTPUT/nghttp3" "$STANDS/nghttp3.cpp" \
	$NGHTTP3_INCLUDE "$NGHTTP3_LIBRARY" || exit 1

##
# Стенд кодека полей ls-qpack
#
# Исходные тексты собираются компилятором языка C: сборка их компилятором C++
# меняет результат, поэтому единицы трансляции выносятся отдельно
#
if [ -f "$VENDOR/lsqpack.c" ]; then
	echo "Build \"lsqpack\""
	cc -std=gnu99 $FLAGS -w -c -o "$OUTPUT/lsqpack.o" "$VENDOR/lsqpack.c" \
		-I"$VENDOR" -I"$VENDOR/deps/xxhash" || exit 1
	cc -std=gnu99 $FLAGS -w -c -o "$OUTPUT/xxhash.o" "$VENDOR/deps/xxhash/xxhash.c" \
		-I"$VENDOR/deps/xxhash" || exit 1
	c++ -std=c++17 $FLAGS -pthread -o "$OUTPUT/lsqpack" "$STANDS/lsqpack.cpp" \
		"$OUTPUT/lsqpack.o" "$OUTPUT/xxhash.o" -I"$VENDOR" $NGHTTP3_INCLUDE "$NGHTTP3_LIBRARY" || exit 1
else
	# Выводим сообщение о необходимости получения исходных текстов
	echo "Skip \"lsqpack\": run \"$STANDS/fetch.sh\" first"
fi

##
# Стенд реализации библиотеки AWH
#
# Собирается по той же обвязке замера, что и остальные стенды: разница в
# обвязке иначе оказалась бы неотделима от разницы в самих реализациях
#
if [ -f "$RELEASE/libawh.a" ]; then
	echo "Build \"awh\""
	c++ -std=gnu++17 $FLAGS -pthread -DAWH_STATICLIB \
		-Wno-reserved-user-defined-literal -o "$OUTPUT/awh" "$STANDS/awh.cpp" \
		-I"$ROOT/contrib/include" -I"$ROOT/third_party/include/pcre2" \
		-I"$ROOT/third_party/include" -isystem "$ROOT/include" $NGHTTP3_INCLUDE \
		"$RELEASE/libawh.a" "$ROOT/third_party/lib/libdependence.a" \
		"$ROOT/third_party/lib/libcommon.a" "$NGHTTP3_LIBRARY" -lz \
		$(if [ "$OS" = "Darwin" ]; then echo "-framework Foundation -framework CoreFoundation -framework Security"; fi) 2> /dev/null || exit 1
else
	# Выводим сообщение об отсутствии сборки библиотеки с оптимизацией
	echo "Skip \"awh\": build the library first with"
	echo "  cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release"
	echo "  cmake --build build-release --target awh -j 8"
fi

# Выводим сообщение о завершении сборки стендов
echo "Stands are placed in \"$OUTPUT\""
