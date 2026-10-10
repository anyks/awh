#!/bin/sh
#
# @file stand.sh
# @date 2026-08-21
#
# @license{LicenseRef-AWH-1.0}
#
# @author Yuriy Lobarev
#
# @brief Отдельный стенд замеров контейнера ABC — сборка набора замеров без библиотеки
#        целиком, ради снятия показателей на отладочных стендах
#
# @details Пороги набора замеров назначаются по САМОЙ МЕДЛЕННОЙ машине, а не по рабочей:
#          порог, снятый на быстрой, отказывает на медленной без всякой регрессии.
#          Полная же сборка библиотеки на стендах занимает десятки минут и тянет за
#          собою третью сторону, тогда как замерить требуется один контейнер
#
# @warning Набор замеров вместимого (benchmark/codec/abc/container.cpp) в стенд НЕ
#          входит: сборка вместимого тянет сжатие и шифрование, то есть третью сторону,
#          ради которой стенд и заводился в обход библиотеки. Снимаются здесь чтение,
#          запись, дерево документа и владеющее значение - двадцать пять сценариев из
#          тридцати одного
#
# @note Опора стенда выросла 23.08.2026, когда кодек перешёл на штатное логгирование:
#       журнал тянет за собою рамку, часы, разбор адресов, Юникод, перекодировку и
#       аллокатор, а сам сжимает вращаемые файлы записей и требует zlib. Берётся тот
#       ИЗ ПОДМОДУЛЯ проекта, а не из системы: стенд, связанный с системным zlib,
#       мерил бы сборку с чужой версией. Полная третья сторона при этом не нужна -
#       «libdependence.a» весит 32 МБ и тянет BoringSSL, тогда как нужен один zlib
#
# @note Каркас замеров опирается лишь на заголовок счётчика системных вызовов, а сам
#       счётчик разыскивается во время выполнения: без него набор печатает «нет
#       счётчика системных вызовов» и работает дальше
#
# @warning Замеры собираются только с оптимизацией: отладочная сборка замедляет всё
#          равномерно и изображает точечную регрессию там, где её нет
#
# @copyright Copyright © 2026
#
# Вызов:
#   benchmark/codec/abc/stand.sh [корень дерева] [каталог сборки]
#
# Переменные окружения:
#   CXX   — собиратель, по умолчанию «c++»
#   FLAGS — добавочные ключи сборки
#

# Прекращаем работу при первом же отказе

# Прекращаем работу при первом же отказе
set -e

# Получаем корень дерева исходных текстов
ROOT="${1:-$(cd "$(dirname "$0")/../../.." && pwd)}"

# Получаем каталог собранного стенда
OUTPUT="${2:-/tmp/awh-abc-bench}"

# Проверяем, что каталогу объектов не место в корне дерева
#
# Совпадение означает пропущенный или неверный второй аргумент: сборка тогда высыпает объекты
# вперемешку с исходниками вместо отдельного каталога
#
if [ -d "$OUTPUT" ] && [ "$(cd "$OUTPUT" && pwd)" = "$ROOT" ]; then
	echo "каталог объектов совпал с корнем дерева - сборку отклоняю"
	exit 1
fi

# Получаем собиратель
COMPILER="${CXX:-c++}"

# Собираем ключи сборки стенда
#
# @warning Оптимизация `-O3` и `NDEBUG` берутся ОБА: пороги набора сняты именно ими, а
#          `-O2` без `NDEBUG` расходится с ними втрое-впятеро на ТОЙ ЖЕ машине (замер
#          05.09.2026 на OpenBSD), и ложь эта приходит настоящими числами
#
OPTIONS="-O3 -DNDEBUG -std=c++17 -I$ROOT/submodules/zlib -I$ROOT/include -I$ROOT/tools/benchmark/syscount $FLAGS"

