#!/usr/bin/env bash

# Получаем корневую дирректорию
readonly ROOT=$(cd "$(dirname "$0")" && pwd)

# Получаем версию OS
OS=$(uname -a | awk '{print $1}')

# Компенсируем название OS Windows
if [[ $OS =~ "MINGW64" ]]; then
	OS="Windows"
fi

# Определяем является ли операционная система Microsoft Windows
if ! [ $OS = "Windows" ]; then
	echo "Error: Only for Windows"
	exit 1
fi

# Имя пакета (не итоговое)
readonly PACKAGE_NAME="awh"

# Удаляем директории если существуют
rm -rf "$ROOT/../setup"
rm -rf "$ROOT/../installer"

# Создаем директории для сборки инсталятора
mkdir -p "$ROOT/../installer" || exit 1
mkdir -p "$ROOT/../setup/include/$PACKAGE_NAME" || exit 1

# Адрес каталога с собранными бинарями
readonly BUILD_DIR="$ROOT/../build"

# Очистка сборочной директории
if [ -d $BUILD_DIR ]; then
	rm -rf $BUILD_DIR || exit 1
fi

# Собираем приложение
mkdir $BUILD_DIR || exit 1
cd $BUILD_DIR || exit 1

# Выполняем сборку приложения
cmake \
 -G "MSYS Makefiles" \
 -DCMAKE_BUILD_IDN=YES \
 -DCMAKE_BUILD_TYPE=Release \
 -DCMAKE_SHARED_BUILD_LIB=YES \
 -DCMAKE_SYSTEM_NAME=Windows \
 .. || exit 1

cmake --build . || exit 1

# Копируем собранную динамическую библиотеку
cp $BUILD_DIR/lib$PACKAGE_NAME.dll $ROOT/../setup/$PACKAGE_NAME.dll

# Очищаем всю дирректорию
cmake --build . --target clean

# Удаляем все, что есть в каталоге
rm -rf $BUILD_DIR/*

# Выполняем сборку приложения
cmake \
 -G "MSYS Makefiles" \
 -DCMAKE_BUILD_IDN=YES \
 -DCMAKE_BUILD_TYPE=Release \
 -DCMAKE_SYSTEM_NAME=Windows \
 .. || exit 1

cmake --build . || exit 1

# Копируем собранную статическую библиотеку
cp $BUILD_DIR/lib$PACKAGE_NAME.a $ROOT/../setup/lib$PACKAGE_NAME.lib

# Переходим в корневой каталог обратно
cd $ROOT/../

# Выполняем удаление директории для сборки пакета
if [ -d $BUILD_DIR ]; then
	# Удаляем ранее собранный каталог
	rm -rf $BUILD_DIR || exit 1
fi

# Получаем версию приложения
readonly VERSION=$(cat $ROOT/../include/sys/lib.hpp | grep AWH_VERSION | awk '{print $3}' | sed "s/^\([\"']\)\(.*\)\1\$/\2/g")

# Копируем иконку приложения
cp "$ROOT/../icons"/icon.ico $ROOT/../setup/
# Копируем иконку установщика
cp "$ROOT/../icons"/setup.ico $ROOT/../setup/
# Копируем файл cmake
cp "$ROOT/../contrib/cmake"/FindAWH.cmake $ROOT/../setup/
# Копируем шаблон установщика
cp "$ROOT/../package/Windows"/install.iss $ROOT/../setup/
# Копируем зависимости сторонние
cp -ar "$ROOT/../contrib/include"/* $ROOT/../setup/include/
# Копируем собранные зависимости
cp -ar "$ROOT/../third_party/include"/* $ROOT/../setup/include/
# Копируем заголовки библиотеки
cp -ar "$ROOT/../include"/* $ROOT/../setup/include/$PACKAGE_NAME/

# Заменяем конечный адрес назначения
sed -i "s%\${CMAKE_SOURCE_DIR}/third_party/lib%/usr/lib%g" $ROOT/../setup/FindAWH.cmake
sed -i "s%\${CMAKE_SOURCE_DIR}/third_party/bin/${PACKAGE_NAME}%/usr/bin%g" $ROOT/../setup/FindAWH.cmake
sed -i "s%\${CMAKE_SOURCE_DIR}/third_party/include%/usr/include/lib${PACKAGE_NAME}%g" $ROOT/../setup/FindAWH.cmake

# Извлекаем все библиотеки зависимостей
for i in $(ldd $ROOT/../setup/lib$PACKAGE_NAME.dll | awk '{print $3}');
do
	# Если мы нашли зависимости MinGW
	if [[ $i == "/mingw64/bin"* ]]; then
		# Копируем библиотеку зависимости
		cp -ar "$i" "$ROOT/../setup"/ || exit 1
	fi
done

# Заполняем поля шаблона для создания конфига инсталятора
SOURCE_DIR=`echo $(cygpath -wm "$ROOT/../setup")`
INSTALLER_DIR=`echo $(cygpath -wm "$ROOT/../installer")`

SOURCE_DIR=$(echo $SOURCE_DIR | sed -e 's|/|\\\\|g')
INSTALLER_DIR=$(echo $INSTALLER_DIR | sed -e 's|/|\\\\|g')

# Формируем конфигурационный файл установщика
sed -i "s|@version@|${VERSION}|g" $ROOT/../setup/install.iss
sed -i "s|@pwd@|${SOURCE_DIR}|g" $ROOT/../setup/install.iss
sed -i "s|@name@|${PACKAGE_NAME}|g" $ROOT/../setup/install.iss
sed -i "s|@installerDir@|${INSTALLER_DIR}|g" $ROOT/../setup/install.iss
