# Сличение разбора INI с эталонами наречий
#
# Общепризнанного набора сверки у записи INI нет и быть не может: единого описания у неё
# нет вовсе, а наречия друг с другом несовместимы. Оттого эталоном служит не набор, а сами
# образцовые разборщики наречий - configparser языка Python и «git config» - и сличение
# ведётся наречие в наречие, а не вообще
#
# Сличение закрывает то, чего не закрывают ни набор проверок, ни ворошитель: согласие с
# тем, как наречие читают ЕГО СОБСТВЕННЫЕ потребители. Ворошитель сличает нас с нами же, а
# проверки писаны теми же руками, что и кодек, и разделяют с ним всякое заблуждение

import configparser, io, json, os, subprocess, sys


def python_oracle(path):
	"""Разбор текста настроек эталоном - модулем configparser языка Python

	Берётся RawConfigParser: подстановка значений у нашего разбора выключена по умолчанию,
	и сличать её здесь значило бы сличать две разные настройки. Строгость снимается тоже —
	эталон отвергает повторное свойство, а наречие того не требует
	"""

	parser = configparser.RawConfigParser(strict = False, allow_no_value = True,
	                                      delimiters = ('=', ':'), comment_prefixes = ('#', ';'))
	with open(path, encoding = 'utf-8') as file:
		text = file.read()
	try:
		parser.read_string(text)
	except (configparser.Error, ValueError):
		return None
	tree = {'': {}}
	# Свойства, до первого раздела объявленные, эталон кладёт в раздел DEFAULT
	for key, value in parser.defaults().items():
		tree[''][key] = [value if value is not None else '']
	for section in parser.sections():
		# Раздел эталона несёт и свойства раздела DEFAULT: их отсеиваем
		tree[section] = {}
		for key, value in parser.items(section, raw = True):
			if key in parser.defaults():
				continue
			tree[section][key] = [value if value is not None else '']
	return tree


def git_oracle(path):
	"""Разбор текста настроек эталоном - средством «git config»"""

	run = subprocess.run(['git', 'config', '-f', path, '--list', '-z'],
	                     capture_output = True, text = True, encoding = 'utf-8')
	if run.returncode != 0:
		return None
	tree = {'': {}}
	for record in run.stdout.split('\0'):
		if not record:
			continue
		name, _, value = record.partition('\n')
		parts = name.split('.')
		key = parts[-1]
		# Имя вида «раздел.свойство», «раздел.подраздел.свойство» либо голое имя свойства
		#
		# Голое имя есть свойство, до первого раздела объявленное: средство их принимает и
		# выдаёт без раздела вовсе. Ложатся они в раздел пустого имени - туда же, куда и у
		# нашего разбора
		if len(parts) == 1:
			label = ''
		elif len(parts) == 2:
			label = parts[0]
		else:
			label = '%s/%s' % (parts[0], '.'.join(parts[1:-1]))
		tree.setdefault(label, {}).setdefault(key, []).append(value)
	return tree


# Эталоны наречий: название наречия щупа, функция разбора, признак хранения перечня
#
# configparser перечня одноимённых свойств не хранит вовсе - повторное имя перезаписывает
# прежнее, - тогда как наш разбор их накапливает. Это разница договоров, а не расхождение
# разбора, и сличается тут последнее значение. «git config» перечень хранит и выдаёт
ORACLES = (
	('python', python_oracle, False, True, False),
	('git', git_oracle, True, True, True),
)


