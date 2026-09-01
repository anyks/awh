# -*- coding: utf-8 -*-
##
# @file: awh-gdb.py
# @brief Описатели значений AWH для отладчика GDB
#
# @details Тот же вывод, что у `awh.py`, но под GDB: у стендов Linux, Solaris и
#          OpenIndiana отладчик один - LLDB там либо нет вовсе, либо он не понимает
#          вывода GCC. Показывает ВИД узла и его содержимое там, где сама ссылка несёт
#          лишь номер узла да клеймо документа
#
# @note Подключается вручную, дабы не спорить с настройками стенда:
#           (gdb) source tools/debug/awh-gdb.py
#
# @author ANYKS
##

import gdb

# Виды значений узла документа JSON: `include/codec/json/common.hpp`
JSON_TYPES = {
	0x0000: 'UNDEFINED', 0x0001: 'null',   0x0002: 'bool',   0x0004: 'string',
	0x0008: 'array',     0x0010: 'object', 0x0020: 'int8',   0x0040: 'int16',
	0x0080: 'int32',     0x0100: 'int64',  0x0200: 'uint8',  0x0400: 'uint16',
	0x0800: 'uint32',    0x1000: 'uint64', 0x2000: 'float',  0x4000: 'double',
	0x8000: 'extended',
}

# Виды значений узла документа YAML: `include/codec/yaml/common.hpp`
YAML_TYPES = {
	0x00000000: 'UNDEFINED', 0x00000001: 'null',    0x00000002: 'bool',   0x00000004: 'string',
	0x00000008: 'sequence',  0x00000010: 'mapping', 0x00000020: 'int8',   0x00000040: 'int16',
	0x00000080: 'int32',     0x00000100: 'int64',   0x00000200: 'uint8',  0x00000400: 'uint16',
	0x00000800: 'uint32',    0x00001000: 'uint64',  0x00002000: 'float',  0x00004000: 'double',
	0x00008000: 'extended',  0x00010000: 'binary',  0x00020000: 'stamp',
}

# Виды узлов дерева разметки XML: `include/codec/xml/document.hpp`
XML_KINDS = {
	0x00: 'NONE',  0x01: 'document', 0x02: 'element',    0x03: 'text',
	0x04: 'cdata', 0x05: 'comment',  0x06: 'processing', 0x07: 'doctype',
	0x08: 'space',
}

def field(value, *names):
	##
	# Читает поле по цепочке имён, отвечая None на первом же отсутствующем
	##
	for name in names:
		if value is None:
			return None
		try:
			value = value[name]
		except Exception:
			return None
	return value

def whole(value, default = 0):
	##
	# Обращает поле в целое, снося оболочку перечисления
	##
	if value is None:
		return default
	try:
		return int(value)
	except Exception:
		##
		# Перечисление у GDB целым не берётся напрямую
		#
		# Приводим его к беззнаковому целому: имена видов нам не нужны, нужны их числа
		##
		try:
			return int(value.cast(gdb.lookup_type('unsigned long')))
		except Exception:
			return default

def storage_slice(storage, offset, length):
	##
	# Выводит кусок хранилища знаков по смещению и длине
	#
	# Читаем ПАМЯТЬ по адресу данных: хранилище одно на весь документ, и целиком оно
	# показало бы содержимое всех узлов разом
	##
	if (storage is None) or (length == 0):
		return ''
	if length > 4096:
		length = 4096
	data, held = string_parts(storage)
	if data is None:
		return None
	if offset >= held:
		return None
	try:
		raw = gdb.selected_inferior().read_memory((data + offset), min(length, (held - offset))).tobytes()
	except Exception:
		return None
	return raw.decode('utf-8', errors = 'replace')

def span_text(storage, span):
	##
	# Выводит отрезок хранилища знаков, заданный смещением да длиной
	##
	if span is None:
		return None
	return storage_slice(storage, whole(field(span, 'offset')), whole(field(span, 'length')))

class JsonValue(object):
	##
	# Описатель ссылки на узел документа JSON
	##
	def __init__(self, value):
		self.value = value
	def to_string(self):
		doc = field(self.value, '_doc')
		if (doc is None) or (whole(doc) == 0):
			return 'недействительная ссылка'
		index = whole(field(self.value, '_index'))
		stamp = whole(field(self.value, '_stamp'))
		doc = doc.dereference()
		current = field(doc, '_stamp')
		if (current is not None) and (whole(current) != stamp):
			return 'ссылка устарела: документ перестроен'
		node = node_at(field(doc, '_nodes'), index)
		if node is None:
			return 'номер узла %d вне перечня' % index
		kind = whole(field(node, 'type'))
		name = JSON_TYPES.get(kind, 'вид 0x%04x' % kind)
		content = field(node, 'content')
		length = whole(content[0]) if (content is not None) else 0
		if name in ('array', 'object'):
			return '%s, детей: %d' % (name, length)
		if name == 'string':
			text = storage_slice(field(doc, '_storage'), whole(field(node, 'offset')), length)
			if text is not None:
				return '"%s"' % text
			return '%s, длина %d' % (name, length)
		if name in ('null', 'UNDEFINED'):
			return name
		if name == 'bool':
			return 'true' if (length != 0) else 'false'
		number = node_number(content, name)
		return str(number) if (number is not None) else ('%s, узел %d' % (name, index))

