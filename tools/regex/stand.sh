#!/bin/sh
# Раскладка модуля регулярных выражений на стенд, сборка и прогон переносимой проверки.
#
# Стенду нужен лишь компилятор C++: посторонних библиотек переносимая проверка
# не требует, поэтому годится и машина, где ничего не установлено.
#
# Использование: tools/regex/stand.sh <пользователь@узел> [порт]

set -e

TARGET="$1"
PORT="${2:-22}"

if [ -z "$TARGET" ]; then
	echo "Использование: $0 <пользователь@узел> [порт]" >&2
	exit 2
fi

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUNDLE=$(mktemp -t awh-regex-stand)".tgz"

echo "Собираем набор исходных текстов модуля"
tar czf "$BUNDLE" -C "$ROOT" \
	include/regex include/unicode include/sys/ascii.hpp include/sys/global.hpp \
	src/regex src/unicode tools/regex/conformance.cpp tools/regex/conformance.hpp

echo "Раскладываем набор на стенд $TARGET"
# Домашний каталог стенда бывает недоступен для записи, поэтому набор
# раскладывается в каталог временных файлов, доступный на всякой машине
scp -q -P "$PORT" "$BUNDLE" "$TARGET:/tmp/awh-regex-stand.tgz"
rm -f "$BUNDLE"

echo "Собираем и прогоняем проверку на стенде"
ssh -p "$PORT" "$TARGET" '
	set -e
	rm -rf /tmp/awh-regex-stand
	mkdir -p /tmp/awh-regex-stand
	# Распаковка выполняется отдельным вызовом распаковщика: набор команд tar
	# систем семейства Solaris сжатия не разбирает вовсе
	gzip -dc /tmp/awh-regex-stand.tgz | (cd /tmp/awh-regex-stand && tar xf -)
	cd /tmp/awh-regex-stand
	uname -srm
	for CXX in "$CXX" c++ g++ clang++ ; do
		[ -n "$CXX" ] || continue
		command -v "$CXX" >/dev/null 2>&1 || continue
		break
	done
	if ! command -v "$CXX" >/dev/null 2>&1 ; then
		echo "Компилятор C++ на стенде не обнаружен" >&2
		exit 3
	fi
	"$CXX" --version | head -1
	for STD in c++2b c++20 c++17 ; do
		if "$CXX" -std=$STD -O2 -Iinclude -Itools/regex \
			tools/regex/conformance.cpp src/regex/*.cpp src/unicode/*.cpp -o conformance 2>compile.log ; then
			echo "Собрано в режиме -std=$STD"
			break
		fi
	done
	if [ ! -x conformance ] ; then
		echo "Сборка не выполнена:" >&2
		tail -30 compile.log >&2
		exit 4
	fi
	./conformance
'
