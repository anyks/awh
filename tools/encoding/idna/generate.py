#!/usr/bin/env python3
"""
@file: generate.py
@date: 2026-08-03
@license: LicenseRef-AWH-1.0

@telegram: @forman
@author: Yuriy Lobarev
@phone: +7 (910) 983-95-90
@email: forman@anyks.com
@site: https://anyks.com

@brief Порождение таблиц приведения доменных имён по таблице преобразований
       приложения по обработке доменных имён стандарта Юникода и по свойству
       вида соединения символов, выгруженному из состава эталонной реализации.

@copyright: Copyright © 2026
"""

import os
import sys

# Путь к таблице преобразований в составе подмодуля эталонной реализации
MAPPING = os.path.join(
	os.path.dirname(os.path.abspath(__file__)),
	'..', '..', 'submodules', 'libidn2', 'lib', 'IdnaMappingTable.txt'
)

# Путь к файлу свойства вида соединения символов
JOINING = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'joining.txt')

# Соответствие состояний символов их обозначениям
STATUSES = {
	'valid': 'VALID',
	'ignored': 'IGNORED',
	'mapped': 'MAPPED',
	'deviation': 'DEVIATION',
	'disallowed': 'DISALLOWED',
	'disallowed_STD3_valid': 'DISALLOWED_STD3_VALID',
	'disallowed_STD3_mapped': 'DISALLOWED_STD3_MAPPED',
}

# Соответствие видов соединения их обозначениям
JOININGS = {
	'T': 'TRANSPARENT',
	'C': 'CAUSING',
	'L': 'LEFT',
	'R': 'RIGHT',
	'D': 'DUAL',
}


def mappings():
	"""Разбор таблицы преобразований символов

	Записи таблицы приводятся к набору диапазонов кодовых значений с общим
	состоянием и общим преобразованием. Преобразования размещаются в едином
	наборе кодовых значений, а записи ссылаются на него смещением.
	"""
	pool = []
	records = []
	for line in open(MAPPING, encoding='utf-8'):
		line = line.split('#')[0].strip()
		if not line:
			continue
		fields = [field.strip() for field in line.split(';')]
		bounds = fields[0].split('..')
		begin = int(bounds[0], 16)
		end = int(bounds[1], 16) if len(bounds) > 1 else begin
		status = fields[1]
		if status not in STATUSES:
			sys.stderr.write('неизвестное состояние символа: %s\n' % status)
			sys.exit(1)
		value = ()
		# Преобразование символа записано третьим полем и задано не всем состояниям
		if (len(fields) > 2) and fields[2]:
			value = tuple(int(item, 16) for item in fields[2].split())
		records.append((begin, end, STATUSES[status], value))
	# Сливаем соседние диапазоны с общим состоянием и пустым преобразованием
	merged = []
	for begin, end, status, value in sorted(records):
		if merged and (not value) and (not merged[-1][3]) and \
		 (merged[-1][2] == status) and (merged[-1][1] + 1 == begin):
			merged[-1][1] = end
		else:
			merged.append([begin, end, status, value])
	# Размещаем преобразования в общем наборе кодовых значений
	result = []
	for begin, end, status, value in merged:
		offset = 0
		if value:
			# Одинаковые преобразования размещаются в наборе однажды
			key = list(value)
			for i in range(0, len(pool) - len(key) + 1):
				if pool[i:i + len(key)] == key:
					offset = i
					break
			else:
				offset = len(pool)
				pool.extend(key)
		result.append((begin, end, offset, len(value), status))
	return result, pool


def joinings():
	"""Разбор свойства вида соединения символов"""
	result = []
	for line in open(JOINING, encoding='utf-8'):
		line = line.split('#')[0].strip()
		if not line:
			continue
		fields = [field.strip() for field in line.split(';')]
		bounds = fields[0].split('..')
		begin = int(bounds[0], 16)
		end = int(bounds[1], 16) if len(bounds) > 1 else begin
		if fields[1] not in JOININGS:
			sys.stderr.write('неизвестный вид соединения: %s\n' % fields[1])
			sys.exit(1)
		result.append((begin, end, JOININGS[fields[1]]))
	return sorted(result)


