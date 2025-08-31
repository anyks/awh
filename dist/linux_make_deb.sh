#!/usr/bin/env bash

# Получаем корневую дирректорию
readonly ROOT=$(cd "$(dirname "$0")" && pwd)

# Получаем версию OS
readonly OS=$(uname -a | awk '{print $1}')

# Определяем является ли операционная система Linux
if ! [ $OS = "Linux" ]; then
	echo "Error: Only for Linux"
	exit 1
fi

# Имя пакета (не итоговое)
readonly PACKAGE_NAME="awh"

# Адрес каталога с собранными бинарями
readonly BUILD_DIR="$ROOT/../build"

# Адрес временного каталога
readonly TMP_DIR="$ROOT/../$PACKAGE_NAME-tmp"

# Выполняем получение номера релиза
RELEASE_NUMBER="$1"
# Если номер релиза не установлен
if [ ! -n "$RELEASE_NUMBER" ]; then
	# Устанавливаем номер релиза по умолчанию
	RELEASE_NUMBER="1"
fi

# Создаём временный каталог
mkdir -p $TMP_DIR

# Очистка сборочной директории
if [ -d $BUILD_DIR ]; then
	rm -rf $BUILD_DIR || exit 1
fi

# Собираем приложение
mkdir $BUILD_DIR || exit 1
cd $BUILD_DIR || exit 1

# Выполняем сборку приложения
cmake \
 -DCMAKE_BUILD_IDN=YES \
 -DCMAKE_BUILD_TYPE=Release \
 -DCMAKE_SHARED_BUILD_LIB=YES \
 .. || exit 1

cmake --build . || exit 1

# Копируем собранную динамическую библиотеку
cp $BUILD_DIR/lib$PACKAGE_NAME.so $TMP_DIR/

# Очищаем всю дирректорию
cmake --build . --target clean

