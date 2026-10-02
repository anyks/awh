#!/bin/sh
#
# @file stands.sh
# @date 2026-09-17
#
# @license{LicenseRef-AWH-1.0}
#
# @author Yuriy Lobarev
#
# @brief Раскладка ворошителя кодека по отладочным стендам
#
# @details Ворошитель осмыслен под надзирателями и на длинных прогонах, а надзиратели у
#          всякой системы свои: у FreeBSD и Linux они полные, у систем Sun иные, и
#          находка, невидимая на рабочей машине, вылезает на чужой. Своя `truncate()`
#          валила сборку всех трёх BSD, и на рабочей машине того видно не было вовсе
#
# @note Числа счётчиков ворошителя обязаны сойтись ДОСЛОВНО на всех машинах: источник
#       случайных величин засевается заданным зерном, и расхождение хоть одного счётчика
#       означает, что кодек повёл себя на этой системе иначе. Оттого свод сличает их, а
#       не одни лишь исходы
#
# @warning Третья сторона берётся ИЗ КЛОНА, живущего на самой машине, а не из свёртка:
#          «libdependence.a» весит 32 МБ, своя на всякий набор команд, а заголовки LZ4,
#          zstd и прочих ворошителю кодеков ABC и CEF нужны напрямую. Свёрток же несёт
#          одни исходные тексты - и оттого лёгок
#
# @warning Журнал сборки читается ДО уборки. Прежде уборка шла первой, и причина отказа
#          пропадала: восемь машин отвечали «исход 1» без единой строки довода, и
#          добывать её приходилось повторным заходом (17.09.2026)
#
# @copyright Copyright © 2026
#
# Вызов:
#   tools/fuzz/stands.sh <имя кодека> [число кругов]
#
# Переменные окружения:
#   AWH_STANDS — перечень машин видом «адрес|метка», по строке на машину
#

# Прекращаем работу при первом же отказе настройки
set -u

# Получаем имя кодека, ворошитель какого раскладывается
CODEC="${1:-}"

# Проверяем, задано ли имя кодека
if [ -z "$CODEC" ]; then
	echo "Вызов: $0 <имя кодека> [число кругов]" >&2
	exit 1
fi

# Получаем число кругов ворошения
ROUNDS="${2:-30000}"

# Проверяем аргументы перед включением в удалённую команду
case "$CODEC" in
	*[!a-z0-9-]*|'') echo "Недопустимое имя кодека" >&2; exit 1 ;;
esac
case "$ROUNDS" in
	*[!0-9]*|'') echo "Число кругов должно быть положительным целым" >&2; exit 1 ;;
esac
if ! [ "$ROUNDS" -gt 0 ] 2>/dev/null; then
	echo "Число кругов должно быть положительным целым" >&2
	exit 1
fi

# Получаем корень дерева исходных текстов
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"

#
# Перечень машин раскладки
#
# @note Умолчание держится здесь, а не в вызове: адреса стендов меняются переездом сети,
#       и править их в одном месте вернее, чем в памяти зовущего
#
MACHINES="${AWH_STANDS:-192.168.53.172|freebsd
192.168.53.173|netbsd
192.168.53.175|openbsd
192.168.53.153|dragonfly
192.168.53.104|solaris
192.168.53.158|openindiana
192.168.53.247|debian
192.168.53.163|alpine}"

# Метка раскладки, разводящая одновременные прогоны
STAMP=$$

# Свёрток исходных текстов, отправляемый машинам
BUNDLE="/tmp/awh-fuzz-$STAMP.tgz"

# Выполняем сборку свёртка исходных текстов
echo "Собираем свёрток: $BUNDLE"

#
# Выполняем сборку свёртка
#
# @note Признак «COPYFILE_DISABLE» обязателен: без него macOS кладёт в свёрток свои
#       служебные записи, а `tar` систем Sun спотыкается на них
#
( cd "$ROOT" && COPYFILE_DISABLE=1 tar --format=ustar -czf "$BUNDLE" \
	include src/codec src/num src/sys src/net src/encoding src/alloc \
	src/compressor src/cryptography submodules/zlib tools/fuzz ) || exit 1

# Каталог локальных журналов, сохраняемых после завершения раскладки
WORK=$(mktemp -d "${TMPDIR:-/tmp}/awh-fuzz-report-XXXXXX") || exit 1
REPORT="$WORK/report.log"
COUNTERS="$WORK/counters.log"
: > "$REPORT"
: > "$COUNTERS"

# Число запрошенных машин, удачных прогонов и отказов
EXPECTED=0
CLEAN=0
FAILED=0

