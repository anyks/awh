# Сличение разбора таблиц CSV с эталоном модуля csv языка Python
#
# Сличается разбор каждой таблицы корпуса при нескольких размерах куска подачи: договор о
# независимости выдачи от нарезки текста нарушается всяким знаком, чьё значение зависит от
# следующего за ним, и проверяется он здесь наравне с самим содержимым выдачи

import csv, json, os, subprocess, sys

# Размеры куска подачи, при каких ведётся сличение, ноль означает подачу текста целиком
CHUNKS = [0, 1, 2, 3, 7]

# Описание отказа, признаваемого намеренным расхождением с эталоном
#
# Поле в кавычках, не закрытое до конца текста, разбор признаёт отказом, а эталон выдаёт
# накопленное как есть. Решение это намеренное и описано в заголовке модуля: причина у
# такого текста ровно одна - он оборван. Сверяется здесь лишь то, что записи, обрыву
# предшествующие, с эталоном совпадают
UNTERMINATED = 'unterminated quoted field'


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
		# Выполняем перебор всех размеров куска подачи
		for chunk in CHUNKS:
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
