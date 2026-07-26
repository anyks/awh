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
 * @brief Заголовочный файл модуля длинных чисел — шаблонный класс BigNum, хранящий число произвольной разрядности
 *        в массиве байтов фиксированного размера и поддерживающий знаковую, беззнаковую и вещественную арифметику
 *        с полной совместимостью со встроенными числовыми типами, строками и потоками ввода/вывода
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
#include "global.hpp"

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
	 * @brief Пространство имён модуля длинных чисел
	 *
	 * @details Пространство имён содержит размер-независимый вычислительный движок,
	 *          выполняющий все арифметические операции над сырыми буферами байтов.
	 *          Буфер числа всегда хранится в порядке от младшего байта к старшему
	 *          (little-endian) вне зависимости от порядка байтов процессора, знаковые
	 *          числа хранятся в дополнительном коде, беззнаковые — в прямом коде,
	 *          вещественные — в формате IEEE-754 binary(N).
	 *
	 */
	namespace bignum {
		/**
		 * @brief Тип хранимого числа
		 *
		 */
		enum class type_t : uint8_t {
			NONE     = 0x00, // Тип числа не установлен
			SIGNED   = 0x01, // Знаковое целое число в дополнительном коде
			UNSIGNED = 0x02, // Беззнаковое целое число
			REAL     = 0x03  // Вещественное число в формате IEEE-754
		};
		/**
		 * @brief Формат представления числа в виде строки
		 *
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
		 * @brief Правило округления числа
		 *
		 */
		enum class round_t : uint8_t {
			NEAREST = 0x00, // К ближайшему значению, половина округляется от нуля
			EVEN    = 0x01, // К ближайшему значению, половина округляется к чётному
			DOWN    = 0x02, // В сторону минус бесконечности
			UP      = 0x03, // В сторону плюс бесконечности
			ZERO    = 0x04  // В сторону нуля с отбрасыванием младших разрядов
		};
		/**
		 * @brief Класс значения числа
		 *
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
	 * @brief Пространство имён вычислительного движка длинных чисел
	 *
	 */
	namespace bignum {
		/**
		 * @brief Метод обнуления буфера числа
		 *
		 * @param value буфер числа для обнуления
		 * @param size  размер буфера числа в байтах
		 *
		 */
		__AWH_SHARED_EXPORT__ void reset(uint8_t * value, const size_t size) noexcept;
		/**
		 * @brief Метод проверки буфера числа на нулевое значение
		 *
		 * @param value буфер числа для проверки
		 * @param size  размер буфера числа в байтах
		 * @return      результат проверки
		 *
		 */
		__AWH_SHARED_EXPORT__ bool zero(const uint8_t * value, const size_t size) noexcept;
		/**
		 * @brief Метод извлечения количества значащих бит числа
		 *
		 * @param value буфер числа для подсчёта
		 * @param size  размер буфера числа в байтах
		 * @return      позиция старшего установленного бита увеличенная на единицу
		 *
		 */
		__AWH_SHARED_EXPORT__ size_t bits(const uint8_t * value, const size_t size) noexcept;
		/**
		 * @brief Метод извлечения значения бита числа
		 *
		 * @param value буфер числа для извлечения
		 * @param size  размер буфера числа в байтах
		 * @param index индекс извлекаемого бита
		 * @return      значение указанного бита
		 *
		 */
		__AWH_SHARED_EXPORT__ bool bit(const uint8_t * value, const size_t size, const size_t index) noexcept;
		/**
		 * @brief Метод установки значения бита числа
		 *
		 * @param value буфер числа для установки
		 * @param size  размер буфера числа в байтах
		 * @param index индекс устанавливаемого бита
		 * @param mode  устанавливаемое значение бита
		 *
		 */
		__AWH_SHARED_EXPORT__ void bit(uint8_t * value, const size_t size, const size_t index, const bool mode) noexcept;
		/**
		 * @brief Метод проверки числа на отрицательное значение
		 *
		 * @param value буфер числа для проверки
		 * @param size  размер буфера числа в байтах
		 * @return      результат проверки старшего бита числа
		 *
		 */
		__AWH_SHARED_EXPORT__ bool negative(const uint8_t * value, const size_t size) noexcept;
		/**
		 * @brief Метод сложения двух целых чисел
		 *
		 * @param result буфер числа приёмника и первого слагаемого
		 * @param value  буфер числа второго слагаемого
		 * @param size   размер буферов чисел в байтах
		 * @return       флаг переноса за пределы разрядной сетки
		 *
		 */
		__AWH_SHARED_EXPORT__ bool add(uint8_t * result, const uint8_t * value, const size_t size) noexcept;
		/**
		 * @brief Метод вычитания двух целых чисел
		 *
		 * @param result буфер числа приёмника и уменьшаемого
		 * @param value  буфер числа вычитаемого
		 * @param size   размер буферов чисел в байтах
		 * @return       флаг заёма за пределами разрядной сетки
		 *
		 */
		__AWH_SHARED_EXPORT__ bool sub(uint8_t * result, const uint8_t * value, const size_t size) noexcept;
		/**
		 * @brief Метод умножения двух целых чисел
		 *
		 * @details Результат умножения усекается до размера разрядной сетки.
		 *
		 * @param result буфер числа приёмника и множимого
		 * @param value  буфер числа множителя
		 * @param size   размер буферов чисел в байтах
		 *
		 */
		__AWH_SHARED_EXPORT__ void mul(uint8_t * result, const uint8_t * value, const size_t size) noexcept;
		/**
		 * @brief Метод деления двух беззнаковых целых чисел с получением остатка
		 *
		 * @details При делении на нуль частное и остаток обнуляются.
		 *
		 * @param result буфер числа приёмника частного и делимого
		 * @param value  буфер числа делителя
		 * @param mod    буфер числа приёмника остатка от деления
		 * @param size   размер буферов чисел в байтах
		 * @return       результат выполнения деления
		 *
		 */
		__AWH_SHARED_EXPORT__ bool divmod(uint8_t * result, const uint8_t * value, uint8_t * mod, const size_t size) noexcept;
		/**
		 * @brief Метод смены знака целого числа в дополнительном коде
		 *
		 * @param value буфер числа для смены знака
		 * @param size  размер буфера числа в байтах
		 *
		 */
		__AWH_SHARED_EXPORT__ void neg(uint8_t * value, const size_t size) noexcept;
		/**
		 * @brief Метод побитовой инверсии целого числа
		 *
		 * @param value буфер числа для инверсии
		 * @param size  размер буфера числа в байтах
		 *
		 */
		__AWH_SHARED_EXPORT__ void inv(uint8_t * value, const size_t size) noexcept;
		/**
		 * @brief Метод побитового умножения двух целых чисел
		 *
		 * @param result буфер числа приёмника и первого операнда
		 * @param value  буфер числа второго операнда
		 * @param size   размер буферов чисел в байтах
		 *
		 */
		__AWH_SHARED_EXPORT__ void band(uint8_t * result, const uint8_t * value, const size_t size) noexcept;
		/**
		 * @brief Метод побитового сложения двух целых чисел
		 *
		 * @param result буфер числа приёмника и первого операнда
		 * @param value  буфер числа второго операнда
		 * @param size   размер буферов чисел в байтах
		 *
		 */
		__AWH_SHARED_EXPORT__ void bor(uint8_t * result, const uint8_t * value, const size_t size) noexcept;
		/**
		 * @brief Метод побитового исключающего сложения двух целых чисел
		 *
		 * @param result буфер числа приёмника и первого операнда
		 * @param value  буфер числа второго операнда
		 * @param size   размер буферов чисел в байтах
		 *
		 */
		__AWH_SHARED_EXPORT__ void bxor(uint8_t * result, const uint8_t * value, const size_t size) noexcept;
		/**
		 * @brief Метод сдвига целого числа влево
		 *
		 * @param value буфер числа для сдвига
		 * @param size  размер буфера числа в байтах
		 * @param count количество бит сдвига
		 *
		 */
		__AWH_SHARED_EXPORT__ void shl(uint8_t * value, const size_t size, const size_t count) noexcept;
		/**
		 * @brief Метод сдвига целого числа вправо
		 *
		 * @param value буфер числа для сдвига
		 * @param size  размер буфера числа в байтах
		 * @param count количество бит сдвига
		 * @param sign  флаг выполнения арифметического сдвига с сохранением знака
		 *
		 */
		__AWH_SHARED_EXPORT__ void shr(uint8_t * value, const size_t size, const size_t count, const bool sign) noexcept;
		/**
		 * @brief Метод сравнения двух беззнаковых целых чисел
		 *
		 * @param value1 буфер первого числа для сравнения
		 * @param value2 буфер второго числа для сравнения
		 * @param size   размер буферов чисел в байтах
		 * @return       результат сравнения (-1, 0 или 1)
		 *
		 */
		__AWH_SHARED_EXPORT__ int8_t ucompare(const uint8_t * value1, const uint8_t * value2, const size_t size) noexcept;
		/**
		 * @brief Метод сравнения двух знаковых целых чисел
		 *
		 * @param value1 буфер первого числа для сравнения
		 * @param value2 буфер второго числа для сравнения
		 * @param size   размер буферов чисел в байтах
		 * @return       результат сравнения (-1, 0 или 1)
		 *
		 */
		__AWH_SHARED_EXPORT__ int8_t scompare(const uint8_t * value1, const uint8_t * value2, const size_t size) noexcept;
		/**
		 * @brief Метод извлечения целочисленного квадратного корня беззнакового числа
		 *
		 * @param value буфер числа для извлечения корня
		 * @param size  размер буфера числа в байтах
		 *
		 */
		__AWH_SHARED_EXPORT__ void sqrt(uint8_t * value, const size_t size) noexcept;
		/**
		 * @brief Метод возведения целого числа в степень
		 *
		 * @param value    буфер числа для возведения в степень
		 * @param size     размер буфера числа в байтах
		 * @param exponent показатель степени
		 *
		 */
		__AWH_SHARED_EXPORT__ void pow(uint8_t * value, const size_t size, const uint64_t exponent) noexcept;
		/**
		 * @brief Метод установки значения целого числа
		 *
		 * @param value буфер числа для установки
		 * @param size  размер буфера числа в байтах
		 * @param num   устанавливаемое значение по модулю
		 * @param sign  флаг отрицательного значения устанавливаемого числа
		 *
		 */
		__AWH_SHARED_EXPORT__ void set(uint8_t * value, const size_t size, const uint64_t num, const bool sign) noexcept;
		/**
		 * @brief Метод извлечения беззнакового целого значения числа
		 *
		 * @param value буфер числа для извлечения
		 * @param size  размер буфера числа в байтах
		 * @return      извлечённое значение младших разрядов числа
		 *
		 */
		__AWH_SHARED_EXPORT__ uint64_t getUint(const uint8_t * value, const size_t size) noexcept;
		/**
		 * @brief Метод извлечения знакового целого значения числа
		 *
		 * @param value буфер числа для извлечения
		 * @param size  размер буфера числа в байтах
		 * @return      извлечённое значение младших разрядов числа
		 *
		 */
		__AWH_SHARED_EXPORT__ int64_t getInt(const uint8_t * value, const size_t size) noexcept;
		/**
		 * @brief Метод извлечения вещественного значения целого числа
		 *
		 * @param value буфер числа для извлечения
		 * @param size  размер буфера числа в байтах
		 * @param sign  флаг знакового представления числа
		 * @return      извлечённое вещественное значение числа
		 *
		 */
		__AWH_SHARED_EXPORT__ long double getReal(const uint8_t * value, const size_t size, const bool sign) noexcept;
		/**
		 * @brief Метод установки вещественного значения целого числа
		 *
		 * @details Дробная часть устанавливаемого значения отбрасывается.
		 *
		 * @param value буфер числа для установки
		 * @param size  размер буфера числа в байтах
		 * @param num   устанавливаемое вещественное значение
		 * @param sign  флаг знакового представления числа
		 *
		 */
		__AWH_SHARED_EXPORT__ void setReal(uint8_t * value, const size_t size, const long double num, const bool sign) noexcept;
		/**
		 * @brief Метод формирования строкового представления целого числа
		 *
		 * @param value  буфер числа для формирования
		 * @param size   размер буфера числа в байтах
		 * @param sign   флаг знакового представления числа
		 * @param format формат представления числа
		 * @return       сформированная строка числа
		 *
		 */
		__AWH_SHARED_EXPORT__ string print(const uint8_t * value, const size_t size, const bool sign, const format_t format) noexcept;
		/**
		 * @brief Метод разбора строкового представления целого числа
		 *
		 * @param value  буфер числа для установки результата разбора
		 * @param size   размер буфера числа в байтах
		 * @param text   разбираемая строка числа
		 * @param sign   флаг знакового представления числа
		 * @param format формат представления числа
		 * @return       результат выполнения разбора
		 *
		 */
		__AWH_SHARED_EXPORT__ bool parse(uint8_t * value, const size_t size, string_view text, const bool sign, const format_t format) noexcept;
		/**
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
		 */
		__AWH_SHARED_EXPORT__ void round(uint8_t * value, const size_t size, const bool sign, const int32_t digits, const round_t mode) noexcept;
	};
	/**
	 * @brief Пространство имён вещественной арифметики длинных чисел
	 *
	 */
	namespace bignum {
		/**
		 * @brief Метод извлечения разрядности порядка вещественного числа
		 *
		 * @details Для стандартных разрядностей 16, 32, 64 и 128 бит используется
		 *          разрядность порядка, определённая стандартом IEEE-754, для
		 *          остальных разрядностей применяется формула round(4 * log2(N)) - 13.
		 *
		 * @param bits разрядность вещественного числа в битах
		 * @return     разрядность порядка вещественного числа в битах
		 *
		 */
		__AWH_SHARED_EXPORT__ size_t exponentBits(const size_t bits) noexcept;
		/**
		 * @brief Метод определения класса вещественного числа
		 *
		 * @param value буфер числа для определения
		 * @param size  размер буфера числа в байтах
		 * @return      класс значения вещественного числа
		 *
		 */
		__AWH_SHARED_EXPORT__ class_t classify(const uint8_t * value, const size_t size) noexcept;
		/**
		 * @brief Метод формирования бесконечности вещественного числа
		 *
		 * @param value буфер числа для формирования
		 * @param size  размер буфера числа в байтах
		 * @param sign  флаг формирования отрицательной бесконечности
		 *
		 */
		__AWH_SHARED_EXPORT__ void realInf(uint8_t * value, const size_t size, const bool sign) noexcept;
		/**
		 * @brief Метод формирования значения не являющегося числом
		 *
		 * @param value буфер числа для формирования
		 * @param size  размер буфера числа в байтах
		 *
		 */
		__AWH_SHARED_EXPORT__ void realNan(uint8_t * value, const size_t size) noexcept;
		/**
		 * @brief Метод формирования машинного эпсилон вещественного числа
		 *
		 * @param value буфер числа для формирования
		 * @param size  размер буфера числа в байтах
		 *
		 */
		__AWH_SHARED_EXPORT__ void realEpsilon(uint8_t * value, const size_t size) noexcept;
		/**
		 * @brief Метод формирования максимального конечного вещественного числа
		 *
		 * @param value буфер числа для формирования
		 * @param size  размер буфера числа в байтах
		 * @param sign  флаг формирования минимального отрицательного числа
		 *
		 */
		__AWH_SHARED_EXPORT__ void realLimit(uint8_t * value, const size_t size, const bool sign) noexcept;
		/**
		 * @brief Метод смены знака вещественного числа
		 *
		 * @param value буфер числа для смены знака
		 * @param size  размер буфера числа в байтах
		 *
		 */
		__AWH_SHARED_EXPORT__ void realNeg(uint8_t * value, const size_t size) noexcept;
		/**
		 * @brief Метод извлечения модуля вещественного числа
		 *
		 * @param value буфер числа для извлечения модуля
		 * @param size  размер буфера числа в байтах
		 *
		 */
		__AWH_SHARED_EXPORT__ void realAbs(uint8_t * value, const size_t size) noexcept;
		/**
		 * @brief Метод сложения двух вещественных чисел
		 *
		 * @param result буфер числа приёмника и первого слагаемого
		 * @param value  буфер числа второго слагаемого
		 * @param size   размер буферов чисел в байтах
		 *
		 */
		__AWH_SHARED_EXPORT__ void realAdd(uint8_t * result, const uint8_t * value, const size_t size) noexcept;
		/**
		 * @brief Метод вычитания двух вещественных чисел
		 *
		 * @param result буфер числа приёмника и уменьшаемого
		 * @param value  буфер числа вычитаемого
		 * @param size   размер буферов чисел в байтах
		 *
		 */
		__AWH_SHARED_EXPORT__ void realSub(uint8_t * result, const uint8_t * value, const size_t size) noexcept;
		/**
		 * @brief Метод умножения двух вещественных чисел
		 *
		 * @param result буфер числа приёмника и множимого
		 * @param value  буфер числа множителя
		 * @param size   размер буферов чисел в байтах
		 *
		 */
		__AWH_SHARED_EXPORT__ void realMul(uint8_t * result, const uint8_t * value, const size_t size) noexcept;
		/**
		 * @brief Метод деления двух вещественных чисел
		 *
		 * @param result буфер числа приёмника и делимого
		 * @param value  буфер числа делителя
		 * @param size   размер буферов чисел в байтах
		 *
		 */
		__AWH_SHARED_EXPORT__ void realDiv(uint8_t * result, const uint8_t * value, const size_t size) noexcept;
		/**
		 * @brief Метод извлечения остатка от деления двух вещественных чисел
		 *
		 * @param result буфер числа приёмника и делимого
		 * @param value  буфер числа делителя
		 * @param size   размер буферов чисел в байтах
		 *
		 */
		__AWH_SHARED_EXPORT__ void realMod(uint8_t * result, const uint8_t * value, const size_t size) noexcept;
		/**
		 * @brief Метод извлечения квадратного корня вещественного числа
		 *
		 * @param value буфер числа для извлечения корня
		 * @param size  размер буфера числа в байтах
		 *
		 */
		__AWH_SHARED_EXPORT__ void realSqrt(uint8_t * value, const size_t size) noexcept;
		/**
		 * @brief Метод возведения вещественного числа в целую степень
		 *
		 * @param value    буфер числа для возведения в степень
		 * @param size     размер буфера числа в байтах
		 * @param exponent показатель степени
		 *
		 */
		__AWH_SHARED_EXPORT__ void realPow(uint8_t * value, const size_t size, const uint64_t exponent) noexcept;
		/**
		 * @brief Метод сравнения двух вещественных чисел
		 *
		 * @param value1 буфер первого числа для сравнения
		 * @param value2 буфер второго числа для сравнения
		 * @param size   размер буферов чисел в байтах
		 * @return       результат сравнения (-1, 0, 1 или 2 для несравнимых значений)
		 *
		 */
		__AWH_SHARED_EXPORT__ int8_t realCompare(const uint8_t * value1, const uint8_t * value2, const size_t size) noexcept;
		/**
		 * @brief Метод установки целого значения вещественного числа
		 *
		 * @param value буфер числа для установки
		 * @param size  размер буфера числа в байтах
		 * @param num   устанавливаемое значение по модулю
		 * @param sign  флаг отрицательного значения устанавливаемого числа
		 *
		 */
		__AWH_SHARED_EXPORT__ void realSet(uint8_t * value, const size_t size, const uint64_t num, const bool sign) noexcept;
		/**
		 * @brief Метод установки вещественного значения числа
		 *
		 * @param value буфер числа для установки
		 * @param size  размер буфера числа в байтах
		 * @param num   устанавливаемое вещественное значение
		 *
		 */
		__AWH_SHARED_EXPORT__ void realSetReal(uint8_t * value, const size_t size, const long double num) noexcept;
		/**
		 * @brief Метод извлечения беззнакового целого значения вещественного числа
		 *
		 * @param value буфер числа для извлечения
		 * @param size  размер буфера числа в байтах
		 * @return      извлечённое целое значение числа
		 *
		 */
		__AWH_SHARED_EXPORT__ uint64_t realGetUint(const uint8_t * value, const size_t size) noexcept;
		/**
		 * @brief Метод извлечения знакового целого значения вещественного числа
		 *
		 * @param value буфер числа для извлечения
		 * @param size  размер буфера числа в байтах
		 * @return      извлечённое целое значение числа
		 *
		 */
		__AWH_SHARED_EXPORT__ int64_t realGetInt(const uint8_t * value, const size_t size) noexcept;
		/**
		 * @brief Метод извлечения вещественного значения числа
		 *
		 * @param value буфер числа для извлечения
		 * @param size  размер буфера числа в байтах
		 * @return      извлечённое вещественное значение числа
		 *
		 */
		__AWH_SHARED_EXPORT__ long double realGetReal(const uint8_t * value, const size_t size) noexcept;
		/**
		 * @brief Метод переноса вещественного числа в буфер другой разрядности
		 *
		 * @param result буфер числа приёмника
		 * @param size1  размер буфера числа приёмника в байтах
		 * @param value  буфер числа источника
		 * @param size2  размер буфера числа источника в байтах
		 *
		 */
		__AWH_SHARED_EXPORT__ void realCast(uint8_t * result, const size_t size1, const uint8_t * value, const size_t size2) noexcept;
		/**
		 * @brief Метод формирования строкового представления вещественного числа
		 *
		 * @param value     буфер числа для формирования
		 * @param size      размер буфера числа в байтах
		 * @param format    формат представления числа
		 * @param precision количество знаков после запятой (отрицательное значение - автоматически)
		 * @return          сформированная строка числа
		 *
		 */
		__AWH_SHARED_EXPORT__ string realPrint(const uint8_t * value, const size_t size, const format_t format, const int16_t precision) noexcept;
		/**
		 * @brief Метод разбора строкового представления вещественного числа
		 *
		 * @param value буфер числа для установки результата разбора
		 * @param size  размер буфера числа в байтах
		 * @param text  разбираемая строка числа
		 * @return      результат выполнения разбора
		 *
		 */
		__AWH_SHARED_EXPORT__ bool realParse(uint8_t * value, const size_t size, string_view text) noexcept;
		/**
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
		 */
		__AWH_SHARED_EXPORT__ void realRound(uint8_t * value, const size_t size, const int32_t digits, const round_t mode) noexcept;
	};
	/**
	 * @brief Шаблон разрядности и типа длинного числа
	 *
	 * @tparam BYTES размер числа в байтах
	 * @tparam TYPE  тип хранимого числа
	 *
	 */
	template <uint16_t BYTES, bignum::type_t TYPE = bignum::type_t::SIGNED>
	/**
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
	 * @note    Реализация класса вынесена в исходный файл, поэтому доступны только те
	 *          разрядности, для которых объявлены прототипы в конце файла src/sys/bignum.cpp.
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
			 * @brief Создаём тип данных типа хранимого числа
			 *
			 */
			using type_t = bignum::type_t;
			/**
			 * @brief Создаём тип данных класса значения числа
			 *
			 */
			using class_t = bignum::class_t;
			/**
			 * @brief Создаём тип данных формата представления числа
			 *
			 */
			using format_t = bignum::format_t;
			/**
			 * @brief Создаём тип данных правила округления числа
			 *
			 */
			using round_t = bignum::round_t;
		private:
			// Буфер хранения числа в порядке от младшего байта к старшему
			array <uint8_t, BYTES> _data;
		public:
			/**
			 * @brief Метод извлечения размера числа в байтах
			 *
			 * @return размер числа в байтах
			 *
			 */
			static constexpr uint16_t size() noexcept {
				// Выводим размер числа в байтах
				return BYTES;
			}
			/**
			 * @brief Метод извлечения разрядности числа в битах
			 *
			 * @return разрядность числа в битах
			 *
			 */
			static constexpr uint32_t bitness() noexcept {
				// Выводим разрядность числа в битах
				return (static_cast <uint32_t> (BYTES) * 8);
			}
			/**
			 * @brief Метод извлечения типа хранимого числа
			 *
			 * @return тип хранимого числа
			 *
			 */
			static constexpr type_t kind() noexcept {
				// Выводим тип хранимого числа
				return TYPE;
			}
		public:
			/**
			 * @brief Метод формирования минимально возможного значения числа
			 *
			 * @return минимально возможное значение числа
			 *
			 */
			static BigNum minimum() noexcept;
			/**
			 * @brief Метод формирования максимально возможного значения числа
			 *
			 * @return максимально возможное значение числа
			 *
			 */
			static BigNum maximum() noexcept;
			/**
			 * @brief Метод формирования машинного эпсилон числа
			 *
			 * @details Для целочисленных типов метод возвращает единицу.
			 *
			 * @return машинный эпсилон числа
			 *
			 */
			static BigNum epsilon() noexcept;
			/**
			 * @brief Метод формирования значения бесконечности
			 *
			 * @details Для целочисленных типов метод возвращает максимально возможное значение.
			 *
			 * @return значение бесконечности
			 *
			 */
			static BigNum unlimited() noexcept;
			/**
			 * @brief Метод формирования значения не являющегося числом
			 *
			 * @details Для целочисленных типов метод возвращает нулевое значение.
			 *
			 * @return значение не являющееся числом
			 *
			 */
			static BigNum undefined() noexcept;
		public:
			/**
			 * @brief Метод деления двух чисел с получением частного и остатка
			 *
			 * @param num1      делимое число
			 * @param num2      делитель числа
			 * @param quotient  ссылка на частное от деления
			 * @param remainder ссылка на остаток от деления
			 *
			 */
			static void divmod(const BigNum & num1, const BigNum & num2, BigNum & quotient, BigNum & remainder) noexcept;
		public:
			/**
			 * @brief Метод очистки значения числа
			 *
			 */
			void clear() noexcept;
			/**
			 * @brief Метод обмена значениями двух чисел
			 *
			 * @param num число для обмена значениями
			 *
			 */
			void swap(BigNum & num) noexcept;
		public:
			/**
			 * @brief Метод проверки числа на нулевое значение
			 *
			 * @return результат проверки
			 *
			 */
			bool zero() const noexcept;
			/**
			 * @brief Метод проверки числа на отрицательное значение
			 *
			 * @return результат проверки
			 *
			 */
			bool negative() const noexcept;
			/**
			 * @brief Метод проверки числа на нечётное значение
			 *
			 * @return результат проверки
			 *
			 */
			bool odd() const noexcept;
			/**
			 * @brief Метод проверки числа на чётное значение
			 *
			 * @return результат проверки
			 *
			 */
			bool even() const noexcept;
		public:
			/**
			 * @brief Метод определения класса значения числа
			 *
			 * @return класс значения числа
			 *
			 */
			class_t category() const noexcept;
			/**
			 * @brief Метод извлечения количества значащих бит числа
			 *
			 * @return количество значащих бит числа
			 *
			 */
			uint32_t bits() const noexcept;
		public:
			/**
			 * @brief Метод извлечения значения бита числа
			 *
			 * @param index индекс извлекаемого бита
			 * @return      значение указанного бита
			 *
			 */
			bool bit(const uint32_t index) const noexcept;
			/**
			 * @brief Метод установки значения бита числа
			 *
			 * @param index индекс устанавливаемого бита
			 * @param mode  устанавливаемое значение бита
			 *
			 */
			void bit(const uint32_t index, const bool mode) noexcept;
		public:
			/**
			 * @brief Метод извлечения буфера числа
			 *
			 * @return буфер хранения числа
			 *
			 */
			uint8_t * data() noexcept;
			/**
			 * @brief Метод извлечения буфера числа
			 *
			 * @return буфер хранения числа
			 *
			 */
			const uint8_t * data() const noexcept;
			/**
			 * @brief Метод извлечения массива байтов числа
			 *
			 * @return массив байтов хранения числа
			 *
			 */
			const array <uint8_t, BYTES> & bytes() const noexcept;
		public:
			/**
			 * @brief Метод установки значения числа из внешнего буфера
			 *
			 * @details Метод выполняет преобразование разрядности и типа числа.
			 *
			 * @param buffer буфер числа источника в порядке от младшего байта к старшему
			 * @param size   размер буфера числа источника в байтах
			 * @param type   тип числа источника
			 *
			 */
			void set(const uint8_t * buffer, const uint16_t size, const type_t type) noexcept;
			/**
			 * @brief Метод извлечения значения числа во внешний буфер
			 *
			 * @details Метод выполняет преобразование разрядности и типа числа.
			 *
			 * @param buffer буфер числа приёмника в порядке от младшего байта к старшему
			 * @param size   размер буфера числа приёмника в байтах
			 * @param type   тип числа приёмника
			 *
			 */
			void get(uint8_t * buffer, const uint16_t size, const type_t type) const noexcept;
		public:
			/**
			 * @brief Метод сравнения двух чисел
			 *
			 * @param num число для сравнения
			 * @return    результат сравнения (-1, 0, 1 или 2 для несравнимых значений)
			 *
			 */
			int8_t compare(const BigNum & num) const noexcept;
		public:
			/**
			 * @brief Метод извлечения модуля числа
			 *
			 * @return модуль текущего числа
			 *
			 */
			BigNum abs() const noexcept;
			/**
			 * @brief Метод извлечения квадратного корня числа
			 *
			 * @details Для целочисленных типов метод выполняет извлечение целочисленного корня,
			 *          при этом корень извлекается из модуля числа. Для вещественных типов метод
			 *          следует стандарту IEEE-754: корень из отрицательного значения не является
			 *          числом, а знак нуля сохраняется.
			 *
			 * @return квадратный корень текущего числа
			 *
			 */
			BigNum sqrt() const noexcept;
			/**
			 * @brief Метод возведения числа в степень
			 *
			 * @param exponent показатель степени
			 * @return         результат возведения в степень
			 *
			 */
			BigNum pow(const uint64_t exponent) const noexcept;
		public:
			/**
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
			 */
			BigNum round(const int32_t digits = 0, const round_t mode = round_t::NEAREST) const noexcept;
			/**
			 * @brief Метод отбрасывания знаков числа после указанной позиции
			 *
			 * @param digits количество сохраняемых знаков после запятой
			 * @return        значение числа с отброшенными младшими знаками
			 *
			 */
			BigNum trunc(const int32_t digits = 0) const noexcept;
			/**
			 * @brief Метод округления числа в сторону минус бесконечности
			 *
			 * @param digits количество сохраняемых знаков после запятой
			 * @return        округлённое значение числа
			 *
			 */
			BigNum floor(const int32_t digits = 0) const noexcept;
			/**
			 * @brief Метод округления числа в сторону плюс бесконечности
			 *
			 * @param digits количество сохраняемых знаков после запятой
			 * @return        округлённое значение числа
			 *
			 */
			BigNum ceil(const int32_t digits = 0) const noexcept;
		public:
			/**
			 * @brief Метод формирования строкового представления числа
			 *
			 * @param format    формат представления числа
			 * @param precision количество знаков после запятой (отрицательное значение - автоматически)
			 * @return          сформированная строка числа
			 *
			 */
			string print(const format_t format = format_t::DEC, const int16_t precision = -1) const noexcept;
			/**
			 * @brief Метод разбора строкового представления числа
			 *
			 * @param text   разбираемая строка числа
			 * @param format формат представления числа
			 * @return       результат выполнения разбора
			 *
			 */
			bool parse(string_view text, const format_t format = format_t::NONE) noexcept;
		public:
			/**
			 * @brief Оператор вывода числа в качестве строки
			 *
			 * @return число в качестве строки
			 *
			 */
			operator string() const noexcept;
		public:
			/**
			 * @brief Оператор приведения к логическому типу
			 *
			 * @return результат приведения
			 *
			 */
			explicit operator bool() const noexcept;
			/**
			 * @brief Оператор приведения к символьному типу
			 *
			 * @return результат приведения
			 *
			 */
			explicit operator char() const noexcept;
			/**
			 * @brief Оператор приведения к знаковому символьному типу
			 *
			 * @return результат приведения
			 *
			 */
			explicit operator signed char() const noexcept;
			/**
			 * @brief Оператор приведения к беззнаковому символьному типу
			 *
			 * @return результат приведения
			 *
			 */
			explicit operator unsigned char() const noexcept;
			/**
			 * @brief Оператор приведения к короткому целому типу
			 *
			 * @return результат приведения
			 *
			 */
			explicit operator short() const noexcept;
			/**
			 * @brief Оператор приведения к короткому беззнаковому целому типу
			 *
			 * @return результат приведения
			 *
			 */
			explicit operator unsigned short() const noexcept;
			/**
			 * @brief Оператор приведения к целому типу
			 *
			 * @return результат приведения
			 *
			 */
			explicit operator int() const noexcept;
			/**
			 * @brief Оператор приведения к беззнаковому целому типу
			 *
			 * @return результат приведения
			 *
			 */
			explicit operator unsigned int() const noexcept;
			/**
			 * @brief Оператор приведения к длинному целому типу
			 *
			 * @return результат приведения
			 *
			 */
			explicit operator long() const noexcept;
			/**
			 * @brief Оператор приведения к длинному беззнаковому целому типу
			 *
			 * @return результат приведения
			 *
			 */
			explicit operator unsigned long() const noexcept;
			/**
			 * @brief Оператор приведения к сверхдлинному целому типу
			 *
			 * @return результат приведения
			 *
			 */
			explicit operator long long() const noexcept;
			/**
			 * @brief Оператор приведения к сверхдлинному беззнаковому целому типу
			 *
			 * @return результат приведения
			 *
			 */
			explicit operator unsigned long long() const noexcept;
			/**
			 * @brief Оператор приведения к вещественному типу одинарной точности
			 *
			 * @return результат приведения
			 *
			 */
			explicit operator float() const noexcept;
			/**
			 * @brief Оператор приведения к вещественному типу двойной точности
			 *
			 * @return результат приведения
			 *
			 */
			explicit operator double() const noexcept;
			/**
			 * @brief Оператор приведения к вещественному типу расширенной точности
			 *
			 * @return результат приведения
			 *
			 */
			explicit operator long double() const noexcept;
		public:
			/**
			 * @brief Оператор извлечения байта числа
			 *
			 * @param index индекс извлекаемого байта
			 * @return      значение указанного байта
			 *
			 */
			uint8_t operator [] (const uint16_t index) const noexcept;
		public:
			/**
			 * @brief Оператор проверки числа на нулевое значение
			 *
			 * @return результат проверки
			 *
			 */
			bool operator ! () const noexcept;
			/**
			 * @brief Оператор унарного плюса
			 *
			 * @return копия текущего числа
			 *
			 */
			BigNum operator + () const noexcept;
			/**
			 * @brief Оператор унарного минуса
			 *
			 * @return число с противоположным знаком
			 *
			 */
			BigNum operator - () const noexcept;
			/**
			 * @brief Оператор побитовой инверсии числа
			 *
			 * @return побитово инвертированное число
			 *
			 */
			BigNum operator ~ () const noexcept;
		public:
			/**
			 * @brief Оператор префиксного инкремента
			 *
			 * @return текущий объект
			 *
			 */
			BigNum & operator ++ () noexcept;
			/**
			 * @brief Оператор постфиксного инкремента
			 *
			 * @return значение числа до инкремента
			 *
			 */
			BigNum operator ++ (int) noexcept;
			/**
			 * @brief Оператор префиксного декремента
			 *
			 * @return текущий объект
			 *
			 */
			BigNum & operator -- () noexcept;
			/**
			 * @brief Оператор постфиксного декремента
			 *
			 * @return значение числа до декремента
			 *
			 */
			BigNum operator -- (int) noexcept;
		public:
			/**
			 * @brief Оператор сложения с присвоением
			 *
			 * @param num число для сложения
			 * @return    текущий объект
			 *
			 */
			BigNum & operator += (const BigNum & num) noexcept;
			/**
			 * @brief Оператор вычитания с присвоением
			 *
			 * @param num число для вычитания
			 * @return    текущий объект
			 *
			 */
			BigNum & operator -= (const BigNum & num) noexcept;
			/**
			 * @brief Оператор умножения с присвоением
			 *
			 * @param num число для умножения
			 * @return    текущий объект
			 *
			 */
			BigNum & operator *= (const BigNum & num) noexcept;
			/**
			 * @brief Оператор деления с присвоением
			 *
			 * @param num число для деления
			 * @return    текущий объект
			 *
			 */
			BigNum & operator /= (const BigNum & num) noexcept;
			/**
			 * @brief Оператор извлечения остатка от деления с присвоением
			 *
			 * @param num число для деления
			 * @return    текущий объект
			 *
			 */
			BigNum & operator %= (const BigNum & num) noexcept;
		public:
			/**
			 * @brief Оператор побитового умножения с присвоением
			 *
			 * @param num число для побитового умножения
			 * @return    текущий объект
			 *
			 */
			BigNum & operator &= (const BigNum & num) noexcept;
			/**
			 * @brief Оператор побитового сложения с присвоением
			 *
			 * @param num число для побитового сложения
			 * @return    текущий объект
			 *
			 */
			BigNum & operator |= (const BigNum & num) noexcept;
			/**
			 * @brief Оператор побитового исключающего сложения с присвоением
			 *
			 * @param num число для побитового исключающего сложения
			 * @return    текущий объект
			 *
			 */
			BigNum & operator ^= (const BigNum & num) noexcept;
			/**
			 * @brief Оператор сдвига влево с присвоением
			 *
			 * @param count количество бит сдвига
			 * @return      текущий объект
			 *
			 */
			BigNum & operator <<= (const uint32_t count) noexcept;
			/**
			 * @brief Оператор сдвига вправо с присвоением
			 *
			 * @param count количество бит сдвига
			 * @return      текущий объект
			 *
			 */
			BigNum & operator >>= (const uint32_t count) noexcept;
		public:
			/**
			 * @brief Оператор присваивания копированием
			 *
			 * @param num число для присвоения
			 * @return    текущий объект
			 *
			 */
			BigNum & operator = (const BigNum & num) noexcept;
			/**
			 * @brief Оператор присваивания перемещением
			 *
			 * @param num число для присвоения
			 * @return    текущий объект
			 *
			 */
			BigNum & operator = (BigNum && num) noexcept;
		public:
			/**
			 * @brief Оператор присваивания строкового значения
			 *
			 * @param text строка числа для присвоения
			 * @return     текущий объект
			 *
			 */
			BigNum & operator = (const char * text) noexcept;
			/**
			 * @brief Оператор присваивания строкового значения
			 *
			 * @param text строка числа для присвоения
			 * @return     текущий объект
			 *
			 */
			BigNum & operator = (const string & text) noexcept;
			/**
			 * @brief Оператор присваивания строкового значения
			 *
			 * @param text строка числа для присвоения
			 * @return     текущий объект
			 *
			 */
			BigNum & operator = (string_view text) noexcept;
		public:
			/**
			 * @brief Оператор присваивания логического значения
			 *
			 * @param num значение для присвоения
			 * @return    текущий объект
			 *
			 */
			BigNum & operator = (const bool num) noexcept;
			/**
			 * @brief Оператор присваивания символьного значения
			 *
			 * @param num значение для присвоения
			 * @return    текущий объект
			 *
			 */
			BigNum & operator = (const char num) noexcept;
			/**
			 * @brief Оператор присваивания знакового символьного значения
			 *
			 * @param num значение для присвоения
			 * @return    текущий объект
			 *
			 */
			BigNum & operator = (const signed char num) noexcept;
			/**
			 * @brief Оператор присваивания беззнакового символьного значения
			 *
			 * @param num значение для присвоения
			 * @return    текущий объект
			 *
			 */
			BigNum & operator = (const unsigned char num) noexcept;
			/**
			 * @brief Оператор присваивания короткого целого значения
			 *
			 * @param num значение для присвоения
			 * @return    текущий объект
			 *
			 */
			BigNum & operator = (const short num) noexcept;
			/**
			 * @brief Оператор присваивания короткого беззнакового целого значения
			 *
			 * @param num значение для присвоения
			 * @return    текущий объект
			 *
			 */
			BigNum & operator = (const unsigned short num) noexcept;
			/**
			 * @brief Оператор присваивания целого значения
			 *
			 * @param num значение для присвоения
			 * @return    текущий объект
			 *
			 */
			BigNum & operator = (const int num) noexcept;
			/**
			 * @brief Оператор присваивания беззнакового целого значения
			 *
			 * @param num значение для присвоения
			 * @return    текущий объект
			 *
			 */
			BigNum & operator = (const unsigned int num) noexcept;
			/**
			 * @brief Оператор присваивания длинного целого значения
			 *
			 * @param num значение для присвоения
			 * @return    текущий объект
			 *
			 */
			BigNum & operator = (const long num) noexcept;
			/**
			 * @brief Оператор присваивания длинного беззнакового целого значения
			 *
			 * @param num значение для присвоения
			 * @return    текущий объект
			 *
			 */
			BigNum & operator = (const unsigned long num) noexcept;
			/**
			 * @brief Оператор присваивания сверхдлинного целого значения
			 *
			 * @param num значение для присвоения
			 * @return    текущий объект
			 *
			 */
			BigNum & operator = (const long long num) noexcept;
			/**
			 * @brief Оператор присваивания сверхдлинного беззнакового целого значения
			 *
			 * @param num значение для присвоения
			 * @return    текущий объект
			 *
			 */
			BigNum & operator = (const unsigned long long num) noexcept;
			/**
			 * @brief Оператор присваивания вещественного значения одинарной точности
			 *
			 * @param num значение для присвоения
			 * @return    текущий объект
			 *
			 */
			BigNum & operator = (const float num) noexcept;
			/**
			 * @brief Оператор присваивания вещественного значения двойной точности
			 *
			 * @param num значение для присвоения
			 * @return    текущий объект
			 *
			 */
			BigNum & operator = (const double num) noexcept;
			/**
			 * @brief Оператор присваивания вещественного значения расширенной точности
			 *
			 * @param num значение для присвоения
			 * @return    текущий объект
			 *
			 */
			BigNum & operator = (const long double num) noexcept;
		public:
			/**
			 * @brief Шаблон разрядности и типа присваиваемого длинного числа
			 *
			 * @tparam SIZE размер присваиваемого числа в байтах
			 * @tparam KIND тип присваиваемого числа
			 *
			 */
			template <uint16_t SIZE, bignum::type_t KIND>
			/**
			 * @brief Оператор присваивания длинного числа иной разрядности
			 *
			 * @param num число для присвоения
			 * @return    текущий объект
			 *
			 */
			BigNum & operator = (const BigNum <SIZE, KIND> & num) noexcept {
				// Выполняем установку значения числа иной разрядности
				this->set(num.data(), SIZE, KIND);
				// Выводим текущий объект
				return (* this);
			}
		public:
			/**
			 * @brief Оператор [+] сложения двух чисел
			 *
			 * @param num1 первое слагаемое
			 * @param num2 второе слагаемое
			 * @return     результат сложения
			 *
			 */
			friend BigNum operator + (BigNum num1, const BigNum & num2) noexcept {
				// Выполняем сложение двух чисел
				return (num1 += num2);
			}
			/**
			 * @brief Оператор [-] вычитания двух чисел
			 *
			 * @param num1 уменьшаемое число
			 * @param num2 вычитаемое число
			 * @return     результат вычитания
			 *
			 */
			friend BigNum operator - (BigNum num1, const BigNum & num2) noexcept {
				// Выполняем вычитание двух чисел
				return (num1 -= num2);
			}
			/**
			 * @brief Оператор [*] умножения двух чисел
			 *
			 * @param num1 множимое число
			 * @param num2 множитель числа
			 * @return     результат умножения
			 *
			 */
			friend BigNum operator * (BigNum num1, const BigNum & num2) noexcept {
				// Выполняем умножение двух чисел
				return (num1 *= num2);
			}
			/**
			 * @brief Оператор [/] деления двух чисел
			 *
			 * @param num1 делимое число
			 * @param num2 делитель числа
			 * @return     результат деления
			 *
			 */
			friend BigNum operator / (BigNum num1, const BigNum & num2) noexcept {
				// Выполняем деление двух чисел
				return (num1 /= num2);
			}
			/**
			 * @brief Оператор [%] извлечения остатка от деления двух чисел
			 *
			 * @param num1 делимое число
			 * @param num2 делитель числа
			 * @return     остаток от деления
			 *
			 */
			friend BigNum operator % (BigNum num1, const BigNum & num2) noexcept {
				// Выполняем извлечение остатка от деления двух чисел
				return (num1 %= num2);
			}
		public:
			/**
			 * @brief Оператор [&] побитового умножения двух чисел
			 *
			 * @param num1 первый операнд
			 * @param num2 второй операнд
			 * @return     результат побитового умножения
			 *
			 */
			friend BigNum operator & (BigNum num1, const BigNum & num2) noexcept {
				// Выполняем побитовое умножение двух чисел
				return (num1 &= num2);
			}
			/**
			 * @brief Оператор [|] побитового сложения двух чисел
			 *
			 * @param num1 первый операнд
			 * @param num2 второй операнд
			 * @return     результат побитового сложения
			 *
			 */
			friend BigNum operator | (BigNum num1, const BigNum & num2) noexcept {
				// Выполняем побитовое сложение двух чисел
				return (num1 |= num2);
			}
			/**
			 * @brief Оператор [^] побитового исключающего сложения двух чисел
			 *
			 * @param num1 первый операнд
			 * @param num2 второй операнд
			 * @return     результат побитового исключающего сложения
			 *
			 */
			friend BigNum operator ^ (BigNum num1, const BigNum & num2) noexcept {
				// Выполняем побитовое исключающее сложение двух чисел
				return (num1 ^= num2);
			}
			/**
			 * @brief Оператор [<<] сдвига числа влево
			 *
			 * @param num   сдвигаемое число
			 * @param count количество бит сдвига
			 * @return      результат сдвига
			 *
			 */
			friend BigNum operator << (BigNum num, const uint32_t count) noexcept {
				// Выполняем сдвиг числа влево
				return (num <<= count);
			}
			/**
			 * @brief Оператор [>>] сдвига числа вправо
			 *
			 * @param num   сдвигаемое число
			 * @param count количество бит сдвига
			 * @return      результат сдвига
			 *
			 */
			friend BigNum operator >> (BigNum num, const uint32_t count) noexcept {
				// Выполняем сдвиг числа вправо
				return (num >>= count);
			}
		public:
			/**
			 * @brief Оператор [==] сравнения двух чисел
			 *
			 * @param num1 первое число для сравнения
			 * @param num2 второе число для сравнения
			 * @return     результат сравнения
			 *
			 */
			friend bool operator == (const BigNum & num1, const BigNum & num2) noexcept {
				// Выводим результат сравнения двух чисел
				return (num1.compare(num2) == 0);
			}
			/**
			 * @brief Оператор [!=] сравнения двух чисел
			 *
			 * @param num1 первое число для сравнения
			 * @param num2 второе число для сравнения
			 * @return     результат сравнения
			 *
			 */
			friend bool operator != (const BigNum & num1, const BigNum & num2) noexcept {
				// Выводим результат сравнения двух чисел
				return (num1.compare(num2) != 0);
			}
			/**
			 * @brief Оператор [<] сравнения двух чисел
			 *
			 * @param num1 первое число для сравнения
			 * @param num2 второе число для сравнения
			 * @return     результат сравнения
			 *
			 */
			friend bool operator < (const BigNum & num1, const BigNum & num2) noexcept {
				// Выводим результат сравнения двух чисел
				return (num1.compare(num2) == -1);
			}
			/**
			 * @brief Оператор [>] сравнения двух чисел
			 *
			 * @param num1 первое число для сравнения
			 * @param num2 второе число для сравнения
			 * @return     результат сравнения
			 *
			 */
			friend bool operator > (const BigNum & num1, const BigNum & num2) noexcept {
				// Выводим результат сравнения двух чисел
				return (num1.compare(num2) == 1);
			}
			/**
			 * @brief Оператор [<=] сравнения двух чисел
			 *
			 * @param num1 первое число для сравнения
			 * @param num2 второе число для сравнения
			 * @return     результат сравнения
			 *
			 */
			friend bool operator <= (const BigNum & num1, const BigNum & num2) noexcept {
				// Получаем результат сравнения двух чисел
				const int8_t result = num1.compare(num2);
				// Выводим результат сравнения двух чисел
				return ((result == -1) || (result == 0));
			}
			/**
			 * @brief Оператор [>=] сравнения двух чисел
			 *
			 * @param num1 первое число для сравнения
			 * @param num2 второе число для сравнения
			 * @return     результат сравнения
			 *
			 */
			friend bool operator >= (const BigNum & num1, const BigNum & num2) noexcept {
				// Получаем результат сравнения двух чисел
				const int8_t result = num1.compare(num2);
				// Выводим результат сравнения двух чисел
				return ((result == 1) || (result == 0));
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			BigNum() noexcept;
			/**
			 * @brief Конструктор копирования
			 *
			 * @param num число для копирования
			 *
			 */
			BigNum(const BigNum & num) noexcept;
			/**
			 * @brief Конструктор перемещения
			 *
			 * @param num число для перемещения
			 *
			 */
			BigNum(BigNum && num) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @details Конструктор объявлен явным, чтобы строка никогда не приводилась
			 *          к числу неявно и не участвовала в разрешении перегрузок операторов.
			 *
			 * @param text строка числа для установки
			 *
			 */
			explicit BigNum(const char * text) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param text строка числа для установки
			 *
			 */
			explicit BigNum(const string & text) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param text строка числа для установки
			 *
			 */
			explicit BigNum(string_view text) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param num значение для установки
			 *
			 */
			BigNum(const bool num) noexcept;
			/**
			 * @note Конструкторы встроенных числовых типов намеренно объявлены неявными.
			 *       Неявное преобразование встроенного числа в длинное необходимо для работы
			 *       арифметических операторов вида [5 + num] и [num * 2], а обратное
			 *       преобразование объявлено явным, что исключает неоднозначность разрешения
			 *       перегрузок между операторами длинных чисел и встроенными операторами.
			 *
			 */
			/**
			 * @brief Конструктор
			 *
			 * @param num значение для установки
			 *
			 */
			BigNum(const char num) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param num значение для установки
			 *
			 */
			BigNum(const signed char num) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param num значение для установки
			 *
			 */
			BigNum(const unsigned char num) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param num значение для установки
			 *
			 */
			BigNum(const short num) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param num значение для установки
			 *
			 */
			BigNum(const unsigned short num) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param num значение для установки
			 *
			 */
			BigNum(const int num) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param num значение для установки
			 *
			 */
			BigNum(const unsigned int num) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param num значение для установки
			 *
			 */
			BigNum(const long num) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param num значение для установки
			 *
			 */
			BigNum(const unsigned long num) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param num значение для установки
			 *
			 */
			BigNum(const long long num) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param num значение для установки
			 *
			 */
			BigNum(const unsigned long long num) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param num значение для установки
			 *
			 */
			BigNum(const float num) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param num значение для установки
			 *
			 */
			BigNum(const double num) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param num значение для установки
			 *
			 */
			BigNum(const long double num) noexcept;
		public:
			/**
			 * @brief Шаблон разрядности и типа исходного длинного числа
			 *
			 * @tparam SIZE размер исходного числа в байтах
			 * @tparam KIND тип исходного числа
			 *
			 */
			template <uint16_t SIZE, bignum::type_t KIND, typename enable_if <((KIND == TYPE) && (SIZE <= BYTES)), int32_t>::type = 0>
			/**
			 * @brief Конструктор расширения разрядности длинного числа
			 *
			 * @details Расширение разрядности числа того же типа выполняется без потери
			 *          значения, поэтому конструктор допускает неявное преобразование.
			 *
			 * @param num число для преобразования
			 *
			 */
			BigNum(const BigNum <SIZE, KIND> & num) noexcept : _data{} {
				// Выполняем установку значения числа иной разрядности
				this->set(num.data(), SIZE, KIND);
			}
			/**
			 * @brief Шаблон разрядности и типа исходного длинного числа
			 *
			 * @tparam SIZE размер исходного числа в байтах
			 * @tparam KIND тип исходного числа
			 *
			 */
			template <uint16_t SIZE, bignum::type_t KIND, typename enable_if <!((KIND == TYPE) && (SIZE <= BYTES)), int32_t>::type = 0>
			/**
			 * @brief Конструктор сужения разрядности либо смены типа длинного числа
			 *
			 * @details Преобразование способно привести к потере значения либо точности,
			 *          поэтому конструктор объявлен явным и требует явного приведения типа.
			 *
			 * @param num число для преобразования
			 *
			 */
			explicit BigNum(const BigNum <SIZE, KIND> & num) noexcept : _data{} {
				// Выполняем установку значения числа иной разрядности
				this->set(num.data(), SIZE, KIND);
			}
		public:
			/**
			 * @brief Деструктор
			 *
			 */
			~BigNum() noexcept;
	};
	/**
	 * @brief Шаблон разрядности и типа длинного числа
	 *
	 * @tparam BYTES размер числа в байтах
	 * @tparam TYPE  тип хранимого числа
	 *
	 */
	template <uint16_t BYTES, bignum::type_t TYPE>
	/**
	 * @brief Оператор [>>] чтения из потока длинного числа
	 *
	 * @param is  поток для чтения
	 * @param num число для присвоения
	 * @return    поток для чтения
	 *
	 */
	__AWH_SHARED_EXPORT__ istream & operator >> (istream & is, BigNum <BYTES, TYPE> & num) noexcept;
	/**
	 * @brief Шаблон разрядности и типа длинного числа
	 *
	 * @tparam BYTES размер числа в байтах
	 * @tparam TYPE  тип хранимого числа
	 *
	 */
	template <uint16_t BYTES, bignum::type_t TYPE>
	/**
	 * @brief Оператор [<<] вывода в поток длинного числа
	 *
	 * @param os  поток куда нужно вывести данные
	 * @param num число для вывода
	 * @return    поток для записи
	 *
	 */
	__AWH_SHARED_EXPORT__ ostream & operator << (ostream & os, const BigNum <BYTES, TYPE> & num) noexcept;
	/**
	 * @brief Шаблон разрядности знакового длинного числа
	 *
	 * @tparam BYTES размер числа в байтах
	 *
	 */
	template <uint16_t BYTES>
	/**
	 * @brief Создаём тип данных знакового длинного числа
	 *
	 */
	using bigint_t = BigNum <BYTES, bignum::type_t::SIGNED>;
	/**
	 * @brief Шаблон разрядности беззнакового длинного числа
	 *
	 * @tparam BYTES размер числа в байтах
	 *
	 */
	template <uint16_t BYTES>
	/**
	 * @brief Создаём тип данных беззнакового длинного числа
	 *
	 */
	using biguint_t = BigNum <BYTES, bignum::type_t::UNSIGNED>;
	/**
	 * @brief Шаблон разрядности вещественного длинного числа
	 *
	 * @tparam BYTES размер числа в байтах
	 *
	 */
	template <uint16_t BYTES>
	/**
	 * @brief Создаём тип данных вещественного длинного числа
	 *
	 */
	using bigreal_t = BigNum <BYTES, bignum::type_t::REAL>;
	/**
	 * @brief Создаём готовые типы данных знаковых длинных чисел
	 *
	 * @details Помимо стандартных разрядностей модуль предоставляет промежуточные
	 *          нестандартные разрядности, позволяющие подобрать минимально достаточный
	 *          размер числа под конкретную задачу и не расходовать память впустую.
	 *
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
	 * @brief Создаём готовые типы данных беззнаковых длинных чисел
	 *
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
	 * @brief Создаём готовые типы данных вещественных длинных чисел
	 *
	 * @details Типы намеренно названы real вместо float, поскольку начиная со стандарта
	 *          C++23 имена float16_t, float32_t, float64_t и float128_t объявлены в
	 *          пространстве имён std и привели бы к неоднозначности разрешения имён.
	 *
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
