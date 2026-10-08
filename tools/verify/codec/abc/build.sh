#!/bin/sh
#
# @file build.sh
# @date 2026-09-01
#
# @license{LicenseRef-AWH-1.0}
#
# @author Yuriy Lobarev
#
# @brief Сборка стенда сличения снятия единицы записи ABC с судьёю по описанию
#
# @details Стенд опирается лишь на две части кодека - снятие единицы да общие виды, - и
#          собирается ТРЕМЯ вызовами собирателя. Ни журнала, ни выделения памяти, ни
#          кодировок он не тянет: работа `abc::take` их не зовёт вовсе
#
# @note Прочие стенды сличения забирают чужой корпус из сети. Этому забирать нечего:
#       основание его - описание формата, живущее прозой заголовков, и оно уже в дереве
#
# @copyright Copyright © 2026
#
# Вызов:
#   tools/verify/codec/abc/build.sh [каталог сборки]
#
# Переменные окружения:
#   CXX   — собиратель, по умолчанию «c++»
#   FLAGS — добавочные ключи сборки
#

# Прекращаем работу при первом же отказе
set -e

# Получаем корень дерева исходных текстов
ROOT=$(cd "$(dirname "$0")/../../../.." && pwd)

# Получаем каталог собранного стенда
OUTPUT="${1:-/tmp/verify-abc}"

# Получаем собиратель
COMPILER="${CXX:-c++}"

# Собираем ключи сборки стенда
OPTIONS="-O2 -std=c++17 -I$ROOT/include $FLAGS"

