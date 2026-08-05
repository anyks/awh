#!/usr/bin/env bash

# Если команда указана
if [ -n "$1" ]; then
	# Если необходимо удалить или очистить модуль
	if [ $1 = "--clean" ]; then
		# Очищаем сабмодуль
		clean_submodule "lz4"
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
		rm -rf "$PREFIX/include/lz4"

		# Переходим обратно в рабочий каталог
		cd "$ROOT" || exit 1
	# Если необходимо выполнить переключение на указанную ветку
	elif [ $1 = "--switch" ] && [ -n "$2" ]; then
		# Переключение ветки
		src="$ROOT/../submodules/lz4"
		printf "\n****** ANYKS libZ4 ******\n"
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
		# Сборка libZ4
		src="$ROOT/../submodules/lz4"
		if [ ! -f "$src/.stamp_done" ]; then
			printf "\n****** ANYKS libZ4 ******\n"
			cd "$src/build/cmake" || exit 1

			# Устанавливаем название флага
			FLAG="--v"
			# Устанавливаем название ветки/тега/версии по умолчанию
			NAME="1.10.0"

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
				# Переключаемся на ветку
				git checkout release
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
					git pull origin release
				fi
			# Если передан непонятный флаг
			else
				# Сообщаем, что флаги не поддерживаются
				echo "Flag \"$FLAG\" is not supported"
				# Выходим из скрипта
				exit 1
			fi

			# Создаём каталог сборки
			mkdir -p "build" || exit 1
			# Переходим в каталог
			cd "build" || exit 1

			# Удаляем старый файл кэша
			rm -rf "$src/build/cmake/build/CMakeCache.txt"

			# Выполняем конфигурацию проекта
			if [[ $OS = "Windows" ]]; then
				cmake \
				 -DBUILD_SHARED_LIBS="OFF" \
				 -DBUILD_STATIC_LIBS="ON" \
				 -DCMAKE_BUILD_TYPE=Release \
				 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
				 -G "MSYS Makefiles" \
				 .. || exit 1
			else
				cmake \
				 -DBUILD_SHARED_LIBS="OFF" \
				 -DBUILD_STATIC_LIBS="ON" \
				 -DCMAKE_BUILD_TYPE=Release \
				 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
				 .. || exit 1
			fi

			# Выполняем сборку на всех логических ядрах
			$MAKE -j"$numproc" || exit 1
			# Выполняем установку проекта
			$MAKE install || exit 1

			# Выполняем компенсацию каталогов
			restorelibs $PREFIX

			# Создаём каталог lizZ4
			mkdir "$PREFIX/include/lz4"

			# Производим установку заголовочных файлов по нужному пути
			for i in $(ls "$PREFIX/include" | grep "lz4.*\.h$");
			do
				echo "Move \"$PREFIX/include/$i\" to \"$PREFIX/include/lz4/$i\""
				mv "$PREFIX/include/$i" "$PREFIX/include/lz4/$i" || exit 1
			done

			# Помечаем флагом, что сборка и установка произведена
			touch "$src/.stamp_done"

			# Переходим обратно в рабочий каталог
			cd "$ROOT" || exit 1
		fi
	fi
fi
