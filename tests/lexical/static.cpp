/**
 * @file: static.cpp
 * @date: 2026-07-22
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Статические тесты модуля лексического разбора чисел — проверка создания и сброса объекта модуля,
 *        а также корректности разбора целых чисел и чисел с плавающей точкой,
 *        обработки форматов записи и кодов ошибок
 *
 * @copyright: Copyright © 2026
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
 * @brief Тест побитовых операторов format_t
 *
 */
TEST_F(LexicalFixture, FormatOperatorsTest){
	// Устанавливаем флаги FIXED и SCIENTIFIC
	const format_t combined = (format_t::FIXED | format_t::SCIENTIFIC);
	// Проверяем, что комбинированный флаг равен GENERAL
	ASSERT_EQ(combined, format_t::GENERAL);
	// Проверяем, что флаг FIXED установлен
	ASSERT_NE((combined & format_t::FIXED), format_t{});
	// Проверяем, что флаг HEX не установлен
	ASSERT_EQ((combined & format_t::HEX), format_t{});
	// Устанавливаем флаг FIXED
	format_t flags = format_t::FIXED;
	// Устанавливаем флаг SCIENTIFIC
	flags |= format_t::SCIENTIFIC;
	// Проверяем, что флаг GENERAL установлен
	ASSERT_EQ(flags, format_t::GENERAL);
	// Снимаем флаг FIXED, оставляем только SCIENTIFIC
	flags &= format_t::FIXED;
	// Проверяем, что флаг SCIENTIFIC сброшен
	ASSERT_EQ(flags, format_t::FIXED);
	// Снимаем флаг FIXED через XOR, оставляем пустой набор флагов
	flags ^= format_t::FIXED;
	// Проверяем, что флаг сброшен
	ASSERT_EQ(flags, format_t{});
	// Проверяем, что ~format_t::FIXED не равен format_t::FIXED
	ASSERT_NE(~format_t::FIXED, format_t::FIXED);
}

/**
 * @brief Тест binary_t для double/float
 *
 */
TEST_F(LexicalFixture, BinaryFormatTraitsTest){
	// Проверяем число явных бит мантиссы double
	ASSERT_EQ(binary_t <double>::mantissaExplicitBits(), 52);
	// Проверяем число явных бит мантиссы float
	ASSERT_EQ(binary_t <float>::mantissaExplicitBits(), 23);
	// Проверяем минимальную экспоненту double
	ASSERT_EQ(binary_t <double>::minimumExponent(), -1023);
	// Проверяем минимальную экспоненту float
	ASSERT_EQ(binary_t <float>::minimumExponent(), -127);
	// Проверяем код бесконечности double
	ASSERT_EQ(binary_t <double>::infinitePower(), 0x7FF);
	// Проверяем код бесконечности float
	ASSERT_EQ(binary_t <float>::infinitePower(), 0xFF);
	// Проверяем наибольшую степень десяти double
	ASSERT_EQ(binary_t <double>::largestPowerOfTen(), 308);
	// Проверяем наибольшую степень десяти float
	ASSERT_EQ(binary_t <float>::largestPowerOfTen(), 38);
	// Проверяем наименьшую степень десяти double
	ASSERT_EQ(binary_t <double>::smallestPowerOfTen(), -342);
	// Проверяем наименьшую степень десяти float
	ASSERT_EQ(binary_t <float>::smallestPowerOfTen(), -64);
	// Проверяем точную степень десяти 10^0 для double
	ASSERT_EQ(binary_t <double>::exactPowerOfTen(0), 1.0);
	// Проверяем точную степень десяти 10^2 для double
	ASSERT_EQ(binary_t <double>::exactPowerOfTen(2), 100.0);
	// Проверяем точную степень десяти 10^3 для double
	ASSERT_EQ(binary_t <double>::exactPowerOfTen(3), 1000.0);
	// Проверяем точную степень десяти 10^3 для float
	ASSERT_EQ(binary_t <float>::exactPowerOfTen(3), 1000.f);
}

/**
 * @brief Тест leadingZeros и multiply128
 *
 */
TEST_F(LexicalFixture, BitArithmeticTest){
	// Проверяем число ведущих нулей для 1
	ASSERT_EQ(leadingZeros(1ULL), 63);
	// Проверяем число ведущих нулей при установленном старшем бите
	ASSERT_EQ(leadingZeros(0x8000000000000000ULL), 0);
	// Проверяем число ведущих нулей для 32-битного значения
	ASSERT_EQ(leadingZeros(0x00000000FFFFFFFFULL), 32);
	// Проверяем, что нулевое значение обрабатывается без неопределённого поведения
	ASSERT_EQ(leadingZeros(0ULL), 64);

	// Умножаем максимальные uint64_t
	const value128_t square = multiply128(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL);
	// Проверяем младшие 64 бита произведения
	ASSERT_EQ(square.low, 1ULL);
	// Проверяем старшие 64 бита произведения
	ASSERT_EQ(square.high, 0xFFFFFFFFFFFFFFFEULL);

	// Умножаем малые значения 2 * 3
	const value128_t simple = multiply128(2ULL, 3ULL);
	// Проверяем младшие 64 бита
	ASSERT_EQ(simple.low, 6ULL);
	// Проверяем отсутствие переноса в старшую часть
	ASSERT_EQ(simple.high, 0ULL);
}

