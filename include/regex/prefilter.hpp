/**
 * @file prefilter.hpp
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
 * @brief Заголовочный файл предварительного отбора позиций сопоставления — набор байтов,
 *        допустимых в начале совпадения, и обязательный литерал совпадения, позволяющие
 *        пропускать участки текста без запуска конечного автомата
 *
 * \~english
 * @brief Header file of the preliminary selection of matching positions — the set of bytes
 *        admissible at the beginning of a match and the mandatory literal of a match, which allow
 *        skipping stretches of the text without starting the finite automaton
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_REGEX_PREFILTER__
#define __AWH_REGEX_PREFILTER__

/**
 * Если принудительная подстановка ещё не определена
 *
 * @details Определение ограждается потому, что заголовочные файлы модуля
 *          подключаются в разном порядке, а определение это несут два из них.
 *          Visual Studio на повторное определение отвечает предупреждением
 *          C4005, и виден он лишь у него: GCC и Clang повторное определение
 *          тождественное пропускают молча.
 *
 */
#ifndef AWH_REGEX_INLINE
	/**
	 * Если компилятор является Visual Studio
	 */
	#if defined(_MSC_VER)
		/**
		 * Принудительная подстановка средствами Visual Studio
		 */
		#define AWH_REGEX_INLINE inline __forceinline
	/**
	 * Если компилятор принадлежит к семейству GCC или Clang
	 */
	#else
		/**
		 * Принудительная подстановка средствами GCC и Clang
		 */
		#define AWH_REGEX_INLINE inline __attribute__((always_inline))
	#endif
