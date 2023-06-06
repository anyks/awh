#!/usr/bin/env bash

# Получаем корневую дирректорию
readonly ROOT=$(cd "$(dirname "$0")" && pwd)

# Получаем версию OS
OS=$(uname -a | awk '{print $1}')

# Флаг активации модуля IDN
IDN="no"

# Флаг активации сборки LibEvent2
LIBEVENT2="no"

if [[ $OS =~ "MINGW64" ]]; then
	OS="Windows"
fi

# Устанавливаем систему сборки
if [[ $OS = "Windows" ]]; then
	export BUILD="mingw32-make"
else
	export BUILD="make"
fi

if [ -n "$1" ]; then
	if [ $1 = "--clean" ]; then
		# Очистка подпроекта
		clean_submodule(){
			cd "$ROOT/submodules/$1" || exit 1
			git clean -dfx
			git stash
			cd "$ROOT" || exit 1
		}

		# Очистка директории
		clean_directory(){
			git clean -dfx "$1"
		}

		# Очищаем подпроекты
		clean_submodule "zlib"
		clean_submodule "pcre"
		clean_submodule "libev"
		clean_submodule "libev-win"
		clean_submodule "brotli"
		clean_submodule "openssl"
		clean_submodule "libevent"

		# Если операционная система не является Windows
		if [[ ! $OS = "Windows" ]]; then
			# Удаляем сборочную дирректорию LibIconv
			rm -rf "$ROOT/submodules/libiconv"
			# Удаляем сборочную дирректорию LibIDN2
			rm -rf "$ROOT/submodules/libidn2"
		fi

		# Удаляем сборочную директорию
		rm -rf "$ROOT/third_party"

		printf "\n****************************************"
		printf "\n************   Success!!!   ************"
		printf "\n****************************************"
		printf "\n\n\n"

		exit 0
	elif [ $1 = "--idn" ]; then
		IDN="yes"
		if [[ -n "$2" ]] && [[ $2 = "--event2" ]]; then
			LIBEVENT2="yes"
		fi
	elif [ $1 = "--event2" ]; then
		LIBEVENT2="yes"
	else
		printf "Usage: config [options]\n"
		printf " --clean - Cleaning all submodules and build directory\n"

		exit 1
	fi
fi

# Каталог для установки собранных библиотек
PREFIX="$ROOT/third_party"
export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig"

# Устанавливаем флаги глобального использования
# export CPPFLAGS=""
export CXXFLAGS="$CXXFLAGS -fPIC"
export CFLAGS="-I$PREFIX/include -fPIC"

export LDFLAGS="-L$PREFIX/lib"
export LD_LIBRARY_PATH="$PREFIX/lib"

# Создаём каталоги
mkdir -p "$PREFIX/bin"
mkdir -p "$PREFIX/lib"
mkdir -p "$PREFIX/include"

# Определяем количество логических ядер
if [ $OS = "Darwin" ]; then
	# Устанавливаем количество ядер системы
	numproc=$(sysctl -n hw.logicalcpu)
	# Устанавливаем версию операционной системы
	export MACOSX_DEPLOYMENT_TARGET=$(sw_vers -productVersion)
elif [ $OS = "FreeBSD" ]; then
	# Устанавливаем количество ядер системы
	numproc=$(sysctl -n hw.ncpu)
else
	# Устанавливаем количество ядер системы
	numproc=$(nproc)
fi

if [ $OS = "Darwin" ]; then # MacOS
	INSTALL_CMD="ditto -v"
elif [ $OS = "FreeBSD" ]; then # FreeBSD
	INSTALL_CMD="install -m 0644"
	# Создаём каталон назначения заголовочных файлов
	mkdir -p "$PREFIX/include/brotli"
elif [ $OS = "Windows" ]; then # Windows
	INSTALL_CMD="install -D -m 0644"
else # Linux
	INSTALL_CMD="install -D -m 0644"
fi

# Применяем патчи
apply_patch(){
	patch="$ROOT/patches/$1/$2"
	if ! git apply --reverse --check "$patch" 2> /dev/null; then
		echo "applaying patch $patch"
		git apply "$patch" || exit 1
	else
		echo "patch $patch already applied"
	fi
}

