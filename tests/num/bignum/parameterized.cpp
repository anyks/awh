/**
 * @file: parameterized.cpp
 * @date: 2026-07-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Параметризованные тесты модуля работы с длинными числами — прогон подготовленных наборов входных данных
 *        через методы модуля с проверкой строкового представления, округления значения и ограничения точности вывода
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "bignum.hpp"

/**
 * @brief Структура параметров теста строкового представления длинного числа
 *
 */
struct BigNumStringTestParameter {
	// Формат представления числа в виде строки
	awh::bignum::format_t format = awh::bignum::format_t::DEC;
	// Десятичная запись числа
	std::string decimal = "";
	// Запись числа в проверяемом формате
	std::string encoded = "";
	// Запись числа в проверяемом формате с префиксом для автоопределения
	std::string prefixed = "";
};

/**
 * @brief Параметризованный тестовый класс строкового представления длинного числа
 *
 */
class BigNumStringParameterizedFixture : public BigNumFixture, public ::testing::WithParamInterface <BigNumStringTestParameter> {
	public:
		// Параметры теста строкового представления длинного числа
		BigNumStringTestParameter _parameter = GetParam();
};

/**
 * @brief Тест строкового представления длинного числа с параметрами
 *
 */
TEST_P(BigNumStringParameterizedFixture, StringBigNumTest){
	// Создаём число из десятичной записи
	awh::uint128_t number(this->_parameter.decimal);

	// Проверяем вывод числа в проверяемом формате
	ASSERT_EQ(number.print(this->_parameter.format), this->_parameter.encoded);
	// Проверяем вывод числа в десятичном формате
	ASSERT_EQ(number.print(awh::bignum::format_t::DEC), this->_parameter.decimal);

	// Создаём число для разбора записи в проверяемом формате
	awh::uint128_t parsed;

	// Разбираем запись числа с явным указанием формата
	ASSERT_TRUE(parsed.parse(this->_parameter.encoded, this->_parameter.format));
	// Проверяем результат разбора с явным указанием формата
	ASSERT_EQ(parsed.print(), this->_parameter.decimal);

	// Разбираем запись числа с автоопределением формата по префиксу
	ASSERT_TRUE(parsed.parse(this->_parameter.prefixed));
	// Проверяем результат разбора с автоопределением формата
	ASSERT_EQ(parsed.print(), this->_parameter.decimal);

	// Проверяем что круговой обход через строку сохраняет значение
	ASSERT_TRUE(awh::uint128_t(number.print()) == number);
	// Проверяем что неявное приведение к строке совпадает с десятичным выводом
	ASSERT_EQ(static_cast <std::string> (number), this->_parameter.decimal);
}

/**
 * @brief Инициализация параметров теста строкового представления длинного числа
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, BigNumStringParameterizedFixture,
	::testing::Values(
		BigNumStringTestParameter({awh::bignum::format_t::HEX,   "0",                                       "0",                                "0x0"}),
		BigNumStringTestParameter({awh::bignum::format_t::HEX,   "255",                                     "ff",                               "0xff"}),
		BigNumStringTestParameter({awh::bignum::format_t::HEXUP, "3735928559",                              "DEADBEEF",                         "0xDEADBEEF"}),
		BigNumStringTestParameter({awh::bignum::format_t::BIN,   "10",                                      "1010",                             "0b1010"}),
		BigNumStringTestParameter({awh::bignum::format_t::OCT,   "15",                                      "17",                               "0o17"}),
		BigNumStringTestParameter({awh::bignum::format_t::HEX,   "18446744073709551616",                    "10000000000000000",                "0x10000000000000000"}),
		BigNumStringTestParameter({awh::bignum::format_t::HEX,   "340282366920938463463374607431768211455", "ffffffffffffffffffffffffffffffff", "0xffffffffffffffffffffffffffffffff"}),
		BigNumStringTestParameter({awh::bignum::format_t::DEC,   "12345678901234567890",                    "12345678901234567890",             "12345678901234567890"})
	)
);

/**
 * @brief Структура параметров теста округления значения длинного числа
 *
 */
