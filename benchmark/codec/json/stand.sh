#!/bin/sh
#
# @file stand.sh
# @date 2026-08-19
#
# @license{LicenseRef-AWH-1.0}
#
# @author Yuriy Lobarev
#
# @brief Отдельный стенд замеров кодека JSON — сборка набора замеров без библиотеки
#        целиком, ради снятия показателей на отладочных стендах
#
# @details Пороги набора замеров назначаются по ДНУ КАЖДОГО сценария, а не по рабочей
#          машине: порог, снятый на быстрой, отказывает на медленной без всякой регрессии.
#
# @warning Прежняя редакция велела равняться на «самую медленную машину». Правило это
#          НЕВЕРНО и правится по замерам (05.09.2026): одной самой медленной машины не
#          существует - дно у разных сценариев приходится на разные машины. Пороги-
#          отношения от машины зависят тем более: отношение сокращает машину лишь
#          настолько, насколько обеим долям она одинаково дорога.
#          Полная же сборка библиотеки на стендах занимает десятки минут и тянет за
#          собою третью сторону, тогда как замерить требуется один кодек
#
# @note Каркас замеров опирается лишь на заголовок счётчика системных вызовов, а сам
#       счётчик разыскивается во время выполнения: без него набор печатает «нет
#       счётчика системных вызовов» и работает дальше
#
# @warning Замеры собираются только с оптимизацией: отладочная сборка замедляет всё
#          равномерно и изображает точечную регрессию там, где её нет
#
# @copyright Copyright © 2026
#
# Вызов:
#   benchmark/codec/json/stand.sh [корень дерева] [каталог сборки]
#
# Переменные окружения:
#   CXX   — собиратель, по умолчанию «c++»
#   FLAGS — добавочные ключи сборки
#

# Прекращаем работу при первом же отказе
set -e

# Получаем корень дерева исходных текстов
ROOT="${1:-$(cd "$(dirname "$0")/../../.." && pwd)}"

# Получаем каталог собранного стенда
OUTPUT="${2:-/tmp/awh-json-bench}"

# Получаем собиратель
COMPILER="${CXX:-c++}"

#
# Ключи сборки стенда
#
# @warning Оптимизация и `NDEBUG` обязательны ОБА, и порядок этот не косметический.
#          Замер 05.09.2026: стенд собирался с `-O2` и БЕЗ `-DNDEBUG`, тогда как пороги
#          сняты сборкой `-O3 -DNDEBUG` (так и записано в README набора). Итог на
#          OpenBSD amd64 разошёлся с прежним рядом ТОЙ ЖЕ машины в три-пять раз - запись
#          полей без кавычек 84 против 299 МБ/с, - и сценарий не уложился в порог. Ложь
#          эта опаснее отказа сборки: она приходит настоящими числами и читается как
#          регрессия кодека. Опознаётся тем, что просели ЛИШЬ сценарии, работающие с
#          памятью, а обход собранной таблицы и доступ по имени столбца остались теми же:
#          общее замедление машины село бы на все сценарии разом
#
OPTIONS="-O3 -DNDEBUG -std=c++17 -I$ROOT/include -I$ROOT/tools/benchmark/syscount $FLAGS"