def collate(ours, theirs, listed, folded, sectioned):
	"""Сличение двух разборов с выдачей перечня расхождений"""

	# Наречие, регистра имён не учитывающее, сличается именами, к одному виду сведёнными
	#
	# configparser имена свойств приводит к нижнему регистру, а наш разбор сохраняет
	# написание первого вхождения: имена «A» и «a» суть одно свойство у обоих, а записаны
	# по-разному, и сличение написанием расходилось бы там, где расхождения нет
	if folded:
		ours = dict((label, dict((key.lower(), value) for key, value in items.items()))
		            for label, items in ours.items())
		theirs = dict((label, dict((key.lower(), value) for key, value in items.items()))
		              for label, items in theirs.items())

	# Наречие, регистра раздела не учитывающее, сличается именами разделов, к одному виду
	# сведёнными, а имя подраздела остаётся как есть: «git config» именно так и поступает
	#
	# Раздел пустой из сличения при этом изымается: «git config --list» перечисляет одни
	# лишь свойства, и раздела без свойств у него нет вовсе - ни объявленного, ни какого
	if sectioned:
		def normal(label):
			head, slash, tail = label.partition('/')
			return head.lower() + slash + tail
		ours = dict((normal(label), items) for label, items in ours.items() if items)
		theirs = dict((normal(label), items) for label, items in theirs.items() if items)

	diffs = []
	for label in sorted(set(ours) | set(theirs)):
		# Раздел пустого имени эталоны выдают по-своему, и пустой он сличению не подлежит
		if (label == '') and not ours.get('') and not theirs.get(''):
			continue
		if label not in ours:
			diffs.append('раздела %r нет у нас' % label)
			continue
		if label not in theirs:
			diffs.append('раздела %r нет у эталона' % label)
			continue
		for key in sorted(set(ours[label]) | set(theirs[label])):
			if key not in ours[label]:
				diffs.append('%s: свойства %r нет у нас' % (label, key))
			elif key not in theirs[label]:
				diffs.append('%s: свойства %r нет у эталона' % (label, key))
			else:
				first, second = ours[label][key], theirs[label][key]
				# Эталон, перечня не хранящий, сличается последним значением
				if not listed:
					first, second = first[-1:], second[-1:]
				if first != second:
					diffs.append('%s/%s: %r против %r' % (label, key, first, second))
	return diffs


