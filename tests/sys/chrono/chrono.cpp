/**
 * @file: chrono.cpp
 * @date: 2025-12-10
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация тестовой фикстуры модуля работы с датой и временем —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "chrono.hpp"

/**
 * @brief Метод настройки тестовой фикстуры
 *
 */
void ChronoFixture::SetUp(){
	// Создаём объект фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
	// Создаём объект логгера
	this->_log = std::make_unique <awh::log_t> (this->_fmk.get());
	// Создаём объект модуля работы с датой и временем
	this->_chrono = std::make_unique <awh::chrono_t> (this->_fmk.get(), this->_log.get());
}

/**
 * @brief Метод очистки тестовой фикстуры
 *
 */
void ChronoFixture::TearDown() {}
