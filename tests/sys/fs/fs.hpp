/**
 * @file: fs.hpp
 * @date: 2026-01-25
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл тестовой фикстуры модуля работы с файловой системой —
 *        объявление класса фикстуры Google Test, подготавливающего и освобождающего тестовое окружение набора тестов
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_FS_TESTS__
#define __AWH_FS_TESTS__

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/sys/fs.hpp"

/**
 * @brief Класс фикстуры для тестов модуля работы с файловой системой
 *
 */
class FSFixture : public testing::Test {
	protected:
		// Объект работы с файловой системой
		std::unique_ptr <awh::fs_t> _fs;
		// Объекты фреймворка
		std::unique_ptr <awh::fmk_t> _fmk;
		// Объект логов
		std::unique_ptr <awh::log_t> _log;
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

#endif // __AWH_FS_TESTS__
