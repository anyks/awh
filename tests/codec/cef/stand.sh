#!/bin/sh
#
# @file stand.sh
# @date 2026-09-09
#
# @license{LicenseRef-AWH-1.0}
#
# @author Yuriy Lobarev
#
# @brief Отдельный стенд проверок кодека cef — сборка набора проверок без библиотеки
#        целиком, ради прогона на отладочных стендах
#
# @details Кодек cef опирается на разбор дат и на владеющее значение дерева, и собрать
#          его проверки выходит без сборки библиотеки целиком. Полная сборка её на стендах
#          занимает десятки минут и тянет за собою третью сторону, тогда как проверить
#          требуется один кодек
#
# @note Часы «sys/chrono» кодеку не попутчик, а опора: обе редакции описания разнятся
#       прежде всего видом даты, и разбор её ведётся ими. Оттого стенд обязан нести часы
#       вместе со всем, на что они опираются, - выкинуть их, как выкинута третья сторона,
#       нельзя
#
# @note Кроме самого кодека стенд несёт ВЕДЕНИЕ ЖУРНАЛА и всё, на что оно опирается:
#       кодек сообщает об отказах разбора и записи в журнал фреймворка, а тот тянет за
#       собою часы, оснастку, сетевые виды, кодировки и выделение памяти. Прежде стенд
#       обходился шестью вызовами собирателя, ныне их около тридцати, и сборка идёт
#       порядка пятнадцати секунд вместо трёх
# @warning Ключ «-Wno-c++11-narrowing» приложен только к ЧУЖИМ файлам, и переносить его
#          на свои нельзя: сужение у себя обязано оставаться отказом сборки
#
# @copyright Copyright © 2026
#
# Вызов:
#   tests/codec/cef/stand.sh [корень дерева] [каталог сборки]
#
# Переменные окружения:
#   CXX        — собиратель, по умолчанию «c++»
#   GTEST_ROOT — корень набора GoogleTest, по умолчанию «/usr»
#   FLAGS      — добавочные ключи сборки
#   ZLIB       — способ связывания с zlib, по умолчанию «-lz»
#
# @note Переменная ZLIB заведена ради систем, где разделяемая библиотека негодна
#       связыванию: у OpenWRT на musl «-lz» отвечает «file in wrong format» у всех
#       трёх разделяемых видов, а статическая «/usr/lib/libz.a» связывается
#       исправно. Звать там: ZLIB=/usr/lib/libz.a
#
# @warning Стенд собирается БЕЗ «DEBUG_MODE», а каталог build у проекта отладочный, и
#          расхождение это уже давало ложное «зелено»: кодек пишет в журнал двумя ветвями
#          по «DEBUG_MODE», и проверка на текст записи проходила у стенда, падая в общей
#          сборке. Всякую проверку, текста записи в журнал касающуюся, гонять ОБЕИМИ:
#
#              sh tests/codec/cef/stand.sh
#              Под надзирателями прогонов ДВА, и они проверяют РАЗНОЕ - один другого не
#              заменяет:
#
#              FLAGS="-fsanitize=address,undefined" sh ...
#                  Распределитель в работе: надзор за стеком, за глобальными и за
#                  неопределённым поведением. Куча при этом под охраною СВОЕЙ - заслоны,
#                  карантин, свой набор проверок, - а адресный надзиратель её НЕ видит:
#                  перехват его подменён нашими метками. Части распределителя стенд
#                  собирает без инструментации сам, см. ALLOC_OPTIONS ниже
#
#              FLAGS="-fsanitize=address,undefined -DAWH_ALLOC_DISABLED" sh ...
#                  Куча под надзирателем ЦЕЛИКОМ: обращения к освобождённому, выход за
#                  край блока. Распределитель не участвует вовсе
#
#              @warning Ключ AWH_ALLOC_DISABLED - НЕ обход, а второй законный прогон.
#                       Записка эта правлена трижды за день 15.09.2026: сперва ключ подан
#                       был правилом, затем обходом, и лишь замером по модулю
#                       распределителя выяснено, что прогона два и охват у них разный.
#                       ЗАМЕРЕНО по модулю распределителя на Debian 12: щуп с
#                       намеренной бедой отвечает «heap-use-after-free» без нашего
#                       распределителя и МОЛЧИТ с ним. Мерить только при -O0: при -O1
#                       собиратель выбрасывает пару malloc/free целиком, и молчат ОБА
#
#              @note Разделение «замерено» и «сказано» проставлено здесь намеренно: два
#                    прежних утверждения об этом ключе пришли чужим словом без замера и
#                    занесены были как свои - оттого записка и правилась трижды
#
#              FLAGS=-DDEBUG_MODE sh tests/codec/cef/stand.sh
#
#