struct BigNumRoundTestParameter {
	// Исходная запись вещественного числа
	std::string value = "";
	// Количество сохраняемых знаков дробной части
	int32_t digits = 0;
	// Правило округления значения
	awh::bignum::round_t mode = awh::bignum::round_t::NEAREST;
	// Ожидаемая запись округлённого значения
	std::string result = "";
};

/**
 * @brief Параметризованный тестовый класс округления значения длинного числа
 *
 */
class BigNumRoundParameterizedFixture : public BigNumFixture, public ::testing::WithParamInterface <BigNumRoundTestParameter> {
	public:
		// Параметры теста округления значения длинного числа
		BigNumRoundTestParameter _parameter = GetParam();
};

/**
 * @brief Тест округления значения длинного числа с параметрами
 *
 */
TEST_P(BigNumRoundParameterizedFixture, RoundBigNumTest){
	// Создаём вещественное число из исходной записи
	awh::real64_t number(this->_parameter.value);
	// Выполняем округление значения по заданному правилу
	awh::real64_t rounded = number.round(this->_parameter.digits, this->_parameter.mode);

	// Проверяем результат округления значения
	ASSERT_EQ(rounded.print(), this->_parameter.result);
	// Проверяем что исходное число не изменилось
	ASSERT_TRUE(number == awh::real64_t(this->_parameter.value));

	/**
	 * Если округление выполнялось по правилу отбрасывания младших разрядов
	 */
	if(this->_parameter.mode == awh::bignum::round_t::ZERO)
		// Проверяем что отбрасывание младших разрядов совпадает с усечением
		ASSERT_EQ(number.trunc(this->_parameter.digits).print(), this->_parameter.result);

	/**
	 * Если округление выполнялось к ближайшему значению
	 *
	 * Идемпотентность гарантирована только для правил округления к ближайшему значению,
	 * поскольку округлённая величина является ближайшим представимым к требуемой записи и
	 * повторное округление к ближайшему возвращает ту же запись. Направленные правила
	 * идемпотентностью не обладают, что проверяется отдельным тестом.
	 *
	 */
	if((this->_parameter.mode == awh::bignum::round_t::NEAREST) || (this->_parameter.mode == awh::bignum::round_t::EVEN))
		// Проверяем что повторное округление к ближайшему не меняет результат
		ASSERT_TRUE(rounded.round(this->_parameter.digits, this->_parameter.mode) == rounded);
}

/**
 * @brief Инициализация параметров теста округления значения длинного числа
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, BigNumRoundParameterizedFixture,
	::testing::Values(
		BigNumRoundTestParameter({"1234.56789",  2, awh::bignum::round_t::NEAREST, "1234.57"}),
		BigNumRoundTestParameter({"1234.56789",  0, awh::bignum::round_t::NEAREST, "1235"}),
		BigNumRoundTestParameter({"1234.56789", -2, awh::bignum::round_t::NEAREST, "1200"}),
		BigNumRoundTestParameter({"1234.56789",  2, awh::bignum::round_t::ZERO,    "1234.56"}),
		BigNumRoundTestParameter({"1234.56789",  2, awh::bignum::round_t::DOWN,    "1234.56"}),
		BigNumRoundTestParameter({"1234.56789",  2, awh::bignum::round_t::UP,      "1234.57"}),
		BigNumRoundTestParameter({"-1234.56789", 2, awh::bignum::round_t::NEAREST, "-1234.57"}),
		BigNumRoundTestParameter({"-1234.56789", 2, awh::bignum::round_t::ZERO,    "-1234.56"}),
		BigNumRoundTestParameter({"-1234.56789", 2, awh::bignum::round_t::DOWN,    "-1234.57"}),
		BigNumRoundTestParameter({"-1234.56789", 2, awh::bignum::round_t::UP,      "-1234.56"}),
		BigNumRoundTestParameter({"2.5",         0, awh::bignum::round_t::NEAREST, "3"}),
		BigNumRoundTestParameter({"2.5",         0, awh::bignum::round_t::EVEN,    "2"}),
		BigNumRoundTestParameter({"3.5",         0, awh::bignum::round_t::EVEN,    "4"}),
		BigNumRoundTestParameter({"-2.5",        0, awh::bignum::round_t::NEAREST, "-3"}),
		BigNumRoundTestParameter({"-2.5",        0, awh::bignum::round_t::EVEN,    "-2"}),
		BigNumRoundTestParameter({"9.99",        1, awh::bignum::round_t::NEAREST, "10"}),
		BigNumRoundTestParameter({"0.999",       2, awh::bignum::round_t::NEAREST, "1"}),
		BigNumRoundTestParameter({"0.006",       1, awh::bignum::round_t::NEAREST, "0"}),
		BigNumRoundTestParameter({"0.006",       1, awh::bignum::round_t::UP,      "0.1"}),
		BigNumRoundTestParameter({"0.0001",      2, awh::bignum::round_t::NEAREST, "0"})
	)
);

/**
 * @brief Структура параметров теста ограничения точности вывода длинного числа
 *
 */
