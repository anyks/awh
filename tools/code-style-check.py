#!/usr/bin/env python3
"""
Контроль оформления C++ по своду CODE-STYLE.md проекта AWH.

Проверки механические; эталон истины — include/net/io.hpp и
src/net/backend/bsd/kqueue.cpp: если эталон нарушает проверку, виновата проверка.

Уровни: ERROR — несомненное нарушение свода; WARN — heuristic, требует глаза.
Выход: 0 — чисто, 1 — есть ERROR (с WARN — только при --strict).
"""
import re, sys, os

DEF_LIST_ERR = r"(strcmp|strncmp|strcpy|strncpy|strcat|strlen|strstr|strchr|memcpy|memmove|memset|memcmp|malloc|calloc|realloc|free|qsort|bsearch|snprintf|vsnprintf|sprintf)"
DEF_LIST_WARN = r"(read|write|close|open|socket|connect|accept|send|recv|bind|listen|stat|fstat|lstat|opendir|readdir)"

def strip_comments_strings(line, in_block):
    """Возвращает код без комментариев/строк и новое состояние block-comment."""
    out = []
    i = 0
    while i < len(line):
        if in_block:
            j = line.find("*/", i)
            if j < 0:
                return "".join(out), True
            i, in_block = j + 2, False
            continue
        c = line[i]
        if c == "/" and line[i:i+2] == "/*":
            in_block = True; i += 2; continue
        if c == "/" and line[i:i+2] == "//":
            break
        if c in "\"'":
            q = c; i += 1
            while i < len(line):
                if line[i] == "\\":
                    i += 2; continue
                if line[i] == q:
                    i += 1; break
                i += 1
            continue
        out.append(c); i += 1
    return "".join(out), in_block


def parse_sections(block_lines):
	"""Разбор doc-блока: {язык: [(tag, blank_before, lineno, conts, width), ...]}"""
	sections = {}
	cur_lang = None
	groups = []
	prev_blank = True
	i = 0
	n = len(block_lines)
	while i < n:
		l = block_lines[i].strip()
		m = re.match(r"^\*\s*\\~(russian|english)?\s*$", l)
		if m:
			if cur_lang is not None or groups:
				sections[cur_lang or "?"] = groups
			groups = []
			cur_lang = m.group(1) or "_"
			prev_blank = True
			i += 1
			continue
		tag = re.match(r"^\*\s*@(\w+)", l)
		if tag:
			conts = 0
			j = i + 1
			while j < n:
				sj = block_lines[j].strip()
				if re.match(r"^\*\s*@", sj) or re.match(r"^\*\s*\\~", sj) or sj in ("", "*/"):
					break
				# строки-продолжения: после '*' два+ пробела
				if re.match(r"^\*\s{2,}\S", sj):
					conts += 1
					j += 1
					continue
				break
			groups.append((tag.group(1), prev_blank, i + 1, conts, len(l)))
			prev_blank = False
			i = j
			continue
		if re.match(r"^\*\s*$", l):
			prev_blank = True
		i += 1
	sections[cur_lang or "?"] = groups
	return sections

def group_extent(block_lines, groups):
	"""Мера наполнения каждой группы тега: (строки, строки таблицы, абзацы, знаки)"""
	n = len(block_lines)
	def bulk(s):
		return len(re.sub(r"\s+", "", s))
	res = []
	for k, gg in enumerate(groups):
		begin = gg[2]
		lim = groups[k + 1][2] if k + 1 < len(groups) else n + 1
		for i in range(begin, lim):
			s = block_lines[i - 1].strip()
			if re.match(r"^\*\s*\\~", s) or s.endswith("*/"):
				lim = i
				break
		content = table = text = 0
		paragraphs = 1
		prev_blank = False
		for i in range(begin + 1, lim):
			s = block_lines[i - 1].strip()
			if s in ("", "*"):
				prev_blank = True
				continue
			if re.match(r"^\*\s*@", s) or re.match(r"^\*\s*\\~", s) or s.endswith("*/"):
				continue
			content += 1
			text += bulk(s)
			if re.match(r"^\*\s*\|", s):
				table += 1
			if prev_blank:
				paragraphs += 1
			prev_blank = False
		res.append((content, table, paragraphs, text))
	return res

def section_has_text(block_lines, lang):
    """В секции языка есть строки без тегов."""
    began = False
    for l in block_lines:
        s = l.strip()
        if re.match(r"^\s*\*\s*\\~" + lang + r"\s*$", s):
            began = True
            continue
        if began:
            if re.match(r"^\s*\*\s*\\~", s) or s.endswith("*/"):
                break
            if s.strip("*").strip():
                return True
    return False

LEAD_MARK = re.compile(r"^\s*\*\s*\\~")
LEAD_TAG = re.compile(r"^\s*\*\s*@(\w+)")
LEAD_TEXT = re.compile(r"^\s*\*\s*[^\s@]")
LEAD_CONT = re.compile(r"^\s*\*\s{2,}\S")
LEAD_LANG = re.compile(r"^\s*\*\s*\\~(russian|english)\s*$")
LEAD_BLANK = re.compile(r"^\s*\*\s*$")
def check_lead_gap(path, start, block, report, fixes=None):
    """Отбивка вокруг зачина секции: лишняя пустая строка после метки языка (§7.1)
    и недостающая перед первым тегом после текста без тега (§7.3)."""
    for k in range(1, len(block) - 1):
        s = block[k].strip()
        mt = LEAD_TAG.match(s)
        if not mt:
            continue
        prev = block[k - 1].strip()
        if not LEAD_TEXT.match(prev) or LEAD_MARK.match(prev):
            continue
        j = k - 1
        while j > 0 and LEAD_TEXT.match(block[j].strip()) and not LEAD_MARK.match(block[j].strip()):
            j -= 1
        if LEAD_MARK.match(block[j].strip()):
            report(path, start + k, "ERROR", "§7.3", f"перед @{mt.group(1)} нет пустой строки ' *' — зачин секции без тега отделяется отбивкой (§7.3)")
    for k in range(len(block) - 2):
        if not LEAD_LANG.match(block[k].strip()):
            continue
        if not LEAD_BLANK.match(block[k + 1].strip()):
            continue
        j = k + 2
        while j < len(block) and LEAD_BLANK.match(block[j].strip()):
            j += 1
        if j >= len(block) or not LEAD_TAG.match(block[j].strip()):
            continue
        report(path, start + k + 1, "ERROR", "§7.1", f"пустая строка сразу после {block[k].strip()[1:].strip()} лишняя — первым идёт тег (§7.1)")
        if fixes is not None:
            for d in range(k + 1, j):
                fixes.add(start + d - 1)


