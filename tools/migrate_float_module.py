#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Migrate include/sys/float -> include/float + src/float with AWH style."""

from __future__ import annotations

import re
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC_DIR = ROOT / "include/sys/float"
OLD_PUBLIC = ROOT / "include/sys/float.hpp"
DST_INC = ROOT / "include/float"
DST_SRC = ROOT / "src/float"

AWH_HEADER = """/**
 * @file {filename}
 * @date 2026-07-22
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * @copyright Copyright © 2026
 *
 * Адаптировано из fast_float (https://github.com/fastfloat/fast_float)
 * Copyright 2021 The fast_float authors — MIT License.
 */

"""

# Longer English comment blocks -> Russian Doxygen (substring replace)
COMMENT_REPLACEMENTS: list[tuple[str, str]] = [
	(
		"When mapping numbers from decimal to binary,\n"
		" * we go from w * 10^q to m * 2^p but we have\n"
		" * 10^q = 5^q * 2^q, so effectively\n"
		" * we are trying to match\n"
		" * w * 2^q * 5^q to m * 2^p. Thus the powers of two\n"
		" * are not a concern since they can be represented\n"
		" * exactly using the binary notation, only the powers of five\n"
		" * affect the binary significand.",
		"@brief Таблицы степеней пяти для преобразования decimal → binary\n"
		" *\n"
		" * @details При отображении чисел из десятичной системы в двоичную\n"
		" *          мы переходим от w * 10^q к m * 2^p. Так как 10^q = 5^q * 2^q,\n"
		" *          степени двойки представимы точно, и на мантиссу влияют\n"
		" *          только степени пяти.",
	),
	(
		"Returns true if the floating-pointing rounding mode is to 'nearest'.\n"
		" * It is the default on most system. This function is meant to be inexpensive.\n"
		" * Credit : @mwalcott3",
		"@brief Проверяет, что режим округления — к ближайшему (to nearest)\n"
		" *\n"
		" * @details По умолчанию используется на большинстве систем.\n"
		" *          Функция должна быть дешёвой. Идея: @mwalcott3.\n"
		" *\n"
		" * @return true, если округление FE_TONEAREST",
	),
	(
		"Special case +inf, -inf, nan, infinity, -infinity.\n"
		" * The case comparisons could be made much faster given that we know that the\n"
		" * strings a null-free and fixed.",
		"@brief Разбор специальных значений +inf/-inf/nan/infinity\n"
		" *\n"
		" * @tparam T  тип числа с плавающей точкой\n"
		" * @tparam UC тип символа\n"
		" *\n"
		" * @param first начало строки\n"
		" * @param last  конец строки\n"
		" * @param value ссылка на результат\n"
		" * @param fmt   допустимый формат\n"
		" * @return      результат разбора",
	),
	(
		"This will compute or rather approximate w * 5**q and return a pair of 64-bit\n"
		" * words approximating the result, with the \"high\" part corresponding to the\n"
		" * most significant bits and the low part corresponding to the least significant\n"
		" * bits.",
		"@brief Приближённо вычисляет w * 5^q\n"
		" *\n"
		" * @details Возвращает пару 64-битных слов: high — старшие биты, low — младшие.",
	),
	(
		"Bias so we can get the real exponent with an invalid AdjustedMantissa.",
		"Смещение для получения реального экспонента при невалидной AdjustedMantissa.",
	),
	(
		"used for BinaryFormatTables<T>::maxMantissa",
		"Используется для BinaryFormatTables<T>::maxMantissa.",
	),
	(
		"Powers of five from 5^-342 all the way to 5^308 rounded toward one.",
		"Степени пяти от 5^-342 до 5^308, округлённые к единице.",
	),
	(
		"adjust for deprecated feature macros",
		"Корректирует формат с учётом устаревших feature-макросов.",
	),
	(
		"Like fromChars, but accepts an `options` argument to govern number parsing.\n"
		" * Both for floating-point types and integer types.",
		"@brief Расширенный разбор числа с опциями\n"
		" *\n"
		" * @details Работает и для чисел с плавающей точкой, и для целых.",
	),
	(
		"fromChars for integer types.",
		"@brief Разбор целого числа из строки",
	),
]

