#!/usr/bin/env python3
"""
@file generate.py
@date 2026-07-31

@license{LicenseRef-AWH-1.0}

@author Yuriy Lobarev

@telegram{forman}
@phone{+7 (910) 983-95-90}

@email forman@anyks.com
@site https://anyks.com

@brief Порождение таблиц базы данных символов Юникода по файлам
       базы данных символов Юникода (UCD). Порождаются таблицы общих категорий,
       письменностей, их расширений, двоичных свойств, классов двунаправленности,
       простого приведения регистра и разбиения на графемные кластеры.

@copyright Copyright © 2026
"""

import os
import sys

# Путь к каталогу файлов базы данных символов Юникода
TABLES = os.path.join(
	os.path.dirname(os.path.abspath(__file__)),
	'..', '..', 'submodules', 'pcre2', 'maint', 'Unicode.tables'
)

# Наибольшее кодовое значение символа Юникода
MAX_CODEPOINT = 0x10FFFF


def records(name):
	"""Разбор файла базы данных символов Юникода на записи «диапазон; значения»"""
	path = os.path.join(TABLES, name)
	for line in open(path, encoding='utf-8'):
		line = line.split('#')[0].strip()
		if not line:
			continue
		fields = [field.strip() for field in line.split(';')]
		bounds = fields[0].split('..')
		begin = int(bounds[0], 16)
		end = int(bounds[1], 16) if len(bounds) > 1 else begin
		yield begin, end, fields[1:]


def compress(items):
	"""Слияние соседних диапазонов с одинаковым значением"""
	result = []
	for begin, end, value in sorted(items):
		if result and (result[-1][2] == value) and (result[-1][1] + 1 == begin):
			result[-1][1] = end
		else:
			result.append([begin, end, value])
	return result


def categories():
	"""Таблица общих категорий символов"""
	items = []
	for begin, end, fields in records('DerivedGeneralCategory.txt'):
		items.append((begin, end, fields[0]))
	return compress(items)


def scripts():
	"""Таблица письменностей символов"""
	items = []
	for begin, end, fields in records('Scripts.txt'):
		items.append((begin, end, fields[0]))
	return compress(items)


def extensions():
	"""Таблица расширений письменностей символов"""
	items = []
	for begin, end, fields in records('ScriptExtensions.txt'):
		items.append((begin, end, tuple(sorted(fields[0].split()))))
	return compress(items)


def defaults(name):
	"""Умолчания значений свойства, заданные строками «@missing»"""
	items = []
	for line in open(os.path.join(TABLES, name), encoding='utf-8'):
		line = line.strip()
		if not line.startswith('#'):
			continue
		line = line.lstrip('#').strip()
		if not line.startswith('@missing:'):
			continue
		fields = [field.strip() for field in line[len('@missing:'):].split(';')]
		bounds = fields[0].split('..')
		items.append((int(bounds[0], 16), int(bounds[1], 16), fields[1]))
	return items


def bidirectional():
	"""Таблица классов двунаправленности символов

	Умолчания классов для символов, значение которых явно не задано, записаны
	строками «@missing» внутри примечаний файла и обязательны к учёту: без них
	незаданные символы арабских и еврейских блоков получают неверный класс.
	"""
	# Умолчания записаны полными именами классов, тогда как записи файла — сокращениями
	short = {}
	for group in aliases('bc').values():
		code = next((name for name in group if len(name) <= 3), None)
		if code is None:
			continue
		for name in group:
			short[name] = code
	# Размещаем умолчания классов двунаправленности по кодовым значениям
	table = {}
	for begin, end, value in defaults('DerivedBidiClass.txt'):
		for code in range(begin, end + 1):
			table[code] = short.get(value, value)
	# Размещаем явно заданные классы двунаправленности
	for begin, end, fields in records('DerivedBidiClass.txt'):
		for code in range(begin, end + 1):
			table[code] = fields[0]
	items = [(code, code, value) for code, value in sorted(table.items())]
	return compress(items)


