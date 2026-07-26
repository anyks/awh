/**
 * @file: static.cpp
 * @date: 2026-01-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Статические тесты модуля работы с версиями — проверка создания и сброса объекта модуля,
 *        а также корректности разбора строкового представления версии, её сравнения и обратного преобразования
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл
 */
#include "version.hpp"

/**
 * @brief Тест создания объекта версии
 *
 */
TEST_F(VersionFixture, CreateVersionTest){
	// Проверяем создание объекта версии
	ASSERT_TRUE(this->_version != nullptr);

	// Сбрасываем объект версии
	this->_version.reset();

	// Проверяем сброс объекта версии
	ASSERT_TRUE(this->_version == nullptr);
}

/**
 * @brief Тест сброса и повторного создания объекта версии
 *
 */
TEST_F(VersionFixture, ResetAndCreateVersionTest){
	// Проверяем создание объекта версии
	ASSERT_TRUE(this->_version != nullptr);

	// Сбрасываем объект версии
	this->_version.reset();
	// Проверяем сброс объекта версии
	ASSERT_TRUE(this->_version == nullptr);

	// Повторно создаём объект версии
	this->_version = std::make_unique <awh::version_t> ();
	// Проверяем создание объекта версии
	ASSERT_TRUE(this->_version != nullptr);
}

/**
 * @brief Тест повторного создания объекта версии
 *
 */
TEST_F(VersionFixture, ReCreateVersionTest){
	// Проверяем создание объекта версии
	ASSERT_TRUE(this->_version != nullptr);

	// Повторно создаём объект версии
	this->_version = std::make_unique <awh::version_t> ();
	// Проверяем создание объекта версии
	ASSERT_TRUE(this->_version != nullptr);
}

/**
 * @brief Тест сравнения версий
 *
 */
TEST_F(VersionFixture, VersionTest){
	// Создаём объекты версий
	awh::version_t version1("39.28.44.52"); // 875306023
	awh::version_t version2(1160518695);    // 39.28.44.69

	// Проверяем сравнение версий
	ASSERT_LT(version1, version2);
	ASSERT_GT(version2, version1);
	ASSERT_LE(version1, version2);
	ASSERT_GE(version2, version1);
	ASSERT_NE(version2, version1);

	// Устанавливаем одинаковые версии
	version2 = 875306023;

	// Проверяем сравнение версий
	ASSERT_LE(version1, version2);
	ASSERT_EQ(version1, version2);

	// Устанавливаем одинаковые версии
	version1 = "39.28.44.69";
	version2 = "39.28.44.69";

	// Проверяем сравнение версий
	ASSERT_GE(version1, version2);
	ASSERT_EQ(version1, version2);
}