class YamlValue(object):
	##
	# Описатель ссылки на узел документа YAML
	#
	# Узел YAML шире узла JSON: запись значения удерживается всегда, а `offset` указывает
	# на ИМЯ пары, за которым следом идёт сама запись
	##
	def __init__(self, value):
		self.value = value
	def to_string(self):
		doc = field(self.value, '_doc')
		if (doc is None) or (whole(doc) == 0):
			return 'недействительная ссылка'
		index = whole(field(self.value, '_index'))
		stamp = whole(field(self.value, '_stamp'))
		doc = doc.dereference()
		# Клеймо документа у YAML зовётся `_generation`, а не `_stamp`
		current = field(doc, '_generation')
		if (current is not None) and (whole(current) != stamp):
			return 'ссылка устарела: документ перестроен'
		node = node_at(field(doc, '_nodes'), index)
		if node is None:
			return 'номер узла %d вне перечня' % index
		kind = whole(field(node, 'type'))
		name = YAML_TYPES.get(kind, 'вид 0x%08x' % kind)
		content = field(node, 'content')
		length = whole(content[0]) if (content is not None) else 0
		offset = whole(field(node, 'offset'))
		named = whole(field(node, 'named'))
		storage = field(doc, '_storage')
		title = ''
		if named > 0:
			text = storage_slice(storage, offset, named)
			if text is not None:
				title = '%s: ' % text
		if name in ('sequence', 'mapping'):
			return '%s%s, детей: %d' % (title, name, length)
		if name in ('null', 'UNDEFINED'):
			return '%s%s' % (title, name)
		# Показываем ЗАПИСЬ, а не разобранное число: `0x1F` записано так, а не числом 31
		text = storage_slice(storage, (offset + named), length)
		if text is not None:
			return '%s%s (%s)' % (title, text, name)
		return '%s%s, длина %d' % (title, name, length)

class XmlNode(object):
	##
	# Описатель узла дерева разметки XML
	##
	def __init__(self, value):
		self.value = value
	def to_string(self):
		doc = field(self.value, '_document')
		if (doc is None) or (whole(doc) == 0):
			return 'недействительный узел'
		node_id = whole(field(self.value, '_id'))
		stamp = whole(field(self.value, '_stamp'))
		doc = doc.dereference()
		current = field(doc, '_stamp')
		if (current is not None) and (whole(current) != stamp):
			return 'узел устарел: дерево перестроено'
		nodes = field(doc, '_nodes')
		record = node_at(nodes, node_id)
		if record is None:
			return 'номер узла %d вне арены' % node_id
		kind = whole(field(record, 'kind'))
		name = XML_KINDS.get(kind, 'вид 0x%02x' % kind)
		storage = field(doc, '_storage')
		title = span_text(storage, field(record, 'name', 'local'))
		prefix = span_text(storage, field(record, 'name', 'prefix'))
		if prefix and title:
			title = '%s:%s' % (prefix, title)
		if name in ('element', 'document'):
			result = ('<%s>' % title) if (name == 'element') else name
			attributes = whole(field(record, 'attributes'))
			if attributes > 0:
				result += ', свойств: %d' % attributes
			return result + (', детей: %d' % children(nodes, record))
		body = span_text(storage, field(record, 'value'))
		if body is not None:
			return '%s "%s"' % (name, body)
		return '%s, узел %d' % (name, node_id)

def vector_bounds(vector):
	##
	# Выводит начало и конец перечня, каким бы ни было имя полей
	#
	# `_M_impl._M_start` у libstdc++, `__begin_` у libc++: знать нужно оба
	##
	if vector is None:
		return (None, None)
	try:
		return (vector['_M_impl']['_M_start'], vector['_M_impl']['_M_finish'])
	except Exception:
		pass
	try:
		return (vector['__begin_'], vector['__end_'])
	except Exception:
		return (None, None)

def node_at(nodes, index):
	##
	# Выдаёт узел перечня по номеру, оградив выдачу длиною перечня
	##
	first, last = vector_bounds(nodes)
	if first is None:
		return None
	if index >= int(last - first):
		return None
	return (first + index).dereference()

def count_of(nodes):
	##
	# Выдаёт длину перечня узлов
	##
	first, last = vector_bounds(nodes)
	return int(last - first) if (first is not None) else 0

def children(nodes, record):
	##
	# Считает детей узла обходом цепочки соседей
	#
	# Счётчика детей записи не несут: держат первого да последнего ребёнка и соседей.
	# Оградою обхода служит длина арены - битая цепочка не должна вешать отладчик
	##
	limit = count_of(nodes)
	child = whole(field(record, 'first'), limit)
	result = 0
	while (child < limit) and (result <= limit):
		result += 1
		following = node_at(nodes, child)
		if following is None:
			break
		child = whole(field(following, 'next'), limit)
	return result

def node_number(content, name):
	##
	# Собирает число узла из восьми байтов содержимого
	##
	if content is None:
		return None
	value = (whole(content[0]) | (whole(content[1]) << 32))
	if name in ('uint8', 'uint16', 'uint32', 'uint64'):
		return value
	if name in ('int8', 'int16', 'int32', 'int64'):
		return (value - (1 << 64)) if (value & (1 << 63)) else value
	if name in ('float', 'double'):
		import struct
		return struct.unpack('<d', struct.pack('<Q', value))[0]
	return None

# Виды значений TOML: `include/codec/toml/common.hpp`
TOML_TYPES = {
	0x00: 'NONE',       0x01: 'string',    0x02: 'integer',        0x03: 'float',
	0x04: 'bool',       0x05: 'offset-datetime', 0x06: 'local-datetime',
	0x07: 'local-date', 0x08: 'local-time', 0x09: 'array',         0x0A: 'table',
}

