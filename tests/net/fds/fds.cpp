/**
 * @file: fds.cpp
 * @date: 2025-12-14
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация тестовой фикстуры модуля партнёрских сокетов —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "fds.hpp"

/**
 * @brief Метод инициализации тестовой среды
 *
 */
void FdsFixture::SetUp(){
	// Создаём объект фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
	// Создаём объект логгера
	this->_log = std::make_unique <awh::log_t> (this->_fmk.get());
	// Создаём объект работы с файловыми дескрипторами
	this->_fds = std::make_unique <awh::fds_t> (this->_log.get());
}

/**
 * @brief Метод очистки тестовой среды
 *
 */
void FdsFixture::TearDown() {}
