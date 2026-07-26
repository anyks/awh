#!/usr/bin/env bash

# Получаем корневую дирректорию репозитория (каталог скрипта на три уровня ниже)
readonly ROOT=$(cd "$(dirname "$0")/../../.." && pwd)

# Каталог стендов сравнения
readonly STANDS="$ROOT/tools/benchmark/http2"

# Каталог исходных текстов сравниваемых реализаций
readonly VENDOR="$ROOT/third_party/rival/http2"

# Каталог собранных стендов
readonly OUTPUT="${1:-/tmp/rival-http2}"

# Каталог сборки библиотеки AWH с оптимизацией
readonly RELEASE="${2:-$ROOT/build-release}"

# Флаги сборки стендов
#
# Уровень оптимизации совпадает с уровнем сборки библиотеки в режиме Release:
# сравнивать реализации, собранные с разной оптимизацией, бессмысленно
readonly FLAGS="-O3 -DNDEBUG -Wall -Wextra"

# Получаем версию OS
OS=$(uname -a | awk '{print $1}')

# Каталог заголовочных файлов системных библиотек
PREFIX="/usr/local"

# Если операционной системой является macOS
if [ "$OS" = "Darwin" ]; then
	# Устанавливаем каталог заголовочных файлов пакетного менеджера Homebrew
	PREFIX="/opt/homebrew"
fi

# Каталог сборки эталонной реализации из подмодуля
readonly NGHTTP2_BUILD="$ROOT/build-nghttp2"

##
# Эталонная реализация берётся из подмодуля, а не системной
#
# Системная версия у каждой ОС своя, а сравнение обязано вестись с одним и тем же
# кодом везде: иначе разница в показателях между машинами окажется неотделима
# от разницы в версиях эталона
#
if [ ! -f "$NGHTTP2_BUILD/lib/libnghttp2.a" ] && [ -f "$ROOT/submodules/nghttp2/CMakeLists.txt" ]; then
	# Выводим сообщение о сборке эталонной реализации
	echo "Build \"nghttp2\" from submodule"
	# Выполняем конфигурацию сборки только библиотеки
	cmake -S "$ROOT/submodules/nghttp2" -B "$NGHTTP2_BUILD" -DCMAKE_BUILD_TYPE=Release \
		-DENABLE_LIB_ONLY=ON -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=OFF \
		-DBUILD_TESTING=OFF > /dev/null 2>&1 || exit 1
	# Выполняем сборку библиотеки
	cmake --build "$NGHTTP2_BUILD" -j 8 > /dev/null 2>&1 || exit 1
fi

# Если эталонная реализация собрана из подмодуля
if [ -f "$NGHTTP2_BUILD/lib/libnghttp2.a" ]; then
	# Заголовочные файлы лежат в двух местах: исходные тексты и сгенерированный nghttp2ver.h
	NGHTTP2_INCLUDE="-I$ROOT/submodules/nghttp2/lib/includes -I$NGHTTP2_BUILD/lib/includes"
	# Устанавливаем библиотеку эталонной реализации
	NGHTTP2_LIBRARY="$NGHTTP2_BUILD/lib/libnghttp2.a"
# Если подмодуль недоступен - откатываемся на системную реализацию
elif [ -f "$PREFIX/opt/libnghttp2/lib/libnghttp2.a" ]; then
	NGHTTP2_INCLUDE="-I$PREFIX/opt/libnghttp2/include"
	NGHTTP2_LIBRARY="$PREFIX/opt/libnghttp2/lib/libnghttp2.a"
else
	NGHTTP2_INCLUDE="-I$PREFIX/include"
	NGHTTP2_LIBRARY="$PREFIX/lib/libnghttp2.a"
fi

# Если эталонная реализация не найдена
if [ ! -f "$NGHTTP2_LIBRARY" ]; then
	# Выводим сообщение о необходимости получения эталонной реализации
	echo "Fetch the submodule first: git submodule update --init submodules/nghttp2"
	exit 1
fi

