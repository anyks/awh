/**
 * @file: fmk.hpp
 * @date: 2025-12-07
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл тестовой фикстуры ядра фреймворка — объявление класса фикстуры Google Test,
 *        подготавливающего и освобождающего тестовое окружение набора тестов
 *
 * @copyright: Copyright © 2025
 *
 */
 
#ifndef __AWH_FMK_TESTS__
#define __AWH_FMK_TESTS__

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/sys/fmk.hpp"

/**
 * @brief Класс фикстуры для тестов фреймворка
 *
 */
class FmkFixture : public testing::Test {
	protected:
		// Объект фреймворка
		std::unique_ptr <awh::fmk_t> _fmk;
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

#endif // __AWH_FMK_TESTS__
