/**
 * @file procre.cpp
 * @date 2026-01-26
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
 * @brief Реализация тестовой фикстуры модуля резольвера процессов —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл
 */
#include "procre.hpp"

/**
 * @brief Метод настройки тестового окружения
 *
 */
void ProcreFixture::SetUp(){
	// Создаём объект фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
	// Создаём объект логов
	this->_log = std::make_unique <awh::log_t> (this->_fmk.get());
	// Создаём объект для работы с процессами
	this->_procre = std::make_unique <awh::procre_t> (this->_log.get());
	// Создаём объект для работы с сетевыми адресами
	this->_addr = std::make_unique <awh::net_addr_t> (this->_fmk.get(), this->_log.get());
}

/**
 * @brief Метод очистки тестового окружения
 *
 */
void ProcreFixture::TearDown() {
	// Сбрасываем объект для работы с процессами
	this->_procre.reset();
	// Сбрасываем объект для работы с сетевыми адресами
	this->_addr.reset();
	// Сбрасываем объект логов
	this->_log.reset();
	// Сбрасываем объект фреймворка
	this->_fmk.reset();
}
