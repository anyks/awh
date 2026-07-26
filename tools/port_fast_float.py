#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Port contrib/include/fast_float into include/sys/float with AWH naming."""

from __future__ import annotations

import re
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "contrib/include/fast_float"
DST = ROOT / "include/float"

FILE_MAP = {
	"constexpr_feature_detect.h": "detect.hpp",
	"float_common.h": "common.hpp",
	"fast_table.h": "table.hpp",
	"ascii_number.h": "ascii.hpp",
	"bigint.h": "bigint.hpp",
	"decimal_to_binary.h": "decimal.hpp",
	"digit_comparison.h": "digits.hpp",
	"parse_number.h": "parse.hpp",
	"fast_float.h": "api.hpp",
}

# Longer keys first for safe replacement.
RENAMES: list[tuple[str, str]] = [
	# macros / helpers
	("FASTFLOAT_DETAIL_MUST_DEFINE_CONSTEXPR_VARIABLE", "AWH_FLOAT_MUST_DEFINE_CONSTEXPR_VARIABLE"),
	("FASTFLOAT_SIMD_DISABLE_WARNINGS", "AWH_FLOAT_SIMD_DISABLE_WARNINGS"),
	("FASTFLOAT_SIMD_RESTORE_WARNINGS", "AWH_FLOAT_SIMD_RESTORE_WARNINGS"),
	("FASTFLOAT_HAS_IS_CONSTANT_EVALUATED", "AWH_FLOAT_HAS_IS_CONSTANT_EVALUATED"),
	("FASTFLOAT_IF_CONSTEXPR17", "AWH_FLOAT_IF_CONSTEXPR17"),
	("FASTFLOAT_CONSTEXPR20", "AWH_FLOAT_CONSTEXPR20"),
	("FASTFLOAT_CONSTEXPR14", "AWH_FLOAT_CONSTEXPR14"),
	("FASTFLOAT_IS_CONSTEXPR", "AWH_FLOAT_IS_CONSTEXPR"),
	("FASTFLOAT_HAS_BIT_CAST", "AWH_FLOAT_HAS_BIT_CAST"),
	("FASTFLOAT_VISUAL_STUDIO", "AWH_FLOAT_VISUAL_STUDIO"),
	("FASTFLOAT_IS_BIG_ENDIAN", "AWH_FLOAT_IS_BIG_ENDIAN"),
	("FASTFLOAT_HAS_SIMD", "AWH_FLOAT_HAS_SIMD"),
	("FASTFLOAT_ENABLE_IF", "AWH_FLOAT_ENABLE_IF"),
	("FASTFLOAT_DEBUG_ASSERT", "AWH_FLOAT_DEBUG_ASSERT"),
	("FASTFLOAT_ASSERT", "AWH_FLOAT_ASSERT"),
	("FASTFLOAT_TRY", "AWH_FLOAT_TRY"),
	("FASTFLOAT_VERSION_MAJOR", "AWH_FLOAT_VERSION_MAJOR"),
	("FASTFLOAT_VERSION_MINOR", "AWH_FLOAT_VERSION_MINOR"),
	("FASTFLOAT_VERSION_PATCH", "AWH_FLOAT_VERSION_PATCH"),
	("FASTFLOAT_VERSION_STR", "AWH_FLOAT_VERSION_STR"),
	("FASTFLOAT_VERSION", "AWH_FLOAT_VERSION"),
	("FASTFLOAT_STRINGIZE_IMPL", "AWH_FLOAT_STRINGIZE_IMPL"),
	("FASTFLOAT_STRINGIZE", "AWH_FLOAT_STRINGIZE"),
	("FASTFLOAT_64BIT", "AWH_FLOAT_64BIT"),
	("FASTFLOAT_32BIT", "AWH_FLOAT_32BIT"),
	("FASTFLOAT_SSE2", "AWH_FLOAT_SSE2"),
	("FASTFLOAT_NEON", "AWH_FLOAT_NEON"),
	("FASTFLOAT_64BIT_LIMB", "AWH_FLOAT_64BIT_LIMB"),
	("FASTFLOAT_32BIT_LIMB", "AWH_FLOAT_32BIT_LIMB"),
	("FASTFLOAT_ALLOWS_LEADING_PLUS", "AWH_FLOAT_ALLOWS_LEADING_PLUS"),
	("FASTFLOAT_SKIP_WHITE_SPACE", "AWH_FLOAT_SKIP_WHITE_SPACE"),
	("fastfloat_really_inline", "AWH_FLOAT_INLINE"),
	("fastfloat_strncasecmp", "strncasecmpAscii"),
	# include guards / includes
	("FASTFLOAT_CONSTEXPR_FEATURE_DETECT_H", "__AWH_FLOAT_DETECT__"),
	("FASTFLOAT_FLOAT_COMMON_H", "__AWH_FLOAT_COMMON__"),
	("FASTFLOAT_FAST_TABLE_H", "__AWH_FLOAT_TABLE__"),
	("FASTFLOAT_ASCII_NUMBER_H", "__AWH_FLOAT_ASCII__"),
	("FASTFLOAT_BIGINT_H", "__AWH_FLOAT_BIGINT__"),
	("FASTFLOAT_DECIMAL_TO_BINARY_H", "__AWH_FLOAT_DECIMAL__"),
	("FASTFLOAT_DIGIT_COMPARISON_H", "__AWH_FLOAT_DIGITS__"),
	("FASTFLOAT_PARSE_NUMBER_H", "__AWH_FLOAT_PARSE__"),
	("FASTFLOAT_FAST_FLOAT_H", "__AWH_FLOAT_API__"),
	# types / API
	("from_chars_float_advanced", "fromCharsFloatAdvanced"),
	("from_chars_int_advanced", "fromCharsIntAdvanced"),
	("from_chars_advanced_caller", "FromCharsAdvancedCaller"),
	("from_chars_advanced", "fromCharsAdvanced"),
	("from_chars_caller", "FromCharsCaller"),
	("from_chars_result_t", "result_t"),
	("from_chars_result", "Result"),
	("from_chars", "fromChars"),
	("integer_times_pow10", "integerTimesPow10"),
	("parse_options_t", "options_t"),
	("parse_options", "Options"),
	("chars_format", "format_t"),
	("parsed_number_string_t", "parsedNumber_t"),
	("parsed_number_string", "ParsedNumber"),
	("binary_format_lookup_tables", "BinaryFormatTables"),
	("binary_format", "BinaryFormat"),
	("adjusted_mantissa", "AdjustedMantissa"),
	("is_supported_integer_type", "IsSupportedInteger"),
	("is_supported_float_type", "IsSupportedFloat"),
	("is_supported_char_type", "IsSupportedChar"),
	("equiv_uint_t", "equivUint_t"),
	("powers_template", "PowersTemplate"),
	("power_of_five_128", "powerOfFive128"),
	("powers_of_ten_uint64", "powersOfTenUint64"),
	("powers_of_ten", "powersOfTen"),
	("basic_fortran_fmt", "basicFortranFmt"),
	("basic_json_fmt", "basicJsonFmt"),
	("json_or_infnan", "JSON_OR_INFNAN"),
	("allow_leading_plus", "ALLOW_LEADING_PLUS"),
	("skip_white_space", "SKIP_WHITE_SPACE"),
	("no_infnan", "NO_INFNAN"),
	# enum-ish values used as format_t::scientific etc — converted after to UPPER
	# functions
	("cpp20_and_in_constexpr", "cpp20AndInConstexpr"),
	("adjust_for_feature_macros", "adjustForFeatureMacros"),
	("rounds_to_nearest", "roundsToNearest"),
	("parse_number_string", "parseNumberString"),
	("parse_int_string", "parseIntString"),
	("parse_infnan", "parseInfNan"),
	("parse_mantissa", "parseMantissa"),
	("parse_eight_digits_unrolled", "parseEightDigitsUnrolled"),
	("parse_if_eight_digits_unrolled", "parseIfEightDigitsUnrolled"),
	("parse_eight_digits", "parseEightDigits"),
	("parse_one_digit", "parseOneDigit"),
	("simd_parse_if_eight_digits_unrolled", "simdParseIfEightDigitsUnrolled"),
	("loop_parse_if_eight_digits", "loopParseIfEightDigits"),
	("is_made_of_eight_digits_fast", "isMadeOfEightDigitsFast"),
	("simd_read8_to_u64", "simdRead8ToU64"),
	("read8_to_u64", "read8ToU64"),
	("has_simd_opt", "hasSimdOpt"),
	("report_parse_error", "reportParseError"),
	("compute_product_approximation", "computeProductApproximation"),
	("compute_error_scaled", "computeErrorScaled"),
	("compute_error", "computeError"),
	("compute_float", "computeFloat"),
	("compute_product", "computeProduct"),
	("negative_digit_comp", "negativeDigitComp"),
	("positive_digit_comp", "positiveDigitComp"),
	("digit_comp", "digitComp"),
	("round_nearest_tie_even", "roundNearestTieEven"),
	("round_up_bigint", "roundUpBigint"),
	("round_down", "roundDown"),
	("to_extended_halfway", "toExtendedHalfway"),
	("to_extended", "toExtended"),
	("to_float", "toFloat"),
	("full_multiplication", "fullMultiplication"),
	("leading_zeroes_generic", "leadingZeroesGeneric"),
	("leading_zeroes", "leadingZeroes"),
	("leading_zero", "leadingZero"),
	("ch_to_digit", "chToDigit"),
	("max_digits_u64", "maxDigitsU64"),
	("min_safe_u64", "minSafeU64"),
	("str_const_nan", "strConstNan"),
	("str_const_inf", "strConstInf"),
	("int_cmp_zeros", "intCmpZeros"),
	("int_cmp_len", "intCmpLen"),
	("is_space", "isSpace"),
	("is_integer", "isInteger"),
	("is_negative", "isNegative"),
	("is_halfway", "isHalfway"),
	("is_above", "isAbove"),
	("is_empty", "isEmpty"),
	("is_odd", "isOdd"),
	("is_truncated", "isTruncated"),
	("small_add_from", "smallAddFrom"),
	("large_add_from", "largeAddFrom"),
	("small_mul", "smallMul"),
	("large_mul", "largeMul"),
	("long_mul", "longMul"),
	("small_add", "smallAdd"),
	("scalar_add", "scalarAdd"),
	("scalar_mul", "scalarMul"),
	("uint64_hi64", "uint64Hi64"),
	("uint32_hi64", "uint32Hi64"),
	("empty_hi64", "emptyHi64"),
	("shl_limbs", "shlLimbs"),
	("shl_bits", "shlBits"),
	("bit_length", "bitLength"),
	("set_len", "setLen"),
	("try_push", "tryPush"),
	("try_extend", "tryExtend"),
	("try_resize", "tryResize"),
	("push_unchecked", "pushUnchecked"),
	("extend_unchecked", "extendUnchecked"),
	("resize_unchecked", "resizeUnchecked"),
	("add_native", "addNative"),
	("skip_zeros", "skipZeros"),
	("clinger_fast_path_impl", "clingerFastPathImpl"),
	("umul128_generic", "umul128Generic"),
	("has_decimal_point", "hasDecimalPoint"),
	("has_leading_zeros", "hasLeadingZeros"),
	("too_many_digits", "tooManyDigits"),
	("scientific_exponent", "scientificExponent"),
	("decimal_exponent", "decimalExponent"),
	("decimal_point", "decimalPoint"),
	("digit_count", "digitCount"),
	("exp_number", "expNumber"),
	("start_digits", "startDigits"),
	("start_num", "startNum"),
	("input_num", "inputNum"),
	("int_end", "intEnd"),
	("frac_end", "fracEnd"),
	("real_digits", "realDigits"),
	("real_exp", "realExp"),
	("theor_digits", "theorDigits"),
	("theor_exp", "theorExp"),
	("pow5_exp", "pow5Exp"),
	("pow2_exp", "pow2Exp"),
	("neg_exp", "negExp"),
	("sci_exp", "sciExp"),
	("max_exp", "maxExp"),
	("new_len", "newLen"),
	("limb_span", "limbSpan"),
	("limb_bits", "limbBits"),
	("byte_span", "byteSpan"),
	("bigint_limbs", "bigintLimbs"),
	("bigint_bits", "bigintBits"),
	("large_power_of_5", "largePowerOf5"),
	("small_power_of_5", "smallPowerOf5"),
	("large_length", "largeLength"),
	("large_step", "largeStep"),
	("small_step", "smallStep"),
	("max_native", "maxNative"),
	("maxdigits_u64", "maxDigitsU64Table"),
	("number_of_entries", "numberOfEntries"),
	("smallest_power_of_five", "smallestPowerOfFive"),
	("largest_power_of_five", "largestPowerOfFive"),
	("smallest_power_of_ten", "smallestPowerOfTen"),
	("largest_power_of_ten", "largestPowerOfTen"),
	("mantissa_explicit_bits", "mantissaExplicitBits"),
	("minimum_exponent", "minimumExponent"),
	("infinite_power", "infinitePower"),
	("sign_index", "signIndex"),
	("max_exponent_fast_path", "maxExponentFastPath"),
	("min_exponent_fast_path", "minExponentFastPath"),
	("max_exponent_round_to_even", "maxExponentRoundToEven"),
	("min_exponent_round_to_even", "minExponentRoundToEven"),
	("max_mantissa_fast_path", "maxMantissaFastPath"),
	("max_mantissa", "maxMantissa"),
	("exact_power_of_ten", "exactPowerOfTen"),
	("max_digits", "maxDigits"),
	("exponent_mask", "exponentMask"),
	("mantissa_mask", "mantissaMask"),
	("hidden_bit_mask", "hiddenBitMask"),
	("mantissa_shift", "mantissaShift"),
	("truncated_bits", "truncatedBits"),
	("invalid_am_bias", "invalidAmBias"),
	("minimal_nineteen_digit_integer", "minimalNineteenDigitInteger"),
	("constant_55555", "constant55555"),
	("precision_mask", "precisionMask"),
	("bit_precision", "bitPrecision"),
	("last_bit", "lastBit"),
	("space_lut", "spaceLut"),
	("int_luts", "intLuts"),
	("int_type", "intType"),
	("pow5_tables", "pow5Tables"),
	("end_of_integer_part", "END_OF_INTEGER_PART"),
	("missing_exponential_part", "MISSING_EXPONENTIAL_PART"),
	("missing_integer_after_sign", "MISSING_INTEGER_AFTER_SIGN"),
	("missing_integer_or_dot_after_sign", "MISSING_INTEGER_OR_DOT_AFTER_SIGN"),
	("no_digits_in_fractional_part", "NO_DIGITS_IN_FRACTIONAL_PART"),
	("no_digits_in_integer_part", "NO_DIGITS_IN_INTEGER_PART"),
	("no_digits_in_mantissa", "NO_DIGITS_IN_MANTISSA"),
	("leading_zeros_in_integer_part", "LEADING_ZEROS_IN_INTEGER_PART"),
	("location_of_e", "LOCATION_OF_E"),
	("no_error", "NO_ERROR"),
	("parse_error", "parseError"),
	("mushtak_lemire", "mushtakLemire"),
	("actual_mixedcase", "actualMixedcase"),
	("expected_lowercase", "expectedLowercase"),
	("adbc_carry", "adbcCarry"),
	("am_b", "amB"),
]

