/**
 * @file os.hpp
 * @date 2025-12-13
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
 * @brief Заголовочный файл тестовой фикстуры модуля работы с операционной системой —
 *        объявление класса фикстуры Google Test, подготавливающего и освобождающего тестовое окружение набора тестов
 *
 * @copyright Copyright © 2025
 *
 */
 
#ifndef __AWH_OS_TESTS__
#define __AWH_OS_TESTS__

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/sys/os.hpp"

/**
 * @brief Класс фикстуры для тестов модуля работы с операционной системой
 *
 */
class OSFixture : public testing::Test {
	protected:
		// Объект работы с операционной системой
		std::unique_ptr <awh::os_t> _os;
		// Объекты фреймворка
		std::unique_ptr <awh::fmk_t> _fmk;
		// Объект логов
		std::unique_ptr <awh::log_t> _log;
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

#endif // __AWH_OS_TESTS__
