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
##
# Сборки на распределителях-соперниках
#
# Исходник тот же самый и ключи те же: разница ОДНА - какая библиотека обслуживает
# выдачу. Соперники подменяют malloc-семейство собою при связывании, ровно как и мы у
# систем ELF, оттого сборка их ничем не отличается от системной, кроме одной библиотеки
#
# Соперника берём, лишь когда он на машине есть: перечень их у систем разный, а
# требовать всех значило бы закрыть себе стенд. У FreeBSD, к слову, СИСТЕМНЫЙ
# распределитель и ЕСТЬ jemalloc - отдельного пакета там нет вовсе, и это не пробел
##
RIVALS=""
##
# Пути поиска библиотек соперников при ЗАПУСКЕ
#
# Собраться мало: у NetBSD библиотеки лежат в /usr/pkg/lib, куда загрузчик сам не
# смотрит, и собранная программа отвечает «Shared object not found» уже на запуске.
# Ключ этот вписывает путь в саму программу
##
##
# Пути ищутся И при СБОРКЕ, а не только при запуске
#
# Прежде здесь стоял один `-rpath`, то есть путь для загрузчика, а искать библиотеку
# при связывании собирателю было негде. Связывание при этом молча УДАВАЛОСЬ: `malloc` и
# `free` есть в самой libc, и программа выходила без соперника вовсе - а столбец его в
# отчёте показывал СИСТЕМНЫЙ распределитель под чужим именем
#
# Проверено на этой самой машине: `otool -L` у собранного стенда соперника показывал
# один libSystem, а рядом лежала библиотека tcmalloc набора x86_64 при машине arm64.
# Связыватель её отбрасывал предупреждением - не отказом, - и всё сходилось
##
##
# Каталоги соперников перебираются ПООДИНОЧКЕ, а не складываются в одну строку
#
# Складывать их нельзя по двум причинам, обе проверены на этой самой машине.
#
# Первая: без `-L` собиратель библиотеку не ищет вовсе, а связывание молча УДАЁТСЯ -
# `malloc` и `free` есть в самой libc, и программа выходит без соперника. Столбец его
# в отчёте показывал СИСТЕМНЫЙ распределитель под чужим именем, а `otool -L` у стенда
# соперника - один libSystem. Так сличение с порогом велось не с тем, с кем задумано.
#
# Вторая: сложенные вместе каталоги ссорятся. Рядом лежала библиотека tcmalloc набора
# x86_64 при машине arm64; связыватель встречал её первой, отбрасывал ПРЕДУПРЕЖДЕНИЕМ
# и до годной библиотеки в соседнем каталоге не доходил. Оттого каждый каталог
# пробуется сам по себе, и годным считается первый, чей соперник ДОКАЗАЛ обслуживание
##
DIRS=""
for dir in /usr/pkg/lib /usr/local/lib /opt/homebrew/lib /opt/local/lib; do
	[ -d "$dir" ] && DIRS="$DIRS $dir"
