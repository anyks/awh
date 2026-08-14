/**
 * @file main.hpp
 * @date 2025-12-07
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
 * @brief Общий заголовочный файл набора автоматических тестов —
 *        подключение Google Test и Google Mock и настройка глобальных параметров тестового окружения
 *
 * @copyright Copyright © 2025
 *
 */
 
#ifndef __AWH_TESTS__
#define __AWH_TESTS__

/**
 * Отключаем поддержку POSIX регулярных выражений в Google Test
 */
#define GTEST_HAS_POSIX_RE 0

#include <gtest/gtest.h>
#include <gmock/gmock.h>

/**
 * Стандартные модули
 */
#include <limits>
#include <cstring>
#include <cstdint>
#include <type_traits>

/**
 * @brief Метод определения сброса субнормальных значений платформой
 *
 * @details Некоторые системы запускают процессы с взведённым в сопроцессоре
 *          признаком «сброс субнормальных в ноль». Так поступает NetBSD на
 *          архитектуре aarch64, выставляя «FPCR.FZ=1» ещё до входа в «main».
 *          В этом режиме субнормальные значения обращаются в ноль как в
 *          арифметике, так и внутри «strtod», отчего сверка с ним на этом
 *          участке теряет смысл: эталон становится хуже проверяемого кода.
 *
 * @note Сложение выполняется через «volatile», чтобы его произвёл сопроцессор
 *       во время работы, а не оптимизатор при сборке
 *
 * @return признак сброса субнормальных значений платформой
 *
 */
static inline bool flushesSubnormals() noexcept {
	// Получаем два наименьших субнормальных значения
	volatile double first = std::numeric_limits <double>::denorm_min();
	volatile double second = std::numeric_limits <double>::denorm_min();
	// Платформа сбрасывает субнормальные значения, если их сумма обратилась в ноль
	return ((first + second) == .0);
}

/**
 * @brief Метод определения субнормального значения по его двоичной записи
 *
 * @details Значение считается субнормальным, когда поле порядка обнулено, а поле
 *          мантиссы — нет.
 *
 * @note Разбор ведётся по битам намеренно: «std::fpclassify» на платформе,
 *       сбрасывающей субнормальные значения, отвечает «FP_ZERO», так как
 *       сравнения внутри него выполняет тот же сопроцессор
 *
 * @tparam T вид дробного значения
 * @param value значение, вид которого требуется определить
 * @return      признак субнормального значения
 *
 */
template <typename T>
static inline bool subnormalBits(const T value) noexcept {
	// Проверяем, что значение является дробным
	static_assert(std::is_floating_point <T>::value, "subnormalBits ждёт дробное значение");
	// Если значение записано четырьмя октетами
	if constexpr(sizeof(T) == sizeof(uint32_t)) {
		// Извлекаем двоичную запись значения
		uint32_t bits = 0;
		// Выполняем копирование двоичной записи значения
		std::memcpy(&bits, &value, sizeof(bits));
		// Выводим признак обнулённого порядка при ненулевой мантиссе
		return (((bits & 0x7F800000u) == 0) && ((bits & 0x007FFFFFu) != 0));
	// Если значение записано восемью октетами
	} else if constexpr(sizeof(T) == sizeof(uint64_t)) {
		// Извлекаем двоичную запись значения
		uint64_t bits = 0;
		// Выполняем копирование двоичной записи значения
		std::memcpy(&bits, &value, sizeof(bits));
		// Выводим признак обнулённого порядка при ненулевой мантиссе
		return (((bits & 0x7FF0000000000000ull) == 0) && ((bits & 0x000FFFFFFFFFFFFFull) != 0));
	}
	// Прочие виды дробных значений разбору не подлежат
	return false;
}

#endif // __AWH_TESTS__
