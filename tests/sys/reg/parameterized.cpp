/**
 * @file: parameterized.cpp
 * @date: 2025-12-12
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
#include "reg.hpp"

/**
 * @brief Параметры теста выполнения регулярного выражения
 *
 */
struct RegTestParameter {
	// Ожидаемый результат
	bool result = false;
	// Текст для проверки
	std::string text = "";
	// Шаблон регулярного выражения
	std::string pattern = "";
	// Опции регулярного выражения
	std::vector <awh::regexp_t::option_t> options;
};

/**
 * @brief Класс параметризованной тестовой фикстуры
 *
 */
class RegTestParameterizedFixture : public RegFixture, public ::testing::WithParamInterface <RegTestParameter> {
	public:
		// Параметры теста
		RegTestParameter _parameter = GetParam();
};

/**
 * @brief Тест параметризованного создания и проверки регулярного выражения
 *
 */
TEST_P(RegTestParameterizedFixture, RegTestingTest){
	// Создаём объект регулярного выражения
	auto exp = this->_reg->build(this->_parameter.pattern, this->_parameter.options);
	// Проверяем результат тестирования регулярного выражения
	ASSERT_EQ(this->_parameter.result, this->_reg->test(this->_parameter.text, exp));
	/**
	 * Шаблон валиден в обоих случаях, поэтому ошибок быть не должно:
	 * отсутствие совпадения ошибкой не является
	 */
	ASSERT_TRUE(this->_reg->error().empty());
}

/**
 * @brief Инициализация параметров теста
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, RegTestParameterizedFixture,
	::testing::Values(
		RegTestParameter({
			true,
			"125AB-32-CD",
			"^\\d+[a-z]+",
			{awh::regexp_t::option_t::CASELESS}
		}),
		RegTestParameter({
			false,
			"ID-AB-32-CD",
			"^\\d+[a-z]+",
			{awh::regexp_t::option_t::CASELESS}
		})
	)
);

/**
 * @brief Параметры теста выполнения регулярного выражения
 *
 */
struct RegExecParameter {
	// Текст для проверки
	std::string text = "";
	// Шаблон регулярного выражения
	std::string pattern = "";
	// Ожидаемый результат
	std::vector <std::string> result;
	// Опции регулярного выражения
	std::vector <awh::regexp_t::option_t> options;
};

/**
 * @brief Класс параметризованной тестовой фикстуры
 *
 */
class RegExecParameterizedFixture : public RegFixture, public ::testing::WithParamInterface <RegExecParameter> {
	public:
		// Параметры теста
		RegExecParameter _parameter = GetParam();
};

/**
 * @brief Тест параметризованного выполнения регулярного выражения
 *
 */
TEST_P(RegExecParameterizedFixture, RegExecTest){
	// Создаём объект регулярного выражения
	auto exp = this->_reg->build(this->_parameter.pattern, this->_parameter.options);
	// Проверяем результат выполнения регулярного выражения
	ASSERT_EQ(this->_parameter.result, this->_reg->exec(this->_parameter.text, exp));
	// Проверяем отсутствие ошибок
	ASSERT_TRUE(this->_reg->error().empty());
}

/**
 * @brief Инициализация параметров теста
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, RegExecParameterizedFixture,
	::testing::Values(
		RegExecParameter({
			"125AB-32-CD",
			"^\\d+([a-z]+)[^a-z]+([a-z]+)$",
			{"125AB-32-CD","AB","CD"},
			{awh::regexp_t::option_t::CASELESS}
		}),
		RegExecParameter({
			"Привет этот дивный мир!!!",
			"^([^\\s]+)\\s([^\\s]+)\\s([^\\s]+)\\s([^\\s]+)!!!$",
			{"Привет этот дивный мир!!!","Привет","этот","дивный","мир"},
			{awh::regexp_t::option_t::UTF8,awh::regexp_t::option_t::CASELESS}
		})
	)
);

/**
 * @brief Параметры теста выполнения регулярного выражения
 *
 */
struct RegMatchParameter {
	// Текст для проверки
	std::string text = "";
	// Шаблон регулярного выражения
	std::string pattern = "";
	// Ожидаемый результат
	std::vector <std::pair <size_t, size_t>> result;
	// Опции регулярного выражения
	std::vector <awh::regexp_t::option_t> options;
};

/**
 * @brief Класс параметризованной тестовой фикстуры
 *
 */
class RegMatchParameterizedFixture : public RegFixture, public ::testing::WithParamInterface <RegMatchParameter> {
	public:
		// Параметры теста
		RegMatchParameter _parameter = GetParam();
};

/**
 * @brief Тест параметризованного выполнения регулярного выражения
 *
 */
TEST_P(RegMatchParameterizedFixture, RegMatchTest){
	// Создаём объект регулярного выражения
	auto exp = this->_reg->build(this->_parameter.pattern, this->_parameter.options);
	// Проверяем результат выполнения регулярного выражения
	ASSERT_EQ(this->_parameter.result, this->_reg->match(this->_parameter.text, exp));
	// Проверяем отсутствие ошибок
	ASSERT_TRUE(this->_reg->error().empty());
}

/**
 * @brief Инициализация параметров теста
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, RegMatchParameterizedFixture,
	::testing::Values(
		RegMatchParameter({
			"125AB-32-CD",
			"^\\d+([a-z]+)[^a-z]+([a-z]+)$",
			{{0,11},{3,2},{9,2}},
			{awh::regexp_t::option_t::CASELESS}
		}),
		RegMatchParameter({
			"Привет этот дивный мир!!!",
			"^([^\\s]+)\\s([^\\s]+)\\s([^\\s]+)\\s([^\\s]+)!!!$",
			{{0,44},{0,12},{13,8},{22,12},{35,6}},
			{awh::regexp_t::option_t::UTF8,awh::regexp_t::option_t::CASELESS}
		})
	)
);
