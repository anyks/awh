/**
 * @file nwt.cpp
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
 * @brief Реализация тестовой фикстуры модуля определения типов сетевых адресов —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <memory>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "nwt.hpp"

/**
 * @brief Метод инициализации тестовой среды
 *
 */
void NwtFixture::SetUp(){
	// Создаём объект работы со списком параметров URL
	this->_nwt = std::make_unique <awh::nwt_t> ();
}

/**
 * @brief Метод очистки тестовой среды
 *
 */
void NwtFixture::TearDown() {}
