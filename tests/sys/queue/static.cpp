/**
 * @file: static.cpp
 * @date: 2025-12-13
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "queue.hpp"

/**
 * @brief Метод инициализации тестовой среды
 *
 */
TEST_F(QueueFixture, CreateQueueTest){
	// Проверяем создание объекта очереди
	ASSERT_TRUE(this->_queue != nullptr);
	// Очищаем объект очереди
	this->_queue.reset();
	// Проверяем удаление объекта очереди
	ASSERT_TRUE(this->_queue == nullptr);
}

/**
 * @brief Метод очистки тестовой среды
 *
 */
TEST_F(QueueFixture, ResetAndCreateQueueTest){
	// Проверяем создание объекта очереди
	ASSERT_TRUE(this->_queue != nullptr);
	// Очищаем объект очереди
	this->_queue.reset();
	// Проверяем удаление объекта очереди
	ASSERT_TRUE(this->_queue == nullptr);
	// Создаём объект очереди
	this->_queue = std::make_unique <awh::queue_t> (this->_fmk.get(), this->_log.get());
	// Проверяем создание объекта очереди
	ASSERT_TRUE(this->_queue != nullptr);
}

/**
 * @brief Метод очистки тестовой среды
 *
 */
TEST_F(QueueFixture, ReCreateQueueTest){
	// Проверяем создание объекта очереди
	ASSERT_TRUE(this->_queue != nullptr);
	// Cоздаём объект очереди заново
	this->_queue = std::make_unique <awh::queue_t> (this->_fmk.get(), this->_log.get());
	// Проверяем создание объекта очереди
	ASSERT_TRUE(this->_queue != nullptr);
}
