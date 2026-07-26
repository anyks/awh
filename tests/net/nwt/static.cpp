/**
 * @file: static.cpp
 * @date: 2025-12-14
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
#include "nwt.hpp"

/**
 * @brief Тест создания объекта работы со списком параметров URL
 *
 */
TEST_F(NwtFixture, CreateNwtTest){
	// Проверяем, что объект работы со списком параметров URL создан
	ASSERT_TRUE(this->_nwt != nullptr);
	// Сбрасываем объект работы со списком параметров URL
	this->_nwt.reset();
	// Проверяем, что объект работы со списком параметров URL сброшен
	ASSERT_TRUE(this->_nwt == nullptr);
}

/**
 * @brief Тест сброса и повторного создания объекта работы со списком параметров URL
 *
 */
TEST_F(NwtFixture, ResetAndCreateNwtTest){
	// Проверяем, что объект работы со списком параметров URL создан
	ASSERT_TRUE(this->_nwt != nullptr);
	// Сбрасываем объект работы со списком параметров URL
	this->_nwt.reset();
	// Проверяем, что объект работы со списком параметров URL сброшен
	ASSERT_TRUE(this->_nwt == nullptr);
	// Создаём объект работы со списком параметров URL заново
	this->_nwt = std::make_unique <awh::nwt_t> (this->_log.get());
	// Проверяем, что объект работы со списком параметров URL создан
	ASSERT_TRUE(this->_nwt != nullptr);
}

/**
 * @brief Тест повторного создания объекта работы со списком параметров URL
 *
 */
TEST_F(NwtFixture, ReCreateNwtTest){
	// Проверяем, что объект работы со списком параметров URL создан
	ASSERT_TRUE(this->_nwt != nullptr);
	// Создаём объект работы со списком параметров URL заново
	this->_nwt = std::make_unique <awh::nwt_t> (this->_log.get());
	// Проверяем, что объект работы со списком параметров URL создан
	ASSERT_TRUE(this->_nwt != nullptr);
}
