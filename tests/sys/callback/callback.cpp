/**
 * @file callback.cpp
 * @date 2026-01-22
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
 * @brief Реализация тестовой фикстуры модуля функций обратного вызова —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "callback.hpp"

/**
 * @brief Метод настройки тестового окружения
 *
 */
void CallbackFixture::SetUp(){
	// Создаём объект модуля обратного вызова
	this->_callback = std::make_unique <awh::callback_t> ();
}

/**
 * @brief Метод очистки тестового окружения
 *
 */
void CallbackFixture::TearDown() {}
