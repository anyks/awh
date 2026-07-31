#!/usr/bin/env bash

##
# Сборка стендов сравнения контейнера HTTP-заголовков
#
# Соперники подобраны по задаче, а не по имени: Boost.Beast хранит поля с тем же
# набором обязанностей - порядок, кратность, поиск без учёта регистра, сборка
# сообщения, - а мультикарта стандартной библиотеки показывает, что даёт
# отдельный контейнер по сравнению с самой распространённой самоделкой.
#
# Boost необязателен: без него собирается один стенд мультикарты, и сверка
# помечает недостающую долю пропуском. Обрывать из-за отсутствующего системного
# пакета всю проверку значило бы лишить машину и тех сторожей, что собрались.
#

set -u

# Получаем корневую дирректорию репозитория
readonly ROOT=$(cd "$(dirname "$0")/../../.." && pwd)

# Каталог исходных текстов стендов
readonly STANDS="$ROOT/tools/benchmark/headers"

# Каталог размещения собранных стендов
readonly OUTPUT="${1:-/tmp/rival-headers}"

# Каталог сборки библиотеки с оптимизацией
readonly RELEASE="${2:-$ROOT/build-release}"

# Флаги сборки стендов
readonly FLAGS="-O3 -DNDEBUG -Wall -Wextra"

# Определяем операционную систему
OS=$(uname -a | awk '{print $1}')

# Определяем префикс системных пакетов
PREFIX="/usr/local"

# Если операционной системой является macOS
if [ "$OS" = "Darwin" ]; then
	# Устанавливаем префикс системных пакетов Homebrew
	PREFIX="/opt/homebrew"
fi

# Создаём каталог размещения собранных стендов
mkdir -p "$OUTPUT" || exit 1

##
# Стенд мультикарты стандартной библиотеки
#
# Собирается всегда: внешних зависимостей у него нет вовсе
#
echo "Build \"multimap\""
c++ -std=c++17 $FLAGS -o "$OUTPUT/multimap" "$STANDS/multimap.cpp" || exit 1

##
# Стенд реализации Boost.Beast
#
if [ -f "$PREFIX/include/boost/beast/http/fields.hpp" ]; then
	echo "Build \"beast\""
	c++ -std=c++17 $FLAGS -o "$OUTPUT/beast" "$STANDS/beast.cpp" \
		-I"$PREFIX/include" -L"$PREFIX/lib" || exit 1
else
	# Выводим сообщение об отсутствии системного пакета
	echo "Skip \"beast\": install boost first"
fi

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
		-isystem "$ROOT/include" \
		"$RELEASE/libawh.a" "$ROOT/third_party/lib/libdependence.a" \
		"$ROOT/third_party/lib/libtcmalloc_minimal.a" "$ROOT/third_party/lib/libcommon.a" \
		$(if [ "$OS" = "Darwin" ]; then echo "-framework Foundation"; fi) 2> /dev/null || exit 1
else
	# Выводим сообщение об отсутствии сборки библиотеки с оптимизацией
	echo "Skip \"awh\": build the library first with"
	echo "  cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release"
	echo "  cmake --build build-release --target awh -j 8"
fi

# Выводим сообщение о завершении сборки стендов
echo "Stands are placed in \"$OUTPUT\""
