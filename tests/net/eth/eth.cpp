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
	// Создаём объект работы с Ethernet
	this->_eth = std::make_unique <awh::eth_t> ();
	// Инициализируем объект сетевого адреса
	this->_addr = std::make_unique <awh::net_addr_t> ();
}

/**
 * @brief Метод очистки тестовой среды
 *
 */
void EthFixture::TearDown() {}
