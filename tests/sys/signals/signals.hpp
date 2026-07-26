/**
 * @file: signals.hpp
 * @date: 2026-01-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл тестовой фикстуры модуля обработки сигналов — объявление класса фикстуры Google Test,
 *        подготавливающего и освобождающего тестовое окружение набора тестов
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_SIGNALS_TESTS__
#define __AWH_SIGNALS_TESTS__

#include "../../main.hpp"
#include "../../../include/sys/signals.hpp"

/**
 * @brief Тестовый класс для работы с сигналами
 *
 */
class SignalsFixture : public testing::Test {
	protected:
		// Объект для работы с сигналами
		std::unique_ptr <awh::signals_t> _signals;
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

#endif // __AWH_SIGNALS_TESTS__