def check_doc_parity(path, lines, report, fixes=None):
	in_block = False
	start = 0
	block = []
	for n, raw in enumerate(lines, 1):
		line = raw.rstrip("\n")
		if not in_block and re.match(r"\s*/\*\*", line):
			in_block = True; start = n; block = []
		if in_block:
			block.append(line)
			if "*/" in line:
				in_block = False
				if len(block) > 2:  # инлайновые /*!< ... */ не трогаем: блок из одной строки
					sec = parse_sections(block)
					if "/include/" in path or path.startswith("include/"):
						if list(sec) == ["?"] and sec.get("?"):
							report(path, start, "ERROR", "§7.6", f"заголовочный блок без языковых секций — публичный заголовок ведёт `\\~russian` и `\\~english` над `\\~` ({len(sec['?'])} тегов)")
						elif "russian" in sec and "english" not in sec:
							report(path, start, "ERROR", "§7.6", f"заголовочный блок: секция `\\~russian` есть, `\\~english` нет — перевод не начат")
						elif "english" in sec and "russian" not in sec:
							report(path, start, "ERROR", "§7.6", f"заголовочный блок: секция `\\~english` есть, `\\~russian` нет — публичный заголовок ведёт обе")
					check_lead_gap(path, start, block, report, fixes)

					ru, en = sec.get("russian"), sec.get("english")
					for lang in ("russian", "english"):
						gg = sec.get(lang) or []
						for gi in range(1, len(gg)):
							tag, blank, ln = gg[gi][0], gg[gi][1], gg[gi][2]
							prev_tag = gg[gi-1][0]
							packed = (tag in ("param", "return") and prev_tag in ("param", "return", "par")) or tag == "endcode" or (tag == "par" and prev_tag in ("param", "return", "par")) or (tag == prev_tag and tag in ("note", "details", "warning", "tparam"))
							if not packed and not blank:
								report(path, start + ln - 1, "ERROR", "§7.3", f"в секции {lang}: перед @{tag} нет пустой строки ' *' (группы тегов разделяются)")
							tight = (tag in ("param", "tparam", "retval", "arg", "return")
								and prev_tag in ("param", "tparam", "retval", "arg", "return"))
							if tight and blank:
								report(path, start + ln - 1, "ERROR", "§7.3", f"лишняя пустая строка перед @{tag} внутри блока @param/@return — блок ведёт плотно (§7.3)")
								if fixes is not None:
									m = ln - 2
									while m >= 0 and re.match(r"\s*\*\s*$", block[m].strip()):
										fixes.add(start + m - 1)
										m -= 1
					if (ru or en) and not (ru and en):
						have, miss = ("russian", "english") if ru else ("english", "russian")
						tags_have = ", ".join("@" + t[0] for t in (ru or en))
						note = "текст в ней есть, но без тегов: разметку секций выравнивай, @details/@note переводятся теми же тегами" if section_has_text(block, miss) else "перевод не начат"
						report(path, start, "ERROR", "§7.6", f"секция {miss} не несёт ни одного тега, в {have} они есть ({tags_have}) — {note} (§7.6.2)")
					if ru and en:
						tags_ru = [t[0] for t in ru]
						tags_en = [t[0] for t in en]
						if tags_ru != tags_en:
							report(path, start, "ERROR", "§7.6", f"английская секция: набор тегов {tags_en} != русская {tags_ru}")
						else:
							for (te, be, le, ce, we), (tr, br, lr, cr, wr) in zip(en, ru):
								lineno = start + le - 1
								if br and not be:
									report(path, lineno, "ERROR", "§7.6", f"английская секция: перед @{te} нет пустой строки ' *' (в русской есть)")
									if fixes is not None:
										fixes.add(lineno - 1)
								elif be and not br:
									report(path, lineno, "ERROR", "§7.6", f"английская секция: лишняя пустая строка перед @{te} (в русской нет)")
								if cr > 0 and ce == 0:
									report(path, lineno, "ERROR", "§7.6", f"английская секция: @{te} смят в одну строку ({we} симв.) — русская группа разбита на {cr} продолжений, английская обязана быть разбита так же")
							ext_en = group_extent(block, en)
							ext_ru = group_extent(block, ru)
							for (te, be, le, ce, we), er, ee in zip(en, ext_ru, ext_en):
								if ee[1] != er[1]:
									report(path, start + le - 1, "WARN", "§7.6", f"английская секция: @{te} строк таблицы: {ee[1]} против {er[1]} в русской — перевод неполон")
								if max(ee[3], er[3]) >= 200 and min(ee[3], er[3]) * 2 <= max(ee[3], er[3]):
									report(path, start + le - 1, "WARN", "§7.6", f"английская секция: @{te} объём {ee[3]} знаков против {er[3]} в русской — при двукратном расхождении вероятен не перевод а иной текст")
				block = []

SNAKE_STOP = {"operator", "no_except", "decltype", "sizeof_", "alignof_", "return_type"}
STD_CALLS = {"make_pair", "make_tuple", "make_unique", "make_shared", "forward_like"}
# слово перед именем в позиции типа означает вызов, а не объявление
KEYWORD_HEAD = {"return", "throw", "co_return", "co_await", "new", "delete", "case", "else"}

def is_snake_name(s):
    """snake_case имя в позиции объявления: внутренние подчёркивания, не член/статика/тип."""
    if not s or s.startswith("_") or s in SNAKE_STOP or s.endswith("_t"):
        return False
    return bool(re.fullmatch(r"[a-z][a-z0-9]*(?:_[a-z0-9]+)+", s))

def collect_members(files):
    """Собирает имена членов вида _name из объявлений в заголовках/источниках."""
    pat = re.compile(r"^\s*(?:const\s+)?[\w:<>]+(?:\s*[*&]\s*|\s+)(_\w+)\s*(?:=[^;]+)?;")
    names = set()
    for f in files:
        if f.endswith((".hpp", ".cpp", ".h")):
            try:
                for line in open(f, errors="ignore"):
                    m = pat.match(line)
                    if m:
                        names.add(m.group(1))
            except OSError:
                pass
    return names

