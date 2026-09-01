# -*- coding: utf-8 -*-
##
# @file: awh.py
# @brief Описатели значений AWH для отладчика LLDB
#
# @details Отладчик показывает поля структуры как они лежат в памяти, и для ссылочных
#          видов - таких как `json::Document::Value`, где лежат лишь номер узла да
#          отпечаток документа, - вывод этот бесполезен. Описатели здесь читают документ
#          и показывают ВИД узла и его содержимое, подобно тому как Visual Studio
#          показывает вместилища стандартной библиотеки
#
# @note Подключается само файлом `.lldbinit` в корне дерева. Отдельно:
#           (lldb) command script import tools/debug/awh.py
#
# @author ANYKS
##

import lldb

# Виды значений узла документа JSON: `include/codec/json/common.hpp`
JSON_TYPES = {
	0x0000: 'UNDEFINED', 0x0001: 'null',  0x0002: 'bool',   0x0004: 'string',
	0x0008: 'array',     0x0010: 'object', 0x0020: 'int8',  0x0040: 'int16',
	0x0080: 'int32',     0x0100: 'int64',  0x0200: 'uint8', 0x0400: 'uint16',
	0x0800: 'uint32',    0x1000: 'uint64', 0x2000: 'float', 0x4000: 'double',
	0x8000: 'extended',
}

def read_member(value, *names):
	##
	# Читает поле по цепочке имён, отвечая None на первом же отсутствующем
	#
	# Цепочкой, а не одним именем: поля лежат в частной части и порой за ссылкой
	##
	for name in names:
		if value is None:
			return None
		value = value.GetChildMemberWithName(name)
		if not value.IsValid():
			return None
	return value

def json_value_summary(value, internal_dict):
	##
	# Описатель ссылки на узел документа JSON - `awh::codec::json::Document::Value`
	#
	# Ссылка эта хранит лишь номер узла да отпечаток документа. Чтобы показать
	# содержимое, читаем сам документ: перечень узлов и хранилище знаков
	##
	doc = read_member(value, '_doc')
	if (doc is None) or (doc.GetValueAsUnsigned(0) == 0):
		return 'недействительная ссылка'
	index = read_member(value, '_index')
	stamp = read_member(value, '_stamp')
	if (index is None) or (stamp is None):
		return '<поля ссылки недоступны>'
	index = index.GetValueAsUnsigned(0)
	# Отпечаток стережёт от чтения перестроенного документа
	current = read_member(doc, '_stamp')
	if (current is not None) and (current.GetValueAsUnsigned(0) != stamp.GetValueAsUnsigned(0)):
		return 'ссылка устарела: документ перестроен'
	nodes = read_member(doc, '_nodes')
	if nodes is None:
		return '<перечень узлов недоступен>'
	count = nodes.GetNumChildren()
	if index >= count:
		return 'номер узла %d вне перечня из %d' % (index, count)
	node = nodes.GetChildAtIndex(index)
	kind = read_member(node, 'type')
	if kind is None:
		return '<узел без вида>'
	kind = kind.GetValueAsUnsigned(0)
	name = JSON_TYPES.get(kind, 'вид 0x%04x' % kind)
	# Длина содержимого либо число детей вместилища
	length = read_member(node, 'content')
	length = length.GetChildAtIndex(0).GetValueAsUnsigned(0) if length is not None else 0
	if name in ('array', 'object'):
		return '%s, детей: %d' % (name, length)
	if name == 'string':
		##
		# Читаем СВОЙ кусок хранилища, а не всё хранилище целиком
		#
		# Хранилище знаков одно на весь документ, и его краткий вид показал бы содержимое
		# всех узлов разом. Свой кусок задают смещение да длина
		##
		offset = read_member(node, 'offset')
		storage = read_member(doc, '_storage')
		if (offset is not None) and (storage is not None):
			text = storage_slice(storage, offset.GetValueAsUnsigned(0), length)
			if text is not None:
				return '"%s"' % text
		return '%s, длина %d' % (name, length)
	if name in ('null', 'UNDEFINED'):
		return name
	if name == 'bool':
		return 'true' if (length != 0) else 'false'
	##
	# Числа: содержимое лежит в тех же восьми байтах, что длина у прочих видов
	##
	number = node_number(node, name)
	if number is not None:
		return str(number)
	return '%s, узел %d' % (name, index)

def storage_slice(storage, offset, length):
	##
	# Выводит кусок хранилища знаков по смещению и длине
	#
	# Читаем память по адресу данных: краткий вид строки годится лишь целиком, а нам
	# нужен именно свой кусок
	##
	if length == 0:
		return ''
	if length > 4096:
		length = 4096
	summary = storage.GetSummary()
	if summary is None:
		return None
	# Краткий вид приходит в кавычках: снимаем их
	body = summary.strip()
	if body.startswith('"') and body.endswith('"'):
		body = body[1:-1]
	##
	# Режем БАЙТЫ, а не знаки
	#
	# Смещение и длина узла считаны в байтах, тогда как краткий вид приходит уже
	# разобранным в знаки: у кириллицы знак занимает два байта, и резка по знакам
	# сдвигала содержимое - «йчисло42» вместо «Юрий»
	##
	raw = body.encode('utf-8', errors='replace')
	if offset >= len(raw):
		return None
	return raw[offset:(offset + length)].decode('utf-8', errors='replace')

def node_number(node, name):
	##
	# Собирает число узла из восьми байтов содержимого
	##
	content = read_member(node, 'content')
	if content is None:
		return None
	low = content.GetChildAtIndex(0).GetValueAsUnsigned(0)
	high = content.GetChildAtIndex(1).GetValueAsUnsigned(0)
	whole = (low | (high << 32))
	if name in ('uint8', 'uint16', 'uint32', 'uint64'):
		return whole
	if name in ('int8', 'int16', 'int32', 'int64'):
		# Знаковое: восстанавливаем знак по старшему разряду
		return (whole - (1 << 64)) if (whole & (1 << 63)) else whole
	if name in ('float', 'double'):
		import struct
		return struct.unpack('<d', struct.pack('<Q', whole))[0]
	return None

# Виды значений узла документа YAML: `include/codec/yaml/common.hpp`
YAML_TYPES = {
	0x00000000: 'UNDEFINED', 0x00000001: 'null',   0x00000002: 'bool',   0x00000004: 'string',
	0x00000008: 'sequence',  0x00000010: 'mapping', 0x00000020: 'int8',  0x00000040: 'int16',
	0x00000080: 'int32',     0x00000100: 'int64',  0x00000200: 'uint8',  0x00000400: 'uint16',
	0x00000800: 'uint32',    0x00001000: 'uint64', 0x00002000: 'float',  0x00004000: 'double',
	0x00008000: 'extended',  0x00010000: 'binary', 0x00020000: 'stamp',
}

