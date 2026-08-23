#!/bin/sh
#
# @file build.sh
# @date 2026-08-23
#
# @license{LicenseRef-AWH-1.0}
#
# @author Yuriy Lobarev
#
# @brief Сборка ворошителя кодека из исходных текстов, без собранной библиотеки
#
# @details Кодеки опираются на журнал работы фреймворка, а тот — на средства времени,
#          строк, кодировок и распределителя памяти. Договор сборки, перечислявший
#          лишь части кодека, с заведением журнала связываться перестал, и ворошитель
#          собрать стало нечем
#
# @warning Связывать ворошитель с «build/libawh.a» НЕЛЬЗЯ: части кодека собираются тут
#          же из свежих исходных текстов, а библиотека остаётся от прежней сборки, и
#          ворошитель выходит несогласованным. Отчитывается он при этом находкою, какая
#          точными настройками из её же отчёта не воспроизводится, — на розыск такой
#          мнимой находки уходит час. Оттого тело фреймворка берётся здесь исходными
#          текстами наравне с кодеком
#
# @note Перечень частей фреймворка держится ВРУЧНУЮ по образцу отдельных стендов
#       проверок: маскою его не снять, сборщик отбирает часть распределителя по системе
#
# @copyright Copyright © 2026
#
# Вызов:
#   tools/fuzz/build.sh <имя кодека> [корень дерева] [каталог сборки]
#
# Переменные окружения:
#   CXX    — собиратель, по умолчанию «c++»
#   FLAGS  — добавочные ключи сборки
#

# Прекращаем работу при первом же отказе
set -e

# Получаем имя кодека, ворошитель какого собирается
CODEC="$1"

# Проверяем, задано ли имя кодека
if [ -z "$CODEC" ]; then
	echo "Вызов: $0 <имя кодека> [корень дерева] [каталог сборки]" >&2
	exit 1
fi

# Получаем корень дерева исходных текстов
ROOT="${2:-$(cd "$(dirname "$0")/../.." && pwd)}"

# Получаем каталог сборки ворошителя
OUTPUT="${3:-/tmp/awh-fuzz}"

# Получаем собиратель
COMPILER="${CXX:-c++}"

# Проверяем наличие ворошителя запрошенного кодека
if [ ! -f "$ROOT/tools/fuzz/$CODEC.cpp" ]; then
	echo "Ворошителя кодека «$CODEC» нет: $ROOT/tools/fuzz/$CODEC.cpp" >&2
	exit 1
fi

# Собираем ключи сборки ворошителя
OPTIONS="-std=gnu++17 -O1 -g -fsanitize=address,undefined -I$ROOT/include $FLAGS"

# Выполняем заведение каталога сборки
mkdir -p "$OUTPUT"

# Выполняем снос прежде собранного ворошителя
#
# @note Снос обязателен: при отказе сборки прежний двоичный файл остаётся на месте
#       и прогон отчитывается успехом по коду, какого в нём уже нет
rm -f "$OUTPUT/$CODEC-fuzz" "$OUTPUT/$CODEC-fuzz.exe"

# Собираем перечень частей тела фреймворка, каких требует журнал работы
FRAMEWORK="src/sys/log.cpp src/sys/chrono.cpp src/sys/fmk.cpp src/net/nwt.cpp
	src/encoding/unicode/normalize.cpp src/encoding/unicode/table.cpp
	src/encoding/unicode/unicode.cpp src/encoding/unicode/utf8.cpp
	src/encoding/charset/charset.cpp src/encoding/charset/table.cpp
	src/alloc/alloc.cpp src/alloc/cache.cpp src/alloc/central.cpp
	src/alloc/classes.cpp src/alloc/guard.cpp src/alloc/huge.cpp
	src/alloc/link.cpp src/alloc/pages.cpp src/alloc/profile.cpp
	src/alloc/source.cpp src/alloc/spin.cpp src/alloc/trace.cpp
	src/alloc/capture/elf.cpp src/alloc/capture/mach.cpp src/alloc/capture/pe.cpp
	src/num/lexical/table.cpp"

##
# Внутренние имена распределителя libc берутся ТОЛЬКО под OpenBSD
#
# Файл «src/alloc/capture/obsd.cpp» собственной охраны по системе не несёт — её несёт
# сборщик: CMakeLists.txt подключает его лишь при OpenBSD. Собранный безусловно, он
# под MinGW валит связывание по «posix_memalign» и «aligned_alloc», каких у той
# библиотеки времени исполнения нет вовсе
##
if [ "$(uname -s)" = "OpenBSD" ]; then
	FRAMEWORK="$FRAMEWORK src/alloc/capture/obsd.cpp"
fi

##
# Системные библиотеки, каких требует тело фреймворка
#
# У MS Windows журнал зовёт «WSAGetLastError»: посредник «__awh_strerror__» разбирает
# сетевые коды отказов, каких «strerror» от MinGW не знает
##
case "$(uname -s)" in
	MINGW*|MSYS*|CYGWIN*) SYSTEM_LIBS="-lws2_32" ;;
	*) SYSTEM_LIBS="" ;;
esac

# Выводим сообщение о начале сборки
echo "Собираем ворошитель кодека $CODEC: $COMPILER"

# Заводим перечень объектных файлов ворошителя
OBJECTS=""

# Выполняем перебор всех частей тела фреймворка
for PART in $FRAMEWORK; do
	# Собираем имя объектного файла очередной части
	NAME="$(echo "$PART" | tr '/' '-')"
	##
	# Пересобираем часть лишь тогда, когда исходный текст её свежее объектного файла
	#
	# Тело фреймворка меняется не нами, и пересборка двадцати шести частей на каждый
	# заход ворошителя стоит дороже самого прогона
	##
	if [ ! -f "$OUTPUT/$NAME.o" ] || [ "$ROOT/$PART" -nt "$OUTPUT/$NAME.o" ]; then
		$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/$PART" -o "$OUTPUT/$NAME.o"
	fi
	# Добавляем собранное к перечню объектных файлов
	OBJECTS="$OBJECTS $OUTPUT/$NAME.o"
done

# Выполняем перебор всех частей кодека
#
# @note Части кодека пересобираются всегда: ворошителем поверяется именно они
for PART in "$ROOT/src/codec/$CODEC/"*.cpp; do
	# Собираем имя объектного файла очередной части
	NAME="codec-$(basename "$PART" .cpp)"
	# Выполняем сборку очередной части кодека
	$COMPILER $OPTIONS -c "$PART" -o "$OUTPUT/$NAME.o"
	# Добавляем собранное к перечню объектных файлов
	OBJECTS="$OBJECTS $OUTPUT/$NAME.o"
done

# Выполняем сборку самого ворошителя
$COMPILER $OPTIONS -c "$ROOT/tools/fuzz/$CODEC.cpp" -o "$OUTPUT/fuzz.o"

# Выполняем связывание ворошителя
$COMPILER $OPTIONS "$OUTPUT/fuzz.o" $OBJECTS $SYSTEM_LIBS -lz -o "$OUTPUT/$CODEC-fuzz"

# Выводим сообщение об окончании сборки
echo "Ворошитель собран: $OUTPUT/$CODEC-fuzz"
