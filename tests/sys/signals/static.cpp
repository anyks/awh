/**
 * @file: static.cpp
 * @date: 2026-01-26
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Подключаем заголовочный файл
 */
#include "signals.hpp"

/**
 * @brief Тест создания объекта сигналов
 *
 */
TEST_F(SignalsFixture, CreateSignalsTest){
	// Проверяем создание объекта
	ASSERT_TRUE(this->_signals != nullptr);
	
	// Сбрасываем объект
	this->_signals.reset();
	
	// Проверяем сброс объекта
	ASSERT_TRUE(this->_signals == nullptr);
}

/**
 * @brief Тест установки функции обратного вызова
 *
 */
TEST_F(SignalsFixture, SetCallbackTest){
	// Проверяем создание объекта
	ASSERT_TRUE(this->_signals != nullptr);

	// Устанавливаем пустой callback
	this->_signals->on([](const int32_t){});
	
	// Если бы был метод getCallback, мы бы проверили его, но здесь просто проверяем что метод вызывается без ошибок
	SUCCEED();
}

/**
 * @brief Тест запуска и остановки отслеживания
 *
 */
TEST_F(SignalsFixture, StartStopTest){
	// Проверяем создание объекта
	ASSERT_TRUE(this->_signals != nullptr);

	// Запускаем отслеживание
	this->_signals->start();
	
	// Останавливаем отслеживание
	this->_signals->stop();
	
	// Повторный запуск
	this->_signals->start();
	
	// Завершаем отслеживание, тест удачен
	SUCCEED();
}
