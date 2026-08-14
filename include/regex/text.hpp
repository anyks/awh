/**
 * @file text.hpp
 * @date 2026-07-31
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
 * @brief Заголовочный файл операций над текстом сопоставления — извлечение кодового
 *        значения символа, проверка привязок к позиции в тексте и проверка принадлежности
 *        символа классу символов, общие для всех способов исполнения регулярного выражения
 *
 * @section text_decisions Намеренные решения
 *
 * @details Перечисленное ниже расходится с поведением эталонной реализации PCRE2,
 *          но выбрано осознанно и правке не подлежит. Раздел заведён затем, чтобы
 *          разбор кода не начинался каждый раз с одних и тех же выводов.
 *
 *          <b>Разбиение на графемные кластеры следует действующей редакции
 *          приложения UAX #29 стандарта Юникода</b>, а не таблице разбиения PCRE2.
 *          Расхождения затрагивают три правила: предшествующий знак присоединяет
 *          изобразительный символ (GB9b), указатель области присоединяет
 *          продолжающие символы, отступающие знаки и соединитель (GB9), а согласные
 *          индийских письменностей, соединённые вирамой, не разделяются (GB9c).
 *          PCRE2 первых двух правил не содержит в таблице, а третье появилось
 *          в стандарте позже и в разбиении PCRE2 не участвует вовсе, хотя свойство
 *          Indic_Conjunct_Break этой реализацией и распознаётся. Разбиение текста
 *          на воспринимаемые читателем знаки - обязательство перед стандартом,
 *          а не перед сторонней реализацией.
 *
 *          Классификация символов по свойству Grapheme_Cluster_Break при этом
 *          совпадает с PCRE2 в точности: правила разбиения PCRE2, применённые
 *          к таблицам этого модуля, воспроизводят его вывод на всех кодовых
 *          значениях без единого расхождения.
 *
 * \~english
 * @brief Header file of the operations on the matched text — getting the code point
 *        value of a character, checking the anchors to a position in the text and checking whether
 *        a character belongs to a character class, common to all ways of executing a regular expression
 * @section text_decisions Deliberate decisions
 * @details What is listed below diverges from the behaviour of the reference PCRE2 implementation,
 *          but was chosen deliberately and is not subject to correction. The section is introduced so that
 *          reading the code does not start every time from the same conclusions.
 *          <b>Splitting into grapheme clusters follows the current revision
 *          of annex UAX #29 of the Unicode standard</b> rather than the PCRE2 splitting table.
 *          The divergences affect three rules: a preceding mark joins
 *          a pictographic character (GB9b), a regional indicator joins
 *          continuing characters, spacing marks and the joiner (GB9), and consonants
 *          of the Indic scripts joined by a virama are not separated (GB9c).
 *          PCRE2 does not have the first two rules in its table, and the third one appeared
 *          in the standard later and does not take part in the PCRE2 splitting at all, although the
 *          Indic_Conjunct_Break property is recognised by that implementation.
 *          Splitting the text into signs perceived by the reader is an obligation to the standard,
 *          not to a third-party implementation.
 *          The classification of characters by the Grapheme_Cluster_Break property at the same time
 *          matches PCRE2 exactly: the PCRE2 splitting rules applied
 *          to the tables of this module reproduce its output on all code point
 *          values without a single divergence.
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_REGEX_TEXT__
#define __AWH_REGEX_TEXT__

/**
 * Стандартные заголовочные файлы
 */
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"

/**
 * Если используется компилятор Microsoft Visual C++
 */
#if defined(_MSC_VER)
	/**
	 * Принудительная подстановка средствами Microsoft Visual C++
	 */
	#define AWH_REGEX_INLINE __forceinline
/**
 * Если компилятор принадлежит к семейству GCC или Clang
 */
#else
	/**
	 * Принудительная подстановка средствами GCC и Clang
	 */
	#define AWH_REGEX_INLINE inline __attribute__((always_inline))
