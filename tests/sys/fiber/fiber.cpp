/**
 * @file fiber.cpp
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
 * @brief Файл реализации фикстуры тестов модуля волокон
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "fiber.hpp"
#include <sys/log.hpp>

/**
 * @brief Метод настройки тестовой фикстуры
 *
 */
void FiberFixture::SetUp(){
	// Отключаем вывод журнала: проверки говорят сами за себя
	awh::log::level(awh::log::level_t::NONE);
}

/**
 * @brief Метод очистки тестовой фикстуры
 *
 */
void FiberFixture::TearDown(){
	// Освобождаем объект работы с логами
	// Освобождаем объект фреймворка
}
