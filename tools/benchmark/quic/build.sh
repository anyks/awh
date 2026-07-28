#!/usr/bin/env bash

# Получаем корневую дирректорию репозитория (каталог скрипта на три уровня ниже)
readonly ROOT=$(cd "$(dirname "$0")/../../.." && pwd)

# Каталог стендов сравнения
readonly STANDS="$ROOT/tools/benchmark/quic"

# Каталог собранных стендов
readonly OUTPUT="${1:-/tmp/rival-quic}"

# Каталог сборки библиотеки AWH с оптимизацией
readonly RELEASE="${2:-$ROOT/build-release}"

# Каталог сборки библиотеки ngtcp2
readonly NGTCP2_BUILD="$ROOT/build-ngtcp2"

# Каталог исходных текстов подмодуля ngtcp2
#
# Путь берётся из .gitmodules: подмодуль зарегистрирован вложенным
# (submodules/submodules/ngtcp2), а на запасной случай подставляется он же
NGTCP2_SRC=$(git -C "$ROOT" config --file "$ROOT/.gitmodules" --get-regexp 'submodule\..*ngtcp2\.path' 2>/dev/null | awk '{print $2}' | head -1)
if [ -z "$NGTCP2_SRC" ] || [ ! -d "$ROOT/$NGTCP2_SRC" ]; then
	# Подставляем канонический вложенный путь подмодуля
	NGTCP2_SRC="submodules/submodules/ngtcp2"
fi
readonly NGTCP2_SRC="$ROOT/$NGTCP2_SRC"

# Флаги сборки стендов
#
# Уровень оптимизации совпадает с уровнем сборки библиотеки в режиме Release:
# сравнивать реализации, собранные с разной оптимизацией, бессмысленно
readonly FLAGS="-O3 -DNDEBUG -Wall -Wextra"

# Пути к встроенному в проект бэкенду BoringSSL
readonly BSSL_INC="$ROOT/submodules/boringssl/include"
readonly BSSL_SSL="$ROOT/submodules/boringssl/build/libssl.a"
readonly BSSL_CRYPTO="$ROOT/submodules/boringssl/build/libcrypto.a"

# Пути к тестовому сертификату и приватному ключу сервера
readonly CERT="$ROOT/sh/certificates/server/cert.pem"
readonly KEY="$ROOT/sh/certificates/server/key.pem"

# Если бэкенд BoringSSL не собран
if [ ! -f "$BSSL_SSL" ] || [ ! -f "$BSSL_CRYPTO" ]; then
	# Выводим сообщение о необходимости сборки BoringSSL
	echo "Build BoringSSL submodule first (submodules/boringssl/build/{libssl,libcrypto}.a)"
	exit 1
fi

# Выполняем создание каталога собранных стендов
mkdir -p "$OUTPUT" || exit 1

##
# Библиотека ngtcp2 из подмодуля проекта
#
# Собирается против встроенного в проект бэкенда BoringSSL, тем же, что и сама
# библиотека AWH: криптографический слой у обоих стендов общий и из сравнения
# движков транспорта таким образом исключается. Флагом ENABLE_LIB_ONLY обходится
# проверка примеров ngtcp2, которым нужны libev и nghttp3
#
if [ ! -f "$NGTCP2_BUILD/lib/libngtcp2.a" ]; then
	echo "Build \"ngtcp2\" library"
	cmake -S "$NGTCP2_SRC" -B "$NGTCP2_BUILD" -DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_CXX_FLAGS=-std=c++17 -DENABLE_LIB_ONLY=ON -DBUILD_TESTING=OFF \
		-DENABLE_BORINGSSL=ON -DENABLE_STATIC_LIB=ON -DENABLE_SHARED_LIB=OFF \
		-DBORINGSSL_INCLUDE_DIR="$BSSL_INC" \
		-DBORINGSSL_LIBRARIES="$BSSL_SSL;$BSSL_CRYPTO;c++" || exit 1
	cmake --build "$NGTCP2_BUILD" -j8 || exit 1
else
	echo "Skip \"ngtcp2\" library: already built"
fi

