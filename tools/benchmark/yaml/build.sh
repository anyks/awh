#!/usr/bin/env bash

# Получаем корневую дирректорию репозитория (каталог скрипта на три уровня ниже)
readonly ROOT=$(cd "$(dirname "$0")/../../.." && pwd)

# Каталог стендов сравнения
readonly STANDS="$ROOT/tools/benchmark/yaml"

# Каталог исходных текстов сравниваемых реализаций, подмодулями выложенных
readonly VENDOR="$ROOT/submodules"

# Каталог исходных текстов сравниваемых реализаций, выпуском получаемых

# Каталог собранных стендов
readonly OUTPUT="${1:-/tmp/rival-yaml}"

# Каталог сборки библиотеки AWH с оптимизацией
readonly RELEASE="${2:-$ROOT/build-release}"

# Флаги сборки стендов
#
# Уровень оптимизации совпадает с уровнем сборки библиотеки в режиме Release:
# сравнивать реализации, собранные с разной оптимизацией, бессмысленно
readonly FLAGS="-O3 -DNDEBUG -std=c++17 -Wall -Wextra"

# Флаги сборки сравниваемых реализаций, писанных на языке C
#
# Реализация libyaml писана на C и собирается им же: сборка её компилятором C++
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
SUITE_SMALL_ROUNDS="$(sed -n 's/.*static constexpr size_t SMALL_ROUNDS = \([0-9]*\);.*/\1/p' "$ROOT/benchmark/codec/yaml/reader.cpp")"
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
SUITE_LARGE_ROUNDS="$(sed -n 's/.*static constexpr size_t LARGE_ROUNDS = \([0-9]*\);.*/\1/p' "$ROOT/benchmark/codec/yaml/reader.cpp")"
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
SUITE_FOCUSED_ROUNDS="$(sed -n 's/.*static constexpr size_t FOCUSED_ROUNDS = \([0-9]*\);.*/\1/p' "$ROOT/benchmark/codec/yaml/reader.cpp")"
if [ -z "$STAND_FOCUSED_ROUNDS" ] || [ -z "$SUITE_FOCUSED_ROUNDS" ]; then
	echo "Error: количество прогонов «FOCUSED_ROUNDS» не найдено (стенд «$STAND_FOCUSED_ROUNDS», набор «$SUITE_FOCUSED_ROUNDS»)" >&2
	exit 1
fi
if [ "$STAND_FOCUSED_ROUNDS" != "$SUITE_FOCUSED_ROUNDS" ]; then
	echo "Error: количество прогонов «FOCUSED_ROUNDS» расходится: стенд $STAND_FOCUSED_ROUNDS, набор $SUITE_FOCUSED_ROUNDS" >&2
	exit 1
fi
# Сличаем количество прогонов «TREE_SMALL_ROUNDS»
STAND_TREE_SMALL_ROUNDS="$(sed -n 's/.*static constexpr size_t TREE_SMALL_ROUNDS = \([0-9]*\);.*/\1/p' "$STANDS/common.hpp")"
SUITE_TREE_SMALL_ROUNDS="$(sed -n 's/.*static constexpr size_t SMALL_ROUNDS = \([0-9]*\);.*/\1/p' "$ROOT/benchmark/codec/yaml/document.cpp")"
if [ -z "$STAND_TREE_SMALL_ROUNDS" ] || [ -z "$SUITE_TREE_SMALL_ROUNDS" ]; then
	echo "Error: количество прогонов «TREE_SMALL_ROUNDS» не найдено (стенд «$STAND_TREE_SMALL_ROUNDS», набор «$SUITE_TREE_SMALL_ROUNDS»)" >&2
	exit 1
fi
if [ "$STAND_TREE_SMALL_ROUNDS" != "$SUITE_TREE_SMALL_ROUNDS" ]; then
	echo "Error: количество прогонов «TREE_SMALL_ROUNDS» расходится: стенд $STAND_TREE_SMALL_ROUNDS, набор $SUITE_TREE_SMALL_ROUNDS" >&2
	exit 1
