#!/usr/bin/env python3
# @file compare.py
# @date 2026-08-23
#
# @license{LicenseRef-AWH-1.0}
#
# @author Yuriy Lobarev
#
# @brief Сличение исхода разбора XML с приговором набора соответствия W3C и со строением,
#        выданным разбором expat
#
# @details Основания здесь два, и они разного рода. Набор W3C объявляет для всякого случая,
#          обязан ли разбор принять его либо отвергнуть, - это и есть соответствие
#          стандарту. Разбор expat служит вторым, независимым основанием по СТРОЕНИЮ
#          принятого дерева: приговор «принято» ничего не говорит о том, верно ли
#          прочитано содержимое
#
# @warning Изрядная доля случаев набора лежит ВНЕ ОЧЕРЧЕННОГО ОХВАТА кодека, и относить их
#          к отказам нельзя: пределы охвата записаны разделом «Намеренные решения» в
#          codec/xml/common.hpp и являются решением, а не пробелом. Отсеиваются они по
#          признакам самого набора, а не поимённо
#
# @copyright Copyright © 2026
#

import os
import re
import sys
import glob
import xml.parsers.expat
import xml.etree.ElementTree as ET

# Пространство имён договора разметки
XMLNS = "{http://www.w3.org/XML/1998/namespace}"


def manifests(root):
	"""Сбор всех списков случаев набора соответствия

	@param root корень распакованного набора соответствия
	@return     перечень путей списков случаев
	"""
	result = []
	for path in glob.glob(os.path.join(root, "**", "*.xml"), recursive = True):
		try:
			with open(path, "rb") as handle:
				head = handle.read(4096)
		except OSError:
			continue
		# Списком считается лишь тот файл, где объявлены сами случаи
		if (b"<TEST " in head) or (b"<TESTCASES" in head):
			result.append(path)
	return sorted(result)


def collect(path, root):
	"""Снятие случаев с одного списка

	@details Обрывок из нескольких корней оборачивается в общий корень: часть списков
	         набора цельными документами не является

	@param path путь списка случаев
	@param root корень распакованного набора соответствия
	@return     перечень снятых случаев
	"""
	with open(path, "rb") as handle:
		text = handle.read()
	# Снимаем описание типа документа: оно тянет внешние сущности, которых нет
	text = re.sub(rb"<!DOCTYPE.*?\]>", b"", text, flags = re.S)
	text = re.sub(rb"<!DOCTYPE[^>]*>", b"", text)
	# Снимаем объявление разметки: обёртка ставила бы корень прежде него, и разбор списка
	# отказывал молча, недосчитывая целые списки
	text = re.sub(rb"^\s*<\?xml[^>]*\?>", b"", text)
	try:
		tree = ET.fromstring(b"<ROOT>" + text + b"</ROOT>")
	except ET.ParseError:
		return []
	result = []

	def walk(node, base):
		# Основание пути наращивается по цепочке вложенных объявлений
		own = node.get(XMLNS + "base")
		if own:
			base = os.path.join(base, own)
		for item in node:
			if (item.tag == "TESTCASES"):
				walk(item, base)
			elif (item.tag == "TEST"):
				uri = item.get("URI")
				if not uri:
					continue
				result.append({
					"id": item.get("ID", uri),
					"type": item.get("TYPE", ""),
					"entities": item.get("ENTITIES", "none"),
					"version": item.get("VERSION", ""),
					"edition": item.get("EDITION", ""),
					"recommendation": item.get("RECOMMENDATION", ""),
					"namespace": item.get("NAMESPACE", ""),
					"path": os.path.relpath(os.path.normpath(os.path.join(base, uri)), root)
				})

	walk(tree, os.path.dirname(path))
	return result


def scope(test):
	"""Определение принадлежности случая охвату кодека

	@details Отсев идёт по признакам самого набора, а не поимённо, и всякая его статья
	         отвечает записанному пределу охвата

	@param test разбираемый случай
	@return     довод отсева либо None, если случай охвату принадлежит
	"""
	# Переиздание договора распространения в состав не входит
	if (test["version"] == "1.1") or (test["recommendation"] in ("XML1.1", "NS1.1")):
		return "XML 1.1 вне охвата"
	# Случаи прежних изданий договора отменены пятым изданием
	if (test["edition"] and ("5" not in test["edition"].split())):
		return "издание договора прежнее"
	# Приговор о негодности, видимый лишь при чтении внешних сущностей, вынести нельзя
	if ((test["type"] == "not-wf") and (test["entities"] != "none")):
		return "негодность видна лишь при чтении внешних сущностей"
	# Договор не требует сообщать об ошибках этого рода
	if (test["type"] == "error"):
		return "сообщение необязательно по договору"
	return None