# Собиратель языка C и ключи его: zlib подмодуля пишется на C
#
# @note Собиратель выводится из «$COMPILER», но выведенного может и не быть: у систем
#       Sun «cc» отсутствует, а есть один «gcc». Оттого выведенное ПРОВЕРЯЕТСЯ, и при
#       отсутствии берётся первый из имеющихся. Довод: стенд обязан подниматься сам,
#       а не требовать угадывания переменной окружения на всякой системе
#
case "$COMPILER" in
	*clang++) COMPILER_C="${COMPILER%++}" ;;
	*g++) COMPILER_C="${COMPILER%++}cc" ;;
	*c++) COMPILER_C="${COMPILER%c++}cc" ;;
	*) COMPILER_C="" ;;
esac
# Если собиратель языка C задан окружением, берём его
if [ -n "$CC" ]; then
	COMPILER_C="$CC"
# Иначе разыскиваем годный собиратель среди имеющихся
elif [ -z "$COMPILER_C" ] || ! command -v "$COMPILER_C" > /dev/null 2>&1; then
	COMPILER_C=""
	for CANDIDATE in cc gcc clang; do
		if command -v "$CANDIDATE" > /dev/null 2>&1; then
			COMPILER_C="$CANDIDATE"
			break
		fi
	done
	# Если собирателя языка C не нашлось вовсе
	if [ -z "$COMPILER_C" ]; then
		# Выводим сообщение об отсутствии собирателя
		echo "Собирателя языка C не нашлось: нужен один из cc, gcc, clang либо переменная CC" >&2
		# Выходим с признаком отказа
		exit 1
	fi
fi
OPTIONS_C="-O2 -I$ROOT/submodules/zlib -DHAVE_UNISTD_H"

