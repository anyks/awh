#!/usr/bin/env bash

# Если команда указана
if [ -n "$1" ]; then
	# Если необходимо удалить или очистить модуль
	if [ $1 = "--clean" ]; then
		# Очищаем сабмодуль
		clean_submodule "lzma"
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
		rm -rf "$PREFIX/include/lzma"

		# Переходим обратно в рабочий каталог
		cd "$ROOT" || exit 1
	# Если необходимо выполнить переключение на указанную ветку
	elif [ $1 = "--switch" ] && [ -n "$2" ]; then
		# Переключение ветки
		src="$ROOT/../submodules/lzma"
		printf "\n****** ANYKS LZma ******\n"
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
		# Сборка LZma
		src="$ROOT/../submodules/lzma"
		if [ ! -f "$src/.stamp_done" ]; then
			printf "\n****** ANYKS LZma ******\n"
			cd "$src" || exit 1

			# Устанавливаем название флага
			FLAG="--v"
			# Устанавливаем название ветки/тега/версии по умолчанию
			NAME="5.2.3"

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
				git checkout master
				# Выполняем удаление предыдущей закаченной версии
				git tag -d v${NAME}-p4
				# Закачиваем все изменения
				git fetch --all
				# Закачиваем все теги
				git fetch --all --tags
				# Выполняем жесткое переключение на hunter
				git reset --hard origin/master
				# Переключаемся на hunter
				git checkout master
				# Выполняем обновление данных
				git pull origin master
				# Удаляем старую ветку
				git branch -D v${NAME}-branch
				# Выполняем переключение на указанную версию
				git checkout -b v${NAME}-branch v${NAME}-p4

				# Если установлена команда обновления сборки
				if [[ $1 = "--update" ]]; then
					# Выполняем обновление репозитория
					git pull origin master
				fi
			# Если передан непонятный флаг
			else
				# Сообщаем, что флаги не поддерживаются
				echo "Flag \"$FLAG\" is not supported"
				# Выходим из скрипта
				exit 1
			fi

			# Выполняем конфигурацию проекта
			if [[ $OS = "Windows" ]]; then
				# Применяем патч
				apply_patch "lzma" "lzma.patch"
			fi

			# Создаём каталог сборки
			mkdir -p "build" || exit 1
			# Переходим в каталог
			cd "build" || exit 1

			# Удаляем старый файл кэша
			rm -rf "$src/build/CMakeCache.txt"

			# Выполняем конфигурацию проекта
			if [[ $OS = "Windows" ]]; then
				cmake \
				 -DLZMA_BUILD_TESTS="OFF" \
				 -DBUILD_SHARED_LIBS="OFF" \
				 -DCMAKE_BUILD_TYPE=Release \
				 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
				 -G "MSYS Makefiles" \
				 .. || exit 1
			else
				cmake \
				 -DLZMA_BUILD_TESTS="OFF" \
				 -DBUILD_SHARED_LIBS="OFF" \
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

			# Создаём каталог LZma
			mkdir "$PREFIX/include/lzma2"

			# Перемещаем каталог lzma в новый каталог
			mv "$PREFIX/include/lzma" "$PREFIX/include/lzma2/lzma"

			# Производим установку заголовочных файлов по нужному пути
			for i in $(ls "$PREFIX/include" | grep ".*\.h$");
			do
				echo "Move \"$PREFIX/include/$i\" to \"$PREFIX/include/lzma/$i\""
				mv "$PREFIX/include/$i" "$PREFIX/include/lzma2/$i" || exit 1
			done

			# Переименовываем каталог lzma2 в новый каталог
			mv "$PREFIX/include/lzma2" "$PREFIX/include/lzma"

			# Выполняем компенсацию каталогов
			restorelibs $PREFIX

			# Помечаем флагом, что сборка и установка произведена
			touch "$src/.stamp_done"

			# Переходим обратно в рабочий каталог
			cd "$ROOT" || exit 1
		fi
	fi
fi