/**
 * @brief Тест charToDigit / maxDigitsU64 / isSpace
 *
 */
TEST_F(LexicalFixture, LutHelpersTest){
	// Проверяем преобразование цифры '0'
	ASSERT_EQ(charToDigit('0'), 0);
	// Проверяем преобразование цифры '9'
	ASSERT_EQ(charToDigit('9'), 9);
	// Проверяем преобразование буквы 'a' в основание 36
	ASSERT_EQ(charToDigit('a'), 10);
	// Проверяем преобразование буквы 'F' в основание 16
	ASSERT_EQ(charToDigit('F'), 15);
	// Проверяем преобразование буквы 'z' в основание 36
	ASSERT_EQ(charToDigit('z'), 35);
	// Проверяем, что недопустимый символ даёт 255
	ASSERT_EQ(charToDigit('@'), 255);
	// Проверяем максимум цифр uint64_t в основании 10
	ASSERT_EQ(maxDigitsU64(10), 20u);
	// Проверяем максимум цифр uint64_t в основании 16
	ASSERT_EQ(maxDigitsU64(16), 16u);
	// Проверяем максимум цифр uint64_t в основании 2
	ASSERT_EQ(maxDigitsU64(2), 64u);
	// Проверяем максимум цифр uint64_t в основании 36
	ASSERT_EQ(maxDigitsU64(36), 13u);
	// Проверяем, что пробел считается пробельным
	ASSERT_TRUE(isSpace(' '));
	// Проверяем, что табуляция считается пробельной
	ASSERT_TRUE(isSpace('\t'));
	// Проверяем, что перевод строки считается пробельным
	ASSERT_TRUE(isSpace('\n'));
	// Проверяем, что цифра не считается пробельной
	ASSERT_FALSE(isSpace('0'));
	// Проверяем, что буква не считается пробельной
	ASSERT_FALSE(isSpace('A'));
}

/**
 * @brief Тест toExtended / toExtendedHalfway и round-trip через computeFloat/toFloat
 *
 */
TEST_F(LexicalFixture, ExtendedMantissaTest){
	// Строим расширенное представление 3.5
	const mantissa_t am = toExtended(3.5);
	// Проверяем, что мантисса ненулевая
	ASSERT_GT(am.mantissa, 0u);
	// Проверяем, что экспонента ненулевая
	ASSERT_NE(am.power2, 0);

	// Строим середину между 1.0 и следующим float
	const mantissa_t halfway = toExtendedHalfway(1.0);
	// Строим расширенное представление 1.0
	const mantissa_t oneExt = toExtended(1.0);
	// Проверяем сдвиг экспоненты середины на -1
	ASSERT_EQ(halfway.power2, oneExt.power2 - 1);
	// Проверяем формулу мантиссы середины
	ASSERT_EQ(halfway.mantissa, (oneExt.mantissa << 1) + 1);

	// Вычисляем mantissa_t для целого 7 через computeFloat
	const mantissa_t computed = computeFloat <binary_t <double>> (0, 7);
	// Задаём переменную для сборки double
	double rebuilt = 0;
	// Собираем положительное значение через toFloat
	toFloat(false, computed, rebuilt);
	// Проверяем побитовое равенство с 7.0
	ASSERT_TRUE(sameBits(rebuilt, 7.0));
	// Собираем отрицательное значение через toFloat
	toFloat(true, computed, rebuilt);
	// Проверяем побитовое равенство с -7.0
	ASSERT_TRUE(sameBits(rebuilt, -7.0));
}

/**
 * @brief Тест computeFloat на простых и граничных значениях
 *
 */
TEST_F(LexicalFixture, ComputeFloatTest){
	// Вычисляем представление нуля
	const mantissa_t zero = computeFloat <binary_t <double>> (0, 0);
	// Проверяем нулевую мантиссу
	ASSERT_EQ(zero.mantissa, 0u);
	// Проверяем нулевую экспоненту
	ASSERT_EQ(zero.power2, 0);

	// Вычисляем представление единицы
	const mantissa_t one = computeFloat <binary_t <double>> (0, 1);
	// Значение для double
	double value = 0;
	// Собираем double из mantissa_t
	toFloat(false, one, value);
	// Проверяем побитовое равенство с 1.0
	ASSERT_TRUE(sameBits(value, 1.0));

	// Вычисляем значение с экспонентой выше максимума
	const mantissa_t huge = computeFloat <binary_t <double>> (400, 1);
	// Проверяем, что получена бесконечность
	ASSERT_EQ(huge.power2, binary_t <double>::infinitePower());
	// Проверяем нулевую мантиссу у infinity
	ASSERT_EQ(huge.mantissa, 0u);

	// Вычисляем значение с экспонентой ниже минимума
	const mantissa_t tiny = computeFloat <binary_t <double>> (-400, 1);
	// Проверяем антипереполнение в ноль
	ASSERT_EQ(tiny.mantissa, 0u);
	// Проверяем, что экспонента не стала бесконечностью
	ASSERT_EQ(tiny.power2, 0);
}

