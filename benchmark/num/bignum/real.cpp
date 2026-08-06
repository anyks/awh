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
 * @brief Сценарии измерения вещественной длинной арифметики — сложение, умножение,
 *        деление и извлечение корня формата IEEE-754 разной разрядности,
 *        а также учёт выделений памяти
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
 * @brief Внутренние параметры и сценарии бенчмарков вещественной арифметики
 *
 */
namespace {
	/**
	 * @brief Количество операций сценариев вещественных чисел двойной точности
	 *
	 */
	static constexpr size_t ROUNDS_64 = 300000;
	/**
	 * @brief Количество операций сценариев вещественных чисел четверной точности
	 *
	 */
	static constexpr size_t ROUNDS_128 = 200000;
	/**
	 * @brief Количество операций сценария извлечения корня четверной точности
	 *
	 */
	static constexpr size_t SQRT_ROUNDS = 50000;
	/**
	 * @brief Количество операций сценария учёта выделений памяти
	 *
	 */
	static constexpr size_t ALLOCATION_ROUNDS = 20000;
	/**
	 * @brief Порог скорости сложения вещественных чисел двойной точности
	 *
	 * @details Пороги пропускной способности зависят от машины и режима сборки:
	 *          библиотека собирается без флагов оптимизации и медленнее
	 *          оптимизированной в несколько раз. Поэтому пороги откалиброваны
	 *          по неоптимизированной сборке с четырёхкратным запасом - они ловят
	 *          регрессии на порядок, а не колебания окружения
	 *
	 */
	static constexpr double ADD_64_THRESHOLD = 850000.0;
	/**
	 * @brief Порог скорости умножения вещественных чисел двойной точности
	 *
	 */
	static constexpr double MUL_64_THRESHOLD = 675000.0;
	/**
	 * @brief Порог скорости деления вещественных чисел двойной точности
	 *
	 */
	static constexpr double DIV_64_THRESHOLD = 415000.0;
	/**
	 * @brief Порог скорости сложения вещественных чисел четверной точности
	 *
	 */
	static constexpr double ADD_128_THRESHOLD = 505000.0;
	/**
	 * @brief Порог скорости умножения вещественных чисел четверной точности
	 *
	 */
	static constexpr double MUL_128_THRESHOLD = 400000.0;
	/**
	 * @brief Порог скорости деления вещественных чисел четверной точности
	 *
	 */
	static constexpr double DIV_128_THRESHOLD = 269000.0;
	/**
	 * @brief Порог скорости извлечения корня вещественного числа четверной точности
	 *
	 */
	static constexpr double SQRT_128_THRESHOLD = 42500.0;
	/**
	 * @brief Количество операций сценария округления значения
	 *
	 */
	static constexpr size_t ROUND_ROUNDS = 30000;
	/**
	 * @brief Порог скорости округления значения вещественного числа
	 *
	 * @details Округление значения выполняется в десятичной системе счисления:
	 *          строится точное десятичное представление числа, отбрасываются младшие
	 *          цифры и результат переводится обратно в двоичный формат. Такой порядок
	 *          дороже наивного умножения на степень десяти, но исключает двойное
	 *          округление, поскольку степени десяти в двоичном формате не представимы
	 *
	 */
	static constexpr double ROUND_THRESHOLD = 128000.0;
	/**
	 * @brief Порог количества выделений памяти на одну вещественную операцию
	 *
	 * @details Ограничение сверху: рабочий буфер мантиссы вещественного числа
	 *          занимает три его размера плюс шестнадцать октетов, что для всех
	 *          объявленных разрядностей не превышает размера буфера на стеке,
	 *          поэтому выделений быть не должно вовсе. Показатель ловит возврат
	 *          к размещению буфера мантиссы в динамической памяти: распаковка
	 *          двух операндов и упаковка результата давали два выделения на
	 *          каждую арифметическую операцию
	 *
	 */
	static constexpr double ALLOCATIONS_THRESHOLD = 0.01;

