/**
 * @file: uri.cpp
 * @date: 2026-03-30
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация тестовой фикстуры модуля работы с универсальными идентификаторами ресурсов —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "uri.hpp"

/**
 * @brief Метод инициализации тестовой среды
 *
 */
void UriFixture::SetUp(){
	// Создаём объект фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
	// Создаём объект логгера
	this->_log = std::make_unique <awh::log_t> (this->_fmk.get());
	// Создаём объект работы с URI
	this->_uri = std::make_unique <awh::uri_t> (this->_fmk.get(), this->_log.get());
}

/**
 * @brief Метод очистки тестовой среды
 *
 */
void UriFixture::TearDown() {}
