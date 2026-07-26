/**
 * @file: eth.hpp
 * @date: 2025-12-14
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл тестовой фикстуры модуля работы с сетевым уровнем Ethernet —
 *        объявление класса фикстуры Google Test, подготавливающего и освобождающего тестовое окружение набора тестов
 *
 * @copyright: Copyright © 2025
 *
 */
 
#ifndef __AWH_ETH_TESTS__
#define __AWH_ETH_TESTS__

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/sys/log.hpp"
#include "../../../include/net/addr.hpp"
#include "../../../include/net/eth/eth.hpp"

/**
 * @brief Класс фикстуры для тестов работы с Ethernet
 *
 */
class EthFixture : public testing::Test {
	protected:
		// Объекты фреймворка
		std::unique_ptr <awh::fmk_t> _fmk;
		// Объект логов
		std::unique_ptr <awh::log_t> _log;
		// Объект работы с Ethernet
		std::unique_ptr <awh::eth_t> _eth;
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

#endif // __AWH_ETH_TESTS__