# Инициализируем подпроекты
git submodule update --init --recursive --remote

# Сборка OpenSSL
src="$ROOT/submodules/openssl"
if [ ! -f "$src/.stamp_done" ]; then
	printf "\n****** OpenSSL ******\n"
	cd "$src" || exit 1

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
	git branch -D v3.1-branch
	# Выполняем переключение на указанную версию
	git checkout origin/openssl-3.1 -b v3.1-branch

	# Выполняем конфигурацию проекта под Linux или FreeBSD
	if [[ $OS = "Linux" ]] || [[ $OS = "FreeBSD" ]]; then
		# Выполняем конфигурацию проекта
		./Configure \
		 sctp \
		 no-async \
		 no-shared \
		 --prefix="$PREFIX" \
		 --openssldir="$PREFIX" \
		 -Wl,-rpath,"$PREFIX/lib" || exit 1
	# Выполняем конфигурацию проекта под Windows
	elif [ $OS = "Windows" ]; then
		# Выполняем конфигурацию проекта
		./Configure \
		 mingw64 \
		 no-async \
		 no-shared \
		 --prefix="$PREFIX" \
		 --openssldir="$PREFIX" \
		 -Wl,-rpath,"$PREFIX/lib" || exit 1
	# Выполняем конфигурацию проекта под все остальные операционные системы
	else
		# Выполняем конфигурацию проекта
		./Configure \
		 no-async \
		 no-shared \
		 --prefix="$PREFIX" \
		 --openssldir="$PREFIX" \
		 -Wl,-rpath,"$PREFIX/lib" || exit 1
	fi

	# Выполняем сборку на всех логических ядрах
	$BUILD -j"$numproc" || exit 1
	# Выполняем установку проекта без документации
	$BUILD install_sw || exit 1
	$BUILD install_ssldirs || exit 1

	# Помечаем флагом, что сборка и установка произведена
	touch "$src/.stamp_done"
	cd "$ROOT" || exit 1
fi

# Устанавливаем переменные окружения
export OPENSSL_CFLAGS="-I$PREFIX/include"
export OPENSSL_INCLUDES="-I$PREFIX/include"
export OPENSSL_LDFLAGS="-L$PREFIX/lib"
export OPENSSL_LIBS="-lssl -lcrypto"

# Сборка Zlib
src="$ROOT/submodules/zlib"
if [ ! -f "$src/.stamp_done" ]; then
	printf "\n****** ZLib ******\n"
	cd "$src" || exit 1

	# Версия Zlib
	ver="1.2.11"

	# Переключаемся на master
	git checkout master
	# Закачиваем все теги
	git fetch --all --tags
	# Удаляем старую ветку
	git branch -D v${ver}-branch
	# Выполняем переключение на указанную версию
	git checkout tags/v${ver} -b v${ver}-branch

	# Выполняем конфигурацию проекта
	if [[ $OS = "Windows" ]]; then
		# Создаём каталог сборки
		mkdir -p "build" || exit 1
		# Переходим в каталог
		cd "build" || exit 1

		# Удаляем старый файл кэша
		rm -rf ./CMakeCache.txt

		cmake \
		 -DCMAKE_BUILD_TYPE="Release" \
		 -DCMAKE_SYSTEM_NAME="Windows" \
		 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
		 -DINSTALL_INC_DIR="$PREFIX/include/zlib" \
		 -DBUILD_SHARED_LIBS="NO" \
		 -G "MinGW Makefiles" \
		 .. || exit 1
	else
		./configure \
		 --prefix="$PREFIX" \
		 --libdir="$PREFIX/lib" \
		 --includedir="$PREFIX/include/zlib" \
		 --static || exit 1
	fi

	# Выполняем сборку на всех логических ядрах
	$BUILD -j"$numproc" || exit 1
	# Выполняем установку проекта
	$BUILD install || exit 1

	# Помечаем флагом, что сборка и установка произведена
	touch "$src/.stamp_done"
	cd "$ROOT" || exit 1
