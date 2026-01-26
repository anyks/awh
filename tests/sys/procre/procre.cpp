/**
 * @file: procre.cpp
 * @date: 2026-01-26
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
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
	// Создаём объект для работы с процессами
	this->_procre = std::make_unique <awh::procre_t> (nullptr);
}

/**
 * @brief Метод очистки тестового окружения
 *
 */
void ProcreFixture::TearDown() {}
