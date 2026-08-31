/**
 * @file program.hpp
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
 * @brief Заголовочный файл представления скомпилированного регулярного выражения —
 *        набор инструкций недетерминированного конечного автомата, структура инструкции
 *        и структура программы с хранилищами классов символов и последовательностей
 *
 * \~english
 * @brief Header file of the representation of a compiled regular expression —
 *        the instruction set of the nondeterministic finite automaton, the structure of an instruction
 *        and the structure of the program with the storages of character classes and sequences
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_REGEX_PROGRAM__
#define __AWH_REGEX_PROGRAM__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <memory>
#include <vector>
#include <cstdint>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"
#include "prefilter.hpp"

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
		 * @brief Адрес инструкции программы
		 *
		 * \~english
		 * @brief Address of a program instruction
		 *
		 * \~
		 */
		using address_t = uint32_t;

		/**
		 * \~russian
		 * @brief Значение адреса отсутствующей инструкции программы
		 *
		 * \~english
		 * @brief Address value of a missing program instruction
		 *
		 * \~
		 */
		constexpr address_t INVALID_ADDRESS = static_cast <address_t> (~0u);

		/**
		 * \~russian
		 * @brief Наибольшее допустимое количество инструкций программы
		 *
		 * \~english
		 * @brief Largest admissible number of program instructions
		 *
		 * \~
		 */
		constexpr size_t MAX_PROGRAM = 0x40000;

		/**
		 * \~russian
		 * @brief Код операции инструкции программы
		 *
		 * \~english
		 * @brief Operation code of a program instruction
		 *
		 * \~
		 */
		enum class opcode_t : uint8_t {
			CHAR     = 0x00, // Сопоставление одиночного символа
			CLASS    = 0x02, // Сопоставление символа из класса символов
			ANY      = 0x03, // Сопоставление любого символа с учётом режима «DOTALL»
			CODEUNIT = 0x04, // Сопоставление одиночной единицы кодирования
			SPLIT    = 0x05, // Переход по двум ветвям в порядке убывания приоритета
			JUMP     = 0x06, // Безусловный переход
			SAVE     = 0x07, // Сохранение позиции в ячейке захвата
			ANCHOR   = 0x08, // Проверка привязки к позиции в тексте
			MATCH    = 0x09, // Завершение сопоставления с успехом
			KEEP     = 0x0A, // Сброс начала совпадения в текущую позицию
			MARK     = 0x0B, // Запоминание состояния возврата в ячейке отметки
			CUT      = 0x0C, // Отказ от точек возврата, накопленных после отметки
			BACKREF  = 0x0D, // Сопоставление текста, захваченного группой
			PROGRESS = 0x0E, // Проверка продвижения по тексту в пределах повторения
			LOOK     = 0x0F, // Проверка окружения позиции сопоставления
			CALL     = 0x10, // Рекурсивный вызов подвыражения
			RETURN   = 0x11, // Завершение рекурсивного вызова подвыражения
			CONDITION = 0x12, // Переход по ветвям условного выражения
			RESUME    = 0x13, // Возврат из рекурсивного вызова подвыражения
			GRAPHEME  = 0x14, // Сопоставление расширенного графемного кластера
			ACCEPT    = 0x15, // Завершение сопоставления с успехом в текущей позиции
			CONTROL   = 0x16  // Размещение точки возврата глагола управления
		};

		/**
		 * \~russian
		 * @brief Тип условия условного выражения
		 *
		 * \~english
		 * @brief Type of the condition of a conditional expression
		 *
		 * \~
		 */
		enum class test_t : uint8_t {
			CAPTURED  = 0x00, // Условием является выполнение захвата группой
			RECURSING = 0x01, // Условием является нахождение в рекурсивном вызове
			ASSERTED  = 0x02, // Условием является выполнение проверки окружения
			ALWAYS    = 0x03  // Условие выполняется всегда
		};

		/**
		 * \~russian
		 * @brief Инструкция программы регулярного выражения
		 *
		 * \~english
		 * @brief Instruction of the program of a regular expression
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Instruction {
			// Код операции инструкции программы
			opcode_t type;
			// Набор режимов компиляции, действующих для инструкции
			uint32_t flags;
			/**
			 * \~russian
			 * @brief Операнды инструкции, определяемые кодом операции
			 *
			 * \~english
			 * @brief Operands of the instruction determined by the operation code
			 *
			 * \~
			 */
			union {
				/**
				 * \~russian
				 * @brief Операнды инструкции сопоставления одиночного символа
				 *
				 * \~english
				 * @brief Operands of the single character matching instruction
				 *
				 * \~
				 */
				struct {
					// Кодовое значение сопоставляемого символа
					uint32_t code;
				} letter;
				/**
				 * \~russian
				 * @brief Операнды инструкции сопоставления символа из класса символов
				 *
				 * \~english
				 * @brief Operands of the instruction matching a character from a character class
				 *
				 * \~
				 */
				struct {
					// Индекс класса символов в хранилище классов
					uint32_t index;
				} charclass;
				/**
				 * \~russian
				 * @brief Операнды инструкции перехода по двум ветвям
				 *
				 * \~english
				 * @brief Operands of the two-branch jump instruction
				 *
				 * \~
				 */
				struct {
					// Адрес ветви с наибольшим приоритетом
					address_t first;
					// Адрес ветви с наименьшим приоритетом
					address_t second;
					/**
					 * \~russian
					 * Адрес тела повторения одиночного символа
					 *
					 * @details Повторение одиночного символа либо класса символов
					 *          компилируется в переход по двум ветвям, тело повторения
					 *          и переход к его началу, отчего проход ряда подходящих
					 *          символов обходится в три инструкции на каждый символ.
					 *          Адрес помечает переход, ветвь повторения которого
					 *          устроена именно так, благодаря чему исполнение проходит
					 *          ряд одним ходом взамен исполнения трёх инструкций на
					 *          символ. Прочие переходы помечены недействительным
					 *          адресом. Пометка размещена в самой инструкции взамен
					 *          отдельного набора длиною во всю программу намеренно:
					 *          набор тот занимал по четыре байта на каждую инструкцию
					 *          программы, тогда как переходов в ней единицы, а
					 *          размещение его при восстановлении записи обходилось
					 *          дороже разбора всех прочих полей программы.
					 *
					 * \~english
					 * Address of the body of a single character repetition
					 * @details A repetition of a single character or of a character class
					 *          is compiled into a two-branch jump, the body of the repetition
					 *          and a jump to its beginning, which is why walking a run of matching
					 *          characters costs three instructions per character.
					 *          The address marks the jump whose repetition branch
					 *          is arranged exactly like that, thanks to which execution walks
					 *          the run in one move instead of executing three instructions per
					 *          character. The other jumps are marked with an invalid
					 *          address. The mark is placed in the instruction itself instead of
					 *          a separate array as long as the whole program deliberately:
					 *          that array occupied four bytes per every instruction of
					 *          the program, whereas there are only a handful of jumps in it, and
					 *          allocating it when restoring a record cost
					 *          more than parsing all the other fields of the program.
					 *
					 * \~
					 */
					address_t run;
					/**
					 * \~russian
					 * Адрес тела ленивого повторения одиночного символа
					 *
					 * @details Ленивое повторение компилируется тем же переходом по
					 *          двум ветвям, но ветви его переставлены: сопоставление
					 *          продолжается за повторением, а тело повторяется лишь
					 *          по отказу продолжения. Проход ряда одним ходом
					 *          ленивому повторению неприменим, поэтому адрес его
					 *          заведён отдельным полем, а не признаком при поле
					 *          общем: исполнение с возвратом читает пометку жадного
					 *          повторения на каждом переходе по двум ветвям, и
					 *          чтение двух полей взамен одного обходилось там
					 *          потерей в четыре сотых.
					 *
					 * \~english
					 * Address of the body of a lazy repetition of a single character
					 * @details A lazy repetition is compiled by the same two-branch
					 *          jump, but its branches are swapped: matching
					 *          continues past the repetition, and the body is repeated only
					 *          when the continuation fails. Walking a run in one move is
					 *          inapplicable to a lazy repetition, therefore its address is
					 *          introduced as a separate field rather than as an indication at a common
					 *          field: backtracking execution reads the mark of a greedy
					 *          repetition at every two-branch jump, and
					 *          reading two fields instead of one cost there
					 *          a loss of four hundredths.
					 *
					 * \~
					 */
					address_t lazy;
				} split;
				/**
				 * \~russian
				 * @brief Операнды инструкции безусловного перехода
				 *
				 * \~english
				 * @brief Operands of the unconditional jump instruction
				 *
				 * \~
				 */
				struct {
					// Адрес инструкции перехода
					address_t target;
				} jump;
				/**
				 * \~russian
				 * @brief Операнды инструкции сохранения позиции
				 *
				 * \~english
				 * @brief Operands of the position saving instruction
				 *
				 * \~
				 */
				struct {
					// Номер ячейки захвата
					uint32_t slot;
				} save;
				/**
				 * \~russian
				 * @brief Операнды инструкции проверки привязки к позиции в тексте
				 *
				 * \~english
				 * @brief Operands of the instruction checking an anchor to a position in the text
				 *
				 * \~
				 */
				struct {
					// Тип привязки к позиции в тексте
					anchor_t type;
				} assertion;
				/**
				 * \~russian
				 * @brief Операнды инструкций запоминания и отказа от точек возврата
				 *
				 * \~english
				 * @brief Operands of the instructions remembering and giving up backtracking points
				 *
				 * \~
				 */
				struct {
					// Номер ячейки отметки состояния возврата
					uint32_t cell;
				} atomic;
				/**
				 * \~russian
				 * @brief Операнды инструкции сопоставления захваченного текста
				 *
				 * \~english
				 * @brief Operands of the instruction matching captured text
				 *
				 * \~
				 */
				struct {
					// Номер группы, захваченный текст которой сопоставляется
					uint32_t number;
				} backref;
				/**
				 * \~russian
				 * @brief Операнды инструкции глагола управления возвратом
				 *
				 * \~english
				 * @brief Operands of the instruction of a backtracking control verb
				 *
				 * \~
				 */
				struct {
					// Вид глагола управления возвратом
					control_t type;
					/**
					 * \~russian
					 * Номер ячейки отметки ветви охватывающей группы
					 *
					 * @details Ячейка отведена глаголу перехода к ветви следующей:
					 *          возврат в него отсекает точки, ветвью накопленные,
					 *          до отметки её начала, отчего возврат продолжается
					 *          ветвью следующей. Значение недостижимое означает
					 *          отсутствие ветви охватывающей вовсе - глагол
					 *          при нём отказывает попытке сопоставления целиком.
					 *
					 * \~english
					 * Number of the mark cell of the branch of the enclosing group
					 * @details The cell is allotted to the verb of moving to the next branch:
					 *          backtracking into it cuts off the points accumulated by the branch
					 *          down to the mark of its beginning, whereby the backtracking continues
					 *          with the next branch. An unreachable value means
					 *          the absence of an enclosing branch at all — the verb
					 *          then refuses the whole matching attempt.
					 *
					 * \~
					 */
					uint32_t cell;
					// Смещение имени отметки в хранилище имён
					uint32_t offset;
					// Длина имени отметки в октетах
					uint32_t length;
				} control;
				/**
				 * \~russian
				 * @brief Операнды инструкции проверки продвижения по тексту
				 *
				 * \~english
				 * @brief Operands of the instruction checking the advance through the text
				 *
				 * \~
				 */
				struct {
					// Номер ячейки позиции начала повторения
					uint32_t cell;
					// Адрес инструкции завершения повторения
					address_t target;
				} progress;
				/**
				 * \~russian
				 * @brief Операнды инструкции проверки окружения
				 *
				 * \~english
				 * @brief Operands of the lookaround instruction
				 *
				 * \~
				 */
				struct {
					// Адрес тела проверки окружения
					address_t body;
					// Адрес инструкции, следующей за проверкой окружения
					address_t target;
					// Наименьшая длина сопоставляемого проверкой текста
					uint32_t least;
					// Наибольшая длина сопоставляемого проверкой текста
					uint32_t most;
					/**
					 * \~russian
					 * Флаг отрицания результата проверки окружения
					 *
					 * @note Хранится байтом, а не логическим значением: набор инструкций
					 *       восстанавливается обзором образа памяти записи хранилища,
					 *       а запись приходит извне - подделанный байт дал бы значению
					 *       логического типа состояние, языком не отведённое, и всякое
					 *       обращение к нему стало бы неопределённым поведением
					 *
					 * \~english
					 * Negation flag of the result of the lookaround check
					 * @note Stored as a byte rather than as a boolean value: the instruction set
					 *       is restored by viewing the memory image of the storage record,
					 *       and the record comes from the outside — a forged byte would give a value of
					 *       the boolean type a state that the language does not provide for, and every
					 *       reference to it would become undefined behaviour
					 *
					 * \~
					 */
					uint8_t negative;
					/**
					 * \~russian
					 * Флаг проверки текста, предшествующего позиции сопоставления
					 *
					 * @note Хранится байтом по той же причине, что и признак отрицания
					 *
					 * \~english
					 * Flag of checking the text preceding the matching position
					 * @note Stored as a byte for the same reason as the negation indication
					 *
					 * \~
					 */
					uint8_t backward;
					/**
					 * \~russian
					 * Адрес ветви, исполняемой при невыполнении проверки
					 *
					 * @details Адрес установлен, если проверка окружения задаёт условие
					 *          условного выражения: невыполнение проверки при этом
					 *          передаёт исполнение ветви невыполненного условия,
					 *          а не отказывает в сопоставлении.
					 *
					 * \~english
					 * Address of the branch executed when the check does not hold
					 * @details The address is set if the lookaround check defines the condition
					 *          of a conditional expression: failure of the check then
					 *          passes execution to the branch of the unsatisfied condition
					 *          rather than refusing the match.
					 *
					 * \~
					 */
					address_t alternate;
				} look;
				/**
				 * \~russian
				 * @brief Операнды инструкции рекурсивного вызова подвыражения
				 *
				 * \~english
				 * @brief Operands of the instruction of a recursive call of a subexpression
				 *
				 * \~
				 */
				struct {
					// Адрес тела вызываемого подвыражения
					address_t body;
					// Номер группы вызываемого подвыражения
					uint32_t number;
				} call;
				/**
				 * \~russian
				 * @brief Операнды инструкции перехода по ветвям условного выражения
				 *
				 * \~english
				 * @brief Operands of the instruction jumping over the branches of a conditional expression
				 *
				 * \~
				 */
				struct {
					// Тип условия условного выражения
					test_t type;
					// Номер проверяемой группы условного выражения
					uint32_t number;
					// Адрес ветви, исполняемой при выполнении условия
					address_t positive;
					// Адрес ветви, исполняемой при невыполнении условия
					address_t negative;
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
			Instruction() noexcept : type(opcode_t::MATCH), flags(0), letter{0} {}
		} instruction_t;

		/**
		 * \~russian
		 * @brief Программа скомпилированного регулярного выражения
		 *
		 * @details Программа представляет регулярное выражение набором инструкций
		 *          недетерминированного конечного автомата. Хранилища классов символов
		 *          и последовательностей символов размещаются в программе, благодаря
		 *          чему исполнение программы не зависит от объекта разбора.
		 *
		 * \~english
		 * @brief Program of a compiled regular expression
		 * @details The program represents a regular expression as a set of instructions
		 *          of a nondeterministic finite automaton. The storages of character classes
		 *          and of character sequences are placed in the program, thanks to which
		 *          executing the program does not depend on the parsing object.
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Program {
			/**
			 * \~russian
			 * Опознание программы регулярного выражения
			 *
			 * @details Опознание присваивается компиляцией и различает содержимое
			 *          программы. Кэш состояний детерминированного исполнения
			 *          сохраняется между сопоставлениями и отличает программы
			 *          по опознанию, а не по расположению в памяти, поскольку
			 *          пересобранная программа занимает прежнее расположение.
			 *
			 * \~english
			 * Identification of the program of a regular expression
			 * @details The identification is assigned by compilation and distinguishes the content
			 *          of the program. The state cache of deterministic execution
			 *          is kept between matches and tells the programs apart
			 *          by identification rather than by their placement in memory, since
			 *          a rebuilt program occupies the former placement.
			 *
			 * \~
			 */
			uint64_t id;
			// Количество захватывающих групп регулярного выражения
			uint32_t captures;
			// Количество ячеек состояния, требуемых исполнением с возвратом
			uint32_t cells;
			// Набор режимов компиляции регулярного выражения
			uint32_t flags;
			/**
			 * \~russian
			 * Наибольшее допустимое количество шагов сопоставления выражения
			 *
			 * @details Предел задаётся самим выражением указанием «(*LIMIT_MATCH=N)»
			 *          и предел вызывающей стороны понижает, но не повышает.
			 *          Отсутствие предела выражается предельным значением
			 *          разрядности, а не нулём: нуль есть предел действующий,
			 *          сопоставление отвергающий первым же шагом, - «(*LIMIT_MATCH=0)»
			 *          эталон принимает и отвечает исчерпанием предела.
			 *
			 * \~english
			 * Largest admissible number of matching steps of the expression
			 * @details The limit is set by the expression itself by the «(*LIMIT_MATCH=N)» option
			 *          and lowers the limit of the calling side but never raises it.
			 *          The absence of a limit is expressed by the largest value of the type
			 *          rather than by zero: zero is an effective limit that refuses the matching
			 *          at the very first step — «(*LIMIT_MATCH=0)» is accepted by the reference
			 *          and answers with the exhaustion of the limit.
			 *
			 * \~
			 */
			uint32_t steps;
			/**
			 * \~russian
			 * Наибольшая допустимая глубина рекурсивных вызовов выражения
			 *
			 * @details Предел задаётся самим выражением указанием «(*LIMIT_DEPTH=N)»
			 *          и предел вызывающей стороны понижает, но не повышает.
			 *          Отсутствие предела выражается предельным значением разрядности
			 *          наравне с пределом шагов сопоставления.
			 *
			 * \~english
			 * Largest admissible depth of the recursive calls of the expression
			 * @details The limit is set by the expression itself by the «(*LIMIT_DEPTH=N)» option
			 *          and lowers the limit of the calling side but never raises it.
			 *          The absence of a limit is expressed by the largest value of the type
			 *          along with the limit of the matching steps.
			 *
			 * \~
			 */
			uint32_t depth;
			/**
			 * \~russian
			 * Наибольший допустимый объём памяти сопоставления выражения в килобайтах
			 *
			 * @details Предел задаётся самим выражением указанием «(*LIMIT_HEAP=N)»
			 *          и считается по наборам точек возврата, кадров вызовов и записей
			 *          журнала - тому, что сопоставление и размещает. Отсутствие
			 *          предела выражается предельным значением разрядности.
			 *
			 * \~english
			 * Largest admissible amount of the matching memory of the expression in kibibytes
			 * @details The limit is set by the expression itself by the «(*LIMIT_HEAP=N)» option
			 *          and is counted over the sets of the backtracking points, the call frames and the journal
			 *          entries — over what the matching actually allocates. The absence
			 *          of a limit is expressed by the largest value of the type.
			 *
			 * \~
			 */
			uint32_t heap;
			/**
			 * \~russian
			 * Соглашение о переводе строки выражения
			 *
			 * @details Соглашение задаётся указанием вида «(*CRLF)» в начале выражения
			 *          и правит точкой, привязками к границам строк и привязкой конца
			 *          текста. Соглашением умолчания выступает перевод строки.
			 *
			 * \~english
			 * Newline convention of the expression
			 * @details The convention is set by an option of the «(*CRLF)» kind at the start of an expression
			 *          and governs the dot, the anchors to the line boundaries and the anchor of the end
			 *          of the text. The default convention is the line feed.
			 *
			 * \~
			 */
			newline_t newline;
			/**
			 * \~russian
			 * Номер ячейки состояния, отметку последнюю хранящей
			 *
			 * @details Ячейка ведётся наравне с ячейками захвата: глагол отметки
			 *          пишет в неё адрес свой, а возврат запись отменяет, отчего
			 *          по совпадении в ней остаётся отметка пути, совпадение
			 *          давшего. Значение недостижимое означает выражение,
			 *          глаголов отметки не несущее вовсе.
			 *
			 * \~english
			 * Number of the state cell that holds the last mark
			 * @details The cell is maintained along with the capture cells: the mark verb
			 *          writes its own address into it, whereas backtracking undoes the write, whereby
			 *          upon a match it holds the mark of the path that produced the match.
			 *          An unreachable value means an expression that carries no mark verbs at all.
			 *
			 * \~
			 */
			uint32_t marker;
			// Набор инструкций программы
			Sequence <instruction_t> instructions;
			/**
			 * \~russian
			 * Хранилище ссылок на классы символов
			 *
			 * @details Диапазоны и свойства всех классов программы хранятся
			 *          сплошными наборами, а класс задаётся ссылкой на участки
			 *          этих наборов. Устройство это заведено взамен набора
			 *          классов, где каждый нёс свои наборы: классов в программе
			 *          бывают десятки тысяч, и размещение под каждый двух
			 *          наборов отдельных обходилось дороже, нежели вся прочая
			 *          сборка программы.
			 *
			 * \~english
			 * Storage of the references to character classes
			 * @details The ranges and properties of all the classes of the program are kept
			 *          as contiguous sequences, and a class is defined by a reference to spans
			 *          of those sequences. This arrangement was introduced in place of a set of
			 *          classes where each one carried its own sequences: there can be tens of thousands
			 *          of classes in a program, and allocating two separate sequences for
			 *          each one cost more than all the rest of
			 *          building the program.
			 *
			 * \~
			 */
			Sequence <classref_t> classes;
			// Сплошной набор диапазонов кодовых значений всех классов символов
			Sequence <range_t> ranges;
			// Сплошной набор ссылок на свойства Юникода всех классов символов
			Sequence <property_t> properties;
			// Хранилище последовательностей символов
			Sequence <uint32_t> strings;
			/**
			 * \~russian
			 * Хранилище имён отметок глаголов управления
			 *
			 * @details Имена всех отметок программы лежат сплошным набором октетов,
			 *          а инструкция глагола ссылается на участок его смещением
			 *          и длиною наравне с классами символов. Имя выводится наружу
			 *          по совпадении и указывает ветвь, совпадение давшую.
			 *
			 * \~english
			 * Storage of the names of the marks of the control verbs
			 * @details The names of all the marks of the program lie in a contiguous set of octets,
			 *          whereas the instruction of a verb refers to a span of it by an offset
			 *          and a length, along with the character classes. The name is yielded outward
			 *          upon a match and tells which branch produced the match.
			 *
			 * \~
			 */
			Sequence <uint8_t> markers;
			/**
			 * \~russian
			 * Держатель записи хранилища, обозреваемой наборами программы
			 *
			 * @details Программа, восстановленная из хранилища, содержимого
			 *          наборов своих не имеет: наборы обозревают участки записи,
			 *          лежащей в памяти целиком. Держатель продлевает жизнь этой
			 *          записи на срок жизни программы и всех её копий. Программа,
			 *          собранная компиляцией, держателя не имеет вовсе.
			 *
			 * \~english
			 * Holder of the storage record viewed by the sequences of the program
			 * @details A program restored from the storage has no content of
			 *          its own sequences: the sequences view spans of the record
			 *          that lies in memory as a whole. The holder extends the life of that
			 *          record for the lifetime of the program and of all its copies. A program
			 *          built by compilation has no holder at all.
			 *
			 * \~
			 */
			shared_ptr <const string> blob;
			// Предварительный отбор позиций сопоставления
			prefilter_t prefilter;
			/**
			 * \~russian
			 * Признак выражения, сопоставляемого одним литералом
			 *
			 * @details Выражение, состоящее из одной последовательности символов,
			 *          сопоставляется поиском этой последовательности в тексте
			 *          и исполнения программы не требует вовсе.
			 *
			 * \~english
			 * Indication of an expression matched by a single literal
			 * @details An expression consisting of a single character sequence
			 *          is matched by searching for that sequence in the text
			 *          and does not require executing the program at all.
			 *
			 * \~
			 */
			bool plain;
			// Последовательность символов выражения, сопоставляемого литералом
			string text;
			/**
			 * \~russian
			 * Признак выражения, проходящего текст единственной попыткой
			 *
			 * @details Выражение начинается неограниченным повторением любого символа,
			 *          отчего совпадение, начинающееся правее, начинается и в позиции
			 *          начала поиска: повторение поглощает всё до него. Исполнение
			 *          с возвратом при этом попытки в каждой позиции текста не повторяет,
			 *          а единственная попытка его проходит текст соразмерно длине,
			 *          если повторения выражения не вложены друг в друга и повторение
			 *          любого символа в нём единственно.
			 *
			 * \~english
			 * Indication of an expression walking the text in a single attempt
			 * @details The expression begins with an unbounded repetition of any character,
			 *          which is why a match beginning further to the right also begins at the position
			 *          where the search starts: the repetition absorbs everything before it. Backtracking
			 *          execution then does not repeat the attempt at every position of the text,
			 *          and its single attempt walks the text proportionally to its length,
			 *          provided that the repetitions of the expression are not nested in one another and the repetition
			 *          of any character in it is a single one.
			 *
			 * \~
			 */
			bool sweeping;
			/**
			 * \~russian
			 * Признак выражения, привязанного к позиции начала поиска
			 *
			 * @details Выражение начинается привязкой, выполнимой лишь в позиции
			 *          начала поиска, поэтому совпадение, начинающееся правее,
			 *          невозможно. Проход по тексту прекращается, как только
			 *          сопоставление, начатое в позиции начала поиска, прервано.
			 *
			 * \~english
			 * Indication of an expression anchored to the position where the search starts
			 * @details The expression begins with an anchor that can hold only at the position
			 *          where the search starts, therefore a match beginning further to the right
			 *          is impossible. Walking the text stops as soon as
			 *          the match started at the position where the search starts is broken.
			 *
			 * \~
			 */
			bool anchored;
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
			Program() noexcept : id(0), captures(0), cells(0), flags(0), steps(~0u), depth(~0u), heap(~0u), newline(newline_t::LF), marker(~0u), plain(false), sweeping(false), anchored(false) {}
			/**
			 * \~russian
			 * @brief Метод извлечения обзора класса символов программы
			 *
			 * @param index номер класса символов в хранилище ссылок
			 * @return      обзор класса символов программы
			 *
			 * \~english
			 * @brief Method of getting a view of a character class of the program
			 * @param index number of the character class in the storage of references
			 * @return      view of the character class of the program
			 *
			 * \~
			 */
			classview_t charclass(const uint32_t index) const noexcept {
				// Обзор класса символов программы
				classview_t result;
				/**
				 * \~russian
				 * Если номер класса символов хранилищу не принадлежит
				 *
				 * @details Проверка эта - последний заслон, а не рабочий ход:
				 *          номера классов расставляет компиляция, а запись,
				 *          восстанавливаемая из хранилища, проверяется на
				 *          принадлежность номеров хранилищу до употребления.
				 *          Обращение к обзору идёт на каждом символе, поэтому
				 *          заслон вынесен под предсказание маловероятной ветви.
				 *
				 * \~english
				 * If the number of the character class does not belong to the storage
				 * @details That check is the last barrier rather than a working move:
				 *          the numbers of the classes are laid out by compilation, and a record
				 *          restored from the storage is checked for the
				 *          belonging of the numbers to the storage before use.
				 *          The view is referred to at every character, therefore the
				 *          barrier is placed under the prediction of an unlikely branch.
				 *
				 * \~
				 */
				if(AWH_REGEX_UNLIKELY(static_cast <size_t> (index) >= this->classes.size()))
					// Выводим обзор класса символов программы
					return result;
				// Получаем ссылку на класс символов программы
				const classref_t & value = this->classes[index];
				// Выполняем установку признака отрицания класса символов
				result.negative = (value.negative != 0);
				// Выполняем установку обзора набора диапазонов класса
				result.ranges = Span <range_t> ((this->ranges.data() + value.ranges), value.rangeCount);
				// Выполняем установку обзора набора свойств класса
				result.properties = Span <property_t> ((this->properties.data() + value.properties), value.propertyCount);
				// Выводим обзор класса символов программы
				return result;
			}
			/**
			 * \~russian
			 * @brief Метод сброса программы с сохранением размещения наборов
			 *
			 * @details Сброс отличается от очистки тем, что место, наборами
			 *          программы занятое, за ними остаётся. Заведён он ради
			 *          сберегательной программы построителя: она наполняется
			 *          при каждой компиляции и место, однажды отведённое,
			 *          переживает построение.
			 *
			 * \~english
			 * @brief Method of resetting the program keeping the allocations of its sequences
			 * @details The reset differs from clearing in that the space occupied by the
			 *          sequences of the program stays with them. It is introduced for the
			 *          scratch program of the compiler: that one is filled at every
			 *          compilation, and the space allocated once outlives the build.
			 *
			 * \~
			 */
			void reset() noexcept {
				// Выполняем сброс опознания программы регулярного выражения
				this->id = 0;
				// Выполняем сброс количества захватывающих групп
				this->captures = 0;
				// Выполняем сброс количества ячеек состояния
				this->cells = 0;
				// Выполняем сброс набора режимов компиляции
				this->flags = 0;
				// Выполняем сброс предела шагов сопоставления выражения
				this->steps = ~0u;
				// Выполняем сброс предела глубины рекурсивных вызовов
				this->depth = ~0u;
				// Выполняем сброс предела объёма памяти сопоставления
				this->heap = ~0u;
				// Выполняем сброс соглашения о переводе строки выражения
				this->newline = newline_t::LF;
				// Выполняем сброс номера ячейки отметки последней
				this->marker = ~0u;
				// Выполняем сброс набора инструкций программы
				this->instructions.reset();
				// Выполняем сброс хранилища ссылок на классы символов
				this->classes.reset();
				// Выполняем сброс набора диапазонов кодовых значений
				this->ranges.reset();
				// Выполняем сброс набора ссылок на свойства Юникода
				this->properties.reset();
				// Выполняем сброс хранилища последовательностей символов
				this->strings.reset();
				// Выполняем сброс хранилища имён отметок глаголов управления
				this->markers.reset();
				// Выполняем освобождение держателя записи хранилища
				this->blob.reset();
				// Выполняем очистку предварительного отбора позиций
				this->prefilter.clear();
				// Выполняем сброс признака сопоставления выражения литералом
				this->plain = false;
				// Выполняем очистку последовательности символов выражения
				this->text.clear();
				// Выполняем сброс признака прохода текста единственной попыткой
				this->sweeping = false;
				// Выполняем сброс признака привязки к позиции начала поиска
				this->anchored = false;
			}
			/**
			 * \~russian
			 * @brief Метод очистки программы регулярного выражения
			 *
			 * \~english
			 * @brief Method of clearing the program of a regular expression
			 *
			 * \~
			 */
			void clear() noexcept {
				// Выполняем сброс опознания программы регулярного выражения
				this->id = 0;
				// Выполняем сброс количества захватывающих групп
				this->captures = 0;
				// Выполняем сброс количества ячеек состояния
				this->cells = 0;
				// Выполняем сброс набора режимов компиляции
				this->flags = 0;
				// Выполняем сброс предела шагов сопоставления выражения
				this->steps = ~0u;
				// Выполняем сброс предела глубины рекурсивных вызовов
				this->depth = ~0u;
				// Выполняем сброс предела объёма памяти сопоставления
				this->heap = ~0u;
				// Выполняем сброс соглашения о переводе строки выражения
				this->newline = newline_t::LF;
				// Выполняем сброс номера ячейки отметки последней
				this->marker = ~0u;
				// Выполняем очистку набора инструкций программы
				this->instructions.clear();
				// Выполняем очистку хранилища ссылок на классы символов
				this->classes.clear();
				// Выполняем очистку набора диапазонов кодовых значений
				this->ranges.clear();
				// Выполняем очистку набора ссылок на свойства Юникода
				this->properties.clear();
				// Выполняем очистку хранилища последовательностей символов
				this->strings.clear();
				// Выполняем очистку хранилища имён отметок глаголов управления
				this->markers.clear();
				// Выполняем освобождение держателя записи хранилища
				this->blob.reset();
				// Выполняем очистку предварительного отбора позиций
				this->prefilter.clear();
				// Выполняем сброс признака сопоставления выражения литералом
				this->plain = false;
				// Выполняем очистку последовательности символов выражения
				this->text.clear();
				// Выполняем сброс признака прохода текста единственной попыткой
				this->sweeping = false;
				// Выполняем сброс признака привязки к позиции начала поиска
				this->anchored = false;
			}
		} program_t;
	};
};

#endif // __AWH_REGEX_PROGRAM__
