#!/usr/bin/env bash

# Получаем корневую дирректорию
readonly ROOT=$(cd "$(dirname "$0")" && pwd)

# Если команда указана
if [ -n "$1" ]; then
	# Если команда указана удаление сабмодуля
	if [ $1 = "remove" ]; then
		# Если название сабмодуля для удаления указано
		if [ -n "$2" ]; then
			# Удаляем указанный сабмодуль
			git submodule deinit -f -- $ROOT/submodules/$2
			rm -rf $ROOT/.git/modules/submodules/$2
			git rm -rf $ROOT/submodules/$2
			# git rm --cached $ROOT/submodules/$2
			# Выводим сообщение что удаление выполнено
			echo "Submodule \"$2\" removed successfully"
		else
			echo "Submodule name to remove is not specified"
		fi
	# Если команда указана добавления собмодуля
	elif [ $1 = "add" ]; then
		# Если название сабмодуля для удаления указано
		if [ -n "$2" ]; then
			# Если URL-адрес сабмодуля передан
			if [ -n "$3" ]; then
				# Добавляем новый сабмодуль
				git submodule add $3 submodules/$2
				# Выводим сообщение что добавление выполнено
				echo "Submodule \"$2\" added successfully"
			else
				echo "URL address of the submodule is not specified"
			fi
		else
			echo "Submodule name to add is not specified"
		fi
	else
		echo "An unknown command was entered, enter \"remove\" or \"add\""
	fi
else
	echo "Command is not specified"
fi