fi
# Сличаем количество прогонов «TREE_LARGE_ROUNDS»
STAND_TREE_LARGE_ROUNDS="$(sed -n 's/.*static constexpr size_t TREE_LARGE_ROUNDS = \([0-9]*\);.*/\1/p' "$STANDS/common.hpp")"
SUITE_TREE_LARGE_ROUNDS="$(sed -n 's/.*static constexpr size_t LARGE_ROUNDS = \([0-9]*\);.*/\1/p' "$ROOT/benchmark/codec/yaml/document.cpp")"
if [ -z "$STAND_TREE_LARGE_ROUNDS" ] || [ -z "$SUITE_TREE_LARGE_ROUNDS" ]; then
	echo "Error: количество прогонов «TREE_LARGE_ROUNDS» не найдено (стенд «$STAND_TREE_LARGE_ROUNDS», набор «$SUITE_TREE_LARGE_ROUNDS»)" >&2
	exit 1
fi
if [ "$STAND_TREE_LARGE_ROUNDS" != "$SUITE_TREE_LARGE_ROUNDS" ]; then
	echo "Error: количество прогонов «TREE_LARGE_ROUNDS» расходится: стенд $STAND_TREE_LARGE_ROUNDS, набор $SUITE_TREE_LARGE_ROUNDS" >&2
	exit 1
fi
# Сличаем количество прогонов «TREE_FOCUSED_ROUNDS»
STAND_TREE_FOCUSED_ROUNDS="$(sed -n 's/.*static constexpr size_t TREE_FOCUSED_ROUNDS = \([0-9]*\);.*/\1/p' "$STANDS/common.hpp")"
SUITE_TREE_FOCUSED_ROUNDS="$(sed -n 's/.*static constexpr size_t FOCUSED_ROUNDS = \([0-9]*\);.*/\1/p' "$ROOT/benchmark/codec/yaml/document.cpp")"
if [ -z "$STAND_TREE_FOCUSED_ROUNDS" ] || [ -z "$SUITE_TREE_FOCUSED_ROUNDS" ]; then
	echo "Error: количество прогонов «TREE_FOCUSED_ROUNDS» не найдено (стенд «$STAND_TREE_FOCUSED_ROUNDS», набор «$SUITE_TREE_FOCUSED_ROUNDS»)" >&2
	exit 1
fi
if [ "$STAND_TREE_FOCUSED_ROUNDS" != "$SUITE_TREE_FOCUSED_ROUNDS" ]; then
	echo "Error: количество прогонов «TREE_FOCUSED_ROUNDS» расходится: стенд $STAND_TREE_FOCUSED_ROUNDS, набор $SUITE_TREE_FOCUSED_ROUNDS" >&2
	exit 1
fi

# Выводим сообщение о сличении эталонных текстов
echo "Check \"corpus\""
# Выполняем сборку сличения эталонных текстов
c++ $FLAGS -I"$ROOT/include" -I"$STANDS" -I"$ROOT/benchmark/codec/yaml" -I"$ROOT/benchmark" \
 -I"$ROOT/tools/benchmark/syscount" "$STANDS/corpus.cpp" "$ROOT/benchmark/codec/yaml/yaml.cpp" \
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

# Выводим сообщение о сборке стенда потокового чтения контейнера AWH
echo "Build \"awh\""

# Выполняем сборку стенда потокового чтения контейнера AWH
c++ $FLAGS -I"$ROOT/include" -I"$STANDS" "$STANDS/awh.cpp" "$RELEASE/libawh.a" "$ROOT/third_party/lib/libdependence.a" -o "$OUTPUT/awh" || exit 1

# Выводим сообщение о сборке стенда дерева настроек контейнера AWH
echo "Build \"awh-tree\""

