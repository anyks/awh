#!/bin/sh
#
# @file stand.sh
# @date 2026-08-19
#
# @license{LicenseRef-AWH-1.0}
#
# @author Yuriy Lobarev
#
# @brief Отдельный стенд проверок бинарного контейнера ABC — сборка набора проверок без
#        библиотеки целиком, ради прогона на отладочных стендах
#
# @details Полная сборка библиотеки на стендах занимает десятки минут и тянет за собою
#          третью сторону целиком, тогда как проверить требуется один кодек. Стенд
#          собирает ядро контейнера, его проверки и опору, какую они тянут
#
# @note Опора эта выросла 23.08.2026, когда кодек перешёл на штатное логгирование:
#       журнал тянет за собою рамку, часы, разбор адресов, Юникод, перекодировку и
#       аллокатор, а сам сжимает вращаемые файлы записей и требует zlib. Прежнее
#       описание обещало «шесть вызовов собирателя без третьей стороны» — обещание
#       это более не выполнимо, и подделка договора заглушкою здесь негодна: мерить
#       следует тот же код, что уходит в библиотеку
#
# @note zlib берётся ИЗ ПОДМОДУЛЯ проекта, а не из системы: у AWH зависимости свои,
#       и стенд, связанный с системным zlib, мерил бы сборку с чужой версией. Полная
#       третья сторона стенду при этом не нужна — «libdependence.a» весит 32 МБ, своя
#       на всякий набор команд, и тянет BoringSSL, тогда как нужен один zlib
#
# @note Кадр, оглавление, правка, подпись и файловый слой стендом не берутся: они
#       опираются на модули сжатия и шифрования, а те тянут за собою третью сторону.
#       Проверено 04.09.2026 связыванием: `chunk.cpp` требует `compressor::Block`,
#       `Crypto::encrypt`/`decrypt` и `hashing::generate`, `index.cpp` — сжатие; собрать
#       их порознь можно, связать без третьей стороны нельзя
#
# @warning ЧЕГО СТЕНД НЕ СТЕРЕЖЁТ, и прежнее описание здесь ЛГАЛО. Стояло: «стенд
#          стережёт вид записи, а он весь в ядре да в заголовке опознания». Это неправда:
#          вид записи несут ЧЕТЫРЕ раскладки, а стенд берёт две. Заголовок опознания в 96
#          октетов и укладка значений — берутся; заголовок кадра в 32 октета и строка
#          оглавления в 24 — НЕТ. Порядок октетов правится там в 27 местах из 58
#
# @warning Замер, доказавший это: порядок октетов у номера кадра обращён вспять - стенд
#          прошёл 169 проверок из 169 начисто. На системе с обратным порядком октетов
#          порок такого рода дал бы молчаливо нечитаемые вместилища, и стенд, ради
#          порядка октетов по семи системам и катаемый, его бы НЕ УВИДЕЛ
#
# @note Стерегутся эти две раскладки полным набором `awh_UNITTEST_codec` - там части эти
#       есть, - но набор тот собирается лишь там, где собирается библиотека целиком со
#       всею третьей стороною. Итог стенда потому читать следует так: «две раскладки из
#       четырёх поверены здесь, две - лишь на системах полной сборки»
#
# @note Заголовок опознания и расписание фиксации взяты стендом с 22.08.2026: третьей
#       стороны они не тянут, а раскладка заголовка — 96 октетов постоянной длины с
#       контрольной суммой — как раз то место, где расхождение по порядку октетов либо
#       выравниванию вскрылось бы прежде всего
#
# @copyright Copyright © 2026
#
# Вызов:
#   tests/codec/abc/stand.sh [корень дерева] [каталог сборки]
#
# Переменные окружения:
#   CXX        — собиратель, по умолчанию «c++»
#   CC         — собиратель языка C для zlib, по умолчанию выводится из CXX
#   GTEST_ROOT — корень набора GoogleTest, по умолчанию «/usr»
#   FLAGS      — добавочные ключи сборки
#

