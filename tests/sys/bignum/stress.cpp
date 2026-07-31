/**
 * @file: stress.cpp
 * @date: 2026-07-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Нагрузочные тесты модуля работы с длинными числами — массовая сверка результатов вычислений
 *        с аппаратными типами и с эталонными побитовыми реализациями деления и извлечения корня
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
#include <random>
#include <limits>
#include <cstring>
#include <cstdint>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "bignum.hpp"

/**
 * @brief Эталонное побитовое деление длинного числа
 *
 * @details Эталон повторяет школьный алгоритм деления в столбик по одному разряду за шаг и
 *          намеренно реализован максимально прямолинейно, чтобы служить независимой проверкой
 *          для быстрого алгоритма деления, работающего сразу над машинными словами.
 *
 * @param result делимое, в которое помещается частное
 * @param value  делитель
 * @param mod    буфер для остатка от деления
 * @param size   размер чисел в байтах
 *
 */
static void referenceDivision(uint8_t * result, const uint8_t * value, uint8_t * mod, const size_t size) noexcept {
	// Создаём буфер частного
	std::vector <uint8_t> quotient(size, 0);
	// Создаём буфер остатка
	std::vector <uint8_t> remainder(size, 0);
	// Создаём копию делимого
	std::vector <uint8_t> dividend(result, (result + size));
	/**
	 * Если делитель является нулевым
	 */
	if(awh::bignum::zero(value, size)){
		// Обнуляем частное
		::memset(result, 0, size);
		// Обнуляем остаток
		::memset(mod, 0, size);
		// Выходим из функции
		return;
	}
	// Получаем количество значащих разрядов делимого
	const size_t count = awh::bignum::bits(dividend.data(), size);
	/**
	 * В цикле обходим значащие разряды делимого от старшего к младшему
	 */
	for(size_t i = count; i-- > 0;){
		// Сдвигаем остаток на один разряд влево
		awh::bignum::shl(remainder.data(), size, 1);
		// Переносим очередной разряд делимого в остаток
		awh::bignum::bit(remainder.data(), size, 0, awh::bignum::bit(dividend.data(), size, i));
		/**
		 * Если остаток не меньше делителя
		 */
		if(awh::bignum::ucompare(remainder.data(), value, size) >= 0){
			// Вычитаем делитель из остатка
			awh::bignum::sub(remainder.data(), value, size);
			// Устанавливаем очередной разряд частного
			awh::bignum::bit(quotient.data(), size, i, true);
		}
	}
	// Копируем частное в результат
	::memcpy(result, quotient.data(), size);
	// Копируем остаток в результат
	::memcpy(mod, remainder.data(), size);
}

/**
 * @brief Эталонное побитовое извлечение корня длинного числа
 *
 * @details Эталон повторяет столбиковый алгоритм извлечения квадратного корня по два разряда
 *          за шаг и служит независимой проверкой для быстрого алгоритма, использующего метод Ньютона.
 *
 * @param value число, в которое помещается результат извлечения корня
 * @param size  размер числа в байтах
 *
 */
static void referenceSqrt(uint8_t * value, const size_t size) noexcept {
	// Если число является нулевым
	if(awh::bignum::zero(value, size))
		// Выходим из функции
		return;
	// Создаём буфер результата
	std::vector <uint8_t> result(size, 0);
	// Создаём буфер пробного разряда
	std::vector <uint8_t> probe(size, 0);
	// Создаём буфер проверяемого значения
	std::vector <uint8_t> checked(size, 0);
	// Получаем количество значащих разрядов числа
	const size_t count = awh::bignum::bits(value, size);

	// Устанавливаем старший чётный разряд в качестве пробного
	awh::bignum::bit(probe.data(), size, (((count - 1) / 2) * 2), true);

	/**
	 * В цикле опускаем пробный разряд до младшего
	 */
	while(!awh::bignum::zero(probe.data(), size)){
		// Копируем накопленный результат в проверяемое значение
		::memcpy(checked.data(), result.data(), size);
		// Прибавляем пробный разряд к проверяемому значению
		awh::bignum::add(checked.data(), probe.data(), size);
		/**
		 * Если остаток числа не меньше проверяемого значения
		 */
		if(awh::bignum::ucompare(value, checked.data(), size) >= 0){
			// Вычитаем проверяемое значение из остатка числа
			awh::bignum::sub(value, checked.data(), size);
			// Сдвигаем накопленный результат на один разряд вправо
			awh::bignum::shr(result.data(), size, 1, false);
			// Прибавляем пробный разряд к накопленному результату
			awh::bignum::add(result.data(), probe.data(), size);
		// Если остаток числа меньше проверяемого значения
		} else awh::bignum::shr(result.data(), size, 1, false);
		// Сдвигаем пробный разряд на два разряда вправо
		awh::bignum::shr(probe.data(), size, 2, false);
	}
	// Копируем накопленный результат в число
	::memcpy(value, result.data(), size);
}

