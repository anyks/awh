#!/usr/bin/env bash

# Root dir
readonly ROOT=$(cd "$(dirname "$0")" && pwd)

OPENSSL_BIN="openssl"
#OPENSSL_BIN="/usr/local/opt/openssl@1.1/bin/openssl"

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

touch ca-db-index
echo 01 > ca-db-serial

# Set domain name
if [ -n "$1" ]; then
	DOM_NAME=$1
else
	DOM_NAME="example.net"
fi

# Certificate Authority
$OPENSSL_BIN req -nodes -x509 -newkey rsa:2048 -days 365 -keyout "$ROOT/ca-key.pem" -out "$ROOT/ca-cert.pem" -subj "/C=RU/ST=Moscow/L=Moscow/O=$DOM_NAME, Inc/OU=ANYKS/CN=dtls-ca.$DOM_NAME"

# Server Certificate
$OPENSSL_BIN req -nodes -new -newkey rsa:2048 -keyout "$ROOT/server-key.pem" -out "$ROOT/server.csr" -subj "/C=RU/ST=Moscow/L=Moscow/O=$DOM_NAME, Inc/OU=ANYKS/CN=dtls-server.$DOM_NAME"

# Sign Server Certificate
$OPENSSL_BIN ca -config $SCRIPT_DIR/ca.conf -days 365 -in "$ROOT/server.csr" -out "$ROOT/server-cert.pem"

# Client Certificate
$OPENSSL_BIN req -nodes -new -newkey rsa:2048 -keyout "$ROOT/client-key.pem" -out "$ROOT/client.csr" -subj "/C=RU/ST=Moscow/L=Moscow/O=$DOM_NAME, Inc/OU=ANYKS/CN=dtls-client.$DOM_NAME"

# Sign Client Certificate
$OPENSSL_BIN ca -config $SCRIPT_DIR/ca.conf -days 365 -in "$ROOT/client.csr" -out "$ROOT/client-cert.pem"

# Remove old certificates
rm -rf "$ROOT/certificates"

# Create certificates dir
mkdir "$ROOT/certificates"

# Copy certificates
cp "$ROOT/client-cert.pem" "$ROOT/certificates/"
cp "$ROOT/client-key.pem" "$ROOT/certificates/"
cp "$ROOT/server-cert.pem" "$ROOT/certificates/"
cp "$ROOT/server-key.pem" "$ROOT/certificates/"

# Remove old file
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
rm "$ROOT/client-cert.pem"
rm "$ROOT/client-key.pem"
rm "$ROOT/server-cert.pem"
rm "$ROOT/server-key.pem"