# Прекращаем работу при первом же отказе
set -e

# Получаем корень дерева исходных текстов
ROOT="${1:-$(cd "$(dirname "$0")/../../.." && pwd)}"

# Получаем каталог собранного стенда
OUTPUT="${2:-/tmp/awh-abc-stand}"

# Получаем корень набора GoogleTest
##
# Корень набора GoogleTest
#
# @details Умолчанием стоит «/usr», но у BSD и у систем Sun третья сторона живёт в
#          «/usr/local», и путь этот у собирателя НЕ подразумевается: заголовок лежит на
#          месте, а сборка валится на «gtest/gtest.h file not found». Оттого при
#          отсутствии заголовка в «/usr» берётся «/usr/local»
#
# @note Переменная «GTEST_ROOT» подбор этот перекрывает: задавший её знает, где набор
#       у него лежит, и гадать за него незачем
##
GTEST="${GTEST_ROOT:-/usr}"

if [ ! -f "$GTEST/include/gtest/gtest.h" ]; then
	##
	# Обходим обычные места третьей стороны
	#
	# @note «/usr/local» — у BSD и у систем Sun, «/usr/pkg» — у NetBSD с pkgsrc,
	#       «/opt/homebrew» и «/usr/local» — у macOS по набору команд, «/opt/csw» — у
	#       Solaris с OpenCSW
	##
	for CANDIDATE in /usr/local /usr/pkg /opt/homebrew /opt/csw; do
		if [ -f "$CANDIDATE/include/gtest/gtest.h" ]; then
			GTEST="$CANDIDATE"
			break
		fi
	done
fi

# Получаем собиратель
COMPILER="${CXX:-c++}"

##
# Путь к набору GoogleTest добавляется, ЛИШЬ ЕСЛИ без него заголовок не находится
#
# @note Заслон этот заведён 05.09.2026 стендом Windows ARM64 (CLANGARM64), где набор
#       лежит в самом корне собирателя («/clangarm64»). Ключ «-I/clangarm64/include»
#       ставил заголовки языка C ВПЕРЕДИ заголовков libc++, и сборка валилась на
#       «<cstring> tried including <string.h> but didn't find libc++'s <string.h>».
#       Судить по имени пути нельзя - корень собирателя у всякой системы свой, - оттого
#       судим ОПЫТОМ: пробуем собрать пробу без ключа
##
GTEST_INCLUDE="-I$GTEST/include"
# Проба, опознающая доступность заголовка набора без ключа пути
if printf '#include <gtest/gtest.h>\nint main(){return 0;}\n' |
 "$COMPILER" -std=c++17 -fsyntax-only -x c++ - > /dev/null 2>&1; then
	# Выполняем снятие ключа пути: заголовок находится и без него
	GTEST_INCLUDE=""
fi

# Собираем ключи сборки стенда
OPTIONS="-O2 -std=c++17 -I$ROOT/submodules/zlib -I$ROOT/include $GTEST_INCLUDE $FLAGS"

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
##
# Ключи сборки zlib подмодуля
#
# @details Признак «HAVE_UNISTD_H» обязателен: без него «zconf.h» не подключает
#          «unistd.h», и «gzlib.c», «gzread.c», «gzwrite.c» зовут «lseek», «read»,
#          «write» и «close» необъявленными. У clang это выходит предупреждениями, а у
#          GCC 14 и новее - ОТКАЗОМ: неявное объявление работы там ошибка по умолчанию,
#          и стенд на Alpine (musl, gcc 15.2) не собирался вовсе
#
# @note Сборка проекта тот же признак ставит сама («sh/build_third_party.sh»), стенд же
#       собирает zlib своими руками - и признак этот повторить обязан
##
OPTIONS_C="-O2 -DHAVE_UNISTD_H=1 -I$ROOT/submodules/zlib"

