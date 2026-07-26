/**
 * @file: threadpool.cpp
 * @date: 2025-12-12
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
#include "threadpool.hpp"

/**
 * @brief Метод настройки тестовой фикстуры
 *
 */
void ThreadPoolFixture::SetUp(){
	// Создаём объект пула потоков
	this->_thr = std::make_unique <awh::thr_t> ();
}

/**
 * @brief Метод очистки тестовой фикстуры
 *
 */
void ThreadPoolFixture::TearDown() {}
