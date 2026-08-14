/**
 * @file strings.cpp
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
 * @brief Сценарии измерения строковых преобразований длинных чисел — формирование
 *        и разбор десятичной и шестнадцатеричной записи целых и вещественных чисел
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>

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
 * @brief Внутренние параметры и сценарии бенчмарков строковых преобразований
 *
 */
namespace {
	/**
	 * @brief Количество операций сценариев целочисленных преобразований
	 *
	 */
	static constexpr size_t INTEGER_ROUNDS = 200000;
	/**
	 * @brief Количество операций сценариев вещественных преобразований
	 *
	 */
	static constexpr size_t REAL_ROUNDS = 30000;
	/**
	 * @brief Порог скорости формирования десятичной записи 128-битного числа
	 *
	 * @details Пороги пропускной способности зависят от машины и режима сборки:
	 *          библиотека собирается без флагов оптимизации и медленнее
	 *          оптимизированной в несколько раз. Поэтому пороги откалиброваны
	 *          по неоптимизированной сборке с четырёхкратным запасом - они ловят
	 *          регрессии на порядок, а не колебания окружения
	 *
	 */
	static constexpr double PRINT_DEC_THRESHOLD = 435000.0;
	/**
	 * @brief Порог скорости разбора десятичной записи 128-битного числа
	 *
	 */
	static constexpr double PARSE_DEC_THRESHOLD = 560000.0;
	/**
	 * @brief Порог скорости формирования шестнадцатеричной записи 128-битного числа
	 *
	 */
	static constexpr double PRINT_HEX_THRESHOLD = 515000.0;
	/**
	 * @brief Порог скорости формирования десятичной записи вещественного числа
	 *
	 * @details Формирование записи вещественного числа дороже целочисленного на
	 *          порядок: точное десятичное представление мантиссы строится умножением
	 *          на степень пятёрки, после чего выполняется поиск минимального
	 *          количества значащих цифр, при котором обратный разбор даёт исходное
	 *          значение. Поиск идёт делением отрезка пополам и выполняет до пяти
	 *          обратных сборок числа
	 *
	 */
	static constexpr double PRINT_REAL_THRESHOLD = 46000.0;
	/**
	 * @brief Порог скорости разбора десятичной записи вещественного числа
	 *
	 */
	static constexpr double PARSE_REAL_THRESHOLD = 205000.0;
	/**
	 * @brief Порог количества выделений памяти на формирование записи числа
	 *
	 * @details Ограничение сверху: строковое представление возвращается объектом
	 *          строки, поэтому одно выделение на вызов неустранимо, а рабочие буферы
	 *          преобразования размещаются на стеке. Показатель ловит появление
	 *          промежуточных строк и векторов в пути формирования записи
	 *
	 */
	static constexpr double PRINT_ALLOCATIONS_THRESHOLD = 1.5;
	/**
	 * @brief Порог количества выделений памяти на разбор записи числа
	 *
	 * @details Ограничение сверху: разбор целочисленной записи выполняется на месте
	 *          и выделений не требует вовсе
	 *
	 */
	static constexpr double PARSE_ALLOCATIONS_THRESHOLD = 0.01;
	/**
	 * @brief Порог количества выделений памяти на формирование записи вещественного числа
	 *
	 * @details Ограничение сверху: формирование записи выделяет память трижды - на
	 *          десятичное представление мантиссы, на буфер поиска кратчайшей записи
	 *          и на возвращаемый объект строки. Показатель ловит регрессию, при
	 *          которой буфер поиска перевыделяется на каждом шаге деления отрезка
	 *          пополам: это давало шесть выделений вместо трёх
	 *
	 */
	static constexpr double PRINT_REAL_ALLOCATIONS_THRESHOLD = 4.0;

