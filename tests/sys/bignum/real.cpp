/**
 * @file: real.cpp
 * @date: 2026-07-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Тесты вещественных длинных чисел — проверка побитового совпадения с аппаратными типами,
 *        арифметики, специальных значений, предельных величин и нестандартных разрядностей формата IEEE-754
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cmath>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "bignum.hpp"

/**
 * @brief Тест побитового совпадения вещественного числа двойной точности с аппаратным типом
 *
 */
TEST_F(BigNumFixture, RealStoreBigNumTest){
	/**
	 * В цикле сверяем битовый образ вещественного числа с аппаратным типом
	 */
	for(const double sample : std::vector <double> ({
		0.0, 1.0, -1.0, 0.5, 2.0, 3.0, 0.1, -0.1, 1e300, 1e-300,
		123456.789, 3.141592653589793, (1.0 / 3.0), 1e16, 2.5, -1024.0
	})){
		// Создаём вещественное длинное число из аппаратного значения
		awh::real64_t number = sample;

		// Проверяем побитовое совпадение с аппаратным значением
		ASSERT_TRUE(this->identical(number, sample)) << "value = " << sample;
		// Проверяем обратное приведение к аппаратному значению
		ASSERT_EQ(static_cast <double> (number), sample);
	}
}

/**
 * @brief Тест арифметики вещественного числа двойной точности против аппаратного типа
 *
 */
TEST_F(BigNumFixture, RealArithmeticBigNumTest){
	// Список первых операндов для проверки арифметики
	const std::vector <double> first = {1.0, 3.0, 0.1, 1e10, -7.5, (1.0 / 3.0)};
	// Список вторых операндов для проверки арифметики
	const std::vector <double> second = {7.0, 0.25, 1e-5, 3.0, 2.0, 6.0};

	/**
	 * В цикле сверяем результаты арифметических операций с аппаратным типом
	 */
	for(size_t i = 0; i < first.size(); i++){
		// Создаём первый операнд в виде длинного числа
		awh::real64_t number1 = first.at(i);
		// Создаём второй операнд в виде длинного числа
		awh::real64_t number2 = second.at(i);

		// Проверяем побитовое совпадение результата сложения
		ASSERT_TRUE(this->identical(number1 + number2, (first.at(i) + second.at(i)))) << "add " << first.at(i) << " " << second.at(i);
		// Проверяем побитовое совпадение результата вычитания
		ASSERT_TRUE(this->identical(number1 - number2, (first.at(i) - second.at(i)))) << "sub " << first.at(i) << " " << second.at(i);
		// Проверяем побитовое совпадение результата умножения
		ASSERT_TRUE(this->identical(number1 * number2, (first.at(i) * second.at(i)))) << "mul " << first.at(i) << " " << second.at(i);
		// Проверяем побитовое совпадение результата деления
		ASSERT_TRUE(this->identical(number1 / number2, (first.at(i) / second.at(i)))) << "div " << first.at(i) << " " << second.at(i);
		// Проверяем побитовое совпадение остатка от деления
		ASSERT_TRUE(this->identical(number1 % number2, ::fmod(first.at(i), second.at(i)))) << "mod " << first.at(i) << " " << second.at(i);
	}
	// Проверяем остаток от деления вещественных чисел
	ASSERT_EQ((awh::real64_t(10.5) % awh::real64_t(3.0)).print(), "1.5");
	// Проверяем сравнение вещественных чисел по возрастанию
	ASSERT_TRUE(awh::real64_t(3.0) < awh::real64_t(10.5));
	// Проверяем сравнение вещественных чисел по убыванию
	ASSERT_TRUE(awh::real64_t(10.5) > awh::real64_t(3.0));
	// Проверяем равенство вещественных чисел
	ASSERT_TRUE(awh::real64_t(1.0) == awh::real64_t(1.0));
}

/**
 * @brief Тест извлечения корня вещественного числа
 *
 */
