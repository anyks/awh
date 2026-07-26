/**
 * @file: static.cpp
 * @date: 2026-01-21
 * @license: LicenseRef-AWH-1.0
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
 * Подключаем заголовочный файлы проекта
 */
#include "compressor.hpp"

/**
 * @brief Тест создания объекта компрессии
 *
 */
TEST_F(CompressorFixture, CreateCompressorTest){
	// Проверяем, что объект компрессии создан
	ASSERT_TRUE(this->_compressor != nullptr);
	// Сбрасываем объект компрессии
	this->_compressor.reset();
	// Проверяем, что объект компрессии сброшен
	ASSERT_TRUE(this->_compressor == nullptr);
}

/**
 * @brief Тест сброса и повторного создания объекта компрессии
 *
 */
TEST_F(CompressorFixture, ResetAndCreateCompressorTest){
	// Проверяем, что объект компрессии создан
	ASSERT_TRUE(this->_compressor != nullptr);
	// Сбрасываем объект компрессии
	this->_compressor.reset();
	// Проверяем, что объект компрессии сброшен
	ASSERT_TRUE(this->_compressor == nullptr);
	// Создаём объект компрессии заново
	this->_compressor = std::make_unique <awh::compressor::block_t> (this->_log.get());
	// Проверяем, что объект компрессии создан
	ASSERT_TRUE(this->_compressor != nullptr);
}

/**
 * @brief Тест повторного создания объекта компрессии
 *
 */
TEST_F(CompressorFixture, ReCreateCompressorTest){
	// Проверяем, что объект компрессии создан
	ASSERT_TRUE(this->_compressor != nullptr);
	// Создаём объект компрессии заново
	this->_compressor = std::make_unique <awh::compressor::block_t> (this->_log.get());
	// Отключаем потокобезопасность
	this->_compressor->threadSafety(false);
	// Проверяем, что объект компрессии создан
	ASSERT_TRUE(this->_compressor != nullptr);
}
