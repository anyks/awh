#!/usr/bin/env bash

# Получаем корневую дирректорию
readonly ROOT=$(cd "$(dirname "$0")" && pwd)

# Каталог статических библиотек
THIRD_PARTY="$ROOT/third_party/lib"

# Если на вход получен каталог с библиотеками
if [[ -d $1 ]]; then
	# Каталог статических библиотек
	THIRD_PARTY="$1"
fi

# Получаем версию OS
OS=$(uname -a | awk '{print $1}')

if [[ $OS =~ "MINGW64" ]]; then
	OS="Windows"
fi

# Название библиотеки
LIBNAME="dependence"

# Выполняем переход в каталог статических библиотек
cd $THIRD_PARTY

# Список модулей для сборки итоговой библиотеки
MODULES=""

# Расширение файла объекта
OBJECT_NAME="o"

# Если операционной системой является Windows
if [ $OS = "Windows" ]; then
	# Расширение файла объекта
	OBJECT_NAME="obj"
fi

# Регулярное выражение поиска
readonly REG=".*\.${OBJECT_NAME}$"

# Производим пересборку всех зависимых библиотек
for i in $(ls . | grep \\.a$);
do
	# Индекс текущей библиотеки
	INDEX=0
	# Выводим сообщение
	echo "Extract \"$i\""
	# Выполняем формирование последовательности списка модулей
	for j in $(ar -t $i | grep $REG);
	do
		# Получаем название модуля
		MODULE=$j
		# Выводим название модуля
		echo "Module: $MODULE in $i"

		# Выполняем извлечение модуля из архива
		ar -xv $i "$MODULE"
		# Выполняем удаление модуля в архиве
		ar -dv $i "$MODULE"

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
	# Удалем архив статической библиотеки
	rm "$i" || exit 1
done

# Производим пересборку всех зависимых библиотек
for i in $(ls . | grep "$REG");
do
	# Если операционной системой является Windows
	if [ $OS = "Windows" ]; then
		# Выполняем сборку новой статической библиотеки
		ar -crv "lib$LIBNAME.a" "$i" || exit 1
	# Если операционной системой является Unix-подобная ОС
	else
		# Выполняем сборку новой статической библиотеки
		ar -cruv "lib$LIBNAME.a" "$i" || exit 1
	fi
done

# Выполняем запуск библиотеки
ranlib "lib$LIBNAME.a"

# Выполняем удаление всех извлечённых модулей
rm -rf *.$OBJECT_NAME

# Если операционной системой является Unix-подобная ОС
if [ ! $OS = "Windows" ]; then
	# Удаляем файл разметки
	rm -f "__.SYMDEF"
	rm -f "__.SYMDEF SORTED"
fi

# Переходим обратно
cd "$ROOT" || exit 1

printf "\n****************************************"
printf "\n************   Success merge static libs!!!   ************"
printf "\n****************************************"
printf "\n\n\n"
