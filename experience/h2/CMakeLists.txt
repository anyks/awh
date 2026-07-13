# ─────────────────────────────────────────────────────────────────────────────
# experience/h2 — собственная реализация HTTP/2 (RFC 9113) + HPACK (RFC 7541).
#
# Сборка состоит из двух частей:
#   1) Ядро h2 (всегда): статическая библиотека `h2`, юнит-тесты `h2-test`, демо `h2-demo`.
#      Ни от чего не зависит, чистый C++17.
#   2) Транспортная интеграция (опция H2_WITH_AWH=ON): примеры HTTP/2-сервера и
#      клиента поверх TCP-транспорта библиотеки AWH (`h2-awh-server`, `h2-awh-client`).
#      Требует предварительно собранных зависимостей AWH (third_party/lib/libdependence.a,
#      см. ./build_third_party.sh в корне AWH). Сама libawh здесь собирается из исходников.
#
# Пример:
#   cmake -S . -B build                       # только ядро + тесты
#   cmake -S . -B build -DH2_WITH_AWH=ON       # + транспорт поверх AWH (macOS/BSD)
#   cmake --build build -j
#   ctest --test-dir build --output-on-failure
# ─────────────────────────────────────────────────────────────────────────────

cmake_minimum_required(VERSION 3.15)
project(awh_h2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# По умолчанию релизная сборка (тяжёлый HPACK/framing любит оптимизацию).
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

# ───────────────────────── 1. Ядро HTTP/2 ─────────────────────────

add_library(h2 STATIC
	h2.cpp
	frame.cpp
	hpack.cpp
	session.cpp
)
target_include_directories(h2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(h2 PRIVATE -Wall -Wextra)

# Юнит-тесты ядра (round-trip HPACK/framing, стейт-машина, flow control, DoS,
# HTTP-семантика, server push).
add_executable(h2-test test.cpp)
target_link_libraries(h2-test PRIVATE h2)

# Минимальное демо разбора входящих фреймов (без сети).
add_executable(h2-demo demo.cpp)
target_link_libraries(h2-demo PRIVATE h2)

enable_testing()
add_test(NAME h2-unit COMMAND h2-test)

# ───────────────────────── 2. Транспорт поверх AWH ─────────────────────────

option(H2_WITH_AWH "Build HTTP/2 server/client over AWH TCP transport" OFF)

if(H2_WITH_AWH)
	# GCC 15 ужесточил SFINAE-проверку ostream::operator<< в libstdc++ — часть
	# логирующих шаблонов AWH под ним не собирается. Рабочие компиляторы: AppleClang
	# и GCC 13/14 (как у самого проекта AWH).
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 15)
		message(WARNING
			"GCC ${CMAKE_CXX_COMPILER_VERSION}: в libstdc++-15 ужесточён ostream::operator<<, "
			"часть логирующих шаблонов AWH может не собраться. Используйте AppleClang или GCC 13/14.")
	endif()

	# Корень репозитория AWH (на два уровня выше: experience/h2 -> experience -> awh).
	get_filename_component(AWH_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
	set(AWH_TP "${AWH_ROOT}/third_party")

	# Предсобранные сторонние зависимости (единый архив libdependence.a).
	find_library(AWH_DEPEND_LIB NAMES dependence PATHS "${AWH_TP}/lib" NO_DEFAULT_PATH)
	if(NOT AWH_DEPEND_LIB)
		message(FATAL_ERROR
			"Не найдена ${AWH_TP}/lib/libdependence.a.\n"
			"Сначала соберите зависимости AWH: в корне репозитория выполните ./build_third_party.sh")
	endif()

	# Исходники ядра AWH (список синхронизирован с корневым CMakeLists.txt).
	set(AWH_SRC "${AWH_ROOT}/src")
	set(AWH_SOURCES
		"${AWH_SRC}/sys/os.cpp"          "${AWH_SRC}/sys/fs.cpp"        "${AWH_SRC}/sys/fmk.cpp"
		"${AWH_SRC}/sys/log.cpp"         "${AWH_SRC}/sys/reg.cpp"       "${AWH_SRC}/sys/queue.cpp"
		"${AWH_SRC}/sys/buffer.cpp"      "${AWH_SRC}/sys/procre.cpp"    "${AWH_SRC}/sys/chrono.cpp"
		"${AWH_SRC}/sys/binbox.cpp"      "${AWH_SRC}/sys/signals.cpp"   "${AWH_SRC}/sys/version.cpp"
		"${AWH_SRC}/sys/crypto.cpp"      "${AWH_SRC}/compressor/block.cpp"
		"${AWH_SRC}/compressor/stream.cpp"
		"${AWH_SRC}/net/fds.cpp"         "${AWH_SRC}/net/uri.cpp"       "${AWH_SRC}/net/nwt.cpp"
		"${AWH_SRC}/net/addr.cpp"        "${AWH_SRC}/net/queue.cpp"
		"${AWH_SRC}/net/tls/coder.cpp"   "${AWH_SRC}/net/tls/fingerprint.cpp"
		"${AWH_SRC}/units/fs.cpp"        "${AWH_SRC}/units/dns.cpp"     "${AWH_SRC}/units/ntp.cpp"
		"${AWH_SRC}/units/icmp.cpp"      "${AWH_SRC}/units/unit.cpp"    "${AWH_SRC}/units/timer.cpp"
		"${AWH_SRC}/units/client.cpp"    "${AWH_SRC}/units/server.cpp"  "${AWH_SRC}/units/tunnel.cpp"
		"${AWH_SRC}/units/cluster.cpp"   "${AWH_SRC}/units/mediator.cpp" "${AWH_SRC}/units/notifier.cpp"
		"${AWH_SRC}/client/client.cpp"   "${AWH_SRC}/client/socks5.cpp"
		"${AWH_SRC}/server/server.cpp"   "${AWH_SRC}/server/socks5.cpp"
		"${AWH_SRC}/proto/socks5/socks5.cpp" "${AWH_SRC}/proto/socks5/client.cpp" "${AWH_SRC}/proto/socks5/server.cpp"
		"${AWH_ROOT}/contrib/src/cityhash/city.cc"
	)

	# Платформенный backend ввода-вывода.
	if(APPLE OR CMAKE_SYSTEM_NAME MATCHES "BSD")
		list(APPEND AWH_SOURCES
			"${AWH_SRC}/net/backend/bsd/eth.cpp"     "${AWH_SRC}/net/backend/bsd/addr.cpp"
			"${AWH_SRC}/net/backend/bsd/iface.cpp"   "${AWH_SRC}/net/backend/bsd/kqueue.cpp"
			"${AWH_SRC}/net/backend/bsd/socket.cpp"  "${AWH_SRC}/net/backend/bsd/gateway.cpp"
			"${AWH_SRC}/net/backend/bsd/portmap.cpp"
		)
		if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
			list(APPEND AWH_SOURCES "${AWH_SRC}/net/backend/bsd/sctp.cpp")
		endif()
	else()
		message(FATAL_ERROR "H2_WITH_AWH в этом эксперименте поддерживается только на macOS/BSD")
	endif()

	# Каталоги заголовков AWH и предсобранных зависимостей.
	set(AWH_INCLUDE_DIRS
		"${AWH_ROOT}/include"
		"${AWH_ROOT}/contrib/include"
		"${AWH_TP}/include"
		"${AWH_TP}/include/zlib"   "${AWH_TP}/include/brotli" "${AWH_TP}/include/lz4"
		"${AWH_TP}/include/bz2"    "${AWH_TP}/include/zstd"   "${AWH_TP}/include/lzma"
		"${AWH_TP}/include/pcre2"  "${AWH_TP}/include/nghttp2" "${AWH_TP}/include/snappy"
		"${AWH_TP}/include/density" "${AWH_TP}/include/lizard"
	)

	add_library(awh STATIC ${AWH_SOURCES})
	# DEBUG_MODE и AWH_STATICLIB — PUBLIC, чтобы распространились и на демо-цели,
	# которые включают те же заголовки AWH. DEBUG_MODE выбирает рабочие ветки кода
	# (релизные ветки в AWH содержат компиляционные дефекты, проект штатно собирается
	# в Debug — см. корневой CMakeLists.txt: CMAKE_CXX_FLAGS_DEBUG="-O0 -DDEBUG_MODE").
	target_compile_definitions(awh PUBLIC AWH_STATICLIB DEBUG_MODE)
	target_include_directories(awh PUBLIC ${AWH_INCLUDE_DIRS})
	# Подавление диагностик, расходящихся между компиляторами (PUBLIC: те же заголовки
	# включают демо-цели). Базовый набор синхронизирован с корневым CMakeLists.txt.
	set(AWH_WARN_FLAGS -Wno-narrowing -Wno-deprecated-declarations -Wno-deprecated)
	if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		# Clang: ""IDENT (ud-литерал) и передача std::string через C-варарги в логере.
		list(APPEND AWH_WARN_FLAGS -Wno-reserved-user-defined-literal -Wno-non-pod-varargs)
	elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		list(APPEND AWH_WARN_FLAGS -fno-permissive -Wno-pedantic -Wno-exceptions
			-Wno-unused-result -Wno-cpp -Wno-literal-suffix)
	endif()
	target_compile_options(awh PUBLIC ${AWH_WARN_FLAGS})
	# На macOS часть исходников AWH использует Objective-C++ (Security/CoreFoundation).
	if(APPLE)
		target_compile_options(awh PRIVATE -x objective-c++ -fobjc-arc)
	endif()

	# Системные фреймворки/библиотеки, требуемые AWH.
	set(AWH_PLATFORM_LIBS "")
	if(APPLE)
		list(APPEND AWH_PLATFORM_LIBS "-framework Security" "-framework CoreFoundation" "-framework Foundation")
	endif()

	# Примеры HTTP/2 поверх транспорта AWH.
	foreach(app server client)
		add_executable(h2-awh-${app} awh/${app}.cpp)
		target_link_libraries(h2-awh-${app} PRIVATE h2 awh ${AWH_DEPEND_LIB} ${AWH_PLATFORM_LIBS})
	endforeach()
endif()
