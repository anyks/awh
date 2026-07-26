/**
 * @file: lexical.hpp
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
 */

#ifndef __AWH_LEXICAL_TESTS__
#define __AWH_LEXICAL_TESTS__

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
 * Подключаем модуль lexical (публичный API и внутренности для unit-покрытия)
 */
#include "../../include/lexical/lexical.hpp"
#include "../../include/lexical/parser.hpp"
#include "../../include/lexical/bigint.hpp"
#include "../../include/lexical/common.hpp"
#include "../../include/lexical/decimal.hpp"
#include "../../include/lexical/digits.hpp"

/**
 * @brief Тестовый класс модуля lexical
 *
 */
class LexicalFixture : public testing::Test {
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
		 * @brief Разбирает double через lexical_t и сверяет со strtod
		 *
		 * @param text исходная строка
		 */
		static void expectDoubleMatchesStrtod(const char * text) noexcept;
		/**
		 * @brief Разбирает float через lexical_t и сверяет со strtof
		 *
		 * @param text исходная строка
		 */
		static void expectFloatMatchesStrtof(const char * text) noexcept;
};

#endif // __AWH_LEXICAL_TESTS__
