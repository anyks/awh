/**
 * @file: prefilter.hpp
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
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_REGEX_PREFILTER__
#define __AWH_REGEX_PREFILTER__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
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
				return (text.find(this->literal, pos) != string_view::npos);
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
					const size_t result = text.find(this->leading, pos);
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