LINE_COMMENT_MAP: dict[str, str] = {
	"// a negative value indicates an invalid result": "// Отрицательное значение означает невалидный результат",
	"// used when fegetround() == FE_TONEAREST": "// Используется при fegetround() == FE_TONEAREST",
	"// we copy it so that it gets loaded at most once.": "// Копируем, чтобы значение загрузилось не более одного раза",
	"// be optimistic": "// Оптимистичная инициализация",
	"// assume first < last, so dereference without checks;": "// Предполагаем first < last, разыменование без проверок",
	"// C++17 20.19.3.(7.1) explicitly forbids '+' sign here": "// C++17 20.19.3.(7.1) явно запрещает знак '+' здесь",
	"// valid nan(n-char-seq-opt)": "// Валидный nan(n-char-seq-opt)",
	"// forbidden char, not nan(n-char-seq-opt)": "// Запрещённый символ, это не nan(n-char-seq-opt)",
	"// Which number formats are accepted": "// Какие форматы числа допускаются",
	"// The character used as decimal point": "// Символ десятичной точки",
	"// The base used for integers": "// Основание системы счисления для целых",
	"// RFC 8259: https://datatracker.ietf.org/doc/html/rfc8259#section-6": "// RFC 8259: https://datatracker.ietf.org/doc/html/rfc8259#section-6",
	"// Extension of RFC 8259 where, e.g., \"inf\" and \"nan\" are allowed.": "// Расширение RFC 8259: допускаются, например, \"inf\" и \"nan\"",
}


def spaces_to_tabs(text: str) -> str:
	"""Convert leading 2-space indents to tabs (fast_float style -> AWH)."""
	out_lines = []
	for line in text.splitlines():
		m = re.match(r"^( +)(.*)$", line)
		if m:
			spaces = len(m.group(1))
			# treat groups of 2 spaces as one indent level
			tabs = spaces // 2
			rem = spaces % 2
			line = ("\t" * tabs) + (" " * rem) + m.group(2)
		out_lines.append(line)
	return "\n".join(out_lines) + ("\n" if text.endswith("\n") else "")


def strip_old_header(text: str) -> str:
	"""Remove previously generated file banner / include guard start stays."""
	# Drop leading /** ... */ banner if present
	text = re.sub(r"^/\*\*.*?\*/\s*", "", text, count=1, flags=re.S)
	return text


def fix_namespace(text: str) -> str:
	text = text.replace("namespace awh::floating {", "namespace awh {\n\tnamespace floating {")
	text = text.replace("} // namespace awh::floating", "\t} // namespace floating\n} // namespace awh")
	# indent content that was directly under awh::floating by one extra tab
	# Done coarsely: after conversion, re-indent body of floating namespace is hard.
	# Instead indent everything between the two namespace opens by +1 tab for non-empty lines
	# that aren't the namespace lines themselves — applied in postprocess_namespace_body
	return text


def postprocess_namespace_body(text: str) -> str:
	"""Add one tab to lines inside `namespace awh { namespace floating { ... }`."""
	lines = text.splitlines()
	out = []
	depth_floating = 0
	for line in lines:
		stripped = line.strip()
		if stripped == "namespace floating {":
			out.append(line)
			depth_floating += 1
			continue
		if stripped.startswith("} // namespace floating"):
			depth_floating = max(0, depth_floating - 1)
			out.append(line)
			continue
		if depth_floating > 0 and line.strip() != "":
			# already may have tabs from spaces_to_tabs; add one more for awh wrapper
			if not line.startswith("\tnamespace") and not line.startswith("namespace"):
				line = "\t" + line
		out.append(line)
	return "\n".join(out) + "\n"


def translate_comments(text: str) -> str:
	for old, new in COMMENT_REPLACEMENTS:
		text = text.replace(old, new)
	for old, new in LINE_COMMENT_MAP.items():
		text = text.replace(old, new)
	return text


def update_includes(text: str, filename: str) -> str:
	replacements = {
		'"detect.hpp"': '"common.hpp"',  # merged
		'"api.hpp"': '"parse.hpp"',
		'"common.hpp"': '"common.hpp"',
		'"table.hpp"': '"table.hpp"',
		'"ascii.hpp"': '"ascii.hpp"',
		'"bigint.hpp"': '"bigint.hpp"',
		'"decimal.hpp"': '"decimal.hpp"',
		'"digits.hpp"': '"digits.hpp"',
		'"parse.hpp"': '"parse.hpp"',
	}
	# Public float.hpp used "float/api.hpp"
	text = text.replace('#include "float/api.hpp"', '#include "parse.hpp"')
	text = text.replace('#include "float/parse.hpp"', '#include "parse.hpp"')
	for a, b in replacements.items():
		text = text.replace(f"#include {a}", f"#include {b}")
	return text


