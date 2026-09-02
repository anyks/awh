/**
 * @file charset.hpp
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
 * @brief Заголовочный файл модуля перекодировки — разбор имён кодировок, перекодировка
 *        текста между однобайтовыми кодировками и UTF-8, определение кодировки текста
 *
 * @section charset_decisions Намеренные решения
 *
 * @details Перечисленное ниже выглядит несообразностью, но выбрано осознанно и
 *          правке не подлежит. Раздел заведён затем, чтобы разбор кода не начинался
 *          каждый раз с одних и тех же выводов.
 *
 *          <b>Многобайтовые кодировки восточных письменностей не заданы</b>. Именно
 *          они составляют основной объём эталонной реализации, тогда как в заголовках
 *          сетевых протоколов встречаются редко. Устройство модуля их добавлению не
 *          препятствует: таблица кодировки задаётся записью набора «CODEPAGES», а разбор
 *          многобайтовой записи потребует отдельного разборщика наравне с UTF-8.
 *
 *          <b>Перекодировка проходит через кодовые значения Юникода</b>, а не прямыми
 *          таблицами пар кодировок. Прямых таблиц потребовалось бы число, равное квадрату
 *          числа кодировок, тогда как накладные расходы промежуточного представления
 *          сводятся к обращению к таблице.
 *
 *          <b>Отказом отвечает перекодировка, а не разбор имени кодировки</b>. Имя,
 *          заданное отправителем, задаётся им произвольно, и его нераспознание — событие
 *          обыкновенное, тогда как непредставимый символ означает потерю данных. Порядок
 *          обращения с непредставимыми символами задаётся значением «replace_t».
 *
 *          <b>Кодировка ISO-8859-1 задана отдельно от CP1252</b>, хотя стандарт кодировок
 *          WHATWG предписывает разбирать её имя как имя CP1252. Framework обязан уметь
 *          записать текст в ISO-8859-1 по требованию, а разбор имени следует стандарту:
 *          имя «iso-8859-1» разбирается как CP1252.
 *
 * \~english
 * @brief Header file of the transcoding module — parsing of encoding names, transcoding of
 *        text between single-byte encodings and UTF-8, detection of the text encoding
 *
 * @section charset_decisions Deliberate decisions
 *
 * @details What is listed below looks like an incongruity, yet it was chosen deliberately and
 *          is not subject to correction. The section exists so that examination of the code does
 *          not start over and over with the very same conclusions.
 *
 *          <b>Multi-byte encodings of Eastern scripts are not defined</b>. They are precisely what
 *          makes up the bulk of the reference implementation, whereas in the headers of network
 *          protocols they are encountered rarely. The design of the module does not hinder adding
 *          them: an encoding table is defined by an entry of the "CODEPAGES" set, while parsing a
 *          multi-byte record will require a separate parser on a par with UTF-8.
 *
 *          <b>Transcoding goes through Unicode code values</b> rather than through direct tables of
 *          encoding pairs. Direct tables would be needed in a number equal to the square of the
 *          number of encodings, whereas the overhead of the intermediate representation comes down
 *          to a table lookup.
 *
 *          <b>It is transcoding that answers with a failure, not the parsing of the encoding name</b>.
 *          The name given by the sender is given by them arbitrarily, and failing to recognize it is
 *          an ordinary event, whereas an unrepresentable character means loss of data. The manner of
 *          handling unrepresentable characters is set by the "replace_t" value.
 *
 *          <b>The ISO-8859-1 encoding is defined separately from CP1252</b>, although the WHATWG
 *          encoding standard prescribes parsing its name as the name of CP1252. The Framework must
 *          be able to record a text in ISO-8859-1 on demand, while the parsing of the name follows
 *          the standard: the name "iso-8859-1" is parsed as CP1252.
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CHARSET__
#define __AWH_CHARSET__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "types.hpp"
#include "table.hpp"
#include "../unicode/utf8.hpp"

/**
 * Снимаем на время объявлений макросы, чьи имена заняты
 * членами перечислений ниже (возвращает их pop.hpp в конце файла)
 */
