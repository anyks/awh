/**
 * @file compiler.hpp
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
 * @brief Заголовочный файл компиляции регулярных выражений — класс Compiler, преобразующий
 *        синтаксическое дерево в программу недетерминированного конечного автомата,
 *        пригодную для исполнения без возврата
 *
 * @section compiler_decisions Намеренные решения
 *
 * @details Перечисленное ниже выглядит несообразностью, но выбрано осознанно и
 *          правке не подлежит. Раздел заведён затем, чтобы разбор кода не начинался
 *          каждый раз с одних и тех же выводов.
 *
 *          <b>Проверка продвижения по тексту размещается для всякого неограниченного
 *          повторения, кроме повторения одиночного символа.</b> Круг тел, для которых
 *          проверка опускается, выглядит излишне узким: пустого сопоставления не
 *          допускает и множество прочих тел. Однако пустое сопоставление тела,
 *          содержащего конструкции вне регулярного подмножества, выводится ненадёжно,
 *          а отсутствие проверки при пустом сопоставлении оставляет повторение
 *          без завершения вовсе. Одиночный символ, класс символов, любой символ
 *          и одиночная единица кодирования продвигаются по тексту при любом составе
 *          выражения, чем и ограничен круг. Расширение круга закреплено тестом
 *          «Regex.EngineProgress»: включение в него групп оставляет выражения теста
 *          без верных границ. Снятие проверки с повторения одиночного символа
 *          измерением даёт превосходство до полутора раз, поскольку избавляет
 *          каждый оборот повторения от сохранения позиции и её проверки.
 *
 *          <b>Повторения одиночного символа помечаются отдельным набором.</b>
 *          Пометка выглядит удвоением сведений, уже содержащихся в программе,
 *          но распознавание устройства повторения при исполнении обходилось бы
 *          в несколько проверок на каждый оборот, тогда как выполняется оно
 *          однократно при компиляции. Пометка позволяет исполнению с возвратом
 *          проходить ряд подходящих символов одним ходом взамен исполнения трёх
 *          инструкций на каждый символ, что измерением даёт ещё четверть.
 *
 * \~english
 * @brief Header file of the compilation of regular expressions — the Compiler class, which converts
 *        a syntax tree into a program of a nondeterministic finite automaton
 *        fit for execution without backtracking
 * @section compiler_decisions Deliberate decisions
 * @details What is listed below looks like an incongruity, but was chosen deliberately and
 *          is not subject to correction. The section is introduced so that reading the code does not start
 *          every time from the same conclusions.
 *          <b>The check of the advance through the text is placed for every unbounded
 *          repetition except the repetition of a single character.</b> The circle of bodies for which
 *          the check is omitted looks unduly narrow: many other bodies also admit
 *          no empty match. However, an empty match of a body
 *          holding constructs outside the regular subset is inferred unreliably,
 *          and the absence of the check on an empty match leaves the repetition
 *          without any termination at all. A single character, a character class, any character
 *          and a single encoding unit advance through the text with any composition
 *          of the expression, and that is what limits the circle. The extension of the circle is fixed by the test
 *          «Regex.EngineProgress»: including groups in it leaves the expressions of the test
 *          without correct boundaries. Removing the check from the repetition of a single character
 *          gives by measurement an advantage of up to one and a half times, since it relieves
 *          every turn of the repetition of saving the position and checking it.
 *          <b>Repetitions of a single character are marked by a separate set.</b>
 *          The mark looks like a duplication of information already held in the program,
 *          but recognising the arrangement of a repetition at execution time would cost
 *          several checks per every turn, whereas it is performed
 *          once at compilation. The mark allows backtracking execution
 *          to walk a run of matching characters in one move instead of executing three
 *          instructions per every character, which by measurement gives another quarter.
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_REGEX_COMPILER__
#define __AWH_REGEX_COMPILER__

/**
 * Стандартные заголовочные файлы
 */