# Виды узлов дерева разметки XML: `include/codec/xml/document.hpp`
XML_KINDS = {
	0x00: 'NONE',    0x01: 'document', 0x02: 'element',    0x03: 'text',
	0x04: 'cdata',   0x05: 'comment',  0x06: 'processing', 0x07: 'doctype',
	0x08: 'space',
}

def yaml_value_summary(value, internal_dict):
	##
	# Описатель ссылки на узел документа YAML - `awh::codec::yaml::Document::Value`
	#
	# Устройство ссылки то же, что у JSON, а вот узел иной: запись значения удерживается
	# всегда, число лежит ОТДЕЛЬНЫМ полем `number`, а `offset` указывает на ИМЯ пары, за
	# которым следом идёт сама запись
	##
	doc = read_member(value, '_doc')
	if (doc is None) or (doc.GetValueAsUnsigned(0) == 0):
		return 'недействительная ссылка'
	index = read_member(value, '_index')
	stamp = read_member(value, '_stamp')
	if (index is None) or (stamp is None):
		return '<поля ссылки недоступны>'
	index = index.GetValueAsUnsigned(0)
	##
	# Клеймо документа зовётся `_generation`, а не `_stamp`
	#
	# Имена полей у кодеков расходятся, и общего чтения тут не выйдет: у JSON клеймо
	# документа названо так же, как у ссылки, у YAML - иначе
	##
	current = read_member(doc, '_generation')
	if (current is not None) and (current.GetValueAsUnsigned(0) != stamp.GetValueAsUnsigned(0)):
		return 'ссылка устарела: документ перестроен'
	nodes = read_member(doc, '_nodes')
	if nodes is None:
		return '<перечень узлов недоступен>'
	count = nodes.GetNumChildren()
	if index >= count:
		return 'номер узла %d вне перечня из %d' % (index, count)
	node = nodes.GetChildAtIndex(index)
	kind = read_member(node, 'type')
	if kind is None:
		return '<узел без вида>'
	kind = kind.GetValueAsUnsigned(0)
	name = YAML_TYPES.get(kind, 'вид 0x%08x' % kind)
	content = read_member(node, 'content')
	length = content.GetChildAtIndex(0).GetValueAsUnsigned(0) if content is not None else 0
	offset = read_member(node, 'offset')
	named = read_member(node, 'named')
	storage = read_member(doc, '_storage')
	offset = offset.GetValueAsUnsigned(0) if offset is not None else 0
	named = named.GetValueAsUnsigned(0) if named is not None else 0
	# Имя пары отображения показываем впереди значения
	title = ''
	if (named > 0) and (storage is not None):
		text = storage_slice(storage, offset, named)
		if text is not None:
			title = '%s: ' % text
	if name in ('sequence', 'mapping'):
		return '%s%s, детей: %d' % (title, name, length)
	if name in ('null', 'UNDEFINED'):
		return '%s%s' % (title, name)
	##
	# Прочие виды показываем ЗАПИСЬЮ, а не разобранным числом
	#
	# Запись удерживается узлом всегда - ради того узел и шире узла JSON, - и она вернее
	# числа: `0x1F` записано так, а не числом 31
	##
	if storage is not None:
		text = storage_slice(storage, (offset + named), length)
		if text is not None:
			return '%s%s (%s)' % (title, text, name)
	return '%s%s, длина %d' % (title, name, length)

def xml_node_summary(value, internal_dict):
	##
	# Описатель узла дерева разметки - `awh::codec::xml::Document::Node`
	#
	# Узел держит номер записи в арене дерева, а имя да содержимое лежат отрезками
	# хранилища знаков: показываем вид узла, имя и содержимое
	##
	doc = read_member(value, '_document')
	if (doc is None) or (doc.GetValueAsUnsigned(0) == 0):
		return 'недействительный узел'
	node_id = read_member(value, '_id')
	stamp = read_member(value, '_stamp')
	if (node_id is None) or (stamp is None):
		return '<поля узла недоступны>'
	node_id = node_id.GetValueAsUnsigned(0)
	current = read_member(doc, '_stamp')
	if (current is not None) and (current.GetValueAsUnsigned(0) != stamp.GetValueAsUnsigned(0)):
		return 'узел устарел: дерево перестроено'
	nodes = read_member(doc, '_nodes')
	if nodes is None:
		return '<арена дерева недоступна>'
	count = nodes.GetNumChildren()
	if node_id >= count:
		return 'номер узла %d вне арены из %d' % (node_id, count)
	record = nodes.GetChildAtIndex(node_id)
	kind = read_member(record, 'kind')
	kind = kind.GetValueAsUnsigned(0) if kind is not None else 0
	name = XML_KINDS.get(kind, 'вид 0x%02x' % kind)
	storage = read_member(doc, '_storage')
	title = span_text(storage, read_member(record, 'name', 'local'))
	prefix = span_text(storage, read_member(record, 'name', 'prefix'))
	if (prefix is not None) and (len(prefix) > 0) and (title is not None):
		title = '%s:%s' % (prefix, title)
	body = span_text(storage, read_member(record, 'value'))
	attributes = read_member(record, 'attributes')
	attributes = attributes.GetValueAsUnsigned(0) if attributes is not None else 0
	if name in ('element', 'document'):
		# Корень дерева имени не несёт: показываем его самим видом
		result = ('<%s>' % title) if (name == 'element') else name
		if attributes > 0:
			result += ', свойств: %d' % attributes
		##
		# Число детей считаем ОБХОДОМ цепочки, а не полем
		#
		# Записи держат первого да последнего ребёнка и соседей, а счётчика детей у них
		# нет вовсе: цепочку обходим, оградив обход длиною арены
		##
		result += ', детей: %d' % xml_children(nodes, record, count)
		return result
	if body is not None:
		return '%s "%s"' % (name, body)
	return '%s, узел %d' % (name, node_id)

def xml_children(nodes, record, limit):
	##
	# Считает детей узла обходом цепочки соседей
	##
	child = read_member(record, 'first')
	if child is None:
		return 0
	child = child.GetValueAsUnsigned(0)
	result = 0
	# Оградою служит длина арены: битая цепочка не должна вешать отладчик
	while (child < limit) and (result <= limit):
		result += 1
		following = read_member(nodes.GetChildAtIndex(child), 'next')
		if following is None:
			break
		child = following.GetValueAsUnsigned(0)
	return result

def span_text(storage, span):
	##
	# Выводит отрезок хранилища знаков, заданный смещением да длиной
	##
	if (storage is None) or (span is None):
		return None
	offset = read_member(span, 'offset')
	length = read_member(span, 'length')
	if (offset is None) or (length is None):
		return None
	return storage_slice(storage, offset.GetValueAsUnsigned(0), length.GetValueAsUnsigned(0))