# Удаляем все, что есть в каталоге
rm -rf $BUILD_DIR/*

# Выполняем сборку приложения
cmake \
 -DCMAKE_BUILD_IDN=YES \
 -DCMAKE_BUILD_TYPE=Release \
 .. || exit 1

cmake --build . || exit 1

# Переходим в корневой каталог обратно
cd $ROOT/../

# Задаем необходимые параметры
WORK_PREFIX="$ROOT/../pkg-${PACKAGE_NAME}"
PACKAGE_SOURCE_DIR="$ROOT/../package/Linux/deb"
PACKAGE_DEST_DIR="$WORK_PREFIX/DEBIAN"
CONTROL_NAME="control"

# Получаем архитектуру машины
SYSTEM_ARCHITECTURE=$(dpkg --print-architecture)

# Получаем имя ОС
OS_NAME=$(cat /etc/os-release | grep VERSION_CODENAME)
OS_NAME=${OS_NAME##*=}

# Получаем версию библиотеки
get_lib_version () {
	local LIB_VER=$(dpkg -s $1 | grep '^Version: ')
	LIB_VER=${LIB_VER%-*}
	LIB_VER=${LIB_VER:9}

	echo "$LIB_VER"
}

# Получаем список нужных нам библиотек
readonly DEPENDENCIES_LIST=$(cat "$PACKAGE_SOURCE_DIR/dependent_pkg")

# Список зависимостей
DEPENDENCIES=""
# Проходим в цикле по списку и добавляем в зависимость
for pkg in $DEPENDENCIES_LIST; do
	echo "Check dependent package: ${pkg}"
	LIB_VER=$(get_lib_version ${pkg})
	# Если библиотека используется в данной ОС, то добавляем в зависимость пакета
	if ! [ "${LIB_VER}" = "" ]; then
		echo "Add dependent package: ${pkg}"
		if [ "$DEPENDENCIES" = "" ]; then
			DEPENDENCIES="${pkg} (>= ${LIB_VER})"
		else
			DEPENDENCIES="$DEPENDENCIES, ${pkg} (>= ${LIB_VER})"
		fi
	fi
done

# Получаем версию приложения
readonly VERSION=$(cat $ROOT/../include/sys/lib.hpp | grep AWH_VERSION | awk '{print $3}' | sed "s/^\([\"']\)\(.*\)\1\$/\2/g")

# Выполняем создание каталогов
mkdir -p "$WORK_PREFIX/tmp" || exit 1
mkdir -p "$WORK_PREFIX/usr/lib" || exit 1
mkdir -p "$WORK_PREFIX/usr/include/lib$PACKAGE_NAME/$PACKAGE_NAME" || exit 1

# Копируем собранную статическую библиотеку
mv $TMP_DIR/lib$PACKAGE_NAME.a "$WORK_PREFIX/usr/lib"/
# Копируем собранную динамическую библиотеку
mv $TMP_DIR/lib$PACKAGE_NAME.so "$WORK_PREFIX/usr/lib"/
# Копируем файл cmake
cp "$ROOT/../contrib/cmake"/FindAWH.cmake "$WORK_PREFIX/tmp"/
# Копируем зависимости сторонние
cp -r "$ROOT/../contrib/include"/* "$WORK_PREFIX/usr/include/lib$PACKAGE_NAME"/
# Копируем собранные зависимости
cp -r "$ROOT/../third_party/include"/* "$WORK_PREFIX/usr/include/lib$PACKAGE_NAME"/
# Копируем заголовки библиотеки
cp -r "$ROOT/../include"/* "$WORK_PREFIX/usr/include/lib$PACKAGE_NAME/$PACKAGE_NAME"/

# Удаляем более ненужный нам каталог
rm -rf $TMP_DIR

# Активируем глобальную сорку
sed -i "s%SET(AHW_GLOBAL_INSTALLATION FALSE)%SET(AHW_GLOBAL_INSTALLATION TRUE)%g" $WORK_PREFIX/tmp/FindAWH.cmake

# Создаем директории для сборки deb пакета
mkdir -p "$PACKAGE_DEST_DIR" || exit 1

# Копируем в нее все необходимые файлы
# /DEBIAN/control
cp -ar "$PACKAGE_SOURCE_DIR/$CONTROL_NAME" "$PACKAGE_DEST_DIR" || exit 1
# /DEBIAN/copyright
cp "$PACKAGE_SOURCE_DIR/copyright" "$PACKAGE_DEST_DIR" || exit 1
# /DEBIAN/postinst
cp "$PACKAGE_SOURCE_DIR/postinst" "$PACKAGE_DEST_DIR" || exit 1

# Определяем размер установаленного пакета
size=$(du -h -s -k "$WORK_PREFIX")
size=${size//[^0-9]/}

# Заполняем поля в файле control необходимыми значениями
sed -i "s%@version@%${VERSION}%g" "$PACKAGE_DEST_DIR/$CONTROL_NAME"
sed -i "s%@package@%${PACKAGE_NAME}%g" "$PACKAGE_DEST_DIR/$CONTROL_NAME"
sed -i "s%@architecture@%${SYSTEM_ARCHITECTURE}%g" "$PACKAGE_DEST_DIR/$CONTROL_NAME"
sed -i "s%@dependencies@%${DEPENDENCIES}%g" "$PACKAGE_DEST_DIR/$CONTROL_NAME"
sed -i "s%@size@%${size}%g" "$PACKAGE_DEST_DIR/$CONTROL_NAME"

# Заменяем поля в скрипте постустановки
sed -i "s%__PACKAGE_NAME__%$PACKAGE_NAME%g" "$PACKAGE_DEST_DIR/postinst"

# Подсчитываем контрольную сумму
pushd "$WORK_PREFIX"
hashdeep -c md5 -r -l $(ls -I "DEBIAN") > "DEBIAN/md5sums"
popd

# Создаем deb пакет
fakeroot dpkg-deb --build "$WORK_PREFIX" || exit 1

# Устанавливаем имя deb пакета foo_VVV-RRR_AAA.deb
if [ "${OS_NAME}" = "" ]; then
	deb_name="${PACKAGE_NAME}_${VERSION}-${RELEASE_NUMBER}_${SYSTEM_ARCHITECTURE}.deb"
else
	deb_name="${PACKAGE_NAME}_${VERSION}-${RELEASE_NUMBER}~${OS_NAME}_${SYSTEM_ARCHITECTURE}.deb"
fi
mv "${WORK_PREFIX}.deb" "$deb_name" || exit 1

# Очищаем сборочную директорию
rm -rf "$WORK_PREFIX"

# Выводим сообщение об удачной сборке
echo "Successfully created package $deb_name"
