#!/bin/sh
#
# @file stand.sh
# @date 2026-09-01
#
# @license{LicenseRef-AWH-1.0}
#
# @author Yuriy Lobarev
#
# @brief Отдельный стенд проверок регулярных выражений — набор без сборки библиотеки
#
# @details Библиотека целиком здесь не собирается: проверить требуется один модуль,
#          а полная сборка на стендах занимает десятки минут и тянет за собою
#          третью сторону. Достаточно исходников самого модуля, разбора
#          Юникода, на который он опирается, и основания фреймворка,
#          требуемого журналом событий
#
# @note Эталонная реализация PCRE2 зависимостью библиотеки НЕ является: модуль
#       awh::regex её и заменил. Сличения с нею собираются лишь тогда, когда
#       эталон собран общим сценарием «sh/reference/pcre2.sh»; без него
#       сличения молча выключаются, а прочие проверки идут своим ходом
#
# @copyright Copyright © 2026
#
# Вызов:
#   tests/regex/stand.sh [корень дерева] [каталог стенда]
#
# Переменные окружения:
#   CXX        — собиратель, по умолчанию «c++»
#   GTEST_ROOT — корень набора GoogleTest, по умолчанию «/usr»
#   FLAGS      — добавочные ключи сборки
#   REFERENCE  — «нет» отменяет сборку эталона и сличения с ним
#
ROOT=${1:-$(cd "$(dirname "$0")/../.." && pwd)}
OUT=${2:-${TMPDIR:-/tmp}/awh-regex-tests}
##
# Ключи набора передаются ТРЕТЬИМ доводом и далее
#
# Первые два довода - корень дерева да каталог стенда, - и передавать их набору
# нельзя: GoogleTest отвечает на неведомый довод отказом разбора
##
[ $# -gt 0 ] && shift
[ $# -gt 0 ] && shift
CXX=${CXX:-c++}
command -v "$CXX" >/dev/null 2>&1 || CXX=g++
GTEST="${GTEST_ROOT:-/usr}"
mkdir -p "$OUT" || exit 1
echo "система: $(uname -s) $(uname -r) $(uname -m)"
echo "собиратель: $CXX"
echo "дерево: $ROOT"
echo "стенд: $OUT"
##
# Эталон собирается ОБЩИМ сценарием, а не своим здесь
#
# Тем же самым эталоном сличаются и замеры «benchmark/regex/stand.sh». Собирай
# его каждый стенд по-своему - разошлись бы они версией да ключами сборки, и
# расхождение пришлось бы разбирать гадая: наш ли это изъян или иное поведение
# иной сборки соперника. Сценарий общий держит источник один - подмодуль дерева
##
PCRE2=""
LIBRARY=""
if [ "$REFERENCE" != "нет" ] && [ "$REFERENCE" != "no" ]; then
	##
	# Поток ошибок сценария эталона НЕ сливается с потоком вывода
	#
	# Ход сборки он печатает в поток ошибок, а два пути - заголовков и библиотеки -
	# в поток вывода. Слитые вместе, они дали бы собирателю строки хода сборки
	# доводами, и стенд отказал бы на первом же прогоне
	##
	OUTPUT=$("$ROOT/sh/reference/pcre2.sh" "$OUT/pcre2") && {
		PCRE2=$(echo "$OUTPUT" | head -1)
		LIBRARY=$(echo "$OUTPUT" | tail -1)
	}
	if [ -z "$LIBRARY" ] || [ ! -f "$LIBRARY" ]; then
		echo "эталон PCRE2 не собран - сличения с ним отключены"
		PCRE2=""
		LIBRARY=""
	else
		echo "эталон: $LIBRARY"
	fi
fi
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
# Зависимости системные, журналом требуемые
##
##
# Сжатие журнальных файлов ведётся zlib, и способ связывания с нею ПРОВЕРЯЕТСЯ
#
# У OpenWrt библиотеки разделяемые лишены таблицы заголовков секций - система
# бережёт место, - и связывание отвечает «file in wrong format», хотя ELF
# и разрядность верны: такая библиотека годна исполнению, но не сборке.
# Оттого способ не угадывается по имени системы, а испытывается пробой:
# отказала разделяемая - берётся статическая, если она в дереве есть
##
LIBZ="-lz"
if [ "$(uname -s)" != "Darwin" ]; then
	PROBE="$OUT/probe-zlib"
	printf '#include <zlib.h>\nint main(void){ return (zlibVersion() == 0); }\n' > "$PROBE.c"
	if ! $CXX -x c "$PROBE.c" -lz -o "$PROBE" > /dev/null 2>&1; then
		for STATIC in /usr/lib/libz.a /usr/lib64/libz.a /usr/local/lib/libz.a; do
			if [ -f "$STATIC" ] && $CXX -x c "$PROBE.c" "$STATIC" -o "$PROBE" > /dev/null 2>&1; then
				echo "zlib: разделяемая связыванию не поддалась, взята $STATIC"
				LIBZ="$STATIC"
				break
			fi
		done
	fi
	rm -f "$PROBE" "$PROBE.c"
fi
LIBS="$LIBZ"
case "$(uname -s)" in
	Darwin) LIBS="$LIBS -framework Foundation -lpthread" ;;
	MINGW*|MSYS*|CYGWIN*) LIBS="$LIBS -lws2_32 -lIphlpapi -lpsapi -ldbghelp -lcrypt32 -lbcrypt -lgdi32" ;;
	*) LIBS="$LIBS -lpthread" ;;
