#!/usr/bin/env bash

# Если команда указана
if [ -n "$1" ]; then
	# Если необходимо удалить или очистить модуль
	if [ $1 = "--clean" ]; then
		# Очищаем сабмодуль
		clean_submodule "pcre2"
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
		rm -rf "$PREFIX/include/pcre2"

		# Переходим обратно в рабочий каталог
		cd "$ROOT" || exit 1
	# Если необходимо выполнить переключение на указанную ветку
	elif [ $1 = "--switch" ] && [ -n "$2" ]; then
		# Переключение ветки
		src="$ROOT/../submodules/pcre2"
		printf "\n****** ANYKS PCRE2 ******\n"
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
		# Сборка PCRE2
		src="$ROOT/../submodules/pcre2"
		if [ ! -f "$src/.stamp_done" ]; then
			printf "\n****** ANYKS PCRE2 ******\n"
			cd "$src" || exit 1

			# Устанавливаем название флага
			FLAG="--v"
			# Устанавливаем название ветки/тега/версии по умолчанию
			NAME="10.46"

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
				# Выполняем удаление предыдущей закаченной версии
				git tag -d pcre2-${NAME}
				# Закачиваем все изменения
				git fetch --all
				# Закачиваем все теги
				git fetch --all --tags
				# Выполняем жесткое переключение на master
				git reset --hard origin/master
				# Переключаемся на master
				git checkout master
				# Выполняем обновление данных
				git pull origin master
				# Удаляем старую ветку
				git branch -D v${NAME}-branch
				# Выполняем переключение на указанную версию
				git checkout -b v${NAME}-branch pcre2-${NAME}

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

			# Создаём каталог сборки
			mkdir -p "build" || exit 1
			# Переходим в каталог
			cd "build" || exit 1

			# Удаляем старый файл кэша
			rm -rf "$src/build/CMakeCache.txt"

			# Выполняем конфигурацию проекта
			if [[ $OS = "Windows" ]]; then
				cmake \
				 -DCMAKE_SYSTEM_NAME=Windows \
				 -DCMAKE_BUILD_TYPE=Release \
				 -DPCRE2_STATIC_PIC="ON" \
				 -DBUILD_STATIC_LIBS="ON" \
				 -DPCRE2_BUILD_TESTS="OFF" \
				 -DPCRE2_SUPPORT_UNICODE="ON" \
				 -DPCRE2_BUILD_PCRE2_8="ON" \
				 -DPCRE2_BUILD_PCRE2_16="ON" \
				 -DPCRE2_BUILD_PCRE2_32="ON" \
				 -DPCRE2_SUPPORT_JIT="OFF" \
				 -DPCRE2_SUPPORT_LIBZ="OFF" \
				 -DPCRE2_SUPPORT_LIBBZ2="OFF" \
				 -DPCRE2_SUPPORT_LIBEDIT="OFF" \
				 -DPCRE2_SUPPORT_LIBREADLINE="OFF" \
				 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
				 -G "MSYS Makefiles" \
				 .. || exit 1
			else
				cmake \
				 -DCMAKE_BUILD_TYPE=Release \
				 -DPCRE2_STATIC_PIC="ON" \
				 -DBUILD_STATIC_LIBS="ON" \
				 -DPCRE2_BUILD_TESTS="OFF" \
				 -DPCRE2_SUPPORT_UNICODE="ON" \
				 -DPCRE2_BUILD_PCRE2_8="ON" \
				 -DPCRE2_BUILD_PCRE2_16="ON" \
				 -DPCRE2_BUILD_PCRE2_32="ON" \
				 -DPCRE2_SUPPORT_JIT="OFF" \
				 -DPCRE2_SUPPORT_LIBZ="OFF" \
				 -DPCRE2_SUPPORT_LIBBZ2="OFF" \
				 -DPCRE2_SUPPORT_LIBEDIT="OFF" \
				 -DPCRE2_SUPPORT_LIBREADLINE="OFF" \
				 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
				 .. || exit 1
			fi

			# Выполняем сборку на всех логических ядрах
			$MAKE -j"$numproc" || exit 1
			# Выполняем установку проекта
			$MAKE install || exit 1

			# Выполняем компенсацию каталогов
			restorelibs $PREFIX

			# Создаём каталог PCRE
			mkdir "$PREFIX/include/pcre2"

			# Производим установку заголовочных файлов по нужному пути
			for i in $(ls "$PREFIX/include" | grep "pcre2.*\.h$");
			do
				echo "Move \"$PREFIX/include/$i\" to \"$PREFIX/include/pcre2/$i\""
				mv "$PREFIX/include/$i" "$PREFIX/include/pcre2/$i" || exit 1
			done

			# Список модулей для сборки итоговой библиотеки
			MODULES=""
			# Переходим в каталог с библиотеками
			cd "$PREFIX/lib"

			# Если операционной системой является Windows
			if [ $OS = "Windows" ]; then
				# Выполняем формирование последовательности списка модулей
				for i in $(ar -t libpcre2-posix.a | grep ".*\.obj$");
				do
					echo "Module: $i in libpcre2-posix.a"
					if [ ! -n "$MODULES" ]; then
						MODULES="$i"
					else
						MODULES="$MODULES $i"
					fi
				done

				# Выполняем формирование последовательности списка модулей
				for i in $(ar -t libpcre2-8.a | grep ".*\.obj$");
				do
					echo "Module: $i in libpcre2-8.a"
					if [ ! -n "$MODULES" ]; then
						MODULES="a_$i"
					else
						MODULES="$MODULES a_$i"
					fi
				done

				# Выполняем формирование последовательности списка модулей
				for i in $(ar -t libpcre2-16.a | grep ".*\.obj$");
				do
					echo "Module: $i in libpcre2-16.a"
					if [ ! -n "$MODULES" ]; then
						MODULES="b_$i"
					else
						MODULES="$MODULES b_$i"
					fi
				done

				# Выполняем формирование последовательности списка модулей
				for i in $(ar -t libpcre2-32.a | grep ".*\.obj$");
				do
					echo "Module: $i in libpcre2-32.a"
					if [ ! -n "$MODULES" ]; then
						MODULES="c_$i"
					else
						MODULES="$MODULES c_$i"
					fi
				done
			# Если операционной системой является Unix-подобная ОС
			else
				# Выполняем формирование последовательности списка модулей
				for i in $(ar -t libpcre2-posix.a | grep ".*\.o$");
				do
					echo "Module: $i in libpcre2-posix.a"
					if [ ! -n "$MODULES" ]; then
						MODULES="$i"
					else
						MODULES="$MODULES $i"
					fi
				done

				# Выполняем формирование последовательности списка модулей
				for i in $(ar -t libpcre2-8.a | grep ".*\.o$");
				do
					echo "Module: $i in libpcre2-8.a"
					if [ ! -n "$MODULES" ]; then
						MODULES="a_$i"
					else
						MODULES="$MODULES a_$i"
					fi
				done

				# Выполняем формирование последовательности списка модулей
				for i in $(ar -t libpcre2-16.a | grep ".*\.o$");
				do
					echo "Module: $i in libpcre2-16.a"
					if [ ! -n "$MODULES" ]; then
						MODULES="b_$i"
					else
						MODULES="$MODULES b_$i"
					fi
				done

				# Выполняем формирование последовательности списка модулей
				for i in $(ar -t libpcre2-32.a | grep ".*\.o$");
				do
					echo "Module: $i in libpcre2-32.a"
					if [ ! -n "$MODULES" ]; then
						MODULES="c_$i"
					else
						MODULES="$MODULES c_$i"
					fi
				done
			fi

			# Если список модулей не получен
			if [ ! -n "$MODULES" ]; then
				echo "PCRE2 library is not build"
				exit 1
			fi

			# Извлекаем все модули из библиотеки
			ar -xv libpcre2-8.a || exit 1
			# Если операционной системой является Windows
			if [ $OS = "Windows" ]; then
				# Выполняем формирование последовательности списка модулей
				for i in $(ar -t libpcre2-8.a | grep ".*\.obj$");
				do
					# Если такой модуль уже существует
					if [[ -f $i ]]; then
						echo "Rename \"$i\" to \"a_$i\""
						mv "$i" "a_$i"
					fi
				done
			# Если операционной системой является Unix-подобная ОС
			else
				# Выполняем формирование последовательности списка модулей
				for i in $(ar -t libpcre2-8.a | grep ".*\.o$");
				do
					# Если такой модуль уже существует
					if [[ -f $i ]]; then
						echo "Rename \"$i\" to \"a_$i\""
						mv "$i" "a_$i"
					fi
				done
			fi
			# Извлекаем все модули из библиотеки
			ar -xv libpcre2-16.a || exit 1
			# Если операционной системой является Windows
			if [ $OS = "Windows" ]; then
				# Выполняем формирование последовательности списка модулей
				for i in $(ar -t libpcre2-16.a | grep ".*\.obj$");
				do
					# Если такой модуль уже существует
					if [[ -f $i ]]; then
						echo "Rename \"$i\" to \"b_$i\""
						mv "$i" "b_$i"
					fi
				done
			# Если операционной системой является Unix-подобная ОС
			else
				# Выполняем формирование последовательности списка модулей
				for i in $(ar -t libpcre2-16.a | grep ".*\.o$");
				do
					# Если такой модуль уже существует
					if [[ -f $i ]]; then
						echo "Rename \"$i\" to \"b_$i\""
						mv "$i" "b_$i"
					fi
				done
			fi
			# Извлекаем все модули из библиотеки
			ar -xv libpcre2-32.a || exit 1
			# Если операционной системой является Windows
			if [ $OS = "Windows" ]; then
				# Выполняем формирование последовательности списка модулей
				for i in $(ar -t libpcre2-32.a | grep ".*\.obj$");
				do
					# Если такой модуль уже существует
					if [[ -f $i ]]; then
						echo "Rename \"$i\" to \"c_$i\""
						mv "$i" "c_$i"
					fi
				done
			# Если операционной системой является Unix-подобная ОС
			else
				# Выполняем формирование последовательности списка модулей
				for i in $(ar -t libpcre2-32.a | grep ".*\.o$");
				do
					# Если такой модуль уже существует
					if [[ -f $i ]]; then
						echo "Rename \"$i\" to \"c_$i\""
						mv "$i" "c_$i"
					fi
				done
			fi
			# Извлекаем все модули из библиотеки
			ar -xv libpcre2-posix.a || exit 1

			# Удаляем все старые библиотеки
			rm libpcre2-8.a || exit 1
			rm libpcre2-16.a || exit 1
			rm libpcre2-32.a || exit 1
			rm libpcre2-posix.a || exit 1

			# Выполняем сборку новой статической библиотеки
			ar -crv libpcre2.a $MODULES

			# Выполняем запуск библиотеки
			ranlib libpcre2.a

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
