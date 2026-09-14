#!/bin/sh
##
 # Обновление отладочного стенда до свежего кода и сборка
 #
 # Приводит единственное рабочее дерево стенда к состоянию ветки в origin и
 # собирает заданную цель. Заводить второе дерево под очередную задачу не нужно:
 # именно от этого стенды и зарастали клонами, каждый из которых оставался на
 # своём коммите.
 #
 # Вызов:  sh stand-sync.sh [дерево] [ветка] [цель]
 #
 # Умолчания:  дерево ~/awh, ветка v5, цель awh
 #
 # Правки в дереве СНОСЯТСЯ: стенд отладочный, источник истины — origin.
 # Если в дереве осталось нужное, снять его до вызова.
 ##

TREE="${1:-awh}"
BRANCH="${2:-v5}"
TARGET="${3:-awh}"

# Средства GNU на системах Sun зовутся иначе, чем родные вида SVR4
MAKE=make
command -v gmake > /dev/null 2>&1 && MAKE=gmake

cd "$HOME/$TREE" 2>/dev/null || {
	echo "Дерева '$HOME/$TREE' нет" >&2
	exit 1
}

echo "=== дерево $TREE, ветка $BRANCH ==="
echo "было: $(git log --oneline -1)"

git fetch origin "$BRANCH" || exit 1

##
 # Снос правок обязателен: без него переход между коммитами оставляет
 # в дереве смесь старых и новых файлов, и сборка отказывает на включениях,
 # которых в старом дереве не существует
 ##
git reset --hard "origin/$BRANCH" || exit 1
git submodule update --init --recursive || exit 1

echo "стало: $(git log --oneline -1)"

[ -d build ] || mkdir build
cd build || exit 1

# Настройка нужна лишь когда её не делали или когда список файлов сменился
[ -f CMakeCache.txt ] || cmake -DCMAKE_BUILD_TYPE=Release .. || exit 1

echo "=== сборка цели $TARGET ==="

if $MAKE "$TARGET" -j4; then
	echo "СБОРКА=0"
else
	echo "СБОРКА=1" >&2
	exit 1
fi