/**
 * @brief Тест bigint_t: базовые операции и сравнение
 *
 */
TEST_F(LexicalFixture, BigintBasicsTest){
	// Создаём bigint_t со значением 10
	bigint_t a(10);
	// Создаём bigint_t со значением 3
	bigint_t b(3);
	// Добавляем 2 к a
	ASSERT_TRUE(a.add(2));
	// Проверяем, что a стало равно 12
	ASSERT_EQ(a.compare(bigint_t(12)), 0);
	// Умножаем a на 5
	ASSERT_TRUE(a.mul(5));
	// Проверяем, что a стало равно 60
	ASSERT_EQ(a.compare(bigint_t(60)), 0);
	// Умножаем a на 10^2
	ASSERT_TRUE(a.pow10(2));
	// Проверяем, что a стало равно 6000
	ASSERT_EQ(a.compare(bigint_t(6000)), 0);
	// Проверяем, что b < a
	ASSERT_LT(b.compare(a), 0);
	// Проверяем, что a > b
	ASSERT_GT(a.compare(b), 0);

	// Берём старшие 64 бита нормализованного значения
	bool truncated = false;
	// Проверяем, что hi64 возвращает корректное значение
	const uint64_t hi = bigint_t(0x1000000000000000ULL).hi64(truncated);
	// Проверяем отсутствие усечения
	ASSERT_FALSE(truncated);
	// Проверяем нормализацию к старшему установленному биту
	ASSERT_EQ(hi, 0x8000000000000000ULL);
}

/**
 * @brief Тест parseBlock и isDigit
 *
 */
TEST_F(LexicalFixture, AsciiDigitHelpersTest){
	// Проверяем, что '0' — десятичная цифра
	ASSERT_TRUE(isDigit('0'));
	// Проверяем, что '9' — десятичная цифра
	ASSERT_TRUE(isDigit('9'));
	// Проверяем, что '/' не цифра
	ASSERT_FALSE(isDigit('/'));
	// Проверяем, что ':' не цифра
	ASSERT_FALSE(isDigit(':'));

	// Разбираем восемь цифр подряд
	const char digits[] = "12345678";
	// Проверяем результат разбора восьми цифр
	ASSERT_EQ(parseBlock(digits), 12345678u);
	// Проверяем быструю проверку «все 8 байт — цифры»
	ASSERT_TRUE(isDigitBlock(readBlock(digits)));
	// Проверяем отказ при наличии буквы среди цифр
	ASSERT_FALSE(isDigitBlock(readBlock("12a45678")));
}

/**
 * @brief Тест успешного разбора double: нули, знаки, экспонента
 *
 */
TEST_F(LexicalFixture, FromCharsDoubleBasicsTest){
	// Сверяем разбор нуля со strtod
	expectDoubleMatchesStrtod("0");
	// Сверяем разбор 0.0 со strtod
	expectDoubleMatchesStrtod("0.0");
	// Сверяем разбор отрицательного нуля со strtod
	expectDoubleMatchesStrtod("-0");
	// Сверяем разбор единицы со strtod
	expectDoubleMatchesStrtod("1");
	// Сверяем разбор -1 со strtod
	expectDoubleMatchesStrtod("-1");
	// Сверяем разбор числа π со strtod
	expectDoubleMatchesStrtod("3.141592653589793");
	// Сверяем разбор научной записи 1e10 со strtod
	expectDoubleMatchesStrtod("1e10");
	// Сверяем разбор научной записи 1E-10 со strtod
	expectDoubleMatchesStrtod("1E-10");
	// Сверяем разбор записи с явным плюсом в экспоненте
	expectDoubleMatchesStrtod("2.5e+1");
	// Сверяем разбор числа без целой части
	expectDoubleMatchesStrtod(".5");
	// Сверяем разбор числа без дробной части после точки
	expectDoubleMatchesStrtod("5.");
	// Сверяем разбор длинного целого
	expectDoubleMatchesStrtod("12345678901234567890");
}

/**
 * @brief Тест успешного разбора float
 *
 */
TEST_F(LexicalFixture, FromCharsFloatBasicsTest){
	// Сверяем разбор нуля со strtof
	expectFloatMatchesStrtof("0");
	// Сверяем разбор отрицательного нуля со strtof
	expectFloatMatchesStrtof("-0.0");
	// Сверяем разбор числа π для float со strtof
	expectFloatMatchesStrtof("3.1415927");
	// Сверяем разбор большой экспоненты со strtof
	expectFloatMatchesStrtof("1e20");
	// Сверяем разбор малой экспоненты со strtof
	expectFloatMatchesStrtof("1.5E-5");
	// Сверяем разбор FLT_MAX в десятичной записи со strtof
	expectFloatMatchesStrtof("340282346638528859811704183484516925440");
}

