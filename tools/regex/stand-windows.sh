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

# Служебные метки файловой системы macOS в передаваемые наборы не кладутся:
# распаковщики прочих систем на них жалуются отказом «Extended header record
# length is out of range», и виден он лишь на стенде, а не на рабочей машине
COPYFILE_DISABLE=1
export COPYFILE_DISABLE

# Набор пишется видом ustar, а не видом pax
#
# Каталоги дерева несут метки файловой системы macOS - значки и ярлыки, -
# и вид pax выносит их в расширенные заголовки. Распаковщик OpenBSD такие
# заголовки разбирает неверно и валит распаковку отказом «Extended header
# record length is out of range», а на рабочей машине изъян не виден вовсе.
# Вид ustar расширенных заголовков не имеет; длины путей набора ему отвечают.

TARGET="$1"
TOOLSET="${2:-x64}"
PORT="${3:-22}"

if [ -z "$TARGET" ]; then
	echo "Использование: $0 <пользователь@узел> [связка] [порт]" >&2
	exit 2
fi

ROOT=$(cd "$(dirname "$0")/../.." && pwd)

# Состав набора ведётся общим для всех стендов файлом
. "$(dirname "$0")/sources.sh"

# Порождаем запись хранилища собранных выражений на рабочей машине, дабы стенд
# проверил её восстановлением у себя. Наборы программы пишутся образом памяти,
# поэтому запись зависит от устройства машины: стенду надлежит либо восстановить
# её в точности, либо отвергнуть опознанием устройства, но не прочесть неверно.
#
# Проверка эта ведётся и стендом tools/regex/stand.sh, однако именно на Windows
# она весома вдвойне: соглашение о вызове там своё, и запись, порождённая
# машиной с иным соглашением, различий в отображении регистров нести не должна
RECORD=""
HOST_CXX="${CXX:-c++}"
if command -v "$HOST_CXX" >/dev/null 2>&1 ; then
	HOST_DIR=$(mktemp -d -t awh-regex-host)
	HOST_SOURCES=$(cd "$ROOT" && stand_sources)
	# shellcheck disable=SC2086
	if (cd "$ROOT" && "$HOST_CXX" -std=c++17 -O2 -Wno-c++11-narrowing -Iinclude -Itools/regex \
		tools/regex/conformance.cpp $HOST_SOURCES -lz \
		-o "$HOST_DIR/conformance") 2>"$HOST_DIR/compile.log" ; then
		if "$HOST_DIR/conformance" "--write=$HOST_DIR/record.bin" >/dev/null 2>&1 ; then
			RECORD="$HOST_DIR/record.bin"
			echo "Порождена запись хранилища рабочей машины"
		fi
	fi
	if [ -z "$RECORD" ] ; then
		echo "Запись хранилища рабочей машины не порождена, проверка её будет пропущена"
	fi
else
	echo "Компилятор на рабочей машине не обнаружен, проверка записи будет пропущена"
fi

# Каталог раскладки на стенде
#
# Каталог временных файлов Windows зависит от пользователя и в оболочке
# доступен не всегда, поэтому набор раскладывается в каталог домашний
REMOTE="awh-regex-stand"

# Перечень собираемых исходных текстов кладётся в набор отдельным файлом
#
# Разбирать состав поиском на самом стенде нельзя: дерево src/regex несёт
# подкаталог grok, модулю выражений подчинённый, однако переносимой проверке
# не нужный и связанный с прочими частями библиотеки
(cd "$ROOT" && stand_sources) > "$ROOT/tools/regex/sources.list"

echo "Раскладываем набор исходных текстов на стенд $TARGET"

# Запись хранилища кладётся в набор исходных текстов, поскольку раскладка
# ведётся единственным вызовом tar, а не передачей файлов по одному
if [ -n "$RECORD" ] ; then
	cp "$RECORD" "$ROOT/tools/regex/record.bin"
	EXTRA="tools/regex/record.bin"
else
	EXTRA=""
fi

# Значки каталогов, средствами macOS заведённые, в набор не кладутся
#
# Имя такого файла - «Icon» с возвратом каретки на конце, и Windows файла
# с этим знаком в имени не создаёт вовсе: распаковка валится отказом
# «Invalid argument», а виден он лишь на стенде. Прочие системы знак этот
# в имени сносят, отчего изъян и не показывался нигде, кроме Windows.
ICON="Icon$(printf '\r')"

# Передача ведётся через ввод tar, а не средством scp: подсистема sftp на
# стенде отключена, и scp обрывает связь
# shellcheck disable=SC2086
tar --format ustar -czf - -C "$ROOT" --exclude "$ICON" \
	$STAND_HEADERS $STAND_SOURCES $STAND_TOOLS \
	tools/regex/sources.list tools/regex/stand.bat \
	tools/regex/preserving-x64.asm tools/regex/preserving-arm64.asm $EXTRA \
	| ssh -p "$PORT" "$TARGET" "
		cmd.exe //c \"rmdir /s /q %USERPROFILE%\\\\$REMOTE 2>nul & mkdir %USERPROFILE%\\\\$REMOTE\" > /dev/null 2>&1
		cd ~/$REMOTE && tar xzf -
	"

# Запись рабочей машины из набора исходных текстов убирается: место ей лишь
# в раскладке на стенд, а в дереве исходных текстов её быть не должно
rm -f "$ROOT/tools/regex/record.bin" "$ROOT/tools/regex/sources.list"

echo "Собираем и прогоняем проверку на стенде"
ssh -p "$PORT" "$TARGET" "cmd.exe //c \"%USERPROFILE%\\\\$REMOTE\\\\tools\\\\regex\\\\stand.bat $TOOLSET\""
