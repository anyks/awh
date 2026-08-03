#!/usr/bin/env bash

##
# Сквозная проверка подмодуля HTTP/3
#
# Прогоняет всю цепочку проверок, накопленную вокруг модуля, и падает на первом
# расхождении. Собрана по образцу verify_http2.sh и по той же причине: проверок
# несколько, они лежат в разных местах, и прогонять их вручную после каждой правки
# — это десяток минут и три способа ошибиться. Самый частый способ: объектные файлы
# стендов собираются вручную, и после правки заголовков модуля они устаревают —
# санитайзер тогда указывает на несуществующий дефект. Скрипт пересобирает их всегда.
#

# Получаем корневую дирректорию репозитория
readonly ROOT=$(cd "$(dirname "$0")/.." && pwd)

# Каталог сборки библиотеки
readonly BUILD="${1:-$ROOT/build}"

# Каталог промежуточных файлов проверки
readonly WORK="${TMPDIR:-/tmp}/awh-verify-http3"

# Получаем версию OS
readonly OS=$(uname -a | awk '{print $1}')

# Каталог сборки эталонной реализации из подмодуля
readonly NGHTTP3_BUILD="$ROOT/build-nghttp3"

##
# Версия эталонной реализации, на которой снята документация
#
# Сверки с чужой реализацией сравнивают не с истиной, а с чужим прочтением RFC,
# и оно меняется от версии к версии. Поэтому версия сверяется, а не просто печатается
#
readonly NGHTTP3_EXPECTED="1.18.90"

##
# Ожидаемое количество известных расхождений в сверке обработки ошибок
#
# Расхождение не означает дефекта: оба известных случая — это места, где nghttp3
# отступает от буквы RFC, и наш парсер точнее. Но изменение их числа означает,
# что поведение сменилось — у нас либо у эталона, — и это обязано попасть человеку
# на глаза, а не пройти молча. Перечень ведётся в tools/interop/README.md
#
readonly NEGATIVE_EXPECTED=2

##
# Эталонная реализация берётся из подмодуля, а не системная
#
# Системная версия у каждой ОС своя, и расхождение в поведении сверок между
# машинами пришлось бы разбирать как дефект. Подмодуль зафиксирован в репозитории,
# поэтому сверки сравнивают с одним и тем же кодом везде
#
if [ ! -f "$NGHTTP3_BUILD/lib/libnghttp3.a" ]; then
	# Если исходные тексты подмодуля получены
	if [ -f "$ROOT/submodules/nghttp3/CMakeLists.txt" ]; then
		# Выводим сообщение о сборке эталонной реализации
		printf "\n\033[1m==> Сборка эталонной реализации из подмодуля\033[0m\n"
		##
		# У nghttp3 есть собственный вложенный подмодуль lib/sfparse: без него
		# сборка падает на отсутствующем заголовке, поэтому обновление рекурсивное
		#
		git -C "$ROOT" submodule update --init --recursive submodules/nghttp3 > /dev/null 2>&1
		# Выполняем конфигурацию сборки только библиотеки
		cmake -S "$ROOT/submodules/nghttp3" -B "$NGHTTP3_BUILD" -DCMAKE_BUILD_TYPE=Release \
			-DENABLE_LIB_ONLY=ON -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=OFF \
			-DBUILD_TESTING=OFF > "${TMPDIR:-/tmp}/awh-nghttp3-cmake.log" 2>&1 \
			|| { printf "не сконфигурирована сборка nghttp3\n"; exit 1; }
		# Выполняем сборку библиотеки
		cmake --build "$NGHTTP3_BUILD" -j 8 > "${TMPDIR:-/tmp}/awh-nghttp3-build.log" 2>&1 \
			|| { printf "не собрана библиотека nghttp3\n"; exit 1; }
	fi
fi

# Каталоги заголовочных файлов эталонной реализации
if [ -f "$NGHTTP3_BUILD/lib/libnghttp3.a" ]; then
	# Заголовочные файлы лежат в двух местах: исходные тексты и сгенерированный version.h
	readonly NGHTTP3_INCLUDE="-I$ROOT/submodules/nghttp3/lib/includes -I$NGHTTP3_BUILD/lib/includes"
	readonly NGHTTP3_LIBRARY="$NGHTTP3_BUILD/lib/libnghttp3.a"
else
	# Эталонная реализация недоступна
	readonly NGHTTP3_INCLUDE=""
	readonly NGHTTP3_LIBRARY=""
fi

# Флаги сборки стендов с санитайзерами
readonly FLAGS="-DAWH_STATICLIB -I$ROOT/contrib/include -I$ROOT/third_party/include \
	-I$ROOT/third_party/include/pcre2 -I$ROOT/include $NGHTTP3_INCLUDE \
	-pthread -std=gnu++17 -O1 -g -fsanitize=address,undefined -Wno-reserved-user-defined-literal"

# Системные библиотеки платформы
if [ "$OS" = "Darwin" ]; then
	readonly SYSTEM="-framework Foundation -framework CoreFoundation -framework Security"
else
	readonly SYSTEM=""
fi

# Библиотеки для сборки стендов
readonly LIBS="$ROOT/third_party/lib/libdependence.a $ROOT/third_party/lib/libcommon.a -lz $SYSTEM"

