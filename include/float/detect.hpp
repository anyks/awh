/**
 * @file: detect.hpp
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

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_FLOAT_DETECT__
#define __AWH_FLOAT_DETECT__

/**
 * Стандартные заголовочные файлы
 *
 * @details <version> даёт feature-test макросы (__cpp_*) при наличии.
 */
#ifdef __has_include
	#if __has_include(<version>)
		#include <version>
	#endif
#endif

/**
 * Поддержка std::bit_cast
 */
#if defined(__cpp_lib_bit_cast) && (__cpp_lib_bit_cast >= 201806L)
	#define AWH_FLOAT_HAS_BIT_CAST 1
#else
	#define AWH_FLOAT_HAS_BIT_CAST 0
#endif

/**
 * Поддержка std::is_constant_evaluated
 */
#if defined(__cpp_lib_is_constant_evaluated) && (__cpp_lib_is_constant_evaluated >= 201811L)
	#define AWH_FLOAT_HAS_IS_CONSTANT_EVALUATED 1
#else
	#define AWH_FLOAT_HAS_IS_CONSTANT_EVALUATED 0
#endif

/**
 * Поддержка релевантных constexpr-возможностей C++20
 *
 * @details Требуются is_constant_evaluated, bit_cast и constexpr-алгоритмы
 *          (std::copy / std::fill). Проект собирается как C++17, поэтому
 *          полный constexpr-путь доступен только при наличии этих фич.
 */
#if AWH_FLOAT_HAS_IS_CONSTANT_EVALUATED && AWH_FLOAT_HAS_BIT_CAST && \
	defined(__cpp_lib_constexpr_algorithms) && (__cpp_lib_constexpr_algorithms >= 201806L)
	#define AWH_FLOAT_CONSTEXPR20 constexpr
#else
	#define AWH_FLOAT_CONSTEXPR20
#endif

#endif // __AWH_FLOAT_DETECT__