# Виды значений INI: `include/codec/ini/common.hpp`
INI_TYPES = { 0x00: 'NONE', 0x01: 'string', 0x02: 'array', 0x03: 'table' }

def pointer_of(value):
	##
	# Обращает поле данных в указатель, годный для сложения со смещением
	#
	# Массив указателем сам не становится: берём адрес первой его записи
	##
	if value is None:
		return None
	try:
		if value.type.strip_typedefs().code == gdb.TYPE_CODE_ARRAY:
			return value[0].address
	except Exception:
		pass
	return value

def string_parts(value):
	##
	# Выводит начало данных строки и её длину, каким бы ни был вид записи
	#
	# Имена внутренних полей у библиотек расходятся, а у libc++ строка вдобавок бывает
	# КОРОТКОЙ: тогда знаки лежат в самом теле строки, а не за указателем. Прежняя
	# редакция знала лишь libstdc++, и на libc++ описатель молча терял ВЕСЬ вывод
	##
	if value is None:
		return (None, 0)
	# libstdc++: указатель да длина полями вровень
	try:
		return (pointer_of(value['_M_dataplus']['_M_p']), int(value['_M_string_length']))
	except Exception:
		pass
	# libc++: короткая запись отличается от долгой признаком `__is_long_`
	try:
		body = value['__r_']['__value_']
		##
		# Признак долгой записи у новых сборок лежит ОТДЕЛЬНЫМ полем, у старых - младшим
		# разрядом размера короткой записи. Различать их нужно строго: у новых размер
		# хранится как есть, и лишний сдвиг режет строку - «Юрий» выходило «Юр»
		##
		try:
			long_form = int(body['__l']['__is_long_'])
			shifted = False
		except Exception:
			long_form = (int(body['__s']['__size_']) & 1)
			shifted = True
		if long_form:
			return (pointer_of(body['__l']['__data_']), int(body['__l']['__size_']))
		size = int(body['__s']['__size_'])
		if shifted:
			size = (size >> 1)
		##
		# У КОРОТКОЙ записи знаки лежат МАССИВОМ внутри строки, а не за указателем
		#
		# Сложение со смещением по массиву gdb отвергает - «Argument to arithmetic
		# operation not a number or boolean», - и описатель терял вывод там, где строка
		# коротка: заголовок таблицы CSV выходил «[?, ?, ?]». Массив приводится к
		# указателю на первый знак
		##
		return (pointer_of(body['__s']['__data_']), size)
	except Exception:
		return (None, 0)

def text_of(value):
	##
	# Выводит содержимое строки стандартной библиотеки целиком
	##
	data, length = string_parts(value)
	if (data is None) or (length == 0):
		return '' if (data is not None) else None
	try:
		raw = gdb.selected_inferior().read_memory(data, length).tobytes()
	except Exception:
		return None
	return raw.decode('utf-8', errors = 'replace')

def size_of(vector):
	##
	# Выводит длину перечня стандартной библиотеки
	##
	return count_of(vector)

class TomlValue(object):
	##
	# Описатель значения TOML
	#
	# Значение это ВЛАДЕЮЩЕЕ, а не ссылка: толк описателя иной - показать вид да
	# содержимое вместо дюжины полей, из коих при всяком виде заполнено одно
	##
	def __init__(self, value):
		self.value = value
	def to_string(self):
		kind = whole(field(self.value, '_type'))
		name = TOML_TYPES.get(kind, 'вид 0x%02x' % kind)
		if name == 'table':
			return 'table, ключей: %d' % size_of(field(self.value, '_names'))
		if name == 'array':
			return 'array, значений: %d' % size_of(field(self.value, '_items'))
		if name == 'bool':
			return 'true' if (whole(field(self.value, '_boolean')) != 0) else 'false'
		if name == 'integer':
			return str(whole(field(self.value, '_integer')))
		if name == 'float':
			real = field(self.value, '_real')
			return str(float(real)) if (real is not None) else name
		if name == 'string':
			text = text_of(field(self.value, '_text'))
			return ('"%s"' % text) if (text is not None) else name
		if name.startswith('local') or name.startswith('offset'):
			# Отметка составная: `date` да `time`, а не полтора десятка полей вровень
			stamp = field(self.value, '_stamp')
			return '%s %04d-%02d-%02d %02d:%02d:%02d' % (
				name,
				whole(field(stamp, 'date', 'year')), whole(field(stamp, 'date', 'month')),
				whole(field(stamp, 'date', 'day')), whole(field(stamp, 'time', 'hour')),
				whole(field(stamp, 'time', 'minute')), whole(field(stamp, 'time', 'second')))
		return name

class IniValue(object):
	##
	# Описатель значения INI
	#
	# Видов у настроек всего четыре: записи там нетипизированы
	##
	def __init__(self, value):
		self.value = value
	def to_string(self):
		kind = whole(field(self.value, '_type'))
		name = INI_TYPES.get(kind, 'вид 0x%02x' % kind)
		if name == 'table':
			return 'table, ключей: %d' % size_of(field(self.value, '_names'))
		if name == 'array':
			return 'array, значений: %d' % size_of(field(self.value, '_items'))
		if name == 'string':
			text = text_of(field(self.value, '_text'))
			return ('"%s"' % text) if (text is not None) else name
		return name