# Виды значений TOML: `include/codec/toml/common.hpp`
TOML_TYPES = {
	0x00: 'NONE',            0x01: 'string',         0x02: 'integer',    0x03: 'float',
	0x04: 'bool',            0x05: 'offset-datetime', 0x06: 'local-datetime',
	0x07: 'local-date',      0x08: 'local-time',     0x09: 'array',      0x0A: 'table',
}

# Виды значений INI: `include/codec/ini/common.hpp`
INI_TYPES = { 0x00: 'NONE', 0x01: 'string', 0x02: 'array', 0x03: 'table' }

def deep_member(value, name, depth = 4):
	##
	# Ищет поле по имени, спускаясь сквозь БЕЗЫМЯННЫЕ объединения и структуры
	#
	# Строка libc++ у macOS держит своё устройство в безымянном объединении: поиск по
	# именам верхнего уровня не достаёт там ничего, и помощник длины отвечал нулём -
	# отказом, неотличимым от пустой строки. Обход этот нужен всякому чтению
	# внутренностей библиотеки стандарта, оттого заведён общим
	#
	# @note Глубина ограничена намеренно: устройство вместилищ мелко, а обход без
	#       предела на порченой памяти уходит в бесконечность
	##
	if (value is None) or (depth <= 0):
		return None
	field = value.GetChildMemberWithName(name)
	if (field is not None) and field.IsValid():
		return field
	raw = value.GetNonSyntheticValue()
	if (raw is not None) and raw.IsValid():
		for index in range(raw.GetNumChildren()):
			child = raw.GetChildAtIndex(index)
			if child is None:
				continue
			label = child.GetName() or ''
			if label == name:
				return child
			##
			# Спускаемся в безымянные И в служебные обёртки библиотеки
			#
			# Новая libc++ прячет устройство строки двумя слоями: безымянной структурой,
			# а под нею именованным `__rep_`. Спуск в одни лишь безымянные до него не
			# доходил, и длина всякой строки выходила нулём. Набивку и распределитель
			# обходим - смотреть в них нечего, а обход стоит времени
			##
			if (not label) or (label.startswith('__') and ('padding' not in label) and (label != '__alloc_')):
				found = deep_member(child, name, depth - 1)
				if found is not None:
					return found
	return None

def string_length(value):
	##
	# Выводит длину строки стандартной библиотеки полем её длины
	#
	# Имя поля у библиотек разное: `__size_` у libc++, `_M_string_length` у libstdc++
	##
	if value is None:
		return 0
	field = read_member(value, '_M_string_length')
	if field is not None:
		return field.GetValueAsUnsigned(0)
	##
	# У libc++ длина живёт в объединении, и короткая строка держит её иначе долгой
	#
	# Поиск идёт СПУСКОМ сквозь безымянные поля: у macOS устройство строки скрыто
	# безымянным объединением, и прежний поиск по именам верхнего уровня отвечал нулём
	# на всякую строку - отказом, неотличимым от пустоты
	##
	body = deep_member(value, '__l')
	if body is not None:
		##
		# Долгая строка помечена признаком, а у короткой длина лежит своим полем
		##
		long_form = deep_member(value, '__is_long_')
		if (long_form is None) or (long_form.GetValueAsUnsigned(0) != 0):
			size = deep_member(body, '__size_')
			if size is not None:
				return size.GetValueAsUnsigned(0)
	short = deep_member(value, '__s')
	if short is not None:
		size = deep_member(short, '__size_')
		if size is not None:
			##
			# У прежних сборок libc++ длина короткой строки сдвинута на разряд признака
			##
			count = size.GetValueAsUnsigned(0)
			return (count >> 1) if (count > 0x7F) else count
	field = deep_member(value, '__size_')
	return field.GetValueAsUnsigned(0) if (field is not None) else 0

def text_of(value):
	##
	# Выводит содержимое строки стандартной библиотеки, сняв кавычки краткого вида
	##
	if value is None:
		return None
	summary = value.GetSummary()
	if summary is None:
		return None
	body = summary.strip()
	if body.startswith('"') and body.endswith('"'):
		body = body[1:-1]
	return body

def toml_value_summary(value, internal_dict):
	##
	# Описатель значения TOML - `awh::codec::toml::Value`
	#
	# Значение это ВЛАДЕЮЩЕЕ, а не ссылка: содержимое лежит в нём самом. Толк описателя
	# иной - показать вид да содержимое вместо дюжины полей, из коих при всяком виде
	# заполнено одно
	##
	kind = read_member(value, '_type')
	if kind is None:
		return '<значение без вида>'
	name = TOML_TYPES.get(kind.GetValueAsUnsigned(0), 'вид 0x%02x' % kind.GetValueAsUnsigned(0))
	if name == 'table':
		names = read_member(value, '_names')
		return 'table, ключей: %d' % (names.GetNumChildren() if (names is not None) else 0)
	if name == 'array':
		items = read_member(value, '_items')
		return 'array, значений: %d' % (items.GetNumChildren() if (items is not None) else 0)
	if name == 'bool':
		flag = read_member(value, '_boolean')
		return 'true' if ((flag is not None) and (flag.GetValueAsUnsigned(0) != 0)) else 'false'
	if name == 'integer':
		number = read_member(value, '_integer')
		return str(number.GetValueAsSigned(0)) if (number is not None) else name
	if name == 'float':
		number = read_member(value, '_real')
		return str(number.GetValueAsUnsigned(0)) if (number is None) else str(float(number.GetValue()))
	if name == 'string':
		text = text_of(read_member(value, '_text'))
		return ('"%s"' % text) if (text is not None) else name
	if name.startswith('local') or name.startswith('offset'):
		##
		# Отметку времени собираем полями, а не записью
		#
		# Записи у неё нет вовсе: разобранная отметка живёт полями, а `_text` при ней пуст
		##
		return '%s %s' % (name, toml_stamp(read_member(value, '_stamp')))
	return name

def toml_stamp(stamp):
	##
	# Собирает отметку времени TOML из полей
	##
	if stamp is None:
		return '<нет полей>'
	##
	# Дата и время лежат ОТДЕЛЬНЫМИ полями отметки
	#
	# Отметка составная: `date` да `time`, а не полтора десятка полей вровень
	##
	def part(*names):
		field = read_member(stamp, *names)
		return field.GetValueAsUnsigned(0) if (field is not None) else 0
	return '%04d-%02d-%02d %02d:%02d:%02d' % (
		part('date', 'year'), part('date', 'month'), part('date', 'day'),
		part('time', 'hour'), part('time', 'minute'), part('time', 'second'))