	/**
	 * @brief Функция прогона сценария формирования десятичной записи целого числа
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t printDecimal() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Строковое представление числа
		string text = "";
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(INTEGER_ROUNDS, [&]() noexcept {
			// Выполняем формирование десятичной записи числа
			text = first128().print();
		});
		// Накапливаем контрольную сумму прогона
		checksum() += text.size();
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария формирования шестнадцатеричной записи целого числа
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t printHex() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Строковое представление числа
		string text = "";
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(INTEGER_ROUNDS, [&]() noexcept {
			// Выполняем формирование шестнадцатеричной записи числа
			text = first128().print(awh::bignum::format_t::HEX);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += text.size();
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария разбора десятичной записи целого числа
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t parseDecimal() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись числа
		const string & text = decimal128();
		// Результат разбора записи числа
		awh::uint128_t number;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(INTEGER_ROUNDS, [&]() noexcept {
			// Выполняем разбор десятичной записи числа
			number.parse(text);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += number[0];
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария формирования записи вещественного числа
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t printReal() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Строковое представление числа
		string text = "";
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(REAL_ROUNDS, [&]() noexcept {
			// Выполняем формирование десятичной записи числа
			text = first64r().print();
		});
		// Накапливаем контрольную сумму прогона
		checksum() += text.size();
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария разбора записи вещественного числа
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t parseReal() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись числа
		const string & text = decimalReal();
		// Результат разбора записи числа
		awh::real64_t number;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(REAL_ROUNDS, [&]() noexcept {
			// Выполняем разбор десятичной записи числа
			number.parse(text);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += number[0];
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария учёта выделений памяти при формировании записи
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t printAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Строковое представление числа
		string text = "";
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(INTEGER_ROUNDS, [&]() noexcept {
			// Выполняем формирование десятичной записи числа
			text = first128().print();
		});
		// Накапливаем контрольную сумму прогона
		checksum() += text.size();
		// Вычисляем количество выделений памяти на одну операцию
		result.value = perOperation(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария учёта выделений памяти при разборе записи
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t parseAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись числа
		const string & text = decimal128();
		// Результат разбора записи числа
		awh::uint128_t number;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(INTEGER_ROUNDS, [&]() noexcept {
			// Выполняем разбор десятичной записи числа
			number.parse(text);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += number[0];
		// Вычисляем количество выделений памяти на одну операцию
		result.value = perOperation(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	/**
	 * @brief Функция прогона сценария учёта выделений памяти при формировании записи вещественного числа
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t printRealAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Строковое представление числа
		string text = "";
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(REAL_ROUNDS, [&]() noexcept {
			// Выполняем формирование десятичной записи числа
			text = first64r().print();
		});
		// Накапливаем контрольную сумму прогона
		checksum() += text.size();
		// Вычисляем количество выделений памяти на одну операцию
		result.value = perOperation(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	// Регистрируем сценарий формирования десятичной записи целого числа
	static const bool gPrintDec = awh::benchmark::add(
		"bignum/string/print-dec-128", "операций/с", PRINT_DEC_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::printDecimal
	);
	// Регистрируем сценарий формирования шестнадцатеричной записи целого числа
	static const bool gPrintHex = awh::benchmark::add(
		"bignum/string/print-hex-128", "операций/с", PRINT_HEX_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::printHex
	);
	// Регистрируем сценарий разбора десятичной записи целого числа
	static const bool gParseDec = awh::benchmark::add(
		"bignum/string/parse-dec-128", "операций/с", PARSE_DEC_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::parseDecimal
	);
	// Регистрируем сценарий формирования записи вещественного числа
	static const bool gPrintReal = awh::benchmark::add(
		"bignum/string/print-real-64", "операций/с", PRINT_REAL_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::printReal
	);
	// Регистрируем сценарий разбора записи вещественного числа
	static const bool gParseReal = awh::benchmark::add(
		"bignum/string/parse-real-64", "операций/с", PARSE_REAL_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::parseReal
	);
	// Регистрируем сценарий учёта выделений памяти при формировании записи
	static const bool gPrintAllocations = awh::benchmark::add(
		"bignum/string/allocations-per-print", "выделений", PRINT_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::printAllocations
	);
	// Регистрируем сценарий учёта выделений памяти при разборе записи
	static const bool gParseAllocations = awh::benchmark::add(
		"bignum/string/allocations-per-parse", "выделений", PARSE_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::parseAllocations
	);
	// Регистрируем сценарий учёта выделений памяти при формировании записи вещественного числа
	static const bool gPrintRealAllocations = awh::benchmark::add(
		"bignum/string/allocations-per-print-real", "выделений", PRINT_REAL_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::printRealAllocations
	);
};
