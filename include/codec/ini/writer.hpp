/**
 * @file writer.hpp
 * @date 2026-08-09
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
 * @brief Заголовочный файл записи текста настроек INI — класс Writer, собирающий текст
 *        из объявлений разделов, свойств и примечаний по правилам выбранного наречия
 *
 * \~english
 * @brief Header file of the writing of an INI settings text — the Writer class, which assembles the text
 *        out of the section declarations, the properties and the comments by the rules of the chosen dialect
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_INI_WRITER__
#define __AWH_CODEC_INI_WRITER__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"

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
		 * @brief Пространство имён контейнера INI
		 *
		 *
		 * \~english
		 * @brief INI container namespace
		 *
		 * \~
		 */
		namespace ini {
			/**
			 * \~russian
			 * @brief Обращение с ограждением значения кавычками при записи
			 *
			 * \~english
			 * @brief Treatment of the fencing of a value with quotes at the writing
			 *
			 * \~
			 */
			enum class quoting_t : uint8_t {
				NEVER  = 0x00, // Значение кавычками не ограждается никогда
				AUTO   = 0x01, // Значение ограждается кавычками лишь по нужде
				ALWAYS = 0x02  // Значение ограждается кавычками всегда
			};

			/**
			 * \~russian
			 * @brief Класс записи текста настроек
			 *
			 * @details Собирает текст настроек из объявлений разделов, свойств и
			 * примечаний. Запись проверяет построение собираемого: имя, которое разбор
			 * прочитал бы иначе, чем оно записано, отвергается вместо того, чтобы
			 * попасть в текст и разойтись со смыслом
			 *
			 * @par Порядок работы
			 *
			 * @warning Значение, которое разбор без кавычек прочитал бы иначе - несущее
			 * пробельную обвязку, знак примечания либо знак конца строки, - ограждается
			 * кавычками либо записывается управляющими последовательностями. Когда ни
			 * то, ни другое настройками не разрешено, запись отвечает отказом, а не
			 * выдаёт текст, читаемый обратно неверно
			 *
			 *  @code{.cpp}
			 *  writer_t writer(log);
			 *
			 *  writer.comment("собрано автоматически");
			 *  writer.section("server");
			 *  writer.property("host", "127.0.0.1");
			 *  writer.property("port", "8080");
			 *
			 *  const string & text = writer.text();
			 *  @endcode
			 *
			 * \~english
			 * @brief Class of the writing of a settings text
			 * @details Assembles a settings text out of the section declarations, the properties and the
			 * comments. The writing checks the construction of what is being assembled: a name which the parsing
			 * would read otherwise than it is written is rejected instead of
			 * getting into the text and diverging from its meaning
			 * @par Order of the work
			 * @warning A value which the parsing without quotes would read otherwise — carrying
			 * a whitespace padding, a comment character or a line ending character — is fenced
			 * with quotes or is written with escape sequences. When neither
			 * the one nor the other is permitted by the settings, the writing answers with a refusal rather than
			 * issuing a text read back incorrectly
			 *
			 *  @code{.cpp}
			 *  writer_t writer(log);
			 *
			 *  writer.comment("built automatically");
			 *  writer.section("server");
			 *  writer.property("host", "127.0.0.1");
			 *  writer.property("port", "8080");
			 *
			 *  const string & text = writer.text();
			 *  @endcode
			 *
			 */
			typedef class __AWH_SHARED_EXPORT__ Writer {
				private:
					/**
					 * \~russian
					 * @brief Метод вывода сообщения об отказе в лог
					 *
					 * @details Код отказа остаётся доступен потребителю через error(): журнал
					 * его не заменяет, а лишь оповещает о случившемся
					 *
					 * \~english
					 * @brief Method of the output of the message about a refusal into the log
					 * @details The code of the refusal remains available to the consumer through error():
					 * the log does not replace it but merely notifies about what has happened
					 *
					 * \~
					 */
					void report() const noexcept;
				private:
					/**
					 * \~russian
					 * Объект для работы с логами
					 *
					 * \~english
					 * Object for working with logs
					 *
					 * \~
					 */
					const log_t * _log;
				public:
					/**
					 * \~russian
					 * @brief Настройки записи текста настроек
					 *
					 * @details Готовые наборы настроек для сложившихся наречий выдают
					 * методы @c windows(), @c python(), @c systemd() и @c git()
					 *
					 * \~english
					 * @brief Settings of the writing of a settings text
					 * @details The ready sets of the settings for the established dialects are issued by
					 * the @c windows(), @c python(), @c systemd() and @c git() methods
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Settings {
						// Знак, которым начинается примечание
						char marker;
						// Знак, разделяющий имя свойства и его значение
						char separator;
						// Знак, отделяющий имя подраздела при построении разделителем
						char delimiter;
						/**
						 * \~russian
						 * Признак того, что читающий признаёт свойства до первого раздела
						 *
						 * @note Свойство, записанное прежде объявления раздела, читающий,
						 * их не признающий, отвергает вовсе: собранный текст ему не
						 * прочитать. Признак взводят по наречию читающего, а не по желанию
						 * пишущего
						 *
						 * \~english
						 * Flag of the reader recognizing the properties before the first section
						 * @note A property written before a section declaration is rejected altogether by a reader
						 * that does not recognize them: it cannot read the assembled text.
						 * The flag is raised by the dialect of the reader rather than by the wish of the
						 * writer
						 *
						 * \~
						 */
						bool global;
						/**
						 * \~russian
						 * Признак того, что читающий признаёт добавление к перечню значений
						 *
						 * @note Запись «имя[] = значение» читающий, перечней не признающий,
						 * прочтёт именем со скобками и отвергнет его как ошибочно
						 * построенное. Признак взводят по наречию читающего: среди
						 * сложившихся наречий перечни этой записью не задаёт ни одно
						 *
						 * \~english
						 * Flag of the reader recognizing an addition to a list of the values
						 * @note The record «name[] = value» a reader that does not recognize the lists
						 * will read as a name with the brackets and will reject it as erroneously
						 * constructed. The flag is raised by the dialect of the reader: among
						 * the established dialects not one gives the lists by this notation
						 *
						 * \~
						 */
						bool arrays;
						/**
						 * \~russian
						 * Признак того, что читающий признаёт свойство без значения
						 *
						 * @note Свойство, записанное без разделителя и значения, читающий,
						 * таких не признающий, отвергает по отсутствию разделителя. Наречий
						 * с ним немного: из сложившихся его признаёт лишь Git
						 *
						 * \~english
						 * Flag of the reader recognizing a property without a value
						 * @note A property written without a separator and a value a reader
						 * that does not recognize such ones rejects by the absence of a separator. There are not many dialects
						 * with it: of the established ones only Git recognizes it
						 *
						 * \~
						 */
						bool valueless;
						/**
						 * \~russian
						 * Признак того, что читающий отбрасывает пробельную обвязку значения
						 *
						 * @note Задаёт обращение с пробелами в конце значения: отбрасывающему
						 * читающему они безразличны, а сохраняющий возьмёт их данными - и
						 * украшающий пробел, поставленный перед примечанием в конце строки,
						 * достался бы ему частью значения, отчего оно росло бы пробелом при
						 * каждом обороте «чтение - запись». Пробелы в начале значения
						 * отбрасываются всяким читающим, и признак их не касается
						 *
						 * \~english
						 * Flag of the reader discarding the whitespace padding of a value
						 * @note Gives the treatment of the spaces at the end of a value: for a discarding
						 * reader they are indifferent, while a preserving one will take them as data — and
						 * an adorning space put before a comment at the end of the line would
						 * go to it as a part of the value, from which it would grow by a space at
						 * every «reading — writing» turn. The spaces at the beginning of a value
						 * are discarded by every reader, and the flag does not concern them
						 *
						 * \~
						 */
						bool trim;
						/**
						 * \~russian
						 * Признак отбрасывания читающим пробельной обвязки имени раздела
						 *
						 * @note Задаёт не запись обвязки, а толкование её читающим. Читающий,
						 * обвязку отбрасывающий, прочтёт «[ раздел ]» именем «раздел», и
						 * записанное имя разошлось бы с прочитанным - оттого имя с обвязкой
						 * такой записи отвергается. Читающий же, обвязку сохраняющий (таково
						 * наречие python), прочтёт имя как записано, и отвергать его не за
						 * что: круг «чтение - запись» на нём рвался, хотя наречие текст
						 * принимало
						 *
						 * \~english
						 * Flag of the reader discarding the whitespace padding of a name of a section
						 * @note Gives the treatment of the padding by the reader, not the writing of it
						 *
						 * \~
						 */
						bool trimSections;
						/**
						 * \~russian
						 * Признак того, что читающий ищет закрывающую скобку объявления раздела до последней в строке
						 *
						 * @note Задаёт не запись скобок, а толкование их читающим. Читающий,
						 * берущий первую скобку, прочтёт «[x[]]» именем «x[» с лишним хвостом,
						 * и записанное имя разошлось бы с прочитанным - оттого имя со скобкой
						 * такой записи отвергается. Читающий же, берущий последнюю (таково
						 * наречие python), прочтёт имя «x[]» как записано, и отвергать его не
						 * за что: круг «чтение - запись» на нём рвался, хотя наречие текст
						 * принимало
						 *
						 * \~english
						 * Flag of the reader searching for the closing bracket of a section declaration up to the last one in the line
						 * @note Gives the treatment of the brackets by the reader, not the writing of them
						 *
						 * \~
						 */
						bool greedySections;
						/**
						 * \~russian
						 * Знаки, признаваемые читающим разделителем имени и значения
						 *
						 * @note Задаёт не запись разделителя, а толкование его читающим:
						 * пишется знак один - `separator`, - а признавать читающий вправе
						 * оба, и имя свойства, второй из них несущее, досталось бы ему
						 * разрезанным. Признак этот и `separator` расходятся намеренно
						 *
						 * \~english
						 * Characters recognized by a reader as the separator of a name and a value
						 * @note Gives the treatment of the separator by the reader, not the writing of it
						 *
						 * \~
						 */
						separator_t separators;
						/**
						 * \~russian
						 * Признак того, что читающий снимает кавычки со значения
						 *
						 * @note Задаёт не запись кавычек, а толкование их читающим: значение,
						 * кавычкой начинающееся, такой читающий прочтёт без неё, и записать
						 * её нужно управляющей последовательностью. Признак этот и
						 * @c quoting задают разное: первый - как прочтут, второй - как
						 * писать, и противоречие между ними запись обнаруживает сама.
						 * Настройки, отвечающие наречию разбора, выдаёт @c document_t по
						 * своим настройкам чтения
						 *
						 * \~english
						 * Flag of the reader removing the quotes from a value
						 * @note Gives not the writing of the quotes but their interpretation by the reader: a value
						 * beginning with a quote such a reader will read without it, and it needs to be written
						 * with an escape sequence. This flag and
						 * @c quoting give different things: the first — how it will be read, the second — how
						 * to write, and the contradiction between them is detected by the writing itself.
						 * The settings corresponding to the dialect of the parsing are issued by @c document_t from
						 * its reading settings
						 *
						 * \~
						 */
						bool quotes;
						/**
						 * \~russian
						 * Обращение с ограждением значения кавычками
						 *
						 * @note Запрет ограждения записи значения не отменяет: значение,
						 * которое читающий прочёл бы иначе - с пробельной обвязкой либо со
						 * знаком примечания внутри, - защищается тогда управляющими
						 * последовательностями, а при выключенной их записи отвергается.
						 * Молча выдать изменённое запись не вправе
						 *
						 * \~english
						 * Treatment of the fencing of a value with quotes
						 * @note A prohibition of the fencing does not cancel the writing of a value: a value
						 * which the reader would read otherwise — with a whitespace padding or with
						 * a comment character inside — is then protected by escape
						 * sequences, while when their writing is disabled it is rejected.
						 * The writing has no right to issue what has been changed silently
						 *
						 * \~
						 */
						quoting_t quoting;
						// Построение имени подраздела
						subsection_t subsections;
						// Вид знака конца строки собираемого текста
						newline_t newline;
						/**
						 * \~russian
						 * Флаг того, что читающий признаёт примечание в конце строки
						 *
						 * @note Задаёт не запись примечаний, а обращение со значением:
						 * знак примечания внутри значения требует ограждения лишь тогда,
						 * когда читающий его примечанием сочтёт. Наречия MS Windows и
						 * systemd такого примечания не признают, и точка с запятой в
						 * пути у них ограждения не требует
						 *
						 * \~english
						 * Flag of the reader recognizing a comment at the end of a line
						 * @note Gives not the writing of the comments but the treatment of a value:
						 * a comment character inside a value requires a fencing only when
						 * the reader considers it a comment. The dialects of MS Windows and of
						 * systemd do not recognize such a comment, and a semicolon in
						 * a path requires no fencing with them
						 *
						 * \~
						 */
						bool inlineComments;
						/**
						 * \~russian
						 * Признак того, что читающий требует пробела перед знаком примечания
						 *
						 * @note Задаёт не запись примечаний, а толкование их читающим: наречие
						 * Git пробела не требует, и знак примечания обрывает у него имя ГДЕ
						 * УГОДНО - имя `k#v` есть ему имя `k` с примечанием. Признак этот
						 * решает о допустимости знака примечания внутри имени
						 *
						 * \~english
						 * Flag of the reader requiring a space before a comment character
						 * @note Gives the treatment of the comments by the reader, not the writing of them
						 *
						 * \~
						 */
						bool spacedComments;
						/**
						 * \~russian
						 * Знаки, которые читающий признаёт началом примечания
						 *
						 * @note Задаёт не запись примечаний, а объём ограждения значений:
						 * писать примечания можно лишь одним знаком, а признавать читающий
						 * вправе оба. Наречие Git пишет примечания решёткой, но читает и
						 * точку с запятой - значение с нею оградить обязаны, иначе при
						 * обратном чтении оно обрежется
						 *
						 * \~english
						 * Characters which the reader recognizes as the beginning of a comment
						 * @note Gives not the writing of the comments but the volume of the fencing of the values:
						 * the comments can be written with only one character, while the reader has the right to recognize
						 * both. The Git dialect writes the comments with a hash but reads
						 * the semicolon as well — a value with it must be fenced, otherwise at
						 * the reading back it will be cut off
						 *
						 * \~
						 */
						marker_t comments;
						// Флаг записи пробелов вокруг разделителя имени и значения
						bool spaces;
						/**
						 * \~russian
						 * Признак того, что читающий разбирает управляющие последовательности
						 *
						 * @note Задаёт не одну лишь запись: обратная косая черта в значении
						 * требует записи последовательностью ровно тогда, когда читающий её
						 * последовательностью и сочтёт. Признак взводят по наречию читающего,
						 * а не по желанию пишущего - иначе значение с косой чертой достанется
						 * ему изменённым. Настройки, отвечающие наречию разбора, выдаёт
						 * @c document_t сам по своим настройкам чтения
						 *
						 * \~english
						 * Flag of the reader parsing the escape sequences
						 * @note Gives not the writing alone: a backslash in a value
						 * requires a writing as a sequence exactly when the reader considers it
						 * a sequence as well. The flag is raised by the dialect of the reader
						 * rather than by the wish of the writer — otherwise a value with a backslash will go to
						 * it changed. The settings corresponding to the dialect of the parsing are issued by
						 * @c document_t itself from its reading settings
						 *
						 * \~
						 */
						bool escapes;
						/**
						 * \~russian
						 * Признак того, что читающий склеивает строки по обратной косой черте
						 *
						 * @note Имя либо значение, оканчивающееся обратной косой чертой,
						 *       при таком чтении слилось бы со следующей строкой. Записать
						 *       его без потери можно лишь управляющей последовательностью,
						 *       и при выключенной их записи такое имя отвергается
						 *
						 * \~english
						 * Flag of the reader gluing the lines by a backslash
						 * @note A name or a value ending with a backslash,
						 *       at such a reading would merge with the next line. It can be written
						 *       without a loss only with an escape sequence,
						 *       and when their writing is disabled such a name is rejected
						 *
						 * \~
						 */
						bool continuations;
						/**
						 * \~russian
						 * Флаг записи отступа перед свойствами раздела
						 *
						 * @note Отступ этот украшающий, и при взведённом @c indents он не
						 * пишется вовсе: читающий, признающий продолжение отступом, принял
						 * бы строку с отступом за продолжение значения предыдущего свойства,
						 * и весь раздел слился бы в одно значение
						 *
						 * \~english
						 * Flag of the writing of an indent before the properties of a section
						 * @note This indent is an adorning one, and when @c indents is raised it is not
						 * written at all: a reader that recognizes a continuation by an indent would take
						 * a line with an indent for a continuation of the value of the previous property,
						 * and the whole section would merge into a single value
						 *
						 * \~
						 */
						bool indent;
						/**
						 * \~russian
						 * Флаг записи многострочного значения продолжением отступом
						 *
						 * @note Значение со знаком конца строки записать в одну строку
						 * нечем: наречие configparser записывает его строками продолжения
						 * с отступом, и лишь оно одно такое значение и порождает
						 *
						 * \~english
						 * Flag of the writing of a multiline value as a continuation by an indent
						 * @note There is nothing to write a value with a line ending character in a single line
						 * with: the configparser dialect writes it as continuation lines
						 * with an indent, and it alone generates such a value
						 *
						 * \~
						 */
						bool indents;
						// Флаг записи пустой строки перед объявлением раздела
						bool separated;
						/**
						 * \~russian
						 * Наибольшая допустимая длина имени раздела или свойства в байтах,
						 * ноль - без предела
						 *
						 * @note Предел тот же, что и у разбора: имя длиннее записать значило
						 * бы собрать текст, который читающий отвергнет. Ноль его снимает - тем
						 * же образом, каким его снимает разбор
						 *
						 * \~english
						 * Largest admissible length of the name of a section or of a property in bytes,
						 * zero — without a limit
						 * @note The limit is the same as that of the parsing: to write a longer name would
						 * mean to assemble a text which the reader will reject. Zero removes it — in the
						 * same way in which the parsing removes it
						 *
						 * \~
						 */
						uint32_t maxName;
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
						Settings() noexcept;
						/**
						 * \~russian
						 * @brief Метод получения настроек наречия MS Windows
						 *
						 * @return настройки записи наречия MS Windows
						 *
						 * \~english
						 * @brief Method of getting the settings of the MS Windows dialect
						 * @return settings of the writing of the MS Windows dialect
						 *
						 * \~
						 */
						static Settings windows() noexcept;
						/**
						 * \~russian
						 * @brief Метод получения настроек наречия configparser языка Python
						 *
						 * @return настройки записи наречия configparser
						 *
						 * \~english
						 * @brief Method of getting the settings of the configparser dialect of the Python language
						 * @return settings of the writing of the configparser dialect
						 *
						 * \~
						 */
						static Settings python() noexcept;
						/**
						 * \~russian
						 * @brief Метод получения настроек наречия описания служб systemd
						 *
						 * @return настройки записи наречия systemd
						 *
						 * \~english
						 * @brief Method of getting the settings of the dialect of the systemd unit files
						 * @return settings of the writing of the systemd dialect
						 *
						 * \~
						 */
						static Settings systemd() noexcept;
						/**
						 * \~russian
						 * @brief Метод получения настроек наречия настроек Git
						 *
						 * @return настройки записи наречия Git
						 *
						 * \~english
						 * @brief Method of getting the settings of the dialect of the Git settings
						 * @return settings of the writing of the Git dialect
						 *
						 * \~
						 */
						static Settings git() noexcept;
					} settings_t;
				private:
					// Код ошибки последней операции записи
					error_t _error;
				private:
					// Признак того, что раздел текста настроек уже объявлен
					bool _sectioned;
					/**
					 * \~russian
					 * Признак того, что последней записанной строкой было свойство
					 *
					 * @details Примечание в конце строки читающий признаёт лишь тогда, когда
					 * знаку его начала предшествует пробел, а пробел этот при читающем,
					 * пробельной обвязки не отбрасывающем, достаётся значению свойства
					 * данными. Дописать примечание к строке свойства там нечем, и оно
					 * уходит отдельной строкой: место примечания украшение, а значение -
					 * записанное потребителем
					 *
					 * \~english
					 * Flag of the last written line having been a property
					 * @details A comment at the end of a line the reader recognizes only when
					 * the character of its beginning is preceded by a space, while that space, with a reader
					 * that does not discard the whitespace padding, goes to the value of the property
					 * as data. There is nothing to append a comment to a property line with there, and it
					 * goes out as a separate line: the place of a comment is an adornment, while the value is
					 * what has been written by the consumer
					 *
					 * \~
					 */
					bool _valued;
					/**
					 * \~russian
					 * Признак того, что записанное примечание оканчивается продолжением
					 *
					 * @details Примечание, оканчивающееся обратной косой чертой, читающий
					 * склеивает со следующей строкой - и та пропадает из его выдачи вовсе:
					 * объявление раздела, ставшее хвостом примечания, читающий разделом уже
					 * не сочтёт. Ограждается такое примечание единственным способом -
					 * пустой строкой, склеивание прекращающей: управляющих
					 * последовательностей примечание не признаёт, а обрезать содержимое
					 * запись не вправе
					 *
					 * @note Пустая строка ставится не сразу, а перед следующей записью: в
					 * конце текста склеивать нечего, и лишней строки там не появляется. Не
					 * появляется она и перед пустой строкой, записанной самим потребителем,
					 * - иначе текст рос бы пустыми строками с каждой перезаписью
					 *
					 * \~english
					 * Flag of the written comment ending with a continuation
					 * @details A comment ending with a backslash the reader
					 * glues with the next line — and that line disappears from its output altogether:
					 * a section declaration that has become the tail of a comment the reader will no longer consider
					 * a section. Such a comment is fenced by a single means —
					 * by an empty line terminating the gluing: a comment does not recognize the escape
					 * sequences, while the writing has no right to truncate the
					 * content
					 * @note The empty line is put not at once but before the next record: at
					 * the end of the text there is nothing to glue, and no superfluous line appears there. It does
					 * not appear before an empty line written by the consumer itself either
					 * — otherwise the text would grow by empty lines with every rewriting
					 *
					 * \~
					 */
					bool _guarded;
				private:
					// Собираемый текст настроек
					string _text;
				private:
					// Настройки записи текста настроек
					/**
					 * \~russian
					 * @brief Разметка конца строки, ПОСЛЕДНЕЙ записью на деле применённая
					 *
					 * @details Дописка примечания снимает знак конца строки с собранного текста,
					 * и снимать она обязана ровно то, что было записано, - а не то, что велит
					 * настройка НЫНЕ: настройка вправе смениться между записью строки и допиской
					 * к ней, и суд по ней снимал бы не ту последовательность
					 *
					 * @warning Замерено щупом: при смене разметки с CRLF на LF дописка снимала
					 * один перевод строки из двух знаков, оставляя одинокий возврат каретки
					 * ПОСРЕДИ строки, - и отвечала УСПЕХОМ. Обратная смена, с LF на CRLF, давала
					 * отказ кодом INTERNAL. Довод этот - «о написанном нельзя судить по
					 * настройке» - принесён Василием от кодека JSON, где та же беда склеивала
					 * документы потока
					 *
					 * \~english
					 * @brief Line ending actually applied by the last writing
					 * @details The appending of a comment removes the line ending from the assembled text,
					 * and it must remove exactly what was written rather than what the settings demand NOW
					 *
					 * \~
					 */
					newline_t _written;
				private:
					settings_t _settings;
				private:
					/**
					 * \~russian
					 * @brief Метод проверки имени раздела или свойства
					 *
					 * @details Имя, которое разбор прочитал бы иначе, чем оно записано,
					 * отвергается: знак конца строки, знак примечания и разделитель имени
					 * со значением в имени недопустимы
					 *
					 * @param name    проверяемое имя раздела или свойства
					 * @param section признак проверки имени раздела
					 * @return        результат выполнения операции
					 *
					 * \~english
					 * @brief Method of checking the name of a section or of a property
					 * @details A name which the parsing would read otherwise than it is written
					 * is rejected: a line ending character, a comment character and the separator of a name
					 * from a value are inadmissible in a name
					 * @param name    name of the section or of the property being checked
					 * @param section flag of the checking of the name of a section
					 * @return        result of performing the operation
					 *
					 * \~
					 */
					bool verify(const string_view name, const bool section) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи значения свойства
					 *
					 * @details Ограждает значение кавычками либо записывает его
					 * управляющими последовательностями - в объёме, разрешённом
					 * настройками записи
					 *
					 * @param value записываемое значение свойства
					 * @return      результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing the value of a property
					 * @details Fences the value with quotes or writes it with
					 * escape sequences — in the volume permitted by
					 * the settings of the writing
					 * @param value value of the property being written
					 * @return      result of performing the operation
					 *
					 * \~
					 */
					bool escape(const string_view value) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи свойства со значением
					 *
					 * @details Запись свойства обычного и запись добавления к перечню значений
					 * разнятся лишь двумя местами - проверкой признания перечней читающим да
					 * скобками за именем, - а прочее у них общее. Держать их двумя телами значило
					 * бы развести своды правил при первой же правке одного из них
					 *
					 * @param key   имя записываемого свойства
					 * @param value значение записываемого свойства
					 * @param array признак записи добавления к перечню значений
					 * @return      результат выполнения операции
					 *
					 * \~english
					 * @brief Method of the writing of a property with a value
					 * @details The writing of an ordinary property and the writing of an appending to an array
					 * of the values differ only in two places — the check of the recognition of the arrays by
					 * the reader and the brackets after the name — while the rest is common to them. Keeping
					 * them as two bodies would mean parting the sets of the rules at the very first edit of one of them
					 * @param key   name of the property being written
					 * @param value value of the property being written
					 * @param array flag of the writing of an appending to an array of the values
					 * @return      result of the performing of the operation
					 *
					 * \~
					 */
					bool emit(const string_view key, const string_view value, const bool array) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи знака конца строки
					 *
					 * \~english
					 * @brief Method of writing the line ending character
					 *
					 * \~
					 */
					void newline() noexcept;
					/**
					 * \~russian
					 * @brief Метод ограждения примечания, оканчивающегося продолжением
					 *
					 * @details Ставит пустую строку перед очередной записью, если записанное
					 * прежде примечание оканчивается обратной косой чертой: без неё читающий
					 * склеил бы очередную строку с примечанием и потерял бы её содержимое
					 *
					 * @param blank признак того, что очередной записью идёт пустая строка,
					 *              ограждением служащая сама по себе
					 *
					 * \~english
					 * @brief Method of fencing a comment ending with a continuation
					 * @details Puts an empty line before the next record if the comment written
					 * before ends with a backslash: without it the reader
					 * would glue the next line with the comment and would lose its content
					 * @param blank flag of the next record being an empty line,
					 *              which serves as a fencing by itself
					 *
					 * \~
					 */
					void guard(const bool blank = false) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения текущих настроек записи
					 *
					 * @return текущие настройки записи текста настроек
					 *
					 * \~english
					 * @brief Method of getting the current settings of the writing
					 * @return current settings of the writing of a settings text
					 *
					 * \~
					 */
					const settings_t & settings() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки настроек записи
					 *
					 * @param settings настройки записи текста настроек
					 *
					 * \~english
					 * @brief Method of setting the settings of the writing
					 * @param settings settings of the writing of a settings text
					 *
					 * \~
					 */
					void settings(const settings_t & settings) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод записи объявления раздела
					 *
					 * @param section    имя записываемого раздела
					 * @param subsection имя записываемого подраздела
					 * @return           результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing a section declaration
					 * @param section    name of the section being written
					 * @param subsection name of the subsection being written
					 * @return           result of performing the operation
					 *
					 * \~
					 */
					bool section(const string_view section, const string_view subsection = "") noexcept;
					/**
					 * \~russian
					 * @brief Метод записи свойства со значением
					 *
					 * @param key   имя записываемого свойства
					 * @param value значение записываемого свойства
					 * @return      результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing a property with a value
					 * @param key   name of the property being written
					 * @param value value of the property being written
					 * @return      result of performing the operation
					 *
					 * \~
					 */
					bool property(const string_view key, const string_view value) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи свойства добавлением к перечню значений
					 *
					 * @details Записывает свойство в виде «имя[] = значение» - записью,
					 * которой наречие PHP задаёт добавление значения к перечню. Читающему
					 * такую запись признаёт настройка @c arrays
					 *
					 * @warning Запись эта признаётся не всяким наречием: читающий без
					 * @c arrays отвергнет её как имя с недопустимым знаком
					 *
					 * @param key    имя записываемого свойства
					 * @param value  записываемое значение свойства
					 * @param append признак добавления значения к перечню
					 * @return       результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing a property as an addition to a list of the values
					 * @details Writes the property in the form «name[] = value» — the notation
					 * by which the PHP dialect gives an addition of a value to a list. For a reader
					 * such a notation is recognized by the @c arrays setting
					 * @warning This notation is recognized not by every dialect: a reader without
					 * @c arrays will reject it as a name with an inadmissible character
					 * @param key    name of the property being written
					 * @param value  value of the property being written
					 * @param append flag of the addition of the value to a list
					 * @return       result of performing the operation
					 *
					 * \~
					 */
					bool property(const string_view key, const string_view value, const bool append) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи свойства без разделителя и значения
					 *
					 * @details Запись эта принята настройками Git и означает истину.
					 * Прочие наречия её не признают, и в них применять её не следует
					 *
					 * @param key имя записываемого свойства
					 * @return    результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing a property without a separator and a value
					 * @details This notation is accepted by the Git settings and means truth.
					 * The other dialects do not recognize it, and it should not be applied in them
					 * @param key name of the property being written
					 * @return    result of performing the operation
					 *
					 * \~
					 */
					bool property(const string_view key) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи примечания
					 *
					 * @details Примечание, занимающее несколько строк, записывается
					 * несколькими строками примечания: знак его начала ставится к каждой
					 *
					 * @param text содержимое записываемого примечания
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing a comment
					 * @details A comment occupying several lines is written as
					 * several comment lines: the character of its beginning is put to each of them
					 * @param text content of the comment being written
					 * @return     result of performing the operation
					 *
					 * \~
					 */
					bool comment(const string_view text) noexcept;
					/**
					 * \~russian
					 * @brief Метод дописывания примечания к последней записанной строке
					 *
					 * @details Примечание дописывается в конец уже записанной строки -
					 * туда, где его оставил человек, писавший файл настроек руками.
					 * Служит перезаписи файла без обеднения его примечаниями
					 *
					 * @warning Примечание такое признаётся не всяким наречием: у MS Windows
					 * и systemd оно станет частью значения. Дописывать его следует лишь
					 * тогда, когда читающий его примечанием сочтёт
					 *
					 * @param text содержимое дописываемого примечания
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of appending a comment to the last written line
					 * @details The comment is appended to the end of an already written line —
					 * there where it has been left by the human who wrote the settings file by hand.
					 * Serves the rewriting of a file without impoverishing it of the comments
					 * @warning Such a comment is recognized not by every dialect: with MS Windows
					 * and systemd it will become a part of the value. It should be appended only
					 * when the reader considers it a comment
					 * @param text content of the comment being appended
					 * @return     result of performing the operation
					 *
					 * \~
					 */
					bool trailing(const string_view text) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи пустой строки
					 *
					 * @return результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing an empty line
					 * @return result of performing the operation
					 *
					 * \~
					 */
					bool blank() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Шаблон типа записываемого числа
					 *
					 * @tparam T тип записываемого числа
					 *
					 * \~english
					 * @brief Template of the type of the number being written
					 * @tparam T type of the number being written
					 *
					 * \~
					 */
					template <typename T>
					/**
					 * \~russian
					 * @brief Метод записи свойства с числовым значением
					 *
					 * @details Число записывается по правилам местности «C» и от
					 * установленной в приложении местности не зависит: разделитель
					 * дробной части местности сделал бы текст настроек непереносимым
					 *
					 * @param key   имя записываемого свойства
					 * @param value значение записываемого свойства
					 * @return      результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing a property with a numeric value
					 * @details The number is written by the rules of the «C» locale and does not depend on the
					 * locale set in the application: the decimal separator of a locale
					 * would make the settings text non-portable
					 * @param key   name of the property being written
					 * @param value value of the property being written
					 * @return      result of performing the operation
					 *
					 * \~
					 */
					bool number(const string_view key, const T value) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения кода ошибки записи
					 *
					 * @details Отказ записи ЛИПКИЙ: писатель, отказом задетый, дальнейших записей
					 * не принимает вовсе, а код отказа держит до сброса. Иначе потребитель, кода
					 * не сверивший, получил бы текст без отвергнутой части и пропажи не заметил
					 *
					 * @note Договор этот один у INI, TOML и YAML: расхождение их сведено решением
					 * владельца. Закреплён он проверкой RefusalLocksWriter, перебирающей ВСЕ
					 * методы записи открытого договора - метод, сторожа не получивший, выпал бы
					 * из договора молча
					 *
					 * @note Сброс clear() код отказа отпускает: переживший его, он указывал бы на
					 * причину, которой более нет
					 *
					 * @return код ошибки последней операции записи
					 *
					 * \~english
					 * @brief Method of getting the error code of the writing
					 * @return error code of the last operation of the writing
					 *
					 * \~
					 */
					error_t error() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения собранного текста настроек
					 *
					 * @return собранный текст настроек
					 *
					 * \~english
					 * @brief Method of getting the assembled settings text
					 * @return assembled settings text
					 *
					 * \~
					 */
					const string & text() const noexcept;
					/**
					 * \~russian
					 * @brief Метод сброса записи в исходное состояние
					 *
					 * \~english
					 * @brief Method of resetting the writing into the initial state
					 *
					 * \~
					 */
					void clear() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param log объект для работы с логами
					 *
					 * \~english
					 * @brief Constructor
					 * @param log object for working with logs
					 *
					 * \~
					 */
					Writer(const log_t * log) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param log      объект для работы с логами
					 * @param settings настройки записи текста настроек
					 *
					 * \~english
					 * @brief Constructor
					 * @param log      object for working with logs
					 * @param settings settings of the writing of a settings text
					 *
					 * \~
					 */
					Writer(const log_t * log, const settings_t & settings) noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					~Writer() noexcept;
			} writer_t;
		};
	};
};

#endif // __AWH_CODEC_INI_WRITER__
