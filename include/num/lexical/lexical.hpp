/**
 * @file: lexical.hpp
 * @date: 2026-07-22
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл модуля конвертации чисел из строкового представления — класс Lexical, выполняющий разбор
 *        целых чисел и чисел с плавающей точкой без выделения памяти и без исключений с побитовым совпадением
 *        результата с strtod в локали «C»
 *
 * \~english
 * @brief Header file of the module converting numbers from their string representation — the Lexical class, which parses
 *        integers and floating-point numbers without allocating memory and without exceptions, with a bit-exact match
 *        of the result with strtod in the «C» locale
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_LEXICAL__
#define __AWH_LEXICAL__

/**
 * Подключаем заголовочные файлы модуля
 */
#include "api.hpp"

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
	 * @brief Класс конвертации чисел из строкового представления
	 *
	 * @details Класс реализует разбор чисел с плавающей точкой и целых чисел
	 *          без выделения памяти и без выбрасывания исключений. Результат
	 *          разбора числа с плавающей точкой совпадает с результатом функции
	 *          strtod в локали «C» вплоть до последнего бита мантиссы.
	 *
	 * \~english
	 * @brief Class converting numbers from their string representation
	 * @details The class implements the parsing of floating-point numbers and of integers
	 *          without allocating memory and without throwing exceptions. The result
	 *          of parsing a floating-point number matches the result of the
	 *          strtod function in the «C» locale down to the last bit of the mantissa.
	 *
	 * \~
	 */
	typedef class Lexical {
		public:
			/**
			 * \~russian
			 * @brief Создаём тип данных кода причины отказа при разборе
			 *
			 * \~english
			 * @brief Create the data type of the failure reason code of the parsing
			 *
			 * \~
			 */
			using error_t = lexical::error_t;
			/**
			 * \~russian
			 * @brief Создаём тип данных формата разбора числовой строки
			 *
			 * \~english
			 * @brief Create the data type of the parsing format of a number string
			 *
			 * \~
			 */
			using format_t = lexical::format_t;
		public:
			/**
			 * \~russian
			 * @brief Шаблон типа символа исходной строки
			 *
			 * @tparam UC тип символа исходной строки
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
			 * @brief Создаём тип данных результата разбора числовой строки
			 *
			 * \~english
			 * @brief Create the data type of the result of parsing a number string
			 *
			 * \~
			 */
			using result_t = lexical::result_t <UC>;
			/**
			 * \~russian
			 * @brief Шаблон типа символа исходной строки
			 *
			 * @tparam UC тип символа исходной строки
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
			 * @brief Создаём тип данных опций разбора числовой строки
			 *
			 * \~english
			 * @brief Create the data type of the parsing options of a number string
			 *
			 * \~
			 */
			using options_t = lexical::options_t <UC>;
		public:
			/**
			 * \~russian
			 * @brief Шаблон типа результата и типа исходного целого
			 *
			 * @tparam T   тип числа с плавающей точкой
			 * @tparam INT тип исходного целого числа
			 *
			 * \~english
			 * @brief Template of the result type and of the source integer type
			 * @tparam T   floating-point type
			 * @tparam INT type of the source integer
			 *
			 * \~
			 */
			template <typename T = double, typename INT, lexical::enableIf_t <lexical::is_supported_float <T>::value && is_integral <INT>::value> = 0>
			/**
			 * \~russian
			 * @brief Метод умножения целого числа на степень десяти
			 *
			 * @details При переполнении результатом является бесконечность, при антипереполнении — нуль.
			 *
			 * @param mantissa значение мантиссы
			 * @param exponent десятичный показатель степени
			 * @return         результат умножения
			 *
			 * \~english
			 * @brief Method of multiplying an integer by a power of ten
			 * @details On overflow the result is infinity, on underflow zero.
			 * @param mantissa value of the mantissa
			 * @param exponent decimal exponent
			 * @return         result of the multiplication
			 *
			 * \~
			 */
			static T integerTimesPow10(const INT mantissa, const int32_t exponent) noexcept {
				// Выполняем умножение целого числа на степень десяти
				return lexical::integerTimesPow10 <T> (mantissa, exponent);
			}
		public:
			/**
			 * \~russian
			 * @brief Шаблон типа результата и типа символа исходной строки
			 *
			 * @tparam T  тип результата разбора
			 * @tparam UC тип символа исходной строки
			 *
			 * \~english
			 * @brief Template of the result type and of the character type of the source string
			 * @tparam T  result type of the parsing
			 * @tparam UC character type of the source string
			 *
			 * \~
			 */
			template <typename T, typename UC = char>
			/**
			 * \~russian
			 * @brief Метод разбора числа из строки с расширенными опциями
			 *
			 * @details Метод поддерживает как числа с плавающей точкой, так и целые.
			 *
			 * @param first   начало разбираемой строки
			 * @param last    конец разбираемой строки
			 * @param value   ссылка на результат разбора
			 * @param options опции разбора числовой строки
			 * @return        результат разбора числовой строки
			 *
			 * \~english
			 * @brief Method of parsing a number from a string with extended options
			 * @details The method supports both floating-point numbers and integers.
			 * @param first   beginning of the parsed string
			 * @param last    end of the parsed string
			 * @param value   reference to the result of the parsing
			 * @param options parsing options of the number string
			 * @return        result of parsing the number string
			 *
			 * \~
			 */
			static lexical::result_t <UC> fromCharsAdvanced(const UC * first, const UC * last, T & value, const lexical::options_t <UC> options) noexcept {
				// Выполняем разбор числа с расширенными опциями
				return lexical::fromCharsAdvanced(first, last, value, options);
			}
		public:
			/**
			 * \~russian
			 * @brief Шаблон типа результата и типа символа исходной строки
			 *
			 * @tparam T  тип целого числа
			 * @tparam UC тип символа исходной строки
			 *
			 * \~english
			 * @brief Template of the result type and of the character type of the source string
			 * @tparam T  integer type
			 * @tparam UC character type of the source string
			 *
			 * \~
			 */
			template <typename T, typename UC = char, lexical::enableIf_t <lexical::is_supported_integer <T>::value> = 0>
			/**
			 * \~russian
			 * @brief Метод разбора целого числа из строки
			 *
			 * @param first начало разбираемой строки
			 * @param last  конец разбираемой строки
			 * @param value ссылка на результат разбора
			 * @param base  основание системы счисления в диапазоне от 2 до 36
			 * @return      результат разбора числовой строки
			 *
			 * \~english
			 * @brief Method of parsing an integer from a string
			 * @param first beginning of the parsed string
			 * @param last  end of the parsed string
			 * @param value reference to the result of the parsing
			 * @param base  base of the numeral system in the range from 2 to 36
			 * @return      result of parsing the number string
			 *
			 * \~
			 */
			static lexical::result_t <UC> fromChars(const UC * first, const UC * last, T & value, const int32_t base = 10) noexcept {
				// Выполняем разбор целого числа
				return lexical::fromChars(first, last, value, base);
			}
			/**
			 * \~russian
			 * @brief Шаблон типа результата и типа символа исходной строки
			 *
			 * @tparam T  тип числа с плавающей точкой
			 * @tparam UC тип символа исходной строки
			 *
			 * \~english
			 * @brief Template of the result type and of the character type of the source string
			 * @tparam T  floating-point type
			 * @tparam UC character type of the source string
			 *
			 * \~
			 */
			template <typename T, typename UC = char, lexical::enableIf_t <lexical::is_supported_float <T>::value> = 0>
			/**
			 * \~russian
			 * @brief Метод разбора числа с плавающей точкой из строки
			 *
			 * @param first  начало разбираемой строки
			 * @param last   конец разбираемой строки
			 * @param value  ссылка на результат разбора
			 * @param format допустимый формат записи числа
			 * @return       результат разбора числовой строки
			 *
			 * \~english
			 * @brief Method of parsing a floating-point number from a string
			 * @param first  beginning of the parsed string
			 * @param last   end of the parsed string
			 * @param value  reference to the result of the parsing
			 * @param format admissible format of the number record
			 * @return       result of parsing the number string
			 *
			 * \~
			 */
			static lexical::result_t <UC> fromChars(const UC * first, const UC * last, T & value, const lexical::format_t format = lexical::format_t::GENERAL) noexcept {
				// Выполняем разбор числа с плавающей точкой
				return lexical::fromChars(first, last, value, format);
			}
	} lexical_t;
};

#endif // __AWH_LEXICAL__
