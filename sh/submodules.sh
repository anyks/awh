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

# Выполняем пересборку сабмодуля lizard
$ROOT/submodule.sh remove lizard
$ROOT/submodule.sh add lizard https://gitflic.ru/project/third_party/lizard.git

# Выполняем пересборку сабмодуля density
$ROOT/submodule.sh remove density
$ROOT/submodule.sh add density https://gitflic.ru/project/third_party/density.git

# Выполняем пересборку сабмодуля snappy
$ROOT/submodule.sh remove snappy
$ROOT/submodule.sh add snappy https://gitflic.ru/project/third_party/snappy.git

# Выполняем пересборку сабмодуля miniupnpc
$ROOT/submodule.sh remove miniupnpc
$ROOT/submodule.sh add miniupnpc https://gitflic.ru/project/third_party/miniupnp.git

# Выполняем пересборку сабмодуля brotli
$ROOT/submodule.sh remove brotli
$ROOT/submodule.sh add brotli https://gitflic.ru/project/third_party/brotli.git

# Выполняем пересборку сабмодуля BoringSSL
$ROOT/submodule.sh remove boringssl
$ROOT/submodule.sh add boringssl https://gitflic.ru/project/third_party/boringssl.git

# Выполняем пересборку сабмодуля gperftools
$ROOT/submodule.sh remove gperftools
$ROOT/submodule.sh add gperftools https://gitflic.ru/project/third_party/gperftools.git

# Выводим список добавленных модулей
cat $ROOT/../.gitmodules