/**
 * @brief Тест inf/nan и запрета NO_INFNAN
 *
 */
TEST_F(LexicalFixture, InfNanTest){
	// Задаём значение для разбора
	double d = 0;
	// Разбираем "inf" в double
	auto r = lexical_t::fromChars(&"inf"[0], &"inf"[3], d);
	// Проверяем успешность разбора
	ASSERT_TRUE(r);
	// Проверяем, что результат — бесконечность
	ASSERT_TRUE(std::isinf(d));
	// Проверяем положительный знак infinity
	ASSERT_GT(d, 0);

	// Разбираем "-Infinity"
	r = lexical_t::fromChars(&"-Infinity"[0], &"-Infinity"[9], d);
	// Проверяем успешность разбора
	ASSERT_TRUE(r);
	// Проверяем, что результат — бесконечность
	ASSERT_TRUE(std::isinf(d));
	// Проверяем отрицательный знак infinity
	ASSERT_LT(d, 0);

	// Разбираем "nan"
	r = lexical_t::fromChars(&"nan"[0], &"nan"[3], d);
	// Проверяем успешность разбора
	ASSERT_TRUE(r);
	// Проверяем, что результат — NaN
	ASSERT_TRUE(std::isnan(d));

	// Разбираем "nan(ind)" с опциональной последовательностью
	r = lexical_t::fromChars(&"nan(ind)"[0], &"nan(ind)"[8], d);
	// Проверяем успешность разбора
	ASSERT_TRUE(r);
	// Проверяем, что результат — NaN
	ASSERT_TRUE(std::isnan(d));
	// Проверяем, что указатель указывает за закрывающую скобку
	ASSERT_EQ(r.ptr, &"nan(ind)"[8]);

	// Разбираем "inf" в JSON-формате (NO_INFNAN)
	r = lexical_t::fromChars(&"inf"[0], &"inf"[3], d, format_t::JSON);
	// Проверяем отказ разбора
	ASSERT_FALSE(r);
	// Проверяем код ошибки invalid_argument
	ASSERT_EQ(r.ec, std::errc::invalid_argument);

	// Значение для float
	float f = 0;
	// Задаём строку L"INF"
	const wchar_t infW[] = L"INF";
	// Разбираем "INF" в float через wchar_t
	auto rf = lexical_t::fromChars(infW, infW + 3, f);
	// Проверяем успешность разбора
	ASSERT_TRUE(rf);
	// Проверяем, что результат — бесконечность
	ASSERT_TRUE(std::isinf(f));
}

/**
 * @brief Тест переполнения и антипереполнения
 *
 */
TEST_F(LexicalFixture, OverflowUnderflowTest){
	// Задаём значение для переполнения
	double d = 1;
	// Разбираем число с огромной положительной экспонентой
	auto r = lexical_t::fromChars(&"1e9999"[0], &"1e9999"[6], d);
	// Проверяем, что разбор завершился с ошибкой диапазона
	ASSERT_FALSE(r);
	// Проверяем код ошибки result_out_of_range
	ASSERT_EQ(r.ec, std::errc::result_out_of_range);
	// Проверяем, что значение стало +infinity
	ASSERT_TRUE(std::isinf(d));
	// Проверяем положительный знак infinity
	ASSERT_GT(d, 0);

	// Задаём значение для антипереполнения
	d = 1;
	// Разбираем число с огромной отрицательной экспонентой
	r = lexical_t::fromChars(&"1e-9999"[0], &"1e-9999"[7], d);
	// Проверяем, что разбор завершился с ошибкой диапазона
	ASSERT_FALSE(r);
	// Проверяем код ошибки result_out_of_range
	ASSERT_EQ(r.ec, std::errc::result_out_of_range);
	// Проверяем антипереполнение в ноль
	ASSERT_EQ(d, 0.0);
}

/**
 * @brief Тест частичного разбора и позиции ptr
 *
 */
TEST_F(LexicalFixture, PartialParsePtrTest){
	// Разбираем число с хвостом нечисловых символов
	const char text[] = "12.34xyz";
	// Значение для разбора
	double d = 0;
	// Разбираем строку "12.34xyz" в double
	const auto r = lexical_t::fromChars(text, text + sizeof(text) - 1, d);
	// Проверяем успешность разбора
	ASSERT_TRUE(r);
	// Проверяем значение разобранного числа
	ASSERT_TRUE(sameBits(d, 12.34));
	// Проверяем, что ptr остановился сразу за числом
	ASSERT_EQ(r.ptr, text + 5);

	// Разбираем целое без хвоста
	const char only[] = "42";
	// Значение для разбора
	int64_t i = 0;
	// Разбираем строку "42" в int64_t
	const auto ri = lexical_t::fromChars(only, only + 2, i);
	// Проверяем успешность разбора
	ASSERT_TRUE(ri);
	// Проверяем значение
	ASSERT_EQ(i, 42);
	// Проверяем, что ptr указывает на конец строки
	ASSERT_EQ(ri.ptr, only + 2);
}

