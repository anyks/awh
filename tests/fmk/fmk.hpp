/**
 * @file: fmk.hpp
 * @date: 2025-12-07
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
 
#ifndef __AWH_FMK_TESTS__
#define __AWH_FMK_TESTS__

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../main.hpp"
#include "../../include/sys/fmk.hpp"

/**
 * @brief Класс фикстуры для тестов фреймворка
 *
 */
class FmkFixture : public testing::Test {
	protected:
		std::unique_ptr <awh::fmk_t> _fmk;
	public:
		void SetUp();
		void TearDown();
};

#endif // __AWH_FMK_TESTS__
