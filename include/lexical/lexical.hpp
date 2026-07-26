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
 * @copyright: Copyright © 2026
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
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Класс конвертации чисел из строкового представления
	 *
	 * @details Класс реализует разбор чисел с плавающей точкой и целых чисел
	 *          без выделения памяти и без выбрасывания исключений. Результат
	 *          разбора числа с плавающей точкой совпадает с результатом функции
	 *          strtod в локали «C» вплоть до последнего бита мантиссы.
	 */
	typedef class Lexical {
		public:
			/**
			 * @brief Создаём тип данных кода причины отказа при разборе
			 *
			 */
			using error_t = lexical::error_t;
			/**
			 * @brief Создаём тип данных формата разбора числовой строки
			 *
			 */
			using format_t = lexical::format_t;
		public:
			/**
			 * @brief Шаблон типа символа исходной строки
			 *
			 * @tparam UC тип символа исходной строки
			 */
			template <typename UC>
			/**
			 * @brief Создаём тип данных результата разбора числовой строки
			 *
			 */
			using result_t = lexical::result_t <UC>;
			/**
			 * @brief Шаблон типа символа исходной строки
			 *
			 * @tparam UC тип символа исходной строки
			 */
			template <typename UC>
			/**
			 * @brief Создаём тип данных опций разбора числовой строки
			 *
			 */
			using options_t = lexical::options_t <UC>;
		public:
			/**
			 * @brief Шаблон типа результата и типа исходного целого
			 *
			 * @tparam T   тип числа с плавающей точкой
			 * @tparam INT тип исходного целого числа
			 */
			template <typename T = double, typename INT, lexical::enableIf_t <lexical::is_supported_float <T>::value && is_integral <INT>::value> = 0>
			/**
			 * @brief Метод умножения целого числа на степень десяти
			 *
			 * @details При переполнении результатом является бесконечность, при антипереполнении — нуль.
			 *
			 * @param mantissa значение мантиссы
			 * @param exponent десятичный показатель степени
			 * @return         результат умножения
			 */
			static T integerTimesPow10(const INT mantissa, const int32_t exponent) noexcept {
				// Выполняем умножение целого числа на степень десяти
				return lexical::integerTimesPow10 <T> (mantissa, exponent);
			}
		public:
			/**
			 * @brief Шаблон типа результата и типа символа исходной строки
			 *
			 * @tparam T  тип результата разбора
			 * @tparam UC тип символа исходной строки
			 */
			template <typename T, typename UC = char>
			/**
			 * @brief Метод разбора числа из строки с расширенными опциями
			 *
			 * @details Метод поддерживает как числа с плавающей точкой, так и целые.
			 *
			 * @param first   начало разбираемой строки
			 * @param last    конец разбираемой строки
			 * @param value   ссылка на результат разбора
			 * @param options опции разбора числовой строки
			 * @return        результат разбора числовой строки
			 */
			static lexical::result_t <UC> fromCharsAdvanced(const UC * first, const UC * last, T & value, const lexical::options_t <UC> options) noexcept {
				// Выполняем разбор числа с расширенными опциями
				return lexical::fromCharsAdvanced(first, last, value, options);
			}
		public:
			/**
			 * @brief Шаблон типа результата и типа символа исходной строки
			 *
			 * @tparam T  тип целого числа
			 * @tparam UC тип символа исходной строки
			 */
			template <typename T, typename UC = char, lexical::enableIf_t <lexical::is_supported_integer <T>::value> = 0>
			/**
			 * @brief Метод разбора целого числа из строки
			 *
			 * @param first начало разбираемой строки
			 * @param last  конец разбираемой строки
			 * @param value ссылка на результат разбора
			 * @param base  основание системы счисления в диапазоне от 2 до 36
			 * @return      результат разбора числовой строки
			 */
			static lexical::result_t <UC> fromChars(const UC * first, const UC * last, T & value, const int32_t base = 10) noexcept {
				// Выполняем разбор целого числа
				return lexical::fromChars(first, last, value, base);
			}
			/**
			 * @brief Шаблон типа результата и типа символа исходной строки
			 *
			 * @tparam T  тип числа с плавающей точкой
			 * @tparam UC тип символа исходной строки
			 */
			template <typename T, typename UC = char, lexical::enableIf_t <lexical::is_supported_float <T>::value> = 0>
			/**
			 * @brief Метод разбора числа с плавающей точкой из строки
			 *
			 * @param first  начало разбираемой строки
			 * @param last   конец разбираемой строки
			 * @param value  ссылка на результат разбора
			 * @param format допустимый формат записи числа
			 * @return       результат разбора числовой строки
			 */
			static lexical::result_t <UC> fromChars(const UC * first, const UC * last, T & value, const lexical::format_t format = lexical::format_t::GENERAL) noexcept {
				// Выполняем разбор числа с плавающей точкой
				return lexical::fromChars(first, last, value, format);
			}
	} lexical_t;
};

#endif // __AWH_LEXICAL__
