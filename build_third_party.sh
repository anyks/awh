#!/usr/bin/env bash

# Получаем корневую дирректорию
readonly ROOT=$(cd "$(dirname "$0")" && pwd)

# Получаем версию OS
OS=$(uname -a | awk '{print $1}')

# Флаг активации модуля IDN
IDN="no"

# Флаг активации режима отладки
DEBUG="no"

# Компенсируем название OS Windows
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

		# Очищаем подпроекты компрессоров
		clean_submodule "bz2"
		clean_submodule "lz4"
		clean_submodule "zlib"
		clean_submodule "zstd"
		clean_submodule "lzma"
		clean_submodule "brotli"
		# Очищаем подпроекты
		clean_submodule "pcre2"
		clean_submodule "openssl"
		clean_submodule "nghttp2"
		clean_submodule "gperftools"

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

	elif [ $1 = "--debug" ]; then

		DEBUG="yes"

		if [[ -n "$2" ]] && [[ $2 = "--idn" ]]; then
			IDN="yes"
		fi
		if [[ -n "$3" ]] && [[ $3 = "--idn" ]]; then
			IDN="yes"
		fi

	elif [ $1 = "--idn" ]; then

		IDN="yes"

		if [[ -n "$2" ]] && [[ $2 = "--debug" ]]; then
			DEBUG="yes"
		fi
		if [[ -n "$3" ]] && [[ $3 = "--debug" ]]; then
			DEBUG="yes"
		fi

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
export CFLAGS="$CFLAGS -I$PREFIX/include -fPIC"

export LDFLAGS="$LDFLAGS -L$PREFIX/lib"
export LD_LIBRARY_PATH="$PREFIX/lib"

# Устанавливаем минимальную версию CMake
export CMAKE_POLICY_VERSION_MINIMUM=3.5

# Создаём каталоги
mkdir -p "$PREFIX/bin"
mkdir -p "$PREFIX/lib"
mkdir -p "$PREFIX/include"

# Если операционная система используется Solaris
if [ $OS = "SunOS" ]; then
	# Устанавливаем жёстко компилятор
	export CC="gcc"
# Если операционная система используется Linux
elif [ $OS = "Linux" ] && [ ! -n "$CC" ]; then
	# Получаем адрес расположения компилятора
	clang=$(whereis clang | awk '{print $2}')
	# Если компилятор clang не установлен
	if [ ! -n "$clang" ]; then
		# Устанавливаем жёстко компилятор
		export CC="gcc"
	# Если же компилятор clang установлен
	else
		# Устанавливаем жёстко компилятор
		export CC="clang"
	fi
fi

# Если сборка производится в операционной системе MacOS X
if [ $OS = "Darwin" ]; then
	# Устанавливаем количество ядер системы
	numproc=$(sysctl -n hw.logicalcpu)
	# Если версия MacOS X не установлена
	if [ ! -n "$MACOSX_DEPLOYMENT_TARGET" ]; then
		# Устанавливаем версию операционной системы
		export MACOSX_DEPLOYMENT_TARGET=$(sw_vers -productVersion)
	fi
# Если сборка производится в операционной системе Windows, Linux или Solaris
elif [ $OS = "Windows" ] || [ $OS = "Linux" ] || [ $OS = "SunOS" ]; then
	# Устанавливаем количество ядер системы
	numproc=$(nproc)
# Если сборка производится в операционной системе FreeBSD, NetBSD или OpenBSD
elif [ $OS = "FreeBSD" ] || [ $OS = "NetBSD" ] || [ $OS = "OpenBSD" ]; then
	# Устанавливаем количество ядер системы
	numproc=$(sysctl -n hw.ncpu)
# Если операционная система не определена
else
	echo "Operating system not defined"
	exit 1
fi

# Если сборка производится в операционной системе MacOS X
if [ $OS = "Darwin" ]; then
	INSTALL_CMD="ditto -v"
# Если сборка производится в операционной системе Windows, Linux или Solaris
elif [ $OS = "Windows" ] || [ $OS = "Linux" ] || [ $OS = "SunOS" ]; then
	INSTALL_CMD="install -D -m 0644"
