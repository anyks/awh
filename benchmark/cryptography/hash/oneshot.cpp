/**
 * @file oneshot.cpp
 * @date 2026-07-31
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
 * @brief Сценарии измерения одноразового хэширования — стоимость операции на размерах данных,
 *        покрывающих все ветви обработки, пропускная способность на крупных буферах
 *        и учёт выделений памяти
 *
 * @copyright Copyright © 2026
 *
 */

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
 * @brief Внутренние параметры и сценарии бенчмарков одноразового хэширования
 *
 */
namespace {
	/**
	 * @brief Количество операций сценария хэширования машинного слова
	 *
	 */
	static constexpr size_t TINY_ROUNDS = 5000000;
	/**
	 * @brief Количество операций сценария хэширования короткого ключа
	 *
	 */
	static constexpr size_t SHORT_ROUNDS = 5000000;
	/**
	 * @brief Количество операций сценария хэширования текстового ключа
	 *
	 */
	static constexpr size_t TEXT_ROUNDS = 3000000;
	/**
	 * @brief Количество операций сценария хэширования данных размером с блок
	 *
	 */
	static constexpr size_t BLOCK_ROUNDS = 3000000;
	/**
	 * @brief Количество операций сценария хэширования данных среднего размера
	 *
	 */
	static constexpr size_t MEDIUM_ROUNDS = 1000000;
	/**
	 * @brief Количество операций сценария хэширования страницы памяти
	 *
	 */
	static constexpr size_t PAGE_ROUNDS = 200000;
	/**
	 * @brief Количество операций сценария хэширования крупного буфера
	 *
	 */
	static constexpr size_t LARGE_ROUNDS = 2000;
	/**
	 * @brief Количество операций сценария учёта выделений памяти
	 *
	 */
	static constexpr size_t ALLOCATION_ROUNDS = 100000;
	/**
	 * @brief Порог скорости хэширования машинного слова в операциях в секунду
	 *
	 * @details Пороги пропускной способности зависят от машины и режима сборки:
	 *          библиотека собирается без флагов оптимизации и медленнее
	 *          оптимизированной в несколько раз. Поэтому пороги откалиброваны
	 *          по неоптимизированной сборке с четырёхкратным запасом - они ловят
	 *          регрессии на порядок, а не колебания окружения
	 *
	 * @details Показатель ловит потерю быстрого пути коротких данных: свёртка
	 *          данных размером до блока выполняется минуя состояние движка и
	 *          возвращается вызывающему числом, а не через буфер результата
	 *
	 */
	static constexpr double TINY_THRESHOLD = 14000000.0;
	/**
	 * @brief Порог скорости хэширования короткого ключа в операциях в секунду
	 *
	 */
	static constexpr double SHORT_THRESHOLD = 15000000.0;
	/**
	 * @brief Порог скорости хэширования текстового ключа в операциях в секунду
	 *
	 */
	static constexpr double TEXT_THRESHOLD = 12000000.0;
	/**
	 * @brief Порог скорости хэширования данных размером с блок в операциях в секунду
	 *
	 */
	static constexpr double BLOCK_THRESHOLD = 8500000.0;
	/**
	 * @brief Порог пропускной способности данных среднего размера в октетах в секунду
	 *
	 */
	static constexpr double MEDIUM_THRESHOLD = 330000000.0;
	/**
	 * @brief Порог пропускной способности страницы памяти в октетах в секунду
	 *
	 */
	static constexpr double PAGE_THRESHOLD = 430000000.0;
	/**
	 * @brief Порог пропускной способности крупного буфера в октетах в секунду
	 *
	 * @details Показатель ловит потерю независимости разрядов состояния движка:
	 *          четыре разряда поглощают блок независимо друг от друга, что
	 *          позволяет процессору выполнять их умножение одновременно, а
	 *          сведение их в цепочку зависимых умножений замедляет обработку
	 *          крупных данных кратно
	 *
	 */
	static constexpr double LARGE_THRESHOLD = 450000000.0;
	/**
	 * @brief Порог количества выделений памяти на одну операцию хэширования
	 *
	 * @details Ограничение сверху: состояние движка и буфер неполного блока
	 *          размещаются в самом объекте хэширования, а результат - в буфере
	 *          вызывающего, поэтому выделений быть не должно вовсе. В отличие
	 *          от пропускной способности показатель от машины и режима сборки
	 *          не зависит, поэтому порог задан вплотную к измеренному значению
	 *
	 */
	static constexpr double ALLOCATIONS_THRESHOLD = 0.01;

