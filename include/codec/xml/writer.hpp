/**
 * @file writer.hpp
 * @date 2026-08-01
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
 * @brief Заголовочный файл записи текста разметки XML — класс Writer, собирающий правильно построенный
 *        текст разметки последовательными указаниями, с экранированием содержимого,
 *        объявлением пространств имён и необязательным отступом
 *
 * \~english
 * @brief Header file of the writing of an XML markup text — the Writer class, which assembles a well-formed
 *        markup text by successive directives, with an escaping of the content,
 *        with a declaration of the namespaces and with an optional indent
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_XML_WRITER__
#define __AWH_CODEC_XML_WRITER__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы модуля
 */
#include <sys/log.hpp>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "document.hpp"

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
		 * @brief Пространство имён контейнера XML
		 *
		 *
		 * \~english
		 * @brief XML container namespace
		 *
		 * \~
		 */
		namespace xml {
			/**
			 * \~russian
			 * @brief Класс записи текста разметки
			 *
			 * @details Собирает текст разметки последовательными указаниями об открытии и
			 * закрытии узлов. Запись сама следит за парностью меток, экранирует содержимое
			 * и не позволяет собрать неправильно построенный текст: указание, нарушающее
			 * строение, отвергается, а не записывается
			 *
			 * @par Порядок работы
			 *
			 * @note Записывается текст в кодировке UTF-8. Прочие кодировки при записи не
			 * применяются намеренно: они лишь сужают круг тех, кто сможет текст прочесть
			 *
			 *  @code{.cpp}
			 *  writer_t writer;
			 *
			 *  writer.declaration();
			 *  writer.open("Envelope", "http://schemas.xmlsoap.org/soap/envelope/");
			 *  writer.attribute("encodingStyle", "http://schemas.xmlsoap.org/soap/encoding/");
			 *  writer.open("Body");
			 *  writer.close();
			 *  writer.close();
			 *
			 *  const string & result = writer.text();
			 *  @endcode
			 *
			 * \~english
			 * @brief Class of the writing of a markup text
			 * @details Assembles a markup text by successive directives about the opening and
			 * the closing of the nodes. The writing itself watches over the pairing of the tags, escapes the content
			 * and does not allow assembling a text that is not well-formed: a directive violating
			 * the construction is rejected rather than written
			 * @par Order of the work
			 * @note The text is written in the UTF-8 encoding. The other encodings are not applied at the writing
			 * deliberately: they only narrow the circle of those who will be able to read the text
			 *
			 *  @code{.cpp}
			 *  writer_t writer;
			 *
			 *  writer.declaration();
			 *  writer.open("Envelope", "http://schemas.xmlsoap.org/soap/envelope/");
			 *  writer.attribute("encodingStyle", "http://schemas.xmlsoap.org/soap/encoding/");
			 *  writer.open("Body");
			 *  writer.close();
			 *  writer.close();
			 *
			 *  const string & result = writer.text();
			 *  @endcode
			 *
			 */
			typedef class __AWH_SHARED_EXPORT__ Writer {
				public:
					/**
					 * \~russian
					 * @brief Настройки записи текста разметки
					 *
					 * \~english
					 * @brief Settings of the writing of a markup text
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Settings {
						// Вид записи собираемого текста разметки
						format_t format;
						// Флаг записи узлов без содержимого самозакрывающейся меткой
						bool collapse;
						// Флаг экранирования знаков, выходящих за пределы US-ASCII, числовыми ссылками
						bool escapeNonAscii;
						// Количество знаков отступа на один уровень вложенности
						uint8_t indent;
						// Знак, которым ставится отступ нарядной записи
						separator_t separator;
						/**
						 * \~russian
						 * Наибольшая допустимая глубина вложенности узлов, ноль - без предела
						 *
						 * @details Предела по умолчанию нет: обход дерева ведётся собственным
						 * стеком, а не рекурсией, и стеку задачи глубина записи не грозит. Держи
						 * запись предел разбора, дерево, разобранное с поднятым `maxDepth`,
						 * обратно бы не записывалось - а разобранное обязано записываться
						 *
						 * @note Предел нужен лишь тому, кто открывает узлы сам, не имея дерева:
						 * ошибка в его собственном ходе иначе растит стек открытых узлов, пока
						 * не исчерпается память. Превышение отвечает отказом `DEPTH_EXCEEDED`
						 *
						 * \~english
						 * Largest admissible depth of the nesting of the nodes, zero — without a limit
						 * @details There is no limit by default: the traversal of the tree is conducted by an own
						 * stack rather than by a recursion, and the depth of the writing does not threaten the stack of the task. Were the
						 * writing to keep the limit of the parsing, a tree parsed with a raised `maxDepth`
						 * would not be written back — while what has been parsed is obliged to be writable
						 * @note The limit is needed only by the one who opens the nodes himself, having no tree:
						 * an error in his own course would otherwise grow the stack of the open nodes until
						 * the memory is exhausted. An excess answers with a `DEPTH_EXCEEDED` refusal
						 *
						 * \~
						 */
						uint32_t maxDepth;
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
					 * @brief Запись открытого узла разметки
					 *
					 * \~english
					 * @brief Record of an open markup node
					 *
					 * \~
					 */
					typedef struct Opened {
						// Имя узла в записи, принятой в исходном тексте
						string name;
						// Признак того, что метка узла ещё не завершена
						bool pending;
						// Признак того, что узел уже содержит вложенное содержимое
						bool filled;
						// Признак того, что узел содержит вложенные узлы разметки
						bool elements;
						/**
						 * \~russian
						 * Признак записи содержимого узла в одну строку без отступов
						 *
						 * @note Взводится узлам со смешанным содержимым и узлам, требующим
						 * сохранения пробельного содержимого: отступ внутри них - не украшение
						 * записи, а перемена самого содержимого
						 *
						 * \~english
						 * Flag of the writing of the content of a node in a single line without the indents
						 * @note Raised for the nodes with a mixed content and for the nodes requiring
						 * a preservation of the whitespace content: an indent inside them is not an adornment
						 * of the writing but a change of the content itself
						 *
						 * \~
						 */
						bool oneline;
						// Количество связываний префиксов, объявленных узлом
						uint32_t bindings;
						/**
						 * Записанные имена атрибутов узла в том виде, в каком они попали
						 * в текст. Договор не допускает повторного имени атрибута у одного
						 * узла, а объявление пространства имён является таким же атрибутом:
						 * без учёта записанного запись собирала бы текст, который её же
						 * собственное чтение принять не может
						 */
						vector <string> names;
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
						Opened() noexcept : pending(false), filled(false), elements(false), oneline(false), bindings(0) {}
					} opened_t;
					/**
					 * \~russian
					 * @brief Связывание префикса с пространством имён при записи
					 *
					 * \~english
					 * @brief Binding of a prefix to a namespace at the writing
					 *
					 * \~
					 */
					typedef struct Scope {
						// Префикс без разделителя
						string prefix;
						// Обозначение связанного пространства имён
						string uri;
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
						Scope() noexcept {}
					} scope_t;
				private:
					// Настройки записи текста разметки
					settings_t _settings;
				private:
					// Код ошибки последней операции записи
					error_t _error;
				private:
					/**
					 * \~russian
					 * Объект ведения журнала работы
					 *
					 * @note Умолчание стоит прямо в объявлении: конструкторы, логгера не
					 *       принимающие, оставили бы поле неопределённым
					 *
					 * \~english
					 * Object of the keeping of the work log
					 *
					 * \~
					 */
					const log_t * _log = nullptr;
				private:
					/**
					 * \~russian
					 * @brief Метод отказа записи с сообщением о нём в журнал
					 *
					 * @details Способ этот стоит ГОРЛОМ: всякий отказ записи обязан идти через
					 * него. Сообщение в журнал пишется в одном месте на всю запись, а не при
					 * всяком присваивании кода отказа: иначе места записи разошлись бы с
					 * местами отказа, и часть бед уходила бы молча
					 *
					 * @param error код ошибки записи
					 * @return      всегда ложь, ради возврата им из места отказа
					 *
					 * \~english
					 * @brief Method of the refusal of the writing with a report of it to the log
					 *
					 * @param error the code of the writing error
					 * @return      always false, for the returning by it from the place of the refusal
					 *
					 * \~
					 */
					bool refuse(const error_t error) noexcept;
				private:
					// Признак того, что корневой узел разметки уже записан
					bool _root;
				private:
					// Собираемый текст разметки
					string _text;
				private:
					/**
					 * \~russian
					 * Глубина стека открытых узлов разметки
					 *
					 * @details Записи открытых узлов со стека не снимаются, а
					 *          переиспользуются: глубина хранится отдельно, и всё, что
					 *          лежит в `_opened` за её пределом, - это записи ранее
					 *          закрытых узлов, сохранённые ради их же ёмкости. Снятие
					 *          записи уничтожало бы перечень записанных имён атрибутов
					 *          вместе с занятой им памятью, и следующий узел заводил бы
					 *          его заново: замер показывал по два выделения памяти на
					 *          каждый записанный узел против семнадцати на чтение
					 *          документа целиком
					 *
					 * @warning Обращаться к вершине стека следует по этой глубине, а не
					 *          методом `back` хранилища: тот выдал бы запись давно
					 *          закрытого узла
					 *
					 * \~english
					 * Depth of the stack of the open markup nodes
					 * @details The records of the open nodes are not popped off the stack but are
					 *          reused: the depth is stored separately, and everything that
					 *          lies in `_opened` beyond its limit is the records of the previously
					 *          closed nodes preserved for the sake of their own capacity. A popping of a
					 *          record would destroy the list of the written names of the attributes
					 *          together with the memory occupied by it, and the next node would create
					 *          it anew: a measurement showed two memory allocations per
					 *          every written node against seventeen for the reading of a
					 *          document in full
					 * @warning The top of the stack should be addressed by this depth rather than
					 *          by the `back` method of the storage: that one would issue the record of a long
					 *          since closed node
					 *
					 * \~
					 */
					size_t _depth;
					// Хранилище записей открытых узлов разметки
					vector <opened_t> _opened;
				private:
					// Действующие связывания префиксов с пространствами имён
					vector <scope_t> _scopes;
					/**
					 * \~russian
					 * Число действующих связываний префиксов
					 *
					 * @note Держится отдельно от размера хранилища намеренно: снятое
					 * связывание из хранилища не удаляется, а лишь перестаёт считаться
					 * действующим - память, отведённая под его строки, остаётся и
					 * достаётся следующему связыванию. Устройство то же, что у стека
					 * открытых узлов, и по тому же доводу
					 *
					 * \~english
					 * Number of the effective bindings of the prefixes
					 * @note It is kept separately from the size of the storage deliberately: a removed
					 * binding is not deleted from the storage but only ceases to be considered
					 * an effective one — the memory allotted for its strings remains and
					 * goes to the next binding. The arrangement is the same as that of the stack
					 * of the open nodes, and by the same argument
					 *
					 * \~
					 */
					size_t _bindings;
				private:
					/**
					 * \~russian
					 * @brief Метод завершения незакрытой метки узла
					 *
					 * \~english
					 * @brief Method of completing an unclosed tag of a node
					 *
					 * \~
					 */
					void flush() noexcept;
					/**
					 * \~russian
					 * @brief Метод записи отступа перед содержимым
					 *
					 * \~english
					 * @brief Method of writing an indent before the content
					 *
					 * \~
					 */
					void indent() noexcept;
					/**
					 * \~russian
					 * @brief Метод записи последовательности знаков с экранированием
					 *
					 * @param text      записываемая последовательность знаков
					 * @param attribute признак записи значения атрибута
					 * @return          результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing a sequence of characters with an escaping
					 * @param text      sequence of characters being written
					 * @param attribute flag of the writing of the value of an attribute
					 * @return          result of performing the operation
					 *
					 * \~
					 */
					bool escape(const string_view text, const bool attribute) noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки допустимости знаков дословно записываемой последовательности
					 *
					 * @details Содержимое примечания, дословного раздела и указания обработчику
					 * попадает в текст без экранирования, и проверка знаков экранированием там
					 * не выполняется. Проверять их всё равно необходимо: недопустимый в
					 * разметке знак либо ошибочная последовательность кодировки делают
					 * собранный текст непрочитываемым
					 *
					 * @param text проверяемая последовательность знаков
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of checking the admissibility of the characters of a sequence being written literally
					 * @details The content of a comment, of a literal section and of a processing instruction
					 * gets into the text without an escaping, and the check of the characters by the escaping there
					 * is not performed. It is all the same necessary to check them: a character inadmissible in a
					 * markup or an erroneous sequence of the encoding make the
					 * assembled text unreadable
					 * @param text sequence of characters being checked
					 * @return     result of performing the operation
					 *
					 * \~
					 */
					bool verify(const string_view text) noexcept;
					/**
					 * \~russian
					 * @brief Метод получения префикса для пространства имён
					 *
					 * @details Обозначение, ещё не связанное ни с одним префиксом,
					 * объявляется при записываемом узле с самостоятельно назначенным префиксом
					 *
					 * @param uri       обозначение пространства имён
					 * @param attribute признак поиска префикса для имени атрибута
					 * @param result    найденный либо назначенный префикс
					 * @return          результат выполнения операции
					 *
					 * \~english
					 * @brief Method of getting the prefix for a namespace
					 * @details A designation not yet bound to any prefix
					 * is declared at the node being written with an independently assigned prefix
					 * @param uri       designation of the namespace
					 * @param attribute flag of the search of a prefix for the name of an attribute
					 * @param result    found or assigned prefix
					 * @return          result of performing the operation
					 *
					 * \~
					 */
					bool prefix(const string_view uri, const bool attribute, string & result) noexcept;
					/**
					 * \~russian
					 * @brief Метод занятия имени атрибута открытым узлом
					 *
					 * @details Договор не допускает повторного имени атрибута у одного узла,
					 * а объявление пространства имён является таким же атрибутом. Без учёта
					 * записанного запись собирала бы текст, который её же собственное чтение
					 * принять не может
					 *
					 * @param name имя атрибута в том виде, в каком оно попадает в текст
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of occupying the name of an attribute by an open node
					 * @details The protocol does not admit a repeated name of an attribute at a single node,
					 * while a declaration of a namespace is the same kind of attribute. Without an accounting
					 * of what has been written the writing would assemble a text which its own reading cannot
					 * accept
					 * @param name name of the attribute in the form in which it gets into the text
					 * @return     result of performing the operation
					 *
					 * \~
					 */
					bool occupy(const string_view name) noexcept;
					/**
					 * \~russian
					 * @brief Метод открытия узла разметки с заданными объявлениями
					 *
					 * @details Объявления записываются узлу прежде подбора префикса, отчего подбор
					 * находит заданный префикс уже связанным и своего не назначает. Порядок здесь
					 * важен: имя узла записывается первым, а объявления стоят в метке за ним, - и
					 * узнать, каким префиксом записать имя, требуется до того, как объявления
					 * попадут в текст
					 *
					 * @param local    местное имя открываемого узла
					 * @param uri      обозначение пространства имён открываемого узла
					 * @param declares  объявления пространств имён, записываемые узлу
					 * @param preferred префикс, каким имя узла записано в исходном тексте
					 * @param verbatim  признак записи имён узла дословно, без пространств имён
					 * @param oneline   признак записи содержимого узла в одну строку
					 * @return          результат выполнения операции
					 *
					 * \~english
					 * @brief Method of opening a markup node with the given declarations
					 * @details The declarations are written to the node before the selection of a prefix, from which the selection
					 * finds the given prefix already bound and does not assign its own. The order here is
					 * important: the name of the node is written first, while the declarations stand in the tag after it — and
					 * it is required to learn by which prefix to write the name before the declarations
					 * get into the text
					 * @param local    local name of the node being opened
					 * @param uri      designation of the namespace of the node being opened
					 * @param declares  declarations of the namespaces being written to the node
					 * @param preferred prefix by which the name of the node is written in the source text
					 * @param verbatim  flag of the writing of the names of the node literally, without the namespaces
					 * @param oneline   flag of the writing of the content of the node in a single line
					 * @return          result of performing the operation
					 *
					 * \~
					 */
					bool open(const string_view local, const string_view uri, const vector <binding_t> * declares, const string_view * preferred, const bool verbatim, const bool oneline) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи узла дерева разметки с наследуемым обращением с пробелами
					 *
					 * @param node     записываемый узел дерева разметки
					 * @param preserve признак сохранения пробельного содержимого, унаследованный
					 *                 от объемлющих узлов
					 * @return         результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing a node of a markup tree with an inherited treatment of the spaces
					 * @param node     node of the markup tree being written
					 * @param preserve flag of the preservation of the whitespace content inherited
					 *                 from the enclosing nodes
					 * @return         result of performing the operation
					 *
					 * \~
					 */
					bool element(const node_t & node, const bool preserve) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи атрибута при открытом узле с дословной записью имени
					 *
					 * @details Дословная запись предназначена дереву, разобранному без
					 * пространств имён: имена лежат там целиком, вместе с разделителем
					 * префикса, а объявления пространств имён считаются обычными атрибутами.
					 * Подбор префикса такому имени не по чему вести, а учёт связываний
					 * учитывать нечего - объявлений в таком дереве нет
					 *
					 * @warning Дословная запись объявления мимо учёта действующих связываний
					 * допустима лишь потому, что задаётся она не вызывающим, а записью дерева:
					 * разбор с пространствами имён объявления из перечня атрибутов изымает, и
					 * смешать оба вида записи в одном тексте неоткуда
					 *
					 * @param local    имя атрибута, записываемое как есть
					 * @param value    значение атрибута
					 * @param uri      обозначение пространства имён атрибута
					 * @param verbatim признак дословной записи имени атрибута
					 * @return         результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing an attribute at an open node with a literal writing of the name
					 * @details The literal writing is intended for a tree parsed without the
					 * namespaces: the names lie there in full, together with the separator of the
					 * prefix, while the declarations of the namespaces are considered ordinary attributes.
					 * There is nothing to conduct a selection of a prefix for such a name by, while there is nothing for the accounting of the bindings
					 * to account — there are no declarations in such a tree
					 * @warning A literal writing of a declaration bypassing the accounting of the effective bindings
					 * is admissible only because it is given not by the caller but by the writing of a tree:
					 * a parsing with the namespaces extracts the declarations from the list of the attributes, and
					 * there is nowhere to mix both kinds of the writing in a single text
					 * @param local    name of the attribute written as it is
					 * @param value    value of the attribute
					 * @param uri      designation of the namespace of the attribute
					 * @param verbatim flag of the literal writing of the name of the attribute
					 * @return         result of performing the operation
					 *
					 * \~
					 */
					bool attribute(const string_view local, const string_view value, const string_view uri, const bool verbatim) noexcept;
				private:
					// Счётчик самостоятельно назначенных префиксов пространств имён
					uint32_t _counter;
				public:
					/**
					 * \~russian
					 * @brief Метод получения текущих настроек записи
					 *
					 * @return текущие настройки записи текста разметки
					 *
					 * \~english
					 * @brief Method of getting the current settings of the writing
					 * @return current settings of the writing of a markup text
					 *
					 * \~
					 */
					const settings_t & settings() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки настроек записи
					 *
					 * @param settings настройки записи текста разметки
					 *
					 * \~english
					 * @brief Method of setting the settings of the writing
					 * @param settings settings of the writing of a markup text
					 *
					 * \~
					 */
					void settings(const settings_t & settings) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод записи объявления разметки
					 *
					 * @details Записывается первым указанием, до всякого содержимого
					 *
					 * @param standalone признак самодостаточности текста разметки
					 * @return           результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing the markup declaration
					 * @details Written as the first directive, before any content
					 * @param standalone flag of the standaloneness of the markup text
					 * @return           result of performing the operation
					 *
					 * \~
					 */
					bool declaration(const standalone_t standalone = standalone_t::NONE) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод открытия узла разметки
					 *
					 * @details Непустое обозначение пространства имён объявляется при узле,
					 * если оно ещё не объявлено выше по стеку открытых узлов
					 *
					 * @param local местное имя открываемого узла
					 * @param uri   обозначение пространства имён открываемого узла
					 * @return      результат выполнения операции
					 *
					 * \~english
					 * @brief Method of opening a markup node
					 * @details A non-empty designation of a namespace is declared at the node
					 * if it has not yet been declared higher up the stack of the open nodes
					 * @param local local name of the node being opened
					 * @param uri   designation of the namespace of the node being opened
					 * @return      result of performing the operation
					 *
					 * \~
					 */
					bool open(const string_view local, const string_view uri = "") noexcept;
					/**
					 * \~russian
					 * @brief Метод открытия узла разметки с его объявлениями и желаемым префиксом
					 *
					 * @details Объявления и префикс подаются вместе с именем намеренно: метка
					 * узла собирается разом, и объявить связывание после открытия значит
					 * опоздать - префикс имени к тому времени уже выбран
					 *
					 * @details Метод этот нужен всякому, кто записывает узел, у какого
					 * объявления свои: без него поток записи назначает префикс сам, а
					 * поданное следом объявление ложится в метку вторым, и запись выходит с
					 * двумя объявлениями одного и того же пространства имён
					 *
					 * @param local    местное имя открываемого узла
					 * @param uri      обозначение пространства имён открываемого узла
					 * @param declares связывания префиксов, объявляемые самим узлом
					 * @param prefix   желаемый префикс имени узла, пустой - на выбор потока
					 * @return         результат выполнения операции
					 *
					 * \~english
					 * @brief Method of opening a markup node with its declarations and a desired prefix
					 * @details The declarations and the prefix are passed together with the name deliberately: the tag
					 * of a node is assembled at once, and to declare a binding after the opening means
					 * to be late — the prefix of the name has been chosen by that time
					 * @details This method is needed by everyone who writes a node having its own
					 * declarations: without it the writing stream assigns the prefix itself, while
					 * a declaration passed afterwards is placed into the tag as the second one, and the record comes out with
					 * two declarations of one and the same namespace
					 * @param local    local name of the node being opened
					 * @param uri      designation of the namespace of the node being opened
					 * @param declares bindings of the prefixes declared by the node itself
					 * @param prefix   desired prefix of the name of the node, an empty one — at the choice of the stream
					 * @return         result of performing the operation
					 *
					 * \~
					 */
					bool open(const string_view local, const string_view uri, const vector <binding_t> & declares, const string_view prefix = "") noexcept;
					/**
					 * \~russian
					 * @brief Метод закрытия последнего открытого узла
					 *
					 * @return результат выполнения операции
					 *
					 * \~english
					 * @brief Method of closing the last open node
					 * @return result of performing the operation
					 *
					 * \~
					 */
					bool close() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод записи атрибута открытого узла
					 *
					 * @warning Записывается только сразу после открытия узла, до его
					 * содержимого: дописать атрибут узлу, у которого уже есть содержимое,
					 * нельзя
					 *
					 * @param local местное имя атрибута
					 * @param value значение атрибута
					 * @param uri   обозначение пространства имён атрибута
					 * @return      результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing an attribute of an open node
					 * @warning Written only right after the opening of a node, before its
					 * content: an attribute cannot be appended to a node which already has a content
					 * @param local local name of the attribute
					 * @param value value of the attribute
					 * @param uri   designation of the namespace of the attribute
					 * @return      result of performing the operation
					 *
					 * \~
					 */
					bool attribute(const string_view local, const string_view value, const string_view uri = "") noexcept;
					/**
					 * \~russian
					 * @brief Метод объявления пространства имён при открытом узле
					 *
					 * @param prefix префикс без разделителя, пустой для объявления по умолчанию
					 * @param uri    обозначение объявляемого пространства имён
					 * @return       результат выполнения операции
					 *
					 * \~english
					 * @brief Method of declaring a namespace at an open node
					 * @param prefix prefix without the separator, empty for a default declaration
					 * @param uri    designation of the namespace being declared
					 * @return       result of performing the operation
					 *
					 * \~
					 */
					bool binding(const string_view prefix, const string_view uri) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод записи текстового содержимого
					 *
					 * @details Знаки, имеющие в разметке особый смысл, экранируются
					 *
					 * @param text записываемое текстовое содержимое
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing a text content
					 * @details The characters having a special meaning in a markup are escaped
					 * @param text text content being written
					 * @return     result of performing the operation
					 *
					 * \~
					 */
					bool text(const string_view text) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи раздела дословного текста
					 *
					 * @warning Содержимое записывается без экранирования, и завершающая
					 * раздел последовательность внутри него недопустима. Такое содержимое
					 * отвергается, а не разрезается на несколько разделов
					 *
					 * @param text записываемое дословное содержимое
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing a literal text section
					 * @warning The content is written without an escaping, and the sequence terminating
					 * the section is inadmissible inside it. Such a content
					 * is rejected rather than being cut into several sections
					 * @param text literal content being written
					 * @return     result of performing the operation
					 *
					 * \~
					 */
					bool cdata(const string_view text) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи примечания
					 *
					 * @param text содержимое примечания
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing a comment
					 * @param text content of the comment
					 * @return     result of performing the operation
					 *
					 * \~
					 */
					bool comment(const string_view text) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи указания обработчику
					 *
					 * @param target цель указания обработчику
					 * @param text   данные указания обработчику
					 * @return       результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing a processing instruction
					 * @param target target of the processing instruction
					 * @param text   data of the processing instruction
					 * @return       result of performing the operation
					 *
					 * \~
					 */
					bool processing(const string_view target, const string_view text) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод записи узла с текстовым содержимым
					 *
					 * @details Равнозначен открытию узла, записи содержимого и закрытию:
					 * такое сочетание в разметке встречается чаще прочих
					 *
					 * @param local местное имя записываемого узла
					 * @param value текстовое содержимое записываемого узла
					 * @param uri   обозначение пространства имён записываемого узла
					 * @return      результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing a node with a text content
					 * @details Equivalent to an opening of a node, a writing of the content and a closing:
					 * such a combination is met in a markup more often than the rest
					 * @param local local name of the node being written
					 * @param value text content of the node being written
					 * @param uri   designation of the namespace of the node being written
					 * @return      result of performing the operation
					 *
					 * \~
					 */
					bool element(const string_view local, const string_view value, const string_view uri = "") noexcept;
					/**
					 * \~russian
					 * @brief Метод записи дерева разметки
					 *
					 * @warning Описание типа документа пропускается: записывать его модуль
					 * не умеет вовсе, и обратный переход дерево→текст его теряет вместе с
					 * объявленными в нём сущностями и умолчаниями атрибутов. Дерево,
					 * прочитанное с таким описанием, записанным выходит без него
					 *
					 * @note Связывания пространств имён из дерева не перенимаются: префиксы
					 * назначаются заново, и записанный текст равнозначен исходному по
					 * смыслу, но не по написанию имён
					 *
					 * @note Обход дерева ведётся собственным стеком, а не рекурсией: глубина
					 * записываемого дерева ничем не ограничена, и стеку задачи она не грозит.
					 * Всякое разобранное дерево записывается обратно, с какой бы глубиной оно
					 * ни было разобрано
					 *
					 * @note Нарядная запись повторяема: отступы, поставленные прошлым её
					 * заходом, дерево принимает пробельным содержимым, и переписываются они
					 * не наравне с прочим, а расставляются заново. Содержимого запись при
					 * этом не меняет: узлы со смешанным содержимым, узлы под отведённым
					 * атрибутом `xml:space` и узлы с разделом дословного текста
					 * записываются в одну строку без отступов вовсе
					 *
					 * @param node записываемый узел дерева разметки вместе с вложенными
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing a markup tree
					 * @warning The document type definition is skipped: the module does not know how to write it
					 * at all, and the reverse transition tree→text loses it together with
					 * the entities and the default values of the attributes declared in it. A tree
					 * read with such a definition comes out written without it
					 * @note The bindings of the namespaces are not taken over from the tree: the prefixes
					 * are assigned anew, and the written text is equivalent to the source one in
					 * the meaning but not in the spelling of the names
					 * @note The traversal of the tree is conducted by an own stack rather than by a recursion: the depth
					 * of the tree being written is not limited by anything, and it does not threaten the stack of the task.
					 * Every parsed tree is written back, with whatever depth it
					 * may have been parsed
					 * @note The adorned writing is repeatable: the indents put by its previous
					 * pass the tree accepts as a whitespace content, and they are rewritten
					 * not on a par with the rest but are arranged anew. The writing does not thereby change
					 * the content: the nodes with a mixed content, the nodes under the allotted
					 * `xml:space` attribute and the nodes with a literal text section
					 * are written in a single line without the indents at all
					 * @param node node of the markup tree being written together with the nested ones
					 * @return     result of performing the operation
					 *
					 * \~
					 */
					bool element(const node_t & node) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения кода ошибки записи
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
					/**
					 * \~russian
					 * @brief Метод проверки завершённости собранного текста
					 *
					 * @details Текст завершён, когда записан корневой узел и все открытые
					 * узлы закрыты. Незавершённый текст выдавать наружу нельзя
					 *
					 * @return результат проверки
					 *
					 * \~english
					 * @brief Method of checking the completeness of the assembled text
					 * @details The text is complete when the root node has been written and all the open
					 * nodes have been closed. An incomplete text must not be issued outside
					 * @return result of the check
					 *
					 * \~
					 */
					bool complete() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения собранного текста разметки
					 *
					 * @note Запись, прекращённая ошибкой, выдаёт пустой текст: собранное к
					 * тому мигу оборвано посреди метки узла, и выдавать его наружу нельзя.
					 * Причина отказа берётся у @c error()
					 *
					 * @return собранный текст разметки в кодировке UTF-8
					 *
					 * \~english
					 * @brief Method of getting the assembled markup text
					 * @note A writing terminated by an error issues an empty text: what has been assembled by
					 * that moment is cut off in the middle of the tag of a node, and it must not be issued outside.
					 * The reason of the refusal is taken from @c error()
					 * @return assembled markup text in the UTF-8 encoding
					 *
					 * \~
					 */
					const string & text() const noexcept;
					/**
					 * \~russian
					 * @brief Метод отведения места под собираемый текст разметки
					 *
					 * @details Отводится место наперёд, чтобы рост накопителя по ходу записи
					 * не перекладывал собранное с места на место. Размер задаётся снаружи
					 * намеренно: ожидаемый объём известен тому, кто разметку собирает, а
					 * догадка изнутри была бы одинаково неверна и для узла ответа SOAP, и
					 * для описания устройства UPnP
					 *
					 * @note Вызов необязателен: без него накопитель растёт сам, удваивая
					 * своё место, отчего перекладывание обходится в удвоенный объём итога.
					 * Заниженный размер отказом не является - недостающее доводится ростом
					 * @param size ожидаемый размер собираемого текста разметки в байтах
					 *
					 *  @code{.cpp}
					 *  writer_t writer;
					 *
					 *  writer.reserve(source.size() + (source.size() / 5));
					 *  writer.declaration();
					 *  @endcode
					 *
					 * \~english
					 * @brief Method of allotting the space for the markup text being assembled
					 * @details The space is allotted in advance so that the growth of the accumulator in the course of the writing
					 * does not move what has been assembled from place to place. The size is given from the outside
					 * deliberately: the expected volume is known to the one who assembles the markup, while
					 * a guess from the inside would be equally wrong both for a node of a SOAP answer and
					 * for a description of a UPnP device
					 * @note The call is optional: without it the accumulator grows by itself, doubling
					 * its own space, from which the moving costs a doubled volume of the result.
					 * An underestimated size is not a refusal — what is missing is added by a growth
					 * @param size expected size of the markup text being assembled in bytes
					 *
					 *  @code{.cpp}
					 *  writer_t writer;
					 *
					 *  writer.reserve(source.size() + (source.size() / 5));
					 *  writer.declaration();
					 *  @endcode
					 *
					 */
					void reserve(const size_t size) noexcept;
					/**
					 * \~russian
					 * @brief Метод очистки собранного текста разметки
					 *
					 * \~english
					 * @brief Method of clearing the assembled markup text
					 *
					 * \~
					 */
					void clear() noexcept;
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
					explicit Writer(const log_t * log) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param settings настройки записи текста разметки
					 *
					 * \~english
					 * @brief Constructor
					 * @param settings settings of the writing of a markup text
					 *
					 * \~
					 */
					explicit Writer(const log_t * log, const settings_t & settings) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки объекта ведения журнала работы
					 *
					 * @param log объект ведения журнала работы
					 *
					 * \~english
					 * @brief Method of the setting of the object of the keeping of the work log
					 *
					 * @param log the object of the keeping of the work log
					 *
					 * \~
					 */
					void setLogger(const log_t * log) noexcept;
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

#endif // __AWH_CODEC_XML_WRITER__
