/**
 * @file regex.hpp
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
 * @brief Заголовочный файл открытого интерфейса модуля регулярных выражений —
 *        сборка разделяемых регулярных выражений с кэшем собранного, сопоставление
 *        выражения с текстом и извлечение захваченных групп по номеру и по имени
 *
 * @section regex_decisions Намеренные решения
 *
 * @details Перечисленное ниже выглядит несообразностью, но выбрано осознанно и
 *          правке не подлежит. Раздел заведён затем, чтобы разбор кода не начинался
 *          каждый раз с одних и тех же выводов.
 *
 *          <b>Согласования доступа модуль не ведёт вовсе.</b> Рабочее состояние
 *          сопоставления хранится отдельно для каждого потока исполнения, а
 *          скомпилированное выражение после сборки не изменяется и разделяется
 *          потоками без согласования. Кэш же собранных выражений и реестр шаблонов
 *          суть поля объекта, а не память, общая на всё приложение: объектом владеет
 *          сторонний разработчик, и защитить его он волен сам, средствами, задаче
 *          отвечающими. Framework работы этой за него не делает: блокировка,
 *          навязанная всем, оборачивается взаимными захватами и провалом
 *          производительности там, где многопоточность не нужна вовсе.
 *
 *          <b>Отчёт об ошибке живёт потоком исполнения, а не объектом.</b>
 *          Код ошибки, смещение её и текст суть состояние последней операции
 *          потока наравне с рабочим состоянием сопоставления. Живи отчёт полем
 *          объекта, сопоставление несколькими потоками одним объектом - способ
 *          работы, договором обещанный, - писало бы в него разом, и это гонка
 *          данных. Ценою тому общий отчёт у нескольких объектов одного потока:
 *          читается он сразу за отказавшей операцией, и порядок этот договором
 *          и предполагается.
 *
 *          <b>Кэш собранных выражений хранит слабые ссылки.</b> Выражение живёт,
 *          пока его удерживает вызывающая сторона; кэш лишь избавляет от повторной
 *          сборки того же выражения с тем же набором режимов. Освобождение
 *          последней ссылки освобождает и память выражения.
 *
 *          <b>Метод exec выводит захваченный текст, а метод match - границы
 *          захвата.</b> Границы избавляют от копирования текста и остаются
 *          единственным способом отличить пустой захват от невыполненного,
 *          поэтому оба способа существуют наравне.
 *
 * \~english
 * @brief Header file of the public interface of the regular expression module —
 *        building shared regular expressions with a cache of what has been built, matching
 *        an expression against a text and getting the captured groups by number and by name
 * @section regex_decisions Deliberate decisions
 * @details What is listed below looks like an incongruity, but was chosen deliberately and
 *          is not subject to correction. The section is introduced so that reading the code does not start
 *          every time from the same conclusions.
 *          <b>The module performs no coordination of the access at all.</b> The working
 *          state of the matching is kept separately for every thread of execution, and
 *          a compiled expression is not changed after building and is shared by the threads
 *          without coordination. The cache of built expressions and the registry of patterns
 *          are fields of an object rather than memory common to the whole application: the object
 *          is owned by an outside developer, and he is free to protect it himself, by the means
 *          that suit the task. The Framework does not do that work for him: a lock imposed
 *          on everyone turns into deadlocks and a collapse of the performance where
 *          multithreading is not needed at all.
 *          <b>The cache of built expressions keeps weak references.</b> An expression lives
 *          as long as the calling side holds it; the cache only relieves from repeated
 *          building of the same expression with the same set of modes. Releasing
 *          the last reference also releases the memory of the expression.
 *          <b>The exec method yields the captured text, and the match method yields the
 *          capture boundaries.</b> The boundaries relieve from copying the text and remain
 *          the only way to tell an empty capture from an unperformed one,
 *          therefore both ways exist on equal terms.
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_REGEX_INTERFACE__
#define __AWH_REGEX_INTERFACE__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <utility>
#include <string_view>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "engine.hpp"

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
	 * @brief Класс работы с регулярными выражениями
	 *
	 * @details Класс собирает регулярные выражения и сопоставляет их с текстом.
	 *          Собранное выражение отделено от объекта сборки, разделяется
	 *          несколькими объектами и потоками исполнения одновременно
	 *          и освобождается вместе с последней ссылкой на него.
	 *
	 * \~english
	 * @brief Class for working with regular expressions
	 * @details The class builds regular expressions and matches them against a text.
	 *          A built expression is separated from the building object, is shared
	 *          by several objects and threads of execution at once
	 *          and is released together with the last reference to it.
	 *
	 * \~
	 */
	typedef class __AWH_SHARED_EXPORT__ RegularExpression {
		public:
			/**
			 * \~russian
			 * @brief Режимы сборки регулярного выражения
			 *
			 * \~english
			 * @brief Build modes of a regular expression
			 *
			 * \~
			 */
			using flag_t = awh::regex::flag_t;
			/**
			 * \~russian
			 * @brief Код ошибки сборки регулярного выражения
			 *
			 * \~english
			 * @brief Build error code of a regular expression
			 *
			 * \~
			 */
			using error_t = awh::regex::error_t;
			/**
			 * \~russian
			 * @brief Собранное регулярное выражение
			 *
			 * \~english
			 * @brief Built regular expression
			 *
			 * \~
			 */
			using exp_t = shared_ptr <const awh::regex::expression_t>;
		private:
			/**
			 * \~russian
			 * @brief Ключ кэша собранных регулярных выражений
			 *
			 * \~english
			 * @brief Key of the cache of built regular expressions
			 *
			 * \~
			 */
			using key_t = pair <uint32_t, string>;
		private:
			/**
			 * \~russian
			 * @brief Хэш-функция ключа кэша собранных регулярных выражений
			 *
			 * \~english
			 * @brief Hash function of the key of the cache of built regular expressions
			 *
			 * \~
			 */
			struct __AWH_SHARED_EXPORT__ Hash {
				/**
				 * \~russian
				 * @brief Оператор вычисления хэша ключа кэша
				 *
				 * @param key ключ кэша собранных регулярных выражений
				 * @return    вычисленное значение хэша ключа
				 *
				 * \~english
				 * @brief Operator computing the hash of a cache key
				 * @param key key of the cache of built regular expressions
				 * @return    computed hash value of the key
				 *
				 * \~
				 */
				size_t operator () (const key_t & key) const noexcept;
			};
		private:
			// Объект журнала событий
			const log_t * _log;
		private:
			/**
			 * \~russian
			 * Наибольший допустимый объём работы сопоставления
			 *
			 * @details Потолок хранится полем объекта, а не движка: движок сопоставления
			 *          принадлежит потоку исполнения, и настройка, на нём поставленная,
			 *          действовала бы в одном потоке, тогда как принадлежит она объекту.
			 *
			 * \~english
			 * Largest admissible amount of work of the matching
			 * @details The ceiling is stored as a field of the object rather than of the engine: the matching
			 *          engine belongs to the thread of execution, and a setting placed on it
			 *          would act in one thread, whereas it belongs to the object.
			 *
			 * \~
			 */
			size_t _limit;
		private:
			/**
			 * \~russian
			 * Наибольшая допустимая глубина рекурсивных вызовов подвыражений
			 * @details Глубина держится полем объекта, а не движка, по той же причине,
			 *          что и предел объёма работы: движок принадлежит потоку исполнения,
			 *          а настройка - объекту.
			 *
			 * \~english
			 * Largest admissible depth of recursive calls of subpatterns
			 * @details The depth is stored as a field of the object rather than of the engine
			 *          for the same reason as the limit of the amount of work: the engine
			 *          belongs to the thread of execution, whereas the setting belongs
			 *          to the object.
			 *
			 * \~
			 */
			size_t _nesting;
		private:
			// Кэш собранных регулярных выражений
			mutable unordered_map <key_t, weak_ptr <const awh::regex::expression_t>, Hash> _cache;
		private:
			/**
			 * \~russian
			 * @brief Метод снятия кода ошибки сопоставления с движка
			 *
			 * @details Отказ сопоставления бывает двух родов: совпадения в тексте
			 *          нет вовсе либо движок исчерпал допустимый объём работы,
			 *          и потребителю различать их надлежит опросом error(). Без
			 *          снятия кода с движка оба рода выглядели бы одинаково.
			 *
			 * \~english
			 * @brief Method of taking the matching error code from the engine
			 * @details A refusal of the matching is of two kinds: there is no match
			 *          in the text at all, or the engine has exhausted the admissible
			 *          amount of work, and the consumer is to tell them apart by
			 *          polling error(). Without taking the code from the engine
			 *          both kinds would look the same.
			 *
			 * \~
			 */
			void receive() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод сборки регулярного выражения
			 *
			 * @details Повторная сборка того же выражения с тем же набором режимов
			 *          выводит собранное ранее выражение, если оно ещё удерживается
			 *          вызывающей стороной.
			 *
			 * @param pattern текст регулярного выражения
			 * @param flags   набор режимов сборки регулярного выражения
			 * @return        собранное регулярное выражение
			 *
			 * \~english
			 * @brief Method of building a regular expression
			 * @details A repeated build of the same expression with the same set of modes
			 *          yields the previously built expression, if it is still held
			 *          by the calling side.
			 * @param pattern text of the regular expression
			 * @param flags   set of build modes of the regular expression
			 * @return        built regular expression
			 *
			 * \~
			 */
			exp_t build(string_view pattern, const uint32_t flags = 0) const noexcept;
			/**
			 * \~russian
			 * @brief Метод сборки регулярного выражения
			 *
			 * @param pattern текст регулярного выражения
			 * @param flags   набор режимов сборки регулярного выражения
			 * @return        собранное регулярное выражение
			 *
			 * \~english
			 * @brief Method of building a regular expression
			 * @param pattern text of the regular expression
			 * @param flags   set of build modes of the regular expression
			 * @return        built regular expression
			 *
			 * \~
			 */
			exp_t build(string_view pattern, const vector <flag_t> & flags) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод проверки наличия совпадения в тексте
			 *
			 * @param text текст для сопоставления
			 * @param exp  собранное регулярное выражение
			 * @return     результат проверки наличия совпадения
			 *
			 * \~english
			 * @brief Method of checking the presence of a match in the text
			 * @param text text to match
			 * @param exp  built regular expression
			 * @return     result of checking the presence of a match
			 *
			 * \~
			 */
			bool test(string_view text, const exp_t & exp) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения границ совпадения и захваченных групп
			 *
			 * @details Границы невыполненного захвата равны позиции отсутствующего
			 *          символа, тогда как границы пустого захвата совпадают между
			 *          собой и позиции отсутствующего символа не равны.
			 *
			 * @param text текст для сопоставления
			 * @param exp  собранное регулярное выражение
			 * @return     набор границ совпадения и захваченных групп
			 *
			 * \~english
			 * @brief Method of getting the boundaries of a match and of the captured groups
			 * @details The boundaries of an unperformed capture equal the position of a missing
			 *          character, whereas the boundaries of an empty capture coincide with each
			 *          other and do not equal the position of a missing character.
			 * @param text text to match
			 * @param exp  built regular expression
			 * @return     set of the boundaries of the match and of the captured groups
			 *
			 * \~
			 */
			vector <pair <size_t, size_t>> match(string_view text, const exp_t & exp) const noexcept;
			/**
			 * \~russian
			 * @brief Метод извлечения границ совпадения и захваченных групп
			 *
			 * @details Набор границ передаётся вызывающей стороной и переиспользуется
			 *          ею между сопоставлениями, избавляя каждое сопоставление
			 *          от размещения памяти.
			 *
			 * @param text   текст для сопоставления
			 * @param exp    собранное регулярное выражение
			 * @param result набор границ совпадения и захваченных групп
			 * @return       результат поиска совпадения
			 *
			 * \~english
			 * @brief Method of getting the boundaries of a match and of the captured groups
			 * @details The set of boundaries is passed by the calling side and is reused
			 *          by it between matches, relieving every match
			 *          from allocating memory.
			 * @param text   text to match
			 * @param exp    built regular expression
			 * @param result set of the boundaries of the match and of the captured groups
			 * @return       result of searching for a match
			 *
			 * \~
			 */
			bool match(string_view text, const exp_t & exp, vector <pair <size_t, size_t>> & result) const noexcept;
			/**
			 * \~russian
			 * @brief Метод извлечения текста совпадения и захваченных групп
			 *
			 * @details Первым выводится текст совпадения целиком, за ним - текст,
			 *          захваченный группами в порядке их объявления. Невыполненному
			 *          захвату соответствует пустой текст.
			 *
			 * @param text текст для сопоставления
			 * @param exp  собранное регулярное выражение
			 * @return     набор текста совпадения и захваченных групп
			 *
			 * \~english
			 * @brief Method of getting the text of a match and of the captured groups
			 * @details First the text of the whole match is yielded, followed by the text
			 *          captured by the groups in the order of their declaration. An unperformed
			 *          capture corresponds to an empty text.
			 * @param text text to match
			 * @param exp  built regular expression
			 * @return     set of the text of the match and of the captured groups
			 *
			 * \~
			 */
			vector <string> exec(string_view text, const exp_t & exp) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения номера именованной группы
			 *
			 * @details В режиме «DUPNAMES» одно имя объявляется несколькими группами,
			 *          и метод выводит номер объявленной первой. Захваченный такими
			 *          группами текст извлекается методом capture, выбирающим ту
			 *          из них, что захват выполнила.
			 *
			 * @param exp  собранное регулярное выражение
			 * @param name имя именованной группы выражения
			 * @return     номер именованной группы либо ноль при её отсутствии
			 *
			 * \~english
			 * @brief Method of getting the number of a named group
			 * @details In the «DUPNAMES» mode one name is declared by several groups,
			 *          and the method yields the number of the one declared first. The text captured by such
			 *          groups is obtained by the capture method, which chooses the one
			 *          of them that performed the capture.
			 * @param exp  built regular expression
			 * @param name name of the named group of the expression
			 * @return     number of the named group or zero if it is absent
			 *
			 * \~
			 */
			uint32_t group(const exp_t & exp, string_view name) const noexcept;
			/**
			 * \~russian
			 * @brief Метод извлечения соответствия имён групп наборам их номеров
			 *
			 * @param exp собранное регулярное выражение
			 * @return    соответствие имён именованных групп наборам их номеров
			 *
			 * \~english
			 * @brief Method of getting the mapping of the group names to the sets of their numbers
			 * @param exp built regular expression
			 * @return    mapping of the names of the named groups to the sets of their numbers
			 *
			 * \~
			 */
			const unordered_map <string, vector <uint32_t>> & groups(const exp_t & exp) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения текста, захваченного именованной группой
			 *
			 * @details Набор границ получен методом match, благодаря чему извлечение
			 *          нескольких групп повторного сопоставления не требует. В режиме
			 *          «DUPNAMES» выводится текст той из одноимённых групп, что захват
			 *          выполнила, как это делает эталонная реализация.
			 *
			 *          Захват пустого текста от невыполненного захвата отличается
			 *          тем, что выводимый вид ссылается на текст сопоставления,
			 *          тогда как при невыполненном захвате вид пуст и ни на что
			 *          не ссылается.
			 *
			 * @param text   текст, с которым выполнялось сопоставление
			 * @param bounds набор границ совпадения и захваченных групп
			 * @param exp    собранное регулярное выражение
			 * @param name   имя именованной группы выражения
			 * @return       текст, захваченный именованной группой
			 *
			 * \~english
			 * @brief Method of getting the text captured by a named group
			 * @details The set of boundaries is obtained by the match method, thanks to which getting
			 *          several groups requires no repeated matching. In the
			 *          «DUPNAMES» mode the text of the one of the same-named groups that performed the
			 *          capture is yielded, as the reference implementation does.
			 *          A capture of an empty text differs from an unperformed capture in
			 *          that the yielded view refers to the matched text,
			 *          whereas on an unperformed capture the view is empty and refers to
			 *          nothing.
			 * @param text   text the matching was performed against
			 * @param bounds set of the boundaries of the match and of the captured groups
			 * @param exp    built regular expression
			 * @param name   name of the named group of the expression
			 * @return       text captured by the named group
			 *
			 * \~
			 */
			string_view capture(string_view text, const vector <pair <size_t, size_t>> & bounds, const exp_t & exp, string_view name) const noexcept;
			/**
			 * \~russian
			 * @brief Метод сопоставления с извлечением именованных групп
			 *
			 * @details Метод выполняет сопоставление и выводит текст, захваченный
			 *          каждой именованной группой выражения. Группы, захвата
			 *          не выполнившие, в выводимое соответствие не попадают.
			 *
			 * @param text   текст для сопоставления
			 * @param exp    собранное регулярное выражение
			 * @param result соответствие имён именованных групп захваченному тексту
			 * @return       результат поиска совпадения
			 *
			 * \~english
			 * @brief Method of matching with getting the named groups
			 * @details The method performs the matching and yields the text captured
			 *          by every named group of the expression. The groups that performed no
			 *          capture do not fall into the yielded mapping.
			 * @param text   text to match
			 * @param exp    built regular expression
			 * @param result mapping of the names of the named groups to the captured text
			 * @return       result of searching for a match
			 *
			 * \~
			 */
			bool exec(string_view text, const exp_t & exp, unordered_map <string, string> & result) const noexcept;
			/**
			 * \~russian
			 * @brief Метод извлечения количества захватывающих групп
			 *
			 * @param exp собранное регулярное выражение
			 * @return    количество захватывающих групп выражения
			 *
			 * \~english
			 * @brief Method of getting the number of capturing groups
			 * @param exp built regular expression
			 * @return    number of capturing groups of the expression
			 *
			 * \~
			 */
			uint32_t captures(const exp_t & exp) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения кода ошибки последней операции
			 *
			 * @return код ошибки последней операции потока исполнения
			 *
			 * \~english
			 * @brief Method of getting the error code of the last operation
			 * @return error code of the last operation of the thread
			 *
			 * \~
			 */
			error_t error() const noexcept;
			/**
			 * \~russian
			 * @brief Метод извлечения имени отметки совпадения последнего
			 *
			 * @details Имя оставляет глагол «(*MARK:имя)», путь отметивший, чем
			 *          указывает ветвь выражения, совпадение давшую. Выражение,
			 *          глаголов отметки не несущее, и совпадение, их не прошедшее,
			 *          выводят имя пустое.
			 *
			 * @return имя отметки совпадения последнего
			 *
			 * \~english
			 * @brief Method of getting the name of the mark of the last match
			 * @details The name is left by the «(*MARK:name)» verb that marked the path, thereby
			 *          telling which branch of the expression produced the match. An expression
			 *          carrying no mark verbs, and a match that passed none of them,
			 *          yield an empty name.
			 * @return name of the mark of the last match
			 *
			 * \~
			 */
			const string & marker() const noexcept;
			/**
			 * \~russian
			 * @brief Метод извлечения смещения ошибки последней операции
			 *
			 * @return смещение ошибки в тексте регулярного выражения
			 *
			 * \~english
			 * @brief Method of getting the error offset of the last operation
			 * @return offset of the error in the text of the regular expression
			 *
			 * \~
			 */
			size_t offset() const noexcept;
			/**
			 * \~russian
			 * @brief Метод извлечения текста ошибки последней операции
			 *
			 * @return текст ошибки последней операции потока исполнения
			 *
			 * \~english
			 * @brief Method of getting the error text of the last build
			 * @return error text of the last build operation
			 *
			 * \~
			 */
			const string & message() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод очистки кэша собранных регулярных выражений
			 *
			 * \~english
			 * @brief Method of clearing the cache of built regular expressions
			 *
			 * \~
			 */
			void clear() noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки наибольшего допустимого объёма работы сопоставления
			 *
			 * @details Сопоставление выражений с вложенными кванторами требует времени,
			 *          растущего с длиной текста показательно, поэтому объём работы
			 *          исполнения с возвратом ограничивается. Исчерпание предела
			 *          совпадения не отменяет: сопоставление доводится способом запасным,
			 *          время которого от вида выражения не зависит, - оттого понижение
			 *          предела лишь уводит на способ этот раньше, а повышение позволяет
			 *          дойти с возвратом выражению более трудному. Нуль предел снимает
			 *          вовсе, и время сопоставления не ограничивается ничем.
			 *
			 * @note Умолчанием служит десять миллионов шагов, чего достаёт выражениям
			 *       обыкновенным на текстах любой длины
			 *
			 * @param limit наибольшее допустимое количество шагов сопоставления
			 *
			 * \~english
			 * @brief Method of setting the largest admissible amount of work of the matching
			 * @details Matching expressions with nested quantifiers requires time
			 *          growing with the length of the text exponentially, therefore the amount of work
			 *          of the execution with backtracking is bounded. Exhausting the limit
			 *          does not cancel the match: the matching is completed by the fallback way,
			 *          whose time does not depend on the kind of the expression, - therefore lowering
			 *          the limit merely moves to that way earlier, and raising it allows
			 *          a more difficult expression to be completed with backtracking. Zero removes the limit
			 *          entirely, and the time of the matching is bounded by nothing.
			 * @note Ten million steps serves as the default, which suffices for ordinary
			 *       expressions on texts of any length
			 * @param limit largest admissible number of matching steps
			 *
			 * \~
			 */
			void limit(const size_t limit) noexcept;
			/**
			 * \~russian
			 * @brief Метод установки наибольшей допустимой глубины рекурсивных вызовов
			 *
			 * @details Ссылки подвыражений на себя разворачиваются вглубь, и глубина
			 *          эта ограничивается: память под кадры вызовов растёт соразмерно
			 *          ей. Превышение глубины отвечает отказом сопоставления с кодом
			 *          превышения допустимого объёма работы. Нуль восстанавливает
			 *          глубину умолчания.
			 *
			 * @note Умолчанием служит тысяча вызовов - столько же берёт и эталон
			 *
			 * @param nesting наибольшая допустимая глубина рекурсивных вызовов
			 *
			 * \~english
			 * @brief Method of setting the largest admissible depth of recursive calls
			 * @details References of subpatterns to themselves are unfolded in depth,
			 *          and that depth is bounded: the memory of the call frames grows
			 *          in proportion to it. Exceeding the depth answers with a refusal
			 *          of the matching with the code of exceeding the admissible amount
			 *          of work. Zero restores the depth of the default.
			 * @note One thousand calls serves as the default - the reference takes
			 *       the same amount
			 * @param nesting largest admissible depth of recursive calls
			 *
			 * \~
			 */
			void nesting(const size_t nesting) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 *
			 * \~english
			 * @brief Constructor
			 * @param log object for working with logs
			 *
			 * \~
			 */
			explicit RegularExpression(const log_t * log) noexcept;
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
			~RegularExpression() noexcept {}
	} regexp_t;
};

#endif // __AWH_REGEX_INTERFACE__