fi

# Сборка Brotli
src="$ROOT/submodules/brotli"
if [ ! -f "$src/.stamp_done" ]; then
	printf "\n****** Brotli ******\n"
	cd "$src" || exit 1

	# Версия Brotli
	ver="1.0.9"

	# Переключаемся на master
	git checkout master
	# Закачиваем все теги
	git fetch --all --tags
	# Удаляем старую ветку
	git branch -D v${ver}-branch
	# Выполняем переключение на указанную версию
	git checkout tags/v${ver} -b v${ver}-branch

	# Каталог для сборки
	build="out"

	# Создаём каталог сборки
	mkdir -p ${build} || exit 1
	# Переходим в каталог
	cd ${build} || exit 1

	# Удаляем старый файл кэша
	rm -rf ./CMakeCache.txt

	# Выполняем конфигурацию проекта
	if [[ $OS = "Windows" ]]; then
		cmake \
		 -DCMAKE_BUILD_TYPE="Release" \
		 -DCMAKE_SYSTEM_NAME="Windows" \
		 -DBROTLI_EMSCRIPTEN="YES" \
		 -DBROTLI_DISABLE_TESTS="YES" \
		 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
		 -DBROTLI_LIBRARIES="$PREFIX/lib" \
		 -DBROTLI_INCLUDE_DIRS="$PREFIX/include" \
		 -DBUILD_SHARED_LIBS="NO" \
		 -G "MinGW Makefiles" \
		 .. || exit 1
	else
		cmake \
		 -DCMAKE_BUILD_TYPE="Release" \
		 -DBROTLI_EMSCRIPTEN="YES" \
		 -DBROTLI_DISABLE_TESTS="YES" \
		 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
		 -DBROTLI_LIBRARIES="$PREFIX/lib" \
		 -DBROTLI_INCLUDE_DIRS="$PREFIX/include" \
		 -DBUILD_SHARED_LIBS="NO" \
		 .. || exit 1
	fi

	# Выполняем сборку на всех логических ядрах
	$BUILD -j"$numproc" || exit 1

	# Производим установку библиотеки по нужному пути
	echo "Install \"$src/${build}/libbrotlicommon-static.a\" to \"$PREFIX/lib/libbrotlicommon-static.a\""
	${INSTALL_CMD} "$src/${build}/libbrotlicommon-static.a" "$PREFIX/lib/libbrotlicommon-static.a" || exit 1

	echo "Install \"$src/${build}/libbrotlidec-static.a\" to \"$PREFIX/lib/libbrotlidec-static.a\""
	${INSTALL_CMD} "$src/${build}/libbrotlidec-static.a" "$PREFIX/lib/libbrotlidec-static.a" || exit 1

	echo "Install \"$src/${build}/libbrotlienc-static.a\" to \"$PREFIX/lib/libbrotlienc-static.a\""
	${INSTALL_CMD} "$src/${build}/libbrotlienc-static.a" "$PREFIX/lib/libbrotlienc-static.a" || exit 1

	# Производим установку заголовочных файлов по нужному пути
	for i in $(ls "$src/c/include/brotli" | grep \\.h$);
	do
		echo "Install \"$src/c/include/brotli/$i\" to \"$PREFIX/include/brotli/$i\""
		${INSTALL_CMD} "$src/c/include/brotli/$i" "$PREFIX/include/brotli/$i" || exit 1
	done

	# Помечаем флагом, что сборка и установка произведена
	touch "$src/.stamp_done"
	cd "$ROOT" || exit 1
fi

