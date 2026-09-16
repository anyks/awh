/**
 * @file fds.cpp
 * @date 2025-12-14
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
 * @brief Реализация тестовой фикстуры модуля партнёрских сокетов —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <memory>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "fds.hpp"

/**
 * @brief Метод инициализации тестовой среды
 *
 */
void FdsFixture::SetUp(){
	// Создаём объект работы с файловыми дескрипторами
	this->_fds = std::make_unique <awh::fds_t> ();
}

/**
 * @brief Метод очистки тестовой среды
 *
 */
void FdsFixture::TearDown() {}