	/**
	 * @brief Шаблон типа результата хэширования сценария
	 *
	 * @tparam T тип результата хэширования
	 *
	 */
	template <typename T = uint64_t>
	/**
	 * @brief Функция прогона сценария одноразового хэширования
	 *
	 * @param rounds количество выполняемых операций
	 * @param size   размер хэшируемых данных в октетах
	 * @return       итоги прогона сценария
	 *
	 */
	static outcome_t oneshot(const size_t rounds, const size_t size) noexcept {
		// Получаем эталонный объект хэширования
		const awh::hash_t & hash = engine();
		// Получаем буфер данных для хэширования
		const uint8_t * data = buffer().data();
		// Накопитель результата операций
		uint64_t accumulator = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(rounds, size, [&]() noexcept {
			// Выполняем хэширование буфера данных
			accumulator += hash.hash <T> (data, size);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += accumulator;
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария хэширования машинного слова
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t tiny() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = oneshot(TINY_ROUNDS, TINY_SIZE);
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария хэширования короткого ключа
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t brief() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = oneshot(SHORT_ROUNDS, SHORT_SIZE);
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария хэширования текстового ключа
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t text() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = oneshot(TEXT_ROUNDS, TEXT_SIZE);
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария хэширования данных размером с блок
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t block() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = oneshot(BLOCK_ROUNDS, BLOCK_SIZE);
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария хэширования данных среднего размера
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t medium() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = oneshot(MEDIUM_ROUNDS, MEDIUM_SIZE);
		// Устанавливаем измеренное значение
		result.value = perBytes(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария хэширования страницы памяти
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t page() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = oneshot(PAGE_ROUNDS, MEMORY_PAGE_SIZE);
		// Устанавливаем измеренное значение
		result.value = perBytes(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария хэширования крупного буфера
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t large() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = oneshot(LARGE_ROUNDS, LARGE_SIZE);
		// Устанавливаем измеренное значение
		result.value = perBytes(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
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
		const outcome_t outcome = oneshot(ALLOCATION_ROUNDS, MEDIUM_SIZE);
		// Устанавливаем измеренное значение
		result.value = perOperation(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	// Регистрируем сценарий хэширования машинного слова
	static const bool gTiny = awh::benchmark::add(
		"hash/oneshot/key-8", "операций/с", TINY_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::tiny
	);
	// Регистрируем сценарий хэширования короткого ключа
	static const bool gShort = awh::benchmark::add(
		"hash/oneshot/key-16", "операций/с", SHORT_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::brief
	);
	// Регистрируем сценарий хэширования текстового ключа
	static const bool gText = awh::benchmark::add(
		"hash/oneshot/key-32", "операций/с", TEXT_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::text
	);
	// Регистрируем сценарий хэширования данных размером с блок
	static const bool gBlock = awh::benchmark::add(
		"hash/oneshot/key-64", "операций/с", BLOCK_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::block
	);
	// Регистрируем сценарий хэширования данных среднего размера
	static const bool gMedium = awh::benchmark::add(
		"hash/oneshot/bandwidth-256", "октетов/с", MEDIUM_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::medium
	);
	// Регистрируем сценарий хэширования страницы памяти
	static const bool gPage = awh::benchmark::add(
		"hash/oneshot/bandwidth-4k", "октетов/с", PAGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::page
	);
	// Регистрируем сценарий хэширования крупного буфера
	static const bool gLarge = awh::benchmark::add(
		"hash/oneshot/bandwidth-1m", "октетов/с", LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::large
	);
	// Регистрируем сценарий учёта выделений памяти
	static const bool gAllocations = awh::benchmark::add(
		"hash/oneshot/allocations-per-op", "выделений", ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::allocations
	);
};
