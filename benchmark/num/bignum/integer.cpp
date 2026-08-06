/**
 * @file: integer.cpp
 * @date: 2026-07-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Сценарии измерения целочисленной длинной арифметики — сложение, умножение,
 *        деление и извлечение корня на разной разрядности, а также учёт выделений памяти
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл бенчмарков модуля длинных чисел
 */
#include "bignum.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён бенчмарков модуля длинных чисел
 */
using namespace awh::benchmark::bignum;

/**
 * @brief Внутренние параметры и сценарии бенчмарков целочисленной арифметики
 *
 */
namespace {
	/**
	 * @brief Количество операций сценария сложения 128-битных чисел
	 *
	 */
	static constexpr size_t ADD_128_ROUNDS = 2000000;
	/**
	 * @brief Количество операций сценария умножения 128-битных чисел
	 *
	 */
	static constexpr size_t MUL_128_ROUNDS = 500000;
	/**
	 * @brief Количество операций сценария деления 128-битных чисел
	 *
	 */
	static constexpr size_t DIV_128_ROUNDS = 300000;
	/**
	 * @brief Количество операций сценария умножения 2048-битных чисел
	 *
	 */
	static constexpr size_t MUL_2048_ROUNDS = 50000;
	/**
	 * @brief Количество операций сценария деления 2048-битных чисел
	 *
	 */
	static constexpr size_t DIV_2048_ROUNDS = 200000;
	/**
	 * @brief Количество операций сценария извлечения корня 512-битного числа
	 *
	 */
	static constexpr size_t SQRT_512_ROUNDS = 20000;
	/**
	 * @brief Количество операций сценария учёта выделений памяти
	 *
	 */
	static constexpr size_t ALLOCATION_ROUNDS = 20000;
	/**
	 * @brief Порог скорости сложения 128-битных чисел в операциях в секунду
	 *
	 * @details Пороги пропускной способности зависят от машины и режима сборки:
	 *          библиотека собирается без флагов оптимизации и медленнее
	 *          оптимизированной в несколько раз. Поэтому пороги откалиброваны
	 *          по неоптимизированной сборке с четырёхкратным запасом - они ловят
	 *          регрессии на порядок, а не колебания окружения
	 *
	 */
	static constexpr double ADD_128_THRESHOLD = 12000000.0;
	/**
	 * @brief Порог скорости умножения 128-битных чисел в операциях в секунду
	 *
	 */
	static constexpr double MUL_128_THRESHOLD = 4000000.0;
	/**
	 * @brief Порог скорости деления 128-битных чисел в операциях в секунду
	 *
	 * @details Показатель ловит возврат к побитовому делению: оно выполняет по
	 *          итерации на каждый значащий бит делимого и медленнее алгоритма D
	 *          Кнута на 128 битах примерно в двадцать раз
	 *
	 */
	static constexpr double DIV_128_THRESHOLD = 1750000.0;
	/**
	 * @brief Порог скорости умножения 2048-битных чисел в операциях в секунду
	 *
	 * @details Показатель ловит возврат к побайтовым разрядам: умножение
	 *          квадратично по количеству разрядов, поэтому переход с 32-битных
	 *          разрядов на 8-битные даёт шестнадцатикратное замедление
	 *
	 */
	static constexpr double MUL_2048_THRESHOLD = 56000.0;
	/**
	 * @brief Порог скорости деления 2048-битных чисел в операциях в секунду
	 *
	 */
	static constexpr double DIV_2048_THRESHOLD = 130000.0;
	/**
	 * @brief Порог скорости извлечения корня 512-битного числа в операциях в секунду
	 *
	 */
	static constexpr double SQRT_512_THRESHOLD = 37000.0;
	/**
	 * @brief Количество операций сценария округления целого числа
	 *
	 */
	static constexpr size_t ROUND_ROUNDS = 200000;
	/**
	 * @brief Порог скорости округления целого числа в операциях в секунду
	 *
	 * @details Округление до степени десяти выполняется делением с остатком на эту
	 *          степень и обратным умножением, поэтому стоит примерно как деление
	 *          и умножение вместе
	 *
	 */
	static constexpr double ROUND_THRESHOLD = 440000.0;
	/**
	 * @brief Порог количества выделений памяти на одну целочисленную операцию
	 *
	 * @details Ограничение сверху: все временные буферы целочисленной арифметики
	 *          разрядностью до 1024 октетов размещаются на стеке, поэтому выделений
	 *          быть не должно вовсе. В отличие от пропускной способности показатель
	 *          от машины и режима сборки не зависит, поэтому порог задан вплотную
	 *          к измеренному значению
	 *
	 */
	static constexpr double ALLOCATIONS_THRESHOLD = 0.01;

