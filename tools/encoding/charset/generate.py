#!/usr/bin/env python3
"""
@file generate.py
@date 2026-08-03

@license{LicenseRef-AWH-1.0}

@author Yuriy Lobarev

@telegram{forman}
@phone{+7 (910) 983-95-90}

@email forman@anyks.com
@site https://anyks.com

@brief Порождение таблиц соответствия однобайтовых кодировок Юникоду по файлам
       отображений эталонной реализации GNU libiconv. Порождаются прямые таблицы
       соответствия байтов кодировки кодовым значениям Юникода, обратные таблицы
       соответствия кодовых значений байтам и таблица имён, которыми кодировки
       обозначаются в заголовках сетевых протоколов.

@copyright Copyright © 2026
"""

import os
import sys

# Путь к каталогу файлов отображений эталонной реализации
TABLES = os.path.join(
	os.path.dirname(os.path.abspath(__file__)),
	'..', '..', 'submodules', 'libiconv', 'tests'
)

# Признак отсутствия соответствия символа кодировке
UNMAPPED = 0xFFFF

# Набор порождаемых кодировок
#
# Состав набора задан стандартом кодировок консорциума WHATWG, перечисляющим
# однобайтовые кодировки, обязательные к поддержке в сети, и дополнен кодировкой
# ISO-8859-1, задаваемой отдельным стандартом. Каждая запись набора состоит из
# обозначения кодировки, имени файла отображения, канонического имени кодировки
# и набора имён, которыми кодировка обозначается отправителем.
#
# Имена приводятся к нормальному виду порождателем и хранятся в таблице
# отсортированными: разбор имени выполняется двоичным поиском.
CHARSETS = (
	('ISO8859_1', 'ISO-8859-1', 'iso-8859-1', ()),
	('ISO8859_2', 'ISO-8859-2', 'iso-8859-2', (
		'iso-8859-2', 'iso8859-2', 'iso88592', 'iso_8859-2', 'iso_8859-2:1987',
		'iso-ir-101', 'l2', 'latin2', 'csisolatin2')),
	('ISO8859_3', 'ISO-8859-3', 'iso-8859-3', (
		'iso-8859-3', 'iso8859-3', 'iso88593', 'iso_8859-3', 'iso_8859-3:1988',
		'iso-ir-109', 'l3', 'latin3', 'csisolatin3')),
	('ISO8859_4', 'ISO-8859-4', 'iso-8859-4', (
		'iso-8859-4', 'iso8859-4', 'iso88594', 'iso_8859-4', 'iso_8859-4:1988',
		'iso-ir-110', 'l4', 'latin4', 'csisolatin4')),
	('ISO8859_5', 'ISO-8859-5', 'iso-8859-5', (
		'iso-8859-5', 'iso8859-5', 'iso88595', 'iso_8859-5', 'iso_8859-5:1988',
		'iso-ir-144', 'cyrillic', 'csisolatincyrillic')),
	('ISO8859_6', 'ISO-8859-6', 'iso-8859-6', (
		'iso-8859-6', 'iso8859-6', 'iso88596', 'iso_8859-6', 'iso_8859-6:1987',
		'iso-ir-127', 'arabic', 'asmo-708', 'ecma-114', 'csiso88596e',
		'csiso88596i', 'iso-8859-6-e', 'iso-8859-6-i', 'csisolatinarabic')),
	('ISO8859_7', 'ISO-8859-7', 'iso-8859-7', (
		'iso-8859-7', 'iso8859-7', 'iso88597', 'iso_8859-7', 'iso_8859-7:1987',
		'iso-ir-126', 'greek', 'greek8', 'ecma-118', 'elot_928', 'sun_eu_greek',
		'csisolatingreek')),
	('ISO8859_8', 'ISO-8859-8', 'iso-8859-8', (
		'iso-8859-8', 'iso8859-8', 'iso88598', 'iso_8859-8', 'iso_8859-8:1988',
		'iso-ir-138', 'hebrew', 'visual', 'iso-8859-8-e', 'csiso88598e',
		'csisolatinhebrew')),
	('ISO8859_9', 'ISO-8859-9', 'iso-8859-9', ()),
	('ISO8859_10', 'ISO-8859-10', 'iso-8859-10', (
		'iso-8859-10', 'iso8859-10', 'iso885910', 'iso-ir-157', 'l6', 'latin6',
		'csisolatin6')),
	('ISO8859_11', 'ISO-8859-11', 'iso-8859-11', ()),
	('ISO8859_13', 'ISO-8859-13', 'iso-8859-13', (
		'iso-8859-13', 'iso8859-13', 'iso885913')),
	('ISO8859_14', 'ISO-8859-14', 'iso-8859-14', (
		'iso-8859-14', 'iso8859-14', 'iso885914')),
	('ISO8859_15', 'ISO-8859-15', 'iso-8859-15', (
		'iso-8859-15', 'iso8859-15', 'iso885915', 'iso_8859-15', 'l9', 'latin9',
		'csisolatin9')),
	('ISO8859_16', 'ISO-8859-16', 'iso-8859-16', ('iso-8859-16',)),
	('CP866', 'CP866', 'IBM866', ('ibm866', 'cp866', '866', 'csibm866')),
	('CP874', 'CP874', 'windows-874', (
		'windows-874', 'cp874', 'dos-874', 'iso-8859-11', 'iso8859-11',
		'iso885911', 'tis-620')),
	('CP1250', 'CP1250', 'windows-1250', (
		'windows-1250', 'cp1250', 'x-cp1250')),
	('CP1251', 'CP1251', 'windows-1251', (
		'windows-1251', 'cp1251', 'x-cp1251')),
	('CP1252', 'CP1252', 'windows-1252', (
		'windows-1252', 'cp1252', 'x-cp1252', 'ansi_x3.4-1968', 'ascii',
		'us-ascii', 'iso-8859-1', 'iso8859-1', 'iso88591', 'iso_8859-1',
		'iso_8859-1:1987', 'iso-ir-100', 'l1', 'latin1', 'cp819', 'ibm819',
		'csisolatin1')),
	('CP1253', 'CP1253', 'windows-1253', (
		'windows-1253', 'cp1253', 'x-cp1253')),
	('CP1254', 'CP1254', 'windows-1254', (
		'windows-1254', 'cp1254', 'x-cp1254', 'iso-8859-9', 'iso8859-9',
		'iso88599', 'iso_8859-9', 'iso_8859-9:1989', 'iso-ir-148', 'l5',
		'latin5', 'csisolatin5')),
	('CP1255', 'CP1255', 'windows-1255', (
		'windows-1255', 'cp1255', 'x-cp1255', 'iso-8859-8-i', 'logical',
		'csiso88598i')),
	('CP1256', 'CP1256', 'windows-1256', (
		'windows-1256', 'cp1256', 'x-cp1256')),
	('CP1257', 'CP1257', 'windows-1257', (
		'windows-1257', 'cp1257', 'x-cp1257')),
	('CP1258', 'CP1258', 'windows-1258', (
		'windows-1258', 'cp1258', 'x-cp1258')),
	('KOI8_R', 'KOI8-R', 'KOI8-R', ('koi8-r', 'koi8r', 'koi', 'koi8', 'cskoi8r')),
	('KOI8_U', 'KOI8-U', 'KOI8-U', ('koi8-u', 'koi8-ru')),
	('MAC_ROMAN', 'MacRoman', 'macintosh', (
		'macintosh', 'mac', 'csmacintosh', 'x-mac-roman')),
	('MAC_CYRILLIC', 'MacCyrillic', 'x-mac-cyrillic', (
		'x-mac-cyrillic', 'x-mac-ukrainian', 'mac-cyrillic', 'maccyrillic')),
)