def ini_value_summary(value, internal_dict):
	##
	# Описатель значения INI - `awh::codec::ini::Value`
	#
	# Видов у настроек всего четыре: записи там нетипизированы, и всякое значение есть
	# строка, перечень одноимённых свойств либо вместилище пар
	##
	kind = read_member(value, '_type')
	if kind is None:
		return '<значение без вида>'
	name = INI_TYPES.get(kind.GetValueAsUnsigned(0), 'вид 0x%02x' % kind.GetValueAsUnsigned(0))
	if name == 'table':
		names = read_member(value, '_names')
		return 'table, ключей: %d' % (names.GetNumChildren() if (names is not None) else 0)
	if name == 'array':
		items = read_member(value, '_items')
		return 'array, значений: %d' % (items.GetNumChildren() if (items is not None) else 0)
	if name == 'string':
		text = text_of(read_member(value, '_text'))
		return ('"%s"' % text) if (text is not None) else name
	return name

# Виды узлов ABC: `include/codec/abc/common.hpp`
ABC_KINDS = {
	0x00: 'NONE', 0x01: 'null',  0x02: 'bool', 0x03: 'number', 0x04: 'string',
	0x05: 'blob', 0x06: 'time',  0x07: 'uuid', 0x08: 'array',  0x09: 'map',
	0x0A: 'custom',
}

# Виды содержимого ABC: тот же перечень, что у прочих кодеков, но шире
ABC_TYPES = {
	0x00000000: 'UNDEFINED', 0x00000001: 'null',   0x00000002: 'bool',   0x00000004: 'string',
	0x00000008: 'blob',      0x00000010: 'array',  0x00000020: 'map',    0x00000040: 'time',
	0x00000080: 'uuid',      0x00000100: 'int8',   0x00000200: 'int16',  0x00000400: 'int32',
	0x00000800: 'int64',     0x00001000: 'uint8',  0x00002000: 'uint16', 0x00004000: 'uint32',
	0x00008000: 'uint64',    0x00010000: 'float',  0x00020000: 'double', 0x00040000: 'extended',
	0x00080000: 'decimal',   0x00100000: 'custom',
}

def abc_value_summary(value, internal_dict):
	##
	# Описатель значения ABC - `awh::codec::abc::Value`
	#
	# Вид у значения ДВОЙНОЙ: `_kind` сказывает род - число, строка, отображение, - а
	# `_type` уточняет ширину да знаковость числа. Показываем оба, ибо `number` без
	# ширины ничего не говорит о том, как значение уляжется в поток
	##
	kind = read_member(value, '_kind')
	if kind is None:
		return '<значение без вида>'
	kind = kind.GetValueAsUnsigned(0)
	name = ABC_KINDS.get(kind, 'вид 0x%02x' % kind)
	if name == 'map':
		keys = read_member(value, '_keys')
		return 'map, пар: %d' % (keys.GetNumChildren() if (keys is not None) else 0)
	if name == 'array':
		items = read_member(value, '_items')
		return 'array, значений: %d' % (items.GetNumChildren() if (items is not None) else 0)
	kind_of = read_member(value, '_type')
	kind_of = kind_of.GetValueAsUnsigned(0) if (kind_of is not None) else 0
	width = ABC_TYPES.get(kind_of, 'вид 0x%08x' % kind_of)
	if name in ('string', 'blob', 'uuid'):
		text = text_of(read_member(value, '_text'))
		if text is None:
			return name
		return ('"%s"' % text) if (name == 'string') else ('%s "%s"' % (name, text))
	if name == 'bool':
		number = read_member(value, '_number', 'flag')
		return 'true' if ((number is not None) and (number.GetValueAsUnsigned(0) != 0)) else 'false'
	if name == 'number':
		return abc_number(value, width)
	if name in ('null', 'NONE'):
		return name
	return '%s (%s)' % (name, width)

def abc_number(value, width):
	##
	# Выводит число ABC полем объединения, отвечающим ширине
	#
	# Объединение хранит все виды разом, и читать нужно ИМЕННО то поле, какое названо
	# видом: чтение целого там, где лежит дробное, выдало бы разряды его записи
	##
	if width in ('uint8', 'uint16', 'uint32', 'uint64'):
		number = read_member(value, '_number', 'natural')
		return ('%d (%s)' % (number.GetValueAsUnsigned(0), width)) if (number is not None) else width
	if width in ('int8', 'int16', 'int32', 'int64'):
		number = read_member(value, '_number', 'integer')
		return ('%d (%s)' % (number.GetValueAsSigned(0), width)) if (number is not None) else width
	if width in ('float', 'double'):
		number = read_member(value, '_number', 'real')
		return ('%s (%s)' % (number.GetValue(), width)) if (number is not None) else width
	##
	# Число, ни в один родной вид не вместимое, живёт ОКТЕТАМИ величины
	#
	# Разрядов у него больше, чем у восьми байтов, и объединение ему не годится вовсе.
	# Показывать эти октеты СТРОКОЮ нельзя: `_text` держит двоичную величину, а не
	# десятичную запись, и вывод её знаками дал бы мусор
	##
	if width in ('extended', 'decimal'):
		##
		# Длину величины берём у САМОЙ строки, а не счётом детей
		#
		# Краткий вид двоичной строки приходит с наращёнными последовательностями, и
		# длина его знаками величине не равна вовсе
		##
		count = string_length(read_member(value, '_text'))
		sign = read_member(value, '_negative')
		sign = '-' if ((sign is not None) and (sign.GetValueAsUnsigned(0) != 0)) else '+'
		if width == 'decimal':
			exponent = read_member(value, '_exponent')
			exponent = exponent.GetValueAsSigned(0) if (exponent is not None) else 0
			return '%s, октетов: %d, знак %s, порядок %d' % (width, count, sign, exponent)
		return '%s, октетов: %d, знак %s' % (width, count, sign)
	return width

def abc_node_summary(value, internal_dict):
	##
	# Описатель ссылки на узел документа ABC - `awh::codec::abc::Document::Value`
	#
	# Ссылка эта отличается от прочих: клейма поколения у неё НЕТ вовсе, есть лишь
	# граница вместилища. Устарелость ссылки описатель поймать не может, и обещать того
	# не станет
	##
	doc = read_member(value, '_doc')
	if (doc is None) or (doc.GetValueAsUnsigned(0) == 0):
		return 'недействительная ссылка'
	index = read_member(value, '_index')
	if index is None:
		return '<поля ссылки недоступны>'
	index = index.GetValueAsUnsigned(0)
	nodes = read_member(doc, '_nodes')
	if nodes is None:
		return '<перечень узлов недоступен>'
	count = nodes.GetNumChildren()
	if index >= count:
		return 'номер узла %d вне перечня из %d' % (index, count)
	node = nodes.GetChildAtIndex(index)
	kind = read_member(node, 'type')
	if kind is None:
		return '<узел без вида>'
	kind = kind.GetValueAsUnsigned(0)
	name = ABC_TYPES.get(kind, 'вид 0x%08x' % kind)
	content = read_member(node, 'content')
	length = content.GetChildAtIndex(0).GetValueAsUnsigned(0) if (content is not None) else 0
	offset = read_member(node, 'offset')
	offset = offset.GetValueAsUnsigned(0) if (offset is not None) else 0
	storage = read_member(doc, '_storage')
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
		# Двоичное содержимое показываем ДЛИНОЙ: знаками оно вышло бы мусором
		return '%s, октетов: %d' % (name, length)
	if name in ('extended', 'decimal', 'custom'):
		##
		# Величина лежит за ВОСЕМЬЮ байтами порядка, а не с самого смещения
		#
		# Первые восемь байтов куска держат порядок либо подвид, и величина начинается
		# лишь за ними: `offset + 8`
		##
		sign = read_member(node, 'negative')
		sign = '-' if ((sign is not None) and (sign.GetValueAsUnsigned(0) != 0)) else '+'
		return '%s, октетов: %d, знак %s' % (name, length, sign)
	number = node_number(node, name)
	if number is not None:
		return '%s (%s)' % (number, name)
	return '%s, узел %d' % (name, index)

