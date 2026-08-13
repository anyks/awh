/**
 * @file: pike.hpp
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
 * @brief Заголовочный файл исполнения регулярных выражений без возврата — класс Pike,
 *        выполняющий одновременную симуляцию всех состояний недетерминированного конечного
 *        автомата с извлечением границ захваченных групп за линейное время
 *
 * \~english
 * @brief Header file of the execution of regular expressions without backtracking — the Pike class,
 *        which performs the simultaneous simulation of all the states of a nondeterministic finite
 *        automaton with getting the boundaries of the captured groups in linear time
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_REGEX_PIKE__
#define __AWH_REGEX_PIKE__

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
#include "dfa.hpp"
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
		 * @brief Признак отсутствия набора позиций захвата групп
		 *
		 * \~english
		 * @brief Indication of the absence of a set of group capture positions
		 *
		 * \~
		 */
		constexpr uint32_t NO_SLOTS = 0xFFFFFFFF;

		/**
		 * \~russian
		 * @brief Режим сопоставления регулярного выражения с текстом
		 *
		 * \~english
		 * @brief Mode of matching a regular expression against a text
		 *
		 * \~
		 */
		enum class mode_t : uint8_t {
			PLAIN    = 0x00, // Поиск совпадения по тексту с проверкой его наличия
			VERIFIED = 0x01, // Поиск совпадения по тексту, наличие которого установлено
			ANCHORED = 0x02  // Сопоставление, начинающееся в позиции начала поиска
		};

		/**
		 * \~russian
		 * @brief Класс исполнения регулярного выражения без возврата
		 *
		 * @details Класс выполняет одновременную симуляцию всех достижимых состояний
		 *          недетерминированного конечного автомата. Состояния упорядочены по
		 *          убыванию приоритета, благодаря чему выбор ветви совпадает с выбором
		 *          реализации с возвратом, а время исполнения остаётся линейным
		 *          относительно длины текста независимо от вида выражения.
		 *
		 * \~english
		 * @brief Class of the execution of a regular expression without backtracking
		 * @details The class performs the simultaneous simulation of all the reachable states
		 *          of a nondeterministic finite automaton. The states are ordered by
		 *          decreasing priority, thanks to which the choice of a branch coincides with the choice of
		 *          an implementation with backtracking, while the execution time remains linear
		 *          with respect to the length of the text regardless of the kind of the expression.
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Pike {
			private:
				/**
				 * \~russian
				 * @brief Состояние симуляции конечного автомата
				 *
				 * \~english
				 * @brief State of the simulation of the finite automaton
				 *
				 * \~
				 */
				typedef struct Thread {
					// Адрес исполняемой инструкции программы
					address_t pc;
					/**
					 * \~russian
					 * Номер набора позиций захвата групп
					 *
					 * @details Набор разделяется состояниями и замещается новым при
					 *          сохранении позиции, благодаря чему размножение состояний
					 *          не сопровождается копированием набора целиком. Состояние
					 *          удерживает одну ссылку на набор, размещённый в хранилище.
					 *
					 * \~english
					 * Number of the set of group capture positions
					 * @details The set is shared by the states and is replaced by a new one when
					 *          a position is saved, thanks to which multiplying the states
					 *          is not accompanied by copying the whole set. A state
					 *          holds one reference to a set placed in the storage.
					 *
					 * \~
					 */
					uint32_t slots;
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
					Thread() noexcept : pc(0), slots(NO_SLOTS) {}
				} thread_t;
			private:
				/**
				 * \~russian
				 * Объект детерминированного исполнения регулярного выражения
				 *
				 * @details Детерминированное исполнение определяет наличие совпадения
				 *          многократно быстрее исполнения с сопровождением набора
				 *          состояний, но границ захваченных групп не устанавливает,
				 *          поэтому применяется исключительно для отказа от поиска.
				 *
				 * \~english
				 * Object of the deterministic execution of a regular expression
				 * @details Deterministic execution determines the presence of a match
				 *          many times faster than execution that keeps a set of
				 *          states, but does not establish the boundaries of the captured groups,
				 *          therefore it is used exclusively to give up the search.
				 *
				 * \~
				 */
				dfa_t _dfa;
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
				// Набор состояний, исполняемых в текущей позиции
				vector <thread_t> _current;
			private:
				// Набор состояний, исполняемых в следующей позиции
				vector <thread_t> _next;
			private:
				/**
				 * \~russian
				 * Стек замыкания состояния по инструкциям без сопоставления символов
				 *
				 * @details Стек размещается в объекте, а не в методе замыкания, благодаря
				 *          чему выделенная под него память применяется повторно всеми
				 *          замыканиями вместо выделения при каждом замыкании.
				 *
				 * \~english
				 * Stack of the closure of a state over the instructions that match no characters
				 * @details The stack is placed in the object rather than in the closure method, thanks to
				 *          which the memory allocated for it is reused by all the
				 *          closures instead of being allocated at every closure.
				 *
				 * \~
				 */
				vector <thread_t> _stack;
			private:
				/**
				 * \~russian
				 * Набор отметок посещения инструкций для состояний текущей позиции
				 *
				 * @details Наборы отметок для текущей и следующей позиций раздельны:
				 *          общий набор блокировал бы размещение состояния продолжения,
				 *          инструкция которого уже посещена в текущей позиции.
				 *
				 * \~english
				 * Set of the visit marks of the instructions for the states of the current position
				 * @details The sets of marks for the current and for the next positions are separate:
				 *          a common set would block the placement of the continuation state
				 *          whose instruction has already been visited at the current position.
				 *
				 * \~
				 */
				vector <uint32_t> _marks;
			private:
				// Набор отметок посещения инструкций для состояний следующей позиции
				vector <uint32_t> _pending;
			private:
				// Номер поколения отметок посещения текущей позиции
				uint32_t _generation;
			private:
				// Номер поколения отметок посещения следующей позиции
				uint32_t _upcoming;
			private:
				// Количество позиций захвата групп в наборе
				size_t _width;
			private:
				/**
				 * \~russian
				 * Хранилище наборов позиций захвата групп
				 *
				 * @details Наборы размещаются в хранилище и учитываются количеством
				 *          удерживающих их состояний. Освобождённые наборы возвращаются
				 *          в хранилище и выдаются повторно, благодаря чему размножение
				 *          состояний не сопровождается выделением памяти.
				 *
				 *          Наборы размещены одним сплошным набором, в котором набору
				 *          отвечает отрезок длиной в количество позиций захвата.
				 *          Размещение каждого набора отдельным набором обходилось
				 *          в два обращения к памяти на каждую позицию захвата, а
				 *          замещение набора при сохранении позиции — в размещение
				 *          набора взамен копирования отрезка.
				 *
				 * \~english
				 * Storage of the sets of group capture positions
				 * @details The sets are placed in the storage and are counted by the number
				 *          of the states holding them. Released sets are returned
				 *          to the storage and are handed out again, thanks to which multiplying
				 *          the states is not accompanied by allocating memory.
				 *          The sets are placed as a single contiguous array in which a set
				 *          corresponds to a stretch as long as the number of capture positions.
				 *          Placing every set as a separate array cost
				 *          two memory references per every capture position, and
				 *          replacing a set when saving a position cost an allocation of
				 *          a set instead of copying a stretch.
				 *
				 * \~
				 */
				vector <size_t> _storage;
			private:
				// Набор количеств состояний, удерживающих наборы позиций захвата групп
				vector <uint32_t> _refs;
			private:
				// Набор номеров освобождённых наборов позиций захвата групп
				vector <uint32_t> _vacant;
			public:
				/**
				 * \~russian
				 * @brief Метод сопоставления регулярного выражения с текстом
				 *
				 * @details Поиск совпадения выполняется начиная с указанной позиции.
				 *          Границы совпадения и захваченных групп размещаются в наборе
				 *          результата: первый элемент набора содержит границы совпадения
				 *          целиком, последующие — границы захваченных групп по их номерам.
				 *          Границы незахваченных групп принимают значение «string_view::npos».
				 *
				 * @param program  исполняемая программа регулярного выражения
				 * @param text     текст для сопоставления
				 * @param start    позиция начала поиска совпадения
				 * @param captures набор границ совпадения и захваченных групп
				 * @return         результат поиска совпадения
				 *
				 * \~english
				 * @brief Method of matching a regular expression against a text
				 * @details The search for a match is performed starting from the specified position.
				 *          The boundaries of the match and of the captured groups are placed in the result
				 *          set: the first element of the set holds the boundaries of the whole
				 *          match, the following ones the boundaries of the captured groups by their numbers.
				 *          The boundaries of the uncaptured groups take the «string_view::npos» value.
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
				 * @details Режим «ANCHORED» требует начала совпадения именно в переданной
				 *          позиции и применяется, если позиция начала совпадения установлена
				 *          заранее. Режим «VERIFIED» избавляет от проверки наличия совпадения
				 *          детерминированным исполнением, если наличие уже установлено
				 *          вызывающей стороной.
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
				 * @details The «ANCHORED» mode requires the match to begin exactly at the passed
				 *          position and is used if the position where the match begins has been established
				 *          in advance. The «VERIFIED» mode relieves from checking the presence of a match
				 *          by deterministic execution if the presence has already been established
				 *          by the calling side.
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
				 * @brief Метод добавления состояния в набор исполняемых состояний
				 *
				 * @details Метод выполняет замыкание состояния по инструкциям, не
				 *          сопоставляющим символов, сохраняя порядок убывания приоритета.
				 *          Повторное добавление инструкции в пределах одной позиции
				 *          не выполняется.
				 *
				 * @param list       набор исполняемых состояний
				 * @param marks      набор отметок посещения инструкций программы
				 * @param generation номер поколения отметок посещения
				 * @param pc         адрес добавляемой инструкции программы
				 * @param slots      номер набора позиций захвата групп
				 * @param pos        позиция в тексте, для которой добавляется состояние
				 *
				 * \~english
				 * @brief Method of adding a state to the set of executed states
				 * @details The method performs the closure of the state over the instructions that do not
				 *          match characters, keeping the order of decreasing priority.
				 *          A repeated addition of an instruction within one position
				 *          is not performed.
				 * @param list       set of executed states
				 * @param marks      set of the visit marks of the program instructions
				 * @param generation generation number of the visit marks
				 * @param pc         address of the added program instruction
				 * @param slots      number of the set of group capture positions
				 * @param pos        position in the text the state is added for
				 *
				 * \~
				 */
				void append(vector <thread_t> & list, vector <uint32_t> & marks, const uint32_t generation, const address_t pc, const uint32_t slots, const size_t pos) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод выделения набора позиций захвата групп
				 *
				 * @details Набор отыскивается среди освобождённых, а при их отсутствии
				 *          создаётся. Выданный набор удерживается единственной ссылкой.
				 *
				 * @return номер выделенного набора позиций захвата групп
				 *
				 * \~english
				 * @brief Method of allotting a set of group capture positions
				 * @details The set is looked up among the released ones, and in their absence
				 *          is created. The handed out set is held by a single reference.
				 * @return number of the allotted set of group capture positions
				 *
				 * \~
				 */
				uint32_t acquire() noexcept;
				/**
				 * \~russian
				 * @brief Метод удержания набора позиций захвата групп
				 *
				 * @param index номер удерживаемого набора позиций захвата групп
				 *
				 * \~english
				 * @brief Method of holding a set of group capture positions
				 * @param index number of the held set of group capture positions
				 *
				 * \~
				 */
				void retain(const uint32_t index) noexcept;
				/**
				 * \~russian
				 * @brief Метод освобождения набора позиций захвата групп
				 *
				 * @details Набор, освобождённый последней удерживающей его ссылкой,
				 *          возвращается в хранилище для повторной выдачи.
				 *
				 * @param index номер освобождаемого набора позиций захвата групп
				 *
				 * \~english
				 * @brief Method of releasing a set of group capture positions
				 * @details A set released by the last reference holding it
				 *          is returned to the storage for being handed out again.
				 * @param index number of the released set of group capture positions
				 *
				 * \~
				 */
				void release(const uint32_t index) noexcept;
				/**
				 * \~russian
				 * @brief Метод сохранения позиции в ячейке захвата
				 *
				 * @details Набор, удерживаемый единственной ссылкой, изменяется на месте.
				 *          Набор, разделяемый прочими состояниями, замещается его копией,
				 *          а удерживаемая вызывающей стороной ссылка освобождается.
				 *
				 * @param index номер изменяемого набора позиций захвата групп
				 * @param slot  номер ячейки захвата, в которой сохраняется позиция
				 * @param pos   сохраняемая в ячейке захвата позиция в тексте
				 * @return      номер набора позиций захвата групп с сохранённой позицией
				 *
				 * \~english
				 * @brief Method of saving a position in a capture cell
				 * @details A set held by a single reference is changed in place.
				 *          A set shared by the other states is replaced by its copy,
				 *          and the reference held by the calling side is released.
				 * @param index number of the changed set of group capture positions
				 * @param slot  number of the capture cell the position is saved in
				 * @param pos   position in the text saved in the capture cell
				 * @return      number of the set of group capture positions with the saved position
				 *
				 * \~
				 */
				uint32_t assign(const uint32_t index, const uint32_t slot, const size_t pos) noexcept;
				/**
				 * \~russian
				 * @brief Метод очистки набора исполняемых состояний
				 *
				 * @details Очистка сопровождается освобождением наборов позиций захвата
				 *          групп, удерживаемых удаляемыми состояниями.
				 *
				 * @param list очищаемый набор исполняемых состояний
				 *
				 * \~english
				 * @brief Method of clearing the set of executed states
				 * @details The clearing is accompanied by the release of the sets of group capture
				 *          positions held by the removed states.
				 * @param list set of executed states to clear
				 *
				 * \~
				 */
				void discard(vector <thread_t> & list) noexcept;
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
				Pike() noexcept;
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
				~Pike() noexcept {}
		} pike_t;
	};
};

#endif // __AWH_REGEX_PIKE__
