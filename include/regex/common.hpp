/**
 * @file: common.hpp
 * @date: 2026-07-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл общих определений модуля регулярных выражений — коды ошибок разбора,
 *        флаги режимов компиляции, типы узлов синтаксического дерева, структуры класса символов,
 *        узла дерева и арены узлов
 *
 * \~english
 * @brief Header file of the common definitions of the regular expression module — parse error codes,
 *        compilation mode flags, syntax tree node types, structures of a character class,
 *        a tree node and the node arena
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_REGEX_COMMON__
#define __AWH_REGEX_COMMON__

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <cstdint>
#include <new>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <stdexcept>
#include <type_traits>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../sys/global.hpp"
#include "../encoding/unicode/types.hpp"

/**
 * Снимаем на время объявлений макросы, чьи имена заняты
 * членами перечислений ниже (возвращает их macro_pop.hpp в конце файла)
 */
#include "../sys/macro_push.hpp"

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
		 * @brief Индекс узла синтаксического дерева в арене
		 *
		 * \~english
		 * @brief Index of a syntax tree node in the arena
		 *
		 * \~
		 */
		using node_id_t = uint32_t;

		/**
		 * \~russian
		 * @brief Значение индекса отсутствующего узла синтаксического дерева
		 *
		 * \~english
		 * @brief Index value of a missing syntax tree node
		 *
		 * \~
		 */
		constexpr node_id_t INVALID_NODE = static_cast <node_id_t> (~0u);

		/**
		 * \~russian
		 * @brief Верхняя граница кванта, обозначающая отсутствие ограничения
		 *
		 * \~english
		 * @brief Upper bound of a quantifier denoting the absence of a limit
		 *
		 * \~
		 */
		constexpr uint32_t UNBOUNDED = static_cast <uint32_t> (~0u);

		/**
		 * \~russian
		 * @brief Наибольшее кодовое значение символа Юникода
		 *
		 * \~english
		 * @brief Largest code point value of a Unicode character
		 *
		 * \~
		 */
		constexpr uint32_t MAX_CODEPOINT = 0x10FFFF;

		/**
		 * \~russian
		 * @brief Значение индекса отсутствующего имени группы
		 *
		 * \~english
		 * @brief Index value of a missing group name
		 *
		 * \~
		 */
		constexpr uint32_t NO_NAME = static_cast <uint32_t> (~0u);

		/**
		 * \~russian
		 * @brief Коды ошибок разбора регулярного выражения
		 *
		 * \~english
		 * @brief Parse error codes of a regular expression
		 *
		 * \~
		 */
		enum class error_t : uint8_t {
			NONE                = 0x00, // Ошибок не обнаружено
			INTERNAL            = 0x01, // Внутренняя ошибка разбора
			TRAILING_BACKSLASH  = 0x02, // Обратная косая черта в конце выражения
			UNKNOWN_ESCAPE      = 0x03, // Неизвестная экранированная последовательность
			UNMATCHED_PAREN     = 0x04, // Непарная круглая скобка
			UNMATCHED_BRACKET   = 0x05, // Непарная квадратная скобка
			UNMATCHED_BRACE     = 0x06, // Непарная фигурная скобка
			BAD_QUANTIFIER      = 0x07, // Некорректный квантор повторения
			QUANTIFIER_NO_ATOM  = 0x08, // Квантор повторения без предшествующего элемента
			QUANTIFIER_TOO_BIG  = 0x09, // Значение кванта повторения превышает допустимое
			BAD_CLASS_RANGE     = 0x0A, // Некорректный диапазон в классе символов
			BAD_ESCAPE_HEX      = 0x0B, // Некорректная шестнадцатеричная последовательность
			BAD_ESCAPE_OCTAL    = 0x0C, // Некорректная восьмеричная последовательность
			BAD_PROPERTY        = 0x0D, // Неизвестное свойство Юникода
			BAD_POSIX_CLASS     = 0x0E, // Неизвестный класс символов POSIX
			BAD_GROUP_SYNTAX    = 0x0F, // Некорректный синтаксис группы
			BAD_GROUP_NAME      = 0x10, // Некорректное имя именованной группы
			DUPLICATE_NAME      = 0x11, // Повторное объявление имени группы
			BAD_BACKREFERENCE   = 0x12, // Ссылка на несуществующую группу
			BAD_CONDITION       = 0x13, // Некорректный условный шаблон
			BAD_RECURSION       = 0x14, // Некорректный рекурсивный вызов
			BAD_OPTIONS         = 0x15, // Некорректные встроенные опции
			NESTING_TOO_DEEP    = 0x16, // Превышена допустимая глубина вложенности
			PATTERN_TOO_LARGE   = 0x17, // Регулярное выражение превышает допустимый размер
			BAD_UTF8            = 0x18, // Некорректная последовательность UTF-8 в выражении
			LOOKBEHIND_INVALID  = 0x19, // Недопустимая ретроспективная проверка
			UNSUPPORTED         = 0x1A, // Конструкция не поддерживается модулем
			BUDGET_EXCEEDED     = 0x1B, // Превышен допустимый объём работы сопоставления
			NESTED_RECURSION    = 0x1C, // Повторный рекурсивный вызов в той же позиции текста
			BAD_UTF8_SUBJECT    = 0x1D  // Некорректная последовательность UTF-8 в тексте сопоставления
		};

		/**
		 * \~russian
		 * @brief Флаги режимов компиляции регулярного выражения
		 *
		 * @details Значения флагов допускают побитовое объединение. Часть флагов
		 *          может изменяться внутри выражения встроенными опциями вида «(?i)»,
		 *          такие флаги хранятся в каждом узле дерева отдельно.
		 *
		 * \~english
		 * @brief Compilation mode flags of a regular expression
		 * @details The flag values admit bitwise combination. Some of the flags
		 *          can be changed inside the expression by inline options of the «(?i)» form,
		 *          such flags are stored in every tree node separately.
		 *
		 * \~
		 */
		enum class flag_t : uint32_t {
			NONE       = 0x0000, // Режимы не установлены
			CASELESS   = 0x0001, // Сопоставление без учёта регистра символов
			MULTILINE  = 0x0002, // Привязки «^» и «$» соответствуют границам строк текста
			DOTALL     = 0x0004, // Точка соответствует любому символу, включая перевод строки
			EXTENDED   = 0x0008, // Пробельные символы выражения игнорируются, «#» начинает комментарий
			UNGREEDY   = 0x0010, // Кванторы повторения инвертируют жадность
			UTF        = 0x0020, // Текст и выражение разбираются как последовательность UTF-8
			UCP        = 0x0040, // Классы символов учитывают свойства Юникода
			ANCHORED   = 0x0080, // Сопоставление выполняется только с начала текста
			NOTEMPTY   = 0x0100, // Пустое совпадение считается отсутствием совпадения
			DOLLAR_END = 0x0200, // Привязка «$» не соответствует переводу строки в конце текста
			DUPNAMES   = 0x0400, // Допускается повторное объявление имён групп
			NOCAPTURE  = 0x0800, // Круглые скобки не образуют захватывающих групп
			ASCII      = 0x1000, // Сокращённые классы символов ограничены символами ASCII
			RESTRICT   = 0x2000, // Сопоставление без учёта регистра ограничено одной письменностью
			/**
			 * \~russian
			 * Сопоставление выполняется порождённым машинным кодом
			 *
			 * @details Режим порождает при сборке выражения сопоставитель в виде
			 *          машинного кода и исполняет им сопоставление. Кодогенерацию
			 *          получает подмножество выражений, а не всякое выражение:
			 *          непринятое сопоставляется исполнением программы, как и без
			 *          режима, - установка режима отказом сборки не оборачивается.
			 *          Режим включается сборкой явно, поскольку выигрыш его
			 *          не всеобщ: на коротких текстах расход на предварительный
			 *          отбор позиций сравним с самим сопоставлением.
			 *
			 * @warning Режим этот прежде звался «MACHINE», и имя пришлось сменить:
			 *          FreeBSD объявляет `MACHINE` макросом в `<machine/param.h>`,
			 *          подставляя вместо него строку с названием обработчика. Область
			 *          видимости перечисления от такого не защищает - препроцессор про
			 *          неё не знает и правит текст до разбора, - и сборка на FreeBSD
			 *          обрывалась отказом. Заводя здесь новые режимы, следует сличать
			 *          имя с системными макросами: `EXTENDED`, `ANCHORED`, `RESTRICT`
			 *          и прочие односложные имена подвержены тому же
			 *
			 * \~english
			 * Matching is performed by generated machine code
			 * @details The mode makes the expression build produce a matcher in the form of
			 *          machine code and performs matching with it. Code generation covers
			 *          a subset of expressions rather than every expression:
			 *          one that is not accepted is matched by executing the program, as without
			 *          the mode — setting the mode never turns into a build failure.
			 *          The mode is enabled by the build explicitly, since its gain
			 *          is not universal: on short texts the cost of preliminary
			 *          position selection is comparable to matching itself.
			 * @warning This mode used to be called «MACHINE», and the name had to be changed:
			 *          FreeBSD declares `MACHINE` a macro in `<machine/param.h>`,
			 *          substituting a string with the processor name for it. The enumeration scope
			 *          gives no protection from that — the preprocessor knows nothing about
			 *          it and edits the text before parsing — and the build on FreeBSD
			 *          ended in failure. When adding new modes here, the name should be checked
			 *          against the system macros: `EXTENDED`, `ANCHORED`, `RESTRICT`
			 *          and other single-word names are subject to the same
			 *
			 * \~
			 */
			JIT        = 0x4000,
			/**
			 * \~russian
			 * Текст сопоставления на правильность записи не проверяется
			 *
			 * @details Под режимом «UTF» текст разбирается посимвольно, и запись,
			 *          кодировке UTF-8 не отвечающая, разбору не поддаётся, поэтому
			 *          сопоставление проверяет её при всяком вызове. Проверка эта
			 *          проходит текст целиком, и проход по длинному тексту, ведомый
			 *          повторными вызовами от очередной позиции, обходится
			 *          квадратично: каждый вызов проверяет весь остаток заново.
			 *
			 *          Режим снимает проверку, возлагая правильность записи на
			 *          вызывающую сторону. Устанавливать его следует лишь там, где
			 *          текст проверен единожды снаружи, - разбор текста с неверной
			 *          записью под этим режимом выдаёт границы, символам не
			 *          отвечающие.
			 *
			 * \~english
			 * The matched text is not checked for encoding validity
			 * @details Under the «UTF» mode the text is parsed character by character, and an encoding
			 *          that does not conform to UTF-8 cannot be parsed, therefore
			 *          matching checks it on every call. That check
			 *          walks the whole text, and a pass over a long text driven
			 *          by repeated calls from the next position costs
			 *          quadratically: every call rechecks the whole remainder anew.
			 *          The mode removes the check, laying the validity of the encoding on
			 *          the calling side. It should be set only where the
			 *          text has been checked once from the outside — parsing a text with an invalid
			 *          encoding under this mode yields boundaries that do not correspond
			 *          to characters.
			 *
			 * \~
			 */
			UNCHECKED  = 0x8000
		};

		/**
		 * \~russian
		 * @brief Тип узла синтаксического дерева регулярного выражения
		 *
		 * \~english
		 * @brief Node type of the syntax tree of a regular expression
		 *
		 * \~
		 */
		enum class node_t : uint8_t {
			EMPTY       = 0x00, // Пустое выражение, соответствующее пустой строке
			LITERAL     = 0x01, // Одиночный символ
			STRING      = 0x02, // Последовательность символов, сопоставляемая целиком
			CLASS       = 0x03, // Класс символов
			ANY         = 0x04, // Любой символ с учётом режима «DOTALL»
			CODEUNIT    = 0x0E, // Одиночная единица кодирования, последовательность «\C»
			GRAPHEME    = 0x0F, // Расширенный графемный кластер, последовательность «\X»
			ANCHOR      = 0x05, // Привязка к позиции в тексте
			CONCAT      = 0x06, // Последовательное сопоставление дочерних узлов
			ALTERNATE   = 0x07, // Выбор одного из дочерних узлов
			REPEAT      = 0x08, // Повторение дочернего узла
			GROUP       = 0x09, // Группа, в том числе захватывающая
			LOOKAROUND  = 0x0A, // Опережающая или ретроспективная проверка
			BACKREF     = 0x0B, // Ссылка на ранее захваченную группу
			RECURSE     = 0x0C, // Рекурсивный вызов выражения или его группы
			CONDITION   = 0x0D  // Условное выражение
		};

		/**
		 * \~russian
		 * @brief Тип привязки к позиции в тексте
		 *
		 * \~english
		 * @brief Type of the anchor to a position in the text
		 *
		 * \~
		 */
		enum class anchor_t : uint8_t {
			TEXT_BEGIN  = 0x00, // Начало текста, последовательность «\A»
			TEXT_END    = 0x01, // Конец текста, последовательность «\z»
			TEXT_FINISH = 0x02, // Конец текста с необязательным переводом строки, последовательность «\Z»
			LINE_BEGIN  = 0x03, // Начало текста или строки, символ «^»
			LINE_END    = 0x04, // Конец текста или строки, символ «$»
			WORD_EDGE   = 0x05, // Граница слова, последовательность «\b»
			WORD_INNER  = 0x06, // Положение вне границы слова, последовательность «\B»
			SEARCH_HEAD = 0x07, // Начало текущей попытки поиска, последовательность «\G»
			KEEP_OUT    = 0x08  // Сброс начала совпадения, последовательность «\K»
		};

		/**
		 * \~russian
		 * @brief Режим жадности квантора повторения
		 *
		 * \~english
		 * @brief Greediness mode of a repetition quantifier
		 *
		 * \~
		 */
		enum class greed_t : uint8_t {
			GREEDY     = 0x00, // Наибольшее число повторений с возвратом
			LAZY       = 0x01, // Наименьшее число повторений с возвратом
			POSSESSIVE = 0x02  // Наибольшее число повторений без возврата
		};

		/**
		 * \~russian
		 * @brief Вид группы синтаксического дерева
		 *
		 * \~english
		 * @brief Kind of a syntax tree group
		 *
		 * \~
		 */
		enum class group_t : uint8_t {
			CAPTURE     = 0x00, // Захватывающая группа
			NAMED       = 0x01, // Именованная захватывающая группа
			NONCAPTURE  = 0x02, // Группа без захвата
			ATOMIC      = 0x03, // Атомарная группа, запрещающая возврат внутрь себя
			RESET       = 0x04  // Группа со сбросом нумерации ветвей, конструкция «(?|...)»
		};

		/**
		 * \~russian
		 * @brief Направление и знак проверки окружения
		 *
		 * \~english
		 * @brief Direction and sign of a lookaround check
		 *
		 * \~
		 */
		enum class look_t : uint8_t {
			AHEAD        = 0x00, // Положительная опережающая проверка
			AHEAD_NEG    = 0x01, // Отрицательная опережающая проверка
			BEHIND       = 0x02, // Положительная ретроспективная проверка
			BEHIND_NEG   = 0x03  // Отрицательная ретроспективная проверка
		};

		/**
		 * \~russian
		 * @brief Вид условия условного выражения
		 *
		 * \~english
		 * @brief Kind of the condition of a conditional expression
		 *
		 * \~
		 */
		enum class condition_t : uint8_t {
			GROUP_SET   = 0x00, // Условие выполнено, если группа с указанным номером захвачена
			NAME_SET    = 0x01, // Условие выполнено, если группа с указанным именем захвачена
			RECURSING   = 0x02, // Условие выполнено, если выполняется рекурсивный вызов
			ASSERTION   = 0x03, // Условие задано проверкой окружения
			DEFINE      = 0x04  // Условие не выполняется никогда, блок определения групп
		};

		/**
		 * \~russian
		 * @brief Идентификатор свойства Юникода
		 *
		 * @details Идентификаторы свойств заданы модулем Юникода, откуда их берут
		 *          и порождаемые таблицы базы данных символов. Обозначение заведено
		 *          затем, чтобы разбор регулярных выражений обращался к свойствам
		 *          Юникода собственным именем, не оговаривая их размещения.
		 *
		 * \~english
		 * @brief Unicode property identifier
		 * @details The property identifiers are set by the Unicode module, which is also where
		 *          the generated character database tables take them from. The designation is introduced
		 *          so that regular expression parsing refers to the Unicode properties
		 *          by their own name without stipulating their placement.
		 *
		 * \~
		 */
		using property_id_t = awh::unicode::property_id_t;

		/**
		 * \~russian
		 * @brief Диапазон кодовых значений символов класса
		 *
		 * \~english
		 * @brief Range of the code point values of the class characters
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Range {
			// Нижняя граница диапазона включительно
			uint32_t begin;
			// Верхняя граница диапазона включительно
			uint32_t end;
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
			Range() noexcept : begin(0), end(0) {}
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param begin нижняя граница диапазона включительно
			 * @param end   верхняя граница диапазона включительно
			 *
			 * \~english
			 * @brief Constructor
			 * @param begin lower bound of the range inclusive
			 * @param end   upper bound of the range inclusive
			 *
			 * \~
			 */
			Range(const uint32_t begin, const uint32_t end) noexcept : begin(begin), end(end) {}
		} range_t;

		/**
		 * \~russian
		 * @brief Ссылка на свойство Юникода в классе символов
		 *
		 * \~english
		 * @brief Reference to a Unicode property in a character class
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Property {
			// Идентификатор свойства Юникода
			uint16_t id;
			/**
			 * \~russian
			 * Флаг отрицания свойства, устанавливается для последовательности «\P»
			 *
			 * @note Хранится байтом, а не логическим значением: набор свойств
			 *       восстанавливается обзором образа памяти записи хранилища, а запись
			 *       приходит извне - подделанный байт дал бы значению логического типа
			 *       состояние, языком не отведённое
			 *
			 * \~english
			 * Property negation flag, set for the «\P» sequence
			 * @note Stored as a byte rather than as a boolean value: the set of properties
			 *       is restored by viewing the memory image of the storage record, and the record
			 *       comes from the outside — a forged byte would give a value of the boolean type
			 *       a state that the language does not provide for
			 *
			 * \~
			 */
			uint8_t negative;
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
			Property() noexcept : id(0), negative(false) {}
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param id       идентификатор свойства Юникода
			 * @param negative флаг отрицания свойства
			 *
			 * \~english
			 * @brief Constructor
			 * @param id       Unicode property identifier
			 * @param negative property negation flag
			 *
			 * \~
			 */
			Property(const uint16_t id, const bool negative) noexcept : id(id), negative(negative ? 1 : 0) {}
		} property_t;

		/**
		 * \~russian
		 * @brief Обзор непрерывного участка набора
		 *
		 * @tparam T тип записи обозреваемого набора
		 *
		 * @details Обзор ведёт указание на начало участка и количество записей
		 *          в нём, владения набором не принимая. Служит он выдаче
		 *          участков наборов сплошных, в каких хранятся диапазоны и
		 *          свойства классов символов программы.
		 *
		 * \~english
		 * @brief View of a contiguous span of a sequence
		 * @tparam T record type of the viewed sequence
		 * @details The view is driven by a pointer to the beginning of the span and the number of records
		 *          in it, taking no ownership of the sequence. It serves to yield
		 *          spans of contiguous sequences that hold the ranges and
		 *          properties of the character classes of the program.
		 *
		 * \~
		 */
		template <typename T>
		struct Span {
			// Указание на начало обозреваемого участка
			const T * records;
			// Количество записей обозреваемого участка
			size_t count;
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
			Span() noexcept : records(nullptr), count(0) {}
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param records указание на начало обозреваемого участка
			 * @param count   количество записей обозреваемого участка
			 *
			 * \~english
			 * @brief Constructor
			 * @param records pointer to the beginning of the viewed span
			 * @param count   number of records of the viewed span
			 *
			 * \~
			 */
			Span(const T * records, const size_t count) noexcept : records(records), count(count) {}
			/**
			 * \~russian
			 * @brief Метод извлечения указания на первую запись участка
			 *
			 * @return указание на первую запись обозреваемого участка
			 *
			 * \~english
			 * @brief Method of getting the pointer to the first record of the span
			 * @return pointer to the first record of the viewed span
			 *
			 * \~
			 */
			const T * begin() const noexcept { return this->records; }
			/**
			 * \~russian
			 * @brief Метод извлечения указания за последнюю запись участка
			 *
			 * @return указание за последнюю запись обозреваемого участка
			 *
			 * \~english
			 * @brief Method of getting the pointer past the last record of the span
			 * @return pointer past the last record of the viewed span
			 *
			 * \~
			 */
			const T * end() const noexcept { return (this->records + this->count); }
			/**
			 * \~russian
			 * @brief Метод извлечения количества записей участка
			 *
			 * @return количество записей обозреваемого участка
			 *
			 * \~english
			 * @brief Method of getting the number of records of the span
			 * @return number of records of the viewed span
			 *
			 * \~
			 */
			size_t size() const noexcept { return this->count; }
			/**
			 * \~russian
			 * @brief Метод проверки отсутствия записей участка
			 *
			 * @return результат проверки отсутствия записей участка
			 *
			 * \~english
			 * @brief Method of checking the absence of records of the span
			 * @return result of checking the absence of records of the viewed span
			 *
			 * \~
			 */
			bool empty() const noexcept { return (this->count == 0); }
			/**
			 * \~russian
			 * @brief Оператор извлечения записи участка по номеру
			 *
			 * @param index номер извлекаемой записи участка
			 * @return      извлечённая запись обозреваемого участка
			 *
			 * \~english
			 * @brief Operator of getting a record of the span by number
			 * @param index number of the record of the span to get
			 * @return      the obtained record of the viewed span
			 *
			 * \~
			 */
			const T & operator [] (const size_t index) const noexcept { return this->records[index]; }
		};

		/**
		 * \~russian
		 * @brief Набор записей, владеющий содержимым либо обозревающий запись
		 *
		 * @tparam T тип записи набора
		 *
		 * @details Набор ведёт себя двояко. Собираемый компиляцией, он содержимым
		 *          владеет и пополняется как обычный набор. Восстановленный из
		 *          хранилища, он обозревает участок записи, лежащей в памяти
		 *          целиком, и своего содержимого не имеет вовсе: восстановление
		 *          сводится к установке указания и количества, минуя размещение
		 *          и перенос. Всякое изменение набора обозревающего обращает его
		 *          во владеющий, перенося содержимое к себе, поэтому обзор
		 *          подмены записи допустить не может.
		 *
		 * \~english
		 * @brief Sequence of records that either owns its content or views a record
		 * @tparam T record type of the sequence
		 * @details The sequence behaves in two ways. Built by compilation, it owns its content
		 *          and is filled like an ordinary sequence. Restored from
		 *          the storage, it views a span of a record that lies in memory
		 *          as a whole, and has no content of its own at all: restoration
		 *          amounts to setting the pointer and the count, bypassing allocation
		 *          and copying. Any modification of a viewing sequence turns it
		 *          into an owning one by moving the content to itself, therefore a view
		 *          cannot allow substitution of the record.
		 *
		 * \~
		 */
		template <typename T>
		class Sequence {
			private:
				// Указание на начало записей набора
				const T * _records;
			private:
				// Количество записей набора
				uint32_t _count;
				/**
				 * \~russian
				 * Количество записей, размещённых набором
				 *
				 * @details Значение отлично от нуля, если набор содержимым
				 *          владеет, и равно нулю, если набор обозревает участок
				 *          записи. Признак владения отдельным полем не заведён
				 *          намеренно: набор входит в программу пятью полями, и
				 *          размер его сказывается на расходе сопоставления
				 *          через попадание полей программы в общую строку кэша.
				 *
				 * \~english
				 * Number of records allocated by the sequence
				 * @details The value is non-zero if the sequence owns its content
				 *          and equals zero if the sequence views a span of a
				 *          record. A separate ownership field is not introduced
				 *          deliberately: the sequence enters the program as five fields, and
				 *          its size tells on the cost of matching
				 *          through the program fields falling into a common cache line.
				 *
				 * \~
				 */
				uint32_t _capacity;
			private:
				/**
				 * \~russian
				 * @brief Метод размещения записей набора
				 *
				 * @param capacity требуемое количество размещаемых записей
				 *
				 * \~english
				 * @brief Method of allocating the records of the sequence
				 * @param capacity required number of records to allocate
				 *
				 * \~
				 */
				void grow(const size_t capacity) noexcept {
					/**
					 * Если размещённого количества записей достаёт
					 */
					if(this->_capacity >= capacity)
						// Выходим из метода размещения записей
						return;
					// Получаем требуемое количество размещаемых записей
					size_t required = (this->_capacity > 0 ? (static_cast <size_t> (this->_capacity) * 2) : 8);
					/**
					 * Если удвоения размещённого количества не достаёт
					 */
					if(required < capacity)
						// Выполняем установку требуемого количества записей
						required = capacity;
					// Выполняем размещение записей набора
					T * records = static_cast <T *> (::malloc(required * sizeof(T)));
					/**
					 * Если размещение записей набора не выполнено
					 */
					if(records == nullptr)
						// Выходим из метода размещения записей
						return;
					/**
					 * Если набор содержит записи
					 */
					if(this->_count > 0)
						// Выполняем перенос записей набора
						::memcpy(records, this->_records, (static_cast <size_t> (this->_count) * sizeof(T)));
					/**
					 * Если набор содержимым владеет
					 */
					if(this->_capacity > 0)
						// Выполняем освобождение прежних записей набора
						::free(const_cast <T *> (this->_records));
					// Выполняем установку указания на начало записей набора
					this->_records = records;
					// Выполняем установку размещённого количества записей
					this->_capacity = static_cast <uint32_t> (required);
				}
			public:
				/**
				 * \~russian
				 * @brief Метод установки обзора участка записи
				 *
				 * @param records указание на начало записей участка
				 * @param count   количество записей участка
				 *
				 * \~english
				 * @brief Method of setting a view of a span of a record
				 * @param records pointer to the beginning of the records of the span
				 * @param count   number of records of the span
				 *
				 * \~
				 */
				void attach(const T * records, const size_t count) noexcept {
					// Выполняем освобождение собственного содержимого набора
					this->clear();
					// Выполняем установку указания на начало записей набора
					this->_records = records;
					// Выполняем установку количества записей набора
					this->_count = static_cast <uint32_t> (count);
				}
				/**
				 * \~russian
				 * @brief Метод проверки владения набором своим содержимым
				 *
				 * @return результат проверки владения содержимым
				 *
				 * \~english
				 * @brief Method of checking whether the sequence owns its content
				 * @return result of checking the ownership of the content
				 *
				 * \~
				 */
				bool owned() const noexcept { return (this->_capacity > 0); }
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения количества записей набора
				 *
				 * @return количество записей набора
				 *
				 * \~english
				 * @brief Method of getting the number of records of the sequence
				 * @return number of records of the sequence
				 *
				 * \~
				 */
				size_t size() const noexcept { return static_cast <size_t> (this->_count); }
				/**
				 * \~russian
				 * @brief Метод проверки отсутствия записей набора
				 *
				 * @return результат проверки отсутствия записей набора
				 *
				 * \~english
				 * @brief Method of checking the absence of records of the sequence
				 * @return result of checking the absence of records of the sequence
				 *
				 * \~
				 */
				bool empty() const noexcept { return (this->_count == 0); }
				/**
				 * \~russian
				 * @brief Метод извлечения указания на начало записей набора
				 *
				 * @return указание на начало записей набора
				 *
				 * \~english
				 * @brief Method of getting the pointer to the beginning of the records of the sequence
				 * @return pointer to the beginning of the records of the sequence
				 *
				 * \~
				 */
				const T * data() const noexcept { return this->_records; }
				/**
				 * \~russian
				 * @brief Метод извлечения указания на первую запись набора
				 *
				 * @return указание на первую запись набора
				 *
				 * \~english
				 * @brief Method of getting the pointer to the first record of the sequence
				 * @return pointer to the first record of the sequence
				 *
				 * \~
				 */
				const T * begin() const noexcept { return this->_records; }
				/**
				 * \~russian
				 * @brief Метод извлечения указания за последнюю запись набора
				 *
				 * @return указание за последнюю запись набора
				 *
				 * \~english
				 * @brief Method of getting the pointer past the last record of the sequence
				 * @return pointer past the last record of the sequence
				 *
				 * \~
				 */
				const T * end() const noexcept { return (this->_records + this->_count); }
				/**
				 * \~russian
				 * @brief Метод извлечения последней записи набора
				 *
				 * @return последняя запись набора
				 *
				 * \~english
				 * @brief Method of getting the last record of the sequence
				 * @return last record of the sequence
				 *
				 * \~
				 */
				const T & back() const noexcept { return this->_records[this->_count - 1]; }
				/**
				 * \~russian
				 * @brief Метод извлечения первой записи набора
				 *
				 * @return первая запись набора
				 *
				 * \~english
				 * @brief Method of getting the first record of the sequence
				 * @return first record of the sequence
				 *
				 * \~
				 */
				const T & front() const noexcept { return this->_records[0]; }
				/**
				 * \~russian
				 * @brief Оператор извлечения записи набора по номеру
				 *
				 * @param index номер извлекаемой записи набора
				 * @return      извлечённая запись набора
				 *
				 * \~english
				 * @brief Operator of getting a record of the sequence by number
				 * @param index number of the record of the sequence to get
				 * @return      the obtained record of the sequence
				 *
				 * \~
				 */
				const T & operator [] (const size_t index) const noexcept { return this->_records[index]; }
				/**
				 * \~russian
				 * @brief Метод извлечения записи набора по номеру с проверкой границ
				 *
				 * @param index номер извлекаемой записи набора
				 * @return      извлечённая запись набора
				 *
				 * \~english
				 * @brief Method of getting a record of the sequence by number with bounds checking
				 * @param index number of the record of the sequence to get
				 * @return      the obtained record of the sequence
				 *
				 * \~
				 */
				const T & at(const size_t index) const {
					/**
					 * Если номер записи набору не принадлежит
					 */
					if(index >= static_cast <size_t> (this->_count))
						// Выполняем выброс исключения выхода за границы набора
						throw out_of_range("regex::Sequence::at");
					// Выводим извлечённую запись набора
					return this->_records[index];
				}
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения изменяемой записи набора по номеру
				 *
				 * @param index номер извлекаемой записи набора
				 * @return      извлечённая запись набора
				 *
				 * \~english
				 * @brief Method of getting a mutable record of the sequence by number
				 * @param index number of the record of the sequence to get
				 * @return      the obtained record of the sequence
				 *
				 * \~
				 */
				T & at(const size_t index) {
					/**
					 * Если номер записи набору не принадлежит
					 */
					if(index >= static_cast <size_t> (this->_count))
						// Выполняем выброс исключения выхода за границы набора
						throw out_of_range("regex::Sequence::at");
					// Выполняем обращение набора обозревающего во владеющий
					this->detach();
					// Выводим извлечённую запись набора
					return const_cast <T *> (this->_records)[index];
				}
				/**
				 * \~russian
				 * @brief Оператор извлечения изменяемой записи набора по номеру
				 *
				 * @param index номер извлекаемой записи набора
				 * @return      извлечённая запись набора
				 *
				 * \~english
				 * @brief Operator of getting a mutable record of the sequence by number
				 * @param index number of the record of the sequence to get
				 * @return      the obtained record of the sequence
				 *
				 * \~
				 */
				T & operator [] (const size_t index) {
					// Выполняем обращение набора обозревающего во владеющий
					this->detach();
					// Выводим извлечённую запись набора
					return const_cast <T *> (this->_records)[index];
				}
				/**
				 * \~russian
				 * @brief Метод извлечения изменяемой последней записи набора
				 *
				 * @return последняя запись набора
				 *
				 * \~english
				 * @brief Method of getting the mutable last record of the sequence
				 * @return last record of the sequence
				 *
				 * \~
				 */
				T & back() {
					// Выполняем обращение набора обозревающего во владеющий
					this->detach();
					// Выводим извлечённую запись набора
					return const_cast <T *> (this->_records)[this->_count - 1];
				}
				/**
				 * \~russian
				 * @brief Метод обращения набора обозревающего во владеющий
				 *
				 * \~english
				 * @brief Method of turning a viewing sequence into an owning one
				 *
				 * \~
				 */
				void detach() noexcept {
					/**
					 * Если набор содержимым уже владеет
					 */
					if(this->_capacity > 0)
						// Выходим из метода обращения набора
						return;
					// Получаем обозреваемый участок записи
					const T * records = this->_records;
					// Получаем количество записей обозреваемого участка
					const size_t count = static_cast <size_t> (this->_count);
					// Выполняем сброс количества записей набора
					this->_count = 0;
					// Выполняем сброс указания на начало записей набора
					this->_records = nullptr;
					// Выполняем размещение записей набора
					this->grow(count > 0 ? count : 1);
					/**
					 * Если размещение записей набора выполнено
					 */
					if((this->_capacity > 0) && (count > 0)) {
						// Выполняем перенос обозреваемого содержимого к себе
						::memcpy(const_cast <T *> (this->_records), records, (count * sizeof(T)));
						// Выполняем установку количества записей набора
						this->_count = static_cast <uint32_t> (count);
					}
				}
				/**
				 * \~russian
				 * @brief Метод добавления записи в конец набора
				 *
				 * @param record добавляемая запись набора
				 *
				 * \~english
				 * @brief Method of appending a record to the end of the sequence
				 * @param record record of the sequence to append
				 *
				 * \~
				 */
				void push_back(const T & record) {
					// Выполняем размещение записи в конце набора
					this->emplace_back(record);
				}
				/**
				 * \~russian
				 * @brief Метод размещения записи в конце набора
				 *
				 * @tparam Args типы параметров размещаемой записи
				 * @param  args параметры размещаемой записи набора
				 *
				 * \~english
				 * @brief Method of constructing a record at the end of the sequence
				 * @tparam Args parameter types of the record to construct
				 * @param  args parameters of the record of the sequence to construct
				 *
				 * \~
				 */
				template <typename... Args>
				void emplace_back(Args &&... args) {
					// Выполняем обращение набора обозревающего во владеющий
					this->detach();
					// Выполняем размещение записей набора
					this->grow(static_cast <size_t> (this->_count) + 1);
					/**
					 * Если размещения записей набора не достаёт
					 */
					if(this->_capacity <= this->_count)
						// Выходим из метода размещения записи
						return;
					// Выполняем размещение записи в конце набора
					new (const_cast <T *> (this->_records) + this->_count) T(forward <Args> (args)...);
					// Выполняем увеличение количества записей набора
					this->_count++;
				}
				/**
				 * \~russian
				 * @brief Метод добавления участка записей в конец набора
				 *
				 * @param records указание на начало добавляемых записей
				 * @param count   количество добавляемых записей
				 *
				 * \~english
				 * @brief Method of appending a span of records to the end of the sequence
				 * @param records pointer to the beginning of the records to append
				 * @param count   number of records to append
				 *
				 * \~
				 */
				void append(const T * records, const size_t count) {
					// Выполняем обращение набора обозревающего во владеющий
					this->detach();
					/**
					 * Если добавляемый участок записей пуст
					 */
					if(count == 0)
						// Выходим из метода добавления участка записей
						return;
					// Выполняем размещение записей набора
					this->grow(static_cast <size_t> (this->_count) + count);
					/**
					 * Если размещения записей набора не достаёт
					 */
					if(static_cast <size_t> (this->_capacity) < (static_cast <size_t> (this->_count) + count))
						// Выходим из метода добавления участка записей
						return;
					// Выполняем перенос добавляемого участка записей
					::memcpy((const_cast <T *> (this->_records) + this->_count), records, (count * sizeof(T)));
					// Выполняем увеличение количества записей набора
					this->_count = static_cast <uint32_t> (static_cast <size_t> (this->_count) + count);
				}
				/**
				 * \~russian
				 * @brief Метод изменения количества записей набора
				 *
				 * @param count требуемое количество записей набора
				 *
				 * \~english
				 * @brief Method of changing the number of records of the sequence
				 * @param count required number of records of the sequence
				 *
				 * \~
				 */
				void resize(const size_t count) {
					// Выполняем обращение набора обозревающего во владеющий
					this->detach();
					/**
					 * Если количество записей набора сокращается
					 */
					if(count <= static_cast <size_t> (this->_count)) {
						// Выполняем установку количества записей набора
						this->_count = static_cast <uint32_t> (count);
						// Выходим из метода изменения количества записей
						return;
					}
					// Выполняем размещение записей набора
					this->grow(count);
					/**
					 * Если размещения записей набора не достаёт
					 */
					if(static_cast <size_t> (this->_capacity) < count)
						// Выходим из метода изменения количества записей
						return;
					/**
					 * Выполняем размещение добавляемых записей набора
					 */
					for(size_t i = static_cast <size_t> (this->_count); i < count; i++)
						// Выполняем размещение очередной записи набора
						new (const_cast <T *> (this->_records) + i) T();
					// Выполняем установку количества записей набора
					this->_count = static_cast <uint32_t> (count);
				}
				/**
				 * \~russian
				 * @brief Метод предварительного размещения записей набора
				 *
				 * @param count количество размещаемых записей набора
				 *
				 * \~english
				 * @brief Method of pre-allocating the records of the sequence
				 * @param count number of records of the sequence to allocate
				 *
				 * \~
				 */
				void reserve(const size_t count) {
					// Выполняем обращение набора обозревающего во владеющий
					this->detach();
					// Выполняем размещение записей набора
					this->grow(count);
				}
				/**
				 * \~russian
				 * @brief Метод замещения содержимого набора повторением записи
				 *
				 * @param count  количество записей набора
				 * @param record замещающая запись набора
				 *
				 * \~english
				 * @brief Method of replacing the content of the sequence with a repeated record
				 * @param count  number of records of the sequence
				 * @param record replacing record
				 *
				 * \~
				 */
				void assign(const size_t count, const T & record) {
					// Выполняем очистку набора записей
					this->clear();
					// Выполняем размещение записей набора
					this->grow(count);
					/**
					 * Если размещения записей набора не достаёт
					 */
					if(static_cast <size_t> (this->_capacity) < count)
						// Выходим из метода замещения содержимого набора
						return;
					/**
					 * Выполняем размещение записей набора
					 */
					for(size_t i = 0; i < count; i++)
						// Выполняем размещение очередной записи набора
						new (const_cast <T *> (this->_records) + i) T(record);
					// Выполняем установку количества записей набора
					this->_count = static_cast <uint32_t> (count);
				}
				/**
				 * \~russian
				 * @brief Метод очистки набора записей
				 *
				 * \~english
				 * @brief Method of clearing the sequence of records
				 *
				 * \~
				 */
				void clear() noexcept {
					/**
					 * Если набор содержимым владеет
					 */
					if(this->_capacity > 0)
						// Выполняем освобождение записей набора
						::free(const_cast <T *> (this->_records));
					// Выполняем сброс указания на начало записей набора
					this->_records = nullptr;
					// Выполняем сброс количества записей набора
					this->_count = 0;
					// Выполняем сброс размещённого количества записей
					this->_capacity = 0;
				}
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
				Sequence() noexcept : _records(nullptr), _count(0), _capacity(0) {
					/**
					 * Набор пригоден лишь записям, перенос каких выполняется
					 * переносом памяти: содержимое его переносится, размещается
					 * и освобождается без вызова конструкторов копирования и
					 * деструкторов.
					 */
					static_assert(is_trivially_copyable <T> ::value, "regex::Sequence: тип записи переносу памятью не поддаётся");
					static_assert(is_trivially_destructible <T> ::value, "regex::Sequence: тип записи разрушения требует");
				}
				/**
				 * \~russian
				 * @brief Конструктор копирования
				 *
				 * @param sequence копируемый набор записей
				 *
				 * \~english
				 * @brief Copy constructor
				 * @param sequence sequence of records to copy
				 *
				 * \~
				 */
				Sequence(const Sequence & sequence) noexcept : _records(sequence._records), _count(sequence._count), _capacity(0) {
					/**
					 * Если копируемый набор содержимым владеет
					 */
					if(sequence._capacity > 0)
						// Выполняем обращение набора обозревающего во владеющий
						this->detach();
				}
				/**
				 * \~russian
				 * @brief Оператор присваивания копированием
				 *
				 * @param sequence копируемый набор записей
				 * @return         текущий набор записей
				 *
				 * \~english
				 * @brief Copy assignment operator
				 * @param sequence sequence of records to copy
				 * @return         the current sequence of records
				 *
				 * \~
				 */
				Sequence & operator = (const Sequence & sequence) noexcept {
					/**
					 * Если присваивание выполняется самому себе
					 */
					if(this == &sequence)
						// Выводим текущий набор записей
						return (* this);
					// Выполняем очистку набора записей
					this->clear();
					// Выполняем установку указания на начало записей набора
					this->_records = sequence._records;
					// Выполняем установку количества записей набора
					this->_count = sequence._count;
					/**
					 * Если копируемый набор содержимым владеет
					 */
					if(sequence._capacity > 0)
						// Выполняем обращение набора обозревающего во владеющий
						this->detach();
					// Выводим текущий набор записей
					return (* this);
				}
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
				~Sequence() noexcept {
					// Выполняем очистку набора записей
					this->clear();
				}
		};

		/**
		 * \~russian
		 * @brief Класс символов регулярного выражения
		 *
		 * @details Класс символов хранится в виде отсортированного набора непересекающихся
		 *          диапазонов кодовых значений и набора ссылок на свойства Юникода. Отрицание
		 *          класса сохраняется отдельным флагом и применяется к объединению диапазонов
		 *          и свойств целиком.
		 *
		 *          Вид этот служит сборке класса разбором выражения. Программа
		 *          скомпилированная хранит классы иначе - сплошными наборами
		 *          диапазонов и свойств вместе с набором ссылок на участки, -
		 *          и выдаёт их обзором.
		 *
		 * \~english
		 * @brief Character class of a regular expression
		 * @details A character class is stored as a sorted sequence of non-overlapping
		 *          ranges of code point values and a sequence of references to Unicode properties. The negation
		 *          of the class is kept as a separate flag and is applied to the union of the ranges
		 *          and the properties as a whole.
		 *          This form serves to build the class by parsing the expression. A compiled
		 *          program stores the classes differently — as contiguous sequences of
		 *          ranges and properties together with a sequence of references to spans —
		 *          and yields them as a view.
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Class {
			// Флаг отрицания класса символов
			bool negative;
			// Набор диапазонов кодовых значений символов
			vector <range_t> ranges;
			// Набор ссылок на свойства Юникода
			vector <property_t> properties;
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
			Class() noexcept : negative(false) {}
		} class_t;

		/**
		 * \~russian
		 * @brief Ссылка на класс символов программы
		 *
		 * @details Программа хранит диапазоны и свойства всех классов сплошными
		 *          наборами, а класс задаёт ссылкой на участки этих наборов.
		 *          Устройство это заведено взамен набора классов, где каждый
		 *          нёс свои наборы: классов в программе десятки тысяч, и
		 *          размещение под каждый двух наборов отдельных обходилось
		 *          дороже всей прочей сборки программы вместе взятой.
		 *
		 * \~english
		 * @brief Reference to a character class of the program
		 * @details The program stores the ranges and properties of all classes as contiguous
		 *          sequences, and defines a class by a reference to spans of those sequences.
		 *          This arrangement was introduced in place of a sequence of classes where each one
		 *          carried its own sequences: there are tens of thousands of classes in a program, and
		 *          allocating two separate sequences for each one cost
		 *          more than all the rest of building the program taken together.
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ ClassRef {
			/**
			 * \~russian
			 * Флаг отрицания класса символов
			 *
			 * @note Хранится байтом, а не логическим значением: набор классов
			 *       восстанавливается обзором образа памяти записи хранилища, а запись
			 *       приходит извне - подделанный байт дал бы значению логического типа
			 *       состояние, языком не отведённое
			 *
			 * \~english
			 * Character class negation flag
			 * @note Stored as a byte rather than as a boolean value: the set of classes
			 *       is restored by viewing the memory image of the storage record, and the record
			 *       comes from the outside — a forged byte would give a value of the boolean type
			 *       a state that the language does not provide for
			 *
			 * \~
			 */
			uint8_t negative;
			// Номер первого диапазона класса в наборе диапазонов программы
			uint32_t ranges;
			// Количество диапазонов класса символов
			uint32_t rangeCount;
			// Номер первого свойства класса в наборе свойств программы
			uint32_t properties;
			// Количество свойств Юникода класса символов
			uint32_t propertyCount;
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
			ClassRef() noexcept : negative(false), ranges(0), rangeCount(0), properties(0), propertyCount(0) {}
		} classref_t;

		/**
		 * \~russian
		 * @brief Обзор класса символов программы
		 *
		 * @details Обзор выдаётся программой и владения наборами не принимает:
		 *          он действителен, пока действительна сама программа.
		 *
		 * \~english
		 * @brief View of a character class of the program
		 * @details The view is yielded by the program and takes no ownership of the sequences:
		 *          it is valid as long as the program itself is valid.
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ ClassView {
			// Флаг отрицания класса символов
			bool negative;
			// Обзор набора диапазонов кодовых значений символов
			Span <range_t> ranges;
			// Обзор набора ссылок на свойства Юникода
			Span <property_t> properties;
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
			ClassView() noexcept : negative(false) {}
		} classview_t;

		/**
		 * \~russian
		 * @brief Узел синтаксического дерева регулярного выражения
		 *
		 * @details Узлы хранятся в арене и связываются между собой индексами. Значение
		 *          отдельных полей определяется типом узла: поля объединены в анонимное
		 *          объединение для узлов, хранящих несовместимые между собой данные.
		 *
		 * \~english
		 * @brief Node of the syntax tree of a regular expression
		 * @details The nodes are stored in an arena and are linked to each other by indices. The meaning
		 *          of the individual fields is determined by the node type: the fields are combined into an anonymous
		 *          union for the nodes that hold data incompatible with each other.
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Node {
			// Тип узла синтаксического дерева
			node_t type;
			// Набор режимов компиляции, действующих для данного узла
			uint32_t flags;
			// Индекс первого дочернего узла в арене
			node_id_t child;
			// Индекс следующего узла того же уровня вложенности
			node_id_t next;
			// Смещение узла в тексте регулярного выражения
			uint32_t offset;
			/**
			 * \~russian
			 * @brief Данные узла, определяемые его типом
			 *
			 * \~english
			 * @brief Node data determined by its type
			 *
			 * \~
			 */
			union {
				/**
				 * \~russian
				 * @brief Данные узла одиночного символа
				 *
				 * \~english
				 * @brief Data of a single character node
				 *
				 * \~
				 */
				struct {
					// Кодовое значение символа
					uint32_t code;
				} literal;
				/**
				 * \~russian
				 * @brief Данные узла последовательности символов
				 *
				 * \~english
				 * @brief Data of a character sequence node
				 *
				 * \~
				 */
				struct {
					// Смещение начала последовательности в хранилище строк
					uint32_t offset;
					// Длина последовательности в кодовых значениях
					uint32_t length;
				} string;
				/**
				 * \~russian
				 * @brief Данные узла класса символов
				 *
				 * \~english
				 * @brief Data of a character class node
				 *
				 * \~
				 */
				struct {
					// Индекс класса символов в хранилище классов
					uint32_t index;
				} charclass;
				/**
				 * \~russian
				 * @brief Данные узла привязки к позиции в тексте
				 *
				 * \~english
				 * @brief Data of a text position anchor node
				 *
				 * \~
				 */
				struct {
					// Тип привязки к позиции в тексте
					anchor_t type;
				} anchor;
				/**
				 * \~russian
				 * @brief Данные узла повторения
				 *
				 * \~english
				 * @brief Data of a repetition node
				 *
				 * \~
				 */
				struct {
					// Наименьшее число повторений
					uint32_t min;
					// Наибольшее число повторений либо значение «UNBOUNDED»
					uint32_t max;
					// Режим жадности квантора повторения
					greed_t greed;
				} repeat;
				/**
				 * \~russian
				 * @brief Данные узла группы
				 *
				 * \~english
				 * @brief Data of a group node
				 *
				 * \~
				 */
				struct {
					// Вид группы синтаксического дерева
					group_t type;
					// Номер захватывающей группы, отсчитываемый с единицы
					uint32_t number;
					// Индекс имени группы в хранилище имён
					uint32_t name;
				} group;
				/**
				 * \~russian
				 * @brief Данные узла проверки окружения
				 *
				 * \~english
				 * @brief Data of a lookaround node
				 *
				 * \~
				 */
				struct {
					// Направление и знак проверки окружения
					look_t type;
					// Наименьшая длина проверяемой последовательности
					uint32_t min;
					// Наибольшая длина проверяемой последовательности
					uint32_t max;
				} look;
				/**
				 * \~russian
				 * @brief Данные узла ссылки на захваченную группу
				 *
				 * \~english
				 * @brief Data of a captured group reference node
				 *
				 * \~
				 */
				struct {
					// Номер группы, отсчитываемый с единицы
					uint32_t number;
					// Индекс имени группы в хранилище имён
					uint32_t name;
				} backref;
				/**
				 * \~russian
				 * @brief Данные узла рекурсивного вызова
				 *
				 * \~english
				 * @brief Data of a recursive call node
				 *
				 * \~
				 */
				struct {
					// Номер вызываемой группы либо нуль для вызова выражения целиком
					uint32_t number;
					// Индекс имени вызываемой группы в хранилище имён
					uint32_t name;
				} recurse;
				/**
				 * \~russian
				 * @brief Данные узла условного выражения
				 *
				 * \~english
				 * @brief Data of a conditional expression node
				 *
				 * \~
				 */
				struct {
					// Вид условия условного выражения
					condition_t type;
					// Номер проверяемой группы, отсчитываемый с единицы
					uint32_t number;
					// Индекс имени проверяемой группы в хранилище имён
					uint32_t name;
				} condition;
			};
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
			 type(node_t::EMPTY), flags(0), child(INVALID_NODE),
			 next(INVALID_NODE), offset(0), literal{0} {}
		} node_data_t;
	};
};

/**
 * Возвращаем макросы, снятые в начале файла
 */
#include "../sys/macro_pop.hpp"

#endif // __AWH_REGEX_COMMON__
