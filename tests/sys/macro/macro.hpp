/**
 * @file macro.hpp
 * @date 2026-08-05
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
 * @brief Заголовочный файл тестовой фикстуры защиты от макросов MS Windows —
 *        объявление класса фикстуры Google Test для набора тестов
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_MACRO_TESTS__
#define __AWH_MACRO_TESTS__

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../main.hpp"

/**
 * @brief Класс фикстуры для тестов защиты от макросов MS Windows
 *
 * @note Тестового окружения набору не требуется: проверка ведётся на этапе сборки
 *       и препроцессором, а фикстура заведена лишь ради единообразия с прочими наборами
 *
 */
class MacroFixture : public testing::Test {
	public:
		/**
		 * @brief Метод настройки тестового окружения
		 *
		 */
		void SetUp();
		/**
		 * @brief Метод очистки тестового окружения
		 *
		 */
		void TearDown();
};

#endif // __AWH_MACRO_TESTS__
