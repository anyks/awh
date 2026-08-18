#!/usr/bin/env bash

# Получаем корневую дирректорию репозитория (каталог скрипта на три уровня ниже)
readonly ROOT=$(cd "$(dirname "$0")/../../.." && pwd)

# Каталог стендов сравнения
readonly STANDS="$ROOT/tools/benchmark/regex"

# Каталог исходных текстов эталонной реализации PCRE2
readonly VENDOR="$ROOT/submodules/pcre2"

# Каталог собранных стендов
readonly OUTPUT="${1:-/tmp/rival-regex}"

# Каталог собранной эталонной реализации
readonly ORACLE="${2:-$OUTPUT/vendor}"

# Флаги сборки стендов
#
# Уровень оптимизации совпадает с уровнем сборки библиотеки в режиме Release:
# сравнивать реализации, собранные с разной оптимизацией, бессмысленно
readonly FLAGS="-std=c++2a -O3 -DNDEBUG -Wall -Wextra"

# Если исходные тексты эталонной реализации не получены
if [ ! -f "$VENDOR/CMakeLists.txt" ]; then
	# Выводим сообщение о необходимости получения исходных текстов
	echo "PCRE2 sources are missing, run:"
	echo "  git submodule update --init submodules/pcre2"
	exit 1
fi

# Признак доступности компиляции выражения в машинный код
JIT="ON"

# Если исходные тексты компилятора в машинный код не получены
if [ ! -f "$VENDOR/deps/sljit/sljit_src/sljitLir.c" ]; then
	# Выводим сообщение о недоступности компиляции в машинный код
	echo "PCRE2 JIT sources are missing, the rival will run interpreted."
	echo "Get them for a full-strength comparison:"
	echo "  git -C submodules/pcre2 submodule update --init deps/sljit"
	echo ""
	# Выполняем сброс признака доступности компиляции в машинный код
	JIT="OFF"
fi

# Выполняем создание каталога собранных стендов
mkdir -p "$OUTPUT" || exit 1

##
# Эталонная реализация PCRE2
#
# В поставку библиотеки эталон не входит и собирается местно, единственно ради
# сравнения. Компиляция выражения в машинный код включается намеренно: сравнивать
# реализацию с эталоном, работающим не в полную силу, бессмысленно
##

# Если эталонная реализация не собрана
if [ ! -f "$ORACLE/lib/libpcre2-8.a" ]; then
	# Выводим сообщение о сборке эталонной реализации
	echo "Building PCRE2 rival..."
	# Выполняем настройку сборки эталонной реализации
	cmake -S "$VENDOR" -B "$OUTPUT/vendor-build" \
		-DCMAKE_BUILD_TYPE=Release \
		-DPCRE2_BUILD_PCRE2_8=ON \
		-DPCRE2_SUPPORT_UNICODE=ON \
		-DPCRE2_SUPPORT_JIT=$JIT \
		-DPCRE2_BUILD_TESTS=OFF \
		-DPCRE2_BUILD_PCRE2GREP=OFF \
		-DBUILD_SHARED_LIBS=OFF \
		-DCMAKE_INSTALL_PREFIX="$ORACLE" > /dev/null || exit 1
	# Выполняем сборку эталонной реализации
	cmake --build "$OUTPUT/vendor-build" -j 8 > /dev/null || exit 1
	# Выполняем установку эталонной реализации
	cmake --install "$OUTPUT/vendor-build" > /dev/null || exit 1
fi

##
# Стенд сравнения модуля регулярных выражений библиотеки AWH
#
# Стенд собирается из исходных текстов модуля напрямую, минуя библиотеку:
# сравнение обязано мерить текущее состояние исходных текстов, а не состояние
# библиотеки, собранной когда-то ранее
#
# Состав перечислен вместе с таблицами Юникода: модуль опирается на них
# приведением регистра, свойствами и разбором кластеров, и без них стенд
# не связывается вовсе
##

# Выводим сообщение о сборке стенда сравнения
echo "Building stand: awh"

# Выполняем сборку стенда сравнения
g++ $FLAGS \
	-I"$ROOT/include" \
	"$STANDS/awh.cpp" "$ROOT"/src/regex/*.cpp "$ROOT"/src/encoding/unicode/*.cpp \
	-o "$OUTPUT/awh" || exit 1

##
# Стенд сравнения эталонной реализации PCRE2
##

# Выводим сообщение о сборке стенда сравнения
echo "Building stand: pcre2"

# Выполняем сборку стенда сравнения
g++ $FLAGS \
	-I"$ORACLE/include" \
	"$STANDS/pcre2.cpp" \
	-o "$OUTPUT/pcre2" \
	-L"$ORACLE/lib" -lpcre2-8 || exit 1

# Выводим сообщение о завершении сборки стендов сравнения
echo ""
echo "Stands are built in $OUTPUT"
echo "Run them:"
echo "  $OUTPUT/awh"
echo "  $OUTPUT/pcre2"
