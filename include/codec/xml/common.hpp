/**
 * @file common.hpp
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
 * @brief Заголовочный файл общих определений контейнера XML — коды ошибок разбора, виды событий чтения,
 *        кодировки исходного текста, пределы разбора, структуры имени с пространством имён,
 *        атрибута и положения в исходном тексте
 *
 * \~english
 * @brief Header file of the common definitions of the XML container — the error codes of the parsing, the kinds of the events of the reading,
 *        the encodings of the source text, the limits of the parsing, the structures of a name with a namespace,
 *        of an attribute and of a position in the source text
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_XML_COMMON__
#define __AWH_CODEC_XML_COMMON__

/**
 * Стандартные заголовочные файлы
 */
#include <limits>
#include <cstdint>
#include <type_traits>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../sys/global.hpp"

/**
 * Снимаем на время объявлений макросы, чьи имена заняты
 * членами перечислений ниже (возвращает их macro_pop.hpp в конце файла)
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
		 * @brief Пространство имён контейнера XML
		 *
		 * @details Разбор и запись текста в разметке XML 1.0 (пятое издание) с поддержкой
		 * пространств имён по договору Namespaces in XML 1.0. Разбор проверяет правильность
		 * построения текста и не выполняет проверки на соответствие описанию типа документа
		 *
		 * @par Намеренные решения
		 *
		 * Перечисленное ниже не является пробелом реализации: это очерченные границы
		 * задачи, и каждое из решений закреплено проверочным испытанием
		 *
		 * @li **Внешние сущности не загружаются никогда.** Ни внешнее подмножество описания
		 * типа документа, ни внешние разобранные сущности, ни объявление NOTATION не влекут
		 * обращения к файловой системе или к сети. Ссылка на внешнюю сущность в тексте
		 * считается ошибкой разбора. Это закрывает целый разряд нападений через подмену
		 * сущностей, и отменить такое поведение настройкой нельзя
		 *
		 * @li **Проверка на соответствие описанию типа документа не выполняется.** Внутреннее
		 * подмножество разбирается лишь в объёме, который требует правильность построения:
		 * объявления сущностей и значения атрибутов по умолчанию. Проверка структуры
		 * документа по описанию типа, по схеме XSD или по договору RelaxNG - работа
		 * отдельная, к разбору отношения не имеющая
		 *
		 * @li **Язык обращения к узлам XPath в состав не входит.** Обращение к содержимому
		 * ведётся обходом дерева либо чтением событий. Язык обращения к узлам - самостоятельный
		 * договор со своим разбором и своим вычислителем, и место ему в отдельном модуле
		 *
		 * @li **Разметка XML 1.1 не поддерживается.** Переиздание договора распространения не
		 * получило, а его отличия - разрешение управляющих знаков в тексте и иной перечень
		 * знаков конца строки - несут больше вреда, чем пользы
		 *
		 * @li **Перечень поддерживаемых кодировок ограничен.** Договор требует UTF-8 и UTF-16,
		 * они поддержаны полностью; сверх того поддержаны ISO-8859-1 и US-ASCII как
		 * встречающиеся в старых описаниях. Прочие кодировки объявляются ошибкой, а не
		 * разбираются наугад: перекодирование - работа отдельного слоя
		 *
		 * @li **Значение атрибута xml:space, отличное от объявленных договором, ошибкой не
		 * считается.** Договор допускает при этом атрибуте лишь значения default и preserve,
		 * но задаёт их перечислением в объявлении типа документа, то есть требованием
		 * действительности, а не правильности построения. Разбор действительность не
		 * проверяет и отвергать такой текст не вправе; всё, что не preserve, принимается
		 * за обращение с пробельным содержимым по умолчанию
		 *
		 * @li **При внешнем подмножестве необъявленная сущность пропускается молча.**
		 * Внешнее подмножество описания типа документа и значение параметрической сущности
		 * разбор не разворачивает, а объявление вправе лежать именно там. Отвергнуть текст
		 * здесь значило бы объявить неправильно построенным то, чего мы попросту не читали,
		 * и потому недостача объявления отнесена к действительности. Ссылка при этом
		 * **из содержимого пропадает**: подставить нечего, а выдать её знаками текста
		 * неверно - на письме она осталась бы ссылкой и прочлась бы обратно уже разметкой.
		 * Отсюда следует, что при внешнем подмножестве выданное разбором содержимое
		 * **полным не является**, и вызывающему полагаться на его полноту нельзя. Правильность
		 * построения самой ссылки при этом проверяется по-прежнему: имя, построенное
		 * ошибочно, отвергается и здесь. Без внешнего подмножества недостача объявления
		 * остаётся ошибкой разбора
		 *
		 * \~english
		 * @brief XML container namespace
		 * @details The parsing and the writing of a text in the XML 1.0 markup (fifth edition) with support for
		 * the namespaces by the Namespaces in XML 1.0 protocol. The parsing checks the well-formedness
		 * of the construction of the text and does not perform the validation against a document type definition
		 * @par Deliberate decisions
		 * What is listed below is not a gap of the implementation: these are the outlined boundaries of the
		 * task, and each of the decisions is fixed by a verifying test
		 * @li **The external entities are never loaded.** Neither the external subset of a document
		 * type definition, nor the external parsed entities, nor a NOTATION declaration entail
		 * a call to the file system or to the network. A reference to an external entity in a text
		 * is considered a parsing error. This closes a whole class of attacks through a substitution of the
		 * entities, and such a behaviour cannot be cancelled by a setting
		 * @li **The validation against a document type definition is not performed.** The internal
		 * subset is parsed only in the volume required by the well-formedness of the construction:
		 * the entity declarations and the default values of the attributes. The validation of the structure of a
		 * document against a type definition, against an XSD schema or against the RelaxNG protocol is a separate
		 * work having no relation to the parsing
		 * @li **The XPath node addressing language is not a part of the composition.** The addressing of the content
		 * is conducted by a traversal of the tree or by a reading of the events. A node addressing language is an independent
		 * protocol with its own parsing and its own evaluator, and its place is in a separate module
		 * @li **The XML 1.1 markup is not supported.** The reissue of the protocol has not gained
		 * a wide spread, while its differences — the permission of the control characters in a text and another list
		 * of the line ending characters — carry more harm than benefit
		 * @li **The list of the supported encodings is limited.** The protocol requires UTF-8 and UTF-16,
		 * they are supported in full; beyond that ISO-8859-1 and US-ASCII are supported as
		 * the ones met in the old specifications. The other encodings are declared an error rather than being
		 * parsed at random: a transcoding is the work of a separate layer
		 * @li **A value of the xml:space attribute other than the ones declared by the protocol is not considered
		 * an error.** The protocol admits with this attribute only the values default and preserve,
		 * but it gives them by an enumeration in the document type declaration, that is, by a requirement
		 * of the validity rather than of the well-formedness. The parsing does not check the validity
		 * and has no right to reject such a text; everything that is not preserve is taken
		 * for the default treatment of the whitespace content
		 * @li **With an external subset an undeclared entity is skipped silently.**
		 * The external subset of a document type definition and the value of a parameter entity
		 * the parsing does not expand, while a declaration has the right to lie exactly there. To reject a text
		 * here would mean to declare not well-formed that which we have simply not read,
		 * and therefore a missing declaration has been assigned to the validity. The reference thereby
		 * **disappears from the content**: there is nothing to substitute, while to issue it as the characters of the text
		 * is wrong — in the writing it would remain a reference and would be read back already as a markup.
		 * It follows from this that with an external subset the content issued by the parsing
		 * **is not complete**, and the caller must not rely on its completeness. The well-formedness
		 * of the construction of the reference itself is thereby checked as before: a name constructed
		 * erroneously is rejected here as well. Without an external subset a missing declaration
		 * remains a parsing error
		 *
		 * \~
		 */
		namespace xml {
			/**
			 * \~russian
			 * @brief Наибольшая допустимая глубина вложенности узлов
			 *
			 * @details Ограничение защищает от исчерпания стека при разборе текста с
			 * искусственно наращенной вложенностью
			 *
			 * @details Тем же пределом держатся и работы ПО ГОТОВОМУ ДЕРЕВУ, возвратные
			 * по устройству: размножение владеющего значения, снятие его, запись и перенос
			 * в арену дерева. Заведение по пути глубже предела оттого отвергается
			 *
			 * @warning Предел этот, поднятый настройкою разбора, стек возвратных работ
			 *          срывает: замер по стендам дал срыв около 9 000 уровней у NetBSD при
			 *          стеке 4 МБ, около 12 000 у OpenBSD при тех же 4 МБ и около 25 000 у
			 *          macOS при 8 МБ. Поднимающему предел надлежит знать, зачем он это делает
			 *
			 * \~english
			 * @brief Largest admissible depth of the nesting of the nodes
			 * @details The restriction protects from an exhaustion of the stack at the parsing of a text with an
			 * artificially increased nesting
			 * @details By the very same limit are held the works UPON A READY TREE, recursive by their arrangement:
			 * the duplication of an owning value, its taking, its writing and its transfer into the arena of a tree.
			 * A creation by a path deeper than the limit is therefore refused
			 * @warning This limit, raised by the setting of the parsing, overflows the stack of the recursive works:
			 *          a measurement across the stands gave an overflow at about 9 000 levels at NetBSD with a stack
			 *          of 4 MB, at about 12 000 at OpenBSD with the same 4 MB and at about 25 000 at macOS with 8 MB.
			 *          Whoever raises the limit ought to know why
			 *
			 * \~
			 */
			constexpr uint32_t MAX_DEPTH = 1024;

			/**
			 * \~russian
			 * @brief Наибольшая допустимая длина имени в знаках
			 *
			 * \~english
			 * @brief Largest admissible length of a name in characters
			 *
			 * \~
			 */
			constexpr uint32_t MAX_NAME = 8192;

			/**
			 * \~russian
			 * @brief Наибольшая допустимая глубина вложенности ссылок на сущности
			 *
			 * @details Сущность вправе ссылаться на другую сущность, и глубина такой связи
			 * ограничивается независимо от общего объёма подстановки
			 *
			 * \~english
			 * @brief Largest admissible depth of the nesting of the references to the entities
			 * @details An entity has the right to refer to another entity, and the depth of such a linkage
			 * is limited independently of the total volume of the substitution
			 *
			 * \~
			 */
			constexpr uint32_t MAX_ENTITY_DEPTH = 32;

			/**
			 * \~russian
			 * @brief Наибольшее допустимое количество объявленных сущностей
			 *
			 * \~english
			 * @brief Largest admissible number of the declared entities
			 *
			 * \~
			 */
			constexpr uint32_t MAX_ENTITY_COUNT = 4096;

			/**
			 * \~russian
			 * @brief Наибольший допустимый общий объём подстановки сущностей в байтах
			 *
			 * @details Предел считается на весь документ, а не на отдельную подстановку:
			 * вложенные друг в друга сущности наращивают объём произведением, и уследить за
			 * этим можно только по общей сумме. Превышение прекращает разбор ошибкой
			 *
			 * @warning Снятие этого предела открывает многократное разрастание текста при
			 * разборе - способ исчерпать память узла подставным документом в несколько
			 * сотен байт
			 *
			 * \~english
			 * @brief Largest admissible total volume of the substitution of the entities in bytes
			 * @details The limit is counted over the whole document rather than over a separate substitution:
			 * the entities nested into one another increase the volume by a multiplication, and it is possible to keep track of
			 * this only by the total sum. An excess terminates the parsing with an error
			 * @warning The removal of this limit opens up a multiple expansion of the text at the
			 * parsing — a way to exhaust the memory of a node with a planted document of several
			 * hundred bytes
			 *
			 * \~
			 */
			constexpr uint64_t MAX_ENTITY_EXPANSION = 0x100000;

			/**
			 * \~russian
			 * @brief Наибольшее кодовое значение знака Юникода
			 *
			 * \~english
			 * @brief Largest code value of a Unicode character
			 *
			 * \~
			 */
			constexpr uint32_t MAX_CODEPOINT = 0x10FFFF;

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
			 * @brief Постоянное обозначение пространства имён «xml»
			 *
			 * @details Префикс «xml» связан с этим обозначением изначально и объявления не
			 * требует, а переопределению не подлежит
			 *
			 * \~english
			 * @brief Constant designation of the «xml» namespace
			 * @details The «xml» prefix is bound to this designation from the outset and requires no
			 * declaration, while it is not subject to a redefinition
			 *
			 * \~
			 */
			constexpr string_view XML_NAMESPACE = "http://www.w3.org/XML/1998/namespace";

			/**
			 * \~russian
			 * @brief Постоянное обозначение пространства имён объявлений «xmlns»
			 *
			 * \~english
			 * @brief Constant designation of the «xmlns» namespace of the declarations
			 *
			 * \~
			 */
			constexpr string_view XMLNS_NAMESPACE = "http://www.w3.org/2000/xmlns/";

			/**
			 * \~russian
			 * @brief Коды ошибок разбора текста разметки
			 *
			 * @details Разбор не выбрасывает исключений: признаком отказа служит код ошибки
			 * вместе с положением в исходном тексте, где отказ произошёл
			 *
			 * \~english
			 * @brief Error codes of the parsing of a markup text
			 * @details The parsing does not throw exceptions: the error code together with the position
			 * in the source text where the refusal has occurred serves as the sign of a refusal
			 *
			 * \~
			 */
			enum class error_t : uint8_t {
				NONE                    = 0x00, // Ошибок не обнаружено
				INTERNAL                = 0x01, // Внутренняя ошибка разбора
				UNEXPECTED_EOF          = 0x02, // Текст оборвался посреди разметки
				INVALID_CHARACTER       = 0x03, // Знак недопустим в разметке
				INVALID_ENCODING        = 0x04, // Последовательность байтов не отвечает объявленной кодировке
				UNSUPPORTED_ENCODING    = 0x05, // Объявленная кодировка не поддерживается
				INVALID_DECLARATION     = 0x06, // Ошибочное объявление разметки в начале текста
				UNSUPPORTED_VERSION     = 0x07, // Объявленное издание разметки не поддерживается
				INVALID_NAME            = 0x08, // Имя содержит недопустимые знаки либо пусто
				NAME_TOO_LONG           = 0x09, // Длина имени превышает допустимую
				INVALID_TAG             = 0x0A, // Ошибочное построение метки
				UNCLOSED_TAG            = 0x0B, // Метка не закрыта до конца текста
				MISMATCHED_TAG          = 0x0C, // Имя закрывающей метки не совпадает с открывающей
				UNEXPECTED_CLOSE_TAG    = 0x0D, // Закрывающая метка без соответствующей открывающей
				MULTIPLE_ROOTS          = 0x0E, // В тексте более одного корневого узла
				MISSING_ROOT            = 0x0F, // В тексте отсутствует корневой узел
				CONTENT_OUTSIDE_ROOT    = 0x10, // Текстовое содержимое вне корневого узла
				DEPTH_EXCEEDED          = 0x11, // Превышена допустимая глубина вложенности узлов
				INVALID_ATTRIBUTE       = 0x12, // Ошибочное построение атрибута
				DUPLICATE_ATTRIBUTE     = 0x13, // Атрибут с таким именем в узле уже объявлен
				UNQUOTED_ATTRIBUTE      = 0x14, // Значение атрибута не заключено в кавычки
				INVALID_REFERENCE       = 0x15, // Ошибочное построение ссылки на сущность
				UNKNOWN_ENTITY          = 0x16, // Ссылка на необъявленную сущность
				EXTERNAL_ENTITY         = 0x17, // Ссылка на внешнюю сущность, загрузка которых запрещена
				RECURSIVE_ENTITY        = 0x18, // Сущность ссылается сама на себя
				ENTITY_DEPTH_EXCEEDED   = 0x19, // Превышена допустимая глубина вложенности сущностей
				ENTITY_LIMIT_EXCEEDED   = 0x1A, // Превышен допустимый объём подстановки сущностей
				ENTITY_COUNT_EXCEEDED   = 0x1B, // Превышено допустимое количество объявленных сущностей
				INVALID_CHAR_REFERENCE  = 0x1C, // Числовая ссылка построена ошибочно либо указывает на недопустимое кодовое значение
				INVALID_COMMENT         = 0x1D, // Ошибочное построение примечания
				INVALID_CDATA           = 0x1E, // Ошибочное построение раздела дословного текста
				INVALID_PROCESSING      = 0x1F, // Ошибочное построение указания обработчику
				RESERVED_PROCESSING     = 0x20, // Имя указания обработчику отведено договором
				INVALID_DOCTYPE         = 0x21, // Ошибочное построение описания типа документа
				DOCTYPE_MISPLACED       = 0x22, // Описание типа документа расположено не на своём месте
				INVALID_PREFIX          = 0x23, // Ошибочное построение префикса пространства имён
				UNBOUND_PREFIX          = 0x24, // Префикс не связан ни с одним пространством имён
				RESERVED_PREFIX         = 0x25, // Попытка переопределить отведённый договором префикс
				INVALID_NAMESPACE       = 0x26, // Объявлению пространства имён дано недопустимое значение
				OVERFLOW_LIMIT          = 0x27, // Превышен предел, заданный настройками разбора
				ENTITY_BOUNDARY         = 0x28  // Узел разметки пересекает границу подставленной сущности
			};

			/**
			 * \~russian
			 * @brief Виды событий чтения текста разметки
			 *
			 * @details Чтение выдаёт события по мере разбора текста, не удерживая его целиком
			 *
			 * \~english
			 * @brief Kinds of the events of the reading of a markup text
			 * @details The reading issues the events as the text is parsed without holding it in full
			 *
			 * \~
			 */
			enum class event_t : uint8_t {
				NONE          = 0x00, // Событие не определено
				DECLARATION   = 0x01, // Объявление разметки в начале текста
				DOCTYPE       = 0x02, // Описание типа документа
				PROCESSING    = 0x03, // Указание обработчику
				COMMENT       = 0x04, // Примечание
				ELEMENT_OPEN  = 0x05, // Начало узла со списком его атрибутов
				ELEMENT_CLOSE = 0x06, // Конец узла
				TEXT          = 0x07, // Текстовое содержимое узла
				CDATA         = 0x08, // Раздел дословного текста
				SPACE         = 0x09, // Пробельное содержимое, не значимое для строения
				FINISH        = 0x0A  // Текст разобран до конца
			};

			/**
			 * \~russian
			 * @brief Кодировки исходного текста разметки
			 *
			 * @details Кодировка определяется по метке порядка байтов и по объявлению
			 * разметки в начале текста; при их отсутствии договор предписывает считать
			 * текст записанным в UTF-8
			 *
			 * \~english
			 * @brief Encodings of the source markup text
			 * @details The encoding is determined by the byte order mark and by the declaration
			 * of the markup at the beginning of the text; in their absence the protocol prescribes considering
			 * the text written in UTF-8
			 *
			 * \~
			 */
			enum class encoding_t : uint8_t {
				NONE     = 0x00, // Кодировка не определена
				UTF8     = 0x01, // Кодировка UTF-8
				UTF16LE  = 0x02, // Кодировка UTF-16 с обратным порядком байтов
				UTF16BE  = 0x03, // Кодировка UTF-16 с прямым порядком байтов
				LATIN1   = 0x04, // Кодировка ISO-8859-1
				ASCII    = 0x05  // Кодировка US-ASCII
			};

			/**
			 * \~russian
			 * @brief Признак самодостаточности текста разметки
			 *
			 * @details Объявляется полем «standalone» объявления разметки и сообщает, влияет
			 * ли внешнее подмножество описания типа документа на смысл текста
			 *
			 * \~english
			 * @brief Flag of the standaloneness of a markup text
			 * @details Declared by the «standalone» field of the markup declaration and reports whether
			 * the external subset of a document type definition affects the meaning of the text
			 *
			 * \~
			 */
			enum class standalone_t : uint8_t {
				NONE = 0x00, // Признак в объявлении разметки отсутствует
				YES  = 0x01, // Текст самодостаточен
				NO   = 0x02  // Смысл текста зависит от внешнего подмножества
			};

			/**
			 * \~russian
			 * @brief Вид записи собираемого текста разметки
			 *
			 * @details Разметка допускает пробельное содержимое между узлами, и запись
			 * вправе им пользоваться либо обходиться без него. Выбор здесь не
			 * оформительский: расставленные отступы делают текст удобным для чтения
			 * человеком, но добавляют в документ текстовые узлы, которых в нём не
			 * задумывалось
			 *
			 * @warning Отступы **меняют содержимое** документа: узел, к которому они
			 * применены, получает пробельное содержимое, и сличение таких документов
			 * знак в знак совпадения не даст. Там, где текст подписывается либо
			 * сличается - скажем, в договоре о подписи XML, - применима лишь плотная запись
			 *
			 * \~english
			 * @brief Form of the writing of the markup text being assembled
			 * @details A markup admits a whitespace content between the nodes, and the writing
			 * has the right to make use of it or to do without it. The choice here is not
			 * a decorative one: the arranged indents make the text convenient for a reading by
			 * a human but add into the document the text nodes which were not intended in it
			 * @warning The indents **change the content** of a document: a node to which they
			 * are applied receives a whitespace content, and a comparison of such documents
			 * character by character will give no coincidence. There where a text is signed or
			 * compared — say, in the XML signature protocol — only the dense writing is applicable
			 *
			 * \~
			 */
			enum class format_t : uint8_t {
				COMPACT = 0x00, // Плотная запись без отступов и переводов строк
				PRETTY  = 0x01  // Запись с отступами и переводами строк для удобства чтения
			};

			/**
			 * \~russian
			 * @brief Знак отступа нарядной записи
			 *
			 * @details Отступ ставится ради удобства чтения, и знак его выбирается тем,
			 * кто записанное потом читает: табуляция даёт вчетверо более лёгкий текст на
			 * той же глубине вложенности, пробелы - одинаковый вид у всякого читающего
			 *
			 * @note Отсутствие знака отступа нарядную запись не отменяет: переводы строк
			 * она расставляет по-прежнему, а вложенность ими не отмечает
			 *
			 * \~english
			 * @brief Indent character of the adorned writing
			 * @details The indent is put for the sake of the convenience of the reading, and its character is chosen by the one
			 * who then reads what has been written: a tabulation gives a four times lighter text at
			 * the same depth of the nesting, the spaces — an identical look for every reader
			 * @note The absence of an indent character does not cancel the adorned writing: it arranges the line breaks
			 * as before while not marking the nesting by them
			 *
			 * \~
			 */
			enum class separator_t : uint8_t {
				NONE   = 0x00, // Отступ не ставится вовсе
				TABS   = 0x01, // Отступ ставится знаками горизонтальной табуляции
				SPACES = 0x02  // Отступ ставится пробелами
			};

			/**
			 * \~russian
			 * @brief Обращение с пробельным содержимым внутри узла
			 *
			 * @details Задаётся отведённым договором атрибутом «xml:space» и наследуется
			 * вложенными узлами
			 *
			 * \~english
			 * @brief Treatment of the whitespace content inside a node
			 * @details Given by the «xml:space» attribute allotted by the protocol and inherited
			 * by the nested nodes
			 *
			 * \~
			 */
			enum class space_t : uint8_t {
				DEFAULT  = 0x00, // Обращение с пробелами оставлено на усмотрение читающего
				PRESERVE = 0x01  // Пробельное содержимое подлежит сохранению как есть
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
			 * @brief Положение в исходном тексте разметки
			 *
			 * @details Служит для указания места ошибки и для привязки узлов дерева к
			 * исходному тексту
			 *
			 * @note Номер строки и положение в строке считаются в знаках Юникода, а
			 * смещение - в байтах исходного текста до перекодирования
			 *
			 * \~english
			 * @brief Position in the source markup text
			 * @details Serves for indicating the place of an error and for binding the nodes of the tree to the
			 * source text
			 * @note The line number and the position in the line are counted in Unicode characters, while
			 * the offset — in the bytes of the source text before the transcoding
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Location {
				// Смещение от начала текста в байтах
				uint64_t offset;
				// Номер строки, считая с единицы
				uint32_t line;
				// Положение в строке, считая с единицы
				uint32_t column;
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
				Location() noexcept : offset(NO_OFFSET), line(0), column(0) {}
			} location_t;

			/**
			 * \~russian
			 * @brief Имя узла или атрибута с учётом пространства имён
			 *
			 * @details Имя в разметке с пространствами имён состоит из необязательного
			 * префикса и местного имени. Смысл имени задаёт не префикс, а связанное с ним
			 * обозначение пространства имён: один и тот же префикс в разных частях текста
			 * вправе обозначать разное
			 *
			 * @warning Сличать имена следует по паре из обозначения пространства имён и
			 * местного имени, а не по записи с префиксом. Устройства, отвечающие по договору
			 * UPnP, ставят префиксы кто во что горазд, и сличение по записи с префиксом на
			 * них разваливается
			 *
			 * @note Поля ссылаются на память, принадлежащую разбираемому тексту либо
			 * хранилищу имён, и живут не дольше их
			 *
			 * \~english
			 * @brief Name of a node or of an attribute with regard to the namespace
			 * @details A name in a markup with the namespaces consists of an optional
			 * prefix and a local name. The meaning of a name is given not by the prefix but by the designation of the
			 * namespace bound to it: one and the same prefix in different parts of a text
			 * has the right to designate different things
			 * @warning The names should be compared by the pair of the designation of the namespace and the
			 * local name rather than by the record with the prefix. The devices answering by the UPnP
			 * protocol put the prefixes each in its own way, and a comparison by the record with the prefix
			 * falls apart on them
			 * @note The fields refer to the memory belonging to the text being parsed or to
			 * the storage of the names, and they live no longer than they do
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Name {
				// Префикс пространства имён без разделителя
				string_view prefix;
				// Местное имя без префикса
				string_view local;
				// Обозначение связанного пространства имён
				string_view uri;
				/**
				 * \~russian
				 * @brief Метод проверки совпадения имени
				 *
				 * @param uri   обозначение пространства имён для сличения
				 * @param local местное имя для сличения
				 * @return      результат проверки
				 *
				 * \~english
				 * @brief Method of checking the coincidence of a name
				 * @param uri   designation of the namespace for the comparison
				 * @param local local name for the comparison
				 * @return      result of the check
				 *
				 * \~
				 */
				bool is(const string_view uri, const string_view local) const noexcept;
				/**
				 * \~russian
				 * @brief Оператор сравнения
				 *
				 * @param name имя для сравнения
				 * @return     результат сравнения
				 *
				 * \~english
				 * @brief Comparison operator
				 * @param name name for the comparison
				 * @return     result of the comparison
				 *
				 * \~
				 */
				bool operator == (const Name & name) const noexcept;
				/**
				 * \~russian
				 * @brief Оператор сравнения
				 *
				 * @param name имя для сравнения
				 * @return     результат сравнения
				 *
				 * \~english
				 * @brief Comparison operator
				 * @param name name for the comparison
				 * @return     result of the comparison
				 *
				 * \~
				 */
				bool operator != (const Name & name) const noexcept;
			} name_t;

			/**
			 * \~russian
			 * @brief Атрибут узла разметки
			 *
			 * @details Значение атрибута выдаётся уже приведённым к окончательному виду:
			 * ссылки на сущности подставлены, знаки конца строки приведены к единому виду,
			 * пробельные знаки заменены пробелом по правилам договора
			 *
			 * @note Объявления пространств имён - «xmlns» и «xmlns:*» - в перечне атрибутов
			 * узла не выдаются: они не описывают узел, а связывают префиксы, и доступны
			 * отдельным перечнем
			 *
			 * \~english
			 * @brief Attribute of a markup node
			 * @details The value of an attribute is issued already brought to its final form:
			 * the references to the entities are substituted, the line ending characters are brought to a single form,
			 * the whitespace characters are replaced by a space by the rules of the protocol
			 * @note The declarations of the namespaces — «xmlns» and «xmlns:*» — are not issued in the list of the attributes
			 * of a node: they do not describe the node but bind the prefixes, and they are available
			 * as a separate list
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Attribute {
				// Имя атрибута с учётом пространства имён
				name_t name;
				// Значение атрибута, приведённое к окончательному виду
				string_view value;
				// Положение атрибута в исходном тексте
				location_t location;
				// Признак того, что значение взято из объявления по умолчанию, а не из текста
				bool defaulted;
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
				Attribute() noexcept : defaulted(false) {}
			} attribute_t;

			/**
			 * \~russian
			 * @brief Связывание префикса с пространством имён
			 *
			 * @details Действует внутри узла, где объявлено, и внутри всех вложенных в него,
			 * пока не будет переопределено
			 *
			 * \~english
			 * @brief Binding of a prefix to a namespace
			 * @details Acts inside the node where it is declared and inside all the ones nested into it,
			 * until it is redefined
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Binding {
				// Префикс без разделителя, пустой для объявления по умолчанию
				string_view prefix;
				// Обозначение пространства имён, пустое для отмены связывания
				string_view uri;
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
				Binding() noexcept {}
			} binding_t;

			/**
			 * \~russian
			 * @brief Метод получения описания кода ошибки разбора
			 *
			 * @param error код ошибки разбора
			 * @return      описание кода ошибки на английском языке
			 *
			 * \~english
			 * @brief Method of getting the description of an error code of the parsing
			 * @param error error code of the parsing
			 * @return      description of the error code in the English language
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const char * message(const error_t error) noexcept;

			/**
			 * \~russian
			 * @brief Метод получения названия кодировки
			 *
			 * @param encoding кодировка исходного текста
			 * @return         общепринятое название кодировки
			 *
			 * \~english
			 * @brief Method of getting the name of an encoding
			 * @param encoding encoding of the source text
			 * @return         commonly accepted name of the encoding
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const char * name(const encoding_t encoding) noexcept;

			/**
			 * \~russian
			 * @brief Метод определения кодировки по её названию
			 *
			 * @param text название кодировки в любом регистре
			 * @return     определённая кодировка исходного текста
			 *
			 * \~english
			 * @brief Method of determining an encoding by its name
			 * @param text name of the encoding in any case
			 * @return     determined encoding of the source text
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ encoding_t encoding(const string_view text) noexcept;

			/**
			 * \~russian
			 * @brief Метод разбора целого числа со знаком из содержимого разметки
			 *
			 * @details Пробельная обвязка по краям отбрасывается, а разобрано обязано быть
			 * всё содержимое целиком: остаток за числом считается отказом. Разбор ведётся
			 * по правилам местности «C» и от установленной в приложении местности не зависит
			 *
			 * @param text   разбираемое содержимое
			 * @param result ссылка на результат разбора
			 * @return       признак успешного разбора
			 *
			 * \~english
			 * @brief Method of parsing a signed integer from the content of a markup
			 * @details The whitespace padding at the edges is discarded, while the whole content as a whole is obliged to be
			 * parsed: a remainder after the number is considered a refusal. The parsing is conducted
			 * by the rules of the «C» locale and does not depend on the locale set in the application
			 * @param text   content being parsed
			 * @param result reference to the result of the parsing
			 * @return       flag of a successful parsing
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool integer(const string_view text, int64_t & result) noexcept;

			/**
			 * \~russian
			 * @brief Метод разбора целого числа без знака из содержимого разметки
			 *
			 * @param text   разбираемое содержимое
			 * @param result ссылка на результат разбора
			 * @return       признак успешного разбора
			 *
			 * \~english
			 * @brief Method of parsing an unsigned integer from the content of a markup
			 * @param text   content being parsed
			 * @param result reference to the result of the parsing
			 * @return       flag of a successful parsing
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool integer(const string_view text, uint64_t & result) noexcept;

			/**
			 * \~russian
			 * @brief Метод разбора числа с плавающей точкой из содержимого разметки
			 *
			 * @details Разбор совпадает с разбором функции strtod в местности «C» вплоть до
			 * последнего бита мантиссы
			 *
			 * @param text   разбираемое содержимое
			 * @param result ссылка на результат разбора
			 * @return       признак успешного разбора
			 *
			 * \~english
			 * @brief Method of parsing a floating-point number from the content of a markup
			 * @details The parsing coincides with the parsing of the strtod function in the «C» locale down to
			 * the last bit of the mantissa
			 * @param text   content being parsed
			 * @param result reference to the result of the parsing
			 * @return       flag of a successful parsing
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool real(const string_view text, double & result) noexcept;

			/**
			 * \~russian
			 * @brief Метод разбора логического значения из содержимого разметки
			 *
			 * @details Признаётся запись по договору XSD: «true», «false», «1» и «0».
			 * Регистр учитывается - договор иных написаний не допускает
			 *
			 * @param text   разбираемое содержимое
			 * @param result ссылка на результат разбора
			 * @return       признак успешного разбора
			 *
			 * \~english
			 * @brief Method of parsing a logical value from the content of a markup
			 * @details The notation of the XSD protocol is recognized: «true», «false», «1» and «0».
			 * The case is taken into account — the protocol admits no other spellings
			 * @param text   content being parsed
			 * @param result reference to the result of the parsing
			 * @return       flag of a successful parsing
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool boolean(const string_view text, bool & result) noexcept;

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
			 * @brief Метод разбора числа из содержимого разметки
			 *
			 * @details Разбор ведётся с проверкой выхода за пределы запрошенного типа:
			 * значение, в тип не помещающееся, отвергается, а не усекается молча. Тип
			 * определяется запрошенным типом результата, отдельного указания не требуя
			 *
			 * @param text   разбираемое содержимое
			 * @param result ссылка на результат разбора
			 * @return       признак успешного разбора
			 *
			 * \~english
			 * @brief Method of parsing a number from the content of a markup
			 * @details The parsing is conducted with a check of going beyond the limits of the requested type:
			 * a value that does not fit into the type is rejected rather than being truncated silently. The type
			 * is determined by the requested type of the result without requiring a separate indication
			 * @param text   content being parsed
			 * @param result reference to the result of the parsing
			 * @return       flag of a successful parsing
			 *
			 * \~
			 */
			bool numeric(const string_view text, T & result) noexcept {
				/**
				 * \~russian
				 * Если запрошено логическое значение
				 *
				 * @note Сличение ведётся прежде целых чисел намеренно: логический тип
				 *       языком причислен к целым, и без этого «true» бы отвергалось
				 *
				 * \~english
				 * If a logical value has been requested
				 * @note The comparison is conducted before the integers deliberately: the logical type
				 *       is reckoned among the integer ones by the language, and without this «true» would be rejected
				 *
				 * \~
				 */
				if constexpr(is_same <T, bool>::value)
					// Выполняем разбор логического значения
					return boolean(text, result);
				/**
				 * Если запрошено число с плавающей точкой
				 */
				else if constexpr(is_floating_point <T>::value) {
					// Значение числа с плавающей точкой наибольшей точности
					double value = 0.;
					/**
					 * Если разбор числа с плавающей точкой выполнить не удалось
					 */
					if(!real(text, value))
						// Выводим признак неудачного разбора
						return false;
					/**
					 * \~russian
					 * Если разобранное число за пределы запрошенного типа выходит
					 *
					 * @note Проверка ведётся лишь для конечных значений: бесконечность
					 *       записана в исходном тексте намеренно и усечением не является
					 *
					 * \~english
					 * If the parsed number goes beyond the limits of the requested type
					 * @note The check is conducted only for the finite values: an infinity is
					 *       written in the source text deliberately and is not a truncation
					 *
					 * \~
					 */
					if((sizeof(T) < sizeof(double)) && (value == value) && ((value > 0. ? value : -value) > static_cast <double> (numeric_limits <T>::max())) &&
					   ((value > 0. ? value : -value) < numeric_limits <double>::infinity()))
						// Выводим признак неудачного разбора
						return false;
					// Запоминаем разобранное число
					result = static_cast <T> (value);
					// Выводим признак успешного разбора
					return true;
				/**
				 * Если запрошено целое число без знака
				 */
				} else if constexpr(is_unsigned <T>::value) {
					// Значение целого числа без знака наибольшей разрядности
					uint64_t value = 0;
					/**
					 * Если разбор целого числа выполнить не удалось
					 */
					if(!integer(text, value))
						// Выводим признак неудачного разбора
						return false;
					/**
					 * Если разобранное число за пределы запрошенного типа выходит
					 */
					if(value > static_cast <uint64_t> (numeric_limits <T>::max()))
						// Выводим признак неудачного разбора
						return false;
					// Запоминаем разобранное число
					result = static_cast <T> (value);
					// Выводим признак успешного разбора
					return true;
				/**
				 * Если запрошено целое число со знаком
				 */
				} else {
					// Значение целого числа со знаком наибольшей разрядности
					int64_t value = 0;
					/**
					 * Если разбор целого числа выполнить не удалось
					 */
					if(!integer(text, value))
						// Выводим признак неудачного разбора
						return false;
					/**
					 * Если разобранное число за пределы запрошенного типа выходит
					 */
					if((value > static_cast <int64_t> (numeric_limits <T>::max())) || (value < static_cast <int64_t> (numeric_limits <T>::min())))
						// Выводим признак неудачного разбора
						return false;
					// Запоминаем разобранное число
					result = static_cast <T> (value);
					// Выводим признак успешного разбора
					return true;
				}
			}
		};
	};
};

/**
 * Возвращаем макросы, снятые в начале файла
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_CODEC_XML_COMMON__
