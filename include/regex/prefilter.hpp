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
		 * @brief Наименьший остаток текста, при каком проба окупается
		 *
		 * @details Проба обходится в две сотни наносекунд - выбор пары байтов
		 *          перебирает пары искомого счётом по пробе текста, - тогда как
		 *          проход текста поиском обычным идёт на 8,6 ГБ/с. Тексты короче
		 *          порога проходят прежним путём в точности, отчего сценарии
		 *          коротких текстов правкой этой не затронуты вовсе.
		 *
		 *          Довод прежний называл пробу платой в десятки наносекунд
		 *          и окупаемость её - двумя-тремя сотнями байтов остатка. Замер
		 *          довод этот отверг: выбор пары стоит 190 наносекунд, а
		 *          окупаемость решает не длина текста, а **расстояние
		 *          до совпадения**. Литерал, лежащий на сороковом байте, обычный
		 *          поиск находит за 4,6 наносекунды при любой длине текста,
		 *          и проба там дороже в сорок раз. Оттого длина остатка порогом
		 *          осталась лишь для отсечения текстов коротких, а расстояние
		 *          до совпадения выясняется окном - см. WINDOW ниже.
		 *
		 * \~english
		 * @brief Smallest remainder of the text at which the probe pays off
		 * @details The probe costs two hundred nanoseconds — the selection of a byte pair
		 *          enumerates the pairs of the sought sequence by counting them over a probe of the text —
		 *          whereas walking the text with the ordinary search goes at 8.6 GB/s. Texts shorter
		 *          than the threshold go the former way exactly, which is why the scenarios
		 *          of short texts are not affected by this change at all.
		 *
		 *          The former reasoning called the probe a cost of tens of nanoseconds and its
		 *          payoff two or three hundred bytes of the remainder. Measurement rejected that
		 *          reasoning: selecting a pair costs 190 nanoseconds, and the payoff is decided
		 *          not by the length of the text but by the **distance to the match**.
		 *          A literal lying at the fortieth byte is found by the ordinary search
		 *          in 4.6 nanoseconds at any length of the text, and the probe is forty times
		 *          dearer there. Hence the length of the remainder remained a threshold only
		 *          for cutting off short texts, while the distance to the match is found out
		 *          by a window — see WINDOW below.
		 *
		 * \~
		 */
		constexpr size_t PAYOFF = 4096;

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
			if((what.size() < 2) || (pos > text.size()) || ((text.size() - pos) < PAYOFF))
				// Выводим результат поиска последовательности средствами обычными
				return text.find(what, pos);
			// Выполняем поиск последовательности в окне средствами обычными
			const size_t found = text.substr(pos, (WINDOW + what.size() - 1)).find(what);
			/**
			 * Если последовательность обнаружена в окне поиска
			 */
			if(found != string_view::npos)
				// Выводим положение обнаруженной последовательности
				return (pos + found);
			// Выводим результат поиска последовательности по якорному байту за окном
			return anchored(text, what, (pos + WINDOW));
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
			Prefilter() noexcept : active(false), utf(false), bytes{}, unique(false), letter(0) {}
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
