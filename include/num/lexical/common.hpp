/**
 * @file common.hpp
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
 * @brief Заголовочный файл общих определений модуля разбора чисел — типажи поддерживаемых типов, коды ошибок,
 *        флаги формата разбора, структуры результата и опций, 128-битная арифметика,
 *        а также таблицы двоичных констант для float, double, float16 и bfloat16
 *
 * \~english
 * @brief Header file of the common definitions of the number parsing module — the traits of the supported types, the error codes,
 *        the parsing format flags, the structures of the result and of the options, the 128-bit arithmetic,
 *        as well as the tables of the binary constants for float, double, float16 and bfloat16
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_LEXICAL_COMMON__
#define __AWH_LEXICAL_COMMON__

/**
 * Стандартные заголовочные файлы
 */
#include <limits>
#include <cfloat>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <system_error>

/**
 * Подключаем расширенные типы чисел с плавающей точкой стандарта C++23
 */
#ifdef __has_include
	/**
	 * Проверяем наличие заголовочного файла <stdfloat>
	 */
	#if __has_include(<stdfloat>) && ((__cplusplus > 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG > 202002L)))
		// Подключаем заголовочный файл <stdfloat>
		#include <stdfloat>
	#endif
#endif

/**
 * Подключаем заголовочные файлы модуля
 */
