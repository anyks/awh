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
			cd "$ROOT" || exit 1
		}

		# Очистка директории
		clean_directory(){
			git clean -dfx "$1"
		}

		# Очищаем подпроекты
		clean_submodule "openssl"
		clean_submodule "zlib"
		clean_submodule "libevent"

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
	export MAKE="mingw32-make"
else
	export MAKE=make
fi

# Каталог для установки собранных библиотек
PREFIX="$ROOT/third_party"
export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig"

# Устанавливаем флаги глобального использования
# export CPPFLAGS=""
export CFLAGS="-I$PREFIX/include"
export CXXFLAGS="$CXXFLAGS -fPIC"

export LDFLAGS="-L$PREFIX/lib"
export LD_LIBRARY_PATH="$PREFIX/lib"

# Определяем количество логических ядер
if [ $OS = "Darwin" ]; then
	numproc=$(sysctl -n hw.logicalcpu)
elif [ $OS = "FreeBSD" ]; then
	numproc=$(sysctl -n hw.ncpu)
else
	numproc=$(nproc)
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

	# Закачиваем все теги
	git fetch --all --tags
	# Выполняем переключение на указанную версию
	git checkout tags/OpenSSL_1_1_1l -b v1.1.1l-branch

	# Выполняем конфигурацию проекта
	./config \
	no-shared no-async \
	--prefix="$PREFIX" \
	--openssldir="$PREFIX" \
	-Wl,-rpath,"$PREFIX/lib"  || exit 1

	# Выполняем сборку на всех логических ядрах
	$MAKE -j"$numproc" || exit 1
	# Выполняем установку проекта без документации
	$MAKE install_sw || exit 1
	$MAKE install_ssldirs || exit 1

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

	# Выполняем конфигурацию проекта
	if [[ $OS = "Windows" ]]; then
		# Создаём каталог сборки
		mkdir -p "build" || exit 1
		# Переходим в каталог
		cd "build" || exit 1

		cmake \
		 -DCMAKE_C_COMPILER=gcc \
		 -DCMAKE_BUILD_TYPE=Release \
		 -DCMAKE_SYSTEM_NAME=Windows \
		 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
		 -DINSTALL_INC_DIR="$PREFIX/include/zlib" \
		 -DBUILD_SHARED_LIBS=NO \
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
	$MAKE -j"$numproc" || exit 1
	# Выполняем установку проекта
	$MAKE install || exit 1

	# Помечаем флагом, что сборка и установка произведена
	touch "$src/.stamp_done"
	cd "$ROOT" || exit 1
fi

# Сборка LibEvent2
src="$ROOT/submodules/libevent"
if [ ! -f "$src/.stamp_done" ]; then
	printf "\n****** LibEvent2 ******\n"

	cd "$src" || exit 1

	apply_patch "libevent" "0001-Correcting-the-error-on-checking-the-pointer.patch"

	# Создаём каталог сборки
	mkdir -p "build" || exit 1
	# Переходим в каталог
	cd "build" || exit 1

	# Выполняем конфигурацию проекта
	if [[ $OS = "Windows" ]]; then
		cmake \
		 -DCMAKE_C_COMPILER=gcc \
		 -DCMAKE_BUILD_TYPE=Release \
		 -DCMAKE_SYSTEM_NAME=Windows \
		 -DEVENT__LIBRARY_TYPE="STATIC" \
		 -DEVENT__DISABLE_DEBUG_MODE="ON" \
		 -DEVENT__DISABLE_BENCHMARK="ON" \
		 -DEVENT__DISABLE_SAMPLES="ON" \
		 -DEVENT__DISABLE_TESTS="ON" \
		 -DEVENT__DISABLE_MBEDTLS="ON" \
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
		 -DEVENT__DISABLE_MBEDTLS="ON" \
		 -DEVENT__DISABLE_THREAD_SUPPORT="ON" \
		 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
		 -DOPENSSL_ROOT_DIR="$PREFIX" \
		 -DOPENSSL_LIBRARIES="$PREFIX/lib" \
		 -DOPENSSL_INCLUDE_DIR="$PREFIX/include" \
		 .. || exit 1
	fi

	 # Выполняем сборку на всех логических ядрах
	$MAKE -j"$numproc" || exit 1
	# Выполняем установку проекта
	$MAKE install || exit 1

	# Помечаем флагом, что сборка и установка произведена
	touch "$src/.stamp_done"
	cd "$ROOT" || exit 1
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
