/**
 * @file: static.cpp
 * @date: 2025-12-14
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "fds.hpp"

/**
 * @brief Тест создания объекта работы с файловыми дескрипторами
 *
 */
TEST_F(FdsFixture, CreateFdsTest){
	// Проверяем, что объект работы с файловыми дескрипторами создан
	ASSERT_TRUE(this->_fds != nullptr);
	// Сбрасываем объект работы с файловыми дескрипторами
	this->_fds.reset();
	// Проверяем, что объект работы с файловыми дескрипторами сброшен
	ASSERT_TRUE(this->_fds == nullptr);
}

/**
 * @brief Тест сброса и повторного создания объекта работы с файловыми дескрипторами
 *
 */
TEST_F(FdsFixture, ResetAndCreateFdsTest){
	// Проверяем, что объект работы с файловыми дескрипторами создан
	ASSERT_TRUE(this->_fds != nullptr);
	// Сбрасываем объект работы с файловыми дескрипторами
	this->_fds.reset();
	// Проверяем, что объект работы с файловыми дескрипторами сброшен
	ASSERT_TRUE(this->_fds == nullptr);
	// Создаём объект работы с файловыми дескрипторами заново
	this->_fds = std::make_unique <awh::fds_t> (this->_log.get());
	// Проверяем, что объект работы с файловыми дескрипторами создан
	ASSERT_TRUE(this->_fds != nullptr);
}

/**
 * @brief Тест повторного создания объекта работы с файловыми дескрипторами
 *
 */
TEST_F(FdsFixture, ReCreateFdsTest){
	// Проверяем, что объект работы с файловыми дескрипторами создан
	ASSERT_TRUE(this->_fds != nullptr);
	// Создаём объект работы с файловыми дескрипторами заново
	this->_fds = std::make_unique <awh::fds_t> (this->_log.get());
	// Проверяем, что объект работы с файловыми дескрипторами создан
	ASSERT_TRUE(this->_fds != nullptr);
}

/**
 * @brief Тест набора сетевых тестов
 *
 */
TEST_F(FdsFixture, FdsSuiteTest){
	// Получаем лимит файловых дескрипторов
	auto limits = this->_fds->limit();
	// Проверяем, что лимит файловых дескрипторов получен
	ASSERT_TRUE(limits.first > 0);
	ASSERT_TRUE(limits.second >= limits.first);
	// Пытаемся установить лимит файловых дескрипторов на текущее значение
	bool result = this->_fds->limit(limits.second);
	// Проверяем, что лимит файловых дескрипторов установлен
	ASSERT_TRUE(result);
	// Записываем в лог справку по файловым дескрипторам
	this->_fds->help(limits.first, limits.second);
}
