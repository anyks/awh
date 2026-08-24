#!/usr/bin/env bash

# Получаем корневую дирректорию репозитория (каталог скрипта на три уровня ниже)
readonly ROOT=$(cd "$(dirname "$0")/../../.." && pwd)

# Каталог стендов сравнения
readonly STANDS="$ROOT/tools/benchmark/toml"

# Каталог исходных текстов сравниваемых реализаций
#
# Соперники собираются из подмодулей, лежащих рядом с зависимостями самого AWH:
# прежде четверо их забирались с сети одиночными заголовочными файлами средством
# fetch.sh, отчего сличение мерило ещё и разницу сборок
readonly VENDOR="$ROOT/submodules"

# Каталог собранных стендов
readonly OUTPUT="${1:-/tmp/rival-toml}"

# Каталог сборки библиотеки AWH с оптимизацией
readonly RELEASE="${2:-$ROOT/build-release}"

# Флаги сборки стендов
#
# Уровень оптимизации совпадает с уровнем сборки библиотеки в режиме Release:
# сравнивать реализации, собранные с разной оптимизацией, бессмысленно
readonly FLAGS="-O3 -DNDEBUG -std=c++17 -Wall -Wextra"

# Флаги сборки сравниваемых реализаций, писанных на языке C
#
# Реализация tomlc99 писана на C и собирается им же: сборка её компилятором C++
# меняет и правила приведения типов, и способ связывания, а сравнивать надо то,
# что получает её обычный потребитель
readonly CFLAGS="-O3 -DNDEBUG -std=c11 -w"

# Если библиотека AWH с оптимизацией ещё не собрана
if [ ! -f "$RELEASE/libawh.a" ]; then
	# Выводим сообщение об отсутствии собранной библиотеки
	echo "Error: \"$RELEASE/libawh.a\" not found"
	# Выводим указание к сборке библиотеки
	echo "Build it: cmake -B \"$RELEASE\" -DCMAKE_BUILD_TYPE=Release \"$ROOT\" && make -C \"$RELEASE\" awh"
	# Выходим с ошибкой
	exit 1
fi

# Выполняем создание каталога собранных стендов
mkdir -p "$OUTPUT"

##
# Сличаем эталонные тексты стенда с текстами собственного набора замеров
#
# Описание стенда утверждает, что тексты эти совпадают, и что расхождение хотя бы в одном
# из них обесценивает отчёт целиком. Утверждение это держалось одним лишь словом: тексты
# собираются двумя телами в двух местах, и правка одного из них расхождения ничем не
# выдавала - отчёт выходил правдоподобным и негодным. Теперь слово поверяется сборкою
#
# Сличаются сами собранные тексты, а не исходные их построения: сличение текстов исходных
# спотыкается об имена переменных да пространства имён, расхождением не являющиеся вовсе
##
##
# Сличаем количества прогонов стенда с количествами собственного набора замеров
#
# Постоянные эти писаны в двух местах числом, и расхождение их обесценивает отчёт
# наравне с расхождением самих текстов: сличались бы разные объёмы работы. Сличение
# идёт по имени постоянной, а не по месту её в файле
##
# Выводим сообщение о сличении количеств прогонов
echo "Check \"rounds\""
# Сличаем количество прогонов «SMALL_ROUNDS»
STAND_SMALL_ROUNDS="$(sed -n 's/.*static constexpr size_t SMALL_ROUNDS = \([0-9]*\);.*/\1/p' "$STANDS/common.hpp")"
SUITE_SMALL_ROUNDS="$(sed -n 's/.*static constexpr size_t SMALL_ROUNDS = \([0-9]*\);.*/\1/p' "$ROOT/benchmark/codec/toml/reader.cpp")"
if [ -z "$STAND_SMALL_ROUNDS" ] || [ -z "$SUITE_SMALL_ROUNDS" ]; then
	echo "Error: количество прогонов «SMALL_ROUNDS» не найдено (стенд «$STAND_SMALL_ROUNDS», набор «$SUITE_SMALL_ROUNDS»)" >&2
	exit 1
fi
if [ "$STAND_SMALL_ROUNDS" != "$SUITE_SMALL_ROUNDS" ]; then
	echo "Error: количество прогонов «SMALL_ROUNDS» расходится: стенд $STAND_SMALL_ROUNDS, набор $SUITE_SMALL_ROUNDS" >&2
	exit 1
fi
# Сличаем количество прогонов «LARGE_ROUNDS»
STAND_LARGE_ROUNDS="$(sed -n 's/.*static constexpr size_t LARGE_ROUNDS = \([0-9]*\);.*/\1/p' "$STANDS/common.hpp")"
SUITE_LARGE_ROUNDS="$(sed -n 's/.*static constexpr size_t LARGE_ROUNDS = \([0-9]*\);.*/\1/p' "$ROOT/benchmark/codec/toml/reader.cpp")"
if [ -z "$STAND_LARGE_ROUNDS" ] || [ -z "$SUITE_LARGE_ROUNDS" ]; then
	echo "Error: количество прогонов «LARGE_ROUNDS» не найдено (стенд «$STAND_LARGE_ROUNDS», набор «$SUITE_LARGE_ROUNDS»)" >&2
	exit 1
fi
if [ "$STAND_LARGE_ROUNDS" != "$SUITE_LARGE_ROUNDS" ]; then
	echo "Error: количество прогонов «LARGE_ROUNDS» расходится: стенд $STAND_LARGE_ROUNDS, набор $SUITE_LARGE_ROUNDS" >&2
	exit 1
