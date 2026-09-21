#!/bin/sh
# Сборка и запуск щупа разыскания модуля регулярных выражений.
#
# Щупы модуля - «passes», «longpass», «lazyhunt» - собираются отдельно от
# библиотеки: им нужен признак «AWH_REGEX_PROBING», какого штатная сборка
# не ставит вовсе. Признак включает учёт путей исполнения, и без него щуп
# «lazyhunt» выводит перечень путей пустым, отказа не давая.
#
# Сборка ведётся ЭТИМ стендом, а не строкой, в заголовке каждого щупа
# переписанной: состав, перечисленный в каждом щупе отдельно, устаревает
# молча, а перекладка дерева исходных текстов ломает его незаметно.
#
# Использование: tools/regex/probe.sh <имя щупа> [доводы щупа]
#
# Переменные окружения:
#   CXX   — собиратель, по умолчанию «c++»
#   FLAGS — добавочные ключи сборки
#   OUT   — каталог сборки, по умолчанию временный каталог системы

set -e

NAME="$1"
if [ -z "$NAME" ]; then
	echo "Использование: $0 <имя щупа> [доводы щупа]" >&2
	echo "Щупы: $(cd "$(dirname "$0")" && ls passes.cpp longpass.cpp lazyhunt.cpp 2>/dev/null | sed 's/\.cpp//' | tr '\n' ' ')" >&2
	exit 2
fi
shift

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${OUT:-${TMPDIR:-/tmp}/awh-regex-probe}
CXX=${CXX:-c++}
command -v "$CXX" >/dev/null 2>&1 || CXX=g++
SOURCE="$ROOT/tools/regex/$NAME.cpp"

if [ ! -f "$SOURCE" ]; then
	echo "щуп «$NAME» не найден: $SOURCE" >&2
	exit 2
fi
mkdir -p "$OUT"

# Состав исходных текстов ведётся общим для всех стендов файлом
. "$ROOT/tools/regex/sources.sh"

##
# Состав щупа ШИРЕ состава переносимой проверки
#
# Щуп собирает выражения открытым договором модуля, а тот тянет за собою запись
# хранилища: она выдаётся кодеком JSON, извлечение чисел которого живёт в
# «codec/numeric.cpp», а чтение с записью документа - в модуле файловой системы.
# Переносимой проверке они не нужны, оттого в «sources.sh» их и нет
##
EXTRA="$ROOT/src/codec/numeric.cpp $ROOT/src/sys/os.cpp $ROOT/src/sys/fs.cpp"

##
# У macOS исходники собираются как Objective-C++
#
# Модуль файловой системы берёт «Foundation» ради пути к домашнему каталогу и
# пишется там на Objective-C++. Признак «-x none» обязателен ПЕРЕД ключами
# связывания: ключ «-x» правит вид всех доводов последующих
##
LANGUAGE=""
RESTORE=""
LIBS="-lz"
case "$(uname -s)" in
	Darwin)
		LANGUAGE="-x objective-c++ -fobjc-arc"
		RESTORE="-x none"
		LIBS="$LIBS -framework Foundation -lpthread"
	;;
	MINGW*|MSYS*|CYGWIN*) LIBS="$LIBS -lws2_32 -lIphlpapi -lpsapi -ldbghelp -lcrypt32 -lbcrypt -lgdi32" ;;
	*) LIBS="$LIBS -lpthread" ;;
esac

##
# Часть «capture/obsd.cpp» берётся только под OpenBSD
#
# Внутренние имена распределителя libc берутся только там, а собранная
# безусловно, часть эта под MinGW валит связывание
##
NATIVE=""
[ "$(uname -s)" = "OpenBSD" ] && NATIVE="$ROOT/src/alloc/capture/obsd.cpp"

echo "система: $(uname -s) $(uname -r) $(uname -m)"
echo "собиратель: $CXX"
echo "щуп: $NAME"
echo "сборка: $OUT/$NAME"

cd "$ROOT"
##
# Сборка идёт с оптимизацией ПОЛНОЙ и без проверок времени исполнения
#
# Щуп меряет время, а сборка отладочная замедляет всё равномерно и изображает
# точечную просадку там, где её нет
##
# shellcheck disable=SC2046,SC2086
$CXX -std=c++17 -O3 -DNDEBUG -DAWH_REGEX_PROBING -Wno-c++11-narrowing $FLAGS \
	-I "$ROOT/include" -o "$OUT/$NAME" $LANGUAGE \
	"$SOURCE" $(stand_sources) $EXTRA $NATIVE $RESTORE $LIBS > "$OUT/build.log" 2>&1 || {
		echo "ОТКАЗ СБОРКИ, смотрите $OUT/build.log"
		head -40 "$OUT/build.log"
		exit 1
	}
echo

##
# Щуп запускается ИЗ КАТАЛОГА СБОРКИ
#
# Хранилище пишет записи выражений по путям относительным: запуск из иного
# каталога оставил бы их где придётся
##
cd "$OUT"
"$OUT/$NAME" "$@"
exit $?