# Виды сетевого адреса: `include/net/addr.hpp`
NET_TYPES = {
	0x00: 'NONE', 0x01: 'путь',  0x02: 'MAC',  0x03: 'URL',   0x04: 'IPv4',
	0x05: 'IPv6', 0x06: 'FQDN',  0x07: 'сеть IPv4', 0x08: 'сеть IPv6',
}

def net_addr_summary(value, internal_dict):
	##
	# Описатель сетевого адреса - `awh::net::Address`
	#
	# Адрес живёт ДВОИЧНЫМ буфером о шестнадцати байтах, и вывод его полями показывал бы
	# ряд чисел: разбираем буфер по виду адреса в привычную запись
	##
	kind = read_member(value, '_type')
	if kind is None:
		return '<адрес без вида>'
	kind = kind.GetValueAsUnsigned(0)
	name = NET_TYPES.get(kind, 'вид 0x%02x' % kind)
	octets = net_octets(value)
	if octets is None:
		return '%s, буфер пуст' % name
	if len(octets) == 4:
		return '%s %d.%d.%d.%d' % (name, octets[0], octets[1], octets[2], octets[3])
	if len(octets) == 6:
		return '%s %s' % (name, ':'.join(('%02x' % octet) for octet in octets))
	if len(octets) == 16:
		##
		# Запись IPv6 сокращаем самой длинной вереницей нулей
		#
		# Полная запись о восьми группах читается плохо, а сокращение - то самое, каким
		# адрес пишут всюду
		##
		##
		# Адрес IPv4, в IPv6 уложенный, показываем ЧЕТВЕРИЦЕЙ
		#
		# Запись `::ffff:102:304` законна, а читается плохо: отлаживающему сеть нужен
		# именно тот адрес IPv4, какой в нём лежит
		##
		if (octets[:10] == ([0] * 10)) and (octets[10] == 0xFF) and (octets[11] == 0xFF):
			return '%s ::ffff:%d.%d.%d.%d' % (name, octets[12], octets[13], octets[14], octets[15])
		groups = [((octets[i] << 8) | octets[i + 1]) for i in range(0, 16, 2)]
		text = net_ipv6(groups)
		zone = text_of(read_member(value, '_zone'))
		if zone:
			text = '%s%%%s' % (text, zone)
		return '%s %s' % (name, text)
	return '%s, байтов: %d' % (name, len(octets))

def net_octets(value):
	##
	# Выводит занятые байты двоичного буфера адреса
	##
	buffer = read_member(value, '_buffer')
	if buffer is None:
		return None
	size = read_member(buffer, '_size')
	data = read_member(buffer, '_data')
	if (size is None) or (data is None):
		return None
	size = size.GetValueAsUnsigned(0)
	if (size == 0) or (size > data.GetNumChildren()):
		return None
	return [data.GetChildAtIndex(i).GetValueAsUnsigned(0) for i in range(size)]

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

# Виды разобранной ссылки: `include/net/nwt.hpp`
NWT_TYPES = { 0x00: 'NONE', 0x01: 'MAC', 0x02: 'URL', 0x03: 'IPv4', 0x04: 'IPv6', 0x05: 'почта' }

def nwt_url_summary(value, internal_dict):
	##
	# Описатель разобранной ссылки - `awh::nwt_t::URL`
	#
	# Ссылка разобрана на десяток строк, и полями своими она нечитаема вовсе: собираем
	# из них ту самую запись, какую разбирали
	##
	kind = read_member(value, 'type')
	kind = kind.GetValueAsUnsigned(0) if (kind is not None) else 0
	name = NWT_TYPES.get(kind, 'вид 0x%02x' % kind)
	def part(field):
		text = text_of(read_member(value, field))
		return text if text else ''
	host = part('host')
	if not host:
		return '%s, пусто' % name
	##
	# Собираем запись слева направо, а не правкою уже собранной
	#
	# Прежде хозяин вставлялся в готовую строку со схемой, и вставка эта его теряла:
	# `user:***@` пропадал вовсе. Порядок сборки здесь тот же, каким запись читается
	##
	result = ''
	schema = part('schema')
	if schema:
		result += '%s://' % schema
	user = part('user')
	if user:
		##
		# Пароль показываем ЗВЁЗДОЧКАМИ, а не как есть
		#
		# Отладочный вывод расходится по журналам да снимкам кадра, и пароль в нём -
		# не то, что стоит показывать вместе с адресом
		##
		result += '%s%s@' % (user, ':***' if part('pass') else '')
	result += host
	port = read_member(value, 'port')
	port = port.GetValueAsUnsigned(0) if (port is not None) else 0
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

def regex_program_summary(value, internal_dict):
	##
	# Описатель порождённой программы выражения - `awh::regex::Program`
	#
	# Поля программы это перечни команд, разрядных карт да полос знаков, и вывод их
	# целиком заслоняет главное: сам шаблон, число команд и признаки, по каким выбирается
	# способ сличения
	##
	text = text_of(read_member(value, 'text'))
	instructions = sequence_count(read_member(value, 'instructions'))
	captures = read_member(value, 'captures')
	captures = captures.GetValueAsUnsigned(0) if (captures is not None) else 0
	marks = []
	##
	# Признаки показываем ТОЛЬКО поднятые
	#
	# Опущенный признак ничего не сказывает, а перечень из шести «нет» заслоняет те
	# один-два, ради каких описатель и заведён
	##
	for field, mark in (('plain', 'простое'), ('anchored', 'с якорем'), ('sweeping', 'обметает')):
		flag = read_member(value, field)
		if (flag is not None) and (flag.GetValueAsUnsigned(0) != 0):
			marks.append(mark)
	result = ('/%s/' % text) if text else 'программа'
	result += ', команд: %d' % instructions
	if captures > 0:
		result += ', захватов: %d' % captures
	if marks:
		result += ', ' + ', '.join(marks)
	return result