/**
 * @brief Тест беззнаковой целочисленной арифметики против аппаратного типа
 *
 */
TEST_F(BigNumFixture, StressUnsignedBigNumTest){
	/**
	 * Если компилятор поддерживает аппаратный 128-битный тип
	 */
	#if defined(__SIZEOF_INT128__)
		// Создаём генератор случайных чисел
		std::mt19937_64 generator(20260726);

		/**
		 * В цикле сверяем результаты операций с аппаратным типом
		 */
		for(uint32_t i = 0; i < 50000; i++){
			// Получаем первый случайный операнд
			const unsigned __int128 first = ((static_cast <unsigned __int128> (generator()) << 64) | generator());
			// Получаем второй случайный операнд
			const unsigned __int128 second = ((static_cast <unsigned __int128> (generator()) << 64) | generator());
			// Создаём первый операнд в виде длинного числа
			const awh::uint128_t number1 = this->unpack <awh::uint128_t> (first);
			// Создаём второй операнд в виде длинного числа
			const awh::uint128_t number2 = this->unpack <awh::uint128_t> (second);
			// Получаем случайное количество разрядов сдвига
			const uint32_t shift = static_cast <uint32_t> (generator() % 128);

			// Проверяем результат сложения
			ASSERT_TRUE(this->identical(number1 + number2, static_cast <unsigned __int128> (first + second))) << "add at " << i;
			// Проверяем результат вычитания
			ASSERT_TRUE(this->identical(number1 - number2, static_cast <unsigned __int128> (first - second))) << "sub at " << i;
			// Проверяем результат умножения
			ASSERT_TRUE(this->identical(number1 * number2, static_cast <unsigned __int128> (first * second))) << "mul at " << i;
			// Проверяем результат сдвига влево
			ASSERT_TRUE(this->identical(number1 << shift, static_cast <unsigned __int128> (first << shift))) << "shl at " << i;
			// Проверяем результат сдвига вправо
			ASSERT_TRUE(this->identical(number1 >> shift, static_cast <unsigned __int128> (first >> shift))) << "shr at " << i;
			// Проверяем результат сравнения по возрастанию
			ASSERT_EQ((number1 < number2), (first < second)) << "less at " << i;
			// Проверяем результат сравнения на равенство
			ASSERT_EQ((number1 == number2), (first == second)) << "equal at " << i;
			// Проверяем круговой обход через десятичную запись
			ASSERT_TRUE(this->identical(awh::uint128_t(number1.print()), first)) << "roundtrip at " << i;

			/**
			 * Если второй операнд не является нулевым
			 */
			if(second != 0){
				// Проверяем результат деления
				ASSERT_TRUE(this->identical(number1 / number2, static_cast <unsigned __int128> (first / second))) << "div at " << i;
				// Проверяем результат взятия остатка
				ASSERT_TRUE(this->identical(number1 % number2, static_cast <unsigned __int128> (first % second))) << "mod at " << i;
			}
		}
	#else
		// Выводим сообщение о пропуске теста
		GTEST_SKIP() << "the compiler provides no native 128-bit integer type";
	#endif
}

/**
 * @brief Тест знаковой целочисленной арифметики против аппаратного типа
 *
 */
