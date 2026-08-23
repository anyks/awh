#!/bin/sh
#
# @file pcre2.sh
# @date 2026-08-23
#
# @license{LicenseRef-AWH-1.0}
#
# @author Yuriy Lobarev
#
# @brief Сборка эталонной реализации PCRE2 для стендов сличения
#
# @details Framework её не использует: разбор регулярных выражений ведёт модуль
#          awh::regex, её и заменивший. Нужна она лишь стендам сличения - набору
#          замеров `benchmark/regex` и проверкам сличения `tests/regex/reference.cpp`, -
#          и в сборку библиотеки не входит вовсе
#
# @note Источник эталона у обоих стендов ОДИН - подмодуль дерева. Системная сборка
#       годится плохо: версия её случайна, и расхождение с нею пришлось бы разбирать
#       всякий раз заново, гадая, наш ли это изъян или иное поведение иной версии
#
# @copyright Copyright © 2026
#
# Вызов:
#   sh/reference/pcre2.sh [каталог сборки]
#
# Выводит две строки - каталог заголовочных файлов и путь к библиотеке, - годные
# к передаче cmake:
#   cmake .. -DAWH_REFERENCE_INCLUDE_DIR=<первая> -DAWH_REFERENCE_LIBRARY=<вторая>
#
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-${TMPDIR:-/tmp}/awh-reference-pcre2}
PCRE="$ROOT/submodules/pcre2"
if [ ! -f "$PCRE/CMakeLists.txt" ]; then
	echo "ОТКАЗ: подмодуль PCRE2 не развёрнут; выполните:" >&2
	echo "  git submodule update --init --recursive submodules/pcre2" >&2
	exit 1
fi
##
# Порождение машинного кода у эталона живёт ВЛОЖЕННЫМ подмодулем
#
# Сам PCRE2 держит `deps/sljit` подмодулем своим, и без рекурсии он не разворачивается:
# заголовки на месте, а сборка падает на `pcre2_jit_compile.c` посреди пути. Отказ этот
# вышел на стенде FreeBSD и не виден вовсе на машине, где подмодуль развёрнут давно
##
if [ ! -f "$PCRE/deps/sljit/sljit_src/sljitLir.c" ]; then
	echo "ОТКАЗ: вложенный подмодуль sljit не развёрнут - эталон без него порождения кода не имеет; выполните:" >&2
	echo "  git submodule update --init --recursive submodules/pcre2" >&2
	exit 1
fi
##
# Сборщик ищется СПЕРВА рядом с cmake, и лишь затем по путям вообще
#
# У систем Sun родной make наследует SVR4 и правил, cmake порождаемых, не разбирает
# вовсе: сборка вставала там молча - файла make.log не появлялось ни одного. У MS
# Windows же беда обратная: первым по путям попадался `gmake` от Strawberry Perl,
# установке этой чужой, и запуск его отвечал 0xc0000142. Сосед cmake и есть тот
# сборщик, для которого он правила пишет, - его и берём
#
# Генератор при этом сообщается ЯВНО: умолчание cmake от системы к системе разное,
# у MSYS2 он берёт Ninja никого не спрашивая, а скрипт звал бы make по чужим правилам
##
NEAR=$(dirname "$(command -v cmake 2> /dev/null)" 2> /dev/null)
MAKE=""
for CANDIDATE in "$NEAR/ninja" "$NEAR/gmake" "$NEAR/mingw32-make" "$NEAR/make" ninja gmake make; do
	command -v "$CANDIDATE" > /dev/null 2>&1 || continue
	##
	# Годность сборщика доказывается ЗАПУСКОМ, а не наличием файла
	#
	# Установка бывает битой: у MS Windows нашёлся сборщик, отвечавший на всякий вызов
	# кодом 0xc0000142, и отказ этот всплывал лишь внутри cmake, за три шага от причины
	##
	"$CANDIDATE" --version > /dev/null 2>&1 || continue
	MAKE="$CANDIDATE"
	break
done
if [ -z "$MAKE" ]; then
	echo "ОТКАЗ: годного сборщика (ninja, gmake, make) на машине не нашлось" >&2
	exit 1
fi
case "$MAKE" in
	*ninja) GENERATOR="-GNinja"; JOBS="" ;;
	*) GENERATOR="-GUnix Makefiles"; JOBS="-j4" ;;
esac
##
# Сборщику, сообщаемому cmake, имя даётся ПОЛНОЕ - с расширением
#
# У MS Windows путь без `.exe` оболочка разрешает, а CreateProcess, каким зовёт cmake,
# уже нет: сборка отвечала «CMAKE_C_COMPILER not set», за два шага от истинной причины
##
PROGRAM=$(command -v "$MAKE")
[ -f "$PROGRAM.exe" ] && PROGRAM="$PROGRAM.exe"
##
# Сборка переиспользуется между вызовами
#
# Она занимает около минуты, а меняться ей нечего: подмодуль закреплён. Пересобрать
# её насильно можно, удалив каталог сборки целиком
##
if [ ! -f "$OUT/libpcre2-8.a" ]; then
	echo "--- сборка эталона PCRE2 ($PROGRAM $GENERATOR)" >&2
	mkdir -p "$OUT"
	##
	# Порождение машинного кода у эталона включается НАРОЧНО
	#
	# Наш модуль сличается с эталоном на равных: у обоих оба способа исполнения.
	# Эталон без кода мерил бы разбор программы против нашего машинного кода, и
	# сличение обратилось бы в похвальбу
	##
	( cd "$OUT" && cmake "$PCRE" "$GENERATOR" -DCMAKE_MAKE_PROGRAM="$PROGRAM" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
	   -DPCRE2_BUILD_PCRE2_8=ON -DPCRE2_SUPPORT_JIT=ON \
	   -DPCRE2_BUILD_TESTS=OFF -DPCRE2_BUILD_PCRE2GREP=OFF > cmake.log 2>&1 \
	  && $MAKE $JOBS > make.log 2>&1 ) || { echo "ОТКАЗ СБОРКИ эталона, смотрите $OUT/make.log и cmake.log" >&2; exit 1; }
fi
##
# Заголовок эталона порождается сборкой, а не лежит в исходниках
#
# В подмодуле его нет вовсе - там pcre2.h.in, - и путь включения ведёт в каталог сборки
##
dirname "$(find "$OUT" -name pcre2.h | head -1)"
echo "$OUT/libpcre2-8.a"
exit 0
