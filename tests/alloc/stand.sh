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
##
# Надзор санитайзеров с самого распределителя снимаем
#
# Вопрос не о том, кто раздаёт память, а о том, какие файлы собираются с проверками
# надзирателя. Инструментировать модули распределителя нельзя по устройству: у систем
# ELF наш `malloc` подменяется связыванием, и первая просьба о памяти приходит из
# `_dl_init` - оттуда, где надзиратель ещё не завёл теневой области. Инструментированный
# вход валится об неё прежде первой строки `main`; не готов НАДЗИРАТЕЛЬ, а не мы.
#
# Сняв надзор с одних лишь этих файлов, получаем боевую связку: раздаёт память НАШ
# распределитель, а надзиратель следит за самим набором проверок. Гасить распределитель
# ключом AWH_ALLOC_DISABLED ради санитайзеров НЕ нужно - тот подменяет наш malloc
# системным и урезает охват молча
##
# Ключи сборки самого распределителя
ALLOCFLAGS="$FLAGS"
# Ключи связывания набора
LINKFLAGS=""
# Если набор собирается под санитайзерами
case "$FLAGS" in
	*-fsanitize=*)
		##
		# Признак сборки ПРОГРАММЫ под надзирателем ставим вручную
		#
		# Сняв инструментацию, мы гасим и встроенные признаки надзирателя, а модуль
		# судит по ним НЕ О ПРОВЕРКАХ, а о своём поведении: у macOS подмена имён под
		# надзирателем снимается сама, и замолчавший признак её оставлял бы
		##
		# Снимаем надзор санитайзеров с модулей распределителя
		ALLOCFLAGS="$FLAGS -fno-sanitize=all -DAWH_ALLOC_SANITIZER_BUILD"
		echo "--- надзор санитайзеров снят с модулей распределителя (сам он ВКЛЮЧЁН)"
		##
		# Среду надзирателя просим ДИНАМИЧЕСКУЮ
		#
		# Статическая среда надзирателя определяет malloc и родню В АРХИВЕ, и связыватель
		# видит два определения одного имени в двух объектных файлах: наше и её. Спор
		# этот неразрешим по устройству связывания, и снятие инструментации от него не
		# спасает - оно про проверки, а не про имена. Замерено на FreeBSD 14 (clang 19):
		# «ld: error: duplicate symbol: malloc», и так по всем шести именам выдачи.
		#
		# Динамическая же среда подменяется НАШИМИ метками при разрешении, как оно и
		# происходит у GNU/Linux, где она динамическая умолчанием. Путь к ней вписываем
		# в сам двоичный файл: иначе запуск требует переменной окружения, а её забудут
		##
		##
		# Ключ этот знает лишь clang
		#
		# Собиратель GNU отвечает на него «unrecognized command-line option», и притом
		# среда у него динамическая умолчанием - просить её незачем. Наличие же среды
		# clang в системе о собирателе НЕ говорит: у Debian нашлась она при сборке
		# собирателем GNU, и набор отказался собираться вовсе
		##
		if ! $CXX --version 2>&1 | grep -qi clang; then
			# Собирателю GNU ключ этот не нужен и неведом
			AWH_ASAN_SKIP=yes
		fi
		##
		# Путь к среде спрашиваем У САМОГО СОБИРАТЕЛЯ
		#
		# Он знает его точно, а перебор по образцу гадает. Спрашивать надо именно
		# `-print-file-name`: соседний `-print-runtime-dir` у FreeBSD 14 называет
		# каталог `.../lib/x86_64-unknown-freebsd15.1`, какого не существует вовсе, а
		# файл лежит в `.../lib/freebsd` (замер 15.09.2026). Перебор по образцу
		# остаётся запасным путём - на случай собирателя, такого вопроса не знающего
		##
		AWH_ASAN_ASKED="$($CXX -print-file-name=libclang_rt.asan-x86_64.so 2>/dev/null)"
		for AWH_ASAN_LIB in "$AWH_ASAN_ASKED" /usr/lib/clang/*/lib/*/libclang_rt.asan-*.so; do
			# Если динамическая среда надзирателя найдена, а собиратель её понимает
			if [ -f "$AWH_ASAN_LIB" ] && [ -z "$AWH_ASAN_SKIP" ]; then
				# Просим динамическую среду и вписываем путь к ней
				LINKFLAGS="-shared-libasan -Wl,-rpath,$(dirname "$AWH_ASAN_LIB")"
				# Сообщаем о найденной динамической среде надзирателя
				echo "--- среда надзирателя взята динамической: $(dirname "$AWH_ASAN_LIB")"
				# Больше искать нечего
				break
			fi
		done
	;;
