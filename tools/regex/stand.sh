#!/bin/sh
# Раскладка модуля регулярных выражений на стенд, сборка и прогон переносимой проверки.
#
# Стенду нужен лишь компилятор C++: посторонних библиотек переносимая проверка
# не требует, поэтому годится и машина, где ничего не установлено.
#
# Использование: tools/regex/stand.sh <пользователь@узел> [порт]

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
PORT="${2:-22}"

if [ -z "$TARGET" ]; then
	echo "Использование: $0 <пользователь@узел> [порт]" >&2
	exit 2
fi

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUNDLE=$(mktemp -t awh-regex-stand)".tgz"

# Состав набора ведётся общим для всех стендов файлом
. "$(dirname "$0")/sources.sh"

# Порождаем запись хранилища собранных выражений на рабочей машине, дабы стенд
# проверил её восстановлением у себя. Наборы программы пишутся образом памяти,
# поэтому запись зависит от устройства машины: стенду надлежит либо восстановить
# её в точности, либо отвергнуть опознанием устройства, но не прочесть неверно.
RECORD=""
HOST_CXX="${CXX:-c++}"
if command -v "$HOST_CXX" >/dev/null 2>&1 ; then
	HOST_DIR=$(mktemp -d -t awh-regex-host)
	# Состав исходных текстов разбирается поиском, а не перечислением: перечень
	# каталогов ведётся общим файлом, а состав каталога - самим деревом
	HOST_SOURCES=$(cd "$ROOT" && find $STAND_SOURCES -maxdepth 1 -name '*.cpp')
	# shellcheck disable=SC2086
	if (cd "$ROOT" && "$HOST_CXX" -std=c++17 -O2 -Iinclude -Itools/regex \
		tools/regex/conformance.cpp $HOST_SOURCES \
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

# Перечень собираемых исходных текстов кладётся в набор отдельным файлом
#
# Разбирать состав поиском на самом стенде нельзя: дерево src/regex несёт
# подкаталог grok, модулю выражений подчинённый, однако переносимой проверке
# не нужный и связанный с прочими частями библиотеки. Перечень потому выводится
# здесь по составу каталогов, а стенд берёт его готовым
(cd "$ROOT" && find $STAND_SOURCES -maxdepth 1 -name '*.cpp') > "$ROOT/tools/regex/sources.list"

echo "Собираем набор исходных текстов модуля"
if [ -n "$RECORD" ] ; then
	cp "$RECORD" "$ROOT/tools/regex/record.bin"
	# shellcheck disable=SC2086
	tar --format ustar -czf "$BUNDLE" -C "$ROOT" \
		$STAND_HEADERS $STAND_SOURCES $STAND_TOOLS \
		tools/regex/sources.list tools/regex/record.bin
	rm -f "$ROOT/tools/regex/record.bin"
else
	# shellcheck disable=SC2086
	tar --format ustar -czf "$BUNDLE" -C "$ROOT" $STAND_HEADERS $STAND_SOURCES $STAND_TOOLS \
		tools/regex/sources.list
fi
rm -f "$ROOT/tools/regex/sources.list"

echo "Раскладываем набор на стенд $TARGET"
# Передача ведётся через ввод ssh, а не средством scp: подсистема sftp на части
# стендов отключена, и scp обрывает связь, тогда как поток ввода доступен всегда.
#
# Домашний каталог стенда бывает недоступен для записи, поэтому набор
# раскладывается в каталог временных файлов, доступный на всякой машине
ssh -p "$PORT" "$TARGET" '
	for DIR in /usr/bin /bin ; do
		[ -d "$DIR" ] && PATH="$DIR:$PATH"
	done
	export PATH
	cat > /tmp/awh-regex-stand.tgz
' < "$BUNDLE"
rm -f "$BUNDLE"

echo "Собираем и прогоняем проверку на стенде"
ssh -p "$PORT" "$TARGET" '
	set -e
	# Оболочка MSYS2 на стендах Windows заводит путь без каталогов набора POSIX
	# и без каталога связки вооружения, отчего ни tar, ни компилятор в нём
	# не находятся. Каталоги эти приписываются к пути при наличии их; системам
	# семейства UNIX приписка безвредна - каталогов таких у них нет
	for DIR in /clangarm64/bin /mingw64/bin /usr/bin ; do
		[ -d "$DIR" ] && PATH="$DIR:$PATH"
	done
	export PATH
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
			tools/regex/conformance.cpp $(cat tools/regex/sources.list) -o conformance 2>compile.log ; then
			echo "Собрано в режиме -std=$STD"
			break
		fi
	done
	if [ ! -x conformance ] ; then
		echo "Сборка не выполнена:" >&2
		tail -30 compile.log >&2
		exit 4
	fi
	if [ -f tools/regex/record.bin ] ; then
		./conformance --read=tools/regex/record.bin
	else
		./conformance
	fi
'
