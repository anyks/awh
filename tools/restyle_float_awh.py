#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Safely restyle include/float to AWH look, extract tables to src/float."""

from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
INC = ROOT / "include/float"
SRC = ROOT / "src/float"

BANNER = """/**
 * @file: {filename}
 * @date: 2026-07-22
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 *
 * Адаптировано из fast_float (https://github.com/fastfloat/fast_float)
 * Copyright 2021 The fast_float authors — MIT License.
 */

"""

COMMENT_BLOCKS: list[tuple[str, str]] = [
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
		"@brief Проверяет, что режим округления — к ближайшему\n"
		" *\n"
		" * @return true, если округление FE_TONEAREST",
	),
	(
		"Special case +inf, -inf, nan, infinity, -infinity.\n"
		" * The case comparisons could be made much faster given that we know that the\n"
		" * strings a null-free and fixed.",
		"@brief Разбор специальных значений inf/nan\n"
		" *\n"
		" * @tparam T  тип числа с плавающей точкой\n"
		" * @tparam UC тип символа",
	),
	(
		"This will compute or rather approximate w * 5**q and return a pair of 64-bit\n"
		" * words approximating the result, with the \"high\" part corresponding to the\n"
		" * most significant bits and the low part corresponding to the least significant\n"
		" * bits.",
		"@brief Приближённо вычисляет произведение w * 5^q\n"
		" *\n"
		" * @details Возвращает пару 64-битных слов (high/low).",
	),
	(
		"Like fromChars, but accepts an `options` argument to govern number parsing.\n"
		" * Both for floating-point types and integer types.",
		"@brief Расширенный разбор числа с опциями\n"
		" *\n"
		" * @details Поддерживает числа с плавающей точкой и целые.",
	),
	(
		"fromChars for integer types.",
		"@brief Разбор целого числа из строки",
	),
	(
		"Bias so we can get the real exponent with an invalid AdjustedMantissa.",
		"Смещение для получения реального экспонента при невалидной AdjustedMantissa.",
	),
	(
		"used for BinaryFormatTables<T>::maxMantissa",
		"Используется в BinaryFormatTables<T>::maxMantissa.",
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
		"This function parses the character sequence [first,last) for a number. It\n"
		" * parses floating-point numbers expecting a locale-indepent format equivalent\n"
		" * to what is used by std::strtod in the default (\"C\") locale. The resulting\n"
		" * floating-point value is the closest floating-point values (using either float\n"
		" * or double), using the \"round to even\" convention for values that would\n"
		" * otherwise fall right in-between two values. That is, we provide exact parsing\n"
		" * according to the IEEE standard.\n"
		" *\n"
		" * Given a successful parse, the pointer (`ptr`) in the returned value is set to\n"
		" * point right after the parsed number, and the `value` referenced is set to the\n"
		" * parsed value. In case of error, the returned `ec` contains a representative\n"
		" * error, otherwise the default (`std::errc()`) value is stored.\n"
		" *\n"
		" * The implementation does not throw and does not allocate memory (e.g., with\n"
		" * `new` or `malloc`).\n"
		" *\n"
		" * Like the C++17 standard, the `awh::floating::fromChars` functions take an\n"
		" * optional last argument of the type `awh::floating::format_t`. It is a bitset\n"
		" * value: we check whether `fmt & awh::floating::format_t::FIXED` and `fmt &\n"
		" * awh::floating::format_t::SCIENTIFIC` are set to determine whether we allow\n"
		" * the fixed point and scientific notation respectively. The default is\n"
		" * `awh::floating::format_t::GENERAL` which allows both `fixed` and\n"
		" * `scientific`.",
		"@brief Разбирает число с плавающей точкой из диапазона [first, last)\n"
		" *\n"
		" * @details Формат соответствует std::strtod в локали \"C\". Результат —\n"
		" *          ближайшее значение float/double с округлением round-to-even (IEEE).\n"
		" *          При успехе ptr указывает за разобранное число, ec пуст.\n"
		" *          Реализация не бросает исключений и не выделяет память.\n"
		" *\n"
		" * @tparam T  тип числа с плавающей точкой\n"
		" * @tparam UC тип символа\n"
		" *\n"
		" * @param first начало строки\n"
		" * @param last  конец строки\n"
		" * @param value ссылка на результат\n"
		" * @param fmt   допустимый формат записи\n"
		" * @return      результат разбора",
	),
	(
		"This function multiplies an integer number by a power of 10 and returns\n"
		" * the result as a double precision floating-point value that is correctly\n"
		" * rounded. The resulting floating-point value is the closest floating-point\n"
		" * value, using the \"round to nearest, tie to even\" convention for values that\n"
		" * would otherwise fall right in-between two values. That is, we provide exact\n"
		" * conversion according to the IEEE standard.\n"
		" *\n"
		" * On overflow infinity is returned, on underflow 0 is returned.\n"
		" *\n"
		" * The implementation does not throw and does not allocate memory (e.g., with\n"
		" * `new` or `malloc`).",
		"@brief Умножает целое на степень 10 с корректным округлением в double\n"
		" *\n"
		" * @details При переполнении возвращает infinity, при антипереполнении — 0.\n"
		" *          Реализация не бросает исключений и не выделяет память.\n"
		" *\n"
		" * @param mantissa         мантисса\n"
		" * @param decimalExponent  десятичный экспонент\n"
		" * @return                 результат преобразования",
	),
	(
		"This function is a template overload of `integerTimesPow10()`\n"
		" * that returns a floating-point value of type `T` that is one of\n"
		" * supported floating-point types (e.g. `double`, `float`).",
		"@brief Шаблонный вариант integerTimesPow10 для типа T\n"
		" *\n"
		" * @tparam T поддерживаемый тип с плавающей точкой",
	),
]

