##
# @file helpers.py
# @date 2026-09-01
#
# @license{LicenseRef-AWH-1.0}
#
# @author Yuriy Lobarev
#
# @email forman@anyks.com
#
# @brief Сплошная проверка помощников описателей LLDB на известных ответах
#
# @details Помощники читают внутренности библиотеки стандарта, а имена полей у libc++ и
#          libstdc++ разные. Отказ такого чтения выходит НУЛЁМ либо мусором соседних
#          байтов, а не ошибкой, и от честного нуля его не отличить. Проверка эта ставит
#          известные значения и сличает с ними
#
# @note Проверяемые поля лежат в СЕРЕДИНЕ структуры щупа, а соседями им заведомо
#       ненулевые значения. В хвосте мусор совпадает с ожидаемым: щуп с двумя
#       атомарными признаками в конце подтвердил неверный помощник
#
# @copyright Copyright © 2026
#
##
import lldb, sys, os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import awh

def check(debugger, command, result, internal_dict):
	frame = debugger.GetSelectedTarget().GetProcess().GetSelectedThread().GetSelectedFrame()
	probe = frame.FindVariable('probe')
	def member(name):
		return probe.GetChildMemberWithName(name)
	cases = [
		('string_length короткая', awh.string_length(member('small')), 8),
		('string_length длинная', awh.string_length(member('large')), 200),
		('text_of короткая', awh.text_of(member('small')), 'Юрий'),
		('text_of длинная', (awh.text_of(member('large')) or '')[:5], 'wwwww'),
		('atomic_value size_t', awh.atomic_value(member('counter')), 230700),
		('atomic_value true', awh.atomic_value(member('raised')), 1),
		('atomic_value false', awh.atomic_value(member('lowered')), 0),
		('map_count', awh.map_count(member('pairs')), 3),
		('shared_target есть', awh.shared_target(member('owned')) is not None, True),
		('shared_target пуст', awh.shared_target(member('empty')), 0),
		('число элементов вектора', member('numbers').GetNumChildren(), 5),
	]
	bad = 0
	for name, got, expect in cases:
		ok = (got == expect)
		bad += (0 if ok else 1)
		print('%-26s ожидалось %-10s получено %-10s %s' % (name, repr(expect), repr(got), 'СХОДИТСЯ' if ok else 'РАСХОЖДЕНИЕ'))
	print('ИТОГ: расхождений %d из %d' % (bad, len(cases)))

def __lldb_init_module(debugger, internal_dict):
	debugger.HandleCommand('command script add -f helpers.check checkhelpers')
