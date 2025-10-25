#!/usr/bin/env bash

# Если команда указана
if [ -n "$1" ]; then
	# Если необходимо удалить или очистить модуль
	if [ $1 = "--clean" ]; then
		# Очищаем сабмодуль
		clean_submodule "openssl"
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
		rm -rf "$PREFIX/include/openssl"

		# Переходим обратно в рабочий каталог
		cd "$ROOT" || exit 1
	# Если необходимо выполнить переключение на указанную ветку
	elif [ $1 = "--switch" ] && [ -n "$2" ]; then
		# Переключение ветки
		src="$ROOT/../submodules/openssl"
		printf "\n****** OpenSSL ******\n"
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
		# Сборка OpenSSL
		src="$ROOT/../submodules/openssl"
		if [ ! -f "$src/.stamp_done" ]; then
			printf "\n****** OpenSSL ******\n"
			cd "$src" || exit 1

			# Устанавливаем название флага
			FLAG="--v"
			# Устанавливаем название ветки/тега/версии по умолчанию
			NAME="3.6.0"

			# Выполняем удаление все неподходящие зависимости
			rm -rf "$src/fuzz/corpora"/*

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
				# Выполняем жесткое переключение на master
				git reset --hard origin/master
				# Переключаемся на master
				git checkout master
				# Выполняем обновление данных
				git pull origin master
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
				git tag -d openssl-${NAME}
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
				git checkout -b v${NAME}-branch openssl-${NAME}

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

			# Выполняем конфигурацию проекта под Linux или FreeBSD
			if [[ $OS = "Linux" ]] || [[ $OS = "FreeBSD" ]]; then
				# Выполняем конфигурацию проекта
				./Configure \
				 sctp \
				 no-docs \
				 no-apps \
				 no-tests \
				 no-async \
				 no-shared \
				 enable-tfo \
				 --release \
				 --prefix="$PREFIX" \
				 --openssldir="$PREFIX" \
				 -Wl,-rpath,"$PREFIX/lib" || exit 1
			# Выполняем конфигурацию проекта под Windows
			elif [ $OS = "Windows" ]; then
				# Выполняем конфигурацию проекта
				./Configure \
				 mingw64 \
				 no-docs \
				 no-apps \
				 no-tests \
				 no-async \
				 no-shared \
				 --release \
				 --prefix="$PREFIX" \
				 --openssldir="$PREFIX" \
				 -Wl,-rpath,"$PREFIX/lib" || exit 1
			# Выполняем конфигурацию проекта под все остальные операционные системы
			else
				# Выполняем конфигурацию проекта
				./Configure \
				 no-docs \
				 no-apps \
				 no-tests \
				 no-async \
				 no-shared \
				 enable-tfo \
				 --release \
				 --prefix="$PREFIX" \
				 --openssldir="$PREFIX" \
				 -Wl,-rpath,"$PREFIX/lib" || exit 1
			fi

			# Выполняем сборку на всех логических ядрах
			$MAKE -j"$numproc" || exit 1

			# Выполняем установку проекта без документации
			$MAKE install_sw || exit 1
			$MAKE install_ssldirs || exit 1

			# Выполняем компенсацию каталогов
			restorelibs $PREFIX

			# Помечаем флагом, что сборка и установка произведена
			touch "$src/.stamp_done"

			# Переходим обратно в рабочий каталог
			cd "$ROOT" || exit 1
		fi
	fi
fi