#
# Путь к библиотеке языка C++ того собирателя, каким собраны поверки
#
# @note Путь этот прописывается в двоичный файл: у DragonFly рядом стоят несколько
#       собирателей, и файл, собранный `g++14`, при запуске подхватывал `libstdc++`
#       от gcc11 и отваливался с «version GLIBCXX_3.4.32 not found». Сборка при этом
#       зелёная, а отказ приходит от прогона, и причину пойдут искать в кодеке
#
# @warning Путь берётся лишь тогда, когда собиратель отдаёт его полным: `clang` на
#          выдачу этого вопроса отвечает одним лишь именем файла, и `dirname` от него
#          дал бы текущий каталог
#
# @note У целей MS Windows выдача эта негодна: библиотека там идёт отдельным файлом
#       рядом с двоичным, и `rpath` у формата PE не работает вовсе
#
case "$(uname -s)" in
	MINGW*|MSYS*|CYGWIN*) ;;
	*)
		STDLIB="$($COMPILER -print-file-name=libstdc++.so 2>/dev/null)"
		case "$STDLIB" in
			/*) OPTIONS="$OPTIONS -Wl,-rpath,$(cd "$(dirname "$STDLIB")" && pwd)" ;;
		esac
	;;
esac

# Выполняем заведение каталога собранного стенда
mkdir -p "$OUTPUT"

# Выполняем снос прежде собранного стенда
#
# @note Снос обязателен: при отказе сборки прежний двоичный файл остаётся на месте
#       и прогон отчитывается успехом по коду, какого в нём уже нет
rm -f "$OUTPUT/verify" "$OUTPUT/verify.exe"

# Выводим сообщение о начале сборки стенда
echo "Собираем стенд сличения ABC: $COMPILER"

# Выполняем сборку общих видов кодека
$COMPILER $OPTIONS -c "$ROOT/src/codec/abc/common.cpp" -o "$OUTPUT/codec-common.o"

# Выполняем сборку снятия единицы проволочной записи
$COMPILER $OPTIONS -c "$ROOT/src/codec/abc/encoding.cpp" -o "$OUTPUT/codec-encoding.o"

#
# Сборка слоёв, на которые опирается кодек: общий разбор UTF-8, заводчик
# фреймворка, журнал, файловая система, сигналы, процессы, сеть, распределитель
# и кодировки
#
# @note Кодек с №69 снимает знак общим телом `awh::utf8::decode`, а стенд зовёт
#       `awh::fmk::initialize`. Перечень частей тут ведётся ВРУЧНУЮ и эти файлы
#       миновали: связывание отвечает телом за телом, начиная с двух перечисленных
#
# @warning Слой файловой системы у macOS написан на Objective-C++ и зовёт
#          `NSFileManager`: обычным C++ он не собирается, а связывается с основой
#          Foundation. Отбор этот повторяет CMakeLists.txt
#
if [ "$(uname -s)" = "Darwin" ]; then
	$COMPILER $OPTIONS -Wno-c++11-narrowing -x objective-c++ -fobjc-arc -c "$ROOT/src/sys/fs.cpp" -o "$OUTPUT/sys-fs.o"
else
	$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/fs.cpp" -o "$OUTPUT/sys-fs.o"
fi
$COMPILER $OPTIONS  -c "$ROOT/src/num/lexical/table.cpp" -o "$OUTPUT/lexical-table.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/codec/numeric.cpp" -o "$OUTPUT/codec-numeric.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/log.cpp" -o "$OUTPUT/sys-log.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/chrono.cpp" -o "$OUTPUT/sys-chrono.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/fmk.cpp" -o "$OUTPUT/sys-fmk.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/os.cpp" -o "$OUTPUT/sys-os.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/signals.cpp" -o "$OUTPUT/sys-signals.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/procre.cpp" -o "$OUTPUT/sys-procre.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/net/nwt.cpp" -o "$OUTPUT/net-nwt.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/net/addr.cpp" -o "$OUTPUT/net-addr.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/net/net.cpp" -o "$OUTPUT/net-net.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/unicode/utf8.cpp" -o "$OUTPUT/uni-utf8.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/unicode/table.cpp" -o "$OUTPUT/uni-table.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/unicode/normalize.cpp" -o "$OUTPUT/uni-normalize.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/unicode/unicode.cpp" -o "$OUTPUT/uni-unicode.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/charset/charset.cpp" -o "$OUTPUT/charset.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/charset/table.cpp" -o "$OUTPUT/charset-table.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/alloc.cpp" -o "$OUTPUT/alloc-alloc.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/cache.cpp" -o "$OUTPUT/alloc-cache.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/central.cpp" -o "$OUTPUT/alloc-central.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/classes.cpp" -o "$OUTPUT/alloc-classes.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/guard.cpp" -o "$OUTPUT/alloc-guard.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/huge.cpp" -o "$OUTPUT/alloc-huge.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/link.cpp" -o "$OUTPUT/alloc-link.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/pages.cpp" -o "$OUTPUT/alloc-pages.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/profile.cpp" -o "$OUTPUT/alloc-profile.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/source.cpp" -o "$OUTPUT/alloc-source.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/spin.cpp" -o "$OUTPUT/alloc-spin.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/trace.cpp" -o "$OUTPUT/alloc-trace.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/capture/elf.cpp" -o "$OUTPUT/alloc-elf.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/capture/mach.cpp" -o "$OUTPUT/alloc-mach.o"
##
# Внутренние имена распределителя libc берутся ТОЛЬКО под OpenBSD
#
# Файл «src/alloc/capture/obsd.cpp» собственной охраны по системе не несёт — её несёт
# сборщик: CMakeLists.txt подключает его лишь при OpenBSD. Собранный безусловно, он под
# MinGW валит связывание по «posix_memalign» и «aligned_alloc», каких у той библиотеки
# времени исполнения нет вовсе
##
OBSD=""
if [ "$(uname -s)" = "OpenBSD" ]; then
	$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/capture/obsd.cpp" -o "$OUTPUT/alloc-obsd.o"
	OBSD="$OUTPUT/alloc-obsd.o"
fi
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/capture/pe.cpp" -o "$OUTPUT/alloc-pe.o"

#
# Библиотеки системы, связыванию потребные
#
# @note Winsock тянет `sys/log` из `FileSink::rotate()`, без него под MinGW сборка
#       валится на `undefined symbol: WSAGetLastError`; Foundation нужен слою
#       файловой системы macOS
#
case "$(uname -s)" in
	#
	# @note Разбор ярлыков в «src/sys/fs.cpp» поднимает COM: «CoCreateInstance» живёт в
	#       «ole32», опознаватели - в «uuid», а сетевой слой зовёт «GetIpForwardTable»
	#       из «iphlpapi». На одном «ws2_32» связывание отвечает отсутствием
	#       «__imp_CoInitialize» (замер 06.10.2026 на стенде Windows 11)
	#
	MINGW*|MSYS*|CYGWIN*) SYSTEM_LIBS="-lws2_32 -liphlpapi -lole32 -luuid" ;;
	#
	# @note Слой файловой системы macOS зовёт «NSFileManager» да «NSURL», и без основы
	#       Foundation связывание отвечает отсутствием знака времени исполнения
	#
	Darwin) SYSTEM_LIBS="-framework Foundation" ;;
	#
	# @note Слой процессов зовёт у FreeBSD «kinfo_getproc», и без «-lutil» связывание
	#       стенда отказывает (замер 06.10.2026 на стенде FreeBSD 19)
	#
	FreeBSD) SYSTEM_LIBS="-pthread -lutil" ;;
	#
	# @note У систем Sun сетевые знаки вынесены в «-lsocket» да «-lnsl»: без них
	#       связывание отвечает отказом на «getpeername» и «if_nametoindex»
	#       (замер 06.10.2026 на OpenIndiana)
	#
	SunOS) SYSTEM_LIBS="-lsocket -lnsl" ;;
	*) SYSTEM_LIBS="" ;;
esac

# Выполняем сборку стенда сличения
#
# @note Объектные файлы перечисляются поимённо, а не маскою: посторонний объектный файл,
#       оставленный в каталоге сборки кем угодно, попадал бы в связывание и валил его
$COMPILER $OPTIONS "$ROOT/tools/verify/codec/abc/verify.cpp" \
 "$OUTPUT/codec-common.o" "$OUTPUT/codec-encoding.o" \
 "$OUTPUT/lexical-table.o" \
 "$OUTPUT/codec-numeric.o" \
 "$OUTPUT/sys-log.o" \
 "$OUTPUT/sys-chrono.o" \
 "$OUTPUT/sys-fmk.o" \
 "$OUTPUT/sys-os.o" \
 "$OUTPUT/sys-signals.o" \
 "$OUTPUT/sys-procre.o" \
 "$OUTPUT/net-nwt.o" \
 "$OUTPUT/net-addr.o" \
 "$OUTPUT/net-net.o" \
 "$OUTPUT/uni-utf8.o" \
 "$OUTPUT/uni-table.o" \
 "$OUTPUT/uni-normalize.o" \
 "$OUTPUT/uni-unicode.o" \
 "$OUTPUT/charset.o" \
 "$OUTPUT/charset-table.o" \
 "$OUTPUT/alloc-alloc.o" \
 "$OUTPUT/alloc-cache.o" \
 "$OUTPUT/alloc-central.o" \
 "$OUTPUT/alloc-classes.o" \
 "$OUTPUT/alloc-guard.o" \
 "$OUTPUT/alloc-huge.o" \
 "$OUTPUT/alloc-link.o" \
 "$OUTPUT/alloc-pages.o" \
 "$OUTPUT/alloc-profile.o" \
 "$OUTPUT/alloc-source.o" \
 "$OUTPUT/alloc-spin.o" \
 "$OUTPUT/alloc-trace.o" \
 "$OUTPUT/alloc-elf.o" \
 "$OUTPUT/alloc-mach.o" \
 "$OUTPUT/alloc-pe.o" \
 "$OUTPUT/sys-fs.o" $OBSD $SYSTEM_LIBS -pthread -lz -o "$OUTPUT/verify"

# Выводим сообщение об окончании сборки стенда
echo "Стенд собран: $OUTPUT/verify"
