/**
 * @file: buffer.cpp
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
#include "buffer.hpp"

/**
 * @brief Метод настройки тестового окружения
 *
 */
void BufferFixture::SetUp(){
	// Создаём объект фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
	// Создаём объект логгера
	this->_log = std::make_unique <awh::log_t> (this->_fmk.get());
	// Создаём объект смартбуфера
	this->_buffer = std::make_unique <awh::buffer_t> (this->_fmk.get(), this->_log.get());
}

/**
 * @brief Метод очистки тестового окружения
 *
 */
void BufferFixture::TearDown() {}
