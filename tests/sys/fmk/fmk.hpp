/**
 * @file fmk.hpp
 * @date 2025-12-07
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
 * @brief Заголовочный файл тестовой фикстуры ядра фреймворка — объявление класса фикстуры Google Test,
 *        подготавливающего и освобождающего тестовое окружение набора тестов
 *
 * @copyright Copyright © 2025
 *
 */
 
#pragma once

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../main.hpp"
#include <sys/fmk.hpp>

/**
 * @brief Класс фикстуры для тестов фреймворка
 *
 */
class FmkFixture : public testing::Test {
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