# Виды узлов ABC: `include/codec/abc/common.hpp`
ABC_KINDS = {
	0x00: 'NONE', 0x01: 'null', 0x02: 'bool', 0x03: 'number', 0x04: 'string',
	0x05: 'blob', 0x06: 'time', 0x07: 'uuid', 0x08: 'array',  0x09: 'map',
	0x0A: 'custom',
}

# Виды содержимого ABC
ABC_TYPES = {
	0x00000000: 'UNDEFINED', 0x00000001: 'null',   0x00000002: 'bool',   0x00000004: 'string',
	0x00000008: 'blob',      0x00000010: 'array',  0x00000020: 'map',    0x00000040: 'time',
	0x00000080: 'uuid',      0x00000100: 'int8',   0x00000200: 'int16',  0x00000400: 'int32',
	0x00000800: 'int64',     0x00001000: 'uint8',  0x00002000: 'uint16', 0x00004000: 'uint32',
	0x00008000: 'uint64',    0x00010000: 'float',  0x00020000: 'double', 0x00040000: 'extended',
	0x00080000: 'decimal',   0x00100000: 'custom',
}

class AbcValue(object):
	##
	# Описатель владеющего значения ABC
	#
	# Вид у значения ДВОЙНОЙ: `_kind` сказывает род, а `_type` уточняет ширину числа
	##
	def __init__(self, value):
		self.value = value
	def to_string(self):
		kind = whole(field(self.value, '_kind'))
		name = ABC_KINDS.get(kind, 'вид 0x%02x' % kind)
		if name == 'map':
			return 'map, пар: %d' % size_of(field(self.value, '_keys'))
		if name == 'array':
			return 'array, значений: %d' % size_of(field(self.value, '_items'))
		width = ABC_TYPES.get(whole(field(self.value, '_type')), 'вид')
		if name in ('string', 'blob', 'uuid'):
			text = text_of(field(self.value, '_text'))
			if text is None:
				return name
			return ('"%s"' % text) if (name == 'string') else ('%s "%s"' % (name, text))
		if name == 'bool':
			return 'true' if (whole(field(self.value, '_number', 'flag')) != 0) else 'false'
		if name == 'number':
			if width in ('uint8', 'uint16', 'uint32', 'uint64'):
				return '%d (%s)' % (whole(field(self.value, '_number', 'natural')), width)
			if width in ('int8', 'int16', 'int32', 'int64'):
				return '%d (%s)' % (whole(field(self.value, '_number', 'integer')), width)
			if width in ('float', 'double'):
				real = field(self.value, '_number', 'real')
				return '%s (%s)' % (float(real) if (real is not None) else '?', width)
			if width in ('extended', 'decimal'):
				##
				# Величина живёт ОКТЕТАМИ, а не десятичной записью
				#
				# Вывод `_text` знаками дал бы мусор: там двоичная величина
				##
				sign = '-' if (whole(field(self.value, '_negative')) != 0) else '+'
				if width == 'decimal':
					return '%s, знак %s, порядок %d' % (width, sign, whole(field(self.value, '_exponent')))
				return '%s, знак %s' % (width, sign)
			return width
		if name in ('null', 'NONE'):
			return name
		return '%s (%s)' % (name, width)

class AbcNode(object):
	##
	# Описатель ссылки на узел документа ABC
	#
	# Клейма поколения у ссылки НЕТ вовсе, есть лишь граница вместилища: устарелость
	# ссылки описатель поймать не может и обещать того не станет
	##
	def __init__(self, value):
		self.value = value
	def to_string(self):
		doc = field(self.value, '_doc')
		if (doc is None) or (whole(doc) == 0):
			return 'недействительная ссылка'
		index = whole(field(self.value, '_index'))
		doc = doc.dereference()
		node = node_at(field(doc, '_nodes'), index)
		if node is None:
			return 'номер узла %d вне перечня' % index
		kind = whole(field(node, 'type'))
		name = ABC_TYPES.get(kind, 'вид 0x%08x' % kind)
		content = field(node, 'content')
		length = whole(content[0]) if (content is not None) else 0
		offset = whole(field(node, 'offset'))
		storage = field(doc, '_storage')
		if name in ('array', 'map'):
			return '%s, детей: %d' % (name, length)
		if name in ('null', 'UNDEFINED'):
			return name
		if name == 'bool':
			return 'true' if (length != 0) else 'false'
		if name == 'string':
			text = storage_slice(storage, offset, length)
			return ('"%s"' % text) if (text is not None) else ('%s, длина %d' % (name, length))
		if name in ('blob', 'uuid'):
			return '%s, октетов: %d' % (name, length)
		if name in ('extended', 'decimal', 'custom'):
			# Величина лежит за восемью байтами порядка либо подвида
			sign = '-' if (whole(field(node, 'negative')) != 0) else '+'
			return '%s, октетов: %d, знак %s' % (name, length, sign)
		number = node_number(content, name)
		return ('%s (%s)' % (number, name)) if (number is not None) else ('%s, узел %d' % (name, index))

# Виды сетевого адреса: `include/net/addr.hpp`
NET_TYPES = {
	0x00: 'NONE', 0x01: 'путь', 0x02: 'MAC', 0x03: 'URL', 0x04: 'IPv4',
	0x05: 'IPv6', 0x06: 'FQDN', 0x07: 'сеть IPv4', 0x08: 'сеть IPv6',
}

