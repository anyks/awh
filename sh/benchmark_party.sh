#!/usr/bin/env bash

# Получаем корневую дирректорию
readonly ROOT=$(cd "$(dirname "$0")" && pwd)

# Скачиваем архив с исходниками libev
curl https://dist.schmorp.de/libev/libev-4.33.tar.gz -o $ROOT/../submodules/libev-4.33.tar.gz
# Переводим в директорию с сабмодулями
cd $ROOT/../submodules
# Распаковываем архив с исходниками libev
tar -xzf libev-4.33.tar.gz
# Переименовываем директорию с исходниками libev
mv $ROOT/../submodules/libev-4.33 $ROOT/../submodules/libev
# Удаляем архив с исходниками libev
rm $ROOT/../submodules/libev-4.33.tar.gz

# Переводим в корневую директорию
cd $ROOT

# Выполняем пересборку сабмодуля libuv
$ROOT/submodule.sh remove libuv
$ROOT/submodule.sh add libuv https://github.com/libuv/libuv.git

# Выполняем пересборку сабмодуля libevent
$ROOT/submodule.sh remove libevent
$ROOT/submodule.sh add libevent https://github.com/libevent/libevent.git

# Выполняем пересборку сабмодуля ngtcp2
$ROOT/submodule.sh remove ngtcp2
$ROOT/submodule.sh add ngtcp2 https://github.com/ngtcp2/ngtcp2.git

# Выполняем пересборку сабмодуля nghttp3
$ROOT/submodule.sh remove nghttp3
$ROOT/submodule.sh add nghttp3 https://github.com/ngtcp2/nghttp3.git

# Выполняем пересборку сабмодуля nghttp2
$ROOT/submodule.sh remove nghttp2
$ROOT/submodule.sh add nghttp2 https://github.com/nghttp2/nghttp2.git

# Выводим список добавленных модулей
cat $ROOT/../.gitmodules
