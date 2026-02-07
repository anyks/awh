/**
 * @file: static.cpp
 * @date: 2026-02-07
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "queue.hpp"

/**
 * @brief Метод инициализации тестовой среды
 *
 */
TEST_F(NetworkQueueFixture, CreateQueueTest){
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
TEST_F(NetworkQueueFixture, ResetAndCreateQueueTest){
	// Проверяем создание объекта очереди
	ASSERT_TRUE(this->_queue != nullptr);
	// Очищаем объект очереди
	this->_queue.reset();
	// Проверяем удаление объекта очереди
	ASSERT_TRUE(this->_queue == nullptr);
	// Создаём объект очереди
	this->_queue = std::make_unique <awh::net_queue_t> (this->_fmk.get(), this->_log.get());
	// Проверяем создание объекта очереди
	ASSERT_TRUE(this->_queue != nullptr);
}

/**
 * @brief Метод очистки тестовой среды
 *
 */
TEST_F(NetworkQueueFixture, ReCreateQueueTest){
	// Проверяем создание объекта очереди
	ASSERT_TRUE(this->_queue != nullptr);
	// Cоздаём объект очереди заново
	this->_queue = std::make_unique <awh::net_queue_t> (this->_fmk.get(), this->_log.get());
	// Проверяем создание объекта очереди
	ASSERT_TRUE(this->_queue != nullptr);
}
