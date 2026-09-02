/**
 * @file common.hpp
 * @date 2026-08-12
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
 * @brief Заголовочный файл общих определений контейнера CSV — коды ошибок разбора, виды событий
 *        чтения, наречия записи, кодировки исходного текста, пределы разбора, структуры поля,
 *        записи и положения в исходном тексте
 *
 * \~english
 * @brief Header file of the common definitions of the CSV container — the error codes of the parsing, the kinds of the events
 *        of the reading, the dialects of the writing, the encodings of the source text, the limits of the parsing, the structures of a field,
 *        of a record and of a position in the source text
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_CSV_COMMON__
#define __AWH_CODEC_CSV_COMMON__

/**
 * Стандартные заголовочные файлы
 */
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../sys/log.hpp"
#include "../../sys/global.hpp"

/**
 * Снимаем на время объявлений макросы, чьи имена заняты
 * членами перечислений ниже (возвращает их pop.hpp в конце файла)
 */
#include "../../sys/push.hpp"

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
	 * @brief Пространство имён контейнеров данных
	 *
	 *
	 * \~english
	 * @brief Data containers namespace
	 *
	 * \~
	 */
	namespace codec {
		/**
		 * \~russian
		 * @brief Пространство имён контейнера CSV
		 *
		 * @details Разбор и запись значений, разделённых знаком-разделителем: записей из
		 * полей, где поле вправе быть заключено в кавычки и содержать внутри себя и
		 * разделитель, и перевод строки, и сами кавычки
		 *
		 * @par Намеренные решения
		 *
		 * Перечисленное ниже не является пробелом реализации: это очерченные границы
		 * задачи, и каждое решение О ПОВЕДЕНИИ закреплено проверочным испытанием
		 *
		 * @warning Оговорка «о поведении» стоит по замеру 01.09.2026: решение «внешние
		 *          файлы не подключаются и содержимое полей не исполняется» говорит об
		 *          ОТСУТСТВИИ работы, и закрепить его испытанием нельзя - закрепляется оно
		 *          отсутствием такой работы в исходниках. Тем же замером вскрылось, что
		 *          довод решения о заголовке («угадать нельзя») закрепления не имел, хотя
		 *          само поведение настройки закреплено давно: заведена
		 *          `HeaderCannotBeGuessedFromTheText`, где один и тот же текст даёт два
		 *          законных прочтения
		 *
		 * @li **Договор RFC 4180 описывает не всё, что встречается на деле.** Документ
		 * этот носит звание информационного, а не обязательного, и сам себя объявляет
		 * описанием сложившегося обихода. Оттого разбор ведётся строго по нему лишь в
		 * согласии с настройками: сверх описанного признаются разделители помимо запятой,
		 * перевод строки одиночным знаком, поля с обвязкой вокруг кавычек и записи с
		 * разным числом полей. Каждое послабление - отдельная настройка, а не общий
		 * признак «нестрого»: без этого нельзя ни сказать, что именно разбирается, ни
		 * закрепить это испытанием
		 *
		 * @li **Тип поля не выводится.** Поле всегда выдаётся последовательностью знаков,
		 * а приведение к числу либо к логическому значению делается по явному запросу.
		 * Угадывание типа порождает разночтения: запись «1.10» числом теряет разряд,
		 * «007» разбирается то восьмеричным, то десятичным, а «2026-08-12» отметкой
		 * времени - в зависимости от разбирающего. Отдельно неприятен случай, когда
		 * угаданный тип меняется от строки к строке в пределах одного столбца
		 *
		 * @li **Кавычки внутри поля без кавычек разбором признаются.** Запись вида
		 * «a"b» договором не описана вовсе. Отвечать на неё отказом значило бы отвергать
		 * файлы, которые пишут и читают все прочие; кавычка признаётся здесь
		 * обыкновенным знаком. Строгое прочтение включается настройкой
		 *
		 * @li **Признак заголовка задаётся, а не угадывается.** Договор относит наличие
		 * заголовка к сведениям о содержимом, передаваемым отдельно от самого текста, и
		 * это не случайность: угадать заголовок нельзя - файл из одних лишь строковых
		 * значений неотличим от файла с заголовком. Настройка признака поэтому имеет
		 * ровно два положения, и «самостоятельного» среди них нет
		 *
		 * @li **Разделитель определяется по содержимому, но лишь по запросу.** Умолчанием
		 * берётся запятая, названная договором. Самостоятельное определение включается
		 * настройкой и опирается на постоянство числа полей в записях, а не на частоту
		 * знака: частота обманывается текстом, где запятых в значениях больше, чем
		 * разделителей
		 *
		 * @li **Приведение окончаний строк к единому виду не производится.** Перевод
		 * строки внутри поля в кавычках выдаётся ровно тем, каким записан: договор велит
		 * хранить содержимое поля неизменным, а приведение испортило бы значения, где
		 * знаки эти значащие
		 *
		 * @li **Поле в кавычках, не закрытое до конца текста, признаётся отказом.**
		 * Прочие разбирающие - и модуль csv языка Python в их числе - выдают накопленное
		 * содержимое как есть, и потому сличение с ними на такой записи расходится
		 * намеренно. Молчаливая выдача здесь неотличима от целого поля, а причина у
		 * такого текста ровно одна: он оборван - будь то передача, прерванная на
		 * середине, обрезанный по размеру файл либо запись, не доведённая до конца.
		 * Выдать оборванное за целое значило бы потерять данные молча, тогда как отказ
		 * доводит обрыв до потребителя. Накопленное при этом не пропадает: разбор выдал
		 * все записи, предшествующие обрыву
		 *
		 * @li **Внешние файлы не подключаются и содержимое полей не исполняется.**
		 * Значение, начинающееся со знака равенства либо плюса, выдаётся
		 * последовательностью знаков как есть: подстановка его в вычисляемое выражение -
		 * дело потребителя, и защита от неё тоже
		 *
		 * \~english
		 * @brief CSV container namespace
		 * @details The parsing and the writing of the values separated by a separator character: of the records made of
		 * the fields, where a field has the right to be enclosed in quotes and to contain inside itself both
		 * the separator, and a line feed, and the quotes themselves
		 * @par Deliberate decisions
		 * What is listed below is not a gap of the implementation: these are the outlined boundaries of the
		 * task, and each of the decisions is fixed by a verifying test
		 * @li **The RFC 4180 protocol describes not everything that is met in practice.** That document
		 * bears the rank of an informational rather than an obligatory one, and it declares itself
		 * a description of an established custom. Because of that the parsing is conducted strictly by it only in
		 * accordance with the settings: beyond what is described, separators other than the comma are recognized,
		 * a line feed by a single character, fields with a padding around the quotes and records with
		 * a differing number of fields. Every relaxation is a separate setting rather than a common
		 * «non-strict» flag: without this one can neither say what exactly is being parsed nor
		 * fix it by a test
		 * @li **The type of a field is not inferred.** A field is always issued as a sequence of characters,
		 * while a conversion to a number or to a logical value is done upon an explicit request.
		 * A guessing of the type gives birth to discrepancies: the record «1.10» as a number loses a digit,
		 * «007» is parsed now as an octal, now as a decimal one, while «2026-08-12» as a timestamp —
		 * depending on the one parsing. Especially unpleasant is the case when
		 * a guessed type changes from row to row within a single column
		 * @li **Quotes inside a field without quotes are recognized by the parsing.** A record of the form
		 * «a"b» is not described by the protocol at all. To answer it with a refusal would mean to reject
		 * the files which all the others write and read; a quote is recognized here as
		 * an ordinary character. A strict reading is enabled by a setting
		 * @li **The flag of a header is given rather than guessed.** The protocol assigns the presence
		 * of a header to the information about the content transmitted separately from the text itself, and
		 * this is not by chance: a header cannot be guessed — a file made of string values alone
		 * is indistinguishable from a file with a header. The setting of the flag therefore has
		 * exactly two positions, and there is no «automatic» one among them
		 * @li **The separator is determined by the content, but only upon a request.** By default
		 * the comma named by the protocol is taken. An automatic determination is enabled by a
		 * setting and relies on the constancy of the number of the fields in the records rather than on the frequency of a
		 * character: the frequency is deceived by a text where there are more commas in the values than
		 * separators
		 * @li **A conversion of the line endings to a single form is not performed.** A line
		 * feed inside a field in quotes is issued exactly as it has been written: the protocol orders
		 * to keep the content of a field unchanged, while a conversion would spoil the values where
		 * those characters are significant
		 * @li **A field in quotes not closed by the end of the text is recognized as a refusal.**
		 * The other parsers — and the csv module of the Python language among them — issue the accumulated
		 * content as it is, and therefore a comparison with them on such a record diverges
		 * deliberately. A silent issuance here is indistinguishable from a whole field, while the reason for
		 * such a text is exactly one: it has been cut off — be it a transmission interrupted in the
		 * middle, a file truncated by size or a record not brought to the end.
		 * To pass off what has been cut off as a whole thing would mean to lose the data silently, whereas a refusal
		 * brings the cut-off to the consumer. What has been accumulated is not lost thereby: the parsing has issued
		 * all the records preceding the cut-off
		 * @li **External files are not included and the content of the fields is not executed.**
		 * A value beginning with an equals sign or a plus is issued as
		 * a sequence of characters as it is: its substitution into a computed expression is
		 * the business of the consumer, and the protection from it as well
		 *
		 * \~
		 */
		namespace csv {
			/**
			 * \~russian
			 * @brief Наибольшая допустимая длина поля в байтах
			 *
			 * @details Предел считается на поле целиком - вместе со всеми строками поля,
			 * заключённого в кавычки, - иначе перевод строки внутри поля давал бы обход
			 * предела
			 *
			 * \~english
			 * @brief Largest admissible length of a field in bytes
			 * @details The limit is counted over the field as a whole — together with all the lines of a field
			 * enclosed in quotes — otherwise a line feed inside a field would give a bypass
			 * of the limit
			 *
			 * \~
			 */
			constexpr uint32_t MAX_FIELD = 0x100000;

			/**
			 * \~russian
			 * @brief Наибольшая допустимая длина записи в байтах
			 *
			 * @details Записью считается строка целиком со всеми своими полями. Предел
			 * этот ограничивает объём, удерживаемый разбором в памяти на одну запись
			 *
			 * \~english
			 * @brief Largest admissible length of a record in bytes
			 * @details A record is the whole line with all its fields. This limit
			 * limits the volume held by the parsing in the memory per record
			 *
			 * \~
			 */
			constexpr uint32_t MAX_RECORD = 0x1000000;

			/**
			 * \~russian
			 * @brief Наибольшее допустимое количество полей в записи
			 *
			 * \~english
			 * @brief Largest admissible number of the fields in a record
			 *
			 * \~
			 */
			constexpr uint32_t MAX_FIELDS = 0x10000;

			/**
			 * \~russian
			 * @brief Количество первых записей, по которым определяется разделитель
			 *
			 * @details Просмотр ведётся до этого числа записей либо до конца текста -
			 * смотря что раньше. Числа этого довольно, чтобы постоянство количества полей
			 * проявилось, и мало настолько, чтобы удержать просмотренное в памяти
			 *
			 * \~english
			 * @brief Number of the first records by which the separator is determined
			 * @details The survey is conducted up to this number of records or to the end of the text —
			 * whichever comes first. This number is enough for the constancy of the number of the fields
			 * to manifest itself, and small enough to hold what has been surveyed in the memory
			 *
			 * \~
			 */
			constexpr uint32_t DETECT_RECORDS = 32;

			/**
			 * \~russian
			 * @brief Обозначение отсутствующего положения в исходном тексте
			 *
			 * \~english
			 * @brief Designation of an absent position in the source text
			 *
			 * \~
			 */
			constexpr uint64_t NO_OFFSET = static_cast <uint64_t> (~0ull);

			/**
			 * \~russian
			 * @brief Обозначение отсутствующего номера поля или записи
			 *
			 * \~english
			 * @brief Designation of an absent number of a field or of a record
			 *
			 * \~
			 */
			constexpr uint32_t NO_INDEX = static_cast <uint32_t> (~0u);

			/**
			 * \~russian
			 * @brief Коды ошибок разбора текста
			 *
			 * @details Разбор не выбрасывает исключений: признаком отказа служит код ошибки
			 * вместе с положением в исходном тексте, где отказ произошёл
			 *
			 * \~english
			 * @brief Error codes of the parsing of a text
			 * @details The parsing does not throw exceptions: the error code together with the position
			 * in the source text where the refusal has occurred serves as the sign of a refusal
			 *
			 * \~
			 */
			enum class error_t : uint8_t {
				NONE                 = 0x00, // Ошибок не обнаружено
				INTERNAL             = 0x01, // Внутренняя ошибка разбора
				INVALID_CHARACTER    = 0x02, // Знак недопустим в тексте
				INVALID_ENCODING     = 0x03, // Последовательность байтов не отвечает объявленной кодировке
				UNSUPPORTED_ENCODING = 0x04, // Объявленная кодировка не поддерживается
				UNTERMINATED_QUOTE   = 0x05, // Поле в кавычках не закрыто до конца текста
				UNESCAPED_QUOTE      = 0x06, // Одиночная кавычка внутри поля без кавычек при строгом разборе
				TRAILING_CHARACTERS  = 0x07, // Знаки за закрывающей кавычкой поля
				FIELD_TOO_LONG       = 0x08, // Длина поля превышает допустимую
				RECORD_TOO_LONG      = 0x09, // Длина записи превышает допустимую
				TOO_MANY_FIELDS      = 0x0A, // Количество полей в записи превышает допустимое
				FIELD_COUNT_MISMATCH = 0x0B, // Количество полей записи расходится с количеством полей заголовка
				SEPARATOR_CONFLICT   = 0x0C, // Разделитель не задан вовсе либо совпадает со знаком кавычек либо с переводом строки
				EMPTY_HEADER         = 0x0D, // Заголовок объявлен пустым именем поля
				DUPLICATE_HEADER     = 0x0E, // Имя поля в заголовке объявлено повторно
				NO_HEADER            = 0x0F, // Заголовок затребован, а текст пуст
				FILE_NOT_OPENED      = 0x10, // Файл таблицы открыть не удалось
				TEXT_ALREADY_ENDED   = 0x11, // Подача продолжена после объявленного конца текста
				FILE_NOT_WRITTEN     = 0x12, // Текст таблицы записать в файл не удалось
				UNWRITABLE_FIELD     = 0x13, // Содержимое поля установленными настройками записи непредставимо
				FILE_NOT_READ        = 0x14, // Файл таблицы прочитать не удалось
				STORAGE_EXHAUSTED    = 0x15  // Разбираемая таблица не помещается в разрядность хранилища
			};

			/**
			 * \~russian
			 * @brief Виды событий чтения текста
			 *
			 * @details Чтение выдаёт события по мере разбора текста, не удерживая его целиком
			 *
			 * @note Поле выдаётся своим событием, а не в составе записи: запись из тысячи
			 * полей иначе удерживалась бы в памяти целиком прежде первой выдачи. Конец
			 * записи отмечается отдельным событием, и оно приходит и у записи пустой
			 *
			 * \~english
			 * @brief Kinds of the events of the reading of a text
			 * @details The reading issues the events as the text is parsed without holding it in full
			 * @note A field is issued by its own event rather than as a part of a record: a record of a thousand
			 * fields would otherwise be held in the memory in full before the first issuance. The end
			 * of a record is marked by a separate event, and it comes for an empty record as well
			 *
			 * \~
			 */
			enum class event_t : uint8_t {
				NONE    = 0x00, // Событие не определено
				HEADER  = 0x01, // Поле заголовка, выдаётся лишь при включённом признаке заголовка
				FIELD   = 0x02, // Поле записи
				RECORD  = 0x03, // Запись окончена, полей её больше не будет
				COMMENT = 0x04, // Строка примечания, выдаётся лишь при заданном знаке примечания
				BLANK   = 0x05, // Пустая строка, выдаётся лишь по настройке
				FINISH  = 0x06  // Текст разобран до конца, событие видно после цикла разбора
			};

			/**
			 * \~russian
			 * @brief Кодировки исходного текста
			 *
			 * @details Текст кодировку не объявляет, и определяется она по метке порядка
			 * байтов в начале текста; при её отсутствии текст считается записанным в UTF-8
			 *
			 * @note Кодировка UTF-32 не поддерживается, но опознаётся: метка её
			 * начинается теми же байтами, что и метка UTF-16 с обратным порядком, и без
			 * такого опознания текст в UTF-32 разбирался бы как UTF-16 с пустыми знаками
			 * между настоящими. Опознав её, разбор отвечает отказом, а не молчаливой
			 * бессмыслицей
			 *
			 * \~english
			 * @brief Encodings of the source text
			 * @details A text does not announce its encoding, and it is determined by the byte order
			 * mark at the beginning of the text; in its absence the text is considered written in UTF-8
			 * @note The UTF-32 encoding is not supported but is recognized: its mark
			 * begins with the same bytes as the mark of UTF-16 with the reverse order, and without
			 * such a recognition a text in UTF-32 would be parsed as UTF-16 with empty characters
			 * between the real ones. Having recognized it, the parsing answers with a refusal rather than with a silent
			 * nonsense
			 *
			 * \~
			 */
			enum class encoding_t : uint8_t {
				NONE    = 0x00, // Кодировка не определена
				UTF8    = 0x01, // Кодировка UTF-8
				UTF16LE = 0x02, // Кодировка UTF-16 с обратным порядком байтов
				UTF16BE = 0x03, // Кодировка UTF-16 с прямым порядком байтов
				LATIN1  = 0x04, // Кодировка ISO-8859-1
				ASCII   = 0x05, // Кодировка US-ASCII
				CP1252  = 0x06  // Кодировка Windows-1252
			};

			/**
			 * \~russian
			 * @brief Признак наличия заголовка в тексте
			 *
			 * @details Положений ровно два, и «самостоятельного» среди них нет намеренно:
			 * файл из одних лишь строковых значений неотличим от файла с заголовком, и
			 * всякое угадывание здесь ошибается молча
			 *
			 * \~english
			 * @brief Flag of the presence of a header in the text
			 * @details There are exactly two positions, and there is no «automatic» one among them deliberately:
			 * a file made of string values alone is indistinguishable from a file with a header, and
			 * every guessing here errs silently
			 *
			 * \~
			 */
			enum class header_t : uint8_t {
				NONE    = 0x00, // Заголовка нет, первая запись содержит значения
				PRESENT = 0x01  // Первая запись содержит имена полей
			};

			/**
			 * \~russian
			 * @brief Способ записи кавычки внутри поля, заключённого в кавычки
			 *
			 * \~english
			 * @brief Way of writing a quote inside a field enclosed in quotes
			 *
			 * \~
			 */
			enum class escape_t : uint8_t {
				DOUBLE    = 0x00, // Кавычка удваивается, как велит RFC 4180
				BACKSLASH = 0x01, // Кавычка предваряется обратной косой чертой
				BOTH      = 0x02  // Признаются оба способа, при записи берётся удвоение
			};

			/**
			 * \~russian
			 * @brief Правило заключения поля в кавычки при записи
			 *
			 * @warning Записанное без кавычек (`NONE`) читается ТОЛЬКО при способе отмены
			 * `escape_t::BACKSLASH` либо `escape_t::BOTH`: кавычек нет, и единственным
			 * способом уберечь разделитель и знаки конца строки внутри поля остаётся знак
			 * отмены. Запись ставит его независимо от собственной настройки `escape` -
			 * настройка эта описывает отмену кавычки ВНУТРИ кавычек, а их тут нет вовсе.
			 * Чтение же снимало отмены лишь внутри кавычек, и записанное прочитывалось не
			 * тем, чем записано, причём молча: поле `a,b` уходило текстом `a\,b`, а читалось
			 * двумя полями `a\` и `b`. Так же поступает и обиход - `escapechar` модуля `csv`
			 * языка Python при чтении снимает особое значение со следующего знака
			 *
			 * \~english
			 * @brief Rule of enclosing a field in quotes at the writing
			 * @warning What has been written without quotes (`NONE`) is read ONLY with the escaping method
			 * `escape_t::BACKSLASH` or `escape_t::BOTH`: there are no quotes, and the only way to protect
			 * the separator and the end-of-line characters inside a field is the escape character.
			 * The writing puts it independently of its own `escape` setting — that setting describes the escaping
			 * of a quote INSIDE quotes, and there are none here at all. The reading, however, removed the escapes
			 * only inside quotes, and what had been written was read back as something other than what was written,
			 * and silently at that: the field `a,b` went out as the text `a\,b` and was read as two fields `a\` and `b`.
			 * The custom does the same — the `escapechar` of the `csv` module of the Python language removes
			 * the special meaning from the following character at the reading
			 *
			 * \~
			 */
			enum class quoting_t : uint8_t {
				/**
				 * \~russian
				 * В кавычки берутся лишь поля, обойтись без них не могущие
				 *
				 * @details Причин тому ШЕСТЬ, и все они здесь названы, ибо снаружи
				 * вывести их неоткуда. Четыре причины даёт само содержимое поля:
				 * знак-разделитель, знак кавычек, перевод строки и возврат каретки.
				 * Пятая - обвязка ПРОБЕЛЬНЫМ знаком по краям поля: пробел, табуляция
				 * и прочие, ибо читающие, снимающие обвязку сами, теряют её без кавычек.
				 * Пробельный знак ПОСРЕДИ поля причиною не является
				 *
				 * @details Две последние причины зависят от МЕСТА поля, а не от его
				 * содержимого. Поле, первым в записи начатое знаком примечания,
				 * обрамляется, иначе разбор унёс бы всю запись примечанием, - и лишь
				 * тогда, когда знак примечания настройкою задан. Поле, начинающее весь
				 * текст меткою порядка байтов, обрамляется тогда, когда запись метки
				 * ВЫКЛЮЧЕНА: включённой метка принадлежит самому тексту, а выключенной
				 * она пришла бы из содержимого и была бы снята чтением как признак
				 * кодировки. Уклад этот обратен ожиданию, и оттого назван здесь прямо
				 *
				 * @note Все шесть замерены 01.09.2026 и закреплены проверкой
				 *       `CodecCsvWriter.MinimalQuotingHasSixCauses`
				 *
				 * @warning Открытый посредник `quotable` знает лишь ЧЕТЫРЕ первые причины
				 *          и пятую: место поля ему не передаётся вовсе, и две последние
				 *          причины он вывести не может. Предсказывать выдачу записи им
				 *          нельзя - см. оговорку у самого посредника
				 *
				 * \~english
				 * Only the fields that cannot do without them are enclosed in quotes
				 * @details There are SIX causes: the separator character, the quote character, the line feed,
				 * the carriage return, a whitespace padding at the EDGES of the field, and two causes
				 * depending on the PLACE of the field — the comment character starting the first field of
				 * a record, and the byte order mark starting the whole text when the writing of the mark
				 * is switched OFF
				 *
				 * \~
				 */
				MINIMAL    = 0x00,
				ALL        = 0x01, // В кавычки берутся все поля без разбора
				NONNUMERIC = 0x02, // В кавычки берутся все поля, кроме записанных числом
				NONE       = 0x03  // Кавычки не ставятся вовсе, а знак-разделитель в поле предваряется знаком отмены
			};

			/**
			 * \~russian
			 * @brief Обращение с обвязкой вокруг поля
			 *
			 * @details Договор велит хранить содержимое поля неизменным, потому умолчанием
			 * обвязка сохраняется
			 *
			 * \~english
			 * @brief Treatment of the padding around a field
			 * @details The protocol orders to keep the content of a field unchanged, therefore by default
			 * the padding is preserved
			 *
			 * \~
			 */
			enum class trim_t : uint8_t {
				NONE      = 0x00, // Обвязка сохраняется как записана
				UNQUOTED  = 0x01, // Обвязка снимается лишь у полей без кавычек
				ALL       = 0x02  // Обвязка снимается и вокруг кавычек, и внутри поля без кавычек
			};

			/**
			 * \~russian
			 * @brief Обращение с записью, число полей которой расходится с заголовком
			 *
			 * \~english
			 * @brief Treatment of a record the number of the fields of which diverges from the header
			 *
			 * \~
			 */
			enum class ragged_t : uint8_t {
				ALLOW = 0x00, // Запись выдаётся как есть, числа полей не сверяются
				FILL  = 0x01, // Недостающие поля дополняются пустыми, лишние выдаются как есть
				ERROR = 0x02  // Расхождение прекращает разбор ошибкой
			};

			/**
			 * \~russian
			 * @brief Знак конца строки собираемого текста
			 *
			 * @warning Записанное всяким видом, кроме `CRLF`, читается ТОЛЬКО разбором
			 * нестрогим: договор знает концом записи одну лишь пару возврата каретки с
			 * переводом строки, и строгое чтение отвечает на одинокий знак отказом.
			 * Оговорка эта здесь потому, что виды эти заведены ради обихода, а не ради
			 * договора: собранное ими законно, но законно оно лишь за пределами договора
			 *
			 * \~english
			 * @brief Line ending character of the text being assembled
			 * @warning What has been written by any kind other than `CRLF` is read ONLY by the non-strict
			 * parsing: the contract knows as the end of a record only the pair of the carriage return with
			 * the line feed, and the strict reading answers a lone character with a refusal.
			 *
			 * \~
			 */
			enum class newline_t : uint8_t {
				CRLF = 0x00, // Возврат каретки с переводом строки, как велит RFC 4180
				LF   = 0x01, // Перевод строки, принятый в системах семейства UNIX
				CR   = 0x02  // Одиночный возврат каретки
			};

			/**
			 * \~russian
			 * @brief Отрезок общего хранилища знаков
			 *
			 * @details Хранилища знаков дописываются по мере разбора и при росте
			 * перемещаются, обесценивая ссылки на своё содержимое. Хранить положение
			 * отрезка вместо ссылки на него - единственный способ пережить такое
			 * перемещение
			 *
			 * \~english
			 * @brief Segment of the common storage of the characters
			 * @details The storages of the characters are appended to as the parsing goes on and at a growth they
			 * are moved, invalidating the references to their content. To keep the position of a
			 * segment instead of a reference to it is the only way to survive such a
			 * move
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Span {
				// Смещение начала отрезка в хранилище знаков
				uint32_t offset;
				// Длина отрезка в байтах
				uint32_t length;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				Span() noexcept : offset(0), length(0) {}
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param offset смещение начала отрезка в хранилище знаков
				 * @param length длина отрезка в байтах
				 *
				 * \~english
				 * @brief Constructor
				 * @param offset offset of the beginning of the segment in the storage of the characters
				 * @param length length of the segment in bytes
				 *
				 * \~
				 */
				Span(const uint32_t offset, const uint32_t length) noexcept : offset(offset), length(length) {}
			} span_t;

			/**
			 * \~russian
			 * @brief Положение в исходном тексте
			 *
			 * @details Служит для указания места ошибки и для привязки полей к исходному
			 * тексту
			 *
			 * @note Номер строки и положение в строке считаются в знаках Юникода, а
			 * смещение - в байтах исходного текста до перекодирования. Строки при этом
			 * считаются настоящие, а не записи: поле в кавычках вправе занимать несколько
			 * строк, и номер строки внутри такого поля растёт
			 *
			 * \~english
			 * @brief Position in the source text
			 * @details Serves for indicating the place of an error and for binding the fields to the source
			 * text
			 * @note The line number and the position in the line are counted in Unicode characters, while
			 * the offset — in the bytes of the source text before the transcoding. The lines are counted
			 * as the real ones rather than as the records: a field in quotes has the right to occupy several
			 * lines, and the line number inside such a field grows
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Location {
				/**
				 * \~russian
				 * @brief Смещение от начала текста в байтах
				 *
				 * @warning Смещение считается по тексту, ПРИВЕДЁННОМУ к UTF-8, а не по
				 *          исходным байтам. Метка порядка байтов снята, а текст в иной
				 *          кодировке уже переведён, и длины расходятся: документ Latin-1
				 *          в 52 байта даёт смещение конца 54, а метка порядка байтов
				 *          сдвигает все смещения на свою длину. Для указания места в
				 *          ИСХОДНОМ файле смещение это негодно, если текст подан не в
				 *          UTF-8 без метки
				 *
				 * \~english
				 * @brief The offset from the beginning of the text in bytes
				 *
				 * @warning The offset is counted over the text CONVERTED to UTF-8 rather than over
				 *          the source bytes. The byte order mark is stripped and a text in another
				 *          encoding is already converted, so the lengths diverge: a Latin-1 document
				 *          of 52 bytes gives the ending offset 54, and a byte order mark shifts every
				 *          offset by its length. For pointing at a place in the SOURCE file this
				 *          offset is unfit unless the text is supplied as UTF-8 without a mark
				 *
				 * \~
				 */
				uint64_t offset;
				// Номер строки, считая с единицы
				uint32_t line;
				// Положение в строке, считая с единицы
				uint32_t column;
				// Номер записи, считая с единицы, заголовок номера не занимает
				uint32_t record;
				// Номер поля в записи, считая с нуля
				uint32_t field;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				Location() noexcept :
				 offset(NO_OFFSET), line(0), column(0),
				 record(0), field(NO_INDEX) {}
			} location_t;

			/**
			 * \~russian
			 * @brief Поле записи
			 *
			 * @details Значение выдаётся уже приведённым к окончательному виду: кавычки
			 * сняты, удвоенные кавычки внутри поля сведены к одной, обвязка снята в объёме,
			 * разрешённом настройками
			 *
			 * @warning Сроки жизни двух видов свёртки РАЗНЫЕ, и мерить их надлежит порознь.
			 * Значение живёт не дольше следующего события: оно ссылается в буфер записи, а
			 * тот переиспользуется, - замер дал обращение к освобождённой памяти после
			 * двухсот поданных записей. Имя же живёт до сброса состояния чтения: оно
			 * ссылается в хранилище заголовка, отдельное от буфера записей и по разборе
			 * заголовка более не растущее, - те же двести подач оно пережило
			 *
			 * @note Прежде свёртка ручалась за оба вида одним, КОРОТКИМ сроком. Ручательство
			 *       это было безопасным, но неверным: звучащий копировал имя без нужды, а
			 *       главное - одинаковая запись на разные сроки скрывала, что сроки разные
			 *
			 * \~english
			 * @brief Field of a record
			 * @details The value is issued already brought to its final form: the quotes are
			 * removed, the doubled quotes inside the field are reduced to one, the padding is removed in the volume
			 * permitted by the settings
			 * @warning The lifetimes of the two views of the structure are DIFFERENT, and they should be measured separately.
			 * The value lives no longer than the next event: it refers into the buffer of a record, and
			 * that one is reused — the measurement gave an access to a freed memory after
			 * two hundred fed records. The name, however, lives until the reset of the state of the reading: it
			 * refers into the storage of the header, separate from the buffer of the records and, once the header
			 * is parsed, no longer growing — it survived those same two hundred feedings
			 * @note Formerly the structure guaranteed one, SHORT lifetime for both views. That guarantee
			 *       was safe but untrue: the caller copied the name without need, and
			 *       above all — one and the same record for different lifetimes concealed that the lifetimes differ
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Field {
				// Значение поля, приведённое к окончательному виду
				string_view value;
				/**
				 * \~russian
				 * Имя поля, взятое из заголовка
				 *
				 * @note Пусто при отключённом признаке заголовка, а равно и у полей, чей
				 *       номер выходит за пределы заголовка
				 *
				 * \~english
				 * Name of the field taken from the header
				 * @note Empty when the flag of the header is disabled, and likewise for the fields whose
				 *       number goes beyond the limits of the header
				 *
				 * \~
				 */
				string_view name;
				// Положение поля в исходном тексте
				location_t location;
				// Признак того, что поле было заключено в кавычки
				bool quoted;
				/**
				 * \~russian
				 * Признак того, что содержимое поля было изменено разбором
				 *
				 * @note Ставится снятием кавычек, сведением удвоенных кавычек и снятием
				 *       обвязки. Служит тому, кто желает выдать поле обратно ровно таким,
				 *       каким оно записано: при снятом признаке значение и есть исходное
				 *
				 * \~english
				 * Flag of the content of the field having been changed by the parsing
				 * @note Set by the removal of the quotes, by the reduction of the doubled quotes and by the removal of the
				 *       padding. Serves the one who wishes to issue the field back exactly as
				 *       it has been written: when the flag is not set, the value is the source one
				 *
				 * \~
				 */
				bool modified;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				Field() noexcept : quoted(false), modified(false) {}
			} field_t;

			/**
			 * \~russian
			 * @brief Метод получения сообщения об ошибке разбора
			 *
			 * @param error код ошибки разбора
			 * @return      сообщение об ошибке
			 *
			 * \~english
			 * @brief Method of getting the message of a parsing error
			 * @param error error code of the parsing
			 * @return      error message
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const char * message(const error_t error) noexcept;

			/**
			 * \~russian
			 * @brief Метод получения названия кодировки
			 *
			 * @param encoding кодировка исходного текста
			 * @return         название кодировки
			 *
			 * \~english
			 * @brief Method of getting the name of an encoding
			 * @param encoding encoding of the source text
			 * @return         name of the encoding
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const char * name(const encoding_t encoding) noexcept;

			/**
			 * \~russian
			 * @brief Метод определения кодировки по метке порядка байтов
			 *
			 * @param text начало исходного текста
			 * @return     определённая кодировка либо UTF-8 при отсутствии метки
			 *
			 * \~english
			 * @brief Method of determining the encoding by the byte order mark
			 * @param text beginning of the source text
			 * @return     determined encoding or UTF-8 in the absence of the mark
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ encoding_t encoding(const string_view text) noexcept;

			/**
			 * \~russian
			 * @brief Метод получения знака конца строки
			 *
			 * @param newline вид знака конца строки
			 * @return        последовательность знаков конца строки
			 *
			 * \~english
			 * @brief Method of getting the line ending character
			 * @param newline kind of the line ending character
			 * @return        sequence of the line ending characters
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ string_view newline(const newline_t newline) noexcept;

			/**
			 * \~russian
			 * @brief Метод проверки пригодности знака в разделители полей
			 *
			 * @details Разделителем не бывают ни кавычка, ни знаки конца строки, ни знак
			 * отмены: всякий из них уже занят разбором, и совпадение с ним делает текст
			 * неразбираемым
			 *
			 * @warning Судится СТОЛКНОВЕНИЕ настроек, и только оно: годен ли знак по
			 *          ГРАММАТИКЕ текста, посредник не спрашивает вовсе. Разбор же следует
			 *          RFC 4180, где содержимым поля дозволены лишь печатные знаки US-ASCII,
			 *          и всякий управляющий знак отвергает кодом `INVALID_CHARACTER` -
			 *          безразлично, стоит ли он разделителем либо в содержимом. Замер
			 *          01.09.2026 по всем 256 значениям: посредник принимает 252, а разбор
			 *          работает с 95, и расходятся приговоры на СТА ПЯТИДЕСЯТИ СЕМИ - это
			 *          28 управляющих знаков, знак отмены `0x7F` и все 128 знаков со
			 *          старшим разрядом (одиночный старший октет годной последовательностью
			 *          UTF-8 не бывает)
			 *
			 * @note Знак табуляции `0x09` в число расхождений НЕ входит: разбор принимает
			 *       его наравне с печатными, ибо он назван разделителем в самом ходу и
			 *       стоит в четвёрке знаков, по которым разделитель определяется
			 *
			 * @note Предсказывать этим посредником успех разбора НЕЛЬЗЯ. Для того надлежит
			 *       спрашивать сам разбор: истина здесь означает лишь «настройки друг другу
			 *       не противоречат», а не «текст таким разделителем разберётся»
			 *
			 * @warning Судится РАЗДЕЛИТЕЛЬ, а кавычка берётся лишь для сличения с ним.
			 *          Довод выше говорит, что знаки конца строки разбором уже заняты, - но
			 *          относится он к разделителю ОДНОМУ: кавычка, равная возврату каретки
			 *          либо переводу строки, посредником принимается. Замер 01.09.2026 снял
			 *          и следствие: обычный текст при такой кавычке разбирается ВЕРНО, ибо
			 *          конец строки распознаётся прежде кавычки, и кавычка просто перестаёт
			 *          работать. Отказа тут нет, и заводить его не на что, - но и обещания
			 *          годности кавычки посредник не даёт
			 *
			 * @param separator знак-разделитель полей
			 * @param quote     знак кавычек
			 * @return          результат проверки
			 *
			 * \~english
			 * @brief Method of checking the suitability of a character as a separator of the fields
			 * @details Neither a quote, nor the line ending characters, nor the escape
			 * character are ever a separator: every one of them is already occupied by the parsing, and a coincidence with it makes the text
			 * unparseable
			 * @param separator separator character of the fields
			 * @param quote     quote character
			 * @return          result of the check
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool suitable(const char separator, const char quote) noexcept;

			/**
			 * \~russian
			 * @brief Метод проверки необходимости заключить поле в кавычки
			 *
			 * @details Судится одно лишь СОДЕРЖИМОЕ поля: знак-разделитель, знак кавычек,
			 * знаки конца строки и пробельная обвязка по краям. Правила `ALL`, `NONE` и
			 * `NONNUMERIC` разбираются целиком, ибо содержимого им и довольно
			 *
			 * @warning Предсказывать выдачу записи этим посредником НЕЛЬЗЯ. Правило
			 *          `MINIMAL` знает ещё две причины обрамления, зависящие от МЕСТА
			 *          поля, - знак примечания в начале записи и метку порядка байтов в
			 *          начале текста, - а места поля посреднику не передаётся вовсе.
			 *          Замер 01.09.2026: поле «#а» при заданном знаке примечания
			 *          посредник объявляет обрамления не требующим, а запись его
			 *          обрамляет. Поле, записанное по приговору посредника голым,
			 *          прочлось бы обратно примечанием, унеся с собою всю запись
			 *
			 * @note Расхождение это НЕ дефект: посредник получает текст поля и ничего
			 *       более, и вывести из него место поля неоткуда. Названо оно здесь
			 *       затем, что молчание об области выглядит обещанием полноты
			 *
			 * @param text      содержимое поля
			 * @param separator знак-разделитель полей
			 * @param quote     знак кавычек
			 * @param quoting   правило заключения поля в кавычки
			 * @return          результат проверки
			 *
			 * \~english
			 * @brief Method of checking the necessity of enclosing a field in quotes
			 * @param text      content of the field
			 * @param separator separator character of the fields
			 * @param quote     quote character
			 * @param quoting   rule of enclosing a field in quotes
			 * @return          result of the check
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool quotable(const string_view text, const char separator, const char quote, const quoting_t quoting) noexcept;

			/**
			 * \~russian
			 * @brief Метод приведения содержимого поля к целому числу со знаком
			 *
			 * @details Судится НАПИСАНИЕ: годной признаётся запись целого числа и только
			 * она. Запись дробная целого числа - «300.0» - отвергается, равно как и запись
			 * с показателем степени «1e3», хотя оба означают число целое
			 *
			 * @warning Предсказывать этим посредником извлечение `document_t::numeric`
			 *          НЕЛЬЗЯ. Извлечение следует решению владельца от 30.08.2026: два
			 *          написания одного числа извлекаются одинаково, и «300.0» видом
			 *          `int64_t` даёт 300 там, где посредник отвечает отказом. Замер
			 *          01.09.2026: из шести написаний приговоры разошлись на двух - «300.0»
			 *          и «1e3»
			 *
			 * @note Расхождение это НЕ дефект: у посредника и у извлечения разные вопросы.
			 *       Посредник спрашивает «целым ли записано», извлечение - «целым ли
			 *       представимо». Названо оно здесь затем, что молчание об области
			 *       выглядит обещанием полноты
			 *
			 * @param text   содержимое поля
			 * @param result полученное значение
			 * @return       результат приведения
			 *
			 * \~english
			 * @brief Method of converting the content of a field to a signed integer
			 * @param text   content of the field
			 * @param result obtained value
			 * @return       result of the conversion
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool integer(const string_view text, int64_t & result) noexcept;

			/**
			 * \~russian
			 * @brief Метод приведения содержимого поля к целому числу без знака
			 *
			 * @details Судится НАПИСАНИЕ: годной признаётся запись целого числа и только
			 * она. Запись дробная целого числа - «300.0» - отвергается, равно как и запись
			 * с показателем степени «1e3», хотя оба означают число целое
			 *
			 * @warning Предсказывать этим посредником извлечение `document_t::numeric`
			 *          НЕЛЬЗЯ. Извлечение следует решению владельца от 30.08.2026: два
			 *          написания одного числа извлекаются одинаково, и «300.0» видом
			 *          `int64_t` даёт 300 там, где посредник отвечает отказом. Замер
			 *          01.09.2026: из шести написаний приговоры разошлись на двух - «300.0»
			 *          и «1e3»
			 *
			 * @note Расхождение это НЕ дефект: у посредника и у извлечения разные вопросы.
			 *       Посредник спрашивает «целым ли записано», извлечение - «целым ли
			 *       представимо». Названо оно здесь затем, что молчание об области
			 *       выглядит обещанием полноты
			 *
			 * @param text   содержимое поля
			 * @param result полученное значение
			 * @return       результат приведения
			 *
			 * \~english
			 * @brief Method of converting the content of a field to an unsigned integer
			 * @param text   content of the field
			 * @param result obtained value
			 * @return       result of the conversion
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool integer(const string_view text, uint64_t & result) noexcept;

			/**
			 * \~russian
			 * @brief Метод приведения содержимого поля к числу с плавающей точкой
			 *
			 * @param text   содержимое поля
			 * @param result полученное значение
			 * @return       результат приведения
			 *
			 * \~english
			 * @brief Method of converting the content of a field to a floating-point number
			 * @param text   content of the field
			 * @param result obtained value
			 * @return       result of the conversion
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool real(const string_view text, double & result) noexcept;

			/**
			 * \~russian
			 * @brief Метод приведения содержимого поля к логическому значению
			 *
			 * @param text   содержимое поля
			 * @param result полученное значение
			 * @return       результат приведения
			 *
			 * \~english
			 * @brief Method of converting the content of a field to a logical value
			 * @param text   content of the field
			 * @param result obtained value
			 * @return       result of the conversion
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool boolean(const string_view text, bool & result) noexcept;
		}
	}
}

/**
 * Возвращаем макросы, снятые в начале файла
 */
#include "../../sys/pop.hpp"

#endif // __AWH_CODEC_CSV_COMMON__
