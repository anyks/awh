#!/usr/bin/env python3
"""Сличение разбора JSON модулем awh::codec::json с эталоном.

Судит выдачу стенда `verify` по двум основаниям:
  * ожиданиям набора JSONTestSuite — имена текстов начинаются с `y_` (обязан
    приниматься), `n_` (обязан отвергаться) и `i_` (отдан на усмотрение);
  * согласию значений с модулем `json` языка Python — перезапись принятого дерева
    обязана читаться тем же самым значением, что и исходный текст.

Возвращает ненулевой код при обнаружении хотя бы одного расхождения.
"""

import json
import math
import sys

# Поднимаем предел глубины рекурсии сличения
#
# Корпус содержит тексты о пятистах уровнях вложенности, а сличение значений идёт вглубь
# по дереву: предел Python по умолчанию на них исчерпывается
sys.setrecursionlimit(20000)


def equal(left, right):
	"""Сличает два значения, разобранные из JSON, с оглядкой на дробные числа."""
	# Если оба значения являются логическими
	if isinstance(left, bool) or isinstance(right, bool):
		return (left is right)
	# Если оба значения являются числами
	if isinstance(left, (int, float)) and isinstance(right, (int, float)):
		# Если оба числа не являются числами вовсе
		if isinstance(left, float) and isinstance(right, float):
			if math.isnan(left) and math.isnan(right):
				return True
		# Сличаем числа с допуском на округление записи
		if (left == right):
			return True
		try:
			return math.isclose(float(left), float(right), rel_tol = 1e-15, abs_tol = 0.0)
		except OverflowError:
			return False
	# Если оба значения являются перечнями
	if isinstance(left, list) and isinstance(right, list):
		return ((len(left) == len(right)) and all(equal(a, b) for a, b in zip(left, right)))
	# Если оба значения являются отображениями
	if isinstance(left, dict) and isinstance(right, dict):
		if (left.keys() != right.keys()):
			return False
		return all(equal(left[k], right[k]) for k in left)
	# Сличаем прочие значения дословно
	return ((type(left) is type(right)) and (left == right))


def main():
	# Разбираем параметры вызова
	if (len(sys.argv) < 3):
		print("Вызов: compare.py <выдача стенда> <каталог корпуса>", file = sys.stderr)
		return 2
	dump, corpus = sys.argv[1], sys.argv[2].rstrip("/")
	# Счётчики исходов сличения
	counts = {"верно": 0, "расхождение": 0, "пропущено": 0}
	# Перечень обнаруженных расхождений
	issues = []
	# Выполняем перебор всех строк выдачи стенда
	with open(dump, encoding = "utf-8", errors = "surrogateescape") as source:
		for line in source:
			parts = line.rstrip("\n").split("\t", 3)
			if (len(parts) < 3):
				continue
			name, verdict, stable = parts[0], parts[1], parts[2]
			written = (parts[3] if (len(parts) > 3) else "")
			# Получаем ожидание набора по имени текста
			#
			# Ожидание заявлено первою буквою имени вместе со знаком подчёркивания за нею:
			# `y_` — обязан приниматься, `n_` — обязан отвергаться, `i_` — отдан на
			# усмотрение. Имена иного вида ожидания не несут вовсе, и судить их по первой
			# букве нельзя: набор преобразований весь состоит из имён вида `number_…`
			kind = (name[0] if ((len(name) > 1) and (name[1] == "_")) else "")
			# Сличаем исход с ожиданием набора
			if ((kind == "y") and (verdict != "accept")):
				issues.append("%s: обязан приниматься, а отвергнут" % name)
			elif ((kind == "n") and (verdict != "reject")):
				issues.append("%s: обязан отвергаться, а принят" % name)
			# Сличаем устойчивость выдачи к нарезке текста на куски
			if (stable != "stable"):
				issues.append("%s: исход разбора зависит от нарезки текста на куски" % name)
			# Сличаем значения с эталоном
			if (verdict != "accept"):
				counts["верно"] += 1
				continue
			try:
				with open("%s/%s" % (corpus, name), "rb") as origin:
					expected = json.loads(origin.read().decode("utf-8", errors = "strict"))
			except Exception:
				# Эталон текста не принимает — сличать значения не с чем
				counts["пропущено"] += 1
				continue
			try:
				actual = json.loads(written)
			except Exception as error:
				issues.append("%s: перезапись эталоном не читается (%s)" % (name, error))
				counts["расхождение"] += 1
				continue
			if not equal(expected, actual):
				issues.append("%s: значение разошлось с эталоном" % name)
				counts["расхождение"] += 1
				continue
			counts["верно"] += 1
	# Выводим итоги сличения
	print("сличено текстов: %d, верно: %d, расхождений: %d, пропущено: %d" % (
		sum(counts.values()), counts["верно"], counts["расхождение"], counts["пропущено"]
	))
	# Выводим обнаруженные расхождения
	for issue in issues:
		print("  %s" % issue)
	# Если сличать было нечего
	#
	# Отчёт «расхождений нет» при нуле сличённых текстов есть молчаливый пропуск:
	# он неотличим от успеха, а означает лишь, что корпус не забран либо пуст
	if (sum(counts.values()) == 0):
		print("  ОТКАЗ: сличать нечего - корпус пуст либо не забран")
		return 2
	return (1 if issues else 0)


if (__name__ == "__main__"):
	sys.exit(main())
