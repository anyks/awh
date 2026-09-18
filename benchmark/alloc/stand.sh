#!/bin/sh
#
# @file stand.sh
# @date 2026-08-22
#
# @license{LicenseRef-AWH-1.0}
#
# @author Yuriy Lobarev
#
# @brief Отдельный стенд замеров распределителя памяти — сличение нашего с системным и
#        с соперниками, собранными из подмодулей AWH (jemalloc, gperftools)
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
##
##
# Соперники собираются ИЗ ПОДМОДУЛЕЙ AWH и только оттуда
#
# Решено владельцем: наш распределитель собирается из дерева, и соперник обязан
# собираться из того же дерева - из `submodules/`, рядом с зависимостями самого AWH.
# Библиотека из системы - это ЧУЖАЯ сборка: иная версия, иные ключи, - и сличение с нею
# мерило бы вдобавок разницу сборок. Прежде стенд перебирал системные каталоги и на
# Эльбрусе подхватил системный jemalloc (18.09.2026). Поиска по системе здесь нет вовсе:
# подмодуль не выложен либо не собрался - соперник пропускается с отчётом, без подмены
#
# Собирается КОПИЯ подмодуля в каталоге стенда: сам подмодуль остаётся нетронутым, а
# заплаты переноса (`sh/patches/<имя>`) ложатся на копию
##
RIVALS=""
# Число ядер для сборки соперников
JOBS=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)
##
# Сборщик GNU
#
# Makefile у jemalloc требует GNU make, а у систем BSD обычный `make` иной породы
##
MAKE=make
command -v gmake >/dev/null 2>&1 && MAKE=gmake
##
# Ключи связывания статической библиотеки ЦЕЛИКОМ
#
# Статическая библиотека отдаёт в программу лишь те части, на какие есть ссылки. У ELF
# этого хватило бы - `malloc` определён в самой библиотеке соперника. Но у macOS
# соперник встаёт на место зоны ОТКЛИКОМ ЗАГРУЗКИ, на какой не ссылается никто, и без
# ключа выпал бы из программы молча. Берём библиотеку целиком всюду, одним порядком
##
whole(){
	case "$(uname -s)" in
		Darwin) echo "-Wl,-force_load,$1" ;;
		SunOS) echo "-Wl,-z,allextract $1 -Wl,-z,defaultextract" ;;
		*) echo "-Wl,--whole-archive $1 -Wl,--no-whole-archive" ;;
	esac
}
##
# Метод сборки jemalloc из подмодуля
#
# Отвечает путём статической библиотеки в ARCHIVE; отказ - кодом возврата
##
build_jemalloc(){
	SRC="$ROOT/submodules/jemalloc"
	WORK="$OUT/rival-jemalloc"
	# Если подмодуль не выложен
	[ -f "$SRC/configure.ac" ] || { echo "подмодуль не выложен: git submodule update --init submodules/jemalloc"; return 1; }
	##
	# Сценарий настройки порождается, лишь если его ещё нет
	#
	# В подмодуле его нет: git хранит одно `configure.ac`. Но поклажа стенда везёт его
	# уже порождённым, и тогда autoconf на машине не нужен - на 18.09.2026 он стоял у пяти
	# стендов из четырнадцати. Порождённый сценарий переносим по самому своему назначению
	##
	[ -f "$SRC/configure" ] || command -v autoconf >/dev/null 2>&1 || { echo "нет autoconf"; return 1; }
	mkdir -p "$WORK" && cp -R "$SRC" "$WORK/src" && rm -f "$WORK/src/.git" || return 1
	##
	# Версию пишем сами: у копии подмодуля нет своей истории, и настройка без неё
	# сочинила бы 0.0.0
	#
	# Пишем её лишь при удаче: дерево, доставленное на стенд поклажей, истории не несёт
	# вовсе, и перенаправление в файл затёрло бы пустотой уже лежащий там VERSION.
	# Спрашиваем git лишь при `.git` в самом подмодуле: без него `git -C` поднялся бы к
	# объемлющему хранилищу и описал бы ЧУЖУЮ историю
	##
	VERSION=""
	[ -e "$SRC/.git" ] && VERSION=$(git -C "$SRC" describe --long --abbrev=40 2>/dev/null)
	[ -n "$VERSION" ] && echo "$VERSION" > "$WORK/src/VERSION"
	##
	# Наименьшее выравнивание задаётся ключом лишь там, где jemalloc его не знает
	#
	# У e2k перечень `quantum.h` отвечает `#error`; `max_align_t` там 16 байт, то есть
	# 2^4. Прочим системам ключ не нужен: соперник собирается так, как задумал сам
	##
	QUANTUM=""
	case "$(uname -m)" in e2k*) QUANTUM="--with-lg-quantum=4" ;; esac
	# Префикс имён пуст: соперник обязан отвечать за сам `malloc`, как и мы
	(cd "$WORK/src" && { [ -f configure ] || autoconf; } && ./configure --prefix="$WORK/root" --with-jemalloc-prefix= $QUANTUM && \
	 $MAKE -j"$JOBS" build_lib_static) > "$WORK/build.log" 2>&1 || { echo "сборка не удалась: $WORK/build.log"; tail -5 "$WORK/build.log"; return 1; }
	ARCHIVE="$WORK/src/lib/libjemalloc.a"
	[ -f "$ARCHIVE" ]
}
##
# Метод сборки tcmalloc (gperftools, часть minimal) из подмодуля
#
# Отвечает путём статической библиотеки в ARCHIVE; отказ - кодом возврата
##
build_tcmalloc(){
	SRC="$ROOT/submodules/gperftools"
	WORK="$OUT/rival-tcmalloc"
	# Если подмодуль не выложен
	[ -f "$SRC/CMakeLists.txt" ] || { echo "подмодуль не выложен: git submodule update --init submodules/gperftools"; return 1; }
	# Если нечем собрать
	command -v cmake >/dev/null 2>&1 || { echo "нет cmake"; return 1; }
	mkdir -p "$WORK" && cp -R "$SRC" "$WORK/src" && rm -f "$WORK/src/.git" || return 1
	##
	# Заплата переноса: gperftools не знает строки кэша у e2k и отвечает `#error`
	#
	# Заплата лишь добавляет ветвь `__e2k__` и прочим системам ничего не меняет
	##
	(cd "$WORK/src" && git apply "$ROOT/sh/patches/gperftools/gperftools.patch") > "$WORK/build.log" 2>&1 || { echo "заплата не легла: $WORK/build.log"; return 1; }
	(cmake -S "$WORK/src" -B "$WORK/build" -DCMAKE_BUILD_TYPE=Release -Dgperftools_build_minimal=ON \
	 -DBUILD_SHARED_LIBS=OFF -Dgperftools_build_benchmark=OFF -DBUILD_TESTING=OFF && \
	 cmake --build "$WORK/build" -j "$JOBS" --target tcmalloc_minimal) >> "$WORK/build.log" 2>&1 || { echo "сборка не удалась: $WORK/build.log"; tail -5 "$WORK/build.log"; return 1; }
	ARCHIVE="$WORK/build/libtcmalloc_minimal.a"
	##
	# Служебная часть gperftools лежит ОТДЕЛЬНОЙ библиотекой
	#
	# Сборка через cmake выносит спин-замки, журнал и разбор окружения в `libcommon.a`, и
	# статическому связыванию она нужна рядом с основной: без неё щуп учёта не связывался
	# вовсе (`SpinLock::SlowUnlock`, `TCMallocGetenvSafe` не определены, macOS 18.09.2026)
	##
	COMMON="$WORK/build/libcommon.a"
	[ -f "$ARCHIVE" ] && [ -f "$COMMON" ]
}
for rival in jemalloc tcmalloc_minimal; do
	ARCHIVE=""
	echo "--- соперник $rival: сборка из подмодуля"
	case "$rival" in
		jemalloc)
			SERVES="jemalloc"
			# Библиотеки, какие jemalloc зовёт сам
			LIBS="-lm -lpthread"
			[ "$(uname -s)" = "Linux" ] && LIBS="$LIBS -ldl"
			build_jemalloc || { echo "--- соперник $rival: НЕ СОБРАН - пропущен"; continue; }
		;;
		tcmalloc_minimal)
			SERVES="tcmalloc"
			build_tcmalloc || { echo "--- соперник $rival: НЕ СОБРАН - пропущен"; continue; }
			# Служебная часть gperftools и нити
			LIBS="$COMMON -lpthread"
		;;
	esac
	# Ключи связывания соперника
	LINK="$(whole "$ARCHIVE") $LIBS"
	##
	# Собраться мало - соперник обязан ДОКАЗАТЬ, что обслуживает выдачу
	#
	# Связывание само по себе не значит, что `malloc` достался сопернику: `malloc` и
	# `free` есть в самой libc, и связывание удаётся, даже когда часть соперника из
	# программы выпала. Так уже было: столбец соперника долго сличал нас с СИСТЕМНЫМ
	# распределителем под чужим именем (разобрано в COMPARISON.md, раздел «ОТЗЫВ»)
	#
	# Доказательством служит УЧЁТ самого соперника: берём мегабайт обычным `malloc` и
	# смотрим, вырос ли счёт занятого у него. `dladdr` и `malloc_zone_from_ptr` для этого
	# негодны - оба отвечали «соперник в стороне» и оба врали
	##
	##
	# Язык сбрасывается `-x none` сразу за исходником со стандартного ввода
	#
	# `-x c++` действует на ВСЕ входные файлы следом, и статическая библиотека, поданная
	# у ELF обычным входным файлом внутри `--whole-archive`, читалась бы как исходник C++:
	# щуп не собирался, и соперник выходил «не доказанным» (Эльбрус, 18.09.2026). У macOS
	# библиотека идёт доводом связывателя (`-force_load`), оттого там это не всплывало
	##
	$CXX -std=c++17 -O2 $FLAGS -D"AWH_RIVAL_$SERVES" -o "$OUT/serves-$rival" -x c++ - -x none $LINK > "$OUT/serves-$rival.log" 2>&1 <<-'SERVE'
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
	##
	# Недоказанного соперника НЕ БЕРЁМ вовсе
	#
	# Столбец недоказанного соперника хуже отсутствия столбца: он выглядит сличением, а
	# сличает нас неведомо с кем. Порог скорости по такому столбцу судить нельзя
	##
	if ! "$OUT/serves-$rival" > /dev/null 2>&1; then
		echo "--- соперник $rival: обслуживание выдачи НЕ ДОКАЗАНО - пропущен ($OUT/serves-$rival.log)"
		continue
	fi
	RIVALS="$RIVALS $rival"
	echo "--- соперник $rival: обслуживание выдачи ДОКАЗАНО учётом"
	echo "--- сборка на сопернике: $rival"
	$CXX -std=c++17 -O2 $NOBUILTIN $FLAGS -DAWH_BENCH_SYSTEM -DAWH_BENCH_RIVAL="\"$rival\"" -I "$ROOT/include" \
	 -o "$OUT/bench-$rival" "$ROOT/benchmark/alloc/stand.cpp" $LINK \
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
