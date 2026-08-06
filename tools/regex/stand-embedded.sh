#!/bin/sh
# Раскладка переносимой проверки модуля регулярных выражений на встроенный стенд
# и прогон её там.
#
# Стенд отделён от tools/regex/stand.sh потому, что на машине встроенной
# компилятора нет вовсе и поставить его некуда: пакетного менеджера у сборок
# вроде OpenWRT не бывает, а места под вооружение не хватило бы и с ним.
# Поэтому проверка собирается встречной сборкой на стороне, а на машину
# передаётся готовым файлом со статической связкой, какому на месте не нужно
# ничего, кроме ядра.
#
# Использование: tools/regex/stand-embedded.sh <пользователь@узел> [порт]
#
# Переменные окружения:
#   CROSS_CXX  - команда встречного компилятора, обязательна
#   CROSS_HOST - узел сборки вида <пользователь@узел>; при задании сборка
#                ведётся на нём, иначе на рабочей машине
#   CROSS_PORT - порт узла сборки, по умолчанию 22
#
# Пример прогона на OpenWRT x86-64 сборкой связкой openwrt-sdk, разложенной
# на стенде Fedora:
#
#   CROSS_HOST=forman@10.100.1.249 \
#   CROSS_CXX=~/openwrt/openwrt-sdk-25.12.4-x86-64_gcc-14.3.0_musl.Linux-x86_64/staging_dir/toolchain-x86_64_gcc-14.3.0_musl/bin/x86_64-openwrt-linux-g++ \
#   sh tools/regex/stand-embedded.sh root@10.100.1.246 2222
#
# Чего проверка эта не покажет: сборка ведётся не тем компилятором, что стоял
# бы на машине, поэтому расхождения самого компилятора она не выявит. Выявит
# она отличия библиотеки С и ядра, - а именно ради них встроенная машина
# и проверяется.

set -e

# Служебные метки файловой системы macOS в передаваемые наборы не кладутся:
# распаковщики прочих систем на них жалуются
COPYFILE_DISABLE=1
export COPYFILE_DISABLE

TARGET="$1"
PORT="${2:-22}"

if [ -z "$TARGET" ]; then
	echo "Использование: $0 <пользователь@узел> [порт]" >&2
	exit 2
fi

if [ -z "$CROSS_CXX" ]; then
	echo "Не задана переменная CROSS_CXX с командой встречного компилятора" >&2
	exit 2
fi

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
CROSS_PORT="${CROSS_PORT:-22}"
WORK=$(mktemp -d -t awh-regex-cross)

# Набор исходных текстов, потребных переносимой проверке
#
# Перечень ведётся вручную, а не сборкой целиком: набор модуля мал, а дерево
# исходных текстов библиотеки велико, и передача его заняла бы много дольше
SOURCES="include/regex include/encoding/unicode include/compressor/types.hpp
	include/encoding/ascii.hpp include/sys/global.hpp
	include/sys/macro_push.hpp include/sys/macro_pop.hpp
	src/regex src/encoding/unicode
	tools/regex/conformance.cpp tools/regex/conformance.hpp"

# Связка ключей сборки
#
# Статическая связка обязательна: набор библиотек встроенной машины неизвестен
# заранее, и связка динамическая упёрлась бы там в отсутствие libstdc++
FLAGS="-std=c++17 -O2 -static -Iinclude -Itools/regex"