#include "detect.hpp"

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
		 * @brief Шаблон помощника SFINAE для шаблонных перегрузок
		 *
		 * @tparam Condition условие подстановки
		 *
		 * \~english
		 * @brief Template of the SFINAE helper for the template overloads
		 * @tparam Condition substitution condition
		 *
		 * \~
		 */
		template <bool Condition>
		/**
		 * \~russian
		 * @brief Тип результата подстановки SFINAE для шаблонных перегрузок
		 *
		 * \~english
		 * @brief Result type of the SFINAE substitution for the template overloads
		 *
		 * \~
		 */
		using enableIf_t = typename enable_if <Condition, int>::type;

		/**
		 * \~russian
		 * @brief Шаблон эквивалентного беззнакового целого типа той же ширины
		 *
		 * @tparam T исходный тип
		 *
		 * \~english
		 * @brief Template of the equivalent unsigned integer type of the same width
		 * @tparam T source type
		 *
		 * \~
		 */
		template <typename T>
		/**
		 * \~russian
		 * @brief Тип результата эквивалентного беззнакового целого типа той же ширины
		 *
		 * \~english
		 * @brief Result type of the equivalent unsigned integer type of the same width
		 *
		 * \~
		 */
		using equivUint_t = typename conditional <
			sizeof(T) == 1, uint8_t,
			typename conditional <
				sizeof(T) == 2, uint16_t,
				typename conditional <sizeof(T) == 4, uint32_t, uint64_t>::type
			>::type
		>::type;

		/**
		 * \~russian
		 * @brief Шаблон признака поддерживаемого типа с плавающей точкой
		 *
		 * @tparam T проверяемый тип
		 *
		 *
		 * \~english
		 * @brief Template of the indication of a supported floating-point type
		 * @tparam T checked type
		 *
		 * \~
		 */
		template <typename T>
		/**
		 * \~russian
		 * @brief Структура признака поддерживаемого типа с плавающей точкой
		 *
		 * \~english
		 * @brief Structure of the indication of a supported floating-point type
		 *
		 * \~
		 */
		struct is_supported_float : integral_constant <bool,
			is_same <T, float>::value ||
			is_same <T, double>::value
			/**
			 * Если поддерживаются 32-битные типы чисел с плавающей точкой стандарта C++23
			 */
			#ifdef __STDCPP_FLOAT32_T__
				|| is_same <T, float32_t>::value
			#endif
			/**
			 * Если поддерживаются 64-битные типы чисел с плавающей точкой стандарта C++23
			 */
			#ifdef __STDCPP_FLOAT64_T__
				|| is_same <T, float64_t>::value
			#endif
			/**
			 * Если поддерживается расширенный тип с плавающей запятой binary16 (16-битный формат)
			 */
			#ifdef __STDCPP_FLOAT16_T__
				|| is_same <T, float16_t>::value
			#endif
			/**
			 * Если компилятор поддерживает тип std::bfloat16_t стандарта C++23
			 */
			#ifdef __STDCPP_BFLOAT16_T__
				|| is_same <T, bfloat16_t>::value
			#endif
		> {};

		/**
		 * \~russian
		 * @brief Шаблон признака поддерживаемого целочисленного типа
		 *
		 * @tparam T проверяемый тип
		 *
		 *
		 * \~english
		 * @brief Template of the indication of a supported integer type
		 * @tparam T checked type
		 *
		 * \~
		 */
		template <typename T>
		/**
		 * \~russian
		 * @brief Структура признака поддерживаемого целочисленного типа
		 *
		 * \~english
		 * @brief Structure of the indication of a supported integer type
		 *
		 * \~
		 */
		struct is_supported_integer : is_integral <T> {};

		/**
		 * \~russian
		 * @brief Шаблон признака поддерживаемого символьного типа
		 *
		 * @tparam UC проверяемый тип символа
		 *
		 *
		 * \~english
		 * @brief Template of the indication of a supported character type
		 * @tparam UC checked character type
		 *
		 * \~
		 */
		template <typename UC>
		/**
		 * \~russian
		 * @brief Структура признака поддерживаемого символьного типа
		 *
		 * \~english
		 * @brief Structure of the indication of a supported character type
		 *
		 * \~
		 */
		struct is_supported_char : integral_constant <bool,
			is_same <UC, char>::value ||
			is_same <UC, wchar_t>::value ||
			is_same <UC, char16_t>::value ||
			is_same <UC, char32_t>::value
			#ifdef __cpp_char8_t
				|| is_same <UC, char8_t>::value
			#endif
		> {};

		/**
		 * \~russian
		 * @brief Код причины отказа при разборе числовой строки
		 *
		 * \~english
		 * @brief Failure reason code of the parsing of a number string
		 *
		 * \~
		 */
		enum class error_t : uint8_t {
			NONE                              = 0x00, // Ошибки нет
			MISSING_INTEGER_AFTER_SIGN        = 0x01, // После знака отсутствует цифра
			MISSING_INTEGER_OR_DOT_AFTER_SIGN = 0x02, // После знака отсутствует цифра или десятичная точка
			LEADING_ZEROS_IN_INTEGER_PART     = 0x03, // Ведущие нули в целой части запрещены форматом
			NO_DIGITS_IN_INTEGER_PART         = 0x04, // В целой части отсутствуют цифры
			NO_DIGITS_IN_FRACTIONAL_PART      = 0x05, // В дробной части отсутствуют цифры
			NO_DIGITS_IN_MANTISSA             = 0x06, // В мантиссе отсутствуют цифры
			MISSING_EXPONENTIAL_PART          = 0x07, // Отсутствует экспоненциальная часть
			INVALID_BASE                      = 0x08, // Недопустимое основание системы счисления
			EMPTY_INPUT                       = 0x09, // Передан пустой диапазон символов
			OVERFLOW_RANGE                    = 0x0A, // Значение не помещается в целевой тип
			INSUFFICIENT_BUFFER               = 0x0B, // Отведённого под запись места недостаточно
			INVALID_PRECISION                 = 0x0C  // Недопустимое количество знаков записи
		};

		/**
		 * \~russian
		 * @brief Формат разбора числовой строки
		 *
		 * @details Значения являются битовыми флагами и комбинируются оператором «|».
		 *
		 * \~english
		 * @brief Parsing format of a number string
		 * @details The values are bit flags and are combined by the «|» operator.
		 *
		 * \~
		 */
		enum class format_t : uint64_t {
			NONE               = 0x00,                                          // Формат не установлен
			SCIENTIFIC         = 0x01,                                          // Разрешена научная запись вида 1.5e3
			FIXED              = 0x04,                                          // Разрешена запись с фиксированной точкой
			HEX                = 0x08,                                          // Разрешена шестнадцатеричная запись
			NO_INFNAN          = 0x10,                                          // Запрещены значения inf и nan
			BASIC_JSON         = 0x20,                                          // Включены строгие правила RFC 8259
			BASIC_FORTRAN      = 0x40,                                          // Включены правила экспоненты языка Fortran
			ALLOW_LEADING_PLUS = 0x80,                                          // Разрешён ведущий знак «+»
			SKIP_WHITE_SPACE   = 0x100,                                         // Разрешён пропуск ведущих пробельных символов
			GENERAL            = (FIXED | SCIENTIFIC),                          // Общий формат записи числа
			JSON               = (BASIC_JSON | FIXED | SCIENTIFIC | NO_INFNAN), // Формат чисел RFC 8259
			JSON_OR_INFNAN     = (BASIC_JSON | FIXED | SCIENTIFIC),             // Формат чисел RFC 8259 с разрешёнными inf и nan
			FORTRAN            = (BASIC_FORTRAN | FIXED | SCIENTIFIC)           // Формат чисел языка Fortran
		};

		/**
		 * \~russian
		 * @brief Оператор побитового отрицания формата разбора
		 *
		 * @param rhs исходный формат разбора
		 * @return    результат побитового отрицания
		 *
		 * \~english
		 * @brief Bitwise negation operator of the parsing format
		 * @param rhs source parsing format
		 * @return    result of the bitwise negation
		 *
		 * \~
		 */
		constexpr format_t operator ~ (const format_t rhs) noexcept {
			// Выполняем побитовое отрицание базового типа
			return static_cast <format_t> (~static_cast <underlying_type <format_t>::type> (rhs));
		}
		/**
		 * \~russian
		 * @brief Оператор побитового «И» формата разбора
		 *
		 * @param lhs первый формат разбора
		 * @param rhs второй формат разбора
		 * @return    результат побитового «И»
		 *
		 * \~english
		 * @brief Bitwise «AND» operator of the parsing format
		 * @param lhs first parsing format
		 * @param rhs second parsing format
		 * @return    result of the bitwise «AND»
		 *
		 * \~
		 */
		constexpr format_t operator & (const format_t lhs, const format_t rhs) noexcept {
			// Выполняем побитовое «И» базовых типов
			return static_cast <format_t> (
				static_cast <underlying_type <format_t>::type> (lhs) &
				static_cast <underlying_type <format_t>::type> (rhs)
			);
		}
		/**
		 * \~russian
		 * @brief Оператор побитового «ИЛИ» формата разбора
		 *
		 * @param lhs первый формат разбора
		 * @param rhs второй формат разбора
		 * @return    результат побитового «ИЛИ»
		 *
		 * \~english
		 * @brief Bitwise «OR» operator of the parsing format
		 * @param lhs first parsing format
		 * @param rhs second parsing format
		 * @return    result of the bitwise «OR»
		 *
		 * \~
		 */
		constexpr format_t operator | (const format_t lhs, const format_t rhs) noexcept {
			// Выполняем побитовое «ИЛИ» базовых типов
			return static_cast <format_t> (
				static_cast <underlying_type <format_t>::type> (lhs) |
				static_cast <underlying_type <format_t>::type> (rhs)
			);
		}
		/**
		 * \~russian
		 * @brief Оператор побитового исключающего «ИЛИ» формата разбора
		 *
		 * @param lhs первый формат разбора
		 * @param rhs второй формат разбора
		 * @return    результат побитового исключающего «ИЛИ»
		 *
		 * \~english
		 * @brief Bitwise exclusive «OR» operator of the parsing format
		 * @param lhs first parsing format
		 * @param rhs second parsing format
		 * @return    result of the bitwise exclusive «OR»
		 *
		 * \~
		 */
		constexpr format_t operator ^ (const format_t lhs, const format_t rhs) noexcept {
			// Выполняем побитовое исключающее «ИЛИ» базовых типов
			return static_cast <format_t> (
				static_cast <underlying_type <format_t>::type> (lhs) ^
				static_cast <underlying_type <format_t>::type> (rhs)
			);
		}
		/**
		 * \~russian
		 * @brief Оператор присваивающего побитового «И» формата разбора
		 *
		 * @param lhs изменяемый формат разбора
		 * @param rhs применяемый формат разбора
		 * @return    изменённый формат разбора
		 *
		 * \~english
		 * @brief Assigning bitwise «AND» operator of the parsing format
		 * @param lhs parsing format being changed
		 * @param rhs applied parsing format
		 * @return    the changed parsing format
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr format_t & operator &= (format_t & lhs, const format_t rhs) noexcept {
			// Выполняем применение операции к исходному формату
			return (lhs = (lhs & rhs));
		}
		/**
		 * \~russian
		 * @brief Оператор присваивающего побитового «ИЛИ» формата разбора
		 *
		 * @param lhs изменяемый формат разбора
		 * @param rhs применяемый формат разбора
		 * @return    изменённый формат разбора
		 *
		 * \~english
		 * @brief Assigning bitwise «OR» operator of the parsing format
		 * @param lhs parsing format being changed
		 * @param rhs applied parsing format
		 * @return    the changed parsing format
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr format_t & operator |= (format_t & lhs, const format_t rhs) noexcept {
			// Выполняем применение операции к исходному формату
			return (lhs = (lhs | rhs));
		}
		/**
		 * \~russian
		 * @brief Оператор присваивающего побитового исключающего «ИЛИ» формата разбора
		 *
		 * @param lhs изменяемый формат разбора
		 * @param rhs применяемый формат разбора
		 * @return    изменённый формат разбора
		 *
		 * \~english
		 * @brief Assigning bitwise exclusive «OR» operator of the parsing format
		 * @param lhs parsing format being changed
		 * @param rhs applied parsing format
		 * @return    the changed parsing format
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr format_t & operator ^= (format_t & lhs, const format_t rhs) noexcept {
			// Выполняем применение операции к исходному формату
			return (lhs = (lhs ^ rhs));
		}
		/**
		 * \~russian
		 * @brief Метод проверки установки флага формата разбора
		 *
		 * @param format проверяемый формат разбора
		 * @param flag   искомый флаг формата разбора
		 * @return       результат проверки
		 *
		 * \~english
		 * @brief Method of checking whether a parsing format flag is set
		 * @param format parsing format to check
		 * @param flag   sought flag of the parsing format
		 * @return       result of the check
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr bool isFormat(const format_t format, const format_t flag) noexcept {
			// Выполняем проверку наличия искомого флага
			return ((format & flag) != format_t::NONE);
		}

		/**
		 * \~russian
		 * @brief Шаблон типа символа результата разбора
		 *
		 * @tparam UC тип символа исходной строки
		 *
		 * \~english
		 * @brief Template of the character type of the parsing result
		 * @tparam UC character type of the source string
		 *
		 * \~
		 */
		template <typename UC>
		/**
		 * \~russian
		 * @brief Структура результата разбора числовой строки
		 *
		 * \~english
		 * @brief Structure of the result of parsing a number string
		 *
		 * \~
		 */
		struct result_t {
			// Код ошибки стандартной библиотеки
			errc ec;
			// Код причины отказа при разборе числовой строки
			error_t error;
			// Указатель на первый символ за разобранным числом
			const UC * ptr;
			/**
			 * \~russian
			 * @brief Оператор проверки успешности разбора
			 *
			 * @return результат проверки
			 *
			 * \~english
			 * @brief Operator of checking the success of the parsing
			 * @return result of the check
			 *
			 * \~
			 */
			constexpr explicit operator bool() const noexcept {
				// Разбор успешен, если код ошибки не установлен
				return (this->ec == errc());
			}
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param ptr   указатель на первый символ за разобранным числом
			 * @param ec    код ошибки стандартной библиотеки
			 * @param error код причины отказа при разборе числовой строки
			 *
			 * \~english
			 * @brief Constructor
			 * @param ptr   pointer to the first character past the parsed number
			 * @param ec    error code of the standard library
			 * @param error failure reason code of the parsing of the number string
			 *
			 * \~
			 */
			constexpr result_t(const UC * ptr = nullptr, const errc ec = errc(), const error_t error = error_t::NONE) noexcept :
			 ec(ec), error(error), ptr(ptr) {}
		};

		/**
		 * \~russian
		 * @brief Шаблон типа символа опций разбора
		 *
		 * @tparam UC тип символа исходной строки
		 *
		 * \~english
		 * @brief Template of the character type of the parsing options
		 * @tparam UC character type of the source string
		 *
		 * \~
		 */
		template <typename UC>
		/**
		 * \~russian
		 * @brief Структура опций разбора числовой строки
		 *
		 * \~english
		 * @brief Structure of the parsing options of a number string
		 *
		 * \~
		 */
		struct options_t {
			// Основание системы счисления для целых чисел
			int32_t base;
			// Допустимый формат записи числа
			format_t format;
			// Символ десятичной точки
			UC decimalPoint;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param format       допустимый формат записи числа
			 * @param decimalPoint символ десятичной точки
			 * @param base         основание системы счисления для целых чисел
			 *
			 * \~english
			 * @brief Constructor
			 * @param format       admissible format of the number record
			 * @param decimalPoint decimal point character
			 * @param base         base of the numeral system for integers
			 *
			 * \~
			 */
			constexpr explicit options_t(const format_t format = format_t::GENERAL, const UC decimalPoint = UC('.'), const int32_t base = 10) noexcept :
			 base(base), format(format), decimalPoint(decimalPoint) {}
		};

		/**
		 * \~russian
		 * @brief Шаблон типа элемента непрерывного диапазона
		 *
		 * @tparam T тип элемента диапазона
		 *
		 * \~english
		 * @brief Template of the element type of a contiguous range
		 * @tparam T element type of the range
		 *
		 * \~
		 */
		template <typename T>
		/**
		 * \~russian
		 * @brief Структура непрерывного диапазона памяти
		 *
		 * \~english
		 * @brief Structure of a contiguous range of memory
		 *
		 * \~
		 */
		struct span_t {
			// Количество элементов диапазона
			size_t length;
			// Указатель на начало диапазона
			const T * ptr;
			/**
			 * \~russian
			 * @brief Метод получения количества элементов диапазона
			 *
			 * @return количество элементов диапазона
			 *
			 * \~english
			 * @brief Method of getting the number of elements of the range
			 * @return number of elements of the range
			 *
			 * \~
			 */
			constexpr size_t len() const noexcept {
				// Выводим количество элементов диапазона
				return this->length;
			}
			/**
			 * \~russian
			 * @brief Оператор доступа к элементу диапазона
			 *
			 * @param index индекс запрашиваемого элемента
			 * @return      запрашиваемый элемент диапазона
			 *
			 * \~english
			 * @brief Operator of accessing an element of the range
			 * @param index index of the requested element
			 * @return      the requested element of the range
			 *
			 * \~
			 */
			constexpr const T & operator [] (const size_t index) const noexcept {
				// Выводим запрашиваемый элемент диапазона
				return this->ptr[index];
			}
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param ptr    указатель на начало диапазона
			 * @param length количество элементов диапазона
			 *
			 * \~english
			 * @brief Constructor
			 * @param ptr    pointer to the beginning of the range
			 * @param length number of elements of the range
			 *
			 * \~
			 */
			constexpr span_t(const T * ptr = nullptr, const size_t length = 0) noexcept :
			 length(length), ptr(ptr) {}
		};

		/**
		 * \~russian
		 * @brief Структура пары 64-битных слов результата умножения
		 *
		 * \~english
		 * @brief Structure of the pair of 64-bit words of the result of a multiplication
		 *
		 * \~
		 */
		typedef struct Value128 {
			// Младшие 64 бита результата
			uint64_t low;
			// Старшие 64 бита результата
			uint64_t high;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param low  младшие 64 бита результата
			 * @param high старшие 64 бита результата
			 *
			 * \~english
			 * @brief Constructor
			 * @param low  low 64 bits of the result
			 * @param high high 64 bits of the result
			 *
			 * \~
			 */
			constexpr Value128(const uint64_t low = 0, const uint64_t high = 0) noexcept :
			 low(low), high(high) {}
		} value128_t;

		/**
		 * \~russian
		 * @brief Структура скорректированной мантиссы двоичного представления
		 *
		 * @details Отрицательное значение степени двойки означает, что быстрый путь
		 *          вычисления не дал однозначного результата и требуется уточнение
		 *          через длинную арифметику.
		 *
		 * \~english
		 * @brief Structure of the corrected mantissa of the binary representation
		 * @details A negative value of the power of two means that the fast computation
		 *          path did not give an unambiguous result and refinement
		 *          through long arithmetic is required.
		 *
		 * \~
		 */
		typedef struct Mantissa {
			// Показатель степени двойки
			int32_t power2;
			// Значение мантиссы
			uint64_t mantissa;
			/**
			 * \~russian
			 * @brief Оператор сравнения на равенство
			 *
			 * @param other сравниваемая скорректированная мантисса
			 * @return      результат сравнения
			 *
			 * \~english
			 * @brief Equality comparison operator
			 * @param other corrected mantissa being compared with
			 * @return      result of the comparison
			 *
			 * \~
			 */
			constexpr bool operator == (const Mantissa & other) const noexcept {
				// Мантиссы равны, если совпадают оба поля
				return ((this->mantissa == other.mantissa) && (this->power2 == other.power2));
			}
			/**
			 * \~russian
			 * @brief Оператор сравнения на неравенство
			 *
			 * @param other сравниваемая скорректированная мантисса
			 * @return      результат сравнения
			 *
			 * \~english
			 * @brief Inequality comparison operator
			 * @param other corrected mantissa being compared with
			 * @return      result of the comparison
			 *
			 * \~
			 */
			constexpr bool operator != (const Mantissa & other) const noexcept {
				// Мантиссы различны, если различается хотя бы одно поле
				return ((this->mantissa != other.mantissa) || (this->power2 != other.power2));
			}
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param mantissa значение мантиссы
			 * @param power2   показатель степени двойки
			 *
			 * \~english
			 * @brief Constructor
			 * @param mantissa value of the mantissa
			 * @param power2   exponent of the power of two
			 *
			 * \~
			 */
			constexpr Mantissa(const uint64_t mantissa = 0, const int32_t power2 = 0) noexcept :
			 power2(power2), mantissa(mantissa) {}
		} mantissa_t;

		/**
		 * \~russian
		 * @brief Смещение показателя степени для невалидной мантиссы
		 *
		 * \~english
		 * @brief Exponent offset for an invalid mantissa
		 *
		 * \~
		 */
		constexpr int32_t INVALID_BIAS = -0x8000;

		/**
		 * \~russian
		 * @brief Константа пятой степени пятёрки для таблиц предельных мантисс
		 *
		 * \~english
		 * @brief Constant of the fifth power of five for the tables of the limiting mantissas
		 *
		 * \~
		 */
		constexpr uint64_t POWER_OF_FIVE_5 = (5ULL * 5ULL * 5ULL * 5ULL * 5ULL);

		/**
		 * \~russian
		 * @brief Метод подсчёта ведущих нулевых бит 64-битного значения
		 *
		 * @param value исходное значение
		 * @return      количество ведущих нулевых бит
		 *
		 * \~english
		 * @brief Method of counting the leading zero bits of a 64-bit value
		 * @param value source value
		 * @return      number of leading zero bits
		 *
		 * \~
		 */
		AWH_ASCII_INLINE int32_t leadingZeros(const uint64_t value) noexcept {
			// Если исходное значение нулевое, значащих бит нет
			if(value == 0)
				// Выводим полную разрядность значения
				return 64;
			/**
			 * Для компилятора Visual Studio используем интринсики
			 */
			#if AWH_LEXICAL_VISUAL_STUDIO && (_M_X64 || _M_ARM64)
				// Позиция старшего установленного бита
				unsigned long result = 0;
				// Выполняем поиск старшего установленного бита
				_BitScanReverse64(&result, value);
				// Выводим количество ведущих нулевых бит
				return static_cast <int32_t> (63 - result);
			/**
			 * Для компилятора Visual Studio без поддержки интринсик используем ручной поиск
			 */
			#elif AWH_LEXICAL_VISUAL_STUDIO
				// Накопленная позиция старшего установленного бита
				int32_t position = 0;
				// Копия исходного значения
				uint64_t number = value;
				// Если старшее слово содержит значащие биты
				if(number & 0xFFFFFFFF00000000ULL){
					// Выполняем сдвиг на разрядность слова
					number >>= 32;
					// Накапливаем позицию старшего бита
					position |= 32;
				}
				// Если старшее полуслово содержит значащие биты
				if(number & 0xFFFF0000ULL){
					// Выполняем сдвиг на разрядность полуслова
					number >>= 16;
					// Накапливаем позицию старшего бита
					position |= 16;
				}
				// Если старший байт содержит значащие биты
				if(number & 0xFF00ULL){
					// Выполняем сдвиг на разрядность байта
					number >>= 8;
					// Накапливаем позицию старшего бита
					position |= 8;
				}
				// Если старший полубайт содержит значащие биты
				if(number & 0xF0ULL){
					// Выполняем сдвиг на разрядность полубайта
					number >>= 4;
					// Накапливаем позицию старшего бита
					position |= 4;
				}
				// Если старшая пара бит содержит значащие биты
				if(number & 0x0CULL){
					// Выполняем сдвиг на разрядность пары бит
					number >>= 2;
					// Накапливаем позицию старшего бита
					position |= 2;
				}
				// Если старший бит пары установлен
				if(number & 0x02ULL)
					// Накапливаем позицию старшего бита
					position |= 1;
				// Выводим количество ведущих нулевых бит
				return (63 - position);
			/**
			 * Для других компиляторов используем встроенную функцию
			 */
			#else
				// Выводим количество ведущих нулевых бит
				return static_cast <int32_t> (__builtin_clzll(value));
			#endif
		}

		/**
		 * \~russian
		 * @brief Метод умножения двух 32-битных значений в 64-битное
		 *
		 * @param x первый множитель
		 * @param y второй множитель
		 * @return  результат умножения
		 *
		 * \~english
		 * @brief Method of multiplying two 32-bit values into a 64-bit one
		 * @param x first multiplier
		 * @param y second multiplier
		 * @return  result of the multiplication
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr uint64_t multiply32(const uint32_t x, const uint32_t y) noexcept {
			// Выполняем умножение с расширением разрядности
			return (static_cast <uint64_t> (x) * static_cast <uint64_t> (y));
		}

		/**
		 * \~russian
		 * @brief Метод универсального 128-битного умножения двух 64-битных значений
		 *
		 * @param x первый множитель
		 * @param y второй множитель
		 * @return  результат умножения в виде пары 64-битных слов
		 *
		 * \~english
		 * @brief Method of the universal 128-bit multiplication of two 64-bit values
		 * @param x first multiplier
		 * @param y second multiplier
		 * @return  result of the multiplication as a pair of 64-bit words
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr value128_t multiply128Generic(const uint64_t x, const uint64_t y) noexcept {
			// Произведение младших слов множителей
			const uint64_t lowLow = multiply32(static_cast <uint32_t> (x), static_cast <uint32_t> (y));
			// Произведение старшего слова первого множителя на младшее слово второго
			const uint64_t highLow = multiply32(static_cast <uint32_t> (x >> 32), static_cast <uint32_t> (y));
			// Сумма перекрёстных произведений
			const uint64_t cross = (highLow + multiply32(static_cast <uint32_t> (x), static_cast <uint32_t> (y >> 32)));
			// Признак переноса при вычислении суммы перекрёстных произведений
			const uint64_t crossCarry = static_cast <uint64_t> (cross < highLow);
			// Младшие 64 бита результата
			const uint64_t low = (lowLow + (cross << 32));
			// Выводим результат умножения
			return value128_t(low, (
				multiply32(static_cast <uint32_t> (x >> 32), static_cast <uint32_t> (y >> 32)) +
				(cross >> 32) + (crossCarry << 32) + static_cast <uint64_t> (low < lowLow)
			));
		}

		/**
		 * \~russian
		 * @brief Метод 128-битного умножения двух 64-битных значений
		 *
		 * @param x первый множитель
		 * @param y второй множитель
		 * @return  результат умножения в виде пары 64-битных слов
		 *
		 * \~english
		 * @brief Method of the 128-bit multiplication of two 64-bit values
		 * @param x first multiplier
		 * @param y second multiplier
		 * @return  result of the multiplication as a pair of 64-bit words
		 *
		 * \~
		 */
		AWH_ASCII_INLINE value128_t multiply128(const uint64_t x, const uint64_t y) noexcept {
			/**
			 * Для платформ с поддержкой 128-битного целого используем нативный тип
			 */
			#if AWH_LEXICAL_64BIT && __SIZEOF_INT128__
				// Выполняем умножение с расширением разрядности
				const __uint128_t result = (static_cast <__uint128_t> (x) * static_cast <__uint128_t> (y));
				// Выводим результат умножения
				return value128_t(static_cast <uint64_t> (result), static_cast <uint64_t> (result >> 64));
			/**
			 * Для платформы ARM64 с поддержкой 64-битного умножения с расширением разрядности используем нативный тип
			 */
			#elif _M_ARM64 && !__MINGW32__
				// Выводим результат умножения
				return value128_t((x * y), ::__umulh(x, y));
			/**
			 * Для платформы Windows с поддержкой 64-битного умножения с расширением разрядности используем нативный тип
			 */
			#elif _WIN64 && AWH_LEXICAL_VISUAL_STUDIO
				// Старшие 64 бита результата
				uint64_t high = 0;
				// Выполняем умножение с расширением разрядности
				const uint64_t low = _umul128(x, y, &high);
				// Выводим результат умножения
				return value128_t(low, high);
			/**
			 * Для других платформ используем универсальный метод умножения
			 */
			#else
				// Выводим результат универсального умножения
				return multiply128Generic(x, y);
			#endif
		}

		/**
		 * \~russian
		 * @brief Шаблон типа таблиц параметров двоичного формата
		 *
		 * @tparam T типа числа с плавающей точкой
		 * @tparam U фиктивный параметр для подстановки при специализации
		 *
		 * \~english
		 * @brief Template of the type of the tables of the binary format parameters
		 * @tparam T floating-point type
		 * @tparam U dummy parameter for the substitution at specialisation
		 *
		 * \~
		 */
		template <typename T, typename U = void>
		/**
		 * \~russian
		 * @brief Предварительное объявление таблиц параметров двоичного формата
		 *
		 * \~english
		 * @brief Forward declaration of the tables of the binary format parameters
		 *
		 * \~
		 */
		struct tables_t;

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
		 * @brief Структура параметров двоичного формата IEEE 754
		 *
		 * \~english
		 * @brief Structure of the parameters of the IEEE 754 binary format
		 *
		 * \~
		 */
		struct binary_t : tables_t <T> {
			/**
			 * \~russian
			 * @brief Создаём тип данных эквивалентного беззнакового целого
			 *
			 *
			 * \~english
			 * @brief Create the data type of the equivalent unsigned integer
			 *
			 * \~
			 */
			using equiv_uint = equivUint_t <T>;
			/**
			 * \~russian
			 * @brief Функция получения максимального количества значащих десятичных цифр
			 *
			 * @return максимальное количество значащих десятичных цифр
			 *
			 *
			 * \~english
			 * @brief Function of getting the maximum number of significant decimal digits
			 * @return maximum number of significant decimal digits
			 *
			 * \~
			 */
			static constexpr size_t maxDigits() noexcept;
			/**
			 * \~russian
			 * @brief Функция получения позиции бита знака
			 *
			 * @return позиция бита знака
			 *
			 *
			 * \~english
			 * @brief Function of getting the position of the sign bit
			 * @return position of the sign bit
			 *
			 * \~
			 */
			static constexpr int32_t signIndex() noexcept;
			/**
			 * \~russian
			 * @brief Функция получения показателя степени бесконечности
			 *
			 * @return показатель степени бесконечности
			 *
			 *
			 * \~english
			 * @brief Function of getting the exponent of infinity
			 * @return exponent of infinity
			 *
			 * \~
			 */
			static constexpr int32_t infinitePower() noexcept;
			/**
			 * \~russian
			 * @brief Функция получения маски показателя степени
			 *
			 * @return маска показателя степени
			 *
			 *
			 * \~english
			 * @brief Function of getting the exponent mask
			 * @return exponent mask
			 *
			 * \~
			 */
			static constexpr equiv_uint exponentMask() noexcept;
			/**
			 * \~russian
			 * @brief Функция получения маски мантиссы
			 *
			 * @return маска мантиссы
			 *
			 *
			 * \~english
			 * @brief Function of getting the mantissa mask
			 * @return mantissa mask
			 *
			 * \~
			 */
			static constexpr equiv_uint mantissaMask() noexcept;
			/**
			 * \~russian
			 * @brief Функция получения минимального показателя степени двойки
			 *
			 * @return минимальный показатель степени двойки
			 *
			 *
			 * \~english
			 * @brief Function of getting the minimum exponent of a power of two
			 * @return minimum exponent of a power of two
			 *
			 * \~
			 */
			static constexpr int32_t minimumExponent() noexcept;
			/**
			 * \~russian
			 * @brief Функция получения маски скрытого бита мантиссы
			 *
			 * @return маска скрытого бита мантиссы
			 *
			 *
			 * \~english
			 * @brief Function of getting the mask of the hidden bit of the mantissa
			 * @return mask of the hidden bit of the mantissa
			 *
			 * \~
			 */
			static constexpr equiv_uint hiddenBitMask() noexcept;
			/**
			 * \~russian
			 * @brief Функция получения наибольшего представимого показателя степени десяти
			 *
			 * @return наибольший представимый показатель степени десяти
			 *
			 *
			 * \~english
			 * @brief Function of getting the largest representable exponent of a power of ten
			 * @return largest representable exponent of a power of ten
			 *
			 * \~
			 */
			static constexpr int32_t largestPowerOfTen() noexcept;
			/**
			 * \~russian
			 * @brief Функция получения наименьшего представимого показателя степени десяти
			 *
			 * @return наименьший представимый показатель степени десяти
			 *
			 *
			 * \~english
			 * @brief Function of getting the smallest representable exponent of a power of ten
			 * @return smallest representable exponent of a power of ten
			 *
			 * \~
			 */
			static constexpr int32_t smallestPowerOfTen() noexcept;
			/**
			 * \~russian
			 * @brief Функция получения минимального показателя степени быстрого пути
			 *
			 * @return минимальный показатель степени быстрого пути
			 *
			 *
			 * \~english
			 * @brief Function of getting the minimum exponent of the fast path
			 * @return minimum exponent of the fast path
			 *
			 * \~
			 */
			static constexpr int32_t minExponentFastPath() noexcept;
			/**
			 * \~russian
			 * @brief Функция получения максимального показателя степени быстрого пути
			 *
			 * @return максимальный показатель степени быстрого пути
			 *
			 *
			 * \~english
			 * @brief Function of getting the maximum exponent of the fast path
			 * @return maximum exponent of the fast path
			 *
			 * \~
			 */
			static constexpr int32_t maxExponentFastPath() noexcept;
			/**
			 * \~russian
			 * @brief Функция получения количества явно хранимых бит мантиссы
			 *
			 * @return количество явно хранимых бит мантиссы
			 *
			 *
			 * \~english
			 * @brief Function of getting the number of explicitly stored bits of the mantissa
			 * @return number of explicitly stored bits of the mantissa
			 *
			 * \~
			 */
			static constexpr int32_t mantissaExplicitBits() noexcept;
			/**
			 * \~russian
			 * @brief Функция получения предельной мантиссы быстрого пути
			 *
			 * @return предельное значение мантиссы
			 *
			 *
			 * \~english
			 * @brief Function of getting the limiting mantissa of the fast path
			 * @return limiting value of the mantissa
			 *
			 * \~
			 */
			static constexpr uint64_t maxMantissaFastPath() noexcept;
			/**
			 * \~russian
			 * @brief Функция получения минимального показателя степени округления к чётному
			 *
			 * @return минимальный показатель степени округления к чётному
			 *
			 *
			 * \~english
			 * @brief Function of getting the minimum exponent of rounding to even
			 * @return minimum exponent of rounding to even
			 *
			 * \~
			 */
			static constexpr int32_t minExponentRoundToEven() noexcept;
			/**
			 * \~russian
			 * @brief Функция получения максимального показателя степени округления к чётному
			 *
			 * @return максимальный показатель степени округления к чётному
			 *
			 *
			 * \~english
			 * @brief Function of getting the maximum exponent of rounding to even
			 * @return maximum exponent of rounding to even
			 *
			 * \~
			 */
			static constexpr int32_t maxExponentRoundToEven() noexcept;
			/**
			 * \~russian
			 * @brief Функция получения точного значения степени десяти
			 *
			 * @param power показатель степени десяти
			 * @return      точное значение степени десяти
			 *
			 *
			 * \~english
			 * @brief Function of getting the exact value of a power of ten
			 * @param power exponent of the power of ten
			 * @return      exact value of the power of ten
			 *
			 * \~
			 */
			static constexpr T exactPowerOfTen(const int64_t power) noexcept;
			/**
			 * \~russian
			 * @brief Функция получения предельной мантиссы быстрого пути для степени десяти
			 *
			 * @param power показатель степени десяти
			 * @return      предельное значение мантиссы
			 *
			 *
			 * \~english
			 * @brief Function of getting the limiting mantissa of the fast path for a power of ten
			 * @param power exponent of the power of ten
			 * @return      limiting value of the mantissa
			 *
			 * \~
			 */
			static constexpr uint64_t maxMantissaFastPath(const int64_t power) noexcept;
		};

		/**
		 * \~russian
		 * @brief Шаблон фиктивного типа подстановки
		 *
		 * @tparam U фиктивный параметр для подстановки при специализации
		 *
		 *
		 * \~english
		 * @brief Template of a dummy substitution type
		 * @tparam U dummy parameter for the substitution at specialisation
		 *
		 * \~
		 */
		template <typename U>
		/**
		 * \~russian
		 * @brief Структура таблиц параметров двоичного формата двойной точности
		 *
		 * \~english
		 * @brief Structure of the tables of the parameters of the double-precision binary format
		 *
		 * \~
		 */
		struct tables_t <double, U> {
			/**
			 * \~russian
			 * @brief Таблица точных значений степеней десяти
			 *
			 *
			 * \~english
			 * @brief Table of the exact values of the powers of ten
			 *
			 * \~
			 */
			static constexpr double powersOfTen[] = {
				1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,
				1e8,  1e9,  1e10, 1e11, 1e12, 1e13, 1e14, 1e15,
				1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22
			};
			/**
			 * \~russian
			 * @brief Таблица предельных мантисс быстрого пути
			 *
			 *
			 * \~english
			 * @brief Table of the limiting mantissas of the fast path
			 *
			 * \~
			 */
			static constexpr uint64_t maxMantissa[] = {
				0x20000000000000ULL,
				0x20000000000000ULL / 5ULL,
				0x20000000000000ULL / (5ULL * 5ULL),
				0x20000000000000ULL / (5ULL * 5ULL * 5ULL),
				0x20000000000000ULL / (5ULL * 5ULL * 5ULL * 5ULL),
				0x20000000000000ULL / (POWER_OF_FIVE_5),
				0x20000000000000ULL / (POWER_OF_FIVE_5 * 5ULL),
				0x20000000000000ULL / (POWER_OF_FIVE_5 * 5ULL * 5ULL),
				0x20000000000000ULL / (POWER_OF_FIVE_5 * 5ULL * 5ULL * 5ULL),
				0x20000000000000ULL / (POWER_OF_FIVE_5 * 5ULL * 5ULL * 5ULL * 5ULL),
				0x20000000000000ULL / (POWER_OF_FIVE_5 * POWER_OF_FIVE_5),
				0x20000000000000ULL / (POWER_OF_FIVE_5 * POWER_OF_FIVE_5 * 5ULL),
				0x20000000000000ULL / (POWER_OF_FIVE_5 * POWER_OF_FIVE_5 * 5ULL * 5ULL),
				0x20000000000000ULL / (POWER_OF_FIVE_5 * POWER_OF_FIVE_5 * 5ULL * 5ULL * 5ULL),
				0x20000000000000ULL / (POWER_OF_FIVE_5 * POWER_OF_FIVE_5 * POWER_OF_FIVE_5),
				0x20000000000000ULL / (POWER_OF_FIVE_5 * POWER_OF_FIVE_5 * POWER_OF_FIVE_5 * 5ULL),
				0x20000000000000ULL / (POWER_OF_FIVE_5 * POWER_OF_FIVE_5 * POWER_OF_FIVE_5 * 5ULL * 5ULL),
				0x20000000000000ULL / (POWER_OF_FIVE_5 * POWER_OF_FIVE_5 * POWER_OF_FIVE_5 * 5ULL * 5ULL * 5ULL),
				0x20000000000000ULL / (POWER_OF_FIVE_5 * POWER_OF_FIVE_5 * POWER_OF_FIVE_5 * 5ULL * 5ULL * 5ULL * 5ULL),
				0x20000000000000ULL / (POWER_OF_FIVE_5 * POWER_OF_FIVE_5 * POWER_OF_FIVE_5 * POWER_OF_FIVE_5),
				0x20000000000000ULL / (POWER_OF_FIVE_5 * POWER_OF_FIVE_5 * POWER_OF_FIVE_5 * POWER_OF_FIVE_5 * 5ULL),
				0x20000000000000ULL / (POWER_OF_FIVE_5 * POWER_OF_FIVE_5 * POWER_OF_FIVE_5 * POWER_OF_FIVE_5 * 5ULL * 5ULL),
				0x20000000000000ULL / (POWER_OF_FIVE_5 * POWER_OF_FIVE_5 * POWER_OF_FIVE_5 * POWER_OF_FIVE_5 * 5ULL * 5ULL * 5ULL),
				0x20000000000000ULL / (POWER_OF_FIVE_5 * POWER_OF_FIVE_5 * POWER_OF_FIVE_5 * POWER_OF_FIVE_5 * 5ULL * 5ULL * 5ULL * 5ULL)
			};
		};

		/**
		 * \~russian
		 * @brief Шаблон фиктивного типа подстановки
		 *
		 * @tparam U фиктивный параметр для подстановки при специализации
		 *
		 *
		 * \~english
		 * @brief Template of a dummy substitution type
		 * @tparam U dummy parameter for the substitution at specialisation
		 *
		 * \~
		 */
		template <typename U>
		/**
		 * \~russian
		 * @brief Структура таблиц параметров двоичного формата одинарной точности
		 *
		 * \~english
		 * @brief Structure of the tables of the parameters of the single-precision binary format
		 *
		 * \~
		 */
		struct tables_t <float, U> {
			/**
			 * \~russian
			 * @brief Таблица точных значений степеней десяти
			 *
			 *
			 * \~english
			 * @brief Table of the exact values of the powers of ten
			 *
			 * \~
			 */
			static constexpr float powersOfTen[] = {
				1e0f, 1e1f, 1e2f, 1e3f, 1e4f, 1e5f,
				1e6f, 1e7f, 1e8f, 1e9f, 1e10f
			};
			/**
			 * \~russian
			 * @brief Таблица предельных мантисс быстрого пути
			 *
			 *
			 * \~english
			 * @brief Table of the limiting mantissas of the fast path
			 *
			 * \~
			 */
			static constexpr uint64_t maxMantissa[] = {
				0x1000000ULL,
				0x1000000ULL / 5ULL,
				0x1000000ULL / (5ULL * 5ULL),
				0x1000000ULL / (5ULL * 5ULL * 5ULL),
				0x1000000ULL / (5ULL * 5ULL * 5ULL * 5ULL),
				0x1000000ULL / (POWER_OF_FIVE_5),
				0x1000000ULL / (POWER_OF_FIVE_5 * 5ULL),
				0x1000000ULL / (POWER_OF_FIVE_5 * 5ULL * 5ULL),
				0x1000000ULL / (POWER_OF_FIVE_5 * 5ULL * 5ULL * 5ULL),
				0x1000000ULL / (POWER_OF_FIVE_5 * 5ULL * 5ULL * 5ULL * 5ULL),
				0x1000000ULL / (POWER_OF_FIVE_5 * POWER_OF_FIVE_5),
				0x1000000ULL / (POWER_OF_FIVE_5 * POWER_OF_FIVE_5 * 5ULL)
			};
		};

		/**
		 * \~russian
		 * @brief Функция получения максимального количества значащих десятичных цифр
		 *
		 * @return максимальное количество значащих десятичных цифр
		 *
		 *
		 * \~english
		 * @brief Function of getting the maximum number of significant decimal digits
		 * @return maximum number of significant decimal digits
		 *
		 * \~
		 */
		template <> inline constexpr size_t binary_t <float>::maxDigits() noexcept {
			// Выводим максимальное количество значащих десятичных цифр
			return 114;
		}
		/**
		 * \~russian
		 * @brief Функция получения максимального количества значащих десятичных цифр
		 *
		 * @return максимальное количество значащих десятичных цифр
		 *
		 *
		 * \~english
		 * @brief Function of getting the maximum number of significant decimal digits
		 * @return maximum number of significant decimal digits
		 *
		 * \~
		 */
		template <> inline constexpr size_t binary_t <double>::maxDigits() noexcept {
			// Выводим максимальное количество значащих десятичных цифр
			return 769;
		}
		/**
		 * \~russian
		 * @brief Функция получения позиции бита знака
		 *
		 * @return позиция бита знака
		 *
		 *
		 * \~english
		 * @brief Function of getting the position of the sign bit
		 * @return position of the sign bit
		 *
		 * \~
		 */
		template <> inline constexpr int32_t binary_t <float>::signIndex() noexcept {
			// Выводим позицию бита знака
			return 31;
		}
		/**
		 * \~russian
		 * @brief Функция получения позиции бита знака
		 *
		 * @return позиция бита знака
		 *
		 *
		 * \~english
		 * @brief Function of getting the position of the sign bit
		 * @return position of the sign bit
		 *
		 * \~
		 */
		template <> inline constexpr int32_t binary_t <double>::signIndex() noexcept {
			// Выводим позицию бита знака
			return 63;
		}
		/**
		 * \~russian
		 * @brief Функция получения показателя степени бесконечности
		 *
		 * @return значение бесконечной степени
		 *
		 *
		 * \~english
		 * @brief Function of getting the exponent of infinity
		 * @return value of the infinite exponent
		 *
		 * \~
		 */
		template <> inline constexpr int32_t binary_t <float>::infinitePower() noexcept {
			// Выводим значение бесконечной степени
			return 0xFF;
		}
		/**
		 * \~russian
		 * @brief Функция получения показателя степени бесконечности
		 *
		 * @return значение бесконечной степени
		 *
		 *
		 * \~english
		 * @brief Function of getting the exponent of infinity
		 * @return value of the infinite exponent
		 *
		 * \~
		 */
		template <> inline constexpr int32_t binary_t <double>::infinitePower() noexcept {
			// Выводим значение бесконечной степени
			return 0x7FF;
		}
		/**
		 * \~russian
		 * @brief Функция получения маски показателя степени
		 *
		 * @return маска показателя степени
		 *
		 *
		 * \~english
		 * @brief Function of getting the exponent mask
		 * @return exponent mask
		 *
		 * \~
		 */
		template <> inline constexpr binary_t <float>::equiv_uint binary_t <float>::exponentMask() noexcept {
			// Выводим маску показателя степени
			return 0x7F800000UL;
		}
		/**
		 * \~russian
		 * @brief Функция получения маски показателя степени
		 *
		 * @return маска показателя степени
		 *
		 *
		 * \~english
		 * @brief Function of getting the exponent mask
		 * @return exponent mask
		 *
		 * \~
		 */
		template <> inline constexpr binary_t <double>::equiv_uint binary_t <double>::exponentMask() noexcept {
			// Выводим маску показателя степени
			return 0x7FF0000000000000ULL;
		}
		/**
		 * \~russian
		 * @brief Функция получения маски мантиссы
		 *
		 * @return маска мантиссы
		 *
		 *
		 * \~english
		 * @brief Function of getting the mantissa mask
		 * @return mantissa mask
		 *
		 * \~
		 */
		template <> inline constexpr binary_t <float>::equiv_uint binary_t <float>::mantissaMask() noexcept {
			// Выводим маску мантиссы
			return 0x007FFFFFUL;
		}
		/**
		 * \~russian
		 * @brief Функция получения маски мантиссы
		 *
		 * @return маска мантиссы
		 *
		 *
		 * \~english
		 * @brief Function of getting the mantissa mask
		 * @return mantissa mask
		 *
		 * \~
		 */
		template <> inline constexpr binary_t <double>::equiv_uint binary_t <double>::mantissaMask() noexcept {
			// Выводим маску мантиссы
			return 0x000FFFFFFFFFFFFFULL;
		}
		/**
		 * \~russian
		 * @brief Функция получения минимального показателя степени двойки
		 *
		 * @return минимальный показатель степени двойки
		 *
		 *
		 * \~english
		 * @brief Function of getting the minimum exponent of a power of two
		 * @return minimum exponent of a power of two
		 *
		 * \~
		 */
		template <> inline constexpr int32_t binary_t <float>::minimumExponent() noexcept {
			// Выводим минимальный показатель степени двойки
			return -127;
		}
		/**
		 * \~russian
		 * @brief Функция получения минимального показателя степени двойки
		 *
		 * @return минимальный показатель степени двойки
		 *
		 *
		 * \~english
		 * @brief Function of getting the minimum exponent of a power of two
		 * @return minimum exponent of a power of two
		 *
		 * \~
		 */
		template <> inline constexpr int32_t binary_t <double>::minimumExponent() noexcept {
			// Выводим минимальный показатель степени двойки
			return -1023;
		}
		/**
		 * \~russian
		 * @brief Функция получения маски скрытого бита мантиссы
		 *
		 * @return маска скрытого бита мантиссы
		 *
		 *
		 * \~english
		 * @brief Function of getting the mask of the hidden bit of the mantissa
		 * @return mask of the hidden bit of the mantissa
		 *
		 * \~
		 */
		template <> inline constexpr binary_t <float>::equiv_uint binary_t <float>::hiddenBitMask() noexcept {
			// Выводим маску скрытого бита мантиссы
			return 0x00800000UL;
		}
		/**
		 * \~russian
		 * @brief Функция получения маски скрытого бита мантиссы
		 *
		 * @return маска скрытого бита мантиссы
		 *
		 *
		 * \~english
		 * @brief Function of getting the mask of the hidden bit of the mantissa
		 * @return mask of the hidden bit of the mantissa
		 *
		 * \~
		 */
		template <> inline constexpr binary_t <double>::equiv_uint binary_t <double>::hiddenBitMask() noexcept {
			// Выводим маску скрытого бита мантиссы
			return 0x0010000000000000ULL;
		}
		/**
		 * \~russian
		 * @brief Функция получения наибольшего представимого показателя степени десяти
		 *
		 * @return наибольший представимый показатель степени десяти
		 *
		 *
		 * \~english
		 * @brief Function of getting the largest representable exponent of a power of ten
		 * @return largest representable exponent of a power of ten
		 *
		 * \~
		 */
		template <> inline constexpr int32_t binary_t <float>::largestPowerOfTen() noexcept {
			// Выводим наибольший представимый показатель степени десяти
			return 38;
		}
		/**
		 * \~russian
		 * @brief Функция получения наибольшего представимого показателя степени десяти
		 *
		 * @return наибольший представимый показатель степени десяти
		 *
		 *
		 * \~english
		 * @brief Function of getting the largest representable exponent of a power of ten
		 * @return largest representable exponent of a power of ten
		 *
		 * \~
		 */
		template <> inline constexpr int32_t binary_t <double>::largestPowerOfTen() noexcept {
			// Выводим наибольший представимый показатель степени десяти
			return 308;
		}
		/**
		 * \~russian
		 * @brief Функция получения наименьшего представимого показателя степени десяти
		 *
		 * @return наименьший представимый показатель степени десяти
		 *
		 *
		 * \~english
		 * @brief Function of getting the smallest representable exponent of a power of ten
		 * @return smallest representable exponent of a power of ten
		 *
		 * \~
		 */
		template <> inline constexpr int32_t binary_t <float>::smallestPowerOfTen() noexcept {
			// Выводим наименьший представимый показатель степени десяти
			return -64;
		}
		/**
		 * \~russian
		 * @brief Функция получения наименьшего представимого показателя степени десяти
		 *
		 * @return наименьший представимый показатель степени десяти
		 *
		 *
		 * \~english
		 * @brief Function of getting the smallest representable exponent of a power of ten
		 * @return smallest representable exponent of a power of ten
		 *
		 * \~
		 */
		template <> inline constexpr int32_t binary_t <double>::smallestPowerOfTen() noexcept {
			// Выводим наименьший представимый показатель степени десяти
			return -342;
		}
		/**
		 * \~russian
		 * @brief Функция получения минимального показателя степени быстрого пути
		 *
		 * @return минимальный показатель степени быстрого пути
		 *
		 *
		 * \~english
		 * @brief Function of getting the minimum exponent of the fast path
		 * @return minimum exponent of the fast path
		 *
		 * \~
		 */
		template <> inline constexpr int32_t binary_t <float>::minExponentFastPath() noexcept {
			/**
			 * При расширенной точности вычислений деление на степень десяти небезопасно
			 */
			#if (FLT_EVAL_METHOD != 1) && (FLT_EVAL_METHOD != 0)
				// Отрицательные показатели степени быстрым путём не обрабатываются
				return 0;
			/**
			 * При стандартной точности вычислений выводим минимальный показатель степени быстрого пути
			 */
			#else
				// Выводим минимальный показатель степени быстрого пути
				return -10;
			#endif
		}
		/**
		 * \~russian
		 * @brief Функция получения минимального показателя степени быстрого пути
		 *
		 * @return минимальный показатель степени быстрого пути
		 *
		 *
		 * \~english
		 * @brief Function of getting the minimum exponent of the fast path
		 * @return minimum exponent of the fast path
		 *
		 * \~
		 */
		template <> inline constexpr int32_t binary_t <double>::minExponentFastPath() noexcept {
			/**
			 * При расширенной точности вычислений деление на степень десяти небезопасно
			 */
			#if (FLT_EVAL_METHOD != 1) && (FLT_EVAL_METHOD != 0)
				// Отрицательные показатели степени быстрым путём не обрабатываются
				return 0;
			/**
			 * При стандартной точности вычислений выводим минимальный показатель степени быстрого пути
			 */
			#else
				// Выводим минимальный показатель степени быстрого пути
				return -22;
			#endif
		}
		/**
		 * \~russian
		 * @brief Функция получения максимального показателя степени быстрого пути
		 *
		 * @return максимальный показатель степени быстрого пути
		 *
		 *
		 * \~english
		 * @brief Function of getting the maximum exponent of the fast path
		 * @return maximum exponent of the fast path
		 *
		 * \~
		 */
		template <> inline constexpr int32_t binary_t <float>::maxExponentFastPath() noexcept {
			// Выводим максимальный показатель степени быстрого пути
			return 10;
		}
		/**
		 * \~russian
		 * @brief Функция получения максимального показателя степени быстрого пути
		 *
		 * @return максимальный показатель степени быстрого пути
		 *
		 *
		 * \~english
		 * @brief Function of getting the maximum exponent of the fast path
		 * @return maximum exponent of the fast path
		 *
		 * \~
		 */
		template <> inline constexpr int32_t binary_t <double>::maxExponentFastPath() noexcept {
			// Выводим максимальный показатель степени быстрого пути
			return 22;
		}
		/**
		 * \~russian
		 * @brief Функция получения количества явно хранимых бит мантиссы
		 *
		 * @return количество явно хранимых бит мантиссы
		 *
		 *
		 * \~english
		 * @brief Function of getting the number of explicitly stored bits of the mantissa
		 * @return number of explicitly stored bits of the mantissa
		 *
		 * \~
		 */
		template <> inline constexpr int32_t binary_t <float>::mantissaExplicitBits() noexcept {
			// Выводим количество явно хранимых бит мантиссы
			return 23;
		}
		/**
		 * \~russian
		 * @brief Функция получения количества явно хранимых бит мантиссы
		 *
		 * @return количество явно хранимых бит мантиссы
		 *
		 *
		 * \~english
		 * @brief Function of getting the number of explicitly stored bits of the mantissa
		 * @return number of explicitly stored bits of the mantissa
		 *
		 * \~
		 */
		template <> inline constexpr int32_t binary_t <double>::mantissaExplicitBits() noexcept {
			// Выводим количество явно хранимых бит мантиссы
			return 52;
		}
		/**
		 * \~russian
		 * @brief Функция получения предельной мантиссы быстрого пути
		 *
		 * @return предельное значение мантиссы
		 *
		 *
		 * \~english
		 * @brief Function of getting the limiting mantissa of the fast path
		 * @return limiting value of the mantissa
		 *
		 * \~
		 */
		template <> inline constexpr uint64_t binary_t <float>::maxMantissaFastPath() noexcept {
			// Выводим предельное значение мантиссы быстрого пути
			return (uint64_t(2) << mantissaExplicitBits());
		}
		/**
		 * \~russian
		 * @brief Функция получения предельной мантиссы быстрого пути
		 *
		 * @return предельное значение мантиссы
		 *
		 *
		 * \~english
		 * @brief Function of getting the limiting mantissa of the fast path
		 * @return limiting value of the mantissa
		 *
		 * \~
		 */
		template <> inline constexpr uint64_t binary_t <double>::maxMantissaFastPath() noexcept {
			// Выводим предельное значение мантиссы быстрого пути
			return (uint64_t(2) << mantissaExplicitBits());
		}
		/**
		 * \~russian
		 * @brief Функция получения минимального показателя степени округления к чётному
		 *
		 * @return минимальный показатель степени округления к чётному
		 *
		 *
		 * \~english
		 * @brief Function of getting the minimum exponent of rounding to even
		 * @return minimum exponent of rounding to even
		 *
		 * \~
		 */
		template <> inline constexpr int32_t binary_t <float>::minExponentRoundToEven() noexcept {
			// Выводим минимальный показатель степени округления к чётному
			return -17;
		}
		/**
		 * \~russian
		 * @brief Функция получения минимального показателя степени округления к чётному
		 *
		 * @return минимальный показатель степени округления к чётному
		 *
		 *
		 * \~english
		 * @brief Function of getting the minimum exponent of rounding to even
		 * @return minimum exponent of rounding to even
		 *
		 * \~
		 */
		template <> inline constexpr int32_t binary_t <double>::minExponentRoundToEven() noexcept {
			// Выводим минимальный показатель степени округления к чётному
			return -4;
		}
		/**
		 * \~russian
		 * @brief Функция получения максимального показателя степени округления к чётному
		 *
		 * @return максимальный показатель степени округления к чётному
		 *
		 *
		 * \~english
		 * @brief Function of getting the maximum exponent of rounding to even
		 * @return maximum exponent of rounding to even
		 *
		 * \~
		 */
		template <> inline constexpr int32_t binary_t <float>::maxExponentRoundToEven() noexcept {
			// Выводим максимальный показатель степени округления к чётному
			return 10;
		}
		/**
		 * \~russian
		 * @brief Функция получения максимального показателя степени округления к чётному
		 *
		 * @return максимальный показатель степени округления к чётному
		 *
		 *
		 * \~english
		 * @brief Function of getting the maximum exponent of rounding to even
		 * @return maximum exponent of rounding to even
		 *
		 * \~
		 */
		template <> inline constexpr int32_t binary_t <double>::maxExponentRoundToEven() noexcept {
			// Выводим максимальный показатель степени округления к чётному
			return 23;
		}
		/**
		 * \~russian
		 * @brief Функция получения точного значения степени десяти
		 *
		 * @param power показатель степени десяти
		 * @return      точное значение степени десяти
		 *
		 *
		 * \~english
		 * @brief Function of getting the exact value of a power of ten
		 * @param power exponent of the power of ten
		 * @return      exact value of the power of ten
		 *
		 * \~
		 */
		template <> inline constexpr float binary_t <float>::exactPowerOfTen(const int64_t power) noexcept {
			// Выводим точное значение степени десяти из таблицы
			return ((void) powersOfTen[0], powersOfTen[power]);
		}
		/**
		 * \~russian
		 * @brief Функция получения точного значения степени десяти
		 *
		 * @param power показатель степени десяти
		 * @return      точное значение степени десяти
		 *
		 *
		 * \~english
		 * @brief Function of getting the exact value of a power of ten
		 * @param power exponent of the power of ten
		 * @return      exact value of the power of ten
		 *
		 * \~
		 */
		template <> inline constexpr double binary_t <double>::exactPowerOfTen(const int64_t power) noexcept {
			// Выводим точное значение степени десяти из таблицы
			return ((void) powersOfTen[0], powersOfTen[power]);
		}
		/**
		 * \~russian
		 * @brief Функция получения предельной мантиссы быстрого пути для степени десяти
		 *
		 * @param power показатель степени десяти
		 * @return      предельное значение мантиссы
		 *
		 *
		 * \~english
		 * @brief Function of getting the limiting mantissa of the fast path for a power of ten
		 * @param power exponent of the power of ten
		 * @return      limiting value of the mantissa
		 *
		 * \~
		 */
		template <> inline constexpr uint64_t binary_t <float>::maxMantissaFastPath(const int64_t power) noexcept {
			// Выводим предельное значение мантиссы из таблицы
			return ((void) maxMantissa[0], maxMantissa[power]);
		}
		/**
		 * \~russian
		 * @brief Функция получения предельной мантиссы быстрого пути для степени десяти
		 *
		 * @param power показатель степени десяти
		 * @return      предельное значение мантиссы
		 *
		 *
		 * \~english
		 * @brief Function of getting the limiting mantissa of the fast path for a power of ten
		 * @param power exponent of the power of ten
		 * @return      limiting value of the mantissa
		 *
		 * \~
		 */
		template <> inline constexpr uint64_t binary_t <double>::maxMantissaFastPath(const int64_t power) noexcept {
			// Выводим предельное значение мантиссы из таблицы
			return ((void) maxMantissa[0], maxMantissa[power]);
		}

		/**
		 * Если компилятор поддерживает типы половинной точности и bfloat16, то определяем их параметры
		 */
		#ifdef __STDCPP_FLOAT16_T__
			/**
			 * \~russian
			 * @brief Шаблон фиктивного типа подстановки
			 *
			 * @tparam U фиктивный параметр для подстановки при специализации
			 *
			 *
			 * \~english
			 * @brief Template of a dummy substitution type
			 * @tparam U dummy parameter for the substitution at specialisation
			 *
			 * \~
			 */
			template <typename U>
			/**
			 * \~russian
			 * @brief Структура таблиц параметров двоичного формата половинной точности
			 *
			 * \~english
			 * @brief Structure of the tables of the parameters of the half-precision binary format
			 *
			 * \~
			 */
			struct tables_t <float16_t, U> {
				/**
				 * \~russian
				 * @brief Таблица точных значений степеней десяти
				 *
				 *
				 * \~english
				 * @brief Table of the exact values of the powers of ten
				 *
				 * \~
				 */
				static constexpr float16_t powersOfTen[] = {
					1e0f16, 1e1f16, 1e2f16, 1e3f16, 1e4f16
				};
				/**
				 * \~russian
				 * @brief Таблица предельных мантисс быстрого пути
				 *
				 *
				 * \~english
				 * @brief Table of the limiting mantissas of the fast path
				 *
				 * \~
				 */
				static constexpr uint64_t maxMantissa[] = {
					0x800ULL,
					0x800ULL / 5ULL,
					0x800ULL / (5ULL * 5ULL),
					0x800ULL / (5ULL * 5ULL * 5ULL),
					0x800ULL / (5ULL * 5ULL * 5ULL * 5ULL),
					0x800ULL / (POWER_OF_FIVE_5)
				};
			};

			/**
			 * \~russian
			 * @brief Функция получения максимального количества значащих десятичных цифр
			 *
			 * @return максимальное количество значащих десятичных цифр
			 *
			 *
			 * \~english
			 * @brief Function of getting the maximum number of significant decimal digits
			 * @return maximum number of significant decimal digits
			 *
			 * \~
			 */
			template <> inline constexpr size_t binary_t <float16_t>::maxDigits() noexcept {
				// Выводим максимальное количество значащих десятичных цифр
				return 22;
			}
			/**
			 * \~russian
			 * @brief Функция получения позиции бита знака
			 *
			 * @return позиция бита знака
			 *
			 *
			 * \~english
			 * @brief Function of getting the position of the sign bit
			 * @return position of the sign bit
			 *
			 * \~
			 */
			template <> inline constexpr int32_t binary_t <float16_t>::signIndex() noexcept {
				// Выводим позицию бита знака
				return 15;
			}
			/**
			 * \~russian
			 * @brief Функция получения показателя степени бесконечности
			 *
			 * @return показатель степени бесконечности
			 *
			 *
			 * \~english
			 * @brief Function of getting the exponent of infinity
			 * @return exponent of infinity
			 *
			 * \~
			 */
			template <> inline constexpr int32_t binary_t <float16_t>::infinitePower() noexcept {
				// Выводим показатель степени бесконечности
				return 0x1F;
			}
			/**
			 * \~russian
			 * @brief Функция получения маски показателя степени
			 *
			 * @return маска показателя степени
			 *
			 *
			 * \~english
			 * @brief Function of getting the exponent mask
			 * @return exponent mask
			 *
			 * \~
			 */
			template <> inline constexpr binary_t <float16_t>::equiv_uint binary_t <float16_t>::exponentMask() noexcept {
				// Выводим маску показателя степени
				return 0x7C00;
			}
			/**
			 * \~russian
			 * @brief Функция получения маски мантиссы
			 *
			 * @return маска мантиссы
			 *
			 *
			 * \~english
			 * @brief Function of getting the mantissa mask
			 * @return mantissa mask
			 *
			 * \~
			 */
			template <> inline constexpr binary_t <float16_t>::equiv_uint binary_t <float16_t>::mantissaMask() noexcept {
				// Выводим маску мантиссы
				return 0x03FF;
			}
			/**
			 * \~russian
			 * @brief Функция получения минимального показателя степени двойки
			 *
			 * @return минимальный показатель степени двойки
			 *
			 *
			 * \~english
			 * @brief Function of getting the minimum exponent of a power of two
			 * @return minimum exponent of a power of two
			 *
			 * \~
			 */
			template <> inline constexpr int32_t binary_t <float16_t>::minimumExponent() noexcept {
				// Выводим минимальный показатель степени двойки
				return -15;
			}
			/**
			 * \~russian
			 * @brief Функция получения маски скрытого бита мантиссы
			 *
			 * @return маска скрытого бита мантиссы
			 *
			 *
			 * \~english
			 * @brief Function of getting the mask of the hidden bit of the mantissa
			 * @return mask of the hidden bit of the mantissa
			 *
			 * \~
			 */
			template <> inline constexpr binary_t <float16_t>::equiv_uint binary_t <float16_t>::hiddenBitMask() noexcept {
				// Выводим маску скрытого бита мантиссы
				return 0x0400;
			}
			/**
			 * \~russian
			 * @brief Функция получения наибольшего представимого показателя степени десяти
			 *
			 * @return наибольший представимый показатель степени десяти
			 *
			 *
			 * \~english
			 * @brief Function of getting the largest representable exponent of a power of ten
			 * @return largest representable exponent of a power of ten
			 *
			 * \~
			 */
			template <> inline constexpr int32_t binary_t <float16_t>::largestPowerOfTen() noexcept {
				// Выводим наибольший представимый показатель степени десяти
				return 4;
			}
			/**
			 * \~russian
			 * @brief Функция получения наименьшего представимого показателя степени десяти
			 *
			 * @return наименьший представимый показатель степени десяти
			 *
			 *
			 * \~english
			 * @brief Function of getting the smallest representable exponent of a power of ten
			 * @return smallest representable exponent of a power of ten
			 *
			 * \~
			 */
			template <> inline constexpr int32_t binary_t <float16_t>::smallestPowerOfTen() noexcept {
				// Выводим наименьший представимый показатель степени десяти
				return -27;
			}
			/**
			 * \~russian
			 * @brief Функция получения минимального показателя степени быстрого пути
			 *
			 * @return минимальный показатель степени быстрого пути
			 *
			 *
			 * \~english
			 * @brief Function of getting the minimum exponent of the fast path
			 * @return minimum exponent of the fast path
			 *
			 * \~
			 */
			template <> inline constexpr int32_t binary_t <float16_t>::minExponentFastPath() noexcept {
				// Выводим минимальный показатель степени быстрого пути
				return 0;
			}
			/**
			 * \~russian
			 * @brief Функция получения максимального показателя степени быстрого пути
			 *
			 * @return максимальный показатель степени быстрого пути
			 *
			 *
			 * \~english
			 * @brief Function of getting the maximum exponent of the fast path
			 * @return maximum exponent of the fast path
			 *
			 * \~
			 */
			template <> inline constexpr int32_t binary_t <float16_t>::maxExponentFastPath() noexcept {
				// Выводим максимальный показатель степени быстрого пути
				return 4;
			}
			/**
			 * \~russian
			 * @brief Функция получения количества явно хранимых бит мантиссы
			 *
			 * @return количество явно хранимых бит мантиссы
			 *
			 *
			 * \~english
			 * @brief Function of getting the number of explicitly stored bits of the mantissa
			 * @return number of explicitly stored bits of the mantissa
			 *
			 * \~
			 */
			template <> inline constexpr int32_t binary_t <float16_t>::mantissaExplicitBits() noexcept {
				// Выводим количество явно хранимых бит мантиссы
				return 10;
			}
			/**
			 * \~russian
			 * @brief Функция получения предельной мантиссы быстрого пути
			 *
			 * @return предельное значение мантиссы
			 *
			 *
			 * \~english
			 * @brief Function of getting the limiting mantissa of the fast path
			 * @return limiting value of the mantissa
			 *
			 * \~
			 */
			template <> inline constexpr uint64_t binary_t <float16_t>::maxMantissaFastPath() noexcept {
				// Выводим предельное значение мантиссы быстрого пути
				return (uint64_t(2) << mantissaExplicitBits());
			}
			/**
			 * \~russian
			 * @brief Функция получения минимального показателя степени округления к чётному
			 *
			 * @return минимальный показатель степени округления к чётному
			 *
			 *
			 * \~english
			 * @brief Function of getting the minimum exponent of rounding to even
			 * @return minimum exponent of rounding to even
			 *
			 * \~
			 */
			template <> inline constexpr int32_t binary_t <float16_t>::minExponentRoundToEven() noexcept {
				// Выводим минимальный показатель степени округления к чётному
				return -22;
			}
			/**
			 * \~russian
			 * @brief Функция получения максимального показателя степени округления к чётному
			 *
			 * @return максимальный показатель степени округления к чётному
			 *
			 *
			 * \~english
			 * @brief Function of getting the maximum exponent of rounding to even
			 * @return maximum exponent of rounding to even
			 *
			 * \~
			 */
			template <> inline constexpr int32_t binary_t <float16_t>::maxExponentRoundToEven() noexcept {
				// Выводим максимальный показатель степени округления к чётному
				return 5;
			}
			/**
			 * \~russian
			 * @brief Функция получения точного значения степени десяти
			 *
			 * @param power показатель степени десяти
			 * @return      точное значение степени десяти
			 *
			 *
			 * \~english
			 * @brief Function of getting the exact value of a power of ten
			 * @param power exponent of the power of ten
			 * @return      exact value of the power of ten
			 *
			 * \~
			 */
			template <> inline constexpr float16_t binary_t <float16_t>::exactPowerOfTen(const int64_t power) noexcept {
				// Выводим точное значение степени десяти из таблицы
				return ((void) powersOfTen[0], powersOfTen[power]);
			}
			/**
			 * \~russian
			 * @brief Функция получения предельной мантиссы быстрого пути для степени десяти
			 *
			 * @param power показатель степени десяти
			 * @return      предельное значение мантиссы
			 *
			 *
			 * \~english
			 * @brief Function of getting the limiting mantissa of the fast path for a power of ten
			 * @param power exponent of the power of ten
			 * @return      limiting value of the mantissa
			 *
			 * \~
			 */
			template <> inline constexpr uint64_t binary_t <float16_t>::maxMantissaFastPath(const int64_t power) noexcept {
				// Выводим предельное значение мантиссы из таблицы
				return ((void) maxMantissa[0], maxMantissa[power]);
			}
		#endif

		/**
		 * Если компилятор поддерживает тип bfloat16, то определяем его параметры
		 */
		#ifdef __STDCPP_BFLOAT16_T__
			/**
			 * \~russian
			 * @brief Шаблон фиктивного типа подстановки
			 *
			 * @tparam U фиктивный параметр для подстановки при специализации
			 *
			 *
			 * \~english
			 * @brief Template of a dummy substitution type
			 * @tparam U dummy parameter for the substitution at specialisation
			 *
			 * \~
			 */
			template <typename U>
			/**
			 * \~russian
			 * @brief Структура таблиц параметров двоичного формата bfloat16
			 *
			 * \~english
			 * @brief Structure of the tables of the parameters of the bfloat16 binary format
			 *
			 * \~
			 */
			struct tables_t <bfloat16_t, U> {
				/**
				 * \~russian
				 * @brief Таблица точных значений степеней десяти
				 *
				 *
				 * \~english
				 * @brief Table of the exact values of the powers of ten
				 *
				 * \~
				 */
				static constexpr bfloat16_t powersOfTen[] = {
					1e0bf16, 1e1bf16, 1e2bf16, 1e3bf16
				};
				/**
				 * \~russian
				 * @brief Таблица предельных мантисс быстрого пути
				 *
				 *
				 * \~english
				 * @brief Table of the limiting mantissas of the fast path
				 *
				 * \~
				 */
				static constexpr uint64_t maxMantissa[] = {
					0x100ULL,
					0x100ULL / 5ULL,
					0x100ULL / (5ULL * 5ULL),
					0x100ULL / (5ULL * 5ULL * 5ULL),
					0x100ULL / (5ULL * 5ULL * 5ULL * 5ULL)
				};
			};

			/**
			 * \~russian
			 * @brief Функция получения максимального количества значащих десятичных цифр
			 *
			 * @return максимальное количество значащих десятичных цифр
			 *
			 *
			 * \~english
			 * @brief Function of getting the maximum number of significant decimal digits
			 * @return maximum number of significant decimal digits
			 *
			 * \~
			 */
			template <> inline constexpr size_t binary_t <bfloat16_t>::maxDigits() noexcept {
				// Выводим максимальное количество значащих десятичных цифр
				return 98;
			}
			/**
			 * \~russian
			 * @brief Функция получения позиции бита знака
			 *
			 * @return позиция бита знака
			 *
			 *
			 * \~english
			 * @brief Function of getting the position of the sign bit
			 * @return position of the sign bit
			 *
			 * \~
			 */
			template <> inline constexpr int32_t binary_t <bfloat16_t>::signIndex() noexcept {
				// Выводим позицию бита знака
				return 15;
			}
			/**
			 * \~russian
			 * @brief Функция получения показателя степени бесконечности
			 *
			 * @return показатель степени бесконечности
			 *
			 *
			 * \~english
			 * @brief Function of getting the exponent of infinity
			 * @return exponent of infinity
			 *
			 * \~
			 */
			template <> inline constexpr int32_t binary_t <bfloat16_t>::infinitePower() noexcept {
				// Выводим показатель степени бесконечности
				return 0xFF;
			}
			/**
			 * \~russian
			 * @brief Функция получения маски показателя степени
			 *
			 * @return маска показателя степени
			 *
			 *
			 * \~english
			 * @brief Function of getting the exponent mask
			 * @return exponent mask
			 *
			 * \~
			 */
			template <> inline constexpr binary_t <bfloat16_t>::equiv_uint binary_t <bfloat16_t>::exponentMask() noexcept {
				// Выводим маску показателя степени
				return 0x7F80;
			}
			/**
			 * \~russian
			 * @brief Функция получения маски мантиссы
			 *
			 * @return маска мантиссы
			 *
			 *
			 * \~english
			 * @brief Function of getting the mantissa mask
			 * @return mantissa mask
			 *
			 * \~
			 */
			template <> inline constexpr binary_t <bfloat16_t>::equiv_uint binary_t <bfloat16_t>::mantissaMask() noexcept {
				// Выводим маску мантиссы
				return 0x007F;
			}
			/**
			 * \~russian
			 * @brief Функция получения минимального показателя степени двойки
			 *
			 * @return минимальный показатель степени двойки
			 *
			 *
			 * \~english
			 * @brief Function of getting the minimum exponent of a power of two
			 * @return minimum exponent of a power of two
			 *
			 * \~
			 */
			template <> inline constexpr int32_t binary_t <bfloat16_t>::minimumExponent() noexcept {
				// Выводим минимальный показатель степени двойки
				return -127;
			}
			/**
			 * \~russian
			 * @brief Функция получения маски скрытого бита мантиссы
			 *
			 * @return маска скрытого бита мантиссы
			 *
			 *
			 * \~english
			 * @brief Function of getting the mask of the hidden bit of the mantissa
			 * @return mask of the hidden bit of the mantissa
			 *
			 * \~
			 */
			template <> inline constexpr binary_t <bfloat16_t>::equiv_uint binary_t <bfloat16_t>::hiddenBitMask() noexcept {
				// Выводим маску скрытого бита мантиссы
				return 0x0080;
			}
			/**
			 * \~russian
			 * @brief Функция получения наибольшего представимого показателя степени десяти
			 *
			 * @return наибольший представимый показатель степени десяти
			 *
			 *
			 * \~english
			 * @brief Function of getting the largest representable exponent of a power of ten
			 * @return largest representable exponent of a power of ten
			 *
			 * \~
			 */
			template <> inline constexpr int32_t binary_t <bfloat16_t>::largestPowerOfTen() noexcept {
				// Выводим наибольший представимый показатель степени десяти
				return 38;
			}
			/**
			 * \~russian
			 * @brief Функция получения наименьшего представимого показателя степени десяти
			 *
			 * @return наименьший представимый показатель степени десяти
			 *
			 *
			 * \~english
			 * @brief Function of getting the smallest representable exponent of a power of ten
			 * @return smallest representable exponent of a power of ten
			 *
			 * \~
			 */
			template <> inline constexpr int32_t binary_t <bfloat16_t>::smallestPowerOfTen() noexcept {
				// Выводим наименьший представимый показатель степени десяти
				return -60;
			}
			/**
			 * \~russian
			 * @brief Функция получения минимального показателя степени быстрого пути
			 *
			 * @return минимальный показатель степени быстрого пути
			 *
			 *
			 * \~english
			 * @brief Function of getting the minimum exponent of the fast path
			 * @return minimum exponent of the fast path
			 *
			 * \~
			 */
			template <> inline constexpr int32_t binary_t <bfloat16_t>::minExponentFastPath() noexcept {
				// Выводим минимальный показатель степени быстрого пути
				return 0;
			}
			/**
			 * \~russian
			 * @brief Функция получения максимального показателя степени быстрого пути
			 *
			 * @return максимальный показатель степени быстрого пути
			 *
			 *
			 * \~english
			 * @brief Function of getting the maximum exponent of the fast path
			 * @return maximum exponent of the fast path
			 *
			 * \~
			 */
			template <> inline constexpr int32_t binary_t <bfloat16_t>::maxExponentFastPath() noexcept {
				// Выводим максимальный показатель степени быстрого пути
				return 3;
			}
			/**
			 * \~russian
			 * @brief Функция получения количества явно хранимых бит мантиссы
			 *
			 * @return количество явно хранимых бит мантиссы
			 *
			 *
			 * \~english
			 * @brief Function of getting the number of explicitly stored bits of the mantissa
			 * @return number of explicitly stored bits of the mantissa
			 *
			 * \~
			 */
			template <> inline constexpr int32_t binary_t <bfloat16_t>::mantissaExplicitBits() noexcept {
				// Выводим количество явно хранимых бит мантиссы
				return 7;
			}
			/**
			 * \~russian
			 * @brief Функция получения предельной мантиссы быстрого пути
			 *
			 * @return предельное значение мантиссы
			 *
			 *
			 * \~english
			 * @brief Function of getting the limiting mantissa of the fast path
			 * @return limiting value of the mantissa
			 *
			 * \~
			 */
			template <> inline constexpr uint64_t binary_t <bfloat16_t>::maxMantissaFastPath() noexcept {
				// Выводим предельное значение мантиссы быстрого пути
				return (uint64_t(2) << mantissaExplicitBits());
			}
			/**
			 * \~russian
			 * @brief Функция получения минимального показателя степени округления к чётному
			 *
			 * @return минимальный показатель степени округления к чётному
			 *
			 *
			 * \~english
			 * @brief Function of getting the minimum exponent of rounding to even
			 * @return minimum exponent of rounding to even
			 *
			 * \~
			 */
			template <> inline constexpr int32_t binary_t <bfloat16_t>::minExponentRoundToEven() noexcept {
				// Выводим минимальный показатель степени округления к чётному
				return -24;
			}
			/**
			 * \~russian
			 * @brief Функция получения максимального показателя степени округления к чётному
			 *
			 * @return максимальный показатель степени округления к чётному
			 *
			 *
			 * \~english
			 * @brief Function of getting the maximum exponent of rounding to even
			 * @return maximum exponent of rounding to even
			 *
			 * \~
			 */
			template <> inline constexpr int32_t binary_t <bfloat16_t>::maxExponentRoundToEven() noexcept {
				// Выводим максимальный показатель степени округления к чётному
				return 3;
			}
			/**
			 * \~russian
			 * @brief Функция получения точного значения степени десяти
			 *
			 * @param power показатель степени десяти
			 * @return      точное значение степени десяти
			 *
			 *
			 * \~english
			 * @brief Function of getting the exact value of a power of ten
			 * @param power exponent of the power of ten
			 * @return      exact value of the power of ten
			 *
			 * \~
			 */
			template <> inline constexpr bfloat16_t binary_t <bfloat16_t>::exactPowerOfTen(const int64_t power) noexcept {
				// Выводим точное значение степени десяти из таблицы
				return ((void) powersOfTen[0], powersOfTen[power]);
			}
			/**
			 * \~russian
			 * @brief Функция получения предельной мантиссы быстрого пути для степени десяти
			 *
			 * @param power показатель степени десяти
			 * @return      предельное значение мантиссы
			 *
			 *
			 * \~english
			 * @brief Function of getting the limiting mantissa of the fast path for a power of ten
			 * @param power exponent of the power of ten
			 * @return      limiting value of the mantissa
			 *
			 * \~
			 */
			template <> inline constexpr uint64_t binary_t <bfloat16_t>::maxMantissaFastPath(const int64_t power) noexcept {
				// Выводим предельное значение мантиссы из таблицы
				return ((void) maxMantissa[0], maxMantissa[power]);
			}
		#endif

		/**
		 * Если компилятор поддерживает тип float32_t, то определяем его параметры
		 */
		#ifdef __STDCPP_FLOAT32_T__
			/**
			 * \~russian
			 * @brief Структура параметров двоичного формата float32_t
			 *
			 * \~english
			 * @brief Structure of the parameters of the float32_t binary format
			 *
			 * \~
			 */
			template <> struct binary_t <float32_t> : public binary_t <float> {};
		#endif

		/**
		 * Если компилятор поддерживает тип float64_t, то определяем его параметры
		 */
		#ifdef __STDCPP_FLOAT64_T__
			/**
			 * \~russian
			 * @brief Структура параметров двоичного формата float64_t
			 *
			 * \~english
			 * @brief Structure of the parameters of the float64_t binary format
			 *
			 * \~
			 */
			template <> struct binary_t <float64_t> : public binary_t <double> {};
		#endif

		/**
		 * \~russian
		 * @brief Шаблон типа результата сборки числа с плавающей точкой
		 *
		 * @tparam T тип числа с плавающей точкой
		 *
		 *
		 * \~english
		 * @brief Template of the result type of the assembly of a floating-point number
		 * @tparam T floating-point type
		 *
		 * \~
		 */
		template <typename T>
		/**
		 * \~russian
		 * @brief Метод сборки числа с плавающей точкой из знака и мантиссы
		 *
		 * @param negative признак отрицательного числа
		 * @param mantissa скорректированная мантисса двоичного представления
		 * @param value    ссылка на результат сборки
		 *
		 * \~english
		 * @brief Method of assembling a floating-point number from the sign and the mantissa
		 * @param negative indication of a negative number
		 * @param mantissa corrected mantissa of the binary representation
		 * @param value    reference to the result of the assembly
		 *
		 * \~
		 */
		AWH_ASCII_INLINE void toFloat(const bool negative, const mantissa_t & mantissa, T & value) noexcept {
			/**
			 * Создаём тип данных эквивалентного беззнакового целого
			 */
			using equiv_uint = equivUint_t <T>;
			// Формируем биты мантиссы
			equiv_uint word = static_cast <equiv_uint> (mantissa.mantissa);
			// Добавляем биты показателя степени
			word = static_cast <equiv_uint> (word | (static_cast <equiv_uint> (mantissa.power2) << binary_t <T>::mantissaExplicitBits()));
			// Добавляем бит знака
			word = static_cast <equiv_uint> (word | (static_cast <equiv_uint> (negative) << binary_t <T>::signIndex()));
			// Выполняем побитовое копирование результата
			::memcpy(&value, &word, sizeof(T));
		}

		/**
		 * \~russian
		 * @brief Шаблон типа результата разбора двоичного представления
		 *
		 * @tparam T тип числа с плавающей точкой
		 *
		 *
		 * \~english
		 * @brief Template of the result type of the parsing of the binary representation
		 * @tparam T floating-point type
		 *
		 * \~
		 */
		template <typename T>
		/**
		 * \~russian
		 * @brief Метод разбора числа с плавающей точкой в расширенное представление
		 *
		 * @param value разбираемое число с плавающей точкой
		 * @return      скорректированная мантисса расширенной точности
		 *
		 * \~english
		 * @brief Method of parsing a floating-point number into the extended representation
		 * @param value floating-point number being parsed
		 * @return      corrected mantissa of extended precision
		 *
		 * \~
		 */
		AWH_ASCII_INLINE mantissa_t toExtended(const T value) noexcept {
			/**
			 * Создаём тип данных эквивалентного беззнакового целого
			 */
			using equiv_uint = equivUint_t <T>;
			// Результат разбора двоичного представления
			mantissa_t result;
			// Смещение показателя степени относительно минимального
			const int32_t bias = (binary_t <T>::mantissaExplicitBits() - binary_t <T>::minimumExponent());
			// Биты двоичного представления числа
			equiv_uint bits = 0;
			// Выполняем побитовое копирование исходного значения
			::memcpy(&bits, &value, sizeof(T));
			// Если число является денормализованным
			if((bits & binary_t <T>::exponentMask()) == 0){
				// Устанавливаем показатель степени денормализованного числа
				result.power2 = (1 - bias);
				// Устанавливаем мантиссу без скрытого бита
				result.mantissa = (bits & binary_t <T>::mantissaMask());
			// Если число является нормализованным
			} else {
				// Извлекаем показатель степени из двоичного представления
				result.power2 = static_cast <int32_t> ((bits & binary_t <T>::exponentMask()) >> binary_t <T>::mantissaExplicitBits());
				// Приводим показатель степени к минимальному
				result.power2 -= bias;
				// Устанавливаем мантиссу со скрытым битом
				result.mantissa = ((bits & binary_t <T>::mantissaMask()) | binary_t <T>::hiddenBitMask());
			}
			// Выводим результат разбора двоичного представления
			return result;
		}

		/**
		 * \~russian
		 * @brief Шаблон типа исходного числа с плавающей точкой
		 *
		 * @tparam T тип числа с плавающей точкой
		 *
		 *
		 * \~english
		 * @brief Template of the type of the source floating-point number
		 * @tparam T floating-point type
		 *
		 * \~
		 */
		template <typename T>
		/**
		 * \~russian
		 * @brief Метод получения середины между числом и следующим представимым числом
		 *
		 * @param value исходное число с плавающей точкой
		 * @return      скорректированная мантисса середины интервала
		 *
		 * \~english
		 * @brief Method of getting the middle between a number and the next representable number
		 * @param value source floating-point number
		 * @return      corrected mantissa of the middle of the interval
		 *
		 * \~
		 */
		AWH_ASCII_INLINE mantissa_t toExtendedHalfway(const T value) noexcept {
			// Получаем расширенное представление исходного числа
			mantissa_t result = toExtended(value);
			// Компенсируем удвоение мантиссы показателем степени
			result.power2 -= 1;
			// Удваиваем мантиссу для получения дополнительного бита точности
			result.mantissa <<= 1;
			// Добавляем половину единицы младшего разряда
			result.mantissa += 1;
			// Выводим середину интервала
			return result;
		}

		/**
		 * \~russian
		 * @brief Шаблон фиктивного типа подстановки
		 *
		 * @tparam U фиктивный параметр для подстановки при специализации
		 *
		 *
		 * \~english
		 * @brief Template of a dummy substitution type
		 * @tparam U dummy parameter for the substitution at specialisation
		 *
		 * \~
		 */
		template <typename U = void>
		/**
		 * \~russian
		 * @brief Структура таблиц быстрого поиска символов
		 *
		 * \~english
		 * @brief Structure of the fast character lookup tables
		 *
		 * \~
		 */
		struct luts_t {
			/**
			 * \~russian
			 * @brief Таблица признаков пробельных символов ASCII
			 *
			 * \~english
			 * @brief Table of the indications of the ASCII whitespace characters
			 *
			 * \~
			 */
			static constexpr bool spaces[] = {
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
			/**
			 * \~russian
			 * @brief Таблица значений цифр по коду символа
			 *
			 * @details Значение 255 означает, что символ не является цифрой.
			 *
			 * \~english
			 * @brief Table of the digit values by character code
			 * @details The value 255 means that the character is not a digit.
			 *
			 * \~
			 */
			static constexpr uint8_t digits[] = {
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
			/**
			 * \~russian
			 * @brief Таблица максимального количества цифр 64-битного значения по основанию
			 *
			 * \~english
			 * @brief Table of the maximum number of digits of a 64-bit value by base
			 *
			 * \~
			 */
			static constexpr size_t maxDigits[] = {
				64, 41, 32, 28, 25, 23, 22, 21, 20, 19, 18, 18, 17, 17, 16, 16, 16, 16,
				15, 15, 15, 15, 14, 14, 14, 14, 14, 14, 14, 13, 13, 13, 13, 13, 13
			};
		};

		/**
		 * \~russian
		 * @brief Шаблон типа символа проверяемой строки
		 *
		 * @tparam UC тип символа исходной строки
		 *
		 *
		 * \~english
		 * @brief Template of the character type of the checked string
		 * @tparam UC character type of the source string
		 *
		 * \~
		 */
		template <typename UC>
		/**
		 * \~russian
		 * @brief Метод сравнения ASCII-строк без учёта регистра
		 *
		 * @param actual   фактическая строка произвольного регистра
		 * @param expected ожидаемая строка в нижнем регистре
		 * @param length   количество сравниваемых символов
		 * @return         результат сравнения
		 *
		 * \~english
		 * @brief Method of comparing ASCII strings case-insensitively
		 * @param actual   actual string of an arbitrary case
		 * @param expected expected string in lower case
		 * @param length   number of compared characters
		 * @return         result of the comparison
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr bool compareIgnoreCase(const UC * actual, const UC * expected, const size_t length) noexcept {
			/**
			 * Выполняем перебор всех сравниваемых символов
			 */
			for(size_t i = 0; i < length; ++i){
				// Получаем очередной символ фактической строки
				const UC symbol = actual[i];
				// Если символ в нижнем регистре не совпадает с ожидаемым
				if(((symbol < 256) ? (symbol | 32) : symbol) != expected[i])
					// Сообщаем, что строки не совпадают
					return false;
			}
			// Сообщаем, что строки совпадают
			return true;
		}

		/**
		 * \~russian
		 * @brief Шаблон типа проверяемого символа
		 *
		 * @tparam UC тип символа исходной строки
		 *
		 *
		 * \~english
		 * @brief Template of the type of the checked character
		 * @tparam UC character type of the source string
		 *
		 * \~
		 */
		template <typename UC>
		/**
		 * \~russian
		 * @brief Метод проверки символа на принадлежность к пробельным
		 *
		 * @param symbol проверяемый символ
		 * @return       результат проверки
		 *
		 * \~english
		 * @brief Method of checking whether a character belongs to the whitespace ones
		 * @param symbol character to check
		 * @return       result of the check
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr bool isSpace(const UC symbol) noexcept {
			// Выполняем проверку по таблице пробельных символов
			return ((symbol < 256) && luts_t <>::spaces[static_cast <uint8_t> (symbol)]);
		}

		/**
		 * \~russian
		 * @brief Шаблон типа проверяемого символа
		 *
		 * @tparam UC тип символа исходной строки
		 *
		 *
		 * \~english
		 * @brief Template of the type of the checked character
		 * @tparam UC character type of the source string
		 *
		 * \~
		 */
		template <typename UC>
		/**
		 * \~russian
		 * @brief Метод проверки символа на принадлежность к десятичным цифрам
		 *
		 * @param symbol проверяемый символ
		 * @return       результат проверки
		 *
		 * \~english
		 * @brief Method of checking whether a character belongs to the decimal digits
		 * @param symbol character to check
		 * @return       result of the check
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr bool isDigit(const UC symbol) noexcept {
			// Выполняем проверку попадания символа в диапазон десятичных цифр
			return !((symbol > UC('9')) || (symbol < UC('0')));
		}

		/**
		 * \~russian
		 * @brief Шаблон типа преобразуемого символа
		 *
		 * @tparam UC тип символа исходной строки
		 *
		 *
		 * \~english
		 * @brief Template of the type of the converted character
		 * @tparam UC character type of the source string
		 *
		 * \~
		 */
		template <typename UC>
		/**
		 * \~russian
		 * @brief Метод преобразования символа в значение цифры
		 *
		 * @param symbol преобразуемый символ
		 * @return       значение цифры или 255 если символ цифрой не является
		 *
		 * \~english
		 * @brief Method of converting a character into a digit value
		 * @param symbol character being converted
		 * @return       digit value or 255 if the character is not a digit
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr uint8_t charToDigit(const UC symbol) noexcept {
			/**
			 * Создаём тип данных беззнакового символа
			 */
			using unsigned_uc = typename make_unsigned <UC>::type;
			// Приводим символ к беззнаковому представлению
			const unsigned_uc value = static_cast <unsigned_uc> (symbol);
			// Формируем маску, обнуляющую символы за пределами таблицы
			const unsigned_uc mask = static_cast <unsigned_uc> (-static_cast <int32_t> ((value & ~static_cast <uint64_t> (0xFF)) == 0));
			// Выводим значение цифры из таблицы
			return luts_t <>::digits[static_cast <uint8_t> (value & mask)];
		}

		/**
		 * \~russian
		 * @brief Метод получения максимального количества цифр 64-битного значения
		 *
		 * @param base основание системы счисления в диапазоне от 2 до 36
		 * @return     максимальное количество цифр
		 *
		 * \~english
		 * @brief Method of getting the maximum number of digits of a 64-bit value
		 * @param base base of the numeral system in the range from 2 to 36
		 * @return     maximum number of digits
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr size_t maxDigitsU64(const int32_t base) noexcept {
			// Выводим максимальное количество цифр из таблицы
			return luts_t <>::maxDigits[base - 2];
		}

		/**
		 * \~russian
		 * @brief Шаблон типа символа константной строки
		 *
		 * @tparam UC тип символа исходной строки
		 *
		 *
		 * \~english
		 * @brief Template of the character type of the constant string
		 * @tparam UC character type of the source string
		 *
		 * \~
		 */
		template <typename UC>
		/**
		 * \~russian
		 * @brief Метод получения строковой константы нечислового значения
		 *
		 * @return строковая константа нечислового значения
		 *
		 *
		 * \~english
		 * @brief Method of getting the string constant of a not-a-number value
		 * @return string constant of a not-a-number value
		 *
		 * \~
		 */
		constexpr const UC * constNan() noexcept;

		/**
		 * \~russian
		 * @brief Метод получения строковой константы нечислового значения
		 *
		 * @return строковая константа нечислового значения
		 *
		 *
		 * \~english
		 * @brief Method of getting the string constant of a not-a-number value
		 * @return string constant of a not-a-number value
		 *
		 * \~
		 */
		template <> inline constexpr const char * constNan <char> () noexcept {
			// Выводим строковую константу нечислового значения
			return "nan";
		}
		/**
		 * \~russian
		 * @brief Метод получения строковой константы нечислового значения (широкий символ)
		 *
		 * @return строковая константа нечислового значения
		 *
		 * \~english
		 * @brief Method of getting the string constant of a not-a-number value (wide character)
		 * @return string constant of a not-a-number value
		 *
		 * \~
		 */
		template <> inline constexpr const wchar_t * constNan <wchar_t> () noexcept {
			// Выводим строковую константу нечислового значения (широкий символ)
			return L"nan";
		}
		/**
		 * \~russian
		 * @brief Метод получения строковой константы нечислового значения (16-битный символ)
		 *
		 * @return строковая константа нечислового значения
		 *
		 * \~english
		 * @brief Method of getting the string constant of a not-a-number value (16-bit character)
		 * @return string constant of a not-a-number value
		 *
		 * \~
		 */
		template <> inline constexpr const char16_t * constNan <char16_t> () noexcept {
			// Выводим строковую константу нечислового значения (16-битный символ)
			return u"nan";
		}
		/**
		 * \~russian
		 * @brief Метод получения строковой константы нечислового значения (32-битный символ)
		 *
		 * @return строковая константа нечислового значения
		 *
		 * \~english
		 * @brief Method of getting the string constant of a not-a-number value (32-bit character)
		 * @return string constant of a not-a-number value
		 *
		 * \~
		 */
		template <> inline constexpr const char32_t * constNan <char32_t> () noexcept {
			// Выводим строковую константу нечислового значения (32-битный символ)
			return U"nan";
		}
		/**
		 * Если компилятор поддерживает тип char8_t, то определяем его параметры
		 */
		#ifdef __cpp_char8_t
			/**
			 * \~russian
			 * @brief Метод получения строковой константы нечислового значения (8-битный символ)
			 *
			 * @return строковая константа нечислового значения
			 *
			 * \~english
			 * @brief Method of getting the string constant of a not-a-number value (8-bit character)
			 * @return string constant of a not-a-number value
			 *
			 * \~
			 */
			template <> inline constexpr const char8_t * constNan <char8_t> () noexcept {
				// Выводим строковую константу нечислового значения (8-битный символ)
				return u8"nan";
			}
		#endif

		/**
		 * \~russian
		 * @brief Шаблон типа символа константной строки
		 *
		 * @tparam UC тип символа исходной строки
		 *
		 *
		 * \~english
		 * @brief Template of the character type of the constant string
		 * @tparam UC character type of the source string
		 *
		 * \~
		 */
		template <typename UC>
		/**
		 * \~russian
		 * @brief Метод получения строковой константы бесконечности
		 *
		 * @return строковая константа бесконечности
		 *
		 *
		 * \~english
		 * @brief Method of getting the string constant of infinity
		 * @return string constant of infinity
		 *
		 * \~
		 */
		constexpr const UC * constInf() noexcept;
		/**
		 * \~russian
		 * @brief Метод получения строковой константы бесконечности
		 *
		 * @return строковая константа бесконечности
		 *
		 *
		 * \~english
		 * @brief Method of getting the string constant of infinity
		 * @return string constant of infinity
		 *
		 * \~
		 */
		template <> inline constexpr const char * constInf <char> () noexcept {
			// Выводим строковую константу бесконечности
			return "infinity";
		}
		/**
		 * \~russian
		 * @brief Метод получения строковой константы бесконечности (широкий символ)
		 *
		 * @return строковая константа бесконечности
		 *
		 * \~english
		 * @brief Method of getting the string constant of infinity (wide character)
		 * @return string constant of infinity
		 *
		 * \~
		 */
		template <> inline constexpr const wchar_t * constInf <wchar_t> () noexcept {
			// Выводим строковую константу бесконечности (широкий символ)
			return L"infinity";
		}
		/**
		 * \~russian
		 * @brief Метод получения строковой константы бесконечности (16-битный символ)
		 *
		 * @return строковая константа бесконечности
		 *
		 * \~english
		 * @brief Method of getting the string constant of infinity (16-bit character)
		 * @return string constant of infinity
		 *
		 * \~
		 */
		template <> inline constexpr const char16_t * constInf <char16_t> () noexcept {
			// Выводим строковую константу бесконечности (16-битный символ)
			return u"infinity";
		}
		/**
		 * \~russian
		 * @brief Метод получения строковой константы бесконечности (32-битный символ)
		 *
		 * @return строковая константа бесконечности
		 *
		 * \~english
		 * @brief Method of getting the string constant of infinity (32-bit character)
		 * @return string constant of infinity
		 *
		 * \~
		 */
		template <> inline constexpr const char32_t * constInf <char32_t> () noexcept {
			// Выводим строковую константу бесконечности (32-битный символ)
			return U"infinity";
		}
		/**
		 * Если компилятор поддерживает тип char8_t, то определяем его параметры
		 */
		#ifdef __cpp_char8_t
			/**
			 * \~russian
			 * @brief Метод получения строковой константы бесконечности (8-битный символ)
			 *
			 * @return строковая константа бесконечности
			 *
			 * \~english
			 * @brief Method of getting the string constant of infinity (8-bit character)
			 * @return string constant of infinity
			 *
			 * \~
			 */
			template <> inline constexpr const char8_t * constInf <char8_t> () noexcept {
				// Выводим строковую константу бесконечности (8-битный символ)
				return u8"infinity";
			}
		#endif

		/**
		 * Выполняем проверку соответствия платформы требованиям модуля
		 */
		// Выводим статическую проверку соответствия типа float размеру 32 бита
		static_assert(is_same <equivUint_t <float>, uint32_t>::value, "AWH lexical: float must be 32-bit wide");
		// Выполняем проверку соответствия платформы требованиям модуля
		static_assert(is_same <equivUint_t <double>, uint64_t>::value, "AWH lexical: double must be 64-bit wide");
		// Выводим статическую проверку соответствия типа float стандарту IEEE 754
		static_assert(numeric_limits <float>::is_iec559, "AWH lexical: float must fulfill IEC 559 (IEEE 754)");
		// Выводим статическую проверку соответствия типа double стандарту IEEE 754
		static_assert(numeric_limits <double>::is_iec559, "AWH lexical: double must fulfill IEC 559 (IEEE 754)");
		
		/**
		 * Если компилятор поддерживает тип float32_t, то выполняем проверку соответствия платформы требованиям модуля
		 */
		#ifdef __STDCPP_FLOAT32_T__
			// Выводим статическую проверку соответствия типа float32_t стандарту IEEE 754
			static_assert(numeric_limits <float32_t>::is_iec559, "AWH lexical: float32_t must fulfill IEC 559 (IEEE 754)");
		#endif
		/**
		 * Если компилятор поддерживает тип float64_t, то выполняем проверку соответствия платформы требованиям модуля
		 */
		#ifdef __STDCPP_FLOAT64_T__
			// Выводим статическую проверку соответствия типа float64_t стандарту IEEE 754
			static_assert(numeric_limits <float64_t>::is_iec559, "AWH lexical: float64_t must fulfill IEC 559 (IEEE 754)");
		#endif
		/**
		 * Если компилятор поддерживает тип float16_t, то выполняем проверку соответствия платформы требованиям модуля
		 */
		#ifdef __STDCPP_FLOAT16_T__
			// Выводим статическую проверку соответствия типа float16_t стандарту IEEE 754
			static_assert(numeric_limits <float16_t>::is_iec559, "AWH lexical: float16_t must fulfill IEC 559 (IEEE 754)");
		#endif
		/**
		 * Если компилятор поддерживает тип bfloat16_t, то выполняем проверку соответствия платформы требованиям модуля
		 */
		#ifdef __STDCPP_BFLOAT16_T__
			// Выводим статическую проверку соответствия типа bfloat16_t стандарту IEEE 754
			static_assert(numeric_limits <bfloat16_t>::is_iec559, "AWH lexical: bfloat16_t must fulfill IEC 559 (IEEE 754)");
		#endif
	};
};

#endif // __AWH_LEXICAL_COMMON__
