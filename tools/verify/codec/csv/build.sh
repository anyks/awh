#!/usr/bin/env bash

# Получаем корневую дирректорию репозитория (каталог скрипта на четыре уровня ниже)
readonly ROOT=$(cd "$(dirname "$0")/../../../.." && pwd)

# Каталог стендов сверки
readonly STANDS="$ROOT/tools/verify/codec/csv"

# Каталог собранных стендов
readonly OUTPUT="${1:-/tmp/verify-csv}"

# Получаем собиратель
#
# @note Собиратель задаётся снаружи, а не пишется в скрипте: на стендах кластера его
#       штатное имя либо отсутствует, либо указывает не на тот тулчейн (Windows требует
#       «/mingw64/bin/g++», DragonFly - порта «g++14»)
COMPILER="${CXX:-c++}"

# Флаги сборки стендов
#
# Уровень оптимизации совпадает с уровнем сборки библиотеки в режиме Release:
# сверять реализации, собранные с разной оптимизацией, бессмысленно
FLAGS="-std=c++2a -O3 -DNDEBUG -Wall -Wextra"

# Путь к библиотеке стандартных средств того собирателя, каким собраны поверки
#
# @note Путь этот прописывается в двоичный файл: у DragonFly рядом стоят несколько
#       собирателей, и файл, собранный «g++14», при запуске подхватывал «libstdc++»
#       от gcc11 и отваливался с «version GLIBCXX_3.4.32 not found». Сборка при этом
#       зелёная, а отказ приходит от прогона, и причину пойдут искать в кодеке
#
# @warning Путь берётся лишь тогда, когда собиратель отдаёт его полным: «clang» на
#          выдачу этого вопроса отвечает одним лишь именем файла, и «dirname» от него
#          дал бы текущий каталог
#
# @note У целей MS Windows выдача эта негодна: библиотека там идёт отдельным файлом
#       рядом с двоичным, и «rpath» у формата PE не работает вовсе
#
case "$(uname -s)" in
	MINGW*|MSYS*|CYGWIN*) ;;
	*)
		STDLIB="$($COMPILER -print-file-name=libstdc++.so 2>/dev/null)"
		case "$STDLIB" in
			/*) FLAGS="$FLAGS -Wl,-rpath,$(cd "$(dirname "$STDLIB")" && pwd)" ;;
		esac
	;;
esac

# Набор стендов сверки
readonly PLAIN="dump"

# Ключи языка сборки слоя файловой системы: вне macOS тот собирается обычным C++
FS_FLAGS=""

#
# Слои системы, связыванию потребные
#
# @note Разряды эти повторяют девятнадцать стендовых скриптов проверок и замеров: те же
#       опоры, те же исключения. Разбирать их заново здесь нечего
#
case "$(uname -s)" in
	#
	# @note Разбор ярлыков в «src/sys/fs.cpp» поднимает COM: «CoCreateInstance» живёт в
	#       «ole32», опознаватели - в «uuid», а сетевой слой зовёт «GetIpForwardTable»
	#       из «iphlpapi». На одном «ws2_32» связывание отвечает отсутствием
	#       «__imp_CoInitialize» (замер 06.10.2026 на стенде Windows 11)
	#
	MINGW*|MSYS*|CYGWIN*) SYSTEM_LIBS="-lws2_32 -liphlpapi -lole32 -luuid" ;;
	#
	# Слой файловой системы у macOS написан на Objective-C++ и опирается на Foundation
	#
	# @note «src/sys/fs.cpp» зовёт там «NSFileManager», обычным C++ он не собирается
	#       вовсе, а без основы связывание отвечает отсутствием знака времени исполнения.
	#       Ровно так же поступает и CMakeLists.txt
	#
	Darwin)
		SYSTEM_LIBS="-framework Foundation"
		FS_FLAGS="-x objective-c++ -fobjc-arc"
	;;
	#
	# @note Слой процессов зовёт у FreeBSD «kinfo_getproc», и без «-lutil» связывание
	#       стенда отказывает (замер 06.10.2026 на стенде FreeBSD 19)
	#
	FreeBSD) SYSTEM_LIBS="-pthread -lutil" ;;
	#
	# @note У систем Sun сетевые знаки вынесены в «-lsocket» да «-lnsl»: без них
	#       связывание отвечает отказом на «getpeername» и «if_nametoindex»
	#       (замер 06.10.2026 на OpenIndiana)
	#
	SunOS) SYSTEM_LIBS="-lsocket -lnsl" ;;
	*) SYSTEM_LIBS="" ;;
esac

# Внутренние имена распределителя libc берутся ТОЛЬКО под OpenBSD
#
# @note Файл «src/alloc/capture/obsd.cpp» собственной охраны по системе не несёт - её
#       несёт сборщик: CMakeLists.txt подключает его лишь при OpenBSD. Собранный
#       безусловно, он под MinGW валит связывание по «posix_memalign» и
#       «aligned_alloc», каких у той библиотеки времени исполнения нет вовсе
OBSD=""
if [ "$(uname -s)" = "OpenBSD" ]; then
	OBSD="$ROOT/src/alloc/capture/obsd.cpp"
fi

# Выполняем создание каталога собранных стендов
mkdir -p "$OUTPUT" || exit 1

# Слой файловой системы собирается отдельно: общий язык для него один, а objective-c++
# в плоской команде достался бы все перечисленные источники
$COMPILER $FLAGS $FS_FLAGS -Wno-c++11-narrowing -I"$ROOT/include" -c "$ROOT/src/sys/fs.cpp" -o "$OUTPUT/sys-fs.o" || exit 1

# Выполняем перебор стендов сверки
for STAND in $PLAIN; do
	# Выводим сообщение о сборке стенда сверки
	echo "Building stand: $STAND"
	# Выполняем сборку стенда сверки
	$COMPILER $FLAGS \
		-Wno-c++11-narrowing \
		-I"$ROOT/include" \
		"$STANDS/$STAND.cpp" "$ROOT"/src/codec/csv/*.cpp "$ROOT"/src/num/lexical/*.cpp \
		"$ROOT"/src/sys/log.cpp "$ROOT"/src/sys/chrono.cpp "$ROOT"/src/sys/fmk.cpp \
		"$ROOT"/src/sys/os.cpp "$ROOT"/src/sys/signals.cpp "$ROOT"/src/sys/procre.cpp \
		"$ROOT"/src/net/addr.cpp "$ROOT"/src/net/net.cpp "$ROOT"/src/codec/numeric.cpp \
		"$OUTPUT/sys-fs.o" \
		"$ROOT"/src/net/nwt.cpp "$ROOT"/src/encoding/unicode/*.cpp "$ROOT"/src/encoding/charset/*.cpp \
		"$ROOT"/src/alloc/*.cpp "$ROOT"/src/alloc/capture/elf.cpp \
		"$ROOT"/src/alloc/capture/mach.cpp "$ROOT"/src/alloc/capture/pe.cpp $OBSD \
		-lz -pthread $SYSTEM_LIBS \
		-o "$OUTPUT/$STAND" || exit 1
done

# Выполняем составление корпуса таблиц для сверки
echo "Building corpus"
python3 "$STANDS/corpus.py" "$OUTPUT/corpus" || exit 1

# Выводим сообщение о завершении сборки стендов сверки
echo ""
echo "Stands are built in $OUTPUT"
echo "Run: python3 $STANDS/compare.py $OUTPUT/corpus $OUTPUT/dump"
