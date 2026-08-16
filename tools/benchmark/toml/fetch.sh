#!/usr/bin/env bash

# Получаем корневую дирректорию репозитория (каталог скрипта на три уровня ниже)
readonly ROOT=$(cd "$(dirname "$0")/../../.." && pwd)

# Каталог исходных текстов сравниваемых реализаций
readonly VENDOR="$ROOT/third_party/rival/toml"

# Ревизия исходных текстов реализации toml++
readonly TOMLPLUSPLUS_REVISION="v3.4.0"

# Ревизия исходных текстов реализации toml11
readonly TOML11_REVISION="v4.4.0"

# Ревизия исходных текстов реализации cpptoml
readonly CPPTOML_REVISION="master"

# Ревизия исходных текстов реализации tomlc99
readonly TOMLC99_REVISION="master"

##
# Функция получения одного файла
#
# @param $1 адрес файла
# @param $2 название файла в каталоге исходных текстов
#
obtain(){
	# Если файл уже получен
	if [ -f "$VENDOR/$2" ]; then
		# Выводим сообщение о пропуске файла
		echo "Skip \"$2\""
		# Выходим без получения файла
		return 0
	fi

	# Выводим сообщение о получении файла
	echo "Fetch \"$2\""

	# Выполняем создание каталога размещения файла
	mkdir -p "$(dirname "$VENDOR/$2")"

	# Выполняем получение файла
	curl -fsSL --max-time 120 -o "$VENDOR/$2" "$1" || exit 1
}

# Выполняем создание каталога исходных текстов
mkdir -p "$VENDOR"

# Выполняем получение исходных текстов реализации toml++
obtain "https://raw.githubusercontent.com/marzer/tomlplusplus/$TOMLPLUSPLUS_REVISION/toml.hpp" "tomlplusplus/toml.hpp"

# Выполняем получение исходных текстов реализации toml11
obtain "https://raw.githubusercontent.com/ToruNiina/toml11/$TOML11_REVISION/single_include/toml.hpp" "toml11/toml.hpp"

# Выполняем получение исходных текстов реализации cpptoml
obtain "https://raw.githubusercontent.com/skystrife/cpptoml/$CPPTOML_REVISION/include/cpptoml.h" "cpptoml/cpptoml.h"

# Выполняем получение исходных текстов реализации tomlc99
obtain "https://raw.githubusercontent.com/cktan/tomlc99/$TOMLC99_REVISION/toml.h" "tomlc99/toml.h"

# Выполняем получение исходных текстов реализации tomlc99
obtain "https://raw.githubusercontent.com/cktan/tomlc99/$TOMLC99_REVISION/toml.c" "tomlc99/toml.c"

# Выводим сообщение о завершении получения исходных текстов
echo "Done: $VENDOR"
