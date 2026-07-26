#!/usr/bin/env bash

# Получаем корневую дирректорию репозитория (каталог скрипта на три уровня ниже)
readonly ROOT=$(cd "$(dirname "$0")/../../.." && pwd)

# Каталог стендов сравнения
readonly STANDS="$ROOT/tools/benchmark/http1"

# Каталог исходных текстов сравниваемых реализаций
readonly VENDOR="$ROOT/third_party/rival/http1"

# Каталог собранных стендов
readonly OUTPUT="${1:-/tmp/rival-http1}"

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

# Если исходные тексты сравниваемых реализаций ещё не получены
if [ ! -f "$VENDOR/picohttpparser.c" ]; then
	# Выводим сообщение о необходимости получения исходных текстов
	echo "Run \"$STANDS/fetch.sh\" first"
	exit 1
fi

# Выполняем создание каталога собранных стендов
mkdir -p "$OUTPUT" || exit 1

##
# Стенд парсера llhttp проекта Node.js
#
# Библиотека берётся системной: пакетная сборка оптимизирована, а сравнивать
# следует с тем, что получает пользователь
#
echo "Build \"llhttp\""
c++ -std=c++17 $FLAGS -o "$OUTPUT/llhttp" "$STANDS/llhttp.cpp" \
	-I"$PREFIX/opt/llhttp/include" -L"$PREFIX/opt/llhttp/lib" -lllhttp || exit 1

##
# Стенд парсера http-parser проекта Node.js
#
# Исходные тексты собираются компилятором языка C: сборка их компилятором C++
# меняет результат, поэтому единица трансляции выносится отдельно
#
echo "Build \"http-parser\""
cc -std=c11 $FLAGS -c -o "$OUTPUT/http_parser.o" "$VENDOR/http_parser.c" -I"$VENDOR" || exit 1
c++ -std=c++17 $FLAGS -o "$OUTPUT/http-parser" "$STANDS/http-parser.cpp" "$OUTPUT/http_parser.o" -I"$VENDOR" || exit 1

##
# Стенд парсера picohttpparser проекта h2o
#
echo "Build \"picohttpparser\""
cc -std=c11 $FLAGS -c -o "$OUTPUT/picohttpparser.o" "$VENDOR/picohttpparser.c" -I"$VENDOR" || exit 1
c++ -std=c++17 $FLAGS -o "$OUTPUT/picohttpparser" "$STANDS/picohttpparser.cpp" "$OUTPUT/picohttpparser.o" -I"$VENDOR" || exit 1

##
# Стенд парсера сервера nginx
#
# Исходные тексты парсера не изменяются: окружение сервера подменяется
# обвязкой стенда, подставляемой каталогом заголовочных файлов
#
echo "Build \"nginx\""
cc -std=c11 $FLAGS -c -o "$OUTPUT/ngx_http_parse.o" "$VENDOR/ngx_http_parse.c" -I"$STANDS/nginx" || exit 1
c++ -std=c++17 $FLAGS -o "$OUTPUT/nginx" "$STANDS/nginx.cpp" "$OUTPUT/ngx_http_parse.o" -I"$STANDS/nginx" || exit 1

##
# Стенд парсера Boost.Beast
#
# Библиотека заголовочная, поэтому достаточно каталога заголовочных файлов
#
echo "Build \"beast\""
c++ -std=c++17 $FLAGS -o "$OUTPUT/beast" "$STANDS/beast.cpp" -I"$PREFIX/include" || exit 1

##
# Стенд парсера библиотеки AWH
#
# Собирается по той же обвязке замера, что и остальные стенды: разница в
# обвязке иначе оказалась бы неотделима от разницы в самих парсерах
#
if [ -f "$RELEASE/libawh.a" ]; then
	echo "Build \"awh\""
	c++ -std=gnu++17 $FLAGS -pthread -DAWH_STATICLIB -D__AWH_USE_TCMALLOC__ \
		-o "$OUTPUT/awh" "$STANDS/awh.cpp" \
		-I"$ROOT/contrib/include" -I"$ROOT/third_party/include/tcmalloc" \
		-I"$ROOT/third_party/include/pcre2" -I"$ROOT/third_party/include" -isystem "$ROOT/include" \
		"$RELEASE/libawh.a" "$ROOT/third_party/lib/libdependence.a" \
		"$ROOT/third_party/lib/libtcmalloc_minimal.a" "$ROOT/third_party/lib/libcommon.a" \
		$(if [ "$OS" = "Darwin" ]; then echo "-framework Foundation"; fi) 2> /dev/null || exit 1
else
	# Выводим сообщение об отсутствии сборки библиотеки с оптимизацией
	echo "Skip \"awh\": build the library first with"
	echo "  cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_BUILD_BENCHMARKS=YES"
	echo "  cmake --build build-release --target awh_BENCHMARK_proto -j 8"
fi

# Выводим сообщение о завершении сборки стендов
echo "Stands are placed in \"$OUTPUT\""
