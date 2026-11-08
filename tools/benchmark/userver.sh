#!/usr/bin/env bash

# Скрипт сборки эталонных стендов сравнения кодеков с реализацией userver
#
# Реализация userver несёт два кодека - JSON и YAML, и оба построены поверх чужих
# двигателей: JSON поверх RapidJSON, YAML поверх yaml-cpp. Стенды эти потому не
# входят в общие сценарии «json/build.sh» и «yaml/build.sh»: им нужна собранная
# библиотека userver, а она тянет за собой fmt, cctz, re2 и cryptopp, каких на
# образах кластера нет. Сличение ведётся на той машине, где всё это собрано
#
# Доводы:
#   $1 - каталог собранных стендов (умолчание /tmp/rival-userver)
#   $2 - каталог промежуточной сборки библиотеки userver (умолчание /tmp/userver-build)
#
# Стороны AWH, RapidJSON и yaml-cpp собираются своими сценариями:
#   ./tools/benchmark/json/build.sh
#   ./tools/benchmark/yaml/build.sh

# Получаем корневую дирректорию репозитория (каталог скрипта на два уровня ниже)
readonly ROOT=$(cd "$(dirname "$0")/../.." && pwd)

# Каталог стендов сравнения контейнера JSON
readonly JSON_STANDS="$ROOT/tools/benchmark/json"

# Каталог стендов сравнения дерева настроек YAML
readonly YAML_STANDS="$ROOT/tools/benchmark/yaml"

# Каталог исходных текстов сравниваемой реализации
readonly VENDOR="$ROOT/submodules/userver"

# Каталог собранных стендов
readonly OUTPUT="${1:-/tmp/rival-userver}"

# Каталог промежуточной сборки библиотеки userver
readonly WORKDIR="${2:-/tmp/userver-build}"

# Каталог префикса библиотеки re2, собранной без поддержки ICU
readonly RE2_PREFIX="$WORKDIR/re2-prefix"

# Флаги сборки стендов
#
# Уровень оптимизации совпадает с уровнем сборки библиотеки в режиме Release:
# сравнивать реализации, собранные с разной оптимизацией, бессмысленно. Стандарт
# языка задан C++20, а не C++17 как у прочих стендов: ниже C++20 реализация userver
# не собирается вовсе, и то оговорено отчётом сличения
readonly FLAGS="-O3 -DNDEBUG -std=c++20 -Wall -Wextra"

# Системные библиотеки, каких требует реализация userver
#
# @details Разбор alias-файлов в cctz зовёт CoreFoundation у macOS, а проверка
#          границ в «utils/assert.cpp» и кэш стека журнала - библиотеку трассировки
#          стека Boost. Порядок этот снят по отказам связывания 07.11.2026
case "$(uname -s)" in
	Darwin) SYSTEM_LIBS="-framework CoreFoundation -framework Foundation" ;;
	*) SYSTEM_LIBS="" ;;
esac

# Если исходные тексты сравниваемой реализации не получены
if [ ! -f "$VENDOR/version.txt" ]; then
	# Выводим сообщение об отсутствии подмодуля
	echo "Submodule \"userver\" is missing in \"$VENDOR\""
	exit 1
fi

# Если сборщик cmake не найден
if ! command -v cmake > /dev/null 2>&1; then
	# Выводим сообщение об отсутствии сборщика
	echo "Build tool \"cmake\" is missing"
	exit 1
fi

# Выполняем создание каталога собранных стендов и каталога промежуточной сборки
mkdir -p "$OUTPUT" "$WORKDIR"

