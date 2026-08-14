/**
 * @file parser.cpp
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
 * @brief Общее окружение бенчмарков модуля разбора чисел — эталонные числовые записи
 *        всех путей разбора, контрольная сумма прогонов и формирование сведений о замере
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>

/**
 * Подключаем заголовочный файл бенчмарков модуля разбора чисел
 */
#include "parser.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Функция формирования сведений о прогоне сценария
 *
 * @param output итоги прогона сценария
 * @return       сведения о прогоне для вывода
 *
 */
string awh::benchmark::parser::details(const outcome_t & output) noexcept {
	// Буфер формирования сведений о прогоне
	char buffer[192];
	// Вычисляем среднее время выполнения одной операции в наносекундах
	const double nanoseconds = ((output.operations > 0)
	 ? ((output.seconds * 1e9) / static_cast <double> (output.operations)) : 0.0);
	// Выполняем формирование сведений о прогоне
	::snprintf(
		buffer, sizeof(buffer),
		"операций: %zu, время: %.3f с, на операцию: %.1f нс, выделений: %zu (%zu октетов)",
		output.operations, output.seconds, nanoseconds, output.allocations, output.allocated
	);
	// Выводим сведения о прогоне
	return string(buffer);
}
/**
 * @brief Функция формирования сведений о прогоне сценария сравнения
 *
 * @param module итоги прогона средствами модуля
 * @param native итоги прогона средствами стандартной библиотеки
 * @return       сведения о прогоне для вывода
 *
 */
string awh::benchmark::parser::comparison(const outcome_t & module, const outcome_t & native) noexcept {
	// Буфер формирования сведений о прогоне
	char buffer[192];
	// Вычисляем среднее время одной операции модуля в наносекундах
	const double first = ((module.operations > 0)
	 ? ((module.seconds * 1e9) / static_cast <double> (module.operations)) : 0.0);
	// Вычисляем среднее время одной операции стандартной библиотеки в наносекундах
	const double second = ((native.operations > 0)
	 ? ((native.seconds * 1e9) / static_cast <double> (native.operations)) : 0.0);
	// Выполняем формирование сведений о прогоне
	::snprintf(
		buffer, sizeof(buffer),
		"операций: %zu, модуль: %.1f нс, стандартная библиотека: %.1f нс",
		module.operations, first, second
	);
	// Выводим сведения о прогоне
	return string(buffer);
}
/**
 * @brief Функция извлечения количества операций в секунду
 *
 * @param output итоги прогона сценария
 * @return       количество операций в секунду
 *
 */
double awh::benchmark::parser::perSecond(const outcome_t & output) noexcept {
	// Если время прогона не измерено
	if(output.seconds <= 0.0)
		// Выводим нулевое количество операций в секунду
		return 0.0;
	// Выводим количество операций в секунду
	return (static_cast <double> (output.operations) / output.seconds);
}
/**
 * @brief Функция извлечения количества выделений памяти на одну операцию
 *
 * @param output итоги прогона сценария
 * @return       количество выделений памяти на одну операцию
 *
 */
double awh::benchmark::parser::perOperation(const outcome_t & output) noexcept {
	// Если операции не выполнялись
	if(output.operations == 0)
		// Выводим нулевое количество выделений памяти
		return 0.0;
	// Выводим количество выделений памяти на одну операцию
	return (static_cast <double> (output.allocations) / static_cast <double> (output.operations));
}
/**
 * @brief Функция извлечения кратности превосходства над стандартной библиотекой
 *
 * @param module итоги прогона средствами модуля
 * @param native итоги прогона средствами стандартной библиотеки
 * @return       кратность превосходства
 *
 */
double awh::benchmark::parser::speedup(const outcome_t & module, const outcome_t & native) noexcept {
	// Получаем скорость разбора средствами модуля
	const double first = perSecond(module);
	// Получаем скорость разбора средствами стандартной библиотеки
	const double second = perSecond(native);
	// Если скорость стандартной библиотеки не измерена
	if(second <= 0.0)
		// Выводим нулевую кратность превосходства
		return 0.0;
	// Выводим кратность превосходства модуля
	return (first / second);
}
/**
 * @brief Функция получения контрольной суммы прогонов
 *
 * @return ссылка на контрольную сумму прогонов
 *
 */
volatile uint64_t & awh::benchmark::parser::checksum() noexcept {
	// Контрольная сумма прогонов
	static volatile uint64_t result = 0;
	// Выводим ссылку на контрольную сумму прогонов
	return result;
}
/**
 * @brief Функция получения эталонной десятичной записи 32-битного целого числа
 *
 * @return эталонная запись числа
 *
 */
