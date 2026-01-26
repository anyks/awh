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
#include "procre.hpp"

/**
 * @brief Тест создания объекта резольвера процессов
 *
 */
TEST_F(ProcreFixture, CreateProcreTest){
	// Проверяем создание объекта
	ASSERT_TRUE(this->_procre != nullptr);
	
	// Сбрасываем объект
	this->_procre.reset();
	
	// Проверяем сброс объекта
	ASSERT_TRUE(this->_procre == nullptr);
}

/**
 * @brief Тест получения имени текущего процесса
 *
 */
TEST_F(ProcreFixture, CurrentProcessNameTest){
	// Проверяем создание объекта
	ASSERT_TRUE(this->_procre != nullptr);

	// Получаем имя текущего процесса
	std::string name = this->_procre->name();
	
	// Проверяем что имя не пустое
	ASSERT_FALSE(name.empty());
}

/**
 * @brief Тест получения имени процесса init/launchd
 *
 */
TEST_F(ProcreFixture, InitProcessNameTest){
	// Проверяем создание объекта
	ASSERT_TRUE(this->_procre != nullptr);

	// Получаем имя процесса с PID 1
	std::string name = this->_procre->name(1);
	
	// Если мы работаем от имени суперпользователя
	if(::geteuid() == 0)
		// Проверяем что имя не пустое
		ASSERT_FALSE(name.empty());
	// Проверяем что имя пустое
	else ASSERT_TRUE(name.empty());
}
