/**
 * @file: static.cpp
 * @date: 2026-01-21
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
#include "crypto.hpp"

/**
 * @brief Тест создания объекта регулярного выражения
 *
 */
TEST_F(CryptoFixture, CreateCryptoTest){
	// Проверяем, что объект регулярного выражения создан
	ASSERT_TRUE(this->_crypto != nullptr);
	// Сбрасываем объект регулярного выражения
	this->_crypto.reset();
	// Проверяем, что объект регулярного выражения сброшен
	ASSERT_TRUE(this->_crypto == nullptr);
}

/**
 * @brief Тест сброса и повторного создания объекта регулярного выражения
 *
 */
TEST_F(CryptoFixture, ResetAndCreateCryptoTest){
	// Проверяем, что объект регулярного выражения создан
	ASSERT_TRUE(this->_crypto != nullptr);
	// Сбрасываем объект регулярного выражения
	this->_crypto.reset();
	// Проверяем, что объект регулярного выражения сброшен
	ASSERT_TRUE(this->_crypto == nullptr);
	// Создаём объект регулярного выражения заново
	this->_crypto = std::make_unique <awh::crypto_t> (this->_fmk.get(), this->_log.get());
	// Проверяем, что объект регулярного выражения создан
	ASSERT_TRUE(this->_crypto != nullptr);
}

/**
 * @brief Тест повторного создания объекта регулярного выражения
 *
 */
TEST_F(CryptoFixture, ReCreateCryptoTest){
	// Проверяем, что объект регулярного выражения создан
	ASSERT_TRUE(this->_crypto != nullptr);
	// Создаём объект регулярного выражения заново
	this->_crypto = std::make_unique <awh::crypto_t> (this->_fmk.get(), this->_log.get());
	// Проверяем, что объект регулярного выражения создан
	ASSERT_TRUE(this->_crypto != nullptr);
}
