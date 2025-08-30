#!/usr/bin/env bash

# Получаем корневую дирректорию
readonly ROOT=$(cd "$(dirname "$0")" && pwd)

# Получаем версию OS
readonly OS=$(uname -a | awk '{print $1}')

# Определяем является ли операционная система MacOS X
if ! [ $OS = "Darwin" ]; then
	echo "Error: Only for MacOS X"
	exit 1
fi

# Имя пакета (не итоговое)
readonly PACKAGE_NAME="awh"

# Адрес каталога с собранными бинарями
readonly BUILD_DIR="$ROOT/../build"

# Устанавливаем флаги глобального использования
# export CPPFLAGS=""
export CXXFLAGS="$CXXFLAGS -fPIC -mmacosx-version-min=13.3"
export CFLAGS="-I$PREFIX/include -fPIC -mmacosx-version-min=13.3"

# Адрес каталога с рабочим приложением
readonly APP_DIR="$ROOT/../package/MacOS/application"

# Очистка рабочей директории приложения
if [ -d $APP_DIR ]; then
	rm -rf $APP_DIR || exit 1
fi

# Создаём адрес рабочего каталога
mkdir -p "$APP_DIR/include/$PACKAGE_NAME" || exit 1

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
cp $BUILD_DIR/lib$PACKAGE_NAME.dylib $APP_DIR/

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
cp $BUILD_DIR/lib$PACKAGE_NAME.a $APP_DIR/

# Переходим в корневой каталог обратно
cd $ROOT/../

# Получаем версию приложения
readonly VERSION=$(cat $ROOT/../include/sys/lib.hpp | grep AWH_VERSION | awk '{print $3}' | sed "s/^\([\"']\)\(.*\)\1\$/\2/g")

# Копируем файл cmake
cp "$ROOT/../contrib/cmake"/FindAWH.cmake $APP_DIR/FindAWH.cmake
# Копируем зависимости сторонние
cp -r "$ROOT/../contrib/include"/* "$APP_DIR/include"/
# Копируем собранные зависимости
cp -r "$ROOT/../third_party/include"/* "$APP_DIR/include"/
# Копируем заголовки библиотеки
cp -r "$ROOT/../include"/* "$APP_DIR/include/$PACKAGE_NAME"/

# Заменяем конечный адрес назначения
sed -i -e "s!\${CMAKE_SOURCE_DIR}/third_party/lib!/usr/local/lib!" $APP_DIR/FindAWH.cmake
sed -i -e "s!\${CMAKE_SOURCE_DIR}/third_party/include!/usr/local/include/lib${PACKAGE_NAME}!" $APP_DIR/FindAWH.cmake

# Удаляем все ненужные нам файлы
find $APP_DIR -type f -name "*-e" -exec rm {} \;

# Выполняем сборку установщика
bash "$ROOT/../package/MacOS/build.sh" $VERSION || exit 1

# Удаляем временный каталог
rm -rf $APP_DIR

printf "\n****************************************"
printf "\n************   Success!!!   ************"
printf "\n****************************************"
printf "\n\n\n"
