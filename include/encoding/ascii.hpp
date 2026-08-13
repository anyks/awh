/**
 * @file: ascii.hpp
 * @date: 2026-07-29
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл проверок и преобразований символов таблицы ASCII —
 *        встраиваемые замены библиотечных функций <cctype> для разбора
 *        протокольных данных
 *
 * \~english
 * @brief Header file of the checks and transformations of the characters of the ASCII table —
 *        inlineable replacements of the <cctype> library functions for parsing
 *        protocol data
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_ASCII__
#define __AWH_ASCII__

/**
 * Стандартные заголовочные файлы
 */
#include <cstdint>

/**
 * Если компилятор принадлежит к семейству Visual Studio
 */
#if defined(_MSC_VER)
	/**
	 * Принудительная подстановка средствами Visual Studio
	 */
	#define AWH_ASCII_INLINE __forceinline
/**
 * Если компилятор принадлежит к семейству GCC или Clang
 */
#else
	/**
	 * Принудительная подстановка средствами GCC и Clang
	 */
	#define AWH_ASCII_INLINE inline __attribute__((always_inline))
#endif

/**
 * \~russian
 * @brief Основное пространство имён
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * \~russian
	 * @brief Пространство имён проверок символов таблицы ASCII
	 *
	 * @details Библиотечные проверки символов из <cctype> смотрят на текущую
	 *          локаль, а локаль эту фреймворк устанавливает сам, в конструкторе,
	 *          и по умолчанию она "en_US.UTF-8" против "C" под MS Windows.
	 *          Протокольные же данные - имена заголовков HTTP, адреса, схемы
	 *          URI, числа - определены стандартами в таблице ASCII и только в
	 *          ней, и смысл их от настроек приложения зависеть не должен.
	 *
	 *          Зависимость эта не умозрительная. Байт 0xA0, неразрывный пробел
	 *          кодировки Latin-1, в локали "en_US.UTF-8" считается пробельным, а
	 *          в локали "C" - нет; строка "1.2.3.4\xA0" из-за этого разбиралась
	 *          как годный IPv4-адрес на одной платформе и не разбиралась на
	 *          другой. Приведение регистра ловушкой известнее: в турецкой локали
	 *          прописная "I" в строчную "i" не переходит, и сравнение имён
	 *          заголовков HTTP без учёта регистра переставало их узнавать.
	 *
	 *          Вторая причина - цена. Библиотечные проверки встраиванию не
	 *          поддаются, а стоят они на пути каждого разбора: сравнение имён
	 *          заголовков вызывает приведение регистра на каждый символ, обрезка
	 *          пробелов - проверку на каждый символ с обоих концов строки.
	 *          Обрезка строки в тринадцать символов обходилась дороже, чем весь
	 *          остальной разбор IPv4-адреса.
	 *
	 * @note Проверки здесь намеренно не покрывают символы старше 0x7F: они не
	 *       буквы, не цифры и не пробелы ни в одной кодировке, к которой
	 *       применимо понятие ASCII. Там, где нужна работа с текстом человека, а
	 *       не с протокольными данными, применяются широкие символы и
	 *       библиотечные проверки для них - те локаль учитывать обязаны
	 *
	 * \~english
	 * @brief Namespace of the checks of the characters of the ASCII table
	 *
	 * @details The library character checks from <cctype> look at the current locale, and that
	 *          locale is set by the framework itself, in the constructor, and by default it is
	 *          "en_US.UTF-8" versus "C" under MS Windows. Protocol data, however - HTTP header
	 *          names, addresses, URI schemes, numbers - are defined by the standards within the
	 *          ASCII table and within it alone, and their meaning must not depend on the settings
	 *          of the application.
	 *
	 *          This dependency is not speculative. The byte 0xA0, the non-breaking space of the
	 *          Latin-1 encoding, is considered whitespace in the "en_US.UTF-8" locale, and is not
	 *          in the "C" locale; because of that the string "1.2.3.4\xA0" was parsed as a valid
	 *          IPv4 address on one platform and was not parsed on another. Case folding is better
	 *          known as a trap: in the Turkish locale the upper-case "I" does not turn into the
	 *          lower-case "i", and the case-insensitive comparison of HTTP header names stopped
	 *          recognizing them.
	 *
	 *          The second reason is the cost. The library checks do not lend themselves to inlining,
	 *          and they stand in the way of every parse: comparing header names invokes case folding
	 *          for every character, trimming whitespace - a check for every character from both ends
	 *          of the string. Trimming a string of thirteen characters cost more than the whole
	 *          remaining parse of an IPv4 address.
	 *
	 * @note The checks here deliberately do not cover characters above 0x7F: they are neither
	 *       letters, nor digits, nor spaces in any encoding to which the notion of ASCII is
	 *       applicable. Where work with human text is needed rather than with protocol data,
	 *       wide characters and the library checks for them are used - those are obliged to take
	 *       the locale into account
	 *
	 * \~
	 */
	namespace ascii {
		/**
		 * \~russian
		 * @brief Функция проверки пробельного символа
		 *
		 * @param letter проверяемый символ
		 * @return       результат проверки
		 *
		 * @details Пробельным набором считается набор основной локали: пробел и
		 *          пятёрка управляющих символов от табуляции до возврата
		 *          каретки, идущих в таблице подряд.
		 *
		 * \~english
		 * @brief Whitespace character check function
		 *
		 * @param letter character being checked
		 * @return       check result
		 *
		 * @details The whitespace set is taken to be the set of the basic locale: the space and
		 *          the five control characters from the tab to the carriage return, which go
		 *          consecutively in the table.
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr bool isSpace(const char letter) noexcept {
			// Возвращаем результат проверки
			return ((letter == ' ') || ((letter >= '\t') && (letter <= '\r')));
		}
		/**
		 * \~russian
		 * @brief Функция проверки десятичного символа
		 *
		 * @param letter проверяемый символ
		 * @return       результат проверки
		 *
		 * \~english
		 * @brief Decimal character check function
		 *
		 * @param letter character being checked
		 * @return       check result
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr bool isDigit(const char letter) noexcept {
			// Возвращаем результат проверки
			return ((letter >= '0') && (letter <= '9'));
		}
		/**
		 * \~russian
		 * @brief Функция проверки восьмеричного символа
		 *
		 * @param letter проверяемый символ
		 * @return       результат проверки
		 *
		 * \~english
		 * @brief Octal character check function
		 *
		 * @param letter character being checked
		 * @return       check result
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr bool isOctal(const char letter) noexcept {
			// Возвращаем результат проверки
			return ((letter >= '0') && (letter <= '7'));
		}
		/**
		 * \~russian
		 * @brief Функция проверки шестнадцатеричного символа
		 *
		 * @param letter проверяемый символ
		 * @return       результат проверки
		 *
		 * \~english
		 * @brief Hexadecimal character check function
		 *
		 * @param letter character being checked
		 * @return       check result
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr bool isHex(const char letter) noexcept {
			// Возвращаем результат проверки
			return (
				((letter >= '0') && (letter <= '9')) ||
				((letter >= 'a') && (letter <= 'f')) ||
				((letter >= 'A') && (letter <= 'F'))
			);
		}
		/**
		 * \~russian
		 * @brief Функция проверки прописного символа
		 *
		 * @param letter проверяемый символ
		 * @return       результат проверки
		 *
		 * \~english
		 * @brief Upper-case character check function
		 *
		 * @param letter character being checked
		 * @return       check result
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr bool isUpper(const char letter) noexcept {
			// Возвращаем результат проверки
			return ((letter >= 'A') && (letter <= 'Z'));
		}
		/**
		 * \~russian
		 * @brief Функция проверки строчного символа
		 *
		 * @param letter проверяемый символ
		 * @return       результат проверки
		 *
		 * \~english
		 * @brief Lower-case character check function
		 *
		 * @param letter character being checked
		 * @return       check result
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr bool isLower(const char letter) noexcept {
			// Возвращаем результат проверки
			return ((letter >= 'a') && (letter <= 'z'));
		}
		/**
		 * \~russian
		 * @brief Функция проверки буквенного символа
		 *
		 * @param letter проверяемый символ
		 * @return       результат проверки
		 *
		 * \~english
		 * @brief Alphabetic character check function
		 *
		 * @param letter character being checked
		 * @return       check result
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr bool isAlpha(const char letter) noexcept {
			// Возвращаем результат проверки
			return (isUpper(letter) || isLower(letter));
		}
		/**
		 * \~russian
		 * @brief Функция проверки буквенно-цифрового символа
		 *
		 * @param letter проверяемый символ
		 * @return       результат проверки
		 *
		 * \~english
		 * @brief Alphanumeric character check function
		 *
		 * @param letter character being checked
		 * @return       check result
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr bool isAlnum(const char letter) noexcept {
			// Возвращаем результат проверки
			return (isAlpha(letter) || isDigit(letter));
		}
		/**
		 * \~russian
		 * @brief Функция проверки печатного символа
		 *
		 * @param letter проверяемый символ
		 * @return       результат проверки
		 *
		 * @details Печатными считаются символы от пробела до тильды
		 *          включительно - весь видимый участок таблицы ASCII.
		 *
		 * \~english
		 * @brief Printable character check function
		 *
		 * @param letter character being checked
		 * @return       check result
		 *
		 * @details Printable are taken to be the characters from the space to the tilde
		 *          inclusive - the whole visible part of the ASCII table.
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr bool isPrint(const char letter) noexcept {
			// Возвращаем результат проверки
			return ((letter >= ' ') && (letter <= '~'));
		}
		/**
		 * \~russian
		 * @brief Функция приведения символа к прописному виду
		 *
		 * @param letter приводимый символ
		 * @return       приведённый символ
		 *
		 * \~english
		 * @brief Function bringing a character to the upper-case form
		 *
		 * @param letter character being brought
		 * @return       brought character
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr char toUpper(const char letter) noexcept {
			// Возвращаем приведённый символ
			return (isLower(letter) ? static_cast <char> (letter - ('a' - 'A')) : letter);
		}
		/**
		 * \~russian
		 * @brief Функция приведения символа к строчному виду
		 *
		 * @param letter приводимый символ
		 * @return       приведённый символ
		 *
		 * \~english
		 * @brief Function bringing a character to the lower-case form
		 *
		 * @param letter character being brought
		 * @return       brought character
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr char toLower(const char letter) noexcept {
			// Возвращаем приведённый символ
			return (isUpper(letter) ? static_cast <char> (letter + ('a' - 'A')) : letter);
		}
		/**
		 * \~russian
		 * @brief Функция сравнения двух символов без учёта регистра
		 *
		 * @param first  первый сравниваемый символ
		 * @param second второй сравниваемый символ
		 * @return       результат сравнения
		 *
		 * @details Совпадающие символы отсеиваются до приведения регистра:
		 *          сравнение без учёта регистра чаще всего применяется к
		 *          строкам, совпадающим посимвольно, и приведение им не нужно.
		 *
		 * \~english
		 * @brief Function comparing two characters case-insensitively
		 *
		 * @param first  first character being compared
		 * @param second second character being compared
		 * @return       comparison result
		 *
		 * @details Coinciding characters are sifted out before case folding: case-insensitive
		 *          comparison is most often applied to strings coinciding character by character,
		 *          and they have no need of folding.
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr bool equals(const char first, const char second) noexcept {
			// Возвращаем результат сравнения
			return ((first == second) || (toLower(first) == toLower(second)));
		}
		/**
		 * \~russian
		 * @brief Функция получения числового значения шестнадцатеричного символа
		 *
		 * @param letter преобразуемый символ
		 * @return       числовое значение символа либо -1 при ошибке
		 *
		 * \~english
		 * @brief Function obtaining the numeric value of a hexadecimal character
		 *
		 * @param letter character being converted
		 * @return       numeric value of the character or -1 on error
		 *
		 * \~
		 */
		AWH_ASCII_INLINE constexpr int32_t hexValue(const char letter) noexcept {
			// Возвращаем числовое значение символа
			return (
				((letter >= '0') && (letter <= '9')) ? (letter - '0') : (
					((letter >= 'a') && (letter <= 'f')) ? ((letter - 'a') + 10) : (
						((letter >= 'A') && (letter <= 'F')) ? ((letter - 'A') + 10) : -1
					)
				)
			);
		}
	};
};

#endif // __AWH_ASCII__
