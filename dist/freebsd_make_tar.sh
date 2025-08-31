#!/usr/bin/env bash

# Получаем корневую дирректорию
readonly ROOT=$(cd "$(dirname "$0")" && pwd)

# Получаем версию OS
readonly OS=$(uname -a | awk '{print $1}')

# Определяем является ли операционная система FreeBSD
if ! [ $OS = "FreeBSD" ]; then
	echo "Error: Only for FreeBSD"
	exit 1
fi

# Имя пакета
readonly PACKAGE_NAME="awh"

# Адрес каталога с рабочим приложением
readonly APP_DIR="$ROOT/../${PACKAGE_NAME}_dist"
# Сборочная дирректория
readonly BUILD_DIR="$ROOT/../${PACKAGE_NAME}_build"

# Выполняем создание каталогов
mkdir -p $APP_DIR/usr/local/lib || exit 1
mkdir -p $APP_DIR/usr/local/share/cmake/Modules || exit 1
mkdir -p $APP_DIR/usr/local/include/lib$PACKAGE_NAME/$PACKAGE_NAME || exit 1

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
cp $BUILD_DIR/lib$PACKAGE_NAME.so $APP_DIR/usr/local/lib/

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

# Копируем собранную статическую библиотеку
cp $BUILD_DIR/lib$PACKAGE_NAME.a $APP_DIR/usr/local/lib/

# Переходим в корневой каталог обратно
cd $ROOT/../

# Получаем версию приложения
readonly VERSION=$(cat $ROOT/../include/sys/lib.hpp | grep AWH_VERSION | awk '{print $3}' | sed "s/^\([\"']\)\(.*\)\1\$/\2/g")

# Получаем архитектуру машины 
SYSTEM_ARCHITECTURE=$(uname -m)
if [ "${SYSTEM_ARCHITECTURE}" = "x86_64" ] || [ "${SYSTEM_ARCHITECTURE}" = "i86pc" ]; then
	SYSTEM_ARCHITECTURE="amd64"
fi

# Копируем файл cmake
cp "$ROOT/../contrib/cmake"/FindAWH.cmake "$APP_DIR/usr/local/share/cmake/Modules"/
# Копируем зависимости сторонние
cp -r "$ROOT/../contrib/include"/* "$APP_DIR/usr/local/include/lib$PACKAGE_NAME"/
# Копируем собранные зависимости
cp -r "$ROOT/../third_party/include"/* "$APP_DIR/usr/local/include/lib$PACKAGE_NAME"/
# Копируем заголовки библиотеки
cp -r "$ROOT/../include"/* "$APP_DIR/usr/local/include/lib$PACKAGE_NAME/$PACKAGE_NAME"/

# Заменяем конечный адрес назначения
sed -i "s%\${CMAKE_SOURCE_DIR}/third_party/lib%/usr/local/lib%g" $APP_DIR/usr/local/share/cmake/Modules/FindAWH.cmake
sed -i "s%\${CMAKE_SOURCE_DIR}/third_party/bin/${PACKAGE_NAME}%/usr/local/bin%g" $APP_DIR/usr/local/share/cmake/Modules/FindAWH.cmake
sed -i "s%\${CMAKE_SOURCE_DIR}/third_party/include%/usr/local/include/lib${PACKAGE_NAME}%g" $APP_DIR/usr/local/share/cmake/Modules/FindAWH.cmake

# Заходим в каталог
cd $APP_DIR || exit 1
# Создаем имя архива
ARCH_NAME="${PACKAGE_NAME}_${VERSION}_${OS}_${SYSTEM_ARCHITECTURE}.tar.gz"
# Выполняем создание архива
tar -czf ../$ARCH_NAME . || exit 1

# Переходим в корневой каталог обратно
cd $ROOT/../

# Удаляем ранее собранный каталог
rm -rf $APP_DIR || exit 1

# Выводим сообщение о результате
printf "\n****************************************"
printf "\n************   Success!!!   ************"
printf "\n****************************************"
printf "\n\n\n"
