#!/usr/bin/env bash

# Получаем корневую дирректорию
readonly ROOT=$(cd "$(dirname "$0")" && pwd)

# Устанавливаем каталог с скриптами
readonly SCRIPTS="$ROOT/third_party"

# Каталог для установки собранных библиотек
readonly PREFIX="$ROOT/../third_party"

# Получаем версию OS
OS=$(uname -a | awk '{print $1}')

# Компенсируем название OS Windows
if [[ $OS =~ "MINGW64" ]]; then
	OS="Windows"
fi

# Тип архитектуры
ARCHITECTURE=""
# Получаем тип архитектуры
if [ $OS = "FreeBSD" ]; then # FreeBSD
	ARCHITECTURE=$(sysctl -a | egrep -i 'hw.machine|hw.model|hw.ncpu' | grep hw.machine: | awk '{print $2}')
else # Linux
	ARCHITECTURE=$(arch)
fi

# Выполняем корректировку типа процессора
if [ $ARCHITECTURE = "arm64" ] || [ $ARCHITECTURE = "aarch64" ]; then
	ARCHITECTURE="arm"
fi

# Устанавливаем флаги глобального использования
# export CPPFLAGS=""
export CXXFLAGS="$CXXFLAGS -fPIC"
export CFLAGS="$CFLAGS -I$PREFIX/include -fPIC"

export LDFLAGS="$LDFLAGS -L$PREFIX/lib"
export LD_LIBRARY_PATH="$PREFIX/lib"

# Инициализируем каталоги установки
export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig"

# Устанавливаем минимальную версию CMake
export CMAKE_POLICY_VERSION_MINIMUM=3.5

# Создаём каталоги
mkdir -p "$PREFIX/bin"
mkdir -p "$PREFIX/lib"
mkdir -p "$PREFIX/include"

# Если операционная система используется Solaris
if [ $OS = "SunOS" ]; then
	# Устанавливаем жёстко компилятор
	export CC="gcc -m64"
# Если операционная система используется Linux
elif [ $OS = "Linux" ]; then
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

# Определяем количество логических ядер
if [ $OS = "Darwin" ]; then
	# Устанавливаем количество ядер системы
	numproc=$(sysctl -n hw.logicalcpu)
	# Если версия macOS не установлена
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

# Если сборка производится в операционной системе FreeBSD, NetBSD, OpenBSD или Solaris
if [ $OS = "FreeBSD" ] || [ $OS = "NetBSD" ] || [ $OS = "OpenBSD" ] || [ $OS = "SunOS" ]; then
	# Устанавливаем сборщик
	MAKE="gmake"
# Если сборка производится в другой операционной системе
else
	# Устанавливаем сборщик
	MAKE="make"
fi

# Очистка директории
clean_directory(){
	git clean -dfx "$1"
}

# Очистка подпроекта
clean_submodule(){
	cd "$ROOT/../submodules/$1" || exit 1
	git clean -dfx
	git checkout .
	cd "$ROOT" || exit 1
}

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

# Если необходимо обновить или удалить зависимости
if [ -n "$1" ]; then
	# Если необходимо удалить или очистить модуль
	if [ $1 = "--clean" ]; then
		# Производим перебор всех скриптов зависимостей
		for i in $(ls $SCRIPTS | grep .sh$);
		do
			source $SCRIPTS/$i --clean || exit 1
		done

		# Удаляем каталог зависимостей
		rm -rf "$ROOT/../third_party"

		printf "\n****************************************"
		printf "\n************   Success!!!   ************"
		printf "\n****************************************"
		printf "\n\n\n"

		exit 0
	# Если необходимо полностью сбросить проект
	elif [ $1 = "--reset" ]; then
		# Производим перебор всех скриптов зависимостей
		for i in $(ls $SCRIPTS | grep .sh$);
		do
			source $SCRIPTS/$i --reset || exit 1
		done

		# Удаляем каталог зависимостей
		rm -rf "$ROOT/../third_party"

		printf "\n****************************************"
		printf "\n************   Success!!!   ************"
		printf "\n****************************************"
		printf "\n\n\n"

		exit 0
	# Если необходимо обновить зависимости
	elif [ $1 = "--update" ]; then
		# Производим перебор всех скриптов зависимостей
		for i in $(ls $SCRIPTS | grep .sh$);
		do
			source $SCRIPTS/$i --update || exit 1
		done

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
	# Если команда не распознана
	else
		printf "Usage: config [options]\n"
		printf " --clean - Cleaning all submodules and build directory\n"

		exit 1
	fi
# Если необходимо собрать зависимости
else
	# Выполняем установку домашнего каталога
	unset PYTHONHOME
	# Выполняем экспорт адреса каталога с модулями Python
	unset PYTHONPATH

	# Выполняем синхронизацию сабмодулей
	git submodule sync
	# Инициализируем подпроекты
	git submodule update --init --recursive
	
	# Если файл зависимостей не найден
	if [ ! -f "$ROOT/../Requirements.txt" ]; then
		# Производим перебор всех скриптов зависимостей
		for i in $(ls $SCRIPTS | grep .sh$);
		do
			source $SCRIPTS/$i --build || exit 1
		done
	# Если зависимости найдены
	else
		# Перебираем все зависимости из файла зависимостей
		while read i; do
			# Если строка начинается с символа #, то пропускаем её
			if [[ $i =~ ^# ]]; then
				continue
			fi
			# Если строка начинается с символа -, то пропускаем её
			if [[ $i =~ ^- ]]; then
				continue
			fi
			# Получаем название модуля
			name=$(echo "$i" | awk '{print $1;}')
			# Получаем название флага
			flag=$(echo "$i" | awk '{print $2;}')
			# Получаем название ветки или тега
			type=$(echo "$i" | awk '{print $3;}')
			# Если указан флаг ветки
			if [ $flag = "--none" ]; then
				source $SCRIPTS/*_$name.sh --build --n$type || exit 1
			# Если указан флаг ветки
			elif [ $flag = "--branch" ]; then
				source $SCRIPTS/*_$name.sh --build --b$type || exit 1
			# Если указан флаг тега
			elif [ $flag = "--tag" ]; then
				source $SCRIPTS/*_$name.sh --build --t$type || exit 1
			# Если указан коммит
			elif [ $flag = "--commit" ]; then
				source $SCRIPTS/*_$name.sh --build --c$type || exit 1
			# Если указана версия модуля
			elif [ $flag = "--version" ]; then
				source $SCRIPTS/*_$name.sh --build --v$type || exit 1
			fi
		done <"$ROOT/../Requirements.txt"
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
fi