# Если нужно собрать модуль LibEvent2
if [[ $LIBEVENT2 = "yes" ]]; then
	# Сборка LibEvent2
	src="$ROOT/submodules/libevent"
	if [ ! -f "$src/.stamp_done" ]; then
		printf "\n****** LibEvent2 ******\n"
		cd "$src" || exit 1

		# Версия LibEvent2
		ver="2.1.12"

		# Переключаемся на master
		git checkout master
		# Закачиваем все теги
		git fetch --all --tags
		# Удаляем старую ветку
		git branch -D v${ver}-branch
		# Выполняем переключение на указанную версию
		git checkout tags/release-${ver}-stable -b v${ver}-branch

		# Создаём каталог сборки
		mkdir -p "build" || exit 1
		# Переходим в каталог
		cd "build" || exit 1

		# Удаляем старый файл кэша
		rm -rf ./CMakeCache.txt

		# Выполняем конфигурацию проекта
		if [[ $OS = "Windows" ]]; then
			cmake \
			 -DCMAKE_C_COMPILER="gcc" \
			 -DCMAKE_BUILD_TYPE="Release" \
			 -DCMAKE_SYSTEM_NAME="Windows" \
			 -DEVENT__LIBRARY_TYPE="STATIC" \
			 -DEVENT__DISABLE_DEBUG_MODE="ON" \
			 -DEVENT__DISABLE_BENCHMARK="ON" \
			 -DEVENT__DISABLE_SAMPLES="ON" \
			 -DEVENT__DISABLE_TESTS="ON" \
			 -DEVENT__DISABLE_THREAD_SUPPORT="ON" \
			 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
			 -DOPENSSL_ROOT_DIR="$PREFIX" \
			 -DOPENSSL_LIBRARIES="$PREFIX/lib" \
			 -DOPENSSL_INCLUDE_DIR="$PREFIX/include" \
			 -G "MinGW Makefiles" \
			 .. || exit 1
		else
			cmake \
			 -DEVENT__LIBRARY_TYPE="STATIC" \
			 -DEVENT__DISABLE_DEBUG_MODE="ON" \
			 -DEVENT__DISABLE_BENCHMARK="ON" \
			 -DEVENT__DISABLE_SAMPLES="ON" \
			 -DEVENT__DISABLE_TESTS="ON" \
			 -DEVENT__DISABLE_THREAD_SUPPORT="ON" \
			 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
			 -DOPENSSL_ROOT_DIR="$PREFIX" \
			 -DOPENSSL_LIBRARIES="$PREFIX/lib" \
			 -DOPENSSL_INCLUDE_DIR="$PREFIX/include" \
			 .. || exit 1
		fi

		# Выполняем сборку на всех логических ядрах
		$BUILD -j"$numproc" || exit 1
		# Выполняем установку проекта
		$BUILD install || exit 1

		# Помечаем флагом, что сборка и установка произведена
		touch "$src/.stamp_done"
		cd "$ROOT" || exit 1
	fi
# Если нужно собрать модуль LibEv
else
	# Если операционной системой является Windows
	if [ $OS = "Windows" ]; then # Windows
		# Сборка LibEv под Windows
		src="$ROOT/submodules/libev-win"
		if [ ! -f "$src/.stamp_done" ]; then
			printf "\n****** LibEv ******\n"
			cd "$src" || exit 1

			# Применяем патч
			apply_patch "libev-win" "libev.patch"

			# Создаём каталог сборки
			mkdir -p "build" || exit 1
			# Переходим в каталог
			cd "build" || exit 1

			# Удаляем старый файл кэша
			rm -rf ./CMakeCache.txt

			# Выполняем конфигурацию проекта
			cmake \
			 -DCMAKE_BUILD_TYPE="Release" \
			 -DCMAKE_SYSTEM_NAME="Windows" \
			 -G "MinGW Makefiles" \
			 .. || exit 1

			# Выполняем сборку на всех логических ядрах
			$BUILD -j"$numproc" || exit 1

			# Производим установку библиотеки по нужному пути
			echo "Install \"$src/build/liblibev_static.a\" to \"$PREFIX/lib/libev.a\""
			${INSTALL_CMD} "$src/build/liblibev_static.a" "$PREFIX/lib/libev.a" || exit 1

			# Производим установку заголовочных файлов по нужному пути
			for i in $(ls "$src" | grep \\.h$);
			do
				echo "Install \"$src/$i\" to \"$PREFIX/include/libev/$i\""
				${INSTALL_CMD} "$src/$i" "$PREFIX/include/libev/$i" || exit 1
			done

			# Помечаем флагом, что сборка и установка произведена
			touch "$src/.stamp_done"
			cd "$ROOT" || exit 1
		fi
	# Для всех остальных версий операционных систем
	else
		# Сборка LibEv
		src="$ROOT/submodules/libev"
		if [ ! -f "$src/.stamp_done" ]; then
			printf "\n****** LibEv ******\n"
			cd "$src" || exit 1

			# Выполняем конфигурирование сборки
			./configure \
			 --with-pic=use \
			 --enable-static=yes \
			 --enable-shared=no \
			 --prefix=$PREFIX \
			 --includedir="$PREFIX/include/libev" \
			 --libdir="$PREFIX/lib"

			# Применяем патч
			apply_patch "libev" "libev.patch"
			
			# Выполняем сборку проекта
			$BUILD -j"$numproc" || exit 1
			# Выполняем установку проекта
			$BUILD install || exit 1

			# Помечаем флагом, что сборка и установка произведена
			touch "$src/.stamp_done"
			cd "$ROOT" || exit 1
		fi
	fi