def graphemes():
	"""Таблица разбиения на графемные кластеры"""
	items = []
	for begin, end, fields in records('GraphemeBreakProperty.txt'):
		items.append((begin, end, fields[0]))
	for begin, end, fields in records('emoji-data.txt'):
		if fields[0] == 'Extended_Pictographic':
			items.append((begin, end, 'Extended_Pictographic'))
	return compress(items)


def indic():
	"""Таблица свойства положения символа в сочетании индийских письменностей"""
	items = []
	for begin, end, fields in records('DerivedCoreProperties.txt'):
		if (len(fields) > 1) and (fields[0] == 'InCB'):
			items.append((begin, end, fields[1]))
	return compress(items)


def binaries():
	"""Таблицы двоичных свойств символов"""
	result = {}
	sources = ('PropList.txt', 'DerivedCoreProperties.txt', 'emoji-data.txt')
	for source in sources:
		for begin, end, fields in records(source):
			result.setdefault(fields[0], []).append((begin, end, 0))
	result = {name: compress(items) for name, items in result.items()}
	# Отражаемость при двунаправленном выводе задана основным файлом базы данных
	result['Bidi_Mirrored'] = mirrored()
	return result


def mirrored():
	"""Диапазоны символов, отражаемых при двунаправленном выводе

	Свойство определяется набором символов, которым сопоставлен отражённый символ,
	то есть файлом BidiMirroring.txt, а не полем отражаемости основного файла базы
	данных. Источник выбран сличением с эталонной реализацией: поле отражаемости
	задаёт 554 символа, тогда как эталон соответствует 428 символам отражения.
	"""
	items = []
	for begin, end, fields in records('BidiMirroring.txt'):
		items.append((begin, end, 0))
	return compress(items)


def folding():
	"""Таблица простого приведения регистра символов"""
	simple = {}
	for begin, end, fields in records('CaseFolding.txt'):
		if fields[0] in ('C', 'S'):
			simple[begin] = int(fields[1].split()[0], 16)
	return simple


def orbits(simple):
	"""Наборы символов, приводимых к одному значению"""
	groups = {}
	for code in range(0, MAX_CODEPOINT + 1):
		value = simple.get(code, code)
		if (value != code) or (code in simple):
			groups.setdefault(value, set()).add(code)
	for value, members in groups.items():
		members.add(value)
	return {value: sorted(members) for value, members in groups.items() if len(members) > 1}


def aliases(kind):
	"""Набор имён и сокращений значения свойства"""
	result = {}
	for line in open(os.path.join(TABLES, 'PropertyValueAliases.txt'), encoding='utf-8'):
		line = line.split('#')[0].strip()
		if not line:
			continue
		fields = [field.strip() for field in line.split(';')]
		if (len(fields) < 3) or (fields[0] != kind):
			continue
		result.setdefault(fields[2], set()).update(fields[1:])
	return result


def properties():
	"""Набор имён и сокращений двоичных свойств"""
	result = {}
	for line in open(os.path.join(TABLES, 'PropertyAliases.txt'), encoding='utf-8'):
		line = line.split('#')[0].strip()
		if not line:
			continue
		fields = [field.strip() for field in line.split(';')]
		if len(fields) < 2:
			continue
		result.setdefault(fields[1], set()).update(fields)
	return result


def normal(name):
	"""Приведение имени свойства к нормальному виду"""
	return ''.join(letter.lower() for letter in name if letter not in '_- ')


# Соответствие обозначений общих категорий их идентификаторам свойств
CATEGORY_ID = {
	'Cc': 0x0003, 'Cf': 0x0004, 'Cn': 0x0005, 'Co': 0x0006, 'Cs': 0x0007,
	'Ll': 0x0009, 'Lm': 0x000A, 'Lo': 0x000B, 'Lt': 0x000C, 'Lu': 0x000D,
	'Mc': 0x0010, 'Me': 0x0011, 'Mn': 0x0012,
	'Nd': 0x0014, 'Nl': 0x0015, 'No': 0x0016,
	'Pc': 0x0018, 'Pd': 0x0019, 'Pe': 0x001A, 'Pf': 0x001B, 'Pi': 0x001C,
	'Po': 0x001D, 'Ps': 0x001E,
	'Sc': 0x0020, 'Sk': 0x0021, 'Sm': 0x0022, 'So': 0x0023,
	'Zl': 0x0025, 'Zp': 0x0026, 'Zs': 0x0027
}

