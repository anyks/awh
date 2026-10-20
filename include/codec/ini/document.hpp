/**
 * @file document.hpp
 * @date 2026-08-10
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
 * @brief Заголовочный файл дерева настроек INI — класс Document, хранящий разобранный текст
 *        вместе с его оформлением, с поиском по разделам и свойствам, подстановкой обращений
 *        к значениям и правкой на месте с обратной записью
 *
 * \~english
 * @brief Header file of the INI settings tree — the Document class, which stores the parsed text
 *        together with its formatting, with a search by the sections and the properties, with the substitution of the references
 *        to the values and with an in-place editing with a writing back
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#pragma once

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <type_traits>
#include <cstdint>
#include <string_view>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"
#include "reader.hpp"
#include "writer.hpp"

/**
 * Подключаем заголовочный файл проекта
 */
#include "../../sys/fs.hpp"

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
	 * @brief Пространство имён контейнеров данных
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
		 * \~english
		 * @brief INI container namespace
		 *
		 * \~
		 */
		namespace ini {
			/**
			 * \~russian
			 * @brief Вид записи текста настроек
			 *
			 * \~english
			 * @brief Kind of a record of a settings text
			 *
			 * \~
			 */
			enum class kind_t : uint8_t {
				NONE     = 0x00, // Запись удалена и в выдачу не идёт
				SECTION  = 0x01, // Объявление раздела
				PROPERTY = 0x02, // Свойство со значением
				COMMENT  = 0x03, // Примечание
				BLANK    = 0x04  // Пустая строка
			};

			/**
			 * \~russian
			 * @brief Предобъявление владеющего значения настроек
			 *
			 * \~english
			 * @brief Forward declaration of the owning value of the settings
			 *
			 * \~
			 */
			class Value;

			/**
			 * \~russian
			 * @brief Класс дерева настроек
			 *
			 * @details Хранит разобранный текст настроек целиком - вместе с примечаниями,
			 *          пустыми строками и порядком записей, - и позволяет читать значения по имени,
			 *          править их на месте и записывать обратно.
			 *
			 * @par Сохранение оформления
			 *
			 *      Файл настроек правит человек, и переписать его, потеряв примечания либо
			 *      переставив разделы, значит отнять у хозяина то, ради чего файл этот
			 *      читаемым и заведён. Оттого дерево держит **все** записи исходного текста в
			 *      порядке их следования, а правка меняет лишь то, что ей велено
			 *
			 * @par Порядок работы
			 *
			 * @warning Выдаваемые последовательности знаков ссылаются на хранилище дерева и остаются пригодными **до
			 *          первой правки**: @c set(), @c erase() и @c create() вправе перестроить хранилище. Значение,
			 *          нужное после правки, следует скопировать
			 *
			 * @par Намеренные решения
			 *
			 * @li **Повторное свойство хранится целиком.** Все объявления попадают в
			 *     дерево, а настройка обращения с повторами решает лишь то, какое из них
			 *     выдаёт @c get(). Перечень всех значений выдаёт @c values() независимо от
			 *     этой настройки: наречия Git и systemd повтором задают перечень, и терять
			 *     его при разборе нельзя
			 *
			 * @li **Подстановка обращений пересчитывается лениво.** Значение, на которое
			 *     ссылаются, вправе стоять ниже по тексту, и разрешить обращение можно лишь
			 *     тогда, когда текст разобран целиком; поэтому разбор разрешает обращения
			 *     разом, по своём завершении. Правка же дерева подстановку не пересчитывает,
			 *     а лишь помечает устаревшей: правок подряд бывает много, и пересчёт после
			 *     каждой обращает сборку дерева в квадратичную. Пересчёт выполняется перед
			 *     первым чтением значения и ведётся от значений **до** подстановки, иначе
			 *     подставленное подставлялось бы дважды
			 *
			 * @li **Обращение, ставшее неразрешимым после правки, отвергает запись, но
			 *     не правку.** Удаление значения, на которое ссылаются, проходит, и @c get()
			 *     выдаёт значение в том виде, как оно записано, вместе с самим обращением.
			 *     А вот @c dump() отвечает отказом @c error_t::UNKNOWN_REFERENCE: разбор
			 *     такой текст с включённой подстановкой не примет, и выдать его значило бы
			 *     отдать потребителю заведомо негодный файл
			 *
			 * @li **Дерево отвергает лишь то, чего не сделает записываемым НИ ОДНА
			 *     настройка.** Критерий этот объясняет обе стороны разом: заставы имени свойства
			 *     с разделителем и имени раздела со знаком-разделителем стоят, ибо построение
			 *     такое недостижимо РАЗБОРОМ при любых настройках; застава же негодной
			 *     последовательности UTF-8 снята 08.09.2026, ибо правилом @c malformed_t
			 *     содержимое это записываемо при @c REPLACE и @c PASS. Стояла она верно, покуда
			 *     запись отвергала негодную кодировку безусловно, - основание её сняла смена
			 *     правила, а не оплошность
			 *
			 * @li **Годность значения к записи решает ЗАПИСЬ, а не правка.** Значение, какое читающий прочёл бы
			 *     иначе, а оградить его настройками нечем - пробельная обвязка при отбрасывании её, знак примечания
			 *     при внутристрочных примечаниях, перевод строки без продолжений и отмен, - деревом принимается, а
			 *     отвергается выдачей текста. Дерево от того выходит незаписываемым целиком: @c dump() отвечает
			 *     пустотою, и прежнее содержимое уходит вместе с нею. Решение это НАМЕРЕННОЕ и закреплено проверками
			 *
			 *     @c WriteFailure, @c UnquotableValues и @c WritingKeptQuotes. Заведение заставы на пути правки
			 *     пробовалось 07.09.2026 пробною записью - решение бралось у самой записи, не заводя второго
			 *     написания правил, - и было отвергнуто: правило это договор, а не оплошность. Заставы правки стоят
			 *     лишь там, где построение недостижимо РАЗБОРОМ: имя свойства с разделителем, имя раздела с
			 *     знаком-разделителем, свойство вне разделов при читающем, таких свойств не признающем
			 *
			 * @li **Удалённая запись остаётся в хранилище надгробием.** Изъятие её из
			 *     середины сдвинуло бы порядковые номера всех записей за нею, а на них
			 *     ссылаются указатели поиска. Надгробие обходится дешевле перестроения и
			 *     при записи в выдачу не идёт
			 *
			 *  @code{.cpp}
			 *          document_t document();
			 *
			 *  if(document.parse(text)){
			 *    const string_view host = document.get("host", "server");
			 *    document.set("port", "9090", "server");
			 *    const string result = document.text();
			 *  }
			 *
			 *  @endcode
			 *
			 * \~english
			 * @brief Class of the settings tree
			 *
			 * @details Stores the parsed settings text in full — together with the comments,
			 *          the empty lines and the order of the records — and makes it possible to read the values by a name,
			 *          to edit them in place and to write them back.
			 *
			 * @par Preservation of the formatting
			 *      A settings file is edited by a human, and to rewrite it, having lost the comments or
			 *      having rearranged the sections, means to take away from the owner that for the sake of which this file
			 *      has been made readable in the first place. Because of that the tree keeps **all** the records of the source text in
			 *      the order of their succession, while an editing changes only what it has been ordered to
			 *
			 * @par Order of the work
			 *
			 * @warning The issued sequences of characters refer to the storage of the tree and remain valid **until
			 *          the first editing**: @c set(), @c erase() and @c create() have the right to rebuild the
			 *          storage. A value needed after an editing should be copied
			 *
			 * @par Deliberate decisions
			 *
			 * @li **A repeated property is stored in full.** All the declarations get into the tree, while the
			 *     setting of the treatment of the repetitions decides only which of them @c get() issues. The list
			 *     of all the values is issued by @c values() independently of this setting: the Git and systemd
			 *     dialects give a list by a repetition, and to lose it at the parsing is impermissible
			 *
			 * @li **The substitution of the references is recomputed lazily.** A value being
			 *     referred to has the right to stand lower in the text, and a reference can be resolved only
			 *     when the text has been parsed in full; therefore the parsing resolves the references
			 *     all at once, upon its completion. An editing of the tree, however, does not recompute the substitution
			 *     but only marks it stale: there happen to be many editings in a row, and a recomputation after
			 *     each of them turns the assembly of the tree into a quadratic one. The recomputation is performed before
			 *     the first reading of a value and is conducted from the values **before** the substitution, otherwise
			 *     what has been substituted would be substituted twice
			 *
			 * @li **A reference that has become unresolvable after an editing rejects the writing but
			 *     not the editing.** A removal of a value being referred to goes through, and @c get()
			 *     issues the value in the form in which it has been written, together with the reference itself.
			 *     But @c dump() answers with an @c error_t::UNKNOWN_REFERENCE refusal: the parsing
			 *     will not accept such a text with the substitution enabled, and to issue it would mean
			 *     to give the consumer a knowingly unfit file
			 *
			 * @li **The tree rejects only that which NO setting can make writable.** This criterion
			 *     explains both sides at once: the guards of a property name with a separator and of a
			 *     section name with a separator character stand, because such a construction is unreachable
			 *     by the PARSING under any settings; while the guard of a malformed UTF-8 sequence was lifted
			 *     on 2026-09-08, because by the @c malformed_t rule this content is writable under @c REPLACE
			 *     and @c PASS. It stood rightly as long as the writing rejected a malformed encoding
			 *     unconditionally - it was a change of the rule, not an oversight, that removed its ground
			 *
			 * @li **The fitness of a value for the writing is decided by the WRITING, not by the editing.** A value
			 *     which the reading would have read differently, and which there is no setting to fence off - the
			 *     whitespace wrapping when it is dropped, a comment character with inline comments, a line ending
			 *     without continuations and cancellations - is accepted by the tree, and rejected by the issuing of
			 *     the text. The tree thereby turns unwritable as a whole: @c dump() answers with emptiness, and the
			 *     previous content goes away together with it. This decision is DELIBERATE and is fixed by the checks
			 *
			 *     @c WriteFailure, @c UnquotableValues and @c WritingKeptQuotes. The instituting of a guard
			 *     on the path of the editing was tried on 2026-09-07 by a trial writing - the decision was taken
			 *     from the writing itself, without instituting a second rendition of the rules - and was rejected:
			 *     this rule is a contract, and not an oversight. The guards of the editing stand only where the
			 *     construction is unreachable by the PARSING: a property name with a separator, a section name with
			 *     a separator character, a property outside the sections under a reading not recognizing such
			 *     properties
			 *
			 * @li **A removed record remains in the storage as a tombstone.** Its extraction from
			 *     the middle would shift the ordinal numbers of all the records after it, while the pointers of the search
			 *     refer to them. A tombstone comes cheaper than a rebuilding and
			 *     does not go into the output at the writing
			 *
			 *  @code{.cpp}
			 *          document_t document();
			 *
			 *  if(document.parse(text)){
			 *    const string_view host = document.get("host", "server");
			 *    document.set("port", "9090", "server");
			 *    const string result = document.text();
			 *  }
			 *
			 *  @endcode
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Document {
				private:
					/**
					 * \~russian
					 * @brief Владеющее значение, дереву доступ дающее
					 *
					 * @details Доступ этот нужен переносу: ведётся он на копии дерева ради
					 *          целости своей, а код отказа по неудаче обязан достаться дереву
					 *          ИСХОДНОМУ - копия с ним пропадает, и потребитель остался бы с
					 *          отказом без названной причины.
					 *
					 * \~english
					 * @brief Owning value granted the access to the tree
					 *
					 * @details This access is needed by the grafting: it is conducted on a copy of the tree
					 *          for the sake of its own integrity, while the code of the refusal must reach the
					 *          ORIGINAL tree — the copy vanishes with it.
					 *
					 * \~
					 */
					friend class Value;
				public:
					/**
					 * \~russian
					 * @brief Структура настроек дерева настроек
					 *
					 * \~english
					 * @brief Structure of the settings tree
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Settings {
						// Наибольший допустимый общий объём подстановки значений в байтах
						uint64_t maxExpansion;
						/**
						 * \~russian
						 * Наибольшая допустимая глубина вложенности обращений
						 *
						 * @note Имя носит слово «обращений» неспроста: у читающего есть своя
						 *       настройка maxDepth, и та стережёт вложенность ПОДРАЗДЕЛОВ. Настройки
						 *       эти вложены одна в другую - «settings.maxDepth» рядом с
						 *       «settings.reader.maxDepth», - и общее имя выходило бы западнёю:
						 *       потребитель, правящий не ту из них, отказа не получит и подмены не
						 *       заметит
						 *
						 * \~english
						 * Largest admissible depth of the nesting of the references
						 *
						 * @note The name carries the word «reference» for a reason: the reader has its own
						 *       maxDepth setting, and that one guards the nesting of the SUBSECTIONS. These
						 *       settings are nested one inside the other — «settings.maxDepth» beside
						 *       «settings.reader.maxDepth» — and a common name would be a trap: a consumer that
						 *       corrects the wrong one of them will not receive a refusal and will not notice
						 *       the substitution
						 *
						 * \~
						 */
						uint32_t maxReferenceDepth;
						// Построение обращения к значению другого свойства
						reference_t references;
						// Настройки записи текста разметки
						writer_t::settings_t writer;
						// Настройки разбора текста разметки
						reader_t::settings_t reader;
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						explicit Settings() noexcept;
					} settings_t;
				private:
					/**
					 * \~russian
					 * @brief Структура раздела разобранного текста настроек
					 *
					 * \~english
					 * @brief Structure of the section containing the parsed settings text
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Section {
						/**
						 * \~russian
						 * Признак того, что раздел объявлен в тексте настроек
						 *
						 * @note Раздел без имени заводится всегда: свойства, записанные до
						 *       первого объявления, принадлежат ему. Объявленным он при этом не
						 *       является и в перечень разделов не идёт
						 *
						 * \~english
						 * Flag of the section having been declared in the settings text
						 *
						 * @note A section without a name is always created: the properties written before
						 *       the first declaration belong to it. It is thereby not a declared one
						 *       and does not go into the list of the sections
						 *
						 * \~
						 */
						bool declared;
						// Место имени раздела в хранилище знаков
						span_t name;
						// Место имени подраздела в хранилище знаков
						span_t subsection;
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						explicit Section() noexcept;
					} section_t;
					/**
					 * \~russian
					 * @brief Структура записи разобранного текста настроек
					 *
					 * @details Место значений хранится отрезком общего хранилища знаков:
					 *          хранилище при правке дописывается и перемещается, обесценивая
					 *          ссылки на своё содержимое.
					 *
					 * \~english
					 * @brief Structure of the entry for the parsed configuration text
					 *
					 * @details The place of the values is stored as a segment of the common storage of the characters:
					 *          the storage is appended to at an editing and is moved, invalidating
					 *          the references to its content.
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Record {
						// Признак свойства, записанного добавлением к перечню значений
						bool append;
						/**
						 * \~russian
						 * Признак того, что значение было заключено в кавычки
						 *
						 * @note Держится ради обратной записи наравне со знаком примечания:
						 *       кавычки, поставленные человеком, при перезаписи сохраняются, даже
						 *       когда значение в них более не нуждается
						 *
						 * \~english
						 * Flag of the value having been enclosed in quotes
						 *
						 * @note It is kept for the sake of the writing back on a par with the comment character:
						 *       the quotes put by a human are preserved at a rewriting even
						 *       when the value no longer needs them
						 *
						 * \~
						 */
						bool quoted;
						// Признак свойства, записанного без разделителя и значения
						bool valueless;
						// Знак, которым начато примечание
						char marker;
						/**
						 * \~russian
						 * Место значения свойства до подстановки обращений
						 *
						 * @note Держится ради обратной записи: записать подставленное
						 *       значение значило бы разрешить обращение навсегда, а оно на то
						 *       и записано, чтобы разрешаться заново при каждом чтении файла
						 *
						 * \~english
						 * Place of the value of the property before the substitution of the references
						 *
						 * @note It is kept for the sake of the writing back: to write a substituted
						 *       value would mean to resolve a reference forever, while it is written exactly
						 *       so as to be resolved anew at every reading of the file
						 *
						 * \~
						 */
						span_t raw;
						// Место имени свойства в хранилище знаков
						span_t key;
						// Место значения свойства в хранилище знаков
						span_t value;
						// Вид записи текста настроек
						kind_t kind;
						// Расположение примечания в тексте настроек
						placement_t placement;
						// Порядковый номер раздела, которому запись принадлежит
						uint32_t section;
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						explicit Record() noexcept;
					} record_t;
				private:
					/**
					 * \~russian
					 * Признак того, что дерево несёт обращения к значениям других свойств
					 *
					 * @note Держится ради цены правки: подстановка выполняется заново после
					 *       всякой правки дерева, а обходится она перебором всех записей. Дереву,
					 *       обращений не несущему, - а таких большинство, - платить за это незачем
					 *
					 * \~english
					 * Flag of the tree carrying the references to the values of the other properties
					 *
					 * @note It is kept for the sake of the price of an editing: the substitution is performed anew after
					 *       every editing of the tree, while it costs a traversal of all the records. A tree that
					 *       carries no references — and such ones are the majority — has no reason to pay for this
					 *
					 * \~
					 */
					bool _referenced;
					/**
					 * \~russian
					 * Признак того, что подстановка обращений устарела после правки
					 *
					 * @note Помечен изменяемым наравне с кодом ошибки: пересчёт откладывается
					 *       до первого чтения значения, а чтение объявлено постоянным. Смысла дерева
					 *       пересчёт не меняет - он приводит разрешённые значения в соответствие с
					 *       записанными, - и постоянности чтения тем не нарушает
					 *
					 * \~english
					 * Flag of the substitution of the references having become stale after an editing
					 *
					 * @note Marked as mutable on a par with the error code: the recomputation is postponed
					 *       until the first reading of a value, while the reading is declared a constant one. The recomputation does not change
					 *       the meaning of the tree — it brings the resolved values into a correspondence with
					 *       the written ones — and thereby does not violate the constancy of the reading
					 *
					 * \~
					 */
					mutable bool _stale;
					/**
					 * \~russian
					 * Признак того, что дерево несёт неразрешимое обращение
					 *
					 * @note Взводится снисходительным пересчётом: правка вправе удалить
					 *       источник обращения, и чтение такое значение выдаёт в записанном виде.
					 *       Обратная же запись им отвечает отказом - текст с обращением в пустоту
					 *       разбор отвергает, и выдать его значило бы отдать заведомо негодный файл
					 *
					 * \~english
					 * Flag of the tree carrying an unresolvable reference
					 *
					 * @note Raised by an indulgent recomputation: an editing has the right to remove
					 *       the source of a reference, and the reading issues such a value in the written form.
					 *       The writing back, however, answers it with a refusal — a text with a reference into emptiness
					 *       the parsing rejects, and to issue it would mean to give away a knowingly unfit file
					 *
					 * \~
					 */
					mutable bool _dangling;
				private:
					// Общее хранилище знаков имён и значений
					string _store;
				private:
					/**
					 * \~russian
					 * Объект работы с файловой системой
					 *
					 * \~english
					 * Object of the work with the filesystem
					 *
					 * \~
					 */
					fs_t _fs;
				private:
					/**
					 * \~russian
					 * Кодировка, какою текст настроек прочитан
					 *
					 * \~english
					 * Encoding by which the text of the settings has been read
					 *
					 * \~
					 */
					encoding_t _encoding;
					// Настройки дерева настроек
					settings_t _settings;
				private:
					// Объект потокового чтения записей
					mutable reader_t _reader;
					// Объект записи событий
					mutable writer_t _writer;
				private:
					/**
					 * \~russian
					 * Код ошибки последней операции
					 *
					 * @note Помечен изменяемым: обратная запись дерева в текст его самого не
					 *       меняет и потому объявлена постоянной, но отказ писателя сообщить
					 *       потребителю обязана
					 *
					 * \~english
					 * Error code of the last operation
					 *
					 * @note Marked as mutable: the writing back of the tree into a text does not change the tree itself
					 *       and is therefore declared a constant one, but it is obliged to report a refusal of the writer
					 *       to the consumer
					 *
					 * \~
					 */
					mutable error_t _error;
					// Положение обнаруженной ошибки в исходном тексте
					mutable location_t _errorLocation;
				private:
					/**
					 * \~russian
					 * Последняя запись каждого раздела в перечне записей
					 *
					 * @details Держит для каждого раздела порядковый номер последней его
					 *          записи, объявлена та в тексте или снята пометкой удаления. Место
					 *          вставки нового свойства находится по нему за одно обращение.
					 *
					 * @note Снятые пометкой записи учитываются наравне с прочими намеренно:
					 *       в текст они не идут, и вставка за такой записью даёт тот же текст,
					 *       что и вставка за последней оставшейся
					 *
					 * \~english
					 * Last record of every section in the list of the records
					 *
					 * @details Keeps for every section the ordinal number of its last
					 *          record, whether it is declared in the text or removed by a mark of a deletion. The place
					 *          of the insertion of a new property is found by it in a single call.
					 *
					 * @note The records removed by a mark are taken into account on a par with the rest deliberately:
					 *       they do not go into the text, and an insertion after such a record gives the same text
					 *       as an insertion after the last remaining one
					 *
					 * \~
					 */
					vector <uint32_t> _last;
					// Перечень записей разобранного текста в порядке их следования
					vector <record_t> _records;
					// Перечень разделов разобранного текста в порядке их объявления
					vector <section_t> _sections;
					/**
					 * \~russian
					 * Порядок первых объявлений свойств по каждому разделу
					 *
					 * @details Держит для каждого раздела перечень записей, где его
					 *          свойства объявлены впервые, в порядке их следования в тексте.
					 *
					 * @note Без него выдача перечня имён свойств обращалась в
					 *       квадратичную: перебор всех записей дерева ради каждого раздела на
					 *       сотне тысяч разделов давал двести раз по сотне тысяч проходов.
					 *       Обнаружено стендом сравнения `tools/benchmark/ini`, где обход
					 *       дерева со множеством разделов показал 0.25 МБ/с против 55 МБ/с
					 *       его же сборки
					 *
					 * \~english
					 * Order of the first declarations of the properties for every section
					 *
					 * @details Keeps for every section a list of the records where its
					 *          properties are declared for the first time, in the order of their succession in the text.
					 *
					 * @note Without it the issuance of the list of the names of the properties turned into a
					 *       quadratic one: a traversal of all the records of the tree for the sake of every section on
					 *       a hundred thousand sections gave two hundred times a hundred thousand passes.
					 *       Detected by the `tools/benchmark/ini` benchmark, where a traversal
					 *       of a tree with a multitude of sections showed 0.25 MB/s against 55 MB/s
					 *       of the assembly of the same tree
					 *
					 * \~
					 */
					vector <vector <uint32_t>> _order;
				private:
					// Указатель разделов по имени, приведённому к виду для сличения
					unordered_map <string, uint32_t> _index;
					// Указатель свойств по разделу и имени, приведённым к виду для сличения
					unordered_map <string, vector <uint32_t>> _properties;
				private:
					/**
					 * \~russian
					 * @brief Метод вывода сообщения об отказе в лог
					 *
					 * @details Код отказа остаётся доступен потребителю через error(): журнал
					 *          его не заменяет, а лишь оповещает о случившемся.
					 *
					 * \~english
					 * @brief Method of the output of the message about a refusal into the log
					 *
					 * @details The code of the refusal remains available to the consumer through error():
					 *          the log does not replace it but merely notifies about what has happened.
					 *
					 * \~
					 */
					void report() const noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод запоминания кода отказа работы над деревом
					 *
					 * @details Положение обнаруженной ошибки сбрасывается вместе с кодом намеренно:
					 *          положение это принадлежит РАЗБОРУ текста, и работа над деревом текста под
					 *          собою не имеет вовсе. Переживи оно код, донесение вышло бы стройным, но
					 *          ложным - «раздел не объявлен в строке 1, столбце 5», указующее в текст,
					 *          которого более нет.
					 *
					 * @note Разбор ПОТОЧНЫЙ через это тело не идёт: он ставит положение сам, следом за кодом
					 *
					 * @warning Прежде здесь стояло «разбор через это тело НЕ идёт» без оговорки, и
					 *          это ложно по слову: замерено 09.09.2026 счётом зовущих по объемлющим связкам -
					 *          у INI зов из `parse()` один и у TOML один. Существо тем не рушится, ибо оба
					 *          приходятся на случаи, где положения НЕ СУЩЕСТВУЕТ вовсе: у INI это срыв
					 *          подстановки обращений уже после очистки дерева, у TOML - отказ по длине текста
					 *          прежде начала разбора
					 *
					 * @param error запоминаемый код отказа
					 * @return      признак отказа для выхода из работы
					 *
					 * \~english
					 * @brief Method of the remembering of the error code of an operation over the tree
					 *
					 * @details The location of the discovered error is reset together with the code deliberately:
					 *          that location belongs to the PARSING of a text, while an operation over the tree has no
					 *          text under it at all. Should it survive the code, the report would come out well-formed
					 *          but false - "section not declared at line 1, column 5", pointing into a text which no
					 *          longer exists.
					 *
					 * @note The STREAMING parsing does not pass through this body: it sets the position itself, following the code
					 *
					 * @warning Formerly there stood here "the parsing does NOT pass through this body" without the
					 *          reservation, and that is false to the letter: measured on 2026-09-09 by the count of
					 *          the callers through the enclosing call stacks - INI has one call from `parse()` and
					 *          TOML has one. The essence does not break because of it, however, since both fall on
					 *          the cases where the position DOES NOT EXIST at all: for INI this is the failure of the
					 *          substitution of the references already after the clearing of the tree, for TOML - a
					 *          refusal by the length of the text before the parsing has begun
					 *
					 * @param error error code being remembered
					 * @return      sign of the refusal for the exit from the operation
					 *
					 * \~
					 */
					bool fault(const error_t error) const noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод получения содержимого отрезка хранилища знаков
					 *
					 * @param span отрезок общего хранилища знаков
					 * @return     содержимое отрезка хранилища знаков
					 *
					 * \~english
					 * @brief Method of getting the content of a segment of the storage of the characters
					 *
					 * @param span segment of the common storage of the characters
					 * @return     content of the segment of the storage of the characters
					 *
					 * \~
					 */
					string_view get(const span_t & span) const noexcept;
					/**
					 * \~russian
					 * @brief Метод добавления содержимого к хранилищу знаков
					 *
					 * @param text добавляемое к хранилищу содержимое
					 * @return     отрезок хранилища с добавленным содержимым
					 *
					 * \~english
					 * @brief Method of adding a content to the storage of the characters
					 *
					 * @param text content being added to the storage
					 * @return     segment of the storage with the added content
					 *
					 * \~
					 */
					span_t add(const string_view text) noexcept;
					/**
					 * \~russian
					 * @brief Метод приведения имени к виду для сличения
					 *
					 * @param name приводимое имя раздела или свойства
					 * @return     имя, приведённое к виду для сличения
					 *
					 * \~english
					 * @brief Method of bringing a name to the form for the comparison
					 *
					 * @param name name of the section or of the property being brought
					 * @return     name brought to the form for the comparison
					 *
					 * \~
					 */
					string fold(const string_view name, const bool section = false) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения вида последней записи, идущей в выдачу
					 *
					 * @return вид последней записи, идущей в выдачу
					 *
					 * \~english
					 * @brief Method of getting the kind of the last record going into the output
					 *
					 * @return kind of the last record going into the output
					 *
					 * \~
					 */
					kind_t kind() const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки имени раздела или свойства
					 *
					 * @details Имя проверяется теми же правилами, какими его проверяет
					 *          разбор: имя, записать которое обратно нельзя, отвергается в месте
					 *          правки, а не при записи собранного дерева.
					 *
					 * @param name    проверяемое имя раздела или свойства
					 * @param section признак проверки имени раздела
					 * @param primary признак проверки имени самого раздела, а не подраздела
					 * @return        результат выполнения операции
					 *
					 * \~english
					 * @brief Method of checking the name of a section or of a property
					 *
					 * @details The name is checked by the same rules by which the parsing checks it:
					 *          a name which cannot be written back is rejected at the place of the
					 *          editing rather than at the writing of the assembled tree.
					 *
					 * @param name    name of the section or of the property being checked
					 * @param section flag of the checking of the name of a section
					 * @param primary flag of the checking of the name of the section itself rather than of a subsection
					 * @return        result of performing the operation
					 *
					 * \~
					 */
					bool acceptable(const string_view name, const bool section, const bool primary = true) noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки ключа указателя разделов
					 *
					 * @param section    имя раздела
					 * @param subsection имя подраздела
					 * @return           ключ указателя разделов
					 *
					 * \~english
					 * @brief Method of assembling the key of the index of the sections
					 *
					 * @param section    name of the section
					 * @param subsection name of the subsection
					 * @return           key of the index of the sections
					 *
					 * \~
					 */
					string label(const string_view section, const string_view subsection) const noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки ключа указателя свойств
					 *
					 * @param section порядковый номер раздела
					 * @param key     имя свойства
					 * @return        ключ указателя свойств
					 *
					 * \~english
					 * @brief Method of assembling the key of the index of the properties
					 *
					 * @param section ordinal number of the section
					 * @param key     name of the property
					 * @return        key of the index of the properties
					 *
					 * \~
					 */
					string label(const uint32_t section, const string_view key) const noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки ключа указателя разделов в переданный буфер
					 *
					 * @details Ключ собирается в переданный буфер, а не выдаётся новой
					 *          строкой: буфер вызывающий заводит однажды и переиспользует, отчего
					 *          на обращение к дереву не приходится ни одного построения строки.
					 *          Приведение имени к нижнему регистру выполняется по ходу дозаписи, а
					 *          не отдельным проходом.
					 *
					 * @param buffer     буфер, куда собирается ключ
					 * @param section    имя раздела
					 * @param subsection имя подраздела
					 *
					 * \~english
					 * @brief Method of assembling the key of the index of the sections into a passed buffer
					 *
					 * @details The key is assembled into the passed buffer rather than being issued as a new
					 *          string: the buffer is created by the caller once and reused, from which
					 *          not a single construction of a string falls on a call to the tree.
					 *          The bringing of the name to the lower case is performed in the course of the appending rather than
					 *          by a separate pass.
					 *
					 * @param buffer     buffer into which the key is assembled
					 * @param section    name of the section
					 * @param subsection name of the subsection
					 *
					 * \~
					 */
					void labelled(string & buffer, const string_view section, const string_view subsection) const noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки ключа указателя свойств в переданный буфер
					 *
					 * @param buffer  буфер, куда собирается ключ
					 * @param section порядковый номер раздела
					 * @param key     имя свойства
					 *
					 * \~english
					 * @brief Method of assembling the key of the index of the properties into a passed buffer
					 *
					 * @param buffer  buffer into which the key is assembled
					 * @param section ordinal number of the section
					 * @param key     name of the property
					 *
					 * \~
					 */
					void labelled(string & buffer, const uint32_t section, const string_view key) const noexcept;
					/**
					 * \~russian
					 * @brief Метод поиска раздела по имени с переиспользуемым буфером
					 *
					 * @param buffer     буфер сборки ключа указателя разделов
					 * @param section    имя искомого раздела
					 * @param subsection имя искомого подраздела
					 * @param result     порядковый номер найденного раздела
					 * @return           признак того, что раздел найден
					 *
					 * \~english
					 * @brief Method of searching for a section by a name with a reusable buffer
					 *
					 * @param buffer     buffer of the assembly of the key of the index of the sections
					 * @param section    name of the section being sought
					 * @param subsection name of the subsection being sought
					 * @param result     ordinal number of the found section
					 * @return           flag of the section having been found
					 *
					 * \~
					 */
					bool search(string & buffer, const string_view section, const string_view subsection, uint32_t & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод поиска объявлений свойства по имени
					 *
					 * @details Поиск выполняется однажды на все обращения к значению
					 *          свойства: получение значения, проверка наличия и разбор числом
					 *          прежде повторяли одну и ту же работу дважды.
					 *
					 * @param key        имя искомого свойства
					 * @param section    имя раздела
					 * @param subsection имя подраздела
					 * @return           перечень объявлений свойства либо пустой указатель
					 *
					 * \~english
					 * @brief Method of searching for the declarations of a property by a name
					 *
					 * @details The search is performed once for all the calls to the value of a
					 *          property: the getting of a value, the check of the presence and the parsing as a number
					 *          used to repeat one and the same work twice.
					 *
					 * @param key        name of the property being sought
					 * @param section    name of the section
					 * @param subsection name of the subsection
					 * @return           list of the declarations of the property or an empty pointer
					 *
					 * \~
					 */
					const vector <uint32_t> * locate(const string_view key, const string_view section, const string_view subsection) const noexcept;
					/**
					 * \~russian
					 * @brief Метод поиска раздела по имени
					 *
					 * @param section    имя искомого раздела
					 * @param subsection имя искомого подраздела
					 * @param result     порядковый номер найденного раздела
					 * @return           признак того, что раздел найден
					 *
					 * \~english
					 * @brief Method of searching for a section by a name
					 *
					 * @param section    name of the section being sought
					 * @param subsection name of the subsection being sought
					 * @param result     ordinal number of the found section
					 * @return           flag of the section having been found
					 *
					 * \~
					 */
					bool search(const string_view section, const string_view subsection, uint32_t & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод перестроения указателей поиска
					 *
					 * @details Выполняется после всякой правки, меняющей состав записей:
					 *          порядковые номера записей при вставке сдвигаются, и указатели,
					 *          на них ссылающиеся, обесцениваются.
					 *
					 * \~english
					 * @brief Method of rebuilding the indexes of the search
					 *
					 * @details Performed after every editing that changes the composition of the records:
					 *          the ordinal numbers of the records are shifted at an insertion, and the indexes
					 *          referring to them are invalidated.
					 *
					 * \~
					 */
					void reindex() noexcept;
					/**
					 * \~russian
					 * @brief Метод подстановки обращений к значениям других свойств
					 *
					 * @param strict признак прекращения подстановки неразрешимым обращением
					 * @return       результат выполнения операции
					 *
					 * \~english
					 * @brief Method of substituting the references to the values of the other properties
					 *
					 * @param strict flag of the termination of the substitution by an unresolvable reference
					 * @return       result of performing the operation
					 *
					 * \~
					 */
					bool resolve(const bool strict = true) noexcept;
					/**
					 * \~russian
					 * @brief Метод пересчёта подстановки обращений после правки дерева
					 *
					 * @details Правка меняет значения, на которые обращения ссылаются, и
					 *          прежде разрешённые значения от неё устаревают: чтение выдавало бы
					 *          подставленное до правки, расходясь с тем, что дало бы чтение
					 *          записанного дерева обратно. Пересчёт ведётся от значений до
					 *          подстановки и потому повторяем.
					 *
					 * @note Дерево, обращений не несущее, пересчёта не требует вовсе:
					 *       признак наличия обращений взводится подстановкой сама
					 *
					 * @return результат выполнения операции
					 *
					 * \~english
					 * @brief Method of recomputing the substitution of the references after an editing of the tree
					 *
					 * @details An editing changes the values which the references refer to, and
					 *          the previously resolved values become stale from it: the reading would issue
					 *          what has been substituted before the editing, diverging from what the reading of the written tree
					 *          back would give. The recomputation is conducted from the values before
					 *          the substitution and is therefore repeatable.
					 *
					 * @note A tree that carries no references does not require a recomputation at all:
					 *       the flag of the presence of the references is raised by the substitution itself
					 *
					 * @return result of performing the operation
					 *
					 * \~
					 */
					bool substitute() noexcept;
					/**
					 * \~russian
					 * @brief Метод заведения новой записи свойства в разделе
					 *
					 * @details Помощник этот общий у установки значения и у долива к перечню:
					 *          обе они заводят запись свойства одинаково, и расходятся лишь тем,
					 *          правится ли объявление, уже имеющееся в разделе.
					 *
					 * @param index порядковый номер раздела, которому запись принадлежит
					 * @param key   имя заводимого свойства
					 * @param value значение заводимого свойства
					 * @return      результат выполнения операции
					 *
					 * \~english
					 * @brief Method of the establishing of a new record of a property in a section
					 *
					 * @details The helper is common to the setting of a value and to the appending to a list:
					 *          both of them establish a record of a property in the same way and differ only in
					 *          whether a declaration already present in the section is edited.
					 *
					 * @param index ordinal number of the section whereto the record belongs
					 * @param key   name of the property being established
					 * @param value value of the property being established
					 * @return      result of performing the operation
					 *
					 * \~
					 */
					bool attach(const uint32_t index, const string_view key, const string_view value) noexcept;
					/**
					 * \~russian
					 * @brief Метод пересчёта устаревшей подстановки перед чтением значения
					 *
					 * @details Правка помечает подстановку устаревшей, а пересчитывается она
					 *          перед первым чтением: правок подряд бывает много, и пересчёт после
					 *          каждой обращал бы сборку дерева правками в квадратичную.
					 *
					 * \~english
					 * @brief Method of recomputing a stale substitution before the reading of a value
					 *
					 * @details An editing marks the substitution stale, while it is recomputed
					 *          before the first reading: there happen to be many editings in a row, and a recomputation after
					 *          each of them would turn the assembly of a tree by editings into a quadratic one.
					 *
					 * \~
					 */
					void refresh() const noexcept;
					/**
					 * \~russian
					 * @brief Метод разрешения обращений внутри значения свойства
					 *
					 * @note Перечень разрешаемых свойств служит обнаружению круговой
					 *       ссылки: значение, к которому обращение вернулось, в нём уже лежит.
					 *       Глубина же его сверх того ограничена пределом настроек - на случай
					 *       связи хоть и не круговой, но неразумно длинной
					 *
					 * @param value   разрешаемое значение свойства
					 * @param section порядковый номер раздела, которому значение принадлежит
					 * @param stack   перечень свойств, разрешаемых в настоящее время
					 * @param budget  остаток допустимого объёма подстановки в байтах
					 * @param result  значение с разрешёнными обращениями
					 * @return        результат выполнения операции
					 *
					 * \~english
					 * @brief Method of resolving the references inside the value of a property
					 *
					 * @note The list of the properties being resolved serves the detection of a circular
					 *       reference: a value to which a reference has returned already lies in it.
					 *       Its depth beyond that is limited by the limit of the settings — for the case of a
					 *       linkage that is not circular but unreasonably long
					 *
					 * @param value   value of the property being resolved
					 * @param section ordinal number of the section to which the value belongs
					 * @param stack   list of the properties being resolved at the present moment
					 * @param budget  remainder of the admissible volume of the substitution in bytes
					 * @param result  value with the resolved references
					 * @return        result of performing the operation
					 *
					 * \~
					 */
					bool expand(const string_view value, const uint32_t section, vector <string> & stack, uint64_t & budget, string & result) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения текущих настроек дерева
					 *
					 * @return текущие настройки дерева настроек
					 *
					 * \~english
					 * @brief Method of getting the current settings of the tree
					 *
					 * @return current settings of the settings tree
					 *
					 * \~
					 */
					const settings_t & settings() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки настроек дерева
					 *
					 * @param settings настройки дерева настроек
					 *
					 * \~english
					 * @brief Method of setting the settings of the tree
					 *
					 * @param settings settings of the settings tree
					 *
					 * \~
					 */
					void settings(const settings_t & settings) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод разбора текста настроек
					 *
					 * @details Разбирает текст целиком, сохраняя примечания, пустые строки
					 *          и порядок записей. Прежнее содержимое дерева при этом освобождается.
					 *
					 * @note Примечания и пустые строки читаются всегда, как бы ни были
					 *       выставлены соответствующие флаги настроек разбора: дерево держит
					 *       оформление текста, и отказ от них лишил бы его этой возможности
					 *
					 * @param text разбираемый текст настроек
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of parsing a settings text
					 *
					 * @details Parses the text in full, preserving the comments, the empty lines
					 *          and the order of the records. The previous content of the tree is thereby released.
					 *
					 * @note The comments and the empty lines are always read, however the corresponding
					 *       flags of the settings of the parsing are set: the tree keeps
					 *       the formatting of the text, and a refusal of them would deprive it of that possibility
					 *
					 * @param text settings text being parsed
					 * @return     result of performing the operation
					 *
					 * \~
					 */
					bool parse(const string_view text) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора текста настроек с заданными настройками
					 *
					 * @param text     разбираемый текст настроек
					 * @param settings настройки дерева настроек
					 * @return         результат выполнения операции
					 *
					 * \~english
					 * @brief Method of parsing a settings text with the given settings
					 *
					 * @param text     settings text being parsed
					 * @param settings settings of the settings tree
					 * @return         result of performing the operation
					 *
					 * \~
					 */
					bool parse(const string_view text, const settings_t & settings) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения кода ошибки разбора
					 *
					 * @return код ошибки последней операции разбора
					 *
					 * \~english
					 * @brief Method of getting the error code of the parsing
					 *
					 * @return error code of the last operation of the parsing
					 *
					 * \~
					 */
					error_t error() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения места обнаружения ошибки
					 *
					 * @return положение обнаруженной ошибки в исходном тексте
					 *
					 * \~english
					 * @brief Method of getting the place of the detection of an error
					 *
					 * @return position of the detected error in the source text
					 *
					 * \~
					 */
					const location_t & errorLocation() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения перечня объявленных разделов
					 *
					 * @details Разделы выдаются в порядке их объявления в исходном тексте.
					 *
					 * @warning Выданные виды живут до СЛЕДУЮЩЕГО обращения к дереву. Дерево
					 *          выдаёт содержимое видами в своё хранилище знаков, а подстановка
					 *          обращений считается по требованию: обращение к дереву, её
					 *          устаревшей заставшее, пересчитывает подстановку и хранилище
					 *          перемещает. Вид, взятый до пересчёта, повисает. Нужен вид
					 *          дольше - снимайте копию
					 *
					 * @return перечень объявленных разделов текста настроек
					 *
					 * \~english
					 * @brief Method of getting the list of the declared sections
					 *
					 * @details The sections are issued in the order of their declaration in the source text.
					 *
					 * @warning The issued views live until the NEXT call to the tree. The tree
					 *          issues the content by the views into its storage of the characters, while the substitution
					 *          of the references is computed on demand: a call to the tree finding it stale
					 *          recomputes the substitution and relocates the storage. A view taken before the
					 *          recomputation dangles. Should a view be needed longer — take a copy
					 *
					 * @return list of the declared sections of the settings text
					 *
					 * \~
					 */
					vector <name_t> sections() const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки наличия раздела
					 *
					 * @param section    имя искомого раздела
					 * @param subsection имя искомого подраздела
					 * @return           результат проверки
					 *
					 * \~english
					 * @brief Method of checking the presence of a section
					 *
					 * @param section    name of the section being sought
					 * @param subsection name of the subsection being sought
					 * @return           result of the check
					 *
					 * \~
					 */
					bool section(const string_view section, const string_view subsection = "") const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения перечня имён свойств раздела
					 *
					 * @details Имена выдаются в порядке их объявления и повторов не несут:
					 *          свойство, объявленное несколько раз, выдаётся именем однажды.
					 *
					 * @warning Выданные виды живут до СЛЕДУЮЩЕГО обращения к дереву. Дерево
					 *          выдаёт содержимое видами в своё хранилище знаков, а подстановка
					 *          обращений считается по требованию: обращение к дереву, её
					 *          устаревшей заставшее, пересчитывает подстановку и хранилище
					 *          перемещает. Вид, взятый до пересчёта, повисает. Нужен вид
					 *          дольше - снимайте копию
					 *
					 * @param section    имя раздела
					 * @param subsection имя подраздела
					 * @return           перечень имён свойств раздела
					 *
					 * \~english
					 * @brief Method of getting the list of the names of the properties of a section
					 *
					 * @details The names are issued in the order of their declaration and carry no repetitions:
					 *          a property declared several times is issued by a name once.
					 *
					 * @warning The issued views live until the NEXT call to the tree. The tree
					 *          issues the content by the views into its storage of the characters, while the substitution
					 *          of the references is computed on demand: a call to the tree finding it stale
					 *          recomputes the substitution and relocates the storage. A view taken before the
					 *          recomputation dangles. Should a view be needed longer — take a copy
					 *
					 * @param section    name of the section
					 * @param subsection name of the subsection
					 * @return           list of the names of the properties of the section
					 *
					 * \~
					 */
					vector <string_view> keys(const string_view section = "", const string_view subsection = "") const noexcept;
					/**
					 * \~russian
					 * @brief Шаблон метода перечисления звеньев пути к детям значения
					 *
					 * @tparam T тип данных формируемого значения
					 *
					 * \~english
					 * @brief Template for a method that enumerates the paths to a value's children
					 *
					 * @tparam T data type of the generated value
					 *
					 * \~
					 */
					template <class T>
					/**
					 * \~russian
					 * @brief Метод перечисления звеньев пути к детям значения
					 *
					 * @details Выдаются НЕ ИМЕНА детей, а ЗВЕНЬЯ ПУТИ к ним: такие, что
					 *          `at(путь + "/" + звено)` неминуемо ведёт к тому самому ребёнку. У вместилища
					 *          пар звеном служит имя пары, у перечня одноимённых объявлений - порядковый
					 *          номер.
					 *
					 * @details Ход этот вместе с `at()` даёт ПОЛНЫЙ ОБХОД дерева, общий у всех
					 *          кодеков рамки. Обход ЗАМКНУТ - всякое выданное звено годно для `at()`, - и
					 *          замкнутость эта держится на отменяющих записях RFC 6901: имя, косую черту
					 *          либо тильду несущее, выдаётся ЗАПИСАННЫМ (`~1` и `~0`), а не как есть.
					 *
					 * @details Ход поставлен ПЕРЕГРУЗКОЙ рядом с прежним перечнем имён свойств
					 *          раздела, а не подменою его: обещания у них РАЗНЫЕ - прежний берёт имя
					 *          раздела с подразделом и выдаёт имена свойств, ничего не записывая.
					 *
					 * @warning Застава стоит по ТОЧНОМУ виду `string`, а НЕ по преобразуемости,
					 *          как у `set()` того же дерева. Разница намеренная и замерена:
					 *          прежний ход берёт `string_view`, и литерал в него проходит, - будь
					 *          застава по преобразуемости, вызов `keys("srv")`, ныне означающий
					 *          свойства раздела `srv`, МОЛЧА стал бы означать путь и отвечал бы
					 *          звеньями числовыми. Замерено щупом: отвечал `«0» «1»` вместо
					 *          `«host» «port»`, собираясь без единого замечания
					 *
					 * @warning Цена заставы: литерал уходит в ход ПРЕЖНИЙ, и запись
					 *          `keys("/раздел/имя")` означает раздел с таким именем, а не путь, -
					 *          в отличие от YAML и TOML, где литерал означает путь. Путь у этого
					 *          дерева задавать надлежит ТОЛЬКО видом `string`. Закреплено
					 *          проверкою `CodecIniDocument.PathEnumerationTakesOnlyTheStringType`
					 *
					 * @warning Литералом путь задавать НЕ через это имя, а через `children()`:
					 *          оно берёт `string_view`, и вид довода на ход его не влияет вовсе.
					 *          Ходы `keys(string)` и `children()` суть один и тот же ход, но
					 *          второе имя однозначно всегда, и правила «подавать переменной»
					 *          при нём не требуется. То же и у соседей: `holds()` и `prune()`
					 *
					 * @note Перечисление НИКОГДА не отвечает отказом: значение простое, пустое
					 *       вместилище и путь в никуда отвечают пустым перечнем ОДИНАКОВО, а
					 *       разнятся они ходом `at()`
					 *
					 * @param path путь до значения
					 * @return     перечень звеньев пути к детям значения
					 *
					 * \~english
					 * @brief Method of the enumeration of the path links to the children of a value
					 *
					 * @details What is issued are NOT the NAMES of the children, but the PATH LINKS to them:
					 *          such links that `at(path + "/" + link)` inevitably leads to that very child. In the
					 *          container of pairs the link is the name of the pair, while in the list of the
					 *          declarations of the same name it is the ordinal number.
					 *
					 * @details This call together with `at()` gives the FULL TRAVERSAL of the tree, common to all
					 *          the codecs of the framework. The traversal is CLOSED - every issued link is fit for
					 *          `at()`, - and this closure rests on the escaping notations of RFC 6901: a name
					 *          carrying a slash or a tilde is issued WRITTEN (`~1` and `~0`), not as it is.
					 *
					 * @details The call was placed as an OVERLOAD beside the former list of the names of the
					 *          properties of a section, and not as its replacement: their promises are DIFFERENT -
					 *          the former takes the name of a section with a subsection and issues the names of the
					 *          properties, writing nothing.
					 *
					 * @warning The guard stands by the EXACT kind `string`, and not by the convertibility as in
					 *          `set()` of the same tree. The difference is deliberate and measured: the former call
					 *          takes `string_view`, and a literal passes into it - had the guard stood by the
					 *          convertibility, the call `keys("srv")`, now meaning the properties of the section
					 *          `srv`, would SILENTLY have come to mean a path and would have answered with numeric
					 *          links. Measured by a probe: it answered `«0» «1»` instead of `«host» «port»`,
					 *          assembling without a single remark
					 *
					 * @warning The price of the guard: a literal goes to the FORMER call, and the record
					 *          `keys("/section/name")` means a section with such a name, and not a path -
					 *          unlike YAML and TOML, where a literal means a path. The path in this tree is to be
					 *          given ONLY by the kind `string`. Fixed by the check
					 *          `CodecIniDocument.PathEnumerationTakesOnlyTheStringType`
					 *
					 * @warning A path is to be given by a literal NOT through this name, but through `children()`:
					 *          it takes `string_view`, and the kind of the argument does not influence its call at
					 *          all. The calls `keys(string)` and `children()` are one and the same call, but the
					 *          second name is always unambiguous, and no rule "to pass a variable" is required with
					 *          it. The same holds for the neighbors: `holds()` and `prune()`
					 *
					 * @note The enumeration NEVER answers with a refusal: a simple value, an empty container and a
					 *       path to nowhere answer with an empty list IDENTICALLY, while they differ by the call
					 *       `at()`
					 *
					 * @param path path to the value
					 * @return     list of the path links to the children of the value
					 *
					 * \~
					 */
					typename std::enable_if <std::is_same <T, string>::value, vector <string>>::type keys(const T & path) const noexcept {
						// Выводим перечень звеньев пути к детям значения
						return this->children(string_view(path));
					}
					/**
					 * \~russian
					 * @brief Метод перечисления звеньев пути к детям значения
					 *
					 * @details Тело посредника `keys()`, шаблоном от него отделённое.
					 *
					 * @param path путь до значения
					 * @return     перечень звеньев пути к детям значения
					 *
					 * \~english
					 * @brief Method of the enumeration of the path links to the children of a value
					 *
					 * @details The body of the intermediary `keys()`, separated from it by a template.
					 *
					 * @param path path to the value
					 * @return     list of the path links to the children of the value
					 *
					 * \~
					 */
					vector <string> children(const string_view path) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод обращения к значению по пути
					 *
					 * @details Ход этот общий у всех семи кодеков рамки, и спрашивается он у
					 *          ДОКУМЕНТА, а не у значения: общие проверки договора зовут
					 *          `document.at(путь)`, и кодек, такого хода не несущий, в перебор
					 *          по семерым не входит вовсе.
					 *
					 * @note Выдаётся ВЛАДЕЮЩЕЕ значение, а не узел дерева: узлового вида у ini
					 *       нет вовсе - настройки держатся записями, а не ареною узлов. Это
					 *       законное расхождение по устройству, а не отступление от договора
					 *
					 * @param path путь до разыскиваемого значения
					 * @return     разысканное значение настроек
					 *
					 * \~english
					 * @brief Method of accessing a value by a path
					 *
					 * @details This call is common to all the seven codecs of the framework, and it is asked of
					 *          the DOCUMENT, and not of the value: the common checks of the contract call
					 *          `document.at(path)`, and a codec not carrying such a call does not enter the
					 *          enumeration of the seven at all.
					 *
					 * @note What is issued is the OWNING value, and not a node of the tree: ini has no node kind
					 *       at all - the settings are held by the records, and not by an arena of nodes. This is a
					 *       lawful divergence by the device, and not a departure from the contract
					 *
					 * @param path path to the value being searched for
					 * @return     found value of the settings
					 *
					 * \~
					 */
					codec::ini::Value at(const string & path) const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки наличия свойства
					 *
					 * @param key        имя искомого свойства
					 * @param section    имя раздела
					 * @param subsection имя подраздела
					 * @return           результат проверки
					 *
					 * \~english
					 * @brief Method of checking the presence of a property
					 *
					 * @param key        name of the property being sought
					 * @param section    name of the section
					 * @param subsection name of the subsection
					 * @return           result of the check
					 *
					 * \~
					 */
					bool has(const string_view key, const string_view section = "", const string_view subsection = "") const noexcept;
					/**
					 * \~russian
					 * @brief Шаблон метода проверки наличия значения по пути
					 *
					 * @tparam T тип данных формируемого значения
					 *
					 * \~english
					 * @brief Method template for checking the existence of a value at a given path
					 *
					 * @tparam T data type of the generated value
					 *
					 * \~
					 */
					template <class T>
					/**
					 * \~russian
					 * @brief Метод проверки наличия значения по пути
					 *
					 * @details Ход этот общий у всех семи кодеков рамки: спрос по пути, строкою
					 *          заданному. Спрашивается значение ЛЮБОГО вида - и свойство, и раздел, и
					 *          значение перечня, - тогда как прежний ход берёт имя свойства с разделом.
					 *
					 * @details Вместе с `keys()` и `at()` ход этот и составляет общий обход
					 *          дерева: всякое звено, перечнем выданное, спрос проходит.
					 *
					 * @warning Застава стоит по ТОЧНОМУ виду `string` по той же причине, по какой
					 *          она стоит у общего перечня звеньев: прежний ход берёт `string_view`,
					 *          и литерал в него проходит, - будь застава по преобразуемости, вызов
					 *          `has("port")`, свойство означающий, МОЛЧА стал бы означать путь.
					 *          Цена та же: путь задавать надлежит ТОЛЬКО видом `string`
					 *
					 * @param path путь до искомого значения
					 * @return     признак наличия значения по пути
					 *
					 * \~english
					 * @brief Method of checking the presence of a value by a path
					 *
					 * @details This call is common to all the seven codecs of the framework: a query by a path
					 *          given by a string. A value of ANY kind is queried - both a property, and a section,
					 *          and a value of a list - while the former call takes the name of a property with a
					 *          section.
					 *
					 * @details Together with `keys()` and `at()` this call is precisely what constitutes the
					 *          common traversal of the tree: every link issued by the enumeration passes the
					 *          query.
					 *
					 * @warning The guard stands by the EXACT kind `string` for the same reason for which it
					 *          stands at the common enumeration of the links: the former call takes `string_view`,
					 *          and a literal passes into it - had the guard stood by the convertibility, the call
					 *          `has("port")`, meaning a property, would SILENTLY have come to mean a path.
					 *          The price is the same: a path is to be given ONLY by the kind `string`
					 *
					 * @param path path to the value being sought
					 * @return     sign of the presence of the value by the path
					 *
					 * \~
					 */
					typename std::enable_if <std::is_same <T, string>::value, bool>::type has(const T & path) const noexcept {
						// Выводим признак наличия значения по пути
						return this->holds(string_view(path));
					}
					/**
					 * \~russian
					 * @brief Метод проверки наличия значения по пути
					 *
					 * @details Тело посредника `has()`, шаблоном от него отделённое.
					 *
					 * @param path путь до искомого значения
					 * @return     признак наличия значения по пути
					 *
					 * \~english
					 * @brief Method of checking the presence of a value by a path
					 *
					 * @details The body of the intermediary `has()`, separated from it by a template.
					 *
					 * @param path path to the value being sought
					 * @return     sign of the presence of the value by the path
					 *
					 * \~
					 */
					bool holds(const string_view path) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения значения свойства
					 *
					 * @details При нескольких объявлениях свойства выдаётся то из них,
					 *          которое велит выдать настройка обращения с повторами: первое, либо
					 *          последнее, либо - при собирании их в перечень - первое из перечня.
					 *
					 * @warning Выданные виды живут до СЛЕДУЮЩЕГО обращения к дереву. Дерево
					 *          выдаёт содержимое видами в своё хранилище знаков, а подстановка
					 *          обращений считается по требованию: обращение к дереву, её
					 *          устаревшей заставшее, пересчитывает подстановку и хранилище
					 *          перемещает. Вид, взятый до пересчёта, повисает. Нужен вид
					 *          дольше - снимайте копию
					 *
					 * @param key        имя искомого свойства
					 * @param section    имя раздела
					 * @param subsection имя подраздела
					 * @return           значение найденного свойства либо пустая последовательность
					 *
					 * \~english
					 * @brief Method of getting the value of a property
					 *
					 * @details At several declarations of a property the one of them is issued
					 *          which the setting of the treatment of the repetitions orders to issue: the first, or
					 *          the last, or — at their assembling into a list — the first of the list.
					 *
					 * @warning The issued views live until the NEXT call to the tree. The tree
					 *          issues the content by the views into its storage of the characters, while the substitution
					 *          of the references is computed on demand: a call to the tree finding it stale
					 *          recomputes the substitution and relocates the storage. A view taken before the
					 *          recomputation dangles. Should a view be needed longer — take a copy
					 *
					 * @param key        name of the property being sought
					 * @param section    name of the section
					 * @param subsection name of the subsection
					 * @return           value of the found property or an empty sequence
					 *
					 * \~
					 */
					string_view get(const string_view key, const string_view section = "", const string_view subsection = "") const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения перечня значений свойства
					 *
					 * @details Выдаются все объявления свойства в порядке их следования.
					 *          Наречия Git и systemd повтором свойства задают перечень значений, и
					 *          читать его следует именно так.
					 *
					 * @warning Выданные виды живут до СЛЕДУЮЩЕГО обращения к дереву. Дерево
					 *          выдаёт содержимое видами в своё хранилище знаков, а подстановка
					 *          обращений считается по требованию: обращение к дереву, её
					 *          устаревшей заставшее, пересчитывает подстановку и хранилище
					 *          перемещает. Вид, взятый до пересчёта, повисает. Нужен вид
					 *          дольше - снимайте копию
					 *
					 * @param key        имя искомого свойства
					 * @param section    имя раздела
					 * @param subsection имя подраздела
					 * @return           перечень значений найденного свойства
					 *
					 * \~english
					 * @brief Method of getting the list of the values of a property
					 *
					 * @details All the declarations of the property are issued in the order of their succession.
					 *          The Git and systemd dialects give a list of the values by a repetition of a property, and
					 *          it should be read exactly that way.
					 *
					 * @warning The issued views live until the NEXT call to the tree. The tree
					 *          issues the content by the views into its storage of the characters, while the substitution
					 *          of the references is computed on demand: a call to the tree finding it stale
					 *          recomputes the substitution and relocates the storage. A view taken before the
					 *          recomputation dangles. Should a view be needed longer — take a copy
					 *
					 * @param key        name of the property being sought
					 * @param section    name of the section
					 * @param subsection name of the subsection
					 * @return           list of the values of the found property
					 *
					 * \~
					 */
					vector <string_view> values(const string_view key, const string_view section = "", const string_view subsection = "") const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения перечня признаков ограды значений свойства
					 *
					 * @details Ограда, человеком поставленная, хранится деревом ради обратной
					 *          записи наравне со знаком примечания. Выдаётся она перечнем ровно
					 *          той же длины и того же порядка, что и перечень значений: свойство,
					 *          объявленное несколько раз, несёт свою ограду у всякого объявления.
					 *
					 * @param key        имя искомого свойства
					 * @param section    имя раздела
					 * @param subsection имя подраздела
					 * @return           перечень признаков ограды значений свойства
					 *
					 * \~english
					 * @brief Method of getting the list of the flags of the quoting of the values of a property
					 *
					 * @details The quoting put by a human is kept by the tree for the sake of the writing back
					 *          on a par with the comment character. It is issued by a list of exactly the same length and
					 *          the same order as the list of the values: a property declared several times carries its
					 *          own quoting at every declaration.
					 *
					 * @param key        name of the property being sought
					 * @param section    name of the section
					 * @param subsection name of the subsection
					 * @return           list of the flags of the quoting of the values of a property
					 *
					 * \~
					 */
					vector <bool> quotes(const string_view key, const string_view section = "", const string_view subsection = "") const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения перечня значений свойства до подстановки обращений
					 *
					 * @details Значение до подстановки держится деревом ради обратной записи:
					 *          записать подставленное значило бы разрешить обращение навсегда, а
					 *          оно на то и записано, чтобы разрешаться заново при каждом чтении
					 *          файла. Выдаётся оно перечнем ровно той же длины и того же порядка,
					 *          что и перечень значений.
					 *
					 * @param key        имя искомого свойства
					 * @param section    имя раздела
					 * @param subsection имя подраздела
					 * @return           перечень значений свойства до подстановки обращений
					 *
					 * \~english
					 * @brief Method of getting the list of the values of a property before the substitution of the references
					 *
					 * @details The value before the substitution is kept by the tree for the sake of the writing back:
					 *          to write a substituted one would mean to resolve a reference forever, while it is written exactly
					 *          so as to be resolved anew at every reading of the file. It is issued by a list of exactly the same
					 *          length and the same order as the list of the values.
					 *
					 * @param key        name of the property being sought
					 * @param section    name of the section
					 * @param subsection name of the subsection
					 * @return           list of the values of a property before the substitution of the references
					 *
					 * \~
					 */
					vector <string_view> sources(const string_view key, const string_view section = "", const string_view subsection = "") const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Шаблон метода получения значения свойства числом
					 *
					 * @tparam T тип числа результата разбора
					 *
					 * \~english
					 * @brief Template of a method for obtaining a property value by number
					 *
					 * @tparam T number type of the parsing result
					 *
					 * \~
					 */
					template <typename T>
					/**
					 * \~russian
					 * @brief Метод получения значения свойства числом
					 *
					 * @param result     ссылка на результат разбора
					 * @param key        имя искомого свойства
					 * @param section    имя раздела
					 * @param subsection имя подраздела
					 * @return           признак успешного разбора
					 *
					 * \~english
					 * @brief Method of getting the value of a property as a number
					 *
					 * @param result     reference to the result of the parsing
					 * @param key        name of the property being sought
					 * @param section    name of the section
					 * @param subsection name of the subsection
					 * @return           flag of a successful parsing
					 *
					 * \~
					 */
					bool value(T & result, const string_view key, const string_view section = "", const string_view subsection = "") const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод объявления раздела
					 *
					 * @details Уже объявленный раздел повторно не объявляется: метод
					 *          отвечает успехом, оставляя дерево нетронутым.
					 *
					 * @param section    имя объявляемого раздела
					 * @param subsection имя объявляемого подраздела
					 * @return           результат выполнения операции
					 *
					 * \~english
					 * @brief Method of declaring a section
					 *
					 * @details An already declared section is not declared again: the method
					 *          answers with success, leaving the tree untouched.
					 *
					 * @param section    name of the section being declared
					 * @param subsection name of the subsection being declared
					 * @return           result of performing the operation
					 *
					 * \~
					 */
					bool create(const string_view section, const string_view subsection = "") noexcept;
					/**
					 * \~russian
					 * @brief Метод установки значения свойства
					 *
					 * @details Значение уже объявленного свойства заменяется на месте, и
					 *          ни порядок записей, ни примечания при этом не страдают. Отсутствующее
					 *          свойство дописывается в конец своего раздела, а отсутствующий раздел
					 *          объявляется.
					 *
					 * @note При нескольких объявлениях свойства заменяется значение того из
					 *       них, которое выдал бы @c get(), а прочие остаются нетронутыми: замена
					 *       их всех обеднила бы перечень значений, ради которого повтор и записан
					 *
					 * @param key        имя устанавливаемого свойства
					 * @param value      устанавливаемое значение свойства
					 * @param section    имя раздела
					 * @param subsection имя подраздела
					 * @return           результат выполнения операции
					 *
					 * \~english
					 * @brief Method of setting the value of a property
					 *
					 * @details The value of an already declared property is replaced in place, and
					 *          neither the order of the records nor the comments suffer thereby. An absent
					 *          property is appended to the end of its section, while an absent section
					 *          is declared.
					 *
					 * @note At several declarations of a property the value of the one of
					 *       them is replaced which @c get() would issue, while the rest remain untouched: a replacement
					 *       of all of them would impoverish the list of the values for the sake of which the repetition has been written
					 *
					 * @param key        name of the property being set
					 * @param value      value of the property being set
					 * @param section    name of the section
					 * @param subsection name of the subsection
					 * @return           result of performing the operation
					 *
					 * \~
					 */
					bool set(const string_view key, const string_view value, const string_view section = "", const string_view subsection = "") noexcept;
					/**
					 * \~russian
					 * @brief Шаблон метода постановки владеющего значения по пути
					 *
					 * @tparam V тип числа результата разбора
					 *
					 * \~english
					 * @brief Pattern of a method for setting the owning value along a path
					 *
					 * @tparam V number type of the parsing result
					 *
					 * \~
					 */
					template <class V>
					/**
					 * \~russian
					 * @brief Метод постановки владеющего значения по пути
					 *
					 * @details Ход этот заведён ради общего вида кодеков рамки: правка дерева
					 *          зовётся у одних ходом значения, у других ходом документа, и
					 *          потребитель, два кодека рядом читающий, разыскивал ход не там,
					 *          где тот лежал. Путь берётся строкою по описанию RFC 6901.
					 *
					 * @note Постановка ведётся ЧЕРЕЗ КОПИЮ дерева: настройки снимаются
					 *       владеющим значением, правятся по пути и переносятся обратно.
					 *       Правка не есть быстрый путь, а целость её важнее цены копии -
					 *       отказ посреди переноса оставил бы дерево с частью правки
					 *
					 * @param path  путь постановки значения
					 * @param value ставимое владеющее значение
					 * @return      признак успешной постановки значения
					 *
					 * \~english
					 * @brief Method of setting an owning value by a path
					 *
					 * @details This call has been instituted for the sake of the common look of the codecs of
					 *          the framework: the editing of a tree is called by some the call of a value, by
					 *          others the call of a document, and a consumer reading two codecs side by side
					 *          sought the call not where it lay. The path is taken by a string following the
					 *          description of RFC 6901.
					 *
					 * @note The setting is conducted THROUGH A COPY of the tree: the settings are taken off by
					 *       the owning value, edited by a path, and moved back. The edit is not a fast path, and
					 *       its integrity weighs more than the price of the copy - a failure in the middle of a
					 *       move would have left the tree with a part of the edit
					 *
					 * @param path  path of the setting of the value
					 * @param value owning value being set
					 * @return      sign of the successful setting of the value
					 *
					 * \~
					 */
					typename std::enable_if <std::is_same <typename std::decay <V>::type, codec::ini::Value>::value, bool>::type set(const string & path, const V & value) noexcept {
						// Выводим результат переноса владеющего значения в дерево настроек
						return this->graft(path, value);
					}
					/**
					 * \~russian
					 * @brief Метод постановки владеющего значения по пути
					 *
					 * @details Тело посредника `set()`, шаблоном от него отделённое. Отделение
					 *          обязательно: постановка по ключу принимает довод строкою, и
					 *          запись `set("x", "1")` подошла бы РАЗОМ к ней и к постановке
					 *          владеющего значения - собиратель отвечал бы отказом по
					 *          двусмысленности. Шаблон, точным видом значения огороженный,
					 *          строковых доводов не принимает вовсе, и двусмысленность тем
					 *          снимается, ничего у потребителя не ломая.
					 *
					 * @param path  путь постановки значения
					 * @param value ставимое владеющее значение
					 * @return      признак успешной постановки значения
					 *
					 * \~english
					 * @brief Method of setting an owning value by a path
					 *
					 * @details The body of the intermediary `set()`, separated from it by a template. The
					 *          separation is obligatory: the setting by a key takes an argument by a string, and
					 *          the record `set("x", "1")` would have fit AT ONCE both it and the setting of an
					 *          owning value - the compilation would have answered with a refusal for the
					 *          ambiguity. A template fenced by the exact kind of the value does not take string
					 *          arguments at all, and the ambiguity is thereby removed without breaking anything
					 *          at the consumer.
					 *
					 * @param path  path of the setting of the value
					 * @param value owning value being set
					 * @return      sign of the successful setting of the value
					 *
					 * \~
					 */
					bool graft(const string & path, const codec::ini::Value & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки пустого значения по пути к нему
					 *
					 * @details Ход этот общий у кодеков рамки. Пустое значение от снятия свойства
					 *          отлично: свойство остаётся налицо, а значения при нём нет.
					 *
					 * @note Пустое значение у настроек законно: запись «ключ =» разбирается
					 *       свойством с пустым значением и переписывается обратно ею же. У
					 *       кодека TOML хода этого нет вовсе, и расхождение то законное -
					 *       запись «a = » отвергается там самим стандартом, а пустая строка
					 *       есть значение иное, нежели его отсутствие
					 *
					 * @param path путь к устанавливаемому свойству
					 * @return     признак успешной установки значения
					 *
					 * \~english
					 * @brief Method of the setting of an empty value by the path to it
					 *
					 * @details This call is common to the codecs of the framework. The empty value differs from
					 *          the removal of a property: the property remains present, while there is no value at
					 *          it.
					 *
					 * @note An empty value is lawful for the settings: the record «key =» is parsed as a
					 *       property with an empty value and is rewritten back as the same. The codec TOML does
					 *       not have this call at all, and that divergence is lawful - the record «a = » is
					 *       rejected there by the standard itself, and an empty line is a value other than its
					 *       absence
					 *
					 * @param path path to the property being set
					 * @return     sign of the successful setting of the value
					 *
					 * \~
					 */
					bool reset(const string & path) noexcept;
					/**
					 * \~russian
					 * @brief Метод долива значения к перечню значений свойства
					 *
					 * @details Установка значения правит объявление, уже имеющееся в
					 *          разделе, и перечня тем самым не строит. Долив же заводит новое
					 *          объявление того же имени всегда: перечень значений записи INI и есть
					 *          последовательность одноимённых свойств, и выдаёт её @c values().
					 *
					 * @note Признак записи «имя[] = значение» доливом не взводится: скобки
					 *       суть послабление разбора языка PHP, и ставить их дереву, собранному
					 *       вызовами, значило бы писать наречие там, где его не просили
					 *
					 * @param key        имя свойства, к перечню значений которого ведётся долив
					 * @param value      доливаемое значение свойства
					 * @param section    имя раздела
					 * @param subsection имя подраздела
					 * @return           результат выполнения операции
					 *
					 * \~english
					 * @brief Method of the appending of a value to the list of the values of a property
					 *
					 * @details The setting of a value edits a declaration already present in the
					 *          section and thereby builds no list. The appending, however, always establishes a new
					 *          declaration of the same name: the list of the values of the INI notation is
					 *          a sequence of the properties of the same name, and @c values() issues it.
					 *
					 * @note The flag of the record «name[] = value» is not raised by the appending: the brackets
					 *       are an indulgence of the parsing of the PHP language, and to put them into a tree assembled
					 *       by the calls would mean to write a dialect where it has not been asked for
					 *
					 * @param key        name of the property to whose list of the values the appending is conducted
					 * @param value      value being appended
					 * @param section    name of the section
					 * @param subsection name of the subsection
					 * @return           result of performing the operation
					 *
					 * \~
					 */
					bool push(const string_view key, const string_view value, const string_view section = "", const string_view subsection = "") noexcept;
					/**
					 * \~russian
					 * @brief Метод удаления свойства
					 *
					 * @details Удаляются все объявления свойства вместе с примечаниями,
					 *          записанными в конце их строк.
					 *
					 * @param key        имя удаляемого свойства
					 * @param section    имя раздела
					 * @param subsection имя подраздела
					 * @return           результат выполнения операции
					 *
					 * \~english
					 * @brief Method of removing a property
					 *
					 * @details All the declarations of the property are removed together with the comments
					 *          written at the end of their lines.
					 *
					 * @param key        name of the property being removed
					 * @param section    name of the section
					 * @param subsection name of the subsection
					 * @return           result of performing the operation
					 *
					 * \~
					 */
					bool erase(const string_view key, const string_view section = "", const string_view subsection = "") noexcept;
					/**
					 * \~russian
					 * @brief Шаблон метода снятия значения по пути
					 *
					 * @tparam T тип числа результата разбора
					 *
					 * \~english
					 * @brief Pattern of a method for removing a value along a path
					 *
					 * @tparam T number type of the parsing result
					 *
					 * \~
					 */
					template <class T>
					/**
					 * \~russian
					 * @brief Метод снятия значения по пути
					 *
					 * @details Ход этот общий у всех семи кодеков рамки: снос по пути, строкою
					 *          заданному. Сносится значение ЛЮБОГО вида - и свойство, и раздел, и значение
					 *          перечня одноимённых объявлений, - тогда как прежний ход снимает свойство в
					 *          разделе, а `remove(раздел)` сносит раздел.
					 *
					 * @details Пустой путь по описанию RFC 6901 означает ВЕСЬ документ, а не
					 *          отсутствующее свойство: снос его опустошает дерево целиком. Дозволен он
					 *          оттого, что пустой файл настроек INI законен вполне.
					 *
					 * @warning Застава стоит по ТОЧНОМУ виду `string` по той же причине, по какой
					 *          она стоит у общего перечня звеньев и общего спроса: прежний ход
					 *          берёт `string_view`, и литерал в него проходит, - будь застава по
					 *          преобразуемости, вызов `erase("port")`, свойство означающий, МОЛЧА
					 *          стал бы означать путь
					 *
					 * @note Числовое звено принимается наравне с именем: повтор имени свойства
					 *       ложится в дерево перечнем, и перечень звеньев выдаёт там номера.
					 *       Не прими снос номера - обход был бы замкнут для чтения и разомкнут
					 *       для правки
					 *
					 * @param path путь до снимаемого значения
					 * @return     признак успешного снятия значения
					 *
					 * \~english
					 * @brief Method of removing a value by a path
					 *
					 * @details This call is common to all the seven codecs of the framework: a removal by a
					 *          path given by a string. A value of ANY kind is removed - both a property, and a
					 *          section, and a value of a list of the declarations of the same name - while the
					 *          former call removes a property in a section, and `remove(section)` removes a
					 *          section.
					 *
					 * @details An empty path by the description of RFC 6901 means the WHOLE document, and not an
					 *          absent property: its removal empties the tree entirely. It is permitted because an
					 *          empty INI settings file is fully lawful.
					 *
					 * @warning The guard stands by the EXACT kind `string` for the same reason for which it
					 *          stands at the common enumeration of the links and at the common query: the former
					 *          call takes `string_view`, and a literal passes into it - had the guard stood by the
					 *          convertibility, the call `erase("port")`, meaning a property, would SILENTLY
					 *          have come to mean a path
					 *
					 * @note A numeric link is accepted on a par with a name: a repetition of a property name
					 *       lies in the tree as a list, and the enumeration of the links issues numbers there.
					 *       Had the removal not accepted numbers - the traversal would have been closed for the
					 *       reading and open for the editing
					 *
					 * @param path path to the value being removed
					 * @return     sign of the successful removal of the value
					 *
					 * \~
					 */
					typename std::enable_if <std::is_same <T, string>::value, bool>::type erase(const T & path) noexcept {
						// Выводим результат снятия значения по пути
						return this->prune(path);
					}
					/**
					 * \~russian
					 * @brief Метод снятия значения по пути
					 *
					 * @details Тело посредника `erase()`, шаблоном от него отделённое.
					 *
					 * @param path путь до снимаемого значения
					 * @return     признак успешного снятия значения
					 *
					 * \~english
					 * @brief Method of removing a value by a path
					 *
					 * @details The body of the intermediary `erase()`, separated from it by a template.
					 *
					 * @param path path to the value being removed
					 * @return     sign of the successful removal of the value
					 *
					 * \~
					 */
					bool prune(const string & path) noexcept;
					/**
					 * \~russian
					 * @brief Метод удаления раздела
					 *
					 * @details Удаляется объявление раздела вместе со всеми его записями.
					 *
					 * @param section    имя удаляемого раздела
					 * @param subsection имя удаляемого подраздела
					 * @return           результат выполнения операции
					 *
					 * \~english
					 * @brief Method of removing a section
					 *
					 * @details The declaration of the section is removed together with all its records.
					 *
					 * @param section    name of the section being removed
					 * @param subsection name of the subsection being removed
					 * @return           result of performing the operation
					 *
					 * \~
					 */
					bool remove(const string_view section, const string_view subsection = "") noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения количества объявленных разделов
					 *
					 * @return количество объявленных разделов текста настроек
					 *
					 * \~english
					 * @brief Method of getting the number of the declared sections
					 *
					 * @return number of the declared sections of the settings text
					 *
					 * \~
					 */
					size_t size() const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки дерева на отсутствие записей
					 *
					 * @return результат проверки
					 *
					 * \~english
					 * @brief Method of checking the tree for the absence of the records
					 *
					 * @return result of the check
					 *
					 * \~
					 */
					bool empty() const noexcept;
					/**
					 * \~russian
					 * @brief Метод освобождения дерева настроек
					 *
					 * \~english
					 * @brief Method of releasing the settings tree
					 *
					 * \~
					 */
					void clear() noexcept;
					/**
					 * \~russian
					 * @brief Псевдоним сброса состояния документа (см. clear())
					 *
					 * \~english
					 * @brief Alias of the reset of the state of a document (see clear())
					 *
					 * \~
					 */
					void reset() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод чтения настроек из файла
					 *
					 * @details Ход этот общий у всех семи кодеков рамки.
					 *
					 * @note Ненайденный файл пустым текстом НЕ является: отказ называет причину
					 *       кодом `FILE_NOT_OPENED`, тогда как пустота текста сообщила бы
					 *       потребителю, будто файл прочтён и пуст
					 *
					 * @param filename путь до читаемого файла настроек
					 * @return         признак успешного чтения настроек
					 *
					 * \~english
					 * @brief Method of reading the settings from a file
					 *
					 * @details This call is common to all the seven codecs of the framework.
					 *
					 * @note A file not found is NOT an empty text: the refusal names the reason by the code
					 *       `FILE_NOT_OPENED`, while the emptiness of the text would have reported to the
					 *       consumer as though the file had been read and were empty
					 *
					 * @param filename path to the file of the settings being read
					 * @return         sign of the successful reading of the settings
					 *
					 * \~
					 */
					bool load(const string & filename) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи настроек в файл
					 *
					 * @details Ход этот общий у всех семи кодеков рамки.
					 *
					 * @param filename путь до записываемого файла настроек
					 * @return         признак успешной записи настроек
					 *
					 * \~english
					 * @brief Method of writing the settings into a file
					 *
					 * @details This call is common to all the seven codecs of the framework.
					 *
					 * @param filename path to the file of the settings being written
					 * @return         sign of the successful writing of the settings
					 *
					 * \~
					 */
					bool save(const string & filename) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения кодировки прочитанного текста
					 *
					 * @details Ход этот общий у кодеков рамки, текст читающих. Кодировка
					 *          определяется чтением по метке порядка октетов либо по самому
					 *          виду текста, а дерево лишь несёт её потребителю.
					 *
					 * @return кодировка, какою текст настроек прочитан
					 *
					 * \~english
					 * @brief Method of getting the encoding of the read text
					 *
					 * @details This call is common to the codecs of the framework which read a text. The
					 *          encoding is determined by the reading by the octet order mark or by the very look
					 *          of the text, while the tree only carries it to the consumer.
					 *
					 * @return encoding by which the text of the settings was read
					 *
					 * \~
					 */
					encoding_t encoding() const noexcept;
					/**
					 * \~russian
					 * @brief Метод записи дерева обратно в текст настроек
					 *
					 * @details Настройки записи выводятся из настроек разбора: наречие, каким
					 *          текст прочитан, тем же и записывается. Задать иное наречие можно,
					 *          передав настройки записи явно.
					 *
					 * @note Значение, записать которое НАРЕЧИЕМ НЕЛЬЗЯ, отвергает сборку
					 *       ЦЕЛИКОМ: выдача пуста, а причина названа кодом `INVALID_CHARACTER`.
					 *       Частичной выдачи тут быть не может - текст настроек либо читается
					 *       обратно без потерь, либо не годен вовсе
					 *
					 * @note Невыразимость эта зависит от НАРЕЧИЯ, а не от значения самого:
					 *       при `escapes` перевод строки, возврат каретки и нуль записываются
					 *       управляющими последовательностями и выдаются успешно, при выключенном
					 *       - отвергаются. Замерено щупом 15.09.2026 на всех четырёх знаках
					 *
					 * @warning Отсюда следует, что правка значения НЕ поверяется наречием, в
					 *          отличие от правки имени: `acceptable()` отвергает невыразимое имя в
					 *          месте правки, а значение проверяется лишь при сборке. Расхождение это
					 *          НАМЕРЕННО - наречие вольно смениться между правкой и сборкой, и
					 *          значение, ныне невыразимое, станет выразимым от одной настройки, тогда
					 *          как имя поверяется правилами РАЗБОРА, от наречия записи не зависящими.
					 *          Донесение об этом приходило трижды от трёх разных читателей кода, оттого
					 *          решение и записано здесь: прежде оно держалось на одном лишь коде и на
					 *          проверке `CodecIniDocument.WriteFailure`, имя которой предмета не
					 *          называет, - оттого трое подряд её и не нашли
					 *
					 * @return собранный текст настроек
					 *
					 * \~english
					 * @brief Method of writing the tree back into a settings text
					 *
					 * @details The settings of the writing are inferred from the settings of the parsing: the dialect by which
					 *          the text has been read is the one it is written by as well. Another dialect can be given
					 *          by passing the settings of the writing explicitly.
					 *
					 * @note A value which CANNOT be written by the DIALECT rejects the assembly AS A WHOLE: the
					 *       issue is empty, and the reason is named by the code `INVALID_CHARACTER`. A partial
					 *       issue here cannot be - a settings text is either read back without losses or is unfit
					 *       altogether
					 *
					 * @note This unexpressibility depends on the DIALECT, and not on the value itself: with
					 *       `escapes` a line ending, a carriage return and a zero are written by the control
					 *       sequences and are issued successfully, while with it off - they are rejected.
					 *       Measured by a probe on 2026-09-15 on all the four characters
					 *
					 * @warning It follows from this that the editing of a value is NOT checked by the dialect, in
					 *          contrast to the editing of a name: `acceptable()` rejects an inexpressible name at
					 *          the place of the editing, while a value is checked only at the assembly. This
					 *          divergence is DELIBERATE - a dialect is free to change between the editing and the
					 *          assembly, and a value now inexpressible will become expressible from a single
					 *          setting, whereas a name is checked by the rules of the PARSING, which do not depend
					 *          on the dialect of the writing. The report about this came three times from three
					 *          different readers of the code, and because of that the decision is written down
					 *          here: formerly it rested on the code alone and on the check
					 *          `CodecIniDocument.WriteFailure`, whose name does not call the subject, - because of
					 *          that the three in a row did not find it
					 *
					 * @return assembled settings text
					 *
					 * \~
					 */
					string dump() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения настроек записи, отвечающих настройкам разбора
					 *
					 * @details Выводит наречие записи из наречия разбора: построение имени
					 *          подраздела, знак-разделитель, знак примечания, признание примечания в
					 *          конце строки и запись управляющих последовательностей. Прочитанное
					 *          этими настройками записывается так, что читается обратно без потерь.
					 *
					 * @return настройки записи текста настроек
					 *
					 * \~english
					 * @brief Method of getting the settings of the writing corresponding to the settings of the parsing
					 *
					 * @details Infers the dialect of the writing from the dialect of the parsing: the construction of the name
					 *          of a subsection, the separator character, the comment character, the recognition of a comment at
					 *          the end of a line and the writing of the escape sequences. What has been read
					 *          by those settings is written so that it is read back without losses.
					 *
					 * @return settings of the writing of a settings text
					 *
					 * \~
					 */
					writer_t::settings_t writing() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Разрешаем оператор копирования
					 *
					 * \~english
					 * @brief Enable the copy operator
					 *
					 * \~
					 */
					Document & operator = (const Document &) = default;
					/**
					 * \~russian
					 * @brief Разрешаем оператор перемещения
					 *
					 * \~english
					 * @brief Enable the move operator
					 *
					 * \~
					 */
					Document & operator = (Document &&) noexcept = default;
				public:
					/**
					 * \~russian
					 * @brief Разрешаем конструктор копирования
					 *
					 * \~english
					 * @brief Enable the copy constructor
					 *
					 * \~
					 */
					Document(const Document &) = default;
					/**
					 * \~russian
					 * @brief Разрешаем конструктор перемещения
					 *
					 * \~english
					 * @brief Enable the move constructor
					 *
					 * \~
					 */
					Document(Document &&) noexcept = default;
				public:
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Document() noexcept;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param settings настройки дерева настроек
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * @param settings settings of the settings tree
					 *
					 * \~
					 */
					explicit Document(const settings_t & settings) noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					~Document() noexcept = default;
			} document_t;
		};
	};
};
