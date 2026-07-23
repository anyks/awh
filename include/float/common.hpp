/**
 * @file: common.hpp
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
#ifndef __AWH_FLOAT_COMMON__
#define __AWH_FLOAT_COMMON__

/**
 * Стандартные заголовочные файлы
 */
#include <cassert>
#include <cfloat>
#include <cstdint>
#include <cstring>
#include <limits>
#include <system_error>
#include <type_traits>

/**
 * Опциональная поддержка расширенных типов float (C++23)
 */
#ifdef __has_include
	#if __has_include(<stdfloat>) && ((__cplusplus > 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG > 202002L)))
		#include <stdfloat>
	#endif
#endif

/**
 * Подключаем детектор возможностей компилятора
 */
#include "detect.hpp"

/**
 * Поддержка bit_cast
 */
#if AWH_FLOAT_HAS_BIT_CAST
	#include <bit>
#endif

/**
 * Определение разрядности платформы
 */
#if (defined(__x86_64) || defined(__x86_64__) || defined(_M_X64) || \
	 defined(__amd64) || defined(__aarch64__) || defined(_M_ARM64) || \
	 defined(__MINGW64__) || defined(__s390x__) || \
	 defined(__ppc64__) || defined(__PPC64__) || defined(__ppc64le__) || defined(__PPC64LE__) || \
	 defined(__loongarch64) || (defined(__riscv) && (__riscv_xlen == 64)))
	#define AWH_FLOAT_64BIT 1
#elif (defined(__i386) || defined(__i386__) || defined(_M_IX86) || \
	 defined(__arm__) || defined(_M_ARM) || defined(__ppc__) || \
	 defined(__MINGW32__) || defined(__EMSCRIPTEN__) || \
	 (defined(__riscv) && (__riscv_xlen == 32)))
	#define AWH_FLOAT_32BIT 1
#else
	#if SIZE_MAX == 0xffff
		#error Unknown platform (16-bit, unsupported)
	#elif SIZE_MAX == 0xffffffff
		#define AWH_FLOAT_32BIT 1
	#elif SIZE_MAX == 0xffffffffffffffff
		#define AWH_FLOAT_64BIT 1
	#else
		#error Unknown platform (not 32-bit, not 64-bit?)
	#endif
#endif

/**
 * Интринсики MSVC
 */
#if ((defined(_WIN32) || defined(_WIN64)) && !defined(__clang__)) || (defined(_M_ARM64) && !defined(__MINGW32__))
	#include <intrin.h>
#endif

#if defined(_MSC_VER) && !defined(__clang__)
	#define AWH_FLOAT_VISUAL_STUDIO 1
#endif

/**
 * Определение порядка байт
 */
#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__)
	#define AWH_FLOAT_IS_BIG_ENDIAN (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#elif defined(_WIN32)
	#define AWH_FLOAT_IS_BIG_ENDIAN 0
#else
	#if defined(__APPLE__) || defined(__FreeBSD__)
		#include <machine/endian.h>
	#elif defined(sun) || defined(__sun)
		#include <sys/byteorder.h>
	#elif defined(__MVS__)
		#include <sys/endian.h>
	#else
		#ifdef __has_include
			#if __has_include(<endian.h>)
				#include <endian.h>
			#endif
		#endif
	#endif
	#ifndef __BYTE_ORDER__
		#define AWH_FLOAT_IS_BIG_ENDIAN 0
	#endif
	#ifndef __ORDER_LITTLE_ENDIAN__
		#define AWH_FLOAT_IS_BIG_ENDIAN 0
	#endif
	#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		#define AWH_FLOAT_IS_BIG_ENDIAN 0
	#else
		#define AWH_FLOAT_IS_BIG_ENDIAN 1
	#endif
#endif

/**
 * Определение SIMD
 */
#if defined(__SSE2__) || (defined(AWH_FLOAT_VISUAL_STUDIO) && (defined(_M_AMD64) || defined(_M_X64) || (defined(_M_IX86_FP) && (_M_IX86_FP == 2))))
	#define AWH_FLOAT_SSE2 1
#endif
#if defined(__aarch64__) || defined(_M_ARM64)
	#define AWH_FLOAT_NEON 1
#endif
#if defined(AWH_FLOAT_SSE2) || defined(AWH_FLOAT_NEON)
	#define AWH_FLOAT_HAS_SIMD 1
#endif

/**
 * Макросы управления предупреждениями SIMD (GCC)
 */
#if defined(__GNUC__)
	#define AWH_FLOAT_SIMD_DISABLE_WARNINGS \
		_Pragma("GCC diagnostic push") \
		_Pragma("GCC diagnostic ignored \"-Wcast-align\"")
	#define AWH_FLOAT_SIMD_RESTORE_WARNINGS _Pragma("GCC diagnostic pop")
#else
	#define AWH_FLOAT_SIMD_DISABLE_WARNINGS
	#define AWH_FLOAT_SIMD_RESTORE_WARNINGS
#endif

/**
 * Принудительный inline
 */
#ifdef AWH_FLOAT_VISUAL_STUDIO
	#define AWH_FLOAT_INLINE __forceinline
#else
	#define AWH_FLOAT_INLINE inline __attribute__((always_inline))
#endif

/**
 * Утверждения и вспомогательные макросы
 */
#ifndef AWH_FLOAT_ASSERT
	#define AWH_FLOAT_ASSERT(x) { ((void)(x)); }
#endif
#ifndef AWH_FLOAT_DEBUG_ASSERT
	#define AWH_FLOAT_DEBUG_ASSERT(x) { ((void)(x)); }
#endif
#define AWH_FLOAT_TRY(x) { if(!(x)) return false; }

