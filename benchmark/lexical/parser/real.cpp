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
 * @brief Сценарии измерения разбора чисел с плавающей точкой — все три пути разбора
 *        от быстрого умножения до точного сравнения длинной арифметикой и учёт выделений памяти
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл бенчмарков модуля разбора чисел
 */
#include "parser.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён бенчмарков модуля разбора чисел
 */
using namespace awh::benchmark::parser;

/**
 * @brief Внутренние параметры и сценарии бенчмарков разбора чисел с плавающей точкой
 *
 */
namespace {
	/**
	 * @brief Количество операций сценариев быстрого и приближённого путей разбора
	 *
	 */
	static constexpr size_t PARSE_ROUNDS = 5000000;
	/**
	 * @brief Количество операций сценариев точного пути разбора
	 *
	 */
	static constexpr size_t EXACT_ROUNDS = 500000;
	/**
	 * @brief Количество операций сценария учёта выделений памяти
	 *
	 */
	static constexpr size_t ALLOCATION_ROUNDS = 200000;
	/**
	 * @brief Порог скорости разбора числа быстрым путём
	 *
	 * @details Пороги пропускной способности зависят от машины и режима сборки:
	 *          модуль является заголовочным и собирается флагами того, кто его
	 *          подключил, а бенчмарки собираются без флагов оптимизации и медленнее
	 *          оптимизированной сборки в несколько раз. Поэтому пороги откалиброваны
	 *          по неоптимизированной сборке с четырёхкратным запасом - они ловят
	 *          регрессии на порядок, а не колебания окружения
	 *
	 */
	static constexpr double PARSE_FAST_THRESHOLD = 3500000.0;
	/**
	 * @brief Порог скорости разбора числа приближением Эйзеля-Лемира
	 *
	 * @details Показатель ловит потерю быстрого пути приближения: без таблицы
	 *          степеней пятёрки каждое такое число уходило бы на точное сравнение
	 *          длинной арифметикой, то есть на путь дороже на порядок
	 *
	 */
	static constexpr double PARSE_SCIENTIFIC_THRESHOLD = 2200000.0;
	/**
	 * @brief Порог скорости разбора числа точным путём
	 *
	 * @details Точный путь строит десятичное представление мантиссы длинной
	 *          арифметикой и сравнивает его с двоичным приближением поразрядно,
	 *          поэтому дороже приближённого пути на порядок
	 *
	 */
	static constexpr double PARSE_EXACT_THRESHOLD = 500000.0;
	/**
	 * @brief Порог скорости разбора числа с усечённой мантиссой
	 *
	 */
	static constexpr double PARSE_LONG_THRESHOLD = 1000000.0;
	/**
	 * @brief Порог скорости разбора числа одинарной точности
	 *
	 * @details Разбор выполняется на той же записи, что и быстрый путь, но идёт
	 *          приближением: шестнадцать значащих цифр не укладываются в
	 *          24-разрядную мантиссу одинарной точности, и точное умножение
	 *          становится невозможным. Показатель поэтому сопоставим со сценарием
	 *          научной записи, а не со сценарием быстрого пути
	 *
	 */
	static constexpr double PARSE_FLOAT_THRESHOLD = 2400000.0;
	/**
	 * @brief Порог количества выделений памяти на один разбор числа
	 *
	 * @details Ограничение сверху: разбор не выделяет память ни на одном из путей,
	 *          включая точный - буфер разрядов длинной арифметики размещается на
	 *          стеке. В отличие от пропускной способности показатель от машины и
	 *          режима сборки не зависит, поэтому порог задан вплотную к измеренному
	 *          значению
	 *
	 */
	static constexpr double ALLOCATIONS_THRESHOLD = 0.01;