# Если сборка производится в операционной системе FreeBSD, NetBSD или OpenBSD
elif [ $OS = "FreeBSD" ] || [ $OS = "NetBSD" ] || [ $OS = "OpenBSD" ]; then
	INSTALL_CMD="install -m 0644"
# Если операционная система не определена
else
	echo "Operating system not defined"
	exit 1
fi

# Если сборка производится в операционной системе FreeBSD, NetBSD, OpenBSD или Solaris
if [ $OS = "FreeBSD" ] || [ $OS = "NetBSD" ] || [ $OS = "OpenBSD" ] || [ $OS = "SunOS" ]; then
	# Устанавливаем сборщик
	MAKE="gmake"
# Если сборка производится в другой операционной системе
else
	# Устанавливаем сборщик
	MAKE="make"
fi

# Функция применения патча
apply_patch(){
	PATCH="$ROOT/patches/$1/$2"
	if ! git apply --reverse --check "$PATCH" 2> /dev/null; then
		echo "applaying patch $PATCH"
		git apply "$PATCH" || exit 1
	else
		echo "patch $PATCH already applied"
	fi
}

# Фукция компенсации неверных каталогов
restorelibs(){
	# Если сборка производится в операционной системе Linux
	if [ $OS = "Windows" ] || [ $OS = "Linux" ]; then
		# Если на вход получен каталог
		if [[ -d "$1/lib64" ]]; then
			# Переносим всё что есть в каталоге, в нужный нам каталог
			for i in $(ls "$1/lib64");
			do
				# Если файла нет в каталоге
				if [[ ! -f "$1/lib/$i" ]] && [[ -f "$1/lib64/$i" ]]; then
					echo "Move \"$1/lib64/$i\" to \"$1/lib/$i\""
					mv "$1/lib64/$i" "$1/lib/$i" || exit 1
				fi
			done
			# Удаляем ненужный нам каталог
			rm -rf "$1/lib64" || exit 1
		fi
	# Если сборка производится в операционной системе Solaris
	elif [ $OS = "SunOS" ]; then
		# Если на вход получен каталог
		if [[ -d "$1/lib/64" ]]; then
			# Переносим всё что есть в каталоге, в нужный нам каталог
			for i in $(ls "$1/lib/64");
			do
				# Если файла нет в каталоге
				if [[ ! -f "$1/lib/$i" ]] && [[ -f "$1/lib/64/$i" ]]; then
					echo "Move \"$1/lib/64/$i\" to \"$1/lib/$i\""
					mv "$1/lib/64/$i" "$1/lib/$i" || exit 1
				fi
			done
			# Удаляем ненужный нам каталог
			rm -rf "$1/lib/64" || exit 1
		fi
	fi
}

# Выполняем синхронизацию сабмодулей
git submodule sync
# Инициализируем подпроекты
git submodule update --init --recursive