fi

# Если нужно собрать модуль IDN и операционная система не является Windows
if [[ $IDN = "yes" ]] && [[ ! $OS = "Windows" ]]; then
	# Сборка ICONV
	src="$ROOT/submodules/libiconv"
	if [ ! -f "$src/.stamp_done" ]; then
		printf "\n****** ICONV ******\n"
		cd "$ROOT/submodules" || exit 1

		# Выполняем закачку архива исходников LibIconv
		curl -o "$ROOT/submodules/libiconv.tar.gz" "https://ftp.gnu.org/pub/gnu/libiconv/libiconv-1.17.tar.gz"

		# Если архив с исходниками получен
		if [ -f "$ROOT/submodules/libiconv.tar.gz" ]; then
			# Выполняем распаковку архива с исходниками
			tar -xzvf "$ROOT/submodules/libiconv.tar.gz"
			# Выполняем переименование каталога
			mv "$ROOT/submodules/libiconv-1.17" "$src"
			# Удаляем уже ненужный архив
			rm "$ROOT/submodules/libiconv.tar.gz"
			# Переходим в каталог сборки
			cd "$src" || exit 1

			# Если операционной системой является Windows
			if [ $OS = "Windows" ]; then # Windows
				# Выполняем конфигурацию модуля
				./configure \
				 --enable-static=yes \
				 --enable-shared=no \
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
				 --enable-static=yes \
				 --enable-shared=no \
				 --prefix=$PREFIX \
				 --includedir="$PREFIX/include/iconv"
			fi

			# Выполняем сборку проекта
			$BUILD -j"$numproc" || exit 1
			# Выполняем установку проекта
			$BUILD install || exit 1

			# Помечаем флагом, что сборка и установка произведена
			touch "$src/.stamp_done"
			cd "$ROOT" || exit 1
		# Если архив с исходниками не скачен
		else
			# Выводим сообщение предупреждения
			echo "DOWNLOAD libiconv sources failed"
		fi
	fi
	# Сборка IDN2
	src="$ROOT/submodules/libidn2"
	if [ ! -f "$src/.stamp_done" ]; then
		printf "\n****** IDN2 ******\n"
		cd "$ROOT/submodules" || exit 1

		# Выполняем закачку архива исходников LibIconv
		curl -o "$ROOT/submodules/libidn2.tar.gz" "https://ftp.gnu.org/gnu/libidn/libidn2-2.3.3.tar.gz"

		# Если архив с исходниками получен
		if [ -f "$ROOT/submodules/libidn2.tar.gz" ]; then
			# Выполняем распаковку архива с исходниками
			tar -xzvf "$ROOT/submodules/libidn2.tar.gz"
			# Выполняем переименование каталога
			mv "$ROOT/submodules/libidn2-2.3.3" "$src"
			# Удаляем уже ненужный архив
			rm "$ROOT/submodules/libidn2.tar.gz"
			# Переходим в каталог сборки
			cd "$src" || exit 1

			# Выполняем конфигурацию модуля
			./configure \
			 --prefix=$PREFIX \
			 --enable-shared=no \
			 --enable-gtk-doc=no \
			 --enable-gtk-doc-html=no \
			 --enable-gtk-doc-pdf=no \
			 --disable-doc \
			 --includedir="$PREFIX/include/idn2" \
			 --oldincludedir="$PREFIX/include/iconv" \
			 --libdir="$PREFIX/lib"

			# Выполняем сборку проекта
			$BUILD -j"$numproc" || exit 1
			# Выполняем установку проекта
			$BUILD install || exit 1

			# Помечаем флагом, что сборка и установка произведена
			touch "$src/.stamp_done"
			cd "$ROOT" || exit 1
		# Если архив с исходниками не скачен
		else
			# Выводим сообщение предупреждения
			echo "DOWNLOAD libidn2 sources failed"
		fi
	fi