#endif

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <cstring>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../sys/global.hpp"

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
		 * @brief Наименьшее ожидаемое число ложных кандидатов, пробу окупающее
		 *
		 * @details Кандидат ложный есть положение, где пара байтов сошлась, а сличение
		 *          остатка искомого отказало. Проход парой умолчания платит за него
		 *          около полутора наносекунд, тогда как проба редкой пары стоит около
		 *          семидесяти наносекунд постоянных, - отсюда и величина: проба
		 *          окупается там, где кандидатов набирается несколько десятков.
		 *
		 *          Прежде величина эта означала **длину остатка** текста, и мера была
		 *          выбрана неверно в самом основании. Замером сличены оба прохода
		 *          на длинах от полукилобайта до шестидесяти четырёх килобайтов:
		 *
		 *          | текст | 2 КБ | 8 КБ | 64 КБ |
		 *          |---|---|---|---|
		 *          | протокола, кандидат один: умолчание к пробе | 0.34 | 0.38 | 0.38 |
		 *          | пару изматывающий: умолчание к пробе | 4.56 | 9.25 | 16.80 |
		 *
		 *          На тексте настоящем проба проигрывает втрое-вчетверо **на всякой
		 *          длине**, на вырожденном - выигрывает до семнадцати раз. Длина эти
		 *          случаи не различает вовсе: порог длины отдавал пробе всякий текст
		 *          подлиннее, кандидатов в нём не спрашивая, - оттого `literal-medium`
		 *          и отставал в 1.37 раза, тогда как проход парой умолчания шёл
		 *          у него 43.8 наносекунды против 63.9 у эталона.
		 *
		 *          Плотность кандидатов берётся окном, уже пройденным, отчего платы
		 *          за неё нет вовсе - см. `seek`.
		 *
		 * \~english
		 * @brief Smallest expected number of false candidates that pays the probe off
		 * @details A false candidate is a position where the pair of bytes matched while
		 *          the comparison of the remainder of the sought sequence failed. A pass by the default
		 *          pair pays about one and a half nanoseconds for it, whereas the probe of a rare pair
		 *          costs about seventy nanoseconds of constant cost - hence the value: the probe
		 *          pays off where the candidates amount to several dozens.
		 *
		 * \~
		 */
		constexpr size_t PAYOFF = 48;

		/**
		 * \~russian
		 * @brief Окно обычного поиска, якорному поиску предшествующее
		 *
		 * @details Окно решает, окупится ли выбор пары байтов: совпадение, в окне
		 *          лежащее, обычный поиск находит дешевле всякой пробы, а
		 *          совпадение за окном оправдывает и пробу, и якорный поиск
		 *          по остатку. Размер окна взят так, чтобы проход его целиком
		 *          не превышал платы за выбор пары: 1024 байта на 8,6 ГБ/с
		 *          обходятся в 120 наносекунд против 190 у выбора.
		 *
		 *          Совпадение, границу окна пересекающее, не теряется: окно
		 *          просматривается с придачей длины искомого без одного байта,
		 *          а якорный поиск начинается ровно за окном, отчего всякое
		 *          положение начала совпадения просматривается единожды.
		 *
		 * \~english
		 * @brief Window of the ordinary search preceding the anchored search
		 * @details The window decides whether selecting a byte pair pays off: a match lying
		 *          within the window is found by the ordinary search more cheaply than any probe,
		 *          while a match beyond the window justifies both the probe and the anchored
		 *          search over the remainder. The size of the window is taken so that walking it
		 *          entirely does not exceed the cost of selecting a pair: 1024 bytes at 8.6 GB/s
		 *          cost 120 nanoseconds against the 190 of the selection.
		 *
		 *          A match crossing the boundary of the window is not lost: the window is examined
		 *          with the length of the sought sequence less one byte added to it, while
		 *          the anchored search begins exactly past the window, which is why every
		 *          position where a match begins is examined once.
		 *
		 * \~
		 */
		constexpr size_t WINDOW = 1024;

		/**
		 * \~russian
		 * @brief Функция поиска последовательности в тексте по паре байтов
		 *
		 * @details Функция отделена от поиска общего и вынесена в отдельный файл
		 *          намеренно. Поиск общий стоит на пути горячем - он зовётся
		 *          и на текстах коротких, где проба не окупается вовсе, - и телом
		 *          своим обязан быть таким, чтобы встраивание его было
		 *          безоговорочным, тогда как тело поиска по паре крупно и несёт
		 *          команды над вектором, каким в заголовочном файле не место:
		 *          заголовок этот подключается всюду, а команды те нужны одному
		 *          лишь поиску.
		 *
		 * @param text текст сопоставления
		 * @param what искомая последовательность
		 * @param pos  позиция начала поиска
		 * @return     позиция найденной последовательности либо признак отсутствия
		 *
		 * \~english
		 * @brief Function of searching for a sequence in the text by a pair of bytes
		 * @details The function is separated from the general search and moved into a separate file
		 *          deliberately. The general search stands on the hot path — it is called
		 *          on short texts too, where the probe does not pay off at all — and its body
		 *          is obliged to be such that its inlining is
		 *          unconditional, whereas the body of the search by a pair is large and carries
		 *          vector instructions, which have no place in a header file:
		 *          that header is included everywhere, while those instructions are needed by the search
		 *          alone.
		 * @param text text to match
		 * @param what sought sequence
		 * @param pos  position to start the search from
		 * @return     position of the found sequence or the indication of its absence
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ size_t anchored(const string_view text, const string_view what, const size_t pos) noexcept;

		/**
		 * \~russian
		 * @brief Функция поиска последовательности в тексте по заданной паре байтов
		 *
		 * @param text   текст сопоставления
		 * @param what   искомая последовательность
		 * @param pos    позиция начала поиска
		 * @param first  смещение первого байта пары в искомой последовательности
		 * @param second смещение второго байта пары в искомой последовательности
		 * @return       позиция найденной последовательности либо признак отсутствия
		 *
		 * \~english
		 * @brief Function of searching for a sequence in the text by a given pair of bytes
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ size_t paired(const string_view text, const string_view what, const size_t pos, const size_t first, const size_t second) noexcept;

		/**
		 * \~russian
		 * @brief Функция поиска последовательности в окне по паре байтов умолчания
		 *
		 * @param text текст сопоставления
		 * @param what искомая последовательность
		 * @param pos  позиция начала поиска
		 * @return     позиция найденной последовательности либо признак отсутствия
		 *
		 * \~english
		 * @brief Function of searching for a sequence in the window by the default pair of bytes
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ size_t windowed(const string_view text, const string_view what, const size_t pos) noexcept;

		/**
		 * \~russian
		 * @brief Функция поиска последовательности в окне со счётом ложных кандидатов
		 *
		 * @details Кандидат ложный есть положение, где пара байтов сошлась, а сличение
		 *          остатка искомого отказало. Число их и решает, окупится ли проба
		 *          редкой пары: проба стоит около семидесяти наносекунд постоянных,
		 *          а кандидат ложный - около полутора, отчего проба окупается лишь
		 *          там, где кандидатов набирается несколько десятков.
		 *
		 * @param text     текст сопоставления
		 * @param what     искомая последовательность
		 * @param pos      позиция начала поиска
		 * @param rejected число ложных кандидатов, проходом отвергнутых
		 * @return         позиция найденной последовательности либо признак отсутствия
		 *
		 * \~english
		 * @brief Function of searching for a sequence in the window with a count of false candidates
		 * @param text     text to match
		 * @param what     sought sequence
		 * @param pos      position to start the search from
		 * @param rejected number of false candidates rejected by the pass
		 * @return         position of the found sequence or the indication of its absence
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ size_t windowed(const string_view text, const string_view what, const size_t pos, size_t & rejected) noexcept;

		/**
		 * \~russian
		 * @brief Наибольший набор начальных байтов, поиском отбираемый
		 *
		 * @details Отбор позиций начала попытки по набору допустимых байтов
		 *          ведётся двумя способами. Набор широкий просеивается
		 *          по позиции: обращение к таблице на каждой позиции дешевле
		 *          всякого поиска, а пропускает такой набор немногое. Набор
		 *          узкий, напротив, пропускает почти весь текст, и там
		 *          окупается поиск байтов набором команд процессора над
		 *          несколькими байтами сразу.
		 *
		 *          Предел взят по числу сравнений, какое вектор несёт за один
		 *          оборот: четыре байта дают четыре сравнения на шестнадцать
		 *          байтов текста против шестнадцати обращений к памяти
		 *          у просеивания. Наборы шире просеиваются, как и прежде.
		 *
		 * \~english
		 * @brief Largest set of starting bytes selected by a search
		 * @details The selection of the positions where an attempt begins by the set
		 *          of admissible bytes is driven in two ways. A wide set is sifted
		 *          position by position: an access to the table at every position is cheaper
		 *          than any search, and such a set skips little. A narrow set, on the contrary,
		 *          skips almost the whole text, and there a search of the bytes by the
		 *          instruction set of the processor over several bytes at once pays off.
		 *
		 *          The limit is taken by the number of comparisons a vector carries per
		 *          iteration: four bytes give four comparisons per sixteen bytes of the text
		 *          against the sixteen memory accesses of the sifting. Wider sets are sifted
		 *          as before.
		 *
		 * \~
		 */
		constexpr size_t SPARSE = 8;

		/**
		 * \~russian
		 * @brief Функция поиска первого байта из набора в тексте
		 *
		 * @details Функция отыскивает позицию первого байта текста,
		 *          набору принадлежащего, и служит отбору позиций начала
		 *          попытки при наборе узком. Набор задаётся перечислением
		 *          значений, а не таблицей: перечисление размножается
		 *          по вектору, тогда как таблица обращения к памяти требует.
		 *
		 * @param text  текст сопоставления
		 * @param bytes набор искомых значений байта
		 * @param count количество искомых значений байта
		 * @param pos   позиция начала поиска
		 * @return      позиция найденного байта либо размер текста
		 *
		 * \~english
		 * @brief Function of searching for the first byte of a set in the text
		 * @details The function finds the position of the first byte of the text belonging
		 *          to the set and serves the selection of the positions where an attempt
		 *          begins when the set is narrow. The set is given by an enumeration
		 *          of the values rather than by a table: the enumeration is spread over
		 *          a vector, whereas a table requires accesses to memory.
		 * @param text  text to match
		 * @param bytes set of the sought byte values
		 * @param count number of the sought byte values
		 * @param pos   position to start the search from
		 * @return      position of the found byte or the size of the text
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ size_t scattered(const string_view text, const uint8_t * bytes,
		 const size_t count, const size_t pos) noexcept;

		/**
		 * \~russian
		 * @brief Функция поиска последовательности в тексте
		 *
		 * @details Поиск последовательности стандартными средствами отыскивает
		 *          первый байт искомого, а затем сличает остаток. Байт первый
		 *          разборчивостью не отличается: в связном тексте он встречается
		 *          на каждом десятке байтов, отчего поиск обрывается непрестанно
		 *          и идёт со скоростью 8,6 ГБ/с при пределе набора команд
		 *          в шесть-восемь раз выше.
		 *
		 *          Разборчивость даёт пара байтов, в тексте редкая, - см. «anchored».
		 *          Здесь же остаётся отсев случаев, пробы не стоящих: искомое
		 *          пустое, искомое об одном байте, отыскиваемое поиском байта,
		 *          и остаток текста, пробы не окупающий.
		 *
		 * @param text текст сопоставления
		 * @param what искомая последовательность
		 * @param pos  позиция начала поиска
		 * @return     позиция найденной последовательности либо признак отсутствия
		 *
		 * \~english
		 * @brief Function of searching for a sequence in the text
		 * @details Searching for a sequence by the standard means locates
		 *          the first byte of the sought sequence and then compares the remainder. The first byte
		 *          is not distinctive: in connected text it occurs
		 *          at every ten bytes, which is why the search breaks off incessantly
		 *          and goes at a speed of 8.6 GB/s while the limit of the instruction set
		 *          is six to eight times higher.
		 *
		 *          Distinctiveness is given by a pair of bytes that is rare in the text — see «anchored».
		 *          What remains here is the sifting out of the cases not worth a probe: an empty
		 *          sought sequence, a sought sequence of one byte located by a byte search,
		 *          and a remainder of the text that does not pay the probe off.
		 * @param text text to match
		 * @param what sought sequence
		 * @param pos  position to start the search from
		 * @return     position of the found sequence or the indication of its absence
		 *
		 * \~
		 */
		AWH_REGEX_INLINE static size_t seek(string_view text, string_view what, const size_t pos) noexcept {
			/**
			 * Если проба текста не окупается
			 *
			 * @details Искомое пустое отыскивается в самой позиции поиска, искомое
			 *          об одном байте - поиском байта, набором команд процессора
			 *          выполняемым, а на остатке коротком проба дороже прохода.
			 *
			 */
			if((what.size() < 2) || (pos > text.size()))
				// Выводим результат поиска последовательности средствами обычными
				return text.find(what, pos);
			/**
			 * Получаем предел положения начала совпадения в окне поиска
			 *
			 * @details Совпадение обязано уместиться в текст целиком, отчего предел
			 *          берётся наименьшим из конца окна и последнего положения,
			 *          длине искомого отвечающего. Искомое, текста длиннее,
			 *          предел обнуляет: вычитание его длины переполнило бы величину
			 *          вниз, и сличение остатка вышло бы за пределы текста.
			 *
			 */
			const size_t reach = ((text.size() >= what.size()) ? ((text.size() - what.size()) + 1) : 0);
			// Получаем предел положения начала совпадения в окне поиска
			const size_t bound = (((pos + WINDOW) < reach) ? (pos + WINDOW) : reach);
			/**
			 * Выполняем поиск последовательности в окне по паре байтов
			 *
			 * @details Окно проходится отбором по паре байтов сразу, а не по байту
			 *          первому. Отбор по одному байту отвергает кандидата вызовом
			 *          поиска байта да сличением остатка, и замером кандидат этот
			 *          стоил 6.6 наносекунды: текст в 600 байтов с частым первым
			 *          байтом обходился в 101 наносекунду против 21 у того же текста
			 *          без кандидатов вовсе, а текст, первым байтом насыщенный, -
			 *          в 6592 наносекунды против 28 у эталона. Отбор же по паре
			 *          сличает участок текста с обоими байтами набором команд
			 *          процессора и остаток сличает лишь там, где сошлись оба.
			 *
			 *          Счёт отвергнутых отборов от того упразднён: он опознавал
			 *          вырождение отбора по одному байту, какого более нет,
			 *          а вырождение пары ограничено самою длиной окна.
			 *
			 *          Окно передаётся усечением текста, а не длиной: положения
			 *          выдаются от начала текста, и усечение их не двигает.
			 *
			 */
			// Число ложных кандидатов, окном отвергнутых
			size_t rejected = 0;
			// Выполняем поиск последовательности в окне по паре байтов
			const size_t found = windowed(text.substr(0, (((bound + what.size()) - 1) < text.size()) ?
			 ((bound + what.size()) - 1) : text.size()), what, pos, rejected);
			/**
			 * Если последовательность обнаружена в окне поиска
			 */
			if(found != string_view::npos)
				// Выводим положение обнаруженной последовательности
				return found;
			// Получаем размер остатка текста за окном поиска
			const size_t remainder = (text.size() - bound);
			/**
			 * Если остаток текста за окном пробу окупает
			 *
			 * @details Окупаемость пробы решается **плотностью ложных кандидатов**,
			 *          а не длиною остатка. Замером сличены оба прохода на текстах
			 *          длины от полукилобайта до шестидесяти четырёх килобайтов:
			 *          на тексте протокола, где пара умолчания даёт кандидата
			 *          одного, проба проигрывает втрое-вчетверо **на всякой длине**,
			 *          включая наибольшую; на тексте же, пару умолчания
			 *          изматывающем, она выигрывает от 2.4 раза на полукилобайте
			 *          до 16.8 на шестидесяти четырёх. Длина эти случаи
			 *          не различает вовсе, отчего порог длины и был негоден:
			 *          он отдавал пробе всякий текст подлиннее, кандидатов
			 *          в нём не спрашивая.
			 *
			 *          Плотность берётся окном, уже пройденным: кандидаты его
			 *          сочтены проходом, платы за счёт нет. Ожидаемое число
			 *          кандидатов остатка есть плотность окна, на остаток
			 *          умноженная, а окупается проба, когда число это
			 *          превышает `PAYOFF`: кандидат ложный стоит около полутора
			 *          наносекунд, проба - около семидесяти постоянных.
			 *
			 */
			if((rejected * remainder) > (PAYOFF * WINDOW))
				// Выводим результат поиска последовательности по якорному байту за окном
				return anchored(text, what, (pos + WINDOW));
			/**
			 * Выводим результат поиска остатка отбором по паре байтов
			 *
			 * @details Окно просмотрено целиком, отчего поиск продолжается за его
			 *          пределом: положения, окном пройденные, просмотрены единожды.
			 *
			 *          Остаток этот короче порога окупаемости пробы, но не короток
			 *          сам по себе: он доходит до целого порога, то есть до тысячи
			 *          байтов, и средствами обычными проходится сличением побайтным.
			 *          Замером лестницы по длине текста: 1554 байта обходились
			 *          в 134.9 наносекунды против 28.6 у 1042 байтов и 64.2
			 *          у 2066 - вчетверо по байту против соседей, - и весь этот
			 *          обрыв приходился на поиск остатка. Проба здесь по-прежнему
			 *          не заводится: заводится лишь проход парой байтов умолчания,
			 *          пробы не требующий.
			 *
			 */
			return windowed(text, what, bound);
		}


		/**
		 * \~russian
		 * @brief Предварительный отбор позиций сопоставления
		 *
		 * @details Отбор сокращает работу конечного автомата двумя способами. Набор
		 *          допустимых начальных байтов позволяет пропускать позиции, с которых
		 *          совпадение начаться не может. Обязательный литерал позволяет
		 *          отказаться от сопоставления целиком, если литерал в тексте
		 *          отсутствует. Оба способа дают надмножество возможных совпадений
		 *          и на результат сопоставления не влияют.
		 *
		 * \~english
		 * @brief Preliminary selection of matching positions
		 * @details The selection reduces the work of the finite automaton in two ways. The set of
		 *          admissible starting bytes allows skipping the positions at which
		 *          a match cannot begin. The mandatory literal allows
		 *          giving up matching entirely if the literal is absent
		 *          from the text. Both ways yield a superset of the possible matches
		 *          and do not affect the result of matching.
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Prefilter {
			// Флаг применимости набора допустимых начальных байтов
			bool active;
			// Флаг разбора текста как последовательности UTF-8
			bool utf;
			// Набор байтов, допустимых в начале совпадения
			bool bytes[256];
			// Литерал, присутствующий в любом совпадении выражения
			string literal;
			/**
			 * \~russian
			 * Наибольшее удаление обязательного литерала от начала совпадения
			 *
			 * @details Совпадение обязано литерал содержать, отчего начало его
			 *          не может лежать дальше чем за удаление до ближайшего
			 *          вхождения литерала: позиции до «вхождение минус удаление»
			 *          пропускаются разом, поиском последовательности взамен
			 *          перебора. Значение «string_view::npos» означает удаление
			 *          неограниченное - такое даёт всякий узел неограниченной
			 *          длины, литералу предшествующий, - и при нём литерал
			 *          служит лишь проверке возможности совпадения.
			 *
			 * \~english
			 * The greatest distance of the mandatory literal from the beginning of a match
			 * @details A match is bound to contain the literal, which is why its beginning
			 *          cannot lie further than the distance before the nearest
			 *          occurrence of the literal: the positions before "occurrence minus distance"
			 *          are skipped at once, by a sequence search instead of
			 *          a walk. The value "string_view::npos" means an unbounded
			 *          distance - such is given by any node of unbounded length
			 *          preceding the literal - and with it the literal
			 *          serves only the check of the possibility of a match.
			 *
			 * \~
			 */
			size_t distance;
			/**
			 * \~russian
			 * Признак единственного допустимого начального байта
			 *
			 * @details Единственный допустимый байт отыскивается поиском байта
			 *          в тексте, выполняемым набором команд процессора над
			 *          несколькими байтами сразу, тогда как набор допустимых
			 *          байтов требует перебора текста побайтно.
			 *
			 * \~english
			 * Indication of a single admissible starting byte
			 * @details A single admissible byte is looked up by a byte search
			 *          in the text performed by processor instructions over
			 *          several bytes at once, whereas a set of admissible
			 *          bytes requires walking the text byte by byte.
			 *
			 * \~
			 */
			bool unique;
			// Единственный допустимый начальный байт совпадения
			char letter;
			/**
			 * \~russian
			 * Последовательность символов, с которой начинается любое совпадение
			 *
			 * @details Ведущий литерал отыскивает позиции возможного начала совпадения
			 *          поиском последовательности в тексте, тогда как набор допустимых
			 *          байтов требует перебора текста побайтно. Поиск последовательности
			 *          пропускает участки текста целиком и потому предпочтителен.
			 *
			 * \~english
			 * The character sequence every match begins with
			 * @details The leading literal locates the positions of a possible beginning of a match
			 *          by searching for the sequence in the text, whereas the set of admissible
			 *          bytes requires walking the text byte by byte. Searching for a sequence
			 *          skips stretches of the text as a whole and is therefore preferable.
			 *
			 * \~
			 */
			string leading;
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
			Prefilter() noexcept : active(false), utf(false), bytes{}, distance(0), unique(false), letter(0) {}
			/**
			 * \~russian
			 * @brief Метод завершения формирования отбора позиций
			 *
			 * @details Признак единственного допустимого байта определяется по
			 *          набору допустимых байтов однажды при компиляции выражения,
			 *          поскольку перебор набора на каждом сопоставлении обошёлся бы
			 *          дороже самого отбора позиций.
			 *
			 * \~english
			 * @brief Method of finishing the building of the position selection
			 * @details The indication of a single admissible byte is determined from the
			 *          set of admissible bytes once when compiling the expression,
			 *          since walking the set on every match would cost
			 *          more than the position selection itself.
			 *
			 * \~
			 */
			void finalize() noexcept {
				// Количество допустимых начальных байтов совпадения
				size_t count = 0;
				/**
				 * Выполняем перебор набора допустимых начальных байтов
				 */
				for(size_t i = 0; i < 256; i++) {
					/**
					 * Если байт в начале совпадения недопустим
					 */
					if(!this->bytes[i])
						// Переходим к следующему байту набора
						continue;
					// Увеличиваем количество допустимых начальных байтов
					count++;
					/**
					 * Если допустимый байт обнаружен впервые
					 */
					if(count == 1)
						// Выполняем установку единственного допустимого байта
						this->letter = static_cast <char> (i);
				}
				// Выполняем установку признака единственного допустимого байта
				this->unique = (count == 1);
			}
			/**
			 * \~russian
			 * @brief Метод очистки предварительного отбора позиций
			 *
			 * \~english
			 * @brief Method of clearing the preliminary selection of positions
			 *
			 * \~
			 */
			void clear() noexcept {
				// Выполняем сброс флага применимости набора байтов
				this->active = false;
				// Выполняем сброс признака единственного допустимого байта
				this->unique = false;
				// Выполняем сброс единственного допустимого начального байта
				this->letter = 0;
				// Выполняем очистку ведущего литерала совпадения
				this->leading.clear();
				// Выполняем сброс флага разбора текста как последовательности UTF-8
				this->utf = false;
				// Выполняем очистку обязательного литерала совпадения
				this->literal.clear();
				// Выполняем сброс удаления обязательного литерала
				this->distance = 0;
				/**
				 * Выполняем очистку набора допустимых начальных байтов
				 */
				for(size_t i = 0; i < 256; i++)
					// Выполняем сброс допустимости очередного байта
					this->bytes[i] = false;
			}
			/**
			 * \~russian
			 * @brief Метод проверки возможности совпадения в оставшемся тексте
			 *
			 * @details Проверка выполняется по обязательному литералу совпадения.
			 *          Отсутствие литерала означает невозможность совпадения.
			 *
			 * @param text текст сопоставления
			 * @param pos  позиция начала проверяемого участка текста
			 * @return     результат проверки возможности совпадения
			 *
			 * \~english
			 * @brief Method of checking the possibility of a match in the remaining text
			 * @details The check is performed by the mandatory literal of a match.
			 *          The absence of the literal means that a match is impossible.
			 * @param text text to match
			 * @param pos  position of the beginning of the checked stretch of the text
			 * @return     result of checking the possibility of a match
			 *
			 * \~
			 */
			bool possible(string_view text, const size_t pos) const noexcept {
				/**
				 * Если обязательный литерал совпадения не определён
				 */
				if(this->literal.empty())
					// Выводим результат проверки возможности совпадения
					return true;
				// Выводим результат поиска обязательного литерала в тексте
				return (seek(text, this->literal, pos) != string_view::npos);
			}
			/**
			 * \~russian
			 * @brief Метод отбора позиции начала совпадения по обязательному литералу
			 *
			 * @details Совпадение обязано обязательный литерал содержать, и литерал
			 *          этот отстоит от начала совпадения не далее чем на удаление.
			 *          Отсюда: начало совпадения не может лежать раньше, чем
			 *          за удаление до ближайшего вхождения литерала, и позиции
			 *          до неё пропускаются разом - поиском последовательности
			 *          взамен перебора по одной.
			 *
			 *          Отсутствие литерала в оставшемся тексте означает отсутствие
			 *          совпадения, и выводится размер текста - тем же уговором,
			 *          какого держится поиск по набору допустимых байтов.
			 *
			 *          Отбор неприменим при удалении неограниченном и при литерале
			 *          пустом: в обоих случаях выводится сама позиция поиска,
			 *          отчего вызывающему проверять применимость не требуется.
			 *
			 * @param text текст сопоставления
			 * @param pos  позиция начала поиска
			 * @return     позиция возможного начала совпадения либо размер текста
			 *
			 * \~english
			 * @brief Method of selecting the position of the beginning of a match by the mandatory literal
			 * @details A match is bound to contain the mandatory literal, and that literal
			 *          stands no further from the beginning of the match than the distance.
			 *          Hence the beginning of a match cannot lie earlier than
			 *          the distance before the nearest occurrence of the literal, and the positions
			 *          before it are skipped at once - by a sequence search
			 *          instead of a walk one by one.
			 * @param text text to match
			 * @param pos  position to start the search from
			 * @return     position of a possible beginning of a match or the size of the text
			 *
			 * \~
			 */
			size_t bounded(string_view text, const size_t pos) const noexcept {
				/**
				 * Если отбор позиции по обязательному литералу неприменим
				 */
				if(this->literal.empty() || (this->distance == string_view::npos))
					// Выводим позицию начала поиска нетронутой
					return pos;
				// Выполняем поиск обязательного литерала в оставшемся тексте
				const size_t found = seek(text, this->literal, pos);
				/**
				 * Если обязательный литерал в оставшемся тексте отсутствует
				 */
				if(found == string_view::npos)
					// Выводим размер текста признаком отсутствия совпадения
					return text.size();
				// Получаем позицию, раньше какой совпадение начаться не может
				const size_t bound = ((found > this->distance) ? (found - this->distance) : 0);
				// Выводим позицию возможного начала совпадения
				return ((bound > pos) ? bound : pos);
			}
			/**
			 * \~russian
			 * @brief Метод поиска ближайшей позиции возможного начала совпадения
			 *
			 * @details Поиск выполняется по набору допустимых начальных байтов.
			 *          В режиме разбора UTF-8 продолжающие байты последовательности
			 *          пропускаются, поскольку совпадение начинается с границы символа.
			 *
			 * @param text текст сопоставления
			 * @param pos  позиция начала поиска
			 * @return     позиция возможного начала совпадения либо признак отсутствия
			 *
			 * \~english
			 * @brief Method of searching for the nearest position of a possible beginning of a match
			 * @details The search is performed by the set of admissible starting bytes.
			 *          In the UTF-8 parsing mode the continuation bytes of a sequence
			 *          are skipped, since a match begins at a character boundary.
			 * @param text text to match
			 * @param pos  position to start the search from
			 * @return     position of a possible beginning of a match or the indication of its absence
			 *
			 * \~
			 */
			size_t search(string_view text, const size_t pos) const noexcept {
				/**
				 * Если набор допустимых начальных байтов неприменим
				 */
				if(!this->active)
					// Выводим переданную позицию начала поиска
					return pos;
				// Получаем размер текста сопоставления
				const size_t size = text.size();
				/**
				 * \~russian
				 * Если ведущий литерал совпадения определён
				 *
				 * @details Поиск последовательности пропускает участки текста целиком,
				 *          тогда как перебор допустимых байтов проходит его побайтно.
				 *
				 * \~english
				 * If the leading literal of a match is defined
				 * @details Searching for a sequence skips stretches of the text as a whole,
				 *          whereas walking the admissible bytes goes through it byte by byte.
				 *
				 * \~
				 */
				if(this->leading.size() > 1) {
					// Выполняем поиск ведущего литерала совпадения в тексте
					const size_t result = seek(text, this->leading, pos);
					// Выводим позицию найденного литерала либо конец текста
					return ((result == string_view::npos) ? size : result);
				}
				/**
				 * \~russian
				 * Если допустимый начальный байт единственный
				 *
				 * @details Поиск одиночного байта выполняется набором команд
				 *          процессора над несколькими байтами сразу и проходит
				 *          текст многократно быстрее перебора его побайтно.
				 *
				 * \~english
				 * If the admissible starting byte is a single one
				 * @details Searching for a single byte is performed by processor
				 *          instructions over several bytes at once and goes through
				 *          the text many times faster than walking it byte by byte.
				 *
				 * \~
				 */
				if(this->unique) {
					// Выполняем поиск единственного допустимого байта в тексте
					const size_t result = text.find(this->letter, pos);
					// Выводим позицию найденного байта либо конец текста
					return ((result == string_view::npos) ? size : result);
				}
				/**
				 * Выполняем поиск позиции возможного начала совпадения
				 */
				for(size_t i = pos; i < size; i++) {
					// Получаем очередной байт текста сопоставления
					const uint8_t letter = static_cast <uint8_t> (text[i]);
					/**
					 * Если байт недопустим в начале совпадения
					 */
					if(!this->bytes[letter])
						// Переходим к следующему байту текста
						continue;
					/**
					 * Если байт является продолжающим байтом последовательности UTF-8
					 */
					if(this->utf && ((letter & 0xC0) == 0x80))
						// Переходим к следующему байту текста
						continue;
					// Выводим позицию возможного начала совпадения
					return i;
				}
				// Выводим позицию конца текста сопоставления
				return size;
			}
		} prefilter_t;
	};
};

#endif // __AWH_REGEX_PREFILTER__
