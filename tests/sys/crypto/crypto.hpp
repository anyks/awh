/**
 * @file: crypto.hpp
 * @date: 2026-01-21
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл тестовой фикстуры модуля криптографии — объявление класса фикстуры Google Test,
 *        подготавливающего и освобождающего тестовое окружение набора тестов
 *
 * @copyright: Copyright © 2026
 *
 */
 
#ifndef __AWH_CRYPTO_TESTS__
#define __AWH_CRYPTO_TESTS__

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/sys/crypto.hpp"

/**
 * @brief Класс фикстуры для тестов криптографии
 *
 */
class CryptoFixture : public testing::Test {
	protected:
		// Объекты фреймворка
		std::unique_ptr <awh::fmk_t> _fmk;
		// Объект логов
		std::unique_ptr <awh::log_t> _log;
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

#endif // __AWH_CRYPTO_TESTS__
