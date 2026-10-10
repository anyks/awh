#!/usr/bin/env bash

# Получаем корневую дирректорию репозитория (каталог скрипта на три уровня ниже)
readonly ROOT=$(cd "$(dirname "$0")/../../.." && pwd)

# Каталог стендов сравнения
readonly STANDS="$ROOT/tools/benchmark/regex"

# Каталог исходных текстов эталонной реализации PCRE2
readonly VENDOR="$ROOT/submodules/pcre2"

# Каталог собранных стендов
readonly OUTPUT="${1:-/tmp/rival-regex}"

# Каталог собранной эталонной реализации
readonly ORACLE="${2:-$OUTPUT/vendor}"

# Флаги сборки стендов
#
# Уровень оптимизации совпадает с уровнем сборки библиотеки в режиме Release:
# сравнивать реализации, собранные с разной оптимизацией, бессмысленно
readonly FLAGS="-std=c++2a -O3 -DNDEBUG -Wall -Wextra"

##
# Состав исходных текстов ведётся общим для всех стендов файлом
#
# Подключается «tools/regex/sources.sh», задающий общий состав набора:
# разбирать состав поиском в каждом стенде отдельно перечень устаревает молча.
# «stand_sources» отдаёт плоский список файлов, пригодный для связки
##

. "$(dirname "$0")/../../regex/sources.sh"

##
# У macOS модуль файловой системы тянет «Foundation»
#
# Без признака «-x objective-c++» собиратель натыкается на синтаксис Objective-C
# в Foundation и валит сборку разбором NSString/NSObject/... как незнакомых имён.
# Признак обязателен ПЕРЕД ключами сборки: ключ «-x» правит вид всех доводов
# последующих, и ссылка на исходник после него собирается по режиму Objective-C++.
# Связка с Foundation и pthread добавляется единым перечнем под платформу
##

LANGUAGE=""
RESTORE=""
LIBS=""
case "$(uname -s)" in
	Darwin)
		LANGUAGE="-x objective-c++ -fobjc-arc"
		RESTORE="-x none"
		LIBS="-framework Foundation -lpthread"
	;;
	FreeBSD) LIBS="-lpthread -lutil" ;;
	MINGW*|MSYS*|CYGWIN*) LIBS="-lws2_32 -lIphlpapi -lpsapi -ldbghelp -lcrypt32 -lbcrypt -lgdi32" ;;
	*) LIBS="-lpthread" ;;
esac

# Если исходные тексты эталонной реализации не получены
if [ ! -f "$VENDOR/CMakeLists.txt" ]; then
	# Выводим сообщение о необходимости получения исходных текстов
	echo "PCRE2 sources are missing, run:"
	echo "  git submodule update --init submodules/pcre2"
	exit 1
fi

# Признак доступности компиляции выражения в машинный код
JIT="ON"

# Если исходные тексты компилятора в машинный код не получены
if [ ! -f "$VENDOR/deps/sljit/sljit_src/sljitLir.c" ]; then
	# Выводим сообщение о недоступности компиляции в машинный код
	echo "PCRE2 JIT sources are missing, the rival will run interpreted."
	echo "Get them for a full-strength comparison:"
	echo "  git -C submodules/pcre2 submodule update --init deps/sljit"
	echo ""
	# Выполняем сброс признака доступности компиляции в машинный код
	JIT="OFF"
fi

# Выполняем создание каталога собранных стендов
mkdir -p "$OUTPUT" || exit 1

##
# Эталонная реализация PCRE2
#
# В поставку библиотеки эталон не входит и собирается местно, единственно ради
# сравнения. Компиляция выражения в машинный код включается намеренно: сравнивать
# реализацию с эталоном, работающим не в полную силу, бессмысленно
##

