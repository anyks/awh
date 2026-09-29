#!/bin/sh
#
# @file race.sh
# @date 2026-09-29
#
# @license{LicenseRef-AWH-1.0}
#
# @author Yuriy Lobarev
#
# @brief Стенд щупа гонок поток-локальных кэшей — сборка под ThreadSanitizer без перехвата
#
# @details Набор проверок гонок назвать не может: перехват выдачи и надзиратель друг с
#          другом не уживаются. Щуп обходится без перехвата вовсе и собирает кэши,
#          центральные списки и страничную кучу ПОД надзором. Разбор записан в
#          tests/alloc/race.cpp
#
# @note Отдельно от tests/alloc/stand.sh намеренно: у набора и щупа противоположные
#       ключи сборки модулей, и обычный прогон набора щуп не удлиняет
#
# @copyright Copyright © 2026
#
# Вызов:
#   tests/alloc/race.sh [корень дерева]
#
# Переменные окружения:
#   CXX    — собиратель, по умолчанию «c++»
#   FLAGS  — добавочные ключи сборки
#
ROOT=${1:-$(cd "$(dirname "$0")/../.." && pwd)}
CXX=${CXX:-c++}
command -v "$CXX" >/dev/null 2>&1 || CXX=g++
OUT=$(mktemp -d 2>/dev/null || echo /tmp/awh-alloc-race)
mkdir -p "$OUT"
echo "система: $(uname -s) $(uname -r) $(uname -m)"
echo "собиратель: $CXX"
echo "--- сборка щупа под ThreadSanitizer"
##
# Модули собираются ОДНИМ вызовом вместе со щупом
#
# Ключи у всех одни и те же - надзор нужен именно на модулях, - и разводить их незачем.
# Собирается лишь цепочка кэшей: захват, слой крупных выдач и прочее щупу не нужны, а
# захват и вовсе противопоказан
##
$CXX -std=c++17 -O1 -g -fsanitize=thread $FLAGS -I "$ROOT/include" -o "$OUT/race" \
 "$ROOT/tests/alloc/race.cpp" \
 "$ROOT/src/alloc/source.cpp" "$ROOT/src/alloc/pages.cpp" "$ROOT/src/alloc/classes.cpp" \
 "$ROOT/src/alloc/spin.cpp" "$ROOT/src/alloc/link.cpp" "$ROOT/src/alloc/central.cpp" \
 "$ROOT/src/alloc/cache.cpp" -lpthread > "$OUT/build.log" 2>&1
##
# Вывод собирателя печатается из файла, а не из канала: причина записана в stand.sh
##
head -40 "$OUT/build.log"
if [ ! -x "$OUT/race" ]; then
	echo "ОТКАЗ СБОРКИ: щуп гонок"
	exit 1
fi
##
# Признак случайной раскладки снимаем у NetBSD: надзиратель с нею несовместим и молчит
# при НУЛЕВОМ коде возврата. Разбор записан в stand.sh
##
if [ "$(uname -s)" = "NetBSD" ]; then
	if [ -x /usr/sbin/paxctl ]; then
		/usr/sbin/paxctl -0 "$OUT/race" > /dev/null 2>&1
		/usr/sbin/paxctl +a "$OUT/race" > /dev/null 2>&1
	else
		echo "--- ВНИМАНИЕ: /usr/sbin/paxctl не найден, надзиратель промолчит при нулевом коде"
	fi
fi
echo "--- прогон щупа"
##
# Приговор выносится по ОТЧЁТУ надзирателя, а не по коду возврата
#
# Код при найденной гонке по системам разный: у macOS надзиратель, отчитавшись,
# завершает процесс через abort (код 134, замер 29.09.2026), а не заданным `exitcode`.
# Судить по коду значило бы выдать найденные гонки за отказ самого щупа
##
##
# У Linux щуп запускается со снятой случайной раскладкой
#
# Среда надзирателя расписывает адресное пространство под свою теневую область
# заранее, и ядро с широкой случайной раскладкой (`vm.mmap_rnd_bits` = 32) кладёт
# программу мимо её расписания: Ubuntu 24.10, ядро 6.11, валится с «FATAL:
# ThreadSanitizer: unexpected memory mapping» прежде первой строки `main` (замер
# 29.09.2026). Свойство это среды надзирателя, а не щупа; настройка же ядра правится
# лишь хозяином машины, тогда как `setarch -R` снимает раскладку одному процессу
##
AWH_RACE_RUN=""
if [ "$(uname -s)" = "Linux" ] && command -v setarch >/dev/null 2>&1; then
	AWH_RACE_RUN="setarch $(uname -m) -R"
fi
TSAN_OPTIONS="halt_on_error=0 $TSAN_OPTIONS" $AWH_RACE_RUN "$OUT/race" > "$OUT/run.log" 2>&1
rc=$?
cat "$OUT/run.log"
if grep -q "WARNING: ThreadSanitizer" "$OUT/run.log"; then
	echo "итог: НАЙДЕНЫ ГОНКИ ($(grep -c "WARNING: ThreadSanitizer" "$OUT/run.log"))"
	rc=66
elif [ "$rc" -eq 0 ]; then
	echo "итог: гонок нет"
else
	echo "итог: ОТКАЗ ЩУПА"
fi
echo "код возврата: $rc"
echo "стенд: $OUT"
exit $rc
