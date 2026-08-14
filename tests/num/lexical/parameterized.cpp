/**
 * @file parameterized.cpp
 * @date 2026-07-22
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
 * @brief Параметризованные тесты модуля лексического разбора чисел — прогон подготовленных наборов входных данных
 *        через методы модуля с проверкой разбора целых чисел и чисел с плавающей точкой,
 *        обработки форматов записи и кодов ошибок
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл
 */
#include "lexical.hpp"

/**
 * Используем пространства имён
 */
using namespace awh;
using namespace awh::lexical;

/**
 * @brief Параметр теста разбора double
 *
 */
struct DoubleParseParameter {
	const char * text = nullptr;
};

/**
 * @brief Параметризованный fixture для double
 *
 */
class DoubleParseParameterizedFixture : public LexicalFixture, public ::testing::WithParamInterface <DoubleParseParameter> {};

/**
 * @brief Сверка fromChars(double) со strtod на таблице строк
 *
 */
TEST_P(DoubleParseParameterizedFixture, MatchesStrtodTest){
	// Проверяем совпадение с strtod
	expectDoubleMatchesStrtod(GetParam().text);
}

/**
 * @brief Таблица строк для double (обычные, граничные, «трудные» кейсы)
 *
 */
INSTANTIATE_TEST_SUITE_P(DoubleParseCases, DoubleParseParameterizedFixture,
	::testing::Values(
		DoubleParseParameter{"0"},
		DoubleParseParameter{"-0"},
		DoubleParseParameter{"0.0"},
		DoubleParseParameter{"1"},
		DoubleParseParameter{"-1"},
		DoubleParseParameter{"10"},
		DoubleParseParameter{"0.1"},
		DoubleParseParameter{"0.3"},
		DoubleParseParameter{"0.5"},
		DoubleParseParameter{".25"},
		DoubleParseParameter{"25."},
		DoubleParseParameter{"1.0"},
		DoubleParseParameter{"-1.5"},
		DoubleParseParameter{"3.141592653589793"},
		DoubleParseParameter{"2.718281828459045"},
		DoubleParseParameter{"1e0"},
		DoubleParseParameter{"1e1"},
		DoubleParseParameter{"1e-1"},
		DoubleParseParameter{"1E+1"},
		DoubleParseParameter{"1.23e4"},
		DoubleParseParameter{"1.23E-4"},
		DoubleParseParameter{"9.999999999999998e22"},
		DoubleParseParameter{"1.0000000000000002"},
		DoubleParseParameter{"9007199254740992"},
		DoubleParseParameter{"9007199254740993"},
		DoubleParseParameter{"4503599627370497.5"},
		DoubleParseParameter{"1.7976931348623157e308"},
		DoubleParseParameter{"-1.7976931348623157e308"},
		DoubleParseParameter{"2.2250738585072014e-308"},
		DoubleParseParameter{"2.2250738585072013e-308"},
		DoubleParseParameter{"4.9406564584124654e-324"},
		DoubleParseParameter{"5e-324"},
		DoubleParseParameter{"1.2345678901234567890123456789012345"},
		DoubleParseParameter{"1234567890123456789012345678901234567890"},
		DoubleParseParameter{"0.00000000000000000000000000000000000000000000000001"},
		DoubleParseParameter{"6.02214076e23"},
		DoubleParseParameter{"1.602176634e-19"},
		DoubleParseParameter{"299792458"},
		DoubleParseParameter{"0e0"},
		DoubleParseParameter{"0e999"},
		DoubleParseParameter{"1234567890123456789"},
		DoubleParseParameter{"1.00000000000000011102230246251565404236316680908203125"}
	)
);

/**
 * @brief Параметр теста разбора float
 *
 */
struct FloatParseParameter {
	const char * text = nullptr;
};

/**
 * @brief Параметризованный fixture для float
 *
 */
class LexicalParseParameterizedFixture : public LexicalFixture, public ::testing::WithParamInterface <FloatParseParameter> {};

/**
 * @brief Сверка fromChars(float) со strtof на таблице строк
 *
 */
TEST_P(LexicalParseParameterizedFixture, MatchesStrtofTest){
	// Проверяем совпадение с strtof
	expectFloatMatchesStrtof(GetParam().text);
}

/**
 * @brief Таблица строк для float
 *
 */
INSTANTIATE_TEST_SUITE_P(FloatParseCases, LexicalParseParameterizedFixture,
	::testing::Values(
		FloatParseParameter{"0"},
		FloatParseParameter{"-0"},
		FloatParseParameter{"1"},
		FloatParseParameter{"-1.5"},
		FloatParseParameter{"3.1415927"},
		FloatParseParameter{"1e10"},
		FloatParseParameter{"1e-10"},
		FloatParseParameter{"3.4028235e38"},
		FloatParseParameter{"1.1754944e-38"},
		FloatParseParameter{"1.4012985e-45"},
		FloatParseParameter{"0.1"},
		FloatParseParameter{"0.3"},
		FloatParseParameter{"16777217"},
		FloatParseParameter{"123456789012345678901234567890"}
	)
);

