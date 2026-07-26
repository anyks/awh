/**
 * @file: bignum.cpp
 * @date: 2026-07-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Общее окружение бенчмарков модуля длинных чисел — эталонные операнды
 *        разной разрядности, контрольная сумма прогонов и формирование сведений о замере
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>

/**
 * Подключаем заголовочный файл бенчмарков модуля длинных чисел
 */
#include "bignum.hpp"

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
string awh::benchmark::bignum::details(const outcome_t & output) noexcept {
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
 * @brief Функция извлечения количества операций в секунду
 *
 * @param output итоги прогона сценария
 * @return       количество операций в секунду
 *
 */
double awh::benchmark::bignum::perSecond(const outcome_t & output) noexcept {
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
double awh::benchmark::bignum::perOperation(const outcome_t & output) noexcept {
	// Если операции не выполнялись
	if(output.operations == 0)
		// Выводим нулевое количество выделений памяти
		return 0.0;
	// Выводим количество выделений памяти на одну операцию
	return (static_cast <double> (output.allocations) / static_cast <double> (output.operations));
}
/**
 * @brief Функция получения контрольной суммы прогонов
 *
 * @return ссылка на контрольную сумму прогонов
 *
 */
volatile uint64_t & awh::benchmark::bignum::checksum() noexcept {
	// Контрольная сумма прогонов
	static volatile uint64_t result = 0;
	// Выводим ссылку на контрольную сумму прогонов
	return result;
}
/**
 * @brief Функция получения эталонного 128-битного беззнакового операнда
 *
 * @return эталонный операнд
 *
 */
const awh::uint128_t & awh::benchmark::bignum::first128() noexcept {
	// Эталонный операнд со всеми значащими разрядами
	static const awh::uint128_t result("264868261418867372845748371426891277031");
	// Выводим эталонный операнд
	return result;
}
/**
 * @brief Функция получения второго эталонного 128-битного беззнакового операнда
 *
 * @return эталонный операнд
 *
 */
const awh::uint128_t & awh::benchmark::bignum::second128() noexcept {
	// Эталонный операнд со всеми значащими разрядами
	static const awh::uint128_t result("183074619283746192837461928374619283741");
	// Выводим эталонный операнд
	return result;
}
/**
 * @brief Функция получения эталонного 512-битного беззнакового операнда
 *
 * @return эталонный операнд
 *
 */
const awh::uint512_t & awh::benchmark::bignum::first512() noexcept {
	// Эталонный операнд со всеми значащими разрядами
	static const awh::uint512_t result = (awh::uint512_t(first128()).pow(4) | awh::uint512_t(1));
	// Выводим эталонный операнд
	return result;
}
/**
 * @brief Функция получения эталонного 2048-битного беззнакового операнда
 *
 * @return эталонный операнд
 *
 */
const awh::uint2048_t & awh::benchmark::bignum::first2048() noexcept {
	// Эталонный операнд со всеми значащими разрядами
	static const awh::uint2048_t result = (awh::uint2048_t(first128()).pow(16) | awh::uint2048_t(1));
	// Выводим эталонный операнд
	return result;
}
/**
 * @brief Функция получения второго эталонного 2048-битного беззнакового операнда
 *
 * @return эталонный операнд
 *
 */
const awh::uint2048_t & awh::benchmark::bignum::second2048() noexcept {
	// Эталонный операнд со всеми значащими разрядами
	static const awh::uint2048_t result = (awh::uint2048_t(second128()).pow(15) | awh::uint2048_t(1));
	// Выводим эталонный операнд
	return result;
}
/**
 * @brief Функция получения эталонного 64-битного вещественного операнда
 *
 * @return эталонный операнд
 *
 */
const awh::real64_t & awh::benchmark::bignum::first64r() noexcept {
	// Эталонный операнд с полностью заполненной мантиссой
	static const awh::real64_t result("3.14159265358979311599796346854418516159057617187500");
	// Выводим эталонный операнд
	return result;
}
/**
 * @brief Функция получения второго эталонного 64-битного вещественного операнда
 *
 * @return эталонный операнд
 *
 */
const awh::real64_t & awh::benchmark::bignum::second64r() noexcept {
	// Эталонный операнд с полностью заполненной мантиссой
	static const awh::real64_t result("2.71828182845904509079559829842764884233474731445312");
	// Выводим эталонный операнд
	return result;
}
/**
 * @brief Функция получения эталонного 128-битного вещественного операнда
 *
 * @return эталонный операнд
 *
 */
const awh::real128_t & awh::benchmark::bignum::first128r() noexcept {
	// Эталонный операнд с полностью заполненной мантиссой
	static const awh::real128_t result("3.14159265358979323846264338327950288419716939937511");
	// Выводим эталонный операнд
	return result;
}
/**
 * @brief Функция получения второго эталонного 128-битного вещественного операнда
 *
 * @return эталонный операнд
 *
 */
const awh::real128_t & awh::benchmark::bignum::second128r() noexcept {
	// Эталонный операнд с полностью заполненной мантиссой
	static const awh::real128_t result("2.71828182845904523536028747135266249775724709369996");
	// Выводим эталонный операнд
	return result;
}
/**
 * @brief Функция получения эталонной десятичной записи 128-битного целого числа
 *
 * @return эталонная десятичная запись
 *
 */
const string & awh::benchmark::bignum::decimal128() noexcept {
	// Эталонная десятичная запись числа
	static const string result = "264868261418867372845748371426891277031";
	// Выводим эталонную десятичную запись
	return result;
}
/**
 * @brief Функция получения эталонной десятичной записи вещественного числа
 *
 * @return эталонная десятичная запись
 *
 */
const string & awh::benchmark::bignum::decimalReal() noexcept {
	// Эталонная десятичная запись числа
	static const string result = "3.141592653589793";
	// Выводим эталонную десятичную запись
	return result;
}
