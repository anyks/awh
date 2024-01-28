#!/usr/bin/env bash

# Получаем корневую дирректорию
readonly ROOT=$(cd "$(dirname "$0")" && pwd)

# Получаем версию OS
OS=$(uname -a | awk '{print $1}')

if [[ $OS =~ "MINGW64" ]]; then
	OS="Windows"
fi

# Получаем адрес назначения
DESTINATION=$1

# Если место назначения указанно
if [ -n "$DESTINATION" ]; then

	# Получаем адрес CMAKE-файла
	CMAKE="$ROOT/third_party/cmake/FindAWH.cmake"

	# Если адрес CMAKE-файла уже существует
	if [ -f "$CMAKE" ]; then
		# Выполняем удаление старого адреса CMAKE-файла
		rm "$CMAKE"
	# Если адрес CMAKE-файла не существует
	else
		# Выполняем создание каталога хранения CMAKE-файла
		mkdir -p "$ROOT/third_party/cmake"
	fi

	# Выполняем копирование CMAKE-файла
	cp "$ROOT/contrib/cmake/FindAWH.cmake" "$CMAKE"

	# Если операционная система FreeBSD
	if [ "$OS" = "FreeBSD" ]; then
		# Заменяем конечный адрес назначения
		sed -i -e "s!\${CMAKE_SOURCE_DIR}/third_party!${DESTINATION}!" "$ROOT/third_party/cmake/FindAWH.cmake"
		# Удаляем временные паразитные файлы
		if [ -f "$ROOT/third_party/cmake/FindAWH.cmake-e" ]; then
			rm "$ROOT/third_party/cmake/FindAWH.cmake-e"
		fi
	# Если операционная система MacOS X
	elif [ "$OS" = "Darwin" ]; then
		# Заменяем конечный адрес назначения
		sed -i -e "s!\${CMAKE_SOURCE_DIR}/third_party!${DESTINATION}!" "$ROOT/third_party/cmake/FindAWH.cmake"
		# Удаляем временные паразитные файлы
		if [ -f "$ROOT/third_party/cmake/FindAWH.cmake-e" ]; then
			rm "$ROOT/third_party/cmake/FindAWH.cmake-e"
		fi
	# Если операционная система Linux
	elif [ "$OS" = "Linux" ]; then
		# Заменяем конечный адрес назначения
		sed -i "s%\${CMAKE_SOURCE_DIR}/third_party%${DESTINATION}%g" "$ROOT/third_party/cmake/FindAWH.cmake"
	# Если операционная система Windows
	elif [ "$OS" = "Windows" ]; then
		# Заменяем конечный адрес назначения
		sed -i "s%\${CMAKE_SOURCE_DIR}/third_party%${DESTINATION}%g" "$ROOT/third_party/cmake/FindAWH.cmake"
	fi
# Если место назначения не указанно
else
	# Выводим сообщение об ошибке
	echo "Destination address is not specified"
	exit 1
fi