	/**
	 * @brief Функция прогона сценария разбора числа быстрым путём
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t parseFast() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись числа
		const string & text = realFast();
		// Начало разбираемой записи числа
		const char * first = text.c_str();
		// Конец разбираемой записи числа
		const char * last = (first + text.size());
		// Накопитель результатов разбора
		double accumulator = 0.0;
		// Результат разбора записи числа
		double value = 0.0;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(PARSE_ROUNDS, [&]() noexcept {
			// Выполняем разбор записи числа
			awh::lexical_t::fromChars(first, last, value);
			// Накапливаем результат разбора
			accumulator += value;
			// Запрещаем вынос измеряемого разбора за пределы цикла
			barrier(&accumulator);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += static_cast <uint64_t> (accumulator);
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария разбора числа приближением Эйзеля-Лемира
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t parseScientific() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись числа
		const string & text = realScientific();
		// Начало разбираемой записи числа
		const char * first = text.c_str();
		// Конец разбираемой записи числа
		const char * last = (first + text.size());
		// Накопитель результатов разбора
		double accumulator = 0.0;
		// Результат разбора записи числа
		double value = 0.0;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(PARSE_ROUNDS, [&]() noexcept {
			// Выполняем разбор записи числа
			awh::lexical_t::fromChars(first, last, value);
			// Накапливаем результат разбора
			accumulator += value;
			// Запрещаем вынос измеряемого разбора за пределы цикла
			barrier(&accumulator);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += static_cast <uint64_t> (accumulator != 0.0);
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария разбора числа точным путём
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t parseExact() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись числа
		const string & text = realExact();
		// Начало разбираемой записи числа
		const char * first = text.c_str();
		// Конец разбираемой записи числа
		const char * last = (first + text.size());
		// Накопитель результатов разбора
		double accumulator = 0.0;
		// Результат разбора записи числа
		double value = 0.0;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(EXACT_ROUNDS, [&]() noexcept {
			// Выполняем разбор записи числа
			awh::lexical_t::fromChars(first, last, value);
			// Накапливаем результат разбора
			accumulator += value;
			// Запрещаем вынос измеряемого разбора за пределы цикла
			barrier(&accumulator);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += static_cast <uint64_t> (accumulator != 0.0);
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария разбора числа с усечённой мантиссой
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t parseLong() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись числа
		const string & text = realLong();
		// Начало разбираемой записи числа
		const char * first = text.c_str();
		// Конец разбираемой записи числа
		const char * last = (first + text.size());
		// Накопитель результатов разбора
		double accumulator = 0.0;
		// Результат разбора записи числа
		double value = 0.0;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(EXACT_ROUNDS, [&]() noexcept {
			// Выполняем разбор записи числа
			awh::lexical_t::fromChars(first, last, value);
			// Накапливаем результат разбора
			accumulator += value;
			// Запрещаем вынос измеряемого разбора за пределы цикла
			barrier(&accumulator);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += static_cast <uint64_t> (accumulator != 0.0);
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария разбора числа одинарной точности
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t parseFloat() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись числа
		const string & text = realFast();
		// Начало разбираемой записи числа
		const char * first = text.c_str();
		// Конец разбираемой записи числа
		const char * last = (first + text.size());
		// Накопитель результатов разбора
		float accumulator = 0.0f;
		// Результат разбора записи числа
		float value = 0.0f;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(PARSE_ROUNDS, [&]() noexcept {
			// Выполняем разбор записи числа
			awh::lexical_t::fromChars(first, last, value);
			// Накапливаем результат разбора
			accumulator += value;
			// Запрещаем вынос измеряемого разбора за пределы цикла
			barrier(&accumulator);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += static_cast <uint64_t> (accumulator);
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария учёта выделений памяти на точном пути разбора
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t parseAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись числа
		const string & text = realExact();
		// Начало разбираемой записи числа
		const char * first = text.c_str();
		// Конец разбираемой записи числа
		const char * last = (first + text.size());
		// Накопитель результатов разбора
		double accumulator = 0.0;
		// Результат разбора записи числа
		double value = 0.0;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(ALLOCATION_ROUNDS, [&]() noexcept {
			// Выполняем разбор записи числа
			awh::lexical_t::fromChars(first, last, value);
			// Накапливаем результат разбора
			accumulator += value;
			// Запрещаем вынос измеряемого разбора за пределы цикла
			barrier(&accumulator);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += static_cast <uint64_t> (accumulator != 0.0);
		// Вычисляем количество выделений памяти на одну операцию
		result.value = perOperation(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	// Регистрируем сценарий разбора числа быстрым путём
	static const bool gParseFast = awh::benchmark::add(
		"lexical/real/parse-double-fast", "операций/с", PARSE_FAST_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::parseFast
	);
	// Регистрируем сценарий разбора числа приближением Эйзеля-Лемира
	static const bool gParseScientific = awh::benchmark::add(
		"lexical/real/parse-double-scientific", "операций/с", PARSE_SCIENTIFIC_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::parseScientific
	);
	// Регистрируем сценарий разбора числа точным путём
	static const bool gParseExact = awh::benchmark::add(
		"lexical/real/parse-double-exact", "операций/с", PARSE_EXACT_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::parseExact
	);
	// Регистрируем сценарий разбора числа с усечённой мантиссой
	static const bool gParseLong = awh::benchmark::add(
		"lexical/real/parse-double-long", "операций/с", PARSE_LONG_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::parseLong
	);
	// Регистрируем сценарий разбора числа одинарной точности
	static const bool gParseFloat = awh::benchmark::add(
		"lexical/real/parse-float", "операций/с", PARSE_FLOAT_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::parseFloat
	);
	// Регистрируем сценарий учёта выделений памяти на точном пути разбора
	static const bool gParseAllocations = awh::benchmark::add(
		"lexical/real/allocations-per-parse", "выделений", ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::parseAllocations
	);
};