/**
 * @brief Тест ошибок разбора: пустая строка и мусор
 *
 */
TEST_F(LexicalFixture, InvalidInputTest){
	// Значение для разбора
	double d = 123;
	// Пытаемся разобрать пустую строку
	auto r = lexical_t::fromChars(static_cast <const char *> (""), static_cast <const char *> (""), d);
	// Проверяем отказ разбора
	ASSERT_FALSE(r);
	// Проверяем код ошибки invalid_argument
	ASSERT_EQ(r.ec, std::errc::invalid_argument);
	// Проверяем, что исходное значение не изменилось
	ASSERT_EQ(d, 123);

	// Пытаемся разобрать нечисловую строку
	r = lexical_t::fromChars(&"abc"[0], &"abc"[3], d);
	// Проверяем отказ разбора
	ASSERT_FALSE(r);
	// Проверяем код ошибки invalid_argument
	ASSERT_EQ(r.ec, std::errc::invalid_argument);

	// Пытаемся разобрать число с ведущим '+' без ALLOW_LEADING_PLUS
	r = lexical_t::fromChars(&"+1"[0], &"+1"[2], d);
	// Проверяем отказ разбора
	ASSERT_FALSE(r);
	// Проверяем код ошибки invalid_argument
	ASSERT_EQ(r.ec, std::errc::invalid_argument);
}

/**
 * @brief Тест ALLOW_LEADING_PLUS и SKIP_WHITE_SPACE
 *
 */
TEST_F(LexicalFixture, OptionsLeadingPlusAndWhitespaceTest){
	// Разбираем число с ведущим '+' при разрешённом плюсе
	double d = 0;
	// Создаём опции с разрешением ведущего плюса
	options_t <char> plusOpts(format_t::GENERAL | format_t::ALLOW_LEADING_PLUS);
	// Разбираем строку "+12.5" с разрешённым ведущим плюсом
	auto r = lexical_t::fromCharsAdvanced(&"+12.5"[0], &"+12.5"[5], d, plusOpts);
	// Проверяем успешность разбора
	ASSERT_TRUE(r);
	// Проверяем значение
	ASSERT_TRUE(sameBits(d, 12.5));

	// Создаём опции с разрешением пропуска пробельных символов
	options_t <char> spaceOpts(format_t::GENERAL | format_t::SKIP_WHITE_SPACE);
	// Разбираем строку с пробелами и табуляцией перед числом
	r = lexical_t::fromCharsAdvanced(&"  \t3.5"[0], &"  \t3.5"[6], d, spaceOpts);
	// Проверяем успешность разбора
	ASSERT_TRUE(r);
	// Проверяем значение
	ASSERT_TRUE(sameBits(d, 3.5));
}

/**
 * @brief Тест кастомной десятичной точки
 *
 */
TEST_F(LexicalFixture, CustomDecimalPointTest){
	// Разбираем число с запятой как десятичной точкой
	double d = 0;
	// Создаём опции с запятой в качестве десятичного разделителя
	options_t <char> opts(format_t::GENERAL, ',');
	// Разбираем строку "3,14" с запятой
	const auto r = lexical_t::fromCharsAdvanced(&"3,14"[0], &"3,14"[4], d, opts);
	// Проверяем успешность разбора
	ASSERT_TRUE(r);
	// Проверяем значение
	ASSERT_TRUE(sameBits(d, 3.14));
}

/**
 * @brief Тест строгого JSON-формата
 *
 */
TEST_F(LexicalFixture, JsonFormatRulesTest){
	// Значение для разбора
	double d = 0;
	// Проверяем запрет ведущих нулей в JSON
	auto r = lexical_t::fromChars(&"01"[0], &"01"[2], d, format_t::JSON);
	// Проверяем отказ разбора
	ASSERT_FALSE(r);
	// Проверяем допустимость одиночного нуля
	r = lexical_t::fromChars(&"0"[0], &"0"[1], d, format_t::JSON);
	// Проверяем успешность разбора
	ASSERT_TRUE(r);
	// Проверяем значение нуля
	ASSERT_TRUE(sameBits(d, 0.0));
	// Проверяем запрет точки без дробных цифр
	r = lexical_t::fromChars(&"1."[0], &"1."[2], d, format_t::JSON);
	// Проверяем отказ разбора
	ASSERT_FALSE(r);
	// Проверяем разбор нормального JSON-числа
	r = lexical_t::fromChars(&"1.25e2"[0], &"1.25e2"[6], d, format_t::JSON);
	// Проверяем успешность разбора
	ASSERT_TRUE(r);
	// Проверяем значение 125.0
	ASSERT_TRUE(sameBits(d, 125.0));
	// Проверяем запрет Infinity в JSON
	r = lexical_t::fromChars(&"Infinity"[0], &"Infinity"[8], d, format_t::JSON);
	// Проверяем отказ разбора
	ASSERT_FALSE(r);
	// Проверяем допуск NaN в JSON_OR_INFNAN
	r = lexical_t::fromChars(&"NaN"[0], &"NaN"[3], d, format_t::JSON_OR_INFNAN);
	// Проверяем успешность разбора
	ASSERT_TRUE(r);
	// Проверяем, что результат — NaN
	ASSERT_TRUE(std::isnan(d));
}