TEST_F(BigNumFixture, RealSqrtBigNumTest){
	/**
	 * В цикле сверяем извлечение корня с аппаратным типом
	 */
	for(const double sample : std::vector <double> ({0.0, 1.0, 2.0, 4.0, 0.25, 1e300, 1e-300, 3.141592653589793, 1234567.89}))
		// Проверяем побитовое совпадение результата извлечения корня
		ASSERT_TRUE(this->identical(awh::real64_t(sample).sqrt(), ::sqrt(sample))) << "value = " << sample;

	// Создаём вещественное число четверной точности
	awh::real128_t number(2);
	// Извлекаем корень из вещественного числа четверной точности
	awh::real128_t root = number.sqrt();

	// Проверяем что корень совпадает с эталоном в пределах разрядности формата
	ASSERT_EQ(root.print().compare(0, 20, "1.414213562373095048"), 0) << "root = " << root.print();
	// Проверяем что обратное возведение в квадрат укладывается в машинную точность
	ASSERT_TRUE(((root * root) - number).abs() < (awh::real128_t::epsilon() * 4));

	/**
	 * В цикле сверяем извлечение корня из отрицательных значений с аппаратным типом
	 */
	for(const double sample : std::vector <double> ({-1.0, -4.0, -0.5, -1e300})){
		// Проверяем что корень из отрицательного числа не является числом
		ASSERT_TRUE(awh::real64_t(sample).sqrt().category() == awh::bignum::class_t::UNDEFINED) << "value = " << sample;
		// Проверяем что аппаратный тип даёт тот же результат
		ASSERT_TRUE(::isnan(::sqrt(sample))) << "value = " << sample;
	}
	// Проверяем что корень из отрицательного нуля сохраняет знак нуля
	ASSERT_TRUE(this->identical(awh::real64_t(-0.0).sqrt(), ::sqrt(-0.0)));
	// Проверяем что корень из значения не являющегося числом не является числом
	ASSERT_TRUE(awh::real64_t::undefined().sqrt().category() == awh::bignum::class_t::UNDEFINED);
	// Проверяем что корень из бесконечности является бесконечностью
	ASSERT_TRUE(awh::real64_t::unlimited().sqrt().category() == awh::bignum::class_t::UNLIMITED);
	// Проверяем что корень из отрицательной целой величины извлекается по модулю
	ASSERT_EQ(awh::int128_t(-16).sqrt().print(), "4");
}

/**
 * @brief Тест специальных значений вещественного числа
 *
 */
TEST_F(BigNumFixture, RealSpecialBigNumTest){
	// Проверяем класс значения не являющегося числом
	ASSERT_TRUE(awh::real128_t::undefined().category() == awh::bignum::class_t::UNDEFINED);
	// Проверяем класс бесконечного значения
	ASSERT_TRUE(awh::real128_t::unlimited().category() == awh::bignum::class_t::UNLIMITED);
	// Проверяем класс нулевого значения
	ASSERT_TRUE(awh::real128_t(0.0).category() == awh::bignum::class_t::ZERO);
	// Проверяем класс нормализованного значения
	ASSERT_TRUE(awh::real128_t(1.0).category() == awh::bignum::class_t::NORMAL);

	// Проверяем что значение не являющееся числом не равно самому себе
	ASSERT_TRUE(awh::real128_t::undefined() != awh::real128_t::undefined());
	// Проверяем что значение не являющееся числом не упорядочено по возрастанию
	ASSERT_FALSE(awh::real128_t::undefined() < awh::real128_t(1));
	// Проверяем что значение не являющееся числом не упорядочено по убыванию
	ASSERT_FALSE(awh::real128_t::undefined() > awh::real128_t(1));

	// Проверяем что деление на нуль даёт бесконечность
	ASSERT_EQ((awh::real128_t(1) / awh::real128_t(0)).print(), "inf");
	// Проверяем что деление нуля на нуль не является числом
	ASSERT_EQ((awh::real128_t(0) / awh::real128_t(0)).print(), "nan");
	// Проверяем что вычитание бесконечностей не является числом
	ASSERT_EQ((awh::real128_t::unlimited() - awh::real128_t::unlimited()).print(), "nan");
	// Проверяем что сложение бесконечностей даёт бесконечность
	ASSERT_EQ((awh::real128_t::unlimited() + awh::real128_t::unlimited()).print(), "inf");
}

