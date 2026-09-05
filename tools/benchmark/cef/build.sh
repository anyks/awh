#!/bin/sh
#
# @file build.sh
# @brief Сборка стенда сличения кодека CEF с прежним модулем ANYKS
#
# @details Стенд собирается прямо, без CMake: прежний модуль и кодек AWH берутся
#          готовыми библиотеками. Пути к прежнему модулю, PCRE2 и RapidJSON
#          задаются переменными окружения, ибо на всякой машине они свои
#
# @license{LicenseRef-AWH-1.0}
# @author Yuriy Lobarev
#

set -e

# Корень дерева AWH
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# Прежний модуль CEF: дерево с собранной libcef.a
LEGACY="${LEGACY:-$HOME/Work/GIT/anyks/cef}"
# Заголовки PCRE2, прежним модулем требуемые
PCRE="${PCRE:-/opt/homebrew}"
# Заголовки RapidJSON, прежним модулем требуемые
RAPIDJSON="${RAPIDJSON:-$HOME/Work/GIT/dev/rapidjson}"
# Собранная библиотека AWH
AWHLIB="${AWHLIB:-$ROOT/build/libawh.a}"
# Собиратель
COMPILER="${CXX:-c++}"

# Если прежний модуль не собран
if [ ! -f "$LEGACY/build/libcef.a" ]; then
	echo "Прежний модуль не собран: $LEGACY/build/libcef.a" >&2
	echo "Задай путь переменной LEGACY либо собери его сам" >&2
	exit 1
fi

# Если библиотека AWH не собрана
if [ ! -f "$AWHLIB" ]; then
	echo "Библиотека AWH не собрана: $AWHLIB" >&2
	echo "Задай путь переменной AWHLIB" >&2
	exit 1
fi

# Выполняем сборку стенда сличения
$COMPILER -std=c++17 -O3 -DNDEBUG \
	-I "$LEGACY/include" -I "$PCRE/include" -I "$RAPIDJSON/include" \
	-I "$ROOT/include" -I "$ROOT/submodules" \
	"$(dirname "$0")/comparison.cpp" "$LEGACY/build/libcef.a" "$AWHLIB" \
	-L "$PCRE/lib" -lpcre2-8 -lpcre2-posix \
	-o "$(dirname "$0")/comparison"

echo "Стенд собран: $(dirname "$0")/comparison"
echo "Запуск: $(dirname "$0")/comparison [число кругов]"
