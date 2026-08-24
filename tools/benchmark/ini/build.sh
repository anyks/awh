#!/usr/bin/env bash

# Получаем корневую дирректорию репозитория (каталог скрипта на три уровня ниже)
readonly ROOT=$(cd "$(dirname "$0")/../../.." && pwd)

# Каталог стендов сравнения
readonly STANDS="$ROOT/tools/benchmark/ini"

# Каталог исходных текстов сравниваемых реализаций
readonly VENDOR="$ROOT/submodules"

# Каталог собранных стендов
readonly OUTPUT="${1:-/tmp/rival-ini}"

# Каталог сборки библиотеки AWH с оптимизацией
readonly RELEASE="${2:-$ROOT/build-release}"

# Флаги сборки стендов
#
# Уровень оптимизации совпадает с уровнем сборки библиотеки в режиме Release:
# сравнивать реализации, собранные с разной оптимизацией, бессмысленно
readonly FLAGS="-O3 -DNDEBUG -std=c++17 -Wall -Wextra"

# Флаги сборки сравниваемых реализаций, писанных на языке C
#
# Реализации inih и iniparser писаны на C и собираются им же: сборка их
# компилятором C++ меняет и правила приведения типов, и способ связывания,
# а сравнивать надо то, что получает их обычный потребитель
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
# Выводим сообщение о сличении эталонных текстов
echo "Check \"corpus\""
# Выполняем сборку сличения эталонных текстов
c++ $FLAGS -I"$ROOT/include" -I"$STANDS" -I"$ROOT/benchmark/codec/ini" -I"$ROOT/benchmark" \
 -I"$ROOT/tools/benchmark/syscount" "$STANDS/corpus.cpp" "$ROOT/benchmark/codec/ini/ini.cpp" \
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

# Выводим сообщение о сборке стенда поэлементного снятия возможностей
echo "Build \"ablation\""

# Выполняем сборку стенда поэлементного снятия возможностей чтения
c++ $FLAGS -I"$ROOT/include" "$STANDS/ablation.cpp" "$RELEASE/libawh.a" "$ROOT/third_party/lib/libdependence.a" -o "$OUTPUT/ablation" || exit 1

# Если исходные тексты реализации inih получены
if [ -f "$VENDOR/inih/ini.c" ]; then
	# Выводим сообщение о сборке стенда реализации inih
	echo "Build \"inih\""
	# Выполняем сборку стенда реализации inih
	cc $CFLAGS -I"$VENDOR/inih" -c "$VENDOR/inih/ini.c" -o "$OUTPUT/inih-ini.o" || exit 1
	# Выполняем связывание стенда реализации inih
	c++ $FLAGS -I"$VENDOR/inih" "$STANDS/inih.cpp" "$OUTPUT/inih-ini.o" -o "$OUTPUT/inih" || exit 1
# Если исходные тексты реализации inih не получены
else
	# Выводим сообщение о пропуске стенда
	omit "inih" "submodules/inih not initialized"
fi

# Если исходные тексты реализации iniparser получены
if [ -f "$VENDOR/iniparser/src/iniparser.c" ]; then
	# Выводим сообщение о сборке стенда реализации iniparser
	echo "Build \"iniparser\""
	# Выполняем сборку стенда реализации iniparser
	cc $CFLAGS -I"$VENDOR/iniparser/src" -c "$VENDOR/iniparser/src/iniparser.c" -o "$OUTPUT/iniparser-core.o" || exit 1
	# Выполняем сборку словаря реализации iniparser
	cc $CFLAGS -I"$VENDOR/iniparser/src" -c "$VENDOR/iniparser/src/dictionary.c" -o "$OUTPUT/iniparser-dict.o" || exit 1
	# Выполняем связывание стенда реализации iniparser
	c++ $FLAGS -I"$VENDOR/iniparser/src" "$STANDS/iniparser.cpp" \
		"$OUTPUT/iniparser-core.o" "$OUTPUT/iniparser-dict.o" \
		-o "$OUTPUT/iniparser" || exit 1
# Если исходные тексты реализации iniparser не получены
else
	# Выводим сообщение о пропуске стенда
	omit "iniparser" "submodules/iniparser not initialized"
fi

# Если исходные тексты реализации SimpleIni получены
if [ -f "$VENDOR/simpleini/SimpleIni.h" ]; then
	# Выводим сообщение о сборке стенда реализации SimpleIni
	echo "Build \"simpleini\""
	# Выполняем сборку стенда реализации SimpleIni
	c++ $FLAGS -I"$VENDOR/simpleini" "$STANDS/simpleini.cpp" -o "$OUTPUT/simpleini" || exit 1
# Если исходные тексты реализации SimpleIni не получены
else
	# Выводим сообщение о пропуске стенда
	omit "simpleini" "submodules/simpleini not initialized"
fi

# Если исходные тексты реализации mINI получены
if [ -f "$VENDOR/mINI/src/mini/ini.h" ]; then
	# Выводим сообщение о сборке стенда реализации mINI
	echo "Build \"mini\""
	# Выполняем сборку стенда реализации mINI
	c++ $FLAGS -I"$VENDOR/mINI/src" "$STANDS/mini.cpp" -o "$OUTPUT/mini" || exit 1
# Если исходные тексты реализации mINI не получены
else
	# Выводим сообщение о пропуске стенда
	omit "mini" "submodules/mINI not initialized"
fi

# Если библиотека GLib установлена в системе
if pkg-config --exists glib-2.0 2>/dev/null; then
	# Выводим сообщение о сборке стенда реализации GKeyFile
	echo "Build \"gkeyfile\""
	# Выполняем сборку стенда реализации GKeyFile
	c++ $FLAGS $(pkg-config --cflags glib-2.0) "$STANDS/gkeyfile.cpp" \
		$(pkg-config --libs glib-2.0) -o "$OUTPUT/gkeyfile" || exit 1
# Если библиотека GLib в системе не установлена
else
	# Выводим сообщение о пропуске стенда
	omit "gkeyfile" "glib-2.0 not found by pkg-config"
fi

# Выполняем удаление промежуточных объектных файлов
rm -f "$OUTPUT"/*.o

# Выводим сообщение о завершении сборки стендов
echo "Done: $OUTPUT"
