/**
 * @file: chrono.hpp
 * @date: 2025-12-10
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */
 
#ifndef __AWH_CHRONO_TESTS__
#define __AWH_CHRONO_TESTS__

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../main.hpp"
#include "../../include/sys/log.hpp"
#include "../../include/sys/chrono.hpp"

/**
 * @brief Класс фикстуры для тестов модуля работы с датой и временем
 *
 */
class ChronoFixture : public testing::Test {
	protected:
		std::unique_ptr <awh::fmk_t> _fmk;
		std::unique_ptr <awh::log_t> _log;
		std::unique_ptr <awh::chrono_t> _chrono;
	public:
		void SetUp();
		void TearDown();
};

#endif // __AWH_CHRONO_TESTS__
