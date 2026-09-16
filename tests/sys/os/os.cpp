/**
 * @file os.cpp
 * @date 2025-12-13
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
 * @brief Реализация тестовой фикстуры модуля работы с операционной системой —
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
#include "os.hpp"

/**
 * @brief Метод настройки тестовой фикстуры
 *
 */
void OSFixture::SetUp(){
	// Создаём объект модуля работы с операционной системой
	this->_os = std::make_unique <awh::os_t> ();
}

/**
 * @brief Метод очистки тестовой фикстуры
 *
 */
void OSFixture::TearDown() {}