fi
# Сличаем количество прогонов «FOCUSED_ROUNDS»
STAND_FOCUSED_ROUNDS="$(sed -n 's/.*static constexpr size_t FOCUSED_ROUNDS = \([0-9]*\);.*/\1/p' "$STANDS/common.hpp")"
SUITE_FOCUSED_ROUNDS="$(sed -n 's/.*static constexpr size_t FOCUSED_ROUNDS = \([0-9]*\);.*/\1/p' "$ROOT/benchmark/codec/toml/reader.cpp")"
if [ -z "$STAND_FOCUSED_ROUNDS" ] || [ -z "$SUITE_FOCUSED_ROUNDS" ]; then
	echo "Error: количество прогонов «FOCUSED_ROUNDS» не найдено (стенд «$STAND_FOCUSED_ROUNDS», набор «$SUITE_FOCUSED_ROUNDS»)" >&2
	exit 1
fi
if [ "$STAND_FOCUSED_ROUNDS" != "$SUITE_FOCUSED_ROUNDS" ]; then
	echo "Error: количество прогонов «FOCUSED_ROUNDS» расходится: стенд $STAND_FOCUSED_ROUNDS, набор $SUITE_FOCUSED_ROUNDS" >&2
	exit 1
fi

# Выводим сообщение о сличении эталонных текстов
echo "Check \"corpus\""
# Выполняем сборку сличения эталонных текстов
c++ $FLAGS -I"$ROOT/include" -I"$STANDS" -I"$ROOT/benchmark/codec/toml" -I"$ROOT/benchmark" \
 -I"$ROOT/tools/benchmark/syscount" "$STANDS/corpus.cpp" "$ROOT/benchmark/codec/toml/toml.cpp" \
 -o "$OUTPUT/corpus" || exit 1
# Выполняем сличение эталонных текстов
"$OUTPUT/corpus" || exit 1

##
# Функция вывода сообщения о пропуске стенда
#
# @param $1 название стенда
# @param $2 причина пропуска стенда
#
omit(){
	# Выводим сообщение о пропуске стенда
	echo "Skip \"$1\": $2"
}

# Выводим сообщение о сборке стенда контейнера AWH
echo "Build \"awh\""

# Выполняем сборку стенда потокового чтения контейнера AWH
c++ $FLAGS -I"$ROOT/include" "$STANDS/awh.cpp" "$RELEASE/libawh.a" "$ROOT/third_party/lib/libdependence.a" -o "$OUTPUT/awh" || exit 1

# Выводим сообщение о сборке стенда дерева настроек контейнера AWH
echo "Build \"awh-tree\""

# Выполняем сборку стенда дерева настроек контейнера AWH
c++ $FLAGS -I"$ROOT/include" "$STANDS/awh-tree.cpp" "$RELEASE/libawh.a" "$ROOT/third_party/lib/libdependence.a" -o "$OUTPUT/awh-tree" || exit 1

# Если исходные тексты реализации toml++ получены
if [ -f "$VENDOR/tomlplusplus/toml.hpp" ]; then
	# Выводим сообщение о сборке стенда реализации toml++
	echo "Build \"tomlplusplus\""
	# Выполняем сборку стенда реализации toml++
	c++ $FLAGS -I"$VENDOR/tomlplusplus" "$STANDS/tomlplusplus.cpp" -o "$OUTPUT/tomlplusplus" || exit 1
# Если исходные тексты реализации toml++ не получены
else
	# Выводим сообщение о пропуске стенда
	omit "tomlplusplus" "submodules/tomlplusplus is not checked out"
fi

# Если исходные тексты реализации toml11 получены
if [ -f "$VENDOR/toml11/single_include/toml.hpp" ]; then
	# Выводим сообщение о сборке стенда реализации toml11
	echo "Build \"toml11\""
	# Выполняем сборку стенда реализации toml11
	c++ $FLAGS -I"$VENDOR/toml11/single_include" "$STANDS/toml11.cpp" -o "$OUTPUT/toml11" || exit 1
# Если исходные тексты реализации toml11 не получены
else
	# Выводим сообщение о пропуске стенда
	omit "toml11" "submodules/toml11 is not checked out"
fi

# Если исходные тексты реализации cpptoml получены
if [ -f "$VENDOR/cpptoml/include/cpptoml.h" ]; then
	# Выводим сообщение о сборке стенда реализации cpptoml
	echo "Build \"cpptoml\""
	# Выполняем сборку стенда реализации cpptoml
	c++ $FLAGS -I"$VENDOR/cpptoml/include" "$STANDS/cpptoml.cpp" -o "$OUTPUT/cpptoml" || exit 1
# Если исходные тексты реализации cpptoml не получены
else
	# Выводим сообщение о пропуске стенда
	omit "cpptoml" "submodules/cpptoml is not checked out"
fi

# Если исходные тексты реализации tomlc99 получены
if [ -f "$VENDOR/tomlc99/toml.c" ]; then
	# Выводим сообщение о сборке стенда реализации tomlc99
	echo "Build \"tomlc99\""
	# Выполняем сборку сравниваемой реализации tomlc99
	cc $CFLAGS -I"$VENDOR/tomlc99" -c "$VENDOR/tomlc99/toml.c" -o "$OUTPUT/tomlc99-core.o" || exit 1
	# Выполняем связывание стенда реализации tomlc99
	c++ $FLAGS -I"$VENDOR/tomlc99" "$STANDS/tomlc99.cpp" "$OUTPUT/tomlc99-core.o" -o "$OUTPUT/tomlc99" || exit 1
# Если исходные тексты реализации tomlc99 не получены
else
	# Выводим сообщение о пропуске стенда
	omit "tomlc99" "submodules/tomlc99 is not checked out"
fi

# Выполняем удаление промежуточных объектных файлов
rm -f "$OUTPUT"/*.o

# Выводим сообщение о завершении сборки стендов
echo "Done: $OUTPUT"
