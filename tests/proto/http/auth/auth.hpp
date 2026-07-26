/**
 * @file: auth.hpp
 * @date: 2026-07-14
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл тестовой фикстуры модуля HTTP-авторизации — объявление класса фикстуры Google Test,
 *        подготавливающего и освобождающего тестовое окружение набора тестов
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_HTTP_AUTH_TESTS__
#define __AWH_HTTP_AUTH_TESTS__

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../../main.hpp"
#include "../../../../include/proto/http/auth/auth.hpp"

/**
 * @brief Класс фикстуры для тестов подмодуля HTTP-авторизации
 *
 */
class AuthFixture : public testing::Test {
	protected:
		// Объект фреймворка
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
	protected:
		/**
		 * @brief Фабричный метод создания модуля авторизации
		 *
		 * @param owner сторона работы (клиент/сервер)
		 * @return      сформированный объект модуля авторизации
		 *
		 */
		std::unique_ptr <awh::http::auth_t> make(const awh::http::auth_t::owner_t owner) const noexcept;
};

#endif // __AWH_HTTP_AUTH_TESTS__
