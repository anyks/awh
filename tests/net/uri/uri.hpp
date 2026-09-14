/**
 * @file uri.hpp
 * @date 2026-03-30
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * @brief Заголовочный файл тестовой фикстуры модуля работы с универсальными идентификаторами ресурсов —
 *        объявление класса фикстуры Google Test, подготавливающего и освобождающего тестовое окружение набора тестов
 *
 * @copyright Copyright © 2026
 *
 */
 
#pragma once

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/net/uri.hpp"

/**
 * @brief Класс фикстуры для тестов работы с URI
 *
 */
class UriFixture : public testing::Test {
	protected:
		// Объект работы с URI
		std::unique_ptr <awh::uri_t> _uri;
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
