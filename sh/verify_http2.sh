#!/usr/bin/env bash

##
# Сквозная проверка подмодуля HTTP/2
#
# Прогоняет всю цепочку проверок, накопленную вокруг модуля, и падает на первом
# расхождении. Собрана потому, что проверок пять, они лежат в разных местах,
# и прогонять их вручную после каждой правки — это десяток минут и три способа
# ошибиться. Самый частый способ: объектные файлы стендов собираются вручную,
# и после правки заголовков модуля они устаревают — санитайзер тогда указывает
# на несуществующий дефект. Скрипт пересобирает их всегда.
#

# Получаем корневую дирректорию репозитория
readonly ROOT=$(cd "$(dirname "$0")/.." && pwd)

# Каталог сборки библиотеки
readonly BUILD="${1:-$ROOT/build}"

# Каталог промежуточных файлов проверки
readonly WORK="${TMPDIR:-/tmp}/awh-verify-http2"

# Получаем версию OS
readonly OS=$(uname -a | awk '{print $1}')

# Каталог заголовочных файлов системных библиотек
if [ "$OS" = "Darwin" ]; then
	readonly PREFIX="/opt/homebrew"
else
	readonly PREFIX="/usr/local"
fi

# Каталог заголовочных файлов эталонной реализации
if [ -d "$PREFIX/opt/libnghttp2/include" ]; then
	readonly NGHTTP2_INCLUDE="$PREFIX/opt/libnghttp2/include"
	readonly NGHTTP2_LIBRARY="$PREFIX/opt/libnghttp2/lib/libnghttp2.a"
else
	readonly NGHTTP2_INCLUDE="$PREFIX/include"
	readonly NGHTTP2_LIBRARY="$PREFIX/lib/libnghttp2.a"
fi

# Флаги сборки стендов с санитайзерами
readonly FLAGS="-DAWH_IDN -DAWH_STATICLIB -I$ROOT/contrib/include -I$ROOT/third_party/include \
	-I$ROOT/third_party/include/pcre2 -I$ROOT/include -I$NGHTTP2_INCLUDE \
	-pthread -std=gnu++17 -O1 -g -fsanitize=address,undefined -Wno-reserved-user-defined-literal"

# Системные библиотеки платформы
if [ "$OS" = "Darwin" ]; then
	readonly SYSTEM="-framework Foundation -framework CoreFoundation -framework Security"
else
	readonly SYSTEM=""
fi

# Библиотеки для сборки стендов
readonly LIBS="$ROOT/third_party/lib/libdependence.a $ROOT/third_party/lib/libcommon.a -lz -liconv $SYSTEM"

# Количество пройденных шагов проверки
STEPS=0

##
# Функция вывода заголовка шага проверки
#
# @param $1 название шага проверки
#
announce(){
	# Выводим заголовок шага проверки
	printf "\n\033[1m==> %s\033[0m\n" "$1"
}

##
# Функция завершения проверки с ошибкой
#
# @param $1 описание расхождения
#
abort(){
	# Выводим описание расхождения
	printf "\n\033[31mПРОВЕРКА НЕ ПРОЙДЕНА: %s\033[0m\n" "$1"
	# Завершаем проверку с ненулевым кодом
	exit 1
}

##
# Функция учёта пройденного шага проверки
#
# @param $1 итог шага проверки
#
accept(){
	# Считаем пройденный шаг проверки
	STEPS=$((STEPS + 1))
	# Выводим итог шага проверки
	printf "\033[32m  ок\033[0m  %s\n" "$1"
}

# Выполняем создание каталога промежуточных файлов
mkdir -p "$WORK" || abort "не создан каталог $WORK"

##
# Шаг 1. Модульные тесты
#
announce "Модульные тесты"
cmake --build "$BUILD" --target awh_UNITTEST_proto -j 8 > "$WORK/build-tests.log" 2>&1 \
	|| abort "не собраны модульные тесты, подробности в $WORK/build-tests.log"
"$BUILD/unit-tests/proto" > "$WORK/tests.log" 2>&1 \
	|| abort "модульные тесты не прошли, подробности в $WORK/tests.log"
accept "$(grep -E '^\[  PASSED  \]' "$WORK/tests.log" | tail -1)"

##
# Шаг 2. Пороги набора бенчмарков
#
# Пороги ловят не только скорость: степень сжатия сторожит адаптивную индексацию,
# а количество выделений памяти — переиспользование арены декодера
#
announce "Пороги набора бенчмарков"
cmake --build "$BUILD" --target awh_BENCHMARK_proto -j 8 > "$WORK/build-bench.log" 2>&1 \
	|| abort "не собран набор бенчмарков, подробности в $WORK/build-bench.log"
"$BUILD/benchmarks/proto" --filter=http2 > "$WORK/bench.log" 2>&1 \
	|| abort "показатели вышли за пороги: $(grep -E 'ХУЖЕ|ЛУЧШЕ' "$WORK/bench.log" | tr -s ' ' | cut -d' ' -f1 | tr '\n' ' ')"
accept "$(tail -1 "$WORK/bench.log")"

