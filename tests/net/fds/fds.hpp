/**
 * @file: fds.hpp
 * @date: 2025-12-14
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл тестовой фикстуры модуля партнёрских сокетов — объявление класса фикстуры Google Test,
 *        подготавливающего и освобождающего тестовое окружение набора тестов
 *
 * @copyright: Copyright © 2025
 *
 */
 
#ifndef __AWH_FDS_TESTS__
#define __AWH_FDS_TESTS__

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/sys/log.hpp"
#include "../../../include/net/fds.hpp"

/**
 * @brief Класс фикстуры для тестов работы с файловыми дескрипторами
 *
 */
class FdsFixture : public testing::Test {
	protected:
		// Объекты фреймворка
		std::unique_ptr <awh::fmk_t> _fmk;
		// Объект логов
		std::unique_ptr <awh::log_t> _log;
		// Объект работы с файловыми дескрипторами
		std::unique_ptr <awh::fds_t> _fds;
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

#endif // __AWH_FDS_TESTS__