esac
echo "--- сборка модулей распределителя"
##
# Модули распределителя собираются ОТДЕЛЬНО от набора
#
# Иначе ключей их сборки не развести вовсе: собиратель, позванный единожды, даёт одни и
# те же ключи всякому поданному файлу
##
for AWH_ALLOC_SOURCE in \
 "$ROOT/src/alloc/alloc.cpp" "$ROOT/src/alloc/source.cpp" "$ROOT/src/alloc/pages.cpp" \
 "$ROOT/src/alloc/classes.cpp" "$ROOT/src/alloc/spin.cpp" "$ROOT/src/alloc/link.cpp" \
 "$ROOT/src/alloc/central.cpp" "$ROOT/src/alloc/cache.cpp" "$ROOT/src/alloc/guard.cpp" \
 "$ROOT/src/alloc/huge.cpp" "$ROOT/src/alloc/trace.cpp" "$ROOT/src/alloc/profile.cpp" \
 "$ROOT/src/alloc/vessel.cpp" "$ROOT/src/alloc/prompt.cpp" "$ROOT/src/alloc/scatter.cpp" "$CAPTURE"; do
	# Собираем очередной модуль распределителя
	$CXX -std=c++17 -O2 $NOBUILTIN $ALLOCFLAGS -I "$ROOT/include" \
	 -c "$AWH_ALLOC_SOURCE" -o "$OUT/$(basename "$AWH_ALLOC_SOURCE" .cpp).o" >> "$OUT/build.log" 2>&1 || {
		# Сообщаем об отказе сборки модуля распределителя
		echo "ОТКАЗ СБОРКИ: $AWH_ALLOC_SOURCE"
		# Печатаем вывод собирателя
		head -40 "$OUT/build.log"
		# Выходим с признаком отказа
		exit 1
	}
done
echo "--- сборка набора"
$CXX -std=c++17 -O2 $NOBUILTIN $FLAGS $GTESTFLAGS -I "$ROOT/include" -o "$OUT/alloc" \
 "$ROOT/tests/alloc/stand.cpp" \
 "$ROOT/tests/alloc/capture.cpp" "$ROOT/tests/alloc/mangle.cpp" \
 "$ROOT/tests/alloc/contract.cpp" "$ROOT/tests/alloc/guard.cpp" \
 "$ROOT/tests/alloc/huge.cpp" "$ROOT/tests/alloc/zone.cpp" \
 "$ROOT/tests/alloc/purge.cpp" "$ROOT/tests/alloc/profile.cpp" \
 "$ROOT/tests/alloc/fork.cpp" "$ROOT/tests/alloc/vessel.cpp" \
 "$ROOT/tests/alloc/scatter.cpp" \
 "$OUT"/*.o $LINKFLAGS -lgtest $GMOCK -lpthread >> "$OUT/build.log" 2>&1
##
# Признак случайной раскладки снимаем у NetBSD
#
# Надзиратель там с нею несовместим, и прогон молчит ОПАСНОГО ВИДА: «This sanitizer is
# not compatible with enabled ASLR», а код возврата НУЛЕВОЙ. Раскладка, судящая по коду,
# засчитала бы такую машину прошедшей, не прогнав ни единой проверки (замер
# 15.09.2026).
#
# Снимается признак с ГОТОВОГО двоичного файла и строго в два шага: сперва `-0`, затем
# `+a`. Прямое `+a` отвечает «New flags 0x30 don't make sense». Зовётся полным путём: в
# путях поиска обычного пользователя средства этого нет
##
if [ "$(uname -s)" = "NetBSD" ] && [ -n "$LINKFLAGS$ALLOCFLAGS" ]; then
	# Если набор собран под надзирателями
	case "$FLAGS" in
		*-fsanitize=*)
			# Если средство смены признаков образа на месте
			if [ -x /usr/sbin/paxctl ]; then
				# Снимаем прежние признаки образа
				/usr/sbin/paxctl -0 "$OUT/alloc" > /dev/null 2>&1
				# Разрешаем образу случайную раскладку не применять
				/usr/sbin/paxctl +a "$OUT/alloc" > /dev/null 2>&1
				# Сообщаем о снятом признаке случайной раскладки
				echo "--- признак случайной раскладки снят: надзиратель с нею несовместим"
			else
				# Сообщаем об отсутствии средства смены признаков образа
				echo "--- ВНИМАНИЕ: /usr/sbin/paxctl не найден, надзиратель промолчит при нулевом коде"
			fi
		;;
	esac
fi

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