# Виды разобранной ссылки: `include/net/nwt.hpp`
NWT_TYPES = { 0x00: 'NONE', 0x01: 'MAC', 0x02: 'URL', 0x03: 'IPv4', 0x04: 'IPv6', 0x05: 'почта' }

def net_ipv6(groups):
	##
	# Собирает запись IPv6, сократив самую длинную вереницу нулей
	##
	best, length, start, count = -1, 0, -1, 0
	for index, group in enumerate(groups + [1]):
		if (index < len(groups)) and (group == 0):
			if start < 0:
				start = index
			count += 1
			continue
		# Вереница длиною в одну группу не сокращается: так велит запись адреса
		if (count > 1) and (count > length):
			best, length = start, count
		start, count = -1, 0
	if best < 0:
		return ':'.join(('%x' % group) for group in groups)
	head = ':'.join(('%x' % group) for group in groups[:best])
	tail = ':'.join(('%x' % group) for group in groups[(best + length):])
	return '%s::%s' % (head, tail)

class NetAddress(object):
	##
	# Описатель сетевого адреса
	#
	# Адрес живёт двоичным буфером о шестнадцати байтах: разбираем его по виду адреса в
	# привычную запись
	##
	def __init__(self, value):
		self.value = value
	def to_string(self):
		kind = whole(field(self.value, '_type'))
		name = NET_TYPES.get(kind, 'вид 0x%02x' % kind)
		buffer = field(self.value, '_buffer')
		size = whole(field(buffer, '_size'))
		data = field(buffer, '_data')
		if (size == 0) or (data is None):
			return '%s, буфер пуст' % name
		octets = [whole(data[i]) for i in range(size)]
		if size == 4:
			return '%s %d.%d.%d.%d' % (name, octets[0], octets[1], octets[2], octets[3])
		if size == 6:
			return '%s %s' % (name, ':'.join(('%02x' % octet) for octet in octets))
		if size == 16:
			# Адрес IPv4, в IPv6 уложенный, показываем четверицей: так он читается
			if (octets[:10] == ([0] * 10)) and (octets[10] == 0xFF) and (octets[11] == 0xFF):
				return '%s ::ffff:%d.%d.%d.%d' % (name, octets[12], octets[13], octets[14], octets[15])
			groups = [((octets[i] << 8) | octets[i + 1]) for i in range(0, 16, 2)]
			text = net_ipv6(groups)
			zone = text_of(field(self.value, '_zone'))
			if zone:
				text = '%s%%%s' % (text, zone)
			return '%s %s' % (name, text)
		return '%s, байтов: %d' % (name, size)

class NwtUrl(object):
	##
	# Описатель разобранной ссылки
	#
	# Ссылка разобрана на десяток строк и полями своими нечитаема: собираем из них ту
	# самую запись, какую разбирали
	##
	def __init__(self, value):
		self.value = value
	def to_string(self):
		kind = whole(field(self.value, 'type'))
		name = NWT_TYPES.get(kind, 'вид 0x%02x' % kind)
		def part(named):
			text = text_of(field(self.value, named))
			return text if text else ''
		host = part('host')
		if not host:
			return '%s, пусто' % name
		# Собираем запись слева направо, а не правкою уже собранной
		result = ''
		schema = part('schema')
		if schema:
			result += '%s://' % schema
		user = part('user')
		if user:
			# Пароль показываем звёздочками: отладочный вывод расходится по журналам
			result += '%s%s@' % (user, ':***' if part('pass') else '')
		result += host
		port = whole(field(self.value, 'port'))
		if port > 0:
			result += ':%d' % port
		result += part('path')
		params = part('params')
		if params:
			result += '?%s' % params
		anchor = part('anchor')
		if anchor:
			result += '#%s' % anchor
		return '%s %s' % (name, result)

class RegexProgram(object):
	##
	# Описатель порождённой программы выражения
	#
	# Поля программы это перечни команд да разрядных карт, и вывод их целиком заслоняет
	# главное: число команд и признаки, по каким выбирается способ сличения
	##
	def __init__(self, value):
		self.value = value
	def to_string(self):
		text = text_of(field(self.value, 'text'))
		result = ('/%s/' % text) if text else 'программа'
		result += ', команд: %d' % whole(field(self.value, 'instructions', '_count'))
		captures = whole(field(self.value, 'captures'))
		if captures > 0:
			result += ', захватов: %d' % captures
		# Признаки показываем только поднятые: перечень из шести «нет» заслоняет нужное
		marks = [mark for named, mark in
		 (('plain', 'простое'), ('anchored', 'с якорем'), ('sweeping', 'обметает'))
		 if (whole(field(self.value, named)) != 0)]
		if marks:
			result += ', ' + ', '.join(marks)
		return result