/**
 * @brief Тест знака нуля вещественного числа
 *
 */
TEST_F(BigNumFixture, RealSignedZeroBigNumTest){
	// Создаём отрицательный нуль
	awh::real64_t number = -0.0;

	// Проверяем что знак нуля сохраняется при выводе
	ASSERT_EQ(number.print(), "-0");
	// Проверяем что отрицательный нуль равен положительному
	ASSERT_TRUE(number == awh::real64_t(0.0));
	// Проверяем что число признано нулевым
	ASSERT_TRUE(number.zero());
	// Проверяем что сложение разных нулей даёт положительный нуль
	ASSERT_EQ((number + awh::real64_t(0.0)).print(), "0");
	// Проверяем что сложение отрицательных нулей сохраняет знак
	ASSERT_EQ((awh::real64_t(-0.0) + awh::real64_t(-0.0)).print(), "-0");
	// Проверяем что деление на отрицательный нуль даёт отрицательную бесконечность
	ASSERT_EQ((awh::real64_t(1.0) / awh::real64_t(-0.0)).print(), "-inf");
}

/**
 * @brief Тест предельных величин вещественного числа
 *
 */
TEST_F(BigNumFixture, RealLimitBigNumTest){
	// Проверяем максимальное конечное значение двойной точности
	ASSERT_EQ(static_cast <double> (awh::real64_t::maximum()), 1.7976931348623157e308);
	// Проверяем минимальное конечное значение двойной точности
	ASSERT_EQ(static_cast <double> (awh::real64_t::minimum()), -1.7976931348623157e308);
	// Проверяем машинную точность двойной точности
	ASSERT_EQ(static_cast <double> (awh::real64_t::epsilon()), 2.220446049250313e-16);

	// Создаём единицу для проверки машинной точности
	awh::real64_t one = 1.0;
	// Получаем машинную точность
	awh::real64_t epsilon = awh::real64_t::epsilon();

	// Проверяем что прибавление машинной точности различимо
	ASSERT_TRUE((one + epsilon) > one);
	// Проверяем что прибавление половины машинной точности неразличимо
	ASSERT_TRUE((one + (epsilon >> 1)) == one);

	// Проверяем что превышение предела разрядности даёт бесконечность
	ASSERT_EQ((awh::real64_t::maximum() * awh::real64_t(2)).print(), "inf");
	// Проверяем что превышение предела разрядности при разборе строки даёт бесконечность
	ASSERT_EQ(awh::real64_t("1e400").print(), "inf");
	// Проверяем что значение ниже предела разрядности обнуляется
	ASSERT_EQ(awh::real64_t("1e-400").print(), "0");
}

/**
 * @brief Тест денормализованных значений вещественного числа
 *
 */
TEST_F(BigNumFixture, RealSubnormalBigNumTest){
	// Создаём минимальное денормализованное значение двойной точности
	awh::real64_t number = this->unpack <awh::real64_t> (static_cast <uint64_t> (1));

	// Проверяем класс денормализованного значения
	ASSERT_TRUE(number.category() == awh::bignum::class_t::SUBNORMAL);
	// Проверяем совпадение денормализованного значения с аппаратным
	ASSERT_EQ(static_cast <double> (number), 4.9406564584124654e-324);
	// Проверяем что удвоение денормализованного значения точно
	ASSERT_TRUE(this->identical(number + number, (4.9406564584124654e-324 * 2.0)));

	// Создаём максимальное денормализованное значение двойной точности
	awh::real64_t maximal = this->unpack <awh::real64_t> (static_cast <uint64_t> (0x000FFFFFFFFFFFFFull));

	// Проверяем класс максимального денормализованного значения
	ASSERT_TRUE(maximal.category() == awh::bignum::class_t::SUBNORMAL);
	// Проверяем что прибавление минимального денормализованного значения переводит число в нормализованное
	ASSERT_TRUE((maximal + number).category() == awh::bignum::class_t::NORMAL);
}

