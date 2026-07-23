/**
 * @file: table.hpp
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