/**
 * @brief Тест FIXED-only и SCIENTIFIC-only
 *
 */
TEST_F(LexicalFixture, FixedAndScientificOnlyTest){
	// Разбираем фиксированную запись в режиме FIXED
	double d = 0;
	// Задаём строку с фиксированным числом
	const char fixedNum[] = "1.5";
	// Разбираем строку "1.5" в режиме FIXED
	auto r = lexical_t::fromChars(fixedNum, fixedNum + 3, d, format_t::FIXED);
	// Проверяем успешность разбора
	ASSERT_TRUE(r);
	// Проверяем значение
	ASSERT_TRUE(sameBits(d, 1.5));

	// Без SCIENTIFIC экспонента не потребляется — разбирается только «1»
	const char mixed[] = "1e2";
	// Разбираем строку "1e2" в режиме FIXED
	r = lexical_t::fromChars(mixed, mixed + 3, d, format_t::FIXED);
	// Проверяем успешность частичного разбора
	ASSERT_TRUE(r);
	// Проверяем значение 1.0
	ASSERT_TRUE(sameBits(d, 1.0));
	// Проверяем остановку указателя перед 'e'
	ASSERT_EQ(r.ptr, mixed + 1);

	// Разбираем ту же строку в режиме SCIENTIFIC
	r = lexical_t::fromChars(mixed, mixed + 3, d, format_t::SCIENTIFIC);
	// Проверяем успешность полного разбора
	ASSERT_TRUE(r);
	// Проверяем значение 100.0
	ASSERT_TRUE(sameBits(d, 100.0));
	// Проверяем, что указатель дошёл до конца
	ASSERT_EQ(r.ptr, mixed + 3);
}

/**
 * @brief Тест длинной мантиссы (путь digitComp / tooManyDigits)
 *
 */
TEST_F(LexicalFixture, LongMantissaDigitCompTest){
	// Сверяем разбор очень длинной мантиссы со strtod
	expectDoubleMatchesStrtod("1.2345678901234567890123456789012345678901234567890");
	// Сверяем разбор наименьшего нормального double со strtod
	expectDoubleMatchesStrtod("2.2250738585072013e-308");
	// Сверяем разбор DBL_MAX со strtod
	expectDoubleMatchesStrtod("1.7976931348623157e+308");
	// Сверяем разбор субнормали со strtod
	expectDoubleMatchesStrtod("5.0e-324");
	// Сверяем разбор числа за пределами точности 53 бит со strtod
	expectDoubleMatchesStrtod("9007199254740993");
	// Сверяем разбор классического 0.1 со strtod
	expectDoubleMatchesStrtod("0.1");
	// Сверяем разбор классического 0.3 со strtod
	expectDoubleMatchesStrtod("0.3");
}

/**
 * @brief Тест разбора целых в разных основаниях
 *
 */
TEST_F(LexicalFixture, IntegerBasesTest){
	// Разбираем шестнадцатеричное ff
	int64_t i = 0;
	// Разбираем строку "ff" в шестнадцатеричной системе
	auto r = lexical_t::fromChars(&"ff"[0], &"ff"[2], i, 16);
	// Проверяем успешность разбора
	ASSERT_TRUE(r);
	// Проверяем значение 255
	ASSERT_EQ(i, 255);

	// Разбираем двоичное 1010
	r = lexical_t::fromChars(&"1010"[0], &"1010"[4], i, 2);
	// Проверяем успешность разбора
	ASSERT_TRUE(r);
	// Проверяем значение 10
	ASSERT_EQ(i, 10);

	// Разбираем отрицательное десятичное
	r = lexical_t::fromChars(&"-42"[0], &"-42"[3], i, 10);
	// Проверяем успешность разбора
	ASSERT_TRUE(r);
	// Проверяем значение -42
	ASSERT_EQ(i, -42);

	// Разбираем цифру основания 36
	r = lexical_t::fromChars(&"z"[0], &"z"[1], i, 36);
	// Проверяем успешность разбора
	ASSERT_TRUE(r);
	// Проверяем значение 35
	ASSERT_EQ(i, 35);

	// Разбираем UINT64_MAX
	uint64_t u = 0;
	// Разбираем строку "18446744073709551615" в десятичной системе
	r = lexical_t::fromChars(&"18446744073709551615"[0], &"18446744073709551615"[20], u, 10);
	// Проверяем успешность разбора
	ASSERT_TRUE(r);
	// Проверяем значение UINT64_MAX
	ASSERT_EQ(u, UINT64_MAX);

	// Пытаемся разобрать недопустимую цифру для основания 16
	r = lexical_t::fromChars(&"g"[0], &"g"[1], i, 16);
	// Проверяем отказ разбора
	ASSERT_FALSE(r);
	// Проверяем код ошибки invalid_argument
	ASSERT_EQ(r.ec, std::errc::invalid_argument);

	// Пытаемся разобрать с недопустимо малым основанием
	r = lexical_t::fromChars(&"10"[0], &"10"[2], i, 1);
	// Проверяем отказ разбора
	ASSERT_FALSE(r);
	// Проверяем код ошибки invalid_argument
	ASSERT_EQ(r.ec, std::errc::invalid_argument);

	// Пытаемся разобрать с недопустимо большим основанием
	r = lexical_t::fromChars(&"10"[0], &"10"[2], i, 37);
	// Проверяем отказ разбора
	ASSERT_FALSE(r);
	// Проверяем код ошибки invalid_argument
	ASSERT_EQ(r.ec, std::errc::invalid_argument);
}