# Соответствие обозначений групп категорий их идентификаторам свойств
GROUP_ID = {
	'C': 0x0002, 'L': 0x0008, 'LC': 0x000E, 'M': 0x000F,
	'N': 0x0013, 'P': 0x0017, 'S': 0x001F, 'Z': 0x0024
}

# Соответствие особых имён расширенных классов их идентификаторам свойств
SPECIAL_ID = {
	'Any': 0x0001, 'Xan': 0x0028, 'Xps': 0x0029,
	'Xsp': 0x002A, 'Xwd': 0x002B, 'Xuc': 0x002C, 'ASCII': 0x002F
}

# Основания идентификаторов свойств, отсчитываемых от вида свойства
SCRIPT_BASE = 0x0100
UNITED_BASE = 0x0400
EXTENDED_BASE = 0x0800
BINARY_BASE = 0x1000
BIDI_BASE = 0x1800


def supported():
	"""Набор имён свойств, принимаемых эталонной реализацией"""
	path = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'sh', 'unicode.accepted')
	result = set()
	for line in open(path, encoding='utf-8'):
		line = line.strip()
		if not line:
			continue
		# Имена сличаются в нормальном виде: эталон имён по написанию не различает
		if '=' in line:
			kind, value = line.split('=', 1)
			result.add(normal(kind) + '=' + normal(value))
		else:
			result.add(normal(line))
	return result


def table(name, items):
	"""Порождение таблицы диапазонов значений свойства"""
	lines = ['/**', ' * @brief Таблица диапазонов кодовых значений: %s' % name, ' *', ' */']
	lines.append('const awh::unicode::interval_t awh::unicode::%s[] = {' % name)
	for begin, end, value in items:
		lines.append('\t{0x%X, 0x%X, %d},' % (begin, end, value))
	lines.append('};')
	lines += ['/**', ' * @brief Количество диапазонов таблицы: %s' % name, ' *', ' */']
	lines.append('const size_t awh::unicode::%s_COUNT = %d;' % (name, len(items)))
	return lines




