#!/usr/bin/env bash

# Получаем корневую дирректорию
readonly ROOT=$(cd "$(dirname "$0")" && pwd)

# Выполняем пересборку сабмодуля lz4
$ROOT/submodule.sh remove lz4
$ROOT/submodule.sh add lz4 https://gitflic.ru/project/third_party/lz4.git

# Выполняем пересборку сабмодуля bz2
$ROOT/submodule.sh remove bz2
$ROOT/submodule.sh add bz2 https://gitflic.ru/project/third_party/bzip2.git

# Выполняем пересборку сабмодуля lzma
$ROOT/submodule.sh remove lzma
$ROOT/submodule.sh add lzma https://gitflic.ru/project/third_party/lzma.git

# Выполняем пересборку сабмодуля zstd
$ROOT/submodule.sh remove zstd
$ROOT/submodule.sh add zstd https://gitflic.ru/project/third_party/zstd.git

# Выполняем пересборку сабмодуля zlib
$ROOT/submodule.sh remove zlib
$ROOT/submodule.sh add zlib https://gitflic.ru/project/third_party/zlib.git

# Выполняем пересборку сабмодуля brotli
$ROOT/submodule.sh remove brotli
$ROOT/submodule.sh add brotli https://gitflic.ru/project/third_party/brotli.git

# Выполняем пересборку сабмодуля pcre2
$ROOT/submodule.sh remove pcre2
$ROOT/submodule.sh add pcre2 https://gitflic.ru/project/third_party/pcre2.git

# Выполняем пересборку сабмодуля openssl
$ROOT/submodule.sh remove openssl
$ROOT/submodule.sh add openssl https://gitflic.ru/project/third_party/openssl.git

# Выполняем пересборку сабмодуля nghttp2
$ROOT/submodule.sh remove nghttp2
$ROOT/submodule.sh add nghttp2 https://gitflic.ru/project/third_party/nghttp2.git

# Выполняем пересборку сабмодуля gperftools
$ROOT/submodule.sh remove gperftools
$ROOT/submodule.sh add gperftools https://gitflic.ru/project/third_party/gperftools.git

# Выводим список добавленных модулей
cat $ROOT/.gitmodules
