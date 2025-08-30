#!/usr/bin/env bash

# Получаем корневую дирректорию
readonly ROOT=$(cd "$(dirname "$0")" && pwd)

# Получаем версию OS
readonly OS=$(uname -a | awk '{print $1}')

# Определяем является ли операционная система Solaris
if ! [ $OS = "SunOS" ]; then
	echo "Error: Only for Solaris"
	exit 1
fi

# Имя пакета (не итоговое)
readonly PACKAGE_NAME="awh"

# Автор публикуемого проекта
readonly PACKAGE_PUBLISHER="ANYKS"

# Название приложения
readonly PACKAGE_SMMARY="Modern network technologies"
# Описание приложения
readonly PACKAGE_DESCRIPTION="A network framework for building modern web applications."

# Получаем архитектуру машины 
readonly SYSTEM_ARCHITECTURE=$(uname -m)

# Адрес каталога с рабочим приложением
readonly APP_DIR="$ROOT/../${PACKAGE_NAME}_dist"

# Сборочная дирректория
readonly BUILD_DIR="$ROOT/../${PACKAGE_NAME}_build"

# Выполняем создание каталогов
mkdir -p $APP_DIR/usr/lib/amd64 || exit 1
mkdir -p $APP_DIR/usr/tmp/scripts || exit 1
mkdir -p $APP_DIR/usr/include/lib$PACKAGE_NAME/$PACKAGE_NAME || exit 1

# Очистка сборочной директории
if [ -d $BUILD_DIR ]; then
	rm -rf $BUILD_DIR || exit 1
fi

# Собираем приложение
mkdir $BUILD_DIR || exit 1
cd $BUILD_DIR || exit 1

# Устанавливаем жёстко компилятор
export CC="gcc -m64"

# Выполняем сборку приложения
cmake \
 -DCMAKE_BUILD_IDN=YES \
 -DCMAKE_BUILD_TYPE=Release \
 -DCMAKE_SHARED_BUILD_LIB=YES \
 .. || exit 1

cmake --build . || exit 1

# Копируем собранную динамическую библиотеку
cp $BUILD_DIR/lib$PACKAGE_NAME.so $APP_DIR/usr/lib/amd64/

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
cp $BUILD_DIR/lib$PACKAGE_NAME.a $APP_DIR/usr/lib/amd64/

# Переходим в корневой каталог обратно
cd $ROOT/../

# Выполняем удаление директории для сборки пакета
if [ -d $BUILD_DIR ]; then
	# Удаляем ранее собранный каталог
	rm -rf $BUILD_DIR || exit 1
fi

# Инициализируем манифест
readonly MANIFEST_PREFIX="$ROOT/../${PACKAGE_NAME}_manifest"

# Получаем версию приложения
readonly VERSION=$(cat $ROOT/../include/sys/lib.hpp | ggrep AWH_VERSION | gawk '{print $3}' | gsed "s/^\([\"']\)\(.*\)\1\$/\2/g")

# Получаем мажёрное значение версии
readonly VERSION_P=$(echo $VERSION | awk -F '\\.' '{print $1}')
# Получаем минорное значение версии
readonly VERSION_u=$(echo $VERSION | awk -F '\\.' '{print $2}')
# Получаем релизное значение версии
readonly VERSION_r=$(echo $VERSION | awk -F '\\.' '{print $3}')

# Создаём название пакета установки
readonly PKG_NAME="${PACKAGE_NAME}_${VERSION}-1_${SYSTEM_ARCHITECTURE}.p5p"