	/**
	 * @brief Функция прогона сценария сложения вещественных чисел двойной точности
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t addition64() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Накопитель результата операций
		awh::real64_t accumulator;
		// Второе слагаемое операции
		const awh::real64_t & value = second64r();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(ROUNDS_64, [&]() noexcept {
			/**
			 * Восстанавливаем первое слагаемое перед каждой операцией: накопление
			 * суммы уводило бы порядок результата и меняло бы объём выравнивания
			 * мантисс от итерации к итерации
			 */
			accumulator = first64r();
			// Выполняем сложение вещественных чисел
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
	 * @brief Функция прогона сценария умножения вещественных чисел двойной точности
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t multiplication64() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Накопитель результата операций
		awh::real64_t accumulator;
		// Множитель операции
		const awh::real64_t & value = second64r();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(ROUNDS_64, [&]() noexcept {
			// Восстанавливаем множимое перед каждой операцией
			accumulator = first64r();
			// Выполняем умножение вещественных чисел
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
	 * @brief Функция прогона сценария деления вещественных чисел двойной точности
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t division64() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Накопитель результата операций
		awh::real64_t accumulator;
		// Делитель операции
		const awh::real64_t & value = second64r();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(ROUNDS_64, [&]() noexcept {
			// Восстанавливаем делимое перед каждой операцией
			accumulator = first64r();
			// Выполняем деление вещественных чисел
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
	 * @brief Функция прогона сценария сложения вещественных чисел четверной точности
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t addition128() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Накопитель результата операций
		awh::real128_t accumulator;
		// Второе слагаемое операции
		const awh::real128_t & value = second128r();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(ROUNDS_128, [&]() noexcept {
			// Восстанавливаем первое слагаемое перед каждой операцией
			accumulator = first128r();
			// Выполняем сложение вещественных чисел
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
	 * @brief Функция прогона сценария умножения вещественных чисел четверной точности
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t multiplication128() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Накопитель результата операций
		awh::real128_t accumulator;
		// Множитель операции
		const awh::real128_t & value = second128r();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(ROUNDS_128, [&]() noexcept {
			// Восстанавливаем множимое перед каждой операцией
			accumulator = first128r();
			// Выполняем умножение вещественных чисел
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
	 * @brief Функция прогона сценария деления вещественных чисел четверной точности
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t division128() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Накопитель результата операций
		awh::real128_t accumulator;
		// Делитель операции
		const awh::real128_t & value = second128r();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(ROUNDS_128, [&]() noexcept {
			// Восстанавливаем делимое перед каждой операцией
			accumulator = first128r();
			// Выполняем деление вещественных чисел
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
	 * @brief Функция прогона сценария извлечения корня четверной точности
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t root128() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Накопитель результата операций
		awh::real128_t accumulator;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(SQRT_ROUNDS, [&]() noexcept {
			// Выполняем извлечение квадратного корня вещественного числа
			accumulator = first128r().sqrt();
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
	 * @details Прогоняется полный набор операций на обеих разрядностях: выделение
	 *          памяти в любой из них покажет себя в общем показателе
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t allocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Накопитель результата операций двойной точности
		awh::real64_t small;
		// Накопитель результата операций четверной точности
		awh::real128_t large;
		// Выполняем прогон измеряемых операций
		const outcome_t outcome = measure(ALLOCATION_ROUNDS, [&]() noexcept {
			// Выполняем сложение вещественных чисел двойной точности
			small = (first64r() + second64r());
			// Выполняем умножение вещественных чисел двойной точности
			small *= second64r();
			// Выполняем деление вещественных чисел двойной точности
			small /= second64r();
			// Выполняем извлечение корня вещественного числа двойной точности
			small = small.sqrt();
			// Выполняем сложение вещественных чисел четверной точности
			large = (first128r() + second128r());
			// Выполняем умножение вещественных чисел четверной точности
			large *= second128r();
			// Выполняем деление вещественных чисел четверной точности
			large /= second128r();
			// Выполняем извлечение корня вещественного числа четверной точности
			large = large.sqrt();
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
	 * @brief Функция прогона сценария округления значения вещественного числа
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t rounding() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Накопитель результата операций
		awh::real64_t accumulator;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(ROUND_ROUNDS, [&]() noexcept {
			// Выполняем округление значения до двух знаков после запятой
			accumulator = first64r().round(2);
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

	// Регистрируем сценарий сложения вещественных чисел двойной точности
	static const bool gAdd64 = awh::benchmark::add(
		"bignum/real/add-64", "операций/с", ADD_64_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::addition64
	);
	// Регистрируем сценарий умножения вещественных чисел двойной точности
	static const bool gMul64 = awh::benchmark::add(
		"bignum/real/mul-64", "операций/с", MUL_64_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::multiplication64
	);
	// Регистрируем сценарий деления вещественных чисел двойной точности
	static const bool gDiv64 = awh::benchmark::add(
		"bignum/real/div-64", "операций/с", DIV_64_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::division64
	);
	// Регистрируем сценарий сложения вещественных чисел четверной точности
	static const bool gAdd128 = awh::benchmark::add(
		"bignum/real/add-128", "операций/с", ADD_128_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::addition128
	);
	// Регистрируем сценарий умножения вещественных чисел четверной точности
	static const bool gMul128 = awh::benchmark::add(
		"bignum/real/mul-128", "операций/с", MUL_128_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::multiplication128
	);
	// Регистрируем сценарий деления вещественных чисел четверной точности
	static const bool gDiv128 = awh::benchmark::add(
		"bignum/real/div-128", "операций/с", DIV_128_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::division128
	);
	// Регистрируем сценарий извлечения корня вещественного числа четверной точности
	static const bool gSqrt128 = awh::benchmark::add(
		"bignum/real/sqrt-128", "операций/с", SQRT_128_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::root128
	);
	// Регистрируем сценарий округления значения вещественного числа
	static const bool gRound = awh::benchmark::add(
		"bignum/real/round-64", "операций/с", ROUND_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::rounding
	);
	// Регистрируем сценарий учёта выделений памяти
	static const bool gAllocations = awh::benchmark::add(
		"bignum/real/allocations-per-op", "выделений", ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::allocations
	);
};
