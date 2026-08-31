/**
 * @file backtrack.hpp
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
 * @brief Заголовочный файл исполнения регулярных выражений с возвратом — класс Backtrack,
 *        исполняющий программу единственным состоянием с сохранением точек возврата,
 *        что позволяет сопоставлять конструкции вне регулярного подмножества синтаксиса
 *
 * @section backtrack_decisions Намеренные решения
 *
 * @details Перечисленное ниже выглядит несообразностью, но выбрано осознанно и
 *          правке не подлежит. Раздел заведён затем, чтобы разбор кода не начинался
 *          каждый раз с одних и тех же выводов.
 *
 *          <b>Ряд точек возврата повторения одиночного символа хранится единственной
 *          точкой.</b> Повторение одиночного символа шириной в байт даёт точки,
 *          отличающиеся лишь позицией, убывающей на единицу, поэтому ряд хранится
 *          позицией последнего символа вместе с количеством оставшихся, а возврат
 *          разбирает такую точку на месте, не снимая её с набора. Порядок перебора
 *          длин повторения при этом остаётся прежним. Ряд символов разной ширины
 *          такого хранения не допускает: позиция предшествующего символа
 *          из позиции последующего вычитанием единицы не выводится, и в режиме
 *          разбора UTF-8 точки размещаются по-прежнему на каждый символ.
 *          Замером на выражении «.*needle» получено превосходство в полтора раза,
 *          решение закреплено тестом «Regex.EngineRepeatRun».
 *
 *          <b>Ряд повторения проходится поиском его границы, а не сопоставлением
 *          символов по одному.</b> Вне режима разбора UTF-8 повторение любого символа
 *          доходит до ближайшего перевода строки либо до конца текста, а повторение
 *          символов класса - до первого байта, классу не принадлежащего. Обе границы
 *          отыскиваются набором команд процессора над несколькими байтами сразу
 *          либо единственным обращением к таблице принадлежности, тогда как
 *          сопоставление посимвольное разбирает инструкцию на каждом байте.
 *          Замером получено превосходство в двенадцать раз на выражении «.*needle»
 *          и до трети на выражениях с классами, решение закреплено тестом
 *          «Regex.EngineRepeatSweep».
 *
 *          <b>Таблица принадлежности байтов удерживается для последнего встреченного
 *          класса и отменяется при смене программы.</b> Удержание оправдано тем, что
 *          проход ряда обращается к одному классу на каждом символе. Отмена же
 *          обязательна: ключом таблицы служит адрес класса, а набор классов
 *          принадлежит программе, отчего адрес класса освобождённой программы
 *          способен совпасть с адресом класса иной. Отказ от отмены пробовался
 *          и даёт расхождения с эталонной реализацией на трети выражений
 *          с классами.
 *
 * \~english
 * @brief Header file of the execution of regular expressions with backtracking — the Backtrack class,
 *        which executes the program by a single state while saving backtracking points,
 *        which allows matching constructs outside the regular subset of the syntax
 * @section backtrack_decisions Deliberate decisions
 * @details What is listed below looks like an incongruity, but was chosen deliberately and
 *          is not subject to correction. The section is introduced so that reading the code does not start
 *          every time from the same conclusions.
 *          <b>A run of backtracking points of a repetition of a single character is kept as a single
 *          point.</b> A repetition of a single character one byte wide yields points
 *          that differ only by the position, decreasing by one, therefore the run is kept
 *          as the position of the last character together with the number of the remaining ones, and backtracking
 *          takes such a point apart in place without removing it from the set. The order of enumerating
 *          the repetition lengths remains the former one. A run of characters of different widths
 *          admits no such keeping: the position of the preceding character
 *          is not derived from the position of the following one by subtracting one, and in the UTF-8
 *          parsing mode the points are placed as before, one per character.
 *          Measurement on the «.*needle» expression yielded a one-and-a-half-fold advantage,
 *          the decision is fixed by the «Regex.EngineRepeatRun» test.
 *          <b>A run of a repetition is walked by searching for its boundary rather than by matching
 *          the characters one by one.</b> Outside the UTF-8 parsing mode a repetition of any character
 *          reaches the nearest line feed or the end of the text, and a repetition
 *          of the characters of a class the first byte not belonging to the class. Both boundaries
 *          are located by processor instructions over several bytes at once
 *          or by a single reference to the belonging table, whereas
 *          character-by-character matching takes the instruction apart at every byte.
 *          Measurement yielded a twelvefold advantage on the «.*needle» expression
 *          and up to a third on the expressions with classes, the decision is fixed by the
 *          «Regex.EngineRepeatSweep» test.
 *          <b>The byte belonging table is held for the last encountered
 *          class and is cancelled when the program changes.</b> The holding is justified by the fact that
 *          walking a run refers to one class at every character. The cancellation, on the other hand,
 *          is mandatory: the key of the table is the address of the class, and the set of classes
 *          belongs to the program, which is why the address of a class of a released program
 *          is able to coincide with the address of a class of another one. Giving up the cancellation was tried
 *          and yields divergences from the reference implementation on a third of the expressions
 *          with classes.
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_REGEX_BACKTRACK__
#define __AWH_REGEX_BACKTRACK__

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <cstdint>
#include <utility>
#include <string_view>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "pike.hpp"
#include "text.hpp"
#include "program.hpp"

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
		 * @brief Наибольшее допустимое количество шагов сопоставления
		 *
		 * @details Исполнение с возвратом требует времени, растущего с длиной текста
		 *          показательно на выражениях с вложенными кванторами, поэтому объём
		 *          работы ограничивается. Исчерпание предела прекращает сопоставление
		 *          с ошибкой, а не молчаливым отказом от совпадения.
		 *
		 * \~english
		 * @brief Largest admissible number of matching steps
		 * @details Execution with backtracking requires time growing with the length of the text
		 *          exponentially on expressions with nested quantifiers, therefore the amount
		 *          of work is bounded. Exhausting the limit stops the matching
		 *          with an error rather than with a silent refusal of a match.
		 *
		 * \~
		 */
		constexpr size_t MAX_STEPS = 0x989680;

		/**
		 * \~russian
		 * @brief Наибольшее допустимое количество точек возврата
		 *
		 * @details Предел количества шагов ограничивает время сопоставления, но не
		 *          занимаемую им память: каждый переход по двум ветвям размещает точку
		 *          возврата, поэтому их количество ограничивается отдельно.
		 *
		 * \~english
		 * @brief Largest admissible number of backtracking points
		 * @details The limit on the number of steps bounds the time of the matching, but not
		 *          the memory it occupies: every two-branch jump places a backtracking
		 *          point, therefore their number is bounded separately.
		 *
		 * \~
		 */
		constexpr size_t MAX_POINTS = 0x100000;

		/**
		 * \~russian
		 * @brief Наибольшее допустимое количество кадров рекурсивных вызовов
		 *
		 * @details Кадр вызова сохраняется до отмены вызова возвратом, поэтому их
		 *          количество растёт с числом выполненных вызовов, а не с глубиной
		 *          вложенности, и ограничивается отдельно от неё.
		 *
		 * \~english
		 * @brief Largest admissible number of recursive call frames
		 * @details A call frame is kept until the call is cancelled by a return, therefore their
		 *          number grows with the number of performed calls rather than with the nesting
		 *          depth, and is bounded separately from it.
		 *
		 * \~
		 */
		constexpr size_t MAX_FRAMES = 0x100000;

		/**
		 * \~russian
		 * @brief Наибольшее допустимое количество записей журнала изменений
		 *
		 * @details Возврат из рекурсивного вызова отменяет выполненные им захваты
		 *          повторной записью прежних значений, что дописывает в журнал его
		 *          собственный хвост. Вложенные вызовы наращивают журнал показательно,
		 *          поэтому его размер ограничивается отдельно.
		 *
		 * \~english
		 * @brief Largest admissible number of change log records
		 * @details A return from a recursive call cancels the captures it performed
		 *          by writing the former values again, which appends its own tail to the
		 *          log. Nested calls grow the log exponentially,
		 *          therefore its size is bounded separately.
		 *
		 * \~
		 */
		constexpr size_t MAX_JOURNAL = 0x400000;

		/**
		 * \~russian
		 * @brief Наибольшая допустимая глубина рекурсивных вызовов
		 *
		 * @details Рекурсивный вызов подвыражения способен не продвигаться по тексту,
		 *          поэтому глубина вызовов ограничивается независимо от объёма работы.
		 *
		 * \~english
		 * @brief Largest admissible depth of recursive calls
		 * @details A recursive call of a subexpression is able not to advance through the text,
		 *          therefore the depth of the calls is bounded independently of the amount of work.
		 *
		 * \~
		 */
		constexpr size_t MAX_RECURSION = 1000;
		/**
		 * \~russian
		 * @brief Наибольшая допустимая вложенность исполнений программы
		 *
		 * @details Тело проверки окружения исполняется вложенным исполнением,
		 *          и вложенность его ограничена стеком машины, а не памятью
		 *          сопоставления. Программа, тело проверки какой указывает
		 *          на неё же, дала бы вложенность бесконечную и исчерпание
		 *          стека: собранная программа такого не содержит, а поддельная
		 *          запись хранилища - вполне.
		 *
		 * \~english
		 * @brief Largest admissible nesting of executions of the program
		 * @details The body of a lookaround assertion is executed by a nested
		 *          execution, and its nesting is bounded by the stack of the machine
		 *          rather than by the memory of the matching. A program whose body
		 *          of an assertion points at itself would give an infinite nesting
		 *          and an exhaustion of the stack: a built program contains no such
		 *          thing, whereas a forged record of the storage may well.
		 *
		 * \~
		 */
		constexpr size_t MAX_NESTED = 256;

		/**
		 * \~russian
		 * @brief Класс исполнения регулярного выражения с возвратом
		 *
		 * @details Класс исполняет программу единственным состоянием, сохраняя точки
		 *          возврата при переходе по двум ветвям и возвращаясь к ним при отказе
		 *          сопоставления. В отличие от исполнения без возврата, способ допускает
		 *          конструкции, требующие обращения к ранее захваченному тексту, но
		 *          требует времени, растущего с длиной текста показательно.
		 *
		 * \~english
		 * @brief Class of the execution of a regular expression with backtracking
		 * @details The class executes the program by a single state, saving backtracking
		 *          points at a two-branch jump and returning to them on a matching
		 *          failure. Unlike execution without backtracking, the way admits
		 *          constructs requiring a reference to previously captured text, but
		 *          requires time growing with the length of the text exponentially.
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Backtrack {
			private:
				/**
				 * \~russian
				 * @brief Точка возврата исполнения программы
				 *
				 * \~english
				 * @brief Backtracking point of the execution of the program
				 *
				 * \~
				 */
				typedef struct Point {
					/**
					 * \~russian
					 * Флаг восстановления кадра рекурсивного вызова
					 *
					 * @details Точка возврата с установленным флагом не продолжает
					 *          исполнения, а восстанавливает исполняемый рекурсивный
					 *          вызов, после чего возврат продолжается далее.
					 *
					 * \~english
					 * Flag of restoring a recursive call frame
					 * @details A backtracking point with the flag set does not continue
					 *          the execution but restores the executed recursive
					 *          call, after which the backtracking continues further.
					 *
					 * \~
					 */
					bool frame;
					// Адрес инструкции, с которой продолжается исполнение
					address_t pc;
					// Позиция в тексте, с которой продолжается исполнение
					size_t pos;
					// Размер журнала изменений ячеек захвата на момент сохранения
					size_t journal;
					// Размер журнала изменений отметок атомарных групп на момент сохранения
					size_t remarks;
					/**
					 * \~russian
					 * Количество оставшихся позиций ряда повторения одиночного символа
					 *
					 * @details Повторение одиночного символа шириной в один байт даёт
					 *          ряд точек возврата, отличающихся лишь позицией, убывающей
					 *          на единицу. Такой ряд хранится единственной точкой,
					 *          отчего проход ряда длиной в текст размещает одну точку
					 *          взамен точки на каждый байт текста.
					 *
					 * \~english
					 * Number of the remaining positions of a run of a repetition of a single character
					 * @details A repetition of a single character one byte wide yields
					 *          a run of backtracking points differing only by the position, decreasing
					 *          by one. Such a run is kept as a single point,
					 *          which is why walking a run as long as the text places one point
					 *          instead of a point per every byte of the text.
					 *
					 * \~
					 */
					size_t span;
					/**
					 * \~russian
					 * Вид глагола управления, точку разместившего
					 *
					 * @details Значение выводится увеличенным на единицу, а нуль
					 *          означает точку обыкновенную: глаголы управления
					 *          размещают точку, исполнения не продолжающую, - возврат
					 *          в неё прекращает попытку сопоставления, а вид глагола
					 *          указывает, как её продолжать.
					 *
					 * \~english
					 * Kind of the control verb that placed the point
					 * @details The value is yielded increased by one, whereas zero
					 *          means an ordinary point: the control verbs place a point
					 *          that does not continue the execution — backtracking into it
					 *          terminates the matching attempt, and the kind of the verb
					 *          tells how to continue it.
					 *
					 * \~
					 */
					uint8_t control;
					// Номер ячейки отметки ветви охватывающей группы глагола перехода
					uint32_t cell;
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
					Point() noexcept : frame(false), pc(0), pos(0), journal(0), span(0), control(0), cell(0) {}
				} point_t;
			private:
				/**
				 * \~russian
				 * @brief Запись журнала изменений ячеек захвата
				 *
				 * @details Журнал сохраняет прежние значения изменённых ячеек, благодаря
				 *          чему возврат восстанавливает состояние захвата без хранения
				 *          набора ячеек целиком в каждой точке возврата.
				 *
				 * \~english
				 * @brief Record of the change log of the capture cells
				 * @details The log keeps the former values of the changed cells, thanks to
				 *          which backtracking restores the state of the capture without keeping
				 *          the whole set of cells at every backtracking point.
				 *
				 * \~
				 */
				typedef struct Change {
					// Номер изменённой ячейки захвата
					uint32_t slot;
					// Прежнее значение изменённой ячейки захвата
					size_t value;
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
					Change() noexcept : slot(0), value(0) {}
				} change_t;
			private:
				// Исполняемая программа регулярного выражения
				const program_t * _program;
			private:
				// Текст, по которому выполняется сопоставление
				string_view _text;
			private:
				// Позиция начала текущей попытки сопоставления
				size_t _start;
			private:
				// Количество выполненных шагов сопоставления
				size_t _steps;
			private:
				/**
				 * \~russian
				 * Допустимое количество шагов сопоставления
				 *
				 * @details Объём действует на одно сопоставление и восстанавливается
				 *          предельным по его завершении.
				 *
				 * \~english
				 * Admissible number of matching steps
				 * @details The amount acts on one match and is restored to
				 *          the limiting one on its completion.
				 *
				 * \~
				 */
				size_t _budget;
			private:
				/**
				 * \~russian
				 * Наибольший допустимый объём работы сопоставления
				 *
				 * @details Потолок обрезает объём, вычисленный вызывающей стороною, и
				 *          устанавливается ею же: умолчанием служит «MAX_STEPS».
				 *
				 * \~english
				 * Largest admissible amount of work of the matching
				 * @details The ceiling truncates the amount computed by the calling side and
				 *          is set by it as well: «MAX_STEPS» serves as the default.
				 *
				 * \~
				 */
				size_t _ceiling;
			private:
				// Действующий объём работы текущего сопоставления
				size_t _limit;
			private:
				// Наибольшая допустимая глубина рекурсивных вызовов подвыражений
				size_t _nesting;
			private:
				/**
				 * \~russian
				 * Действующая наибольшая глубина рекурсивных вызовов сопоставления
				 *
				 * @details Глубина берётся наименьшей из заданной вызывающей стороной
				 *          и заданной самим выражением указанием «(*LIMIT_DEPTH=N)»:
				 *          предел выражения понижает предел вызывающей стороны,
				 *          но не повышает его.
				 *
				 * \~english
				 * Effective largest depth of the recursive calls of the matching
				 * @details The depth is taken as the smallest of the one set by the calling side
				 *          and the one set by the expression itself by the «(*LIMIT_DEPTH=N)» option:
				 *          the limit of the expression lowers the limit of the calling side
				 *          but never raises it.
				 *
				 * \~
				 */
				size_t _deepest;
			private:
				/**
				 * \~russian
				 * Действующий наибольший объём памяти сопоставления в байтах
				 *
				 * @details Объём задаётся самим выражением указанием «(*LIMIT_HEAP=N)»
				 *          и считается по наборам точек возврата, кадров вызовов
				 *          и записей журнала наравне с прочими пределами их размеров.
				 *
				 * \~english
				 * Effective largest amount of the matching memory in bytes
				 * @details The amount is set by the expression itself by the «(*LIMIT_HEAP=N)» option
				 *          and is counted over the sets of the backtracking points, the call frames
				 *          and the journal entries along with the other limits of their sizes.
				 *
				 * \~
				 */
				size_t _memory;
			private:
				/**
				 * \~russian
				 * Вид глагола управления, попытку сопоставления прекратившего
				 *
				 * @details Значение выводится увеличенным на единицу наравне с точкой
				 *          возврата, а нуль означает прекращение обыкновенное. Внешний
				 *          обход позиций начала читает его и решает, продолжать ли
				 *          попытки: глагол отказа целиком их прекращает, а глаголы
				 *          переноса задают позицию продолжения.
				 *
				 * \~english
				 * Kind of the control verb that terminated the matching attempt
				 * @details The value is yielded increased by one along with the backtracking point,
				 *          whereas zero means an ordinary termination. The outer walk over the starting
				 *          positions reads it and decides whether to continue the attempts:
				 *          the verb of the whole refusal terminates them, whereas the verbs of moving
				 *          set the position of continuation.
				 *
				 * \~
				 */
				uint8_t _control;
			private:
				// Позиция продолжения попытки сопоставления глаголом переноса
				size_t _resume;
			private:
				/**
				 * \~russian
				 * Адрес глагола отметки, попыткою последней пройденного
				 *
				 * @details Ячейка отметки возвратом отменяется, отчего по отказу
				 *          сопоставления в ней ничего не остаётся. Эталонная же
				 *          реализация имя отметки выводит и при отказе - отметку
				 *          последнюю попытки последней, - и адрес ведётся потому
				 *          отдельно: возвратом он не отменяется, началом попытки
				 *          очищается, а глаголом отсечения снимается.
				 *
				 * \~english
				 * Address of the mark verb passed by the last attempt
				 * @details The mark cell is undone by backtracking, whereby nothing remains in it
				 *          upon a failure of the matching. The reference implementation, however,
				 *          yields the name of the mark upon a failure as well — the last mark
				 *          of the last attempt — and the address is therefore maintained
				 *          separately: it is not undone by backtracking, is cleared at the start
				 *          of an attempt and is removed by a cutting verb.
				 *
				 * \~
				 */
				size_t _failing;
			private:
				// Действующая вложенность исполнений программы
				size_t _nested;
			private:
				/**
				 * \~russian
				 * Класс символов, для какого построена таблица принадлежности байтов
				 *
				 * @details Проход ряда повторения обращается к одному и тому же классу
				 *          на каждом символе, поэтому таблица удерживается для последнего
				 *          встреченного класса. Набор режимов сохраняется вместе с ним:
				 *          принадлежность зависит от режима сопоставления без учёта
				 *          регистра наравне с самим классом.
				 *
				 * \~english
				 * The character class the byte belonging table is built for
				 * @details Walking a run of a repetition refers to one and the same class
				 *          at every character, therefore the table is held for the last
				 *          encountered class. The set of modes is kept together with it:
				 *          the belonging depends on the case-insensitive matching mode
				 *          on a par with the class itself.
				 *
				 * \~
				 */
				uint32_t _member;
			private:
				// Набор режимов, при каком построена таблица принадлежности байтов
				uint32_t _modes;
			private:
				// Опознание программы, для какой построена таблица принадлежности байтов
				uint64_t _identity;
			private:
				// Таблица принадлежности значений байта классу символов
				uint8_t _bytes[0x100];
			private:
				// Набор точек возврата исполнения программы
				vector <point_t> _points;
			private:
				// Журнал изменений ячеек захвата групп
				vector <change_t> _journal;
			private:
				// Набор позиций захвата групп и ячеек состояния исполнения
				vector <size_t> _slots;
			private:
				/**
				 * \~russian
				 * Набор отметок состояния возврата атомарных конструкций
				 *
				 * @details Отметка сохраняет глубину набора точек возврата на входе
				 *          в атомарную конструкцию, благодаря чему её завершение
				 *          отказывается от точек возврата, накопленных внутри неё.
				 *
				 * \~english
				 * Set of the marks of the backtracking state of the atomic constructs
				 * @details A mark keeps the depth of the set of backtracking points at the entry
				 *          into an atomic construct, thanks to which its completion
				 *          gives up the backtracking points accumulated inside it.
				 *
				 * \~
				 */
				vector <size_t> _marks;
			private:
				/**
				 * \~russian
				 * @brief Изменение отметки атомарной группы
				 *
				 * @details Ячейка отметки одна на всю программу, а рекурсивный
				 *          вызов входит в ту же атомарную группу заново
				 *          и ячейку перезаписывает. Оттого отсечение уровня
				 *          внешнего брало глубину уровня внутреннего и точек
				 *          возврата не отсекало вовсе. Изменения ведутся
				 *          журналом наравне с ячейками захвата: кадр вызова
				 *          и точка возврата держат отсечку журнала, а возврат
				 *          восстанавливает прежние значения отметок.
				 *
				 * \~english
				 * @brief Change of an atomic group mark
				 * @details The mark cell is a single one for the whole program, while
				 *          a recursive call enters the same atomic group anew
				 *          and overwrites the cell. Because of that the cut of an outer level
				 *          took the depth of an inner level and cut no backtracking
				 *          points at all. Changes are kept in a journal alongside
				 *          the capture cells: the call frame and the backtracking point
				 *          keep the journal watermark, and backtracking
				 *          restores the previous values of the marks.
				 *
				 * \~
				 */
				typedef struct Remark {
					// Номер ячейки изменённой отметки
					uint32_t cell;
					// Прежнее значение отметки
					size_t value;
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
					Remark() noexcept : cell(0), value(0) {}
				} remark_t;
			private:
				// Журнал изменений отметок атомарных групп
				vector <remark_t> _remarks;
			private:
				/**
				 * \~russian
				 * @brief Кадр исполняемого рекурсивного вызова подвыражения
				 *
				 * \~english
				 * @brief Frame of an executed recursive call of a subexpression
				 *
				 * \~
				 */
				typedef struct Frame {
					// Адрес инструкции, к которой возвращается исполнение
					address_t back;
					// Номер группы, рекурсивный вызов которой исполняется
					uint32_t number;
					// Размер журнала изменений ячеек захвата на момент вызова
					size_t journal;
					// Размер журнала изменений отметок атомарных групп на момент вызова
					size_t remarks;
					// Номер кадра вызова, из которого выполнен рекурсивный вызов
					size_t parent;
					// Глубина рекурсивного вызова, отсчитываемая с единицы
					size_t depth;
					/**
					 * \~russian
					 * Позиция текста, в какой рекурсивный вызов начат
					 *
					 * @details Позиция служит распознаванию вызова, вошедшего заново
					 *          в той же позиции текста: такой вызов повторяет уже
					 *          выполняемое и завершиться не может.
					 *
					 * \~english
					 * Position of the text at which the recursive call was started
					 * @details The position serves to recognise a call that has entered anew
					 *          at the same position of the text: such a call repeats what is
					 *          already being executed and cannot complete.
					 *
					 * \~
					 */
					size_t pos;
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
					Frame() noexcept : back(0), number(0), journal(0), parent(0), depth(0), pos(0) {}
				} frame_t;
			private:
				// Набор кадров рекурсивных вызовов подвыражений
				vector <frame_t> _frames;
			private:
				// Номер кадра исполняемого рекурсивного вызова
				size_t _current;
			private:
				// Набор изменений ячеек захвата, отменяемых возвратом из вызова
				vector <change_t> _undo;
			private:
				// Код ошибки последней операции сопоставления
				error_t _error;
			public:
				/**
				 * \~russian
				 * @brief Метод сопоставления регулярного выражения с текстом
				 *
				 * @param program  исполняемая программа регулярного выражения
				 * @param text     текст для сопоставления
				 * @param start    позиция начала поиска совпадения
				 * @param captures набор границ совпадения и захваченных групп
				 * @return         результат поиска совпадения
				 *
				 * \~english
				 * @brief Method of matching a regular expression against a text
				 * @param program  program of the regular expression being executed
				 * @param text     text to match
				 * @param start    position to start the search for a match from
				 * @param captures set of the boundaries of the match and of the captured groups
				 * @return         result of searching for a match
				 *
				 * \~
				 */
				bool exec(const program_t & program, string_view text, const size_t start, vector <pair <size_t, size_t>> & captures) noexcept;
				/**
				 * \~russian
				 * @brief Метод сопоставления регулярного выражения с текстом в заданном режиме
				 *
				 * @param program  исполняемая программа регулярного выражения
				 * @param text     текст для сопоставления
				 * @param start    позиция начала поиска совпадения
				 * @param captures набор границ совпадения и захваченных групп
				 * @param mode     режим сопоставления регулярного выражения с текстом
				 * @return         результат поиска совпадения
				 *
				 * \~english
				 * @brief Method of matching a regular expression against a text in the given mode
				 * @param program  program of the regular expression being executed
				 * @param text     text to match
				 * @param start    position to start the search for a match from
				 * @param captures set of the boundaries of the match and of the captured groups
				 * @param mode     mode of matching the regular expression against the text
				 * @return         result of searching for a match
				 *
				 * \~
				 */
				bool exec(const program_t & program, string_view text, const size_t start, vector <pair <size_t, size_t>> & captures, const mode_t mode) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод сопоставления символа одиночной инструкцией
				 *
				 * @details Метод применяется при проходе ряда подходящих символов
				 *          и сопоставляет лишь инструкции, продвигающиеся по тексту
				 *          независимо от состояния исполнения.
				 *
				 * @param instruction сопоставляющая инструкция программы
				 * @param pos         позиция сопоставления в тексте
				 * @param width       длина сопоставленного символа в байтах
				 * @return            результат сопоставления символа инструкцией
				 *
				 * \~english
				 * @brief Method of matching a character by a single instruction
				 * @details The method is used when walking a run of matching characters
				 *          and matches only the instructions that advance through the text
				 *          independently of the state of the execution.
				 * @param instruction matching instruction of the program
				 * @param pos         matching position in the text
				 * @param width       length of the matched character in bytes
				 * @return            result of matching the character by the instruction
				 *
				 * \~
				 */
				bool single(const instruction_t & instruction, const size_t pos, size_t & width) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки допустимого объёма работы сопоставления
				 *
				 * @details Установленный объём действует на одно последующее сопоставление,
				 *          после которого восстанавливается предельный. Уменьшенный объём
				 *          требуется вызывающей стороне, располагающей запасным способом
				 *          сопоставления: исчерпание объёма прекращает исполнение с ошибкой
				 *          «BUDGET_EXCEEDED», и сопоставление выполняется запасным способом,
				 *          время которого не зависит от вида выражения.
				 *
				 * @param budget допустимое количество шагов сопоставления
				 *
				 * \~english
				 * @brief Method of setting the admissible amount of work of the matching
				 * @details The set amount acts on one subsequent match,
				 *          after which the limiting one is restored. A reduced amount
				 *          is required by a calling side that has a fallback way of
				 *          matching at its disposal: exhausting the amount stops the execution with the
				 *          «BUDGET_EXCEEDED» error, and the matching is performed by the fallback way,
				 *          whose time does not depend on the kind of the expression.
				 * @param budget admissible number of matching steps
				 *
				 * \~
				 */
				void budget(const size_t budget) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки наибольшего допустимого объёма работы сопоставления
				 *
				 * @details Потолок обрезает объём, вычисленный вызывающей стороною от длины
				 *          текста и длины программы, и действует на все последующие
				 *          сопоставления, а не на одно. Нуль потолок снимает вовсе.
				 *
				 * @param ceiling наибольшее допустимое количество шагов сопоставления
				 *
				 * \~english
				 * @brief Method of setting the largest admissible amount of work of the matching
				 * @details The ceiling truncates the amount computed by the calling side from the length
				 *          of the text and the length of the program, and acts on all subsequent
				 *          matches rather than on one. Zero removes the ceiling entirely.
				 * @param ceiling largest admissible number of matching steps
				 *
				 * \~
				 */
				void ceiling(const size_t ceiling) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки наибольшей допустимой глубины рекурсивных вызовов
				 *
				 * @details Глубина ограничивает вложенность рекурсивных вызовов подвыражений:
				 *          память под кадры вызовов растёт соразмерно ей. Нуль восстанавливает
				 *          глубину умолчания.
				 *
				 * @param nesting наибольшая допустимая глубина рекурсивных вызовов
				 *
				 * \~english
				 * @brief Method of setting the largest admissible depth of recursive calls
				 * @details The depth bounds the nesting of recursive calls of subpatterns:
				 *          the memory of the call frames grows in proportion to it. Zero
				 *          restores the depth of the default.
				 * @param nesting largest admissible depth of recursive calls
				 *
				 * \~
				 */
				void nesting(const size_t nesting) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения кода ошибки последней операции
				 *
				 * @details Код ошибки «BUDGET_EXCEEDED» означает исчерпание допустимого
				 *          объёма работы сопоставления, при котором отсутствие совпадения
				 *          не установлено.
				 *
				 * @return код ошибки последней операции сопоставления
				 *
				 * \~english
				 * @brief Method of getting the error code of the last operation
				 * @details The «BUDGET_EXCEEDED» error code means the exhaustion of the admissible
				 *          amount of work of the matching, at which the absence of a match
				 *          is not established.
				 * @return error code of the last matching operation
				 *
				 * \~
				 */
				error_t error() const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения адреса глагола отметки совпадения последнего
				 *
				 * @details Адрес берётся из ячейки отметки последней, ведомой наравне
				 *          с ячейками захвата: по совпадении в ней остаётся глагол
				 *          пути, совпадение давшего. Значение недостижимое означает
				 *          совпадение, глаголов отметки не прошедшее.
				 *
				 * @return адрес глагола отметки совпадения последнего
				 *
				 * \~english
				 * @brief Method of getting the address of the mark verb of the last match
				 * @details The address is taken from the cell of the last mark, maintained along
				 *          with the capture cells: upon a match it holds the verb of the path
				 *          that produced the match. An unreachable value means a match
				 *          that passed no mark verbs.
				 * @return address of the mark verb of the last match
				 *
				 * \~
				 */
				size_t marked() const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения адреса глагола отметки попытки последней
				 *
				 * @return адрес глагола отметки попытки последней
				 *
				 * \~english
				 * @brief Method of getting the address of the mark verb of the last attempt
				 * @return address of the mark verb of the last attempt
				 *
				 * \~
				 */
				size_t failed() const noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод выполнения попытки сопоставления с заданной позиции
				 *
				 * @details Попытка исполняет программу с её первой инструкции, сохраняя
				 *          точки возврата и возвращаясь к ним при отказе сопоставления.
				 *          Исчерпание точек возврата означает отказ попытки.
				 *
				 * @param pos позиция начала попытки сопоставления
				 * @return    результат выполнения попытки сопоставления
				 *
				 * \~english
				 * @brief Method of performing a match attempt from the given position
				 * @details The attempt executes the program from its first instruction, saving
				 *          backtracking points and returning to them on a matching failure.
				 *          Exhausting the backtracking points means the failure of the attempt.
				 * @param pos position where the match attempt begins
				 * @return    result of performing the match attempt
				 *
				 * \~
				 */
				bool attempt(const size_t pos) noexcept;
				/**
				 * \~russian
				 * @brief Метод исполнения программы с заданной инструкции
				 *
				 * @details Исполнение сохраняет точки возврата в общем наборе, не опускаясь
				 *          ниже заданной глубины, благодаря чему вложенные исполнения
				 *          проверок окружения и рекурсивных вызовов не расходуют памяти
				 *          на собственные наборы. Завершение исполнения отказывается
				 *          от накопленных точек возврата, что соответствует запрету
				 *          возврата внутрь проверок окружения и рекурсивных вызовов.
				 *
				 * @param pc    адрес инструкции, с которой начинается исполнение
				 * @param pos   позиция в тексте, с которой начинается исполнение
				 * @param base  глубина набора точек возврата, ниже которой возврат недопустим
				 * @param bound позиция, в которой обязано завершиться исполнение
				 * @param end   позиция завершения исполнения программы
				 * @return      результат исполнения программы
				 *
				 * \~english
				 * @brief Method of executing the program from the given instruction
				 * @details The execution saves the backtracking points in a common set without descending
				 *          below the given depth, thanks to which the nested executions
				 *          of lookarounds and of recursive calls spend no memory
				 *          on sets of their own. Finishing the execution gives up
				 *          the accumulated backtracking points, which corresponds to the prohibition
				 *          of backtracking into lookarounds and recursive calls.
				 * @param pc    address of the instruction the execution starts from
				 * @param pos   position in the text the execution starts from
				 * @param base  depth of the set of backtracking points below which backtracking is inadmissible
				 * @param bound position at which the execution is obliged to finish
				 * @param end   position where the execution of the program finishes
				 * @return      result of executing the program
				 *
				 * \~
				 */
				bool run(const address_t pc, const size_t pos, const size_t base, const size_t bound, size_t & end) noexcept;
				/**
				 * \~russian
				 * @brief Метод сопоставления текста, захваченного группой
				 *
				 * @param number номер группы, захваченный текст которой сопоставляется
				 * @param flags  набор режимов компиляции инструкции
				 * @param pos    позиция сопоставления в тексте
				 * @param length длина сопоставленного захваченного текста
				 * @return       результат сопоставления захваченного текста
				 *
				 * \~english
				 * @brief Method of matching the text captured by a group
				 * @param number number of the group whose captured text is matched
				 * @param flags  set of compilation modes of the instruction
				 * @param pos    matching position in the text
				 * @param length length of the matched captured text
				 * @return       result of matching the captured text
				 *
				 * \~
				 */
				bool matches(const uint32_t number, const uint32_t flags, const size_t pos, size_t & length) const noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод сохранения позиции в ячейке захвата
				 *
				 * @details Прежнее значение ячейки размещается в журнале изменений,
				 *          благодаря чему возврат восстанавливает состояние захвата.
				 *
				 * @param slot  номер ячейки захвата
				 * @param value сохраняемая в ячейке захвата позиция в тексте
				 *
				 * \~english
				 * @brief Method of saving a position in a capture cell
				 * @details The former value of the cell is placed in the change log,
				 *          thanks to which backtracking restores the state of the capture.
				 * @param slot  number of the capture cell
				 * @param value position in the text saved in the capture cell
				 *
				 * \~
				 */
				void store(const uint32_t slot, const size_t value) noexcept;
				/**
				 * \~russian
				 * @brief Метод восстановления состояния захвата групп
				 *
				 * @param mark размер журнала изменений, до которого выполняется откат
				 *
				 * \~english
				 * @brief Method of restoring the state of the group capture
				 * @param mark size of the change log the rollback is performed down to
				 *
				 * \~
				 */
				void restore(const size_t mark) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод отката изменений отметок атомарных групп
				 *
				 * @details Откат ведётся по возврате из рекурсивного вызова
				 *          и по отмене его: ячейка отметки одна на всю программу,
				 *          и вызов, вошедший в ту же атомарную группу, значение
				 *          её перезаписывает.
				 *
				 * @param mark отсечка журнала изменений отметок
				 *
				 * \~english
				 * @brief Method of rolling back the changes of the atomic group marks
				 * @details The rollback is performed on returning from a recursive call
				 *          and on cancelling it: the mark cell is a single one for the whole program,
				 *          and a call that has entered the same atomic group overwrites
				 *          its value.
				 * @param mark watermark of the journal of the mark changes
				 *
				 * \~
				 */
				void rollback(const size_t mark) noexcept;
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
				Backtrack() noexcept;
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
				~Backtrack() noexcept {}
		} backtrack_t;
	};
};

#endif // __AWH_REGEX_BACKTRACK__
