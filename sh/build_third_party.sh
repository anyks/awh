#!/usr/bin/env bash

# Получаем корневую дирректорию
readonly ROOT=$(cd "$(dirname "$0")" && pwd)

# Устанавливаем каталог с скриптами
readonly SCRIPTS="$ROOT/third_party"

# Каталог для установки собранных библиотек
readonly PREFIX="$ROOT/../third_party"

# Получаем версию OS
OS=$(uname -a | awk '{print $1}')

##
# Название системы у MSYS2 зависит от оболочки, а не от машины
#
# Оболочек там несколько, и uname отвечает по той, из какой позван: у MINGW64 и
# CLANGARM64 выходит MINGW64_NT, а у оболочки MSYS - MSYS_NT. Прежде опознавалась
# лишь первая, и запуск из MSYS отвечал "Operating system not defined", ничего не
# поясняя. Вход по ssh даёт как раз MSYS, потому отказ этот встречал всякого, кто
# собирал не из ярлыка
##
if [[ $OS =~ "MINGW" ]] || [[ $OS =~ "MSYS" ]] || [[ $OS =~ "CLANG" ]]; then
	OS="Windows"
fi

##
# Оболочка MSYS для сборки не годится и подменяется на своё
#
# В ней на пути стоит компилятор, целящий в cygwin: c++ -dumpmachine отвечает
# x86_64-pc-cygwin, и собранное зависело бы от msys-2.0.dll вместо того, чтобы быть
# программой Windows. Набор же для Windows лежит в /clangarm64/bin либо /mingw64/bin,
# и на путь их выводит переменная MSYSTEM
#
# Оболочка выбирается по набору команд машины, а он читается из названия системы:
# у Windows ARM64 строка uname оканчивается на ARM64 при любой оболочке, у x86-64
# суффикса нет вовсе (проверено на обоих стендах). Подражанием строка эта не портится,
# в отличие от uname -m, который на ARM64 отвечает x86_64
#
# Признак AWH_SHELL_SWITCHED ограждает от круга: если оболочка не сменилась и со
# второго захода, работа продолжается в имеющейся
##
if [ $OS = "Windows" ] && [ -z "$AWH_SHELL_SWITCHED" ] && [[ ! $MSYSTEM =~ "MINGW" ]] && [[ ! $MSYSTEM =~ "CLANG" ]]; then
	# Оболочка, отвечающая набору команд машины
	SHELL_NAME="MINGW64"
	# Если машина набора команд ARM64
	if [[ $(uname -s) =~ "ARM64" ]]; then
		SHELL_NAME="CLANGARM64"
	fi
	echo "MSYS shell is not suitable for building, work continues in $SHELL_NAME"
	##
	# Скрипт зовётся по пути безусловному, а рабочий каталог восстанавливается
	#
	# Вход в оболочку (-l) уводит в каталог домашний, и относительный путь, каким
	# скрипт был позван, там уже никуда не ведёт: отказ выходит "No such file or
	# directory" при том, что файл на месте
	##
	AWH_SHELL_SWITCHED=1 MSYSTEM="$SHELL_NAME" exec /usr/bin/bash -l -c \
	 'cd "$1" && shift && exec bash "$@"' -- "$(pwd)" "$ROOT/$(basename "$0")" "$@"
fi

# Команда установки собранных файлов по месту
#
# Задаётся она здесь, а не в скриптах отдельных модулей, потому что пользуются ею
# несколько из них - brotli, density и lizard, - а способ установки зависит от системы,
# а не от модуля
#
# Прежде переменная эта присваивалась единственно в 007_brotli.sh, а density и lizard
# только читали её. Работало это потому, что скрипты подключаются через source в общей
# оболочке, а brotli идёт прежде них - и по сортировке ls, и в перечне Requirements.txt.
# Обе опоры случайны: перестановка модулей, отключение brotli или правка его ветки по
# системам оставили бы двум другим модулям чужую команду либо пустоту, и отказ вышел бы
# молчаливым
#
# У macOS берётся ditto: он переносит файл со всеми его признаками
#
# У Sun Solaris и illumos утилит с именем install две. Solaris выводит в путь
# /usr/bin/install из набора GNU coreutils, а OpenIndiana - /usr/sbin/install наследия
# SVR4, который ключей -D и -m в такой записи не знает и отвечает подсказкой
# "usage: install [options] file [dir1 ...]". Проверено опытом на обоих стендах. Набор
# GNU лежит на обеих системах в /usr/gnu/bin, потому путь указывается явно
#
# У систем BSD ключа -D нет, каталог назначения там создаётся заранее
INSTALL_CMD=""
if [ $OS = "Darwin" ]; then
	INSTALL_CMD="ditto -v"
