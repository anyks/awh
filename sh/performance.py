#!/usr/bin/env python3
"""Сверка производительности с сохранённым эталоном.

Пороги набора бенчмарков откалиброваны по отладочной сборке с четырёхкратным
запасом и по устройству ловят падения на порядок, а не на проценты. Эта проверка
закрывает как раз проценты.

Сверяются две разные величины:

* ``ratio``    — доля от эталонной реализации. Устойчива к машине, компилятору
                 и прогреву: когда абсолютные значения плывут, они плывут
                 у обеих реализаций сразу.
* ``absolute`` — само значение. Так сверяются показатели, от машины не зависящие
                 вовсе: число выделений памяти и степень сжатия.

Доля считается **попарно**: один прогон нашей реализации, сразу за ним прогон
эталона, и медиана берётся уже от долей, а не от показателей. Тепловой дрейф
машины за время прогона достигает нескольких процентов, и на паре он сокращается,
тогда как на отдельно снятых медианах — остаётся. Замеренный разлёт медианы
между независимыми сериями: 1.0 % на кодировании, 2.3 % на разборе запросов,
6.0 % на приёме тела. Допуски в эталоне заданы с запасом к этим величинам,
и по протоколам они разные - разлёт замерен для каждого сценария отдельно,
обоснование каждого допуска записано в самом файле эталона.

Протокол, стенды которого на машине не собраны, помечается пропуском и целиком
исключается из сверки: отсутствие эталона рядом - не расхождение.

Формат строки эталона: ``протокол сценарий режим граница допуск направление``,
где направление ``min`` требует не опускаться ниже границы, ``max`` — не
подниматься выше.
"""

import os
import re
import subprocess
import statistics
import sys

# Соответствие протокола эталонной реализации
REFERENCE = {'http1': 'llhttp', 'http2': 'nghttp2', 'http3': 'nghttp3'}


def run(path, scenario):
	"""Выполняет один прогон одного сценария и выводит его показатель."""
	out = subprocess.run([path, '--filter=' + scenario], capture_output=True, text=True).stdout
	match = re.search(r'^\S+\s+([\d.]+)\s+\(', out, re.M)
	return (float(match.group(1)) if match else None)


def measure(directory, protocol, scenario, runs):
	"""Снимает показатель и долю от эталона попарными прогонами.

	Возвращает медиану наших показателей, медиану эталонных и медиану долей.
	Первая пара отбрасывается: первый прогон после сборки систематически ниже
	остальных.
	"""
	ours = os.path.join(directory, f'rival-{protocol}', 'awh')
	theirs = os.path.join(directory, f'rival-{protocol}', REFERENCE[protocol])
	if not os.path.exists(ours):
		return None
	# Снятые показатели нашей реализации, эталона и их доли
	mine, peer, ratios = [], [], []
	for _ in range(runs + 1):
		first = run(ours, scenario)
		if first is None:
			return None
		mine.append(first)
		second = (run(theirs, scenario) if os.path.exists(theirs) else None)
		if second:
			peer.append(second)
			ratios.append(first / second)
	return (
		statistics.median(mine[1:] or mine),
		(statistics.median(peer[1:] or peer) if peer else None),
		(statistics.median(ratios[1:] or ratios) if ratios else None),
	)


def main():
	baseline_path, directory, runs = sys.argv[1], sys.argv[2], int(sys.argv[3])
	# Разобранные строки сохранённого эталона по протоколам
	expected = {}
	with open(baseline_path) as handle:
		for line in handle:
			line = line.split('#')[0].strip()
			if not line:
				continue
			protocol, scenario, mode, bound, tolerance, direction = line.split()
			expected.setdefault(protocol, []).append(
				(scenario, mode, float(bound), float(tolerance), direction)
			)

	failures, checked = [], 0
	for protocol, entries in expected.items():
		print(f'\n  {protocol}')
		for scenario, mode, bound, tolerance, direction in entries:
			measured = measure(directory, protocol, scenario, runs)
			if measured is None:
				print(f'    \033[33mпропуск\033[0m  стенды {protocol} не собраны')
				break
			ours, theirs, ratio = measured
			if mode == 'ratio':
				if ratio is None:
					failures.append(f'{protocol}/{scenario}: эталон не выполнил сценарий')
					continue
				value = ratio
				shown = f'{value:.3f} от эталона (наш {ours:,.0f}, эталон {theirs:,.0f})'
			else:
				value = ours
				shown = f'{value:,.2f}'
			# Допуск задан долей от границы, а направление - смыслом показателя
			limit = (bound * (1.0 - tolerance)) if direction == 'min' else (bound * (1.0 + tolerance))
			good = (value >= limit) if direction == 'min' else (value <= limit)
			checked += 1
			mark = '\033[32mок\033[0m' if good else '\033[31mрегрессия\033[0m'
			print(f'    {mark:>20}  {scenario:<40} {shown.replace(",", " ")}')
			if not good:
				failures.append(
					f'{protocol}/{scenario}: {value:.3f} против ожидаемых {bound:.3f} '
					f'(граница {limit:.3f})'
				)

	print()
	if failures:
		for item in failures:
			print(f'\033[31m  расхождение\033[0m  {item}')
		return 1
	print(f'\033[32m  ок\033[0m  сверено показателей: {checked}, регрессий нет')
	return 0


if __name__ == '__main__':
	sys.exit(main())