	/**
	 * @brief Функция прогона сценария сложения 128-битных чисел
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t addition128() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Накопитель результата операций
		awh::uint128_t accumulator = first128();
		// Второе слагаемое операции
		const awh::uint128_t & value = second128();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(ADD_128_ROUNDS, [&]() noexcept {
			// Выполняем сложение длинных чисел
			accumulator += value;
		});
		// Накапливаем контрольную сумму прогона
		checksum() += accumulator[0];
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария умножения 128-битных чисел
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t multiplication128() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Накопитель результата операций
		awh::uint128_t accumulator;
		// Множитель операции
		const awh::uint128_t & value = second128();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(MUL_128_ROUNDS, [&]() noexcept {
			/**
			 * Восстанавливаем множимое перед каждой операцией: накопление
			 * произведения обнулило бы его за несколько итераций и умножение
			 * пошло бы по короткому пути нулевых разрядов
			 */
			accumulator = first128();
			// Выполняем умножение длинных чисел
			accumulator *= value;
		});
		// Накапливаем контрольную сумму прогона
		checksum() += accumulator[0];
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария деления 128-битных чисел
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t division128() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Накопитель результата операций
		awh::uint128_t accumulator;
		// Делитель операции
		const awh::uint128_t & value = second128();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(DIV_128_ROUNDS, [&]() noexcept {
			// Восстанавливаем делимое перед каждой операцией
			accumulator = first128();
			// Выполняем деление длинных чисел
			accumulator /= value;
		});
		// Накапливаем контрольную сумму прогона
		checksum() += accumulator[0];
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария умножения 2048-битных чисел
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t multiplication2048() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Накопитель результата операций
		awh::uint2048_t accumulator;
		// Множитель операции
		const awh::uint2048_t & value = second2048();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(MUL_2048_ROUNDS, [&]() noexcept {
			// Восстанавливаем множимое перед каждой операцией
			accumulator = first2048();
			// Выполняем умножение длинных чисел
			accumulator *= value;
		});
		// Накапливаем контрольную сумму прогона
		checksum() += accumulator[0];
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария деления 2048-битных чисел
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t division2048() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Накопитель результата операций
		awh::uint2048_t accumulator;
		// Делитель операции
		const awh::uint2048_t & value = second2048();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(DIV_2048_ROUNDS, [&]() noexcept {
			// Восстанавливаем делимое перед каждой операцией
			accumulator = first2048();
			// Выполняем деление длинных чисел
			accumulator /= value;
		});
		// Накапливаем контрольную сумму прогона
		checksum() += accumulator[0];
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария извлечения корня 512-битного числа
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t root512() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Накопитель результата операций
		awh::uint512_t accumulator;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(SQRT_512_ROUNDS, [&]() noexcept {
			// Выполняем извлечение целочисленного квадратного корня
			accumulator = first512().sqrt();
		});
		// Накапливаем контрольную сумму прогона
		checksum() += accumulator[0];
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария учёта выделений памяти
	 *
	 * @details Прогоняется полный набор операций на всех участвующих разрядностях:
	 *          выделение памяти в любой из них покажет себя в общем показателе
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t allocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Накопитель результата операций разрядностью 128 бит
		awh::uint128_t small;
		// Накопитель результата операций разрядностью 2048 бит
		awh::uint2048_t large;
		// Выполняем прогон измеряемых операций
		const outcome_t outcome = measure(ALLOCATION_ROUNDS, [&]() noexcept {
			// Выполняем сложение длинных чисел разрядностью 128 бит
			small = (first128() + second128());
			// Выполняем умножение длинных чисел разрядностью 128 бит
			small *= second128();
			// Выполняем деление длинных чисел разрядностью 128 бит
			small /= second128();
			// Выполняем извлечение остатка от деления длинных чисел разрядностью 128 бит
			small %= second128();
			// Выполняем сложение длинных чисел разрядностью 2048 бит
			large = (first2048() + second2048());
			// Выполняем умножение длинных чисел разрядностью 2048 бит
			large *= second2048();
			// Выполняем деление длинных чисел разрядностью 2048 бит
			large /= second2048();
			// Выполняем извлечение остатка от деления длинных чисел разрядностью 2048 бит
			large %= second2048();
		});
		// Накапливаем контрольную сумму прогона
		checksum() += (small[0] + large[0]);
		// Приводим итоги прогона к количеству отдельных операций
		outcome_t scaled = outcome;
		// Каждый круг прогона выполняет восемь отдельных операций
		scaled.operations = (outcome.operations * 8);
		// Вычисляем количество выделений памяти на одну операцию
		result.value = perOperation(scaled);
		// Устанавливаем сведения о прогоне
		result.details = details(scaled);
		// Выводим результат измерения
		return result;
	}

	/**
	 * @brief Функция прогона сценария округления целого числа
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t rounding128() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Накопитель результата операций
		awh::uint128_t accumulator;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(ROUND_ROUNDS, [&]() noexcept {
			// Выполняем округление числа до миллионов
			accumulator = first128().round(-6);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += accumulator[0];
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	// Регистрируем сценарий сложения 128-битных чисел
	static const bool gAdd128 = awh::benchmark::add(
		"bignum/integer/add-128", "операций/с", ADD_128_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::addition128
	);
	// Регистрируем сценарий умножения 128-битных чисел
	static const bool gMul128 = awh::benchmark::add(
		"bignum/integer/mul-128", "операций/с", MUL_128_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::multiplication128
	);
	// Регистрируем сценарий деления 128-битных чисел
	static const bool gDiv128 = awh::benchmark::add(
		"bignum/integer/div-128", "операций/с", DIV_128_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::division128
	);
	// Регистрируем сценарий умножения 2048-битных чисел
	static const bool gMul2048 = awh::benchmark::add(
		"bignum/integer/mul-2048", "операций/с", MUL_2048_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::multiplication2048
	);
	// Регистрируем сценарий деления 2048-битных чисел
	static const bool gDiv2048 = awh::benchmark::add(
		"bignum/integer/div-2048", "операций/с", DIV_2048_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::division2048
	);
	// Регистрируем сценарий извлечения корня 512-битного числа
	static const bool gSqrt512 = awh::benchmark::add(
		"bignum/integer/sqrt-512", "операций/с", SQRT_512_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::root512
	);
	// Регистрируем сценарий округления целого числа
	static const bool gRound128 = awh::benchmark::add(
		"bignum/integer/round-128", "операций/с", ROUND_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::rounding128
	);
	// Регистрируем сценарий учёта выделений памяти
	static const bool gAllocations = awh::benchmark::add(
		"bignum/integer/allocations-per-op", "выделений", ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::allocations
	);
};
