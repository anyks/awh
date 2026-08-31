# Сличение разбора TOML с эталоном - набором сверки toml-test
#
# Набор даёт для всякого годного текста ожидаемое дерево записью JSON, где всякое скалярное
# значение выдано парою вида {"type": "integer", "value": "42"}, а для негодного - один лишь
# сам текст, который разбор обязан отвергнуть. Сличение закрывает то, чего не закрывают ни
# набор проверок, ни ворошитель: СООТВЕТСТВИЕ ОПИСАНИЮ. Ворошитель сличает нас с нами же, а
# проверки писаны теми же руками, что и кодек, и разделяют с ним всякое заблуждение

import json, math, os, subprocess, sys

# Виды значений, сличаемые числом, а не записью
#
# Запись числа двойной точности у разных средств расходится начиная с последних знаков, и
# сличение записью выдавало бы расхождения там, где число одно и то же
NUMERIC = ('float',)

# Виды значений, сличаемые по составу отметки времени, а не по записи
#
# Набор сверки записывает долю секунды приведённою к трём знакам, тогда как описание TOML
# число знаков сохраняет за человеком: «.6» и «.600» дают одно и то же время, а записаны
# по-разному, и подменять написание кодек не вправе
STAMPED = ('datetime', 'datetime-local', 'time-local')


def stamp(text):
	"""Приведение отметки времени к виду, от числа знаков доли не зависящему"""

	head, dot, rest = text.partition('.')
	if not dot:
		return text
	# Отделяем долю секунды от смещения часового пояса, за нею идущего
	digits = ''
	for letter in rest:
		if not letter.isdigit():
			break
		digits += letter
	# Долю дополняем нулями до девяти знаков: столько несёт наносекунда
	return '%s.%s%s' % (head, digits.ljust(9, '0'), rest[len(digits):])


def collate(ours, theirs, path = ''):
	"""Сличение двух деревьев с выдачей перечня расхождений"""

	# Перечень найденных расхождений
	diffs = []

	# Если сличаемое место у обоих является скалярным значением
	if isinstance(ours, dict) and isinstance(theirs, dict) and ('type' in theirs):
		# Если вид значения не совпадает
		if ours.get('type') != theirs.get('type'):
			diffs.append('%s: вид %r против %r' % (path, ours.get('type'), theirs.get('type')))
			return diffs
		first, second = ours.get('value'), theirs.get('value')
		# Если значение сличается числом
		if theirs.get('type') in NUMERIC:
			try:
				ours_number, theirs_number = float(first), float(second)
				# Нечисло сличается признаком, а не равенством: нечисло не равно ничему,
				# в том числе и самому себе, и сличение равенством всегда бы расходилось
				if math.isnan(ours_number) or math.isnan(theirs_number):
					if not (math.isnan(ours_number) and math.isnan(theirs_number)):
						diffs.append('%s: нечисло против числа %r против %r' % (path, first, second))
				elif ours_number != theirs_number:
					diffs.append('%s: число %r против %r' % (path, first, second))
			except ValueError:
				if first != second:
					diffs.append('%s: запись %r против %r' % (path, first, second))
		elif theirs.get('type') in STAMPED:
			if stamp(first) != stamp(second):
				diffs.append('%s: отметка %r против %r' % (path, first, second))
		elif first != second:
			diffs.append('%s: содержимое %r против %r' % (path, first, second))
		return diffs

	# Если сличаемое место у обоих является таблицею
	if isinstance(ours, dict) and isinstance(theirs, dict):
		for name in sorted(set(ours) | set(theirs)):
			if name not in ours:
				diffs.append('%s/%s: пары нет у нас' % (path, name))
			elif name not in theirs:
				diffs.append('%s/%s: пары нет у эталона' % (path, name))
			else:
				diffs += collate(ours[name], theirs[name], '%s/%s' % (path, name))
		return diffs

	# Если сличаемое место у обоих является перечнем
	if isinstance(ours, list) and isinstance(theirs, list):
		if len(ours) != len(theirs):
			diffs.append('%s: длина перечня %d против %d' % (path, len(ours), len(theirs)))
			return diffs
		for i, (first, second) in enumerate(zip(ours, theirs)):
			diffs += collate(first, second, '%s/%d' % (path, i))
		return diffs

	# Если построения сличаемых мест расходятся
	diffs.append('%s: построение %s против %s' % (path, type(ours).__name__, type(theirs).__name__))
	return diffs