/**
 * @brief Параметр теста разбора целых
 *
 */
struct IntegerParseParameter {
	bool ok = true;
	int32_t base = 10;
	int64_t expected = 0;
	const char * text = nullptr;
};

/**
 * @brief Параметризованный fixture для целых
 *
 */
class IntegerParseParameterizedFixture : public LexicalFixture, public ::testing::WithParamInterface <IntegerParseParameter> {};

/**
 * @brief Разбор целых в разных основаниях
 *
 */
TEST_P(IntegerParseParameterizedFixture, ParseIntegerTest){
	// Разбираем целое
	int64_t value = 0;
	// Получаем параметры
	const auto & p = GetParam();
	// Выполняем разбор
	const auto answer = lexical_t::fromChars(p.text, p.text + std::strlen(p.text), value, p.base);
	// Если ожидаемое значение корректно, проверяем совпадение
	if(p.ok){
		// Проверяем результат
		ASSERT_TRUE(answer) << p.text << " base=" << p.base;
		// Проверяем совпадение значения
		ASSERT_EQ(value, p.expected) << p.text << " base=" << p.base;
		// Проверяем код ошибки
		ASSERT_EQ(answer.ec, std::errc());
	// Если ожидаемое значение некорректно, проверяем ошибку
	} else {
		// Проверяем, что разбор не удался
		ASSERT_FALSE(answer) << p.text << " base=" << p.base;
		// Проверяем код ошибки
		ASSERT_NE(answer.ec, std::errc());
	}
}

/**
 * @brief Таблица целых
 *
 */
INSTANTIATE_TEST_SUITE_P(IntegerParseCases, IntegerParseParameterizedFixture,
	::testing::Values(
		IntegerParseParameter{true, 10, 0, "0"},
		IntegerParseParameter{true, 10, 42, "42"},
		IntegerParseParameter{true, 10, -42, "-42"},
		IntegerParseParameter{true, 16, 0x7fffffff, "7fffffff"},
		IntegerParseParameter{true, 16, 255, "ff"},
		IntegerParseParameter{true, 16, 255, "FF"},
		IntegerParseParameter{true, 2, 42, "101010"},
		IntegerParseParameter{true, 8, 511, "777"},
		IntegerParseParameter{true, 36, 35, "z"},
		IntegerParseParameter{true, 10, 10, "10"},
		IntegerParseParameter{false, 10, 0, "+"},
		IntegerParseParameter{false, 10, 0, ""},
		IntegerParseParameter{true, 10, 12, "12x"},
		IntegerParseParameter{false, 16, 0, "g"},
		IntegerParseParameter{false, 2, 0, "2"}
	)
);

/**
 * @brief Параметр теста integerTimesPow10
 *
 */
struct Pow10Parameter {
	uint64_t mantissa = 0;
	int exponent = 0;
	double expected = 0;
	bool infinite = false;
};

/**
 * @brief Параметризованный fixture для integerTimesPow10
 *
 */
class Pow10ParameterizedFixture : public LexicalFixture, public ::testing::WithParamInterface <Pow10Parameter> {};

/**
 * @brief Проверка integerTimesPow10 на таблице
 *
 */
TEST_P(Pow10ParameterizedFixture, IntegerTimesPow10CasesTest){
	// Получаем параметры
	const auto & p = GetParam();
	// Вычисляем результат
	const double value = integerTimesPow10(p.mantissa, p.exponent);
	// Если ожидаемое значение бесконечно, проверяем, что результат бесконечность
	if(p.infinite){
		// Проверяем, что результат бесконечность
		ASSERT_TRUE(std::isinf(value));
		// Проверяем знак бесконечности
		ASSERT_GT(value, 0);
	// Если ожидаемое значение не бесконечно, проверяем совпадение
	} else ASSERT_TRUE(sameBits(value, p.expected)) << "mant=" << p.mantissa << " exp=" << p.exponent;
}

/**
 * @brief Таблица integerTimesPow10
 *
 */
INSTANTIATE_TEST_SUITE_P(Pow10Cases, Pow10ParameterizedFixture,
	::testing::Values(
		Pow10Parameter{1, 0, 1.0, false},
		Pow10Parameter{1, 1, 10.0, false},
		Pow10Parameter{1, -1, 0.1, false},
		Pow10Parameter{5, 2, 500.0, false},
		Pow10Parameter{123, -2, 1.23, false},
		Pow10Parameter{1, 308, 1e308, false},
		Pow10Parameter{1, 400, 0.0, true},
		Pow10Parameter{0, 10, 0.0, false}
	)
);