# Выполняем сборку стенда дерева настроек контейнера AWH
c++ $FLAGS -I"$ROOT/include" -I"$STANDS" "$STANDS/awh-tree.cpp" "$RELEASE/libawh.a" "$ROOT/third_party/lib/libdependence.a" -o "$OUTPUT/awh-tree" || exit 1

# Если исходные тексты реализации libyaml выложены
if [ -f "$VENDOR/libyaml/src/api.c" ]; then
	# Выводим сообщение о сборке стенда реализации libyaml
	echo "Build \"libyaml\""
	# Выполняем создание каталога заголовочного файла сборки реализации libyaml
	mkdir -p "$OUTPUT/libyaml-config"
	# Выполняем сборку заголовочного файла сборки реализации libyaml
	#
	# Собранного заголовочного файла в подмодуле нет: он выкладывается сборочной
	# оснасткой, а нужны из него четыре объявления выпуска, и проще выложить его
	# самому, нежели заводить ради него отдельную сборку подмодуля
	{
		echo "#define YAML_VERSION_MAJOR 0"
		echo "#define YAML_VERSION_MINOR 2"
		echo "#define YAML_VERSION_PATCH 5"
		echo "#define YAML_VERSION_STRING \"0.2.5\""
	} > "$OUTPUT/libyaml-config/config.h"
	# Перечень объектных файлов сравниваемой реализации
	OBJECTS=""
	# Выполняем перебор всех исходных текстов сравниваемой реализации
	for SOURCE in "$VENDOR"/libyaml/src/*.c; do
		# Выполняем сборку очередного исходного текста сравниваемой реализации
		cc $CFLAGS -DHAVE_CONFIG_H=1 -I"$OUTPUT/libyaml-config" -I"$VENDOR/libyaml/include" \
			-I"$VENDOR/libyaml/src" -c "$SOURCE" -o "$OUTPUT/libyaml-$(basename "$SOURCE" .c).o" || exit 1
		# Выполняем добавление объектного файла в перечень
		OBJECTS="$OBJECTS $OUTPUT/libyaml-$(basename "$SOURCE" .c).o"
	done
	# Выполняем связывание стенда реализации libyaml
	c++ $FLAGS -I"$VENDOR/libyaml/include" -I"$STANDS" "$STANDS/libyaml.cpp" $OBJECTS -o "$OUTPUT/libyaml" || exit 1
# Если исходные тексты реализации libyaml не выложены
else
	# Выводим сообщение о пропуске стенда
	omit "libyaml" "submodules/libyaml is not checked out"
fi

# Если исходные тексты реализации libfyaml выложены
if [ -f "$VENDOR/libfyaml/CMakeLists.txt" ]; then
	# Выводим сообщение о сборке стенда реализации libfyaml
	echo "Build \"libfyaml\""
	# Если сравниваемая реализация ещё не собрана
	#
	# Реализация эта единственная из пяти, чьи исходные тексты одним вызовом
	# компилятора не собираются: сборка её требует настройки, а настройка отнимает
	# около двух минут, и повторять её при всякой сборке стендов незачем
	if [ ! -f "$OUTPUT/libfyaml-build/libfyaml.dylib" ] && [ ! -f "$OUTPUT/libfyaml-build/libfyaml.so" ]; then
		# Выполняем настройку сборки сравниваемой реализации
		cmake -S "$VENDOR/libfyaml" -B "$OUTPUT/libfyaml-build" -DCMAKE_BUILD_TYPE=Release > /dev/null || exit 1
		# Выполняем сборку сравниваемой реализации
		cmake --build "$OUTPUT/libfyaml-build" --target fyaml -j 4 > /dev/null || exit 1
	fi
	# Выполняем сборку стенда реализации libfyaml
	c++ $FLAGS -I"$VENDOR/libfyaml/include" -I"$STANDS" "$STANDS/libfyaml.cpp" \
		-L"$OUTPUT/libfyaml-build" -lfyaml -Wl,-rpath,"$OUTPUT/libfyaml-build" -o "$OUTPUT/libfyaml" || exit 1
# Если исходные тексты реализации libfyaml не выложены
else
	# Выводим сообщение о пропуске стенда
	omit "libfyaml" "submodules/libfyaml is not checked out"
fi

# Если исходные тексты реализации yaml-cpp выложены
if [ -f "$VENDOR/yaml-cpp/src/parser.cpp" ]; then
	# Выводим сообщение о сборке стенда реализации yaml-cpp
	echo "Build \"yaml-cpp\""
	# Перечень объектных файлов сравниваемой реализации
	OBJECTS=""
	# Выполняем перебор всех исходных текстов сравниваемой реализации
	for SOURCE in "$VENDOR"/yaml-cpp/src/*.cpp; do
		# Выполняем сборку очередного исходного текста сравниваемой реализации
		c++ -O3 -DNDEBUG -std=c++17 -w -I"$VENDOR/yaml-cpp/include" -I"$VENDOR/yaml-cpp/src" \
			-c "$SOURCE" -o "$OUTPUT/yaml-cpp-$(basename "$SOURCE" .cpp).o" || exit 1
		# Выполняем добавление объектного файла в перечень
		OBJECTS="$OBJECTS $OUTPUT/yaml-cpp-$(basename "$SOURCE" .cpp).o"
	done
	# Выполняем связывание стенда реализации yaml-cpp
	c++ $FLAGS -I"$VENDOR/yaml-cpp/include" -I"$STANDS" "$STANDS/yaml-cpp.cpp" $OBJECTS -o "$OUTPUT/yaml-cpp" || exit 1
# Если исходные тексты реализации yaml-cpp не выложены
else
	# Выводим сообщение о пропуске стенда
	omit "yaml-cpp" "submodules/yaml-cpp is not checked out"
fi

# Если исходные тексты реализации fkYAML выложены
if [ -f "$VENDOR/fkYAML/single_include/fkYAML/node.hpp" ]; then
	# Выводим сообщение о сборке стенда реализации fkYAML
	echo "Build \"fkyaml\""
	# Выполняем сборку стенда реализации fkYAML
	c++ $FLAGS -I"$VENDOR/fkYAML/single_include" -I"$STANDS" "$STANDS/fkyaml.cpp" -o "$OUTPUT/fkyaml" || exit 1
# Если исходные тексты реализации fkYAML не выложены
else
	# Выводим сообщение о пропуске стенда
	omit "fkyaml" "submodules/fkYAML is not checked out"
fi

##
# Если исходные тексты реализации rapidyaml выложены
#
# Разбор ведёт c4core, выложенный самою реализацией свёртком «ext/c4core.src»:
# отдельного подмодуля ему не нужно. Прежде стенд собирался единым заголовочным
# файлом, забранным со стороны, тогда как соперник обязан
# собираться из подмодуля наравне с прочими
##
if [ -f "$VENDOR/rapidyaml/src/ryml.hpp" ]; then
	# Выводим сообщение о сборке стенда реализации rapidyaml
	echo "Build \"rapidyaml\""
	# Выполняем сборку стенда реализации rapidyaml
	c++ $FLAGS -I"$VENDOR/rapidyaml/src" -I"$VENDOR/rapidyaml/ext/c4core.src" -I"$STANDS" \
		"$STANDS/rapidyaml.cpp" "$VENDOR"/rapidyaml/src/c4/yml/*.cpp \
		"$VENDOR"/rapidyaml/ext/c4core.src/c4/*.cpp -o "$OUTPUT/rapidyaml" || exit 1
# Если исходные тексты реализации rapidyaml не выложены
else
	# Выводим сообщение о пропуске стенда
	omit "rapidyaml" "submodules/rapidyaml is not checked out"
fi

# Выполняем удаление промежуточных объектных файлов
rm -f "$OUTPUT"/*.o

# Выводим сообщение о завершении сборки стендов
echo "Done: $OUTPUT"