#endif

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
	 * @brief Пространство имён модуля регулярных выражений
	 *
	 * \~english
	 * @brief Namespace of the regular expression module
	 *
	 * \~
	 */
	namespace regex {
		/**
		 * \~russian
		 * @brief Функция проверки установки режима компиляции
		 *
		 * @param flags набор режимов компиляции регулярного выражения
		 * @param value проверяемый режим компиляции регулярного выражения
		 * @return      результат проверки установки режима компиляции
		 *
		 * \~english
		 * @brief Function of checking whether a compilation mode is set
		 * @param flags set of compilation modes of the regular expression
		 * @param value compilation mode of the regular expression to check
		 * @return      result of checking whether the compilation mode is set
		 *
		 * \~
		 */
		AWH_REGEX_INLINE bool hasFlag(const uint32_t flags, const flag_t value) noexcept {
			// Выполняем проверку установки режима компиляции
			return ((flags & static_cast <uint32_t> (value)) != 0);
		}
		/**
		 * \~russian
		 * @brief Функция приведения кодового значения символа к нижнему регистру
		 *
		 * @details Приведение выполняется для символов набора ASCII. Приведение символов
		 *          за пределами набора ASCII требует таблиц свойств Юникода.
		 *
		 * @param code кодовое значение приводимого символа
		 * @return     приведённое кодовое значение символа
		 *
		 * \~english
		 * @brief Function of converting the code point value of a character to lower case
		 * @details The conversion is performed for the characters of the ASCII set. Converting characters
		 *          beyond the ASCII set requires the Unicode property tables.
		 * @param code code point value of the character to convert
		 * @return     converted code point value of the character
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ uint32_t fold(const uint32_t code) noexcept;
		/**
		 * \~russian
		 * @brief Функция приведения кодового значения символа к нижнему регистру
		 *
		 * @details В режимах «UTF» и «UCP» приведение выполняется по таблицам
		 *          приведения регистра стандарта Юникода, иначе ограничивается
		 *          символами набора ASCII.
		 *
		 * @param code  кодовое значение приводимого символа
		 * @param flags набор режимов компиляции инструкции
		 * @return      приведённое кодовое значение символа
		 *
		 * \~english
		 * @brief Function of converting the code point value of a character to lower case
		 * @details In the «UTF» and «UCP» modes the conversion is performed by the case
		 *          conversion tables of the Unicode standard, otherwise it is limited to
		 *          the characters of the ASCII set.
		 * @param code  code point value of the character to convert
		 * @param flags set of compilation modes of the instruction
		 * @return      converted code point value of the character
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ uint32_t fold(const uint32_t code, const uint32_t flags) noexcept;
		/**
		 * \~russian
		 * @brief Функция проверки принадлежности символа символам слова
		 *
		 * @param code кодовое значение проверяемого символа
		 * @return     результат проверки принадлежности символа символам слова
		 *
		 * \~english
		 * @brief Function of checking whether a character belongs to the word characters
		 * @param code code point value of the character to check
		 * @return     result of checking whether the character belongs to the word characters
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool isWord(const uint32_t code) noexcept;
		/**
		 * \~russian
		 * @brief Функция извлечения кодового значения символа в позиции текста
		 *
		 * @details В режиме «UTF» функция разбирает последовательность UTF-8 целиком,
		 *          иначе возвращает кодовое значение одиночного байта. Некорректная
		 *          последовательность разбирается как одиночный байт.
		 *
		 * @param text  текст для сопоставления
		 * @param pos   позиция символа в тексте
		 * @param flags набор режимов компиляции инструкции
		 * @param width длина символа в байтах
		 * @return      кодовое значение символа в позиции текста
		 *
		 * \~english
		 * @brief Function of getting the code point value of the character at a position in the text
		 * @details In the «UTF» mode the function parses the whole UTF-8 sequence,
		 *          otherwise it returns the code point value of a single byte. An invalid
		 *          sequence is parsed as a single byte.
		 * @param text  text to match
		 * @param pos   position of the character in the text
		 * @param flags set of compilation modes of the instruction
		 * @param width length of the character in bytes
		 * @return      code point value of the character at the position in the text
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ uint32_t decode(string_view text, const size_t pos, const uint32_t flags, size_t & width) noexcept;
		/**
		 * \~russian
		 * @brief Функция извлечения кодового значения символа набора ASCII
		 *
		 * @details Функция разбирает символ набора ASCII на месте, а разбор
		 *          последовательности UTF-8 передаёт функции разбора символа.
		 *          Посредник заведён затем, что разбор символа выполняется при
		 *          сопоставлении каждого символа текста, а подавляющее их
		 *          большинство принадлежит набору ASCII и разбирается чтением
		 *          одного байта: вызов ради него обходился дороже самого чтения.
		 *
		 * @param text  текст для сопоставления
		 * @param pos   позиция символа в тексте
		 * @param flags набор режимов компиляции инструкции
		 * @param width длина символа в байтах
		 * @return      кодовое значение символа в позиции текста
		 *
		 * \~english
		 * @brief Function of getting the code point value of a character of the ASCII set
		 * @details The function parses a character of the ASCII set in place, and passes parsing of
		 *          a UTF-8 sequence to the character parsing function.
		 *          The intermediary is introduced because character parsing is performed when
		 *          matching every character of the text, while the overwhelming
		 *          majority of them belong to the ASCII set and is parsed by reading
		 *          one byte: a call for its sake cost more than the reading itself.
		 * @param text  text to match
		 * @param pos   position of the character in the text
		 * @param flags set of compilation modes of the instruction
		 * @param width length of the character in bytes
		 * @return      code point value of the character at the position in the text
		 *
		 * \~
		 */
		AWH_REGEX_INLINE uint32_t letter(string_view text, const size_t pos, const uint32_t flags, size_t & width) noexcept {
			// Выполняем установку длины символа в байтах
			width = 1;
			/**
			 * Если позиция символа находится за пределами текста
			 */
			if(pos >= text.size())
				// Выводим отсутствие кодового значения символа
				return 0;
			// Получаем первый байт последовательности
			const uint8_t first = static_cast <uint8_t> (text[pos]);
			/**
			 * \~russian
			 * Если байт принадлежит набору ASCII
			 *
			 * @details Символ набора ASCII занимает один байт в любом режиме разбора,
			 *          поэтому кодовое значение его совпадает со значением байта.
			 *
			 * \~english
			 * If the byte belongs to the ASCII set
			 * @details A character of the ASCII set occupies one byte in any parsing mode,
			 *          therefore its code point value matches the value of the byte.
			 *
			 * \~
			 */
			if(first < 0x80)
				// Выводим кодовое значение одиночного байта
				return static_cast <uint32_t> (first);
			// Выводим кодовое значение символа, разобранного целиком
			return decode(text, pos, flags, width);
		}
		/**
		 * \~russian
		 * @brief Функция извлечения длины символа, предшествующего позиции текста
		 *
		 * @details В режиме «UTF» функция отступает к началу последовательности UTF-8,
		 *          иначе отступает на один байт. Функция применяется исполнением,
		 *          продвигающимся по тексту в обратном направлении.
		 *
		 * @param text  текст для сопоставления
		 * @param pos   позиция, предшествующий которой символ измеряется
		 * @param flags набор режимов компиляции инструкции
		 * @return      длина предшествующего позиции символа в байтах
		 *
		 * \~english
		 * @brief Function of getting the length of the character preceding a position in the text
		 * @details In the «UTF» mode the function steps back to the beginning of the UTF-8 sequence,
		 *          otherwise it steps back by one byte. The function is used by execution
		 *          that advances through the text backwards.
		 * @param text  text to match
		 * @param pos   position the character preceding which is measured
		 * @param flags set of compilation modes of the instruction
		 * @return      length in bytes of the character preceding the position
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ size_t behind(string_view text, const size_t pos, const uint32_t flags) noexcept;
		/**
		 * \~russian
		 * @brief Функция проверки привязки к позиции в тексте
		 *
		 * @param text  текст для сопоставления
		 * @param start позиция начала попытки сопоставления
		 * @param type  тип проверяемой привязки к позиции в тексте
		 * @param flags набор режимов компиляции инструкции
		 * @param pos   проверяемая позиция в тексте
		 * @return      результат проверки привязки к позиции в тексте
		 *
		 * \~english
		 * @brief Function of checking an anchor to a position in the text
		 * @param text  text to match
		 * @param start position of the beginning of the match attempt
		 * @param type  type of the checked anchor to a position in the text
		 * @param flags set of compilation modes of the instruction
		 * @param pos   position in the text to check
		 * @return      result of checking the anchor to a position in the text
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool assertion(string_view text, const size_t start, const anchor_t type, const uint32_t flags, const size_t pos) noexcept;
		/**
		 * \~russian
		 * @brief Функция проверки принадлежности символа классу символов
		 *
		 * @param value класс символов регулярного выражения
		 * @param code  кодовое значение проверяемого символа
		 * @param flags набор режимов компиляции инструкции
		 * @return      результат проверки принадлежности символа классу
		 *
		 * \~english
		 * @brief Function of checking whether a character belongs to a character class
		 * @param value character class of the regular expression
		 * @param code  code point value of the character to check
		 * @param flags set of compilation modes of the instruction
		 * @return      result of checking whether the character belongs to the class
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool belongs(const classview_t & value, const uint32_t code, const uint32_t flags) noexcept;
		/**
		 * \~russian
		 * @brief Функция извлечения длины графемного кластера в позиции текста
		 *
		 * @details Графемный кластер объединяет символы, воспринимаемые читателем как
		 *          единый знак, по правилам разбиения текста стандарта Юникода.
		 *
		 * @param text  текст для сопоставления
		 * @param pos   позиция начала графемного кластера в тексте
		 * @param flags набор режимов компиляции инструкции
		 * @return      длина графемного кластера в байтах
		 *
		 * \~english
		 * @brief Function of getting the length of the grapheme cluster at a position in the text
		 * @details A grapheme cluster joins the characters perceived by the reader as
		 *          a single sign, by the text splitting rules of the Unicode standard.
		 * @param text  text to match
		 * @param pos   position of the beginning of the grapheme cluster in the text
		 * @param flags set of compilation modes of the instruction
		 * @return      length of the grapheme cluster in bytes
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ size_t grapheme(string_view text, const size_t pos, const uint32_t flags) noexcept;
	};
};

#endif // __AWH_REGEX_TEXT__
