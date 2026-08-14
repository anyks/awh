/**
 * @file compressor.cpp
 * @date 2026-01-21
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
 * @brief Реализация тестовой фикстуры подсистемы компрессии —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "compressor.hpp"

/**
 * @brief Метод инициализации тестовой среды
 *
 */
void CompressorFixture::SetUp(){
	// Создаём объект фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
	// Создаём объект логгера
	this->_log = std::make_unique <awh::log_t> (this->_fmk.get());
	// Создаём объект компрессии
	this->_compressor = std::make_unique <awh::compressor::block_t> (this->_log.get());
}

/**
 * @brief Метод очистки тестовой среды
 *
 */
void CompressorFixture::TearDown() {}
