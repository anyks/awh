/**
 * @file document.hpp
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
 * @brief Заголовочный файл дерева настроек TOML — класс Document, хранящий разобранный
 *        текст целиком вместе с примечаниями, пустыми строками и порядком записей,
 *        дающий чтение значений по составному имени, правку их на месте и запись обратно
 *
 * \~english
 * @brief Header file of the TOML settings tree — the Document class, which stores the parsed
 *        text in full together with the comments, the empty lines and the order of the records,
 *        giving the reading of the values by a compound name, their in-place editing and a writing back
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_TOML_DOCUMENT__
#define __AWH_CODEC_TOML_DOCUMENT__

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <string>
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

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
		 * @brief Пространство имён контейнера TOML
		 *
		 *
		 * \~english
		 * @brief TOML container namespace
		 *
		 * \~
		 */
		namespace toml {
			/**
			 * \~russian
			 * @brief Виды записей текста настроек
			 *
			 * \~english
			 * @brief Kinds of the records of a settings text
			 *
			 * \~
			 */
			enum class kind_t : uint8_t {
				NONE        = 0x00, // Запись удалена и в выдачу не идёт
				TABLE       = 0x01, // Объявление таблицы
				ARRAY_TABLE = 0x02, // Объявление очередной таблицы набора таблиц
				PAIR        = 0x03, // Пара «ключ = значение»
				COMMENT     = 0x04, // Примечание
				BLANK       = 0x05  // Пустая строка
			};

			/**
			 * \~russian
			 * @brief Класс дерева настроек
			 *
			 * @details Хранит разобранный текст настроек целиком - вместе с примечаниями,
			 * пустыми строками и порядком записей, - и позволяет читать значения по
			 * составному имени, править их на месте и записывать обратно
			 *
			 * @par Сохранение оформления
			 *
			 * Файл настроек правит человек, и переписать его, потеряв примечания либо
			 * переставив таблицы, значит отнять у хозяина то, ради чего файл этот читаемым
			 * и заведён. Оттого дерево держит **все** записи исходного текста в порядке их
			 * следования вместе с оградой строк, системой счисления чисел и видом отметок
			 * времени, а правка меняет лишь то, что ей велено
			 *
			 * @par Порядок работы
			 *
			 * @warning Выдаваемые последовательности знаков ссылаются на хранилище дерева и
			 * остаются пригодными **до первой правки**: @c set(), @c erase() и @c create()
			 * вправе перестроить хранилище. Значение, нужное после правки, следует
			 * скопировать
			 * @par Намеренные решения
			 * @li **Составное имя задаётся частями, а не строкой с точками.** Точка внутри
			 * имени, огражденного кавычками, частей не разделяет, и разбирать составное имя
			 * из строки значило бы вводить второй разбор со своими правилами ограждения.
			 * Части передаются перечнем, и всякая из них - имя целиком
			 * @li **Таблица набора таблиц адресуется порядковым номером частью имени.**
			 * Запись «[[products]]» заводит не таблицу, а их набор, и обратиться к его
			 * второй таблице можно лишь номером: имя у них общее. Номер записывается
			 * десятичной частью имени - «products», «1», «name», - и разночтения с
			 * настоящим ключом «1» не даёт: набор таблиц дерево знает по своему устройству,
			 * а не по записи имени
			 * @li **Правка меняет запись на месте, а не переписывает файл.** Значение уже
			 * объявленной пары заменяется в её собственной записи, отчего ни порядок, ни
			 * примечания, ни пустые строки не страдают. Отсутствующая пара дописывается в
			 * конец своей таблицы, а отсутствующая таблица объявляется в конце текста
			 * @li **Удалённая запись остаётся в хранилище надгробием.** Изъятие её из
			 * середины сдвинуло бы порядковые номера всех записей за нею, а на них
			 * ссылаются указатели поиска. Надгробие обходится дешевле перестроения и при
			 * записи в выдачу не идёт
			 * @li **Примечание в конце строки удаляется вместе со своей записью.**
			 * Примечание это писано к паре, и оставить его при удалении пары значило бы
			 * оставить пояснение к тому, чего в файле более нет
			 * @li **Пустая строка перед объявлением таблицы записью не ставится.**
			 * Расстановка их взята из исходного текста, и добавлять к ней свою значило бы
			 * наращивать пустые строки при каждом обороте «чтение - запись»
			 * @li **Оформление записи числа перезаписью не хранится.** Держится система
			 * счисления, но не разделители разрядов, не знак «плюс» перед положительным числом
			 * и не разряд букв: «1_000_000» перезаписывается как «1000000», «+42» как «42»,
			 * «0xdead» как «0xDEAD», «1E10» как «1e+10». Значение при этом то же самое, а
			 * хранить исходную запись числа значило бы держать её текст рядом со значением ради
			 * одного лишь вида
			 * @li **Удаление таблицы вложенных её таблиц не трогает.** Вызов
			 * @c remove({"server"}) снимает объявление «[server]» и пары его, а «[server.limits]»
			 * остаётся: таблица эта объявлена своей записью и вложена в «server» лишь именем.
			 * Удаляют её отдельным вызовом по её имени
			 * @li **Остаток составного имени ведёт внутрь значения.** Имя ищется от самого
			 * длинного начала к самому короткому, а остаток его спускается в значение найденной
			 * пары: порядковым номером в перечень, именем ключа во встроенную таблицу. Записи
			 * «a = [[1, 2], [3, 4]]» отвечает имя «a», «1», «0», а записи «c = {d = {e = 5}}» -
			 * имя «c», «d», «e». Спуск этот назначен чтению: правкою составные значения не
			 * задаются
			 * @li **Правка задаёт простые значения, но не составные.** Перечень либо встроенную
			 * таблицу правка целиком не собирает: назначение дерева - править готовый файл
			 * настроек, а не строить его с нуля, и составное значение, собранное правкой,
			 * потребовало бы своего построителя со своими правилами
			 * @li **Длина исходного текста ограничена четырьмя гигабайтами.** Отрезок хранилища
			 * знаков задан смещением и длиной в четыре октета, и текст длиннее отвергается
			 * отказом: смещение в восемь октетов стоило бы пятой части памяти дерева на всяком
			 * имени и всяком значении ради размера, которого файл настроек не достигает
			 * @li **Мусор правки дерево уплотняет само.** Замещённый узел значения, снятая
			 * запись и содержимое, никем не читаемое, остаются в хранилищах до уплотнения, а
			 * уплотнение приходит по накоплении их вровень с живым. Оттого объём дерева гуляет,
			 * но неограниченным не бывает: узнают его вызовом @c footprint()
			 *
			 *  @code{.cpp}
			 *  document_t document;
			 *
			 *  if(document.parse(text)){
			 *    const string_view host = document.text({"server", "host"});
			 *    document.set({"server", "port"}, static_cast <int64_t> (9090));
			 *    const string result = document.text();
			 *  }
			 *  @endcode
			 *
			 * \~english
			 * @brief Class of the settings tree
			 * @details Stores the parsed settings text in full — together with the comments,
			 * the empty lines and the order of the records — and makes it possible to read the values by a
			 * compound name, to edit them in place and to write them back
			 * @par Preservation of the formatting
			 * A settings file is edited by a human, and to rewrite it, having lost the comments or
			 * having rearranged the tables, means to take away from the owner that for the sake of which this file
			 * has been made readable in the first place. Because of that the tree keeps **all** the records of the source text in the order of their
			 * succession together with the fence of the strings, the numeral system of the numbers and the kind of the timestamps,
			 * while an editing changes only what it has been ordered to
			 * @par Order of the work
			 * @warning The issued sequences of characters refer to the storage of the tree and
			 * remain valid **until the first editing**: @c set(), @c erase() and @c create()
			 * have the right to rebuild the storage. A value needed after an editing should be
			 * copied
			 * @par Deliberate decisions
			 * @li **A compound name is given by parts rather than as a string with dots.** A dot inside
			 * a name fenced with quotes does not separate the parts, and to parse a compound name
			 * out of a string would mean to introduce a second parsing with its own rules of the fencing.
			 * The parts are passed as a list, and every one of them is a name in full
			 * @li **A table of an array of tables is addressed by an ordinal number as a part of the name.**
			 * The record «[[products]]» creates not a table but an array of them, and its second
			 * table can be addressed only by a number: their name is a common one. The number is written
			 * as a decimal part of the name — «products», «1», «name» — and it gives no discrepancy with
			 * an actual key «1»: the tree knows an array of tables by its own arrangement
			 * rather than by the notation of the name
			 * @li **An editing changes a record in place rather than rewriting the file.** The value of an already
			 * declared pair is replaced in its own record, from which neither the order, nor the
			 * comments, nor the empty lines suffer. An absent pair is appended to the
			 * end of its table, while an absent table is declared at the end of the text
			 * @li **A removed record remains in the storage as a tombstone.** Its extraction from
			 * the middle would shift the ordinal numbers of all the records after it, while the pointers of the search
			 * refer to them. A tombstone comes cheaper than a rebuilding and at
			 * the writing does not go into the output
			 * @li **A comment at the end of a line is removed together with its record.**
			 * That comment has been written to a pair, and to leave it at the removal of the pair would mean
			 * to leave an explanation of what is no longer in the file
			 * @li **An empty line before a table declaration is not put by the writing.**
			 * Their arrangement is taken from the source text, and to add one's own to it would mean
			 * to increase the empty lines at every «reading — writing» turn
			 * @li **The formatting of the notation of a number is not preserved by a rewriting.** The numeral
			 * system is kept, but not the digit separators, not the «plus» sign before a positive number
			 * and not the case of the letters: «1_000_000» is rewritten as «1000000», «+42» as «42»,
			 * «0xdead» as «0xDEAD», «1E10» as «1e+10». The value is thereby the same, while
			 * to keep the source notation of a number would mean to keep its text next to the value for the sake of
			 * the form alone
			 * @li **A removal of a table does not touch the tables nested in it.** The call
			 * @c remove({"server"}) removes the declaration «[server]» and its pairs, while «[server.limits]»
			 * remains: that table is declared by its own record and is nested into «server» only by the name.
			 * It is removed by a separate call by its name
			 * @li **The remainder of a compound name leads inside a value.** The name is sought from the
			 * longest beginning to the shortest one, while its remainder descends into the value of the found
			 * pair: by an ordinal number into an array, by the name of a key into an inline table. To the record
			 * «a = [[1, 2], [3, 4]]» corresponds the name «a», «1», «0», while to the record «c = {d = {e = 5}}» —
			 * the name «c», «d», «e». This descent is intended for the reading: the compound values are not
			 * given by an editing
			 * @li **An editing gives the simple values but not the compound ones.** An array or an inline
			 * table an editing does not assemble in full: the purpose of the tree is to edit a ready settings
			 * file rather than to build it from scratch, and a compound value assembled by an editing
			 * would require its own builder with its own rules
			 * @li **The length of the source text is limited by four gigabytes.** A segment of the storage
			 * of the characters is given by an offset and a length of four octets, and a longer text is rejected
			 * with a refusal: an offset of eight octets would cost a fifth of the memory of the tree on every
			 * name and every value for the sake of a size which a settings file does not reach
			 * @li **The tree compacts the garbage of the editing itself.** A replaced node of a value, a removed
			 * record and a content read by nobody remain in the storages until a compaction, while
			 * the compaction comes upon their accumulation on a par with the live one. Because of that the volume of the tree fluctuates
			 * but is never unlimited: it is learnt by a call to @c footprint()
			 *
			 *  @code{.cpp}
			 *  document_t document;
			 *
			 *  if(document.parse(text)){
			 *    const string_view host = document.text({"server", "host"});
			 *    document.set({"server", "port"}, static_cast <int64_t> (9090));
			 *    const string result = document.text();
			 *  }
			 *  @endcode
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
					 * @brief Отрезок общего хранилища знаков
					 *
					 * \~english
					 * @brief Segment of the common storage of the characters
					 *
					 * \~
					 */
					typedef struct Span {
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
					} span_t;
					/**
					 * \~russian
					 * @brief Составная часть имени ключа
					 *
					 * \~english
					 * @brief Component part of the name of a key
					 *
					 * \~
					 */
					typedef struct Key {
						// Место имени части в хранилище знаков
						span_t name;
						// Запись имени части в исходном тексте
						naming_t naming;
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
						Key() noexcept : naming(naming_t::BARE) {}
					} key_t;
					/**
					 * \~russian
					 * @brief Примечание, стоящее внутри перечня значений
					 *
					 * @details Примечание внутри перечня держится не записью, а самим узлом
					 * перечня: строкой текста настроек оно не является и живёт вместе со
					 * значением, которому писано
					 *
					 * \~english
					 * @brief Comment standing inside an array of the values
					 * @details A comment inside an array is kept not by a record but by the node of the array
					 * itself: it is not a line of the settings text and lives together with
					 * the value to which it has been written
					 *
					 * \~
					 */
					typedef struct Remark {
						/**
						 * \~russian
						 * Количество значений перечня, записанных до примечания
						 *
						 * @note Место примечания задано счётом значений, а не ссылкою на
						 *       значение: примечание стоит и перед первым значением, и за
						 *       последним, где ссылаться не на что
						 *
						 * \~english
						 * Number of the values of the array written before the comment
						 * @note The place of a comment is given by a count of the values rather than by a reference to a
						 *       value: a comment stands both before the first value and after
						 *       the last one, where there is nothing to refer to
						 *
						 * \~
						 */
						uint32_t index;
						// Место содержимого примечания в хранилище знаков
						span_t text;
						/**
						 * \~russian
						 * Признак примечания, дописанного к строке значения
						 *
						 * @note Примечание перечня стоит либо своей строкой, либо за
						 *       значением на строке его: обратная запись расстановку эту
						 *       хранит
						 *
						 * \~english
						 * Flag of a comment appended to the line of a value
						 * @note A comment of an array stands either as its own line or after
						 *       a value on its line: the writing back preserves that
						 *       arrangement
						 *
						 * \~
						 */
						bool trailing;
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
						Remark() noexcept : index(0), trailing(false) {}
					} remark_t;
					/**
					 * \~russian
					 * @brief Узел значения текста настроек
					 *
					 * @details Значение составное хранится деревом узлов: у перечня узлы
					 * значений его, у встроенной таблицы - узлы её пар. Узлы держатся общим
					 * перечнем и ссылаются друг на друга порядковыми номерами: ссылки на
					 * память хранилище при дописывании обесценивает
					 *
					 * \~english
					 * @brief Node of a value of a settings text
					 * @details A compound value is stored as a tree of the nodes: an array has the nodes
					 * of its values, an inline table — the nodes of its pairs. The nodes are kept as a common
					 * list and refer to one another by the ordinal numbers: the references to
					 * the memory are invalidated by the storage at an appending
					 *
					 * \~
					 */
					typedef struct Node {
						// Тип значения
						type_t type;
						// Запись строкового значения
						string_t quoting;
						// Система счисления записи целого числа
						radix_t radix;
						// Логическое значение
						bool boolean;
						/**
						 * \~russian
						 * Признак записи перечня несколькими строками
						 *
						 * @note Держится ради обратной записи: расстановка строк выбрана
						 *       человеком, и перезапись обязана её сохранять
						 *
						 * \~english
						 * Flag of the writing of an array in several lines
						 * @note It is kept for the sake of the writing back: the arrangement of the lines has been chosen
						 *       by the human, and a rewriting is obliged to preserve it
						 *
						 * \~
						 */
						bool multiline;
						// Целое число со знаком
						int64_t integer;
						// Число с плавающей точкой
						double real;
						// Отметка времени
						stamp_t stamp;
						// Место строкового значения в хранилище знаков
						span_t text;
						// Порядковые номера узлов значений перечня либо пар встроенной таблицы
						vector <uint32_t> items;
						/**
						 * \~russian
						 * Составные имена ключей пар встроенной таблицы
						 *
						 * @note Перечень этот идёт рядом с перечнем узлов: имя каждой пары
						 *       встроенной таблицы задано отрезком перечня частей имён
						 *
						 * \~english
						 * Compound names of the keys of the pairs of an inline table
						 * @note This list goes next to the list of the nodes: the name of every pair
						 *       of an inline table is given by a segment of the list of the parts of the names
						 *
						 * \~
						 */
						vector <span_t> names;
						/**
						 * \~russian
						 * Примечания, стоящие внутри перечня значений
						 *
						 * @note Держатся ради обратной записи: примечание писано человеком,
						 *       и перезапись, его теряющая, обедняет файл настроек
						 *
						 * \~english
						 * Comments standing inside an array of the values
						 * @note They are kept for the sake of the writing back: a comment has been written by a human,
						 *       and a rewriting that loses it impoverishes the settings file
						 *
						 * \~
						 */
						vector <remark_t> remarks;
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
						Node() noexcept :
						 type(type_t::NONE), quoting(string_t::BASIC), radix(radix_t::DECIMAL),
						 boolean(false), multiline(false), integer(0), real(0.0) {}
					} node_t;
					/**
					 * \~russian
					 * @brief Запись разобранного текста настроек
					 *
					 * \~english
					 * @brief Record of the parsed settings text
					 *
					 * \~
					 */
					typedef struct Record {
						// Вид записи текста настроек
						kind_t kind;
						/**
						 * \~russian
						 * Место составного имени записи в перечне частей имён
						 *
						 * @note Отрезок задан смещением и количеством частей, а не длиной в
						 *       байтах: части имени хранятся своим перечнем
						 *
						 * \~english
						 * Place of the compound name of the record in the list of the parts of the names
						 * @note The segment is given by an offset and a number of the parts rather than by a length in
						 *       bytes: the parts of a name are stored in their own list
						 *
						 * \~
						 */
						span_t path;
						// Порядковый номер узла значения пары
						uint32_t node;
						// Порядковый номер записи объявления таблицы, которой запись принадлежит
						uint32_t table;
						// Место содержимого примечания в хранилище знаков
						span_t text;
						/**
						 * \~russian
						 * Признак примечания, дописанного к готовой строке
						 *
						 * @note Примечание это писано к своей записи и удаляется вместе с
						 *       нею: пояснение к тому, чего в файле нет, смысла не несёт
						 *
						 * \~english
						 * Flag of a comment appended to a ready line
						 * @note That comment has been written to its record and is removed together with
						 *       it: an explanation of what is not in the file carries no sense
						 *
						 * \~
						 */
						bool trailing;
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
						 kind(kind_t::NONE), node(0), table(0), trailing(false) {}
					} record_t;
				private:
					// Код ошибки последней операции
					mutable error_t _error;
				private:
					// Положение обнаруженной ошибки в исходном тексте
					location_t _errorLocation;
				private:
					// Общее хранилище знаков имён, значений и примечаний
					string _store;
				private:
					// Перечень записей разобранного текста в порядке их следования
					vector <record_t> _records;
				private:
					// Перечень составных частей имён записей и пар встроенных таблиц
					vector <key_t> _keys;
				private:
					// Перечень узлов значений текста настроек
					vector <node_t> _nodes;
				private:
					// Указатель записей по составному имени, приведённому к виду для сличения
					unordered_map <string, uint32_t> _index;
				private:
					/**
					 * \~russian
					 * Порядок объявления дочерних имён по каждому составному имени
					 *
					 * @details Держит для каждой таблицы перечень записей её дочерних имён в
					 * порядке следования их в тексте
					 *
					 * @note Без него выдача перечня имён обращалась бы в квадратичную:
					 *       перебор всех записей дерева ради каждой таблицы
					 *
					 * \~english
					 * Order of the declaration of the child names for every compound name
					 * @details Keeps for every table a list of the records of its child names in
					 * the order of their succession in the text
					 * @note Without it the issuance of the list of the names would turn into a quadratic one:
					 *       a traversal of all the records of the tree for the sake of every table
					 *
					 * \~
					 */
					unordered_map <string, vector <uint32_t>> _children;
					private:
					/**
					 * \~russian
					 * Имена таблиц, заведённых составным именем ключа
					 *
					 * @details Составное имя ключа «a.b = 1» заводит таблицу «a», собственного
					 * объявления не имеющую, и описание запрещает объявлять её заголовком:
					 * правка, такой заголовок заводящая, собрала бы текст, который разбор
					 * отверг бы повтором. Таблица, заведённая исподволь заголовком «[a.b]»,
					 * тем же заголовком «[a]» объявляется законно, и в перечень этот не идёт
					 *
					 * \~english
					 * Names of the tables created by a compound name of a key
					 * @details The compound name of a key «a.b = 1» creates the table «a», which has no declaration
					 * of its own, and the specification prohibits declaring it by a header:
					 * an editing that creates such a header would assemble a text which the parsing
					 * would reject as a repetition. A table created implicitly by the header «[a.b]»
					 * is declared lawfully by the same header «[a]», and it does not go into this list
					 *
					 * \~
					 */
					unordered_set <string> _dotted;
				private:
					/**
					 * \~russian
					 * Состояние обхода записей, удерживаемое между перестроениями указателей
					 *
					 * @details Перестроение обходит всё дерево, и правка, зовущая его на всякую
					 * заводимую запись, обращает сборку дерева правками в квадратичную: восемь
					 * тысяч правок стоили три секунды, и цена одной правки удваивалась с каждым
					 * удвоением их числа. Запись, встающая в конец перечня, порядковых номеров
					 * прежних записей не сдвигает, и указатели ей достаточно нарастить - для
					 * чего и держится состояние, с каким обход дошёл до конца дерева
					 *
					 * @note Разбор записи ведётся одним и тем же телом и при полном перестроении,
					 * и при доборном наращивании: два списка правил разошлись бы при первой же
					 * правке одного из них
					 *
					 * \~english
					 * State of the traversal of the records kept between the rebuildings of the indexes
					 * @details A rebuilding traverses the whole tree, and an editing calling it for every
					 * created record turns the assembly of a tree by the editings into a quadratic one: eight
					 * thousand editings used to cost three seconds, and the price of one editing doubled with
					 * every doubling of their number. A record placed at the end of the list does not shift the
					 * ordinal numbers of the previous records, and it is enough to grow the indexes for it —
					 * which is what the state the traversal reached the end of the tree with is kept for
					 * @note The parsing of a record is conducted by one and the same body both at a full
					 * rebuilding and at an incremental growing: two lists of the rules would diverge at the
					 * first editing of one of them
					 *
					 * \~
					 */
					typedef struct Traversal {
						/**
						 * \~russian
						 * Ключ указателя таблицы, которой принадлежат разбираемые записи
						 * \~english
						 * Key of the index of the table the records being parsed belong to
						 * \~
						 */
						string table;
						/**
						 * \~russian
						 * Количество таблиц, объявленных каждым набором таблиц
						 * \~english
						 * Number of the tables declared by every array of tables
						 * \~
						 */
						unordered_map <string, uint32_t> ordinals;
						/**
						 * \~russian
						 * Объявленные дочерние имена по каждому объемлющему имени
						 *
						 * @note Перечень дочерних имён держится порядком объявления, и сличать
						 * добавляемое имя перебором его значило бы обращать перестроение в
						 * квадратичное: текст из одних объявлений таблиц собирался вдевятеро
						 * медленнее текста с парами, и замер это показал сразу
						 *
						 * \~english
						 * Declared child names for every enclosing name
						 * @note The list of the child names is kept in the order of the declaration, and
						 * comparing an added name by a traversal of it would turn the rebuilding into a
						 * quadratic one: a text made of the declarations of the tables alone used to be
						 * assembled nine times slower than a text with the pairs, and a measurement showed it at once
						 *
						 * \~
						 */
						unordered_map <string, unordered_set <string>> declared;
						/**
						 * \~russian
						 * Подсказки места вставки записи по порядковому номеру таблицы
						 *
						 * @details Заведение пары ищет место за последней парой её таблицы обходом
						 * области таблицы, а область эта растёт с каждой заведённой парой: обход
						 * выходил квадратичным - шестнадцать тысяч правок стоили сто двадцать
						 * восемь миллионов шагов. Подсказка хранит место, найденное прошлым
						 * заведением, и обход продолжается от него, проходя лишь записи, с тех
						 * пор добавленные
						 *
						 * @note Подсказка есть подсказка, а не истина: правило поиска места
						 * остаётся одно и то же, и обход от подсказки даёт тот же итог, что и
						 * обход с начала области. Всякая правка, сдвигающая порядковые номера
						 * записей, подсказки отменяет вместе с перестроением указателей
						 *
						 * \~english
						 * Hints of the place of the insertion of a record by the ordinal number of a table
						 * @details The creation of a pair looks for a place after the last pair of its table by a
						 * traversal of the area of the table, and that area grows with every created pair: the
						 * traversal used to be quadratic — sixteen thousand editings cost one hundred twenty eight
						 * million steps. A hint keeps the place found by the previous creation, and the traversal
						 * continues from it, passing only the records added since then
						 * @note A hint is a hint rather than a truth: the rule of the search for a place remains one
						 * and the same, and a traversal from a hint gives the same result as a traversal from the
						 * beginning of the area. Every editing that shifts the ordinal numbers of the records
						 * cancels the hints together with the rebuilding of the indexes
						 *
						 * \~
						 */
						unordered_map <uint32_t, size_t> hints;
						/**
						 * \~russian
						 * @brief Метод сброса состояния обхода записей
						 * \~english
						 * @brief Method of the reset of the state of the traversal of the records
						 * \~
						 */
						void reset() noexcept {
							// Выполняем очистку ключа указателя таблицы
							this->table.clear();
							// Выполняем очистку количества таблиц наборов
							this->ordinals.clear();
							// Выполняем очистку объявленных дочерних имён
							this->declared.clear();
							// Выполняем очистку подсказок места вставки записи
							this->hints.clear();
						}
					} traversal_t;
				private:
					/**
					 * \~russian
					 * Состояние обхода, с каким перестроение дошло до конца дерева
					 * \~english
					 * State of the traversal the rebuilding reached the end of the tree with
					 * \~
					 */
					traversal_t _traversal;
				private:
					/**
					 * \~russian
					 * Количество записей и узлов, правкой в мусор обращённых
					 *
					 * @note Считается приблизительно: узел замещённого значения идёт за один,
					 *       сколько бы вложенных он ни нёс. Занижение уплотнение лишь
					 *       отдаляет, а рост всё равно остаётся ограниченным
					 *
					 * \~english
					 * Number of the records and the nodes turned into garbage by an editing
					 * @note Counted approximately: the node of a replaced value goes as one,
					 *       however many nested ones it may carry. An underestimation only postpones
					 *       the compaction, while the growth all the same remains limited
					 *
					 * \~
					 */
					size_t _garbage;
					/**
					 * \~russian
					 * Длина хранилища знаков, снятая последним уплотнением
					 *
					 * @note Держится ради правки строковых значений: содержимое дописывается
					 *       к хранилищу всякий раз, а прежнее остаётся в нём мусором, и
					 *       счётом записей рост этот не виден
					 *
					 * \~english
					 * Length of the storage of the characters taken at the last compaction
					 * @note It is kept for the sake of the editing of the string values: the content is appended
					 *       to the storage every time, while the previous one remains in it as garbage, and
					 *       by the count of the records that growth is not visible
					 *
					 * \~
					 */
					size_t _compacted;
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
					 * @brief Метод добавления составного имени к перечню частей имён
					 *
					 * @param path добавляемое составное имя
					 * @return     отрезок перечня частей с добавленным именем
					 *
					 * \~english
					 * @brief Method of adding a compound name to the list of the parts of the names
					 * @param path compound name being added
					 * @return     segment of the list of the parts with the added name
					 *
					 * \~
					 */
					span_t keep(const vector <part_t> & path) noexcept;
					/**
					 * \~russian
					 * @brief Метод добавления составного имени к перечню частей имён
					 *
					 * @param path добавляемое составное имя
					 * @return     отрезок перечня частей с добавленным именем
					 *
					 * \~english
					 * @brief Method of adding a compound name to the list of the parts of the names
					 * @param path compound name being added
					 * @return     segment of the list of the parts with the added name
					 *
					 * \~
					 */
					span_t keep(const vector <string_view> & path) noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки ключа указателя записей
					 *
					 * @details Части имени соединяются знаком, в имени встретиться не
					 * могущим: соединение точкой давало бы разночтение имён «a.b» и «"a.b"»
					 *
					 * @param buffer буфер, куда собирается ключ указателя
					 * @param path   отрезок перечня частей собираемого имени
					 * @param parent ключ указателя объемлющей таблицы
					 *
					 * \~english
					 * @brief Method of assembling the key of the index of the records
					 * @details The parts of the name are joined by a character that cannot be met in a
					 * name: a joining by a dot would give a discrepancy of the names «a.b» and «"a.b"»
					 * @param buffer buffer into which the key of the index is assembled
					 * @param path   segment of the list of the parts of the name being assembled
					 * @param parent key of the index of the enclosing table
					 *
					 * \~
					 */
					void labelled(string & buffer, const span_t & path, const string_view parent = "") const noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки ключа указателя записей
					 *
					 * @param buffer буфер, куда собирается ключ указателя
					 * @param path   составное имя записи
					 *
					 * \~english
					 * @brief Method of assembling the key of the index of the records
					 * @param buffer buffer into which the key of the index is assembled
					 * @param path   compound name of the record
					 *
					 * \~
					 */
					void labelled(string & buffer, const vector <string_view> & path) const noexcept;
					/**
					 * \~russian
					 * @brief Метод поиска записи по составному имени
					 *
					 * @param path составное имя искомой записи
					 * @param kind вид искомой записи
					 * @return     порядковый номер найденной записи либо количество записей
					 *
					 * \~english
					 * @brief Method of searching for a record by a compound name
					 * @param path compound name of the record being sought
					 * @param kind kind of the record being sought
					 * @return     ordinal number of the found record or the number of the records
					 *
					 * \~
					 */
					uint32_t locate(const vector <string_view> & path, const kind_t kind) const noexcept;
					/**
					 * \~russian
					 * @brief Метод перестроения указателей поиска
					 *
					 * @details Выполняется после всякой правки, меняющей состав записей:
					 * порядковые номера записей при вставке сдвигаются, и указатели, на них
					 * ссылающиеся, обесцениваются
					 *
					 * \~english
					 * @brief Method of rebuilding the indexes of the search
					 * @details Performed after every editing that changes the composition of the records:
					 * the ordinal numbers of the records are shifted at an insertion, and the indexes referring to
					 * them are invalidated
					 *
					 * \~
					 */
					void reindex() noexcept;
					/**
					 * \~russian
					 * @brief Метод внесения записи дерева в указатели поиска
					 *
					 * @note Тело это общее у полного перестроения и у доборного наращивания:
					 * порядок разбора записи задаётся в одном месте
					 *
					 * @param index порядковый номер вносимой записи дерева настроек
					 *
					 * \~english
					 * @brief Method of entering a record of the tree into the indexes of the search
					 * @note This body is common to the full rebuilding and to the incremental growing:
					 * the order of the parsing of a record is set in one place
					 * @param index ordinal number of the record of the tree of the settings being entered
					 *
					 * \~
					 */
					void absorb(const uint32_t index) noexcept;
					/**
					 * \~russian
					 * @brief Метод наращивания указателей записью, вставшей в конец перечня
					 *
					 * @details Запись, встающая в конец, порядковых номеров прежних записей не
					 * сдвигает: перестраивать указатели целиком незачем
					 *
					 * @note Уплотнение дерева при этом не исполняется: оно сдвигает номера всех
					 * записей и требует полного перестроения, а зовётся оно по накоплении мусора,
					 * которого заведение записи не создаёт
					 *
					 * \~english
					 * @brief Method of growing the indexes by a record placed at the end of the list
					 * @details A record placed at the end does not shift the ordinal numbers of the previous
					 * records: there is no point in rebuilding the indexes entirely
					 * @note The compaction of the tree is not performed at that: it shifts the numbers of all
					 * the records and requires a full rebuilding, while it is called upon the accumulation of
					 * the garbage, which the creation of a record does not produce
					 *
					 * \~
					 */
					void grow() noexcept;
					/**
					 * \~russian
					 * @brief Метод уплотнения дерева настроек
					 *
					 * @details Собирает записи, узлы, части имён и хранилище знаков заново,
					 * оставляя одно лишь живое: снятые пометкой удаления записи, узлы
					 * замещённых значений и содержимое, на которое никто не ссылается,
					 * выбрасываются
					 *
					 * @note Правка дерева мусор оставляет по устройству: запись снимается
					 * пометкой, а не изъятием, - иначе сдвинулись бы все, кто за нею, - и
					 * значение замещается новым узлом, ибо прежнее вправе нести вложенные.
					 * Без уплотнения дерево неограниченно росло бы при долгой правке:
					 * четыреста тысяч правок двух ключей отнимали сто тринадцать мегабайт
					 *
					 * \~english
					 * @brief Method of compacting the settings tree
					 * @details Assembles the records, the nodes, the parts of the names and the storage of the characters anew,
					 * leaving only the live: the records removed by a mark of a deletion, the nodes
					 * of the replaced values and the content which nobody refers to
					 * are thrown out
					 * @note An editing of the tree leaves the garbage by its arrangement: a record is removed by
					 * a mark rather than by an extraction — otherwise all the ones after it would be shifted — while
					 * a value is replaced by a new node, for the previous one has the right to carry the nested ones.
					 * Without a compaction the tree would grow without limit at a long editing:
					 * four hundred thousand editings of two keys took away a hundred and thirteen megabytes
					 *
					 * \~
					 */
					void compact() noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки дерева на засорённость мусором правки
					 *
					 * @return результат проверки
					 *
					 * \~english
					 * @brief Method of checking the tree for a littering with the garbage of the editing
					 * @return result of the check
					 *
					 * \~
					 */
					bool cluttered() const noexcept;
					/**
					 * \~russian
					 * @brief Метод переноса содержимого в уплотняемое хранилище знаков
					 *
					 * @param span  переносимый отрезок хранилища знаков
					 * @param store уплотняемое хранилище знаков
					 * @return      отрезок уплотнённого хранилища знаков
					 *
					 * \~english
					 * @brief Method of transferring a content into the storage of the characters being compacted
					 * @param span  segment of the storage of the characters being transferred
					 * @param store storage of the characters being compacted
					 * @return      segment of the compacted storage of the characters
					 *
					 * \~
					 */
					span_t relocate(const span_t & span, string & store) const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки хранилища знаков на вместимость содержимого
					 *
					 * @details Отрезок хранилища задан смещением и длиной в четыре октета:
					 * содержимое, за них выходящее, обрезало бы смещение молча, и дерево
					 * выдавало бы взятое не оттуда
					 *
					 * @param length длина добавляемого содержимого в октетах
					 * @return       результат проверки
					 *
					 * \~english
					 * @brief Method of checking the storage of the characters for the capacity of a content
					 * @details A segment of the storage is given by an offset and a length of four octets:
					 * a content going beyond them would truncate the offset silently, and the tree
					 * would issue what has been taken from elsewhere
					 * @param length length of the content being added in octets
					 * @return       result of the check
					 *
					 * \~
					 */
					bool spacious(const size_t length) noexcept;
					/**
					 * \~russian
					 * @brief Метод переноса составного имени в уплотняемый перечень частей
					 *
					 * @param span  переносимый отрезок перечня частей имён
					 * @param store уплотняемое хранилище знаков
					 * @param keys  уплотняемый перечень составных частей имён
					 * @return      отрезок уплотнённого перечня частей имён
					 *
					 * \~english
					 * @brief Method of transferring a compound name into the list of the parts being compacted
					 * @param span  segment of the list of the parts of the names being transferred
					 * @param store storage of the characters being compacted
					 * @param keys  list of the component parts of the names being compacted
					 * @return      segment of the compacted list of the parts of the names
					 *
					 * \~
					 */
					span_t relocate(const span_t & span, string & store, vector <key_t> & keys) const noexcept;
					/**
					 * \~russian
					 * @brief Метод переноса узла значения в уплотняемый перечень узлов
					 *
					 * @param node  порядковый номер переносимого узла значения
					 * @param store уплотняемое хранилище знаков
					 * @param keys  уплотняемый перечень составных частей имён
					 * @param nodes уплотняемый перечень узлов значений
					 * @return      порядковый номер узла в уплотнённом перечне
					 *
					 * \~english
					 * @brief Method of transferring a node of a value into the list of the nodes being compacted
					 * @param node  ordinal number of the node of the value being transferred
					 * @param store storage of the characters being compacted
					 * @param keys  list of the component parts of the names being compacted
					 * @param nodes list of the nodes of the values being compacted
					 * @return      ordinal number of the node in the compacted list
					 *
					 * \~
					 */
					uint32_t relocate(const uint32_t node, string & store, vector <key_t> & keys, vector <node_t> & nodes) const noexcept;
					/**
					 * \~russian
					 * @brief Метод заведения узла значения
					 *
					 * @return порядковый номер заведённого узла значения
					 *
					 * \~english
					 * @brief Method of creating a node of a value
					 * @return ordinal number of the created node of the value
					 *
					 * \~
					 */
					uint32_t reserve() noexcept;
					/**
					 * \~russian
					 * @brief Метод переноса значения разбора в узел дерева
					 *
					 * @param node  порядковый номер заполняемого узла значения
					 * @param value переносимое значение разбора
					 *
					 * \~english
					 * @brief Method of transferring a value of the parsing into a node of the tree
					 * @param node  ordinal number of the node of the value being filled
					 * @param value value of the parsing being transferred
					 *
					 * \~
					 */
					void assign(const uint32_t node, const content_t & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи узла значения собираемым текстом
					 *
					 * @param writer запись собираемого текста настроек
					 * @param node   порядковый номер записываемого узла значения
					 * @return       результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing a node of a value into the text being assembled
					 * @param writer writing of the settings text being assembled
					 * @param node   ordinal number of the node of the value being written
					 * @return       result of performing the operation
					 *
					 * \~
					 */
					bool compose(writer_t & writer, const uint32_t node) const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки составного имени на пригодность к записи
					 *
					 * @details Имя проверяется теми же правилами, какими его проверяет
					 * разбор: имя, записать которое обратно нельзя, отвергается в месте
					 * правки, а не при записи собранного дерева
					 *
					 * @note Проверяется и занятость имени: объявить таблицу поверх пары
					 *       либо завести пару поверх таблицы значило бы собрать текст,
					 *       который разбор отвергнет переопределением. Отвергается это в
					 *       месте правки, а не при записи собранного дерева
					 *
					 * @param path  проверяемое составное имя
					 * @param table признак проверки имени объявляемой таблицы
					 * @return      результат выполнения операции
					 *
					 * \~english
					 * @brief Method of checking a compound name for the suitability for the writing
					 * @details The name is checked by the same rules by which the parsing checks it:
					 * a name which cannot be written back is rejected at the place of the
					 * editing rather than at the writing of the assembled tree
					 * @note The occupancy of the name is checked as well: to declare a table on top of a pair
					 *       or to create a pair on top of a table would mean to assemble a text
					 *       which the parsing will reject as a redefinition. This is rejected at
					 *       the place of the editing rather than at the writing of the assembled tree
					 * @param path  compound name being checked
					 * @param table flag of the checking of the name of a table being declared
					 * @return      result of performing the operation
					 *
					 * \~
					 */
					bool acceptable(const vector <string_view> & path, const bool table) noexcept;
					/**
					 * \~russian
					 * @brief Метод получения узла значения по составному имени
					 *
					 * @param path составное имя искомой пары
					 * @return     узел значения найденной пары либо пустой указатель
					 *
					 * \~english
					 * @brief Method of getting the node of a value by a compound name
					 * @param path compound name of the pair being sought
					 * @return     node of the value of the found pair or an empty pointer
					 *
					 * \~
					 */
					const node_t * seek(const vector <string_view> & path) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения узла значения для правки
					 *
					 * @details Отыскивает пару по составному имени, а при её отсутствии
					 * заводит её в конце своей таблицы, объявляя недостающую таблицу
					 *
					 * @param path составное имя правимой пары
					 * @return     узел значения правимой пары либо пустой указатель
					 *
					 * \~english
					 * @brief Method of getting the node of a value for an editing
					 * @details Finds a pair by a compound name, and in its absence
					 * creates it at the end of its table, declaring the missing table
					 * @param path compound name of the pair being edited
					 * @return     node of the value of the pair being edited or an empty pointer
					 *
					 * \~
					 */
					node_t * reach(const vector <string_view> & path) noexcept;
					/**
					 * \~russian
					 * @brief Метод учёта узла значения и всего вложенного в него мусором правки
					 *
					 * @param node порядковый номер узла значения, в мусор обращаемого
					 *
					 * \~english
					 * @brief Method of the accounting of a node of a value and of everything nested in it as a garbage of an editing
					 * @param node ordinal number of the node of the value being turned into a garbage
					 *
					 * \~
					 */
					void discard(const uint32_t node) noexcept;
					/**
					 * \~russian
					 * @brief Метод получения порядкового номера узла значения по составному имени
					 *
					 * @note Выдаётся порядковый номер, а не указатель: правка перечня узлов
					 *       наращивает его, и указатель, выданный до того, повис бы
					 *
					 * @param path составное имя искомого места
					 * @return     порядковый номер найденного узла значения либо признак отсутствия
					 *
					 * \~english
					 * @brief Method of the obtaining of the ordinal number of a node of a value by a compound name
					 * @note An ordinal number rather than a pointer is issued: an editing of the list of the nodes
					 *       grows it, and a pointer issued before that would dangle
					 * @param path compound name of the place being sought
					 * @return     ordinal number of the found node of the value or the sign of the absence
					 *
					 * \~
					 */
					uint32_t dive(const vector <string_view> & path) noexcept;
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
					 * @details Разбирает текст целиком, сохраняя примечания, пустые строки и
					 * порядок записей. Прежнее содержимое дерева при этом освобождается
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
					 * @details Parses the text in full, preserving the comments, the empty lines and
					 * the order of the records. The previous content of the tree is thereby released
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
					 * @brief Метод получения кода ошибки последней операции
					 *
					 * @return код ошибки последней операции
					 *
					 * \~english
					 * @brief Method of getting the error code of the last operation
					 * @return error code of the last operation
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
					 * @brief Метод получения перечня объявленных таблиц
					 *
					 * @details Таблицы выдаются в порядке их объявления в исходном тексте, а
					 * составное имя каждой - перечнем частей
					 *
					 * @note Таблицы набора выдаются каждая своей записью, а имя у них одно на
					 * всех: порядкового номера имя таблицы набора не несёт по описанию.
					 * Обращаются к ним номером составной частью имени - «a», «0» - а сколько
					 * их, узнают счётом одинаковых имён в этом перечне
					 *
					 * @return перечень объявленных таблиц текста настроек
					 *
					 * \~english
					 * @brief Method of getting the list of the declared tables
					 * @details The tables are issued in the order of their declaration in the source text, while
					 * the compound name of each of them — as a list of the parts
					 * @note The tables of an array are issued each by its own record, while their name is one for
					 * all of them: the name of a table of an array does not carry an ordinal number by the specification.
					 * They are addressed by a number as a component part of the name — «a», «0» — while how many
					 * of them there are is learnt by a count of the identical names in this list
					 * @return list of the declared tables of the settings text
					 *
					 * \~
					 */
					vector <vector <string_view>> tables() const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки наличия таблицы
					 *
					 * @param path составное имя искомой таблицы
					 * @return     результат проверки
					 *
					 * \~english
					 * @brief Method of checking the presence of a table
					 * @param path compound name of the table being sought
					 * @return     result of the check
					 *
					 * \~
					 */
					bool table(const vector <string_view> & path) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения количества таблиц набора таблиц
					 *
					 * @param path составное имя искомого набора таблиц
					 * @return     количество таблиц набора таблиц
					 *
					 * \~english
					 * @brief Method of getting the number of the tables of an array of tables
					 * @param path compound name of the array of tables being sought
					 * @return     number of the tables of the array of tables
					 *
					 * \~
					 */
					size_t count(const vector <string_view> & path) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения перечня дочерних имён таблицы
					 *
					 * @details Имена выдаются в порядке их объявления. Пустое составное имя
					 * означает верхний уровень текста настроек
					 *
					 * @param path составное имя таблицы
					 * @return     перечень дочерних имён таблицы
					 *
					 * \~english
					 * @brief Method of getting the list of the child names of a table
					 * @details The names are issued in the order of their declaration. An empty compound name
					 * means the top level of the settings text
					 * @param path compound name of the table
					 * @return     list of the child names of the table
					 *
					 * \~
					 */
					vector <string_view> keys(const vector <string_view> & path = {}) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод проверки наличия пары
					 *
					 * @param path составное имя искомой пары
					 * @return     результат проверки
					 *
					 * \~english
					 * @brief Method of checking the presence of a pair
					 * @param path compound name of the pair being sought
					 * @return     result of the check
					 *
					 * \~
					 */
					bool has(const vector <string_view> & path) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения типа значения пары
					 *
					 * @param path составное имя искомой пары
					 * @return     тип значения найденной пары
					 *
					 * \~english
					 * @brief Method of getting the type of the value of a pair
					 * @param path compound name of the pair being sought
					 * @return     type of the value of the found pair
					 *
					 * \~
					 */
					type_t type(const vector <string_view> & path) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения значения пары
					 *
					 * @details Значение составное - перечень либо встроенная таблица -
					 * выдаётся типом своим, а содержимое его читается перечнем дочерних
					 * имён и обращением к ним по отдельности
					 *
					 * @param path   составное имя искомой пары
					 * @param result значение найденной пары
					 * @return       результат выполнения операции
					 *
					 * @warning Выданные виды живут до следующей ПРАВКИ дерева. Дерево выдаёт
					 *          содержимое видами в своё хранилище знаков, а правка его
					 *          наращивает и по мере накопления мусора уплотняет, хранилище
					 *          перемещая: вид, взятый до правки, повисает. Нужен вид дольше -
					 *          снимайте копию
					 *
					 * \~english
					 * @brief Method of getting the value of a pair
					 * @details A compound value — an array or an inline table —
					 * is issued by its type, while its content is read by a list of the child
					 * names and by addressing them separately
					 * @param path   compound name of the pair being sought
					 * @param result value of the found pair
					 * @return       result of performing the operation
					 *
					 * @warning The issued views live until the next EDITING of the tree. The tree issues
					 *          the content by the views into its storage of the characters, while an editing grows it
					 *          and compacts it as the garbage accumulates, relocating the storage: a view taken
					 *          before an editing dangles. Should a view be needed longer — take a copy
					 *
					 * \~
					 */
					bool get(const vector <string_view> & path, content_t & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения строкового значения пары
					 *
					 * @param path составное имя искомой пары
					 * @return     строковое значение найденной пары либо пустая последовательность
					 *
					 * @warning Выданные виды живут до следующей ПРАВКИ дерева. Дерево выдаёт
					 *          содержимое видами в своё хранилище знаков, а правка его
					 *          наращивает и по мере накопления мусора уплотняет, хранилище
					 *          перемещая: вид, взятый до правки, повисает. Нужен вид дольше -
					 *          снимайте копию
					 *
					 * \~english
					 * @brief Method of getting the string value of a pair
					 * @param path compound name of the pair being sought
					 * @return     string value of the found pair or an empty sequence
					 *
					 * @warning The issued views live until the next EDITING of the tree. The tree issues
					 *          the content by the views into its storage of the characters, while an editing grows it
					 *          and compacts it as the garbage accumulates, relocating the storage: a view taken
					 *          before an editing dangles. Should a view be needed longer — take a copy
					 *
					 * \~
					 */
					string_view text(const vector <string_view> & path) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения количества значений перечня
					 *
					 * @param path составное имя искомого перечня
					 * @return     количество значений перечня
					 *
					 * \~english
					 * @brief Method of getting the number of the values of an array
					 * @param path compound name of the array being sought
					 * @return     number of the values of the array
					 *
					 * \~
					 */
					size_t length(const vector <string_view> & path) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения значения перечня по порядковому номеру
					 *
					 * @param path   составное имя искомого перечня
					 * @param index  порядковый номер значения перечня
					 * @param result значение перечня
					 * @return       результат выполнения операции
					 *
					 * @warning Выданные виды живут до следующей ПРАВКИ дерева. Дерево выдаёт
					 *          содержимое видами в своё хранилище знаков, а правка его
					 *          наращивает и по мере накопления мусора уплотняет, хранилище
					 *          перемещая: вид, взятый до правки, повисает. Нужен вид дольше -
					 *          снимайте копию
					 *
					 * \~english
					 * @brief Method of getting a value of an array by an ordinal number
					 * @param path   compound name of the array being sought
					 * @param index  ordinal number of the value of the array
					 * @param result value of the array
					 * @return       result of performing the operation
					 *
					 * @warning The issued views live until the next EDITING of the tree. The tree issues
					 *          the content by the views into its storage of the characters, while an editing grows it
					 *          and compacts it as the garbage accumulates, relocating the storage: a view taken
					 *          before an editing dangles. Should a view be needed longer — take a copy
					 *
					 * \~
					 */
					bool item(const vector <string_view> & path, const size_t index, content_t & result) const noexcept;
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
					 * @brief Метод получения значения пары числом
					 *
					 * @details Отказом получение завершается лишь тогда, когда значение
					 * числом не является вовсе. Вид хранения извлечению не указ: целое
					 * выдаётся и дробным видом, а дробное - и целым
					 *
					 * @details Дробное, извлекаемое целым видом, округляется по правилам
					 * математики с уводом половины от нуля: `1.5` выдаётся двойкой, а `-1.5` -
					 * минус двойкой. Целое, за отрезок затребованного вида выходящее,
					 * переносится младшими разрядами, а дробное вне его пределов выдаётся
					 * пределом: приведение такое стандарт зовёт неопределённым поведением,
					 * а неопределённого поведения в кодеке не будет
					 *
					 * @note Прежде вид значения соблюдался, и приведение одного к другому
					 *       выполнялось лишь тогда, когда оно значения не искажало. Отменено
					 *       владельцем 20.08.2026: договор извлечения общий у всех кодеков
					 *       рамки, а приведение языка не отказывает нигде
					 *
					 * @param result ссылка на результат разбора
					 * @param path   составное имя искомой пары
					 * @return       признак успешного разбора
					 *
					 * \~english
					 * @brief Method of getting the value of a pair as a number
					 * @details The receipt ends with a refusal only when the value is not a number
					 * at all. The kind of the storage is not a directive to the extraction: an integer is issued
					 * also as a fractional kind, and a fractional one — also as an integer
					 * @details A fractional number extracted as an integer kind is rounded by the rules
					 * of mathematics with a half taken away from zero: `1.5` is issued as a two, while `-1.5` —
					 * as a minus two. An integer going beyond the range of the requested kind is carried over
					 * by the lower bits, while a fractional one beyond its limits is issued as the limit: such a
					 * conversion is called an undefined behaviour by the standard, and there will be no
					 * undefined behaviour in the codec
					 * @param result reference to the result of the parsing
					 * @param path   compound name of the pair being sought
					 * @return       flag of a successful parsing
					 *
					 * \~
					 */
					bool value(T & result, const vector <string_view> & path) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод объявления таблицы
					 *
					 * @details Уже объявленная таблица повторно не объявляется: метод
					 * отвечает успехом, оставляя дерево нетронутым
					 *
					 * @param path составное имя объявляемой таблицы
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of declaring a table
					 * @details An already declared table is not declared again: the method
					 * answers with success, leaving the tree untouched
					 * @param path compound name of the table being declared
					 * @return     result of performing the operation
					 *
					 * \~
					 */
					bool create(const vector <string_view> & path) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки строкового значения пары
					 *
					 * @param path    составное имя устанавливаемой пары
					 * @param value   устанавливаемое значение пары
					 * @param quoting запись строкового значения
					 * @return        результат выполнения операции
					 *
					 * \~english
					 * @brief Method of setting the string value of a pair
					 * @param path    compound name of the pair being set
					 * @param value   value of the pair being set
					 * @param quoting notation of the string value
					 * @return        result of performing the operation
					 *
					 * \~
					 */
					bool set(const vector <string_view> & path, const string_view value, const string_t quoting = string_t::BASIC) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки строкового значения пары
					 *
					 * @details Вид этот заведён ради последовательности знаков, записанной
					 * прямо в месте вызова: без него «set(path, "текст")» ушло бы установкой
					 * логического значения - язык предпочитает приведение указателя к
					 * логическому типу построению последовательности знаков
					 *
					 * @param path    составное имя устанавливаемой пары
					 * @param value   устанавливаемое значение пары
					 * @param quoting запись строкового значения
					 * @return        результат выполнения операции
					 *
					 * \~english
					 * @brief Method of setting the string value of a pair
					 * @details This overload has been introduced for the sake of a sequence of characters written
					 * right at the place of the call: without it «set(path, "text")» would go as a setting
					 * of a logical value — the language prefers a conversion of a pointer to
					 * the logical type over a construction of a sequence of characters
					 * @param path    compound name of the pair being set
					 * @param value   value of the pair being set
					 * @param quoting notation of the string value
					 * @return        result of performing the operation
					 *
					 * \~
					 */
					bool set(const vector <string_view> & path, const char * value, const string_t quoting = string_t::BASIC) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки логического значения пары
					 *
					 * @param path  составное имя устанавливаемой пары
					 * @param value устанавливаемое значение пары
					 * @return      результат выполнения операции
					 *
					 * \~english
					 * @brief Method of setting the logical value of a pair
					 * @param path  compound name of the pair being set
					 * @param value value of the pair being set
					 * @return      result of performing the operation
					 *
					 * \~
					 */
					bool set(const vector <string_view> & path, const bool value) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки целого числа значением пары
					 *
					 * @param path  составное имя устанавливаемой пары
					 * @param value устанавливаемое значение пары
					 * @param radix система счисления записи числа
					 * @return      результат выполнения операции
					 *
					 * \~english
					 * @brief Method of setting an integer as the value of a pair
					 * @param path  compound name of the pair being set
					 * @param value value of the pair being set
					 * @param radix numeral system of the notation of the number
					 * @return      result of performing the operation
					 *
					 * \~
					 */
					bool set(const vector <string_view> & path, const int64_t value, const radix_t radix = radix_t::DECIMAL) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки числа с плавающей точкой значением пары
					 *
					 * @param path  составное имя устанавливаемой пары
					 * @param value устанавливаемое значение пары
					 * @return      результат выполнения операции
					 *
					 * \~english
					 * @brief Method of setting a floating-point number as the value of a pair
					 * @param path  compound name of the pair being set
					 * @param value value of the pair being set
					 * @return      result of performing the operation
					 *
					 * \~
					 */
					bool set(const vector <string_view> & path, const double value) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки отметки времени значением пары
					 *
					 * @param path  составное имя устанавливаемой пары
					 * @param value устанавливаемая отметка времени
					 * @param type  тип устанавливаемой отметки времени
					 * @return      результат выполнения операции
					 *
					 * \~english
					 * @brief Method of setting a timestamp as the value of a pair
					 * @param path  compound name of the pair being set
					 * @param value timestamp being set
					 * @param type  type of the timestamp being set
					 * @return      result of performing the operation
					 *
					 * \~
					 */
					bool set(const vector <string_view> & path, const stamp_t & value, const type_t type) noexcept;
					/**
					 * \~russian
					 * @brief Метод объявления составного значения пары
					 *
					 * @details Место объявления задаётся тем же построением, каким задаёт его
					 * поиск значения: начало имени именует пару, а остаток ведёт внутрь
					 * составного значения - порядковым номером в перечень, именем ключа во
					 * встроенную таблицу. Объявленное значение пусто, а наполняется оно
					 * вызовами @c push() и @c put()
					 *
					 * @note Прежнее содержимое места объявлением снимается: объявить перечень
					 *       поверх перечня значит завести его заново, а не дописать к нему
					 *
					 * @param path  составное имя объявляемого места
					 * @param table признак объявления встроенной таблицы заместо перечня
					 * @return      результат выполнения операции
					 *
					 * \~english
					 * @brief Method of the declaring of a compound value of a pair
					 * @details The place of the declaring is given by the same construction by which the search
					 * of a value gives it: the beginning of the name names a pair, while the remainder leads inside
					 * a compound value — by an ordinal number into an array, by a name of a key into
					 * an inline table. The declared value is empty and is filled by
					 * the calls @c push() and @c put()
					 * @note The previous content of the place is removed by the declaring: to declare an array
					 *       on top of an array means to establish it anew rather than to append to it
					 * @param path  compound name of the place being declared
					 * @param table flag of the declaring of an inline table instead of an array
					 * @return      result of performing the operation
					 *
					 * \~
					 */
					bool arrange(const vector <string_view> & path, const bool table = false) noexcept;
					/**
					 * \~russian
					 * @brief Метод добавления значения к перечню
					 *
					 * @details Значение задаётся тем же построением, каким выдаёт его чтение
					 * значения перечня: тип его задан полем @c type, а вложенному перечню и
					 * встроенной таблице отвечает значение пустое, наполняемое вызовами по
					 * составному имени с порядковым номером его на конце
					 *
					 * @param path  составное имя перечня
					 * @param value добавляемое значение перечня
					 * @return      результат выполнения операции
					 *
					 * \~english
					 * @brief Method of the appending of a value to an array
					 * @details The value is given by the same construction by which the reading of a value
					 * of an array issues it: its type is given by the field @c type, while an empty value
					 * corresponds to a nested array and to an inline table, filled by the calls by
					 * a compound name with its ordinal number at the end
					 * @param path  compound name of the array
					 * @param value value being appended to the array
					 * @return      result of performing the operation
					 *
					 * \~
					 */
					bool push(const vector <string_view> & path, const content_t & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод добавления пары к встроенной таблице
					 *
					 * @note Имя пары составное: запись «{a.b = 1}» описанием дозволена, и имя
					 *       такой пары есть перечень частей его
					 *
					 * @param path  составное имя встроенной таблицы
					 * @param name  составное имя добавляемой пары
					 * @param value значение добавляемой пары
					 * @return      результат выполнения операции
					 *
					 * \~english
					 * @brief Method of the appending of a pair to an inline table
					 * @note The name of a pair is compound: the record «{a.b = 1}» is allowed by the specification, and the name
					 *       of such a pair is a list of its parts
					 * @param path  compound name of the inline table
					 * @param name  compound name of the pair being appended
					 * @param value value of the pair being appended
					 * @return      result of performing the operation
					 *
					 * \~
					 */
					bool put(const vector <string_view> & path, const vector <string_view> & name, const content_t & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод удаления пары
					 *
					 * @details Пара удаляется вместе с примечанием, дописанным к её строке
					 *
					 * @param path составное имя удаляемой пары
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of removing a pair
					 * @details The pair is removed together with the comment appended to its line
					 * @param path compound name of the pair being removed
					 * @return     result of performing the operation
					 *
					 * \~
					 */
					bool erase(const vector <string_view> & path) noexcept;
					/**
					 * \~russian
					 * @brief Метод удаления таблицы
					 *
					 * @details Удаляется объявление таблицы вместе со всеми её парами
					 *
					 * @param path составное имя удаляемой таблицы
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of removing a table
					 * @details The declaration of the table is removed together with all its pairs
					 * @param path compound name of the table being removed
					 * @return     result of performing the operation
					 *
					 * \~
					 */
					bool remove(const vector <string_view> & path) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения количества объявленных таблиц
					 *
					 * @return количество объявленных таблиц текста настроек
					 *
					 * \~english
					 * @brief Method of getting the number of the declared tables
					 * @return number of the declared tables of the settings text
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
					 * @brief Метод получения объёма памяти, деревом занимаемого
					 *
					 * @details Считается вместе с памятью, взятой хранилищами про запас:
					 * показатель этот назначен потребителю, который дерево долго правит, и
					 * судить о занятом по одному лишь живому содержимому ему мало
					 *
					 * @note Мусор, правкою накопленный, дерево уплотняет само по накоплении
					 * его вровень с живым: объём этот растёт до уплотнения и опадает после,
					 * но неограниченным не бывает
					 *
					 * @return объём памяти в октетах
					 *
					 * \~english
					 * @brief Method of getting the volume of the memory occupied by the tree
					 * @details Counted together with the memory taken by the storages in reserve: this
					 * indicator is intended for a consumer who edits the tree for a long time, and
					 * to judge the occupied by the live content alone is not enough for him
					 * @note The garbage accumulated by the editing the tree compacts itself upon its accumulation
					 * on a par with the live one: this volume grows before a compaction and falls after it,
					 * but is never unlimited
					 * @return volume of the memory in octets
					 *
					 * \~
					 */
					size_t footprint() const noexcept;
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
					 * @param settings настройки записи текста настроек
					 * @return         собранный текст настроек
					 *
					 * \~english
					 * @brief Method of writing the tree back into a settings text
					 * @details The records are issued in the same order in which they have been read,
					 * together with the comments and the empty lines
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
					 * @details Настройки записи выводятся из настроек разбора: пределы, каким
					 * текст прочитан, теми же и записывается
					 *
					 * @return собранный текст настроек
					 *
					 * \~english
					 * @brief Method of writing the tree back into a settings text
					 * @details The settings of the writing are inferred from the settings of the parsing: the limits by which
					 * the text has been read are the ones it is written by as well
					 * @return assembled settings text
					 *
					 * \~
					 */
					string text() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения настроек записи, отвечающих настройкам разбора
					 *
					 * @details Выводит пределы записи из пределов разбора и признание знаков
					 * Юникода в имени без кавычек. Прочитанное этими настройками записывается
					 * так, что читается обратно без потерь
					 *
					 * @return настройки записи текста настроек
					 *
					 * \~english
					 * @brief Method of getting the settings of the writing corresponding to the settings of the parsing
					 * @details Infers the limits of the writing from the limits of the parsing and the recognition of the Unicode
					 * characters in a name without quotes. What has been read by those settings is written
					 * so that it is read back without losses
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

#endif // __AWH_CODEC_TOML_DOCUMENT__
