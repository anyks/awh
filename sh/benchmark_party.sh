#!/usr/bin/env bash

# Получаем корневую дирректорию
readonly ROOT=$(cd "$(dirname "$0")" && pwd)

# Удаляем директорию с исходниками libev
rm -rf $ROOT/../submodules/libev
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

# Выполняем пересборку сабмодуля cityhash
$ROOT/submodule.sh remove cityhash
$ROOT/submodule.sh add cityhash https://github.com/google/cityhash.git

# Выполняем пересборку сабмодуля PCRE2
$ROOT/submodule.sh remove pcre2
$ROOT/submodule.sh add pcre2 https://github.com/PCRE2Project/pcre2.git

# Выполняем пересборку сабмодуля rapidxml
$ROOT/submodule.sh remove rapidxml
$ROOT/submodule.sh add rapidxml https://github.com/dwd/rapidxml.git

# Выполняем пересборку сабмодуля pugixml
$ROOT/submodule.sh remove pugixml
$ROOT/submodule.sh add pugixml https://github.com/zeux/pugixml.git

# Выполняем пересборку сабмодуля xerces
$ROOT/submodule.sh remove xerces
$ROOT/submodule.sh add xerces https://github.com/AaronNGray/xerces.git

# Выполняем пересборку сабмодуля libexpat
$ROOT/submodule.sh remove libexpat
$ROOT/submodule.sh add libexpat https://github.com/libexpat/libexpat.git

# Выполняем пересборку сабмодуля libxml2
$ROOT/submodule.sh remove libxml2
$ROOT/submodule.sh add libxml2 https://gitlab.gnome.org/GNOME/libxml2.git

# Выполняем пересборку сабмодуля tinyxml2
$ROOT/submodule.sh remove tinyxml2
$ROOT/submodule.sh add tinyxml2 https://github.com/leethomason/tinyxml2.git

# Выполняем пересборку сабмодуля inih
$ROOT/submodule.sh remove inih
$ROOT/submodule.sh add inih https://github.com/benhoyt/inih.git

# Выполняем пересборку сабмодуля iniparser
$ROOT/submodule.sh remove iniparser
$ROOT/submodule.sh add iniparser https://github.com/ndevilla/iniparser.git

# Выполняем пересборку сабмодуля simpleini
$ROOT/submodule.sh remove simpleini
$ROOT/submodule.sh add simpleini https://github.com/brofield/simpleini.git

# Выполняем пересборку сабмодуля mINI
$ROOT/submodule.sh remove mINI
$ROOT/submodule.sh add mINI https://github.com/metayeti/mINI.git

# Выполняем пересборку сабмодуля boost
$ROOT/submodule.sh remove boost
$ROOT/submodule.sh add boost https://github.com/boostorg/boost.git

# Выполняем пересборку сабмодуля seastar
$ROOT/submodule.sh remove seastar
$ROOT/submodule.sh add seastar https://github.com/scylladb/seastar.git

# Выполняем пересборку сабмодуля csv2
$ROOT/submodule.sh remove csv2
$ROOT/submodule.sh add csv2 https://github.com/p-ranav/csv2.git

# Выполняем пересборку сабмодуля libcsv
$ROOT/submodule.sh remove libcsv
$ROOT/submodule.sh add libcsv https://github.com/rgamble/libcsv.git

# Выполняем пересборку сабмодуля rapidcsv
$ROOT/submodule.sh remove rapidcsv
$ROOT/submodule.sh add rapidcsv https://github.com/d99kris/rapidcsv.git

# Выполняем пересборку сабмодуля csv-parser
$ROOT/submodule.sh remove csv-parser
$ROOT/submodule.sh add csv-parser https://github.com/vincentlaucsb/csv-parser.git

# Выполняем пересборку сабмодуля fast-cpp-csv-parser
$ROOT/submodule.sh remove fast-cpp-csv-parser
$ROOT/submodule.sh add fast-cpp-csv-parser https://github.com/ben-strasser/fast-cpp-csv-parser.git

# Выполняем пересборку сабмодуля nlohmann
$ROOT/submodule.sh remove nlohmann
$ROOT/submodule.sh add nlohmann https://github.com/nlohmann/json.git

# Выполняем пересборку сабмодуля yyjson
$ROOT/submodule.sh remove yyjson
$ROOT/submodule.sh add yyjson https://github.com/ibireme/yyjson.git

# Выполняем пересборку сабмодуля simdjson
$ROOT/submodule.sh remove simdjson
$ROOT/submodule.sh add simdjson https://github.com/simdjson/simdjson.git

# Выполняем пересборку сабмодуля rapidjson
$ROOT/submodule.sh remove rapidjson
$ROOT/submodule.sh add rapidjson https://github.com/Tencent/rapidjson.git

# Выполняем пересборку сабмодуля libyaml
$ROOT/submodule.sh remove libyaml
$ROOT/submodule.sh add libyaml https://github.com/yaml/libyaml.git

# Выполняем пересборку сабмодуля libfyaml
$ROOT/submodule.sh remove libfyaml
$ROOT/submodule.sh add libfyaml https://github.com/pantoniou/libfyaml.git

# Выполняем пересборку сабмодуля rapidyaml
$ROOT/submodule.sh remove rapidyaml
$ROOT/submodule.sh add rapidyaml https://github.com/biojppm/rapidyaml.git

# Выполняем пересборку сабмодуля yaml-cpp
$ROOT/submodule.sh remove yaml-cpp
$ROOT/submodule.sh add yaml-cpp https://github.com/jbeder/yaml-cpp.git

# Выполняем пересборку сабмодуля fkYAML
$ROOT/submodule.sh remove fkYAML
$ROOT/submodule.sh add fkYAML https://github.com/fktn-k/fkYAML.git

# Выполняем пересборку сабмодуля libcbor
$ROOT/submodule.sh remove libcbor
$ROOT/submodule.sh add libcbor https://github.com/PJK/libcbor.git

# Выполняем пересборку сабмодуля msgpack-c
$ROOT/submodule.sh remove msgpack-c
$ROOT/submodule.sh add msgpack-c https://github.com/msgpack/msgpack-c.git

# Выполняем пересборку сабмодуля pprof
$ROOT/submodule.sh remove pprof
$ROOT/submodule.sh add pprof https://github.com/google/pprof.git

# Выполняем пересборку сабмодуля mimalloc
$ROOT/submodule.sh remove mimalloc
$ROOT/submodule.sh add mimalloc https://github.com/microsoft/mimalloc.git

# Выполняем пересборку сабмодуля jemalloc
$ROOT/submodule.sh remove jemalloc
$ROOT/submodule.sh add jemalloc https://github.com/jemalloc/jemalloc.git

# Выводим список добавленных модулей
cat $ROOT/../.gitmodules