# format_t enum member renames (after chars_format -> format_t)
FORMAT_ENUM_VALUES = [
	("scientific", "SCIENTIFIC"),
	("fixed", "FIXED"),
	("hex", "HEX"),
	("fortran", "FORTRAN"),
	("general", "GENERAL"),
	("json", "JSON"),
]


HEADER_PREFIX = """/**
 * @file: {filename}
 * @date: 2026-07-22
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 *
 * Adapted from fast_float (https://github.com/fastfloat/fast_float)
 * Copyright 2021 The fast_float authors — MIT License.
 */

"""


def word_replace(text: str, old: str, new: str) -> str:
	"""Replace identifier-like occurrences of old with new."""
	return re.sub(rf"\b{re.escape(old)}\b", new, text)


INCLUDE_MAP = {
	'"constexpr_feature_detect.h"': '"detect.hpp"',
	'"float_common.h"': '"common.hpp"',
	'"fast_table.h"': '"table.hpp"',
	'"ascii_number.h"': '"ascii.hpp"',
	'"bigint.h"': '"bigint.hpp"',
	'"decimal_to_binary.h"': '"decimal.hpp"',
	'"digit_comparison.h"': '"digits.hpp"',
	'"parse_number.h"': '"parse.hpp"',
	'"fast_float.h"': '"api.hpp"',
}


