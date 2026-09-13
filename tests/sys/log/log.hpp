/**
 * @file log.hpp
 * @date 2025-12-12
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
 * @brief Заголовочный файл тестовой фикстуры модуля логирования — объявление класса фикстуры Google Test,
 *        подготавливающего и освобождающего тестовое окружение набора тестов
 *
 * @copyright Copyright © 2025
 *
 */
 
#ifndef __AWH_LOG_TESTS__
#define __AWH_LOG_TESTS__

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../main.hpp"
#include <sys/log.hpp>
#include <sys/macro/lib.hpp>

/**
 * @brief Тестовый класс для работы с логами
 *
 */
class LogFixture : public testing::Test {
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

#endif // __AWH_LOG_TESTS__