/**
 * @brief Тест вещественного числа половинной точности против аппаратного типа
 *
 */
TEST_F(BigNumFixture, RealHalfBigNumTest){
	/**
	 * Если компилятор поддерживает аппаратный тип половинной точности
	 */
	#if defined(__FLT16_MANT_DIG__)
		/**
		 * В цикле сверяем битовый образ числа половинной точности с аппаратным типом
		 */
		for(const double sample : std::vector <double> ({
			0.0, 1.0, -1.0, 0.5, 65504.0, 6.103515625e-05, 5.960464477539063e-08, 3.14159
		})){
			// Приводим значение к аппаратному типу половинной точности
			const _Float16 reference = static_cast <_Float16> (sample);
			// Создаём длинное число половинной точности
			awh::real16_t number = sample;

			// Проверяем побитовое совпадение с аппаратным значением
			ASSERT_TRUE(this->identical(number, reference)) << "value = " << sample;
		}
	#endif
	// Проверяем что превышение предела разрядности даёт бесконечность
	ASSERT_EQ(awh::real16_t(1e6).print(), "inf");
	// Проверяем класс денормализованного значения половинной точности
	ASSERT_TRUE(awh::real16_t(5.960464477539063e-08).category() == awh::bignum::class_t::SUBNORMAL);
	// Проверяем максимальное конечное значение половинной точности
	ASSERT_EQ(static_cast <double> (awh::real16_t::maximum()), 65504.0);
	/**
	 * Кратчайшая запись максимального значения половинной точности короче его точного значения,
	 * поскольку шаг разрядной сетки в этой области равен тридцати двум и запись 65500
	 * восстанавливается в то же самое значение, обходясь тремя значащими цифрами вместо пяти
	 */
	// Проверяем кратчайшую запись максимального значения половинной точности
	ASSERT_EQ(awh::real16_t::maximum().print(), "65500");
	// Проверяем что кратчайшая запись восстанавливается в исходное значение
	ASSERT_TRUE(awh::real16_t(awh::real16_t::maximum().print()) == awh::real16_t::maximum());
}

/**
 * @brief Тест нестандартных разрядностей вещественного числа
 *
 */
TEST_F(BigNumFixture, RealIntermediateBigNumTest){
	// Проверяем вывод вещественного числа 48-битной разрядности
	ASSERT_EQ(awh::real48_t(1.5).print(), "1.5");
	// Проверяем вывод вещественного числа 24-битной разрядности
	ASSERT_EQ(awh::real24_t(2.25).print(), "2.25");
	// Проверяем вывод вещественного числа половинной точности
	ASSERT_EQ(awh::real16_t(1.0).print(), "1");
	// Проверяем вывод вещественного числа 80-битной разрядности
	ASSERT_EQ(awh::real80_t(1.25).print(), "1.25");
	// Проверяем вывод вещественного числа 96-битной разрядности
	ASSERT_EQ(awh::real96_t(0.125).print(), "0.125");
	// Проверяем вывод вещественного числа 256-битной разрядности
	ASSERT_EQ(awh::real256_t("1.5").print(), "1.5");
	// Проверяем что вещественное число одинарной точности совпадает с аппаратным
	ASSERT_TRUE(this->identical(awh::real32_t(0.1), 0.1f));
}