def main():
	if len(sys.argv) < 3:
		print('Применение: compare.py <каталог корпуса> <стенд разбора>')
		return 2

	root, dump = sys.argv[1], sys.argv[2]

	# Перечень случаев описания версии 1.0.0, по которому кодек и ведёт разбор
	#
	# Набор несёт случаи двух описаний разом, и без отбора кодеку вменялось бы в отказ то,
	# чего он не обещал: время без секунд, переводы строк во встроенной таблице, ограждение
	# «\e» и «\x41» - всё это нововведения 1.1.0
	listing = os.path.join(root, 'files-toml-1.0.0')
	allowed = None
	if os.path.exists(listing):
		with open(listing, encoding = 'utf-8') as file:
			allowed = set(line.strip() for line in file if line.strip())

	# Количество случаев, отсеянных как случаи иного описания
	skipped = 0

	# Счётчики исходов сличения
	matched = refused = accepted = diverged = broken = 0
	rewritten = refuted = 0
	# Счётчики случаев, сличению не подлежащих, и срывов работы щупа
	unreferenced = crashed = 0

	# Перечень расхождений для раскладки
	report = []

	# Выполняем перебор всех годных текстов набора сверки
	for base, _, files in sorted(os.walk(os.path.join(root, 'valid'))):
		for name in sorted(files):
			if not name.endswith('.toml'):
				continue
			source = os.path.join(base, name)
			label = os.path.relpath(source, root)
			# Случаи иного описания отсеиваются перечнем
			if (allowed is not None) and (label not in allowed):
				skipped += 1
				continue
			target = source[:-5] + '.json'
			##
			# Описания без эталона сличать нечем
			#
			# @note Считаются они отдельно: прежде такой случай отбрасывался молча, ни в
			#       один счётчик не попадая, и отчёт выглядел полным, не будучи им
			##
			if not os.path.exists(target):
				unreferenced += 1
				continue
			run = subprocess.run([dump, source], capture_output = True, text = True)
			##
			# Если работа щупа прекращена сигналом
			#
			# @note Срыв работы отказом разбора не является, и выдавать его за отвергнутый
			#       текст нельзя: отказ есть суждение кодека, а срыв - его отсутствие
			##
			if run.returncode < 0:
				crashed += 1
				report.append({'случай': label, 'исход': 'щуп сорван сигналом %d' % (-run.returncode),
				               'отказ': run.stderr.strip()[:400]})
				continue
			# Если разбор отверг годный текст
			if run.returncode != 0:
				broken += 1
				report.append({'случай': label, 'исход': 'годный текст отвергнут', 'отказ': run.stderr.strip()})
				continue
			with open(target, encoding = 'utf-8') as file:
				expected = json.load(file)
			try:
				produced = json.loads(run.stdout)
			except json.JSONDecodeError as error:
				broken += 1
				report.append({'случай': label, 'исход': 'выдача щупа не читается', 'отказ': str(error)})
				continue
			diffs = collate(produced, expected)
			if diffs:
				diverged += 1
				report.append({'случай': label, 'исход': 'дерево разошлось', 'расхождения': diffs})
			else:
				matched += 1
				##
				# Сличение перезаписи: наш текст читается заново и судится ЭТАЛОНОМ
				#
				# Сличение выше поверяет РАЗБОР, а запись поверялась одними нами - кругом
				# через собственный разбор, - и заблуждение, чтению и записи общее, круг
				# тот пережило бы. Здесь дерево, прочтённое из НАШЕЙ записи, сличается с
				# деревом набора: общим у двух путей остаётся один разбор, а он эталоном
				# и поверен. Расхождение значит, что запись поменяла смысл
				#
				# Сличается лишь то, на чём разбор уже сошёлся: расхождение разбора
				# отчитано выше, и вменять его записи вторым разом нечего
				##
				written = subprocess.run([dump, source, 'rewrite'], capture_output = True, text = True)
				if written.returncode != 0:
					refuted += 1
					report.append({'случай': label, 'исход': 'ЗАПИСЬ ОТВЕРГЛА ДЕРЕВО',
					               'отказ': written.stderr.strip()[:400]})
				else:
					mirror = os.path.join(os.path.dirname(source), '.rewrite.toml')
					with open(mirror, 'w', newline = '', encoding = 'utf-8') as file:
						file.write(written.stdout)
					again = subprocess.run([dump, mirror], capture_output = True, text = True)
					os.unlink(mirror)
					if again.returncode != 0:
						rewritten += 1
						report.append({'случай': label, 'исход': 'ПЕРЕЗАПИСЬ НЕ ЧИТАЕТСЯ ОБРАТНО',
						               'отказ': again.stderr.strip()[:400]})
					else:
						try:
							mirrored = json.loads(again.stdout)
						except json.JSONDecodeError as error:
							rewritten += 1
							report.append({'случай': label, 'исход': 'выдача щупа не читается',
							               'отказ': str(error)})
							continue
						marks = collate(mirrored, expected)
						if marks:
							rewritten += 1
							report.append({'случай': label, 'исход': 'ПЕРЕЗАПИСЬ СМЫСЛ ПОМЕНЯЛА',
							               'расхождения': marks})

	# Выполняем перебор всех негодных текстов набора сверки
	for base, _, files in sorted(os.walk(os.path.join(root, 'invalid'))):
		for name in sorted(files):
			if not name.endswith('.toml'):
				continue
			source = os.path.join(base, name)
			# Случаи иного описания отсеиваются перечнем
			if (allowed is not None) and (os.path.relpath(source, root) not in allowed):
				skipped += 1
				continue
			run = subprocess.run([dump, source], capture_output = True, text = True)
			# Если работа щупа прекращена сигналом
			if run.returncode < 0:
				crashed += 1
				report.append({'случай': os.path.relpath(source, root),
				               'исход': 'щуп сорван сигналом %d' % (-run.returncode),
				               'отказ': run.stderr.strip()[:400]})
				continue
			# Если разбор верно отверг негодный текст
			if run.returncode == 1:
				refused += 1
			else:
				accepted += 1
				report.append({'случай': os.path.relpath(source, root), 'исход': 'негодный текст принят',
				               'выдача': run.stdout.strip()[:400]})

	print('Всего случаев: %d' % (matched + diverged + broken + refused + accepted + crashed))
	print('  дерево совпало:            %d' % matched)
	print('  негодный текст отвергнут:  %d' % refused)
	print('  НЕГОДНЫЙ ТЕКСТ ПРИНЯТ:     %d' % accepted)
	print('  ДЕРЕВО РАЗОШЛОСЬ:          %d' % diverged)
	print('  ГОДНЫЙ ТЕКСТ ОТВЕРГНУТ:    %d' % broken)
	print('  ПЕРЕЗАПИСЬ СМЫСЛ ПОМЕНЯЛА:  %d' % rewritten)
	print('  ЗАПИСЬ ОТВЕРГЛА ДЕРЕВО:     %d' % refuted)
	print('  ЩУП СОРВАН СИГНАЛОМ:       %d' % crashed)
	print('  отсеяно как случаи 1.1.0:  %d' % skipped)
	print('  годных текстов без эталона: %d' % unreferenced)

	# Выполняем раскладку расхождений
	if report:
		path = os.path.join(root, 'diffs.json')
		with open(path, 'w', encoding = 'utf-8') as file:
			json.dump(report, file, ensure_ascii = False, indent = '\t')
		print('\nРасхождения разложены: %s' % path)


	##
	# Пустой корпус — отказ, а не чистый прогон
	#
	# Сорванный захват корпуса оставлял каталог пустым, сторож сборки смотрел на наличие
	# каталога, а не на состав его, и сличение отчитывалось нулём расхождений, не поверив
	# ни единого случая
	##
	if (matched + diverged + broken + refused + accepted + crashed) == 0:
		print('ОТКАЗ: корпус пуст, поверять нечего', file = sys.stderr)
		return 2

	# Отказом отвечаем при всяком расхождении: рост совпавших, купленный ослаблением
	# отказов, ростом не является
	return 1 if (accepted or diverged or broken or crashed or rewritten or refuted) else 0


if __name__ == '__main__':
	sys.exit(main())
