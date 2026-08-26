/**
 * @file fiber.hpp
 * @date 2026-08-26
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
 * @brief Заголовочный файл тестов модуля волокон
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_FIBER_TESTS__
#define __AWH_FIBER_TESTS__

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/sys/fiber.hpp"

/**
 * @brief Класс фикстуры для тестов волокон
 *
 */
class FiberFixture : public testing::Test {
	protected:
		// Объект фреймворка
		std::unique_ptr <awh::fmk_t> _fmk;
		// Объект работы с логами
		std::unique_ptr <awh::log_t> _log;
	public:
		/**
		 * @brief Метод настройки тестовой фикстуры
		 *
		 */
		void SetUp();
		/**
		 * @brief Метод очистки тестовой фикстуры
		 *
		 */
		void TearDown();
};

#endif // __AWH_FIBER_TESTS__
