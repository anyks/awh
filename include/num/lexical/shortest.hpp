/**
 * @file shortest.hpp
 * @date 2026-08-15
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
 * \~russian
 * @brief Заголовочный файл формирования кратчайшей записи числа с плавающей точкой —
 *        выбор десятичного представления с наименьшим количеством значащих цифр,
 *        читаемого обратно тем же самым числом вплоть до последнего бита мантиссы
 *
 * \~english
 * @brief Header file of the forming of the shortest record of a floating-point number —
 *        the choice of the decimal representation with the least number of significant digits,
 *        which is read back as the very same number down to the last bit of the mantissa
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_LEXICAL_SHORTEST__
#define __AWH_LEXICAL_SHORTEST__

/**
 * Стандартные заголовочные файлы
 */
#include <cstdint>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "table.hpp"
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
		 * @brief Структура десятичного представления числа с плавающей точкой
		 *
		 * @details Значение числа равно mantissa * 10^exponent, где мантисса записана
		 *          наименьшим количеством значащих цифр, при котором запись читается
		 *          обратно тем же самым числом.
		 *
		 * \~english
		 * @brief Structure of the decimal representation of a floating-point number
		 * @details The value of the number equals mantissa * 10^exponent, where the mantissa is written
		 *          with the least number of significant digits at which the record is read
		 *          back as the very same number.
		 *
		 * \~
		 */
		struct decimal_t {
			// Мантисса десятичного представления числа
			uint64_t mantissa;
			// Показатель степени десяти десятичного представления числа
			int32_t exponent;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param mantissa мантисса десятичного представления числа
			 * @param exponent показатель степени десяти десятичного представления числа
			 *
			 * \~english
			 * @brief Constructor
			 * @param mantissa mantissa of the decimal representation of the number
			 * @param exponent exponent of the power of ten of the decimal representation of the number
			 *
			 * \~
			 */
			constexpr decimal_t(const uint64_t mantissa = 0, const int32_t exponent = 0) noexcept :
			 mantissa(mantissa), exponent(exponent) {}
		};

		/**
		 * \~russian
		 * @brief Метод извлечения целой части двоичного логарифма степени двойки по основанию десять
		 *
		 * @details Приближение задано целочисленной дробью и точно совпадает с истинным
		 *          значением на всём диапазоне показателей, встречающихся при записи чисел.
		 *
		 * @param exponent показатель степени двойки в диапазоне от нуля до 1650
		 * @return         целая часть логарифма
		 *
		 * \~english
		 * @brief Method of extracting the integer part of the base ten logarithm of a power of two
		 * @details The approximation is set by an integer fraction and exactly matches the true
		 *          value over the whole range of the exponents met while writing numbers.
		 * @param exponent exponent of the power of two in the range from zero to 1650
		 * @return         integer part of the logarithm
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr int32_t log10PowerOfTwo(const int32_t exponent) noexcept {
			// Выводим целую часть логарифма степени двойки по основанию десять
			return static_cast <int32_t> ((static_cast <uint32_t> (exponent) * 78913u) >> 18);
		}

		/**
		 * \~russian
		 * @brief Метод извлечения целой части логарифма степени пятёрки по основанию десять
		 *
		 * @param exponent показатель степени пятёрки в диапазоне от нуля до 2620
		 * @return         целая часть логарифма
		 *
		 * \~english
		 * @brief Method of extracting the integer part of the base ten logarithm of a power of five
		 * @param exponent exponent of the power of five in the range from zero to 2620
		 * @return         integer part of the logarithm
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr int32_t log10PowerOfFive(const int32_t exponent) noexcept {
			// Выводим целую часть логарифма степени пятёрки по основанию десять
			return static_cast <int32_t> ((static_cast <uint32_t> (exponent) * 732923u) >> 20);
		}

		/**
		 * \~russian
		 * @brief Метод извлечения количества двоичных разрядов степени пятёрки
		 *
		 * @param exponent показатель степени пятёрки в диапазоне от нуля до 3528
		 * @return         количество двоичных разрядов
		 *
		 * \~english
		 * @brief Method of extracting the number of the binary digits of a power of five
		 * @param exponent exponent of the power of five in the range from zero to 3528
		 * @return         number of the binary digits
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr int32_t bitsPowerOfFive(const int32_t exponent) noexcept {
			// Выводим количество двоичных разрядов степени пятёрки
			return (static_cast <int32_t> ((static_cast <uint32_t> (exponent) * 1217359u) >> 19) + 1);
		}

		/**
		 * \~russian
		 * @brief Метод извлечения кратности значения степени пятёрки
		 *
		 * @param value    проверяемое значение
		 * @param exponent показатель степени пятёрки
		 * @return         результат проверки
		 *
		 * \~english
		 * @brief Method of extracting the divisibility of a value by a power of five
		 * @param value    checked value
		 * @param exponent exponent of the power of five
		 * @return         result of the check
		 *
		 * \~
		 */
		AWH_ASCII_INLINE bool multipleOfPowerOfFive(const uint64_t value, const int32_t exponent) noexcept {
			// Количество множителей пятёрки в проверяемом значении
			int32_t count = 0;
			// Обрабатываемое значение
			uint64_t rest = value;
			/**
			 * Выполняем подсчёт множителей пятёрки в проверяемом значении
			 */
			while((rest % 5ull) == 0ull){
				// Выполняем деление обрабатываемого значения на пятёрку
				rest /= 5ull;
				// Увеличиваем количество найденных множителей пятёрки
				count++;
			}
			// Выводим результат проверки кратности значения степени пятёрки
			return (count >= exponent);
		}

		/**
		 * \~russian
		 * @brief Метод извлечения кратности значения степени двойки
		 *
		 * @param value    проверяемое значение
		 * @param exponent показатель степени двойки меньший разрядности значения
		 * @return         результат проверки
		 *
		 * \~english
		 * @brief Method of extracting the divisibility of a value by a power of two
		 * @param value    checked value
		 * @param exponent exponent of the power of two smaller than the bit width of the value
		 * @return         result of the check
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr bool multipleOfPowerOfTwo(const uint64_t value, const int32_t exponent) noexcept {
			// Выводим результат проверки кратности значения степени двойки
			return ((value & ((1ull << exponent) - 1ull)) == 0ull);
		}

		/**
		 * \~russian
		 * @brief Метод умножения значения на степень пятёрки со сдвигом
		 *
		 * @details Младшие разряды произведения на младшее слово множителя на результат
		 *          не влияют и потому не вычисляются: множимое не превосходит
		 *          пятидесяти шести разрядов, а сдвиг составляет не менее ста
		 *          восемнадцати разрядов, отчего отбрасываемая часть заведомо меньше
		 *          единицы младшего разряда результата.
		 *
		 * @param value значение для умножения
		 * @param high  старшее слово множителя
		 * @param low   младшее слово множителя
		 * @param shift количество разрядов сдвига в диапазоне от 64 до 127
		 * @return      результат умножения со сдвигом
		 *
		 * \~english
		 * @brief Method of multiplying a value by a power of five with a shift
		 * @details The low digits of the product by the low word of the multiplier do not influence
		 *          the result and therefore are not computed: the multiplicand does not exceed
		 *          fifty six digits, while the shift makes up no less than one hundred
		 *          and eighteen digits, from which the discarded part is knowingly smaller
		 *          than the unit of the least digit of the result.
		 * @param value value to multiply
		 * @param high  high word of the multiplier
		 * @param low   low word of the multiplier
		 * @param shift number of the digits of the shift in the range from 64 to 127
		 * @return      result of the multiplication with the shift
		 *
		 * \~
		 */
		AWH_ASCII_INLINE uint64_t multiplyShift(const uint64_t value, const uint64_t high, const uint64_t low, const int32_t shift) noexcept {
			// Выполняем умножение значения на младшее слово множителя
			const value128_t product = multiply128(value, low);
			// Выполняем умножение значения на старшее слово множителя
			const value128_t result = multiply128(value, high);
			// Формируем младшее слово суммы произведений
			const uint64_t sum = (result.low + product.high);
			// Формируем старшее слово суммы произведений с учётом переноса
			const uint64_t carry = (result.high + static_cast <uint64_t> (sum < result.low));
			// Определяем количество разрядов сдвига суммы произведений
			const int32_t count = (shift - 64);
			// Выводим результат умножения со сдвигом
			return ((sum >> count) | (carry << (64 - count)));
		}

		/**
		 * \~russian
		 * @brief Шаблон типа числа с плавающей точкой
		 *
		 * @tparam T тип числа с плавающей точкой
		 *
		 * \~english
		 * @brief Template of the floating-point type
		 * @tparam T floating-point type
		 *
		 * \~
		 */
		template <typename T>
		/**
		 * \~russian
		 * @brief Метод формирования кратчайшего десятичного представления числа
		 *
		 * @details Метод принимает разобранное на части двоичное представление конечного
		 *          числа, отличного от нуля, и выводит десятичное представление с
		 *          наименьшим количеством значащих цифр из числа тех, что читаются
		 *          обратно этим же самым числом. Порядок выбирается из промежутка
		 *          значений, округляемых к исходному числу, отчего запись обратима
		 *          при любом способе разбора, соблюдающем округление к ближайшему.
		 *
		 * @param mantissa значение поля мантиссы двоичного представления
		 * @param exponent значение поля порядка двоичного представления
		 * @return         кратчайшее десятичное представление числа
		 *
		 * \~english
		 * @brief Method of forming the shortest decimal representation of a number
		 * @details The method takes the binary representation of a finite non-zero number parsed
		 *          into parts and outputs the decimal representation with the least number
		 *          of significant digits among those which are read back as this very same
		 *          number. The exponent is chosen from the interval of the values rounded
		 *          to the source number, from which the record is reversible under any way
		 *          of parsing which observes the rounding to the nearest.
		 * @param mantissa value of the mantissa field of the binary representation
		 * @param exponent value of the exponent field of the binary representation
		 * @return         shortest decimal representation of the number
		 *
		 * \~
		 */
		inline decimal_t shortest(const uint64_t mantissa, const int32_t exponent) noexcept {
			// Получаем количество явных разрядов мантиссы формата числа
			constexpr int32_t bits = binary_t <T>::mantissaExplicitBits();
			// Получаем смещение показателя степени формата числа
			constexpr int32_t bias = -binary_t <T>::minimumExponent();
			// Показатель степени двойки представления числа
			int32_t power = 0;
			// Мантисса двоичного представления числа
			uint64_t value = 0;
			/**
			 * Если число является денормализованным
			 */
			if(exponent == 0){
				// Определяем показатель степени двойки денормализованного числа
				power = ((1 - bias) - bits) - 2;
				// Определяем мантиссу денормализованного числа
				value = mantissa;
			/**
			 * Если число является нормализованным
			 */
			} else {
				// Определяем показатель степени двойки нормализованного числа
				power = ((exponent - bias) - bits) - 2;
				// Определяем мантиссу нормализованного числа с учётом неявного разряда
				value = ((1ull << bits) | mantissa);
			}
			// Определяем допустимость включения границ промежутка
			const bool bounds = ((value & 1ull) == 0ull);
			// Формируем середину промежутка значений округляемых к числу
			const uint64_t middle = (4ull * value);
			// Определяем поправку нижней границы промежутка на неравномерность шага
			const uint64_t correction = static_cast <uint64_t> ((mantissa != 0ull) || (exponent <= 1));
			// Признак отсутствия значащих разрядов ниже нижней границы промежутка
			bool lowerZeros = false;
			// Признак отсутствия значащих разрядов ниже середины промежутка
			bool middleZeros = false;
			// Верхняя граница промежутка после приведения к десятичному масштабу
			uint64_t upper = 0;
			// Середина промежутка после приведения к десятичному масштабу
			uint64_t centre = 0;
			// Нижняя граница промежутка после приведения к десятичному масштабу
			uint64_t lower = 0;
			// Показатель степени десяти приведённого промежутка
			int32_t scale = 0;
			/**
			 * Если показатель степени двойки является неотрицательным
			 */
			if(power >= 0){
				// Определяем показатель степени десяти приведения промежутка
				const int32_t index = (log10PowerOfTwo(power) - static_cast <int32_t> (power > 3));
				// Определяем количество разрядов сдвига приведения промежутка
				const int32_t shift = ((powers::INVERSE_BITCOUNT + bitsPowerOfFive(index)) - 1) - power + index;
				// Получаем старшее слово обратной степени пятёрки
				const uint64_t high = powers::POWER_OF_FIVE_INVERSE[index * 2];
				// Получаем младшее слово обратной степени пятёрки
				const uint64_t low = powers::POWER_OF_FIVE_INVERSE[(index * 2) + 1];
				// Запоминаем показатель степени десяти приведённого промежутка
				scale = index;
				// Выполняем приведение середины промежутка
				centre = multiplyShift(middle, high, low, shift);
				// Выполняем приведение верхней границы промежутка
				upper = multiplyShift(middle + 2ull, high, low, shift);
				// Выполняем приведение нижней границы промежутка
				lower = multiplyShift((middle - 1ull) - correction, high, low, shift);
				/**
				 * Если показатель степени десяти допускает наличие незначащих разрядов
				 *
				 * @note Предел взят по разрядности середины промежутка: большая степень
				 *       пятёрки её заведомо превосходит и делителем быть не может
				 */
				if(index <= 21){
					/**
					 * Если середина промежутка кратна пятёрке
					 */
					if((middle % 5ull) == 0ull)
						// Определяем отсутствие значащих разрядов ниже середины промежутка
						middleZeros = multipleOfPowerOfFive(middle, index);
					/**
					 * Если границы промежутка включаются в рассмотрение
					 */
					else if(bounds)
						// Определяем отсутствие значащих разрядов ниже нижней границы
						lowerZeros = multipleOfPowerOfFive((middle - 1ull) - correction, index);
					/**
					 * Если верхняя граница промежутка в рассмотрение не включается
					 */
					else upper -= static_cast <uint64_t> (multipleOfPowerOfFive(middle + 2ull, index));
				}
			/**
			 * Если показатель степени двойки является отрицательным
			 */
			} else {
				// Определяем показатель степени десяти приведения промежутка
				const int32_t index = (log10PowerOfFive(-power) - static_cast <int32_t> (-power > 1));
				// Определяем номер записи таблицы прямых степеней пятёрки
				const int32_t entry = (-power - index);
				// Определяем количество разрядов сдвига приведения промежутка
				const int32_t shift = index - (bitsPowerOfFive(entry) - powers::DIRECT_BITCOUNT);
				// Получаем старшее слово прямой степени пятёрки
				const uint64_t high = powers::POWER_OF_FIVE_DIRECT[entry * 2];
				// Получаем младшее слово прямой степени пятёрки
				const uint64_t low = powers::POWER_OF_FIVE_DIRECT[(entry * 2) + 1];
				// Запоминаем показатель степени десяти приведённого промежутка
				scale = (index + power);
				// Выполняем приведение середины промежутка
				centre = multiplyShift(middle, high, low, shift);
				// Выполняем приведение верхней границы промежутка
				upper = multiplyShift(middle + 2ull, high, low, shift);
				// Выполняем приведение нижней границы промежутка
				lower = multiplyShift((middle - 1ull) - correction, high, low, shift);
				/**
				 * Если приведение выполнено без потери разрядов
				 */
				if(index <= 1){
					// Определяем отсутствие значащих разрядов ниже середины промежутка
					middleZeros = true;
					/**
					 * Если границы промежутка включаются в рассмотрение
					 */
					if(bounds)
						// Определяем отсутствие значащих разрядов ниже нижней границы
						lowerZeros = (correction == 1ull);
					/**
					 * Если верхняя граница промежутка в рассмотрение не включается
					 */
					else upper--;
				/**
				 * Если показатель степени десяти умещается в разрядность середины промежутка
				 */
				} else if(index < 63)
					// Определяем отсутствие значащих разрядов ниже середины промежутка
					middleZeros = multipleOfPowerOfTwo(middle, index);
			}
			// Количество отброшенных десятичных разрядов
			int32_t removed = 0;
			// Значение последнего отброшенного десятичного разряда
			uint8_t digit = 0;
			// Результирующая мантисса десятичного представления
			uint64_t result = 0;
			/**
			 * Если промежуток содержит записи без значащих разрядов ниже границ
			 *
			 * @note Ветвь эта редка и отброс разрядов ведётся в ней по одному, зато с
			 *       отслеживанием точности: обычная ветвь ниже отбрасывает разряды парами
			 */
			if(lowerZeros || middleZeros){
				/**
				 * Выполняем отбрасывание разрядов пока промежуток содержит запись короче
				 */
				while((upper / 10ull) > (lower / 10ull)){
					// Уточняем отсутствие значащих разрядов ниже нижней границы
					lowerZeros &= ((lower % 10ull) == 0ull);
					// Уточняем отсутствие значащих разрядов ниже середины промежутка
					middleZeros &= (digit == 0);
					// Запоминаем отбрасываемый разряд середины промежутка
					digit = static_cast <uint8_t> (centre % 10ull);
					// Выполняем отбрасывание разряда середины промежутка
					centre /= 10ull;
					// Выполняем отбрасывание разряда верхней границы промежутка
					upper /= 10ull;
					// Выполняем отбрасывание разряда нижней границы промежутка
					lower /= 10ull;
					// Увеличиваем количество отброшенных разрядов
					removed++;
				}
				/**
				 * Если нижняя граница промежутка значащих разрядов не содержит
				 */
				if(lowerZeros){
					/**
					 * Выполняем отбрасывание незначащих разрядов нижней границы
					 */
					while((lower % 10ull) == 0ull){
						// Уточняем отсутствие значащих разрядов ниже середины промежутка
						middleZeros &= (digit == 0);
						// Запоминаем отбрасываемый разряд середины промежутка
						digit = static_cast <uint8_t> (centre % 10ull);
						// Выполняем отбрасывание разряда середины промежутка
						centre /= 10ull;
						// Выполняем отбрасывание разряда верхней границы промежутка
						upper /= 10ull;
						// Выполняем отбрасывание разряда нижней границы промежутка
						lower /= 10ull;
						// Увеличиваем количество отброшенных разрядов
						removed++;
					}
				}
				/**
				 * Если отброшенная часть составляет ровно половину разряда при чётном остатке
				 */
				if(middleZeros && (digit == 5) && ((centre % 2ull) == 0ull))
					// Выполняем округление к чётному значению разряда
					digit = 4;
				// Формируем мантиссу с учётом округления отброшенной части
				result = (centre + static_cast <uint64_t> (
					((centre == lower) && (!bounds || !lowerZeros)) || (digit >= 5)
				));
			/**
			 * Если промежуток записей без значащих разрядов ниже границ не содержит
			 */
			} else {
				// Признак округления мантиссы в большую сторону
				bool round = false;
				/**
				 * Если промежуток содержит запись короче на два разряда
				 */
				if((upper / 100ull) > (lower / 100ull)){
					// Определяем необходимость округления отброшенной пары разрядов
					round = ((centre % 100ull) >= 50ull);
					// Выполняем отбрасывание пары разрядов середины промежутка
					centre /= 100ull;
					// Выполняем отбрасывание пары разрядов верхней границы промежутка
					upper /= 100ull;
					// Выполняем отбрасывание пары разрядов нижней границы промежутка
					lower /= 100ull;
					// Увеличиваем количество отброшенных разрядов
					removed += 2;
				}
				/**
				 * Выполняем отбрасывание разрядов пока промежуток содержит запись короче
				 */
				while((upper / 10ull) > (lower / 10ull)){
					// Определяем необходимость округления отброшенного разряда
					round = ((centre % 10ull) >= 5ull);
					// Выполняем отбрасывание разряда середины промежутка
					centre /= 10ull;
					// Выполняем отбрасывание разряда верхней границы промежутка
					upper /= 10ull;
					// Выполняем отбрасывание разряда нижней границы промежутка
					lower /= 10ull;
					// Увеличиваем количество отброшенных разрядов
					removed++;
				}
				// Формируем мантиссу с учётом округления отброшенной части
				result = (centre + static_cast <uint64_t> ((centre == lower) || round));
			}
			// Выводим кратчайшее десятичное представление числа
			return decimal_t(result, (scale + removed));
		}
	};
};

#endif // __AWH_LEXICAL_SHORTEST__
