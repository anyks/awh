#!/usr/bin/env bash

# Получаем корневую дирректорию
readonly ROOT=$(cd "$(dirname "$0")" && pwd)

# Получаем версию OS
readonly OS=$(uname -a | awk '{print $1}')

# Если операционной системой является MacOS X
if [ $OS = "Darwin" ]; then
	# Выполняем активирование разрешения создания дампа ядра
	/usr/libexec/PlistBuddy -c "Add :com.apple.security.get-task-allow bool true" $2
	# Выполняем подпись приложения
	codesign -s - -f --entitlements $2 $1
	# Разрешаем создание дампа ядра
	ulimit -c unlimited
# Если операционная система не является MacOS X
else
	# Выводим сообщение об ошибке
	echo "This script cannot be executed in the $OS operating environment."
fi
