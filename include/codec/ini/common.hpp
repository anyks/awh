/**
 * @file common.hpp
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
 * @brief Заголовочный файл общих определений контейнера INI — коды ошибок разбора, виды событий чтения,
 *        наречия записи, кодировки исходного текста, пределы разбора, структуры имени раздела,
 *        свойства, примечания и положения в исходном тексте
 *
 * \~english
 * @brief Header file of the common definitions of the INI container — the error codes of the parsing, the kinds of the events of the reading,
 *        the dialects of the writing, the encodings of the source text, the limits of the parsing, the structures of the name of a section,
 *        of a property, of a comment and of a position in the source text
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_INI_COMMON__
#define __AWH_CODEC_INI_COMMON__

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
		 * @brief Пространство имён контейнера INI
		 *
		 * @details Разбор и запись текста настроек в записи INI - разделов в квадратных
		 * скобках, свойств вида «имя = значение» и примечаний. Записи этой единого
		 * описания не существует, и разбор ведётся по наречию, выбранному настройками
		 *
		 * @par Намеренные решения
		 *
		 * Перечисленное ниже не является пробелом реализации: это очерченные границы
		 * задачи, и каждое из решений закреплено проверочным испытанием
		 *
		 * @li **Договора RFC на запись INI не существует.** Ни один документ IETF этой
		 * записи не описывает, и «соответствие договору» здесь невозможно по существу.
		 * Вместо этого поддержаны наречия, сложившиеся на деле: MS Windows, configparser
		 * языка Python, файлы описания служб systemd, настройки Git и разбор языка PHP.
		 * Наречия эти между собой **несовместимы** - расходятся и в знаках примечания, и
		 * в разделителе имени со значением, и в обращении с кавычками, - поэтому выбор
		 * наречия задаётся настройками, а не зашит. Умолчание берёт общее их пересечение
		 *
		 * @li **Примечание в конце строки значения по умолчанию не признаётся.** Наречия
		 * MS Windows и systemd считают такое примечание частью значения: запись
		 * «key=value ; текст» даёт значение «value ; текст» целиком. Признание его -
		 * настройка, а не поведение по умолчанию: обратный выбор молча отрезал бы
		 * значащую часть значения у тех, кто пишет точку с запятой в пути или в пароле
		 *
		 * @li **Тип значения не выводится.** Значение свойства всегда выдаётся
		 * последовательностью знаков, а приведение к числу либо к логическому значению
		 * делается по явному запросу. Угадывание типа порождает разночтения: запись
		 * «1.10» числом теряет разряд, а «011» разбирается то восьмеричным, то
		 * десятичным - в зависимости от разбирающего
		 *
		 * @li **Вложенность разделов не встроена.** Запись «[a.b]» разбирается на раздел
		 * и подраздел лишь при включённой настройке; иначе именем раздела считается вся
		 * последовательность знаков между скобками. Наречия расходятся и здесь: Git
		 * отделяет подраздел кавычками и учитывает его регистр, а разбор языка Python
		 * точку в имени раздела значащей не считает вовсе
		 *
		 * @li **Подстановка значений выключена по умолчанию.** Обращение вида «${имя}»
		 * либо «%(имя)s» подставляется лишь по настройке и ограничено глубиной связи и
		 * общим объёмом подстановки. Без этих пределов подставной файл настроек в
		 * несколько сотен байт исчерпывает память узла многократным разрастанием - тот
		 * же разряд нападений, что подстановка сущностей в разметке XML
		 *
		 * @li **Запись TOML отдельным наречием не считается.** Внешне она на INI похожа,
		 * но имеет собственное описание со своим набором типов, со своими таблицами и
		 * перечнями. Натягивать её на настройки разбора INI значило бы разбирать оба
		 * неверно; место ей в отдельном модуле
		 *
		 * @li **Внешние файлы не подключаются.** Указания вида «include» разбором не
		 * исполняются: ни обращения к файловой системе, ни к сети разбор не совершает.
		 * Кому такое подключение нужно, тот исполняет его сам, разбирая полученное
		 * повторно - это оставляет ему и проверку пути, и защиту от круговых ссылок
		 *
		 * \~english
		 * @brief INI container namespace
		 * @details The parsing and the writing of a settings text in the INI notation — of the sections in square
		 * brackets, of the properties of the form «name = value» and of the comments. There is no single description
		 * of this notation, and the parsing is conducted by the dialect chosen by the settings
		 * @par Deliberate decisions
		 * What is listed below is not a gap of the implementation: these are the outlined boundaries of the
		 * task, and each of the decisions is fixed by a verifying test
		 * @li **There is no RFC protocol for the INI notation.** No IETF document describes this
		 * notation, and a «conformance to the protocol» is here impossible in essence.
		 * Instead of that the dialects that have taken shape in practice are supported: MS Windows, the configparser
		 * of the Python language, the unit files of systemd, the settings of Git and the parsing of the PHP language.
		 * Those dialects are **incompatible** with one another — they diverge both in the comment characters, and
		 * in the separator of a name from a value, and in the treatment of the quotes — therefore the choice of the
		 * dialect is given by the settings rather than being hardwired. The default takes their common intersection
		 * @li **A comment at the end of a value line is not recognized by default.** The dialects
		 * of MS Windows and of systemd consider such a comment a part of the value: the record
		 * «key=value ; text» gives the value «value ; text» in full. Its recognition is
		 * a setting rather than a behaviour by default: the opposite choice would silently cut off
		 * a significant part of the value from those who write a semicolon in a path or in a password
		 * @li **The type of a value is not inferred.** The value of a property is always issued as
		 * a sequence of characters, while a conversion to a number or to a logical value
		 * is done upon an explicit request. A guessing of the type gives birth to discrepancies: the record
		 * «1.10» as a number loses a digit, while «011» is parsed now as an octal, now as a
		 * decimal one — depending on the one parsing
		 * @li **A nesting of the sections is not built in.** The record «[a.b]» is parsed into a section
		 * and a subsection only when the setting is enabled; otherwise the whole
		 * sequence of characters between the brackets is considered the name of the section. The dialects diverge here as well: Git
		 * separates a subsection with quotes and takes its case into account, while the parsing of the Python language
		 * does not consider a dot in the name of a section significant at all
		 * @li **The substitution of the values is disabled by default.** A reference of the form «${name}»
		 * or «%(name)s» is substituted only by a setting and is limited by the depth of the linkage and
		 * by the total volume of the substitution. Without those limits a planted settings file of
		 * several hundred bytes exhausts the memory of a node by a multiple expansion — the
		 * same class of attacks as the substitution of the entities in an XML markup
		 * @li **The TOML notation is not considered a separate dialect.** Outwardly it resembles INI,
		 * but it has its own specification with its own set of types, with its own tables and
		 * arrays. To stretch it onto the settings of the INI parsing would mean to parse both
		 * wrongly; its place is in a separate module
		 * @li **External files are not included.** Directives of the form «include» are not executed by
		 * the parsing: the parsing performs neither calls to the file system nor to the network.
		 * The one who needs such an inclusion executes it himself, parsing what has been obtained
		 * once more — this leaves him both the checking of the path and the protection from the circular references
		 *
		 * \~
		 */
		namespace ini {
			/**
			 * \~russian
			 * @brief Наибольшая допустимая длина логической строки в байтах
			 *
			 * @details Предел считается на строку целиком - вместе со всеми её
			 * продолжениями, - иначе продолжение строки давало бы обход предела
			 *
			 * \~english
			 * @brief Largest admissible length of a logical line in bytes
			 * @details The limit is counted over the line as a whole — together with all its
			 * continuations — otherwise a continuation of a line would give a bypass of the limit
			 *
			 * \~
			 */
			constexpr uint32_t MAX_LINE = 0x10000;

			/**
			 * \~russian
			 * @brief Наибольшая допустимая длина имени раздела или свойства в байтах
			 *
			 * \~english
			 * @brief Largest admissible length of the name of a section or of a property in bytes
			 *
			 * \~
			 */
			constexpr uint32_t MAX_NAME = 4096;

			/**
			 * \~russian
			 * @brief Наибольшая допустимая глубина вложенности подразделов
			 *
			 * \~english
			 * @brief Largest admissible depth of the nesting of the subsections
			 *
			 * \~
			 */
			constexpr uint32_t MAX_DEPTH = 64;

			/**
			 * \~russian
			 * @brief Количество пар вместилища, начиная с какого заводится указатель имён
			 *
			 * @details Ниже порога имя разыскивается перебором, и это дешевле всякого
			 * указателя: сличение имён идёт по памяти подряд, а заведение указателя стоит
			 * выделения памяти и подсчёта отпечатка на всякое имя. Выше порога перебор
			 * обращает и сборку вместилища, и чтение его по именам в квадратичные, и
			 * указатель окупается
			 *
			 * @note Указатель заводится **по требованию** - при первом обращении по имени, а
			 * не при разборе. Вместилище, к которому по имени не обращались, не платит ничего
			 *
			 * \~english
			 * @brief Number of the pairs of a container starting from which the index of the names is created
			 * @details Below this threshold a name is searched for by an enumeration, and this is cheaper than any
			 * index: above the threshold an enumeration turns both the assembly of a container and the reading of it
			 * by the names into quadratic ones, and the index pays off
			 * @note The index is created **on demand** — at the first access by a name
			 *
			 * \~
			 */
			constexpr uint32_t INDEX_THRESHOLD = 16;


			/**
			 * \~russian
			 * @brief Наибольшее допустимое количество строк продолжения у одной записи
			 *
			 * \~english
			 * @brief Largest admissible number of the continuation lines of a single record
			 *
			 * \~
			 */
			constexpr uint32_t MAX_CONTINUATION = 1024;

			/**
			 * \~russian
			 * @brief Наибольшая допустимая глубина вложенности обращений к значениям
			 *
			 * \~english
			 * @brief Largest admissible depth of the nesting of the references to the values
			 *
			 * \~
			 */
			constexpr uint32_t MAX_REFERENCE_DEPTH = 32;

			/**
			 * \~russian
			 * @brief Наибольший допустимый общий объём подстановки значений в байтах
			 *
			 * @details Предел считается на разбор целиком, а не на отдельную подстановку:
			 * вложенные друг в друга обращения наращивают объём произведением, и уследить
			 * за этим можно только по общей сумме
			 *
			 * @warning Снятие этого предела открывает многократное разрастание текста при
			 * разборе - способ исчерпать память узла подставным файлом настроек в
			 * несколько сотен байт
			 *
			 * \~english
			 * @brief Largest admissible total volume of the substitution of the values in bytes
			 * @details The limit is counted over the parsing as a whole rather than over a separate substitution:
			 * the references nested into one another increase the volume by a multiplication, and it is possible to keep track
			 * of this only by the total sum
			 * @warning The removal of this limit opens up a multiple expansion of the text at the
			 * parsing — a way to exhaust the memory of a node with a planted settings file of
			 * several hundred bytes
			 *
			 * \~
			 */
			constexpr uint64_t MAX_EXPANSION = 0x100000;

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
			 * @brief Обозначение отсутствующей записи разобранного текста
			 *
			 * \~english
			 * @brief Designation of an absent record of the parsed text
			 *
			 * \~
			 */
			constexpr uint32_t NO_RECORD = static_cast <uint32_t> (~0u);

			/**
			 * \~russian
			 * @brief Коды ошибок разбора текста настроек
			 *
			 * @details Разбор не выбрасывает исключений: признаком отказа служит код ошибки
			 * вместе с положением в исходном тексте, где отказ произошёл
			 *
			 * \~english
			 * @brief Error codes of the parsing of a settings text
			 * @details The parsing does not throw exceptions: the error code together with the position
			 * in the source text where the refusal has occurred serves as the sign of a refusal
			 *
			 * \~
			 */
			enum class error_t : uint8_t {
				NONE                  = 0x00, // Ошибок не обнаружено
				INTERNAL              = 0x01, // Внутренняя ошибка разбора
				UNEXPECTED_EOF        = 0x02, // Текст оборвался посреди записи
				INVALID_CHARACTER     = 0x03, // Знак недопустим в тексте настроек
				INVALID_ENCODING      = 0x04, // Последовательность байтов не отвечает объявленной кодировке
				UNSUPPORTED_ENCODING  = 0x05, // Объявленная кодировка не поддерживается
				INVALID_SECTION       = 0x06, // Ошибочное построение объявления раздела
				UNCLOSED_SECTION      = 0x07, // Объявление раздела не закрыто квадратной скобкой
				EMPTY_SECTION         = 0x08, // Имя раздела пусто
				DUPLICATE_SECTION     = 0x09, // Раздел с таким именем уже объявлен
				INVALID_SUBSECTION    = 0x0A, // Ошибочное построение имени подраздела
				INVALID_KEY           = 0x0B, // Имя свойства содержит недопустимые знаки
				EMPTY_KEY             = 0x0C, // Имя свойства пусто
				DUPLICATE_KEY         = 0x0D, // Свойство с таким именем в разделе уже объявлено
				NAME_TOO_LONG         = 0x0E, // Длина имени превышает допустимую
				MISSING_SEPARATOR     = 0x0F, // Строка не содержит разделителя имени и значения
				KEY_OUTSIDE_SECTION   = 0x10, // Свойство объявлено до первого раздела
				UNTERMINATED_QUOTE    = 0x11, // Значение в кавычках не закрыто до конца строки
				INVALID_ESCAPE        = 0x12, // Ошибочное построение управляющей последовательности
				UNEXPECTED_CONTENT    = 0x13, // Содержимое за закрывающей скобкой объявления раздела
				LINE_TOO_LONG         = 0x14, // Длина логической строки превышает допустимую
				DEPTH_EXCEEDED        = 0x15, // Превышена допустимая глубина вложенности подразделов
				CONTINUATION_EXCEEDED = 0x16, // Превышено допустимое количество строк продолжения
				UNKNOWN_REFERENCE     = 0x17, // Обращение к необъявленному значению
				RECURSIVE_REFERENCE   = 0x18, // Значение ссылается само на себя
				REFERENCE_DEPTH       = 0x19, // Превышена допустимая глубина вложенности обращений
				EXPANSION_EXCEEDED    = 0x1A, // Превышен допустимый объём подстановки значений
				OVERFLOW_LIMIT        = 0x1B, // Превышен предел, заданный настройками разбора
				CONFLICTING_SETTINGS  = 0x1C  // Настройки записи противоречат толкованию читающего
			};

			/**
			 * \~russian
			 * @brief Виды событий чтения текста настроек
			 *
			 * @details Чтение выдаёт события по мере разбора текста, не удерживая его целиком
			 *
			 * \~english
			 * @brief Kinds of the events of the reading of a settings text
			 * @details The reading issues the events as the text is parsed without holding it in full
			 *
			 * \~
			 */
			enum class event_t : uint8_t {
				NONE     = 0x00, // Событие не определено
				SECTION  = 0x01, // Объявление раздела
				PROPERTY = 0x02, // Свойство со значением
				COMMENT  = 0x03, // Примечание
				BLANK    = 0x04, // Пустая строка
				FINISH   = 0x05  // Текст разобран до конца, событие видно после цикла разбора
			};

			/**
			 * \~russian
			 * @brief Кодировки исходного текста настроек
			 *
			 * @details Текст настроек кодировку не объявляет, и определяется она по метке
			 * порядка байтов в начале текста; при её отсутствии текст считается записанным
			 * в UTF-8
			 *
			 * \~english
			 * @brief Encodings of the source settings text
			 * @details A settings text does not announce its encoding, and it is determined by the byte
			 * order mark at the beginning of the text; in its absence the text is considered written
			 * in UTF-8
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
			 * @brief Знаки, начинающие примечание
			 *
			 * @details Наречия расходятся: MS Windows и PHP признают лишь точку с запятой,
			 * а Git и systemd - оба знака
			 *
			 * \~english
			 * @brief Characters beginning a comment
			 * @details The dialects diverge: MS Windows and PHP recognize only the semicolon,
			 * while Git and systemd — both characters
			 *
			 * \~
			 */
			enum class marker_t : uint8_t {
				NONE      = 0x00, // Примечания не признаются вовсе
				SEMICOLON = 0x01, // Примечание начинает точка с запятой
				HASH      = 0x02, // Примечание начинает знак решётки
				BOTH      = 0x03  // Примечание начинает любой из двух знаков
			};

			/**
			 * \~russian
			 * @brief Знаки, разделяющие имя свойства и его значение
			 *
			 * @details Разбор языка Python признаёт наравне со знаком равенства двоеточие,
			 * прочие наречия - только знак равенства
			 *
			 * \~english
			 * @brief Characters separating the name of a property and its value
			 * @details The parsing of the Python language recognizes the colon on a par with the equals sign,
			 * the other dialects — only the equals sign
			 *
			 * \~
			 */
			enum class separator_t : uint8_t {
				EQUALS = 0x00, // Имя и значение разделяет знак равенства
				COLON  = 0x01, // Имя и значение разделяет двоеточие
				BOTH   = 0x02  // Имя и значение разделяет любой из двух знаков
			};

			/**
			 * \~russian
			 * @brief Обращение с повторным объявлением свойства в разделе
			 *
			 * @details Наречия расходятся и здесь: MS Windows берёт первое объявление,
			 * разбор языка Python - последнее, а Git и systemd собирают перечень значений
			 *
			 * \~english
			 * @brief Treatment of a repeated declaration of a property in a section
			 * @details The dialects diverge here as well: MS Windows takes the first declaration,
			 * the parsing of the Python language — the last one, while Git and systemd assemble a list of the values
			 *
			 * \~
			 */
			enum class duplicate_t : uint8_t {
				FIRST = 0x00, // Сохраняется первое объявленное значение
				LAST  = 0x01, // Сохраняется последнее объявленное значение
				MERGE = 0x02, // Значения собираются в перечень в порядке объявления
				ERROR = 0x03  // Повторное объявление прекращает разбор ошибкой
			};

			/**
			 * \~russian
			 * @brief Виды значения владеющего типа
			 *
			 * @details Своих видов значения у наречия INI нет вовсе: значение свойства
			 * есть последовательность знаков, а число ли это, логическое значение либо
			 * запись как она есть - решает извлечение. Перечисление это описывает не вид
			 * записи, а устройство владеющего значения: простое оно, перечнем является
			 * либо вместилищем пар
			 *
			 * @note Перечисление заведено ради владеющего значения из `value.hpp` и им
			 *       одним употребляется: разбор, запись и дерево настроек видов значения
			 *       не различают вовсе, и вводить их туда незачем
			 *
			 * \~english
			 * @brief Kinds of the value of the owning type
			 * @details The INI dialect has no kinds of a value of its own at all: the value of a property
			 * is a sequence of characters, while whether this is a number, a logical value or
			 * a record as it is — is decided by the extraction. This enumeration describes not the kind of
			 * the record but the arrangement of the owning value: whether it is a simple one, an array
			 * or a container of the pairs
			 * @note The enumeration is created for the sake of the owning value from `value.hpp` and is used
			 *       by it alone: the parsing, the writing and the settings tree do not distinguish the kinds
			 *       of a value at all, and there is no point in introducing them there
			 *
			 * \~
			 */
			enum class type_t : uint8_t {
				NONE   = 0x00, // Значение не определено
				STRING = 0x01, // Значение свойства последовательностью знаков
				ARRAY  = 0x02, // Перечень значений одноимённого свойства
				TABLE  = 0x03  // Вместилище пар: раздел, подраздел либо корень настроек
			};

			/**
			 * \~russian
			 * @brief Обращение с кавычками вокруг значения свойства
			 *
			 * \~english
			 * @brief Treatment of the quotes around the value of a property
			 *
			 * \~
			 */
			enum class quote_t : uint8_t {
				KEEP  = 0x00, // Кавычки считаются частью значения
				STRIP = 0x01  // Кавычки снимаются, а обвязка внутри них сохраняется
			};

			/**
			 * \~russian
			 * @brief Построение имени подраздела
			 *
			 * \~english
			 * @brief Construction of the name of a subsection
			 *
			 * \~
			 */
			enum class subsection_t : uint8_t {
				NONE      = 0x00, // Подразделы не выделяются, имя раздела берётся целиком
				DELIMITED = 0x01, // Подраздел отделяется знаком-разделителем: «[раздел.подраздел]»
				QUOTED    = 0x02  // Подраздел заключается в кавычки: «[раздел "подраздел"]»
			};

			/**
			 * \~russian
			 * @brief Расположение примечания в тексте настроек
			 *
			 * \~english
			 * @brief Placement of a comment in a settings text
			 *
			 * \~
			 */
			enum class placement_t : uint8_t {
				OWN    = 0x00, // Примечание занимает строку целиком
				TAIL   = 0x01, // Примечание записано в конце строки свойства
				HEADER = 0x02  // Примечание записано в конце строки объявления раздела
			};

			/**
			 * \~russian
			 * @brief Признаваемая запись логического значения
			 *
			 * \~english
			 * @brief Recognized notation of a logical value
			 *
			 * \~
			 */
			enum class boolean_t : uint8_t {
				STRICT   = 0x00, // Признаются лишь «true», «false», «1» и «0»
				EXTENDED = 0x01  // Признаются сверх того «yes», «no», «on», «off» в любом регистре
			};

			/**
			 * \~russian
			 * @brief Знак конца строки собираемого текста настроек
			 *
			 * \~english
			 * @brief Line ending character of the settings text being assembled
			 *
			 * \~
			 */
			enum class newline_t : uint8_t {
				LF   = 0x00, // Перевод строки, принятый в системах семейства UNIX
				CRLF = 0x01, // Возврат каретки с переводом строки, принятый в MS Windows
				CR   = 0x02  // Одиночный возврат каретки
			};

			/**
			 * \~russian
			 * @brief Построение обращения к значению другого свойства
			 *
			 * \~english
			 * @brief Construction of a reference to the value of another property
			 *
			 * \~
			 */
			enum class reference_t : uint8_t {
				NONE   = 0x00, // Обращения к значениям не подставляются
				SHELL  = 0x01, // Обращение записывается видом «${имя}» либо «${раздел:имя}»
				PYTHON = 0x02  // Обращение записывается видом «%(имя)s» по образцу configparser
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
			 * @brief Положение в исходном тексте настроек
			 *
			 * @details Служит для указания места ошибки и для привязки записей к исходному
			 * тексту
			 *
			 * @note Номер строки и положение в строке считаются в знаках Юникода, а
			 * смещение - в байтах исходного текста до перекодирования
			 *
			 * \~english
			 * @brief Position in the source settings text
			 * @details Serves for indicating the place of an error and for binding the records to the source
			 * text
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
			 * @brief Имя раздела текста настроек
			 *
			 * @details Имя состоит из имени раздела и необязательного имени подраздела.
			 * Разделены они бывают знаком-разделителем либо кавычками - в зависимости от
			 * наречия, - но смысл их от записи не зависит
			 *
			 * @warning Сличать имена следует по паре из раздела и подраздела, а не по
			 * записи целиком: наречие Git учитывает регистр подраздела и не учитывает
			 * регистр раздела, и сличение по записи целиком на таких именах расходится
			 *
			 * @note Поля ссылаются на память, принадлежащую разбираемому тексту либо
			 * хранилищу имён, и живут не дольше их
			 *
			 * \~english
			 * @brief Name of a section of a settings text
			 * @details The name consists of the name of the section and of an optional name of a subsection.
			 * They happen to be separated by a separator character or by quotes — depending on the
			 * dialect — but their meaning does not depend on the notation
			 * @warning The names should be compared by the pair of the section and the subsection rather than by the
			 * record as a whole: the Git dialect takes the case of a subsection into account and does not take into account
			 * the case of a section, and a comparison by the record as a whole diverges on such names
			 * @note The fields refer to the memory belonging to the text being parsed or to
			 * the storage of the names, and they live no longer than they do
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Name {
				// Имя раздела без имени подраздела
				string_view section;
				// Имя подраздела, пустое при его отсутствии
				string_view subsection;
				/**
				 * \~russian
				 * @brief Метод проверки совпадения имени
				 *
				 * @param section    имя раздела для сличения
				 * @param subsection имя подраздела для сличения
				 * @return           результат проверки
				 *
				 * \~english
				 * @brief Method of checking the coincidence of a name
				 * @param section    name of the section for the comparison
				 * @param subsection name of the subsection for the comparison
				 * @return           result of the check
				 *
				 * \~
				 */
				bool is(const string_view section, const string_view subsection = "") const noexcept;
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
			 * @brief Свойство раздела текста настроек
			 *
			 * @details Значение выдаётся уже приведённым к окончательному виду: кавычки
			 * сняты, управляющие последовательности разобраны, строки продолжения склеены,
			 * обращения к другим значениям подставлены - в объёме, разрешённом настройками
			 *
			 * \~english
			 * @brief Property of a section of a settings text
			 * @details The value is issued already brought to its final form: the quotes are
			 * removed, the escape sequences are parsed, the continuation lines are glued together,
			 * the references to the other values are substituted — in the volume permitted by the settings
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Property {
				// Имя свойства
				string_view key;
				// Значение свойства, приведённое к окончательному виду
				string_view value;
				// Положение свойства в исходном тексте
				location_t location;
				// Признак того, что значение было заключено в кавычки
				bool quoted;
				/**
				 * \~russian
				 * Признак того, что свойство записано без разделителя и значения
				 *
				 * @note Наречие Git признаёт такую запись за истину, прочие наречия
				 *       отвечают на неё отказом. Значение такого свойства пусто, и
				 *       отличить его от свойства с пустым значением можно лишь настоящим
				 *       признаком
				 *
				 * \~english
				 * Flag of the property having been written without a separator and a value
				 * @note The Git dialect recognizes such a record as truth, the other dialects
				 *       answer it with a refusal. The value of such a property is empty, and
				 *       it can be distinguished from a property with an empty value only by the present
				 *       flag
				 *
				 * \~
				 */
				bool valueless;
				/**
				 * \~russian
				 * Признак того, что свойство записано добавлением к перечню
				 *
				 * @note Запись «имя[] = значение» принята разбором языка PHP и означает
				 *       добавление к перечню значений, а не замену прежнего. Скобки в
				 *       имени при этом сняты: отличить такое свойство от обычного можно
				 *       лишь настоящим признаком
				 *
				 * \~english
				 * Flag of the property having been written as an addition to a list
				 * @note The record «name[] = value» is accepted by the parsing of the PHP language and means
				 *       an addition to a list of the values rather than a replacement of the previous one. The brackets in
				 *       the name are thereby removed: such a property can be distinguished from an ordinary one
				 *       only by the present flag
				 *
				 * \~
				 */
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
				Property() noexcept : quoted(false), valueless(false), append(false) {}
			} property_t;

			/**
			 * \~russian
			 * @brief Примечание текста настроек
			 *
			 * @details Примечания в тексте настроек пишет человек, и при перезаписи файла
			 * их надлежит сохранять: файл настроек без примечаний своему хозяину
			 * непонятен. Оттого примечание выдаётся событием со всеми своими признаками,
			 * а не отбрасывается разбором
			 *
			 * \~english
			 * @brief Comment of a settings text
			 * @details The comments in a settings text are written by a human, and at a rewriting of the file
			 * they ought to be preserved: a settings file without the comments is incomprehensible to its
			 * owner. Because of that a comment is issued as an event with all its attributes
			 * rather than being discarded by the parsing
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Comment {
				// Содержимое примечания без начального знака и без пробельной обвязки
				string_view text;
				// Знак, которым примечание начато
				char marker;
				// Расположение примечания в тексте настроек
				placement_t placement;
				// Положение примечания в исходном тексте
				location_t location;
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
				Comment() noexcept : marker(';'), placement(placement_t::OWN) {}
			} comment_t;

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
			 * @brief Метод проверки знака на признак начала примечания
			 *
			 * @param letter проверяемый знак
			 * @param marker признаваемые знаки начала примечания
			 * @return       результат проверки
			 *
			 * \~english
			 * @brief Method of checking a character for being the beginning of a comment
			 * @param letter character being checked
			 * @param marker recognized characters of the beginning of a comment
			 * @return       result of the check
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool commented(const char letter, const marker_t marker) noexcept;

			/**
			 * \~russian
			 * @brief Метод проверки знака на признак разделителя имени и значения
			 *
			 * @param letter    проверяемый знак
			 * @param separator признаваемые знаки разделителя имени и значения
			 * @return          результат проверки
			 *
			 * \~english
			 * @brief Method of checking a character for being a separator of a name and a value
			 * @param letter    character being checked
			 * @param separator recognized characters of the separator of a name and a value
			 * @return          result of the check
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool separated(const char letter, const separator_t separator) noexcept;

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
			 * @brief Метод разбора целого числа со знаком из значения свойства
			 *
			 * @details Пробельная обвязка по краям отбрасывается, а разобрано обязано быть
			 * всё значение целиком: остаток за числом считается отказом. Разбор ведётся
			 * по правилам местности «C» и от установленной в приложении местности не зависит
			 *
			 * @param text   разбираемое значение свойства
			 * @param result ссылка на результат разбора
			 * @return       признак успешного разбора
			 *
			 * \~english
			 * @brief Method of parsing a signed integer from the value of a property
			 * @details The whitespace padding at the edges is discarded, while the whole value as a whole is obliged to be
			 * parsed: a remainder after the number is considered a refusal. The parsing is conducted
			 * by the rules of the «C» locale and does not depend on the locale set in the application
			 * @param text   value of the property being parsed
			 * @param result reference to the result of the parsing
			 * @return       flag of a successful parsing
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool integer(const string_view text, int64_t & result) noexcept;

			/**
			 * \~russian
			 * @brief Метод разбора целого числа без знака из значения свойства
			 *
			 * @param text   разбираемое значение свойства
			 * @param result ссылка на результат разбора
			 * @return       признак успешного разбора
			 *
			 * \~english
			 * @brief Method of parsing an unsigned integer from the value of a property
			 * @param text   value of the property being parsed
			 * @param result reference to the result of the parsing
			 * @return       flag of a successful parsing
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool integer(const string_view text, uint64_t & result) noexcept;

			/**
			 * \~russian
			 * @brief Метод разбора числа с плавающей точкой из значения свойства
			 *
			 * @details Разбор совпадает с разбором функции strtod в местности «C» вплоть до
			 * последнего бита мантиссы
			 *
			 * @param text   разбираемое значение свойства
			 * @param result ссылка на результат разбора
			 * @return       признак успешного разбора
			 *
			 * \~english
			 * @brief Method of parsing a floating-point number from the value of a property
			 * @details The parsing coincides with the parsing of the strtod function in the «C» locale down to
			 * the last bit of the mantissa
			 * @param text   value of the property being parsed
			 * @param result reference to the result of the parsing
			 * @return       flag of a successful parsing
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool real(const string_view text, double & result) noexcept;

			/**
			 * \~russian
			 * @brief Метод разбора логического значения из значения свойства
			 *
			 * @details Записи «yes», «no», «on» и «off» признаются наравне с «true» и
			 * «false», пока не запрошена строгая запись. Расширенная запись взята
			 * умолчанием намеренно: в текстах настроек она встречается чаще строгой, и
			 * отказ на «on» удивил бы всякого, кто такой файл писал руками
			 *
			 * @param text   разбираемое значение свойства
			 * @param result ссылка на результат разбора
			 * @param forms  признаваемая запись логического значения
			 * @return       признак успешного разбора
			 *
			 * \~english
			 * @brief Method of parsing a logical value from the value of a property
			 * @details The records «yes», «no», «on» and «off» are recognized on a par with «true» and
			 * «false» until the strict notation is requested. The extended notation has been taken as the
			 * default deliberately: in the settings texts it is met more often than the strict one, and
			 * a refusal on «on» would surprise anyone who has written such a file by hand
			 * @param text   value of the property being parsed
			 * @param result reference to the result of the parsing
			 * @param forms  recognized notation of a logical value
			 * @return       flag of a successful parsing
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool boolean(const string_view text, bool & result, const boolean_t forms = boolean_t::EXTENDED) noexcept;

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
			 * @brief Метод разбора числа из значения свойства
			 *
			 * @details Отказом разбор завершается лишь тогда, когда значение числом не
			 * является вовсе. Запись значения разбору не указ: дробная запись извлекается
			 * и целым видом, а целая - и дробным. Тип определяется запрошенным типом
			 * результата, отдельного указания не требуя
			 *
			 * @details Дробное, извлекаемое целым видом, округляется по правилам математики
			 * с уводом половины от нуля: `1.5` выдаётся двойкой, а `-1.5` - минус двойкой.
			 * Целое, за отрезок затребованного вида выходящее, переносится младшими
			 * разрядами, а дробное вне его пределов выдаётся пределом: приведение такое
			 * стандарт зовёт неопределённым поведением, а неопределённого поведения в
			 * кодеке не будет
			 *
			 * @note Прежде разбор вёлся с проверкой выхода за пределы запрошенного типа, и
			 *       значение, в тип не помещающееся, отвергалось. Отменено владельцем
			 *       20.08.2026: договор извлечения общий у всех кодеков рамки, а приведение
			 *       языка не отказывает нигде
			 *
			 * @param text   разбираемое значение свойства
			 * @param result ссылка на результат разбора
			 * @param forms  признаваемая запись логического значения
			 * @return       признак успешного разбора
			 *
			 * @note Перечень поддерживаемых типов задан заранее и порождается явно в
			 * файле реализации: логический тип, целые числа со знаком и без знака
			 * разрядностью от восьми до шестидесяти четырёх, а также числа с плавающей
			 * точкой одинарной и двойной точности. Запрос иного типа отвечает отказом
			 * сборки, а не молчаливым приведением
			 *
			 * \~english
			 * @brief Method of parsing a number from the value of a property
			 * @details The parsing ends with a refusal only when the value is not a number at all.
			 * The notation of the value is not a directive to the parsing: a fractional notation is extracted
			 * also as an integer kind, and an integer one — also as a fractional. The type
			 * is determined by the requested type of the result without requiring a separate indication
			 * @details A fractional number extracted as an integer kind is rounded by the rules of mathematics
			 * with a half taken away from zero: `1.5` is issued as a two, while `-1.5` — as a minus two.
			 * An integer going beyond the range of the requested kind is carried over by the lower bits,
			 * while a fractional one beyond its limits is issued as the limit: such a conversion is called
			 * an undefined behaviour by the standard, and there will be no undefined behaviour in the codec
			 * @param text   value of the property being parsed
			 * @param result reference to the result of the parsing
			 * @param forms  recognized notation of a logical value
			 * @return       flag of a successful parsing
			 * @note The list of the supported types is given beforehand and is generated explicitly in
			 * the implementation file: the logical type, the signed and unsigned integers
			 * of a width from eight to sixty-four bits, and also the floating-point
			 * numbers of a single and a double precision. A request of another type answers with a refusal
			 * of the build rather than with a silent conversion
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool numeric(const string_view text, T & result, const boolean_t forms = boolean_t::EXTENDED) noexcept;
		};
	};
};

/**
 * Возвращаем макросы, снятые в начале файла
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_CODEC_INI_COMMON__
