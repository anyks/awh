/**
 * @file: document.hpp
 * @date: 2026-08-10
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
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
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_INI_DOCUMENT__
#define __AWH_CODEC_INI_DOCUMENT__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
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
 * Снимаем на время объявлений макросы, чьи имена заняты
 * членами перечислений, применяемых ниже (возвращает их macro_pop.hpp в конце файла)
 */
#include "../../sys/macro_push.hpp"

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
			 * @brief Класс дерева настроек
			 *
			 * @details Хранит разобранный текст настроек целиком - вместе с примечаниями,
			 * пустыми строками и порядком записей, - и позволяет читать значения по имени,
			 * править их на месте и записывать обратно
			 *
			 * @par Сохранение оформления
			 *
			 * Файл настроек правит человек, и переписать его, потеряв примечания либо
			 * переставив разделы, значит отнять у хозяина то, ради чего файл этот
			 * читаемым и заведён. Оттого дерево держит **все** записи исходного текста в
			 * порядке их следования, а правка меняет лишь то, что ей велено
			 *
			 * @par Порядок работы
			 *
			 * @warning Выдаваемые последовательности знаков ссылаются на хранилище дерева
			 * и остаются пригодными **до первой правки**: @c set(), @c erase() и
			 * @c create() вправе перестроить хранилище. Значение, нужное после правки,
			 * следует скопировать
			 * @par Намеренные решения
			 * @li **Повторное свойство хранится целиком.** Все объявления попадают в
			 * дерево, а настройка обращения с повторами решает лишь то, какое из них
			 * выдаёт @c get(). Перечень всех значений выдаёт @c values() независимо от
			 * этой настройки: наречия Git и systemd повтором задают перечень, и терять
			 * его при разборе нельзя
			 * @li **Подстановка обращений пересчитывается лениво.** Значение, на которое
			 * ссылаются, вправе стоять ниже по тексту, и разрешить обращение можно лишь
			 * тогда, когда текст разобран целиком; поэтому разбор разрешает обращения
			 * разом, по своём завершении. Правка же дерева подстановку не пересчитывает,
			 * а лишь помечает устаревшей: правок подряд бывает много, и пересчёт после
			 * каждой обращает сборку дерева в квадратичную. Пересчёт выполняется перед
			 * первым чтением значения и ведётся от значений **до** подстановки, иначе
			 * подставленное подставлялось бы дважды
			 * @li **Обращение, ставшее неразрешимым после правки, отвергает запись, но
			 * не правку.** Удаление значения, на которое ссылаются, проходит, и @c get()
			 * выдаёт значение в том виде, как оно записано, вместе с самим обращением.
			 * А вот @c text() отвечает отказом @c error_t::UNKNOWN_REFERENCE: разбор
			 * такой текст с включённой подстановкой не примет, и выдать его значило бы
			 * отдать потребителю заведомо негодный файл
			 * @li **Удалённая запись остаётся в хранилище надгробием.** Изъятие её из
			 * середины сдвинуло бы порядковые номера всех записей за нею, а на них
			 * ссылаются указатели поиска. Надгробие обходится дешевле перестроения и
			 * при записи в выдачу не идёт
			 *
			 * \~english
			 * @brief Class of the settings tree
			 * @details Stores the parsed settings text in full — together with the comments,
			 * the empty lines and the order of the records — and makes it possible to read the values by a name,
			 * to edit them in place and to write them back
			 * @par Preservation of the formatting
			 * A settings file is edited by a human, and to rewrite it, having lost the comments or
			 * having rearranged the sections, means to take away from the owner that for the sake of which this file
			 * has been made readable in the first place. Because of that the tree keeps **all** the records of the source text in
			 * the order of their succession, while an editing changes only what it has been ordered to
			 * @par Order of the work
			 * @warning The issued sequences of characters refer to the storage of the tree
			 * and remain valid **until the first editing**: @c set(), @c erase() and
			 * @c create() have the right to rebuild the storage. A value needed after an editing
			 * should be copied
			 * @par Deliberate decisions
			 * @li **A repeated property is stored in full.** All the declarations get into
			 * the tree, while the setting of the treatment of the repetitions decides only which of them
			 * @c get() issues. The list of all the values is issued by @c values() independently of
			 * this setting: the Git and systemd dialects give a list by a repetition, and to lose
			 * it at the parsing is impermissible
			 * @li **The substitution of the references is recomputed lazily.** A value being
			 * referred to has the right to stand lower in the text, and a reference can be resolved only
			 * when the text has been parsed in full; therefore the parsing resolves the references
			 * all at once, upon its completion. An editing of the tree, however, does not recompute the substitution
			 * but only marks it stale: there happen to be many editings in a row, and a recomputation after
			 * each of them turns the assembly of the tree into a quadratic one. The recomputation is performed before
			 * the first reading of a value and is conducted from the values **before** the substitution, otherwise
			 * what has been substituted would be substituted twice
			 * @li **A reference that has become unresolvable after an editing rejects the writing but
			 * not the editing.** A removal of a value being referred to goes through, and @c get()
			 * issues the value in the form in which it has been written, together with the reference itself.
			 * But @c text() answers with an @c error_t::UNKNOWN_REFERENCE refusal: the parsing
			 * will not accept such a text with the substitution enabled, and to issue it would mean
			 * to give the consumer a knowingly unfit file
			 * @li **A removed record remains in the storage as a tombstone.** Its extraction from
			 * the middle would shift the ordinal numbers of all the records after it, while the pointers of the search
			 * refer to them. A tombstone comes cheaper than a rebuilding and
			 * does not go into the output at the writing
			 *
			 * \~
			 *
			 * @code{.cpp}
			 * document_t document;
			 *
			 * if(document.parse(text)){
			 *   const string_view host = document.get("host", "server");
			 *   document.set("port", "9090", "server");
			 *   const string result = document.text();
			 * }
			 * @endcode
			 *
			 *
			 *
			 *
			 *
			 *
			 *
			 */
			typedef class __AWH_SHARED_EXPORT__ Document {
				public:
					/**
					 * \~russian
					 * @brief Настройки дерева настроек
					 *
					 * \~english
					 * @brief Settings of the settings tree
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Settings {
						// Настройки разбора текста настроек
						reader_t::settings_t reader;
						// Построение обращения к значению другого свойства
						reference_t references;
						// Наибольшая допустимая глубина вложенности обращений
						uint32_t maxDepth;
						// Наибольший допустимый общий объём подстановки значений в байтах
						uint64_t maxExpansion;
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
					} settings_t;
				private:
					/**
					 * \~russian
					 * @brief Запись разобранного текста настроек
					 *
					 * @details Место значений хранится отрезком общего хранилища знаков:
					 * хранилище при правке дописывается и перемещается, обесценивая
					 * ссылки на своё содержимое
					 *
					 * \~english
					 * @brief Record of the parsed settings text
					 * @details The place of the values is stored as a segment of the common storage of the characters:
					 * the storage is appended to at an editing and is moved, invalidating
					 * the references to its content
					 *
					 * \~
					 */
					typedef struct Record {
						// Вид записи текста настроек
						kind_t kind;
						// Порядковый номер раздела, которому запись принадлежит
						uint32_t section;
						// Место имени свойства в хранилище знаков
						span_t key;
						// Место значения свойства в хранилище знаков
						span_t value;
						/**
						 * \~russian
						 * Место значения свойства до подстановки обращений
						 *
						 * @note Держится ради обратной записи: записать подставленное
						 * значение значило бы разрешить обращение навсегда, а оно на то
						 * и записано, чтобы разрешаться заново при каждом чтении файла
						 *
						 * \~english
						 * Place of the value of the property before the substitution of the references
						 * @note It is kept for the sake of the writing back: to write a substituted
						 * value would mean to resolve a reference forever, while it is written exactly
						 * so as to be resolved anew at every reading of the file
						 *
						 * \~
						 */
						span_t raw;
						// Знак, которым начато примечание
						char marker;
						// Расположение примечания в тексте настроек
						placement_t placement;
						/**
						 * \~russian
						 * Признак того, что значение было заключено в кавычки
						 *
						 * @note Держится ради обратной записи наравне со знаком примечания:
						 * кавычки, поставленные человеком, при перезаписи сохраняются, даже
						 * когда значение в них более не нуждается
						 *
						 * \~english
						 * Flag of the value having been enclosed in quotes
						 * @note It is kept for the sake of the writing back on a par with the comment character:
						 * the quotes put by a human are preserved at a rewriting even
						 * when the value no longer needs them
						 *
						 * \~
						 */
						bool quoted;
						// Признак свойства, записанного без разделителя и значения
						bool valueless;
						// Признак свойства, записанного добавлением к перечню значений
						bool append;
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
						Record() noexcept :
						 kind(kind_t::NONE), section(0), marker(';'), placement(placement_t::OWN),
						 quoted(false), valueless(false), append(false) {}
					} record_t;
					/**
					 * \~russian
					 * @brief Раздел разобранного текста настроек
					 *
					 * \~english
					 * @brief Section of the parsed settings text
					 *
					 * \~
					 */
					typedef struct Section {
						// Место имени раздела в хранилище знаков
						span_t name;
						// Место имени подраздела в хранилище знаков
						span_t subsection;
						/**
						 * \~russian
						 * Признак того, что раздел объявлен в тексте настроек
						 *
						 * @note Раздел без имени заводится всегда: свойства, записанные до
						 * первого объявления, принадлежат ему. Объявленным он при этом не
						 * является и в перечень разделов не идёт
						 *
						 * \~english
						 * Flag of the section having been declared in the settings text
						 * @note A section without a name is always created: the properties written before
						 * the first declaration belong to it. It is thereby not a declared one
						 * and does not go into the list of the sections
						 *
						 * \~
						 */
						bool declared;
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
						Section() noexcept : declared(false) {}
					} section_t;
				private:
					/**
					 * \~russian
					 * Код ошибки последней операции
					 *
					 * @note Помечен изменяемым: обратная запись дерева в текст его самого не
					 * меняет и потому объявлена постоянной, но отказ писателя сообщить
					 * потребителю обязана
					 *
					 * \~english
					 * Error code of the last operation
					 * @note Marked as mutable: the writing back of the tree into a text does not change the tree itself
					 * and is therefore declared a constant one, but it is obliged to report a refusal of the writer
					 * to the consumer
					 *
					 * \~
					 */
					mutable error_t _error;
				private:
					// Положение обнаруженной ошибки в исходном тексте
					location_t _errorLocation;
				private:
					/**
					 * \~russian
					 * Признак того, что дерево несёт обращения к значениям других свойств
					 *
					 * @note Держится ради цены правки: подстановка выполняется заново после
					 * всякой правки дерева, а обходится она перебором всех записей. Дереву,
					 * обращений не несущему, - а таких большинство, - платить за это незачем
					 *
					 * \~english
					 * Flag of the tree carrying the references to the values of the other properties
					 * @note It is kept for the sake of the price of an editing: the substitution is performed anew after
					 * every editing of the tree, while it costs a traversal of all the records. A tree that
					 * carries no references — and such ones are the majority — has no reason to pay for this
					 *
					 * \~
					 */
					bool _referenced;
					/**
					 * \~russian
					 * Признак того, что подстановка обращений устарела после правки
					 *
					 * @note Помечен изменяемым наравне с кодом ошибки: пересчёт откладывается
					 * до первого чтения значения, а чтение объявлено постоянным. Смысла дерева
					 * пересчёт не меняет - он приводит разрешённые значения в соответствие с
					 * записанными, - и постоянности чтения тем не нарушает
					 *
					 * \~english
					 * Flag of the substitution of the references having become stale after an editing
					 * @note Marked as mutable on a par with the error code: the recomputation is postponed
					 * until the first reading of a value, while the reading is declared a constant one. The recomputation does not change
					 * the meaning of the tree — it brings the resolved values into a correspondence with
					 * the written ones — and thereby does not violate the constancy of the reading
					 *
					 * \~
					 */
					mutable bool _stale;
					/**
					 * \~russian
					 * Признак того, что дерево несёт неразрешимое обращение
					 *
					 * @note Взводится снисходительным пересчётом: правка вправе удалить
					 * источник обращения, и чтение такое значение выдаёт в записанном виде.
					 * Обратная же запись им отвечает отказом - текст с обращением в пустоту
					 * разбор отвергает, и выдать его значило бы отдать заведомо негодный файл
					 *
					 * \~english
					 * Flag of the tree carrying an unresolvable reference
					 * @note Raised by an indulgent recomputation: an editing has the right to remove
					 * the source of a reference, and the reading issues such a value in the written form.
					 * The writing back, however, answers it with a refusal — a text with a reference into emptiness
					 * the parsing rejects, and to issue it would mean to give away a knowingly unfit file
					 *
					 * \~
					 */
					mutable bool _dangling;
				private:
					// Общее хранилище знаков имён и значений
					string _store;
				private:
					// Перечень записей разобранного текста в порядке их следования
					vector <record_t> _records;
				private:
					// Перечень разделов разобранного текста в порядке их объявления
					vector <section_t> _sections;
				private:
					// Указатель разделов по имени, приведённому к виду для сличения
					unordered_map <string, uint32_t> _index;
				private:
					// Указатель свойств по разделу и имени, приведённым к виду для сличения
					unordered_map <string, vector <uint32_t>> _properties;
				private:
					/**
					 * \~russian
					 * Порядок первых объявлений свойств по каждому разделу
					 *
					 * @details Держит для каждого раздела перечень записей, где его
					 * свойства объявлены впервые, в порядке их следования в тексте
					 *
					 * @note Без него выдача перечня имён свойств обращалась в
					 * квадратичную: перебор всех записей дерева ради каждого раздела на
					 * сотне тысяч разделов давал двести раз по сотне тысяч проходов.
					 * Обнаружено стендом сравнения `tools/benchmark/ini`, где обход
					 * дерева со множеством разделов показал 0.25 МБ/с против 55 МБ/с
					 * его же сборки
					 *
					 * \~english
					 * Order of the first declarations of the properties for every section
					 * @details Keeps for every section a list of the records where its
					 * properties are declared for the first time, in the order of their succession in the text
					 * @note Without it the issuance of the list of the names of the properties turned into a
					 * quadratic one: a traversal of all the records of the tree for the sake of every section on
					 * a hundred thousand sections gave two hundred times a hundred thousand passes.
					 * Detected by the `tools/benchmark/ini` benchmark, where a traversal
					 * of a tree with a multitude of sections showed 0.25 MB/s against 55 MB/s
					 * of the assembly of the same tree
					 *
					 * \~
					 */
					vector <vector <uint32_t>> _order;
				private:
					/**
					 * \~russian
					 * Последняя запись каждого раздела в перечне записей
					 *
					 * @details Держит для каждого раздела порядковый номер последней его
					 * записи, объявлена та в тексте или снята пометкой удаления. Место
					 * вставки нового свойства находится по нему за одно обращение
					 *
					 * @note Снятые пометкой записи учитываются наравне с прочими намеренно:
					 * в текст они не идут, и вставка за такой записью даёт тот же текст,
					 * что и вставка за последней оставшейся
					 *
					 * \~english
					 * Last record of every section in the list of the records
					 * @details Keeps for every section the ordinal number of its last
					 * record, whether it is declared in the text or removed by a mark of a deletion. The place
					 * of the insertion of a new property is found by it in a single call
					 * @note The records removed by a mark are taken into account on a par with the rest deliberately:
					 * they do not go into the text, and an insertion after such a record gives the same text
					 * as an insertion after the last remaining one
					 *
					 * \~
					 */
					vector <uint32_t> _last;
				private:
					// Настройки дерева настроек
					settings_t _settings;
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
					 * разбор: имя, записать которое обратно нельзя, отвергается в месте
					 * правки, а не при записи собранного дерева
					 *
					 * @param name    проверяемое имя раздела или свойства
					 * @param section признак проверки имени раздела
					 * @param primary признак проверки имени самого раздела, а не подраздела
					 * @return        результат выполнения операции
					 *
					 * \~english
					 * @brief Method of checking the name of a section or of a property
					 * @details The name is checked by the same rules by which the parsing checks it:
					 * a name which cannot be written back is rejected at the place of the
					 * editing rather than at the writing of the assembled tree
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
					 * строкой: буфер вызывающий заводит однажды и переиспользует, отчего
					 * на обращение к дереву не приходится ни одного построения строки.
					 * Приведение имени к нижнему регистру выполняется по ходу дозаписи, а
					 * не отдельным проходом
					 *
					 * @param buffer     буфер, куда собирается ключ
					 * @param section    имя раздела
					 * @param subsection имя подраздела
					 *
					 * \~english
					 * @brief Method of assembling the key of the index of the sections into a passed buffer
					 * @details The key is assembled into the passed buffer rather than being issued as a new
					 * string: the buffer is created by the caller once and reused, from which
					 * not a single construction of a string falls on a call to the tree.
					 * The bringing of the name to the lower case is performed in the course of the appending rather than
					 * by a separate pass
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
					 * свойства: получение значения, проверка наличия и разбор числом
					 * прежде повторяли одну и ту же работу дважды
					 *
					 * @param key        имя искомого свойства
					 * @param section    имя раздела
					 * @param subsection имя подраздела
					 * @return           перечень объявлений свойства либо пустой указатель
					 *
					 * \~english
					 * @brief Method of searching for the declarations of a property by a name
					 * @details The search is performed once for all the calls to the value of a
					 * property: the getting of a value, the check of the presence and the parsing as a number
					 * used to repeat one and the same work twice
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
					 * порядковые номера записей при вставке сдвигаются, и указатели,
					 * на них ссылающиеся, обесцениваются
					 *
					 * \~english
					 * @brief Method of rebuilding the indexes of the search
					 * @details Performed after every editing that changes the composition of the records:
					 * the ordinal numbers of the records are shifted at an insertion, and the indexes
					 * referring to them are invalidated
					 *
					 * \~
					 */
					void reindex() noexcept;
					/**
					 * \~russian
					 * @brief Метод подстановки обращений к значениям других свойств
					 *
					 * @param strict признак прекращения подстановки неразрешимым обращением
					 *
					 * @return результат выполнения операции
					 *
					 * \~english
					 * @brief Method of substituting the references to the values of the other properties
					 * @param strict flag of the termination of the substitution by an unresolvable reference
					 * @return result of performing the operation
					 *
					 * \~
					 */
					bool resolve(const bool strict = true) noexcept;
					/**
					 * \~russian
					 * @brief Метод пересчёта подстановки обращений после правки дерева
					 *
					 * @details Правка меняет значения, на которые обращения ссылаются, и
					 * прежде разрешённые значения от неё устаревают: чтение выдавало бы
					 * подставленное до правки, расходясь с тем, что дало бы чтение
					 * записанного дерева обратно. Пересчёт ведётся от значений до
					 * подстановки и потому повторяем
					 *
					 * @note Дерево, обращений не несущее, пересчёта не требует вовсе:
					 * признак наличия обращений взводится подстановкой сама
					 *
					 * @return результат выполнения операции
					 *
					 * \~english
					 * @brief Method of recomputing the substitution of the references after an editing of the tree
					 * @details An editing changes the values which the references refer to, and
					 * the previously resolved values become stale from it: the reading would issue
					 * what has been substituted before the editing, diverging from what the reading of the written tree
					 * back would give. The recomputation is conducted from the values before
					 * the substitution and is therefore repeatable
					 * @note A tree that carries no references does not require a recomputation at all:
					 * the flag of the presence of the references is raised by the substitution itself
					 * @return result of performing the operation
					 *
					 * \~
					 */
					bool substitute() noexcept;
					/**
					 * \~russian
					 * @brief Метод пересчёта устаревшей подстановки перед чтением значения
					 *
					 * @details Правка помечает подстановку устаревшей, а пересчитывается она
					 * перед первым чтением: правок подряд бывает много, и пересчёт после
					 * каждой обращал бы сборку дерева правками в квадратичную
					 *
					 * \~english
					 * @brief Method of recomputing a stale substitution before the reading of a value
					 * @details An editing marks the substitution stale, while it is recomputed
					 * before the first reading: there happen to be many editings in a row, and a recomputation after
					 * each of them would turn the assembly of a tree by editings into a quadratic one
					 *
					 * \~
					 */
					void refresh() const noexcept;
					/**
					 * \~russian
					 * @brief Метод разрешения обращений внутри значения свойства
					 *
					 * @param value   разрешаемое значение свойства
					 * @param section порядковый номер раздела, которому значение принадлежит
					 * @param stack   перечень свойств, разрешаемых в настоящее время
					 * @param budget  остаток допустимого объёма подстановки в байтах
					 * @param result  значение с разрешёнными обращениями
					 * @return        результат выполнения операции
					 *
					 * @note Перечень разрешаемых свойств служит обнаружению круговой
					 * ссылки: значение, к которому обращение вернулось, в нём уже лежит.
					 * Глубина же его сверх того ограничена пределом настроек - на случай
					 * связи хоть и не круговой, но неразумно длинной
					 *
					 * \~english
					 * @brief Method of resolving the references inside the value of a property
					 * @param value   value of the property being resolved
					 * @param section ordinal number of the section to which the value belongs
					 * @param stack   list of the properties being resolved at the present moment
					 * @param budget  remainder of the admissible volume of the substitution in bytes
					 * @param result  value with the resolved references
					 * @return        result of performing the operation
					 * @note The list of the properties being resolved serves the detection of a circular
					 * reference: a value to which a reference has returned already lies in it.
					 * Its depth beyond that is limited by the limit of the settings — for the case of a
					 * linkage that is not circular but unreasonably long
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
					 * и порядок записей. Прежнее содержимое дерева при этом освобождается
					 *
					 * @note Примечания и пустые строки читаются всегда, как бы ни были
					 * выставлены соответствующие флаги настроек разбора: дерево держит
					 * оформление текста, и отказ от них лишил бы его этой возможности
					 *
					 * @param text разбираемый текст настроек
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of parsing a settings text
					 * @details Parses the text in full, preserving the comments, the empty lines
					 * and the order of the records. The previous content of the tree is thereby released
					 * @note The comments and the empty lines are always read, however the corresponding
					 * flags of the settings of the parsing are set: the tree keeps
					 * the formatting of the text, and a refusal of them would deprive it of that possibility
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
					 *
					 * \~english
					 * @brief Method of getting the error code of the parsing
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
					 *
					 * \~english
					 * @brief Method of getting the place of the detection of an error
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
					 * @details Разделы выдаются в порядке их объявления в исходном тексте
					 *
					 * @return перечень объявленных разделов текста настроек
					 *
					 * \~english
					 * @brief Method of getting the list of the declared sections
					 * @details The sections are issued in the order of their declaration in the source text
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
					 * свойство, объявленное несколько раз, выдаётся именем однажды
					 *
					 * @param section    имя раздела
					 * @param subsection имя подраздела
					 * @return           перечень имён свойств раздела
					 *
					 * \~english
					 * @brief Method of getting the list of the names of the properties of a section
					 * @details The names are issued in the order of their declaration and carry no repetitions:
					 * a property declared several times is issued by a name once
					 * @param section    name of the section
					 * @param subsection name of the subsection
					 * @return           list of the names of the properties of the section
					 *
					 * \~
					 */
					vector <string_view> keys(const string_view section = "", const string_view subsection = "") const noexcept;
				public:
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
					 * @brief Метод получения значения свойства
					 *
					 * @details При нескольких объявлениях свойства выдаётся то из них,
					 * которое велит выдать настройка обращения с повторами: первое, либо
					 * последнее, либо - при собирании их в перечень - первое из перечня
					 *
					 * @param key        имя искомого свойства
					 * @param section    имя раздела
					 * @param subsection имя подраздела
					 * @return           значение найденного свойства либо пустая последовательность
					 *
					 * \~english
					 * @brief Method of getting the value of a property
					 * @details At several declarations of a property the one of them is issued
					 * which the setting of the treatment of the repetitions orders to issue: the first, or
					 * the last, or — at their assembling into a list — the first of the list
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
					 * Наречия Git и systemd повтором свойства задают перечень значений, и
					 * читать его следует именно так
					 *
					 * @param key        имя искомого свойства
					 * @param section    имя раздела
					 * @param subsection имя подраздела
					 * @return           перечень значений найденного свойства
					 *
					 * \~english
					 * @brief Method of getting the list of the values of a property
					 * @details All the declarations of the property are issued in the order of their succession.
					 * The Git and systemd dialects give a list of the values by a repetition of a property, and
					 * it should be read exactly that way
					 * @param key        name of the property being sought
					 * @param section    name of the section
					 * @param subsection name of the subsection
					 * @return           list of the values of the found property
					 *
					 * \~
					 */
					vector <string_view> values(const string_view key, const string_view section = "", const string_view subsection = "") const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Шаблон типа числа результата разбора
					 *
					 * @tparam T тип числа результата разбора
					 *
					 *
					 * \~english
					 * @brief Template of the number type of the parsing result
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
					 * отвечает успехом, оставляя дерево нетронутым
					 *
					 * @param section    имя объявляемого раздела
					 * @param subsection имя объявляемого подраздела
					 * @return           результат выполнения операции
					 *
					 * \~english
					 * @brief Method of declaring a section
					 * @details An already declared section is not declared again: the method
					 * answers with success, leaving the tree untouched
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
					 * ни порядок записей, ни примечания при этом не страдают. Отсутствующее
					 * свойство дописывается в конец своего раздела, а отсутствующий раздел
					 * объявляется
					 *
					 * @note При нескольких объявлениях свойства заменяется значение того из
					 * них, которое выдал бы @c get(), а прочие остаются нетронутыми: замена
					 * их всех обеднила бы перечень значений, ради которого повтор и записан
					 *
					 * @param key        имя устанавливаемого свойства
					 * @param value      устанавливаемое значение свойства
					 * @param section    имя раздела
					 * @param subsection имя подраздела
					 * @return           результат выполнения операции
					 *
					 * \~english
					 * @brief Method of setting the value of a property
					 * @details The value of an already declared property is replaced in place, and
					 * neither the order of the records nor the comments suffer thereby. An absent
					 * property is appended to the end of its section, while an absent section
					 * is declared
					 * @note At several declarations of a property the value of the one of
					 * them is replaced which @c get() would issue, while the rest remain untouched: a replacement
					 * of all of them would impoverish the list of the values for the sake of which the repetition has been written
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
					 * @brief Метод удаления свойства
					 *
					 * @details Удаляются все объявления свойства вместе с примечаниями,
					 * записанными в конце их строк
					 *
					 * @param key        имя удаляемого свойства
					 * @param section    имя раздела
					 * @param subsection имя подраздела
					 * @return           результат выполнения операции
					 *
					 * \~english
					 * @brief Method of removing a property
					 * @details All the declarations of the property are removed together with the comments
					 * written at the end of their lines
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
					 * @brief Метод удаления раздела
					 *
					 * @details Удаляется объявление раздела вместе со всеми его записями
					 *
					 * @param section    имя удаляемого раздела
					 * @param subsection имя удаляемого подраздела
					 * @return           результат выполнения операции
					 *
					 * \~english
					 * @brief Method of removing a section
					 * @details The declaration of the section is removed together with all its records
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
				public:
					/**
					 * \~russian
					 * @brief Метод записи дерева обратно в текст настроек
					 *
					 * @details Записи выдаются в том же порядке, в каком они прочитаны,
					 * вместе с примечаниями и пустыми строками
					 *
					 * @note Пустая строка перед объявлением раздела настройками записи не
					 * ставится: расстановка их взята из исходного текста, и добавлять к
					 * ней свою значило бы наращивать пустые строки при каждом обороте
					 * «чтение - запись»
					 *
					 * @param settings настройки записи текста настроек
					 * @return         собранный текст настроек
					 *
					 * \~english
					 * @brief Method of writing the tree back into a settings text
					 * @details The records are issued in the same order in which they have been read,
					 * together with the comments and the empty lines
					 * @note An empty line before a section declaration is not put by the settings of the writing:
					 * their arrangement is taken from the source text, and to add one's own to
					 * it would mean to increase the empty lines at every «reading — writing»
					 * turn
					 * @param settings settings of the writing of a settings text
					 * @return         assembled settings text
					 *
					 * \~
					 */
					string text(const writer_t::settings_t & settings) const noexcept;
					/**
					 * \~russian
					 * @brief Метод записи дерева обратно в текст настроек
					 *
					 * @details Настройки записи выводятся из настроек разбора: наречие, каким
					 * текст прочитан, тем же и записывается. Задать иное наречие можно,
					 * передав настройки записи явно
					 *
					 * @return собранный текст настроек
					 *
					 * \~english
					 * @brief Method of writing the tree back into a settings text
					 * @details The settings of the writing are inferred from the settings of the parsing: the dialect by which
					 * the text has been read is the one it is written by as well. Another dialect can be given
					 * by passing the settings of the writing explicitly
					 * @return assembled settings text
					 *
					 * \~
					 */
					string text() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения настроек записи, отвечающих настройкам разбора
					 *
					 * @details Выводит наречие записи из наречия разбора: построение имени
					 * подраздела, знак-разделитель, знак примечания, признание примечания в
					 * конце строки и запись управляющих последовательностей. Прочитанное
					 * этими настройками записывается так, что читается обратно без потерь
					 *
					 * @return настройки записи текста настроек
					 *
					 * \~english
					 * @brief Method of getting the settings of the writing corresponding to the settings of the parsing
					 * @details Infers the dialect of the writing from the dialect of the parsing: the construction of the name
					 * of a subsection, the separator character, the comment character, the recognition of a comment at
					 * the end of a line and the writing of the escape sequences. What has been read
					 * by those settings is written so that it is read back without losses
					 * @return settings of the writing of a settings text
					 *
					 * \~
					 */
					writer_t::settings_t writing() const noexcept;
				public:
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
					Document() noexcept;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param settings настройки дерева настроек
					 *
					 * \~english
					 * @brief Constructor
					 * @param settings settings of the settings tree
					 *
					 * \~
					 */
					explicit Document(const settings_t & settings) noexcept;
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
					~Document() noexcept;
			} document_t;
		};
	};
};

/**
 * Возвращаем макросы, снятые в начале файла
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_CODEC_INI_DOCUMENT__
