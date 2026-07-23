/**
 * @file: digits.hpp
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
#ifndef __AWH_FLOAT_DIGITS__
#define __AWH_FLOAT_DIGITS__

/**
 * Стандартные заголовочные файлы
 */
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iterator>

/**
 * Подключаем зависимости модуля
 */
#include "common.hpp"
#include "bigint.hpp"
#include "ascii.hpp"

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
		 * @brief Степени десяти от 1e0 до 1e19
		 *
		 */
		constexpr uint64_t powersOfTenUint64[] = {
			1UL,
			10UL,
			100UL,
			1000UL,
			10000UL,
			100000UL,
			1000000UL,
			10000000UL,
			100000000UL,
			1000000000UL,
			10000000000UL,
			100000000000UL,
			1000000000000UL,
			10000000000000UL,
			100000000000000UL,
			1000000000000000UL,
			10000000000000000UL,
			100000000000000000UL,
			1000000000000000000UL,
			10000000000000000000UL
		};

		/**
		 * @brief Вычисляет экспонент числа в научной записи
		 *
		 * @details Алгоритм не оптимизирован: ускорение потребовало бы
		 *          замедлить быстрые пути, а текущий вариант достаточно быстр.
		 *
		 * @param mantissa мантисса
		 * @param exponent исходный экспонент
		 * @return         научный экспонент
		 */
		AWH_FLOAT_INLINE constexpr int32_t scientificExponent(uint64_t mantissa, int32_t exponent) noexcept {
			/**
			 *  Сжимаем мантиссу четвёрками десятичных разрядов
			 */
			while(mantissa >= 10000){
				mantissa /= 10000;
				exponent += 4;
			}
			/**
			 *  Сжимаем парами разрядов
			 */
			while(mantissa >= 100){
				mantissa /= 100;
				exponent += 2;
			}
			/**
			 *  Сжимаем по одному разряду
			 */
			while(mantissa >= 10){
				mantissa /= 10;
				exponent += 1;
			}
			// Возвращаем научный экспонент
			return exponent;
		}

		/**
		 * @brief Преобразует нативный float в расширенную точность
		 *
		 * @tparam T тип с плавающей точкой
		 * @param value исходное значение
		 * @return      AdjustedMantissa в расширенной точности
		 */
		template <typename T>
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 AdjustedMantissa toExtended(const T value) noexcept {
			using equiv_uint = equivUint_t <T>;
			// Маски двоичного формата
			constexpr equiv_uint exponentMask = BinaryFormat <T>::exponentMask();
			constexpr equiv_uint mantissaMask = BinaryFormat <T>::mantissaMask();
			constexpr equiv_uint hiddenBitMask = BinaryFormat <T>::hiddenBitMask();
			// Результат
			AdjustedMantissa am;
			// Смещение экспоненты относительно bias
			const int32_t bias = BinaryFormat <T>::mantissaExplicitBits() - BinaryFormat <T>::minimumExponent();
			// Биты значения
			equiv_uint bits;
			#if AWH_FLOAT_HAS_BIT_CAST
				bits = bit_cast <equiv_uint> (value);
			#else
				::memcpy(&bits, &value, sizeof(T));
			#endif
			// Денормализованное значение
			if((bits & exponentMask) == 0){
				am.power2 = 1 - bias;
				am.mantissa = bits & mantissaMask;
			// Нормальное значение
			} else {
				am.power2 = int32_t((bits & exponentMask) >> BinaryFormat <T>::mantissaExplicitBits());
				am.power2 -= bias;
				am.mantissa = (bits & mantissaMask) | hiddenBitMask;
			}
			// Возвращаем расширенное представление
			return am;
		}

		/**
		 * @brief Возвращает середину между b и b+u в расширенной точности
		 *
		 * @tparam T тип с плавающей точкой
		 * @param value нативный float для нижней границы b
		 * @return      середина между b и следующим float
		 */
		template <typename T>
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 AdjustedMantissa toExtendedHalfway(const T value) noexcept {
			// Берём расширенное представление b
			AdjustedMantissa am = toExtended(value);
			// Удваиваем мантиссу и добавляем 1 — середина до следующего float
			am.mantissa <<= 1;
			am.mantissa += 1;
			am.power2 -= 1;
			// Возвращаем середину
			return am;
		}

		/**
		 * @brief Округляет float расширенной точности к машинному float
		 *
		 * @tparam T        тип с плавающей точкой
		 * @tparam callback тип колбэка округления
		 * @param am        мантисса расширенной точности (изменяется)
		 * @param cb        колбэк сдвига/округления
		 */
		template <typename T, typename callback>
		AWH_FLOAT_INLINE constexpr void round(AdjustedMantissa & am, callback cb) noexcept {
			// Сдвиг для нормального float
			const int32_t mantissaShift = 64 - BinaryFormat <T>::mantissaExplicitBits() - 1;
			// Денормализованный float
			if((-am.power2) >= mantissaShift){
				const int32_t shift = -am.power2 + 1;
				cb(am, min <int32_t> (shift, 64));
				// Если round-nearest дошёл до скрытого бита — значение стало нормальным
				am.power2 = (am.mantissa < (uint64_t(1) << BinaryFormat <T>::mantissaExplicitBits())) ? 0 : 1;
				return;
			}
			// Нормальный float — сдвиг по умолчанию
			cb(am, mantissaShift);
			// Перенос при переполнении мантиссы
			if(am.mantissa >= (uint64_t(2) << BinaryFormat <T>::mantissaExplicitBits())){
				am.mantissa = (uint64_t(1) << BinaryFormat <T>::mantissaExplicitBits());
				am.power2++;
			}
			// Сбрасываем скрытый бит; проверяем infinity
			am.mantissa &= ~(uint64_t(1) << BinaryFormat <T>::mantissaExplicitBits());
			if(am.power2 >= BinaryFormat <T>::infinitePower()){
				am.power2 = BinaryFormat <T>::infinitePower();
				am.mantissa = 0;
			}
		}

		/**
		 * @brief Округляет к ближайшему с tie-to-even
		 *
		 * @tparam callback тип колбэка решения об округлении вверх
		 * @param am        мантисса (изменяется)
		 * @param shift     число отбрасываемых младших бит
		 * @param cb        колбэк (isOdd, isHalfway, isAbove) → округлить вверх
		 */
		template <typename callback>
		AWH_FLOAT_INLINE constexpr void roundNearestTieEven(AdjustedMantissa & am, const int32_t shift, callback cb) noexcept {
			// Маска отбрасываемых бит
			const uint64_t mask = (shift == 64) ? UINT64_MAX : ((uint64_t(1) << shift) - 1);
			// Порог «ровно посередине»
			const uint64_t halfway = (shift == 0) ? 0 : (uint64_t(1) << (shift - 1));
			// Отбрасываемые биты
			const uint64_t truncatedBits = am.mantissa & mask;
			const bool isAbove = truncatedBits > halfway;
			const bool isHalfway = truncatedBits == halfway;
			// Сдвигаем цифры на место
			if(shift == 64)
				am.mantissa = 0;
			else
				am.mantissa >>= shift;
			am.power2 += shift;
			// Решаем, округлять ли вверх
			const bool isOdd = ((am.mantissa & 1) == 1);
			am.mantissa += uint64_t(cb(isOdd, isHalfway, isAbove));
		}

		/**
		 * @brief Округляет вниз (отбрасывает младшие биты)
		 *
		 * @param am    мантисса (изменяется)
		 * @param shift число отбрасываемых младших бит
		 */
		AWH_FLOAT_INLINE constexpr void roundDown(AdjustedMantissa & am, const int32_t shift) noexcept {
			// Полный сброс при сдвиге на 64
			if(shift == 64)
				am.mantissa = 0;
			else
				am.mantissa >>= shift;
			// Компенсируем сдвиг в экспоненте
			am.power2 += shift;
		}

		/**
		 * @brief Пропускает ведущие нули в диапазоне символов
		 *
		 * @tparam UC тип символа
		 * @param first начало диапазона (смещается)
		 * @param last  конец диапазона
		 */
		template <typename UC>
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 void skipZeros(const UC * & first, const UC * last) noexcept {
			// Блочное сравнение с нулями вне constexpr
			uint64_t val;
			while(!cpp20AndInConstexpr() && (distance(first, last) >= intCmpLen <UC>())){
				::memcpy(&val, first, sizeof(uint64_t));
				if(val != intCmpZeros <UC>())
					break;
				first += intCmpLen <UC>();
			}
			/**
			 *  Побайтовый пропуск оставшихся нулей
			 */
			while(first != last){
				if(* first != UC('0'))
					break;
				first++;
			}
		}

		/**
		 * @brief Проверяет, были ли отброшены ненулевые цифры
		 *
		 * @details Все символы диапазона должны быть валидными цифрами.
		 *
		 * @tparam UC тип символа
		 * @param first начало диапазона
		 * @param last  конец диапазона
		 * @return      true, если есть ненулевая цифра
		 */
		template <typename UC>
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 bool isTruncated(const UC * first, const UC * last) noexcept {
			// Блочное сравнение с нулями вне constexpr
			uint64_t val;
			while(!cpp20AndInConstexpr() && (distance(first, last) >= intCmpLen <UC>())){
				::memcpy(&val, first, sizeof(uint64_t));
				if(val != intCmpZeros <UC>())
					return true;
				first += intCmpLen <UC>();
			}
			/**
			 *  Побайтовая проверка
			 */
			while(first != last){
				if(* first != UC('0'))
					return true;
				++first;
			}
			return false;
		}

		/**
		 * @brief Проверяет усечение по диапазону span
		 *
		 * @tparam UC тип символа
		 * @param s   диапазон символов
		 * @return    true, если есть ненулевая цифра
		 */
		template <typename UC>
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 bool isTruncated(span <const UC> s) noexcept {
			return isTruncated(s.ptr, s.ptr + s.len());
		}

		/**
		 * @brief Разбирает восемь цифр и добавляет к аккумулятору
		 *
		 * @tparam UC тип символа
		 * @param p       указатель на цифры (смещается на 8)
		 * @param value   аккумулятор
		 * @param counter счётчик цифр в текущем блоке
		 * @param count   общий счётчик цифр
		 */
		template <typename UC>
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 void parseEightDigits(const UC * & p, limb & value, size_t & counter, size_t & count) noexcept {
			value = value * 100000000 + parseEightDigitsUnrolled(p);
			p += 8;
			counter += 8;
			count += 8;
		}

		/**
		 * @brief Разбирает одну цифру и добавляет к аккумулятору
		 *
		 * @tparam UC тип символа
		 * @param p       указатель на цифру (смещается на 1)
		 * @param value   аккумулятор
		 * @param counter счётчик цифр в текущем блоке
		 * @param count   общий счётчик цифр
		 */
		template <typename UC>
		AWH_FLOAT_INLINE constexpr void parseOneDigit(const UC * & p, limb & value, size_t & counter, size_t & count) noexcept {
			value = value * 10 + limb(* p - UC('0'));
			p++;
			counter++;
			count++;
		}

		/**
		 * @brief Умножает bigint на степень и добавляет значение
		 *
		 * @param big   большое целое
		 * @param power множитель (степень десяти)
		 * @param value добавляемое значение
		 */
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 void addNative(bigint & big, const limb power, const limb value) noexcept {
			big.mul(power);
			big.add(value);
		}

		/**
		 * @brief Округляет bigint вверх на одну единицу младшего разряда
		 *
		 * @details Избегает сценария …9999 → …10000, дающего ложную середину.
		 *
		 * @param big   большое целое
		 * @param count счётчик цифр (увеличивается)
		 */
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 void roundUpBigint(bigint & big, size_t & count) noexcept {
			addNative(big, 10, 1);
			count++;
		}

		/**
		 * @brief Разбирает значащие цифры в bigint
		 *
		 * @details Минимизирует число умножений: разбирает по 8 цифр и
		 *          умножает на наибольшую скалярную величину (9 или 19 цифр).
		 *
		 * @tparam UC тип символа
		 * @param result    результат (bigint)
		 * @param num       разобранное число
		 * @param maxDigits максимум значащих цифр
		 * @param digits    фактическое число разобранных цифр
		 */
		template <typename UC>
		inline AWH_FLOAT_CONSTEXPR20 void parseMantissa(bigint & result, parsedNumber_t <UC> & num, const size_t maxDigits, size_t & digits) noexcept {
			// Счётчик цифр в текущем блоке и аккумулятор
			size_t counter = 0;
			digits = 0;
			limb value = 0;
			// Размер блока скалярного умножения
			#ifdef AWH_FLOAT_64BIT_LIMB
				const size_t step = 19;
			#else
				const size_t step = 9;
			#endif
			// Обрабатываем целую часть
			const UC * p = num.integer.ptr;
			const UC * pend = p + num.integer.len();
			skipZeros(p, pend);
			/**
			 *  Шагами step за итерацию
			 */
			while(p != pend){
				while((distance(p, pend) >= 8) && ((step - counter) >= 8) && ((maxDigits - digits) >= 8))
					parseEightDigits(p, value, counter, digits);
				while((counter < step) && (p != pend) && (digits < maxDigits))
					parseOneDigit(p, value, counter, digits);
				// Достигнут лимит цифр — проверяем усечение
				if(digits == maxDigits){
					addNative(result, limb(powersOfTenUint64[counter]), value);
					bool truncated = isTruncated(p, pend);
					if(num.fraction.ptr != nullptr)
						truncated |= isTruncated(num.fraction);
					if(truncated)
						roundUpBigint(result, digits);
					return;
				} else {
					addNative(result, limb(powersOfTenUint64[counter]), value);
					counter = 0;
					value = 0;
				}
			}
			// Добавляем дробную часть, если есть
			if(num.fraction.ptr != nullptr){
				p = num.fraction.ptr;
				pend = p + num.fraction.len();
				if(digits == 0)
					skipZeros(p, pend);
				/**
				 *  Шагами step за итерацию
				 */
				while(p != pend){
					while((distance(p, pend) >= 8) && ((step - counter) >= 8) && ((maxDigits - digits) >= 8))
						parseEightDigits(p, value, counter, digits);
					while((counter < step) && (p != pend) && (digits < maxDigits))
						parseOneDigit(p, value, counter, digits);
					if(digits == maxDigits){
						addNative(result, limb(powersOfTenUint64[counter]), value);
						const bool truncated = isTruncated(p, pend);
						if(truncated)
							roundUpBigint(result, digits);
						return;
					} else {
						addNative(result, limb(powersOfTenUint64[counter]), value);
						counter = 0;
						value = 0;
					}
				}
			}
			// Остаток блока
			if(counter != 0)
				addNative(result, limb(powersOfTenUint64[counter]), value);
		}

		/**
		 * @brief Округляет при неотрицательном десятичном экспоненте
		 *
		 * @tparam T тип с плавающей точкой
		 * @param bigmant  bigint-мантисса
		 * @param exponent неотрицательный экспонент (степень 10)
		 * @return         округлённая AdjustedMantissa
		 */
		template <typename T>
		inline AWH_FLOAT_CONSTEXPR20 AdjustedMantissa positiveDigitComp(bigint & bigmant, const int32_t exponent) noexcept {
			// Умножаем на 10^exponent
			AWH_FLOAT_ASSERT(bigmant.pow10(uint32_t(exponent)));
			// Берём старшие 64 бита
			AdjustedMantissa answer;
			bool truncated;
			answer.mantissa = bigmant.hi64(truncated);
			const int bias = BinaryFormat <T>::mantissaExplicitBits() - BinaryFormat <T>::minimumExponent();
			answer.power2 = bigmant.bitLength() - 64 + bias;
			// Округляем к ближайшему с учётом усечения
			round <T> (answer, [truncated](AdjustedMantissa & a, const int32_t shift){
				roundNearestTieEven(a, shift, [truncated](const bool isOdd, const bool isHalfway, const bool isAbove) -> bool {
					return isAbove || (isHalfway && truncated) || (isOdd && isHalfway);
				});
			});
			// Возвращаем результат
			return answer;
		}

		/**
		 * @brief Округляет при отрицательном десятичном экспоненте
		 *
		 * @details Реальные цифры — m * 10^e, теоретические — n * 2^f.
		 *          Так как e отрицателен, масштабируем к одной степени и сравниваем.
		 *
		 * @tparam T тип с плавающей точкой
		 * @param bigmant  bigint-мантисса реальных цифр
		 * @param am       исходная AdjustedMantissa
		 * @param exponent отрицательный экспонент
		 * @return         округлённая AdjustedMantissa
		 */
		template <typename T>
		inline AWH_FLOAT_CONSTEXPR20 AdjustedMantissa negativeDigitComp(bigint & bigmant, const AdjustedMantissa am, const int32_t exponent) noexcept {
			// Реальные цифры и их экспонент
			bigint & realDigits = bigmant;
			const int32_t realExp = exponent;
			// Берём b вниз (округление вниз am)
			AdjustedMantissa amB = am;
			// Обход бага gcc7: лямбда снимает noexcept (-Wnoexcept-type)
			round <T> (amB, [](AdjustedMantissa & a, const int32_t shift){
				roundDown(a, shift);
			});
			// Собираем нативный float b
			T b;
			toFloat(false, amB, b);
			// Середина между b и следующим float
			const AdjustedMantissa theor = toExtendedHalfway(b);
			bigint theorDigits(theor.mantissa);
			const int32_t theorExp = theor.power2;
			// Масштабируем реальные и теоретические цифры к одной степени
			const int32_t pow2Exp = theorExp - realExp;
			const uint32_t pow5Exp = uint32_t(-realExp);
			if(pow5Exp != 0){
				AWH_FLOAT_ASSERT(theorDigits.pow5(pow5Exp));
			}
			if(pow2Exp > 0){
				AWH_FLOAT_ASSERT(theorDigits.pow2(uint32_t(pow2Exp)));
			} else if(pow2Exp < 0){
				AWH_FLOAT_ASSERT(realDigits.pow2(uint32_t(-pow2Exp)));
			}
			// Сравниваем и направляем округление
			const int ord = realDigits.compare(theorDigits);
			AdjustedMantissa answer = am;
			round <T> (answer, [ord](AdjustedMantissa & a, const int32_t shift){
				roundNearestTieEven(a, shift, [ord](const bool isOdd, const bool, const bool) -> bool {
					if(ord > 0)
						return true;
					else if(ord < 0)
						return false;
					else
						return isOdd;
				});
			});
			// Возвращаем результат
			return answer;
		}

		/**
		 * @brief Разбирает значащие цифры как bigint для однозначного округления
		 *
		 * @details При положительном экспоненте относительно значащих цифр
		 *          (например 1234) берём старшие 64 бита bigint и учитываем
		 *          усечение. При отрицательном (например 1.2345) сравниваем
		 *          bigint-представление фактических цифр с теоретическим b+h.
		 *
		 * @tparam T  тип с плавающей точкой
		 * @tparam UC тип символа
		 * @param num разобранное число
		 * @param am  исходная AdjustedMantissa (с invalidAmBias)
		 * @return    округлённая AdjustedMantissa
		 */
		template <typename T, typename UC>
		inline AWH_FLOAT_CONSTEXPR20 AdjustedMantissa digitComp(parsedNumber_t <UC> & num, AdjustedMantissa am) noexcept {
			// Убираем смещение невалидного экспонента
			am.power2 -= invalidAmBias;
			// Научный экспонент и лимит цифр
			const int32_t sciExp = scientificExponent(num.mantissa, static_cast <int32_t> (num.exponent));
			const size_t maxDigits = BinaryFormat <T>::maxDigits();
			size_t digits = 0;
			// Разбираем мантиссу в bigint
			bigint bigmant;
			parseMantissa(bigmant, num, maxDigits, digits);
			// digits не больше maxDigits — переполнения нет
			const int32_t exponent = sciExp + 1 - int32_t(digits);
			// Выбор стратегии по знаку экспонента
			if(exponent >= 0)
				return positiveDigitComp <T> (bigmant, exponent);
			else
				return negativeDigitComp <T> (bigmant, am, exponent);
		}
	} // namespace floating
} // namespace awh

#endif // __AWH_FLOAT_DIGITS__