LINE_MAP = {
	"// a negative value indicates an invalid result": "// Отрицательное значение означает невалидный результат",
	"// used when fegetround() == FE_TONEAREST": "// Используется при fegetround() == FE_TONEAREST",
	"// we copy it so that it gets loaded at most once.": "// Копируем, чтобы значение загрузилось один раз",
	"// be optimistic": "// Оптимистичная инициализация",
	"// assume first < last, so dereference without checks;": "// Предполагаем first < last",
	"// C++17 20.19.3.(7.1) explicitly forbids '+' sign here": "// C++17 запрещает знак '+' здесь",
	"// valid nan(n-char-seq-opt)": "// Валидный nan(n-char-seq-opt)",
	"// forbidden char, not nan(n-char-seq-opt)": "// Запрещённый символ",
	"// Which number formats are accepted": "// Допустимые форматы числа",
	"// The character used as decimal point": "// Символ десятичной точки",
	"// The base used for integers": "// Основание системы счисления",
	"// Extension of RFC 8259 where, e.g., \"inf\" and \"nan\" are allowed.": "// Расширение RFC 8259: допускаются inf/nan",
	"// Largest integer value v so that (5**index * v) <= 1<<53.": "// Наибольшее целое v, для которого (5**index * v) <= 1<<53",
	"// 0x20000000000000 == 1 << 53": "// 0x20000000000000 == 1 << 53",
}


