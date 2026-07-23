/**
 * @file: api.hpp
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

#ifndef __AWH_FLOAT_API__
#define __AWH_FLOAT_API__

/**
 * Стандартные заголовочные файлы
 */
#include "common.hpp"

namespace awh {
	namespace floating {
/**
 * @brief Разбирает число с плавающей точкой из диапазона [first, last)
 *
 * @details Формат соответствует std::strtod в локали "C". Результат —
 *          ближайшее значение float/double с округлением round-to-even (IEEE).
 *          При успехе ptr указывает за разобранное число, ec пуст.
 *          Реализация не бросает исключений и не выделяет память.
 *
 * @tparam T  тип числа с плавающей точкой
 * @tparam UC тип символа
 *
 * @param first начало строки
 * @param last  конец строки
 * @param value ссылка на результат
 * @param fmt   допустимый формат записи
 * @return      результат разбора
 */
template <typename T, typename UC = char,
					typename = enableIf_t <IsSupportedFloat<T>::value>>
AWH_FLOAT_CONSTEXPR20 result_t<UC>
fromChars(UC const *first, UC const *last, T &value,
					 format_t fmt = format_t::GENERAL) noexcept;

/**
 * @brief Расширенный разбор числа с опциями
 *
 * @details Поддерживает числа с плавающей точкой и целые.
 */
template <typename T, typename UC = char>
AWH_FLOAT_CONSTEXPR20 result_t<UC>
fromCharsAdvanced(UC const *first, UC const *last, T &value,
										options_t<UC> options) noexcept;

/**
 * @brief Умножает целое на степень 10 с корректным округлением в double
 *
 * @details При переполнении возвращает infinity, при антипереполнении — 0.
 *          Реализация не бросает исключений и не выделяет память.
 *
 * @param mantissa         мантисса
 * @param decimalExponent  десятичный экспонент
 * @return                 результат преобразования
 */
AWH_FLOAT_CONSTEXPR20 inline double
integerTimesPow10(uint64_t mantissa, int decimalExponent) noexcept;
AWH_FLOAT_CONSTEXPR20 inline double
integerTimesPow10(int64_t mantissa, int decimalExponent) noexcept;

/**
 * @brief Шаблонный вариант integerTimesPow10 для типа T
 *
 * @tparam T поддерживаемый тип с плавающей точкой
 */
template <typename T>
AWH_FLOAT_CONSTEXPR20
		typename std::enable_if<IsSupportedFloat<T>::value, T>::type
		integerTimesPow10(uint64_t mantissa, int decimalExponent) noexcept;
template <typename T>
AWH_FLOAT_CONSTEXPR20
		typename std::enable_if<IsSupportedFloat<T>::value, T>::type
		integerTimesPow10(int64_t mantissa, int decimalExponent) noexcept;

/**
 * @brief Разбор целого числа из строки
 */
template <typename T, typename UC = char,
					typename = enableIf_t <IsSupportedInteger<T>::value>>
AWH_FLOAT_CONSTEXPR20 result_t<UC>
fromChars(UC const *first, UC const *last, T &value, int base = 10) noexcept;

	} // namespace floating
} // namespace awh

#include "parse.hpp"
#endif // __AWH_FLOAT_API__
