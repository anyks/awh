/**
 * @file integer.cpp
 * @date 2026-07-26
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
 * @brief Сценарии измерения разбора целых чисел — десятичная запись разной длины,
 *        произвольные основания системы счисления, отказ по переполнению и учёт выделений памяти
 *
 * @copyright Copyright © 2026
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
 * @brief Внутренние параметры и сценарии бенчмарков разбора целых чисел
 *
 */
namespace {
	/**
	 * @brief Количество операций сценариев разбора целых чисел
	 *
	 */
	static constexpr size_t PARSE_ROUNDS = 5000000;
	/**
	 * @brief Количество операций сценария учёта выделений памяти
	 *
	 */
	static constexpr size_t ALLOCATION_ROUNDS = 1000000;
	/**
	 * @brief Порог скорости разбора десятичной записи 32-битного числа
	 *
	 * @details Пороги пропускной способности зависят от машины и режима сборки:
	 *          модуль является заголовочным и собирается флагами того, кто его
	 *          подключил, а бенчмарки собираются без флагов оптимизации и медленнее
	 *          оптимизированной сборки в несколько раз. Поэтому пороги откалиброваны
	 *          по неоптимизированной сборке с четырёхкратным запасом - они ловят
	 *          регрессии на порядок, а не колебания окружения
	 *
	 */
	static constexpr double PARSE_INT32_THRESHOLD = 7000000.0;
	/**
	 * @brief Порог скорости разбора десятичной записи 64-битного числа
	 *
	 * @details Девятнадцать цифр разбираются в один проход: блочно по восемь цифр
	 *          за такт и поразрядно для остатка. Показатель ловит потерю блочного
	 *          разбора - без него каждая цифра стоила бы отдельного умножения
	 *
	 */
	static constexpr double PARSE_INT64_THRESHOLD = 6000000.0;
	/**
	 * @brief Порог скорости разбора десятичной записи с контролем переполнения
	 *
	 * @details При предельных для десятичной записи двадцати цифрах накопленное
	 *          значение могло переполниться, поэтому разбор выполняется повторно
	 *          с точной проверкой на каждой цифре. Разница с предыдущим сценарием -
	 *          это цена одной лишней цифры в записи
	 *
	 */
	static constexpr double PARSE_CHECKED_THRESHOLD = 3000000.0;
	/**
	 * @brief Порог скорости разбора шестнадцатеричной записи целого числа
	 *
	 * @details Показатель ловит потерю блочного разбора десятичных цифр: запись по
	 *          основанию, отличному от десяти, разбирается только поразрядно, и
	 *          сравнение с десятичным сценарием показывает вклад блочного пути.
	 *          Шестнадцать цифр записи являются для основания предельными, поэтому
	 *          разбор дополнительно выполняется повторно с контролем переполнения
	 *
	 */
	static constexpr double PARSE_HEX_THRESHOLD = 2500000.0;
	/**
	 * @brief Порог скорости разбора записи целого числа по основанию 36
	 *
	 */
	static constexpr double PARSE_BASE36_THRESHOLD = 3000000.0;
	/**
	 * @brief Порог скорости отказа по переполнению разрядности
	 *
	 * @details Отказ обязан обнаруживаться по количеству цифр до накопления значения:
	 *          показатель ловит регрессию, при которой заведомо непомещающаяся запись
	 *          разбирается полностью и лишь затем отбраковывается
	 *
	 */
	static constexpr double PARSE_OVERFLOW_THRESHOLD = 6000000.0;
	/**
	 * @brief Порог количества выделений памяти на один разбор целого числа
	 *
	 * @details Ограничение сверху: разбор выполняется на месте по указателям на
	 *          исходную строку и выделений не требует вовсе. В отличие от пропускной
	 *          способности показатель от машины и режима сборки не зависит, поэтому
	 *          порог задан вплотную к измеренному значению
	 *
	 */
	static constexpr double ALLOCATIONS_THRESHOLD = 0.01;