# Если эталонная реализация не собрана
if [ ! -f "$ORACLE/lib/libpcre2-8.a" ]; then
	# Выводим сообщение о сборке эталонной реализации
	echo "Building PCRE2 rival..."
	# Выполняем настройку сборки эталонной реализации
	cmake -S "$VENDOR" -B "$OUTPUT/vendor-build" \
		-DCMAKE_BUILD_TYPE=Release \
		-DPCRE2_BUILD_PCRE2_8=ON \
		-DPCRE2_SUPPORT_UNICODE=ON \
		-DPCRE2_SUPPORT_JIT=$JIT \
		-DPCRE2_BUILD_TESTS=OFF \
		-DPCRE2_BUILD_PCRE2GREP=OFF \
		-DBUILD_SHARED_LIBS=OFF \
		-DCMAKE_INSTALL_PREFIX="$ORACLE" > /dev/null || exit 1
	# Выполняем сборку эталонной реализации
	cmake --build "$OUTPUT/vendor-build" -j 8 > /dev/null || exit 1
	# Выполняем установку эталонной реализации
	cmake --install "$OUTPUT/vendor-build" > /dev/null || exit 1
fi

##
# Стенд сравнения модуля регулярных выражений библиотеки AWH
#
# Стенд собирается из исходных текстов модуля напрямую, минуя библиотеку:
# сравнение обязано мерить текущее состояние исходных текстов, а не состояние
# библиотеки, собранной когда-то ранее
#
# Состав берётся общим с прочими стендами через «tools/regex/sources.sh»:
# модуль опирается на таблицы Юникода (приведение регистра, свойства, разбор
# кластеров), на средства ядра фреймворка (журнал, диспетчер памяти) и на
# распределитель памяти — без них стенд не связывается вовсе
##

##
# Сборка ведётся в двух вариантах: штатном и без встроенного распределителя
#
# Штатный вариант «awh» собирается без макроса AWH_ALLOC_DISABLED: имена
# функций выдачи памяти в двоичном файле совпадают с именами libc, и система
# на системах семейства ELF подменяет libc-распределитель нашим связыванием.
# Вариант «awh.noalloc» собирается с признаком AWH_ALLOC_DISABLED: имена
# функций выдачи переименовываются в частные (__awh_alloc_malloc__ и т.п.),
# и системный распределитель остаётся штатным. Сравнение двух сборок одного
# текста даёт долю встроенного распределителя на каждый сценарий; сличение
# с PCRE2 идёт по варианту «awh.noalloc», равноценному сопернику по
# распределителю. Подробнее — Work/AI/protocols/awh/regex/ALLOCATOR.md
##

# Часть захвата имён у OpenBSD собирается только под OpenBSD
NATIVE=""
[ "$(uname -s)" = "OpenBSD" ] && NATIVE="$ROOT/src/alloc/capture/obsd.cpp"

# Состав исходных текстов стенда: общий модульный состав + драйвер стенда
SOURCES=$(stand_sources)

# Выводим сообщение о сборке стенда сравнения
echo "Building stand: awh (allocator enabled)"

# Выполняем сборку стенда сравнения со встроенным распределителем памяти
# shellcheck disable=SC2086
g++ $FLAGS $LANGUAGE \
	-I"$ROOT/include" \
	-o "$OUTPUT/awh" \
	"$STANDS/awh.cpp" $SOURCES $NATIVE $RESTORE $LIBS -lz || exit 1

# Выводим сообщение о сборке стенда сравнения
echo "Building stand: awh.noalloc (system allocator)"

# Выполняем сборку стенда сравнения без встроенного распределителя памяти:
# макрос AWH_ALLOC_DISABLED переименовывает имена функций выдачи в частные,
# и связывание с libc-распределителем не происходит
# shellcheck disable=SC2086
g++ $FLAGS -DAWH_ALLOC_DISABLED $LANGUAGE \
	-I"$ROOT/include" \
	-o "$OUTPUT/awh.noalloc" \
	"$STANDS/awh.cpp" $SOURCES $NATIVE $RESTORE $LIBS -lz || exit 1

##
# Стенд сравнения эталонной реализации PCRE2
##

# Выводим сообщение о сборке стенда сравнения
echo "Building stand: pcre2"

# Выполняем сборку стенда сравнения
# shellcheck disable=SC2086
g++ $FLAGS \
	-I"$ORACLE/include" \
	"$STANDS/pcre2.cpp" \
	-o "$OUTPUT/pcre2" \
	-L"$ORACLE/lib" -lpcre2-8 $LIBS || exit 1

# Выводим сообщение о завершении сборки стендов сравнения
echo ""
echo "Stands are built in $OUTPUT"
echo "Run them:"
echo "  $OUTPUT/awh             (with built-in allocator)"
echo "  $OUTPUT/awh.noalloc     (with system allocator)"
echo "  $OUTPUT/pcre2"
