/**
 * @file queue.cpp
 * @date 2026-02-07
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
 * @brief Реализация тестовой фикстуры сетевой очереди —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright Copyright © 2026
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
void NetworkQueueFixture::SetUp(){
	// Создаём объекты фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
	// Создаём объект логгера
	this->_log = std::make_unique <awh::log_t> (this->_fmk.get());
	// Создаём объект очереди
	this->_queue = std::make_unique <awh::net_queue_t> (this->_fmk.get(), this->_log.get());
}

/**
 * @brief Метод очистки тестовой среды
 *
 */
void NetworkQueueFixture::TearDown() {}
