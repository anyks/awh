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

SNAKE_STOP = {"operator", "no_except", "decltype", "sizeof_", "alignof_", "return_type"}
STD_CALLS = {"make_pair", "make_tuple", "make_unique", "make_shared", "forward_like"}

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
    """Конструктор должен идти после методов (typedef class {...} alias_t;)."""
    src = "\n".join(lines)
    for m in re.finditer(r"typedef\s+class\s+(?:__AWH_SHARED_EXPORT__\s+)?(\w+)\s*\{", src):
        depth, start = 0, m.end() - 1
        end = None
        for i in range(start, len(src)):
            if src[i] == "{": depth += 1
            elif src[i] == "}":
                depth -= 1
                if depth == 0:
                    end = i; break
        if end is None: break
        alias = re.match(r"\s*(?:const\s+)?(\w+)", src[end+1:])
        body = src[start+1:end]
        first_line = src[:start].count("\n") + 1
        cname = m.group(1)
        aname = alias.group(1) if alias else cname
        body_lines = body.split("\n")
        proto = ctor = None
        for idx, l in enumerate(body_lines):
            code = re.sub(r"//.*|/\*.*?\*/", "", l).strip()
            if not code or code.startswith(("public:", "private:", "protected:", "/", "*", "#")):
                continue
            if re.match(r"~?" + re.escape(aname) + r"\s*\(", code) or re.match(r"~?" + re.escape(cname) + r"\s*\(", code):
                if ctor is None: ctor = first_line + idx
            elif re.search(r"\w+\s*\([^)]*\)(?:\s*const)?(?:\s*noexcept)?\s*(?:override\s*)?(?:=\s*0\s*)?;", code) or re.search(r"\w+\s*\(", code):
                if proto is None and not re.match(r"(?:static\s+)?(?:typedef|using|union|struct|enum)", code):
                    proto = first_line + idx
        if ctor and proto and ctor < proto:
            report(path, ctor, "ERROR", "§5.1", "конструктор объявлен до методов — конструктор в конце блока")

def check_file(path, lines, members, report):
    is_cpp = path.endswith(".cpp") or "/src/" in path
    is_hdr = path.endswith((".hpp", ".h")) and "/include/" in path
    in_block = False
    func_depth = 0
    in_func = False
    blank_ctx = None
    prev_code = ""
    static_names = set(re.findall(r"^\s*static\s+[\w:<>]+(?:\s*[*&][\w:<>]*)?\s+(\w+)\s*\(", "\n".join(lines), re.M))
    eng_words = re.compile(r"\b(the|with|from|where|which|this value|returns|performs?|we (perform|check|set|get))\b")
    for n, raw in enumerate(lines, 1):
        line = raw.rstrip("\n")
        code, in_block = strip_comments_strings(line, in_block)
        if line.strip() == "":
            if in_func and blank_ctx is None:
                blank_ctx = n
            continue
        if blank_ctx is not None:
            report(path, blank_ctx, "ERROR", "§1.3", "пустая строка внутри тела функции")
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
        # func depth (грубо: считаем только в коде)
        opens, closes = code.count("{"), code.count("}")
        base = func_depth
        func_depth = max(0, func_depth + opens - closes)
        if not in_func and opens and base == 0 and re.search(r"\)[^()]*\{\s*$", code) and not re.search(r"\b(if|for|while|switch|else|catch|try|namespace|struct|class|enum|union|do)\b", code):
            in_func = True
        elif in_func and func_depth == 0 and closes:
            in_func = False
        # if/for/while/switch с пробелом
        for m in re.finditer(r"\b(if|for|while|switch)\s+\(", code):
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
        for m in re.finditer(r"(?<![\w:>.])" + DEF_LIST_ERR + r"\s*\(", code):
            if m.group(1) in static_names or is_declaration(m.group(1), m.start()):
                continue
            report(path, n, "ERROR", "§6.2", f"обращение '{m.group(1)}(' без :: — системные функции только через ::")
        for m in re.finditer(r"(?<![\w:>.])" + DEF_LIST_WARN + r"\s*\(", code):
            if m.group(1) in static_names or is_declaration(m.group(1), m.start()) or re.search(r"::\s*$", code[:m.start()]):
                continue
            report(path, n, "WARN", "§6.2", f"возможно голое обращение к системной '{m.group(1)}(' (без ::)")
        # snake_case в именах функций/методов (§4.2), кроме внутренней статики __awh_{name}__ (§4.10)
        fm = re.match(r"^\s*(?:static\s+|inline\s+|constexpr\s+|virtual\s+|explicit\s+)*"
                      r"[A-Za-z_][\w:<>]*(?:\s*[*&]\s*)?\s+((?:\w+::)*~?\w+)\s*\(", code)
        if fm and "class" not in code and "struct" not in code.split("(")[0]:
            fname = fm.group(1).split("::")[-1]
            head = code[:fm.start(1)]
            call_kw = bool(re.search(r"\b(return|offsetof|sizeof|static_cast|awh_cast|co_return)\b[^;]*$", head))
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
            if re.fullmatch(r"(?:const\s+)?(?:struct|union|enum)\s*", headtok):
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

def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    strict = "--strict" in sys.argv
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
    for f in files:
        try:
            lines = open(f, errors="ignore").read().split("\n")
        except OSError as e:
            print(f"{f}: не читаю ({e})", file=sys.stderr); unreadable += 1; continue
        check_file(f, lines, members, report)
        check_class_order(lines, report, f)
    results.sort()
    errors = warns = 0
    for path, n, level, rule, msg in results:
        if level == "ERROR": errors += 1
        else: warns += 1
        print(f"{path}:{n}: {level} [{rule}] {msg}")
    print(f"\n{len(files)} файлов: ERROR={errors} WARN={warns}")
    if unreadable:
        return 2
    return 1 if errors or (strict and warns) else 0

if __name__ == "__main__":
    sys.exit(main())
