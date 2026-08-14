/**
 * @file dfa.hpp
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
 * @brief Заголовочный файл детерминированного исполнения регулярных выражений — класс Dfa,
 *        строящий состояния детерминированного автомата по мере необходимости и определяющий
 *        наличие совпадения за один проход по тексту без сопровождения набора состояний
 *
 * @section dfa_decisions Намеренные решения
 *
 * @details Перечисленное ниже выглядит несообразностью, но выбрано осознанно и
 *          правке не подлежит. Раздел заведён затем, чтобы разбор кода не начинался
 *          каждый раз с одних и тех же выводов.
 *
 *          <b>Классы эквивалентности байтов дробятся принадлежностью символам слова
 *          и соответствием переводу строки помимо инструкций программы.</b> Дробление
 *          выглядит избыточным, поскольку ни одна инструкция таких байтов не
 *          сопоставляет, однако привязки к позиции в тексте различают их наравне
 *          с инструкциями: переход состояния кэшируется по классу, тогда как признаки
 *          положения достигаемого состояния вычисляются по значению байта, и байты
 *          одного класса получили бы признаки первого встреченного из них. Отказ
 *          от дробления пробовался и даёт расхождения с эталонной реализацией
 *          на выражениях «.+\\b\\D.?» и «^\\b» — оба закреплены тестом
 *          «Regex.AutomatonAlphabet».
 *
 *          <b>Проход участка текста, не затрагивающего краёв, выделен отдельным
 *          ходом.</b> Разделение выглядит удвоением кода, но сопоставление последнего
 *          байта текста и достижение края текста делают переход зависящим
 *          от положения, отчего кэшированию он не подлежит, и общий ход проверяет
 *          это на каждом байте. Выделенный ход извлекает переход из таблицы
 *          единственным обращением и измерением даёт превосходство до двух с
 *          половиной раз.
 *
 *          <b>Проход по тексту прекращается на отсутствии начатых сопоставлений
 *          лишь при привязке выражения к позиции начала поиска.</b> Отсутствие
 *          начатых сопоставлений вообще прекращением прохода не является:
 *          выражение без привязки начинает совпадение в любой позиции текста,
 *          и состояние с единственным адресом начала выражения означает лишь
 *          неудачу попыток, начатых левее. Привязка же означает невозможность
 *          совпадения правее позиции начала поиска, отчего такое состояние
 *          означает исчерпание всех путей. Признак привязки выводится
 *          компилятором по синтаксическому дереву и требует привязки
 *          от всех ветвей выражения, а в режиме «MULTILINE» привязкой
 *          не считает «^»: она выполнима после каждого перевода строки.
 *          Замером на выражении «^[A-Za-z0-9-]+: .+$» получено превосходство
 *          более чем в двадцать раз, признак закреплён тестом
 *          «Regex.EngineAnchored».
 *
 * \~english
 * @brief Header file of the deterministic execution of regular expressions — the Dfa class,
 *        which builds the states of a deterministic automaton as they become necessary and determines
 *        the presence of a match in one pass over the text without keeping a set of states
 * @section dfa_decisions Deliberate decisions
 * @details What is listed below looks like an incongruity, but was chosen deliberately and
 *          is not subject to correction. The section is introduced so that reading the code does not start
 *          every time from the same conclusions.
 *          <b>The equivalence classes of bytes are split by belonging to the word characters
 *          and by matching a line feed on top of the instructions of the program.</b> The splitting
 *          looks redundant, since no instruction matches such bytes,
 *          however the anchors to a position in the text tell them apart on a par
 *          with the instructions: a state transition is cached by class, whereas the indications
 *          of the position of the reached state are computed from the byte value, and the bytes
 *          of one class would receive the indications of the first of them encountered. Giving up
 *          the splitting was tried and yields divergences from the reference implementation
 *          on the expressions «.+\\b\\D.?» and «^\\b» — both are fixed by the
 *          «Regex.AutomatonAlphabet» test.
 *          <b>The pass over a stretch of the text that does not touch the edges is set apart as a separate
 *          move.</b> The separation looks like a duplication of code, but matching the last
 *          byte of the text and reaching the edge of the text make the transition depend
 *          on the position, which is why it is not subject to caching, and the common move checks
 *          that at every byte. The separate move gets the transition from the table
 *          by a single reference and by measurement gives an advantage of up to two and
 *          a half times.
 *          <b>The pass over the text stops on the absence of started matches
 *          only when the expression is anchored to the position where the search starts.</b> The absence
 *          of started matches in general is not a reason to stop the pass:
 *          an expression without an anchor begins a match at any position of the text,
 *          and a state with the single address of the beginning of the expression means only
 *          the failure of the attempts started further to the left. An anchor, on the other hand, means the impossibility of a
 *          match further to the right than the position where the search starts, which is why such a state
 *          means the exhaustion of all paths. The indication of the anchor is inferred
 *          by the compiler from the syntax tree and requires an anchor
 *          from all branches of the expression, and in the «MULTILINE» mode it does not consider «^»
 *          an anchor: it is satisfiable after every line feed.
 *          Measurement on the expression «^[A-Za-z0-9-]+: .+$» yielded an advantage
 *          of more than twentyfold, the indication is fixed by the
 *          «Regex.EngineAnchored» test.
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_REGEX_DFA__
#define __AWH_REGEX_DFA__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstdint>
#include <string_view>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы модуля
 */
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
		 * @brief Наибольшее количество состояний детерминированного автомата
		 *
		 * @details Состояния строятся по мере необходимости и сохраняются в кэше.
		 *          Достижение предела приводит к сбросу кэша, после которого построение
		 *          продолжается с текущего состояния.
		 *
		 * \~english
		 * @brief Largest number of states of the deterministic automaton
		 * @details The states are built as they become necessary and are kept in a cache.
		 *          Reaching the limit leads to a reset of the cache, after which the building
		 *          continues from the current state.
		 *
		 * \~
		 */
		constexpr size_t MAX_STATES = 0x2000;

		/**
		 * \~russian
		 * @brief Класс детерминированного исполнения регулярного выражения
		 *
		 * @details Класс определяет наличие совпадения за один проход по тексту,
		 *          обрабатывая каждый байт единственным переходом вместо сопровождения
		 *          набора состояний. Состояния автомата соответствуют наборам состояний
		 *          недетерминированного автомата и строятся по мере необходимости.
		 *          Захват групп при этом недоступен, поэтому границы совпадения
		 *          устанавливаются исполнением без возврата.
		 *
		 * \~english
		 * @brief Class of the deterministic execution of a regular expression
		 * @details The class determines the presence of a match in one pass over the text,
		 *          handling every byte by a single transition instead of keeping
		 *          a set of states. The states of the automaton correspond to sets of states
		 *          of the nondeterministic automaton and are built as they become necessary.
		 *          Capturing groups is unavailable there, therefore the boundaries of a match
		 *          are established by execution without backtracking.
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Dfa {
			private:
				/**
				 * \~russian
				 * @brief Признаки положения в тексте, действующие в состоянии
				 *
				 * \~english
				 * @brief Indications of the position in the text that hold in a state
				 *
				 * \~
				 */
				enum class context_t : uint32_t {
					NONE    = 0x00, // Признаки не установлены
					EDGE    = 0x01, // Положение соответствует краю текста, с которого начат проход
					ATTEMPT = 0x02, // Положение соответствует началу попытки сопоставления
					WORD    = 0x04, // Пройденный байт является символом слова
					NEWLINE = 0x08, // Пройденный байт является переводом строки
					LAST    = 0x10  // Пройденный байт является последним байтом текста
				};
			private:
				/**
				 * \~russian
				 * @brief Состояние детерминированного автомата
				 *
				 * \~english
				 * @brief State of the deterministic automaton
				 *
				 * \~
				 */
				typedef struct State {
					/**
					 * \~russian
					 * Флаг отсутствия начатых сопоставлений в состоянии
					 *
					 * @details Состояние содержит единственный адрес начала выражения,
					 *          то есть ни одно сопоставление не начато. Из такого состояния
					 *          допустим пропуск позиций предварительным отбором.
					 *
					 * \~english
					 * Flag of the absence of started matches in a state
					 * @details The state holds the single address of the beginning of the expression,
					 *          that is, no match has been started. From such a state
					 *          skipping positions by the preliminary selection is admissible.
					 *
					 * \~
					 */
					bool sparse;
					// Набор признаков положения в тексте
					uint32_t context;
					// Набор адресов инструкций, замыкание которых не выполнено
					vector <address_t> list;
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
					State() noexcept : sparse(false), context(0) {}
				} state_t;
			private:
				// Исполняемая программа регулярного выражения
				const program_t * _program;
			private:
				// Опознание исполняемой программы регулярного выражения
				uint64_t _identity;
			private:
				// Текст, по которому выполняется сопоставление
				string_view _text;
			private:
				// Позиция начала текущей попытки сопоставления
				size_t _start;
			private:
				// Флаг прохода по тексту в обратном направлении
				bool _backward;
			private:
				// Набор построенных состояний детерминированного автомата
				vector <state_t> _states;
			private:
				/**
				 * \~russian
				 * Набор признаков состояний, проверяемых при проходе по тексту
				 *
				 * @details Признаки вынесены из состояния отдельным набором, поскольку
				 *          проверяются на каждом байте текста, тогда как остальные поля
				 *          состояния требуются лишь при построении перехода.
				 *
				 * \~english
				 * Set of the indications of the states checked during the pass over the text
				 * @details The indications are taken out of the state as a separate array, since
				 *          they are checked at every byte of the text, whereas the other fields
				 *          of the state are required only when building a transition.
				 *
				 * \~
				 */
				vector <uint8_t> _marks;
			private:
				/**
				 * \~russian
				 * Непрерывная таблица переходов всех состояний автомата
				 *
				 * @details Переходы всех состояний размещены одним набором, в котором
				 *          состоянию отвечает участок длиной в количество классов
				 *          эквивалентности байтов. Размещение переходов каждого состояния
				 *          отдельным набором обходилось в два обращения к памяти на байт
				 *          текста: за адресом набора и за самим переходом.
				 *
				 * \~english
				 * Contiguous transition table of all the states of the automaton
				 * @details The transitions of all the states are placed as a single array in which
				 *          a state corresponds to a span as long as the number of equivalence
				 *          classes of bytes. Placing the transitions of every state
				 *          as a separate array cost two memory references per byte of
				 *          the text: for the address of the array and for the transition itself.
				 *
				 * \~
				 */
				vector <uint32_t> _table;
			private:
				// Длина участка таблицы переходов, отвечающего одному состоянию
				uint32_t _stride;
			private:
				// Количество классов эквивалентности байтов
				uint32_t _count;
			private:
				/**
				 * \~russian
				 * Соответствие значений байтов классам эквивалентности
				 *
				 * @details Байты, неразличимые ни одной инструкцией программы и ни одной
				 *          привязкой к позиции в тексте, объединены в один класс, отчего
				 *          участок таблицы переходов состояния сокращается с пространства
				 *          значений байта до количества классов.
				 *
				 * \~english
				 * Mapping of the byte values to the equivalence classes
				 * @details The bytes indistinguishable by any instruction of the program and by any
				 *          anchor to a position in the text are joined into one class, which is why
				 *          the span of the transition table of a state shrinks from the space
				 *          of byte values to the number of classes.
				 *
				 * \~
				 */
				uint8_t _classes[0x100];
			private:
				/**
				 * \~russian
				 * Соответствие признаков положения индексам исходных состояний
				 *
				 * @details Исходное состояние прохода определяется единственным адресом
				 *          начала выражения вместе с признаками положения в тексте,
				 *          а признаков этих не более тридцати двух. Отыскание такого
				 *          состояния в кэше состояний требовало бы размещения набора
				 *          адресов и построения ключа при каждом проходе, тогда как
				 *          проход по тексту с совпадением вблизи его начала занимает
				 *          немногим больше. Замером на выражении «[0-9]+» получено
				 *          сокращение прохода вдвое.
				 *
				 * \~english
				 * Mapping of the position indications to the indices of the initial states
				 * @details The initial state of a pass is determined by the single address
				 *          of the beginning of the expression together with the indications of the position in the text,
				 *          and there are no more than thirty-two of those indications. Locating such a
				 *          state in the state cache would require allocating an array
				 *          of addresses and building a key on every pass, whereas
				 *          a pass over a text with a match near its beginning takes
				 *          little more. Measurement on the expression «[0-9]+» yielded
				 *          a twofold reduction of the pass.
				 *
				 * \~
				 */
				uint32_t _entries[0x20];
			private:
				// Соответствие наборов состояний их индексам
				unordered_map <string, uint32_t> _cache;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки применимости детерминированного исполнения
				 *
				 * @details Детерминированное исполнение выполняется побайтово, тогда как
				 *          программа в режиме разбора UTF-8 сопоставляет символы целиком,
				 *          поэтому в этом режиме исполнение неприменимо.
				 *
				 * @param program исполняемая программа регулярного выражения
				 * @return        результат проверки применимости исполнения
				 *
				 * \~english
				 * @brief Method of checking the applicability of deterministic execution
				 * @details Deterministic execution is performed byte by byte, whereas
				 *          a program in the UTF-8 parsing mode matches whole characters,
				 *          therefore in that mode the execution is inapplicable.
				 * @param program program of the regular expression being executed
				 * @return        result of checking the applicability of the execution
				 *
				 * \~
				 */
				bool available(const program_t & program) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки наличия совпадения в тексте
				 *
				 * @param program исполняемая программа регулярного выражения
				 * @param text    текст для сопоставления
				 * @param start   позиция начала поиска совпадения
				 * @return        результат проверки наличия совпадения
				 *
				 * \~english
				 * @brief Method of checking the presence of a match in the text
				 * @param program program of the regular expression being executed
				 * @param text    text to match
				 * @param start   position to start the search for a match from
				 * @return        result of checking the presence of a match
				 *
				 * \~
				 */
				bool test(const program_t & program, string_view text, const size_t start) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод поиска позиции завершения совпадения
				 *
				 * @details Метод определяет позицию, в которой завершается совпадение,
				 *          обнаруженное первым при проходе по тексту. Позиция начала
				 *          совпадения при этом не определяется.
				 *
				 * @param program исполняемая программа регулярного выражения
				 * @param text    текст для сопоставления
				 * @param start   позиция начала поиска совпадения
				 * @param finish  позиция завершения обнаруженного совпадения
				 * @return        результат поиска совпадения
				 *
				 * \~english
				 * @brief Method of searching for the position where a match ends
				 * @details The method determines the position at which the match found
				 *          first during the pass over the text ends. The position where the match
				 *          begins is not determined there.
				 * @param program program of the regular expression being executed
				 * @param text    text to match
				 * @param start   position to start the search for a match from
				 * @param finish  position where the found match ends
				 * @return        result of searching for a match
				 *
				 * \~
				 */
				bool search(const program_t & program, string_view text, const size_t start, size_t & finish) noexcept;
				/**
				 * \~russian
				 * @brief Метод поиска позиции начала совпадения
				 *
				 * @details Метод выполняет проход по тексту в обратном направлении по
				 *          развёрнутой программе регулярного выражения и определяет наименьшую
				 *          позицию, с которой начинается совпадение. Проход выполняется до
				 *          начала текста, поскольку совпадение, начинающееся левее, может
				 *          завершаться правее ранее обнаруженного.
				 *
				 * @param program исполняемая развёрнутая программа регулярного выражения
				 * @param text    текст для сопоставления
				 * @param from    позиция, с которой начинается проход в обратном направлении
				 * @param begin   наименьшая позиция начала совпадения
				 * @return        результат поиска позиции начала совпадения
				 *
				 * \~english
				 * @brief Method of searching for the position where a match begins
				 * @details The method performs a backward pass over the text by
				 *          the reversed program of the regular expression and determines the smallest
				 *          position at which a match begins. The pass is performed up to
				 *          the beginning of the text, since a match beginning further to the left may
				 *          end further to the right than the one found earlier.
				 * @param program reversed program of the regular expression being executed
				 * @param text    text to match
				 * @param from    position the backward pass starts from
				 * @param begin   smallest position where a match begins
				 * @return        result of searching for the position where a match begins
				 *
				 * \~
				 */
				bool reverse(const program_t & program, string_view text, const size_t from, size_t & begin) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод извлечения индекса состояния автомата
				 *
				 * @details Состояние отыскивается в кэше по набору адресов инструкций
				 *          и признакам положения в тексте, а при отсутствии создаётся.
				 *
				 * @param list    набор адресов инструкций состояния
				 * @param context набор признаков положения в тексте
				 * @return        индекс состояния детерминированного автомата
				 *
				 * \~english
				 * @brief Method of getting the index of a state of the automaton
				 * @details The state is located in the cache by the set of instruction addresses
				 *          and by the indications of the position in the text, and is created if absent.
				 * @param list    set of the instruction addresses of the state
				 * @param context set of the indications of the position in the text
				 * @return        index of the state of the deterministic automaton
				 *
				 * \~
				 */
				uint32_t state(const vector <address_t> & list, const uint32_t context) noexcept;
				/**
				 * \~russian
				 * @brief Метод вычисления перехода состояния автомата
				 *
				 * @details Метод выполняет замыкание состояния по инструкциям, не
				 *          сопоставляющим символов, с проверкой привязок к позиции в тексте,
				 *          после чего определяет состояние, достигаемое сопоставлением байта.
				 *          Признак обнаружения совпадения размещается в старшем бите результата.
				 *
				 * @param index  индекс исходного состояния автомата
				 * @param letter значение сопоставляемого байта либо признак конца текста
				 * @param last   флаг сопоставления последнего байта текста
				 * @return       индекс достигнутого состояния с признаком совпадения
				 *
				 * \~english
				 * @brief Method of computing a transition of a state of the automaton
				 * @details The method performs the closure of the state over the instructions that do not
				 *          match characters, with a check of the anchors to a position in the text,
				 *          after which it determines the state reached by matching a byte.
				 *          The indication that a match was found is placed in the highest bit of the result.
				 * @param index  index of the initial state of the automaton
				 * @param letter value of the matched byte or the indication of the end of the text
				 * @param last   flag of matching the last byte of the text
				 * @return       index of the reached state with the indication of a match
				 *
				 * \~
				 */
				uint32_t transition(const uint32_t index, const uint32_t letter, const bool last) noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения признаков положения исходного состояния
				 *
				 * @param text текст для сопоставления
				 * @param from позиция начала прохода по тексту
				 * @return     набор признаков положения исходного состояния
				 *
				 * \~english
				 * @brief Method of getting the position indications of the initial state
				 * @param text text to match
				 * @param from position where the pass over the text starts
				 * @return     set of the position indications of the initial state
				 *
				 * \~
				 */
				uint32_t initial(string_view text, const size_t from) const noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод проверки привязки к позиции в тексте
				 *
				 * @param type    тип проверяемой привязки к позиции в тексте
				 * @param flags   набор режимов компиляции инструкции
				 * @param context набор признаков положения в тексте
				 * @param letter  значение следующего байта либо признак конца текста
				 * @param last    флаг нахождения следующего байта в конце текста
				 * @return        результат проверки привязки к позиции в тексте
				 *
				 * \~english
				 * @brief Method of checking an anchor to a position in the text
				 * @param type    type of the checked anchor to a position in the text
				 * @param flags   set of compilation modes of the instruction
				 * @param context set of the indications of the position in the text
				 * @param letter  value of the next byte or the indication of the end of the text
				 * @param last    flag of the next byte being at the end of the text
				 * @return        result of checking the anchor to a position in the text
				 *
				 * \~
				 */
				bool assertion(const anchor_t type, const uint32_t flags, const uint32_t context, const uint32_t letter, const bool last) const noexcept;
				/**
				 * \~russian
				 * @brief Метод прохода по тексту с построением состояний автомата
				 *
				 * @details Направление прохода определяется состоянием объекта. При проходе
				 *          в прямом направлении поиск завершается первым обнаруженным
				 *          совпадением, при проходе в обратном направлении проход выполняется
				 *          до края текста с сохранением наименьшей обнаруженной позиции.
				 *
				 * @param text   текст для сопоставления
				 * @param from   позиция начала прохода по тексту
				 * @param result позиция обнаруженного совпадения
				 * @return       результат прохода по тексту
				 *
				 * \~english
				 * @brief Method of the pass over the text with building the states of the automaton
				 * @details The direction of the pass is determined by the state of the object. During a pass
				 *          in the forward direction the search ends at the first found
				 *          match, during a backward pass the pass is performed
				 *          up to the edge of the text keeping the smallest found position.
				 * @param text   text to match
				 * @param from   position where the pass over the text starts
				 * @param result position of the found match
				 * @return       result of the pass over the text
				 *
				 * \~
				 */
				bool scan(string_view text, const size_t from, size_t & result) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод дробления классов эквивалентности байтов
				 *
				 * @details Метод разделяет каждый класс эквивалентности на два по
				 *          принадлежности байта переданному набору, чем сохраняет
				 *          в одном классе лишь байты, неразличимые всеми переданными
				 *          ранее наборами.
				 *
				 * @param members набор принадлежности значений байта различаемому набору
				 *
				 * \~english
				 * @brief Method of splitting the equivalence classes of bytes
				 * @details The method splits every equivalence class into two by the
				 *          belonging of a byte to the passed set, which keeps
				 *          in one class only the bytes indistinguishable by all the sets passed
				 *          earlier.
				 * @param members set of the belonging of the byte values to the distinguished set
				 *
				 * \~
				 */
				void refine(const bool * members) noexcept;
				/**
				 * \~russian
				 * @brief Метод построения классов эквивалентности байтов
				 *
				 * @details Метод объединяет в один класс байты, неразличимые ни одной
				 *          инструкцией программы и ни одной привязкой к позиции в тексте,
				 *          чем сокращает участок таблицы переходов, отвечающий состоянию.
				 *
				 * \~english
				 * @brief Method of building the equivalence classes of bytes
				 * @details The method joins into one class the bytes indistinguishable by any
				 *          instruction of the program and by any anchor to a position in the text,
				 *          which shrinks the span of the transition table corresponding to a state.
				 *
				 * \~
				 */
				void alphabet() noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод сброса кэша состояний автомата
				 *
				 * \~english
				 * @brief Method of resetting the state cache of the automaton
				 *
				 * \~
				 */
				void reset() noexcept;
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
				Dfa() noexcept;
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
				~Dfa() noexcept {}
		} dfa_t;
	};
};

#endif // __AWH_REGEX_DFA__
