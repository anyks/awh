/**
 * @file version.hpp
 * @date 2026-01-26
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
 * @brief Заголовочный файл тестовой фикстуры модуля работы с версиями — объявление класса фикстуры Google Test,
 *        подготавливающего и освобождающего тестовое окружение набора тестов
 *
 * @copyright Copyright © 2026
 *
 */
 
#ifndef __AWH_VERSION_TESTS__
#define __AWH_VERSION_TESTS__

#include "../../main.hpp"
#include "../../../include/sys/version.hpp"

/**
 * @brief Тестовый класс для работы с версионированием
 *
 */
class VersionFixture : public testing::Test {
	protected:
		// Объект версии
		std::unique_ptr <awh::version_t> _version;
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

#endif // __AWH_VERSION_TESTS__
