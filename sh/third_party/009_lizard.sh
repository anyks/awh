#!/usr/bin/env bash

# Если команда указана
if [ -n "$1" ]; then
	# Если необходимо удалить или очистить модуль
	if [ $1 = "--clean" ]; then
		# Очищаем сабмодуль
		clean_submodule "lizard"
		# Для операционной системы Windows
		if [[ $OS = "Windows" ]]; then
			# Удаляем все зависимости библиотеки
			rm -f "$PREFIX/lib/libdependence.lib"
		# Для всех остальных операционных систем
		else
			# Удаляем все зависимости библиотеки
			rm -f "$PREFIX/lib/libdependence.a"
		fi
		# Удаляем все зависимые заголовки библиотеки
		rm -rf "$PREFIX/include/lizard"

		# Переходим обратно в рабочий каталог
		cd "$ROOT" || exit 1
	# Если необходимо выполнить переключение на указанную ветку
	elif [ $1 = "--switch" ] && [ -n "$2" ]; then
		# Переключение ветки
		src="$ROOT/../submodules/lizard"
		printf "\n****** ANYKS Lizard ******\n"
		cd "$src" || exit 1

		# Выполняем получение данных с репозитория
		git pull origin $2 || exit 1
		git fetch origin $2 || exit 1

		# Переключаемся на указанную ветку
		git checkout $2 || exit 1

		# Переходим обратно в рабочий каталог
		cd "$ROOT" || exit 1
	# Если необходимо собрать проект
	elif [ $1 = "--build" ] || [ $1 = "--update" ]; then
		# Сборка Lizard
		src="$ROOT/../submodules/lizard"
		if [ ! -f "$src/.stamp_done" ]; then
			printf "\n****** ANYKS Lizard ******\n"
			cd "$src" || exit 1

			# Устанавливаем название флага
			FLAG="--v"
			# Устанавливаем название ветки/тега/версии по умолчанию
			NAME="1.0"

			# Если ветка или тег передан
			if [ -n "$2" ]; then
				# Получаем название ветки или тега
				NAME=$(echo "${2:3}")
				# Получаем флаг ветки или тега
				FLAG=$(echo "$2" | cut -c-3)
			fi

			# Если флаг передан в виде ветки
			if [ $FLAG = "--b" ]; then
				# Скачиваем зависимости
				git fetch origin $NAME
				# Переключаемся на ветку
				git checkout $NAME
				# Выполняем обновление репозитория
				git pull origin $NAME
			# Если флаг передан в виде коммита
			elif [ $FLAG = "--c" ]; then
				# Если установлена команда обновления сборки
				if [[ $1 = "--update" ]]; then
					# Выполняем обновление репозитория
					git pull origin
				fi
				# Переходим на указанный коммит
				git reset --hard $NAME
			# Если флаг передан в виде тега
			elif [ $FLAG = "--t" ]; then
				# Выполняем удаление предыдущей закаченной версии
				git tag -d $NAME
				# Закачиваем все изменения
				git fetch --all
				# Закачиваем все теги
				git fetch --all --tags
				# Удаляем старую ветку
				git branch -D $NAME-branch
				# Выполняем переключение на указанную версию
				git checkout -b $NAME-branch $NAME

				# Если установлена команда обновления сборки
				if [[ $1 = "--update" ]]; then
					# Выполняем обновление репозитория
					git pull origin
				fi
			# Если флаг передан в виде версии
			elif [ $FLAG = "--v" ]; then
				# Переключаемся на lizard
				git checkout lizard
				# Выполняем удаление предыдущей закаченной версии
				git tag -d v${NAME}
				# Закачиваем все изменения
				git fetch --all
				# Закачиваем все теги
				git fetch --all --tags
				# Удаляем старую ветку
				git branch -D v${NAME}-branch
				# Выполняем переключение на указанную версию
				git checkout -b v${NAME}-branch v${NAME}

				# Если установлена команда обновления сборки
				if [[ $1 = "--update" ]]; then
					# Выполняем обновление репозитория
					git pull origin lizard
				fi
			# Если передан непонятный флаг
			else
				# Сообщаем, что флаги не поддерживаются
				echo "Flag \"$FLAG\" is not supported"
				# Выходим из скрипта
				exit 1
			fi

			# Применяем патч
			apply_patch "lizard" "lizard.patch"

			# Выполняем сборку библиотеки
			$MAKE lib || exit 1

			# Производим установку библиотеки по нужному пути
			echo "Install \"$src/lib/liblizard.a\" to \"$PREFIX/lib/liblizard.a\""
			${INSTALL_CMD} "$src/lib/liblizard.a" "$PREFIX/lib/liblizard.a" || exit 1

			mkdir -p "$PREFIX/include/lizard/xxhash"
			mkdir -p "$PREFIX/include/lizard/entropy"

			# Производим установку заголовочных файлов по нужному пути
			for i in $(ls "$src/lib" | grep \\.h$);
			do
				echo "Install \"$src/lib/$i\" to \"$PREFIX/include/lizard/$i\""
				${INSTALL_CMD} "$src/lib/$i" "$PREFIX/include/lizard/$i" || exit 1
			done

			# Производим установку оставшихся заголовочных файлов по нужному пути
			for i in $(ls "$src/lib/xxhash" | grep \\.h$);
			do
				echo "Install \"$src/lib/xxhash/$i\" to \"$PREFIX/include/lizard/xxhash/$i\""
				${INSTALL_CMD} "$src/lib/xxhash/$i" "$PREFIX/include/lizard/xxhash/$i" || exit 1
			done

			# Производим установку оставшихся заголовочных файлов по нужному пути
			for i in $(ls "$src/lib/entropy" | grep \\.h$);
			do
				echo "Install \"$src/lib/entropy/$i\" to \"$PREFIX/include/lizard/entropy/$i\""
				${INSTALL_CMD} "$src/lib/entropy/$i" "$PREFIX/include/lizard/entropy/$i" || exit 1
			done

			# Выполняем компенсацию каталогов
			restorelibs $PREFIX

			# Помечаем флагом, что сборка и установка произведена
			touch "$src/.stamp_done"

			# Переходим обратно в рабочий каталог
			cd "$ROOT" || exit 1
		fi
	fi
fi