#
# Путь к библиотеке языка C++ того собирателя, каким собран стенд
#
# @note Путь этот прописывается в двоичный файл: у DragonFly рядом стоят несколько
#       собирателей, и стенд, собранный `g++14`, при запуске подхватывал `libstdc++`
#       от gcc11 и отваливался с «version GLIBCXX_3.4.32 not found». Собиратель по
#       умолчанию там от 2019 года, и наш код им не собрать вовсе, так что выбор
#       собирателя обязателен, а с ним обязателен и путь
#
# @warning Путь берётся лишь тогда, когда собиратель отдаёт его полным: `clang` на
#          выдачу этого вопроса отвечает одним лишь именем файла, и `dirname` от него
#          дал бы текущий каталог
#
STDLIB="$($COMPILER -print-file-name=libstdc++.so 2>/dev/null)"
case "$STDLIB" in
	/*) OPTIONS="$OPTIONS -Wl,-rpath,$(cd "$(dirname "$STDLIB")" && pwd)" ;;
esac

# Выполняем заведение каталога собранного стенда
mkdir -p "$OUTPUT"

# Выполняем снос прежде собранного стенда
#
# @note Снос обязателен: при отказе сборки прежний двоичный файл остаётся на месте
#       и прогон отчитывается успехом по коду, какого в нём уже нет
rm -f "$OUTPUT/abc-bench" "$OUTPUT/abc-bench.exe"

# Собираем перечень объектных файлов стенда
OBJECTS="$OUTPUT/lexical-table.o $OUTPUT/main.o $OUTPUT/sys-log.o $OUTPUT/sys-chrono.o $OUTPUT/sys-fmk.o $OUTPUT/charset.o $OUTPUT/charset-table.o $OUTPUT/net-nwt.o $OUTPUT/alloc-alloc.o $OUTPUT/alloc-cache.o $OUTPUT/alloc-central.o $OUTPUT/alloc-classes.o $OUTPUT/alloc-guard.o $OUTPUT/alloc-huge.o $OUTPUT/alloc-link.o $OUTPUT/alloc-pages.o $OUTPUT/alloc-profile.o $OUTPUT/alloc-source.o $OUTPUT/alloc-spin.o $OUTPUT/alloc-trace.o $OUTPUT/alloc-elf.o $OUTPUT/alloc-mach.o $OUTPUT/alloc-pe.o $OUTPUT/uni-normalize.o $OUTPUT/uni-table.o $OUTPUT/uni-unicode.o $OUTPUT/uni-utf8.o"

##
# Внутренние имена распределителя libc берутся ТОЛЬКО под OpenBSD
#
# Файл «src/alloc/capture/obsd.cpp» собственной охраны по системе не несёт - её
# несёт сборщик: CMakeLists.txt подключает его в перечень исходных текстов лишь
# при `CMAKE_SYSTEM_NAME STREQUAL "OpenBSD"`. Стенд обязан повторять этот отбор:
# собранный безусловно, он под MinGW валит связывание по `posix_memalign` и
# `aligned_alloc`, каких у той библиотеки времени исполнения нет вовсе
##
if [ "$(uname -s)" = "OpenBSD" ]; then
	OBJECTS="$OBJECTS $OUTPUT/alloc-obsd.o"
fi

##
# Системные библиотеки, каких требует ядро библиотеки
#
# У MS Windows журнал зовёт `WSAGetLastError`: посредник `__awh_strerror__` разбирает
# сетевые коды отказов, каких `strerror` от MinGW не знает. Живёт этот вызов в
# «ws2_32», и без неё связывание стенда отказывает
##
case "$(uname -s)" in
	MINGW*|MSYS*|CYGWIN*) SYSTEM_LIBS="-lws2_32 -liphlpapi -lole32 -luuid" ;;
	#
	# @note Основа «-lutil» потребна слою процессов: «src/sys/procre.cpp» зовёт у FreeBSD
	#       «kinfo_getproc», и без неё связывание стенда отказывает (замер 06.10.2026 на
	#       стенде FreeBSD 19: «undefined symbol: kinfo_getproc»)
	#
	FreeBSD) SYSTEM_LIBS="-pthread -lutil" ;;
	#
	# @warning Системам Sun нужны «libsocket» и «libnsl» отдельно: `if_nametoindex` и
	#          `getsockopt` в варианте xnet, какие зовут «src/net/addr.cpp» и
	#          «src/sys/procre.cpp», живут там, а не в libc. Без них связывание стенда
	#          отказывает «symbol referencing errors» на OpenIndiana (замер 06.10.2026:
	#          на Solaris тот же набор сходился, а на illumos — нет)
	#
	SunOS) SYSTEM_LIBS="-lsocket -lnsl" ;;
	*) SYSTEM_LIBS="" ;;
esac

#
# Разновидность связывания библиотеки стандартных средств под Windows
#
# @details Учёт выделений памяти набора замеров держит перегрузка `operator new` в
#          самом двоичном файле. Под MinGW библиотека стандартных средств подключается
#          отдельной библиотекой исполнения, и выдачи, выполненные её нешаблонным кодом,
#          замене не видны: рост строки `std::string` учётом не считается вовсе. Замер
#          06.10.2026 на стенде Windows 11 - 0 выдач против 16 при связывании со
#          статической библиотекой. Оттого здесь связывание статическое
#
# @warning Страж `counted` нулевой расход не скрывает - он объявляет измерение
#          недействительным, - но самой меры без этого ключа не будет вовсе
#
case "$(uname -s)" in
	MINGW*|MSYS*|CYGWIN*) OPTIONS="$OPTIONS -static-libstdc++ -static-libgcc" ;;
esac

# Выводим сообщение о начале сборки стенда
echo "Собираем стенд замеров ABC: $COMPILER"

# Выполняем сборку таблицы степеней пятёрки модуля разбора чисел
$COMPILER $OPTIONS -c "$ROOT/src/num/lexical/table.cpp" -o "$OUTPUT/lexical-table.o"

#
# Выполняем сборку zlib из подмодуля проекта
#
# @note Журнал сжимает вращаемые файлы записей, и без zlib не связывается. Берётся
#       он ИЗ ПОДМОДУЛЯ проекта, а не из системы: у AWH зависимости свои, ставятся
#       они `sh/build_third_party.sh`, и стенд, связанный с системным zlib, мерил бы
#       сборку с чужой версией - то есть не тот код, что уходит в библиотеку
#
# @note Собираются исходники подмодуля напрямую, а не через `build_third_party.sh`:
#       тот сводит все зависимости в `libdependence.a` (32 МБ, своя на всякий набор
#       команд) и тянет BoringSSL, а стенду нужен один zlib. Настроек у zlib проект
#       не задаёт вовсе - `./configure --static`, - и `zconf.h` подмодуля СОВПАДАЕТ
#       с собранным, оттого прямая сборка даёт тот же код
#
ZLIB="$ROOT/submodules/zlib"
if [ ! -f "$ZLIB/zlib.h" ]; then
	# Выводим сообщение об отсутствии подмодуля zlib
	echo "Нет подмодуля zlib: $ZLIB" >&2
	# Выходим с признаком отказа
	exit 1
fi
for PART in $(ls "$ZLIB"/*.c); do
	# Выполняем сборку очередной части zlib
	$COMPILER_C $OPTIONS_C -c "$PART" -o "$OUTPUT/zlib-$(basename "$PART" .c).o"
	# Добавляем собранное к перечню объектных файлов стенда
	OBJECTS="$OBJECTS $OUTPUT/zlib-$(basename "$PART" .c).o"
done

#
# Выполняем сборку ведения журнала работы и опоры его на средства системы
#
# @note Кодек сообщает об отказах разбора в журнал фреймворка, и стенд обязан нести
#       его с собою. Иного пути нет: договор кодека принимает «log_t», и подделка его
#       заглушкою мерила бы не тот код, что собирается в библиотеку
#
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/log.cpp" -o "$OUTPUT/sys-log.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/chrono.cpp" -o "$OUTPUT/sys-chrono.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/fmk.cpp" -o "$OUTPUT/sys-fmk.o"
#
# @note Опоры, какие «sys/fs.cpp» зовёт с 13.09.2026: разбор накопителя по частям держит
#       подписку на прерывание (`awh::Signals::Bus`), подписка зовёт разбор имени процесса,
#       а тому нужны сетевые адреса. Без этих четырёх частей связывание стенда валится
#       отказом, по одному виду неотличимым от просадки самого кодека. Перечень повторяет
#       замыкание проверочного стенда «tests/codec/.../stand.sh», где части эти стояли всегда
#
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/signals.cpp" -o "$OUTPUT/sys-signals.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/procre.cpp" -o "$OUTPUT/sys-procre.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/net/addr.cpp" -o "$OUTPUT/net-addr.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/net/net.cpp" -o "$OUTPUT/net-net.o"
OBJECTS="$OBJECTS $OUTPUT/sys-signals.o $OUTPUT/sys-procre.o $OUTPUT/net-addr.o $OUTPUT/net-net.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/net/nwt.cpp" -o "$OUTPUT/net-nwt.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/unicode/normalize.cpp" -o "$OUTPUT/uni-normalize.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/unicode/table.cpp" -o "$OUTPUT/uni-table.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/unicode/unicode.cpp" -o "$OUTPUT/uni-unicode.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/unicode/utf8.cpp" -o "$OUTPUT/uni-utf8.o"
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
if [ "$(uname -s)" = "OpenBSD" ]; then
	$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/capture/obsd.cpp" -o "$OUTPUT/alloc-obsd.o"
fi
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/capture/pe.cpp" -o "$OUTPUT/alloc-pe.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/charset/charset.cpp" -o "$OUTPUT/charset.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/charset/table.cpp" -o "$OUTPUT/charset-table.o"


# Выполняем сборку точки входа набора замеров
$COMPILER $OPTIONS -c "$ROOT/benchmark/main.cpp" -o "$OUTPUT/main.o"

# Выполняем перебор всех частей ядра контейнера ABC
# Выполняем сборку общих для кодеков частей
#
# @note Части эти лежат ВНЕ каталога кодека, а перечень стенда ведётся ВРУЧНУЮ: вынос
#       общего кода из кодека ломает такую сборку молча, тогда как CMake остаётся зелёным.
#       Так вышло 07.09.2026 при сведении приведения чисел к общему `awh::codec::convert` -
#       пали стенд проверок и этот, а набор и ворошитель прошли
#
for SHARED in numeric; do
	# Выполняем сборку очередной общей части
	$COMPILER $OPTIONS -c "$ROOT/src/codec/$SHARED.cpp" -o "$OUTPUT/shared-$SHARED.o"
	# Добавляем собранное к перечню объектных файлов стенда
	OBJECTS="$OBJECTS $OUTPUT/shared-$SHARED.o"
done

for PART in common encoding reader writer document value; do
	# Выполняем сборку очередной части ядра контейнера
	$COMPILER $OPTIONS -c "$ROOT/src/codec/abc/$PART.cpp" -o "$OUTPUT/codec-$PART.o"
	# Добавляем собранное к перечню объектных файлов стенда
	OBJECTS="$OBJECTS $OUTPUT/codec-$PART.o"
done

# Перечень частей набора замеров контейнера ABC, какие стенд собирает порознь
BENCH="abc reader writer document value"
#
# @warning Часть «container» в перечне НЕТ намеренно: её среда заводит собственный
#          аппарат сжатия (`awh::compressor::Block`), а тот тянет lz4, zstd и brotli,
#           тогда как стенд держится БЕЗ третьей стороны вовсе. Проверено 05.10.2026:
#           с внесённой частью связывание валится на `compressor::Block::Block`.
#           Тридцать шесть сценариев набора дают штатной сборкой `awh_BENCHMARK_codec`,
#           стенд же снимает тридцать - и поверка ниже это называет, а не прячет
#
# Перечень частей набора, каких стенд без третьей стороны не соберёт
SKIP="container"
#
# @warning Перечень ведётся ВРУЧНУЮ и обязан нести всякую часть набора, какую стенд
#          МОЖЕТ собрать: молчание о пропущенной части неотлично от полного прогона
#
# Выполняем поверку полноты перечня частей набора замеров
for FOUND in "$ROOT/benchmark/codec/abc"/*.cpp; do
	# Пропускаем строку перечня при пустом каталоге
	[ -f "$FOUND" ] || continue
	# Получаем краткое имя части набора по файлу её
	FOUND=$(basename "$FOUND" .cpp)
	# Выполняем разбор части набора по обоим перечням
	case " $BENCH " in
		# Часть набора в перечень сборки внесена - поверка не нужна
		*" $FOUND "*) continue ;;
	esac
	case " $SKIP " in
		# Часть набора объявлена несобираемой - выводим её с причиной
		*" $FOUND "*) echo "ЧАСТЬ БЕЗ ЗАМЕРОВ СТЕНДОМ: $FOUND - требует третьей стороны" ;;
		*) echo "!!! ЧАСТЬ НАБОРА ВНЕ ПЕРЕЧНЯ: $FOUND - её сценарии не замеряются" ;;
	esac
done

# Выполняем перебор всех частей набора замеров контейнера ABC
for PART in $BENCH; do
	# Выполняем сборку очередной части набора замеров
	$COMPILER $OPTIONS -c "$ROOT/benchmark/codec/abc/$PART.cpp" -o "$OUTPUT/bench-$PART.o"
	# Добавляем собранное к перечню объектных файлов стенда
	OBJECTS="$OBJECTS $OUTPUT/bench-$PART.o"
done

# Выполняем связывание стенда замеров
#
# @note Объектные файлы перечисляются поимённо, а не маскою: посторонний объектный файл,
#       оставленный в каталоге сборки кем угодно, попадал бы в связывание и валил его
#       повтором имён
$COMPILER $OPTIONS $OBJECTS -pthread $SYSTEM_LIBS -o "$OUTPUT/abc-bench"

# Выводим сообщение об окончании сборки стенда
echo "Стенд собран: $OUTPUT/abc-bench"