/**
 * @brief Тест точности определения переполнения целых чисел
 *
 * @details Проверка количества цифр не в состоянии обнаружить переполнение,
 *          при котором накопленное по модулю значение остаётся выше нижней
 *          границы диапазона, поэтому разбор обязан выполнять точный контроль.
 *
 */
TEST_F(LexicalFixture, IntegerOverflowDetectionTest){
	/**
	 * @brief Структура параметра проверки переполнения
	 *
	 */
	struct sample_t {
		const char * text; // Разбираемая строка
		int32_t base;      // Основание системы счисления
	};
	// Значения, переполняющие разрядность uint64_t
	const sample_t samples[] = {
		{"18446744073709551616", 10},
		{"20000000000000000000", 10},
		{"30000000000000000000", 10},
		{"50000000000000000000", 10},
		{"99999999999999999999", 10},
		{"0000030000000000000000000", 10},
		{"3777777777777777777777", 8},
		{"zzzzzzzzzzzzz", 36}
	};
	/**
	 * Выполняем перебор всех переполняющих значений
	 */
	for(const auto & sample : samples){
		// Результат разбора числовой строки
		uint64_t value = 0;
		// Выполняем разбор переполняющего значения
		const auto result = lexical_t::fromChars(sample.text, sample.text + std::strlen(sample.text), value, sample.base);
		// Проверяем обнаружение выхода значения за пределы диапазона
		ASSERT_EQ(result.ec, std::errc::result_out_of_range) << sample.text;
	}
	// Результат разбора наибольшего представимого значения
	uint64_t value = 0;
	// Выполняем разбор наибольшего представимого значения
	const auto result = lexical_t::fromChars(&"18446744073709551615"[0], &"18446744073709551615"[20], value);
	// Проверяем успешность разбора наибольшего представимого значения
	ASSERT_TRUE(result);
	// Проверяем корректность разобранного значения
	ASSERT_EQ(value, 18446744073709551615ULL);
}

/**
 * @brief Тест integerTimesPow10
 *
 */
TEST_F(LexicalFixture, IntegerTimesPow10Test){
	// Проверяем умножение на 10^0
	ASSERT_TRUE(sameBits(integerTimesPow10(42ull, 0), 42.0));
	// Проверяем умножение на 10^3
	ASSERT_TRUE(sameBits(integerTimesPow10(42ull, 3), 42000.0));
	// Проверяем умножение на 10^-2
	ASSERT_TRUE(sameBits(integerTimesPow10(42ull, -2), 0.42));
	// Проверяем умножение отрицательного целого на 10^2
	ASSERT_TRUE(sameBits(integerTimesPow10(-7ll, 2), -700.0));
	// Проверяем переполнение в infinity
	ASSERT_TRUE(std::isinf(integerTimesPow10(1ull, 400)));
	// Проверяем антипереполнение в ноль
	ASSERT_EQ(integerTimesPow10(1ull, -400), 0.0);
	// Проверяем шаблонный вариант для float
	ASSERT_TRUE(sameBits(integerTimesPow10 <float> (5u, 2), 500.f));
}

/**
 * @brief Тест wchar_t / char16_t / char32_t
 *
 */
TEST_F(LexicalFixture, WideCharTypesTest){
	// Разбираем число из wchar_t
	double d = 0;
	// Значение 2.5e1 в wchar_t
	const wchar_t w[] = L"2.5e1";
	// Выполняем разбор в double
	const auto rw = lexical_t::fromChars(w, w + 5, d);
	// Проверяем успешность разбора
	ASSERT_TRUE(rw);
	// Проверяем значение 25.0
	ASSERT_TRUE(sameBits(d, 25.0));

	// Разбираем число из char16_t
	const char16_t u16[] = u"0.125";
	// Выполняем разбор в double
	const auto r16 = lexical_t::fromChars(u16, u16 + 5, d);
	// Проверяем успешность разбора
	ASSERT_TRUE(r16);
	// Проверяем значение 0.125
	ASSERT_TRUE(sameBits(d, 0.125));

	// Разбираем целое из char32_t
	const char32_t u32[] = U"-8";
	// Значение для разбора в int32_t
	int32_t i = 0;
	// Выполняем разбор в int32_t
	const auto r32 = lexical_t::fromChars(u32, u32 + 2, i, 10);
	// Проверяем успешность разбора
	ASSERT_TRUE(r32);
	// Проверяем значение -8
	ASSERT_EQ(i, -8);
}

