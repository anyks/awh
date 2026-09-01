##
# @file helpers-gdb.py
# @date 2026-09-01
#
# @license{LicenseRef-AWH-1.0}
#
# @author Yuriy Lobarev
#
# @email forman@anyks.com
#
# @brief Сплошная проверка помощников описателей GDB на известных ответах
#
# @details Помощники читают внутренности библиотеки стандарта, а имена полей у libc++ и
#          libstdc++ разные. Отказ такого чтения выходит НУЛЁМ либо мусором соседних
#          байтов, а не ошибкой, и от честного нуля его не отличить
#
# @note Щуп собирается из того же `helpers.cpp`, что и редакция для LLDB: сличение двух
#       редакций на ОДНОМ щупе и есть весь толк проверки
#
# @note Проверяемые поля лежат в СЕРЕДИНЕ структуры, а соседями им заведомо ненулевые
#       значения: в хвосте мусор совпадает с ожидаемым
#
# @copyright Copyright © 2026
#
##
import gdb, os, importlib.util

##
# Берём редакцию описателей соседним файлом, а не по имени модуля: имя её несёт дефис
##
source = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'awh-gdb.py')
spec = importlib.util.spec_from_file_location('awhgdb', source)
awh = importlib.util.module_from_spec(spec)
spec.loader.exec_module(awh)

frame = gdb.selected_frame()
probe = frame.read_var('probe')

##
# Ожидания совпадают с редакцией для LLDB дословно
##
cases = [
	('string_parts короткая', awh.string_parts(probe['small'])[1], 8),
	('string_parts длинная', awh.string_parts(probe['large'])[1], 200),
	('text_of короткая', awh.text_of(probe['small']), 'Юрий'),
	('text_of длинная', (awh.text_of(probe['large']) or '')[:5], 'wwwww'),
	('atomic_value size_t', awh.atomic_value(probe['counter']), 230700),
	('atomic_value true', awh.atomic_value(probe['raised']), 1),
	('atomic_value false', awh.atomic_value(probe['lowered']), 0),
	('map_count', awh.map_count(probe['pairs']), 3),
	('count_of вектора', awh.count_of(probe['numbers']), 5),
]
bad = 0
for name, got, expect in cases:
	ok = (got == expect)
	bad += (0 if ok else 1)
	print('%-24s ожидалось %-10s получено %-10s %s' % (name, repr(expect), repr(got), 'СХОДИТСЯ' if ok else 'РАСХОЖДЕНИЕ'))
print('ИТОГ: расхождений %d из %d' % (bad, len(cases)))
