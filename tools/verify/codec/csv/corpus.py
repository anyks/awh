# Составление корпуса таблиц для сличения с эталоном
import os, random, sys

root = sys.argv[1]
os.makedirs(root, exist_ok = True)

# Образцы набора csv-spectrum, воспроизведённые по его описанию
spectrum = {
	'comma_in_quotes': 'first,last,address,city,zip\nJohn,Doe,120 any st.,"Anytown, WW",08123\n',
	'empty': 'a,b,c\n1,"",""\n2,3,4\n',
	'empty_crlf': 'a,b,c\r\n1,"",""\r\n2,3,4\r\n',
	'escaped_quotes': 'a,b\n1,"ha ""ha"" ha"\n3,4\n',
	'json': 'key,val\n1,"{""type"": ""Point"", ""coordinates"": [102.0, 0.5]}"\n',
	'newlines': 'a,b,c\n1,2,3\n"Once upon\na time",5,6\n7,8,9\n',
	'newlines_crlf': 'a,b,c\r\n1,2,3\r\n"Once upon\r\na time",5,6\r\n7,8,9\r\n',
	'quotes_and_newlines': 'a,b\n1,"ha \n""ha"" \nha"\n3,4\n',
	'simple': 'a,b,c\n1,2,3\n',
	'simple_crlf': 'a,b,c\r\n1,2,3\r\n',
	'utf8': 'a,b,c\n1,2,3\n4,5,ʤ\n',
	'quotes_and_spaces': 'a,b\n1, "ha ""ha"" ha"\n',
	'leading_space': 'a,b\n 1,2\n',
	'trailing_space': 'a,b\n1 ,2 \n',
	'quoted_empty_last': 'a,b,c\n1,2,""\n',
	'no_trailing_newline': 'a,b\n1,2',
	'single_column': 'a\nb\nc\n',
	'blank_lines': 'a,b\n\n1,2\n\n\n3,4\n',
	'crlf_in_quotes': 'a,b\n"x\r\ny",2\n',
	'cr_only': 'a,b\r1,2\r',
	'quote_at_end': 'a,"b"\n',
	'ragged': 'a,b,c\n1\n2,3\n4,5,6,7\n',
}

for name, text in spectrum.items():
	open(os.path.join(root, 'spectrum_' + name + '.csv'), 'w', newline = '', encoding = 'utf-8').write(text)

# Порождение случайных таблиц из знаков, разбор задевающих
random.seed(20260813)
pieces = ['a', 'bb', '', ',', '"', '""', '\n', '\r', '\r\n', ' ', '\t', 'ʤ', 'x"y', '"z"', 'ha ""ha""', '\\', 'null']

for index in range(2000):
	fields = []
	for _ in range(random.randint(1, 6)):
		value = ''.join(random.choice(pieces) for _ in range(random.randint(0, 4)))
		# Поле берётся в кавычки с отменой кавычек внутри него
		if random.random() < 0.5:
			fields.append('"' + value.replace('"', '""') + '"')
		# Поле записывается как есть, а знаки, разбор задевающие, из него убираются
		else: fields.append(value.replace('"', '').replace(',', '').replace('\n', '').replace('\r', ''))
	text = ''
	for _ in range(random.randint(1, 5)):
		text += ','.join(fields) + random.choice(['\n', '\r\n', '\r'])
	open(os.path.join(root, 'random_%04d.csv' % index), 'w', newline = '', encoding = 'utf-8').write(text)

# Порождение таблиц с намеренно нарушенной записью
#
# Записи эти договором не описаны вовсе, и сходиться с эталоном разбор здесь не обязан:
# ценность их в том, что расхождение на такой записи требует объяснения - либо это
# намеренное решение, либо дефект
random.seed(777)
alphabet = list('ab,"\n\r \t') + ['\r\n', '""', '"a"', ',,']

for index in range(4000):
	text = ''.join(random.choice(alphabet) for _ in range(random.randint(1, 30)))
	open(os.path.join(root, 'rough_%04d.csv' % index), 'w', newline = '', encoding = 'utf-8').write(text)

print('составлено файлов:', len(os.listdir(root)))