# Объектные файлы модуля, собранные с санитайзерами
readonly OBJECTS="$WORK/h3-http.o $WORK/h3-qpack.o $WORK/h3-frame.o $WORK/h3-h3.o"

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
# Пороги ловят не только скорость: степень сжатия сторожит использование
# динамической таблицы QPACK, а количество выделений памяти — переиспользование
# арены декодера и отдачу тела представлением без копирования
#
announce "Пороги набора бенчмарков"
cmake --build "$BUILD" --target awh_BENCHMARK_proto -j 8 > "$WORK/build-bench.log" 2>&1 \
	|| abort "не собран набор бенчмарков, подробности в $WORK/build-bench.log"
"$BUILD/benchmarks/proto" --filter=http3 > "$WORK/bench.log" 2>&1 \
	|| abort "показатели вышли за пороги: $(grep -E 'ХУЖЕ|ЛУЧШЕ' "$WORK/bench.log" | tr -s ' ' | cut -d' ' -f1 | tr '\n' ' ')"
accept "$(tail -1 "$WORK/bench.log")"

##
# Шаг 3. Сборка модуля с санитайзерами
#
# Объектные файлы пересобираются всегда: устаревшие указывают на несуществующие
# дефекты, если менялась разметка объектов модуля
#
announce "Сборка модуля с ASan и UBSan"
rm -f "$WORK"/h3-*.o
for NAME in http qpack frame h3; do
	c++ $FLAGS -c -o "$WORK/h3-$NAME.o" "$ROOT/src/proto/http/parser/http3/$NAME.cpp" 2> "$WORK/build-$NAME.log" \
		|| abort "не собран $NAME.cpp, подробности в $WORK/build-$NAME.log"
done
accept "четыре единицы трансляции модуля"

##
# Шаг 4. Сверки с эталонной реализацией nghttp3
#
# Получаем версию собранной эталонной реализации
NGHTTP3_VERSION=$(grep -hoE '"[0-9][0-9.]*"' "$NGHTTP3_BUILD/lib/includes/nghttp3/version.h" 2>/dev/null | head -1 | tr -d '"')
# Если версия не определена - помечаем её как отсутствующую
[ -n "$NGHTTP3_VERSION" ] || NGHTTP3_VERSION="нет"
announce "Сверки с эталонной реализацией nghttp3 $NGHTTP3_VERSION"
# Если версия эталона разошлась с той, на которой снята документация
if [ "$NGHTTP3_VERSION" != "$NGHTTP3_EXPECTED" ]; then
	# Выводим предупреждение о смене версии эталона
	printf "\033[33m  вним\033[0m  документация снята на %s: перечень известных расхождений\n" "$NGHTTP3_EXPECTED"
	printf "        в tools/interop/README.md подлежит пересмотру\n"
fi
if [ ! -f "$NGHTTP3_LIBRARY" ]; then
	# Выводим сообщение о пропуске шага
	printf "\033[33m  нет\033[0m  эталонная реализация недоступна: подтяните подмодуль \"git submodule update --init --recursive submodules/nghttp3\"\n"
else
	for NAME in qpack session negative; do
		c++ $FLAGS -c -o "$WORK/interop-$NAME.o" "$ROOT/tools/interop/nghttp3-$NAME.cpp" 2> "$WORK/build-interop-$NAME.log" \
			|| abort "не собрана сверка $NAME, подробности в $WORK/build-interop-$NAME.log"
		c++ -fsanitize=address,undefined -o "$WORK/interop-$NAME" "$WORK/interop-$NAME.o" \
			$OBJECTS "$BUILD/libawh.a" $LIBS "$NGHTTP3_LIBRARY" 2> "$WORK/link-interop-$NAME.log" \
			|| abort "не слинкована сверка $NAME, подробности в $WORK/link-interop-$NAME.log"
		"$WORK/interop-$NAME" > "$WORK/interop-$NAME.log" 2>&1 \
			|| abort "сверка $NAME не прошла, подробности в $WORK/interop-$NAME.log"
		accept "$NAME: $(grep -E 'расхождений|сверено' "$WORK/interop-$NAME.log" | tail -1)"
		# Если завершена сверка обработки некорректных потоков
		if [ "$NAME" = "negative" ]; then
			# Получаем количество обнаруженных расхождений
			FOUND=$(grep -oE 'расхождений: [0-9]+' "$WORK/interop-negative.log" | tail -1 | grep -oE '[0-9]+')
			# Если количество расхождений изменилось
			[ "$FOUND" = "$NEGATIVE_EXPECTED" ] || abort "известных расхождений было $NEGATIVE_EXPECTED, стало $FOUND: разберите каждое по tools/interop/README.md и обновите перечень"
		fi
	done
fi

##
# Шаг 5. Генератор искажённых потоков под санитайзерами
#
announce "Генератор искажённых потоков"
c++ $FLAGS -c -o "$WORK/h3-fuzz.o" "$ROOT/tools/fuzz/http3.cpp" 2> "$WORK/build-fuzz.log" \
	|| abort "не собран генератор, подробности в $WORK/build-fuzz.log"
c++ -fsanitize=address,undefined -o "$WORK/h3-fuzz" "$WORK/h3-fuzz.o" \
	$OBJECTS "$BUILD/libawh.a" $LIBS 2> "$WORK/link-fuzz.log" \
	|| abort "не слинкован генератор, подробности в $WORK/link-fuzz.log"
"$WORK/h3-fuzz" 4000 > "$WORK/fuzz.log" 2>&1 \
	|| abort "генератор нашёл дефект, подробности в $WORK/fuzz.log"
accept "$(tail -1 "$WORK/fuzz.log")"

# Выводим итог сквозной проверки
printf "\n\033[32mПРОВЕРКА ПРОЙДЕНА\033[0m: шагов %d, эталон nghttp3 %s, журналы в %s\n" "$STEPS" "$NGHTTP3_VERSION" "$WORK"
