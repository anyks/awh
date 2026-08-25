# Сличение разбора таблиц CSV с эталоном модуля csv языка Python
#
# Сличается разбор каждой таблицы корпуса при нескольких размерах куска подачи: договор о
# независимости выдачи от нарезки текста нарушается всяким знаком, чьё значение зависит от
# следующего за ним, и проверяется он здесь наравне с самим содержимым выдачи

import csv, json, os, subprocess, sys

# Размеры куска подачи, при каких ведётся сличение, ноль означает подачу текста целиком
CHUNKS = [0, 1, 2, 3, 7, 63, 64, 65, 511, 512, 1024, 4096]

# Описание отказа, признаваемого намеренным расхождением с эталоном
#
# Поле в кавычках, не закрытое до конца текста, разбор признаёт отказом, а эталон выдаёт
# накопленное как есть. Решение это намеренное и описано в заголовке модуля: причина у
# такого текста ровно одна - он оборван. Сверяется здесь лишь то, что записи, обрыву
# предшествующие, с эталоном совпадают
UNTERMINATED = 'unterminated quoted field'

# Отказы режима заголовка, признаваемые намеренными расхождениями с эталоном
#
# Эталон о заголовке не судит вовсе: пустое имя он отдаёт пустым ключом, повторное имя
# молча теряет вместе со значением, а по пустому тексту выдаёт «fieldnames = None» и ни
# единой записи. Разбор наш все три случая отвергает нарочно, и правила эти записаны
# кодами отказов: имя поля есть ключ обращения, а ключ пустой либо повторный обращения
# не задаёт. Сличается здесь лишь то, что ПРИНЯТОЕ нами совпадает с эталоном
HEADING = (
	'empty field name in header',
	'duplicate field name in header',
	'header requested but input is empty',
	'field count does not match the header'
)


def oracle(path):
	"""Разбор таблицы эталоном - модулем csv языка Python"""

	with open(path, newline = '', encoding = 'utf-8') as file:
		rows = []
		# Выполняем перебор всех записей, выданных эталоном
		for row in csv.reader(file):
			# Пустую строку эталон выдаёт записью без полей, а разбор её пропускает
			if len(row) == 0:
				continue
			# Строку из единственного пустого поля эталон от пустой строки не отличает
			if (len(row) == 1) and (row[0] == ''):
				continue
			rows.append(row)
	return rows


def heading(path):
	"""Разбор таблицы заголовком эталоном - модулем csv языка Python

	Сличению подлежат лишь таблицы, у каких всякая запись несёт ровно столько полей,
	сколько несёт заголовок: запись короче заголовка эталон дополняет пустыми значениями
	по ключам, а длиннее - складывает остаток в отдельный ключ, тогда как разбор наш
	отдаёт запись как есть. Расхождение это не в разборе, а в СПОСОБЕ ВЫДАЧИ: у эталона
	запись есть отображение, у нас - перечень, и ровнять одно к другому значило бы
	сличать не разбор, а собственный переводчик. Таблицы неровные сличаются обычным
	путём, без заголовка, и без надзора не остаются
	"""

	rows = oracle(path)
	# Если таблица пуста, заголовка в ней нет вовсе
	if not rows:
		return None
	# Количество полей заголовка таблицы
	width = len(rows[0])
	# Выполняем перебор всех записей таблицы
	for row in rows[1:]:
		# Если количество полей записи расходится с заголовком, сличение пропускается
		if len(row) != width:
			return None
	return rows


def main():
	if len(sys.argv) < 3:
		print('Применение: compare.py <каталог корпуса> <стенд разбора>')
		return 2

	root, dump = sys.argv[1], sys.argv[2]

	# Количество выполненных прогонов сличения
	checked = 0
	# Количество намеренных расхождений с эталоном
	expected = 0
	# Обнаруженные расхождения с эталоном
	mismatch = []

	# Выполняем перебор всех таблиц корпуса
	for name in sorted(os.listdir(root)):
		path = os.path.join(root, name)
		# Получаем разбор таблицы эталоном
		try:
			reference = oracle(path)
		except Exception as error:
			print('эталон отказал на %s: %s' % (name, error))
			continue
		# Получаем разбор таблицы заголовком эталоном
		try:
			headed = heading(path)
		except Exception:
			headed = None
		# Выполняем перебор всех размеров куска подачи
		for chunk in CHUNKS:
			# Выполняем сличение разбора таблицы заголовком
			if headed is not None:
				run = subprocess.run([dump, path, str(chunk), 'header'], capture_output = True)
				checked += 1
				# Если разбор заголовком прекращён отказом
				if run.returncode != 0:
					note = run.stderr.decode().strip()
					# Если отказ намеренным расхождением с эталоном не является
					if (note not in HEADING) and (note != UNTERMINATED):
						mismatch.append((name, chunk, 'отказ разбора заголовком: ' + note, ''))
					else:
						expected += 1
				else:
					value = json.loads(run.stdout.decode('utf-8'))
					if value != headed:
						mismatch.append((name, chunk, 'заголовком: ' + json.dumps(value, ensure_ascii = False), json.dumps(headed, ensure_ascii = False)))
			result = subprocess.run([dump, path, str(chunk)], capture_output = True)
			checked += 1
			# Если разбор прекращён отказом
			if result.returncode != 0:
				message = result.stderr.decode().strip()
				# Если отказ не является намеренным расхождением с эталоном
				if message != UNTERMINATED:
					mismatch.append((name, chunk, 'отказ разбора: ' + message, ''))
					continue
				actual = json.loads(result.stdout.decode('utf-8'))
				# Выполняем сверку записей, обрыву предшествующих
				if actual != reference[:len(actual)]:
					mismatch.append((name, chunk, json.dumps(actual, ensure_ascii = False), json.dumps(reference, ensure_ascii = False)))
					continue
				expected += 1
				continue
			actual = json.loads(result.stdout.decode('utf-8'))
			if actual != reference:
				mismatch.append((name, chunk, json.dumps(actual, ensure_ascii = False), json.dumps(reference, ensure_ascii = False)))

	print('прогонов: %d, намеренных расхождений: %d, расхождений: %d' % (checked, expected, len(mismatch)))

	# Выполняем вывод обнаруженных расхождений
	for name, chunk, actual, reference in mismatch[:20]:
		print('--- %s, нарезка %d' % (name, chunk))
		print('  разбор:', actual[:300])
		print('  эталон:', reference[:300])


	##
	# Пустой корпус — отказ, а не чистый прогон
	#
	# Сорванный захват корпуса оставлял каталог пустым, сторож сборки смотрел на наличие
	# каталога, а не на состав его, и сличение отчитывалось нулём расхождений, не поверив
	# ни единого случая
	##
	if checked == 0:
		print('ОТКАЗ: корпус пуст, поверять нечего', file = sys.stderr)
		return 2

	return (1 if mismatch else 0)


sys.exit(main())
