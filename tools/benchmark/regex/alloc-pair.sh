#!/usr/bin/env bash

# Парный прогон стенда замеров модуля регулярных выражений в двух сборках:
# штатной («awh», со встроенным распределителем) и без него («awh.noalloc»).
#
# Доля между двумя сборками одного текста отдаёт вклад awh::alloc в каждую
# строку набора: на иных сценариях наш распределитель оказывается БЫСТРЕЕ
# системного (его кэши тёплые), на иных — медленнее, и только снятие парой
# двух сборок делает вывод состоятельным. Сличение с PCRE2 идёт по варианту
# «awh.noalloc» — он равноценен сопернику по распределителю.
#
# Использование: tools/benchmark/regex/alloc-pair.sh [каталог стендов] [круги]
#   первый довод — каталог, где лежат собранные стенды (по умолчанию /tmp/rival-regex)
#   второй довод — количество кругов прогона каждой сборки (по умолчанию 3)
#
# Под каждый круг создаётся отдельный файл вида
#   awh.NN.log, awh.noalloc.NN.log, pcre2.NN.log
# в каталоге /tmp/awh-regex-alloc-<дата>, и сводная таблица per-scenario.

set -e

# Каталог собранных стендов сравнения
STANDS="${1:-/tmp/rival-regex}"
# Количество кругов прогона каждой сборки
ROUNDS="${2:-3}"

# Дата отметки времени для каталога и итогов
DATE=$(date +%Y%m%d)
# Каталог отчёта о парном прогоне
REPORT="/tmp/awh-regex-alloc-${DATE}"

# Если стенды не собраны — это отказ, а не предупреждение
if [ ! -x "$STANDS/awh" ] || [ ! -x "$STANDS/awh.noalloc" ] || [ ! -x "$STANDS/pcre2" ]; then
	echo "Стенды не собраны: ожидаются «$STANDS/{awh,awh.noalloc,pcre2}»"
	echo "Соберите их прежде:"
	echo "  tools/benchmark/regex/build.sh $STANDS"
	exit 1
fi

# Заводим каталог отчёта и снимаем версию собирателя, которой собран стенд
mkdir -p "$REPORT"
{
	echo "система:    $(uname -srm)"
	# Строк версии стенд сам не печатает, и запуск без доводов катит замер:
	# берём версию собирателя отдельным вызовом подменяющей команды
	echo "собиратель: $( (g++ --version 2>/dev/null || clang++ --version 2>/dev/null || echo unknown) | head -1)"
	echo "стенды:     $STANDS"
	echo "круги:      $ROUNDS"
	echo
} > "$REPORT/info.txt"

##
# Прогон идёт по фильтру сценариев с stdin
#
# Фильтр снимается аргументом «-f имя_сценария» у стенда: без него прогон
# отрабатывает все сценарии подряд. Здесь фильтр пуст — каждый сценарий
# идёт по одному кругу в одном прогоне, и медиана берётся по кругам
##

# Прогон стенда до результата в файл
#
# @param stand путь к бинарю стенда
# @param label метка (awh / awh.noalloc / pcre2)
# @param round номер круга
##
run_round() {
	local stand="$1"
	local label="$2"
	local round="$3"
	local outfile="$REPORT/${label}.$(printf '%02d' "$round").log"
	"$stand" > "$outfile" 2>&1
}

# Прогоняем каждый стенд положенное число кругов
for round in $(seq 1 "$ROUNDS"); do
	echo "круг $round / $ROUNDS"
	run_round "$STANDS/awh" "awh" "$round"
	run_round "$STANDS/awh.noalloc" "awh.noalloc" "$round"
	run_round "$STANDS/pcre2" "pcre2" "$round"
done

##
# Разбор логов и сведение трёх рядов в одну таблицу
#
# Стенд печатает блоками: имя сценария, скорость (match/s), затем строка
# с числом операций, временем, памятью процесса. Имя сценария отыскивается
# регулярным выражением — у новых сценариев оно самое длинное слово строки
# и не содержит латинских слогов на кириллице. Значение скорости лежит
# сразу за именем, округлено до двух разрядов, с суффиксом «(match/s)»
##

python3 - "$REPORT" "$ROUNDS" <<'PYEOF'
import os, re, sys
from statistics import median

report_dir = sys.argv[1]
rounds = int(sys.argv[2])

# Образец разбора строк лога: имя сценария и скорость в формате «(match/s)»
scen_re = re.compile(r'^(?P<name>[a-z][a-z0-9-]+)\s+(?P<rate>[\d.]+)\s+\(match/s\)$')

def parse_log(path):
    rates = {}
    with open(path) as f:
        for line in f:
            m = scen_re.match(line.rstrip())
            if m:
                rates.setdefault(m.group('name'), []).append(float(m.group('rate')))
    return rates

# Считываем все три стенда по всем кругам
stends = ['awh', 'awh.noalloc', 'pcre2']
data = {st: {} for st in stends}
for st in stends:
    for r in range(1, rounds + 1):
        log = os.path.join(report_dir, f'{st}.{r:02d}.log')
        if not os.path.exists(log):
            continue
        for name, vals in parse_log(log).items():
            data[st].setdefault(name, []).extend(vals)

def med(values):
    if not values:
        return None
    return median(values)

def f_rate(v):
    return f'{v:,.0f}' if v is not None else '   —'

def f_ratio(v):
    return f'{v:.3f}' if v is not None else '   —'

# Печатаем сводную таблицу
print(f"{'Сценарий':<22} {'awh ON':>12} {'awh OFF':>12} {'pcre2':>12}  {'ON/OFF':>7} {'OFF/PCRE2':>9} {'ON/PCRE2':>9}")
rows = []
for name in sorted(data['awh'].keys() | data['awh.noalloc'].keys() | data['pcre2'].keys()):
    on = med(data['awh'].get(name))
    off = med(data['awh.noalloc'].get(name))
    pc = med(data['pcre2'].get(name))
    on_off = (off / on) if (on and off) else None
    off_pc = (off / pc) if (off and pc) else None
    on_pc = (on / pc) if (on and pc) else None
    rows.append((name, on, off, pc, on_off, off_pc, on_pc))

for name, on, off, pc, on_off, off_pc, on_pc in rows:
    print(f'{name:<22} {f_rate(on):>12} {f_rate(off):>12} {f_rate(pc):>12}  {f_ratio(on_off):>7} {f_ratio(off_pc):>9} {f_ratio(on_pc):>9}')

# Дополнительно: сортированный список доли awh::alloc
print()
print('ДОЛЯ awh::alloc (медиана OFF/ON − 1, в процентах;')
print('  положительное число — наш распределитель ЗАМЕДЛЯЕТ, отрицательное — УСКОРЯЕТ):')
diffs = [(n, on, off, (off / on - 1.0) * 100.0) for (n, on, off, *_) in rows if on and off]
diffs.sort(key=lambda x: -x[3])
for name, on, off, pct in diffs:
    flag = '↑медленнее' if pct > 0 else '↓быстрее'
    print(f'  {name:<22} ON={on:>10,.0f} OFF={off:>10,.0f}  {pct:+7.2f} %  {flag}')
PYEOF

echo
echo "Отчёт: $REPORT"
echo "Таблица доли: $REPORT/diff.txt"