#include "../../sys/push.hpp"

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
	 * @brief Пространство имён модуля перекодировки
	 *
	 * \~english
	 * @brief Namespace of the transcoding module
	 *
	 * \~
	 */
	namespace charset {
		/**
		 * \~russian
		 * @brief Функция разбора имени кодировки
		 *
		 * @details Имя кодировки приводится к нормальному виду: буквы записываются
		 *          в нижнем регистре, окружающие пробельные символы опускаются. Набор
		 *          распознаваемых имён задан стандартом кодировок консорциума WHATWG.
		 *
		 * @param name имя кодировки, заданное заголовком протокола
		 * @return     обозначение кодировки либо признак нераспознанного имени
		 *
		 * \~english
		 * @brief Encoding name parsing function
		 *
		 * @details The encoding name is brought to a normal form: the letters are recorded in lower
		 *          case, the surrounding whitespace characters are omitted. The set of recognized names
		 *          is defined by the encoding standard of the WHATWG consortium.
		 *
		 * @param name encoding name given by a protocol header
		 * @return     encoding designation or the sign of an unrecognized name
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ encoding_t encoding(string_view name) noexcept;
		/**
		 * \~russian
		 * @brief Функция извлечения канонического имени кодировки
		 *
		 * @param encoding обозначение кодировки текста
		 * @return         каноническое имя кодировки
		 *
		 * \~english
		 * @brief Function extracting the canonical name of an encoding
		 *
		 * @param encoding designation of the text encoding
		 * @return         canonical name of the encoding
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ string_view label(const encoding_t encoding) noexcept;
		/**
		 * \~russian
		 * @brief Функция извлечения таблицы соответствия кодировки Юникоду
		 *
		 * @param encoding обозначение кодировки текста
		 * @return         таблица соответствия либо пустой указатель
		 *
		 * \~english
		 * @brief Function extracting the table of correspondence of an encoding to Unicode
		 *
		 * @param encoding designation of the text encoding
		 * @return         correspondence table or a null pointer
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ const table_t * table(const encoding_t encoding) noexcept;
		/**
		 * \~russian
		 * @brief Функция перекодировки текста из одной кодировки в другую
		 *
		 * @param text     перекодируемый текст
		 * @param from     кодировка, в которой записан текст
		 * @param to       кодировка, в которую записывается результат
		 * @param result   результат перекодировки текста
		 * @param replace  порядок обращения с непредставимыми символами
		 * @return         результат выполнения перекодировки текста
		 *
		 * \~english
		 * @brief Function transcoding a text from one encoding into another
		 *
		 * @param text     text being transcoded
		 * @param from     encoding the text is recorded in
		 * @param to       encoding the result is recorded into
		 * @param result   result of transcoding the text
		 * @param replace  manner of handling unrepresentable characters
		 * @return         result of performing the text transcoding
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool transcode(string_view text, const encoding_t from, const encoding_t to, string & result, const replace_t replace = replace_t::STRICT) noexcept;
		/**
		 * \~russian
		 * @brief Функция перекодировки текста из одной кодировки в другую
		 *
		 * @details Отказ перекодировки выводится пустым текстом. Перекодировка, отказ
		 *          которой требуется отличить от пустого исходного текста, выполняется
		 *          одноимённой функцией, выводящей результат выполнения отдельно.
		 *
		 * @param text     перекодируемый текст
		 * @param from     кодировка, в которой записан текст
		 * @param to       кодировка, в которую записывается результат
		 * @param replace  порядок обращения с непредставимыми символами
		 * @return         получившийся в результате текст
		 *
		 * \~english
		 * @brief Function transcoding a text from one encoding into another
		 *
		 * @details A failure of the transcoding is output as an empty text. A transcoding whose failure
		 *          is required to be told apart from an empty source text is performed by the function of
		 *          the same name, which outputs the result of the performance separately.
		 *
		 * @param text     text being transcoded
		 * @param from     encoding the text is recorded in
		 * @param to       encoding the result is recorded into
		 * @param replace  manner of handling unrepresentable characters
		 * @return         text obtained as the result
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ string transcode(string_view text, const encoding_t from, const encoding_t to, const replace_t replace = replace_t::STRICT) noexcept;
		/**
		 * \~russian
		 * @brief Функция определения кодировки текста
		 *
		 * @details Определение выполняется проверкой правильности записи текста
		 *          в кодировке UTF-8: текст, ей отвечающий, признаётся записанным
		 *          в UTF-8, а не отвечающий — записанным в заданной кодировке.
		 *
		 * @param text      текст, кодировку которого требуется определить
		 * @param fallback  кодировка, предполагаемая для текста, записью UTF-8 не являющегося
		 * @return          обозначение определённой кодировки текста
		 *
		 * \~english
		 * @brief Text encoding detection function
		 *
		 * @details The detection is performed by verifying the correctness of the text record in the
		 *          UTF-8 encoding: a text conforming to it is recognized as recorded in UTF-8, and one
		 *          not conforming — as recorded in the given encoding.
		 *
		 * @param text      text whose encoding is required to be detected
		 * @param fallback  encoding assumed for a text that is not a UTF-8 record
		 * @return          designation of the detected text encoding
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ encoding_t detect(string_view text, const encoding_t fallback = encoding_t::CP1251) noexcept;
	};
};

/**
 * Возвращаем макросы, снятые в начале файла
 */
#include "../../sys/pop.hpp"

#endif // __AWH_CHARSET__