# Имена кодировок, не задаваемых таблицей соответствия
BUILTIN = (
	('UTF8', 'UTF-8', ('utf-8', 'utf8', 'unicode-1-1-utf-8', 'unicode11utf8',
	 'unicode20utf8', 'x-unicode20utf8')),
)


def normal(name):
	"""Приведение имени кодировки к нормальному виду"""
	return ''.join(letter.lower() for letter in name.strip() if not letter.isspace())


def mapping(name):
	"""Разбор файла отображения кодировки на соответствие байтов кодовым значениям"""
	path = os.path.join(TABLES, '%s.TXT' % name)
	table = [UNMAPPED] * 256
	for line in open(path, encoding='utf-8'):
		line = line.split('#')[0].strip()
		if not line:
			continue
		fields = line.split()
		if len(fields) < 2:
			continue
		byte = int(fields[0], 16)
		code = int(fields[1], 16)
		# Кодовые значения за пределами базовой плоскости однобайтовым кодировкам
		# не встречаются: таблица хранит кодовые значения двухбайтовыми
		if code > 0xFFFF:
			sys.stderr.write('кодировка %s: значение U+%04X вне базовой плоскости\n' % (name, code))
			sys.exit(1)
		table[byte] = code
	return table


def reverse(table):
	"""Обратное соответствие кодовых значений Юникода байтам кодировки"""
	result = {}
	for byte, code in enumerate(table):
		if code == UNMAPPED:
			continue
		# Кодовому значению соответствует наименьший из байтов, его записывающих
		if code not in result:
			result[code] = byte
	return sorted(result.items())


