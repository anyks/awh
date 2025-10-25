#!/usr/bin/env bash

# Полный адрес к текущему каталогу
readonly ROOT=$(cd "$(dirname "$0")" && pwd)

# Определяем где находится OpenSSL установленный в системе
OPENSSL_BIN="/usr/bin/env openssl"

# Получаем каталог со скриптами
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"

touch ca-db-index
echo 01 > ca-db-serial

# Устанавливаем доменное имя по умолчанию
if [ -n "$1" ]; then
	DOM_NAME=$1
else
	DOM_NAME="example.net"
fi

# Усдаляем старые каталоги сертификатов
rm -rf "$ROOT/client"
rm -rf "$ROOT/server"

# Создаём новые каталоги сертификатов
mkdir "$ROOT/client"
mkdir "$ROOT/server"

# Генерируем сертификат авторизации
$OPENSSL_BIN req -nodes -x509 -newkey rsa:2048 -days 365 -keyout "$ROOT/ca-key.pem" -out "$ROOT/ca-cert.pem" -subj "/C=RU/ST=Moscow/L=Moscow/O=$DOM_NAME, Inc/OU=ANYKS/CN=ca.$DOM_NAME"
# Генерируем сертификат сервера
$OPENSSL_BIN req -nodes -new -newkey rsa:2048 -keyout "$ROOT/server/key.pem" -out "$ROOT/server.csr" -subj "/C=RU/ST=Moscow/L=Moscow/O=$DOM_NAME, Inc/OU=ANYKS/CN=server.$DOM_NAME"
# Генерируем спртификат клиента
$OPENSSL_BIN req -nodes -new -newkey rsa:2048 -keyout "$ROOT/client/key.pem" -out "$ROOT/client.csr" -subj "/C=RU/ST=Moscow/L=Moscow/O=$DOM_NAME, Inc/OU=ANYKS/CN=client.$DOM_NAME"
# Подписываем сертификат сервера
$OPENSSL_BIN ca -config $SCRIPT_DIR/ca.conf -days 365 -in "$ROOT/server.csr" -out "$ROOT/server/cert.pem"
# Подписываем сертификат клиента
$OPENSSL_BIN ca -config $SCRIPT_DIR/ca.conf -days 365 -in "$ROOT/client.csr" -out "$ROOT/client/cert.pem"

# Удаляем старые ненужные нам файлы
rm "$ROOT/ca-db-index"
rm "$ROOT/ca-db-index.attr"
rm "$ROOT/ca-db-index.attr.old"
rm "$ROOT/ca-db-index.old"
rm "$ROOT/ca-db-serial"
rm "$ROOT/ca-db-serial.old"
rm "$ROOT/client.csr"
rm "$ROOT/server.csr"
rm "$ROOT/01.pem"
rm "$ROOT/02.pem"
rm "$ROOT/ca-cert.pem"
rm "$ROOT/ca-key.pem"

printf "\n****************************************"
printf "\n************   Success!!!   ************"
printf "\n****************************************"
printf "\n\n\n"
