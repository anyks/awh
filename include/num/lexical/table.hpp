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
 * \~russian
 * @brief Заголовочный файл таблиц степеней модуля разбора чисел —
 *        объявления предвычисленных 128-битных таблиц степеней пятёрки и границ показателей,
 *        обеспечивающих корректное округление при быстром пути разбора
 *
 * \~english
 * @brief Header file of the power tables of the number parsing module —
 *        the declarations of the precomputed 128-bit tables of the powers of five and of the exponent bounds,
 *        which ensure correct rounding on the fast parsing path
 *
 * \~
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
 * \~russian
 * @brief Основное пространство имён
 *
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён модуля разбора чисел
	 *
	 * \~english
	 * @brief Namespace of the number parsing module
	 *
	 * \~
	 */
	namespace lexical {
		/**
		 * \~russian
		 * @brief Пространство имён таблиц степеней
		 *
		 * \~english
		 * @brief Namespace of the power tables
		 *
		 * \~
		 */
		namespace powers {
			/**
			 * \~russian
			 * @brief Наибольший показатель степени пятёрки в таблице
			 *
			 * \~english
			 * @brief Largest exponent of a power of five in the table
			 *
			 * \~
			 */
			constexpr int32_t LARGEST_POWER = binary_t <double>::largestPowerOfTen();

			/**
			 * \~russian
			 * @brief Наименьший показатель степени пятёрки в таблице
			 *
			 * \~english
			 * @brief Smallest exponent of a power of five in the table
			 *
			 * \~
			 */
			constexpr int32_t SMALLEST_POWER = binary_t <double>::smallestPowerOfTen();

			/**
			 * \~russian
			 * @brief Количество 64-битных слов таблицы степеней пятёрки
			 *
			 * @details Каждая степень хранится парой слов: старшим и младшим.
			 *
			 * \~english
			 * @brief Number of 64-bit words of the table of the powers of five
			 * @details Every power is kept as a pair of words: the high one and the low one.
			 *
			 * \~
			 */
			constexpr int32_t ENTRIES = (2 * (LARGEST_POWER - SMALLEST_POWER + 1));

			/**
			 * \~russian
			 * @brief Таблица степеней пятёрки в виде пар 64-битных слов
			 *
			 * @details Определение таблицы размещено в файле src/num/lexical/table.cpp,
			 *          что даёт одну копию данных на всю программу.
			 *
			 * \~english
			 * @brief Table of the powers of five as pairs of 64-bit words
			 * @details The definition of the table is placed in the src/num/lexical/table.cpp file,
			 *          which gives one copy of the data for the whole program.
			 *
			 * \~
			 */
			extern const uint64_t POWER_OF_FIVE[ENTRIES];

			/**
			 * \~russian
			 * @brief Метод проверки представимости показателя степени таблицей
			 *
			 * @param power проверяемый показатель степени десяти
			 * @return      результат проверки
			 *
			 * \~english
			 * @brief Method of checking whether an exponent is representable by the table
			 * @param power exponent of a power of ten to check
			 * @return      result of the check
			 *
			 * \~
			 */
			AWH_ASCII_INLINE constexpr bool isSupported(const int64_t power) noexcept {
				// Выполняем проверку попадания показателя степени в диапазон таблицы
				return ((power >= static_cast <int64_t> (SMALLEST_POWER)) && (power <= static_cast <int64_t> (LARGEST_POWER)));
			}
		};
	};
};

#endif // __AWH_LEXICAL_TABLE__
