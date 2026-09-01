/**
 * @file unicode.hpp
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
 * @brief Заголовочный файл модуля Юникода — поиск свойств символа по таблицам базы
 *        данных символов, разбор имён свойств, простое приведение регистра, разбиение
 *        текста на графемные кластеры и положение символа в сочетании письменностей
 *
 * @section unicode_decisions Намеренные решения
 *
 * @details Перечисленное ниже выглядит несообразностью, но выбрано осознанно и
 *          правке не подлежит. Раздел заведён затем, чтобы разбор кода не начинался
 *          каждый раз с одних и тех же выводов.
 *
 *          <b>Модуль отделён от модуля регулярных выражений</b>, откуда он происходит.
 *          Таблицы базы данных символов Юникода потребны не одному разбору регулярных
 *          выражений: ими же держатся приведение доменных имён к виду ASCII и перекодировка
 *          текста. Обратной зависимости нет: модуль Юникода ни о каком своём потребителе
 *          не осведомлён.
 *
 *          <b>Идентификаторы свойств «Xan», «Xps», «Xsp», «Xwd», «Xuc» и «UCP_»</b>
 *          заданы стандартом Юникода не будучи, а происходя от расширенных классов
 *          символов PCRE. Они размещены здесь потому, что занимают то же пространство
 *          численных значений, что и общие категории, и порождаются в таблицу имён
 *          вместе с ними. Разделить их без перенумерования таблиц нельзя.
 *
 *          <b>Таблицы порождаются из состава подмодуля эталонной реализации</b>
 *          порождателем tools/encoding/unicode/generate.py, а не из состава стандарта Юникода
 *          напрямую. Издание стандарта тем самым совпадает с изданием, которого
 *          держится эталон, и сверка с ним остаётся осмысленной. Смена издания
 *          подмодуля требует повторного порождения таблиц.
 *
 *          <b>Набор принимаемых имён свойств снимается опросом эталонной реализации</b>
 *          стендом tools/verify/regex/accepted.cpp и хранится файлом sh/unicode.accepted.
 *          Набор имён, признаваемых эталоном, ни таблицами стандарта, ни его текстом
 *          не задаётся: часть имён эталон отвергает, а часть разрешает сверх стандарта.
 *
 *          <b>Свойство Bidi_Mirrored берётся из таблицы BidiMirroring.txt</b>, а не
 *          из девятого поля таблицы UnicodeData.txt, откуда его берёт сам стандарт.
 *          Так поступает эталонная реализация, и наборы символов этих двух источников
 *          между собой не совпадают.
 *
 *          <b>Обозначения «bc=M», «bc=C» и «bc=Control» задают двоичные свойства</b>
 *          Bidi_Mirrored и Bidi_Control, тогда как обозначение вида «bc=» задаёт класс
 *          двунаправленности. Так их разрешает эталонная реализация, сличающая имена
 *          нестрого, и совместимость с ней здесь важнее строгости обозначения.
 *
 * \~english
 * @brief Header file of the Unicode module — lookup of character properties in the tables of the
 *        character database, parsing of property names, simple case folding, splitting of text
 *        into grapheme clusters and the position of a character within an Indic conjunct
 *
 * @section unicode_decisions Deliberate decisions
 *
 * @details What is listed below looks like an incongruity, yet it was chosen deliberately and
 *          is not subject to correction. The section exists so that examination of the code does
 *          not start over and over with the very same conclusions.
 *
 *          <b>The module is separated from the regular expressions module</b> it originates from.
 *          The tables of the Unicode character database are needed by more than the parsing of
 *          regular expressions: the bringing of domain names to the ASCII form and the transcoding
 *          of text rest upon them as well. There is no reverse dependency: the Unicode module is
 *          not aware of any of its consumers.
 *
 *          <b>The property identifiers "Xan", "Xps", "Xsp", "Xwd", "Xuc" and "UCP_"</b> are not
 *          defined by the Unicode standard, but originate from the extended character classes of
 *          PCRE. They are placed here because they occupy the same space of numeric values as the
 *          general categories and are generated into the names table together with them. They
 *          cannot be separated without renumbering the tables.
 *
 *          <b>The tables are generated from the submodule of the reference implementation</b> by the
 *          generator tools/encoding/unicode/generate.py rather than from the Unicode standard
 *          directly. The edition of the standard thereby coincides with the edition the reference
 *          adheres to, and verification against it remains meaningful. A change of the submodule
 *          edition requires regenerating the tables.
 *
 *          <b>The set of accepted property names is taken by interrogating the reference
 *          implementation</b> with the rig tools/verify/regex/accepted.cpp and is stored in the file
 *          sh/unicode.accepted. The set of names recognized by the reference is defined neither by
 *          the tables of the standard nor by its text: some names the reference rejects, and some
 *          it permits beyond the standard.
 *
 *          <b>The Bidi_Mirrored property is taken from the BidiMirroring.txt table</b> rather than
 *          from the ninth field of the UnicodeData.txt table, where the standard itself takes it
 *          from. That is what the reference implementation does, and the character sets of these
 *          two sources do not coincide with one another.
 *
 *          <b>The designations "bc=M", "bc=C" and "bc=Control" denote the binary properties</b>
 *          Bidi_Mirrored and Bidi_Control, whereas a designation of the form "bc=" denotes a
 *          bidirectional class. That is how the reference implementation resolves them, comparing
 *          names loosely, and compatibility with it matters here more than strictness of designation.
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_UNICODE__
#define __AWH_UNICODE__

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <cstdint>
#include <string_view>

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
		 * @brief Функция извлечения идентификатора свойства по его имени
		 *
		 * @details Имя свойства приводится к нормальному виду: буквы записываются
		 *          в нижнем регистре, разделители имени опускаются. Имена свойств,
		 *          не поддерживаемых модулем, не распознаются.
		 *
		 * @param name имя свойства Юникода
		 * @return     идентификатор свойства либо признак нераспознанного имени
		 *
		 * \~english
		 * @brief Function extracting a property identifier by its name
		 *
		 * @details The property name is brought to a normal form: the letters are recorded in lower
		 *          case, the name separators are omitted. Names of properties not supported by the
		 *          module are not recognized.
		 *
		 * @param name name of the Unicode property
		 * @return     property identifier or the sign of an unrecognized name
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ uint16_t property(string_view name) noexcept;
		/**
		 * \~russian
		 * @brief Функция проверки обладания символом свойством Юникода
		 *
		 * @param code кодовое значение проверяемого символа
		 * @param id   идентификатор проверяемого свойства Юникода
		 * @return     результат проверки обладания символом свойством
		 *
		 * \~english
		 * @brief Function checking whether a character holds a Unicode property
		 *
		 * @param code code value of the character being checked
		 * @param id   identifier of the Unicode property being checked
		 * @return     result of checking whether the character holds the property
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool holds(const uint32_t code, const uint16_t id) noexcept;
		/**
		 * \~russian
		 * @brief Функция извлечения общей категории символа
		 *
		 * @param code кодовое значение символа
		 * @return     идентификатор общей категории символа
		 *
		 * \~english
		 * @brief Function extracting the general category of a character
		 *
		 * @param code character code value
		 * @return     identifier of the general category of the character
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ uint16_t general(const uint32_t code) noexcept;
		/**
		 * \~russian
		 * @brief Функция извлечения класса двунаправленности символа
		 *
		 * @details Класс выводится значением, к которому прибавлено основание
		 *          идентификаторов классов двунаправленности, что позволяет сличать
		 *          его с идентификатором, полученным разбором имени класса.
		 *
		 * @param code кодовое значение символа
		 * @return     идентификатор класса двунаправленности символа
		 *
		 * \~english
		 * @brief Function extracting the bidirectional class of a character
		 *
		 * @details The class is output as a value to which the base of the bidirectional class
		 *          identifiers has been added, which makes it possible to compare it with an identifier
		 *          obtained by parsing a class name.
		 *
		 * @param code character code value
		 * @return     identifier of the bidirectional class of the character
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ uint16_t bidirectional(const uint32_t code) noexcept;
		/**
		 * \~russian
		 * @brief Функция извлечения набора письменностей символа
		 *
		 * @details Набор выводится расширением письменности символа, а при отсутствии
		 *          расширения - единственной его письменностью. Номера выводятся
		 *          без основания идентификаторов письменностей, отчего сличаются
		 *          между собою напрямую. Набор нужен прогону письменности:
		 *          он сводит письменности прогона пересечением наборов.
		 *
		 * @param code   кодовое значение символа
		 * @param output набор номеров письменностей символа
		 * @param size   размер набора номеров письменностей
		 *
		 * @return       количество письменностей, в набор выведенных
		 *
		 * \~english
		 * @brief Function extracting the set of scripts of a character
		 *
		 * @details The set is output as the script extension of the character, and in the absence
		 *          of an extension — as its single script. The numbers are output
		 *          without the base of the script identifiers, whereby they are compared
		 *          with one another directly. The set is needed by a script run:
		 *          it reduces the scripts of the run by an intersection of the sets.
		 *
		 * @param code   character code value
		 * @param output set of the numbers of the scripts of the character
		 * @param size   size of the set of the numbers of the scripts
		 *
		 * @return       number of scripts output into the set
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ size_t scripts(const uint32_t code, uint16_t * output, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Функция простого приведения регистра символа
		 *
		 * @details Приведение выполняется по таблице простого приведения регистра
		 *          Юникода, приводящей символы, различающиеся лишь регистром,
		 *          к одному кодовому значению.
		 *
		 * @param code кодовое значение приводимого символа
		 * @return     приведённое кодовое значение символа
		 *
		 * \~english
		 * @brief Function of simple case folding of a character
		 *
		 * @details The folding is performed by the Unicode simple case folding table, which brings
		 *          characters differing only in case to one and the same code value.
		 *
		 * @param code code value of the character being folded
		 * @return     folded code value of the character
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ uint32_t casefold(const uint32_t code) noexcept;
		/**
		 * \~russian
		 * @brief Функция извлечения набора символов, приводимых к одному значению
		 *
		 * @details Набор содержит все символы, приводимые приведением регистра
		 *          к одному значению, включая сам символ. Символы, не имеющие иного
		 *          регистра, набора не образуют.
		 *
		 * @param code   кодовое значение символа
		 * @param result набор символов, приводимых к одному значению
		 * @return       результат наличия набора приведения регистра
		 *
		 * \~english
		 * @brief Function extracting the set of characters folded to one and the same value
		 *
		 * @details The set contains all the characters folded by case folding to one and the same
		 *          value, including the character itself. Characters having no other case do not form
		 *          a set.
		 *
		 * @param code   character code value
		 * @param result set of characters folded to one and the same value
		 * @return       result of the presence of a case folding set
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool variants(const uint32_t code, vector <uint32_t> & result) noexcept;
		/**
		 * \~russian
		 * @brief Функция извлечения класса разбиения текста на графемные кластеры
		 *
		 * @param code кодовое значение символа
		 * @return     класс разбиения текста на графемные кластеры
		 *
		 * \~english
		 * @brief Function extracting the class of splitting text into grapheme clusters
		 *
		 * @param code character code value
		 * @return     class of splitting text into grapheme clusters
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ cluster_t cluster(const uint32_t code) noexcept;
		/**
		 * \~russian
		 * @brief Функция извлечения положения символа в сочетании индийских письменностей
		 *
		 * @param code кодовое значение символа
		 * @return     положение символа в сочетании индийских письменностей
		 *
		 * \~english
		 * @brief Function extracting the position of a character within an Indic conjunct
		 *
		 * @param code character code value
		 * @return     position of the character within an Indic conjunct
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ indic_t indic(const uint32_t code) noexcept;
	};
};

#endif // __AWH_UNICODE__
