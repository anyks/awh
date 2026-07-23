/**
 * @file: ascii.hpp
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
#ifndef __AWH_FLOAT_ASCII__
#define __AWH_FLOAT_ASCII__

/**
 * Стандартные заголовочные файлы
 */
#include <cctype>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <limits>
#include <type_traits>

/**
 * Подключаем общие определения модуля
 */
#include "common.hpp"

/**
 * Подключаем SIMD-заголовки при наличии поддержки
 */
#ifdef AWH_FLOAT_SSE2
	#include <emmintrin.h>
#endif
#ifdef AWH_FLOAT_NEON
	#include <arm_neon.h>
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
		 * @brief Проверяет наличие SIMD-оптимизации для типа символа
		 *
		 * @tparam UC тип символа
		 * @return    true, если для UC доступна SIMD-оптимизация
		 */
		template <typename UC>
		AWH_FLOAT_INLINE constexpr bool hasSimdOpt() noexcept {
			#ifdef AWH_FLOAT_HAS_SIMD
				// SIMD-путь поддерживается для char16_t
				return is_same <UC, char16_t>::value;
			#else
				// SIMD недоступен
				return false;
			#endif
		}

		/**
		 * @brief Проверяет, является ли символ десятичной цифрой
		 *
		 * @tparam UC тип символа
		 * @param c   проверяемый символ
		 * @return    true, если символ в диапазоне '0'..'9'
		 */
		template <typename UC>
		AWH_FLOAT_INLINE constexpr bool isInteger(const UC c) noexcept {
			// Проверяем попадание символа в диапазон десятичных цифр
			return !((c > UC('9')) || (c < UC('0')));
		}

		/**
		 * @brief Меняет порядок байт в 64-битном значении
		 *
		 * @param val исходное значение
		 * @return    значение с переставленными байтами
		 */
		AWH_FLOAT_INLINE constexpr uint64_t byteswap(const uint64_t val) noexcept {
			// Выполняем перестановку байт
			return (
				((val & 0xFF00000000000000ULL) >> 56) |
				((val & 0x00FF000000000000ULL) >> 40) |
				((val & 0x0000FF0000000000ULL) >> 24) |
				((val & 0x000000FF00000000ULL) >> 8)  |
				((val & 0x00000000FF000000ULL) << 8)  |
				((val & 0x0000000000FF0000ULL) << 24) |
				((val & 0x000000000000FF00ULL) << 40) |
				((val & 0x00000000000000FFULL) << 56)
			);
		}

		/**
		 * @brief Читает 8 символов в uint64_t
		 *
		 * @details Для типов шире char выполняется усечение до младшего байта.
		 *          На big-endian платформах результат приводится к little-endian.
		 *
		 * @tparam UC тип символа
		 * @param chars указатель на начало последовательности
		 * @return      упакованное 64-битное значение
		 */
		template <typename UC>
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 uint64_t read8ToU64(const UC * chars) noexcept {
			// В constexpr-контексте или для не-char читаем побайтово
			if(cpp20AndInConstexpr() || !is_same <UC, char>::value){
				// Инициализируем аккумулятор
				uint64_t val = 0;
				/**
				 * Упаковываем 8 символов в little-endian порядке
				 */
				for(int i = 0; i < 8; ++i){
					// Записываем младший байт символа
					val |= uint64_t(uint8_t(* chars)) << (i * 8);
					// Смещаем указатель
					++chars;
				}
				// Возвращаем результат
				return val;
			}
			// Читаем 8 байт одним блоком
			uint64_t val = 0;
			::memcpy(&val, chars, sizeof(uint64_t));
			#if AWH_FLOAT_IS_BIG_ENDIAN == 1
				// На big-endian приводим к little-endian порядку
				val = byteswap(val);
			#endif
			// Возвращаем результат
			return val;
		}

		#ifdef AWH_FLOAT_SSE2
			/**
			 * @brief Читает 8 символов char16_t из SSE-регистра в uint64_t
			 *
			 * @param data загруженные данные SSE
			 * @return     упакованное 64-битное значение
			 */
			AWH_FLOAT_INLINE uint64_t simdRead8ToU64(const __m128i data) noexcept {
				AWH_FLOAT_SIMD_DISABLE_WARNINGS
				// Упаковываем 16-битные элементы в 8-битные
				const __m128i packed = _mm_packus_epi16(data, data);
				#ifdef AWH_FLOAT_64BIT
					// Извлекаем младшие 64 бита
					return uint64_t(_mm_cvtsi128_si64(packed));
				#else
					// Для 32-битных платформ сохраняем через storel
					uint64_t value = 0;
					_mm_storel_epi64(reinterpret_cast <__m128i *> (&value), packed);
					// Возвращаем результат
					return value;
				#endif
				AWH_FLOAT_SIMD_RESTORE_WARNINGS
			}

			/**
			 * @brief Читает 8 символов char16_t через SSE в uint64_t
			 *
			 * @param chars указатель на начало последовательности
			 * @return      упакованное 64-битное значение
			 */
			AWH_FLOAT_INLINE uint64_t simdRead8ToU64(const char16_t * chars) noexcept {
				AWH_FLOAT_SIMD_DISABLE_WARNINGS
				// Загружаем 128 бит и упаковываем
				return simdRead8ToU64(_mm_loadu_si128(reinterpret_cast <const __m128i *> (chars)));
				AWH_FLOAT_SIMD_RESTORE_WARNINGS
			}
		#elif defined(AWH_FLOAT_NEON)
			/**
			 * @brief Читает 8 символов char16_t из NEON-регистра в uint64_t
			 *
			 * @param data загруженные данные NEON
			 * @return     упакованное 64-битное значение
			 */
			AWH_FLOAT_INLINE uint64_t simdRead8ToU64(const uint16x8_t data) noexcept {
				AWH_FLOAT_SIMD_DISABLE_WARNINGS
				// Сужаем 16-битные элементы до 8-битных
				const uint8x8_t packed = vmovn_u16(data);
				// Извлекаем 64-битное значение
				return vget_lane_u64(vreinterpret_u64_u8(packed), 0);
				AWH_FLOAT_SIMD_RESTORE_WARNINGS
			}

			/**
			 * @brief Читает 8 символов char16_t через NEON в uint64_t
			 *
			 * @param chars указатель на начало последовательности
			 * @return      упакованное 64-битное значение
			 */
			AWH_FLOAT_INLINE uint64_t simdRead8ToU64(const char16_t * chars) noexcept {
				AWH_FLOAT_SIMD_DISABLE_WARNINGS
				// Загружаем 8 значений uint16 и упаковываем
				return simdRead8ToU64(vld1q_u16(reinterpret_cast <const uint16_t *> (chars)));
				AWH_FLOAT_SIMD_RESTORE_WARNINGS
			}
		#endif

		/**
		 * @brief Заглушка SIMD-чтения для типов без SIMD-оптимизации
		 *
		 * @tparam UC тип символа
		 * @return    всегда 0
		 */
		#if defined(_MSC_VER) && (_MSC_VER <= 1900)
			template <typename UC>
		#else
			template <typename UC, enableIf_t <!hasSimdOpt <UC>()> = 0>
		#endif
		uint64_t simdRead8ToU64(const UC *) noexcept {
			// SIMD для данного типа недоступен
			return 0;
		}

		/**
		 * @brief Разбирает 8 цифр из упакованного uint64_t
		 *
		 * @param val упакованные ASCII-цифры
		 * @return    числовое значение 8 цифр
		 */
		AWH_FLOAT_INLINE constexpr uint32_t parseEightDigitsUnrolled(uint64_t val) noexcept {
			// Маска для выделения байтовых пар
			constexpr uint64_t mask = 0x000000FF000000FFULL;
			// Множители для свёртки разрядов
			constexpr uint64_t mul1 = 0x000F424000000064ULL; // 100 + (1000000ULL << 32)
			constexpr uint64_t mul2 = 0x0000271000000001ULL; // 1 + (10000ULL << 32)
			// Переводим ASCII '0'..'9' в числовые значения
			val -= 0x3030303030303030ULL;
			// Первая свёртка соседних разрядов
			val = (val * 10) + (val >> 8);
			// Финальная свёртка в 32-битное число
			val = (((val & mask) * mul1) + (((val >> 16) & mask) * mul2)) >> 32;
			// Возвращаем результат
			return uint32_t(val);
		}

		/**
		 * @brief Разбирает ровно 8 цифр из строки
		 *
		 * @tparam UC тип символа
		 * @param chars указатель на начало 8 цифр
		 * @return      числовое значение
		 */
		template <typename UC>
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 uint32_t parseEightDigitsUnrolled(const UC * chars) noexcept {
			// Без SIMD или в constexpr читаем скалярно
			if(cpp20AndInConstexpr() || !hasSimdOpt <UC>())
				return parseEightDigitsUnrolled(read8ToU64(chars));
			// Иначе используем SIMD-чтение
			return parseEightDigitsUnrolled(simdRead8ToU64(chars));
		}

		/**
		 * @brief Быстро проверяет, что в uint64_t упакованы 8 ASCII-цифр
		 *
		 * @param val упакованные символы
		 * @return    true, если все 8 байт — цифры
		 */
		AWH_FLOAT_INLINE constexpr bool isMadeOfEightDigitsFast(const uint64_t val) noexcept {
			// Проверяем, что каждый байт лежит в диапазоне '0'..'9'
			return !((((val + 0x4646464646464646ULL) | (val - 0x3030303030303030ULL)) & 0x8080808080808080ULL));
		}

		#ifdef AWH_FLOAT_HAS_SIMD
			/**
			 * @brief Пытается разобрать 8 цифр через SIMD и добавить к аккумулятору
			 *
			 * @param chars указатель на начало последовательности
			 * @param i     аккумулятор целого значения
			 * @return      true, если успешно разобраны 8 цифр
			 */
			AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 bool simdParseIfEightDigitsUnrolled(const char16_t * chars, uint64_t & i) noexcept {
				// В constexpr SIMD недоступен
				if(cpp20AndInConstexpr())
					return false;
				#ifdef AWH_FLOAT_SSE2
					AWH_FLOAT_SIMD_DISABLE_WARNINGS
					// Загружаем 8 символов char16_t
					const __m128i data = _mm_loadu_si128(reinterpret_cast <const __m128i *> (chars));
					// Проверка (x - '0') <= 9
					const __m128i t0 = _mm_add_epi16(data, _mm_set1_epi16(32720));
					const __m128i t1 = _mm_cmpgt_epi16(t0, _mm_set1_epi16(-32759));
					// Результат разбора
					bool result = false;
					// Если все символы — цифры, накапливаем значение
					if(_mm_movemask_epi8(t1) == 0){
						i = (i * 100000000ULL) + parseEightDigitsUnrolled(simdRead8ToU64(data));
						result = true;
					}
					AWH_FLOAT_SIMD_RESTORE_WARNINGS
					// Возвращаем результат
					return result;
				#elif defined(AWH_FLOAT_NEON)
					AWH_FLOAT_SIMD_DISABLE_WARNINGS
					// Загружаем 8 символов char16_t
					const uint16x8_t data = vld1q_u16(reinterpret_cast <const uint16_t *> (chars));
					// Проверка (x - '0') <= 9
					const uint16x8_t t0 = vsubq_u16(data, vmovq_n_u16('0'));
					const uint16x8_t mask = vcltq_u16(t0, vmovq_n_u16(static_cast <uint16_t> ('9' - '0' + 1)));
					// Результат разбора
					bool result = false;
					// Если все символы — цифры, накапливаем значение
					if(vminvq_u16(mask) == 0xFFFF){
						i = (i * 100000000ULL) + parseEightDigitsUnrolled(simdRead8ToU64(data));
						result = true;
					}
					AWH_FLOAT_SIMD_RESTORE_WARNINGS
					// Возвращаем результат
					return result;
				#else
					// SIMD-заголовки недоступны
					(void) chars;
					(void) i;
					return false;
				#endif
			}
		#endif

		/**
		 * @brief Заглушка SIMD-разбора для типов без SIMD-оптимизации
		 *
		 * @tparam UC тип символа
		 * @return    всегда false
		 */
		#if defined(_MSC_VER) && (_MSC_VER <= 1900)
			template <typename UC>
		#else
			template <typename UC, enableIf_t <!hasSimdOpt <UC>()> = 0>
		#endif
		bool simdParseIfEightDigitsUnrolled(const UC *, uint64_t &) noexcept {
			// SIMD для данного типа недоступен
			return false;
		}

		/**
		 * @brief Циклически разбирает блоки по 8 цифр для не-char типов
		 *
		 * @tparam UC тип символа
		 * @param p    текущая позиция в строке
		 * @param pend конец строки
		 * @param i    аккумулятор целого значения
		 */
		template <typename UC, enableIf_t <!is_same <UC, char>::value> = 0>
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 void loopParseIfEightDigits(const UC * & p, const UC * const pend, uint64_t & i) noexcept {
			// Без SIMD сразу выходим
			if(!hasSimdOpt <UC>())
				return;
			/**
			 *  Разбираем блоки по 8 цифр, пока это возможно
			 */
			while((distance(p, pend) >= 8) && simdParseIfEightDigitsUnrolled(p, i)){
				// В редких случаях возможно переполнение — оно обрабатывается выше по стеку
				p += 8;
			}
		}

		/**
		 * @brief Циклически разбирает блоки по 8 цифр для char
		 *
		 * @param p    текущая позиция в строке
		 * @param pend конец строки
		 * @param i    аккумулятор целого значения
		 */
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 void loopParseIfEightDigits(const char * & p, const char * const pend, uint64_t & i) noexcept {
			/**
			 *  Для char скалярный путь обычно лучше SIMD-варианта
			 */
			while((distance(p, pend) >= 8) && isMadeOfEightDigitsFast(read8ToU64(p))){
				// Накапливаем очередные 8 цифр
				i = (i * 100000000ULL) + parseEightDigitsUnrolled(read8ToU64(p));
				// Смещаем указатель
				p += 8;
			}
		}

		/**
		 * @brief Коды ошибок разбора ASCII-числа
		 *
		 */
		enum class error_t : uint8_t {
			NO_ERROR = 0x00,                       // Ошибки нет
			MISSING_INTEGER_AFTER_SIGN = 0x01,     // После знака нет цифры (JSON)
			MISSING_INTEGER_OR_DOT_AFTER_SIGN = 0x02, // После знака нет цифры или точки
			LEADING_ZEROS_IN_INTEGER_PART = 0x03,  // Ведущие нули в целой части (JSON)
			NO_DIGITS_IN_INTEGER_PART = 0x04,      // Нет цифр в целой части (JSON)
			NO_DIGITS_IN_FRACTIONAL_PART = 0x05,   // Нет цифр в дробной части (JSON)
			NO_DIGITS_IN_MANTISSA = 0x06,          // Нет цифр в мантиссе
			MISSING_EXPONENTIAL_PART = 0x07        // Отсутствует экспоненциальная часть
		};

		/**
		 * @brief Результат разбора ASCII-представления числа
		 *
		 * @tparam UC тип символа
		 */
		template <typename UC>
		struct parsedNumber_t {
			int64_t exponent{0};          // Десятичный экспонент
			uint64_t mantissa{0};         // Мантисса
			const UC * lastmatch{nullptr}; // Указатель сразу за разобранным числом
			bool negative{false};         // Признак отрицательного числа
			bool valid{false};            // Признак успешного разбора
			bool tooManyDigits{false};    // Признак слишком большого числа цифр
			span <const UC> integer{};    // Диапазон целой части
			span <const UC> fraction{};   // Диапазон дробной части
			error_t error{error_t::NO_ERROR}; // Код ошибки разбора
		};

		/**
		 * @brief Псевдоним диапазона байт
		 *
		 */
		using byteSpan_t = span <const char>;

		/**
		 * @brief Псевдоним результата разбора для char
		 *
		 */
		using ParsedNumber = parsedNumber_t <char>;

		/**
		 * @brief Формирует результат с ошибкой разбора
		 *
		 * @tparam UC тип символа
		 * @param p     позиция ошибки
		 * @param error код ошибки
		 * @return      заполненная структура результата
		 */
		template <typename UC>
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 parsedNumber_t <UC> reportParseError(const UC * p, const error_t error) noexcept {
			// Формируем ответ об ошибке
			parsedNumber_t <UC> answer;
			// Разбор неуспешен
			answer.valid = false;
			// Сохраняем позицию ошибки
			answer.lastmatch = p;
			// Сохраняем код ошибки
			answer.error = error;
			// Возвращаем результат
			return answer;
		}

		/**
		 * @brief Разбирает ASCII-строку числа с плавающей точкой
		 *
		 * @tparam basicJsonFmt включить строгие правила JSON
		 * @tparam UC           тип символа
		 *
		 * @param p       начало строки
		 * @param pend    конец строки
		 * @param options опции разбора
		 * @return        результат разбора
		 */
		template <bool basicJsonFmt, typename UC>
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 parsedNumber_t <UC> parseNumberString(const UC * p, const UC * pend, const options_t <UC> options) noexcept {
			// Нормализуем формат с учётом feature-макросов
			const format_t fmt = detail::adjustForFeatureMacros(options.format);
			// Извлекаем символ десятичной точки
			const UC decimalPoint = options.decimalPoint;
			// Инициализируем результат
			parsedNumber_t <UC> answer;
			answer.valid = false;
			answer.tooManyDigits = false;
			// Определяем знак числа (предполагается p < pend)
			answer.negative = (* p == UC('-'));
			// Обрабатываем ведущий знак
			if((* p == UC('-')) || ((uint64_t(fmt & format_t::ALLOW_LEADING_PLUS) != 0) && !basicJsonFmt && (* p == UC('+')))){
				// Пропускаем знак
				++p;
				// После знака строка не должна заканчиваться
				if(p == pend)
					return reportParseError <UC> (p, error_t::MISSING_INTEGER_OR_DOT_AFTER_SIGN);
				// Для JSON после знака обязательна цифра
				if constexpr (basicJsonFmt) {
					if(!isInteger(* p))
						return reportParseError <UC> (p, error_t::MISSING_INTEGER_AFTER_SIGN);
				} else {
					// В общем режиме допускаем цифру или точку
					if(!isInteger(* p) && (* p != decimalPoint))
						return reportParseError <UC> (p, error_t::MISSING_INTEGER_OR_DOT_AFTER_SIGN);
				}
			}
			// Запоминаем начало цифр целой части
			const UC * const startDigits = p;
			// Аккумулятор мантиссы
			uint64_t i = 0;
			/**
			 *  Разбираем цифры целой части
			 */
			while((p != pend) && isInteger(* p)){
				// Накапливаем очередную цифру
				i = (10ULL * i) + uint64_t(* p - UC('0'));
				++p;
			}
			// Конец целой части
			const UC * const endOfIntegerPart = p;
			// Число цифр (с учётом дробной части ниже)
			int64_t digitCount = int64_t(endOfIntegerPart - startDigits);
			// Сохраняем диапазон целой части
			answer.integer = span <const UC> (startDigits, size_t(digitCount));
			// Дополнительные проверки JSON для целой части
			if constexpr (basicJsonFmt) {
				if(digitCount == 0)
					return reportParseError <UC> (p, error_t::NO_DIGITS_IN_INTEGER_PART);
				if((startDigits[0] == UC('0')) && (digitCount > 1))
					return reportParseError <UC> (startDigits, error_t::LEADING_ZEROS_IN_INTEGER_PART);
			}
			// Экспонент от дробной части и явной записи
			int64_t exponent = 0;
			// Признак наличия десятичной точки
			const bool hasDecimalPoint = ((p != pend) && (* p == decimalPoint));
			if(hasDecimalPoint){
				// Пропускаем точку
				++p;
				// Начало дробной части
				const UC * const before = p;
				// Быстрый разбор блоков по 8 цифр
				loopParseIfEightDigits(p, pend, i);
				/**
				 *  Разбираем оставшиеся цифры по одной
				 */
				while((p != pend) && isInteger(* p)){
					const uint8_t digit = uint8_t(* p - UC('0'));
					++p;
					i = (i * 10ULL) + digit;
				}
				// Отрицательный вклад дробной части в экспонент
				exponent = before - p;
				// Сохраняем диапазон дробной части
				answer.fraction = span <const UC> (before, size_t(p - before));
				// Учитываем дробные цифры в общем счётчике
				digitCount -= exponent;
			}
			// Проверки JSON/общего режима для дробной части и мантиссы
			if constexpr (basicJsonFmt) {
				if(hasDecimalPoint && (exponent == 0))
					return reportParseError <UC> (p, error_t::NO_DIGITS_IN_FRACTIONAL_PART);
			} else if(digitCount == 0){
				return reportParseError <UC> (p, error_t::NO_DIGITS_IN_MANTISSA);
			}
			// Явная экспоненциальная часть
			int64_t expNumber = 0;
			if(
				((uint64_t(fmt & format_t::SCIENTIFIC) != 0) && (p != pend) && ((* p == UC('e')) || (* p == UC('E')))) ||
				((uint64_t(fmt & detail::basicFortranFmt) != 0) && (p != pend) && ((* p == UC('+')) || (* p == UC('-')) || (* p == UC('d')) || (* p == UC('D'))))
			){
				// Позиция маркера экспоненты на случай отката
				const UC * const locationOfE = p;
				// Пропускаем маркер экспоненты
				if((* p == UC('e')) || (* p == UC('E')) || (* p == UC('d')) || (* p == UC('D')))
					++p;
				// Знак экспоненты
				bool negExp = false;
				if((p != pend) && (* p == UC('-'))){
					negExp = true;
					++p;
				} else if((p != pend) && (* p == UC('+'))){
					++p;
				}
				// Если цифр экспоненты нет
				if((p == pend) || !isInteger(* p)){
					// В scientific-only режиме это ошибка
					if((uint64_t(fmt & format_t::FIXED) == 0))
						return reportParseError <UC> (p, error_t::MISSING_EXPONENTIAL_PART);
					// Иначе игнорируем незавершённый маркер 'e'
					p = locationOfE;
				} else {
					/**
					 *  Разбираем цифры экспоненты
					 */
					while((p != pend) && isInteger(* p)){
						const uint8_t digit = uint8_t(* p - UC('0'));
						if(expNumber < 0x10000000)
							expNumber = (10 * expNumber) + digit;
						++p;
					}
					// Применяем знак
					if(negExp)
						expNumber = -expNumber;
					// Суммируем с вкладом дробной части
					exponent += expNumber;
				}
			} else {
				// Scientific без fixed требует экспоненту
				if(((uint64_t(fmt & format_t::SCIENTIFIC) != 0) && ((uint64_t(fmt & format_t::FIXED) == 0))))
					return reportParseError <UC> (p, error_t::MISSING_EXPONENTIAL_PART);
			}
			// Фиксируем конец разбора
			answer.lastmatch = p;
			answer.valid = true;
			// Обрабатываем слишком длинные мантиссы (> 19 цифр)
			if(digitCount > 19){
				// Пропускаем ведущие нули и точки
				const UC * start = startDigits;
				while((start != pend) && ((* start == UC('0')) || (* start == decimalPoint))){
					if(* start == UC('0'))
						--digitCount;
					++start;
				}
				// Если значащих цифр всё ещё больше 19 — усекаем
				if(digitCount > 19){
					answer.tooManyDigits = true;
					i = 0;
					p = answer.integer.ptr;
					const UC * const intEnd = p + answer.integer.len();
					constexpr uint64_t minimalNineteenDigitInteger = 1000000000000000000ULL;
					/**
					 *  Набираем до 19 цифр из целой части
					 */
					while((i < minimalNineteenDigitInteger) && (p != intEnd)){
						i = (i * 10ULL) + uint64_t(* p - UC('0'));
						++p;
					}
					if(i >= minimalNineteenDigitInteger){
						// Целой части хватило на 19 цифр
						exponent = (endOfIntegerPart - p) + expNumber;
					} else {
						// Добираем из дробной части
						p = answer.fraction.ptr;
						const UC * const fracEnd = p + answer.fraction.len();
						while((i < minimalNineteenDigitInteger) && (p != fracEnd)){
							i = (i * 10ULL) + uint64_t(* p - UC('0'));
							++p;
						}
						exponent = (answer.fraction.ptr - p) + expNumber;
					}
				}
			}
			// Сохраняем итоговые поля
			answer.exponent = exponent;
			answer.mantissa = i;
			// Возвращаем результат
			return answer;
		}

		/**
		 * @brief Разбирает ASCII-строку целого числа
		 *
		 * @tparam T  тип целого результата
		 * @tparam UC тип символа
		 *
		 * @param p       начало строки
		 * @param pend    конец строки
		 * @param value   ссылка на результат
		 * @param options опции разбора
		 * @return        результат разбора
		 */
		template <typename T, typename UC>
		AWH_FLOAT_INLINE AWH_FLOAT_CONSTEXPR20 result_t <UC> parseIntString(const UC * p, const UC * pend, T & value, const options_t <UC> options) noexcept {
			// Нормализуем формат
			const format_t fmt = detail::adjustForFeatureMacros(options.format);
			// Основание системы счисления
			const int base = options.base;
			// Результат разбора
			result_t <UC> answer;
			// Запоминаем начало исходной строки
			const UC * const first = p;
			// Определяем знак
			const bool negative = (* p == UC('-'));
			#ifdef AWH_FLOAT_VISUAL_STUDIO
				#pragma warning(push)
				#pragma warning(disable : 4127)
			#endif
			// Для беззнакового типа минус недопустим
			if(!is_signed <T>::value && negative){
				answer.ec = errc::invalid_argument;
				answer.ptr = first;
				#ifdef AWH_FLOAT_VISUAL_STUDIO
					#pragma warning(pop)
				#endif
				return answer;
			}
			#ifdef AWH_FLOAT_VISUAL_STUDIO
				#pragma warning(pop)
			#endif
			// Пропускаем знак
			if((* p == UC('-')) || ((uint64_t(fmt & format_t::ALLOW_LEADING_PLUS) != 0) && (* p == UC('+'))))
				++p;
			// Начало числовой части
			const UC * const startNum = p;
			/**
			 *  Пропускаем ведущие нули
			 */
			while((p != pend) && (* p == UC('0')))
				++p;
			// Были ли ведущие нули
			const bool hasLeadingZeros = (p > startNum);
			// Начало значащих цифр
			const UC * const startDigits = p;
			// Аккумулятор
			uint64_t i = 0;
			// Для баз 10 пробуем быстрый SIMD/блочный путь
			if(base == 10)
				loopParseIfEightDigits(p, pend, i);
			/**
			 *  Разбираем оставшиеся цифры
			 */
			while(p != pend){
				const uint8_t digit = chToDigit(* p);
				if(digit >= base)
					break;
				i = (uint64_t(base) * i) + digit;
				++p;
			}
			// Количество значащих цифр
			const size_t digitCount = size_t(p - startDigits);
			// Пустая мантисса
			if(digitCount == 0){
				if(hasLeadingZeros){
					value = static_cast <T> (0);
					answer.ec = errc();
					answer.ptr = p;
				} else {
					answer.ec = errc::invalid_argument;
					answer.ptr = first;
				}
				return answer;
			}
			// Успешная позиция конца числа
			answer.ptr = p;
			// Проверка переполнения uint64_t по числу цифр
			const size_t maxDigits = maxDigitsU64(base);
			if(digitCount > maxDigits){
				answer.ec = errc::result_out_of_range;
				return answer;
			}
			if((digitCount == maxDigits) && (i < minSafeU64(base))){
				answer.ec = errc::result_out_of_range;
				return answer;
			}
			// Проверка переполнения целевого типа
			if(!is_same <T, uint64_t>::value){
				if(i > (uint64_t(numeric_limits <T>::max()) + uint64_t(negative))){
					answer.ec = errc::result_out_of_range;
					return answer;
				}
			}
			// Записываем результат с учётом знака
			if(negative){
				#ifdef AWH_FLOAT_VISUAL_STUDIO
					#pragma warning(push)
					#pragma warning(disable : 4146)
				#endif
				// Безопасное отрицание для constexpr/UB-ограничений
				value = T(-numeric_limits <T>::max() - T(i - uint64_t(numeric_limits <T>::max())));
				#ifdef AWH_FLOAT_VISUAL_STUDIO
					#pragma warning(pop)
				#endif
			} else {
				value = T(i);
			}
			// Успех
			answer.ec = errc();
			return answer;
		}

		/**
		 * @brief Совместимый псевдоним старого имени enum ошибок
		 *
		 */
		using parseError = error_t;
	} // namespace floating
} // namespace awh

#endif // __AWH_FLOAT_ASCII__