##
# Шаг 3. Сборка модуля с санитайзерами
#
# Объектные файлы пересобираются всегда: устаревшие указывают на несуществующие
# дефекты, если менялась разметка объектов модуля
#
announce "Сборка модуля с ASan и UBSan"
rm -f "$WORK"/h2-*.o
for NAME in http hpack frame h2; do
	c++ $FLAGS -c -o "$WORK/h2-$NAME.o" "$ROOT/src/proto/http/parser/http2/$NAME.cpp" 2> "$WORK/build-$NAME.log" \
		|| abort "не собран $NAME.cpp, подробности в $WORK/build-$NAME.log"
done
accept "четыре единицы трансляции модуля"

##
# Шаг 4. Сверки с эталонной реализацией nghttp2
#
announce "Сверки с эталонной реализацией"
if [ ! -f "$NGHTTP2_LIBRARY" ]; then
	# Выводим сообщение о пропуске шага
	printf "\033[33m  нет\033[0m  nghttp2 не установлена, сверки пропущены (brew install libnghttp2)\n"
else
	for NAME in hpack server client negative extensions; do
		c++ $FLAGS -c -o "$WORK/interop-$NAME.o" "$ROOT/tools/interop/nghttp2-$NAME.cpp" 2> "$WORK/build-interop-$NAME.log" \
			|| abort "не собрана сверка $NAME, подробности в $WORK/build-interop-$NAME.log"
		c++ -fsanitize=address,undefined -o "$WORK/interop-$NAME" "$WORK/interop-$NAME.o" \
			"$WORK/h2-http.o" "$WORK/h2-hpack.o" "$WORK/h2-frame.o" "$WORK/h2-h2.o" \
			"$BUILD/libawh.a" $LIBS "$NGHTTP2_LIBRARY" 2> "$WORK/link-interop-$NAME.log" \
			|| abort "не слинкована сверка $NAME, подробности в $WORK/link-interop-$NAME.log"
		"$WORK/interop-$NAME" > "$WORK/interop-$NAME.log" 2>&1 \
			|| abort "сверка $NAME не прошла, подробности в $WORK/interop-$NAME.log"
		accept "$NAME: $(grep -E 'расхождений|сверено' "$WORK/interop-$NAME.log" | tail -1)"
	done
fi

##
# Шаг 5. Набор конформных проверок h2spec
#
announce "Конформность h2spec"
if ! command -v h2spec > /dev/null 2>&1; then
	# Выводим сообщение о пропуске шага
	printf "\033[33m  нет\033[0m  h2spec не установлен, конформность не проверена\n"
else
	c++ $FLAGS -c -o "$WORK/h2spec-server.o" "$ROOT/tools/interop/h2spec-server.cpp" 2> "$WORK/build-h2spec.log" \
		|| abort "не собран сервер h2spec, подробности в $WORK/build-h2spec.log"
	c++ -fsanitize=address,undefined -o "$WORK/h2spec-server" "$WORK/h2spec-server.o" \
		"$WORK/h2-http.o" "$WORK/h2-hpack.o" "$WORK/h2-frame.o" "$WORK/h2-h2.o" \
		"$BUILD/libawh.a" $LIBS 2> "$WORK/link-h2spec.log" \
		|| abort "не слинкован сервер h2spec, подробности в $WORK/link-h2spec.log"
	# Поднимаем сервер на свободном порту
	"$WORK/h2spec-server" 18080 > "$WORK/h2spec-server.log" 2>&1 &
	# Запоминаем идентификатор процесса сервера
	SERVER=$!
	# Ждём готовности сервера принимать соединения
	sleep 2
	# Выполняем прогон набора конформных проверок
	h2spec -h 127.0.0.1 -p 18080 -o 3 > "$WORK/h2spec.log" 2>&1
	# Запоминаем итог прогона
	OUTCOME=$?
	# Останавливаем сервер
	kill "$SERVER" 2> /dev/null
	# Если набор конформных проверок не пройден
	[ $OUTCOME -eq 0 ] || abort "конформность не подтверждена: $(grep -E '^[0-9]+ tests' "$WORK/h2spec.log")"
	accept "$(grep -E '^[0-9]+ tests' "$WORK/h2spec.log")"
fi

##
# Шаг 6. Генератор искажённых потоков под санитайзерами
#
announce "Генератор искажённых потоков"
c++ $FLAGS -c -o "$WORK/h2-fuzz.o" "$ROOT/tools/fuzz/http2.cpp" 2> "$WORK/build-fuzz.log" \
	|| abort "не собран генератор, подробности в $WORK/build-fuzz.log"
c++ -fsanitize=address,undefined -o "$WORK/h2-fuzz" "$WORK/h2-fuzz.o" \
	"$WORK/h2-http.o" "$WORK/h2-hpack.o" "$WORK/h2-frame.o" "$WORK/h2-h2.o" \
	"$BUILD/libawh.a" $LIBS 2> "$WORK/link-fuzz.log" \
	|| abort "не слинкован генератор, подробности в $WORK/link-fuzz.log"
"$WORK/h2-fuzz" 4000 > "$WORK/fuzz.log" 2>&1 \
	|| abort "генератор нашёл дефект, подробности в $WORK/fuzz.log"
accept "$(tail -1 "$WORK/fuzz.log")"

# Выводим итог сквозной проверки
printf "\n\033[32mПРОВЕРКА ПРОЙДЕНА\033[0m: шагов %d, журналы в %s\n" "$STEPS" "$WORK"