# Если библиотека re2 без поддержки ICU ещё не собрана
if [ ! -f "$RE2_PREFIX/lib/libre2.a" ]; then
	# Выводим сообщение о необходимости предварительной настройки userver
	echo "Fetch \"re2\" sources by userver setup"

	# Выполняем предварительную настройку userver: она одна скачивает исходные
	# тексты re2, отдельно их не получить
	#
	# Настройка эта заведомо не доходит до конца: сборка re2 отказывает на
	# заголовках ICU, и нужен от неё лишь каталог скачанных исходных текстов
	cmake -S "$VENDOR" -B "$WORKDIR" -DCMAKE_BUILD_TYPE=Release \
		-DUSERVER_FEATURE_CORE=OFF -DUSERVER_BUILD_TESTS=OFF \
		-DUSERVER_BUILD_SAMPLES=OFF -DUSERVER_FEATURE_CHAOTIC=OFF \
		-DUSERVER_FEATURE_CHAOTIC_OPENAPI=OFF > "$WORKDIR/setup.log" 2>&1

	# Если исходные тексты re2 скачаны не были
	if [ ! -f "$WORKDIR/_deps/re2-src/CMakeLists.txt" ]; then
		# Выводим сообщение о неудаче скачивания
		echo "Fetch of \"re2\" sources failed, see \"$WORKDIR/setup.log\""
		exit 1
	fi

	# Выводим сообщение о сборке библиотеки re2 без поддержки ICU
	echo "Build \"re2\" without ICU"

	# Выполняем сборку и установку библиотеки re2 без поддержки ICU
	#
	# Реализация userver жёстко ставит «RE2_USE_ICU ON» в «cmake/SetupRe2.cmake»,
	# а розыск ICU на половине машин находит библиотеку в одном префиксе, а
	# заголовки в другом: сборка re2 тогда отказывает на «unicode/uniset.h».
	# Своя сборка re2 без ICU уводит предмет целиком, и поддержке полных свойств
	# юникода в их регулярных выражениях взяться в кодеках неоткуда
	cmake -S "$WORKDIR/_deps/re2-src" -B "$WORKDIR/re2-build" \
		-DCMAKE_BUILD_TYPE=Release -DRE2_USE_ICU=OFF \
		-DRE2_BUILD_TESTING=OFF -DBUILD_SHARED_LIBS=OFF \
		-DCMAKE_INSTALL_PREFIX="$RE2_PREFIX" > "$WORKDIR/re2-cfg.log" 2>&1 &&
	cmake --build "$WORKDIR/re2-build" -j"$(getconf _NPROCESSORS_ONLN)" \
		> "$WORKDIR/re2-build.log" 2>&1 &&
	cmake --install "$WORKDIR/re2-build" > "$WORKDIR/re2-inst.log" 2>&1

	# Если библиотека re2 не собрана
	if [ ! -f "$RE2_PREFIX/lib/libre2.a" ]; then
		# Выводим сообщение о неудаче сборки
		echo "Build of \"re2\" failed, see \"$WORKDIR/re2-build.log\""
		exit 1
	fi
fi

# Если библиотека userver ещё не собрана
if [ ! -f "$WORKDIR/universal/libuserver-universal.a" ]; then
	# Выводим сообщении о настройке библиотеки userver
	echo "Configure \"userver-universal\""

	# Выполняем настройку библиотеки userver
	#
	# Выключатель «USERVER_FEATURE_CORE» оставляет одну библиотеку universal,
	# в какой оба кодека и живут: ядро с сопрограммами кодекам не нужно, а тянет
	# оно за собой добрую половину дерева. Проверка версий пакетов выключена
	# потому, что собранный выше re2 своей версии не сообщает, и userver бракует
	# его при включённой проверке, уходя обратно на свою скачиваемую сборку
	cmake -S "$VENDOR" -B "$WORKDIR" -DCMAKE_BUILD_TYPE=Release \
		-DUSERVER_FEATURE_CORE=OFF -DUSERVER_BUILD_TESTS=OFF \
		-DUSERVER_BUILD_SAMPLES=OFF -DUSERVER_FEATURE_CHAOTIC=OFF \
		-DUSERVER_FEATURE_CHAOTIC_OPENAPI=OFF \
		-DUSERVER_CHECK_PACKAGE_VERSIONS=0 \
		-DCMAKE_PREFIX_PATH="$RE2_PREFIX" > "$WORKDIR/cfg.log" 2>&1

	# Если настройка библиотеки завершилась отказом
	if [ $? -ne 0 ]; then
		# Выводим сообщение о неудаче настройки
		echo "Configure of \"userver-universal\" failed, see \"$WORKDIR/cfg.log\""
		exit 1
	fi

	# Выводим сообщение о сборке библиотеки userver
	echo "Build \"userver-universal\""

	# Выполняем сборку библиотеки userver
	cmake --build "$WORKDIR" --target userver-universal \
		-j"$(getconf _NPROCESSORS_ONLN)" > "$WORKDIR/build.log" 2>&1

	# Если библиотека userver не собрана
	if [ ! -f "$WORKDIR/universal/libuserver-universal.a" ]; then
		# Выводим сообщение о неудаче сборки
		echo "Build of \"userver-universal\" failed, see \"$WORKDIR/build.log\""
		exit 1
	fi
