#!/bin/sh
#
# @file stand.sh
# @date 2026-08-18
#
# @license{LicenseRef-AWH-1.0}
#
# @author Yuriy Lobarev
#
# @brief Отдельный стенд сличения кодеков — сборка проверок согласия договора между
#        всеми девятью кодеками дерева без библиотеки целиком, ради прогона на стендах
#
# @details Расхождение договора невидимо изнутри кодека: набор всякого из них
#          самодостаточен и проходит целиком, тогда как неправ из них один. Оттого
#          сличение и вынесено отдельным стендом, а не оставлено в наборе одного кодека
#
# @note Стенд этот собирает ВСЕ ДЕВЯТЬ кодеков разом и оттого дороже одиночных. Гонять его
#       надлежит вместе с ними, а не вместо них. Число это записано и в перечне «BUILT»,
#       и сторож сличения перечней держит его в согласии с `contract.cpp`; в шапке же
#       ничей сторож его не держит - правя перечень, правь и эти строки
#
# @warning Стенд требует собранного набора третьей стороны - «third_party/lib/libdependence.a».
#          Восемь кодеков из девяти опираются на одни заголовочные файлы, а девятый, ABC,
#          скрепляет части подписью и сжимает содержимое: за подписью стоит BoringSSL, за
#          сжатием - четыре сжимателя, и собрать их стендом нельзя. Путь к набору меняется
#          переменною DEPEND
#
# @note Правлено 07.09.2026 по донесению: перечень предметов отставал от `contract.cpp` и
#       не знал ни CSV, ни ABC, ни ведения журнала - стенд не связывался вовсе. Отставание
#       это второе по счёту, и оба раза молчание неотличимо было от согласия
#
# @copyright Copyright © 2026
#
# Вызов:
#   tests/codec/stand.sh [корень дерева] [каталог сборки]
#
# Переменные окружения:
#   CXX        — собиратель, по умолчанию «c++»
#   GTEST_ROOT — корень набора GoogleTest, по умолчанию «/usr»
#   FLAGS      — добавочные ключи сборки
#

# Прекращаем работу при первом же отказе
set -e

# Получаем корень дерева исходных текстов
ROOT="${1:-$(cd "$(dirname "$0")/../.." && pwd)}"

# Получаем каталог собранного стенда
OUTPUT="${2:-/tmp/awh-contract-stand}"

# Получаем корень набора GoogleTest
GTEST="${GTEST_ROOT:-/usr}"

# Получаем собиратель
COMPILER="${CXX:-c++}"

#
# Заголовочные файлы третьей стороны нужны ОДНОМУ кодеку из семи
#
# @note Сжиматель ABC зовёт density, lz4, zstd, brotli и прочих поимённо, и заголовки их
#       лежат подкаталогами. Шести прочим кодекам они не нужны вовсе
#
##
# Размещение третьей стороны даётся переменными окружения
#
# @details Стенд разворачивается на машине из свёртка, а свёрток третьей стороны не несёт
#          вовсе - она весит десятки мегабайт и собирается у каждой машины своя. Зато на
#          отладочных машинах стоят постоянные клоны дерева, где она уже собрана: DEPEND
#          указывает на собранный набор, AWH_THIRD - на его заголовки. Раскладка по
#          машинам разыскивает их сама
#
# @warning Без этого стенд пропускался кодом 77 на ВСЕХ восьми машинах, и договор семи
#          кодеков не сличался нигде, кроме рабочей машины. Заведено 15.09.2026
##
THIRD="${AWH_THIRD:-$ROOT/third_party/include}"
THIRD_INCLUDES="-I$THIRD"
##
# Подкаталоги перечисляются ПОИМЁННО, а не маскою
#
# @warning Маска `-I` по всем подкаталогам сюда заводит и «openssl», а тот несёт свои
#          «time.h» и «stack.h» - они заслоняют одноимённые заголовки системы, и сборка
#          валится полутора десятками отказов внутри libc++, к кодекам отношения не
#          имеющих. Заголовки BoringSSL зовутся `openssl/...` и подкаталога в путях
#          поиска не требуют вовсе
##
for ENTRY in density lz4 zstd brotli lzma snappy lizard bz2 zlib; do
	if [ -d "$THIRD/$ENTRY" ]; then
		THIRD_INCLUDES="$THIRD_INCLUDES -I$THIRD/$ENTRY"
	fi