def build():
	"""Формирование таблиц и соответствия имён свойств их идентификаторам"""
	accepted = supported()
	names = {}
	tables = {}
	# Общие категории символов
	tables['CATEGORIES'] = [
		(begin, end, CATEGORY_ID[value]) for begin, end, value in categories()
	]
	for group in aliases('gc').values():
		# Определяем обозначение категории среди её имён и сокращений
		code = next((name for name in group if (name in CATEGORY_ID) or (name in GROUP_ID)), None)
		if code is None:
			continue
		value = CATEGORY_ID.get(code, GROUP_ID.get(code))
		for name in group:
			if normal(name) in accepted:
				names[normal(name)] = value
	# Особые имена расширенных классов, не принадлежащие базе данных Юникода
	for name, value in SPECIAL_ID.items():
		if normal(name) in accepted:
			names[normal(name)] = value
	# Письменности символов
	order = sorted(set(item[2] for item in scripts()))
	index = {value: position for position, value in enumerate(order)}
	tables['SCRIPTS'] = [(begin, end, index[value]) for begin, end, value in scripts()]
	# Письменность неназначенных символов диапазонами не задана и отводится последней
	index['Unknown'] = len(order)
	# Дополняем соответствие письменностей их сокращёнными обозначениями
	for group in aliases('sc').values():
		position = next((index[name] for name in group if name in index), None)
		if position is None:
			continue
		for name in group:
			index[name] = position
	for group in aliases('sc').values():
		value = next((name for name in group if name in index), None)
		if value is None:
			continue
		for name in group:
			if normal(name) in accepted:
				names[normal(name)] = UNITED_BASE + index[value]
			if ('sc=' + normal(name)) in accepted:
				names['sc=' + normal(name)] = SCRIPT_BASE + index[value]
			if ('scx=' + normal(name)) in accepted:
				names['scx=' + normal(name)] = UNITED_BASE + index[value]
	# Расширения письменностей символов
	sets = []
	items = []
	for begin, end, value in extensions():
		members = tuple(index[name] for name in value if name in index)
		if members not in sets:
			sets.append(members)
		items.append((begin, end, sets.index(members)))
	tables['EXTENSIONS'] = items
	# Классы двунаправленности символов
	order = sorted(set(item[2] for item in bidirectional()))
	position = {value: number for number, value in enumerate(order)}
	tables['BIDIRECTIONAL'] = [
		(begin, end, position[value]) for begin, end, value in bidirectional()
	]
	for group in aliases('bc').values():
		value = next((name for name in group if name in position), None)
		if value is None:
			continue
		for name in group:
			if ('bc=' + normal(name)) in accepted:
				names['bc=' + normal(name)] = BIDI_BASE + position[value]
	# Классы разбиения текста на графемные кластеры
	order = ['Control', 'CR', 'LF', 'Extend', 'ZWJ', 'Regional_Indicator', 'Prepend',
	 'SpacingMark', 'L', 'V', 'T', 'LV', 'LVT', 'Extended_Pictographic']
	position = {value: number for number, value in enumerate(order)}
	tables['CLUSTERS'] = [
		(begin, end, position[value]) for begin, end, value in graphemes() if value in position
	]
	# Положение символа в сочетании индийских письменностей
	order = ['None', 'Consonant', 'Extend', 'Linker']
	position = {value: number for number, value in enumerate(order)}
	tables['INDIC'] = [
		(begin, end, position[value]) for begin, end, value in indic() if value in position
	]
	# Двоичные свойства символов
	properties_map = properties()
	order = sorted(binaries().keys())
	items = []
	spans = []
	for number, value in enumerate(order):
		spans.append((len(items), len(binaries()[value])))
		items += [(begin, end, number) for begin, end, _ in binaries()[value]]
	tables['BINARIES'] = items
	for value in order:
		group = properties_map.get(value, {value})
		for name in group:
			if normal(name) in accepted:
				names[normal(name)] = BINARY_BASE + order.index(value)
	# Обозначения двоичных свойств двунаправленности, разрешаемые эталонной реализацией
	#
	# Обозначение вида «bc=» задаёт класс двунаправленности, однако эталонная реализация
	# разрешает им и двоичные свойства двунаправленности, сличая имена нестрого
	for name, value in (('bc=m', 'Bidi_Mirrored'), ('bc=c', 'Bidi_Control'), ('bc=control', 'Bidi_Control')):
		if (name in accepted) and (value in order):
			names[name] = BINARY_BASE + order.index(value)
	return tables, sets, spans, names, order