#
# Путь к библиотеке языка C++ того собирателя, каким собран стенд
#
# @note Путь этот прописывается в двоичный файл: у DragonFly рядом стоят несколько
#       собирателей, и стенд, собранный `g++14`, при запуске подхватывал `libstdc++`
#       от gcc11 и отваливался с «version GLIBCXX_3.4.32 not found». У проверок беда
#       эта хуже, чем у замеров: незапустившийся стенд отчитывается отказом прогона, и
#       дефект пойдут искать в кодеке, какого там нет
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

# Заводим метку начала сборки, по какой сторож судит о свежести двоичного файла
#
# @note Метка эта рождена ТЕМИ ЖЕ часами, что и двоичный файл, и в том её смысл: сличать
#       время файла с временами исходников нельзя, ибо исходники приходят с чужой машины
STARTED="$OUTPUT/.started"

# Выполняем снятие прежней метки начала сборки вместе с прежним двоичным файлом
#
# @note Прежний двоичный файл сносится ЗДЕСЬ намеренно: уцелей он при отказе сборки -
#       и прогон отчитался бы зеленью о дереве, какого на стенде нет
rm -f "$OUTPUT/abc-tests"

# Заводим метку начала сборки
: > "$STARTED"

# Выполняем снос прежде собранного стенда
#
# @note Снос обязателен: при отказе сборки прежний двоичный файл остаётся на месте
#       и прогон отчитывается успехом по коду, какого в нём уже нет
rm -f "$OUTPUT/abc-tests" "$OUTPUT/abc-tests.exe"

# Собираем перечень объектных файлов стенда
OBJECTS="$OUTPUT/lexical-table.o $OUTPUT/sys-log.o $OUTPUT/sys-chrono.o $OUTPUT/sys-fmk.o $OUTPUT/charset.o $OUTPUT/charset-table.o $OUTPUT/net-nwt.o $OUTPUT/alloc-alloc.o $OUTPUT/alloc-cache.o $OUTPUT/alloc-central.o $OUTPUT/alloc-classes.o $OUTPUT/alloc-guard.o $OUTPUT/alloc-huge.o $OUTPUT/alloc-link.o $OUTPUT/alloc-pages.o $OUTPUT/alloc-profile.o $OUTPUT/alloc-source.o $OUTPUT/alloc-spin.o $OUTPUT/alloc-trace.o $OUTPUT/alloc-elf.o $OUTPUT/alloc-mach.o $OUTPUT/alloc-pe.o $OUTPUT/uni-normalize.o $OUTPUT/uni-table.o $OUTPUT/uni-unicode.o $OUTPUT/uni-utf8.o"

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
	MINGW*|MSYS*|CYGWIN*) SYSTEM_LIBS="-lws2_32" ;;
	*) SYSTEM_LIBS="" ;;
esac

# Выводим сообщение о начале сборки стенда
echo "Собираем стенд проверок ABC: $COMPILER"

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

#
# Собираем опорные части, какие тянет за собою заголовок опознания
#
# @note Заголовок считает контрольную сумму через `awh::hashing::generate`, а тот тянет
#       числа неограниченной ширины и разбор чисел. Третьей стороны в этой цепочке нет
#       вовсе, оттого части эти стенду по силам, а вместе с ними на стенд приходит
#       раскладка октетов заголовка - место, самое чувствительное к порядку октетов и
#       выравниванию

for PART in "$ROOT/src/cryptography/hash.cpp" "$ROOT/src/num/bignum.cpp"; do
	# Выполняем сборку очередной опорной части
	$COMPILER $OPTIONS -c "$PART" -o "$OUTPUT/support-$(basename "$PART" .cpp).o"
	# Добавляем собранное к перечню объектных файлов стенда
	OBJECTS="$OBJECTS $OUTPUT/support-$(basename "$PART" .cpp).o"
done