def check_class_order(lines, report, path):
    """Конструкторы и деструктор — в конце блока class/struct/union (§5.1)."""
    src = "\n".join(lines)
    head = re.compile(r"(?:typedef\s+)?\b(?:class|struct|union)\s+(?:__AWH_SHARED_EXPORT__\s+)?([A-Z]\w*)\s*(?:final\s*)?\{")
    for m in head.finditer(src):
        depth, start = 0, m.end() - 1
        end = None
        for i in range(start, len(src)):
            if src[i] == "{": depth += 1
            elif src[i] == "}":
                depth -= 1
                if depth == 0:
                    end = i; break
        if end is None: continue
        cname = m.group(1)
        alias = re.match(r"\s*(?:const\s+)?(\w+)", src[end + 1:])
        aname = alias.group(1) if alias else cname
        names = {cname, aname}
        first_line = src[:start].count("\n") + 1
        qual = r"(?:explicit\s+|constexpr\s+|inline\s+|virtual\s+|[A-Z][A-Z0-9_]*\s+)*"
        names_re = "(" + "|".join(re.escape(x) for x in names) + ")"
        seq = []          # (разряд, номер строки): METHOD | CTOR | DTOR
        bd = 0
        buf = ""
        begin = 0
        for idx, l in enumerate(src[start + 1:end].split("\n")):
            code = re.sub(r"//.*|/\*.*?\*/", "", l).strip()
            if bd:
                bd += code.count("{") - code.count("}")
                continue
            if code.startswith("#"):
                buf = ""
                continue
            if not buf:
                if not code or code.startswith(("public:", "private:", "protected:", "/", "*", "{", "}")):
                    continue
                begin = first_line + idx
            buf = (buf + " " + code).strip()
            opens, closes = code.count("{"), code.count("}")
            closes_all = bd + opens - closes
            done = closes_all > 0 or buf.endswith(";") or (opens and closes_all == 0)
            bd = closes_all
            if not done:
                continue
            stmt = buf
            buf = ""
            if re.match(qual + r"~\s*" + names_re + r"\s*\(", stmt):
                seq.append(("DTOR", begin))
            elif re.match(qual + names_re + r"\s*\(", stmt):
                seq.append(("CTOR", begin))
            elif re.match(qual + names_re + r"\s*(?:&\s*)?operator\s*=\s*\(\s*(?:const\s+)?" + names_re + r"\s*(?:&|\*)", stmt):
                seq.append(("SPECIAL", begin))
            elif ("(" in stmt) and not re.match(r"(?:template\b|(?:static\s+)?(?:typedef|using|struct|union|enum|friend\s+(?:class|struct|union)))", stmt) and ("(*" not in stmt):
                seq.append(("METHOD", begin))
        methods = [ln for k, ln in seq if k == "METHOD"]
        ctors = [ln for k, ln in seq if k == "CTOR"]
        dtors = [ln for k, ln in seq if k == "DTOR"]
        if ctors and methods and min(ctors) < max(methods):
            report(path, min(ctors), "ERROR", "§5.1", f"{cname}: конструктор объявлен до методов — конструктор в конце блока")
        if dtors and methods and min(dtors) < max(methods):
            report(path, min(dtors), "ERROR", "§5.1", f"{cname}: деструктор объявлен до методов — деструктор в конце блока")
        if dtors and ctors and min(dtors) < max(ctors):
            report(path, min(dtors), "ERROR", "§5.1", f"{cname}: деструктор идёт прежде конструктора — сначала конструкторы, затем деструктор")
        if len(ctors) > 1 and (not methods or min(ctors) >= max(methods)):
            broken = [(k, ln) for k, ln in seq if k not in ("CTOR", "SPECIAL") and min(ctors) < ln < max(ctors)]
            if broken:
                kind, ln = broken[0]
                what = "деструктором" if kind == "DTOR" else "объявлением метода"
                report(path, ln, "ERROR", "§5.1", f"{cname}: конструкторы разорваны {what} — конструкторы идут подряд в конце блока, деструктор за ними")

def template_above(code, n):
    """Над строкой n стоит шаблонный заголовок (одно- или многострочный)."""
    j = n - 2
    while j >= 0:
        t = code[j].strip()
        if not t:
            j -= 1; continue
        if re.search(r"\btemplate\b", t):
            return True
        if t.startswith("#") or re.search(r"[;{}]$", t) or t.startswith("public:") or t.startswith("private:"):
            return False
        j -= 1
    return False

DEF_OPEN = re.compile(r"\)\s*(?:(?:const|noexcept|override|volatile|mutable)\s+|&\s*|\*\s*)*\{")
DECL_OPEN = re.compile(r"^(?:namespace\b|(?:extern\s+\"[^\"]*\"\s*)?(?:typedef\s+)?(?:class|struct|union|enum)\b)|\b(?:class|struct|union)\s+\w+\s*(?:final\s*)?\{$")
STMT_OPEN = re.compile(r"^(?:if|for|while|switch|else|try|catch|do)\b|\b(?:if|for|while|switch)\s*\(")


HEADER_TAGS = ["file", "date", "license", "author", "telegram", "phone", "email", "site", "brief", "copyright"]