done

##
# Заголовки третьей стороны стоят ПРЕЖДЕ каталога GoogleTest
#
# @warning Порядок здесь значим: `GTEST_ROOT=/opt/homebrew` заводит в пути поиска
#          «/opt/homebrew/include», а там лежит OpenSSL 3. Стоя первым, он перебивает
#          BoringSSL из набора третьей стороны, и подпись собиралась по заголовкам
#          OpenSSL 3, тогда как связывалась с BoringSSL: шесть имён вида
#          `EVP_PKEY_Q_keygen` не находились вовсе
##
OPTIONS="-O2 -std=c++17 -I$ROOT/include $THIRD_INCLUDES -I$GTEST/include $FLAGS"

# Выполняем заведение каталога собранного стенда
mkdir -p "$OUTPUT"

# Собираем перечень объектных файлов стенда
OBJECTS="$OUTPUT/lexical-table.o $OUTPUT/contract.o $OUTPUT/sys-log.o $OUTPUT/sys-chrono.o $OUTPUT/sys-fmk.o $OUTPUT/sys-fs.o $OUTPUT/sys-os.o $OUTPUT/net-nwt.o $OUTPUT/uni-normalize.o $OUTPUT/uni-table.o $OUTPUT/uni-unicode.o $OUTPUT/uni-utf8.o $OUTPUT/alloc-alloc.o $OUTPUT/alloc-cache.o $OUTPUT/alloc-central.o $OUTPUT/alloc-classes.o $OUTPUT/alloc-guard.o $OUTPUT/alloc-huge.o $OUTPUT/alloc-link.o $OUTPUT/alloc-pages.o $OUTPUT/alloc-profile.o $OUTPUT/alloc-source.o $OUTPUT/alloc-spin.o $OUTPUT/alloc-trace.o $OUTPUT/alloc-elf.o $OUTPUT/alloc-mach.o $OUTPUT/alloc-pe.o $OUTPUT/charset.o $OUTPUT/charset-table.o"

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

