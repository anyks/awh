/**
 * @file hash.cpp
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
 * @brief Общее окружение бенчмарков модуля хэширования — эталонный буфер данных,
 *        контрольная сумма прогонов и формирование сведений о замере
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>

/**
 * Подключаем заголовочный файл бенчмарков модуля хэширования
 */
#include "hash.hpp"

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
string awh::benchmark::hash::details(const outcome_t & output) noexcept {
	// Буфер формирования сведений о прогоне
	char buffer[224];
	// Вычисляем среднее время выполнения одной операции в наносекундах
	const double nanoseconds = ((output.operations > 0)
	 ? ((output.seconds * 1e9) / static_cast <double> (output.operations)) : 0.0);
	// Вычисляем пропускную способность хэширования в гибиоктетах в секунду
	const double bandwidth = (perBytes(output) / 1073741824.0);
	// Выполняем формирование сведений о прогоне
	::snprintf(
		buffer, sizeof(buffer),
		"операций: %zu, данных: %zu октетов, время: %.3f с, на операцию: %.1f нс, "
		"пропускная способность: %.2f ГиБ/с, выделений: %zu (%zu октетов)",
		output.operations, output.size, output.seconds, nanoseconds,
		bandwidth, output.allocations, output.allocated
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
double awh::benchmark::hash::perSecond(const outcome_t & output) noexcept {
	// Если время прогона не измерено
	if(output.seconds <= 0.0)
		// Выводим нулевое количество операций в секунду
		return 0.0;
	// Выводим количество операций в секунду
	return (static_cast <double> (output.operations) / output.seconds);
}
/**
 * @brief Функция извлечения пропускной способности хэширования
 *
 * @param output итоги прогона сценария
 * @return       количество обработанных октетов в секунду
 *
 */
double awh::benchmark::hash::perBytes(const outcome_t & output) noexcept {
	// Если время прогона не измерено
	if(output.seconds <= 0.0)
		// Выводим нулевую пропускную способность
		return 0.0;
	// Выводим количество обработанных октетов в секунду
	return ((static_cast <double> (output.operations) * static_cast <double> (output.size)) / output.seconds);
}
/**
 * @brief Функция извлечения количества выделений памяти на одну операцию
 *
 * @param output итоги прогона сценария
 * @return       количество выделений памяти на одну операцию
 *
 */
double awh::benchmark::hash::perOperation(const outcome_t & output) noexcept {
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
volatile uint64_t & awh::benchmark::hash::checksum() noexcept {
	// Контрольная сумма прогонов
	static volatile uint64_t result = 0;
	// Выводим ссылку на контрольную сумму прогонов
	return result;
}
/**
 * @brief Функция получения эталонного буфера данных
 *
 * @return эталонный буфер данных
 *
 */
const vector <uint8_t> & awh::benchmark::hash::buffer() noexcept {
	/**
	 * @brief Функция формирования эталонного буфера данных
	 *
	 * @return эталонный буфер данных
	 *
	 */
	static const vector <uint8_t> result = []() noexcept -> vector <uint8_t> {
		// Эталонный буфер данных
		vector <uint8_t> data(LARGE_SIZE, 0);
		/**
		 * Выполняем заполнение эталонного буфера данных значащими октетами
		 */
		for(size_t i = 0; i < data.size(); i++)
			// Заполняем очередной октет эталонного буфера данных
			data[i] = static_cast <uint8_t> ((i * 131) ^ (i >> 3));
		// Выводим эталонный буфер данных
		return data;
	}();
	// Выводим эталонный буфер данных
	return result;
}
/**
 * @brief Функция получения эталонного объекта хэширования
 *
 * @return эталонный объект хэширования
 *
 */
const awh::hash_t & awh::benchmark::hash::engine() noexcept {
	// Эталонный объект хэширования
	static const awh::hash_t result;
	// Выводим эталонный объект хэширования
	return result;
}