#ifndef FLT_EVAL_METHOD
	#error "FLT_EVAL_METHOD should be defined, please include cfloat."
#endif

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
		 * @brief SFINAE-помощник для шаблонных перегрузок
		 *
		 * @tparam B условие enable_if
		 */
		template <bool B>
		using enableIf_t = typename enable_if <B, int>::type;

		/**
		 * @brief Формат разбора числовой строки (битовые флаги)
		 *
		 */
		enum class format_t : uint64_t;

		/**
		 * @brief Пространство внутренних деталей
		 *
		 */
		namespace detail {
			constexpr format_t basicJsonFmt = format_t(1 << 5);
			constexpr format_t basicFortranFmt = format_t(1 << 6);
		} // namespace detail

		/**
		 * @brief Формат разбора числовой строки
		 *
		 */
		enum class format_t : uint64_t {
			SCIENTIFIC = 1 << 0,          // Научная запись
			FIXED = 1 << 2,               // Фиксированная запись
			HEX = 1 << 3,                 // Шестнадцатеричная запись
			NO_INFNAN = 1 << 4,           // Запрет inf/nan
			JSON = uint64_t(detail::basicJsonFmt) | FIXED | SCIENTIFIC | NO_INFNAN, // RFC 8259
			JSON_OR_INFNAN = uint64_t(detail::basicJsonFmt) | FIXED | SCIENTIFIC,   // JSON + inf/nan
			FORTRAN = uint64_t(detail::basicFortranFmt) | FIXED | SCIENTIFIC,       // Fortran-стиль
			GENERAL = FIXED | SCIENTIFIC, // Общий формат
			ALLOW_LEADING_PLUS = 1 << 7,  // Разрешить ведущий '+'
			SKIP_WHITE_SPACE = 1 << 8     // Пропускать пробелы
		};

		/**
		 * @brief Результат разбора строки
		 *
		 * @tparam UC тип символа
		 */
		template <typename UC>
		struct result_t {
			const UC * ptr; // Указатель сразу за разобранным числом
			errc ec;        // Код ошибки

			/**
			 * @brief Проверяет успешность разбора
			 *
			 * @return true при отсутствии ошибки
			 */
			constexpr explicit operator bool() const noexcept {
				return (ec == errc());
			}
		};

		/**
		 * @brief Псевдоним результата для char
		 *
		 */
		using Result = result_t <char>;

		/**
		 * @brief Опции разбора числа
		 *
		 * @tparam UC тип символа
		 */
		template <typename UC>
		struct options_t {
			format_t format;   // Допустимые форматы
			UC decimalPoint;   // Символ десятичной точки
			int base;          // Основание для целых

			/**
			 * @brief Конструктор
			 *
			 * @param fmt формат
			 * @param dot десятичная точка
			 * @param b   основание
			 */
			constexpr explicit options_t(const format_t fmt = format_t::GENERAL, const UC dot = UC('.'), const int b = 10) noexcept :
			 format(fmt), decimalPoint(dot), base(b) {}
		};

		/**
		 * @brief Псевдоним опций для char
		 *
		 */
		using Options = options_t <char>;

		/**
		 * @brief Проверяет выполнение constexpr C++20
		 *
		 * @return true, если вычисление выполняется в constexpr-контексте
		 */
		AWH_FLOAT_INLINE constexpr bool cpp20AndInConstexpr() noexcept {
			#if AWH_FLOAT_HAS_IS_CONSTANT_EVALUATED
				return is_constant_evaluated();
			#else
				return false;
			#endif
		}

		/**
		 * @brief Признак поддерживаемого типа с плавающей точкой
		 *
		 * @tparam T проверяемый тип
		 */
		template <typename T>
		struct IsSupportedFloat : integral_constant <bool,
			is_same <T, double>::value || is_same <T, float>::value
			#ifdef __STDCPP_FLOAT64_T__
				|| is_same <T, float64_t>::value
			#endif
			#ifdef __STDCPP_FLOAT32_T__
				|| is_same <T, float32_t>::value
			#endif
			#ifdef __STDCPP_FLOAT16_T__
				|| is_same <T, float16_t>::value
			#endif
			#ifdef __STDCPP_BFLOAT16_T__
				|| is_same <T, bfloat16_t>::value
			#endif
		> {};

		/**
		 * @brief Эквивалентный беззнаковый целочисленный тип той же ширины
		 *
		 * @tparam T исходный тип
		 */
		template <typename T>
		using equivUint_t = typename conditional <
			sizeof(T) == 1, uint8_t,
			typename conditional <
				sizeof(T) == 2, uint16_t,
				typename conditional <sizeof(T) == 4, uint32_t, uint64_t>::type
			>::type
		>::type;

		/**
		 * @brief Признак поддерживаемого целого типа
		 *
		 * @tparam T проверяемый тип
		 */
		template <typename T>
		struct IsSupportedInteger : is_integral <T> {};

		/**
		 * @brief Признак поддерживаемого символьного типа
		 *
		 * @tparam UC проверяемый тип
		 */
		template <typename UC>
		struct IsSupportedChar : integral_constant <bool,
			is_same <UC, char>::value ||
			is_same <UC, wchar_t>::value ||
			is_same <UC, char16_t>::value ||
			is_same <UC, char32_t>::value
			#ifdef __cpp_char8_t
				|| is_same <UC, char8_t>::value
			#endif
		> {};

		/**
		 * @brief Сравнивает ASCII-строки без учёта регистра
		 *
		 * @tparam UC тип символа
		 * @param actualMixedcase   фактическая строка
		 * @param expectedLowercase ожидаемая строка в нижнем регистре
		 * @param length            длина сравнения
		 * @return                  true при совпадении
		 */
		template <typename UC>
		inline constexpr bool strncasecmpAscii(const UC * actualMixedcase, const UC * expectedLowercase, const size_t length) noexcept {
			for(size_t i = 0; i < length; ++i){
				const UC actual = actualMixedcase[i];
				if(((actual < 256) ? (actual | 32) : actual) != expectedLowercase[i])
					return false;
			}
			return true;
		}

		/**
		 * @brief Непрерывный диапазон памяти
		 *
		 * @tparam T тип элемента
		 */
		template <typename T>
		struct span {
			const T * ptr;  // Указатель на начало
			size_t length;  // Длина диапазона

			/**
			 * @brief Конструктор
			 *
			 * @param pointer указатель
			 * @param len     длина
			 */
			constexpr span(const T * pointer, const size_t len) noexcept : ptr(pointer), length(len) {}

			/**
			 * @brief Конструктор по умолчанию
			 *
			 */
			constexpr span() noexcept : ptr(nullptr), length(0) {}

			/**
			 * @brief Возвращает длину диапазона
			 *
			 * @return длина
			 */
			constexpr size_t len() const noexcept {
				return length;
			}

			/**
			 * @brief Доступ к элементу
			 *
			 * @param index индекс
			 * @return      элемент
			 */
			constexpr const T & operator [] (const size_t index) const noexcept {
				AWH_FLOAT_DEBUG_ASSERT(index < length);
				return ptr[index];
			}
		};

		/**
		 * @brief Пара 64-битных слов (low/high)
		 *
		 */
		struct value128 {
			uint64_t low;  // Младшие 64 бита
			uint64_t high; // Старшие 64 бита

			/**
			 * @brief Конструктор
			 *
			 * @param lowValue  младшая часть
			 * @param highValue старшая часть
			 */
			constexpr value128(const uint64_t lowValue, const uint64_t highValue) noexcept : low(lowValue), high(highValue) {}

			/**
			 * @brief Конструктор по умолчанию
			 *
			 */
			constexpr value128() noexcept : low(0), high(0) {}
		};

		/**
		 * @brief Универсальная реализация подсчёта ведущих нулей
		 *
		 * @param inputNum исходное значение
		 * @param lastBit  накопленный сдвиг
		 * @return         число ведущих нулей
		 */
		AWH_FLOAT_INLINE constexpr int leadingZeroesGeneric(uint64_t inputNum, int lastBit = 0) noexcept {
			if(inputNum & uint64_t(0xffffffff00000000ULL)){
				inputNum >>= 32;
				lastBit |= 32;
			}
			if(inputNum & uint64_t(0xffff0000ULL)){
				inputNum >>= 16;
				lastBit |= 16;
			}
			if(inputNum & uint64_t(0xff00ULL)){
				inputNum >>= 8;
				lastBit |= 8;
			}
			if(inputNum & uint64_t(0xf0ULL)){
				inputNum >>= 4;
				lastBit |= 4;
			}
			if(inputNum & uint64_t(0xcULL)){
				inputNum >>= 2;
				lastBit |= 2;
			}
			if(inputNum & uint64_t(0x2ULL))
				lastBit |= 1;
			return 63 - lastBit;
		}

		/**
		 * @brief Подсчитывает ведущие нули в ненулевом uint64_t
		 *
		 * @param inputNum исходное значение (должно быть > 0)
		 * @return         число ведущих нулей
		 */
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 int leadingZeroes(const uint64_t inputNum) noexcept {
			assert(inputNum > 0);
			if(cpp20AndInConstexpr())
				return leadingZeroesGeneric(inputNum);
			#ifdef AWH_FLOAT_VISUAL_STUDIO
				#if defined(_M_X64) || defined(_M_ARM64)
					unsigned long leadingZero = 0;
					_BitScanReverse64(&leadingZero, inputNum);
					return int(63 - leadingZero);
				#else
					return leadingZeroesGeneric(inputNum);
				#endif
			#else
				return __builtin_clzll(inputNum);
			#endif
		}

		/**
		 * @brief Умножает два uint32_t в uint64_t
		 *
		 * @param x множитель
		 * @param y множитель
		 * @return  произведение
		 */
		AWH_FLOAT_INLINE constexpr uint64_t emulu(const uint32_t x, const uint32_t y) noexcept {
			return x * uint64_t(y);
		}

		/**
		 * @brief Универсальное 128-битное умножение uint64_t
		 *
		 * @param ab  множитель
		 * @param cd  множитель
		 * @param hi  старшие 64 бита результата
		 * @return    младшие 64 бита результата
		 */
		AWH_FLOAT_INLINE constexpr uint64_t umul128Generic(const uint64_t ab, const uint64_t cd, uint64_t * hi) noexcept {
			const uint64_t ad = emulu(uint32_t(ab >> 32), uint32_t(cd));
			const uint64_t bd = emulu(uint32_t(ab), uint32_t(cd));
			const uint64_t adbc = ad + emulu(uint32_t(ab), uint32_t(cd >> 32));
			const uint64_t adbcCarry = uint64_t(adbc < ad);
			const uint64_t lo = bd + (adbc << 32);
			* hi = emulu(uint32_t(ab >> 32), uint32_t(cd >> 32)) + (adbc >> 32) + (adbcCarry << 32) + uint64_t(lo < bd);
			return lo;
		}

		#ifdef AWH_FLOAT_32BIT
			#if !defined(__MINGW64__)
				/**
				 * @brief Обёртка umul128 для 32-битных платформ
				 *
				 */
				AWH_FLOAT_INLINE constexpr uint64_t _umul128(const uint64_t ab, const uint64_t cd, uint64_t * hi) noexcept {
					return umul128Generic(ab, cd, hi);
				}
			#endif
		#endif

		/**
		 * @brief Полное 128-битное умножение двух uint64_t
		 *
		 * @param a множитель
		 * @param b множитель
		 * @return  пара low/high
		 */
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 value128 fullMultiplication(const uint64_t a, const uint64_t b) noexcept {
			if(cpp20AndInConstexpr()){
				value128 answer;
				answer.low = umul128Generic(a, b, &answer.high);
				return answer;
			}
			value128 answer;
			#if defined(_M_ARM64) && !defined(__MINGW32__)
				answer.high = __umulh(a, b);
				answer.low = a * b;
			#elif defined(AWH_FLOAT_32BIT) || (defined(_WIN64) && !defined(__clang__) && !defined(_M_ARM64))
				answer.low = _umul128(a, b, &answer.high);
			#elif defined(AWH_FLOAT_64BIT) && defined(__SIZEOF_INT128__)
				const __uint128_t r = (__uint128_t(a) * b);
				answer.low = uint64_t(r);
				answer.high = uint64_t(r >> 64);
			#else
				answer.low = umul128Generic(a, b, &answer.high);
			#endif
			return answer;
		}

		/**
		 * @brief Скорректированная мантисса двоичного представления
		 *
		 */
		struct AdjustedMantissa {
			uint64_t mantissa{0}; // Мантисса
			int32_t power2{0};    // Степень двойки; отрицательное — невалидный результат

			/**
			 * @brief Конструктор по умолчанию
			 *
			 */
			AdjustedMantissa() = default;

			/**
			 * @brief Оператор сравнения на равенство
			 *
			 */
			constexpr bool operator == (const AdjustedMantissa & other) const noexcept {
				return ((mantissa == other.mantissa) && (power2 == other.power2));
			}

			/**
			 * @brief Оператор сравнения на неравенство
			 *
			 */
			constexpr bool operator != (const AdjustedMantissa & other) const noexcept {
				return ((mantissa != other.mantissa) || (power2 != other.power2));
			}
		};

		/**
		 * @brief Смещение для невалидной AdjustedMantissa
		 *
		 */
		constexpr int32_t invalidAmBias = -0x8000;

		/**
		 * @brief Константа 5^5 для таблиц maxMantissa
		 *
		 */
		constexpr uint64_t constant55555 = 5 * 5 * 5 * 5 * 5;

		/**
		 * @brief Предварительное объявление таблиц двоичного формата
		 *
		 * @tparam T тип float
		 * @tparam U фиктивный параметр
		 */
		template <typename T, typename U = void>
		struct BinaryFormatTables;

		/**
		 * @brief Параметры двоичного формата IEEE float/double
		 *
		 * @tparam T тип с плавающей точкой
		 */
		template <typename T>
		struct BinaryFormat : BinaryFormatTables <T> {
			using equiv_uint = equivUint_t <T>;

			static constexpr int mantissaExplicitBits();
			static constexpr int minimumExponent();
			static constexpr int infinitePower();
			static constexpr int signIndex();
			static constexpr int minExponentFastPath();
			static constexpr int maxExponentFastPath();
			static constexpr int maxExponentRoundToEven();
			static constexpr int minExponentRoundToEven();
			static constexpr uint64_t maxMantissaFastPath(int64_t power);
			static constexpr uint64_t maxMantissaFastPath();
			static constexpr int largestPowerOfTen();
			static constexpr int smallestPowerOfTen();
			static constexpr T exactPowerOfTen(int64_t power);
			static constexpr size_t maxDigits();
			static constexpr equiv_uint exponentMask();
			static constexpr equiv_uint mantissaMask();
			static constexpr equiv_uint hiddenBitMask();
		};

		/**
		 * @brief Таблицы для double
		 *
		 * @tparam U фиктивный параметр
		 */
		template <typename U>
		struct BinaryFormatTables <double, U> {
			static constexpr double powersOfTen[] = {
			1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,  1e8,  1e9,  1e10, 1e11,
						1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22
			};
			static constexpr uint64_t maxMantissa[] = {
			0x20000000000000,
						0x20000000000000 / 5,
						0x20000000000000 / (5 * 5),
						0x20000000000000 / (5 * 5 * 5),
						0x20000000000000 / (5 * 5 * 5 * 5),
						0x20000000000000 / (constant55555),
						0x20000000000000 / (constant55555 * 5),
						0x20000000000000 / (constant55555 * 5 * 5),
						0x20000000000000 / (constant55555 * 5 * 5 * 5),
						0x20000000000000 / (constant55555 * 5 * 5 * 5 * 5),
						0x20000000000000 / (constant55555 * constant55555),
						0x20000000000000 / (constant55555 * constant55555 * 5),
						0x20000000000000 / (constant55555 * constant55555 * 5 * 5),
						0x20000000000000 / (constant55555 * constant55555 * 5 * 5 * 5),
						0x20000000000000 / (constant55555 * constant55555 * constant55555),
						0x20000000000000 / (constant55555 * constant55555 * constant55555 * 5),
						0x20000000000000 /
								(constant55555 * constant55555 * constant55555 * 5 * 5),
						0x20000000000000 /
								(constant55555 * constant55555 * constant55555 * 5 * 5 * 5),
						0x20000000000000 /
								(constant55555 * constant55555 * constant55555 * 5 * 5 * 5 * 5),
						0x20000000000000 /
								(constant55555 * constant55555 * constant55555 * constant55555),
						0x20000000000000 / (constant55555 * constant55555 * constant55555 *
																constant55555 * 5),
						0x20000000000000 / (constant55555 * constant55555 * constant55555 *
																constant55555 * 5 * 5),
						0x20000000000000 / (constant55555 * constant55555 * constant55555 *
																constant55555 * 5 * 5 * 5),
						0x20000000000000 / (constant55555 * constant55555 * constant55555 *
																constant55555 * 5 * 5 * 5 * 5)
			};
		};


		/**
		 * @brief Таблицы для float
		 *
		 * @tparam U фиктивный параметр
		 */
		template <typename U>
		struct BinaryFormatTables <float, U> {
			static constexpr float powersOfTen[] = {
			1e0f, 1e1f, 1e2f, 1e3f, 1e4f, 1e5f,
																									1e6f, 1e7f, 1e8f, 1e9f, 1e10f
			};
			static constexpr uint64_t maxMantissa[] = {
			0x1000000,
						0x1000000 / 5,
						0x1000000 / (5 * 5),
						0x1000000 / (5 * 5 * 5),
						0x1000000 / (5 * 5 * 5 * 5),
						0x1000000 / (constant55555),
						0x1000000 / (constant55555 * 5),
						0x1000000 / (constant55555 * 5 * 5),
						0x1000000 / (constant55555 * 5 * 5 * 5),
						0x1000000 / (constant55555 * 5 * 5 * 5 * 5),
						0x1000000 / (constant55555 * constant55555),
						0x1000000 / (constant55555 * constant55555 * 5)
			};
		};


		template <> inline constexpr int BinaryFormat <double>::minExponentFastPath() {
			#if (FLT_EVAL_METHOD != 1) && (FLT_EVAL_METHOD != 0)
				return 0;
			#else
				return -22;
			#endif
		}
		template <> inline constexpr int BinaryFormat <float>::minExponentFastPath() {
			#if (FLT_EVAL_METHOD != 1) && (FLT_EVAL_METHOD != 0)
				return 0;
			#else
				return -10;
			#endif
		}
		template <> inline constexpr int BinaryFormat <double>::mantissaExplicitBits() { return 52; }
		template <> inline constexpr int BinaryFormat <float>::mantissaExplicitBits() { return 23; }
		template <> inline constexpr int BinaryFormat <double>::maxExponentRoundToEven() { return 23; }
		template <> inline constexpr int BinaryFormat <float>::maxExponentRoundToEven() { return 10; }
		template <> inline constexpr int BinaryFormat <double>::minExponentRoundToEven() { return -4; }
		template <> inline constexpr int BinaryFormat <float>::minExponentRoundToEven() { return -17; }
		template <> inline constexpr int BinaryFormat <double>::minimumExponent() { return -1023; }
		template <> inline constexpr int BinaryFormat <float>::minimumExponent() { return -127; }
		template <> inline constexpr int BinaryFormat <double>::infinitePower() { return 0x7FF; }
		template <> inline constexpr int BinaryFormat <float>::infinitePower() { return 0xFF; }
		template <> inline constexpr int BinaryFormat <double>::signIndex() { return 63; }
		template <> inline constexpr int BinaryFormat <float>::signIndex() { return 31; }
		template <> inline constexpr int BinaryFormat <double>::maxExponentFastPath() { return 22; }
		template <> inline constexpr int BinaryFormat <float>::maxExponentFastPath() { return 10; }
		template <> inline constexpr uint64_t BinaryFormat <double>::maxMantissaFastPath() { return uint64_t(2) << mantissaExplicitBits(); }
		template <> inline constexpr uint64_t BinaryFormat <float>::maxMantissaFastPath() { return uint64_t(2) << mantissaExplicitBits(); }

		#ifdef __STDCPP_FLOAT16_T__
			/**
			 * @brief Таблицы для float16_t
			 *
			 * @tparam U фиктивный параметр
			 */
			template <typename U>
			struct BinaryFormatTables <float16_t, U> {
				static constexpr float16_t powersOfTen[] = {
				1e0f16, 1e1f16, 1e2f16,
																														 1e3f16, 1e4f16
				};
				static constexpr uint64_t maxMantissa[] = {
				0x800,
																											0x800 / 5,
																											0x800 / (5 * 5),
																											0x800 / (5 * 5 * 5),
																											0x800 / (5 * 5 * 5 * 5),
																											0x800 / (constant55555)
				};
			};


			template <> inline constexpr float16_t BinaryFormat <float16_t>::exactPowerOfTen(const int64_t power) {
				return ((void) powersOfTen[0], powersOfTen[power]);
			}
			template <> inline constexpr BinaryFormat <float16_t>::equiv_uint BinaryFormat <float16_t>::exponentMask() { return 0x7C00; }
			template <> inline constexpr BinaryFormat <float16_t>::equiv_uint BinaryFormat <float16_t>::mantissaMask() { return 0x03FF; }
			template <> inline constexpr BinaryFormat <float16_t>::equiv_uint BinaryFormat <float16_t>::hiddenBitMask() { return 0x0400; }
			template <> inline constexpr int BinaryFormat <float16_t>::maxExponentFastPath() { return 4; }
			template <> inline constexpr int BinaryFormat <float16_t>::mantissaExplicitBits() { return 10; }
			template <> inline constexpr uint64_t BinaryFormat <float16_t>::maxMantissaFastPath() { return uint64_t(2) << mantissaExplicitBits(); }
			template <> inline constexpr uint64_t BinaryFormat <float16_t>::maxMantissaFastPath(const int64_t power) {
				return ((void) maxMantissa[0], maxMantissa[power]);
			}
			template <> inline constexpr int BinaryFormat <float16_t>::minExponentFastPath() { return 0; }
			template <> inline constexpr int BinaryFormat <float16_t>::maxExponentRoundToEven() { return 5; }
			template <> inline constexpr int BinaryFormat <float16_t>::minExponentRoundToEven() { return -22; }
			template <> inline constexpr int BinaryFormat <float16_t>::minimumExponent() { return -15; }
			template <> inline constexpr int BinaryFormat <float16_t>::infinitePower() { return 0x1F; }
			template <> inline constexpr int BinaryFormat <float16_t>::signIndex() { return 15; }
			template <> inline constexpr int BinaryFormat <float16_t>::largestPowerOfTen() { return 4; }
			template <> inline constexpr int BinaryFormat <float16_t>::smallestPowerOfTen() { return -27; }
			template <> inline constexpr size_t BinaryFormat <float16_t>::maxDigits() { return 22; }
		#endif

		#ifdef __STDCPP_BFLOAT16_T__
			/**
			 * @brief Таблицы для bfloat16_t
			 *
			 * @tparam U фиктивный параметр
			 */
			template <typename U>
			struct BinaryFormatTables <bfloat16_t, U> {
				static constexpr bfloat16_t powersOfTen[] = {
				1e0bf16, 1e1bf16, 1e2bf16,
																															1e3bf16
				};
				static constexpr uint64_t maxMantissa[] = {
				0x100, 0x100 / 5, 0x100 / (5 * 5),
																											0x100 / (5 * 5 * 5),
																											0x100 / (5 * 5 * 5 * 5)
				};
			};


			template <> inline constexpr bfloat16_t BinaryFormat <bfloat16_t>::exactPowerOfTen(const int64_t power) {
				return ((void) powersOfTen[0], powersOfTen[power]);
			}
			template <> inline constexpr int BinaryFormat <bfloat16_t>::maxExponentFastPath() { return 3; }
			template <> inline constexpr BinaryFormat <bfloat16_t>::equiv_uint BinaryFormat <bfloat16_t>::exponentMask() { return 0x7F80; }
			template <> inline constexpr BinaryFormat <bfloat16_t>::equiv_uint BinaryFormat <bfloat16_t>::mantissaMask() { return 0x007F; }
			template <> inline constexpr BinaryFormat <bfloat16_t>::equiv_uint BinaryFormat <bfloat16_t>::hiddenBitMask() { return 0x0080; }
			template <> inline constexpr int BinaryFormat <bfloat16_t>::mantissaExplicitBits() { return 7; }
			template <> inline constexpr uint64_t BinaryFormat <bfloat16_t>::maxMantissaFastPath() { return uint64_t(2) << mantissaExplicitBits(); }
			template <> inline constexpr uint64_t BinaryFormat <bfloat16_t>::maxMantissaFastPath(const int64_t power) {
				return ((void) maxMantissa[0], maxMantissa[power]);
			}
			template <> inline constexpr int BinaryFormat <bfloat16_t>::minExponentFastPath() { return 0; }
			template <> inline constexpr int BinaryFormat <bfloat16_t>::maxExponentRoundToEven() { return 3; }
			template <> inline constexpr int BinaryFormat <bfloat16_t>::minExponentRoundToEven() { return -24; }
			template <> inline constexpr int BinaryFormat <bfloat16_t>::minimumExponent() { return -127; }
			template <> inline constexpr int BinaryFormat <bfloat16_t>::infinitePower() { return 0xFF; }
			template <> inline constexpr int BinaryFormat <bfloat16_t>::signIndex() { return 15; }
			template <> inline constexpr int BinaryFormat <bfloat16_t>::largestPowerOfTen() { return 38; }
			template <> inline constexpr int BinaryFormat <bfloat16_t>::smallestPowerOfTen() { return -60; }
			template <> inline constexpr size_t BinaryFormat <bfloat16_t>::maxDigits() { return 98; }
		#endif

		template <> inline constexpr uint64_t BinaryFormat <double>::maxMantissaFastPath(const int64_t power) {
			return ((void) maxMantissa[0], maxMantissa[power]);
		}
		template <> inline constexpr uint64_t BinaryFormat <float>::maxMantissaFastPath(const int64_t power) {
			return ((void) maxMantissa[0], maxMantissa[power]);
		}
		template <> inline constexpr double BinaryFormat <double>::exactPowerOfTen(const int64_t power) {
			return ((void) powersOfTen[0], powersOfTen[power]);
		}
		template <> inline constexpr float BinaryFormat <float>::exactPowerOfTen(const int64_t power) {
			return ((void) powersOfTen[0], powersOfTen[power]);
		}
		template <> inline constexpr int BinaryFormat <double>::largestPowerOfTen() { return 308; }
		template <> inline constexpr int BinaryFormat <float>::largestPowerOfTen() { return 38; }
		template <> inline constexpr int BinaryFormat <double>::smallestPowerOfTen() { return -342; }
		template <> inline constexpr int BinaryFormat <float>::smallestPowerOfTen() { return -64; }
		template <> inline constexpr size_t BinaryFormat <double>::maxDigits() { return 769; }
		template <> inline constexpr size_t BinaryFormat <float>::maxDigits() { return 114; }
		template <> inline constexpr BinaryFormat <float>::equiv_uint BinaryFormat <float>::exponentMask() { return 0x7F800000; }
		template <> inline constexpr BinaryFormat <double>::equiv_uint BinaryFormat <double>::exponentMask() { return 0x7FF0000000000000ULL; }
		template <> inline constexpr BinaryFormat <float>::equiv_uint BinaryFormat <float>::mantissaMask() { return 0x007FFFFF; }
		template <> inline constexpr BinaryFormat <double>::equiv_uint BinaryFormat <double>::mantissaMask() { return 0x000FFFFFFFFFFFFFULL; }
		template <> inline constexpr BinaryFormat <float>::equiv_uint BinaryFormat <float>::hiddenBitMask() { return 0x00800000; }
		template <> inline constexpr BinaryFormat <double>::equiv_uint BinaryFormat <double>::hiddenBitMask() { return 0x0010000000000000ULL; }

		/**
		 * @brief Собирает значение float/double из знака и AdjustedMantissa
		 *
		 * @tparam T тип результата
		 * @param negative знак
		 * @param am       мантисса и экспонент
		 * @param value    ссылка на результат
		 */
		template <typename T>
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 void toFloat(const bool negative, const AdjustedMantissa am, T & value) noexcept {
			using equiv_uint = equivUint_t <T>;
			equiv_uint word = equiv_uint(am.mantissa);
			word = equiv_uint(word | (equiv_uint(am.power2) << BinaryFormat <T>::mantissaExplicitBits()));
			word = equiv_uint(word | (equiv_uint(negative) << BinaryFormat <T>::signIndex()));
			#if AWH_FLOAT_HAS_BIT_CAST
				value = bit_cast <T> (word);
			#else
				::memcpy(&value, &word, sizeof(T));
			#endif
		}

		/**
		 * @brief LUT пробельных символов ASCII
		 *
		 * @tparam unused фиктивный параметр
		 */
		template <typename = void>
		struct spaceLut {
			static constexpr bool value[] = {
			0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
			};
		};


		/**
		 * @brief Проверяет, является ли символ пробельным
		 *
		 * @tparam UC тип символа
		 * @param c   символ
		 * @return    true для пробельных ASCII-символов
		 */
		template <typename UC>
		constexpr bool isSpace(const UC c) noexcept {
			return ((c < 256) && spaceLut <>::value[uint8_t(c)]);
		}

		/**
		 * @brief Маска нулей для сравнения блоков символов
		 *
		 * @tparam UC тип символа
		 */
		template <typename UC>
		static constexpr uint64_t intCmpZeros() noexcept {
			static_assert((sizeof(UC) == 1) || (sizeof(UC) == 2) || (sizeof(UC) == 4), "Unsupported character size");
			return (sizeof(UC) == 1) ? 0x3030303030303030ULL
				: (sizeof(UC) == 2)
					? ((uint64_t(UC('0')) << 48) | (uint64_t(UC('0')) << 32) | (uint64_t(UC('0')) << 16) | UC('0'))
					: ((uint64_t(UC('0')) << 32) | UC('0'));
		}

		/**
		 * @brief Число символов в блоке uint64_t
		 *
		 * @tparam UC тип символа
		 */
		template <typename UC>
		static constexpr int intCmpLen() noexcept {
			return int(sizeof(uint64_t) / sizeof(UC));
		}

		template <typename UC> constexpr const UC * strConstNan();
		template <> constexpr const char * strConstNan <char> () { return "nan"; }
		template <> constexpr const wchar_t * strConstNan <wchar_t> () { return L"nan"; }
		template <> constexpr const char16_t * strConstNan <char16_t> () { return u"nan"; }
		template <> constexpr const char32_t * strConstNan <char32_t> () { return U"nan"; }
		#ifdef __cpp_char8_t
			template <> constexpr const char8_t * strConstNan <char8_t> () { return u8"nan"; }
		#endif

		template <typename UC> constexpr const UC * strConstInf();
		template <> constexpr const char * strConstInf <char> () { return "infinity"; }
		template <> constexpr const wchar_t * strConstInf <wchar_t> () { return L"infinity"; }
		template <> constexpr const char16_t * strConstInf <char16_t> () { return u"infinity"; }
		template <> constexpr const char32_t * strConstInf <char32_t> () { return U"infinity"; }
		#ifdef __cpp_char8_t
			template <> constexpr const char8_t * strConstInf <char8_t> () { return u8"infinity"; }
		#endif

		/**
		 * @brief LUT для разбора цифр в произвольном основании
		 *
		 * @tparam unused фиктивный параметр
		 */
		template <typename = void>
		struct intLuts {
			static constexpr uint8_t chdigit[] = {
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
						255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
						255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
						255, 255, 255, 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   255, 255,
						255, 255, 255, 255, 255, 10,  11,  12,  13,  14,  15,  16,  17,  18,  19,
						20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,
						35,  255, 255, 255, 255, 255, 255, 10,  11,  12,  13,  14,  15,  16,  17,
						18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,
						33,  34,  35,  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
						255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
						255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
						255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
						255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
						255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
						255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
						255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
						255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
						255
			};
			static constexpr size_t maxDigitsU64Table[] = {
			64, 41, 32, 28, 25, 23, 22, 21, 20, 19, 18, 18, 17, 17, 16, 16, 16, 16,
						15, 15, 15, 15, 14, 14, 14, 14, 14, 14, 14, 13, 13, 13, 13, 13, 13
			};
			static constexpr uint64_t minSafeU64[] = {
			9223372036854775808ull,  12157665459056928801ull, 4611686018427387904,
						7450580596923828125,     4738381338321616896,     3909821048582988049,
						9223372036854775808ull,  12157665459056928801ull, 10000000000000000000ull,
						5559917313492231481,     2218611106740436992,     8650415919381337933,
						2177953337809371136,     6568408355712890625,     1152921504606846976,
						2862423051509815793,     6746640616477458432,     15181127029874798299ull,
						1638400000000000000,     3243919932521508681,     6221821273427820544,
						11592836324538749809ull, 876488338465357824,      1490116119384765625,
						2481152873203736576,     4052555153018976267,     6502111422497947648,
						10260628712958602189ull, 15943230000000000000ull, 787662783788549761,
						1152921504606846976,     1667889514952984961,     2386420683693101056,
						3379220508056640625,     4738381338321616896
			};
		};


		/**
		 * @brief Преобразует символ в цифру для заданного основания
		 *
		 * @tparam UC тип символа
		 * @param c   символ
		 * @return    значение цифры или 255 при ошибке
		 */
		template <typename UC>
		AWH_FLOAT_INLINE constexpr uint8_t chToDigit(const UC c) noexcept {
			using UnsignedUC = typename make_unsigned <UC>::type;
			return intLuts <>::chdigit[static_cast <unsigned char> (
				static_cast <UnsignedUC> (c) &
				static_cast <UnsignedUC> (-((static_cast <UnsignedUC> (c) & ~0xFFull) == 0))
			)];
		}

		/**
		 * @brief Максимальное число цифр uint64_t в заданном основании
		 *
		 * @param base основание (2..36)
		 */
		AWH_FLOAT_INLINE constexpr size_t maxDigitsU64(const int base) noexcept {
			return intLuts <>::maxDigitsU64Table[base - 2];
		}

		/**
		 * @brief Нижняя граница безопасного uint64_t при maxDigitsU64 цифрах
		 *
		 * @param base основание (2..36)
		 */
		AWH_FLOAT_INLINE constexpr uint64_t minSafeU64(const int base) noexcept {
			return intLuts <>::minSafeU64[base - 2];
		}

		static_assert(is_same <equivUint_t <double>, uint64_t>::value, "equiv_uint should be uint64_t for double");
		static_assert(numeric_limits <double>::is_iec559, "double must fulfill IEC 559 (IEEE 754)");
		static_assert(is_same <equivUint_t <float>, uint32_t>::value, "equiv_uint should be uint32_t for float");
		static_assert(numeric_limits <float>::is_iec559, "float must fulfill IEC 559 (IEEE 754)");

		#ifdef __STDCPP_FLOAT64_T__
			static_assert(is_same <equivUint_t <float64_t>, uint64_t>::value, "equiv_uint should be uint64_t for float64_t");
			static_assert(numeric_limits <float64_t>::is_iec559, "float64_t must fulfill IEC 559 (IEEE 754)");
			template <>
			struct BinaryFormat <float64_t> : public BinaryFormat <double> {};
		#endif
		#ifdef __STDCPP_FLOAT32_T__
			static_assert(is_same <equivUint_t <float32_t>, uint32_t>::value, "equiv_uint should be uint32_t for float32_t");
			static_assert(numeric_limits <float32_t>::is_iec559, "float32_t must fulfill IEC 559 (IEEE 754)");
			template <>
			struct BinaryFormat <float32_t> : public BinaryFormat <float> {};
		#endif
		#ifdef __STDCPP_FLOAT16_T__
			static_assert(is_same <BinaryFormat <float16_t>::equiv_uint, uint16_t>::value, "equiv_uint should be uint16_t for float16_t");
			static_assert(numeric_limits <float16_t>::is_iec559, "float16_t must fulfill IEC 559 (IEEE 754)");
		#endif
		#ifdef __STDCPP_BFLOAT16_T__
			static_assert(is_same <BinaryFormat <bfloat16_t>::equiv_uint, uint16_t>::value, "equiv_uint should be uint16_t for bfloat16_t");
			static_assert(numeric_limits <bfloat16_t>::is_iec559, "bfloat16_t must fulfill IEC 559 (IEEE 754)");
		#endif

		/**
		 * @brief Побитовые операторы для format_t
		 *
		 */
		constexpr format_t operator ~ (const format_t rhs) noexcept {
			using intType = underlying_type <format_t>::type;
			return static_cast <format_t> (~static_cast <intType> (rhs));
		}
		constexpr format_t operator & (const format_t lhs, const format_t rhs) noexcept {
			using intType = underlying_type <format_t>::type;
			return static_cast <format_t> (static_cast <intType> (lhs) & static_cast <intType> (rhs));
		}
		constexpr format_t operator | (const format_t lhs, const format_t rhs) noexcept {
			using intType = underlying_type <format_t>::type;
			return static_cast <format_t> (static_cast <intType> (lhs) | static_cast <intType> (rhs));
		}
		constexpr format_t operator ^ (const format_t lhs, const format_t rhs) noexcept {
			using intType = underlying_type <format_t>::type;
			return static_cast <format_t> (static_cast <intType> (lhs) ^ static_cast <intType> (rhs));
		}
		AWH_FLOAT_INLINE constexpr format_t & operator &= (format_t & lhs, const format_t rhs) noexcept {
			return lhs = (lhs & rhs);
		}
		AWH_FLOAT_INLINE constexpr format_t & operator |= (format_t & lhs, const format_t rhs) noexcept {
			return lhs = (lhs | rhs);
		}
		AWH_FLOAT_INLINE constexpr format_t & operator ^= (format_t & lhs, const format_t rhs) noexcept {
			return lhs = (lhs ^ rhs);
		}

		namespace detail {
			/**
			 * @brief Корректирует формат с учётом feature-макросов
			 *
			 * @param fmt исходный формат
			 * @return    скорректированный формат
			 */
			constexpr format_t adjustForFeatureMacros(const format_t fmt) noexcept {
				return fmt
					#ifdef AWH_FLOAT_ALLOWS_LEADING_PLUS
						| format_t::ALLOW_LEADING_PLUS
					#endif
					#ifdef AWH_FLOAT_SKIP_WHITE_SPACE
						| format_t::SKIP_WHITE_SPACE
					#endif
				;
			}
		} // namespace detail
	} // namespace floating
} // namespace awh

#endif // __AWH_FLOAT_COMMON__