	/**
	 * @brief Функция прогона сценария разбора десятичной записи 32-битного числа
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t parseInteger32() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись числа
		const string & text = integer32();
		// Начало разбираемой записи числа
		const char * first = text.c_str();
		// Конец разбираемой записи числа
		const char * last = (first + text.size());
		// Накопитель результатов разбора
		uint64_t accumulator = 0;
		// Результат разбора записи числа
		int32_t value = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(PARSE_ROUNDS, [&]() noexcept {
			// Выполняем разбор десятичной записи числа
			awh::lexical_t::fromChars(first, last, value);
			// Накапливаем результат разбора
			accumulator += static_cast <uint64_t> (value);
			// Запрещаем вынос измеряемого разбора за пределы цикла
			barrier(&accumulator);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += accumulator;
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария разбора десятичной записи 64-битного числа
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t parseInteger64() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись числа
		const string & text = integer64();
		// Начало разбираемой записи числа
		const char * first = text.c_str();
		// Конец разбираемой записи числа
		const char * last = (first + text.size());
		// Накопитель результатов разбора
		uint64_t accumulator = 0;
		// Результат разбора записи числа
		int64_t value = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(PARSE_ROUNDS, [&]() noexcept {
			// Выполняем разбор десятичной записи числа
			awh::lexical_t::fromChars(first, last, value);
			// Накапливаем результат разбора
			accumulator += static_cast <uint64_t> (value);
			// Запрещаем вынос измеряемого разбора за пределы цикла
			barrier(&accumulator);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += accumulator;
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария разбора десятичной записи с контролем переполнения
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t parseChecked() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись числа
		const string & text = integerChecked();
		// Начало разбираемой записи числа
		const char * first = text.c_str();
		// Конец разбираемой записи числа
		const char * last = (first + text.size());
		// Накопитель результатов разбора
		uint64_t accumulator = 0;
		// Результат разбора записи числа
		uint64_t value = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(PARSE_ROUNDS, [&]() noexcept {
			// Выполняем разбор десятичной записи числа
			awh::lexical_t::fromChars(first, last, value);
			// Накапливаем результат разбора
			accumulator += value;
			// Запрещаем вынос измеряемого разбора за пределы цикла
			barrier(&accumulator);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += accumulator;
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария разбора шестнадцатеричной записи целого числа
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t parseHex() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись числа
		const string & text = integerHex();
		// Начало разбираемой записи числа
		const char * first = text.c_str();
		// Конец разбираемой записи числа
		const char * last = (first + text.size());
		// Накопитель результатов разбора
		uint64_t accumulator = 0;
		// Результат разбора записи числа
		int64_t value = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(PARSE_ROUNDS, [&]() noexcept {
			// Выполняем разбор шестнадцатеричной записи числа
			awh::lexical_t::fromChars(first, last, value, 16);
			// Накапливаем результат разбора
			accumulator += static_cast <uint64_t> (value);
			// Запрещаем вынос измеряемого разбора за пределы цикла
			barrier(&accumulator);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += accumulator;
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария разбора записи целого числа по основанию 36
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t parseBase36() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись числа
		const string & text = integerBase36();
		// Начало разбираемой записи числа
		const char * first = text.c_str();
		// Конец разбираемой записи числа
		const char * last = (first + text.size());
		// Накопитель результатов разбора
		uint64_t accumulator = 0;
		// Результат разбора записи числа
		int64_t value = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(PARSE_ROUNDS, [&]() noexcept {
			// Выполняем разбор записи числа по основанию 36
			awh::lexical_t::fromChars(first, last, value, 36);
			// Накапливаем результат разбора
			accumulator += static_cast <uint64_t> (value);
			// Запрещаем вынос измеряемого разбора за пределы цикла
			barrier(&accumulator);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += accumulator;
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария отказа по переполнению разрядности
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t parseOverflow() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись числа
		const string & text = integerOverflow();
		// Начало разбираемой записи числа
		const char * first = text.c_str();
		// Конец разбираемой записи числа
		const char * last = (first + text.size());
		// Накопитель кодов отказа разбора
		uint64_t accumulator = 0;
		// Результат разбора записи числа
		int64_t value = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(PARSE_ROUNDS, [&]() noexcept {
			// Выполняем разбор заведомо непомещающейся записи числа
			const auto answer = awh::lexical_t::fromChars(first, last, value);
			// Накапливаем код отказа разбора
			accumulator += static_cast <uint64_t> (answer.error);
			// Запрещаем вынос измеряемого разбора за пределы цикла
			barrier(&accumulator);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += accumulator;
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария учёта выделений памяти при разборе целого числа
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t parseAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись числа
		const string & text = integer64();
		// Начало разбираемой записи числа
		const char * first = text.c_str();
		// Конец разбираемой записи числа
		const char * last = (first + text.size());
		// Накопитель результатов разбора
		uint64_t accumulator = 0;
		// Результат разбора записи числа
		int64_t value = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(ALLOCATION_ROUNDS, [&]() noexcept {
			// Выполняем разбор десятичной записи числа
			awh::lexical_t::fromChars(first, last, value);
			// Накапливаем результат разбора
			accumulator += static_cast <uint64_t> (value);
			// Запрещаем вынос измеряемого разбора за пределы цикла
			barrier(&accumulator);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += accumulator;
		// Вычисляем количество выделений памяти на одну операцию
		result.value = perOperation(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	// Регистрируем сценарий разбора десятичной записи 32-битного числа
	static const bool gParseInteger32 = awh::benchmark::add(
		"lexical/integer/parse-int32-dec", "операций/с", PARSE_INT32_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::parseInteger32
	);
	// Регистрируем сценарий разбора десятичной записи 64-битного числа
	static const bool gParseInteger64 = awh::benchmark::add(
		"lexical/integer/parse-int64-dec", "операций/с", PARSE_INT64_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::parseInteger64
	);
	// Регистрируем сценарий разбора десятичной записи с контролем переполнения
	static const bool gParseChecked = awh::benchmark::add(
		"lexical/integer/parse-uint64-checked", "операций/с", PARSE_CHECKED_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::parseChecked
	);
	// Регистрируем сценарий разбора шестнадцатеричной записи целого числа
	static const bool gParseHex = awh::benchmark::add(
		"lexical/integer/parse-int64-hex", "операций/с", PARSE_HEX_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::parseHex
	);
	// Регистрируем сценарий разбора записи целого числа по основанию 36
	static const bool gParseBase36 = awh::benchmark::add(
		"lexical/integer/parse-int64-base36", "операций/с", PARSE_BASE36_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::parseBase36
	);
	// Регистрируем сценарий отказа по переполнению разрядности
	static const bool gParseOverflow = awh::benchmark::add(
		"lexical/integer/parse-overflow", "операций/с", PARSE_OVERFLOW_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::parseOverflow
	);
	// Регистрируем сценарий учёта выделений памяти при разборе целого числа
	static const bool gParseAllocations = awh::benchmark::add(
		"lexical/integer/allocations-per-parse", "выделений", ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::parseAllocations
	);
};