# Прекращаем работу при первом же отказе
set -e

# Получаем корень дерева исходных текстов
ROOT="${1:-$(cd "$(dirname "$0")/../../.." && pwd)}"

# Получаем каталог собранного стенда
OUTPUT="${2:-/tmp/awh-cef-stand}"

# Получаем корень набора GoogleTest
GTEST="${GTEST_ROOT:-/usr}"

# Получаем собиратель
COMPILER="${CXX:-c++}"

# Получаем способ связывания с библиотекой сжатия
ZLIB="${ZLIB:--lz}"

# Собираем ключи сборки стенда
OPTIONS="-O2 -std=c++17 -I$ROOT/include -I$GTEST/include $FLAGS"

#
# Выполняем прописывание пути к библиотеке стандартных средств собирателя
#
# @details Собранное одним собирателем при запуске подхватывает библиотеку стандартных
#          средств ИНОГО, если тот стоит в путях поиска раньше. У DragonFly это и
#          случается: код собирается `g++14` из портов, а при запуске подхватывается
#          `libstdc++` от `gcc11`, и двоичный файл валится с «version GLIBCXX_3.4.32
#          required ... not found». Сборка при этом проходит успешно, и отказ виден лишь
#          при запуске
#
# @warning Оговорка в `case` обязательна и молчаливую порчу предотвращает: `clang`
#          отвечает на тот же вопрос ОДНИМ ИМЕНЕМ файла без пути, и `dirname` от него дал
#          бы текущий каталог. Прописав его путём поиска, мы получили бы двоичный файл,
#          ищущий библиотеку рядом с собою, - то есть замену одной беды на другую, худшую
#
# @note Отказ этот молчалив по существу раскладки: машина отчитывается пройденной, а
#       показателей с неё ноль, и дно считается по одиннадцати машинам вместо двенадцати
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
rm -f "$OUTPUT/cef-tests" "$OUTPUT/cef-tests.exe"

# Собираем перечень объектных файлов стенда
OBJECTS="$OUTPUT/lexical-table.o $OUTPUT/sys-log.o $OUTPUT/sys-fs.o $OUTPUT/sys-os.o $OUTPUT/sys-chrono.o $OUTPUT/sys-fmk.o $OUTPUT/net-nwt.o $OUTPUT/uni-normalize.o $OUTPUT/uni-table.o $OUTPUT/uni-unicode.o $OUTPUT/uni-utf8.o $OUTPUT/alloc-alloc.o $OUTPUT/alloc-cache.o $OUTPUT/alloc-central.o $OUTPUT/alloc-classes.o $OUTPUT/alloc-guard.o $OUTPUT/alloc-huge.o $OUTPUT/alloc-link.o $OUTPUT/alloc-pages.o $OUTPUT/alloc-profile.o $OUTPUT/alloc-source.o $OUTPUT/alloc-spin.o $OUTPUT/alloc-trace.o $OUTPUT/alloc-vessel.o $OUTPUT/alloc-elf.o $OUTPUT/alloc-mach.o $OUTPUT/alloc-pe.o $OUTPUT/charset.o $OUTPUT/charset-table.o"

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
#
# @warning У систем Sun сетевые имена лежат ОТДЕЛЬНО от libc: разбор адресов сети
#          «src/net/addr.cpp» зовёт `if_indextoname` и `if_nametoindex`, а те живут в
#          «libsocket» с «libnsl». Без них связывание валится «ld: fatal: symbol
#          referencing errors», причём ИМЯ символа Solaris печатает отдельной строкою
#          под шапкою «Undefined first referenced» - без слова «error», отчего отбор
#          строк отказа его и срезает
#
# @note Замерено 09.09.2026 раскладкой: стенд CEF не собрался на OpenIndiana, тогда как
#       на Solaris прошёл. Обе системы Sun, а расходятся - оттого перечень библиотек
#       задаётся по семейству, а не по машине, где отказ виден
#
case "$(uname -s)" in
	MINGW*|MSYS*|CYGWIN*) SYSTEM_LIBS="-lws2_32" ;;
	#
	# @note Разбор alias-файлов в «src/sys/fs.cpp» зовёт Foundation, и без неё
	#       связывание отказывает на средствах Objective-C
	#
	Darwin) SYSTEM_LIBS="-framework Foundation" ;;
	SunOS) SYSTEM_LIBS="-lsocket -lnsl" ;;
	*) SYSTEM_LIBS="" ;;
