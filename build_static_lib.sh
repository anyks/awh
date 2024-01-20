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

# Расширение файла объекта
OBJECT_NAME="o"

# Если операционной системой является Windows
if [ $OS = "Windows" ]; then
	# Расширение файла объекта
	OBJECT_NAME="obj"
fi

# Получаем название файла
readonly FILENAME=$(basename ${LIB})

# Если такая библиотека уже есть в сборке
if [ -f "$THIRD_PARTY/$FILENAME" ]; then
	# Выполняем удаление старой версии библиотеки
	rm "$THIRD_PARTY/$FILENAME"
fi

##
# Функция упаковка модулей в библиотеку
#
intract(){
	# Производим пересборку всех зависимых библиотек
	for i in $(ls . | grep ".*\.${OBJECT_NAME}$");
	do
		# Выполняем сборку новой статической библиотеки
		ar -crUuv "$1" "$i" || exit 1
		# Выполняем удаление уже добавленный модуль
		rm "$i" || exit 1
	done

	# Выполняем запуск библиотеки
	ranlib "$1"

	# Если операционной системой является Unix-подобная ОС
	if [ ! $OS = "Windows" ]; then
		# Удаляем файл разметки
		rm -f "__.SYMDEF"
		rm -f "__.SYMDEF SORTED"
	fi
}

##
# Функция извлечения модулей из библиотеки
#
extract(){
	# Индекс текущей библиотеки
	INDEX=0
	# Выводим сообщение
	echo "Extract \"$1\""
	# Выполняем формирование последовательности списка модулей
	for i in $(ar -t $1 | grep ".*\.$2$");
	do
		# Выводим название модуля
		echo "Module: $i in $1"

		# Выполняем извлечение модуля из архива
		ar -xv $1 "$i"
		# Выполняем удаление модуля в архиве
		ar -dv $1 "$i"

		# Выполняем переименование модуля
		mv "${i%.*}.$2" "${i%.*}_$INDEX.$OBJECT_NAME"

		# Выполняем увеличение индекса
		INDEX=$((INDEX+1))
	done
	# Удалем архив статической библиотеки
	rm "$1" || exit 1
}

# Выполняем копирование библиотеки
cp "$LIB" "$THIRD_PARTY/$FILENAME" || exit 1

# Переходим в каталог с библиотеками
cd "$THIRD_PARTY"

# Если операционной системой является Windows
if [ $OS = "Windows" ]; then
	# Извлекаем все модули из библиотеки
	extract $FILENAME "obj"
	# Выполняем копирование библиотеки зависимостей
	cp libdependence.lib libdependence.tmp.lib
	# Извлекаем все модули из библиотеки
	extract libdependence.tmp.lib "obj"
# Если операционной системой является Unix-подобная ОС
else
	# Извлекаем все модули из библиотеки
	extract $FILENAME "o"
	# Выполняем копирование библиотеки зависимостей
	cp libdependence.a libdependence.tmp.a
	# Извлекаем все модули из библиотеки
	extract libdependence.tmp.a "o"
fi

# Выполняем сборку библиотеки
intract $FILENAME

# Удаляем исходную библиотеку
rm $LIB

# Копируем полученную библиотеку
mv "$THIRD_PARTY/$FILENAME" $LIB
