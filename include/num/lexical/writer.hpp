/**
 * @file writer.hpp
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
 * @brief Заголовочный файл записи чисел в строковое представление —
 *        запись целых чисел в системах счисления от двоичной до тридцатишестеричной
 *        и запись чисел с плавающей точкой кратчайшим обратимым представлением
 *
 * \~english
 * @brief Header file of writing numbers into their string representation —
 *        the writing of integers in the numeral systems from binary to base thirty six
 *        and the writing of floating-point numbers by the shortest reversible representation
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_LEXICAL_WRITER__
#define __AWH_LEXICAL_WRITER__

/**
 * Стандартные заголовочные файлы
 */
#include <cstdint>
#include <cstring>
#include <type_traits>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"
#include "shortest.hpp"

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
		 * @brief Шаблон типа символа записываемой строки
		 *
		 * @tparam UC тип символа записываемой строки
		 *
		 * \~english
		 * @brief Template of the character type of the written string
		 * @tparam UC character type of the written string
		 *
		 * \~
		 */
		template <typename UC>
		/**
		 * \~russian
		 * @brief Структура результата записи числа в строку
		 *
		 * @details В отличие от результата разбора, указатель на конец записи является
		 *          изменяемым: он указывает в тот же буфер, что был передан методу.
		 *
		 * \~english
		 * @brief Structure of the result of writing a number into a string
		 * @details Unlike the result of the parsing, the pointer to the end of the record is
		 *          mutable: it points into the very buffer which was passed to the method.
		 *
		 * \~
		 */
		struct output_t {
			// Код ошибки стандартной библиотеки
			errc ec;
			// Код причины отказа при записи числовой строки
			error_t error;
			// Указатель на первый символ за записанным числом
			UC * ptr;
			/**
			 * \~russian
			 * @brief Оператор проверки успешности записи
			 *
			 * @return результат проверки
			 *
			 * \~english
			 * @brief Operator of checking the success of the writing
			 * @return result of the check
			 *
			 * \~
			 */
			constexpr explicit operator bool() const noexcept {
				// Запись успешна, если код ошибки не установлен
				return (this->ec == errc());
			}
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param ptr   указатель на первый символ за записанным числом
			 * @param ec    код ошибки стандартной библиотеки
			 * @param error код причины отказа при записи числовой строки
			 *
			 * \~english
			 * @brief Constructor
			 * @param ptr   pointer to the first character past the written number
			 * @param ec    error code of the standard library
			 * @param error failure reason code of the writing of the number string
			 *
			 * \~
			 */
			constexpr output_t(UC * ptr = nullptr, const errc ec = errc(), const error_t error = error_t::NONE) noexcept :
			 ec(ec), error(error), ptr(ptr) {}
		};

		/**
		 * \~russian
		 * @brief Таблица цифр систем счисления до тридцатишестеричной
		 *
		 * \~english
		 * @brief Table of the digits of the numeral systems up to base thirty six
		 *
		 * \~
		 */
		static constexpr char DIGITS_TABLE[37] = "0123456789abcdefghijklmnopqrstuvwxyz";

		/**
		 * \~russian
		 * @brief Таблица пар десятичных цифр
		 *
		 * @details Таблица позволяет записывать десятичное число сразу парами цифр,
		 *          вдвое сокращая количество делений.
		 *
		 * \~english
		 * @brief Table of the pairs of the decimal digits
		 * @details The table allows writing a decimal number at once by pairs of digits,
		 *          halving the number of the divisions.
		 *
		 * \~
		 */
		static constexpr char DIGITS_PAIRS[201] =
			"00010203040506070809"
			"10111213141516171819"
			"20212223242526272829"
			"30313233343536373839"
			"40414243444546474849"
			"50515253545556575859"
			"60616263646566676869"
			"70717273747576777879"
			"80818283848586878889"
			"90919293949596979899";

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
		 * @brief Метод извлечения достаточного размера места под запись числа
		 *
		 * @details Наибольшую длину даёт запись без показателя степени у наименьшего
		 *          представимого значения: она состоит из знака, ведущего нуля целой
		 *          части, разделителя дробной части, незначащих нулей дробной части и
		 *          значащих цифр. Количество незначащих нулей равно десятичному порядку
		 *          наименьшего представимого значения, а количество значащих цифр —
		 *          разрядности мантиссы, выраженной десятичными цифрами. Оценка взята
		 *          с запасом и проверена перебором.
		 *
		 * @return достаточный размер места под запись числа в символах
		 *
		 * \~english
		 * @brief Method of extracting the sufficient size of the place for the record of a number
		 * @details The greatest length is given by the record without an exponent of the smallest
		 *          representable value: it consists of the sign, of the leading zero of the integer
		 *          part, of the decimal point, of the insignificant zeros of the fractional part and
		 *          of the significant digits. The number of the insignificant zeros equals the decimal
		 *          exponent of the smallest representable value, while the number of the significant
		 *          digits equals the bit width of the mantissa expressed in decimal digits. The estimate
		 *          is taken with a margin and is checked by an enumeration.
		 * @return sufficient size of the place for the record of a number in characters
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr size_t maxRecordLength() noexcept {
			// Выводим достаточный размер места под запись числа
			return (
				// Знак числа, ведущий ноль целой части и разделитель дробной части
				3 +
				// Незначащие нули дробной части наименьшего представимого значения
				((((-binary_t <T>::minimumExponent() + binary_t <T>::mantissaExplicitBits()) * 302) / 1000) + 1) +
				// Значащие цифры кратчайшей записи мантиссы
				((((binary_t <T>::mantissaExplicitBits() + 1) * 302) / 1000) + 2)
			);
		}

		/**
		 * \~russian
		 * @brief Метод извлечения количества десятичных цифр значения
		 *
		 * @param value значение для подсчёта
		 * @return      количество десятичных цифр
		 *
		 * \~english
		 * @brief Method of extracting the number of the decimal digits of a value
		 * @param value value to count
		 * @return      number of the decimal digits
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr size_t decimalLength(const uint64_t value) noexcept {
			// Выводим количество десятичных цифр значения
			return (
				(value >= 10000000000000000000ull) ? 20 : (value >= 1000000000000000000ull) ? 19 :
				(value >= 100000000000000000ull) ? 18 : (value >= 10000000000000000ull) ? 17 :
				(value >= 1000000000000000ull) ? 16 : (value >= 100000000000000ull) ? 15 :
				(value >= 10000000000000ull) ? 14 : (value >= 1000000000000ull) ? 13 :
				(value >= 100000000000ull) ? 12 : (value >= 10000000000ull) ? 11 :
				(value >= 1000000000ull) ? 10 : (value >= 100000000ull) ? 9 :
				(value >= 10000000ull) ? 8 : (value >= 1000000ull) ? 7 :
				(value >= 100000ull) ? 6 : (value >= 10000ull) ? 5 :
				(value >= 1000ull) ? 4 : (value >= 100ull) ? 3 :
				(value >= 10ull) ? 2 : 1
			);
		}

		/**
		 * \~russian
		 * @brief Шаблон типа символа записываемой строки
		 *
		 * @tparam UC тип символа записываемой строки
		 *
		 * \~english
		 * @brief Template of the character type of the written string
		 * @tparam UC character type of the written string
		 *
		 * \~
		 */
		template <typename UC>
		/**
		 * \~russian
		 * @brief Метод записи десятичного значения в буфер справа налево
		 *
		 * @details Запись ведётся от конца отведённого места к началу, что избавляет
		 *          от разворота записи после её формирования.
		 *
		 * @param last  конец отведённого под запись места
		 * @param value записываемое значение
		 *
		 * \~english
		 * @brief Method of writing a decimal value into a buffer from right to left
		 * @details The writing is conducted from the end of the allotted place to the beginning,
		 *          which frees from the reversal of the record after its forming.
		 * @param last  end of the place allotted for the record
		 * @param value written value
		 *
		 * \~
		 */
		AWH_ASCII_INLINE void writeDecimal(UC * last, uint64_t value) noexcept {
			// Указатель на текущую позицию записи
			UC * position = last;
			/**
			 * Выполняем запись значения парами десятичных цифр
			 */
			while(value >= 100ull){
				// Определяем индекс записываемой пары цифр
				const uint32_t index = static_cast <uint32_t> ((value % 100ull) * 2ull);
				// Выполняем деление значения на сотню
				value /= 100ull;
				// Записываем младшую цифру пары
				*(--position) = static_cast <UC> (DIGITS_PAIRS[index + 1]);
				// Записываем старшую цифру пары
				*(--position) = static_cast <UC> (DIGITS_PAIRS[index]);
			}
			/**
			 * Если у значения остались две цифры
			 */
			if(value >= 10ull){
				// Определяем индекс записываемой пары цифр
				const uint32_t index = static_cast <uint32_t> (value * 2ull);
				// Записываем младшую цифру пары
				*(--position) = static_cast <UC> (DIGITS_PAIRS[index + 1]);
				// Записываем старшую цифру пары
				*(--position) = static_cast <UC> (DIGITS_PAIRS[index]);
			// Записываем единственную оставшуюся цифру значения
			} else *(--position) = static_cast <UC> ('0' + value);
		}

		/**
		 * \~russian
		 * @brief Шаблон типа символа записываемой строки
		 *
		 * @tparam UC тип символа записываемой строки
		 *
		 * \~english
		 * @brief Template of the character type of the written string
		 * @tparam UC character type of the written string
		 *
		 * \~
		 */
		template <typename UC>
		/**
		 * \~russian
		 * @brief Метод записи беззнакового значения в произвольной системе счисления
		 *
		 * @param first начало отведённого под запись места
		 * @param last  конец отведённого под запись места
		 * @param value записываемое значение
		 * @param base  основание системы счисления в диапазоне от 2 до 36
		 * @return      результат записи числовой строки
		 *
		 * \~english
		 * @brief Method of writing an unsigned value in an arbitrary numeral system
		 * @param first beginning of the place allotted for the record
		 * @param last  end of the place allotted for the record
		 * @param value written value
		 * @param base  base of the numeral system in the range from 2 to 36
		 * @return      result of writing the number string
		 *
		 * \~
		 */
		inline output_t <UC> writeUnsigned(UC * first, UC * const last, const uint64_t value, const int32_t base) noexcept {
			/**
			 * Если основание системы счисления является десятичным
			 */
			if(base == 10){
				// Определяем количество записываемых цифр
				const size_t length = decimalLength(value);
				// Если отведённого под запись места недостаточно
				if(static_cast <size_t> (last - first) < length)
					// Выводим результат неуспешной записи
					return output_t <UC> (last, errc::value_too_large, error_t::INSUFFICIENT_BUFFER);
				// Выполняем запись значения десятичными цифрами
				writeDecimal <UC> (first + length, value);
				// Выводим результат успешной записи
				return output_t <UC> (first + length);
			}
			// Хранилище цифр записи в обратном порядке
			UC digits[64];
			// Количество записанных цифр
			size_t count = 0;
			// Обрабатываемое значение
			uint64_t rest = value;
			/**
			 * Выполняем запись значения цифрами системы счисления
			 */
			do {
				// Записываем очередную цифру значения
				digits[count++] = static_cast <UC> (DIGITS_TABLE[rest % static_cast <uint64_t> (base)]);
				// Выполняем деление значения на основание системы счисления
				rest /= static_cast <uint64_t> (base);
			} while(rest != 0ull);
			// Если отведённого под запись места недостаточно
			if(static_cast <size_t> (last - first) < count)
				// Выводим результат неуспешной записи
				return output_t <UC> (last, errc::value_too_large, error_t::INSUFFICIENT_BUFFER);
			// Указатель на текущую позицию записи
			UC * position = first;
			/**
			 * Выполняем перенос цифр значения в прямом порядке
			 */
			for(size_t i = count; i-- > 0;)
				// Переносим очередную цифру значения
				*(position++) = digits[i];
			// Выводим результат успешной записи
			return output_t <UC> (position);
		}

		/**
		 * \~russian
		 * @brief Шаблон типа символа записываемой строки
		 *
		 * @tparam UC тип символа записываемой строки
		 *
		 * \~english
		 * @brief Template of the character type of the written string
		 * @tparam UC character type of the written string
		 *
		 * \~
		 */
		template <typename UC>
		/**
		 * \~russian
		 * @brief Метод записи десятичного представления числа с порядком
		 *
		 * @details Запись имеет вид одной цифры целой части, дробной части и показателя
		 *          степени десяти со знаком: 1.5e+300. Показатель записывается не менее
		 *          чем двумя цифрами, как это делает функция printf.
		 *
		 * @param first        начало отведённого под запись места
		 * @param last         конец отведённого под запись места
		 * @param decimal      десятичное представление числа
		 * @param decimalPoint символ десятичной точки
		 * @return             результат записи числовой строки
		 *
		 * \~english
		 * @brief Method of writing the decimal representation of a number with an exponent
		 * @details The record has the form of one digit of the integer part, of the fractional part
		 *          and of the exponent of the power of ten with a sign: 1.5e+300. The exponent is written
		 *          by no less than two digits, as the printf function does.
		 * @param first        beginning of the place allotted for the record
		 * @param last         end of the place allotted for the record
		 * @param decimal      decimal representation of the number
		 * @param decimalPoint decimal point character
		 * @return             result of writing the number string
		 *
		 * \~
		 */
		inline output_t <UC> writeScientific(UC * first, UC * const last, const decimal_t & decimal, const UC decimalPoint) noexcept {
			// Определяем количество значащих цифр мантиссы
			const size_t length = decimalLength(decimal.mantissa);
			// Определяем показатель степени десяти старшей цифры мантиссы
			int32_t exponent = (decimal.exponent + static_cast <int32_t> (length) - 1);
			// Определяем количество цифр записи показателя степени
			const size_t width = ((exponent >= 100) || (exponent <= -100)) ? 3 : 2;
			// Определяем полную длину записи числа
			const size_t total = (length + static_cast <size_t> (length > 1) + 2 + width);
			// Если отведённого под запись места недостаточно
			if(static_cast <size_t> (last - first) < total)
				// Выводим результат неуспешной записи
				return output_t <UC> (last, errc::value_too_large, error_t::INSUFFICIENT_BUFFER);
			/**
			 * Если мантисса состоит из единственной цифры
			 */
			if(length == 1)
				// Записываем единственную цифру мантиссы
				*first = static_cast <UC> ('0' + decimal.mantissa);
			/**
			 * Если мантисса состоит из нескольких цифр
			 */
			else {
				// Выполняем запись цифр мантиссы со сдвигом под десятичную точку
				writeDecimal <UC> (first + length + 1, decimal.mantissa);
				// Переносим старшую цифру мантиссы перед десятичной точкой
				first[0] = first[1];
				// Записываем символ десятичной точки
				first[1] = decimalPoint;
			}
			// Указатель на текущую позицию записи
			UC * position = (first + length + static_cast <size_t> (length > 1));
			// Записываем букву показателя степени
			*(position++) = static_cast <UC> ('e');
			/**
			 * Если показатель степени является отрицательным
			 */
			if(exponent < 0){
				// Записываем знак показателя степени
				*(position++) = static_cast <UC> ('-');
				// Выполняем смену знака показателя степени
				exponent = -exponent;
			// Записываем знак показателя степени
			} else *(position++) = static_cast <UC> ('+');
			// Выполняем запись показателя степени
			writeDecimal <UC> (position + width, static_cast <uint64_t> (exponent));
			/**
			 * Если запись показателя степени короче отведённого ей места
			 */
			if((width == 2) && (exponent < 10))
				// Дописываем показателю степени ведущий ноль
				position[0] = static_cast <UC> ('0');
			// Выводим результат успешной записи
			return output_t <UC> (position + width);
		}

		/**
		 * \~russian
		 * @brief Шаблон типа символа записываемой строки
		 *
		 * @tparam UC тип символа записываемой строки
		 *
		 * \~english
		 * @brief Template of the character type of the written string
		 * @tparam UC character type of the written string
		 *
		 * \~
		 */
		template <typename UC>
		/**
		 * \~russian
		 * @brief Метод записи десятичного представления числа без порядка
		 *
		 * @details Запись имеет вид целой и дробной частей без показателя степени.
		 *          Числу, меньшему единицы, дописывается ведущий ноль целой части.
		 *
		 * @param first        начало отведённого под запись места
		 * @param last         конец отведённого под запись места
		 * @param decimal      десятичное представление числа
		 * @param decimalPoint символ десятичной точки
		 * @return             результат записи числовой строки
		 *
		 * \~english
		 * @brief Method of writing the decimal representation of a number without an exponent
		 * @details The record has the form of the integer and of the fractional parts without an exponent.
		 *          A number smaller than one is given a leading zero of the integer part.
		 * @param first        beginning of the place allotted for the record
		 * @param last         end of the place allotted for the record
		 * @param decimal      decimal representation of the number
		 * @param decimalPoint decimal point character
		 * @return             result of writing the number string
		 *
		 * \~
		 */
		inline output_t <UC> writeFixed(UC * first, UC * const last, const decimal_t & decimal, const UC decimalPoint) noexcept {
			// Определяем количество значащих цифр мантиссы
			const size_t length = decimalLength(decimal.mantissa);
			// Определяем полную длину записи числа
			const size_t total = (
				(decimal.exponent >= 0) ? (length + static_cast <size_t> (decimal.exponent)) :
				((static_cast <size_t> (-decimal.exponent) < length) ? (length + 1) :
				 ((static_cast <size_t> (-decimal.exponent) - length) + length + 2))
			);
			// Если отведённого под запись места недостаточно
			if(static_cast <size_t> (last - first) < total)
				// Выводим результат неуспешной записи
				return output_t <UC> (last, errc::value_too_large, error_t::INSUFFICIENT_BUFFER);
			/**
			 * Если число дробной части не имеет
			 */
			if(decimal.exponent >= 0){
				// Выполняем запись цифр мантиссы
				writeDecimal <UC> (first + length, decimal.mantissa);
				/**
				 * Выполняем дописывание незначащих нулей целой части
				 */
				for(size_t i = 0; i < static_cast <size_t> (decimal.exponent); ++i)
					// Дописываем очередной незначащий ноль
					first[length + i] = static_cast <UC> ('0');
				// Выводим результат успешной записи
				return output_t <UC> (first + total);
			}
			// Определяем количество цифр дробной части числа
			const size_t fraction = static_cast <size_t> (-decimal.exponent);
			/**
			 * Если целая часть числа значащие цифры содержит
			 */
			if(fraction < length){
				// Выполняем запись цифр мантиссы со сдвигом под десятичную точку
				writeDecimal <UC> (first + length + 1, decimal.mantissa);
				/**
				 * Выполняем перенос цифр целой части на место перед десятичной точкой
				 */
				for(size_t i = 0; i < (length - fraction); ++i)
					// Переносим очередную цифру целой части
					first[i] = first[i + 1];
				// Записываем символ десятичной точки
				first[length - fraction] = decimalPoint;
				// Выводим результат успешной записи
				return output_t <UC> (first + total);
			}
			// Записываем ведущий ноль целой части
			first[0] = static_cast <UC> ('0');
			// Записываем символ десятичной точки
			first[1] = decimalPoint;
			/**
			 * Выполняем дописывание незначащих нулей дробной части
			 */
			for(size_t i = 0; i < (fraction - length); ++i)
				// Дописываем очередной незначащий ноль
				first[2 + i] = static_cast <UC> ('0');
			// Выполняем запись цифр мантиссы
			writeDecimal <UC> (first + total, decimal.mantissa);
			// Выводим результат успешной записи
			return output_t <UC> (first + total);
		}

		/**
		 * \~russian
		 * @brief Шаблон типа числа с плавающей точкой и типа символа записываемой строки
		 *
		 * @tparam T  тип числа с плавающей точкой
		 * @tparam UC тип символа записываемой строки
		 *
		 * \~english
		 * @brief Template of the floating-point type and of the character type of the written string
		 * @tparam T  floating-point type
		 * @tparam UC character type of the written string
		 *
		 * \~
		 */
		template <typename T, typename UC>
		/**
		 * \~russian
		 * @brief Метод записи числа с плавающей точкой кратчайшим обратимым представлением
		 *
		 * @details Запись содержит наименьшее количество значащих цифр, при котором она
		 *          читается обратно тем же самым числом. Вид записи задаётся форматом:
		 *          при указании единственного вида применяется он, при указании обоих
		 *          выбирается более короткая запись, а при равной длине — запись без
		 *          показателя степени.
		 *
		 * @param first   начало отведённого под запись места
		 * @param last    конец отведённого под запись места
		 * @param value   записываемое число
		 * @param options опции записи числовой строки
		 * @return        результат записи числовой строки
		 *
		 * \~english
		 * @brief Method of writing a floating-point number by the shortest reversible representation
		 * @details The record contains the least number of significant digits at which it
		 *          is read back as the very same number. The form of the record is set by the format:
		 *          at the indication of a single form it is applied, at the indication of both
		 *          the shorter record is chosen, and at an equal length — the record without
		 *          an exponent.
		 * @param first   beginning of the place allotted for the record
		 * @param last    end of the place allotted for the record
		 * @param value   written number
		 * @param options writing options of the number string
		 * @return        result of writing the number string
		 *
		 * \~
		 */
		inline output_t <UC> toCharsFloat(UC * first, UC * const last, const T value, const options_t <UC> options) noexcept {
			// Создаём тип беззнакового целого равной разрядности
			using uint_t = typename binary_t <T>::equiv_uint;
			// Двоичное представление записываемого числа
			uint_t bits = 0;
			// Выполняем извлечение двоичного представления числа
			::memcpy(&bits, &value, sizeof(bits));
			// Определяем знак записываемого числа
			const bool sign = ((bits >> binary_t <T>::signIndex()) != 0);
			// Извлекаем поле мантиссы двоичного представления
			const uint64_t mantissa = static_cast <uint64_t> (bits & binary_t <T>::mantissaMask());
			// Извлекаем поле порядка двоичного представления
			const int32_t exponent = static_cast <int32_t> ((bits & binary_t <T>::exponentMask()) >> binary_t <T>::mantissaExplicitBits());
			// Указатель на текущую позицию записи
			UC * position = first;
			/**
			 * Если записываемое число является отрицательным
			 *
			 * @note Знак выводится и у отрицательного нуля: он отличает его от нуля
			 *       положительного, и запись обязана читаться обратно тем же значением
			 */
			if(sign){
				// Если отведённого под запись места недостаточно
				if(position == last)
					// Выводим результат неуспешной записи
					return output_t <UC> (last, errc::value_too_large, error_t::INSUFFICIENT_BUFFER);
				// Записываем знак числа
				*(position++) = static_cast <UC> ('-');
			}
			/**
			 * Если записываемое число конечным не является
			 */
			if(exponent == binary_t <T>::infinitePower()){
				// Определяем записываемое слово вида числа
				const char * word = ((mantissa != 0ull) ? "nan" : "inf");
				// Если отведённого под запись места недостаточно
				if(static_cast <size_t> (last - position) < 3)
					// Выводим результат неуспешной записи
					return output_t <UC> (last, errc::value_too_large, error_t::INSUFFICIENT_BUFFER);
				/**
				 * Выполняем запись слова вида числа
				 */
				for(size_t i = 0; i < 3; ++i)
					// Записываем очередную букву слова вида числа
					*(position++) = static_cast <UC> (word[i]);
				// Выводим результат успешной записи
				return output_t <UC> (position);
			}
			/**
			 * Если записываемое число является нулевым
			 */
			if((exponent == 0) && (mantissa == 0ull)){
				// Если отведённого под запись места недостаточно
				if(position == last)
					// Выводим результат неуспешной записи
					return output_t <UC> (last, errc::value_too_large, error_t::INSUFFICIENT_BUFFER);
				// Записываем единственную цифру числа
				*(position++) = static_cast <UC> ('0');
				// Выводим результат успешной записи
				return output_t <UC> (position);
			}
			// Выполняем формирование кратчайшего десятичного представления числа
			const decimal_t decimal = shortest <T> (mantissa, exponent);
			// Определяем допустимость записи без показателя степени
			const bool fixed = ((options.format & format_t::FIXED) != format_t::NONE);
			// Определяем допустимость записи с показателем степени
			const bool scientific = ((options.format & format_t::SCIENTIFIC) != format_t::NONE);
			/**
			 * Если допустима запись единственного вида
			 */
			if(fixed != scientific){
				// Выводим результат записи числа допустимым видом
				return (fixed ?
					writeFixed <UC> (position, last, decimal, options.decimalPoint) :
					writeScientific <UC> (position, last, decimal, options.decimalPoint)
				);
			}
			// Определяем количество значащих цифр мантиссы
			const size_t length = decimalLength(decimal.mantissa);
			// Определяем показатель степени десяти старшей цифры мантиссы
			const int32_t power = (decimal.exponent + static_cast <int32_t> (length) - 1);
			// Определяем длину записи с показателем степени
			const size_t sizeScientific = (
				length + static_cast <size_t> (length > 1) + 2 + (((power >= 100) || (power <= -100)) ? 3 : 2)
			);
			// Определяем длину записи без показателя степени
			const size_t sizeFixed = (
				(decimal.exponent >= 0) ? (length + static_cast <size_t> (decimal.exponent)) :
				((static_cast <size_t> (-decimal.exponent) < length) ? (length + 1) :
				 (static_cast <size_t> (-decimal.exponent) + 2))
			);
			// Выводим результат записи числа более коротким видом
			return ((sizeFixed <= sizeScientific) ?
				writeFixed <UC> (position, last, decimal, options.decimalPoint) :
				writeScientific <UC> (position, last, decimal, options.decimalPoint)
			);
		}
	};
};

#endif // __AWH_LEXICAL_WRITER__