done
# Пустой каталог означает поиск по умолчанию: у большинства систем библиотека лежит там
DIRS="_ $DIRS"
##
# Ищем соперников пробной сборкой, а не наличием файла
#
# Файл библиотеки может лежать без заголовков или быть неподходящего набора команд;
# связывание отвечает на вопрос «годен ли он» окончательно
##
for rival in jemalloc tcmalloc_minimal; do
	# Признак учёта, каким соперник доказывает обслуживание выдачи
	SERVES=""
	case "$rival" in
		tcmalloc_minimal|tcmalloc) SERVES="tcmalloc" ;;
		jemalloc) SERVES="jemalloc" ;;
	esac
	# Ключи связывания годного каталога
	FOUND=""
	##
	# Найденность отмечается ОТДЕЛЬНЫМ признаком, а не пустотой ключей
	#
	# Ключи годного каталога пусты, когда соперник лежит на пути поиска по умолчанию, -
	# а так он лежит у большинства систем Linux. Судить о находке по пустоте строки
	# значило бы объявлять «не доказано» именно там, где соперник и вправду обслуживает
	# выдачу: обе пробы проходили, признак оставался пустым, и соперник отбрасывался.
	# Проверено на Debian 12, где jemalloc и tcmalloc лежат в /usr/lib
	##
	SEIZED=""
	##
	# Перебираем каталоги ПООДИНОЧКЕ
	#
	# Годным считается первый каталог, чей соперник прошёл все три испытания подряд:
	# связался, запустился и ДОКАЗАЛ обслуживание выдачи. Первые два по отдельности
	# ничего не значат - оба проходят и при библиотеке, не связанной вовсе
	##
	for dir in $DIRS; do
		# Собираем ключи каталога: подчёркивание означает поиск по умолчанию
		if [ "$dir" = "_" ]; then LINK=""; else LINK="-L$dir -Wl,-rpath,$dir"; fi
		# Пробуем связаться с соперником из этого каталога
		$CXX -std=c++17 -O2 $FLAGS $LINK -o "$OUT/probe-$rival" -x c++ - "-l$rival" > /dev/null 2>&1 <<-'PROBE' || continue
		#include <cstdlib>
		int main(){ void * one = ::malloc(64); ::free(one); return 0; }
		PROBE
		##
		# Собраться мало - соперник обязан ЗАПУСТИТЬСЯ
		#
		# Программа, не нашедшая библиотеку при запуске, печатает пустоту, и свод по
		# такому выводу отчитывается «соперника нет» вместо отказа
		##
		"$OUT/probe-$rival" > /dev/null 2>&1 || continue
		##
		# Запуститься мало - соперник обязан ДОКАЗАТЬ, что обслуживает выдачу
		#
		# Связывание само по себе не значит, что `malloc` достался сопернику: `malloc`
		# и `free` есть в самой libc, и связывание УДАЁТСЯ даже тогда, когда библиотека
		# соперника отброшена по несходству набора команд - предупреждением, не отказом.
		# Проверено здесь же: `otool -L` у стенда соперника показывал один libSystem, а
		# столбец в отчёте всё это время сличал нас с СИСТЕМНЫМ распределителем под
		# чужим именем
		#
		# Доказательством служит УЧЁТ самого соперника: берём мегабайт обычным `malloc`
		# и смотрим, вырос ли счёт занятого у него. Признак этот единственный надёжный.
		# Проверено на трёх щупах: `dladdr` у `malloc` называет библиотеку, где имя
		# ОПРЕДЕЛЕНО, а не ту, что его обслуживает, и всегда отвечает системной; а
		# `malloc_zone_from_ptr` у macOS зовёт зоной «DefaultMallocZone» даже блоки
		# самого tcmalloc. Оба щупа отвечали «соперник в стороне», и оба врали
		##
		[ -n "$SERVES" ] || { FOUND="$LINK"; SEIZED="да"; break; }
		$CXX -std=c++17 -O2 $FLAGS $LINK -D"AWH_RIVAL_$SERVES" -o "$OUT/serves-$rival" -x c++ - "-l$rival" > "$OUT/serves-$rival.log" 2>&1 <<-'SERVE' || continue
		#include <cstdlib>
		#include <cstddef>
		#if defined(AWH_RIVAL_tcmalloc)
			extern "C" int MallocExtension_GetNumericProperty(const char *, size_t *);
			static bool counted(size_t & value){ return (MallocExtension_GetNumericProperty("generic.current_allocated_bytes", &value) != 0); }
		#else
			extern "C" int mallctl(const char *, void *, size_t *, void *, size_t);
			static bool counted(size_t & value){
				unsigned long long epoch = 1; size_t span = sizeof(epoch);
				mallctl("epoch", &epoch, &span, &epoch, span);
				size_t size = sizeof(value);
				return (mallctl("stats.allocated", &value, &size, nullptr, 0) == 0);
			}
		#endif
		int main(){
			size_t before = 0, after = 0;
			if(!counted(before)) return 2;
			void * blocks[256];
			for(int i = 0; i < 256; i++) blocks[i] = ::malloc(4096);
			if(!counted(after)) return 2;
			for(int i = 0; i < 256; i++) ::free(blocks[i]);
			return ((after - before) > 500000) ? 0 : 1;
		}
		SERVE
		# Соперник обязан ответить приростом учёта
		"$OUT/serves-$rival" > /dev/null 2>&1 || continue
		# Каталог годен: соперник доказал обслуживание
		FOUND="$LINK"
		SEIZED="да"
		break
	done
	##
	# Недоказанного соперника НЕ БЕРЁМ вовсе
	#
	# Столбец недоказанного соперника хуже отсутствия столбца: он выглядит сличением, а
	# сличает нас с системным распределителем под чужим именем. Порог скорости по такому
	# столбцу судить нельзя
	##
	if [ -z "$SEIZED" ]; then
		echo "--- соперник $rival: обслуживание выдачи НЕ ДОКАЗАНО ни в одном каталоге - пропущен"
		continue
	fi
	RIVALS="$RIVALS $rival"
	echo "--- соперник $rival: обслуживание выдачи ДОКАЗАНО учётом"
	echo "--- сборка на сопернике: $rival"
	$CXX -std=c++17 -O2 $NOBUILTIN $FLAGS -DAWH_BENCH_SYSTEM -I "$ROOT/include" \
	 -o "$OUT/bench-$rival" "$ROOT/benchmark/alloc/stand.cpp" $FOUND "-l$rival" -lpthread \
	 > "$OUT/$rival.log" 2>&1
	head -40 "$OUT/$rival.log"
done
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
for rival in $RIVALS; do
	if [ -x "$OUT/bench-$rival" ]; then
		echo
		echo "============ $rival ============"
		"$OUT/bench-$rival" || FAILED=$((FAILED + 1))
	else
		echo
		echo "ОТКАЗ СБОРКИ соперника: $rival"
		FAILED=$((FAILED + 1))
	fi
done
echo
echo "стенд: $OUT"
[ $FAILED -eq 0 ] || exit 1
exit 0