# Порождаем запись хранилища собранных выражений на рабочей машине, дабы стенд
# проверил её восстановлением у себя. Наборы программы пишутся образом памяти,
# поэтому запись зависит от устройства машины: стенду надлежит либо восстановить
# её в точности, либо отвергнуть опознанием устройства, но не прочесть неверно.
RECORD=""
HOST_CXX="${CXX:-c++}"
if command -v "$HOST_CXX" >/dev/null 2>&1 ; then
	if "$HOST_CXX" -std=c++17 -O2 -I"$ROOT/include" -I"$ROOT/tools/regex" \
		"$ROOT/tools/regex/conformance.cpp" "$ROOT"/src/regex/*.cpp "$ROOT"/src/encoding/unicode/*.cpp \
		-o "$WORK/conformance" 2>"$WORK/compile.log" ; then
		if "$WORK/conformance" "--write=$WORK/record.bin" >/dev/null 2>&1 ; then
			RECORD="$WORK/record.bin"
			echo "Порождена запись хранилища рабочей машины"
		fi
	fi
	if [ -z "$RECORD" ] ; then
		echo "Запись хранилища рабочей машины не порождена, проверка её будет пропущена"
	fi
else
	echo "Компилятор на рабочей машине не обнаружен, проверка записи будет пропущена"
fi

# Ведём встречную сборку
#
# Сборка на узле стороннем и сборка на машине рабочей разведены потому, что
# связка встречного вооружения зачастую разложена не там, где ведётся работа:
# вооружение OpenWRT собирается лишь под Linux, тогда как работа идёт с macOS
if [ -n "$CROSS_HOST" ]; then
	echo "Ведём встречную сборку на узле $CROSS_HOST"
	# shellcheck disable=SC2086
	tar czf - -C "$ROOT" $SOURCES | ssh -p "$CROSS_PORT" "$CROSS_HOST" "
		set -e
		rm -rf /tmp/awh-regex-cross
		mkdir -p /tmp/awh-regex-cross
		cd /tmp/awh-regex-cross
		gzip -dc | tar xf -
		# Связка вооружения OpenWRT отыскивает свои заголовочные файлы
		# по переменной STAGING_DIR и без неё берёт заголовочные файлы
		# рабочей машины, отчего сборка рассыпается. Переменная выводится
		# из пути к компилятору: он лежит в bin связки
		export STAGING_DIR=\"\${STAGING_DIR:-\$(dirname \$(dirname \"$CROSS_CXX\"))}\"
		# Разбор обработки исключений при статической связке лежит в libgcc_eh,
		# и связки, где libgcc поставляется общей библиотекой, её отдельно
		# не подключают. Наличие её заранее неизвестно, поэтому связывание
		# ведётся сперва без неё, а при отказе повторяется с нею
		\"$CROSS_CXX\" $FLAGS \
			tools/regex/conformance.cpp src/regex/*.cpp src/encoding/unicode/*.cpp \
			-o conformance 2>compile.log || \
		\"$CROSS_CXX\" $FLAGS \
			tools/regex/conformance.cpp src/regex/*.cpp src/encoding/unicode/*.cpp \
			-o conformance -lgcc_eh 2>compile.log || \
		{ echo 'Встречная сборка не выполнена:' >&2; tail -30 compile.log >&2; exit 4; }
	"
	# Готовый файл забирается через вывод ssh: подсистема sftp на узле сборки
	# доступна не всегда, тогда как поток вывода доступен всегда
	ssh -p "$CROSS_PORT" "$CROSS_HOST" 'cat /tmp/awh-regex-cross/conformance' > "$WORK/embedded"
else
	echo "Ведём встречную сборку на рабочей машине"
	# shellcheck disable=SC2086
	(cd "$ROOT" && "$CROSS_CXX" $FLAGS \
		tools/regex/conformance.cpp src/regex/*.cpp src/encoding/unicode/*.cpp \
		-o "$WORK/embedded")
fi

chmod +x "$WORK/embedded"
echo "Собран файл проверки, $(wc -c < "$WORK/embedded") байт"

echo "Раскладываем проверку на стенд $TARGET"

# Передача ведётся через ввод tar, а не средством scp: подсистема sftp на
# встроенных машинах отключена чаще, чем включена
if [ -n "$RECORD" ] ; then
	tar cf - -C "$WORK" embedded record.bin | ssh -p "$PORT" "$TARGET" '
		rm -rf /tmp/awh-regex-stand && mkdir -p /tmp/awh-regex-stand
		cd /tmp/awh-regex-stand && tar xf -
	'
else
	tar cf - -C "$WORK" embedded | ssh -p "$PORT" "$TARGET" '
		rm -rf /tmp/awh-regex-stand && mkdir -p /tmp/awh-regex-stand
		cd /tmp/awh-regex-stand && tar xf -
	'
fi

rm -rf "$WORK"

echo "Прогоняем проверку на стенде"
ssh -p "$PORT" "$TARGET" '
	set -e
	cd /tmp/awh-regex-stand
	uname -srm
	chmod +x embedded
	if [ -f record.bin ] ; then
		./embedded --read=record.bin
	else
		./embedded
	fi
'
