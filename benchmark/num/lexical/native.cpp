/**
 * @file native.cpp
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
 * @brief Сценарии сравнения со средствами стандартной библиотеки — разбор одной и той же
 *        записи модулем и функциями strtod и strtoll с измерением кратности превосходства
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstdlib>

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
 * @brief Внутренние параметры и сценарии бенчмарков сравнения со стандартной библиотекой
 *
 */
namespace {
	/**
	 * @brief Количество операций сценариев сравнения
	 *
	 */
	static constexpr size_t COMPARE_ROUNDS = 1000000;
	/**
	 * @brief Порог кратности превосходства над функцией strtod на быстром пути
	 *
	 * @details Сравнение ведётся в заведомо невыгодных для модуля условиях: модуль
	 *          является заголовочным и собирается вместе с бенчмарком, то есть без
	 *          флагов оптимизации, тогда как функция стандартной библиотеки берётся
	 *          из оптимизированной системной библиотеки. Кратность в такой сборке
	 *          меньше единицы, и порог откалиброван именно по ней с двукратным
	 *          запасом: он ловит регрессию разбора, а не проигрыш режима сборки.
	 *          Кратность, которую модуль даёт в оптимизированной сборке, приведена
	 *          в отчёте набора
	 *
	 */
	static constexpr double STRTOD_FAST_THRESHOLD = 0.1;
	/**
	 * @brief Порог кратности превосходства над функцией strtod на точном пути
	 *
	 * @details Показатель ловит регрессию точного пути: там, где приближения
	 *          недостаточно для однозначного округления, разбор уходит на сравнение
	 *          длинной арифметикой, и именно там сохранить преимущество перед
	 *          стандартной библиотекой труднее всего
	 *
	 */
	static constexpr double STRTOD_EXACT_THRESHOLD = 0.08;
	/**
	 * @brief Порог кратности превосходства над функцией strtoll
	 *
	 */
	static constexpr double STRTOLL_THRESHOLD = 0.2;

	/**
	 * @brief Функция прогона сценария сравнения разбора числа быстрым путём
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t compareStrtodFast() noexcept {
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
		// Выполняем прогон разбора средствами модуля
		const outcome_t module = measure(COMPARE_ROUNDS, [&]() noexcept {
			// Выполняем разбор записи числа средствами модуля
			awh::lexical_t::fromChars(first, last, value);
			// Накапливаем результат разбора
			accumulator += value;
			// Запрещаем вынос измеряемого разбора за пределы цикла
			barrier(&accumulator);
		});
		// Выполняем прогон разбора средствами стандартной библиотеки
		const outcome_t native = measure(COMPARE_ROUNDS, [&]() noexcept {
			// Выполняем разбор записи числа средствами стандартной библиотеки
			accumulator += ::strtod(first, nullptr);
			// Запрещаем вынос измеряемого разбора за пределы цикла
			barrier(&accumulator);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += static_cast <uint64_t> (accumulator != 0.0);
		// Устанавливаем измеренное значение
		result.value = speedup(module, native);
		// Устанавливаем сведения о прогоне
		result.details = comparison(module, native);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария сравнения разбора числа точным путём
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t compareStrtodExact() noexcept {
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
		// Выполняем прогон разбора средствами модуля
		const outcome_t module = measure(COMPARE_ROUNDS, [&]() noexcept {
			// Выполняем разбор записи числа средствами модуля
			awh::lexical_t::fromChars(first, last, value);
			// Накапливаем результат разбора
			accumulator += value;
			// Запрещаем вынос измеряемого разбора за пределы цикла
			barrier(&accumulator);
		});
		// Выполняем прогон разбора средствами стандартной библиотеки
		const outcome_t native = measure(COMPARE_ROUNDS, [&]() noexcept {
			// Выполняем разбор записи числа средствами стандартной библиотеки
			accumulator += ::strtod(first, nullptr);
			// Запрещаем вынос измеряемого разбора за пределы цикла
			barrier(&accumulator);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += static_cast <uint64_t> (accumulator != 0.0);
		// Устанавливаем измеренное значение
		result.value = speedup(module, native);
		// Устанавливаем сведения о прогоне
		result.details = comparison(module, native);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария сравнения разбора целого числа
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t compareStrtoll() noexcept {
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
		// Выполняем прогон разбора средствами модуля
		const outcome_t module = measure(COMPARE_ROUNDS, [&]() noexcept {
			// Выполняем разбор записи числа средствами модуля
			awh::lexical_t::fromChars(first, last, value);
			// Накапливаем результат разбора
			accumulator += static_cast <uint64_t> (value);
			// Запрещаем вынос измеряемого разбора за пределы цикла
			barrier(&accumulator);
		});
		// Выполняем прогон разбора средствами стандартной библиотеки
		const outcome_t native = measure(COMPARE_ROUNDS, [&]() noexcept {
			// Выполняем разбор записи числа средствами стандартной библиотеки
			accumulator += static_cast <uint64_t> (::strtoll(first, nullptr, 10));
			// Запрещаем вынос измеряемого разбора за пределы цикла
			barrier(&accumulator);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += accumulator;
		// Устанавливаем измеренное значение
		result.value = speedup(module, native);
		// Устанавливаем сведения о прогоне
		result.details = comparison(module, native);
		// Выводим результат измерения
		return result;
	}

	// Регистрируем сценарий сравнения разбора числа быстрым путём
	static const bool gCompareStrtodFast = awh::benchmark::add(
		"lexical/native/vs-strtod-fast", "раз", STRTOD_FAST_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::compareStrtodFast
	);
	// Регистрируем сценарий сравнения разбора числа точным путём
	static const bool gCompareStrtodExact = awh::benchmark::add(
		"lexical/native/vs-strtod-exact", "раз", STRTOD_EXACT_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::compareStrtodExact
	);
	// Регистрируем сценарий сравнения разбора целого числа
	static const bool gCompareStrtoll = awh::benchmark::add(
		"lexical/native/vs-strtoll", "раз", STRTOLL_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::compareStrtoll
	);
};
