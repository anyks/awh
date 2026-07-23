/**
 * @file: parse.hpp
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
#ifndef __AWH_FLOAT_PARSE__
#define __AWH_FLOAT_PARSE__

/**
 * Стандартные заголовочные файлы
 */
#include <cmath>
#include <cstring>
#include <limits>
#include <system_error>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "ascii.hpp"
#include "common.hpp"
#include "decimal.hpp"
#include "digits.hpp"

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
		 * @brief Пространство внутренних деталей
		 *
		 */
		namespace detail {
			/**
			 * @brief Разбирает специальные значения inf/nan
			 *
			 * @tparam T  тип числа с плавающей точкой
			 * @tparam UC тип символа
			 * @param first начало строки
			 * @param last  конец строки
			 * @param value ссылка на результат
			 * @param fmt   формат разбора
			 * @return      результат разбора
			 */
			template <typename T, typename UC>
			constexpr result_t <UC> parseInfNan(const UC * first, const UC * last, T & value, const format_t fmt) noexcept {
				// Результат
				result_t <UC> answer{};
				answer.ptr = first;
				answer.ec = errc(); // Оптимистичная инициализация
				// Предполагаем first < last
				const bool minusSign = (* first == UC('-'));
				// C++17 запрещает знак '+' здесь, если не разрешён явно
				if((* first == UC('-')) || (uint64_t(fmt & format_t::ALLOW_LEADING_PLUS) && (* first == UC('+'))))
					++first;
				// Минимум 3 символа для "nan" / "inf"
				if((last - first) >= 3){
					// Разбор NaN
					if(strncasecmpAscii(first, strConstNan <UC>(), 3)){
						answer.ptr = (first += 3);
						value = minusSign ? -numeric_limits <T>::quiet_NaN() : numeric_limits <T>::quiet_NaN();
						// Возможный nan(n-char-seq-opt), C++17 20.19.3.7 / C11 7.20.1.3.3
						// MSVC может давать nan(ind) и nan(snan)
						if((first != last) && (* first == UC('('))){
							for(const UC * ptr = first + 1; ptr != last; ++ptr){
								if(* ptr == UC(')')){
									answer.ptr = ptr + 1; // Валидный nan(n-char-seq-opt)
									break;
								} else if(!(((UC('a') <= * ptr) && (* ptr <= UC('z'))) ||
									 ((UC('A') <= * ptr) && (* ptr <= UC('Z'))) ||
									 ((UC('0') <= * ptr) && (* ptr <= UC('9'))) ||
									 (* ptr == UC('_')))){
									break; // Запрещённый символ
								}
							}
						}
						return answer;
					}
					// Разбор Infinity / Inf
					if(strncasecmpAscii(first, strConstInf <UC>(), 3)){
						if(((last - first) >= 8) && strncasecmpAscii(first + 3, strConstInf <UC>() + 3, 5))
							answer.ptr = first + 8;
						else
							answer.ptr = first + 3;
						value = minusSign ? -numeric_limits <T>::infinity() : numeric_limits <T>::infinity();
						return answer;
					}
				}
				// Не inf/nan
				answer.ec = errc::invalid_argument;
				return answer;
			}

			/**
			 * @brief Проверяет, что режим округления — к ближайшему
			 *
			 * @details Эквивалентно fegetround() == FE_TONEAREST, но быстрее.
			 *          @see https://lemire.me/blog/2022/11/16/a-fast-function-to-check-your-floating-point-rounding-mode/
			 *
			 * @return true, если округление FE_TONEAREST
			 */
			AWH_FLOAT_INLINE bool roundsToNearest() noexcept {
				#if (FLT_EVAL_METHOD != 1) && (FLT_EVAL_METHOD != 0)
					return false;
				#endif
				// volatile мешает вычислить функцию на этапе компиляции
				static float volatile fmin = numeric_limits <float>::min();
				// Копируем, чтобы значение загрузилось один раз
				const float fmini = fmin;
				// Только при FE_TONEAREST: fmin + 1.0f == 1.0f - fmin
				#ifdef AWH_FLOAT_VISUAL_STUDIO
					#pragma warning(push)
				#elif defined(__clang__)
					#pragma clang diagnostic push
					#pragma clang diagnostic ignored "-Wfloat-equal"
				#elif defined(__GNUC__)
					#pragma GCC diagnostic push
					#pragma GCC diagnostic ignored "-Wfloat-equal"
				#endif
				const bool result = (fmini + 1.0f == 1.0f - fmini);
				#ifdef AWH_FLOAT_VISUAL_STUDIO
					#pragma warning(pop)
				#elif defined(__clang__)
					#pragma clang diagnostic pop
				#elif defined(__GNUC__)
					#pragma GCC diagnostic pop
				#endif
				return result;
			}
		} // namespace detail

		/**
		 * @brief Диспетчер fromChars для типа с плавающей точкой
		 *
		 * @tparam T тип значения
		 */
		template <typename T>
		struct FromCharsCaller {
			/**
			 * @brief Вызывает расширенный разбор
			 *
			 * @tparam UC тип символа
			 * @param first   начало строки
			 * @param last    конец строки
			 * @param value   ссылка на результат
			 * @param options опции разбора
			 * @return        результат разбора
			 */
			template <typename UC>
			AWH_FLOAT_CONSTEXPR20 static result_t <UC> call(const UC * first, const UC * last, T & value, const options_t <UC> options) noexcept {
				return fromCharsAdvanced(first, last, value, options);
			}
		};

		#ifdef __STDCPP_FLOAT32_T__
			/**
			 * @brief Диспетчер fromChars для float32_t
			 *
			 */
			template <>
			struct FromCharsCaller <float32_t> {
				/**
				 * @brief Разбирает через float (эквивалентен float32_t)
				 *
				 * @tparam UC тип символа
				 */
				template <typename UC>
				AWH_FLOAT_CONSTEXPR20 static result_t <UC> call(const UC * first, const UC * last, float32_t & value, const options_t <UC> options) noexcept {
					float val;
					const auto ret = fromCharsAdvanced(first, last, val, options);
					value = val;
					return ret;
				}
			};
		#endif

		#ifdef __STDCPP_FLOAT64_T__
			/**
			 * @brief Диспетчер fromChars для float64_t
			 *
			 */
			template <>
			struct FromCharsCaller <float64_t> {
				/**
				 * @brief Разбирает через double (эквивалентен float64_t)
				 *
				 * @tparam UC тип символа
				 */
				template <typename UC>
				AWH_FLOAT_CONSTEXPR20 static result_t <UC> call(const UC * first, const UC * last, float64_t & value, const options_t <UC> options) noexcept {
					double val;
					const auto ret = fromCharsAdvanced(first, last, val, options);
					value = val;
					return ret;
				}
			};
		#endif

		/**
		 * @brief Разбирает число с плавающей точкой из диапазона [first, last)
		 *
		 * @tparam T  тип числа с плавающей точкой
		 * @tparam UC тип символа
		 * @param first начало строки
		 * @param last  конец строки
		 * @param value ссылка на результат
		 * @param fmt   допустимый формат записи
		 * @return      результат разбора
		 */
		template <typename T, typename UC, typename>
		AWH_FLOAT_CONSTEXPR20 result_t <UC> fromChars(const UC * first, const UC * last, T & value, const format_t fmt) noexcept {
			return FromCharsCaller <T>::call(first, last, value, options_t <UC> (fmt));
		}

		/**
		 * @brief Быстрый путь Clinger для преобразования мантиссы и экспонента
		 *
		 * @details Должен давать round-to-nearest независимо от режима округления потока.
		 *
		 * @tparam T тип с плавающей точкой
		 * @param mantissa   мантисса
		 * @param exponent   десятичный экспонент
		 * @param isNegative знак
		 * @param value      ссылка на результат
		 * @return           true, если быстрый путь успешен
		 */
		template <typename T>
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 bool clingerFastPathImpl(const uint64_t mantissa, const int64_t exponent, const bool isNegative, T & value) noexcept {
			// Проверяем диапазон экспонента быстрого пути
			if((BinaryFormat <T>::minExponentFastPath() <= exponent) && (exponent <= BinaryFormat <T>::maxExponentFastPath())){
				// Классический быстрый путь Clinger — только при round-to-nearest
				if(!cpp20AndInConstexpr() && detail::roundsToNearest()){
					// fegetround() == FE_TONEAREST
					if(mantissa <= BinaryFormat <T>::maxMantissaFastPath()){
						value = T(mantissa);
						if(exponent < 0)
							value = value / BinaryFormat <T>::exactPowerOfTen(-exponent);
						else
							value = value * BinaryFormat <T>::exactPowerOfTen(exponent);
						if(isNegative)
							value = -value;
						return true;
					}
				} else {
					// fegetround() != FE_TONEAREST — модифицированный путь (Jakub Jelínek)
					if((exponent >= 0) && (mantissa <= BinaryFormat <T>::maxMantissaFastPath(exponent))){
						#if defined(__clang__) || defined(AWH_FLOAT_32BIT)
							// Clang может отобразить 0 в -0.0 при FE_DOWNWARD
							if(mantissa == 0){
								value = isNegative ? T(-0.) : T(0.);
								return true;
							}
						#endif
						value = T(mantissa) * BinaryFormat <T>::exactPowerOfTen(exponent);
						if(isNegative)
							value = -value;
						return true;
					}
				}
			}
			return false;
		}

		/**
		 * @brief Расширенный разбор по уже разобранному parsedNumber_t
		 *
		 * @details Структура заполняется fromCharsAdvanced по диапазону символов
		 *          или пользовательским парсером.
		 *
		 * @tparam T  тип с плавающей точкой
		 * @tparam UC тип символа
		 * @param pns   разобранное число
		 * @param value ссылка на результат
		 * @return      результат разбора
		 */
		template <typename T, typename UC>
		AWH_FLOAT_CONSTEXPR20 result_t <UC> fromCharsAdvanced(parsedNumber_t <UC> & pns, T & value) noexcept {
			static_assert(IsSupportedFloat <T>::value, "only some floating-point types are supported");
			static_assert(IsSupportedChar <UC>::value, "only char, wchar_t, char16_t and char32_t are supported");
			// Результат
			result_t <UC> answer;
			answer.ec = errc(); // Оптимистичная инициализация
			answer.ptr = pns.lastmatch;
			// Быстрый путь Clinger
			if(!pns.tooManyDigits && clingerFastPathImpl(pns.mantissa, pns.exponent, pns.negative, value))
				return answer;
			// Основной путь через computeFloat
			AdjustedMantissa am = computeFloat <BinaryFormat <T>> (pns.exponent, pns.mantissa);
			// При избытке цифр и валидном результате уточняем неоднозначность
			if(pns.tooManyDigits && (am.power2 >= 0)){
				if(am != computeFloat <BinaryFormat <T>> (pns.exponent, pns.mantissa + 1))
					am = computeError <BinaryFormat <T>> (pns.exponent, pns.mantissa);
			}
			// Невалидная степень (am.power2 < 0) — редкий длинный путь через digitComp
			if(am.power2 < 0)
				am = digitComp <T> (pns, am);
			// Собираем значение
			toFloat(pns.negative, am, value);
			// Переполнение / антипереполнение
			if(((pns.mantissa != 0) && (am.mantissa == 0) && (am.power2 == 0)) || (am.power2 == BinaryFormat <T>::infinitePower()))
				answer.ec = errc::result_out_of_range;
			return answer;
		}

		/**
		 * @brief Расширенный разбор числа с плавающей точкой из строки
		 *
		 * @tparam T  тип с плавающей точкой
		 * @tparam UC тип символа
		 * @param first   начало строки
		 * @param last    конец строки
		 * @param value   ссылка на результат
		 * @param options опции разбора
		 * @return        результат разбора
		 */
		template <typename T, typename UC>
		AWH_FLOAT_CONSTEXPR20 result_t <UC> fromCharsFloatAdvanced(const UC * first, const UC * last, T & value, const options_t <UC> options) noexcept {
			static_assert(IsSupportedFloat <T>::value, "only some floating-point types are supported");
			static_assert(IsSupportedChar <UC>::value, "only char, wchar_t, char16_t and char32_t are supported");
			// Корректируем формат с учётом feature-макросов
			const format_t fmt = detail::adjustForFeatureMacros(options.format);
			// Результат
			result_t <UC> answer;
			// Пропуск пробелов
			if(uint64_t(fmt & format_t::SKIP_WHITE_SPACE)){
				while((first != last) && isSpace(* first))
					first++;
			}
			// Пустой диапазон
			if(first == last){
				answer.ec = errc::invalid_argument;
				answer.ptr = first;
				return answer;
			}
			// Разбор числовой строки (JSON-строгий или обычный)
			parsedNumber_t <UC> pns = uint64_t(fmt & detail::basicJsonFmt)
				? parseNumberString <true, UC> (first, last, options)
				: parseNumberString <false, UC> (first, last, options);
			// Невалидное число — пробуем inf/nan, если разрешено
			if(!pns.valid){
				if(uint64_t(fmt & format_t::NO_INFNAN)){
					answer.ec = errc::invalid_argument;
					answer.ptr = first;
					return answer;
				} else {
					return detail::parseInfNan(first, last, value, fmt);
				}
			}
			// Разбор по parsedNumber_t
			return fromCharsAdvanced(pns, value);
		}

		/**
		 * @brief Разбирает целое число из диапазона [first, last)
		 *
		 * @tparam T  целочисленный тип
		 * @tparam UC тип символа
		 * @param first начало строки
		 * @param last  конец строки
		 * @param value ссылка на результат
		 * @param base  основание (2..36)
		 * @return      результат разбора
		 */
		template <typename T, typename UC, typename>
		AWH_FLOAT_CONSTEXPR20 result_t <UC> fromChars(const UC * first, const UC * last, T & value, const int base) noexcept {
			static_assert(IsSupportedInteger <T>::value, "only integer types are supported");
			static_assert(IsSupportedChar <UC>::value, "only char, wchar_t, char16_t and char32_t are supported");
			options_t <UC> options;
			options.base = base;
			return fromCharsAdvanced(first, last, value, options);
		}

		/**
		 * @brief Умножает беззнаковое целое на 10^decimalExponent
		 *
		 * @tparam T тип с плавающей точкой
		 * @param mantissa        мантисса
		 * @param decimalExponent десятичный экспонент
		 * @return                результат
		 */
		template <typename T>
		AWH_FLOAT_CONSTEXPR20 typename enable_if <IsSupportedFloat <T>::value, T>::type integerTimesPow10(const uint64_t mantissa, const int decimalExponent) noexcept {
			T value;
			if(clingerFastPathImpl(mantissa, decimalExponent, false, value))
				return value;
			const AdjustedMantissa am = computeFloat <BinaryFormat <T>> (decimalExponent, mantissa);
			toFloat(false, am, value);
			return value;
		}

		/**
		 * @brief Умножает знаковое целое на 10^decimalExponent
		 *
		 * @tparam T тип с плавающей точкой
		 * @param mantissa        мантисса
		 * @param decimalExponent десятичный экспонент
		 * @return                результат
		 */
		template <typename T>
		AWH_FLOAT_CONSTEXPR20 typename enable_if <IsSupportedFloat <T>::value, T>::type integerTimesPow10(const int64_t mantissa, const int decimalExponent) noexcept {
			const bool isNegative = (mantissa < 0);
			const uint64_t m = static_cast <uint64_t> (isNegative ? -mantissa : mantissa);
			T value;
			if(clingerFastPathImpl(m, decimalExponent, isNegative, value))
				return value;
			const AdjustedMantissa am = computeFloat <BinaryFormat <T>> (decimalExponent, m);
			toFloat(isNegative, am, value);
			return value;
		}

		/**
		 * @brief Умножает uint64_t на 10^decimalExponent в double
		 *
		 * @param mantissa        мантисса
		 * @param decimalExponent десятичный экспонент
		 * @return                результат
		 */
		AWH_FLOAT_CONSTEXPR20 inline double integerTimesPow10(const uint64_t mantissa, const int decimalExponent) noexcept {
			return integerTimesPow10 <double> (mantissa, decimalExponent);
		}

		/**
		 * @brief Умножает int64_t на 10^decimalExponent в double
		 *
		 * @param mantissa        мантисса
		 * @param decimalExponent десятичный экспонент
		 * @return                результат
		 */
		AWH_FLOAT_CONSTEXPR20 inline double integerTimesPow10(const int64_t mantissa, const int decimalExponent) noexcept {
			return integerTimesPow10 <double> (mantissa, decimalExponent);
		}

		/**
		 * @brief Перегрузка integerTimesPow10 для беззнаковых целых
		 *
		 * @details Нужна, чтобы избежать неоднозначности для int/unsigned и т.п.
		 *
		 * @tparam T   тип с плавающей точкой
		 * @tparam Int беззнаковый целочисленный тип
		 */
		template <typename T, typename Int>
		AWH_FLOAT_CONSTEXPR20 typename enable_if <
			IsSupportedFloat <T>::value && is_integral <Int>::value && !is_signed <Int>::value, T
		>::type integerTimesPow10(const Int mantissa, const int decimalExponent) noexcept {
			return integerTimesPow10 <T> (static_cast <uint64_t> (mantissa), decimalExponent);
		}

		/**
		 * @brief Перегрузка integerTimesPow10 для знаковых целых
		 *
		 * @tparam T   тип с плавающей точкой
		 * @tparam Int знаковый целочисленный тип
		 */
		template <typename T, typename Int>
		AWH_FLOAT_CONSTEXPR20 typename enable_if <
			IsSupportedFloat <T>::value && is_integral <Int>::value && is_signed <Int>::value, T
		>::type integerTimesPow10(const Int mantissa, const int decimalExponent) noexcept {
			return integerTimesPow10 <T> (static_cast <int64_t> (mantissa), decimalExponent);
		}

		/**
		 * @brief Перегрузка integerTimesPow10 (double) для беззнаковых целых
		 *
		 * @tparam Int беззнаковый целочисленный тип
		 */
		template <typename Int>
		AWH_FLOAT_CONSTEXPR20 typename enable_if <
			is_integral <Int>::value && !is_signed <Int>::value, double
		>::type integerTimesPow10(const Int mantissa, const int decimalExponent) noexcept {
			return integerTimesPow10(static_cast <uint64_t> (mantissa), decimalExponent);
		}

		/**
		 * @brief Перегрузка integerTimesPow10 (double) для знаковых целых
		 *
		 * @tparam Int знаковый целочисленный тип
		 */
		template <typename Int>
		AWH_FLOAT_CONSTEXPR20 typename enable_if <
			is_integral <Int>::value && is_signed <Int>::value, double
		>::type integerTimesPow10(const Int mantissa, const int decimalExponent) noexcept {
			return integerTimesPow10(static_cast <int64_t> (mantissa), decimalExponent);
		}

		/**
		 * @brief Расширенный разбор целого числа из строки
		 *
		 * @tparam T  целочисленный тип
		 * @tparam UC тип символа
		 * @param first   начало строки
		 * @param last    конец строки
		 * @param value   ссылка на результат
		 * @param options опции разбора
		 * @return        результат разбора
		 */
		template <typename T, typename UC>
		AWH_FLOAT_CONSTEXPR20 result_t <UC> fromCharsIntAdvanced(const UC * first, const UC * last, T & value, const options_t <UC> options) noexcept {
			static_assert(IsSupportedInteger <T>::value, "only integer types are supported");
			static_assert(IsSupportedChar <UC>::value, "only char, wchar_t, char16_t and char32_t are supported");
			// Корректируем формат и читаем основание
			const format_t fmt = detail::adjustForFeatureMacros(options.format);
			const int base = options.base;
			// Результат
			result_t <UC> answer;
			// Пропуск пробелов
			if(uint64_t(fmt & format_t::SKIP_WHITE_SPACE)){
				while((first != last) && isSpace(* first))
					first++;
			}
			// Пустой диапазон или недопустимое основание
			if((first == last) || (base < 2) || (base > 36)){
				answer.ec = errc::invalid_argument;
				answer.ptr = first;
				return answer;
			}
			// Разбор целой строки
			return parseIntString(first, last, value, options);
		}

		/**
		 * @brief Диспетчер расширенного разбора по индексу типа
		 *
		 * @tparam TypeIx 1 — float, 2 — integer
		 */
		template <size_t TypeIx>
		struct FromCharsAdvancedCaller {
			static_assert(TypeIx > 0, "unsupported type");
		};

		/**
		 * @brief Диспетчер расширенного разбора для float
		 *
		 */
		template <>
		struct FromCharsAdvancedCaller <1> {
			/**
			 * @brief Вызывает fromCharsFloatAdvanced
			 *
			 * @tparam T  тип с плавающей точкой
			 * @tparam UC тип символа
			 */
			template <typename T, typename UC>
			AWH_FLOAT_CONSTEXPR20 static result_t <UC> call(const UC * first, const UC * last, T & value, const options_t <UC> options) noexcept {
				return fromCharsFloatAdvanced(first, last, value, options);
			}
		};

		/**
		 * @brief Диспетчер расширенного разбора для integer
		 *
		 */
		template <>
		struct FromCharsAdvancedCaller <2> {
			/**
			 * @brief Вызывает fromCharsIntAdvanced
			 *
			 * @tparam T  целочисленный тип
			 * @tparam UC тип символа
			 */
			template <typename T, typename UC>
			AWH_FLOAT_CONSTEXPR20 static result_t <UC> call(const UC * first, const UC * last, T & value, const options_t <UC> options) noexcept {
				return fromCharsIntAdvanced(first, last, value, options);
			}
		};

		/**
		 * @brief Расширенный разбор числа с опциями
		 *
		 * @details Поддерживает числа с плавающей точкой и целые.
		 *
		 * @tparam T  тип результата
		 * @tparam UC тип символа
		 * @param first   начало строки
		 * @param last    конец строки
		 * @param value   ссылка на результат
		 * @param options опции разбора
		 * @return        результат разбора
		 */
		template <typename T, typename UC>
		AWH_FLOAT_CONSTEXPR20 result_t <UC> fromCharsAdvanced(const UC * first, const UC * last, T & value, const options_t <UC> options) noexcept {
			return FromCharsAdvancedCaller <
				size_t(IsSupportedFloat <T>::value) + 2 * size_t(IsSupportedInteger <T>::value)
			>::call(first, last, value, options);
		}
	} // namespace floating
} // namespace awh

#endif // __AWH_FLOAT_PARSE__
