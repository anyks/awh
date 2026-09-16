/**
 * @file crypto.hpp
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
 * @brief Заголовочный файл тестовой фикстуры модуля криптографии — объявление класса фикстуры Google Test,
 *        подготавливающего и освобождающего тестовое окружение набора тестов
 *
 * @copyright Copyright © 2026
 *
 */
 
#pragma once

/**
 * Стандартные заголовочные файлы
 */
#include <memory>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/cryptography/crypto.hpp"

/**
 * @brief Класс фикстуры для тестов криптографии
 *
 */
class CryptoFixture : public testing::Test {
	protected:
		// Объект криптографии
		std::unique_ptr <awh::crypto_t> _crypto;
	public:
		/**
		 * @brief Метод инициализации тестовой среды
		 *
		 */
		void SetUp();
		/**
		 * @brief Метод очистки тестовой среды
		 *
		 */
		void TearDown();
};