struct BigNumPrecisionTestParameter {
	// Исходная запись вещественного числа
	std::string value = "";
	// Количество выводимых знаков дробной части
	int16_t precision = 0;
	// Ожидаемая запись числа в десятичном формате
	std::string decimal = "";
	// Ожидаемая запись числа в научной нотации
	std::string scientific = "";
};

/**
 * @brief Параметризованный тестовый класс ограничения точности вывода длинного числа
 *
 */
class BigNumPrecisionParameterizedFixture : public BigNumFixture, public ::testing::WithParamInterface <BigNumPrecisionTestParameter> {
	public:
		// Параметры теста ограничения точности вывода длинного числа
		BigNumPrecisionTestParameter _parameter = GetParam();
};

/**
 * @brief Тест ограничения точности вывода длинного числа с параметрами
 *
 */
TEST_P(BigNumPrecisionParameterizedFixture, PrecisionBigNumTest){
	// Создаём вещественное число из исходной записи
	awh::real64_t number(this->_parameter.value);

	// Проверяем вывод числа в десятичном формате с заданной точностью
	ASSERT_EQ(number.print(awh::bignum::format_t::DEC, this->_parameter.precision), this->_parameter.decimal);
	// Проверяем вывод числа в научной нотации с заданной точностью
	ASSERT_EQ(number.print(awh::bignum::format_t::SCI, this->_parameter.precision), this->_parameter.scientific);
	// Проверяем что округление значения согласовано с округлением при выводе
	ASSERT_EQ(number.round(this->_parameter.precision).print(awh::bignum::format_t::DEC, this->_parameter.precision), this->_parameter.decimal);
	// Проверяем что вывод без ограничения точности не теряет значащих разрядов
	ASSERT_TRUE(awh::real64_t(number.print()) == number);
}

/**
 * @brief Инициализация параметров теста ограничения точности вывода длинного числа
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, BigNumPrecisionParameterizedFixture,
	::testing::Values(
		BigNumPrecisionTestParameter({"3.7",        0, "4",          "4e+0"}),
		BigNumPrecisionTestParameter({"3.7",        1, "3.7",        "3.7e+0"}),
		BigNumPrecisionTestParameter({"3.7",        4, "3.7000",     "3.7000e+0"}),
		BigNumPrecisionTestParameter({"1234.56789", 2, "1234.57",    "1.23e+3"}),
		BigNumPrecisionTestParameter({"1234.56789", 0, "1235",       "1e+3"}),
		BigNumPrecisionTestParameter({"-2.5",       0, "-3",         "-3e+0"}),
		BigNumPrecisionTestParameter({"0.5",        3, "0.500",      "5.000e-1"}),
		BigNumPrecisionTestParameter({"100",        2, "100.00",     "1.00e+2"})
	)
);
