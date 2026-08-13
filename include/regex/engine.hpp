/**
 * @file: engine.hpp
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
 * @brief Заголовочный файл движка регулярных выражений — класс Engine, объединяющий разбор,
 *        компиляцию прямой и развёрнутой программ, детерминированное исполнение и исполнение
 *        без возврата, и выбирающий способ сопоставления по свойствам выражения и текста
 *
 * @section engine_decisions Намеренные решения
 *
 * @details Перечисленное ниже выглядит несообразностью, но выбрано осознанно и
 *          правке не подлежит. Раздел заведён затем, чтобы разбор кода не начинался
 *          каждый раз с одних и тех же выводов.
 *
 *          <b>Проход в обратном направлении начинается с конца текста, а не с конца
 *          найденного совпадения.</b> Проход от конца совпадения обошёлся бы дешевле
 *          на порядки, но дал бы неверный ответ: проход в прямом направлении находит
 *          самый ранний конец совпадения, тогда как совпадение с самым ранним началом
 *          вправе кончаться правее и в отсчёт от найденного конца не попадёт.
 *          Измерением подтверждено расхождение с эталонной реализацией на выражениях
 *          вида «.b+.*a|b» - привязка развёрнутой программы к концу совпадения
 *          пробовалась и откачена.
 *
 *          <b>Выбор между проходом в обратном направлении и исполнением без возврата
 *          определяется наличием ведущего литерала</b>, а не одной лишь длиной
 *          пройденного участка текста. Проход в обратном направлении проходит текст
 *          целиком и стоит одинаково при любом выражении, тогда как исполнение
 *          без возврата стоит соразмерно количеству заводимых им состояний. Ведущий
 *          литерал переводит его сразу к позициям возможного начала совпадения,
 *          отчего оно обходится дешевле прохода по тексту целиком - измерением
 *          получено восьмикратное превосходство на выражении «\bneedle\b»
 *          и двенадцатикратное отставание на выражении «\w+@\w+\.\w+»,
 *          ведущего литерала не имеющем.
 *
 *          <b>Границы совпадения на коротком участке текста устанавливаются
 *          исполнением с возвратом, а не исполнением без возврата.</b> Выбор
 *          выглядит противоречащим устройству модуля, отводящему исполнению
 *          с возвратом лишь нерегулярное подмножество синтаксиса, но исполнение
 *          с возвратом ведёт единственный набор границ, изменяя его на месте,
 *          тогда как исполнение без возврата ведёт набор при каждом состоянии
 *          и замещает его копией при каждом захвате. Измерением на тексте в сто
 *          байт получено семьдесят две копии набора за одно сопоставление и
 *          превосходство исполнения с возвратом до пяти раз. Показательный рост
 *          времени исполнения с возвратом ограничен объёмом работы, соразмерным
 *          участку текста и размеру программы, а исчерпание объёма возвращает
 *          сопоставление исполнению без возврата, время которого от вида выражения
 *          не зависит. Возврат закреплён тестом «Regex.EngineBudget» на выражении,
 *          где эталонная реализация, располагающая одним лишь исполнением
 *          с возвратом, отказывает с ошибкой исчерпания предела совпадений.
 *
 *          <b>На длинном участке текста исполнение с возвратом применяется лишь
 *          вслед за проходом в обратном направлении, установившим позицию начала
 *          совпадения.</b> Непривязанное исполнение с возвратом повторяет попытку
 *          сопоставления в каждой позиции текста и на длинном тексте становится
 *          непригодным: измерением получено четыреста пятьдесят семь миллисекунд
 *          на выражении «\w+@\w+\.\w+» по тексту в двести пятьдесят шесть килобайт
 *          против девятисот микросекунд у прохода по тексту с исполнением без
 *          возврата. Ограничение объёма работы от этого не спасает: отказ по
 *          исчерпании объёма измерен в двести пятьдесят миллисекунд, поскольку
 *          объём считается шагами, а время уходит в размещение точек возврата
 *          и журнала изменений. Известная позиция начала совпадения повторение
 *          попытки устраняет, и тогда исполнение с возвратом даёт превосходство
 *          вдвое на выражении «.*needle» при равенстве на прочих.
 *
 *          <b>Выражение с ведущим литералом сопоставляется исполнением с возвратом
 *          прежде проверки наличия совпадения детерминированным исполнением.</b>
 *          Порядок выглядит отказом от быстрой проверки наличия совпадения, но
 *          ведущий литерал переводит исполнение с возвратом сразу к позициям
 *          вхождения литерала тем же поиском последовательности, каким проверка
 *          наличия пользуется сама, отчего проход её оказывается вторым поиском
 *          того же литерала. Отсутствие совпадения исполнение с возвратом при этом
 *          устанавливает окончательно, поскольку проходит текст целиком, и лишь
 *          исчерпание допустимого объёма работы возвращает сопоставление прежнему
 *          порядку. Измерением получено превосходство почти вдвое на выражениях
 *          «\\bneedle\\b» и «^GET /\\S+ HTTP/1\\.1». Порядок закреплён тестом
 *          «Regex.EngineLeading».
 *
 *          <b>Выражение, начинающееся неограниченным жадным повторением любого
 *          символа, сопоставляется исполнением с возвратом без проходов
 *          детерминированного исполнения.</b> Такое повторение поглощает текст
 *          до совпадения, начинающегося правее, отчего попытка сопоставления
 *          в каждой позиции текста не повторяется и одна попытка устанавливает
 *          границы целиком. Прежний порядок проходил текст трижды - проверкой
 *          наличия совпадения, поиском его начала и сопоставлением, - и измерением
 *          получено превосходство втрое на выражении «.*needle».
 *
 *          Признак намеренно узок и расширению не подлежит: он требует
 *          единственности повторения любого символа и отсутствия повторений,
 *          вложенных друг в друга. Оба условия проверены измерением: выражение
 *          «.*needle.*tail» при двух повторениях любого символа исполняется
 *          с возвратом в сорок раз дольше, а «.*(a+)+b» при вложенных повторениях
 *          исчерпывает допустимый объём работы. Узость закреплена тестом
 *          «Regex.EngineSweeping».
 *
 * \~english
 * @brief Header file of the regular expression engine — the Engine class, which unites parsing,
 *        compilation of the forward and the reversed programs, deterministic execution and execution
 *        without backtracking, and chooses the way of matching by the properties of the expression and of the text
 * @section engine_decisions Deliberate decisions
 * @details What is listed below looks like an incongruity, but was chosen deliberately and
 *          is not subject to correction. The section is introduced so that reading the code does not start
 *          every time from the same conclusions.
 *          <b>The backward pass starts from the end of the text rather than from the end of
 *          the found match.</b> A pass from the end of the match would cost orders of magnitude
 *          less, but would give a wrong answer: the forward pass finds
 *          the earliest end of a match, whereas the match with the earliest beginning
 *          is entitled to end further to the right and would not fall into the count from the found end.
 *          The divergence from the reference implementation on expressions
 *          of the «.b+.*a|b» form is confirmed by measurement — anchoring the reversed program to the end of the match
 *          was tried and rolled back.
 *          <b>The choice between the backward pass and execution without backtracking
 *          is determined by the presence of a leading literal</b> rather than by the length
 *          of the walked stretch of the text alone. The backward pass walks the text
 *          as a whole and costs the same for any expression, whereas execution
 *          without backtracking costs proportionally to the number of states it creates. A leading
 *          literal moves it straight to the positions of a possible beginning of a match,
 *          which is why it costs less than a pass over the text as a whole — measurement
 *          yielded an eightfold advantage on the «\bneedle\b» expression
 *          and a twelvefold lag on the «\w+@\w+\.\w+» expression,
 *          which has no leading literal.
 *          <b>The boundaries of a match on a short stretch of the text are established
 *          by execution with backtracking rather than by execution without backtracking.</b> The choice
 *          looks as if it contradicts the arrangement of the module, which allots to execution
 *          with backtracking only the non-regular subset of the syntax, but execution
 *          with backtracking keeps a single set of boundaries, changing it in place,
 *          whereas execution without backtracking keeps a set at every state
 *          and replaces it with a copy at every capture. Measurement on a text of a hundred
 *          bytes yielded seventy-two copies of the set per one match and
 *          an advantage of execution with backtracking of up to five times. The exponential growth
 *          of the time of execution with backtracking is bounded by an amount of work proportional to
 *          the stretch of the text and to the size of the program, and exhausting the amount returns
 *          the matching to execution without backtracking, whose time does not depend on the kind of the
 *          expression. The return is fixed by the «Regex.EngineBudget» test on an expression
 *          where the reference implementation, having execution with backtracking
 *          alone at its disposal, fails with the error of exhausting the match limit.
 *          <b>On a long stretch of the text execution with backtracking is applied only
 *          after the backward pass that has established the position where the match
 *          begins.</b> Unanchored execution with backtracking repeats the match
 *          attempt at every position of the text and becomes unfit on a long text:
 *          measurement yielded four hundred and fifty-seven milliseconds
 *          on the «\w+@\w+\.\w+» expression over a text of two hundred and fifty-six kilobytes
 *          against nine hundred microseconds for a pass over the text with execution without
 *          backtracking. Bounding the amount of work is no rescue from that: the failure on
 *          exhausting the amount was measured at two hundred and fifty milliseconds, since
 *          the amount is counted in steps while the time goes into allocating the backtracking points
 *          and the change log. A known position where the match begins removes the repetition
 *          of the attempt, and then execution with backtracking gives a twofold advantage
 *          on the «.*needle» expression while being equal on the others.
 *          <b>An expression with a leading literal is matched by execution with backtracking
 *          before checking the presence of a match by deterministic execution.</b>
 *          The order looks like giving up the fast check of the presence of a match, but
 *          a leading literal moves execution with backtracking straight to the positions
 *          where the literal occurs by the same sequence search that the presence
 *          check uses itself, which is why its pass turns out to be a second search
 *          for the same literal. The absence of a match is then established by execution with backtracking
 *          conclusively, since it walks the text as a whole, and only
 *          exhausting the admissible amount of work returns the matching to the former
 *          order. Measurement yielded an advantage of almost twofold on the expressions
 *          «\\bneedle\\b» and «^GET /\\S+ HTTP/1\\.1». The order is fixed by the
 *          «Regex.EngineLeading» test.
 *          <b>An expression beginning with an unbounded greedy repetition of any
 *          character is matched by execution with backtracking without the passes
 *          of deterministic execution.</b> Such a repetition absorbs the text
 *          up to a match beginning further to the right, which is why the match attempt
 *          is not repeated at every position of the text and one attempt establishes
 *          the boundaries as a whole. The former order walked the text three times — by the check
 *          of the presence of a match, by the search for its beginning and by the matching — and measurement
 *          yielded a threefold advantage on the «.*needle» expression.
 *          The indication is deliberately narrow and is not subject to extension: it requires
 *          the repetition of any character to be a single one and the absence of repetitions
 *          nested in one another. Both conditions were checked by measurement: the expression
 *          «.*needle.*tail» with two repetitions of any character is executed
 *          with backtracking forty times longer, and «.*(a+)+b» with nested repetitions
 *          exhausts the admissible amount of work. The narrowness is fixed by the
 *          «Regex.EngineSweeping» test.
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_REGEX_ENGINE__
#define __AWH_REGEX_ENGINE__

/**
 * Стандартные заголовочные файлы
 */
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <utility>
#include <string_view>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "dfa.hpp"
#include "pike.hpp"
#include "backtrack.hpp"
#include "parser.hpp"
#include "compiler.hpp"
#include "codegen.hpp"

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
		 * @brief Наименьшая длина участка текста, оправдывающая поиск начала совпадения
		 *
		 * @details Поиск позиции начала совпадения выполняется проходом по тексту
		 *          в обратном направлении и оправдан лишь тогда, когда исполнение
		 *          без возврата обошлось бы дороже.
		 *
		 * \~english
		 * @brief Smallest length of a stretch of the text justifying the search for the beginning of a match
		 * @details The search for the position where a match begins is performed by a backward pass
		 *          over the text and is justified only when execution
		 *          without backtracking would cost more.
		 *
		 * \~
		 */
		constexpr size_t MIN_REVERSE = 512;

		/**
		 * \~russian
		 * @brief Отношение стоимости исполнения без возврата к детерминированному
		 *
		 * @details Отношение получено измерением и определяет, во сколько раз более
		 *          длинный участок текста допустимо пройти детерминированным
		 *          исполнением взамен исполнения без возврата.
		 *
		 * \~english
		 * @brief Ratio of the cost of execution without backtracking to the deterministic one
		 * @details The ratio was obtained by measurement and determines how many times longer
		 *          a stretch of the text may be walked by deterministic
		 *          execution instead of execution without backtracking.
		 *
		 * \~
		 */
		constexpr size_t REVERSE_RATIO = 40;

		/**
		 * \~russian
		 * @brief Наибольшая длина участка текста, оправдывающая исполнение с возвратом
		 *
		 * @details Исполнение с возвратом устанавливает границы совпадения дешевле
		 *          исполнения без возврата, но время его растёт с длиной текста
		 *          показательно, поэтому длина участка ограничивается.
		 *
		 * \~english
		 * @brief Largest length of a stretch of the text justifying execution with backtracking
		 * @details Execution with backtracking establishes the boundaries of a match cheaper than
		 *          execution without backtracking, but its time grows with the length of the text
		 *          exponentially, therefore the length of the stretch is bounded.
		 *
		 * \~
		 */
		constexpr size_t MAX_BACKTRACK = 0x1000;

		/**
		 * \~russian
		 * @brief Отношение допустимого объёма работы исполнения с возвратом к его оценке
		 *
		 * @details Объём работы исполнения с возвратом, не прибегающего к перебору,
		 *          соразмерен произведению длины участка текста на размер программы.
		 *          Отношение задаёт запас, при исчерпании которого сопоставление
		 *          возвращается исполнению без возврата.
		 *
		 * \~english
		 * @brief Ratio of the admissible amount of work of execution with backtracking to its estimate
		 * @details The amount of work of execution with backtracking that does not resort to enumeration
		 *          is proportional to the product of the length of the stretch of the text by the size of the program.
		 *          The ratio sets the margin, on exhausting which the matching
		 *          returns to execution without backtracking.
		 *
		 * \~
		 */
		constexpr size_t BACKTRACK_RATIO = 4;

		/**
		 * \~russian
		 * @brief Скомпилированное регулярное выражение
		 *
		 * @details Выражение собирается движком единожды и после сборки не изменяется,
		 *          благодаря чему разделяется несколькими потоками исполнения без
		 *          согласования доступа. Рабочее состояние сопоставления выражению
		 *          не принадлежит и хранится движком, исполняющим выражение.
		 *
		 * \~english
		 * @brief Compiled regular expression
		 * @details The expression is built by the engine once and is not changed after building,
		 *          thanks to which it is shared by several threads of execution without
		 *          coordinating the access. The working state of the matching does not belong to the expression
		 *          and is kept by the engine executing the expression.
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Expression {
			// Программа сопоставления в прямом направлении
			program_t forward;
			// Программа сопоставления в обратном направлении
			program_t backward;
			/**
			 * \~russian
			 * Соответствие имён именованных групп наборам их номеров
			 *
			 * @details В режиме «DUPNAMES» одно имя объявляется несколькими группами,
			 *          поэтому имени отвечает набор их номеров в порядке объявления.
			 *
			 * \~english
			 * Mapping of the names of the named groups to the sets of their numbers
			 * @details In the «DUPNAMES» mode one name is declared by several groups,
			 *          therefore a name corresponds to a set of their numbers in declaration order.
			 *
			 * \~
			 */
			unordered_map <string, vector <uint32_t>> names;
			/**
			 * \~russian
			 * Флаг исполнения выражения с возвратом
			 *
			 * @details Флаг установлен, если выражение содержит конструкции вне
			 *          регулярного подмножества синтаксиса, недостижимые исполнением
			 *          без возврата: ссылки на захваченные группы, рекурсивные вызовы,
			 *          условные выражения, проверки окружения, атомарные группы,
			 *          захватывающие кванторы и графемные кластеры.
			 *
			 * \~english
			 * Flag of executing the expression with backtracking
			 * @details The flag is set if the expression holds constructs outside
			 *          the regular subset of the syntax, unreachable by execution
			 *          without backtracking: references to captured groups, recursive calls,
			 *          conditional expressions, lookarounds, atomic groups,
			 *          possessive quantifiers and grapheme clusters.
			 *
			 * \~
			 */
			bool backtracking;
			// Флаг готовности выражения к сопоставлению
			bool ready;
			// Флаг применимости поиска позиции начала совпадения
			bool reversible;
			/**
			 * \~russian
			 * Сопоставитель выражения в виде порождённого машинного кода
			 *
			 * @details Сопоставитель порождается сборкой в режиме «JIT», если
			 *          выражение принадлежит подмножеству, кодогенерацию получающему.
			 *          Владение им разделяемое: выражение копируется потребителями
			 *          свободно, а порождённый код исполняется несколькими потоками
			 *          одновременно, поскольку рабочего состояния он не несёт.
			 *
			 * \~english
			 * Matcher of the expression in the form of generated machine code
			 * @details The matcher is generated by the build in the «JIT» mode if
			 *          the expression belongs to the subset that receives code generation.
			 *          Its ownership is shared: the expression is copied by the consumers
			 *          freely, and the generated code is executed by several threads
			 *          at once, since it carries no working state.
			 *
			 * \~
			 */
			shared_ptr <codegen_t> machine;
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
			Expression() noexcept : backtracking(false), ready(false), reversible(false) {}
		} expression_t;

		/**
		 * \~russian
		 * @brief Класс движка регулярных выражений
		 *
		 * @details Класс объединяет способы сопоставления и выбирает между ними.
		 *          Наличие совпадения определяется детерминированным исполнением.
		 *          Границы захваченных групп устанавливаются исполнением без возврата,
		 *          которому предшествует поиск позиции начала совпадения проходом
		 *          по тексту в обратном направлении, если исполнение без возврата
		 *          обошлось бы дороже такого прохода.
		 *
		 * \~english
		 * @brief Class of the regular expression engine
		 * @details The class unites the ways of matching and chooses between them.
		 *          The presence of a match is determined by deterministic execution.
		 *          The boundaries of the captured groups are established by execution without backtracking,
		 *          which is preceded by the search for the position where a match begins by a backward pass
		 *          over the text, if execution without backtracking
		 *          would cost more than such a pass.
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Engine {
			private:
				// Объект разбора регулярного выражения
				Parser _parser;
			private:
				// Объект компиляции регулярного выражения
				Compiler _compiler;
			private:
				// Собственное скомпилированное регулярное выражение движка
				expression_t _expression;
			private:
				// Объект детерминированного исполнения в прямом направлении
				Dfa _dfa;
			private:
				// Объект детерминированного исполнения в обратном направлении
				Dfa _reverse;
			private:
				// Объект исполнения без возврата
				Pike _pike;
			private:
				// Объект исполнения с возвратом
				Backtrack _backtrack;
			private:
				/**
				 * \~russian
				 * Набор границ, отбрасываемый проверкой наличия совпадения
				 *
				 * @details Проверка наличия совпадения границ захвата не выдаёт,
				 *          но исполнение их записывает, и набор под них требуется
				 *          всякому вызову. Набор заведён полем, а не переменной
				 *          вызова, затем, чтобы размещение его выполнялось однажды:
				 *          проверка вызывается на каждой позиции текста, и
				 *          размещение обходилось дороже самой проверки. Движок
				 *          принадлежит потоку исполнения, поэтому поле общим
				 *          для потоков не становится.
				 *
				 * \~english
				 * Set of boundaries discarded by the check of the presence of a match
				 * @details The check of the presence of a match yields no capture boundaries,
				 *          but execution writes them, and a set for them is required by
				 *          every call. The set is introduced as a field rather than as a variable of
				 *          the call so that its allocation is performed once:
				 *          the check is called at every position of the text, and
				 *          the allocation cost more than the check itself. The engine
				 *          belongs to the thread of execution, therefore the field does not become common
				 *          to the threads.
				 *
				 * \~
				 */
				vector <pair <size_t, size_t>> _spare;
			private:
				// Код ошибки последней операции движка
				error_t _error;
			public:
				/**
				 * \~russian
				 * @brief Метод сборки регулярного выражения
				 *
				 * @details Отказ сборки с кодом ошибки «UNSUPPORTED» означает принадлежность
				 *          выражения нерегулярному подмножеству синтаксиса и требует
				 *          исполнения выражения с возвратом.
				 *
				 * @param pattern текст регулярного выражения
				 * @param flags   набор режимов компиляции регулярного выражения
				 * @return        результат выполнения сборки
				 *
				 * \~english
				 * @brief Method of building a regular expression
				 * @details A build failure with the «UNSUPPORTED» error code means that the expression
				 *          belongs to the non-regular subset of the syntax and requires
				 *          execution of the expression with backtracking.
				 * @param pattern text of the regular expression
				 * @param flags   set of compilation modes of the regular expression
				 * @return        result of performing the build
				 *
				 * \~
				 */
				bool build(string_view pattern, const uint32_t flags = 0) noexcept;
				/**
				 * \~russian
				 * @brief Метод сборки регулярного выражения в отдельное выражение
				 *
				 * @details Собранное выражение движку не принадлежит и после сборки
				 *          не изменяется, благодаря чему исполняется несколькими
				 *          движками одновременно.
				 *
				 * @param pattern    текст регулярного выражения
				 * @param flags      набор режимов компиляции регулярного выражения
				 * @param expression собираемое регулярное выражение
				 * @return           результат выполнения сборки
				 *
				 * \~english
				 * @brief Method of building a regular expression into a separate expression
				 * @details The built expression does not belong to the engine and is not changed
				 *          after building, thanks to which it is executed by several
				 *          engines at once.
				 * @param pattern    text of the regular expression
				 * @param flags      set of compilation modes of the regular expression
				 * @param expression regular expression being built
				 * @return           result of performing the build
				 *
				 * \~
				 */
				bool build(string_view pattern, const uint32_t flags, expression_t & expression) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки наличия совпадения в тексте
				 *
				 * @param text  текст для сопоставления
				 * @param start позиция начала поиска совпадения
				 * @return      результат проверки наличия совпадения
				 *
				 * \~english
				 * @brief Method of checking the presence of a match in the text
				 * @param text  text to match
				 * @param start position to start the search for a match from
				 * @return      result of checking the presence of a match
				 *
				 * \~
				 */
				bool test(string_view text, const size_t start = 0) noexcept;
				/**
				 * \~russian
				 * @brief Метод сопоставления регулярного выражения с текстом
				 *
				 * @param text     текст для сопоставления
				 * @param start    позиция начала поиска совпадения
				 * @param captures набор границ совпадения и захваченных групп
				 * @return         результат поиска совпадения
				 *
				 * \~english
				 * @brief Method of matching a regular expression against the text
				 * @param text     text to match
				 * @param start    position to start the search for a match from
				 * @param captures set of the boundaries of the match and of the captured groups
				 * @return         result of searching for a match
				 *
				 * \~
				 */
				bool exec(string_view text, const size_t start, vector <pair <size_t, size_t>> & captures) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки наличия совпадения отдельного выражения
				 *
				 * @param expression исполняемое регулярное выражение
				 * @param text       текст для сопоставления
				 * @param start      позиция начала поиска совпадения
				 * @return           результат проверки наличия совпадения
				 *
				 * \~english
				 * @brief Method of checking the presence of a match of a separate expression
				 * @param expression regular expression being executed
				 * @param text       text to match
				 * @param start      position to start the search for a match from
				 * @return           result of checking the presence of a match
				 *
				 * \~
				 */
				bool test(const expression_t & expression, string_view text, const size_t start = 0) noexcept;
				/**
				 * \~russian
				 * @brief Метод сопоставления отдельного выражения с текстом
				 *
				 * @param expression исполняемое регулярное выражение
				 * @param text       текст для сопоставления
				 * @param start      позиция начала поиска совпадения
				 * @param captures   набор границ совпадения и захваченных групп
				 * @return           результат поиска совпадения
				 *
				 * \~english
				 * @brief Method of matching a separate expression against the text
				 * @param expression regular expression being executed
				 * @param text       text to match
				 * @param start      position to start the search for a match from
				 * @param captures   set of the boundaries of the match and of the captured groups
				 * @return           result of searching for a match
				 *
				 * \~
				 */
				bool exec(const expression_t & expression, string_view text, const size_t start, vector <pair <size_t, size_t>> & captures) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения кода ошибки последней операции
				 *
				 * @return код ошибки последней операции движка
				 *
				 * \~english
				 * @brief Method of getting the error code of the last operation
				 * @return error code of the last operation of the engine
				 *
				 * \~
				 */
				error_t error() const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения текста ошибки последней операции
				 *
				 * @return текст ошибки последней операции движка
				 *
				 * \~english
				 * @brief Method of getting the error text of the last operation
				 * @return error text of the last operation of the engine
				 *
				 * \~
				 */
				string message() const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения смещения ошибки разбора выражения
				 *
				 * @return смещение ошибки в тексте регулярного выражения
				 *
				 * \~english
				 * @brief Method of getting the offset of the parse error of the expression
				 * @return offset of the error in the text of the regular expression
				 *
				 * \~
				 */
				size_t offset() const noexcept;
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
				Engine() noexcept;
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
				~Engine() noexcept {}
		} engine_t;
	};
};

#endif // __AWH_REGEX_ENGINE__
