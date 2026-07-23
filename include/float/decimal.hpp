/**
 * @file: decimal.hpp
 * @date: 2026-07-22
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 *
 * Адаптировано из fast_float (https://github.com/fastfloat/fast_float)
 * Copyright 2021 The fast_float authors — MIT License.
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_FLOAT_DECIMAL__
#define __AWH_FLOAT_DECIMAL__

/**
 * Подключаем общие определения и таблицы степеней
 */
#include "common.hpp"
#include "table.hpp"

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
	 * @brief Пространство имён модуля чисел с плавающей точкой
	 *
	 */
	namespace floating {
		/**
		 * @brief Приближённо вычисляет w * 5^q
		 *
		 * @details Возвращает пару 64-битных слов: high — старшие биты, low — младшие.
		 *          Для малых q (например [0, 27]) результат точен, так как
		 *          fullMultiplication(w, powerOfFive128[index]) даёт точное произведение.
		 *
		 * @tparam bitPrecision требуемая точность в битах (0..64)
		 * @param q             степень пятёрки
		 * @param w             множитель
		 * @return              приближённое 128-битное произведение
		 */
		template <int bitPrecision>
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 value128 computeProductApproximation(const int64_t q, const uint64_t w) noexcept {
			// Индекс в таблице степеней пятёрки
			const int index = 2 * int(q - powers::smallestPowerOfFive);
			// Первое (основное) произведение
			value128 firstproduct = fullMultiplication(w, powers::powerOfFive128[index]);
			// Проверяем допустимый диапазон точности
			static_assert((bitPrecision >= 0) && (bitPrecision <= 64), "precision should be in (0,64]");
			// Маска младших битов high-части, которые должны быть значимы
			constexpr uint64_t precisionMask = (bitPrecision < 64)
				? (uint64_t(0xFFFFFFFFFFFFFFFFULL) >> bitPrecision)
				: uint64_t(0xFFFFFFFFFFFFFFFFULL);
			// При насыщении значимых битов уточняем через соседнюю степень
			if((firstproduct.high & precisionMask) == precisionMask){
				// Второе произведение — нужна только high-часть
				const value128 secondproduct = fullMultiplication(w, powers::powerOfFive128[index + 1]);
				// Добавляем high второго произведения к low первого
				firstproduct.low += secondproduct.high;
				// Учитываем перенос в high
				if(secondproduct.high > firstproduct.low)
					firstproduct.high++;
			}
			// Возвращаем приближение
			return firstproduct;
		}

		/**
		 * @brief Пространство внутренних деталей
		 *
		 */
		namespace detail {
			/**
			 * @brief Вычисляет смещение двоичной экспоненты для степени пятёрки
			 *
			 * @details Для q в (0, 350):
			 *            f = (((152170 + 65536) * q) >> 16)
			 *          равно floor(p) + q, где p = log(5^q) / log(2).
			 *          Для отрицательных q в (-400, 0):
			 *            f = (((152170 + 65536) * q) >> 16)
			 *          равно -ceil(p) + q, где p = log(5^-q) / log(2).
			 *
			 * @param q степень пятёрки
			 * @return  смещение экспоненты
			 */
			constexpr AWH_FLOAT_INLINE int32_t power(const int32_t q) noexcept {
				// Аппроксимация q * log2(5) со смещением
				return (((152170 + 65536) * q) >> 16) + 63;
			}
		} // namespace detail

		/**
		 * @brief Строит AdjustedMantissa с невалидным power2
		 *
		 * @details Используется для значащих цифр, уже умноженных на 10^q.
		 *          power2 смещается через invalidAmBias.
		 *
		 * @tparam binary двоичный формат (BinaryFormat<T>)
		 * @param q       десятичный экспонент
		 * @param w       нормализованная мантисса (старший бит)
		 * @param lz      число ведущих нулей исходного w
		 * @return        AdjustedMantissa с невалидным power2
		 */
		template <typename binary>
		AWH_FLOAT_INLINE constexpr AdjustedMantissa computeErrorScaled(const int64_t q, const uint64_t w, const int lz) noexcept {
			// Дополнительный сдвиг, если старший бит w не установлен
			const int hilz = int(w >> 63) ^ 1;
			// Результат
			AdjustedMantissa answer;
			// Нормализуем мантиссу
			answer.mantissa = w << hilz;
			// Смещение относительно минимальной экспоненты формата
			const int bias = binary::mantissaExplicitBits() - binary::minimumExponent();
			// Формируем невалидный power2
			answer.power2 = int32_t(detail::power(int32_t(q)) + bias - hilz - lz - 62 + invalidAmBias);
			// Возвращаем результат
			return answer;
		}

		/**
		 * @brief Вычисляет w * 10^q без округления вверх
		 *
		 * @details power2 в экспоненте корректируется через invalidAmBias.
		 *
		 * @tparam binary двоичный формат (BinaryFormat<T>)
		 * @param q       десятичный экспонент
		 * @param w       мантисса
		 * @return        AdjustedMantissa с невалидным power2
		 */
		template <typename binary>
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 AdjustedMantissa computeError(const int64_t q, const uint64_t w) noexcept {
			// Число ведущих нулей
			const int lz = leadingZeroes(w);
			// Нормализуем мантиссу
			const uint64_t shifted = w << lz;
			// Приближённое произведение с запасом точности
			const value128 product = computeProductApproximation <binary::mantissaExplicitBits() + 3> (q, shifted);
			// Собираем результат с невалидным power2
			return computeErrorScaled <binary> (q, product.high, lz);
		}

		/**
		 * @brief Вычисляет w * 10^q в двоичном формате
		 *
		 * @details В типичных случаях возвращает валидное представление числа.
		 *          В редких случаях вычисление не удаётся — тогда power2
		 *          отрицательный, и вызывающий должен пересчитать результат.
		 *
		 * @tparam binary двоичный формат (BinaryFormat<T>)
		 * @param q       десятичный экспонент
		 * @param w       мантисса
		 * @return        AdjustedMantissa (валидная или с отрицательным power2)
		 */
		template <typename binary>
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 AdjustedMantissa computeFloat(const int64_t q, const uint64_t w) noexcept {
			// Результат
			AdjustedMantissa answer;
			// Ноль или экспонент ниже минимума — возвращаем ноль
			if((w == 0) || (q < binary::smallestPowerOfTen())){
				answer.power2 = 0;
				answer.mantissa = 0;
				return answer;
			}
			// Экспонент выше максимума — возвращаем infinity
			if(q > binary::largestPowerOfTen()){
				answer.power2 = binary::infinitePower();
				answer.mantissa = 0;
				return answer;
			}
			// Здесь q в [powers::smallestPowerOfFive, powers::largestPowerOfFive]
			// Нужен старший бит i == 1 — нормализуем сдвигом
			const int lz = leadingZeroes(w);
			const uint64_t shifted = w << lz;
			// Требуемая точность: mantissaExplicitBits + 3
			// (скрытый бит + бит округления + возможный сдвиг из-за upperbit)
			const value128 product = computeProductApproximation <binary::mantissaExplicitBits() + 3> (q, shifted);
			// product всегда достаточен (Mushtak & Lemire, Fast Number Parsing Without Fallback)
			const int upperbit = int(product.high >> 63);
			const int shift = upperbit + 64 - binary::mantissaExplicitBits() - 3;
			// Извлекаем мантиссу
			answer.mantissa = product.high >> shift;
			// Вычисляем экспоненту
			answer.power2 = int32_t(detail::power(int32_t(q)) + upperbit - lz - binary::minimumExponent());
			// Субнормальное значение?
			if(answer.power2 <= 0){
				// Если ниже минимума более чем на 64 бита — результат точно ноль
				if((-answer.power2 + 1) >= 64){
					answer.power2 = 0;
					answer.mantissa = 0;
					return answer;
				}
				// Сдвигаем в субнормальный диапазон (-answer.power2 + 1 < 64)
				answer.mantissa >>= -answer.power2 + 1;
				// round-to-even и субнормали не встречаются вместе
				answer.mantissa += (answer.mantissa & 1); // округление вверх
				answer.mantissa >>= 1;
				// После округления значение может стать нормальным
				answer.power2 = (answer.mantissa < (uint64_t(1) << binary::mantissaExplicitBits())) ? 0 : 1;
				return answer;
			}
			// Обычно округляем вверх; при точном «посередине» и чётном базисе — вниз.
			// Важны случаи, когда 5^q помещается в одно 64-битное слово.
			if((product.low <= 1) && (q >= binary::minExponentRoundToEven()) &&
				 (q <= binary::maxExponentRoundToEven()) && ((answer.mantissa & 3) == 1)){
				// Между двумя float: отброшены только нули — откатываем округление вверх
				if((answer.mantissa << shift) == product.high)
					answer.mantissa &= ~uint64_t(1);
			}
			// Округление вверх
			answer.mantissa += (answer.mantissa & 1);
			answer.mantissa >>= 1;
			// Переполнение мантиссы после округления
			if(answer.mantissa >= (uint64_t(2) << binary::mantissaExplicitBits())){
				answer.mantissa = (uint64_t(1) << binary::mantissaExplicitBits());
				answer.power2++;
			}
			// Сбрасываем скрытый бит
			answer.mantissa &= ~(uint64_t(1) << binary::mantissaExplicitBits());
			// Infinity при переполнении экспоненты
			if(answer.power2 >= binary::infinitePower()){
				answer.power2 = binary::infinitePower();
				answer.mantissa = 0;
			}
			// Возвращаем результат
			return answer;
		}
	} // namespace floating
} // namespace awh

#endif // __AWH_FLOAT_DECIMAL__
