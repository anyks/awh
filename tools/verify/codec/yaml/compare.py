#!/usr/bin/env python3
"""Сверка кодека YAML с набором yaml-test-suite по потоку событий"""
import os, re, subprocess, sys, json
OUTPUT = sys.argv[1] if len(sys.argv) > 1 else '/tmp/verify-yaml'
SRC = os.path.join(OUTPUT, 'corpus')
EVENTS = os.path.join(OUTPUT, 'events')
cases = []
for name in sorted(os.listdir(SRC)):
    if not name.endswith('.yaml'): continue
    text = open(os.path.join(SRC, name), encoding='utf-8').read()
    # разбор простого подмножества YAML набора: список записей с блочными значениями
    entries = []
    cur = None
    key = None
    buf = []
    indent = None
    for line in text.split('\n'):
        m = re.match(r'^- (\w+): ?(.*)$', line)
        if m and (cur is None or key is None or True):
            if key and buf is not None and indent is not None:
                cur[key] = '\n'.join(buf) + ('\n' if buf else '')
            cur = {}; entries.append(cur); key = None; buf = []; indent = None
            k, v = m.group(1), m.group(2)
            if v.startswith('|'):
                # Указатель отступа заголовка отсчитывается от отступа отображения,
                # а не от первой строки содержимого: без него значащие пробелы в
                # начале строк срезались вместе с отступом
                marked = re.search(r'\|[+-]?(\d)', v)
                key = k; buf = []; indent = (2 + int(marked.group(1))) if marked else None
            else:
                cur[k] = v
            continue
        m = re.match(r'^  (\w+): ?(.*)$', line)
        if m and cur is not None:
            if key and indent is not None:
                cur[key] = '\n'.join(buf) + ('\n' if buf else '')
            key = None; buf = []; indent = None
            k, v = m.group(1), m.group(2)
            if v.startswith('|'):
                # Указатель отступа заголовка отсчитывается от отступа отображения
                marked = re.search(r'\|[+-]?(\d)', v)
                key = k; buf = []; indent = (2 + int(marked.group(1))) if marked else None
            else:
                cur[k] = v
            continue
        if key is not None and cur is not None:
            if line.strip() == '':
                buf.append(''); continue
            if indent is None:
                indent = len(line) - len(line.lstrip(' '))
            if len(line) - len(line.lstrip(' ')) >= indent:
                buf.append(line[indent:])
            else:
                cur[key] = '\n'.join(buf) + ('\n' if buf else '')
                key = None; buf = []; indent = None
    if key and cur is not None:
        cur[key] = '\n'.join(buf) + ('\n' if buf else '')
    # Наследуются лишь пояснительные поля: указания `fail` и `tree` принадлежат
    # своей записи, и наследование их приписывало бы негодность всякой записи файла
    base = {k: v for k, v in (entries[0] if entries else {}).items()
            if k in ('name', 'from', 'tags', 'note')}
    for i, e in enumerate(entries):
        merged = dict(base); merged.update(e)
        if 'yaml' in merged:
            cases.append((name if i == 0 else '%s:%d' % (name, i), merged))

ok = bad = failok = failbad = skipped = marked = crashed = 0
diffs = []
for label, case in cases:
    body = case.get('yaml')
    if body is None: continue
    # Знаки набора: ␣ пробел, последовательность ———» есть одна подача, ∎ конец без перевода
    body = body.replace('␣', ' ').replace('—', '').replace('»', '\t').replace('∎', '').replace('↵', '')
    ##
    # Случаи с указателями знаков в теле сличению не подлежат
    #
    # Знаки «<SPC>» и «<TAB>» набор оставляет пояснением для человека, а не телом
    # текста, и подставить их нечем. Прежде такой случай отбрасывался молча, ни в один
    # счётчик не попадая: отчёт выглядел полным, не будучи им
    ##
    if '<SPC>' in body or '<TAB>' in body:
        marked += 1
        continue
    open('/tmp/yts.yaml', 'w', encoding='utf-8').write(body)
    r = subprocess.run([EVENTS, '/tmp/yts.yaml'], capture_output=True, text=True, errors='replace')
    got = r.stdout
    if str(case.get('fail', '')).strip() in ('true', 'True'):
        if 'ОТКАЗ' in got: failok += 1
        else: failbad += 1; diffs.append((label, 'ДОЛЖЕН БЫЛ ОТКАЗАТЬ', case.get('tree', ''), got))
        continue
    want = case.get('tree')
    ##
    # Случай без эталонного потока событий проверяется на живучесть
    #
    # Набор для части случаев потока не даёт вовсе - о них сказано, что они годны по
    # правилам описания, но полезными не являются, и составители не желают поощрять
    # их поддержку. Сличать там нечего, однако прежде такой случай пропускался совсем,
    # и падение разбора на нём осталось бы незамеченным
    ##
    if want is None:
        skipped += 1
        # Если щуп прекратил работу сигналом либо донесением санитайзера
        if (r.returncode < 0) or ('Sanitizer' in r.stderr) or ('runtime error' in r.stderr):
            crashed += 1
            diffs.append((label, 'ПАДЕНИЕ БЕЗ ЭТАЛОНА', '', (got + r.stderr)[:2000]))
        continue
    # Знаки набора стоят и в эталонном дереве: пробел в конце записи иначе не разглядеть
    want = want.replace('␣', ' ').replace('—', '').replace('»', '\t').replace('∎', '').replace('↵', '')
    if got.strip() == want.strip(): ok += 1
    else: bad += 1; diffs.append((label, 'РАСХОЖДЕНИЕ', want, got))

print('всего %d: совпало %d, разошлось %d, отказ верный %d, отказа не дал %d, без эталона %d (падений %d), со знаками-пояснениями %d'
      % (len(cases), ok, bad, failok, failbad, skipped, crashed, marked))

##
# Пустой корпус — отказ, а не чистый прогон
#
# Сорванный захват корпуса оставлял каталог пустым, сторож сборки смотрел на наличие
# каталога, а не на состав его, и сличение отчитывалось «разошлось 0» ни по одному
# случаю. Прогон без единого случая ничего не поверяет
##
if len(cases) == 0:
    print('ОТКАЗ: корпус пуст, поверять нечего', file=sys.stderr)
    sys.exit(2)
path = os.path.join(OUTPUT, 'diffs.json')
json.dump([{'case': d[0], 'вид': d[1], 'эталон': d[2], 'наше': d[3]} for d in diffs],
          open(path, 'w', encoding='utf-8'), ensure_ascii=False, indent=1)
print('расхождения разложены в', path)

# Отчитываемся отказом при всяком расхождении с эталоном
sys.exit(1 if (bad or failbad or crashed) else 0)
