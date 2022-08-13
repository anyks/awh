#!/usr/bin/env bash

# Получаем корневую дирректорию
readonly ROOT=$(cd "$(dirname "$0")" && pwd)

# Получаем версию OS
OS=$(uname -a | awk '{print $1}')

if [[ $OS =~ "MINGW64" ]]; then
	OS="Windows"
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
		clean_submodule "libev"
		clean_submodule "libev-win"
		clean_submodule "brotli"
		clean_submodule "openssl"

		# Удаляем сборочную директорию
		rm -rf "$ROOT/third_party"

		printf "\n****************************************"
		printf "\n************   Success!!!   ************"
		printf "\n****************************************"
		printf "\n\n\n"

		exit 0
	else
		printf "Usage: config [options]\n"
		printf " --clean - Cleaning all submodules and build directory\n"

		exit 1
	fi
fi

# Устанавливаем систему сборки
if [[ $OS = "Windows" ]]; then
	export BUILD="mingw32-make"
else
	export BUILD="make"
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
	numproc=$(sysctl -n hw.logicalcpu)
elif [ $OS = "FreeBSD" ]; then
	numproc=$(sysctl -n hw.ncpu)
else
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

	# Версия OpenSSL
	ver="p"

	# Переключаемся на master
	git checkout master
	# Закачиваем все теги
	git fetch --all --tags
	# Удаляем старую ветку
	git branch -D v1.1.1${ver}-branch
	# Выполняем переключение на указанную версию
	git checkout tags/OpenSSL_1_1_1${ver} -b v1.1.1${ver}-branch

	# Выполняем конфигурацию проекта под Linux или FreeBSD
	if [[ $OS = "Linux" ]] || [[ $OS = "FreeBSD" ]]; then
		# Выполняем конфигурацию проекта
		./config \
		 sctp \
		 no-async \
		 no-shared \
		 --prefix="$PREFIX" \
		 --openssldir="$PREFIX" \
		 -Wl,-rpath,"$PREFIX/lib" || exit 1
	# Выполняем конфигурацию проекта под все остальные операционные системы
	else
		# Выполняем конфигурацию проекта
		./config \
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
export OPENSSL_CFLAGS="-I$BUILD/openssl/include"
export OPENSSL_LIBS="-L$BUILD/openssl/lib -lssl -lcrypto"

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
	echo "Install \"$ROOT/submodules/brotli/${build}/libbrotlicommon-static.a\" to \"$PREFIX/lib/libbrotlicommon-static.a\""
	${INSTALL_CMD} "$ROOT/submodules/brotli/${build}/libbrotlicommon-static.a" "$PREFIX/lib/libbrotlicommon-static.a" || exit 1

	echo "Install \"$ROOT/submodules/brotli/${build}/libbrotlidec-static.a\" to \"$PREFIX/lib/libbrotlidec-static.a\""
	${INSTALL_CMD} "$ROOT/submodules/brotli/${build}/libbrotlidec-static.a" "$PREFIX/lib/libbrotlidec-static.a" || exit 1

	echo "Install \"$ROOT/submodules/brotli/${build}/libbrotlienc-static.a\" to \"$PREFIX/lib/libbrotlienc-static.a\""
	${INSTALL_CMD} "$ROOT/submodules/brotli/${build}/libbrotlienc-static.a" "$PREFIX/lib/libbrotlienc-static.a" || exit 1

	# Производим установку заголовочных файлов по нужному пути
	for i in $(ls "$ROOT/submodules/brotli/c/include/brotli" | grep \\.h$);
	do
		echo "Install \"$ROOT/submodules/brotli/c/include/brotli/$i\" to \"$PREFIX/include/brotli/$i\""
		${INSTALL_CMD} "$ROOT/submodules/brotli/c/include/brotli/$i" "$PREFIX/include/brotli/$i" || exit 1
	done

	# Помечаем флагом, что сборка и установка произведена
	touch "$src/.stamp_done"
	cd "$ROOT" || exit 1
fi

# Если операционной системой является Windows
if [ $OS = "Windows" ]; then # Windows
	# Сборка LibEv под Windows
	src="$ROOT/submodules/libev-win"
	if [ ! -f "$src/.stamp_done" ]; then
		printf "\n****** LibEv ******\n"
		cd "$src" || exit 1

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
		echo "Install \"$ROOT/submodules/libev-win/build/liblibev_static.a\" to \"$PREFIX/lib/libev.a\""
		${INSTALL_CMD} "$ROOT/submodules/libev-win/build/liblibev_static.a" "$PREFIX/lib/libev.a" || exit 1

		# Производим установку заголовочных файлов по нужному пути
		for i in $(ls "$ROOT/submodules/libev-win" | grep \\.h$);
		do
			echo "Install \"$ROOT/submodules/libev-win/$i\" to \"$PREFIX/include/libev/$i\""
			${INSTALL_CMD} "$ROOT/submodules/libev-win/$i" "$PREFIX/include/libev/$i" || exit 1
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
		
		# Выполняем сборку проекта
		$BUILD || exit 1
		# Выполняем установку проекта
		$BUILD install || exit 1

		# Помечаем флагом, что сборка и установка произведена
		touch "$src/.stamp_done"
		cd "$ROOT" || exit 1
	fi
fi

# Переименовываем расширение библиотек для Windows
if [ $OS = "Windows" ]; then # Windows
	for i in $(ls "$PREFIX/lib" | grep .a$);
	do
		mv "$PREFIX/lib/$i" "$PREFIX/lib/$(basename "$i" .a).lib"
	done
fi

printf "\n****************************************"
printf "\n************   Success!!!   ************"
printf "\n****************************************"
printf "\n\n\n"
