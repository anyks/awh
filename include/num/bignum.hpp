/**
 * @file: bignum.hpp
 * @date: 2026-07-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл модуля длинных чисел — шаблонный класс BigNum, хранящий число произвольной разрядности
 *        в массиве байтов фиксированного размера и поддерживающий знаковую, беззнаковую и вещественную арифметику
 *        с полной совместимостью со встроенными числовыми типами, строками и потоками ввода/вывода
 *
 * \~english
 * @brief Header file of the long number module — the BigNum template class, which keeps a number of an arbitrary bit width
 *        in an array of bytes of a fixed size and supports signed, unsigned and real arithmetic
 *        with full compatibility with the built-in numeric types, strings and input/output streams
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_BIGNUM__
#define __AWH_BIGNUM__

/**
 * Стандартные заголовочные файлы
 */
#include <array>
#include <string>
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <string_view>
#include <type_traits>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../sys/global.hpp"

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
	 * @brief Пространство имён модуля длинных чисел
	 *
	 * @details Пространство имён содержит размер-независимый вычислительный движок,
	 *          выполняющий все арифметические операции над сырыми буферами байтов.
	 *          Буфер числа всегда хранится в порядке от младшего байта к старшему
	 *          (little-endian) вне зависимости от порядка байтов процессора, знаковые
	 *          числа хранятся в дополнительном коде, беззнаковые — в прямом коде,
	 *          вещественные — в формате IEEE-754 binary(N).
	 *
	 * \~english
	 * @brief Namespace of the long number module
	 * @details The namespace holds the size-independent computation engine,
	 *          which performs all the arithmetic operations over raw byte buffers.
	 *          The buffer of a number is always kept in the order from the low byte to the high one
	 *          (little-endian) regardless of the byte order of the processor, signed
	 *          numbers are kept in two's complement, unsigned ones in sign-magnitude,
	 *          real ones in the IEEE-754 binary(N) format.
	 *
	 * \~
	 */
	namespace bignum {
		/**
		 * \~russian
		 * @brief Тип хранимого числа
		 *
		 * \~english
		 * @brief Type of the kept number
		 *
		 * \~
		 */
		enum class type_t : uint8_t {
			NONE     = 0x00, // Тип числа не установлен
			SIGNED   = 0x01, // Знаковое целое число в дополнительном коде
			UNSIGNED = 0x02, // Беззнаковое целое число
			REAL     = 0x03  // Вещественное число в формате IEEE-754
		};
		/**
		 * \~russian
		 * @brief Формат представления числа в виде строки
		 *
		 * \~english
		 * @brief Format of the representation of a number as a string
		 *
		 * \~
		 */
		enum class format_t : uint8_t {
			NONE  = 0x00, // Формат определяется автоматически по префиксу строки
			BIN   = 0x01, // Двоичный формат представления числа
			OCT   = 0x02, // Восьмеричный формат представления числа
			DEC   = 0x03, // Десятичный формат представления числа
			HEX   = 0x04, // Шестнадцатеричный формат представления числа в нижнем регистре
			HEXUP = 0x05, // Шестнадцатеричный формат представления числа в верхнем регистре
			SCI   = 0x06  // Научная нотация представления вещественного числа
		};
		/**
		 * \~russian
		 * @brief Правило округления числа
		 *
		 * \~english
		 * @brief Rounding rule of a number
		 *
		 * \~
		 */
		enum class round_t : uint8_t {
			NEAREST = 0x00, // К ближайшему значению, половина округляется от нуля
			EVEN    = 0x01, // К ближайшему значению, половина округляется к чётному
			DOWN    = 0x02, // В сторону минус бесконечности
			UP      = 0x03, // В сторону плюс бесконечности
			ZERO    = 0x04  // В сторону нуля с отбрасыванием младших разрядов
		};
		/**
		 * \~russian
		 * @brief Класс значения числа
		 *
		 * \~english
		 * @brief Class of the value of a number
		 *
		 * \~
		 */
		enum class class_t : uint8_t {
			ZERO      = 0x00, // Нулевое значение числа
			SUBNORMAL = 0x01, // Денормализованное значение числа
			NORMAL    = 0x02, // Нормализованное значение числа
			UNLIMITED = 0x03, // Бесконечность
			UNDEFINED = 0x04  // Значение не является числом (NaN)
		};
	};
	/**
	 * \~russian
	 * @brief Пространство имён вычислительного движка длинных чисел
	 *
	 * \~english
	 * @brief Namespace of the computation engine of long numbers
	 *
	 * \~
	 */
	namespace bignum {
		/**
		 * \~russian
		 * @brief Метод обнуления буфера числа
		 *
		 * @param value буфер числа для обнуления
		 * @param size  размер буфера числа в байтах
		 *
		 * \~english
		 * @brief Method of zeroing the buffer of a number
		 * @param value buffer of the number to zero
		 * @param size  size of the buffer of the number in bytes
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void reset(uint8_t * value, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод проверки буфера числа на нулевое значение
		 *
		 * @param value буфер числа для проверки
		 * @param size  размер буфера числа в байтах
		 * @return      результат проверки
		 *
		 * \~english
		 * @brief Method of checking the buffer of a number for a zero value
		 * @param value buffer of the number to check
		 * @param size  size of the buffer of the number in bytes
		 * @return      result of the check
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool zero(const uint8_t * value, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод извлечения количества значащих бит числа
		 *
		 * @param value буфер числа для подсчёта
		 * @param size  размер буфера числа в байтах
		 * @return      позиция старшего установленного бита увеличенная на единицу
		 *
		 * \~english
		 * @brief Method of getting the number of significant bits of a number
		 * @param value buffer of the number to count over
		 * @param size  size of the buffer of the number in bytes
		 * @return      position of the highest set bit increased by one
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ size_t bits(const uint8_t * value, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод извлечения значения бита числа
		 *
		 * @param value буфер числа для извлечения
		 * @param size  размер буфера числа в байтах
		 * @param index индекс извлекаемого бита
		 * @return      значение указанного бита
		 *
		 * \~english
		 * @brief Method of getting the value of a bit of a number
		 * @param value buffer of the number to get from
		 * @param size  size of the buffer of the number in bytes
		 * @param index index of the bit to get
		 * @return      value of the specified bit
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool bit(const uint8_t * value, const size_t size, const size_t index) noexcept;
		/**
		 * \~russian
		 * @brief Метод установки значения бита числа
		 *
		 * @param value буфер числа для установки
		 * @param size  размер буфера числа в байтах
		 * @param index индекс устанавливаемого бита
		 * @param mode  устанавливаемое значение бита
		 *
		 * \~english
		 * @brief Method of setting the value of a bit of a number
		 * @param value buffer of the number to set in
		 * @param size  size of the buffer of the number in bytes
		 * @param index index of the bit to set
		 * @param mode  value of the bit being set
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void bit(uint8_t * value, const size_t size, const size_t index, const bool mode) noexcept;
		/**
		 * \~russian
		 * @brief Метод проверки числа на отрицательное значение
		 *
		 * @param value буфер числа для проверки
		 * @param size  размер буфера числа в байтах
		 * @return      результат проверки старшего бита числа
		 *
		 * \~english
		 * @brief Method of checking a number for a negative value
		 * @param value buffer of the number to check
		 * @param size  size of the buffer of the number in bytes
		 * @return      result of checking the highest bit of the number
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool negative(const uint8_t * value, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод сложения двух целых чисел
		 *
		 * @param result буфер числа приёмника и первого слагаемого
		 * @param value  буфер числа второго слагаемого
		 * @param size   размер буферов чисел в байтах
		 * @return       флаг переноса за пределы разрядной сетки
		 *
		 * \~english
		 * @brief Method of adding two integers
		 * @param result buffer of the destination number and of the first addend
		 * @param value  buffer of the number of the second addend
		 * @param size   size of the buffers of the numbers in bytes
		 * @return       carry flag beyond the bounds of the bit grid
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool add(uint8_t * result, const uint8_t * value, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод вычитания двух целых чисел
		 *
		 * @param result буфер числа приёмника и уменьшаемого
		 * @param value  буфер числа вычитаемого
		 * @param size   размер буферов чисел в байтах
		 * @return       флаг заёма за пределами разрядной сетки
		 *
		 * \~english
		 * @brief Method of subtracting two integers
		 * @param result buffer of the destination number and of the minuend
		 * @param value  buffer of the number of the subtrahend
		 * @param size   size of the buffers of the numbers in bytes
		 * @return       borrow flag beyond the bounds of the bit grid
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool sub(uint8_t * result, const uint8_t * value, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод умножения двух целых чисел
		 *
		 * @details Результат умножения усекается до размера разрядной сетки.
		 *
		 * @param result буфер числа приёмника и множимого
		 * @param value  буфер числа множителя
		 * @param size   размер буферов чисел в байтах
		 *
		 * \~english
		 * @brief Method of multiplying two integers
		 * @details The result of the multiplication is truncated to the size of the bit grid.
		 * @param result buffer of the destination number and of the multiplicand
		 * @param value  buffer of the number of the multiplier
		 * @param size   size of the buffers of the numbers in bytes
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void mul(uint8_t * result, const uint8_t * value, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод деления двух беззнаковых целых чисел с получением остатка
		 *
		 * @details При делении на нуль частное и остаток обнуляются, метод выводит
		 *          отрицательный результат. Буфер остатка допускается совмещать с буфером
		 *          делителя, но не с буфером частного: частное и остаток являются двумя
		 *          независимыми результатами и в одном буфере не помещаются, поэтому при
		 *          совмещении буферов деление не выполняется и метод выводит отрицательный
		 *          результат, оставляя переданные буферы без изменений. Буфер остатка
		 *          допускается не передавать, указав нулевой указатель.
		 *
		 * @param result буфер числа приёмника частного и делимого
		 * @param value  буфер числа делителя
		 * @param mod    буфер числа приёмника остатка от деления
		 * @param size   размер буферов чисел в байтах
		 * @return       результат выполнения деления
		 *
		 * \~english
		 * @brief Method of dividing two unsigned integers with getting the remainder
		 * @details On division by zero the quotient and the remainder are zeroed, the method yields
		 *          a negative result. The buffer of the remainder is allowed to coincide with the buffer
		 *          of the divisor, but not with the buffer of the quotient: the quotient and the remainder are two
		 *          independent results and do not fit into one buffer, therefore on
		 *          coinciding buffers the division is not performed and the method yields a negative
		 *          result, leaving the passed buffers unchanged. The buffer of the remainder
		 *          is allowed not to be passed, by specifying a null pointer.
		 * @param result buffer of the destination number of the quotient and of the dividend
		 * @param value  buffer of the number of the divisor
		 * @param mod    buffer of the destination number of the remainder of the division
		 * @param size   size of the buffers of the numbers in bytes
		 * @return       result of performing the division
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool divmod(uint8_t * result, const uint8_t * value, uint8_t * mod, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод смены знака целого числа в дополнительном коде
		 *
		 * @param value буфер числа для смены знака
		 * @param size  размер буфера числа в байтах
		 *
		 * \~english
		 * @brief Method of changing the sign of an integer in two's complement
		 * @param value buffer of the number to change the sign of
		 * @param size  size of the buffer of the number in bytes
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void neg(uint8_t * value, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод побитовой инверсии целого числа
		 *
		 * @param value буфер числа для инверсии
		 * @param size  размер буфера числа в байтах
		 *
		 * \~english
		 * @brief Method of the bitwise inversion of an integer
		 * @param value buffer of the number to invert
		 * @param size  size of the buffer of the number in bytes
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void inv(uint8_t * value, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод побитового умножения двух целых чисел
		 *
		 * @param result буфер числа приёмника и первого операнда
		 * @param value  буфер числа второго операнда
		 * @param size   размер буферов чисел в байтах
		 *
		 * \~english
		 * @brief Method of the bitwise multiplication of two integers
		 * @param result buffer of the destination number and of the first operand
		 * @param value  buffer of the number of the second operand
		 * @param size   size of the buffers of the numbers in bytes
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void band(uint8_t * result, const uint8_t * value, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод побитового сложения двух целых чисел
		 *
		 * @param result буфер числа приёмника и первого операнда
		 * @param value  буфер числа второго операнда
		 * @param size   размер буферов чисел в байтах
		 *
		 * \~english
		 * @brief Method of the bitwise addition of two integers
		 * @param result buffer of the destination number and of the first operand
		 * @param value  buffer of the number of the second operand
		 * @param size   size of the buffers of the numbers in bytes
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void bor(uint8_t * result, const uint8_t * value, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод побитового исключающего сложения двух целых чисел
		 *
		 * @param result буфер числа приёмника и первого операнда
		 * @param value  буфер числа второго операнда
		 * @param size   размер буферов чисел в байтах
		 *
		 * \~english
		 * @brief Method of the bitwise exclusive addition of two integers
		 * @param result buffer of the destination number and of the first operand
		 * @param value  buffer of the number of the second operand
		 * @param size   size of the buffers of the numbers in bytes
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void bxor(uint8_t * result, const uint8_t * value, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод сдвига целого числа влево
		 *
		 * @param value буфер числа для сдвига
		 * @param size  размер буфера числа в байтах
		 * @param count количество бит сдвига
		 *
		 * \~english
		 * @brief Method of shifting an integer left
		 * @param value buffer of the number to shift
		 * @param size  size of the buffer of the number in bytes
		 * @param count number of bits of the shift
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void shl(uint8_t * value, const size_t size, const size_t count) noexcept;
		/**
		 * \~russian
		 * @brief Метод сдвига целого числа вправо
		 *
		 * @param value буфер числа для сдвига
		 * @param size  размер буфера числа в байтах
		 * @param count количество бит сдвига
		 * @param sign  флаг выполнения арифметического сдвига с сохранением знака
		 *
		 * \~english
		 * @brief Method of shifting an integer right
		 * @param value buffer of the number to shift
		 * @param size  size of the buffer of the number in bytes
		 * @param count number of bits of the shift
		 * @param sign  flag of performing an arithmetic shift preserving the sign
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void shr(uint8_t * value, const size_t size, const size_t count, const bool sign) noexcept;
		/**
		 * \~russian
		 * @brief Метод сравнения двух беззнаковых целых чисел
		 *
		 * @param value1 буфер первого числа для сравнения
		 * @param value2 буфер второго числа для сравнения
		 * @param size   размер буферов чисел в байтах
		 * @return       результат сравнения (-1, 0 или 1)
		 *
		 * \~english
		 * @brief Method of comparing two unsigned integers
		 * @param value1 buffer of the first number to compare
		 * @param value2 buffer of the second number to compare
		 * @param size   size of the buffers of the numbers in bytes
		 * @return       result of the comparison (-1, 0 or 1)
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ int8_t ucompare(const uint8_t * value1, const uint8_t * value2, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод сравнения двух знаковых целых чисел
		 *
		 * @param value1 буфер первого числа для сравнения
		 * @param value2 буфер второго числа для сравнения
		 * @param size   размер буферов чисел в байтах
		 * @return       результат сравнения (-1, 0 или 1)
		 *
		 * \~english
		 * @brief Method of comparing two signed integers
		 * @param value1 buffer of the first number to compare
		 * @param value2 buffer of the second number to compare
		 * @param size   size of the buffers of the numbers in bytes
		 * @return       result of the comparison (-1, 0 or 1)
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ int8_t scompare(const uint8_t * value1, const uint8_t * value2, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод извлечения целочисленного квадратного корня беззнакового числа
		 *
		 * @param value буфер числа для извлечения корня
		 * @param size  размер буфера числа в байтах
		 *
		 * \~english
		 * @brief Method of taking the integer square root of an unsigned number
		 * @param value buffer of the number to take the root of
		 * @param size  size of the buffer of the number in bytes
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void sqrt(uint8_t * value, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод возведения целого числа в степень
		 *
		 * @param value    буфер числа для возведения в степень
		 * @param size     размер буфера числа в байтах
		 * @param exponent показатель степени
		 *
		 * \~english
		 * @brief Method of raising an integer to a power
		 * @param value    buffer of the number to raise to a power
		 * @param size     size of the buffer of the number in bytes
		 * @param exponent exponent of the power
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void pow(uint8_t * value, const size_t size, const uint64_t exponent) noexcept;
		/**
		 * \~russian
		 * @brief Метод установки значения целого числа
		 *
		 * @param value буфер числа для установки
		 * @param size  размер буфера числа в байтах
		 * @param num   устанавливаемое значение по модулю
		 * @param sign  флаг отрицательного значения устанавливаемого числа
		 *
		 * \~english
		 * @brief Method of setting the value of an integer
		 * @param value buffer of the number to set in
		 * @param size  size of the buffer of the number in bytes
		 * @param num   value being set by modulus
		 * @param sign  flag of a negative value of the number being set
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void set(uint8_t * value, const size_t size, const uint64_t num, const bool sign) noexcept;
		/**
		 * \~russian
		 * @brief Метод извлечения беззнакового целого значения числа
		 *
		 * @param value буфер числа для извлечения
		 * @param size  размер буфера числа в байтах
		 * @return      извлечённое значение младших разрядов числа
		 *
		 * \~english
		 * @brief Method of getting the unsigned integer value of a number
		 * @param value buffer of the number to get from
		 * @param size  size of the buffer of the number in bytes
		 * @return      the obtained value of the low limbs of the number
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ uint64_t getUint(const uint8_t * value, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод извлечения знакового целого значения числа
		 *
		 * @param value буфер числа для извлечения
		 * @param size  размер буфера числа в байтах
		 * @return      извлечённое значение младших разрядов числа
		 *
		 * \~english
		 * @brief Method of getting the signed integer value of a number
		 * @param value buffer of the number to get from
		 * @param size  size of the buffer of the number in bytes
		 * @return      the obtained value of the low limbs of the number
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ int64_t getInt(const uint8_t * value, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод извлечения вещественного значения целого числа
		 *
		 * @param value буфер числа для извлечения
		 * @param size  размер буфера числа в байтах
		 * @param sign  флаг знакового представления числа
		 * @return      извлечённое вещественное значение числа
		 *
		 * \~english
		 * @brief Method of getting the real value of an integer
		 * @param value buffer of the number to get from
		 * @param size  size of the buffer of the number in bytes
		 * @param sign  flag of the signed representation of the number
		 * @return      the obtained real value of the number
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ long double getReal(const uint8_t * value, const size_t size, const bool sign) noexcept;
		/**
		 * \~russian
		 * @brief Метод установки вещественного значения целого числа
		 *
		 * @details Дробная часть устанавливаемого значения отбрасывается. При выходе значения
		 *          за пределы разрядной сетки числа выполняется насыщение её предельным
		 *          значением, при снятом флаге знакового представления отрицательное значение
		 *          насыщается нулём, а значение, не являющееся числом, обнуляет число.
		 *
		 * @param value буфер числа для установки
		 * @param size  размер буфера числа в байтах
		 * @param num   устанавливаемое вещественное значение
		 * @param sign  флаг знакового представления числа
		 *
		 * \~english
		 * @brief Method of setting the real value of an integer
		 * @details The fractional part of the value being set is discarded. When the value goes
		 *          beyond the bounds of the bit grid of the number it is saturated with the limiting
		 *          value, when the flag of the signed representation is cleared a negative value
		 *          is saturated with zero, and a value that is not a number zeroes the number.
		 * @param value buffer of the number to set in
		 * @param size  size of the buffer of the number in bytes
		 * @param num   real value being set
		 * @param sign  flag of the signed representation of the number
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void setReal(uint8_t * value, const size_t size, const long double num, const bool sign) noexcept;
		/**
		 * \~russian
		 * @brief Метод формирования строкового представления целого числа
		 *
		 * @param value  буфер числа для формирования
		 * @param size   размер буфера числа в байтах
		 * @param sign   флаг знакового представления числа
		 * @param format формат представления числа
		 * @return       сформированная строка числа
		 *
		 * \~english
		 * @brief Method of building the string representation of an integer
		 * @param value  buffer of the number to build from
		 * @param size   size of the buffer of the number in bytes
		 * @param sign   flag of the signed representation of the number
		 * @param format format of the representation of the number
		 * @return       the built string of the number
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ string print(const uint8_t * value, const size_t size, const bool sign, const format_t format) noexcept;
		/**
		 * \~russian
		 * @brief Метод разбора строкового представления целого числа
		 *
		 * @details Разбор прекращается на первом символе, не принадлежащем числу, а уже
		 *          разобранная часть сохраняется. При снятом флаге знакового представления
		 *          ведущий минус частью числа не считается: отрицательное значение беззнаковым
		 *          представлением не выражается, поэтому разбор завершается отказом и число
		 *          остаётся нулевым, как это делает функция std::from_chars.
		 *
		 * @param value  буфер числа для установки результата разбора
		 * @param size   размер буфера числа в байтах
		 * @param text   разбираемая строка числа
		 * @param sign   флаг знакового представления числа
		 * @param format формат представления числа
		 * @return       результат выполнения разбора
		 *
		 * \~english
		 * @brief Method of parsing the string representation of an integer
		 * @details The parsing stops at the first character not belonging to the number, while the already
		 *          parsed part is kept. When the flag of the signed representation is cleared the
		 *          leading minus is not considered a part of the number: a negative value is not expressed by an unsigned
		 *          representation, therefore the parsing ends in a failure and the number
		 *          stays zero, as the std::from_chars function does.
		 * @param value  buffer of the number to set the result of the parsing in
		 * @param size   size of the buffer of the number in bytes
		 * @param text   string of the number being parsed
		 * @param sign   flag of the signed representation of the number
		 * @param format format of the representation of the number
		 * @return       result of performing the parsing
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool parse(uint8_t * value, const size_t size, string_view text, const bool sign, const format_t format) noexcept;
		/**
		 * \~russian
		 * @brief Метод округления целого числа до указанного десятичного разряда
		 *
		 * @details Неотрицательное количество знаков оставляет целое число без изменений,
		 *          отрицательное округляет его до соответствующей степени десяти: значение
		 *          -3 округляет число до тысяч.
		 *
		 * @param value  буфер числа для округления
		 * @param size   размер буфера числа в байтах
		 * @param sign   флаг знакового представления числа
		 * @param digits количество знаков после запятой
		 * @param mode   правило округления числа
		 *
		 * \~english
		 * @brief Method of rounding an integer to the specified decimal place
		 * @details A non-negative number of digits leaves an integer unchanged,
		 *          a negative one rounds it to the corresponding power of ten: the value
		 *          -3 rounds the number to thousands.
		 * @param value  buffer of the number to round
		 * @param size   size of the buffer of the number in bytes
		 * @param sign   flag of the signed representation of the number
		 * @param digits number of digits after the decimal point
		 * @param mode   rounding rule of the number
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void round(uint8_t * value, const size_t size, const bool sign, const int32_t digits, const round_t mode) noexcept;
	};
	/**
	 * \~russian
	 * @brief Пространство имён вещественной арифметики длинных чисел
	 *
	 * \~english
	 * @brief Namespace of the real arithmetic of long numbers
	 *
	 * \~
	 */
	namespace bignum {
		/**
		 * \~russian
		 * @brief Метод извлечения разрядности порядка вещественного числа
		 *
		 * @details Для стандартных разрядностей 16, 32, 64 и 128 бит используется
		 *          разрядность порядка, определённая стандартом IEEE-754, для
		 *          остальных разрядностей применяется формула round(4 * log2(N)) - 13.
		 *
		 * @param bits разрядность вещественного числа в битах
		 * @return     разрядность порядка вещественного числа в битах
		 *
		 * \~english
		 * @brief Method of getting the bit width of the exponent of a real number
		 * @details For the standard bit widths of 16, 32, 64 and 128 bits the exponent
		 *          bit width defined by the IEEE-754 standard is used, for the
		 *          other bit widths the formula round(4 * log2(N)) - 13 is applied.
		 * @param bits bit width of the real number in bits
		 * @return     bit width of the exponent of the real number in bits
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ size_t exponentBits(const size_t bits) noexcept;
		/**
		 * \~russian
		 * @brief Метод определения класса вещественного числа
		 *
		 * @param value буфер числа для определения
		 * @param size  размер буфера числа в байтах
		 * @return      класс значения вещественного числа
		 *
		 * \~english
		 * @brief Method of determining the class of a real number
		 * @param value buffer of the number to determine for
		 * @param size  size of the buffer of the number in bytes
		 * @return      class of the value of the real number
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ class_t classify(const uint8_t * value, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод формирования бесконечности вещественного числа
		 *
		 * @param value буфер числа для формирования
		 * @param size  размер буфера числа в байтах
		 * @param sign  флаг формирования отрицательной бесконечности
		 *
		 * \~english
		 * @brief Method of building the infinity of a real number
		 * @param value buffer of the number to build in
		 * @param size  size of the buffer of the number in bytes
		 * @param sign  flag of building a negative infinity
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void realInf(uint8_t * value, const size_t size, const bool sign) noexcept;
		/**
		 * \~russian
		 * @brief Метод формирования значения не являющегося числом
		 *
		 * @param value буфер числа для формирования
		 * @param size  размер буфера числа в байтах
		 *
		 * \~english
		 * @brief Method of building a value that is not a number
		 * @param value buffer of the number to build in
		 * @param size  size of the buffer of the number in bytes
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void realNan(uint8_t * value, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод формирования машинного эпсилон вещественного числа
		 *
		 * @param value буфер числа для формирования
		 * @param size  размер буфера числа в байтах
		 *
		 * \~english
		 * @brief Method of building the machine epsilon of a real number
		 * @param value buffer of the number to build in
		 * @param size  size of the buffer of the number in bytes
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void realEpsilon(uint8_t * value, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод формирования максимального конечного вещественного числа
		 *
		 * @param value буфер числа для формирования
		 * @param size  размер буфера числа в байтах
		 * @param sign  флаг формирования минимального отрицательного числа
		 *
		 * \~english
		 * @brief Method of building the maximum finite real number
		 * @param value buffer of the number to build in
		 * @param size  size of the buffer of the number in bytes
		 * @param sign  flag of building the minimum negative number
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void realLimit(uint8_t * value, const size_t size, const bool sign) noexcept;
		/**
		 * \~russian
		 * @brief Метод смены знака вещественного числа
		 *
		 * @param value буфер числа для смены знака
		 * @param size  размер буфера числа в байтах
		 *
		 * \~english
		 * @brief Method of changing the sign of a real number
		 * @param value buffer of the number to change the sign of
		 * @param size  size of the buffer of the number in bytes
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void realNeg(uint8_t * value, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод извлечения модуля вещественного числа
		 *
		 * @param value буфер числа для извлечения модуля
		 * @param size  размер буфера числа в байтах
		 *
		 * \~english
		 * @brief Method of taking the modulus of a real number
		 * @param value buffer of the number to take the modulus of
		 * @param size  size of the buffer of the number in bytes
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void realAbs(uint8_t * value, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод сложения двух вещественных чисел
		 *
		 * @param result буфер числа приёмника и первого слагаемого
		 * @param value  буфер числа второго слагаемого
		 * @param size   размер буферов чисел в байтах
		 *
		 * \~english
		 * @brief Method of adding two real numbers
		 * @param result buffer of the destination number and of the first addend
		 * @param value  buffer of the number of the second addend
		 * @param size   size of the buffers of the numbers in bytes
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void realAdd(uint8_t * result, const uint8_t * value, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод вычитания двух вещественных чисел
		 *
		 * @param result буфер числа приёмника и уменьшаемого
		 * @param value  буфер числа вычитаемого
		 * @param size   размер буферов чисел в байтах
		 *
		 * \~english
		 * @brief Method of subtracting two real numbers
		 * @param result buffer of the destination number and of the minuend
		 * @param value  buffer of the number of the subtrahend
		 * @param size   size of the buffers of the numbers in bytes
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void realSub(uint8_t * result, const uint8_t * value, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод умножения двух вещественных чисел
		 *
		 * @param result буфер числа приёмника и множимого
		 * @param value  буфер числа множителя
		 * @param size   размер буферов чисел в байтах
		 *
		 * \~english
		 * @brief Method of multiplying two real numbers
		 * @param result buffer of the destination number and of the multiplicand
		 * @param value  buffer of the number of the multiplier
		 * @param size   size of the buffers of the numbers in bytes
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void realMul(uint8_t * result, const uint8_t * value, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод деления двух вещественных чисел
		 *
		 * @param result буфер числа приёмника и делимого
		 * @param value  буфер числа делителя
		 * @param size   размер буферов чисел в байтах
		 *
		 * \~english
		 * @brief Method of dividing two real numbers
		 * @param result buffer of the destination number and of the dividend
		 * @param value  buffer of the number of the divisor
		 * @param size   size of the buffers of the numbers in bytes
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void realDiv(uint8_t * result, const uint8_t * value, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод извлечения остатка от деления двух вещественных чисел
		 *
		 * @param result буфер числа приёмника и делимого
		 * @param value  буфер числа делителя
		 * @param size   размер буферов чисел в байтах
		 *
		 * \~english
		 * @brief Method of getting the remainder of the division of two real numbers
		 * @param result buffer of the destination number and of the dividend
		 * @param value  buffer of the number of the divisor
		 * @param size   size of the buffers of the numbers in bytes
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void realMod(uint8_t * result, const uint8_t * value, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод извлечения квадратного корня вещественного числа
		 *
		 * @param value буфер числа для извлечения корня
		 * @param size  размер буфера числа в байтах
		 *
		 * \~english
		 * @brief Method of taking the square root of a real number
		 * @param value buffer of the number to take the root of
		 * @param size  size of the buffer of the number in bytes
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void realSqrt(uint8_t * value, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод возведения вещественного числа в целую степень
		 *
		 * @param value    буфер числа для возведения в степень
		 * @param size     размер буфера числа в байтах
		 * @param exponent показатель степени
		 *
		 * \~english
		 * @brief Method of raising a real number to an integer power
		 * @param value    buffer of the number to raise to a power
		 * @param size     size of the buffer of the number in bytes
		 * @param exponent exponent of the power
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void realPow(uint8_t * value, const size_t size, const uint64_t exponent) noexcept;
		/**
		 * \~russian
		 * @brief Метод сравнения двух вещественных чисел
		 *
		 * @param value1 буфер первого числа для сравнения
		 * @param value2 буфер второго числа для сравнения
		 * @param size   размер буферов чисел в байтах
		 * @return       результат сравнения (-1, 0, 1 или 2 для несравнимых значений)
		 *
		 * \~english
		 * @brief Method of comparing two real numbers
		 * @param value1 buffer of the first number to compare
		 * @param value2 buffer of the second number to compare
		 * @param size   size of the buffers of the numbers in bytes
		 * @return       result of the comparison (-1, 0, 1 or 2 for incomparable values)
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ int8_t realCompare(const uint8_t * value1, const uint8_t * value2, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод установки целого значения вещественного числа
		 *
		 * @param value буфер числа для установки
		 * @param size  размер буфера числа в байтах
		 * @param num   устанавливаемое значение по модулю
		 * @param sign  флаг отрицательного значения устанавливаемого числа
		 *
		 * \~english
		 * @brief Method of setting the integer value of a real number
		 * @param value buffer of the number to set in
		 * @param size  size of the buffer of the number in bytes
		 * @param num   value being set by modulus
		 * @param sign  flag of a negative value of the number being set
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void realSet(uint8_t * value, const size_t size, const uint64_t num, const bool sign) noexcept;
		/**
		 * \~russian
		 * @brief Метод установки вещественного значения числа
		 *
		 * @param value буфер числа для установки
		 * @param size  размер буфера числа в байтах
		 * @param num   устанавливаемое вещественное значение
		 *
		 * \~english
		 * @brief Method of setting the real value of a number
		 * @param value buffer of the number to set in
		 * @param size  size of the buffer of the number in bytes
		 * @param num   real value being set
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void realSetReal(uint8_t * value, const size_t size, const long double num) noexcept;
		/**
		 * \~russian
		 * @brief Метод извлечения беззнакового целого значения вещественного числа
		 *
		 * @param value буфер числа для извлечения
		 * @param size  размер буфера числа в байтах
		 * @return      извлечённое целое значение числа
		 *
		 * \~english
		 * @brief Method of getting the unsigned integer value of a real number
		 * @param value buffer of the number to get from
		 * @param size  size of the buffer of the number in bytes
		 * @return      the obtained integer value of the number
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ uint64_t realGetUint(const uint8_t * value, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод извлечения знакового целого значения вещественного числа
		 *
		 * @param value буфер числа для извлечения
		 * @param size  размер буфера числа в байтах
		 * @return      извлечённое целое значение числа
		 *
		 * \~english
		 * @brief Method of getting the signed integer value of a real number
		 * @param value buffer of the number to get from
		 * @param size  size of the buffer of the number in bytes
		 * @return      the obtained integer value of the number
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ int64_t realGetInt(const uint8_t * value, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод извлечения вещественного значения числа
		 *
		 * @param value буфер числа для извлечения
		 * @param size  размер буфера числа в байтах
		 * @return      извлечённое вещественное значение числа
		 *
		 * \~english
		 * @brief Method of getting the real value of a number
		 * @param value buffer of the number to get from
		 * @param size  size of the buffer of the number in bytes
		 * @return      the obtained real value of the number
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ long double realGetReal(const uint8_t * value, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Метод переноса вещественного числа в буфер другой разрядности
		 *
		 * @param result буфер числа приёмника
		 * @param size1  размер буфера числа приёмника в байтах
		 * @param value  буфер числа источника
		 * @param size2  размер буфера числа источника в байтах
		 *
		 * \~english
		 * @brief Method of transferring a real number into a buffer of another bit width
		 * @param result buffer of the destination number
		 * @param size1  size of the buffer of the destination number in bytes
		 * @param value  buffer of the source number
		 * @param size2  size of the buffer of the source number in bytes
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void realCast(uint8_t * result, const size_t size1, const uint8_t * value, const size_t size2) noexcept;
		/**
		 * \~russian
		 * @brief Метод формирования строкового представления вещественного числа
		 *
		 * @param value     буфер числа для формирования
		 * @param size      размер буфера числа в байтах
		 * @param format    формат представления числа
		 * @param precision количество знаков после запятой (отрицательное значение - автоматически)
		 * @return          сформированная строка числа
		 *
		 * \~english
		 * @brief Method of building the string representation of a real number
		 * @param value     buffer of the number to build from
		 * @param size      size of the buffer of the number in bytes
		 * @param format    format of the representation of the number
		 * @param precision number of digits after the decimal point (a negative value means automatic)
		 * @return          the built string of the number
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ string realPrint(const uint8_t * value, const size_t size, const format_t format, const int16_t precision) noexcept;
		/**
		 * \~russian
		 * @brief Метод разбора строкового представления вещественного числа
		 *
		 * @param value буфер числа для установки результата разбора
		 * @param size  размер буфера числа в байтах
		 * @param text  разбираемая строка числа
		 * @return      результат выполнения разбора
		 *
		 * \~english
		 * @brief Method of parsing the string representation of a real number
		 * @param value buffer of the number to set the result of the parsing in
		 * @param size  size of the buffer of the number in bytes
		 * @param text  string of the number being parsed
		 * @return      result of performing the parsing
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool realParse(uint8_t * value, const size_t size, string_view text) noexcept;
		/**
		 * \~russian
		 * @brief Метод округления вещественного числа до указанного количества знаков после запятой
		 *
		 * @details Округление выполняется в десятичной системе счисления: строится точное
		 *          десятичное представление числа, отбрасываются младшие цифры по указанному
		 *          правилу, после чего результат переводится обратно в двоичный формат с
		 *          корректным округлением. Такой порядок исключает двойное округление,
		 *          возникающее при наивном умножении на степень десяти, поскольку степени
		 *          десяти в двоичном формате точно не представимы.
		 *
		 * @note    Результат округления является ближайшим представимым к десятичному
		 *          значением: само десятичное значение вида 0.33 в двоичном формате точно
		 *          не представимо, поэтому дробная часть остаётся бесконечной по устройству
		 *          формата. Точное количество знаков гарантируется при выводе числа методом
		 *          формирования строкового представления с указанной точностью.
		 *
		 * @param value  буфер числа для округления
		 * @param size   размер буфера числа в байтах
		 * @param digits количество знаков после запятой
		 * @param mode   правило округления числа
		 *
		 * \~english
		 * @brief Method of rounding a real number to the specified number of digits after the decimal point
		 * @details The rounding is performed in the decimal numeral system: an exact
		 *          decimal representation of the number is built, the low digits are discarded by the specified
		 *          rule, after which the result is converted back into the binary format with
		 *          correct rounding. Such an order rules out the double rounding
		 *          arising from a naive multiplication by a power of ten, since the powers of
		 *          ten are not exactly representable in the binary format.
		 * @note    The result of the rounding is the value nearest representable to the decimal
		 *          one: the decimal value 0.33 itself is not exactly representable in the binary format,
		 *          therefore the fractional part stays infinite by the arrangement
		 *          of the format. The exact number of digits is guaranteed when the number is output by the method
		 *          building the string representation with the specified precision.
		 * @param value  buffer of the number to round
		 * @param size   size of the buffer of the number in bytes
		 * @param digits number of digits after the decimal point
		 * @param mode   rounding rule of the number
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void realRound(uint8_t * value, const size_t size, const int32_t digits, const round_t mode) noexcept;
	};
	/**
	 * \~russian
	 * @brief Шаблон разрядности и типа длинного числа
	 *
	 * @tparam BYTES размер числа в байтах
	 * @tparam TYPE  тип хранимого числа
	 *
	 *
	 * \~english
	 * @brief Template of the bit width and of the type of the long number
	 * @tparam BYTES size of the number in bytes
	 * @tparam TYPE  type of the kept number
	 *
	 * \~
	 */
	template <uint16_t BYTES, bignum::type_t TYPE = bignum::type_t::SIGNED>
	/**
	 * \~russian
	 * @brief Класс длинного числа произвольной разрядности
	 *
	 * @details Класс хранит число в массиве байтов фиксированного размера в порядке
	 *          от младшего байта к старшему и предоставляет полный набор арифметических,
	 *          побитовых операций и операций сравнения. Число полностью совместимо со
	 *          встроенными числовыми типами в обе стороны: преобразование из встроенного
	 *          типа выполняется неявно, преобразование во встроенный тип — явно, что
	 *          исключает неоднозначность разрешения перегрузок арифметических операторов.
	 *
	 * @note    Целочисленная арифметика выполняется по модулю разрядной сетки: переполнение
	 *          отбрасывает старшие разряды, деление на нуль даёт нулевое частное и нулевой
	 *          остаток, остаток от деления принимает знак делимого.
	 *
	 * @note    Преобразование вещественного числа в целое отбрасывает дробную часть, а при
	 *          выходе значения за пределы разрядной сетки приёмника выполняет насыщение
	 *          предельным значением. Преобразование бесконечности выполняет насыщение,
	 *          преобразование значения не являющегося числом даёт нуль.
	 *
	 * @note    Правила сужения зависят от вида числа источника и намеренно различаются:
	 *          преобразование между целыми выполняется по модулю разрядной сетки приёмника,
	 *          то есть отбрасывает старшие разряды, как это делает язык, а преобразование из
	 *          вещественного числа выполняет насыщение. Поэтому запись отрицательного целого
	 *          в беззнаковое число даёт значение в дополнительном коде, а запись отрицательного
	 *          вещественного числа даёт нуль.
	 *
	 * @note    Побитовые операции над вещественным числом выполняются над его двоичным
	 *          представлением по стандарту IEEE-754, а не над значением числа, поэтому их
	 *          результат следует трактовать как работу с кодировкой: выражение [~num] для
	 *          единицы даёт около минус четырёх, а не значение не являющееся числом. Операции
	 *          сдвига являются исключением и выполняют масштабирование значения степенью двойки.
	 *
	 * @note    Реализация класса вынесена в исходный файл, поэтому доступны только те
	 *          разрядности, для которых объявлены прототипы в конце файла src/num/bignum.cpp.
	 *          Для добавления собственной разрядности достаточно дописать туда вызов макроса
	 *          AWH_BIGNUM_INSTANTIATE_INTEGER либо AWH_BIGNUM_INSTANTIATE_REAL с требуемым
	 *          размером числа в байтах. Использование необъявленной разрядности приводит к
	 *          ошибке компоновки с указанием отсутствующей специализации. Вычислительный
	 *          движок пространства имён bignum работает с любым размером числа без
	 *          объявления прототипов.
	 *
	 * @note    В операциях целочисленного длинного числа со встроенным вещественным числом
	 *          вещественный операнд предварительно преобразуется в длинное целое с
	 *          отбрасыванием дробной части, поэтому выражение вида [num < 5.5] сравнивает
	 *          число с пятёркой. Для сравнения с учётом дробной части следует использовать
	 *          вещественное длинное число либо явное приведение длинного числа к double.
	 *
	 * @note    Поведение модуля, которое со стороны выглядит ошибочным, но принято намеренно,
	 *          сведено в файл include/num/BIGNUM.md вместе с основаниями каждого решения:
	 *          различие правил сужения целых и вещественных чисел, побитовые операции над
	 *          кодировкой вещественного числа, тихий нуль при делении на нуль, кратчайшая
	 *          обратимая запись при выводе, разбор приставок nan и inf, отсутствие
	 *          идемпотентности направленного округления, запрет совпадения приёмников в
	 *          divmod, отличие формата real80_t от расширенной точности x87 и отказ от
	 *          алгоритма Карацубы. Перед правкой перечисленного следует прочитать основание.
	 *
	 * \~english
	 * @brief Class of a long number of an arbitrary bit width
	 * @details The class keeps the number in an array of bytes of a fixed size in the order
	 *          from the low byte to the high one and provides a full set of arithmetic,
	 *          bitwise and comparison operations. The number is fully compatible with
	 *          the built-in numeric types in both directions: the conversion from a built-in
	 *          type is performed implicitly, the conversion into a built-in type explicitly, which
	 *          rules out the ambiguity of the overload resolution of the arithmetic operators.
	 * @note    Integer arithmetic is performed modulo the bit grid: an overflow
	 *          discards the high limbs, a division by zero gives a zero quotient and a zero
	 *          remainder, the remainder of a division takes the sign of the dividend.
	 * @note    The conversion of a real number into an integer discards the fractional part, and when
	 *          the value goes beyond the bounds of the bit grid of the destination it saturates with the
	 *          limiting value. The conversion of infinity performs a saturation,
	 *          the conversion of a value that is not a number gives zero.
	 * @note    The narrowing rules depend on the kind of the source number and differ deliberately:
	 *          the conversion between integers is performed modulo the bit grid of the destination,
	 *          that is, it discards the high limbs, as the language does, whereas the conversion from
	 *          a real number performs a saturation. Therefore writing a negative integer
	 *          into an unsigned number gives a value in two's complement, and writing a negative
	 *          real number gives zero.
	 * @note    Bitwise operations over a real number are performed over its binary
	 *          representation by the IEEE-754 standard rather than over the value of the number, therefore their
	 *          result should be treated as work with the encoding: the expression [~num] for
	 *          one gives about minus four rather than a value that is not a number. The shift
	 *          operations are an exception and scale the value by a power of two.
	 * @note    The implementation of the class is moved into a source file, therefore only those
	 *          bit widths are available for which prototypes are declared at the end of the src/num/bignum.cpp file.
	 *          To add a bit width of your own it is enough to append there a call of the
	 *          AWH_BIGNUM_INSTANTIATE_INTEGER or AWH_BIGNUM_INSTANTIATE_REAL macro with the required
	 *          size of the number in bytes. Using an undeclared bit width leads to a
	 *          link error stating the missing specialisation. The computation
	 *          engine of the bignum namespace works with any size of a number without
	 *          declaring prototypes.
	 * @note    In the operations of an integer long number with a built-in real number
	 *          the real operand is beforehand converted into a long integer with the
	 *          fractional part discarded, therefore an expression of the [num < 5.5] form compares
	 *          the number with five. To compare taking the fractional part into account one should use
	 *          a real long number or an explicit cast of the long number to double.
	 * @note    The behaviour of the module that looks erroneous from the outside but is adopted deliberately
	 *          is collected in the include/num/BIGNUM.md file together with the grounds for every decision:
	 *          the difference of the narrowing rules of integer and real numbers, bitwise operations over
	 *          the encoding of a real number, the quiet zero on division by zero, the shortest
	 *          round-trip record on output, the parsing of the nan and inf prefixes, the absence of
	 *          idempotence of directed rounding, the prohibition of coinciding destinations in
	 *          divmod, the difference of the real80_t format from the x87 extended precision and the rejection of
	 *          the Karatsuba algorithm. Before changing what is listed one should read the ground.
	 *
	 * \~
	 */
	class __AWH_SHARED_EXPORT__ BigNum {
		/**
		 * Проверяем корректность разрядности длинного числа
		 */
		static_assert(BYTES >= 2, "AWH bignum: the number size must be at least 2 bytes");
		/**
		 * Проверяем корректность типа длинного числа
		 */
		static_assert(TYPE != bignum::type_t::NONE, "AWH bignum: the number type must be specified");
		public:
			/**
			 * \~russian
			 * @brief Создаём тип данных типа хранимого числа
			 *
			 * \~english
			 * @brief Create the data type of the type of the kept number
			 *
			 * \~
			 */
			using type_t = bignum::type_t;
			/**
			 * \~russian
			 * @brief Создаём тип данных класса значения числа
			 *
			 * \~english
			 * @brief Create the data type of the class of the value of the number
			 *
			 * \~
			 */
			using class_t = bignum::class_t;
			/**
			 * \~russian
			 * @brief Создаём тип данных формата представления числа
			 *
			 * \~english
			 * @brief Create the data type of the format of the representation of the number
			 *
			 * \~
			 */
			using format_t = bignum::format_t;
			/**
			 * \~russian
			 * @brief Создаём тип данных правила округления числа
			 *
			 * \~english
			 * @brief Create the data type of the rounding rule of the number
			 *
			 * \~
			 */
			using round_t = bignum::round_t;
		private:
			// Буфер хранения числа в порядке от младшего байта к старшему
			array <uint8_t, BYTES> _data;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения размера числа в байтах
			 *
			 * @return размер числа в байтах
			 *
			 * \~english
			 * @brief Method of getting the size of the number in bytes
			 * @return size of the number in bytes
			 *
			 * \~
			 */
			static constexpr uint16_t size() noexcept {
				// Выводим размер числа в байтах
				return BYTES;
			}
			/**
			 * \~russian
			 * @brief Метод извлечения разрядности числа в битах
			 *
			 * @return разрядность числа в битах
			 *
			 * \~english
			 * @brief Method of getting the bit width of the number in bits
			 * @return bit width of the number in bits
			 *
			 * \~
			 */
			static constexpr uint32_t bitness() noexcept {
				// Выводим разрядность числа в битах
				return (static_cast <uint32_t> (BYTES) * 8);
			}
			/**
			 * \~russian
			 * @brief Метод извлечения типа хранимого числа
			 *
			 * @return тип хранимого числа
			 *
			 * \~english
			 * @brief Method of getting the type of the kept number
			 * @return type of the kept number
			 *
			 * \~
			 */
			static constexpr type_t kind() noexcept {
				// Выводим тип хранимого числа
				return TYPE;
			}
		public:
			/**
			 * \~russian
			 * @brief Метод формирования минимально возможного значения числа
			 *
			 * @return минимально возможное значение числа
			 *
			 * \~english
			 * @brief Method of building the minimum possible value of the number
			 * @return minimum possible value of the number
			 *
			 * \~
			 */
			static BigNum minimum() noexcept;
			/**
			 * \~russian
			 * @brief Метод формирования максимально возможного значения числа
			 *
			 * @return максимально возможное значение числа
			 *
			 * \~english
			 * @brief Method of building the maximum possible value of the number
			 * @return maximum possible value of the number
			 *
			 * \~
			 */
			static BigNum maximum() noexcept;
			/**
			 * \~russian
			 * @brief Метод формирования машинного эпсилон числа
			 *
			 * @details Для целочисленных типов метод возвращает единицу.
			 *
			 * @return машинный эпсилон числа
			 *
			 * \~english
			 * @brief Method of building the machine epsilon of the number
			 * @details For the integer types the method returns one.
			 * @return machine epsilon of the number
			 *
			 * \~
			 */
			static BigNum epsilon() noexcept;
			/**
			 * \~russian
			 * @brief Метод формирования значения бесконечности
			 *
			 * @details Для целочисленных типов метод возвращает максимально возможное значение.
			 *
			 * @return значение бесконечности
			 *
			 * \~english
			 * @brief Method of building the value of infinity
			 * @details For the integer types the method returns the maximum possible value.
			 * @return value of infinity
			 *
			 * \~
			 */
			static BigNum unlimited() noexcept;
			/**
			 * \~russian
			 * @brief Метод формирования значения не являющегося числом
			 *
			 * @details Для целочисленных типов метод возвращает нулевое значение.
			 *
			 * @return значение не являющееся числом
			 *
			 * \~english
			 * @brief Method of building a value that is not a number
			 * @details For the integer types the method returns a zero value.
			 * @return value that is not a number
			 *
			 * \~
			 */
			static BigNum undefined() noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод деления двух чисел с получением частного и остатка
			 *
			 * @param num1      делимое число
			 * @param num2      делитель числа
			 * @param quotient  ссылка на частное от деления
			 * @param remainder ссылка на остаток от деления
			 *
			 * \~english
			 * @brief Method of dividing two numbers with getting the quotient and the remainder
			 * @param num1      dividend number
			 * @param num2      divisor of the number
			 * @param quotient  reference to the quotient of the division
			 * @param remainder reference to the remainder of the division
			 *
			 * \~
			 */
			static void divmod(const BigNum & num1, const BigNum & num2, BigNum & quotient, BigNum & remainder) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод очистки значения числа
			 *
			 * \~english
			 * @brief Method of clearing the value of the number
			 *
			 * \~
			 */
			void clear() noexcept;
			/**
			 * \~russian
			 * @brief Метод обмена значениями двух чисел
			 *
			 * @param num число для обмена значениями
			 *
			 * \~english
			 * @brief Method of swapping the values of two numbers
			 * @param num number to swap values with
			 *
			 * \~
			 */
			void swap(BigNum & num) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод проверки числа на нулевое значение
			 *
			 * @return результат проверки
			 *
			 * \~english
			 * @brief Method of checking the number for a zero value
			 * @return result of the check
			 *
			 * \~
			 */
			bool zero() const noexcept;
			/**
			 * \~russian
			 * @brief Метод проверки числа на отрицательное значение
			 *
			 * @return результат проверки
			 *
			 * \~english
			 * @brief Method of checking the number for a negative value
			 * @return result of the check
			 *
			 * \~
			 */
			bool negative() const noexcept;
			/**
			 * \~russian
			 * @brief Метод проверки числа на нечётное значение
			 *
			 * @note   Проверка вещественного числа выполняется над его целой частью, то есть
			 *         совпадает с проверкой результата приведения числа к целому: значение
			 *         3.5 является нечётным, а значение 2.5 чётным. Правило согласовано с
			 *         преобразованием вещественного числа в целое, которое отбрасывает
			 *         дробную часть. Бесконечность и значение не являющееся числом чётностью
			 *         не обладают, поэтому нечётными не считаются.
			 *
			 * @return результат проверки
			 *
			 * \~english
			 * @brief Method of checking the number for an odd value
			 * @note   The check of a real number is performed over its integer part, that is, it
			 *         coincides with the check of the result of casting the number to an integer: the value
			 *         3.5 is odd, and the value 2.5 even. The rule is agreed with
			 *         the conversion of a real number into an integer, which discards the
			 *         fractional part. Infinity and a value that is not a number possess no parity,
			 *         therefore they are not considered odd.
			 * @return result of the check
			 *
			 * \~
			 */
			bool odd() const noexcept;
			/**
			 * \~russian
			 * @brief Метод проверки числа на чётное значение
			 *
			 * @note   Проверка вещественного числа выполняется над его целой частью, поэтому
			 *         бесконечность и значение не являющееся числом считаются чётными как
			 *         отрицание проверки на нечётность. Подробности указаны у метода odd.
			 *
			 * @return результат проверки
			 *
			 * \~english
			 * @brief Method of checking the number for an even value
			 * @note   The check of a real number is performed over its integer part, therefore
			 *         infinity and a value that is not a number are considered even as
			 *         the negation of the check for oddness. The details are given at the odd method.
			 * @return result of the check
			 *
			 * \~
			 */
			bool even() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод определения класса значения числа
			 *
			 * @return класс значения числа
			 *
			 * \~english
			 * @brief Method of determining the class of the value of the number
			 * @return class of the value of the number
			 *
			 * \~
			 */
			class_t category() const noexcept;
			/**
			 * \~russian
			 * @brief Метод извлечения количества значащих бит числа
			 *
			 * @return количество значащих бит числа
			 *
			 * \~english
			 * @brief Method of getting the number of significant bits of the number
			 * @return number of significant bits of the number
			 *
			 * \~
			 */
			uint32_t bits() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения значения бита числа
			 *
			 * @param index индекс извлекаемого бита
			 * @return      значение указанного бита
			 *
			 * \~english
			 * @brief Method of getting the value of a bit of the number
			 * @param index index of the bit to get
			 * @return      value of the specified bit
			 *
			 * \~
			 */
			bool bit(const uint32_t index) const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки значения бита числа
			 *
			 * @param index индекс устанавливаемого бита
			 * @param mode  устанавливаемое значение бита
			 *
			 * \~english
			 * @brief Method of setting the value of a bit of the number
			 * @param index index of the bit to set
			 * @param mode  value of the bit being set
			 *
			 * \~
			 */
			void bit(const uint32_t index, const bool mode) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения буфера числа
			 *
			 * @return буфер хранения числа
			 *
			 * \~english
			 * @brief Method of getting the buffer of the number
			 * @return storage buffer of the number
			 *
			 * \~
			 */
			uint8_t * data() noexcept;
			/**
			 * \~russian
			 * @brief Метод извлечения буфера числа
			 *
			 * @return буфер хранения числа
			 *
			 * \~english
			 * @brief Method of getting the buffer of the number
			 * @return storage buffer of the number
			 *
			 * \~
			 */
			const uint8_t * data() const noexcept;
			/**
			 * \~russian
			 * @brief Метод извлечения массива байтов числа
			 *
			 * @return массив байтов хранения числа
			 *
			 * \~english
			 * @brief Method of getting the byte array of the number
			 * @return storage byte array of the number
			 *
			 * \~
			 */
			const array <uint8_t, BYTES> & bytes() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки значения числа из внешнего буфера
			 *
			 * @details Метод выполняет преобразование разрядности и типа числа.
			 *
			 * @param buffer буфер числа источника в порядке от младшего байта к старшему
			 * @param size   размер буфера числа источника в байтах
			 * @param type   тип числа источника
			 *
			 * \~english
			 * @brief Method of setting the value of the number from an external buffer
			 * @details The method performs the conversion of the bit width and of the type of the number.
			 * @param buffer buffer of the source number in the order from the low byte to the high one
			 * @param size   size of the buffer of the source number in bytes
			 * @param type   type of the source number
			 *
			 * \~
			 */
			void set(const uint8_t * buffer, const uint16_t size, const type_t type) noexcept;
			/**
			 * \~russian
			 * @brief Метод извлечения значения числа во внешний буфер
			 *
			 * @details Метод выполняет преобразование разрядности и типа числа.
			 *
			 * @param buffer буфер числа приёмника в порядке от младшего байта к старшему
			 * @param size   размер буфера числа приёмника в байтах
			 * @param type   тип числа приёмника
			 *
			 * \~english
			 * @brief Method of getting the value of the number into an external buffer
			 * @details The method performs the conversion of the bit width and of the type of the number.
			 * @param buffer buffer of the destination number in the order from the low byte to the high one
			 * @param size   size of the buffer of the destination number in bytes
			 * @param type   type of the destination number
			 *
			 * \~
			 */
			void get(uint8_t * buffer, const uint16_t size, const type_t type) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод сравнения двух чисел
			 *
			 * @param num число для сравнения
			 * @return    результат сравнения (-1, 0, 1 или 2 для несравнимых значений)
			 *
			 * \~english
			 * @brief Method of comparing two numbers
			 * @param num number to compare with
			 * @return    result of the comparison (-1, 0, 1 or 2 for incomparable values)
			 *
			 * \~
			 */
			int8_t compare(const BigNum & num) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения модуля числа
			 *
			 * @return модуль текущего числа
			 *
			 * \~english
			 * @brief Method of taking the modulus of the number
			 * @return modulus of the current number
			 *
			 * \~
			 */
			BigNum abs() const noexcept;
			/**
			 * \~russian
			 * @brief Метод извлечения квадратного корня числа
			 *
			 * @details Для целочисленных типов метод выполняет извлечение целочисленного корня,
			 *          при этом корень извлекается из модуля числа. Для вещественных типов метод
			 *          следует стандарту IEEE-754: корень из отрицательного значения не является
			 *          числом, а знак нуля сохраняется.
			 *
			 * @return квадратный корень текущего числа
			 *
			 * \~english
			 * @brief Method of taking the square root of the number
			 * @details For the integer types the method takes the integer root,
			 *          and the root is taken of the modulus of the number. For the real types the method
			 *          follows the IEEE-754 standard: the root of a negative value is not a
			 *          number, and the sign of zero is preserved.
			 * @return square root of the current number
			 *
			 * \~
			 */
			BigNum sqrt() const noexcept;
			/**
			 * \~russian
			 * @brief Метод возведения числа в степень
			 *
			 * @param exponent показатель степени
			 * @return         результат возведения в степень
			 *
			 * \~english
			 * @brief Method of raising the number to a power
			 * @param exponent exponent of the power
			 * @return         result of the raising to a power
			 *
			 * \~
			 */
			BigNum pow(const uint64_t exponent) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод округления числа до указанного количества знаков после запятой
			 *
			 * @details Округление выполняется в десятичной системе счисления, поэтому
			 *          двойного округления через степень десяти не возникает. Для целых
			 *          чисел неотрицательное количество знаков оставляет значение без
			 *          изменений, а отрицательное округляет до соответствующей степени
			 *          десяти: значение -3 округляет число до тысяч.
			 *
			 * @note    Дробная часть вещественного числа по устройству формата остаётся
			 *          бесконечной: результат округления является ближайшим представимым
			 *          к десятичному значением. Точное количество знаков после запятой
			 *          гарантируется при выводе методом print с указанной точностью.
			 *
			 * @note    Повторное округление вещественного числа возвращает тот же результат
			 *          только для правил NEAREST и EVEN. Направленные правила DOWN, UP и ZERO
			 *          идемпотентностью не обладают, поскольку ближайшее представимое значение
			 *          лежит либо чуть ниже, либо чуть выше требуемой десятичной записи и
			 *          каждое следующее применение правила сдвигает результат ещё на один шаг.
			 *          Для целых чисел округление идемпотентно при любом правиле.
			 *
			 * @param digits количество знаков после запятой
			 * @param mode   правило округления числа
			 * @return       округлённое значение числа
			 *
			 * \~english
			 * @brief Method of rounding the number to the specified number of digits after the decimal point
			 * @details The rounding is performed in the decimal numeral system, therefore
			 *          no double rounding through a power of ten arises. For integer
			 *          numbers a non-negative number of digits leaves the value unchanged,
			 *          and a negative one rounds to the corresponding power of
			 *          ten: the value -3 rounds the number to thousands.
			 * @note    The fractional part of a real number stays infinite by the arrangement of the format:
			 *          the result of the rounding is the value nearest representable
			 *          to the decimal one. The exact number of digits after the decimal point
			 *          is guaranteed on output by the print method with the specified precision.
			 * @note    A repeated rounding of a real number returns the same result
			 *          only for the NEAREST and EVEN rules. The directed DOWN, UP and ZERO rules
			 *          possess no idempotence, since the nearest representable value
			 *          lies either slightly below or slightly above the required decimal record and
			 *          every next application of the rule shifts the result by one more step.
			 *          For integer numbers the rounding is idempotent under any rule.
			 * @param digits number of digits after the decimal point
			 * @param mode   rounding rule of the number
			 * @return       rounded value of the number
			 *
			 * \~
			 */
			BigNum round(const int32_t digits = 0, const round_t mode = round_t::NEAREST) const noexcept;
			/**
			 * \~russian
			 * @brief Метод отбрасывания знаков числа после указанной позиции
			 *
			 * @param digits количество сохраняемых знаков после запятой
			 * @return        значение числа с отброшенными младшими знаками
			 *
			 * \~english
			 * @brief Method of discarding the digits of the number after the specified position
			 * @param digits number of kept digits after the decimal point
			 * @return        value of the number with the low digits discarded
			 *
			 * \~
			 */
			BigNum trunc(const int32_t digits = 0) const noexcept;
			/**
			 * \~russian
			 * @brief Метод округления числа в сторону минус бесконечности
			 *
			 * @param digits количество сохраняемых знаков после запятой
			 * @return        округлённое значение числа
			 *
			 * \~english
			 * @brief Method of rounding the number towards minus infinity
			 * @param digits number of kept digits after the decimal point
			 * @return        rounded value of the number
			 *
			 * \~
			 */
			BigNum floor(const int32_t digits = 0) const noexcept;
			/**
			 * \~russian
			 * @brief Метод округления числа в сторону плюс бесконечности
			 *
			 * @param digits количество сохраняемых знаков после запятой
			 * @return        округлённое значение числа
			 *
			 * \~english
			 * @brief Method of rounding the number towards plus infinity
			 * @param digits number of kept digits after the decimal point
			 * @return        rounded value of the number
			 *
			 * \~
			 */
			BigNum ceil(const int32_t digits = 0) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод формирования строкового представления числа
			 *
			 * @note   Знак минус выводится только в десятичном формате. Двоичный,
			 *         восьмеричный и шестнадцатеричный форматы выводят двоичное
			 *         представление числа, то есть отрицательное целое печатается
			 *         в дополнительном коде, а не как модуль со знаком. Разбор такой
			 *         записи возвращает исходное значение, поэтому обратимость
			 *         сохраняется во всех форматах.
			 *
			 * @param format    формат представления числа
			 * @param precision количество знаков после запятой (отрицательное значение - автоматически)
			 * @return          сформированная строка числа
			 *
			 * \~english
			 * @brief Method of building the string representation of the number
			 * @note   The minus sign is output only in the decimal format. The binary,
			 *         octal and hexadecimal formats output the binary
			 *         representation of the number, that is, a negative integer is printed
			 *         in two's complement rather than as a magnitude with a sign. Parsing such
			 *         a record returns the source value, therefore the round trip
			 *         is preserved in all the formats.
			 * @param format    format of the representation of the number
			 * @param precision number of digits after the decimal point (a negative value means automatic)
			 * @return          the built string of the number
			 *
			 * \~
			 */
			string print(const format_t format = format_t::DEC, const int16_t precision = -1) const noexcept;
			/**
			 * \~russian
			 * @brief Метод разбора строкового представления числа
			 *
			 * @note   Формат по умолчанию определяется по приставке записи: 0x, 0b и 0o
			 *         задают шестнадцатеричную, двоичную и восьмеричную системы счисления,
			 *         запись без приставки считается десятичной. Метод print приставку не
			 *         выводит, поэтому запись в системе счисления отличной от десятичной
			 *         следует разбирать с явным указанием формата, иначе она будет прочтена
			 *         как десятичная. Приставка, соответствующая явно указанному формату,
			 *         принимается и пропускается.
			 *
			 * @param text   разбираемая строка числа
			 * @param format формат представления числа
			 * @return       результат выполнения разбора
			 *
			 * \~english
			 * @brief Method of parsing the string representation of the number
			 * @note   The default format is determined by the prefix of the record: 0x, 0b and 0o
			 *         set the hexadecimal, the binary and the octal numeral systems,
			 *         a record without a prefix is considered decimal. The print method outputs no
			 *         prefix, therefore a record in a numeral system other than the decimal one
			 *         should be parsed with an explicit statement of the format, otherwise it will be read
			 *         as a decimal one. A prefix corresponding to the explicitly stated format
			 *         is accepted and skipped.
			 * @param text   string of the number being parsed
			 * @param format format of the representation of the number
			 * @return       result of performing the parsing
			 *
			 * \~
			 */
			bool parse(string_view text, const format_t format = format_t::NONE) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор вывода числа в качестве строки
			 *
			 * @return число в качестве строки
			 *
			 * \~english
			 * @brief Operator of outputting the number as a string
			 * @return the number as a string
			 *
			 * \~
			 */
			operator string() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор приведения к логическому типу
			 *
			 * @return результат приведения
			 *
			 * \~english
			 * @brief Cast operator to the boolean type
			 * @return result of the cast
			 *
			 * \~
			 */
			explicit operator bool() const noexcept;
			/**
			 * \~russian
			 * @brief Оператор приведения к символьному типу
			 *
			 * @return результат приведения
			 *
			 * \~english
			 * @brief Cast operator to the character type
			 * @return result of the cast
			 *
			 * \~
			 */
			explicit operator char() const noexcept;
			/**
			 * \~russian
			 * @brief Оператор приведения к знаковому символьному типу
			 *
			 * @return результат приведения
			 *
			 * \~english
			 * @brief Cast operator to the signed character type
			 * @return result of the cast
			 *
			 * \~
			 */
			explicit operator signed char() const noexcept;
			/**
			 * \~russian
			 * @brief Оператор приведения к беззнаковому символьному типу
			 *
			 * @return результат приведения
			 *
			 * \~english
			 * @brief Cast operator to the unsigned character type
			 * @return result of the cast
			 *
			 * \~
			 */
			explicit operator unsigned char() const noexcept;
			/**
			 * \~russian
			 * @brief Оператор приведения к короткому целому типу
			 *
			 * @return результат приведения
			 *
			 * \~english
			 * @brief Cast operator to the short integer type
			 * @return result of the cast
			 *
			 * \~
			 */
			explicit operator short() const noexcept;
			/**
			 * \~russian
			 * @brief Оператор приведения к короткому беззнаковому целому типу
			 *
			 * @return результат приведения
			 *
			 * \~english
			 * @brief Cast operator to the short unsigned integer type
			 * @return result of the cast
			 *
			 * \~
			 */
			explicit operator unsigned short() const noexcept;
			/**
			 * \~russian
			 * @brief Оператор приведения к целому типу
			 *
			 * @return результат приведения
			 *
			 * \~english
			 * @brief Cast operator to the integer type
			 * @return result of the cast
			 *
			 * \~
			 */
			explicit operator int() const noexcept;
			/**
			 * \~russian
			 * @brief Оператор приведения к беззнаковому целому типу
			 *
			 * @return результат приведения
			 *
			 * \~english
			 * @brief Cast operator to the unsigned integer type
			 * @return result of the cast
			 *
			 * \~
			 */
			explicit operator unsigned int() const noexcept;
			/**
			 * \~russian
			 * @brief Оператор приведения к длинному целому типу
			 *
			 * @return результат приведения
			 *
			 * \~english
			 * @brief Cast operator to the long integer type
			 * @return result of the cast
			 *
			 * \~
			 */
			explicit operator long() const noexcept;
			/**
			 * \~russian
			 * @brief Оператор приведения к длинному беззнаковому целому типу
			 *
			 * @return результат приведения
			 *
			 * \~english
			 * @brief Cast operator to the long unsigned integer type
			 * @return result of the cast
			 *
			 * \~
			 */
			explicit operator unsigned long() const noexcept;
			/**
			 * \~russian
			 * @brief Оператор приведения к сверхдлинному целому типу
			 *
			 * @return результат приведения
			 *
			 * \~english
			 * @brief Cast operator to the long long integer type
			 * @return result of the cast
			 *
			 * \~
			 */
			explicit operator long long() const noexcept;
			/**
			 * \~russian
			 * @brief Оператор приведения к сверхдлинному беззнаковому целому типу
			 *
			 * @return результат приведения
			 *
			 * \~english
			 * @brief Cast operator to the long long unsigned integer type
			 * @return result of the cast
			 *
			 * \~
			 */
			explicit operator unsigned long long() const noexcept;
			/**
			 * \~russian
			 * @brief Оператор приведения к вещественному типу одинарной точности
			 *
			 * @return результат приведения
			 *
			 * \~english
			 * @brief Cast operator to the single-precision real type
			 * @return result of the cast
			 *
			 * \~
			 */
			explicit operator float() const noexcept;
			/**
			 * \~russian
			 * @brief Оператор приведения к вещественному типу двойной точности
			 *
			 * @return результат приведения
			 *
			 * \~english
			 * @brief Cast operator to the double-precision real type
			 * @return result of the cast
			 *
			 * \~
			 */
			explicit operator double() const noexcept;
			/**
			 * \~russian
			 * @brief Оператор приведения к вещественному типу расширенной точности
			 *
			 * @return результат приведения
			 *
			 * \~english
			 * @brief Cast operator to the extended-precision real type
			 * @return result of the cast
			 *
			 * \~
			 */
			explicit operator long double() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор извлечения байта числа
			 *
			 * @param index индекс извлекаемого байта
			 * @return      значение указанного байта
			 *
			 * \~english
			 * @brief Operator of getting a byte of the number
			 * @param index index of the byte to get
			 * @return      value of the specified byte
			 *
			 * \~
			 */
			uint8_t operator [] (const uint16_t index) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор проверки числа на нулевое значение
			 *
			 * @return результат проверки
			 *
			 * \~english
			 * @brief Operator of checking the number for a zero value
			 * @return result of the check
			 *
			 * \~
			 */
			bool operator ! () const noexcept;
			/**
			 * \~russian
			 * @brief Оператор унарного плюса
			 *
			 * @return копия текущего числа
			 *
			 * \~english
			 * @brief Unary plus operator
			 * @return copy of the current number
			 *
			 * \~
			 */
			BigNum operator + () const noexcept;
			/**
			 * \~russian
			 * @brief Оператор унарного минуса
			 *
			 * @return число с противоположным знаком
			 *
			 * \~english
			 * @brief Unary minus operator
			 * @return number with the opposite sign
			 *
			 * \~
			 */
			BigNum operator - () const noexcept;
			/**
			 * \~russian
			 * @brief Оператор побитовой инверсии числа
			 *
			 * @return побитово инвертированное число
			 *
			 * \~english
			 * @brief Bitwise inversion operator of the number
			 * @return bitwise inverted number
			 *
			 * \~
			 */
			BigNum operator ~ () const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор префиксного инкремента
			 *
			 * @return текущий объект
			 *
			 * \~english
			 * @brief Prefix increment operator
			 * @return the current object
			 *
			 * \~
			 */
			BigNum & operator ++ () noexcept;
			/**
			 * \~russian
			 * @brief Оператор постфиксного инкремента
			 *
			 * @return значение числа до инкремента
			 *
			 * \~english
			 * @brief Postfix increment operator
			 * @return value of the number before the increment
			 *
			 * \~
			 */
			BigNum operator ++ (int) noexcept;
			/**
			 * \~russian
			 * @brief Оператор префиксного декремента
			 *
			 * @return текущий объект
			 *
			 * \~english
			 * @brief Prefix decrement operator
			 * @return the current object
			 *
			 * \~
			 */
			BigNum & operator -- () noexcept;
			/**
			 * \~russian
			 * @brief Оператор постфиксного декремента
			 *
			 * @return значение числа до декремента
			 *
			 * \~english
			 * @brief Postfix decrement operator
			 * @return value of the number before the decrement
			 *
			 * \~
			 */
			BigNum operator -- (int) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор сложения с присвоением
			 *
			 * @param num число для сложения
			 * @return    текущий объект
			 *
			 * \~english
			 * @brief Addition with assignment operator
			 * @param num number to add
			 * @return    the current object
			 *
			 * \~
			 */
			BigNum & operator += (const BigNum & num) noexcept;
			/**
			 * \~russian
			 * @brief Оператор вычитания с присвоением
			 *
			 * @param num число для вычитания
			 * @return    текущий объект
			 *
			 * \~english
			 * @brief Subtraction with assignment operator
			 * @param num number to subtract
			 * @return    the current object
			 *
			 * \~
			 */
			BigNum & operator -= (const BigNum & num) noexcept;
			/**
			 * \~russian
			 * @brief Оператор умножения с присвоением
			 *
			 * @param num число для умножения
			 * @return    текущий объект
			 *
			 * \~english
			 * @brief Multiplication with assignment operator
			 * @param num number to multiply by
			 * @return    the current object
			 *
			 * \~
			 */
			BigNum & operator *= (const BigNum & num) noexcept;
			/**
			 * \~russian
			 * @brief Оператор деления с присвоением
			 *
			 * @param num число для деления
			 * @return    текущий объект
			 *
			 * \~english
			 * @brief Division with assignment operator
			 * @param num number to divide by
			 * @return    the current object
			 *
			 * \~
			 */
			BigNum & operator /= (const BigNum & num) noexcept;
			/**
			 * \~russian
			 * @brief Оператор извлечения остатка от деления с присвоением
			 *
			 * @param num число для деления
			 * @return    текущий объект
			 *
			 * \~english
			 * @brief Remainder of division with assignment operator
			 * @param num number to divide by
			 * @return    the current object
			 *
			 * \~
			 */
			BigNum & operator %= (const BigNum & num) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор побитового умножения с присвоением
			 *
			 * @param num число для побитового умножения
			 * @return    текущий объект
			 *
			 * \~english
			 * @brief Bitwise multiplication with assignment operator
			 * @param num number for the bitwise multiplication
			 * @return    the current object
			 *
			 * \~
			 */
			BigNum & operator &= (const BigNum & num) noexcept;
			/**
			 * \~russian
			 * @brief Оператор побитового сложения с присвоением
			 *
			 * @param num число для побитового сложения
			 * @return    текущий объект
			 *
			 * \~english
			 * @brief Bitwise addition with assignment operator
			 * @param num number for the bitwise addition
			 * @return    the current object
			 *
			 * \~
			 */
			BigNum & operator |= (const BigNum & num) noexcept;
			/**
			 * \~russian
			 * @brief Оператор побитового исключающего сложения с присвоением
			 *
			 * @param num число для побитового исключающего сложения
			 * @return    текущий объект
			 *
			 * \~english
			 * @brief Bitwise exclusive addition with assignment operator
			 * @param num number for the bitwise exclusive addition
			 * @return    the current object
			 *
			 * \~
			 */
			BigNum & operator ^= (const BigNum & num) noexcept;
			/**
			 * \~russian
			 * @brief Оператор сдвига влево с присвоением
			 *
			 * @param count количество бит сдвига
			 * @return      текущий объект
			 *
			 * \~english
			 * @brief Left shift with assignment operator
			 * @param count number of bits of the shift
			 * @return      the current object
			 *
			 * \~
			 */
			BigNum & operator <<= (const uint32_t count) noexcept;
			/**
			 * \~russian
			 * @brief Оператор сдвига вправо с присвоением
			 *
			 * @param count количество бит сдвига
			 * @return      текущий объект
			 *
			 * \~english
			 * @brief Right shift with assignment operator
			 * @param count number of bits of the shift
			 * @return      the current object
			 *
			 * \~
			 */
			BigNum & operator >>= (const uint32_t count) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор присваивания копированием
			 *
			 * @param num число для присвоения
			 * @return    текущий объект
			 *
			 * \~english
			 * @brief Copy assignment operator
			 * @param num number to assign
			 * @return    the current object
			 *
			 * \~
			 */
			BigNum & operator = (const BigNum & num) noexcept;
			/**
			 * \~russian
			 * @brief Оператор присваивания перемещением
			 *
			 * @param num число для присвоения
			 * @return    текущий объект
			 *
			 * \~english
			 * @brief Move assignment operator
			 * @param num number to assign
			 * @return    the current object
			 *
			 * \~
			 */
			BigNum & operator = (BigNum && num) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор присваивания строкового значения
			 *
			 * @param text строка числа для присвоения
			 * @return     текущий объект
			 *
			 *
			 * \~english
			 * @brief Assignment operator of a string value
			 * @param text string of the number to assign
			 * @return     the current object
			 *
			 * \~
			 */
			BigNum & operator = (const char * text) noexcept;
			/**
			 * \~russian
			 * @brief Оператор присваивания строкового значения
			 *
			 * @param text строка числа для присвоения
			 * @return     текущий объект
			 *
			 *
			 * \~english
			 * @brief Assignment operator of a string value
			 * @param text string of the number to assign
			 * @return     the current object
			 *
			 * \~
			 */
			BigNum & operator = (const string & text) noexcept;
			/**
			 * \~russian
			 * @brief Оператор присваивания строкового значения
			 *
			 * @param text строка числа для присвоения
			 * @return     текущий объект
			 *
			 *
			 * \~english
			 * @brief Assignment operator of a string value
			 * @param text string of the number to assign
			 * @return     the current object
			 *
			 * \~
			 */
			BigNum & operator = (string_view text) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор присваивания логического значения
			 *
			 * @param num значение для присвоения
			 * @return    текущий объект
			 *
			 * \~english
			 * @brief Assignment operator of a boolean value
			 * @param num value to assign
			 * @return    the current object
			 *
			 * \~
			 */
			BigNum & operator = (const bool num) noexcept;
			/**
			 * \~russian
			 * @brief Оператор присваивания символьного значения
			 *
			 * @param num значение для присвоения
			 * @return    текущий объект
			 *
			 * \~english
			 * @brief Assignment operator of a character value
			 * @param num value to assign
			 * @return    the current object
			 *
			 * \~
			 */
			BigNum & operator = (const char num) noexcept;
			/**
			 * \~russian
			 * @brief Оператор присваивания знакового символьного значения
			 *
			 * @param num значение для присвоения
			 * @return    текущий объект
			 *
			 * \~english
			 * @brief Assignment operator of a signed character value
			 * @param num value to assign
			 * @return    the current object
			 *
			 * \~
			 */
			BigNum & operator = (const signed char num) noexcept;
			/**
			 * \~russian
			 * @brief Оператор присваивания беззнакового символьного значения
			 *
			 * @param num значение для присвоения
			 * @return    текущий объект
			 *
			 * \~english
			 * @brief Assignment operator of an unsigned character value
			 * @param num value to assign
			 * @return    the current object
			 *
			 * \~
			 */
			BigNum & operator = (const unsigned char num) noexcept;
			/**
			 * \~russian
			 * @brief Оператор присваивания короткого целого значения
			 *
			 * @param num значение для присвоения
			 * @return    текущий объект
			 *
			 * \~english
			 * @brief Assignment operator of a short integer value
			 * @param num value to assign
			 * @return    the current object
			 *
			 * \~
			 */
			BigNum & operator = (const short num) noexcept;
			/**
			 * \~russian
			 * @brief Оператор присваивания короткого беззнакового целого значения
			 *
			 * @param num значение для присвоения
			 * @return    текущий объект
			 *
			 * \~english
			 * @brief Assignment operator of a short unsigned integer value
			 * @param num value to assign
			 * @return    the current object
			 *
			 * \~
			 */
			BigNum & operator = (const unsigned short num) noexcept;
			/**
			 * \~russian
			 * @brief Оператор присваивания целого значения
			 *
			 * @param num значение для присвоения
			 * @return    текущий объект
			 *
			 * \~english
			 * @brief Assignment operator of an integer value
			 * @param num value to assign
			 * @return    the current object
			 *
			 * \~
			 */
			BigNum & operator = (const int num) noexcept;
			/**
			 * \~russian
			 * @brief Оператор присваивания беззнакового целого значения
			 *
			 * @param num значение для присвоения
			 * @return    текущий объект
			 *
			 * \~english
			 * @brief Assignment operator of an unsigned integer value
			 * @param num value to assign
			 * @return    the current object
			 *
			 * \~
			 */
			BigNum & operator = (const unsigned int num) noexcept;
			/**
			 * \~russian
			 * @brief Оператор присваивания длинного целого значения
			 *
			 * @param num значение для присвоения
			 * @return    текущий объект
			 *
			 * \~english
			 * @brief Assignment operator of a long integer value
			 * @param num value to assign
			 * @return    the current object
			 *
			 * \~
			 */
			BigNum & operator = (const long num) noexcept;
			/**
			 * \~russian
			 * @brief Оператор присваивания длинного беззнакового целого значения
			 *
			 * @param num значение для присвоения
			 * @return    текущий объект
			 *
			 * \~english
			 * @brief Assignment operator of a long unsigned integer value
			 * @param num value to assign
			 * @return    the current object
			 *
			 * \~
			 */
			BigNum & operator = (const unsigned long num) noexcept;
			/**
			 * \~russian
			 * @brief Оператор присваивания сверхдлинного целого значения
			 *
			 * @param num значение для присвоения
			 * @return    текущий объект
			 *
			 * \~english
			 * @brief Assignment operator of a long long integer value
			 * @param num value to assign
			 * @return    the current object
			 *
			 * \~
			 */
			BigNum & operator = (const long long num) noexcept;
			/**
			 * \~russian
			 * @brief Оператор присваивания сверхдлинного беззнакового целого значения
			 *
			 * @param num значение для присвоения
			 * @return    текущий объект
			 *
			 * \~english
			 * @brief Assignment operator of a long long unsigned integer value
			 * @param num value to assign
			 * @return    the current object
			 *
			 * \~
			 */
			BigNum & operator = (const unsigned long long num) noexcept;
			/**
			 * \~russian
			 * @brief Оператор присваивания вещественного значения одинарной точности
			 *
			 * @param num значение для присвоения
			 * @return    текущий объект
			 *
			 * \~english
			 * @brief Assignment operator of a single-precision real value
			 * @param num value to assign
			 * @return    the current object
			 *
			 * \~
			 */
			BigNum & operator = (const float num) noexcept;
			/**
			 * \~russian
			 * @brief Оператор присваивания вещественного значения двойной точности
			 *
			 * @param num значение для присвоения
			 * @return    текущий объект
			 *
			 * \~english
			 * @brief Assignment operator of a double-precision real value
			 * @param num value to assign
			 * @return    the current object
			 *
			 * \~
			 */
			BigNum & operator = (const double num) noexcept;
			/**
			 * \~russian
			 * @brief Оператор присваивания вещественного значения расширенной точности
			 *
			 * @param num значение для присвоения
			 * @return    текущий объект
			 *
			 * \~english
			 * @brief Assignment operator of an extended-precision real value
			 * @param num value to assign
			 * @return    the current object
			 *
			 * \~
			 */
			BigNum & operator = (const long double num) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Шаблон разрядности и типа присваиваемого длинного числа
			 *
			 * @tparam SIZE размер присваиваемого числа в байтах
			 * @tparam KIND тип присваиваемого числа
			 *
			 * \~english
			 * @brief Template of the bit width and of the type of the assigned long number
			 * @tparam SIZE size of the assigned number in bytes
			 * @tparam KIND type of the assigned number
			 *
			 * \~
			 */
			template <uint16_t SIZE, bignum::type_t KIND>
			/**
			 * \~russian
			 * @brief Оператор присваивания длинного числа иной разрядности
			 *
			 * @param num число для присвоения
			 * @return    текущий объект
			 *
			 * \~english
			 * @brief Assignment operator of a long number of another bit width
			 * @param num number to assign
			 * @return    the current object
			 *
			 * \~
			 */
			BigNum & operator = (const BigNum <SIZE, KIND> & num) noexcept {
				// Выполняем установку значения числа иной разрядности
				this->set(num.data(), SIZE, KIND);
				// Выводим текущий объект
				return (* this);
			}
		public:
			/**
			 * \~russian
			 * @brief Оператор [+] сложения двух чисел
			 *
			 * @param num1 первое слагаемое
			 * @param num2 второе слагаемое
			 * @return     результат сложения
			 *
			 * \~english
			 * @brief Operator [+] of the addition of two numbers
			 * @param num1 first addend
			 * @param num2 second addend
			 * @return     result of the addition
			 *
			 * \~
			 */
			friend BigNum operator + (BigNum num1, const BigNum & num2) noexcept {
				// Выполняем сложение двух чисел
				return (num1 += num2);
			}
			/**
			 * \~russian
			 * @brief Оператор [-] вычитания двух чисел
			 *
			 * @param num1 уменьшаемое число
			 * @param num2 вычитаемое число
			 * @return     результат вычитания
			 *
			 * \~english
			 * @brief Operator [-] of the subtraction of two numbers
			 * @param num1 minuend number
			 * @param num2 subtrahend number
			 * @return     result of the subtraction
			 *
			 * \~
			 */
			friend BigNum operator - (BigNum num1, const BigNum & num2) noexcept {
				// Выполняем вычитание двух чисел
				return (num1 -= num2);
			}
			/**
			 * \~russian
			 * @brief Оператор [*] умножения двух чисел
			 *
			 * @param num1 множимое число
			 * @param num2 множитель числа
			 * @return     результат умножения
			 *
			 * \~english
			 * @brief Operator [*] of the multiplication of two numbers
			 * @param num1 multiplicand number
			 * @param num2 multiplier of the number
			 * @return     result of the multiplication
			 *
			 * \~
			 */
			friend BigNum operator * (BigNum num1, const BigNum & num2) noexcept {
				// Выполняем умножение двух чисел
				return (num1 *= num2);
			}
			/**
			 * \~russian
			 * @brief Оператор [/] деления двух чисел
			 *
			 * @param num1 делимое число
			 * @param num2 делитель числа
			 * @return     результат деления
			 *
			 * \~english
			 * @brief Operator [/] of the division of two numbers
			 * @param num1 dividend number
			 * @param num2 divisor of the number
			 * @return     result of the division
			 *
			 * \~
			 */
			friend BigNum operator / (BigNum num1, const BigNum & num2) noexcept {
				// Выполняем деление двух чисел
				return (num1 /= num2);
			}
			/**
			 * \~russian
			 * @brief Оператор [%] извлечения остатка от деления двух чисел
			 *
			 * @param num1 делимое число
			 * @param num2 делитель числа
			 * @return     остаток от деления
			 *
			 * \~english
			 * @brief Operator [%] of getting the remainder of the division of two numbers
			 * @param num1 dividend number
			 * @param num2 divisor of the number
			 * @return     remainder of the division
			 *
			 * \~
			 */
			friend BigNum operator % (BigNum num1, const BigNum & num2) noexcept {
				// Выполняем извлечение остатка от деления двух чисел
				return (num1 %= num2);
			}
		public:
			/**
			 * \~russian
			 * @brief Оператор [&] побитового умножения двух чисел
			 *
			 * @param num1 первый операнд
			 * @param num2 второй операнд
			 * @return     результат побитового умножения
			 *
			 * \~english
			 * @brief Operator [&] of the bitwise multiplication of two numbers
			 * @param num1 first operand
			 * @param num2 second operand
			 * @return     result of the bitwise multiplication
			 *
			 * \~
			 */
			friend BigNum operator & (BigNum num1, const BigNum & num2) noexcept {
				// Выполняем побитовое умножение двух чисел
				return (num1 &= num2);
			}
			/**
			 * \~russian
			 * @brief Оператор [|] побитового сложения двух чисел
			 *
			 * @param num1 первый операнд
			 * @param num2 второй операнд
			 * @return     результат побитового сложения
			 *
			 * \~english
			 * @brief Operator [|] of the bitwise addition of two numbers
			 * @param num1 first operand
			 * @param num2 second operand
			 * @return     result of the bitwise addition
			 *
			 * \~
			 */
			friend BigNum operator | (BigNum num1, const BigNum & num2) noexcept {
				// Выполняем побитовое сложение двух чисел
				return (num1 |= num2);
			}
			/**
			 * \~russian
			 * @brief Оператор [^] побитового исключающего сложения двух чисел
			 *
			 * @param num1 первый операнд
			 * @param num2 второй операнд
			 * @return     результат побитового исключающего сложения
			 *
			 * \~english
			 * @brief Operator [^] of the bitwise exclusive addition of two numbers
			 * @param num1 first operand
			 * @param num2 second operand
			 * @return     result of the bitwise exclusive addition
			 *
			 * \~
			 */
			friend BigNum operator ^ (BigNum num1, const BigNum & num2) noexcept {
				// Выполняем побитовое исключающее сложение двух чисел
				return (num1 ^= num2);
			}
			/**
			 * \~russian
			 * @brief Оператор [<<] сдвига числа влево
			 *
			 * @param num   сдвигаемое число
			 * @param count количество бит сдвига
			 * @return      результат сдвига
			 *
			 * \~english
			 * @brief Operator [<<] of shifting a number left
			 * @param num   number being shifted
			 * @param count number of bits of the shift
			 * @return      result of the shift
			 *
			 * \~
			 */
			friend BigNum operator << (BigNum num, const uint32_t count) noexcept {
				// Выполняем сдвиг числа влево
				return (num <<= count);
			}
			/**
			 * \~russian
			 * @brief Оператор [>>] сдвига числа вправо
			 *
			 * @param num   сдвигаемое число
			 * @param count количество бит сдвига
			 * @return      результат сдвига
			 *
			 * \~english
			 * @brief Operator [>>] of shifting a number right
			 * @param num   number being shifted
			 * @param count number of bits of the shift
			 * @return      result of the shift
			 *
			 * \~
			 */
			friend BigNum operator >> (BigNum num, const uint32_t count) noexcept {
				// Выполняем сдвиг числа вправо
				return (num >>= count);
			}
		public:
			/**
			 * \~russian
			 * @brief Оператор [==] сравнения двух чисел
			 *
			 * @param num1 первое число для сравнения
			 * @param num2 второе число для сравнения
			 * @return     результат сравнения
			 *
			 * \~english
			 * @brief Operator [==] of the comparison of two numbers
			 * @param num1 first number to compare
			 * @param num2 second number to compare
			 * @return     result of the comparison
			 *
			 * \~
			 */
			friend bool operator == (const BigNum & num1, const BigNum & num2) noexcept {
				// Выводим результат сравнения двух чисел
				return (num1.compare(num2) == 0);
			}
			/**
			 * \~russian
			 * @brief Оператор [!=] сравнения двух чисел
			 *
			 * @param num1 первое число для сравнения
			 * @param num2 второе число для сравнения
			 * @return     результат сравнения
			 *
			 * \~english
			 * @brief Operator [!=] of the comparison of two numbers
			 * @param num1 first number to compare
			 * @param num2 second number to compare
			 * @return     result of the comparison
			 *
			 * \~
			 */
			friend bool operator != (const BigNum & num1, const BigNum & num2) noexcept {
				// Выводим результат сравнения двух чисел
				return (num1.compare(num2) != 0);
			}
			/**
			 * \~russian
			 * @brief Оператор [<] сравнения двух чисел
			 *
			 * @param num1 первое число для сравнения
			 * @param num2 второе число для сравнения
			 * @return     результат сравнения
			 *
			 * \~english
			 * @brief Operator [<] of the comparison of two numbers
			 * @param num1 first number to compare
			 * @param num2 second number to compare
			 * @return     result of the comparison
			 *
			 * \~
			 */
			friend bool operator < (const BigNum & num1, const BigNum & num2) noexcept {
				// Выводим результат сравнения двух чисел
				return (num1.compare(num2) == -1);
			}
			/**
			 * \~russian
			 * @brief Оператор [>] сравнения двух чисел
			 *
			 * @param num1 первое число для сравнения
			 * @param num2 второе число для сравнения
			 * @return     результат сравнения
			 *
			 * \~english
			 * @brief Operator [>] of the comparison of two numbers
			 * @param num1 first number to compare
			 * @param num2 second number to compare
			 * @return     result of the comparison
			 *
			 * \~
			 */
			friend bool operator > (const BigNum & num1, const BigNum & num2) noexcept {
				// Выводим результат сравнения двух чисел
				return (num1.compare(num2) == 1);
			}
			/**
			 * \~russian
			 * @brief Оператор [<=] сравнения двух чисел
			 *
			 * @param num1 первое число для сравнения
			 * @param num2 второе число для сравнения
			 * @return     результат сравнения
			 *
			 * \~english
			 * @brief Operator [<=] of the comparison of two numbers
			 * @param num1 first number to compare
			 * @param num2 second number to compare
			 * @return     result of the comparison
			 *
			 * \~
			 */
			friend bool operator <= (const BigNum & num1, const BigNum & num2) noexcept {
				// Получаем результат сравнения двух чисел
				const int8_t result = num1.compare(num2);
				// Выводим результат сравнения двух чисел
				return ((result == -1) || (result == 0));
			}
			/**
			 * \~russian
			 * @brief Оператор [>=] сравнения двух чисел
			 *
			 * @param num1 первое число для сравнения
			 * @param num2 второе число для сравнения
			 * @return     результат сравнения
			 *
			 * \~english
			 * @brief Operator [>=] of the comparison of two numbers
			 * @param num1 first number to compare
			 * @param num2 second number to compare
			 * @return     result of the comparison
			 *
			 * \~
			 */
			friend bool operator >= (const BigNum & num1, const BigNum & num2) noexcept {
				// Получаем результат сравнения двух чисел
				const int8_t result = num1.compare(num2);
				// Выводим результат сравнения двух чисел
				return ((result == 1) || (result == 0));
			}
		public:
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * \~
			 */
			BigNum() noexcept;
			/**
			 * \~russian
			 * @brief Конструктор копирования
			 *
			 * @param num число для копирования
			 *
			 *
			 * \~english
			 * @brief Copy constructor
			 * @param num number to copy
			 *
			 * \~
			 */
			BigNum(const BigNum & num) noexcept;
			/**
			 * \~russian
			 * @brief Конструктор перемещения
			 *
			 * @param num число для перемещения
			 *
			 *
			 * \~english
			 * @brief Move constructor
			 * @param num number to move
			 *
			 * \~
			 */
			BigNum(BigNum && num) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @details Конструктор объявлен явным, чтобы строка никогда не приводилась
			 *          к числу неявно и не участвовала в разрешении перегрузок операторов.
			 *
			 * @param text строка числа для установки
			 *
			 * \~english
			 * @brief Constructor
			 * @details The constructor is declared explicit so that a string is never cast
			 *          to a number implicitly and does not take part in the overload resolution of the operators.
			 * @param text string of the number to set
			 *
			 * \~
			 */
			explicit BigNum(const char * text) noexcept;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param text строка числа для установки
			 *
			 *
			 * \~english
			 * @brief Constructor
			 * @param text string of the number to set
			 *
			 * \~
			 */
			explicit BigNum(const string & text) noexcept;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param text строка числа для установки
			 *
			 *
			 * \~english
			 * @brief Constructor
			 * @param text string of the number to set
			 *
			 * \~
			 */
			explicit BigNum(string_view text) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param num значение для установки
			 *
			 *
			 * \~english
			 * @brief Constructor
			 * @param num value to set
			 *
			 * \~
			 */
			BigNum(const bool num) noexcept;
			/**
			 * \~russian
			 * @note Конструкторы встроенных числовых типов намеренно объявлены неявными.
			 *       Неявное преобразование встроенного числа в длинное необходимо для работы
			 *       арифметических операторов вида [5 + num] и [num * 2], а обратное
			 *       преобразование объявлено явным, что исключает неоднозначность разрешения
			 *       перегрузок между операторами длинных чисел и встроенными операторами.
			 *
			 * \~english
			 * @note The constructors of the built-in numeric types are deliberately declared implicit.
			 *       The implicit conversion of a built-in number into a long one is required for the work
			 *       of the arithmetic operators of the [5 + num] and [num * 2] form, while the reverse
			 *       conversion is declared explicit, which rules out the ambiguity of the overload
			 *       resolution between the operators of long numbers and the built-in operators.
			 *
			 * \~
			 */
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param num значение для установки
			 *
			 *
			 * \~english
			 * @brief Constructor
			 * @param num value to set
			 *
			 * \~
			 */
			BigNum(const char num) noexcept;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param num значение для установки
			 *
			 *
			 * \~english
			 * @brief Constructor
			 * @param num value to set
			 *
			 * \~
			 */
			BigNum(const signed char num) noexcept;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param num значение для установки
			 *
			 *
			 * \~english
			 * @brief Constructor
			 * @param num value to set
			 *
			 * \~
			 */
			BigNum(const unsigned char num) noexcept;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param num значение для установки
			 *
			 *
			 * \~english
			 * @brief Constructor
			 * @param num value to set
			 *
			 * \~
			 */
			BigNum(const short num) noexcept;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param num значение для установки
			 *
			 *
			 * \~english
			 * @brief Constructor
			 * @param num value to set
			 *
			 * \~
			 */
			BigNum(const unsigned short num) noexcept;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param num значение для установки
			 *
			 *
			 * \~english
			 * @brief Constructor
			 * @param num value to set
			 *
			 * \~
			 */
			BigNum(const int num) noexcept;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param num значение для установки
			 *
			 *
			 * \~english
			 * @brief Constructor
			 * @param num value to set
			 *
			 * \~
			 */
			BigNum(const unsigned int num) noexcept;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param num значение для установки
			 *
			 *
			 * \~english
			 * @brief Constructor
			 * @param num value to set
			 *
			 * \~
			 */
			BigNum(const long num) noexcept;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param num значение для установки
			 *
			 *
			 * \~english
			 * @brief Constructor
			 * @param num value to set
			 *
			 * \~
			 */
			BigNum(const unsigned long num) noexcept;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param num значение для установки
			 *
			 *
			 * \~english
			 * @brief Constructor
			 * @param num value to set
			 *
			 * \~
			 */
			BigNum(const long long num) noexcept;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param num значение для установки
			 *
			 *
			 * \~english
			 * @brief Constructor
			 * @param num value to set
			 *
			 * \~
			 */
			BigNum(const unsigned long long num) noexcept;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param num значение для установки
			 *
			 *
			 * \~english
			 * @brief Constructor
			 * @param num value to set
			 *
			 * \~
			 */
			BigNum(const float num) noexcept;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param num значение для установки
			 *
			 *
			 * \~english
			 * @brief Constructor
			 * @param num value to set
			 *
			 * \~
			 */
			BigNum(const double num) noexcept;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param num значение для установки
			 *
			 *
			 * \~english
			 * @brief Constructor
			 * @param num value to set
			 *
			 * \~
			 */
			BigNum(const long double num) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Шаблон разрядности и типа исходного длинного числа
			 *
			 * @tparam SIZE размер исходного числа в байтах
			 * @tparam KIND тип исходного числа
			 *
			 *
			 * \~english
			 * @brief Template of the bit width and of the type of the source long number
			 * @tparam SIZE size of the source number in bytes
			 * @tparam KIND type of the source number
			 *
			 * \~
			 */
			template <uint16_t SIZE, bignum::type_t KIND, typename enable_if <((KIND == TYPE) && (SIZE <= BYTES)), int32_t>::type = 0>
			/**
			 * \~russian
			 * @brief Конструктор расширения разрядности длинного числа
			 *
			 * @details Расширение разрядности числа того же типа выполняется без потери
			 *          значения, поэтому конструктор допускает неявное преобразование.
			 *
			 * @param num число для преобразования
			 *
			 * \~english
			 * @brief Constructor of the widening of the bit width of a long number
			 * @details The widening of the bit width of a number of the same type is performed without a loss
			 *          of the value, therefore the constructor admits an implicit conversion.
			 * @param num number to convert
			 *
			 * \~
			 */
			BigNum(const BigNum <SIZE, KIND> & num) noexcept : _data{} {
				// Выполняем установку значения числа иной разрядности
				this->set(num.data(), SIZE, KIND);
			}
			/**
			 * \~russian
			 * @brief Шаблон разрядности и типа исходного длинного числа
			 *
			 * @tparam SIZE размер исходного числа в байтах
			 * @tparam KIND тип исходного числа
			 *
			 *
			 * \~english
			 * @brief Template of the bit width and of the type of the source long number
			 * @tparam SIZE size of the source number in bytes
			 * @tparam KIND type of the source number
			 *
			 * \~
			 */
			template <uint16_t SIZE, bignum::type_t KIND, typename enable_if <!((KIND == TYPE) && (SIZE <= BYTES)), int32_t>::type = 0>
			/**
			 * \~russian
			 * @brief Конструктор сужения разрядности либо смены типа длинного числа
			 *
			 * @details Преобразование способно привести к потере значения либо точности,
			 *          поэтому конструктор объявлен явным и требует явного приведения типа.
			 *
			 * @param num число для преобразования
			 *
			 * \~english
			 * @brief Constructor of the narrowing of the bit width or of the change of the type of a long number
			 * @details The conversion is able to lead to a loss of the value or of the precision,
			 *          therefore the constructor is declared explicit and requires an explicit cast.
			 * @param num number to convert
			 *
			 * \~
			 */
			explicit BigNum(const BigNum <SIZE, KIND> & num) noexcept : _data{} {
				// Выполняем установку значения числа иной разрядности
				this->set(num.data(), SIZE, KIND);
			}
		public:
			/**
			 * \~russian
			 * @brief Деструктор
			 *
			 *
			 * \~english
			 * @brief Destructor
			 *
			 * \~
			 */
			~BigNum() noexcept;
	};
	/**
	 * \~russian
	 * @brief Шаблон разрядности и типа длинного числа
	 *
	 * @tparam BYTES размер числа в байтах
	 * @tparam TYPE  тип хранимого числа
	 *
	 *
	 * \~english
	 * @brief Template of the bit width and of the type of the long number
	 * @tparam BYTES size of the number in bytes
	 * @tparam TYPE  type of the kept number
	 *
	 * \~
	 */
	template <uint16_t BYTES, bignum::type_t TYPE>
	/**
	 * \~russian
	 * @brief Оператор [>>] чтения из потока длинного числа
	 *
	 * @param is  поток для чтения
	 * @param num число для присвоения
	 * @return    поток для чтения
	 *
	 * \~english
	 * @brief Operator [>>] of reading a long number from a stream
	 * @param is  stream to read from
	 * @param num number to assign
	 * @return    the stream to read from
	 *
	 * \~
	 */
	__AWH_SHARED_EXPORT__ istream & operator >> (istream & is, BigNum <BYTES, TYPE> & num) noexcept;
	/**
	 * \~russian
	 * @brief Шаблон разрядности и типа длинного числа
	 *
	 * @tparam BYTES размер числа в байтах
	 * @tparam TYPE  тип хранимого числа
	 *
	 *
	 * \~english
	 * @brief Template of the bit width and of the type of the long number
	 * @tparam BYTES size of the number in bytes
	 * @tparam TYPE  type of the kept number
	 *
	 * \~
	 */
	template <uint16_t BYTES, bignum::type_t TYPE>
	/**
	 * \~russian
	 * @brief Оператор [<<] вывода в поток длинного числа
	 *
	 * @param os  поток куда нужно вывести данные
	 * @param num число для вывода
	 * @return    поток для записи
	 *
	 * \~english
	 * @brief Operator [<<] of outputting a long number to a stream
	 * @param os  stream to output the data to
	 * @param num number to output
	 * @return    the stream to write to
	 *
	 * \~
	 */
	__AWH_SHARED_EXPORT__ ostream & operator << (ostream & os, const BigNum <BYTES, TYPE> & num) noexcept;
	/**
	 * \~russian
	 * @brief Шаблон разрядности знакового длинного числа
	 *
	 * @tparam BYTES размер числа в байтах
	 *
	 * \~english
	 * @brief Template of the bit width of a signed long number
	 * @tparam BYTES size of the number in bytes
	 *
	 * \~
	 */
	template <uint16_t BYTES>
	/**
	 * \~russian
	 * @brief Создаём тип данных знакового длинного числа
	 *
	 * \~english
	 * @brief Create the data type of a signed long number
	 *
	 * \~
	 */
	using bigint_t = BigNum <BYTES, bignum::type_t::SIGNED>;
	/**
	 * \~russian
	 * @brief Шаблон разрядности беззнакового длинного числа
	 *
	 * @tparam BYTES размер числа в байтах
	 *
	 * \~english
	 * @brief Template of the bit width of an unsigned long number
	 * @tparam BYTES size of the number in bytes
	 *
	 * \~
	 */
	template <uint16_t BYTES>
	/**
	 * \~russian
	 * @brief Создаём тип данных беззнакового длинного числа
	 *
	 * \~english
	 * @brief Create the data type of an unsigned long number
	 *
	 * \~
	 */
	using biguint_t = BigNum <BYTES, bignum::type_t::UNSIGNED>;
	/**
	 * \~russian
	 * @brief Шаблон разрядности вещественного длинного числа
	 *
	 * @tparam BYTES размер числа в байтах
	 *
	 * \~english
	 * @brief Template of the bit width of a real long number
	 * @tparam BYTES size of the number in bytes
	 *
	 * \~
	 */
	template <uint16_t BYTES>
	/**
	 * \~russian
	 * @brief Создаём тип данных вещественного длинного числа
	 *
	 * \~english
	 * @brief Create the data type of a real long number
	 *
	 * \~
	 */
	using bigreal_t = BigNum <BYTES, bignum::type_t::REAL>;
	/**
	 * \~russian
	 * @brief Создаём готовые типы данных знаковых длинных чисел
	 *
	 * @details Помимо стандартных разрядностей модуль предоставляет промежуточные
	 *          нестандартные разрядности, позволяющие подобрать минимально достаточный
	 *          размер числа под конкретную задачу и не расходовать память впустую.
	 *
	 * \~english
	 * @brief Create the ready data types of signed long numbers
	 * @details Besides the standard bit widths the module provides intermediate
	 *          non-standard bit widths, which allow choosing the minimally sufficient
	 *          size of a number for a particular task and not spending memory in vain.
	 *
	 * \~
	 */
	using int24_t    = bigint_t <3>;    // Знаковое 24-битное целое число
	using int40_t    = bigint_t <5>;    // Знаковое 40-битное целое число
	using int48_t    = bigint_t <6>;    // Знаковое 48-битное целое число
	using int56_t    = bigint_t <7>;    // Знаковое 56-битное целое число
	using int72_t    = bigint_t <9>;    // Знаковое 72-битное целое число
	using int80_t    = bigint_t <10>;   // Знаковое 80-битное целое число
	using int96_t    = bigint_t <12>;   // Знаковое 96-битное целое число
	using int128_t   = bigint_t <16>;   // Знаковое 128-битное целое число
	using int160_t   = bigint_t <20>;   // Знаковое 160-битное целое число
	using int192_t   = bigint_t <24>;   // Знаковое 192-битное целое число
	using int224_t   = bigint_t <28>;   // Знаковое 224-битное целое число
	using int256_t   = bigint_t <32>;   // Знаковое 256-битное целое число
	using int320_t   = bigint_t <40>;   // Знаковое 320-битное целое число
	using int384_t   = bigint_t <48>;   // Знаковое 384-битное целое число
	using int512_t   = bigint_t <64>;   // Знаковое 512-битное целое число
	using int768_t   = bigint_t <96>;   // Знаковое 768-битное целое число
	using int1024_t  = bigint_t <128>;  // Знаковое 1024-битное целое число
	using int1536_t  = bigint_t <192>;  // Знаковое 1536-битное целое число
	using int2048_t  = bigint_t <256>;  // Знаковое 2048-битное целое число
	using int3072_t  = bigint_t <384>;  // Знаковое 3072-битное целое число
	using int4096_t  = bigint_t <512>;  // Знаковое 4096-битное целое число
	using int6144_t  = bigint_t <768>;  // Знаковое 6144-битное целое число
	using int8192_t  = bigint_t <1024>; // Знаковое 8192-битное целое число
	/**
	 * \~russian
	 * @brief Создаём готовые типы данных беззнаковых длинных чисел
	 *
	 * \~english
	 * @brief Create the ready data types of unsigned long numbers
	 *
	 * \~
	 */
	using uint24_t   = biguint_t <3>;    // Беззнаковое 24-битное целое число
	using uint40_t   = biguint_t <5>;    // Беззнаковое 40-битное целое число
	using uint48_t   = biguint_t <6>;    // Беззнаковое 48-битное целое число
	using uint56_t   = biguint_t <7>;    // Беззнаковое 56-битное целое число
	using uint72_t   = biguint_t <9>;    // Беззнаковое 72-битное целое число
	using uint80_t   = biguint_t <10>;   // Беззнаковое 80-битное целое число
	using uint96_t   = biguint_t <12>;   // Беззнаковое 96-битное целое число
	using uint128_t  = biguint_t <16>;   // Беззнаковое 128-битное целое число
	using uint160_t  = biguint_t <20>;   // Беззнаковое 160-битное целое число
	using uint192_t  = biguint_t <24>;   // Беззнаковое 192-битное целое число
	using uint224_t  = biguint_t <28>;   // Беззнаковое 224-битное целое число
	using uint256_t  = biguint_t <32>;   // Беззнаковое 256-битное целое число
	using uint320_t  = biguint_t <40>;   // Беззнаковое 320-битное целое число
	using uint384_t  = biguint_t <48>;   // Беззнаковое 384-битное целое число
	using uint512_t  = biguint_t <64>;   // Беззнаковое 512-битное целое число
	using uint768_t  = biguint_t <96>;   // Беззнаковое 768-битное целое число
	using uint1024_t = biguint_t <128>;  // Беззнаковое 1024-битное целое число
	using uint1536_t = biguint_t <192>;  // Беззнаковое 1536-битное целое число
	using uint2048_t = biguint_t <256>;  // Беззнаковое 2048-битное целое число
	using uint3072_t = biguint_t <384>;  // Беззнаковое 3072-битное целое число
	using uint4096_t = biguint_t <512>;  // Беззнаковое 4096-битное целое число
	using uint6144_t = biguint_t <768>;  // Беззнаковое 6144-битное целое число
	using uint8192_t = biguint_t <1024>; // Беззнаковое 8192-битное целое число
	/**
	 * \~russian
	 * @brief Создаём готовые типы данных вещественных длинных чисел
	 *
	 * @details Типы намеренно названы real вместо float, поскольку начиная со стандарта
	 *          C++23 имена float16_t, float32_t, float64_t и float128_t объявлены в
	 *          пространстве имён std и привели бы к неоднозначности разрешения имён.
	 *
	 * @note    Разрядность порядка нестандартных форматов рассчитывается по общей формуле
	 *          стандарта IEEE-754, поэтому такие типы не совпадают с одноимёнными аппаратными
	 *          форматами. В частности, real80_t это binary80 с двенадцатиразрядным порядком и
	 *          неявным старшим разрядом мантиссы, тогда как расширенный формат x87, которым на
	 *          архитектуре x86 представлен long double, имеет пятнадцатиразрядный порядок и
	 *          явный старший разряд мантиссы. Взаимно однозначного соответствия между ними нет,
	 *          и побитовый перенос значения из одного формата в другой недопустим.
	 *
	 * \~english
	 * @brief Create the ready data types of real long numbers
	 * @details The types are deliberately named real instead of float, since starting with the
	 *          C++23 standard the names float16_t, float32_t, float64_t and float128_t are declared in
	 *          the std namespace and would lead to an ambiguity of the name resolution.
	 * @note    The bit width of the exponent of the non-standard formats is computed by the common formula
	 *          of the IEEE-754 standard, therefore such types do not coincide with the same-named hardware
	 *          formats. In particular, real80_t is binary80 with a twelve-bit exponent and
	 *          an implicit high bit of the mantissa, whereas the extended x87 format, by which on
	 *          the x86 architecture long double is represented, has a fifteen-bit exponent and
	 *          an explicit high bit of the mantissa. There is no one-to-one correspondence between them,
	 *          and a bitwise transfer of a value from one format into the other is inadmissible.
	 *
	 * \~
	 */
	using real16_t   = bigreal_t <2>;    // Вещественное 16-битное число половинной точности
	using real24_t   = bigreal_t <3>;    // Вещественное 24-битное число
	using real32_t   = bigreal_t <4>;    // Вещественное 32-битное число одинарной точности
	using real48_t   = bigreal_t <6>;    // Вещественное 48-битное число
	using real64_t   = bigreal_t <8>;    // Вещественное 64-битное число двойной точности
	using real80_t   = bigreal_t <10>;   // Вещественное 80-битное число
	using real96_t   = bigreal_t <12>;   // Вещественное 96-битное число
	using real128_t  = bigreal_t <16>;   // Вещественное 128-битное число четверной точности
	using real192_t  = bigreal_t <24>;   // Вещественное 192-битное число
	using real256_t  = bigreal_t <32>;   // Вещественное 256-битное число восьмерной точности
	using real384_t  = bigreal_t <48>;   // Вещественное 384-битное число
	using real512_t  = bigreal_t <64>;   // Вещественное 512-битное число
	using real768_t  = bigreal_t <96>;   // Вещественное 768-битное число
	using real1024_t = bigreal_t <128>;  // Вещественное 1024-битное число
};

#endif // __AWH_BIGNUM__