TEST_F(BigNumFixture, StressSignedBigNumTest){
	/**
	 * Если компилятор поддерживает аппаратный 128-битный тип
	 */
	#if defined(__SIZEOF_INT128__)
		// Создаём генератор случайных чисел
		std::mt19937_64 generator(20260727);

		/**
		 * В цикле сверяем результаты операций с аппаратным типом
		 */
		for(uint32_t i = 0; i < 50000; i++){
			// Получаем битовый образ первого случайного операнда
			const unsigned __int128 raw1 = ((static_cast <unsigned __int128> (generator()) << 64) | generator());
			// Получаем битовый образ второго случайного операнда
			const unsigned __int128 raw2 = ((static_cast <unsigned __int128> (generator()) << 64) | generator());
			// Приводим битовый образ первого операнда к знаковому типу
			const __int128 first = static_cast <__int128> (raw1);
			// Приводим битовый образ второго операнда к знаковому типу
			const __int128 second = static_cast <__int128> (raw2);
			// Создаём первый операнд в виде длинного числа
			const awh::int128_t number1 = this->unpack <awh::int128_t> (first);
			// Создаём второй операнд в виде длинного числа
			const awh::int128_t number2 = this->unpack <awh::int128_t> (second);

			/**
			 * Сложение и умножение выполняем над битовыми образами, поскольку знаковое
			 * переполнение аппаратного типа не определено стандартом, а результат по модулю
			 * разрядной сетки совпадает с результатом беззнаковой операции
			 */
			// Проверяем результат сложения
			ASSERT_TRUE(this->identical(number1 + number2, static_cast <__int128> (raw1 + raw2))) << "add at " << i;
			// Проверяем результат вычитания
			ASSERT_TRUE(this->identical(number1 - number2, static_cast <__int128> (raw1 - raw2))) << "sub at " << i;
			// Проверяем результат умножения
			ASSERT_TRUE(this->identical(number1 * number2, static_cast <__int128> (raw1 * raw2))) << "mul at " << i;
			// Проверяем результат сравнения по возрастанию
			ASSERT_EQ((number1 < number2), (first < second)) << "less at " << i;
			// Проверяем круговой обход через десятичную запись
			ASSERT_TRUE(this->identical(awh::int128_t(number1.print()), first)) << "roundtrip at " << i;

			/**
			 * Если деление на второй операнд определено для аппаратного типа
			 */
			if((second != 0) && !((first == std::numeric_limits <__int128>::min()) && (second == -1))){
				// Проверяем результат деления
				ASSERT_TRUE(this->identical(number1 / number2, static_cast <__int128> (first / second))) << "div at " << i;
				// Проверяем результат взятия остатка
				ASSERT_TRUE(this->identical(number1 % number2, static_cast <__int128> (first % second))) << "mod at " << i;

				// Создаём частное статического деления
				awh::int128_t quotient;
				// Создаём остаток статического деления
				awh::int128_t remainder;

				// Выполняем деление с получением частного и остатка за один проход
				awh::int128_t::divmod(number1, number2, quotient, remainder);

				// Проверяем согласованность частного с оператором деления
				ASSERT_TRUE(quotient == (number1 / number2)) << "divmod quotient at " << i;
				// Проверяем согласованность остатка с оператором остатка
				ASSERT_TRUE(remainder == (number1 % number2)) << "divmod remainder at " << i;
				// Проверяем тождество делимого через частное, делитель и остаток
				ASSERT_TRUE(((quotient * number2) + remainder) == number1) << "divmod identity at " << i;
			}
		}
	#else
		// Выводим сообщение о пропуске теста
		GTEST_SKIP() << "the compiler provides no native 128-bit integer type";
	#endif
}

/**
 * @brief Тест вещественной арифметики против аппаратного типа на произвольных битовых образах
 *
 */
