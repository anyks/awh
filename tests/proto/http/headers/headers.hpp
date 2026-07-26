/**
 * @file: headers.hpp
 * @date: 2026-07-12
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл тестовой фикстуры контейнера HTTP-заголовков — объявление класса фикстуры Google Test,
 *        подготавливающего и освобождающего тестовое окружение набора тестов
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_HTTP_HEADERS_TESTS__
#define __AWH_HTTP_HEADERS_TESTS__

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../../main.hpp"
#include "../../../../include/proto/http/headers.hpp"

/**
 * @brief Класс фикстуры для тестов подмодуля контейнера HTTP-заголовков
 *
 */
class HeadersFixture : public testing::Test {
	protected:
		// Объект фреймворка
		std::unique_ptr <awh::fmk_t> _fmk;
		// Объект логов
		std::unique_ptr <awh::log_t> _log;
		// Объект контейнера HTTP-заголовков
		std::unique_ptr <awh::http::headers_t> _headers;
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
		 * @brief Фабричный метод создания HTTP-заголовка
		 *
		 * @param name  название HTTP-заголовка
		 * @param value значение HTTP-заголовка
		 * @return      сформированный объект HTTP-заголовка
		 *
		 */
		awh::http::headers_t::header_t header(const std::string & name, const std::string & value) const noexcept;
};

#endif // __AWH_HTTP_HEADERS_TESTS__
