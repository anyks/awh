/**
 * @file formats.cpp
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
 * @brief Сценарии измерения форматов разбора — строгие правила RFC 8259, специальные значения,
 *        двухбайтовая кодировка исходной записи и прямая сборка числа из мантиссы и порядка
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
 * @brief Внутренние параметры и сценарии бенчмарков форматов разбора
 *
 */
namespace {
	/**
	 * @brief Количество операций сценариев разбора числовых записей
	 *
	 */
	static constexpr size_t PARSE_ROUNDS = 5000000;
	/**
	 * @brief Порог скорости разбора записи числа формата RFC 8259
	 *
	 * @details Пороги пропускной способности зависят от машины и режима сборки:
	 *          модуль является заголовочным и собирается флагами того, кто его
	 *          подключил, а бенчмарки собираются без флагов оптимизации и медленнее
	 *          оптимизированной сборки в несколько раз. Поэтому пороги откалиброваны
	 *          по неоптимизированной сборке с четырёхкратным запасом - они ловят
	 *          регрессии на порядок, а не колебания окружения
	 *
	 */
	static constexpr double PARSE_JSON_THRESHOLD = 2300000.0;
	/**
	 * @brief Порог скорости разбора записи специального значения
	 *
	 */
	static constexpr double PARSE_INFNAN_THRESHOLD = 5000000.0;
	/**
	 * @brief Порог скорости разбора записи числа в двухбайтовой кодировке
	 *
	 * @details Показатель ловит потерю векторного разбора блоков двухбайтовых цифр:
	 *          без него запись такой кодировки разбирается только поразрядно, тогда
	 *          как однобайтовая запись сохраняет блочный путь на целочисленной
	 *          арифметике и не замедляется вовсе
	 *
	 */
	static constexpr double PARSE_UTF16_THRESHOLD = 1400000.0;
	/**
	 * @brief Порог скорости сборки числа из мантиссы и показателя степени
	 *
	 * @details Сборка минует разбор строки целиком и показывает нижнюю границу
	 *          стоимости преобразования: всё, что сверх неё в сценариях разбора -
	 *          это цена работы с исходной записью
	 *
	 */
	static constexpr double TIMES_POW10_THRESHOLD = 8000000.0;

	/**
	 * @brief Функция прогона сценария разбора записи числа формата RFC 8259
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t parseJson() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись числа
		const string & text = realJson();
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
			// Выполняем разбор записи числа по строгим правилам RFC 8259
			awh::lexical_t::fromChars(first, last, value, awh::lexical::format_t::JSON);
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
	 * @brief Функция прогона сценария разбора записи специального значения
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t parseInfNan() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись числа
		const string & text = realInfinity();
		// Начало разбираемой записи числа
		const char * first = text.c_str();
		// Конец разбираемой записи числа
		const char * last = (first + text.size());
		// Накопитель признаков разбора
		uint64_t accumulator = 0;
		// Результат разбора записи числа
		double value = 0.0;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(PARSE_ROUNDS, [&]() noexcept {
			// Выполняем разбор записи специального значения
			const auto answer = awh::lexical_t::fromChars(first, last, value);
			// Накапливаем признак успешности разбора
			accumulator += static_cast <uint64_t> (static_cast <bool> (answer));
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
	 * @brief Функция прогона сценария разбора записи числа в двухбайтовой кодировке
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t parseUtf16() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемая запись числа
		const u16string & text = realUtf16();
		// Начало разбираемой записи числа
		const char16_t * first = text.c_str();
		// Конец разбираемой записи числа
		const char16_t * last = (first + text.size());
		// Накопитель результатов разбора
		double accumulator = 0.0;
		// Результат разбора записи числа
		double value = 0.0;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(PARSE_ROUNDS, [&]() noexcept {
			// Выполняем разбор записи числа в двухбайтовой кодировке
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
	 * @brief Функция прогона сценария сборки числа из мантиссы и показателя степени
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t timesPow10() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Значение мантиссы собираемого числа
		const uint64_t mantissa = 1234567890123456789ULL;
		// Показатель степени собираемого числа
		const int32_t exponent = -17;
		// Накопитель результатов сборки
		double accumulator = 0.0;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(PARSE_ROUNDS, [&]() noexcept {
			// Выполняем сборку числа из мантиссы и показателя степени
			accumulator += awh::lexical_t::integerTimesPow10 <double> (mantissa, exponent);
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

	// Регистрируем сценарий разбора записи числа формата RFC 8259
	static const bool gParseJson = awh::benchmark::add(
		"lexical/format/parse-json", "операций/с", PARSE_JSON_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::parseJson
	);
	// Регистрируем сценарий разбора записи специального значения
	static const bool gParseInfNan = awh::benchmark::add(
		"lexical/format/parse-infnan", "операций/с", PARSE_INFNAN_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::parseInfNan
	);
	// Регистрируем сценарий разбора записи числа в двухбайтовой кодировке
	static const bool gParseUtf16 = awh::benchmark::add(
		"lexical/format/parse-utf16", "операций/с", PARSE_UTF16_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::parseUtf16
	);
	// Регистрируем сценарий сборки числа из мантиссы и показателя степени
	static const bool gTimesPow10 = awh::benchmark::add(
		"lexical/format/integer-times-pow10", "операций/с", TIMES_POW10_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::timesPow10
	);
};
