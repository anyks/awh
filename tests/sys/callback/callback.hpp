/**
 * @file callback.hpp
 * @date 2026-01-22
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
 * @brief Заголовочный файл тестовой фикстуры модуля функций обратного вызова —
 *        объявление класса фикстуры Google Test, подготавливающего и освобождающего тестовое окружение набора тестов
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_CALLBACK_TESTS__
#define __AWH_CALLBACK_TESTS__

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/sys/fmk.hpp"
#include "../../../include/sys/log.hpp"
#include "../../../include/sys/callback.hpp"

/**
 * @brief Класс фикстуры для тестов модуля обратного вызова
 *
 */
class CallbackFixture : public testing::Test {
	protected:
		// Объект фреймворка
		std::unique_ptr <awh::fmk_t> _fmk;
		// Объект логов
		std::unique_ptr <awh::log_t> _log;
		// Объект модуля обратного вызова
		std::unique_ptr <awh::callback_t> _callback;
	public:
		/**
		 * @brief Метод инициализации тестовой среды
		 *
		 */
		void SetUp();
		/**
		 * @brief Метод очистки тестовой среды
		 *
		 */
		void TearDown();
};

#endif // __AWH_CALLBACK_TESTS__