# Копируем файл cmake
cp "$ROOT/../contrib/cmake"/FindAWH.cmake "$APP_DIR/usr/tmp"/
# Копируем каталог с скриптом последующей установки
cp "$ROOT/../package/Solaris"/postinstall "$APP_DIR/usr/tmp/scripts"/
# Копируем зависимости сторонние
cp -r "$ROOT/../contrib/include"/* "$APP_DIR/usr/include/lib$PACKAGE_NAME"/
# Копируем собранные зависимости
cp -r "$ROOT/../third_party/include"/* "$APP_DIR/usr/include/lib$PACKAGE_NAME"/
# Копируем заголовки библиотеки
cp -r "$ROOT/../include"/* "$APP_DIR/usr/include/lib$PACKAGE_NAME/$PACKAGE_NAME"/

# Заменяем конечный адрес назначения
gsed -i "s%\${CMAKE_SOURCE_DIR}/third_party/lib%/usr/lib/amd64%g" $APP_DIR/usr/tmp/FindAWH.cmake
gsed -i "s%\${CMAKE_SOURCE_DIR}/third_party/include%/usr/include/lib${PACKAGE_NAME}%g" $APP_DIR/usr/tmp/FindAWH.cmake

# Выставляем права доступа на каталог
find $APP_DIR -type d ! -perm 755 -exec chmod 755 {} \;
# Выставляем права доступа на файлы
find $APP_DIR -type f ! -perm 644 -exec chmod 644 {} \;

# Выполняем удаление директории для хранения манифестов
if [ -d $MANIFEST_PREFIX ]; then
	rm -rf $MANIFEST_PREFIX || exit 1
fi

# Создаем директории для хранения манифестов
mkdir -p "$MANIFEST_PREFIX" || exit 1

# Генерируем Manifest файл
pkgsend generate $APP_DIR | pkgfmt > $MANIFEST_PREFIX/$PACKAGE_NAME.p5m.1

# Заменяем группу пользователя по умолчанию
#gsed -i "s%group=bin%group=root%g" $MANIFEST_PREFIX/$PACKAGE_NAME.p5m.1

# Создаём файл информационных данных
touch $MANIFEST_PREFIX/$PACKAGE_NAME.mog
# Формируем версию приложения
echo "set name=pkg.fmri value=$PACKAGE_NAME@$VERSION" >> $MANIFEST_PREFIX/$PACKAGE_NAME.mog
# Формируем название приложения
echo "set name=pkg.summary value=\"$PACKAGE_SMMARY\"" >> $MANIFEST_PREFIX/$PACKAGE_NAME.mog
# Формируем описание приложения
echo "set name=pkg.description value=\"$PACKAGE_DESCRIPTION\"" >> $MANIFEST_PREFIX/$PACKAGE_NAME.mog
# Добавляем человекочитаемую значение версии приложения
echo "set name=pkg.human-version value=\"P$VERSION_P-u$VERSION_u-r$VERSION_r\"" >> $MANIFEST_PREFIX/$PACKAGE_NAME.mog
# Формируем название архитектуры процессора
echo "set name=variant.arch value=\$(ARCH)" >> $MANIFEST_PREFIX/$PACKAGE_NAME.mog
# Формируем категорию размещения приложения согласно файлу ($ cat /usr/share/lib/pkg/opensolaris.org.sections)
echo "set name=info.classification value=\"org.opensolaris.category.2008:Applications/Internet\"" >> $MANIFEST_PREFIX/$PACKAGE_NAME.mog
# Выполняем установку скрипта postinstall
# echo "set name=postinstall value=\"usr/tmp/scripts/postinstall\"" >> $MANIFEST_PREFIX/$PACKAGE_NAME.mog
# Выполняем установку скрипта postinstall
echo "legacy pkg=$PACKAGE_NAME pkg.relocation.pkgmap=no postinstall=usr/tmp/scripts/postinstall" >> $MANIFEST_PREFIX/$PACKAGE_NAME.mog
# Формируем правила сборки
echo "<transform dir path=usr\$->drop>" >> $MANIFEST_PREFIX/$PACKAGE_NAME.mog

# Комбинируем наш манифест с информационным файлом
pkgmogrify -DARCH=`uname -p` $MANIFEST_PREFIX/$PACKAGE_NAME.p5m.1 $MANIFEST_PREFIX/$PACKAGE_NAME.mog | pkgfmt > $MANIFEST_PREFIX/$PACKAGE_NAME.p5m.2
# Выполняем генерацию итогового значения манифеста
pkgdepend generate -md $APP_DIR $MANIFEST_PREFIX/$PACKAGE_NAME.p5m.2 | pkgfmt > $MANIFEST_PREFIX/$PACKAGE_NAME.p5m

# Выполняем резолвинг сгенерированного манифеста
pkgdepend resolve -m $MANIFEST_PREFIX/$PACKAGE_NAME.p5m

# Проверяем соответствие зависимостям
pkglint $MANIFEST_PREFIX/$PACKAGE_NAME.p5m.res
# Проверяем соответствие зависимостей в репозитории
pkglint -c $MANIFEST_PREFIX/solaris-reference -r http://pkg.oracle.com/solaris/release $MANIFEST_PREFIX/$PACKAGE_NAME.p5m.res
# Перепроверяем результат с учётом кэша
pkglint -c $MANIFEST_PREFIX/solaris-reference $MANIFEST_PREFIX/$PACKAGE_NAME.p5m.res

# Выполняем создание репозитория под проект
pkgrepo create $MANIFEST_PREFIX/$PACKAGE_NAME-repository
# Проверяем наличие созданного репозитория
ls $MANIFEST_PREFIX/$PACKAGE_NAME-repository

# Переходим в каталог с репозиторием
cd $MANIFEST_PREFIX

# Выполняем установку автора репозитория
pkgrepo -s $PACKAGE_NAME-repository set publisher/prefix="$PACKAGE_PUBLISHER"
# Выполняем побликацию проекта в репозитории
pkgsend -s $PACKAGE_NAME-repository publish -d $APP_DIR $PACKAGE_NAME.p5m.res

# Выполняем проверку созданного репозитория
pkgrepo verify -s $PACKAGE_NAME-repository
# Выводим информацию о репозитории
pkgrepo info -s $PACKAGE_NAME-repository
# Выводим список пакетов в репозитории
pkgrepo list -s $PACKAGE_NAME-repository
# Выводим проверку всего репозитория
pkg list -afv -g $PACKAGE_NAME-repository

# Выполняем подпись репозитория
pkgsign -s $PACKAGE_NAME-repository -a sha256 '*'

# Выполняем генерацию пакета установки
pkgrecv -s $PACKAGE_NAME-repository -a -d $ROOT/../$PKG_NAME $PACKAGE_NAME

# Переходим в корневой каталог обратно
cd $ROOT/../

# Выполняем удаление директории для сборки пакета
rm -rf $APP_DIR
# Выполняем удаление директории для хранения манифестов
rm -rf $MANIFEST_PREFIX

# Выводим сообщение об удачной сборке
echo "Successfully created package $PKG_NAME"
echo "To install the application, please perform a:"
echo ""
echo "$ sudo pkg install -g $PKG_NAME $PACKAGE_NAME"
echo ""