##
# Стенд транспорта ngtcp2
#
# Пути к сертификату и ключу сервера задаются при сборке: стенд их только
# загружает до начала измерения, к измеряемой работе рукопожатие отношения не имеет
#
echo "Build \"ngtcp2\""
c++ -std=c++17 $FLAGS \
	-DQUIC_CERT_FILE="\"$CERT\"" -DQUIC_KEY_FILE="\"$KEY\"" \
	-o "$OUTPUT/ngtcp2" "$STANDS/ngtcp2.cpp" \
	-I"$NGTCP2_SRC/lib/includes" -I"$NGTCP2_BUILD/lib/includes" \
	-I"$NGTCP2_SRC/crypto/includes" -I"$BSSL_INC" \
	"$NGTCP2_BUILD/crypto/boringssl/libngtcp2_crypto_boringssl.a" "$NGTCP2_BUILD/lib/libngtcp2.a" \
	"$BSSL_SSL" "$BSSL_CRYPTO" -lc++ 2> /dev/null || exit 1

##
# Стенд транспорта библиотеки AWH
#
# Собирается по той же обвязке замера, что и стенд ngtcp2: разница в обвязке иначе
# оказалась бы неотделима от разницы в самих движках транспорта. Установление
# соединения и криптографический контекст берутся из окружения набора бенчмарков
# `benchmark/proto/quic`. Стенд собирается без аллокатора TcMalloc, чтобы учёт
# выделений вёлся тем же системным распределителем, что и у стенда ngtcp2
#
if [ -f "$RELEASE/libawh.a" ]; then
	echo "Build \"awh\""
	c++ -std=gnu++17 $FLAGS -pthread -DAWH_STATICLIB \
		-Wno-reserved-user-defined-literal -Wno-implicitly-unsigned-literal -Wno-unused-command-line-argument \
		-o "$OUTPUT/awh" "$STANDS/awh.cpp" "$ROOT/benchmark/proto/quic/quic.cpp" \
		-I"$ROOT/tools/benchmark/syscount" \
		-I"$ROOT/contrib/include" -I"$ROOT/third_party/include/pcre2" -I"$ROOT/third_party/include" -isystem "$ROOT/include" \
		"$RELEASE/libawh.a" "$ROOT/third_party/lib/libdependence.a" "$ROOT/third_party/lib/libcommon.a" \
		$(if [ "$(uname -a | awk '{print $1}')" = "Darwin" ]; then echo "-framework Foundation"; fi) 2> /dev/null || exit 1

	##
	# Стенд транспорта параллельного движка AWH (quic2)
	#
	# Тот же стенд и та же обвязка замера, что и у "awh", но поверх движка awh::quic2 -
	# изолированной ветки рефакторинга памяти. Позволяет мерить прогресс quic2 против
	# ngtcp2 и исходного quic, не трогая рабочий движок
	#
	echo "Build \"awh2\""
	c++ -std=gnu++17 $FLAGS -pthread -DAWH_STATICLIB \
		-Wno-reserved-user-defined-literal -Wno-implicitly-unsigned-literal -Wno-unused-command-line-argument \
		-o "$OUTPUT/awh2" "$STANDS/awh2.cpp" "$ROOT/benchmark/proto/quic/quic2.cpp" \
		-I"$ROOT/tools/benchmark/syscount" \
		-I"$ROOT/contrib/include" -I"$ROOT/third_party/include/pcre2" -I"$ROOT/third_party/include" -isystem "$ROOT/include" \
		"$RELEASE/libawh.a" "$ROOT/third_party/lib/libdependence.a" "$ROOT/third_party/lib/libcommon.a" \
		$(if [ "$(uname -a | awk '{print $1}')" = "Darwin" ]; then echo "-framework Foundation"; fi) 2> /dev/null || exit 1
else
	# Выводим сообщение об отсутствии сборки библиотеки с оптимизацией
	echo "Skip \"awh\": build the library first with"
	echo "  cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_BUILD_BENCHMARKS=YES"
	echo "  cmake --build build-release --target awh_BENCHMARK_proto -j 8"
fi

# Выводим сообщение о завершении сборки стендов
echo "Stands are placed in \"$OUTPUT\""