TEST_F(BigNumFixture, StressRealBigNumTest){
	// Создаём генератор случайных чисел
	std::mt19937_64 generator(20260728);

	/**
	 * В цикле сверяем результаты операций с аппаратным типом
	 */
	for(uint32_t i = 0; i < 50000; i++){
		// Получаем битовый образ первого случайного операнда
		const uint64_t raw1 = generator();
		// Получаем битовый образ второго случайного операнда
		const uint64_t raw2 = generator();
		// Создаём первый аппаратный операнд
		double first = 0.0;
		// Создаём второй аппаратный операнд
		double second = 0.0;

		// Восстанавливаем первый аппаратный операнд из битового образа
		::memcpy(&first, &raw1, sizeof(first));
		// Восстанавливаем второй аппаратный операнд из битового образа
		::memcpy(&second, &raw2, sizeof(second));

		/**
		 * Если один из операндов является специальным значением
		 */
		if(::isnan(first) || ::isnan(second) || ::isinf(first) || ::isinf(second))
			// Переходим к следующей итерации
			continue;

		// Создаём первый операнд в виде длинного числа
		const awh::real64_t number1 = this->unpack <awh::real64_t> (first);
		// Создаём второй операнд в виде длинного числа
		const awh::real64_t number2 = this->unpack <awh::real64_t> (second);

		// Проверяем результат сложения
		ASSERT_TRUE(this->identical(number1 + number2, (first + second)) || ((first + second) == 0.0)) << "add at " << i;
		// Проверяем результат вычитания
		ASSERT_TRUE(this->identical(number1 - number2, (first - second)) || ((first - second) == 0.0)) << "sub at " << i;
		// Проверяем результат умножения
		ASSERT_TRUE(this->identical(number1 * number2, (first * second)) || ((first * second) == 0.0)) << "mul at " << i;
		// Проверяем результат сравнения по возрастанию
		ASSERT_EQ((number1 < number2), (first < second)) << "less at " << i;
		// Проверяем результат сравнения на равенство
		ASSERT_EQ((number1 == number2), (first == second)) << "equal at " << i;

		// Если второй операнд не является нулевым
		if(second != 0.0)
			// Проверяем результат деления
			ASSERT_TRUE(this->identical(number1 / number2, (first / second)) || ((first / second) == 0.0)) << "div at " << i;

		// Если первый операнд не является отрицательным
		if(first >= 0.0)
			// Проверяем результат извлечения корня
			ASSERT_TRUE(this->identical(number1.sqrt(), ::sqrt(first)) || (first == 0.0)) << "sqrt at " << i;
		// Если первый операнд является отрицательным
		else ASSERT_TRUE(number1.sqrt().category() == awh::bignum::class_t::UNDEFINED) << "negative sqrt at " << i;

		// Создаём число для разбора битового образа
		awh::real64_t restored;

		// Разбираем битовый образ числа
		restored.parse(number1.print(awh::bignum::format_t::HEX), awh::bignum::format_t::HEX);

		// Проверяем круговой обход через битовый образ
		ASSERT_TRUE(this->identical(restored, first)) << "hex roundtrip at " << i;

		/**
		 * Круговой обход через десятичную запись выполняем на каждом четвёртом операнде,
		 * поскольку значения с предельными показателями степени разворачиваются в записи
		 * из сотен значащих цифр и обходятся существенно дороже самой арифметики
		 */
		if((i % 4) != 0)
			// Переходим к следующей итерации
			continue;

		// Проверяем круговой обход через десятичную запись
		ASSERT_TRUE(this->identical(awh::real64_t(number1.print()), first) || (first == 0.0)) << "dec roundtrip at " << i;
		// Проверяем круговой обход через научную нотацию
		ASSERT_TRUE(this->identical(awh::real64_t(number1.print(awh::bignum::format_t::SCI)), first) || (first == 0.0)) << "sci roundtrip at " << i;
	}
}

/**
 * @brief Тест вещественной арифметики против аппаратного типа в обычном диапазоне значений
 *
 */
TEST_F(BigNumFixture, StressRealRangeBigNumTest){
	// Создаём генератор случайных чисел
	std::mt19937_64 generator(20260729);
	// Создаём распределение случайных вещественных значений
	std::uniform_real_distribution <double> distribution(-1e12, 1e12);

	/**
	 * В цикле сверяем результаты операций с аппаратным типом
	 */
	for(uint32_t i = 0; i < 30000; i++){
		// Получаем первый случайный операнд
		const double first = distribution(generator);
		// Получаем второй случайный операнд
		const double second = distribution(generator);
		// Создаём первый операнд в виде длинного числа
		const awh::real64_t number1 = first;
		// Создаём второй операнд в виде длинного числа
		const awh::real64_t number2 = second;

		// Проверяем результат сложения
		ASSERT_TRUE(this->identical(number1 + number2, (first + second))) << "add at " << i;
		// Проверяем результат умножения
		ASSERT_TRUE(this->identical(number1 * number2, (first * second))) << "mul at " << i;
		// Проверяем результат деления
		ASSERT_TRUE(this->identical(number1 / number2, (first / second))) << "div at " << i;
		// Проверяем результат взятия остатка
		ASSERT_TRUE(this->identical(number1 % number2, ::fmod(first, second))) << "mod at " << i;
		// Проверяем круговой обход через десятичную запись
		ASSERT_TRUE(this->identical(awh::real64_t(number1.print()), first)) << "roundtrip at " << i;
	}
}

