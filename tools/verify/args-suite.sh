#!/bin/sh
#
# @file suite.sh
# @date 2026-09-08
#
# @license{LicenseRef-AWH-1.0}
#
# @author Yuriy Lobarev
#
# @brief Сборка набора проверок модуля разбора доводов запуска из исходных текстов,
#        без собранной библиотеки и без подмодулей
#
# @details Ворошитель модуля берёт лишь разборщик и описание («lexer», «schema»,
#          «common»), фасада же «Args::parse/config/env/dump» не трогает вовсе.
#          Стенд этот заведён ради фасада: набор проверок его покрывает, а полного
#          дерева с подмодулями на стендах нет и заводить его ради 44 проверок дорого
#
# @note Сторонние библиотеки берутся ЧУЖИМ деревом стенда через «THIRD_ROOT» и только
#       на чтение: своего «third_party» у стенда нет, а собирать его заново - часы
#
# @warning Стенд НЕ годен для macOS: «src/net/backend/bsd/eth.cpp» там пишется на
#          Objective-C++ и одним вызовом собирателя не берётся. На рабочей машине
#          набор берётся обычной сборкой CMake
#
# @warning Переписывать этот файл, пока прежний его прогон ещё идёт, НЕЛЬЗЯ: «sh»
#          читает сценарий по мере исполнения, и подмена под работающим процессом
#          даёт МНИМУЮ синтаксическую ошибку в середине. Проверено 08.09.2026 на
#          Solaris - «sh -n» того же файла проходил чисто
#
# @copyright Copyright © 2026
#
# Вызов:
#   tools/verify/args-suite.sh [корень дерева] [каталог сборки]
#
# Переменные окружения:
#   CXX         — собиратель, по умолчанию «c++»
#   FLAGS       — добавочные ключи сборки
#   THIRD_ROOT  — корень дерева со сторонними библиотеками, по умолчанию корень свой
#
# Сборка набора проверок args из исходных текстов, без собранной библиотеки
set -e
ROOT="${1:-.}"
OUT="${2:-/tmp/awh-args}"
CXX="${CXX:-c++}"
mkdir -p "$OUT"
cd "$ROOT"
# @warning Стенд НЕ годен для macOS: «src/net/backend/bsd/eth.cpp» там пишется на
#          Objective-C++ и одним вызовом собирателя не берётся - сборка валится внутри
#          заголовков Foundation. На рабочей машине набор берётся обычной сборкой CMake,
#          стенд же заведён ради систем, где полного дерева с подмодулями нет
SYS=$(uname -s)
[ "$SYS" = Darwin ] && { echo "стенд не для macOS: набор берётся сборкою CMake" >&2; exit 5; }
case "$SYS" in
	Darwin|FreeBSD|OpenBSD|NetBSD|DragonFly) PLATFORM=bsd ;;
	SunOS) PLATFORM=sun ;;
	MINGW*|MSYS*|CYGWIN*) PLATFORM=win ;;
	*) PLATFORM=gnu ;;
esac
FRAMEWORK="src/sys/log.cpp src/sys/chrono.cpp src/sys/fmk.cpp src/net/nwt.cpp
	src/encoding/unicode/normalize.cpp src/encoding/unicode/table.cpp
	src/encoding/unicode/unicode.cpp src/encoding/unicode/utf8.cpp
	src/encoding/charset/charset.cpp src/encoding/charset/table.cpp
	src/alloc/alloc.cpp src/alloc/cache.cpp src/alloc/central.cpp
	src/alloc/classes.cpp src/alloc/guard.cpp src/alloc/huge.cpp
	src/alloc/link.cpp src/alloc/pages.cpp src/alloc/profile.cpp
	src/alloc/source.cpp src/alloc/spin.cpp src/alloc/trace.cpp
	src/alloc/capture/elf.cpp src/alloc/capture/mach.cpp src/alloc/capture/pe.cpp
	src/num/lexical/table.cpp"
[ "$SYS" = OpenBSD ] && FRAMEWORK="$(echo "$FRAMEWORK" | sed 's|src/alloc/alloc\.cpp||') src/alloc/capture/obsd.cpp"
# «src/codec/replace.cpp» из перечня СНЯТ: подмена целевого файла перенесена в
# «sys/fs» ходом replaceAddress, а модуль удалён после перехода всех кодеков. Перечень
# частей у стенда ведётся вручную, и удалённый файл остался бы в нём молча - стенд
# отказал бы на сборке, а причина выглядела бы поломкой кода
CODEC="src/codec/numeric.cpp src/codec/bridge.cpp"
for D in abc json yaml xml toml ini; do CODEC="$CODEC $(echo src/codec/$D/*.cpp)"; done
# Часть шифрования и сжатия: её требует контейнер ABC, на который опирается мост
EXTRA="src/compressor/block.cpp src/compressor/stream.cpp src/compressor/types.cpp
	src/cryptography/crypto.cpp src/cryptography/hash.cpp src/cryptography/vault.cpp
	src/num/bignum.cpp"
# Корень сторонних библиотек: на стенде берётся от соседнего дерева, своего у стенда нет
TP="${THIRD_ROOT:-.}"
THIRD="-I$TP/third_party/include"
for D in lz4 bz2 zstd lzma zlib brotli snappy lizard density; do
	[ -d "$TP/third_party/include/$D" ] && THIRD="$THIRD -I$TP/third_party/include/$D"
