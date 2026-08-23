#!/usr/bin/env bash

# Получаем корневую дирректорию репозитория (каталог скрипта на три уровня ниже)
readonly ROOT=$(cd "$(dirname "$0")/../../.." && pwd)

# Каталог стендов сравнения
readonly STANDS="$ROOT/tools/benchmark/abc"

# Каталог исходных текстов сравниваемых реализаций
readonly VENDOR="$ROOT/submodules"

# Каталог собранных стендов
readonly OUTPUT="${1:-/tmp/rival-abc}"

# Каталог сборки библиотеки AWH с оптимизацией
readonly RELEASE="${2:-$ROOT/build-release}"

# Флаги сборки стендов
#
# Уровень оптимизации совпадает с уровнем сборки библиотеки в режиме Release:
# сравнивать реализации, собранные с разной оптимизацией, бессмысленно
readonly FLAGS="-O3 -DNDEBUG -std=c++17 -Wall -Wextra"

# Если исходные тексты сравниваемых реализаций не получены
for MODULE in libcbor msgpack-c; do
	# Если очередной подмодуль отсутствует
	if [ ! -d "$VENDOR/$MODULE" ]; then
		# Выводим сообщение об отсутствии подмодуля
		echo "Submodule \"$MODULE\" is missing in \"$VENDOR\""
		exit 1
	fi
done

# Если библиотека AWH с оптимизацией ещё не собрана
if [ ! -f "$RELEASE/libawh.a" ]; then
	# Выводим сообщение о необходимости сборки библиотеки
	echo "Build AWH in Release mode first: cmake -B \"$RELEASE\" -DCMAKE_BUILD_TYPE=Release && make -C \"$RELEASE\" awh"
	exit 1
fi

# Выполняем создание каталога собранных стендов
mkdir -p "$OUTPUT"

# Выводим сообщение о сборке стенда библиотеки AWH
echo "Build \"awh\""

# Выполняем сборку стенда библиотеки AWH
c++ $FLAGS -Wno-reserved-user-defined-literal -I"$ROOT/include" -I"$STANDS" \
	"$STANDS/awh.cpp" "$RELEASE/libawh.a" -o "$OUTPUT/awh" || exit 1

# Каталог сборки реализации libcbor
readonly CBOR="$OUTPUT/libcbor"

# Если реализация libcbor ещё не собрана
#
# Собирается она своим устройством сборки: заголовок настройки и заголовок вывода
# имён порождаются им, и собрать исходные тексты в обход него нельзя
if [ ! -f "$CBOR/src/libcbor.a" ]; then
	# Выводим сообщение о сборке реализации libcbor
	echo "Build \"libcbor\""

	# Выполняем сборку реализации libcbor
	cmake -S "$VENDOR/libcbor" -B "$CBOR" -DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_FLAGS="-O3 -DNDEBUG" -DWITH_TESTS=OFF -DWITH_EXAMPLES=OFF > /dev/null || exit 1
	cmake --build "$CBOR" --target cbor -j 4 > /dev/null || exit 1
fi

# Выводим сообщение о сборке стенда реализации libcbor
echo "Build \"cbor\""

# Выполняем сборку стенда реализации libcbor
c++ $FLAGS -I"$STANDS" -I"$VENDOR/libcbor/src" -I"$CBOR" -I"$CBOR/src" \
	"$STANDS/cbor.cpp" "$CBOR/src/libcbor.a" -o "$OUTPUT/cbor" || exit 1

# Каталог сборки реализации msgpack-c
#
# Предел вложенности разбора поднят со стандартных тридцати двух ярусов: образец с
# глубокой вложенностью глубже его, и умолчание сличало бы не скорость разбора, а
# настройку. Тем же чередом поднят он и у стенда сравнения контейнера JSON
readonly MSGPACK="$OUTPUT/msgpack-c"

##
# Исходные тексты msgpack-c берутся из ветви «c_master»
#
# Устройство проекта msgpack-c разведено по ветвям: у «master» лежат одни описания, а
# исходные тексты языка C - у «c_master». Подмодуль закреплён на «master», и сборка из
# закреплённого состояния валится отказом «msgpack.h: file not found»
#
# Ветвь разворачивается ОТДЕЛЬНЫМ рабочим деревом в каталоге сборки, а не переключением
# подмодуля: переключение сменило бы состояние дерева всем, кто с ним работает, и
# закрепление подмодуля перестало бы отвечать тому, что собрано
##
readonly MSGPACK_SOURCE="$OUTPUT/msgpack-src"

# Если исходные тексты msgpack-c ещё не развёрнуты
if [ ! -f "$MSGPACK_SOURCE/CMakeLists.txt" ]; then
	# Выводим сообщение о развёртывании исходных текстов
	echo "Checkout \"msgpack-c\" sources from \"c_master\""
	# Выполняем развёртывание ветви исходных текстов отдельным рабочим деревом
	git -C "$VENDOR/msgpack-c" worktree add --detach "$MSGPACK_SOURCE" origin/c_master > /dev/null 2>&1 || {
		# Выводим сообщение об отказе развёртывания
		echo "Unable to checkout \"c_master\" of \"msgpack-c\""
		exit 1
	}
fi

# Если реализация msgpack-c ещё не собрана
#
# Собирается она своим устройством сборки: заголовки `sysdep.h`, `pack_template.h` и
# запись выпуска порождаются им, и собрать исходные тексты в обход него нельзя
if [ ! -f "$MSGPACK/libmsgpack-c.a" ]; then
	# Выводим сообщение о сборке реализации msgpack-c
	echo "Build \"msgpack-c\""

	# Выполняем сборку реализации msgpack-c
	cmake -S "$MSGPACK_SOURCE" -B "$MSGPACK" -DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_FLAGS="-O3 -DNDEBUG -DMSGPACK_EMBED_STACK_SIZE=128" -DMSGPACK_BUILD_TESTS=OFF \
		-DMSGPACK_BUILD_EXAMPLES=OFF -DBUILD_SHARED_LIBS=OFF > /dev/null || exit 1
	cmake --build "$MSGPACK" -j 4 > /dev/null || exit 1
fi

# Выводим сообщение о сборке стенда реализации msgpack-c
echo "Build \"msgpack\""

# Выполняем сборку стенда реализации msgpack-c
c++ $FLAGS -DMSGPACK_EMBED_STACK_SIZE=128 -I"$STANDS" -I"$MSGPACK_SOURCE/include" \
	-I"$MSGPACK/include" -I"$MSGPACK/include/msgpack" \
	"$STANDS/msgpack.cpp" "$MSGPACK/libmsgpack-c.a" -o "$OUTPUT/msgpack" || exit 1

# Выводим сообщение об успешной сборке стендов
echo ""
echo "Stands are placed in \"$OUTPUT\""