def extract_table(table_text: str) -> tuple[str, str]:
	"""Split table.hpp into declaration header + cpp definition."""
	# Extract array body between powerOfFive128[...] = { ... };
	m = re.search(
		r"constexpr static uint64_t powerOfFive128\[numberOfEntries\] = \{(\n.*?)\n\s*\};",
		table_text,
		flags=re.S,
	)
	if not m:
		raise RuntimeError("powerOfFive128 array not found")
	array_body = m.group(1)

	header = AWH_HEADER.format(filename="table.hpp") + """/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_FLOAT_TABLE__
#define __AWH_FLOAT_TABLE__

/**
 * Стандартные заголовочные файлы
 */
#include <cstdint>

/**
 * Подключаем общие определения модуля
 */
#include "common.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён модуля чисел с плавающей точкой
	 *
	 */
	namespace floating {
		/**
		 * @brief Пространство имён таблиц степеней
		 *
		 */
		namespace powers {
			/**
			 * @brief Наименьший показатель степени пяти
			 *
			 */
			constexpr int smallestPowerOfFive = BinaryFormat <double>::smallestPowerOfTen();
			/**
			 * @brief Наибольший показатель степени пяти
			 *
			 */
			constexpr int largestPowerOfFive = BinaryFormat <double>::largestPowerOfTen();
			/**
			 * @brief Количество элементов таблицы (пары high/low)
			 *
			 */
			constexpr int numberOfEntries = 2 * (largestPowerOfFive - smallestPowerOfFive + 1);
			/**
			 * @brief Таблица степеней пяти в формате пар 64-битных слов
			 *
			 */
			extern const uint64_t powerOfFive128[numberOfEntries];
		} // namespace powers
	} // namespace floating
} // namespace awh

#endif // __AWH_FLOAT_TABLE__
"""

	cpp = AWH_HEADER.format(filename="table.cpp") + f"""/**
 * Подключаем заголовочные файлы проекта
 */
#include <float/table.hpp>

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {{
	/**
	 * @brief Пространство имён модуля чисел с плавающей точкой
	 *
	 */
	namespace floating {{
		/**
		 * @brief Пространство имён таблиц степеней
		 *
		 */
		namespace powers {{
			/**
			 * @brief Таблица степеней пяти от 5^-342 до 5^308
			 *
			 * @details Значения округлены к единице и хранятся парами (high, low).
			 */
			const uint64_t powerOfFive128[numberOfEntries] = {{{array_body}
			}};
		}} // namespace powers
	}} // namespace floating
}} // namespace awh
"""
	return header, cpp


def process_generic(filename: str, text: str) -> str:
	text = strip_old_header(text)
	# remove include of detect — merge: replace include detect with nothing if common already has macros
	text = re.sub(r'#include "detect\.hpp"\n', "", text)
	text = update_includes(text, filename)
	text = translate_comments(text)
	text = fix_namespace(text)
	text = spaces_to_tabs(text)
	text = postprocess_namespace_body(text)
	# Prepend AWH header + keep guards
	if not text.lstrip().startswith("/**"):
		text = AWH_HEADER.format(filename=filename) + text
	else:
		# ensure file banner is AWH
		text = re.sub(r"^/\*\*.*?\*/\s*", AWH_HEADER.format(filename=filename), text, count=1, flags=re.S)
	# Ensure guard comment style
	text = re.sub(r"#endif\s*$", f"#endif // file {filename}", text, count=1, flags=re.M)
	return text


def merge_detect_into_common(common: str, detect: str) -> str:
	detect_body = strip_old_header(detect)
	detect_body = re.sub(r"#ifndef .*?\n#define .*?\n", "", detect_body, count=1)
	detect_body = re.sub(r"#endif.*", "", detect_body).strip() + "\n"
	# Insert detect macros after includes / before version macros in common
	common = strip_old_header(common)
	common = re.sub(r'#include "detect\.hpp"\n', "", common)
	# Place feature detect right after standard includes block start
	marker = "#include <system_error>\n"
	if marker in common:
		common = common.replace(marker, marker + "\n" + detect_body + "\n", 1)
	else:
		common = detect_body + "\n" + common
	return common


