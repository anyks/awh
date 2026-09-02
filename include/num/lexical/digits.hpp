/**
 * @file digits.hpp
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
 * @brief Заголовочный файл работы с десятичными разрядами модуля разбора чисел — таблицы степеней десятки,
 *        накопление значащих цифр, нормализация мантиссы и медленный точный путь разбора через длинную арифметику
 *
 * \~english
 * @brief Header file of the work with the decimal digits of the number parsing module — the tables of the powers of ten,
 *        the accumulation of the significant digits, the normalisation of the mantissa and the slow exact parsing path through long arithmetic
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_LEXICAL_DIGITS__
#define __AWH_LEXICAL_DIGITS__

/**
 * Стандартные заголовочные файлы
 */
#include <cstdint>
#include <cstring>
#include <algorithm>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"
#include "bigint.hpp"
#include "parser.hpp"

/**
 * Подавляем системные макросы, занявшие имена членов перечислений ниже:
 * DELETE и ERROR у MS Windows, CS и PRIVATE у Sun Solaris, CS5 у termios.
 * Имена снимаются лишь на время объявлений - возврат в конце файла
 */
#include "../../sys/macro/suppress.hpp"

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
	 *
	 * \~english
	 * @brief Namespace of the number parsing module
	 *
	 * \~
	 */
	namespace lexical {
		/**
		 * \~russian
		 * @brief Таблица степеней десятки, помещающихся в 64-битное слово
		 *
		 * \~english
		 * @brief Table of the powers of ten that fit into a 64-bit word
		 *
		 * \~
		 */
		constexpr uint64_t POWERS_OF_TEN[] = {
			1ULL,
			10ULL,
			100ULL,
			1000ULL,
			10000ULL,
			100000ULL,
			1000000ULL,
			10000000ULL,
			100000000ULL,
			1000000000ULL,
			10000000000ULL,
			100000000000ULL,
			1000000000000ULL,
			10000000000000ULL,
			100000000000000ULL,
			1000000000000000ULL,
			10000000000000000ULL,
			100000000000000000ULL,
			1000000000000000000ULL,
			10000000000000000000ULL
		};

		/**
		 * \~russian
		 * @brief Шаблон типа символа исходной строки
		 *
		 * @tparam UC тип символа исходной строки
		 *
		 *
		 * \~english
		 * @brief Template of the character type of the source string
		 * @tparam UC character type of the source string
		 *
		 * \~
		 */
		template <typename UC>
		/**
		 * \~russian
		 * @brief Функция получения упакованного блока нулевых десятичных цифр
		 *
		 * @return упакованный блок символов нуля
		 *
		 * \~english
		 * @brief Function of getting a packed block of zero decimal digits
		 * @return packed block of zero characters
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr uint64_t zerosPattern() noexcept {
			// Выполняем проверку допустимости разрядности символа
			static_assert((sizeof(UC) == 1) || (sizeof(UC) == 2) || (sizeof(UC) == 4), "AWH lexical: unsupported character size");
			// Выводим упакованный блок символов нуля
			return ((sizeof(UC) == 1)
				? 0x3030303030303030ULL
				: ((sizeof(UC) == 2)
					? ((uint64_t(UC('0')) << 48) | (uint64_t(UC('0')) << 32) | (uint64_t(UC('0')) << 16) | uint64_t(UC('0')))
					: ((uint64_t(UC('0')) << 32) | uint64_t(UC('0')))));
		}

		/**
		 * \~russian
		 * @brief Шаблон типа символа исходной строки
		 *
		 * @tparam UC тип символа исходной строки
		 *
		 *
		 * \~english
		 * @brief Template of the character type of the source string
		 * @tparam UC character type of the source string
		 *
		 * \~
		 */
		template <typename UC>
		/**
		 * \~russian
		 * @brief Функция получения количества символов в упакованном блоке
		 *
		 * @return количество символов в упакованном блоке
		 *
		 * \~english
		 * @brief Function of getting the number of characters in a packed block
		 * @return number of characters in the packed block
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr size_t patternLength() noexcept {
			// Выводим количество символов, помещающихся в 64-битное слово
			return (sizeof(uint64_t) / sizeof(UC));
		}

		/**
		 * \~russian
		 * @brief Функция вычисления показателя степени в научной записи
		 *
		 * @details Функция намеренно не оптимизирована: ускорение потребовало бы
		 *          замедлить быстрые пути разбора, а текущий вариант вызывается
		 *          только на редком уточняющем пути.
		 *
		 * @param mantissa значение мантиссы
		 * @param exponent исходный показатель степени
		 * @return         показатель степени в научной записи
		 *
		 * \~english
		 * @brief Function of computing the exponent in scientific notation
		 * @details The function is deliberately not optimised: speeding it up would require
		 *          slowing down the fast parsing paths, while the current variant is called
		 *          only on the rare refining path.
		 * @param mantissa value of the mantissa
		 * @param exponent source exponent
		 * @return         exponent in scientific notation
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr int32_t scientificExponent(uint64_t mantissa, int32_t exponent) noexcept {
			/**
			 * Выполняем сжатие мантиссы четвёрками десятичных разрядов
			 */
			while(mantissa >= 10000){
				// Выполняем деление мантиссы на четыре разряда
				mantissa /= 10000;
				// Учитываем сжатие в показателе степени
				exponent += 4;
			}
			/**
			 * Выполняем сжатие мантиссы парами десятичных разрядов
			 */
			while(mantissa >= 100){
				// Выполняем деление мантиссы на два разряда
				mantissa /= 100;
				// Учитываем сжатие в показателе степени
				exponent += 2;
			}
			/**
			 * Выполняем сжатие мантиссы по одному десятичному разряду
			 */
			while(mantissa >= 10){
				// Выполняем деление мантиссы на один разряд
				mantissa /= 10;
				// Учитываем сжатие в показателе степени
				exponent += 1;
			}
			// Выводим показатель степени в научной записи
			return exponent;
		}

		/**
		 * \~russian
		 * @brief Шаблон типа обработчика решения об округлении
		 *
		 * @tparam CALLBACK тип обработчика решения об округлении
		 *
		 * \~english
		 * @brief Template of the type of the rounding decision handler
		 * @tparam CALLBACK type of the rounding decision handler
		 *
		 * \~
		 */
		template <typename CALLBACK>
		/**
		 * \~russian
		 * @brief Функция округления мантиссы к ближайшему с разрешением ничьей к чётному
		 *
		 * @param mantissa ссылка на округляемую мантиссу
		 * @param shift    количество отбрасываемых младших бит
		 * @param callback обработчик решения об округлении вверх
		 *
		 * \~english
		 * @brief Function of rounding the mantissa to nearest with ties resolved to even
		 * @param mantissa reference to the rounded mantissa
		 * @param shift    number of discarded low bits
		 * @param callback handler of the decision to round up
		 *
		 * \~
		 */
		AWH_ASCII_INLINE void roundNearestTieEven(mantissa_t & mantissa, const int32_t shift, CALLBACK callback) noexcept {
			// Маска отбрасываемых младших бит
			const uint64_t mask = ((shift == 64) ? UINT64_MAX : ((uint64_t(1) << shift) - 1));
			// Значение ровно посередине между представимыми числами
			const uint64_t halfway = ((shift == 0) ? 0 : (uint64_t(1) << (shift - 1)));
			// Отбрасываемая часть мантиссы
			const uint64_t truncated = (mantissa.mantissa & mask);
			// Признак превышения середины интервала
			const bool isAbove = (truncated > halfway);
			// Признак попадания ровно в середину интервала
			const bool isHalfway = (truncated == halfway);
			// Если отбрасывается вся мантисса
			if(shift == 64)
				// Обнуляем мантиссу
				mantissa.mantissa = 0;
			// Выполняем сдвиг мантиссы на место
			else mantissa.mantissa >>= shift;
			// Компенсируем сдвиг показателем степени
			mantissa.power2 += shift;
			// Признак нечётности младшего значащего бита мантиссы
			const bool isOdd = ((mantissa.mantissa & 1) == 1);
			// Выполняем округление по решению обработчика
			mantissa.mantissa += static_cast <uint64_t> (callback(isOdd, isHalfway, isAbove));
		}

		/**
		 * \~russian
		 * @brief Функция округления мантиссы вниз
		 *
		 * @param mantissa ссылка на округляемую мантиссу
		 * @param shift    количество отбрасываемых младших бит
		 *
		 * \~english
		 * @brief Function of rounding the mantissa down
		 * @param mantissa reference to the rounded mantissa
		 * @param shift    number of discarded low bits
		 *
		 * \~
		 */
		AWH_ASCII_INLINE void roundDown(mantissa_t & mantissa, const int32_t shift) noexcept {
			// Если отбрасывается вся мантисса
			if(shift == 64)
				// Обнуляем мантиссу
				mantissa.mantissa = 0;
			// Выполняем сдвиг мантиссы на место
			else mantissa.mantissa >>= shift;
			// Компенсируем сдвиг показателем степени
			mantissa.power2 += shift;
		}

		/**
		 * \~russian
		 * @brief Шаблон типа результата и обработчика округления
		 *
		 * @tparam T        тип числа с плавающей точкой
		 * @tparam CALLBACK тип обработчика сдвига и округления
		 *
		 * \~english
		 * @brief Template of the result type and of the rounding handler
		 * @tparam T        floating-point type
		 * @tparam CALLBACK type of the shift and rounding handler
		 *
		 * \~
		 */
		template <typename T, typename CALLBACK>
		/**
		 * \~russian
		 * @brief Функция приведения мантиссы расширенной точности к машинному формату
		 *
		 * @param mantissa ссылка на приводимую мантиссу
		 * @param callback обработчик сдвига и округления мантиссы
		 *
		 * \~english
		 * @brief Function of bringing an extended-precision mantissa to the machine format
		 * @param mantissa reference to the mantissa being brought
		 * @param callback handler of the shift and rounding of the mantissa
		 *
		 * \~
		 */
		AWH_ASCII_INLINE void roundMantissa(mantissa_t & mantissa, CALLBACK callback) noexcept {
			// Величина сдвига для нормализованного числа
			const int32_t shift = (64 - binary_t <T>::mantissaExplicitBits() - 1);
			// Если результат попадает в субнормальный диапазон
			if((-mantissa.power2) >= shift){
				// Выполняем округление со сдвигом в субнормальный диапазон
				callback(mantissa, std::min <int32_t> (1 - mantissa.power2, 64));
				// Определяем, стало ли значение нормализованным после округления
				mantissa.power2 = ((mantissa.mantissa < (uint64_t(1) << binary_t <T>::mantissaExplicitBits())) ? 0 : 1);
				// Завершаем приведение мантиссы
				return;
			}
			// Выполняем округление со сдвигом нормализованного числа
			callback(mantissa, shift);
			// Если округление вызвало переполнение мантиссы
			if(mantissa.mantissa >= (uint64_t(2) << binary_t <T>::mantissaExplicitBits())){
				// Выполняем нормализацию мантиссы
				mantissa.mantissa = (uint64_t(1) << binary_t <T>::mantissaExplicitBits());
				// Компенсируем нормализацию показателем степени
				mantissa.power2++;
			}
			// Выполняем сброс скрытого бита мантиссы
			mantissa.mantissa &= ~(uint64_t(1) << binary_t <T>::mantissaExplicitBits());
			// Если показатель степени вышел за пределы представимого диапазона
			if(mantissa.power2 >= binary_t <T>::infinitePower()){
				// Устанавливаем показатель степени бесконечности
				mantissa.power2 = binary_t <T>::infinitePower();
				// Обнуляем мантиссу бесконечности
				mantissa.mantissa = 0;
			}
		}

		/**
		 * \~russian
		 * @brief Шаблон типа символа исходной строки
		 *
		 * @tparam UC тип символа исходной строки
		 *
		 *
		 * \~english
		 * @brief Template of the character type of the source string
		 * @tparam UC character type of the source string
		 *
		 * \~
		 */
		template <typename UC>
		/**
		 * \~russian
		 * @brief Функция пропуска ведущих нулевых цифр
		 *
		 * @param first ссылка на начало диапазона символов
		 * @param last  конец диапазона символов
		 *
		 * \~english
		 * @brief Function of skipping the leading zero digits
		 * @param first reference to the beginning of the range of characters
		 * @param last  end of the range of characters
		 *
		 * \~
		 */
		AWH_ASCII_INLINE void skipZeros(const UC * & first, const UC * const last) noexcept {
			// Упакованный блок символов
			uint64_t block = 0;
			/**
			 * Выполняем блочный пропуск нулевых цифр
			 */
			while(static_cast <size_t> (last - first) >= patternLength <UC> ()){
				// Выполняем чтение очередного блока символов
				::memcpy(&block, first, sizeof(uint64_t));
				// Если блок содержит не только нулевые цифры
				if(block != zerosPattern <UC> ())
					// Завершаем блочный пропуск
					break;
				// Выполняем смещение позиции на размер блока
				first += patternLength <UC> ();
			}
			/**
			 * Выполняем посимвольный пропуск оставшихся нулевых цифр
			 */
			while(first != last){
				// Если символ не является нулевой цифрой
				if(* first != UC('0'))
					// Завершаем посимвольный пропуск
					break;
				// Переходим к следующему символу
				++first;
			}
		}

		/**
		 * \~russian
		 * @brief Шаблон типа символа исходной строки
		 *
		 * @tparam UC тип символа исходной строки
		 *
		 *
		 * \~english
		 * @brief Template of the character type of the source string
		 * @tparam UC character type of the source string
		 *
		 * \~
		 */
		template <typename UC>
		/**
		 * \~russian
		 * @brief Функция проверки наличия отброшенных значащих цифр
		 *
		 * @details Все символы диапазона обязаны быть десятичными цифрами.
		 *
		 * @param first начало диапазона символов
		 * @param last  конец диапазона символов
		 * @return      результат проверки
		 *
		 * \~english
		 * @brief Function of checking the presence of discarded significant digits
		 * @details All the characters of the range must be decimal digits.
		 * @param first beginning of the range of characters
		 * @param last  end of the range of characters
		 * @return      result of the check
		 *
		 * \~
		 */
		AWH_ASCII_INLINE bool isTruncated(const UC * first, const UC * const last) noexcept {
			// Упакованный блок символов
			uint64_t block = 0;
			/**
			 * Выполняем блочную проверку диапазона символов
			 */
			while(static_cast <size_t> (last - first) >= patternLength <UC> ()){
				// Выполняем чтение очередного блока символов
				::memcpy(&block, first, sizeof(uint64_t));
				// Если блок содержит не только нулевые цифры
				if(block != zerosPattern <UC> ())
					// Сообщаем о наличии отброшенных значащих цифр
					return true;
				// Выполняем смещение позиции на размер блока
				first += patternLength <UC> ();
			}
			/**
			 * Выполняем посимвольную проверку оставшихся цифр
			 */
			while(first != last){
				// Если символ не является нулевой цифрой
				if(* first != UC('0'))
					// Сообщаем о наличии отброшенных значащих цифр
					return true;
				// Переходим к следующему символу
				++first;
			}
			// Сообщаем об отсутствии отброшенных значащих цифр
			return false;
		}

		/**
		 * \~russian
		 * @brief Шаблон типа символа исходной строки
		 *
		 * @tparam UC тип символа исходной строки
		 *
		 *
		 * \~english
		 * @brief Template of the character type of the source string
		 * @tparam UC character type of the source string
		 *
		 * \~
		 */
		template <typename UC>
		/**
		 * \~russian
		 * @brief Функция проверки наличия отброшенных значащих цифр в диапазоне
		 *
		 * @param range проверяемый диапазон символов
		 * @return      результат проверки
		 *
		 * \~english
		 * @brief Function of checking the presence of discarded significant digits in a range
		 * @param range range of characters to check
		 * @return      result of the check
		 *
		 * \~
		 */
		AWH_ASCII_INLINE bool isTruncated(const span_t <UC> range) noexcept {
			// Выполняем проверку диапазона символов
			return isTruncated(range.ptr, range.ptr + range.len());
		}

		/**
		 * \~russian
		 * @brief Функция умножения значения длинной арифметики с добавлением скаляра
		 *
		 * @param value    значение длинной арифметики
		 * @param power    множитель в виде степени десятки
		 * @param addition добавляемое значение
		 * @return         результат выполнения операции
		 *
		 * \~english
		 * @brief Function of multiplying a long arithmetic value with the addition of a scalar
		 * @param value    long arithmetic value
		 * @param power    multiplier in the form of a power of ten
		 * @param addition added value
		 * @return         result of performing the operation
		 *
		 * \~
		 */
		AWH_ASCII_INLINE bool mulAdd(bigint_t & value, const limb_t power, const limb_t addition) noexcept {
			// Выполняем умножение значения на степень десятки
			if(!value.mul(power))
				// Сообщаем, что операция не выполнена
				return false;
			// Выполняем добавление накопленного значения
			return value.add(addition);
		}

		/**
		 * \~russian
		 * @brief Шаблон типа символа исходной строки
		 *
		 * @tparam UC тип символа исходной строки
		 *
		 *
		 * \~english
		 * @brief Template of the character type of the source string
		 * @tparam UC character type of the source string
		 *
		 * \~
		 */
		template <typename UC>
		/**
		 * \~russian
		 * @brief Функция разбора значащих цифр в значение длинной арифметики
		 *
		 * @details Разбор ведётся блоками по восемь цифр с последующим умножением
		 *          на наибольшую степень десятки, помещающуюся в один разряд, что
		 *          минимизирует количество операций длинной арифметики.
		 *
		 * @param result    ссылка на результат разбора
		 * @param number    разобранная числовая строка
		 * @param maxDigits максимальное количество значащих цифр
		 * @param digits    ссылка на количество разобранных цифр
		 * @return          результат выполнения операции
		 *
		 * \~english
		 * @brief Function of parsing the significant digits into a long arithmetic value
		 * @details The parsing is done in blocks of eight digits with a subsequent multiplication
		 *          by the largest power of ten that fits into one limb, which
		 *          minimises the number of long arithmetic operations.
		 * @param result    reference to the result of the parsing
		 * @param number    parsed number string
		 * @param maxDigits maximum number of significant digits
		 * @param digits    reference to the number of parsed digits
		 * @return          result of performing the operation
		 *
		 * \~
		 */
		inline bool parseMantissa(bigint_t & result, const parsedNumber_t <UC> & number, const size_t maxDigits, size_t & digits) noexcept {
			// Количество цифр, накопленных в текущем блоке
			size_t counter = 0;
			// Накопленное значение текущего блока
			limb_t value = 0;
			// Сбрасываем количество разобранных цифр
			digits = 0;
			/**
			 * Определяем размер блока скалярного умножения по разрядности
			 */
			#ifdef AWH_LEXICAL_64BIT_LIMB
				// Количество десятичных цифр, помещающихся в один разряд
				constexpr size_t STEP = 19;
			/**
			 * Если разрядность меньше 64 бит, то используем меньший размер блока
			 */
			#else
				// Количество десятичных цифр, помещающихся в один разряд
				constexpr size_t STEP = 9;
			#endif
			// Текущая позиция разбора значащих цифр
			const UC * position = number.integer.ptr;
			// Конец разбираемого диапазона символов
			const UC * end = (position + number.integer.len());
			// Выполняем пропуск ведущих нулей целой части
			skipZeros(position, end);
			/**
			 * Выполняем разбор значащих цифр целой части
			 */
			while(position != end){
				/**
				 * Выполняем блочный разбор цифр, пока это возможно
				 */
				while((static_cast <size_t> (end - position) >= DIGITS_PER_BLOCK) &&
					  ((STEP - counter) >= DIGITS_PER_BLOCK) &&
					  ((maxDigits - digits) >= DIGITS_PER_BLOCK)){
					// Выполняем накопление разобранного блока цифр
					value = ((value * 100000000ULL) + parseBlock(position));
					// Выполняем смещение позиции на размер блока
					position += DIGITS_PER_BLOCK;
					// Учитываем разобранные цифры в счётчике блока
					counter += DIGITS_PER_BLOCK;
					// Учитываем разобранные цифры в общем счётчике
					digits += DIGITS_PER_BLOCK;
				}
				/**
				 * Выполняем поразрядный разбор оставшихся цифр блока
				 */
				while((counter < STEP) && (position != end) && (digits < maxDigits)){
					// Выполняем накопление очередной цифры
					value = ((value * 10ULL) + static_cast <limb_t> (* position - UC('0')));
					// Переходим к следующему символу
					++position;
					// Учитываем разобранную цифру в счётчике блока
					++counter;
					// Учитываем разобранную цифру в общем счётчике
					++digits;
				}
				// Выполняем перенос накопленного блока в значение длинной арифметики
				if(!mulAdd(result, static_cast <limb_t> (POWERS_OF_TEN[counter]), value))
					// Сообщаем, что операция не выполнена
					return false;
				// Если достигнут предел количества значащих цифр
				if(digits == maxDigits){
					// Определяем наличие отброшенных значащих цифр целой части
					bool truncated = isTruncated(position, end);
					// Если дробная часть присутствует
					if(number.fraction.ptr != nullptr)
						// Учитываем отброшенные значащие цифры дробной части
						truncated = (truncated || isTruncated(number.fraction));
					// Если значащие цифры были отброшены
					if(truncated){
						/**
						 * Выполняем округление вверх на единицу младшего разряда,
						 * что исключает ложное попадание в середину интервала
						 */
						if(!mulAdd(result, 10, 1))
							// Сообщаем, что операция не выполнена
							return false;
						// Учитываем добавленный разряд в общем счётчике
						++digits;
					}
					// Завершаем разбор значащих цифр
					return true;
				}
				// Сбрасываем счётчик цифр текущего блока
				counter = 0;
				// Сбрасываем накопленное значение текущего блока
				value = 0;
			}
			// Если дробная часть присутствует
			if(number.fraction.ptr != nullptr){
				// Переходим к началу дробной части
				position = number.fraction.ptr;
				// Устанавливаем конец разбираемого диапазона символов
				end = (position + number.fraction.len());
				// Если значащих цифр ещё не обнаружено
				if(digits == 0)
					// Выполняем пропуск ведущих нулей дробной части
					skipZeros(position, end);
				/**
				 * Выполняем разбор значащих цифр дробной части
				 */
				while(position != end){
					/**
					 * Выполняем блочный разбор цифр, пока это возможно
					 */
					while((static_cast <size_t> (end - position) >= DIGITS_PER_BLOCK) &&
						  ((STEP - counter) >= DIGITS_PER_BLOCK) &&
						  ((maxDigits - digits) >= DIGITS_PER_BLOCK)){
						// Выполняем накопление разобранного блока цифр
						value = ((value * 100000000ULL) + parseBlock(position));
						// Выполняем смещение позиции на размер блока
						position += DIGITS_PER_BLOCK;
						// Учитываем разобранные цифры в счётчике блока
						counter += DIGITS_PER_BLOCK;
						// Учитываем разобранные цифры в общем счётчике
						digits += DIGITS_PER_BLOCK;
					}
					/**
					 * Выполняем поразрядный разбор оставшихся цифр блока
					 */
					while((counter < STEP) && (position != end) && (digits < maxDigits)){
						// Выполняем накопление очередной цифры
						value = ((value * 10ULL) + static_cast <limb_t> (* position - UC('0')));
						// Переходим к следующему символу
						++position;
						// Учитываем разобранную цифру в счётчике блока
						++counter;
						// Учитываем разобранную цифру в общем счётчике
						++digits;
					}
					// Выполняем перенос накопленного блока в значение длинной арифметики
					if(!mulAdd(result, static_cast <limb_t> (POWERS_OF_TEN[counter]), value))
						// Сообщаем, что операция не выполнена
						return false;
					// Если достигнут предел количества значащих цифр
					if(digits == maxDigits){
						// Если значащие цифры дробной части были отброшены
						if(isTruncated(position, end)){
							/**
							 * Выполняем округление вверх на единицу младшего разряда, 
							 * что исключает ложное попадание в середину интервала
							 */
							if(!mulAdd(result, 10, 1))
								// Сообщаем, что операция не выполнена
								return false;
							// Учитываем добавленный разряд в общем счётчике
							++digits;
						}
						// Завершаем разбор значащих цифр
						return true;
					}
					// Сбрасываем счётчик цифр текущего блока
					counter = 0;
					// Сбрасываем накопленное значение текущего блока
					value = 0;
				}
			}
			// Если остался неперенесённый блок цифр
			if(counter != 0)
				// Выполняем перенос накопленного блока в значение длинной арифметики
				return mulAdd(result, static_cast <limb_t> (POWERS_OF_TEN[counter]), value);
			// Сообщаем, что операция выполнена успешно
			return true;
		}

		/**
		 * \~russian
		 * @brief Шаблон типа числа с плавающей точкой
		 *
		 * @tparam T тип числа с плавающей точкой
		 *
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
		 * @brief Функция округления при неотрицательном десятичном показателе степени
		 *
		 * @param digits   значащие цифры в виде значения длинной арифметики
		 * @param exponent неотрицательный десятичный показатель степени
		 * @param result   ссылка на результат округления
		 * @return         результат выполнения операции
		 *
		 * \~english
		 * @brief Function of rounding at a non-negative decimal exponent
		 * @param digits   significant digits as a long arithmetic value
		 * @param exponent non-negative decimal exponent
		 * @param result   reference to the result of the rounding
		 * @return         result of performing the operation
		 *
		 * \~
		 */
		inline bool positiveDigitComp(bigint_t & digits, const int32_t exponent, mantissa_t & result) noexcept {
			// Выполняем умножение значащих цифр на степень десятки
			if(!digits.pow10(static_cast <uint32_t> (exponent)))
				// Сообщаем, что операция не выполнена
				return false;
			// Признак отброшенных значащих бит
			bool truncated = false;
			// Выполняем извлечение старших 64 бит значения
			result.mantissa = digits.hi64(truncated);
			// Смещение показателя степени относительно минимального
			const int32_t bias = (binary_t <T>::mantissaExplicitBits() - binary_t <T>::minimumExponent());
			// Выполняем вычисление показателя степени результата
			result.power2 = (digits.bitLength() - 64 + bias);
			// Выполняем округление к ближайшему с учётом отброшенных бит
			roundMantissa <T> (result, [truncated](mantissa_t & value, const int32_t shift){
				// Выполняем округление к ближайшему с разрешением ничьей к чётному
				roundNearestTieEven(value, shift, [truncated](const bool isOdd, const bool isHalfway, const bool isAbove) -> bool {
					// Округляем вверх при превышении середины или при отброшенных битах
					return (isAbove || (isHalfway && truncated) || (isOdd && isHalfway));
				});
			});
			// Сообщаем, что операция выполнена успешно
			return true;
		}

		/**
		 * \~russian
		 * @brief Шаблон типа числа с плавающей точкой
		 *
		 * @tparam T тип числа с плавающей точкой
		 *
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
		 * @brief Функция округления при отрицательном десятичном показателе степени
		 *
		 * @details Фактические цифры представимы как m * 10^e, а теоретическое
		 *          значение середины интервала как n * 2^f. Поскольку показатель
		 *          степени отрицателен, обе величины приводятся к общей степени
		 *          и сравниваются точно.
		 *
		 * @param digits   значащие цифры в виде значения длинной арифметики
		 * @param source   исходная скорректированная мантисса
		 * @param exponent отрицательный десятичный показатель степени
		 * @param result   ссылка на результат округления
		 * @return         результат выполнения операции
		 *
		 * \~english
		 * @brief Function of rounding at a negative decimal exponent
		 * @details The actual digits are representable as m * 10^e, and the theoretical
		 *          value of the middle of the interval as n * 2^f. Since the exponent
		 *          is negative, both quantities are brought to a common power
		 *          and compared exactly.
		 * @param digits   significant digits as a long arithmetic value
		 * @param source   source corrected mantissa
		 * @param exponent negative decimal exponent
		 * @param result   reference to the result of the rounding
		 * @return         result of performing the operation
		 *
		 * \~
		 */
		inline bool negativeDigitComp(bigint_t & digits, const mantissa_t & source, const int32_t exponent, mantissa_t & result) noexcept {
			// Копия исходной мантиссы для округления вниз
			mantissa_t lower = source;
			// Выполняем округление исходной мантиссы вниз
			roundMantissa <T> (lower, [](mantissa_t & value, const int32_t shift){
				// Выполняем отбрасывание младших бит мантиссы
				roundDown(value, shift);
			});
			// Машинное значение нижней границы интервала
			T boundary = T(0);
			// Выполняем сборку машинного значения нижней границы
			toFloat(false, lower, boundary);
			// Середина интервала между нижней границей и следующим представимым числом
			const mantissa_t halfway = toExtendedHalfway(boundary);
			// Теоретическое значение середины интервала в длинной арифметике
			bigint_t theory(halfway.mantissa);
			// Показатель степени двойки для приведения к общей степени
			const int32_t power2 = (halfway.power2 - exponent);
			// Показатель степени пятёрки для приведения к общей степени
			const uint32_t power5 = static_cast <uint32_t> (-exponent);
			// Если требуется приведение по степени пятёрки
			if(power5 != 0){
				// Выполняем умножение теоретического значения на степень пятёрки
				if(!theory.pow5(power5))
					// Сообщаем, что операция не выполнена
					return false;
			}
			// Если теоретическое значение требует приведения по степени двойки
			if(power2 > 0){
				// Выполняем умножение теоретического значения на степень двойки
				if(!theory.pow2(static_cast <uint32_t> (power2)))
					// Сообщаем, что операция не выполнена
					return false;
			// Если фактические цифры требуют приведения по степени двойки
			} else if(power2 < 0){
				// Выполняем умножение фактических цифр на степень двойки
				if(!digits.pow2(static_cast <uint32_t> (-power2)))
					// Сообщаем, что операция не выполнена
					return false;
			}
			// Выполняем точное сравнение фактических цифр с теоретическим значением
			const int32_t order = digits.compare(theory);
			// Копируем исходную мантиссу для округления
			result = source;
			// Выполняем округление по результату сравнения
			roundMantissa <T> (result, [order](mantissa_t & value, const int32_t shift){
				// Выполняем округление к ближайшему с разрешением ничьей к чётному
				roundNearestTieEven(value, shift, [order](const bool isOdd, const bool, const bool) -> bool {
					// Если фактические цифры больше теоретической середины
					if(order > 0)
						// Выполняем округление вверх
						return true;
					// Если фактические цифры меньше теоретической середины
					else if(order < 0)
						// Выполняем округление вниз
						return false;
					// Разрешаем точное попадание в середину к чётному значению
					return isOdd;
				});
			});
			// Сообщаем, что операция выполнена успешно
			return true;
		}

		/**
		 * \~russian
		 * @brief Шаблон типа результата и типа символа исходной строки
		 *
		 * @tparam T  тип числа с плавающей точкой
		 * @tparam UC тип символа исходной строки
		 *
		 *
		 * \~english
		 * @brief Template of the result type and of the character type of the source string
		 * @tparam T  floating-point type
		 * @tparam UC character type of the source string
		 *
		 * \~
		 */
		template <typename T, typename UC>
		/**
		 * \~russian
		 * @brief Функция точного округления значащих цифр длинной арифметикой
		 *
		 * @details При неотрицательном показателе степени относительно значащих цифр
		 *          берутся старшие 64 бита значения длинной арифметики с учётом
		 *          отброшенных цифр. При отрицательном показателе выполняется точное
		 *          сравнение фактических цифр с теоретической серединой интервала.
		 *          При исчерпании ёмкости длинной арифметики выводится исходное
		 *          приближение, поскольку уточнить его точнее невозможно.
		 *
		 * @param number разобранная числовая строка
		 * @param source исходная скорректированная мантисса с невалидным показателем
		 * @return       скорректированная мантисса двоичного представления
		 *
		 * \~english
		 * @brief Function of the exact rounding of the significant digits by long arithmetic
		 * @details At a non-negative exponent relative to the significant digits
		 *          the high 64 bits of the long arithmetic value are taken, taking into account
		 *          the discarded digits. At a negative exponent an exact
		 *          comparison of the actual digits with the theoretical middle of the interval is performed.
		 *          On exhausting the capacity of the long arithmetic the source
		 *          approximation is yielded, since refining it more precisely is impossible.
		 * @param number parsed number string
		 * @param source source corrected mantissa with an invalid exponent
		 * @return       corrected mantissa of the binary representation
		 *
		 * \~
		 */
		inline mantissa_t digitComp(const parsedNumber_t <UC> & number, mantissa_t source) noexcept {
			// Выполняем снятие смещения невалидного показателя степени
			source.power2 -= INVALID_BIAS;
			// Показатель степени разбираемого числа в научной записи
			const int32_t scientific = scientificExponent(number.mantissa, static_cast <int32_t> (number.exponent));
			// Максимальное количество значащих цифр двоичного формата
			const size_t maxDigits = binary_t <T>::maxDigits();
			// Количество разобранных значащих цифр
			size_t digits = 0;
			// Значащие цифры в виде значения длинной арифметики
			bigint_t value;
			// Выполняем разбор значащих цифр в значение длинной арифметики
			if(!parseMantissa(value, number, maxDigits, digits))
				// Выводим исходное приближение без уточнения
				return source;
			// Показатель степени относительно разобранных значащих цифр
			const int32_t exponent = (scientific + 1 - static_cast <int32_t> (digits));
			// Результат точного округления
			mantissa_t result;
			// Если показатель степени является неотрицательным
			if(exponent >= 0){
				// Выполняем округление при неотрицательном показателе степени
				if(!positiveDigitComp <T> (value, exponent, result))
					// Выводим исходное приближение без уточнения
					return source;
			// Если показатель степени является отрицательным
			} else if(!negativeDigitComp <T> (value, source, exponent, result))
				// Выводим исходное приближение без уточнения
				return source;
			// Выводим результат точного округления
			return result;
		}
	};
};

/**
 * Возвращаем системные макросы потребителю библиотеки:
 * имена, подавленные в начале файла, снова принадлежат ему
 */
#include "../../sys/macro/restore.hpp"

#endif // __AWH_LEXICAL_DIGITS__