def main():
	if len(sys.argv) < 3:
		print('Применение: compare.py <каталог корпуса> <стенд разбора>')
		return 2

	root, dump = sys.argv[1], sys.argv[2]

	# Счётчики исходов сличения по всякому наречию
	totals = {}

	# Перечень расхождений для раскладки
	report = []

	for dialect, oracle, listed, folded, sectioned in ORACLES:
		matched = diverged = refused = skipped = crashed = 0
		wider = narrower = 0
		rewritten = unwritten = 0
		for name in sorted(os.listdir(root)):
			if not name.endswith('.ini'):
				continue
			# Тексты, под наречие Git писанные, прочим наречиям не подаются
			#
			# Строятся они по правилам Git - подраздел в кавычках, отступ табуляцией, имя
			# без значения, - и наречию python неведомы вовсе: сличать там нечего, а счёт
			# расхождений эти тексты забивали шумом. Заведены они затем, что корпус общий
			# наречие Git почти не задевал: эталон отвергал 377 текстов из 434
			if name.startswith('gitlike-') and (dialect != 'git'):
				continue
			source = os.path.join(root, name)
			# Эталон, текста не осиливший, сличению не годен: своего мнения у него нет
			expected = oracle(source)
			if expected is None:
				skipped += 1
				##
				# Разбор текста, эталоном не осиленного, всё же ведётся - ради счёта
				#
				# Сличать тут нечего: своего мнения у эталона нет. Но молчание о целой
				# доле корпуса читается как «разобрано всё»: текстов таких у наречия Git
				# больше, чем сличённых. Счёт делит их надвое - принятые нами и
				# отвергнутые, - и доля принятых есть мера того, насколько мы шире
				# образца. Мера эта сама по себе не приговор: чтение вправе быть шире
				# там, где наречие того требует, - но расти она не должна незаметно
				##
				run = subprocess.run([dump, source, dialect], capture_output = True, text = True, encoding = 'utf-8')
				if run.returncode == 0:
					wider += 1
				elif run.returncode < 0:
					crashed += 1
					report.append({'наречие': dialect, 'случай': name,
					               'исход': 'щуп сорван сигналом %d на тексте, эталоном не осиленном' % (-run.returncode),
					               'отказ': run.stderr.strip()[:400]})
				else: narrower += 1
				continue
			run = subprocess.run([dump, source, dialect], capture_output = True, text = True, encoding = 'utf-8')
			##
			# Если работа щупа прекращена сигналом
			#
			# @note Срыв работы отказом разбора не является: отказ есть суждение кодека,
			#       а срыв - его отсутствие, и смешивать их нельзя
			##
			if run.returncode < 0:
				crashed += 1
				report.append({'наречие': dialect, 'случай': name,
				               'исход': 'щуп сорван сигналом %d' % (-run.returncode),
				               'отказ': run.stderr.strip()[:400]})
				continue
			if run.returncode != 0:
				refused += 1
				report.append({'наречие': dialect, 'случай': name, 'исход': 'разбор отвергнут',
				               'отказ': run.stderr.strip()})
				continue
			try:
				produced = json.loads(run.stdout)
			except json.JSONDecodeError as error:
				refused += 1
				report.append({'наречие': dialect, 'случай': name, 'исход': 'выдача щупа не читается',
				               'отказ': str(error)})
				continue
			# Сличение перезаписи: текст, нами записанный, подаётся эталону
			#
			# Сличение выше поверяет РАЗБОР, а запись поверялась одними нами - кругом через
			# собственный разбор, - и заблуждение, обоим общее, круг тот пережило бы. Здесь
			# же прочтённое эталоном из НАШЕЙ записи сличается с прочтённым им из текста
			# исходного: расхождение означает, что запись поменяла смысл
			#
			# Сличается лишь то, на чём разбор уже сошёлся: расхождение разбора отчитано
			# выше, и вменять его записи вторым разом нечего
			written = subprocess.run([dump, source, dialect, 'rewrite'], capture_output = True, text = True, encoding = 'utf-8')
			if (written.returncode == 0) and not collate(produced, expected, listed, folded, sectioned):
				mirror = os.path.join(root, '.rewrite-%s.ini' % dialect)
				with open(mirror, 'w', newline = '', encoding = 'utf-8') as file:
					file.write(written.stdout)
				# Эталон, перезаписи не осиливший, сличению не годен: своего мнения нет
				reflected = oracle(mirror)
				os.unlink(mirror)
				if reflected is None:
					unwritten += 1
					report.append({'наречие': dialect, 'случай': name,
					               'исход': 'эталон перезаписи не осилил'})
				else:
					marks = collate(produced, reflected, listed, folded, sectioned)
					if marks:
						rewritten += 1
						report.append({'наречие': dialect, 'случай': name,
						               'исход': 'ПЕРЕЗАПИСЬ СМЫСЛ ПОМЕНЯЛА', 'расхождения': marks})
			diffs = collate(produced, expected, listed, folded, sectioned)
			if diffs:
				diverged += 1
				report.append({'наречие': dialect, 'случай': name, 'исход': 'разбор разошёлся',
				               'расхождения': diffs[:8]})
			else:
				matched += 1
		totals[dialect] = (matched, diverged, refused, skipped, crashed, rewritten, unwritten, wider, narrower)

	for dialect, (matched, diverged, refused, skipped, crashed, rewritten, unwritten, wider, narrower) in totals.items():
		print('Наречие %s:' % dialect)
		print('  разбор совпал:          %d' % matched)
		print('  РАЗБОР РАЗОШЁЛСЯ:       %d' % diverged)
		print('  РАЗБОР ОТВЕРГНУТ:       %d' % refused)
		print('  ЩУП СОРВАН СИГНАЛОМ:    %d' % crashed)
		print('  ПЕРЕЗАПИСЬ ПОМЕНЯЛА СМЫСЛ: %d' % rewritten)
		print('  эталон текст не осилил: %d (из них приняли мы %d, отвергли %d)' % (skipped, wider, narrower))
		print('  эталон перезаписи не осилил: %d' % unwritten)

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
	if sum(sum(counts) for counts in totals.values()) == 0:
		print('ОТКАЗ: корпус пуст, поверять нечего', file = sys.stderr)
		return 2

	# Отказом отвечаем при всяком расхождении
	return 1 if any((d or r or c or w) for _, d, r, _, c, w, _, _, _ in totals.values()) else 0


if __name__ == '__main__':
	sys.exit(main())