fi

# Выполняем конфигурацию проекта
if [[ ! $OS = "Windows" ]]; then
	# Сборка PCRE
	src="$ROOT/submodules/pcre"
	if [ ! -f "$src/.stamp_done" ]; then
		printf "\n****** PCRE ******\n"
		cd "$src" || exit 1

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
			-DPCRE_BUILD_TESTS=NO \
			-DPCRE_BUILD_PCRECPP=YES \
			-DPCRE_SUPPORT_UTF=YES \
			-DPCRE_SUPPORT_JIT=NO \
			-DPCRE_SUPPORT_LIBZ=NO \
			-DPCRE_SUPPORT_LIBBZ2=NO \
			-DPCRE_SUPPORT_LIBEDIT=NO \
			-DPCRE_SUPPORT_LIBREADLINE=NO \
			-DPCRE_SUPPORT_UNICODE_PROPERTIES=YES \
			-DCMAKE_INSTALL_PREFIX="$PREFIX" \
			-G "MinGW Makefiles" \
			.. || exit 1
		else
			cmake \
			-DCMAKE_BUILD_TYPE=Release \
			-DPCRE_BUILD_TESTS=NO \
			-DPCRE_BUILD_PCRECPP=YES \
			-DPCRE_SUPPORT_UTF=YES \
			-DPCRE_SUPPORT_JIT=NO \
			-DPCRE_SUPPORT_LIBZ=NO \
			-DPCRE_SUPPORT_LIBBZ2=NO \
			-DPCRE_SUPPORT_LIBEDIT=NO \
			-DPCRE_SUPPORT_LIBREADLINE=NO \
			-DPCRE_SUPPORT_UNICODE_PROPERTIES=YES \
			-DCMAKE_INSTALL_PREFIX="$PREFIX" \
			.. || exit 1
		fi

		# Выполняем сборку на всех логических ядрах
		$BUILD -j"$numproc" || exit 1
		# Выполняем установку проекта
		$BUILD install || exit 1

		# Создаём каталог PCRE
		mkdir "$PREFIX/include/pcre"

		# Производим установку заголовочных файлов по нужному пути
		for i in $(ls "$PREFIX/include" | grep "pcre.*\.h$");
		do
			echo "Move \"$PREFIX/include/$i\" to \"$PREFIX/include/pcre/$i\""
			mv "$PREFIX/include/$i" "$PREFIX/include/pcre/$i" || exit 1
		done

		# Помечаем флагом, что сборка и установка произведена
		touch "$src/.stamp_done"
		cd "$ROOT" || exit 1
	fi
fi

# Переименовываем расширение библиотек для Windows
if [ $OS = "Windows" ]; then # Windows
	# Выполняем исправление библиотек для x32
	for i in $(ls "$PREFIX/lib" | grep .a$);
	do
		mv "$PREFIX/lib/$i" "$PREFIX/lib/$(basename "$i" .a).lib"
	done
	# Выполняем исправление библиотек для x64
	for i in $(ls "$PREFIX/lib64" | grep .a$);
	do
		mv "$PREFIX/lib64/$i" "$PREFIX/lib64/$(basename "$i" .a).lib"
	done
fi

printf "\n****************************************"
printf "\n************   Success!!!   ************"
printf "\n****************************************"
printf "\n\n\n"
