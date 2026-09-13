/**
 * @file fs.cpp
 * @date 2026-01-25
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
 * @brief Реализация тестовой фикстуры модуля работы с файловой системой —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "fs.hpp"

/**
 * @brief Метод настройки тестовой фикстуры
 *
 */
void FSFixture::SetUp(){
	// Создаём объект модуля работы с файловой системой
	this->_fs = std::make_unique <awh::fs_t> ();
}

/**
 * @brief Метод очистки тестовой фикстуры
 *
 */
void FSFixture::TearDown() {
	// Сбрасываем объект системы
	this->_fs.reset();
	// Сбрасываем объект логгера
	// Сбрасываем объект фреймворка
}
