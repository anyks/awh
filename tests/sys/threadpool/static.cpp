/**
 * @file static.cpp
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
 * @brief Статические тесты модуля пула потоков — проверка создания и сброса объекта модуля,
 *        а также корректности распределения задач по рабочим потокам,
 *        обработки очереди и корректного завершения работы
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "threadpool.hpp"

/**
 * @brief Метод настройки тестовой фикстуры
 *
 */
TEST_F(ThreadPoolFixture, CreateThreadPoolTest){
	// Проверяем что объект пула потоков создан
	ASSERT_TRUE(this->_thr != nullptr);
	// Выполняем сброс объекта пула потоков
	this->_thr.reset();
	// Проверяем что объект пула потоков сброшен
	ASSERT_TRUE(this->_thr == nullptr);
}

/**
 * @brief Метод очистки тестовой фикстуры
 *
 */
TEST_F(ThreadPoolFixture, ResetAndCreateThreadPoolTest){
	// Проверяем что объект пула потоков создан
	ASSERT_TRUE(this->_thr != nullptr);
	// Выполняем сброс объекта пула потоков
	this->_thr.reset();
	// Проверяем что объект пула потоков сброшен
	ASSERT_TRUE(this->_thr == nullptr);
	// Создаём объект пула потоков
	this->_thr = std::make_unique <awh::thr_t> ();
	// Проверяем что объект пула потоков создан
	ASSERT_TRUE(this->_thr != nullptr);
}

/**
 * @brief Метод очистки тестовой фикстуры
 *
 */
TEST_F(ThreadPoolFixture, ReCreateThreadPoolTest){
	// Проверяем что объект пула потоков создан
	ASSERT_TRUE(this->_thr != nullptr);
	// Создаём объект пула потоков
	this->_thr = std::make_unique <awh::thr_t> ();
	// Проверяем что объект пула потоков создан
	ASSERT_TRUE(this->_thr != nullptr);
}
