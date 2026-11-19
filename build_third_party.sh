#!/usr/bin/env bash

# Получаем корневую дирректорию
readonly ROOT=$(cd "$(dirname "$0")" && pwd)

# Получаем версию OS
OS=$(uname -a | awk '{print $1}')

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
		clean_submodule "a-nts"
		# clean_submodule "openssl"

		# Удаляем сборочную директорию (префикс у каждой оснастки свой)
		rm -rf "${AWH_THIRD_PARTY:-$ROOT/third_party}"

		printf "\n****************************************"
		printf "\n************   Success!!!   ************"
		printf "\n****************************************"
		printf "\n\n\n"

		exit 0

	elif [ $1 = "--debug" ]; then

		DEBUG="yes"


	else
		printf "Usage: config [options]\n"
		printf " --clean - Cleaning all submodules and build directory\n"

		exit 1
	fi
fi

##
 # Каталог для установки собранных библиотек
 #
 # Префикс берётся из окружения: у каждой оснастки он свой (MSVC -
 # third_party-msvc, CLANGARM64 - third_party-clangarm64), и жёстко прописанный
 # third_party у них не находится. Без переменной адрес прежний
 ##
PREFIX="${AWH_THIRD_PARTY:-$ROOT/third_party}"
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
		export CXX="clang++"
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
	# Устанавливаем тип процессора для сборки
	export CFLAGS="-arch $(uname -m)"
	export CXXFLAGS="-arch $(uname -m)"
	# Устанавливаем тип процессора для cmake
	export CMAKE_OSX_ARCHITECTURES="$(uname -m)"
# Если сборка производится в операционной системе Windows, Linux или Solaris
elif [ $OS = "Windows" ] || [ $OS = "Linux" ] || [ $OS = "SunOS" ]; then
	# Устанавливаем количество ядер системы
	numproc=$(nproc)
# Если сборка производится в операционной системе FreeBSD, DragonFly, NetBSD или OpenBSD
elif [ $OS = "FreeBSD" ] || [ $OS = "DragonFly" ] || [ $OS = "NetBSD" ] || [ $OS = "OpenBSD" ]; then
	# Устанавливаем количество ядер системы
	numproc=$(sysctl -n hw.ncpu)
# Если операционная система не определена
else
	echo "Operating system not defined"
	exit 1
fi

# Значение комманды установки по умолчанию
INSTALL_CMD=""
# Если сборка производится в операционной системе MacOS X
if [ $OS = "Darwin" ]; then
	INSTALL_CMD="ditto -v"
# Если сборка производится в операционной системе Windows или Linux
elif [ $OS = "Windows" ] || [ $OS = "Linux" ]; then
	INSTALL_CMD="install -D -m 0644"
# Если сборка производится в операционной системе Solaris
elif [ $OS = "SunOS" ]; then
	if [ -x "/usr/gnu/bin/install" ]; then
		INSTALL_CMD="/usr/gnu/bin/install -D -m 0644"
	else
		INSTALL_CMD="install -D -m 0644"
	fi
# Если сборка производится в операционной системе FreeBSD, DragonFly, NetBSD или OpenBSD
elif [ $OS = "FreeBSD" ] || [ $OS = "DragonFly" ] || [ $OS = "NetBSD" ] || [ $OS = "OpenBSD" ]; then
	INSTALL_CMD="install -m 0644"
# Если операционная система не определена
else
	echo "Operating system not defined"
	exit 1
fi

# Если сборка производится в операционной системе FreeBSD, DragonFly, NetBSD, OpenBSD или Solaris
if [ $OS = "FreeBSD" ] || [ $OS = "DragonFly" ] || [ $OS = "NetBSD" ] || [ $OS = "OpenBSD" ] || [ $OS = "SunOS" ]; then
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

##
 # Функция переноса содержимого каталога библиотек в основной каталог
 #
 # Переносится и файл, и каталог: A-NTS ставит библиотеку в подкаталог anyks
 # (lib/anyks, на Solaris lib/amd64/anyks), и проверка «только файл» оставляла
 # бы каталог на месте, а следом удаление исходного каталога снесло бы его
 # вместе с библиотекой. Совпавший каталог не пропускается: сборка могла
 # положить туда свежую библиотеку, и она переносится поверх прежней
 ##
movelibs(){
	# Перебираем всё, что лежит в каталоге
	for i in $(ls "$1");
	do
		# Если в основном каталоге имени ещё нет
		if [[ ! -e "$2/$i" ]] && [[ -e "$1/$i" ]]; then
			echo "Move \"$1/$i\" to \"$2/$i\""
			mv "$1/$i" "$2/$i" || exit 1
		# Если каталог с таким именем уже есть
		elif [[ -d "$2/$i" ]] && [[ -d "$1/$i" ]]; then
			cp -R "$1/$i/." "$2/$i/" || exit 1
		fi
	done
	# Удаляем ненужный нам каталог
	rm -rf "$1" || exit 1
}

