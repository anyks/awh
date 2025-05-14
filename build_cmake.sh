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

	# Название CMAKE-файла
	NAME="FindAWH.cmake"

	# Получаем адрес CMAKE-файла
	CMAKE="$ROOT/third_party/cmake/$NAME"

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
	cp "$ROOT/contrib/cmake/$NAME" "$CMAKE"

	# Если операционная система MacOS X
	if [ "$OS" = "Darwin" ]; then
		# Заменяем конечный адрес назначения
		sed -i -e "s!\${CMAKE_SOURCE_DIR}/third_party!${DESTINATION}!" "$CMAKE"
		# Удаляем временные паразитные файлы
		if [ -f "$CMAKE-e" ]; then
			rm "$CMAKE-e"
		fi
	# Если операционная система Linux
	elif [ "$OS" = "Linux" ]; then
		# Заменяем конечный адрес назначения
		sed -i "s%\${CMAKE_SOURCE_DIR}/third_party%${DESTINATION}%g" "$CMAKE"
	# Если операционная система Windows
	elif [ "$OS" = "Windows" ]; then
		# Заменяем конечный адрес назначения
		sed -i "s%\${CMAKE_SOURCE_DIR}/third_party%${DESTINATION}%g" "$CMAKE"
	# Если операционная система FreeBSD, NetBSD или OpenBSD
	elif [ "$OS" = "FreeBSD" ] || [ "$OS" = "NetBSD" ] || [ "$OS" = "OpenBSD" ]; then
		# Заменяем конечный адрес назначения
		sed -i -e "s!\${CMAKE_SOURCE_DIR}/third_party!${DESTINATION}!" "$CMAKE"
		# Удаляем временные паразитные файлы
		if [ -f "$CMAKE-e" ]; then
			rm "$CMAKE-e"
		fi
	fi
# Если место назначения не указанно
else
	# Выводим сообщение об ошибке
	echo "Destination address is not specified"
	exit 1
fi