fi

# Выполняем снятие определений и каталогов включений из настройки userver
#
# Пространство имён у userver задано не заголовком, а определением сборки, и
# значение его содержит вложенное пространство имён с версией: брать его нужно
# дословно из той настройки, какой библиотека была собрана
readonly FLAGS_MAKE="$WORKDIR/universal/CMakeFiles/userver-universal.dir/flags.make"

# Если файл настройки сборки библиотеки не найден
if [ ! -f "$FLAGS_MAKE" ]; then
	# Выводим сообщение об отсутствии файла настройки
	echo "File of build flags is missing: \"$FLAGS_MAKE\""
	exit 1
fi

# Выполняем чтение определений сборки библиотеки
readonly DEFS=$(sed -n 's/^CXX_DEFINES = //p' "$FLAGS_MAKE")

# Выполняем чтение каталогов включений библиотеки
readonly INCS=$(sed -n 's/^CXX_INCLUDES = //p' "$FLAGS_MAKE")

# Выполняем чтение перечня собранных зависимостей
readonly LIBS=$(find "$WORKDIR/_deps" -name 'libfmt.a' -o -name 'libcctz.a' \
	-o -name 'libcryptopp.a' | tr '\n' ' ')

# Каталог установленной библиотеки yaml-cpp
#
# Реализация userver берёт yaml-cpp из установленной библиотеки: настроенная выше
# сборка разрешает её своим розыском, и стенду нужен тот же самый префикс. Префикс
# этот обязан нести тот же выпуск yaml-cpp, каким собран стенд самого yaml-cpp в
# наборе «tools/benchmark/yaml»: расхождение выпусков делает несравнимыми ряд
# соперника и ряд слоя поверх него
readonly YAMLCPP="${YAMLCPP_ROOT:-/opt/homebrew}"

# Каталог библиотек Boost
#
# Настройка userver сама нашла Boost и записала путь к его настройке в кэш сборки,
# а каталог библиотек лежит на два уровня выше записанного. Прежде стенд брал этот
# каталог из префикса yaml-cpp, что верно лишь пока префикс совпадает с системным:
# на префиксе, собранном отдельно, связывание отказывало розыском библиотеки
# «boost_stacktrace_basic»
readonly BOOST_CMAKE_DIR=$(sed -n 's|^Boost_DIR:PATH=\(.*\)$|\1|p' "$WORKDIR/CMakeCache.txt")

# Если настройка Boost в кэше сборки не найдена
if [ -z "$BOOST_CMAKE_DIR" ]; then
	# Выводим сообщение об отсутствии записи
	echo "Error: \"Boost_DIR\" not found in \"$WORKDIR/CMakeCache.txt\"" >&2
	# Выходим с ошибкой
	exit 1
fi

# Выполняем выведение каталога библиотек Boost из каталога его настройки
readonly BOOST_LIBDIR=$(cd "$BOOST_CMAKE_DIR/../.." && pwd)

# Если каталог библиотек Boost не существует
if [ ! -d "$BOOST_LIBDIR" ]; then
	# Выводим сообщение о неверном пути
	echo "Error: Boost library directory \"$BOOST_LIBDIR\" does not exist" >&2
	# Выходим с ошибкой
	exit 1
fi

##
# Сличаем выпуск yaml-cpp, какой ушёл в компиляцию библиотеки userver, с запрошенным
#
# Настройка userver ставит системный каталог включений в список раньше префикса, какой
# ей указали, и компилятор берёт заголовки yaml-cpp из системного каталога. Запись
# журнала настройки о найденной библиотеке остаётся при этом верной, а заголовки
# оказываются чужого выпуска: стенд собирается и даёт ряд слоя userver поверх одного
# выпуска, тогда как стенд самого yaml-cpp в наборе «tools/benchmark/yaml» собран из
# подмодуля другого выпуска. Расхождение это молчаливо, потому и поверяется здесь
##
# Выполняем чтение списка включений цели сборки библиотеки userver
readonly UV_INCLUDES=$(sed -n 's/^CXX_INCLUDES = //p' "$FLAGS_MAKE")