/**
 * @brief Тест деления длинного числа против эталонной побитовой реализации
 *
 */
TEST_F(BigNumFixture, StressDivisionBigNumTest){
	// Создаём генератор случайных чисел
	std::mt19937_64 generator(777);

	/**
	 * В цикле обходим проверяемые разрядности чисел
	 */
	for(const size_t size : std::vector <size_t> ({2, 3, 4, 5, 6, 7, 8, 9, 12, 16, 17, 20, 24, 32, 33, 48, 64})){
		/**
		 * В цикле проверяем деление на случайных операндах выбранной разрядности
		 */
		for(uint32_t i = 0; i < 300; i++){
			// Создаём буфер делимого
			std::vector <uint8_t> dividend(size, 0);
			// Создаём буфер делителя
			std::vector <uint8_t> divisor(size, 0);

			/**
			 * В цикле заполняем операнды случайными байтами
			 */
			for(size_t j = 0; j < size; j++){
				// Заполняем очередной байт делимого
				dividend[j] = static_cast <uint8_t> (generator());
				// Заполняем очередной байт делителя
				divisor[j] = static_cast <uint8_t> (generator());
			}
			// Получаем количество сохраняемых байтов делителя
			const size_t keep = (generator() % (size + 1));

			/**
			 * В цикле обнуляем старшую часть делителя для изменения количества значащих разрядов
			 */
			for(size_t j = keep; j < size; j++)
				// Обнуляем очередной байт делителя
				divisor[j] = 0;

			// Создаём список проверяемых делителей
			std::vector <std::vector <uint8_t>> divisors;

			// Добавляем случайный делитель
			divisors.push_back(divisor);
			// Добавляем делитель равный делимому
			divisors.push_back(dividend);
			// Добавляем нулевой делитель
			divisors.push_back(std::vector <uint8_t> (size, 0));
			// Добавляем делитель из всех единичных разрядов
			divisors.push_back(std::vector <uint8_t> (size, 0xFF));

			// Создаём делитель равный единице
			std::vector <uint8_t> one(size, 0);

			// Устанавливаем единичное значение делителя
			one[0] = 1;
			// Добавляем единичный делитель
			divisors.push_back(one);

			// Создаём делитель на единицу меньше делимого
			std::vector <uint8_t> lesser(dividend);

			// Если делимое не является нулевым
			if(!awh::bignum::zero(lesser.data(), size))
				// Уменьшаем делитель на единицу
				awh::bignum::sub(lesser.data(), one.data(), size);

			// Добавляем делитель на единицу меньше делимого
			divisors.push_back(lesser);

			// Создаём делитель с граничным старшим разрядом нормализации
			std::vector <uint8_t> normalized(divisor);

			// Устанавливаем граничный старший байт делителя
			normalized[size - 1] = 0x80;
			// Добавляем делитель с граничным старшим разрядом нормализации
			divisors.push_back(normalized);

			/**
			 * В цикле обходим проверяемые делители
			 */
			for(auto & item : divisors){
				// Создаём частное быстрого алгоритма
				std::vector <uint8_t> quotient1(dividend);
				// Создаём остаток быстрого алгоритма
				std::vector <uint8_t> remainder1(size, 0);
				// Создаём частное эталонного алгоритма
				std::vector <uint8_t> quotient2(dividend);
				// Создаём остаток эталонного алгоритма
				std::vector <uint8_t> remainder2(size, 0);

				// Выполняем деление быстрым алгоритмом
				awh::bignum::divmod(quotient1.data(), item.data(), remainder1.data(), size);
				// Выполняем деление эталонным алгоритмом
				referenceDivision(quotient2.data(), item.data(), remainder2.data(), size);

				// Проверяем совпадение частного с эталоном
				ASSERT_EQ(quotient1, quotient2) << "quotient for size " << size << " at " << i;
				// Проверяем совпадение остатка с эталоном
				ASSERT_EQ(remainder1, remainder2) << "remainder for size " << size << " at " << i;

				/**
				 * Если делитель не является нулевым
				 */
				if(!awh::bignum::zero(item.data(), size)){
					// Создаём буфер для проверки тождества деления
					std::vector <uint8_t> identity(quotient1);

					// Умножаем частное на делитель
					awh::bignum::mul(identity.data(), item.data(), size);
					// Прибавляем остаток к произведению
					awh::bignum::add(identity.data(), remainder1.data(), size);

					// Проверяем тождество делимого через частное, делитель и остаток
					ASSERT_EQ(identity, dividend) << "identity for size " << size << " at " << i;
					// Проверяем что остаток меньше делителя
					ASSERT_LT(awh::bignum::ucompare(remainder1.data(), item.data(), size), 0) << "remainder range for size " << size << " at " << i;
				}
			}
		}
	}
}

