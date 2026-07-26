/**
 * @file: callback.cpp
 * @date: 2026-01-22
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация тестовой фикстуры модуля функций обратного вызова —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "callback.hpp"

/**
 * @brief Метод настройки тестового окружения
 *
 */
void CallbackFixture::SetUp(){
	// Создаём объект фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
	// Создаём объект логов
	this->_log = std::make_unique <awh::log_t> (this->_fmk.get());
	// Создаём объект модуля обратного вызова
	this->_callback = std::make_unique <awh::callback_t> (this->_fmk.get(), this->_log.get());
}

/**
 * @brief Метод очистки тестового окружения
 *
 */
void CallbackFixture::TearDown() {}
