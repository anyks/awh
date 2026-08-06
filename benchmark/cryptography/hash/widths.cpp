/**
 * @file: widths.cpp
 * @date: 2026-07-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Сценарии измерения вывода результата хэширования — стоимость формирования результата
 *        разной разрядности, вывода в длинные числа модуля BigNum и работы специализации
 *        хэширования стандартной библиотеки
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <functional>

/**
 * Подключаем заголовочный файл бенчмарков модуля хэширования
 */
#include "hash.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён бенчмарков модуля хэширования
 */
using namespace awh::benchmark::hash;

/**
 * @brief Внутренние параметры и сценарии бенчмарков вывода результата хэширования
 *
 */
namespace {
	/**
	 * @brief Количество операций сценариев вывода результата хэширования
	 *
	 */
	static constexpr size_t WIDTH_ROUNDS = 2000000;
	/**
	 * @brief Количество операций сценария вывода результата наибольшей разрядности
	 *
	 */
	static constexpr size_t WIDE_ROUNDS = 200000;
	/**
	 * @brief Количество операций сценария учёта выделений памяти
	 *
	 */
	static constexpr size_t ALLOCATION_ROUNDS = 100000;
	/**
	 * @brief Порог скорости вывода 32-битного результата в операциях в секунду
	 *
	 * @details Результат разрядностью до машинного слова возвращается числом,
	 *          минуя буфер и поток октетов: показатель ловит потерю этого пути,
	 *          при которой хэширование ключа хэш-таблицы начинает проходить
	 *          через формирование потока октетов
	 *
	 */
	static constexpr double WIDTH_32_THRESHOLD = 12000000.0;
	/**
	 * @brief Порог скорости вывода 64-битного результата в операциях в секунду
	 *
	 */
	static constexpr double WIDTH_64_THRESHOLD = 12000000.0;
	/**
	 * @brief Порог скорости вывода 128-битного результата в операциях в секунду
	 *
	 * @details Результат свыше машинного слова формируется потоком октетов:
	 *          каждые следующие восемь октетов стоят одного перемешивания
	 *
	 */
	static constexpr double WIDTH_128_THRESHOLD = 3900000.0;
	/**
	 * @brief Порог скорости вывода 256-битного результата в операциях в секунду
	 *
	 */
	static constexpr double WIDTH_256_THRESHOLD = 2350000.0;
	/**
	 * @brief Порог скорости вывода 1024-битного результата в операциях в секунду
	 *
	 */
	static constexpr double WIDTH_1024_THRESHOLD = 700000.0;
	/**
	 * @brief Порог скорости хэширования длинного числа в операциях в секунду
	 *
	 */
	static constexpr double BIGNUM_THRESHOLD = 11500000.0;
	/**
	 * @brief Порог скорости работы специализации стандартной библиотеки в операциях в секунду
	 *
	 * @details Специализация создаёт объект хэширования на каждый вызов, поэтому
	 *          показатель ловит появление в конструкторе работы, выходящей за
	 *          пределы заполнения состояния
	 *
	 */
	static constexpr double STANDARD_THRESHOLD = 3300000.0;
	/**
	 * @brief Порог количества выделений памяти на один вывод результата
	 *
	 * @details Ограничение сверху: результат любой разрядности формируется в
	 *          буфере вызывающего, а длинное число хранит его в собственном
	 *          массиве октетов, поэтому выделений быть не должно вовсе
	 *
	 */
	static constexpr double ALLOCATIONS_THRESHOLD = 0.01;

