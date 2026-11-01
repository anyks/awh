/**
 * @file static.cpp
 * @date 2025-12-14
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
 * @brief Статические тесты модуля партнёрских сокетов — проверка получения и установки
 *        предела файловых дескрипторов
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "fds.hpp"

/**
 * @brief Тест набора сетевых тестов
 *
 */
TEST_F(FdsFixture, FdsSuiteTest){
	// Получаем лимит файловых дескрипторов
	auto limits = awh::fds::limit();
	// Проверяем, что лимит файловых дескрипторов получен
	ASSERT_TRUE(limits.first > 0);
	ASSERT_TRUE(limits.second >= limits.first);
	// Пытаемся установить лимит файловых дескрипторов на текущее значение
	bool result = awh::fds::limit(limits.second);
	// Проверяем, что лимит файловых дескрипторов установлен
	ASSERT_TRUE(result);
	// Записываем в лог справку по файловым дескрипторам
	awh::fds::help(limits.first, limits.second);
}
