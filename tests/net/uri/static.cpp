/**
 * @file: static.cpp
 * @date: 2026-03-30
 * @license: GPL-3.0
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
#include "uri.hpp"

/**
 * @brief Тест создания объекта работы с URI
 *
 */
TEST_F(UriFixture, CreateUriTest){
	// Проверяем, что объект работы с URI создан
	ASSERT_TRUE(this->_uri != nullptr);
	// Сбрасываем объект работы с URI
	this->_uri.reset();
	// Проверяем, что объект работы с URI сброшен
	ASSERT_TRUE(this->_uri == nullptr);
}

/**
 * @brief Тест сброса и повторного создания объекта работы с URI
 *
 */
TEST_F(UriFixture, ResetAndCreateUriTest){
	// Проверяем, что объект работы с URI создан
	ASSERT_TRUE(this->_uri != nullptr);
	// Сбрасываем объект работы с URI
	this->_uri.reset();
	// Проверяем, что объект работы с URI сброшен
	ASSERT_TRUE(this->_uri == nullptr);
	// Создаём объект работы с URI заново
	this->_uri = std::make_unique <awh::uri_t> (this->_fmk.get(), this->_log.get());
	// Проверяем, что объект работы с URI создан
	ASSERT_TRUE(this->_uri != nullptr);
}

/**
 * @brief Тест повторного создания объекта работы с URI
 *
 */
TEST_F(UriFixture, ReCreateUriTest){
	// Проверяем, что объект работы с URI создан
	ASSERT_TRUE(this->_uri != nullptr);
	// Создаём объект работы с URI заново
	this->_uri = std::make_unique <awh::uri_t> (this->_fmk.get(), this->_log.get());
	// Проверяем, что объект работы с URI создан
	ASSERT_TRUE(this->_uri != nullptr);
}