# Выполняем подготовку переменной действующего каталога включений
YAMLCPP_EFFECTIVE=""

# Выполняем перебор слов списка включений
for INCLUDE in $UV_INCLUDES; do
	# Если слово есть ключ с приклеенным каталогом, отделяем каталог от ключа
	case "$INCLUDE" in
		-I*) INCLUDE="${INCLUDE#-I}" ;;
		-*) continue ;;
	esac
	# Если заголовок yaml-cpp в очередном каталоге выложен, розыск окончен
	if [ -f "$INCLUDE/yaml-cpp/yaml.h" ]; then
		# Запоминаем действующий каталог включений
		YAMLCPP_EFFECTIVE="$INCLUDE"
		# Прекращаем перебор
		break
	fi
done

# Если каталог включений, несущий заголовок yaml-cpp, не найден
if [ -z "$YAMLCPP_EFFECTIVE" ]; then
	# Выводим сообщение об отсутствии заголовка
	echo "Error: no include directory with \"yaml-cpp/yaml.h\" in \"$FLAGS_MAKE\"" >&2
	# Выходим с ошибкой
	exit 1
fi

# Если в компиляцию ушёл не тот каталог, какой запрошен префиксом
if [ "$YAMLCPP_EFFECTIVE" != "$YAMLCPP/include" ]; then
	# Выводим сообщение о подмене выпуска
	echo "Error: userver is compiled against \"$YAMLCPP_EFFECTIVE\", not the requested \"$YAMLCPP/include\"" >&2
	# Выводим указание к устранению подмены
	echo "The system include directory precedes the prefix: drop the system yaml-cpp or take it as the prefix" >&2
	# Выходим с ошибкой
	exit 1
fi

# Выполняем сборку стенда контейнера JSON
echo "Build \"userver-json\""

# Выполняем сборку стенда контейнера JSON
#
# Определение «eval» здесь обязательно: значение определения пространства имён
# несет пробелы и заключено в кавычки внутри самой строки настроек
eval c++ $FLAGS $DEFS $INCS \
	-I"$JSON_STANDS" -I"$RE2_PREFIX/include" -I"$YAMLCPP/include" \
	-L"$YAMLCPP/lib" -L"$BOOST_LIBDIR" \
	"\"$JSON_STANDS/userver.cpp\"" \
	"\"$WORKDIR/universal/libuserver-universal.a\"" $LIBS \
	"\"$RE2_PREFIX/lib/libre2.a\"" \
	-lboost_stacktrace_basic -lyaml-cpp $SYSTEM_LIBS -lpthread \
	-o "\"$OUTPUT/userver-json\"" || exit 1

# Выполняем сборку стенда дерева настроек YAML
echo "Build \"userver-yaml\""

# Выполняем сборку стенда дерева настроек YAML
eval c++ $FLAGS $DEFS $INCS \
	-I"$YAML_STANDS" -I"$RE2_PREFIX/include" -I"$YAMLCPP/include" \
	-L"$YAMLCPP/lib" -L"$BOOST_LIBDIR" \
	"\"$YAML_STANDS/userver.cpp\"" \
	"\"$WORKDIR/universal/libuserver-universal.a\"" $LIBS \
	"\"$RE2_PREFIX/lib/libre2.a\"" \
	-lboost_stacktrace_basic -lyaml-cpp $SYSTEM_LIBS -lpthread \
	-o "\"$OUTPUT/userver-yaml\"" || exit 1

# Выводим сообщение об успешной сборке стендов
echo ""
echo "Stands are placed in \"$OUTPUT\""

# Выводим напоминание о сверке выполненной работы
#
# Стенды userver обязаны дать ту же контрольную сумму, что и стенды AWH и стенды
# их собственных двигателей: расхождение суммы означает, что сличается разный
# объём работы, и ряд такой цены не имеет
echo "Check parity before measuring: \"$OUTPUT/userver-json --checksum\""
echo "against \"/tmp/rival-json/awh --checksum\" and \"/tmp/rival-json/rapidjson --checksum\""
