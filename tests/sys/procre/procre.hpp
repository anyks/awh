/**
 * @file: procre.hpp
 * @date: 2026-01-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

#ifndef __AWH_PROCRE_TESTS__
#define __AWH_PROCRE_TESTS__

#include "../../main.hpp"
#include "../../../include/net/addr.hpp"
#include "../../../include/sys/procre.hpp"

/**
 * @brief Тестовый класс для работы с процессами
 *
 */
class ProcreFixture : public testing::Test {
	protected:
		// Объект фреймворка
		std::unique_ptr <awh::fmk_t> _fmk;
		// Объект логов
		std::unique_ptr <awh::log_t> _log;
		// Объект для работы с сетевыми адресами
		std::unique_ptr <awh::net_addr_t> _addr;
		// Объект для работы с процессами
		std::unique_ptr <awh::procre_t> _procre;
	public:
		/**
		 * @brief Метод настройки тестового окружения
		 *
		 */
		void SetUp();
		/**
		 * @brief Метод очистки тестового окружения
		 *
		 */
		void TearDown();
};

#endif // __AWH_PROCRE_TESTS__
