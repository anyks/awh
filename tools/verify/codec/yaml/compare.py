#!/usr/bin/env python3
"""Сверка кодека YAML с набором yaml-test-suite по потоку событий"""
import os, re, subprocess, sys, json

import sys
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
            if v == '|' or v.startswith('|'):
                key = k; buf = []; indent = None
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
                key = k; buf = []; indent = None
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
    base = entries[0] if entries else {}
    for i, e in enumerate(entries):
        merged = dict(base); merged.update(e)
        if 'yaml' in merged:
            cases.append((name if i == 0 else '%s:%d' % (name, i), merged))

ok = bad = failok = failbad = skipped = 0
diffs = []
for label, case in cases:
    body = case.get('yaml')
    if body is None: continue
    # Знаки набора: ␣ пробел, последовательность ———» есть одна подача, ∎ конец без перевода
    body = body.replace('␣', ' ').replace('—', '').replace('»', '\t').replace('∎', '').replace('↵', '')
    if '<SPC>' in body or '<TAB>' in body: continue
    open('/tmp/yts.yaml', 'w', encoding='utf-8').write(body)
    r = subprocess.run([EVENTS, '/tmp/yts.yaml'], capture_output=True, text=True, errors='replace')
    got = r.stdout
    if str(case.get('fail', '')).strip() in ('true', 'True'):
        if 'ОТКАЗ' in got: failok += 1
        else: failbad += 1; diffs.append((label, 'ДОЛЖЕН БЫЛ ОТКАЗАТЬ', case.get('tree', ''), got))
        continue
    want = case.get('tree')
    if want is None: skipped += 1; continue
    if got.strip() == want.strip(): ok += 1
    else: bad += 1; diffs.append((label, 'РАСХОЖДЕНИЕ', want, got))

print('всего %d: совпало %d, разошлось %d, отказ верный %d, отказа не дал %d, без эталона %d'
      % (len(cases), ok, bad, failok, failbad, skipped))
path = os.path.join(OUTPUT, 'diffs.json')
json.dump([{'case': d[0], 'вид': d[1], 'эталон': d[2], 'наше': d[3]} for d in diffs],
          open(path, 'w', encoding='utf-8'), ensure_ascii=False, indent=1)
print('расхождения разложены в', path)