def check_file_header(path, lines, report):
    """Шапка файла: обязательные теги, их порядок и формат (§2.1)."""
    if not lines or not lines[0].strip().startswith("/**"):
        if any(l.strip() for l in lines[:3]):
            report(path, 1, "ERROR", "§2.1", "файл не начинается с doc-блока шапки `/** */`")
        return
    block = []
    for l in lines:
        block.append(l)
        if "*/" in l:
            break
    else:
        report(path, 1, "ERROR", "§2.1", "doc-блок шапки не закрыт `*/`")
        return
    is_cpp = path.endswith(".cpp")
    tags = []
    loose = []
    for i, l in enumerate(block, 1):
        s = l.strip()
        if s in ("/**", "*/") or s in ("*", ""):
            continue
        m = re.match(r"^\*\s*@(\w+)", s)
        if m:
            tags.append((m.group(1), i, re.sub(r"^\s*\*\s*@\w+\s*", "", s).strip()))
            continue
        if re.match(r"^\*\s*\\~", s):
            continue
        if re.match(r"^\*\s\s", s):        # продолжение тега — выровненный отступ
            continue
        loose.append((i, s[1:].strip()[:44]))
    names = [t for t, _, _ in tags]
    values = {t: v for t, _, v in tags}
    lines_of = {t: i for t, i, _ in tags}
    if "file" not in names:
        report(path, 1, "ERROR", "§2.1", "в шапке нет `@file` — название файла ведут тегом, а не голым текстом")
    else:
        base = os.path.basename(path)
        if values["file"] != base:
            report(path, lines_of["file"], "ERROR", "§2.1", f"`@file {values['file']}` не совпадает с именем файла `{base}`")
    if "date" not in names:
        report(path, 1, "ERROR", "§2.1", "в шапке нет `@date`")
    elif not re.match(r"^\d{4}-\d{2}-\d{2}$", values["date"]):
        report(path, lines_of["date"], "ERROR", "§2.1", f"`@date {values['date']}` не в формате ГГГГ-ММ-ДД")
    if "copyright" not in names:
        report(path, 1, "ERROR", "§2.1", "в шапке нет `@copyright`")
    elif not re.match(r"^Copyright\s+©\s+\d{4}$", values["copyright"]):
        report(path, lines_of["copyright"], "ERROR", "§2.1", f"`@copyright {values['copyright'][:32]}` — свод требует `Copyright © ГГГГ`")
    miss = [t for t in HEADER_TAGS if t not in names]
    if miss:
        report(path, 1, "ERROR", "§2.1", "в шапке нет " + ", ".join("`@" + t + "`" for t in miss))
    keep = []
    for t in names:
        if t in HEADER_TAGS and (not keep or keep[-1] != t):
            keep.append(t)
    if is_cpp and any(re.search(r"\\~(russian|english)", l) for l in block):
        report(path, 1, "ERROR", "§2.1", "шапка файла реализации несёт `\\~`-секции — по §2.3 в .cpp только русский")
    elif keep != [t for t in HEADER_TAGS if t in names]:
        report(path, 1, "ERROR", "§2.1", "шапка файла: порядок " + " → ".join("`@" + t + "`" for t in keep) +
               ", ожидание " + " → ".join("`@" + t + "`" for t in HEADER_TAGS))
    for i, text in loose:
        report(path, i, "ERROR", "§2.1", f"в шапке текст вне тегов — «{text}», содержание ведёт `@brief`")


def project_root(path):
    """Корень проекта по файлу: ближайший предок с include/ или src/."""
    d = os.path.dirname(os.path.abspath(path))
    while d != os.path.dirname(d):
        if os.path.isdir(os.path.join(d, "include")) or os.path.isdir(os.path.join(d, "src")):
            return d
        d = os.path.dirname(d)
    return None

def build_impl_index(files):
    """Заголовки, которым есть куда переносить тело: парная .cpp или .cpp, его подключающий."""
    roots = []
    for f in files:
        r = project_root(f)
        if r and r not in roots:
            roots.append(r)
    impl = {}
    for r in roots:
        keys = set(); tokens = set()
        for base in ("src", "tools", "sample", "unit"):
            d = os.path.join(r, base)
            if not os.path.isdir(d):
                continue
            for rr, _, fs in os.walk(d):
                for f in fs:
                    if not f.endswith(".cpp"):
                        continue
                    full = os.path.join(rr, f)
                    keys.add(os.path.relpath(full, d).rsplit(".", 1)[0].replace(os.sep, "/"))
                    try:
                        body = open(full, errors="ignore").read()
                    except OSError:
                        continue
                    for m in re.finditer(r'^\s*#\s*include\s*[<"]([^">\n]+)[>"]', body, re.M):
                        tokens.add(m.group(1).strip().replace(os.sep, "/"))
        impl[r] = (keys, tokens)
    return impl

def has_implementation(path, impl):
    """Есть ли у заголовка файл реализации, куда тело можно перенести."""
    if not impl:
        return True
    r = project_root(path)
    if r is None or r not in impl:
        return True
    keys, tokens = impl[r]
    ap = os.path.abspath(path)
    rel = os.path.relpath(ap, os.path.join(r, "include")).replace(os.sep, "/")
    key = rel.rsplit(".", 1)[0]
    if key in keys:
        return True
    for t in tokens:
        if t == rel or t.endswith("/" + rel) or t == key or t.endswith("/" + key):
            return True
    return False

def check_header_bodies(path, lines, report, impl=None, waived=None):
    """Тело определения в заголовке допустимо только с явной пометкой inline (§5.5).

    Заголовки без файла реализации правило не снимает целиком, а молчит: переносить
    тело некуда (§5.5.1).
    """
    if impl is not None and not has_implementation(path, impl):
        if waived is not None:
            waived.append(path)
        return
    code = []
    in_block = False
    for l in lines:
        c, in_block = strip_comments_strings(l, in_block)
        code.append(c)
    stack = []
    for n, c in enumerate(code, 1):
        s = c.strip()
        if not s:
            continue
        marked_decl = bool(DECL_OPEN.search(s))
        prev_sig = ""
        j = n - 2
        while j >= 0:
            t = code[j].strip()
            if t:
                prev_sig = t; break
            j -= 1
        tmpl_ctx = bool(re.search(r"\btemplate\b", s) or template_above(code, n))
        marked_stmt = (not marked_decl) and bool(STMT_OPEN.search(s))
        opener = None
        if not marked_decl and not marked_stmt:
            m = DEF_OPEN.search(s)
            if m and (not stack or stack[-1] == "decl"):
                opener = m.start()
        declarator = ""
        if opener is not None:
            declarator = s[:opener].strip()
            if not re.match(r"^(?:template\b|(?:[\w:<>,*&\s]+)?(?:~?[A-Za-z_~][\w:<>]*|operator\s*[^\s(]+)\s*\()", declarator + "("):
                opener = None
            elif re.match(r"^(?:[\w:<>,*&\s]*\b(?:if|for|while|switch)\b)", declarator):
                opener = None
        if opener is not None and not (tmpl_ctx or any(k == "tmpl" for k in stack)):
            acc = declarator
            j = n - 2
            while j >= 0:
                t = code[j].strip()
                if not t or re.search(r"[;{}]", t):
                    break
                acc = t + " " + acc
                j -= 1
            prev = ""
            k = j
            while k >= 0:
                t = code[k].strip()
                if t:
                    prev = t; break
                k -= 1
            whole = acc + " " + prev
            if not re.search(r"\b(?:inline|constexpr|__forceinline)\b|[A-Z][A-Z0-9_]*_INLINE\b", whole) and not template_above(code, n):
                if not (prev.startswith("template") or re.search(r"\btemplate\b", whole)):
                    name = None
                    for cand in re.finditer(r"(?<![\w:<>~])(~?[A-Za-z_~][\w:<>]*|operator\s*(?:[^(){}\s]+|new\[\]|delete\[\]))\s*\(", s):
                        if cand.group(1).strip() in ("if", "for", "while", "switch", "return", "catch", "sizeof", "static_cast", "dynamic_cast", "reinterpret_cast", "const_cast", "new", "delete", "throw"):
                            continue
                        name = cand.group(1).strip(); break
                    if name is None:
                        name = declarator[:24]
                    report(path, n, "ERROR", "§5.5", f"определение «{name}» с телом в заголовке — перенеси тело в .cpp, в заголовке оставь объявление; в заголовке тело допустимо лишь там, где пометка `inline`/`constexpr`/модульный `AWH_*_INLINE` или шаблон уже стоят")
        opens, closes = s.count("{"), s.count("}")
        kind = ("decl" if marked_decl else ("stmt" if marked_stmt else "body"))
        if marked_decl and tmpl_ctx:
            kind = "tmpl"
        for _ in range(opens):
            stack.append(kind)
        for _ in range(closes):
            if stack:
                stack.pop()