/**
 * @brief Тест parseNumberString: коды ошибок JSON
 *
 */
TEST_F(LexicalFixture, ParseNumberStringErrorsTest){
	options_t <char> opts(format_t::JSON);
	// В JSON '+' не считается допустимым знаком — ошибка «нет цифр»
	auto pns = parseNumberString <true> (&"+"[0], &"+"[1], opts);
	// Проверяем отказ разбора
	ASSERT_FALSE(pns.valid);
	// Проверяем код ошибки NO_DIGITS_IN_INTEGER_PART
	ASSERT_EQ(pns.error, error_t::NO_DIGITS_IN_INTEGER_PART);

	// После '-' обязательна цифра (не точка)
	pns = parseNumberString <true> (&"-."[0], &"-."[2], opts);
	// Проверяем отказ разбора
	ASSERT_FALSE(pns.valid);
	// Проверяем код ошибки MISSING_INTEGER_AFTER_SIGN
	ASSERT_EQ(pns.error, error_t::MISSING_INTEGER_AFTER_SIGN);

	// Проверяем запрет ведущих нулей
	pns = parseNumberString <true> (&"01"[0], &"01"[2], opts);
	// Проверяем отказ разбора
	ASSERT_FALSE(pns.valid);
	// Проверяем код ошибки LEADING_ZEROS_IN_INTEGER_PART
	ASSERT_EQ(pns.error, error_t::LEADING_ZEROS_IN_INTEGER_PART);

	// Проверяем запрет числа без целой части
	pns = parseNumberString <true> (&".1"[0], &".1"[2], opts);
	// Проверяем отказ разбора
	ASSERT_FALSE(pns.valid);
	// Проверяем код ошибки NO_DIGITS_IN_INTEGER_PART
	ASSERT_EQ(pns.error, error_t::NO_DIGITS_IN_INTEGER_PART);

	// Проверяем запрет точки без дробных цифр
	pns = parseNumberString <true> (&"1."[0], &"1."[2], opts);
	// Проверяем отказ разбора
	ASSERT_FALSE(pns.valid);
	// Проверяем код ошибки NO_DIGITS_IN_FRACTIONAL_PART
	ASSERT_EQ(pns.error, error_t::NO_DIGITS_IN_FRACTIONAL_PART);

	// В JSON (FIXED|SCIENTIFIC) незавершённый «e» игнорируется — валидно число «1»
	pns = parseNumberString <true> (&"1e"[0], &"1e"[2], opts);
	// Проверяем успешность разбора
	ASSERT_TRUE(pns.valid);
	// Проверяем мантиссу 1
	ASSERT_EQ(pns.mantissa, 1ull);
	// Проверяем остановку указателя перед 'e'
	ASSERT_EQ(pns.lastmatch, &"1e"[1]);

	// SCIENTIFIC без FIXED требует экспоненту
	options_t <char> sciOnly(format_t::SCIENTIFIC);
	// Проверяем отказ разбора числа без экспоненты
	pns = parseNumberString <false> (&"1"[0], &"1"[1], sciOnly);
	// Проверяем отказ разбора
	ASSERT_FALSE(pns.valid);
	// Проверяем код ошибки MISSING_EXPONENTIAL_PART
	ASSERT_EQ(pns.error, error_t::MISSING_EXPONENTIAL_PART);

	// Проверяем успешный разбор в общем режиме
	pns = parseNumberString <false> (&"1.25e3"[0], &"1.25e3"[6], options_t <char> (format_t::GENERAL));
	// Проверяем успешность разбора
	ASSERT_TRUE(pns.valid);
	// Проверяем нормализованную мантиссу
	ASSERT_EQ(pns.mantissa, 125ull);
	// Проверяем экспонент после нормализации
	ASSERT_EQ(pns.exponent, 1);
	// Проверяем отсутствие знака минус
	ASSERT_FALSE(pns.negative);
}

/**
 * @brief Тест scientificExponent
 *
 */
TEST_F(LexicalFixture, ScientificExponentTest){
	// Проверяем экспонент для мантиссы 1
	ASSERT_EQ(scientificExponent(1, 0), 0);
	// Проверяем экспонент для мантиссы 9999
	ASSERT_EQ(scientificExponent(9999, 0), 3);
	// Проверяем экспонент для мантиссы 10000
	ASSERT_EQ(scientificExponent(10000, 0), 4);
	// Проверяем экспонент с отрицательным исходным смещением
	ASSERT_EQ(scientificExponent(123456789, -2), 6);
}
