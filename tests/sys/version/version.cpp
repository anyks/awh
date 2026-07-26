/**
 * @file: version.cpp
 * @date: 2026-01-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация тестовой фикстуры модуля работы с версиями —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл
 */
#include "version.hpp"

/**
 * @brief Метод настройки тестового окружения
 *
 */
void VersionFixture::SetUp(){
	// Создаём объект версии
	this->_version = std::make_unique <awh::version_t> ();
}

/**
 * @brief Метод очистки тестового окружения
 *
 */
void VersionFixture::TearDown() {}
