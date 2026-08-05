#!/usr/bin/env bash

# Если команда указана
if [ -n "$1" ]; then
	# Если необходимо удалить или очистить модуль
	if [ $1 = "--clean" ]; then
		# Очищаем сабмодуль
		clean_submodule "snappy"
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
		rm -rf "$PREFIX/include/snappy"

		# Переходим обратно в рабочий каталог
		cd "$ROOT" || exit 1
	# Если необходимо выполнить переключение на указанную ветку
	elif [ $1 = "--switch" ] && [ -n "$2" ]; then
		# Переключение ветки
		src="$ROOT/../submodules/snappy"
		printf "\n****** ANYKS Snappy ******\n"
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
		# Сборка Snappy
		src="$ROOT/../submodules/snappy"
		if [ ! -f "$src/.stamp_done" ]; then
			printf "\n****** ANYKS Snappy ******\n"
			cd "$src" || exit 1

			# Устанавливаем название флага
			FLAG="--v"
			# Устанавливаем название ветки/тега/версии по умолчанию
			NAME="1.2.1"

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
				git checkout main
				# Выполняем удаление предыдущей закаченной версии
				git tag -d ${NAME}
				# Закачиваем все изменения
				git fetch --all
				# Закачиваем все теги
				git fetch --all --tags
				# Удаляем старую ветку
				git branch -D v${NAME}-branch
				# Выполняем переключение на указанную версию
				git checkout -b v${NAME}-branch ${NAME}

				# Если установлена команда обновления сборки
				if [[ $1 = "--update" ]]; then
					# Выполняем обновление репозитория
					git pull origin main
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
			rm -rf ./CMakeCache.txt

			# Выполняем конфигурацию проекта
			if [[ $OS = "Windows" ]]; then
				cmake \
				 -DCMAKE_BUILD_TYPE="Release" \
				 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
				 -DSNAPPY_INSTALL="YES" \
				 -DBUILD_SHARED_LIBS="NO" \
				 -DSNAPPY_BUILD_TESTS="NO" \
				 -DSNAPPY_FUZZING_BUILD="NO" \
				 -DSNAPPY_BUILD_BENCHMARKS="NO" \
				 -G "MSYS Makefiles" \
				 .. || exit 1
			else
				cmake \
				 -DCMAKE_BUILD_TYPE="Release" \
				 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
				 -DSNAPPY_INSTALL="YES" \
				 -DBUILD_SHARED_LIBS="NO" \
				 -DSNAPPY_BUILD_TESTS="NO" \
				 -DSNAPPY_FUZZING_BUILD="NO" \
				 -DSNAPPY_BUILD_BENCHMARKS="NO" \
				 .. || exit 1
			fi

			# Выполняем сборку на всех логических ядрах
			$MAKE -j"$numproc" || exit 1
			# Выполняем установку проекта
			$MAKE install || exit 1

			# Выполняем компенсацию каталогов
			restorelibs $PREFIX

			# Создаём каталог Snappy
			mkdir "$PREFIX/include/snappy"

			# Производим установку заголовочных файлов по нужному пути
			for i in $(ls "$PREFIX/include" | grep ".*\.h$");
			do
				echo "Move \"$PREFIX/include/$i\" to \"$PREFIX/include/snappy/$i\""
				mv "$PREFIX/include/$i" "$PREFIX/include/snappy/$i" || exit 1
			done

			# Помечаем флагом, что сборка и установка произведена
			touch "$src/.stamp_done"

			# Переходим обратно в рабочий каталог
			cd "$ROOT" || exit 1
		fi
	fi
fi
