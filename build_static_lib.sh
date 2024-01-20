#!/usr/bin/env bash

# Получаем корневую дирректорию
readonly ROOT=$(cd "$(dirname "$0")" && pwd)

# Каталог статических библиотек
readonly THIRD_PARTY="$ROOT/third_party/lib"

# Получаем версию OS
OS=$(uname -a | awk '{print $1}')

if [[ $OS =~ "MINGW64" ]]; then
	OS="Windows"
fi

# Адрес собранной библиотеки
LIB=$1

# Если библиотека не собрана
if [ ! -f "$LIB" ]; then
	echo "Library was not build"
	exit 1
fi

# Получаем название файла
readonly FILENAME=$(basename ${LIB})

# Если такая библиотека уже есть в сборке
if [ -f "$THIRD_PARTY/$FILENAME" ]; then
	# Выполняем удаление старой версии библиотеки
	rm "$THIRD_PARTY/$FILENAME"
fi

# Выполняем копирование библиотеки
cp "$LIB" "$THIRD_PARTY/$FILENAME" || exit 1

# Список модулей для сборки итоговой библиотеки
MODULES=""
# Переходим в каталог с библиотеками
cd "$THIRD_PARTY"

# Расширение файла объекта
OBJECT_NAME="o"
# Расширение файла библиотеки
LIBRARY_NAME="a"

# Если операционной системой является Windows
if [ $OS = "Windows" ]; then
	# Расширение файла объекта
	OBJECT_NAME="obj"
	# Расширение файла библиотеки
	LIBRARY_NAME="lib"
fi

# Регулярное выражение поиска
readonly REG=".*\.${OBJECT_NAME}$"

# Индекс текущей библиотеки
INDEX=0
# Выполняем формирование последовательности списка модулей
for i in $(ar -t $FILENAME | grep "$REG");
do
	# Получаем название модуля
	MODULE=$i
	# Выводим название модуля
	echo "Module: $MODULE in $FILENAME"

	# Выполняем извлечение модуля из архива
	ar -xv $FILENAME "$MODULE"
	# Выполняем удаление модуля в архиве
	ar -dv $FILENAME "$MODULE"

	# Выполняем переименование модуля
	mv "${MODULE%.*}.$OBJECT_NAME" "${MODULE%.*}_$INDEX.$OBJECT_NAME"
	
	# Если список модулей не заполнен
	if [ ! -n "$MODULES" ]; then
		MODULES="${MODULE%.*}_$INDEX.$OBJECT_NAME"
	# Если список модулей уже назполнен
	else
		MODULES="$MODULES ${MODULE%.*}_$INDEX.$OBJECT_NAME"
	fi

	# Выполняем увеличение индекса
	INDEX=$((INDEX+1))
done

# Индекс текущей библиотеки
INDEX=0
# Выполняем копирование библиотеки зависимостей
cp libdependence.$LIBRARY_NAME libdependence.tmp.$LIBRARY_NAME
# Выполняем формирование последовательности списка модулей
for i in $(ar -t libdependence.tmp.$LIBRARY_NAME | grep "$REG");
do
	# Получаем название модуля
	MODULE=$i
	# Выводим название модуля
	echo "Module: $MODULE in libdependence.$LIBRARY_NAME"

	# Выполняем извлечение модуля из архива
	ar -xv libdependence.tmp.$LIBRARY_NAME "$MODULE"
	# Выполняем удаление модуля в архиве
	ar -dv libdependence.tmp.$LIBRARY_NAME "$MODULE"

	# Выполняем переименование модуля
	mv "${MODULE%.*}.$OBJECT_NAME" "${MODULE%.*}_$INDEX.$OBJECT_NAME"
	
	# Если список модулей не заполнен
	if [ ! -n "$MODULES" ]; then
		MODULES="${MODULE%.*}_$INDEX.$OBJECT_NAME"
	# Если список модулей уже назполнен
	else
		MODULES="$MODULES ${MODULE%.*}_$INDEX.$OBJECT_NAME"
	fi

	# Выполняем увеличение индекса
	INDEX=$((INDEX+1))
done

# Удаляем временную библиотеку
rm libdependence.tmp.$LIBRARY_NAME || exit 1

# Если список модулей не получен
if [ ! -n "$MODULES" ]; then
	echo "Danube library is not build"
	exit 1
fi

# Удаляем все старые библиотеки
rm $FILENAME

# Производим пересборку всех зависимых библиотек
for i in $(ls . | grep "$REG");
do
	# Если операционной системой является Windows
	if [ $OS = "Windows" ]; then
		# Выполняем сборку новой статической библиотеки
		ar -crv $FILENAME "$i" || exit 1
	# Если операционной системой является Unix-подобная ОС
	else
		# Выполняем сборку новой статической библиотеки
		ar -cruv $FILENAME "$i" || exit 1
	fi
	# Выполняем удаление уже добавленный модуль
	rm "$i" || exit 1
done

# Выполняем запуск библиотеки
ranlib $FILENAME

# Если операционной системой является Unix-подобная ОС
if [ ! $OS = "Windows" ]; then
	# Удаляем файл разметки
	rm -f "__.SYMDEF"
	rm -f "__.SYMDEF SORTED"
fi

# Удаляем исходную библиотеку
rm $LIB

# Копируем полученную библиотеку
mv "$THIRD_PARTY/$FILENAME" $LIB