#
# Перебираем машины в текущей оболочке, сохраняя итоговые счётчики
#
while IFS='|' read -r HOST TAG; do
	# Пропускаем пустые строки перечня машин
	[ -n "${HOST:-}" ] || continue
	EXPECTED=$((EXPECTED + 1))
	LOG="$WORK/machine-$EXPECTED.log"
	echo "=== $TAG ($HOST)" | tee -a "$REPORT"
	# Передаём свёрток и учитываем отказ передачи в общем результате
	if ! scp -q -o BatchMode=yes -o StrictHostKeyChecking=no -o ConnectTimeout=20 \
		-o ServerAliveInterval=30 -o ServerAliveCountMax=10 "$BUNDLE" "forman@$HOST:/tmp/" 2> "$LOG"; then
		cat "$LOG" | tee -a "$REPORT"
		echo "    !!! передача не удалась" | tee -a "$REPORT"
		FAILED=$((FAILED + 1))
		continue
	fi
	#
	# Сохраняем код SSH непосредственно, без конвейера с tee или tail.
	# Удалённая оболочка также сохраняет исход сборки или ворошителя до вывода и уборки.
	#
	ssh -n -o BatchMode=yes -o StrictHostKeyChecking=no -o ConnectTimeout=20 \
		-o ServerAliveInterval=30 -o ServerAliveCountMax=10 "forman@$HOST" \
		"STATUS=0
		 mkdir -p ~/fuzz-$STAMP && cd ~/fuzz-$STAMP &&
		 gzip -dc /tmp/awh-fuzz-$STAMP.tgz | tar -xf - || STATUS=\$?
		 if [ \$STATUS -eq 0 ]; then
			for CLONE in \$HOME/awh \$HOME/awh-v5 \$HOME/awh5; do
				[ -f \$CLONE/third_party/lib/libdependence.a ] && ln -sfn \$CLONE/third_party ./third_party && break
			done
			sh tools/fuzz/build.sh $CODEC . /tmp/fz-$STAMP > /tmp/fzb-$STAMP.log 2>&1
			STATUS=\$?
			if [ \$STATUS -ne 0 ]; then
				echo '    !!! СБОРКА ОТКАЗАЛА'
				cat /tmp/fzb-$STAMP.log
			else
				/tmp/fz-$STAMP/$CODEC-fuzz $ROUNDS > /tmp/fzr-$STAMP.log 2>&1
				STATUS=\$?
				cat /tmp/fzr-$STAMP.log
			fi
		 fi
		 echo \"    исход: \$STATUS\"
		 cd /
		 rm -rf ~/fuzz-$STAMP /tmp/fz-$STAMP /tmp/fzb-$STAMP.log /tmp/fzr-$STAMP.log /tmp/awh-fuzz-$STAMP.tgz
		 exit \$STATUS" > "$LOG" 2>&1
	STATUS=$?
	cat "$LOG" | tee -a "$REPORT"
	# Отказываем раскладке при ошибке SSH, сборки или самого ворошителя
	if [ "$STATUS" -ne 0 ]; then
		echo "    !!! удалённый прогон завершился с кодом $STATUS" | tee -a "$REPORT"
		FAILED=$((FAILED + 1))
		continue
	fi
	# Требуем ровно одну строку счётчиков от каждой запрошенной машины
	COUNT=$(grep -c "^$CODEC fuzz: [0-9]" "$LOG")
	if [ "$COUNT" -ne 1 ]; then
		echo "    !!! ожидалась одна строка счётчиков, получено $COUNT" | tee -a "$REPORT"
		FAILED=$((FAILED + 1))
		continue
	fi
	# Пометка инструментации не входит в счётчики воспроизводимого прогона
	grep "^$CODEC fuzz: [0-9]" "$LOG" | sed 's/ \[БЕЗ НАДЗИРАТЕЛЕЙ\]$//' >> "$COUNTERS"
	CLEAN=$((CLEAN + 1))
done <<EOF
$MACHINES
EOF

# Удаляем только свёрток текущего запуска; журналы остаются для разбора отказов
rm -f "$BUNDLE"

# Сличаем все полученные строки после удаления служебной пометки инструментации
VARIANTS=$(sort -u "$COUNTERS" | wc -l | tr -d ' ')
echo "=== СЛИЧЕНИЕ СЧЁТЧИКОВ" | tee -a "$REPORT"
if [ "$EXPECTED" -eq 0 ] || [ "$FAILED" -ne 0 ] || [ "$CLEAN" -ne "$EXPECTED" ] || [ "$VARIANTS" -ne 1 ]; then
	echo "!!! РАСКЛАДКА НЕ ПРОШЛА: запрошено $EXPECTED, успешно $CLEAN, отказов $FAILED, разных строк $VARIANTS" | tee -a "$REPORT"
	sort "$COUNTERS" | uniq -c | tee -a "$REPORT"
	echo "Журналы: $WORK"
	exit 1
fi

echo "счётчики сошлись число в число: машин $EXPECTED, без отказа $CLEAN" | tee -a "$REPORT"
echo "=== РАСКЛАДКА ВОРОШИТЕЛЯ ОКОНЧЕНА" | tee -a "$REPORT"
echo "Журналы: $WORK"
exit 0
