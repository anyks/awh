# Сверка QUIC с внешней реализацией

Два минимальных эндпоинта на основе [ngtcp2](https://github.com/ngtcp2/ngtcp2)
для проверки транспорта QUIC модуля AWH против чужой реализации. Клиент и сервер
переведены на бэкенд boringssl из состава проекта и на `poll()`, поэтому кроме
подмодулей проекта им ничего не требуется.

## Сборка

Библиотеки ngtcp2 собираются из подмодуля. Проверка наличия QUIC в boringssl
у ngtcp2 опирается на `SSL_set_quic_early_data_context`, которого в нашей ревизии
boringssl уже нет, поэтому используется `ENABLE_LIB_ONLY`: сам бэкенд собирается,
а проверка пропускается.

```sh
ROOT=$(pwd)
cmake -S submodules/ngtcp2 -B build-ngtcp2 -DCMAKE_BUILD_TYPE=Release \
	-DENABLE_LIB_ONLY=ON -DENABLE_BORINGSSL=ON -DENABLE_STATIC_LIB=ON -DENABLE_SHARED_LIB=OFF \
	-DBORINGSSL_INCLUDE_DIR=$ROOT/submodules/boringssl/include \
	-DBORINGSSL_LIBRARIES="$ROOT/submodules/boringssl/build/libssl.a;$ROOT/submodules/boringssl/build/libcrypto.a"
cmake --build build-ngtcp2 -j8

for NAME in ngtcp2-client ngtcp2-server; do
	cc -O1 -g -Wall -Wextra -o build-ngtcp2/$NAME tools/interop/$NAME.c \
		-I submodules/ngtcp2/lib/includes -I build-ngtcp2/lib/includes \
		-I submodules/ngtcp2/crypto/includes -I submodules/boringssl/include \
		build-ngtcp2/crypto/boringssl/libngtcp2_crypto_boringssl.a build-ngtcp2/lib/libngtcp2.a \
		submodules/boringssl/build/libssl.a submodules/boringssl/build/libcrypto.a -lc++
done
```

## Прогон

Оба эндпоинта запускаются из каталога `build`: пути к тестовым сертификатам
заданы относительно него, как и в семплах.

Клиент ngtcp2 против сервера AWH:

```sh
./samples/proto/quic/proto-quic-server &
../build-ngtcp2/ngtcp2-client --size 4194304          # передача с побайтовой сверкой эха
../build-ngtcp2/ngtcp2-client --datagram              # датаграммы приложения (RFC 9221)
../build-ngtcp2/ngtcp2-client --size 2097152 --key-update  # обновление ключей (RFC 9001 §6)
../build-ngtcp2/ngtcp2-client --size 2097152 --migrate     # миграция пути (RFC 9000 §9)
```

Клиент AWH против сервера ngtcp2:

```sh
../build-ngtcp2/ngtcp2-server &
./samples/proto/quic/proto-quic-client

../build-ngtcp2/ngtcp2-server --retry &               # проверка адреса пакетом Retry
../build-ngtcp2/ngtcp2-server --broken-retry ocid &   # искажён original_destination_connection_id
../build-ngtcp2/ngtcp2-server --broken-retry noscid & # опущен retry_source_connection_id
../build-ngtcp2/ngtcp2-server --broken-retry scid &   # искажён retry_source_connection_id
../build-ngtcp2/ngtcp2-server --alpn hq-interop &     # несовпадение ALPN
```

Отрицательные прогоны `--broken-retry` проверяют, что клиент AWH выполняет
сверку идентификаторов после Retry (RFC 9000 §7.3) и рвёт соединение с
`TRANSPORT_PARAMETER_ERROR`, а не продолжает рукопожатие.

Каждый эндпоинт печатает итог сверки и возвращает ненулевой код при сбое,
поэтому прогоны годятся для запуска из скрипта.
