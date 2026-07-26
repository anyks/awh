/**
 * @file: fmk.cpp
 * @date: 2025-12-07
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация тестовой фикстуры ядра фреймворка —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "fmk.hpp"

/**
 * @brief Метод настройки тестовой фикстуры
 *
 */
void FmkFixture::SetUp(){
	// Создаём объект фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
}

/**
 * @brief Метод очистки тестовой фикстуры
 *
 */
void FmkFixture::TearDown() {}
