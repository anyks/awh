#!/usr/bin/env bash

# Если команда указана
if [ -n "$1" ]; then
	# Если необходимо удалить или очистить модуль
	if [ $1 = "--clean" ]; then
		# Для операционной системы Windows
		if [[ $OS = "Windows" ]]; then
			# Удаляем все зависимости библиотеки
			rm -f "$PREFIX/lib/libdependence.lib"
		# Для всех остальных операционных систем
		else
			# Удаляем все зависимости библиотеки
			rm -f "$PREFIX/lib/libdependence.a"
		fi

		# Переходим обратно в рабочий каталог
		cd "$ROOT" || exit 1
	# Если необходимо собрать проект
	elif [ $1 = "--build" ] || [ $1 = "--update" ]; then
		# Выполняем объединение статических библиотек
		bash "$ROOT/merge_static_libs.sh"

		# Копируем скрипты хуков в директорию .git/hooks
		cp -f "$ROOT/prepare-commit-msg.sh" "$ROOT/../.git/hooks/commit-msg"
		cp -f "$ROOT/prepare-commit-msg.sh" "$ROOT/../.git/hooks/prepare-commit-msg"
		# Делаем скрипт prepare-commit-msg исполняемым
		chmod +x "$ROOT/../.git/hooks/commit-msg"
		chmod +x "$ROOT/../.git/hooks/prepare-commit-msg"

		# Переходим обратно в рабочий каталог
		cd "$ROOT" || exit 1
	fi
fi