elif [ $OS = "Windows" ] || [ $OS = "Linux" ]; then
	INSTALL_CMD="install -D -m 0644"
elif [ $OS = "SunOS" ]; then
	if [ -x "/usr/gnu/bin/install" ]; then
		INSTALL_CMD="/usr/gnu/bin/install -D -m 0644"
	else
		INSTALL_CMD="install -D -m 0644"
	fi
elif [ $OS = "FreeBSD" ] || [ $OS = "DragonFly" ] || [ $OS = "NetBSD" ] || [ $OS = "OpenBSD" ]; then
	INSTALL_CMD="install -m 0644"
else
	echo "Operating system not defined"
	exit 1
fi

##
# Тип архитектуры берётся у uname, а не у sysctl и не у arch
#
# Утилиты arch у систем BSD нет вовсе: на NetBSD вызов отвечал "command not found",
# тип выходил пустым, и следовавшая за ним сверка ломалась с "unary operator expected".
# Утилита sysctl лежит в /sbin, которого нет в PATH неинтерактивного сеанса ssh у
# NetBSD, - по имени она тоже не находится. Утилита uname лежит в /usr/bin у всех
# перечисленных систем и доступна всегда
#
# Спрашивается именно процессор (-p), а не машина (-m): NetBSD называет машину evbarm,
# а процессор aarch64, и по машине разрядность там не опознать. Часть систем отвечает
# на -p словом unknown - тогда берётся -m
##
# Тип архитектуры
ARCHITECTURE=$(uname -p 2>/dev/null)

# Если процессор не назван, спрашиваем машину
case "$ARCHITECTURE" in
	''|unknown|*[!A-Za-z0-9_-]*)
		ARCHITECTURE=$(uname -m 2>/dev/null)
	;;
esac

##
# У Windows набор команд читается из названия системы, а не у uname -m
#
# Оболочка MSYS2 исполняется под подражанием x86-64, и uname -m отвечает x86_64 даже
# на машине ARM64 - вслед за ним и uname -p, отвечающий unknown. Строка же названия
# системы оканчивается на ARM64 при любой оболочке, а у машины x86-64 суффикса не
# несёт вовсе (проверено на обоих стендах Windows)
#
# Цена ошибки не отвлечённая: приняв ARM64 за x86-64, сборка подставляет -mrdrnd и
# -march=core2, а измеритель gperftools собирается вместо того, чтобы быть погашенным
##
if [ $OS = "Windows" ] && [[ $(uname -s) =~ "ARM64" ]]; then
	ARCHITECTURE="arm"
fi

# Выполняем корректировку типа процессора
case "$ARCHITECTURE" in
	arm|arm64|aarch64|evbarm)
		ARCHITECTURE="arm"
	;;
esac

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
# Если сборка производится в операционной системе FreeBSD, DragonFly, NetBSD или OpenBSD
elif [ $OS = "FreeBSD" ] || [ $OS = "DragonFly" ] || [ $OS = "NetBSD" ] || [ $OS = "OpenBSD" ]; then
	##
	# Утилита опроса системы зовётся по полному пути, а не по имени
	#
	# Лежит она в /sbin, а каталога этого нет в PATH неинтерактивного сеанса ssh у
	# NetBSD: там PATH складывается из /usr/bin, /bin, /usr/pkg/bin и /usr/local/bin.
	# Вызов по имени отвечал "command not found", число ядер выходило пустым, и
	# сборщик получал ключ -j без числа - а это у GNU make не одно задание, а без
	# предела вовсе. Заданий плодилось сколько влезет, и ядро убивало компиляторы:
	# "fatal error: Killed signal terminated program cc1plus". Проверено опытом на
	# стенде NetBSD 11.0 aarch64
	#
	# Имя оставлено первым доводом на случай, если утилита лежит в другом месте
	##
	numproc=$(sysctl -n hw.ncpu 2>/dev/null || /sbin/sysctl -n hw.ncpu 2>/dev/null)
# Если операционная система не определена
else
	echo "Operating system not defined"
	exit 1
fi

##
# Число ядер сверяется прежде употребления
#
# Пустое или нечисловое значение оборачивается ключом -j без числа, а он у GNU make
# снимает предел одновременности вовсе - сборка душит машину и теряет компиляторы,
# убитые ядром. Отступление к одному заданию медленнее, но доводит работу до конца
##
case "$numproc" in
	''|*[!0-9]*)
		echo "Number of CPU cores could not be determined: falling back to a single job"
		numproc=1
	;;
esac

# Если сборка производится в операционной системе FreeBSD, DragonFly, NetBSD, OpenBSD или Solaris
if [ $OS = "FreeBSD" ] || [ $OS = "DragonFly" ] || [ $OS = "NetBSD" ] || [ $OS = "OpenBSD" ] || [ $OS = "SunOS" ]; then
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
