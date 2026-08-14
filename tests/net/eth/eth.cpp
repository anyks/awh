/**
 * @file eth.cpp
 * @date 2025-12-14
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
 * @brief Реализация тестовой фикстуры модуля работы с сетевым уровнем Ethernet —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "eth.hpp"

/**
 * @brief Метод инициализации тестовой среды
 *
 */
void EthFixture::SetUp(){
	// Создаём объект фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
	// Создаём объект логгера
	this->_log = std::make_unique <awh::log_t> (this->_fmk.get());
	// Создаём объект работы с Ethernet
	this->_eth = std::make_unique <awh::eth_t> (this->_fmk.get(), this->_log.get());
	// Инициализируем объект сетевого адреса
	this->_addr = std::make_unique <awh::net_addr_t> (this->_fmk.get(), this->_log.get());
}

/**
 * @brief Метод очистки тестовой среды
 *
 */
void EthFixture::TearDown() {}
