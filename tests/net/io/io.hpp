/**
 * @file: io.hpp
 * @date: 2025-12-15
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
 
#ifndef __AWH_IO_TESTS__
#define __AWH_IO_TESTS__

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/net/io.hpp"
#include "../../../include/net/addr.hpp"

/**
 * @brief Класс фикстуры для тестов асинхронного движка ввода-вывода
 *
 */
class IoFixture : public testing::Test {
	protected:
		// Объект асинхронного движка ввода-вывода
		std::unique_ptr <awh::io_t> _io;
		// Объекты фреймворка
		std::unique_ptr <awh::fmk_t> _fmk;
		// Объект логов
		std::unique_ptr <awh::log_t> _log;
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

#endif // __AWH_IO_TESTS__
