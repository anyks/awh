/**
 * @file addr.hpp
 * @date 2025-12-13
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
 * @brief Заголовочный файл тестовой фикстуры модуля работы с сетевыми адресами —
 *        объявление класса фикстуры Google Test, подготавливающего и освобождающего тестовое окружение набора тестов
 *
 * @copyright Copyright © 2025
 *
 */
 
#pragma once

#include "../../main.hpp"
#include "../../../include/net/addr.hpp"

/**
 * @brief Класс фикстуры для тестов сетевых адресов
 *
 */
class NetFixture : public testing::Test {
	protected:
		// Объект сетевого адреса
		std::unique_ptr <awh::net_addr_t> _addr;
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