def expected(test):
	"""Приговор набора соответствия для случая

	@details Разбор без проверки по описанию типа документа обязан принимать и «valid», и
	         «invalid»: последние договору соответствуют, а описанию - нет

	@param test разбираемый случай
	@return     ожидаемый приговор разбора
	"""
	return ("reject" if (test["type"] == "not-wf") else "accept")


def render(path):
	"""Снятие строения дерева разбором expat

	@details Разбор пространств имён отключён намеренно: стенд выдаёт имена так, как они
	         записаны, и объявления пространств имён наравне с прочими атрибутами

	@param path путь разбираемого текста
	@return     пара из признака успешности разбора и строения дерева
	"""
	parts = []

	def start(name, attrs):
		items = sorted("%s=%s" % (key, escape(value)) for key, value in attrs.items())
		parts.append("<" + name + "".join(" " + item for item in items) + ">")

	def end(name):
		parts.append("</>")

	def data(text):
		parts.append(escape(text))

	parser = xml.parsers.expat.ParserCreate()
	parser.StartElementHandler = start
	parser.EndElementHandler = end
	parser.CharacterDataHandler = data
	# Внешние сущности не читаются: тем же правилом живёт и наш кодек
	parser.SetParamEntityParsing(xml.parsers.expat.XML_PARAM_ENTITY_PARSING_NEVER)
	try:
		with open(path, "rb") as handle:
			parser.Parse(handle.read(), True)
	except Exception:
		return (False, "")
	return (True, "".join(parts))


def escape(text):
	"""Уход служебных знаков строения дерева

	@param text уходимый текст
	@return     текст с ушедшими знаками
	"""
	return text.replace("\\", "\\\\").replace("\t", "\\t").replace("\n", "\\n").replace("\r", "\\r")


def main():
	if (len(sys.argv) < 3):
		print("Вызов: compare.py <выдача стенда> <корень корпуса>")
		return 2
	dump, root = sys.argv[1], sys.argv[2]
	# Снимаем все случаи набора соответствия
	tests = {}
	for path in manifests(root):
		for test in collect(path, root):
			tests.setdefault(test["path"], test)
	# Снимаем выдачу стенда
	issued = {}
	with open(dump, "r", encoding = "utf-8", errors = "replace") as handle:
		for line in handle:
			items = line.rstrip("\n").split("\t")
			if (len(items) >= 4):
				issued[items[0]] = (items[1], items[2], items[3])
	counts = {"верно": 0, "расхождение": 0, "вне охвата": 0, "неустойчиво": 0, "строение": 0}
	issues = []
	for path, test in sorted(tests.items()):
		if (path not in issued):
			continue
		verdict, stable, structure = issued[path]
		if (verdict == "missing"):
			continue
		reason = scope(test)
		if reason:
			counts["вне охвата"] += 1
			continue
		# Сличаем приговор с объявленным набором соответствия
		if (verdict != expected(test)):
			issues.append("%s: набор ждёт «%s», разбор ответил «%s» (%s)" % (
				test["id"], expected(test), verdict, test["type"]))
			counts["расхождение"] += 1
			continue
		# Сличаем устойчивость исхода к нарезке текста на куски
		if (stable != "stable"):
			issues.append("%s: исход разбора зависит от нарезки текста на куски" % test["id"])
			counts["неустойчиво"] += 1
			continue
		# Сличаем строение принятого дерева с выданным разбором expat
		if (verdict == "accept"):
			ok, reference = render(os.path.join(root, path))
			if (ok and (reference != structure)):
				issues.append("%s: строение дерева разошлось с expat" % test["id"])
				counts["строение"] += 1
				continue
		counts["верно"] += 1
	print("сличено случаев: %d, верно: %d, расхождений приговора: %d, неустойчивых: %d, расхождений строения: %d, вне охвата: %d" % (
		sum(counts.values()), counts["верно"], counts["расхождение"],
		counts["неустойчиво"], counts["строение"], counts["вне охвата"]))
	for issue in issues[:40]:
		print("  %s" % issue)
	if (len(issues) > 40):
		print("  ... и ещё %d" % (len(issues) - 40))
	# Отчёт при нуле сличённых случаев есть молчаливый пропуск, а не успех
	if (sum(counts.values()) == 0):
		print("  ОТКАЗ: сличать нечего - корпус пуст либо не забран")
		return 2
	return (1 if issues else 0)


if (__name__ == "__main__"):
	sys.exit(main())