def write_public_header() -> None:
	content = AWH_HEADER.format(filename="float.hpp") + """/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_FLOATING__
#define __AWH_FLOATING__

/**
 * Подключаем внутреннюю реализацию парсера
 */
#include "parse.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Класс конвертации чисел с плавающей точкой и целых из строки
	 *
	 */
	typedef class Floating {
		public:
			/**
			 * @brief Формат разбора числовой строки
			 *
			 */
			using format_t = floating::format_t;
			/**
			 * @brief Результат разбора числовой строки
			 *
			 * @tparam UC тип символа исходной строки
			 */
			template <typename UC>
			using result_t = floating::result_t <UC>;
			/**
			 * @brief Опции разбора числовой строки
			 *
			 * @tparam UC тип символа исходной строки
			 */
			template <typename UC>
			using options_t = floating::options_t <UC>;
		public:
			/**
			 * @brief Метод разбора числа с плавающей точкой из строки
			 *
			 * @tparam T  тип числа с плавающей точкой
			 * @tparam UC тип символа исходной строки
			 *
			 * @param first  указатель на начало строки
			 * @param last   указатель на конец строки
			 * @param value  ссылка на результат
			 * @param format допустимый формат записи числа
			 * @return       результат разбора
			 */
			template <typename T, typename UC = char, typename = AWH_FLOAT_ENABLE_IF(floating::IsSupportedFloat <T>::value)>
			static floating::result_t <UC> fromChars(const UC * first, const UC * last, T & value, const floating::format_t format = floating::format_t::GENERAL) noexcept {
				// Выполняем разбор числа
				return floating::fromChars(first, last, value, format);
			}
			/**
			 * @brief Метод разбора целого числа из строки
			 *
			 * @tparam T  тип целого числа
			 * @tparam UC тип символа исходной строки
			 *
			 * @param first указатель на начало строки
			 * @param last  указатель на конец строки
			 * @param value ссылка на результат
			 * @param base  основание системы счисления
			 * @return      результат разбора
			 */
			template <typename T, typename UC = char, typename = AWH_FLOAT_ENABLE_IF(floating::IsSupportedInteger <T>::value)>
			static floating::result_t <UC> fromChars(const UC * first, const UC * last, T & value, const int base = 10) noexcept {
				// Выполняем разбор целого числа
				return floating::fromChars(first, last, value, base);
			}
			/**
			 * @brief Метод расширенного разбора числа из строки
			 *
			 * @tparam T  тип числа
			 * @tparam UC тип символа исходной строки
			 *
			 * @param first   указатель на начало строки
			 * @param last    указатель на конец строки
			 * @param value   ссылка на результат
			 * @param options опции разбора
			 * @return        результат разбора
			 */
			template <typename T, typename UC = char>
			static floating::result_t <UC> fromCharsAdvanced(const UC * first, const UC * last, T & value, const floating::options_t <UC> options) noexcept {
				// Выполняем расширенный разбор числа
				return floating::fromCharsAdvanced(first, last, value, options);
			}
	} floating_t;
};

#endif // __AWH_FLOATING__
"""
	(DST_INC / "float.hpp").write_text(content, encoding="utf-8")


def extract_rounds_to_nearest(parse_text: str) -> tuple[str, str]:
	"""Move roundsToNearest out of header into common.cpp declaration/impl."""
	pattern = re.compile(
		r"/\*\*.*?Returns true if the floating-pointing rounding mode.*?\*/\s*"
		r"AWH_FLOAT_INLINE bool roundsToNearest\(\) noexcept \{.*?\n\}",
		re.S,
	)
	# After translation the English phrase may be gone — match by function name
	pattern = re.compile(
		r"/\*\*.*?roundsToNearest.*?\*/\s*"
		r"AWH_FLOAT_INLINE bool roundsToNearest\(\) noexcept \{.*?\n\}"
		r"|"
		r"/\*\*.*?режим округления.*?\*/\s*"
		r"AWH_FLOAT_INLINE bool roundsToNearest\(\) noexcept \{.*?\n\}"
		r"|"
		r"AWH_FLOAT_INLINE bool roundsToNearest\(\) noexcept \{.*?\n\}",
		re.S,
	)
	m = pattern.search(parse_text)
	impl = ""
	if m:
		impl = m.group(0)
		# declaration stub in header
		decl = (
			"\t\t\t/**\n"
			"\t\t\t * @brief Проверяет, что режим округления — к ближайшему\n"
			"\t\t\t *\n"
			"\t\t\t * @return true, если округление FE_TONEAREST\n"
			"\t\t\t */\n"
			"\t\t\tbool roundsToNearest() noexcept;\n"
		)
		# Keep unindented replacement; indentation fixed later
		parse_text = parse_text[: m.start()] + "bool roundsToNearest() noexcept;\n" + parse_text[m.end() :]
		# Build cpp impl: strip INLINE, keep body
		impl_body = re.sub(r"^/\*\*.*?\*/\s*", "", impl, count=1, flags=re.S)
		impl_body = impl_body.replace("AWH_FLOAT_INLINE ", "")
		impl = impl_body
	return parse_text, impl


