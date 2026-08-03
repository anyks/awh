/**
 * @file: charset.hpp
 * @date: 2026-08-03
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл тестовой фикстуры модуля перекодировки — объявление класса
 *        фикстуры Google Test, подготавливающего и освобождающего тестовое окружение
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_CHARSET_TESTS__
#define __AWH_CHARSET_TESTS__

/**
 * Подключаем общие заголовки тестов
 */
#include "../main.hpp"

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstdint>

/**
 * Подключаем модуль перекодировки
 */
#include "../../include/charset/charset.hpp"

/**
 * @brief Тестовый класс модуля перекодировки
 *
 */
class CharsetFixture : public testing::Test {
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
};

#endif // __AWH_CHARSET_TESTS__
