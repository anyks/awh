/**
 * @file vault.hpp
 * @date 2026-08-22
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
 * @brief Заголовочный файл тестовой фикстуры склада тайн
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_VAULT_TESTS__
#define __AWH_VAULT_TESTS__

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/cryptography/vault.hpp"

/**
 * @brief Класс фикстуры для тестов склада тайн
 *
 */
class VaultFixture : public testing::Test {
	protected:
		// Объекты фреймворка
		std::unique_ptr <awh::fmk_t> _fmk;
		// Объект логов
		std::unique_ptr <awh::log_t> _log;
		// Объект склада тайн
		std::unique_ptr <awh::vault_t> _vault;
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

#endif // __AWH_VAULT_TESTS__