# Если исходные тексты сравниваемых реализаций ещё не получены
if [ ! -f "$VENDOR/lshpack.c" ]; then
	# Выводим сообщение о необходимости получения исходных текстов
	echo "Run \"$STANDS/fetch.sh\" first"
	exit 1
fi

# Выполняем создание каталога собранных стендов
mkdir -p "$OUTPUT" || exit 1

##
# Стенд эталонной реализации nghttp2
#
# Библиотека берётся из подмодуля и собирается тем же компилятором с той же
# оптимизацией, что и остальные стенды
#
echo "Build \"nghttp2\""
c++ -std=c++17 $FLAGS -o "$OUTPUT/nghttp2" "$STANDS/nghttp2.cpp" \
	$NGHTTP2_INCLUDE "$NGHTTP2_LIBRARY" || exit 1

##
# Стенд кодека заголовков ls-hpack
#
# Исходные тексты собираются компилятором языка C: сборка их компилятором C++
# меняет результат, поэтому единицы трансляции выносятся отдельно.
#
# Вызовы аллокатора подставляются определениями препроцессора: реализация
# собственного интерфейса аллокатора не предоставляет, а без учёта выделений
# памяти половина сравнения теряется. Исходные тексты при этом не изменяются.
#
# Вывод декодера переводится в режим без форматирования HTTP/1.x: по умолчанию
# он дописывает к каждому заголовку разделитель и перевод строки, а остальные
# сравниваемые реализации такой работы не выполняют
#
echo "Build \"lshpack\""
cc -std=c11 $FLAGS -DLSHPACK_DEC_HTTP1X_OUTPUT=0 '-DXXH_HEADER_NAME="xxhash.h"' \
	-Dmalloc=rivalMalloc -Dfree=rivalFree -Dcalloc=rivalCalloc -Drealloc=rivalRealloc \
	-c -o "$OUTPUT/lshpack.o" "$VENDOR/lshpack.c" -I"$VENDOR" -I"$VENDOR/deps/xxhash" || exit 1
cc -std=c11 $FLAGS -c -o "$OUTPUT/xxhash.o" "$VENDOR/deps/xxhash/xxhash.c" -I"$VENDOR/deps/xxhash" || exit 1
c++ -std=c++17 $FLAGS -DLSHPACK_DEC_HTTP1X_OUTPUT=0 -o "$OUTPUT/lshpack" "$STANDS/lshpack.cpp" \
	"$OUTPUT/lshpack.o" "$OUTPUT/xxhash.o" -I"$VENDOR" $NGHTTP2_INCLUDE "$NGHTTP2_LIBRARY" || exit 1

##
# Стенд реализации библиотеки AWH
#
# Собирается по той же обвязке замера, что и остальные стенды: разница в
# обвязке иначе оказалась бы неотделима от разницы в самих реализациях
#
if [ -f "$RELEASE/libawh.a" ]; then
	echo "Build \"awh\""
	c++ -std=gnu++17 $FLAGS -pthread -DAWH_STATICLIB -D__AWH_USE_TCMALLOC__ \
		-o "$OUTPUT/awh" "$STANDS/awh.cpp" \
		-I"$ROOT/contrib/include" -I"$ROOT/third_party/include/tcmalloc" \
		-I"$ROOT/third_party/include/pcre2" -I"$ROOT/third_party/include" \
		-isystem "$ROOT/include" $NGHTTP2_INCLUDE \
		"$RELEASE/libawh.a" "$ROOT/third_party/lib/libdependence.a" \
		"$ROOT/third_party/lib/libtcmalloc_minimal.a" "$ROOT/third_party/lib/libcommon.a" \
		"$NGHTTP2_LIBRARY" \
		$(if [ "$OS" = "Darwin" ]; then echo "-framework Foundation"; fi) 2> /dev/null || exit 1
else
	# Выводим сообщение об отсутствии сборки библиотеки с оптимизацией
	echo "Skip \"awh\": build the library first with"
	echo "  cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_BUILD_BENCHMARKS=YES"
	echo "  cmake --build build-release --target awh_BENCHMARK_proto -j 8"
fi

# Выводим сообщение о завершении сборки стендов
echo "Stands are placed in \"$OUTPUT\""
