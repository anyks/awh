#!/bin/sh
# Отдельный стенд проверок криптографии (crypto + hash), без TLS и без libawh
set -e
cd "$(dirname "$0")"
CXX=${CXX:-g++}
INC=""; LIB=""; EXTRA=""; OBJCXX=""
case "$(uname -s)" in
  Darwin)  CAPTURE=src/alloc/capture/mach.cpp
           INC="-I/opt/homebrew/include"; LIB="-L/opt/homebrew/lib"
           # Разбор файлов-псевдонимов у macOS написан на Objective-C++ (см. CMakeLists.txt)
           EXTRA="-framework Foundation"; OBJCXX="-x objective-c++ -fobjc-arc" ;;
  OpenBSD) CAPTURE="src/alloc/capture/elf.cpp src/alloc/capture/obsd.cpp"
           INC="-I/usr/local/include"; LIB="-L/usr/local/lib" ;;
  FreeBSD|NetBSD|DragonFly) CAPTURE=src/alloc/capture/elf.cpp
           INC="-I/usr/local/include"; LIB="-L/usr/local/lib" ;;
  SunOS)   CAPTURE=src/alloc/capture/elf.cpp
           INC="-I/usr/openssl/3/include -I/usr/local/include"
           LIB="-L/usr/openssl/3/lib/amd64 -R/usr/openssl/3/lib/amd64 -L/usr/local/lib -R/usr/local/lib"
           EXTRA="-lsocket -lnsl" ;;
  *)       CAPTURE=src/alloc/capture/elf.cpp ;;
esac
[ -d /usr/pkg/include ] && { INC="$INC -I/usr/pkg/include"; LIB="$LIB -L/usr/pkg/lib -Wl,-R/usr/pkg/lib"; }
$CXX -std=gnu++17 -O2 -g -Wno-narrowing -Wno-deprecated-declarations \
  -I include $INC \
  tests/main.cpp tests/cryptography/crypto/*.cpp tests/cryptography/hash/*.cpp \
  src/cryptography/crypto.cpp src/cryptography/hash.cpp \
  src/num/bignum.cpp src/num/lexical/table.cpp src/net/nwt.cpp src/net/net.cpp \
  src/encoding/charset/*.cpp src/encoding/unicode/*.cpp $(if [ -n "$OBJCXX" ]; then echo "$OBJCXX"; fi) src/sys/*.cpp -x c++ src/alloc/*.cpp $CAPTURE \
  -o ctests $LIB -lgtest -lgmock -lcrypto -lz -lpthread $EXTRA
echo "BUILD-OK"
