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
 * @brief Заголовочный файл проверок и преобразований символов таблицы ASCII —
 *        встраиваемые замены библиотечных функций <cctype> для разбора
 *        протокольных данных
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
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
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
	 */
	namespace ascii {
		/**
		 * @brief Функция проверки пробельного символа
		 *
		 * @param letter проверяемый символ
		 * @return       результат проверки
		 *
		 * @details Пробельным набором считается набор основной локали: пробел и
		 *          пятёрка управляющих символов от табуляции до возврата
		 *          каретки, идущих в таблице подряд.
		 *
		 */
		AWH_ASCII_INLINE constexpr bool isSpace(const char letter) noexcept {
			// Возвращаем результат проверки
			return ((letter == ' ') || ((letter >= '\t') && (letter <= '\r')));
		}
		/**
		 * @brief Функция проверки десятичного символа
		 *
		 * @param letter проверяемый символ
		 * @return       результат проверки
		 *
		 */
		AWH_ASCII_INLINE constexpr bool isDigit(const char letter) noexcept {
			// Возвращаем результат проверки
			return ((letter >= '0') && (letter <= '9'));
		}
		/**
		 * @brief Функция проверки восьмеричного символа
		 *
		 * @param letter проверяемый символ
		 * @return       результат проверки
		 *
		 */
		AWH_ASCII_INLINE constexpr bool isOctal(const char letter) noexcept {
			// Возвращаем результат проверки
			return ((letter >= '0') && (letter <= '7'));
		}
		/**
		 * @brief Функция проверки шестнадцатеричного символа
		 *
		 * @param letter проверяемый символ
		 * @return       результат проверки
		 *
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
		 * @brief Функция проверки прописного символа
		 *
		 * @param letter проверяемый символ
		 * @return       результат проверки
		 *
		 */
		AWH_ASCII_INLINE constexpr bool isUpper(const char letter) noexcept {
			// Возвращаем результат проверки
			return ((letter >= 'A') && (letter <= 'Z'));
		}
		/**
		 * @brief Функция проверки строчного символа
		 *
		 * @param letter проверяемый символ
		 * @return       результат проверки
		 *
		 */
		AWH_ASCII_INLINE constexpr bool isLower(const char letter) noexcept {
			// Возвращаем результат проверки
			return ((letter >= 'a') && (letter <= 'z'));
		}
		/**
		 * @brief Функция проверки буквенного символа
		 *
		 * @param letter проверяемый символ
		 * @return       результат проверки
		 *
		 */
		AWH_ASCII_INLINE constexpr bool isAlpha(const char letter) noexcept {
			// Возвращаем результат проверки
			return (isUpper(letter) || isLower(letter));
		}
		/**
		 * @brief Функция проверки буквенно-цифрового символа
		 *
		 * @param letter проверяемый символ
		 * @return       результат проверки
		 *
		 */
		AWH_ASCII_INLINE constexpr bool isAlnum(const char letter) noexcept {
			// Возвращаем результат проверки
			return (isAlpha(letter) || isDigit(letter));
		}
		/**
		 * @brief Функция проверки печатного символа
		 *
		 * @param letter проверяемый символ
		 * @return       результат проверки
		 *
		 * @details Печатными считаются символы от пробела до тильды
		 *          включительно - весь видимый участок таблицы ASCII.
		 *
		 */
		AWH_ASCII_INLINE constexpr bool isPrint(const char letter) noexcept {
			// Возвращаем результат проверки
			return ((letter >= ' ') && (letter <= '~'));
		}
		/**
		 * @brief Функция приведения символа к прописному виду
		 *
		 * @param letter приводимый символ
		 * @return       приведённый символ
		 *
		 */
		AWH_ASCII_INLINE constexpr char toUpper(const char letter) noexcept {
			// Возвращаем приведённый символ
			return (isLower(letter) ? static_cast <char> (letter - ('a' - 'A')) : letter);
		}
		/**
		 * @brief Функция приведения символа к строчному виду
		 *
		 * @param letter приводимый символ
		 * @return       приведённый символ
		 *
		 */
		AWH_ASCII_INLINE constexpr char toLower(const char letter) noexcept {
			// Возвращаем приведённый символ
			return (isUpper(letter) ? static_cast <char> (letter + ('a' - 'A')) : letter);
		}
		/**
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
		 */
		AWH_ASCII_INLINE constexpr bool equals(const char first, const char second) noexcept {
			// Возвращаем результат сравнения
			return ((first == second) || (toLower(first) == toLower(second)));
		}
		/**
		 * @brief Функция получения числового значения шестнадцатеричного символа
		 *
		 * @param letter преобразуемый символ
		 * @return       числовое значение символа либо -1 при ошибке
		 *
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
