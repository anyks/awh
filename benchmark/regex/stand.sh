#!/bin/sh
#
# @file stand.sh
# @date 2026-08-23
#
# @license{LicenseRef-AWH-1.0}
#
# @author Yuriy Lobarev
#
# @brief Отдельный стенд замеров регулярных выражений — сличение нашего модуля с эталоном PCRE2
#
# @details Эталонная реализация PCRE2 зависимостью библиотеки НЕ является: модуль
#          awh::regex её и заменил. Оттого штатная сборка о ней не знает вовсе, а
#          сличение с нею живёт здесь: стенд собирает PCRE2 из подмодуля сам и
#          связывает с нею ту же самую программу замеров, что собирается штатно
#
# @note Библиотека целиком здесь не собирается: замерить требуется один модуль, а
#       полная сборка на стендах занимает десятки минут и тянет за собою третью сторону.
#       Достаточно исходников самого модуля и разбора Юникода, на который он опирается
#
# @warning Замеры собираются только с оптимизацией: отладочная сборка замедляет всё
#          равномерно и изображает точечную регрессию там, где её нет
#
# @warning Снимать показатели надо на СВОБОДНОЙ машине: под сборкой отказывают сценарии
#          по скорости
#
# @copyright Copyright © 2026
#
# Вызов:
#   benchmark/regex/stand.sh [корень дерева]
#
# Переменные окружения:
#   CXX   — собиратель, по умолчанию «c++»
#   FLAGS — добавочные ключи сборки
#   OUT   — каталог стенда; сборка PCRE2 в нём переиспользуется между прогонами
#
ROOT=${1:-$(cd "$(dirname "$0")/../.." && pwd)}
CXX=${CXX:-c++}
command -v "$CXX" >/dev/null 2>&1 || CXX=g++
OUT=${OUT:-${TMPDIR:-/tmp}/awh-regex-bench}
mkdir -p "$OUT"
echo "система: $(uname -s) $(uname -r) $(uname -m)"
echo "собиратель: $CXX"
echo "дерево: $ROOT"
echo "стенд: $OUT"
PCRE="$ROOT/submodules/pcre2"
if [ ! -f "$PCRE/CMakeLists.txt" ]; then
	echo "ОТКАЗ: подмодуль PCRE2 не развёрнут; выполните: git submodule update --init submodules/pcre2"
	exit 1
fi
##
# Сборка эталона переиспользуется между прогонами
#
# Она занимает около минуты, а меняться ей нечего: подмодуль закреплён. Пересобрать
# её насильно можно, удалив каталог стенда целиком
##
if [ ! -f "$OUT/pcre2/libpcre2-8.a" ]; then
	echo "--- сборка эталона PCRE2 (единожды на стенд)"
	mkdir -p "$OUT/pcre2"
	##
	# Порождение машинного кода у эталона включается НАРОЧНО
	#
	# Наш модуль сличается с эталоном на равных: у обоих оба способа исполнения.
	# Эталон без кода мерил бы разбор программы против нашего машинного кода, и
	# сличение обратилось бы в похвальбу
	##
	( cd "$OUT/pcre2" && cmake "$PCRE" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
	   -DPCRE2_BUILD_PCRE2_8=ON -DPCRE2_SUPPORT_JIT=ON \
	   -DPCRE2_BUILD_TESTS=OFF -DPCRE2_BUILD_PCRE2GREP=OFF > cmake.log 2>&1 \
	  && make -j4 > make.log 2>&1 ) || { echo "ОТКАЗ СБОРКИ эталона, смотрите $OUT/pcre2/make.log"; exit 1; }
fi
##
# Заголовок эталона порождается сборкой, а не лежит в исходниках
#
# В подмодуле его нет вовсе - там pcre2.h.in, - и путь включения ведёт в каталог сборки
##
HEADERS=$(dirname "$(find "$OUT/pcre2" -name pcre2.h | head -1)")
##
# Зависимости системные, журналом требуемые
#
# Сжатие журнальных файлов ведётся zlib, а у macOS объект фреймворка обращается
# к основанию системы средствами Objective-C
##
LIBS="-lz"
case "$(uname -s)" in
	Darwin) LIBS="$LIBS -framework Foundation" ;;
	*) LIBS="$LIBS -lpthread" ;;
esac
echo "--- сборка замеров со сличением"
##
# Состав исходников берётся МАСКОЙ, а не перечнем поимённо
#
# Перечень отстаёт от дерева молча: приписанный к разбору Юникода файл таблиц в
# него не попал, и стенд отказал связыванием. Маска же берёт каталог целиком, а
# подкаталоги (`src/regex/grok`) в неё не входят - и не нужны здесь
##
##
# Свой набор исходников захвата выделений на систему
#
# Приём захвата у систем разный: подмена именами у ELF, зона у macOS, переписывание
# входа у MS Windows. Лишний файл захвата не соберётся - у него свои заголовки системы
##
case "$(uname -s)" in
	Darwin) CAPTURE="$ROOT/src/alloc/capture/mach.cpp" ;;
	MINGW*|MSYS*|CYGWIN*) CAPTURE="$ROOT/src/alloc/capture/pe.cpp" ;;
	*) CAPTURE="$ROOT/src/alloc/capture/elf.cpp" ;;
esac
##
# Опора модуля на журнал событий тянет за собою основание фреймворка
#
# Модуль сообщает журналом отказы, потребителю иначе невидимые: размещение
# исполняемой памяти, запрещённое ядром укреплённым, и отказ сборки выражения.
# Журнал же опирается на объект фреймворка, а тот - на время, систему, разбор
# адресов и распределитель памяти. Перечень этот и есть цена сообщения об отказах;
# библиотеки целиком он всё равно не составляет, и сборка идёт секундами
##
SUPPORT="$ROOT/src/sys/log.cpp $ROOT/src/sys/fmk.cpp $ROOT/src/sys/chrono.cpp \
 $ROOT/src/sys/os.cpp $ROOT/src/net/nwt.cpp $ROOT/src/num/lexical/table.cpp \
 $ROOT/src/encoding/charset/*.cpp $ROOT/src/alloc/*.cpp $CAPTURE"
SOURCES="$ROOT/benchmark/main.cpp $ROOT/benchmark/regex/matching/matching.cpp \
 $ROOT/src/regex/*.cpp $ROOT/src/encoding/unicode/*.cpp $SUPPORT"
$CXX -std=c++17 -O2 -Wno-c++11-narrowing $FLAGS -DAWH_BENCHMARK_PCRE2 \
 -I "$ROOT/include" -I "$ROOT/tools/benchmark/syscount" -I "$HEADERS" \
 -o "$OUT/bench-regex" $SOURCES "$OUT/pcre2/libpcre2-8.a" $LIBS > "$OUT/build.log" 2>&1
##
# Вывод собирателя печатается ИЗ ФАЙЛА, а не через `head` из канала
#
# Канал, закрытый на сороковой строке, шлёт собирателю обрыв, и тот гибнет на полпути:
# стенд отчитывается ложным отказом сборки
##
head -40 "$OUT/build.log"
if [ ! -x "$OUT/bench-regex" ] && [ ! -x "$OUT/bench-regex.exe" ]; then
	echo "ОТКАЗ СБОРКИ, смотрите $OUT/build.log"
	exit 1
fi
echo
"$OUT/bench-regex" || exit 1
exit 0