esac

# Выводим сообщение о начале сборки стенда
echo "Собираем стенд проверок CEF: $COMPILER"

# Выполняем сборку таблицы степеней пятёрки модуля разбора чисел
$COMPILER $OPTIONS -c "$ROOT/src/num/lexical/table.cpp" -o "$OUTPUT/lexical-table.o"

#
# Выполняем сборку ведения журнала работы и опор его
#
# @note Кодек сообщает об отказах разбора и записи в журнал фреймворка, и стенд обязан
#       нести его с собою вместе со всем, на что он опирается
# @warning Ключ «-Wno-c++11-narrowing» приложен только к ЧУЖИМ файлам: свои им глушить
#          нельзя, сужение у себя обязано оставаться отказом сборки
#
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/log.cpp" -o "$OUTPUT/sys-log.o"
#
# @note Работа кодека с файлами ведётся средством рамки «sys/fs», а не потоками языка
#       напрямую: у MS Windows узкий путь в UTF-8 кладёт файл под именем, поданному не
#       равным, и узкий же ход находит его обратно - отказа не будет никогда. Третьей
#       стороны модуль не тянет, опирается лишь на рамку и журнал
#
#
# @note Модуль «sys/fs» опирается на сведения о системе - розыск пользователя и группы
#       для смены владельца файла, - и без «src/sys/os.cpp» связывание отказывает на
#       `awh::Operating_System::group`. Третьей стороны тот не тянет
#
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/os.cpp" -o "$OUTPUT/sys-os.o"

#
# @warning Под macOS файл этот собирается как Objective-C++, а не как C++: разбор
#          alias-файлов зовёт Foundation, и сборка обычным ходом валится сотнями
#          отказов в системных заголовках. Отбор этот повторяет CMakeLists.txt, где
#          тому же файлу и только ему назначены «-x objective-c++ -fobjc-arc»
#
if [ "$(uname -s)" = "Darwin" ]; then
	$COMPILER $OPTIONS -Wno-c++11-narrowing -x objective-c++ -fobjc-arc -c "$ROOT/src/sys/fs.cpp" -o "$OUTPUT/sys-fs.o"
else
	$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/fs.cpp" -o "$OUTPUT/sys-fs.o"