#
# Выполняем прописывание пути к библиотеке стандартных средств собирателя
#
# @details Собранное одним собирателем при запуске подхватывает библиотеку стандартных
#          средств ИНОГО, если тот стоит в путях поиска раньше. У DragonFly это и
#          случается: код собирается `g++14` из портов, а при запуске библиотеки его не
#          находится вовсе - «Shared object "libstdc++.so.6" not found». Сборка при этом
#          проходит успешно, и отказ виден лишь при запуске
#
# @warning Оговорка в `case` обязательна и молчаливую порчу предотвращает: `clang`
#          отвечает на тот же вопрос ОДНИМ ИМЕНЕМ файла без пути, и `dirname` от него дал
#          бы текущий каталог. Прописав его путём поиска, мы получили бы двоичный файл,
#          ищущий библиотеку рядом с собою, - то есть замену одной беды на другую, худшую
#
# @note Приём этот несут все восемь отдельных стендов кодеков, а стенд сличения его не
#       нёс: он и пропускался кодом 77 всюду, оттого беда эта вскрылась лишь 15.09.2026,
#       когда третья сторона машинам нашлась. Отказ был молчалив - машина печатала
#       «--- стенд ---» и ни строки итога под ним
#
STDLIB="$($COMPILER -print-file-name=libstdc++.so 2>/dev/null)"
case "$STDLIB" in
	/*) OPTIONS="$OPTIONS -Wl,-rpath,$(cd "$(dirname "$STDLIB")" && pwd)" ;;
esac

##
# Системные библиотеки, каких требует ядро библиотеки
#
# У MS Windows журнал зовёт `WSAGetLastError`: посредник `__awh_strerror__` разбирает
# сетевые коды отказов, каких `strerror` от MinGW не знает. Живёт этот вызов в
# «ws2_32», и без неё связывание стенда отказывает
##
case "$(uname -s)" in
	MINGW*|MSYS*|CYGWIN*) SYSTEM_LIBS="-lws2_32" ;;
	##
	# Основа «Foundation» потребна слою файловой системы: «src/sys/fs.cpp» зовёт у macOS
	# «NSFileManager», и без неё связывание отвечает отсутствием знаков Objective-C
	##
	Darwin) SYSTEM_LIBS="-framework Foundation" ;;
	##
	# Системы Sun держат сетевые вызовы вне libc: «src/net/addr.cpp» зовёт
	# «if_nametoindex» и «if_indextoname», а живут они в «libsocket» с «libnsl»
	#
	# @warning Без этой ветви связывание отказывало у OpenIndiana на обоих именах разом,
	#          тогда как Solaris ту же сборку проходил. Замерено 15.09.2026
	##
	SunOS) SYSTEM_LIBS="-lsocket -lnsl" ;;
	*) SYSTEM_LIBS="" ;;
esac

# Выводим сообщение о начале сборки стенда
##
# Выполняем сверку перечня кодеков с тем, какие зовёт сам щуп
#
# @details Перечень ниже держится ВРУЧНУЮ, а обещание его - «отвечать перечню кодеков, какие
#          зовёт contract.cpp» - записано было одним лишь словом. Обещание, записанное словом,
#          своего предмета проверки не имеет и расходится с делом МОЛЧА: так стенд и отстал
#          дважды - при внесении TOML с INI и при внесении CSV с ABC, - а раскладка по машинам
#          печатала «СБОРКА ОТКАЗАЛА» и шла дальше
#
# @note Сверка берёт имена кодеков из подключаемых файлов щупа и сличает их с перечнем ниже.
#       Расхождение прекращает сборку ТОТЧАС и называет недостающее поимённо - вместо двадцати
#       строк неразрешённых имён от связывателя
#
# @warning Сверка эта стережёт САМО ОБЕЩАНИЕ, а не работу стенда, и в том её назначение
##
CALLED=$(sed -n 's|^#include <codec/\([a-z]*\)/.*|\1|p' "$ROOT/tests/codec/contract.cpp" | sort -u)

# Перечень кодеков, какие собирает стенд
BUILT="abc cef csv ini json syslog toml xml yaml"

# Выполняем поиск кодеков, щупом званых, но стендом не собираемых
MISSING=""
for CODEC in $CALLED; do
	case " $BUILT " in
		*" $CODEC "*) ;;
		*) MISSING="$MISSING $CODEC" ;;
	esac
done

##
# Если щуп зовёт кодек, какого перечень стенда не знает
#
# @note Прекращаем сборку тотчас: связывание всё равно откажет, но назовёт беду двадцатью
#       строками неразрешённых имён, из каких причина не читается
##
if [ -n "$MISSING" ]; then
	echo "Перечень стенда отстал от tests/codec/contract.cpp: нет кодеков$MISSING" >&2
	echo "Допишите их в перечень частей ниже по тексту стенда" >&2
	exit 1
fi

##
# Сверка ВТОРАЯ, обратная: кодек, в дереве заведённый, но щупом сличения не званый
#
# @note Первая сверка стережёт перечень стенда, отстающий от щупа. Эта - сам щуп, отстающий
#       от дерева: заведись десятый кодек, договор его никто не сличит, и молчание опять
#       будет неотличимо от согласия. Проверка `OversizedPathIndexIsRefusedByEveryCodec`
#       обещает словом «всякий», и держится обещание это ЗДЕСЬ, а не чтением имени
#
# @warning Отказом сборку не валим: кодек может быть заведён и не дописан, а стенд - вещь
#          повседневная. Кричим внятно и идём дальше; решение о внесении за владельцем
##
PRESENT=$(ls -1 "$ROOT/src/codec" | sed -n 's|^\([a-z][a-z0-9]*\)$|\1|p' | sort -u | tr '\n' ' ')

# Перечень кодеков, щупом званых, приведённый к одной строке ради сличения образцом
NAMED=$(echo "$CALLED" | tr '\n' ' ')

# Выполняем поиск кодеков, в дереве заведённых, но щупом сличения не званых
UNTESTED=""
for CODEC in $PRESENT; do
	case " $NAMED " in
		*" $CODEC "*) ;;
		*) UNTESTED="$UNTESTED $CODEC" ;;
	esac
done

# Если дерево содержит кодек, договор какого не сличается ничем
if [ -n "$UNTESTED" ]; then
	echo "ВНИМАНИЕ: щуп сличения не знает кодеков дерева:$UNTESTED" >&2
	echo "Договор их не сличается ни с чем - внесите их в tests/codec/contract.cpp" >&2
fi

##
# Проверка наличия третьей стороны, стоящая ДО всякой сборки
#
# @details Стенду нужны и собранный набор `third_party/lib/libdependence.a`, и ЗАГОЛОВКИ
# третьей стороны: части сжатия подключают `density_api.h` и его собратьев прямо в теле.
# Отладочные машины ни того, ни другого не несут - ради обхода третьей стороны отдельные
# стенды и заводились, - и стенд сличения годен лишь там, где дерево собрано целиком
#
# @warning Правлено 08.09.2026 по замеру раскладки: заслон стоял ПОСЛЕ сборки и стерёг одну
#          лишь библиотеку, отчего все восемь машин падали на «density_api.h: No such file or
#          directory» - отказом сборки, неотличимым от беды кодеков. Заслон перенесён к
#          началу и стережёт ОБА условия
#
# @note Отказ отдаётся кодом 77, а не 1: раскладка по машинам обязана отличать «стенд здесь
#       неприменим» от «сборка отказала». Число это взято у GNU Automake, где 77 означает
#       ровно пропуск проверки
##
DEPENDENCE="${DEPEND:-$ROOT/third_party/lib/libdependence.a}"

# Если собранного набора третьей стороны на месте нет
if [ ! -f "$DEPENDENCE" ]; then
	echo "Третьей стороны нет: $DEPENDENCE" >&2
	echo "Стенд сличения годен лишь на дереве, собранном целиком - соберите проект однажды либо укажите путь переменною DEPEND" >&2
	exit 77
fi

# Если заголовков третьей стороны на месте нет
if [ ! -f "$THIRD/density/density_api.h" ] && [ ! -f "$THIRD/density_api.h" ]; then
	echo "Заголовков третьей стороны нет: $THIRD" >&2
	echo "Стенд сличения годен лишь на дереве, собранном целиком - части сжатия подключают их прямо в теле" >&2
	exit 77
fi

echo "Собираем стенд сличения кодеков: $COMPILER"

# Выполняем сборку таблицы степеней пятёрки модуля разбора чисел
$COMPILER $OPTIONS -c "$ROOT/src/num/lexical/table.cpp" -o "$OUTPUT/lexical-table.o"

# Выполняем сборку проверок сличения кодеков
$COMPILER $OPTIONS -c "$ROOT/tests/codec/contract.cpp" -o "$OUTPUT/contract.o"

#
# Выполняем сборку ведения журнала работы и опор его
#
# @note Кодек сообщает об отказах разбора и записи в журнал фреймворка, и стенд обязан
#       нести его с собою вместе со всем, на что он опирается
# @warning Ключ «-Wno-c++11-narrowing» приложен только к ЧУЖИМ файлам: свои им глушить
#          нельзя, сужение у себя обязано оставаться отказом сборки
#
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/log.cpp" -o "$OUTPUT/sys-log.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/chrono.cpp" -o "$OUTPUT/sys-chrono.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/fmk.cpp" -o "$OUTPUT/sys-fmk.o"
##
# Собираем слой файловой системы и сведения о системе
#
# @details Работу с файлами кодеки ведут через «sys/fs» - чтение, запись, снос и
#          подмену, - а прежняя переносимая подмена «src/codec/replace.cpp» снесена
#          владельцем как нарушение договора. Без этих двух частей связывание отвечает
#          отсутствием знаков «awh::Filesystem» у всякого кодека, файлы читающего
#
# @note У macOS слой этот написан на Objective-C++ и обычным C++ не собирается вовсе:
#       приходит «stray @ in program». Прочим системам ключи эти не нужны и вредны,
#       оттого ветвь по «uname», а не общий довод
##
if [ "$(uname -s)" = "Darwin" ]; then
	$COMPILER $OPTIONS -Wno-c++11-narrowing -x objective-c++ -fobjc-arc -c "$ROOT/src/sys/fs.cpp" -o "$OUTPUT/sys-fs.o"
else
	$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/fs.cpp" -o "$OUTPUT/sys-fs.o"
fi
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/os.cpp" -o "$OUTPUT/sys-os.o"
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

$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/codec/numeric.cpp" -o "$OUTPUT/codec-numeric.o"
OBJECTS="$OBJECTS $OUTPUT/codec-numeric.o"

#
# Переносимая подмена файла «src/codec/replace.cpp» отсюда СНЯТА
#
# @details Часть эта снесена владельцем как нарушение договора: подмену файла ведёт
#          теперь слой файловой системы «sys/fs», и второй её записи в дереве нет.
#          Стенд же продолжал звать её поимённо и оттого не собирался ВОВСЕ - ни у кого
#          и ни на одной машине: сборка обрывалась на «no such file or directory», а
#          договор семи кодеков не сличался с тех пор ничем
#
# @note Снято 15.09.2026. Отсутствие стенда договора неотличимо от стенда пройденного:
#       раскладка по машинам пропускала его кодом 77 «проверка неприменима», и молчание
#       это выглядело как согласие
#

#
# Собираем подпись содержимого, какой требует кодек ABC
#
# @note Кодек ABC скрепляет свои части подписью, а та живёт в «src/cryptography/crypto.cpp».
#       Прочие шесть кодеков к шифрованию не обращаются вовсе, и в одиночных их стендах
#       файла этого нет
#
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/cryptography/crypto.cpp" -o "$OUTPUT/crypto.o"
OBJECTS="$OBJECTS $OUTPUT/crypto.o"

#
# Собираем сжатие содержимого, какого требует кодек ABC
#
# @note Части свои ABC сжимает, и сжиматель зовёт четыре набора третьей стороны разом
#
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/compressor/block.cpp" -o "$OUTPUT/compressor-block.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/compressor/stream.cpp" -o "$OUTPUT/compressor-stream.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/compressor/types.cpp" -o "$OUTPUT/compressor-types.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/num/bignum.cpp" -o "$OUTPUT/num-bignum.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/cryptography/hash.cpp" -o "$OUTPUT/crypto-hash.o"
OBJECTS="$OBJECTS $OUTPUT/compressor-block.o $OUTPUT/compressor-stream.o $OUTPUT/compressor-types.o $OUTPUT/num-bignum.o $OUTPUT/crypto-hash.o"

#
# Выполняем перебор всех сличаемых кодеков вместе с составом частей каждого
#
# @warning Перечень этот держится ВРУЧНУЮ и обязан отвечать перечню кодеков, какие зовёт
#          `contract.cpp`. Разойдись они - стенд не свяжется вовсе, а раскладка по
#          машинам напечатает «СБОРКА ОТКАЗАЛА» и пойдёт дальше: проверка, ради какой
#          стенд и заведён, окажется не прогнанной ни разу, и молчание это неотличимо от
#          согласия. Так и вышло дважды: при внесении TOML с INI в щуп и 07.09.2026, когда
#          `contract.cpp` стал звать CSV и ABC, а перечень их не знал вовсе
#
# @note Состав частей у кодеков разный, и перечисляется он поимённо: у CSV нет части
#       `writer` отдельным именем разбора, у ABC частей четырнадцать. Маскою `*.cpp`
#       перечень не берётся - посторонний файл, в каталоге оставленный, попал бы в сборку
#
#
# Собираем проверку адресов сети, какую тянет за собою кодек CEF
#
# @note Кодек CEF сличает поля адресов через «net_addr_t», и без этих двух частей стенд
#       не связывается. Прочим восьми кодекам сети не нужны вовсе
#
for PART in "$ROOT/src/net/addr.cpp" "$ROOT/src/net/net.cpp"; do
	# Выполняем сборку очередной части проверки адресов сети
	$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$PART" -o "$OUTPUT/net-$(basename "$PART" .cpp).o"
	# Добавляем собранное к перечню объектных файлов стенда
	OBJECTS="$OBJECTS $OUTPUT/net-$(basename "$PART" .cpp).o"
done

for ENTRY in \
	"json:common encoding reader writer document value" \
	"xml:common encoding reader writer document value" \
	"csv:common encoding reader writer document value" \
	"yaml:common encoding reader writer document value" \
	"toml:common encoding reader writer document value" \
	"ini:common encoding reader writer document value" \
	"abc:chunk common container document editor encoding header index reader schedule signature storage value writer" \
	"syslog:common reader writer document dictionary" \
	"cef:common reader writer document dictionary"; do
	# Получаем название очередного сличаемого кодека
	CODEC="${ENTRY%%:*}"
	# Выполняем перебор всех частей очередного кодека
	for PART in ${ENTRY#*:}; do
		# Выполняем сборку очередной части кодека
		$COMPILER $OPTIONS -c "$ROOT/src/codec/$CODEC/$PART.cpp" -o "$OUTPUT/$CODEC-$PART.o"
		# Добавляем собранное к перечню объектных файлов стенда
		OBJECTS="$OBJECTS $OUTPUT/$CODEC-$PART.o"
	done
done

# Выполняем связывание стенда сличения
#
# @note Объектные файлы перечисляются поимённо, а не маскою: посторонний объектный файл,
#       оставленный в каталоге сборки кем угодно, попадал бы в связывание и валил его
#       повтором имён
#
# Собранный набор третьей стороны, какого требует кодек ABC
#
# @details Шесть кодеков из семи опираются на одни заголовочные файлы, и стенд оттого
#          собирается без библиотеки целиком. Седьмой - ABC - скрепляет части подписью и
#          сжимает содержимое, а за подписью стоит BoringSSL, за сжатием - четыре
#          сжимателя. Собрать их стендом нельзя, и потому берётся собранный набор
#
# @warning Без него связывание валится на `i2d_PUBKEY` и `compressor::Block::compress`.
#          Собирается он один раз сборкою проекта: `third_party/lib/libdependence.a`.
#          Переменная DEPEND даёт указать иное его размещение
#
##
# @note Заслон наличия третьей стороны стоит ВЫШЕ, до всякой сборки: здесь он был бы поздним -
#       сборка частей сжатия падает на заголовках третьей стороны прежде связывания
##
$COMPILER $OPTIONS $OBJECTS "$DEPENDENCE" -L"$GTEST/lib" -lgtest -lgtest_main -pthread ${ZLIB:--lz} $SYSTEM_LIBS -o "$OUTPUT/contract-tests"

# Выводим сообщение об окончании сборки стенда
echo "Стенд собран: $OUTPUT/contract-tests"
