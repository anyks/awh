/**
 * @file link.cpp
 * @date 2026-08-22
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
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл
 */
#include <alloc/link.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <chrono>

/**
 * Зерно перемешивания указателей
 *
 * Нуль до посева намеренно: до заведения центральных списков связан не бывает ни
 * один блок, а значит и прочитан старым зерном быть не может
 */
uintptr_t awh::alloc::Link::_cookie = 0;

/**
 * @brief Пространство имён вспомогательных средств
 *
 */
namespace {
	/**
	 * @brief Метод перемешивания разрядов числа
	 *
	 * @note Взят у splitmix64: рассеивает единичный разряд по всему числу
	 *
	 * @param value перемешиваемое число
	 * @return      перемешанное число
	 *
	 */
	static uint64_t stir(uint64_t value) noexcept {
		// Перемешиваем разряды числа
		value += 0x9E3779B97F4A7C15ULL;
		value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ULL;
		value = (value ^ (value >> 27)) * 0x94D049BB133111EBULL;
		// Выводим перемешанное число
		return (value ^ (value >> 31));
	}
};

/**
 * @brief Метод посева зерна перемешивания
 *
 */
void awh::alloc::Link::seed() noexcept {
	// Если зерно уже посеяно
	if(Link::_cookie != 0)
		// Выходим из функции
		return;
	// Место переменной стека
	uint64_t anchor = 0;
	/**
	 * Собираем зерно из того, что не требует выделения памяти
	 *
	 * Случайных чисел здесь взять неоткуда: всякий их источник волен обратиться к
	 * выделению памяти, а зовут нас из-под перехваченного malloc. Оттого зерно
	 * собирается из разброса адресов, какой даёт сама система, и показаний часов
	 */
	uint64_t result = ::stir(static_cast <uint64_t> (reinterpret_cast <uintptr_t> (&anchor)));
	// Примешиваем место кода самого распределителя
	result ^= ::stir(static_cast <uint64_t> (reinterpret_cast <uintptr_t> (&Link::_cookie)));
	// Примешиваем показания часов
	result ^= ::stir(static_cast <uint64_t> (std::chrono::steady_clock::now().time_since_epoch().count()));
	// Перемешиваем собранное ещё раз
	result = ::stir(result);
	// Закрепляем зерно, не допуская нуля
	Link::_cookie = static_cast <uintptr_t> (result | 1);
}
