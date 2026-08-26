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

/**
 * @brief Метод настройки тестовой фикстуры
 *
 */
void FiberFixture::SetUp(){
	// Создаём объект фреймворка
	this->_fmk = std::unique_ptr <awh::fmk_t> (new awh::fmk_t());
	// Создаём объект работы с логами
	this->_log = std::unique_ptr <awh::log_t> (new awh::log_t(this->_fmk.get()));
	// Отключаем вывод журнала: проверки говорят сами за себя
	this->_log->level(awh::log_t::level_t::NONE);
}

/**
 * @brief Метод очистки тестовой фикстуры
 *
 */
void FiberFixture::TearDown(){
	// Освобождаем объект работы с логами
	this->_log.reset(nullptr);
	// Освобождаем объект фреймворка
	this->_fmk.reset(nullptr);
}
