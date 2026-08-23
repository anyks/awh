#!/bin/sh
#
# @file stand.sh
# @date 2026-08-22
#
# @license{LicenseRef-AWH-1.0}
#
# @author Yuriy Lobarev
#
# @brief Отдельный стенд проверок распределителя памяти — сборка набора без библиотеки
#
# @details Распределитель от библиотеки не зависит, а полная её сборка на отладочных
#          стендах занимает десятки минут и требует места, какого там нет: у OpenBSD
#          свободно 2 ГБ, у Alpine и Solaris — около одного, тогда как дерево сборки
#          занимает от двух до четырёх
#
# @note Набор проверок склада тайн сюда НЕ входит: складу нужны шифрование и журнал, то
#       есть библиотека целиком. Гонять его положено в дереве
#
# @warning Стенду нужен GTest. Без него сборка отвечает отказом, а не молчит
#
# @copyright Copyright © 2026
#
# Вызов:
#   tests/alloc/stand.sh [корень дерева]
#
# Переменные окружения:
#   CXX    — собиратель, по умолчанию «c++»
#   FLAGS  — добавочные ключи сборки
#   GTEST  — корень установленного GTest; берётся первым при переборе каталогов
#
ROOT=${1:-$(cd "$(dirname "$0")/../.." && pwd)}
CXX=${CXX:-c++}
command -v "$CXX" >/dev/null 2>&1 || CXX=g++
OUT=$(mktemp -d 2>/dev/null || echo /tmp/awh-alloc-tests)
mkdir -p "$OUT"
echo "система: $(uname -s) $(uname -r) $(uname -m)"
echo "собиратель: $CXX"
##
# Свой набор исходников захвата на систему
#
# Приём захвата у систем разный: подмена именами у ELF, зона у macOS, переписывание
# входа у MS Windows
##
case "$(uname -s)" in
	Darwin) CAPTURE="$ROOT/src/alloc/capture/mach.cpp" ;;
	MINGW*|MSYS*|CYGWIN*) CAPTURE="$ROOT/src/alloc/capture/pe.cpp" ;;
	*) CAPTURE="$ROOT/src/alloc/capture/elf.cpp" ;;
esac
##
# Запрещаем собирателю числить malloc-семейство встроенным
##
NOBUILTIN="-fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free"
# Пути к GTest, когда он лежит не там, где ищет собиратель
##
# Google Test ищем там же, где и подделки: одним перебором
#
# Полагаться на то, что собиратель найдёт его сам, нельзя - у macOS он лежит в
# /opt/homebrew, куда clang не заглядывает вовсе. Переменная GTEST задаёт корень
# первым, а не единственным: заданный вручную корень главнее найденного перебором
##
GTESTFLAGS=""
GMOCK=""
for dir in ${GTEST:-} /usr/local /usr/pkg /usr /opt/local /opt/homebrew; do
	[ -d "$dir/include/gtest" ] || continue
	GTESTFLAGS="-I $dir/include -L $dir/lib"
	##
	# Google Mock связываем, лишь когда он есть
	#
	# У части систем стоит один Google Test: у OpenIndiana подделок нет вовсе.
	# Набор распределителя их не использует, и требовать их значило бы закрыть
	# себе стенд
	##
	if [ -f "$dir/lib/libgmock.a" ] || [ -f "$dir/lib/libgmock.so" ] || [ -f "$dir/lib/libgmock.dylib" ]; then
		GMOCK="-lgmock"
	fi
	break
done
echo "--- сборка набора"
$CXX -std=c++17 -O2 $NOBUILTIN $FLAGS $GTESTFLAGS -I "$ROOT/include" -o "$OUT/alloc" \
 "$ROOT/tests/alloc/stand.cpp" \
 "$ROOT/tests/alloc/capture.cpp" "$ROOT/tests/alloc/mangle.cpp" \
 "$ROOT/tests/alloc/contract.cpp" "$ROOT/tests/alloc/guard.cpp" \
 "$ROOT/tests/alloc/huge.cpp" "$ROOT/tests/alloc/zone.cpp" \
 "$ROOT/tests/alloc/purge.cpp" "$ROOT/tests/alloc/profile.cpp" \
 "$ROOT/tests/alloc/fork.cpp" \
 "$ROOT/src/alloc/alloc.cpp" "$ROOT/src/alloc/source.cpp" "$ROOT/src/alloc/pages.cpp" \
 "$ROOT/src/alloc/classes.cpp" "$ROOT/src/alloc/spin.cpp" "$ROOT/src/alloc/link.cpp" \
 "$ROOT/src/alloc/central.cpp" "$ROOT/src/alloc/cache.cpp" "$ROOT/src/alloc/guard.cpp" \
 "$ROOT/src/alloc/huge.cpp" "$ROOT/src/alloc/trace.cpp" "$ROOT/src/alloc/profile.cpp" \
 "$CAPTURE" -lgtest $GMOCK -lpthread > "$OUT/build.log" 2>&1
##
# Вывод собирателя печатается ИЗ ФАЙЛА, а не через `head` из канала
#
# Канал, закрытый на двадцать пятой строке, шлёт собирателю обрыв - и тот гибнет на
# полпути, а стенд отчитывается отказом сборки. У GCC вывод длиннее, чем у clang:
# на Debian отказ был ложным, тогда как на macOS и FreeBSD не проявлялся вовсе
##
head -40 "$OUT/build.log"
if [ ! -x "$OUT/alloc" ] && [ ! -x "$OUT/alloc.exe" ]; then
	echo "ОТКАЗ СБОРКИ: набор проверок"
	exit 1
fi
echo "--- прогон набора"
"$OUT/alloc"
rc=$?
echo "код возврата: $rc"
echo "стенд: $OUT"
exit $rc
