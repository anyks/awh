# Сверка QUIC с внешней реализацией

Два минимальных эндпоинта на основе [ngtcp2](https://github.com/ngtcp2/ngtcp2)
для проверки транспорта QUIC модуля AWH против чужой реализации. Клиент и сервер
переведены на бэкенд boringssl из состава проекта и на `poll()`, поэтому кроме
подмодулей проекта им ничего не требуется.

## Сборка

Библиотеки ngtcp2 собираются из подмодуля. Наличие поддержки QUIC в boringssl
ngtcp2 проверяет через `check_cxx_symbol_exists(SSL_set_quic_early_data_context)`,
и проверка не проходит по двум причинам, ни одна из которых к самому символу
отношения не имеет: `openssl/ssl.h` тянет `openssl/span.h`, требующий C++17,
а статические архивы boringssl написаны на C++ и без `libc++` не линкуются.
Символ в нашей ревизии boringssl присутствует, и обе помехи снимаются
параметрами сборки.

```sh
ROOT=$(pwd)
cmake -S submodules/ngtcp2 -B build-ngtcp2 -DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_CXX_FLAGS=-std=c++17 \
	-DENABLE_BORINGSSL=ON -DENABLE_STATIC_LIB=ON -DENABLE_SHARED_LIB=OFF \
	-DBORINGSSL_INCLUDE_DIR=$ROOT/submodules/boringssl/include \
	-DBORINGSSL_LIBRARIES="$ROOT/submodules/boringssl/build/libssl.a;$ROOT/submodules/boringssl/build/libcrypto.a;c++"
cmake --build build-ngtcp2 -j8

for NAME in ngtcp2-client ngtcp2-server; do
	cc -O1 -g -Wall -Wextra -o build-ngtcp2/$NAME tools/interop/$NAME.c \
		-I submodules/ngtcp2/lib/includes -I build-ngtcp2/lib/includes \
		-I submodules/ngtcp2/crypto/includes -I submodules/boringssl/include \
		build-ngtcp2/crypto/boringssl/libngtcp2_crypto_boringssl.a build-ngtcp2/lib/libngtcp2.a \
		submodules/boringssl/build/libssl.a submodules/boringssl/build/libcrypto.a -lc++
done
```

Если полная сборка ngtcp2 не нужна, проверку можно обойти флагом
`-DENABLE_LIB_ONLY=ON`: библиотека и крипто-бэкенд соберутся, а примеры,
которым всё равно нужны libev и nghttp3, собираться не будут.

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
../build-ngtcp2/ngtcp2-client --version 0x6b3343cf    # согласование версии (RFC 9000 §6)
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
../build-ngtcp2/ngtcp2-server --vneg other &          # согласование без версии клиента
../build-ngtcp2/ngtcp2-server --vneg same &           # согласование с версией клиента
../build-ngtcp2/ngtcp2-server --vneg badcid &         # согласование с чужими идентификаторами
```

Передача объёма и возобновление сессии проверяются параметрами семпла клиента:

```sh
./samples/proto/quic/proto-quic-client --size 4194304        # блок с побайтовой сверкой эха
./samples/proto/quic/proto-quic-client --state /tmp/quic.st  # сохранение и подстановка билета
```

Второй запуск с тем же `--state` возобновляет сессию и отправляет запрос
ранними данными: клиент сообщает `ранние данные=приняты`, сервер стенда -
`возобновление=да, ранние данные=да`. Чтобы это было воспроизводимо, стенд
задаёт постоянный ключ шифрования билетов и контекст ранних данных: со
случайным ключом билет прошлого запуска расшифровать было бы нечем, а без
контекста boringssl ранние данные для QUIC не предлагает вовсе.

Отрицательные прогоны `--vneg` проверяют защиту от понижения версии
(RFC 9000 §6.2): согласование, где предложена версия, которую клиент уже
использует, либо где не совпадают идентификаторы соединения, обязано быть
отброшено, а рукопожатие - продолжено повторной отправкой Initial.

Отрицательные прогоны `--broken-retry` проверяют, что клиент AWH выполняет
сверку идентификаторов после Retry (RFC 9000 §7.3) и рвёт соединение с
`TRANSPORT_PARAMETER_ERROR`, а не продолжает рукопожатие.

Каждый эндпоинт печатает итог сверки и возвращает ненулевой код при сбое,
поэтому прогоны годятся для запуска из скрипта.
