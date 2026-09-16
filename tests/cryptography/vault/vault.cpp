/**
 * @file vault.cpp
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
 * @brief Реализация тестовой фикстуры склада тайн
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <memory>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "vault.hpp"

/**
 * @brief Метод инициализации тестовой среды
 *
 */
void VaultFixture::SetUp(){
	// Создаём объект склада тайн
	this->_vault = std::make_unique <awh::vault_t> ();
}

/**
 * @brief Метод очистки тестовой среды
 *
 */
void VaultFixture::TearDown() {}
