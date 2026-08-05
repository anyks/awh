#!/usr/bin/env bash

##
# Распределитель памяти TcMalloc собирается не везде
#
# Библиотека gperftools обращается к системе вызовом syscall(2), а OpenBSD этот
# вызов убрала целиком - таково их решение по усилению защиты, а не пробел выпуска.
# Собрать её там нельзя, и подменить обращение нечем: вызов убран намеренно.
# Распределитель памяти при этом не пропадает - его задачу берёт на себя
# собственный распределитель OpenBSD, писанный с тем же вниманием к защите
##
if [ "$OS" = "OpenBSD" ]; then
	# Сообщаем о пропуске сборки распределителя памяти
	echo "TcMalloc is not available on OpenBSD: skipping gperftools"
	# Завершаем работу без ошибки
	return 0 2>/dev/null || exit 0
fi

# Если команда указана
if [ -n "$1" ]; then
	# Если необходимо удалить или очистить модуль
	if [ $1 = "--clean" ]; then
		# Очищаем сабмодуль
		clean_submodule "gperftools"
		# Для операционной системы Windows
		if [[ $OS = "Windows" ]]; then
			# Удаляем все зависимости библиотеки
			rm -f "$PREFIX/lib/libcommon.lib"
			rm -f "$PREFIX/lib/libtcmalloc.lib"
			rm -f "$PREFIX/lib/libprofiler.lib"
			rm -f "$PREFIX/lib/libsymbolize.lib"
			rm -f "$PREFIX/lib/libstacktrace.lib"
			rm -f "$PREFIX/lib/liblibbacktrace.lib"
			rm -f "$PREFIX/lib/librun_benchmark.lib"
			rm -f "$PREFIX/lib/libtcmalloc_debug.lib"
			rm -f "$PREFIX/lib/libtcmalloc_minimal.lib"
			rm -f "$PREFIX/lib/liblow_level_alloc.lib"
			rm -f "$PREFIX/lib/libtcmalloc_minimal_debug.lib"
		# Для всех остальных операционных систем
		else
			# Удаляем все зависимости библиотеки
			rm -f "$PREFIX/lib/libcommon.a"
			rm -f "$PREFIX/lib/libtcmalloc.a"
			rm -f "$PREFIX/lib/libprofiler.a"
			rm -f "$PREFIX/lib/libsymbolize.a"
			rm -f "$PREFIX/lib/libstacktrace.a"
			rm -f "$PREFIX/lib/liblibbacktrace.a"
			rm -f "$PREFIX/lib/librun_benchmark.a"
			rm -f "$PREFIX/lib/libtcmalloc_debug.a"
			rm -f "$PREFIX/lib/liblow_level_alloc.a"
			rm -f "$PREFIX/lib/libtcmalloc_minimal.a"
			rm -f "$PREFIX/lib/libtcmalloc_minimal_debug.a"
		fi
		# Удаляем все зависимые заголовки библиотеки
		rm -rf "$PREFIX/include/tcmalloc"

		# Переходим обратно в рабочий каталог
		cd "$ROOT" || exit 1
	# Если необходимо выполнить переключение на указанную ветку
	elif [ $1 = "--switch" ] && [ -n "$2" ]; then
		# Переключение ветки
		src="$ROOT/../submodules/gperftools"
		printf "\n****** ANYKS GPerfTools ******\n"
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
		# Сборка GPerfTools
		src="$ROOT/../submodules/gperftools"
		if [ ! -f "$src/.stamp_done" ]; then
			printf "\n****** ANYKS GPerfTools ******\n"
			cd "$src" || exit 1

			# Устанавливаем название флага
			FLAG="--v"
			# Устанавливаем название ветки/тега/версии по умолчанию
			NAME="2.17.2"

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
				git branch -D gperftools-v${NAME}-branch
				# Выполняем переключение на указанную версию
				git checkout -b gperftools-v${NAME}-branch gperftools-${NAME}

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
			mkdir -p "build_awh" || exit 1
			# Переходим в каталог
			cd "build_awh" || exit 1

			# Удаляем старый файл кэша
			rm -rf "$src/build_awh/CMakeCache.txt"

			# Выполняем конфигурацию проекта
			if [[ $OS = "Windows" ]]; then
				cmake \
				 -DCMAKE_BUILD_TYPE=Release \
				 -DBUILD_TESTING="OFF" \
				 -DBUILD_SHARED_LIBS="OFF" \
				 -DDEFAULT_BUILD_MINIMAL="ON" \
				 -DDEFAULT_BUILD_DEBUGALLOC="OFF" \
				 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
				 -G "MSYS Makefiles" \
				 .. || exit 1
			else
				# Дополнительные ключи настройки
				GPERFTOOLS_EXTRA=""

				# Если операционной системой является SunOS
				if [ $OS = "SunOS" ]; then
					# Измеритель нагрузки на illumos не собирается
					#
					# Заголовок src/getpc-inl.h перебирает способы достать счётчик команд из
					# ucontext_t и ни одного подходящего для illumos не находит: сборка отвечает
					# отказом "'Get' is not a member of ...". Это изъян самой gperftools на
					# системе, ею не поддержанной, и правке с нашей стороны он не подлежит
					#
					# Гасится точечно лишь измеритель: сами tcmalloc и tcmalloc_minimal, ради
					# которых gperftools и собирается, встают целыми - проверено опытом на
					# стенде OpenIndiana. Solaris измеритель собирает, и там он остаётся
					GPERFTOOLS_EXTRA="-DGPERFTOOLS_BUILD_CPU_PROFILER=OFF"
				fi

				cmake \
				 -DBUILD_TESTING="OFF" \
				 -DBUILD_SHARED_LIBS="OFF" \
				 -DDEFAULT_BUILD_MINIMAL="ON" \
				 -DDEFAULT_BUILD_DEBUGALLOC="OFF" \
				 -DCMAKE_BUILD_TYPE=Release \
				 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
				 $GPERFTOOLS_EXTRA \
				 .. || exit 1
			fi

			# Выполняем сборку на всех логических ядрах
			$MAKE -j"$numproc" || exit 1

			# Создаём каталог для заголовочных файлов
			mkdir "$PREFIX/include/tcmalloc" || exit 1

			# Производим установку собранных библиотек
			for i in $(ls "$src/build_awh" | grep ".*\.a$");
			do
				echo "Move \"$src/build_awh/$i\" to \"$PREFIX/lib/$i\""
				cp "$src/build_awh/$i" "$PREFIX/lib/$i" || exit 1
			done

			# Если сборка производится в операционной системе macOS, FreeBSD, NetBSD или OpenBSD
			if [ $OS = "Darwin" ] || [ $OS = "FreeBSD" ] || [ $OS = "NetBSD" ] || [ $OS = "OpenBSD" ]; then
				# Используем find для поиска всех .h файлов и копируем их с воссозданием структуры
				cd "$src/src" || exit 1
				# Выполняем перенос всех заголовочных файлов
				find . -type f -name "*.h" | while IFS= read -r file; do
					# Получаем путь к директории файла (без имени файла)
					dir_path=$(dirname "$file")
					# Создаём такую же поддиректорию в целевом каталоге
					mkdir -p "$PREFIX/include/tcmalloc/$dir_path"
					# Копируем файл
					cp "$file" "$PREFIX/include/tcmalloc/$file"
				done
			# Если сборка производится в операционной системе Solaris
			elif [ $OS = "SunOS" ]; then
				# Используем find для поиска всех заголовочных .h файлов и копируем их с воссозданием структуры
				cd "$src/src" || exit 1
				# Выполняем перенос всех заголовочных файлов
				find . -type f -name "*.h" -print0 | xargs -0 gcp --parents -t "$PREFIX/include/tcmalloc"
			# Если сборка производится в операционной системе Windows или Linux
			elif [ $OS = "Windows" ] || [ $OS = "Linux" ]; then
				# Используем find для поиска всех заголовочных .h файлов и копируем их с воссозданием структуры
				cd "$src/src" || exit 1
				# Выполняем перенос всех заголовочных файлов
				find . -type f -name "*.h" -print0 | xargs -0 cp --parents -t "$PREFIX/include/tcmalloc"
			# Если операционная система не определена
			else
				echo "Operating system not defined"
				exit 1
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
