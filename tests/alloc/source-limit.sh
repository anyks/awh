#!/bin/sh
#
# @file source-limit.sh
# @date 2026-10-17
# @license{LicenseRef-AWH-1.0}
# @author Yuriy Lobarev
# @brief Стенд проверки размеров и бюджета источника без перехвата malloc
# @copyright Copyright © 2026
#
# Вызов: tests/alloc/source-limit.sh [корень дерева]
# CXX, FLAGS, GTEST — компилятор, дополнительные флаги, каталог Google Test.
# AWH_SOURCE_LIMIT_OUT — каталог результатов; по умолчанию новый каталог в /tmp.
# Только POSIX: системные вызовы Windows требуют отдельного стенда.
set -eu
ROOT=${1:-$(cd "$(dirname "$0")/../.." && pwd)}
CXX=${CXX:-c++}
OUT=${AWH_SOURCE_LIMIT_OUT:-$(mktemp -d /tmp/awh-source-limit.XXXXXX)}
mkdir -p "$OUT"
GTESTFLAGS=""
for dir in ${GTEST:-} /opt/homebrew /usr/local /usr/pkg /usr /opt/local; do
	if [ -d "$dir/include/gtest" ]; then
		GTESTFLAGS="-I $dir/include -L $dir/lib"
		break
	fi
done
printf 'Results: %s\n' "$OUT"
# source.cpp включён самим тестом для контролируемого перехвата mmap/munmap.
# alloc.cpp и захват malloc не связываются: надзор применим ко всем компонентам.
$CXX -std=c++17 -O3 -DNDEBUG -pthread ${FLAGS:-} $GTESTFLAGS -I "$ROOT/include" \
 "$ROOT/tests/alloc/source-limit.cpp" "$ROOT/src/alloc/huge.cpp" \
 "$ROOT/src/alloc/guard.cpp" "$ROOT/src/alloc/spin.cpp" \
 -lgtest -lgtest_main -o "$OUT/source-limit" > "$OUT/build.log" 2>&1 || {
	cat "$OUT/build.log"
	exit 1
}
# Сохраняем действительный статус прогона, не подменяя его статусом печати журнала.
STATUS=0
"$OUT/source-limit" > "$OUT/run.log" 2>&1 || STATUS=$?
cat "$OUT/run.log"
printf '%s\n' "$STATUS" > "$OUT/status"
exit "$STATUS"
