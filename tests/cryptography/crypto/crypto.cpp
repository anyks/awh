/**
 * @file crypto.cpp
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
 * @brief Реализация тестовой фикстуры модуля криптографии —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "crypto.hpp"

/**
 * @brief Метод инициализации тестовой среды
 *
 */
void CryptoFixture::SetUp(){
	// Создаём объект криптографии
	this->_crypto = std::make_unique <awh::crypto_t> ();
}

/**
 * @brief Метод очистки тестовой среды
 *
 */
void CryptoFixture::TearDown() {}
