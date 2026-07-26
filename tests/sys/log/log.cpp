/**
 * @file: log.cpp
 * @date: 2025-12-12
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
#include "log.hpp"

/**
 * @brief Метод настройки тестового окружения
 *
 */
void LogFixture::SetUp(){
	// Создаём объекты фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
	// Создаём объект логов
	this->_log = std::make_unique <awh::log_t> (this->_fmk.get());
}

/**
 * @brief Метод очистки тестового окружения
 *
 */
void LogFixture::TearDown() {}