fi
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/chrono.cpp" -o "$OUTPUT/sys-chrono.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/fmk.cpp" -o "$OUTPUT/sys-fmk.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/net/nwt.cpp" -o "$OUTPUT/net-nwt.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/unicode/normalize.cpp" -o "$OUTPUT/uni-normalize.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/unicode/table.cpp" -o "$OUTPUT/uni-table.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/unicode/unicode.cpp" -o "$OUTPUT/uni-unicode.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/unicode/utf8.cpp" -o "$OUTPUT/uni-utf8.o"
##
# Ключи сборки частей распределителя памяти, надзирателей НЕ несущие
#
# @details Части эти собираются БЕЗ инструментации надзирателя, тогда как всё прочее - с
# нею. Разделение это возвращает прогону полный охват: выдачу памяти обслуживает свой
# распределитель, а надзиратель следит за кодеками
#
# @warning Собранные С инструментацией, части эти валят двоичный файл до первой проверки:
#          перехваченный `malloc` зовётся из `_dl_init`, когда ASAN ещё сам себя
#          размечает, и инструментированный пролог валится о неотображённую теневую
#          память. Готовность распределителя тут ни при чём - не готов САНИТАЙЗЕР
#
# @note Прежде здесь стоял обход «-DAWH_ALLOC_DISABLED», отключавший распределитель вовсе:
#       прогон шёл с распределителем системным, и охват был уже обещанного. Рецепт этот
#       взят у модуля распределителя 15.09.2026 вместе с починкой дефекта посева
#       зерна, найденного по замеру отсюда, и проверен показанием выдачи: под надзирателем
#       ALLOCATED=73872 против 73728 без него - выдаёт наш распределитель
##
##
# Среда надзирателя берётся ДИНАМИЧЕСКОЙ, коль скоро собиратель - clang
#
# @details У систем, где среда надзирателя поставляется архивом (FreeBSD и всякий clang с
# «libclang_rt»), связывание валится повтором имён: архив определяет `malloc`, `free`,
# `calloc`, `realloc`, `aligned_alloc` и `valloc`, и наш распределитель определяет их же.
# Спор этот неразрешим по устройству связывания - два определения одного имени в двух
# объектных файлах, - и снятие инструментации его не снимает: оно про проверки, а не про
# имена. Динамическая же среда подменяется нашими метками при разрешении, ровно как у
# GNU/Linux, где она динамическая умолчанием
#
# @warning Путь к среде вписывается в двоичный файл через `rpath`: иначе запуск требует
#          переменной окружения, а её забудут. Путь спрашивается у самого собирателя ходом
#          `-print-file-name`, а НЕ `-print-runtime-dir`: последний у FreeBSD 14 указывает
#          на новый уклад каталогов, где файла нет вовсе
#
# @note Ключ `-shared-libasan` знает ОДИН clang: собиратель GNU отвечает «unrecognized
#       command-line option». Оттого отбор ведётся по собирателю, а НЕ по наличию среды в
#       системе - у Debian она нашлась при сборке через g++, и набор перестал собираться
#       вовсе. Путь этот взят у модуля распределителя 15.09.2026
##
case "$OPTIONS" in
	# Если сборка ведётся под надзирателями
	*-fsanitize=*)
		# Если собирателем является clang
		if $COMPILER --version 2>/dev/null | grep -q "clang"; then
			# Получаем путь к динамической среде надзирателя
			ASAN_LIBRARY="$($COMPILER -print-file-name=libclang_rt.asan-x86_64.so 2>/dev/null)"
			# Если динамическая среда надзирателя отыскалась
			case "$ASAN_LIBRARY" in
				/*)
					# Если файл динамической среды надзирателя на месте
					if [ -f "$ASAN_LIBRARY" ]; then
						# Просим среду надзирателя динамической
						OPTIONS="$OPTIONS -shared-libasan"
						# Вписываем путь к среде надзирателя в двоичный файл
						OPTIONS="$OPTIONS -Wl,-rpath,$(dirname "$ASAN_LIBRARY")"
					fi
				;;
			esac
		fi
	;;
esac

ALLOC_OPTIONS=""
# Выполняем перебор всех ключей сборки стенда
for OPTION in $OPTIONS; do
	# Определяем очередной ключ сборки стенда
	case "$OPTION" in
		# Ключи надзирателей частям распределителя не передаются
		-fsanitize=*|-fno-sanitize=*|-fsanitize-*) ;;
		# Прочие ключи сборки передаются частям распределителя как есть
		*) ALLOC_OPTIONS="$ALLOC_OPTIONS $OPTION" ;;
	esac
done

$COMPILER $ALLOC_OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/alloc.cpp" -o "$OUTPUT/alloc-alloc.o"
$COMPILER $ALLOC_OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/cache.cpp" -o "$OUTPUT/alloc-cache.o"
$COMPILER $ALLOC_OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/central.cpp" -o "$OUTPUT/alloc-central.o"
$COMPILER $ALLOC_OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/classes.cpp" -o "$OUTPUT/alloc-classes.o"
$COMPILER $ALLOC_OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/guard.cpp" -o "$OUTPUT/alloc-guard.o"
$COMPILER $ALLOC_OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/huge.cpp" -o "$OUTPUT/alloc-huge.o"
$COMPILER $ALLOC_OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/link.cpp" -o "$OUTPUT/alloc-link.o"
$COMPILER $ALLOC_OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/pages.cpp" -o "$OUTPUT/alloc-pages.o"
$COMPILER $ALLOC_OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/profile.cpp" -o "$OUTPUT/alloc-profile.o"
$COMPILER $ALLOC_OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/source.cpp" -o "$OUTPUT/alloc-source.o"
$COMPILER $ALLOC_OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/spin.cpp" -o "$OUTPUT/alloc-spin.o"
$COMPILER $ALLOC_OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/trace.cpp" -o "$OUTPUT/alloc-trace.o"
$COMPILER $ALLOC_OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/vessel.cpp" -o "$OUTPUT/alloc-vessel.o"
$COMPILER $ALLOC_OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/capture/elf.cpp" -o "$OUTPUT/alloc-elf.o"
$COMPILER $ALLOC_OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/capture/mach.cpp" -o "$OUTPUT/alloc-mach.o"
if [ "$(uname -s)" = "OpenBSD" ]; then
	$COMPILER $ALLOC_OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/capture/obsd.cpp" -o "$OUTPUT/alloc-obsd.o"
fi
$COMPILER $ALLOC_OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/capture/pe.cpp" -o "$OUTPUT/alloc-pe.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/charset/charset.cpp" -o "$OUTPUT/charset.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/charset/table.cpp" -o "$OUTPUT/charset-table.o"

$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/codec/numeric.cpp" -o "$OUTPUT/codec-numeric.o"
OBJECTS="$OBJECTS $OUTPUT/codec-numeric.o"

#
# Собираем переносимую подмену файла, общую всем кодекам
#
# @details Лежит в «src/codec», а не в каталоге кодека, и перебором частей кодека
#          ниже не берётся — как и «numeric.cpp», её нужно называть поимённо.
#          У MS Windows «rename» существующий файл не заменяет, потому сохранение
#          через временный файл ходит здесь, а не через вызов системы напрямую.
#
# Подмена целевого файла временным кодеком НЕ ЗОВЁТСЯ
#
# @details «src/codec/replace.cpp» из сборки стенда снят 11.09.2026: кодеки syslog и CEF
#          его не звали никогда - сохранение шло потоком, а ныне идёт через `sys/fs`, где
#          подмена живёт ходом `fs_t::replaceAddress`. Держать чужую часть в сборке ради
#          одного лишь соседства значило бы ловить её отказы своею раскладкой: 10.09.2026
#          стенд не собрался на ШЕСТИ машинах из восьми - `unique_ptr` без `<memory>`, - и
#          беда была не в кодеке вовсе
#

#
# Собираем опорные части, какие тянет за собою заголовок опознания «abc»
#
# @note Заголовок считает контрольную сумму через `awh::hashing::generate`, а тот тянет
#       числа неограниченной ширины. Третьей стороны в этой цепочке нет, оттого части эти
#       стенду по силам
#
for PART in "$ROOT/src/cryptography/hash.cpp" "$ROOT/src/num/bignum.cpp"; do
	# Выполняем сборку очередной опорной части
	$COMPILER $OPTIONS -c "$PART" -o "$OUTPUT/support-$(basename "$PART" .cpp).o"
	# Добавляем собранное к перечню объектных файлов стенда
	OBJECTS="$OBJECTS $OUTPUT/support-$(basename "$PART" .cpp).o"
done

#
# Собираем владеющее значение дерева вместе с замыканием его по «abc»
#
# @details Дерево записи журнала кодек держит на «abc::value_t», своего значения не
#          заводя. Одним файлом «value.cpp» дело не обходится: ход `absorb` принимает
#          значение документа «abc», а тот тянет за собою чтение и запись контейнера.
#          Перечень ниже и есть наименьшее замыкание, какое связывается БЕЗ третьей
#          стороны, - тот же самый восьмичастный набор, каким обходится и стенд «abc»
#
# @warning Перечень держится ВРУЧНУЮ и сокращению на глаз не подлежит: части
#          container, editor, index, chunk, signature и storage тянут шифрование и
#          сжатие, то есть ровно ту третью сторону, ради обхода какой стенд и заведён.
#          Внесение любой из них валит связывание на всякой машине раскладки разом
#
# @note Проверок частей «abc» здесь нет вовсе: они живут в своём наборе и гоняются
#       своим стендом. Взято отсюда лишь то, без чего дерево cef не связывается
#
for PART in common encoding reader writer document value header schedule; do
	# Выполняем сборку очередной части ядра контейнера
	$COMPILER $OPTIONS -c "$ROOT/src/codec/abc/$PART.cpp" -o "$OUTPUT/abc-$PART.o"
	# Добавляем собранное к перечню объектных файлов стенда
	OBJECTS="$OBJECTS $OUTPUT/abc-$PART.o"
done

#
# Собираем разбор адресов сети, какой тянет за собою кодек CEF
#
# @details Ключи расширения вида «src», «dst» и «dvc» описанием объявлены АДРЕСАМИ, и
#          кодек поверяет их разбором через `awh::net_t`, а не глазами: значение, адресом
#          не являющееся, отвечается кодом INVALID_ADDRESS. Тянет это «src/net/addr.cpp»
#          и «src/net/net.cpp», третьей стороны в них нет
#
for PART in addr net; do
	# Выполняем сборку очередной части разбора адресов сети
	$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/net/$PART.cpp" -o "$OUTPUT/net-$PART.o"
	# Добавляем собранное к перечню объектных файлов стенда
	OBJECTS="$OBJECTS $OUTPUT/net-$PART.o"
done

#
# Выполняем перебор всех частей кодека CEF
#
# @note Части «encoding» и «value» у него нет: кодировкою запись ведает сам разбор, а
#       владеющее значение взято у «abc» выше. Зато есть «dictionary» - словарь ключей
#       расширения, их видов и полных названий
#
for PART in common reader writer document dictionary; do
	# Выполняем сборку очередной части кодека cef
	$COMPILER $OPTIONS -c "$ROOT/src/codec/cef/$PART.cpp" -o "$OUTPUT/codec-$PART.o"
	# Выполняем сборку проверок очередной части кодека cef
	$COMPILER $OPTIONS -c "$ROOT/tests/codec/cef/$PART.cpp" -o "$OUTPUT/test-$PART.o"
	# Добавляем собранное к перечню объектных файлов стенда
	OBJECTS="$OBJECTS $OUTPUT/codec-$PART.o $OUTPUT/test-$PART.o"
done

# Выполняем связывание стенда проверок
#
# @note Объектные файлы перечисляются поимённо, а не маскою: посторонний объектный файл,
#       оставленный в каталоге сборки кем угодно, попадал бы в связывание и валил его
#       повтором имён
$COMPILER $OPTIONS $OBJECTS -L"$GTEST/lib" -lgtest -lgtest_main -pthread $SYSTEM_LIBS $ZLIB -o "$OUTPUT/cef-tests"

# Выводим сообщение об окончании сборки стенда
##
# Снимаем у NetBSD случайное размещение образа, коль скоро сборка шла под надзирателями
#
# @details NetBSD включает ASLR всем двоичным файлам, а адресный надзиратель с ним не
# уживается: собранный стенд печатает «This sanitizer is not compatible with enabled ASLR»
# и НЕ ПРОГОНЯЕТ НИ ОДНОЙ проверки, отвечая при том кодом ноль. Молчание это неотличимо от
# успеха, если смотреть на код возврата
#
# @warning Признак снимается с ГОТОВОГО двоичного файла и ровно в таком порядке: сперва
#          «-0» - сброс всех признаков, - затем «+a». Прямое «+a» отвечает «New flags 0x30
#          don't make sense». Зовётся «paxctl» полным путём: в «/usr/sbin» он есть, а в
#          путях поиска обычного пользователя его нет
#
# @note Замерено 15.09.2026: без этого NetBSD давала 0 проверок, с этим - 91 из 91 под
#       ASAN со своим распределителем. UBSan там отсутствует, потому «undefined» в ключах
#       давать этой машине нельзя
##
if [ "$(uname -s)" = "NetBSD" ] && [ -x /usr/sbin/paxctl ]; then
	# Определяем ключи сборки стенда
	case "$OPTIONS" in
		# Если сборка велась под надзирателями
		*-fsanitize=*)
			# Сбрасываем все признаки PaX готового двоичного файла
			/usr/sbin/paxctl -0 "$OUTPUT/cef-tests" > /dev/null 2>&1
			# Запрещаем случайное размещение образа явно
			/usr/sbin/paxctl +a "$OUTPUT/cef-tests" > /dev/null 2>&1
		;;
	esac
fi

echo "Стенд собран: $OUTPUT/cef-tests"
