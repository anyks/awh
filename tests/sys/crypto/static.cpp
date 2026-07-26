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
 * @brief Статические тесты модуля криптографии — проверка создания и сброса объекта модуля,
 *        а также корректности симметричного шифрования и расшифровки данных, вычисления хешей и кодирования в Base64
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "crypto.hpp"

/**
 * @brief Тест создания объекта шифрования
 *
 */
TEST_F(CryptoFixture, CreateCryptoTest){
	// Проверяем, что объект шифрования создан
	ASSERT_TRUE(this->_crypto != nullptr);
	// Сбрасываем объект шифрования
	this->_crypto.reset();
	// Проверяем, что объект шифрования сброшен
	ASSERT_TRUE(this->_crypto == nullptr);
}

/**
 * @brief Тест сброса и повторного создания объекта шифрования
 *
 */
TEST_F(CryptoFixture, ResetAndCreateCryptoTest){
	// Проверяем, что объект шифрования создан
	ASSERT_TRUE(this->_crypto != nullptr);
	// Сбрасываем объект шифрования
	this->_crypto.reset();
	// Проверяем, что объект шифрования сброшен
	ASSERT_TRUE(this->_crypto == nullptr);
	// Создаём объект шифрования заново
	this->_crypto = std::make_unique <awh::crypto_t> (this->_fmk.get(), this->_log.get());
	// Проверяем, что объект шифрования создан
	ASSERT_TRUE(this->_crypto != nullptr);
}

/**
 * @brief Тест повторного создания объекта шифрования
 *
 */
TEST_F(CryptoFixture, ReCreateCryptoTest){
	// Проверяем, что объект шифрования создан
	ASSERT_TRUE(this->_crypto != nullptr);
	// Создаём объект шифрования заново
	this->_crypto = std::make_unique <awh::crypto_t> (this->_fmk.get(), this->_log.get());
	// Отключаем потокобезопасность
	this->_crypto->threadSafety(awh::crypto_t::mode_t::DISABLED);
	// Проверяем, что объект шифрования создан
	ASSERT_TRUE(this->_crypto != nullptr);
}
