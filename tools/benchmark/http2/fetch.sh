#!/usr/bin/env bash

# Получаем корневую дирректорию репозитория (каталог скрипта на три уровня ниже)
readonly ROOT=$(cd "$(dirname "$0")/../../.." && pwd)

# Каталог исходных текстов сравниваемых реализаций
readonly VENDOR="$ROOT/third_party/rival/http2"

# Ревизия исходных текстов кодека ls-hpack
readonly LSHPACK_REVISION="master"

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

	# Выполняем создание каталога назначения файла
	mkdir -p "$(dirname "$VENDOR/$2")" || exit 1

	# Выполняем получение файла
	curl -fsSL --max-time 120 -o "$VENDOR/$2" "$1" || exit 1
}

# Выполняем создание каталога исходных текстов
mkdir -p "$VENDOR" || exit 1

##
# Кодек заголовков ls-hpack компании LiteSpeed
#
# Распространяется исходными текстами и собирается вместе со стендом:
# отдельной пакетной сборки у него нет, а в проектах-потребителях
# (lsquic, OpenLiteSpeed) исходные тексты включаются как есть
#
obtain "https://raw.githubusercontent.com/litespeedtech/ls-hpack/$LSHPACK_REVISION/lshpack.c" "lshpack.c"
obtain "https://raw.githubusercontent.com/litespeedtech/ls-hpack/$LSHPACK_REVISION/lshpack.h" "lshpack.h"
obtain "https://raw.githubusercontent.com/litespeedtech/ls-hpack/$LSHPACK_REVISION/lsxpack_header.h" "lsxpack_header.h"
obtain "https://raw.githubusercontent.com/litespeedtech/ls-hpack/$LSHPACK_REVISION/huff-tables.h" "huff-tables.h"

##
# Хеш-функция xxHash
#
# Кодек ls-hpack считает ею хеши названий и значений заголовков и поставляет
# её собственной копией в составе исходных текстов
#
obtain "https://raw.githubusercontent.com/litespeedtech/ls-hpack/$LSHPACK_REVISION/deps/xxhash/xxhash.c" "deps/xxhash/xxhash.c"
obtain "https://raw.githubusercontent.com/litespeedtech/ls-hpack/$LSHPACK_REVISION/deps/xxhash/xxhash.h" "deps/xxhash/xxhash.h"

# Выводим сообщение о завершении получения исходных текстов
echo "Sources are placed in \"$VENDOR\""