def spaces_to_tabs(text: str) -> str:
	out = []
	for line in text.splitlines():
		m = re.match(r"^( +)(.*)$", line)
		if m:
			n = len(m.group(1))
			line = ("\t" * (n // 2)) + (" " * (n % 2)) + m.group(2)
		out.append(line)
	return "\n".join(out) + "\n"


def set_banner(text: str, filename: str) -> str:
	text = re.sub(r"^/\*\*.*?\*/\s*", "", text, count=1, flags=re.S)
	return BANNER.format(filename=filename) + text


def translate(text: str) -> str:
	for a, b in COMMENT_BLOCKS:
		text = text.replace(a, b)
	for a, b in LINE_MAP.items():
		text = text.replace(a, b)
	return text


def nest_namespace(text: str) -> str:
	"""Convert `namespace awh::floating` to nested AWH form without reindenting body."""
	text = text.replace(
		"namespace awh::floating {",
		"namespace awh {\n\tnamespace floating {",
	)
	text = text.replace(
		"} // namespace awh::floating",
		"\t} // namespace floating\n} // namespace awh",
	)
	return text


def extract_table() -> None:
	path = INC / "table.hpp"
	text = path.read_text(encoding="utf-8")
	m = re.search(
		r"constexpr static uint64_t powerOfFive128\[numberOfEntries\] = \{(\n.*?)\n\s*\};",
		text,
		flags=re.S,
	)
	if not m:
		raise RuntimeError("powerOfFive128 not found")
	body = m.group(1)

	header = BANNER.format(filename="table.hpp") + """/**
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
	cpp = BANNER.format(filename="table.cpp") + f"""/**
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
			const uint64_t powerOfFive128[numberOfEntries] = {{{body}
			}};
		}} // namespace powers
	}} // namespace floating
}} // namespace awh
"""
	SRC.mkdir(parents=True, exist_ok=True)
	path.write_text(header, encoding="utf-8")
	(SRC / "table.cpp").write_text(cpp, encoding="utf-8")


def extract_rounds() -> None:
	path = INC / "parse.hpp"
	text = path.read_text(encoding="utf-8")
	pat = re.compile(
		r"/\*\*.*?Returns true if the floating-pointing rounding mode.*?\*/\s*"
		r"AWH_FLOAT_INLINE bool roundsToNearest\(\) noexcept \{.*?\n\}",
		re.S,
	)
	m = pat.search(text)
	if not m:
		# already translated?
		pat = re.compile(
			r"/\*\*.*?режим округления.*?\*/\s*"
			r"AWH_FLOAT_INLINE bool roundsToNearest\(\) noexcept \{.*?\n\}",
			re.S,
		)
		m = pat.search(text)
	if not m:
		pat = re.compile(
			r"AWH_FLOAT_INLINE bool roundsToNearest\(\) noexcept \{.*?\n\}",
			re.S,
		)
		m = pat.search(text)
	if not m:
		print("WARN: roundsToNearest not extracted")
		return

	impl = m.group(0)
	text = text[: m.start()] + "bool roundsToNearest() noexcept;\n" + text[m.end() :]
	path.write_text(text, encoding="utf-8")

	impl = re.sub(r"^/\*\*.*?\*/\s*", "", impl, count=1, flags=re.S)
	impl = impl.replace("AWH_FLOAT_INLINE ", "")
	impl = spaces_to_tabs(impl)

	cpp = BANNER.format(filename="common.cpp") + """/**
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
			/**
			 * @brief Проверяет, что режим округления — к ближайшему
			 *
			 * @return true, если округление FE_TONEAREST
			 */
"""
	for ln in impl.splitlines():
		cpp += ("\t\t\t" + ln if ln.strip() else "") + "\n"
	cpp += """\t\t} // namespace detail
	} // namespace floating
} // namespace awh
"""
	(SRC / "common.cpp").write_text(cpp, encoding="utf-8")


def write_public() -> None:
	(INC / "float.hpp").write_text(
		BANNER.format(filename="float.hpp")
		+ """/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_FLOATING__
#define __AWH_FLOATING__

/**
 * Подключаем внутреннюю реализацию парсера
 */
#include "api.hpp"

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
""",
		encoding="utf-8",
	)


def restyle_file(path: Path) -> None:
	text = path.read_text(encoding="utf-8")
	text = set_banner(text, path.name)
	text = translate(text)
	text = nest_namespace(text)
	text = spaces_to_tabs(text)
	# Section comment for standard includes (once)
	if path.name != "float.hpp" and "Стандартные заголовочные файлы" not in text:
		text = re.sub(
			r"(#define __AWH_[A-Z_]+__\n\n)",
			r"\1/**\n * Стандартные заголовочные файлы\n */\n",
			text,
			count=1,
		)
	path.write_text(text, encoding="utf-8")


def main() -> None:
	SRC.mkdir(parents=True, exist_ok=True)

	# 1) extract table first (from freshly ported table.hpp)
	extract_table()

	# 2) extract roundsToNearest before restyle translation changes its docblock optionally
	# extract_rounds()  # disabled: keep inline in header

	# 3) restyle all headers except table.hpp (already written) and float.hpp (written next)
	for path in sorted(INC.glob("*.hpp")):
		if path.name in {"table.hpp", "float.hpp"}:
			continue
		restyle_file(path)

	write_public()
	print("restyle complete")
	for p in sorted(INC.glob("*.hpp")):
		print(f"  {p.relative_to(ROOT)} {len(p.read_text().splitlines())} lines")
	for p in sorted(SRC.glob("*.cpp")):
		print(f"  {p.relative_to(ROOT)} {len(p.read_text().splitlines())} lines")


if __name__ == "__main__":
	main()