done
DEPEND="$TP/third_party/lib/libdependence.a"
[ -f "$DEPEND" ] || DEPEND="$TP/third_party/lib/libdependence.lib"
[ -f "$DEPEND" ] || { echo "libdependence не найдена: $TP/third_party/lib" >&2; exit 4; }
ARGS="src/args/args.cpp src/args/lexer.cpp src/args/schema.cpp src/args/common.cpp"
NET="src/sys/fs.cpp src/sys/os.cpp"
# Проверки моста берутся сюда же: тела кодеков и самого моста стенд собирает и без
# них, а стенда своего у моста нет вовсе - без этой строки правки его выверялись бы
# на одной лишь рабочей машине, тогда как обе находки стендов были ей невидимы
TESTS="tests/main.cpp tests/args/args.cpp tests/args/lexer.cpp tests/args/schema.cpp tests/codec/bridge.cpp"
# Розыск gtest: путь его от системы к системе разный, а по умолчанию виден не везде
GT=""
# У MSYS2 оснастка своя у каждого окружения: под ARM64 набор берётся «/clangarm64»,
# и без него стенд отвечал «gtest не найден» на машине, где gtest стоит. Замерено
# 14.09.2026 на стенде Windows ARM64
for D in /opt/homebrew /usr/local /usr /usr/pkg /opt/local /opt/csw /mingw64 /clangarm64 /ucrt64 /mingw32; do
	if [ -f "$D/include/gtest/gtest.h" ]; then GT="-I$D/include -L$D/lib"; break; fi
done
[ -n "$GT" ] || { echo "gtest не найден: набор собрать нечем" >&2; exit 3; }
LIBS="-lgmock -lgmock_main -lgtest -lpthread"
case "$SYS" in SunOS) LIBS="$LIBS -lsocket -lnsl" ;; MINGW*|MSYS*|CYGWIN*) LIBS="-lgmock -lgmock_main -lgtest -lws2_32 -liphlpapi -lbcrypt -lole32 -luuid -lshlwapi -lshell32 -ladvapi32" ;; esac
# Потоки у MS Windows: libdependence с BoringSSL внутри собран бывает НА PTHREAD, и
# тогда связывание требует «-lpthread» - без неё стенд отвечает десятком неведомых
# имён вида «pthread_rwlock_rdlock». Отбираем её пробою, а не по имени системы: у
# сборок, того не требующих, лишняя библиотека безвредна. Замерено 14.09.2026 на
# стенде Windows ARM64 (окружение CLANGARM64)
case "$SYS" in
	MINGW*|MSYS*|CYGWIN*)
		for D in /clangarm64/lib /mingw64/lib /ucrt64/lib /mingw32/lib /usr/lib; do
			if [ -f "$D/libpthread.a" ] || [ -f "$D/libwinpthread.a" ]; then LIBS="$LIBS -lpthread"; break; fi
		done
	;;
esac
# Шифрование: у одних систем оно берётся из libdependence, у других требует
# библиотек системы; отбираем их пробою, а не по имени системы
SSL=""
for D in /usr/local/lib /usr/lib /usr/lib64 /opt/csw/lib; do
	if [ -f "$D/libcrypto.so" ] || [ -f "$D/libcrypto.a" ]; then SSL="-L$D -lssl -lcrypto"; break; fi
done
# Наречие языка берётся ТО ЖЕ, что объявлено проектом: CMakeLists.txt держит
# CMAKE_CXX_STANDARD 17 при CXX_STANDARD_REQUIRED ON, и все девять стендов кодеков
# строят по c++17.
#
# ПРЕЖДЕ здесь стоял отбор пробою по цепочке «c++2b → c++23 → c++20 → gnu++17», и это
# была ошибка устройства, а не мелочь: цепочка искала наречие НОВЕЕ объявленного, и
# стенд выходил зелёным там, где сборка проекта красна. Ровно так и вышло с
# ныне удалённом «src/codec/replace.cpp», где unique_ptr стоял без «#include <memory>»: при c++2b
# заголовок приходит косвенно и стенд зелен, при c++17 сборка отказывает. Стенд обязан
# мерить ТО, что собирает проект, иначе он мерит нечто иное и молчит о настоящем.
#
# Отступление вниз оставлено на случай собирателя, c++17 не знающего вовсе: там стенд
# лучше провести старым наречием, чем не провести вовсе, - но отступление это ГРОМКОЕ.
STD=""
for N in c++17 gnu++17 c++14; do
	if echo 'int main(){return 0;}' | $CXX -x c++ -std=$N -fsyntax-only - 2>/dev/null; then STD="-std=$N"; break; fi
done
[ -n "$STD" ] || { echo "собиратель не знает ни одного годного наречия" >&2; exit 6; }
case "$STD" in
	-std=c++17|-std=gnu++17) ;;
	*) echo "ВНИМАНИЕ: собиратель не знает c++17, стенд идёт наречием $STD - итог с проектом НЕ сличается" >&2 ;;
esac
echo "Собираем набор проверок args: $CXX ($SYS/$PLATFORM, $STD)"
$CXX $GT $STD -O1 -g -Iinclude -Itests $FLAGS $THIRD $FRAMEWORK $CODEC $ARGS $EXTRA $NET $TESTS $DEPEND $SSL $LIBS -lz -o "$OUT/args-tests"
echo "Набор собран: $OUT/args-tests"