# Фукция компенсации неверных каталогов
restorelibs(){
	# Если сборка производится в операционной системе Linux
	if [ $OS = "Windows" ] || [ $OS = "Linux" ]; then
		# Если на вход получен каталог
		if [[ -d "$1/lib64" ]]; then
			# Переносим всё что есть в каталоге, в нужный нам каталог
			movelibs "$1/lib64" "$1/lib"
		fi
	# Если сборка производится в операционной системе Solaris
	elif [ $OS = "SunOS" ]; then
		# Если на вход получен каталог
		if [[ -d "$1/lib/64" ]]; then
			# Переносим всё что есть в каталоге, в нужный нам каталог
			movelibs "$1/lib/64" "$1/lib"
		fi
		# A-NTS на Solaris ставит библиотеку в lib/amd64/anyks
		if [[ -d "$1/lib/amd64" ]]; then
			# Переносим всё что есть в каталоге, в нужный нам каталог
			movelibs "$1/lib/amd64" "$1/lib"
		fi
	fi
}

# Выполняем синхронизацию сабмодулей
git submodule sync
# Инициализируем подпроекты
git submodule update --init --recursive

# Сборка A-NTS
src="$ROOT/submodules/a-nts"
if [ ! -f "$src/.stamp_done" ]; then
	printf "\n****** A-NTS ******\n"
	cd "$src" || exit 1

	# Версия A-NTS
	# VER="1.0.0"

	##
	 # A-NTS доводится под требования AWH v4 прямо сейчас, поэтому сборка берёт свежий
	 # main, а не коммит, записанный в суперпроекте: иначе правки наружной библиотеки
	 # (наборы TLS 1.2, base64, хвосты по договорам) не попадали бы в сборку, пока
	 # кто-нибудь не сдвинет указатель. Запись подмодуля при этом остаётся просто
	 # отметкой о том, на чём последний раз принимали, — вести сборку по ней не нужно
	 # Выполняем удаление предыдущей закаченной версии
	 ##
	#git tag -d nts-${VER}
	# Закачиваем все изменения
	git fetch --all
	# Закачиваем все теги
	git fetch --all --tags
	# Выполняем жесткое переключение на main
	git reset --hard origin/main
	# Переключаемся на main (ветка заводится от origin/main, если её ещё нет)
	git checkout -B main origin/main
	# Выполняем обновление данных
	git pull origin main
	# Удаляем старую ветку
	#git branch -D v${VER}-branch
	# Выполняем переключение на указанную версию
	#git checkout -b v${VER}-branch nts-${VER}
	# Печатаем коммит, с которого реально собрали: по нему сверяют приёмку
	echo "A-NTS commit: $(git rev-parse HEAD)"

	##
	 # Снимаем остатки прежней раскладки A-NTS
	 #
	 # Новый выпуск ставит заголовки в include/anyks/nts и библиотеку в
	 # lib/anyks, а прежние include/nts и lib/libnts.a (на Solaris
	 # lib/amd64/libnts.a) установка не трогает. Оставшись, старая библиотека
	 # вошла бы в объединение наравне с новой, и линковка взяла бы модули
	 # первой из них молча
	 ##
	rm -rf "$PREFIX/include/nts"
	rm -f "$PREFIX/lib/libnts.a" "$PREFIX/lib64/libnts.a" "$PREFIX/lib/64/libnts.a" "$PREFIX/lib/amd64/libnts.a"

	# Создаём каталог сборки
	mkdir -p "build" || exit 1
	# Переходим в каталог
	cd "build" || exit 1

	# Удаляем старый файл кэша
	rm -rf "$src/build/CMakeCache.txt"

	# Выполняем конфигурацию проекта
	if [[ $OS = "Windows" ]]; then
		cmake \
		 -DCMAKE_BUILD_TESTS="OFF" \
		 -DCMAKE_BUILD_SANITIZE="OFF" \
		 -DCMAKE_BUILD_BENCHMARKS="OFF" \
		 -DCMAKE_BUILD_SAMPLES="OFF" \
		 -DCMAKE_BUILD_LIBFUZZER="OFF" \
		 -DCMAKE_BUILD_APP="OFF" \
		 -DCMAKE_BUILD_TYPE="Release" \
		 -DCMAKE_SYSTEM_NAME=Windows \
		 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
		 -G "MSYS Makefiles" \
		 .. || exit 1
	else
		cmake \
		 -DCMAKE_BUILD_TESTS="OFF" \
		 -DCMAKE_BUILD_SANITIZE="OFF" \
		 -DCMAKE_BUILD_BENCHMARKS="OFF" \
		 -DCMAKE_BUILD_SAMPLES="OFF" \
		 -DCMAKE_BUILD_LIBFUZZER="OFF" \
		 -DCMAKE_BUILD_APP="OFF" \
		 -DCMAKE_BUILD_TYPE="Release" \
		 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
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

# Выполняем объединение статических библиотек (каталог берётся из префикса)
bash "$ROOT/merge_static_libs.sh" "$PREFIX/lib"

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
