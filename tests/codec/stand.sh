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
#        кодеками JSON, XML и YAML без библиотеки целиком, ради прогона на стендах
#
# @details Расхождение договора невидимо изнутри кодека: набор всякого из них
#          самодостаточен и проходит целиком, тогда как неправ из них один. Оттого
#          сличение и вынесено отдельным стендом, а не оставлено в наборе одного кодека
#
# @note Стенд этот собирает ВСЕ СЕМЬ кодеков разом и оттого дороже одиночных. Гонять его
#       надлежит вместе с ними, а не вместо них
#
# @warning Стенд требует собранного набора третьей стороны - «third_party/lib/libdependence.a».
#          Шесть кодеков из семи опираются на одни заголовочные файлы, а седьмой, ABC,
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
THIRD="$ROOT/third_party/include"
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
OBJECTS="$OUTPUT/lexical-table.o $OUTPUT/contract.o $OUTPUT/sys-log.o $OUTPUT/sys-chrono.o $OUTPUT/sys-fmk.o $OUTPUT/net-nwt.o $OUTPUT/uni-normalize.o $OUTPUT/uni-table.o $OUTPUT/uni-unicode.o $OUTPUT/uni-utf8.o $OUTPUT/alloc-alloc.o $OUTPUT/alloc-cache.o $OUTPUT/alloc-central.o $OUTPUT/alloc-classes.o $OUTPUT/alloc-guard.o $OUTPUT/alloc-huge.o $OUTPUT/alloc-link.o $OUTPUT/alloc-pages.o $OUTPUT/alloc-profile.o $OUTPUT/alloc-source.o $OUTPUT/alloc-spin.o $OUTPUT/alloc-trace.o $OUTPUT/alloc-elf.o $OUTPUT/alloc-mach.o $OUTPUT/alloc-pe.o $OUTPUT/charset.o $OUTPUT/charset-table.o"

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
BUILT="abc csv ini json toml xml yaml"

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
# Собираем переносимую подмену файла, общую всем кодекам
#
# @details Лежит в «src/codec», а не в каталоге кодека, и перебором частей кодека
#          ниже не берётся — как и «numeric.cpp», её нужно называть поимённо.
#          У MS Windows «rename» существующий файл не заменяет, потому сохранение
#          через временный файл ходит здесь, а не через вызов системы напрямую.
#
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/codec/replace.cpp" -o "$OUTPUT/codec-replace.o"
OBJECTS="$OBJECTS $OUTPUT/codec-replace.o"

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
for ENTRY in \
	"json:common encoding reader writer document value" \
	"xml:common encoding reader writer document value" \
	"csv:common encoding reader writer document value" \
	"yaml:common encoding reader writer document value" \
	"toml:common encoding reader writer document value" \
	"ini:common encoding reader writer document value" \
	"abc:chunk common container document editor encoding header index reader schedule signature storage value writer"; do
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
DEPENDENCE="${DEPEND:-$ROOT/third_party/lib/libdependence.a}"

##
# Если собранного набора третьей стороны на месте нет
#
# @note Сообщаем об этом ЗАРАНЕЕ и отказом: связывание без него падает перечнем имён на
#       двадцать строк, и причина в нём теряется
##
if [ ! -f "$DEPENDENCE" ]; then
	echo "Набор третьей стороны не найден: $DEPENDENCE" >&2
	echo "Соберите проект однажды либо укажите путь переменною DEPEND" >&2
	exit 1
fi

$COMPILER $OPTIONS $OBJECTS "$DEPENDENCE" -L"$GTEST/lib" -lgtest -lgtest_main -pthread ${ZLIB:--lz} $SYSTEM_LIBS -o "$OUTPUT/contract-tests"

# Выводим сообщение об окончании сборки стенда
echo "Стенд собран: $OUTPUT/contract-tests"