def regex_expression_summary(value, internal_dict):
	##
	# Описатель собранного выражения - `awh::regex::Expression`
	#
	# Выражение держит две программы - прямую да обратную, - перечень имён захватов и
	# порождённый код. Показываем шаблон прямой программы и то, чем сличение пойдёт
	##
	forward = read_member(value, 'forward')
	##
	# Шаблон показывается ТОЛЬКО у простого выражения
	#
	# Поле `text` держит полосу знаков простого выражения, а не сам шаблон: у выражения
	# со строением его нет вовсе, и обещать шаблон описатель не вправе. Вместо него
	# показываем число команд - оно сказывает о размере порождённого не меньше
	##
	text = text_of(read_member(forward, 'text')) if (forward is not None) else None
	plain = read_member(forward, 'plain') if (forward is not None) else None
	if text and (plain is not None) and (plain.GetValueAsUnsigned(0) != 0):
		result = '/%s/' % text
	else:
		result = 'выражение, команд: %d' % sequence_count(read_member(forward, 'instructions'))
	ready = read_member(value, 'ready')
	if (ready is None) or (ready.GetValueAsUnsigned(0) == 0):
		return '%s, НЕ СОБРАНО' % result
	names = read_member(value, 'names')
	names = names.GetNumChildren() if (names is not None) else 0
	if names > 0:
		result += ', имён: %d' % names
	##
	# Способ сличения важнее прочего: им объясняется и скорость, и поведение
	#
	# Порождённый код - самый быстрый путь, возвраты - самый медленный, а без обоих
	# сличение идёт лентой Pike
	##
	##
	# Умный указатель целым НЕ БЕРЁТСЯ
	#
	# `GetValueAsUnsigned` на `shared_ptr` отвечает нулём всегда, и проверка на пустоту
	# по нему объявляла ленту Pike даже там, где порождённый код собран. Смотреть нужно
	# поле самого указателя: `pointer` у LLDB, `__ptr_` у libc++
	##
	machine = shared_target(read_member(value, 'machine'))
	if machine != 0:
		result += ', порождённый код'
	else:
		backtracking = read_member(value, 'backtracking')
		if (backtracking is not None) and (backtracking.GetValueAsUnsigned(0) != 0):
			result += ', возвраты'
		else:
			result += ', лента Pike'
	return result

def shared_target(pointer):
	##
	# Выводит адрес, на какой смотрит умный указатель
	##
	if pointer is None:
		return 0
	for name in ('pointer', '__ptr_', '_M_ptr'):
		field = read_member(pointer, name)
		if field is not None:
			return field.GetValueAsUnsigned(0)
	return pointer.GetValueAsUnsigned(0)

def sequence_count(sequence):
	##
	# Выводит число записей своего перечня `regex::Sequence`
	##
	if sequence is None:
		return 0
	count = read_member(sequence, '_count')
	return count.GetValueAsUnsigned(0) if (count is not None) else 0

def queue_summary(value, internal_dict):
	##
	# Описатель очереди записей - `awh::Queue`
	#
	# Очередь живёт одним куском памяти да границами внутри него: полями своими она
	# сказывает числа, а не занятость. Показываем число записей и занятое ими место
	##
	count = read_member(value, '_range', 'count')
	count = count.GetValueAsUnsigned(0) if (count is not None) else 0
	begin = read_member(value, '_range', 'begin')
	end = read_member(value, '_range', 'end')
	begin = begin.GetValueAsUnsigned(0) if (begin is not None) else 0
	end = end.GetValueAsUnsigned(0) if (end is not None) else 0
	if count == 0:
		return 'очередь пуста'
	result = 'очередь, записей: %d, область: %s' % (count, bytes_text(end - begin))
	##
	# «Область» - это ЗАНЯТЫЙ КУСОК со служебными полями, а не сумма записей
	#
	# Первая редакция звала его «занято», и подпись эта врала: `size()` очереди отдаёт
	# длину БЛИЖАЙШЕЙ записи за вычетом уже прочитанного, а не всё занятое. Разошлось
	# втрое - 85 против 25, - и вскрылось лишь сличением с самим методом
	##
	head = queue_head(value, begin)
	if head is not None:
		result += ', к выдаче: %s' % bytes_text(head)
	##
	# Предел показываем ЛИШЬ когда он поставлен
	#
	# Ноль там значит «без предела», и печатать «предел: 0» значило бы сказать обратное
	##
	limit = read_member(value, '_max', 'memory')
	limit = limit.GetValueAsUnsigned(0) if (limit is not None) else 0
	if limit > 0:
		result += ', предел: %s' % bytes_text(limit)
	records = read_member(value, '_max', 'records')
	records = records.GetValueAsUnsigned(0) if (records is not None) else 0
	if records > 0:
		result += ', предел записей: %d' % records
	return result

def buffer_summary(value, internal_dict):
	##
	# Описатель буфера - `awh::Buffer`
	#
	# Буфер держит начало да конец занятого куска: показываем занятое и отведённое
	##
	begin = read_member(value, '_range', 'begin')
	end = read_member(value, '_range', 'end')
	reserved = read_member(value, '_range', 'reserved')
	begin = begin.GetValueAsUnsigned(0) if (begin is not None) else 0
	end = end.GetValueAsUnsigned(0) if (end is not None) else 0
	reserved = reserved.GetValueAsUnsigned(0) if (reserved is not None) else 0
	if end <= begin:
		return 'буфер пуст, отведено: %s' % bytes_text(reserved)
	return 'буфер, занято: %s из %s' % (bytes_text(end - begin), bytes_text(reserved))

def queue_head(value, begin):
	##
	# Выводит, сколько отдаст ближайшее чтение очереди
	#
	# Длина записи лежит в САМОМ буфере впереди её содержимого, а прочитанное считается
	# смещением чтения: то и другое нужно вместе
	##
	buffer = read_member(value, '_buffer')
	if buffer is None:
		return None
	##
	# Байты берём ДЕТЬМИ вектора, а не чтением памяти по его полю
	#
	# Поле начала данных у отладчика заслонено собственным описателем вектора: LLDB
	# показывает его знаками и внутренних полей не отдаёт вовсе
	##
	count = buffer.GetNumChildren()
	if count < (begin + 8):
		return None
	length = 0
	for index in range(8):
		octet = buffer.GetChildAtIndex(begin + index).GetValueAsUnsigned(0)
		length |= ((octet & 0xFF) << (index * 8))
	offset = read_member(value, '_range', 'offset')
	offset = offset.GetValueAsUnsigned(0) if (offset is not None) else 0
	return (length - min(length, offset))

def bytes_text(count):
	##
	# Выводит размер приставкой, чтобы четыре мегабайта не читались числом 4194304
	##
	if count < 1024:
		return '%d Б' % count
	if count < (1024 * 1024):
		return '%.1f КБ' % (count / 1024.0)
	return '%.1f МБ' % (count / (1024.0 * 1024.0))

