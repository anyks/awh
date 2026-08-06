#!/usr/bin/env bash

# Получаем корневую дирректорию репозитория (каталог скрипта на три уровня ниже)
readonly ROOT=$(cd "$(dirname "$0")/../../.." && pwd)

# Каталог стендов сверки
readonly STANDS="$ROOT/tools/verify/regex"

# Каталог исходных текстов эталонной реализации PCRE2
readonly VENDOR="$ROOT/submodules/pcre2"

# Каталог собранных стендов
readonly OUTPUT="${1:-/tmp/verify-regex}"

# Каталог собранной эталонной реализации
readonly ORACLE="${2:-$OUTPUT/pcre2}"

# Каталог тестового окружения GoogleTest
readonly GTEST="${GTEST_ROOT:-/opt/homebrew}"

# Флаги сборки стендов
#
# Уровень оптимизации совпадает с уровнем сборки библиотеки в режиме Release:
# сверять реализации, собранные с разной оптимизацией, бессмысленно
readonly FLAGS="-std=c++2a -O3 -DNDEBUG -Wall -Wextra"

# Набор стендов сверки, не требующих тестового окружения
readonly PLAIN="accepted corpus syntax automaton_verdict reverse backtrack long word classes properties grapheme grapheme_tables grapheme_rules caseless names byname"

# Набор стендов сверки, собираемых вместе с тестовым окружением
readonly SUITE="static matching automaton engine"

# Если исходные тексты эталонной реализации не получены
if [ ! -f "$VENDOR/CMakeLists.txt" ]; then
	# Выводим сообщение о необходимости получения исходных текстов
	echo "PCRE2 sources are missing, run:"
	echo "  git submodule update --init submodules/pcre2"
	exit 1
fi

# Выполняем создание каталога собранных стендов
mkdir -p "$OUTPUT" || exit 1

##
# Эталонная реализация PCRE2
#
# В поставку библиотеки эталон не входит и собирается местно, единственно ради
# сверки: набор синтаксиса и границы совпадения этого модуля отвечают эталону,
# и отвечать ему они обязаны и впредь
##

# Если эталонная реализация не собрана
if [ ! -f "$ORACLE/lib/libpcre2-8.a" ]; then
	# Выводим сообщение о сборке эталонной реализации
	echo "Building PCRE2 oracle..."
	# Выполняем настройку сборки эталонной реализации
	cmake -S "$VENDOR" -B "$OUTPUT/pcre2-build" \
		-DCMAKE_BUILD_TYPE=Release \
		-DPCRE2_BUILD_PCRE2_8=ON \
		-DPCRE2_SUPPORT_UNICODE=ON \
		-DPCRE2_BUILD_TESTS=OFF \
		-DPCRE2_BUILD_PCRE2GREP=OFF \
		-DBUILD_SHARED_LIBS=OFF \
		-DCMAKE_INSTALL_PREFIX="$ORACLE" > /dev/null || exit 1
	# Выполняем сборку эталонной реализации
	cmake --build "$OUTPUT/pcre2-build" -j 8 > /dev/null || exit 1
	# Выполняем установку эталонной реализации
	cmake --install "$OUTPUT/pcre2-build" > /dev/null || exit 1
fi

##
# Стенды сверки модуля регулярных выражений
#
# Стенды собираются из исходных текстов модуля напрямую, минуя библиотеку:
# сверка обязана проверять текущее состояние исходных текстов, а не состояние
# библиотеки, собранной когда-то ранее
##

# Выполняем перебор стендов сверки, не требующих тестового окружения
for STAND in $PLAIN; do
	# Выводим сообщение о сборке стенда сверки
	echo "Building stand: $STAND"
	# Выполняем сборку стенда сверки
	g++ $FLAGS \
		-I"$ROOT/include" -I"$ROOT/tests" -I"$ORACLE/include" \
		"$STANDS/$STAND.cpp" "$ROOT"/src/regex/*.cpp "$ROOT"/src/encoding/unicode/*.cpp \
		-o "$OUTPUT/$STAND" \
		-L"$ORACLE/lib" -lpcre2-8 || exit 1
done

# Выполняем перебор стендов сверки, собираемых вместе с тестовым окружением
for STAND in $SUITE; do
	# Выводим сообщение о сборке стенда сверки
	echo "Building stand: $STAND"
	# Выполняем сборку стенда сверки
	g++ $FLAGS \
		-I"$ROOT/include" -I"$ROOT/tests" -I"$ORACLE/include" -I"$GTEST/include" \
		"$STANDS/$STAND.cpp" "$ROOT/tests/main.cpp" "$ROOT"/src/regex/*.cpp "$ROOT"/src/encoding/unicode/*.cpp \
		-o "$OUTPUT/$STAND" \
		-L"$ORACLE/lib" -L"$GTEST/lib" -lpcre2-8 -lgtest -lgmock || exit 1
done

# Выводим сообщение о завершении сборки стендов сверки
echo ""
echo "Stands are built in $OUTPUT"
echo "Run them all:"
echo "  for s in $PLAIN $SUITE; do echo \"== \$s\"; \"$OUTPUT/\$s\" || echo FAILED; done"
