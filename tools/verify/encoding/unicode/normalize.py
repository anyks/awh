#!/usr/bin/env python3
"""
@file normalize.py
@date 2026-08-03
@license{LicenseRef-AWH-1.0}

@brief Стенд сверки нормализации текста со сторонним эталоном — набором таблиц
       Юникода в составе Python. Стенд дополняет стенд normalize.cpp: эталонная
       реализация GNU libunistring в составе сборки задаёт лишь канонические
       представления, тогда как представления совместимости NFKD и NFKC ей
       не задаются, а приведением доменных имён применяются.

       Набор сверки выгружается стендом normalize.cpp в режиме «--dump».
       Символы, изданию стандарта эталона неизвестные, из сверки исключаются.

@copyright Copyright © 2026
"""

import sys
import unicodedata

# Имена видов нормального представления в порядке их обозначений
FORMS = ('NFD', 'NFC', 'NFKD', 'NFKC')


def main():
	"""Сверка выгруженного набора приведений с эталоном"""
	source = (open(sys.argv[1], encoding='utf-8') if len(sys.argv) > 1 else sys.stdin)
	checked = 0
	skipped = 0
	diverged = 0
	shown = []
	for line in source:
		left, right = line.split('|')
		fields = left.split()
		form = FORMS[int(fields[0])]
		codes = [int(field, 16) for field in fields[1:]]
		# Символы, изданию стандарта эталона неизвестные, сверке не подлежат
		if any(unicodedata.category(chr(code)) == 'Cn' for code in codes):
			skipped += 1
			continue
		checked += 1
		ours = [int(value, 16) for value in right.split()]
		theirs = [ord(letter) for letter in unicodedata.normalize(form, ''.join(chr(code) for code in codes))]
		if ours != theirs:
			diverged += 1
			if len(shown) < 20:
				shown.append('%s, текст %s: AWH %s, Python %s' % (
					form,
					' '.join('U+%04X' % code for code in codes),
					' '.join('U+%04X' % code for code in ours),
					' '.join('U+%04X' % code for code in theirs)))
	for item in shown:
		sys.stdout.write('%s\n' % item)
	sys.stdout.write('издание Юникода эталона: %s\n' % unicodedata.unidata_version)
	sys.stdout.write('сличений нормализации: %d, пропущено: %d, расхождений: %d\n' % (checked, skipped, diverged))
	return (1 if diverged > 0 else 0)


if __name__ == '__main__':
	sys.exit(main())