const string & awh::benchmark::parser::integer32() noexcept {
	// Эталонная запись числа
	static const string result = "1234567890";
	// Выводим эталонную запись числа
	return result;
}
/**
 * @brief Функция получения эталонной десятичной записи 64-битного целого числа
 *
 * @return эталонная запись числа
 *
 */
const string & awh::benchmark::parser::integer64() noexcept {
	// Эталонная запись числа из девятнадцати цифр
	static const string result = "9223372036854775807";
	// Выводим эталонную запись числа
	return result;
}
/**
 * @brief Функция получения эталонной десятичной записи с контролем переполнения
 *
 * @return эталонная запись числа
 *
 */
const string & awh::benchmark::parser::integerChecked() noexcept {
	// Эталонная запись числа предельной для десятичной записи длины
	static const string result = "18446744073709551615";
	// Выводим эталонную запись числа
	return result;
}
/**
 * @brief Функция получения эталонной шестнадцатеричной записи целого числа
 *
 * @return эталонная запись числа
 *
 */
const string & awh::benchmark::parser::integerHex() noexcept {
	// Эталонная запись числа
	static const string result = "7FFFFFFFFFFFFFFF";
	// Выводим эталонную запись числа
	return result;
}
/**
 * @brief Функция получения эталонной записи целого числа по основанию 36
 *
 * @return эталонная запись числа
 *
 */
const string & awh::benchmark::parser::integerBase36() noexcept {
	// Эталонная запись числа
	static const string result = "1Y2P0IJ32E8E7";
	// Выводим эталонную запись числа
	return result;
}
/**
 * @brief Функция получения эталонной записи целого числа с переполнением разрядности
 *
 * @return эталонная запись числа
 *
 */
const string & awh::benchmark::parser::integerOverflow() noexcept {
	// Эталонная запись числа за пределами разрядности
	static const string result = "99999999999999999999999999";
	// Выводим эталонную запись числа
	return result;
}
/**
 * @brief Функция получения эталонной записи числа быстрого пути разбора
 *
 * @return эталонная запись числа
 *
 */
const string & awh::benchmark::parser::realFast() noexcept {
	// Эталонная запись числа
	static const string result = "3.14159265358979";
	// Выводим эталонную запись числа
	return result;
}
/**
 * @brief Функция получения эталонной записи числа научной записи
 *
 * @return эталонная запись числа
 *
 */
const string & awh::benchmark::parser::realScientific() noexcept {
	// Эталонная запись числа
	static const string result = "1.7976931348623157e308";
	// Выводим эталонную запись числа
	return result;
}
/**
 * @brief Функция получения эталонной записи числа точного пути разбора
 *
 * @return эталонная запись числа
 *
 */
const string & awh::benchmark::parser::realExact() noexcept {
	// Эталонная запись числа, требующая точного сравнения длинной арифметикой
	static const string result = "8.079366546375798353879909e-14";
	// Выводим эталонную запись числа
	return result;
}
/**
 * @brief Функция получения эталонной записи числа с усечённой мантиссой
 *
 * @return эталонная запись числа
 *
 */
const string & awh::benchmark::parser::realLong() noexcept {
	// Эталонная запись числа с шестьюдесятью четырьмя значащими цифрами
	static const string result = "1.234567890123456789012345678901234567890123456789012345678901234e-20";
	// Выводим эталонную запись числа
	return result;
}
/**
 * @brief Функция получения эталонной записи числа формата RFC 8259
 *
 * @return эталонная запись числа
 *
 */
const string & awh::benchmark::parser::realJson() noexcept {
	// Эталонная запись числа
	static const string result = "-1.2345678901234567e-7";
	// Выводим эталонную запись числа
	return result;
}
/**
 * @brief Функция получения эталонной записи бесконечности
 *
 * @return эталонная запись числа
 *
 */
const string & awh::benchmark::parser::realInfinity() noexcept {
	// Эталонная запись числа
	static const string result = "-Infinity";
	// Выводим эталонную запись числа
	return result;
}
/**
 * @brief Функция получения эталонной записи числа в двухбайтовой кодировке
 *
 * @return эталонная запись числа
 *
 */
const u16string & awh::benchmark::parser::realUtf16() noexcept {
	// Эталонная запись числа
	static const u16string result = u"1.7976931348623157e308";
	// Выводим эталонную запись числа
	return result;
}
