#!/usr/bin/env bash

##
 # Сборка эталонных реализаций GNU libiconv и GNU libidn2
 #
 # Framework их не использует: перекодировка выполняется модулем awh::charset,
 # а приведение доменных имён — модулем awh::idna. Эталонные реализации нужны
 # лишь стендам сверки в tools/verify, где с ними сличается наша реализация,
 # и в сборку библиотеки не входят. Собирать их поэтому не обязательно, и на
 # операционных системах, где их сборка не проходит, её можно опустить.
 #
 # Скрипт запускается вручную: составом sh/build_third_party.sh он не вызывается.
 ##

# Если корневая дирректория не задана вызывающим скриптом
if [ -z "$ROOT" ]; then
	# Получаем корневую дирректорию скриптов
	readonly ROOT=$(cd "$(dirname "$0")/../third_party" && pwd)
fi

# Если каталог установки не задан вызывающим скриптом
if [ -z "$PREFIX" ]; then
	# Получаем каталог установки собранных зависимостей
	readonly PREFIX=$(cd "$(dirname "$0")/../.." && pwd)/third_party
fi

# Если команда указана
if [ -n "$1" ]; then
	# Если необходимо удалить или очистить модуль
	if [ $1 = "--clean" ]; then
		# Если операционная система не является Windows
		if [[ ! $OS = "Windows" ]]; then
			# Удаляем сборочную дирректорию LibIconv
			rm -rf "$ROOT/../submodules/libiconv"
			# Удаляем сборочную дирректорию LibIDN2
			rm -rf "$ROOT/../submodules/libidn2"
			# Удаляем все зависимости библиотеки
			rm -f "$PREFIX/lib/libdependence.a"
			# Удаляем все зависимые заголовки библиотеки LibIDN2
			rm -rf "$PREFIX/include/idn2"
			# Удаляем все зависимые заголовки библиотеки LibIconv
			rm -rf "$PREFIX/include/iconv"
		fi

		# Переходим обратно в рабочий каталог
		cd "$ROOT" || exit 1
	# Если необходимо собрать проект
	elif [ $1 = "--build" ] || [ $1 = "--update" ]; then
		# Сборка LibIconv
		src="$ROOT/../submodules/libiconv"
		if [ ! -f "$src/.stamp_done" ]; then
			printf "\n****** ANYKS LibIconv ******\n"
			cd "$ROOT/../submodules" || exit 1

			# Выполняем копирование архива исходников LibIconv
			cp "$ROOT/tar/libiconv-1.17.tar.gz" "$ROOT/../submodules/libiconv.tar.gz"

			# Если архив с исходниками получен
			if [ -f "$ROOT/../submodules/libiconv.tar.gz" ]; then
				# Выполняем распаковку архива с исходниками
				tar -xzvf "$ROOT/../submodules/libiconv.tar.gz"
				# Выполняем переименование каталога
				mv "$ROOT/../submodules/libiconv-1.17" "$src"
				# Удаляем уже ненужный архив
				rm "$ROOT/../submodules/libiconv.tar.gz"
				# Переходим в каталог сборки
				cd "$src" || exit 1

				# Если операционной системой является Windows
				if [ $OS = "Windows" ]; then # Windows
					# Выполняем конфигурацию модуля
					./configure \
					 --enable-static="ON" \
					 --enable-shared="OFF" \
					 --host=x86_64-w64-mingw32 \
					 --prefix=$PREFIX \
					 --includedir="$PREFIX/include/iconv" \
					 CC=x86_64-w64-mingw32-gcc \
					 CPPFLAGS="-I/usr/local/mingw64/include -Wall" \
					 LDFLAGS="-L/usr/local/mingw64/lib"
				# Для всех остальных версий операционных систем
				else
					# Выполняем конфигурацию модуля
					./configure \
					 --enable-static="ON" \
					 --enable-shared="OFF" \
					 --prefix=$PREFIX \
					 --includedir="$PREFIX/include/iconv"
				fi

				# Выполняем сборку проекта
				$MAKE -j"$numproc" || exit 1
				# Выполняем установку проекта
				$MAKE install || exit 1

				# Выполняем компенсацию каталогов
				restorelibs $PREFIX

				# Помечаем флагом, что сборка и установка произведена
				touch "$src/.stamp_done"
				cd "$ROOT" || exit 1
			# Если архив с исходниками не скачен
			else
				# Выводим сообщение предупреждения
				echo "DOWNLOAD libiconv sources failed"
			fi

			# Помечаем флагом, что сборка и установка произведена
			touch "$src/.stamp_done"

			# Переходим обратно в рабочий каталог
			cd "$ROOT" || exit 1
		fi
		# Сборка IDN2
		src="$ROOT/../submodules/libidn2"
		if [ ! -f "$src/.stamp_done" ]; then
			printf "\n****** IDN2 ******\n"
			cd "$ROOT/../submodules" || exit 1

			# Выполняем копирование архива исходников LibIDN
			cp "$ROOT/tar/libidn2-2.3.3.tar.gz" "$ROOT/../submodules/libidn2.tar.gz"

			# Если архив с исходниками получен
			if [ -f "$ROOT/../submodules/libidn2.tar.gz" ]; then
				# Выполняем распаковку архива с исходниками
				tar -xzvf "$ROOT/../submodules/libidn2.tar.gz"
				# Выполняем переименование каталога
				mv "$ROOT/../submodules/libidn2-2.3.3" "$src"
				# Удаляем уже ненужный архив
				rm "$ROOT/../submodules/libidn2.tar.gz"
				# Переходим в каталог сборки
				cd "$src" || exit 1

				# Если операционной системой является Solaris
				if [[ $OS = "SunOS" ]]; then
					# Выполняем патчинг библиотеки для дальнейшей сборки
					gsed -i "s/#ifdef HAVE_SYMVER_ALIAS_SUPPORT/#if 0/g" "$src/lib/puny_encode.c"
					gsed -i "s/#ifdef HAVE_SYMVER_ALIAS_SUPPORT/#if 0/g" "$src/lib/puny_decode.c"
				# Если операционной системой является Linux или Windows
				elif [[ $OS = "Linux" ]] || [[ $OS = "Windows" ]]; then
					# Выполняем патчинг библиотеки для дальнейшей сборки
					sed -i "s/#ifdef HAVE_SYMVER_ALIAS_SUPPORT/#if 0/g" "$src/lib/puny_encode.c"
					sed -i "s/#ifdef HAVE_SYMVER_ALIAS_SUPPORT/#if 0/g" "$src/lib/puny_decode.c"
				# Если операционной системой является macOS, FreeBSD, NetBSD или OpenBSD
				elif [[ $OS = "Darwin" ]] || [[ $OS = "FreeBSD" ]] || [[ $OS = "NetBSD" ]] || [[ $OS = "OpenBSD" ]]; then
					# Выполняем патчинг библиотеки для дальнейшей сборки
					sed -i -e 's!#ifdef HAVE_SYMVER_ALIAS_SUPPORT!#if 0!' "$src/lib/puny_encode.c"
					sed -i -e 's!#ifdef HAVE_SYMVER_ALIAS_SUPPORT!#if 0!' "$src/lib/puny_decode.c"

					# Удаляем временные паразитные файлы
					if [ -f "$src/lib/puny_encode.c-e" ]; then
						rm "$src/lib/puny_encode.c-e"
					fi
					if [ -f "$src/lib/puny_decode.c-e" ]; then
						rm "$src/lib/puny_decode.c-e"
					fi
				fi

				# Выполняем конфигурацию модуля
				./configure \
				 --prefix=$PREFIX \
				 --enable-shared="OFF" \
				 --enable-gtk-doc="OFF" \
				 --enable-gtk-doc-html="OFF" \
				 --enable-gtk-doc-pdf="OFF" \
				 --disable-doc \
				 --includedir="$PREFIX/include/idn2" \
				 --oldincludedir="$PREFIX/include/iconv" \
				 --libdir="$PREFIX/lib"

				# Удаляем неправильную конфигурацию
				sed -e '2088d' "$src/Makefile" > "$src/Makefile.tmp"
				# Восстанавливаем правила сборки
				mv "$src/Makefile.tmp" "$src/Makefile"

				# Выполняем сборку проекта
				$MAKE -j"$numproc" || exit 1
				# Выполняем установку проекта
				$MAKE install || exit 1

				# Выполняем компенсацию каталогов
				restorelibs $PREFIX

				# Помечаем флагом, что сборка и установка произведена
				touch "$src/.stamp_done"

				# Переходим обратно в рабочий каталог
				cd "$ROOT" || exit 1
			# Если архив с исходниками не скачен
			else
				# Выводим сообщение предупреждения
				echo "DOWNLOAD libidn2 sources failed"
			fi
		fi
	fi
fi
