#!/usr/bin/env bash

# Если команда указана
if [ -n "$1" ]; then
	# Если необходимо удалить или очистить модуль
	if [ $1 = "--clean" ]; then
		# Очищаем сабмодуль
		clean_submodule "nghttp2"
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
		rm -rf "$PREFIX/include/nghttp2"

		# Переходим обратно в рабочий каталог
		cd "$ROOT" || exit 1
	# Если необходимо выполнить переключение на указанную ветку
	elif [ $1 = "--switch" ] && [ -n "$2" ]; then
		# Переключение ветки
		src="$ROOT/../submodules/nghttp2"
		printf "\n****** ANYKS NgHttp2 ******\n"
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
		# Сборка NgHttp2
		src="$ROOT/../submodules/nghttp2"
		if [ ! -f "$src/.stamp_done" ]; then
			printf "\n****** ANYKS NgHttp2 ******\n"
			cd "$src" || exit 1

			# Устанавливаем название флага
			FLAG="--v"
			# Устанавливаем название ветки/тега/версии по умолчанию
			NAME="1.67.1"

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

			# Деактивируем режим отладки
			ENABLE_DEBUG="OFF"
			# Деактивируем сборку отладочной информации
			BUILD_TYPE="Release"

			# Если режим отладки активирован
			if [[ $DEBUG = "yes" ]]; then
				# Активируем режим отладки
				ENABLE_DEBUG="ON"
				# Активируем сборку отладочной информации
				BUILD_TYPE="Debug"
			fi

			# Устанавливаем переменные окружения
			export OPENSSL_CFLAGS="-I$PREFIX/include"
			export OPENSSL_INCLUDES="-I$PREFIX/include"
			export OPENSSL_LDFLAGS="-L$PREFIX/lib"
			export OPENSSL_LIBS="-lssl -lcrypto"

			# Выполняем конфигурацию проекта
			if [[ $OS = "Windows" ]]; then
				cmake \
				 -DCMAKE_SYSTEM_NAME="Windows" \
				 -DENABLE_STATIC_CRT="ON" \
				 -DENABLE_APP="OFF" \
				 -DENABLE_DOC="OFF" \
				 -DWITH_MRUBY="OFF" \
				 -DWITH_LIBXML2="OFF" \
				 -DWITH_LIBBPF="OFF" \
				 -DWITH_NEVERBLEED="OFF" \
				 -DENABLE_HTTP3="OFF" \
				 -DENABLE_LIB_ONLY="ON" \
				 -DENABLE_EXAMPLES="OFF" \
				 -DBUILD_STATIC_LIBS="ON" \
				 -DBUILD_SHARED_LIBS="OFF" \
				 -DENABLE_APP_DEFAULT="ON" \
				 -DENABLE_HPACK_TOOLS="OFF" \
				 -DENABLE_DEBUG="$ENABLE_DEBUG" \
				 -DENABLE_WERROR="$ENABLE_DEBUG" \
				 -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
				 -DENABLE_FAILMALLOC="$ENABLE_DEBUG" \
				 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
				 -DOPENSSL_LIBRARIES="$PREFIX/lib" \
				 -DOPENSSL_INCLUDE_DIR="$PREFIX/include" \
				 -G "MSYS Makefiles" \
				 .. || exit 1
			else
				cmake \
				 -DENABLE_APP="OFF" \
				 -DENABLE_DOC="OFF" \
				 -DWITH_MRUBY="OFF" \
				 -DWITH_LIBXML2="OFF" \
				 -DWITH_LIBBPF="OFF" \
				 -DWITH_NEVERBLEED="OFF" \
				 -DENABLE_HTTP3="OFF" \
				 -DENABLE_LIB_ONLY="ON" \
				 -DENABLE_EXAMPLES="OFF" \
				 -DBUILD_STATIC_LIBS="ON" \
				 -DBUILD_SHARED_LIBS="OFF" \
				 -DENABLE_APP_DEFAULT="ON" \
				 -DENABLE_HPACK_TOOLS="OFF" \
				 -DENABLE_DEBUG="$ENABLE_DEBUG" \
				 -DENABLE_WERROR="$ENABLE_DEBUG" \
				 -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
				 -DENABLE_FAILMALLOC="$ENABLE_DEBUG" \
				 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
				 -DOPENSSL_LIBRARIES="$PREFIX/lib" \
				 -DOPENSSL_INCLUDE_DIR="$PREFIX/include" \
				 .. || exit 1
			fi

			# Выполняем сборку на всех логических ядрах
			$MAKE -j"$numproc" || exit 1
			# Выполняем установку проекта
			$MAKE install || exit 1

			# Выполняем компенсацию каталогов
			restorelibs $PREFIX

			# Помечаем флагом, что сборка и установка произведена
			touch "$src/.stamp_done"

			# Переходим обратно в рабочий каталог
			cd "$ROOT" || exit 1
		fi
	fi
fi