def csv_document_summary(value, internal_dict):
	##
	# Описатель таблицы CSV - `awh::codec::csv::Document`
	#
	# Таблица живёт двумя хранилищами знаков да перечнями указаний в них: полями своими
	# она сказывает лишь длины. Показываем размер таблицы и имена столбцов, коли они есть
	##
	records = read_member(value, '_records')
	fields = read_member(value, '_fields')
	header = read_member(value, '_header')
	rows = records.GetNumChildren() if (records is not None) else 0
	cells = fields.GetNumChildren() if (fields is not None) else 0
	titles = header.GetNumChildren() if (header is not None) else 0
	if (rows == 0) and (cells == 0):
		return 'таблица пуста'
	##
	# Число столбцов считаем ПО ЗАГОЛОВКУ, а коли его нет - по первой записи
	#
	# Строки CSV равной ширины не обещают, и «столбцов» здесь - ширина заголовка либо
	# первой записи, а не общее свойство таблицы
	##
	columns = titles
	if (columns == 0) and (rows > 0):
		first = records.GetChildAtIndex(0).GetValueAsUnsigned(0)
		following = records.GetChildAtIndex(1).GetValueAsUnsigned(0) if (rows > 1) else cells
		columns = (following - first)
	result = 'таблица, записей: %d, столбцов: %d, полей: %d' % (rows, columns, cells)
	if titles > 0:
		##
		# Имена столбцов лежат в СВОЁМ хранилище, отдельном от полей
		#
		# Хранилищ у таблицы два: `_names` под заголовок и `_storage` под поля. Резать
		# заголовок по хранилищу полей значило бы выдать чужие знаки
		##
		names = read_member(value, '_names')
		shown = []
		for index in range(min(titles, 4)):
			text = span_text(names, header.GetChildAtIndex(index))
			shown.append(text if text else '?')
		if titles > 4:
			shown.append('…')
		result += ' [%s]' % ', '.join(shown)
	return result

def alloc_cache_summary(value, internal_dict):
	##
	# Описатель склада разрядов потока - `awh::alloc::Cache`
	#
	# Склад держит девяносто шесть списков свободных блоков, и вывод их подряд
	# бесполезен: непустых там обычно единицы. Показываем занятое, предел и то, какие
	# разряды заселены
	##
	bytes_held = atomic_value(read_member(value, '_bytes'))
	limit = atomic_value(read_member(value, '_limit'))
	kept = atomic_value(read_member(value, '_kept'))
	lists = read_member(value, '_lists')
	##
	# Считаем ЗАСЕЛЁННЫЕ разряды, а не сумму блоков
	#
	# Сумма ничего не говорит о раскладе, а перечень заселённых разрядов сразу
	# показывает, чем именно поток пользуется
	##
	busy = []
	total = 0
	if lists is not None:
		for index in range(lists.GetNumChildren()):
			count = read_member(lists.GetChildAtIndex(index), 'count')
			count = count.GetValueAsUnsigned(0) if (count is not None) else 0
			if count > 0:
				total += count
				if len(busy) < 6:
					busy.append('%d:%d' % (index, count))
	result = 'склад, занято: %s' % bytes_text(bytes_held)
	if limit > 0:
		result += ' из %s' % bytes_text(limit)
	if kept > 0:
		result += ', придержано: %s' % bytes_text(kept)
	if total > 0:
		tail = '…' if (len(busy) == 6) else ''
		result += ', блоков: %d [%s%s]' % (total, ' '.join(busy), tail)
	else:
		result += ', списки пусты'
	##
	# Учёт расхода снимается признаком сборки, и тогда счётчик стоит на нуле
	#
	# Нуль этот - не пустой склад, а снятый учёт: `AWH_ALLOC_NO_ACCOUNTING`
	##
	tally = read_member(value, '_tally')
	if (tally is not None) and (atomic_value(tally) == 0) and (total > 0):
		result += ' (учёт снят либо склад свеж)'
	return result

def atomic_value(value):
	##
	# Выводит содержимое атомарного поля
	#
	# У атомарного поля значение лежит внутри, а не в нём самом: имена внутренние у
	# библиотек разные, и брать нужно первого ребёнка, коли имя не найдено
	##
	if value is None:
		return 0
	##
	# Пути перебираются ЦЕЛИКОМ, а не по одному имени
	#
	# У `std::atomic <bool>` в libstdc++ значение лежит уровнем глубже -
	# `_M_base._M_i`, - и перебор одних верхних имён его не достаёт. Запасной путь
	# «первый ребёнок» упирался там в СОСТАВНОЕ поле `_M_base` и отвечал нулём: пул с
	# поднятым признаком остановки вышел бы живым. Найдено на редакции для GDB, где тот
	# же промах давал ложную истину, и потому был виден
	##
	for path in (('_M_i',), ('__a_',), ('__val_',), ('_M_base', '_M_i'), ('__a_', '__val_')):
		field = value
		for name in path:
			field = read_member(field, name) if (field is not None) else None
		if field is not None:
			return field.GetValueAsUnsigned(0)
	##
	# Берём первого ребёнка, но лишь ПРОСТОГО: у составного числа нет, и молчаливый
	# нуль от него не отличить от честного нуля
	##
	if value.GetNumChildren() > 0:
		child = value.GetChildAtIndex(0)
		if child.GetNumChildren() == 0:
			return child.GetValueAsUnsigned(0)
	return value.GetValueAsUnsigned(0)