/**
 * @brief Тест оценки очередного разряда частного на предельных значениях делителя
 *
 * @details Алгоритм деления уточняет оценку разряда частного по двум старшим разрядам
 *          делителя, поэтому проверяются делители, у которых старший разряд лежит на
 *          границе нормализации, а следующий за ним равен предельному значению: именно
 *          такая пара даёт максимальную начальную оценку и максимальное произведение
 *          в условии уточнения. Делимое подбирается так, чтобы его старшая часть
 *          совпадала с делителем, что доводит оценку до предельного значения разряда.
 *
 */
TEST_F(BigNumFixture, StressQuotientEstimateBigNumTest){
	// Создаём генератор случайных чисел
	std::mt19937_64 generator(2026);

	/**
	 * В цикле обходим предельные значения старшего разряда делителя
	 */
	for(const uint32_t top : std::vector <uint32_t> ({0x80000000u, 0x80000001u, 0xC0000000u, 0xFFFFFFFEu, 0xFFFFFFFFu})){
		/**
		 * В цикле обходим количество значащих разрядов делителя
		 */
		for(const size_t words : std::vector <size_t> ({2, 3, 4, 8, 16})){
			// Определяем размер проверяемых чисел в байтах
			const size_t size = ((words + 2) * sizeof(uint32_t));
			// Создаём буфер делителя
			std::vector <uint8_t> divisor(size, 0);
			// Создаём буфер делимого
			std::vector <uint8_t> dividend(size, 0);

			// Устанавливаем старший разряд делителя на границе нормализации
			::memcpy(&divisor[(words - 1) * sizeof(uint32_t)], &top, sizeof(top));

			// Создаём предельное значение следующего разряда делителя
			const uint32_t second = 0xFFFFFFFFu;

			// Устанавливаем предельное значение следующего разряда делителя
			::memcpy(&divisor[(words - 2) * sizeof(uint32_t)], &second, sizeof(second));

			/**
			 * В цикле заполняем младшие разряды делителя предельными значениями
			 */
			for(size_t i = 0; (i + 2) < words; i++)
				// Устанавливаем предельное значение очередного разряда делителя
				::memcpy(&divisor[i * sizeof(uint32_t)], &second, sizeof(second));

			/**
			 * В цикле формируем делимое, старшая часть которого совпадает с делителем
			 */
			for(size_t i = 0; i < words; i++)
				// Копируем очередной разряд делителя в делимое со сдвигом на разряд
				::memcpy(&dividend[(i + 1) * sizeof(uint32_t)], &divisor[i * sizeof(uint32_t)], sizeof(uint32_t));

			// Устанавливаем предельное значение младшего разряда делимого
			::memcpy(&dividend[0], &second, sizeof(second));

			// Создаём единичное значение для смещения делимого
			std::vector <uint8_t> one(size, 0);

			// Устанавливаем единичное значение
			one[0] = 1;

			/**
			 * В цикле проверяем деление на делимых вокруг предельной оценки
			 */
			for(int32_t offset = -3; offset <= 3; offset++){
				// Создаём смещённое делимое
				std::vector <uint8_t> shifted(dividend);

				/**
				 * В цикле смещаем делимое на заданное количество единиц
				 */
				for(int32_t i = 0; i < ((offset < 0) ? -offset : offset); i++){
					// Если требуется уменьшить делимое
					if(offset < 0)
						// Уменьшаем делимое на единицу
						awh::bignum::sub(shifted.data(), one.data(), size);
					// Если требуется увеличить делимое
					else awh::bignum::add(shifted.data(), one.data(), size);
				}
				// Создаём частное быстрого алгоритма
				std::vector <uint8_t> quotient1(shifted);
				// Создаём остаток быстрого алгоритма
				std::vector <uint8_t> remainder1(size, 0);
				// Создаём частное эталонного алгоритма
				std::vector <uint8_t> quotient2(shifted);
				// Создаём остаток эталонного алгоритма
				std::vector <uint8_t> remainder2(size, 0);

				// Выполняем деление быстрым алгоритмом
				awh::bignum::divmod(quotient1.data(), divisor.data(), remainder1.data(), size);
				// Выполняем деление эталонным алгоритмом
				referenceDivision(quotient2.data(), divisor.data(), remainder2.data(), size);

				// Проверяем совпадение частного с эталоном
				ASSERT_EQ(quotient1, quotient2) << "quotient for top " << top << " words " << words << " offset " << offset;
				// Проверяем совпадение остатка с эталоном
				ASSERT_EQ(remainder1, remainder2) << "remainder for top " << top << " words " << words << " offset " << offset;
			}
			/**
			 * В цикле проверяем деление на случайных делимых при том же делителе
			 */
			for(uint32_t i = 0; i < 2000; i++){
				// Создаём случайное делимое
				std::vector <uint8_t> random(size);

				/**
				 * В цикле заполняем делимое случайными байтами
				 */
				for(size_t j = 0; j < size; j++)
					// Заполняем очередной байт делимого
					random[j] = static_cast <uint8_t> (generator());

				// Создаём частное быстрого алгоритма
				std::vector <uint8_t> quotient1(random);
				// Создаём остаток быстрого алгоритма
				std::vector <uint8_t> remainder1(size, 0);
				// Создаём частное эталонного алгоритма
				std::vector <uint8_t> quotient2(random);
				// Создаём остаток эталонного алгоритма
				std::vector <uint8_t> remainder2(size, 0);

				// Выполняем деление быстрым алгоритмом
				awh::bignum::divmod(quotient1.data(), divisor.data(), remainder1.data(), size);
				// Выполняем деление эталонным алгоритмом
				referenceDivision(quotient2.data(), divisor.data(), remainder2.data(), size);

				// Проверяем совпадение частного с эталоном
				ASSERT_EQ(quotient1, quotient2) << "random quotient for top " << top << " words " << words << " at " << i;
				// Проверяем совпадение остатка с эталоном
				ASSERT_EQ(remainder1, remainder2) << "random remainder for top " << top << " words " << words << " at " << i;
			}
		}
	}
}