TAG_HEAD = re.compile(r"^\s*\*\s*@(\w+)")
PARAM_LINE = re.compile(r"^\s*\*\s*@(param|tparam|retval|arg)\s+(\S+)(\s+)(\S)")
RETURN_LINE = re.compile(r"^\s*\*\s*@(return)(\s+)(\S)")
CONT_LINE = re.compile(r"^\s*\*(\s{2,})(\S)")
SKIP_TAGS = ("code", "verbatim", "dontinclude", "image")


def flush_run(path, run, report):
    """Описания группы @param/@return: одна колонка, на шаг за самым длинным именем."""
    if len(run) < 2:
        return
    target = max(end for _, _, end, _, _ in run) + 1
    for n, col, end, tag, name in run:
        if col != target:
            what = f"@{tag} {name}".strip()
            report(path, n, "ERROR", "§7.3", f"«{what}»: описание с колонки {col}, в группе — {target}; имена и описания выравниваются по одной колонке (§7.3)")


def walk_block(path, block, report):
    run = []
    skip = False
    for n, s in block:
        if skip:
            if "@endcode" in s or "@endverbatim" in s or "@enddot" in s:
                skip = False
            continue
        m = TAG_HEAD.match(s)
        if m and m.group(1) in SKIP_TAGS:
            flush_run(path, run, report)
            run = []
            skip = True
            continue
        if m and m.group(1) in ("param", "tparam", "retval", "arg"):
            mm = PARAM_LINE.match(s)
            if mm:
                run.append((n, mm.start(4), mm.end(2), m.group(1), mm.group(2)))
            continue
        if m and m.group(1) == "return":
            mm = RETURN_LINE.match(s)
            if mm:
                run.append((n, mm.start(3), mm.end(1), "return", ""))
            continue
        if m:
            flush_run(path, run, report)
            run = []
            continue
        if re.match(r"^\s*\*\s*$", s) or s.strip() in ("/**", "*/"):
            flush_run(path, run, report)
            run = []
            continue
        cm = CONT_LINE.match(s)
        if cm and run:
            if cm.start(2) != run[-1][1]:
                report(path, n, "ERROR", "§7.3", f"перенос описания «@{run[-1][3]} {run[-1][4]}».strip() с колонки {cm.start(2)}, ожидание {run[-1][1]} (§7.3)".replace("».strip()", "»"))
            continue
        flush_run(path, run, report)
        run = []
    flush_run(path, run, report)


def check_alignment(path, lines, report):
    """Колонка описаний у @param/@return в публичных заголовках (§7.3).

    Внутренние блоки реализаций (`src/**`) колонку не выдерживают — там `@param`
    идёт с двумя пробелами после тега и без выравнивания по группе, поэтому
    проверка смотрит только `include/**`.
    """
    if not ("/include/" in path or path.startswith("include/")):
        return
    in_block = False
    block = []
    for n, raw in enumerate(lines, 1):
        s = raw.rstrip("\n")
        if not in_block and re.match(r"\s*/\*\*", s):
            in_block = True
            block = []
        if not in_block:
            continue
        block.append((n, s))
        if "*/" in s:
            in_block = False
            walk_block(path, block, report)
            block = []

def find_bodies(lines):
    """Строки тел определений: от заголовка до его закрывающей скобки (§1.3).

    Накопным по файлу счётом скобок тело не ищут: у ветвей условной компиляции
    открывающая «{» стоит в одной, а закрывающая «}» в другой (и у «case …: {»
    тоже), отчего сбой уносил метку «внутри тела» до конца файла. Тело кончают
    раньше из двух: счёт скобок от заголовка вернулся к нулю, или попала
    одиночная «}» не глубже отступа заголовка. Заголовок начинается не дальше
    одного таба, кончается «{», «:» (список инициализации) или «{}».
    """
    code = []
    in_block = False
    for raw in lines:
        c, in_block = strip_comments_strings(raw.rstrip("\n"), in_block)
        code.append(c)
    inside = [False] * len(lines)
    kw = re.compile(r"^(?:if|for|while|switch|else|catch|try|do|return|namespace|struct|class|union|enum|typedef|using|static_assert|case|default)\b")
    closer = re.compile(r"^([ \t]*)\}([();, \t]*)(?://.*|/\*.*\*/)?$")
    n = 0
    while n < len(lines):
        line = lines[n].rstrip("\n")
        st = line.strip()
        tabs = len(line) - len(line.lstrip("\t"))
        head = bool(st) and tabs <= 1 and not st.startswith(("#", "}", "*", "/")) \
            and "(" in code[n] and ";" not in code[n] and not kw.match(st) \
            and st.endswith(("{", ":", "{}"))
        if not head:
            n += 1
            continue
        pad = len(line) - len(line.lstrip())
        open_idx = None
        cur = n
        while cur < len(lines):
            if "{" in code[cur]:
                open_idx = cur
                break
            if ";" in code[cur]:
                break
            cur += 1
        if open_idx is None:
            n += 1
            continue
        end_idx = None
        depth = 0
        cur = open_idx
        while cur < len(lines):
            body = code[cur][code[cur].index("{"):] if cur == open_idx else code[cur]
            depth += body.count("{") - body.count("}")
            m = closer.match(lines[cur].rstrip("\n"))
            if depth <= 0 or (m and len(m.group(1)) <= pad):
                end_idx = cur
                break
            cur += 1
        if end_idx is None:
            # закрывающей скобки не найдено: тело не помечать, иначе разметка
            # уезжает в конец файла ложными «§1.3»
            n += 1
            continue
        for idx in range(n, end_idx + 1):
            inside[idx] = True
        n = end_idx + 1
    return inside