class RegexExpression(object):
	##
	# Описатель собранного выражения
	##
	def __init__(self, value):
		self.value = value
	def to_string(self):
		forward = field(self.value, 'forward')
		##
		# Шаблон показывается ТОЛЬКО у простого выражения
		#
		# Поле `text` держит полосу знаков простого выражения, а не сам шаблон
		##
		text = text_of(field(forward, 'text'))
		if text and (whole(field(forward, 'plain')) != 0):
			result = '/%s/' % text
		else:
			result = 'выражение, команд: %d' % whole(field(forward, 'instructions', '_count'))
		if whole(field(self.value, 'ready')) == 0:
			return '%s, НЕ СОБРАНО' % result
		names = field(self.value, 'names')
		##
		# Число имён берём у отображения, а не счётом детей
		##
		##
		# Число записей отображения: `_M_h._M_element_count` у libstdc++,
		# `__table_.__p2_` у libc++
		##
		count = map_count(names)
		if count > 0:
			result += ', имён: %d' % count
		##
		# Умный указатель целым не берётся: смотреть нужно поле самого указателя
		##
		##
		# Умный указатель зовётся `_M_ptr` у libstdc++ и `__ptr_` у libc++
		##
		machine = field(self.value, 'machine', '_M_ptr')
		if machine is None:
			machine = field(self.value, 'machine', '__ptr_')
		if (machine is not None) and (whole(machine) != 0):
			return result + ', порождённый код'
		if whole(field(self.value, 'backtracking')) != 0:
			return result + ', возвраты'
		return result + ', лента Pike'

def map_count(value):
	##
	# Выводит число записей отображения стандартной библиотеки
	#
	# Поле счёта у библиотек разное: `_M_h._M_element_count` у libstdc++ и
	# `__table_.__p2_.__value_` у libc++. Счёт детьми здесь негоден - у отображения
	# дети суть корзины, а не записи
	##
	if value is None:
		return 0
	for path in (('_M_h', '_M_element_count'), ('__table_', '__p2_', '__value_')):
		try:
			current = value
			for name in path:
				current = current[name]
			return int(current)
		except Exception:
			continue
	return 0

def bytes_text(count):
	##
	# Выводит размер приставкой, чтобы четыре мегабайта не читались числом 4194304
	##
	if count < 1024:
		return '%d Б' % count
	if count < (1024 * 1024):
		return '%.1f КБ' % (count / 1024.0)
	return '%.1f МБ' % (count / (1024.0 * 1024.0))

class QueueBox(object):
	##
	# Описатель очереди записей
	#
	# Очередь живёт одним куском памяти да границами внутри него: показываем число
	# записей и занятое ими место
	##
	def __init__(self, value):
		self.value = value
	def head(self, begin):
		##
		# Выводит, сколько отдаст ближайшее чтение: длина записи лежит впереди её
		# содержимого, а прочитанное считается смещением чтения
		##
		buffer = field(self.value, '_buffer')
		if buffer is None:
			return None
		data, _ = vector_bounds(buffer)
		if data is None:
			return None
		try:
			raw = gdb.selected_inferior().read_memory((data + begin), 8).tobytes()
		except Exception:
			return None
		length = 0
		for index in range(8):
			length |= (raw[index] << (index * 8))
		offset = whole(field(self.value, '_range', 'offset'))
		return (length - min(length, offset))
	def to_string(self):
		count = whole(field(self.value, '_range', 'count'))
		if count == 0:
			return 'очередь пуста'
		begin = whole(field(self.value, '_range', 'begin'))
		end = whole(field(self.value, '_range', 'end'))
		##
		# «Область» - это ЗАНЯТЫЙ КУСОК со служебными полями, а не сумма записей
		#
		# Первая редакция звала его «занято», и подпись эта врала: `size()` очереди
		# отдаёт длину БЛИЖАЙШЕЙ записи за вычетом прочитанного. Разошлось втрое
		##
		result = 'очередь, записей: %d, область: %s' % (count, bytes_text(end - begin))
		head = self.head(begin)
		if head is not None:
			result += ', к выдаче: %s' % bytes_text(head)
		# Предел показываем лишь когда он поставлен: ноль там значит «без предела»
		limit = whole(field(self.value, '_max', 'memory'))
		if limit > 0:
			result += ', предел: %s' % bytes_text(limit)
		records = whole(field(self.value, '_max', 'records'))
		if records > 0:
			result += ', предел записей: %d' % records
		return result

class BufferBox(object):
	##
	# Описатель буфера
	##
	def __init__(self, value):
		self.value = value
	def to_string(self):
		begin = whole(field(self.value, '_range', 'begin'))
		end = whole(field(self.value, '_range', 'end'))
		reserved = whole(field(self.value, '_range', 'reserved'))
		if end <= begin:
			return 'буфер пуст, отведено: %s' % bytes_text(reserved)
		return 'буфер, занято: %s из %s' % (bytes_text(end - begin), bytes_text(reserved))

class CsvDocument(object):
	##
	# Описатель таблицы CSV
	#
	# Таблица живёт двумя хранилищами знаков да перечнями указаний в них: показываем
	# размер таблицы и имена столбцов, коли заголовок объявлен
	##
	def __init__(self, value):
		self.value = value
	def to_string(self):
		records = field(self.value, '_records')
		fields = field(self.value, '_fields')
		header = field(self.value, '_header')
		rows = size_of(records)
		cells = size_of(fields)
		titles = size_of(header)
		if (rows == 0) and (cells == 0):
			return 'таблица пуста'
		##
		# Число столбцов считаем по заголовку, а коли его нет - по первой записи
		#
		# Строки CSV равной ширины не обещают
		##
		columns = titles
		if (columns == 0) and (rows > 0):
			first = whole(at(records, 0))
			following = whole(at(records, 1)) if (rows > 1) else cells
			columns = (following - first)
		result = 'таблица, записей: %d, столбцов: %d, полей: %d' % (rows, columns, cells)
		if titles > 0:
			##
			# Имена столбцов лежат в СВОЁМ хранилище `_names`, отдельном от полей
			##
			names = field(self.value, '_names')
			shown = []
			for index in range(min(titles, 4)):
				text = span_text(names, at(header, index))
				shown.append(text if text else '?')
			if titles > 4:
				shown.append('…')
			result += ' [%s]' % ', '.join(shown)
		return result

