/**
 * @file signals.cpp
 * @date 2026-01-26
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
 * @brief Реализация тестовой фикстуры модуля обработки сигналов —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл
 */
#include "signals.hpp"

/**
 * @brief Метод настройки тестового окружения
 *
 */
void SignalsFixture::SetUp(){
	// Создаём объект для работы с сигналами
	this->_signals = std::make_unique <awh::signals_t> (nullptr, nullptr);
}

/**
 * @brief Метод очистки тестового окружения
 *
 */
void SignalsFixture::TearDown() {
	// Если объект сигналов существует
	if(this->_signals != nullptr)
		// Останавливаем обработку сигналов
		this->_signals->stop();
}
