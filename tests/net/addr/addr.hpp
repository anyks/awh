/**
 * @file: addr.hpp
 * @date: 2025-12-13
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
 
#ifndef __AWH_NET_ADDR_TESTS__
#define __AWH_NET_ADDR_TESTS__

#include "../../main.hpp"
#include "../../../include/net/addr.hpp"

/**
 * @brief Класс фикстуры для тестов сетевых адресов
 *
 */
class NetFixture : public testing::Test {
	protected:
		// Объекты фреймворка
		std::unique_ptr <awh::fmk_t> _fmk;
		// Объект для работы с логами
		std::unique_ptr <awh::log_t> _log;
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

#endif // __AWH_NET_ADDR_TESTS__