def at(vector, index):
	##
	# Выдаёт запись перечня по номеру
	##
	return node_at(vector, index)

def atomic_value(value):
	##
	# Выводит содержимое атомарного поля
	#
	# Значение лежит ВНУТРИ поля, и зовётся оно у библиотек по-разному: `_M_i` у
	# libstdc++, `__a_` либо `__val_` у libc++. Прежняя редакция знала лишь первое имя, и
	# на libc++ описатель молча показывал «занято: 0 Б» при полном складе - отказ, какой
	# не отличить от пустоты
	##
	if value is None:
		return 0
	##
	# Пути перебираются ЦЕЛИКОМ, а не по одному имени
	#
	# У `std::atomic <bool>` значение лежит уровнем глубже - `_M_base._M_i`, - и
	# перебор одних лишь верхних имён его не достаёт. Отказ этот выходил не пустотой,
	# а ЛОЖНОЙ ИСТИНОЙ: запасной путь брал первое поле, каким оказывалось составное
	# `_M_base`, и пул с обоими признаками ложными выводился «ОСТАНОВЛЕН, ждёт
	# завершения». Вскрылось сличением с редакцией для LLDB, где libc++ отдаёт `__a_`
	# верхним полем
	##
	for path in (('_M_i',), ('__a_',), ('__val_',), ('_M_base', '_M_i'), ('__a_', '__val_')):
		try:
			current = value
			for name in path:
				current = current[name]
			return int(current)
		except Exception:
			continue
	##
	# Имени нет вовсе - берём первое поле, но лишь ПРОСТОЕ
	#
	# Составное поле целым числом не берётся, и попытка эта давала мусор вместо отказа
	##
	try:
		fields = value.type.fields()
		if fields and (fields[0].type.strip_typedefs().code not in (gdb.TYPE_CODE_STRUCT, gdb.TYPE_CODE_UNION)):
			return int(value[fields[0].name])
	except Exception:
		pass
	return whole(value)

class AllocCache(object):
	##
	# Описатель склада разрядов потока
	#
	# Склад держит девяносто шесть списков свободных блоков, и вывод их подряд бесполезен:
	# непустых там единицы. Показываем занятое, предел и заселённые разряды
	##
	def __init__(self, value):
		self.value = value
	def to_string(self):
		held = atomic_value(field(self.value, '_bytes'))
		limit = atomic_value(field(self.value, '_limit'))
		kept = atomic_value(field(self.value, '_kept'))
		lists = field(self.value, '_lists')
		##
		# Считаем ЗАСЕЛЁННЫЕ разряды, а не сумму блоков: сумма о раскладе молчит
		##
		busy, total = [], 0
		if lists is not None:
			try:
				count = lists.type.range()[1] + 1
			except Exception:
				count = 0
			for index in range(count):
				number = whole(field(lists[index], 'count'))
				if number > 0:
					total += number
					if len(busy) < 6:
						busy.append('%d:%d' % (index, number))
		result = 'склад, занято: %s' % bytes_text(held)
		if limit > 0:
			result += ' из %s' % bytes_text(limit)
		if kept > 0:
			result += ', придержано: %s' % bytes_text(kept)
		if total > 0:
			tail = ' …' if (len(busy) == 6) else ''
			return result + ', блоков: %d [%s%s]' % (total, ' '.join(busy), tail)
		return result + ', списки пусты'

