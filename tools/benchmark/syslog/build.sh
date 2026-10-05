#!/bin/sh
#
# @file build.sh
# @date 2026-10-05
#
# @license{LicenseRef-AWH-1.0}
#
# @author Yuriy Lobarev
#
# @brief Сборка стенда сличения кодека syslog с прежним модулем ANYKS
#
# @details Стенд собирается прямо, без CMake: прежний модуль и кодек AWH берутся
#          готовыми библиотеками. Пути к прежнему модулю, PCRE2 и RapidJSON
#          задаются переменными окружения, ибо на всякой машине они свои
#
# Вызов:
#   tools/benchmark/syslog/build.sh
#
# Переменные окружения:
#   LEGACY    — дерево прежнего модуля syslog с собранной библиотекой
#   PCRE      — каталог установки PCRE2
#   RAPIDJSON — каталог исходных текстов RapidJSON
#   AWHLIB    — собранная библиотека AWH
#   CXX       — собиратель
#
# @copyright Copyright © 2026
#

set -e

# Корень дерева AWH
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# Прежний модуль syslog: дерево с собранной libsyslog.a
LEGACY="${LEGACY:-$HOME/Work/GIT/anyks/syslog}"
# Заголовки PCRE2, прежним модулем требуемые
PCRE="${PCRE:-/opt/homebrew}"
# Заголовки RapidJSON, прежним модулем требуемые
RAPIDJSON="${RAPIDJSON:-$HOME/Work/GIT/dev/rapidjson}"
# Каталог собранных стендов (первый довод либо принятое место по умолчанию)
OUTPUT="${1:-/tmp/rival-syslog}"
# Собранная библиотека AWH
AWHLIB="${AWHLIB:-$ROOT/build/libawh.a}"
# Собиратель
COMPILER="${CXX:-c++}"

# Системные библиотеки, каких требует ядро библиотеки
#
# @details Разбор alias-файлов в «src/sys/fs.cpp» зовёт Foundation у macOS и `ws2_32`
#          у MS Windows: без них связывание стенда валится отказом на классах
#          Objective-C либо на `WSAGetLastError`
#
case "$(uname -s)" in
	MINGW*|MSYS*|CYGWIN*) SYSTEM_LIBS="-lws2_32" ;;
	Darwin) SYSTEM_LIBS="-framework Foundation" ;;
	*) SYSTEM_LIBS="" ;;
esac

# Если прежний модуль не собран
if [ ! -f "$LEGACY/build/libsyslog.a" ]; then
	echo "Прежний модуль не собран: $LEGACY/build/libsyslog.a" >&2
	echo "Задай путь переменной LEGACY либо собери его сам" >&2
	exit 1
fi

# Если библиотека AWH не собрана
if [ ! -f "$AWHLIB" ]; then
	echo "Библиотека AWH не собрана: $AWHLIB" >&2
	echo "Задай путь переменной AWHLIB" >&2
	exit 1
fi

# Выполняем создание каталога собранных стендов
mkdir -p "$OUTPUT"

# Выполняем сборку стенда сличения
$COMPILER -std=c++17 -O3 -DNDEBUG \
	-I "$LEGACY/include" -I "$PCRE/include" -I "$RAPIDJSON/include" \
	-I "$ROOT/include" -I "$ROOT/submodules" \
	"$(dirname "$0")/comparison.cpp" "$LEGACY/build/libsyslog.a" "$AWHLIB" \
	-L "$PCRE/lib" -lpcre2-8 -lpcre2-posix $SYSTEM_LIBS \
	-o "$OUTPUT/comparison"

echo "Стенд собран: $OUTPUT/comparison"
echo "Запуск: $OUTPUT/comparison [число кругов]"
