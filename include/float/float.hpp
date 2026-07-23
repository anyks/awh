/**
 * @file: float.hpp
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
			template <typename T, typename UC = char, typename = floating::enableIf_t <floating::IsSupportedFloat <T>::value>>
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
			template <typename T, typename UC = char, typename = floating::enableIf_t <floating::IsSupportedInteger <T>::value>>
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