	/**
	 * @brief Шаблон типа результата хэширования сценария
	 *
	 * @tparam T тип результата хэширования
	 *
	 */
	template <typename T>
	/**
	 * @brief Функция прогона сценария вывода результата хэширования в длинное число
	 *
	 * @param rounds количество выполняемых операций
	 * @return       итоги прогона сценария
	 *
	 */
	static outcome_t width(const size_t rounds) noexcept {
		// Получаем эталонный объект хэширования
		const awh::hash_t & hash = engine();
		// Получаем буфер данных для хэширования
		const uint8_t * data = buffer().data();
		// Накопитель результата операций
		uint64_t accumulator = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(rounds, TEXT_SIZE, [&]() noexcept {
			// Выполняем хэширование буфера данных
			const T value = hash.hash <T> (data, TEXT_SIZE);
			/**
			 * Результат читается из буфера длинного числа напрямую: обращение к
			 * нему методом объекта добавило бы к замеру вызов через границу
			 * единицы трансляции, стоящий дороже самого хэширования
			 */
			accumulator += (* reinterpret_cast <const uint8_t *> (&value));
		});
		// Накапливаем контрольную сумму прогона
		checksum() += accumulator;
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция формирования результата измерения по итогам прогона
	 *
	 * @param outcome итоги прогона сценария
	 * @return        результат измерения
	 *
	 */
	static awh::benchmark::result_t outcome(const outcome_t & outcome) noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария вывода 32-битного результата
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t width32() noexcept {
		// Получаем эталонный объект хэширования
		const awh::hash_t & hash = engine();
		// Получаем буфер данных для хэширования
		const uint8_t * data = buffer().data();
		// Накопитель результата операций
		uint64_t accumulator = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(WIDTH_ROUNDS, TEXT_SIZE, [&]() noexcept {
			// Выполняем хэширование буфера данных
			accumulator += hash.hash <uint32_t> (data, TEXT_SIZE);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += accumulator;
		// Выводим результат измерения
		return ::outcome(result);
	}
	/**
	 * @brief Функция прогона сценария вывода 64-битного результата
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t width64() noexcept {
		// Получаем эталонный объект хэширования
		const awh::hash_t & hash = engine();
		// Получаем буфер данных для хэширования
		const uint8_t * data = buffer().data();
		// Накопитель результата операций
		uint64_t accumulator = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(WIDTH_ROUNDS, TEXT_SIZE, [&]() noexcept {
			// Выполняем хэширование буфера данных
			accumulator += hash.hash <uint64_t> (data, TEXT_SIZE);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += accumulator;
		// Выводим результат измерения
		return ::outcome(result);
	}
	/**
	 * @brief Функция прогона сценария вывода 128-битного результата
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t width128() noexcept {
		// Выводим результат измерения
		return ::outcome(width <awh::uint128_t> (WIDTH_ROUNDS));
	}
	/**
	 * @brief Функция прогона сценария вывода 256-битного результата
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t width256() noexcept {
		// Выводим результат измерения
		return ::outcome(width <awh::uint256_t> (WIDTH_ROUNDS));
	}
	/**
	 * @brief Функция прогона сценария вывода 1024-битного результата
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t width1024() noexcept {
		// Выводим результат измерения
		return ::outcome(width <awh::uint1024_t> (WIDE_ROUNDS));
	}
	/**
	 * @brief Функция прогона сценария хэширования длинного числа
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t bignum() noexcept {
		// Получаем эталонный объект хэширования
		const awh::hash_t & hash = engine();
		// Создаём длинное число для хэширования
		const awh::uint256_t num = (awh::uint256_t(1) << 200);
		// Накопитель результата операций
		uint64_t accumulator = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(WIDTH_ROUNDS, awh::uint256_t::size(), [&]() noexcept {
			// Выполняем хэширование длинного числа
			accumulator += hash.hash <uint64_t> (num);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += accumulator;
		// Выводим результат измерения
		return ::outcome(result);
	}
	/**
	 * @brief Функция прогона сценария работы специализации стандартной библиотеки
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t standard() noexcept {
		// Создаём объект хэширования стандартной библиотеки
		const std::hash <awh::uint256_t> hash;
		// Создаём длинное число для хэширования
		const awh::uint256_t num = (awh::uint256_t(1) << 200);
		// Накопитель результата операций
		uint64_t accumulator = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(WIDTH_ROUNDS, awh::uint256_t::size(), [&]() noexcept {
			// Выполняем хэширование длинного числа
			accumulator += hash(num);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += accumulator;
		// Выводим результат измерения
		return ::outcome(result);
	}
	/**
	 * @brief Функция прогона сценария учёта выделений памяти
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t allocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = width <awh::uint1024_t> (ALLOCATION_ROUNDS);
		// Устанавливаем измеренное значение
		result.value = perOperation(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	// Регистрируем сценарий вывода 32-битного результата
	static const bool gWidth32 = awh::benchmark::add(
		"hash/widths/uint32", "операций/с", WIDTH_32_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::width32
	);
	// Регистрируем сценарий вывода 64-битного результата
	static const bool gWidth64 = awh::benchmark::add(
		"hash/widths/uint64", "операций/с", WIDTH_64_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::width64
	);
	// Регистрируем сценарий вывода 128-битного результата
	static const bool gWidth128 = awh::benchmark::add(
		"hash/widths/uint128", "операций/с", WIDTH_128_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::width128
	);
	// Регистрируем сценарий вывода 256-битного результата
	static const bool gWidth256 = awh::benchmark::add(
		"hash/widths/uint256", "операций/с", WIDTH_256_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::width256
	);
	// Регистрируем сценарий вывода 1024-битного результата
	static const bool gWidth1024 = awh::benchmark::add(
		"hash/widths/uint1024", "операций/с", WIDTH_1024_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::width1024
	);
	// Регистрируем сценарий хэширования длинного числа
	static const bool gBigNum = awh::benchmark::add(
		"hash/widths/bignum-key", "операций/с", BIGNUM_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::bignum
	);
	// Регистрируем сценарий работы специализации стандартной библиотеки
	static const bool gStandard = awh::benchmark::add(
		"hash/widths/std-hash", "операций/с", STANDARD_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::standard
	);
	// Регистрируем сценарий учёта выделений памяти
	static const bool gAllocations = awh::benchmark::add(
		"hash/widths/allocations-per-op", "выделений", ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::allocations
	);
};
