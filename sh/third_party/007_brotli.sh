#!/usr/bin/env bash

# Если команда указана
if [ -n "$1" ]; then
	# Если необходимо удалить или очистить модуль
	if [ $1 = "--clean" ]; then
		# Очищаем сабмодуль
		clean_submodule "brotli"
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
		rm -rf "$PREFIX/include/brotli"

		# Переходим обратно в рабочий каталог
		cd "$ROOT" || exit 1
	# Если необходимо выполнить переключение на указанную ветку
	elif [ $1 = "--switch" ] && [ -n "$2" ]; then
		# Переключение ветки
		src="$ROOT/../submodules/brotli"
		printf "\n****** ANYKS Brotli ******\n"
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
		# Сборка Brotli
		src="$ROOT/../submodules/brotli"
		if [ ! -f "$src/.stamp_done" ]; then
			printf "\n****** ANYKS Brotli ******\n"
			cd "$src" || exit 1

			# Устанавливаем название флага
			FLAG="--v"
			# Устанавливаем название ветки/тега/версии по умолчанию
			NAME="1.1.0"

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
				# Переключаемся на master
				git checkout master
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
					git pull origin master
				fi
			# Если передан непонятный флаг
			else
				# Сообщаем, что флаги не поддерживаются
				echo "Flag \"$FLAG\" is not supported"
				# Выходим из скрипта
				exit 1
			fi

			# Каталог для сборки
			build="out"

			# Создаём каталог сборки
			mkdir -p ${build} || exit 1
			# Переходим в каталог
			cd ${build} || exit 1

			# Удаляем старый файл кэша
			rm -rf ./CMakeCache.txt

			# Создаём каталон назначения заголовочных файлов
			mkdir -p "$PREFIX/include/brotli"

			# Выполняем конфигурацию проекта
			if [[ $OS = "Windows" ]]; then
				cmake \
				 -DCMAKE_BUILD_TYPE="Release" \
				 -DBROTLI_EMSCRIPTEN="ON" \
				 -DBROTLI_DISABLE_TESTS="ON" \
				 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
				 -DBROTLI_LIBRARIES="$PREFIX/lib" \
				 -DBROTLI_INCLUDE_DIRS="$PREFIX/include" \
				 -DBUILD_SHARED_LIBS="OFF" \
				 -G "MSYS Makefiles" \
				 .. || exit 1
			else
				cmake \
				 -DCMAKE_BUILD_TYPE="Release" \
				 -DBROTLI_EMSCRIPTEN="ON" \
				 -DBROTLI_DISABLE_TESTS="ON" \
				 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
				 -DBROTLI_LIBRARIES="$PREFIX/lib" \
				 -DBROTLI_INCLUDE_DIRS="$PREFIX/include" \
				 -DBUILD_SHARED_LIBS="OFF" \
				 .. || exit 1
			fi

			# Выполняем сборку на всех логических ядрах
			$MAKE -j"$numproc" || exit 1

			##
			 # Команда установки задаётся родительским скриптом build_third_party.sh:
			 # пользуются ею несколько модулей, и способ её зависит от системы, а не от
			 # модуля. Прежде она задавалась здесь, а density и lizard читали её отсюда же -
			 # связь эта держалась лишь на том, что brotli идёт прежде них
			 ##

			# Производим установку библиотеки по нужному пути
			echo "Install \"$src/${build}/libbrotlicommon.a\" to \"$PREFIX/lib/libbrotlicommon.a\""
			${INSTALL_CMD} "$src/${build}/libbrotlicommon.a" "$PREFIX/lib/libbrotlicommon.a" || exit 1

			echo "Install \"$src/${build}/libbrotlidec.a\" to \"$PREFIX/lib/libbrotlidec.a\""
			${INSTALL_CMD} "$src/${build}/libbrotlidec.a" "$PREFIX/lib/libbrotlidec.a" || exit 1

			echo "Install \"$src/${build}/libbrotlienc.a\" to \"$PREFIX/lib/libbrotlienc.a\""
			${INSTALL_CMD} "$src/${build}/libbrotlienc.a" "$PREFIX/lib/libbrotlienc.a" || exit 1

			# Производим установку заголовочных файлов по нужному пути
			for i in $(ls "$src/c/include/brotli" | grep \\.h$);
			do
				echo "Install \"$src/c/include/brotli/$i\" to \"$PREFIX/include/brotli/$i\""
				${INSTALL_CMD} "$src/c/include/brotli/$i" "$PREFIX/include/brotli/$i" || exit 1
			done

			# Список модулей для сборки итоговой библиотеки
			MODULES=""
			# Переходим в каталог с библиотеками
			cd "$PREFIX/lib"

			# Если операционной системой является Windows
			if [ $OS = "Windows" ]; then
				# Выполняем формирование последовательности списка модулей
				for i in $(ar -t libbrotlienc.a | grep ".*\.obj$");
				do
					echo "Module: $i in libbrotlienc.a"
					if [ ! -n "$MODULES" ]; then
						MODULES="$i"
					else
						MODULES="$MODULES $i"
					fi
				done

				# Выполняем формирование последовательности списка модулей
				for i in $(ar -t libbrotlidec.a | grep ".*\.obj$");
				do
					echo "Module: $i in libbrotlidec.a"
					if [ ! -n "$MODULES" ]; then
						MODULES="$i"
					else
						MODULES="$MODULES $i"
					fi
				done

				# Выполняем формирование последовательности списка модулей
				for i in $(ar -t libbrotlicommon.a | grep ".*\.obj$");
				do
					echo "Module: $i in libbrotlicommon.a"
					if [ ! -n "$MODULES" ]; then
						MODULES="$i"
					else
						MODULES="$MODULES $i"
					fi
				done
			# Если операционной системой является Unix-подобная ОС
			else
				# Выполняем формирование последовательности списка модулей
				for i in $(ar -t libbrotlienc.a | grep ".*\.o$");
				do
					echo "Module: $i in libbrotlienc.a"
					if [ ! -n "$MODULES" ]; then
						MODULES="$i"
					else
						MODULES="$MODULES $i"
					fi
				done

				# Выполняем формирование последовательности списка модулей
				for i in $(ar -t libbrotlidec.a | grep ".*\.o$");
				do
					echo "Module: $i in libbrotlidec.a"
					if [ ! -n "$MODULES" ]; then
						MODULES="$i"
					else
						MODULES="$MODULES $i"
					fi
				done

				# Выполняем формирование последовательности списка модулей
				for i in $(ar -t libbrotlicommon.a | grep ".*\.o$");
				do
					echo "Module: $i in libbrotlicommon.a"
					if [ ! -n "$MODULES" ]; then
						MODULES="$i"
					else
						MODULES="$MODULES $i"
					fi
				done
			fi

			# Если список модулей не получен
			if [ ! -n "$MODULES" ]; then
				echo "Brotli library is not build"
				exit 1
			fi

			# Извлекаем все модули из библиотеки
			ar -xv libbrotlienc.a || exit 1
			ar -xv libbrotlidec.a || exit 1
			ar -xv libbrotlicommon.a || exit 1

			# Удаляем все старые библиотеки
			rm libbrotlienc.a || exit 1
			rm libbrotlidec.a || exit 1
			rm libbrotlicommon.a || exit 1

			# Выполняем сборку новой статической библиотеки
			ar -crv libbrotli.a $MODULES

			# Выполняем запуск библиотеки
			ranlib libbrotli.a

			# Если операционной системой является Windows
			if [ $OS = "Windows" ]; then
				# Выполняем удаление всех извлечённых модулей
				rm -rf *.obj
			# Если операционной системой является Unix-подобная ОС
			else
				# Выполняем удаление всех извлечённых модулей
				rm -rf *.o
				# Удаляем файл разметки
				rm -f "__.SYMDEF SORTED"
			fi

			# Выполняем компенсацию каталогов
			restorelibs $PREFIX

			# Помечаем флагом, что сборка и установка произведена
			touch "$src/.stamp_done"

			# Переходим обратно в рабочий каталог
			cd "$ROOT" || exit 1
		fi
	fi
fi
