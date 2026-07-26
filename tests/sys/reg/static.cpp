/**
 * @file: static.cpp
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
#include "reg.hpp"

/**
 * @brief Тест создания объекта регулярного выражения
 *
 */
TEST_F(RegFixture, CreateRegTest){
	// Проверяем, что объект регулярного выражения создан
	ASSERT_TRUE(this->_reg != nullptr);
	// Сбрасываем объект регулярного выражения
	this->_reg.reset();
	// Проверяем, что объект регулярного выражения сброшен
	ASSERT_TRUE(this->_reg == nullptr);
}

/**
 * @brief Тест сброса и повторного создания объекта регулярного выражения
 *
 */
TEST_F(RegFixture, ResetAndCreateRegTest){
	// Проверяем, что объект регулярного выражения создан
	ASSERT_TRUE(this->_reg != nullptr);
	// Сбрасываем объект регулярного выражения
	this->_reg.reset();
	// Проверяем, что объект регулярного выражения сброшен
	ASSERT_TRUE(this->_reg == nullptr);
	// Создаём объект регулярного выражения заново
	this->_reg = std::make_unique <awh::regexp_t> (this->_log.get());
	// Проверяем, что объект регулярного выражения создан
	ASSERT_TRUE(this->_reg != nullptr);
}

/**
 * @brief Тест повторного создания объекта регулярного выражения
 *
 */
TEST_F(RegFixture, ReCreateRegTest){
	// Проверяем, что объект регулярного выражения создан
	ASSERT_TRUE(this->_reg != nullptr);
	// Создаём объект регулярного выражения заново
	this->_reg = std::make_unique <awh::regexp_t> (this->_log.get());
	// Проверяем, что объект регулярного выражения создан
	ASSERT_TRUE(this->_reg != nullptr);
}
