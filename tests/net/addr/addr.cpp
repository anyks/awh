/**
 * @file: addr.cpp
 * @date: 2025-12-13
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация тестовой фикстуры модуля работы с сетевыми адресами —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "addr.hpp"

/**
 * @brief Метод инициализации тестовой среды
 *
 */
void NetFixture::SetUp(){
	// Инициализируем объект фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
	// Инициализируем объект логирования
	this->_log = std::make_unique <awh::log_t> (this->_fmk.get());
	// Инициализируем объект сетевого адреса
	this->_addr = std::make_unique <awh::net_addr_t> (this->_fmk.get(), this->_log.get());
}

/**
 * @brief Метод очистки тестовой среды
 *
 */
void NetFixture::TearDown() {}
