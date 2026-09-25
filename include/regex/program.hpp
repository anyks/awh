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
#pragma once

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
		 * @brief Наибольшая длина литерала, пометкой одной инструкции выразимая
		 *
		 * @details Байты литерала лежат в зазоре операндов одиночного символа:
		 *          объединение операндов держит тридцать два байта по наибольшему
		 *          члену своему, а кодовое значение символа и длина литерала
		 *          занимают из них пять. Литерал длиннее пометки проходится
		 *          несколькими заходами: пометка у всякой инструкции своя.
		 *
		 * \~english
		 * @brief Largest length of a literal expressible by the mark of a single instruction
		 * @details The bytes of the literal lie in the gap of the operands of a single character:
		 *          the union of the operands holds thirty-two bytes by its largest member,
		 *          while the code value of the character and the length of the literal take five
		 *          of them. A literal longer than the mark is walked in several trips: every
		 *          instruction has a mark of its own.
		 *
		 * \~
		 */
		constexpr size_t MAX_LITERAL = 27;

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
			CONTROL   = 0x16, // Размещение точки возврата глагола управления
			RESET     = 0x17, // Восстановление позиции сопоставления из ячейки
			SCRIPT    = 0x18  // Проверка прогона письменности от позиции из ячейки
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
			/**
			 * \~russian
			 * Количество одинаковых инструкций подряд, с этой начинающихся
			 *
			 * @details Счётное повторение одиночного символа разворачивается
			 *          копиями: «[0-9]{3,5}» даёт три копии обязательные
			 *          да две необязательные. Копии обязательные идут подряд
			 *          и управления между собою не принимают, отчего проход
			 *          их тесным циклом равен проходу по одной, а заходов
			 *          в разбор кода операции стоит один взамен трёх.
			 *
			 *          Пометка развёрнутой формы НЕ отменяет: исполнение
			 *          без возврата и детерминированное исполнение ведут
			 *          наборы состояний по графу инструкций и счётчиков
			 *          не имеют вовсе, отчего обходят копии по одной,
			 *          пометки не замечая. Оттого же пометка ставится
			 *          КАЖДОЙ копии ряда со своим остатком: управление,
			 *          в середину ряда пришедшее, поглотит ровно столько,
			 *          сколько от ряда осталось.
			 *
			 *          Поле умещается в зазор выравнивания: размер инструкции
			 *          остаётся прежним, и устройство записи хранилища
			 *          пометкой не затрагивается. Единица означает отсутствие
			 *          ряда и есть значение по умолчанию.
			 *
			 * \~english
			 * Number of identical instructions in a row starting with this one
			 * @details A counted repetition of a single character is unrolled into copies:
			 *          "[0-9]{3,5}" yields three mandatory copies and two optional ones.
			 *          The mandatory copies follow one another and do not take control
			 *          between themselves, so traversing them in a tight loop equals
			 *          traversing them one by one, while costing one trip through the
			 *          dispatch of the operation code instead of three.
			 *
			 *          The mark does NOT cancel the unrolled form: the execution without
			 *          backtracking and the deterministic execution carry sets of states
			 *          over the graph of instructions and have no counters at all, so they
			 *          walk the copies one by one without noticing the mark. For the same
			 *          reason the mark is placed on EVERY copy of the row with its own
			 *          remainder: control that arrives in the middle of the row will consume
			 *          exactly as much as is left of the row.
			 *
			 *          The field fits into the alignment gap: the size of the instruction
			 *          stays the same, and the layout of the storage record is not affected
			 *          by the mark. A unit means the absence of a row and is the default.
			 *
			 * \~
			 */
			uint16_t repeat;
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
					/**
					 * \~russian
					 * Длина литерала, с этой инструкции начинающегося, в символах
					 *
					 * @details Литерал выражения компилируется инструкциями одиночного
					 *          символа по одной на символ, и исполнение с возвратом
					 *          проходило его заходом в разбор кода операции на каждом
					 *          символе. Подряд идущие символы, сопоставляемые байтом
					 *          дословно, управления между собою не принимают и точек
					 *          возврата не ставят, отчего сличение их одним заходом
					 *          равно сличению по одному.
					 *
					 *          Пометка ставится КАЖДОЙ инструкции литерала со своим
					 *          остатком, как и пометка ряда: управление приходит
					 *          и в середину литерала - ветвью либо переходом. Нуль
					 *          означает инструкцию, байтом дословно не сопоставляемую,
					 *          либо инструкцию развёрнутой программы, какой пометка
					 *          не ставится, и есть значение по умолчанию; единица -
					 *          литерал из одного символа, заходу одиночному равный.
					 *
					 * \~english
					 * Length of the literal starting with this instruction, in characters
					 * @details A literal of the expression is compiled into single character
					 *          instructions, one per character, and the backtracking execution
					 *          walked it with a trip through the dispatch of the operation code
					 *          on every character. Consecutive characters matched by a byte
					 *          verbatim take no control between themselves and set no backtracking
					 *          points, so matching them in one trip equals matching them one by one.
					 *
					 *          The mark is placed on EVERY instruction of the literal with its own
					 *          remainder, like the mark of a row: control arrives in the middle of the
					 *          literal as well — by a branch or a jump. Zero means an instruction
					 *          not matched by a byte verbatim, or an instruction of the reverse
					 *          program, which receives no mark, and is the default; a unit is a
					 *          literal of a single character, equal to a single trip.
					 *
					 * \~
					 */
					uint8_t length;
					/**
					 * \~russian
					 * Байты литерала, с этой инструкции начинающегося
					 *
					 * @details Байты лежат в самой инструкции, а не в хранилище
					 *          последовательностей программы намеренно: исполнение
					 *          читает их из той же строки кэша, что и код операции,
					 *          и инструкций следующих не касается вовсе. Байты
					 *          за длиною литерала нулевые: запись хранилища несёт
					 *          образ инструкций, и мусор в ней делал бы записи
					 *          одного выражения различными.
					 *
					 * \~english
					 * Bytes of the literal starting with this instruction
					 * @details The bytes lie in the instruction itself rather than in the storage
					 *          of sequences of the program deliberately: the execution reads them from
					 *          the same cache line as the operation code and does not touch the
					 *          following instructions at all. The bytes past the length of the literal
					 *          are zero: the storage record carries the image of the instructions, and
					 *          garbage in it would make records of one expression differ.
					 *
					 * \~
					 */
					char bytes[MAX_LITERAL];
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
					 * Признак ленивости повторения одиночного символа
					 *
					 * @details Ленивое повторение компилируется тем же переходом по
					 *          двум ветвям, но ветви его переставлены: сопоставление
					 *          продолжается за повторением, а тело повторяется лишь
					 *          по отказу продолжения. Адрес тела оба повторения ведут
					 *          полем общим, а рознятся признаком этим: исполнение
					 *          с возвратом читает пометку повторения на КАЖДОМ переходе
					 *          по двум ветвям, и выбор одной из ветвей - переход самый
					 *          частый. Поле отдельное под ленивое тело пробовалось
					 *          и стоило чтения второго на каждом переходе: замером
					 *          чередованием получена потеря до двенадцати сотых
					 *          на «GET|POST|PUT|DELETE|HEAD|OPTIONS» и до десяти
					 *          на выражениях с повторениями ограниченными.
					 *
					 * \~english
					 * Flag of the laziness of a repetition of a single character
					 * @details A lazy repetition is compiled by the same two-branch jump, but
					 *          its branches are swapped: matching continues past the repetition,
					 *          and the body is repeated only upon a refusal of the continuation.
					 *          Both repetitions carry the address of the body in a common field
					 *          and differ by this flag: backtracking execution reads the mark of
					 *          a repetition at EVERY two-branch jump, and choosing one of the
					 *          branches is the most frequent jump. A separate field for the lazy
					 *          body was tried and cost a second read at every jump: interleaved
					 *          measurement yielded a loss of up to twelve hundredths on
					 *          «GET|POST|PUT|DELETE|HEAD|OPTIONS» and up to ten on the
					 *          expressions with bounded repetitions.
					 *
					 * \~
					 */
					uint8_t lazily;
					/**
					 * \~russian
					 * Признак бесплодности возврата в ряд повторения
					 *
					 * @details Ряд жадный проходит подходящие символы до упора, а возврат
					 *          в него перебирает длины ряда убывающие, продолжение
					 *          на каждой повторяя. Перебор этот бесплоден, когда символ,
					 *          телом повторения поглощаемый, продолжению заведомо
					 *          не отвечает: «\w+@» на ряду из букв упрётся в «собаку»
					 *          при всякой длине, ибо «собака» букве не равна. Признак
					 *          ставится разбором при сборке и означает, что точки
					 *          возврата ряду не нужны вовсе. Эталон зовёт это
					 *          «auto-possessification» и ведёт разбор тем же доводом.
					 *
					 * \~english
					 * Flag of the futility of backtracking into a run of the repetition
					 * @details A greedy run walks the matching characters to the limit, while
					 *          backtracking into it enumerates the decreasing lengths of the run,
					 *          repeating the continuation at every one. That enumeration is futile
					 *          when a character consumed by the body of the repetition certainly
					 *          does not suit the continuation: «\w+@» on a run of letters runs into
					 *          the «at» sign at every length, because the «at» sign does not equal
					 *          a letter. The flag is set by the analysis during the build and means
					 *          that the run needs no backtracking points at all. The reference calls
					 *          this «auto-possessification» and conducts the analysis by the same
					 *          argument.
					 *
					 * \~
					 */
					uint8_t solid;
					/**
					 * \~russian
					 * Количество копий ограниченного повторения одиночного символа
					 *
					 * @details Повторение ограниченное - «\d{1,3}» - компилируется
					 *          не переходом с возвратом к началу, а ЦЕПОЧКОЙ переходов:
					 *          «SPLIT(тело,выход) тело SPLIT(тело,выход) тело», где
					 *          выход у всей цепочки общий, а тело всякий раз следует
					 *          за переходом. Обратного перехода в ней нет вовсе, отчего
					 *          пометка безграничного ряда её не берёт, и проход её
					 *          стоил двух инструкций на каждую копию.
					 *
					 *          Значение есть количество копий, переходом возглавляемых,
					 *          и ставится оно КАЖДОМУ переходу цепочки со своим остатком:
					 *          управление приходит и в середину её. Нуль означает,
					 *          что переход цепочки не возглавляет.
					 *
					 *          Поле легло в зазор выравнивания операндов перехода:
					 *          размер инструкции от пометки не изменился. Адрес тела
					 *          в нём не хранится намеренно - тело следует за переходом
					 *          всегда, - а поле «run» под него не занято оттого, что
					 *          кодогенератор читает его десятью местами и порождённый
					 *          им код прошёл бы цепочку без предела.
					 *
					 * \~english
					 * Number of copies of a bounded repetition of a single character
					 * @details A bounded repetition — «\d{1,3}» — is compiled not as a jump
					 *          with a return to the beginning but as a CHAIN of jumps:
					 *          «SPLIT(body,exit) body SPLIT(body,exit) body», where
					 *          the exit is common to the whole chain and the body always
					 *          follows the jump. There is no backward jump in it at all,
					 *          which is why the mark of an unbounded row does not cover it,
					 *          and traversing it cost two instructions per every copy.
					 *
					 *          The value is the number of copies headed by the jump,
					 *          and it is placed on EVERY jump of the chain with its own
					 *          remainder: control arrives in the middle of it as well.
					 *          Zero means that the jump heads no chain.
					 *
					 *          The field fits into the alignment gap of the operands of
					 *          the jump: the size of the instruction is not changed by the
					 *          mark. The address of the body is deliberately not kept in it —
					 *          the body always follows the jump — and the «run» field is not
					 *          taken for it because the code generator reads that field in ten
					 *          places and the code it generates would walk the chain
					 *          without a limit.
					 *
					 * \~
					 */
					uint16_t most;
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
					/**
					 * \~russian
					 * Флаг отсечения точек возврата, накопленных телом проверки
					 *
					 * @details Проверка окружения обыкновенная атомарна: точки возврата,
					 *          телом её накопленные, отсекаются выполнением проверки,
					 *          отчего отказ последующего текста тела не пересопоставляет.
					 *          Проверка не отсекающая точки сохраняет, отчего отказ
					 *          последующего текста продолжается перебором тела.
					 *          Хранится байтом по той же причине, что и признак отрицания
					 *
					 * \~english
					 * Flag of cutting off the backtracking points accumulated by the body of the check
					 * @details An ordinary lookaround check is atomic: the backtracking points
					 *          accumulated by its body are cut off by the fulfilment of the check,
					 *          whereby a failure of the following text does not rematch the body.
					 *          A non-atomic check keeps the points, whereby a failure
					 *          of the following text continues with a walk over the body.
					 *          Stored as a byte for the same reason as the negation indication
					 *
					 * \~
					 */
					uint8_t atomic;
					/**
					 * \~russian
					 * Номер ячейки позиции начала не отсекающей проверки
					 *
					 * @details Ячейка отведена проверке не отсекающей: тело её исполняется
					 *          не отдельным запуском, а продолжением исполнения текущего,
					 *          отчего позиция начала проверки обязана пережить тело.
					 *          Значение недостижимое означает проверку обыкновенную
					 *
					 * \~english
					 * Number of the cell of the starting position of a non-atomic check
					 * @details The cell is allotted to a non-atomic check: its body is executed
					 *          not as a separate run but as a continuation of the current one,
					 *          whereby the starting position of the check must outlive the body.
					 *          An unreachable value means an ordinary check
					 *
					 * \~
					 */
					uint32_t cell;
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
				 * @brief Операнды инструкции восстановления позиции сопоставления
				 *
				 * \~english
				 * @brief Operands of the instruction restoring the matching position
				 *
				 * \~
				 */
				struct {
					// Номер ячейки позиции начала не отсекающей проверки
					uint32_t cell;
					// Адрес инструкции, следующей за проверкой окружения
					address_t target;
					/**
					 * \~russian
					 * Флаг проверки текста, предшествующего позиции сопоставления
					 *
					 * @note Хранится байтом по той же причине, что и признак отрицания:
					 *       тело ретроспективной проверки обязано завершиться
					 *       в позиции её начала, тогда как телу опережающей
					 *       проверки позиция завершения безразлична
					 *
					 * \~english
					 * Flag of checking the text preceding the matching position
					 * @note Stored as a byte for the same reason as the negation indication:
					 *       the body of a lookbehind check must end
					 *       at the position of its beginning, whereas for the body of a lookahead
					 *       check the ending position is immaterial
					 *
					 * \~
					 */
					uint8_t backward;
				} reset;
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
			Instruction() noexcept : type(opcode_t::MATCH), repeat(1), flags(0), letter{0} {}
		} instruction_t;

		/**
		 * \~russian
		 * @brief Функция проверки одинаковости двух инструкций программы
		 *
		 * @details Одинаковыми считаются инструкции, сопоставляющие один и тот же
		 *          одиночный символ одними и теми же режимами: ряд таких копий
		 *          проходится одним заходом, и длина ряда помечает каждую копию.
		 *          Инструкции прочие ряду неподвластны, сколь бы ни были похожи:
		 *          переход по двум ветвям и сохранение позиции управление принимают,
		 *          и проход их одним заходом равенства исполнению не сохраняет.
		 *
		 *          Правило ведётся здесь одним местом намеренно: пометку ставит
		 *          сборка, а поверяет её восстановление записи, и разойдись два
		 *          прочтения правила - поверка отвергла бы пометку правильную либо,
		 *          что хуже, приняла бы поддельную.
		 *
		 * @param first  инструкция первая
		 * @param second инструкция вторая
		 * @return       результат проверки одинаковости инструкций
		 *
		 * \~english
		 * @brief Function of checking whether two instructions of the program are identical
		 * @details Identical are the instructions matching one and the same single character
		 *          with one and the same modes: a row of such copies is traversed in one trip,
		 *          and the length of the row marks every copy. The other instructions are
		 *          not subject to a row however similar they are: a two-branch jump and
		 *          a saving of a position take control, and traversing them in one trip
		 *          does not preserve the equality to the execution.
		 *
		 *          The rule is kept here in a single place deliberately: the mark is placed
		 *          by the build and verified by the restoration of the record, and were
		 *          the two readings of the rule to diverge, the verification would reject
		 *          a correct mark or, what is worse, accept a forged one.
		 *
		 * @param first  the first instruction
		 * @param second the second instruction
		 * @return       result of checking whether the instructions are identical
		 *
		 * \~
		 */
		AWH_REGEX_INLINE bool identical(const instruction_t & first, const instruction_t & second) noexcept {
			/**
			 * Если код операции либо набор режимов не совпадает
			 */
			if((first.type != second.type) || (first.flags != second.flags))
				// Выводим отсутствие одинаковости инструкций
				return false;
			/**
			 * Определяем код операции сличаемых инструкций
			 */
			switch(static_cast <uint8_t> (first.type)) {
				/**
				 * Сопоставление одиночного символа сличается кодовым значением
				 */
				case static_cast <uint8_t> (opcode_t::CHAR):
				case static_cast <uint8_t> (opcode_t::CODEUNIT):
					// Выводим результат сличения кодовых значений символов
					return (first.letter.code == second.letter.code);
				// Сопоставление символа из класса сличается номером класса
				case static_cast <uint8_t> (opcode_t::CLASS):
					// Выводим результат сличения номеров классов символов
					return (first.charclass.index == second.charclass.index);
				/**
				 * Сопоставление любого символа операндов не несёт вовсе
				 *
				 * @details Режимы сличены выше, а «DOTALL» лежит именно в них:
				 *          инструкции с режимами равными неотличимы
				 *
				 */
				case static_cast <uint8_t> (opcode_t::ANY): return true;
			}
			// Инструкция прочая ряду неподвластна
			return false;
		}

		/**
		 * \~russian
		 * @brief Функция проверки сопоставления инструкцией байта текста дословно
		 *
		 * @details Дословно сопоставляется одиночный символ ASCII с учётом регистра:
		 *          он отвечает ровно одному байту текста, равному своему кодовому
		 *          значению, во всяком режиме разбора текста. Символ за пределами
		 *          ASCII в режиме разбора UTF-8 занимает несколько байтов, а символ
		 *          без учёта регистра отвечает и иным значениям - у буквы «k» среди
		 *          них знак кельвина за пределами ASCII, - отчего литералу, байтами
		 *          сличаемому, не принадлежат ни тот, ни другой.
		 *
		 *          Правило ведётся здесь одним местом по той же причине, что
		 *          и одинаковость инструкций: пометку литерала ставит сборка,
		 *          а поверяет её восстановление записи.
		 *
		 * @param instruction проверяемая инструкция программы
		 * @return            результат проверки сопоставления байта дословно
		 *
		 * \~english
		 * @brief Function of checking whether an instruction matches a byte of the text verbatim
		 * @details An ASCII single character with case sensitivity is matched verbatim: it
		 *          corresponds to exactly one byte of the text equal to its code value in every
		 *          mode of parsing the text. A character beyond ASCII takes several bytes in the
		 *          UTF-8 parsing mode, and a character without case sensitivity corresponds to
		 *          other values as well — among those of the letter «k» is the Kelvin sign beyond
		 *          ASCII, — which is why neither belongs to a literal compared by bytes.
		 *
		 *          The rule is kept here in a single place for the same reason as the identity
		 *          of instructions: the mark of a literal is placed by the build and verified
		 *          by the restoration of the record.
		 *
		 * @param instruction the instruction of the program to check
		 * @return            result of checking whether a byte is matched verbatim
		 *
		 * \~
		 */
		AWH_REGEX_INLINE bool verbatim(const instruction_t & instruction) noexcept {
			// Выводим результат проверки сопоставления байта дословно
			return ((instruction.type == opcode_t::CHAR) && (instruction.letter.code < 0x80) &&
			 ((instruction.flags & static_cast <uint32_t> (flag_t::CASELESS)) == 0));
		}

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
			 * Признак выражения, привязанного к началу строки
			 *
			 * @details Выражение начинается привязкой, выполнимой лишь в начале
			 *          строки, поэтому попытка сопоставления в позиции иной
			 *          заведомо отказывает. Обход позиций начала попытки
			 *          пропускает такие позиции разом, не разбирая программы:
			 *          на тексте из четырёх строк попыток выходит четыре
			 *          вместо ста пяти.
			 *
			 *          Признак этот с признаком привязки к позиции начала поиска
			 *          не совпадает: тот означает единственную попытку, а этот -
			 *          попытки в позициях, числом ограниченных. Выражение,
			 *          привязанное к началу поиска, несёт оба, и обход позиций
			 *          разбирает сперва привязку.
			 *
			 * \~english
			 * Indication of an expression anchored to the beginning of a line
			 * @details The expression begins with an anchor that can hold only at the beginning
			 *          of a line, therefore a matching attempt at any other position
			 *          fails for certain. Walking the positions of the beginning of an attempt
			 *          skips such positions at once without taking the program apart:
			 *          on a text of four lines there are four attempts
			 *          instead of a hundred and five.
			 *          This indication does not coincide with the indication of the anchoring to the position
			 *          where the search starts: that one means a single attempt, while this one means
			 *          attempts at positions limited in number. An expression
			 *          anchored to the beginning of the search carries both, and the walk over the positions
			 *          takes the anchoring apart first.
			 *
			 * \~
			 */
			bool startline;
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
			Program() noexcept : id(0), captures(0), cells(0), flags(0), steps(~0u), depth(~0u), heap(~0u), newline(newline_t::LF), marker(~0u), plain(false), sweeping(false), anchored(false), startline(false) {}
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