def main() -> None:
	if not SRC_DIR.exists():
		raise SystemExit(f"missing source dir {SRC_DIR}")

	if DST_INC.exists():
		shutil.rmtree(DST_INC)
	if DST_SRC.exists():
		shutil.rmtree(DST_SRC)
	DST_INC.mkdir(parents=True)
	DST_SRC.mkdir(parents=True)

	detect = (SRC_DIR / "detect.hpp").read_text(encoding="utf-8")
	common = (SRC_DIR / "common.hpp").read_text(encoding="utf-8")
	common = merge_detect_into_common(common, detect)
	common = process_generic("common.hpp", common)
	# Add section comments for includes if missing
	if "Стандартные заголовочные файлы" not in common:
		common = common.replace(
			"#include <cfloat>",
			"/**\n * Стандартные заголовочные файлы\n */\n#include <cfloat>",
			1,
		)
	(DST_INC / "common.hpp").write_text(common, encoding="utf-8")

	table_raw = (SRC_DIR / "table.hpp").read_text(encoding="utf-8")
	table_hdr, table_cpp = extract_table(table_raw)
	(DST_INC / "table.hpp").write_text(table_hdr, encoding="utf-8")
	(DST_SRC / "table.cpp").write_text(table_cpp, encoding="utf-8")

	# Process remaining headers
	for name in ("ascii.hpp", "bigint.hpp", "decimal.hpp", "digits.hpp", "parse.hpp"):
		raw = (SRC_DIR / name).read_text(encoding="utf-8")
		# parse.hpp previously included via api.hpp which included common then parse
		if name == "parse.hpp":
			# Ensure it doesn't depend on api; add forward decls from api if needed
			raw = raw.replace('#include "parse_number.h"', "")
			# Remove include of api cycle
			if '#include "common.hpp"' not in raw:
				raw = '#include "common.hpp"\n' + raw
			# Extract roundsToNearest before heavy processing of comments
			raw, rounds_impl = extract_rounds_to_nearest(raw)
			processed = process_generic(name, raw)
			# Fix declaration indentation roughly — leave as-is if present
			(DST_INC / name).write_text(processed, encoding="utf-8")

			cpp = AWH_HEADER.format(filename="common.cpp") + """/**
 * Стандартные заголовочные файлы
 */
#include <cfloat>
#include <limits>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <float/parse.hpp>

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён модуля чисел с плавающей точкой
	 *
	 */
	namespace floating {
		/**
		 * @brief Пространство внутренних деталей реализации
		 *
		 */
		namespace detail {
"""
			if rounds_impl:
				# indent impl for AWH
				body = rounds_impl
				body = body.replace("AWH_FLOAT_INLINE ", "")
				# ensure function is in detail namespace
				if "roundsToNearest" in body:
					# indent lines
					indented = "\n".join(
						("\t\t\t" + ln) if ln.strip() else ln
						for ln in spaces_to_tabs(body).splitlines()
					)
					cpp += indented + "\n"
			cpp += """\t\t} // namespace detail
	} // namespace floating
} // namespace awh
"""
			(DST_SRC / "common.cpp").write_text(cpp, encoding="utf-8")
		else:
			(DST_INC / name).write_text(process_generic(name, raw), encoding="utf-8")

	write_public_header()

	# Remove obsolete api.hpp (not written)
	# Delete old locations
	if OLD_PUBLIC.exists():
		OLD_PUBLIC.unlink()
	if SRC_DIR.exists():
		shutil.rmtree(SRC_DIR)

	print("Migrated to", DST_INC)
	print("Sources in", DST_SRC)
	for p in sorted(DST_INC.rglob("*")):
		if p.is_file():
			print(" ", p.relative_to(ROOT), p.stat().st_size)
	for p in sorted(DST_SRC.rglob("*")):
		if p.is_file():
			print(" ", p.relative_to(ROOT), p.stat().st_size)


if __name__ == "__main__":
	main()
