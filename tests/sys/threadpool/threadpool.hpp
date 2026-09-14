/**
 * @file threadpool.hpp
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
 * @brief Заголовочный файл тестовой фикстуры модуля пула потоков — объявление класса фикстуры Google Test,
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
#include "../../../include/sys/locker.hpp"
#include "../../../include/sys/threadpool.hpp"

/**
 * @brief Класс фикстуры для тестов пула потоков
 *
 */
class ThreadPoolFixture : public testing::Test {
	protected:
		// Объект пула потоков
		std::unique_ptr <awh::thr_t> _thr;
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