def check_file(path, lines, members, report, fixes=None):
    is_cpp = path.endswith(".cpp") or "/src/" in path
    is_hdr = path.endswith((".hpp", ".h")) and "/include/" in path
    in_block = False
    bodies = find_bodies(lines)
    blank_ctx = None
    prev_code = ""
    static_names = set(re.findall(r"^\s*(?:\[\[.*?\]\]\s*)?static\s+(?:[\w:<>]+\s+)+?[*&]?\s*(\w+)\s*\(", "\n".join(lines), re.M))
    eng_words = re.compile(r"\b(the|with|from|where|which|this value|returns|performs?|we (perform|check|set|get))\b")
    for n, raw in enumerate(lines, 1):
        line = raw.rstrip("\n")
        code, in_block = strip_comments_strings(line, in_block)
        if line.strip() == "":
            if bodies[n - 1] and blank_ctx is None:
                blank_ctx = n
            continue
        if blank_ctx is not None:
            report(path, blank_ctx, "ERROR", "§1.3", "пустая строка внутри тела функции")
            if fixes is not None:
                fixes.add(blank_ctx - 1)
        blank_ctx = None
        # отступы табами
        indent = re.match(r"^([ \t]*)\S", line)
        if indent and indent.group(1).startswith(" "):
            stripped = line.lstrip()
            cont = prev_code.rstrip().endswith((":", ",", "("))
            if not (stripped.startswith("*") or stripped.startswith("#") or in_block or cont):
                report(path, n, "ERROR", "§1.1", "пробелы вместо табуляции в отступе")
        if is_cpp:
            for m in re.finditer(r"//\s*(.*)$", line):
                if eng_words.search(m.group(1)):
                    report(path, n, "ERROR", "§2.3", f"английский в комментарии .cpp: '{m.group(1)[:60]}'")
            cm = re.match(r"\s*/\*\*?\s*(.*)$", line)
            if cm and eng_words.search(cm.group(1)):
                report(path, n, "ERROR", "§2.3", f"английский в doc-комментарии .cpp: '{cm.group(1)[:60]}'")
        if not code.strip():
            continue
        # комментарий над циклом/switch (§6.6): непосредственно над заголовком — // или */
        lm = re.match(r"^\s*(for|while|switch)\s*\(", line)
        if lm and not re.match(r"^\s*}\s*while", line):
            j = n - 2
            while j >= 0 and lines[j].strip() == "":
                j -= 1
            pl = lines[j].strip() if j >= 0 else ""
            if not (pl.endswith("*/") or pl.startswith("*")):
                if pl.startswith("//"):
                    report(path, n, "ERROR", "§6.6", f"над «{lm.group(1)}» строчный комментарий — по §6.6 заголовок цикла/switch требует doc-блок /** */")
                else:
                    report(path, n, "ERROR", "§6.6", f"над «{lm.group(1)}» нет комментария — поставьте блок /** */ непосредственно перед заголовком")
        # закрывание блочного комментария, склеенное с кодом (§1.4)
        gm = re.match(r"^(\s*)\*/(\s*\S.*)$", line)
        if gm:
            report(path, n, "ERROR", "§1.4", "закрывающая `*/` склеена с кодом — закрывание блока на своей строке")
        # if/for/while/switch с пробелом
        for m in re.finditer(r"\b(if|for|while|switch)\s+\(", code):
            if code[:m.start()].rstrip().endswith("#"):
                continue  # ж мосл #if/#elif
            report(path, n, "ERROR", "§1.2", f"{m.group(1)} с пробелом перед скобкой: '{m.group(0)}'")
        # static_cast без пробелов
        if "static_cast<" in code:
            report(path, n, "ERROR", "§6.3", "static_cast требует пробелов: static_cast <T> (x)")
        for m in re.finditer(r">\s{0,0}\(", code):
            if "static_cast" in code[:m.start()] and "static_cast <" not in code[:m.start()].rsplit("static_cast <",1)[-1][:0]:
                pass
        if re.search(r"static_cast\s*<[^>]*>\(", code):
            report(path, n, "ERROR", "§6.3", "нет пробела между > и ( в static_cast <T> (x)")
        # Си-касты примитивов
        if re.search(r"\(\s*(?:u?int(?:8|16|32|64)_t|size_t)\s*\)\s*\w", code) or re.search(r"\(\s*void\s*\*\s*\)\s*\w", code):
            report(path, n, "ERROR", "§6.3", "Си-каст примитива — использовать static_cast <T> (x)")
        # голые системные функции
        def is_declaration(name, pos):
            head = code[:pos]
            return bool(re.match(r"^[\w:]+(?:\s*[*&]\s*)?\s+", head.lstrip())) and (head.strip() == "" or re.search(r"[\w:*>\s]$", head)) and (head.lstrip() != "" or pos == 0) and re.match(r"^[\w:]+(?:<[^>]*>)?(?:\s*[*&])?\s+" + re.escape(name) + r"\s*\(", code.lstrip())
        for m in re.finditer(r"(?<![\w:>.=-])" + DEF_LIST_ERR + r"\s*\(", code):
            if m.group(1) in static_names or is_declaration(m.group(1), m.start()):
                continue
            report(path, n, "ERROR", "§6.2", f"обращение '{m.group(1)}(' без :: — системные функции только через ::")
        for m in re.finditer(r"(?<![\w:>.=-])" + DEF_LIST_WARN + r"\s*\(", code):
            if m.group(1) in static_names or is_declaration(m.group(1), m.start()) or re.search(r"(::|[,:(])\s*$", code[:m.start()].rstrip()):
                continue
            if not code[:m.start()].strip() and prev_code.rstrip().endswith((":", ",")):
                continue
            if re.search(r"#\s*define\s+$", code[:m.start()]):
                continue
            report(path, n, "WARN", "§6.2", f"возможно голое обращение к системной '{m.group(1)}(' (без ::)")
        # snake_case в именах функций/методов (§4.2), кроме внутренней статики __awh_{name}__ (§4.10)
        fm = re.match(r"^\s*(?:static\s+|inline\s+|constexpr\s+|virtual\s+|explicit\s+)*"
                      r"[A-Za-z_][\w:<>]*(?:\s*[*&]\s*)?\s+((?:\w+::)*~?\w+)\s*\(", code)
        if fm and "class" not in code and "struct" not in code.split("(")[0]:
            fname = fm.group(1).split("::")[-1]
            head = code[:fm.start(1)]
            call_kw = bool(re.search(r"\b(return|throw|co_return|co_await|new|delete|offsetof|sizeof|static_cast|awh_cast)\b[^;]*$", head))
            if call_kw or fname in STD_CALLS:
                pass
            elif is_snake_name(fname) and "__awh" not in fm.group(1):
                report(path, n, "ERROR", "§4.2", f"snake_case в имени функции/метода '{fname}' — требуется camelCase")
        # snake_case в объявлениях переменных и параметров (§4.3)
        for vm in re.finditer(r"(?:const\s+)?[A-Za-z_][\w:<>]*(?:\s*[*&]\s*)?\s+([a-z]\w*)\s*(?==[^=]|;|,|\)|\()", code):
            name = vm.group(1)
            kw = {"if", "for", "while", "switch", "return", "case", "sizeof", "noexcept", "const", "static", "auto", "int", "void", "bool"} | STD_CALLS
            if name in kw or not is_snake_name(name):
                continue
            before = code[:vm.start()]
            headtok = code[vm.start(0):vm.start(1)].strip()
            if headtok and headtok.split()[-1] in KEYWORD_HEAD:
                continue
            if headtok == "const" or re.fullmatch(r"(?:const\s+)?(?:struct|union|enum)\s*", headtok):
                continue
            if re.search(r"(?:__awh_\w*__|::|\b(?:return|offsetof|sizeof|static_cast|awh_cast|co_return|struct|union|enum|reinterpret_cast|const_cast))$", before.rstrip()) or re.search(r"(offsetof|sizeof)\s*\($", before.rstrip()) or re.search(r"<[^>]*$", before):
                continue
            report(path, n, "ERROR", "§4.3", f"snake_case '{name}' в объявлении — требуется camelCase")
        # m_/s_ префиксы
        if re.search(r"(?<![\w.>-])(m_[a-z]\w*|s_[a-z]\w*)\b", code):
            report(path, n, "ERROR", "§4.4", "префиксы m_/s_ запрещены — члены с нижним подчёркиванием на конце")
        # this-> для известных членов
        for mm in re.finditer(r"(?<![\w_])(_\w+)(?![\w_])", code):
            name = mm.group(1)
            if name not in members or name in ("__awh_nodes__",):
                continue
            before = code[:mm.start()]
            after = code[mm.end():]
            if re.search(r"(this\s*->|->|::|\.\s*)$", before.rstrip()) or re.search(r"^(?:\s*(?:\+=|-=|\*=|/=|\+\+|--|;|\)|,|\.|->|\[|&&|\|\||\?|\())", after):
                continue
            if re.search(r"(?:const\s+)?[\w:<>]+(?:[*&]\s*)?\s+$", before) or re.search(r"^[*\s]+[\w:<>]+\s+" + re.escape(name) + r"$", before.strip() + name):
                continue  # объявление поля/параметра
            report(path, n, "WARN", "§6.1", f"член '{name}' без this->")
        # макросы не в hex
        if re.match(r"#\s*define\s+AWH_\w+\s+\d{2,}\s*$", line.strip()):
            report(path, n, "WARN", "§3.3", "значение макроса в десятичной системе — свод требует hex с пояснением")
        # @brief глаголом
        if re.search(r"@brief\s+(Возвращает|Устанавливает|Получает|Проверяет|Выполняет|Определяем|Устанавливаем)", line):
            report(path, n, "WARN", "§7.2", "@brief существительным ('Метод …'), не глаголом")
        # doc-блок без завершающей пустой ' * ' строки
        if line.strip() == "*/":
            prev = lines[n-2].strip() if n >= 2 else ""
            if prev and not prev.startswith("@") and prev != "*" and not prev.startswith("*"):
                pass
            if prev.startswith("@"):
                report(path, n-1, "WARN", "§7.5", "перед */ нет пустой строки ' *'")
        prev_code = code

