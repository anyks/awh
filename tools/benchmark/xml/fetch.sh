#!/usr/bin/env bash

# Получаем корневую дирректорию репозитория (каталог скрипта на три уровня ниже)
readonly ROOT=$(cd "$(dirname "$0")/../../.." && pwd)

# Каталог исходных текстов сравниваемых реализаций
readonly VENDOR="$ROOT/third_party/rival/xml"

# Ревизия исходных текстов реализации pugixml
readonly PUGIXML_REVISION="v1.15"

# Ревизия исходных текстов реализации RapidXML
readonly RAPIDXML_REVISION="master"

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

	# Выполняем получение файла
	curl -fsSL --max-time 60 -o "$VENDOR/$2" "$1" || exit 1
}

# Выполняем создание каталога исходных текстов
mkdir -p "$VENDOR"

# Выполняем получение исходных текстов реализации pugixml
obtain "https://raw.githubusercontent.com/zeux/pugixml/$PUGIXML_REVISION/src/pugixml.hpp" "pugixml.hpp"
obtain "https://raw.githubusercontent.com/zeux/pugixml/$PUGIXML_REVISION/src/pugixml.cpp" "pugixml.cpp"
obtain "https://raw.githubusercontent.com/zeux/pugixml/$PUGIXML_REVISION/src/pugiconfig.hpp" "pugiconfig.hpp"

# Выполняем получение исходных текстов реализации RapidXML
obtain "https://raw.githubusercontent.com/discord/rapidxml/$RAPIDXML_REVISION/rapidxml.hpp" "rapidxml.hpp"

# Выводим сообщение об успешном получении исходных текстов
echo ""
echo "Sources are placed in \"$VENDOR\""
echo ""
echo "Expat and libxml2 are taken from the system:"
echo "  macOS   - both ship with the SDK, nothing to install"
echo "  FreeBSD - pkg install expat libxml2"
echo "  Linux   - apt install libexpat1-dev libxml2-dev"