/**
 * @brief Тест точности вещественного числа повышенной разрядности
 *
 */
TEST_F(BigNumFixture, RealPrecisionBigNumTest){
	// Вычисляем одну третью в четверной точности
	awh::real128_t third = (awh::real128_t(1) / awh::real128_t(3));
	// Получаем строковое представление одной третьей
	const std::string text = third.print();

	// Проверяем начало периодической записи одной третьей
	ASSERT_EQ(text.compare(0, 12, "0.3333333333"), 0) << "value = " << text;
	// Проверяем что разрядность формата даёт более тридцати значащих цифр
	ASSERT_GT(text.size(), 30) << "value = " << text;

	// Создаём вещественное число с тридцатью четырьмя значащими цифрами
	awh::real128_t precise("1.234567890123456789012345678901234");

	// Проверяем что значащие цифры разобраны точно
	ASSERT_EQ(precise.print().compare(0, 31, "1.23456789012345678901234567890"), 0) << "value = " << precise.print();

	// Создаём вещественное число повышенной разрядности
	awh::real768_t extended("3.14159265358979323846264338327950288419716939937510582097494459");

	// Проверяем что значащие цифры разобраны точно на повышенной разрядности
	ASSERT_EQ(extended.print().compare(0, 41, "3.141592653589793238462643383279502884197"), 0) << "value = " << extended.print();
}

/**
 * @brief Тест диапазона вещественного числа повышенной разрядности
 *
 */
TEST_F(BigNumFixture, RealRangeBigNumTest){
	// Создаём значение выходящее за пределы двойной точности
	awh::real128_t number("1e4000");

	// Проверяем что значение является нормализованным в четверной точности
	ASSERT_TRUE(number.category() == awh::bignum::class_t::NORMAL);
	// Проверяем вывод значения в научной нотации
	ASSERT_EQ(number.print(awh::bignum::format_t::SCI, 4), "1.0000e+4000");
	// Проверяем что превышение предела четверной точности даёт бесконечность
	ASSERT_EQ(awh::real128_t("1e5000").print(), "inf");
	// Проверяем что значение сохраняется при переносе в повышенную разрядность
	ASSERT_TRUE(awh::real256_t(number).category() == awh::bignum::class_t::NORMAL);
}

/**
 * @brief Тест точного переноса вещественного числа между разрядностями
 *
 */
TEST_F(BigNumFixture, RealConvertBigNumTest){
	// Создаём вещественное число двойной точности
	awh::real64_t number1 = 0.1;
	// Расширяем разрядность вещественного числа
	awh::real256_t number2(number1);

	// Проверяем что расширение разрядности выводит точное двоичное значение
	ASSERT_EQ(number2.print(), "0.1000000000000000055511151231257827021181583404541015625");

	// Сужаем разрядность вещественного числа обратно
	awh::real64_t number3(number2);

	// Проверяем что обратное сужение разрядности вернуло исходное значение
	ASSERT_EQ(number3.print(), "0.1");
	// Проверяем побитовое совпадение с аппаратным значением
	ASSERT_TRUE(this->identical(number3, 0.1));
}

/**
 * @brief Тест масштабирования вещественного числа сдвигом
 *
 */
TEST_F(BigNumFixture, RealShiftBigNumTest){
	// Создаём вещественное число для масштабирования
	awh::real64_t number = 3.0;

	// Проверяем что сдвиг влево умножает число на степень двойки
	ASSERT_EQ((number << 2).print(), "12");
	// Проверяем что сдвиг вправо делит число на степень двойки
	ASSERT_EQ((number >> 1).print(), "1.5");
	// Проверяем что сдвиг за пределы разрядности даёт бесконечность
	ASSERT_EQ((number << 4096).print(), "inf");
	// Проверяем что сдвиг за пределы разрядности обнуляет число
	ASSERT_EQ((number >> 4096).print(), "0");
}
