/**
 * @file: chrono.cpp
 * @date: 2025-10-25
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация модуля работы с датой и временем — разбор и форматирование дат в различных форматах,
 *        конвертация единиц времени, работа с временными зонами и переходами на летнее время,
 *        получение штампов времени высокого разрешения
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cmath>
#include <ctime>
#include <chrono>
#include <cstdarg>
#include <iostream>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/log.hpp>
#include <sys/chrono.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Инкапсулируем статические параметры в пространство имён
 *
 */
namespace {
	/**
	 * @brief Структура справочных таблиц для календарных вычислений
	 *
	 * @details Структура содержит справочные таблицы, необходимые для выполнения различных календарных вычислений,
	 *          таких как определение дня недели, количества дней в каждом месяце, а также
	 *          сокращённые и полные названия дней недели и месяцев.
	 *
	 */
	struct Params {
		// Коды (сдвиги) месяцев для расчёта дня недели
		vector <uint8_t> rateMonths;
		// Количество дней в каждом месяце для невисокосного года
		vector <uint8_t> daysInMonths;
		// Сокращённые и полные названия дней недели (Mon/Monday ...)
		vector <std::pair <string, string>> nameDays;
		// Сокращённые и полные названия месяцев (Jan/January ...)
		vector <std::pair <string, string>> nameMonths;
		// Коды годов в 4-летнем цикле для расчёта дня недели
		unordered_map <uint16_t, uint8_t> rateLeapYears;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Params() noexcept :
		 rateMonths({
			6,2,2,5,0,3,
			5,1,4,6,2,4
		 }),
		 daysInMonths({
			31,28,31,30,31,30,
			31,31,30,31,30,31
		 }),
		 nameDays({
			{"Mon", "Monday"},
			{"Tue", "Tuesday"},
			{"Wed", "Wednesday"},
			{"Thu", "Thursday"},
			{"Fri", "Friday"},
			{"Sat", "Saturday"},
			{"Sun", "Sunday"}
		 }),
		 nameMonths({
			{"Jan", "January"},
			{"Feb", "February"},
			{"Mar", "March"},
			{"Apr", "April"},
			{"May", "May"},
			{"Jun", "June"},
			{"Jul", "July"},
			{"Aug", "August"},
			{"Sep", "September"},
			{"Oct", "October"},
			{"Nov", "November"},
			{"Dec", "December"}
		 }),
		 rateLeapYears({
			{0,6},{1,2},{2,5},
			{3,1},{4,4},{5,0},{6,3}
		 }) {}
	} params;
};

/**
 * @brief Инкапсулируем статические функции в пространство имён
 *
 */
namespace {
	/**
	 * @brief Структура одной группы совпадения (полуинтервал [begin, end) в тексте)
	 *
	 * @details Структура содержит два поля: begin - индекс первого символа группы (относительно начала анализируемого текста),
	 *          end - индекс символа сразу за концом группы (относительно начала анализируемого текста).
	 *          Если группа не найдена, оба поля устанавливаются в -1.
	 *
	 */
	struct match_t {
		// Индекс символа сразу за концом группы (относительно начала анализируемого текста)
		ssize_t end;
		// Индекс первого символа группы (относительно начала анализируемого текста)
		ssize_t begin;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit match_t() noexcept : end(-1), begin(-1) {}
	};

	/**
	 * @brief Функция проверки, является ли символ десятичной цифрой ('0'–'9')
	 *
	 * @param letter проверяемый символ
	 * @return       true если символ является цифрой
	 *
	 */
	inline bool isDigitChar(const char letter) noexcept {
		// Цифра попадает в диапазон от '0' до '9' таблицы ASCII
		return ((letter >= '0') && (letter <= '9'));
	}
	/**
	 * @brief Функция проверки, является ли символ заглавной латинской буквой ('A'–'Z')
	 *
	 * @param letter проверяемый символ
	 * @return       true если символ является заглавной буквой
	 *
	 */
	inline bool isUpperChar(const char letter) noexcept {
		// Заглавная латинская буква попадает в диапазон от 'A' до 'Z'
		return ((letter >= 'A') && (letter <= 'Z'));
	}
	/**
	 * @brief Функция проверки, является ли символ строчной латинской буквой ('a'–'z')
	 *
	 * @param letter проверяемый символ
	 * @return       true если символ является строчной буквой
	 *
	 */
	inline bool isLowerChar(const char letter) noexcept {
		// Строчная латинская буква попадает в диапазон от 'a' до 'z'
		return ((letter >= 'a') && (letter <= 'z'));
	}
	/**
	 * @brief Функция проверки, является ли символ латинской буквой (заглавной или строчной)
	 *
	 * @param letter проверяемый символ
	 * @return       true если символ является буквой
	 *
	 */
	inline bool isAlphaChar(const char letter) noexcept {
		// Буквой считается как заглавная, так и строчная латинская буква
		return (isUpperChar(letter) || isLowerChar(letter));
	}
	/**
	 * @brief Функция проверки, является ли символ словесным (буква, цифра или '_')
	 *
	 * @param letter проверяемый символ
	 * @return       true если символ является словесным
	 *
	 */
	inline bool isWordChar(const char letter) noexcept {
		// Словесный символ — это буква, цифра или знак подчёркивания
		return (isAlphaChar(letter) || isDigitChar(letter) || (letter == '_'));
	}
	/**
	 * @brief Функция проверки, является ли символ пробельным
	 *
	 * @param letter проверяемый символ
	 * @return       true если символ является пробельным
	 *
	 */
	inline bool isSpaceChar(const char letter) noexcept {
		// Пробельными считаются пробел, табуляция, перевод строки, возврат каретки и перевод страницы
		return (
			(letter == ' ') || (letter == '\t') ||
			(letter == '\n') || (letter == '\r') ||
			(letter == '\f') || (letter == '\v')
		);
	}
	/**
	 * @brief Функция жадного захвата серии цифр в позиции pos
	 *
	 * @param text   анализируемый текст
	 * @param length длина анализируемого текста
	 * @param pos    позиция в тексте
	 * @param min    минимальное количество цифр
	 * @param max    максимальное количество цифр
	 * @return       количество захваченных цифр или 0 если их меньше min
	 *
	 */
	inline size_t takeDigits(const char * text, const size_t length, const size_t pos, const size_t min, const size_t max) noexcept {
		// Счётчик подряд идущих цифр, захваченных начиная с позиции pos
		size_t count = 0;
		/**
		 * Двигаемся вперёд, пока не выйдем за конец текста, не упрёмся в предел max или не встретим нецифровой символ
		 */
		while(((pos + count) < length) && (count < max) && isDigitChar(text[pos + count]))
			// Очередной символ — цифра, расширяем захваченную серию
			count++;
		// Серия короче требуемого минимума считается несовпадением и даёт 0
		return ((count >= min) ? count : 0);
	}
	/**
	 * @brief Функция проверки совпадения серии цифр \d{min,max} в позиции pos с заполнением группы
	 *
	 * @param text   анализируемый текст
	 * @param length длина анализируемого текста
	 * @param pos    позиция в тексте
	 * @param min    минимальное количество цифр
	 * @param max    максимальное количество цифр
	 * @param match  группа совпадения
	 * @return       результат проверки совпадения
	 *
	 */
	inline bool matchDigits(const char * text, const size_t length, size_t & pos, const size_t min, const size_t max, match_t & match) noexcept {
		// Пробуем захватить от min до max цифр начиная с текущей позиции
		const size_t digits = takeDigits(text, length, pos, min, max);
		// Нужного количества цифр в этой позиции нет — совпадения не будет
		if(digits == 0)
			// Выводим отрицательный результат проверки совпадения
			return false;
		// Начало группы — текущая позиция курсора
		match.begin = static_cast <ssize_t> (pos);
		// Конец группы — позиция сразу за последней захваченной цифрой
		match.end = static_cast <ssize_t> (pos + digits);
		// Продвигаем курсор за конец захваченной серии цифр
		pos += digits;
		// Серия цифр успешно разобрана
		return true;
	}
	/**
	 * @brief Функция проверки совпадения одного литерала в позиции pos с заполнением группы
	 *
	 * @param text   анализируемый текст
	 * @param length длина анализируемого текста
	 * @param pos    позиция в тексте
	 * @param letter символ для совпадения
	 * @param match  группа совпадения
	 * @return       результат проверки совпадения
	 *
	 */
	inline bool matchLiteral(const char * text, const size_t length, size_t & pos, const char letter) noexcept {
		// За концом текста или при несовпадении символа разделитель не найден
		if((pos >= length) || (text[pos] != letter))
			// Выводим отрицательный результат проверки совпадения
			return false;
		// Символ совпал — перешагиваем через него
		pos++;
		// Литерал успешно разобран
		return true;
	}
	/**
	 * @brief Функция проверки совпадения серии пробельных символов \s+ в позиции pos с заполнением группы
	 *
	 * @param text   анализируемый текст
	 * @param length длина анализируемого текста
	 * @param pos    позиция в тексте
	 * @return       результат проверки совпадения
	 *
	 */
	inline bool matchSpaces(const char * text, const size_t length, size_t & pos) noexcept {
		// Требуется хотя бы один пробельный символ — иначе совпадения нет
		if((pos >= length) || !isSpaceChar(text[pos]))
			// Выводим отрицательный результат проверки совпадения
			return false;
		/**
		 * Поглощаем подряд идущую серию пробельных символов
		 */
		while((pos < length) && isSpaceChar(text[pos]))
			// Перешагиваем через очередной пробельный символ
			pos++;
		// Серия пробелов успешно пропущена
		return true;
	}
	/**
	 * @brief Функция проверки совпадения шаблона [A-Z][a-z]{min,max} в позиции pos с заполнением группы
	 *
	 * @param text   анализируемый текст
	 * @param length длина анализируемого текста
	 * @param pos    позиция в тексте
	 * @param min    минимальное количество строчных букв
	 * @param max    максимальное количество строчных букв
	 * @param match  группа совпадения
	 * @return       результат проверки совпадения
	 *
	 */
	inline bool matchName(const char * text, const size_t length, size_t & pos, const size_t min, const size_t max, match_t & match) noexcept {
		// Название обязано начинаться с заглавной буквы (например, Jan или Monday)
		if((pos >= length) || !isUpperChar(text[pos]))
			// Выводим отрицательный результат проверки совпадения
			return false;
		// Счётчик строчных букв, идущих после заглавной
		size_t count = 0;
		/**
		 * Жадно набираем строчные буквы вслед за заглавной, не превышая предел max
		 */
		while(((pos + 1 + count) < length) && (count < max) && isLowerChar(text[pos + 1 + count]))
			// Очередная строчная буква входит в название
			count++;
		// Строчных букв меньше требуемого минимума — это не название нужной длины
		if(count < min)
			// Выводим отрицательный результат проверки совпадения
			return false;
		// Начало группы — позиция заглавной буквы
		match.begin = static_cast <ssize_t> (pos);
		// Конец группы — позиция за последней строчной буквой (заглавная + count строчных)
		match.end = static_cast <ssize_t> (pos + 1 + count);
		// Продвигаем курсор за всё разобранное название
		pos += (1 + count);
		// Название успешно разобрано
		return true;
	}
	/**
	 * @brief Функция проверки совпадения шаблона [A-Za-z]{2} в позиции pos с заполнением группы
	 *
	 * @param text   анализируемый текст
	 * @param length длина анализируемого текста
	 * @param pos    позиция в тексте
	 * @param match  группа совпадения
	 * @return       результат проверки совпадения
	 *
	 */
	inline bool matchAlpha2(const char * text, const size_t length, size_t & pos, match_t & match) noexcept {
		// Нужны ровно две буквы подряд (например, метка AM/PM) и место под них в тексте
		if(((pos + 2) > length) || !isAlphaChar(text[pos]) || !isAlphaChar(text[pos + 1]))
			// Выводим отрицательный результат проверки совпадения
			return false;
		// Начало группы — позиция первой буквы
		match.begin = static_cast <ssize_t> (pos);
		// Конец группы — позиция сразу за второй буквой
		match.end = static_cast <ssize_t> (pos + 2);
		// Продвигаем курсор за обе разобранные буквы
		pos += 2;
		// Пара букв успешно разобрана
		return true;
	}
	/**
	 * @brief Парсер серии цифр \d{min,max} (поиск первого совпадения)
	 *
	 * @param text   анализируемый текст
	 * @param length длина анализируемого текста
	 * @param min    минимальное количество цифр
	 * @param max    максимальное количество цифр
	 * @return       список групп совпадения (пустой если совпадения нет)
	 *
	 */
	vector <match_t> parseDigits(const char * text, const size_t length, const size_t min, const size_t max) noexcept {
		// Итоговый список групп: остаётся пустым, если совпадение не найдено
		vector <match_t> result;
		/**
		 * Сканируем текст слева направо в поисках первой подходящей серии цифр
		 */
		for(size_t i = 0; i < length; i++){
			// Серия может начинаться только с цифры — остальные позиции пропускаем
			if(!isDigitChar(text[i]))
				// Переходим к следующей позиции текста
				continue;
			// Курсор для пробного разбора, начинающийся с текущей позиции
			size_t pos = i;
			// Группа, которую заполнит matchDigits в случае успеха
			match_t match{};
			// Пробуем захватить серию цифр требуемой длины
			if(matchDigits(text, length, pos, min, max, match)){
				// Найдено первое совпадение — отдаём его единственной группой
				result.resize(1);
				// Группа совпадения уже заполнена функцией matchDigits, просто копируем её в результат
				result[0] = match;
				// Выводим группу с позицией найденной серии цифр в тексте
				return result;
			}
		}
		// Подходящей серии цифр в тексте не нашлось
		return result;
	}
	/**
	 * @brief Функция парсинга слова \w+ (поиск первого совпадения)
	 *
	 * @param text   анализируемый текст
	 * @param length длина анализируемого текста
	 * @return       список групп совпадения (пустой если совпадения нет)
	 *
	 */
	vector <match_t> parseWord(const char * text, const size_t length) noexcept {
		// Итоговый список групп: остаётся пустым, если слово не найдено
		vector <match_t> result;
		/**
		 * Сканируем текст слева направо в поисках первого словесного символа
		 */
		for(size_t i = 0; i < length; i++){
			// Слово может начинаться только со словесного символа — остальные пропускаем
			if(!isWordChar(text[i]))
				// Переходим к следующей позиции текста
				continue;
			// Курсор, которым поглотим всё слово, начиная с его первого символа
			size_t pos = i;
			/**
			 * Расширяем слово, пока подряд идут словесные символы
			 */
			while((pos < length) && isWordChar(text[pos]))
				// Включаем очередной словесный символ в слово
				pos++;
			// Возвращаем единственную группу с границами найденного слова
			result.resize(1);
			// Начало слова — позиция его первого символа
			result[0].begin = static_cast <ssize_t> (i);
			// Конец слова — позиция сразу за последним словесным символом
			result[0].end = static_cast <ssize_t> (pos);
			return result;
		}
		// Словесных символов в тексте не нашлось
		return result;
	}
	/**
	 * @brief Функция парсера шаблона [A-Za-z]{2} (поиск первого совпадения)
	 *
	 * @param text   анализируемый текст
	 * @param length длина анализируемого текста
	 * @return       список групп совпадения (пустой если совпадения нет)
	 *
	 */
	vector <match_t> parseAlpha2(const char * text, const size_t length) noexcept {
		// Итоговый список групп: остаётся пустым, если пара букв не найдена
		vector <match_t> result;
		/**
		 * Пробуем найти пару букв, начиная поочерёдно с каждой позиции текста
		 */
		for(size_t i = 0; i < length; i++){
			// Курсор для пробного разбора, начинающийся с текущей позиции
			size_t pos = i;
			// Группа, которую заполнит matchAlpha2 в случае успеха
			match_t match{};
			// Пробуем захватить две буквы подряд
			if(matchAlpha2(text, length, pos, match)){
				// Найдено первое совпадение — отдаём его единственной группой
				result.resize(1);
				// Группа совпадения уже заполнена функцией matchAlpha2, просто копируем её в результат
				result[0] = match;
				// Выводим группу с позицией найденной пары букв в тексте
				return result;
			}
		}
		// Пары букв в тексте не нашлось
		return result;
	}
	/**
	 * @brief Функция парсера шаблона [A-Z][a-z]{min,max} (поиск первого совпадения)
	 *
	 * @param text   анализируемый текст
	 * @param length длина анализируемого текста
	 * @param min    минимальное количество строчных букв
	 * @param max    максимальное количество строчных букв
	 * @return       список групп совпадения (пустой если совпадения нет)
	 *
	 */
	vector <match_t> parseName(const char * text, const size_t length, const size_t min, const size_t max) noexcept {
		// Итоговый список групп: остаётся пустым, если название не найдено
		vector <match_t> result;
		/**
		 * Пробуем разобрать название, начиная поочерёдно с каждой позиции текста
		 */
		for(size_t i = 0; i < length; i++){
			// Курсор для пробного разбора, начинающийся с текущей позиции
			size_t pos = i;
			// Группа, которую заполнит matchName в случае успеха
			match_t match{};
			// Пробуем разобрать название вида [A-Z][a-z]{min,max}
			if(matchName(text, length, pos, min, max, match)){
				// Найдено первое совпадение — отдаём его единственной группой
				result.resize(1);
				// Группа совпадения уже заполнена функцией matchName, просто копируем её в результат
				result[0] = match;
				// Выводим группу с позицией найденного названия в тексте
				return result;
			}
		}
		// Подходящего названия в тексте не нашлось
		return result;
	}
	/**
	 * @brief Функция парсера последовательности цифровых групп с разделителем
	 *
	 * @param text   анализируемый текст
	 * @param length длина анализируемого текста
	 * @param sep    символ разделителя между группами
	 * @param specs  список диапазонов [min,max] для каждой группы
	 * @return       список групп совпадения (группа 0 - всё совпадение)
	 *
	 */
	vector <match_t> parseDigitGroups(const char * text, const size_t length, const char sep, const vector <pair <size_t, size_t>> & specs) noexcept {
		// Итоговый список групп: остаётся пустым, если последовательность не найдена
		vector <match_t> result;
		/**
		 * Сканируем текст в поисках начала последовательности (первой цифры)
		 */
		for(size_t i = 0; i < length; i++){
			// Последовательность начинается с цифры — остальные позиции пропускаем
			if(!isDigitChar(text[i]))
				// Переходим к следующей позиции текста
				continue;
			// Курсор для пробного разбора, начинающийся с текущей позиции
			size_t pos = i;
			// Признак того, что все группы и разделители разобраны без ошибок
			bool ok = true;
			// Группы совпадения: индекс 0 зарезервирован под всё совпадение, далее по группе на каждый spec
			vector <match_t> groups(specs.size() + 1);
			/**
			 * Последовательно разбираем каждую цифровую группу из specs
			 */
			for(size_t j = 0; j < specs.size(); j++){
				// Группа не набрала нужное число цифр — разбор последовательности сорван
				if(!matchDigits(text, length, pos, specs[j].first, specs[j].second, groups[j + 1])){
					// Разбор очередной группы не удался — выставляем признак ошибки
					ok = false;
					// Выходим из цикла, так как дальнейший разбор уже не имеет смысла
					break;
				}
				// Между группами (но не после последней) обязателен разделитель
				if((j + 1) < specs.size()){
					// Ожидаемого разделителя нет — разбор последовательности сорван
					if(!matchLiteral(text, length, pos, sep)){
						// Разделитель не найден — выставляем признак ошибки
						ok = false;
						// Выходим из цикла, так как дальнейший разбор уже не имеет смысла
						break;
					}
				}
			}
			// Все группы и разделители разобраны успешно
			if(ok){
				/**
				 * Группа 0 охватывает всю разобранную последовательность
				 */
				// Начало группы 0 — позиция первой цифры в последовательности
				groups[0].begin = static_cast <ssize_t> (i);
				// Конец группы 0 — позиция сразу за последним разобранным символом
				groups[0].end = static_cast <ssize_t> (pos);
				// Выводим группы с позициями разобранных чисел в тексте
				return groups;
			}
		}
		// Подходящей последовательности групп в тексте не нашлось
		return result;
	}
	/**
	 * @brief Функция парсера шаблона (\d{1,2}):(\d{1,2}):(\d{1,2})\s+([A-Za-z]{2}) (формат %r)
	 *
	 * @param text   анализируемый текст
	 * @param length длина анализируемого текста
	 * @return       список групп совпадения (группа 0 - всё совпадение)
	 *
	 */
	vector <match_t> parseTimeMeridiem(const char * text, const size_t length) noexcept {
		// Итоговый список групп: остаётся пустым, если время с меткой не найдено
		vector <match_t> result;
		/**
		 * Время начинается с цифры часов — ищем её, сканируя текст
		 */
		for(size_t i = 0; i < length; i++){
			// Кандидат на начало времени — только цифра, остальные позиции пропускаем
			if(!isDigitChar(text[i]))
				// Переходим к следующей позиции текста
				continue;
			// Курсор для пробного разбора, начинающийся с текущей позиции
			size_t pos = i;
			// Группы: [0] — всё совпадение, [1..3] — часы/минуты/секунды, [4] — метка AM/PM
			vector <match_t> groups(5);
			// Разбираем «ЧЧ:ММ:СС <метка>» одной цепочкой: любое звено может оборвать совпадение
			if(matchDigits(text, length, pos, 1, 2, groups[1]) &&
			   matchLiteral(text, length, pos, ':') &&
			   matchDigits(text, length, pos, 1, 2, groups[2]) &&
			   matchLiteral(text, length, pos, ':') &&
			   matchDigits(text, length, pos, 1, 2, groups[3]) &&
			   matchSpaces(text, length, pos) &&
			   matchAlpha2(text, length, pos, groups[4])){
				/**
				 * Группа 0 охватывает всё разобранное время вместе с меткой
				 */
				// Начало группы 0 — позиция первой цифры в времени
				groups[0].begin = static_cast <ssize_t> (i);
				// Конец группы 0 — позиция сразу за последним разобранным символом
				groups[0].end = static_cast <ssize_t> (pos);
				// Выводим группы с позициями разобранного времени в тексте
				return groups;
			}
		}
		// Времени с меткой суток в тексте не нашлось
		return result;
	}
	/**
	 * @brief Функция парсера шаблона ([A-Z][a-z]{2})\s+([A-Z][a-z]{2})\s+(\d{1,2})\s+(\d{1,2}):(\d{1,2}):(\d{1,2})\s+(\d{4}) (формат %c)
	 *
	 * @param text   анализируемый текст
	 * @param length длина анализируемого текста
	 * @return       список групп совпадения (группа 0 - всё совпадение)
	 *
	 */
	vector <match_t> parseAsctime(const char * text, const size_t length) noexcept {
		// Итоговый список групп: остаётся пустым, если дата asctime не найдена
		vector <match_t> result;
		/**
		 * Дата asctime начинается с названия дня недели (с заглавной буквы) — ищем её
		 */
		for(size_t i = 0; i < length; i++){
			// Кандидат на начало даты — только заглавная буква, остальные позиции пропускаем
			if(!isUpperChar(text[i]))
				// Переходим к следующей позиции текста
				continue;
			// Курсор для пробного разбора, начинающийся с текущей позиции
			size_t pos = i;
			// Группы: [0] — всё совпадение, далее день недели, месяц, число, ЧЧ, ММ, СС и год
			vector <match_t> groups(8);
			// Разбираем «Ddd Mmm DD HH:MM:SS YYYY» одной цепочкой: сбой любого звена прерывает разбор
			if(matchName(text, length, pos, 2, 2, groups[1]) &&
			   matchSpaces(text, length, pos) &&
			   matchName(text, length, pos, 2, 2, groups[2]) &&
			   matchSpaces(text, length, pos) &&
			   matchDigits(text, length, pos, 1, 2, groups[3]) &&
			   matchSpaces(text, length, pos) &&
			   matchDigits(text, length, pos, 1, 2, groups[4]) &&
			   matchLiteral(text, length, pos, ':') &&
			   matchDigits(text, length, pos, 1, 2, groups[5]) &&
			   matchLiteral(text, length, pos, ':') &&
			   matchDigits(text, length, pos, 1, 2, groups[6]) &&
			   matchSpaces(text, length, pos) &&
			   matchDigits(text, length, pos, 4, 4, groups[7])){
				/**
				 * Группа 0 охватывает всю разобранную дату asctime
				 */
				// Начало группы 0 — позиция заглавной буквы дня недели
				groups[0].begin = static_cast <ssize_t> (i);
				// Конец группы 0 — позиция сразу за последним разобранным символом
				groups[0].end = static_cast <ssize_t> (pos);
				// Выводим группы с позициями разобранной даты asctime в тексте
				return groups;
			}
		}
		// Даты в формате asctime в тексте не нашлось
		return result;
	}
	/**
	 * @brief Функция парсера шаблона (\+|\-)((\d{1,2}):(\d{1,2})|\d{1,4}) (формат %z)
	 *
	 * @param text   анализируемый текст
	 * @param length длина анализируемого текста
	 * @return       список групп совпадения (группа 0 - всё совпадение, группа 1 - знак, группа 2 - блок смещения, группы 3 и 4 - часы и минуты при формате с двоеточием, группа 5 - цифровое смещение при формате без двоеточия)
	 *
	 */
	vector <match_t> parseZoneOffset(const char * text, const size_t length) noexcept {
		// Итоговый список групп: остаётся пустым, если смещение зоны не найдено
		vector <match_t> result;
		/**
		 * Смещение начинается со знака — ищем '+' или '-', сканируя текст
		 */
		for(size_t i = 0; i < length; i++){
			// Кандидат на начало смещения — только знак, остальные позиции пропускаем
			if((text[i] != '+') && (text[i] != '-'))
				// Переходим к следующей позиции текста
				continue;
			// Группы: [0] — всё совпадение, [1] — знак, [2] — блок смещения, [3]/[4] — часы/минуты, [5] — слитное смещение
			vector <match_t> groups(6);
			/**
			 * Группа знака занимает один символ — сам '+' или '-'
			 */
			// Начало группы знака — позиция этого символа
			groups[1].begin = static_cast <ssize_t> (i);
			// Конец группы знака — позиция сразу за ним
			groups[1].end = static_cast <ssize_t> (i + 1);
			// Значение смещения начинается сразу за знаком
			const size_t branchStart = (i + 1);
			// Позиция конца всего совпадения (уточнится после разбора значения)
			size_t pos = branchStart;
			// Отдельный курсор для пробы формата с двоеточием, чтобы не сдвигать pos при неудаче
			size_t begin = branchStart;
			// Часы и минуты для формата HH:MM
			match_t hh{}, mm{};
			// Сначала пробуем формат с двоеточием HH:MM
			if(matchDigits(text, length, begin, 1, 2, hh) &&
			   matchLiteral(text, length, begin, ':') &&
			   matchDigits(text, length, begin, 1, 2, mm)){
				// Сохраняем разобранные часы и минуты в их группы
				groups[3] = hh;
				groups[4] = mm;
				/**
				 * Блок смещения охватывает разобранное значение HH:MM
				 */
				// Начало блока смещения — позиция сразу за знаком
				groups[2].begin = static_cast <ssize_t> (branchStart);
				// Конец блока смещения — позиция сразу за разобранными часами и минутами
				groups[2].end = static_cast <ssize_t> (begin);
				// Двигаем конец совпадения за разобранное значение
				pos = begin;
			// Двоеточия нет — пробуем слитное цифровое смещение \d{1,4}
			} else {
				// Группа для слитного цифрового смещения
				match_t dd{};
				// Отдельный курсор для пробы слитного формата
				size_t begin = branchStart;
				// Пробуем захватить от 1 до 4 цифр смещения
				if(matchDigits(text, length, begin, 1, 4, dd)){
					// Сохраняем слитное смещение в его группу
					groups[5] = dd;
					/**
					 * Блок смещения охватывает разобранные цифры
					 */
					// Начало блока смещения — позиция сразу за знаком
					groups[2].begin = static_cast <ssize_t> (branchStart);
					// Конец блока смещения — позиция сразу за разобранными цифрами
					groups[2].end = static_cast <ssize_t> (begin);
					// Двигаем конец совпадения за разобранное значение
					pos = begin;
				// За знаком не оказалось значения смещения — это не зона, ищем дальше
				} else continue;
			}
			/**
			 * Группа 0 охватывает знак вместе со значением смещения
			 */
			// Начало группы 0 — позиция знака
			groups[0].begin = static_cast <ssize_t> (i);
			// Конец группы 0 — позиция сразу за последним разобранным символом
			groups[0].end = static_cast <ssize_t> (pos);
			// Выводим группы с позициями разобранного смещения в тексте
			return groups;
		}
		// Смещения временной зоны в тексте не нашлось
		return result;
	}
	/**
	 * @brief Функция парсера шаблона (\w+)?((\+|\-)((\d{1,2}):(\d{1,2})|\d{1,4}))? (формат %e)
	 *
	 * @param text   анализируемый текст
	 * @param length длина анализируемого текста
	 * @return       список групп совпадения (группа 0 - всё совпадение, группа 1 - слово, группа 2 - блок смещения, группа 3 - знак, группа 4 - значение смещения, группы 5 и 6 - часы и минуты при формате с двоеточием)
	 *
	 */
	vector <match_t> parseZoneFull(const char * text, const size_t length) noexcept {
		// Итоговый список групп: остаётся пустым, если ни слова, ни смещения не нашлось
		vector <match_t> result;
		/**
		 * Оба элемента шаблона необязательны, поэтому ищем первое непустое совпадение
		 */
		for(size_t i = 0; i < length; i++){
			// Курсор разбора, начинающийся с текущей позиции
			size_t pos = i;
			// Группы: [0] всё, [1] слово, [2] блок смещения, [3] знак, [4] значение, [5]/[6] часы/минуты
			vector <match_t> groups(7);
			// Необязательная часть (\w+)?: название зоны словом
			if(isWordChar(text[pos])){
				// Курсор для поглощения всего слова
				size_t offset = pos;
				/**
				 * Расширяем слово, пока подряд идут словесные символы
				 */
				while((offset < length) && isWordChar(text[offset]))
					// Включаем очередной словесный символ в слово
					offset++;
				/**
				 * Группа слова охватывает захваченные словесные символы
				 */
				// Начало группы слова — позиция первого словесного символа
				groups[1].begin = static_cast <ssize_t> (pos);
				// Конец группы слова — позиция сразу за последним словесным символом
				groups[1].end = static_cast <ssize_t> (offset);
				// Сдвигаем основной курсор за слово
				pos = offset;
			}
			// Необязательный блок смещения ((\+|\-)(...))?: начинается со знака
			if((pos < length) && ((text[pos] == '+') || (text[pos] == '-'))){
				// off — диапазон значения смещения, hh/mm — часы и минуты
				match_t off{}, hh{}, mm{};
				// Признак того, что значение смещения за знаком успешно разобрано
				bool inner = false;
				// Позиция самого знака смещения
				const size_t blockStart = pos;
				// Конец блока смещения (уточнится после разбора значения)
				size_t offset = (pos + 1);
				/**
				 * Сначала пробуем формат с двоеточием (\d{1,2}):(\d{1,2})
				 */
				// Часы и минуты для формата HH:MM
				match_t h2{}, m2{};
				// Отдельный курсор после знака, чтобы не портить offset при неудаче
				size_t begin = (pos + 1);
				// Проверяем формат HH:MM
				if(matchDigits(text, length, begin, 1, 2, h2) &&
				   matchLiteral(text, length, begin, ':') &&
				   matchDigits(text, length, begin, 1, 2, m2)){
					// Переносим разобранные часы и минуты в выходные группы
					hh = h2;
					mm = m2;
					/**
					 * Диапазон значения смещения — от позиции за знаком до конца HH:MM
					 */
					// Начало диапазона — позиция сразу за знаком
					off.begin = static_cast <ssize_t> (pos + 1);
					// Конец диапазона — позиция сразу за разобранными часами и минутами
					off.end = static_cast <ssize_t> (begin);
					// Двигаем конец блока за разобранное значение
					offset = begin;
					// Значение смещения разобрано
					inner = true;
				// Двоеточия нет — пробуем слитное цифровое смещение \d{1,4}
				} else {
					// Группа для слитного цифрового смещения
					match_t d2{};
					// Отдельный курсор после знака для пробы слитного формата
					size_t begin = (pos + 1);
					// Пробуем захватить от 1 до 4 цифр смещения
					if(matchDigits(text, length, begin, 1, 4, d2)){
						/**
						 * Диапазон значения смещения — захваченные цифры за знаком
						 */
						// Начало диапазона — позиция сразу за знаком
						off.begin = static_cast <ssize_t> (pos + 1);
						// Конец диапазона — позиция сразу за разобранными цифрами
						off.end = static_cast <ssize_t> (begin);
						// Двигаем конец блока за разобранное значение
						offset = begin;
						// Значение смещения разобрано
						inner = true;
					}
				}
				// Значение за знаком разобрано — фиксируем блок смещения целиком
				if(inner){
					/**
					 * Группа знака занимает один символ — сам '+' или '-'
					 */
					// Начало группы знака — позиция этого символа
					groups[3].begin = static_cast <ssize_t> (blockStart);
					// Конец группы знака — позиция сразу за ним
					groups[3].end = static_cast <ssize_t> (blockStart + 1);
					// Сохраняем диапазон значения и разобранные часы/минуты
					groups[4] = off;
					groups[5] = hh;
					groups[6] = mm;
					/**
					 * Блок смещения охватывает знак вместе со значением
					 */
					// Начало блока смещения — позиция знака
					groups[2].begin = static_cast <ssize_t> (blockStart);
					// Конец блока смещения — позиция сразу за разобранным значением
					groups[2].end = static_cast <ssize_t> (offset);
					// Сдвигаем основной курсор за блок смещения
					pos = offset;
				}
			}
			// Принимаем совпадение, только если хоть что-то разобрано (аналог REG_NOTEMPTY)
			if(pos > i){
				/**
				 * Группа 0 охватывает всё разобранное (слово и/или блок смещения)
				 */
				// Начало группы 0 — позиция начала разбора
				groups[0].begin = static_cast <ssize_t> (i);
				// Конец группы 0 — позиция сразу за последним разобранным символом
				groups[0].end = static_cast <ssize_t> (pos);
				// Выводим группы с позициями разобранного слова и/или смещения в тексте
				return groups;
			}
		}
		// Ни слова, ни смещения зоны в тексте не нашлось
		return result;
	}
	/**
	 * @brief Функция парсера шаблона ([\d\.\,]+)\s*(s|m|h|d|w|M|y)$ (формат %S размерности времени)
	 *
	 * @param text   анализируемый текст
	 * @param length длина анализируемого текста
	 * @return       список групп совпадения (группа 0 - всё совпадение, группа 1 - число, группа 2 - единица размерности)
	 *
	 */
	vector <match_t> parseSeconds(const char * text, const size_t length) noexcept {
		// Итоговый список групп: остаётся пустым, если число с единицей не найдено
		vector <match_t> result;
		/**
		 * Число начинается с цифры, точки или запятой — ищем такую позицию
		 */
		for(size_t i = 0; i < length; i++){
			// Число может начинаться только с цифры, точки или запятой — иначе пропускаем
			if(!(isDigitChar(text[i]) || (text[i] == '.') || (text[i] == ',')))
				// Переходим к следующей позиции текста
				continue;
			// pos1 — курсор конца числа (поглощает серию [\d.,]+)
			size_t pos1 = i;
			/**
			 * Расширяем число, пока подряд идут цифры, точки или запятые
			 */
			while((pos1 < length) && (isDigitChar(text[pos1]) || (text[pos1] == '.') || (text[pos1] == ',')))
				// Включаем очередной символ числа
				pos1++;
			// pos2 — курсор поиска единицы; пропускаем необязательные пробелы \s* после числа
			size_t pos2 = pos1;
			/**
			 * Поглощаем пробелы между числом и единицей размерности
			 */
			while((pos2 < length) && isSpaceChar(text[pos2]))
				// Перешагиваем через очередной пробел
				pos2++;
			// После числа и пробелов не осталось символа единицы — совпадение неполное
			if(pos2 >= length)
				// Переходим к следующей позиции текста
				continue;
			// Символ-кандидат на единицу размерности
			const char letter = text[pos2];
			// Допустимы только s, m, h, d, w, M, y — иначе это не наш формат
			if((letter != 's') && (letter != 'm') && (letter != 'h') && (letter != 'd') && (letter != 'w') && (letter != 'M') && (letter != 'y'))
				// Символ единицы размерности не соответствует допустимым — переходим к следующей позиции текста
				continue;
			// Единица обязана быть последним символом строки (имитация якоря $)
			if((pos2 + 1) != length)
				// После единицы есть ещё символы — это не совпадение по формату, переходим к следующей позиции текста
				continue;
			// Группы: [0] — всё совпадение, [1] — число, [2] — единица размерности
			vector <match_t> groups(3);
			/**
			 * Группа 0 охватывает число вместе с единицей (включая пробелы между ними)
			 */
			// Начало группы 0 — позиция первой цифры или точки в числе
			groups[0].begin = static_cast <ssize_t> (i);
			// Конец группы 0 — позиция сразу за единицей размерности
			groups[0].end = static_cast <ssize_t> (pos2 + 1);
			/**
			 * Группа числа — от начала до конца серии [\d.,]+
			 */
			// Начало группы числа — позиция первой цифры или точки в числе
			groups[1].begin = static_cast <ssize_t> (i);
			// Конец группы числа — позиция сразу за последним символом числа
			groups[1].end = static_cast <ssize_t> (pos1);
			/**
			 * Группа единицы — единственный символ размерности
			 */
			// Начало группы единицы — позиция этого символа
			groups[2].begin = static_cast <ssize_t> (pos2);
			// Конец группы единицы — позиция сразу за символом размерности
			groups[2].end = static_cast <ssize_t> (pos2 + 1);
			// Выводим группы с позициями найденного числа и его единицы размерности в тексте
			return groups;
		}
		// Числа с единицей размерности в тексте не нашлось
		return result;
	}
};

/**
 * @brief Конструктор
 *
 */
awh::Chrono::DateTime::DateTime() noexcept :
 dst(false), leap(false),
 h12(h12_t::AM), zone(zone_t::NONE),
 day(2), date(1), hour(0), month(1),
 weeks(0), seconds(0), minutes(0),
 year(1970), days(0), offset(0),
 milliseconds(0), microseconds(0), nanoseconds(0) {}

/**
 * @brief Метод очистки всех локальных данных
 *
 */
void awh::Chrono::clear() noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Устанавливаем временную зону по умолчанию
			::_tzset();
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#else
			// Устанавливаем временную зону по умолчанию
			::tzset();
		#endif
		// Выполняем очистку списка временных зон
		this->clearTimeZones();
		// Выполняем блокировку потока
		const locker_t <> lock(this->_mtx.date);
		// Выполняем сброс локального объекта даты и времени
		this->_dt = dt_t();
		// Получаем текущий штамп времени
		const auto now = chrono::system_clock::now();
		// Получаем штамп времени в наносекундах
		const chrono::nanoseconds nanoseconds = chrono::duration_cast <chrono::nanoseconds> (now.time_since_epoch());
		// Получаем штамп времени в миллисекундах
		const chrono::milliseconds milliseconds = chrono::duration_cast <chrono::milliseconds> (now.time_since_epoch());
		// Получаем штамп времени в микросекундах
		const chrono::microseconds microseconds = chrono::duration_cast <chrono::microseconds> (now.time_since_epoch());
		// Устанавливаем количество микросекунд
		this->_dt.microseconds = (microseconds.count() % 1000);
		// Устанавливаем количество наносекунд
		this->_dt.nanoseconds = (nanoseconds.count() % 1000000);
		// Заполняем объект даты из штампа времени в миллисекундах
		this->makeDate(static_cast <uint64_t> (milliseconds.count()), this->_dt);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки безопасности работы потоков
 *
 * @param mode флаг режима безопасности потоков
 *
 */
void awh::Chrono::threadSafety(const bool mode) noexcept {
	/** 
	 * Активируем или деактивируем мьютексы в зависимости от переданного флага
	 */
	this->_mtx.tz.enabled   = mode;
	this->_mtx.date.enabled = mode;
}
/**
 * @brief Метод подсчёта количества десятичных разрядов числа
 *
 * @param value число для которого выполняется подсчёт разрядов
 * @return      количество десятичных разрядов
 *
 */
uint8_t awh::Chrono::digits(const uint64_t value) const noexcept {
	// Количество десятичных разрядов
	uint8_t result = 0;
	// Текущее значение для подсчёта разрядов
	uint64_t number = value;
	/**
	 * Подсчитываем количество разрядов
	 */
	do {
		// Увеличиваем количество разрядов
		result++;
		// Уменьшаем число на один разряд
		number /= 10;
	/**
	 * Продолжаем пока число не закончится
	 */
	} while(number > 0);
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод подсчёта количества високосных лет, прошедших с 1970 года
 *
 * @param years количество прошедших лет с 1970 года
 * @return      количество високосных лет с учётом григорианского календаря
 *
 */
uint16_t awh::Chrono::leapYears(const uint16_t years) const noexcept {
	// Получаем последний полный год перед началом искомого года
	const uint32_t last = (1970 + static_cast <uint32_t> (years) - 1);
	// Количество високосных лет от Рождества Христова до 1969 года включительно
	constexpr uint32_t base = ((1969 / 4) - (1969 / 100) + (1969 / 400));
	// Возвращаем количество високосных лет, прошедших с 1970 года
	return static_cast <uint16_t> (((last / 4) - (last / 100) + (last / 400)) - base);
}
/**
 * @brief Метод получения штампа времени начала указанного года в миллисекундах
 *
 * @param year год для которого необходимо получить начало
 * @return     штамп времени начала года в миллисекундах
 *
 */
uint64_t awh::Chrono::beginOfYear(const uint16_t year) const noexcept {
	// Определяем количество прошедших лет
	const uint16_t lastYears = (year > 1970 ? (year - 1970) : 0);
	// Определяем количество прошедших високосных лет
	const uint16_t leapCount = (lastYears > 0 ? this->leapYears(lastYears) : 0);
	// Возвращаем штамп времени начала года
	return (
		(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
		(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
	);
}
/**
 * @brief Метод проверки действует ли летнее время (DST) по правилам США/Канады
 *
 * @param month номер месяца (1-12)
 * @param date  число месяца (1-31)
 * @param day   день недели (1 - понедельник, 7 - воскресенье)
 * @param hour  количество часов (0-23)
 * @return      результат проверки действия летнего времени
 *
 */
bool awh::Chrono::isDST(const uint8_t month, const uint8_t date, const uint8_t day, const uint8_t hour) const noexcept {
	// До марта и после ноября летнее время не действует
	if((month < 3) || (month > 11))
		// Возвращаем отсутствие летнего времени
		return false;
	// С апреля по октябрь летнее время действует всегда
	if((month > 3) && (month < 11))
		// Возвращаем наличие летнего времени
		return true;
	// Получаем день недели первого числа месяца (0 - воскресенье, 6 - суббота)
	const int8_t weekday = static_cast <int8_t> (day % 7);
	// Вычисляем день недели первого числа месяца относительно текущего числа
	const int8_t firstDow = static_cast <int8_t> (((((weekday - static_cast <int8_t> ((date - 1) % 7)) % 7) + 7) % 7));
	// Получаем число первого воскресенья месяца
	const uint8_t firstSunday = static_cast <uint8_t> (((7 - firstDow) % 7) + 1);
	// Если на дворе март, летнее время начинается со 2-го воскресенья в 02:00
	if(month == 3){
		// Получаем число второго воскресенья марта
		const uint8_t secondSunday = (firstSunday + 7);
		// Если текущее число не совпадает с днём перехода
		if(date != secondSunday)
			// Летнее время действует после дня перехода
			return (date > secondSunday);
		// В день перехода летнее время наступает с 02:00
		return (hour >= 2);
	}
	/**
	 * Иначе на дворе ноябрь, летнее время заканчивается в 1-е воскресенье в 02:00
	 * Если текущее число не совпадает с днём перехода
	 */
	if(date != firstSunday)
		// Летнее время действует до дня перехода
		return (date < firstSunday);
	// В день перехода летнее время действует до 02:00
	return (hour < 2);
}
/**
 * @brief Метод получения штампа времени из объекта даты
 *
 * @param dt объект даты из которой необходимо получить штамп времени
 * @return   штамп времени в миллисекундах
 *
 */
uint64_t awh::Chrono::makeDate(const dt_t & dt) const noexcept {
	// Переменная результата
	uint64_t result = 0;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Определяем количество прошедших лет
		const uint16_t lastYears = (dt.year > 0 ? (dt.year - 1970) : 0);
		// Определяем количество прошедших високосных лет
		const uint16_t leapCount = (lastYears > 0 ? this->leapYears(lastYears) : 0);
		// Получаем штамп времени начала года
		result = (
			(static_cast <uint64_t> (leapCount) * static_cast <uint64_t> (31622400000)) +
			(static_cast <uint64_t> (lastYears - leapCount) * static_cast <uint64_t> (31536000000))
		);
		/**
		 * Выполняем подсчёт количества дней
		 */
		for(uint8_t i = 0; (i < params.daysInMonths.size()) && (i < (dt.month - 1)); i++){
			// Если месяц февраль и год високосный
			if((i == 1) && dt.leap)
				// Увеличиваем результат на один день
				result += static_cast <uint64_t> (86400000);
			// Увеличиваем смещение времени до указанного месяца
			result += (static_cast <uint64_t> (params.daysInMonths[i]) * static_cast <uint64_t> (86400000));
		}
		// Получаем число месяца с компенсацией нулевого значения
		const uint8_t date = (dt.date == 0 ? 1 : dt.date);
		// Увеличиваем на количество прошедших дней
		result += (static_cast <uint64_t> (date - 1) * static_cast <uint64_t> (86400000));
		// Увеличиваем на количество часов
		result += (static_cast <uint64_t> (dt.hour) * static_cast <uint64_t> (3600000));
		// Увеличиваем на указанное смещение времени
		result += static_cast <uint64_t> (dt.offset * 1000);
		// Увеличиваем на количество минут
		result += (static_cast <uint64_t> (dt.minutes) * static_cast <uint64_t> (60000));
		// Увеличиваем на количество секунд
		result += (static_cast <uint64_t> (dt.seconds) * static_cast <uint64_t> (1000));
		// Увеличиваем на количество миллисекунд
		result += static_cast <uint64_t> (dt.milliseconds);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
		// Выполняем сброс результата
		result = 0;
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод заполнения объекта даты из штампа времени
 *
 * @param date дата из которой необходимо заполнить объект
 * @param dt   объект даты который необходимо заполнить
 *
 */
void awh::Chrono::makeDate(const uint64_t date, dt_t & dt) const noexcept {
	// Если дата передана
	if(date > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем установку текущее значение года
			dt.year = this->year(date);
			// Устанавливаем флаг високосного года
			dt.leap = this->leap(dt.year);
			// Получаем штамп времени начала года
			const uint64_t beginYear = this->beginOfYear(dt.year);
			// Начало месяца и начало суток
			uint64_t beginMonth = 0, beginDay = 0;
			// Определяем сколько дней прошло с начала года
			dt.days = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
			{
				// Подсчитываем количество дней в предыдущих месяцах
				uint16_t count = 0, days = 0;
				/**
				 * Выполняем перебор всех дней месяца
				 */
				for(uint8_t i = 0; i < params.daysInMonths.size(); i++){
					// Увеличиваем номер месяца
					dt.month = (i + 1);
					// Получаем текущее количество дней с компенсацией високосного года
					days = (static_cast <uint16_t> (params.daysInMonths[i]) + ((i == 1) && dt.leap ? 1 : 0));
					// Если мы не дошли до предела
					if(dt.days >= (days + count))
						// Увеличиваем количество прошедших дней
						count += days;
					// Выходим из цикла
					else break;
				}
				// Устанавливаем текущее значение даты
				dt.date = static_cast <uint8_t> ((dt.days - count) + 1);
				// Получаем начало месяца указанной даты
				beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
				// Получаем начало суток указанной даты
				beginDay = (beginMonth + (static_cast <uint64_t> (dt.date - 1) * static_cast <uint64_t> (86400000)));
				// Получаем множитель текущего года
				auto i = params.rateLeapYears.find(static_cast <uint16_t> ((dt.year - (dt.year % 4)) % 7));
				// Если множитель получен
				if(i != params.rateLeapYears.end()){
					// Подробнее: https://habr.com/ru/articles/217389
					// Устанавливаем день недели
					dt.day = (((i->second + static_cast <uint8_t> (dt.year % 4) + params.rateMonths[dt.month - 1] + dt.date) - (((dt.month == 1) || (dt.month == 2)) && dt.leap ? 1 : 0)) % 7);
					// Если воскресенье установлен как нулевой
					if(dt.day == 0)
						// Выполняем компенсацию
						dt.day = 7;
				}
				// Получаем количество недель прошедших с начала года
				dt.weeks = static_cast <uint8_t> (::round((date - beginYear) / 604800000.L));
			}
			// Получаем количество миллисекунд
			dt.milliseconds = static_cast <uint32_t> (date % 1000);
			// Получаем количество часов
			dt.hour = static_cast <uint8_t> (::floor((date - beginDay) / 3600000.L));
			// Получаем количество минут
			dt.minutes = static_cast <uint8_t> (::floor(((date - beginDay) % 3600000) / 60000.));
			// Получаем количество секунд
			dt.seconds = static_cast <uint8_t> (::floor((((date - beginDay) % 3600000) % 60000) / 1000.));
			// Если время утреннее
			if(dt.hour < 12)
				// Устанавливаем статус времени до полудня
				dt.h12 = h12_t::AM;
			// Устанавливаем время после полудня
			else dt.h12 = h12_t::PM;
			// Устанавливаем флаг летнего времени (DST) по правилам США/Канады
			dt.dst = this->isDST(dt.month, dt.date, dt.day, dt.hour);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(date), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	// Выполняем сброс значения даты
	} else dt = dt_t();
}
/**
 * @brief Функция заполнения объекта даты и времени
 *
 * @param dt     объект даты и времени для заполнения
 * @param text   текст в котором производится поиск
 * @param format формат выполнения поиска
 * @param pos    начальная позиция в тексте
 * @return       конечная позиция обработанных данных в тексте
 *
 */
ssize_t awh::Chrono::prepare(dt_t & dt, string_view text, const format_t format, const size_t pos) const noexcept {
	// Переменная результата
	ssize_t result = -1;
	// Если данные переданы
	if(!text.empty() && (pos < text.size()) && (format != format_t::NONE)){
		// Запоминаем оригинальное значение формата
		const format_t fmt = format;
		/**
		 * Определяем нужный нам формат
		 */
		switch(static_cast <uint8_t> (format)){
			// Если формат получен как %w
			case static_cast <uint8_t> (format_t::w):
				// Выполняем подмену формата
				const_cast <format_t &> (format) = format_t::u;
			break;
			// Если формат получен как %W
			case static_cast <uint8_t> (format_t::W):
				// Выполняем подмену формата
				const_cast <format_t &> (format) = format_t::s;
			break;
			// Если формат получен как %H
			case static_cast <uint8_t> (format_t::H):
			// Если формат получен как %I
			case static_cast <uint8_t> (format_t::I):
			// Если формат получен как %M
			case static_cast <uint8_t> (format_t::M):
			// Если формат получен как %s
			case static_cast <uint8_t> (format_t::S):
			// Если формат получен как %m
			case static_cast <uint8_t> (format_t::m):
			// Если формат получен как %d
			case static_cast <uint8_t> (format_t::d):
				// Выполняем подмену формата
				const_cast <format_t &> (format) = format_t::y;
			break;
			// Если формат получен как %b
			case static_cast <uint8_t> (format_t::b):
				// Выполняем подмену формата
				const_cast <format_t &> (format) = format_t::a;
			break;
			// Если формат получен как %B
			case static_cast <uint8_t> (format_t::B):
				// Выполняем подмену формата
				const_cast <format_t &> (format) = format_t::A;
			break;
		}
		// Указатель на начало анализируемого текста
		const char * src = (text.data() + pos);
		// Длина анализируемого текста
		const size_t len = (text.size() - pos);
		// Список групп совпадения
		vector <match_t> match;
		/**
		 * Выбираем нативный парсер в зависимости от формата
		 */
		switch(static_cast <uint8_t> (format)){
			// Если формат соответствует %u (\d{1})
			case static_cast <uint8_t> (format_t::u): match = ::parseDigits(src, len, 1, 1); break;
			// Если формат соответствует %s (\d+)
			case static_cast <uint8_t> (format_t::s): match = ::parseDigits(src, len, 1, static_cast <size_t> (-1)); break;
			// Если формат соответствует %j (\d{3})
			case static_cast <uint8_t> (format_t::j): match = ::parseDigits(src, len, 3, 3); break;
			// Если формат соответствует %Y (\d{4})
			case static_cast <uint8_t> (format_t::Y): match = ::parseDigits(src, len, 4, 4); break;
			// Если формат соответствует %y (\d{1,2})
			case static_cast <uint8_t> (format_t::y): match = ::parseDigits(src, len, 1, 2); break;
			// Если формат соответствует %p ([A-Za-z]{2})
			case static_cast <uint8_t> (format_t::p): match = ::parseAlpha2(src, len); break;
			// Если формат соответствует %a ([A-Z][a-z]{2})
			case static_cast <uint8_t> (format_t::a): match = ::parseName(src, len, 2, 2); break;
			// Если формат соответствует %A ([A-Z][a-z]{2,})
			case static_cast <uint8_t> (format_t::A): match = ::parseName(src, len, 2, static_cast <size_t> (-1)); break;
			// Если формат соответствует %Z (\w+)
			case static_cast <uint8_t> (format_t::Z): match = ::parseWord(src, len); break;
			// Если формат соответствует %R ((\d{1,2}):(\d{1,2}))
			case static_cast <uint8_t> (format_t::R): match = ::parseDigitGroups(src, len, ':', {{1, 2}, {1, 2}}); break;
			// Если формат соответствует %T ((\d{1,2}):(\d{1,2}):(\d{1,2}))
			case static_cast <uint8_t> (format_t::T): match = ::parseDigitGroups(src, len, ':', {{1, 2}, {1, 2}, {1, 2}}); break;
			// Если формат соответствует %D ((\d{1,2})/(\d{1,2})/(\d{2}))
			case static_cast <uint8_t> (format_t::D): match = ::parseDigitGroups(src, len, '/', {{1, 2}, {1, 2}, {2, 2}}); break;
			// Если формат соответствует %F ((\d{4})-(\d{1,2})-(\d{1,2}))
			case static_cast <uint8_t> (format_t::F): match = ::parseDigitGroups(src, len, '-', {{4, 4}, {1, 2}, {1, 2}}); break;
			// Если формат соответствует %r ((\d{1,2}):(\d{1,2}):(\d{1,2})\s+([A-Za-z]{2}))
			case static_cast <uint8_t> (format_t::r): match = ::parseTimeMeridiem(src, len); break;
			// Если формат соответствует %c (asctime)
			case static_cast <uint8_t> (format_t::c): match = ::parseAsctime(src, len); break;
			// Если формат соответствует %z ((\+|\-)((\d{1,2}):(\d{1,2})|(\d{1,4})))
			case static_cast <uint8_t> (format_t::z): match = ::parseZoneOffset(src, len); break;
		}
		// Если совпадение получено
		if(!match.empty()){
			// Обрабатываем полученные группы совпадения
			{
				/**
				 * Определяем нужный нам формат
				 */
				switch(static_cast <uint8_t> (format)){
					// Если формат получен как %u
					case static_cast <uint8_t> (format_t::u):
					// Если формат получен как %j
					case static_cast <uint8_t> (format_t::j):
					// Если формат получен как %s
					case static_cast <uint8_t> (format_t::s):
					// Если формат получен как %y
					case static_cast <uint8_t> (format_t::y):
					// Если формат получен как %Y
					case static_cast <uint8_t> (format_t::Y):
					// Если формат получен как %p
					case static_cast <uint8_t> (format_t::p):
					// Если формат получен как %a
					case static_cast <uint8_t> (format_t::a):
					// Если формат получен как %A
					case static_cast <uint8_t> (format_t::A):
					// Если формат получен как %Z
					case static_cast <uint8_t> (format_t::Z):
					// Если формат получен как %W
					case static_cast <uint8_t> (format_t::W): {
						/**
						 * Выполняем перебор всех полученных вариантов
						 */
						for(uint8_t j = 0; j < static_cast <uint8_t> (match.size()); j++){
							// Если результат получен
							if(match[j].end > match[j].begin){
								// Получаем смещение в тексте
								result = static_cast <ssize_t> (pos + match[j].end);
								/**
								 * Определяем тип входящих данных
								 */
								switch(static_cast <uint8_t> (fmt)){
									// Если мы определяем номер дня недели %w
									case static_cast <uint8_t> (format_t::w): {
										// Устанавливаем номер дня недели
										dt.day = this->_fmk->atoi <uint8_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
										// Если день установлен как нулевой
										if(dt.day == 0)
											// Устанавливаем номер дня недели
											dt.day = 7;
									} break;
									// Если мы определяем номер недели в году %W
									case static_cast <uint8_t> (format_t::W):
										// Устанавливаем количество недель прошедших с начала года
										dt.weeks = this->_fmk->atoi <uint8_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
									break;
									// Если мы определяем порядковый номер дня в году %j
									case static_cast <uint8_t> (format_t::j):
										// Устанавливаем порядковый номер дня в году
										dt.days = (this->_fmk->atoi <uint16_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin) - 1);
									break;
									// Если мы определяем номер дня недели %u
									case static_cast <uint8_t> (format_t::u):
										// Устанавливаем номер дня недели
										dt.day = this->_fmk->atoi <uint8_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
									break;
									// Если формат получен как %y
									case static_cast <uint8_t> (format_t::y): {
										// Получаем значение указанного года
										const uint16_t num = this->_fmk->atoi <uint16_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
										// Устанавливаем год
										dt.year = (2000 + num);
										// Устанавливаем флаг високосного года
										dt.leap = this->leap(dt.year);
									} break;
									// Если формат получен как %Y
									case static_cast <uint8_t> (format_t::Y): {
										// Устанавливаем год
										dt.year = this->_fmk->atoi <uint16_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
										// Устанавливаем флаг високосного года
										dt.leap = this->leap(dt.year);
									} break;
									// Если формат получен как %d
									case static_cast <uint8_t> (format_t::d):
										// Устанавливаем число месяца
										dt.date = this->_fmk->atoi <uint8_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
									break;
									// Если формат получен как %m
									case static_cast <uint8_t> (format_t::m):
										// Получаем значение номера месяца
										dt.month = this->_fmk->atoi <uint8_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
									break;
									// Если формат получен как %I
									case static_cast <uint8_t> (format_t::I):
										// Устанавливаем полученный час времени
										dt.hour = this->_fmk->atoi <uint8_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
									break;
									// Если формат получен как %H
									case static_cast <uint8_t> (format_t::H):
										// Устанавливаем полученный час времени
										dt.hour = this->_fmk->atoi <uint8_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
									break;
									// Если формат получен как %M
									case static_cast <uint8_t> (format_t::M):
										// Устанавливаем значение указанного количества минут
										dt.minutes = this->_fmk->atoi <uint8_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
									break;
									// Если формат получен как %s
									case static_cast <uint8_t> (format_t::s):
										// Устанавливаем количество миллисекунд
										dt.milliseconds = this->_fmk->atoi <uint32_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
									break;
									// Если формат получен как %S
									case static_cast <uint8_t> (format_t::S):
										// Устанавливаем значение указанного количества секунд
										dt.seconds = this->_fmk->atoi <uint8_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
									break;
									// Если формат получен как %Z
									case static_cast <uint8_t> (format_t::Z): {
										// Выполняем матчинг временной зоны
										dt.zone = this->matchTimeZone(string(text.data() + pos + match[j].begin, match[j].end - match[j].begin));
										// Получаем название временной зоны
										dt.offset = this->getTimeZone(dt.zone);
									} break;
									// Если формат получен как %a
									case static_cast <uint8_t> (format_t::a): {
										// Получаем название дня недели
										const string day(text.data() + pos + match[j].begin, match[j].end - match[j].begin);
										/**
										 * Выполняем перебор всего списка дней недели
										 */
										for(size_t i = 0; i < params.nameDays.size(); i++){
											// Если мы нашли нужный нам день недели
											if(this->_fmk->compare(day, params.nameDays[i].first)){
												// Устанавливаем день недели
												dt.day = static_cast <uint8_t> (i + 1);
												// Выходим из цикла
												break;
											}
										}
									} break;
									// Если формат получен как %A
									case static_cast <uint8_t> (format_t::A): {
										// Получаем название дня недели
										const string day(text.data() + pos + match[j].begin, match[j].end - match[j].begin);
										/**
										 * Выполняем перебор всего списка дней недели
										 */
										for(size_t i = 0; i < params.nameDays.size(); i++){
											// Если мы нашли нужный нам день недели
											if(this->_fmk->compare(day, params.nameDays[i].second)){
												// Устанавливаем день недели
												dt.day = static_cast <uint8_t> (i + 1);
												// Выходим из цикла
												break;
											}
										}
									} break;
									// Если формат получен как %b
									case static_cast <uint8_t> (format_t::b): {
										// Получаем название месяца
										const string month(text.data() + pos + match[j].begin, match[j].end - match[j].begin);
										/**
										 * Выполняем перебор всего списка месяцев
										 */
										for(size_t i = 0; i < params.nameMonths.size(); i++){
											// Если мы нашли нужный нам месяц
											if(this->_fmk->compare(month, params.nameMonths[i].first)){
												// Устанавливаем месяц
												dt.month = static_cast <uint8_t> (i + 1);
												// Выходим из цикла
												break;
											}
										}
									} break;
									// Если формат получен как %B
									case static_cast <uint8_t> (format_t::B): {
										// Получаем название месяца
										const string month(text.data() + pos + match[j].begin, match[j].end - match[j].begin);
										/**
										 * Выполняем перебор всего списка месяцев
										 */
										for(size_t i = 0; i < params.nameMonths.size(); i++){
											// Если мы нашли нужный нам месяц
											if(this->_fmk->compare(month, params.nameMonths[i].second)){
												// Устанавливаем месяц
												dt.month = static_cast <uint8_t> (i + 1);
												// Выходим из цикла
												break;
											}
										}
									} break;
									// Если формат получен как %p
									case static_cast <uint8_t> (format_t::p): {
										// Получаем название времени суток
										const string name(text.data() + pos + match[j].begin, match[j].end - match[j].begin);
										// Определяем 12-и часовой формат времени
										dt.h12 = (this->_fmk->compare("pm", name) ? h12_t::PM : h12_t::AM);
										// Если мы получили вечернее время
										if((dt.h12 == h12_t::PM) && (dt.hour < 12))
											// Увеличиваем полученный час времени
											dt.hour += 12;
										// Если мы получили утреннее время
										else if((dt.h12 == h12_t::AM) && (dt.hour == 12))
											// Обнуляем полученный час времени
											dt.hour = 0;
									} break;
								}
							}
						}
					} break;
					// Если формат получен как %z
					case static_cast <uint8_t> (format_t::z): {
						// Если временная зона не установлена
						if(dt.zone == zone_t::NONE)
							// Выполняем сброс временной зоны
							dt.offset = 0;
						// Создаём массив собранных результатов
						vector <string> data(match.size());
						/**
						 * Выполняем перебор всех полученных вариантов
						 */
						for(uint8_t j = 0; j < static_cast <uint8_t> (match.size()); j++){
							// Если результат получен
							if(match[j].end > match[j].begin){
								// Если это первый элемент
								if(j == 0)
									// Получаем смещение в тексте
									result = static_cast <ssize_t> (pos + match[j].end);
								// Выполняем установку результата
								data[j].assign(text.data() + pos + match[j].begin, match[j].end - match[j].begin);
							}
						}
						// Если название временной зоны указано
						if(!data[1].empty() && (dt.zone == zone_t::UTC))
							// Выполняем установку временной зоны
							dt.zone = zone_t::UTC;
						// Получаем смещение времени
						const string & offset = data[2];
						// Если полученное смещение является числом
						if(this->_fmk->is(offset, fmk_t::check_t::NUMBER)){
							// Если указано 4 символа
							if(offset.size() == 4){
								// Получаем количество часов
								const uint8_t hour = this->_fmk->atoi <uint8_t> (offset.c_str(), 2);
								// Получаем количество минут
								const uint8_t minutes = this->_fmk->atoi <uint8_t> (offset.c_str() + 2, offset.length() - 2);
								// Если смещение времени положительное
								if(data[1].compare("+") == 0)
									// Получаем время смещения
									dt.offset += static_cast <int32_t> ((hour * 60 * 60) + (minutes * 60));
								// Устанавливаем отрицательное смещение времени
								else dt.offset -= static_cast <int32_t> ((hour * 60 * 60) + (minutes * 60));
							// Если установлен всего один символ
							} else {
								// Если смещение времени положительное
								if(data[1].compare("+") == 0)
									// Получаем время смещения
									dt.offset += (this->_fmk->atoi <int32_t> (offset) * 60 * 60);
								// Устанавливаем отрицательное смещение времени
								else dt.offset -= (this->_fmk->atoi <int32_t> (offset) * 60 * 60);
							}
						// Если получено время в формате часов
						} else if((data.size() > 4) && !data[3].empty() && !data[4].empty()) {
							// Получаем количество часов
							const uint8_t hour = this->_fmk->atoi <uint8_t> (data[3]);
							// Получаем количество минут
							const uint8_t minutes = this->_fmk->atoi <uint8_t> (data[4]);
							// Если смещение времени положительное
							if(data[1].compare("+") == 0)
								// Получаем время смещения
								dt.offset += static_cast <int32_t> ((hour * 60 * 60) + (minutes * 60));
							// Устанавливаем отрицательное смещение времени
							else dt.offset -= static_cast <int32_t> ((hour * 60 * 60) + (minutes * 60));
						}
					} break;
					// Если формат получен как %R
					case static_cast <uint8_t> (format_t::R): {
						/**
						 * Выполняем перебор всех полученных вариантов
						 */
						for(uint8_t j = 0; j < static_cast <uint8_t> (match.size()); j++){
							// Если результат получен
							if(match[j].end > match[j].begin){
								// Если это первый элемент
								if(j == 0)
									// Получаем смещение в тексте
									result = static_cast <ssize_t> (pos + match[j].end);
								// Если мы получили час
								else if(j == 1)
									// Устанавливаем полученный час времени
									dt.hour = this->_fmk->atoi <uint8_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
								// Если мы получили минуты
								else if(j == 2)
									// Устанавливаем значение указанного количества минут
									dt.minutes = this->_fmk->atoi <uint8_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
							}
						}
					} break;
					// Если формат получен как %D
					case static_cast <uint8_t> (format_t::D): {
						/**
						 * Выполняем перебор всех полученных вариантов
						 */
						for(uint8_t j = 0; j < static_cast <uint8_t> (match.size()); j++){
							// Если результат получен
							if(match[j].end > match[j].begin){
								// Если это первый элемент
								if(j == 0)
									// Получаем смещение в тексте
									result = static_cast <ssize_t> (pos + match[j].end);
								// Если мы получили номер месяца
								else if(j == 1)
									// Устанавливаем полученный номер месяца
									dt.month = this->_fmk->atoi <uint8_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
								// Если мы получили число месяца
								else if(j == 2)
									// Устанавливаем число месяца
									dt.date = this->_fmk->atoi <uint8_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
								// Если мы получили год
								else if(j == 3) {
									// Получаем значение указанного года
									const uint16_t num = this->_fmk->atoi <uint16_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
									// Устанавливаем год
									dt.year = (2000 + num);
									// Устанавливаем флаг високосного года
									dt.leap = this->leap(dt.year);
								}
							}
						}
					} break;
					// Если формат получен как %F
					case static_cast <uint8_t> (format_t::F): {
						/**
						 * Выполняем перебор всех полученных вариантов
						 */
						for(uint8_t j = 0; j < static_cast <uint8_t> (match.size()); j++){
							// Если результат получен
							if(match[j].end > match[j].begin){
								// Если это первый элемент
								if(j == 0)
									// Получаем смещение в тексте
									result = static_cast <ssize_t> (pos + match[j].end);
								// Если мы получили год
								else if(j == 1) {
									// Устанавливаем год
									dt.year = this->_fmk->atoi <uint16_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
									// Устанавливаем флаг високосного года
									dt.leap = this->leap(dt.year);
								// Если мы получили номер месяца
								} else if(j == 2)
									// Получаем значение номера месяца
									dt.month = this->_fmk->atoi <uint8_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
								// Если мы получили число месяца
								else if(j == 3)
									// Устанавливаем число месяца
									dt.date = this->_fmk->atoi <uint8_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
							}
						}
					} break;
					// Если формат получен как %T
					case static_cast <uint8_t> (format_t::T): {
						/**
						 * Выполняем перебор всех полученных вариантов
						 */
						for(uint8_t j = 0; j < static_cast <uint8_t> (match.size()); j++){
							// Если результат получен
							if(match[j].end > match[j].begin){
								// Если это первый элемент
								if(j == 0)
									// Получаем смещение в тексте
									result = static_cast <ssize_t> (pos + match[j].end);
								// Если мы получили час
								else if(j == 1)
									// Устанавливаем полученный час времени
									dt.hour = this->_fmk->atoi <uint8_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
								// Если мы получили минуты
								else if(j == 2)
									// Устанавливаем значение указанного количества минут
									dt.minutes = this->_fmk->atoi <uint8_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
								// Если мы получили секунды
								else if(j == 3)
									// Устанавливаем значение указанного количества секунд
									dt.seconds = this->_fmk->atoi <uint8_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
							}
						}
					} break;
					// Если формат получен как %r
					case static_cast <uint8_t> (format_t::r): {
						/**
						 * Выполняем перебор всех полученных вариантов
						 */
						for(uint8_t j = 0; j < static_cast <uint8_t> (match.size()); j++){
							// Если результат получен
							if(match[j].end > match[j].begin){
								// Если это первый элемент
								if(j == 0)
									// Получаем смещение в тексте
									result = static_cast <ssize_t> (pos + match[j].end);
								// Если мы получили час
								else if(j == 1)
									// Устанавливаем полученный час времени
									dt.hour = this->_fmk->atoi <uint8_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
								// Если мы получили минуты
								else if(j == 2)
									// Устанавливаем значение указанного количества минут
									dt.minutes = this->_fmk->atoi <uint8_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
								// Если мы получили секунды
								else if(j == 3)
									// Устанавливаем значение указанного количества секунд
									dt.seconds = this->_fmk->atoi <uint8_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
								// Если мы получили метку времени
								else if(j == 4) {
									// Получаем название времени суток
									const string name(text.data() + pos + match[j].begin, match[j].end - match[j].begin);
									// Определяем 12-и часовой формат времени
									dt.h12 = (this->_fmk->compare("pm", name) ? h12_t::PM : h12_t::AM);
									// Если мы получили вечернее время
									if((dt.h12 == h12_t::PM) && (dt.hour < 12))
										// Увеличиваем полученный час времени
										dt.hour += 12;
									// Если мы получили утреннее время
									else if((dt.h12 == h12_t::AM) && (dt.hour == 12))
										// Обнуляем полученный час времени
										dt.hour = 0;
								}
							}
						}
					} break;
					// Если формат получен как %c
					case static_cast <uint8_t> (format_t::c): {
						/**
						 * Выполняем перебор всех полученных вариантов
						 */
						for(uint8_t j = 0; j < static_cast <uint8_t> (match.size()); j++){
							// Если результат получен
							if(match[j].end > match[j].begin){
								// Если это первый элемент
								if(j == 0)
									// Получаем смещение в тексте
									result = static_cast <ssize_t> (pos + match[j].end);
								// Если мы получили название дня недели
								else if(j == 1) {
									// Получаем название дня недели
									const string day(text.data() + pos + match[j].begin, match[j].end - match[j].begin);
									/**
									 * Выполняем перебор всего списка дней недели
									 */
									for(size_t i = 0; i < params.nameDays.size(); i++){
										// Если мы нашли нужный нам день недели
										if(this->_fmk->compare(day, params.nameDays[i].first)){
											// Устанавливаем день недели
											dt.day = static_cast <uint8_t> (i + 1);
											// Выходим из цикла
											break;
										}
									}
								// Если мы получили название месяца
								} else if(j == 2) {
									// Получаем название месяца
									const string month(text.data() + pos + match[j].begin, match[j].end - match[j].begin);
									/**
									 * Выполняем перебор всего списка месяцев
									 */
									for(size_t i = 0; i < params.nameMonths.size(); i++){
										// Если мы нашли нужный нам месяц
										if(this->_fmk->compare(month, params.nameMonths[i].first)){
											// Устанавливаем месяц
											dt.month = static_cast <uint8_t> (i + 1);
											// Выходим из цикла
											break;
										}
									}
								// Если мы получили число месяца
								} else if(j == 3)
									// Устанавливаем число месяца
									dt.date = this->_fmk->atoi <uint8_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
								// Если мы получили час
								else if(j == 4)
									// Устанавливаем полученный час времени
									dt.hour = this->_fmk->atoi <uint8_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
								// Если мы получили минуты
								else if(j == 5)
									// Устанавливаем значение указанного количества минут
									dt.minutes = this->_fmk->atoi <uint8_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
								// Если мы получили секунды
								else if(j == 6)
									// Устанавливаем значение указанного количества секунд
									dt.seconds = this->_fmk->atoi <uint8_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
								// Если мы получили год
								else if(j == 7) {
									// Устанавливаем год
									dt.year = this->_fmk->atoi <uint16_t> (text.data() + (pos + match[j].begin), match[j].end - match[j].begin);
									// Устанавливаем флаг високосного года
									dt.leap = this->leap(dt.year);
								}
							}
						}
					} break;
				}
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод перевода времени в аббревиатуру
 *
 * @param date дата в UnixTimestamp
 * @return     сформированная аббревиатура даты
 *
 */
std::pair <awh::Chrono::type_t, double> awh::Chrono::abbreviation(const uint64_t date) const noexcept {
	// Переменная результата
	std::pair <type_t, double> result = {type_t::MILLISECONDS, 0.};
	// Если число передано
	if(date > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если число больше года
			if(date >= 29030400000)
				// Выполняем формирование результата
				result = std::make_pair(type_t::YEAR, static_cast <double> (date / 29030400000.L));
			// Если число больше месяца
			else if(date >= 2419200000)
				// Выполняем формирование результата
				result = std::make_pair(type_t::MONTH, static_cast <double> (date / 2419200000.L));
			// Если число больше недели
			else if(date >= 604800000)
				// Выполняем формирование результата
				result = std::make_pair(type_t::WEEK, static_cast <double> (date / 604800000.L));
			// Если число больше дня
			else if(date >= 86400000)
				// Выполняем формирование результата
				result = std::make_pair(type_t::DAY, static_cast <double> (date / 86400000.L));
			// Если число больше часа
			else if(date >= 3600000)
				// Выполняем формирование результата
				result = std::make_pair(type_t::HOUR, static_cast <double> (date / 3600000.L));
			// Если число больше минуты
			else if(date >= 60000)
				// Выполняем формирование результата
				result = std::make_pair(type_t::MINUTES, static_cast <double> (date / 60000.L));
			// Если число больше секунды
			else if(date >= 1000)
				// Выполняем формирование результата
				result = std::make_pair(type_t::SECONDS, static_cast <double> (date / 1000.L));
			// Иначе выводим как есть
			else result = std::make_pair(type_t::MILLISECONDS, static_cast <double> (date));
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(date), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод получения конца позиции указанной даты
 *
 * @param date дата для которой необходимо получить позицию
 * @param type тип единиц измерений даты
 * @return     конец указанной даты в формате UnixTimestamp
 *
 */
uint64_t awh::Chrono::end(const uint64_t date, const type_t type) const noexcept {
	// Переменная результата
	uint64_t result = 0;
	// Если дата передана
	if(date > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем тип единиц измерений
			 */
			switch(static_cast <uint8_t> (type)){
				// Если нам нужно получить конец года
				case static_cast <uint8_t> (type_t::YEAR): {
					// Получаем значение текущего года
					const uint16_t year = this->year(date);
					// Получаем штамп времени начала года
					result = this->beginOfYear(year);
					/**
					 * Выполняем перебор всех месяцев в году
					 */
					for(size_t i = 0; i < params.daysInMonths.size(); i++)
						// Увеличиваем количество дней в месяце
						result += (static_cast <uint64_t> (params.daysInMonths[i]) * static_cast <uint64_t> (86400000));
					// Если год високосный
					if(this->leap(year))
						// Добавляем ещё один день
						result += 86400000;
				} break;
				// Если нам нужно получить конец месяца
				case static_cast <uint8_t> (type_t::MONTH): {
					// Получаем значение текущего года
					const uint16_t year = this->year(date);
					// Получаем штамп времени начала года
					const uint64_t beginYear = this->beginOfYear(year);
					// Определяем сколько дней прошло с начала года
					const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
					{
						// Подсчитываем количество дней в предыдущих месяцах
						uint16_t count = 0, days = 0;
						// Устанавливаем флаг високосного года
						const bool leap = this->leap(year);
						/**
						 * Выполняем перебор всех дней месяца
						 */
						for(uint8_t i = 0; i < params.daysInMonths.size(); i++){
							// Получаем текущее количество дней с компенсацией високосного года
							days = (static_cast <uint16_t> (params.daysInMonths[i]) + ((i == 1) && leap ? 1 : 0));
							// Если мы не дошли до предела
							if(lastDays >= (days + count))
								// Увеличиваем количество прошедших дней
								count += days;
							// Выходим из цикла
							else break;
						}
						// Получаем начало месяца указанной даты
						result = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
						// Увеличиваем количество дней до конца месяца
						result += (static_cast <uint64_t> (days) * static_cast <uint64_t> (86400000));
					}
				} break;
				// Если нам нужно получить конец недели
				case static_cast <uint8_t> (type_t::WEEK): {
					// Получаем количество миллисекунд с начала текущей недели
					result = this->begin(date, type);
					// Увеличиваем количество дней до конца недели
					result += (static_cast <uint64_t> (7) * static_cast <uint64_t> (86400000));
				} break;
				// Если нам нужно получить конец дня
				case static_cast <uint8_t> (type_t::DAY):
					// Получаем количество миллисекунд конца текущего дня
					result = (this->begin(date, type) + 86400000);
				break;
				// Если нам нужно получить конец часа
				case static_cast <uint8_t> (type_t::HOUR):
					// Получаем количество миллисекунд конца текущего часа
					result = (this->begin(date, type) + 3600000);
				break;
				// Если нам нужно получить конец минуты
				case static_cast <uint8_t> (type_t::MINUTES):
					// Получаем количество миллисекунд конца текущей минуты
					result = (this->begin(date, type) + 60000);
				break;
				// Если нам нужно получить конец секунды
				case static_cast <uint8_t> (type_t::SECONDS):
					// Получаем количество миллисекунд конца текущей секунды
					result = (this->begin(date, type) + 1000);
				break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(date, static_cast <uint16_t> (type)), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод получения конца позиции текущей даты
 *
 * @param type    тип единиц измерений даты
 * @param storage хранение значение времени
 * @return        конец текущей даты в формате UnixTimestamp
 *
 */
uint64_t awh::Chrono::end(const type_t type, const storage_t storage) const noexcept {
	// Выполняем получение конца позиции текущей даты
	return this->end(this->timestamp(type_t::MILLISECONDS, storage), type);
}
/**
 * @brief Метод получения начала позиции указанной даты
 *
 * @param date дата для которой необходимо получить позицию
 * @param type тип единиц измерений даты
 * @return     начало указанной даты в формате UnixTimestamp
 *
 */
uint64_t awh::Chrono::begin(const uint64_t date, const type_t type) const noexcept {
	// Переменная результата
	uint64_t result = 0;
	// Если дата передана
	if(date > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем тип единиц измерений
			 */
			switch(static_cast <uint8_t> (type)){
				// Если нам нужно получить начало года
				case static_cast <uint8_t> (type_t::YEAR): {
					// Получаем значение текущего года
					const uint16_t year = this->year(date);
					// Получаем штамп времени начала года
					result = this->beginOfYear(year);
				} break;
				// Если нам нужно получить начало месяца
				case static_cast <uint8_t> (type_t::MONTH): {
					// Получаем значение текущего года
					const uint16_t year = this->year(date);
					// Получаем штамп времени начала года
					const uint64_t beginYear = this->beginOfYear(year);
					// Определяем сколько дней прошло с начала года
					const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
					{
						// Подсчитываем количество дней в предыдущих месяцах
						uint16_t count = 0, days = 0;
						// Устанавливаем флаг високосного года
						const bool leap = this->leap(year);
						/**
						 * Выполняем перебор всех дней месяца
						 */
						for(uint8_t i = 0; i < params.daysInMonths.size(); i++){
							// Получаем текущее количество дней с компенсацией високосного года
							days = (static_cast <uint16_t> (params.daysInMonths[i]) + ((i == 1) && leap ? 1 : 0));
							// Если мы не дошли до предела
							if(lastDays >= (days + count))
								// Увеличиваем количество прошедших дней
								count += days;
							// Выходим из цикла
							else break;
						}
						// Получаем начало месяца указанной даты
						result = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
					}
				} break;
				// Если нам нужно получить начало недели
				case static_cast <uint8_t> (type_t::WEEK): {
					// Получаем значение текущего года
					const uint16_t year = this->year(date);
					// Получаем штамп времени начала года
					const uint64_t beginYear = this->beginOfYear(year);
					// Определяем сколько дней прошло с начала года
					const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
					{
						// Номер текущего месяца
						uint8_t month = 0;
						// Подсчитываем количество дней в предыдущих месяцах
						uint16_t count = 0, days = 0;
						// Устанавливаем флаг високосного года
						const bool leap = this->leap(year);
						/**
						 * Выполняем перебор всех дней месяца
						 */
						for(uint8_t i = 0; i < params.daysInMonths.size(); i++){
							// Увеличиваем номер месяца
							month = (i + 1);
							// Получаем текущее количество дней с компенсацией високосного года
							days = (static_cast <uint16_t> (params.daysInMonths[i]) + ((i == 1) && leap ? 1 : 0));
							// Если мы не дошли до предела
							if(lastDays >= (days + count))
								// Увеличиваем количество прошедших дней
								count += days;
							// Выходим из цикла
							else break;
						}
						// Устанавливаем текущее значение даты
						const uint8_t date = static_cast <uint8_t> ((lastDays - count) + 1);
						// Получаем начало месяца указанной даты
						const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
						// Получаем начало суток указанной даты
						const uint64_t beginDay = (beginMonth + (static_cast <uint64_t> (date - 1) * static_cast <uint64_t> (86400000)));
						// Получаем множитель текущего года
						auto i = params.rateLeapYears.find(static_cast <uint16_t> ((year - (year % 4)) % 7));
						// Если множитель получен
						if(i != params.rateLeapYears.end()){
							// Подробнее: https://habr.com/ru/articles/217389
							// Устанавливаем день недели
							uint8_t day = (((i->second + static_cast <uint8_t> (year % 4) + params.rateMonths[month - 1] + date) - (((month == 1) || (month == 2)) && leap ? 1 : 0)) % 7);
							// Если воскресенье установлен как нулевой
							if(day == 0)
								// Выполняем компенсацию
								day = 6;
							// Уменьшаем день на один
							else day--;
							// Получаем начало недели
							result = (beginDay - (static_cast <uint64_t> (day) * static_cast <uint64_t> (86400000)));
						}
					}
				} break;
				// Если нам нужно получить начало дня
				case static_cast <uint8_t> (type_t::DAY):
					// Выполняем определение начала дня
					result = (date - (date % 86400000));
				break;
				// Если нам нужно получить начало часа
				case static_cast <uint8_t> (type_t::HOUR):
					// Выполняем определение начала часа
					result = (date - (date % 3600000));
				break;
				// Если нам нужно получить начало минуты
				case static_cast <uint8_t> (type_t::MINUTES):
					// Выполняем определение начала минут
					result = (date - (date % 60000));
				break;
				// Если нам нужно получить начало секунды
				case static_cast <uint8_t> (type_t::SECONDS):
					// Выполняем определение начала секунд
					result = (date - (date % 1000));
				break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(date, static_cast <uint16_t> (type)), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод получения начала позиции текущей даты
 *
 * @param type    тип единиц измерений даты
 * @param storage хранение значение времени
 * @return        начало текущей даты в формате UnixTimestamp
 *
 */
uint64_t awh::Chrono::begin(const type_t type, const storage_t storage) const noexcept {
	// Выполняем получение начала позиции текущей даты
	return this->begin(this->timestamp(type_t::MILLISECONDS, storage), type);
}
/**
 * @brief Метод актуализации прошедшего и оставшегося времени
 *
 * @param date   дата относительно которой производятся расчёты
 * @param value  тип определяемых единиц измерений времени
 * @param type   тип единиц измерений даты
 * @param actual направление актуализации
 * @return       результат вычисления
 *
 */
uint64_t awh::Chrono::actual(const uint64_t date, const type_t value, const type_t type, const actual_t actual) const noexcept {
	// Переменная результата
	uint64_t result = 0;
	// Если дата передана
	if(date > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем направление актуализации
			 */
			switch(static_cast <uint8_t> (actual)){
				// Если нужно определить сколько осталось времени
				case static_cast <uint8_t> (actual_t::LEFT): {
					/**
					 * Определяем тип единиц измерений
					 */
					switch(static_cast <uint8_t> (type)){
						// Если нам нужно получить количество оставшего времени в году
						case static_cast <uint8_t> (type_t::YEAR): {
							/**
							 * Определяем тип определяемых единиц измерений
							 */
							switch(static_cast <uint8_t> (value)){
								// Если нам нужно получить количество оставшихся месяцев
								case static_cast <uint8_t> (type_t::MONTH): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Получаем штамп времени начала года
									const uint64_t beginYear = this->beginOfYear(year);
									// Определяем сколько дней прошло с начала года
									const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
									{
										// Номер текущего месяца
										uint8_t month = 0;
										// Подсчитываем количество дней в предыдущих месяцах
										uint16_t count = 0, days = 0;
										// Устанавливаем флаг високосного года
										const bool leap = this->leap(year);
										/**
										 * Выполняем перебор всех дней месяца
										 */
										for(uint8_t i = 0; i < params.daysInMonths.size(); i++){
											// Увеличиваем номер месяца
											month = (i + 1);
											// Получаем текущее количество дней с компенсацией високосного года
											days = (static_cast <uint16_t> (params.daysInMonths[i]) + ((i == 1) && leap ? 1 : 0));
											// Если мы не дошли до предела
											if(lastDays >= (days + count))
												// Увеличиваем количество прошедших дней
												count += days;
											// Выходим из цикла
											else break;
										}
										// Формируем полученный результат
										result = static_cast <uint64_t> (12 - month);
									}
								} break;
								// Если нам нужно получить количество оставшихся недель
								case static_cast <uint8_t> (type_t::WEEK): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Получаем штамп времени начала года
									const uint64_t beginYear = this->beginOfYear(year);
									// Количество недель в году
									uint8_t weeks = 0;
									// Если год високосный
									if(this->leap(year))
										// Получаем количество недель в году
										weeks = static_cast <uint8_t> (::round(31622400000 / 604800000.L));
									// Если год не високосный
									else weeks = static_cast <uint8_t> (::round(31536000000 / 604800000.L));
									// Получаем количество недель оставшихся в году
									result = static_cast <uint64_t> (weeks - static_cast <uint8_t> (::round((date - beginYear) / 604800000.L)));
								} break;
								// Если нам нужно получить количество оставшихся дней
								case static_cast <uint8_t> (type_t::DAY): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Получаем штамп времени начала года
									const uint64_t beginYear = this->beginOfYear(year);
									// Определяем сколько дней прошло с начала года
									const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
									// Если год високосный
									if(this->leap(year))
										// Определяем сколько осталось дней в году
										result = static_cast <uint64_t> (366 - (lastDays + 1));
									// Если год не високосный
									else result = static_cast <uint64_t> (365 - (lastDays + 1));
								} break;
								// Если нам нужно получить количество оставшихся часов
								case static_cast <uint8_t> (type_t::HOUR): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Получаем штамп времени начала года
									const uint64_t beginYear = this->beginOfYear(year);
									// Получаем количество часов прошедших с начала года
									const uint32_t hours = static_cast <uint32_t> (::ceil((date - beginYear) / 3600000.L));
									// Если год високосный
									if(this->leap(year))
										// Определяем количество оставшихся часов
										result = static_cast <uint64_t> (static_cast <uint32_t> (31622400000 / 3600000) - hours);
									// Если год не високосный
									else result = static_cast <uint64_t> (static_cast <uint32_t> (31536000000 / 3600000) - hours);
								} break;
								// Если нам нужно получить количество оставшихся минут
								case static_cast <uint8_t> (type_t::MINUTES): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Получаем штамп времени начала года
									const uint64_t beginYear = this->beginOfYear(year);
									// Получаем количество минут прошедших с начала года
									const uint64_t minutes = static_cast <uint64_t> (::ceil((date - beginYear) / 60000.));
									// Если год високосный
									if(this->leap(year))
										// Определяем количество оставшихся минут
										result = (static_cast <uint64_t> (31622400000 / 60000) - minutes);
									// Если год не високосный
									else result = (static_cast <uint64_t> (31536000000 / 60000) - minutes);
								} break;
								// Если нам нужно получить количество оставшихся секунд
								case static_cast <uint8_t> (type_t::SECONDS): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Получаем штамп времени начала года
									const uint64_t beginYear = this->beginOfYear(year);
									// Получаем количество секунд прошедших с начала года
									const uint64_t seconds = static_cast <uint64_t> (::ceil((date - beginYear) / 1000.));
									// Если год високосный
									if(this->leap(year))
										// Определяем количество оставшихся секунд
										result = (static_cast <uint64_t> (31622400000 / 1000) - seconds);
									// Если год не високосный
									else result = (static_cast <uint64_t> (31536000000 / 1000) - seconds);
								} break;
								// Если нам нужно получить количество оставшихся миллисекунд
								case static_cast <uint8_t> (type_t::MILLISECONDS): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Получаем штамп времени начала года
									const uint64_t beginYear = this->beginOfYear(year);
									// Получаем количество миллисекунд прошедших с начала года
									const uint64_t milliseconds = static_cast <uint64_t> (date - beginYear);
									// Если год високосный
									if(this->leap(year))
										// Определяем количество оставшихся миллисекунд
										result = (static_cast <uint64_t> (31622400000) - (milliseconds + 1));
									// Если год не високосный
									else result = (static_cast <uint64_t> (31536000000) - (milliseconds + 1));
								} break;
								// Если нам нужно получить количество оставшихся микросекунд
								case static_cast <uint8_t> (type_t::MICROSECONDS): {
									// Получаем текущее значение размерности даты
									const uint8_t current = static_cast <uint8_t> (this->digits(date) - 1);
									// Получаем размерность актуальной размерности даты
									const uint8_t actual = static_cast <uint8_t> (this->digits(this->timestamp(type_t::MILLISECONDS)) - 1);
									// Если текущее значение даты передано в микросекундах
									if(current == (actual + 3)){
										// Получаем значение текущего года
										const uint16_t year = this->year(date);
										// Получаем штамп времени начала года в микросекундах
										const uint64_t beginYear = (this->beginOfYear(year) * static_cast <uint64_t> (1000));
										// Получаем количество микросекунд прошедших с начала года
										const uint64_t microseconds = static_cast <uint64_t> (date - beginYear);
										// Если год високосный
										if(this->leap(year))
											// Определяем количество оставшихся микросекунд
											result = (static_cast <uint64_t> (31622400000000) - (microseconds + 1));
										// Если год не високосный
										else result = (static_cast <uint64_t> (31536000000000) - (microseconds + 1));
									// Если текущее значение даты передано в других единицах
									} else {
										// Устанавливаем текущее значение актуализации
										result = this->actual(date, type_t::MILLISECONDS, type, actual_t::LEFT);
										// Увеличиваем размер количества миллисекунд до микросекунд
										result *= 1000;
									}
								} break;
								// Если нам нужно получить количество оставшихся наносекунд
								case static_cast <uint8_t> (type_t::NANOSECONDS): {
									// Получаем текущее значение размерности даты
									const uint8_t current = static_cast <uint8_t> (this->digits(date) - 1);
									// Получаем размерность актуальной размерности даты
									const uint8_t actual = static_cast <uint8_t> (this->digits(this->timestamp(type_t::MILLISECONDS)) - 1);
									// Если текущее значение даты передано в наносекундах
									if(current == (actual + 6)){
										// Получаем значение текущего года
										const uint16_t year = this->year(date);
										// Получаем штамп времени начала года в наносекундах
										const uint64_t beginYear = (this->beginOfYear(year) * static_cast <uint64_t> (1000000));
										// Получаем количество наносекунд прошедших с начала года
										const uint64_t nanoseconds = static_cast <uint64_t> (date - beginYear);
										// Если год високосный
										if(this->leap(year))
											// Определяем количество оставшихся наносекунд
											result = (static_cast <uint64_t> (31622400000000000) - (nanoseconds + 1));
										// Если год не високосный
										else result = (static_cast <uint64_t> (31536000000000000) - (nanoseconds + 1));
									// Если текущее значение даты передано в других единицах
									} else {
										// Устанавливаем текущее значение актуализации
										result = this->actual(date, type_t::MILLISECONDS, type, actual_t::LEFT);
										// Увеличиваем размер количества миллисекунд до наносекунд
										result *= 1000000;
									}
								} break;
							}
						} break;
						// Если нам нужно получить количество оставшего времени в месяце
						case static_cast <uint8_t> (type_t::MONTH): {
							/**
							 * Определяем тип определяемых единиц измерений
							 */
							switch(static_cast <uint8_t> (value)){
								// Если нам нужно получить количество оставшихся недель
								case static_cast <uint8_t> (type_t::WEEK): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Получаем штамп времени начала года
									const uint64_t beginYear = this->beginOfYear(year);
									// Определяем сколько дней прошло с начала года
									const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
									{
										// Номер текущего месяца
										uint8_t month = 0;
										// Подсчитываем количество дней в предыдущих месяцах
										uint16_t count = 0, days = 0;
										// Устанавливаем флаг високосного года
										const bool leap = this->leap(year);
										/**
										 * Выполняем перебор всех дней месяца
										 */
										for(uint8_t i = 0; i < params.daysInMonths.size(); i++){
											// Увеличиваем номер месяца
											month = (i + 1);
											// Получаем текущее количество дней с компенсацией високосного года
											days = (static_cast <uint16_t> (params.daysInMonths[i]) + ((i == 1) && leap ? 1 : 0));
											// Если мы не дошли до предела
											if(lastDays >= (days + count))
												// Увеличиваем количество прошедших дней
												count += days;
											// Выходим из цикла
											else break;
										}
										// Получаем начало месяца указанной даты
										const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
										// Если год високосный и месяц февраль
										if(leap && (month == 2)){
											// Получаем количество недель в месяце
											const uint8_t weeks = static_cast <uint8_t> (::ceil((params.daysInMonths[month - 1] + 1) / 7.));
											// Получаем количество оставшихся недель в месяце
											result = static_cast <uint64_t> (weeks - static_cast <uint8_t> (::round((date - beginMonth) / 604800000.L)));
										// Если год не високосный или месяц не февраль
										} else {
											// Получаем количество недель в месяце
											const uint8_t weeks = static_cast <uint8_t> (::ceil(params.daysInMonths[month - 1] / 7.));
											// Получаем количество оставшихся недель в месяце
											result = static_cast <uint64_t> (weeks - static_cast <uint8_t> (::round((date - beginMonth) / 604800000.L)));
										}
									}
								} break;
								// Если нам нужно получить количество оставшихся дней
								case static_cast <uint8_t> (type_t::DAY): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Получаем штамп времени начала года
									const uint64_t beginYear = this->beginOfYear(year);
									// Определяем сколько дней прошло с начала года
									const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
									{
										// Номер текущего месяца
										uint8_t month = 0;
										// Подсчитываем количество дней в предыдущих месяцах
										uint16_t count = 0, days = 0;
										// Устанавливаем флаг високосного года
										const bool leap = this->leap(year);
										/**
										 * Выполняем перебор всех дней месяца
										 */
										for(uint8_t i = 0; i < params.daysInMonths.size(); i++){
											// Увеличиваем номер месяца
											month = (i + 1);
											// Получаем текущее количество дней с компенсацией високосного года
											days = (static_cast <uint16_t> (params.daysInMonths[i]) + ((i == 1) && leap ? 1 : 0));
											// Если мы не дошли до предела
											if(lastDays >= (days + count))
												// Увеличиваем количество прошедших дней
												count += days;
											// Выходим из цикла
											else break;
										}
										// Получаем начало месяца указанной даты
										const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
										// Если год високосный и месяц февраль
										if(leap && (month == 2))
											// Получаем количество оставшихся дней в месяце
											result = static_cast <uint64_t> ((params.daysInMonths[month - 1] + 1) - static_cast <uint8_t> (::round((date - beginMonth) / 86400000.L)));
										// Получаем количество оставшихся дней в месяце
										else result = static_cast <uint64_t> (params.daysInMonths[month - 1] - static_cast <uint8_t> (::round((date - beginMonth) / 86400000.L)));
									}
								} break;
								// Если нам нужно получить количество оставшихся часов
								case static_cast <uint8_t> (type_t::HOUR): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Получаем штамп времени начала года
									const uint64_t beginYear = this->beginOfYear(year);
									// Определяем сколько дней прошло с начала года
									const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
									{
										// Номер текущего месяца
										uint8_t month = 0;
										// Подсчитываем количество дней в предыдущих месяцах
										uint16_t count = 0, days = 0;
										// Устанавливаем флаг високосного года
										const bool leap = this->leap(year);
										/**
										 * Выполняем перебор всех дней месяца
										 */
										for(uint8_t i = 0; i < params.daysInMonths.size(); i++){
											// Увеличиваем номер месяца
											month = (i + 1);
											// Получаем текущее количество дней с компенсацией високосного года
											days = (static_cast <uint16_t> (params.daysInMonths[i]) + ((i == 1) && leap ? 1 : 0));
											// Если мы не дошли до предела
											if(lastDays >= (days + count))
												// Увеличиваем количество прошедших дней
												count += days;
											// Выходим из цикла
											else break;
										}
										// Получаем начало месяца указанной даты
										const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
										// Если год високосный и месяц февраль
										if(leap && (month == 2))
											// Получаем количество оставшихся часов в месяце
											result = static_cast <uint64_t> ((static_cast <uint32_t> ((params.daysInMonths[month - 1] + 1) * static_cast <uint64_t> (86400000)) - static_cast <uint32_t> (date - beginMonth)) / 3600000);
										// Получаем количество оставшихся часов в месяце
										else result = static_cast <uint64_t> ((static_cast <uint32_t> (params.daysInMonths[month - 1] * static_cast <uint64_t> (86400000)) - static_cast <uint32_t> (date - beginMonth)) / 3600000);
									}
								} break;
								// Если нам нужно получить количество оставшихся минут
								case static_cast <uint8_t> (type_t::MINUTES): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Получаем штамп времени начала года
									const uint64_t beginYear = this->beginOfYear(year);
									// Определяем сколько дней прошло с начала года
									const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
									{
										// Номер текущего месяца
										uint8_t month = 0;
										// Подсчитываем количество дней в предыдущих месяцах
										uint16_t count = 0, days = 0;
										// Устанавливаем флаг високосного года
										const bool leap = this->leap(year);
										/**
										 * Выполняем перебор всех дней месяца
										 */
										for(uint8_t i = 0; i < params.daysInMonths.size(); i++){
											// Увеличиваем номер месяца
											month = (i + 1);
											// Получаем текущее количество дней с компенсацией високосного года
											days = (static_cast <uint16_t> (params.daysInMonths[i]) + ((i == 1) && leap ? 1 : 0));
											// Если мы не дошли до предела
											if(lastDays >= (days + count))
												// Увеличиваем количество прошедших дней
												count += days;
											// Выходим из цикла
											else break;
										}
										// Получаем начало месяца указанной даты
										const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
										// Если год високосный и месяц февраль
										if(leap && (month == 2))
											// Получаем количество оставшихся минут в месяце
											result = static_cast <uint64_t> ((static_cast <uint32_t> ((params.daysInMonths[month - 1] + 1) * static_cast <uint64_t> (86400000)) - static_cast <uint32_t> (date - beginMonth)) / 60000);
										// Получаем количество оставшихся минут в месяце
										else result = static_cast <uint64_t> ((static_cast <uint32_t> (params.daysInMonths[month - 1] * static_cast <uint64_t> (86400000)) - static_cast <uint32_t> (date - beginMonth)) / 60000);
									}
								} break;
								// Если нам нужно получить количество оставшихся секунд
								case static_cast <uint8_t> (type_t::SECONDS): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Получаем штамп времени начала года
									const uint64_t beginYear = this->beginOfYear(year);
									// Определяем сколько дней прошло с начала года
									const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
									{
										// Номер текущего месяца
										uint8_t month = 0;
										// Подсчитываем количество дней в предыдущих месяцах
										uint16_t count = 0, days = 0;
										// Устанавливаем флаг високосного года
										const bool leap = this->leap(year);
										/**
										 * Выполняем перебор всех дней месяца
										 */
										for(uint8_t i = 0; i < params.daysInMonths.size(); i++){
											// Увеличиваем номер месяца
											month = (i + 1);
											// Получаем текущее количество дней с компенсацией високосного года
											days = (static_cast <uint16_t> (params.daysInMonths[i]) + ((i == 1) && leap ? 1 : 0));
											// Если мы не дошли до предела
											if(lastDays >= (days + count))
												// Увеличиваем количество прошедших дней
												count += days;
											// Выходим из цикла
											else break;
										}
										// Получаем начало месяца указанной даты
										const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
										// Если год високосный и месяц февраль
										if(leap && (month == 2))
											// Получаем количество оставшихся секунд в месяце
											result = static_cast <uint64_t> ((static_cast <uint32_t> ((params.daysInMonths[month - 1] + 1) * static_cast <uint64_t> (86400000)) - static_cast <uint32_t> (date - beginMonth)) / 1000);
										// Получаем количество оставшихся секунд в месяце
										else result = static_cast <uint64_t> ((static_cast <uint32_t> (params.daysInMonths[month - 1] * static_cast <uint64_t> (86400000)) - static_cast <uint32_t> (date - beginMonth)) / 1000);
									}
								} break;
								// Если нам нужно получить количество оставшихся миллисекунд
								case static_cast <uint8_t> (type_t::MILLISECONDS): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Получаем штамп времени начала года
									const uint64_t beginYear = this->beginOfYear(year);
									// Определяем сколько дней прошло с начала года
									const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
									{
										// Номер текущего месяца
										uint8_t month = 0;
										// Подсчитываем количество дней в предыдущих месяцах
										uint16_t count = 0, days = 0;
										// Устанавливаем флаг високосного года
										const bool leap = this->leap(year);
										/**
										 * Выполняем перебор всех дней месяца
										 */
										for(uint8_t i = 0; i < params.daysInMonths.size(); i++){
											// Увеличиваем номер месяца
											month = (i + 1);
											// Получаем текущее количество дней с компенсацией високосного года
											days = (static_cast <uint16_t> (params.daysInMonths[i]) + ((i == 1) && leap ? 1 : 0));
											// Если мы не дошли до предела
											if(lastDays >= (days + count))
												// Увеличиваем количество прошедших дней
												count += days;
											// Выходим из цикла
											else break;
										}
										// Получаем начало месяца указанной даты
										const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
										// Если год високосный и месяц февраль
										if(leap && (month == 2))
											// Получаем количество оставшихся миллисекунд в месяце
											result = static_cast <uint64_t> (static_cast <uint32_t> ((params.daysInMonths[month - 1] + 1) * static_cast <uint64_t> (86400000)) - static_cast <uint32_t> (date - beginMonth));
										// Получаем количество оставшихся миллисекунд в месяце
										else result = static_cast <uint64_t> (static_cast <uint32_t> (params.daysInMonths[month - 1] * static_cast <uint64_t> (86400000)) - static_cast <uint32_t> (date - beginMonth));
									}
								} break;
								// Если нам нужно получить количество оставшихся микросекунд
								case static_cast <uint8_t> (type_t::MICROSECONDS): {
									// Получаем текущее значение размерности даты
									const uint8_t current = static_cast <uint8_t> (this->digits(date) - 1);
									// Получаем размерность актуальной размерности даты
									const uint8_t actual = static_cast <uint8_t> (this->digits(this->timestamp(type_t::MILLISECONDS)) - 1);
									// Если текущее значение даты передано в микросекундах
									if(current == (actual + 3)){
										// Получаем значение текущего года
										const uint16_t year = this->year(date);
										// Получаем штамп времени начала года в микросекундах
										const uint64_t beginYear = (this->beginOfYear(year) * static_cast <uint64_t> (1000));
										// Определяем сколько дней прошло с начала года
										const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000000.L));
										{
											// Номер текущего месяца
											uint8_t month = 0;
											// Подсчитываем количество дней в предыдущих месяцах
											uint16_t count = 0, days = 0;
											// Устанавливаем флаг високосного года
											const bool leap = this->leap(year);
											/**
											 * Выполняем перебор всех дней месяца
											 */
											for(uint8_t i = 0; i < params.daysInMonths.size(); i++){
												// Увеличиваем номер месяца
												month = (i + 1);
												// Получаем текущее количество дней с компенсацией високосного года
												days = (static_cast <uint16_t> (params.daysInMonths[i]) + ((i == 1) && leap ? 1 : 0));
												// Если мы не дошли до предела
												if(lastDays >= (days + count))
													// Увеличиваем количество прошедших дней
													count += days;
												// Выходим из цикла
												else break;
											}
											// Получаем начало месяца указанной даты
											const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000000)));
											// Если год високосный и месяц февраль
											if(leap && (month == 2))
												// Получаем количество оставшихся микросекунд в месяце
												result = static_cast <uint64_t> (static_cast <uint64_t> ((params.daysInMonths[month - 1] + 1) * 86400000000) - static_cast <uint64_t> (date - beginMonth));
											// Получаем количество оставшихся микросекунд в месяце
											else result = static_cast <uint64_t> (static_cast <uint64_t> (params.daysInMonths[month - 1] * 86400000000) - static_cast <uint64_t> (date - beginMonth));
										}
									// Если текущее значение даты передано в других единицах
									} else {
										// Устанавливаем текущее значение актуализации
										result = this->actual(date, type_t::MILLISECONDS, type, actual_t::LEFT);
										// Увеличиваем размер количества миллисекунд до микросекунд
										result *= 1000;
									}
								} break;
								// Если нам нужно получить количество оставшихся наносекунд
								case static_cast <uint8_t> (type_t::NANOSECONDS): {
									// Получаем текущее значение размерности даты
									const uint8_t current = static_cast <uint8_t> (this->digits(date) - 1);
									// Получаем размерность актуальной размерности даты
									const uint8_t actual = static_cast <uint8_t> (this->digits(this->timestamp(type_t::MILLISECONDS)) - 1);
									// Если текущее значение даты передано в наносекундах
									if(current == (actual + 6)){
										// Получаем значение текущего года
										const uint16_t year = this->year(date);
										// Получаем штамп времени начала года в наносекундах
										const uint64_t beginYear = (this->beginOfYear(year) * static_cast <uint64_t> (1000000));
										// Определяем сколько дней прошло с начала года
										const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000000000.L));
										{
											// Номер текущего месяца
											uint8_t month = 0;
											// Подсчитываем количество дней в предыдущих месяцах
											uint16_t count = 0, days = 0;
											// Устанавливаем флаг високосного года
											const bool leap = this->leap(year);
											/**
											 * Выполняем перебор всех дней месяца
											 */
											for(uint8_t i = 0; i < params.daysInMonths.size(); i++){
												// Увеличиваем номер месяца
												month = (i + 1);
												// Получаем текущее количество дней с компенсацией високосного года
												days = (static_cast <uint16_t> (params.daysInMonths[i]) + ((i == 1) && leap ? 1 : 0));
												// Если мы не дошли до предела
												if(lastDays >= (days + count))
													// Увеличиваем количество прошедших дней
													count += days;
												// Выходим из цикла
												else break;
											}
											// Получаем начало месяца указанной даты
											const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000000000)));
											// Если год високосный и месяц февраль
											if(leap && (month == 2))
												// Получаем количество оставшихся наносекунд в месяце
												result = static_cast <uint64_t> (static_cast <uint64_t> ((params.daysInMonths[month - 1] + 1) * 86400000000000) - static_cast <uint64_t> (date - beginMonth));
											// Получаем количество оставшихся наносекунд в месяце
											else result = static_cast <uint64_t> (static_cast <uint64_t> (params.daysInMonths[month - 1] * 86400000000000) - static_cast <uint64_t> (date - beginMonth));
										}
									// Если текущее значение даты передано в других единицах
									} else {
										// Устанавливаем текущее значение актуализации
										result = this->actual(date, type_t::MILLISECONDS, type, actual_t::LEFT);
										// Увеличиваем размер количества миллисекунд до наносекунд
										result *= 1000000;
									}
								} break;
							}
						} break;
						// Если нам нужно получить количество оставшего времени в неделе
						case static_cast <uint8_t> (type_t::WEEK): {
							/**
							 * Определяем тип определяемых единиц измерений
							 */
							switch(static_cast <uint8_t> (value)){
								// Если нам нужно получить количество оставшихся дней
								case static_cast <uint8_t> (type_t::DAY): {
									// Получаем начало текущего периода
									const uint64_t begin = this->begin(date, type);
									// Определяем количество оставшихся дней
									result = static_cast <uint64_t> ((static_cast <uint64_t> (604800000) - static_cast <uint64_t> (::floor(static_cast <long double> (date - begin)))) / 86400000.L);
								} break;
								// Если нам нужно получить количество оставшихся часов
								case static_cast <uint8_t> (type_t::HOUR): {
									// Получаем начало текущего периода
									const uint64_t begin = this->begin(date, type);
									// Определяем количество оставшихся часов
									result = static_cast <uint64_t> ((static_cast <uint64_t> (604800000) - static_cast <uint64_t> (::floor(static_cast <long double> (date - begin)))) / 3600000.L);
								} break;
								// Если нам нужно получить количество оставшихся минут
								case static_cast <uint8_t> (type_t::MINUTES): {
									// Получаем начало текущего периода
									const uint64_t begin = this->begin(date, type);
									// Определяем количество оставшихся минут
									result = static_cast <uint64_t> ((static_cast <uint64_t> (604800000) - static_cast <uint64_t> (::floor(static_cast <long double> (date - begin)))) / 60000.);
								} break;
								// Если нам нужно получить количество оставшихся секунд
								case static_cast <uint8_t> (type_t::SECONDS): {
									// Получаем начало текущего периода
									const uint64_t begin = this->begin(date, type);
									// Определяем количество оставшихся секунд
									result = static_cast <uint64_t> ((static_cast <uint64_t> (604800000) - static_cast <uint64_t> (::floor(static_cast <long double> (date - begin)))) / 1000.);
								} break;
								// Если нам нужно получить количество оставшихся миллисекунд
								case static_cast <uint8_t> (type_t::MILLISECONDS): {
									// Получаем начало текущего периода
									const uint64_t begin = this->begin(date, type);
									// Определяем количество оставшихся миллисекунд
									result = static_cast <uint64_t> (static_cast <uint64_t> (604800000) - static_cast <uint64_t> (::floor(static_cast <long double> (date - begin))));
								} break;
								// Если нам нужно получить количество оставшихся микросекунд
								case static_cast <uint8_t> (type_t::MICROSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::LEFT);
									// Увеличиваем размер количества миллисекунд до микросекунд
									result *= 1000;
								} break;
								// Если нам нужно получить количество оставшихся наносекунд
								case static_cast <uint8_t> (type_t::NANOSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::LEFT);
									// Увеличиваем размер количества миллисекунд до наносекунд
									result *= 1000000;
								} break;
							}
						} break;
						// Если нам нужно получить количество оставшего времени в дне
						case static_cast <uint8_t> (type_t::DAY): {
							/**
							 * Определяем тип определяемых единиц измерений
							 */
							switch(static_cast <uint8_t> (value)){
								// Если нам нужно получить количество оставшихся часов
								case static_cast <uint8_t> (type_t::HOUR): {
									// Получаем начало текущего периода
									const uint64_t begin = this->begin(date, type);
									// Определяем количество оставшихся часов
									result = static_cast <uint64_t> ((static_cast <uint64_t> (86400000) - static_cast <uint64_t> (::floor(static_cast <long double> (date - begin)))) / 3600000.L);
								} break;
								// Если нам нужно получить количество оставшихся минут
								case static_cast <uint8_t> (type_t::MINUTES): {
									// Получаем начало текущего периода
									const uint64_t begin = this->begin(date, type);
									// Определяем количество оставшихся минут
									result = static_cast <uint64_t> ((static_cast <uint64_t> (86400000) - static_cast <uint64_t> (::floor(static_cast <long double> (date - begin)))) / 60000.);
								} break;
								// Если нам нужно получить количество оставшихся секунд
								case static_cast <uint8_t> (type_t::SECONDS): {
									// Получаем начало текущего периода
									const uint64_t begin = this->begin(date, type);
									// Определяем количество оставшихся секунд
									result = static_cast <uint64_t> ((static_cast <uint64_t> (86400000) - static_cast <uint64_t> (::floor(static_cast <long double> (date - begin)))) / 1000.);
								} break;
								// Если нам нужно получить количество оставшихся миллисекунд
								case static_cast <uint8_t> (type_t::MILLISECONDS): {
									// Получаем начало текущего периода
									const uint64_t begin = this->begin(date, type);
									// Определяем количество оставшихся миллисекунд
									result = static_cast <uint64_t> (static_cast <uint64_t> (86400000) - static_cast <uint64_t> (::floor(static_cast <long double> (date - begin))));
								} break;
								// Если нам нужно получить количество оставшихся микросекунд
								case static_cast <uint8_t> (type_t::MICROSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::LEFT);
									// Увеличиваем размер количества миллисекунд до микросекунд
									result *= 1000;
								} break;
								// Если нам нужно получить количество оставшихся наносекунд
								case static_cast <uint8_t> (type_t::NANOSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::LEFT);
									// Увеличиваем размер количества миллисекунд до наносекунд
									result *= 1000000;
								} break;
							}
						} break;
						// Если нам нужно получить количество оставшего времени в часе
						case static_cast <uint8_t> (type_t::HOUR): {
							/**
							 * Определяем тип определяемых единиц измерений
							 */
							switch(static_cast <uint8_t> (value)){
								// Если нам нужно получить количество оставшихся минут
								case static_cast <uint8_t> (type_t::MINUTES): {
									// Получаем начало текущего периода
									const uint64_t begin = this->begin(date, type);
									// Определяем количество оставшихся минут
									result = static_cast <uint64_t> ((static_cast <uint64_t> (3600000) - static_cast <uint64_t> (::floor(static_cast <long double> (date - begin)))) / 60000.);
								} break;
								// Если нам нужно получить количество оставшихся секунд
								case static_cast <uint8_t> (type_t::SECONDS): {
									// Получаем начало текущего периода
									const uint64_t begin = this->begin(date, type);
									// Определяем количество оставшихся секунд
									result = static_cast <uint64_t> ((static_cast <uint64_t> (3600000) - static_cast <uint64_t> (::floor(static_cast <long double> (date - begin)))) / 1000.);
								} break;
								// Если нам нужно получить количество оставшихся миллисекунд
								case static_cast <uint8_t> (type_t::MILLISECONDS): {
									// Получаем начало текущего периода
									const uint64_t begin = this->begin(date, type);
									// Определяем количество оставшихся миллисекунд
									result = static_cast <uint64_t> (static_cast <uint64_t> (3600000) - static_cast <uint64_t> (::floor(static_cast <long double> (date - begin))));
								} break;
								// Если нам нужно получить количество оставшихся микросекунд
								case static_cast <uint8_t> (type_t::MICROSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::LEFT);
									// Увеличиваем размер количества миллисекунд до микросекунд
									result *= 1000;
								} break;
								// Если нам нужно получить количество оставшихся наносекунд
								case static_cast <uint8_t> (type_t::NANOSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::LEFT);
									// Увеличиваем размер количества миллисекунд до наносекунд
									result *= 1000000;
								} break;
							}
						} break;
						// Если нам нужно получить количество оставшего времени в минуте
						case static_cast <uint8_t> (type_t::MINUTES): {
							/**
							 * Определяем тип определяемых единиц измерений
							 */
							switch(static_cast <uint8_t> (value)){
								// Если нам нужно получить количество оставшихся секунд
								case static_cast <uint8_t> (type_t::SECONDS): {
									// Получаем начало текущего периода
									const uint64_t begin = this->begin(date, type);
									// Определяем количество оставшихся секунд
									result = static_cast <uint64_t> ((static_cast <uint64_t> (60000) - static_cast <uint64_t> (::floor(static_cast <long double> (date - begin)))) / 1000.);
								} break;
								// Если нам нужно получить количество оставшихся миллисекунд
								case static_cast <uint8_t> (type_t::MILLISECONDS): {
									// Получаем начало текущего периода
									const uint64_t begin = this->begin(date, type);
									// Определяем количество оставшихся миллисекунд
									result = static_cast <uint64_t> (static_cast <uint64_t> (60000) - static_cast <uint64_t> (::floor(static_cast <long double> (date - begin))));
								} break;
								// Если нам нужно получить количество оставшихся микросекунд
								case static_cast <uint8_t> (type_t::MICROSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::LEFT);
									// Увеличиваем размер количества миллисекунд до микросекунд
									result *= 1000;
								} break;
								// Если нам нужно получить количество оставшихся наносекунд
								case static_cast <uint8_t> (type_t::NANOSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::LEFT);
									// Увеличиваем размер количества миллисекунд до наносекунд
									result *= 1000000;
								} break;
							}
						} break;
						// Если нам нужно получить количество оставшего времени в секунде
						case static_cast <uint8_t> (type_t::SECONDS): {
							/**
							 * Определяем тип определяемых единиц измерений
							 */
							switch(static_cast <uint8_t> (value)){
								// Если нам нужно получить количество оставшихся миллисекунд
								case static_cast <uint8_t> (type_t::MILLISECONDS): {
									// Получаем начало текущего периода
									const uint64_t begin = this->begin(date, type);
									// Определяем количество оставшихся миллисекунд
									result = static_cast <uint64_t> (static_cast <uint64_t> (1000) - static_cast <uint64_t> (::floor(static_cast <long double> (date - begin))));
								} break;
								// Если нам нужно получить количество оставшихся микросекунд
								case static_cast <uint8_t> (type_t::MICROSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::LEFT);
									// Увеличиваем размер количества миллисекунд до микросекунд
									result *= 1000;
								} break;
								// Если нам нужно получить количество оставшихся наносекунд
								case static_cast <uint8_t> (type_t::NANOSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::LEFT);
									// Увеличиваем размер количества миллисекунд до наносекунд
									result *= 1000000;
								} break;
							}
						} break;
					}
				} break;
				// Если нужно определить сколько прошло времени
				case static_cast <uint8_t> (actual_t::PASSED): {
					/**
					 * Определяем тип единиц измерений
					 */
					switch(static_cast <uint8_t> (type)){
						// Если нам нужно получить количество прошедшего времени в году
						case static_cast <uint8_t> (type_t::YEAR): {
							/**
							 * Определяем тип определяемых единиц измерений
							 */
							switch(static_cast <uint8_t> (value)){
								// Если нам нужно получить количество прошедших месяцев
								case static_cast <uint8_t> (type_t::MONTH): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Получаем штамп времени начала года
									const uint64_t beginYear = this->beginOfYear(year);
									// Определяем сколько дней прошло с начала года
									const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
									{
										// Номер текущего месяца
										uint8_t month = 0;
										// Подсчитываем количество дней в предыдущих месяцах
										uint16_t count = 0, days = 0;
										// Устанавливаем флаг високосного года
										const bool leap = this->leap(year);
										/**
										 * Выполняем перебор всех дней месяца
										 */
										for(uint8_t i = 0; i < params.daysInMonths.size(); i++){
											// Увеличиваем номер месяца
											month = (i + 1);
											// Получаем текущее количество дней с компенсацией високосного года
											days = (static_cast <uint16_t> (params.daysInMonths[i]) + ((i == 1) && leap ? 1 : 0));
											// Если мы не дошли до предела
											if(lastDays >= (days + count))
												// Увеличиваем количество прошедших дней
												count += days;
											// Выходим из цикла
											else break;
										}
										// Формируем полученный результат
										result = static_cast <uint64_t> (month - 1);
									}
								} break;
								// Если нам нужно получить количество прошедших недель
								case static_cast <uint8_t> (type_t::WEEK): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Получаем штамп времени начала года
									const uint64_t beginYear = this->beginOfYear(year);
									// Получаем количество недель прошедших в году
									result = static_cast <uint64_t> (::round((date - beginYear) / 604800000.L));
								} break;
								// Если нам нужно получить количество прошедших дней
								case static_cast <uint8_t> (type_t::DAY): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Получаем штамп времени начала года
									const uint64_t beginYear = this->beginOfYear(year);
									// Определяем сколько дней прошло с начала года
									result = static_cast <uint64_t> (::floor((date - beginYear) / 86400000.L));
								} break;
								// Если нам нужно получить количество прошедших часов
								case static_cast <uint8_t> (type_t::HOUR): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Получаем штамп времени начала года
									const uint64_t beginYear = this->beginOfYear(year);
									// Получаем количество часов прошедших с начала года
									result = static_cast <uint64_t> (::floor((date - beginYear) / 3600000.L));
								} break;
								// Если нам нужно получить количество прошедших минут
								case static_cast <uint8_t> (type_t::MINUTES): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Получаем штамп времени начала года
									const uint64_t beginYear = this->beginOfYear(year);
									// Получаем количество минут прошедших с начала года
									result = static_cast <uint64_t> (::floor((date - beginYear) / 60000.));
								} break;
								// Если нам нужно получить количество прошедших секунд
								case static_cast <uint8_t> (type_t::SECONDS): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Получаем штамп времени начала года
									const uint64_t beginYear = this->beginOfYear(year);
									// Получаем количество секунд прошедших с начала года
									result = static_cast <uint64_t> (::floor((date - beginYear) / 1000.));
								} break;
								// Если нам нужно получить количество прошедших миллисекунд
								case static_cast <uint8_t> (type_t::MILLISECONDS): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Получаем штамп времени начала года
									const uint64_t beginYear = this->beginOfYear(year);
									// Получаем количество миллисекунд прошедших с начала года
									result = (date - beginYear);
								} break;
								// Если нам нужно получить количество прошедших микросекунд
								case static_cast <uint8_t> (type_t::MICROSECONDS): {
									// Получаем текущее значение размерности даты
									const uint8_t current = static_cast <uint8_t> (this->digits(date) - 1);
									// Получаем размерность актуальной размерности даты
									const uint8_t actual = static_cast <uint8_t> (this->digits(this->timestamp(type_t::MILLISECONDS)) - 1);
									// Если текущее значение даты передано в микросекундах
									if(current == (actual + 3)){
										// Получаем значение текущего года
										const uint16_t year = this->year(date);
										// Получаем штамп времени начала года в микросекундах
										const uint64_t beginYear = (this->beginOfYear(year) * static_cast <uint64_t> (1000));
										// Получаем количество микросекунд прошедших с начала года
										result = static_cast <uint64_t> (date - beginYear);
									// Если текущее значение даты передано в других единицах
									} else {
										// Устанавливаем текущее значение актуализации
										result = this->actual(date, type_t::MILLISECONDS, type, actual_t::PASSED);
										// Увеличиваем размер количества миллисекунд до микросекунд
										result *= 1000;
									}
								} break;
								// Если нам нужно получить количество прошедших наносекунд
								case static_cast <uint8_t> (type_t::NANOSECONDS): {
									// Получаем текущее значение размерности даты
									const uint8_t current = static_cast <uint8_t> (this->digits(date) - 1);
									// Получаем размерность актуальной размерности даты
									const uint8_t actual = static_cast <uint8_t> (this->digits(this->timestamp(type_t::MILLISECONDS)) - 1);
									// Если текущее значение даты передано в наносекундах
									if(current == (actual + 6)){
										// Получаем значение текущего года
										const uint16_t year = this->year(date);
										// Получаем штамп времени начала года в наносекундах
										const uint64_t beginYear = (this->beginOfYear(year) * static_cast <uint64_t> (1000000));
										// Получаем количество наносекунд прошедших с начала года
										result = static_cast <uint64_t> (date - beginYear);
									// Если текущее значение даты передано в других единицах
									} else {
										// Устанавливаем текущее значение актуализации
										result = this->actual(date, type_t::MILLISECONDS, type, actual_t::PASSED);
										// Увеличиваем размер количества миллисекунд до наносекунд
										result *= 1000000;
									}
								} break;
							}
						} break;
						// Если нам нужно получить количество прошедшего времени в месяце
						case static_cast <uint8_t> (type_t::MONTH): {
							/**
							 * Определяем тип определяемых единиц измерений
							 */
							switch(static_cast <uint8_t> (value)){
								// Если нам нужно получить количество прошедших недель
								case static_cast <uint8_t> (type_t::WEEK): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Получаем штамп времени начала года
									const uint64_t beginYear = this->beginOfYear(year);
									// Определяем сколько дней прошло с начала года
									const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
									{
										// Номер текущего месяца
										uint8_t month = 0;
										// Подсчитываем количество дней в предыдущих месяцах
										uint16_t count = 0, days = 0;
										// Устанавливаем флаг високосного года
										const bool leap = this->leap(year);
										/**
										 * Выполняем перебор всех дней месяца
										 */
										for(uint8_t i = 0; i < params.daysInMonths.size(); i++){
											// Увеличиваем номер месяца
											month = (i + 1);
											// Получаем текущее количество дней с компенсацией високосного года
											days = (static_cast <uint16_t> (params.daysInMonths[i]) + ((i == 1) && leap ? 1 : 0));
											// Если мы не дошли до предела
											if(lastDays >= (days + count))
												// Увеличиваем количество прошедших дней
												count += days;
											// Выходим из цикла
											else break;
										}
										// Получаем начало месяца указанной даты
										const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
										// Получаем количество прошедших недель в месяце
										result = static_cast <uint64_t> (::round((date - beginMonth) / 604800000.L));
									}
								} break;
								// Если нам нужно получить количество прошедших дней
								case static_cast <uint8_t> (type_t::DAY): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Получаем штамп времени начала года
									const uint64_t beginYear = this->beginOfYear(year);
									// Определяем сколько дней прошло с начала года
									const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
									{
										// Номер текущего месяца
										uint8_t month = 0;
										// Подсчитываем количество дней в предыдущих месяцах
										uint16_t count = 0, days = 0;
										// Устанавливаем флаг високосного года
										const bool leap = this->leap(year);
										/**
										 * Выполняем перебор всех дней месяца
										 */
										for(uint8_t i = 0; i < params.daysInMonths.size(); i++){
											// Увеличиваем номер месяца
											month = (i + 1);
											// Получаем текущее количество дней с компенсацией високосного года
											days = (static_cast <uint16_t> (params.daysInMonths[i]) + ((i == 1) && leap ? 1 : 0));
											// Если мы не дошли до предела
											if(lastDays >= (days + count))
												// Увеличиваем количество прошедших дней
												count += days;
											// Выходим из цикла
											else break;
										}
										// Получаем начало месяца указанной даты
										const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
										// Получаем количество прошедших дней в месяце
										result = static_cast <uint64_t> (::round((date - beginMonth) / 86400000.L) - 1);
									}
								} break;
								// Если нам нужно получить количество прошедших часов
								case static_cast <uint8_t> (type_t::HOUR): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Получаем штамп времени начала года
									const uint64_t beginYear = this->beginOfYear(year);
									// Определяем сколько дней прошло с начала года
									const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
									{
										// Номер текущего месяца
										uint8_t month = 0;
										// Подсчитываем количество дней в предыдущих месяцах
										uint16_t count = 0, days = 0;
										// Устанавливаем флаг високосного года
										const bool leap = this->leap(year);
										/**
										 * Выполняем перебор всех дней месяца
										 */
										for(uint8_t i = 0; i < params.daysInMonths.size(); i++){
											// Увеличиваем номер месяца
											month = (i + 1);
											// Получаем текущее количество дней с компенсацией високосного года
											days = (static_cast <uint16_t> (params.daysInMonths[i]) + ((i == 1) && leap ? 1 : 0));
											// Если мы не дошли до предела
											if(lastDays >= (days + count))
												// Увеличиваем количество прошедших дней
												count += days;
											// Выходим из цикла
											else break;
										}
										// Получаем начало месяца указанной даты
										const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
										// Получаем количество прошедших часов в месяце
										result = static_cast <uint64_t> (static_cast <uint32_t> (date - beginMonth) / 3600000);
									}
								} break;
								// Если нам нужно получить количество прошедших минут
								case static_cast <uint8_t> (type_t::MINUTES): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Получаем штамп времени начала года
									const uint64_t beginYear = this->beginOfYear(year);
									// Определяем сколько дней прошло с начала года
									const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
									{
										// Номер текущего месяца
										uint8_t month = 0;
										// Подсчитываем количество дней в предыдущих месяцах
										uint16_t count = 0, days = 0;
										// Устанавливаем флаг високосного года
										const bool leap = this->leap(year);
										/**
										 * Выполняем перебор всех дней месяца
										 */
										for(uint8_t i = 0; i < params.daysInMonths.size(); i++){
											// Увеличиваем номер месяца
											month = (i + 1);
											// Получаем текущее количество дней с компенсацией високосного года
											days = (static_cast <uint16_t> (params.daysInMonths[i]) + ((i == 1) && leap ? 1 : 0));
											// Если мы не дошли до предела
											if(lastDays >= (days + count))
												// Увеличиваем количество прошедших дней
												count += days;
											// Выходим из цикла
											else break;
										}
										// Получаем начало месяца указанной даты
										const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
										// Получаем количество прошедших минут в месяце
										result = static_cast <uint64_t> (static_cast <uint32_t> (date - beginMonth) / 60000);
									}
								} break;
								// Если нам нужно получить количество прошедших секунд
								case static_cast <uint8_t> (type_t::SECONDS): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Получаем штамп времени начала года
									const uint64_t beginYear = this->beginOfYear(year);
									// Определяем сколько дней прошло с начала года
									const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
									{
										// Номер текущего месяца
										uint8_t month = 0;
										// Подсчитываем количество дней в предыдущих месяцах
										uint16_t count = 0, days = 0;
										// Устанавливаем флаг високосного года
										const bool leap = this->leap(year);
										/**
										 * Выполняем перебор всех дней месяца
										 */
										for(uint8_t i = 0; i < params.daysInMonths.size(); i++){
											// Увеличиваем номер месяца
											month = (i + 1);
											// Получаем текущее количество дней с компенсацией високосного года
											days = (static_cast <uint16_t> (params.daysInMonths[i]) + ((i == 1) && leap ? 1 : 0));
											// Если мы не дошли до предела
											if(lastDays >= (days + count))
												// Увеличиваем количество прошедших дней
												count += days;
											// Выходим из цикла
											else break;
										}
										// Получаем начало месяца указанной даты
										const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
										// Получаем количество прошедших секунд в месяце
										result = static_cast <uint64_t> (static_cast <uint32_t> (date - beginMonth) / 1000);
									}
								} break;
								// Если нам нужно получить количество прошедших миллисекунд
								case static_cast <uint8_t> (type_t::MILLISECONDS): {
									// Получаем значение текущего года
									const uint16_t year = this->year(date);
									// Получаем штамп времени начала года
									const uint64_t beginYear = this->beginOfYear(year);
									// Определяем сколько дней прошло с начала года
									const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000.L));
									{
										// Номер текущего месяца
										uint8_t month = 0;
										// Подсчитываем количество дней в предыдущих месяцах
										uint16_t count = 0, days = 0;
										// Устанавливаем флаг високосного года
										const bool leap = this->leap(year);
										/**
										 * Выполняем перебор всех дней месяца
										 */
										for(uint8_t i = 0; i < params.daysInMonths.size(); i++){
											// Увеличиваем номер месяца
											month = (i + 1);
											// Получаем текущее количество дней с компенсацией високосного года
											days = (static_cast <uint16_t> (params.daysInMonths[i]) + ((i == 1) && leap ? 1 : 0));
											// Если мы не дошли до предела
											if(lastDays >= (days + count))
												// Увеличиваем количество прошедших дней
												count += days;
											// Выходим из цикла
											else break;
										}
										// Получаем начало месяца указанной даты
										const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000)));
										// Получаем количество прошедших миллисекунд в месяце
										result = static_cast <uint64_t> (date - beginMonth);
									}
								} break;
								// Если нам нужно получить количество прошедших микросекунд
								case static_cast <uint8_t> (type_t::MICROSECONDS): {
									// Получаем текущее значение размерности даты
									const uint8_t current = static_cast <uint8_t> (this->digits(date) - 1);
									// Получаем размерность актуальной размерности даты
									const uint8_t actual = static_cast <uint8_t> (this->digits(this->timestamp(type_t::MILLISECONDS)) - 1);
									// Если текущее значение даты передано в микросекундах
									if(current == (actual + 3)){
										// Получаем значение текущего года
										const uint16_t year = this->year(date);
										// Получаем штамп времени начала года в микросекундах
										const uint64_t beginYear = (this->beginOfYear(year) * static_cast <uint64_t> (1000));
										// Определяем сколько дней прошло с начала года
										const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000000.L));
										{
											// Номер текущего месяца
											uint8_t month = 0;
											// Подсчитываем количество дней в предыдущих месяцах
											uint16_t count = 0, days = 0;
											// Устанавливаем флаг високосного года
											const bool leap = this->leap(year);
											/**
											 * Выполняем перебор всех дней месяца
											 */
											for(uint8_t i = 0; i < params.daysInMonths.size(); i++){
												// Увеличиваем номер месяца
												month = (i + 1);
												// Получаем текущее количество дней с компенсацией високосного года
												days = (static_cast <uint16_t> (params.daysInMonths[i]) + ((i == 1) && leap ? 1 : 0));
												// Если мы не дошли до предела
												if(lastDays >= (days + count))
													// Увеличиваем количество прошедших дней
													count += days;
												// Выходим из цикла
												else break;
											}
											// Получаем начало месяца указанной даты
											const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000000)));
											// Получаем количество прошедших микросекунд в месяце
											result = static_cast <uint64_t> (date - beginMonth);
										}
									// Если текущее значение даты передано в других единицах
									} else {
										// Устанавливаем текущее значение актуализации
										result = this->actual(date, type_t::MILLISECONDS, type, actual_t::PASSED);
										// Увеличиваем размер количества миллисекунд до микросекунд
										result *= 1000;
									}
								} break;
								// Если нам нужно получить количество прошедших наносекунд
								case static_cast <uint8_t> (type_t::NANOSECONDS): {
									// Получаем текущее значение размерности даты
									const uint8_t current = static_cast <uint8_t> (this->digits(date) - 1);
									// Получаем размерность актуальной размерности даты
									const uint8_t actual = static_cast <uint8_t> (this->digits(this->timestamp(type_t::MILLISECONDS)) - 1);
									// Если текущее значение даты передано в наносекундах
									if(current == (actual + 6)){
										// Получаем значение текущего года
										const uint16_t year = this->year(date);
										// Получаем штамп времени начала года в наносекундах
										const uint64_t beginYear = (this->beginOfYear(year) * static_cast <uint64_t> (1000000));
										// Определяем сколько дней прошло с начала года
										const uint16_t lastDays = static_cast <uint16_t> (::floor((date - beginYear) / 86400000000000.L));
										{
											// Номер текущего месяца
											uint8_t month = 0;
											// Подсчитываем количество дней в предыдущих месяцах
											uint16_t count = 0, days = 0;
											// Устанавливаем флаг високосного года
											const bool leap = this->leap(year);
											/**
											 * Выполняем перебор всех дней месяца
											 */
											for(uint8_t i = 0; i < params.daysInMonths.size(); i++){
												// Увеличиваем номер месяца
												month = (i + 1);
												// Получаем текущее количество дней с компенсацией високосного года
												days = (static_cast <uint16_t> (params.daysInMonths[i]) + ((i == 1) && leap ? 1 : 0));
												// Если мы не дошли до предела
												if(lastDays >= (days + count))
													// Увеличиваем количество прошедших дней
													count += days;
												// Выходим из цикла
												else break;
											}
											// Получаем начало месяца указанной даты
											const uint64_t beginMonth = (beginYear + (static_cast <uint64_t> (count) * static_cast <uint64_t> (86400000000000)));
											// Получаем количество прошедших наносекунд в месяце
											result = static_cast <uint64_t> (date - beginMonth);
										}
									// Если текущее значение даты передано в других единицах
									} else {
										// Устанавливаем текущее значение актуализации
										result = this->actual(date, type_t::MILLISECONDS, type, actual_t::PASSED);
										// Увеличиваем размер количества миллисекунд до наносекунд
										result *= 1000000;
									}
								} break;
							}
						} break;
						// Если нам нужно получить количество прошедшего времени в неделе
						case static_cast <uint8_t> (type_t::WEEK): {
							/**
							 * Определяем тип определяемых единиц измерений
							 */
							switch(static_cast <uint8_t> (value)){
								// Если нам нужно получить количество прошедших дней
								case static_cast <uint8_t> (type_t::DAY): {
									// Получаем начало текущего периода
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших дней
									result = static_cast <uint64_t> (::floor(static_cast <long double> (date - begin)) / 86400000.L);
								} break;
								// Если нам нужно получить количество прошедших часов
								case static_cast <uint8_t> (type_t::HOUR): {
									// Получаем начало текущего периода
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших часов
									result = static_cast <uint64_t> (::floor(static_cast <long double> (date - begin)) / 3600000.L);
								} break;
								// Если нам нужно получить количество прошедших минут
								case static_cast <uint8_t> (type_t::MINUTES): {
									// Получаем начало текущего периода
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших минут
									result = static_cast <uint64_t> (::floor(static_cast <long double> (date - begin)) / 60000.);
								} break;
								// Если нам нужно получить количество прошедших секунд
								case static_cast <uint8_t> (type_t::SECONDS): {
									// Получаем начало текущего периода
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших секунд
									result = static_cast <uint64_t> (::floor(static_cast <long double> (date - begin)) / 1000.);
								} break;
								// Если нам нужно получить количество прошедших миллисекунд
								case static_cast <uint8_t> (type_t::MILLISECONDS): {
									// Получаем начало текущего периода
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших миллисекунд
									result = static_cast <uint64_t> (::floor(static_cast <long double> (date - begin)));
								} break;
								// Если нам нужно получить количество прошедших микросекунд
								case static_cast <uint8_t> (type_t::MICROSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::PASSED);
									// Увеличиваем размер количества миллисекунд до микросекунд
									result *= 1000;
								} break;
								// Если нам нужно получить количество прошедших наносекунд
								case static_cast <uint8_t> (type_t::NANOSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::PASSED);
									// Увеличиваем размер количества миллисекунд до наносекунд
									result *= 1000000;
								} break;
							}
						} break;
						// Если нам нужно получить количество прошедшего времени в дне
						case static_cast <uint8_t> (type_t::DAY): {
							/**
							 * Определяем тип определяемых единиц измерений
							 */
							switch(static_cast <uint8_t> (value)){
								// Если нам нужно получить количество прошедших часов
								case static_cast <uint8_t> (type_t::HOUR): {
									// Получаем начало текущего периода
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших часов
									result = static_cast <uint64_t> (::floor(static_cast <long double> (date - begin)) / 3600000.L);
								} break;
								// Если нам нужно получить количество прошедших минут
								case static_cast <uint8_t> (type_t::MINUTES): {
									// Получаем начало текущего периода
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших минут
									result = static_cast <uint64_t> (::floor(static_cast <long double> (date - begin)) / 60000.);
								} break;
								// Если нам нужно получить количество прошедших секунд
								case static_cast <uint8_t> (type_t::SECONDS): {
									// Получаем начало текущего периода
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших секунд
									result = static_cast <uint64_t> (::floor(static_cast <long double> (date - begin)) / 1000.);
								} break;
								// Если нам нужно получить количество прошедших миллисекунд
								case static_cast <uint8_t> (type_t::MILLISECONDS): {
									// Получаем начало текущего периода
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших миллисекунд
									result = static_cast <uint64_t> (::floor(static_cast <long double> (date - begin)));
								} break;
								// Если нам нужно получить количество прошедших микросекунд
								case static_cast <uint8_t> (type_t::MICROSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::PASSED);
									// Увеличиваем размер количества миллисекунд до микросекунд
									result *= 1000;
								} break;
								// Если нам нужно получить количество прошедших наносекунд
								case static_cast <uint8_t> (type_t::NANOSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::PASSED);
									// Увеличиваем размер количества миллисекунд до наносекунд
									result *= 1000000;
								} break;
							}
						} break;
						// Если нам нужно получить количество прошедшего времени в часе
						case static_cast <uint8_t> (type_t::HOUR): {
							/**
							 * Определяем тип определяемых единиц измерений
							 */
							switch(static_cast <uint8_t> (value)){
								// Если нам нужно получить количество прошедших минут
								case static_cast <uint8_t> (type_t::MINUTES): {
									// Получаем начало текущего периода
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших минут
									result = static_cast <uint64_t> (::floor(static_cast <long double> (date - begin)) / 60000.);
								} break;
								// Если нам нужно получить количество прошедших секунд
								case static_cast <uint8_t> (type_t::SECONDS): {
									// Получаем начало текущего периода
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших секунд
									result = static_cast <uint64_t> (::floor(static_cast <long double> (date - begin)) / 1000.);
								} break;
								// Если нам нужно получить количество прошедших миллисекунд
								case static_cast <uint8_t> (type_t::MILLISECONDS): {
									// Получаем начало текущего периода
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших миллисекунд
									result = static_cast <uint64_t> (::floor(static_cast <long double> (date - begin)));
								} break;
								// Если нам нужно получить количество прошедших микросекунд
								case static_cast <uint8_t> (type_t::MICROSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::PASSED);
									// Увеличиваем размер количества миллисекунд до микросекунд
									result *= 1000;
								} break;
								// Если нам нужно получить количество прошедших наносекунд
								case static_cast <uint8_t> (type_t::NANOSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::PASSED);
									// Увеличиваем размер количества миллисекунд до наносекунд
									result *= 1000000;
								} break;
							}
						} break;
						// Если нам нужно получить количество прошедшего времени в минуте
						case static_cast <uint8_t> (type_t::MINUTES): {
							/**
							 * Определяем тип определяемых единиц измерений
							 */
							switch(static_cast <uint8_t> (value)){
								// Если нам нужно получить количество прошедших секунд
								case static_cast <uint8_t> (type_t::SECONDS): {
									// Получаем начало текущего периода
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших секунд
									result = static_cast <uint64_t> (::floor(static_cast <long double> (date - begin)) / 1000.);
								} break;
								// Если нам нужно получить количество прошедших миллисекунд
								case static_cast <uint8_t> (type_t::MILLISECONDS): {
									// Получаем начало текущего периода
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших миллисекунд
									result = static_cast <uint64_t> (::floor(static_cast <long double> (date - begin)));
								} break;
								// Если нам нужно получить количество прошедших микросекунд
								case static_cast <uint8_t> (type_t::MICROSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::PASSED);
									// Увеличиваем размер количества миллисекунд до микросекунд
									result *= 1000;
								} break;
								// Если нам нужно получить количество прошедших наносекунд
								case static_cast <uint8_t> (type_t::NANOSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::PASSED);
									// Увеличиваем размер количества миллисекунд до наносекунд
									result *= 1000000;
								} break;
							}
						} break;
						// Если нам нужно получить количество прошедшего времени в секунде
						case static_cast <uint8_t> (type_t::SECONDS): {
							/**
							 * Определяем тип определяемых единиц измерений
							 */
							switch(static_cast <uint8_t> (value)){
								// Если нам нужно получить количество прошедших миллисекунд
								case static_cast <uint8_t> (type_t::MILLISECONDS): {
									// Получаем начало текущего периода
									const uint64_t begin = this->begin(date, type);
									// Определяем количество прошедших миллисекунд
									result = static_cast <uint64_t> (::floor(static_cast <long double> (date - begin)));
								} break;
								// Если нам нужно получить количество прошедших микросекунд
								case static_cast <uint8_t> (type_t::MICROSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::PASSED);
									// Увеличиваем размер количества миллисекунд до микросекунд
									result *= 1000;
								} break;
								// Если нам нужно получить количество прошедших наносекунд
								case static_cast <uint8_t> (type_t::NANOSECONDS): {
									// Устанавливаем текущее значение актуализации
									result = this->actual(date, type_t::MILLISECONDS, type, actual_t::PASSED);
									// Увеличиваем размер количества миллисекунд до наносекунд
									result *= 1000000;
								} break;
							}
						} break;
					}
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(date, static_cast <uint16_t> (value), static_cast <uint16_t> (type), static_cast <uint16_t> (actual)), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод актуализации прошедшего и оставшегося времени
 *
 * @param value   тип определяемых единиц измерений времени
 * @param type    тип единиц измерений даты
 * @param actual  направление актуализации
 * @param storage хранение значение времени
 * @return        результат вычисления
 *
 */
uint64_t awh::Chrono::actual(const type_t value, const type_t type, const actual_t actual, const storage_t storage) const noexcept {
	// Выполняем актуализацию текущей даты на указанное количество единиц времени
	return this->actual(this->timestamp(type_t::MILLISECONDS, storage), value, type, actual);
}
/**
 * @brief Метод смещения на указанное количество единиц времени
 *
 * @param date   дата относительно которой производится смещение
 * @param value  значение на которое производится смещение
 * @param type   тип единиц измерений даты
 * @param offset направление смещения
 * @return       результат вычисления в формате UnixTimestamp
 *
 */
uint64_t awh::Chrono::offset(const uint64_t date, const uint64_t value, const type_t type, const offset_t offset) const noexcept {
	// Переменная результата
	uint64_t result = 0;
	// Если дата передана
	if(date > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем направление смещения
			 */
			switch(static_cast <uint8_t> (offset)){
				// Если необходимо выполнить инкремент
				case static_cast <uint8_t> (offset_t::INCREMENT): {
					/**
					 * Определяем тип единиц измерений
					 */
					switch(static_cast <uint8_t> (type)){
						// Если нам нужно получить начало года
						case static_cast <uint8_t> (type_t::YEAR): {
							// Устанавливаем текущее значение даты
							result = date;
							// Создаём объект даты и времени
							dt_t dt;
							// Заполняем объект даты из переданного штампа времени
							this->makeDate(date, dt);
							// Определяем количество прошедших лет
							const uint16_t year = dt.year;
							// Для дат до марта 29-е февраля попадает в год начала интервала, иначе - в следующий год
							const uint8_t shift = (dt.month > 2 ? 1 : 0);
							/**
							 * Выполняем перебор всех лет
							 */
							for(size_t i = 0; i < static_cast <size_t> (value); i++){
								// Если год, в который попадает 29-е февраля прибавляемого интервала, високосный
								if(this->leap(static_cast <uint16_t> (year + i + shift)))
									// Увеличиваем текущее значение года на 366 дней
									result += 31622400000;
								// Увеличиваем текущее значение года на 365 дней
								else result += 31536000000;
							}
						} break;
						// Если нам нужно получить начало месяца
						case static_cast <uint8_t> (type_t::MONTH): {
							// Создаём объект даты и времени
							dt_t dt;
							// Заполняем объект даты из переданного штампа времени
							this->makeDate(date, dt);
							// Вычисляем суммарный порядковый номер месяца и прибавляем смещение
							const uint64_t total = ((static_cast <uint64_t> (dt.year) * 12) + (dt.month - 1) + value);
							// Определяем новый год и месяц
							dt.year = static_cast <uint16_t> (total / 12);
							dt.month = static_cast <uint8_t> ((total % 12) + 1);
							// Обновляем флаг високосного года
							dt.leap = this->leap(dt.year);
							// Получаем количество дней в новом месяце с учётом високосного года
							const uint8_t days = static_cast <uint8_t> (static_cast <uint16_t> (params.daysInMonths[dt.month - 1]) + (((dt.month == 2) && dt.leap) ? 1 : 0));
							// Если число месяца отсутствует в новом месяце, ограничиваем его последним днём
							if(dt.date > days)
								// Ограничиваем число последним днём месяца
								dt.date = days;
							// Собираем итоговый штамп времени
							result = this->makeDate(dt);
						} break;
						// Если нам нужно получить начало недели
						case static_cast <uint8_t> (type_t::WEEK):
							// Увеличиваем значение даты на указанное количество недель
							result = (date + (value * static_cast <uint64_t> (604800000)));
						break;
						// Если нам нужно получить начало дня
						case static_cast <uint8_t> (type_t::DAY):
							// Увеличиваем значение даты на указанное количество дней
							result = (date + (value * static_cast <uint64_t> (86400000)));
						break;
						// Если нам нужно получить начало часа
						case static_cast <uint8_t> (type_t::HOUR):
							// Увеличиваем значение даты на указанное количество часов
							result = (date + (value * static_cast <uint64_t> (3600000)));
						break;
						// Если нам нужно получить начало минуты
						case static_cast <uint8_t> (type_t::MINUTES):
							// Увеличиваем значение даты на указанное количество минут
							result = (date + (value * static_cast <uint64_t> (60000)));
						break;
						// Если нам нужно получить начало секунды
						case static_cast <uint8_t> (type_t::SECONDS):
							// Увеличиваем значение даты на указанное количество секунд
							result = (date + (value * static_cast <uint64_t> (1000)));
						break;
						// Если нам нужно получить начало миллисекунды
						case static_cast <uint8_t> (type_t::MILLISECONDS):
							// Увеличиваем значение даты на указанное количество миллисекунд
							result = (date + value);
						break;
						// Если нам нужно получить начало микросекунды
						case static_cast <uint8_t> (type_t::MICROSECONDS): {
							// Получаем текущее значение размерности даты
							const uint8_t current = static_cast <uint8_t> (this->digits(date) - 1);
							// Получаем размерность актуальной размерности даты
							const uint8_t actual = static_cast <uint8_t> (this->digits(this->timestamp(type_t::MILLISECONDS)) - 1);
							// Если текущее значение даты передано в микросекундах
							if(current == (actual + 3))
								// Увеличиваем значение даты на указанное количество микросекунд
								result = (date + value);
							// Если текущее значение даты передано в других единицах
							else {
								// Устанавливаем текущее значение даты
								result = date;
								// Увеличиваем размер даты на указанное количество микросекунд
								result *= 1000;
								// Увеличиваем значение даты на указанное количество микросекунд
								result += value;
							}
						} break;
						// Если нам нужно получить начало наносекунды
						case static_cast <uint8_t> (type_t::NANOSECONDS): {
							// Получаем текущее значение размерности даты
							const uint8_t current = static_cast <uint8_t> (this->digits(date) - 1);
							// Получаем размерность актуальной размерности даты
							const uint8_t actual = static_cast <uint8_t> (this->digits(this->timestamp(type_t::MILLISECONDS)) - 1);
							// Если текущее значение даты передано в наносекундах
							if(current == static_cast <uint8_t> (actual + 6))
								// Увеличиваем значение даты на указанное количество наносекунд
								result = (date + value);
							// Если текущее значение даты передано в других единицах
							else {
								// Устанавливаем текущее значение даты
								result = date;
								// Увеличиваем размер даты на указанное количество наносекунд
								result *= 1000000;
								// Увеличиваем значение даты на указанное количество наносекунд
								result += value;
							}
						} break;
					}
				} break;
				// Если необходимо выполнить декремент
				case static_cast <uint8_t> (offset_t::DECREMENT): {
					/**
					 * Определяем тип единиц измерений
					 */
					switch(static_cast <uint8_t> (type)){
						// Если нам нужно получить начало года
						case static_cast <uint8_t> (type_t::YEAR): {
							// Устанавливаем текущее значение даты
							result = date;
							// Создаём объект даты и времени
							dt_t dt;
							// Заполняем объект даты из переданного штампа времени
							this->makeDate(date, dt);
							// Определяем количество прошедших лет
							const uint16_t year = dt.year;
							// Для дат до марта 29-е февраля попадает в предыдущий год, иначе - в текущий год
							const uint8_t shift = (dt.month > 2 ? 0 : 1);
							/**
							 * Выполняем перебор всех лет
							 */
							for(size_t i = 0; i < static_cast <size_t> (value); i++){
								// Если год, в который попадает 29-е февраля вычитаемого интервала, високосный
								if(this->leap(static_cast <uint16_t> (year - i - shift)))
									// Уменьшаем текущее значение года на 366 дней
									result -= (result >= 31622400000 ? 31622400000 : 0);
								// Уменьшаем текущее значение года на 365 дней
								else result -= (result >= 31536000000 ? 31536000000 : 0);
							}
						} break;
						// Если нам нужно получить начало месяца
						case static_cast <uint8_t> (type_t::MONTH): {
							// Создаём объект даты и времени
							dt_t dt;
							// Заполняем объект даты из переданного штампа времени
							this->makeDate(date, dt);
							// Вычисляем суммарный порядковый номер месяца
							const uint64_t base = ((static_cast <uint64_t> (dt.year) * 12) + (dt.month - 1));
							// Если смещение не выходит за пределы эпохи
							if(value <= base){
								// Вычитаем смещение из порядкового номера месяца
								const uint64_t total = (base - value);
								// Определяем новый год и месяц
								dt.year = static_cast <uint16_t> (total / 12);
								dt.month = static_cast <uint8_t> ((total % 12) + 1);
								// Обновляем флаг високосного года
								dt.leap = this->leap(dt.year);
								// Получаем количество дней в новом месяце с учётом високосного года
								const uint8_t days = static_cast <uint8_t> (static_cast <uint16_t> (params.daysInMonths[dt.month - 1]) + (((dt.month == 2) && dt.leap) ? 1 : 0));
								// Если число месяца отсутствует в новом месяце, ограничиваем его последним днём
								if(dt.date > days)
									// Ограничиваем число последним днём месяца
									dt.date = days;
								// Собираем итоговый штамп времени
								result = this->makeDate(dt);
							}
						} break;
						// Если нам нужно получить начало недели
						case static_cast <uint8_t> (type_t::WEEK): {
							// Определяем количество недель, не выходящее за пределы даты
							const uint64_t count = (value < (date / 604800000) ? value : (date / 604800000));
							// Уменьшаем значение даты на указанное количество недель
							result = (date - (count * static_cast <uint64_t> (604800000)));
						} break;
						// Если нам нужно получить начало дня
						case static_cast <uint8_t> (type_t::DAY): {
							// Определяем количество дней, не выходящее за пределы даты
							const uint64_t count = (value < (date / 86400000) ? value : (date / 86400000));
							// Уменьшаем значение даты на указанное количество дней
							result = (date - (count * static_cast <uint64_t> (86400000)));
						} break;
						// Если нам нужно получить начало часа
						case static_cast <uint8_t> (type_t::HOUR): {
							// Определяем количество часов, не выходящее за пределы даты
							const uint64_t count = (value < (date / 3600000) ? value : (date / 3600000));
							// Уменьшаем значение даты на указанное количество часов
							result = (date - (count * static_cast <uint64_t> (3600000)));
						} break;
						// Если нам нужно получить начало минуты
						case static_cast <uint8_t> (type_t::MINUTES): {
							// Определяем количество минут, не выходящее за пределы даты
							const uint64_t count = (value < (date / 60000) ? value : (date / 60000));
							// Уменьшаем значение даты на указанное количество минут
							result = (date - (count * static_cast <uint64_t> (60000)));
						} break;
						// Если нам нужно получить начало секунды
						case static_cast <uint8_t> (type_t::SECONDS): {
							// Определяем количество секунд, не выходящее за пределы даты
							const uint64_t count = (value < (date / 1000) ? value : (date / 1000));
							// Уменьшаем значение даты на указанное количество секунд
							result = (date - (count * static_cast <uint64_t> (1000)));
						} break;
						// Если нам нужно получить начало миллисекунды
						case static_cast <uint8_t> (type_t::MILLISECONDS):
							// Уменьшаем значение даты на указанное количество миллисекунд
							result = (date >= value ? (date - value) : 0);
						break;
						// Если нам нужно получить начало микросекунды
						case static_cast <uint8_t> (type_t::MICROSECONDS): {
							// Получаем текущее значение размерности даты
							const uint8_t current = static_cast <uint8_t> (this->digits(date) - 1);
							// Получаем размерность актуальной размерности даты
							const uint8_t actual = static_cast <uint8_t> (this->digits(this->timestamp(type_t::MILLISECONDS)) - 1);
							// Если текущее значение даты передано в микросекундах
							if(current == (actual + 3))
								// Уменьшаем значение даты на указанное количество микросекунд
								result = (date >= value ? (date - value) : 0);
							// Если текущее значение даты передано в других единицах
							else {
								// Устанавливаем текущее значение даты
								result = date;
								// Увеличиваем размер даты на указанное количество микросекунд
								result *= 1000;
								// Уменьшаем значение даты на указанное количество микросекунд
								result -= (result >= value ? value : 0);
							}
						} break;
						// Если нам нужно получить начало наносекунды
						case static_cast <uint8_t> (type_t::NANOSECONDS): {
							// Получаем текущее значение размерности даты
							const uint8_t current = static_cast <uint8_t> (this->digits(date) - 1);
							// Получаем размерность актуальной размерности даты
							const uint8_t actual = static_cast <uint8_t> (this->digits(this->timestamp(type_t::MILLISECONDS)) - 1);
							// Если текущее значение даты передано в наносекундах
							if(current == (actual + 6))
								// Уменьшаем значение даты на указанное количество наносекунд
								result = (date >= value ? (date - value) : 0);
							// Если текущее значение даты передано в других единицах
							else {
								// Устанавливаем текущее значение даты
								result = date;
								// Увеличиваем размер даты на указанное количество наносекунд
								result *= 1000000;
								// Уменьшаем значение даты на указанное количество наносекунд
								result -= (result >= value ? value : 0);
							}
						} break;
					}
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(date, value, static_cast <uint16_t> (type), static_cast <uint16_t> (offset)), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод смещения текущей даты на указанное количество единиц времени
 *
 * @param value   значение на которое производится смещение
 * @param type    тип единиц измерений даты
 * @param offset  направление смещения
 * @param storage хранение значение времени
 * @return        результат вычисления в формате UnixTimestamp
 *
 */
uint64_t awh::Chrono::offset(const uint64_t value, const type_t type, const offset_t offset, const storage_t storage) const noexcept {
	// Выполняем смещение текущей даты на указанное количество единиц времени
	return this->offset(this->timestamp(type_t::MILLISECONDS, storage), value, type, offset);
}
/**
 * @brief Метод получения текстового значения времени
 *
 * @param seconds количество секунд для конвертации
 * @return        обозначение времени с указанием размерности
 *
 */
string awh::Chrono::seconds(const double seconds) const noexcept {
	// Переменная результата
	string result = "0s";
	// Если количество секунд передано
	if(seconds > 0.){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Шаблон минуты
			const double minute = 60.;
			// Шаблон часа
			const double hour = 3600.;
			// Шаблон дня
			const double day = 86400.;
			// Шаблон недели
			const double week = 604800.;
			// Шаблон месяца (средняя длительность месяца по григорианскому календарю)
			const double month = 2629746.;
			// Шаблон года
			const double year = 31536000.;
			// Если переданное значение соответствует году
			if(seconds >= year){
				// Выполняем преобразование в количество лет
				result = this->_fmk->noexp(seconds / year, true);
				// Добавляем наименование единицы измерения
				result.append(1, 'y');
			// Если переданное значение соответствует месяцу
			} else if((seconds >= month) && (seconds < year)) {
				// Выполняем преобразование в количество месяцев
				result = this->_fmk->noexp(seconds / month, true);
				// Добавляем наименование единицы измерения
				result.append(1, 'M');
			// Если переданное значение соответствует недели
			} else if((seconds >= week) && (seconds < month)) {
				// Выполняем преобразование в количество недель
				result = this->_fmk->noexp(seconds / week, true);
				// Добавляем наименование единицы измерения
				result.append(1, 'w');
			// Если переданное значение соответствует дням
			} else if((seconds >= day) && (seconds < week)) {
				// Выполняем преобразование в количество дней
				result = this->_fmk->noexp(seconds / day, true);
				// Добавляем наименование единицы измерения
				result.append(1, 'd');
			// Если переданное значение соответствует часам
			} else if((seconds >= hour) && (seconds < day)) {
				// Выполняем преобразование в количество часов
				result = this->_fmk->noexp(seconds / hour, true);
				// Добавляем наименование единицы измерения
				result.append(1, 'h');
			// Если переданное значение соответствует минут
			} else if((seconds >= minute) && (seconds < hour)) {
				// Выполняем преобразование в количество минут
				result = this->_fmk->noexp(seconds / minute, true);
				// Добавляем наименование единицы измерения
				result.append(1, 'm');
			// Если переданное значение соответствует секундам
			} else {
				// Выполняем преобразование в количество секунд
				result = this->_fmk->noexp(seconds, true);
				// Добавляем наименование единицы измерения
				result.append(1, 's');
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(seconds), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод получения размера в секундах из строки
 *
 * @param value строка обозначения размерности (s, m, h, d, w, M, y)
 * @return      размер в секундах
 *
 */
double awh::Chrono::seconds(string_view value) const noexcept {
	// Количество секунд
	double result = 0.;
	// Если строка с секундами передана
	if(!value.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем нативный разбор строки размерности времени
			const vector <match_t> match = ::parseSeconds(value.data(), value.size());
			// Если совпадение получено
			if(!match.empty()){
				// Обрабатываем полученные группы совпадения
				{
					// Обозначение размерности числа
					string label = "";
					// Размерность времени и размерность секунд
					double dimension = 1., seconds = 0.;
					/**
					 * Выполняем перебор всех полученных вариантов
					 */
					for(uint8_t j = 1; j < static_cast <uint8_t> (match.size()); j++){
						// Если результат получен
						if(match[j].end > match[j].begin){
							/**
							 * Определяем номер найденного элемента
							 */
							switch(j){
								// Если мы получили само число
								case 1:
									// Получаем значение числа
									seconds = this->_fmk->atoi <double> (value.data() + match[j].begin, match[j].end - match[j].begin);
								break;
								// Если мы получили размерность числа
								case 2: {
									// Получаем обозначение размерности числа
									label.assign(value.data() + match[j].begin, match[j].end - match[j].begin);
									// Если мы получили секунды
									if(label.front() == 's')
										// Выполняем установку множителя
										dimension = 1.;
									// Если мы получили минуты
									else if(label.front() == 'm')
										// Выполняем установку множителя
										dimension = 60.;
									// Если мы получили часы
									else if(label.front() == 'h')
										// Выполняем установку множителя
										dimension = 3600.;
									// Если мы получили дни
									else if(label.front() == 'd')
										// Выполняем установку множителя
										dimension = 86400.;
									// Если мы получили недели
									else if(label.front() == 'w')
										// Выполняем установку множителя
										dimension = 604800.;
									// Если мы получили месяцы
									else if(label.front() == 'M')
										// Выполняем установку множителя
										dimension = 2629746.;
									// Если мы получили годы
									else if(label.front() == 'y')
										// Выполняем установку множителя
										dimension = 31536000.;
								} break;
							}
						}
					}
					// Выполняем получение количества секунд
					result = (seconds * dimension);
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(value), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод извлечения статуса 12-и часового формата времени
 *
 * @param date дата для проверки
 *
 */
awh::Chrono::h12_t awh::Chrono::h12(const uint64_t date) const noexcept {
	// Если дата передана
	if(date > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаем структуру времени
			dt_t dt;
			// Заполняем объект даты из штампа времени
			this->makeDate(date, dt);
			// Получаем текущий статус 12-и часового формата времени
			return dt.h12;
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(date), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return h12_t::AM;
}
/**
 * @brief Метод извлечения текущего статуса 12-и часового формата времени
 *
 * @param storage хранение значение времени
 * @return        текущее установленное значение статуса 12-и часового формата времени
 *
 */
awh::Chrono::h12_t awh::Chrono::h12(const storage_t storage) const noexcept {
	// Переменная результата
	h12_t result = h12_t::AM;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем хранилище значение времени
		 */
		switch(static_cast <uint8_t> (storage)){
			// Если хранилище локальное
			case static_cast <uint8_t> (storage_t::LOCAL):
				// Получаем текущий статус 12-и часового формата времени
				result = this->_dt.h12;
			break;
			// Если хранилище глобальное
			case static_cast <uint8_t> (storage_t::GLOBAL):
				// Выполняем извлечение текущего статуса 12-и часового формата времени
				result = this->h12(this->timestamp(type_t::MILLISECONDS, storage));
			break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (storage)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод извлечения значения года
 *
 * @param date дата для извлечения года
 *
 */
uint16_t awh::Chrono::year(const uint64_t date) const noexcept {
	// Переменная результата
	uint16_t result = 0;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем извлечение значения года из даты
		result = static_cast <uint16_t> (
			::floor(
				(
					date - (
						static_cast <uint64_t> (
							::ceil(date / 126489600000.L)
						) * static_cast <uint64_t> (86400000)
					)
				) / 31536000000.L
			)
		);
		// Определяем количество прошедших високосных лет
		const uint16_t leaps = this->leapYears(result);
		// Получаем штамп времени начала года
		const uint64_t begin = (
			(static_cast <uint64_t> (leaps) * static_cast <uint64_t> (31622400000)) +
			(static_cast <uint64_t> (result - leaps) * static_cast <uint64_t> (31536000000))
		);
		// Формируем итоговый результат
		result += 1970;
		// Если прошло больше года с начала года
		if((date - begin) >= (this->leap(result) ? 31622400000 : 31536000000))
			// Увеличиваем полученный результат
			result++;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(date), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод получение текущего значения года
 *
 * @param storage хранение значение времени
 * @return        текущее значение года
 *
 */
uint16_t awh::Chrono::year(const storage_t storage) const noexcept {
	// Переменная результата
	uint16_t result = 0;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем хранилище значение времени
		 */
		switch(static_cast <uint8_t> (storage)){
			// Если хранилище локальное
			case static_cast <uint8_t> (storage_t::LOCAL):
				// Получаем установленное значение года
				result = this->_dt.year;
			break;
			// Если хранилище глобальное
			case static_cast <uint8_t> (storage_t::GLOBAL):
				// Выполняем извлечение текущее значение года
				result = this->year(this->timestamp(type_t::MILLISECONDS, storage));
			break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (storage)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод проверки действует ли на дату летнее время (DST)
 *
 * @param date дата для проверки
 * @return     результат проверки
 *
 */
bool awh::Chrono::dst(const uint64_t date) const noexcept {
	// Если дата передана
	if(date > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём объект даты и времени
			dt_t dt;
			// Заполняем объект даты из переданного штампа времени
			this->makeDate(date, dt);
			// Возвращаем рассчитанный флаг летнего времени (DST)
			return dt.dst;
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(date), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод проверки действует ли летнее время (DST)
 *
 * @param storage хранение значение времени
 * @return        результат проверки
 *
 */
bool awh::Chrono::dst(const storage_t storage) const noexcept {
	// Выполняем проверку действия летнего времени
	return this->dst(this->timestamp(type_t::MILLISECONDS, storage));
}
/**
 * @brief Метод проверки является ли год високосным
 *
 * @param year год для проверки
 * @return     результат проверки
 *
 */
bool awh::Chrono::leap(const uint16_t year) const noexcept {
	// Если дата передана
	if(year > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Устанавливаем флаг високосного года
			return (((year % 4) == 0) && (((year % 100) != 0) || ((year % 400) == 0)));
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(year), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод проверки является ли год високосным
 *
 * @param date дата для проверки
 * @return     результат проверки
 *
 */
bool awh::Chrono::leap(const uint64_t date) const noexcept {
	// Если дата передана
	if(date > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Устанавливаем флаг високосного года
			return this->leap(this->year(date));
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(date), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод проверки является ли текущий год високосным
 *
 * @param storage хранение значение времени
 * @return        результат проверки
 *
 */
bool awh::Chrono::leap(const storage_t storage) const noexcept {
	// Выполняем проверку является ли текущий год високосным
	return this->leap(this->timestamp(type_t::MILLISECONDS, storage));
}
/**
 * @brief Шаблон метода установки данных даты и времени
 *
 * @tparam T тип данных в котором устанавливаются данные
 *
 */
template <typename T>
/**
 * @brief Метод установки данных даты и времени
 *
 * @param date дата для обработки
 * @param unit элементы данных для установки
 *
 */
void awh::Chrono::set(const T date, const unit_t unit) noexcept {
	// Выполняем установку данных
	this->set(&date, sizeof(date), unit, is_class_v <T>);
}
/**
 * Объявляем прототипы для метода установки данных даты и времени
 */
template void awh::Chrono::set(const int8_t, const unit_t) noexcept;
template void awh::Chrono::set(const uint8_t, const unit_t) noexcept;
template void awh::Chrono::set(const int16_t, const unit_t) noexcept;
template void awh::Chrono::set(const uint16_t, const unit_t) noexcept;
template void awh::Chrono::set(const int32_t, const unit_t) noexcept;
template void awh::Chrono::set(const uint32_t, const unit_t) noexcept;
template void awh::Chrono::set(const int64_t, const unit_t) noexcept;
template void awh::Chrono::set(const uint64_t, const unit_t) noexcept;
template void awh::Chrono::set(const float, const unit_t) noexcept;
template void awh::Chrono::set(const double, const unit_t) noexcept;
template void awh::Chrono::set(const string, const unit_t) noexcept;
/**
 * Если операционной системой является macOS или Linux
 */
#if __APPLE__ || __MACH__ || __Linux__
	template void awh::Chrono::set(const size_t, const unit_t) noexcept;
	template void awh::Chrono::set(const ssize_t, const unit_t) noexcept;
#endif
/**
 * @brief Метод установки данных даты и времени
 *
 * @param buffer бинарный буфер данных
 * @param size   размер бинарного буфера
 * @param unit   элементы данных для установки
 * @param text   данные переданы в виде текста
 *
 */
void awh::Chrono::set(const void * buffer, const size_t size, const unit_t unit, const bool text) noexcept {
	// Если данные переданы правильно
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем блокировку потока
			const locker_t <> lock(this->_mtx.date);
			/**
			 * Определяем элементы устанавливаемых данных
			 */
			switch(static_cast <uint8_t> (unit)){
				// Если требуется установить номер текущего дня недели от 1 до 7
				case static_cast <uint8_t> (unit_t::DAY): {
					// Если данные переданы в виде текста
					if(text){
						// Получаем номер дня недели
						const string * day = reinterpret_cast <const string *> (buffer);
						// Если номер дня недели передан
						if(!day->empty()){
							// Если день передан в виде числа
							if(this->_fmk->is(* day, fmk_t::check_t::NUMBER)){
								// День для установки
								const uint8_t num = this->_fmk->atoi <uint8_t> (day->c_str(), day->size());
								// Если номер дня недели передан
								if((num > 0) && (num < 8))
									// Устанавливаем номер дня недели
									this->_dt.day = num;
							// Если день передан в виде названия
							} else {
								/**
								 * Выполняем перебор всего списка дней недели
								 */
								for(size_t i = 0; i < params.nameDays.size(); i++){
									// Получаем название дня
									const auto & name = params.nameDays[i];
									// Если мы нашли нужный нам день недели
									if(this->_fmk->compare(* day, name.first) || this->_fmk->compare(* day, name.second)){
										// Выполняем установку номера дня недели
										this->_dt.day = static_cast <uint8_t> (i + 1);
										// Выходим из цикла
										break;
									}
								}
							}
						}
					// Если данные переданы в виде числа
					} else {
						// Если устанавливаемые данные достаточны
						if(size >= sizeof(uint8_t)){
							// Номер текущего дня недели
							uint8_t day = 0;
							// Выполняем получение номера текущего дня недели
							::memcpy(&day, buffer, sizeof(day));
							// Если номер дня недели передан
							if((day > 0) && (day < 8))
								// Устанавливаем номер дня недели
								this->_dt.day = day;
						}
					}
				} break;
				// Если требуется установить число месяца от 1 до 31
				case static_cast <uint8_t> (unit_t::DATE): {
					// Если данные переданы в виде текста
					if(text){
						// Дата для установки
						const uint8_t date = this->_fmk->atoi <uint8_t> (
							reinterpret_cast <const string *> (buffer)->c_str(),
							reinterpret_cast <const string *> (buffer)->length()
						);
						// Если дата передана в нужном виде
						if((date > 0) && (date < 32))
							// Устанавливаем дату
							this->_dt.date = date;
					// Если данные переданы в виде числа
					} else {
						// Если устанавливаемые данные достаточны
						if(size >= sizeof(uint8_t)){
							// Дата для установки
							uint8_t date = 0;
							// Выполняем получение даты
							::memcpy(&date, buffer, sizeof(date));
							// Если дата передана в нужном виде
							if((date > 0) && (date < 32))
								// Устанавливаем дату
								this->_dt.date = date;
						}
					}
				} break;
				// Если требуется установить полное обозначение года
				case static_cast <uint8_t> (unit_t::YEAR): {
					// Если данные переданы в виде текста
					if(text){
						// Год для установки
						const uint16_t year = this->_fmk->atoi <uint16_t> (
							reinterpret_cast <const string *> (buffer)->c_str(),
							reinterpret_cast <const string *> (buffer)->length()
						);
						// Если год передан
						if(year > 0){
							// Устанавливаем год
							this->_dt.year = year;
							// Устанавливаем флаг високосного года
							this->_dt.leap = this->leap(this->_dt.year);
						}
					// Если данные переданы в виде числа
					} else {
						// Если устанавливаемые данные достаточны
						if(size >= sizeof(uint16_t)){
							// Год для установки
							uint16_t year = 0;
							// Выполняем получение года
							::memcpy(&year, buffer, sizeof(year));
							// Если год передан
							if(year > 0){
								// Устанавливаем год
								this->_dt.year = year;
								// Устанавливаем флаг високосного года
								this->_dt.leap = this->leap(this->_dt.year);
							}
						}
					}
				} break;
				// Если требуется установить количество часов от 0 до 23
				case static_cast <uint8_t> (unit_t::HOUR): {
					// Если данные переданы в виде текста
					if(text){
						// Час времени для установки
						const uint8_t hour = this->_fmk->atoi <uint8_t> (
							reinterpret_cast <const string *> (buffer)->c_str(),
							reinterpret_cast <const string *> (buffer)->length()
						);
						// Если количество часов передано
						if(hour < 24)
							// Устанавливаем количество часов
							this->_dt.hour = hour;
					// Если данные переданы в виде числа
					} else {
						// Если устанавливаемые данные достаточны
						if(size >= sizeof(uint8_t)){
							// Час времени для установки
							uint8_t hour = 0;
							// Выполняем получение часа
							::memcpy(&hour, buffer, sizeof(hour));
							// Если количество часов передано
							if(hour < 24)
								// Устанавливаем количество часов
								this->_dt.hour = hour;
						}
					}
				} break;
				// Если требуется установить количество прошедших дней от 1 января
				case static_cast <uint8_t> (unit_t::DAYS): {
					// Если данные переданы в виде текста
					if(text){
						// Количество прошедших дней для установки
						const uint16_t days = this->_fmk->atoi <uint16_t> (
							reinterpret_cast <const string *> (buffer)->c_str(),
							reinterpret_cast <const string *> (buffer)->length()
						);
						// Если количество прошедших дней от 1 января
						if(days < 366)
							// Устанавливаем количество прошедших дней
							this->_dt.days = days;
					// Если данные переданы в виде числа
					} else {
						// Если устанавливаемые данные достаточны
						if(size >= sizeof(uint16_t)){
							// Количество прошедших дней для установки
							uint16_t days = 0;
							// Выполняем получение количество прошедших дней
							::memcpy(&days, buffer, sizeof(days));
							// Если количество прошедших дней от 1 января
							if(days < 366)
								// Устанавливаем количество прошедших дней
								this->_dt.days = days;
						}
					}
				} break;
				// Если требуется установить номер месяца от 1 до 12 (начиная с Января)
				case static_cast <uint8_t> (unit_t::MONTH): {
					// Если данные переданы в виде текста
					if(text){
						// Получаем название месяца
						const string * month = reinterpret_cast <const string *> (buffer);
						// Если месяц передан
						if(!month->empty()){
							// Если месяц передан в виде числа
							if(this->_fmk->is(* month, fmk_t::check_t::NUMBER)){
								// Месяц для установки
								const uint8_t num = this->_fmk->atoi <uint8_t> (month->c_str(), month->length());
								// Если месяц передан
								if((num > 0) && (num < 13))
									// Устанавливаем месяц
									this->_dt.month = num;
							// Если день передан в виде названия
							} else {
								/**
								 * Выполняем перебор всего списка месяцев
								 */
								for(size_t i = 0; i < params.nameMonths.size(); i++){
									// Получаем название месяца
									const auto & name = params.nameMonths[i];
									// Если мы нашли нужный нам месяц
									if(this->_fmk->compare(* month, name.first) || this->_fmk->compare(* month, name.second)){
										// Устанавливаем месяц
										this->_dt.month = static_cast <uint8_t> (i + 1);
										// Выходим из цикла
										break;
									}
								}
							}
						}
					// Если данные переданы в виде числа
					} else {
						// Если устанавливаемые данные достаточны
						if(size >= sizeof(uint8_t)){
							// Номер месяца для установки
							uint8_t month = 0;
							// Выполняем получение номера месяца
							::memcpy(&month, buffer, sizeof(month));
							// Если месяц передан
							if((month > 0) && (month < 13))
								// Устанавливаем месяц
								this->_dt.month = month;
						}
					}
				} break;
				// Если требуется установить количество недель прошедших с начала года
				case static_cast <uint8_t> (unit_t::WEEKS): {
					// Если данные переданы в виде текста
					if(text){
						// Количество недель для установки
						const uint8_t weeks = this->_fmk->atoi <uint8_t> (
							reinterpret_cast <const string *> (buffer)->c_str(),
							reinterpret_cast <const string *> (buffer)->length()
						);
						// Если количество недель прошедших с начала года
						if(weeks < 53)
							// Устанавливаем количество недель прошедших с начала года
							this->_dt.weeks = weeks;
					// Если данные переданы в виде числа
					} else {
						// Если устанавливаемые данные достаточны
						if(size >= sizeof(uint8_t)){
							// Количество недель для установки
							uint8_t weeks = 0;
							// Выполняем получение количества недель
							::memcpy(&weeks, buffer, sizeof(weeks));
							// Если количество недель прошедших с начала года
							if(weeks < 53)
								// Устанавливаем количество недель прошедших с начала года
								this->_dt.weeks = weeks;
						}
					}
				} break;
				// Если требуется установить количество смещение временной зоны в секундах относительно UTC
				case static_cast <uint8_t> (unit_t::OFFSET): {
					// Если данные переданы в виде текста
					if(text)
						// Устанавливаем смещение временной зоны в секундах относительно UTC
						this->_dt.offset = this->_fmk->atoi <int32_t> (
							reinterpret_cast <const string *> (buffer)->c_str(),
							reinterpret_cast <const string *> (buffer)->length()
						);
					// Если данные переданы в виде числа
					else {
						// Если устанавливаемые данные достаточны
						if(size >= sizeof(int32_t)){
							// Смещение временной зоны в секундах относительно UTC
							int32_t offset = 0;
							// Выполняем получение смещения временной зоны в секундах относительно UTC
							::memcpy(&offset, buffer, sizeof(offset));
							// Устанавливаем смещение временной зоны в секундах относительно UTC
							this->_dt.offset = offset;
						}
					}
				} break;
				// Если требуется установить количество минут от 0 до 59
				case static_cast <uint8_t> (unit_t::MINUTES): {
					// Если данные переданы в виде текста
					if(text){
						// Количество минут для установки
						const uint8_t minutes = this->_fmk->atoi <uint8_t> (
							reinterpret_cast <const string *> (buffer)->c_str(),
							reinterpret_cast <const string *> (buffer)->length()
						);
						// Если количество минут передано
						if(minutes < 60)
							// Устанавливаем количество минут
							this->_dt.minutes = minutes;
					// Если данные переданы в виде числа
					} else {
						// Если устанавливаемые данные достаточны
						if(size >= sizeof(uint8_t)){
							// Количество минут для установки
							uint8_t minutes = 0;
							// Выполняем получение минут
							::memcpy(&minutes, buffer, sizeof(minutes));
							// Если количество минут передано
							if(minutes < 60)
								// Устанавливаем количество минут
								this->_dt.minutes = minutes;
						}
					}
				} break;
				// Если требуется установить количество секунд от 0 до 59
				case static_cast <uint8_t> (unit_t::SECONDS): {
					// Если данные переданы в виде текста
					if(text){
						// Количество секунд для установки
						const uint8_t seconds = this->_fmk->atoi <uint8_t> (
							reinterpret_cast <const string *> (buffer)->c_str(),
							reinterpret_cast <const string *> (buffer)->length()
						);
						// Если количество секунд передано
						if(seconds < 60)
							// Устанавливаем количество секунд
							this->_dt.seconds = seconds;
					// Если данные переданы в виде числа
					} else {
						// Если устанавливаемые данные достаточны
						if(size >= sizeof(uint8_t)){
							// Количество секунд для установки
							uint8_t seconds = 0;
							// Выполняем получение секунд
							::memcpy(&seconds, buffer, sizeof(seconds));
							// Если количество секунд передано
							if(seconds < 60)
								// Устанавливаем количество секунд
								this->_dt.seconds = seconds;
						}
					}
				} break;
				// Если требуется установить количество наносекунд
				case static_cast <uint8_t> (unit_t::NANOSECONDS): {
					// Если данные переданы в виде текста
					if(text){
						// Количество наносекунд для установки
						const uint64_t nanoseconds = this->_fmk->atoi <uint64_t> (
							reinterpret_cast <const string *> (buffer)->c_str(),
							reinterpret_cast <const string *> (buffer)->length()
						);
						// Получаем текущее значение размерности даты
						const uint8_t current = static_cast <uint8_t> (this->digits(nanoseconds) - 1);
						// Получаем размерность актуальной размерности даты
						const uint8_t actual = static_cast <uint8_t> (this->digits(this->timestamp(type_t::MILLISECONDS)) - 1);
						// Если текущее значение даты передано в наносекундах
						if(current >= (actual + 6))
							// Устанавливаем количество наносекунд
							this->_dt.nanoseconds = (nanoseconds % 1000000);
						// Устанавливаем количество наносекунд
						else this->_dt.nanoseconds = nanoseconds;
					// Если данные переданы в виде числа
					} else {
						// Если устанавливаемые данные достаточны
						if(size >= sizeof(uint64_t)){
							// Количество наносекунд для установки
							uint64_t nanoseconds = 0;
							// Выполняем получение наносекунд
							::memcpy(&nanoseconds, buffer, sizeof(nanoseconds));
							// Получаем текущее значение размерности даты
							const uint8_t current = static_cast <uint8_t> (this->digits(nanoseconds) - 1);
							// Получаем размерность актуальной размерности даты
							const uint8_t actual = static_cast <uint8_t> (this->digits(this->timestamp(type_t::MILLISECONDS)) - 1);
							// Если текущее значение даты передано в наносекундах
							if(current >= (actual + 6))
								// Устанавливаем количество наносекунд
								this->_dt.nanoseconds = (nanoseconds % 1000000);
							// Устанавливаем количество наносекунд
							else this->_dt.nanoseconds = nanoseconds;
						}
					}
				} break;
				// Если требуется установить количество микросекунд
				case static_cast <uint8_t> (unit_t::MICROSECONDS): {
					// Если данные переданы в виде текста
					if(text){
						// Количество микросекунд для установки
						const uint64_t microseconds = this->_fmk->atoi <uint64_t> (
							reinterpret_cast <const string *> (buffer)->c_str(),
							reinterpret_cast <const string *> (buffer)->length()
						);
						// Получаем текущее значение размерности даты
						const uint8_t current = static_cast <uint8_t> (this->digits(microseconds) - 1);
						// Получаем размерность актуальной размерности даты
						const uint8_t actual = static_cast <uint8_t> (this->digits(this->timestamp(type_t::MILLISECONDS)) - 1);
						// Если текущее значение даты передано в микросекундах
						if(current >= (actual + 3))
							// Устанавливаем количество микросекунд
							this->_dt.microseconds = (microseconds % 1000);
						// Устанавливаем количество микросекунд
						else this->_dt.microseconds = microseconds;
					// Если данные переданы в виде числа
					} else {
						// Если устанавливаемые данные достаточны
						if(size >= sizeof(uint64_t)){
							// Количество микросекунд для установки
							uint64_t microseconds = 0;
							// Выполняем получение микросекунд
							::memcpy(&microseconds, buffer, sizeof(microseconds));
							// Получаем текущее значение размерности даты
							const uint8_t current = static_cast <uint8_t> (this->digits(microseconds) - 1);
							// Получаем размерность актуальной размерности даты
							const uint8_t actual = static_cast <uint8_t> (this->digits(this->timestamp(type_t::MILLISECONDS)) - 1);
							// Если текущее значение даты передано в микросекундах
							if(current >= (actual + 3))
								// Устанавливаем количество микросекунд
								this->_dt.microseconds = (microseconds % 1000);
							// Устанавливаем количество микросекунд
							else this->_dt.microseconds = microseconds;
						}
					}
				} break;
				// Если требуется установить количество миллисекунд
				case static_cast <uint8_t> (unit_t::MILLISECONDS): {
					// Если данные переданы в виде текста
					if(text)
						// Устанавливаем количество миллисекунд
						this->_dt.milliseconds = this->_fmk->atoi <uint32_t> (
							reinterpret_cast <const string *> (buffer)->c_str(),
							reinterpret_cast <const string *> (buffer)->length()
						);
					// Если данные переданы в виде числа
					else {
						// Если устанавливаемые данные достаточны
						if(size >= sizeof(uint32_t)){
							// Количество миллисекунд для установки
							uint32_t milliseconds = 0;
							// Выполняем получение миллисекунд
							::memcpy(&milliseconds, buffer, sizeof(milliseconds));
							// Устанавливаем количество миллисекунд
							this->_dt.milliseconds = milliseconds;
						}
					}
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (unit), text), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Шаблон метода извлечения данных даты и времени
 *
 * @tparam T тип данных в котором извлекаются данные
 *
 */
template <typename T>
/**
 * @brief Метод извлечения данных даты и времени
 *
 * @param date дата для обработки
 * @param unit элементы данных для извлечения
 * @return     значение данных даты и времени
 *
 */
T awh::Chrono::get(const uint64_t date, const unit_t unit) const noexcept {
	// Переменная результата
	T result;
	// Если данные являются основными
	if(is_integral <T>::value || is_floating_point <T>::value || is_array <T>::value){
		// Буфер результата по умолчанию
		uint8_t buffer[sizeof(T)];
		// Заполняем нулями буфер данных
		::memset(buffer, 0, sizeof(T));
		// Выполняем установку результата по умолчанию
		::memcpy(&result, reinterpret_cast <T *> (buffer), sizeof(T));
	}
	// Выполняем извлечение данных
	this->get(&result, sizeof(result), date, unit, is_class_v <T>);
	// Возвращаем результат
	return result;
}
/**
 * Объявляем прототипы для метода извлечения данных даты и времени
 */
template int8_t awh::Chrono::get(const uint64_t, const unit_t) const noexcept;
template uint8_t awh::Chrono::get(const uint64_t, const unit_t) const noexcept;
template int16_t awh::Chrono::get(const uint64_t, const unit_t) const noexcept;
template uint16_t awh::Chrono::get(const uint64_t, const unit_t) const noexcept;
template int32_t awh::Chrono::get(const uint64_t, const unit_t) const noexcept;
template uint32_t awh::Chrono::get(const uint64_t, const unit_t) const noexcept;
template int64_t awh::Chrono::get(const uint64_t, const unit_t) const noexcept;
template uint64_t awh::Chrono::get(const uint64_t, const unit_t) const noexcept;
template float awh::Chrono::get(const uint64_t, const unit_t) const noexcept;
template double awh::Chrono::get(const uint64_t, const unit_t) const noexcept;
template string awh::Chrono::get(const uint64_t, const unit_t) const noexcept;
/**
 * Если операционной системой является macOS или Linux
 */
#if __APPLE__ || __MACH__ || __Linux__
	template size_t awh::Chrono::get(const uint64_t, const unit_t) const noexcept;
	template ssize_t awh::Chrono::get(const uint64_t, const unit_t) const noexcept;
#endif
/**
 * @brief Шаблон метода извлечения данных даты и времени
 *
 * @tparam T тип данных в котором извлекаются данные
 *
 */
template <typename T>
/**
 * @brief Метод извлечения данных даты и времени
 *
 * @param unit элементы данных для извлечения
 * @return     значение данных даты и времени
 *
 */
T awh::Chrono::get(const unit_t unit) const noexcept {
	// Переменная результата
	T result;
	// Если данные являются основными
	if(is_integral <T>::value || is_floating_point <T>::value || is_array <T>::value){
		// Буфер результата по умолчанию
		uint8_t buffer[sizeof(T)];
		// Заполняем нулями буфер данных
		::memset(buffer, 0, sizeof(T));
		// Выполняем установку результата по умолчанию
		::memcpy(&result, reinterpret_cast <T *> (buffer), sizeof(T));
	}
	// Выполняем извлечение данных
	this->get(&result, sizeof(result), unit, is_class_v <T>, storage_t::GLOBAL);
	// Возвращаем результат
	return result;
}
/**
 * Объявляем прототипы для метода извлечения данных даты и времени
 */
template int8_t awh::Chrono::get(const unit_t) const noexcept;
template uint8_t awh::Chrono::get(const unit_t) const noexcept;
template int16_t awh::Chrono::get(const unit_t) const noexcept;
template uint16_t awh::Chrono::get(const unit_t) const noexcept;
template int32_t awh::Chrono::get(const unit_t) const noexcept;
template uint32_t awh::Chrono::get(const unit_t) const noexcept;
template int64_t awh::Chrono::get(const unit_t) const noexcept;
template uint64_t awh::Chrono::get(const unit_t) const noexcept;
template float awh::Chrono::get(const unit_t) const noexcept;
template double awh::Chrono::get(const unit_t) const noexcept;
template string awh::Chrono::get(const unit_t) const noexcept;
/**
 * Если операционной системой является macOS или Linux
 */
#if __APPLE__ || __MACH__ || __Linux__
	template size_t awh::Chrono::get(const unit_t) const noexcept;
	template ssize_t awh::Chrono::get(const unit_t) const noexcept;
#endif
/**
 * @brief Шаблон метода извлечения данных даты и времени
 *
 * @tparam T тип данных в котором извлекаются данные
 *
 */
template <typename T>
/**
 * @brief Метод извлечения данных даты и времени
 *
 * @param unit    элементы данных для извлечения
 * @param storage хранение значение времени
 * @return        значение данных даты и времени
 *
 */
T awh::Chrono::get(const unit_t unit, const storage_t storage) const noexcept {
	// Переменная результата
	T result;
	// Если данные являются основными
	if(is_integral <T>::value || is_floating_point <T>::value || is_array <T>::value){
		// Буфер результата по умолчанию
		uint8_t buffer[sizeof(T)];
		// Заполняем нулями буфер данных
		::memset(buffer, 0, sizeof(T));
		// Выполняем установку результата по умолчанию
		::memcpy(&result, reinterpret_cast <T *> (buffer), sizeof(T));
	}
	// Выполняем извлечение данных
	this->get(&result, sizeof(result), unit, is_class_v <T>, storage);
	// Возвращаем результат
	return result;
}
/**
 * Объявляем прототипы для метода извлечения данных даты и времени
 */
template int8_t awh::Chrono::get(const unit_t, const storage_t) const noexcept;
template uint8_t awh::Chrono::get(const unit_t, const storage_t) const noexcept;
template int16_t awh::Chrono::get(const unit_t, const storage_t) const noexcept;
template uint16_t awh::Chrono::get(const unit_t, const storage_t) const noexcept;
template int32_t awh::Chrono::get(const unit_t, const storage_t) const noexcept;
template uint32_t awh::Chrono::get(const unit_t, const storage_t) const noexcept;
template int64_t awh::Chrono::get(const unit_t, const storage_t) const noexcept;
template uint64_t awh::Chrono::get(const unit_t, const storage_t) const noexcept;
template float awh::Chrono::get(const unit_t, const storage_t) const noexcept;
template double awh::Chrono::get(const unit_t, const storage_t) const noexcept;
template string awh::Chrono::get(const unit_t, const storage_t) const noexcept;
/**
 * Если операционной системой является macOS или Linux
 */
#if __APPLE__ || __MACH__ || __Linux__
	template size_t awh::Chrono::get(const unit_t, const storage_t) const noexcept;
	template ssize_t awh::Chrono::get(const unit_t, const storage_t) const noexcept;
#endif
/**
 * @brief Метод извлечения данных даты и времени
 *
 * @param buffer бинарный буфер данных
 * @param size   размер бинарного буфера
 * @param date   дата для обработки
 * @param unit   элементы данных для установки
 * @param text   данные переданы в виде текста
 *
 */
void awh::Chrono::get(void * buffer, const size_t size, const uint64_t date, const unit_t unit, const bool text) const noexcept {
	// Если данные переданы правильно
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем элементы устанавливаемых данных
			 */
			switch(static_cast <uint8_t> (unit)){
				// Если требуется установить номер текущего дня недели от 1 до 7
				case static_cast <uint8_t> (unit_t::DAY): {
					// Если размер данных умещается в буфер
					if(size >= sizeof(uint8_t)){
						// Создаем структуру времени
						dt_t dt;
						// Заполняем объект даты из штампа времени
						this->makeDate(date, dt);
						// Если данные переданы в виде текста
						if(text){
							// Получаем результирующий буфер для получения результата
							string * result = reinterpret_cast <string *> (buffer);
							// Выполняем получение текущего дня недели
							(* result) = params.nameDays[dt.day - 1].second;
						// Выполняем копирование текущего дня недели
						} else ::memcpy(buffer, &dt.day, sizeof(dt.day));
					}
				} break;
				// Если требуется установить число месяца от 1 до 31
				case static_cast <uint8_t> (unit_t::DATE): {
					// Если размер данных умещается в буфер
					if(size >= sizeof(uint8_t)){
						// Создаем структуру времени
						dt_t dt;
						// Заполняем объект даты из штампа времени
						this->makeDate(date, dt);
						// Если данные переданы в виде текста
						if(text){
							// Получаем результирующий буфер для получения результата
							string * result = reinterpret_cast <string *> (buffer);
							// Выполняем получение текущего значения даты
							(* result) = std::to_string(dt.date);
							// Если первого нуля нет
							if(result->length() == 1)
								// Добавляем предстоящий ноль
								result->insert(result->begin(), 1, '0');
						// Выполняем копирование текущего значения даты
						} else ::memcpy(buffer, &dt.date, sizeof(dt.date));
					}
				} break;
				// Если требуется установить полное обозначение года
				case static_cast <uint8_t> (unit_t::YEAR): {
					// Если размер данных умещается в буфер
					if(size >= sizeof(uint16_t)){
						// Получаем значение текущего года
						const uint16_t year = this->year(date);
						// Если данные переданы в виде текста
						if(text){
							// Получаем результирующий буфер для получения результата
							string * result = reinterpret_cast <string *> (buffer);
							// Выполняем получение текущего значения года
							(* result) = std::to_string(year);
						// Выполняем копирование текущего значения года
						} else ::memcpy(buffer, &year, sizeof(year));
					}
				} break;
				// Если требуется установить количество часов от 0 до 23
				case static_cast <uint8_t> (unit_t::HOUR): {
					// Если размер данных умещается в буфер
					if(size >= sizeof(uint8_t)){
						// Создаем структуру времени
						dt_t dt;
						// Заполняем объект даты из штампа времени
						this->makeDate(date, dt);
						// Если данные переданы в виде текста
						if(text){
							// Получаем результирующий буфер для получения результата
							string * result = reinterpret_cast <string *> (buffer);
							// Выполняем получение количество часов от 0 до 23
							(* result) = std::to_string(dt.hour);
							// Если первого нуля нет
							if(result->length() == 1)
								// Добавляем предстоящий ноль
								result->insert(result->begin(), 1, '0');
						// Выполняем копирование количество часов от 0 до 23
						} else ::memcpy(buffer, &dt.hour, sizeof(dt.hour));
					}
				} break;
				// Если требуется установить количество прошедших дней от 1 января
				case static_cast <uint8_t> (unit_t::DAYS): {
					// Если размер данных умещается в буфер
					if(size >= sizeof(uint16_t)){
						// Создаем структуру времени
						dt_t dt;
						// Заполняем объект даты из штампа времени
						this->makeDate(date, dt);
						// Если данные переданы в виде текста
						if(text){
							// Получаем результирующий буфер для получения результата
							string * result = reinterpret_cast <string *> (buffer);
							// Выполняем получение количество прошедших дней от 1 января
							(* result) = std::to_string(dt.days);
							// Если первого нуля нет
							if(result->length() == 1)
								// Добавляем предстоящий ноль
								result->insert(result->begin(), 1, '0');
						// Выполняем копирование количество прошедших дней от 1 января
						} else ::memcpy(buffer, &dt.days, sizeof(dt.days));
					}
				} break;
				// Если требуется установить номер месяца от 1 до 12 (начиная с Января)
				case static_cast <uint8_t> (unit_t::MONTH): {
					// Если размер данных умещается в буфер
					if(size >= sizeof(uint8_t)){
						// Создаем структуру времени
						dt_t dt;
						// Заполняем объект даты из штампа времени
						this->makeDate(date, dt);
						// Если данные переданы в виде текста
						if(text){
							// Получаем результирующий буфер для получения результата
							string * result = reinterpret_cast <string *> (buffer);
							// Выполняем получение названия месяца
							(* result) = params.nameMonths[dt.month - 1].second;
						// Выполняем копирование текущего значения месяца
						} else ::memcpy(buffer, &dt.month, sizeof(dt.month));
					}
				} break;
				// Если требуется установить количество недель прошедших с начала года
				case static_cast <uint8_t> (unit_t::WEEKS): {
					// Если размер данных умещается в буфер
					if(size >= sizeof(uint8_t)){
						// Создаем структуру времени
						dt_t dt;
						// Заполняем объект даты из штампа времени
						this->makeDate(date, dt);
						// Если данные переданы в виде текста
						if(text){
							// Получаем результирующий буфер для получения результата
							string * result = reinterpret_cast <string *> (buffer);
							// Выполняем получение количество недель прошедших с начала года
							(* result) = std::to_string(dt.weeks);
						// Выполняем копирование количество недель прошедших с начала года
						} else ::memcpy(buffer, &dt.weeks, sizeof(dt.weeks));
					}
				} break;
				// Если требуется установить смещение временной зоны в секундах относительно UTC
				case static_cast <uint8_t> (unit_t::OFFSET): {
					// Если размер данных умещается в буфер
					if(size >= sizeof(int32_t)){
						// Создаем структуру времени
						dt_t dt;
						// Заполняем объект даты из штампа времени
						this->makeDate(date, dt);
						// Если данные переданы в виде текста
						if(text){
							// Получаем результирующий буфер для получения результата
							string * result = reinterpret_cast <string *> (buffer);
							// Выполняем получение смещения временной зоны в секундах относительно UTC
							(* result) = std::to_string(dt.offset);
						// Выполняем копирование смещения временной зоны в секундах относительно UTC
						} else ::memcpy(buffer, &dt.offset, sizeof(dt.offset));
					}
				} break;
				// Если требуется установить количество минут от 0 до 59
				case static_cast <uint8_t> (unit_t::MINUTES): {
					// Если размер данных умещается в буфер
					if(size >= sizeof(uint8_t)){
						// Создаем структуру времени
						dt_t dt;
						// Заполняем объект даты из штампа времени
						this->makeDate(date, dt);
						// Если данные переданы в виде текста
						if(text){
							// Получаем результирующий буфер для получения результата
							string * result = reinterpret_cast <string *> (buffer);
							// Выполняем получение количества минут от 0 до 59
							(* result) = std::to_string(dt.minutes);
							// Если первого нуля нет
							if(result->length() == 1)
								// Добавляем предстоящий ноль
								result->insert(result->begin(), 1, '0');
						// Выполняем получение количества минут от 0 до 59
						} else ::memcpy(buffer, &dt.minutes, sizeof(dt.minutes));
					}
				} break;
				// Если требуется установить количество секунд от 0 до 59
				case static_cast <uint8_t> (unit_t::SECONDS): {
					// Если размер данных умещается в буфер
					if(size >= sizeof(uint8_t)){
						// Создаем структуру времени
						dt_t dt;
						// Заполняем объект даты из штампа времени
						this->makeDate(date, dt);
						// Если данные переданы в виде текста
						if(text){
							// Получаем результирующий буфер для получения результата
							string * result = reinterpret_cast <string *> (buffer);
							// Выполняем получение количества секунд от 0 до 59
							(* result) = std::to_string(dt.seconds);
							// Если первого нуля нет
							if(result->length() == 1)
								// Добавляем предстоящий ноль
								result->insert(result->begin(), 1, '0');
						// Выполняем получение количества секунд от 0 до 59
						} else ::memcpy(buffer, &dt.seconds, sizeof(dt.seconds));
					}
				} break;
				// Если требуется установить количество наносекунд
				case static_cast <uint8_t> (unit_t::NANOSECONDS): {
					// Если размер данных умещается в буфер
					if(size >= sizeof(uint64_t)){
						// Количество наносекунд
						const uint64_t nanoseconds = (date % 1000000);
						// Если данные переданы в виде текста
						if(text){
							// Получаем результирующий буфер для получения результата
							string * result = reinterpret_cast <string *> (buffer);
							// Выполняем получение количества наносекунд
							(* result) = std::to_string(nanoseconds);
							// Если первого нуля нет
							if(result->length() == 1)
								// Добавляем предстоящий ноль
								result->insert(result->begin(), 5, '0');
							// Если первого нуля нет
							else if(result->length() == 2)
								// Добавляем предстоящий ноль
								result->insert(result->begin(), 4, '0');
							// Если первого нуля нет
							else if(result->length() == 3)
								// Добавляем предстоящий ноль
								result->insert(result->begin(), 3, '0');
							// Если первого нуля нет
							else if(result->length() == 4)
								// Добавляем предстоящий ноль
								result->insert(result->begin(), 2, '0');
							// Если первого нуля нет
							else if(result->length() == 5)
								// Добавляем предстоящий ноль
								result->insert(result->begin(), 1, '0');
						// Выполняем получение количества наносекунд
						} else ::memcpy(buffer, &nanoseconds, sizeof(nanoseconds));
					}
				} break;
				// Если требуется установить количество микросекунд
				case static_cast <uint8_t> (unit_t::MICROSECONDS): {
					// Если размер данных умещается в буфер
					if(size >= sizeof(uint64_t)){
						// Количество микросекунд
						const uint64_t microseconds = (date % 1000);
						// Если данные переданы в виде текста
						if(text){
							// Получаем результирующий буфер для получения результата
							string * result = reinterpret_cast <string *> (buffer);
							// Выполняем получение количества микросекунд
							(* result) = std::to_string(microseconds);
							// Если первого нуля нет
							if(result->length() == 1)
								// Добавляем предстоящий ноль
								result->insert(result->begin(), 2, '0');
							// Если первого нуля нет
							else if(result->length() == 2)
								// Добавляем предстоящий ноль
								result->insert(result->begin(), 1, '0');
						// Выполняем получение количества микросекунд
						} else ::memcpy(buffer, &microseconds, sizeof(microseconds));
					}
				} break;
				// Если требуется установить количество миллисекунд
				case static_cast <uint8_t> (unit_t::MILLISECONDS): {
					// Если размер данных умещается в буфер
					if(size >= sizeof(uint32_t)){
						// Создаем структуру времени
						dt_t dt;
						// Заполняем объект даты из штампа времени
						this->makeDate(date, dt);
						// Если данные переданы в виде текста
						if(text){
							// Получаем результирующий буфер для получения результата
							string * result = reinterpret_cast <string *> (buffer);
							// Выполняем получение количества миллисекунд
							(* result) = std::to_string(dt.milliseconds);
							// Если первого нуля нет
							if(result->length() == 1)
								// Добавляем предстоящий ноль
								result->insert(result->begin(), 2, '0');
							// Если первого нуля нет
							else if(result->length() == 2)
								// Добавляем предстоящий ноль
								result->insert(result->begin(), 1, '0');
						// Выполняем получение количества миллисекунд
						} else ::memcpy(buffer, &dt.milliseconds, sizeof(dt.milliseconds));
					}
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size, date, static_cast <uint16_t> (unit), text), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод извлечения данных даты и времени
 *
 * @param buffer  бинарный буфер данных
 * @param size    размер бинарного буфера
 * @param unit    элементы данных для установки
 * @param text    данные переданы в виде текста
 * @param storage хранение значение времени
 *
 */
void awh::Chrono::get(void * buffer, const size_t size, const unit_t unit, const bool text, const storage_t storage) const noexcept {
	// Если данные переданы правильно
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем элементы устанавливаемых данных
			 */
			switch(static_cast <uint8_t> (unit)){
				// Если требуется установить номер текущего дня недели от 1 до 7
				case static_cast <uint8_t> (unit_t::DAY): {
					// Если данные переданы в виде текста
					if(text){
						// Получаем номер текущего дня недели
						string * result = reinterpret_cast <string *> (buffer);
						/**
						 * Определяем хранилище значение времени
						 */
						switch(static_cast <uint8_t> (storage)){
							// Если хранилище локальное
							case static_cast <uint8_t> (storage_t::LOCAL):
								// Выполняем получение номера текущего дня недели
								(* result) = params.nameDays[this->_dt.day - 1].second;
							break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Текущий номер дня недели
								uint8_t day = 0;
								// Выполняем извлечение текущего дня недели
								this->get(&day, sizeof(day), this->timestamp(type_t::MILLISECONDS), unit, false);
								// Выполняем получение номера текущего дня недели
								(* result) = params.nameDays[day - 1].second;
							} break;
						}
					// Если данные переданы в виде числа
					} else {
						/**
						 * Определяем хранилище значение времени
						 */
						switch(static_cast <uint8_t> (storage)){
							// Если хранилище локальное
							case static_cast <uint8_t> (storage_t::LOCAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(this->_dt.day))
									// Выполняем копирование текущего дня недели
									::memcpy(buffer, &this->_dt.day, sizeof(this->_dt.day));
							} break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(uint8_t)){
									// Текущий номер дня недели
									uint8_t day = 0;
									// Выполняем извлечение текущего дня недели
									this->get(&day, sizeof(day), this->timestamp(type_t::MILLISECONDS), unit, false);
									// Выполняем копирование текущего дня недели
									::memcpy(buffer, &day, sizeof(day));
								}
							} break;
						}
					}
				} break;
				// Если требуется установить число месяца от 1 до 31
				case static_cast <uint8_t> (unit_t::DATE): {
					// Если данные переданы в виде текста
					if(text){
						// Получаем номер текущего дня недели
						string * result = reinterpret_cast <string *> (buffer);
						/**
						 * Определяем хранилище значение времени
						 */
						switch(static_cast <uint8_t> (storage)){
							// Если хранилище локальное
							case static_cast <uint8_t> (storage_t::LOCAL):
								// Выполняем копирование текущего значения даты
								(* result) = std::to_string(this->_dt.date);
							break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Число месяца от 1 до 31
								uint8_t date = 0;
								// Выполняем извлечение числа месяца
								this->get(&date, sizeof(date), this->timestamp(type_t::MILLISECONDS), unit, false);
								// Выполняем копирование текущего значения даты
								(* result) = std::to_string(date);
							} break;
						}
						// Если первого нуля нет
						if(result->length() == 1)
							// Добавляем предстоящий ноль
							result->insert(result->begin(), 1, '0');
					// Если данные переданы в виде числа
					} else {
						/**
						 * Определяем хранилище значение времени
						 */
						switch(static_cast <uint8_t> (storage)){
							// Если хранилище локальное
							case static_cast <uint8_t> (storage_t::LOCAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(this->_dt.date))
									// Выполняем копирование текущего значения даты
									::memcpy(buffer, &this->_dt.date, sizeof(this->_dt.date));
							} break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(uint8_t)){
									// Число месяца от 1 до 31
									uint8_t date = 0;
									// Выполняем извлечение числа месяца
									this->get(&date, sizeof(date), this->timestamp(type_t::MILLISECONDS), unit, false);
									// Выполняем копирование числа месяца
									::memcpy(buffer, &date, sizeof(date));
								}
							} break;
						}
					}
				} break;
				// Если требуется установить полное обозначение года
				case static_cast <uint8_t> (unit_t::YEAR): {
					// Если данные переданы в виде текста
					if(text){
						// Получаем номер текущего дня недели
						string * result = reinterpret_cast <string *> (buffer);
						/**
						 * Определяем хранилище значение времени
						 */
						switch(static_cast <uint8_t> (storage)){
							// Если хранилище локальное
							case static_cast <uint8_t> (storage_t::LOCAL):
								// Выполняем копирование текущего значения года
								(* result) = std::to_string(this->_dt.year);
							break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Полное обозначение года
								uint16_t year = 0;
								// Выполняем извлечение года
								this->get(&year, sizeof(year), this->timestamp(type_t::MILLISECONDS), unit, false);
								// Выполняем копирование текущего значения года
								(* result) = std::to_string(year);
							} break;
						}
					// Если данные переданы в виде числа
					} else {
						/**
						 * Определяем хранилище значение времени
						 */
						switch(static_cast <uint8_t> (storage)){
							// Если хранилище локальное
							case static_cast <uint8_t> (storage_t::LOCAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(this->_dt.year))
									// Выполняем копирование текущего значения года
									::memcpy(buffer, &this->_dt.year, sizeof(this->_dt.year));
							} break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(uint16_t)){
									// Полное обозначение года
									uint16_t year = 0;
									// Выполняем извлечение года
									this->get(&year, sizeof(year), this->timestamp(type_t::MILLISECONDS), unit, false);
									// Выполняем копирование текущего значения года
									::memcpy(buffer, &year, sizeof(year));
								}
							} break;
						}
					}
				} break;
				// Если требуется установить количество часов от 0 до 23
				case static_cast <uint8_t> (unit_t::HOUR): {
					// Если данные переданы в виде текста
					if(text){
						// Получаем номер текущего дня недели
						string * result = reinterpret_cast <string *> (buffer);
						/**
						 * Определяем хранилище значение времени
						 */
						switch(static_cast <uint8_t> (storage)){
							// Если хранилище локальное
							case static_cast <uint8_t> (storage_t::LOCAL):
								// Выполняем копирование текущее значение часа
								(* result) = std::to_string(this->_dt.hour);
							break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Текущее значение часа
								uint8_t hour = 0;
								// Выполняем извлечение текущее значение часа
								this->get(&hour, sizeof(hour), this->timestamp(type_t::MILLISECONDS), unit, false);
								// Выполняем копирование текущее значение часа
								(* result) = std::to_string(hour);
							} break;
						}
						// Если первого нуля нет
						if(result->length() == 1)
							// Добавляем предстоящий ноль
							result->insert(result->begin(), 1, '0');
					// Если данные переданы в виде числа
					} else {
						/**
						 * Определяем хранилище значение времени
						 */
						switch(static_cast <uint8_t> (storage)){
							// Если хранилище локальное
							case static_cast <uint8_t> (storage_t::LOCAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(this->_dt.hour))
									// Выполняем копирование текущее значение часа
									::memcpy(buffer, &this->_dt.hour, sizeof(this->_dt.hour));
							} break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(uint8_t)){
									// Текущее значение часа
									uint8_t hour = 0;
									// Выполняем извлечение текущее значение часа
									this->get(&hour, sizeof(hour), this->timestamp(type_t::MILLISECONDS), unit, false);
									// Выполняем копирование текущее значение часа
									::memcpy(buffer, &hour, sizeof(hour));
								}
							} break;
						}
					}
				} break;
				// Если требуется установить количество прошедших дней от 1 января
				case static_cast <uint8_t> (unit_t::DAYS): {
					// Если данные переданы в виде текста
					if(text){
						// Получаем номер текущего дня недели
						string * result = reinterpret_cast <string *> (buffer);
						/**
						 * Определяем хранилище значение времени
						 */
						switch(static_cast <uint8_t> (storage)){
							// Если хранилище локальное
							case static_cast <uint8_t> (storage_t::LOCAL):
								// Выполняем копирование количество прошедших дней от 1 января
								(* result) = std::to_string(this->_dt.days);
							break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Количество прошедших дней от 1 января
								uint16_t days = 0;
								// Выполняем извлечение количества прошедших дней от 1 января
								this->get(&days, sizeof(days), this->timestamp(type_t::MILLISECONDS), unit, false);
								// Выполняем копирование количество прошедших дней от 1 января
								(* result) = std::to_string(days);
							} break;
						}
						// Если первого нуля нет
						if(result->length() == 1)
							// Добавляем предстоящий ноль
							result->insert(result->begin(), 1, '0');
					// Если данные переданы в виде числа
					} else {
						/**
						 * Определяем хранилище значение времени
						 */
						switch(static_cast <uint8_t> (storage)){
							// Если хранилище локальное
							case static_cast <uint8_t> (storage_t::LOCAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(this->_dt.days))
									// Выполняем копирование количество прошедших дней от 1 января
									::memcpy(buffer, &this->_dt.days, sizeof(this->_dt.days));
							} break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(uint16_t)){
									// Количество прошедших дней от 1 января
									uint16_t days = 0;
									// Выполняем извлечение количества прошедших дней от 1 января
									this->get(&days, sizeof(days), this->timestamp(type_t::MILLISECONDS), unit, false);
									// Выполняем копирование количество прошедших дней от 1 января
									::memcpy(buffer, &days, sizeof(days));
								}
							} break;
						}
					}
				} break;
				// Если требуется установить номер месяца от 1 до 12 (начиная с Января)
				case static_cast <uint8_t> (unit_t::MONTH): {
					// Если данные переданы в виде текста
					if(text){
						// Получаем название месяца
						string * result = reinterpret_cast <string *> (buffer);
						/**
						 * Определяем хранилище значение времени
						 */
						switch(static_cast <uint8_t> (storage)){
							// Если хранилище локальное
							case static_cast <uint8_t> (storage_t::LOCAL):
								// Выполняем получение названия месяца
								(* result) = params.nameMonths[this->_dt.month - 1].second;
							break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Текущее значение месяца
								uint8_t month = 0;
								// Выполняем извлечение текущего значения месяца
								this->get(&month, sizeof(month), this->timestamp(type_t::MILLISECONDS), unit, false);
								// Выполняем получение названия месяца
								(* result) = params.nameMonths[month - 1].second;
							} break;
						}
					// Если данные переданы в виде числа
					} else {
						/**
						 * Определяем хранилище значение времени
						 */
						switch(static_cast <uint8_t> (storage)){
							// Если хранилище локальное
							case static_cast <uint8_t> (storage_t::LOCAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(this->_dt.month))
									// Выполняем копирование текущего значения месяца
									::memcpy(buffer, &this->_dt.month, sizeof(this->_dt.month));
							} break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(uint8_t)){
									// Текущее значение месяца
									uint8_t month = 0;
									// Выполняем извлечение текущего значения месяца
									this->get(&month, sizeof(month), this->timestamp(type_t::MILLISECONDS), unit, false);
									// Выполняем копирование текущего значения месяца
									::memcpy(buffer, &month, sizeof(month));
								}
							} break;
						}
					}
				} break;
				// Если требуется установить количество недель прошедших с начала года
				case static_cast <uint8_t> (unit_t::WEEKS): {
					// Если данные переданы в виде текста
					if(text){
						// Получаем номер текущего дня недели
						string * result = reinterpret_cast <string *> (buffer);
						/**
						 * Определяем хранилище значение времени
						 */
						switch(static_cast <uint8_t> (storage)){
							// Если хранилище локальное
							case static_cast <uint8_t> (storage_t::LOCAL):
								// Выполняем копирование текущего количества недель прошедших с начала года
								(* result) = std::to_string(this->_dt.weeks);
							break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Количество недель прошедших с начала года
								uint8_t weeks = 0;
								// Выполняем извлечение количества недель прошедших с начала года
								this->get(&weeks, sizeof(weeks), this->timestamp(type_t::MILLISECONDS), unit, false);
								// Выполняем копирование текущего количества недель прошедших с начала года
								(* result) = std::to_string(weeks);
							} break;
						}
					// Если данные переданы в виде числа
					} else {
						/**
						 * Определяем хранилище значение времени
						 */
						switch(static_cast <uint8_t> (storage)){
							// Если хранилище локальное
							case static_cast <uint8_t> (storage_t::LOCAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(this->_dt.weeks))
									// Выполняем копирование текущего количества недель прошедших с начала года
									::memcpy(buffer, &this->_dt.weeks, sizeof(this->_dt.weeks));
							} break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(uint8_t)){
									// Количество недель прошедших с начала года
									uint8_t weeks = 0;
									// Выполняем извлечение количества недель прошедших с начала года
									this->get(&weeks, sizeof(weeks), this->timestamp(type_t::MILLISECONDS), unit, false);
									// Выполняем копирование количества недель прошедших с начала года
									::memcpy(buffer, &weeks, sizeof(weeks));
								}
							} break;
						}
					}
				} break;
				// Если требуется установить смещение временной зоны в секундах относительно UTC
				case static_cast <uint8_t> (unit_t::OFFSET): {
					// Если данные переданы в виде текста
					if(text){
						// Получаем номер текущего дня недели
						string * result = reinterpret_cast <string *> (buffer);
						/**
						 * Определяем хранилище значение времени
						 */
						switch(static_cast <uint8_t> (storage)){
							// Если хранилище локальное
							case static_cast <uint8_t> (storage_t::LOCAL):
								// Выполняем копирование смещение временной зоны в секундах относительно UTC
								(* result) = std::to_string(this->_dt.offset);
							break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Смещение временной зоны в секундах относительно UTC
								int32_t offset = 0;
								// Выполняем извлечение смещения временной зоны в секундах относительно UTC
								this->get(&offset, sizeof(offset), this->timestamp(type_t::MILLISECONDS), unit, false);
								// Выполняем копирование смещение временной зоны в секундах относительно UTC
								(* result) = std::to_string(offset);
							} break;
						}
					// Если данные переданы в виде числа
					} else {
						/**
						 * Определяем хранилище значение времени
						 */
						switch(static_cast <uint8_t> (storage)){
							// Если хранилище локальное
							case static_cast <uint8_t> (storage_t::LOCAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(this->_dt.offset))
									// Выполняем копирование смещение временной зоны в секундах относительно UTC
									::memcpy(buffer, &this->_dt.offset, sizeof(this->_dt.offset));
							} break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(int32_t)){
									// Смещение временной зоны в секундах относительно UTC
									int32_t offset = 0;
									// Выполняем извлечение смещения временной зоны в секундах относительно UTC
									this->get(&offset, sizeof(offset), this->timestamp(type_t::MILLISECONDS), unit, false);
									// Выполняем копирование смещение временной зоны в секундах относительно UTC
									::memcpy(buffer, &offset, sizeof(offset));
								}
							} break;
						}
					}
				} break;
				// Если требуется установить количество минут от 0 до 59
				case static_cast <uint8_t> (unit_t::MINUTES): {
					// Если данные переданы в виде текста
					if(text){
						// Получаем номер текущего дня недели
						string * result = reinterpret_cast <string *> (buffer);
						/**
						 * Определяем хранилище значение времени
						 */
						switch(static_cast <uint8_t> (storage)){
							// Если хранилище локальное
							case static_cast <uint8_t> (storage_t::LOCAL):
								// Выполняем копирование текущее количество минут
								(* result) = std::to_string(this->_dt.minutes);
							break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Количество минут от 0 до 59
								uint8_t minutes = 0;
								// Выполняем извлечение количество минут от 0 до 59
								this->get(&minutes, sizeof(minutes), this->timestamp(type_t::MILLISECONDS), unit, false);
								// Выполняем копирование текущее количество минут
								(* result) = std::to_string(minutes);
							} break;
						}
						// Если первого нуля нет
						if(result->length() == 1)
							// Добавляем предстоящий ноль
							result->insert(result->begin(), 1, '0');
					// Если данные переданы в виде числа
					} else {
						/**
						 * Определяем хранилище значение времени
						 */
						switch(static_cast <uint8_t> (storage)){
							// Если хранилище локальное
							case static_cast <uint8_t> (storage_t::LOCAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(this->_dt.minutes))
									// Выполняем копирование текущее количество минут
									::memcpy(buffer, &this->_dt.minutes, sizeof(this->_dt.minutes));
							} break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(uint8_t)){
									// Количество минут от 0 до 59
									uint8_t minutes = 0;
									// Выполняем извлечение количество минут от 0 до 59
									this->get(&minutes, sizeof(minutes), this->timestamp(type_t::MILLISECONDS), unit, false);
									// Выполняем копирование количество минут от 0 до 59
									::memcpy(buffer, &minutes, sizeof(minutes));
								}
							} break;
						}
					}
				} break;
				// Если требуется установить количество секунд от 0 до 59
				case static_cast <uint8_t> (unit_t::SECONDS): {
					// Если данные переданы в виде текста
					if(text){
						// Получаем номер текущего дня недели
						string * result = reinterpret_cast <string *> (buffer);
						/**
						 * Определяем хранилище значение времени
						 */
						switch(static_cast <uint8_t> (storage)){
							// Если хранилище локальное
							case static_cast <uint8_t> (storage_t::LOCAL):
								// Выполняем копирование текущее количество секунд
								(* result) = std::to_string(this->_dt.seconds);
							break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Количество секунд от 0 до 59
								uint8_t seconds = 0;
								// Выполняем извлечение количество секунд от 0 до 59
								this->get(&seconds, sizeof(seconds), this->timestamp(type_t::MILLISECONDS), unit, false);
								// Выполняем копирование текущее количество секунд
								(* result) = std::to_string(seconds);
							} break;
						}
						// Если первого нуля нет
						if(result->length() == 1)
							// Добавляем предстоящий ноль
							result->insert(result->begin(), 1, '0');
					// Если данные переданы в виде числа
					} else {
						/**
						 * Определяем хранилище значение времени
						 */
						switch(static_cast <uint8_t> (storage)){
							// Если хранилище локальное
							case static_cast <uint8_t> (storage_t::LOCAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(this->_dt.seconds))
									// Выполняем копирование текущее количество секунд
									::memcpy(buffer, &this->_dt.seconds, sizeof(this->_dt.seconds));
							} break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(uint8_t)){
									// Количество секунд от 0 до 59
									uint8_t seconds = 0;
									// Выполняем извлечение количество секунд от 0 до 59
									this->get(&seconds, sizeof(seconds), this->timestamp(type_t::MILLISECONDS), unit, false);
									// Выполняем копирование количество секунд от 0 до 59
									::memcpy(buffer, &seconds, sizeof(seconds));
								}
							} break;
						}
					}
				} break;
				// Если требуется установить количество наносекунд
				case static_cast <uint8_t> (unit_t::NANOSECONDS): {
					// Если данные переданы в виде текста
					if(text){
						// Получаем номер текущего дня недели
						string * result = reinterpret_cast <string *> (buffer);
						/**
						 * Определяем хранилище значение времени
						 */
						switch(static_cast <uint8_t> (storage)){
							// Если хранилище локальное
							case static_cast <uint8_t> (storage_t::LOCAL):
								// Выполняем копирование количество наносекунд
								(* result) = std::to_string(this->_dt.nanoseconds);
							break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL):
								// Выполняем копирование количество наносекунд
								(* result) = std::to_string(this->timestamp(type_t::NANOSECONDS) % 1000000);
							break;
						}
						// Если первого нуля нет
						if(result->length() == 1)
							// Добавляем предстоящий ноль
							result->insert(result->begin(), 5, '0');
						// Если первого нуля нет
						else if(result->length() == 2)
							// Добавляем предстоящий ноль
							result->insert(result->begin(), 4, '0');
						// Если первого нуля нет
						else if(result->length() == 3)
							// Добавляем предстоящий ноль
							result->insert(result->begin(), 3, '0');
						// Если первого нуля нет
						else if(result->length() == 4)
							// Добавляем предстоящий ноль
							result->insert(result->begin(), 2, '0');
						// Если первого нуля нет
						else if(result->length() == 5)
							// Добавляем предстоящий ноль
							result->insert(result->begin(), 1, '0');
					// Если данные переданы в виде числа
					} else {
						/**
						 * Определяем хранилище значение времени
						 */
						switch(static_cast <uint8_t> (storage)){
							// Если хранилище локальное
							case static_cast <uint8_t> (storage_t::LOCAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(this->_dt.nanoseconds))
									// Выполняем копирование количество наносекунд
									::memcpy(buffer, &this->_dt.nanoseconds, sizeof(this->_dt.nanoseconds));
							} break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(uint64_t)){
									// Количество наносекунд
									const uint64_t nanoseconds = (this->timestamp(type_t::NANOSECONDS) % 1000000);
									// Выполняем копирование количество наносекунд
									::memcpy(buffer, &nanoseconds, sizeof(nanoseconds));
								}
							} break;
						}
					}
				} break;
				// Если требуется установить количество микросекунд
				case static_cast <uint8_t> (unit_t::MICROSECONDS): {
					// Если данные переданы в виде текста
					if(text){
						// Получаем номер текущего дня недели
						string * result = reinterpret_cast <string *> (buffer);
						/**
						 * Определяем хранилище значение времени
						 */
						switch(static_cast <uint8_t> (storage)){
							// Если хранилище локальное
							case static_cast <uint8_t> (storage_t::LOCAL): {
								// Выполняем копирование количество микросекунд
								(* result) = std::to_string(this->_dt.microseconds);
							} break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL):
								// Выполняем копирование количество микросекунд
								(* result) = std::to_string(this->timestamp(type_t::MICROSECONDS) % 1000);
							break;
						}
						// Если первого нуля нет
						if(result->length() == 1)
							// Добавляем предстоящий ноль
							result->insert(result->begin(), 2, '0');
						// Если первого нуля нет
						else if(result->length() == 2)
							// Добавляем предстоящий ноль
							result->insert(result->begin(), 1, '0');
					// Если данные переданы в виде числа
					} else {
						/**
						 * Определяем хранилище значение времени
						 */
						switch(static_cast <uint8_t> (storage)){
							// Если хранилище локальное
							case static_cast <uint8_t> (storage_t::LOCAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(this->_dt.microseconds))
									// Получаем текущее количество микросекунд
									::memcpy(buffer, &this->_dt.microseconds, sizeof(this->_dt.microseconds));
							} break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(uint64_t)){
									// Количество микросекунд
									const uint64_t microseconds = (this->timestamp(type_t::MICROSECONDS) % 1000);
									// Выполняем копирование количество микросекунд
									::memcpy(buffer, &microseconds, sizeof(microseconds));
								}
							} break;
						}
					}
				} break;
				// Если требуется установить количество миллисекунд
				case static_cast <uint8_t> (unit_t::MILLISECONDS): {
					// Если данные переданы в виде текста
					if(text){
						// Получаем номер текущего дня недели
						string * result = reinterpret_cast <string *> (buffer);
						/**
						 * Определяем хранилище значение времени
						 */
						switch(static_cast <uint8_t> (storage)){
							// Если хранилище локальное
							case static_cast <uint8_t> (storage_t::LOCAL):
								// Выполняем копирование количество миллисекунд
								(* result) = std::to_string(this->_dt.milliseconds);
							break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Количество миллисекунд
								uint32_t milliseconds = 0;
								// Выполняем извлечение количество миллисекунд
								this->get(&milliseconds, sizeof(milliseconds), this->timestamp(type_t::MILLISECONDS), unit, false);
								// Выполняем копирование количество миллисекунд
								(* result) = std::to_string(milliseconds);
							} break;
						}
						// Если первого нуля нет
						if(result->length() == 1)
							// Добавляем предстоящий ноль
							result->insert(result->begin(), 2, '0');
						// Если первого нуля нет
						else if(result->length() == 2)
							// Добавляем предстоящий ноль
							result->insert(result->begin(), 1, '0');
					// Если данные переданы в виде числа
					} else {
						/**
						 * Определяем хранилище значение времени
						 */
						switch(static_cast <uint8_t> (storage)){
							// Если хранилище локальное
							case static_cast <uint8_t> (storage_t::LOCAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(this->_dt.milliseconds))
									// Получаем текущее количество миллисекунд
									::memcpy(buffer, &this->_dt.milliseconds, sizeof(this->_dt.milliseconds));
							} break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								// Если размер данных умещается в буфер
								if(size >= sizeof(uint32_t)){
									// Количество миллисекунд
									uint32_t milliseconds = 0;
									// Выполняем извлечение количество миллисекунд
									this->get(&milliseconds, sizeof(milliseconds), this->timestamp(type_t::MILLISECONDS), unit, false);
									// Выполняем копирование количество миллисекунд
									::memcpy(buffer, &milliseconds, sizeof(milliseconds));
								}
							} break;
						}
					}
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (unit), text, static_cast <uint16_t> (storage)), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод установки временной зоны
 *
 * @param zone смещение временной зоны для установки (в секундах)
 *
 */
void awh::Chrono::setTimeZone(const int32_t zone) noexcept {
	// Выполняем блокировку потока
	const locker_t <> lock(this->_mtx.date);
	// Устанавливаем временную зону в секундах
	this->_dt.offset = zone;
	// Устанавливаем идентификатор временной зоны
	this->_dt.zone = zone_t::UTC;
	// Выполняем перерасчёт локальной версии даты
	this->makeDate(this->makeDate(this->_dt), this->_dt);
}
/**
 * @brief Метод установки временной зоны
 *
 * @param zone временная зона для установки
 *
 */
void awh::Chrono::setTimeZone(const zone_t zone) noexcept {
	// Выполняем блокировку потока
	const locker_t <> lock(this->_mtx.date);
	// Устанавливаем идентификатор временной зоны
	this->_dt.zone = zone;
	// Устанавливаем временную зону в секундах
	this->_dt.offset = this->getTimeZone(zone);
	// Выполняем перерасчёт локальной версии даты
	this->makeDate(this->makeDate(this->_dt), this->_dt);
}
/**
 * @brief Метод установки временной зоны
 *
 * @param zone временная зона для установки
 *
 */
void awh::Chrono::setTimeZone(string_view zone) noexcept {
	// Выполняем блокировку потока
	const locker_t <> lock(this->_mtx.date);
	// Устанавливаем идентификатор временной зоны
	this->_dt.zone = this->matchTimeZone(zone);
	// Устанавливаем временную зону в секундах
	this->_dt.offset = this->getTimeZone(zone);
	// Выполняем перерасчёт локальной версии даты
	this->makeDate(this->makeDate(this->_dt), this->_dt);
}
/**
 * @brief Метод выполнения матчинга временной зоны
 *
 * @param zone временная зона для конвертации
 * @return     определённая временная зона
 *
 */
awh::Chrono::zone_t awh::Chrono::matchTimeZone(string_view zone) const noexcept {
	// Переменная результата
	zone_t result = zone_t::NONE;
	// Если временная зона для матчинга передана
	if(!zone.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем нативный разбор названия временной зоны (\w+)
			const vector <match_t> match = ::parseWord(zone.data(), zone.size());
			// Если совпадение получено
			if(!match.empty()){
				// Обрабатываем полученные группы совпадения
				{
					// Создаём массив собранных результатов
					vector <string> data(match.size());
					/**
					 * Выполняем перебор всех полученных вариантов
					 */
					for(uint8_t j = 0; j < static_cast <uint8_t> (match.size()); j++){
						// Если результат получен
						if(match[j].end > match[j].begin)
							// Выполняем установку результата
							data[j].assign(zone.data() + match[j].begin, match[j].end - match[j].begin);
					}
					// Если временная зона извлечена
					if(!data.empty() && !data.front().empty()){
						// Статическая таблица соответствия названий временных зон их идентификаторам
						static const unordered_map <string, zone_t> matches = {
							{"z", zone_t::UTC},
							{"ct", zone_t::CT},
							{"et", zone_t::ET},
							{"mt", zone_t::MT},
							{"nt", zone_t::NT},
							{"pt", zone_t::PT},
							{"gmt", zone_t::GMT},
							{"utc", zone_t::UTC},
							{"at", zone_t::AT},
							{"acdt", zone_t::ACDT},
							{"acst", zone_t::ACST},
							{"act", zone_t::ACT},
							{"adt", zone_t::ADT},
							{"aft", zone_t::AFT},
							{"art", zone_t::ART},
							{"azt", zone_t::AZT},
							{"bdt", zone_t::BDT},
							{"bot", zone_t::BOT},
							{"brt", zone_t::BRT},
							{"btt", zone_t::BTT},
							{"cat", zone_t::CAT},
							{"cct", zone_t::CCT},
							{"cet", zone_t::CET},
							{"cit", zone_t::CIT},
							{"ckt", zone_t::CKT},
							{"clt", zone_t::CLT},
							{"cot", zone_t::COT},
							{"cvt", zone_t::CVT},
							{"cxt", zone_t::CXT},
							{"eat", zone_t::EAT},
							{"ect", zone_t::ECT},
							{"edt", zone_t::EDT},
							{"eet", zone_t::EET},
							{"egt", zone_t::EGT},
							{"eit", zone_t::EIT},
							{"est", zone_t::EST},
							{"fet", zone_t::FET},
							{"fjt", zone_t::FJT},
							{"fkt", zone_t::FKT},
							{"fnt", zone_t::FNT},
							{"get", zone_t::GET},
							{"gft", zone_t::GFT},
							{"git", zone_t::GIT},
							{"gyt", zone_t::GYT},
							{"hkt", zone_t::HKT},
							{"ict", zone_t::ICT},
							{"idt", zone_t::IDT},
							{"jst", zone_t::JST},
							{"kgt", zone_t::KGT},
							{"kst", zone_t::KST},
							{"mdt", zone_t::MDT},
							{"mht", zone_t::MHT},
							{"mit", zone_t::MIT},
							{"mmt", zone_t::MMT},
							{"msk", zone_t::MSK},
							{"msd", zone_t::MSD},
							{"mut", zone_t::MUT},
							{"mvt", zone_t::MVT},
							{"myt", zone_t::MYT},
							{"nct", zone_t::NCT},
							{"ndt", zone_t::NDT},
							{"nft", zone_t::NFT},
							{"npt", zone_t::NPT},
							{"nrt", zone_t::NRT},
							{"nst", zone_t::NST},
							{"nut", zone_t::NUT},
							{"pdt", zone_t::PDT},
							{"pet", zone_t::PET},
							{"pgt", zone_t::PGT},
							{"pht", zone_t::PHT},
							{"pkt", zone_t::PKT},
							{"pst", zone_t::PST},
							{"pwt", zone_t::PWT},
							{"pyt", zone_t::PYT},
							{"ret", zone_t::RET},
							{"sbt", zone_t::SBT},
							{"sct", zone_t::SCT},
							{"sgt", zone_t::SGT},
							{"srt", zone_t::SRT},
							{"sst", zone_t::SST},
							{"tft", zone_t::TFT},
							{"tha", zone_t::THA},
							{"tjt", zone_t::TJT},
							{"tkt", zone_t::TKT},
							{"tlt", zone_t::TLT},
							{"tmt", zone_t::TMT},
							{"tot", zone_t::TOT},
							{"trt", zone_t::TRT},
							{"tvt", zone_t::TVT},
							{"uyt", zone_t::UYT},
							{"uzt", zone_t::UZT},
							{"vet", zone_t::VET},
							{"vut", zone_t::VUT},
							{"wat", zone_t::WAT},
							{"wet", zone_t::WET},
							{"wft", zone_t::WFT},
							{"wib", zone_t::WIB},
							{"wit", zone_t::WIT},
							{"amt", zone_t::AMTAM},
							{"ast", zone_t::ASTAL},
							{"bst", zone_t::BSTBR},
							{"cdt", zone_t::CDTNA},
							{"cst", zone_t::CSTNA},
							{"gst", zone_t::GSTPG},
							{"ist", zone_t::ISTID},
							{"mst", zone_t::MSTNA},
							{"aedt", zone_t::AEDT},
							{"akdt", zone_t::AKDT},
							{"akst", zone_t::AKST},
							{"amst", zone_t::AMST},
							{"awst", zone_t::AWST},
							{"azot", zone_t::AZOT},
							{"brst", zone_t::BRST},
							{"cest", zone_t::CEST},
							{"aest", zone_t::AEST},
							{"chot", zone_t::CHOT},
							{"chst", zone_t::CHST},
							{"chut", zone_t::CHUT},
							{"clst", zone_t::CLST},
							{"cost", zone_t::COST},
							{"davt", zone_t::DAVT},
							{"ddut", zone_t::DDUT},
							{"east", zone_t::EAST},
							{"eest", zone_t::EEST},
							{"egst", zone_t::EGST},
							{"fkst", zone_t::FKST},
							{"galt", zone_t::GALT},
							{"gamt", zone_t::GAMT},
							{"gilt", zone_t::GILT},
							{"hadt", zone_t::HADT},
							{"hast", zone_t::HAST},
							{"hovt", zone_t::HOVT},
							{"irdt", zone_t::IRDT},
							{"irkt", zone_t::IRKT},
							{"irst", zone_t::IRST},
							{"kost", zone_t::KOST},
							{"krat", zone_t::KRAT},
							{"lhdt", zone_t::LHDT},
							{"lhst", zone_t::LHST},
							{"lint", zone_t::LINT},
							{"magt", zone_t::MAGT},
							{"mart", zone_t::MART},
							{"mawt", zone_t::MAWT},
							{"mist", zone_t::MIST},
							{"nzdt", zone_t::NZDT},
							{"nzst", zone_t::NZST},
							{"omst", zone_t::OMST},
							{"orat", zone_t::ORAT},
							{"pett", zone_t::PETT},
							{"phot", zone_t::PHOT},
							{"phst", zone_t::PhST},
							{"pmdt", zone_t::PMDT},
							{"pmst", zone_t::PMST},
							{"pont", zone_t::PONT},
							{"pyst", zone_t::PYST},
							{"rott", zone_t::ROTT},
							{"sakt", zone_t::SAKT},
							{"samt", zone_t::SAMT},
							{"sast", zone_t::SAST},
							{"slst", zone_t::SLST},
							{"syot", zone_t::SYOT},
							{"taht", zone_t::TAHT},
							{"ulat", zone_t::ULAT},
							{"usz1", zone_t::USZ1},
							{"uyst", zone_t::UYST},
							{"vlat", zone_t::VLAT},
							{"volt", zone_t::VOLT},
							{"vost", zone_t::VOST},
							{"wakt", zone_t::WAKT},
							{"wast", zone_t::WAST},
							{"west", zone_t::WEST},
							{"yakt", zone_t::YAKT},
							{"yekt", zone_t::YEKT},
							{"wgst", zone_t::WGSTST},
							{"chadt", zone_t::CHADT},
							{"chast", zone_t::CHAST},
							{"chost", zone_t::CHOST},
							{"acwst", zone_t::ACWST},
							{"azost", zone_t::AZOST},
							{"easst", zone_t::EASST},
							{"hovst", zone_t::HOVST},
							{"ulast", zone_t::ULAST}
						};
						// Приводим извлечённое название временной зоны к нижнему регистру
						string name = data.front();
						// Выполняем поиск временной зоны в таблице соответствия
						auto j = matches.find(this->_fmk->transform(name, fmk_t::transform_t::LOWER_CASE));
						// Если временная зона найдена в таблице соответствия
						if(j != matches.end())
							// Устанавливаем найденную временную зону
							result = j->second;
					}
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(zone), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
			// Переменная результата
			result = zone_t::NONE;
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод выполнения матчинга временной зоны
 *
 * @param storage хранение значение времени
 * @return        определённая временная зона
 *
 */
awh::Chrono::zone_t awh::Chrono::matchTimeZone(const storage_t storage) const noexcept {
	// Переменная результата
	zone_t result = zone_t::NONE;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем хранилище значение времени
		 */
		switch(static_cast <uint8_t> (storage)){
			// Если хранилище локальное
			case static_cast <uint8_t> (storage_t::LOCAL):
				// Получаем локальное значение временной зоны
				result = this->_dt.zone;
			break;
			// Если хранилище глобальное
			case static_cast <uint8_t> (storage_t::GLOBAL):
				// Получаем глобальное значение временной зоны
				result = zone_t::UTC;
			break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (storage)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод перевода временной зоны в смещение
 *
 * @param zone временная зона для конвертации
 * @return     смещение временной зоны в секундах
 *
 */
int32_t awh::Chrono::getTimeZone(const zone_t zone) const noexcept {
	/**
	 * Определяем временную зону
	 */
	switch(static_cast <uint8_t> (zone)){
		// Если временная зона не установлена
		case static_cast <uint8_t> (zone_t::NONE):
			// Возвращаем значение локальной временной зоны
			return this->_dt.offset;
		// Если временная зона установлена как (Атлантическое Время)
		case static_cast <uint8_t> (zone_t::AT):
			// Выполняем определение точного значение временной зоны
			return this->getTimeZone(zone_t::ASTAL, zone_t::ADT);
		// Если временная зона установлена как (Северноамериканское Центральное Время)
		case static_cast <uint8_t> (zone_t::CT):
			// Выполняем определение точного значение временной зоны
			return this->getTimeZone(zone_t::CSTNA, zone_t::CDTNA);
		// Если временная зона установлена как (Северноамериканское Восточное Время)
		case static_cast <uint8_t> (zone_t::ET):
			// Выполняем определение точного значение временной зоны
			return this->getTimeZone(zone_t::EST, zone_t::EDT);
		// Если временная зона установлена как (Северноамериканское Горное Время)
		case static_cast <uint8_t> (zone_t::MT):
			// Выполняем определение точного значение временной зоны
			return this->getTimeZone(zone_t::MSTNA, zone_t::MDT);
		// Если временная зона установлена как (Северноамериканское Тихоокеанское Время)
		case static_cast <uint8_t> (zone_t::PT):
			// Выполняем определение точного значение временной зоны
			return this->getTimeZone(zone_t::PST, zone_t::PDT);
		// Если временная зона установлена как (Время На Острове Ниуэ)
		case static_cast <uint8_t> (zone_t::NUT):
		// Если временная зона установлена как (Стандартное Время На Острове Самоа)
		case static_cast <uint8_t> (zone_t::SST):
			// Формируем смещение временной зоны (UTC-11)
			return -39600;
		// Если временная зона установлена как (Стандартное Время На Островах Кука)
		case static_cast <uint8_t> (zone_t::CKT):
		// Если временная зона установлена как (Гавайско-Алеутское Стандартное Время)
		case static_cast <uint8_t> (zone_t::HAST):
		// Если временная зона установлена как (Время На Острове Таити)
		case static_cast <uint8_t> (zone_t::TAHT):
			// Формируем смещение временной зоны (UTC-10)
			return -36000;
		// Если временная зона установлена как (Время На Маркизских Островах)
		case static_cast <uint8_t> (zone_t::MIT):
		// Если временная зона установлена как (Время На Маркизских Островах)
		case static_cast <uint8_t> (zone_t::MART):
			// Формируем смещение временной зоны (UTC-9:30)
			return -34200;
		// Если временная зона установлена как (Время На О. Гамбье)
		case static_cast <uint8_t> (zone_t::GIT):
		// Если временная зона установлена как (Стандартное Время На Аляске)
		case static_cast <uint8_t> (zone_t::AKST):
		// Если временная зона установлена как (Время На Острове Гамбье)
		case static_cast <uint8_t> (zone_t::GAMT):
		// Если временная зона установлена как (Гавайско-Алеутское Летнее Время)
		case static_cast <uint8_t> (zone_t::HADT):
			// Формируем смещение временной зоны (UTC-9)
			return -32400;
		// Если временная зона установлена как (Северноамериканское Тихоокеанское Стандартное Время)
		case static_cast <uint8_t> (zone_t::PST):
		// Если временная зона установлена как (Летнее Время На Аляске)
		case static_cast <uint8_t> (zone_t::AKDT):
			// Формируем смещение временной зоны (UTC-8)
			return -28800;
		// Если временная зона установлена как (Северноамериканское Тихоокеанское Летнее Время)
		case static_cast <uint8_t> (zone_t::PDT):
		// Если временная зона установлена как (Северноамериканское Горное Стандартное Время)
		case static_cast <uint8_t> (zone_t::MSTNA):
			// Формируем смещение временной зоны (UTC-7)
			return -25200;
		// Если временная зона установлена как (Северноамериканское Горное Летнее Время)
		case static_cast <uint8_t> (zone_t::MDT):
		// Если временная зона установлена как (Стандартное Время На Острове Пасхи)
		case static_cast <uint8_t> (zone_t::EAST):
		// Если временная зона установлена как (Время На Галапагосских Островах)
		case static_cast <uint8_t> (zone_t::GALT):
		// Если временная зона установлена как (Северноамериканское Центральное Стандартное Время)
		case static_cast <uint8_t> (zone_t::CSTNA):
			// Формируем смещение временной зоны (UTC-6)
			return -21600;
		// Если временная зона установлена как (Амазонское Стандартное Время)
		case static_cast <uint8_t> (zone_t::ACT):
		// Если временная зона установлена как (Колумбийское Стандартное Время)
		case static_cast <uint8_t> (zone_t::COT):
		// Если временная зона установлена как (Эквадорское Время)
		case static_cast <uint8_t> (zone_t::ECT):
		// Если временная зона установлена как (Северноамериканское Восточное Стандартное Время)
		case static_cast <uint8_t> (zone_t::EST):
		// Если временная зона установлена как (Стандартное Время В Перу)
		case static_cast <uint8_t> (zone_t::PET):
		// Если временная зона установлена как (Летнее Время На Острове Пасхи)
		case static_cast <uint8_t> (zone_t::EASST):
		// Если временная зона установлена как (Северноамериканское Центральное Летнее Время)
		case static_cast <uint8_t> (zone_t::CDTNA):
		// Если временная зона установлена как (Кубинское Стандартное Время)
		case static_cast <uint8_t> (zone_t::CSTCB):
			// Формируем смещение временной зоны (UTC-5)
			return -18000;
		// Если временная зона установлена как (Парагвайское Стандартное Время)
		case static_cast <uint8_t> (zone_t::PYT):
		// Если временная зона установлена как (Стандартное Время На Фолклендах)
		case static_cast <uint8_t> (zone_t::FKT):
		// Если временная зона установлена как (Боливийское Время)
		case static_cast <uint8_t> (zone_t::BOT):
		// Если временная зона установлена как (Чилийское Стандартное Время)
		case static_cast <uint8_t> (zone_t::CLT):
		// Если временная зона установлена как (Северноамериканское Восточное Летнее Время)
		case static_cast <uint8_t> (zone_t::EDT):
		// Если временная зона установлена как (Время В Гайане)
		case static_cast <uint8_t> (zone_t::GYT):
		// Если временная зона установлена как (Время В Венесуеле)
		case static_cast <uint8_t> (zone_t::VET):
		// Если временная зона установлена как (Колумбийское Летнее Время)
		case static_cast <uint8_t> (zone_t::COST):
		// Если временная зона установлена как (Амазонское Стандартное Время)
		case static_cast <uint8_t> (zone_t::AMTAM):
		// Если временная зона установлена как (Атлантическое Стандартное Время)
		case static_cast <uint8_t> (zone_t::ASTAL):
		// Если временная зона установлена как (Кубинское Летнее Время)
		case static_cast <uint8_t> (zone_t::CDTCB):
			// Формируем смещение временной зоны (UTC-4)
			return -14400;
		// Если временная зона установлена как (Время В Ньюфаундленде)
		case static_cast <uint8_t> (zone_t::NT):
		// Если временная зона установлена как (Стандартное Время В Ньюфаундленде)
		case static_cast <uint8_t> (zone_t::NST):
			// Формируем смещение временной зоны (UTC-3:30)
			return -12600;
		// Если временная зона установлена как (Время В Суринаме)
		case static_cast <uint8_t> (zone_t::SRT):
		// Если временная зона установлена как (Атлантическое Летнее Время)
		case static_cast <uint8_t> (zone_t::ADT):
		// Если временная зона установлена как (Аргентинское Стандартное Время)
		case static_cast <uint8_t> (zone_t::ART):
		// Если временная зона установлена как (Бразильское Стандартное Время)
		case static_cast <uint8_t> (zone_t::BRT):
		// Если временная зона установлена как (Время В Французской Гвиане)
		case static_cast <uint8_t> (zone_t::GFT):
		// Если временная зона установлена как (Стандартное Время В Уругвае)
		case static_cast <uint8_t> (zone_t::UYT):
		// Если временная зона установлена как (Амазонка, Летнее Время)
		case static_cast <uint8_t> (zone_t::AMST):
		// Если временная зона установлена как (Чилийское Летнее Время)
		case static_cast <uint8_t> (zone_t::CLST):
		// Если временная зона установлена как (Летнее Время На Фолклендах)
		case static_cast <uint8_t> (zone_t::FKST):
		// Если временная зона установлена как (Парагвайское Летнее Время)
		case static_cast <uint8_t> (zone_t::PYST):
		// Если временная зона установлена как (Стандартное Время На Островах Сен-Пьер И Микелон)
		case static_cast <uint8_t> (zone_t::PMST):
		// Если временная зона установлена как (Время На Станции Ротера)
		case static_cast <uint8_t> (zone_t::ROTT):
		// Если временная зона установлена как (Стандартное Время В Западной Гренландии)
		case static_cast <uint8_t> (zone_t::WGST):
			// Формируем смещение временной зоны (UTC-3)
			return -10800;
		// Если временная зона установлена как (Летнее Время В Ньюфаундленде)
		case static_cast <uint8_t> (zone_t::NDT):
			// Формируем смещение временной зоны (UTC-2:30)
			return -9000;
		// Если временная зона установлена как (Стандартное Время На Фернанду-Ди-Норонья)
		case static_cast <uint8_t> (zone_t::FNT):
		// Если временная зона установлена как (Бразильское Летнее Время)
		case static_cast <uint8_t> (zone_t::BRST):
		// Если временная зона установлена как (Летнее Время На Островах Сен-Пьер И Микелон)
		case static_cast <uint8_t> (zone_t::PMDT):
		// Если временная зона установлена как (Летнее Время В Уругвае)
		case static_cast <uint8_t> (zone_t::UYST):
		// Если временная зона установлена как (Время В Южной Георгии)
		case static_cast <uint8_t> (zone_t::GSTSG):
		// Если временная зона установлена как (Летнее Время В Западной Гренландии)
		case static_cast <uint8_t> (zone_t::WGSTST):
			// Формируем смещение временной зоны (UTC-2)
			return -7200;
		// Если временная зона установлена как (Стандартное Время На Островах Кабо-Верде)
		case static_cast <uint8_t> (zone_t::CVT):
		// Если временная зона установлена как (Стандартное Время В Восточной Гренландии)
		case static_cast <uint8_t> (zone_t::EGT):
		// Если временная зона установлена как (Стандартное Время На Азорских Островах)
		case static_cast <uint8_t> (zone_t::AZOT):
			// Формируем смещение временной зоны (UTC-1)
			return -3600;
		// Если временная зона установлена как (Среднее Время По Гринвичу)
		case static_cast <uint8_t> (zone_t::GMT):
		// Если временная зона установлена как (Всемирное Координированное Время)
		case static_cast <uint8_t> (zone_t::UTC):
		// Если временная зона установлена как (Западноевропейское Стандартное Время)
		case static_cast <uint8_t> (zone_t::WET):
		// Если временная зона установлена как (Летнее Время В Восточной Гренландии)
		case static_cast <uint8_t> (zone_t::EGST):
		// Если временная зона установлена как (Летнее Время На Азорских Островах)
		case static_cast <uint8_t> (zone_t::AZOST):
			// Формируем смещение временной зоны (UTC+0)
			return 0;
		// Если временная зона установлена как (Центральноевропейское Стандартное Время)
		case static_cast <uint8_t> (zone_t::CET):
		// Если временная зона установлена как (Западноафриканское Стандартное Время)
		case static_cast <uint8_t> (zone_t::WAT):
		// Если временная зона установлена как (Западноевропейское Летнее Время)
		case static_cast <uint8_t> (zone_t::WEST):
		// Если временная зона установлена как (Британское Летнее Время)
		case static_cast <uint8_t> (zone_t::BSTBR):
		// Если временная зона установлена как (Ирландия, Летнее Время)
		case static_cast <uint8_t> (zone_t::ISTIR):
			// Формируем смещение временной зоны (UTC+1)
			return 3600;
		// Если временная зона установлена как (Восточноафриканское Время)
		case static_cast <uint8_t> (zone_t::CAT):
		// Если временная зона установлена как (Восточноевропейское Стандартное Время)
		case static_cast <uint8_t> (zone_t::EET):
		// Если временная зона установлена как (Центральноевропейское Летнее Время)
		case static_cast <uint8_t> (zone_t::CEST):
		// Если временная зона установлена как (Южноафриканское Время)
		case static_cast <uint8_t> (zone_t::SAST):
		// Если временная зона установлена как (Калининградское Время)
		case static_cast <uint8_t> (zone_t::USZ1):
		// Если временная зона установлена как (Западноафриканское Летнее Время)
		case static_cast <uint8_t> (zone_t::WAST):
		// Если временная зона установлена как (Израильское Стандартное Время)
		case static_cast <uint8_t> (zone_t::ISTIS):
			// Формируем смещение временной зоны (UTC+2)
			return 7200;
		// Если временная зона установлена как (Минское Время)
		case static_cast <uint8_t> (zone_t::FET):
		// Если временная зона установлена как (Турецкое Время)
		case static_cast <uint8_t> (zone_t::TRT):
		// Если временная зона установлена как (Восточноафриканский Час)
		case static_cast <uint8_t> (zone_t::EAT):
		// Если временная зона установлена как (Израильское Летнее Время)
		case static_cast <uint8_t> (zone_t::IDT):
		// Если временная зона установлена как (Московское Время)
		case static_cast <uint8_t> (zone_t::MSK):
		// Если временная зона установлена как (Восточноевропейское Летнее Время)
		case static_cast <uint8_t> (zone_t::EEST):
		// Если временная зона установлена как (Время На Станции Сёва)
		case static_cast <uint8_t> (zone_t::SYOT):
		// Если временная зона установлена как (Стандартное Время В Саудовской Аравии)
		case static_cast <uint8_t> (zone_t::ASTSA):
			// Формируем смещение временной зоны (UTC+3)
			return 10800;
		// Если временная зона установлена как (Иранское Стандартное Время)
		case static_cast <uint8_t> (zone_t::IRST):
			// Формируем смещение временной зоны (UTC+3:30)
			return 12600;
		// Если временная зона установлена как (Стандартное Время На Острове Маврикий)
		case static_cast <uint8_t> (zone_t::MUT):
		// Если временная зона установлена как (Время На Острове Реюньон)
		case static_cast <uint8_t> (zone_t::RET):
		// Если временная зона установлена как (Время На Сейшелах)
		case static_cast <uint8_t> (zone_t::SCT):
		// Если временная зона установлена как (Азербайджанское Стандартное Время)
		case static_cast <uint8_t> (zone_t::AZT):
		// Если временная зона установлена как (Грузинское Стандартное Время)
		case static_cast <uint8_t> (zone_t::GET):
		// Если временная зона установлена как (Московское Летнее Время)
		case static_cast <uint8_t> (zone_t::MSD):
		// Если временная зона установлена как (Самарское Время)
		case static_cast <uint8_t> (zone_t::SAMT):
		// Если временная зона установлена как (Волгоградское Время)
		case static_cast <uint8_t> (zone_t::VOLT):
		// Если временная зона установлена как (Армянское Стандартное Время)
		case static_cast <uint8_t> (zone_t::AMTAR):
		// Если временная зона установлена как (Время В Персидском Заливе)
		case static_cast <uint8_t> (zone_t::GSTPG):
			// Формируем смещение временной зоны (UTC+4)
			return 14400;
		// Если временная зона установлена как (Время В Афганистане)
		case static_cast <uint8_t> (zone_t::AFT):
		// Если временная зона установлена как (Иранское Летнее Время)
		case static_cast <uint8_t> (zone_t::IRDT):
			// Формируем смещение временной зоны (UTC+4:30)
			return 16200;
		// Если временная зона установлена как (Время На Мальдивах)
		case static_cast <uint8_t> (zone_t::MVT):
		// Если временная зона установлена как (Французское Южное И Антарктическое Время)
		case static_cast <uint8_t> (zone_t::TFT):
		// Если временная зона установлена как (Время В Таджикистане)
		case static_cast <uint8_t> (zone_t::TJT):
		// Если временная зона установлена как (Стандартное Время В Туркмении)
		case static_cast <uint8_t> (zone_t::TMT):
		// Если временная зона установлена как (Пакистанское Стандартное Время)
		case static_cast <uint8_t> (zone_t::PKT):
		// Если временная зона установлена как (Время В Узбекистане)
		case static_cast <uint8_t> (zone_t::UZT):
		// Если временная зона установлена как (Время На Станции Моусон)
		case static_cast <uint8_t> (zone_t::MAWT):
		// Если временная зона установлена как (Время В Западном Казахстане)
		case static_cast <uint8_t> (zone_t::ORAT):
		// Если временная зона установлена как (Екатеринбургское Время)
		case static_cast <uint8_t> (zone_t::YEKT):
			// Формируем смещение временной зоны (UTC+5)
			return 18000;
		// Если временная зона установлена как (Стандартное Время В Шри-Ланке)
		case static_cast <uint8_t> (zone_t::SLST):
		// Если временная зона установлена как (Индийское Стандартное Время)
		case static_cast <uint8_t> (zone_t::ISTID):
			// Формируем смещение временной зоны (UTC+5:30)
			return 19800;
		// Если временная зона установлена как (Непальское Время)
		case static_cast <uint8_t> (zone_t::NPT):
			// Формируем смещение временной зоны (UTC+5:45)
			return 20700;
		// Если временная зона установлена как (Бутанское Время)
		case static_cast <uint8_t> (zone_t::BTT):
		// Если временная зона установлена как (Время В Киргизии)
		case static_cast <uint8_t> (zone_t::KGT):
		// Если временная зона установлена как (Омское Время)
		case static_cast <uint8_t> (zone_t::OMST):
		// Если временная зона установлена как (Время На Станции Восток)
		case static_cast <uint8_t> (zone_t::VOST):
		// Если временная зона установлена как (Стандартное Время В Бангладеш)
		case static_cast <uint8_t> (zone_t::BSTBL):
			// Формируем смещение временной зоны (UTC+6)
			return 21600;
		// Если временная зона установлена как (Время На Кокосовые Островах)
		case static_cast <uint8_t> (zone_t::CCT):
		// Если временная зона установлена как (Время В Мьянме)
		case static_cast <uint8_t> (zone_t::MMT):
			// Формируем смещение временной зоны (UTC+6:30)
			return 23400;
		// Если временная зона установлена как (Тайландское Время)
		case static_cast <uint8_t> (zone_t::THA):
		// Если временная зона установлена как (Время На Острове Рождества)
		case static_cast <uint8_t> (zone_t::CXT):
		// Если временная зона установлена как (Время В Индокитае)
		case static_cast <uint8_t> (zone_t::ICT):
		// Если временная зона установлена как (Время В Западной Индонезии)
		case static_cast <uint8_t> (zone_t::WIB):
		// Если временная зона установлена как (Дейвис)
		case static_cast <uint8_t> (zone_t::DAVT):
		// Если временная зона установлена как (Стандартное Время В Ховде)
		case static_cast <uint8_t> (zone_t::HOVT):
		// Если временная зона установлена как (Красноярское Стандартное Время)
		case static_cast <uint8_t> (zone_t::KRAT):
			// Формируем смещение временной зоны (UTC+7)
			return 25200;
		// Если временная зона установлена как (Малайское Время)
		case static_cast <uint8_t> (zone_t::MYT):
		// Если временная зона установлена как (Сингапурское Время)
		case static_cast <uint8_t> (zone_t::SGT):
		// Если временная зона установлена как (Время В Бруней-Даруссаламе)
		case static_cast <uint8_t> (zone_t::BDT):
		// Если временная зона установлена как (Время В Бруней-Даруссаламе)
		case static_cast <uint8_t> (zone_t::BNT):
		// Если временная зона установлена как (Время В Центральной Индонезии)
		case static_cast <uint8_t> (zone_t::CIT):
		// Если временная зона установлена как (Гонконгское Стандартное Время)
		case static_cast <uint8_t> (zone_t::HKT):
		// Если временная зона установлена как (Стандартное Время На Филлипинах)
		case static_cast <uint8_t> (zone_t::PHT):
		// Если временная зона установлена как (Стандартное Время В Западной Австралии)
		case static_cast <uint8_t> (zone_t::AWST):
		// Если временная зона установлена как (Стандартное Время В Чойлобалсане)
		case static_cast <uint8_t> (zone_t::CHOT):
		// Если временная зона установлена как (Иркутское Стандартное Время)
		case static_cast <uint8_t> (zone_t::IRKT):
		// Если временная зона установлена как (Стандартное Время На Филлипинах)
		case static_cast <uint8_t> (zone_t::PhST):
		// Если временная зона установлена как (Стандартное Время В Монголии)
		case static_cast <uint8_t> (zone_t::ULAT):
		// Если временная зона установлена как (Летнее Время В Ховде)
		case static_cast <uint8_t> (zone_t::HOVST):
		// Если временная зона установлена как (Китайское Стандартное Время)
		case static_cast <uint8_t> (zone_t::CSTKT):
		// Если временная зона установлена как (Время В Малайзии)
		case static_cast <uint8_t> (zone_t::MSTMS):
			// Формируем смещение временной зоны (UTC+8)
			return 28800;
		// Если временная зона установлена как (Центрально-Западная Австралия, Стандартное Время)
		case static_cast <uint8_t> (zone_t::ACWST):
			// Формируем смещение временной зоны (UTC+8:45)
			return 31500;
		// Если временная зона установлена как (Время На Острове Палау)
		case static_cast <uint8_t> (zone_t::PWT):
		// Если временная зона установлена как (Время В Восточном Тиморе)
		case static_cast <uint8_t> (zone_t::TLT):
		// Если временная зона установлена как (Время В Восточной Индонезии)
		case static_cast <uint8_t> (zone_t::EIT):
		// Если временная зона установлена как (Японское Стандартное Время)
		case static_cast <uint8_t> (zone_t::JST):
		// Если временная зона установлена как (Корейское Стандартное Время)
		case static_cast <uint8_t> (zone_t::KST):
		// Если временная зона установлена как (Время В Восточной Индонезии)
		case static_cast <uint8_t> (zone_t::WIT):
		// Если временная зона установлена как (Якутское Время)
		case static_cast <uint8_t> (zone_t::YAKT):
		// Если временная зона установлена как (Летнее Время В Чойлобалсане)
		case static_cast <uint8_t> (zone_t::CHOST):
		// Если временная зона установлена как (Летнее Время В Монголии)
		case static_cast <uint8_t> (zone_t::ULAST):
			// Формируем смещение временной зоны (UTC+9)
			return 32400;
		// Если временная зона установлена как (Стандартное Время В Центральной Австралии)
		case static_cast <uint8_t> (zone_t::ACST):
			// Формируем смещение временной зоны (UTC+9:30)
			return 34200;
		// Если временная зона установлена как (Время В Папуа-Новой Гвинее)
		case static_cast <uint8_t> (zone_t::PGT):
		// Если временная зона установлена как (Стандартное Время В Восточной Австралии)
		case static_cast <uint8_t> (zone_t::AEST):
		// Если временная зона установлена как (Час Чаморро)
		case static_cast <uint8_t> (zone_t::CHST):
		// Если временная зона установлена как (Время На Островах Чуук)
		case static_cast <uint8_t> (zone_t::CHUT):
		// Если временная зона установлена как (Дюмон-Д'юрвиль)
		case static_cast <uint8_t> (zone_t::DDUT):
		// Если временная зона установлена как (Владивостокское Время)
		case static_cast <uint8_t> (zone_t::VLAT):
			// Формируем смещение временной зоны (UTC+10)
			return 36000;
		// Если временная зона установлена как (Летнее Время В Центральной Австралии)
		case static_cast <uint8_t> (zone_t::ACDT):
		// Если временная зона установлена как (Стандартное Время На Лорд-Хау)
		case static_cast <uint8_t> (zone_t::LHST):
			// Формируем смещение временной зоны (UTC+10:30)
			return 37800;
		// Если временная зона установлена как (Стандартное Время В Новой Каледонии)
		case static_cast <uint8_t> (zone_t::NCT):
		// Если временная зона установлена как (Время На Острове Норфолк)
		case static_cast <uint8_t> (zone_t::NFT):
		// Если временная зона установлена как (Время На Соломоновых Островах)
		case static_cast <uint8_t> (zone_t::SBT):
		// Если временная зона установлена как (Стандартное Время На Островах Вануату)
		case static_cast <uint8_t> (zone_t::VUT):
		// Если временная зона установлена как (Летнее Время В Восточной Австралии)
		case static_cast <uint8_t> (zone_t::AEDT):
		// Если временная зона установлена как (Время На Острове Косраэ)
		case static_cast <uint8_t> (zone_t::KOST):
		// Если временная зона установлена как (Летнее Время На Лорд-Хау)
		case static_cast <uint8_t> (zone_t::LHDT):
		// Если временная зона установлена как (Магаданское Стандартное Время)
		case static_cast <uint8_t> (zone_t::MAGT):
		// Если временная зона установлена как (Время На Станции Маккуори)
		case static_cast <uint8_t> (zone_t::MIST):
		// Если временная зона установлена как (Время На Острове Понапе)
		case static_cast <uint8_t> (zone_t::PONT):
		// Если временная зона установлена как (Сахалинское Стандартное Время)
		case static_cast <uint8_t> (zone_t::SAKT):
			// Формируем смещение временной зоны (UTC+11)
			return 39600;
		// Если временная зона установлена как (Время На Острове Науру)
		case static_cast <uint8_t> (zone_t::NRT):
		// Если временная зона установлена как (Летнее Время На О. Фиджи)
		case static_cast <uint8_t> (zone_t::FJT):
		// Если временная зона установлена как (Время На Островах Тувалу)
		case static_cast <uint8_t> (zone_t::TVT):
		// Если временная зона установлена как (Время На Маршалловых Островах)
		case static_cast <uint8_t> (zone_t::MHT):
		// Если временная зона установлена как (Время На Островах Уоллис И Футуна)
		case static_cast <uint8_t> (zone_t::WFT):
		// Если временная зона установлена как (Время На Островах Гилберта)
		case static_cast <uint8_t> (zone_t::GILT):
		// Если временная зона установлена как (Стандартное Время В Новой Зеландии)
		case static_cast <uint8_t> (zone_t::NZST):
		// Если временная зона установлена как (Камчатское Время)
		case static_cast <uint8_t> (zone_t::PETT):
		// Если временная зона установлена как (Время На Острове Уэйк)
		case static_cast <uint8_t> (zone_t::WAKT):
			// Формируем смещение временной зоны (UTC+12)
			return 43200;
		// Если временная зона установлена как (Стандартное Время На Архипелаге Чатем)
		case static_cast <uint8_t> (zone_t::CHAST):
			// Формируем смещение временной зоны (UTC+12:45)
			return 45900;
		// Если временная зона установлена как (Время На Островах Токелау)
		case static_cast <uint8_t> (zone_t::TKT):
		// Если временная зона установлена как (Время На Островах Тонга)
		case static_cast <uint8_t> (zone_t::TOT):
		// Если временная зона установлена как (Летнее Время В Новой Зеландии)
		case static_cast <uint8_t> (zone_t::NZDT):
		// Если временная зона установлена как (Время На Островах Феникс)
		case static_cast <uint8_t> (zone_t::PHOT):
			// Формируем смещение временной зоны (UTC+13)
			return 46800;
		// Если временная зона установлена как (Летнее Время На Архипелаге Чатем)
		case static_cast <uint8_t> (zone_t::CHADT):
			// Формируем смещение временной зоны (UTC+13:45)
			return 49500;
		// Если временная зона установлена как (Время На Острове Лайн)
		case static_cast <uint8_t> (zone_t::LINT):
			// Формируем смещение временной зоны (UTC+14)
			return 50400;
	}
	// Возвращаем результат
	return this->_dt.offset;
}
/**
 * @brief Метод перевода временной зоны в смещение
 *
 * @param zone временная зона для конвертации
 * @return     смещение временной зоны в секундах
 *
 */
int32_t awh::Chrono::getTimeZone(string_view zone) const noexcept {
	// Переменная результата
	int32_t result = this->_dt.offset;
	// Если временная зона указана
	if(!zone.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем нативный разбор временной зоны со смещением (формат %e)
			const vector <match_t> match = ::parseZoneFull(zone.data(), zone.size());
			// Если совпадение получено
			if(!match.empty()){
				// Обрабатываем полученные группы совпадения
				{
					// Создаём массив собранных результатов
					vector <string> data(match.size());
					/**
					 * Выполняем перебор всех полученных вариантов
					 */
					for(uint8_t j = 0; j < static_cast <uint8_t> (match.size()); j++){
						// Если результат получен
						if(match[j].end > match[j].begin)
							// Выполняем установку результата
							data[j].assign(zone.data() + match[j].begin, match[j].end - match[j].begin);
					}
					// Если временная зона извлечена
					if(!data.empty() && (data.size() > 1)){
						// Получаем название временной зоны
						string name = data[1];
						// Выполняем поиск временной зоны в списке временных зон
						auto i = this->_timeZones.find(this->_fmk->transform(name, fmk_t::transform_t::LOWER_CASE));
						// Если временная зона найдена
						if(i != this->_timeZones.end())
							// Устанавливаем значение временной зоны
							result = i->second;
						// Если временная зона не найдена
						else {
							// Если название временной зоны не получено
							if(name.empty()){
								// Если смещение времени указано
								if((data.size() > 5) && !data[4].empty()){
									// Получаем смещение времени
									const string & offset = data[4];
									// Если полученное смещение является числом
									if(this->_fmk->is(offset, fmk_t::check_t::NUMBER)){
										// Если указано 4 символа
										if(offset.size() == 4){
											// Получаем количество часов
											const uint8_t hour = this->_fmk->atoi <uint8_t> (offset.c_str(), 2);
											// Получаем количество минут
											const uint8_t minutes = this->_fmk->atoi <uint8_t> (offset.c_str() + 2, offset.length() - 2);
											// Если смещение времени положительное
											if(data[3].compare("+") == 0)
												// Получаем время смещения
												result += static_cast <int32_t> ((hour * 60 * 60) + (minutes * 60));
											// Устанавливаем отрицательное смещение времени
											else result -= static_cast <int32_t> ((hour * 60 * 60) + (minutes * 60));
										// Если установлен всего один символ
										} else {
											// Если смещение времени положительное
											if(data[3].compare("+") == 0)
												// Получаем время смещения
												result += (this->_fmk->atoi <int32_t> (offset) * 60 * 60);
											// Устанавливаем отрицательное смещение времени
											else result -= (this->_fmk->atoi <int32_t> (offset) * 60 * 60);
										}
									// Если получено время в формате часов
									} else if((data.size() > 6) && !data[5].empty() && !data[6].empty()) {
										// Получаем количество часов
										const uint8_t hour = this->_fmk->atoi <uint8_t> (data[5]);
										// Получаем количество минут
										const uint8_t minutes = this->_fmk->atoi <uint8_t> (data[6]);
										// Если смещение времени положительное
										if(data[3].compare("+") == 0)
											// Получаем время смещения
											result += static_cast <int32_t> ((hour * 60 * 60) + (minutes * 60));
										// Устанавливаем отрицательное смещение времени
										else result -= static_cast <int32_t> ((hour * 60 * 60) + (minutes * 60));
									}
								}
							// Если название временной зоны является числом
							} else if(this->_fmk->is(name, fmk_t::check_t::NUMBER))
								// Получаем время смещения
								result += (this->_fmk->atoi <int32_t> (name) * 60 * 60);
							// Если временная зона получена в виде названия
							else {
								// Получаем смещение времени по найденной в таблице соответствия временной зоне
								result = this->getTimeZone(this->matchTimeZone(name));
								// Если смещение времени указано
								if((data.size() > 5) && !data[4].empty()){
									// Получаем смещение времени
									const string & offset = data[4];
									// Если полученное смещение является числом
									if(this->_fmk->is(offset, fmk_t::check_t::NUMBER)){
										// Если указано 4 символа
										if(offset.size() == 4){
											// Получаем количество часов
											const uint8_t hour = this->_fmk->atoi <uint8_t> (offset.c_str(), 2);
											// Получаем количество минут
											const uint8_t minutes = this->_fmk->atoi <uint8_t> (offset.c_str() + 2, offset.length() - 2);
											// Если смещение времени положительное
											if(data[3].compare("+") == 0)
												// Получаем время смещения
												result += static_cast <int32_t> ((hour * 60 * 60) + (minutes * 60));
											// Устанавливаем отрицательное смещение времени
											else result -= static_cast <int32_t> ((hour * 60 * 60) + (minutes * 60));
										// Если установлен всего один символ
										} else {
											// Если смещение времени положительное
											if(data[3].compare("+") == 0)
												// Получаем время смещения
												result += (this->_fmk->atoi <int32_t> (offset) * 60 * 60);
											// Устанавливаем отрицательное смещение времени
											else result -= (this->_fmk->atoi <int32_t> (offset) * 60 * 60);
										}
									// Если получено время в формате часов
									} else if((data.size() > 6) && !data[5].empty() && !data[6].empty()) {
										// Получаем количество часов
										const uint8_t hour = this->_fmk->atoi <uint8_t> (data[5]);
										// Получаем количество минут
										const uint8_t minutes = this->_fmk->atoi <uint8_t> (data[6]);
										// Если смещение времени положительное
										if(data[3].compare("+") == 0)
											// Получаем время смещения
											result += static_cast <int32_t> ((hour * 60 * 60) + (minutes * 60));
										// Устанавливаем отрицательное смещение времени
										else result -= static_cast <int32_t> ((hour * 60 * 60) + (minutes * 60));
									}
								}
							}
						}
					}
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(zone), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
			// Переменная результата
			result = this->_dt.offset;
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод определения текущей временной зоны относительно летнего времени
 *
 * @param std временная зона стандартного времени
 * @param sum временная зона летнего времени
 * @return    смещение временной зоны в секундах
 *
 */
int32_t awh::Chrono::getTimeZone(const zone_t std, const zone_t sum) const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Создаем структуру времени
		dt_t dt;
		// Получаем стандартное время
		const int32_t result = this->getTimeZone(std);
		// Заполняем объект даты из штампа времени
		this->makeDate(this->timestamp(type_t::MILLISECONDS), dt);
		// Если время летнее
		if(dt.dst)
			// Получаем результат для летнего времени
			return this->getTimeZone(sum);
		// Возвращаем результат
		return result;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (std), static_cast <uint16_t> (sum)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return this->_dt.offset;
}
/**
 * @brief Метод получения установленной временной зоны
 *
 * @param storage хранение значение времени
 * @return        смещение временной зоны в секундах
 *
 */
int32_t awh::Chrono::getTimeZone(const storage_t storage) const noexcept {
	// Переменная результата
	int32_t result = 0;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем хранилище значение времени
		 */
		switch(static_cast <uint8_t> (storage)){
			// Если хранилище локальное
			case static_cast <uint8_t> (storage_t::LOCAL):
				// Получаем локальное значение временной зоны в секундах
				result = this->_dt.offset;
			break;
			// Если хранилище глобальное
			case static_cast <uint8_t> (storage_t::GLOBAL): {
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Устанавливаем временную зону по умолчанию
					::_tzset();
					// Получаем глобальное значение временной зоны в секундах
					result = static_cast <int32_t> (_timezone * -1);
				/**
				 * Для операционной системы FreeBSD, NetBSD или OpenBSD
				 */
				#elif __FreeBSD__ || __NetBSD__ || __OpenBSD__
					// Устанавливаем временную зону по умолчанию
					::tzset();
					// Создаем структуру времени
					std::tm tm = {};
					// Получаем значение текущего времени
					const time_t time = std::time(nullptr);
					// Заполняем структуру времени
					localtime_r(&time, &tm);
					// Получаем смещение временной зоны в секундах
					result = static_cast <int32_t> (tm.tm_gmtoff);
				/**
				 * Для всех остальных операционных систем
				 */
				#else
					// Устанавливаем временную зону по умолчанию
					::tzset();
					// Получаем глобальное значение временной зоны в секундах
					result = static_cast <int32_t> (timezone * -1);
				#endif
			} break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (storage)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод очистки списка временных зон
 *
 */
void awh::Chrono::clearTimeZones() noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока
		const locker_t <> lock(this->_mtx.tz);
		// Выполняем очистку списка временных зон
		this->_timeZones.clear();
		// Выполняем освобождение выделенной памяти
		unordered_map <decltype(this->_timeZones)::key_type, decltype(this->_timeZones)::mapped_type> ().swap(this->_timeZones);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод добавления собственной временной зоны
 *
 * @param name   название временной зоны
 * @param offset смещение времени в секундах
 *
 */
void awh::Chrono::addTimeZone(string_view name, const int32_t offset) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока
		const locker_t <> lock(this->_mtx.tz);
		// Выполняем добавление временной зоны в список временных зон
		this->_timeZones.emplace(this->_fmk->transform(name, fmk_t::transform_t::LOWER_CASE), offset);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name, offset), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки своего списка временных зон
 *
 * @param zones список временных зон для установки
 *
 */
void awh::Chrono::setTimeZones(const unordered_map <string, int32_t> & zones) noexcept {
	// Выполняем блокировку потока
	const locker_t <> lock(this->_mtx.tz);
	// Название временной зоны
	string name = "";
	/**
	 * Выполняем перебор всего списка временных зон
	 */
	for(auto & zone : zones){
		// Получаем название временной зоны
		name = zone.first;
		// Выполняем добавление временной зоны в список временных зон
		this->_timeZones.emplace(this->_fmk->transform(name, fmk_t::transform_t::LOWER_CASE), zone.second);
	}
}
/**
 * @brief Метод установки штампа времени в указанных единицах измерения
 *
 * @param date дата для установки
 * @param type единицы измерения штампа времени
 *
 */
void awh::Chrono::timestamp(const uint64_t date, const type_t type) noexcept {
	// Если дата передана
	if(date > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Текущий штамп времени
			uint64_t stamp = 0;
			/**
			 * Определяем единицы измерения штампа времени
			 */
			switch(static_cast <uint8_t> (type)){
				// Если единицы измерения штампа времени требуется установить в годах
				case static_cast <uint8_t> (type_t::YEAR):
					// Получаем текущий штамп времени
					stamp = static_cast <uint64_t> (date * 31536000000);
				break;
				// Если единицы измерения штампа времени требуется установить в месяцах
				case static_cast <uint8_t> (type_t::MONTH):
					// Получаем текущий штамп времени
					stamp = static_cast <uint64_t> (date * 2629746000);
				break;
				// Если единицы измерения штампа времени требуется установить в неделях
				case static_cast <uint8_t> (type_t::WEEK):
					// Получаем текущий штамп времени
					stamp = static_cast <uint64_t> (date * 604800000);
				break;
				// Если единицы измерения штампа времени требуется установить в днях
				case static_cast <uint8_t> (type_t::DAY):
					// Получаем текущий штамп времени
					stamp = static_cast <uint64_t> (date * 86400000);
				break;
				// Если единицы измерения штампа времени требуется установить в часах
				case static_cast <uint8_t> (type_t::HOUR):
					// Получаем текущий штамп времени
					stamp = static_cast <uint64_t> (date * 3600000);
				break;
				// Если единицы измерения штампа времени требуется установить в минутах
				case static_cast <uint8_t> (type_t::MINUTES):
					// Получаем текущий штамп времени
					stamp = static_cast <uint64_t> (date * 60000);
				break;
				// Если единицы измерения штампа времени требуется установить в секундах
				case static_cast <uint8_t> (type_t::SECONDS):
					// Получаем текущий штамп времени
					stamp = static_cast <uint64_t> (date * 1000);
				break;
				// Если единицы измерения штампа времени требуется установить в миллисекундах
				case static_cast <uint8_t> (type_t::MILLISECONDS):
					// Получаем текущий штамп времени
					stamp = date;
				break;
				// Если единицы измерения штампа времени требуется установить в микросекундах
				case static_cast <uint8_t> (type_t::MICROSECONDS): {
					// Устанавливаем количество микросекунд
					this->_dt.microseconds = (date % 1000);
					// Получаем текущий штамп времени
					stamp = (date / 1000);
				} break;
				// Если единицы измерения штампа времени требуется установить в наносекундах
				case static_cast <uint8_t> (type_t::NANOSECONDS): {
					// Устанавливаем количество наносекунд
					this->_dt.nanoseconds = (date % 1000000);
					// Получаем текущий штамп времени
					stamp = (date / 1000000);
				} break;
			}
			// Выполняем блокировку потока
			const locker_t <> lock(this->_mtx.date);
			// Заполняем объект даты из штампа времени
			this->makeDate(stamp, this->_dt);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(date, static_cast <uint16_t> (type)), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод получения штампа времени в указанных единицах измерения
 *
 * @param type    единицы измерения штампа времени
 * @param storage хранение значение времени
 * @return        штамп времени в указанных единицах измерения
 *
 */
uint64_t awh::Chrono::timestamp(const type_t type, const storage_t storage) const noexcept {
	// Переменная результата
	uint64_t result = 0;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем единицы измерения штампа времени
		 */
		switch(static_cast <uint8_t> (type)){
			// Если единицы измерения штампа времени требуется получить в годы
			case static_cast <uint8_t> (type_t::YEAR): {
				/**
				 * Определяем хранилище значение времени
				 */
				switch(static_cast <uint8_t> (storage)){
					// Если хранилище локальное
					case static_cast <uint8_t> (storage_t::LOCAL):
						// Получаем результат
						result = static_cast <uint64_t> (this->makeDate(this->_dt) / 31536000000.L);
					break;
					// Если хранилище глобальное
					case static_cast <uint8_t> (storage_t::GLOBAL): {
						// Получаем штамп времени в часы
						chrono::hours hours = chrono::duration_cast <chrono::hours> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (hours.count() / 8760.L);
					} break;
				}
			} break;
			// Если единицы измерения штампа времени требуется получить в месяцах
			case static_cast <uint8_t> (type_t::MONTH): {
				/**
				 * Определяем хранилище значение времени
				 */
				switch(static_cast <uint8_t> (storage)){
					// Если хранилище локальное
					case static_cast <uint8_t> (storage_t::LOCAL):
						// Получаем результат
						result = static_cast <uint64_t> (this->makeDate(this->_dt) / 2629746000.L);
					break;
					// Если хранилище глобальное
					case static_cast <uint8_t> (storage_t::GLOBAL): {
						// Получаем штамп времени в секундах
						chrono::seconds seconds = chrono::duration_cast <chrono::seconds> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (seconds.count() / 2629746.L);
					} break;
				}
			} break;
			// Если единицы измерения штампа времени требуется получить в неделях
			case static_cast <uint8_t> (type_t::WEEK): {
				/**
				 * Определяем хранилище значение времени
				 */
				switch(static_cast <uint8_t> (storage)){
					// Если хранилище локальное
					case static_cast <uint8_t> (storage_t::LOCAL):
						// Получаем результат
						result = static_cast <uint64_t> (this->makeDate(this->_dt) / 604800000.L);
					break;
					// Если хранилище глобальное
					case static_cast <uint8_t> (storage_t::GLOBAL): {
						// Получаем штамп времени в часы
						chrono::hours hours = chrono::duration_cast <chrono::hours> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (hours.count() / 168.L);
					} break;
				}
			} break;
			// Если единицы измерения штампа времени требуется получить в днях
			case static_cast <uint8_t> (type_t::DAY): {
				/**
				 * Определяем хранилище значение времени
				 */
				switch(static_cast <uint8_t> (storage)){
					// Если хранилище локальное
					case static_cast <uint8_t> (storage_t::LOCAL):
						// Получаем результат
						result = static_cast <uint64_t> (this->makeDate(this->_dt) / 86400000.L);
					break;
					// Если хранилище глобальное
					case static_cast <uint8_t> (storage_t::GLOBAL): {
						// Получаем штамп времени в часы
						chrono::hours hours = chrono::duration_cast <chrono::hours> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (hours.count() / 24.L);
					} break;
				}
			} break;
			// Если единицы измерения штампа времени требуется получить в часах
			case static_cast <uint8_t> (type_t::HOUR): {
				/**
				 * Определяем хранилище значение времени
				 */
				switch(static_cast <uint8_t> (storage)){
					// Если хранилище локальное
					case static_cast <uint8_t> (storage_t::LOCAL):
						// Получаем результат
						result = static_cast <uint64_t> (this->makeDate(this->_dt) / 3600000.L);
					break;
					// Если хранилище глобальное
					case static_cast <uint8_t> (storage_t::GLOBAL): {
						// Получаем штамп времени в часы
						chrono::hours hours = chrono::duration_cast <chrono::hours> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (hours.count());
					} break;
				}
			} break;
			// Если единицы измерения штампа времени требуется получить в минутах
			case static_cast <uint8_t> (type_t::MINUTES): {
				/**
				 * Определяем хранилище значение времени
				 */
				switch(static_cast <uint8_t> (storage)){
					// Если хранилище локальное
					case static_cast <uint8_t> (storage_t::LOCAL):
						// Получаем результат
						result = static_cast <uint64_t> (this->makeDate(this->_dt) / 60000.L);
					break;
					// Если хранилище глобальное
					case static_cast <uint8_t> (storage_t::GLOBAL): {
						// Получаем штамп времени в минуты
						chrono::minutes minutes = chrono::duration_cast <chrono::minutes> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (minutes.count());
					} break;
				}
			} break;
			// Если единицы измерения штампа времени требуется получить в секундах
			case static_cast <uint8_t> (type_t::SECONDS): {
				/**
				 * Определяем хранилище значение времени
				 */
				switch(static_cast <uint8_t> (storage)){
					// Если хранилище локальное
					case static_cast <uint8_t> (storage_t::LOCAL):
						// Получаем результат
						result = static_cast <uint64_t> (this->makeDate(this->_dt) / 1000.L);
					break;
					// Если хранилище глобальное
					case static_cast <uint8_t> (storage_t::GLOBAL): {
						// Получаем штамп времени в секундах
						chrono::seconds seconds = chrono::duration_cast <chrono::seconds> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (seconds.count());
					} break;
				}
			} break;
			// Если единицы измерения штампа времени требуется получить в миллисекундах
			case static_cast <uint8_t> (type_t::MILLISECONDS): {
				/**
				 * Определяем хранилище значение времени
				 */
				switch(static_cast <uint8_t> (storage)){
					// Если хранилище локальное
					case static_cast <uint8_t> (storage_t::LOCAL):
						// Получаем результат
						result = this->makeDate(this->_dt);
					break;
					// Если хранилище глобальное
					case static_cast <uint8_t> (storage_t::GLOBAL): {
						// Получаем штамп времени в миллисекундах
						chrono::milliseconds milliseconds = chrono::duration_cast <chrono::milliseconds> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (milliseconds.count());
					} break;
				}
			} break;
			// Если единицы измерения штампа времени требуется получить в микросекундах
			case static_cast <uint8_t> (type_t::MICROSECONDS): {
				/**
				 * Определяем хранилище значение времени
				 */
				switch(static_cast <uint8_t> (storage)){
					// Если хранилище локальное
					case static_cast <uint8_t> (storage_t::LOCAL):
						// Получаем результат
						result = ((this->makeDate(this->_dt) * 1000) + this->_dt.microseconds);
					break;
					// Если хранилище глобальное
					case static_cast <uint8_t> (storage_t::GLOBAL): {
						// Получаем штамп времени в микросекунды
						chrono::microseconds microseconds = chrono::duration_cast <chrono::microseconds> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (microseconds.count());
					} break;
				}
			} break;
			// Если единицы измерения штампа времени требуется получить в наносекундах
			case static_cast <uint8_t> (type_t::NANOSECONDS): {
				/**
				 * Определяем хранилище значение времени
				 */
				switch(static_cast <uint8_t> (storage)){
					// Если хранилище локальное
					case static_cast <uint8_t> (storage_t::LOCAL):
						// Получаем результат
						result = ((this->makeDate(this->_dt) * 1000000) + this->_dt.nanoseconds);
					break;
					// Если хранилище глобальное
					case static_cast <uint8_t> (storage_t::GLOBAL): {
						// Получаем штамп времени в наносекундах
						chrono::nanoseconds nanoseconds = chrono::duration_cast <chrono::nanoseconds> (chrono::system_clock::now().time_since_epoch());
						// Получаем результат
						result = static_cast <uint64_t> (nanoseconds.count());
					} break;
				}
			} break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (type), static_cast <uint16_t> (storage)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод парсинга строки даты и времени в UnixTimestamp
 *
 * @param date    строка даты
 * @param format  формат даты
 * @param storage хранение значение времени
 * @return        дата в UnixTimestamp
 *
 */
uint64_t awh::Chrono::parse(string_view date, string_view format, const storage_t storage) noexcept {
	// Переменная результата
	uint64_t result = 0;
	// Если дата для парсинга и формат переданы
	if(!date.empty() && !format.empty()){
		// Создаем структуру времени
		dt_t dt;
		// Начальная позиция данных в тексте
		ssize_t pos = 0;
		// Символ для обработки
		char letter = 0;
		// Режим детекции переменной формата
		bool mode = false;
		// Текущее количество минут прошедших с 1970-го года
		uint64_t lastMinutes = 0;
		// Флаги установки параметров
		bool flags[6] = {
			false, // Флаг установки временной зоны
			false, // Флаг установки года
			false, // Флаг установки часа
			false, // Флаг установки минут
			false, // Флаг установки секунд
			false  // Флаг установки миллисекунд
		};
		/**
		 * Определяем хранилище значение времени
		 */
		switch(static_cast <uint8_t> (storage)){
			// Если хранилище локальное
			case static_cast <uint8_t> (storage_t::LOCAL): {
				// Выполняем блокировку потока
				const locker_t <> lock(this->_mtx.date);
				// Выполняем сброс временной зоны
				this->_dt.offset = 0;
				// Выполняем сброс количества наносекунд
				this->_dt.nanoseconds = 0;
				// Выполняем сброс количества микросекунд
				this->_dt.microseconds = 0;
				// Получаем текущее значение штампа времени
				result = this->makeDate(this->_dt);
				// Получаем количество минут прошедших с 1970-го года
				lastMinutes = (result / 60000);
			} break;
			// Если хранилище глобальное
			case static_cast <uint8_t> (storage_t::GLOBAL): {
				// Получаем текущее значение штампа времени
				result = this->timestamp(type_t::MILLISECONDS);
				// Заполняем объект даты из штампа времени
				this->makeDate(result, dt);
				// Получаем количество минут прошедших с 1970-го года
				lastMinutes = (result / 60000);
			} break;
		}
		/**
		 * Выполняем перебор формата
		 */
		for(size_t i = 0; i < format.length(); i++){
			// Получаем символ для обработки
			letter = format[i];
			/**
			 * Определяем символ парсинга
			 */
			switch(letter){
				// Если мы нашли идентификатор переменной
				case '%': mode = true; break;
				// Если мы нашли переменную (y)
				case 'y':
				// Если мы нашли переменную (g)
				case 'g':
				// Если мы нашли переменную (Y)
				case 'Y':
				// Если мы нашли переменную (G)
				case 'G':
				// Если мы нашли переменную (b)
				case 'b':
				// Если мы нашли переменную (h)
				case 'h':
				// Если мы нашли переменную (B)
				case 'B':
				// Если мы нашли переменную (m)
				case 'm':
				// Если мы нашли переменную (d)
				case 'd':
				// Если мы нашли переменную (e)
				case 'e':
				// Если мы нашли переменную (a)
				case 'a':
				// Если мы нашли переменную (A)
				case 'A':
				// Если мы нашли переменную (j)
				case 'j':
				// Если мы нашли переменную (u)
				case 'u':
				// Если мы нашли переменную (U)
				case 'U':
				// Если мы нашли переменную (w)
				case 'w':
				// Если мы нашли переменную (W)
				case 'W':
				// Если мы нашли переменную (D)
				case 'D':
				// Если мы нашли переменную (x)
				case 'x':
				// Если мы нашли переменную (F)
				case 'F':
				// Если мы нашли переменную (H)
				case 'H':
				// Если мы нашли переменную (I)
				case 'I':
				// Если мы нашли переменную (M)
				case 'M':
				// Если мы нашли переменную (s)
				case 's':
				// Если мы нашли переменную (S)
				case 'S':
				// Если мы нашли переменную (p)
				case 'p':
				// Если мы нашли переменную (R)
				case 'R':
				// Если мы нашли переменную (T)
				case 'T':
				// Если мы нашли переменную (X)
				case 'X':
				// Если мы нашли переменную (r)
				case 'r':
				// Если мы нашли переменную (c)
				case 'c':
				// Если мы нашли переменную (o)
				case 'o':
				// Если мы нашли переменную (z)
				case 'z':
				// Если мы нашли переменную (Z)
				case 'Z': {
					// Если мы ищем переменную
					if(mode){
						/**
						 * Определяем хранилище значение времени
						 */
						switch(static_cast <uint8_t> (storage)){
							// Если хранилище локальное
							case static_cast <uint8_t> (storage_t::LOCAL): {
								// Выполняем блокировку потока
								const locker_t <> lock(this->_mtx.date);
								/**
								 * Определяем символ парсинга
								 */
								switch(letter){
									// Если мы нашли переменную (y)
									case 'y':
									// Если мы нашли переменную (g)
									case 'g': {
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::y, pos);
										// Устанавливаем флаг установки года
										flags[1] = (pos > -1);
									} break;
									// Если мы нашли переменную (Y)
									case 'Y':
									// Если мы нашли переменную (G)
									case 'G': {
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::Y, pos);
										// Устанавливаем флаг установки года
										flags[1] = (pos > -1);
									} break;
									// Если мы нашли переменную (b)
									case 'b':
									// Если мы нашли переменную (h)
									case 'h':
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::b, pos);
									break;
									// Если мы нашли переменную (B)
									case 'B':
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::B, pos);
									break;
									// Если мы нашли переменную (m)
									case 'm':
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::m, pos);
									break;
									// Если мы нашли переменную (d)
									case 'd':
									// Если мы нашли переменную (e)
									case 'e':
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::d, pos);
									break;
									// Если мы нашли переменную (a)
									case 'a':
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::a, pos);
									break;
									// Если мы нашли переменную (A)
									case 'A':
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::A, pos);
									break;
									// Если мы нашли переменную (j)
									case 'j':
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::j, pos);
									break;
									// Если мы нашли переменную (u)
									case 'u':
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::u, pos);
									break;
									// Если мы нашли переменную (U)
									case 'U':
									// Если мы нашли переменную (W)
									case 'W':
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::W, pos);
									break;
									// Если мы нашли переменную (w)
									case 'w':
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::w, pos);
									break;
									// Если мы нашли переменную (D)
									case 'D':
									// Если мы нашли переменную (x)
									case 'x': {
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::D, pos);
										// Устанавливаем флаг установки года
										flags[1] = (pos > -1);
									} break;
									// Если мы нашли переменную (F)
									case 'F': {
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::F, pos);
										// Устанавливаем флаг установки года
										flags[1] = (pos > -1);
									} break;
									// Если мы нашли переменную (H)
									case 'H': {
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::H, pos);
										// Устанавливаем флаг установки часа
										flags[2] = (pos > -1);
									} break;
									// Если мы нашли переменную (I)
									case 'I': {
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::I, pos);
										// Устанавливаем флаг установки часа
										flags[2] = (pos > -1);
									} break;
									// Если мы нашли переменную (M)
									case 'M': {
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::M, pos);
										// Устанавливаем флаг установки минут
										flags[3] = (pos > -1);
									} break;
									// Если мы нашли переменную (s)
									case 's': {
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::s, pos);
										// Устанавливаем флаг установки миллисекунд
										flags[5] = (pos > -1);
									} break;
									// Если мы нашли переменную (S)
									case 'S': {
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::S, pos);
										// Устанавливаем флаг установки секунд
										flags[4] = (pos > -1);
									} break;
									// Если мы нашли переменную (p)
									case 'p':
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::p, pos);
									break;
									// Если мы нашли переменную (R)
									case 'R': {
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::R, pos);
										// Устанавливаем флаг установки часа
										flags[2] = (pos > -1);
										// Устанавливаем флаг установки минут
										flags[3] = (pos > -1);
									} break;
									// Если мы нашли переменную (T)
									case 'T':
									// Если мы нашли переменную (X)
									case 'X': {
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::T, pos);
										// Устанавливаем флаг установки часа
										flags[2] = (pos > -1);
										// Устанавливаем флаг установки минут
										flags[3] = (pos > -1);
										// Устанавливаем флаг установки секунд
										flags[4] = (pos > -1);
									} break;
									// Если мы нашли переменную (r)
									case 'r': {
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::r, pos);
										// Устанавливаем флаг установки часа
										flags[2] = (pos > -1);
										// Устанавливаем флаг установки минут
										flags[3] = (pos > -1);
										// Устанавливаем флаг установки секунд
										flags[4] = (pos > -1);
									} break;
									// Если мы нашли переменную (c)
									case 'c': {
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::c, pos);
										// Устанавливаем флаг установки года
										flags[1] = (pos > -1);
										// Устанавливаем флаг установки часа
										flags[2] = (pos > -1);
										// Устанавливаем флаг установки минут
										flags[3] = (pos > -1);
										// Устанавливаем флаг установки секунд
										flags[4] = (pos > -1);
									} break;
									// Если мы нашли переменную (z)
									case 'z': {
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::z, pos);
										// Если флаг ещё не установлен
										if(!flags[0])
											// Устанавливаем флаг установки смещения временной зоны
											flags[0] = (pos > -1);
									} break;
									// Если мы нашли переменную (Z)
									case 'Z': {
										// Выполняем обработку полученных данных
										pos = this->prepare(this->_dt, date, format_t::Z, pos);
										// Устанавливаем флаг установки смещения временной зоны
										flags[0] = (pos > -1);
									} break;
									// Если пришёл любой другой формат, завершаем работу
									default: pos = -1;
								}
							} break;
							// Если хранилище глобальное
							case static_cast <uint8_t> (storage_t::GLOBAL): {
								/**
								 * Определяем символ парсинга
								 */
								switch(letter){
									// Если мы нашли переменную (y)
									case 'y':
									// Если мы нашли переменную (g)
									case 'g': {
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::y, pos);
										// Устанавливаем флаг установки года
										flags[1] = (pos > -1);
									} break;
									// Если мы нашли переменную (Y)
									case 'Y':
									// Если мы нашли переменную (G)
									case 'G': {
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::Y, pos);
										// Устанавливаем флаг установки года
										flags[1] = (pos > -1);
									} break;
									// Если мы нашли переменную (b)
									case 'b':
									// Если мы нашли переменную (h)
									case 'h':
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::b, pos);
									break;
									// Если мы нашли переменную (B)
									case 'B':
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::B, pos);
									break;
									// Если мы нашли переменную (m)
									case 'm':
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::m, pos);
									break;
									// Если мы нашли переменную (d)
									case 'd':
									// Если мы нашли переменную (e)
									case 'e':
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::d, pos);
									break;
									// Если мы нашли переменную (a)
									case 'a':
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::a, pos);
									break;
									// Если мы нашли переменную (A)
									case 'A':
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::A, pos);
									break;
									// Если мы нашли переменную (j)
									case 'j':
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::j, pos);
									break;
									// Если мы нашли переменную (u)
									case 'u':
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::u, pos);
									break;
									// Если мы нашли переменную (U)
									case 'U':
									// Если мы нашли переменную (W)
									case 'W':
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::W, pos);
									break;
									// Если мы нашли переменную (w)
									case 'w':
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::w, pos);
									break;
									// Если мы нашли переменную (D)
									case 'D':
									// Если мы нашли переменную (x)
									case 'x': {
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::D, pos);
										// Устанавливаем флаг установки года
										flags[1] = (pos > -1);
									} break;
									// Если мы нашли переменную (F)
									case 'F': {
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::F, pos);
										// Устанавливаем флаг установки года
										flags[1] = (pos > -1);
									} break;
									// Если мы нашли переменную (H)
									case 'H': {
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::H, pos);
										// Устанавливаем флаг установки часа
										flags[2] = (pos > -1);
									} break;
									// Если мы нашли переменную (I)
									case 'I': {
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::I, pos);
										// Устанавливаем флаг установки часа
										flags[2] = (pos > -1);
									} break;
									// Если мы нашли переменную (M)
									case 'M': {
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::M, pos);
										// Устанавливаем флаг установки минут
										flags[3] = (pos > -1);
									} break;
									// Если мы нашли переменную (s)
									case 's': {
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::s, pos);
										// Устанавливаем флаг установки миллисекунд
										flags[5] = (pos > -1);
									} break;
									// Если мы нашли переменную (S)
									case 'S': {
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::S, pos);
										// Устанавливаем флаг установки секунд
										flags[4] = (pos > -1);
									} break;
									// Если мы нашли переменную (p)
									case 'p':
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::p, pos);
									break;
									// Если мы нашли переменную (R)
									case 'R': {
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::R, pos);
										// Устанавливаем флаг установки часа
										flags[2] = (pos > -1);
										// Устанавливаем флаг установки минут
										flags[3] = (pos > -1);
									} break;
									// Если мы нашли переменную (T)
									case 'T':
									// Если мы нашли переменную (X)
									case 'X': {
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::T, pos);
										// Устанавливаем флаг установки часа
										flags[2] = (pos > -1);
										// Устанавливаем флаг установки минут
										flags[3] = (pos > -1);
										// Устанавливаем флаг установки секунд
										flags[4] = (pos > -1);
									} break;
									// Если мы нашли переменную (r)
									case 'r': {
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::r, pos);
										// Устанавливаем флаг установки часа
										flags[2] = (pos > -1);
										// Устанавливаем флаг установки минут
										flags[3] = (pos > -1);
										// Устанавливаем флаг установки секунд
										flags[4] = (pos > -1);
									} break;
									// Если мы нашли переменную (c)
									case 'c': {
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::c, pos);
										// Устанавливаем флаг установки года
										flags[1] = (pos > -1);
										// Устанавливаем флаг установки часа
										flags[2] = (pos > -1);
										// Устанавливаем флаг установки минут
										flags[3] = (pos > -1);
										// Устанавливаем флаг установки секунд
										flags[4] = (pos > -1);
									} break;
									// Если мы нашли переменную (o)
									case 'o':
									// Если мы нашли переменную (z)
									case 'z': {
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::z, pos);
										// Если флаг ещё не установлен
										if(!flags[0])
											// Устанавливаем флаг установки смещения временной зоны
											flags[0] = (pos > -1);
									} break;
									// Если мы нашли переменную (Z)
									case 'Z': {
										// Выполняем обработку полученных данных
										pos = this->prepare(dt, date, format_t::Z, pos);
										// Устанавливаем флаг установки смещения временной зоны
										flags[0] = (pos > -1);
									} break;
									// Если пришёл любой другой формат, завершаем работу
									default: pos = -1;
								}
							} break;
						}
						// Если позиция не определена
						if(pos < 0)
							// Завершаем перебор
							i = format.length();
					}
					// Сбрасываем режим детекции переменной формата
					mode = false;
				} break;
				// Если получен любой другой символ
				default: mode = false;
			}
		}
		/**
		 * Определяем хранилище значение времени
		 */
		switch(static_cast <uint8_t> (storage)){
			// Если хранилище локальное
			case static_cast <uint8_t> (storage_t::LOCAL): {
				// Выполняем блокировку потока
				const locker_t <> lock(this->_mtx.date);
				// Если флаг смещения временной зоны не передан
				if(!flags[0]){
					// Устанавливаем идентификатор временной зоны
					this->_dt.zone = zone_t::UTC;
					// Устанавливаем смещение временной зоны по умолчанию
					this->_dt.offset = this->getTimeZone();
				}
				// Получаем смещение временной зоны
				const int32_t offset = this->_dt.offset;
				// Если смещение временной зоны установлено
				if(offset != 0)
					// Выполняем инверсию
					this->_dt.offset *= -1;
				// Если час или минуты установлены а секунды нет
				if((flags[2] || flags[3]) && !flags[4])
					// Выполняем сброс секунд
					this->_dt.seconds = 0;
				// Если часы, минуты или секунды установлены а миллисекунды нет
				if((flags[2] || flags[3] || flags[4]) && !flags[5])
					// Выполняем сброс миллисекунд
					this->_dt.milliseconds = 0;
				// Выполняем формирование UnixTimestamp
				result = this->makeDate(this->_dt);
				// Если смещение временной зоны установлено
				if(offset != 0){
					// Выполняем установку пересчитанной временной зоны обратно
					this->makeDate(result, this->_dt);
					// Возвращаем значение временной зоны обратно
					this->_dt.offset = offset;
				}
				// Если количество минут переданной даты с начала 1970-го года выше чем текущее количество минут
				if(!flags[1] && ((result / 60000) > lastMinutes)){
					// Уменьшаем значение текущего года
					this->_dt.year--;
					// Устанавливаем флаг високосного года
					this->_dt.leap = this->leap(this->_dt.year);
					// Выполняем формирование UnixTimestamp
					result = this->makeDate(this->_dt);
					// Выполняем установку пересчитанной временной зоны обратно
					this->makeDate(result, this->_dt);
				}
			} break;
			// Если хранилище глобальное
			case static_cast <uint8_t> (storage_t::GLOBAL): {
				// Если флаг смещения временной зоны не передан
				if(!flags[0]){
					// Устанавливаем идентификатор временной зоны
					dt.zone = zone_t::UTC;
					// Устанавливаем смещение временной зоны по умолчанию
					dt.offset = this->getTimeZone();
				}
				// Если смещение временной зоны установлено
				if(dt.offset != 0)
					// Выполняем инверсию
					dt.offset *= -1;
				// Если час или минуты установлены а секунды нет
				if((flags[2] || flags[3]) && !flags[4])
					// Выполняем сброс секунд
					dt.seconds = 0;
				// Если часы, минуты или секунды установлены а миллисекунды нет
				if((flags[2] || flags[3] || flags[4]) && !flags[5])
					// Выполняем сброс миллисекунд
					dt.milliseconds = 0;
				// Выполняем формирование UnixTimestamp
				result = this->makeDate(dt);
				// Если количество минут переданной даты с начала 1970-го года выше чем текущее количество минут
				if(!flags[1] && ((result / 60000) > lastMinutes)){
					// Уменьшаем значение текущего года
					dt.year--;
					// Устанавливаем флаг високосного года
					dt.leap = this->leap(dt.year);
					// Выполняем формирование UnixTimestamp
					result = this->makeDate(dt);
				}
			} break;
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод форматирования временной зоны
 *
 * @param zone временная зона (в секундах) в которой нужно получить результат
 * @return     строковое обозначение временной зоны
 *
 */
string awh::Chrono::format(const int32_t zone) const noexcept {
	// Переменная результата
	string result = "UTC";
	// Если временная зона передана
	if(zone != 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если переданная зона больше нуля
			if(zone >= 0)
				// Добавляем плюс
				result.append(1, '+');
			// Добавляем минус
			else result.append(1, '-');
			// Временное значение переменной
			double intpart = 0;
			// Выполняем конвертацию секунд в часы
			const double seconds = (::abs(zone) / 3600.L);
			// Выполняем проверку есть ли дробная часть у числа
			if(::modf(seconds, &intpart) == 0)
				// Добавляем переданную зону
				result.append(std::to_string(static_cast <uint32_t> (seconds)));
			// Если мы нашли дробную часть числа
			else {
				// Добавляем первую часть часа
				result.append(std::to_string(static_cast <uint32_t> (intpart)));
				// Добавляем разделитель времени
				result.append(1, ':');
				// Добавляем дробную часть часа
				result.append(std::to_string(static_cast <uint32_t> ((seconds - intpart) * 60)));
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Формируем результат по умолчанию
			result = "UTC";
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(zone), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод форматирования временной зоны
 *
 * @param zone временная зона в которой нужно получить результат
 * @return     строковое обозначение временной зоны
 *
 */
string awh::Chrono::format(const zone_t zone) const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем временную зону
		 */
		switch(static_cast <uint8_t> (zone)){
			// Если временная зона не установлена
			case static_cast <uint8_t> (zone_t::NONE):
				// Возвращаем временную зону по умолчанию
				return this->format(this->_dt.offset);
			// Если временная зона установлена как (Атлантическое Время)
			case static_cast <uint8_t> (zone_t::AT): {
				// Создаем структуру времени
				dt_t dt;
				// Заполняем объект даты из штампа времени
				this->makeDate(this->timestamp(type_t::MILLISECONDS), dt);
				// Если время летнее
				if(dt.dst)
					// Формируем летнее время
					return "ADT";
				// Если инверсия не включена
				return "UTC-4";
			}
			// Если временная зона установлена как (Северноамериканское Центральное Время)
			case static_cast <uint8_t> (zone_t::CT): {
				// Создаем структуру времени
				dt_t dt;
				// Заполняем объект даты из штампа времени
				this->makeDate(this->timestamp(type_t::MILLISECONDS), dt);
				// Если время летнее
				if(dt.dst)
					// Формируем летнее время
					return "CDT";
				// Если инверсия не включена
				return "UTC-6";
			}
			// Если временная зона установлена как (Северноамериканское Восточное Время)
			case static_cast <uint8_t> (zone_t::ET): {
				// Создаем структуру времени
				dt_t dt;
				// Заполняем объект даты из штампа времени
				this->makeDate(this->timestamp(type_t::MILLISECONDS), dt);
				// Если время летнее
				if(dt.dst)
					// Формируем летнее время
					return "EDT";
				// Возвращаем результат
				return "EST";
			}
			// Если временная зона установлена как (Северноамериканское Горное Время)
			case static_cast <uint8_t> (zone_t::MT): {
				// Создаем структуру времени
				dt_t dt;
				// Заполняем объект даты из штампа времени
				this->makeDate(this->timestamp(type_t::MILLISECONDS), dt);
				// Если время летнее
				if(dt.dst)
					// Формируем летнее время
					return "MDT";
				// Возвращаем результат
				return "UTC-7";
			}
			// Если временная зона установлена как (Северноамериканское Тихоокеанское Время)
			case static_cast <uint8_t> (zone_t::PT): {
				// Создаем структуру времени
				dt_t dt;
				// Заполняем объект даты из штампа времени
				this->makeDate(this->timestamp(type_t::MILLISECONDS), dt);
				// Если время летнее
				if(dt.dst)
					// Формируем летнее время
					return "PDT";
				// Возвращаем результат
				return "PST";
			}
			// Если временная зона установлена как (Время В Ньюфаундленде)
			case static_cast <uint8_t> (zone_t::NT):
				// Формируем временную зону
				return "NT";
			// Если временная зона установлена как (Стандартное Время На Острове Маврикий)
			case static_cast <uint8_t> (zone_t::MUT):
				// Формируем временную зону
				return "MUT";
			// Если временная зона установлена как (Время На Мальдивах)
			case static_cast <uint8_t> (zone_t::MVT):
				// Формируем временную зону
				return "MVT";
			// Если временная зона установлена как (Малайское Время)
			case static_cast <uint8_t> (zone_t::MYT):
				// Формируем временную зону
				return "MYT";
			// Если временная зона установлена как (Стандартное Время В Новой Каледонии)
			case static_cast <uint8_t> (zone_t::NCT):
				// Формируем временную зону
				return "NCT";
			// Если временная зона установлена как (Летнее Время В Ньюфаундленде)
			case static_cast <uint8_t> (zone_t::NDT):
				// Формируем временную зону
				return "NDT";
			// Если временная зона установлена как (Время На Острове Норфолк)
			case static_cast <uint8_t> (zone_t::NFT):
				// Формируем временную зону
				return "NFT";
			// Если временная зона установлена как (Непальское Время)
			case static_cast <uint8_t> (zone_t::NPT):
				// Формируем временную зону
				return "NPT";
			// Если временная зона установлена как (Время На Острове Науру)
			case static_cast <uint8_t> (zone_t::NRT):
				// Формируем временную зону
				return "NRT";
			// Если временная зона установлена как (Стандартное Время В Ньюфаундленде)
			case static_cast <uint8_t> (zone_t::NST):
				// Формируем временную зону
				return "NST";
			// Если временная зона установлена как (Время На Острове Палау)
			case static_cast <uint8_t> (zone_t::PWT):
				// Формируем временную зону
				return "PWT";
			// Если временная зона установлена как (Время На Острове Ниуэ)
			case static_cast <uint8_t> (zone_t::NUT):
				// Формируем временную зону
				return "NUT";
			// Если временная зона установлена как (Минское Время)
			case static_cast <uint8_t> (zone_t::FET):
				// Формируем временную зону
				return "FET";
			// Если временная зона установлена как (Летнее Время На О. Фиджи)
			case static_cast <uint8_t> (zone_t::FJT):
				// Формируем временную зону
				return "FJT";
			// Если временная зона установлена как (Парагвайское Стандартное Время)
			case static_cast <uint8_t> (zone_t::PYT):
				// Формируем временную зону
				return "PYT";
			// Если временная зона установлена как (Время На Острове Реюньон)
			case static_cast <uint8_t> (zone_t::RET):
				// Формируем временную зону
				return "RET";
			// Если временная зона установлена как (Время На Соломоновых Островах)
			case static_cast <uint8_t> (zone_t::SBT):
				// Формируем временную зону
				return "SBT";
			// Если временная зона установлена как (Время На Сейшелах)
			case static_cast <uint8_t> (zone_t::SCT):
				// Формируем временную зону
				return "SCT";
			// Если временная зона установлена как (Сингапурское Время)
			case static_cast <uint8_t> (zone_t::SGT):
				// Формируем временную зону
				return "SGT";
			// Если временная зона установлена как (Время В Суринаме)
			case static_cast <uint8_t> (zone_t::SRT):
				// Формируем временную зону
				return "SRT";
			// Если временная зона установлена как (Стандартное Время На Острове Самоа)
			case static_cast <uint8_t> (zone_t::SST):
				// Формируем временную зону
				return "SST";
			// Если временная зона установлена как (Французское Южное И Антарктическое Время)
			case static_cast <uint8_t> (zone_t::TFT):
				// Формируем временную зону
				return "TFT";
			// Если временная зона установлена как (Тайландское Время)
			case static_cast <uint8_t> (zone_t::THA):
				// Формируем временную зону
				return "THA";
			// Если временная зона установлена как (Время В Таджикистане)
			case static_cast <uint8_t> (zone_t::TJT):
				// Формируем временную зону
				return "TJT";
			// Если временная зона установлена как (Время На Островах Токелау)
			case static_cast <uint8_t> (zone_t::TKT):
				// Формируем временную зону
				return "TKT";
			// Если временная зона установлена как (Время В Восточном Тиморе)
			case static_cast <uint8_t> (zone_t::TLT):
				// Формируем временную зону
				return "TLT";
			// Если временная зона установлена как (Стандартное Время В Туркмении)
			case static_cast <uint8_t> (zone_t::TMT):
				// Формируем временную зону
				return "TMT";
			// Если временная зона установлена как (Время На Островах Тонга)
			case static_cast <uint8_t> (zone_t::TOT):
				// Формируем временную зону
				return "TOT";
			// Если временная зона установлена как (Турецкое Время)
			case static_cast <uint8_t> (zone_t::TRT):
				// Формируем временную зону
				return "TRT";
			// Если временная зона установлена как (Время На Островах Тувалу)
			case static_cast <uint8_t> (zone_t::TVT):
				// Формируем временную зону
				return "TVT";
			// Если временная зона установлена как (Стандартное Время На Фолклендах)
			case static_cast <uint8_t> (zone_t::FKT):
				// Формируем временную зону
				return "FKT";
			// Если временная зона установлена как (Стандартное Время На Фернанду-Ди-Норонья)
			case static_cast <uint8_t> (zone_t::FNT):
				// Формируем временную зону
				return "FNT";
			// Если временная зона установлена как (Время В Афганистане)
			case static_cast <uint8_t> (zone_t::AFT):
				// Формируем временную зону
				return "AFT";
			// Если временная зона установлена как (Амазонское Стандартное Время)
			case static_cast <uint8_t> (zone_t::ACT):
				// Формируем временную зону
				return "ACT";
			// Если временная зона установлена как (Атлантическое Летнее Время)
			case static_cast <uint8_t> (zone_t::ADT):
				// Формируем временную зону
				return "ADT";
			// Если временная зона установлена как (Азербайджанское Стандартное Время)
			case static_cast <uint8_t> (zone_t::AZT):
				// Формируем временную зону
				return "AZT";
			// Если временная зона установлена как (Аргентинское Стандартное Время)
			case static_cast <uint8_t> (zone_t::ART):
				// Формируем временную зону
				return "ART";
			// Если временная зона установлена как (Время В Бруней-Даруссаламе)
			case static_cast <uint8_t> (zone_t::BDT):
				// Формируем временную зону
				return "BDT";
			// Если временная зона установлена как (Время В Бруней-Даруссаламе)
			case static_cast <uint8_t> (zone_t::BNT):
				// Формируем временную зону
				return "BNT";
			// Если временная зона установлена как (Боливийское Время)
			case static_cast <uint8_t> (zone_t::BOT):
				// Формируем временную зону
				return "BOT";
			// Если временная зона установлена как (Бразильское Стандартное Время)
			case static_cast <uint8_t> (zone_t::BRT):
				// Формируем временную зону
				return "BRT";
			// Если временная зона установлена как (Бутанское Время)
			case static_cast <uint8_t> (zone_t::BTT):
				// Формируем временную зону
				return "BTT";
			// Если временная зона установлена как (Восточноафриканское Время)
			case static_cast <uint8_t> (zone_t::CAT):
				// Формируем временную зону
				return "CAT";
			// Если временная зона установлена как (Стандартное Время На Островах Кабо-Верде)
			case static_cast <uint8_t> (zone_t::CVT):
				// Формируем временную зону
				return "CVT";
			// Если временная зона установлена как (Время На Острове Рождества)
			case static_cast <uint8_t> (zone_t::CXT):
				// Формируем временную зону
				return "CXT";
			// Если временная зона установлена как (Время На Кокосовые Островах)
			case static_cast <uint8_t> (zone_t::CCT):
				// Формируем временную зону
				return "CCT";
			// Если временная зона установлена как (Центральноевропейское Стандартное Время)
			case static_cast <uint8_t> (zone_t::CET):
				// Формируем временную зону
				return "CET";
			// Если временная зона установлена как (Время В Центральной Индонезии)
			case static_cast <uint8_t> (zone_t::CIT):
				// Формируем временную зону
				return "CIT";
			// Если временная зона установлена как (Стандартное Время На Островах Кука)
			case static_cast <uint8_t> (zone_t::CKT):
				// Формируем временную зону
				return "CKT";
			// Если временная зона установлена как (Чилийское Стандартное Время)
			case static_cast <uint8_t> (zone_t::CLT):
				// Формируем временную зону
				return "CLT";
			// Если временная зона установлена как (Колумбийское Стандартное Время)
			case static_cast <uint8_t> (zone_t::COT):
				// Формируем временную зону
				return "COT";
			// Если временная зона установлена как (Восточноафриканский Час)
			case static_cast <uint8_t> (zone_t::EAT):
				// Формируем временную зону
				return "EAT";
			// Если временная зона установлена как (Эквадорское Время)
			case static_cast <uint8_t> (zone_t::ECT):
				// Формируем временную зону
				return "ECT";
			// Если временная зона установлена как (Северноамериканское Восточное Летнее Время)
			case static_cast <uint8_t> (zone_t::EDT):
				// Формируем временную зону
				return "EDT";
			// Если временная зона установлена как (Восточноевропейское Стандартное Время)
			case static_cast <uint8_t> (zone_t::EET):
				// Формируем временную зону
				return "EET";
			// Если временная зона установлена как (Стандартное Время В Восточной Гренландии)
			case static_cast <uint8_t> (zone_t::EGT):
				// Формируем временную зону
				return "EGT";
			// Если временная зона установлена как (Время В Восточной Индонезии)
			case static_cast <uint8_t> (zone_t::EIT):
				// Формируем временную зону
				return "EIT";
			// Если временная зона установлена как (Северноамериканское Восточное Стандартное Время)
			case static_cast <uint8_t> (zone_t::EST):
				// Формируем временную зону
				return "EST";
			// Если временная зона установлена как (Грузинское Стандартное Время)
			case static_cast <uint8_t> (zone_t::GET):
				// Формируем временную зону
				return "GET";
			// Если временная зона установлена как (Время В Индокитае)
			case static_cast <uint8_t> (zone_t::ICT):
				// Формируем временную зону
				return "ICT";
			// Если временная зона установлена как (Израильское Летнее Время)
			case static_cast <uint8_t> (zone_t::IDT):
				// Формируем временную зону
				return "IDT";
			// Если временная зона установлена как (Время В Французской Гвиане)
			case static_cast <uint8_t> (zone_t::GFT):
				// Формируем временную зону
				return "GFT";
			// Если временная зона установлена как (Время На О. Гамбье)
			case static_cast <uint8_t> (zone_t::GIT):
				// Формируем временную зону
				return "GIT";
			// Если временная зона установлена как (Среднее Время По Гринвичу)
			case static_cast <uint8_t> (zone_t::GMT):
				// Формируем временную зону
				return "GMT";
			// Если временная зона установлена как (Время В Гайане)
			case static_cast <uint8_t> (zone_t::GYT):
				// Формируем временную зону
				return "GYT";
			// Если временная зона установлена как (Гонконгское Стандартное Время)
			case static_cast <uint8_t> (zone_t::HKT):
				// Формируем временную зону
				return "HKT";
			// Если временная зона установлена как (Японское Стандартное Время)
			case static_cast <uint8_t> (zone_t::JST):
				// Формируем временную зону
				return "JST";
			// Если временная зона установлена как (Время В Киргизии)
			case static_cast <uint8_t> (zone_t::KGT):
				// Формируем временную зону
				return "KGT";
			// Если временная зона установлена как (Корейское Стандартное Время)
			case static_cast <uint8_t> (zone_t::KST):
				// Формируем временную зону
				return "KST";
			// Если временная зона установлена как (Северноамериканское Горное Летнее Время)
			case static_cast <uint8_t> (zone_t::MDT):
				// Формируем временную зону
				return "MDT";
			// Если временная зона установлена как (Время На Маршалловых Островах)
			case static_cast <uint8_t> (zone_t::MHT):
				// Формируем временную зону
				return "MHT";
			// Если временная зона установлена как (Время На Маркизских Островах)
			case static_cast <uint8_t> (zone_t::MIT):
				// Формируем временную зону
				return "MIT";
			// Если временная зона установлена как (Время В Мьянме)
			case static_cast <uint8_t> (zone_t::MMT):
				// Формируем временную зону
				return "MMT";
			// Если временная зона установлена как (Московское Время)
			case static_cast <uint8_t> (zone_t::MSK):
				// Формируем временную зону
				return "MSK";
			// Если временная зона установлена как (Московское Летнее Время)
			case static_cast <uint8_t> (zone_t::MSD):
				// Формируем временную зону
				return "MSD";
			// Если временная зона установлена как (Северноамериканское Тихоокеанское Стандартное Время)
			case static_cast <uint8_t> (zone_t::PST):
				// Формируем временную зону
				return "PST";
			// Если временная зона установлена как (Северноамериканское Тихоокеанское Летнее Время)
			case static_cast <uint8_t> (zone_t::PDT):
				// Формируем временную зону
				return "PDT";
			// Если временная зона установлена как (Стандартное Время В Перу)
			case static_cast <uint8_t> (zone_t::PET):
				// Формируем временную зону
				return "PET";
			// Если временная зона установлена как (Время В Папуа-Новой Гвинее)
			case static_cast <uint8_t> (zone_t::PGT):
				// Формируем временную зону
				return "PGT";
			// Если временная зона установлена как (Всемирное Координированное Время)
			case static_cast <uint8_t> (zone_t::UTC):
				// Формируем временную зону
				return "UTC";
			// Если временная зона установлена как (Стандартное Время На Филлипинах)
			case static_cast <uint8_t> (zone_t::PHT):
				// Формируем временную зону
				return "PHT";
			// Если временная зона установлена как (Пакистанское Стандартное Время)
			case static_cast <uint8_t> (zone_t::PKT):
				// Формируем временную зону
				return "PKT";
			// Если временная зона установлена как (Стандартное Время В Уругвае)
			case static_cast <uint8_t> (zone_t::UYT):
				// Формируем временную зону
				return "UYT";
			// Если временная зона установлена как (Время В Узбекистане)
			case static_cast <uint8_t> (zone_t::UZT):
				// Формируем временную зону
				return "UZT";
			// Если временная зона установлена как (Время В Венесуеле)
			case static_cast <uint8_t> (zone_t::VET):
				// Формируем временную зону
				return "VET";
			// Если временная зона установлена как (Стандартное Время На Островах Вануату)
			case static_cast <uint8_t> (zone_t::VUT):
				// Формируем временную зону
				return "VUT";
			// Если временная зона установлена как (Западноафриканское Стандартное Время)
			case static_cast <uint8_t> (zone_t::WAT):
				// Формируем временную зону
				return "WAT";
			// Если временная зона установлена как (Западноевропейское Стандартное Время)
			case static_cast <uint8_t> (zone_t::WET):
				// Формируем временную зону
				return "WET";
			// Если временная зона установлена как (Время На Островах Уоллис И Футуна)
			case static_cast <uint8_t> (zone_t::WFT):
				// Формируем временную зону
				return "WFT";
			// Если временная зона установлена как (Время В Западной Индонезии)
			case static_cast <uint8_t> (zone_t::WIB):
				// Формируем временную зону
				return "WIB";
			// Если временная зона установлена как (Время В Восточной Индонезии)
			case static_cast <uint8_t> (zone_t::WIT):
				// Формируем временную зону
				return "WIT";
			// Если временная зона установлена как (Летнее Время В Центральной Австралии)
			case static_cast <uint8_t> (zone_t::ACDT):
				// Формируем временную зону
				return "ACDT";
			// Если временная зона установлена как (Стандартное Время В Центральной Австралии)
			case static_cast <uint8_t> (zone_t::ACST):
				// Формируем временную зону
				return "ACST";
			// Если временная зона установлена как (Летнее Время В Восточной Австралии)
			case static_cast <uint8_t> (zone_t::AEDT):
				// Формируем временную зону
				return "AEDT";
			// Если временная зона установлена как (Стандартное Время В Восточной Австралии)
			case static_cast <uint8_t> (zone_t::AEST):
				// Формируем временную зону
				return "AEST";
			// Если временная зона установлена как (Летнее Время На Аляске)
			case static_cast <uint8_t> (zone_t::AKDT):
				// Формируем временную зону
				return "AKDT";
			// Если временная зона установлена как (Стандартное Время На Аляске)
			case static_cast <uint8_t> (zone_t::AKST):
				// Формируем временную зону
				return "AKST";
			// Если временная зона установлена как (Амазонка, Летнее Время)
			case static_cast <uint8_t> (zone_t::AMST):
				// Формируем временную зону
				return "AMST";
			// Если временная зона установлена как (Стандартное Время В Западной Австралии)
			case static_cast <uint8_t> (zone_t::AWST):
				// Формируем временную зону
				return "AWST";
			// Если временная зона установлена как (Стандартное Время На Азорских Островах)
			case static_cast <uint8_t> (zone_t::AZOT):
				// Формируем временную зону
				return "AZOT";
			// Если временная зона установлена как (Бразильское Летнее Время)
			case static_cast <uint8_t> (zone_t::BRST):
				// Формируем временную зону
				return "BRST";
			// Если временная зона установлена как (Чилийское Летнее Время)
			case static_cast <uint8_t> (zone_t::CLST):
				// Формируем временную зону
				return "CLST";
			// Если временная зона установлена как (Центральноевропейское Летнее Время)
			case static_cast <uint8_t> (zone_t::CEST):
				// Формируем временную зону
				return "CEST";
			// Если временная зона установлена как (Стандартное Время В Чойлобалсане)
			case static_cast <uint8_t> (zone_t::CHOT):
				// Формируем временную зону
				return "CHOT";
			// Если временная зона установлена как (Час Чаморро)
			case static_cast <uint8_t> (zone_t::CHST):
				// Формируем временную зону
				return "CHST";
			// Если временная зона установлена как (Время На Островах Чуук)
			case static_cast <uint8_t> (zone_t::CHUT):
				// Формируем временную зону
				return "CHUT";
			// Если временная зона установлена как (Колумбийское Летнее Время)
			case static_cast <uint8_t> (zone_t::COST):
				// Формируем временную зону
				return "COST";
			// Если временная зона установлена как (Дейвис)
			case static_cast <uint8_t> (zone_t::DAVT):
				// Формируем временную зону
				return "DAVT";
			// Если временная зона установлена как (Дюмон-Д'юрвиль)
			case static_cast <uint8_t> (zone_t::DDUT):
				// Формируем временную зону
				return "DDUT";
			// Если временная зона установлена как (Летнее Время В Восточной Гренландии)
			case static_cast <uint8_t> (zone_t::EGST):
				// Формируем временную зону
				return "EGST";
			// Если временная зона установлена как (Стандартное Время На Острове Пасхи)
			case static_cast <uint8_t> (zone_t::EAST):
				// Формируем временную зону
				return "EAST";
			// Если временная зона установлена как (Восточноевропейское Летнее Время)
			case static_cast <uint8_t> (zone_t::EEST):
				// Формируем временную зону
				return "EEST";
			// Если временная зона установлена как (Летнее Время На Фолклендах)
			case static_cast <uint8_t> (zone_t::FKST):
				// Формируем временную зону
				return "FKST";
			// Если временная зона установлена как (Время На Острове Гамбье)
			case static_cast <uint8_t> (zone_t::GAMT):
				// Формируем временную зону
				return "GAMT";
			// Если временная зона установлена как (Стандартное Время В Ховде)
			case static_cast <uint8_t> (zone_t::HOVT):
				// Формируем временную зону
				return "HOVT";
			// Если временная зона установлена как (Гавайско-Алеутское Летнее Время)
			case static_cast <uint8_t> (zone_t::HADT):
				// Формируем временную зону
				return "HADT";
			// Если временная зона установлена как (Гавайско-Алеутское Стандартное Время)
			case static_cast <uint8_t> (zone_t::HAST):
				// Формируем временную зону
				return "HAST";
			// Если временная зона установлена как (Иранское Летнее Время)
			case static_cast <uint8_t> (zone_t::IRDT):
				// Формируем временную зону
				return "IRDT";
			// Если временная зона установлена как (Иркутское Стандартное Время)
			case static_cast <uint8_t> (zone_t::IRKT):
				// Формируем временную зону
				return "IRKT";
			// Если временная зона установлена как (Иранское Стандартное Время)
			case static_cast <uint8_t> (zone_t::IRST):
				// Формируем временную зону
				return "IRST";
			// Если временная зона установлена как (Время На Островах Гилберта)
			case static_cast <uint8_t> (zone_t::GILT):
				// Формируем временную зону
				return "GILT";
			// Если временная зона установлена как (Время На Галапагосских Островах)
			case static_cast <uint8_t> (zone_t::GALT):
				// Формируем временную зону
				return "GALT";
			// Если временная зона установлена как (Время На Острове Косраэ)
			case static_cast <uint8_t> (zone_t::KOST):
				// Формируем временную зону
				return "KOST";
			// Если временная зона установлена как (Красноярское Стандартное Время)
			case static_cast <uint8_t> (zone_t::KRAT):
				// Формируем временную зону
				return "KRAT";
			// Если временная зона установлена как (Летнее Время На Лорд-Хау)
			case static_cast <uint8_t> (zone_t::LHDT):
				// Формируем временную зону
				return "LHDT";
			// Если временная зона установлена как (Стандартное Время На Лорд-Хау)
			case static_cast <uint8_t> (zone_t::LHST):
				// Формируем временную зону
				return "LHST";
			// Если временная зона установлена как (Время На Острове Лайн)
			case static_cast <uint8_t> (zone_t::LINT):
				// Формируем временную зону
				return "LINT";
			// Если временная зона установлена как (Магаданское Стандартное Время)
			case static_cast <uint8_t> (zone_t::MAGT):
				// Формируем временную зону
				return "MAGT";
			// Если временная зона установлена как (Время На Маркизских Островах)
			case static_cast <uint8_t> (zone_t::MART):
				// Формируем временную зону
				return "MART";
			// Если временная зона установлена как (Время На Станции Маккуори)
			case static_cast <uint8_t> (zone_t::MIST):
				// Формируем временную зону
				return "MIST";
			// Если временная зона установлена как (Время На Станции Моусон)
			case static_cast <uint8_t> (zone_t::MAWT):
				// Формируем временную зону
				return "MAWT";
			// Если временная зона установлена как (Летнее Время В Новой Зеландии)
			case static_cast <uint8_t> (zone_t::NZDT):
				// Формируем временную зону
				return "NZDT";
			// Если временная зона установлена как (Стандартное Время В Новой Зеландии)
			case static_cast <uint8_t> (zone_t::NZST):
				// Формируем временную зону
				return "NZST";
			// Если временная зона установлена как (Парагвайское Летнее Время)
			case static_cast <uint8_t> (zone_t::PYST):
				// Формируем временную зону
				return "PYST";
			// Если временная зона установлена как (Камчатское Время)
			case static_cast <uint8_t> (zone_t::PETT):
				// Формируем временную зону
				return "PETT";
			// Если временная зона установлена как (Летнее Время На Островах Сен-Пьер И Микелон)
			case static_cast <uint8_t> (zone_t::PMDT):
				// Формируем временную зону
				return "PMDT";
			// Если временная зона установлена как (Стандартное Время На Островах Сен-Пьер И Микелон)
			case static_cast <uint8_t> (zone_t::PMST):
				// Формируем временную зону
				return "PMST";
			// Если временная зона установлена как (Время На Острове Понапе)
			case static_cast <uint8_t> (zone_t::PONT):
				// Формируем временную зону
				return "PONT";
			// Если временная зона установлена как (Время На Островах Феникс)
			case static_cast <uint8_t> (zone_t::PHOT):
				// Формируем временную зону
				return "PHOT";
			// Если временная зона установлена как (Стандартное Время На Филлипинах)
			case static_cast <uint8_t> (zone_t::PhST):
				// Формируем временную зону
				return "PhST";
			// Если временная зона установлена как (Время На Станции Ротера)
			case static_cast <uint8_t> (zone_t::ROTT):
				// Формируем временную зону
				return "ROTT";
			// Если временная зона установлена как (Стандартное Время В Шри-Ланке)
			case static_cast <uint8_t> (zone_t::SLST):
				// Формируем временную зону
				return "SLST";
			// Если временная зона установлена как (Сахалинское Стандартное Время)
			case static_cast <uint8_t> (zone_t::SAKT):
				// Формируем временную зону
				return "SAKT";
			// Если временная зона установлена как (Самарское Время)
			case static_cast <uint8_t> (zone_t::SAMT):
				// Формируем временную зону
				return "SAMT";
			// Если временная зона установлена как (Южноафриканское Время)
			case static_cast <uint8_t> (zone_t::SAST):
				// Формируем временную зону
				return "SAST";
			// Если временная зона установлена как (Время На Станции Сёва)
			case static_cast <uint8_t> (zone_t::SYOT):
				// Формируем временную зону
				return "SYOT";
			// Если временная зона установлена как (Время На Острове Таити)
			case static_cast <uint8_t> (zone_t::TAHT):
				// Формируем временную зону
				return "TAHT";
			// Если временная зона установлена как (Омское Время)
			case static_cast <uint8_t> (zone_t::OMST):
				// Формируем временную зону
				return "OMST";
			// Если временная зона установлена как (Время В Западном Казахстане)
			case static_cast <uint8_t> (zone_t::ORAT):
				// Формируем временную зону
				return "ORAT";
			// Если временная зона установлена как (Владивостокское Время)
			case static_cast <uint8_t> (zone_t::VLAT):
				// Формируем временную зону
				return "VLAT";
			// Если временная зона установлена как (Волгоградское Время)
			case static_cast <uint8_t> (zone_t::VOLT):
				// Формируем временную зону
				return "VOLT";
			// Если временная зона установлена как (Время На Станции Восток)
			case static_cast <uint8_t> (zone_t::VOST):
				// Формируем временную зону
				return "VOST";
			// Если временная зона установлена как (Летнее Время В Уругвае)
			case static_cast <uint8_t> (zone_t::UYST):
				// Формируем временную зону
				return "UYST";
			// Если временная зона установлена как (Стандартное Время В Монголии)
			case static_cast <uint8_t> (zone_t::ULAT):
				// Формируем временную зону
				return "ULAT";
			// Если временная зона установлена как (Калининградское Время)
			case static_cast <uint8_t> (zone_t::USZ1):
				// Формируем временную зону
				return "USZ1";
			// Если временная зона установлена как (Время На Острове Уэйк)
			case static_cast <uint8_t> (zone_t::WAKT):
				// Формируем временную зону
				return "WAKT";
			// Если временная зона установлена как (Западноафриканское Летнее Время)
			case static_cast <uint8_t> (zone_t::WAST):
				// Формируем временную зону
				return "WAST";
			// Если временная зона установлена как (Западноевропейское Летнее Время)
			case static_cast <uint8_t> (zone_t::WEST):
				// Формируем временную зону
				return "WEST";
			// Если временная зона установлена как (Стандартное Время В Западной Гренландии)
			case static_cast <uint8_t> (zone_t::WGST):
				// Формируем временную зону
				return "UTC-3";
			// Если временная зона установлена как (Якутское Время)
			case static_cast <uint8_t> (zone_t::YAKT):
				// Формируем временную зону
				return "YAKT";
			// Если временная зона установлена как (Екатеринбургское Время)
			case static_cast <uint8_t> (zone_t::YEKT):
				// Формируем временную зону
				return "YEKT";
			// Если временная зона установлена как (Центрально-Западная Австралия, Стандартное Время)
			case static_cast <uint8_t> (zone_t::ACWST):
				// Формируем временную зону
				return "ACWST";
			// Если временная зона установлена как (Летнее Время На Азорских Островах)
			case static_cast <uint8_t> (zone_t::AZOST):
				// Формируем временную зону
				return "AZOST";
			// Если временная зона установлена как (Летнее Время На Архипелаге Чатем)
			case static_cast <uint8_t> (zone_t::CHADT):
				// Формируем временную зону
				return "CHADT";
			// Если временная зона установлена как (Стандартное Время На Архипелаге Чатем)
			case static_cast <uint8_t> (zone_t::CHAST):
				// Формируем временную зону
				return "CHAST";
			// Если временная зона установлена как (Летнее Время В Чойлобалсане)
			case static_cast <uint8_t> (zone_t::CHOST):
				// Формируем временную зону
				return "CHOST";
			// Если временная зона установлена как (Летнее Время На Острове Пасхи)
			case static_cast <uint8_t> (zone_t::EASST):
				// Формируем временную зону
				return "EASST";
			// Если временная зона установлена как (Летнее Время В Ховде)
			case static_cast <uint8_t> (zone_t::HOVST):
				// Формируем временную зону
				return "HOVST";
			// Если временная зона установлена как (Летнее Время В Монголии)
			case static_cast <uint8_t> (zone_t::ULAST):
				// Формируем временную зону
				return "ULAST";
			// Если временная зона установлена как (Амазонское Стандартное Время)
			case static_cast <uint8_t> (zone_t::AMTAM):
				// Если инверсия не включена
				return "UTC-4";
			// Если временная зона установлена как (Армянское Стандартное Время)
			case static_cast <uint8_t> (zone_t::AMTAR):
				// Если инверсия не включена
				return "UTC+4";
			// Если временная зона установлена как (Атлантическое Стандартное Время)
			case static_cast <uint8_t> (zone_t::ASTAL):
				// Если инверсия не включена
				return "UTC-4";
			// Если временная зона установлена как (Стандартное Время В Саудовской Аравии)
			case static_cast <uint8_t> (zone_t::ASTSA):
				// Если инверсия не включена
				return "UTC+3";
			// Если временная зона установлена как (Британское Летнее Время)
			case static_cast <uint8_t> (zone_t::BSTBR):
				// Если инверсия не включена
				return "UTC+1";
			// Если временная зона установлена как (Стандартное Время В Бангладеш)
			case static_cast <uint8_t> (zone_t::BSTBL):
				// Если инверсия не включена
				return "UTC+6";
			// Если временная зона установлена как (Северноамериканское Центральное Летнее Время)
			case static_cast <uint8_t> (zone_t::CDTNA):
				// Если инверсия не включена
				return "UTC-5";
			// Если временная зона установлена как (Кубинское Летнее Время)
			case static_cast <uint8_t> (zone_t::CDTCB):
				// Если инверсия не включена
				return "UTC-4";
			// Если временная зона установлена как (Северноамериканское Центральное Стандартное Время)
			case static_cast <uint8_t> (zone_t::CSTNA):
				// Если инверсия не включена
				return "UTC-6";
			// Если временная зона установлена как (Китайское Стандартное Время)
			case static_cast <uint8_t> (zone_t::CSTKT):
				// Если инверсия не включена
				return "UTC+8";
			// Если временная зона установлена как (Кубинское Стандартное Время)
			case static_cast <uint8_t> (zone_t::CSTCB):
				// Если инверсия не включена
				return "UTC-5";
			// Если временная зона установлена как (Время В Персидском Заливе)
			case static_cast <uint8_t> (zone_t::GSTPG):
				// Если инверсия не включена
				return "UTC+4";
			// Если временная зона установлена как (Время В Южной Георгии)
			case static_cast <uint8_t> (zone_t::GSTSG):
				// Если инверсия не включена
				return "UTC-2";
			// Если временная зона установлена как (Индийское Стандартное Время)
			case static_cast <uint8_t> (zone_t::ISTID):
				// Если инверсия не включена
				return "UTC+5:30";
			// Если временная зона установлена как (Ирландия, Летнее Время)
			case static_cast <uint8_t> (zone_t::ISTIR):
				// Если инверсия не включена
				return "UTC+1";
			// Если временная зона установлена как (Израильское Стандартное Время)
			case static_cast <uint8_t> (zone_t::ISTIS):
				// Если инверсия не включена
				return "UTC+2";
			// Если временная зона установлена как (Северноамериканское Горное Стандартное Время)
			case static_cast <uint8_t> (zone_t::MSTNA):
				// Если инверсия не включена
				return "UTC-7";
			// Если временная зона установлена как (Время В Малайзии)
			case static_cast <uint8_t> (zone_t::MSTMS):
				// Если инверсия не включена
				return "UTC+8";
			// Если временная зона установлена как (Летнее Время В Западной Гренландии)
			case static_cast <uint8_t> (zone_t::WGSTST):
				// Если инверсия не включена
				return "UTC-2";
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (zone)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return "UTC";
}
/**
 * @brief Метод формирования объекта даты и времени
 *
 * @param dt     объект даты и времени
 * @param format формат даты
 * @return       строка содержащая дату
 *
 */
string awh::Chrono::format(const dt_t & dt, string_view format) const noexcept {
	// Переменная результата
	string result = "";
	// Если формат даты передан
	if(!format.empty()){
		// Символ для обработки
		char letter = 0;
		// Режим детекции переменной формата
		bool mode = false;
		/**
		 * Выполняем перебор формата
		 */
		for(size_t i = 0; i < format.length(); i++){
			// Получаем символ для обработки
			letter = format[i];
			/**
			 * Определяем символ парсинга
			 */
			switch(letter){
				// Если мы нашли идентификатор переменной
				case '%': mode = true; break;
				// Если мы нашли переменную (y)
				case 'y':
				// Если мы нашли переменную (g)
				case 'g':
				// Если мы нашли переменную (Y)
				case 'Y':
				// Если мы нашли переменную (G)
				case 'G':
				// Если мы нашли переменную (b)
				case 'b':
				// Если мы нашли переменную (h)
				case 'h':
				// Если мы нашли переменную (B)
				case 'B':
				// Если мы нашли переменную (m)
				case 'm':
				// Если мы нашли переменную (d)
				case 'd':
				// Если мы нашли переменную (e)
				case 'e':
				// Если мы нашли переменную (a)
				case 'a':
				// Если мы нашли переменную (A)
				case 'A':
				// Если мы нашли переменную (j)
				case 'j':
				// Если мы нашли переменную (u)
				case 'u':
				// Если мы нашли переменную (U)
				case 'U':
				// Если мы нашли переменную (w)
				case 'w':
				// Если мы нашли переменную (W)
				case 'W':
				// Если мы нашли переменную (D)
				case 'D':
				// Если мы нашли переменную (x)
				case 'x':
				// Если мы нашли переменную (F)
				case 'F':
				// Если мы нашли переменную (H)
				case 'H':
				// Если мы нашли переменную (I)
				case 'I':
				// Если мы нашли переменную (M)
				case 'M':
				// Если мы нашли переменную (s)
				case 's':
				// Если мы нашли переменную (S)
				case 'S':
				// Если мы нашли переменную (p)
				case 'p':
				// Если мы нашли переменную (R)
				case 'R':
				// Если мы нашли переменную (T)
				case 'T':
				// Если мы нашли переменную (X)
				case 'X':
				// Если мы нашли переменную (r)
				case 'r':
				// Если мы нашли переменную (c)
				case 'c':
				// Если мы нашли переменную (o)
				case 'o':
				// Если мы нашли переменную (z)
				case 'z':
				// Если мы нашли переменную (Z)
				case 'Z': {
					// Если мы ищем переменную
					if(mode){
						/**
						 * Определяем символ парсинга
						 */
						switch(letter){
							// Если мы нашли переменную (y)
							case 'y':
							// Если мы нашли переменную (g)
							case 'g':
								// Выполняем формирование номера года
								result.append(std::to_string(dt.year).substr(2));
							break;
							// Если мы нашли переменную (Y)
							case 'Y':
							// Если мы нашли переменную (G)
							case 'G':
								// Выполняем формирование номера года
								result.append(std::to_string(dt.year));
							break;
							// Если мы нашли переменную (b)
							case 'b':
							// Если мы нашли переменную (h)
							case 'h':
								// Выполняем формирование названия месяца
								result.append(params.nameMonths[dt.month - 1].first);
							break;
							// Если мы нашли переменную (B)
							case 'B':
								// Выполняем формирование названия месяца
								result.append(params.nameMonths[dt.month - 1].second);
							break;
							// Если мы нашли переменную (m)
							case 'm': {
								// Получаем номер месяца
								string month = std::to_string(dt.month);
								// Если первого нуля нет
								if(month.length() == 1)
									// Добавляем предстоящий ноль
									month.insert(month.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(month));
							} break;
							// Если мы нашли переменную (d)
							case 'd': {
								// Получаем число месяца
								string date = std::to_string(dt.date);
								// Если первого нуля нет
								if(date.length() == 1)
									// Добавляем предстоящий ноль
									date.insert(date.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(date));
							} break;
							// Если мы нашли переменную (e)
							case 'e':
								// Добавляем полученный результат
								result.append(std::to_string(dt.date));
							break;
							// Если мы нашли переменную (a)
							case 'a':
								// Выполняем формирование названия дня недели
								result.append(params.nameDays[dt.day - 1].first);
							break;
							// Если мы нашли переменную (A)
							case 'A':
								// Выполняем формирование названия дня недели
								result.append(params.nameDays[dt.day - 1].second);
							break;
							// Если мы нашли переменную (u)
							case 'u':
								// Добавляем полученный результат
								result.append(std::to_string(dt.day));
							break;
							// Если мы нашли переменную (w)
							case 'w':
								// Добавляем полученный результат
								result.append(std::to_string(dt.day == 7 ? 0 : dt.day));
							break;
							// Если мы нашли переменную (W)
							case 'W':
							// Если мы нашли переменную (U)
							case 'U': {
								// Получаем количество недель с начала года
								string weeks = std::to_string(dt.weeks);
								// Если первого нуля нет
								if(weeks.length() == 1)
									// Добавляем предстоящий ноль
									weeks.insert(weeks.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(weeks));
							} break;
							// Если мы нашли переменную (j)
							case 'j': {
								// Получаем количество дней с начала года
								string days = std::to_string(dt.days + 1);
								// Если первого нуля нет
								if(days.length() == 1)
									// Добавляем предстоящий ноль
									days.insert(days.begin(), 2, '0');
								// Если второго нуля нет
								else if(days.length() == 2)
									// Добавляем предстоящий ноль
									days.insert(days.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(days));
							} break;
							// Если мы нашли переменную (D)
							case 'D':
							// Если мы нашли переменную (x)
							case 'x': {
								// Получаем номер месяца
								string num = std::to_string(dt.month);
								// Если первого нуля нет
								if(num.length() == 1)
									// Добавляем предстоящий ноль
									num.insert(num.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(num));
								// Добавляем разделитель
								result.append(1, '/');
								// Получаем число месяца
								num = std::to_string(dt.date);
								// Если первого нуля нет
								if(num.length() == 1)
									// Добавляем предстоящий ноль
									num.insert(num.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(num));
								// Добавляем разделитель
								result.append(1, '/');
								// Выполняем формирование номера года
								result.append(std::to_string(dt.year).substr(2));
							} break;
							// Если мы нашли переменную (F)
							case 'F': {
								// Выполняем формирование номера года
								result.append(std::to_string(dt.year));
								// Добавляем разделитель
								result.append(1, '-');
								// Получаем номер месяца
								string num = std::to_string(dt.month);
								// Если первого нуля нет
								if(num.length() == 1)
									// Добавляем предстоящий ноль
									num.insert(num.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(num));
								// Добавляем разделитель
								result.append(1, '-');
								// Получаем число месяца
								num = std::to_string(dt.date);
								// Если первого нуля нет
								if(num.length() == 1)
									// Добавляем предстоящий ноль
									num.insert(num.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(num));
							} break;
							// Если мы нашли переменную (H)
							case 'H': {
								// Получаем час времени
								string hour = std::to_string(dt.hour);
								// Если первого нуля нет
								if(hour.length() == 1)
									// Добавляем предстоящий ноль
									hour.insert(hour.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(hour));
							} break;
							// Если мы нашли переменную (I)
							case 'I': {
								// Преобразуем час в 12-и часовой формат (полночь и полдень обозначаются как 12)
								uint8_t value = (dt.hour % 12);
								// Если час кратен 12, выставляем значение 12
								if(value == 0)
									// Устанавливаем час равный 12
									value = 12;
								// Получаем час времени
								string hour = std::to_string(value);
								// Если первого нуля нет
								if(hour.length() == 1)
									// Добавляем предстоящий ноль
									hour.insert(hour.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(hour));
							} break;
							// Если мы нашли переменную (M)
							case 'M': {
								// Получаем количество минут времени
								string minutes = std::to_string(dt.minutes);
								// Если первого нуля нет
								if(minutes.length() == 1)
									// Добавляем предстоящий ноль
									minutes.insert(minutes.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(minutes));
							} break;
							// Если мы нашли переменную (s)
							case 's': {
								// Получаем количество миллисекунд времени
								string milliseconds = std::to_string(dt.milliseconds);
								// Если первого нуля нет
								if(milliseconds.length() == 1)
									// Добавляем предстоящий ноль
									milliseconds.insert(milliseconds.begin(), 2, '0');
								// Если второго нуля нет
								else if(milliseconds.length() == 2)
									// Добавляем предстоящий ноль
									milliseconds.insert(milliseconds.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(milliseconds));
							} break;
							// Если мы нашли переменную (S)
							case 'S': {
								// Получаем количество секунд времени
								string seconds = std::to_string(dt.seconds);
								// Если первого нуля нет
								if(seconds.length() == 1)
									// Добавляем предстоящий ноль
									seconds.insert(seconds.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(seconds));
							} break;
							// Если мы нашли переменную (p)
							case 'p':
								// Добавляем формат 12-и часового времени
								result.append(dt.h12 == h12_t::AM ? "AM" : "PM");
							break;
							// Если мы нашли переменную (R)
							case 'R': {
								// Получаем час времени
								string num = std::to_string(dt.hour);
								// Если первого нуля нет
								if(num.length() == 1)
									// Добавляем предстоящий ноль
									num.insert(num.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(num));
								// Добавляем разделитель
								result.append(1, ':');
								// Получаем количество минут времени
								num = std::to_string(dt.minutes);
								// Если первого нуля нет
								if(num.length() == 1)
									// Добавляем предстоящий ноль
									num.insert(num.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(num));
							} break;
							// Если мы нашли переменную (T)
							case 'T':
							// Если мы нашли переменную (X)
							case 'X': {
								// Получаем час времени
								string num = std::to_string(dt.hour);
								// Если первого нуля нет
								if(num.length() == 1)
									// Добавляем предстоящий ноль
									num.insert(num.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(num));
								// Добавляем разделитель
								result.append(1, ':');
								// Получаем количество минут времени
								num = std::to_string(dt.minutes);
								// Если первого нуля нет
								if(num.length() == 1)
									// Добавляем предстоящий ноль
									num.insert(num.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(num));
								// Добавляем разделитель
								result.append(1, ':');
								// Получаем количество секунд времени
								num = std::to_string(dt.seconds);
								// Если первого нуля нет
								if(num.length() == 1)
									// Добавляем предстоящий ноль
									num.insert(num.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(num));
							} break;
							// Если мы нашли переменную (r)
							case 'r': {
								// Преобразуем час в 12-и часовой формат (полночь и полдень обозначаются как 12)
								uint8_t value = (dt.hour % 12);
								// Если час кратен 12, выставляем значение 12
								if(value == 0)
									// Устанавливаем час равный 12
									value = 12;
								// Получаем час времени
								string num = std::to_string(value);
								// Если первого нуля нет
								if(num.length() == 1)
									// Добавляем предстоящий ноль
									num.insert(num.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(num));
								// Добавляем разделитель
								result.append(1, ':');
								// Получаем количество минут времени
								num = std::to_string(dt.minutes);
								// Если первого нуля нет
								if(num.length() == 1)
									// Добавляем предстоящий ноль
									num.insert(num.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(num));
								// Добавляем разделитель
								result.append(1, ':');
								// Получаем количество секунд времени
								num = std::to_string(dt.seconds);
								// Если первого нуля нет
								if(num.length() == 1)
									// Добавляем предстоящий ноль
									num.insert(num.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(num));
								// Добавляем разделитель
								result.append(1, ' ');
								// Добавляем формат 12-и часового времени
								result.append(dt.h12 == h12_t::AM ? "AM" : "PM");
							} break;
							// Если мы нашли переменную (c)
							case 'c': {
								// Выполняем формирование названия дня недели
								result.append(params.nameDays[dt.day - 1].first);
								// Добавляем разделитель
								result.append(1, ' ');
								// Выполняем формирование названия месяца
								result.append(params.nameMonths[dt.month - 1].first);
								// Добавляем разделитель
								result.append(1, ' ');
								// Добавляем полученный результат
								result.append(std::to_string(dt.date));
								// Добавляем разделитель
								result.append(1, ' ');
								// Получаем час времени
								string num = std::to_string(dt.hour);
								// Если первого нуля нет
								if(num.length() == 1)
									// Добавляем предстоящий ноль
									num.insert(num.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(num));
								// Добавляем разделитель
								result.append(1, ':');
								// Получаем количество минут времени
								num = std::to_string(dt.minutes);
								// Если первого нуля нет
								if(num.length() == 1)
									// Добавляем предстоящий ноль
									num.insert(num.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(num));
								// Добавляем разделитель
								result.append(1, ':');
								// Получаем количество секунд времени
								num = std::to_string(dt.seconds);
								// Если первого нуля нет
								if(num.length() == 1)
									// Добавляем предстоящий ноль
									num.insert(num.begin(), 1, '0');
								// Добавляем полученный результат
								result.append(::move(num));
								// Добавляем разделитель
								result.append(1, ' ');
								// Выполняем формирование номера года
								result.append(std::to_string(dt.year));
							} break;
							// Если мы нашли переменную (o)
							case 'o': {
								// Если переданная зона больше нуля
								if(dt.offset >= 0)
									// Добавляем плюс
									result.append(1, '+');
								// Добавляем минус
								else result.append(1, '-');
								// Временное значение переменной
								double intpart = 0;
								// Выполняем конвертацию секунд в часы
								const double seconds = (::abs(dt.offset) / 3600.);
								// Выполняем проверку есть ли дробная часть у числа
								if(::modf(seconds, &intpart) == 0) {
									// Получаем количество часов времени
									string num = std::to_string(static_cast <uint32_t> (seconds));
									// Если первого нуля нет
									if(num.length() == 1)
										// Добавляем предстоящий ноль
										num.insert(num.begin(), 1, '0');
									// Добавляем полученный результат
									result.append(::move(num));
									// Добавляем разделитель
									result.append(1, ':');
									// Добавляем конечные нули
									result.append(2, '0');
								// Если мы нашли дробную часть числа
								} else {
									// Добавляем первую часть часа
									result.append(std::to_string(static_cast <uint32_t> (intpart)));
									// Добавляем разделитель
									result.append(1, ':');
									// Добавляем дробную часть часа
									result.append(std::to_string(static_cast <uint32_t> ((seconds - intpart) * 60)));
								}
							} break;
							// Если мы нашли переменную (z)
							case 'z': {
								// Если переданная зона больше нуля
								if(dt.offset >= 0)
									// Добавляем плюс
									result.append(1, '+');
								// Добавляем минус
								else result.append(1, '-');
								// Временное значение переменной
								double intpart = 0;
								// Выполняем конвертацию секунд в часы
								const double seconds = (::abs(dt.offset) / 3600.);
								// Выполняем проверку есть ли дробная часть у числа
								if(::modf(seconds, &intpart) == 0) {
									// Получаем количество часов времени
									string num = std::to_string(static_cast <uint32_t> (seconds));
									// Если первого нуля нет
									if(num.length() == 1)
										// Добавляем предстоящий ноль
										num.insert(num.begin(), 1, '0');
									// Добавляем полученный результат
									result.append(::move(num));
									// Добавляем конечные нули
									result.append(2, '0');
								// Если мы нашли дробную часть числа
								} else {
									// Добавляем первую часть часа
									result.append(std::to_string(static_cast <uint32_t> (intpart)));
									// Добавляем дробную часть часа
									result.append(std::to_string(static_cast <uint32_t> ((seconds - intpart) * 60)));
								}
							} break;
							// Если мы нашли переменную (Z)
							case 'Z': {
								// Если временная зона установлена пустая
								if(dt.zone == zone_t::NONE)
									// Выполняем формирование временной зоны
									result.append(this->format(dt.offset));
								// Возвращаем установленную временную зону
								else result.append(this->format(dt.zone));
							} break;
							// Добавляем полученный символ в результат
							default: result.append(1, letter);
						}
					// Добавляем полученный символ в результат
					} else result.append(1, letter);
					// Сбрасываем режим соответствия переменной формата
					mode = false;
				} break;
				// Если получен любой другой символ
				default: {
					// Сбрасываем режим соответствия переменной формата
					mode = false;
					// Добавляем полученный символ в результат
					result.append(1, letter);
				}
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод формирования UnixTimestamp без учёта временной зоны
 *
 * @param date   дата в UnixTimestamp
 * @param format формат даты
 * @return       строка содержащая дату
 *
 */
string awh::Chrono::format(const uint64_t date, string_view format) const noexcept {
	// Если формат даты передан
	if((date > 0) && !format.empty()){
		// Создаем структуру времени
		dt_t dt;
		// Заполняем объект даты из штампа времени
		this->makeDate(date, dt);
		// Устанавливаем локальную временную зону
		dt.offset = this->getTimeZone();
		// Если смещение выше нуля
		if(dt.offset != 0){
			// Выполняем замену полученной даты
			const_cast <uint64_t &> (date) = this->makeDate(dt);
			// Заполняем объект даты из штампа времени
			this->makeDate(date, dt);
		}
		// Выполняем формирование формата даты
		return this->format(dt, format);
	}
	// Возвращаем результат
	return "";
}
/**
 * @brief Метод формирования UnixTimestamp с учётом временной зоны
 *
 * @param date   дата в UnixTimestamp
 * @param zone   временная зона в которой нужно получить дату (в секундах)
 * @param format формат даты
 * @return       строка содержащая дату
 *
 */
string awh::Chrono::format(const uint64_t date, const int32_t zone, string_view format) const noexcept {
	// Если формат даты передан
	if((date > 0) && !format.empty()){
		// Создаем структуру времени
		dt_t dt;
		// Заполняем объект даты из штампа времени
		this->makeDate(date, dt);
		// Выполняем установку смещения временной зоны
		dt.offset = zone;
		// Если смещение выше нуля
		if(dt.offset != 0){
			// Выполняем замену полученной даты
			const_cast <uint64_t &> (date) = this->makeDate(dt);
			// Заполняем объект даты из штампа времени
			this->makeDate(date, dt);
		}
		// Выполняем формирование формата даты
		return this->format(dt, format);
	}
	// Возвращаем результат
	return "";
}
/**
 * @brief Метод формирования UnixTimestamp с учётом временной зоны
 *
 * @param date   дата в UnixTimestamp
 * @param zone   временная зона в которой нужно получить дату
 * @param format формат даты
 * @return       строка содержащая дату
 *
 */
string awh::Chrono::format(const uint64_t date, const zone_t zone, string_view format) const noexcept {
	// Если формат даты передан
	if((date > 0) && !format.empty()){
		// Создаем структуру времени
		dt_t dt;
		// Заполняем объект даты из штампа времени
		this->makeDate(date, dt);
		// Устанавливаем временную зону
		dt.zone = zone;
		// Выполняем установку смещения временной зоны
		dt.offset = this->getTimeZone(zone);
		// Если смещение выше нуля
		if(dt.offset != 0){
			// Выполняем замену полученной даты
			const_cast <uint64_t &> (date) = this->makeDate(dt);
			// Заполняем объект даты из штампа времени
			this->makeDate(date, dt);
		}
		// Выполняем формирование формата даты
		return this->format(dt, format);
	}
	// Возвращаем результат
	return "";
}
/**
 * @brief Метод формирования UnixTimestamp с учётом временной зоны
 *
 * @param date   дата в UnixTimestamp
 * @param zone   временная зона в которой нужно получить дату
 * @param format формат даты
 * @return       строка содержащая дату
 *
 */
string awh::Chrono::format(const uint64_t date, string_view zone, string_view format) const noexcept {
	// Если формат даты передан
	if((date > 0) && !format.empty()){
		// Создаем структуру времени
		dt_t dt;
		// Заполняем объект даты из штампа времени
		this->makeDate(date, dt);
		// Устанавливаем временную зону
		dt.zone = this->matchTimeZone(zone);
		// Выполняем установку смещения временной зоны
		dt.offset = this->getTimeZone(zone);
		// Если смещение выше нуля
		if(dt.offset != 0){
			// Выполняем замену полученной даты
			const_cast <uint64_t &> (date) = this->makeDate(dt);
			// Заполняем объект даты из штампа времени
			this->makeDate(date, dt);
		}
		// Выполняем формирование формата даты
		return this->format(dt, format);
	}
	// Возвращаем результат
	return "";
}
/**
 * @brief Метод формирования текущей даты без учёта временной зоны
 *
 * @param format  формат даты
 * @param storage хранение значение времени
 * @return        строка содержащая дату
 *
 */
string awh::Chrono::format(string_view format, const storage_t storage) const noexcept {
	// Если формат даты передан
	if(!format.empty()){
		/**
		 * Определяем хранилище значение времени
		 */
		switch(static_cast <uint8_t> (storage)){
			// Если хранилище локальное
			case static_cast <uint8_t> (storage_t::LOCAL): {
				// Создаем структуру времени
				dt_t dt = this->_dt;
				// Если временная зона не установлена
				if((dt.offset == 0) && (dt.zone == zone_t::NONE))
					// Устанавливаем смещение временной зоны по умолчанию
					dt.offset = this->getTimeZone();
				// Заполняем объект даты из штампа времени
				this->makeDate(this->makeDate(dt), dt);
				// Выполняем формирование формата даты
				return this->format(dt, format);
			}
			// Если хранилище глобальное
			case static_cast <uint8_t> (storage_t::GLOBAL): {
				// Создаем структуру времени
				dt_t dt;
				// Заполняем объект даты из штампа времени
				this->makeDate(this->timestamp(type_t::MILLISECONDS), dt);
				// Устанавливаем локальную временную зону
				dt.offset = this->getTimeZone(storage);
				// Если смещение выше нуля
				if(dt.offset != 0)
					// Заполняем объект даты из штампа времени
					this->makeDate(this->makeDate(dt), dt);
				// Выполняем формирование формата даты
				return this->format(dt, format);
			}
		}
	}
	// Возвращаем результат
	return "";
}
/**
 * @brief Метод формирования текущей даты с учётом временной зоны
 *
 * @param zone    временная зона в которой нужно получить дату (в секундах)
 * @param format  формат даты
 * @param storage хранение значение времени
 * @return        строка содержащая дату
 *
 */
string awh::Chrono::format(const int32_t zone, string_view format, const storage_t storage) const noexcept {
	// Если формат даты передан
	if(!format.empty()){
		/**
		 * Определяем хранилище значение времени
		 */
		switch(static_cast <uint8_t> (storage)){
			// Если хранилище локальное
			case static_cast <uint8_t> (storage_t::LOCAL): {
				// Создаем структуру времени
				dt_t dt = this->_dt;
				// Выполняем установку смещения временной зоны
				dt.offset = zone;
				// Если смещение выше нуля
				if(dt.offset != 0)
					// Заполняем объект даты из штампа времени
					this->makeDate(this->makeDate(dt), dt);
				// Выполняем формирование формата даты
				return this->format(dt, format);
			}
			// Если хранилище глобальное
			case static_cast <uint8_t> (storage_t::GLOBAL): {
				// Создаем структуру времени
				dt_t dt;
				// Заполняем объект даты из штампа времени
				this->makeDate(this->timestamp(type_t::MILLISECONDS), dt);
				// Выполняем установку смещения временной зоны
				dt.offset = zone;
				// Если смещение выше нуля
				if(dt.offset != 0)
					// Заполняем объект даты из штампа времени
					this->makeDate(this->makeDate(dt), dt);
				// Выполняем формирование формата даты
				return this->format(dt, format);
			}
		}
	}
	// Возвращаем результат
	return "";
}
/**
 * @brief Метод формирования текущей даты с учётом временной зоны
 *
 * @param zone    временная зона в которой нужно получить дату
 * @param format  формат даты
 * @param storage хранение значение времени
 * @return        строка содержащая дату
 *
 */
string awh::Chrono::format(const zone_t zone, string_view format, const storage_t storage) const noexcept {
	// Если формат даты передан
	if(!format.empty()){
		/**
		 * Определяем хранилище значение времени
		 */
		switch(static_cast <uint8_t> (storage)){
			// Если хранилище локальное
			case static_cast <uint8_t> (storage_t::LOCAL): {
				// Создаем структуру времени
				dt_t dt = this->_dt;
				// Устанавливаем временную зону
				dt.zone = zone;
				// Выполняем установку смещения временной зоны
				dt.offset = this->getTimeZone(zone);
				// Если смещение выше нуля
				if(dt.offset != 0)
					// Заполняем объект даты из штампа времени
					this->makeDate(this->makeDate(dt), dt);
				// Выполняем формирование формата даты
				return this->format(dt, format);
			}
			// Если хранилище глобальное
			case static_cast <uint8_t> (storage_t::GLOBAL): {
				// Создаем структуру времени
				dt_t dt;
				// Заполняем объект даты из штампа времени
				this->makeDate(this->timestamp(type_t::MILLISECONDS), dt);
				// Устанавливаем временную зону
				dt.zone = zone;
				// Выполняем установку смещения временной зоны
				dt.offset = this->getTimeZone(zone);
				// Если смещение выше нуля
				if(dt.offset != 0)
					// Заполняем объект даты из штампа времени
					this->makeDate(this->makeDate(dt), dt);
				// Выполняем формирование формата даты
				return this->format(dt, format);
			}
		}
	}
	// Возвращаем результат
	return "";
}
/**
 * @brief Метод формирования текущей даты с учётом временной зоны
 *
 * @param zone    временная зона в которой нужно получить дату
 * @param format  формат даты
 * @param storage хранение значение времени
 * @return        строка содержащая дату
 *
 */
string awh::Chrono::format(string_view zone, string_view format, const storage_t storage) const noexcept {
	// Если формат даты передан
	if(!format.empty()){
		/**
		 * Определяем хранилище значение времени
		 */
		switch(static_cast <uint8_t> (storage)){
			// Если хранилище локальное
			case static_cast <uint8_t> (storage_t::LOCAL): {
				// Создаем структуру времени
				dt_t dt = this->_dt;
				// Устанавливаем временную зону
				dt.zone = this->matchTimeZone(zone);
				// Выполняем установку смещения временной зоны
				dt.offset = this->getTimeZone(zone);
				// Если смещение выше нуля
				if(dt.offset != 0)
					// Заполняем объект даты из штампа времени
					this->makeDate(this->makeDate(dt), dt);
				// Выполняем формирование формата даты
				return this->format(dt, format);
			}
			// Если хранилище глобальное
			case static_cast <uint8_t> (storage_t::GLOBAL): {
				// Создаем структуру времени
				dt_t dt;
				// Заполняем объект даты из штампа времени
				this->makeDate(this->timestamp(type_t::MILLISECONDS), dt);
				// Устанавливаем временную зону
				dt.zone = this->matchTimeZone(zone);
				// Выполняем установку смещения временной зоны
				dt.offset = this->getTimeZone(zone);
				// Если смещение выше нуля
				if(dt.offset != 0)
					// Заполняем объект даты из штампа времени
					this->makeDate(this->makeDate(dt), dt);
				// Выполняем формирование формата даты
				return this->format(dt, format);
			}
		}
	}
	// Возвращаем результат
	return "";
}
/**
 * @brief Метод преобразования даты из оного формата в другой
 *
 * @param date    строка даты для преобразования
 * @param format1 формат даты из которой нужно получить дату
 * @param format2 формат даты в который нужно перевести дату
 * @param storage хранение значение времени
 * @return        результат работы
 *
 */
string awh::Chrono::strip(string_view date, string_view format1, string_view format2, const storage_t storage) const noexcept {
	// Если данные переданы
	if(!date.empty() && !format1.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем парсинг даты
			const uint64_t stamp = const_cast <chrono_t *> (this)->parse(date, format1, storage);
			// Если штамп времени получен
			if(stamp > 0)
				// Выполняем формирование формата даты и времени
				return this->format(stamp, this->getTimeZone(storage), format2);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(date, format1, format2, static_cast <uint16_t> (storage)), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return "";
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::Chrono::Chrono(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {
	/**
	 * Деактивируем мьютексы на время инициализации
	 */
	this->_mtx.tz.enabled   = false;
	this->_mtx.date.enabled = false;
	// Выполняем инициализацию локального объекта даты и времени
	this->clear();
}
/**
 * @brief Деструктор
 *
 */
awh::Chrono::~Chrono() noexcept {
	/**
	 * Нативные парсеры не требуют освобождения ресурсов
	 */
}