def quic_fifo_summary(value, internal_dict):
	##
	# Описатель очереди блоков QUIC - `awh::quic::chunked_fifo_t`
	#
	# Очередь копит принятое цепочкой блоков по 16 КБ, беря их из пула. Смотрят у неё
	# ровно две вещи: сколько данных накоплено и на сколько блоков они разошлись -
	# второе говорит о том, растёт ли цепочка сверх ожидаемого
	#
	# @note Логический размер берётся ПОЛЕМ `_bytes`, а не суммой длин блоков: хвостовой
	#       блок обычно неполон, и сумма ёмкостей ввела бы в заблуждение
	##
	bytes_held = read_member(value, '_bytes')
	bytes_held = bytes_held.GetValueAsUnsigned(0) if (bytes_held is not None) else 0
	blocks = read_member(value, '_blocks')
	##
	# Число блоков берётся ДЕТЬМИ, а не полем `_count`
	#
	# Цепочка эта - обычный `std::vector`, и поля такого у неё нет вовсе. Первая
	# редакция звала здесь `sequence_count`, писанный под свой перечень `regex::Sequence`,
	# и тот отвечал нулём: «накоплено 39.1 КБ, блоков: 0» - число, невозможное само по
	# себе, и лишь потому находка и вскрылась
	##
	count = blocks.GetNumChildren() if (blocks is not None) else 0
	if (count == 0) and (bytes_held == 0):
		return 'очередь блоков пуста'
	result = 'очередь блоков, накоплено: %s, блоков: %d' % (bytes_text(bytes_held), count)
	##
	# Показываем среднее наполнение блока
	#
	# Цепочка в сотни блоков при малом накопленном - признак того, что блоки берутся из
	# пула, но не заполняются: беда не в объёме, а в раскладе
	##
	if count > 0:
		result += ' (в среднем по %s)' % bytes_text(bytes_held // count)
	return result

def quic_small_vector_summary(value, internal_dict):
	##
	# Описатель вектора с коротким запасом - `awh::quic::small_vector`
	#
	# Вектор держит первые N элементов у себя, а сверх того уходит в кучу. Уход этот и
	# есть то, ради чего на него смотрят: он означает выделение памяти там, где его не
	# ждали
	#
	# @note Место хранения определяется СЛИЧЕНИЕМ указателя с адресом своего запаса, а
	#       не отдельным признаком: признака такого у вектора нет вовсе, и иначе о нём
	#       не судить
	##
	size = read_member(value, '_size')
	size = size.GetValueAsUnsigned(0) if (size is not None) else 0
	cap = read_member(value, '_cap')
	cap = cap.GetValueAsUnsigned(0) if (cap is not None) else 0
	data = read_member(value, '_data')
	storage = read_member(value, '_storage')
	where = 'место неизвестно'
	if (data is not None) and (storage is not None):
		address = data.GetValueAsUnsigned(0)
		own = storage.GetLoadAddress()
		if (address != 0) and (own != lldb.LLDB_INVALID_ADDRESS):
			where = 'свой запас' if (address == own) else 'куча'
	if size == 0:
		return 'вектор пуст, ёмкость: %d (%s)' % (cap, where)
	return 'вектор, элементов: %d из %d (%s)' % (size, cap, where)

def map_count(value):
	##
	# Выводит число записей отображения стандартной библиотеки
	#
	# Счёт детьми здесь негоден: LLDB отдаёт детьми ЗАПИСИ лишь при своём описателе
	# отображения, а поле счёта надёжно у обеих библиотек - `_M_h._M_element_count`
	# у libstdc++ и `__table_.__p2_` у libc++
	##
	if value is None:
		return 0
	for path in (('_M_h', '_M_element_count'), ('__table_', '__p2_')):
		current = value
		for name in path:
			current = read_member(current, name) if (current is not None) else None
		if current is not None:
			count = current.GetValueAsUnsigned(0)
			if count > 0:
				return count
	##
	# Запасным путём берём детей: пустое отображение и отсутствие поля с виду одно и
	# то же, и лишь дети разводят их порознь
	##
	return value.GetNumChildren()

def callback_summary(value, internal_dict):
	##
	# Описатель набора откликов - `awh::Callback`
	#
	# Набор держит отклики отображением по опознавателю. Смотрят у него ровно число
	# подписок: пустой набор там, где ждали подписки, и есть частая беда
	##
	count = map_count(read_member(value, '_callbacks'))
	if count == 0:
		return 'набор откликов пуст'
	return 'набор откликов, подписок: %d' % count

def threadpool_summary(value, internal_dict):
	##
	# Описатель пула потоков - `awh::Threadpool`
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
	wanted = read_member(value, '_threads')
	wanted = wanted.GetValueAsUnsigned(0) if (wanted is not None) else 0
	workers = read_member(value, '_workers')
	workers = workers.GetNumChildren() if (workers is not None) else 0
	##
	# Очередь задач - `std::queue`, и длина её лежит у вложенного вместилища: `c` у
	# libstdc++ и `__c` у libc++
	##
	tasks = 0
	for name in ('c', '__c'):
		holder = read_member(value, '_tasks', name)
		if holder is not None:
			tasks = holder.GetNumChildren()
			break
	result = 'пул, работников: %d' % workers
	if wanted != workers:
		result += ' (заказано: %d)' % wanted
	result += ', задач: %d' % tasks
	if atomic_value(read_member(value, '_stop')) != 0:
		result += ', ОСТАНОВЛЕН'
	if atomic_value(read_member(value, '_wait')) != 0:
		result += ', ждёт завершения'
	return result

def __lldb_init_module(debugger, internal_dict):
	##
	# Заводит описатели при подключении файла
	##
	debugger.HandleCommand(
		'type summary add -F awh.json_value_summary -x "^awh::codec::json::Document::Value$"')
	debugger.HandleCommand(
		'type summary add -F awh.yaml_value_summary -x "^awh::codec::yaml::Document::Value$"')
	debugger.HandleCommand(
		'type summary add -F awh.xml_node_summary -x "^awh::codec::xml::Node$"')
	debugger.HandleCommand(
		'type summary add -F awh.toml_value_summary -x "^awh::codec::toml::Value$"')
	debugger.HandleCommand(
		'type summary add -F awh.ini_value_summary -x "^awh::codec::ini::Value$"')
	debugger.HandleCommand(
		'type summary add -F awh.abc_value_summary -x "^awh::codec::abc::Value$"')
	debugger.HandleCommand(
		'type summary add -F awh.abc_node_summary -x "^awh::codec::abc::Document::Value$"')
	debugger.HandleCommand(
		'type summary add -F awh.net_addr_summary -x "^awh::Network_Address$"')
	debugger.HandleCommand(
		'type summary add -F awh.nwt_url_summary -x "^awh::Network_Types::URL$"')
	debugger.HandleCommand(
		'type summary add -F awh.regex_program_summary -x "^awh::regex::Program$"')
	debugger.HandleCommand(
		'type summary add -F awh.regex_expression_summary -x "^awh::regex::Expression$"')
	debugger.HandleCommand(
		'type summary add -F awh.csv_document_summary -x "^awh::codec::csv::Document$"')
	debugger.HandleCommand(
		'type summary add -F awh.alloc_cache_summary -x "^awh::alloc::Cache$"')
	debugger.HandleCommand('type summary add -F awh.queue_summary -x "^awh::Queue$"')
	debugger.HandleCommand('type summary add -F awh.buffer_summary -x "^awh::Buffer$"')
	##
	# Образец вектора берётся БЕЗ якоря конца
	#
	# `small_vector` - шаблон, и полное имя его несёт доводы шаблона в угловых скобках.
	# Образец с якорем `$` не совпал бы ни с одним воплощением
	##
	debugger.HandleCommand(
		'type summary add -F awh.quic_fifo_summary -x "^awh::quic::chunked_fifo_t$"')
	debugger.HandleCommand(
		'type summary add -F awh.quic_small_vector_summary -x "^awh::quic::small_vector<"')
	debugger.HandleCommand('type summary add -F awh.callback_summary -x "^awh::Callback$"')
	debugger.HandleCommand(
		'type summary add -F awh.threadpool_summary -x "^awh::Threadpool$"')
	print('AWH: описатели значений заведены')
