/**
 * @file: parameterized.cpp
 * @date: 2026-01-26
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

/**
 * Подключаем заголовочный файл
 */
#include "version.hpp"

/**
 * @brief Структура параметров теста
 *
 */
struct VersionTestParameter {
	uint8_t octets = 0;
	uint32_t version1 = 0;
	std::string version2 = "";
	uint32_t result1 = 0;
	std::string result2 = "";
};

/**
 * @brief Параметризованный тестовый класс для работы с версионированием
 *
 */
class VersionTestParameterizedFixture : public VersionFixture, public ::testing::WithParamInterface <VersionTestParameter> {
	public:
		VersionTestParameter _parameter = GetParam();
};

/**
 * @brief Тест параметризованного создания и работы с версией
 *
 */
TEST_P(VersionTestParameterizedFixture, VersionTest){
	// Проверяем создание объекта версии
	this->_version->set(this->_parameter.version1);

	// Проверяем работу методов объекта версии
	ASSERT_EQ(this->_parameter.result2, this->_version->str(this->_parameter.octets));
	
	// Проверяем работу операторов объекта версии
	ASSERT_EQ(this->_parameter.version1, static_cast <uint32_t> (* this->_version.get()));

	// Проверяем установку версии из строки
	this->_version->set(this->_parameter.version2);
	// Проверяем работу методов объекта версии
	ASSERT_EQ(this->_parameter.result1, this->_version->num());

	// Проверяем работу операторов объекта версии
	ASSERT_EQ(this->_parameter.version2.substr(0, 8), static_cast <std::string> (* this->_version.get()));
}

/**
 * @brief Инициализация параметров теста
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, VersionTestParameterizedFixture,
	::testing::Values(
		VersionTestParameter({
			4,
			875306023,
			"39.28.44.69",
			1160518695,
			"39.28.44.52"
		}),
		VersionTestParameter({
			3,
			1448999,
			"39.28.44",
			2890791,
			"39.28.22"
		})
	)
);
