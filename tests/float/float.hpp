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
 */

#ifndef __AWH_FLOAT_TESTS__
#define __AWH_FLOAT_TESTS__

/**
 * Подключаем общие заголовки тестов
 */
#include "../main.hpp"

/**
 * Стандартные заголовочные файлы
 */
#include <cmath>
#include <limits>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <system_error>

/**
 * Подключаем модуль float (публичный API и внутренности для unit-покрытия)
 */
#include "../../include/float/float.hpp"
#include "../../include/float/ascii.hpp"
#include "../../include/float/bigint.hpp"
#include "../../include/float/common.hpp"
#include "../../include/float/decimal.hpp"
#include "../../include/float/digits.hpp"

/**
 * @brief Тестовый класс модуля float
 *
 */
class FloatFixture : public testing::Test {
	protected:
		/**
		 * @brief Метод настройки тестового окружения
		 *
		 */
		void SetUp() override;
		/**
		 * @brief Метод очистки тестового окружения
		 *
		 */
		void TearDown() override;
	public:
		/**
		 * @brief Сравнивает биты двух double
		 *
		 * @param a первое значение
		 * @param b второе значение
		 * @return  true при побитовом равенстве
		 */
		static bool sameBits(const double a, const double b) noexcept;
		/**
		 * @brief Сравнивает биты двух float
		 *
		 * @param a первое значение
		 * @param b второе значение
		 * @return  true при побитовом равенстве
		 */
		static bool sameBits(const float a, const float b) noexcept;
	public:
		/**
		 * @brief Разбирает double через floating_t и сверяет со strtod
		 *
		 * @param text исходная строка
		 */
		static void expectDoubleMatchesStrtod(const char * text) noexcept;
		/**
		 * @brief Разбирает float через floating_t и сверяет со strtof
		 *
		 * @param text исходная строка
		 */
		static void expectFloatMatchesStrtof(const char * text) noexcept;
};

#endif // __AWH_FLOAT_TESTS__