# Выполняем перебор всех частей ядра контейнера
#
# @note Частей у кодека ЧЕТЫРНАДЦАТЬ, а стенд собирает ВОСЕМЬ. Вне его остаются
#       container, editor, index, chunk, signature и storage, и остаются не по
#       недосмотру: первые пять тянут шифрование и сжатие - ровно ту третью сторону,
#       ради обхода какой стенд и заведён, - а проверки шестой опираются на правку.
#       Итог стенда читать следует как «восемь частей прошли», а не «кодек прошёл»:
#       правка, сборка контейнера, оглавление, кадры, подпись и носитель поверяются
#       лишь набором кодеков через CMake. Установлено 31.08.2026
#
for PART in common encoding reader writer document value header schedule; do
	# Выполняем сборку очередной части ядра контейнера
	$COMPILER $OPTIONS -c "$ROOT/src/codec/abc/$PART.cpp" -o "$OUTPUT/codec-$PART.o"
	# Выполняем сборку проверок очередной части ядра контейнера
	$COMPILER $OPTIONS -c "$ROOT/tests/codec/abc/$PART.cpp" -o "$OUTPUT/test-$PART.o"
	# Добавляем собранное к перечню объектных файлов стенда
	OBJECTS="$OBJECTS $OUTPUT/codec-$PART.o $OUTPUT/test-$PART.o"
done

# Выполняем связывание стенда проверок
#
# @note Объектные файлы перечисляются поимённо, а не маскою: посторонний объектный файл,
#       оставленный в каталоге сборки кем угодно, попадал бы в связывание и валил его
#       повтором имён
$COMPILER $OPTIONS $OBJECTS -L"$GTEST/lib" -lgtest -lgtest_main -pthread $SYSTEM_LIBS -o "$OUTPUT/abc-tests"

# Выполняем проверку того, что собранный стенд НОВЕЕ своих исходных текстов
#
# @note Сторож этот заведён 03.09.2026 по случаю, где стенд объявил успех сборки, а
#       прогнался ДВОИЧНЫЙ ФАЙЛ ПРОШЛОЙ ПЕРЕКАТКИ: проверок он нёс 163 вместо 166 и
#       был зелен, то есть отчитался о дереве, какого на стенде нет. Зелень старого
#       двоичного файла неотличима от зелени нового, и цена ошибки здесь полная -
#       отчёт о непроверенном дереве
#
# @note Сторож смотрит на ВРЕМЯ, а не на исход собирателя: исход стережёт `set -e`, а
#       случай тот показал, что одного его мало - двоичный файл остался старым при
#       успешном, по виду журнала, проходе. Журнал лжёт, время не лжёт
#
# @warning Сличается двоичный файл с МЕТКОЙ НАЧАЛА, заведённой этой же сборкой, а НЕ с
#          исходными текстами, и разница здесь существенна. Времена исходников приходят
#          с машины, откуда дерево раскатано, а время двоичного файла ставят часы
#          стенда, и часы эти расходятся: замер 03.09.2026 показал стенд, отстающий на
#          восемь часов. Сличение исходника с двоичным файлом сличало бы ПОКАЗАНИЯ ДВУХ
#          РАЗНЫХ ЧАСОВ - при перекосе в одну сторону сторож пропускал бы старый файл,
#          при перекосе в другую отвергал бы исправную сборку. Метка же и двоичный файл
#          рождены одними часами, и перекос им безразличен
if [ ! -f "$OUTPUT/abc-tests" ] || [ ! "$OUTPUT/abc-tests" -nt "$STARTED" ]; then
	# Выводим сообщение о том, что двоичный файл этой сборкой не рождён
	echo "ОТКАЗ: двоичный файл стенда не рождён этой сборкой" >&2
	# Прекращаем работу отказом
	exit 1
fi

# Выполняем снятие метки начала сборки
rm -f "$STARTED"

# Выводим сообщение об окончании сборки стенда вместе со временем его сборки
#
# @note Время печатается намеренно: читающий отчёт видит, ТОТ ЛИ двоичный файл прогнан,
#       не заглядывая на стенд отдельно
echo "Стенд собран: $OUTPUT/abc-tests"
ls -l "$OUTPUT/abc-tests" | awk '{print "Время сборки:", $6, $7, $8}'