def check_steps(path, lines, report):
    """§8.5: над каждым оператором — комментарий; закрывающие скобки/препроцессор/макросы/else-цепочки не требуют."""
    in_block = False
    in_macro = False
    balance = 0
    init_depth = 0
    prev_code_raw = ""
    prev_stmt_shape = None
    prev_significant = None   # ('kind', index) kind: comment/code/blank
    block_star = []           # (lineno, indent) строк «*» текущего doc-блока
    for n, raw in enumerate(lines, 1):
        line = raw.rstrip("\n")
        code, new_block = strip_comments_strings(line, in_block)
        block_only = (code.strip() == "" and line.strip() != "" and (new_block or in_block or line.strip().startswith(("//", "*", "/*"))))
        in_block = new_block
        if in_macro:
            in_macro = line.rstrip().endswith("\\")
            prev_significant = ("code", n)
            continue
        if line.rstrip().endswith("\\") and re.match(r"^\s*#\s*define", line):
            in_macro = True
        if line.strip() == "":
            prev_significant = ("blank", n)
            continue
        if block_only or code.strip() == "":
            if in_block or line.strip().startswith(("/**", "/*!", "*", "/*")):
                ind = len(line) - len(line.lstrip("\t "))
                if re.match(r"^\s*\*", line):
                    block_star.append((n, ind))
            if line.strip().endswith("*/"):
                if len(block_star) > 1:
                    from collections import Counter
                    mode = Counter(i for _, i in block_star).most_common(1)[0][0]
                    for ln, ind in block_star:
                        if ind != mode:
                            report(path, ln, "ERROR", "§1.1", f"звёздочка сместилась относительно блока (отступ {ind}, ожидание {mode}) — выровняйте табуляцией")
                block_star = []
            prev_significant = ("comment", n)
            continue
        st = line.strip()
        if re.match(r"^#\s*endif\b", st):
            j = n - 2
            while j >= 0 and lines[j].strip() == "":
                j -= 1
            pl = lines[j].strip() if j >= 0 else ""
            if pl.endswith("*/") or pl.startswith("*") or pl.startswith("//"):
                report(path, j + 1, "ERROR", "§1.6", f"комментарий над `#endif` (закрытие на строке {n}) — по §1.6 закрытие условия не поясняют")
            if re.search(r"#\s*endif\b\s*(/\*|//)", st):
                report(path, n, "ERROR", "§1.6", "комментарий-хвост у `#endif` — по §1.6 директива пишется голым `#endif`")
        exempt = (
            re.match(r"^\}", st) or st in ("{", "};") or re.match(r"^#\s*(?:include|pragma|undef|endif|error|warning)\b", st) or
            re.match(r"^(?:case\b|default\b)", st) or
            re.match(r"^(?:break|continue);$", st) or
            re.match(r"^\s*(?:else\b|try\b)", st) or
            re.match(r"^(?:namespace|class|struct|union|enum)(?:\s+[\w:]+)?\s*\{?$", st) or
            re.match(r"^(?:static\s+)?(?:struct|union)\s+\w+[{}]*(?:\[\w*\])?\s*[{;]$", st) or
            (st.endswith(":") and re.match(r"^[A-Za-z_]\w*:$", st)) or
            st == "public:" or st == "private:" or st == "protected:"
        )
        prev_ctrl = prev_code_raw.strip() in ("else", "try") or bool(re.match(r"^\s*}\s*(?:else\b|catch\b|while\b)[^)]*$", prev_code_raw) and not prev_code_raw.rstrip().endswith("{"))
        prev_ctrl = prev_ctrl or prev_code_raw.strip().startswith("template")
        decl_pair = bool(re.match(r"^(?:const\s+)?(?:struct\s+)?[\w:<>]+\s+\w+\{\s*\};$", st)) and bool(re.match(r"^(?:const\s+)?(?:struct\s+)?[\w:<>]+\s+\w+\{\s*\};$", prev_code_raw.strip()))
        continued = balance > 0 or prev_ctrl or decl_pair or (prev_code_raw.rstrip().endswith((":", ",", "(", "&", "|", "?", "+", "-", "="))) or ("//" in line)
        continued = continued or (prev_code_raw.strip() != "" and prev_stmt_shape and re.match(r"^[\w:>.-]*\s*", st).group(0) and re.match(r"^[\w:>.<-]*", st).group(0) == prev_stmt_shape)
        continued = continued or (init_depth > 0)
        continued = continued or bool(re.match(r"^(?:\<\<|>>|&&|\|\|\|->|::|\.|\?|:|[+=*/,-])", st))
        balance += code.count("(") + code.count("[") - code.count(")") - code.count("]")
        if init_depth:
            init_depth += st.count("{") - st.count("}")
            if init_depth <= 0:
                init_depth = 0
        elif re.search(r"=\s*\{\s*$", st):
            init_depth = 1
        if re.match(r"^namespace(?:\s+[\w:]+)?\s*\{?$", st):
            j = n - 2
            while j >= 0 and lines[j].strip() == "":
                j -= 1
            pl = lines[j].strip() if j >= 0 else ""
            if not (pl.endswith("*/") or pl.startswith("*")):
                if pl.startswith("//"):
                    report(path, n, "ERROR", "§9.5", "объявление namespace со строчным комментарием — по §9.5 нужен doc-блок /** */ с @brief")
                else:
                    report(path, n, "ERROR", "§9.5", "объявление namespace без doc-блока /** */ над ним (§9.5)")
        elif re.match(r"^#\s*define\b", st) or re.match(r"^#\s*(?:if|ifdef|ifndef|elif|else)\b", st):
            j = n - 2
            while j >= 0 and lines[j].strip() == "":
                j -= 1
            pl = lines[j].strip() if j >= 0 else ""
            if pl.endswith("*/") or pl.startswith("*"):
                pass
            elif pl.startswith("//"):
                rule = "§3.3" if st.startswith("#define") else "§3.4"
                report(path, n, "ERROR", rule, f"над {st.split()[0]} одиночный // — по {rule} требуется doc-блок /** */")
            else:
                rule = "§3.3" if st.startswith("#define") else "§3.4"
                report(path, n, "ERROR", rule, f"над {st.split()[0]} нет комментария — по {rule} требуется doc-блок /** */")
        elif not exempt and not continued and prev_significant and prev_significant[0] == "code":
            report(path, n, "ERROR", "§8.5", "оператор без пошагового комментария над ним (§8.5)")
        prev_significant = ("code", n)
        if not continued:
            prev_code_raw = line
            m2 = re.match(r"^[\w:>.<-]*\s*\(", st)
            prev_stmt_shape = m2.group(0)[:-1] if m2 else None
        else:
            prev_code_raw = line