#
# Выполняем прописывание пути к библиотеке стандартных средств собирателя
#
# @details Собранное одним собирателем при запуске подхватывает библиотеку стандартных
#          средств ИНОГО, если тот стоит в путях поиска раньше. У DragonFly это и
#          случается: код собирается `g++14` из портов, а при запуске подхватывается
#          `libstdc++` от `gcc11`, и двоичный файл валится с «version GLIBCXX_3.4.32
#          required ... not found». Сборка при этом проходит успешно, и отказ виден лишь
#          при запуске
#
# @warning Оговорка в `case` обязательна и молчаливую порчу предотвращает: `clang`
#          отвечает на тот же вопрос ОДНИМ ИМЕНЕМ файла без пути, и `dirname` от него дал
#          бы текущий каталог. Прописав его путём поиска, мы получили бы двоичный файл,
#          ищущий библиотеку рядом с собою, - то есть замену одной беды на другую, худшую
#
# @note Отказ этот молчалив по существу раскладки: машина отчитывается пройденной, а
#       показателей с неё ноль, и дно считается по одиннадцати машинам вместо двенадцати
#
STDLIB="$($COMPILER -print-file-name=libstdc++.so 2>/dev/null)"
case "$STDLIB" in
	/*) OPTIONS="$OPTIONS -Wl,-rpath,$(cd "$(dirname "$STDLIB")" && pwd)" ;;
esac

# Выполняем заведение каталога собранного стенда
mkdir -p "$OUTPUT"

# Выполняем снос прежде собранного стенда
#
# @note Снос обязателен: при отказе сборки прежний двоичный файл остаётся на месте
#       и прогон отчитывается успехом по коду, какого в нём уже нет
rm -f "$OUTPUT/json-bench" "$OUTPUT/json-bench.exe"

# Собираем перечень объектных файлов стенда
OBJECTS="$OUTPUT/lexical-table.o $OUTPUT/main.o $OUTPUT/sys-log.o $OUTPUT/sys-chrono.o $OUTPUT/sys-fmk.o $OUTPUT/net-nwt.o $OUTPUT/uni-normalize.o $OUTPUT/uni-table.o $OUTPUT/uni-unicode.o $OUTPUT/uni-utf8.o $OUTPUT/alloc-alloc.o $OUTPUT/alloc-cache.o $OUTPUT/alloc-central.o $OUTPUT/alloc-classes.o $OUTPUT/alloc-guard.o $OUTPUT/alloc-huge.o $OUTPUT/alloc-link.o $OUTPUT/alloc-pages.o $OUTPUT/alloc-profile.o $OUTPUT/alloc-source.o $OUTPUT/alloc-spin.o $OUTPUT/alloc-trace.o $OUTPUT/alloc-elf.o $OUTPUT/alloc-mach.o $OUTPUT/alloc-pe.o $OUTPUT/charset.o $OUTPUT/charset-table.o"

##
# Внутренние имена распределителя libc берутся ТОЛЬКО под OpenBSD
#
# Файл «src/alloc/capture/obsd.cpp» собственной охраны по системе не несёт - её
# несёт сборщик: CMakeLists.txt подключает его в перечень исходных текстов лишь
# при `CMAKE_SYSTEM_NAME STREQUAL "OpenBSD"`. Стенд обязан повторять этот отбор:
# собранный безусловно, он под MinGW валит связывание по `posix_memalign` и
# `aligned_alloc`, каких у той библиотеки времени исполнения нет вовсе
##
if [ "$(uname -s)" = "OpenBSD" ]; then
	OBJECTS="$OBJECTS $OUTPUT/alloc-obsd.o"
fi

##
# Системные библиотеки, каких требует ядро библиотеки
#
# У MS Windows журнал зовёт `WSAGetLastError`: посредник `__awh_strerror__` разбирает
# сетевые коды отказов, каких `strerror` от MinGW не знает. Живёт этот вызов в
# «ws2_32», и без неё связывание стенда отказывает
##
case "$(uname -s)" in
	MINGW*|MSYS*|CYGWIN*) SYSTEM_LIBS="-lws2_32" ;;
	#
	# @note Основа «Foundation» потребна слою файловой системы: «src/sys/fs.cpp» зовёт у
	#       macOS «NSFileManager», и без неё связывание отвечает отсутствием знаков
	#       Objective-C
	#
	Darwin) SYSTEM_LIBS="-framework Foundation" ;;
	SunOS) SYSTEM_LIBS="-lsocket -lnsl" ;;
	*) SYSTEM_LIBS="" ;;
esac

# Выводим сообщение о начале сборки стенда
echo "Собираем стенд замеров JSON: $COMPILER"

# Выполняем сборку таблицы степеней пятёрки модуля разбора чисел
$COMPILER $OPTIONS -c "$ROOT/src/num/lexical/table.cpp" -o "$OUTPUT/lexical-table.o"

#
# Выполняем сборку ведения журнала работы и опор его
#
# @note Кодек сообщает об отказах разбора и записи в журнал фреймворка, и стенд обязан
#       нести его с собою вместе со всем, на что он опирается
# @warning Ключ «-Wno-c++11-narrowing» приложен только к ЧУЖИМ файлам: свои им глушить
#          нельзя, сужение у себя обязано оставаться отказом сборки
#
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/log.cpp" -o "$OUTPUT/sys-log.o"
#
# Выполняем сборку слоя файловой системы и опознания системы
#
# @details Подмена целевого файла временным живёт ходом «fs_t::replaceAddress» с той
#          поры, как модуль «codec/replace» удалён владельцем. Кодеки зовут её сохранением
#          документа, и без этих частей связывание валится на «awh::Filesystem::Filesystem»
#
# @warning Под macOS «fs.cpp» собирается как Objective-C++, а не как C++: разбор
#          alias-файлов зовёт Foundation, и сборка обычным ходом валится сотнями отказов
#          в системных заголовках. Отбор этот повторяет CMakeLists.txt
#
if [ "$(uname -s)" = "Darwin" ]; then
	$COMPILER $OPTIONS -Wno-c++11-narrowing -x objective-c++ -fobjc-arc -c "$ROOT/src/sys/fs.cpp" -o "$OUTPUT/sys-fs.o"
else
	$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/fs.cpp" -o "$OUTPUT/sys-fs.o"
fi
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/os.cpp" -o "$OUTPUT/sys-os.o"
OBJECTS="$OBJECTS $OUTPUT/sys-fs.o $OUTPUT/sys-os.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/chrono.cpp" -o "$OUTPUT/sys-chrono.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/sys/fmk.cpp" -o "$OUTPUT/sys-fmk.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/net/nwt.cpp" -o "$OUTPUT/net-nwt.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/unicode/normalize.cpp" -o "$OUTPUT/uni-normalize.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/unicode/table.cpp" -o "$OUTPUT/uni-table.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/unicode/unicode.cpp" -o "$OUTPUT/uni-unicode.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/unicode/utf8.cpp" -o "$OUTPUT/uni-utf8.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/alloc.cpp" -o "$OUTPUT/alloc-alloc.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/cache.cpp" -o "$OUTPUT/alloc-cache.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/central.cpp" -o "$OUTPUT/alloc-central.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/classes.cpp" -o "$OUTPUT/alloc-classes.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/guard.cpp" -o "$OUTPUT/alloc-guard.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/huge.cpp" -o "$OUTPUT/alloc-huge.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/link.cpp" -o "$OUTPUT/alloc-link.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/pages.cpp" -o "$OUTPUT/alloc-pages.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/profile.cpp" -o "$OUTPUT/alloc-profile.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/source.cpp" -o "$OUTPUT/alloc-source.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/spin.cpp" -o "$OUTPUT/alloc-spin.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/trace.cpp" -o "$OUTPUT/alloc-trace.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/capture/elf.cpp" -o "$OUTPUT/alloc-elf.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/capture/mach.cpp" -o "$OUTPUT/alloc-mach.o"
if [ "$(uname -s)" = "OpenBSD" ]; then
	$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/capture/obsd.cpp" -o "$OUTPUT/alloc-obsd.o"
fi
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/alloc/capture/pe.cpp" -o "$OUTPUT/alloc-pe.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/charset/charset.cpp" -o "$OUTPUT/charset.o"
$COMPILER $OPTIONS -Wno-c++11-narrowing -c "$ROOT/src/encoding/charset/table.cpp" -o "$OUTPUT/charset-table.o"

# Выполняем сборку точки входа набора замеров
$COMPILER $OPTIONS -c "$ROOT/benchmark/main.cpp" -o "$OUTPUT/main.o"

# Выполняем сборку общего для кодеков извлечения числа
#
# @note Место это общее у всех кодеков рамки с 01.09.2026: разбор записи числа и
#       приведение его к затребованному виду держатся там, и без него стенд не
#       связывается - у кодека остаются несозданные тела шаблонов извлечения
$COMPILER $OPTIONS -c "$ROOT/src/codec/numeric.cpp" -o "$OUTPUT/codec-numeric.o"
OBJECTS="$OBJECTS $OUTPUT/codec-numeric.o"

#
# Собираем переносимую подмену файла, общую всем кодекам
#
# @details Лежит в «src/codec», а не в каталоге кодека, и перебором частей кодека
#          не берётся - как и «numeric.cpp», её нужно называть поимённо. У MS Windows
#          «rename» существующего файла не заменяет, потому сохранение через временный
#          файл ходит здесь, а не через вызов системы напрямую
#
# @note Часть «replace» изъята 15.09.2026: модуль «codec/replace» удалён владельцем как
#       нарушение договора, а подмена целевого файла временным ведётся ныне ходом
#       «fs_t::replaceAddress». Стенд валился сборкою на отсутствующем исходнике

# Выполняем перебор всех частей кодека JSON
for PART in common encoding reader writer document value; do
	# Выполняем сборку очередной части кодека JSON
	$COMPILER $OPTIONS -c "$ROOT/src/codec/json/$PART.cpp" -o "$OUTPUT/codec-$PART.o"
	# Добавляем собранное к перечню объектных файлов стенда
	OBJECTS="$OBJECTS $OUTPUT/codec-$PART.o"
done

# Выполняем перебор всех частей набора замеров кодека JSON
for PART in json reader writer document value; do
	# Выполняем сборку очередной части набора замеров
	$COMPILER $OPTIONS -c "$ROOT/benchmark/codec/json/$PART.cpp" -o "$OUTPUT/bench-$PART.o"
	# Добавляем собранное к перечню объектных файлов стенда
	OBJECTS="$OBJECTS $OUTPUT/bench-$PART.o"
done

# Выполняем связывание стенда замеров
#
# @note Объектные файлы перечисляются поимённо, а не маскою: посторонний объектный файл,
#       оставленный в каталоге сборки кем угодно, попадал бы в связывание и валил его
#       повтором имён
$COMPILER $OPTIONS $OBJECTS -pthread $SYSTEM_LIBS -lz -o "$OUTPUT/json-bench"

# Выводим сообщение об окончании сборки стенда
echo "Стенд собран: $OUTPUT/json-bench"
