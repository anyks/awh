/**
 * @file normalize.hpp
 * @date 2026-08-03
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
 * @brief Заголовочный файл нормализации текста — приведение текста к нормальным
 *        представлениям NFD, NFC, NFKD и NFKC, разложение и сочетание символов,
 *        а также канонический класс сочетания символа
 *
 * @section normalize_decisions Намеренные решения
 *
 * @details Перечисленное ниже выглядит несообразностью, но выбрано осознанно и
 *          правке не подлежит. Раздел заведён затем, чтобы разбор кода не начинался
 *          каждый раз с одних и тех же выводов.
 *
 *          <b>Слоги хангыля разлагаются и сочетаются вычислением</b>, а не таблицей.
 *          Так предписывает само приложение по нормализации стандарта Юникода: слогов
 *          одиннадцать тысяч, и их размещение в таблице сделало бы её на порядок больше
 *          без всякой пользы.
 *
 *          <b>Свойство исключения из сочетания берётся из состава подмодуля эталонной
 *          реализации приведения доменных имён</b>, тогда как остальные таблицы модуля
 *          Юникода — из состава подмодуля эталонной реализации регулярных выражений.
 *          Издания стандарта этих двух подмодулей между собой не совпадают. Список
 *          исключений сочетания стандартом заморожен и от издания не зависит, тогда как
 *          прочие таблицы обязаны быть одного издания, и им выбрано более позднее.
 *
 * \~english
 * @brief Header file of text normalization — bringing a text to the NFD, NFC, NFKD and NFKC
 *        normal forms, decomposition and composition of characters, as well as the canonical
 *        combining class of a character
 *
 * @section normalize_decisions Deliberate decisions
 *
 * @details What is listed below looks like an incongruity, yet it was chosen deliberately and
 *          is not subject to correction. The section exists so that examination of the code does
 *          not start over and over with the very same conclusions.
 *
 *          <b>Hangul syllables are decomposed and composed by computation</b> rather than by a
 *          table. That is what the Unicode standard annex on normalization itself prescribes:
 *          there are eleven thousand syllables, and placing them in a table would make it an
 *          order of magnitude larger to no benefit whatsoever.
 *
 *          <b>The composition exclusion property is taken from the submodule of the reference
 *          implementation of domain name mapping</b>, whereas the remaining tables of the Unicode
 *          module come from the submodule of the reference implementation of regular expressions.
 *          The editions of the standard of these two submodules do not coincide with one another.
 *          The list of composition exclusions is frozen by the standard and does not depend on the
 *          edition, whereas the other tables must be of one edition, and the later one was chosen
 *          for them.
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_UNICODE_NORMALIZE__
#define __AWH_UNICODE_NORMALIZE__

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <cstdint>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "types.hpp"
#include "table.hpp"

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
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён модуля Юникода
	 *
	 * \~english
	 * @brief Namespace of the Unicode module
	 *
	 * \~
	 */
	namespace unicode {
		/**
		 * \~russian
		 * @brief Функция извлечения канонического класса сочетания символа
		 *
		 * @param code кодовое значение символа
		 * @return     канонический класс сочетания символа
		 *
		 * \~english
		 * @brief Function extracting the canonical combining class of a character
		 *
		 * @param code character code value
		 * @return     canonical combining class of the character
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ uint8_t combining(const uint32_t code) noexcept;
		/**
		 * \~russian
		 * @brief Функция разложения символа набором кодовых значений
		 *
		 * @details Разложение выполняется полностью: символы разложения, разлагаемые
		 *          в свою очередь, разлагаются до неразложимых. Символ, разложения
		 *          не имеющий, выводится набором из самого себя.
		 *
		 * @param code   кодовое значение разлагаемого символа
		 * @param compat признак применения разложений совместимости
		 * @param result набор кодовых значений разложения символа
		 *
		 * \~english
		 * @brief Function decomposing a character into a set of code values
		 *
		 * @details The decomposition is performed in full: characters of the decomposition that are
		 *          in their turn decomposable are decomposed down to the indecomposable ones. A character
		 *          having no decomposition is output as a set consisting of itself.
		 *
		 * @param code   code value of the character being decomposed
		 * @param compat sign of applying compatibility decompositions
		 * @param result set of code values of the character decomposition
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void decompose(const uint32_t code, const bool compat, vector <uint32_t> & result) noexcept;
		/**
		 * \~russian
		 * @brief Функция канонического сочетания пары символов
		 *
		 * @param first  кодовое значение начального символа пары
		 * @param second кодовое значение сочетающегося символа пары
		 * @return       кодовое значение получившегося символа либо нулевое значение
		 *
		 * \~english
		 * @brief Function of canonical composition of a pair of characters
		 *
		 * @param first  code value of the starting character of the pair
		 * @param second code value of the combining character of the pair
		 * @return       code value of the resulting character or zero
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ uint32_t compose(const uint32_t first, const uint32_t second) noexcept;
		/**
		 * \~russian
		 * @brief Функция приведения текста к нормальному представлению
		 *
		 * @param text   набор кодовых значений приводимого текста
		 * @param form   вид нормального представления текста
		 * @param result набор кодовых значений приведённого текста
		 *
		 * \~english
		 * @brief Function bringing a text to a normal form
		 *
		 * @param text   set of code values of the text being brought
		 * @param form   kind of the normal form of the text
		 * @param result set of code values of the brought text
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void normalize(const vector <uint32_t> & text, const form_t form, vector <uint32_t> & result) noexcept;
	};
};

#endif // __AWH_UNICODE_NORMALIZE__
