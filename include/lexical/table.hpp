/**
 * @file: table.hpp
 * @date: 2026-07-22
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл таблиц степеней модуля разбора чисел —
 *        объявления предвычисленных 128-битных таблиц степеней пятёрки и границ показателей,
 *        обеспечивающих корректное округление при быстром пути разбора
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_LEXICAL_TABLE__
#define __AWH_LEXICAL_TABLE__

/**
 * Стандартные заголовочные файлы
 */
#include <cstdint>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Пространство имён модуля разбора чисел
	 *
	 */
	namespace lexical {
		/**
		 * @brief Пространство имён таблиц степеней
		 *
		 */
		namespace powers {
			/**
			 * @brief Наибольший показатель степени пятёрки в таблице
			 *
			 */
			constexpr int32_t LARGEST_POWER = binary_t <double>::largestPowerOfTen();

			/**
			 * @brief Наименьший показатель степени пятёрки в таблице
			 *
			 */
			constexpr int32_t SMALLEST_POWER = binary_t <double>::smallestPowerOfTen();

			/**
			 * @brief Количество 64-битных слов таблицы степеней пятёрки
			 *
			 * @details Каждая степень хранится парой слов: старшим и младшим.
			 *
			 */
			constexpr int32_t ENTRIES = (2 * (LARGEST_POWER - SMALLEST_POWER + 1));

			/**
			 * @brief Таблица степеней пятёрки в виде пар 64-битных слов
			 *
			 * @details Определение таблицы размещено в файле src/lexical/table.cpp,
			 *          что даёт одну копию данных на всю программу.
			 *
			 */
			extern const uint64_t POWER_OF_FIVE[ENTRIES];

			/**
			 * @brief Метод проверки представимости показателя степени таблицей
			 *
			 * @param power проверяемый показатель степени десяти
			 * @return      результат проверки
			 *
			 */
			AWH_LEXICAL_INLINE constexpr bool isSupported(const int64_t power) noexcept {
				// Выполняем проверку попадания показателя степени в диапазон таблицы
				return ((power >= static_cast <int64_t> (SMALLEST_POWER)) && (power <= static_cast <int64_t> (LARGEST_POWER)));
			}
		};
	};
};

#endif // __AWH_LEXICAL_TABLE__
