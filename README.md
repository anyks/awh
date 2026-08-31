[![ANYKS - Web Hub](https://raw.githubusercontent.com/anyks/awh/main/img/banner.jpg)](https://anyks.com)

# ANYKS - Web Hub (AWH) C++

[![LicenseRef-AWH-1.0](https://img.shields.io/badge/license-AWH%201.0-blue.svg)](LICENSE)

## Requirements

| Name | Description |
|------|-------------|
| [LZ4](https://lz4.org) | A lossless data compression algorithm whose primary goal is to ensure the highest possible speed for both compression and decompression. It belongs to the LZ77 (Lempel-Ziv) family of methods and operates on byte streams. It was developed by French programmer Yann Collet in 2011. |
| [Zlib](http://www.zlib.net) | A free, cross-platform library for data compression and decompression. It implements the DEFLATE algorithm, which combines two methods: run-length encoding (LZ77) and Huffman coding. The library is widely used in various software products, operating systems, and embedded systems. |
| [BZip2](http://www.bzip.org) | A free, open-source command-line data compression utility. It implements an algorithm that combines the Burrows-Wheeler block-sorting transform with Huffman coding. |
| [Brotli](https://brotli.org) | An open-source lossless data compression algorithm developed by Google. It is designed specifically to speed up web page loading by reducing the size of text resources such as HTML, CSS, JavaScript, JSON, and others. |
| [Snappy](https://github.com/google/snappy) | A library for fast data compression and decompression developed by Google. Its key feature is its focus on high speed (compression and decompression) rather than achieving the highest possible compression ratio. |
| [Lizard](https://github.com/inikep/lizard/tree/lizard) | Lizard library is based on frequently used LZ4 library by Yann Collet but the Lizard compression format is not compatible with LZ4. Lizard library is provided as open-source software using BSD 2-Clause license. The high compression/decompression speed is achieved without any SSE and AVX extensions. |
| [Density](https://github.com/k0dai/density) | Superfast compression library. DENSITY is a free C99, open-source, BSD licensed compression library. It is focused on high-speed compression, at the best ratio possible. All three of DENSITY’s algorithms are currently at the pareto frontier of compression speed vs ratio (cf. here for an independent benchmark). |
| [ZStandart](https://github.com/facebook/zstd) | A lossless data compression algorithm developed since 2015 by Yann Collet with support from Facebook. The goal is to achieve compression ratios comparable to or superior to the classic deflate algorithm (implemented in Zip, gzip, and others), while achieving higher compression and decompression speeds. |
| [Lempel–Ziv–Markov](https://github.com/hunter-packages/lzma) | A lossless data compression algorithm developed by Igor Pavlov between 1996 and 1998 as part of the 7-Zip archiver project. |
| [BoringSSL](https://boringssl.googlesource.com/boringssl) | An open-source cryptographic library, a fork of the OpenSSL project maintained by Google. It was created specifically for Google's internal needs: to ensure strong security, simplify the codebase, and speed up development by removing deprecated and unsupported features. |

## Project goals and features

### Supported proxy
| PROXY       | RFC  |
|-------------|------|
| **HTTP(S)** | 7230 |
| **Socks5**  | 1928 |

### Supported compressors
| Compressor  | Block  | Stream |
|-------------|--------|--------|
| **LZ4**     |  ✅    |  ✅    |
| **LZMA**    |  ✅    |  ✅    |
| **ZSTD**    |  ✅    |  ✅    |
| **GZIP**    |  ✅    |  ✅    |
| **ZLIB**    |  ✅    |  ✅    |
| **BZIP2**   |  ✅    |  ✅    |
| **BROTLI**  |  ✅    |  ✅    |
| **LIZARD**  |  ✅    |  ✅    |
| **SNAPPY**  |  ✅    |  ❌    |
| **DEFLATE** |  ✅    |  ✅    |
| **DENSITY** |  ✅    |  ❌    |

### Supported protocols
| PROTOCOL                   | RFC  |
|----------------------------|------|
| **HTTP/1.1**               | 9112 |
| **HTTP/2**                 | 9113 |
| **HTTP/3**                 | 9114 |
| **QUIC**                   | 9000 |
| **Websocket**              | 6455 |
| **WebSockets with HTTP/2** | 8441 |
| **WebSockets with HTTP/3** | 9220 |

### Supported HTTP authentications
| Name       | Description |
|------------|-------------|
| **HMAC**   | A cryptographic mechanism for verifying the integrity and authenticity of data that uses a combination of a cryptographic hash function and a secret key. It verifies that a message has not been altered during transmission or storage, and that it was sent by a trusted source. |
| **BASIC**  | A simple method for verifying user authentication when accessing web resources using the HTTP protocol. When using this method, the client (e.g., a browser) transmits the username and password to the server in the header of each HTTP request. |
| **DIGEST** | An authentication method used in the HTTP protocol to verify user identity. Its key advantage over the simpler Basic Authentication is that the password is not transmitted over the network in cleartext, but is replaced with a cryptographic hash (digest). |
| **BEARER** | A common method of securing APIs involves the client passing a special token to the server, which then verifies its validity to decide whether to grant access to the resource. The name reflects the concept: access is granted to the person who "bears" the token. |

### Supported codecs
| Name       | Description                     |
|------------|---------------------------------|
| **INI**    | Initialization Container        |
| **CEF**    | Common Event Format             |
| **CSV**    | Comma-Separated Values          |
| **ABC**    | ANYKS Binary Container          |
| **XML**    | eXtensible Markup Language      |
| **JSON**   | JavaScript Object Notation      |
| **YAML**   | YAML Ain't Markup Language      |
| **TOML**   | Tom’s Obvious, Minimal Language |
| **Syslog** | System Log Container            |

### Supported operating systems
| OS                | Engine      | CPU            |
|-------------------|-------------|----------------|
| **iOS**           | kqueue      | ARM64          |
| **macOS**         | kqueue      | x86_64 / ARM64 |
| **FreeBSD**       | kqueue      | AMD64 / ARM64  |
| **OpenBSD**       | kqueue      | AMD64 / ARM64  |
| **NetBSD**        | kqueue      | AMD64 / ARM64  |
| **DragonFly BSD** | kqueue      | x86_64         |
| **Android**       | epoll       | ARM64          |
| **Linux**         | epoll       | AMD64 / ARM64  |
| **Linux**         | io_uring    | AMD64 / ARM64  |
| **Solaris 11.4**  | event ports | x86_64         |
| **OpenIndiana**   | event ports | x86_64         |
| **Windows**       | IOCP        | AMD64 / ARM64  |

## Build tools

The build needs CMake, Git, a C++17 compiler and **GNU Make** — the build scripts call
`gmake` on the BSD systems, where the system `make` is a different program. Unit tests
additionally need **GoogleTest**; without it CMake stops with an explicit error instead of
silently producing no tests.

### NetBSD

```bash
$ sudo pkgin install git cmake gmake googletest
```

> On NetBSD the packages live under `/usr/pkg`, which is not among the paths CMake searches
> by default — the build adds it on its own, no configuration needed.

> On aarch64 the vendored BoringSSL has no runtime CPU feature detection for NetBSD, so the
> build turns it off (`OPENSSL_STATIC_ARMCAP`) and takes the features from the compiler
> instead: NEON is always present on ARMv8, the crypto extensions follow `-march`.

### FreeBSD

```bash
$ sudo pkg install git cmake gmake googletest
```

### OpenBSD

```bash
$ doas pkg_add git cmake gmake googletest
```

> On OpenBSD the C runtime calls its allocator directly, by an internal name, so symbol
> substitution never reaches those calls. Defining the internal names hands them over to
> us as well, but only under static linking — in a shared `libc` they are already bound
> inside it. Build with `-DCMAKE_BUILD_ALLOCATOR_OPENBSD_FULL=YES` and link your program
> with `-static`. Details are in `include/alloc/README.md`.

### Linux

```bash
$ sudo dnf install git cmake gcc-c++ gtest-devel gmock-devel      # Fedora, RHEL
$ sudo apt install git cmake g++ libgtest-dev libgmock-dev        # Debian, Ubuntu
```

> Here the system `make` already is GNU Make, so `gmake` is not needed.

> The command above was verified on Fedora 42 aarch64; the Debian one lists the packages
> that carry the same libraries and has not been run on a stand.

## To build and launch the project

### To clone the project

```bash
$ git clone --recursive https://gitflic.ru/project/anyks/awh.git
```

### Activate SCTP only (FreeBSD / Linux)

#### FreeBSD

```bash
$ sudo kldload sctp
```

#### Linux (ALT)

```bash
$ sudo apt-get install liblksctp-devel
$ sudo modprobe sctp
$ sudo sysctl -w net.sctp.auth_enable=1
```

#### Linux (Ubuntu)

```bash
$ sudo apt install libsctp-dev
$ sudo modprobe sctp
$ sudo sysctl -w net.sctp.auth_enable=1
```

#### Linux (Fedora)

```bash
$ sudo yum install lksctp-tools-devel
$ sudo modprobe sctp
$ sudo sysctl -w net.sctp.auth_enable=1
```

#### Linux (openSUSE)

```bash
$ sudo zypper install lksctp-tools-devel
$ sudo modprobe sctp
$ sudo sysctl -w net.sctp.auth_enable=1
```

### Prepare a tunnel device (Sun Solaris / illumos)

These systems have no `/dev/tun` or `/dev/tap` device: a tunnel is an ordinary
data link, and creating one is an administrative step performed once, outside the
application. Prepare the device beforehand and pass its name to the interface.

```bash
$ sudo dladm create-etherstub awhstub0
$ sudo dladm create-vnic -l awhstub0 awhtun0
$ sudo dladm create-vnic -l awhstub0 awhtun1
```

Two links are needed, not one. The framework opens the first of them and reads the
raw frames off it, so no IP address may live there — the address belongs on the
second link, and the kernel routes through that one. The etherstub carries the
frames between the two.

On illumos a VNIC also filters by its own address by default, and the frames of its
neighbour never reach the reader; the filtering has to be switched off on the link
the framework opens.

```bash
$ sudo dladm set-linkprop -p promisc-filtered=off awhtun0
```

The framework then opens the prepared device by name and exchanges frames over a
plain descriptor. Removing the device is administrative as well, because the system
ships no declarations for it.

```bash
$ sudo dladm delete-vnic awhtun0
$ sudo dladm delete-vnic awhtun1
$ sudo dladm delete-etherstub awhstub0
```

### Prepare a tunnel driver (MS Windows)

Neither tunnel driver ships with the framework: both are installed by the user.
Two are supported, and they are different devices, not two names for one.

* **tap-windows6** — the driver of OpenVPN. It carries **link-layer frames** and is
  installed together with OpenVPN, or on its own from the OpenVPN distribution.
* **Wintun** — a ring in shared memory. It carries **network-layer packets** and
  creates its own device when the application asks for one. It ships as a single
  `wintun.dll`, downloaded from the WireGuard site.

By default the driver is chosen automatically: Wintun when `wintun.dll` can be
loaded, tap-windows6 otherwise. The choice can be made explicit — the tunnel samples
take `--driver auto|wintun|tap`, and the engine takes `setDriver()`.

**`wintun.dll` must sit next to the executable that loads it**, or in `System32`,
and nowhere else: the framework loads it with `LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
LOAD_LIBRARY_SEARCH_SYSTEM32`, so neither the current directory nor `PATH` is
searched. That holds for unit tests as much as for applications: a test exercising
Wintun needs the library beside the **test** binary, not beside the samples.

Creating a tunnel device requires administrator rights on this system.

Incoming ICMP is dropped by the Windows firewall by default, which makes a tunnel
look one-way — the packets arrive and are written to the device, yet nothing answers.
Allow it once:

```
netsh advfirewall firewall add rule name="AWH ICMP" protocol=icmpv4:8,any dir=in action=allow
```

```bash
$ cd ./sh/certificates
$ ./generate.sh example.com
```

### Bring up a tunnel pair

Two properties of a working pair look exactly like a broken one. Both were measured on
a Windows (Wintun) to FreeBSD pair, 5140 octets each way.

**The MTU of the tunnel must be smaller than the MTU of the network underneath it.**
Left equal — 1500 over 1500 — a packet above the limit is fragmented twice: once by the
host onto the device, and again by the network under the UDP wrapper. The first fragment
is lost and only the tail arrives: a 2000-octet ping reached the far side as 548 octets,
with no head at all. Anything up to 1472 (+28 = 1500) passed, anything above it did not.
Give the tunnel 1400 on both ends and 5140 octets pass in both directions.

**The mediator cannot speak first.** A ping *from the server* to the client is answered
by nothing, and the log says what is wrong:

```
Прочитано из посредника: ID=0
Ошибка отправки в сервер: ID=0
```

The zero in place of an identifier means the client has no session yet: the mediator
matches a packet to a session by its source, so until the client has spoken once there
is nobody to send to. Let a single exchange go *from the client* and the reverse
direction works immediately. This is by design, not a fault — a server behind a mediator
answers, it does not initiate.

### Build third party

```bash
$ ./sh/build_third_party.sh
```

### Build documentation

```bash
$ mkdir ./build
$ cd ./build

$ cmake ..

# Build documentation for Russian and English language
$ cmake --build . --target doc

# Build documentation for Russian language
$ cmake --build . --target doc-ru

# Build documentation for English language
$ cmake --build . --target doc-en

# Or

$ make doc
```
> Result in ./docs/html/index.html

### Build macOS framework

```bash
$ cmake -B build -DCMAKE_BUILD_TYPE=Release

$ cd build && cmake --build . --target framework

# Or

$ cd build && make framework
```
> Result in ./build/dist/awh.framework

### Build on macOS, Linux, FreeBSD or Solaris without Unit-tests

```bash
$ mkdir ./build
$ cd ./build

$ cmake \
 -DCMAKE_BUILD_TYPE=Release \
 -DCMAKE_SHARED_BUILD_LIB=YES \
 ..

$ make
```

### Build on macOS, Linux, FreeBSD or Solaris with Unit-tests

```bash
$ mkdir ./build
$ cd ./build

$ cmake \
 -DCMAKE_BUILD_TESTS=YES \
 -DCMAKE_BUILD_TYPE=Release \
 ..

$ make
```

### Build unit tests with sanitizers

```bash
$ mkdir ./build-sanitize
$ cd ./build-sanitize

$ cmake \
 -DCMAKE_BUILD_TESTS=YES \
 -DCMAKE_BUILD_SANITIZE=YES \
 -DCMAKE_BUILD_COVERAGE=NO \
 -DCMAKE_BUILD_NET_TESTS=NO \
 -DCMAKE_BUILD_TYPE=Debug \
 ..

$ make
```

Sanitizers are enabled for the test library and every unit test binary at once.
Use a separate build directory: object files built with and without sanitizers
are not interchangeable. On macOS the built-in allocator drops its symbol
interposition automatically under a sanitizer — AddressSanitizer replaces the
allocator itself, and a second replacement makes it blind. The set of sanitizers is chosen by `CMAKE_BUILD_SANITIZE_LIST`
(`address,undefined` by default; `thread` is incompatible with `address` and is
enabled separately).

### Unit tests can be disabled in groups

```bash
# Disabling unit test building for SYS modules
$ cmake \
 -DCMAKE_BUILD_TESTS=YES \
 -DCMAKE_BUILD_SYS_TESTS=NO \
 ..

# Disabling unit test building for NET modules
$ cmake \
 -DCMAKE_BUILD_TESTS=YES \
 -DCMAKE_BUILD_NET_TESTS=NO \
 ..
```

### You can disable the compilation of specific unit tests.

#### Disabling specific unit tests for the SYS group

```bash
# We indicate that there is no need to compile tests for working with the OS module.
$ cmake \
 -DCMAKE_BUILD_TESTS=YES \
 -DCMAKE_BUILD_SYS_OS_TEST=NO \
 ..

# We indicate that there is no need to compile tests for working with the NET module.
$ cmake \
 -DCMAKE_BUILD_TESTS=YES \
 -DCMAKE_BUILD_SYS_FMK_TEST=NO \
 ..

# We indicate that there is no need to compile tests for working with the LOG module.
$ cmake \
 -DCMAKE_BUILD_TESTS=YES \
 -DCMAKE_BUILD_SYS_LOG_TEST=NO \
 ..

# We indicate that there is no need to compile tests for working with the REG module.
$ cmake \
 -DCMAKE_BUILD_TESTS=YES \
 -DCMAKE_BUILD_SYS_REG_TEST=NO \
 ..

# We indicate that there is no need to compile tests for working with the QUEUE module.
$ cmake \
 -DCMAKE_BUILD_TESTS=YES \
 -DCMAKE_BUILD_SYS_QUEUE_TEST=NO \
 ..

# We indicate that there is no need to compile tests for working with the BUFFER module.
$ cmake \
 -DCMAKE_BUILD_TESTS=YES \
 -DCMAKE_BUILD_SYS_BUFFER_TEST=NO \
 ..

# We indicate that there is no need to compile tests for working with the CHRONO module.
$ cmake \
 -DCMAKE_BUILD_TESTS=YES \
 -DCMAKE_BUILD_SYS_CHRONO_TEST=NO \
 ..

# We indicate that there is no need to compile tests for working with the THREADPOOL module.
$ cmake \
 -DCMAKE_BUILD_TESTS=YES \
 -DCMAKE_BUILD_SYS_THREADPOOL_TEST=NO \
 ..
```

#### Disabling specific unit tests for the NET group

```bash
# We indicate that there is no need to compile tests for working with the ADDR module.
$ cmake \
 -DCMAKE_BUILD_TESTS=YES \
 -DCMAKE_BUILD_NET_ADDR_TEST=NO \
 ..

# We indicate that there is no need to compile tests for working with the NWT module.
$ cmake \
 -DCMAKE_BUILD_TESTS=YES \
 -DCMAKE_BUILD_NET_NWT_TEST=NO \
 ..

# We indicate that there is no need to compile tests for working with the FDS module.
$ cmake \
 -DCMAKE_BUILD_TESTS=YES \
 -DCMAKE_BUILD_NET_FDS_TEST=NO \
 ..

# We indicate that there is no need to compile tests for working with the ETH module.
$ cmake \
 -DCMAKE_BUILD_TESTS=YES \
 -DCMAKE_BUILD_NET_ETH_TEST=NO \
 ..

# We indicate that there is no need to compile tests for working with the IO module.
$ cmake \
 -DCMAKE_BUILD_TESTS=YES \
 -DCMAKE_BUILD_NET_IO_TEST=NO \
 ..
```

#### Running unittest, benchmark and coverage statistic on Linux

> **Unit tests must be started from the `build` directory.** Several network
> tests open certificates, keys and payload samples by paths relative to the
> current directory: there is no way to pass those paths into the tests so that
> they resolve from any working directory. Started from anywhere else, such a
> test hangs waiting for a file it will never open — this is the launch
> directory, not a defect in the test.

> The `${DEPEND_LIBRARY}` dependency is linked **only in the debug build**. The
> release build repacks `libawh.a` so that it statically contains the whole
> content of that library, and nothing but `libawh.a` is needed to link against
> a release build. The repacker is single-threaded and slow by design: it has to
> unpack and repack a large number of object files without letting them
> overwrite each other.

```bash
# Assume we in build folder
$ lcov --capture --initial --directory . --output-file awh_base.info

# Running unit tests
$ unit-tests/sys
$ unit-tests/net

# Running benchmark
#$ benchmark/awh_BENCHMARK

# Collect coverage statistic, should be executed after UNITTEST
$ gcovr -r ../ -e ../unit-tests/ -e ../contrib/ -e ../sample/ -e ../submodules/ -e ../third_party/ -e ../benchmark/ -e ../experience/ -e /usr/include/

# Create html report with lcov 
$ lcov --capture --directory . --output-file awh_test.info
$ lcov --add-tracefile awh_base.info --add-tracefile awh_test.info -o awh.info
$ lcov --remove awh.info '*/unit-tests/*' '/usr/include/*' '*/contrib/*' '*/benchmark/*' '*/sample/*' '*/submodules/*' '*/third_party/*' '*/experience/*' -o awh_filtered.info
$ genhtml -o report awh_filtered.info
```

### Build on Windows [MSYS2 MinGW]

#### Development environment configuration
- [GIT](https://git-scm.com)
- [Perl](https://strawberryperl.com)
- [Python](https://www.python.org/downloads/windows)
- [MSYS2](https://www.msys2.org)
- [CMAKE](https://cmake.org/download)

#### Assembly is done in MSYS2 - MINGW64 terminal (x86_64)

```bash
$ pacman -Syuu
$ pacman -S make
$ pacman -S cmake
$ pacman -S curl
$ pacman -S wget
$ pacman -S mc
$ pacman -S gdb
$ pacman -S bash
$ pacman -S unzip
$ pacman -S clang
$ pacman -S git
$ pacman -S autoconf
$ pacman -S --needed base-devel mingw-w64-x86_64-toolchain
$ pacman -S mingw-w64-x86_64-dlfcn
```

#### Project build

```bash
$ mkdir ./build
$ cd ./build

$ cmake \
 -G "MSYS Makefiles" \
 -DCMAKE_BUILD_TYPE=Release \
 -DCMAKE_SYSTEM_NAME=Windows \
 -DCMAKE_SHARED_BUILD_LIB=YES \
 ..

$ cmake --build .
```

#### Assembly is done in MSYS2 - CLANGARM64 terminal (ARM64)

```bash
$ pacman -Syu
$ pacman -S make
$ pacman -S cmake
$ pacman -S curl
$ pacman -S wget
$ pacman -S mc
$ pacman -S gdbm
$ pacman -S bash
$ pacman -S unzip
$ pacman -S clang
$ pacman -S git
$ pacman -S autoconf
```

#### Project build

```bash
$ mkdir ./build
$ cd ./build

$ cmake \
 -G "MSYS Makefiles" \
 -DCMAKE_BUILD_TYPE=Release \
 -DCMAKE_SYSTEM_NAME=Windows \
 -DCMAKE_SHARED_BUILD_LIB=YES \
 ..

$ cmake --build .
```

### Make installation packages

#### Build PKG package for macOS

```bash
# Build installation package
$ ./sh/dist/macos_make_installer.sh
```

#### Build EXE installator for MS Windows

```bash
# Build installation package
$ ./sh/dist/windows_make_installer.sh
```

#### Build P5P package for Solaris

```bash
# Build installation package
$ ./sh/dist/solaris_make_installer.sh

# Install AWH library
$ sudo pkg install -g awh_X.X.X-1_i86pc.p5p awh

# Registering installed components
$ sudo postinstall-awh
```

#### Build TAR.GZ package for FreeBSD

```bash
# Build installation package
$ ./sh/dist/freebsd_make_tar.sh

# Install AWH library
$ sudo tar -xzvf awh_X.X.X_FreeBSD_amd64.tar.gz -C /
```

#### Build DEB package for Linux (Astra, Ubuntu, Debian, Deepin)

```bash
# Build installation package
$ ./sh/dist/linux_make_deb.sh

# Install AWH library
sudo dpkg -i awh_X.X.X-X~X_amd64.deb
```

#### Build RPM package for Linux (ALT, RedOS, Fedora, openSUSE, CentOS, RedHat)

```bash
# Build installation package
$ ./sh/dist/linux_make_rpm.sh

# Install AWH library
sudo rpm -i glb-X.X.X-X.X_amd64.rpm
```

### Example include AWH in your project

#### Example c++ project

```c++
// main.cpp
// cmake -DCMAKE_SHARED_LIB_AWH=YES ..
#include <iostream>
#include <awh/sys/lib.hpp>

int main(){
    std::cout << "AWH Version: " << AWH_VERSION << std::endl;
    return 0;
}
```

#### Example cmake project

```cmake
cmake_minimum_required(VERSION 3.16)

project(my_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(AWH REQUIRED)

if (AWH_FOUND)
    message(STATUS "AWH found: " ${AWH_LIBRARY} " " ${AWH_LIBRARY_DLL} " " ${AWH_INCLUDE_DIR})
else (AWH_FOUND)
    message(FATAL_ERROR "AWH not found")
endif (AWH_FOUND)

add_executable(my_app main.cpp)

target_link_libraries(my_app PRIVATE ${AWH_LIBRARY})
```

#### Example project build for macOS, Linux, FreeBSD or Solaris

```bash
$ mkdir ./build
$ cd ./build

$ cmake \
 -DCMAKE_BUILD_TYPE=Release \
 -DCMAKE_SHARED_LIB_AWH=YES \
 ..

$ cmake --build .
```

#### Example project build for MS Windows

```bash
$ mkdir ./build
$ cd ./build

$ cmake \
 -G "MSYS Makefiles" \
 -DCMAKE_BUILD_TYPE=Release \
 -DCMAKE_SYSTEM_NAME=Windows \
 -DCMAKE_SHARED_LIB_AWH=YES \
 ..

$ cmake --build .
```

---

### Example client
```c++

```

---

### Example server
```c++

```

---

## License

AWH is distributed under the **AWH License 1.0** — see [LICENSE](LICENSE) for the full text.

### In short

- **Free for any use**, including commercial, proprietary and closed-source products, on any operating system and any number of machines. No fee, no registration, no separate permission.
- **Link it any way you like** — statically (`libawh.a`, `awh.lib`) or dynamically (`libawh.so`, `libawh.dylib`, `libawh.dll`, `awh.dll`).
- **Your code stays yours.** Linking AWH does not oblige you to publish, disclose or open source anything of your own.
- **You must credit the author** — see below. This is the one thing asked in return.
- **You may not take AWH apart.** Copying its individual files, classes or algorithms into another project, or publishing any part of it as a separate library, is not permitted. AWH is used as a whole.

Note that this is a source-available license, not an OSI-approved open source one: the requirement to use the library as a whole is incompatible with the Open Source Definition. Everything else about it is permissive.

### Required attribution

A product that uses AWH must reproduce the following notice somewhere its users can reasonably find it — the documentation, an "About" or "Credits" screen, a third-party notices page, a `--version` output, or a README:

```
This product uses AWH (ANYKS Web Hub) by Yuriy Lobarev (ANYKS).
https://github.com/anyks/awh
```

The repository address must be reproduced verbatim, and presented as a working link wherever links are supported.

### Third-party components

AWH builds on a number of external libraries which are **not** covered by the AWH License and keep their own terms — you are free to use any of them separately. They are listed, with their licenses, in [THIRD-PARTY-LICENSES.md](THIRD-PARTY-LICENSES.md).

### Commercial and extended terms

Need rights beyond this license — for example the right to use individual components separately, or a waiver of the attribution requirement? Write to <forman@anyks.com>.
