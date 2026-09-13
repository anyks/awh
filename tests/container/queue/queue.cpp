/**
 * @file queue.cpp
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
 * @brief Реализация тестовой фикстуры бинарной очереди —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "queue.hpp"

/**
 * @brief Метод инициализации тестовой среды
 *
 */
void QueueFixture::SetUp(){
	// Создаём объект очереди
	this->_queue = std::make_unique <awh::queue_t> ();
}

/**
 * @brief Метод очистки тестовой среды
 *
 */
void QueueFixture::TearDown() {}
