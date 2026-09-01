/**
 * @file parser.hpp
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
 * @brief Заголовочный файл синтаксического разбора регулярных выражений — класс Parser,
 *        преобразующий текст регулярного выражения синтаксиса PCRE в синтаксическое дерево,
 *        размещаемое в арене узлов, без выделения памяти на каждый узел и без выбрасывания исключений
 *
 * \~english
 * @brief Header file of the syntax parsing of regular expressions — the Parser class,
 *        which converts the text of a regular expression of the PCRE syntax into a syntax tree
 *        placed in the node arena, without allocating memory for every node and without throwing exceptions
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_REGEX_PARSER__
#define __AWH_REGEX_PARSER__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstdint>
#include <string_view>
#include <unordered_set>
#include <unordered_map>

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
		 * @brief Наибольшая допустимая глубина вложенности групп регулярного выражения
		 *
		 * \~english
		 * @brief Largest admissible nesting depth of the groups of a regular expression
		 *
		 * \~
		 */
		constexpr uint32_t MAX_NESTING = 250;

		/**
		 * \~russian
		 * @brief Наибольшее допустимое значение границы кванта повторения
		 *
		 * \~english
		 * @brief Largest admissible value of a repetition quantifier bound
		 *
		 * \~
		 */
		constexpr uint32_t MAX_REPEAT = 65535;

		/**
		 * \~russian
		 * @brief Класс синтаксического разбора регулярного выражения
		 *
		 * @details Класс выполняет разбор текста регулярного выражения синтаксиса PCRE
		 *          и формирует синтаксическое дерево в арене узлов. Узлы дерева связываются
		 *          индексами, тексты классов символов, имён групп и последовательностей
		 *          символов размещаются в отдельных хранилищах. Разбор не выбрасывает
		 *          исключений: признаком отказа является код ошибки, доступный после разбора.
		 *
		 * \~english
		 * @brief Class of the syntax parsing of a regular expression
		 * @details The class parses the text of a regular expression of the PCRE syntax
		 *          and builds a syntax tree in the node arena. The nodes of the tree are linked
		 *          by indices, the texts of the character classes, of the group names and of the character
		 *          sequences are placed in separate storages. Parsing throws no
		 *          exceptions: the indication of a failure is the error code available after parsing.
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Parser {
			private:
				/**
				 * \~russian
				 * @brief Отложенная ссылка на именованную группу
				 *
				 * @details Ссылка на именованную группу может встретиться в тексте
				 *          регулярного выражения раньше объявления самой группы, поэтому
				 *          разрешение таких ссылок выполняется после завершения разбора.
				 *
				 * \~english
				 * @brief Deferred reference to a named group
				 * @details A reference to a named group may occur in the text of
				 *          a regular expression before the declaration of the group itself, therefore
				 *          resolution of such references is performed after parsing has finished.
				 *
				 * \~
				 */
				typedef struct Deferred {
					// Индекс узла ссылки в арене узлов
					node_id_t node;
					// Индекс имени группы в хранилище имён
					uint32_t name;
					// Смещение ссылки в тексте регулярного выражения
					uint32_t offset;
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
					Deferred() noexcept : node(INVALID_NODE), name(0), offset(0) {}
				} deferred_t;
			private:
				// Текст разбираемого регулярного выражения
				string_view _pattern;
			private:
				// Текущая позиция разбора в тексте регулярного выражения
				size_t _pos;
			private:
				// Индекс корневого узла синтаксического дерева
				node_id_t _root;
			private:
				// Текущая глубина вложенности групп
				uint32_t _depth;
			private:
				// Количество захватывающих групп, обнаруженных к текущей позиции разбора
				uint32_t _captures;
			private:
				// Общее количество захватывающих групп, определённое предварительным проходом
				uint32_t _total;
			private:
				/**
				 * \~russian
				 * Пределы шагов сопоставления и глубины вызовов, выражением заданные
				 *
				 * @details Пределы задаются указаниями «(*LIMIT_MATCH=N)»
				 *          и «(*LIMIT_DEPTH=N)» в начале выражения, а нуль означает,
				 *          что выражение предела своего не задаёт вовсе.
				 *
				 * \~english
				 * Limits of the matching steps and of the call depth set by the expression
				 * @details The limits are set by the «(*LIMIT_MATCH=N)» and «(*LIMIT_DEPTH=N)»
				 *          options at the start of an expression, whereas zero means
				 *          that the expression sets no limit of its own at all.
				 *
				 * \~
				 */
				uint32_t _stepLimit;
				uint32_t _depthLimit;
				uint32_t _heapLimit;
			private:
				/**
				 * \~russian
				 * Соглашение о переводе строки и охват последовательности «\\R»
				 *
				 * @details Соглашение задаётся указанием вида «(*CRLF)» в начале
				 *          выражения и правит точкой и привязками, а охват «\\R»
				 *          правится указаниями «(*BSR_ANYCRLF)» и «(*BSR_UNICODE)»
				 *          отдельно от соглашения.
				 *
				 * \~english
				 * Newline convention and the coverage of the «\\R» sequence
				 * @details The convention is set by an option of the «(*CRLF)» kind at the start
				 *          of an expression and governs the dot and the anchors, whereas the coverage of «\\R»
				 *          is governed by the «(*BSR_ANYCRLF)» and «(*BSR_UNICODE)» options
				 *          separately from the convention.
				 *
				 * \~
				 */
				newline_t _convention;
				bool _restricted;
			private:
				/**
				 * \~russian
				 * Хранилище имён отметок глаголов управления
				 *
				 * @details Имена всех отметок выражения лежат сплошным набором
				 *          октетов, а узел глагола ссылается на участок его
				 *          смещением и длиною наравне с классами символов.
				 *
				 * \~english
				 * Storage of the names of the marks of the control verbs
				 * @details The names of all the marks of an expression lie in a contiguous set
				 *          of octets, whereas the node of a verb refers to a span of it
				 *          by an offset and a length, along with the character classes.
				 *
				 * \~
				 */
				vector <uint8_t> _markers;
			private:
				// Набор режимов компиляции, действующих в текущей позиции разбора
				uint32_t _flags;
			private:
				// Исходный набор режимов компиляции, переданный при разборе
				uint32_t _options;
			private:
				// Текущая глубина вложенности проверок окружения
				uint32_t _look;
			private:
				// Код ошибки последней операции разбора
				error_t _error;
			private:
				// Смещение ошибки в тексте регулярного выражения
				size_t _errorPos;
			private:
				// Арена узлов синтаксического дерева
				vector <node_data_t> _nodes;
			private:
				/**
				 * \~russian
				 * Сберегательный ряд составов узлов, разбором накапливаемых
				 *
				 * @details Разбор последовательности и разбор выбора ветвей
				 *          накапливают состав узла прежде, чем связать его:
				 *          число элементов заранее не известно. Ряд для того
				 *          заводился на каждый вызов, а вызовы эти приходятся
				 *          на всякую группу и всякую ветвь выражения - и
                 *          всякий раз размещался заново.
				 *
				 *          Ряд ныне общий, а вложенность держится отметкою
				 *          основания: вызов помнит длину ряда при входе,
				 *          накапливает поверх неё и усекает ряд обратно,
				 *          связав состав. Место, однажды отведённое,
				 *          переживает и вызовы, и разборы.
				 *
				 * \~english
				 * Scratch sequence of the node contents accumulated by the parsing
				 * @details Parsing a concatenation and parsing an alternation
				 *          accumulate the content of a node before linking it:
				 *          the number of elements is not known in advance.
				 *
				 * \~
				 */
				vector <node_id_t> _items;
			private:
				// Хранилище классов символов
				vector <class_t> _classes;
			private:
				// Хранилище имён именованных групп
				vector <string> _names;
			private:
				// Хранилище последовательностей символов узлов типа «STRING»
				vector <uint32_t> _strings;
			private:
				/**
				 * \~russian
				 * @brief Признак разрыва связи квантора встроенными настройками
				 *
				 * @details Встроенные настройки символов не сопоставляют и элемента
				 *          выражения не образуют, однако связь квантора повторения
				 *          с элементом предшествующим разрывают: квантор за ними
				 *          считается ошибкой. Прочие же элементы пустые - примечание,
				 *          завершение экранирования и дословная последовательность
				 *          пустая - остаются прозрачными, и квантор за ними
				 *          применяется к элементу, им предшествующему. Признак
				 *          выставляется разбором настроек и снимается разбором
				 *          последовательности элементов.
				 *
				 * \~english
				 * @brief Flag of the quantifier binding break by inline options
				 * @details Inline character options match nothing and do not form
				 *          an item of the expression, yet they break the binding
				 *          of a repetition quantifier to the preceding item: a quantifier
				 *          after them is treated as an error. Other empty items —
				 *          a comment, the end of quoting and an empty quoted sequence —
				 *          stay transparent, and a quantifier after them applies
				 *          to the item preceding them. The flag is set by the parsing
				 *          of options and cleared by the parsing of an item sequence.
				 *
				 * \~
				 */
				bool _barrier;
			private:
				// Набор отложенных ссылок на именованные группы
				vector <deferred_t> _deferred;
			private:
				/**
				 * \~russian
				 * Набор ретроспективных проверок, длину коих даёт вызов подпрограммы
				 *
				 * @details Длина вызова берётся длиною вызываемой группы, а группа
				 *          вправе объявляться и позже проверки: длина таких проверок
				 *          вычисляется по завершении разбора наравне со ссылками
				 *          отложенными. Хранится индекс узла проверки и смещение
				 *          её в выражении - им отказ и выводится.
				 *
				 * \~english
				 * Set of lookbehinds whose length is given by a subroutine call
				 * @details The length of a call is taken as the length of the called group,
				 *          and the group may be declared later than the lookbehind: the length
				 *          of such lookbehinds is computed after parsing completes, on a par with
				 *          deferred references. The node index of the lookbehind and its offset
				 *          in the pattern are stored — the refusal is reported with it.
				 *
				 * \~
				 */
				vector <pair <node_id_t, size_t>> _behinds;
			private:
				// Набор номеров групп, длина каких вычисляется ныне
				mutable unordered_set <uint32_t> _calling;
			private:
				// Признак вычисления длины с разрешением вызовов подпрограмм
				bool _resolving;
			private:
				// Соответствие имён групп их номерам
				unordered_map <string, vector <uint32_t>> _groups;
			public:
				/**
				 * \~russian
				 * @brief Метод разбора регулярного выражения
				 *
				 * @details Результат предыдущего разбора сбрасывается перед началом работы.
				 *          При отказе разбора состояние объекта сохраняет код ошибки и
				 *          смещение, в котором ошибка обнаружена.
				 *
				 * @param pattern текст регулярного выражения для разбора
				 * @param flags   набор режимов компиляции регулярного выражения
				 * @return        результат выполнения разбора
				 *
				 * \~english
				 * @brief Method of parsing a regular expression
				 * @details The result of the previous parsing is reset before the work begins.
				 *          On a parsing failure the state of the object keeps the error code and
				 *          the offset at which the error was found.
				 * @param pattern text of the regular expression to parse
				 * @param flags   set of compilation modes of the regular expression
				 * @return        result of performing the parsing
				 *
				 * \~
				 */
				bool parse(string_view pattern, const uint32_t flags = 0) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод сброса результатов разбора
				 *
				 * \~english
				 * @brief Method of resetting the parsing results
				 *
				 * \~
				 */
				void reset() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения кода ошибки разбора
				 *
				 * @return код ошибки последней операции разбора
				 *
				 * \~english
				 * @brief Method of getting the parsing error code
				 * @return error code of the last parsing operation
				 *
				 * \~
				 */
				error_t error() const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения смещения ошибки разбора
				 *
				 * @return смещение ошибки в тексте регулярного выражения
				 *
				 * \~english
				 * @brief Method of getting the offset of the parsing error
				 * @return offset of the error in the text of the regular expression
				 *
				 * \~
				 */
				size_t errorPos() const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения текста ошибки разбора
				 *
				 * @return текст ошибки последней операции разбора
				 *
				 * \~english
				 * @brief Method of getting the text of the parsing error
				 * @return text of the error of the last parsing operation
				 *
				 * \~
				 */
				string message() const noexcept;
				/**
				 * \~russian
				 * @brief Метод толкования кода ошибки текстом
				 *
				 * @details Толкование заведено отдельно от message(): код ошибки
				 *          сопоставления держит движок, а не разборщик, и толковать
				 *          его текстом надлежит тем же порядком, что и код разбора.
				 *
				 * @param error толкуемый код ошибки
				 * @return      текст толкуемого кода ошибки
				 *
				 * \~english
				 * @brief Method of interpreting an error code as text
				 * @details The interpretation is provided separately from message():
				 *          the code of a matching error is held by the engine rather
				 *          than by the parser, and it is to be interpreted as text
				 *          in the same order as the code of the parsing.
				 * @param error interpreted error code
				 * @return      text of the interpreted error code
				 *
				 * \~
				 */
				static string message(const error_t error) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения индекса корневого узла синтаксического дерева
				 *
				 * @return индекс корневого узла синтаксического дерева
				 *
				 * \~english
				 * @brief Method of getting the index of the root node of the syntax tree
				 * @return index of the root node of the syntax tree
				 *
				 * \~
				 */
				node_id_t root() const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения количества захватывающих групп
				 *
				 * @return количество захватывающих групп регулярного выражения
				 *
				 * \~english
				 * @brief Method of getting the number of capturing groups
				 * @return number of capturing groups of the regular expression
				 *
				 * \~
				 */
				uint32_t captures() const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения предела шагов сопоставления выражения
				 *
				 * @return предел шагов сопоставления либо нуль при его отсутствии
				 *
				 * \~english
				 * @brief Method of getting the limit of the matching steps of the expression
				 * @return limit of the matching steps or zero if it is absent
				 *
				 * \~
				 */
				uint32_t steps() const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения предела глубины рекурсивных вызовов выражения
				 *
				 * @return предел глубины рекурсивных вызовов либо нуль при его отсутствии
				 *
				 * \~english
				 * @brief Method of getting the limit of the recursive call depth of the expression
				 * @return limit of the recursive call depth or zero if it is absent
				 *
				 * \~
				 */
				uint32_t depth() const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения предела объёма памяти сопоставления выражения
				 *
				 * @return предел объёма памяти в килобайтах либо предельное значение при его отсутствии
				 *
				 * \~english
				 * @brief Method of getting the limit of the matching memory of the expression
				 * @return limit of the memory in kibibytes or the largest value of the type if it is absent
				 *
				 * \~
				 */
				uint32_t heap() const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения соглашения о переводе строки выражения
				 *
				 * @return соглашение о переводе строки выражения
				 *
				 * \~english
				 * @brief Method of getting the newline convention of the expression
				 * @return newline convention of the expression
				 *
				 * \~
				 */
				newline_t newline() const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения хранилища имён отметок глаголов управления
				 *
				 * @return хранилище имён отметок глаголов управления
				 *
				 * \~english
				 * @brief Method of getting the storage of the names of the marks of the control verbs
				 * @return storage of the names of the marks of the control verbs
				 *
				 * \~
				 */
				const vector <uint8_t> & markers() const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения исходного набора режимов компиляции
				 *
				 * @details Набор режимов соответствует переданному при разборе и не
				 *          учитывает встроенных опций выражения, действие которых
				 *          сохраняется в каждом узле синтаксического дерева отдельно.
				 *
				 * @return исходный набор режимов компиляции регулярного выражения
				 *
				 * \~english
				 * @brief Method of getting the original set of compilation modes
				 * @details The set of modes matches the one passed at parsing and does not
				 *          take into account the inline options of the expression, whose effect
				 *          is kept in every node of the syntax tree separately.
				 * @return original set of compilation modes of the regular expression
				 *
				 * \~
				 */
				uint32_t options() const noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки возможности пустого сопоставления узла
				 *
				 * @details Узел допускает пустое сопоставление, если наименьшая длина
				 *          сопоставляемой им последовательности равна нулю.
				 *
				 * @param id индекс узла в арене узлов
				 * @return   результат проверки возможности пустого сопоставления
				 *
				 * \~english
				 * @brief Method of checking the possibility of an empty match of a node
				 * @details A node admits an empty match if the smallest length
				 *          of the sequence it matches equals zero.
				 * @param id index of the node in the node arena
				 * @return   result of checking the possibility of an empty match
				 *
				 * \~
				 */
				bool nullable(const node_id_t id) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения узла синтаксического дерева
				 *
				 * @param id индекс узла в арене узлов
				 * @return   узел синтаксического дерева
				 *
				 * \~english
				 * @brief Method of getting a node of the syntax tree
				 * @param id index of the node in the node arena
				 * @return   node of the syntax tree
				 *
				 * \~
				 */
				const node_data_t & node(const node_id_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения арены узлов синтаксического дерева
				 *
				 * @return арена узлов синтаксического дерева
				 *
				 * \~english
				 * @brief Method of getting the node arena of the syntax tree
				 * @return node arena of the syntax tree
				 *
				 * \~
				 */
				const vector <node_data_t> & nodes() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения класса символов
				 *
				 * @param index индекс класса символов в хранилище классов
				 * @return      класс символов регулярного выражения
				 *
				 * \~english
				 * @brief Method of getting a character class
				 * @param index index of the character class in the class storage
				 * @return      character class of the regular expression
				 *
				 * \~
				 */
				const class_t & charClass(const uint32_t index) const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения имени именованной группы
				 *
				 * @param index индекс имени в хранилище имён
				 * @return      имя именованной группы
				 *
				 * \~english
				 * @brief Method of getting the name of a named group
				 * @param index index of the name in the name storage
				 * @return      name of the named group
				 *
				 * \~
				 */
				const string & name(const uint32_t index) const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения соответствия имён групп их номерам
				 *
				 * @details Соответствие содержит имена всех именованных групп выражения.
				 *          В режиме «DUPNAMES» одно имя объявляется несколькими группами,
				 *          поэтому имени отвечает набор их номеров в порядке объявления.
				 *
				 * @return соответствие имён именованных групп наборам их номеров
				 *
				 * \~english
				 * @brief Method of getting the mapping of the group names to their numbers
				 * @details The mapping holds the names of all the named groups of the expression.
				 *          In the «DUPNAMES» mode one name is declared by several groups,
				 *          therefore a name corresponds to a set of their numbers in declaration order.
				 * @return mapping of the names of the named groups to the sets of their numbers
				 *
				 * \~
				 */
				const unordered_map <string, vector <uint32_t>> & groups() const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения последовательности символов узла
				 *
				 * @param offset смещение начала последовательности в хранилище строк
				 * @param length длина последовательности в кодовых значениях
				 * @return       адрес начала последовательности кодовых значений
				 *
				 * \~english
				 * @brief Method of getting the character sequence of a node
				 * @param offset offset of the beginning of the sequence in the string storage
				 * @param length length of the sequence in code point values
				 * @return       address of the beginning of the sequence of code point values
				 *
				 * \~
				 */
				const uint32_t * sequence(const uint32_t offset, const uint32_t length) const noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод разбора выражения выбора одной из ветвей
				 *
				 * @return индекс сформированного узла синтаксического дерева
				 *
				 * \~english
				 * @brief Method of parsing an expression choosing one of the branches
				 * @return index of the built node of the syntax tree
				 *
				 * \~
				 */
				node_id_t parseAlternate() noexcept;
				/**
				 * \~russian
				 * @brief Метод разбора последовательности элементов выражения
				 *
				 * @return индекс сформированного узла синтаксического дерева
				 *
				 * \~english
				 * @brief Method of parsing a sequence of expression elements
				 * @return index of the built node of the syntax tree
				 *
				 * \~
				 */
				node_id_t parseConcat() noexcept;
				/**
				 * \~russian
				 * @brief Метод разбора элемента выражения с квантором повторения
				 *
				 * @return индекс сформированного узла синтаксического дерева
				 *
				 * \~english
				 * @brief Method of parsing an expression element with a repetition quantifier
				 * @return index of the built node of the syntax tree
				 *
				 * \~
				 */
				node_id_t parseRepeat() noexcept;
				/**
				 * \~russian
				 * @brief Метод разбора одиночного элемента выражения
				 *
				 * @return индекс сформированного узла синтаксического дерева
				 *
				 * \~english
				 * @brief Method of parsing a single expression element
				 * @return index of the built node of the syntax tree
				 *
				 * \~
				 */
				node_id_t parseAtom() noexcept;
				/**
				 * \~russian
				 * @brief Метод разбора группы регулярного выражения
				 *
				 * @return индекс сформированного узла синтаксического дерева
				 *
				 * \~english
				 * @brief Method of parsing a group of a regular expression
				 * @return index of the built node of the syntax tree
				 *
				 * \~
				 */
				node_id_t parseGroup() noexcept;
				/**
				 * \~russian
				 * @brief Метод разбора класса символов
				 *
				 * @return индекс сформированного узла синтаксического дерева
				 *
				 * \~english
				 * @brief Method of parsing a character class
				 * @return index of the built node of the syntax tree
				 *
				 * \~
				 */
				node_id_t parseClass() noexcept;
				/**
				 * \~russian
				 * @brief Метод разбора условного выражения
				 *
				 * @details Позиция разбора установлена на круглую скобку, открывающую условие.
				 *          Метод разбирает условие, обе ветви условного выражения и завершающую
				 *          круглую скобку условного выражения целиком.
				 *
				 * @return индекс сформированного узла синтаксического дерева
				 *
				 * \~english
				 * @brief Method of parsing a conditional expression
				 * @details The parsing position is set at the parenthesis opening the condition.
				 *          The method parses the condition, both branches of the conditional expression and the closing
				 *          parenthesis of the conditional expression as a whole.
				 * @return index of the built node of the syntax tree
				 *
				 * \~
				 */
				node_id_t parseCondition() noexcept;
				/**
				 * \~russian
				 * @brief Метод разбора экранированной последовательности вне класса символов
				 *
				 * @return индекс сформированного узла синтаксического дерева
				 *
				 * \~english
				 * @brief Method of parsing an escaped sequence outside a character class
				 * @return index of the built node of the syntax tree
				 *
				 * \~
				 */
				node_id_t parseEscape() noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод разбора кванторов повторения элемента выражения
				 *
				 * @details Метод применяет к переданному узлу все следующие за ним кванторы
				 *          повторения. Если квантор отсутствует, узел возвращается без изменений.
				 *
				 * @param node индекс узла, к которому применяется квантор повторения
				 * @return     индекс сформированного узла синтаксического дерева
				 *
				 * \~english
				 * @brief Method of parsing the repetition quantifiers of an expression element
				 * @details The method applies to the passed node all the repetition quantifiers
				 *          that follow it. If there is no quantifier, the node is returned unchanged.
				 * @param node index of the node the repetition quantifier is applied to
				 * @return     index of the built node of the syntax tree
				 *
				 * \~
				 */
				node_id_t parseQuantifier(const node_id_t node) noexcept;
				/**
				 * \~russian
				 * @brief Метод разбора встроенных опций регулярного выражения
				 *
				 * @details Метод разбирает последовательность букв опций вида «(?imsx-imsx»
				 *          и формирует набор устанавливаемых и снимаемых режимов компиляции.
				 *
				 * @param enable набор устанавливаемых режимов компиляции
				 * @param disable набор снимаемых режимов компиляции
				 * @return       результат выполнения разбора
				 *
				 * \~english
				 * @brief Method of parsing the inline options of a regular expression
				 * @details The method parses a sequence of option letters of the «(?imsx-imsx» form
				 *          and builds the set of the compilation modes being set and cleared.
				 * @param enable set of the compilation modes being set
				 * @param disable set of the compilation modes being cleared
				 * @return       result of performing the parsing
				 *
				 * \~
				 */
				bool parseOptions(uint32_t & enable, uint32_t & disable) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод разбора имени именованной группы
				 *
				 * @param terminator символ завершения имени группы
				 * @param index      индекс имени в хранилище имён
				 * @return           результат выполнения разбора
				 *
				 * \~english
				 * @brief Method of parsing the name of a named group
				 * @param terminator character terminating the group name
				 * @param index      index of the name in the name storage
				 * @return           result of performing the parsing
				 *
				 * \~
				 */
				bool parseName(const char terminator, uint32_t & index) noexcept;
				/**
				 * \~russian
				 * @brief Метод разбора свойства Юникода
				 *
				 * @details Метод разбирает последовательность вида «\p{Lu}» либо «\pL»
				 *          и добавляет соответствующее свойство в класс символов.
				 *
				 * @param negative флаг отрицания свойства
				 * @param result   класс символов для добавления свойства
				 * @return         результат выполнения разбора
				 *
				 * \~english
				 * @brief Method of parsing a Unicode property
				 * @details The method parses a sequence of the «\p{Lu}» or «\pL» form
				 *          and adds the corresponding property to the character class.
				 * @param negative property negation flag
				 * @param result   character class to add the property to
				 * @return         result of performing the parsing
				 *
				 * \~
				 */
				bool parseProperty(const bool negative, class_t & result) noexcept;
				/**
				 * \~russian
				 * @brief Метод разбора класса символов POSIX
				 *
				 * @details Метод разбирает последовательность вида «[:alpha:]»
				 *          и добавляет соответствующие диапазоны в класс символов.
				 *
				 * @param result класс символов для добавления диапазонов
				 * @return       результат выполнения разбора
				 *
				 * \~english
				 * @brief Method of parsing a POSIX character class
				 * @details The method parses a sequence of the «[:alpha:]» form
				 *          and adds the corresponding ranges to the character class.
				 * @param result character class to add the ranges to
				 * @return       result of performing the parsing
				 *
				 * \~
				 */
				bool parsePosix(class_t & result) noexcept;
				/**
				 * \~russian
				 * @brief Метод разбора экранированной последовательности внутри класса символов
				 *
				 * @details Метод разбирает экранированную последовательность и либо возвращает
				 *          кодовое значение одиночного символа, либо добавляет диапазоны
				 *          сокращённого класса символов в формируемый класс.
				 *
				 *          Дословная последовательность «\\Q...\\E» одиночным символом
				 *          выступает краем диапазона: нижним краем служит символ её
				 *          последний, а верхним - первый, прочие же символы входят
				 *          в класс наравне с остальными.
				 *
				 * @param result класс символов для добавления диапазонов
				 * @param code   кодовое значение разобранного одиночного символа
				 * @param single флаг разбора одиночного символа
				 * @param ending флаг разбора верхнего края диапазона
				 * @return       результат выполнения разбора
				 *
				 * \~english
				 * @brief Method of parsing an escaped sequence inside a character class
				 * @details The method parses an escaped sequence and either returns
				 *          the code point value of a single character or adds the ranges
				 *          of an abbreviated character class to the class being built.
				 *          A literal «\\Q...\\E» sequence acts as a single character
				 *          at an edge of a range: its last character serves as the lower
				 *          edge and its first one as the upper edge, whereas the rest of
				 *          the characters enter the class along with the others.
				 * @param result character class to add the ranges to
				 * @param code   code point value of the parsed single character
				 * @param single flag of parsing a single character
				 * @param ending flag of parsing the upper edge of a range
				 * @return       result of performing the parsing
				 *
				 * \~
				 */
				bool parseClassEscape(class_t & result, uint32_t & code, bool & single, const bool ending = false) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод создания узла синтаксического дерева
				 *
				 * @param type тип создаваемого узла синтаксического дерева
				 * @return     индекс созданного узла в арене узлов
				 *
				 * \~english
				 * @brief Method of creating a node of the syntax tree
				 * @param type type of the node of the syntax tree to create
				 * @return     index of the created node in the node arena
				 *
				 * \~
				 */
				node_id_t createNode(const node_t type) noexcept;
				/**
				 * \~russian
				 * @brief Метод создания узла последовательности символов
				 *
				 * @param codes набор кодовых значений символов последовательности
				 * @return      индекс созданного узла в арене узлов
				 *
				 * \~english
				 * @brief Method of creating a character sequence node
				 * @param codes set of the code point values of the characters of the sequence
				 * @return      index of the created node in the node arena
				 *
				 * \~
				 */
				node_id_t makeString(const vector <uint32_t> & codes) noexcept;
				/**
				 * \~russian
				 * @brief Метод создания узла класса символов
				 *
				 * @details Набор диапазонов класса символов приводится к нормальному виду
				 *          перед размещением класса в хранилище классов.
				 *
				 * @param value класс символов для размещения в хранилище классов
				 * @return      индекс созданного узла в арене узлов
				 *
				 * \~english
				 * @brief Method of creating a character class node
				 * @details The set of ranges of the character class is brought to the normal form
				 *          before the class is placed in the class storage.
				 * @param value character class to place in the class storage
				 * @return      index of the created node in the node arena
				 *
				 * \~
				 */
				/**
				 * \~russian
				 * @brief Метод создания узла любого символа с учётом соглашения
				 *
				 * @details Точка вне режима «DOTALL» сопоставляет всякий символ,
				 *          кроме тех, что соглашение о переводе строки завершением
				 *          строки почитает. Узел любого символа знает лишь перевод
				 *          строки, поэтому прочие соглашения выражаются классом
				 *          символов отрицающим - устройством, всем путям исполнения
				 *          уже известным. Соглашение пары возврата каретки одиночных
				 *          завершений не знает вовсе, отчего точка при нём
				 *          сопоставляет всякий символ.
				 *
				 * @return индекс созданного узла в арене узлов
				 *
				 * \~english
				 * @brief Method of creating a node of any character with regard to the convention
				 * @details Outside the «DOTALL» mode the dot matches every character
				 *          except those that the newline convention deems an end
				 *          of a line. The node of any character knows the line feed alone,
				 *          therefore the other conventions are expressed by a negated character
				 *          class — a construct already known to every execution path. The convention
				 *          of the carriage return pair knows no single-character terminators at all,
				 *          hence the dot under it matches every character.
				 * @return index of the created node in the node arena
				 *
				 * \~
				 */
				node_id_t makeAny() noexcept;
				node_id_t makeClass(class_t & value) noexcept;
				/**
				 * \~russian
				 * @brief Метод создания узла из набора дочерних узлов
				 *
				 * @details Если набор дочерних узлов пуст, создаётся узел пустого выражения.
				 *          Если набор состоит из единственного узла, узел возвращается без обёртки.
				 *
				 * @param type  тип создаваемого узла синтаксического дерева
				 * @param items набор индексов дочерних узлов в арене узлов
				 * @return      индекс созданного узла в арене узлов
				 *
				 * \~english
				 * @brief Method of creating a node from a set of child nodes
				 * @details If the set of child nodes is empty, a node of an empty expression is created.
				 *          If the set consists of a single node, the node is returned without a wrapper.
				 * @param type  type of the node of the syntax tree to create
				 * @param items set of the indices of the child nodes in the node arena
				 * @return      index of the created node in the node arena
				 *
				 * \~
				 */
				node_id_t makeList(const node_t type, const vector <node_id_t> & items, const size_t from = 0) noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки наличия вызова подпрограммы либо ссылки на захват
				 *
				 * @param id индекс узла, с которого начинается обход
				 *
				 * @return   результат проверки наличия вызова либо ссылки
				 *
				 * \~english
				 * @brief Method of checking the presence of a subroutine call or a backreference
				 * @param id index of the node the traversal starts from
				 * @return   result of checking the presence of a subroutine call
				 *
				 * \~
				 */
				bool referring(const node_id_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод поиска узла захватывающей группы по её номеру
				 *
				 * @param number номер разыскиваемой захватывающей группы
				 *
				 * @return       индекс узла захватывающей группы
				 *
				 * \~english
				 * @brief Method of searching for a capturing group node by its number
				 * @param number number of the capturing group being searched for
				 * @return       index of the capturing group node
				 *
				 * \~
				 */
				node_id_t grouping(const uint32_t number) const noexcept;
				/**
				 * \~russian
				 * @brief Метод создания узла рекурсивного вызова
				 *
				 * @details Если вызов задан именем группы, номер вызываемой группы
				 *          разрешается после завершения разбора регулярного выражения.
				 *
				 * @param number номер вызываемой группы либо нуль для вызова выражения целиком
				 * @param index  индекс имени вызываемой группы в хранилище имён
				 * @param offset смещение вызова в тексте регулярного выражения
				 * @return       индекс созданного узла в арене узлов
				 *
				 * \~english
				 * @brief Method of creating a recursive call node
				 * @details If the call is given by a group name, the number of the called group
				 *          is resolved after parsing of the regular expression has finished.
				 * @param number number of the called group or zero for a call of the whole expression
				 * @param index  index of the name of the called group in the name storage
				 * @param offset offset of the call in the text of the regular expression
				 * @return       index of the created node in the node arena
				 *
				 * \~
				 */
				node_id_t makeRecurse(const uint32_t number, const uint32_t index, const size_t offset) noexcept;
				/**
				 * \~russian
				 * @brief Метод создания узла ссылки на именованную группу
				 *
				 * @details Номер группы разрешается после завершения разбора
				 *          регулярного выражения методом разрешения отложенных ссылок.
				 *
				 * @param index  индекс имени группы в хранилище имён
				 * @param offset смещение ссылки в тексте регулярного выражения
				 * @return       индекс созданного узла в арене узлов
				 *
				 * \~english
				 * @brief Method of creating a named group reference node
				 * @details The number of the group is resolved after parsing of
				 *          the regular expression has finished by the method resolving the deferred references.
				 * @param index  index of the group name in the name storage
				 * @param offset offset of the reference in the text of the regular expression
				 * @return       index of the created node in the node arena
				 *
				 * \~
				 */
				node_id_t makeBackref(const uint32_t index, const size_t offset) noexcept;
				/**
				 * \~russian
				 * @brief Метод добавления дочернего узла синтаксического дерева
				 *
				 * @param parent индекс родительского узла в арене узлов
				 * @param child  индекс добавляемого дочернего узла в арене узлов
				 *
				 * \~english
				 * @brief Method of adding a child node of the syntax tree
				 * @param parent index of the parent node in the node arena
				 * @param child  index of the added child node in the node arena
				 *
				 * \~
				 */
				void appendChild(const node_id_t parent, const node_id_t child) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод извлечения кодового значения символа в текущей позиции
				 *
				 * @details В режиме «UTF» метод разбирает последовательность UTF-8 целиком,
				 *          иначе возвращает кодовое значение одиночного байта.
				 *
				 * @param code кодовое значение извлечённого символа
				 * @return     результат извлечения кодового значения символа
				 *
				 * \~english
				 * @brief Method of getting the code point value of the character at the current position
				 * @details In the «UTF» mode the method parses the whole UTF-8 sequence,
				 *          otherwise it returns the code point value of a single byte.
				 * @param code code point value of the obtained character
				 * @return     result of getting the code point value of the character
				 *
				 * \~
				 */
				bool readCode(uint32_t & code) noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения целого числа в текущей позиции
				 *
				 * @param result извлечённое значение целого числа
				 * @return       результат извлечения целого числа
				 *
				 * \~english
				 * @brief Method of getting the integer at the current position
				 * @param result the obtained value of the integer
				 * @return       result of getting the integer
				 *
				 * \~
				 */
				bool readNumber(uint32_t & result) noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения кодового значения экранированного символа
				 *
				 * @details Метод разбирает экранированные последовательности, обозначающие
				 *          одиночный символ: управляющие символы, шестнадцатеричные,
				 *          восьмеричные и управляющие последовательности вида «\cX».
				 *          Позиция разбора установлена на букву последовательности.
				 *
				 * @param code кодовое значение разобранного символа
				 * @return     результат извлечения кодового значения символа
				 *
				 * \~english
				 * @brief Method of getting the code point value of an escaped character
				 * @details The method parses the escaped sequences denoting
				 *          a single character: control characters, hexadecimal,
				 *          octal and control sequences of the «\cX» form.
				 *          The parsing position is set at the letter of the sequence.
				 * @param code code point value of the parsed character
				 * @return     result of getting the code point value of the character
				 *
				 * \~
				 */
				bool readEscapeCode(uint32_t & code) noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки начала квантора повторения
				 *
				 * @details Метод определяет, образует ли последовательность, начинающаяся
				 *          с указанной позиции, корректный квантор повторения в фигурных
				 *          скобках. Позиция разбора при проверке не изменяется.
				 *
				 * @param pos позиция проверяемой последовательности
				 * @return    результат проверки начала квантора повторения
				 *
				 * \~english
				 * @brief Method of checking the beginning of a repetition quantifier
				 * @details The method determines whether the sequence starting
				 *          at the specified position forms a valid repetition quantifier in curly
				 *          braces. The parsing position is not changed by the check.
				 * @param pos position of the checked sequence
				 * @return    result of checking the beginning of a repetition quantifier
				 *
				 * \~
				 */
				bool isQuantifier(const size_t pos) const noexcept;
				/**
				 * \~russian
				 * @brief Метод пропуска пробельных символов и комментариев
				 *
				 * @details Пропуск выполняется только в режиме «EXTENDED», в котором
				 *          пробельные символы выражения игнорируются, а символ «#»
				 *          начинает комментарий до конца строки.
				 *
				 * \~english
				 * @brief Method of skipping whitespace characters and comments
				 * @details The skipping is performed only in the «EXTENDED» mode, in which
				 *          the whitespace characters of the expression are ignored, and the «#» character
				 *          begins a comment up to the end of the line.
				 *
				 * \~
				 */
				void skipSpaces() noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод приведения класса символов к нормальному виду
				 *
				 * @details Диапазоны класса символов упорядочиваются по нижней границе,
				 *          пересекающиеся и смежные диапазоны объединяются.
				 *
				 * @param value класс символов для приведения к нормальному виду
				 *
				 * \~english
				 * @brief Method of bringing a character class to the normal form
				 * @details The ranges of the character class are ordered by the lower bound,
				 *          overlapping and adjacent ranges are merged.
				 * @param value character class to bring to the normal form
				 *
				 * \~
				 */
				void normalize(class_t & value) const noexcept;
				/**
				 * \~russian
				 * @brief Метод добавления сокращённого класса символов
				 *
				 * @details Метод добавляет в класс символов диапазоны сокращённых классов,
				 *          обозначаемых последовательностями вида «\d», «\w», «\s», «\h» и «\v».
				 *
				 * @param letter буква сокращённого класса символов
				 * @param result класс символов для добавления диапазонов
				 * @return       результат добавления сокращённого класса символов
				 *
				 * \~english
				 * @brief Method of adding an abbreviated character class
				 * @details The method adds to the character class the ranges of the abbreviated classes
				 *          denoted by the sequences of the «\d», «\w», «\s», «\h» and «\v» form.
				 * @param letter letter of the abbreviated character class
				 * @param result character class to add the ranges to
				 * @return       result of adding the abbreviated character class
				 *
				 * \~
				 */
				bool shorthand(const char letter, class_t & result) const noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод предварительного прохода по регулярному выражению
				 *
				 * @details Проход определяет общее количество захватывающих групп выражения.
				 *          Количество групп требуется для различения ссылок на захваченные
				 *          группы и восьмеричных экранированных последовательностей.
				 *
				 * @return результат выполнения предварительного прохода
				 *
				 * \~english
				 * @brief Method of the preliminary pass over the regular expression
				 * @details The pass determines the total number of capturing groups of the expression.
				 *          The number of groups is required to tell apart the references to captured
				 *          groups and the octal escaped sequences.
				 * @return result of performing the preliminary pass
				 *
				 * \~
				 */
				bool prescan() noexcept;
				/**
				 * \~russian
				 * @brief Метод разбора начальных указаний регулярного выражения
				 *
				 * @details Указания вида «(*UTF)» размещаются в начале выражения
				 *          и правят режимами его сборки. Разбираются те из них,
				 *          что ложатся на признаки сборки, модулем заведённые,
				 *          а прочие остаются разбору общему, отвергающему их
				 *          доводом неподдерживаемой конструкции.
				 *
				 * \~english
				 * @brief Method of parsing the start-of-pattern options of a regular expression
				 * @details The options of the «(*UTF)» kind are placed at the start of an expression
				 *          and govern the modes of its compilation. The ones parsed are those
				 *          that map onto the compilation flags declared by the module,
				 *          whereas the rest are left to the general parsing, which refuses them
				 *          with the reason of an unsupported construct.
				 *
				 * \~
				 */
				void settings() noexcept;
				/**
				 * \~russian
				 * @brief Метод вычисления длины сопоставляемой узлами последовательности
				 *
				 * @details Метод вычисляет наименьшую и наибольшую длину последовательности,
				 *          сопоставляемой цепочкой узлов одного уровня вложенности. Наибольшая
				 *          длина принимает значение «UNBOUNDED», если последовательность
				 *          не ограничена сверху.
				 *
				 * @param id  индекс первого узла цепочки в арене узлов
				 * @param min наименьшая длина сопоставляемой последовательности
				 * @param max наибольшая длина сопоставляемой последовательности
				 *
				 * \~english
				 * @brief Method of computing the length of the sequence matched by the nodes
				 * @details The method computes the smallest and the largest length of the sequence
				 *          matched by a chain of nodes of the same nesting level. The largest
				 *          length takes the «UNBOUNDED» value if the sequence
				 *          is not bounded from above.
				 * @param id  index of the first node of the chain in the node arena
				 * @param min smallest length of the matched sequence
				 * @param max largest length of the matched sequence
				 *
				 * \~
				 */
				void measure(const node_id_t id, uint32_t & min, uint32_t & max) const noexcept;
				/**
				 * \~russian
				 * @brief Метод вычисления длины сопоставляемой узлом последовательности
				 *
				 * @details Метод вычисляет длину последовательности, сопоставляемой
				 *          единственным узлом, без учёта следующих за ним узлов
				 *          того же уровня вложенности.
				 *
				 * @param id  индекс узла в арене узлов
				 * @param min наименьшая длина сопоставляемой последовательности
				 * @param max наибольшая длина сопоставляемой последовательности
				 *
				 * \~english
				 * @brief Method of computing the length of the sequence matched by a node
				 * @details The method computes the length of the sequence matched by
				 *          a single node, without taking into account the nodes that follow it
				 *          at the same nesting level.
				 * @param id  index of the node in the node arena
				 * @param min smallest length of the matched sequence
				 * @param max largest length of the matched sequence
				 *
				 * \~
				 */
				void measureNode(const node_id_t id, uint32_t & min, uint32_t & max) const noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод разрешения отложенных ссылок на именованные группы
				 *
				 * @return результат разрешения отложенных ссылок
				 *
				 * \~english
				 * @brief Method of resolving the deferred references to named groups
				 * @return result of resolving the deferred references
				 *
				 * \~
				 */
				bool resolve() noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод установки ошибки разбора
				 *
				 * @param error код ошибки разбора регулярного выражения
				 * @param pos   смещение ошибки в тексте регулярного выражения
				 * @return      индекс отсутствующего узла синтаксического дерева
				 *
				 * \~english
				 * @brief Method of setting a parsing error
				 * @param error parse error code of the regular expression
				 * @param pos   offset of the error in the text of the regular expression
				 * @return      index of a missing node of the syntax tree
				 *
				 * \~
				 */
				node_id_t fail(const error_t error, const size_t pos) noexcept;
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
				Parser() noexcept;
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
				~Parser() noexcept {}
		} parser_t;
	};
};

#endif // __AWH_REGEX_PARSER__