def emit(tables, sets, spans, names, order):
	"""Выпуск исходного файла таблиц свойств Юникода"""
	lines = []
	lines.append('/**')
	lines.append(' * @file table.cpp')
	lines.append(' * @date 2026-07-31')
	lines.append(' *')
	lines.append(' * @license{LicenseRef-AWH-1.0}')
	lines.append(' *')
	lines.append(' * @author Yuriy Lobarev')
	lines.append(' *')
	lines.append(' * @telegram{forman}')
	lines.append(' * @phone{+7 (910) 983-95-90}')
	lines.append(' *')
	lines.append(' * @email forman@anyks.com')
	lines.append(' * @site https://anyks.com')
	lines.append(' *')
	lines.append(' * @brief Таблицы базы данных символов Юникода')
	lines.append(' *')
	lines.append(' * @warning Файл порождён средством «tools/encoding/unicode/generate.py» по файлам базы')
	lines.append(' *          данных символов Юникода. Правки, внесённые в файл вручную,')
	lines.append(' *          теряются при очередном порождении.')
	lines.append(' *')
	lines.append(' * @copyright Copyright © 2026')
	lines.append(' *')
	lines.append(' */')
	lines.append('')
	lines.append('/**')
	lines.append(' * Подключаем заголовочные файлы проекта')
	lines.append(' */')
	lines.append('#include <encoding/unicode/unicode.hpp>')
	lines.append('')
	lines.append('/**')
	lines.append(' * Используем стандартное пространство имён')
	lines.append(' */')
	lines.append('using namespace std;')
	lines.append('using namespace awh;')
	lines.append('')
	for name in ('CATEGORIES', 'SCRIPTS', 'EXTENSIONS', 'BIDIRECTIONAL', 'BINARIES', 'CLUSTERS', 'INDIC'):
		lines += table(name, tables[name])
	# Наборы расширений письменностей
	lines += ['/**', ' * @brief Наборы письменностей расширений, завершаемые признаком конца', ' *', ' */']
	lines.append('const uint16_t awh::unicode::EXTENSION_SETS[] = {')
	offsets = []
	flat = []
	for members in sets:
		offsets.append(len(flat))
		flat += list(members) + [0xFFFF]
	lines.append('\t' + ', '.join('0x%X' % value for value in flat))
	lines.append('};')
	lines += ['/**', ' * @brief Смещения наборов письменностей расширений', ' *', ' */']
	lines.append('const uint32_t awh::unicode::EXTENSION_OFFSETS[] = {')
	lines.append('\t' + ', '.join(str(value) for value in offsets))
	lines.append('};')
	# Размещение диапазонов двоичных свойств
	lines += ['/**', ' * @brief Размещение диапазонов двоичных свойств: смещение и количество', ' *', ' */']
	lines.append('const uint32_t awh::unicode::BINARY_SPANS[] = {')
	lines.append('\t' + ', '.join('%d, %d' % span for span in spans))
	lines.append('};')
	lines += ['/**', ' * @brief Количество двоичных свойств', ' *', ' */']
	lines.append('const size_t awh::unicode::BINARY_COUNT = %d;' % len(order))
	# Соответствие имён свойств их идентификаторам
	lines += ['/**', ' * @brief Соответствие имён свойств их идентификаторам', ' *', ' */']
	lines.append('const awh::unicode::naming_t awh::unicode::NAMES[] = {')
	for name in sorted(names):
		lines.append('\t{"%s", 0x%X},' % (name, names[name]))
	lines.append('};')
	lines += ['/**', ' * @brief Количество имён свойств', ' *', ' */']
	lines.append('const size_t awh::unicode::NAMES_COUNT = %d;' % len(names))
	lines += ['/**', ' * @brief Номер письменности неназначенных символов', ' *', ' */']
	lines.append('const size_t awh::unicode::SCRIPTS_UNKNOWN = %d;' % (
		max(item[2] for item in tables['SCRIPTS']) + 1))
	return '\n'.join(lines) + '\n'