/**
 * @brief Тест извлечения корня длинного числа против эталонной побитовой реализации
 *
 */
TEST_F(BigNumFixture, StressSqrtBigNumTest){
	// Создаём генератор случайных чисел
	std::mt19937_64 generator(31337);

	/**
	 * В цикле обходим проверяемые разрядности чисел
	 */
	for(const size_t size : std::vector <size_t> ({2, 3, 4, 5, 6, 7, 8, 9, 12, 16, 17, 24, 32, 33, 48, 64})){
		/**
		 * В цикле проверяем извлечение корня из малых значений подряд
		 */
		for(uint32_t i = 0; i < 500; i++){
			// Создаём буфер проверяемого значения
			std::vector <uint8_t> value(size, 0);

			/**
			 * В цикле раскладываем малое значение по байтам
			 */
			for(size_t j = 0; ((j < 4) && (j < size)); j++)
				// Заполняем очередной байт значения
				value[j] = static_cast <uint8_t> ((i >> (j * 8)) & 0xFF);

			// Создаём результат быстрого алгоритма
			std::vector <uint8_t> result1(value);
			// Создаём результат эталонного алгоритма
			std::vector <uint8_t> result2(value);

			// Извлекаем корень быстрым алгоритмом
			awh::bignum::sqrt(result1.data(), size);
			// Извлекаем корень эталонным алгоритмом
			referenceSqrt(result2.data(), size);

			// Проверяем совпадение результата с эталоном
			ASSERT_EQ(result1, result2) << "small value " << i << " for size " << size;
		}
		/**
		 * В цикле проверяем извлечение корня на случайных значениях выбранной разрядности
		 */
		for(uint32_t i = 0; i < 300; i++){
			// Создаём буфер случайного значения
			std::vector <uint8_t> value(size, 0);

			/**
			 * В цикле заполняем значение случайными байтами
			 */
			for(size_t j = 0; j < size; j++)
				// Заполняем очередной байт значения
				value[j] = static_cast <uint8_t> (generator());

			// Получаем количество сохраняемых байтов значения
			const size_t keep = (generator() % (size + 1));

			/**
			 * В цикле обнуляем старшую часть значения для изменения количества значащих разрядов
			 */
			for(size_t j = keep; j < size; j++)
				// Обнуляем очередной байт значения
				value[j] = 0;

			// Создаём буфер основания точного квадрата
			std::vector <uint8_t> base(size, 0);

			/**
			 * В цикле заполняем младшую половину основания случайными байтами
			 */
			for(size_t j = 0; j < (size / 2); j++)
				// Заполняем очередной байт основания
				base[j] = static_cast <uint8_t> (generator());

			// Создаём буфер точного квадрата
			std::vector <uint8_t> square(base);

			// Возводим основание в квадрат
			awh::bignum::mul(square.data(), base.data(), size);

			// Создаём единичное значение для смещения точного квадрата
			std::vector <uint8_t> one(size, 0);

			// Устанавливаем единичное значение
			one[0] = 1;

			// Создаём значение на единицу больше точного квадрата
			std::vector <uint8_t> greater(square);

			// Увеличиваем точный квадрат на единицу
			awh::bignum::add(greater.data(), one.data(), size);

			// Создаём список проверяемых значений
			std::vector <std::vector <uint8_t>> values;

			// Добавляем случайное значение
			values.push_back(value);
			// Добавляем точный квадрат
			values.push_back(square);
			// Добавляем значение на единицу больше точного квадрата
			values.push_back(greater);
			// Добавляем значение из всех единичных разрядов
			values.push_back(std::vector <uint8_t> (size, 0xFF));

			/**
			 * В цикле обходим проверяемые значения
			 */
			for(auto & item : values){
				// Создаём результат быстрого алгоритма
				std::vector <uint8_t> result1(item);
				// Создаём результат эталонного алгоритма
				std::vector <uint8_t> result2(item);

				// Извлекаем корень быстрым алгоритмом
				awh::bignum::sqrt(result1.data(), size);
				// Извлекаем корень эталонным алгоритмом
				referenceSqrt(result2.data(), size);

				// Проверяем совпадение результата с эталоном
				ASSERT_EQ(result1, result2) << "random value for size " << size << " at " << i;

				// Создаём буфер квадрата полученного корня
				std::vector <uint8_t> squared(result1);

				// Возводим полученный корень в квадрат
				awh::bignum::mul(squared.data(), result1.data(), size);

				// Проверяем что квадрат корня не превышает исходное значение
				ASSERT_LE(awh::bignum::ucompare(squared.data(), item.data(), size), 0) << "lower bound for size " << size << " at " << i;

				// Создаём буфер следующего за корнем значения
				std::vector <uint8_t> next(result1);

				// Увеличиваем полученный корень на единицу
				awh::bignum::add(next.data(), one.data(), size);

				// Создаём буфер квадрата следующего за корнем значения
				std::vector <uint8_t> upper(next);

				// Возводим следующее за корнем значение в квадрат
				awh::bignum::mul(upper.data(), next.data(), size);

				/**
				 * Верхнюю границу проверяем только когда квадрат не выходит за разрядную сетку
				 */
				if((awh::bignum::bits(next.data(), size) * 2) <= (size * 8))
					// Проверяем что квадрат следующего за корнем значения превышает исходное
					ASSERT_GT(awh::bignum::ucompare(upper.data(), item.data(), size), 0) << "upper bound for size " << size << " at " << i;
			}
		}
	}
}