def emit():
	"""Выпуск исходного файла таблиц приведения доменных имён"""
	records, pool = mappings()
	joins = joinings()
	lines = []
	lines.append('/**')
	lines.append(' * @file: table.cpp')
	lines.append(' * @date: 2026-08-03')
	lines.append(' * @license: LicenseRef-AWH-1.0')
	lines.append(' *')
	lines.append(' * @telegram: @forman')
	lines.append(' * @author: Yuriy Lobarev')
	lines.append(' * @phone: +7 (910) 983-95-90')
	lines.append(' * @email: forman@anyks.com')
	lines.append(' * @site: https://anyks.com')
	lines.append(' *')
	lines.append(' * @brief Таблицы приведения доменных имён')
	lines.append(' *')
	lines.append(' * @warning Файл порождён средством «tools/encoding/idna/generate.py» по таблице')
	lines.append(' *          преобразований стандарта Юникода. Правки, внесённые в файл')
	lines.append(' *          вручную, теряются при очередном порождении.')
	lines.append(' *')
	lines.append(' * @copyright: Copyright © 2026')
	lines.append(' *')
	lines.append(' */')
	lines.append('')
	lines.append('/**')
	lines.append(' * Подключаем заголовочные файлы проекта')
	lines.append(' */')
	lines.append('#include <encoding/idna/table.hpp>')
	lines.append('')
	lines.append('/**')
	lines.append(' * Используем стандартное пространство имён')
	lines.append(' */')
	lines.append('using namespace std;')
	lines.append('using namespace awh;')
	lines.append('')
	lines.append('/**')
	lines.append(' * @brief Таблица преобразований символов доменных имён')
	lines.append(' *')
	lines.append(' */')
	lines.append('const awh::idna::mapping_t awh::idna::MAPPINGS[] = {')
	for begin, end, offset, length, status in records:
		lines.append('\t{0x%X, 0x%X, %d, %d, status_t::%s},' % (begin, end, offset, length, status))
	lines[-1] = lines[-1].rstrip(',')
	lines.append('};')
	lines.append('const size_t awh::idna::MAPPINGS_COUNT = %d;' % len(records))
	lines.append('')
	lines.append('/**')
	lines.append(' * @brief Набор кодовых значений преобразований символов')
	lines.append(' *')
	lines.append(' */')
	lines.append('const uint32_t awh::idna::MAPPING_SETS[] = {')
	for row in range(0, len(pool), 12):
		lines.append('\t' + ', '.join('0x%X' % value for value in pool[row:row + 12]) + ',')
	lines[-1] = lines[-1].rstrip(',')
	lines.append('};')
	lines.append('')
	lines.append('/**')
	lines.append(' * @brief Таблица видов соединения символов')
	lines.append(' *')
	lines.append(' */')
	lines.append('const awh::idna::joining_range_t awh::idna::JOININGS[] = {')
	for begin, end, value in joins:
		lines.append('\t{0x%X, 0x%X, joining_t::%s},' % (begin, end, value))
	lines[-1] = lines[-1].rstrip(',')
	lines.append('};')
	lines.append('const size_t awh::idna::JOININGS_COUNT = %d;' % len(joins))
	lines.append('')
	return '\n'.join(lines), len(records), len(pool), len(joins)


def main():
	"""Порождение файла таблиц приведения доменных имён"""
	if not os.path.isfile(MAPPING):
		sys.stderr.write('Таблица преобразований отсутствует: %s\n' % MAPPING)
		return 1
	if not os.path.isfile(JOINING):
		sys.stderr.write('Файл видов соединения отсутствует: %s\n' % JOINING)
		return 1
	source, records, pool, joins = emit()
	path = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'src', 'idna', 'table.cpp')
	open(path, 'w', encoding='utf-8').write(source)
	sys.stdout.write('выпущен файл: %s\n' % os.path.normpath(path))
	sys.stdout.write('диапазонов преобразований: %d, кодовых значений: %d, видов соединения: %d\n' % (
		records, pool, joins))
	return 0


if __name__ == '__main__':
	sys.exit(main())