esac
##
# Состав исходников берётся МАСКОЙ, а не перечнем поимённо
#
# Перечень отстаёт от дерева молча: приписанный к разбору Юникода файл таблиц в
# него не попал бы, и стенд отказал бы связыванием. Подкаталог «src/regex/grok»
# в маску не входит и приписан отдельно - его проверяет «tests/regex/grok.cpp».
# Оттуда же и кодек JSON: надстройка Grok выдаёт разбор деревом «json_t»
##
SUPPORT="$ROOT/src/sys/log.cpp $ROOT/src/sys/fmk.cpp $ROOT/src/sys/chrono.cpp \
 $ROOT/src/sys/os.cpp $ROOT/src/net/nwt.cpp $ROOT/src/num/lexical/table.cpp \
 $ROOT/src/encoding/charset/*.cpp $ROOT/src/alloc/*.cpp $CAPTURE"
SOURCES="$ROOT/tests/main.cpp $ROOT/tests/regex/*.cpp \
 $ROOT/src/regex/*.cpp $ROOT/src/regex/grok/*.cpp \
 $ROOT/src/encoding/unicode/*.cpp $ROOT/src/codec/json/*.cpp $SUPPORT"
##
# Признак «AWH_REGEX_PROBING» обязателен: без него набор недосчитывается путей
#
# Проверка «Regex.EngineProbing» удостоверяет, что щупы путей исполнения собраны:
# без признака они выключены, и проверка отказывает. Полная сборка ставит признак
# цели с покрытием кода, а стенд ставит его сам - иначе набор врёт отказом
##
DEFINES="-DAWH_REGEX_PROBING"
##
# Набор связывается и с GoogleMock
#
# Тело набора «tests/main.cpp» зовёт «InitGoogleMock»: без библиотеки этой
# связывание отказывает символом, к самим проверкам отношения не имеющим
##
INCLUDES="-I$ROOT/include -I$GTEST/include"
if [ -n "$LIBRARY" ]; then
	DEFINES="$DEFINES -DAWH_TEST_PCRE2 -DPCRE2_STATIC"
	INCLUDES="$INCLUDES -I$PCRE2"
fi
echo "--- сборка набора проверок"
##
# Вывод собирателя печатается ИЗ ФАЙЛА, а не через `head` из канала
#
# Канал, закрытый на сороковой строке, шлёт собирателю обрыв, и тот гибнет на полпути:
# стенд отчитывается ложным отказом сборки
##
$CXX -std=c++17 -O2 -Wno-c++11-narrowing $FLAGS $DEFINES $INCLUDES \
 -o "$OUT/regex-tests" $SOURCES $LIBRARY \
 -L"$GTEST/lib" -lgmock -lgtest -lgtest_main $LIBS > "$OUT/build.log" 2>&1
head -40 "$OUT/build.log"
if [ ! -x "$OUT/regex-tests" ] && [ ! -x "$OUT/regex-tests.exe" ]; then
	echo "ОТКАЗ СБОРКИ, смотрите $OUT/build.log"
	exit 1
fi
echo
##
# Набор запускается ИЗ КАТАЛОГА СТЕНДА
#
# Проверки хранилища пишут записи выражений файлами по путям относительным:
# запуск из иного каталога оставил бы их где придётся
##
cd "$OUT" || exit 1
"$OUT/regex-tests" "$@"
exit $?
