/**
 * @file decimal.hpp
 * @date 2026-07-22
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
 * @brief Заголовочный файл быстрого десятичного пути разбора чисел — вычисление двоичной экспоненты по степени десяти
 *        и построение мантиссы через таблицы степеней (алгоритм Eisel-Lemire) без обращения к длинной арифметике
 *
 * \~english
 * @brief Header file of the fast decimal parsing path of numbers — computing the binary exponent from a power of ten
 *        and building the mantissa through the power tables (the Eisel-Lemire algorithm) without resorting to long arithmetic
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_LEXICAL_DECIMAL__
#define __AWH_LEXICAL_DECIMAL__

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
		 * @brief Метод вычисления смещения двоичной экспоненты для степени десяти
		 *
		 * @details Для показателя степени в диапазоне (0, 350) выражение
		 *          ((152170 + 65536) * power) >> 16 равно floor(p) + power,
		 *          где p = log(5^power) / log(2). Для диапазона (-400, 0)
		 *          то же выражение равно -ceil(p) + power.
		 *
		 * @param power показатель степени десяти
		 * @return      смещение двоичной экспоненты
		 *
		 * \~english
		 * @brief Method of computing the binary exponent offset for a power of ten
		 * @details For an exponent in the (0, 350) range the expression
		 *          ((152170 + 65536) * power) >> 16 equals floor(p) + power,
		 *          where p = log(5^power) / log(2). For the (-400, 0) range
		 *          the same expression equals -ceil(p) + power.
		 * @param power exponent of the power of ten
		 * @return      binary exponent offset
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr int32_t powerExponent(const int32_t power) noexcept {
			// Выводим приближение произведения показателя степени на двоичный логарифм пяти
			return ((((152170 + 65536) * power) >> 16) + 63);
		}

		/**
		 * \~russian
		 * @brief Шаблон требуемой точности приближения
		 *
		 * @tparam PRECISION требуемая точность приближения в битах
		 *
		 * \~english
		 * @brief Template of the required precision of the approximation
		 * @tparam PRECISION required precision of the approximation in bits
		 *
		 * \~
		 */
		template <int32_t PRECISION>
		/**
		 * \~russian
		 * @brief Метод приближённого вычисления произведения мантиссы на степень пяти
		 *
		 * @details Для небольших показателей степени результат является точным,
		 *          поскольку произведение помещается в 128 бит без потерь.
		 *          Показатель степени обязан принадлежать диапазону таблицы:
		 *          проверка выполняется вызывающей стороной.
		 *
		 * @param power    показатель степени пяти
		 * @param mantissa нормализованное значение мантиссы
		 * @return         приближённое 128-битное произведение
		 *
		 * \~english
		 * @brief Method of the approximate computation of the product of the mantissa by a power of five
		 * @details For small exponents the result is exact,
		 *          since the product fits into 128 bits without losses.
		 *          The exponent must belong to the range of the table:
		 *          the check is performed by the calling side.
		 * @param power    exponent of the power of five
		 * @param mantissa normalised value of the mantissa
		 * @return         approximate 128-bit product
		 *
		 * \~
		 */
		AWH_ASCII_INLINE value128_t approximateProduct(const int64_t power, const uint64_t mantissa) noexcept {
			// Выполняем проверку допустимости требуемой точности
			static_assert((PRECISION >= 0) && (PRECISION <= 64), "AWH lexical: precision should be in range (0, 64]");
			// Выполняем проверку принадлежности показателя степени диапазону таблицы
			AWH_LEXICAL_ASSERT(powers::isSupported(power));
			// Индекс старшего слова степени пяти в таблице
			const int32_t index = (2 * static_cast <int32_t> (power - powers::SMALLEST_POWER));
			// Выполняем вычисление основного произведения
			value128_t result = multiply128(mantissa, powers::POWER_OF_FIVE[index]);
			// Маска значимых младших бит старшего слова произведения
			constexpr uint64_t PRECISION_MASK = ((PRECISION < 64)
				? (0xFFFFFFFFFFFFFFFFULL >> PRECISION)
				: 0xFFFFFFFFFFFFFFFFULL);
			// Если значимые биты произведения оказались насыщены
			if((result.high & PRECISION_MASK) == PRECISION_MASK){
				// Выполняем вычисление уточняющего произведения по младшему слову степени
				const value128_t refinement = multiply128(mantissa, powers::POWER_OF_FIVE[index + 1]);
				// Выполняем уточнение младшего слова основного произведения
				result.low += refinement.high;
				// Если уточнение вызвало перенос в старшее слово
				if(refinement.high > result.low)
					// Учитываем перенос в старшем слове произведения
					result.high++;
			}
			// Выводим приближённое произведение
			return result;
		}

		/**
		 * \~russian
		 * @brief Шаблон типа параметров двоичного формата
		 *
		 * @tparam BINARY параметры двоичного формата результата
		 *
		 * \~english
		 * @brief Template of the type of the binary format parameters
		 * @tparam BINARY binary format parameters of the result
		 *
		 * \~
		 */
		template <typename BINARY>
		/**
		 * \~russian
		 * @brief Метод вычисления мантиссы с заведомо невалидным показателем степени
		 *
		 * @details Показатель степени смещается на INVALID_BIAS, что помечает
		 *          результат как требующий уточнения длинной арифметикой.
		 *
		 * @param power    показатель степени десяти
		 * @param mantissa нормализованное значение мантиссы
		 * @param zeros    количество ведущих нулевых бит исходной мантиссы
		 * @return         скорректированная мантисса с невалидным показателем степени
		 *
		 * \~english
		 * @brief Method of computing the mantissa with a deliberately invalid exponent
		 * @details The exponent is offset by INVALID_BIAS, which marks
		 *          the result as requiring refinement by long arithmetic.
		 * @param power    exponent of the power of ten
		 * @param mantissa normalised value of the mantissa
		 * @param zeros    number of leading zero bits of the source mantissa
		 * @return         corrected mantissa with an invalid exponent
		 *
		 * \~
		 */
		AWH_ASCII_INLINE mantissa_t computeErrorScaled(const int64_t power, const uint64_t mantissa, const int32_t zeros) noexcept {
			// Дополнительный сдвиг, если старший бит мантиссы не установлен
			const int32_t shift = (static_cast <int32_t> (mantissa >> 63) ^ 1);
			// Смещение показателя степени относительно минимального
			const int32_t bias = (BINARY::mantissaExplicitBits() - BINARY::minimumExponent());
			// Выводим скорректированную мантиссу с невалидным показателем степени
			return mantissa_t(
				(mantissa << shift),
				(powerExponent(static_cast <int32_t> (power)) + bias - shift - zeros - 62 + INVALID_BIAS)
			);
		}

		/**
		 * \~russian
		 * @brief Шаблон типа параметров двоичного формата
		 *
		 * @tparam BINARY параметры двоичного формата результата
		 *
		 * \~english
		 * @brief Template of the type of the binary format parameters
		 * @tparam BINARY binary format parameters of the result
		 *
		 * \~
		 */
		template <typename BINARY>
		/**
		 * \~russian
		 * @brief Метод вычисления произведения мантиссы на степень десяти без округления
		 *
		 * @details Результат всегда помечается невалидным показателем степени и
		 *          подлежит уточнению длинной арифметикой.
		 *
		 * @param power    показатель степени десяти
		 * @param mantissa значение мантиссы
		 * @return         скорректированная мантисса с невалидным показателем степени
		 *
		 * \~english
		 * @brief Method of computing the product of the mantissa by a power of ten without rounding
		 * @details The result is always marked with an invalid exponent and
		 *          is subject to refinement by long arithmetic.
		 * @param power    exponent of the power of ten
		 * @param mantissa value of the mantissa
		 * @return         corrected mantissa with an invalid exponent
		 *
		 * \~
		 */
		AWH_ASCII_INLINE mantissa_t computeError(const int64_t power, const uint64_t mantissa) noexcept {
			// Если показатель степени выходит за пределы таблицы степеней
			if(!powers::isSupported(power))
				// Выводим нулевую мантиссу с невалидным показателем степени
				return mantissa_t(0, INVALID_BIAS);
			// Количество ведущих нулевых бит мантиссы
			const int32_t zeros = leadingZeros(mantissa);
			// Выполняем нормализацию мантиссы сдвигом к старшему биту
			const uint64_t shifted = (mantissa << zeros);
			// Выполняем приближённое вычисление произведения с запасом точности
			const value128_t product = approximateProduct <BINARY::mantissaExplicitBits() + 3> (power, shifted);
			// Выводим скорректированную мантиссу с невалидным показателем степени
			return computeErrorScaled <BINARY> (power, product.high, zeros);
		}

		/**
		 * \~russian
		 * @brief Шаблон типа параметров двоичного формата
		 *
		 * @tparam BINARY параметры двоичного формата результата
		 *
		 * \~english
		 * @brief Template of the type of the binary format parameters
		 * @tparam BINARY binary format parameters of the result
		 *
		 * \~
		 */
		template <typename BINARY>
		/**
		 * \~russian
		 * @brief Метод вычисления произведения мантиссы на степень десяти
		 *
		 * @details В подавляющем большинстве случаев метод даёт корректно округлённое
		 *          двоичное представление числа. В редких случаях однозначное округление
		 *          невозможно: тогда показатель степени результата отрицателен и
		 *          вызывающая сторона обязана уточнить результат длинной арифметикой.
		 *
		 * @param power    показатель степени десяти
		 * @param mantissa значение мантиссы
		 * @return         скорректированная мантисса двоичного представления
		 *
		 * \~english
		 * @brief Method of computing the product of the mantissa by a power of ten
		 * @details In the overwhelming majority of cases the method gives a correctly rounded
		 *          binary representation of the number. In rare cases an unambiguous rounding
		 *          is impossible: then the exponent of the result is negative and
		 *          the calling side must refine the result by long arithmetic.
		 * @param power    exponent of the power of ten
		 * @param mantissa value of the mantissa
		 * @return         corrected mantissa of the binary representation
		 *
		 * \~
		 */
		inline mantissa_t computeFloat(const int64_t power, const uint64_t mantissa) noexcept {
			// Если мантисса нулевая или показатель степени ниже представимого
			if((mantissa == 0) || (power < static_cast <int64_t> (BINARY::smallestPowerOfTen())))
				// Выводим нулевое значение
				return mantissa_t(0, 0);
			// Если показатель степени выше представимого
			if(power > static_cast <int64_t> (BINARY::largestPowerOfTen()))
				// Выводим бесконечность
				return mantissa_t(0, BINARY::infinitePower());
			// Результат вычисления двоичного представления
			mantissa_t result;
			// Количество ведущих нулевых бит мантиссы
			const int32_t zeros = leadingZeros(mantissa);
			// Выполняем нормализацию мантиссы сдвигом к старшему биту
			const uint64_t shifted = (mantissa << zeros);
			// Выполняем приближённое вычисление произведения с запасом точности
			// на скрытый бит, бит округления и возможный сдвиг нормализации
			const value128_t product = approximateProduct <BINARY::mantissaExplicitBits() + 3> (power, shifted);
			// Признак установленного старшего бита произведения
			const int32_t upperbit = static_cast <int32_t> (product.high >> 63);
			// Величина сдвига для извлечения мантиссы результата
			const int32_t shift = (upperbit + 64 - BINARY::mantissaExplicitBits() - 3);
			// Выполняем извлечение мантиссы результата
			result.mantissa = (product.high >> shift);
			// Выполняем вычисление показателя степени результата
			result.power2 = (powerExponent(static_cast <int32_t> (power)) + upperbit - zeros - BINARY::minimumExponent());
			// Если результат попадает в субнормальный диапазон
			if(result.power2 <= 0){
				// Если результат находится ниже субнормального диапазона
				if((1 - result.power2) >= 64)
					// Выводим нулевое значение
					return mantissa_t(0, 0);
				// Выполняем сдвиг мантиссы в субнормальный диапазон
				result.mantissa >>= (1 - result.power2);
				/**
				 * Выполняем округление вверх, так как округление к чётному 
				 * и субнормальные значения одновременно не встречаются
				 */
				result.mantissa += (result.mantissa & 1);
				// Выполняем сдвиг мантиссы на место
				result.mantissa >>= 1;
				// Определяем, стало ли значение нормализованным после округления
				result.power2 = ((result.mantissa < (uint64_t(1) << BINARY::mantissaExplicitBits())) ? 0 : 1);
				// Выводим субнормальное значение
				return result;
			}
			/**
			 * Если значение находится ровно посередине между двумя представимыми
			 * числами, а младший значащий бит чётный, округление выполняется вниз
			 */
			if((product.low <= 1) &&
			   (power >= static_cast <int64_t> (BINARY::minExponentRoundToEven())) &&
			   (power <= static_cast <int64_t> (BINARY::maxExponentRoundToEven())) &&
			   ((result.mantissa & 3) == 1)){
				// Если отброшенная часть произведения состоит из одних нулей
				if((result.mantissa << shift) == product.high)
					// Отменяем последующее округление вверх
					result.mantissa &= ~uint64_t(1);
			}
			// Выполняем округление мантиссы вверх
			result.mantissa += (result.mantissa & 1);
			// Выполняем сдвиг мантиссы на место
			result.mantissa >>= 1;
			// Если округление вызвало переполнение мантиссы
			if(result.mantissa >= (uint64_t(2) << BINARY::mantissaExplicitBits())){
				// Выполняем нормализацию мантиссы
				result.mantissa = (uint64_t(1) << BINARY::mantissaExplicitBits());
				// Компенсируем нормализацию показателем степени
				result.power2++;
			}
			// Выполняем сброс скрытого бита мантиссы
			result.mantissa &= ~(uint64_t(1) << BINARY::mantissaExplicitBits());
			// Если показатель степени вышел за пределы представимого диапазона
			if(result.power2 >= BINARY::infinitePower())
				// Выводим бесконечность
				return mantissa_t(0, BINARY::infinitePower());
			// Выводим двоичное представление результата
			return result;
		}
	};
};

#endif // __AWH_LEXICAL_DECIMAL__
