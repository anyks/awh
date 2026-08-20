#!/bin/sh
# Раскладка стенда замеров модуля регулярных выражений на стенд и прогон его.
#
# Стенд заведён ради калибровки порогов набора замеров: правило велит брать
# порог по самому медленному стенду, а не по рабочей машине, - стенды идут
# медленнее неравномерно, и единая доля от рабочей машины одним сценариям
# даёт запас излишний, а другим не даёт никакого.
#
# Собирается стенд из исходных текстов напрямую, минуя библиотеку: сборка
# библиотеки целиком требует посторонних зависимостей, тогда как замеру
# сопоставления нужен один лишь модуль выражений.
#
# Эталон PCRE2 сюда не кладётся: он собирается стендом сравнения
# tools/benchmark/regex/build.sh на рабочей машине, а калибровке порогов
# не нужен вовсе - пороги ставятся по нашему показателю.
#
# Использование: tools/benchmark/regex/stand.sh <пользователь@узел> [порт]

set -e

# Служебные метки файловой системы macOS в передаваемые наборы не кладутся
COPYFILE_DISABLE=1
export COPYFILE_DISABLE

TARGET="$1"
PORT="${2:-22}"

if [ -z "$TARGET" ]; then
	echo "Использование: $0 <пользователь@узел> [порт]" >&2
	exit 2
fi

ROOT=$(cd "$(dirname "$0")/../../.." && pwd)

# Состав набора ведётся общим для всех стендов файлом
. "$ROOT/tools/regex/sources.sh"

# Состав стенда замеров
STAND_BENCH="tools/benchmark/regex/awh.cpp tools/benchmark/regex/driver.hpp tools/benchmark/regex/scenarios.hpp tools/benchmark/common.hpp"

BUNDLE=$(mktemp -t awh-regex-bench)

# Перечень собираемых исходных текстов кладётся в набор отдельным файлом:
# разбирать состав поиском на самом стенде нельзя, дерево src/regex несёт
# подкаталог grok, замеру не нужный и связанный с прочими частями библиотеки
(cd "$ROOT" && find $STAND_SOURCES -maxdepth 1 -name '*.cpp') > "$ROOT/tools/regex/sources.list"

echo "Собираем набор исходных текстов модуля"
# shellcheck disable=SC2086
tar --format ustar -czf "$BUNDLE" -C "$ROOT" \
	$STAND_HEADERS $STAND_SOURCES $STAND_BENCH tools/regex/sources.list

rm -f "$ROOT/tools/regex/sources.list"

echo "Раскладываем набор на стенд $TARGET"
# Передача ведётся через ввод ssh, а не средством scp: подсистема sftp на части
# стендов отключена, и scp обрывает связь, тогда как поток ввода доступен всегда
ssh -p "$PORT" "$TARGET" '
	for DIR in /usr/bin /bin ; do
		[ -d "$DIR" ] && PATH="$DIR:$PATH"
	done
	export PATH
	cat > /tmp/awh-regex-bench.tgz
' < "$BUNDLE"
rm -f "$BUNDLE"

echo "Собираем и прогоняем замер на стенде"
ssh -p "$PORT" "$TARGET" '
	set -e
	for DIR in /usr/bin /mingw64/bin /clangarm64/bin ; do
		[ -d "$DIR" ] && PATH="$DIR:$PATH"
	done
	export PATH
	rm -rf /tmp/awh-regex-bench
	mkdir -p /tmp/awh-regex-bench
	# Распаковка выполняется отдельным вызовом распаковщика: набор команд tar
	# систем семейства Solaris сжатия не разбирает вовсе
	gzip -dc /tmp/awh-regex-bench.tgz | (cd /tmp/awh-regex-bench && tar xf -)
	cd /tmp/awh-regex-bench
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
	# Уровень оптимизации совпадает с уровнем сборки библиотеки в режиме
	# выпуска: замер, снятый со сборки отладочной, занижает показатели
	# равномерно и порогом служить не может
	for STD in c++2b c++20 c++17 ; do
		if "$CXX" -std=$STD -O3 -DNDEBUG -Iinclude -Itools/benchmark/regex \
			tools/benchmark/regex/awh.cpp $(cat tools/regex/sources.list) -o bench 2>compile.log ; then
			echo "Собрано в режиме -std=$STD"
			break
		fi
	done
	if [ ! -x bench ] ; then
		echo "Сборка не выполнена:" >&2
		tail -30 compile.log >&2
		exit 4
	fi
	./bench
'