def casing():
	"""Выпуск таблиц приведения регистра символов"""
	simple = folding()
	items = []
	for code in sorted(simple):
		items.append((code, code, simple[code] - code))
	merged = []
	for begin, end, delta in items:
		if merged and (merged[-1][2] == delta) and (merged[-1][1] + 1 == begin):
			merged[-1][1] = end
		else:
			merged.append([begin, end, delta])
	sets = orbits(simple)
	flat = []
	index = []
	for value in sorted(sets):
		members = sets[value]
		for member in members:
			index.append((member, member, len(flat)))
		flat += members + [0]
	index = sorted(index)
	lines = ['/**', ' * @brief Таблица простого приведения регистра: смещение приведённого значения', ' *', ' */']
	lines.append('const awh::unicode::folding_t awh::unicode::FOLDING[] = {')
	for begin, end, delta in merged:
		lines.append('\t{0x%X, 0x%X, %d},' % (begin, end, delta))
	lines.append('};')
	lines += ['/**', ' * @brief Количество диапазонов таблицы приведения регистра', ' *', ' */']
	lines.append('const size_t awh::unicode::FOLDING_COUNT = %d;' % len(merged))
	lines += ['/**', ' * @brief Размещение наборов символов, приводимых к одному значению', ' *', ' */']
	lines.append('const awh::unicode::interval_t awh::unicode::ORBITS[] = {')
	for begin, end, offset in index:
		lines.append('\t{0x%X, 0x%X, %d},' % (begin, end, offset))
	lines.append('};')
	lines += ['/**', ' * @brief Количество размещений наборов приведения регистра', ' *', ' */']
	lines.append('const size_t awh::unicode::ORBITS_COUNT = %d;' % len(index))
	lines += ['/**', ' * @brief Наборы символов, приводимых к одному значению, завершаемые нулём', ' *', ' */']
	lines.append('const uint32_t awh::unicode::ORBIT_SETS[] = {')
	lines.append('\t' + ', '.join('0x%X' % value for value in flat))
	lines.append('};')
	return '\n'.join(lines) + '\n'


# Путь к файлу выведенных свойств нормализации эталонной реализации приведения
# доменных имён, откуда берётся свойство исключения из сочетания
EXCLUSIONS = os.path.join(
	os.path.dirname(os.path.abspath(__file__)),
	'..', '..', 'submodules', 'libidn2', 'lib', 'DerivedNormalizationProps.txt'
)


def unicodedata():
	"""Разбор основного файла базы данных символов Юникода

	Выводятся канонические классы сочетания и разложения символов: каноническое
	разложение записывается набором кодовых значений, разложение совместимости —
	тем же набором, предваряемым обозначением вида разложения в угловых скобках.
	"""
	combining = []
	decompositions = []
	for line in open(os.path.join(TABLES, 'UnicodeData.txt'), encoding='utf-8'):
		fields = line.split(';')
		if len(fields) < 6:
			continue
		code = int(fields[0], 16)
		# Канонический класс сочетания символа записан третьим полем
		combining.append((code, code, int(fields[3])))
		value = fields[5].strip()
		if not value:
			continue
		compat = value.startswith('<')
		if compat:
			value = value[value.index('>') + 1:]
		decompositions.append((code, tuple(int(item, 16) for item in value.split()), compat))
	return compress(combining), decompositions


def excluded():
	"""Набор символов, исключённых из канонического сочетания

	Свойство берётся файлом выведенных свойств нормализации: часть исключений
	выводится из разложений самого файла базы данных, а часть задана списком
	исключений, из его состава не выводимым.
	"""
	result = set()
	for line in open(EXCLUSIONS, encoding='utf-8'):
		line = line.split('#')[0].strip()
		if not line:
			continue
		fields = [field.strip() for field in line.split(';')]
		if (len(fields) < 2) or (fields[1] != 'Full_Composition_Exclusion'):
			continue
		bounds = fields[0].split('..')
		begin = int(bounds[0], 16)
		end = int(bounds[1], 16) if len(bounds) > 1 else begin
		for code in range(begin, end + 1):
			result.add(code)
	return result


def normalization():
	"""Таблицы нормализации текста

	Порождаются канонические классы сочетания, разложения символов и обратные им
	сочетания пар символов. Слоги хангыля разлагаются и сочетаются вычислением
	и в таблицы не входят.
	"""
	combining, decompositions = unicodedata()
	classes = {}
	for begin, end, value in combining:
		for code in range(begin, end + 1):
			classes[code] = value
	skip = excluded()
	# Размещаем разложения в общем наборе кодовых значений
	pool = []
	records = []
	for code, value, compat in sorted(decompositions):
		records.append((code, len(pool), len(value), compat))
		pool.extend(value)
	# Выводим сочетания обращением канонических разложений
	compositions = []
	for code, value, compat in sorted(decompositions):
		# Разложения совместимости сочетанию не подлежат
		if compat or (len(value) != 2):
			continue
		# Исключённые символы и разложения, начинающиеся не с начального символа,
		# сочетанию не подлежат: их сочетание нарушило бы устойчивость нормализации
		if (code in skip) or (classes.get(value[0], 0) != 0):
			continue
		compositions.append((value[0], value[1], code))
	return combining, records, pool, sorted(compositions)


