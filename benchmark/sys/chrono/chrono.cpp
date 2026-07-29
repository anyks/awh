/**
 * @file: chrono.cpp
 * @date: 2026-07-29
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Общее окружение бенчмарков модуля работы с датой и временем — объекты фреймворка
 *        и логирования, контрольная сумма прогонов и формирование сведений о замере
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>

/**
 * Подключаем заголовочный файл бенчмарков модуля работы с датой и временем
 */
#include "chrono.hpp"

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
string awh::benchmark::chrono::details(const outcome_t & output) noexcept {
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
double awh::benchmark::chrono::perSecond(const outcome_t & output) noexcept {
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
double awh::benchmark::chrono::perOperation(const outcome_t & output) noexcept {
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
volatile uint64_t & awh::benchmark::chrono::checksum() noexcept {
	// Контрольная сумма прогонов
	static volatile uint64_t result = 0;
	// Выводим ссылку на контрольную сумму прогонов
	return result;
}
/**
 * @brief Функция получения объекта фреймворка сценариев
 *
 * @return объект фреймворка
 *
 */
const awh::fmk_t * awh::benchmark::chrono::framework() noexcept {
	// Объект фреймворка сценариев
	static awh::fmk_t result;
	// Выводим объект фреймворка
	return &result;
}
/**
 * @brief Функция получения объекта логирования сценариев
 *
 * @return объект логирования
 *
 */
const awh::log_t * awh::benchmark::chrono::logger() noexcept {
	// Объект логирования сценариев
	static awh::log_t result(framework());
	// Отключаем логирование на время прогона сценариев
	const_cast <awh::log_t &> (result).level(awh::log_t::level_t::NONE);
	// Выводим объект логирования
	return &result;
}