class QuicFifo(object):
	##
	# Описатель очереди блоков QUIC
	#
	# Очередь копит принятое цепочкой блоков по 16 КБ, беря их из пула. Смотрят у неё
	# накопленное да число блоков: цепочка в сотни блоков при малом накопленном говорит,
	# что блоки берутся, но не заполняются
	#
	# @note Логический размер берётся ПОЛЕМ `_bytes`, а не суммой длин блоков: хвостовой
	#       блок обычно неполон
	##
	def __init__(self, value):
		self.value = value
	def to_string(self):
		held = whole(field(self.value, '_bytes'))
		##
		# Число блоков берётся ГРАНИЦАМИ вектора, а не полем `_count`
		#
		# Цепочка эта - обычный `std::vector`, поля такого у неё нет. Редакция для LLDB
		# на том и споткнулась, отвечая «блоков: 0» при 39 КБ накопленного
		##
		count = count_of(field(self.value, '_blocks'))
		if (count == 0) and (held == 0):
			return 'очередь блоков пуста'
		result = 'очередь блоков, накоплено: %s, блоков: %d' % (bytes_text(held), count)
		if count > 0:
			result += ' (в среднем по %s)' % bytes_text(held // count)
		return result

class QuicSmallVector(object):
	##
	# Описатель вектора с коротким запасом
	#
	# Вектор держит первые N элементов у себя, а сверх того уходит в кучу. Уход этот и
	# есть то, ради чего на него смотрят: выделение памяти там, где его не ждали
	#
	# @note Место хранения определяется СЛИЧЕНИЕМ указателя с адресом своего запаса:
	#       отдельного признака у вектора нет вовсе
	##
	def __init__(self, value):
		self.value = value
	def to_string(self):
		size = whole(field(self.value, '_size'))
		cap = whole(field(self.value, '_cap'))
		where = 'место неизвестно'
		try:
			data = int(field(self.value, '_data'))
			own = int(pointer_of(field(self.value, '_storage')))
			where = 'свой запас' if (data == own) else 'куча'
		except Exception:
			pass
		if size == 0:
			return 'вектор пуст, ёмкость: %d (%s)' % (cap, where)
		return 'вектор, элементов: %d из %d (%s)' % (size, cap, where)

class CallbackBox(object):
	##
	# Описатель набора откликов
	#
	# Набор держит отклики отображением по опознавателю. Смотрят у него ровно число
	# подписок: пустой набор там, где ждали подписки, и есть частая беда
	##
	def __init__(self, value):
		self.value = value
	def to_string(self):
		count = map_count(field(self.value, '_callbacks'))
		if count == 0:
			return 'набор откликов пуст'
		return 'набор откликов, подписок: %d' % count

class ThreadpoolBox(object):
	##
	# Описатель пула потоков
	#
	# Показываем работников, очередь задач и признаки остановки: пул, стоящий с полной
	# очередью, от пула простаивающего по виду полей не отличить
	#
	# @note Число работников берётся ПЕРЕЧНЕМ `_workers`, а не полем `_threads`: поле
	#       хранит заказанное число, а перечень - заведённое на деле, и расходятся они
	#       ровно тогда, когда пул не поднялся
	#
	# @note Признак `ОСТАНОВЛЕН` виден лишь ВО ВРЕМЯ остановки, изнутри работника:
	#       `stop()` в конце сам возвращает `_stop` в ложь и чистит перечень работников.
	#       Оттого у остановленного пула выходит «работников: 0 (заказано: N)» без
	#       признака - и это верно, а не пробел описателя
	##
	def __init__(self, value):
		self.value = value
	def to_string(self):
		wanted = whole(field(self.value, '_threads'))
		workers = count_of(field(self.value, '_workers'))
		##
		# Длина очереди задач берётся с оглядкой на её устройство
		#
		# Очередь эта - `std::queue` поверх `std::deque`, а у deque нет пары указателей
		# «начало-конец»: помощник `count_of` там отвечает ОТКАЗОМ, а не нулём, и отказ
		# этот GDB глотает молча - описатель выводил пустоту вместо строки. Оттого счёт
		# обёрнут, а неудача его говорится словом, а не подменяется нулём
		##
		tasks = None
		for path in (('_tasks', 'c'), ('_tasks', '__c')):
			holder = field(self.value, *path)
			if holder is None:
				continue
			try:
				tasks = count_of(holder)
			except Exception:
				##
				# У deque длина считается по картам блоков, и брать её вручную дороже
				# пользы: пул с задачами смотрят методом `getTaskQueueSize()`
				##
				tasks = None
			break
		result = 'пул, работников: %d' % workers
		if wanted != workers:
			result += ' (заказано: %d)' % wanted
		result += ', задач: %s' % (('%d' % tasks) if (tasks is not None) else 'см. getTaskQueueSize()')
		if atomic_value(field(self.value, '_stop')) != 0:
			result += ', ОСТАНОВЛЕН'
		if atomic_value(field(self.value, '_wait')) != 0:
			result += ', ждёт завершения'
		return result

def lookup(value):
	##
	# Выбирает описатель по имени вида
	#
	# Имя берём у НЕПОКРЫТОГО вида: `value_t` есть псевдоним, и сличение по нему пропустило
	# бы переменные, объявленные самим видом
	##
	kind = value.type.unqualified().strip_typedefs()
	if kind.code == gdb.TYPE_CODE_REF:
		kind = kind.target().unqualified().strip_typedefs()
	name = kind.tag if (kind.tag is not None) else str(kind)
	if name == 'awh::codec::json::Document::Value':
		return JsonValue(value)
	if name == 'awh::codec::yaml::Document::Value':
		return YamlValue(value)
	if name == 'awh::codec::xml::Node':
		return XmlNode(value)
	if name == 'awh::codec::toml::Value':
		return TomlValue(value)
	if name == 'awh::codec::ini::Value':
		return IniValue(value)
	if name == 'awh::codec::abc::Value':
		return AbcValue(value)
	if name == 'awh::codec::abc::Document::Value':
		return AbcNode(value)
	if name == 'awh::Network_Address':
		return NetAddress(value)
	if name == 'awh::Network_Types::URL':
		return NwtUrl(value)
	if name == 'awh::regex::Program':
		return RegexProgram(value)
	if name == 'awh::regex::Expression':
		return RegexExpression(value)
	if name == 'awh::codec::csv::Document':
		return CsvDocument(value)
	if name == 'awh::alloc::Cache':
		return AllocCache(value)
	if name == 'awh::Queue':
		return QueueBox(value)
	if name == 'awh::Buffer':
		return BufferBox(value)
	if name == 'awh::quic::chunked_fifo_t':
		return QuicFifo(value)
	##
	# Вектор сличается НАЧАЛОМ имени, а не всем именем
	#
	# `small_vector` - шаблон, и полное имя его несёт доводы в угловых скобках:
	# `awh::quic::small_vector<int, 4>`. Сличение целым именем не совпало бы ни с одним
	# воплощением
	##
	if name.startswith('awh::quic::small_vector<'):
		return QuicSmallVector(value)
	if name == 'awh::Callback':
		return CallbackBox(value)
	if name == 'awh::Threadpool':
		return ThreadpoolBox(value)
	return None

gdb.pretty_printers.append(lookup)
print('AWH: описатели значений заведены')
