/**
 * @file: provider.hpp
 * @date: 2026-07-12
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

#ifndef __AWH_HTTP_PROVIDER_TESTS__
#define __AWH_HTTP_PROVIDER_TESTS__

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../../main.hpp"
#include "../../../../include/proto/http/provider.hpp"

/**
 * @brief Класс фикстуры для тестов подмодуля провайдера HTTP-запроса/ответа
 *
 */
class ProviderFixture : public testing::Test {
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

#endif // __AWH_HTTP_PROVIDER_TESTS__