def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    strict = "--strict" in sys.argv
    do_fix = "--fix" in sys.argv
    fixes = {}
    files = []
    for a in args:
        if os.path.isdir(a):
            for root, _, fs in os.walk(a):
                files += [os.path.join(root, f) for f in fs if f.endswith((".cpp", ".hpp", ".h"))]
        else:
            files.append(a)
    if not files:
        print("использование: code-style-check.py [--strict] ФАЙЛ|КАТАЛОГ ..."); return 2
    members = collect_members(files)
    unreadable = 0
    results = []
    def report(path, n, level, rule, msg):
        results.append((path, n, level, rule, msg))
    impl_index = build_impl_index(files)
    waived = []
    for f in files:
        try:
            lines = open(f, errors="ignore").read().split("\n")
        except OSError as e:
            print(f"{f}: не читаю ({e})", file=sys.stderr); unreadable += 1; continue
        fixset = set() if do_fix else None
        check_file(f, lines, members, report, fixset)
        check_class_order(lines, report, f)
        check_file_header(f, lines, report)
        check_alignment(f, lines, report)
        if f.endswith((".hpp", ".h")) and re.search(r"(^|/)include/", f):
            check_header_bodies(f, lines, report, impl_index, waived)
        check_doc_parity(f, lines, report, fixset)
        if "--no-steps" not in sys.argv:
            check_steps(f, lines, report)
        if fixset:
            fixes[f] = fixset
    if do_fix and fixes:
        for f, idxs in fixes.items():
            fl = open(f, errors="ignore").read().split("\n")
            ins = 0
            dele = 0
            for idx in sorted(idxs, reverse=True):
                anchor = fl[idx] if idx < len(fl) else ""
                if anchor.strip() in ("", "*"):
                    del fl[idx]
                    dele += 1
                    continue
                m = re.match(r"^(\s*)\*", anchor)
                fl.insert(idx, (m.group(1) + "*") if m else anchor)
                ins += 1
            open(f, "w").write("\n".join(fl))
            print(f"{f}: вставлено {ins}, удалено {dele}")
    results.sort()
    errors = warns = 0
    for path, n, level, rule, msg in results:
        if level == "ERROR": errors += 1
        else: warns += 1
        print(f"{path}:{n}: {level} [{rule}] {msg}")
    if waived:
        print(f"§5.5 молчит на {len(waived)} заголовках без файла реализации: {', '.join(sorted(set(waived))[:24])}" + (" …" if len(set(waived)) > 24 else ""))
    print(f"\n{len(files)} файлов: ERROR={errors} WARN={warns}")
    if unreadable:
        return 2
    return 1 if errors or (strict and warns) else 0

if __name__ == "__main__":
    sys.exit(main())
