#!/bin/sh
#
# @file stand.sh
# @date 2026-08-22
#
# @license{LicenseRef-AWH-1.0}
#
# @author Yuriy Lobarev
#
# @brief Отдельный стенд замеров распределителя памяти — сличение нашего с системным
#
# @details Собирается ДВЕ программы из ОДНОГО исходника: одна на нашем распределителе,
#          другая на системном. Разница между ними одна - какой распределитель
#          обслуживает выдачу; всё прочее совпадает дословно. Вторая разница обратила
#          бы сличение в догадку
#
# @note Библиотека целиком здесь не собирается: замерить требуется один модуль, а
#       полная сборка на стендах занимает десятки минут и тянет за собою третью сторону
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
#   benchmark/alloc/stand.sh [корень дерева]
#
# Переменные окружения:
#   CXX   — собиратель, по умолчанию «c++»
#   FLAGS — добавочные ключи сборки
#
ROOT=${1:-$(cd "$(dirname "$0")/../.." && pwd)}
CXX=${CXX:-c++}
command -v "$CXX" >/dev/null 2>&1 || CXX=g++
OUT=$(mktemp -d 2>/dev/null || echo /tmp/awh-alloc-bench)
mkdir -p "$OUT"
echo "система: $(uname -s) $(uname -r) $(uname -m)"
echo "собиратель: $CXX"
echo "дерево: $ROOT"
##
# Свой набор исходников захвата на систему
#
# Приём захвата у систем разный: подмена именами у ELF, зона у macOS, переписывание
# входа у MS Windows. Лишний файл захвата не соберётся — у него свои заголовки системы
##
case "$(uname -s)" in
	Darwin) CAPTURE="$ROOT/src/alloc/capture/mach.cpp" ;;
	MINGW*|MSYS*|CYGWIN*) CAPTURE="$ROOT/src/alloc/capture/pe.cpp" ;;
	*) CAPTURE="$ROOT/src/alloc/capture/elf.cpp" ;;
esac
##
# Запрещаем собирателю числить malloc-семейство встроенным
#
# Иначе он узнаёт в нашем calloc пару «выделение плюс обнуление» и подменяет её
# вызовом... calloc, то есть себя самого
##
#
# Ключи эти идут ОБЕИМ сборкам, а не одной
#
# Со встроенным malloc собиратель вправе выбросить выдачу целиком - её итог программе
# не нужен, - и сценарий отчитывается нулевым временем. Проверено: системная сборка без
# этих ключей выбросила рост перевыдачей начисто, а прочие сценарии мерила в иных
# условиях, чем наша
##
NOBUILTIN="-fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free"
SOURCES="$ROOT/src/alloc/alloc.cpp $ROOT/src/alloc/source.cpp $ROOT/src/alloc/pages.cpp \
 $ROOT/src/alloc/classes.cpp $ROOT/src/alloc/spin.cpp $ROOT/src/alloc/link.cpp \
 $ROOT/src/alloc/central.cpp $ROOT/src/alloc/cache.cpp $ROOT/src/alloc/guard.cpp \
 $ROOT/src/alloc/huge.cpp $ROOT/src/alloc/trace.cpp $ROOT/src/alloc/profile.cpp $CAPTURE"
echo "--- сборка на нашем распределителе"
$CXX -std=c++17 -O2 $NOBUILTIN $FLAGS -I "$ROOT/include" -o "$OUT/bench-awh" \
 "$ROOT/benchmark/alloc/stand.cpp" $SOURCES -lpthread > "$OUT/awh.log" 2>&1
##
# Вывод собирателя печатается ИЗ ФАЙЛА, а не через `head` из канала
#
# Канал, закрытый на двадцатой строке, шлёт собирателю обрыв, и тот гибнет на полпути:
# стенд отчитывается ложным отказом сборки. У GCC вывод длиннее, чем у clang
##
head -40 "$OUT/awh.log"
echo "--- сборка на системном распределителе"
$CXX -std=c++17 -O2 $NOBUILTIN $FLAGS -DAWH_BENCH_SYSTEM -I "$ROOT/include" -o "$OUT/bench-sys" \
 "$ROOT/benchmark/alloc/stand.cpp" -lpthread > "$OUT/sys.log" 2>&1
head -40 "$OUT/sys.log"
FAILED=0
for name in bench-awh bench-sys; do
	if [ ! -x "$OUT/$name" ] && [ ! -x "$OUT/$name.exe" ]; then
		echo "ОТКАЗ СБОРКИ: $name"
		FAILED=$((FAILED + 1))
	fi
done
[ $FAILED -eq 0 ] || exit 1
echo
echo "=============== НАШ ==============="
"$OUT/bench-awh" || FAILED=$((FAILED + 1))
echo
echo "============ СИСТЕМНЫЙ ============"
"$OUT/bench-sys" || FAILED=$((FAILED + 1))
echo
echo "стенд: $OUT"
[ $FAILED -eq 0 ] || exit 1
exit 0
