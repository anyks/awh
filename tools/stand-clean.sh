#!/bin/sh
##
 # Чистка отладочного стенда
 #
 # Сносит в домашнем каталоге всё, кроме одного рабочего дерева, имя которого
 # передано первым доводом. Файлы профиля (начинающиеся с точки) не трогает.
 #
 # Вызов:  sh stand-clean.sh awh
 ##

KEEP="$1"

if [ -z "$KEEP" ]; then
	echo "Не задано имя дерева, которое следует оставить" >&2
	exit 1
fi

cd "$HOME" || exit 1

if [ ! -d "$KEEP" ]; then
	echo "Дерева '$KEEP' в домашнем каталоге нет — чистка отменена" >&2
	exit 1
fi

echo "Оставляю: $KEEP"
echo "Сношу:"

for entry in *; do
	[ "$entry" = "$KEEP" ] && continue
	echo "  $entry"
	rm -rf -- "$entry"
done

echo "Осталось:"
ls -1
echo "Диск:"
df -h . | tail -1
