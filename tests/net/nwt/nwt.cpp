/**
 * @file: nwt.cpp
 * @date: 2025-12-14
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
#include "nwt.hpp"

/**
 * @brief Метод инициализации тестовой среды
 *
 */
void NwtFixture::SetUp(){
	// Создаём объект фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
	// Создаём объект логгера
	this->_log = std::make_unique <awh::log_t> (this->_fmk.get());
	// Создаём объект работы со списком параметров URL
	this->_nwt = std::make_unique <awh::nwt_t> (this->_log.get());
}

/**
 * @brief Метод очистки тестовой среды
 *
 */
void NwtFixture::TearDown() {}