def transform(content: str) -> str:
	# namespace
	content = content.replace("namespace fast_float", "namespace awh::floating")
	content = content.replace("} // namespace fast_float", "} // namespace awh::floating")
	content = content.replace("// namespace fast_float", "// namespace awh::floating")
	# Qualified calls / comments still using the old namespace
	content = content.replace("fast_float::", "awh::floating::")

	for old, new in INCLUDE_MAP.items():
		content = content.replace(old, new)

	for old, new in RENAMES:
		content = word_replace(content, old, new)

	# format_t::value renames — only after format_t is in place
	for old, new in FORMAT_ENUM_VALUES:
		content = re.sub(rf"\bformat_t::{old}\b", f"format_t::{new}", content)
		# enum definition body: "scientific =", "fixed =", etc.
		content = re.sub(rf"(enum class format_t[^{{]*\{{[^}}]*?)\b{old}\b(\s*=)", rf"\1{new}\2", content, flags=re.S)

	# Second pass for enum members inside enum class format_t block more robustly
	def enum_fix(m: re.Match[str]) -> str:
		body = m.group(0)
		for old, new in FORMAT_ENUM_VALUES + [
			("ALLOW_LEADING_PLUS", "ALLOW_LEADING_PLUS"),
			("SKIP_WHITE_SPACE", "SKIP_WHITE_SPACE"),
			("NO_INFNAN", "NO_INFNAN"),
			("JSON_OR_INFNAN", "JSON_OR_INFNAN"),
		]:
			if old.islower() or old in ("scientific", "fixed", "hex", "fortran", "general", "json"):
				body = re.sub(rf"\b{old}\b", new, body)
		return body

	content = re.sub(
		r"enum class format_t\s*:\s*uint64_t\s*\{.*?\};",
		enum_fix,
		content,
		flags=re.S,
	)

	# Fix accidental renames of std::errc values if any
	# resultOutOfRange was parse error enum in ascii — keep
	return content


def main() -> None:
	if DST.exists():
		shutil.rmtree(DST)
	DST.mkdir(parents=True)

	for src_name, dst_name in FILE_MAP.items():
		src = SRC / src_name
		dst = DST / dst_name
		raw = src.read_text(encoding="utf-8")
		out = HEADER_PREFIX.format(filename=dst_name) + transform(raw)
		dst.write_text(out, encoding="utf-8")
		print(f"wrote {dst.relative_to(ROOT)} ({len(out.splitlines())} lines)")

	print("done")


if __name__ == "__main__":
	main()