def normalize():
	"""Выпуск таблиц нормализации текста"""
	combining, records, pool, compositions = normalization()
	lines = []
	lines.append('')
	lines.append('/**')
	lines.append(' * @brief Таблица канонических классов сочетания символов')
	lines.append(' *')
	lines.append(' */')
	lines.append('const awh::unicode::interval_t awh::unicode::COMBINING[] = {')
	for begin, end, value in combining:
		lines.append('\t{0x%X, 0x%X, %s},' % (begin, end, value))
	lines[-1] = lines[-1].rstrip(',')
	lines.append('};')
	lines.append('const size_t awh::unicode::COMBINING_COUNT = %d;' % len(combining))
	lines.append('')
	lines.append('/**')
	lines.append(' * @brief Таблица разложений символов')
	lines.append(' *')
	lines.append(' */')
	lines.append('const awh::unicode::decomposition_t awh::unicode::DECOMPOSITIONS[] = {')
	for code, offset, length, compat in records:
		lines.append('\t{0x%X, %d, %d, %s},' % (code, offset, length, ('true' if compat else 'false')))
	lines[-1] = lines[-1].rstrip(',')
	lines.append('};')
	lines.append('const size_t awh::unicode::DECOMPOSITIONS_COUNT = %d;' % len(records))
	lines.append('')
	lines.append('/**')
	lines.append(' * @brief Набор кодовых значений разложений символов')
	lines.append(' *')
	lines.append(' */')
	lines.append('const uint32_t awh::unicode::DECOMPOSITION_SETS[] = {')
	for row in range(0, len(pool), 12):
		lines.append('\t' + ', '.join('0x%X' % value for value in pool[row:row + 12]) + ',')
	lines[-1] = lines[-1].rstrip(',')
	lines.append('};')
	lines.append('')
	lines.append('/**')
	lines.append(' * @brief Таблица канонических сочетаний пар символов')
	lines.append(' *')
	lines.append(' */')
	lines.append('const awh::unicode::composition_t awh::unicode::COMPOSITIONS[] = {')
	for first, second, code in compositions:
		lines.append('\t{0x%X, 0x%X, 0x%X},' % (first, second, code))
	lines[-1] = lines[-1].rstrip(',')
	lines.append('};')
	lines.append('const size_t awh::unicode::COMPOSITIONS_COUNT = %d;' % len(compositions))
	return '\n'.join(lines) + '\n'


def main():
	"""Порождение таблиц свойств Юникода"""
	if not os.path.isdir(TABLES):
		sys.stderr.write('Каталог таблиц Юникода отсутствует: %s\n' % TABLES)
		return 1
	if not os.path.isfile(EXCLUSIONS):
		sys.stderr.write('Файл выведенных свойств нормализации отсутствует: %s\n' % EXCLUSIONS)
		return 1
	tables, sets, spans, names, order = build()
	source = emit(tables, sets, spans, names, order) + casing() + normalize()
	path = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'src', 'unicode', 'table.cpp')
	open(path, 'w', encoding='utf-8').write(source)
	sys.stdout.write('выпущен файл: %s\n' % os.path.normpath(path))
	sys.stdout.write('имён свойств: %d, письменностей: %d, двоичных свойств: %d\n' % (
		len(names), len(set(item[2] for item in tables['SCRIPTS'])) + 1, len(order)))
	sys.stdout.write('диапазонов: категорий %d, письменностей %d, расширений %d, двунаправленности %d, двоичных %d\n' % (
		len(tables['CATEGORIES']), len(tables['SCRIPTS']), len(tables['EXTENSIONS']),
		len(tables['BIDIRECTIONAL']), len(tables['BINARIES'])))
	return 0


if __name__ == '__main__':
	sys.exit(main())
