#!/usr/bin/env bash

# Получаем корневую дирректорию
readonly ROOT=$(cd "$(dirname "$0")" && pwd)

# Получаем версию OS
readonly OS=$(uname -a | awk '{print $1}')

# Если операционной системой является macOS
if [ $OS = "Darwin" ]; then
	##
	# Файл прав здесь только ЧИТАЕТСЯ
	#
	# Прежде тут стояла дозапись через `PlistBuddy -c "Add ..."`, и это был дефект: файл
	# прав один на все цели, а подпись идёт из десятков целей разом при сборке в
	# несколько потоков. Одна цель читала файл, пока другая его переписывала, - отсюда
	# «Cannot parse a NULL or zero-length data» и «Error Reading File» в журнале сборки.
	# Дозапись притом не добавляла НИЧЕГО: нужный ключ в файле уже есть, и всякий раз
	# отвечалось «Entry Already Exists». Испортить же она могла файл, лежащий в дереве,
	# - он отслеживается git
	##
	codesign -s - -f --entitlements $2 $1
	# Разрешаем создание дампа ядра
	ulimit -c unlimited
# Если операционная система не является macOS
else
	# Выводим сообщение об ошибке
	echo "This script cannot be executed in the $OS operating environment."
fi
