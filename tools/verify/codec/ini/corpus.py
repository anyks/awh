# Составление корпуса текстов настроек для сличения с эталоном
#
# Общепризнанного набора сверки у записи INI нет и быть не может: единого описания у неё
# нет вовсе, а наречия друг с другом несовместимы. Оттого корпус порождается здесь, а не
# забирается со стороны, и порождается он воспроизводимо: тот же корпус на всякой машине

import os, random, sys

root = sys.argv[1]
os.makedirs(root, exist_ok = True)

# Тексты, покрывающие разночтения наречий и края разбора
#
# Всякий подобран под то, в чём наречия расходятся либо в чём разбор способен оступиться:
# знак примечания, разделитель имени со значением, кавычки, продолжение строки, пустое
# значение, повторное имя, обвязка пробелами
SAMPLES = {
	'simple': '[section]\nkey = value\n',
	'no-spaces': '[section]\nkey=value\n',
	'colon': '[section]\nkey: value\n',
	'empty-value': '[section]\nkey =\n',
	'empty-section': '[section]\n',
	'global-before-section': 'loose = 1\n[section]\nkey = value\n',
	'comment-hash': '# примечание\n[section]\nkey = value\n',
	'comment-semicolon': '; примечание\n[section]\nkey = value\n',
	'comment-indented': '  # примечание\n[section]\nkey = value\n',
	'inline-semicolon': '[section]\nkey = value ; хвост\n',
	'inline-hash': '[section]\nkey = value # хвост\n',
	'repeated-key': '[section]\nkey = first\nkey = second\n',
	'repeated-section': '[section]\na = 1\n[section]\nb = 2\n',
	'spaces-around': '[ section ]\n  key   =   value  \n',
	'value-with-equals': '[section]\nkey = a=b=c\n',
	'value-with-colon': '[section]\nkey = a:b:c\n',
	'value-quoted': '[section]\nkey = "value"\n',
	'value-with-hash': '[section]\nkey = a#b\n',
	'value-empty-quotes': '[section]\nkey = ""\n',
	'name-with-dot': '[a.b]\nkey = value\n',
	'name-with-space': '[a b]\nkey = value\n',
	'key-with-dot': '[section]\na.b = value\n',
	'utf8': '[раздел]\nимя = значение\n',
	'crlf': '[section]\r\nkey = value\r\n',
	'no-trailing-newline': '[section]\nkey = value',
	'blank-lines': '[section]\n\nkey = value\n\n\n',
	'continuation-indent': '[section]\nkey = first\n  second\n',
	'tabs': '[section]\nkey\t=\tvalue\n',
	'value-only-spaces': '[section]\nkey =    \n',
	'many-sections': '[a]\nx = 1\n[b]\nx = 2\n[c]\nx = 3\n',
	'digits': '[section]\nkey = 0123\nother = 1.10\n',
	'bom': '﻿[section]\nkey = value\n',
	'array-suffix': '[section]\nkey[] = first\nkey[] = second\n',
	'bracket-in-name': '[x[]]\nkey = value\n',
}

for name, text in SAMPLES.items():
	with open(os.path.join(root, 'sample-' + name + '.ini'), 'w', newline = '', encoding = 'utf-8') as file:
		file.write(text)

# Порождение текстов из кусков, разбор задевающих
#
# Зерно закрепляется намеренно: корпус обязан быть одним и тем же при всяком прогоне и на
# всякой машине, иначе расхождение, однажды найденное, второй раз не воспроизвести
random.seed(20260823)

# Куски имён и значений
# Запись «имя[]» сюда не берётся намеренно: она есть добавление к перечню значений и
# правится отдельной настройкой, а порождённая случайно мерила бы не разбор, а настройку
NAMES = ['a', 'bb', 'key', 'ключ', 'a.b', 'a b', 'A']

# Куски значений
VALUES = ['', 'v', 'значение', '1', '0123', 'a=b', 'a:b', 'a;b', 'a#b', '"q"', ' pad ', 'a\\nb']

# Разделители имени со значением
SEPARATORS = [' = ', '=', ': ', ':', '\t=\t']

for index in range(400):
	lines = []
	for _ in range(random.randint(1, 4)):
		# Заголовок раздела ставится не всякий раз: свойства до первого раздела законны
		if random.random() < 0.7:
			lines.append('[%s]' % random.choice(NAMES))
		for _ in range(random.randint(0, 4)):
			lines.append('%s%s%s' % (random.choice(NAMES), random.choice(SEPARATORS), random.choice(VALUES)))
		# Примечание ставится изредка
		if random.random() < 0.2:
			lines.append(random.choice(['# текст', '; текст']))
		# Пустая строка ставится изредка
		if random.random() < 0.2:
			lines.append('')
	text = '\n'.join(lines) + '\n'
	with open(os.path.join(root, 'random-%04d.ini' % index), 'w', newline = '', encoding = 'utf-8') as file:
		file.write(text)

print('Корпус составлен: %s' % root)