def header():
	"""Заголовок порождаемого файла таблиц"""
	lines = []
	lines.append('/**')
	lines.append(' * @file table.cpp')
	lines.append(' * @date 2026-08-03')
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
	lines.append(' * @brief Таблицы соответствия однобайтовых кодировок Юникоду')
	lines.append(' *')
	lines.append(' * @warning Файл порождён средством «tools/encoding/charset/generate.py» по файлам')
	lines.append(' *          отображений эталонной реализации GNU libiconv. Правки, внесённые')
	lines.append(' *          в файл вручную, теряются при очередном порождении.')
	lines.append(' *')
	lines.append(' * @copyright Copyright © 2026')
	lines.append(' *')
	lines.append(' */')
	lines.append('')
	lines.append('/**')
	lines.append(' * Подключаем заголовочные файлы проекта')
	lines.append(' */')
	lines.append('#include <encoding/charset/charset.hpp>')
	lines.append('')
	lines.append('/**')
	lines.append(' * Используем стандартное пространство имён')
	lines.append(' */')
	lines.append('using namespace std;')
	lines.append('using namespace awh;')
	lines.append('')
	return lines


def emit():
	"""Выпуск исходного файла таблиц соответствия кодировок"""
	lines = header()
	labels = []
	for name, source, canonical, names in CHARSETS:
		table = mapping(source)
		pairs = reverse(table)
		lines.append('/**')
		lines.append(' * @brief Соответствие байтов кодировки %s кодовым значениям Юникода' % canonical)
		lines.append(' *')
		lines.append(' */')
		lines.append('static const uint16_t %s_UNICODE[256] = {' % name)
		for row in range(0, 256, 8):
			values = ', '.join('0x%04X' % code for code in table[row:row + 8])
			lines.append('\t%s,' % values)
		lines[-1] = lines[-1].rstrip(',')
		lines.append('};')
		lines.append('')
		lines.append('/**')
		lines.append(' * @brief Соответствие кодовых значений Юникода байтам кодировки %s' % canonical)
		lines.append(' *')
		lines.append(' */')
		lines.append('static const awh::charset::mapping_t %s_MAPPINGS[] = {' % name)
		for row in range(0, len(pairs), 4):
			values = ', '.join('{0x%04X, 0x%02X}' % pair for pair in pairs[row:row + 4])
			lines.append('\t%s,' % values)
		lines[-1] = lines[-1].rstrip(',')
		lines.append('};')
		lines.append('')
		for label in names:
			labels.append((normal(label), name))
	# Выпускаем набор таблиц кодировок
	lines.append('/**')
	lines.append(' * @brief Набор таблиц соответствия однобайтовых кодировок Юникоду')
	lines.append(' *')
	lines.append(' */')
	lines.append('const awh::charset::table_t awh::charset::CODEPAGES[] = {')
	for name, source, canonical, names in CHARSETS:
		lines.append('\t{encoding_t::%s, "%s", %s_UNICODE, %s_MAPPINGS, (sizeof(%s_MAPPINGS) / sizeof(%s_MAPPINGS[0]))},' % (
			name, canonical, name, name, name, name))
	lines[-1] = lines[-1].rstrip(',')
	lines.append('};')
	lines.append('')
	lines.append('/**')
	lines.append(' * @brief Количество таблиц соответствия однобайтовых кодировок')
	lines.append(' *')
	lines.append(' */')
	lines.append('const size_t awh::charset::CODEPAGES_COUNT = %d;' % len(CHARSETS))
	lines.append('')
	# Выпускаем таблицу имён кодировок
	for name, canonical, names in BUILTIN:
		for label in names:
			labels.append((normal(label), name))
	table = {}
	for label, name in labels:
		if label in table and table[label] != name:
			sys.stderr.write('имя «%s» задано кодировкам %s и %s\n' % (label, table[label], name))
			sys.exit(1)
		table[label] = name
	lines.append('/**')
	lines.append(' * @brief Соответствие имён кодировок их обозначениям')
	lines.append(' *')
	lines.append(' */')
	lines.append('const awh::charset::labeling_t awh::charset::LABELS[] = {')
	for label in sorted(table):
		lines.append('\t{"%s", encoding_t::%s},' % (label, table[label]))
	lines[-1] = lines[-1].rstrip(',')
	lines.append('};')
	lines.append('')
	lines.append('/**')
	lines.append(' * @brief Количество имён кодировок таблицы соответствия')
	lines.append(' *')
	lines.append(' */')
	lines.append('const size_t awh::charset::LABELS_COUNT = %d;' % len(table))
	lines.append('')
	return '\n'.join(lines)


def main():
	"""Порождение файла таблиц соответствия кодировок"""
	if not os.path.isdir(TABLES):
		sys.stderr.write('Каталог файлов отображений отсутствует: %s\n' % TABLES)
		return 1
	source = emit()
	path = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'src', 'charset', 'table.cpp')
	open(path, 'w', encoding='utf-8').write(source)
	sys.stdout.write('выпущен файл: %s\n' % os.path.normpath(path))
	sys.stdout.write('кодировок: %d\n' % len(CHARSETS))
	return 0


if __name__ == '__main__':
	sys.exit(main())