# Сборка OpenSSL
src="$ROOT/submodules/openssl"
if [ ! -f "$src/.stamp_done" ]; then
	printf "\n****** OpenSSL ******\n"
	cd "$src" || exit 1

	# Версия OpenSSL v3.5.0
	VER="3.5.0"

	# Выполняем удаление все неподходящие зависимости
	rm -rf "$src/fuzz/corpora"/*

	# Выполняем удаление предыдущей закаченной версии
	git tag -d openssl-${VER}
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
	git branch -D v${VER}-branch
	# Выполняем переключение на указанную версию
	git checkout -b v${VER}-branch openssl-${VER}

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
	VER="1.3.1"

	# Переключаемся на master
	git checkout master
	# Выполняем удаление предыдущей закаченной версии
	git tag -d v${VER}
	# Закачиваем все теги
	git fetch --all --tags
	# Удаляем старую ветку
	git branch -D v${VER}-branch
	# Выполняем переключение на указанную версию
	git checkout -b v${VER}-branch v${VER}

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
		 -G "MSYS Makefiles" \
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

	# Выполняем компенсацию каталогов
	restorelibs $PREFIX

	# Если операционной системой является Windows
	if [ $OS = "Windows" ]; then
		# Удаляем лишние библиотеки
		rm "$PREFIX/lib/libzlib.dll"
		rm "$PREFIX/lib/libzlib.dll.a"
	fi

	# Помечаем флагом, что сборка и установка произведена
	touch "$src/.stamp_done"
	cd "$ROOT" || exit 1
fi

# Сборка BZip2
src="$ROOT/submodules/bz2"
if [ ! -f "$src/.stamp_done" ]; then
	printf "\n****** BZip2 ******\n"
	cd "$src" || exit 1

	# Версия BZip2
	# VER="1.0.8"

	# Выполняем удаление предыдущей закаченной версии
	#git tag -d bzip2-${VER}
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
	#git branch -D v${VER}-branch
	# Выполняем переключение на указанную версию
	#git checkout -b v${VER}-branch bzip2-${VER}

	# Создаём каталог сборки
	mkdir -p "build" || exit 1
	# Переходим в каталог
	cd "build" || exit 1

	# Удаляем старый файл кэша
	rm -rf "$src/build/CMakeCache.txt"

	# Выполняем конфигурацию проекта
	if [[ $OS = "Windows" ]]; then
		cmake \
		 -DENABLE_LIB_ONLY="ON" \
		 -DENABLE_DOCS="OFF" \
		 -DENABLE_TESTS="OFF" \
		 -DENABLE_DEBUG="OFF" \
		 -DENABLE_SHARED_LIB="OFF" \
		 -DENABLE_STATIC_LIB="ON" \
		 -DCMAKE_BUILD_TYPE=Release \
		 -DCMAKE_SYSTEM_NAME=Windows \
		 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
		 -G "MSYS Makefiles" \
		 .. || exit 1
	else
		cmake \
		 -DENABLE_LIB_ONLY="ON" \
		 -DENABLE_DOCS="OFF" \
		 -DENABLE_TESTS="OFF" \
		 -DENABLE_DEBUG="OFF" \
		 -DENABLE_SHARED_LIB="OFF" \
		 -DENABLE_STATIC_LIB="ON" \
		 -DCMAKE_BUILD_TYPE=Release \
		 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
		 .. || exit 1
	fi

	# Выполняем сборку на всех логических ядрах
	$MAKE -j"$numproc" || exit 1
	# Выполняем установку проекта
	$MAKE install || exit 1

	# Выполняем компенсацию каталогов
	restorelibs $PREFIX

	# Создаём каталог BZip2
	mkdir "$PREFIX/include/bz2"

	# Производим установку заголовочных файлов по нужному пути
	for i in $(ls "$PREFIX/include" | grep ".*\.h$");
	do
		echo "Move \"$PREFIX/include/$i\" to \"$PREFIX/include/bz2/$i\""
		mv "$PREFIX/include/$i" "$PREFIX/include/bz2/$i" || exit 1
	done

	# Помечаем флагом, что сборка и установка произведена
	touch "$src/.stamp_done"
	cd "$ROOT" || exit 1
fi

# Сборка LZ4
src="$ROOT/submodules/lz4"
if [ ! -f "$src/.stamp_done" ]; then
	printf "\n****** LZ4 ******\n"
	cd "$src/build/cmake" || exit 1

	# Версия LZ4
	VER="1.10.0"

	# Выполняем удаление предыдущей закаченной версии
	git tag -d v${VER}
	# Закачиваем все изменения
	git fetch --all
	# Закачиваем все теги
	git fetch --all --tags
	# Выполняем жесткое переключение на release
	git reset --hard origin/release
	# Переключаемся на release
	git checkout release
	# Выполняем обновление данных
	git pull origin release
	# Удаляем старую ветку
	git branch -D v${VER}-branch
	# Выполняем переключение на указанную версию
	git checkout -b v${VER}-branch v${VER}

	# Создаём каталог сборки
	mkdir -p "build" || exit 1
	# Переходим в каталог
	cd "build" || exit 1

	# Удаляем старый файл кэша
	rm -rf "$src/build/cmake/build/CMakeCache.txt"

	# Выполняем конфигурацию проекта
	if [[ $OS = "Windows" ]]; then
		cmake \
		 -DBUILD_SHARED_LIBS="OFF" \
		 -DBUILD_STATIC_LIBS="ON" \
		 -DCMAKE_BUILD_TYPE=Release \
		 -DCMAKE_SYSTEM_NAME=Windows \
		 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
		 -G "MSYS Makefiles" \
		 .. || exit 1
	else
		cmake \
		 -DBUILD_SHARED_LIBS="OFF" \
		 -DBUILD_STATIC_LIBS="ON" \
		 -DCMAKE_BUILD_TYPE=Release \
		 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
		 .. || exit 1
	fi

	# Выполняем сборку на всех логических ядрах
	$MAKE -j"$numproc" || exit 1
	# Выполняем установку проекта
	$MAKE install || exit 1

	# Выполняем компенсацию каталогов
	restorelibs $PREFIX

	# Создаём каталог LZ4
	mkdir "$PREFIX/include/lz4"

	# Производим установку заголовочных файлов по нужному пути
	for i in $(ls "$PREFIX/include" | grep "lz4.*\.h$");
	do
		echo "Move \"$PREFIX/include/$i\" to \"$PREFIX/include/lz4/$i\""
		mv "$PREFIX/include/$i" "$PREFIX/include/lz4/$i" || exit 1
	done

	# Помечаем флагом, что сборка и установка произведена
	touch "$src/.stamp_done"
	cd "$ROOT" || exit 1
fi

# Сборка ZSTD
src="$ROOT/submodules/zstd"
if [ ! -f "$src/.stamp_done" ]; then
	printf "\n****** ZSTD ******\n"
	cd "$src/build/cmake" || exit 1

	# Версия ZSTD
	VER="1.5.7"

	# Выполняем удаление предыдущей закаченной версии
	git tag -d v${VER}
	# Закачиваем все изменения
	git fetch --all
	# Закачиваем все теги
	git fetch --all --tags
	# Выполняем жесткое переключение на release
	git reset --hard origin/release
	# Переключаемся на release
	git checkout release
	# Выполняем обновление данных
	git pull origin release
	# Удаляем старую ветку
	git branch -D v${VER}-branch
	# Выполняем переключение на указанную версию
	git checkout -b v${VER}-branch v${VER}

	# Создаём каталог сборки
	mkdir -p "build" || exit 1
	# Переходим в каталог
	cd "build" || exit 1

	# Удаляем старый файл кэша
	rm -rf "$src/build/cmake/build/CMakeCache.txt"

	# Выполняем конфигурацию проекта
	if [[ $OS = "Windows" ]]; then
		cmake \
		 -DZSTD_BUILD_TESTS="OFF" \
		 -DZSTD_BUILD_STATIC="ON" \
		 -DZSTD_BUILD_SHARED="OFF" \
		 -DCMAKE_BUILD_TYPE=Release \
		 -DCMAKE_SYSTEM_NAME=Windows \
		 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
		 -G "MSYS Makefiles" \
		 .. || exit 1
	else
		cmake \
		 -DZSTD_BUILD_TESTS="OFF" \
		 -DZSTD_BUILD_STATIC="ON" \
		 -DZSTD_BUILD_SHARED="OFF" \
		 -DCMAKE_BUILD_TYPE=Release \
		 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
		 .. || exit 1
	fi

	# Выполняем сборку на всех логических ядрах
	$MAKE -j"$numproc" || exit 1
	# Выполняем установку проекта
	$MAKE install || exit 1

	# Выполняем компенсацию каталогов
	restorelibs $PREFIX

	# Создаём каталог ZSTD
	mkdir "$PREFIX/include/zstd"

	# Производим установку заголовочных файлов по нужному пути
	for i in $(ls "$PREFIX/include" | grep ".*\.h$");
	do
		echo "Move \"$PREFIX/include/$i\" to \"$PREFIX/include/zstd/$i\""
		mv "$PREFIX/include/$i" "$PREFIX/include/zstd/$i" || exit 1
	done

	# Помечаем флагом, что сборка и установка произведена
	touch "$src/.stamp_done"
	cd "$ROOT" || exit 1
fi

# Сборка LZma
src="$ROOT/submodules/lzma"
if [ ! -f "$src/.stamp_done" ]; then
	printf "\n****** LZma ******\n"
	cd "$src" || exit 1

	# Версия LZMA
	VER="5.2.3"

	# Выполняем удаление предыдущей закаченной версии
	git tag -d v${VER}-p4
	# Закачиваем все изменения
	git fetch --all
	# Закачиваем все теги
	git fetch --all --tags
	# Выполняем жесткое переключение на hunter
	git reset --hard origin/hunter-${VER}
	# Переключаемся на hunter
	git checkout hunter-${VER}
	# Выполняем обновление данных
	git pull origin hunter-${VER}
	# Удаляем старую ветку
	git branch -D v${VER}-branch
	# Выполняем переключение на указанную версию
	git checkout -b v${VER}-branch v${VER}-p4

	# Выполняем конфигурацию проекта
	if [[ $OS = "Windows" ]]; then
		# Применяем патч
		apply_patch "lzma" "lzma.patch"
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
		 -DLZMA_BUILD_TESTS="OFF" \
		 -DBUILD_SHARED_LIBS="OFF" \
		 -DCMAKE_BUILD_TYPE=Release \
		 -DCMAKE_SYSTEM_NAME=Windows \
		 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
		 -G "MSYS Makefiles" \
		 .. || exit 1
	else
		cmake \
		 -DLZMA_BUILD_TESTS="OFF" \
		 -DBUILD_SHARED_LIBS="OFF" \
		 -DCMAKE_BUILD_TYPE=Release \
		 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
		 .. || exit 1
	fi

	# Выполняем сборку на всех логических ядрах
	$MAKE -j"$numproc" || exit 1
	# Выполняем установку проекта
	$MAKE install || exit 1

	# Выполняем компенсацию каталогов
	restorelibs $PREFIX

	# Создаём каталог LZMA
	mkdir "$PREFIX/include/lzma2"

	# Перемещаем каталог lzma в новый каталог
	mv "$PREFIX/include/lzma" "$PREFIX/include/lzma2/lzma"

	# Производим установку заголовочных файлов по нужному пути
	for i in $(ls "$PREFIX/include" | grep ".*\.h$");
	do
		echo "Move \"$PREFIX/include/$i\" to \"$PREFIX/include/lzma/$i\""
		mv "$PREFIX/include/$i" "$PREFIX/include/lzma2/$i" || exit 1
	done

	# Переименовываем каталог lzma2 в новый каталог
	mv "$PREFIX/include/lzma2" "$PREFIX/include/lzma"

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
	VER="1.1.0"

	# Переключаемся на master
	git checkout master
	# Выполняем удаление предыдущей закаченной версии
	git tag -d v${VER}
	# Закачиваем все теги
	git fetch --all --tags
	# Удаляем старую ветку
	git branch -D v${VER}-branch
	# Выполняем переключение на указанную версию
	git checkout -b v${VER}-branch v${VER}

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
		 -DCMAKE_SYSTEM_NAME="Windows" \
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

	# Помечаем флагом, что сборка и установка произведена
	touch "$src/.stamp_done"
	cd "$ROOT" || exit 1
fi

# Если нужно собрать модуль IDN и операционная система не является Windows
if [[ $IDN = "yes" ]] && [[ ! $OS = "Windows" ]]; then
	# Сборка ICONV
	src="$ROOT/submodules/libiconv"
	if [ ! -f "$src/.stamp_done" ]; then
		printf "\n****** ICONV ******\n"
		cd "$ROOT/submodules" || exit 1

		# Выполняем копирование архива исходников LibIconv
		cp "$ROOT/tar/libiconv-1.17.tar.gz" "$ROOT/submodules/libiconv.tar.gz"

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
	fi
	# Сборка IDN2
	src="$ROOT/submodules/libidn2"
	if [ ! -f "$src/.stamp_done" ]; then
		printf "\n****** IDN2 ******\n"
		cd "$ROOT/submodules" || exit 1

		# Выполняем копирование архива исходников LibIDN
		cp "$ROOT/tar/libidn2-2.3.3.tar.gz" "$ROOT/submodules/libidn2.tar.gz"

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
			# Если операционной системой является MacOS X, FreeBSD, NetBSD или OpenBSD
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
			cd "$ROOT" || exit 1
		# Если архив с исходниками не скачен
		else
			# Выводим сообщение предупреждения
			echo "DOWNLOAD libidn2 sources failed"
		fi
	fi
fi

# Сборка PCRE2
src="$ROOT/submodules/pcre2"
if [ ! -f "$src/.stamp_done" ]; then
	printf "\n****** PCRE2 ******\n"
	cd "$src" || exit 1

	# Версия PCRE2
	VER="10.45"

	# Выполняем удаление предыдущей закаченной версии
	git tag -d pcre2-${VER}
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
	git branch -D v${VER}-branch
	# Выполняем переключение на указанную версию
	git checkout -b v${VER}-branch pcre2-${VER}

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

	# Помечаем флагом, что сборка и установка произведена
	touch "$src/.stamp_done"
	cd "$ROOT" || exit 1
fi

# Сборка NgHttp2
src="$ROOT/submodules/nghttp2"
if [ ! -f "$src/.stamp_done" ]; then
	printf "\n****** NgHttp2 ******\n"
	cd "$src" || exit 1

	# Версия NgHttp2
	VER="1.65.0"

	# Переключаемся на master
	git checkout master
	# Выполняем удаление предыдущей закаченной версии
	git tag -d v${VER}
	# Закачиваем все теги
	git fetch --all --tags
	# Удаляем старую ветку
	git branch -D v${VER}-branch
	# Выполняем переключение на указанную версию
	git checkout -b v${VER}-branch v${VER}

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
	cd "$ROOT" || exit 1
fi

# Выполняем объединение статических библиотек
bash "$ROOT/merge_static_libs.sh"

# Сборка GPerfTools
src="$ROOT/submodules/gperftools"
if [ ! -f "$src/.stamp_done" ]; then
	printf "\n****** GPerfTools ******\n"
	cd "$src" || exit 1

	# Версия GPerfTools
	VER="2.16.90"

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
	git branch -D gperftools-v${VER}-branch
	# Выполняем переключение на указанную версию
	git checkout -b gperftools-v${VER}-branch gperftools-${VER}

	# Создаём каталог сборки
	mkdir -p "build_awh" || exit 1
	# Переходим в каталог
	cd "build_awh" || exit 1

	# Удаляем старый файл кэша
	rm -rf "$src/build_awh/CMakeCache.txt"

	# Выполняем конфигурацию проекта
	if [[ $OS = "Windows" ]]; then
		cmake \
		 -DCMAKE_SYSTEM_NAME=Windows \
		 -DCMAKE_BUILD_TYPE=Release \
		 -DBUILD_TESTING="OFF" \
		 -DBUILD_SHARED_LIBS="OFF" \
		 -DDEFAULT_BUILD_MINIMAL="ON" \
		 -DDEFAULT_BUILD_DEBUGALLOC="OFF" \
		 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
		 -G "MSYS Makefiles" \
		 .. || exit 1
	else
		cmake \
		 -DBUILD_TESTING="OFF" \
		 -DBUILD_SHARED_LIBS="OFF" \
		 -DDEFAULT_BUILD_MINIMAL="ON" \
		 -DDEFAULT_BUILD_DEBUGALLOC="OFF" \
		 -DCMAKE_BUILD_TYPE=Release \
		 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
		 .. || exit 1
	fi

	# Выполняем сборку на всех логических ядрах
	$MAKE -j"$numproc" || exit 1

	# Создаём каталог GPerfTools
	mkdir "$PREFIX/include/gperftools"

	# Производим установку собранных библиотек
	for i in $(ls "$src/build_awh" | grep ".*\.a$");
	do
		echo "Move \"$src/build_awh/$i\" to \"$PREFIX/lib/$i\""
		cp "$src/build_awh/$i" "$PREFIX/lib/$i" || exit 1
	done

	# Производим установку заголовочных файлов по нужному пути
	for i in $(ls "$src/src" | grep ".*\.h$");
	do
		echo "Move \"$src/src/$i\" to \"$PREFIX/include/gperftools/$i\""
		cp "$src/src/$i" "$PREFIX/include/gperftools/$i" || exit 1
	done

	# Помечаем флагом, что сборка и установка произведена
	touch "$src/.stamp_done"
	cd "$ROOT" || exit 1
fi

# Переименовываем расширение библиотек для Windows
if [ $OS = "Windows" ]; then # Windows
	# Выполняем исправление библиотек для x32
	for i in $(ls "$PREFIX/lib" | grep .a$);
	do
		mv "$PREFIX/lib/$i" "$PREFIX/lib/$(basename "$i" .a).lib"
	done
fi

printf "\n****************************************"
printf "\n************   Success!!!   ************"
printf "\n****************************************"
printf "\n\n\n"