#include <map>
#include <string>
#include <vector>
#include <cstdint>
#include <utility>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "parser.hpp"
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
		 * @brief Класс компиляции регулярного выражения
		 *
		 * @details Класс преобразует синтаксическое дерево в программу
		 *          недетерминированного конечного автомата. Компиляции подлежат
		 *          выражения, принадлежащие регулярному подмножеству синтаксиса PCRE.
		 *          Выражения, содержащие ссылки на захваченные группы, рекурсивные
		 *          вызовы, условные выражения, проверки окружения, атомарные группы
		 *          и захватывающие кванторы, регулярному подмножеству не принадлежат
		 *          и требуют исполнения с возвратом.
		 *
		 * \~english
		 * @brief Class of the compilation of a regular expression
		 * @details The class converts a syntax tree into a program of a
		 *          nondeterministic finite automaton. Subject to compilation are
		 *          the expressions belonging to the regular subset of the PCRE syntax.
		 *          Expressions holding references to captured groups, recursive
		 *          calls, conditional expressions, lookarounds, atomic groups
		 *          and possessive quantifiers do not belong to the regular subset
		 *          and require execution with backtracking.
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Compiler {
			private:
				// Объект разбора, предоставляющий синтаксическое дерево
				const Parser * _parser;
			private:
				/**
				 * \~russian
				 * @brief Арена узлов синтаксического дерева объекта разбора
				 *
				 * @details Арена сберегается указателем потому, что извлечение
				 *          узла по индексу происходит на каждом шаге построения,
				 *          а метод извлечения объекта разбора лежит в единице
				 *          трансляции чужой и подстановке не поддаётся. Обращение
				 *          через сбережённый указатель встраивается целиком, и
				 *          вызов подпрограммы на месте каждого обращения исчезает.
				 *
				 * \~english
				 * @brief Arena of the syntax tree nodes of the parser object
				 * @details The arena is kept as a pointer because getting a node by
				 *          index happens at every step of the compilation, while the
				 *          getter of the parser object lies in a foreign translation
				 *          unit and cannot be inlined. Access through the kept pointer
				 *          is inlined entirely, and the subroutine call at the place
				 *          of every access disappears.
				 *
				 * \~
				 */
				const vector <node_data_t> * _arena;
			private:
				// Компилируемая программа регулярного выражения
				program_t * _program;
			private:
				// Флаг компиляции развёрнутого регулярного выражения
				bool _reverse;
			private:
				/**
				 * \~russian
				 * Флаг компиляции выражения целиком
				 *
				 * @details Компиляция выражения целиком размещает инструкции конструкций,
				 *          не принадлежащих регулярному подмножеству синтаксиса, исполнение
				 *          которых доступно только способу исполнения с возвратом.
				 *
				 * \~english
				 * Flag of compiling the expression as a whole
				 * @details Compiling the expression as a whole places the instructions of the constructs
				 *          that do not belong to the regular subset of the syntax, whose execution
				 *          is available only to the way of execution with backtracking.
				 *
				 * \~
				 */
				bool _full;
			private:
				// Количество ячеек состояния, размещённых компиляцией
				uint32_t _cells;
			private:
				// Количество ячеек отметки состояния возврата
				uint32_t _atomics;
			private:
				// Набор адресов инструкций рекурсивного вызова и номеров вызываемых групп
				vector <pair <address_t, uint32_t>> _calls;
			private:
				/**
				 * \~russian
				 * Соответствие номеров групп адресам их рекурсивно вызываемых тел
				 *
				 * @details Соответствие держится плотным рядом, а не отображением
				 *          упорядоченным: номера захватывающих групп идут подряд
				 *          от единицы, и число их известно разбором. Отображение
				 *          размещало узел на каждую запись и уравновешивало
				 *          дерево, тогда как ряд берётся номером прямо, а место
				 *          под него отводится однажды и построения переживает.
				 *
				 *          Отсутствие записи означено приметою `INVALID_ADDRESS`.
				 *
				 * \~english
				 * Correspondence of the group numbers to the addresses of their recursively called bodies
				 * @details The correspondence is held by a dense sequence rather than an ordered map:
				 *          the numbers of the capturing groups run consecutively from one,
				 *          and their count is known from the parsing.
				 *
				 * \~
				 */
				vector <address_t> _sections;
			private:
				/**
				 * \~russian
				 * Соответствие номеров групп индексам их узлов синтаксического дерева
				 *
				 * @details Держится плотным рядом по тем же доводам, что и
				 *          соответствие адресов тел. Отсутствие записи означено
				 *          приметою `INVALID_NODE`.
				 *
				 * \~english
				 * Correspondence of the group numbers to the indices of their nodes of the syntax tree
				 * @details Held by a dense sequence for the same reasons as the correspondence
				 *          of the addresses of the bodies.
				 *
				 * \~
				 */
				vector <node_id_t> _groups;
			private:
				/**
				 * \~russian
				 * Сберегательный ряд адресов переходов, построением накапливаемых
				 *
				 * @details Выбор одной из ветвей и повторение с границами копят
				 *          адреса переходов прежде, чем разрешить их: число
				 *          ветвей и проходов заранее не известно. Ряд для того
				 *          заводился на каждый узел, а узлов таких в выражении
				 *          столько же, сколько чередований и повторителей.
				 *
				 *          Ряд ныне общий, а вложенность держится отметкою
				 *          основания: построение помнит длину ряда при входе,
				 *          копит поверх неё и усекает ряд обратно, разрешив
				 *          переходы. Место, однажды отведённое, переживает
				 *          и узлы, и построения.
				 *
				 * \~english
				 * Scratch sequence of the jump addresses accumulated by the building
				 * @details Alternation and bounded repetition accumulate the addresses
				 *          of the jumps before resolving them: the number of branches
				 *          and passes is not known in advance.
				 *
				 * \~
				 */
				vector <address_t> _exits;
			private:
				/**
				 * \~russian
				 * Сберегательный след узлов при проверке продвижения по тексту
				 *
				 * @details Проверка обязательного продвижения обходит дерево
				 *          со следом, вызовы рекурсивные различающим, и след
				 *          этот заводился на каждую проверку. Проверка зовётся
				 *          на всякое повторение неограниченное, а вложенности
				 *          не знает - оттого достаточно очистки, а не отметки.
				 *
				 * \~english
				 * Scratch trace of the nodes for the check of advancing over the text
				 * @details The check of the obligatory advancing walks the tree
				 *          with a trace telling the recursive calls apart, and that trace
				 *          was introduced for every check.
				 *
				 * \~
				 */
				mutable vector <node_id_t> _visited;
			private:
				/**
				 * \~russian
				 * Сберегательный ряд узлов цепочки при построении развёрнутом
				 *
				 * @details Развёрнутое выражение сопоставляет последовательность
				 *          элементов в обратном порядке, отчего узлы цепочки
				 *          строятся с конца, а связка узлов ведёт лишь вперёд -
				 *          цепочку приходится накапливать. Ряд для того
				 *          заводился на каждый уровень вложенности.
				 *
				 *          Ряд ныне общий, а вложенность держится отметкою
				 *          основания - тем же порядком, каким живут прочие
				 *          сберегательные ряды разбора и построения.
				 *
				 * \~english
				 * Scratch sequence of the nodes of a chain for the reversed building
				 * @details A reversed expression matches the sequence of the elements
				 *          in the reverse order, which is why the nodes of a chain
				 *          are built from the end, while the linking of the nodes leads only forward.
				 *
				 * \~
				 */
				vector <node_id_t> _chain;
			private:
				// Код ошибки последней операции компиляции
				error_t _error;
			public:
				/**
				 * \~russian
				 * @brief Метод компиляции регулярного выражения
				 *
				 * @details Компиляция выполняется по синтаксическому дереву, полученному
				 *          объектом разбора. Отказ компиляции с кодом ошибки «UNSUPPORTED»
				 *          означает принадлежность выражения нерегулярному подмножеству
				 *          и требует исполнения выражения с возвратом.
				 *
				 * @param parser  объект разбора регулярного выражения
				 * @param program компилируемая программа регулярного выражения
				 * @return        результат выполнения компиляции
				 *
				 * \~english
				 * @brief Method of compiling a regular expression
				 * @details The compilation is performed over the syntax tree obtained by
				 *          the parsing object. A compilation failure with the «UNSUPPORTED» error code
				 *          means that the expression belongs to the non-regular subset
				 *          and requires execution of the expression with backtracking.
				 * @param parser  parsing object of the regular expression
				 * @param program program of the regular expression being compiled
				 * @return        result of performing the compilation
				 *
				 * \~
				 */
				bool compile(const Parser & parser, program_t & program) noexcept;
				/**
				 * \~russian
				 * @brief Метод компиляции развёрнутого регулярного выражения
				 *
				 * @details Развёрнутая программа сопоставляет выражение при проходе по тексту
				 *          в обратном направлении: последовательности элементов и символов
				 *          размещаются в обратном порядке. Программа предназначена для поиска
				 *          позиции начала совпадения и захвата групп не выполняет.
				 *
				 * @param parser  объект разбора регулярного выражения
				 * @param program компилируемая программа регулярного выражения
				 * @return        результат выполнения компиляции
				 *
				 * \~english
				 * @brief Method of compiling a reversed regular expression
				 * @details A reversed program matches the expression while walking the text
				 *          backwards: the sequences of elements and of characters
				 *          are placed in reverse order. The program is intended for searching for
				 *          the position where a match begins and performs no capture of groups.
				 * @param parser  parsing object of the regular expression
				 * @param program program of the regular expression being compiled
				 * @return        result of performing the compilation
				 *
				 * \~
				 */
				bool compileReverse(const Parser & parser, program_t & program) noexcept;
				/**
				 * \~russian
				 * @brief Метод компиляции регулярного выражения целиком
				 *
				 * @details Компиляция размещает инструкции всех конструкций синтаксиса,
				 *          включая не принадлежащие регулярному подмножеству. Полученная
				 *          программа исполняется исключительно способом с возвратом.
				 *
				 * @param parser  объект разбора регулярного выражения
				 * @param program компилируемая программа регулярного выражения
				 * @return        результат выполнения компиляции
				 *
				 * \~english
				 * @brief Method of compiling a regular expression as a whole
				 * @details The compilation places the instructions of all the syntax constructs,
				 *          including those not belonging to the regular subset. The resulting
				 *          program is executed exclusively by the way with backtracking.
				 * @param parser  parsing object of the regular expression
				 * @param program program of the regular expression being compiled
				 * @return        result of performing the compilation
				 *
				 * \~
				 */
				bool compileFull(const Parser & parser, program_t & program) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения кода ошибки компиляции
				 *
				 * @return код ошибки последней операции компиляции
				 *
				 * \~english
				 * @brief Method of getting the compilation error code
				 * @return error code of the last compilation operation
				 *
				 * \~
				 */
				error_t error() const noexcept;
			private:
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
				 * @brief Метод компиляции цепочки узлов синтаксического дерева
				 *
				 * @param id индекс первого узла цепочки в арене узлов
				 * @return   результат выполнения компиляции
				 *
				 * \~english
				 * @brief Method of compiling a chain of nodes of the syntax tree
				 * @param id index of the first node of the chain in the node arena
				 * @return   result of performing the compilation
				 *
				 * \~
				 */
				bool compileChain(const node_id_t id) noexcept;
				/**
				 * \~russian
				 * @brief Метод компиляции узла синтаксического дерева
				 *
				 * @param id индекс узла в арене узлов
				 * @return   результат выполнения компиляции
				 *
				 * \~english
				 * @brief Method of compiling a node of the syntax tree
				 * @param id index of the node in the node arena
				 * @return   result of performing the compilation
				 *
				 * \~
				 */
				bool compileNode(const node_id_t id) noexcept;
				/**
				 * \~russian
				 * @brief Метод компиляции узла повторения
				 *
				 * @param id индекс узла повторения в арене узлов
				 * @return   результат выполнения компиляции
				 *
				 * \~english
				 * @brief Method of compiling a repetition node
				 * @param id index of the repetition node in the node arena
				 * @return   result of performing the compilation
				 *
				 * \~
				 */
				bool compileRepeat(const node_id_t id) noexcept;
				/**
				 * \~russian
				 * @brief Метод компиляции повторения элемента выражения
				 *
				 * @details Метод размещает инструкции повторения без учёта запрета
				 *          возврата внутрь повторяемого элемента, размещаемого
				 *          методом компиляции узла повторения.
				 *
				 * @param id индекс узла повторения в арене узлов
				 * @return   результат выполнения компиляции
				 *
				 * \~english
				 * @brief Method of compiling a repetition of an expression element
				 * @details The method places the instructions of the repetition without taking into account the prohibition
				 *          of backtracking into the repeated element, which is placed by
				 *          the method of compiling a repetition node.
				 * @param id index of the repetition node in the node arena
				 * @return   result of performing the compilation
				 *
				 * \~
				 */
				bool compileIteration(const node_id_t id) noexcept;
				/**
				 * \~russian
				 * @brief Метод компиляции узла выбора одной из ветвей
				 *
				 * @param id индекс узла выбора одной из ветвей в арене узлов
				 * @return   результат выполнения компиляции
				 *
				 * \~english
				 * @brief Method of compiling a node choosing one of the branches
				 * @param id index of the node choosing one of the branches in the node arena
				 * @return   result of performing the compilation
				 *
				 * \~
				 */
				/**
				 * \~russian
				 * @brief Метод свёртки ветвей выбора в один класс символов
				 *
				 * @details Выбор из одиночных символов и классов есть тот же класс,
				 *          объединением ветвей составленный, а порождение их различает:
				 *          выбор компилируется переходом по двум ветвям, и повторение
				 *          над ним идёт дорогим путём записи прохода вместо дешёвого
				 *          прохода ряда по таблице принадлежности байтов
				 *
				 * @param id индекс узла выбора одной из ветвей в арене узлов
				 * @return   результат выполнения свёртки ветвей
				 *
				 * \~english
				 * @brief Method of folding the branches of a choice into a single character class
				 * @details A choice among single characters and classes is the very same class
				 *          made up by the union of the branches, whereas the generation tells
				 *          them apart: a choice is compiled into a two-branch jump
				 *
				 * @param id index of the node of the choice of one of the branches in the arena of nodes
				 * @return   result of performing the folding of the branches
				 *
				 * \~
				 */
				bool merging(const node_id_t id) noexcept;
			/**
			 * \~russian
			 * @brief Метод свёртки повторения над необязательными частями в ряд символов
			 *
			 * @details Повторение вида «(?:A* B?)*», всякая часть тела какого необязательна,
			 *          принимает ровно те же строки, что и ряд «[A∪B]*»: тело набирает любую
			 *          часть по одной, а внешнее повторение повторяет это без счёта. Ряд же
			 *          проходится таблицей байтов одним ходом, тогда как повторение над
			 *          областью идёт записью кадра на всякий проход.
			 *
			 * @param id индекс узла повторения в арене узлов
			 * @return   результат выполнения свёртки повторения
			 *
			 * \~english
			 * @brief Method of folding a repetition over optional parts into a run of characters
			 * @details A repetition of the form «(?:A* B?)*», every part of whose body is optional,
			 *          accepts exactly the same strings as the run «[A∪B]*»: the body takes any
			 *          part one at a time, and the outer repetition repeats this without count. The run
			 *          is passed by a table of bytes in one go, whereas a repetition over a region
			 *          goes by a frame record on every pass.
			 * @param id index of the repetition node in the arena of nodes
			 * @return   result of performing the folding of the repetition
			 *
			 * \~
			 */
			bool flattening(const node_id_t id) noexcept;
				/**
				 * \~russian
				 * @brief Метод компиляции узла выбора одной из ветвей
				 *
				 * @param id индекс узла выбора одной из ветвей в арене узлов
				 * @return   результат выполнения компиляции
				 *
				 * \~english
				 * @brief Method of compiling the node of the choice of one of the branches
				 *
				 * @param id index of the node of the choice of one of the branches in the arena of nodes
				 * @return   result of performing the compilation
				 *
				 * \~
				 */
				bool compileAlternate(const node_id_t id) noexcept;
				/**
				 * \~russian
				 * @brief Метод компиляции узла проверки окружения
				 *
				 * @param id      индекс узла проверки окружения в арене узлов
				 * @param address адрес размещённой инструкции проверки окружения
				 * @return        результат выполнения компиляции
				 *
				 * \~english
				 * @brief Method of compiling a lookaround node
				 * @param id      index of the lookaround node in the node arena
				 * @param address address of the placed lookaround instruction
				 * @return        result of performing the compilation
				 *
				 * \~
				 */
				bool compileLook(const node_id_t id, address_t & address) noexcept;
				/**
				 * \~russian
				 * @brief Метод компиляции узла условного выражения
				 *
				 * @param id индекс узла условного выражения в арене узлов
				 * @return   результат выполнения компиляции
				 *
				 * \~english
				 * @brief Method of compiling a conditional expression node
				 * @param id index of the conditional expression node in the node arena
				 * @return   result of performing the compilation
				 *
				 * \~
				 */
				bool compileCondition(const node_id_t id) noexcept;
				/**
				 * \~russian
				 * @brief Метод компиляции тел рекурсивно вызываемых подвыражений
				 *
				 * @details Тела размещаются за основной программой отдельными разделами,
				 *          завершаемыми инструкцией возврата, после чего адреса разделов
				 *          устанавливаются в инструкциях рекурсивного вызова.
				 *
				 * @return результат выполнения компиляции
				 *
				 * \~english
				 * @brief Method of compiling the bodies of the recursively called subexpressions
				 * @details The bodies are placed after the main program as separate sections
				 *          terminated by a return instruction, after which the addresses of the sections
				 *          are set in the recursive call instructions.
				 * @return result of performing the compilation
				 *
				 * \~
				 */
				bool compileSections() noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод сбора узлов захватывающих групп выражения
				 *
				 * @param id индекс узла, с которого начинается сбор
				 *
				 * \~english
				 * @brief Method of collecting the nodes of the capturing groups of the expression
				 * @param id index of the node the collection starts from
				 *
				 * \~
				 */
				void collect(const node_id_t id) noexcept;
				/**
				 * \~russian
				 * @brief Метод размещения ячейки состояния исполнения
				 *
				 * @return номер ячейки состояния в наборе позиций захвата групп
				 *
				 * \~english
				 * @brief Method of allocating an execution state cell
				 * @return number of the state cell in the set of the group capture positions
				 *
				 * \~
				 */
				uint32_t reserve() noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод формирования предварительного отбора позиций
				 *
				 * @details Метод определяет набор байтов, допустимых в начале совпадения,
				 *          и обязательный литерал совпадения. Набор байтов не применяется,
				 *          если выражение допускает совпадение нулевой длины, поскольку
				 *          такое совпадение возможно в любой позиции текста.
				 *
				 * \~english
				 * @brief Method of building the preliminary selection of positions
				 * @details The method determines the set of bytes admissible at the beginning of a match
				 *          and the mandatory literal of a match. The set of bytes is not applied
				 *          if the expression admits a match of zero length, since
				 *          such a match is possible at any position of the text.
				 *
				 * \~
				 */
				void analyze() noexcept;
				/**
				 * \~russian
				 * @brief Метод пометки повторений одиночного символа
				 *
				 * @details Метод помечает переходы по двум ветвям, ветвь повторения которых
				 *          состоит из сопоставления одиночного символа и перехода к началу
				 *          повторения, адресом тела повторения, благодаря чему исполнение
				 *          проходит ряд подходящих символов одним ходом.
				 *
				 * \~english
				 * @brief Method of marking the repetitions of a single character
				 * @details The method marks the two-branch jumps whose repetition branch
				 *          consists of matching a single character and a jump to the beginning of
				 *          the repetition, with the address of the body of the repetition, thanks to which execution
				 *          walks a run of matching characters in one move.
				 *
				 * \~
				 */
				void mark() noexcept;
				/**
				 * \~russian
				 * @brief Метод подсчёта повторений любого символа и проверки их вложенности
				 *
				 * @param id     индекс проверяемого узла в арене узлов
				 * @param inside флаг нахождения узла в пределах повторения
				 * @param nested флаг обнаружения повторения в пределах повторения
				 * @return       количество неограниченных повторений любого символа
				 *
				 * \~english
				 * @brief Method of counting the repetitions of any character and checking their nesting
				 * @param id     index of the checked node in the node arena
				 * @param inside flag of the node being within a repetition
				 * @param nested flag of a repetition being found within a repetition
				 * @return       number of unbounded repetitions of any character
				 *
				 * \~
				 */
				size_t sweeps(const node_id_t id, const bool inside, bool & nested) const noexcept;
				/**
				 * \~russian
				 * @brief Метод распознавания выражения, проходящего текст единственной попыткой
				 *
				 * @details Признак устанавливается выражению, начинающемуся неограниченным
				 *          жадным повторением любого символа, при отсутствии вложенных
				 *          повторений и единственности повторения любого символа.
				 *
				 * \~english
				 * @brief Method of recognising an expression walking the text in a single attempt
				 * @details The indication is set for an expression beginning with an unbounded
				 *          greedy repetition of any character, in the absence of nested
				 *          repetitions and when the repetition of any character is a single one.
				 *
				 * \~
				 */
				void sweeping() noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки начала сопоставления привязкой к позиции начала поиска
				 *
				 * @details Метод подтверждает привязку лишь тогда, когда сопоставление
				 *          начинается ею на всех путях выражения. Привязки нулевой длины,
				 *          позиции совпадения не ограничивающие, обходятся насквозь,
				 *          а сброс начала совпадения обход прекращает.
				 *
				 * @param id    индекс проверяемого узла в арене узлов
				 * @param chain флаг обхода цепочки узлов одного уровня вложенности
				 * @return      результат проверки начала сопоставления привязкой
				 *
				 * \~english
				 * @brief Method of checking that matching begins with an anchor to the position where the search starts
				 * @details The method confirms the anchor only when matching
				 *          begins with it on all paths of the expression. Zero-length anchors
				 *          that do not limit the positions of a match are walked through,
				 *          and a reset of the beginning of a match stops the walk.
				 * @param id    index of the checked node in the node arena
				 * @param chain flag of walking a chain of nodes of the same nesting level
				 * @return      result of checking that matching begins with an anchor
				 *
				 * \~
				 */
				bool anchoring(const node_id_t id, const bool chain) const noexcept;
				/**
				 * \~russian
				 * @brief Метод распознавания выражения, привязанного к позиции начала поиска
				 *
				 * @details Признак устанавливается выражению, начинающемуся привязкой
				 *          к началу текста на всех путях, а также выражению, сопоставляемому
				 *          в режиме «ANCHORED».
				 *
				 * \~english
				 * @brief Method of recognising an expression anchored to the position where the search starts
				 * @details The indication is set for an expression beginning with an anchor
				 *          to the beginning of the text on all paths, as well as for an expression matched
				 *          in the «ANCHORED» mode.
				 *
				 * \~
				 */
				void anchored() noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки обязательного продвижения узла по тексту
				 *
				 * @details Метод подтверждает продвижение по строению узла: цепочка
				 *          продвигается, если продвигается хоть один узел её, выбор
				 *          ветвей - если продвигается всякая ветвь его, группа - по
				 *          содержимому своему, а повторение - если обязательно хотя бы
				 *          однажды. Об узлах, пустое сопоставление каких определяется
				 *          составом конструкций вне регулярного подмножества, метод
				 *          умалчивает: вывод там ненадёжен.
				 *
				 *          Умолчание безопасно, а ошибочное подтверждение - нет: оно
				 *          сняло бы сторожа продвижения, и повторение на пустом теле
				 *          не прекращалось бы вовсе.
				 *
				 * @param id индекс проверяемого узла в арене узлов
				 * @return   результат проверки обязательного продвижения узла по тексту
				 *
				 * \~english
				 * @brief Method of checking the mandatory advance of a node through the text
				 * @details The method confirms the advance by the structure of the node:
				 *          a concatenation advances if at least one of its nodes advances,
				 *          an alternation - if every one of its branches advances, a group -
				 *          by its own content, and a repetition - if it is mandatory at least
				 *          once. The method says nothing about the nodes whose empty match
				 *          is determined by constructs outside the regular subset: the
				 *          inference there is unreliable.
				 *
				 *          Saying nothing is safe, whereas confirming wrongly is not: it
				 *          would remove the guard of the advance, and the repetition would
				 *          not terminate on an empty body at all.
				 * @param id index of the checked node in the node arena
				 * @return   result of checking the mandatory advance of the node through the text
				 *
				 * \~
				 */
				bool advancing(const node_id_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки обязательного продвижения узла по тексту со следом обхода
				 *
				 * @details След несёт узлы, проверка каких ещё не завершена, и служит
				 *          сторожем кругового обхода: рекурсивный вызов ссылается
				 *          на выражение, вызов этот содержащее, отчего проверка его
				 *          пришла бы к себе же. Узел, в следе уже стоящий, продвижения
				 *          не подтверждает - умолчание безопасно, а подтверждение
				 *          ошибочное сняло бы сторожа продвижения.
				 *
				 * @param id      индекс проверяемого узла в арене узлов
				 * @param visited след узлов, проверка каких не завершена
				 * @return        результат проверки обязательного продвижения узла по тексту
				 *
				 * \~english
				 * @brief Method of checking the mandatory advance of a node through the text with a trail of the walk
				 * @details The trail holds the nodes whose check is not yet finished and serves
				 *          as a guard of a circular walk: a recursive call refers to the expression
				 *          containing that very call, which is why checking it would come to itself.
				 *          A node already standing in the trail confirms no advance - saying nothing
				 *          is safe, whereas confirming wrongly would remove the guard of the advance.
				 * @param id      index of the checked node in the node arena
				 * @param visited trail of the nodes whose check is not finished
				 * @return        result of checking the mandatory advance of the node through the text
				 *
				 * \~
				 */
				bool advancing(const node_id_t id, vector <node_id_t> & visited) const noexcept;
				/**
				 * \~russian
				 * @brief Метод распознавания выражения, сопоставляемого литералом
				 *
				 * @details Выражение, состоящее из одной последовательности символов,
				 *          сопоставляется поиском этой последовательности в тексте,
				 *          минуя исполнение программы. Метод распознаёт такое выражение
				 *          по набору инструкций и сохраняет искомую последовательность
				 *          в программе.
				 *
				 * \~english
				 * @brief Method of recognising an expression matched by a literal
				 * @details An expression consisting of a single character sequence
				 *          is matched by searching for that sequence in the text,
				 *          bypassing the execution of the program. The method recognises such an expression
				 *          by the set of instructions and keeps the sought sequence
				 *          in the program.
				 *
				 * \~
				 */
				void condense() noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения ведущего литерала совпадения
				 *
				 * @details Ведущий литерал - последовательность символов, с которой
				 *          начинается любое совпадение выражения. Его наличие позволяет
				 *          отыскивать позиции возможного начала совпадения поиском
				 *          последовательности, а не перебором текста побайтно.
				 *          Привязки к позиции длины не имеют и пропускаются.
				 *
				 * @param id индекс первого узла цепочки в арене узлов
				 * @return   ведущий литерал совпадения выражения
				 *
				 * \~english
				 * @brief Method of getting the leading literal of a match
				 * @details The leading literal is the character sequence every match
				 *          of the expression begins with. Its presence allows
				 *          locating the positions of a possible beginning of a match by searching for a
				 *          sequence rather than by walking the text byte by byte.
				 *          Anchors to a position have no length and are skipped.
				 * @param id index of the first node of the chain in the node arena
				 * @return   leading literal of a match of the expression
				 *
				 * \~
				 */
				string leading(const node_id_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод дополнения набора допустимых начальных байтов
				 *
				 * @details Метод обходит инструкции программы, достижимые без сопоставления
				 *          символов, и дополняет набор байтами, допустимыми в начале
				 *          совпадения. Проверки привязок к позиции в тексте считаются
				 *          выполнимыми, благодаря чему набор остаётся надмножеством.
				 *
				 * @param address адрес инструкции, с которой начинается обход
				 * @return        результат применимости набора допустимых байтов
				 *
				 * \~english
				 * @brief Method of extending the set of admissible starting bytes
				 * @details The method walks the instructions of the program reachable without matching
				 *          characters and extends the set with the bytes admissible at the beginning of
				 *          a match. The checks of the anchors to a position in the text are considered
				 *          satisfiable, thanks to which the set remains a superset.
				 * @param address address of the instruction the walk starts from
				 * @return        result of the applicability of the set of admissible bytes
				 *
				 * \~
				 */
				bool reachable(const address_t address) noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения обязательного литерала цепочки узлов
				 *
				 * @param id индекс первого узла цепочки в арене узлов
				 * @return   обязательный литерал совпадения цепочки узлов
				 *
				 * \~english
				 * @brief Method of getting the mandatory literal of a chain of nodes
				 * @param id index of the first node of the chain in the node arena
				 * @return   mandatory literal of a match of the chain of nodes
				 *
				 * \~
				 */
				string required(const node_id_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения обязательного литерала цепочки узлов с удалением его
				 *
				 * @details Удаление есть наибольшее число байтов, какое способно
				 *          предшествовать литералу внутри совпадения. Оно позволяет
				 *          употребить литерал позиционно: совпадение обязано его
				 *          содержать, отчего начало совпадения не может лежать
				 *          дальше чем за удаление до ближайшего вхождения литерала.
				 *          Значение «string_view::npos» означает удаление
				 *          неограниченное, при каком позиционное употребление
				 *          литерала неприменимо.
				 *
				 * @param id       индекс первого узла цепочки в арене узлов
				 * @param distance наибольшее удаление литерала от начала совпадения
				 * @return         обязательный литерал совпадения цепочки узлов
				 *
				 * \~english
				 * @brief Method of getting the mandatory literal of a chain of nodes with its distance
				 * @param id       index of the first node of the chain in the node arena
				 * @param distance the greatest distance of the literal from the beginning of a match
				 * @return         mandatory literal of a match of the chain of nodes
				 *
				 * \~
				 */
				string required(const node_id_t id, size_t & distance) const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения наибольшей длины сопоставления узла
				 *
				 * @details Длина выводится в байтах текста. Значение
				 *          «string_view::npos» означает длину неограниченную:
				 *          её дают повторение без верхнего предела, обратная
				 *          ссылка, рекурсивный вызов и условное выражение.
				 *
				 * @param id индекс узла в арене узлов
				 * @return   наибольшая длина сопоставления узла в байтах
				 *
				 * \~english
				 * @brief Method of getting the greatest matching length of a node
				 * @param id index of the node in the node arena
				 * @return   the greatest matching length of the node in bytes
				 *
				 * \~
				 */
				size_t spanningNode(const node_id_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения наибольшей длины сопоставления цепочки узлов
				 *
				 * @param id индекс первого узла цепочки в арене узлов
				 * @return   наибольшая длина сопоставления цепочки узлов в байтах
				 *
				 * \~english
				 * @brief Method of getting the greatest matching length of a chain of nodes
				 * @param id index of the first node of the chain in the node arena
				 * @return   the greatest matching length of the chain of nodes in bytes
				 *
				 * \~
				 */
				size_t spanning(const node_id_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения обязательного литерала узла
				 *
				 * @param id индекс узла в арене узлов
				 * @return   обязательный литерал совпадения узла
				 *
				 * \~english
				 * @brief Method of getting the mandatory literal of a node
				 * @param id index of the node in the node arena
				 * @return   mandatory literal of a match of the node
				 *
				 * \~
				 */
				string requiredNode(const node_id_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения обязательного литерала узла с удалением его
				 *
				 * @param id       индекс узла в арене узлов
				 * @param distance наибольшее удаление литерала от начала сопоставления узла
				 * @return         обязательный литерал совпадения узла
				 *
				 * \~english
				 * @brief Method of getting the mandatory literal of a node with its distance
				 * @param id       index of the node in the node arena
				 * @param distance the greatest distance of the literal from the beginning of the node match
				 * @return         mandatory literal of a match of the node
				 *
				 * \~
				 */
				string requiredNode(const node_id_t id, size_t & distance) const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения литерала, сопоставляемого узлом целиком
				 *
				 * @details Литерал извлекается для узлов одиночного символа и
				 *          последовательности символов, не сопоставляемых без учёта
				 *          регистра и представимых символами набора ASCII.
				 *
				 * @param id индекс узла в арене узлов
				 * @return   литерал, сопоставляемый узлом целиком
				 *
				 * \~english
				 * @brief Method of getting the literal matched by a node as a whole
				 * @details The literal is obtained for the nodes of a single character and of a
				 *          character sequence that are not matched case-insensitively
				 *          and are representable by the characters of the ASCII set.
				 * @param id index of the node in the node arena
				 * @return   literal matched by the node as a whole
				 *
				 * \~
				 */
				string literal(const node_id_t id) const noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод размещения инструкции программы
				 *
				 * @param type  код операции размещаемой инструкции
				 * @param flags набор режимов компиляции инструкции
				 * @return      адрес размещённой инструкции программы
				 *
				 * \~english
				 * @brief Method of placing a program instruction
				 * @param type  operation code of the placed instruction
				 * @param flags set of compilation modes of the instruction
				 * @return      address of the placed program instruction
				 *
				 * \~
				 */
				address_t emit(const opcode_t type, const uint32_t flags) noexcept;
				/**
				 * \~russian
				 * @brief Метод размещения класса символов в программе
				 *
				 * @param value класс символов для размещения в программе
				 * @return      индекс класса символов в хранилище классов
				 *
				 * \~english
				 * @brief Method of placing a character class in the program
				 * @param value character class to place in the program
				 * @return      index of the character class in the class storage
				 *
				 * \~
				 */
				uint32_t store(const class_t & value) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод извлечения адреса следующей размещаемой инструкции
				 *
				 * @return адрес следующей размещаемой инструкции программы
				 *
				 * \~english
				 * @brief Method of getting the address of the next instruction to place
				 * @return address of the next program instruction to place
				 *
				 * \~
				 */
				address_t position() const noexcept;
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
				Compiler() noexcept;
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
				~Compiler() noexcept {}
		} compiler_t;
	};
};

#endif // __AWH_REGEX_COMPILER__
