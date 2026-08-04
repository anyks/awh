#!/bin/sh
# Раскладка модуля регулярных выражений на стенд Windows, сборка и прогон
# переносимой проверки.
#
# Стенд отделён от tools/regex/stand.sh потому, что оболочка на стороне
# Windows набором POSIX не располагает вовсе: ни ls, ни cp, ни mkdir, ни
# gzip там нет, - есть лишь встроенные средства bash, cmd и tar из состава
# системы. Раскладка потому ведётся передачей архива через ввод tar, а
# сборка - отдельным пакетным файлом tools/regex/stand.bat.
#
# Использование: tools/regex/stand-windows.sh <пользователь@узел> [связка] [порт]
#
# Связка вооружения: x64 (сборка своя на AMD64), x64_arm64 (сборка встречная
# на ARM64 с машины AMD64 либо с машины ARM64 под подражанием), arm64.

set -e

TARGET="$1"
TOOLSET="${2:-x64}"
PORT="${3:-22}"

if [ -z "$TARGET" ]; then
	echo "Использование: $0 <пользователь@узел> [связка] [порт]" >&2
	exit 2
fi

ROOT=$(cd "$(dirname "$0")/../.." && pwd)

# Каталог раскладки на стенде
#
# Каталог временных файлов Windows зависит от пользователя и в оболочке
# доступен не всегда, поэтому набор раскладывается в каталог домашний
REMOTE="awh-regex-stand"

echo "Раскладываем набор исходных текстов на стенд $TARGET"

# Передача ведётся через ввод tar, а не средством scp: подсистема sftp на
# стенде отключена, и scp обрывает связь
tar czf - -C "$ROOT" \
	include/regex include/unicode include/sys/ascii.hpp include/sys/global.hpp \
	src/regex src/unicode tools/regex/conformance.cpp tools/regex/conformance.hpp \
	tools/regex/stand.bat \
	| ssh -p "$PORT" "$TARGET" "
		cmd.exe //c \"rmdir /s /q %USERPROFILE%\\\\$REMOTE 2>nul & mkdir %USERPROFILE%\\\\$REMOTE\" > /dev/null 2>&1
		cd ~/$REMOTE && tar xzf -
	"

echo "Собираем и прогоняем проверку на стенде"
ssh -p "$PORT" "$TARGET" "cmd.exe //c \"%USERPROFILE%\\\\$REMOTE\\\\tools\\\\regex\\\\stand.bat $TOOLSET\""
